/*
 * Afatech AF9033 demodulator driver
 *
 * Copyright (C) 2009 Antti Palosaari <crope@iki.fi>
 * Copyright (C) 2012 Antti Palosaari <crope@iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef AF9033_PRIV_H
#define AF9033_PRIV_H

#include "dvb_frontend.h"
#include "af9033.h"

struct reg_val {
	u32 reg;
	u8  val;
};

struct reg_val_mask {
	u32 reg;
	u8  val;
	u8  mask;
};

struct coeff {
	u32 clock;
	u32 bandwidth_hz;
	u8 val[36];
};

struct clock_adc {
	u32 clock;
	u32 adc;
};

struct val_snr {
	u32 val;
	u8 snr;
};

/* Xtal clock vs. ADC clock lookup table */
static const struct clock_adc clock_adc_lut[] = {
	{ 16384000, 20480000 },
	{ 20480000, 20480000 },
	{ 36000000, 20250000 },
	{ 30000000, 20156250 },
	{ 26000000, 20583333 },
	{ 28000000, 20416667 },
	{ 32000000, 20500000 },
	{ 34000000, 20187500 },
	{ 24000000, 20500000 },
	{ 22000000, 20625000 },
	{ 12000000, 20250000 },
};

/* pre-calculated coeff lookup table */
static const struct coeff coeff_lut[] = {
	/* 12.000 MHz */
	{ 12000000, 8000000, {
		0x01, 0xce, 0x55, 0xc9, 0x00, 0xe7, 0x2a, 0xe4, 0x00, 0x73,
		0x99, 0x0f, 0x00, 0x73, 0x95, 0x72, 0x00, 0x73, 0x91, 0xd5,
		0x00, 0x39, 0xca, 0xb9, 0x00, 0xe7, 0x2a, 0xe4, 0x00, 0x73,
		0x95, 0x72, 0x37, 0x02, 0xce, 0x01 }
	},
	{ 12000000, 7000000, {
		0x01, 0x94, 0x8b, 0x10, 0x00, 0xca, 0x45, 0x88, 0x00, 0x65,
		0x25, 0xed, 0x00, 0x65, 0x22, 0xc4, 0x00, 0x65, 0x1f, 0x9b,
		0x00, 0x32, 0x91, 0x62, 0x00, 0xca, 0x45, 0x88, 0x00, 0x65,
		0x22, 0xc4, 0x88, 0x02, 0x95, 0x01 }
	},
	{ 12000000, 6000000, {
		0x01, 0x5a, 0xc0, 0x56, 0x00, 0xad, 0x60, 0x2b, 0x00, 0x56,
		0xb2, 0xcb, 0x00, 0x56, 0xb0, 0x15, 0x00, 0x56, 0xad, 0x60,
		0x00, 0x2b, 0x58, 0x0b, 0x00, 0xad, 0x60, 0x2b, 0x00, 0x56,
		0xb0, 0x15, 0xf4, 0x02, 0x5b, 0x01 }
	},
};

/* QPSK SNR lookup table */
static const struct val_snr qpsk_snr_lut[] = {
	{ 0x0b4771,  0 },
	{ 0x0c1aed,  1 },
	{ 0x0d0d27,  2 },
	{ 0x0e4d19,  3 },
	{ 0x0e5da8,  4 },
	{ 0x107097,  5 },
	{ 0x116975,  6 },
	{ 0x1252d9,  7 },
	{ 0x131fa4,  8 },
	{ 0x13d5e1,  9 },
	{ 0x148e53, 10 },
	{ 0x15358b, 11 },
	{ 0x15dd29, 12 },
	{ 0x168112, 13 },
	{ 0x170b61, 14 },
	{ 0x17a532, 15 },
	{ 0x180f94, 16 },
	{ 0x186ed2, 17 },
	{ 0x18b271, 18 },
	{ 0x18e118, 19 },
	{ 0x18ff4b, 20 },
	{ 0x190af1, 21 },
	{ 0x191451, 22 },
	{ 0xffffff, 23 },
};

/* QAM16 SNR lookup table */
static const struct val_snr qam16_snr_lut[] = {
	{ 0x04f0d5,  0 },
	{ 0x05387a,  1 },
	{ 0x0573a4,  2 },
	{ 0x05a99e,  3 },
	{ 0x05cc80,  4 },
	{ 0x05eb62,  5 },
	{ 0x05fecf,  6 },
	{ 0x060b80,  7 },
	{ 0x062501,  8 },
	{ 0x064865,  9 },
	{ 0x069604, 10 },
	{ 0x06f356, 11 },
	{ 0x07706a, 12 },
	{ 0x0804d3, 13 },
	{ 0x089d1a, 14 },
	{ 0x093e3d, 15 },
	{ 0x09e35d, 16 },
	{ 0x0a7c3c, 17 },
	{ 0x0afaf8, 18 },
	{ 0x0b719d, 19 },
	{ 0x0bda6a, 20 },
	{ 0x0c0c75, 21 },
	{ 0x0c3f7d, 22 },
	{ 0x0c5e62, 23 },
	{ 0x0c6c31, 24 },
	{ 0x0c7925, 25 },
	{ 0xffffff, 26 },
};

/* QAM64 SNR lookup table */
static const struct val_snr qam64_snr_lut[] = {
	{ 0x0256d0,  0 },
	{ 0x027a65,  1 },
	{ 0x029873,  2 },
	{ 0x02b7fe,  3 },
	{ 0x02cf1e,  4 },
	{ 0x02e234,  5 },
	{ 0x02f409,  6 },
	{ 0x030046,  7 },
	{ 0x030844,  8 },
	{ 0x030a02,  9 },
	{ 0x030cde, 10 },
	{ 0x031031, 11 },
	{ 0x03144c, 12 },
	{ 0x0315dd, 13 },
	{ 0x031920, 14 },
	{ 0x0322d0, 15 },
	{ 0x0339fc, 16 },
	{ 0x0364a1, 17 },
	{ 0x038bcc, 18 },
	{ 0x03c7d3, 19 },
	{ 0x0408cc, 20 },
	{ 0x043bed, 21 },
	{ 0x048061, 22 },
	{ 0x04be95, 23 },
	{ 0x04fa7d, 24 },
	{ 0x052405, 25 },
	{ 0x05570d, 26 },
	{ 0x059feb, 27 },
	{ 0x05bf38, 28 },
	{ 0xffffff, 29 },
};

static const struct reg_val ofsm_init[] = {
	{ 0x800051, 0x01 },
	{ 0x800070, 0x0a },
	{ 0x80007e, 0x04 },
	{ 0x800081, 0x0a },
	{ 0x80008a, 0x01 },
	{ 0x80008e, 0x01 },
	{ 0x800092, 0x06 },
	{ 0x800099, 0x01 },
	{ 0x80009f, 0xe1 },
	{ 0x8000a0, 0xcf },
	{ 0x8000a3, 0x01 },
	{ 0x8000a5, 0x01 },
	{ 0x8000a6, 0x01 },
	{ 0x8000a9, 0x00 },
	{ 0x8000aa, 0x01 },
	{ 0x8000b0, 0x01 },
	{ 0x8000c4, 0x05 },
	{ 0x8000c8, 0x19 },
	{ 0x80f000, 0x0f },
	{ 0x80f016, 0x10 },
	{ 0x80f017, 0x04 },
	{ 0x80f018, 0x05 },
	{ 0x80f019, 0x04 },
	{ 0x80f01a, 0x05 },
	{ 0x80f021, 0x03 },
	{ 0x80f022, 0x0a },
	{ 0x80f023, 0x0a },
	{ 0x80f02b, 0x00 },
	{ 0x80f02c, 0x01 },
	{ 0x80f064, 0x03 },
	{ 0x80f065, 0xf9 },
	{ 0x80f066, 0x03 },
	{ 0x80f067, 0x01 },
	{ 0x80f06f, 0xe0 },
	{ 0x80f070, 0x03 },
	{ 0x80f072, 0x0f },
	{ 0x80f073, 0x03 },
	{ 0x80f078, 0x00 },
	{ 0x80f087, 0x00 },
	{ 0x80f09b, 0x3f },
	{ 0x80f09c, 0x00 },
	{ 0x80f09d, 0x20 },
	{ 0x80f09e, 0x00 },
	{ 0x80f09f, 0x0c },
	{ 0x80f0a0, 0x00 },
	{ 0x80f130, 0x04 },
	{ 0x80f132, 0x04 },
	{ 0x80f144, 0x1a },
	{ 0x80f146, 0x00 },
	{ 0x80f14a, 0x01 },
	{ 0x80f14c, 0x00 },
	{ 0x80f14d, 0x00 },
	{ 0x80f14f, 0x04 },
	{ 0x80f158, 0x7f },
	{ 0x80f15a, 0x00 },
	{ 0x80f15b, 0x08 },
	{ 0x80f15d, 0x03 },
	{ 0x80f15e, 0x05 },
	{ 0x80f163, 0x05 },
	{ 0x80f166, 0x01 },
	{ 0x80f167, 0x40 },
	{ 0x80f168, 0x0f },
	{ 0x80f17a, 0x00 },
	{ 0x80f17b, 0x00 },
	{ 0x80f183, 0x01 },
	{ 0x80f19d, 0x40 },
	{ 0x80f1bc, 0x36 },
	{ 0x80f1bd, 0x00 },
	{ 0x80f1cb, 0xa0 },
	{ 0x80f1cc, 0x01 },
	{ 0x80f204, 0x10 },
	{ 0x80f214, 0x00 },
	{ 0x80f40e, 0x0a },
	{ 0x80f40f, 0x40 },
	{ 0x80f410, 0x08 },
	{ 0x80f55f, 0x0a },
	{ 0x80f561, 0x15 },
	{ 0x80f562, 0x20 },
	{ 0x80f5df, 0xfb },
	{ 0x80f5e0, 0x00 },
	{ 0x80f5e3, 0x09 },
	{ 0x80f5e4, 0x01 },
	{ 0x80f5e5, 0x01 },
	{ 0x80f5f8, 0x01 },
	{ 0x80f5fd, 0x01 },
	{ 0x80f600, 0x05 },
	{ 0x80f601, 0x08 },
	{ 0x80f602, 0x0b },
	{ 0x80f603, 0x0e },
	{ 0x80f604, 0x11 },
	{ 0x80f605, 0x14 },
	{ 0x80f606, 0x17 },
	{ 0x80f607, 0x1f },
	{ 0x80f60e, 0x00 },
	{ 0x80f60f, 0x04 },
	{ 0x80f610, 0x32 },
	{ 0x80f611, 0x10 },
	{ 0x80f707, 0xfc },
	{ 0x80f708, 0x00 },
	{ 0x80f709, 0x37 },
	{ 0x80f70a, 0x00 },
	{ 0x80f78b, 0x01 },
	{ 0x80f80f, 0x40 },
	{ 0x80f810, 0x54 },
	{ 0x80f811, 0x5a },
	{ 0x80f905, 0x01 },
	{ 0x80fb06, 0x03 },
	{ 0x80fd8b, 0x00 },
};

/* Infineon TUA 9001 tuner init
   AF9033_TUNER_TUA9001    = 0x27 */
static const struct reg_val tuner_init_tua9001[] = {
	{ 0x800046, 0x27 },
	{ 0x800057, 0x00 },
	{ 0x800058, 0x01 },
	{ 0x80005f, 0x00 },
	{ 0x800060, 0x00 },
	{ 0x80006d, 0x00 },
	{ 0x800071, 0x05 },
	{ 0x800072, 0x02 },
	{ 0x800074, 0x01 },
	{ 0x800075, 0x03 },
	{ 0x800076, 0x02 },
	{ 0x800077, 0x00 },
	{ 0x800078, 0x01 },
	{ 0x800079, 0x00 },
	{ 0x80007a, 0x7e },
	{ 0x80007b, 0x3e },
	{ 0x800093, 0x00 },
	{ 0x800094, 0x01 },
	{ 0x800095, 0x02 },
	{ 0x800096, 0x01 },
	{ 0x800098, 0x0a },
	{ 0x80009b, 0x05 },
	{ 0x80009c, 0x80 },
	{ 0x8000b3, 0x00 },
	{ 0x8000c5, 0x01 },
	{ 0x8000c6, 0x00 },
	{ 0x8000c9, 0x5d },
	{ 0x80f007, 0x00 },
	{ 0x80f01f, 0x82 },
	{ 0x80f020, 0x00 },
	{ 0x80f029, 0x82 },
	{ 0x80f02a, 0x00 },
	{ 0x80f047, 0x00 },
	{ 0x80f054, 0x00 },
	{ 0x80f055, 0x00 },
	{ 0x80f077, 0x01 },
	{ 0x80f1e6, 0x00 },
};

/* Fitipower fc0011 tuner init
   AF9033_TUNER_FC0011    = 0x28 */
static const struct reg_val tuner_init_fc0011[] = {
	{ 0x800046, 0x28 },
	{ 0x800057, 0x00 },
	{ 0x800058, 0x01 },
	{ 0x80005f, 0x00 },
	{ 0x800060, 0x00 },
	{ 0x800068, 0xa5 },
	{ 0x80006e, 0x01 },
	{ 0x800071, 0x0a },
	{ 0x800072, 0x02 },
	{ 0x800074, 0x01 },
	{ 0x800079, 0x01 },
	{ 0x800093, 0x00 },
	{ 0x800094, 0x00 },
	{ 0x800095, 0x00 },
	{ 0x800096, 0x00 },
	{ 0x80009b, 0x2d },
	{ 0x80009c, 0x60 },
	{ 0x80009d, 0x23 },
	{ 0x8000a4, 0x50 },
	{ 0x8000ad, 0x50 },
	{ 0x8000b3, 0x01 },
	{ 0x8000b7, 0x88 },
	{ 0x8000b8, 0xa6 },
	{ 0x8000c5, 0x01 },
	{ 0x8000c6, 0x01 },
	{ 0x8000c9, 0x69 },
	{ 0x80f007, 0x00 },
	{ 0x80f00a, 0x1b },
	{ 0x80f00b, 0x1b },
	{ 0x80f00c, 0x1b },
	{ 0x80f00d, 0x1b },
	{ 0x80f00e, 0xff },
	{ 0x80f00f, 0x01 },
	{ 0x80f010, 0x00 },
	{ 0x80f011, 0x02 },
	{ 0x80f012, 0xff },
	{ 0x80f013, 0x01 },
	{ 0x80f014, 0x00 },
	{ 0x80f015, 0x02 },
	{ 0x80f01b, 0xef },
	{ 0x80f01c, 0x01 },
	{ 0x80f01d, 0x0f },
	{ 0x80f01e, 0x02 },
	{ 0x80f01f, 0x6e },
	{ 0x80f020, 0x00 },
	{ 0x80f025, 0xde },
	{ 0x80f026, 0x00 },
	{ 0x80f027, 0x0a },
	{ 0x80f028, 0x03 },
	{ 0x80f029, 0x6e },
	{ 0x80f02a, 0x00 },
	{ 0x80f047, 0x00 },
	{ 0x80f054, 0x00 },
	{ 0x80f055, 0x00 },
	{ 0x80f077, 0x01 },
	{ 0x80f1e6, 0x00 },
};

/* Fitipower FC0012 tuner init
   AF9033_TUNER_FC0012    = 0x2e */
static const struct reg_val tuner_init_fc0012[] = {
	{ 0x800046, 0x2e },
	{ 0x800057, 0x00 },
	{ 0x800058, 0x01 },
	{ 0x800059, 0x01 },
	{ 0x80005f, 0x00 },
	{ 0x800060, 0x00 },
	{ 0x80006d, 0x00 },
	{ 0x800071, 0x05 },
	{ 0x800072, 0x02 },
	{ 0x800074, 0x01 },
	{ 0x800075, 0x03 },
	{ 0x800076, 0x02 },
	{ 0x800077, 0x01 },
	{ 0x800078, 0x00 },
	{ 0x800079, 0x00 },
	{ 0x80007a, 0x90 },
	{ 0x80007b, 0x90 },
	{ 0x800093, 0x00 },
	{ 0x800094, 0x01 },
	{ 0x800095, 0x02 },
	{ 0x800096, 0x01 },
	{ 0x800098, 0x0a },
	{ 0x80009b, 0x05 },
	{ 0x80009c, 0x80 },
	{ 0x8000b3, 0x00 },
	{ 0x8000c5, 0x01 },
	{ 0x8000c6, 0x00 },
	{ 0x8000c9, 0x5d },
	{ 0x80f007, 0x00 },
	{ 0x80f01f, 0xa0 },
	{ 0x80f020, 0x00 },
	{ 0x80f029, 0x82 },
	{ 0x80f02a, 0x00 },
	{ 0x80f047, 0x00 },
	{ 0x80f054, 0x00 },
	{ 0x80f055, 0x00 },
	{ 0x80f077, 0x01 },
	{ 0x80f1e6, 0x00 },
};

/* MaxLinear MxL5007T tuner init
   AF9033_TUNER_MXL5007T    = 0xa0 */
static const struct reg_val tuner_init_mxl5007t[] = {
	{ 0x800046, 0x1b },
	{ 0x800057, 0x01 },
	{ 0x800058, 0x01 },
	{ 0x80005f, 0x00 },
	{ 0x800060, 0x00 },
	{ 0x800068, 0x96 },
	{ 0x800071, 0x05 },
	{ 0x800072, 0x02 },
	{ 0x800074, 0x01 },
	{ 0x800079, 0x01 },
	{ 0x800093, 0x00 },
	{ 0x800094, 0x00 },
	{ 0x800095, 0x00 },
	{ 0x800096, 0x00 },
	{ 0x8000b3, 0x01 },
	{ 0x8000c1, 0x01 },
	{ 0x8000c2, 0x00 },
	{ 0x80f007, 0x00 },
	{ 0x80f00c, 0x19 },
	{ 0x80f00d, 0x1a },
	{ 0x80f012, 0xda },
	{ 0x80f013, 0x00 },
	{ 0x80f014, 0x00 },
	{ 0x80f015, 0x02 },
	{ 0x80f01f, 0x82 },
	{ 0x80f020, 0x00 },
	{ 0x80f029, 0x82 },
	{ 0x80f02a, 0x00 },
	{ 0x80f077, 0x02 },
	{ 0x80f1e6, 0x00 },
};

/* NXP TDA 18218HN tuner init
   AF9033_TUNER_TDA18218    = 0xa1 */
static const struct reg_val tuner_init_tda18218[] = {
	{0x800046, 0xa1},
	{0x800057, 0x01},
	{0x800058, 0x01},
	{0x80005f, 0x00},
	{0x800060, 0x00},
	{0x800071, 0x05},
	{0x800072, 0x02},
	{0x800074, 0x01},
	{0x800079, 0x01},
	{0x800093, 0x00},
	{0x800094, 0x00},
	{0x800095, 0x00},
	{0x800096, 0x00},
	{0x8000b3, 0x01},
	{0x8000c3, 0x01},
	{0x8000c4, 0x00},
	{0x80f007, 0x00},
	{0x80f00c, 0x19},
	{0x80f00d, 0x1a},
	{0x80f012, 0xda},
	{0x80f013, 0x00},
	{0x80f014, 0x00},
	{0x80f015, 0x02},
	{0x80f01f, 0x82},
	{0x80f020, 0x00},
	{0x80f029, 0x82},
	{0x80f02a, 0x00},
	{0x80f077, 0x02},
	{0x80f1e6, 0x00},
};

/* FCI FC2580 tuner init */
static const struct reg_val tuner_init_fc2580[] = {
	{ 0x800046, 0x32 },
	{ 0x800057, 0x01 },
	{ 0x800058, 0x00 },
	{ 0x80005f, 0x00 },
	{ 0x800060, 0x00 },
	{ 0x800071, 0x05 },
	{ 0x800072, 0x02 },
	{ 0x800074, 0x01 },
	{ 0x800079, 0x01 },
	{ 0x800093, 0x00 },
	{ 0x800094, 0x00 },
	{ 0x800095, 0x00 },
	{ 0x800096, 0x05 },
	{ 0x8000b3, 0x01 },
	{ 0x8000c5, 0x01 },
	{ 0x8000c6, 0x00 },
	{ 0x8000d1, 0x01 },
	{ 0x80f007, 0x00 },
	{ 0x80f00c, 0x19 },
	{ 0x80f00d, 0x1a },
	{ 0x80f00e, 0x00 },
	{ 0x80f00f, 0x02 },
	{ 0x80f010, 0x00 },
	{ 0x80f011, 0x02 },
	{ 0x80f012, 0x00 },
	{ 0x80f013, 0x02 },
	{ 0x80f014, 0x00 },
	{ 0x80f015, 0x02 },
	{ 0x80f01f, 0x96 },
	{ 0x80f020, 0x00 },
	{ 0x80f029, 0x96 },
	{ 0x80f02a, 0x00 },
	{ 0x80f077, 0x01 },
	{ 0x80f1e6, 0x01 },
};

static const struct reg_val ofsm_init_it9135_v1[] = {
	{ 0x800051, 0x01 },
	{ 0x800070, 0x0a },
	{ 0x80007e, 0x04 },
	{ 0x800081, 0x0a },
	{ 0x80008a, 0x01 },
	{ 0x80008e, 0x01 },
	{ 0x800092, 0x06 },
	{ 0x800099, 0x01 },
	{ 0x80009f, 0xe1 },
	{ 0x8000a0, 0xcf },
	{ 0x8000a3, 0x01 },
	{ 0x8000a5, 0x01 },
	{ 0x8000a6, 0x01 },
	{ 0x8000a9, 0x00 },
	{ 0x8000aa, 0x01 },
	{ 0x8000b0, 0x01 },
	{ 0x8000c2, 0x05 },
	{ 0x8000c6, 0x19 },
	{ 0x80f000, 0x0f },
	{ 0x80f016, 0x10 },
	{ 0x80f017, 0x04 },
	{ 0x80f018, 0x05 },
	{ 0x80f019, 0x04 },
	{ 0x80f01a, 0x05 },
	{ 0x80f021, 0x03 },
	{ 0x80f022, 0x0a },
	{ 0x80f023, 0x0a },
	{ 0x80f02b, 0x00 },
	{ 0x80f02c, 0x01 },
	{ 0x80f064, 0x03 },
	{ 0x80f065, 0xf9 },
	{ 0x80f066, 0x03 },
	{ 0x80f067, 0x01 },
	{ 0x80f06f, 0xe0 },
	{ 0x80f070, 0x03 },
	{ 0x80f072, 0x0f },
	{ 0x80f073, 0x03 },
	{ 0x80f078, 0x00 },
	{ 0x80f087, 0x00 },
	{ 0x80f09b, 0x3f },
	{ 0x80f09c, 0x00 },
	{ 0x80f09d, 0x20 },
	{ 0x80f09e, 0x00 },
	{ 0x80f09f, 0x0c },
	{ 0x80f0a0, 0x00 },
	{ 0x80f130, 0x04 },
	{ 0x80f132, 0x04 },
	{ 0x80f144, 0x1a },
	{ 0x80f146, 0x00 },
	{ 0x80f14a, 0x01 },
	{ 0x80f14c, 0x00 },
	{ 0x80f14d, 0x00 },
	{ 0x80f14f, 0x04 },
	{ 0x80f158, 0x7f },
	{ 0x80f15a, 0x00 },
	{ 0x80f15b, 0x08 },
	{ 0x80f15d, 0x03 },
	{ 0x80f15e, 0x05 },
	{ 0x80f163, 0x05 },
	{ 0x80f166, 0x01 },
	{ 0x80f167, 0x40 },
	{ 0x80f168, 0x0f },
	{ 0x80f17a, 0x00 },
	{ 0x80f17b, 0x00 },
	{ 0x80f183, 0x01 },
	{ 0x80f19d, 0x40 },
	{ 0x80f1bc, 0x36 },
	{ 0x80f1bd, 0x00 },
	{ 0x80f1cb, 0xa0 },
	{ 0x80f1cc, 0x01 },
	{ 0x80f204, 0x10 },
	{ 0x80f214, 0x00 },
	{ 0x80f40e, 0x0a },
	{ 0x80f40f, 0x40 },
	{ 0x80f410, 0x08 },
	{ 0x80f55f, 0x0a },
	{ 0x80f561, 0x15 },
	{ 0x80f562, 0x20 },
	{ 0x80f5df, 0xfb },
	{ 0x80f5e0, 0x00 },
	{ 0x80f5e3, 0x09 },
	{ 0x80f5e4, 0x01 },
	{ 0x80f5e5, 0x01 },
	{ 0x80f5f8, 0x01 },
	{ 0x80f5fd, 0x01 },
	{ 0x80f600, 0x05 },
	{ 0x80f601, 0x08 },
	{ 0x80f602, 0x0b },
	{ 0x80f603, 0x0e },
	{ 0x80f604, 0x11 },
	{ 0x80f605, 0x14 },
	{ 0x80f606, 0x17 },
	{ 0x80f607, 0x1f },
	{ 0x80f60e, 0x00 },
	{ 0x80f60f, 0x04 },
	{ 0x80f610, 0x32 },
	{ 0x80f611, 0x10 },
	{ 0x80f707, 0xfc },
	{ 0x80f708, 0x00 },
	{ 0x80f709, 0x37 },
	{ 0x80f70a, 0x00 },
	{ 0x80f78b, 0x01 },
	{ 0x80f80f, 0x40 },
	{ 0x80f810, 0x54 },
	{ 0x80f811, 0x5a },
	{ 0x80f905, 0x01 },
	{ 0x80fb06, 0x03 },
	{ 0x80fd8b, 0x00 },
};

static const struct reg_val ofsm_init_it9135_v2[] = {
	{ 0x800051, 0x01 },
	{ 0x800070, 0x0a },
	{ 0x80007e, 0x04 },
	{ 0x800081, 0x0a },
	{ 0x80008a, 0x01 },
	{ 0x80008e, 0x01 },
	{ 0x800092, 0x06 },
	{ 0x800099, 0x01 },
	{ 0x80009f, 0xe1 },
	{ 0x8000a0, 0xcf },
	{ 0x8000a3, 0x01 },
	{ 0x8000a5, 0x01 },
	{ 0x8000a6, 0x01 },
	{ 0x8000a9, 0x00 },
	{ 0x8000aa, 0x01 },
	{ 0x8000b0, 0x01 },
	{ 0x8000c2, 0x05 },
	{ 0x8000c6, 0x19 },
	{ 0x80f000, 0x0f },
	{ 0x80f02b, 0x00 },
	{ 0x80f064, 0x03 },
	{ 0x80f065, 0xf9 },
	{ 0x80f066, 0x03 },
	{ 0x80f067, 0x01 },
	{ 0x80f06f, 0xe0 },
	{ 0x80f070, 0x03 },
	{ 0x80f072, 0x0f },
	{ 0x80f073, 0x03 },
	{ 0x80f078, 0x00 },
	{ 0x80f087, 0x00 },
	{ 0x80f09b, 0x3f },
	{ 0x80f09c, 0x00 },
	{ 0x80f09d, 0x20 },
	{ 0x80f09e, 0x00 },
	{ 0x80f09f, 0x0c },
	{ 0x80f0a0, 0x00 },
	{ 0x80f130, 0x04 },
	{ 0x80f132, 0x04 },
	{ 0x80f144, 0x1a },
	{ 0x80f146, 0x00 },
	{ 0x80f14a, 0x01 },
	{ 0x80f14c, 0x00 },
	{ 0x80f14d, 0x00 },
	{ 0x80f14f, 0x04 },
	{ 0x80f158, 0x7f },
	{ 0x80f15a, 0x00 },
	{ 0x80f15b, 0x08 },
	{ 0x80f15d, 0x03 },
	{ 0x80f15e, 0x05 },
	{ 0x80f163, 0x05 },
	{ 0x80f166, 0x01 },
	{ 0x80f167, 0x40 },
	{ 0x80f168, 0x0f },
	{ 0x80f17a, 0x00 },
	{ 0x80f17b, 0x00 },
	{ 0x80f183, 0x01 },
	{ 0x80f19d, 0x40 },
	{ 0x80f1bc, 0x36 },
	{ 0x80f1bd, 0x00 },
	{ 0x80f1cb, 0xa0 },
	{ 0x80f1cc, 0x01 },
	{ 0x80f204, 0x10 },
	{ 0x80f214, 0x00 },
	{ 0x80f40e, 0x0a },
	{ 0x80f40f, 0x40 },
	{ 0x80f410, 0x08 },
	{ 0x80f55f, 0x0a },
	{ 0x80f561, 0x15 },
	{ 0x80f562, 0x20 },
	{ 0x80f5e3, 0x09 },
	{ 0x80f5e4, 0x01 },
	{ 0x80f5e5, 0x01 },
	{ 0x80f600, 0x05 },
	{ 0x80f601, 0x08 },
	{ 0x80f602, 0x0b },
	{ 0x80f603, 0x0e },
	{ 0x80f604, 0x11 },
	{ 0x80f605, 0x14 },
	{ 0x80f606, 0x17 },
	{ 0x80f607, 0x1f },
	{ 0x80f60e, 0x00 },
	{ 0x80f60f, 0x04 },
	{ 0x80f610, 0x32 },
	{ 0x80f611, 0x10 },
	{ 0x80f707, 0xfc },
	{ 0x80f708, 0x00 },
	{ 0x80f709, 0x37 },
	{ 0x80f70a, 0x00 },
	{ 0x80f78b, 0x01 },
	{ 0x80f80f, 0x40 },
	{ 0x80f810, 0x54 },
	{ 0x80f811, 0x5a },
	{ 0x80f905, 0x01 },
	{ 0x80fb06, 0x03 },
	{ 0x80fd8b, 0x00 },
};

#endif /* AF9033_PRIV_H */

