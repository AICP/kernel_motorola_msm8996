/*
 * Analog Devices AD9389B/AD9889B video encoder driver
 *
 * Copyright 2012 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * References (c = chapter, p = page):
 * REF_01 - Analog Devices, Programming Guide, AD9889B/AD9389B,
 * HDMI Transitter, Rev. A, October 2010
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/workqueue.h>
#include <linux/v4l2-dv-timings.h>
#include <media/v4l2-device.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/ad9389b.h>

static int debug;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "debug level (0-2)");

MODULE_DESCRIPTION("Analog Devices AD9389B/AD9889B video encoder driver");
MODULE_AUTHOR("Hans Verkuil <hans.verkuil@cisco.com>");
MODULE_AUTHOR("Martin Bugge <marbugge@cisco.com>");
MODULE_LICENSE("GPL");

#define MASK_AD9389B_EDID_RDY_INT   0x04
#define MASK_AD9389B_MSEN_INT       0x40
#define MASK_AD9389B_HPD_INT        0x80

#define MASK_AD9389B_HPD_DETECT     0x40
#define MASK_AD9389B_MSEN_DETECT    0x20
#define MASK_AD9389B_EDID_RDY       0x10

#define EDID_MAX_RETRIES (8)
#define EDID_DELAY 250
#define EDID_MAX_SEGM 8

/*
**********************************************************************
*
*  Arrays with configuration parameters for the AD9389B
*
**********************************************************************
*/

struct i2c_reg_value {
	u8 reg;
	u8 value;
};

struct ad9389b_state_edid {
	/* total number of blocks */
	u32 blocks;
	/* Number of segments read */
	u32 segments;
	u8 data[EDID_MAX_SEGM * 256];
	/* Number of EDID read retries left */
	unsigned read_retries;
};

struct ad9389b_state {
	struct ad9389b_platform_data pdata;
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_ctrl_handler hdl;
	int chip_revision;
	/* Is the ad9389b powered on? */
	bool power_on;
	/* Did we receive hotplug and rx-sense signals? */
	bool have_monitor;
	/* timings from s_dv_timings */
	struct v4l2_dv_timings dv_timings;
	/* controls */
	struct v4l2_ctrl *hdmi_mode_ctrl;
	struct v4l2_ctrl *hotplug_ctrl;
	struct v4l2_ctrl *rx_sense_ctrl;
	struct v4l2_ctrl *have_edid0_ctrl;
	struct v4l2_ctrl *rgb_quantization_range_ctrl;
	struct i2c_client *edid_i2c_client;
	struct ad9389b_state_edid edid;
	/* Running counter of the number of detected EDIDs (for debugging) */
	unsigned edid_detect_counter;
	struct workqueue_struct *work_queue;
	struct delayed_work edid_handler; /* work entry */
};

static void ad9389b_check_monitor_present_status(struct v4l2_subdev *sd);
static bool ad9389b_check_edid_status(struct v4l2_subdev *sd);
static void ad9389b_setup(struct v4l2_subdev *sd);
static int ad9389b_s_i2s_clock_freq(struct v4l2_subdev *sd, u32 freq);
static int ad9389b_s_clock_freq(struct v4l2_subdev *sd, u32 freq);

static inline struct ad9389b_state *get_ad9389b_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ad9389b_state, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct ad9389b_state, hdl)->sd;
}

/* ------------------------ I2C ----------------------------------------------- */

static int ad9389b_rd(struct v4l2_subdev *sd, u8 reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return i2c_smbus_read_byte_data(client, reg);
}

static int ad9389b_wr(struct v4l2_subdev *sd, u8 reg, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	int i;

	for (i = 0; i < 3; i++) {
		ret = i2c_smbus_write_byte_data(client, reg, val);
		if (ret == 0)
			return 0;
	}
	v4l2_err(sd, "I2C Write Problem\n");
	return ret;
}

/* To set specific bits in the register, a clear-mask is given (to be AND-ed),
   and then the value-mask (to be OR-ed). */
static inline void ad9389b_wr_and_or(struct v4l2_subdev *sd, u8 reg,
						u8 clr_mask, u8 val_mask)
{
	ad9389b_wr(sd, reg, (ad9389b_rd(sd, reg) & clr_mask) | val_mask);
}

static void ad9389b_edid_rd(struct v4l2_subdev *sd, u16 len, u8 *buf)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	int i;

	v4l2_dbg(1, debug, sd, "%s:\n", __func__);

	for (i = 0; i < len; i++)
		buf[i] = i2c_smbus_read_byte_data(state->edid_i2c_client, i);
}

static inline bool ad9389b_have_hotplug(struct v4l2_subdev *sd)
{
	return ad9389b_rd(sd, 0x42) & MASK_AD9389B_HPD_DETECT;
}

static inline bool ad9389b_have_rx_sense(struct v4l2_subdev *sd)
{
	return ad9389b_rd(sd, 0x42) & MASK_AD9389B_MSEN_DETECT;
}

static void ad9389b_csc_conversion_mode(struct v4l2_subdev *sd, u8 mode)
{
	ad9389b_wr_and_or(sd, 0x17, 0xe7, (mode & 0x3)<<3);
	ad9389b_wr_and_or(sd, 0x18, 0x9f, (mode & 0x3)<<5);
}

static void ad9389b_csc_coeff(struct v4l2_subdev *sd,
			      u16 A1, u16 A2, u16 A3, u16 A4,
			      u16 B1, u16 B2, u16 B3, u16 B4,
			      u16 C1, u16 C2, u16 C3, u16 C4)
{
	/* A */
	ad9389b_wr_and_or(sd, 0x18, 0xe0, A1>>8);
	ad9389b_wr(sd, 0x19, A1);
	ad9389b_wr_and_or(sd, 0x1A, 0xe0, A2>>8);
	ad9389b_wr(sd, 0x1B, A2);
	ad9389b_wr_and_or(sd, 0x1c, 0xe0, A3>>8);
	ad9389b_wr(sd, 0x1d, A3);
	ad9389b_wr_and_or(sd, 0x1e, 0xe0, A4>>8);
	ad9389b_wr(sd, 0x1f, A4);

	/* B */
	ad9389b_wr_and_or(sd, 0x20, 0xe0, B1>>8);
	ad9389b_wr(sd, 0x21, B1);
	ad9389b_wr_and_or(sd, 0x22, 0xe0, B2>>8);
	ad9389b_wr(sd, 0x23, B2);
	ad9389b_wr_and_or(sd, 0x24, 0xe0, B3>>8);
	ad9389b_wr(sd, 0x25, B3);
	ad9389b_wr_and_or(sd, 0x26, 0xe0, B4>>8);
	ad9389b_wr(sd, 0x27, B4);

	/* C */
	ad9389b_wr_and_or(sd, 0x28, 0xe0, C1>>8);
	ad9389b_wr(sd, 0x29, C1);
	ad9389b_wr_and_or(sd, 0x2A, 0xe0, C2>>8);
	ad9389b_wr(sd, 0x2B, C2);
	ad9389b_wr_and_or(sd, 0x2C, 0xe0, C3>>8);
	ad9389b_wr(sd, 0x2D, C3);
	ad9389b_wr_and_or(sd, 0x2E, 0xe0, C4>>8);
	ad9389b_wr(sd, 0x2F, C4);
}

static void ad9389b_csc_rgb_full2limit(struct v4l2_subdev *sd, bool enable)
{
	if (enable) {
		u8 csc_mode = 0;

		ad9389b_csc_conversion_mode(sd, csc_mode);
		ad9389b_csc_coeff(sd,
				  4096-564, 0, 0, 256,
				  0, 4096-564, 0, 256,
				  0, 0, 4096-564, 256);
		/* enable CSC */
		ad9389b_wr_and_or(sd, 0x3b, 0xfe, 0x1);
		/* AVI infoframe: Limited range RGB (16-235) */
		ad9389b_wr_and_or(sd, 0xcd, 0xf9, 0x02);
	} else {
		/* disable CSC */
		ad9389b_wr_and_or(sd, 0x3b, 0xfe, 0x0);
		/* AVI infoframe: Full range RGB (0-255) */
		ad9389b_wr_and_or(sd, 0xcd, 0xf9, 0x04);
	}
}

static void ad9389b_set_IT_content_AVI_InfoFrame(struct v4l2_subdev *sd)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);

	if (state->dv_timings.bt.standards & V4L2_DV_BT_STD_CEA861) {
		/* CEA format, not IT  */
		ad9389b_wr_and_or(sd, 0xcd, 0xbf, 0x00);
	} else {
		/* IT format */
		ad9389b_wr_and_or(sd, 0xcd, 0xbf, 0x40);
	}
}

static int ad9389b_set_rgb_quantization_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);

	switch (ctrl->val) {
	case V4L2_DV_RGB_RANGE_AUTO:
		/* automatic */
		if (state->dv_timings.bt.standards & V4L2_DV_BT_STD_CEA861) {
			/* cea format, RGB limited range (16-235) */
			ad9389b_csc_rgb_full2limit(sd, true);
		} else {
			/* not cea format, RGB full range (0-255) */
			ad9389b_csc_rgb_full2limit(sd, false);
		}
		break;
	case V4L2_DV_RGB_RANGE_LIMITED:
		/* RGB limited range (16-235) */
		ad9389b_csc_rgb_full2limit(sd, true);
		break;
	case V4L2_DV_RGB_RANGE_FULL:
		/* RGB full range (0-255) */
		ad9389b_csc_rgb_full2limit(sd, false);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void ad9389b_set_manual_pll_gear(struct v4l2_subdev *sd, u32 pixelclock)
{
	u8 gear;

	/* Workaround for TMDS PLL problem
	 * The TMDS PLL in AD9389b change gear when the chip is heated above a
	 * certain temperature. The output is disabled when the PLL change gear
	 * so the monitor has to lock on the signal again. A workaround for
	 * this is to use the manual PLL gears. This is a solution from Analog
	 * Devices that is not documented in the datasheets.
	 * 0x98 [7] = enable manual gearing. 0x98 [6:4] = gear
	 *
	 * The pixel frequency ranges are based on readout of the gear the
	 * automatic gearing selects for different pixel clocks
	 * (read from 0x9e [3:1]).
	 */

	if (pixelclock > 140000000)
		gear = 0xc0; /* 4th gear */
	else if (pixelclock > 117000000)
		gear = 0xb0; /* 3rd gear */
	else if (pixelclock > 87000000)
		gear = 0xa0; /* 2nd gear */
	else if (pixelclock > 60000000)
		gear = 0x90; /* 1st gear */
	else
		gear = 0x80; /* 0th gear */

	ad9389b_wr_and_or(sd, 0x98, 0x0f, gear);
}

/* ------------------------------ CTRL OPS ------------------------------ */

static int ad9389b_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct ad9389b_state *state = get_ad9389b_state(sd);

	v4l2_dbg(1, debug, sd,
		"%s: ctrl id: %d, ctrl->val %d\n", __func__, ctrl->id, ctrl->val);

	if (state->hdmi_mode_ctrl == ctrl) {
		/* Set HDMI or DVI-D */
		ad9389b_wr_and_or(sd, 0xaf, 0xfd,
				ctrl->val == V4L2_DV_TX_MODE_HDMI ? 0x02 : 0x00);
		return 0;
	}
	if (state->rgb_quantization_range_ctrl == ctrl)
		return ad9389b_set_rgb_quantization_mode(sd, ctrl);
	return -EINVAL;
}

static const struct v4l2_ctrl_ops ad9389b_ctrl_ops = {
	.s_ctrl = ad9389b_s_ctrl,
};

/* ---------------------------- CORE OPS ------------------------------------------- */

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ad9389b_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	reg->val = ad9389b_rd(sd, reg->reg & 0xff);
	reg->size = 1;
	return 0;
}

static int ad9389b_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	ad9389b_wr(sd, reg->reg & 0xff, reg->val & 0xff);
	return 0;
}
#endif

static int ad9389b_log_status(struct v4l2_subdev *sd)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	struct ad9389b_state_edid *edid = &state->edid;

	static const char * const states[] = {
		"in reset",
		"reading EDID",
		"idle",
		"initializing HDCP",
		"HDCP enabled",
		"initializing HDCP repeater",
		"6", "7", "8", "9", "A", "B", "C", "D", "E", "F"
	};
	static const char * const errors[] = {
		"no error",
		"bad receiver BKSV",
		"Ri mismatch",
		"Pj mismatch",
		"i2c error",
		"timed out",
		"max repeater cascade exceeded",
		"hash check failed",
		"too many devices",
		"9", "A", "B", "C", "D", "E", "F"
	};

	u8 manual_gear;

	v4l2_info(sd, "chip revision %d\n", state->chip_revision);
	v4l2_info(sd, "power %s\n", state->power_on ? "on" : "off");
	v4l2_info(sd, "%s hotplug, %s Rx Sense, %s EDID (%d block(s))\n",
			(ad9389b_rd(sd, 0x42) & MASK_AD9389B_HPD_DETECT) ?
							"detected" : "no",
			(ad9389b_rd(sd, 0x42) & MASK_AD9389B_MSEN_DETECT) ?
							"detected" : "no",
			edid->segments ? "found" : "no", edid->blocks);
	if (state->have_monitor) {
		v4l2_info(sd, "%s output %s\n",
				  (ad9389b_rd(sd, 0xaf) & 0x02) ?
				  "HDMI" : "DVI-D",
				  (ad9389b_rd(sd, 0xa1) & 0x3c) ?
				  "disabled" : "enabled");
	}
	v4l2_info(sd, "ad9389b: %s\n", (ad9389b_rd(sd, 0xb8) & 0x40) ?
					"encrypted" : "no encryption");
	v4l2_info(sd, "state: %s, error: %s, detect count: %u, msk/irq: %02x/%02x\n",
			states[ad9389b_rd(sd, 0xc8) & 0xf],
			errors[ad9389b_rd(sd, 0xc8) >> 4],
			state->edid_detect_counter,
			ad9389b_rd(sd, 0x94), ad9389b_rd(sd, 0x96));
	manual_gear = ad9389b_rd(sd, 0x98) & 0x80;
	v4l2_info(sd, "ad9389b: RGB quantization: %s range\n",
			ad9389b_rd(sd, 0x3b) & 0x01 ? "limited" : "full");
	v4l2_info(sd, "ad9389b: %s gear %d\n",
		  manual_gear ? "manual" : "automatic",
		  manual_gear ? ((ad9389b_rd(sd, 0x98) & 0x70) >> 4) :
				((ad9389b_rd(sd, 0x9e) & 0x0e) >> 1));
	if (state->have_monitor) {
		if (ad9389b_rd(sd, 0xaf) & 0x02) {
			/* HDMI only */
			u8 manual_cts = ad9389b_rd(sd, 0x0a) & 0x80;
			u32 N = (ad9389b_rd(sd, 0x01) & 0xf) << 16 |
				 ad9389b_rd(sd, 0x02) << 8 |
				 ad9389b_rd(sd, 0x03);
			u8 vic_detect = ad9389b_rd(sd, 0x3e) >> 2;
			u8 vic_sent = ad9389b_rd(sd, 0x3d) & 0x3f;
			u32 CTS;

			if (manual_cts)
				CTS = (ad9389b_rd(sd, 0x07) & 0xf) << 16 |
				       ad9389b_rd(sd, 0x08) << 8 |
				       ad9389b_rd(sd, 0x09);
			else
				CTS = (ad9389b_rd(sd, 0x04) & 0xf) << 16 |
				       ad9389b_rd(sd, 0x05) << 8 |
				       ad9389b_rd(sd, 0x06);
			N = (ad9389b_rd(sd, 0x01) & 0xf) << 16 |
			     ad9389b_rd(sd, 0x02) << 8 |
			     ad9389b_rd(sd, 0x03);

			v4l2_info(sd, "ad9389b: CTS %s mode: N %d, CTS %d\n",
				manual_cts ? "manual" : "automatic", N, CTS);

			v4l2_info(sd, "ad9389b: VIC: detected %d, sent %d\n",
				vic_detect, vic_sent);
		}
	}
	if (state->dv_timings.type == V4L2_DV_BT_656_1120) {
		struct v4l2_bt_timings *bt = bt = &state->dv_timings.bt;
		u32 frame_width = bt->width + bt->hfrontporch +
			bt->hsync + bt->hbackporch;
		u32 frame_height = bt->height + bt->vfrontporch +
			bt->vsync + bt->vbackporch;
		u32 frame_size = frame_width * frame_height;

		v4l2_info(sd, "timings: %ux%u%s%u (%ux%u). Pix freq. = %u Hz. Polarities = 0x%x\n",
			bt->width, bt->height, bt->interlaced ? "i" : "p",
			frame_size > 0 ?  (unsigned)bt->pixelclock / frame_size : 0,
			frame_width, frame_height,
			(unsigned)bt->pixelclock, bt->polarities);
	} else {
		v4l2_info(sd, "no timings set\n");
	}
	return 0;
}

/* Power up/down ad9389b */
static int ad9389b_s_power(struct v4l2_subdev *sd, int on)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	struct ad9389b_platform_data *pdata = &state->pdata;
	const int retries = 20;
	int i;

	v4l2_dbg(1, debug, sd, "%s: power %s\n", __func__, on ? "on" : "off");

	state->power_on = on;

	if (!on) {
		/* Power down */
		ad9389b_wr_and_or(sd, 0x41, 0xbf, 0x40);
		return true;
	}

	/* Power up */
	/* The ad9389b does not always come up immediately.
	   Retry multiple times. */
	for (i = 0; i < retries; i++) {
		ad9389b_wr_and_or(sd, 0x41, 0xbf, 0x0);
		if ((ad9389b_rd(sd, 0x41) & 0x40) == 0)
			break;
		ad9389b_wr_and_or(sd, 0x41, 0xbf, 0x40);
		msleep(10);
	}
	if (i == retries) {
		v4l2_dbg(1, debug, sd, "failed to powerup the ad9389b\n");
		ad9389b_s_power(sd, 0);
		return false;
	}
	if (i > 1)
		v4l2_dbg(1, debug, sd,
			"needed %d retries to powerup the ad9389b\n", i);

	/* Select chip: AD9389B */
	ad9389b_wr_and_or(sd, 0xba, 0xef, 0x10);

	/* Reserved registers that must be set according to REF_01 p. 11*/
	ad9389b_wr_and_or(sd, 0x98, 0xf0, 0x07);
	ad9389b_wr(sd, 0x9c, 0x38);
	ad9389b_wr_and_or(sd, 0x9d, 0xfc, 0x01);

	/* Differential output drive strength */
	if (pdata->diff_data_drive_strength > 0)
		ad9389b_wr(sd, 0xa2, pdata->diff_data_drive_strength);
	else
		ad9389b_wr(sd, 0xa2, 0x87);

	if (pdata->diff_clk_drive_strength > 0)
		ad9389b_wr(sd, 0xa3, pdata->diff_clk_drive_strength);
	else
		ad9389b_wr(sd, 0xa3, 0x87);

	ad9389b_wr(sd, 0x0a, 0x01);
	ad9389b_wr(sd, 0xbb, 0xff);

	/* Set number of attempts to read the EDID */
	ad9389b_wr(sd, 0xc9, 0xf);
	return true;
}

/* Enable interrupts */
static void ad9389b_set_isr(struct v4l2_subdev *sd, bool enable)
{
	u8 irqs = MASK_AD9389B_HPD_INT | MASK_AD9389B_MSEN_INT;
	u8 irqs_rd;
	int retries = 100;

	/* The datasheet says that the EDID ready interrupt should be
	   disabled if there is no hotplug. */
	if (!enable)
		irqs = 0;
	else if (ad9389b_have_hotplug(sd))
		irqs |= MASK_AD9389B_EDID_RDY_INT;

	/*
	 * This i2c write can fail (approx. 1 in 1000 writes). But it
	 * is essential that this register is correct, so retry it
	 * multiple times.
	 *
	 * Note that the i2c write does not report an error, but the readback
	 * clearly shows the wrong value.
	 */
	do {
		ad9389b_wr(sd, 0x94, irqs);
		irqs_rd = ad9389b_rd(sd, 0x94);
	} while (retries-- && irqs_rd != irqs);

	if (irqs_rd != irqs)
		v4l2_err(sd, "Could not set interrupts: hw failure?\n");
}

/* Interrupt handler */
static int ad9389b_isr(struct v4l2_subdev *sd, u32 status, bool *handled)
{
	u8 irq_status;

	/* disable interrupts to prevent a race condition */
	ad9389b_set_isr(sd, false);
	irq_status = ad9389b_rd(sd, 0x96);
	/* clear detected interrupts */
	ad9389b_wr(sd, 0x96, irq_status);

	if (irq_status & (MASK_AD9389B_HPD_INT | MASK_AD9389B_MSEN_INT))
		ad9389b_check_monitor_present_status(sd);
	if (irq_status & MASK_AD9389B_EDID_RDY_INT)
		ad9389b_check_edid_status(sd);

	/* enable interrupts */
	ad9389b_set_isr(sd, true);
	*handled = true;
	return 0;
}

static const struct v4l2_subdev_core_ops ad9389b_core_ops = {
	.log_status = ad9389b_log_status,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ad9389b_g_register,
	.s_register = ad9389b_s_register,
#endif
	.s_power = ad9389b_s_power,
	.interrupt_service_routine = ad9389b_isr,
};

/* ------------------------------ PAD OPS ------------------------------ */

static int ad9389b_get_edid(struct v4l2_subdev *sd, struct v4l2_subdev_edid *edid)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);

	if (edid->pad != 0)
		return -EINVAL;
	if (edid->blocks == 0 || edid->blocks > 256)
		return -EINVAL;
	if (!edid->edid)
		return -EINVAL;
	if (!state->edid.segments) {
		v4l2_dbg(1, debug, sd, "EDID segment 0 not found\n");
		return -ENODATA;
	}
	if (edid->start_block >= state->edid.segments * 2)
		return -E2BIG;
	if (edid->blocks + edid->start_block >= state->edid.segments * 2)
		edid->blocks = state->edid.segments * 2 - edid->start_block;
	memcpy(edid->edid, &state->edid.data[edid->start_block * 128],
				128 * edid->blocks);
	return 0;
}

static const struct v4l2_subdev_pad_ops ad9389b_pad_ops = {
	.get_edid = ad9389b_get_edid,
};

/* ------------------------------ VIDEO OPS ------------------------------ */

/* Enable/disable ad9389b output */
static int ad9389b_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);

	v4l2_dbg(1, debug, sd, "%s: %sable\n", __func__, (enable ? "en" : "dis"));

	ad9389b_wr_and_or(sd, 0xa1, ~0x3c, (enable ? 0 : 0x3c));
	if (enable) {
		ad9389b_check_monitor_present_status(sd);
	} else {
		ad9389b_s_power(sd, 0);
		state->have_monitor = false;
	}
	return 0;
}

static const struct v4l2_dv_timings ad9389b_timings[] = {
	V4L2_DV_BT_CEA_720X480P59_94,
	V4L2_DV_BT_CEA_720X576P50,
	V4L2_DV_BT_CEA_1280X720P24,
	V4L2_DV_BT_CEA_1280X720P25,
	V4L2_DV_BT_CEA_1280X720P30,
	V4L2_DV_BT_CEA_1280X720P50,
	V4L2_DV_BT_CEA_1280X720P60,
	V4L2_DV_BT_CEA_1920X1080P24,
	V4L2_DV_BT_CEA_1920X1080P25,
	V4L2_DV_BT_CEA_1920X1080P30,
	V4L2_DV_BT_CEA_1920X1080P50,
	V4L2_DV_BT_CEA_1920X1080P60,

	V4L2_DV_BT_DMT_640X350P85,
	V4L2_DV_BT_DMT_640X400P85,
	V4L2_DV_BT_DMT_720X400P85,
	V4L2_DV_BT_DMT_640X480P60,
	V4L2_DV_BT_DMT_640X480P72,
	V4L2_DV_BT_DMT_640X480P75,
	V4L2_DV_BT_DMT_640X480P85,
	V4L2_DV_BT_DMT_800X600P56,
	V4L2_DV_BT_DMT_800X600P60,
	V4L2_DV_BT_DMT_800X600P72,
	V4L2_DV_BT_DMT_800X600P75,
	V4L2_DV_BT_DMT_800X600P85,
	V4L2_DV_BT_DMT_848X480P60,
	V4L2_DV_BT_DMT_1024X768P60,
	V4L2_DV_BT_DMT_1024X768P70,
	V4L2_DV_BT_DMT_1024X768P75,
	V4L2_DV_BT_DMT_1024X768P85,
	V4L2_DV_BT_DMT_1152X864P75,
	V4L2_DV_BT_DMT_1280X768P60_RB,
	V4L2_DV_BT_DMT_1280X768P60,
	V4L2_DV_BT_DMT_1280X768P75,
	V4L2_DV_BT_DMT_1280X768P85,
	V4L2_DV_BT_DMT_1280X800P60_RB,
	V4L2_DV_BT_DMT_1280X800P60,
	V4L2_DV_BT_DMT_1280X800P75,
	V4L2_DV_BT_DMT_1280X800P85,
	V4L2_DV_BT_DMT_1280X960P60,
	V4L2_DV_BT_DMT_1280X960P85,
	V4L2_DV_BT_DMT_1280X1024P60,
	V4L2_DV_BT_DMT_1280X1024P75,
	V4L2_DV_BT_DMT_1280X1024P85,
	V4L2_DV_BT_DMT_1360X768P60,
	V4L2_DV_BT_DMT_1400X1050P60_RB,
	V4L2_DV_BT_DMT_1400X1050P60,
	V4L2_DV_BT_DMT_1400X1050P75,
	V4L2_DV_BT_DMT_1400X1050P85,
	V4L2_DV_BT_DMT_1440X900P60_RB,
	V4L2_DV_BT_DMT_1440X900P60,
	V4L2_DV_BT_DMT_1600X1200P60,
	V4L2_DV_BT_DMT_1680X1050P60_RB,
	V4L2_DV_BT_DMT_1680X1050P60,
	V4L2_DV_BT_DMT_1792X1344P60,
	V4L2_DV_BT_DMT_1856X1392P60,
	V4L2_DV_BT_DMT_1920X1200P60_RB,
	V4L2_DV_BT_DMT_1366X768P60,
	V4L2_DV_BT_DMT_1920X1080P60,
	{},
};

static int ad9389b_s_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_dv_timings *timings)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	int i;

	v4l2_dbg(1, debug, sd, "%s:\n", __func__);

	/* quick sanity check */
	if (timings->type != V4L2_DV_BT_656_1120)
		return -EINVAL;

	if (timings->bt.interlaced)
		return -EINVAL;
	if (timings->bt.pixelclock < 27000000 ||
	    timings->bt.pixelclock > 170000000)
		return -EINVAL;

	/* Fill the optional fields .standards and .flags in struct v4l2_dv_timings
	   if the format is listed in ad9389b_timings[] */
	for (i = 0; ad9389b_timings[i].bt.width; i++) {
		if (v4l_match_dv_timings(timings, &ad9389b_timings[i], 0)) {
			*timings = ad9389b_timings[i];
			break;
		}
	}

	timings->bt.flags &= ~V4L2_DV_FL_REDUCED_FPS;

	/* save timings */
	state->dv_timings = *timings;

	/* update quantization range based on new dv_timings */
	ad9389b_set_rgb_quantization_mode(sd, state->rgb_quantization_range_ctrl);

	/* update PLL gear based on new dv_timings */
	if (state->pdata.tmds_pll_gear == AD9389B_TMDS_PLL_GEAR_SEMI_AUTOMATIC)
		ad9389b_set_manual_pll_gear(sd, (u32)timings->bt.pixelclock);

	/* update AVI infoframe */
	ad9389b_set_IT_content_AVI_InfoFrame(sd);

	return 0;
}

static int ad9389b_g_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_dv_timings *timings)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);

	v4l2_dbg(1, debug, sd, "%s:\n", __func__);

	if (!timings)
		return -EINVAL;

	*timings = state->dv_timings;

	return 0;
}

static int ad9389b_enum_dv_timings(struct v4l2_subdev *sd,
			struct v4l2_enum_dv_timings *timings)
{
	if (timings->index >= ARRAY_SIZE(ad9389b_timings))
		return -EINVAL;

	memset(timings->reserved, 0, sizeof(timings->reserved));
	timings->timings = ad9389b_timings[timings->index];
	return 0;
}

static int ad9389b_dv_timings_cap(struct v4l2_subdev *sd,
			struct v4l2_dv_timings_cap *cap)
{
	cap->type = V4L2_DV_BT_656_1120;
	cap->bt.max_width = 1920;
	cap->bt.max_height = 1200;
	cap->bt.min_pixelclock = 27000000;
	cap->bt.max_pixelclock = 170000000;
	cap->bt.standards = V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
			 V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT;
	cap->bt.capabilities = V4L2_DV_BT_CAP_PROGRESSIVE |
		V4L2_DV_BT_CAP_REDUCED_BLANKING | V4L2_DV_BT_CAP_CUSTOM;
	return 0;
}

static const struct v4l2_subdev_video_ops ad9389b_video_ops = {
	.s_stream = ad9389b_s_stream,
	.s_dv_timings = ad9389b_s_dv_timings,
	.g_dv_timings = ad9389b_g_dv_timings,
	.enum_dv_timings = ad9389b_enum_dv_timings,
	.dv_timings_cap = ad9389b_dv_timings_cap,
};

static int ad9389b_s_audio_stream(struct v4l2_subdev *sd, int enable)
{
	v4l2_dbg(1, debug, sd, "%s: %sable\n", __func__, (enable ? "en" : "dis"));

	if (enable)
		ad9389b_wr_and_or(sd, 0x45, 0x3f, 0x80);
	else
		ad9389b_wr_and_or(sd, 0x45, 0x3f, 0x40);

	return 0;
}

static int ad9389b_s_clock_freq(struct v4l2_subdev *sd, u32 freq)
{
	u32 N;

	switch (freq) {
	case 32000: N = 4096; break;
	case 44100: N = 6272; break;
	case 48000: N = 6144; break;
	case 88200: N = 12544; break;
	case 96000: N = 12288; break;
	case 176400: N = 25088; break;
	case 192000: N = 24576; break;
	default:
		return -EINVAL;
	}

	/* Set N (used with CTS to regenerate the audio clock) */
	ad9389b_wr(sd, 0x01, (N >> 16) & 0xf);
	ad9389b_wr(sd, 0x02, (N >> 8) & 0xff);
	ad9389b_wr(sd, 0x03, N & 0xff);

	return 0;
}

static int ad9389b_s_i2s_clock_freq(struct v4l2_subdev *sd, u32 freq)
{
	u32 i2s_sf;

	switch (freq) {
	case 32000: i2s_sf = 0x30; break;
	case 44100: i2s_sf = 0x00; break;
	case 48000: i2s_sf = 0x20; break;
	case 88200: i2s_sf = 0x80; break;
	case 96000: i2s_sf = 0xa0; break;
	case 176400: i2s_sf = 0xc0; break;
	case 192000: i2s_sf = 0xe0; break;
	default:
		return -EINVAL;
	}

	/* Set sampling frequency for I2S audio to 48 kHz */
	ad9389b_wr_and_or(sd, 0x15, 0xf, i2s_sf);

	return 0;
}

static int ad9389b_s_routing(struct v4l2_subdev *sd, u32 input, u32 output, u32 config)
{
	/* TODO based on input/output/config */
	/* TODO See datasheet "Programmers guide" p. 39-40 */

	/* Only 2 channels in use for application */
	ad9389b_wr_and_or(sd, 0x50, 0x1f, 0x20);
	/* Speaker mapping */
	ad9389b_wr(sd, 0x51, 0x00);

	/* TODO Where should this be placed? */
	/* 16 bit audio word length */
	ad9389b_wr_and_or(sd, 0x14, 0xf0, 0x02);

	return 0;
}

static const struct v4l2_subdev_audio_ops ad9389b_audio_ops = {
	.s_stream = ad9389b_s_audio_stream,
	.s_clock_freq = ad9389b_s_clock_freq,
	.s_i2s_clock_freq = ad9389b_s_i2s_clock_freq,
	.s_routing = ad9389b_s_routing,
};

/* --------------------- SUBDEV OPS --------------------------------------- */

static const struct v4l2_subdev_ops ad9389b_ops = {
	.core  = &ad9389b_core_ops,
	.video = &ad9389b_video_ops,
	.audio = &ad9389b_audio_ops,
	.pad = &ad9389b_pad_ops,
};

/* ----------------------------------------------------------------------- */
static void ad9389b_dbg_dump_edid(int lvl, int debug, struct v4l2_subdev *sd,
							int segment, u8 *buf)
{
	int i, j;

	if (debug < lvl)
		return;

	v4l2_dbg(lvl, debug, sd, "edid segment %d\n", segment);
	for (i = 0; i < 256; i += 16) {
		u8 b[128];
		u8 *bp = b;

		if (i == 128)
			v4l2_dbg(lvl, debug, sd, "\n");
		for (j = i; j < i + 16; j++) {
			sprintf(bp, "0x%02x, ", buf[j]);
			bp += 6;
		}
		bp[0] = '\0';
		v4l2_dbg(lvl, debug, sd, "%s\n", b);
	}
}

static void ad9389b_edid_handler(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct ad9389b_state *state = container_of(dwork,
			struct ad9389b_state, edid_handler);
	struct v4l2_subdev *sd = &state->sd;
	struct ad9389b_edid_detect ed;

	v4l2_dbg(1, debug, sd, "%s:\n", __func__);

	if (ad9389b_check_edid_status(sd)) {
		/* Return if we received the EDID. */
		return;
	}

	if (ad9389b_have_hotplug(sd)) {
		/* We must retry reading the EDID several times, it is possible
		 * that initially the EDID couldn't be read due to i2c errors
		 * (DVI connectors are particularly prone to this problem). */
		if (state->edid.read_retries) {
			state->edid.read_retries--;
			/* EDID read failed, trigger a retry */
			ad9389b_wr(sd, 0xc9, 0xf);
			queue_delayed_work(state->work_queue,
					&state->edid_handler, EDID_DELAY);
			return;
		}
	}

	/* We failed to read the EDID, so send an event for this. */
	ed.present = false;
	ed.segment = ad9389b_rd(sd, 0xc4);
	v4l2_subdev_notify(sd, AD9389B_EDID_DETECT, (void *)&ed);
	v4l2_dbg(1, debug, sd, "%s: no edid found\n", __func__);
}

static void ad9389b_audio_setup(struct v4l2_subdev *sd)
{
	v4l2_dbg(1, debug, sd, "%s\n", __func__);

	ad9389b_s_i2s_clock_freq(sd, 48000);
	ad9389b_s_clock_freq(sd, 48000);
	ad9389b_s_routing(sd, 0, 0, 0);
}

/* Initial setup of AD9389b */

/* Configure hdmi transmitter. */
static void ad9389b_setup(struct v4l2_subdev *sd)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);

	v4l2_dbg(1, debug, sd, "%s\n", __func__);

	/* Input format: RGB 4:4:4 */
	ad9389b_wr_and_or(sd, 0x15, 0xf1, 0x0);
	/* Output format: RGB 4:4:4 */
	ad9389b_wr_and_or(sd, 0x16, 0x3f, 0x0);
	/* CSC fixed point: +/-2, 1st order interpolation 4:2:2 -> 4:4:4 up
	   conversion, Aspect ratio: 16:9 */
	ad9389b_wr_and_or(sd, 0x17, 0xe1, 0x0e);
	/* Disable pixel repetition and CSC */
	ad9389b_wr_and_or(sd, 0x3b, 0x9e, 0x0);
	/* Output format: RGB 4:4:4, Active Format Information is valid. */
	ad9389b_wr_and_or(sd, 0x45, 0xc7, 0x08);
	/* Underscanned */
	ad9389b_wr_and_or(sd, 0x46, 0x3f, 0x80);
	/* Setup video format */
	ad9389b_wr(sd, 0x3c, 0x0);
	/* Active format aspect ratio: same as picure. */
	ad9389b_wr(sd, 0x47, 0x80);
	/* No encryption */
	ad9389b_wr_and_or(sd, 0xaf, 0xef, 0x0);
	/* Positive clk edge capture for input video clock */
	ad9389b_wr_and_or(sd, 0xba, 0x1f, 0x60);

	ad9389b_audio_setup(sd);

	v4l2_ctrl_handler_setup(&state->hdl);

	ad9389b_set_IT_content_AVI_InfoFrame(sd);
}

static void ad9389b_notify_monitor_detect(struct v4l2_subdev *sd)
{
	struct ad9389b_monitor_detect mdt;
	struct ad9389b_state *state = get_ad9389b_state(sd);

	mdt.present = state->have_monitor;
	v4l2_subdev_notify(sd, AD9389B_MONITOR_DETECT, (void *)&mdt);
}

static void ad9389b_check_monitor_present_status(struct v4l2_subdev *sd)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	/* read hotplug and rx-sense state */
	u8 status = ad9389b_rd(sd, 0x42);

	v4l2_dbg(1, debug, sd, "%s: status: 0x%x%s%s\n",
			 __func__,
			 status,
			 status & MASK_AD9389B_HPD_DETECT ? ", hotplug" : "",
			 status & MASK_AD9389B_MSEN_DETECT ? ", rx-sense" : "");

	if ((status & MASK_AD9389B_HPD_DETECT) &&
	    ((status & MASK_AD9389B_MSEN_DETECT) || state->edid.segments)) {
		v4l2_dbg(1, debug, sd,
				"%s: hotplug and (rx-sense or edid)\n", __func__);
		if (!state->have_monitor) {
			v4l2_dbg(1, debug, sd, "%s: monitor detected\n", __func__);
			state->have_monitor = true;
			ad9389b_set_isr(sd, true);
			if (!ad9389b_s_power(sd, true)) {
				v4l2_dbg(1, debug, sd,
					"%s: monitor detected, powerup failed\n", __func__);
				return;
			}
			ad9389b_setup(sd);
			ad9389b_notify_monitor_detect(sd);
			state->edid.read_retries = EDID_MAX_RETRIES;
			queue_delayed_work(state->work_queue,
					&state->edid_handler, EDID_DELAY);
		}
	} else if (status & MASK_AD9389B_HPD_DETECT) {
		v4l2_dbg(1, debug, sd, "%s: hotplug detected\n", __func__);
		state->edid.read_retries = EDID_MAX_RETRIES;
		queue_delayed_work(state->work_queue,
				&state->edid_handler, EDID_DELAY);
	} else if (!(status & MASK_AD9389B_HPD_DETECT)) {
		v4l2_dbg(1, debug, sd, "%s: hotplug not detected\n", __func__);
		if (state->have_monitor) {
			v4l2_dbg(1, debug, sd, "%s: monitor not detected\n", __func__);
			state->have_monitor = false;
			ad9389b_notify_monitor_detect(sd);
		}
		ad9389b_s_power(sd, false);
		memset(&state->edid, 0, sizeof(struct ad9389b_state_edid));
	}

	/* update read only ctrls */
	v4l2_ctrl_s_ctrl(state->hotplug_ctrl, ad9389b_have_hotplug(sd) ? 0x1 : 0x0);
	v4l2_ctrl_s_ctrl(state->rx_sense_ctrl, ad9389b_have_rx_sense(sd) ? 0x1 : 0x0);
	v4l2_ctrl_s_ctrl(state->have_edid0_ctrl, state->edid.segments ? 0x1 : 0x0);
}

static bool edid_block_verify_crc(u8 *edid_block)
{
	int i;
	u8 sum = 0;

	for (i = 0; i < 127; i++)
		sum += *(edid_block + i);
	return ((255 - sum + 1) == edid_block[127]);
}

static bool edid_segment_verify_crc(struct v4l2_subdev *sd, u32 segment)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	u32 blocks = state->edid.blocks;
	u8 *data = state->edid.data;

	if (edid_block_verify_crc(&data[segment * 256])) {
		if ((segment + 1) * 2 <= blocks)
			return edid_block_verify_crc(&data[segment * 256 + 128]);
		return true;
	}
	return false;
}

static bool ad9389b_check_edid_status(struct v4l2_subdev *sd)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	struct ad9389b_edid_detect ed;
	int segment;
	u8 edidRdy = ad9389b_rd(sd, 0xc5);

	v4l2_dbg(1, debug, sd, "%s: edid ready (retries: %d)\n",
			 __func__, EDID_MAX_RETRIES - state->edid.read_retries);

	if (!(edidRdy & MASK_AD9389B_EDID_RDY))
		return false;

	segment = ad9389b_rd(sd, 0xc4);
	if (segment >= EDID_MAX_SEGM) {
		v4l2_err(sd, "edid segment number too big\n");
		return false;
	}
	v4l2_dbg(1, debug, sd, "%s: got segment %d\n", __func__, segment);
	ad9389b_edid_rd(sd, 256, &state->edid.data[segment * 256]);
	ad9389b_dbg_dump_edid(2, debug, sd, segment,
			&state->edid.data[segment * 256]);
	if (segment == 0) {
		state->edid.blocks = state->edid.data[0x7e] + 1;
		v4l2_dbg(1, debug, sd, "%s: %d blocks in total\n",
				__func__, state->edid.blocks);
	}
	if (!edid_segment_verify_crc(sd, segment)) {
		/* edid crc error, force reread of edid segment */
		ad9389b_s_power(sd, false);
		ad9389b_s_power(sd, true);
		return false;
	}
	/* one more segment read ok */
	state->edid.segments = segment + 1;
	if (((state->edid.data[0x7e] >> 1) + 1) > state->edid.segments) {
		/* Request next EDID segment */
		v4l2_dbg(1, debug, sd, "%s: request segment %d\n",
				__func__, state->edid.segments);
		ad9389b_wr(sd, 0xc9, 0xf);
		ad9389b_wr(sd, 0xc4, state->edid.segments);
		state->edid.read_retries = EDID_MAX_RETRIES;
		queue_delayed_work(state->work_queue,
				&state->edid_handler, EDID_DELAY);
		return false;
	}

	/* report when we have all segments but report only for segment 0 */
	ed.present = true;
	ed.segment = 0;
	v4l2_subdev_notify(sd, AD9389B_EDID_DETECT, (void *)&ed);
	state->edid_detect_counter++;
	v4l2_ctrl_s_ctrl(state->have_edid0_ctrl, state->edid.segments ? 0x1 : 0x0);
	return ed.present;
}

/* ----------------------------------------------------------------------- */

static void ad9389b_init_setup(struct v4l2_subdev *sd)
{
	struct ad9389b_state *state = get_ad9389b_state(sd);
	struct ad9389b_state_edid *edid = &state->edid;

	v4l2_dbg(1, debug, sd, "%s\n", __func__);

	/* clear all interrupts */
	ad9389b_wr(sd, 0x96, 0xff);

	memset(edid, 0, sizeof(struct ad9389b_state_edid));
	state->have_monitor = false;
	ad9389b_set_isr(sd, false);
}

static int ad9389b_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	const struct v4l2_dv_timings dv1080p60 = V4L2_DV_BT_CEA_1920X1080P60;
	struct ad9389b_state *state;
	struct ad9389b_platform_data *pdata = client->dev.platform_data;
	struct v4l2_ctrl_handler *hdl;
	struct v4l2_subdev *sd;
	int err = -EIO;

	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	v4l_dbg(1, debug, client, "detecting ad9389b client on address 0x%x\n",
			client->addr << 1);

	state = devm_kzalloc(&client->dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	/* Platform data */
	if (pdata == NULL) {
		v4l_err(client, "No platform data!\n");
		return -ENODEV;
	}
	memcpy(&state->pdata, pdata, sizeof(state->pdata));

	sd = &state->sd;
	v4l2_i2c_subdev_init(sd, client, &ad9389b_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	hdl = &state->hdl;
	v4l2_ctrl_handler_init(hdl, 5);

	/* private controls */

	state->hdmi_mode_ctrl = v4l2_ctrl_new_std_menu(hdl, &ad9389b_ctrl_ops,
			V4L2_CID_DV_TX_MODE, V4L2_DV_TX_MODE_HDMI,
			0, V4L2_DV_TX_MODE_DVI_D);
	state->hdmi_mode_ctrl->is_private = true;
	state->hotplug_ctrl = v4l2_ctrl_new_std(hdl, NULL,
			V4L2_CID_DV_TX_HOTPLUG, 0, 1, 0, 0);
	state->hotplug_ctrl->is_private = true;
	state->rx_sense_ctrl = v4l2_ctrl_new_std(hdl, NULL,
			V4L2_CID_DV_TX_RXSENSE, 0, 1, 0, 0);
	state->rx_sense_ctrl->is_private = true;
	state->have_edid0_ctrl = v4l2_ctrl_new_std(hdl, NULL,
			V4L2_CID_DV_TX_EDID_PRESENT, 0, 1, 0, 0);
	state->have_edid0_ctrl->is_private = true;
	state->rgb_quantization_range_ctrl =
		v4l2_ctrl_new_std_menu(hdl, &ad9389b_ctrl_ops,
			V4L2_CID_DV_TX_RGB_RANGE, V4L2_DV_RGB_RANGE_FULL,
			0, V4L2_DV_RGB_RANGE_AUTO);
	state->rgb_quantization_range_ctrl->is_private = true;
	sd->ctrl_handler = hdl;
	if (hdl->error) {
		err = hdl->error;

		goto err_hdl;
	}

	state->pad.flags = MEDIA_PAD_FL_SINK;
	err = media_entity_init(&sd->entity, 1, &state->pad, 0);
	if (err)
		goto err_hdl;

	state->chip_revision = ad9389b_rd(sd, 0x0);
	if (state->chip_revision != 2) {
		v4l2_err(sd, "chip_revision %d != 2\n", state->chip_revision);
		err = -EIO;
		goto err_entity;
	}
	v4l2_dbg(1, debug, sd, "reg 0x41 0x%x, chip version (reg 0x00) 0x%x\n",
			ad9389b_rd(sd, 0x41), state->chip_revision);

	state->edid_i2c_client = i2c_new_dummy(client->adapter, (0x7e>>1));
	if (state->edid_i2c_client == NULL) {
		v4l2_err(sd, "failed to register edid i2c client\n");
		err = -ENOMEM;
		goto err_entity;
	}

	state->work_queue = create_singlethread_workqueue(sd->name);
	if (state->work_queue == NULL) {
		v4l2_err(sd, "could not create workqueue\n");
		err = -ENOMEM;
		goto err_unreg;
	}

	INIT_DELAYED_WORK(&state->edid_handler, ad9389b_edid_handler);
	state->dv_timings = dv1080p60;

	ad9389b_init_setup(sd);
	ad9389b_set_isr(sd, true);

	v4l2_info(sd, "%s found @ 0x%x (%s)\n", client->name,
			  client->addr << 1, client->adapter->name);
	return 0;

err_unreg:
	i2c_unregister_device(state->edid_i2c_client);
err_entity:
	media_entity_cleanup(&sd->entity);
err_hdl:
	v4l2_ctrl_handler_free(&state->hdl);
	return err;
}

/* ----------------------------------------------------------------------- */

static int ad9389b_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ad9389b_state *state = get_ad9389b_state(sd);

	state->chip_revision = -1;

	v4l2_dbg(1, debug, sd, "%s removed @ 0x%x (%s)\n", client->name,
		 client->addr << 1, client->adapter->name);

	ad9389b_s_stream(sd, false);
	ad9389b_s_audio_stream(sd, false);
	ad9389b_init_setup(sd);
	cancel_delayed_work(&state->edid_handler);
	i2c_unregister_device(state->edid_i2c_client);
	destroy_workqueue(state->work_queue);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	return 0;
}

/* ----------------------------------------------------------------------- */

static struct i2c_device_id ad9389b_id[] = {
	{ "ad9389b", 0 },
	{ "ad9889b", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ad9389b_id);

static struct i2c_driver ad9389b_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ad9389b",
	},
	.probe = ad9389b_probe,
	.remove = ad9389b_remove,
	.id_table = ad9389b_id,
};

module_i2c_driver(ad9389b_driver);
