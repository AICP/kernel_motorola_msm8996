/*
 * apei-base.c - ACPI Platform Error Interface (APEI) supporting
 * infrastructure
 *
 * APEI allows to report errors (for example from the chipset) to the
 * the operating system. This improves NMI handling especially. In
 * addition it supports error serialization and error injection.
 *
 * For more information about APEI, please refer to ACPI Specification
 * version 4.0, chapter 17.
 *
 * This file has Common functions used by more than one APEI table,
 * including framework of interpreter for ERST and EINJ; resource
 * management for APEI registers.
 *
 * Copyright (C) 2009, Intel Corp.
 *	Author: Huang Ying <ying.huang@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/kref.h>
#include <linux/rculist.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <acpi/atomicio.h>

#include "apei-internal.h"

#define APEI_PFX "APEI: "

/*
 * APEI ERST (Error Record Serialization Table) and EINJ (Error
 * INJection) interpreter framework.
 */

#define APEI_EXEC_PRESERVE_REGISTER	0x1

void apei_exec_ctx_init(struct apei_exec_context *ctx,
			struct apei_exec_ins_type *ins_table,
			u32 instructions,
			struct acpi_whea_header *action_table,
			u32 entries)
{
	ctx->ins_table = ins_table;
	ctx->instructions = instructions;
	ctx->action_table = action_table;
	ctx->entries = entries;
}
EXPORT_SYMBOL_GPL(apei_exec_ctx_init);

int __apei_exec_read_register(struct acpi_whea_header *entry, u64 *val)
{
	int rc;

	rc = acpi_atomic_read(val, &entry->register_region);
	if (rc)
		return rc;
	*val >>= entry->register_region.bit_offset;
	*val &= entry->mask;

	return 0;
}

int apei_exec_read_register(struct apei_exec_context *ctx,
			    struct acpi_whea_header *entry)
{
	int rc;
	u64 val = 0;

	rc = __apei_exec_read_register(entry, &val);
	if (rc)
		return rc;
	ctx->value = val;

	return 0;
}
EXPORT_SYMBOL_GPL(apei_exec_read_register);

int apei_exec_read_register_value(struct apei_exec_context *ctx,
				  struct acpi_whea_header *entry)
{
	int rc;

	rc = apei_exec_read_register(ctx, entry);
	if (rc)
		return rc;
	ctx->value = (ctx->value == entry->value);

	return 0;
}
EXPORT_SYMBOL_GPL(apei_exec_read_register_value);

int __apei_exec_write_register(struct acpi_whea_header *entry, u64 val)
{
	int rc;

	val &= entry->mask;
	val <<= entry->register_region.bit_offset;
	if (entry->flags & APEI_EXEC_PRESERVE_REGISTER) {
		u64 valr = 0;
		rc = acpi_atomic_read(&valr, &entry->register_region);
		if (rc)
			return rc;
		valr &= ~(entry->mask << entry->register_region.bit_offset);
		val |= valr;
	}
	rc = acpi_atomic_write(val, &entry->register_region);

	return rc;
}

int apei_exec_write_register(struct apei_exec_context *ctx,
			     struct acpi_whea_header *entry)
{
	return __apei_exec_write_register(entry, ctx->value);
}
EXPORT_SYMBOL_GPL(apei_exec_write_register);

int apei_exec_write_register_value(struct apei_exec_context *ctx,
				   struct acpi_whea_header *entry)
{
	int rc;

	ctx->value = entry->value;
	rc = apei_exec_write_register(ctx, entry);

	return rc;
}
EXPORT_SYMBOL_GPL(apei_exec_write_register_value);

int apei_exec_noop(struct apei_exec_context *ctx,
		   struct acpi_whea_header *entry)
{
	return 0;
}
EXPORT_SYMBOL_GPL(apei_exec_noop);

/*
 * Interpret the specified action. Go through whole action table,
 * execute all instructions belong to the action.
 */
int apei_exec_run(struct apei_exec_context *ctx, u8 action)
{
	int rc;
	u32 i, ip;
	struct acpi_whea_header *entry;
	apei_exec_ins_func_t run;

	ctx->ip = 0;

	/*
	 * "ip" is the instruction pointer of current instruction,
	 * "ctx->ip" specifies the next instruction to executed,
	 * instruction "run" function may change the "ctx->ip" to
	 * implement "goto" semantics.
	 */
rewind:
	ip = 0;
	for (i = 0; i < ctx->entries; i++) {
		entry = &ctx->action_table[i];
		if (entry->action != action)
			continue;
		if (ip == ctx->ip) {
			if (entry->instruction >= ctx->instructions ||
			    !ctx->ins_table[entry->instruction].run) {
				pr_warning(FW_WARN APEI_PFX
			"Invalid action table, unknown instruction type: %d\n",
					   entry->instruction);
				return -EINVAL;
			}
			run = ctx->ins_table[entry->instruction].run;
			rc = run(ctx, entry);
			if (rc < 0)
				return rc;
			else if (rc != APEI_EXEC_SET_IP)
				ctx->ip++;
		}
		ip++;
		if (ctx->ip < ip)
			goto rewind;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(apei_exec_run);

typedef int (*apei_exec_entry_func_t)(struct apei_exec_context *ctx,
				      struct acpi_whea_header *entry,
				      void *data);

static int apei_exec_for_each_entry(struct apei_exec_context *ctx,
				    apei_exec_entry_func_t func,
				    void *data,
				    int *end)
{
	u8 ins;
	int i, rc;
	struct acpi_whea_header *entry;
	struct apei_exec_ins_type *ins_table = ctx->ins_table;

	for (i = 0; i < ctx->entries; i++) {
		entry = ctx->action_table + i;
		ins = entry->instruction;
		if (end)
			*end = i;
		if (ins >= ctx->instructions || !ins_table[ins].run) {
			pr_warning(FW_WARN APEI_PFX
			"Invalid action table, unknown instruction type: %d\n",
				   ins);
			return -EINVAL;
		}
		rc = func(ctx, entry, data);
		if (rc)
			return rc;
	}

	return 0;
}

static int pre_map_gar_callback(struct apei_exec_context *ctx,
				struct acpi_whea_header *entry,
				void *data)
{
	u8 ins = entry->instruction;

	if (ctx->ins_table[ins].flags & APEI_EXEC_INS_ACCESS_REGISTER)
		return acpi_pre_map_gar(&entry->register_region);

	return 0;
}

/*
 * Pre-map all GARs in action table to make it possible to access them
 * in NMI handler.
 */
int apei_exec_pre_map_gars(struct apei_exec_context *ctx)
{
	int rc, end;

	rc = apei_exec_for_each_entry(ctx, pre_map_gar_callback,
				      NULL, &end);
	if (rc) {
		struct apei_exec_context ctx_unmap;
		memcpy(&ctx_unmap, ctx, sizeof(*ctx));
		ctx_unmap.entries = end;
		apei_exec_post_unmap_gars(&ctx_unmap);
	}

	return rc;
}
EXPORT_SYMBOL_GPL(apei_exec_pre_map_gars);

static int post_unmap_gar_callback(struct apei_exec_context *ctx,
				   struct acpi_whea_header *entry,
				   void *data)
{
	u8 ins = entry->instruction;

	if (ctx->ins_table[ins].flags & APEI_EXEC_INS_ACCESS_REGISTER)
		acpi_post_unmap_gar(&entry->register_region);

	return 0;
}

/* Post-unmap all GAR in action table. */
int apei_exec_post_unmap_gars(struct apei_exec_context *ctx)
{
	return apei_exec_for_each_entry(ctx, post_unmap_gar_callback,
					NULL, NULL);
}
EXPORT_SYMBOL_GPL(apei_exec_post_unmap_gars);

/*
 * Resource management for GARs in APEI
 */
struct apei_res {
	struct list_head list;
	unsigned long start;
	unsigned long end;
};

/* Collect all resources requested, to avoid conflict */
struct apei_resources apei_resources_all = {
	.iomem = LIST_HEAD_INIT(apei_resources_all.iomem),
	.ioport = LIST_HEAD_INIT(apei_resources_all.ioport),
};

static int apei_res_add(struct list_head *res_list,
			unsigned long start, unsigned long size)
{
	struct apei_res *res, *resn, *res_ins = NULL;
	unsigned long end = start + size;

	if (end <= start)
		return 0;
repeat:
	list_for_each_entry_safe(res, resn, res_list, list) {
		if (res->start > end || res->end < start)
			continue;
		else if (end <= res->end && start >= res->start) {
			kfree(res_ins);
			return 0;
		}
		list_del(&res->list);
		res->start = start = min(res->start, start);
		res->end = end = max(res->end, end);
		kfree(res_ins);
		res_ins = res;
		goto repeat;
	}

	if (res_ins)
		list_add(&res_ins->list, res_list);
	else {
		res_ins = kmalloc(sizeof(*res), GFP_KERNEL);
		if (!res_ins)
			return -ENOMEM;
		res_ins->start = start;
		res_ins->end = end;
		list_add(&res_ins->list, res_list);
	}

	return 0;
}

static int apei_res_sub(struct list_head *res_list1,
			struct list_head *res_list2)
{
	struct apei_res *res1, *resn1, *res2, *res;
	res1 = list_entry(res_list1->next, struct apei_res, list);
	resn1 = list_entry(res1->list.next, struct apei_res, list);
	while (&res1->list != res_list1) {
		list_for_each_entry(res2, res_list2, list) {
			if (res1->start >= res2->end ||
			    res1->end <= res2->start)
				continue;
			else if (res1->end <= res2->end &&
				 res1->start >= res2->start) {
				list_del(&res1->list);
				kfree(res1);
				break;
			} else if (res1->end > res2->end &&
				   res1->start < res2->start) {
				res = kmalloc(sizeof(*res), GFP_KERNEL);
				if (!res)
					return -ENOMEM;
				res->start = res2->end;
				res->end = res1->end;
				res1->end = res2->start;
				list_add(&res->list, &res1->list);
				resn1 = res;
			} else {
				if (res1->start < res2->start)
					res1->end = res2->start;
				else
					res1->start = res2->end;
			}
		}
		res1 = resn1;
		resn1 = list_entry(resn1->list.next, struct apei_res, list);
	}

	return 0;
}

static void apei_res_clean(struct list_head *res_list)
{
	struct apei_res *res, *resn;

	list_for_each_entry_safe(res, resn, res_list, list) {
		list_del(&res->list);
		kfree(res);
	}
}

void apei_resources_fini(struct apei_resources *resources)
{
	apei_res_clean(&resources->iomem);
	apei_res_clean(&resources->ioport);
}
EXPORT_SYMBOL_GPL(apei_resources_fini);

static int apei_resources_merge(struct apei_resources *resources1,
				struct apei_resources *resources2)
{
	int rc;
	struct apei_res *res;

	list_for_each_entry(res, &resources2->iomem, list) {
		rc = apei_res_add(&resources1->iomem, res->start,
				  res->end - res->start);
		if (rc)
			return rc;
	}
	list_for_each_entry(res, &resources2->ioport, list) {
		rc = apei_res_add(&resources1->ioport, res->start,
				  res->end - res->start);
		if (rc)
			return rc;
	}

	return 0;
}

/*
 * EINJ has two groups of GARs (EINJ table entry and trigger table
 * entry), so common resources are subtracted from the trigger table
 * resources before the second requesting.
 */
int apei_resources_sub(struct apei_resources *resources1,
		       struct apei_resources *resources2)
{
	int rc;

	rc = apei_res_sub(&resources1->iomem, &resources2->iomem);
	if (rc)
		return rc;
	return apei_res_sub(&resources1->ioport, &resources2->ioport);
}
EXPORT_SYMBOL_GPL(apei_resources_sub);

/*
 * IO memory/port rersource management mechanism is used to check
 * whether memory/port area used by GARs conflicts with normal memory
 * or IO memory/port of devices.
 */
int apei_resources_request(struct apei_resources *resources,
			   const char *desc)
{
	struct apei_res *res, *res_bak;
	struct resource *r;

	apei_resources_sub(resources, &apei_resources_all);

	list_for_each_entry(res, &resources->iomem, list) {
		r = request_mem_region(res->start, res->end - res->start,
				       desc);
		if (!r) {
			pr_err(APEI_PFX
		"Can not request iomem region <%016llx-%016llx> for GARs.\n",
			       (unsigned long long)res->start,
			       (unsigned long long)res->end);
			res_bak = res;
			goto err_unmap_iomem;
		}
	}

	list_for_each_entry(res, &resources->ioport, list) {
		r = request_region(res->start, res->end - res->start, desc);
		if (!r) {
			pr_err(APEI_PFX
		"Can not request ioport region <%016llx-%016llx> for GARs.\n",
			       (unsigned long long)res->start,
			       (unsigned long long)res->end);
			res_bak = res;
			goto err_unmap_ioport;
		}
	}

	apei_resources_merge(&apei_resources_all, resources);

	return 0;
err_unmap_ioport:
	list_for_each_entry(res, &resources->ioport, list) {
		if (res == res_bak)
			break;
		release_mem_region(res->start, res->end - res->start);
	}
	res_bak = NULL;
err_unmap_iomem:
	list_for_each_entry(res, &resources->iomem, list) {
		if (res == res_bak)
			break;
		release_region(res->start, res->end - res->start);
	}
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(apei_resources_request);

void apei_resources_release(struct apei_resources *resources)
{
	struct apei_res *res;

	list_for_each_entry(res, &resources->iomem, list)
		release_mem_region(res->start, res->end - res->start);
	list_for_each_entry(res, &resources->ioport, list)
		release_region(res->start, res->end - res->start);

	apei_resources_sub(&apei_resources_all, resources);
}
EXPORT_SYMBOL_GPL(apei_resources_release);

static int apei_check_gar(struct acpi_generic_address *reg, u64 *paddr)
{
	u32 width, space_id;

	width = reg->bit_width;
	space_id = reg->space_id;
	/* Handle possible alignment issues */
	memcpy(paddr, &reg->address, sizeof(*paddr));
	if (!*paddr) {
		pr_warning(FW_BUG APEI_PFX
			   "Invalid physical address in GAR [0x%llx/%u/%u]\n",
			   *paddr, width, space_id);
		return -EINVAL;
	}

	if ((width != 8) && (width != 16) && (width != 32) && (width != 64)) {
		pr_warning(FW_BUG APEI_PFX
			   "Invalid bit width in GAR [0x%llx/%u/%u]\n",
			   *paddr, width, space_id);
		return -EINVAL;
	}

	if (space_id != ACPI_ADR_SPACE_SYSTEM_MEMORY &&
	    space_id != ACPI_ADR_SPACE_SYSTEM_IO) {
		pr_warning(FW_BUG APEI_PFX
			   "Invalid address space type in GAR [0x%llx/%u/%u]\n",
			   *paddr, width, space_id);
		return -EINVAL;
	}

	return 0;
}

static int collect_res_callback(struct apei_exec_context *ctx,
				struct acpi_whea_header *entry,
				void *data)
{
	struct apei_resources *resources = data;
	struct acpi_generic_address *reg = &entry->register_region;
	u8 ins = entry->instruction;
	u64 paddr;
	int rc;

	if (!(ctx->ins_table[ins].flags & APEI_EXEC_INS_ACCESS_REGISTER))
		return 0;

	rc = apei_check_gar(reg, &paddr);
	if (rc)
		return rc;

	switch (reg->space_id) {
	case ACPI_ADR_SPACE_SYSTEM_MEMORY:
		return apei_res_add(&resources->iomem, paddr,
				    reg->bit_width / 8);
	case ACPI_ADR_SPACE_SYSTEM_IO:
		return apei_res_add(&resources->ioport, paddr,
				    reg->bit_width / 8);
	default:
		return -EINVAL;
	}
}

/*
 * Same register may be used by multiple instructions in GARs, so
 * resources are collected before requesting.
 */
int apei_exec_collect_resources(struct apei_exec_context *ctx,
				struct apei_resources *resources)
{
	return apei_exec_for_each_entry(ctx, collect_res_callback,
					resources, NULL);
}
EXPORT_SYMBOL_GPL(apei_exec_collect_resources);

struct dentry *apei_get_debugfs_dir(void)
{
	static struct dentry *dapei;

	if (!dapei)
		dapei = debugfs_create_dir("apei", NULL);

	return dapei;
}
EXPORT_SYMBOL_GPL(apei_get_debugfs_dir);
