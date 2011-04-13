/*
 * Copyright (c) 2010-2011 Atheros Communications Inc.
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

#include "htc.h"

static struct dentry *ath9k_debugfs_root;

static int ath9k_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t read_file_tgt_stats(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	struct ath9k_htc_priv *priv = file->private_data;
	struct ath9k_htc_target_stats cmd_rsp;
	char buf[512];
	unsigned int len = 0;
	int ret = 0;

	memset(&cmd_rsp, 0, sizeof(cmd_rsp));

	WMI_CMD(WMI_TGT_STATS_CMDID);
	if (ret)
		return -EINVAL;


	len += snprintf(buf + len, sizeof(buf) - len,
			"%19s : %10u\n", "TX Short Retries",
			be32_to_cpu(cmd_rsp.tx_shortretry));
	len += snprintf(buf + len, sizeof(buf) - len,
			"%19s : %10u\n", "TX Long Retries",
			be32_to_cpu(cmd_rsp.tx_longretry));
	len += snprintf(buf + len, sizeof(buf) - len,
			"%19s : %10u\n", "TX Xretries",
			be32_to_cpu(cmd_rsp.tx_xretries));
	len += snprintf(buf + len, sizeof(buf) - len,
			"%19s : %10u\n", "TX Unaggr. Xretries",
			be32_to_cpu(cmd_rsp.ht_txunaggr_xretry));
	len += snprintf(buf + len, sizeof(buf) - len,
			"%19s : %10u\n", "TX Xretries (HT)",
			be32_to_cpu(cmd_rsp.ht_tx_xretries));
	len += snprintf(buf + len, sizeof(buf) - len,
			"%19s : %10u\n", "TX Rate", priv->debug.txrate);

	if (len > sizeof(buf))
		len = sizeof(buf);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_tgt_stats = {
	.read = read_file_tgt_stats,
	.open = ath9k_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_xmit(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct ath9k_htc_priv *priv = file->private_data;
	char buf[512];
	unsigned int len = 0;

	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "Buffers queued",
			priv->debug.tx_stats.buf_queued);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "Buffers completed",
			priv->debug.tx_stats.buf_completed);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "SKBs queued",
			priv->debug.tx_stats.skb_queued);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "SKBs completed",
			priv->debug.tx_stats.skb_completed);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "SKBs dropped",
			priv->debug.tx_stats.skb_dropped);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "CAB queued",
			priv->debug.tx_stats.cab_queued);

	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "BE queued",
			priv->debug.tx_stats.queue_stats[WME_AC_BE]);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "BK queued",
			priv->debug.tx_stats.queue_stats[WME_AC_BK]);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "VI queued",
			priv->debug.tx_stats.queue_stats[WME_AC_VI]);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "VO queued",
			priv->debug.tx_stats.queue_stats[WME_AC_VO]);

	if (len > sizeof(buf))
		len = sizeof(buf);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_xmit = {
	.read = read_file_xmit,
	.open = ath9k_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_recv(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct ath9k_htc_priv *priv = file->private_data;
	char buf[512];
	unsigned int len = 0;

	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "SKBs allocated",
			priv->debug.rx_stats.skb_allocated);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "SKBs completed",
			priv->debug.rx_stats.skb_completed);
	len += snprintf(buf + len, sizeof(buf) - len,
			"%20s : %10u\n", "SKBs Dropped",
			priv->debug.rx_stats.skb_dropped);

	if (len > sizeof(buf))
		len = sizeof(buf);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_recv = {
	.read = read_file_recv,
	.open = ath9k_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

int ath9k_htc_init_debug(struct ath_hw *ah)
{
	struct ath_common *common = ath9k_hw_common(ah);
	struct ath9k_htc_priv *priv = (struct ath9k_htc_priv *) common->priv;

	if (!ath9k_debugfs_root)
		return -ENOENT;

	priv->debug.debugfs_phy = debugfs_create_dir(wiphy_name(priv->hw->wiphy),
						     ath9k_debugfs_root);
	if (!priv->debug.debugfs_phy)
		goto err;

	priv->debug.debugfs_tgt_stats = debugfs_create_file("tgt_stats", S_IRUSR,
						    priv->debug.debugfs_phy,
						    priv, &fops_tgt_stats);
	if (!priv->debug.debugfs_tgt_stats)
		goto err;


	priv->debug.debugfs_xmit = debugfs_create_file("xmit", S_IRUSR,
						       priv->debug.debugfs_phy,
						       priv, &fops_xmit);
	if (!priv->debug.debugfs_xmit)
		goto err;

	priv->debug.debugfs_recv = debugfs_create_file("recv", S_IRUSR,
						       priv->debug.debugfs_phy,
						       priv, &fops_recv);
	if (!priv->debug.debugfs_recv)
		goto err;

	return 0;

err:
	ath9k_htc_exit_debug(ah);
	return -ENOMEM;
}

void ath9k_htc_exit_debug(struct ath_hw *ah)
{
	struct ath_common *common = ath9k_hw_common(ah);
	struct ath9k_htc_priv *priv = (struct ath9k_htc_priv *) common->priv;

	debugfs_remove(priv->debug.debugfs_recv);
	debugfs_remove(priv->debug.debugfs_xmit);
	debugfs_remove(priv->debug.debugfs_tgt_stats);
	debugfs_remove(priv->debug.debugfs_phy);
}

int ath9k_htc_debug_create_root(void)
{
	ath9k_debugfs_root = debugfs_create_dir(KBUILD_MODNAME, NULL);
	if (!ath9k_debugfs_root)
		return -ENOENT;

	return 0;
}

void ath9k_htc_debug_remove_root(void)
{
	debugfs_remove(ath9k_debugfs_root);
	ath9k_debugfs_root = NULL;
}
