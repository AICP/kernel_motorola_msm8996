#ifndef _ASM_X86_IPI_H
#define _ASM_X86_IPI_H

#ifdef CONFIG_X86_LOCAL_APIC

/*
 * Copyright 2004 James Cleverdon, IBM.
 * Subject to the GNU Public License, v.2
 *
 * Generic APIC InterProcessor Interrupt code.
 *
 * Moved to include file by James Cleverdon from
 * arch/x86-64/kernel/smp.c
 *
 * Copyrights from kernel/smp.c:
 *
 * (c) 1995 Alan Cox, Building #3 <alan@redhat.com>
 * (c) 1998-99, 2000 Ingo Molnar <mingo@redhat.com>
 * (c) 2002,2003 Andi Kleen, SuSE Labs.
 * Subject to the GNU Public License, v.2
 */

#include <asm/hw_irq.h>
#include <asm/apic.h>
#include <asm/smp.h>

/*
 * the following functions deal with sending IPIs between CPUs.
 *
 * We use 'broadcast', CPU->CPU IPIs and self-IPIs too.
 */

static inline unsigned int __prepare_ICR(unsigned int shortcut, int vector,
					 unsigned int dest)
{
	unsigned int icr = shortcut | dest;

	switch (vector) {
	default:
		icr |= APIC_DM_FIXED | vector;
		break;
	case NMI_VECTOR:
		icr |= APIC_DM_NMI;
		break;
	}
	return icr;
}

static inline int __prepare_ICR2(unsigned int mask)
{
	return SET_APIC_DEST_FIELD(mask);
}

static inline void __xapic_wait_icr_idle(void)
{
	while (native_apic_mem_read(APIC_ICR) & APIC_ICR_BUSY)
		cpu_relax();
}

static inline void
__default_send_IPI_shortcut(unsigned int shortcut, int vector, unsigned int dest)
{
	/*
	 * Subtle. In the case of the 'never do double writes' workaround
	 * we have to lock out interrupts to be safe.  As we don't care
	 * of the value read we use an atomic rmw access to avoid costly
	 * cli/sti.  Otherwise we use an even cheaper single atomic write
	 * to the APIC.
	 */
	unsigned int cfg;

	/*
	 * Wait for idle.
	 */
	__xapic_wait_icr_idle();

	/*
	 * No need to touch the target chip field
	 */
	cfg = __prepare_ICR(shortcut, vector, dest);

	/*
	 * Send the IPI. The write to APIC_ICR fires this off.
	 */
	native_apic_mem_write(APIC_ICR, cfg);
}

/*
 * This is used to send an IPI with no shorthand notation (the destination is
 * specified in bits 56 to 63 of the ICR).
 */
static inline void
 __default_send_IPI_dest_field(unsigned int mask, int vector, unsigned int dest)
{
	unsigned long cfg;

	/*
	 * Wait for idle.
	 */
	if (unlikely(vector == NMI_VECTOR))
		safe_apic_wait_icr_idle();
	else
		__xapic_wait_icr_idle();

	/*
	 * prepare target chip field
	 */
	cfg = __prepare_ICR2(mask);
	native_apic_mem_write(APIC_ICR2, cfg);

	/*
	 * program the ICR
	 */
	cfg = __prepare_ICR(0, vector, dest);

	/*
	 * Send the IPI. The write to APIC_ICR fires this off.
	 */
	native_apic_mem_write(APIC_ICR, cfg);
}

static inline void
default_send_IPI_mask_sequence(const struct cpumask *mask, int vector)
{
	unsigned long query_cpu;
	unsigned long flags;

	/*
	 * Hack. The clustered APIC addressing mode doesn't allow us to send
	 * to an arbitrary mask, so I do a unicast to each CPU instead.
	 * - mbligh
	 */
	local_irq_save(flags);
	for_each_cpu(query_cpu, mask) {
		__default_send_IPI_dest_field(per_cpu(x86_cpu_to_apicid,
				query_cpu), vector, APIC_DEST_PHYSICAL);
	}
	local_irq_restore(flags);
}

static inline void
default_send_IPI_mask_allbutself(const struct cpumask *mask, int vector)
{
	unsigned int this_cpu = smp_processor_id();
	unsigned int query_cpu;
	unsigned long flags;

	/* See Hack comment above */

	local_irq_save(flags);
	for_each_cpu(query_cpu, mask) {
		if (query_cpu == this_cpu)
			continue;
		__default_send_IPI_dest_field(per_cpu(x86_cpu_to_apicid,
				 query_cpu), vector, APIC_DEST_PHYSICAL);
	}
	local_irq_restore(flags);
}


/* Avoid include hell */
#define NMI_VECTOR 0x02

void default_send_IPI_mask_bitmask(const struct cpumask *mask, int vector);
void default_send_IPI_mask_allbutself(const struct cpumask *mask, int vector);

extern int no_broadcast;

#ifdef CONFIG_X86_64
#include <asm/genapic.h>
#else
static inline void default_send_IPI_mask(const struct cpumask *mask, int vector)
{
	default_send_IPI_mask_bitmask(mask, vector);
}
void default_send_IPI_mask_allbutself(const struct cpumask *mask, int vector);
#endif

static inline void __default_local_send_IPI_allbutself(int vector)
{
	if (no_broadcast || vector == NMI_VECTOR)
		apic->send_IPI_mask_allbutself(cpu_online_mask, vector);
	else
		__default_send_IPI_shortcut(APIC_DEST_ALLBUT, vector, apic->dest_logical);
}

static inline void __default_local_send_IPI_all(int vector)
{
	if (no_broadcast || vector == NMI_VECTOR)
		apic->send_IPI_mask(cpu_online_mask, vector);
	else
		__default_send_IPI_shortcut(APIC_DEST_ALLINC, vector, apic->dest_logical);
}

#ifdef CONFIG_X86_32
static inline void default_send_IPI_allbutself(int vector)
{
	/*
	 * if there are no other CPUs in the system then we get an APIC send 
	 * error if we try to broadcast, thus avoid sending IPIs in this case.
	 */
	if (!(num_online_cpus() > 1))
		return;

	__default_local_send_IPI_allbutself(vector);
}

static inline void default_send_IPI_all(int vector)
{
	__default_local_send_IPI_all(vector);
}
#endif

#endif

#endif /* _ASM_X86_IPI_H */
