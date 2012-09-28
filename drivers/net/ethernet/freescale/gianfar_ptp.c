/*
 * PTP 1588 clock using the eTSEC
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/timex.h>
#include <linux/io.h>

#include <linux/ptp_clock_kernel.h>

#include "gianfar.h"

/*
 * gianfar ptp registers
 * Generated by regen.tcl on Thu May 13 01:38:57 PM CEST 2010
 */
struct gianfar_ptp_registers {
	u32 tmr_ctrl;     /* Timer control register */
	u32 tmr_tevent;   /* Timestamp event register */
	u32 tmr_temask;   /* Timer event mask register */
	u32 tmr_pevent;   /* Timestamp event register */
	u32 tmr_pemask;   /* Timer event mask register */
	u32 tmr_stat;     /* Timestamp status register */
	u32 tmr_cnt_h;    /* Timer counter high register */
	u32 tmr_cnt_l;    /* Timer counter low register */
	u32 tmr_add;      /* Timer drift compensation addend register */
	u32 tmr_acc;      /* Timer accumulator register */
	u32 tmr_prsc;     /* Timer prescale */
	u8  res1[4];
	u32 tmroff_h;     /* Timer offset high */
	u32 tmroff_l;     /* Timer offset low */
	u8  res2[8];
	u32 tmr_alarm1_h; /* Timer alarm 1 high register */
	u32 tmr_alarm1_l; /* Timer alarm 1 high register */
	u32 tmr_alarm2_h; /* Timer alarm 2 high register */
	u32 tmr_alarm2_l; /* Timer alarm 2 high register */
	u8  res3[48];
	u32 tmr_fiper1;   /* Timer fixed period interval */
	u32 tmr_fiper2;   /* Timer fixed period interval */
	u32 tmr_fiper3;   /* Timer fixed period interval */
	u8  res4[20];
	u32 tmr_etts1_h;  /* Timestamp of general purpose external trigger */
	u32 tmr_etts1_l;  /* Timestamp of general purpose external trigger */
	u32 tmr_etts2_h;  /* Timestamp of general purpose external trigger */
	u32 tmr_etts2_l;  /* Timestamp of general purpose external trigger */
};

/* Bit definitions for the TMR_CTRL register */
#define ALM1P                 (1<<31) /* Alarm1 output polarity */
#define ALM2P                 (1<<30) /* Alarm2 output polarity */
#define FS                    (1<<28) /* FIPER start indication */
#define PP1L                  (1<<27) /* Fiper1 pulse loopback mode enabled. */
#define PP2L                  (1<<26) /* Fiper2 pulse loopback mode enabled. */
#define TCLK_PERIOD_SHIFT     (16) /* 1588 timer reference clock period. */
#define TCLK_PERIOD_MASK      (0x3ff)
#define RTPE                  (1<<15) /* Record Tx Timestamp to PAL Enable. */
#define FRD                   (1<<14) /* FIPER Realignment Disable */
#define ESFDP                 (1<<11) /* External Tx/Rx SFD Polarity. */
#define ESFDE                 (1<<10) /* External Tx/Rx SFD Enable. */
#define ETEP2                 (1<<9) /* External trigger 2 edge polarity */
#define ETEP1                 (1<<8) /* External trigger 1 edge polarity */
#define COPH                  (1<<7) /* Generated clock output phase. */
#define CIPH                  (1<<6) /* External oscillator input clock phase */
#define TMSR                  (1<<5) /* Timer soft reset. */
#define BYP                   (1<<3) /* Bypass drift compensated clock */
#define TE                    (1<<2) /* 1588 timer enable. */
#define CKSEL_SHIFT           (0)    /* 1588 Timer reference clock source */
#define CKSEL_MASK            (0x3)

/* Bit definitions for the TMR_TEVENT register */
#define ETS2                  (1<<25) /* External trigger 2 timestamp sampled */
#define ETS1                  (1<<24) /* External trigger 1 timestamp sampled */
#define ALM2                  (1<<17) /* Current time = alarm time register 2 */
#define ALM1                  (1<<16) /* Current time = alarm time register 1 */
#define PP1                   (1<<7)  /* periodic pulse generated on FIPER1 */
#define PP2                   (1<<6)  /* periodic pulse generated on FIPER2 */
#define PP3                   (1<<5)  /* periodic pulse generated on FIPER3 */

/* Bit definitions for the TMR_TEMASK register */
#define ETS2EN                (1<<25) /* External trigger 2 timestamp enable */
#define ETS1EN                (1<<24) /* External trigger 1 timestamp enable */
#define ALM2EN                (1<<17) /* Timer ALM2 event enable */
#define ALM1EN                (1<<16) /* Timer ALM1 event enable */
#define PP1EN                 (1<<7) /* Periodic pulse event 1 enable */
#define PP2EN                 (1<<6) /* Periodic pulse event 2 enable */

/* Bit definitions for the TMR_PEVENT register */
#define TXP2                  (1<<9) /* PTP transmitted timestamp im TXTS2 */
#define TXP1                  (1<<8) /* PTP transmitted timestamp in TXTS1 */
#define RXP                   (1<<0) /* PTP frame has been received */

/* Bit definitions for the TMR_PEMASK register */
#define TXP2EN                (1<<9) /* Transmit PTP packet event 2 enable */
#define TXP1EN                (1<<8) /* Transmit PTP packet event 1 enable */
#define RXPEN                 (1<<0) /* Receive PTP packet event enable */

/* Bit definitions for the TMR_STAT register */
#define STAT_VEC_SHIFT        (0) /* Timer general purpose status vector */
#define STAT_VEC_MASK         (0x3f)

/* Bit definitions for the TMR_PRSC register */
#define PRSC_OCK_SHIFT        (0) /* Output clock division/prescale factor. */
#define PRSC_OCK_MASK         (0xffff)


#define DRIVER		"gianfar_ptp"
#define DEFAULT_CKSEL	1
#define N_ALARM		1 /* first alarm is used internally to reset fipers */
#define N_EXT_TS	2
#define REG_SIZE	sizeof(struct gianfar_ptp_registers)

struct etsects {
	struct gianfar_ptp_registers *regs;
	spinlock_t lock; /* protects regs */
	struct ptp_clock *clock;
	struct ptp_clock_info caps;
	struct resource *rsrc;
	int irq;
	u64 alarm_interval; /* for periodic alarm */
	u64 alarm_value;
	u32 tclk_period;  /* nanoseconds */
	u32 tmr_prsc;
	u32 tmr_add;
	u32 cksel;
	u32 tmr_fiper1;
	u32 tmr_fiper2;
};

/*
 * Register access functions
 */

/* Caller must hold etsects->lock. */
static u64 tmr_cnt_read(struct etsects *etsects)
{
	u64 ns;
	u32 lo, hi;

	lo = gfar_read(&etsects->regs->tmr_cnt_l);
	hi = gfar_read(&etsects->regs->tmr_cnt_h);
	ns = ((u64) hi) << 32;
	ns |= lo;
	return ns;
}

/* Caller must hold etsects->lock. */
static void tmr_cnt_write(struct etsects *etsects, u64 ns)
{
	u32 hi = ns >> 32;
	u32 lo = ns & 0xffffffff;

	gfar_write(&etsects->regs->tmr_cnt_l, lo);
	gfar_write(&etsects->regs->tmr_cnt_h, hi);
}

/* Caller must hold etsects->lock. */
static void set_alarm(struct etsects *etsects)
{
	u64 ns;
	u32 lo, hi;

	ns = tmr_cnt_read(etsects) + 1500000000ULL;
	ns = div_u64(ns, 1000000000UL) * 1000000000ULL;
	ns -= etsects->tclk_period;
	hi = ns >> 32;
	lo = ns & 0xffffffff;
	gfar_write(&etsects->regs->tmr_alarm1_l, lo);
	gfar_write(&etsects->regs->tmr_alarm1_h, hi);
}

/* Caller must hold etsects->lock. */
static void set_fipers(struct etsects *etsects)
{
	set_alarm(etsects);
	gfar_write(&etsects->regs->tmr_fiper1, etsects->tmr_fiper1);
	gfar_write(&etsects->regs->tmr_fiper2, etsects->tmr_fiper2);
}

/*
 * Interrupt service routine
 */

static irqreturn_t isr(int irq, void *priv)
{
	struct etsects *etsects = priv;
	struct ptp_clock_event event;
	u64 ns;
	u32 ack = 0, lo, hi, mask, val;

	val = gfar_read(&etsects->regs->tmr_tevent);

	if (val & ETS1) {
		ack |= ETS1;
		hi = gfar_read(&etsects->regs->tmr_etts1_h);
		lo = gfar_read(&etsects->regs->tmr_etts1_l);
		event.type = PTP_CLOCK_EXTTS;
		event.index = 0;
		event.timestamp = ((u64) hi) << 32;
		event.timestamp |= lo;
		ptp_clock_event(etsects->clock, &event);
	}

	if (val & ETS2) {
		ack |= ETS2;
		hi = gfar_read(&etsects->regs->tmr_etts2_h);
		lo = gfar_read(&etsects->regs->tmr_etts2_l);
		event.type = PTP_CLOCK_EXTTS;
		event.index = 1;
		event.timestamp = ((u64) hi) << 32;
		event.timestamp |= lo;
		ptp_clock_event(etsects->clock, &event);
	}

	if (val & ALM2) {
		ack |= ALM2;
		if (etsects->alarm_value) {
			event.type = PTP_CLOCK_ALARM;
			event.index = 0;
			event.timestamp = etsects->alarm_value;
			ptp_clock_event(etsects->clock, &event);
		}
		if (etsects->alarm_interval) {
			ns = etsects->alarm_value + etsects->alarm_interval;
			hi = ns >> 32;
			lo = ns & 0xffffffff;
			spin_lock(&etsects->lock);
			gfar_write(&etsects->regs->tmr_alarm2_l, lo);
			gfar_write(&etsects->regs->tmr_alarm2_h, hi);
			spin_unlock(&etsects->lock);
			etsects->alarm_value = ns;
		} else {
			gfar_write(&etsects->regs->tmr_tevent, ALM2);
			spin_lock(&etsects->lock);
			mask = gfar_read(&etsects->regs->tmr_temask);
			mask &= ~ALM2EN;
			gfar_write(&etsects->regs->tmr_temask, mask);
			spin_unlock(&etsects->lock);
			etsects->alarm_value = 0;
			etsects->alarm_interval = 0;
		}
	}

	if (val & PP1) {
		ack |= PP1;
		event.type = PTP_CLOCK_PPS;
		ptp_clock_event(etsects->clock, &event);
	}

	if (ack) {
		gfar_write(&etsects->regs->tmr_tevent, ack);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

/*
 * PTP clock operations
 */

static int ptp_gianfar_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	u64 adj;
	u32 diff, tmr_add;
	int neg_adj = 0;
	struct etsects *etsects = container_of(ptp, struct etsects, caps);

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}
	tmr_add = etsects->tmr_add;
	adj = tmr_add;
	adj *= ppb;
	diff = div_u64(adj, 1000000000ULL);

	tmr_add = neg_adj ? tmr_add - diff : tmr_add + diff;

	gfar_write(&etsects->regs->tmr_add, tmr_add);

	return 0;
}

static int ptp_gianfar_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	s64 now;
	unsigned long flags;
	struct etsects *etsects = container_of(ptp, struct etsects, caps);

	spin_lock_irqsave(&etsects->lock, flags);

	now = tmr_cnt_read(etsects);
	now += delta;
	tmr_cnt_write(etsects, now);

	spin_unlock_irqrestore(&etsects->lock, flags);

	set_fipers(etsects);

	return 0;
}

static int ptp_gianfar_gettime(struct ptp_clock_info *ptp, struct timespec *ts)
{
	u64 ns;
	u32 remainder;
	unsigned long flags;
	struct etsects *etsects = container_of(ptp, struct etsects, caps);

	spin_lock_irqsave(&etsects->lock, flags);

	ns = tmr_cnt_read(etsects);

	spin_unlock_irqrestore(&etsects->lock, flags);

	ts->tv_sec = div_u64_rem(ns, 1000000000, &remainder);
	ts->tv_nsec = remainder;
	return 0;
}

static int ptp_gianfar_settime(struct ptp_clock_info *ptp,
			       const struct timespec *ts)
{
	u64 ns;
	unsigned long flags;
	struct etsects *etsects = container_of(ptp, struct etsects, caps);

	ns = ts->tv_sec * 1000000000ULL;
	ns += ts->tv_nsec;

	spin_lock_irqsave(&etsects->lock, flags);

	tmr_cnt_write(etsects, ns);
	set_fipers(etsects);

	spin_unlock_irqrestore(&etsects->lock, flags);

	return 0;
}

static int ptp_gianfar_enable(struct ptp_clock_info *ptp,
			      struct ptp_clock_request *rq, int on)
{
	struct etsects *etsects = container_of(ptp, struct etsects, caps);
	unsigned long flags;
	u32 bit, mask;

	switch (rq->type) {
	case PTP_CLK_REQ_EXTTS:
		switch (rq->extts.index) {
		case 0:
			bit = ETS1EN;
			break;
		case 1:
			bit = ETS2EN;
			break;
		default:
			return -EINVAL;
		}
		spin_lock_irqsave(&etsects->lock, flags);
		mask = gfar_read(&etsects->regs->tmr_temask);
		if (on)
			mask |= bit;
		else
			mask &= ~bit;
		gfar_write(&etsects->regs->tmr_temask, mask);
		spin_unlock_irqrestore(&etsects->lock, flags);
		return 0;

	case PTP_CLK_REQ_PPS:
		spin_lock_irqsave(&etsects->lock, flags);
		mask = gfar_read(&etsects->regs->tmr_temask);
		if (on)
			mask |= PP1EN;
		else
			mask &= ~PP1EN;
		gfar_write(&etsects->regs->tmr_temask, mask);
		spin_unlock_irqrestore(&etsects->lock, flags);
		return 0;

	default:
		break;
	}

	return -EOPNOTSUPP;
}

static struct ptp_clock_info ptp_gianfar_caps = {
	.owner		= THIS_MODULE,
	.name		= "gianfar clock",
	.max_adj	= 512000,
	.n_alarm	= N_ALARM,
	.n_ext_ts	= N_EXT_TS,
	.n_per_out	= 0,
	.pps		= 1,
	.adjfreq	= ptp_gianfar_adjfreq,
	.adjtime	= ptp_gianfar_adjtime,
	.gettime	= ptp_gianfar_gettime,
	.settime	= ptp_gianfar_settime,
	.enable		= ptp_gianfar_enable,
};

/* OF device tree */

static int get_of_u32(struct device_node *node, char *str, u32 *val)
{
	int plen;
	const u32 *prop = of_get_property(node, str, &plen);

	if (!prop || plen != sizeof(*prop))
		return -1;
	*val = *prop;
	return 0;
}

static int gianfar_ptp_probe(struct platform_device *dev)
{
	struct device_node *node = dev->dev.of_node;
	struct etsects *etsects;
	struct timespec now;
	int err = -ENOMEM;
	u32 tmr_ctrl;
	unsigned long flags;

	etsects = kzalloc(sizeof(*etsects), GFP_KERNEL);
	if (!etsects)
		goto no_memory;

	err = -ENODEV;

	etsects->caps = ptp_gianfar_caps;
	etsects->cksel = DEFAULT_CKSEL;

	if (get_of_u32(node, "fsl,tclk-period", &etsects->tclk_period) ||
	    get_of_u32(node, "fsl,tmr-prsc", &etsects->tmr_prsc) ||
	    get_of_u32(node, "fsl,tmr-add", &etsects->tmr_add) ||
	    get_of_u32(node, "fsl,tmr-fiper1", &etsects->tmr_fiper1) ||
	    get_of_u32(node, "fsl,tmr-fiper2", &etsects->tmr_fiper2) ||
	    get_of_u32(node, "fsl,max-adj", &etsects->caps.max_adj)) {
		pr_err("device tree node missing required elements\n");
		goto no_node;
	}

	etsects->irq = platform_get_irq(dev, 0);

	if (etsects->irq == NO_IRQ) {
		pr_err("irq not in device tree\n");
		goto no_node;
	}
	if (request_irq(etsects->irq, isr, 0, DRIVER, etsects)) {
		pr_err("request_irq failed\n");
		goto no_node;
	}

	etsects->rsrc = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!etsects->rsrc) {
		pr_err("no resource\n");
		goto no_resource;
	}
	if (request_resource(&ioport_resource, etsects->rsrc)) {
		pr_err("resource busy\n");
		goto no_resource;
	}

	spin_lock_init(&etsects->lock);

	etsects->regs = ioremap(etsects->rsrc->start,
				resource_size(etsects->rsrc));
	if (!etsects->regs) {
		pr_err("ioremap ptp registers failed\n");
		goto no_ioremap;
	}
	getnstimeofday(&now);
	ptp_gianfar_settime(&etsects->caps, &now);

	tmr_ctrl =
	  (etsects->tclk_period & TCLK_PERIOD_MASK) << TCLK_PERIOD_SHIFT |
	  (etsects->cksel & CKSEL_MASK) << CKSEL_SHIFT;

	spin_lock_irqsave(&etsects->lock, flags);

	gfar_write(&etsects->regs->tmr_ctrl,   tmr_ctrl);
	gfar_write(&etsects->regs->tmr_add,    etsects->tmr_add);
	gfar_write(&etsects->regs->tmr_prsc,   etsects->tmr_prsc);
	gfar_write(&etsects->regs->tmr_fiper1, etsects->tmr_fiper1);
	gfar_write(&etsects->regs->tmr_fiper2, etsects->tmr_fiper2);
	set_alarm(etsects);
	gfar_write(&etsects->regs->tmr_ctrl,   tmr_ctrl|FS|RTPE|TE|FRD);

	spin_unlock_irqrestore(&etsects->lock, flags);

	etsects->clock = ptp_clock_register(&etsects->caps);
	if (IS_ERR(etsects->clock)) {
		err = PTR_ERR(etsects->clock);
		goto no_clock;
	}
	gfar_phc_index = ptp_clock_index(etsects->clock);

	dev_set_drvdata(&dev->dev, etsects);

	return 0;

no_clock:
no_ioremap:
	release_resource(etsects->rsrc);
no_resource:
	free_irq(etsects->irq, etsects);
no_node:
	kfree(etsects);
no_memory:
	return err;
}

static int gianfar_ptp_remove(struct platform_device *dev)
{
	struct etsects *etsects = dev_get_drvdata(&dev->dev);

	gfar_write(&etsects->regs->tmr_temask, 0);
	gfar_write(&etsects->regs->tmr_ctrl,   0);

	gfar_phc_index = -1;
	ptp_clock_unregister(etsects->clock);
	iounmap(etsects->regs);
	release_resource(etsects->rsrc);
	free_irq(etsects->irq, etsects);
	kfree(etsects);

	return 0;
}

static struct of_device_id match_table[] = {
	{ .compatible = "fsl,etsec-ptp" },
	{},
};

static struct platform_driver gianfar_ptp_driver = {
	.driver = {
		.name		= "gianfar_ptp",
		.of_match_table	= match_table,
		.owner		= THIS_MODULE,
	},
	.probe       = gianfar_ptp_probe,
	.remove      = gianfar_ptp_remove,
};

module_platform_driver(gianfar_ptp_driver);

MODULE_AUTHOR("Richard Cochran <richardcochran@gmail.com>");
MODULE_DESCRIPTION("PTP clock using the eTSEC");
MODULE_LICENSE("GPL");
