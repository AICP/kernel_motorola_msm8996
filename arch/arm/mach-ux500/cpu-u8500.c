/*
 * Copyright (C) 2008-2009 ST-Ericsson
 *
 * Author: Srinidhi KASAGAR <srinidhi.kasagar@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/localtimer.h>
#include <asm/hardware/gic.h>
#include <asm/mach/map.h>
#include <plat/mtu.h>
#include <mach/hardware.h>
#include <mach/setup.h>

/* add any platform devices here - TODO */
static struct platform_device *platform_devs[] __initdata = {
	/* yet to be added, add i2c0, gpio.. */
};

#define __IO_DEV_DESC(x, sz)	{		\
	.virtual	= IO_ADDRESS(x),	\
	.pfn		= __phys_to_pfn(x),	\
	.length		= sz,			\
	.type		= MT_DEVICE,		\
}

/* minimum static i/o mapping required to boot U8500 platforms */
static struct map_desc u8500_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_UART2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GIC_CPU_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GIC_DIST_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_TWD_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_SCU_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_BACKUPRAM0_BASE, SZ_8K),
};

static struct map_desc u8500ed_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_MTU0_BASE_ED, SZ_4K),
};

static struct map_desc u8500v1_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_MTU0_BASE_V1, SZ_4K),
};

void __init u8500_map_io(void)
{
	iotable_init(u8500_io_desc, ARRAY_SIZE(u8500_io_desc));

	if (cpu_is_u8500ed())
		iotable_init(u8500ed_io_desc, ARRAY_SIZE(u8500ed_io_desc));
	else
		iotable_init(u8500v1_io_desc, ARRAY_SIZE(u8500v1_io_desc));
}

void __init u8500_init_irq(void)
{
	gic_dist_init(0, __io_address(U8500_GIC_DIST_BASE), 29);
	gic_cpu_init(0, __io_address(U8500_GIC_CPU_BASE));
}

/*
 * This function is called from the board init
 */
void __init u8500_init_devices(void)
{
	/* Register the platform devices */
	platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

	return ;
}

static void __init u8500_timer_init(void)
{
#ifdef CONFIG_LOCAL_TIMERS
	/* Setup the local timer base */
	twd_base = __io_address(U8500_TWD_BASE);
#endif
	/* Setup the MTU base */
	if (cpu_is_u8500ed())
		mtu_base = __io_address(U8500_MTU0_BASE_ED);
	else
		mtu_base = __io_address(U8500_MTU0_BASE_V1);

	nmdk_timer_init();
}

struct sys_timer u8500_timer = {
	.init	= u8500_timer_init,
};
