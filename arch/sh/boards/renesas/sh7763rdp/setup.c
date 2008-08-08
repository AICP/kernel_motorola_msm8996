/*
 * linux/arch/sh/boards/renesas/sh7763rdp/setup.c
 *
 * Renesas Solutions sh7763rdp board
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Copyright (C) 2008 Nobuhiro Iwamatsu <iwamatsu.nobuhiro@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/mtd/physmap.h>
#include <asm/io.h>
#include <asm/sh7763rdp.h>

/* NOR Flash */
static struct mtd_partition sh7763rdp_nor_flash_partitions[] = {
	{
		.name = "U-Boot",
		.offset = 0,
		.size = (2 * 128 * 1024),
		.mask_flags = MTD_WRITEABLE,	/* Read-only */
	}, {
		.name = "Linux-Kernel",
		.offset = MTDPART_OFS_APPEND,
		.size = (20 * 128 * 1024),
	}, {
		.name = "Root Filesystem",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	},
};

static struct physmap_flash_data sh7763rdp_nor_flash_data = {
	.width = 2,
	.parts = sh7763rdp_nor_flash_partitions,
	.nr_parts = ARRAY_SIZE(sh7763rdp_nor_flash_partitions),
};

static struct resource sh7763rdp_nor_flash_resources[] = {
	[0] = {
		.name = "NOR Flash",
		.start = 0,
		.end = (64 * 1024 * 1024),
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device sh7763rdp_nor_flash_device = {
	.name = "physmap-flash",
	.resource = sh7763rdp_nor_flash_resources,
	.num_resources = ARRAY_SIZE(sh7763rdp_nor_flash_resources),
	.dev = {
		.platform_data = &sh7763rdp_nor_flash_data,
	},
};

static struct platform_device *sh7763rdp_devices[] __initdata = {
	&sh7763rdp_nor_flash_device,
};

static int __init sh7763rdp_devices_setup(void)
{
	return platform_add_devices(sh7763rdp_devices,
				    ARRAY_SIZE(sh7763rdp_devices));
}
__initcall(sh7763rdp_devices_setup);

static void __init sh7763rdp_setup(char **cmdline_p)
{
	/* Board version check */
	if (ctrl_inw(CPLD_BOARD_ID_ERV_REG) == 0xECB1)
		printk(KERN_INFO "RTE Standard Configuration\n");
	else
		printk(KERN_INFO "RTA Standard Configuration\n");

	/* USB pin select bits (clear bit 5-2 to 0) */
	ctrl_outw((ctrl_inw(PORT_PSEL2) & 0xFFC3), PORT_PSEL2);
	/* USBH setup port I controls to other (clear bits 4-9 to 0) */
	ctrl_outw(ctrl_inw(PORT_PICR) & 0xFC0F, PORT_PICR);

	/* Select USB Host controller */
	ctrl_outw(0x00, USB_USBHSC);

	/* For LCD */
	/* set PTJ7-1, bits 15-2 of PJCR to 0 */
	ctrl_outw(ctrl_inw(PORT_PJCR) & 0x0003, PORT_PJCR);
	/* set PTI5, bits 11-10 of PICR to 0 */
	ctrl_outw(ctrl_inw(PORT_PICR) & 0xF3FF, PORT_PICR);
	ctrl_outw(0, PORT_PKCR);
	ctrl_outw(0, PORT_PLCR);
	/* set PSEL2 bits 14-8, 5-4, of PSEL2 to 0 */
	ctrl_outw((ctrl_inw(PORT_PSEL2) & 0x00C0), PORT_PSEL2);
	/* set PSEL3 bits 14-12, 6-4, 2-0 of PSEL3 to 0 */
	ctrl_outw((ctrl_inw(PORT_PSEL3) & 0x0700), PORT_PSEL3);

	/* For HAC */
	/* bit3-0  0100:HAC & SSI1 enable */
	ctrl_outw((ctrl_inw(PORT_PSEL1) & 0xFFF0) | 0x0004, PORT_PSEL1);
	/* bit14      1:SSI_HAC_CLK enable */
	ctrl_outw(ctrl_inw(PORT_PSEL4) | 0x4000, PORT_PSEL4);

	/* SH-Ether */
	ctrl_outw((ctrl_inw(PORT_PSEL1) & ~0xff00) | 0x2400, PORT_PSEL1);
	ctrl_outw(0x0, PORT_PFCR);
	ctrl_outw(0x0, PORT_PFCR);
	ctrl_outw(0x0, PORT_PFCR);

	/* MMC */
	/*selects SCIF and MMC other functions */
	ctrl_outw(0x0001, PORT_PSEL0);
	/* MMC clock operates */
	ctrl_outl(ctrl_inl(MSTPCR1) & ~0x8, MSTPCR1);
	ctrl_outw(ctrl_inw(PORT_PACR) & ~0x3000, PORT_PACR);
	ctrl_outw(ctrl_inw(PORT_PCCR) & ~0xCFC3, PORT_PCCR);
}

static struct sh_machine_vector mv_sh7763rdp __initmv = {
	.mv_name = "sh7763drp",
	.mv_setup = sh7763rdp_setup,
	.mv_nr_irqs = 112,
	.mv_init_irq = init_sh7763rdp_IRQ,
};
