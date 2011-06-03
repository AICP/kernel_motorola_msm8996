#ifndef __MACH_MX53_H__
#define __MACH_MX53_H__

/*
 * IROM
 */
#define MX53_IROM_BASE_ADDR		0x0
#define MX53_IROM_SIZE			SZ_64K

/* TZIC */
#define MX53_TZIC_BASE_ADDR		0x0FFFC000

/*
 * AHCI SATA
 */
#define MX53_SATA_BASE_ADDR		0x10000000

/*
 * NFC
 */
#define MX53_NFC_AXI_BASE_ADDR	0xF7FF0000	/* NAND flash AXI */
#define MX53_NFC_AXI_SIZE		SZ_64K

/*
 * IRAM
 */
#define MX53_IRAM_BASE_ADDR	0xF8000000	/* internal ram */
#define MX53_IRAM_PARTITIONS	16
#define MX53_IRAM_SIZE		(MX53_IRAM_PARTITIONS * SZ_8K)	/* 128KB */

/*
 * Graphics Memory of GPU
 */
#define MX53_IPU_CTRL_BASE_ADDR	0x18000000
#define MX53_GPU2D_BASE_ADDR		0x20000000
#define MX53_GPU_BASE_ADDR		0x30000000
#define MX53_GPU_GMEM_BASE_ADDR	0xF8020000

#define MX53_DEBUG_BASE_ADDR		0x40000000
#define MX53_DEBUG_SIZE		SZ_1M
#define MX53_ETB_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00001000)
#define MX53_ETM_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00002000)
#define MX53_TPIU_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00003000)
#define MX53_CTI0_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00004000)
#define MX53_CTI1_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00005000)
#define MX53_CTI2_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00006000)
#define MX53_CTI3_BASE_ADDR		(MX53_DEBUG_BASE_ADDR + 0x00007000)
#define MX53_CORTEX_DBG_BASE_ADDR	(MX53_DEBUG_BASE_ADDR + 0x00008000)

/*
 * SPBA global module enabled #0
 */
#define MX53_SPBA0_BASE_ADDR		0x50000000
#define MX53_SPBA0_SIZE		SZ_1M

#define MX53_ESDHC1_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00004000)
#define MX53_ESDHC2_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00008000)
#define MX53_UART3_BASE_ADDR		(MX53_SPBA0_BASE_ADDR + 0x0000C000)
#define MX53_ECSPI1_BASE_ADDR		(MX53_SPBA0_BASE_ADDR + 0x00010000)
#define MX53_SSI2_BASE_ADDR		(MX53_SPBA0_BASE_ADDR + 0x00014000)
#define MX53_ESDHC3_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00020000)
#define MX53_ESDHC4_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00024000)
#define MX53_SPDIF_BASE_ADDR		(MX53_SPBA0_BASE_ADDR + 0x00028000)
#define MX53_ASRC_BASE_ADDR		(MX53_SPBA0_BASE_ADDR + 0x0002C000)
#define MX53_ATA_DMA_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00030000)
#define MX53_SLIM_DMA_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00034000)
#define MX53_HSI2C_DMA_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x00038000)
#define MX53_SPBA_CTRL_BASE_ADDR	(MX53_SPBA0_BASE_ADDR + 0x0003C000)

/*
 * AIPS 1
 */
#define MX53_AIPS1_BASE_ADDR	0x53F00000
#define MX53_AIPS1_SIZE		SZ_1M

#define MX53_OTG_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x00080000)
#define MX53_GPIO1_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x00084000)
#define MX53_GPIO2_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x00088000)
#define MX53_GPIO3_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x0008C000)
#define MX53_GPIO4_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x00090000)
#define MX53_KPP_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x00094000)
#define MX53_WDOG1_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x00098000)
#define MX53_WDOG2_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x0009C000)
#define MX53_GPT1_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000A0000)
#define MX53_SRTC_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000A4000)
#define MX53_IOMUXC_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000A8000)
#define MX53_EPIT1_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000AC000)
#define MX53_EPIT2_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000B0000)
#define MX53_PWM1_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000B4000)
#define MX53_PWM2_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000B8000)
#define MX53_UART1_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000BC000)
#define MX53_UART2_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000C0000)
#define MX53_SRC_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000D0000)
#define MX53_CCM_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000D4000)
#define MX53_GPC_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000D8000)
#define MX53_GPIO5_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000DC000)
#define MX53_GPIO6_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000E0000)
#define MX53_GPIO7_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000E4000)
#define MX53_ATA_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000E8000)
#define MX53_I2C3_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000EC000)
#define MX53_UART4_BASE_ADDR	(MX53_AIPS1_BASE_ADDR + 0x000F0000)

/*
 * AIPS 2
 */
#define MX53_AIPS2_BASE_ADDR		0x63F00000
#define MX53_AIPS2_SIZE			SZ_1M

#define MX53_PLL1_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x00080000)
#define MX53_PLL2_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x00084000)
#define MX53_PLL3_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x00088000)
#define MX53_PLL4_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x0008C000)
#define MX53_UART5_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x00090000)
#define MX53_AHBMAX_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x00094000)
#define MX53_IIM_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x00098000)
#define MX53_CSU_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x0009C000)
#define MX53_ARM_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000A0000)
#define MX53_OWIRE_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000A4000)
#define MX53_FIRI_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000A8000)
#define MX53_ECSPI2_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000AC000)
#define MX53_SDMA_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000B0000)
#define MX53_SCC_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000B4000)
#define MX53_ROMCP_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000B8000)
#define MX53_RTIC_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000BC000)
#define MX53_CSPI_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000C0000)
#define MX53_I2C2_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000C4000)
#define MX53_I2C1_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000C8000)
#define MX53_SSI1_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000CC000)
#define MX53_AUDMUX_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000D0000)
#define MX53_RTC_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000D4000)
#define MX53_M4IF_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000D8000)
#define MX53_ESDCTL_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000D9000)
#define MX53_WEIM_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000DA000)
#define MX53_NFC_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000DB000)
#define MX53_EMI_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000DBF00)
#define MX53_MIPI_HSC_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000DC000)
#define MX53_MLB_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000E4000)
#define MX53_SSI3_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000E8000)
#define MX53_FEC_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000EC000)
#define MX53_TVE_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000F0000)
#define MX53_VPU_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000F4000)
#define MX53_SAHARA_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000F8000)
#define MX53_PTP_BASE_ADDR	(MX53_AIPS2_BASE_ADDR + 0x000FC000)

/*
 * Memory regions and CS
 */
#define MX53_CSD0_BASE_ADDR		0x90000000
#define MX53_CSD1_BASE_ADDR		0xA0000000
#define MX53_CS0_BASE_ADDR		0xF0000000
#define MX53_CS1_32MB_BASE_ADDR	0xF2000000
#define MX53_CS1_64MB_BASE_ADDR		0xF4000000
#define MX53_CS2_64MB_BASE_ADDR		0xF4000000
#define MX53_CS2_96MB_BASE_ADDR		0xF6000000
#define MX53_CS3_BASE_ADDR		0xF6000000

#define MX53_IO_P2V(x)			IMX_IO_P2V(x)
#define MX53_IO_ADDRESS(x)		IOMEM(MX53_IO_P2V(x))

/*
 * defines for SPBA modules
 */
#define MX53_SPBA_SDHC1	0x04
#define MX53_SPBA_SDHC2	0x08
#define MX53_SPBA_UART3	0x0C
#define MX53_SPBA_CSPI1	0x10
#define MX53_SPBA_SSI2		0x14
#define MX53_SPBA_SDHC3	0x20
#define MX53_SPBA_SDHC4	0x24
#define MX53_SPBA_SPDIF	0x28
#define MX53_SPBA_ATA		0x30
#define MX53_SPBA_SLIM		0x34
#define MX53_SPBA_HSI2C	0x38
#define MX53_SPBA_CTRL		0x3C

/*
 * DMA request assignments
 */
#define MX53_DMA_REQ_SSI3_TX1		47
#define MX53_DMA_REQ_SSI3_RX1		46
#define MX53_DMA_REQ_SSI3_TX2		45
#define MX53_DMA_REQ_SSI3_RX2		44
#define MX53_DMA_REQ_UART3_TX	43
#define MX53_DMA_REQ_UART3_RX	42
#define MX53_DMA_REQ_ESAI_TX		41
#define MX53_DMA_REQ_ESAI_RX		40
#define MX53_DMA_REQ_CSPI_TX		39
#define MX53_DMA_REQ_CSPI_RX		38
#define MX53_DMA_REQ_ASRC_DMA6	37
#define MX53_DMA_REQ_ASRC_DMA5	36
#define MX53_DMA_REQ_ASRC_DMA4	35
#define MX53_DMA_REQ_ASRC_DMA3	34
#define MX53_DMA_REQ_ASRC_DMA2	33
#define MX53_DMA_REQ_ASRC_DMA1	32
#define MX53_DMA_REQ_EMI_WR		31
#define MX53_DMA_REQ_EMI_RD		30
#define MX53_DMA_REQ_SSI1_TX1		29
#define MX53_DMA_REQ_SSI1_RX1		28
#define MX53_DMA_REQ_SSI1_TX2		27
#define MX53_DMA_REQ_SSI1_RX2		26
#define MX53_DMA_REQ_SSI2_TX1		25
#define MX53_DMA_REQ_SSI2_RX1		24
#define MX53_DMA_REQ_SSI2_TX2		23
#define MX53_DMA_REQ_SSI2_RX2		22
#define MX53_DMA_REQ_I2C2_SDHC2	21
#define MX53_DMA_REQ_I2C1_SDHC1	20
#define MX53_DMA_REQ_UART1_TX	19
#define MX53_DMA_REQ_UART1_RX	18
#define MX53_DMA_REQ_UART5_TX	17
#define MX53_DMA_REQ_UART5_RX	16
#define MX53_DMA_REQ_SPDIF_TX		15
#define MX53_DMA_REQ_SPDIF_RX		14
#define MX53_DMA_REQ_UART2_FIRI_TX	13
#define MX53_DMA_REQ_UART2_FIRI_RX	12
#define MX53_DMA_REQ_SDHC4		11
#define MX53_DMA_REQ_I2C3_SDHC3	10
#define MX53_DMA_REQ_CSPI2_TX		9
#define MX53_DMA_REQ_CSPI2_RX		8
#define MX53_DMA_REQ_CSPI1_TX		7
#define MX53_DMA_REQ_CSPI1_RX		6
#define MX53_DMA_REQ_IPU		5
#define MX53_DMA_REQ_ATA_TX_END	4
#define MX53_DMA_REQ_ATA_UART4_TX	3
#define MX53_DMA_REQ_ATA_UART4_RX	2
#define MX53_DMA_REQ_GPC		1
#define MX53_DMA_REQ_VPU		0

/*
 * Interrupt numbers
 */
#define MX53_INT_RESV0		0
#define MX53_INT_ESDHC1	1
#define MX53_INT_ESDHC2	2
#define MX53_INT_ESDHC3	3
#define MX53_INT_ESDHC4	4
#define MX53_INT_RESV5	5
#define MX53_INT_SDMA	6
#define MX53_INT_IOMUX	7
#define MX53_INT_NFC	8
#define MX53_INT_VPU	9
#define MX53_INT_IPU_ERR	10
#define MX53_INT_IPU_SYN	11
#define MX53_INT_GPU	12
#define MX53_INT_RESV13	13
#define MX53_INT_USB_H1	14
#define MX53_INT_EMI	15
#define MX53_INT_USB_H2	16
#define MX53_INT_USB_H3	17
#define MX53_INT_USB_OTG	18
#define MX53_INT_SAHARA_H0	19
#define MX53_INT_SAHARA_H1	20
#define MX53_INT_SCC_SMN	21
#define MX53_INT_SCC_STZ	22
#define MX53_INT_SCC_SCM	23
#define MX53_INT_SRTC_NTZ	24
#define MX53_INT_SRTC_TZ	25
#define MX53_INT_RTIC	26
#define MX53_INT_CSU	27
#define MX53_INT_SATA	28
#define MX53_INT_SSI1	29
#define MX53_INT_SSI2	30
#define MX53_INT_UART1	31
#define MX53_INT_UART2	32
#define MX53_INT_UART3	33
#define MX53_INT_RESV34	34
#define MX53_INT_RESV35	35
#define MX53_INT_ECSPI1	36
#define MX53_INT_ECSPI2	37
#define MX53_INT_CSPI	38
#define MX53_INT_GPT	39
#define MX53_INT_EPIT1	40
#define MX53_INT_EPIT2	41
#define MX53_INT_GPIO1_INT7	42
#define MX53_INT_GPIO1_INT6	43
#define MX53_INT_GPIO1_INT5	44
#define MX53_INT_GPIO1_INT4	45
#define MX53_INT_GPIO1_INT3	46
#define MX53_INT_GPIO1_INT2	47
#define MX53_INT_GPIO1_INT1	48
#define MX53_INT_GPIO1_INT0	49
#define MX53_INT_GPIO1_LOW	50
#define MX53_INT_GPIO1_HIGH	51
#define MX53_INT_GPIO2_LOW	52
#define MX53_INT_GPIO2_HIGH	53
#define MX53_INT_GPIO3_LOW	54
#define MX53_INT_GPIO3_HIGH	55
#define MX53_INT_GPIO4_LOW	56
#define MX53_INT_GPIO4_HIGH	57
#define MX53_INT_WDOG1	58
#define MX53_INT_WDOG2	59
#define MX53_INT_KPP	60
#define MX53_INT_PWM1	61
#define MX53_INT_I2C1	62
#define MX53_INT_I2C2	63
#define MX53_INT_I2C3	64
#define MX53_INT_RESV65	65
#define MX53_INT_RESV66	66
#define MX53_INT_SPDIF	67
#define MX53_INT_SIM_DAT	68
#define MX53_INT_IIM	69
#define MX53_INT_ATA	70
#define MX53_INT_CCM1	71
#define MX53_INT_CCM2	72
#define MX53_INT_GPC1	73
#define MX53_INT_GPC2	74
#define MX53_INT_SRC	75
#define MX53_INT_NM		76
#define MX53_INT_PMU	77
#define MX53_INT_CTI_IRQ	78
#define MX53_INT_CTI1_TG0	79
#define MX53_INT_CTI1_TG1	80
#define MX53_INT_ESAI	81
#define MX53_INT_CAN1	82
#define MX53_INT_CAN2	83
#define MX53_INT_GPU2_IRQ	84
#define MX53_INT_GPU2_BUSY	85
#define MX53_INT_RESV86	86
#define MX53_INT_FEC	87
#define MX53_INT_OWIRE	88
#define MX53_INT_CTI1_TG2	89
#define MX53_INT_SJC	90
#define MX53_INT_TVE	92
#define MX53_INT_FIRI	93
#define MX53_INT_PWM2	94
#define MX53_INT_SLIM_EXP	95
#define MX53_INT_SSI3	96
#define MX53_INT_EMI_BOOT	97
#define MX53_INT_CTI1_TG3	98
#define MX53_INT_SMC_RX	99
#define MX53_INT_VPU_IDLE	100
#define MX53_INT_EMI_NFC	101
#define MX53_INT_GPU_IDLE	102
#define MX53_INT_GPIO5_LOW	103
#define MX53_INT_GPIO5_HIGH	104
#define MX53_INT_GPIO6_LOW	105
#define MX53_INT_GPIO6_HIGH	106
#define MX53_INT_GPIO7_LOW	107
#define MX53_INT_GPIO7_HIGH	108

#endif /* ifndef __MACH_MX53_H__ */
