/*
 * Marvell Wireless LAN device driver: AP TX and RX data handling
 *
 * Copyright (C) 2012, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available by writing to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 */

#include "decl.h"
#include "ioctl.h"
#include "main.h"
#include "wmm.h"

static void mwifiex_uap_queue_bridged_pkt(struct mwifiex_private *priv,
					 struct sk_buff *skb)
{
	struct mwifiex_adapter *adapter = priv->adapter;
	struct uap_rxpd *uap_rx_pd;
	struct rx_packet_hdr *rx_pkt_hdr;
	struct sk_buff *new_skb;
	struct mwifiex_txinfo *tx_info;
	int hdr_chop;
	struct timeval tv;
	u8 rfc1042_eth_hdr[ETH_ALEN] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

	uap_rx_pd = (struct uap_rxpd *)(skb->data);
	rx_pkt_hdr = (void *)uap_rx_pd + le16_to_cpu(uap_rx_pd->rx_pkt_offset);

	if ((atomic_read(&adapter->pending_bridged_pkts) >=
					     MWIFIEX_BRIDGED_PKTS_THRESHOLD)) {
		dev_err(priv->adapter->dev,
			"Tx: Bridge packet limit reached. Drop packet!\n");
		kfree_skb(skb);
		return;
	}

	if (!memcmp(&rx_pkt_hdr->rfc1042_hdr,
		    rfc1042_eth_hdr, sizeof(rfc1042_eth_hdr)))
		/* Chop off the rxpd + the excess memory from
		 * 802.2/llc/snap header that was removed.
		 */
		hdr_chop = (u8 *)eth_hdr - (u8 *)uap_rx_pd;
	else
		/* Chop off the rxpd */
		hdr_chop = (u8 *)&rx_pkt_hdr->eth803_hdr - (u8 *)uap_rx_pd;

	/* Chop off the leading header bytes so the it points
	 * to the start of either the reconstructed EthII frame
	 * or the 802.2/llc/snap frame.
	 */
	skb_pull(skb, hdr_chop);

	if (skb_headroom(skb) < MWIFIEX_MIN_DATA_HEADER_LEN) {
		dev_dbg(priv->adapter->dev,
			"data: Tx: insufficient skb headroom %d\n",
			skb_headroom(skb));
		/* Insufficient skb headroom - allocate a new skb */
		new_skb =
			skb_realloc_headroom(skb, MWIFIEX_MIN_DATA_HEADER_LEN);
		if (unlikely(!new_skb)) {
			dev_err(priv->adapter->dev,
				"Tx: cannot allocate new_skb\n");
			kfree_skb(skb);
			priv->stats.tx_dropped++;
			return;
		}

		kfree_skb(skb);
		skb = new_skb;
		dev_dbg(priv->adapter->dev, "info: new skb headroom %d\n",
			skb_headroom(skb));
	}

	tx_info = MWIFIEX_SKB_TXCB(skb);
	tx_info->bss_num = priv->bss_num;
	tx_info->bss_type = priv->bss_type;
	tx_info->flags |= MWIFIEX_BUF_FLAG_BRIDGED_PKT;

	do_gettimeofday(&tv);
	skb->tstamp = timeval_to_ktime(tv);
	mwifiex_wmm_add_buf_txqueue(priv, skb);
	atomic_inc(&adapter->tx_pending);
	atomic_inc(&adapter->pending_bridged_pkts);

	if ((atomic_read(&adapter->tx_pending) >= MAX_TX_PENDING)) {
		mwifiex_set_trans_start(priv->netdev);
		mwifiex_stop_net_dev_queue(priv->netdev, priv->adapter);
	}
	return;
}

/*
 * This function contains logic for AP packet forwarding.
 *
 * If a packet is multicast/broadcast, it is sent to kernel/upper layer
 * as well as queued back to AP TX queue so that it can be sent to other
 * associated stations.
 * If a packet is unicast and RA is present in associated station list,
 * it is again requeued into AP TX queue.
 * If a packet is unicast and RA is not in associated station list,
 * packet is forwarded to kernel to handle routing logic.
 */
int mwifiex_handle_uap_rx_forward(struct mwifiex_private *priv,
				  struct sk_buff *skb)
{
	struct mwifiex_adapter *adapter = priv->adapter;
	struct uap_rxpd *uap_rx_pd;
	struct rx_packet_hdr *rx_pkt_hdr;
	u8 ra[ETH_ALEN];
	struct sk_buff *skb_uap;

	uap_rx_pd = (struct uap_rxpd *)(skb->data);
	rx_pkt_hdr = (void *)uap_rx_pd + le16_to_cpu(uap_rx_pd->rx_pkt_offset);

	/* don't do packet forwarding in disconnected state */
	if (!priv->media_connected) {
		dev_err(adapter->dev, "drop packet in disconnected state.\n");
		dev_kfree_skb_any(skb);
		return 0;
	}

	memcpy(ra, rx_pkt_hdr->eth803_hdr.h_dest, ETH_ALEN);

	if (is_multicast_ether_addr(ra)) {
		skb_uap = skb_copy(skb, GFP_ATOMIC);
		mwifiex_uap_queue_bridged_pkt(priv, skb_uap);
	} else {
		if (mwifiex_get_sta_entry(priv, ra)) {
			/* Requeue Intra-BSS packet */
			mwifiex_uap_queue_bridged_pkt(priv, skb);
			return 0;
		}
	}

	/* Forward unicat/Inter-BSS packets to kernel. */
	return mwifiex_process_rx_packet(adapter, skb);
}

/*
 * This function processes the packet received on AP interface.
 *
 * The function looks into the RxPD and performs sanity tests on the
 * received buffer to ensure its a valid packet before processing it
 * further. If the packet is determined to be aggregated, it is
 * de-aggregated accordingly. Then skb is passed to AP packet forwarding logic.
 *
 * The completion callback is called after processing is complete.
 */
int mwifiex_process_uap_rx_packet(struct mwifiex_adapter *adapter,
				  struct sk_buff *skb)
{
	int ret;
	struct uap_rxpd *uap_rx_pd;
	struct mwifiex_rxinfo *rx_info = MWIFIEX_SKB_RXCB(skb);
	struct rx_packet_hdr *rx_pkt_hdr;
	u16 rx_pkt_type;
	struct mwifiex_private *priv =
			mwifiex_get_priv_by_id(adapter, rx_info->bss_num,
					       rx_info->bss_type);

	if (!priv)
		return -1;

	uap_rx_pd = (struct uap_rxpd *)(skb->data);
	rx_pkt_type = le16_to_cpu(uap_rx_pd->rx_pkt_type);
	rx_pkt_hdr = (void *)uap_rx_pd + le16_to_cpu(uap_rx_pd->rx_pkt_offset);

	if ((le16_to_cpu(uap_rx_pd->rx_pkt_offset) +
	     le16_to_cpu(uap_rx_pd->rx_pkt_length)) > (u16) skb->len) {
		dev_err(adapter->dev,
			"wrong rx packet: len=%d, offset=%d, length=%d\n",
			skb->len, le16_to_cpu(uap_rx_pd->rx_pkt_offset),
			le16_to_cpu(uap_rx_pd->rx_pkt_length));
		priv->stats.rx_dropped++;

		if (adapter->if_ops.data_complete)
			adapter->if_ops.data_complete(adapter, skb);
		else
			dev_kfree_skb_any(skb);

		return 0;
	}
	ret = mwifiex_handle_uap_rx_forward(priv, skb);

	if (ret) {
		priv->stats.rx_dropped++;
		if (adapter->if_ops.data_complete)
			adapter->if_ops.data_complete(adapter, skb);
		else
			dev_kfree_skb_any(skb);
	}

	return ret;
}
