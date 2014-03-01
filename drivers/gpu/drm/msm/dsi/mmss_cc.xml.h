#ifndef MMSS_CC_XML
#define MMSS_CC_XML

/* Autogenerated file, DO NOT EDIT manually!

This file was generated by the rules-ng-ng headergen tool in this git repository:
http://github.com/freedreno/envytools/
git clone https://github.com/freedreno/envytools.git

The rules-ng-ng source files this header was generated from are:
- /home/robclark/src/freedreno/envytools/rnndb/msm.xml                 (    647 bytes, from 2013-11-30 14:45:35)
- /home/robclark/src/freedreno/envytools/rnndb/freedreno_copyright.xml (   1453 bytes, from 2013-03-31 16:51:27)
- /home/robclark/src/freedreno/envytools/rnndb/mdp/mdp4.xml            (  17996 bytes, from 2013-12-01 19:10:31)
- /home/robclark/src/freedreno/envytools/rnndb/mdp/mdp_common.xml      (   1615 bytes, from 2013-11-30 15:00:52)
- /home/robclark/src/freedreno/envytools/rnndb/mdp/mdp5.xml            (  22517 bytes, from 2013-12-03 20:59:13)
- /home/robclark/src/freedreno/envytools/rnndb/dsi/dsi.xml             (  11712 bytes, from 2013-08-17 17:13:43)
- /home/robclark/src/freedreno/envytools/rnndb/dsi/sfpb.xml            (    344 bytes, from 2013-08-11 19:26:32)
- /home/robclark/src/freedreno/envytools/rnndb/dsi/mmss_cc.xml         (   1544 bytes, from 2013-08-16 19:17:05)
- /home/robclark/src/freedreno/envytools/rnndb/hdmi/qfprom.xml         (    600 bytes, from 2013-07-05 19:21:12)
- /home/robclark/src/freedreno/envytools/rnndb/hdmi/hdmi.xml           (  20932 bytes, from 2013-12-01 15:13:04)

Copyright (C) 2013 by the following authors:
- Rob Clark <robdclark@gmail.com> (robclark)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


enum mmss_cc_clk {
	CLK = 0,
	PCLK = 1,
};

#define REG_MMSS_CC_AHB						0x00000008

static inline uint32_t __offset_CLK(enum mmss_cc_clk idx)
{
	switch (idx) {
		case CLK: return 0x0000004c;
		case PCLK: return 0x00000130;
		default: return INVALID_IDX(idx);
	}
}
static inline uint32_t REG_MMSS_CC_CLK(enum mmss_cc_clk i0) { return 0x00000000 + __offset_CLK(i0); }

static inline uint32_t REG_MMSS_CC_CLK_CC(enum mmss_cc_clk i0) { return 0x00000000 + __offset_CLK(i0); }
#define MMSS_CC_CLK_CC_CLK_EN					0x00000001
#define MMSS_CC_CLK_CC_ROOT_EN					0x00000004
#define MMSS_CC_CLK_CC_MND_EN					0x00000020
#define MMSS_CC_CLK_CC_MND_MODE__MASK				0x000000c0
#define MMSS_CC_CLK_CC_MND_MODE__SHIFT				6
static inline uint32_t MMSS_CC_CLK_CC_MND_MODE(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_CC_MND_MODE__SHIFT) & MMSS_CC_CLK_CC_MND_MODE__MASK;
}
#define MMSS_CC_CLK_CC_PMXO_SEL__MASK				0x00000300
#define MMSS_CC_CLK_CC_PMXO_SEL__SHIFT				8
static inline uint32_t MMSS_CC_CLK_CC_PMXO_SEL(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_CC_PMXO_SEL__SHIFT) & MMSS_CC_CLK_CC_PMXO_SEL__MASK;
}

static inline uint32_t REG_MMSS_CC_CLK_MD(enum mmss_cc_clk i0) { return 0x00000004 + __offset_CLK(i0); }
#define MMSS_CC_CLK_MD_D__MASK					0x000000ff
#define MMSS_CC_CLK_MD_D__SHIFT					0
static inline uint32_t MMSS_CC_CLK_MD_D(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_MD_D__SHIFT) & MMSS_CC_CLK_MD_D__MASK;
}
#define MMSS_CC_CLK_MD_M__MASK					0x0000ff00
#define MMSS_CC_CLK_MD_M__SHIFT					8
static inline uint32_t MMSS_CC_CLK_MD_M(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_MD_M__SHIFT) & MMSS_CC_CLK_MD_M__MASK;
}

static inline uint32_t REG_MMSS_CC_CLK_NS(enum mmss_cc_clk i0) { return 0x00000008 + __offset_CLK(i0); }
#define MMSS_CC_CLK_NS_SRC__MASK				0x0000000f
#define MMSS_CC_CLK_NS_SRC__SHIFT				0
static inline uint32_t MMSS_CC_CLK_NS_SRC(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_NS_SRC__SHIFT) & MMSS_CC_CLK_NS_SRC__MASK;
}
#define MMSS_CC_CLK_NS_PRE_DIV_FUNC__MASK			0x00fff000
#define MMSS_CC_CLK_NS_PRE_DIV_FUNC__SHIFT			12
static inline uint32_t MMSS_CC_CLK_NS_PRE_DIV_FUNC(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_NS_PRE_DIV_FUNC__SHIFT) & MMSS_CC_CLK_NS_PRE_DIV_FUNC__MASK;
}
#define MMSS_CC_CLK_NS_VAL__MASK				0xff000000
#define MMSS_CC_CLK_NS_VAL__SHIFT				24
static inline uint32_t MMSS_CC_CLK_NS_VAL(uint32_t val)
{
	return ((val) << MMSS_CC_CLK_NS_VAL__SHIFT) & MMSS_CC_CLK_NS_VAL__MASK;
}


#endif /* MMSS_CC_XML */
