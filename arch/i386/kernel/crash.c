/*
 * Architecture specific (i386) functions for kexec based crash dumps.
 *
 * Created by: Hariprasad Nellitheertha (hari@in.ibm.com)
 *
 * Copyright (C) IBM Corporation, 2004. All rights reserved.
 *
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/irq.h>
#include <linux/reboot.h>
#include <linux/kexec.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/elf.h>
#include <linux/elfcore.h>

#include <asm/processor.h>
#include <asm/hardirq.h>
#include <asm/nmi.h>
#include <asm/hw_irq.h>

#define MAX_NOTE_BYTES 1024
typedef u32 note_buf_t[MAX_NOTE_BYTES/4];

note_buf_t crash_notes[NR_CPUS];

void machine_crash_shutdown(void)
{
	/* This function is only called after the system
	 * has paniced or is otherwise in a critical state.
	 * The minimum amount of code to allow a kexec'd kernel
	 * to run successfully needs to happen here.
	 *
	 * In practice this means shooting down the other cpus in
	 * an SMP system.
	 */
}
