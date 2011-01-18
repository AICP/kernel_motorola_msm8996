/*
 * Versatile Express Core Tile Cortex A9x4 Support
 */
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clkdev.h>

#include <asm/pgtable.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/pmu.h>
#include <asm/smp_twd.h>

#include <mach/ct-ca9x4.h>

#include <asm/hardware/timer-sp.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include "core.h"

#include <mach/motherboard.h>

#include <plat/clcd.h>

#define V2M_PA_CS7	0x10000000

static struct map_desc ct_ca9x4_io_desc[] __initdata = {
	{
		.virtual	= __MMIO_P2V(CT_CA9X4_MPIC),
		.pfn		= __phys_to_pfn(CT_CA9X4_MPIC),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= __MMIO_P2V(CT_CA9X4_SP804_TIMER),
		.pfn		= __phys_to_pfn(CT_CA9X4_SP804_TIMER),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= __MMIO_P2V(CT_CA9X4_L2CC),
		.pfn		= __phys_to_pfn(CT_CA9X4_L2CC),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

static void __init ct_ca9x4_map_io(void)
{
#ifdef CONFIG_LOCAL_TIMERS
	twd_base = MMIO_P2V(A9_MPCORE_TWD);
#endif
	v2m_map_io(ct_ca9x4_io_desc, ARRAY_SIZE(ct_ca9x4_io_desc));
}

static void __init ct_ca9x4_init_irq(void)
{
	gic_init(0, 29, MMIO_P2V(A9_MPCORE_GIC_DIST),
		 MMIO_P2V(A9_MPCORE_GIC_CPU));
}

#if 0
static void __init ct_ca9x4_timer_init(void)
{
	writel(0, MMIO_P2V(CT_CA9X4_TIMER0) + TIMER_CTRL);
	writel(0, MMIO_P2V(CT_CA9X4_TIMER1) + TIMER_CTRL);

	sp804_clocksource_init(MMIO_P2V(CT_CA9X4_TIMER1));
	sp804_clockevents_init(MMIO_P2V(CT_CA9X4_TIMER0), IRQ_CT_CA9X4_TIMER0);
}

static struct sys_timer ct_ca9x4_timer = {
	.init	= ct_ca9x4_timer_init,
};
#endif

static void ct_ca9x4_clcd_enable(struct clcd_fb *fb)
{
	v2m_cfg_write(SYS_CFG_MUXFPGA | SYS_CFG_SITE_DB1, 0);
	v2m_cfg_write(SYS_CFG_DVIMODE | SYS_CFG_SITE_DB1, 2);
}

static int ct_ca9x4_clcd_setup(struct clcd_fb *fb)
{
	unsigned long framesize = 1024 * 768 * 2;

	fb->panel = versatile_clcd_get_panel("XVGA");
	if (!fb->panel)
		return -EINVAL;

	return versatile_clcd_setup_dma(fb, framesize);
}

static struct clcd_board ct_ca9x4_clcd_data = {
	.name		= "CT-CA9X4",
	.caps		= CLCD_CAP_5551 | CLCD_CAP_565,
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.enable		= ct_ca9x4_clcd_enable,
	.setup		= ct_ca9x4_clcd_setup,
	.mmap		= versatile_clcd_mmap_dma,
	.remove		= versatile_clcd_remove_dma,
};

static AMBA_DEVICE(clcd, "ct:clcd", CT_CA9X4_CLCDC, &ct_ca9x4_clcd_data);
static AMBA_DEVICE(dmc, "ct:dmc", CT_CA9X4_DMC, NULL);
static AMBA_DEVICE(smc, "ct:smc", CT_CA9X4_SMC, NULL);
static AMBA_DEVICE(gpio, "ct:gpio", CT_CA9X4_GPIO, NULL);

static struct amba_device *ct_ca9x4_amba_devs[] __initdata = {
	&clcd_device,
	&dmc_device,
	&smc_device,
	&gpio_device,
};


static long ct_round(struct clk *clk, unsigned long rate)
{
	return rate;
}

static int ct_set(struct clk *clk, unsigned long rate)
{
	return v2m_cfg_write(SYS_CFG_OSC | SYS_CFG_SITE_DB1 | 1, rate);
}

static const struct clk_ops osc1_clk_ops = {
	.round	= ct_round,
	.set	= ct_set,
};

static struct clk osc1_clk = {
	.ops	= &osc1_clk_ops,
	.rate	= 24000000,
};

static struct clk_lookup lookups[] = {
	{	/* CLCD */
		.dev_id		= "ct:clcd",
		.clk		= &osc1_clk,
	},
};

static struct resource pmu_resources[] = {
	[0] = {
		.start	= IRQ_CT_CA9X4_PMU_CPU0,
		.end	= IRQ_CT_CA9X4_PMU_CPU0,
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= IRQ_CT_CA9X4_PMU_CPU1,
		.end	= IRQ_CT_CA9X4_PMU_CPU1,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= IRQ_CT_CA9X4_PMU_CPU2,
		.end	= IRQ_CT_CA9X4_PMU_CPU2,
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		.start	= IRQ_CT_CA9X4_PMU_CPU3,
		.end	= IRQ_CT_CA9X4_PMU_CPU3,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.num_resources	= ARRAY_SIZE(pmu_resources),
	.resource	= pmu_resources,
};

static void __init ct_ca9x4_init(void)
{
	int i;

#ifdef CONFIG_CACHE_L2X0
	void __iomem *l2x0_base = MMIO_P2V(CT_CA9X4_L2CC);

	/* set RAM latencies to 1 cycle for this core tile. */
	writel(0, l2x0_base + L2X0_TAG_LATENCY_CTRL);
	writel(0, l2x0_base + L2X0_DATA_LATENCY_CTRL);

	l2x0_init(l2x0_base, 0x00400000, 0xfe0fffff);
#endif

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	for (i = 0; i < ARRAY_SIZE(ct_ca9x4_amba_devs); i++)
		amba_device_register(ct_ca9x4_amba_devs[i], &iomem_resource);

	platform_device_register(&pmu_device);
}

MACHINE_START(VEXPRESS, "ARM-Versatile Express CA9x4")
	.boot_params	= PHYS_OFFSET + 0x00000100,
	.map_io		= ct_ca9x4_map_io,
	.init_irq	= ct_ca9x4_init_irq,
#if 0
	.timer		= &ct_ca9x4_timer,
#else
	.timer		= &v2m_timer,
#endif
	.init_machine	= ct_ca9x4_init,
MACHINE_END
