/*
 * arch/arm/mach-ep93xx/include/mach/ep93xx-regs.h
 */

#ifndef __ASM_ARCH_EP93XX_REGS_H
#define __ASM_ARCH_EP93XX_REGS_H

/*
 * EP93xx Physical Memory Map:
 *
 * The ASDO pin is sampled at system reset to select a synchronous or
 * asynchronous boot configuration.  When ASDO is "1" (i.e. pulled-up)
 * the synchronous boot mode is selected.  When ASDO is "0" (i.e
 * pulled-down) the asynchronous boot mode is selected.
 *
 * In synchronous boot mode nSDCE3 is decoded starting at physical address
 * 0x00000000 and nCS0 is decoded starting at 0xf0000000.  For asynchronous
 * boot mode they are swapped with nCS0 decoded at 0x00000000 ann nSDCE3
 * decoded at 0xf0000000.
 *
 * There is known errata for the EP93xx dealing with External Memory
 * Configurations.  Please refer to "AN273: EP93xx Silicon Rev E Design
 * Guidelines" for more information.  This document can be found at:
 *
 *	http://www.cirrus.com/en/pubs/appNote/AN273REV4.pdf
 */

#define EP93XX_CS0_PHYS_BASE_ASYNC	0x00000000	/* ASDO Pin = 0 */
#define EP93XX_SDCE3_PHYS_BASE_SYNC	0x00000000	/* ASDO Pin = 1 */
#define EP93XX_CS1_PHYS_BASE		0x10000000
#define EP93XX_CS2_PHYS_BASE		0x20000000
#define EP93XX_CS3_PHYS_BASE		0x30000000
#define EP93XX_PCMCIA_PHYS_BASE		0x40000000
#define EP93XX_CS6_PHYS_BASE		0x60000000
#define EP93XX_CS7_PHYS_BASE		0x70000000
#define EP93XX_SDCE0_PHYS_BASE		0xc0000000
#define EP93XX_SDCE1_PHYS_BASE		0xd0000000
#define EP93XX_SDCE2_PHYS_BASE		0xe0000000
#define EP93XX_SDCE3_PHYS_BASE_ASYNC	0xf0000000	/* ASDO Pin = 0 */
#define EP93XX_CS0_PHYS_BASE_SYNC	0xf0000000	/* ASDO Pin = 1 */

/*
 * EP93xx linux memory map:
 *
 * virt		phys		size
 * fe800000			5M		per-platform mappings
 * fed00000	80800000	2M		APB
 * fef00000	80000000	1M		AHB
 */

#define EP93XX_AHB_PHYS_BASE		0x80000000
#define EP93XX_AHB_VIRT_BASE		0xfef00000
#define EP93XX_AHB_SIZE			0x00100000

#define EP93XX_AHB_PHYS(x)		(EP93XX_AHB_PHYS_BASE + (x))
#define EP93XX_AHB_IOMEM(x)		IOMEM(EP93XX_AHB_VIRT_BASE + (x))

#define EP93XX_APB_PHYS_BASE		0x80800000
#define EP93XX_APB_VIRT_BASE		0xfed00000
#define EP93XX_APB_SIZE			0x00200000

#define EP93XX_APB_PHYS(x)		(EP93XX_APB_PHYS_BASE + (x))
#define EP93XX_APB_IOMEM(x)		IOMEM(EP93XX_APB_VIRT_BASE + (x))


/* AHB peripherals */
#define EP93XX_DMA_BASE			EP93XX_AHB_IOMEM(0x00000000)

#define EP93XX_ETHERNET_PHYS_BASE	EP93XX_AHB_PHYS(0x00010000)
#define EP93XX_ETHERNET_BASE		EP93XX_AHB_IOMEM(0x00010000)

#define EP93XX_USB_PHYS_BASE		EP93XX_AHB_PHYS(0x00020000)
#define EP93XX_USB_BASE			EP93XX_AHB_IOMEM(0x00020000)

#define EP93XX_RASTER_PHYS_BASE		EP93XX_AHB_PHYS(0x00030000)
#define EP93XX_RASTER_BASE		EP93XX_AHB_IOMEM(0x00030000)

#define EP93XX_GRAPHICS_ACCEL_BASE	EP93XX_AHB_IOMEM(0x00040000)

#define EP93XX_SDRAM_CONTROLLER_BASE	EP93XX_AHB_IOMEM(0x00060000)

#define EP93XX_PCMCIA_CONTROLLER_BASE	EP93XX_AHB_IOMEM(0x00080000)

#define EP93XX_BOOT_ROM_BASE		EP93XX_AHB_IOMEM(0x00090000)

#define EP93XX_IDE_BASE			EP93XX_AHB_IOMEM(0x000a0000)

#define EP93XX_VIC1_BASE		EP93XX_AHB_IOMEM(0x000b0000)

#define EP93XX_VIC2_BASE		EP93XX_AHB_IOMEM(0x000c0000)


/* APB peripherals */
#define EP93XX_TIMER_BASE		EP93XX_APB_IOMEM(0x00010000)

#define EP93XX_I2S_PHYS_BASE		EP93XX_APB_PHYS(0x00020000)
#define EP93XX_I2S_BASE			EP93XX_APB_IOMEM(0x00020000)

#define EP93XX_SECURITY_BASE		EP93XX_APB_IOMEM(0x00030000)

#define EP93XX_GPIO_BASE		EP93XX_APB_IOMEM(0x00040000)
#define EP93XX_GPIO_REG(x)		(EP93XX_GPIO_BASE + (x))
#define EP93XX_GPIO_F_INT_STATUS	EP93XX_GPIO_REG(0x5c)
#define EP93XX_GPIO_A_INT_STATUS	EP93XX_GPIO_REG(0xa0)
#define EP93XX_GPIO_B_INT_STATUS	EP93XX_GPIO_REG(0xbc)
#define EP93XX_GPIO_EEDRIVE		EP93XX_GPIO_REG(0xc8)

#define EP93XX_AAC_PHYS_BASE		EP93XX_APB_PHYS(0x00080000)
#define EP93XX_AAC_BASE			EP93XX_APB_IOMEM(0x00080000)

#define EP93XX_SPI_PHYS_BASE		EP93XX_APB_PHYS(0x000a0000)
#define EP93XX_SPI_BASE			EP93XX_APB_IOMEM(0x000a0000)

#define EP93XX_IRDA_BASE		EP93XX_APB_IOMEM(0x000b0000)

#define EP93XX_UART1_PHYS_BASE		EP93XX_APB_PHYS(0x000c0000)
#define EP93XX_UART1_BASE		EP93XX_APB_IOMEM(0x000c0000)

#define EP93XX_UART2_PHYS_BASE		EP93XX_APB_PHYS(0x000d0000)
#define EP93XX_UART2_BASE		EP93XX_APB_IOMEM(0x000d0000)

#define EP93XX_UART3_PHYS_BASE		EP93XX_APB_PHYS(0x000e0000)
#define EP93XX_UART3_BASE		EP93XX_APB_IOMEM(0x000e0000)

#define EP93XX_KEY_MATRIX_PHYS_BASE	EP93XX_APB_PHYS(0x000f0000)
#define EP93XX_KEY_MATRIX_BASE		EP93XX_APB_IOMEM(0x000f0000)

#define EP93XX_ADC_BASE			EP93XX_APB_IOMEM(0x00100000)
#define EP93XX_TOUCHSCREEN_BASE		EP93XX_APB_IOMEM(0x00100000)

#define EP93XX_PWM_PHYS_BASE		EP93XX_APB_PHYS(0x00110000)
#define EP93XX_PWM_BASE			EP93XX_APB_IOMEM(0x00110000)

#define EP93XX_RTC_PHYS_BASE		EP93XX_APB_PHYS(0x00120000)
#define EP93XX_RTC_BASE			EP93XX_APB_IOMEM(0x00120000)

#define EP93XX_SYSCON_BASE		EP93XX_APB_IOMEM(0x00130000)
#define EP93XX_SYSCON_REG(x)		(EP93XX_SYSCON_BASE + (x))
#define EP93XX_SYSCON_POWER_STATE	EP93XX_SYSCON_REG(0x00)
#define EP93XX_SYSCON_PWRCNT		EP93XX_SYSCON_REG(0x04)
#define EP93XX_SYSCON_PWRCNT_FIR_EN	(1<<31)
#define EP93XX_SYSCON_PWRCNT_UARTBAUD	(1<<29)
#define EP93XX_SYSCON_PWRCNT_USH_EN	(1<<28)
#define EP93XX_SYSCON_PWRCNT_DMA_M2M1	(1<<27)
#define EP93XX_SYSCON_PWRCNT_DMA_M2M0	(1<<26)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P8	(1<<25)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P9	(1<<24)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P6	(1<<23)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P7	(1<<22)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P4	(1<<21)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P5	(1<<20)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P2	(1<<19)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P3	(1<<18)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P0	(1<<17)
#define EP93XX_SYSCON_PWRCNT_DMA_M2P1	(1<<16)
#define EP93XX_SYSCON_HALT		EP93XX_SYSCON_REG(0x08)
#define EP93XX_SYSCON_STANDBY		EP93XX_SYSCON_REG(0x0c)
#define EP93XX_SYSCON_CLKSET1		EP93XX_SYSCON_REG(0x20)
#define EP93XX_SYSCON_CLKSET1_NBYP1	(1<<23)
#define EP93XX_SYSCON_CLKSET2		EP93XX_SYSCON_REG(0x24)
#define EP93XX_SYSCON_CLKSET2_NBYP2	(1<<19)
#define EP93XX_SYSCON_CLKSET2_PLL2_EN	(1<<18)
#define EP93XX_SYSCON_DEVCFG		EP93XX_SYSCON_REG(0x80)
#define EP93XX_SYSCON_DEVCFG_SWRST	(1<<31)
#define EP93XX_SYSCON_DEVCFG_D1ONG	(1<<30)
#define EP93XX_SYSCON_DEVCFG_D0ONG	(1<<29)
#define EP93XX_SYSCON_DEVCFG_IONU2	(1<<28)
#define EP93XX_SYSCON_DEVCFG_GONK	(1<<27)
#define EP93XX_SYSCON_DEVCFG_TONG	(1<<26)
#define EP93XX_SYSCON_DEVCFG_MONG	(1<<25)
#define EP93XX_SYSCON_DEVCFG_U3EN	(1<<24)
#define EP93XX_SYSCON_DEVCFG_CPENA	(1<<23)
#define EP93XX_SYSCON_DEVCFG_A2ONG	(1<<22)
#define EP93XX_SYSCON_DEVCFG_A1ONG	(1<<21)
#define EP93XX_SYSCON_DEVCFG_U2EN	(1<<20)
#define EP93XX_SYSCON_DEVCFG_EXVC	(1<<19)
#define EP93XX_SYSCON_DEVCFG_U1EN	(1<<18)
#define EP93XX_SYSCON_DEVCFG_TIN	(1<<17)
#define EP93XX_SYSCON_DEVCFG_HC3IN	(1<<15)
#define EP93XX_SYSCON_DEVCFG_HC3EN	(1<<14)
#define EP93XX_SYSCON_DEVCFG_HC1IN	(1<<13)
#define EP93XX_SYSCON_DEVCFG_HC1EN	(1<<12)
#define EP93XX_SYSCON_DEVCFG_HONIDE	(1<<11)
#define EP93XX_SYSCON_DEVCFG_GONIDE	(1<<10)
#define EP93XX_SYSCON_DEVCFG_PONG	(1<<9)
#define EP93XX_SYSCON_DEVCFG_EONIDE	(1<<8)
#define EP93XX_SYSCON_DEVCFG_I2SONSSP	(1<<7)
#define EP93XX_SYSCON_DEVCFG_I2SONAC97	(1<<6)
#define EP93XX_SYSCON_DEVCFG_RASONP3	(1<<4)
#define EP93XX_SYSCON_DEVCFG_RAS	(1<<3)
#define EP93XX_SYSCON_DEVCFG_ADCPD	(1<<2)
#define EP93XX_SYSCON_DEVCFG_KEYS	(1<<1)
#define EP93XX_SYSCON_DEVCFG_SHENA	(1<<0)
#define EP93XX_SYSCON_VIDCLKDIV		EP93XX_SYSCON_REG(0x84)
#define EP93XX_SYSCON_CLKDIV_ENABLE	(1<<15)
#define EP93XX_SYSCON_CLKDIV_ESEL	(1<<14)
#define EP93XX_SYSCON_CLKDIV_PSEL	(1<<13)
#define EP93XX_SYSCON_CLKDIV_PDIV_SHIFT	8
#define EP93XX_SYSCON_I2SCLKDIV		EP93XX_SYSCON_REG(0x8c)
#define EP93XX_SYSCON_I2SCLKDIV_SENA	(1<<31)
#define EP93XX_SYSCON_I2SCLKDIV_ORIDE   (1<<29)
#define EP93XX_SYSCON_I2SCLKDIV_SPOL	(1<<19)
#define EP93XX_I2SCLKDIV_SDIV		(1 << 16)
#define EP93XX_I2SCLKDIV_LRDIV32	(0 << 17)
#define EP93XX_I2SCLKDIV_LRDIV64	(1 << 17)
#define EP93XX_I2SCLKDIV_LRDIV128 	(2 << 17)
#define EP93XX_I2SCLKDIV_LRDIV_MASK 	(3 << 17)
#define EP93XX_SYSCON_KEYTCHCLKDIV	EP93XX_SYSCON_REG(0x90)
#define EP93XX_SYSCON_KEYTCHCLKDIV_TSEN	(1<<31)
#define EP93XX_SYSCON_KEYTCHCLKDIV_ADIV	(1<<16)
#define EP93XX_SYSCON_KEYTCHCLKDIV_KEN	(1<<15)
#define EP93XX_SYSCON_KEYTCHCLKDIV_KDIV	(1<<0)
#define EP93XX_SYSCON_SYSCFG		EP93XX_SYSCON_REG(0x9c)
#define EP93XX_SYSCON_SYSCFG_REV_MASK	(0xf0000000)
#define EP93XX_SYSCON_SYSCFG_REV_SHIFT	(28)
#define EP93XX_SYSCON_SYSCFG_SBOOT	(1<<8)
#define EP93XX_SYSCON_SYSCFG_LCSN7	(1<<7)
#define EP93XX_SYSCON_SYSCFG_LCSN6	(1<<6)
#define EP93XX_SYSCON_SYSCFG_LASDO	(1<<5)
#define EP93XX_SYSCON_SYSCFG_LEEDA	(1<<4)
#define EP93XX_SYSCON_SYSCFG_LEECLK	(1<<3)
#define EP93XX_SYSCON_SYSCFG_LCSN2	(1<<1)
#define EP93XX_SYSCON_SYSCFG_LCSN1	(1<<0)
#define EP93XX_SYSCON_SWLOCK		EP93XX_SYSCON_REG(0xc0)

#define EP93XX_WATCHDOG_BASE		EP93XX_APB_IOMEM(0x00140000)


#endif
