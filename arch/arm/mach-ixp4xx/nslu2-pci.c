/*
 * arch/arm/mach-ixp4xx/nslu2-pci.c
 *
 * NSLU2 board-level PCI initialization
 *
 * based on ixdp425-pci.c:
 *	Copyright (C) 2002 Intel Corporation.
 *	Copyright (C) 2003-2004 MontaVista Software, Inc.
 *
 * Maintainer: http://www.nslu2-linux.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <asm/mach/pci.h>
#include <asm/mach-types.h>

#define NSLU2_PCI_MAX_DEV	3
#define NSLU2_PCI_IRQ_LINES	3

/* PCI controller GPIO to IRQ pin mappings */
#define NSLU2_PCI_INTA_PIN	11
#define NSLU2_PCI_INTB_PIN	10
#define NSLU2_PCI_INTC_PIN	9
#define NSLU2_PCI_INTD_PIN	8
#define IRQ_NSLU2_PCI_INTA      IRQ_IXP4XX_GPIO11
#define IRQ_NSLU2_PCI_INTB      IRQ_IXP4XX_GPIO10
#define IRQ_NSLU2_PCI_INTC      IRQ_IXP4XX_GPIO9

void __init nslu2_pci_preinit(void)
{
	set_irq_type(IRQ_NSLU2_PCI_INTA, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_NSLU2_PCI_INTB, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_NSLU2_PCI_INTC, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}

static int __init nslu2_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	static int pci_irq_table[NSLU2_PCI_IRQ_LINES] = {
		IRQ_NSLU2_PCI_INTA,
		IRQ_NSLU2_PCI_INTB,
		IRQ_NSLU2_PCI_INTC,
	};

	int irq = -1;

	if (slot >= 1 && slot <= NSLU2_PCI_MAX_DEV &&
		pin >= 1 && pin <= NSLU2_PCI_IRQ_LINES) {
			irq = pci_irq_table[(slot + pin - 2) % NSLU2_PCI_IRQ_LINES];
	}

	return irq;
}

struct hw_pci __initdata nslu2_pci = {
	.nr_controllers = 1,
	.preinit	= nslu2_pci_preinit,
	.swizzle	= pci_std_swizzle,
	.setup		= ixp4xx_setup,
	.scan		= ixp4xx_scan_bus,
	.map_irq	= nslu2_map_irq,
};

int __init nslu2_pci_init(void) /* monkey see, monkey do */
{
	if (machine_is_nslu2())
		pci_common_init(&nslu2_pci);

	return 0;
}

subsys_initcall(nslu2_pci_init);
