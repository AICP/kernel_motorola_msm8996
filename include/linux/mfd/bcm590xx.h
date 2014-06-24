/*
 * Broadcom BCM590xx PMU
 *
 * Copyright 2014 Linaro Limited
 * Author: Matt Porter <mporter@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#ifndef __LINUX_MFD_BCM590XX_H
#define __LINUX_MFD_BCM590XX_H

#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

/* max register address */
#define BCM590XX_MAX_REGISTER	0xe7

struct bcm590xx {
	struct device *dev;
	struct i2c_client *i2c_client;
	struct regmap *regmap;
	unsigned int id;
};

#endif /*  __LINUX_MFD_BCM590XX_H */
