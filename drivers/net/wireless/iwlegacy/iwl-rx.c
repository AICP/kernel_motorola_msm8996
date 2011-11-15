/******************************************************************************
 *
 * Copyright(c) 2003 - 2011 Intel Corporation. All rights reserved.
 *
 * Portions of this file are derived from the ipw3945 project, as well
 * as portions of the ieee80211 subsystem header files.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 *  Intel Linux Wireless <ilw@linux.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 *****************************************************************************/

#include <linux/etherdevice.h>
#include <linux/slab.h>
#include <net/mac80211.h>
#include <asm/unaligned.h>
#include "iwl-eeprom.h"
#include "iwl-dev.h"
#include "iwl-core.h"
#include "iwl-sta.h"
#include "iwl-io.h"
#include "iwl-helpers.h"
/************************** RX-FUNCTIONS ****************************/
/*
 * Rx theory of operation
 *
 * Driver allocates a circular buffer of Receive Buffer Descriptors (RBDs),
 * each of which point to Receive Buffers to be filled by the NIC.  These get
 * used not only for Rx frames, but for any command response or notification
 * from the NIC.  The driver and NIC manage the Rx buffers by means
 * of idxes into the circular buffer.
 *
 * Rx Queue Indexes
 * The host/firmware share two idx registers for managing the Rx buffers.
 *
 * The READ idx maps to the first position that the firmware may be writing
 * to -- the driver can read up to (but not including) this position and get
 * good data.
 * The READ idx is managed by the firmware once the card is enabled.
 *
 * The WRITE idx maps to the last position the driver has read from -- the
 * position preceding WRITE is the last slot the firmware can place a packet.
 *
 * The queue is empty (no good data) if WRITE = READ - 1, and is full if
 * WRITE = READ.
 *
 * During initialization, the host sets up the READ queue position to the first
 * IDX position, and WRITE to the last (READ - 1 wrapped)
 *
 * When the firmware places a packet in a buffer, it will advance the READ idx
 * and fire the RX interrupt.  The driver can then query the READ idx and
 * process as many packets as possible, moving the WRITE idx forward as it
 * resets the Rx queue buffers with new memory.
 *
 * The management in the driver is as follows:
 * + A list of pre-allocated SKBs is stored in iwl->rxq->rx_free.  When
 *   iwl->rxq->free_count drops to or below RX_LOW_WATERMARK, work is scheduled
 *   to replenish the iwl->rxq->rx_free.
 * + In il_rx_replenish (scheduled) if 'processed' != 'read' then the
 *   iwl->rxq is replenished and the READ IDX is updated (updating the
 *   'processed' and 'read' driver idxes as well)
 * + A received packet is processed and handed to the kernel network stack,
 *   detached from the iwl->rxq.  The driver 'processed' idx is updated.
 * + The Host/Firmware iwl->rxq is replenished at tasklet time from the rx_free
 *   list. If there are no allocated buffers in iwl->rxq->rx_free, the READ
 *   IDX is not incremented and iwl->status(RX_STALLED) is set.  If there
 *   were enough free buffers and RX_STALLED is set it is cleared.
 *
 *
 * Driver sequence:
 *
 * il_rx_queue_alloc()   Allocates rx_free
 * il_rx_replenish()     Replenishes rx_free list from rx_used, and calls
 *                            il_rx_queue_restock
 * il_rx_queue_restock() Moves available buffers from rx_free into Rx
 *                            queue, updates firmware pointers, and updates
 *                            the WRITE idx.  If insufficient rx_free buffers
 *                            are available, schedules il_rx_replenish
 *
 * -- enable interrupts --
 * ISR - il_rx()         Detach il_rx_bufs from pool up to the
 *                            READ IDX, detaching the SKB from the pool.
 *                            Moves the packet buffer from queue to rx_used.
 *                            Calls il_rx_queue_restock to refill any empty
 *                            slots.
 * ...
 *
 */

/**
 * il_rx_queue_space - Return number of free slots available in queue.
 */
int il_rx_queue_space(const struct il_rx_queue *q)
{
	int s = q->read - q->write;
	if (s <= 0)
		s += RX_QUEUE_SIZE;
	/* keep some buffer to not confuse full and empty queue */
	s -= 2;
	if (s < 0)
		s = 0;
	return s;
}
EXPORT_SYMBOL(il_rx_queue_space);

/**
 * il_rx_queue_update_write_ptr - Update the write pointer for the RX queue
 */
void
il_rx_queue_update_write_ptr(struct il_priv *il,
					struct il_rx_queue *q)
{
	unsigned long flags;
	u32 rx_wrt_ptr_reg = il->hw_params.rx_wrt_ptr_reg;
	u32 reg;

	spin_lock_irqsave(&q->lock, flags);

	if (q->need_update == 0)
		goto exit_unlock;

	/* If power-saving is in use, make sure device is awake */
	if (test_bit(S_POWER_PMI, &il->status)) {
		reg = _il_rd(il, CSR_UCODE_DRV_GP1);

		if (reg & CSR_UCODE_DRV_GP1_BIT_MAC_SLEEP) {
			D_INFO(
				"Rx queue requesting wakeup,"
				" GP1 = 0x%x\n", reg);
			il_set_bit(il, CSR_GP_CNTRL,
				CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
			goto exit_unlock;
		}

		q->write_actual = (q->write & ~0x7);
		il_wr(il, rx_wrt_ptr_reg,
				q->write_actual);

	/* Else device is assumed to be awake */
	} else {
		/* Device expects a multiple of 8 */
		q->write_actual = (q->write & ~0x7);
		il_wr(il, rx_wrt_ptr_reg,
			q->write_actual);
	}

	q->need_update = 0;

 exit_unlock:
	spin_unlock_irqrestore(&q->lock, flags);
}
EXPORT_SYMBOL(il_rx_queue_update_write_ptr);

int il_rx_queue_alloc(struct il_priv *il)
{
	struct il_rx_queue *rxq = &il->rxq;
	struct device *dev = &il->pci_dev->dev;
	int i;

	spin_lock_init(&rxq->lock);
	INIT_LIST_HEAD(&rxq->rx_free);
	INIT_LIST_HEAD(&rxq->rx_used);

	/* Alloc the circular buffer of Read Buffer Descriptors (RBDs) */
	rxq->bd = dma_alloc_coherent(dev, 4 * RX_QUEUE_SIZE, &rxq->bd_dma,
				     GFP_KERNEL);
	if (!rxq->bd)
		goto err_bd;

	rxq->rb_stts = dma_alloc_coherent(dev, sizeof(struct il_rb_status),
					  &rxq->rb_stts_dma, GFP_KERNEL);
	if (!rxq->rb_stts)
		goto err_rb;

	/* Fill the rx_used queue with _all_ of the Rx buffers */
	for (i = 0; i < RX_FREE_BUFFERS + RX_QUEUE_SIZE; i++)
		list_add_tail(&rxq->pool[i].list, &rxq->rx_used);

	/* Set us so that we have processed and used all buffers, but have
	 * not restocked the Rx queue with fresh buffers */
	rxq->read = rxq->write = 0;
	rxq->write_actual = 0;
	rxq->free_count = 0;
	rxq->need_update = 0;
	return 0;

err_rb:
	dma_free_coherent(&il->pci_dev->dev, 4 * RX_QUEUE_SIZE, rxq->bd,
			  rxq->bd_dma);
err_bd:
	return -ENOMEM;
}
EXPORT_SYMBOL(il_rx_queue_alloc);


void il_hdl_spectrum_measurement(struct il_priv *il,
					  struct il_rx_buf *rxb)
{
	struct il_rx_pkt *pkt = rxb_addr(rxb);
	struct il_spectrum_notification *report = &(pkt->u.spectrum_notif);

	if (!report->state) {
		D_11H(
			"Spectrum Measure Notification: Start\n");
		return;
	}

	memcpy(&il->measure_report, report, sizeof(*report));
	il->measurement_status |= MEASUREMENT_READY;
}
EXPORT_SYMBOL(il_hdl_spectrum_measurement);

/*
 * returns non-zero if packet should be dropped
 */
int il_set_decrypted_flag(struct il_priv *il,
			   struct ieee80211_hdr *hdr,
			   u32 decrypt_res,
			   struct ieee80211_rx_status *stats)
{
	u16 fc = le16_to_cpu(hdr->frame_control);

	/*
	 * All contexts have the same setting here due to it being
	 * a module parameter, so OK to check any context.
	 */
	if (il->ctx.active.filter_flags &
						RXON_FILTER_DIS_DECRYPT_MSK)
		return 0;

	if (!(fc & IEEE80211_FCTL_PROTECTED))
		return 0;

	D_RX("decrypt_res:0x%x\n", decrypt_res);
	switch (decrypt_res & RX_RES_STATUS_SEC_TYPE_MSK) {
	case RX_RES_STATUS_SEC_TYPE_TKIP:
		/* The uCode has got a bad phase 1 Key, pushes the packet.
		 * Decryption will be done in SW. */
		if ((decrypt_res & RX_RES_STATUS_DECRYPT_TYPE_MSK) ==
		    RX_RES_STATUS_BAD_KEY_TTAK)
			break;

	case RX_RES_STATUS_SEC_TYPE_WEP:
		if ((decrypt_res & RX_RES_STATUS_DECRYPT_TYPE_MSK) ==
		    RX_RES_STATUS_BAD_ICV_MIC) {
			/* bad ICV, the packet is destroyed since the
			 * decryption is inplace, drop it */
			D_RX("Packet destroyed\n");
			return -1;
		}
	case RX_RES_STATUS_SEC_TYPE_CCMP:
		if ((decrypt_res & RX_RES_STATUS_DECRYPT_TYPE_MSK) ==
		    RX_RES_STATUS_DECRYPT_OK) {
			D_RX("hw decrypt successfully!!!\n");
			stats->flag |= RX_FLAG_DECRYPTED;
		}
		break;

	default:
		break;
	}
	return 0;
}
EXPORT_SYMBOL(il_set_decrypted_flag);
