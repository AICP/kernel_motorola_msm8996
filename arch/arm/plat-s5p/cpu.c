/* linux/arch/arm/plat-s5p/cpu.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P CPU Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <mach/map.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/regs-clock.h>
#include <plat/cpu.h>
#include <plat/s5p6440.h>

/* table of supported CPUs */

static const char name_s5p6440[] = "S5P6440";

static struct cpu_table cpu_ids[] __initdata = {
	{
		.idcode		= 0x56440100,
		.idmask		= 0xffffff00,
		.map_io		= s5p6440_map_io,
		.init_clocks	= s5p6440_init_clocks,
		.init_uarts	= s5p6440_init_uarts,
		.init		= s5p6440_init,
		.name		= name_s5p6440,
	},
};

/* minimal IO mapping */

#define UART_OFFS (S5P_PA_UART & 0xfffff)

static struct map_desc s5p_iodesc[] __initdata = {
	{
		.virtual	= (unsigned long)S5P_VA_SYSCON,
		.pfn		= __phys_to_pfn(S5P_PA_SYSCON),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S5P_VA_UART + UART_OFFS),
		.pfn		= __phys_to_pfn(S5P_PA_UART),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_VIC0,
		.pfn		= __phys_to_pfn(S5P_PA_VIC0),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_VIC1,
		.pfn		= __phys_to_pfn(S5P_PA_VIC1),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_TIMER,
		.pfn		= __phys_to_pfn(S5P_PA_TIMER),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_GPIO,
		.pfn		= __phys_to_pfn(S5P_PA_GPIO),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

/* read cpu identification code */

void __init s5p_init_io(struct map_desc *mach_desc,
			int size, void __iomem *cpuid_addr)
{
	unsigned long idcode;

	/* initialize the io descriptors we need for initialization */
	iotable_init(s5p_iodesc, ARRAY_SIZE(s5p_iodesc));
	if (mach_desc)
		iotable_init(mach_desc, size);

	idcode = __raw_readl(cpuid_addr);
	s3c_init_cpu(idcode, cpu_ids, ARRAY_SIZE(cpu_ids));
}
