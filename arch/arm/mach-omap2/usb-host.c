/*
 * usb-host.c - OMAP USB Host
 *
 * This file will contain the board specific details for the
 * Synopsys EHCI/OHCI host controller on OMAP3430 and onwards
 *
 * Copyright (C) 2007-2011 Texas Instruments
 * Author: Vikram Pandita <vikram.pandita@ti.com>
 *
 * Generalization by:
 * Felipe Balbi <balbi@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <plat/usb.h>

#include "mux.h"

#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)

static struct resource ehci_resources[] = {
	{
		.flags	= IORESOURCE_MEM,
	},
	{
		.flags	= IORESOURCE_MEM,
	},
	{
		.flags	= IORESOURCE_MEM,
	},
	{         /* general IRQ */
		.flags   = IORESOURCE_IRQ,
	}
};

static u64 ehci_dmamask = ~(u32)0;
static struct platform_device ehci_device = {
	.name           = "ehci-omap",
	.id             = 0,
	.dev = {
		.dma_mask               = &ehci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = NULL,
	},
	.num_resources  = ARRAY_SIZE(ehci_resources),
	.resource       = ehci_resources,
};

/* MUX settings for EHCI pins */
/*
 * setup_ehci_io_mux - initialize IO pad mux for USBHOST
 */
static void setup_ehci_io_mux(const enum usbhs_omap_port_mode *port_mode)
{
	switch (port_mode[0]) {
	case OMAP_EHCI_PORT_MODE_PHY:
		omap_mux_init_signal("hsusb1_stp", OMAP_PIN_OUTPUT);
		omap_mux_init_signal("hsusb1_clk", OMAP_PIN_OUTPUT);
		omap_mux_init_signal("hsusb1_dir", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_nxt", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data0", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data1", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data2", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data3", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data4", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data5", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data6", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_data7", OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_EHCI_PORT_MODE_TLL:
		omap_mux_init_signal("hsusb1_tll_stp",
			OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("hsusb1_tll_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb1_tll_data7",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
		/* FALLTHROUGH */
	default:
		break;
	}

	switch (port_mode[1]) {
	case OMAP_EHCI_PORT_MODE_PHY:
		omap_mux_init_signal("hsusb2_stp", OMAP_PIN_OUTPUT);
		omap_mux_init_signal("hsusb2_clk", OMAP_PIN_OUTPUT);
		omap_mux_init_signal("hsusb2_dir", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_nxt", OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_data7",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_EHCI_PORT_MODE_TLL:
		omap_mux_init_signal("hsusb2_tll_stp",
			OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("hsusb2_tll_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb2_tll_data7",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
		/* FALLTHROUGH */
	default:
		break;
	}

	switch (port_mode[2]) {
	case OMAP_EHCI_PORT_MODE_PHY:
		printk(KERN_WARNING "Port3 can't be used in PHY mode\n");
		break;
	case OMAP_EHCI_PORT_MODE_TLL:
		omap_mux_init_signal("hsusb3_tll_stp",
			OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("hsusb3_tll_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("hsusb3_tll_data7",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
		/* FALLTHROUGH */
	default:
		break;
	}

	return;
}

static void setup_4430ehci_io_mux(const enum usbhs_omap_port_mode *port_mode)
{
	switch (port_mode[0]) {
	case OMAP_EHCI_PORT_MODE_PHY:
		omap_mux_init_signal("usbb1_ulpiphy_stp",
			OMAP_PIN_OUTPUT);
		omap_mux_init_signal("usbb1_ulpiphy_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpiphy_dat7",
			OMAP_PIN_INPUT_PULLDOWN);
			break;
	case OMAP_EHCI_PORT_MODE_TLL:
		omap_mux_init_signal("usbb1_ulpitll_stp",
			OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("usbb1_ulpitll_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb1_ulpitll_dat7",
			OMAP_PIN_INPUT_PULLDOWN);
			break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
	default:
			break;
	}
	switch (port_mode[1]) {
	case OMAP_EHCI_PORT_MODE_PHY:
		omap_mux_init_signal("usbb2_ulpiphy_stp",
			OMAP_PIN_OUTPUT);
		omap_mux_init_signal("usbb2_ulpiphy_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpiphy_dat7",
			OMAP_PIN_INPUT_PULLDOWN);
			break;
	case OMAP_EHCI_PORT_MODE_TLL:
		omap_mux_init_signal("usbb2_ulpitll_stp",
			OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("usbb2_ulpitll_clk",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dir",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_nxt",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat1",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat2",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat3",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat4",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat5",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat6",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("usbb2_ulpitll_dat7",
			OMAP_PIN_INPUT_PULLDOWN);
			break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
	default:
			break;
	}
}

void __init usb_ehci_init(const struct usbhs_omap_board_data *pdata)
{
	platform_device_add_data(&ehci_device, pdata, sizeof(*pdata));

	/* Setup Pin IO MUX for EHCI */
	if (cpu_is_omap34xx()) {
		ehci_resources[0].start	= OMAP34XX_EHCI_BASE;
		ehci_resources[0].end	= OMAP34XX_EHCI_BASE + SZ_1K - 1;
		ehci_resources[1].start	= OMAP34XX_UHH_CONFIG_BASE;
		ehci_resources[1].end	= OMAP34XX_UHH_CONFIG_BASE + SZ_1K - 1;
		ehci_resources[2].start	= OMAP34XX_USBTLL_BASE;
		ehci_resources[2].end	= OMAP34XX_USBTLL_BASE + SZ_4K - 1;
		ehci_resources[3].start = INT_34XX_EHCI_IRQ;
		setup_ehci_io_mux(pdata->port_mode);
	} else if (cpu_is_omap44xx()) {
		ehci_resources[0].start	= OMAP44XX_HSUSB_EHCI_BASE;
		ehci_resources[0].end	= OMAP44XX_HSUSB_EHCI_BASE + SZ_1K - 1;
		ehci_resources[1].start	= OMAP44XX_UHH_CONFIG_BASE;
		ehci_resources[1].end	= OMAP44XX_UHH_CONFIG_BASE + SZ_2K - 1;
		ehci_resources[2].start	= OMAP44XX_USBTLL_BASE;
		ehci_resources[2].end	= OMAP44XX_USBTLL_BASE + SZ_4K - 1;
		ehci_resources[3].start = OMAP44XX_IRQ_EHCI;
		setup_4430ehci_io_mux(pdata->port_mode);
	}

	ehci_resources[0].name	= "ehci";
	ehci_resources[1].name	= "uhh";
	ehci_resources[2].name	= "tll";
	ehci_resources[3].name	= "irq";

	if (platform_device_register(&ehci_device) < 0) {
		printk(KERN_ERR "Unable to register HS-USB (EHCI) device\n");
		return;
	}
}

#else

void __init usb_ehci_init(const struct usbhs_omap_board_data *pdata)

{
}

#endif /* CONFIG_USB_EHCI_HCD */

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)

static struct resource ohci_resources[] = {
	{
		.name	= "ohci",
		.start	= OMAP34XX_OHCI_BASE,
		.end	= OMAP34XX_OHCI_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "uhh",
		.start	= OMAP34XX_UHH_CONFIG_BASE,
		.end	= OMAP34XX_UHH_CONFIG_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "tll",
		.start	= OMAP34XX_USBTLL_BASE,
		.end	= OMAP34XX_USBTLL_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{	/* general IRQ */
		.name	= "irq",
		.start	= INT_34XX_OHCI_IRQ,
		.flags	= IORESOURCE_IRQ,
	}
};

static u64 ohci_dmamask = DMA_BIT_MASK(32);

static struct platform_device ohci_device = {
	.name		= "ohci-omap3",
	.id		= 0,
	.dev = {
		.dma_mask		= &ohci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ohci_resources),
	.resource	= ohci_resources,
};

static void setup_ohci_io_mux(const enum usbhs_omap_port_mode *port_mode)
{
	switch (port_mode[0]) {
	case OMAP_OHCI_PORT_MODE_PHY_6PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_PHY_6PIN_DPDM:
	case OMAP_OHCI_PORT_MODE_TLL_6PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_6PIN_DPDM:
		omap_mux_init_signal("mm1_rxdp",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mm1_rxdm",
			OMAP_PIN_INPUT_PULLDOWN);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_PHY_4PIN_DPDM:
	case OMAP_OHCI_PORT_MODE_TLL_4PIN_DPDM:
		omap_mux_init_signal("mm1_rxrcv",
			OMAP_PIN_INPUT_PULLDOWN);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_PHY_3PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_3PIN_DATSE0:
		omap_mux_init_signal("mm1_txen_n", OMAP_PIN_OUTPUT);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_TLL_2PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_2PIN_DPDM:
		omap_mux_init_signal("mm1_txse0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mm1_txdat",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
		/* FALLTHROUGH */
	default:
		break;
	}
	switch (port_mode[1]) {
	case OMAP_OHCI_PORT_MODE_PHY_6PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_PHY_6PIN_DPDM:
	case OMAP_OHCI_PORT_MODE_TLL_6PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_6PIN_DPDM:
		omap_mux_init_signal("mm2_rxdp",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mm2_rxdm",
			OMAP_PIN_INPUT_PULLDOWN);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_PHY_4PIN_DPDM:
	case OMAP_OHCI_PORT_MODE_TLL_4PIN_DPDM:
		omap_mux_init_signal("mm2_rxrcv",
			OMAP_PIN_INPUT_PULLDOWN);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_PHY_3PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_3PIN_DATSE0:
		omap_mux_init_signal("mm2_txen_n", OMAP_PIN_OUTPUT);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_TLL_2PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_2PIN_DPDM:
		omap_mux_init_signal("mm2_txse0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mm2_txdat",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
		/* FALLTHROUGH */
	default:
		break;
	}
	switch (port_mode[2]) {
	case OMAP_OHCI_PORT_MODE_PHY_6PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_PHY_6PIN_DPDM:
	case OMAP_OHCI_PORT_MODE_TLL_6PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_6PIN_DPDM:
		omap_mux_init_signal("mm3_rxdp",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mm3_rxdm",
			OMAP_PIN_INPUT_PULLDOWN);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_PHY_4PIN_DPDM:
	case OMAP_OHCI_PORT_MODE_TLL_4PIN_DPDM:
		omap_mux_init_signal("mm3_rxrcv",
			OMAP_PIN_INPUT_PULLDOWN);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_PHY_3PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_3PIN_DATSE0:
		omap_mux_init_signal("mm3_txen_n", OMAP_PIN_OUTPUT);
		/* FALLTHROUGH */
	case OMAP_OHCI_PORT_MODE_TLL_2PIN_DATSE0:
	case OMAP_OHCI_PORT_MODE_TLL_2PIN_DPDM:
		omap_mux_init_signal("mm3_txse0",
			OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mm3_txdat",
			OMAP_PIN_INPUT_PULLDOWN);
		break;
	case OMAP_USBHS_PORT_MODE_UNUSED:
		/* FALLTHROUGH */
	default:
		break;
	}
}

void __init usb_ohci_init(const struct usbhs_omap_board_data *pdata)
{
	platform_device_add_data(&ohci_device, pdata, sizeof(*pdata));

	/* Setup Pin IO MUX for OHCI */
	if (cpu_is_omap34xx())
		setup_ohci_io_mux(pdata->port_mode);

	if (platform_device_register(&ohci_device) < 0) {
		pr_err("Unable to register FS-USB (OHCI) device\n");
		return;
	}
}

#else

void __init usb_ohci_init(const struct usbhs_omap_board_data *pdata)
{
}

#endif /* CONFIG_USB_OHCI_HCD */
