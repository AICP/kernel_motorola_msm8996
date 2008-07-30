/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2004-2008 Silicon Graphics, Inc.  All Rights Reserved.
 */

/*
 * Cross Partition Communication (XPC) partition support.
 *
 *	This is the part of XPC that detects the presence/absence of
 *	other partitions. It provides a heartbeat and monitors the
 *	heartbeats of other partitions.
 *
 */

#include <linux/kernel.h>
#include <linux/sysctl.h>
#include <linux/cache.h>
#include <linux/mmzone.h>
#include <linux/nodemask.h>
#include <asm/sn/intr.h>
#include <asm/sn/sn_sal.h>
#include <asm/sn/nodepda.h>
#include <asm/sn/addrs.h>
#include "xpc.h"

/* XPC is exiting flag */
int xpc_exiting;

/* this partition's reserved page pointers */
struct xpc_rsvd_page *xpc_rsvd_page;
static u64 *xpc_part_nasids;
u64 *xpc_mach_nasids;

/* >>> next two variables should be 'xpc_' if they remain here */
static int xp_sizeof_nasid_mask;	/* actual size in bytes of nasid mask */
int xp_nasid_mask_words;	/* actual size in words of nasid mask */

struct xpc_partition *xpc_partitions;

/*
 * Generic buffer used to store a local copy of portions of a remote
 * partition's reserved page (either its header and part_nasids mask,
 * or its vars).
 */
char *xpc_remote_copy_buffer;
void *xpc_remote_copy_buffer_base;

/*
 * Guarantee that the kmalloc'd memory is cacheline aligned.
 */
void *
xpc_kmalloc_cacheline_aligned(size_t size, gfp_t flags, void **base)
{
	/* see if kmalloc will give us cachline aligned memory by default */
	*base = kmalloc(size, flags);
	if (*base == NULL)
		return NULL;

	if ((u64)*base == L1_CACHE_ALIGN((u64)*base))
		return *base;

	kfree(*base);

	/* nope, we'll have to do it ourselves */
	*base = kmalloc(size + L1_CACHE_BYTES, flags);
	if (*base == NULL)
		return NULL;

	return (void *)L1_CACHE_ALIGN((u64)*base);
}

/*
 * Given a nasid, get the physical address of the  partition's reserved page
 * for that nasid. This function returns 0 on any error.
 */
static u64
xpc_get_rsvd_page_pa(int nasid)
{
	enum xp_retval ret;
	s64 status;
	u64 cookie = 0;
	u64 rp_pa = nasid;	/* seed with nasid */
	u64 len = 0;
	u64 buf = buf;
	u64 buf_len = 0;
	void *buf_base = NULL;

	while (1) {

		status = sn_partition_reserved_page_pa(buf, &cookie, &rp_pa,
						       &len);

		dev_dbg(xpc_part, "SAL returned with status=%li, cookie="
			"0x%016lx, address=0x%016lx, len=0x%016lx\n",
			status, cookie, rp_pa, len);

		if (status != SALRET_MORE_PASSES)
			break;

		/* >>> L1_CACHE_ALIGN() is only a sn2-bte_copy requirement */
		if (L1_CACHE_ALIGN(len) > buf_len) {
			kfree(buf_base);
			buf_len = L1_CACHE_ALIGN(len);
			buf = (u64)xpc_kmalloc_cacheline_aligned(buf_len,
								 GFP_KERNEL,
								 &buf_base);
			if (buf_base == NULL) {
				dev_err(xpc_part, "unable to kmalloc "
					"len=0x%016lx\n", buf_len);
				status = SALRET_ERROR;
				break;
			}
		}

		ret = xp_remote_memcpy((void *)buf, (void *)rp_pa, buf_len);
		if (ret != xpSuccess) {
			dev_dbg(xpc_part, "xp_remote_memcpy failed %d\n", ret);
			status = SALRET_ERROR;
			break;
		}
	}

	kfree(buf_base);

	if (status != SALRET_OK)
		rp_pa = 0;

	dev_dbg(xpc_part, "reserved page at phys address 0x%016lx\n", rp_pa);
	return rp_pa;
}

/*
 * Fill the partition reserved page with the information needed by
 * other partitions to discover we are alive and establish initial
 * communications.
 */
struct xpc_rsvd_page *
xpc_setup_rsvd_page(void)
{
	struct xpc_rsvd_page *rp;
	u64 rp_pa;
	unsigned long new_stamp;

	/* get the local reserved page's address */

	preempt_disable();
	rp_pa = xpc_get_rsvd_page_pa(cpuid_to_nasid(smp_processor_id()));
	preempt_enable();
	if (rp_pa == 0) {
		dev_err(xpc_part, "SAL failed to locate the reserved page\n");
		return NULL;
	}
	rp = (struct xpc_rsvd_page *)__va(rp_pa);

	if (rp->SAL_version < 3) {
		/* SAL_versions < 3 had a SAL_partid defined as a u8 */
		rp->SAL_partid &= 0xff;
	}
	BUG_ON(rp->SAL_partid != sn_partition_id);

	if (rp->SAL_partid < 0 || rp->SAL_partid >= xp_max_npartitions) {
		dev_err(xpc_part, "the reserved page's partid of %d is outside "
			"supported range (< 0 || >= %d)\n", rp->SAL_partid,
			xp_max_npartitions);
		return NULL;
	}

	rp->version = XPC_RP_VERSION;
	rp->max_npartitions = xp_max_npartitions;

	/* establish the actual sizes of the nasid masks */
	if (rp->SAL_version == 1) {
		/* SAL_version 1 didn't set the nasids_size field */
		rp->SAL_nasids_size = 128;
	}
	xp_sizeof_nasid_mask = rp->SAL_nasids_size;
	xp_nasid_mask_words = DIV_ROUND_UP(xp_sizeof_nasid_mask,
					   BYTES_PER_WORD);

	/* setup the pointers to the various items in the reserved page */
	xpc_part_nasids = XPC_RP_PART_NASIDS(rp);
	xpc_mach_nasids = XPC_RP_MACH_NASIDS(rp);

	if (xpc_rsvd_page_init(rp) != xpSuccess)
		return NULL;

	/*
	 * Set timestamp of when reserved page was setup by XPC.
	 * This signifies to the remote partition that our reserved
	 * page is initialized.
	 */
	new_stamp = jiffies;
	if (new_stamp == 0 || new_stamp == rp->stamp)
		new_stamp++;
	rp->stamp = new_stamp;

	return rp;
}

/*
 * Get a copy of a portion of the remote partition's rsvd page.
 *
 * remote_rp points to a buffer that is cacheline aligned for BTE copies and
 * is large enough to contain a copy of their reserved page header and
 * part_nasids mask.
 */
enum xp_retval
xpc_get_remote_rp(int nasid, u64 *discovered_nasids,
		  struct xpc_rsvd_page *remote_rp, u64 *remote_rp_pa)
{
	int i;
	enum xp_retval ret;

	/* get the reserved page's physical address */

	*remote_rp_pa = xpc_get_rsvd_page_pa(nasid);
	if (*remote_rp_pa == 0)
		return xpNoRsvdPageAddr;

	/* pull over the reserved page header and part_nasids mask */
	ret = xp_remote_memcpy(remote_rp, (void *)*remote_rp_pa,
			       XPC_RP_HEADER_SIZE + xp_sizeof_nasid_mask);
	if (ret != xpSuccess)
		return ret;

	if (discovered_nasids != NULL) {
		u64 *remote_part_nasids = XPC_RP_PART_NASIDS(remote_rp);

		for (i = 0; i < xp_nasid_mask_words; i++)
			discovered_nasids[i] |= remote_part_nasids[i];
	}

	/* see if the reserved page has been set up by XPC */
	if (remote_rp->stamp == 0)
		return xpRsvdPageNotSet;

	if (XPC_VERSION_MAJOR(remote_rp->version) !=
	    XPC_VERSION_MAJOR(XPC_RP_VERSION)) {
		return xpBadVersion;
	}

	/* check that both local and remote partids are valid for each side */
	if (remote_rp->SAL_partid < 0 ||
	    remote_rp->SAL_partid >= xp_max_npartitions ||
	    remote_rp->max_npartitions <= sn_partition_id) {
		return xpInvalidPartid;
	}

	if (remote_rp->SAL_partid == sn_partition_id)
		return xpLocalPartid;

	return xpSuccess;
}

/*
 * See if the other side has responded to a partition disengage request
 * from us.
 */
int
xpc_partition_disengaged(struct xpc_partition *part)
{
	short partid = XPC_PARTID(part);
	int disengaged;

	disengaged = (xpc_partition_engaged(1UL << partid) == 0);
	if (part->disengage_request_timeout) {
		if (!disengaged) {
			if (time_is_after_jiffies(part->
						  disengage_request_timeout)) {
				/* timelimit hasn't been reached yet */
				return 0;
			}

			/*
			 * Other side hasn't responded to our disengage
			 * request in a timely fashion, so assume it's dead.
			 */

			dev_info(xpc_part, "disengage from remote partition %d "
				 "timed out\n", partid);
			xpc_disengage_request_timedout = 1;
			xpc_clear_partition_engaged(1UL << partid);
			disengaged = 1;
		}
		part->disengage_request_timeout = 0;

		/* cancel the timer function, provided it's not us */
		if (!in_interrupt()) {
			del_singleshot_timer_sync(&part->
						  disengage_request_timer);
		}

		DBUG_ON(part->act_state != XPC_P_DEACTIVATING &&
			part->act_state != XPC_P_INACTIVE);
		if (part->act_state != XPC_P_INACTIVE)
			xpc_wakeup_channel_mgr(part);

		if (XPC_SUPPORTS_DISENGAGE_REQUEST(part->remote_vars_version))
			xpc_cancel_partition_disengage_request(part);
	}
	return disengaged;
}

/*
 * Mark specified partition as active.
 */
enum xp_retval
xpc_mark_partition_active(struct xpc_partition *part)
{
	unsigned long irq_flags;
	enum xp_retval ret;

	dev_dbg(xpc_part, "setting partition %d to ACTIVE\n", XPC_PARTID(part));

	spin_lock_irqsave(&part->act_lock, irq_flags);
	if (part->act_state == XPC_P_ACTIVATING) {
		part->act_state = XPC_P_ACTIVE;
		ret = xpSuccess;
	} else {
		DBUG_ON(part->reason == xpSuccess);
		ret = part->reason;
	}
	spin_unlock_irqrestore(&part->act_lock, irq_flags);

	return ret;
}

/*
 * Notify XPC that the partition is down.
 */
void
xpc_deactivate_partition(const int line, struct xpc_partition *part,
			 enum xp_retval reason)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&part->act_lock, irq_flags);

	if (part->act_state == XPC_P_INACTIVE) {
		XPC_SET_REASON(part, reason, line);
		spin_unlock_irqrestore(&part->act_lock, irq_flags);
		if (reason == xpReactivating) {
			/* we interrupt ourselves to reactivate partition */
			xpc_IPI_send_local_reactivate(part->reactivate_nasid);
		}
		return;
	}
	if (part->act_state == XPC_P_DEACTIVATING) {
		if ((part->reason == xpUnloading && reason != xpUnloading) ||
		    reason == xpReactivating) {
			XPC_SET_REASON(part, reason, line);
		}
		spin_unlock_irqrestore(&part->act_lock, irq_flags);
		return;
	}

	part->act_state = XPC_P_DEACTIVATING;
	XPC_SET_REASON(part, reason, line);

	spin_unlock_irqrestore(&part->act_lock, irq_flags);

	if (XPC_SUPPORTS_DISENGAGE_REQUEST(part->remote_vars_version)) {
		xpc_request_partition_disengage(part);
		xpc_IPI_send_disengage(part);

		/* set a timelimit on the disengage request */
		part->disengage_request_timeout = jiffies +
		    (xpc_disengage_request_timelimit * HZ);
		part->disengage_request_timer.expires =
		    part->disengage_request_timeout;
		add_timer(&part->disengage_request_timer);
	}

	dev_dbg(xpc_part, "bringing partition %d down, reason = %d\n",
		XPC_PARTID(part), reason);

	xpc_partition_going_down(part, reason);
}

/*
 * Mark specified partition as inactive.
 */
void
xpc_mark_partition_inactive(struct xpc_partition *part)
{
	unsigned long irq_flags;

	dev_dbg(xpc_part, "setting partition %d to INACTIVE\n",
		XPC_PARTID(part));

	spin_lock_irqsave(&part->act_lock, irq_flags);
	part->act_state = XPC_P_INACTIVE;
	spin_unlock_irqrestore(&part->act_lock, irq_flags);
	part->remote_rp_pa = 0;
}

/*
 * SAL has provided a partition and machine mask.  The partition mask
 * contains a bit for each even nasid in our partition.  The machine
 * mask contains a bit for each even nasid in the entire machine.
 *
 * Using those two bit arrays, we can determine which nasids are
 * known in the machine.  Each should also have a reserved page
 * initialized if they are available for partitioning.
 */
void
xpc_discovery(void)
{
	void *remote_rp_base;
	struct xpc_rsvd_page *remote_rp;
	u64 remote_rp_pa;
	int region;
	int region_size;
	int max_regions;
	int nasid;
	struct xpc_rsvd_page *rp;
	u64 *discovered_nasids;
	enum xp_retval ret;

	remote_rp = xpc_kmalloc_cacheline_aligned(XPC_RP_HEADER_SIZE +
						  xp_sizeof_nasid_mask,
						  GFP_KERNEL, &remote_rp_base);
	if (remote_rp == NULL)
		return;

	discovered_nasids = kzalloc(sizeof(u64) * xp_nasid_mask_words,
				    GFP_KERNEL);
	if (discovered_nasids == NULL) {
		kfree(remote_rp_base);
		return;
	}

	rp = (struct xpc_rsvd_page *)xpc_rsvd_page;

	/*
	 * The term 'region' in this context refers to the minimum number of
	 * nodes that can comprise an access protection grouping. The access
	 * protection is in regards to memory, IOI and IPI.
	 */
	max_regions = 64;
	region_size = sn_region_size;

	switch (region_size) {
	case 128:
		max_regions *= 2;
	case 64:
		max_regions *= 2;
	case 32:
		max_regions *= 2;
		region_size = 16;
		DBUG_ON(!is_shub2());
	}

	for (region = 0; region < max_regions; region++) {

		if (xpc_exiting)
			break;

		dev_dbg(xpc_part, "searching region %d\n", region);

		for (nasid = (region * region_size * 2);
		     nasid < ((region + 1) * region_size * 2); nasid += 2) {

			if (xpc_exiting)
				break;

			dev_dbg(xpc_part, "checking nasid %d\n", nasid);

			if (XPC_NASID_IN_ARRAY(nasid, xpc_part_nasids)) {
				dev_dbg(xpc_part, "PROM indicates Nasid %d is "
					"part of the local partition; skipping "
					"region\n", nasid);
				break;
			}

			if (!(XPC_NASID_IN_ARRAY(nasid, xpc_mach_nasids))) {
				dev_dbg(xpc_part, "PROM indicates Nasid %d was "
					"not on Numa-Link network at reset\n",
					nasid);
				continue;
			}

			if (XPC_NASID_IN_ARRAY(nasid, discovered_nasids)) {
				dev_dbg(xpc_part, "Nasid %d is part of a "
					"partition which was previously "
					"discovered\n", nasid);
				continue;
			}

			/* pull over the rsvd page header & part_nasids mask */

			ret = xpc_get_remote_rp(nasid, discovered_nasids,
						remote_rp, &remote_rp_pa);
			if (ret != xpSuccess) {
				dev_dbg(xpc_part, "unable to get reserved page "
					"from nasid %d, reason=%d\n", nasid,
					ret);

				if (ret == xpLocalPartid)
					break;

				continue;
			}

			xpc_initiate_partition_activation(remote_rp,
							  remote_rp_pa, nasid);
		}
	}

	kfree(discovered_nasids);
	kfree(remote_rp_base);
}

/*
 * Given a partid, get the nasids owned by that partition from the
 * remote partition's reserved page.
 */
enum xp_retval
xpc_initiate_partid_to_nasids(short partid, void *nasid_mask)
{
	struct xpc_partition *part;
	u64 part_nasid_pa;

	part = &xpc_partitions[partid];
	if (part->remote_rp_pa == 0)
		return xpPartitionDown;

	memset(nasid_mask, 0, XP_NASID_MASK_BYTES);

	part_nasid_pa = (u64)XPC_RP_PART_NASIDS(part->remote_rp_pa);

	return xp_remote_memcpy(nasid_mask, (void *)part_nasid_pa,
				xp_sizeof_nasid_mask);
}
