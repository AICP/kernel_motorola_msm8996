/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * Copyright 2010-2011 Paul Mackerras, IBM Corp. <paulus@au1.ibm.com>
 */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/kvm.h>
#include <linux/kvm_host.h>
#include <linux/hugetlb.h>
#include <linux/module.h>

#include <asm/tlbflush.h>
#include <asm/kvm_ppc.h>
#include <asm/kvm_book3s.h>
#include <asm/mmu-hash64.h>
#include <asm/hvcall.h>
#include <asm/synch.h>
#include <asm/ppc-opcode.h>

/*
 * Since this file is built in even if KVM is a module, we need
 * a local copy of this function for the case where kvm_main.c is
 * modular.
 */
static struct kvm_memory_slot *builtin_gfn_to_memslot(struct kvm *kvm,
						gfn_t gfn)
{
	struct kvm_memslots *slots;
	struct kvm_memory_slot *memslot;

	slots = kvm_memslots(kvm);
	kvm_for_each_memslot(memslot, slots)
		if (gfn >= memslot->base_gfn &&
		      gfn < memslot->base_gfn + memslot->npages)
			return memslot;
	return NULL;
}

/* Translate address of a vmalloc'd thing to a linear map address */
static void *real_vmalloc_addr(void *x)
{
	unsigned long addr = (unsigned long) x;
	pte_t *p;

	p = find_linux_pte(swapper_pg_dir, addr);
	if (!p || !pte_present(*p))
		return NULL;
	/* assume we don't have huge pages in vmalloc space... */
	addr = (pte_pfn(*p) << PAGE_SHIFT) | (addr & ~PAGE_MASK);
	return __va(addr);
}

/*
 * Add this HPTE into the chain for the real page.
 * Must be called with the chain locked; it unlocks the chain.
 */
void kvmppc_add_revmap_chain(struct kvm *kvm, struct revmap_entry *rev,
			     unsigned long *rmap, long pte_index, int realmode)
{
	struct revmap_entry *head, *tail;
	unsigned long i;

	if (*rmap & KVMPPC_RMAP_PRESENT) {
		i = *rmap & KVMPPC_RMAP_INDEX;
		head = &kvm->arch.revmap[i];
		if (realmode)
			head = real_vmalloc_addr(head);
		tail = &kvm->arch.revmap[head->back];
		if (realmode)
			tail = real_vmalloc_addr(tail);
		rev->forw = i;
		rev->back = head->back;
		tail->forw = pte_index;
		head->back = pte_index;
	} else {
		rev->forw = rev->back = pte_index;
		i = pte_index;
	}
	smp_wmb();
	*rmap = i | KVMPPC_RMAP_REFERENCED | KVMPPC_RMAP_PRESENT; /* unlock */
}
EXPORT_SYMBOL_GPL(kvmppc_add_revmap_chain);

/* Remove this HPTE from the chain for a real page */
static void remove_revmap_chain(struct kvm *kvm, long pte_index,
				unsigned long hpte_v)
{
	struct revmap_entry *rev, *next, *prev;
	unsigned long gfn, ptel, head;
	struct kvm_memory_slot *memslot;
	unsigned long *rmap;

	rev = real_vmalloc_addr(&kvm->arch.revmap[pte_index]);
	ptel = rev->guest_rpte;
	gfn = hpte_rpn(ptel, hpte_page_size(hpte_v, ptel));
	memslot = builtin_gfn_to_memslot(kvm, gfn);
	if (!memslot || (memslot->flags & KVM_MEMSLOT_INVALID))
		return;

	rmap = real_vmalloc_addr(&memslot->rmap[gfn - memslot->base_gfn]);
	lock_rmap(rmap);

	head = *rmap & KVMPPC_RMAP_INDEX;
	next = real_vmalloc_addr(&kvm->arch.revmap[rev->forw]);
	prev = real_vmalloc_addr(&kvm->arch.revmap[rev->back]);
	next->back = rev->back;
	prev->forw = rev->forw;
	if (head == pte_index) {
		head = rev->forw;
		if (head == pte_index)
			*rmap &= ~(KVMPPC_RMAP_PRESENT | KVMPPC_RMAP_INDEX);
		else
			*rmap = (*rmap & ~KVMPPC_RMAP_INDEX) | head;
	}
	unlock_rmap(rmap);
}

static pte_t lookup_linux_pte(struct kvm_vcpu *vcpu, unsigned long hva,
			      int writing, unsigned long *pte_sizep)
{
	pte_t *ptep;
	unsigned long ps = *pte_sizep;
	unsigned int shift;

	ptep = find_linux_pte_or_hugepte(vcpu->arch.pgdir, hva, &shift);
	if (!ptep)
		return __pte(0);
	if (shift)
		*pte_sizep = 1ul << shift;
	else
		*pte_sizep = PAGE_SIZE;
	if (ps > *pte_sizep)
		return __pte(0);
	if (!pte_present(*ptep))
		return __pte(0);
	return kvmppc_read_update_linux_pte(ptep, writing);
}

long kvmppc_h_enter(struct kvm_vcpu *vcpu, unsigned long flags,
		    long pte_index, unsigned long pteh, unsigned long ptel)
{
	struct kvm *kvm = vcpu->kvm;
	unsigned long i, pa, gpa, gfn, psize;
	unsigned long slot_fn, hva;
	unsigned long *hpte;
	struct revmap_entry *rev;
	unsigned long g_ptel = ptel;
	struct kvm_memory_slot *memslot;
	unsigned long *physp, pte_size;
	unsigned long is_io;
	unsigned long *rmap;
	pte_t pte;
	unsigned int writing;
	unsigned long mmu_seq;
	bool realmode = vcpu->arch.vcore->vcore_state == VCORE_RUNNING;

	psize = hpte_page_size(pteh, ptel);
	if (!psize)
		return H_PARAMETER;
	writing = hpte_is_writable(ptel);
	pteh &= ~(HPTE_V_HVLOCK | HPTE_V_ABSENT | HPTE_V_VALID);

	/* used later to detect if we might have been invalidated */
	mmu_seq = kvm->mmu_notifier_seq;
	smp_rmb();

	/* Find the memslot (if any) for this address */
	gpa = (ptel & HPTE_R_RPN) & ~(psize - 1);
	gfn = gpa >> PAGE_SHIFT;
	memslot = builtin_gfn_to_memslot(kvm, gfn);
	pa = 0;
	is_io = ~0ul;
	rmap = NULL;
	if (!(memslot && !(memslot->flags & KVM_MEMSLOT_INVALID))) {
		/* PPC970 can't do emulated MMIO */
		if (!cpu_has_feature(CPU_FTR_ARCH_206))
			return H_PARAMETER;
		/* Emulated MMIO - mark this with key=31 */
		pteh |= HPTE_V_ABSENT;
		ptel |= HPTE_R_KEY_HI | HPTE_R_KEY_LO;
		goto do_insert;
	}

	/* Check if the requested page fits entirely in the memslot. */
	if (!slot_is_aligned(memslot, psize))
		return H_PARAMETER;
	slot_fn = gfn - memslot->base_gfn;
	rmap = &memslot->rmap[slot_fn];

	if (!kvm->arch.using_mmu_notifiers) {
		physp = kvm->arch.slot_phys[memslot->id];
		if (!physp)
			return H_PARAMETER;
		physp += slot_fn;
		if (realmode)
			physp = real_vmalloc_addr(physp);
		pa = *physp;
		if (!pa)
			return H_TOO_HARD;
		is_io = pa & (HPTE_R_I | HPTE_R_W);
		pte_size = PAGE_SIZE << (pa & KVMPPC_PAGE_ORDER_MASK);
		pa &= PAGE_MASK;
	} else {
		/* Translate to host virtual address */
		hva = gfn_to_hva_memslot(memslot, gfn);

		/* Look up the Linux PTE for the backing page */
		pte_size = psize;
		pte = lookup_linux_pte(vcpu, hva, writing, &pte_size);
		if (pte_present(pte)) {
			if (writing && !pte_write(pte))
				/* make the actual HPTE be read-only */
				ptel = hpte_make_readonly(ptel);
			is_io = hpte_cache_bits(pte_val(pte));
			pa = pte_pfn(pte) << PAGE_SHIFT;
		}
	}
	if (pte_size < psize)
		return H_PARAMETER;
	if (pa && pte_size > psize)
		pa |= gpa & (pte_size - 1);

	ptel &= ~(HPTE_R_PP0 - psize);
	ptel |= pa;

	if (pa)
		pteh |= HPTE_V_VALID;
	else
		pteh |= HPTE_V_ABSENT;

	/* Check WIMG */
	if (is_io != ~0ul && !hpte_cache_flags_ok(ptel, is_io)) {
		if (is_io)
			return H_PARAMETER;
		/*
		 * Allow guest to map emulated device memory as
		 * uncacheable, but actually make it cacheable.
		 */
		ptel &= ~(HPTE_R_W|HPTE_R_I|HPTE_R_G);
		ptel |= HPTE_R_M;
	}

	/* Find and lock the HPTEG slot to use */
 do_insert:
	if (pte_index >= HPT_NPTE)
		return H_PARAMETER;
	if (likely((flags & H_EXACT) == 0)) {
		pte_index &= ~7UL;
		hpte = (unsigned long *)(kvm->arch.hpt_virt + (pte_index << 4));
		for (i = 0; i < 8; ++i) {
			if ((*hpte & HPTE_V_VALID) == 0 &&
			    try_lock_hpte(hpte, HPTE_V_HVLOCK | HPTE_V_VALID |
					  HPTE_V_ABSENT))
				break;
			hpte += 2;
		}
		if (i == 8) {
			/*
			 * Since try_lock_hpte doesn't retry (not even stdcx.
			 * failures), it could be that there is a free slot
			 * but we transiently failed to lock it.  Try again,
			 * actually locking each slot and checking it.
			 */
			hpte -= 16;
			for (i = 0; i < 8; ++i) {
				while (!try_lock_hpte(hpte, HPTE_V_HVLOCK))
					cpu_relax();
				if (!(*hpte & (HPTE_V_VALID | HPTE_V_ABSENT)))
					break;
				*hpte &= ~HPTE_V_HVLOCK;
				hpte += 2;
			}
			if (i == 8)
				return H_PTEG_FULL;
		}
		pte_index += i;
	} else {
		hpte = (unsigned long *)(kvm->arch.hpt_virt + (pte_index << 4));
		if (!try_lock_hpte(hpte, HPTE_V_HVLOCK | HPTE_V_VALID |
				   HPTE_V_ABSENT)) {
			/* Lock the slot and check again */
			while (!try_lock_hpte(hpte, HPTE_V_HVLOCK))
				cpu_relax();
			if (*hpte & (HPTE_V_VALID | HPTE_V_ABSENT)) {
				*hpte &= ~HPTE_V_HVLOCK;
				return H_PTEG_FULL;
			}
		}
	}

	/* Save away the guest's idea of the second HPTE dword */
	rev = &kvm->arch.revmap[pte_index];
	if (realmode)
		rev = real_vmalloc_addr(rev);
	if (rev)
		rev->guest_rpte = g_ptel;

	/* Link HPTE into reverse-map chain */
	if (pteh & HPTE_V_VALID) {
		if (realmode)
			rmap = real_vmalloc_addr(rmap);
		lock_rmap(rmap);
		/* Check for pending invalidations under the rmap chain lock */
		if (kvm->arch.using_mmu_notifiers &&
		    mmu_notifier_retry(vcpu, mmu_seq)) {
			/* inval in progress, write a non-present HPTE */
			pteh |= HPTE_V_ABSENT;
			pteh &= ~HPTE_V_VALID;
			unlock_rmap(rmap);
		} else {
			kvmppc_add_revmap_chain(kvm, rev, rmap, pte_index,
						realmode);
		}
	}

	hpte[1] = ptel;

	/* Write the first HPTE dword, unlocking the HPTE and making it valid */
	eieio();
	hpte[0] = pteh;
	asm volatile("ptesync" : : : "memory");

	vcpu->arch.gpr[4] = pte_index;
	return H_SUCCESS;
}
EXPORT_SYMBOL_GPL(kvmppc_h_enter);

#define LOCK_TOKEN	(*(u32 *)(&get_paca()->lock_token))

static inline int try_lock_tlbie(unsigned int *lock)
{
	unsigned int tmp, old;
	unsigned int token = LOCK_TOKEN;

	asm volatile("1:lwarx	%1,0,%2\n"
		     "	cmpwi	cr0,%1,0\n"
		     "	bne	2f\n"
		     "  stwcx.	%3,0,%2\n"
		     "	bne-	1b\n"
		     "  isync\n"
		     "2:"
		     : "=&r" (tmp), "=&r" (old)
		     : "r" (lock), "r" (token)
		     : "cc", "memory");
	return old == 0;
}

long kvmppc_h_remove(struct kvm_vcpu *vcpu, unsigned long flags,
		     unsigned long pte_index, unsigned long avpn,
		     unsigned long va)
{
	struct kvm *kvm = vcpu->kvm;
	unsigned long *hpte;
	unsigned long v, r, rb;

	if (pte_index >= HPT_NPTE)
		return H_PARAMETER;
	hpte = (unsigned long *)(kvm->arch.hpt_virt + (pte_index << 4));
	while (!try_lock_hpte(hpte, HPTE_V_HVLOCK))
		cpu_relax();
	if ((hpte[0] & (HPTE_V_ABSENT | HPTE_V_VALID)) == 0 ||
	    ((flags & H_AVPN) && (hpte[0] & ~0x7fUL) != avpn) ||
	    ((flags & H_ANDCOND) && (hpte[0] & avpn) != 0)) {
		hpte[0] &= ~HPTE_V_HVLOCK;
		return H_NOT_FOUND;
	}
	if (atomic_read(&kvm->online_vcpus) == 1)
		flags |= H_LOCAL;
	vcpu->arch.gpr[4] = v = hpte[0] & ~HPTE_V_HVLOCK;
	vcpu->arch.gpr[5] = r = hpte[1];
	rb = compute_tlbie_rb(v, r, pte_index);
	if (v & HPTE_V_VALID)
		remove_revmap_chain(kvm, pte_index, v);
	smp_wmb();
	hpte[0] = 0;
	if (!(v & HPTE_V_VALID))
		return H_SUCCESS;
	if (!(flags & H_LOCAL)) {
		while (!try_lock_tlbie(&kvm->arch.tlbie_lock))
			cpu_relax();
		asm volatile("ptesync" : : : "memory");
		asm volatile(PPC_TLBIE(%1,%0)"; eieio; tlbsync"
			     : : "r" (rb), "r" (kvm->arch.lpid));
		asm volatile("ptesync" : : : "memory");
		kvm->arch.tlbie_lock = 0;
	} else {
		asm volatile("ptesync" : : : "memory");
		asm volatile("tlbiel %0" : : "r" (rb));
		asm volatile("ptesync" : : : "memory");
	}
	return H_SUCCESS;
}

long kvmppc_h_bulk_remove(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;
	unsigned long *args = &vcpu->arch.gpr[4];
	unsigned long *hp, tlbrb[4];
	long int i, found;
	long int n_inval = 0;
	unsigned long flags, req, pte_index;
	long int local = 0;
	long int ret = H_SUCCESS;

	if (atomic_read(&kvm->online_vcpus) == 1)
		local = 1;
	for (i = 0; i < 4; ++i) {
		pte_index = args[i * 2];
		flags = pte_index >> 56;
		pte_index &= ((1ul << 56) - 1);
		req = flags >> 6;
		flags &= 3;
		if (req == 3)
			break;
		if (req != 1 || flags == 3 ||
		    pte_index >= HPT_NPTE) {
			/* parameter error */
			args[i * 2] = ((0xa0 | flags) << 56) + pte_index;
			ret = H_PARAMETER;
			break;
		}
		hp = (unsigned long *)(kvm->arch.hpt_virt + (pte_index << 4));
		while (!try_lock_hpte(hp, HPTE_V_HVLOCK))
			cpu_relax();
		found = 0;
		if (hp[0] & (HPTE_V_ABSENT | HPTE_V_VALID)) {
			switch (flags & 3) {
			case 0:		/* absolute */
				found = 1;
				break;
			case 1:		/* andcond */
				if (!(hp[0] & args[i * 2 + 1]))
					found = 1;
				break;
			case 2:		/* AVPN */
				if ((hp[0] & ~0x7fUL) == args[i * 2 + 1])
					found = 1;
				break;
			}
		}
		if (!found) {
			hp[0] &= ~HPTE_V_HVLOCK;
			args[i * 2] = ((0x90 | flags) << 56) + pte_index;
			continue;
		}
		/* insert R and C bits from PTE */
		flags |= (hp[1] >> 5) & 0x0c;
		args[i * 2] = ((0x80 | flags) << 56) + pte_index;
		if (hp[0] & HPTE_V_VALID) {
			tlbrb[n_inval++] = compute_tlbie_rb(hp[0], hp[1], pte_index);
			remove_revmap_chain(kvm, pte_index, hp[0]);
		}
		smp_wmb();
		hp[0] = 0;
	}
	if (n_inval == 0)
		return ret;

	if (!local) {
		while(!try_lock_tlbie(&kvm->arch.tlbie_lock))
			cpu_relax();
		asm volatile("ptesync" : : : "memory");
		for (i = 0; i < n_inval; ++i)
			asm volatile(PPC_TLBIE(%1,%0)
				     : : "r" (tlbrb[i]), "r" (kvm->arch.lpid));
		asm volatile("eieio; tlbsync; ptesync" : : : "memory");
		kvm->arch.tlbie_lock = 0;
	} else {
		asm volatile("ptesync" : : : "memory");
		for (i = 0; i < n_inval; ++i)
			asm volatile("tlbiel %0" : : "r" (tlbrb[i]));
		asm volatile("ptesync" : : : "memory");
	}
	return ret;
}

long kvmppc_h_protect(struct kvm_vcpu *vcpu, unsigned long flags,
		      unsigned long pte_index, unsigned long avpn,
		      unsigned long va)
{
	struct kvm *kvm = vcpu->kvm;
	unsigned long *hpte;
	struct revmap_entry *rev;
	unsigned long v, r, rb, mask, bits;

	if (pte_index >= HPT_NPTE)
		return H_PARAMETER;

	hpte = (unsigned long *)(kvm->arch.hpt_virt + (pte_index << 4));
	while (!try_lock_hpte(hpte, HPTE_V_HVLOCK))
		cpu_relax();
	if ((hpte[0] & (HPTE_V_ABSENT | HPTE_V_VALID)) == 0 ||
	    ((flags & H_AVPN) && (hpte[0] & ~0x7fUL) != avpn)) {
		hpte[0] &= ~HPTE_V_HVLOCK;
		return H_NOT_FOUND;
	}

	if (atomic_read(&kvm->online_vcpus) == 1)
		flags |= H_LOCAL;
	v = hpte[0];
	bits = (flags << 55) & HPTE_R_PP0;
	bits |= (flags << 48) & HPTE_R_KEY_HI;
	bits |= flags & (HPTE_R_PP | HPTE_R_N | HPTE_R_KEY_LO);

	/* Update guest view of 2nd HPTE dword */
	mask = HPTE_R_PP0 | HPTE_R_PP | HPTE_R_N |
		HPTE_R_KEY_HI | HPTE_R_KEY_LO;
	rev = real_vmalloc_addr(&kvm->arch.revmap[pte_index]);
	if (rev) {
		r = (rev->guest_rpte & ~mask) | bits;
		rev->guest_rpte = r;
	}
	r = (hpte[1] & ~mask) | bits;

	/* Update HPTE */
	if (v & HPTE_V_VALID) {
		rb = compute_tlbie_rb(v, r, pte_index);
		hpte[0] = v & ~HPTE_V_VALID;
		if (!(flags & H_LOCAL)) {
			while(!try_lock_tlbie(&kvm->arch.tlbie_lock))
				cpu_relax();
			asm volatile("ptesync" : : : "memory");
			asm volatile(PPC_TLBIE(%1,%0)"; eieio; tlbsync"
				     : : "r" (rb), "r" (kvm->arch.lpid));
			asm volatile("ptesync" : : : "memory");
			kvm->arch.tlbie_lock = 0;
		} else {
			asm volatile("ptesync" : : : "memory");
			asm volatile("tlbiel %0" : : "r" (rb));
			asm volatile("ptesync" : : : "memory");
		}
	}
	hpte[1] = r;
	eieio();
	hpte[0] = v & ~HPTE_V_HVLOCK;
	asm volatile("ptesync" : : : "memory");
	return H_SUCCESS;
}

long kvmppc_h_read(struct kvm_vcpu *vcpu, unsigned long flags,
		   unsigned long pte_index)
{
	struct kvm *kvm = vcpu->kvm;
	unsigned long *hpte, v, r;
	int i, n = 1;
	struct revmap_entry *rev = NULL;

	if (pte_index >= HPT_NPTE)
		return H_PARAMETER;
	if (flags & H_READ_4) {
		pte_index &= ~3;
		n = 4;
	}
	if (flags & H_R_XLATE)
		rev = real_vmalloc_addr(&kvm->arch.revmap[pte_index]);
	for (i = 0; i < n; ++i, ++pte_index) {
		hpte = (unsigned long *)(kvm->arch.hpt_virt + (pte_index << 4));
		v = hpte[0] & ~HPTE_V_HVLOCK;
		r = hpte[1];
		if (v & HPTE_V_ABSENT) {
			v &= ~HPTE_V_ABSENT;
			v |= HPTE_V_VALID;
		}
		if (v & HPTE_V_VALID) {
			if (rev)
				r = rev[i].guest_rpte;
			else
				r = hpte[1] | HPTE_R_RPN;
		}
		vcpu->arch.gpr[4 + i * 2] = v;
		vcpu->arch.gpr[5 + i * 2] = r;
	}
	return H_SUCCESS;
}

void kvmppc_invalidate_hpte(struct kvm *kvm, unsigned long *hptep,
			unsigned long pte_index)
{
	unsigned long rb;

	hptep[0] &= ~HPTE_V_VALID;
	rb = compute_tlbie_rb(hptep[0], hptep[1], pte_index);
	while (!try_lock_tlbie(&kvm->arch.tlbie_lock))
		cpu_relax();
	asm volatile("ptesync" : : : "memory");
	asm volatile(PPC_TLBIE(%1,%0)"; eieio; tlbsync"
		     : : "r" (rb), "r" (kvm->arch.lpid));
	asm volatile("ptesync" : : : "memory");
	kvm->arch.tlbie_lock = 0;
}
EXPORT_SYMBOL_GPL(kvmppc_invalidate_hpte);

static int slb_base_page_shift[4] = {
	24,	/* 16M */
	16,	/* 64k */
	34,	/* 16G */
	20,	/* 1M, unsupported */
};

long kvmppc_hv_find_lock_hpte(struct kvm *kvm, gva_t eaddr, unsigned long slb_v,
			      unsigned long valid)
{
	unsigned int i;
	unsigned int pshift;
	unsigned long somask;
	unsigned long vsid, hash;
	unsigned long avpn;
	unsigned long *hpte;
	unsigned long mask, val;
	unsigned long v, r;

	/* Get page shift, work out hash and AVPN etc. */
	mask = SLB_VSID_B | HPTE_V_AVPN | HPTE_V_SECONDARY;
	val = 0;
	pshift = 12;
	if (slb_v & SLB_VSID_L) {
		mask |= HPTE_V_LARGE;
		val |= HPTE_V_LARGE;
		pshift = slb_base_page_shift[(slb_v & SLB_VSID_LP) >> 4];
	}
	if (slb_v & SLB_VSID_B_1T) {
		somask = (1UL << 40) - 1;
		vsid = (slb_v & ~SLB_VSID_B) >> SLB_VSID_SHIFT_1T;
		vsid ^= vsid << 25;
	} else {
		somask = (1UL << 28) - 1;
		vsid = (slb_v & ~SLB_VSID_B) >> SLB_VSID_SHIFT;
	}
	hash = (vsid ^ ((eaddr & somask) >> pshift)) & HPT_HASH_MASK;
	avpn = slb_v & ~(somask >> 16);	/* also includes B */
	avpn |= (eaddr & somask) >> 16;

	if (pshift >= 24)
		avpn &= ~((1UL << (pshift - 16)) - 1);
	else
		avpn &= ~0x7fUL;
	val |= avpn;

	for (;;) {
		hpte = (unsigned long *)(kvm->arch.hpt_virt + (hash << 7));

		for (i = 0; i < 16; i += 2) {
			/* Read the PTE racily */
			v = hpte[i] & ~HPTE_V_HVLOCK;

			/* Check valid/absent, hash, segment size and AVPN */
			if (!(v & valid) || (v & mask) != val)
				continue;

			/* Lock the PTE and read it under the lock */
			while (!try_lock_hpte(&hpte[i], HPTE_V_HVLOCK))
				cpu_relax();
			v = hpte[i] & ~HPTE_V_HVLOCK;
			r = hpte[i+1];

			/*
			 * Check the HPTE again, including large page size
			 * Since we don't currently allow any MPSS (mixed
			 * page-size segment) page sizes, it is sufficient
			 * to check against the actual page size.
			 */
			if ((v & valid) && (v & mask) == val &&
			    hpte_page_size(v, r) == (1ul << pshift))
				/* Return with the HPTE still locked */
				return (hash << 3) + (i >> 1);

			/* Unlock and move on */
			hpte[i] = v;
		}

		if (val & HPTE_V_SECONDARY)
			break;
		val |= HPTE_V_SECONDARY;
		hash = hash ^ HPT_HASH_MASK;
	}
	return -1;
}
EXPORT_SYMBOL(kvmppc_hv_find_lock_hpte);

/*
 * Called in real mode to check whether an HPTE not found fault
 * is due to accessing a paged-out page or an emulated MMIO page,
 * or if a protection fault is due to accessing a page that the
 * guest wanted read/write access to but which we made read-only.
 * Returns a possibly modified status (DSISR) value if not
 * (i.e. pass the interrupt to the guest),
 * -1 to pass the fault up to host kernel mode code, -2 to do that
 * and also load the instruction word (for MMIO emulation),
 * or 0 if we should make the guest retry the access.
 */
long kvmppc_hpte_hv_fault(struct kvm_vcpu *vcpu, unsigned long addr,
			  unsigned long slb_v, unsigned int status, bool data)
{
	struct kvm *kvm = vcpu->kvm;
	long int index;
	unsigned long v, r, gr;
	unsigned long *hpte;
	unsigned long valid;
	struct revmap_entry *rev;
	unsigned long pp, key;

	/* For protection fault, expect to find a valid HPTE */
	valid = HPTE_V_VALID;
	if (status & DSISR_NOHPTE)
		valid |= HPTE_V_ABSENT;

	index = kvmppc_hv_find_lock_hpte(kvm, addr, slb_v, valid);
	if (index < 0) {
		if (status & DSISR_NOHPTE)
			return status;	/* there really was no HPTE */
		return 0;		/* for prot fault, HPTE disappeared */
	}
	hpte = (unsigned long *)(kvm->arch.hpt_virt + (index << 4));
	v = hpte[0] & ~HPTE_V_HVLOCK;
	r = hpte[1];
	rev = real_vmalloc_addr(&kvm->arch.revmap[index]);
	gr = rev->guest_rpte;

	/* Unlock the HPTE */
	asm volatile("lwsync" : : : "memory");
	hpte[0] = v;

	/* For not found, if the HPTE is valid by now, retry the instruction */
	if ((status & DSISR_NOHPTE) && (v & HPTE_V_VALID))
		return 0;

	/* Check access permissions to the page */
	pp = gr & (HPTE_R_PP0 | HPTE_R_PP);
	key = (vcpu->arch.shregs.msr & MSR_PR) ? SLB_VSID_KP : SLB_VSID_KS;
	status &= ~DSISR_NOHPTE;	/* DSISR_NOHPTE == SRR1_ISI_NOPT */
	if (!data) {
		if (gr & (HPTE_R_N | HPTE_R_G))
			return status | SRR1_ISI_N_OR_G;
		if (!hpte_read_permission(pp, slb_v & key))
			return status | SRR1_ISI_PROT;
	} else if (status & DSISR_ISSTORE) {
		/* check write permission */
		if (!hpte_write_permission(pp, slb_v & key))
			return status | DSISR_PROTFAULT;
	} else {
		if (!hpte_read_permission(pp, slb_v & key))
			return status | DSISR_PROTFAULT;
	}

	/* Check storage key, if applicable */
	if (data && (vcpu->arch.shregs.msr & MSR_DR)) {
		unsigned int perm = hpte_get_skey_perm(gr, vcpu->arch.amr);
		if (status & DSISR_ISSTORE)
			perm >>= 1;
		if (perm & 1)
			return status | DSISR_KEYFAULT;
	}

	/* Save HPTE info for virtual-mode handler */
	vcpu->arch.pgfault_addr = addr;
	vcpu->arch.pgfault_index = index;
	vcpu->arch.pgfault_hpte[0] = v;
	vcpu->arch.pgfault_hpte[1] = r;

	/* Check the storage key to see if it is possibly emulated MMIO */
	if (data && (vcpu->arch.shregs.msr & MSR_IR) &&
	    (r & (HPTE_R_KEY_HI | HPTE_R_KEY_LO)) ==
	    (HPTE_R_KEY_HI | HPTE_R_KEY_LO))
		return -2;	/* MMIO emulation - load instr word */

	return -1;		/* send fault up to host kernel mode */
}
