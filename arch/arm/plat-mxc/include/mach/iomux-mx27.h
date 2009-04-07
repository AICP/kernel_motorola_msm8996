/*
* Copyright (C) 2008 by Sascha Hauer <kernel@pengutronix.de>
* Copyright (C) 2009 by Holger Schurig <hs4233@mail.mn-solutions.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA 02110-1301, USA.
*/

#ifndef _MXC_IOMUX_MX27_H
#define _MXC_IOMUX_MX27_H

#ifndef GPIO_PORTA
#error Please include mach/iomux.h
#endif


/* Primary GPIO pin functions */

#define PA0_PF_USBH2_CLK        (GPIO_PORTA | GPIO_PF | 0)
#define PA1_PF_USBH2_DIR        (GPIO_PORTA | GPIO_PF | 1)
#define PA2_PF_USBH2_DATA7      (GPIO_PORTA | GPIO_PF | 2)
#define PA3_PF_USBH2_NXT        (GPIO_PORTA | GPIO_PF | 3)
#define PA4_PF_USBH2_STP        (GPIO_PORTA | GPIO_PF | 4)
#define PB22_PF_USBH1_SUSP      (GPIO_PORTB | GPIO_PF | 22)
#define PB25_PF_USBH1_RCV       (GPIO_PORTB | GPIO_PF | 25)
#define PC5_PF_I2C2_SDA         (GPIO_PORTC | GPIO_PF | GPIO_IN | 5)
#define PC6_PF_I2C2_SCL         (GPIO_PORTC | GPIO_PF | GPIO_IN | 6)
#define PC7_PF_USBOTG_DATA5     (GPIO_PORTC | GPIO_PF | GPIO_OUT | 7)
#define PC8_PF_USBOTG_DATA6     (GPIO_PORTC | GPIO_PF | GPIO_OUT | 8)
#define PC9_PF_USBOTG_DATA0     (GPIO_PORTC | GPIO_PF | GPIO_OUT | 9)
#define PC10_PF_USBOTG_DATA2    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 10)
#define PC11_PF_USBOTG_DATA1    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 11)
#define PC12_PF_USBOTG_DATA4    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 12)
#define PC13_PF_USBOTG_DATA3    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 13)
#define PC16_PF_SSI4_FS         (GPIO_PORTC | GPIO_PF | GPIO_IN | 16)
#define PC17_PF_SSI4_RXD        (GPIO_PORTC | GPIO_PF | GPIO_IN | 17)
#define PC18_PF_SSI4_TXD        (GPIO_PORTC | GPIO_PF | GPIO_IN | 18)
#define PC19_PF_SSI4_CLK        (GPIO_PORTC | GPIO_PF | GPIO_IN | 19)
#define PC25_AF_GPT5_TIN        (GPIO_PORTC | GPIO_AF | 25)
#define PC27_AF_GPT4_TIN        (GPIO_PORTC | GPIO_AF | 27)
#define PD0_PF_SD3_CMD          (GPIO_PORTD | GPIO_PF | 0)
#define PD1_PF_SD3_CLK          (GPIO_PORTD | GPIO_PF | 1)
#define PD2_PF_ATA_DATA0        (GPIO_PORTD | GPIO_PF | 2)
#define PD3_PF_ATA_DATA1        (GPIO_PORTD | GPIO_PF | 3)
#define PD4_PF_ATA_DATA2        (GPIO_PORTD | GPIO_PF | 4)
#define PD5_PF_ATA_DATA3        (GPIO_PORTD | GPIO_PF | 5)
#define PD6_PF_ATA_DATA4        (GPIO_PORTD | GPIO_PF | 6)
#define PD7_PF_ATA_DATA5        (GPIO_PORTD | GPIO_PF | 7)
#define PD8_PF_ATA_DATA6        (GPIO_PORTD | GPIO_PF | 8)
#define PD9_PF_ATA_DATA7        (GPIO_PORTD | GPIO_PF | 9)
#define PD10_PF_ATA_DATA8       (GPIO_PORTD | GPIO_PF | 10)
#define PD11_PF_ATA_DATA9       (GPIO_PORTD | GPIO_PF | 11)
#define PD12_PF_ATA_DATA10      (GPIO_PORTD | GPIO_PF | 12)
#define PD13_PF_ATA_DATA11      (GPIO_PORTD | GPIO_PF | 13)
#define PD14_PF_ATA_DATA12      (GPIO_PORTD | GPIO_PF | 14)
#define PD15_PF_ATA_DATA13      (GPIO_PORTD | GPIO_PF | 15)
#define PD16_PF_ATA_DATA14      (GPIO_PORTD | GPIO_PF | 16)
#define PE0_PF_USBOTG_NXT       (GPIO_PORTE | GPIO_PF | GPIO_OUT | 0)
#define PE1_PF_USBOTG_STP       (GPIO_PORTE | GPIO_PF | GPIO_OUT | 1)
#define PE2_PF_USBOTG_DIR       (GPIO_PORTE | GPIO_PF | GPIO_OUT | 2)
#define PE24_PF_USBOTG_CLK      (GPIO_PORTE | GPIO_PF | GPIO_OUT | 24)
#define PE25_PF_USBOTG_DATA7    (GPIO_PORTE | GPIO_PF | GPIO_OUT | 25)
#define PF1_PF_NFCLE            (GPIO_PORTF | GPIO_PF | 1)
#define PF3_PF_NFCE             (GPIO_PORTF | GPIO_PF | 3)
#define PF7_PF_PC_POE           (GPIO_PORTF | GPIO_PF | 7)
#define PF8_PF_PC_RW            (GPIO_PORTF | GPIO_PF | 8)
#define PF9_PF_PC_IOIS16        (GPIO_PORTF | GPIO_PF | 9)
#define PF10_PF_PC_RST          (GPIO_PORTF | GPIO_PF | 10)
#define PF11_PF_PC_BVD2         (GPIO_PORTF | GPIO_PF | 11)
#define PF12_PF_PC_BVD1         (GPIO_PORTF | GPIO_PF | 12)
#define PF13_PF_PC_VS2          (GPIO_PORTF | GPIO_PF | 13)
#define PF14_PF_PC_VS1          (GPIO_PORTF | GPIO_PF | 14)
#define PF16_PF_PC_PWRON        (GPIO_PORTF | GPIO_PF | 16)
#define PF17_PF_PC_READY        (GPIO_PORTF | GPIO_PF | 17)
#define PF18_PF_PC_WAIT         (GPIO_PORTF | GPIO_PF | 18)
#define PF19_PF_PC_CD2          (GPIO_PORTF | GPIO_PF | 19)
#define PF20_PF_PC_CD1          (GPIO_PORTF | GPIO_PF | 20)
#define PF23_PF_ATA_DATA15      (GPIO_PORTF | GPIO_PF | 23)

/* Alternate GPIO pin functions */

#define PB4_AF_MSHC_DATA0       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 4)
#define PB5_AF_MSHC_DATA1       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 5)
#define PB6_AF_MSHC_DATA2       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 6)
#define PB7_AF_MSHC_DATA4       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 7)
#define PB8_AF_MSHC_BS          (GPIO_PORTB | GPIO_AF | GPIO_OUT | 8)
#define PB9_AF_MSHC_SCLK        (GPIO_PORTB | GPIO_AF | GPIO_OUT | 9)
#define PB10_AF_UART6_TXD       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 10)
#define PB11_AF_UART6_RXD       (GPIO_PORTB | GPIO_AF | GPIO_IN | 11)
#define PB12_AF_UART6_CTS       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 12)
#define PB13_AF_UART6_RTS       (GPIO_PORTB | GPIO_AF | GPIO_IN | 13)
#define PB18_AF_UART5_TXD       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 18)
#define PB19_AF_UART5_RXD       (GPIO_PORTB | GPIO_AF | GPIO_IN | 19)
#define PB20_AF_UART5_CTS       (GPIO_PORTB | GPIO_AF | GPIO_OUT | 20)
#define PB21_AF_UART5_RTS       (GPIO_PORTB | GPIO_AF | GPIO_IN | 21)
#define PC8_AF_FEC_MDIO         (GPIO_PORTC | GPIO_AF | GPIO_IN | 8)
#define PC24_AF_GPT5_TOUT       (GPIO_PORTC | GPIO_AF | 24)
#define PC26_AF_GPT4_TOUT       (GPIO_PORTC | GPIO_AF | 26)
#define PD1_AF_ETMTRACE_PKT15   (GPIO_PORTD | GPIO_AF | 1)
#define PD6_AF_ETMTRACE_PKT14   (GPIO_PORTD | GPIO_AF | 6)
#define PD7_AF_ETMTRACE_PKT13   (GPIO_PORTD | GPIO_AF | 7)
#define PD9_AF_ETMTRACE_PKT12   (GPIO_PORTD | GPIO_AF | 9)
#define PD2_AF_SD3_D0           (GPIO_PORTD | GPIO_AF | 2)
#define PD3_AF_SD3_D1           (GPIO_PORTD | GPIO_AF | 3)
#define PD4_AF_SD3_D2           (GPIO_PORTD | GPIO_AF | 4)
#define PD5_AF_SD3_D3           (GPIO_PORTD | GPIO_AF | 5)
#define PD8_AF_FEC_MDIO         (GPIO_PORTD | GPIO_AF | GPIO_IN | 8)
#define PD10_AF_ETMTRACE_PKT11  (GPIO_PORTD | GPIO_AF | 10)
#define PD11_AF_ETMTRACE_PKT10  (GPIO_PORTD | GPIO_AF | 11)
#define PD12_AF_ETMTRACE_PKT9   (GPIO_PORTD | GPIO_AF | 12)
#define PD13_AF_ETMTRACE_PKT8   (GPIO_PORTD | GPIO_AF | 13)
#define PD14_AF_ETMTRACE_PKT7   (GPIO_PORTD | GPIO_AF | 14)
#define PD15_AF_ETMTRACE_PKT6   (GPIO_PORTD | GPIO_AF | 15)
#define PD16_AF_ETMTRACE_PKT5   (GPIO_PORTD | GPIO_AF | 16)
#define PF1_AF_ETMTRACE_PKT0    (GPIO_PORTF | GPIO_AF | 1)
#define PF3_AF_ETMTRACE_PKT2    (GPIO_PORTF | GPIO_AF | 3)
#define PF5_AF_ETMPIPESTAT11    (GPIO_PORTF | GPIO_AF | 5)
#define PF7_AF_ATA_BUFFER_EN    (GPIO_PORTF | GPIO_AF | 7)
#define PF8_AF_ATA_IORDY        (GPIO_PORTF | GPIO_AF | 8)
#define PF9_AF_ATA_INTRQ        (GPIO_PORTF | GPIO_AF | 9)
#define PF10_AF_ATA_RESET       (GPIO_PORTF | GPIO_AF | 10)
#define PF11_AF_ATA_DMACK       (GPIO_PORTF | GPIO_AF | 11)
#define PF12_AF_ATA_DMAREQ      (GPIO_PORTF | GPIO_AF | 12)
#define PF13_AF_ATA_DA0         (GPIO_PORTF | GPIO_AF | 13)
#define PF14_AF_ATA_DA1         (GPIO_PORTF | GPIO_AF | 14)
#define PF15_AF_ETMTRACE_SYNC   (GPIO_PORTF | GPIO_AF | 15)
#define PF16_AF_ATA_DA2         (GPIO_PORTF | GPIO_AF | 16)
#define PF17_AF_ATA_CS0         (GPIO_PORTF | GPIO_AF | 17)
#define PF18_AF_ATA_CS1         (GPIO_PORTF | GPIO_AF | 18)
#define PF19_AF_ATA_DIOW        (GPIO_PORTF | GPIO_AF | 19)
#define PF20_AF_ATA_DIOR        (GPIO_PORTF | GPIO_AF | 20)
#define PF22_AF_ETMTRACE_CLK    (GPIO_PORTF | GPIO_AF | 22)
#define PF23_AF_ETMTRACE_PKT4   (GPIO_PORTF | GPIO_AF | 23)

/* AIN GPIO pin functions */

#define PC14_AIN_SSI1_MCLK      (GPIO_PORTC | GPIO_AIN | GPIO_OUT | 14)
#define PC15_AIN_GPT6_TOUT      (GPIO_PORTC | GPIO_AIN | GPIO_OUT | 15)
#define PD0_AIN_FEC_TXD0        (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 0)
#define PD1_AIN_FEC_TXD1        (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 1)
#define PD2_AIN_FEC_TXD2        (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 2)
#define PD3_AIN_FEC_TXD3        (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 3)
#define PD9_AIN_FEC_MDC         (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 9)
#define PD16_AIN_FEC_TX_ER      (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 16)
#define PD27_AIN_EXT_DMA_GRANT  (GPIO_PORTD | GPIO_AIN | GPIO_OUT | 27)
#define PF23_AIN_FEC_TX_EN      (GPIO_PORTF | GPIO_AIN | GPIO_OUT | 23)

/* BIN GPIO pin functions */

#define PC14_BIN_SSI2_MCLK      (GPIO_PORTC | GPIO_BIN | GPIO_OUT | 14)

/* CIN GPIO pin functions */

#define PD2_CIN_SLCDC1_DAT0     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 2)
#define PD3_CIN_SLCDC1_DAT1     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 3)
#define PD4_CIN_SLCDC1_DAT2     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 4)
#define PD5_CIN_SLCDC1_DAT3     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 5)
#define PD6_CIN_SLCDC1_DAT4     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 6)
#define PD7_CIN_SLCDC1_DAT5     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 7)
#define PD8_CIN_SLCDC1_DAT6     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 8)
#define PD9_CIN_SLCDC1_DAT7     (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 9)
#define PD10_CIN_SLCDC1_DAT8    (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 10)
#define PD11_CIN_SLCDC1_DAT9    (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 11)
#define PD12_CIN_SLCDC1_DAT10   (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 12)
#define PD13_CIN_SLCDC1_DAT11   (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 13)
#define PD14_CIN_SLCDC1_DAT12   (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 14)
#define PD15_CIN_SLCDC1_DAT13   (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 15)
#define PD16_CIN_SLCDC1_DAT14   (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 16)
#define PD23_CIN_SLCDC1_DAT15   (GPIO_PORTD | GPIO_CIN | GPIO_OUT | 23)
#define PF27_CIN_EXT_DMA_GRANT  (GPIO_PORTF | GPIO_CIN | GPIO_OUT | 27)
/* LCDC_TESTx on PBxx omitted, because it's not clear what they do */

/* AOUT GPIO pin functions */

#define PC14_AOUT_GPT6_TIN      (GPIO_PORTC | GPIO_AOUT | GPIO_IN | 14)
#define PD4_AOUT_FEC_RX_ER      (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 4)
#define PD5_AOUT_FEC_RXD1       (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 5)
#define PD6_AOUT_FEC_RXD2       (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 6)
#define PD7_AOUT_FEC_RXD3       (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 7)
#define PD10_AOUT_FEC_CRS       (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 10)
#define PD11_AOUT_FEC_TX_CLK    (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 11)
#define PD12_AOUT_FEC_RXD0      (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 12)
#define PD13_AOUT_FEC_RX_DV     (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 13)
#define PD14_AOUT_FEC_RX_CLK    (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 14)
#define PD15_AOUT_FEC_COL       (GPIO_PORTD | GPIO_AOUT | GPIO_IN | 15)

#define PC17_BOUT_PC_IOIS16     (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 17)
#define PC18_BOUT_PC_BVD2       (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 18)
#define PC19_BOUT_PC_BVD1       (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 19)
#define PC28_BOUT_PC_BVD2       (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 28)
#define PC29_BOUT_PC_VS1        (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 29)
#define PC30_BOUT_PC_READY      (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 30)
#define PC31_BOUT_PC_WAIT       (GPIO_PORTC | GPIO_BOUT | GPIO_IN | 31)


#endif /* _MXC_GPIO_MX1_MX2_H */
