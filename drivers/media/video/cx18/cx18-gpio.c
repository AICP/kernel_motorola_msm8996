/*
 *  cx18 gpio functions
 *
 *  Derived from ivtv-gpio.c
 *
 *  Copyright (C) 2007  Hans Verkuil <hverkuil@xs4all.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA
 */

#include "cx18-driver.h"
#include "cx18-cards.h"
#include "cx18-gpio.h"
#include "tuner-xc2028.h"

/********************* GPIO stuffs *********************/

/* GPIO registers */
#define CX18_REG_GPIO_IN     0xc72010
#define CX18_REG_GPIO_OUT1   0xc78100
#define CX18_REG_GPIO_DIR1   0xc78108
#define CX18_REG_GPIO_OUT2   0xc78104
#define CX18_REG_GPIO_DIR2   0xc7810c

/*
 * HVR-1600 GPIO pins, courtesy of Hauppauge:
 *
 * gpio0: zilog ir process reset pin
 * gpio1: zilog programming pin (you should never use this)
 * gpio12: cx24227 reset pin
 * gpio13: cs5345 reset pin
*/

void cx18_gpio_init(struct cx18 *cx)
{
	if (cx->card->gpio_init.direction == 0)
		return;

	CX18_DEBUG_INFO("GPIO initial dir: %08x out: %08x\n",
		   read_reg(CX18_REG_GPIO_DIR1), read_reg(CX18_REG_GPIO_OUT1));

	/* init output data then direction */
	write_reg(cx->card->gpio_init.direction << 16, CX18_REG_GPIO_DIR1);
	write_reg(0, CX18_REG_GPIO_DIR2);
	write_reg((cx->card->gpio_init.direction << 16) |
			cx->card->gpio_init.initial_value, CX18_REG_GPIO_OUT1);
	write_reg(0, CX18_REG_GPIO_OUT2);
}

/* Xceive tuner reset function */
int cx18_reset_tuner_gpio(void *dev, int cmd, int value)
{
	struct i2c_algo_bit_data *algo = dev;
	struct cx18 *cx = algo->data;
/*	int curdir, curout;*/

	if (cmd != XC2028_TUNER_RESET)
		return 0;
	CX18_DEBUG_INFO("Resetting tuner\n");
	return 0;
}
