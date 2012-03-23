/*
 *  linux/arch/arm/mach-realview/realview_pba8.c
 *
 *  Copyright (C) 2008 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl061.h>
#include <linux/amba/mmci.h>
#include <linux/amba/pl022.h>
#include <linux/io.h>

#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/pmu.h>
#include <asm/pgtable.h>
#include <asm/hardware/gic.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/hardware.h>
#include <mach/board-pba8.h>
#include <mach/irqs.h>

#include "core.h"

static struct map_desc realview_pba8_io_desc[] __initdata = {
	{
		.virtual	= IO_ADDRESS(REALVIEW_SYS_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_SYS_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(REALVIEW_PBA8_GIC_CPU_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_PBA8_GIC_CPU_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(REALVIEW_PBA8_GIC_DIST_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_PBA8_GIC_DIST_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(REALVIEW_SCTL_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_SCTL_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(REALVIEW_PBA8_TIMER0_1_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_PBA8_TIMER0_1_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(REALVIEW_PBA8_TIMER2_3_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_PBA8_TIMER2_3_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
#ifdef CONFIG_PCI
	{
		.virtual	= PCIX_UNIT_BASE,
		.pfn		= __phys_to_pfn(REALVIEW_PBA8_PCI_BASE),
		.length		= REALVIEW_PBA8_PCI_BASE_SIZE,
		.type		= MT_DEVICE
	},
#endif
#ifdef CONFIG_DEBUG_LL
	{
		.virtual	= IO_ADDRESS(REALVIEW_PBA8_UART0_BASE),
		.pfn		= __phys_to_pfn(REALVIEW_PBA8_UART0_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
#endif
};

static void __init realview_pba8_map_io(void)
{
	iotable_init(realview_pba8_io_desc, ARRAY_SIZE(realview_pba8_io_desc));
}

static struct pl061_platform_data gpio0_plat_data = {
	.gpio_base	= 0,
};

static struct pl061_platform_data gpio1_plat_data = {
	.gpio_base	= 8,
};

static struct pl061_platform_data gpio2_plat_data = {
	.gpio_base	= 16,
};

static struct pl022_ssp_controller ssp0_plat_data = {
	.bus_id = 0,
	.enable_dma = 0,
	.num_chipselect = 1,
};

/*
 * RealView PBA8Core AMBA devices
 */

#define GPIO2_IRQ		{ IRQ_PBA8_GPIO2, NO_IRQ }
#define GPIO3_IRQ		{ IRQ_PBA8_GPIO3, NO_IRQ }
#define AACI_IRQ		{ IRQ_PBA8_AACI, NO_IRQ }
#define MMCI0_IRQ		{ IRQ_PBA8_MMCI0A, IRQ_PBA8_MMCI0B }
#define KMI0_IRQ		{ IRQ_PBA8_KMI0, NO_IRQ }
#define KMI1_IRQ		{ IRQ_PBA8_KMI1, NO_IRQ }
#define PBA8_SMC_IRQ		{ NO_IRQ, NO_IRQ }
#define MPMC_IRQ		{ NO_IRQ, NO_IRQ }
#define PBA8_CLCD_IRQ		{ IRQ_PBA8_CLCD, NO_IRQ }
#define DMAC_IRQ		{ IRQ_PBA8_DMAC, NO_IRQ }
#define SCTL_IRQ		{ NO_IRQ, NO_IRQ }
#define PBA8_WATCHDOG_IRQ	{ IRQ_PBA8_WATCHDOG, NO_IRQ }
#define PBA8_GPIO0_IRQ		{ IRQ_PBA8_GPIO0, NO_IRQ }
#define GPIO1_IRQ		{ IRQ_PBA8_GPIO1, NO_IRQ }
#define PBA8_RTC_IRQ		{ IRQ_PBA8_RTC, NO_IRQ }
#define SCI_IRQ			{ IRQ_PBA8_SCI, NO_IRQ }
#define PBA8_UART0_IRQ		{ IRQ_PBA8_UART0, NO_IRQ }
#define PBA8_UART1_IRQ		{ IRQ_PBA8_UART1, NO_IRQ }
#define PBA8_UART2_IRQ		{ IRQ_PBA8_UART2, NO_IRQ }
#define PBA8_UART3_IRQ		{ IRQ_PBA8_UART3, NO_IRQ }
#define PBA8_SSP_IRQ		{ IRQ_PBA8_SSP, NO_IRQ }

/* FPGA Primecells */
AMBA_DEVICE(aaci,	"fpga:aaci",	AACI,		NULL);
AMBA_DEVICE(mmc0,	"fpga:mmc0",	MMCI0,		&realview_mmc0_plat_data);
AMBA_DEVICE(kmi0,	"fpga:kmi0",	KMI0,		NULL);
AMBA_DEVICE(kmi1,	"fpga:kmi1",	KMI1,		NULL);
AMBA_DEVICE(uart3,	"fpga:uart3",	PBA8_UART3,	NULL);

/* DevChip Primecells */
AMBA_DEVICE(smc,	"dev:smc",	PBA8_SMC,	NULL);
AMBA_DEVICE(sctl,	"dev:sctl",	SCTL,		NULL);
AMBA_DEVICE(wdog,	"dev:wdog",	PBA8_WATCHDOG, NULL);
AMBA_DEVICE(gpio0,	"dev:gpio0",	PBA8_GPIO0,	&gpio0_plat_data);
AMBA_DEVICE(gpio1,	"dev:gpio1",	GPIO1,		&gpio1_plat_data);
AMBA_DEVICE(gpio2,	"dev:gpio2",	GPIO2,		&gpio2_plat_data);
AMBA_DEVICE(rtc,	"dev:rtc",	PBA8_RTC,	NULL);
AMBA_DEVICE(sci0,	"dev:sci0",	SCI,		NULL);
AMBA_DEVICE(uart0,	"dev:uart0",	PBA8_UART0,	NULL);
AMBA_DEVICE(uart1,	"dev:uart1",	PBA8_UART1,	NULL);
AMBA_DEVICE(uart2,	"dev:uart2",	PBA8_UART2,	NULL);
AMBA_DEVICE(ssp0,	"dev:ssp0",	PBA8_SSP,	&ssp0_plat_data);

/* Primecells on the NEC ISSP chip */
AMBA_DEVICE(clcd,	"issp:clcd",	PBA8_CLCD,	&clcd_plat_data);
AMBA_DEVICE(dmac,	"issp:dmac",	DMAC,		NULL);

static struct amba_device *amba_devs[] __initdata = {
	&dmac_device,
	&uart0_device,
	&uart1_device,
	&uart2_device,
	&uart3_device,
	&smc_device,
	&clcd_device,
	&sctl_device,
	&wdog_device,
	&gpio0_device,
	&gpio1_device,
	&gpio2_device,
	&rtc_device,
	&sci0_device,
	&ssp0_device,
	&aaci_device,
	&mmc0_device,
	&kmi0_device,
	&kmi1_device,
};

/*
 * RealView PB-A8 platform devices
 */
static struct resource realview_pba8_flash_resource[] = {
	[0] = {
		.start		= REALVIEW_PBA8_FLASH0_BASE,
		.end		= REALVIEW_PBA8_FLASH0_BASE + REALVIEW_PBA8_FLASH0_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= REALVIEW_PBA8_FLASH1_BASE,
		.end		= REALVIEW_PBA8_FLASH1_BASE + REALVIEW_PBA8_FLASH1_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
};

static struct resource realview_pba8_smsc911x_resources[] = {
	[0] = {
		.start		= REALVIEW_PBA8_ETH_BASE,
		.end		= REALVIEW_PBA8_ETH_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_PBA8_ETH,
		.end		= IRQ_PBA8_ETH,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource realview_pba8_isp1761_resources[] = {
	[0] = {
		.start		= REALVIEW_PBA8_USB_BASE,
		.end		= REALVIEW_PBA8_USB_BASE + SZ_128K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_PBA8_USB,
		.end		= IRQ_PBA8_USB,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource pmu_resource = {
	.start		= IRQ_PBA8_PMU,
	.end		= IRQ_PBA8_PMU,
	.flags		= IORESOURCE_IRQ,
};

static struct platform_device pmu_device = {
	.name			= "arm-pmu",
	.id			= ARM_PMU_DEVICE_CPU,
	.num_resources		= 1,
	.resource		= &pmu_resource,
};

static void __init gic_init_irq(void)
{
	/* ARM PB-A8 on-board GIC */
	gic_init(0, IRQ_PBA8_GIC_START,
		 __io_address(REALVIEW_PBA8_GIC_DIST_BASE),
		 __io_address(REALVIEW_PBA8_GIC_CPU_BASE));
}

static void __init realview_pba8_timer_init(void)
{
	timer0_va_base = __io_address(REALVIEW_PBA8_TIMER0_1_BASE);
	timer1_va_base = __io_address(REALVIEW_PBA8_TIMER0_1_BASE) + 0x20;
	timer2_va_base = __io_address(REALVIEW_PBA8_TIMER2_3_BASE);
	timer3_va_base = __io_address(REALVIEW_PBA8_TIMER2_3_BASE) + 0x20;

	realview_timer_init(IRQ_PBA8_TIMER0_1);
}

static struct sys_timer realview_pba8_timer = {
	.init		= realview_pba8_timer_init,
};

static void realview_pba8_restart(char mode, const char *cmd)
{
	void __iomem *reset_ctrl = __io_address(REALVIEW_SYS_RESETCTL);
	void __iomem *lock_ctrl = __io_address(REALVIEW_SYS_LOCK);

	/*
	 * To reset, we hit the on-board reset register
	 * in the system FPGA
	 */
	__raw_writel(REALVIEW_SYS_LOCK_VAL, lock_ctrl);
	__raw_writel(0x0000, reset_ctrl);
	__raw_writel(0x0004, reset_ctrl);
	dsb();
}

static void __init realview_pba8_init(void)
{
	int i;

	realview_flash_register(realview_pba8_flash_resource,
				ARRAY_SIZE(realview_pba8_flash_resource));
	realview_eth_register(NULL, realview_pba8_smsc911x_resources);
	platform_device_register(&realview_i2c_device);
	platform_device_register(&realview_cf_device);
	realview_usb_register(realview_pba8_isp1761_resources);
	platform_device_register(&pmu_device);

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		struct amba_device *d = amba_devs[i];
		amba_device_register(d, &iomem_resource);
	}

#ifdef CONFIG_LEDS
	leds_event = realview_leds_event;
#endif
}

MACHINE_START(REALVIEW_PBA8, "ARM-RealView PB-A8")
	/* Maintainer: ARM Ltd/Deep Blue Solutions Ltd */
	.atag_offset	= 0x100,
	.fixup		= realview_fixup,
	.map_io		= realview_pba8_map_io,
	.init_early	= realview_init_early,
	.init_irq	= gic_init_irq,
	.timer		= &realview_pba8_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= realview_pba8_init,
#ifdef CONFIG_ZONE_DMA
	.dma_zone_size	= SZ_256M,
#endif
	.restart	= realview_pba8_restart,
MACHINE_END
