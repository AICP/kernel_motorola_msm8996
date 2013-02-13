/*
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include <core/object.h>
#include <core/client.h>
#include <core/class.h>

#include <engine/fifo.h>

#include "nouveau_drm.h"
#include "nouveau_dma.h"
#include "nouveau_fence.h"

#include "nv50_display.h"

static int
nvc0_fence_emit32(struct nouveau_channel *chan, u64 virtual, u32 sequence)
{
	int ret = RING_SPACE(chan, 6);
	if (ret == 0) {
		BEGIN_NVC0(chan, 0, NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH, 5);
		OUT_RING  (chan, upper_32_bits(virtual));
		OUT_RING  (chan, lower_32_bits(virtual));
		OUT_RING  (chan, sequence);
		OUT_RING  (chan, NV84_SUBCHAN_SEMAPHORE_TRIGGER_WRITE_LONG);
		OUT_RING  (chan, 0x00000000);
		FIRE_RING (chan);
	}
	return ret;
}

static int
nvc0_fence_sync32(struct nouveau_channel *chan, u64 virtual, u32 sequence)
{
	int ret = RING_SPACE(chan, 5);
	if (ret == 0) {
		BEGIN_NVC0(chan, 0, NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH, 4);
		OUT_RING  (chan, upper_32_bits(virtual));
		OUT_RING  (chan, lower_32_bits(virtual));
		OUT_RING  (chan, sequence);
		OUT_RING  (chan, NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_GEQUAL |
				 NVC0_SUBCHAN_SEMAPHORE_TRIGGER_YIELD);
		FIRE_RING (chan);
	}
	return ret;
}

int
nvc0_fence_create(struct nouveau_drm *drm)
{
	struct nouveau_fifo *pfifo = nouveau_fifo(drm->device);
	struct nv84_fence_priv *priv;
	int ret;

	priv = drm->fence = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base.dtor = nv84_fence_destroy;
	priv->base.suspend = nv84_fence_suspend;
	priv->base.resume = nv84_fence_resume;
	priv->base.context_new = nv84_fence_context_new;
	priv->base.context_del = nv84_fence_context_del;
	priv->base.emit32 = nvc0_fence_emit32;
	priv->base.emit = nv84_fence_emit;
	priv->base.sync32 = nvc0_fence_sync32;
	priv->base.sync = nv84_fence_sync;
	priv->base.read = nv84_fence_read;

	init_waitqueue_head(&priv->base.waiting);
	priv->base.uevent = true;

	ret = nouveau_bo_new(drm->dev, 16 * (pfifo->max + 1), 0,
			     TTM_PL_FLAG_VRAM, 0, 0, NULL, &priv->bo);
	if (ret == 0) {
		ret = nouveau_bo_pin(priv->bo, TTM_PL_FLAG_VRAM);
		if (ret == 0) {
			ret = nouveau_bo_map(priv->bo);
			if (ret)
				nouveau_bo_unpin(priv->bo);
		}
		if (ret)
			nouveau_bo_ref(NULL, &priv->bo);
	}

	if (ret)
		nv84_fence_destroy(drm);
	return ret;
}
