/* Copyright (c) 2007, 2012-2013 The Linux Foundation. All rights reserved.
 * Copyright (C) 2007 Google Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/types.h>

#include "mdss_fb.h"
#include "mdp3_ppp.h"

#define MDP_IS_IMGTYPE_BAD(x) ((x) >= MDP_IMGTYPE_LIMIT)

/* bg_config_lut not needed since it is same as src */
const uint32_t src_cfg_lut[MDP_IMGTYPE_LIMIT] = {
	[MDP_RGB_565] = MDP_RGB_565_SRC_REG,
	[MDP_BGR_565] = MDP_RGB_565_SRC_REG,
	[MDP_RGB_888] = MDP_RGB_888_SRC_REG,
	[MDP_BGR_888] = MDP_RGB_888_SRC_REG,
	[MDP_BGRA_8888] = MDP_RGBX_8888_SRC_REG,
	[MDP_RGBA_8888] = MDP_RGBX_8888_SRC_REG,
	[MDP_ARGB_8888] = MDP_RGBX_8888_SRC_REG,
	[MDP_XRGB_8888] = MDP_RGBX_8888_SRC_REG,
	[MDP_RGBX_8888] = MDP_RGBX_8888_SRC_REG,
	[MDP_Y_CRCB_H2V2] = MDP_Y_CBCR_H2V2_SRC_REG,
	[MDP_Y_CBCR_H2V2] = MDP_Y_CBCR_H2V2_SRC_REG,
	[MDP_Y_CBCR_H2V2_ADRENO] = MDP_Y_CBCR_H2V2_SRC_REG,
	[MDP_Y_CBCR_H2V2_VENUS] = MDP_Y_CBCR_H2V2_SRC_REG,
	[MDP_YCRYCB_H2V1] = MDP_YCRYCB_H2V1_SRC_REG,
	[MDP_Y_CBCR_H2V1] = MDP_Y_CRCB_H2V1_SRC_REG,
	[MDP_Y_CRCB_H2V1] = MDP_Y_CRCB_H2V1_SRC_REG,
	[MDP_BGRX_8888] = MDP_RGBX_8888_SRC_REG,
};

const uint32_t out_cfg_lut[MDP_IMGTYPE_LIMIT] = {
	[MDP_RGB_565] = MDP_RGB_565_DST_REG,
	[MDP_BGR_565] = MDP_RGB_565_DST_REG,
	[MDP_RGB_888] = MDP_RGB_888_DST_REG,
	[MDP_BGR_888] = MDP_RGB_888_DST_REG,
	[MDP_BGRA_8888] = MDP_RGBX_8888_DST_REG,
	[MDP_RGBA_8888] = MDP_RGBX_8888_DST_REG,
	[MDP_ARGB_8888] = MDP_RGBX_8888_DST_REG,
	[MDP_XRGB_8888] = MDP_RGBX_8888_DST_REG,
	[MDP_RGBX_8888] = MDP_RGBX_8888_DST_REG,
	[MDP_Y_CRCB_H2V2] = MDP_Y_CBCR_H2V2_DST_REG,
	[MDP_Y_CBCR_H2V2] = MDP_Y_CBCR_H2V2_DST_REG,
	[MDP_Y_CBCR_H2V2_ADRENO] = MDP_Y_CBCR_H2V2_DST_REG,
	[MDP_Y_CBCR_H2V2_VENUS] = MDP_Y_CBCR_H2V2_DST_REG,
	[MDP_YCRYCB_H2V1] = MDP_YCRYCB_H2V1_DST_REG,
	[MDP_Y_CBCR_H2V1] = MDP_Y_CRCB_H2V1_DST_REG,
	[MDP_Y_CRCB_H2V1] = MDP_Y_CRCB_H2V1_DST_REG,
	[MDP_BGRX_8888] = MDP_RGBX_8888_DST_REG,
};

const uint32_t pack_patt_lut[MDP_IMGTYPE_LIMIT] = {
	[MDP_RGB_565] = PPP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 8),
	[MDP_BGR_565] = PPP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 8),
	[MDP_RGB_888] = PPP_GET_PACK_PATTERN(0, CLR_R, CLR_G, CLR_B, 8),
	[MDP_BGR_888] = PPP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 8),
	[MDP_BGRA_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B,
		CLR_G, CLR_R, 8),
	[MDP_RGBA_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R,
		CLR_G, CLR_B, 8),
	[MDP_ARGB_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R,
		CLR_G, CLR_B, 8),
	[MDP_XRGB_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R,
		CLR_G, CLR_B, 8),
	[MDP_RGBX_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R,
		CLR_G, CLR_B, 8),
	[MDP_Y_CRCB_H2V2] = PPP_GET_PACK_PATTERN(0, 0, CLR_CR, CLR_CB, 8),
	[MDP_Y_CBCR_H2V2] = PPP_GET_PACK_PATTERN(0, 0, CLR_CB, CLR_CR, 8),
	[MDP_Y_CBCR_H2V2_ADRENO] = PPP_GET_PACK_PATTERN(0, 0, CLR_CB,
		CLR_CR, 8),
	[MDP_Y_CBCR_H2V2_VENUS] = PPP_GET_PACK_PATTERN(0, 0, CLR_CB,
		CLR_CR, 8),
	[MDP_YCRYCB_H2V1] = PPP_GET_PACK_PATTERN(CLR_Y,
		CLR_CR, CLR_Y, CLR_CB, 8),
	[MDP_Y_CBCR_H2V1] = PPP_GET_PACK_PATTERN(0, 0, CLR_CB, CLR_CR, 8),
	[MDP_Y_CRCB_H2V1] = PPP_GET_PACK_PATTERN(0, 0, CLR_CR, CLR_CB, 8),
	[MDP_BGRX_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B,
		CLR_G, CLR_R, 8),
};

const uint32_t swapped_pack_patt_lut[MDP_IMGTYPE_LIMIT] = {
	[MDP_RGB_565] = PPP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 8),
	[MDP_BGR_565] = PPP_GET_PACK_PATTERN(0, CLR_R, CLR_G, CLR_B, 8),
	[MDP_RGB_888] = PPP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 8),
	[MDP_BGR_888] = PPP_GET_PACK_PATTERN(0, CLR_R, CLR_G, CLR_B, 8),
	[MDP_BGRA_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R,
		CLR_G, CLR_B, 8),
	[MDP_RGBA_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B,
		CLR_G, CLR_R, 8),
	[MDP_ARGB_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B,
		CLR_G, CLR_R, 8),
	[MDP_XRGB_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B,
		CLR_G, CLR_R, 8),
	[MDP_RGBX_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B,
		CLR_G, CLR_R, 8),
	[MDP_Y_CRCB_H2V2] = PPP_GET_PACK_PATTERN(0, 0, CLR_CB, CLR_CR, 8),
	[MDP_Y_CBCR_H2V2] = PPP_GET_PACK_PATTERN(0, 0, CLR_CR, CLR_CB, 8),
	[MDP_Y_CBCR_H2V2_ADRENO] = PPP_GET_PACK_PATTERN(0, 0, CLR_CR,
		CLR_CB, 8),
	[MDP_Y_CBCR_H2V2_VENUS] = PPP_GET_PACK_PATTERN(0, 0, CLR_CR,
		CLR_CB, 8),
	[MDP_YCRYCB_H2V1] = PPP_GET_PACK_PATTERN(CLR_Y,
		CLR_CB, CLR_Y, CLR_CR, 8),
	[MDP_Y_CBCR_H2V1] = PPP_GET_PACK_PATTERN(0, 0, CLR_CR, CLR_CB, 8),
	[MDP_Y_CRCB_H2V1] = PPP_GET_PACK_PATTERN(0, 0, CLR_CB, CLR_CR, 8),
	[MDP_BGRX_8888] = PPP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R,
		CLR_G, CLR_B, 8),
};

const uint32_t dst_op_reg[MDP_IMGTYPE_LIMIT] = {
	[MDP_Y_CRCB_H2V2] = PPP_OP_DST_CHROMA_420,
	[MDP_Y_CBCR_H2V2] = PPP_OP_DST_CHROMA_420,
	[MDP_Y_CBCR_H2V1] = PPP_OP_DST_CHROMA_H2V1,
	[MDP_Y_CRCB_H2V1] = PPP_OP_DST_CHROMA_H2V1,
	[MDP_YCRYCB_H2V1] = PPP_OP_DST_CHROMA_H2V1,
};

const uint32_t src_op_reg[MDP_IMGTYPE_LIMIT] = {
	[MDP_Y_CRCB_H2V2] = PPP_OP_SRC_CHROMA_420 | PPP_OP_COLOR_SPACE_YCBCR,
	[MDP_Y_CBCR_H2V2] = PPP_OP_SRC_CHROMA_420 | PPP_OP_COLOR_SPACE_YCBCR,
	[MDP_Y_CBCR_H2V2_ADRENO] = PPP_OP_SRC_CHROMA_420 |
			PPP_OP_COLOR_SPACE_YCBCR,
	[MDP_Y_CBCR_H2V2_VENUS] = PPP_OP_SRC_CHROMA_420 |
			PPP_OP_COLOR_SPACE_YCBCR,
	[MDP_Y_CBCR_H2V1] = PPP_OP_SRC_CHROMA_H2V1,
	[MDP_Y_CRCB_H2V1] = PPP_OP_SRC_CHROMA_H2V1,
	[MDP_YCRYCB_H2V1] = PPP_OP_SRC_CHROMA_H2V1,
};

const uint32_t bytes_per_pixel[MDP_IMGTYPE_LIMIT] = {
	[MDP_RGB_565] = 2,
	[MDP_BGR_565] = 2,
	[MDP_RGB_888] = 3,
	[MDP_BGR_888] = 3,
	[MDP_XRGB_8888] = 4,
	[MDP_ARGB_8888] = 4,
	[MDP_RGBA_8888] = 4,
	[MDP_BGRA_8888] = 4,
	[MDP_RGBX_8888] = 4,
	[MDP_Y_CBCR_H2V1] = 1,
	[MDP_Y_CBCR_H2V2] = 1,
	[MDP_Y_CBCR_H2V2_ADRENO] = 1,
	[MDP_Y_CBCR_H2V2_VENUS] = 1,
	[MDP_Y_CRCB_H2V1] = 1,
	[MDP_Y_CRCB_H2V2] = 1,
	[MDP_YCRYCB_H2V1] = 2,
	[MDP_BGRX_8888] = 4,
};

const bool per_pixel_alpha[MDP_IMGTYPE_LIMIT] = {
	[MDP_BGRA_8888] = true,
	[MDP_RGBA_8888] = true,
	[MDP_ARGB_8888] = true,
};

const bool multi_plane[MDP_IMGTYPE_LIMIT] = {
	[MDP_Y_CRCB_H2V2] = true,
	[MDP_Y_CBCR_H2V2] = true,
	[MDP_Y_CBCR_H2V1] = true,
	[MDP_Y_CRCB_H2V1] = true,
};

/* lut default */
uint32_t default_pre_lut_val[PPP_LUT_MAX] = {
	0x0,
	0x151515,
	0x1d1d1d,
	0x232323,
	0x272727,
	0x2b2b2b,
	0x2f2f2f,
	0x333333,
	0x363636,
	0x393939,
	0x3b3b3b,
	0x3e3e3e,
	0x404040,
	0x434343,
	0x454545,
	0x474747,
	0x494949,
	0x4b4b4b,
	0x4d4d4d,
	0x4f4f4f,
	0x515151,
	0x535353,
	0x555555,
	0x565656,
	0x585858,
	0x5a5a5a,
	0x5b5b5b,
	0x5d5d5d,
	0x5e5e5e,
	0x606060,
	0x616161,
	0x636363,
	0x646464,
	0x666666,
	0x676767,
	0x686868,
	0x6a6a6a,
	0x6b6b6b,
	0x6c6c6c,
	0x6e6e6e,
	0x6f6f6f,
	0x707070,
	0x717171,
	0x727272,
	0x747474,
	0x757575,
	0x767676,
	0x777777,
	0x787878,
	0x797979,
	0x7a7a7a,
	0x7c7c7c,
	0x7d7d7d,
	0x7e7e7e,
	0x7f7f7f,
	0x808080,
	0x818181,
	0x828282,
	0x838383,
	0x848484,
	0x858585,
	0x868686,
	0x878787,
	0x888888,
	0x898989,
	0x8a8a8a,
	0x8b8b8b,
	0x8c8c8c,
	0x8d8d8d,
	0x8e8e8e,
	0x8f8f8f,
	0x8f8f8f,
	0x909090,
	0x919191,
	0x929292,
	0x939393,
	0x949494,
	0x959595,
	0x969696,
	0x969696,
	0x979797,
	0x989898,
	0x999999,
	0x9a9a9a,
	0x9b9b9b,
	0x9c9c9c,
	0x9c9c9c,
	0x9d9d9d,
	0x9e9e9e,
	0x9f9f9f,
	0xa0a0a0,
	0xa0a0a0,
	0xa1a1a1,
	0xa2a2a2,
	0xa3a3a3,
	0xa4a4a4,
	0xa4a4a4,
	0xa5a5a5,
	0xa6a6a6,
	0xa7a7a7,
	0xa7a7a7,
	0xa8a8a8,
	0xa9a9a9,
	0xaaaaaa,
	0xaaaaaa,
	0xababab,
	0xacacac,
	0xadadad,
	0xadadad,
	0xaeaeae,
	0xafafaf,
	0xafafaf,
	0xb0b0b0,
	0xb1b1b1,
	0xb2b2b2,
	0xb2b2b2,
	0xb3b3b3,
	0xb4b4b4,
	0xb4b4b4,
	0xb5b5b5,
	0xb6b6b6,
	0xb6b6b6,
	0xb7b7b7,
	0xb8b8b8,
	0xb8b8b8,
	0xb9b9b9,
	0xbababa,
	0xbababa,
	0xbbbbbb,
	0xbcbcbc,
	0xbcbcbc,
	0xbdbdbd,
	0xbebebe,
	0xbebebe,
	0xbfbfbf,
	0xc0c0c0,
	0xc0c0c0,
	0xc1c1c1,
	0xc1c1c1,
	0xc2c2c2,
	0xc3c3c3,
	0xc3c3c3,
	0xc4c4c4,
	0xc5c5c5,
	0xc5c5c5,
	0xc6c6c6,
	0xc6c6c6,
	0xc7c7c7,
	0xc8c8c8,
	0xc8c8c8,
	0xc9c9c9,
	0xc9c9c9,
	0xcacaca,
	0xcbcbcb,
	0xcbcbcb,
	0xcccccc,
	0xcccccc,
	0xcdcdcd,
	0xcecece,
	0xcecece,
	0xcfcfcf,
	0xcfcfcf,
	0xd0d0d0,
	0xd0d0d0,
	0xd1d1d1,
	0xd2d2d2,
	0xd2d2d2,
	0xd3d3d3,
	0xd3d3d3,
	0xd4d4d4,
	0xd4d4d4,
	0xd5d5d5,
	0xd6d6d6,
	0xd6d6d6,
	0xd7d7d7,
	0xd7d7d7,
	0xd8d8d8,
	0xd8d8d8,
	0xd9d9d9,
	0xd9d9d9,
	0xdadada,
	0xdbdbdb,
	0xdbdbdb,
	0xdcdcdc,
	0xdcdcdc,
	0xdddddd,
	0xdddddd,
	0xdedede,
	0xdedede,
	0xdfdfdf,
	0xdfdfdf,
	0xe0e0e0,
	0xe0e0e0,
	0xe1e1e1,
	0xe1e1e1,
	0xe2e2e2,
	0xe3e3e3,
	0xe3e3e3,
	0xe4e4e4,
	0xe4e4e4,
	0xe5e5e5,
	0xe5e5e5,
	0xe6e6e6,
	0xe6e6e6,
	0xe7e7e7,
	0xe7e7e7,
	0xe8e8e8,
	0xe8e8e8,
	0xe9e9e9,
	0xe9e9e9,
	0xeaeaea,
	0xeaeaea,
	0xebebeb,
	0xebebeb,
	0xececec,
	0xececec,
	0xededed,
	0xededed,
	0xeeeeee,
	0xeeeeee,
	0xefefef,
	0xefefef,
	0xf0f0f0,
	0xf0f0f0,
	0xf1f1f1,
	0xf1f1f1,
	0xf2f2f2,
	0xf2f2f2,
	0xf2f2f2,
	0xf3f3f3,
	0xf3f3f3,
	0xf4f4f4,
	0xf4f4f4,
	0xf5f5f5,
	0xf5f5f5,
	0xf6f6f6,
	0xf6f6f6,
	0xf7f7f7,
	0xf7f7f7,
	0xf8f8f8,
	0xf8f8f8,
	0xf9f9f9,
	0xf9f9f9,
	0xfafafa,
	0xfafafa,
	0xfafafa,
	0xfbfbfb,
	0xfbfbfb,
	0xfcfcfc,
	0xfcfcfc,
	0xfdfdfd,
	0xfdfdfd,
	0xfefefe,
	0xfefefe,
	0xffffff,
	0xffffff,
};

uint32_t default_post_lut_val[PPP_LUT_MAX] = {
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x10101,
	0x20202,
	0x20202,
	0x20202,
	0x20202,
	0x20202,
	0x20202,
	0x30303,
	0x30303,
	0x30303,
	0x30303,
	0x30303,
	0x40404,
	0x40404,
	0x40404,
	0x40404,
	0x40404,
	0x50505,
	0x50505,
	0x50505,
	0x50505,
	0x60606,
	0x60606,
	0x60606,
	0x70707,
	0x70707,
	0x70707,
	0x70707,
	0x80808,
	0x80808,
	0x80808,
	0x90909,
	0x90909,
	0xa0a0a,
	0xa0a0a,
	0xa0a0a,
	0xb0b0b,
	0xb0b0b,
	0xb0b0b,
	0xc0c0c,
	0xc0c0c,
	0xd0d0d,
	0xd0d0d,
	0xe0e0e,
	0xe0e0e,
	0xe0e0e,
	0xf0f0f,
	0xf0f0f,
	0x101010,
	0x101010,
	0x111111,
	0x111111,
	0x121212,
	0x121212,
	0x131313,
	0x131313,
	0x141414,
	0x151515,
	0x151515,
	0x161616,
	0x161616,
	0x171717,
	0x171717,
	0x181818,
	0x191919,
	0x191919,
	0x1a1a1a,
	0x1b1b1b,
	0x1b1b1b,
	0x1c1c1c,
	0x1c1c1c,
	0x1d1d1d,
	0x1e1e1e,
	0x1f1f1f,
	0x1f1f1f,
	0x202020,
	0x212121,
	0x212121,
	0x222222,
	0x232323,
	0x242424,
	0x242424,
	0x252525,
	0x262626,
	0x272727,
	0x272727,
	0x282828,
	0x292929,
	0x2a2a2a,
	0x2b2b2b,
	0x2c2c2c,
	0x2c2c2c,
	0x2d2d2d,
	0x2e2e2e,
	0x2f2f2f,
	0x303030,
	0x313131,
	0x323232,
	0x333333,
	0x333333,
	0x343434,
	0x353535,
	0x363636,
	0x373737,
	0x383838,
	0x393939,
	0x3a3a3a,
	0x3b3b3b,
	0x3c3c3c,
	0x3d3d3d,
	0x3e3e3e,
	0x3f3f3f,
	0x404040,
	0x414141,
	0x424242,
	0x434343,
	0x444444,
	0x464646,
	0x474747,
	0x484848,
	0x494949,
	0x4a4a4a,
	0x4b4b4b,
	0x4c4c4c,
	0x4d4d4d,
	0x4f4f4f,
	0x505050,
	0x515151,
	0x525252,
	0x535353,
	0x545454,
	0x565656,
	0x575757,
	0x585858,
	0x595959,
	0x5b5b5b,
	0x5c5c5c,
	0x5d5d5d,
	0x5e5e5e,
	0x606060,
	0x616161,
	0x626262,
	0x646464,
	0x656565,
	0x666666,
	0x686868,
	0x696969,
	0x6a6a6a,
	0x6c6c6c,
	0x6d6d6d,
	0x6f6f6f,
	0x707070,
	0x717171,
	0x737373,
	0x747474,
	0x767676,
	0x777777,
	0x797979,
	0x7a7a7a,
	0x7c7c7c,
	0x7d7d7d,
	0x7f7f7f,
	0x808080,
	0x828282,
	0x838383,
	0x858585,
	0x868686,
	0x888888,
	0x898989,
	0x8b8b8b,
	0x8d8d8d,
	0x8e8e8e,
	0x909090,
	0x919191,
	0x939393,
	0x959595,
	0x969696,
	0x989898,
	0x9a9a9a,
	0x9b9b9b,
	0x9d9d9d,
	0x9f9f9f,
	0xa1a1a1,
	0xa2a2a2,
	0xa4a4a4,
	0xa6a6a6,
	0xa7a7a7,
	0xa9a9a9,
	0xababab,
	0xadadad,
	0xafafaf,
	0xb0b0b0,
	0xb2b2b2,
	0xb4b4b4,
	0xb6b6b6,
	0xb8b8b8,
	0xbababa,
	0xbbbbbb,
	0xbdbdbd,
	0xbfbfbf,
	0xc1c1c1,
	0xc3c3c3,
	0xc5c5c5,
	0xc7c7c7,
	0xc9c9c9,
	0xcbcbcb,
	0xcdcdcd,
	0xcfcfcf,
	0xd1d1d1,
	0xd3d3d3,
	0xd5d5d5,
	0xd7d7d7,
	0xd9d9d9,
	0xdbdbdb,
	0xdddddd,
	0xdfdfdf,
	0xe1e1e1,
	0xe3e3e3,
	0xe5e5e5,
	0xe7e7e7,
	0xe9e9e9,
	0xebebeb,
	0xeeeeee,
	0xf0f0f0,
	0xf2f2f2,
	0xf4f4f4,
	0xf6f6f6,
	0xf8f8f8,
	0xfbfbfb,
	0xfdfdfd,
	0xffffff,
};

struct ppp_csc_table rgb2yuv = {
	.fwd_matrix = {
		0x83,
		0x102,
		0x32,
		0xffb5,
		0xff6c,
		0xe1,
		0xe1,
		0xff45,
		0xffdc,
	},
	.rev_matrix = {
		0x254,
		0x0,
		0x331,
		0x254,
		0xff38,
		0xfe61,
		0x254,
		0x409,
		0x0,
	},
	.bv = {
		0x10,
		0x80,
		0x80,
	},
	.lv = {
		0x10,
		0xeb,
		0x10,
		0xf0,
	},
};

struct ppp_csc_table default_table2 = {
	.fwd_matrix = {
		0x5d,
		0x13a,
		0x20,
		0xffcd,
		0xff54,
		0xe1,
		0xe1,
		0xff35,
	},
	.rev_matrix = {
		0x254,
		0x0,
		0x396,
		0x254,
		0xff94,
		0xfef0,
		0x254,
		0x43a,
		0x0,
	},
	.bv = {
		0x10,
		0x80,
		0x80,
	},
	.lv = {
		0x10,
		0xeb,
		0x10,
		0xf0,
	},
};

const struct ppp_table upscale_table[PPP_UPSCALE_MAX] = {
	{ 0x5fffc, 0x0 },
	{ 0x50200, 0x7fc00000 },
	{ 0x5fffc, 0xff80000d },
	{ 0x50204, 0x7ec003f9 },
	{ 0x5fffc, 0xfec0001c },
	{ 0x50208, 0x7d4003f3 },
	{ 0x5fffc, 0xfe40002b },
	{ 0x5020c, 0x7b8003ed },
	{ 0x5fffc, 0xfd80003c },
	{ 0x50210, 0x794003e8 },
	{ 0x5fffc, 0xfcc0004d },
	{ 0x50214, 0x76c003e4 },
	{ 0x5fffc, 0xfc40005f },
	{ 0x50218, 0x73c003e0 },
	{ 0x5fffc, 0xfb800071 },
	{ 0x5021c, 0x708003de },
	{ 0x5fffc, 0xfac00085 },
	{ 0x50220, 0x6d0003db },
	{ 0x5fffc, 0xfa000098 },
	{ 0x50224, 0x698003d9 },
	{ 0x5fffc, 0xf98000ac },
	{ 0x50228, 0x654003d8 },
	{ 0x5fffc, 0xf8c000c1 },
	{ 0x5022c, 0x610003d7 },
	{ 0x5fffc, 0xf84000d5 },
	{ 0x50230, 0x5c8003d7 },
	{ 0x5fffc, 0xf7c000e9 },
	{ 0x50234, 0x580003d7 },
	{ 0x5fffc, 0xf74000fd },
	{ 0x50238, 0x534003d8 },
	{ 0x5fffc, 0xf6c00112 },
	{ 0x5023c, 0x4e8003d8 },
	{ 0x5fffc, 0xf6800126 },
	{ 0x50240, 0x494003da },
	{ 0x5fffc, 0xf600013a },
	{ 0x50244, 0x448003db },
	{ 0x5fffc, 0xf600014d },
	{ 0x50248, 0x3f4003dd },
	{ 0x5fffc, 0xf5c00160 },
	{ 0x5024c, 0x3a4003df },
	{ 0x5fffc, 0xf5c00172 },
	{ 0x50250, 0x354003e1 },
	{ 0x5fffc, 0xf5c00184 },
	{ 0x50254, 0x304003e3 },
	{ 0x5fffc, 0xf6000195 },
	{ 0x50258, 0x2b0003e6 },
	{ 0x5fffc, 0xf64001a6 },
	{ 0x5025c, 0x260003e8 },
	{ 0x5fffc, 0xf6c001b4 },
	{ 0x50260, 0x214003eb },
	{ 0x5fffc, 0xf78001c2 },
	{ 0x50264, 0x1c4003ee },
	{ 0x5fffc, 0xf80001cf },
	{ 0x50268, 0x17c003f1 },
	{ 0x5fffc, 0xf90001db },
	{ 0x5026c, 0x134003f3 },
	{ 0x5fffc, 0xfa0001e5 },
	{ 0x50270, 0xf0003f6 },
	{ 0x5fffc, 0xfb4001ee },
	{ 0x50274, 0xac003f9 },
	{ 0x5fffc, 0xfcc001f5 },
	{ 0x50278, 0x70003fb },
	{ 0x5fffc, 0xfe4001fb },
	{ 0x5027c, 0x34003fe },
};

const struct ppp_table mdp_gaussian_blur_table[PPP_BLUR_SCALE_MAX] = {
	/* max variance */
	{ 0x5fffc, 0x20000080 },
	{ 0x50280, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50284, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50288, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5028c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50290, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50294, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50298, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5029c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502a0, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502a4, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502a8, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502ac, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502b0, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502b4, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502b8, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502bc, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502c0, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502c4, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502c8, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502cc, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502d0, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502d4, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502d8, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502dc, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502e0, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502e4, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502e8, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502ec, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502f0, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502f4, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502f8, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x502fc, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50300, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50304, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50308, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5030c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50310, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50314, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50318, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5031c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50320, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50324, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50328, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5032c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50330, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50334, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50338, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5033c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50340, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50344, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50348, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5034c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50350, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50354, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50358, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5035c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50360, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50364, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50368, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5036c, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50370, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50374, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x50378, 0x20000080 },
	{ 0x5fffc, 0x20000080 },
	{ 0x5037c, 0x20000080 },
};

const struct ppp_table downscale_x_table_pt2topt4[] = {
	{ 0x5fffc, 0x740008c },
	{ 0x50280, 0x33800088 },
	{ 0x5fffc, 0x800008e },
	{ 0x50284, 0x33400084 },
	{ 0x5fffc, 0x8400092 },
	{ 0x50288, 0x33000080 },
	{ 0x5fffc, 0x9000094 },
	{ 0x5028c, 0x3300007b },
	{ 0x5fffc, 0x9c00098 },
	{ 0x50290, 0x32400077 },
	{ 0x5fffc, 0xa40009b },
	{ 0x50294, 0x32000073 },
	{ 0x5fffc, 0xb00009d },
	{ 0x50298, 0x31c0006f },
	{ 0x5fffc, 0xbc000a0 },
	{ 0x5029c, 0x3140006b },
	{ 0x5fffc, 0xc8000a2 },
	{ 0x502a0, 0x31000067 },
	{ 0x5fffc, 0xd8000a5 },
	{ 0x502a4, 0x30800062 },
	{ 0x5fffc, 0xe4000a8 },
	{ 0x502a8, 0x2fc0005f },
	{ 0x5fffc, 0xec000aa },
	{ 0x502ac, 0x2fc0005b },
	{ 0x5fffc, 0xf8000ad },
	{ 0x502b0, 0x2f400057 },
	{ 0x5fffc, 0x108000b0 },
	{ 0x502b4, 0x2e400054 },
	{ 0x5fffc, 0x114000b2 },
	{ 0x502b8, 0x2e000050 },
	{ 0x5fffc, 0x124000b4 },
	{ 0x502bc, 0x2d80004c },
	{ 0x5fffc, 0x130000b6 },
	{ 0x502c0, 0x2d000049 },
	{ 0x5fffc, 0x140000b8 },
	{ 0x502c4, 0x2c800045 },
	{ 0x5fffc, 0x150000b9 },
	{ 0x502c8, 0x2c000042 },
	{ 0x5fffc, 0x15c000bd },
	{ 0x502cc, 0x2b40003e },
	{ 0x5fffc, 0x16c000bf },
	{ 0x502d0, 0x2a80003b },
	{ 0x5fffc, 0x17c000bf },
	{ 0x502d4, 0x2a000039 },
	{ 0x5fffc, 0x188000c2 },
	{ 0x502d8, 0x29400036 },
	{ 0x5fffc, 0x19c000c4 },
	{ 0x502dc, 0x28800032 },
	{ 0x5fffc, 0x1ac000c5 },
	{ 0x502e0, 0x2800002f },
	{ 0x5fffc, 0x1bc000c7 },
	{ 0x502e4, 0x2740002c },
	{ 0x5fffc, 0x1cc000c8 },
	{ 0x502e8, 0x26c00029 },
	{ 0x5fffc, 0x1dc000c9 },
	{ 0x502ec, 0x26000027 },
	{ 0x5fffc, 0x1ec000cc },
	{ 0x502f0, 0x25000024 },
	{ 0x5fffc, 0x200000cc },
	{ 0x502f4, 0x24800021 },
	{ 0x5fffc, 0x210000cd },
	{ 0x502f8, 0x23800020 },
	{ 0x5fffc, 0x220000ce },
	{ 0x502fc, 0x2300001d },
};

static const struct ppp_table downscale_x_table_pt4topt6[] = {
	{ 0x5fffc, 0x740008c },
	{ 0x50280, 0x33800088 },
	{ 0x5fffc, 0x800008e },
	{ 0x50284, 0x33400084 },
	{ 0x5fffc, 0x8400092 },
	{ 0x50288, 0x33000080 },
	{ 0x5fffc, 0x9000094 },
	{ 0x5028c, 0x3300007b },
	{ 0x5fffc, 0x9c00098 },
	{ 0x50290, 0x32400077 },
	{ 0x5fffc, 0xa40009b },
	{ 0x50294, 0x32000073 },
	{ 0x5fffc, 0xb00009d },
	{ 0x50298, 0x31c0006f },
	{ 0x5fffc, 0xbc000a0 },
	{ 0x5029c, 0x3140006b },
	{ 0x5fffc, 0xc8000a2 },
	{ 0x502a0, 0x31000067 },
	{ 0x5fffc, 0xd8000a5 },
	{ 0x502a4, 0x30800062 },
	{ 0x5fffc, 0xe4000a8 },
	{ 0x502a8, 0x2fc0005f },
	{ 0x5fffc, 0xec000aa },
	{ 0x502ac, 0x2fc0005b },
	{ 0x5fffc, 0xf8000ad },
	{ 0x502b0, 0x2f400057 },
	{ 0x5fffc, 0x108000b0 },
	{ 0x502b4, 0x2e400054 },
	{ 0x5fffc, 0x114000b2 },
	{ 0x502b8, 0x2e000050 },
	{ 0x5fffc, 0x124000b4 },
	{ 0x502bc, 0x2d80004c },
	{ 0x5fffc, 0x130000b6 },
	{ 0x502c0, 0x2d000049 },
	{ 0x5fffc, 0x140000b8 },
	{ 0x502c4, 0x2c800045 },
	{ 0x5fffc, 0x150000b9 },
	{ 0x502c8, 0x2c000042 },
	{ 0x5fffc, 0x15c000bd },
	{ 0x502cc, 0x2b40003e },
	{ 0x5fffc, 0x16c000bf },
	{ 0x502d0, 0x2a80003b },
	{ 0x5fffc, 0x17c000bf },
	{ 0x502d4, 0x2a000039 },
	{ 0x5fffc, 0x188000c2 },
	{ 0x502d8, 0x29400036 },
	{ 0x5fffc, 0x19c000c4 },
	{ 0x502dc, 0x28800032 },
	{ 0x5fffc, 0x1ac000c5 },
	{ 0x502e0, 0x2800002f },
	{ 0x5fffc, 0x1bc000c7 },
	{ 0x502e4, 0x2740002c },
	{ 0x5fffc, 0x1cc000c8 },
	{ 0x502e8, 0x26c00029 },
	{ 0x5fffc, 0x1dc000c9 },
	{ 0x502ec, 0x26000027 },
	{ 0x5fffc, 0x1ec000cc },
	{ 0x502f0, 0x25000024 },
	{ 0x5fffc, 0x200000cc },
	{ 0x502f4, 0x24800021 },
	{ 0x5fffc, 0x210000cd },
	{ 0x502f8, 0x23800020 },
	{ 0x5fffc, 0x220000ce },
	{ 0x502fc, 0x2300001d },
};

static const struct ppp_table downscale_x_table_pt6topt8[] = {
	{ 0x5fffc, 0xfe000070 },
	{ 0x50280, 0x4bc00068 },
	{ 0x5fffc, 0xfe000078 },
	{ 0x50284, 0x4bc00060 },
	{ 0x5fffc, 0xfe000080 },
	{ 0x50288, 0x4b800059 },
	{ 0x5fffc, 0xfe000089 },
	{ 0x5028c, 0x4b000052 },
	{ 0x5fffc, 0xfe400091 },
	{ 0x50290, 0x4a80004b },
	{ 0x5fffc, 0xfe40009a },
	{ 0x50294, 0x4a000044 },
	{ 0x5fffc, 0xfe8000a3 },
	{ 0x50298, 0x4940003d },
	{ 0x5fffc, 0xfec000ac },
	{ 0x5029c, 0x48400037 },
	{ 0x5fffc, 0xff0000b4 },
	{ 0x502a0, 0x47800031 },
	{ 0x5fffc, 0xff8000bd },
	{ 0x502a4, 0x4640002b },
	{ 0x5fffc, 0xc5 },
	{ 0x502a8, 0x45000026 },
	{ 0x5fffc, 0x8000ce },
	{ 0x502ac, 0x43800021 },
	{ 0x5fffc, 0x10000d6 },
	{ 0x502b0, 0x4240001c },
	{ 0x5fffc, 0x18000df },
	{ 0x502b4, 0x40800018 },
	{ 0x5fffc, 0x24000e6 },
	{ 0x502b8, 0x3f000014 },
	{ 0x5fffc, 0x30000ee },
	{ 0x502bc, 0x3d400010 },
	{ 0x5fffc, 0x40000f5 },
	{ 0x502c0, 0x3b80000c },
	{ 0x5fffc, 0x50000fc },
	{ 0x502c4, 0x39800009 },
	{ 0x5fffc, 0x6000102 },
	{ 0x502c8, 0x37c00006 },
	{ 0x5fffc, 0x7000109 },
	{ 0x502cc, 0x35800004 },
	{ 0x5fffc, 0x840010e },
	{ 0x502d0, 0x33800002 },
	{ 0x5fffc, 0x9800114 },
	{ 0x502d4, 0x31400000 },
	{ 0x5fffc, 0xac00119 },
	{ 0x502d8, 0x2f4003fe },
	{ 0x5fffc, 0xc40011e },
	{ 0x502dc, 0x2d0003fc },
	{ 0x5fffc, 0xdc00121 },
	{ 0x502e0, 0x2b0003fb },
	{ 0x5fffc, 0xf400125 },
	{ 0x502e4, 0x28c003fa },
	{ 0x5fffc, 0x11000128 },
	{ 0x502e8, 0x268003f9 },
	{ 0x5fffc, 0x12c0012a },
	{ 0x502ec, 0x244003f9 },
	{ 0x5fffc, 0x1480012c },
	{ 0x502f0, 0x224003f8 },
	{ 0x5fffc, 0x1640012e },
	{ 0x502f4, 0x200003f8 },
	{ 0x5fffc, 0x1800012f },
	{ 0x502f8, 0x1e0003f8 },
	{ 0x5fffc, 0x1a00012f },
	{ 0x502fc, 0x1c0003f8 },
};

static const struct ppp_table downscale_x_table_pt8topt1[] = {
	{ 0x5fffc, 0x0 },
	{ 0x50280, 0x7fc00000 },
	{ 0x5fffc, 0xff80000d },
	{ 0x50284, 0x7ec003f9 },
	{ 0x5fffc, 0xfec0001c },
	{ 0x50288, 0x7d4003f3 },
	{ 0x5fffc, 0xfe40002b },
	{ 0x5028c, 0x7b8003ed },
	{ 0x5fffc, 0xfd80003c },
	{ 0x50290, 0x794003e8 },
	{ 0x5fffc, 0xfcc0004d },
	{ 0x50294, 0x76c003e4 },
	{ 0x5fffc, 0xfc40005f },
	{ 0x50298, 0x73c003e0 },
	{ 0x5fffc, 0xfb800071 },
	{ 0x5029c, 0x708003de },
	{ 0x5fffc, 0xfac00085 },
	{ 0x502a0, 0x6d0003db },
	{ 0x5fffc, 0xfa000098 },
	{ 0x502a4, 0x698003d9 },
	{ 0x5fffc, 0xf98000ac },
	{ 0x502a8, 0x654003d8 },
	{ 0x5fffc, 0xf8c000c1 },
	{ 0x502ac, 0x610003d7 },
	{ 0x5fffc, 0xf84000d5 },
	{ 0x502b0, 0x5c8003d7 },
	{ 0x5fffc, 0xf7c000e9 },
	{ 0x502b4, 0x580003d7 },
	{ 0x5fffc, 0xf74000fd },
	{ 0x502b8, 0x534003d8 },
	{ 0x5fffc, 0xf6c00112 },
	{ 0x502bc, 0x4e8003d8 },
	{ 0x5fffc, 0xf6800126 },
	{ 0x502c0, 0x494003da },
	{ 0x5fffc, 0xf600013a },
	{ 0x502c4, 0x448003db },
	{ 0x5fffc, 0xf600014d },
	{ 0x502c8, 0x3f4003dd },
	{ 0x5fffc, 0xf5c00160 },
	{ 0x502cc, 0x3a4003df },
	{ 0x5fffc, 0xf5c00172 },
	{ 0x502d0, 0x354003e1 },
	{ 0x5fffc, 0xf5c00184 },
	{ 0x502d4, 0x304003e3 },
	{ 0x5fffc, 0xf6000195 },
	{ 0x502d8, 0x2b0003e6 },
	{ 0x5fffc, 0xf64001a6 },
	{ 0x502dc, 0x260003e8 },
	{ 0x5fffc, 0xf6c001b4 },
	{ 0x502e0, 0x214003eb },
	{ 0x5fffc, 0xf78001c2 },
	{ 0x502e4, 0x1c4003ee },
	{ 0x5fffc, 0xf80001cf },
	{ 0x502e8, 0x17c003f1 },
	{ 0x5fffc, 0xf90001db },
	{ 0x502ec, 0x134003f3 },
	{ 0x5fffc, 0xfa0001e5 },
	{ 0x502f0, 0xf0003f6 },
	{ 0x5fffc, 0xfb4001ee },
	{ 0x502f4, 0xac003f9 },
	{ 0x5fffc, 0xfcc001f5 },
	{ 0x502f8, 0x70003fb },
	{ 0x5fffc, 0xfe4001fb },
	{ 0x502fc, 0x34003fe },
};

static const struct ppp_table *downscale_x_table[PPP_DOWNSCALE_MAX] = {
	[PPP_DOWNSCALE_PT2TOPT4] = downscale_x_table_pt2topt4,
	[PPP_DOWNSCALE_PT4TOPT6] = downscale_x_table_pt4topt6,
	[PPP_DOWNSCALE_PT6TOPT8] = downscale_x_table_pt6topt8,
	[PPP_DOWNSCALE_PT8TOPT1] = downscale_x_table_pt8topt1,
};

static const struct ppp_table downscale_y_table_pt2topt4[] = {
	{ 0x5fffc, 0x740008c },
	{ 0x50300, 0x33800088 },
	{ 0x5fffc, 0x800008e },
	{ 0x50304, 0x33400084 },
	{ 0x5fffc, 0x8400092 },
	{ 0x50308, 0x33000080 },
	{ 0x5fffc, 0x9000094 },
	{ 0x5030c, 0x3300007b },
	{ 0x5fffc, 0x9c00098 },
	{ 0x50310, 0x32400077 },
	{ 0x5fffc, 0xa40009b },
	{ 0x50314, 0x32000073 },
	{ 0x5fffc, 0xb00009d },
	{ 0x50318, 0x31c0006f },
	{ 0x5fffc, 0xbc000a0 },
	{ 0x5031c, 0x3140006b },
	{ 0x5fffc, 0xc8000a2 },
	{ 0x50320, 0x31000067 },
	{ 0x5fffc, 0xd8000a5 },
	{ 0x50324, 0x30800062 },
	{ 0x5fffc, 0xe4000a8 },
	{ 0x50328, 0x2fc0005f },
	{ 0x5fffc, 0xec000aa },
	{ 0x5032c, 0x2fc0005b },
	{ 0x5fffc, 0xf8000ad },
	{ 0x50330, 0x2f400057 },
	{ 0x5fffc, 0x108000b0 },
	{ 0x50334, 0x2e400054 },
	{ 0x5fffc, 0x114000b2 },
	{ 0x50338, 0x2e000050 },
	{ 0x5fffc, 0x124000b4 },
	{ 0x5033c, 0x2d80004c },
	{ 0x5fffc, 0x130000b6 },
	{ 0x50340, 0x2d000049 },
	{ 0x5fffc, 0x140000b8 },
	{ 0x50344, 0x2c800045 },
	{ 0x5fffc, 0x150000b9 },
	{ 0x50348, 0x2c000042 },
	{ 0x5fffc, 0x15c000bd },
	{ 0x5034c, 0x2b40003e },
	{ 0x5fffc, 0x16c000bf },
	{ 0x50350, 0x2a80003b },
	{ 0x5fffc, 0x17c000bf },
	{ 0x50354, 0x2a000039 },
	{ 0x5fffc, 0x188000c2 },
	{ 0x50358, 0x29400036 },
	{ 0x5fffc, 0x19c000c4 },
	{ 0x5035c, 0x28800032 },
	{ 0x5fffc, 0x1ac000c5 },
	{ 0x50360, 0x2800002f },
	{ 0x5fffc, 0x1bc000c7 },
	{ 0x50364, 0x2740002c },
	{ 0x5fffc, 0x1cc000c8 },
	{ 0x50368, 0x26c00029 },
	{ 0x5fffc, 0x1dc000c9 },
	{ 0x5036c, 0x26000027 },
	{ 0x5fffc, 0x1ec000cc },
	{ 0x50370, 0x25000024 },
	{ 0x5fffc, 0x200000cc },
	{ 0x50374, 0x24800021 },
	{ 0x5fffc, 0x210000cd },
	{ 0x50378, 0x23800020 },
	{ 0x5fffc, 0x220000ce },
	{ 0x5037c, 0x2300001d },
};

static const struct ppp_table downscale_y_table_pt4topt6[] = {
	{ 0x5fffc, 0x740008c },
	{ 0x50300, 0x33800088 },
	{ 0x5fffc, 0x800008e },
	{ 0x50304, 0x33400084 },
	{ 0x5fffc, 0x8400092 },
	{ 0x50308, 0x33000080 },
	{ 0x5fffc, 0x9000094 },
	{ 0x5030c, 0x3300007b },
	{ 0x5fffc, 0x9c00098 },
	{ 0x50310, 0x32400077 },
	{ 0x5fffc, 0xa40009b },
	{ 0x50314, 0x32000073 },
	{ 0x5fffc, 0xb00009d },
	{ 0x50318, 0x31c0006f },
	{ 0x5fffc, 0xbc000a0 },
	{ 0x5031c, 0x3140006b },
	{ 0x5fffc, 0xc8000a2 },
	{ 0x50320, 0x31000067 },
	{ 0x5fffc, 0xd8000a5 },
	{ 0x50324, 0x30800062 },
	{ 0x5fffc, 0xe4000a8 },
	{ 0x50328, 0x2fc0005f },
	{ 0x5fffc, 0xec000aa },
	{ 0x5032c, 0x2fc0005b },
	{ 0x5fffc, 0xf8000ad },
	{ 0x50330, 0x2f400057 },
	{ 0x5fffc, 0x108000b0 },
	{ 0x50334, 0x2e400054 },
	{ 0x5fffc, 0x114000b2 },
	{ 0x50338, 0x2e000050 },
	{ 0x5fffc, 0x124000b4 },
	{ 0x5033c, 0x2d80004c },
	{ 0x5fffc, 0x130000b6 },
	{ 0x50340, 0x2d000049 },
	{ 0x5fffc, 0x140000b8 },
	{ 0x50344, 0x2c800045 },
	{ 0x5fffc, 0x150000b9 },
	{ 0x50348, 0x2c000042 },
	{ 0x5fffc, 0x15c000bd },
	{ 0x5034c, 0x2b40003e },
	{ 0x5fffc, 0x16c000bf },
	{ 0x50350, 0x2a80003b },
	{ 0x5fffc, 0x17c000bf },
	{ 0x50354, 0x2a000039 },
	{ 0x5fffc, 0x188000c2 },
	{ 0x50358, 0x29400036 },
	{ 0x5fffc, 0x19c000c4 },
	{ 0x5035c, 0x28800032 },
	{ 0x5fffc, 0x1ac000c5 },
	{ 0x50360, 0x2800002f },
	{ 0x5fffc, 0x1bc000c7 },
	{ 0x50364, 0x2740002c },
	{ 0x5fffc, 0x1cc000c8 },
	{ 0x50368, 0x26c00029 },
	{ 0x5fffc, 0x1dc000c9 },
	{ 0x5036c, 0x26000027 },
	{ 0x5fffc, 0x1ec000cc },
	{ 0x50370, 0x25000024 },
	{ 0x5fffc, 0x200000cc },
	{ 0x50374, 0x24800021 },
	{ 0x5fffc, 0x210000cd },
	{ 0x50378, 0x23800020 },
	{ 0x5fffc, 0x220000ce },
	{ 0x5037c, 0x2300001d },
};

static const struct ppp_table downscale_y_table_pt6topt8[] = {
	{ 0x5fffc, 0xfe000070 },
	{ 0x50300, 0x4bc00068 },
	{ 0x5fffc, 0xfe000078 },
	{ 0x50304, 0x4bc00060 },
	{ 0x5fffc, 0xfe000080 },
	{ 0x50308, 0x4b800059 },
	{ 0x5fffc, 0xfe000089 },
	{ 0x5030c, 0x4b000052 },
	{ 0x5fffc, 0xfe400091 },
	{ 0x50310, 0x4a80004b },
	{ 0x5fffc, 0xfe40009a },
	{ 0x50314, 0x4a000044 },
	{ 0x5fffc, 0xfe8000a3 },
	{ 0x50318, 0x4940003d },
	{ 0x5fffc, 0xfec000ac },
	{ 0x5031c, 0x48400037 },
	{ 0x5fffc, 0xff0000b4 },
	{ 0x50320, 0x47800031 },
	{ 0x5fffc, 0xff8000bd },
	{ 0x50324, 0x4640002b },
	{ 0x5fffc, 0xc5 },
	{ 0x50328, 0x45000026 },
	{ 0x5fffc, 0x8000ce },
	{ 0x5032c, 0x43800021 },
	{ 0x5fffc, 0x10000d6 },
	{ 0x50330, 0x4240001c },
	{ 0x5fffc, 0x18000df },
	{ 0x50334, 0x40800018 },
	{ 0x5fffc, 0x24000e6 },
	{ 0x50338, 0x3f000014 },
	{ 0x5fffc, 0x30000ee },
	{ 0x5033c, 0x3d400010 },
	{ 0x5fffc, 0x40000f5 },
	{ 0x50340, 0x3b80000c },
	{ 0x5fffc, 0x50000fc },
	{ 0x50344, 0x39800009 },
	{ 0x5fffc, 0x6000102 },
	{ 0x50348, 0x37c00006 },
	{ 0x5fffc, 0x7000109 },
	{ 0x5034c, 0x35800004 },
	{ 0x5fffc, 0x840010e },
	{ 0x50350, 0x33800002 },
	{ 0x5fffc, 0x9800114 },
	{ 0x50354, 0x31400000 },
	{ 0x5fffc, 0xac00119 },
	{ 0x50358, 0x2f4003fe },
	{ 0x5fffc, 0xc40011e },
	{ 0x5035c, 0x2d0003fc },
	{ 0x5fffc, 0xdc00121 },
	{ 0x50360, 0x2b0003fb },
	{ 0x5fffc, 0xf400125 },
	{ 0x50364, 0x28c003fa },
	{ 0x5fffc, 0x11000128 },
	{ 0x50368, 0x268003f9 },
	{ 0x5fffc, 0x12c0012a },
	{ 0x5036c, 0x244003f9 },
	{ 0x5fffc, 0x1480012c },
	{ 0x50370, 0x224003f8 },
	{ 0x5fffc, 0x1640012e },
	{ 0x50374, 0x200003f8 },
	{ 0x5fffc, 0x1800012f },
	{ 0x50378, 0x1e0003f8 },
	{ 0x5fffc, 0x1a00012f },
	{ 0x5037c, 0x1c0003f8 },
};

static const struct ppp_table downscale_y_table_pt8topt1[] = {
	{ 0x5fffc, 0x0 },
	{ 0x50300, 0x7fc00000 },
	{ 0x5fffc, 0xff80000d },
	{ 0x50304, 0x7ec003f9 },
	{ 0x5fffc, 0xfec0001c },
	{ 0x50308, 0x7d4003f3 },
	{ 0x5fffc, 0xfe40002b },
	{ 0x5030c, 0x7b8003ed },
	{ 0x5fffc, 0xfd80003c },
	{ 0x50310, 0x794003e8 },
	{ 0x5fffc, 0xfcc0004d },
	{ 0x50314, 0x76c003e4 },
	{ 0x5fffc, 0xfc40005f },
	{ 0x50318, 0x73c003e0 },
	{ 0x5fffc, 0xfb800071 },
	{ 0x5031c, 0x708003de },
	{ 0x5fffc, 0xfac00085 },
	{ 0x50320, 0x6d0003db },
	{ 0x5fffc, 0xfa000098 },
	{ 0x50324, 0x698003d9 },
	{ 0x5fffc, 0xf98000ac },
	{ 0x50328, 0x654003d8 },
	{ 0x5fffc, 0xf8c000c1 },
	{ 0x5032c, 0x610003d7 },
	{ 0x5fffc, 0xf84000d5 },
	{ 0x50330, 0x5c8003d7 },
	{ 0x5fffc, 0xf7c000e9 },
	{ 0x50334, 0x580003d7 },
	{ 0x5fffc, 0xf74000fd },
	{ 0x50338, 0x534003d8 },
	{ 0x5fffc, 0xf6c00112 },
	{ 0x5033c, 0x4e8003d8 },
	{ 0x5fffc, 0xf6800126 },
	{ 0x50340, 0x494003da },
	{ 0x5fffc, 0xf600013a },
	{ 0x50344, 0x448003db },
	{ 0x5fffc, 0xf600014d },
	{ 0x50348, 0x3f4003dd },
	{ 0x5fffc, 0xf5c00160 },
	{ 0x5034c, 0x3a4003df },
	{ 0x5fffc, 0xf5c00172 },
	{ 0x50350, 0x354003e1 },
	{ 0x5fffc, 0xf5c00184 },
	{ 0x50354, 0x304003e3 },
	{ 0x5fffc, 0xf6000195 },
	{ 0x50358, 0x2b0003e6 },
	{ 0x5fffc, 0xf64001a6 },
	{ 0x5035c, 0x260003e8 },
	{ 0x5fffc, 0xf6c001b4 },
	{ 0x50360, 0x214003eb },
	{ 0x5fffc, 0xf78001c2 },
	{ 0x50364, 0x1c4003ee },
	{ 0x5fffc, 0xf80001cf },
	{ 0x50368, 0x17c003f1 },
	{ 0x5fffc, 0xf90001db },
	{ 0x5036c, 0x134003f3 },
	{ 0x5fffc, 0xfa0001e5 },
	{ 0x50370, 0xf0003f6 },
	{ 0x5fffc, 0xfb4001ee },
	{ 0x50374, 0xac003f9 },
	{ 0x5fffc, 0xfcc001f5 },
	{ 0x50378, 0x70003fb },
	{ 0x5fffc, 0xfe4001fb },
	{ 0x5037c, 0x34003fe },
};

static const struct ppp_table *downscale_y_table[PPP_DOWNSCALE_MAX] = {
	[PPP_DOWNSCALE_PT2TOPT4] = downscale_y_table_pt2topt4,
	[PPP_DOWNSCALE_PT4TOPT6] = downscale_y_table_pt4topt6,
	[PPP_DOWNSCALE_PT6TOPT8] = downscale_y_table_pt6topt8,
	[PPP_DOWNSCALE_PT8TOPT1] = downscale_y_table_pt8topt1,
};

void ppp_load_table(const struct ppp_table *table, int len)
{
	int i;
	for (i = 0; i < len; i++)
		PPP_WRITEL(table[i].val, table[i].reg);
}

void ppp_load_up_lut(void)
{
	ppp_load_table(upscale_table,
		PPP_UPSCALE_MAX);
}

void ppp_load_gaussian_lut(void)
{
	ppp_load_table(mdp_gaussian_blur_table,
		PPP_BLUR_SCALE_MAX);
}

void ppp_load_x_scale_table(int idx)
{
	ppp_load_table(downscale_x_table[idx], 64);
}

void ppp_load_y_scale_table(int idx)
{
	ppp_load_table(downscale_y_table[idx], 64);
}

uint32_t ppp_bpp(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return bytes_per_pixel[type];
}

uint32_t ppp_src_config(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return src_cfg_lut[type];
}

uint32_t ppp_out_config(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return out_cfg_lut[type];
}

uint32_t ppp_pack_pattern(uint32_t type, uint32_t yuv2rgb)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	if (yuv2rgb)
		return swapped_pack_patt_lut[type];

	return pack_patt_lut[type];
}

uint32_t ppp_dst_op_reg(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return dst_op_reg[type];
}

uint32_t ppp_src_op_reg(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return src_op_reg[type];
}

bool ppp_per_p_alpha(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return per_pixel_alpha[type];
}

bool ppp_multi_plane(uint32_t type)
{
	if (MDP_IS_IMGTYPE_BAD(type))
		return 0;
	return multi_plane[type];
}

uint32_t *ppp_default_pre_lut(void)
{
	return default_pre_lut_val;
}

uint32_t *ppp_default_post_lut(void)
{
	return default_post_lut_val;
}

struct ppp_csc_table *ppp_csc_rgb2yuv(void)
{
	return &rgb2yuv;
}

struct ppp_csc_table *ppp_csc_table2(void)
{
	return &default_table2;
}
