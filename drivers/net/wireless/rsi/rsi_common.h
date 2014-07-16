/**
 * Copyright (c) 2014 Redpine Signals Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __RSI_COMMON_H__
#define __RSI_COMMON_H__

#include <linux/kthread.h>

#define EVENT_WAIT_FOREVER              0
#define TA_LOAD_ADDRESS                 0x00
#define FIRMWARE_RSI9113                "rsi_91x.fw"
#define QUEUE_NOT_FULL                  1
#define QUEUE_FULL                      0

static inline int rsi_init_event(struct rsi_event *pevent)
{
	atomic_set(&pevent->event_condition, 1);
	init_waitqueue_head(&pevent->event_queue);
	return 0;
}

static inline int rsi_wait_event(struct rsi_event *event, u32 timeout)
{
	int status = 0;

	if (!timeout)
		status = wait_event_interruptible(event->event_queue,
				(atomic_read(&event->event_condition) == 0));
	else
		status = wait_event_interruptible_timeout(event->event_queue,
				(atomic_read(&event->event_condition) == 0),
				timeout);
	return status;
}

static inline void rsi_set_event(struct rsi_event *event)
{
	atomic_set(&event->event_condition, 0);
	wake_up_interruptible(&event->event_queue);
}

static inline void rsi_reset_event(struct rsi_event *event)
{
	atomic_set(&event->event_condition, 1);
}

static inline int rsi_create_kthread(struct rsi_common *common,
				     struct rsi_thread *thread,
				     void *func_ptr,
				     u8 *name)
{
	init_completion(&thread->completion);
	thread->task = kthread_run(func_ptr, common, "%s", name);
	if (IS_ERR(thread->task))
		return (int)PTR_ERR(thread->task);

	return 0;
}

static inline int rsi_kill_thread(struct rsi_thread *handle)
{
	atomic_inc(&handle->thread_done);
	rsi_set_event(&handle->event);

	wait_for_completion(&handle->completion);
	return kthread_stop(handle->task);
}

void rsi_mac80211_detach(struct rsi_hw *hw);
u16 rsi_get_connected_channel(struct rsi_hw *adapter);
struct rsi_hw *rsi_91x_init(void);
void rsi_91x_deinit(struct rsi_hw *adapter);
int rsi_read_pkt(struct rsi_common *common, s32 rcv_pkt_len);
#endif
