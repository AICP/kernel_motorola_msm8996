/*
 *  Copyright (C) 2001  MandrakeSoft S.A.
 *
 *    MandrakeSoft S.A.
 *    43, rue d'Aboukir
 *    75002 Paris - France
 *    http://www.linux-mandrake.com/
 *    http://www.mandrakesoft.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Yunhong Jiang <yunhong.jiang@intel.com>
 *  Yaozu (Eddie) Dong <eddie.dong@intel.com>
 *  Based on Xen 3.1 code.
 */

#include <linux/kvm_host.h>
#include <linux/kvm.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/smp.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <asm/processor.h>
#include <asm/page.h>
#include <asm/current.h>

#include "ioapic.h"
#include "lapic.h"
#include "irq.h"

#if 0
#define ioapic_debug(fmt,arg...) printk(KERN_WARNING fmt,##arg)
#else
#define ioapic_debug(fmt, arg...)
#endif
static int ioapic_deliver(struct kvm_ioapic *vioapic, int irq);

static unsigned long ioapic_read_indirect(struct kvm_ioapic *ioapic,
					  unsigned long addr,
					  unsigned long length)
{
	unsigned long result = 0;

	switch (ioapic->ioregsel) {
	case IOAPIC_REG_VERSION:
		result = ((((IOAPIC_NUM_PINS - 1) & 0xff) << 16)
			  | (IOAPIC_VERSION_ID & 0xff));
		break;

	case IOAPIC_REG_APIC_ID:
	case IOAPIC_REG_ARB_ID:
		result = ((ioapic->id & 0xf) << 24);
		break;

	default:
		{
			u32 redir_index = (ioapic->ioregsel - 0x10) >> 1;
			u64 redir_content;

			ASSERT(redir_index < IOAPIC_NUM_PINS);

			redir_content = ioapic->redirtbl[redir_index].bits;
			result = (ioapic->ioregsel & 0x1) ?
			    (redir_content >> 32) & 0xffffffff :
			    redir_content & 0xffffffff;
			break;
		}
	}

	return result;
}

static int ioapic_service(struct kvm_ioapic *ioapic, unsigned int idx)
{
	union kvm_ioapic_redirect_entry *pent;
	int injected = -1;

	pent = &ioapic->redirtbl[idx];

	if (!pent->fields.mask) {
		injected = ioapic_deliver(ioapic, idx);
		if (injected && pent->fields.trig_mode == IOAPIC_LEVEL_TRIG)
			pent->fields.remote_irr = 1;
	}
	if (!pent->fields.trig_mode)
		ioapic->irr &= ~(1 << idx);

	return injected;
}

static void ioapic_write_indirect(struct kvm_ioapic *ioapic, u32 val)
{
	unsigned index;
	bool mask_before, mask_after;

	switch (ioapic->ioregsel) {
	case IOAPIC_REG_VERSION:
		/* Writes are ignored. */
		break;

	case IOAPIC_REG_APIC_ID:
		ioapic->id = (val >> 24) & 0xf;
		break;

	case IOAPIC_REG_ARB_ID:
		break;

	default:
		index = (ioapic->ioregsel - 0x10) >> 1;

		ioapic_debug("change redir index %x val %x\n", index, val);
		if (index >= IOAPIC_NUM_PINS)
			return;
		mask_before = ioapic->redirtbl[index].fields.mask;
		if (ioapic->ioregsel & 1) {
			ioapic->redirtbl[index].bits &= 0xffffffff;
			ioapic->redirtbl[index].bits |= (u64) val << 32;
		} else {
			ioapic->redirtbl[index].bits &= ~0xffffffffULL;
			ioapic->redirtbl[index].bits |= (u32) val;
			ioapic->redirtbl[index].fields.remote_irr = 0;
		}
		mask_after = ioapic->redirtbl[index].fields.mask;
		if (mask_before != mask_after)
			kvm_fire_mask_notifiers(ioapic->kvm, index, mask_after);
		if (ioapic->irr & (1 << index))
			ioapic_service(ioapic, index);
		break;
	}
}

static int ioapic_inj_irq(struct kvm_ioapic *ioapic,
			   struct kvm_vcpu *vcpu,
			   u8 vector, u8 trig_mode, u8 delivery_mode)
{
	ioapic_debug("irq %d trig %d deliv %d\n", vector, trig_mode,
		     delivery_mode);

	ASSERT((delivery_mode == IOAPIC_FIXED) ||
	       (delivery_mode == IOAPIC_LOWEST_PRIORITY));

	return kvm_apic_set_irq(vcpu, vector, trig_mode);
}

static void ioapic_inj_nmi(struct kvm_vcpu *vcpu)
{
	kvm_inject_nmi(vcpu);
	kvm_vcpu_kick(vcpu);
}

static int ioapic_deliver(struct kvm_ioapic *ioapic, int irq)
{
	union kvm_ioapic_redirect_entry entry = ioapic->redirtbl[irq];
	DECLARE_BITMAP(deliver_bitmask, KVM_MAX_VCPUS);
	struct kvm_vcpu *vcpu;
	int vcpu_id, r = -1;

	ioapic_debug("dest=%x dest_mode=%x delivery_mode=%x "
		     "vector=%x trig_mode=%x\n",
		     entry.fields.dest, entry.fields.dest_mode,
		     entry.fields.delivery_mode, entry.fields.vector,
		     entry.fields.trig_mode);

	/* Always delivery PIT interrupt to vcpu 0 */
#ifdef CONFIG_X86
	if (irq == 0) {
                bitmap_zero(deliver_bitmask, KVM_MAX_VCPUS);
		__set_bit(0, deliver_bitmask);
        } else
#endif
		kvm_get_intr_delivery_bitmask(ioapic, &entry, deliver_bitmask);

	if (find_first_bit(deliver_bitmask, KVM_MAX_VCPUS) >= KVM_MAX_VCPUS) {
		ioapic_debug("no target on destination\n");
		return 0;
	}

	while ((vcpu_id = find_first_bit(deliver_bitmask, KVM_MAX_VCPUS))
			< KVM_MAX_VCPUS) {
		__clear_bit(vcpu_id, deliver_bitmask);
		vcpu = ioapic->kvm->vcpus[vcpu_id];
		if (vcpu) {
			if (entry.fields.delivery_mode ==
					IOAPIC_LOWEST_PRIORITY ||
			    entry.fields.delivery_mode == IOAPIC_FIXED) {
				if (r < 0)
					r = 0;
				r += ioapic_inj_irq(ioapic, vcpu,
						    entry.fields.vector,
						    entry.fields.trig_mode,
						    entry.fields.delivery_mode);
			} else if (entry.fields.delivery_mode == IOAPIC_NMI) {
				r = 1;
				ioapic_inj_nmi(vcpu);
			} else
				ioapic_debug("unsupported delivery mode %x!\n",
					     entry.fields.delivery_mode);
		} else
			ioapic_debug("null destination vcpu: "
				     "mask=%x vector=%x delivery_mode=%x\n",
				     entry.fields.deliver_bitmask,
				     entry.fields.vector,
				     entry.fields.delivery_mode);
	}
	return r;
}

int kvm_ioapic_set_irq(struct kvm_ioapic *ioapic, int irq, int level)
{
	u32 old_irr = ioapic->irr;
	u32 mask = 1 << irq;
	union kvm_ioapic_redirect_entry entry;
	int ret = 1;

	if (irq >= 0 && irq < IOAPIC_NUM_PINS) {
		entry = ioapic->redirtbl[irq];
		level ^= entry.fields.polarity;
		if (!level)
			ioapic->irr &= ~mask;
		else {
			ioapic->irr |= mask;
			if ((!entry.fields.trig_mode && old_irr != ioapic->irr)
			    || !entry.fields.remote_irr)
				ret = ioapic_service(ioapic, irq);
		}
	}
	return ret;
}

static void __kvm_ioapic_update_eoi(struct kvm_ioapic *ioapic, int pin,
				    int trigger_mode)
{
	union kvm_ioapic_redirect_entry *ent;

	ent = &ioapic->redirtbl[pin];

	kvm_notify_acked_irq(ioapic->kvm, KVM_IRQCHIP_IOAPIC, pin);

	if (trigger_mode == IOAPIC_LEVEL_TRIG) {
		ASSERT(ent->fields.trig_mode == IOAPIC_LEVEL_TRIG);
		ent->fields.remote_irr = 0;
		if (!ent->fields.mask && (ioapic->irr & (1 << pin)))
			ioapic_service(ioapic, pin);
	}
}

void kvm_ioapic_update_eoi(struct kvm *kvm, int vector, int trigger_mode)
{
	struct kvm_ioapic *ioapic = kvm->arch.vioapic;
	int i;

	for (i = 0; i < IOAPIC_NUM_PINS; i++)
		if (ioapic->redirtbl[i].fields.vector == vector)
			__kvm_ioapic_update_eoi(ioapic, i, trigger_mode);
}

static int ioapic_in_range(struct kvm_io_device *this, gpa_t addr,
			   int len, int is_write)
{
	struct kvm_ioapic *ioapic = (struct kvm_ioapic *)this->private;

	return ((addr >= ioapic->base_address &&
		 (addr < ioapic->base_address + IOAPIC_MEM_LENGTH)));
}

static void ioapic_mmio_read(struct kvm_io_device *this, gpa_t addr, int len,
			     void *val)
{
	struct kvm_ioapic *ioapic = (struct kvm_ioapic *)this->private;
	u32 result;

	ioapic_debug("addr %lx\n", (unsigned long)addr);
	ASSERT(!(addr & 0xf));	/* check alignment */

	addr &= 0xff;
	switch (addr) {
	case IOAPIC_REG_SELECT:
		result = ioapic->ioregsel;
		break;

	case IOAPIC_REG_WINDOW:
		result = ioapic_read_indirect(ioapic, addr, len);
		break;

	default:
		result = 0;
		break;
	}
	switch (len) {
	case 8:
		*(u64 *) val = result;
		break;
	case 1:
	case 2:
	case 4:
		memcpy(val, (char *)&result, len);
		break;
	default:
		printk(KERN_WARNING "ioapic: wrong length %d\n", len);
	}
}

static void ioapic_mmio_write(struct kvm_io_device *this, gpa_t addr, int len,
			      const void *val)
{
	struct kvm_ioapic *ioapic = (struct kvm_ioapic *)this->private;
	u32 data;

	ioapic_debug("ioapic_mmio_write addr=%p len=%d val=%p\n",
		     (void*)addr, len, val);
	ASSERT(!(addr & 0xf));	/* check alignment */
	if (len == 4 || len == 8)
		data = *(u32 *) val;
	else {
		printk(KERN_WARNING "ioapic: Unsupported size %d\n", len);
		return;
	}

	addr &= 0xff;
	switch (addr) {
	case IOAPIC_REG_SELECT:
		ioapic->ioregsel = data;
		break;

	case IOAPIC_REG_WINDOW:
		ioapic_write_indirect(ioapic, data);
		break;
#ifdef	CONFIG_IA64
	case IOAPIC_REG_EOI:
		kvm_ioapic_update_eoi(ioapic->kvm, data, IOAPIC_LEVEL_TRIG);
		break;
#endif

	default:
		break;
	}
}

void kvm_ioapic_reset(struct kvm_ioapic *ioapic)
{
	int i;

	for (i = 0; i < IOAPIC_NUM_PINS; i++)
		ioapic->redirtbl[i].fields.mask = 1;
	ioapic->base_address = IOAPIC_DEFAULT_BASE_ADDRESS;
	ioapic->ioregsel = 0;
	ioapic->irr = 0;
	ioapic->id = 0;
}

int kvm_ioapic_init(struct kvm *kvm)
{
	struct kvm_ioapic *ioapic;

	ioapic = kzalloc(sizeof(struct kvm_ioapic), GFP_KERNEL);
	if (!ioapic)
		return -ENOMEM;
	kvm->arch.vioapic = ioapic;
	kvm_ioapic_reset(ioapic);
	ioapic->dev.read = ioapic_mmio_read;
	ioapic->dev.write = ioapic_mmio_write;
	ioapic->dev.in_range = ioapic_in_range;
	ioapic->dev.private = ioapic;
	ioapic->kvm = kvm;
	kvm_io_bus_register_dev(&kvm->mmio_bus, &ioapic->dev);
	return 0;
}

