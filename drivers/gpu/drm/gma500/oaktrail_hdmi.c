/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Li Peng <peng.li@intel.com>
 */

#include <drm/drmP.h>
#include <drm/drm.h>
#include "psb_intel_drv.h"
#include "psb_intel_reg.h"
#include "psb_drv.h"

#define HDMI_READ(reg)		readl(hdmi_dev->regs + (reg))
#define HDMI_WRITE(reg, val)	writel(val, hdmi_dev->regs + (reg))

#define HDMI_HCR	0x1000
#define HCR_ENABLE_HDCP		(1 << 5)
#define HCR_ENABLE_AUDIO	(1 << 2)
#define HCR_ENABLE_PIXEL	(1 << 1)
#define HCR_ENABLE_TMDS		(1 << 0)

#define HDMI_HICR	0x1004
#define HDMI_HSR	0x1008
#define HDMI_HISR	0x100C
#define HDMI_DETECT_HDP		(1 << 0)

#define HDMI_VIDEO_REG	0x3000
#define HDMI_UNIT_EN		(1 << 7)
#define HDMI_MODE_OUTPUT	(1 << 0)
#define HDMI_HBLANK_A	0x3100

#define HDMI_AUDIO_CTRL	0x4000
#define HDMI_ENABLE_AUDIO	(1 << 0)

#define PCH_HTOTAL_B	0x3100
#define PCH_HBLANK_B	0x3104
#define PCH_HSYNC_B	0x3108
#define PCH_VTOTAL_B	0x310C
#define PCH_VBLANK_B	0x3110
#define PCH_VSYNC_B	0x3114
#define PCH_PIPEBSRC	0x311C

#define PCH_PIPEB_DSL	0x3800
#define PCH_PIPEB_SLC	0x3804
#define PCH_PIPEBCONF	0x3808
#define PCH_PIPEBSTAT	0x3824

#define CDVO_DFT	0x5000
#define CDVO_SLEWRATE	0x5004
#define CDVO_STRENGTH	0x5008
#define CDVO_RCOMP	0x500C

#define DPLL_CTRL       0x6000
#define DPLL_PDIV_SHIFT		16
#define DPLL_PDIV_MASK		(0xf << 16)
#define DPLL_PWRDN		(1 << 4)
#define DPLL_RESET		(1 << 3)
#define DPLL_FASTEN		(1 << 2)
#define DPLL_ENSTAT		(1 << 1)
#define DPLL_DITHEN		(1 << 0)

#define DPLL_DIV_CTRL   0x6004
#define DPLL_CLKF_MASK		0xffffffc0
#define DPLL_CLKR_MASK		(0x3f)

#define DPLL_CLK_ENABLE 0x6008
#define DPLL_EN_DISP		(1 << 31)
#define DPLL_SEL_HDMI		(1 << 8)
#define DPLL_EN_HDMI		(1 << 1)
#define DPLL_EN_VGA		(1 << 0)

#define DPLL_ADJUST     0x600C
#define DPLL_STATUS     0x6010
#define DPLL_UPDATE     0x6014
#define DPLL_DFT        0x6020

struct intel_range {
	int	min, max;
};

struct oaktrail_hdmi_limit {
	struct intel_range vco, np, nr, nf;
};

struct oaktrail_hdmi_clock {
	int np;
	int nr;
	int nf;
	int dot;
};

#define VCO_MIN		320000
#define VCO_MAX		1650000
#define	NP_MIN		1
#define	NP_MAX		15
#define	NR_MIN		1
#define	NR_MAX		64
#define NF_MIN		2
#define NF_MAX		4095

static const struct oaktrail_hdmi_limit oaktrail_hdmi_limit = {
	.vco = { .min = VCO_MIN,		.max = VCO_MAX },
	.np  = { .min = NP_MIN,			.max = NP_MAX  },
	.nr  = { .min = NR_MIN,			.max = NR_MAX  },
	.nf  = { .min = NF_MIN,			.max = NF_MAX  },
};

static void oaktrail_hdmi_audio_enable(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;

	HDMI_WRITE(HDMI_HCR, 0x67);
	HDMI_READ(HDMI_HCR);

	HDMI_WRITE(0x51a8, 0x10);
	HDMI_READ(0x51a8);

	HDMI_WRITE(HDMI_AUDIO_CTRL, 0x1);
	HDMI_READ(HDMI_AUDIO_CTRL);
}

static void oaktrail_hdmi_audio_disable(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;

	HDMI_WRITE(0x51a8, 0x0);
	HDMI_READ(0x51a8);

	HDMI_WRITE(HDMI_AUDIO_CTRL, 0x0);
	HDMI_READ(HDMI_AUDIO_CTRL);

	HDMI_WRITE(HDMI_HCR, 0x47);
	HDMI_READ(HDMI_HCR);
}

static void oaktrail_hdmi_dpms(struct drm_encoder *encoder, int mode)
{
	static int dpms_mode = -1;

	struct drm_device *dev = encoder->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;
	u32 temp;

	if (dpms_mode == mode)
		return;

	if (mode != DRM_MODE_DPMS_ON)
		temp = 0x0;
	else
		temp = 0x99;

	dpms_mode = mode;
	HDMI_WRITE(HDMI_VIDEO_REG, temp);
}

static int oaktrail_hdmi_mode_valid(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	if (mode->clock > 165000)
		return MODE_CLOCK_HIGH;
	if (mode->clock < 20000)
		return MODE_CLOCK_LOW;

	if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		return MODE_NO_DBLESCAN;

	return MODE_OK;
}

static bool oaktrail_hdmi_mode_fixup(struct drm_encoder *encoder,
				 const struct drm_display_mode *mode,
				 struct drm_display_mode *adjusted_mode)
{
	return true;
}

static enum drm_connector_status
oaktrail_hdmi_detect(struct drm_connector *connector, bool force)
{
	enum drm_connector_status status;
	struct drm_device *dev = connector->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;
	u32 temp;

	temp = HDMI_READ(HDMI_HSR);
	DRM_DEBUG_KMS("HDMI_HSR %x\n", temp);

	if ((temp & HDMI_DETECT_HDP) != 0)
		status = connector_status_connected;
	else
		status = connector_status_disconnected;

	return status;
}

static const unsigned char raw_edid[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x10, 0xac, 0x2f, 0xa0,
	0x53, 0x55, 0x33, 0x30, 0x16, 0x13, 0x01, 0x03, 0x0e, 0x3a, 0x24, 0x78,
	0xea, 0xe9, 0xf5, 0xac, 0x51, 0x30, 0xb4, 0x25, 0x11, 0x50, 0x54, 0xa5,
	0x4b, 0x00, 0x81, 0x80, 0xa9, 0x40, 0x71, 0x4f, 0xb3, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x28, 0x3c, 0x80, 0xa0, 0x70, 0xb0,
	0x23, 0x40, 0x30, 0x20, 0x36, 0x00, 0x46, 0x6c, 0x21, 0x00, 0x00, 0x1a,
	0x00, 0x00, 0x00, 0xff, 0x00, 0x47, 0x4e, 0x37, 0x32, 0x31, 0x39, 0x35,
	0x52, 0x30, 0x33, 0x55, 0x53, 0x0a, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x44,
	0x45, 0x4c, 0x4c, 0x20, 0x32, 0x37, 0x30, 0x39, 0x57, 0x0a, 0x20, 0x20,
	0x00, 0x00, 0x00, 0xfd, 0x00, 0x38, 0x4c, 0x1e, 0x53, 0x11, 0x00, 0x0a,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x8d
};

static int oaktrail_hdmi_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct i2c_adapter *i2c_adap;
	struct edid *edid;
	struct drm_display_mode *mode, *t;
	int i = 0, ret = 0;

	i2c_adap = i2c_get_adapter(3);
	if (i2c_adap == NULL) {
		DRM_ERROR("No ddc adapter available!\n");
		edid = (struct edid *)raw_edid;
	} else {
		edid = (struct edid *)raw_edid;
		/* FIXME ? edid = drm_get_edid(connector, i2c_adap); */
	}

	if (edid) {
		drm_mode_connector_update_edid_property(connector, edid);
		ret = drm_add_edid_modes(connector, edid);
	}

	/*
	 * prune modes that require frame buffer bigger than stolen mem
	 */
	list_for_each_entry_safe(mode, t, &connector->probed_modes, head) {
		if ((mode->hdisplay * mode->vdisplay * 4) >= dev_priv->vram_stolen_size) {
			i++;
			drm_mode_remove(connector, mode);
		}
	}
	return ret - i;
}

static void oaktrail_hdmi_mode_set(struct drm_encoder *encoder,
			       struct drm_display_mode *mode,
			       struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = encoder->dev;

	oaktrail_hdmi_audio_enable(dev);
	return;
}

static void oaktrail_hdmi_destroy(struct drm_connector *connector)
{
	return;
}

static const struct drm_encoder_helper_funcs oaktrail_hdmi_helper_funcs = {
	.dpms = oaktrail_hdmi_dpms,
	.mode_fixup = oaktrail_hdmi_mode_fixup,
	.prepare = psb_intel_encoder_prepare,
	.mode_set = oaktrail_hdmi_mode_set,
	.commit = psb_intel_encoder_commit,
};

static const struct drm_connector_helper_funcs
					oaktrail_hdmi_connector_helper_funcs = {
	.get_modes = oaktrail_hdmi_get_modes,
	.mode_valid = oaktrail_hdmi_mode_valid,
	.best_encoder = psb_intel_best_encoder,
};

static const struct drm_connector_funcs oaktrail_hdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = oaktrail_hdmi_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = oaktrail_hdmi_destroy,
};

static void oaktrail_hdmi_enc_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs oaktrail_hdmi_enc_funcs = {
	.destroy = oaktrail_hdmi_enc_destroy,
};

void oaktrail_hdmi_init(struct drm_device *dev,
					struct psb_intel_mode_device *mode_dev)
{
	struct psb_intel_encoder *psb_intel_encoder;
	struct psb_intel_connector *psb_intel_connector;
	struct drm_connector *connector;
	struct drm_encoder *encoder;

	psb_intel_encoder = kzalloc(sizeof(struct psb_intel_encoder), GFP_KERNEL);
	if (!psb_intel_encoder)
		return;

	psb_intel_connector = kzalloc(sizeof(struct psb_intel_connector), GFP_KERNEL);
	if (!psb_intel_connector)
		goto failed_connector;

	connector = &psb_intel_connector->base;
	encoder = &psb_intel_encoder->base;
	drm_connector_init(dev, connector,
			   &oaktrail_hdmi_connector_funcs,
			   DRM_MODE_CONNECTOR_DVID);

	drm_encoder_init(dev, encoder,
			 &oaktrail_hdmi_enc_funcs,
			 DRM_MODE_ENCODER_TMDS);

	psb_intel_connector_attach_encoder(psb_intel_connector,
					   psb_intel_encoder);

	psb_intel_encoder->type = INTEL_OUTPUT_HDMI;
	drm_encoder_helper_add(encoder, &oaktrail_hdmi_helper_funcs);
	drm_connector_helper_add(connector, &oaktrail_hdmi_connector_helper_funcs);

	connector->display_info.subpixel_order = SubPixelHorizontalRGB;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;
	drm_sysfs_connector_add(connector);

	return;

failed_connector:
	kfree(psb_intel_encoder);
}

static DEFINE_PCI_DEVICE_TABLE(hdmi_ids) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x080d) },
	{ 0 }
};

void oaktrail_hdmi_setup(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct pci_dev *pdev;
	struct oaktrail_hdmi_dev *hdmi_dev;
	int ret;

	pdev = pci_get_device(PCI_VENDOR_ID_INTEL, 0x080d, NULL);
	if (!pdev)
		return;

	hdmi_dev = kzalloc(sizeof(struct oaktrail_hdmi_dev), GFP_KERNEL);
	if (!hdmi_dev) {
		dev_err(dev->dev, "failed to allocate memory\n");
		goto out;
	}


	ret = pci_enable_device(pdev);
	if (ret) {
		dev_err(dev->dev, "failed to enable hdmi controller\n");
		goto free;
	}

	hdmi_dev->mmio = pci_resource_start(pdev, 0);
	hdmi_dev->mmio_len = pci_resource_len(pdev, 0);
	hdmi_dev->regs = ioremap(hdmi_dev->mmio, hdmi_dev->mmio_len);
	if (!hdmi_dev->regs) {
		dev_err(dev->dev, "failed to map hdmi mmio\n");
		goto free;
	}

	hdmi_dev->dev = pdev;
	pci_set_drvdata(pdev, hdmi_dev);

	/* Initialize i2c controller */
	ret = oaktrail_hdmi_i2c_init(hdmi_dev->dev);
	if (ret)
		dev_err(dev->dev, "HDMI I2C initialization failed\n");

	dev_priv->hdmi_priv = hdmi_dev;
	oaktrail_hdmi_audio_disable(dev);
	return;

free:
	kfree(hdmi_dev);
out:
	return;
}

void oaktrail_hdmi_teardown(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;
	struct pci_dev *pdev;

	if (hdmi_dev) {
		pdev = hdmi_dev->dev;
		pci_set_drvdata(pdev, NULL);
		oaktrail_hdmi_i2c_exit(pdev);
		iounmap(hdmi_dev->regs);
		kfree(hdmi_dev);
		pci_dev_put(pdev);
	}
}

/* save HDMI register state */
void oaktrail_hdmi_save(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;
	struct psb_state *regs = &dev_priv->regs.psb;
	struct psb_pipe *pipeb = &dev_priv->regs.pipe[1];
	int i;

	/* dpll */
	hdmi_dev->saveDPLL_CTRL = PSB_RVDC32(DPLL_CTRL);
	hdmi_dev->saveDPLL_DIV_CTRL = PSB_RVDC32(DPLL_DIV_CTRL);
	hdmi_dev->saveDPLL_ADJUST = PSB_RVDC32(DPLL_ADJUST);
	hdmi_dev->saveDPLL_UPDATE = PSB_RVDC32(DPLL_UPDATE);
	hdmi_dev->saveDPLL_CLK_ENABLE = PSB_RVDC32(DPLL_CLK_ENABLE);

	/* pipe B */
	pipeb->conf = PSB_RVDC32(PIPEBCONF);
	pipeb->src = PSB_RVDC32(PIPEBSRC);
	pipeb->htotal = PSB_RVDC32(HTOTAL_B);
	pipeb->hblank = PSB_RVDC32(HBLANK_B);
	pipeb->hsync = PSB_RVDC32(HSYNC_B);
	pipeb->vtotal = PSB_RVDC32(VTOTAL_B);
	pipeb->vblank = PSB_RVDC32(VBLANK_B);
	pipeb->vsync = PSB_RVDC32(VSYNC_B);

	hdmi_dev->savePCH_PIPEBCONF = PSB_RVDC32(PCH_PIPEBCONF);
	hdmi_dev->savePCH_PIPEBSRC = PSB_RVDC32(PCH_PIPEBSRC);
	hdmi_dev->savePCH_HTOTAL_B = PSB_RVDC32(PCH_HTOTAL_B);
	hdmi_dev->savePCH_HBLANK_B = PSB_RVDC32(PCH_HBLANK_B);
	hdmi_dev->savePCH_HSYNC_B  = PSB_RVDC32(PCH_HSYNC_B);
	hdmi_dev->savePCH_VTOTAL_B = PSB_RVDC32(PCH_VTOTAL_B);
	hdmi_dev->savePCH_VBLANK_B = PSB_RVDC32(PCH_VBLANK_B);
	hdmi_dev->savePCH_VSYNC_B  = PSB_RVDC32(PCH_VSYNC_B);

	/* plane */
	pipeb->cntr = PSB_RVDC32(DSPBCNTR);
	pipeb->stride = PSB_RVDC32(DSPBSTRIDE);
	pipeb->addr = PSB_RVDC32(DSPBBASE);
	pipeb->surf = PSB_RVDC32(DSPBSURF);
	pipeb->linoff = PSB_RVDC32(DSPBLINOFF);
	pipeb->tileoff = PSB_RVDC32(DSPBTILEOFF);

	/* cursor B */
	regs->saveDSPBCURSOR_CTRL = PSB_RVDC32(CURBCNTR);
	regs->saveDSPBCURSOR_BASE = PSB_RVDC32(CURBBASE);
	regs->saveDSPBCURSOR_POS = PSB_RVDC32(CURBPOS);

	/* save palette */
	for (i = 0; i < 256; i++)
		pipeb->palette[i] = PSB_RVDC32(PALETTE_B + (i << 2));
}

/* restore HDMI register state */
void oaktrail_hdmi_restore(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct oaktrail_hdmi_dev *hdmi_dev = dev_priv->hdmi_priv;
	struct psb_state *regs = &dev_priv->regs.psb;
	struct psb_pipe *pipeb = &dev_priv->regs.pipe[1];
	int i;

	/* dpll */
	PSB_WVDC32(hdmi_dev->saveDPLL_CTRL, DPLL_CTRL);
	PSB_WVDC32(hdmi_dev->saveDPLL_DIV_CTRL, DPLL_DIV_CTRL);
	PSB_WVDC32(hdmi_dev->saveDPLL_ADJUST, DPLL_ADJUST);
	PSB_WVDC32(hdmi_dev->saveDPLL_UPDATE, DPLL_UPDATE);
	PSB_WVDC32(hdmi_dev->saveDPLL_CLK_ENABLE, DPLL_CLK_ENABLE);
	DRM_UDELAY(150);

	/* pipe */
	PSB_WVDC32(pipeb->src, PIPEBSRC);
	PSB_WVDC32(pipeb->htotal, HTOTAL_B);
	PSB_WVDC32(pipeb->hblank, HBLANK_B);
	PSB_WVDC32(pipeb->hsync,  HSYNC_B);
	PSB_WVDC32(pipeb->vtotal, VTOTAL_B);
	PSB_WVDC32(pipeb->vblank, VBLANK_B);
	PSB_WVDC32(pipeb->vsync,  VSYNC_B);

	PSB_WVDC32(hdmi_dev->savePCH_PIPEBSRC, PCH_PIPEBSRC);
	PSB_WVDC32(hdmi_dev->savePCH_HTOTAL_B, PCH_HTOTAL_B);
	PSB_WVDC32(hdmi_dev->savePCH_HBLANK_B, PCH_HBLANK_B);
	PSB_WVDC32(hdmi_dev->savePCH_HSYNC_B,  PCH_HSYNC_B);
	PSB_WVDC32(hdmi_dev->savePCH_VTOTAL_B, PCH_VTOTAL_B);
	PSB_WVDC32(hdmi_dev->savePCH_VBLANK_B, PCH_VBLANK_B);
	PSB_WVDC32(hdmi_dev->savePCH_VSYNC_B,  PCH_VSYNC_B);

	PSB_WVDC32(pipeb->conf, PIPEBCONF);
	PSB_WVDC32(hdmi_dev->savePCH_PIPEBCONF, PCH_PIPEBCONF);

	/* plane */
	PSB_WVDC32(pipeb->linoff, DSPBLINOFF);
	PSB_WVDC32(pipeb->stride, DSPBSTRIDE);
	PSB_WVDC32(pipeb->tileoff, DSPBTILEOFF);
	PSB_WVDC32(pipeb->cntr, DSPBCNTR);
	PSB_WVDC32(pipeb->surf, DSPBSURF);

	/* cursor B */
	PSB_WVDC32(regs->saveDSPBCURSOR_CTRL, CURBCNTR);
	PSB_WVDC32(regs->saveDSPBCURSOR_POS, CURBPOS);
	PSB_WVDC32(regs->saveDSPBCURSOR_BASE, CURBBASE);

	/* restore palette */
	for (i = 0; i < 256; i++)
		PSB_WVDC32(pipeb->palette[i], PALETTE_B + (i << 2));
}
