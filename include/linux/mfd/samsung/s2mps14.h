/*
 * s2mps14.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_MFD_S2MPS14_H
#define __LINUX_MFD_S2MPS14_H

/* S2MPS14 registers */
enum s2mps14_reg {
	S2MPS14_REG_ID,
	S2MPS14_REG_INT1,
	S2MPS14_REG_INT2,
	S2MPS14_REG_INT3,
	S2MPS14_REG_INT1M,
	S2MPS14_REG_INT2M,
	S2MPS14_REG_INT3M,
	S2MPS14_REG_ST1,
	S2MPS14_REG_ST2,
	S2MPS14_REG_PWRONSRC,
	S2MPS14_REG_OFFSRC,
	S2MPS14_REG_BU_CHG,
	S2MPS14_REG_RTCCTRL,
	S2MPS14_REG_CTRL1,
	S2MPS14_REG_CTRL2,
	S2MPS14_REG_RSVD1,
	S2MPS14_REG_RSVD2,
	S2MPS14_REG_RSVD3,
	S2MPS14_REG_RSVD4,
	S2MPS14_REG_RSVD5,
	S2MPS14_REG_RSVD6,
	S2MPS14_REG_CTRL3,
	S2MPS14_REG_RSVD7,
	S2MPS14_REG_RSVD8,
	S2MPS14_REG_WRSTBI,
	S2MPS14_REG_B1CTRL1,
	S2MPS14_REG_B1CTRL2,
	S2MPS14_REG_B2CTRL1,
	S2MPS14_REG_B2CTRL2,
	S2MPS14_REG_B3CTRL1,
	S2MPS14_REG_B3CTRL2,
	S2MPS14_REG_B4CTRL1,
	S2MPS14_REG_B4CTRL2,
	S2MPS14_REG_B5CTRL1,
	S2MPS14_REG_B5CTRL2,
	S2MPS14_REG_L1CTRL,
	S2MPS14_REG_L2CTRL,
	S2MPS14_REG_L3CTRL,
	S2MPS14_REG_L4CTRL,
	S2MPS14_REG_L5CTRL,
	S2MPS14_REG_L6CTRL,
	S2MPS14_REG_L7CTRL,
	S2MPS14_REG_L8CTRL,
	S2MPS14_REG_L9CTRL,
	S2MPS14_REG_L10CTRL,
	S2MPS14_REG_L11CTRL,
	S2MPS14_REG_L12CTRL,
	S2MPS14_REG_L13CTRL,
	S2MPS14_REG_L14CTRL,
	S2MPS14_REG_L15CTRL,
	S2MPS14_REG_L16CTRL,
	S2MPS14_REG_L17CTRL,
	S2MPS14_REG_L18CTRL,
	S2MPS14_REG_L19CTRL,
	S2MPS14_REG_L20CTRL,
	S2MPS14_REG_L21CTRL,
	S2MPS14_REG_L22CTRL,
	S2MPS14_REG_L23CTRL,
	S2MPS14_REG_L24CTRL,
	S2MPS14_REG_L25CTRL,
	S2MPS14_REG_LDODSCH1,
	S2MPS14_REG_LDODSCH2,
	S2MPS14_REG_LDODSCH3,
};

/* S2MPS14 regulator ids */
enum s2mps14_regulators {
	S2MPS14_LDO1,
	S2MPS14_LDO2,
	S2MPS14_LDO3,
	S2MPS14_LDO4,
	S2MPS14_LDO5,
	S2MPS14_LDO6,
	S2MPS14_LDO7,
	S2MPS14_LDO8,
	S2MPS14_LDO9,
	S2MPS14_LDO10,
	S2MPS14_LDO11,
	S2MPS14_LDO12,
	S2MPS14_LDO13,
	S2MPS14_LDO14,
	S2MPS14_LDO15,
	S2MPS14_LDO16,
	S2MPS14_LDO17,
	S2MPS14_LDO18,
	S2MPS14_LDO19,
	S2MPS14_LDO20,
	S2MPS14_LDO21,
	S2MPS14_LDO22,
	S2MPS14_LDO23,
	S2MPS14_LDO24,
	S2MPS14_LDO25,
	S2MPS14_BUCK1,
	S2MPS14_BUCK2,
	S2MPS14_BUCK3,
	S2MPS14_BUCK4,
	S2MPS14_BUCK5,

	S2MPS14_REGULATOR_MAX,
};

/* Regulator constraints for BUCKx */
#define S2MPS14_BUCK1235_MIN_600MV	600000
#define S2MPS14_BUCK4_MIN_1400MV	1400000
#define S2MPS14_BUCK1235_STEP_6_25MV	6250
#define S2MPS14_BUCK4_STEP_12_5MV	12500
#define S2MPS14_BUCK1235_START_SEL	0x20
#define S2MPS14_BUCK4_START_SEL		0x40
/*
 * Default ramp delay in uv/us. Datasheet says that ramp delay can be
 * controlled however it does not specify which register is used for that.
 * Let's assume that default value will be set.
 */
#define S2MPS14_BUCK_RAMP_DELAY		12500

/* Regulator constraints for different types of LDOx */
#define S2MPS14_LDO_MIN_800MV		800000
#define S2MPS14_LDO_MIN_1800MV		1800000
#define S2MPS14_LDO_STEP_12_5MV		12500
#define S2MPS14_LDO_STEP_25MV		25000

#define S2MPS14_LDO_VSEL_MASK		0x3F
#define S2MPS14_BUCK_VSEL_MASK		0xFF
#define S2MPS14_ENABLE_MASK		(0x03 << S2MPS14_ENABLE_SHIFT)
#define S2MPS14_ENABLE_SHIFT		6
/* On/Off controlled by PWREN */
#define S2MPS14_ENABLE_SUSPEND		(0x01 << S2MPS14_ENABLE_SHIFT)
/* On/Off controlled by LDO10EN or EMMCEN */
#define S2MPS14_ENABLE_EXT_CONTROL	(0x00 << S2MPS14_ENABLE_SHIFT)
#define S2MPS14_LDO_N_VOLTAGES		(S2MPS14_LDO_VSEL_MASK + 1)
#define S2MPS14_BUCK_N_VOLTAGES		(S2MPS14_BUCK_VSEL_MASK + 1)

#endif /*  __LINUX_MFD_S2MPS14_H */
