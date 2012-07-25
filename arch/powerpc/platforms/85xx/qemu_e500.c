/*
 * Paravirt target for a generic QEMU e500 machine
 *
 * This is intended to be a flexible device-tree-driven platform, not fixed
 * to a particular piece of hardware or a particular spec of virtual hardware,
 * beyond the assumption of an e500-family CPU.  Some things are still hardcoded
 * here, such as MPIC, but this is a limitation of the current code rather than
 * an interface contract with QEMU.
 *
 * Copyright 2012 Freescale Semiconductor Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/of_fdt.h>
#include <asm/machdep.h>
#include <asm/time.h>
#include <asm/udbg.h>
#include <asm/mpic.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>
#include "smp.h"
#include "mpc85xx.h"

void __init qemu_e500_pic_init(void)
{
	struct mpic *mpic;

	mpic = mpic_alloc(NULL, 0, MPIC_BIG_ENDIAN | MPIC_SINGLE_DEST_CPU,
			0, 256, " OpenPIC  ");

	BUG_ON(mpic == NULL);
	mpic_init(mpic);
}

static void __init qemu_e500_setup_arch(void)
{
	ppc_md.progress("qemu_e500_setup_arch()", 0);

	fsl_pci_init();
	mpc85xx_smp_init();
}

/*
 * Called very early, device-tree isn't unflattened
 */
static int __init qemu_e500_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	return !!of_flat_dt_is_compatible(root, "fsl,qemu-e500");
}

machine_device_initcall(qemu_e500, mpc85xx_common_publish_devices);

define_machine(qemu_e500) {
	.name			= "QEMU e500",
	.probe			= qemu_e500_probe,
	.setup_arch		= qemu_e500_setup_arch,
	.init_IRQ		= qemu_e500_pic_init,
#ifdef CONFIG_PCI
	.pcibios_fixup_bus	= fsl_pcibios_fixup_bus,
#endif
	.get_irq		= mpic_get_irq,
	.restart		= fsl_rstcr_restart,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
};
