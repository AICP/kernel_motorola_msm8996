/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "debugfs.h"

#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "wlcore.h"
#include "debug.h"
#include "acx.h"
#include "ps.h"
#include "io.h"
#include "tx.h"
#include "hw_ops.h"

/* ms */
#define WL1271_DEBUGFS_STATS_LIFETIME 1000

/* debugfs macros idea from mac80211 */
int wl1271_format_buffer(char __user *userbuf, size_t count,
			 loff_t *ppos, char *fmt, ...)
{
	va_list args;
	char buf[DEBUGFS_FORMAT_BUFFER_SIZE];
	int res;

	va_start(args, fmt);
	res = vscnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	return simple_read_from_buffer(userbuf, count, ppos, buf, res);
}
EXPORT_SYMBOL_GPL(wl1271_format_buffer);

void wl1271_debugfs_update_stats(struct wl1271 *wl)
{
	int ret;

	mutex_lock(&wl->mutex);

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	if (wl->state == WL1271_STATE_ON && !wl->plt &&
	    time_after(jiffies, wl->stats.fw_stats_update +
		       msecs_to_jiffies(WL1271_DEBUGFS_STATS_LIFETIME))) {
		wl1271_acx_statistics(wl, wl->stats.fw_stats);
		wl->stats.fw_stats_update = jiffies;
	}

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
}
EXPORT_SYMBOL_GPL(wl1271_debugfs_update_stats);

DEBUGFS_READONLY_FILE(retry_count, "%u", wl->stats.retry_count);
DEBUGFS_READONLY_FILE(excessive_retries, "%u",
		      wl->stats.excessive_retries);

static ssize_t tx_queue_len_read(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	u32 queue_len;
	char buf[20];
	int res;

	queue_len = wl1271_tx_total_queue_count(wl);

	res = scnprintf(buf, sizeof(buf), "%u\n", queue_len);
	return simple_read_from_buffer(userbuf, count, ppos, buf, res);
}

static const struct file_operations tx_queue_len_ops = {
	.read = tx_queue_len_read,
	.open = simple_open,
	.llseek = default_llseek,
};

static void chip_op_handler(struct wl1271 *wl, unsigned long value,
			    void *arg)
{
	int ret;
	int (*chip_op) (struct wl1271 *wl);

	if (!arg) {
		wl1271_warning("debugfs chip_op_handler with no callback");
		return;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		return;

	chip_op = arg;
	chip_op(wl);

	wl1271_ps_elp_sleep(wl);
}


static inline void no_write_handler(struct wl1271 *wl,
				    unsigned long value,
				    unsigned long param)
{
}

#define WL12XX_CONF_DEBUGFS(param, conf_sub_struct,			\
			    min_val, max_val, write_handler_locked,	\
			    write_handler_arg)				\
	static ssize_t param##_read(struct file *file,			\
				      char __user *user_buf,		\
				      size_t count, loff_t *ppos)	\
	{								\
	struct wl1271 *wl = file->private_data;				\
	return wl1271_format_buffer(user_buf, count,			\
				    ppos, "%d\n",			\
				    wl->conf.conf_sub_struct.param);	\
	}								\
									\
	static ssize_t param##_write(struct file *file,			\
				     const char __user *user_buf,	\
				     size_t count, loff_t *ppos)	\
	{								\
	struct wl1271 *wl = file->private_data;				\
	unsigned long value;						\
	int ret;							\
									\
	ret = kstrtoul_from_user(user_buf, count, 10, &value);		\
	if (ret < 0) {							\
		wl1271_warning("illegal value for " #param);		\
		return -EINVAL;						\
	}								\
									\
	if (value < min_val || value > max_val) {			\
		wl1271_warning(#param " is not in valid range");	\
		return -ERANGE;						\
	}								\
									\
	mutex_lock(&wl->mutex);						\
	wl->conf.conf_sub_struct.param = value;				\
									\
	write_handler_locked(wl, value, write_handler_arg);		\
									\
	mutex_unlock(&wl->mutex);					\
	return count;							\
	}								\
									\
	static const struct file_operations param##_ops = {		\
		.read = param##_read,					\
		.write = param##_write,					\
		.open = simple_open,					\
		.llseek = default_llseek,				\
	};
static ssize_t gpio_power_read(struct file *file, char __user *user_buf,
			  size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	bool state = test_bit(WL1271_FLAG_GPIO_POWER, &wl->flags);

	int res;
	char buf[10];

	res = scnprintf(buf, sizeof(buf), "%d\n", state);

	return simple_read_from_buffer(user_buf, count, ppos, buf, res);
}

static ssize_t gpio_power_write(struct file *file,
			   const char __user *user_buf,
			   size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value in gpio_power");
		return -EINVAL;
	}

	mutex_lock(&wl->mutex);

	if (value)
		wl1271_power_on(wl);
	else
		wl1271_power_off(wl);

	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations gpio_power_ops = {
	.read = gpio_power_read,
	.write = gpio_power_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t start_recovery_write(struct file *file,
				    const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;

	mutex_lock(&wl->mutex);
	wl12xx_queue_recovery_work(wl);
	mutex_unlock(&wl->mutex);

	return count;
}

static const struct file_operations start_recovery_ops = {
	.write = start_recovery_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t dynamic_ps_timeout_read(struct file *file, char __user *user_buf,
			  size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;

	return wl1271_format_buffer(user_buf, count,
				    ppos, "%d\n",
				    wl->conf.conn.dynamic_ps_timeout);
}

static ssize_t dynamic_ps_timeout_write(struct file *file,
				    const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	struct wl12xx_vif *wlvif;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value in dynamic_ps");
		return -EINVAL;
	}

	if (value < 1 || value > 65535) {
		wl1271_warning("dyanmic_ps_timeout is not in valid range");
		return -ERANGE;
	}

	mutex_lock(&wl->mutex);

	wl->conf.conn.dynamic_ps_timeout = value;

	if (wl->state == WL1271_STATE_OFF)
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	/* In case we're already in PSM, trigger it again to set new timeout
	 * immediately without waiting for re-association
	 */

	wl12xx_for_each_wlvif_sta(wl, wlvif) {
		if (test_bit(WLVIF_FLAG_IN_PS, &wlvif->flags))
			wl1271_ps_set_mode(wl, wlvif, STATION_AUTO_PS_MODE);
	}

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations dynamic_ps_timeout_ops = {
	.read = dynamic_ps_timeout_read,
	.write = dynamic_ps_timeout_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t forced_ps_read(struct file *file, char __user *user_buf,
			  size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;

	return wl1271_format_buffer(user_buf, count,
				    ppos, "%d\n",
				    wl->conf.conn.forced_ps);
}

static ssize_t forced_ps_write(struct file *file,
				    const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	struct wl12xx_vif *wlvif;
	unsigned long value;
	int ret, ps_mode;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value in forced_ps");
		return -EINVAL;
	}

	if (value != 1 && value != 0) {
		wl1271_warning("forced_ps should be either 0 or 1");
		return -ERANGE;
	}

	mutex_lock(&wl->mutex);

	if (wl->conf.conn.forced_ps == value)
		goto out;

	wl->conf.conn.forced_ps = value;

	if (wl->state == WL1271_STATE_OFF)
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	/* In case we're already in PSM, trigger it again to switch mode
	 * immediately without waiting for re-association
	 */

	ps_mode = value ? STATION_POWER_SAVE_MODE : STATION_AUTO_PS_MODE;

	wl12xx_for_each_wlvif_sta(wl, wlvif) {
		if (test_bit(WLVIF_FLAG_IN_PS, &wlvif->flags))
			wl1271_ps_set_mode(wl, wlvif, ps_mode);
	}

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations forced_ps_ops = {
	.read = forced_ps_read,
	.write = forced_ps_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t split_scan_timeout_read(struct file *file, char __user *user_buf,
			  size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;

	return wl1271_format_buffer(user_buf, count,
				    ppos, "%d\n",
				    wl->conf.scan.split_scan_timeout / 1000);
}

static ssize_t split_scan_timeout_write(struct file *file,
				    const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value in split_scan_timeout");
		return -EINVAL;
	}

	if (value == 0)
		wl1271_info("split scan will be disabled");

	mutex_lock(&wl->mutex);

	wl->conf.scan.split_scan_timeout = value * 1000;

	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations split_scan_timeout_ops = {
	.read = split_scan_timeout_read,
	.write = split_scan_timeout_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t driver_state_read(struct file *file, char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	int res = 0;
	ssize_t ret;
	char *buf;

#define DRIVER_STATE_BUF_LEN 1024

	buf = kmalloc(DRIVER_STATE_BUF_LEN, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mutex_lock(&wl->mutex);

#define DRIVER_STATE_PRINT(x, fmt)   \
	(res += scnprintf(buf + res, DRIVER_STATE_BUF_LEN - res,\
			  #x " = " fmt "\n", wl->x))

#define DRIVER_STATE_PRINT_LONG(x) DRIVER_STATE_PRINT(x, "%ld")
#define DRIVER_STATE_PRINT_INT(x)  DRIVER_STATE_PRINT(x, "%d")
#define DRIVER_STATE_PRINT_STR(x)  DRIVER_STATE_PRINT(x, "%s")
#define DRIVER_STATE_PRINT_LHEX(x) DRIVER_STATE_PRINT(x, "0x%lx")
#define DRIVER_STATE_PRINT_HEX(x)  DRIVER_STATE_PRINT(x, "0x%x")

	DRIVER_STATE_PRINT_INT(tx_blocks_available);
	DRIVER_STATE_PRINT_INT(tx_allocated_blocks);
	DRIVER_STATE_PRINT_INT(tx_allocated_pkts[0]);
	DRIVER_STATE_PRINT_INT(tx_allocated_pkts[1]);
	DRIVER_STATE_PRINT_INT(tx_allocated_pkts[2]);
	DRIVER_STATE_PRINT_INT(tx_allocated_pkts[3]);
	DRIVER_STATE_PRINT_INT(tx_frames_cnt);
	DRIVER_STATE_PRINT_LHEX(tx_frames_map[0]);
	DRIVER_STATE_PRINT_INT(tx_queue_count[0]);
	DRIVER_STATE_PRINT_INT(tx_queue_count[1]);
	DRIVER_STATE_PRINT_INT(tx_queue_count[2]);
	DRIVER_STATE_PRINT_INT(tx_queue_count[3]);
	DRIVER_STATE_PRINT_INT(tx_packets_count);
	DRIVER_STATE_PRINT_INT(tx_results_count);
	DRIVER_STATE_PRINT_LHEX(flags);
	DRIVER_STATE_PRINT_INT(tx_blocks_freed);
	DRIVER_STATE_PRINT_INT(rx_counter);
	DRIVER_STATE_PRINT_INT(state);
	DRIVER_STATE_PRINT_INT(channel);
	DRIVER_STATE_PRINT_INT(band);
	DRIVER_STATE_PRINT_INT(power_level);
	DRIVER_STATE_PRINT_INT(sg_enabled);
	DRIVER_STATE_PRINT_INT(enable_11a);
	DRIVER_STATE_PRINT_INT(noise);
	DRIVER_STATE_PRINT_HEX(ap_fw_ps_map);
	DRIVER_STATE_PRINT_LHEX(ap_ps_map);
	DRIVER_STATE_PRINT_HEX(quirks);
	DRIVER_STATE_PRINT_HEX(irq);
	/* TODO: ref_clock and tcxo_clock were moved to wl12xx priv */
	DRIVER_STATE_PRINT_HEX(hw_pg_ver);
	DRIVER_STATE_PRINT_HEX(platform_quirks);
	DRIVER_STATE_PRINT_HEX(chip.id);
	DRIVER_STATE_PRINT_STR(chip.fw_ver_str);
	DRIVER_STATE_PRINT_INT(sched_scanning);

#undef DRIVER_STATE_PRINT_INT
#undef DRIVER_STATE_PRINT_LONG
#undef DRIVER_STATE_PRINT_HEX
#undef DRIVER_STATE_PRINT_LHEX
#undef DRIVER_STATE_PRINT_STR
#undef DRIVER_STATE_PRINT
#undef DRIVER_STATE_BUF_LEN

	mutex_unlock(&wl->mutex);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, res);
	kfree(buf);
	return ret;
}

static const struct file_operations driver_state_ops = {
	.read = driver_state_read,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t vifs_state_read(struct file *file, char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	struct wl12xx_vif *wlvif;
	int ret, res = 0;
	const int buf_size = 4096;
	char *buf;
	char tmp_buf[64];

	buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mutex_lock(&wl->mutex);

#define VIF_STATE_PRINT(x, fmt)				\
	(res += scnprintf(buf + res, buf_size - res,	\
			  #x " = " fmt "\n", wlvif->x))

#define VIF_STATE_PRINT_LONG(x)  VIF_STATE_PRINT(x, "%ld")
#define VIF_STATE_PRINT_INT(x)   VIF_STATE_PRINT(x, "%d")
#define VIF_STATE_PRINT_STR(x)   VIF_STATE_PRINT(x, "%s")
#define VIF_STATE_PRINT_LHEX(x)  VIF_STATE_PRINT(x, "0x%lx")
#define VIF_STATE_PRINT_LLHEX(x) VIF_STATE_PRINT(x, "0x%llx")
#define VIF_STATE_PRINT_HEX(x)   VIF_STATE_PRINT(x, "0x%x")

#define VIF_STATE_PRINT_NSTR(x, len)				\
	do {							\
		memset(tmp_buf, 0, sizeof(tmp_buf));		\
		memcpy(tmp_buf, wlvif->x,			\
		       min_t(u8, len, sizeof(tmp_buf) - 1));	\
		res += scnprintf(buf + res, buf_size - res,	\
				 #x " = %s\n", tmp_buf);	\
	} while (0)

	wl12xx_for_each_wlvif(wl, wlvif) {
		VIF_STATE_PRINT_INT(role_id);
		VIF_STATE_PRINT_INT(bss_type);
		VIF_STATE_PRINT_LHEX(flags);
		VIF_STATE_PRINT_INT(p2p);
		VIF_STATE_PRINT_INT(dev_role_id);
		VIF_STATE_PRINT_INT(dev_hlid);

		if (wlvif->bss_type == BSS_TYPE_STA_BSS ||
		    wlvif->bss_type == BSS_TYPE_IBSS) {
			VIF_STATE_PRINT_INT(sta.hlid);
			VIF_STATE_PRINT_INT(sta.ba_rx_bitmap);
			VIF_STATE_PRINT_INT(sta.basic_rate_idx);
			VIF_STATE_PRINT_INT(sta.ap_rate_idx);
			VIF_STATE_PRINT_INT(sta.p2p_rate_idx);
			VIF_STATE_PRINT_INT(sta.qos);
		} else {
			VIF_STATE_PRINT_INT(ap.global_hlid);
			VIF_STATE_PRINT_INT(ap.bcast_hlid);
			VIF_STATE_PRINT_LHEX(ap.sta_hlid_map[0]);
			VIF_STATE_PRINT_INT(ap.mgmt_rate_idx);
			VIF_STATE_PRINT_INT(ap.bcast_rate_idx);
			VIF_STATE_PRINT_INT(ap.ucast_rate_idx[0]);
			VIF_STATE_PRINT_INT(ap.ucast_rate_idx[1]);
			VIF_STATE_PRINT_INT(ap.ucast_rate_idx[2]);
			VIF_STATE_PRINT_INT(ap.ucast_rate_idx[3]);
		}
		VIF_STATE_PRINT_INT(last_tx_hlid);
		VIF_STATE_PRINT_LHEX(links_map[0]);
		VIF_STATE_PRINT_NSTR(ssid, wlvif->ssid_len);
		VIF_STATE_PRINT_INT(band);
		VIF_STATE_PRINT_INT(channel);
		VIF_STATE_PRINT_HEX(bitrate_masks[0]);
		VIF_STATE_PRINT_HEX(bitrate_masks[1]);
		VIF_STATE_PRINT_HEX(basic_rate_set);
		VIF_STATE_PRINT_HEX(basic_rate);
		VIF_STATE_PRINT_HEX(rate_set);
		VIF_STATE_PRINT_INT(beacon_int);
		VIF_STATE_PRINT_INT(default_key);
		VIF_STATE_PRINT_INT(aid);
		VIF_STATE_PRINT_INT(session_counter);
		VIF_STATE_PRINT_INT(psm_entry_retry);
		VIF_STATE_PRINT_INT(power_level);
		VIF_STATE_PRINT_INT(rssi_thold);
		VIF_STATE_PRINT_INT(last_rssi_event);
		VIF_STATE_PRINT_INT(ba_support);
		VIF_STATE_PRINT_INT(ba_allowed);
		VIF_STATE_PRINT_LLHEX(tx_security_seq);
		VIF_STATE_PRINT_INT(tx_security_last_seq_lsb);
	}

#undef VIF_STATE_PRINT_INT
#undef VIF_STATE_PRINT_LONG
#undef VIF_STATE_PRINT_HEX
#undef VIF_STATE_PRINT_LHEX
#undef VIF_STATE_PRINT_LLHEX
#undef VIF_STATE_PRINT_STR
#undef VIF_STATE_PRINT_NSTR
#undef VIF_STATE_PRINT

	mutex_unlock(&wl->mutex);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, res);
	kfree(buf);
	return ret;
}

static const struct file_operations vifs_state_ops = {
	.read = vifs_state_read,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t dtim_interval_read(struct file *file, char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	u8 value;

	if (wl->conf.conn.wake_up_event == CONF_WAKE_UP_EVENT_DTIM ||
	    wl->conf.conn.wake_up_event == CONF_WAKE_UP_EVENT_N_DTIM)
		value = wl->conf.conn.listen_interval;
	else
		value = 0;

	return wl1271_format_buffer(user_buf, count, ppos, "%d\n", value);
}

static ssize_t dtim_interval_write(struct file *file,
				   const char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value for dtim_interval");
		return -EINVAL;
	}

	if (value < 1 || value > 10) {
		wl1271_warning("dtim value is not in valid range");
		return -ERANGE;
	}

	mutex_lock(&wl->mutex);

	wl->conf.conn.listen_interval = value;
	/* for some reason there are different event types for 1 and >1 */
	if (value == 1)
		wl->conf.conn.wake_up_event = CONF_WAKE_UP_EVENT_DTIM;
	else
		wl->conf.conn.wake_up_event = CONF_WAKE_UP_EVENT_N_DTIM;

	/*
	 * we don't reconfigure ACX_WAKE_UP_CONDITIONS now, so it will only
	 * take effect on the next time we enter psm.
	 */
	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations dtim_interval_ops = {
	.read = dtim_interval_read,
	.write = dtim_interval_write,
	.open = simple_open,
	.llseek = default_llseek,
};



static ssize_t suspend_dtim_interval_read(struct file *file,
					  char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	u8 value;

	if (wl->conf.conn.suspend_wake_up_event == CONF_WAKE_UP_EVENT_DTIM ||
	    wl->conf.conn.suspend_wake_up_event == CONF_WAKE_UP_EVENT_N_DTIM)
		value = wl->conf.conn.suspend_listen_interval;
	else
		value = 0;

	return wl1271_format_buffer(user_buf, count, ppos, "%d\n", value);
}

static ssize_t suspend_dtim_interval_write(struct file *file,
					   const char __user *user_buf,
					   size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value for suspend_dtim_interval");
		return -EINVAL;
	}

	if (value < 1 || value > 10) {
		wl1271_warning("suspend_dtim value is not in valid range");
		return -ERANGE;
	}

	mutex_lock(&wl->mutex);

	wl->conf.conn.suspend_listen_interval = value;
	/* for some reason there are different event types for 1 and >1 */
	if (value == 1)
		wl->conf.conn.suspend_wake_up_event = CONF_WAKE_UP_EVENT_DTIM;
	else
		wl->conf.conn.suspend_wake_up_event = CONF_WAKE_UP_EVENT_N_DTIM;

	mutex_unlock(&wl->mutex);
	return count;
}


static const struct file_operations suspend_dtim_interval_ops = {
	.read = suspend_dtim_interval_read,
	.write = suspend_dtim_interval_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t beacon_interval_read(struct file *file, char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	u8 value;

	if (wl->conf.conn.wake_up_event == CONF_WAKE_UP_EVENT_BEACON ||
	    wl->conf.conn.wake_up_event == CONF_WAKE_UP_EVENT_N_BEACONS)
		value = wl->conf.conn.listen_interval;
	else
		value = 0;

	return wl1271_format_buffer(user_buf, count, ppos, "%d\n", value);
}

static ssize_t beacon_interval_write(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value for beacon_interval");
		return -EINVAL;
	}

	if (value < 1 || value > 255) {
		wl1271_warning("beacon interval value is not in valid range");
		return -ERANGE;
	}

	mutex_lock(&wl->mutex);

	wl->conf.conn.listen_interval = value;
	/* for some reason there are different event types for 1 and >1 */
	if (value == 1)
		wl->conf.conn.wake_up_event = CONF_WAKE_UP_EVENT_BEACON;
	else
		wl->conf.conn.wake_up_event = CONF_WAKE_UP_EVENT_N_BEACONS;

	/*
	 * we don't reconfigure ACX_WAKE_UP_CONDITIONS now, so it will only
	 * take effect on the next time we enter psm.
	 */
	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations beacon_interval_ops = {
	.read = beacon_interval_read,
	.write = beacon_interval_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t rx_streaming_interval_write(struct file *file,
			   const char __user *user_buf,
			   size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	struct wl12xx_vif *wlvif;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value in rx_streaming_interval!");
		return -EINVAL;
	}

	/* valid values: 0, 10-100 */
	if (value && (value < 10 || value > 100)) {
		wl1271_warning("value is not in range!");
		return -ERANGE;
	}

	mutex_lock(&wl->mutex);

	wl->conf.rx_streaming.interval = value;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl12xx_for_each_wlvif_sta(wl, wlvif) {
		wl1271_recalc_rx_streaming(wl, wlvif);
	}

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
	return count;
}

static ssize_t rx_streaming_interval_read(struct file *file,
			    char __user *userbuf,
			    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	return wl1271_format_buffer(userbuf, count, ppos,
				    "%d\n", wl->conf.rx_streaming.interval);
}

static const struct file_operations rx_streaming_interval_ops = {
	.read = rx_streaming_interval_read,
	.write = rx_streaming_interval_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t rx_streaming_always_write(struct file *file,
			   const char __user *user_buf,
			   size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	struct wl12xx_vif *wlvif;
	unsigned long value;
	int ret;

	ret = kstrtoul_from_user(user_buf, count, 10, &value);
	if (ret < 0) {
		wl1271_warning("illegal value in rx_streaming_write!");
		return -EINVAL;
	}

	/* valid values: 0, 10-100 */
	if (!(value == 0 || value == 1)) {
		wl1271_warning("value is not in valid!");
		return -EINVAL;
	}

	mutex_lock(&wl->mutex);

	wl->conf.rx_streaming.always = value;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl12xx_for_each_wlvif_sta(wl, wlvif) {
		wl1271_recalc_rx_streaming(wl, wlvif);
	}

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
	return count;
}

static ssize_t rx_streaming_always_read(struct file *file,
			    char __user *userbuf,
			    size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	return wl1271_format_buffer(userbuf, count, ppos,
				    "%d\n", wl->conf.rx_streaming.always);
}

static const struct file_operations rx_streaming_always_ops = {
	.read = rx_streaming_always_read,
	.write = rx_streaming_always_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static ssize_t beacon_filtering_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct wl1271 *wl = file->private_data;
	struct wl12xx_vif *wlvif;
	char buf[10];
	size_t len;
	unsigned long value;
	int ret;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';

	ret = kstrtoul(buf, 0, &value);
	if (ret < 0) {
		wl1271_warning("illegal value for beacon_filtering!");
		return -EINVAL;
	}

	mutex_lock(&wl->mutex);

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl12xx_for_each_wlvif(wl, wlvif) {
		ret = wl1271_acx_beacon_filter_opt(wl, wlvif, !!value);
	}

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
	return count;
}

static const struct file_operations beacon_filtering_ops = {
	.write = beacon_filtering_write,
	.open = simple_open,
	.llseek = default_llseek,
};

static int wl1271_debugfs_add_files(struct wl1271 *wl,
				    struct dentry *rootdir)
{
	int ret = 0;
	struct dentry *entry, *streaming;

	DEBUGFS_ADD(tx_queue_len, rootdir);
	DEBUGFS_ADD(retry_count, rootdir);
	DEBUGFS_ADD(excessive_retries, rootdir);

	DEBUGFS_ADD(gpio_power, rootdir);
	DEBUGFS_ADD(start_recovery, rootdir);
	DEBUGFS_ADD(driver_state, rootdir);
	DEBUGFS_ADD(vifs_state, rootdir);
	DEBUGFS_ADD(dtim_interval, rootdir);
	DEBUGFS_ADD(suspend_dtim_interval, rootdir);
	DEBUGFS_ADD(beacon_interval, rootdir);
	DEBUGFS_ADD(beacon_filtering, rootdir);
	DEBUGFS_ADD(dynamic_ps_timeout, rootdir);
	DEBUGFS_ADD(forced_ps, rootdir);
	DEBUGFS_ADD(split_scan_timeout, rootdir);

	streaming = debugfs_create_dir("rx_streaming", rootdir);
	if (!streaming || IS_ERR(streaming))
		goto err;

	DEBUGFS_ADD_PREFIX(rx_streaming, interval, streaming);
	DEBUGFS_ADD_PREFIX(rx_streaming, always, streaming);


	return 0;

err:
	if (IS_ERR(entry))
		ret = PTR_ERR(entry);
	else
		ret = -ENOMEM;

	return ret;
}

void wl1271_debugfs_reset(struct wl1271 *wl)
{
	if (!wl->stats.fw_stats)
		return;

	memset(wl->stats.fw_stats, 0, wl->stats.fw_stats_len);
	wl->stats.retry_count = 0;
	wl->stats.excessive_retries = 0;
}

int wl1271_debugfs_init(struct wl1271 *wl)
{
	int ret;
	struct dentry *rootdir;

	rootdir = debugfs_create_dir(KBUILD_MODNAME,
				     wl->hw->wiphy->debugfsdir);

	if (IS_ERR(rootdir)) {
		ret = PTR_ERR(rootdir);
		goto out;
	}

	wl->stats.fw_stats = kzalloc(wl->stats.fw_stats_len, GFP_KERNEL);
	if (!wl->stats.fw_stats) {
		ret = -ENOMEM;
		goto out_remove;
	}

	wl->stats.fw_stats_update = jiffies;

	ret = wl1271_debugfs_add_files(wl, rootdir);
	if (ret < 0)
		goto out_exit;

	ret = wlcore_debugfs_init(wl, rootdir);
	if (ret < 0)
		goto out_exit;

	goto out;

out_exit:
	wl1271_debugfs_exit(wl);

out_remove:
	debugfs_remove_recursive(rootdir);

out:
	return ret;
}

void wl1271_debugfs_exit(struct wl1271 *wl)
{
	kfree(wl->stats.fw_stats);
	wl->stats.fw_stats = NULL;
}
