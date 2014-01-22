/*
 * Copyright(c) 2004-2005 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/in.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/inet.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <linux/nsproxy.h>

#include "bonding.h"

#define to_dev(obj)	container_of(obj, struct device, kobj)
#define to_bond(cd)	((struct bonding *)(netdev_priv(to_net_dev(cd))))

/*
 * "show" function for the bond_masters attribute.
 * The class parameter is ignored.
 */
static ssize_t bonding_show_bonds(struct class *cls,
				  struct class_attribute *attr,
				  char *buf)
{
	struct bond_net *bn =
		container_of(attr, struct bond_net, class_attr_bonding_masters);
	int res = 0;
	struct bonding *bond;

	rtnl_lock();

	list_for_each_entry(bond, &bn->dev_list, bond_list) {
		if (res > (PAGE_SIZE - IFNAMSIZ)) {
			/* not enough space for another interface name */
			if ((PAGE_SIZE - res) > 10)
				res = PAGE_SIZE - 10;
			res += sprintf(buf + res, "++more++ ");
			break;
		}
		res += sprintf(buf + res, "%s ", bond->dev->name);
	}
	if (res)
		buf[res-1] = '\n'; /* eat the leftover space */

	rtnl_unlock();
	return res;
}

static struct net_device *bond_get_by_name(struct bond_net *bn, const char *ifname)
{
	struct bonding *bond;

	list_for_each_entry(bond, &bn->dev_list, bond_list) {
		if (strncmp(bond->dev->name, ifname, IFNAMSIZ) == 0)
			return bond->dev;
	}
	return NULL;
}

/*
 * "store" function for the bond_masters attribute.  This is what
 * creates and deletes entire bonds.
 *
 * The class parameter is ignored.
 *
 */

static ssize_t bonding_store_bonds(struct class *cls,
				   struct class_attribute *attr,
				   const char *buffer, size_t count)
{
	struct bond_net *bn =
		container_of(attr, struct bond_net, class_attr_bonding_masters);
	char command[IFNAMSIZ + 1] = {0, };
	char *ifname;
	int rv, res = count;

	sscanf(buffer, "%16s", command); /* IFNAMSIZ*/
	ifname = command + 1;
	if ((strlen(command) <= 1) ||
	    !dev_valid_name(ifname))
		goto err_no_cmd;

	if (command[0] == '+') {
		pr_info("%s is being created...\n", ifname);
		rv = bond_create(bn->net, ifname);
		if (rv) {
			if (rv == -EEXIST)
				pr_info("%s already exists.\n", ifname);
			else
				pr_info("%s creation failed.\n", ifname);
			res = rv;
		}
	} else if (command[0] == '-') {
		struct net_device *bond_dev;

		rtnl_lock();
		bond_dev = bond_get_by_name(bn, ifname);
		if (bond_dev) {
			pr_info("%s is being deleted...\n", ifname);
			unregister_netdevice(bond_dev);
		} else {
			pr_err("unable to delete non-existent %s\n", ifname);
			res = -ENODEV;
		}
		rtnl_unlock();
	} else
		goto err_no_cmd;

	/* Always return either count or an error.  If you return 0, you'll
	 * get called forever, which is bad.
	 */
	return res;

err_no_cmd:
	pr_err("no command found in bonding_masters. Use +ifname or -ifname.\n");
	return -EPERM;
}

/* class attribute for bond_masters file.  This ends up in /sys/class/net */
static const struct class_attribute class_attr_bonding_masters = {
	.attr = {
		.name = "bonding_masters",
		.mode = S_IWUSR | S_IRUGO,
	},
	.show = bonding_show_bonds,
	.store = bonding_store_bonds,
};

/*
 * Show the slaves in the current bond.
 */
static ssize_t bonding_show_slaves(struct device *d,
				   struct device_attribute *attr, char *buf)
{
	struct bonding *bond = to_bond(d);
	struct list_head *iter;
	struct slave *slave;
	int res = 0;

	if (!rtnl_trylock())
		return restart_syscall();

	bond_for_each_slave(bond, slave, iter) {
		if (res > (PAGE_SIZE - IFNAMSIZ)) {
			/* not enough space for another interface name */
			if ((PAGE_SIZE - res) > 10)
				res = PAGE_SIZE - 10;
			res += sprintf(buf + res, "++more++ ");
			break;
		}
		res += sprintf(buf + res, "%s ", slave->dev->name);
	}

	rtnl_unlock();

	if (res)
		buf[res-1] = '\n'; /* eat the leftover space */

	return res;
}

/*
 * Set the slaves in the current bond.
 * This is supposed to be only thin wrapper for bond_enslave and bond_release.
 * All hard work should be done there.
 */
static ssize_t bonding_store_slaves(struct device *d,
				    struct device_attribute *attr,
				    const char *buffer, size_t count)
{
	char command[IFNAMSIZ + 1] = { 0, };
	char *ifname;
	int res, ret = count;
	struct net_device *dev;
	struct bonding *bond = to_bond(d);

	if (!rtnl_trylock())
		return restart_syscall();

	sscanf(buffer, "%16s", command); /* IFNAMSIZ*/
	ifname = command + 1;
	if ((strlen(command) <= 1) ||
	    !dev_valid_name(ifname))
		goto err_no_cmd;

	dev = __dev_get_by_name(dev_net(bond->dev), ifname);
	if (!dev) {
		pr_info("%s: Interface %s does not exist!\n",
			bond->dev->name, ifname);
		ret = -ENODEV;
		goto out;
	}

	switch (command[0]) {
	case '+':
		pr_info("%s: Adding slave %s.\n", bond->dev->name, dev->name);
		res = bond_enslave(bond->dev, dev);
		break;

	case '-':
		pr_info("%s: Removing slave %s.\n", bond->dev->name, dev->name);
		res = bond_release(bond->dev, dev);
		break;

	default:
		goto err_no_cmd;
	}

	if (res)
		ret = res;
	goto out;

err_no_cmd:
	pr_err("no command found in slaves file for bond %s. Use +ifname or -ifname.\n",
	       bond->dev->name);
	ret = -EPERM;

out:
	rtnl_unlock();
	return ret;
}

static DEVICE_ATTR(slaves, S_IRUGO | S_IWUSR, bonding_show_slaves,
		   bonding_store_slaves);

/*
 * Show and set the bonding mode.  The bond interface must be down to
 * change the mode.
 */
static ssize_t bonding_show_mode(struct device *d,
				 struct device_attribute *attr, char *buf)
{
	struct bonding *bond = to_bond(d);
	struct bond_opt_value *val;

	val = bond_opt_get_val(BOND_OPT_MODE, bond->params.mode);

	return sprintf(buf, "%s %d\n", val->string, bond->params.mode);
}

static ssize_t bonding_store_mode(struct device *d,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_MODE, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR,
		   bonding_show_mode, bonding_store_mode);

/*
 * Show and set the bonding transmit hash method.
 */
static ssize_t bonding_show_xmit_hash(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);
	struct bond_opt_value *val;

	val = bond_opt_get_val(BOND_OPT_XMIT_HASH, bond->params.xmit_policy);

	return sprintf(buf, "%s %d\n", val->string, bond->params.xmit_policy);
}

static ssize_t bonding_store_xmit_hash(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_XMIT_HASH, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(xmit_hash_policy, S_IRUGO | S_IWUSR,
		   bonding_show_xmit_hash, bonding_store_xmit_hash);

/*
 * Show and set arp_validate.
 */
static ssize_t bonding_show_arp_validate(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct bonding *bond = to_bond(d);
	struct bond_opt_value *val;

	val = bond_opt_get_val(BOND_OPT_ARP_VALIDATE,
			       bond->params.arp_validate);

	return sprintf(buf, "%s %d\n", val->string, bond->params.arp_validate);
}

static ssize_t bonding_store_arp_validate(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_ARP_VALIDATE, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}

static DEVICE_ATTR(arp_validate, S_IRUGO | S_IWUSR, bonding_show_arp_validate,
		   bonding_store_arp_validate);
/*
 * Show and set arp_all_targets.
 */
static ssize_t bonding_show_arp_all_targets(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct bonding *bond = to_bond(d);
	struct bond_opt_value *val;

	val = bond_opt_get_val(BOND_OPT_ARP_ALL_TARGETS,
			       bond->params.arp_all_targets);
	return sprintf(buf, "%s %d\n",
		       val->string, bond->params.arp_all_targets);
}

static ssize_t bonding_store_arp_all_targets(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_ARP_ALL_TARGETS, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}

static DEVICE_ATTR(arp_all_targets, S_IRUGO | S_IWUSR,
		   bonding_show_arp_all_targets, bonding_store_arp_all_targets);

/*
 * Show and store fail_over_mac.  User only allowed to change the
 * value when there are no slaves.
 */
static ssize_t bonding_show_fail_over_mac(struct device *d,
					  struct device_attribute *attr,
					  char *buf)
{
	struct bonding *bond = to_bond(d);
	struct bond_opt_value *val;

	val = bond_opt_get_val(BOND_OPT_FAIL_OVER_MAC,
			       bond->params.fail_over_mac);

	return sprintf(buf, "%s %d\n", val->string, bond->params.fail_over_mac);
}

static ssize_t bonding_store_fail_over_mac(struct device *d,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_FAIL_OVER_MAC, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}

static DEVICE_ATTR(fail_over_mac, S_IRUGO | S_IWUSR,
		   bonding_show_fail_over_mac, bonding_store_fail_over_mac);

/*
 * Show and set the arp timer interval.  There are two tricky bits
 * here.  First, if ARP monitoring is activated, then we must disable
 * MII monitoring.  Second, if the ARP timer isn't running, we must
 * start it.
 */
static ssize_t bonding_show_arp_interval(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.arp_interval);
}

static ssize_t bonding_store_arp_interval(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_ARP_INTERVAL, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(arp_interval, S_IRUGO | S_IWUSR,
		   bonding_show_arp_interval, bonding_store_arp_interval);

/*
 * Show and set the arp targets.
 */
static ssize_t bonding_show_arp_targets(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	struct bonding *bond = to_bond(d);
	int i, res = 0;

	for (i = 0; i < BOND_MAX_ARP_TARGETS; i++) {
		if (bond->params.arp_targets[i])
			res += sprintf(buf + res, "%pI4 ",
				       &bond->params.arp_targets[i]);
	}
	if (res)
		buf[res-1] = '\n'; /* eat the leftover space */

	return res;
}

static ssize_t bonding_store_arp_targets(struct device *d,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_ARP_TARGETS, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(arp_ip_target, S_IRUGO | S_IWUSR , bonding_show_arp_targets, bonding_store_arp_targets);

/*
 * Show and set the up and down delays.  These must be multiples of the
 * MII monitoring value, and are stored internally as the multiplier.
 * Thus, we must translate to MS for the real world.
 */
static ssize_t bonding_show_downdelay(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.downdelay * bond->params.miimon);
}

static ssize_t bonding_store_downdelay(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_DOWNDELAY, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(downdelay, S_IRUGO | S_IWUSR,
		   bonding_show_downdelay, bonding_store_downdelay);

static ssize_t bonding_show_updelay(struct device *d,
				    struct device_attribute *attr,
				    char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.updelay * bond->params.miimon);

}

static ssize_t bonding_store_updelay(struct device *d,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_UPDELAY, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(updelay, S_IRUGO | S_IWUSR,
		   bonding_show_updelay, bonding_store_updelay);

/*
 * Show and set the LACP interval.  Interface must be down, and the mode
 * must be set to 802.3ad mode.
 */
static ssize_t bonding_show_lacp(struct device *d,
				 struct device_attribute *attr,
				 char *buf)
{
	struct bonding *bond = to_bond(d);
	struct bond_opt_value *val;

	val = bond_opt_get_val(BOND_OPT_LACP_RATE, bond->params.lacp_fast);

	return sprintf(buf, "%s %d\n", val->string, bond->params.lacp_fast);
}

static ssize_t bonding_store_lacp(struct device *d,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_LACP_RATE, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(lacp_rate, S_IRUGO | S_IWUSR,
		   bonding_show_lacp, bonding_store_lacp);

static ssize_t bonding_show_min_links(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.min_links);
}

static ssize_t bonding_store_min_links(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_MINLINKS, (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR(min_links, S_IRUGO | S_IWUSR,
		   bonding_show_min_links, bonding_store_min_links);

static ssize_t bonding_show_ad_select(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		ad_select_tbl[bond->params.ad_select].modename,
		bond->params.ad_select);
}


static ssize_t bonding_store_ad_select(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int new_value, ret;
	struct bonding *bond = to_bond(d);

	new_value = bond_parse_parm(buf, ad_select_tbl);
	if (new_value < 0) {
		pr_err("%s: Ignoring invalid ad_select value %.*s.\n",
		       bond->dev->name, (int)strlen(buf) - 1, buf);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_ad_select_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(ad_select, S_IRUGO | S_IWUSR,
		   bonding_show_ad_select, bonding_store_ad_select);

/*
 * Show and set the number of peer notifications to send after a failover event.
 */
static ssize_t bonding_show_num_peer_notif(struct device *d,
					   struct device_attribute *attr,
					   char *buf)
{
	struct bonding *bond = to_bond(d);
	return sprintf(buf, "%d\n", bond->params.num_peer_notif);
}

static ssize_t bonding_store_num_peer_notif(struct device *d,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	u8 new_value;
	int ret;

	ret = kstrtou8(buf, 10, &new_value);
	if (ret) {
		pr_err("%s: invalid value %s specified.\n",
		       bond->dev->name, buf);
		return ret;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_num_peer_notif_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(num_grat_arp, S_IRUGO | S_IWUSR,
		   bonding_show_num_peer_notif, bonding_store_num_peer_notif);
static DEVICE_ATTR(num_unsol_na, S_IRUGO | S_IWUSR,
		   bonding_show_num_peer_notif, bonding_store_num_peer_notif);

/*
 * Show and set the MII monitor interval.  There are two tricky bits
 * here.  First, if MII monitoring is activated, then we must disable
 * ARP monitoring.  Second, if the timer isn't running, we must
 * start it.
 */
static ssize_t bonding_show_miimon(struct device *d,
				   struct device_attribute *attr,
				   char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.miimon);
}

static ssize_t bonding_store_miimon(struct device *d,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int new_value, ret;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err("%s: no miimon value specified.\n",
		       bond->dev->name);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_miimon_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(miimon, S_IRUGO | S_IWUSR,
		   bonding_show_miimon, bonding_store_miimon);

/*
 * Show and set the primary slave.  The store function is much
 * simpler than bonding_store_slaves function because it only needs to
 * handle one interface name.
 * The bond must be a mode that supports a primary for this be
 * set.
 */
static ssize_t bonding_show_primary(struct device *d,
				    struct device_attribute *attr,
				    char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->primary_slave)
		count = sprintf(buf, "%s\n", bond->primary_slave->dev->name);

	return count;
}

static ssize_t bonding_store_primary(struct device *d,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	char ifname[IFNAMSIZ];
	int ret;

	sscanf(buf, "%15s", ifname); /* IFNAMSIZ */
	if (ifname[0] == '\n')
		ifname[0] = '\0';

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_primary_set(bond, ifname);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(primary, S_IRUGO | S_IWUSR,
		   bonding_show_primary, bonding_store_primary);

/*
 * Show and set the primary_reselect flag.
 */
static ssize_t bonding_show_primary_reselect(struct device *d,
					     struct device_attribute *attr,
					     char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		       pri_reselect_tbl[bond->params.primary_reselect].modename,
		       bond->params.primary_reselect);
}

static ssize_t bonding_store_primary_reselect(struct device *d,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	int new_value, ret;
	struct bonding *bond = to_bond(d);

	new_value = bond_parse_parm(buf, pri_reselect_tbl);
	if (new_value < 0)  {
		pr_err("%s: Ignoring invalid primary_reselect value %.*s.\n",
		       bond->dev->name,
		       (int) strlen(buf) - 1, buf);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_primary_reselect_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(primary_reselect, S_IRUGO | S_IWUSR,
		   bonding_show_primary_reselect,
		   bonding_store_primary_reselect);

/*
 * Show and set the use_carrier flag.
 */
static ssize_t bonding_show_carrier(struct device *d,
				    struct device_attribute *attr,
				    char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.use_carrier);
}

static ssize_t bonding_store_carrier(struct device *d,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int new_value, ret;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err("%s: no use_carrier value specified.\n",
		       bond->dev->name);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_use_carrier_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(use_carrier, S_IRUGO | S_IWUSR,
		   bonding_show_carrier, bonding_store_carrier);


/*
 * Show and set currently active_slave.
 */
static ssize_t bonding_show_active_slave(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct bonding *bond = to_bond(d);
	struct net_device *slave_dev;
	int count = 0;

	rcu_read_lock();
	slave_dev = bond_option_active_slave_get_rcu(bond);
	if (slave_dev)
		count = sprintf(buf, "%s\n", slave_dev->name);
	rcu_read_unlock();

	return count;
}

static ssize_t bonding_store_active_slave(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	int ret;
	struct bonding *bond = to_bond(d);
	char ifname[IFNAMSIZ];
	struct net_device *dev;

	if (!rtnl_trylock())
		return restart_syscall();

	sscanf(buf, "%15s", ifname); /* IFNAMSIZ */
	if (!strlen(ifname) || buf[0] == '\n') {
		dev = NULL;
	} else {
		dev = __dev_get_by_name(dev_net(bond->dev), ifname);
		if (!dev) {
			ret = -ENODEV;
			goto out;
		}
	}

	ret = bond_option_active_slave_set(bond, dev);
	if (!ret)
		ret = count;

 out:
	rtnl_unlock();

	return ret;

}
static DEVICE_ATTR(active_slave, S_IRUGO | S_IWUSR,
		   bonding_show_active_slave, bonding_store_active_slave);


/*
 * Show link status of the bond interface.
 */
static ssize_t bonding_show_mii_status(struct device *d,
				       struct device_attribute *attr,
				       char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s\n", bond->curr_active_slave ? "up" : "down");
}
static DEVICE_ATTR(mii_status, S_IRUGO, bonding_show_mii_status, NULL);

/*
 * Show current 802.3ad aggregator ID.
 */
static ssize_t bonding_show_ad_aggregator(struct device *d,
					  struct device_attribute *attr,
					  char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				bond_3ad_get_active_agg_info(bond, &ad_info)
				?  0 : ad_info.aggregator_id);
	}

	return count;
}
static DEVICE_ATTR(ad_aggregator, S_IRUGO, bonding_show_ad_aggregator, NULL);


/*
 * Show number of active 802.3ad ports.
 */
static ssize_t bonding_show_ad_num_ports(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				bond_3ad_get_active_agg_info(bond, &ad_info)
				?  0 : ad_info.ports);
	}

	return count;
}
static DEVICE_ATTR(ad_num_ports, S_IRUGO, bonding_show_ad_num_ports, NULL);


/*
 * Show current 802.3ad actor key.
 */
static ssize_t bonding_show_ad_actor_key(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				bond_3ad_get_active_agg_info(bond, &ad_info)
				?  0 : ad_info.actor_key);
	}

	return count;
}
static DEVICE_ATTR(ad_actor_key, S_IRUGO, bonding_show_ad_actor_key, NULL);


/*
 * Show current 802.3ad partner key.
 */
static ssize_t bonding_show_ad_partner_key(struct device *d,
					   struct device_attribute *attr,
					   char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				bond_3ad_get_active_agg_info(bond, &ad_info)
				?  0 : ad_info.partner_key);
	}

	return count;
}
static DEVICE_ATTR(ad_partner_key, S_IRUGO, bonding_show_ad_partner_key, NULL);


/*
 * Show current 802.3ad partner mac.
 */
static ssize_t bonding_show_ad_partner_mac(struct device *d,
					   struct device_attribute *attr,
					   char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		if (!bond_3ad_get_active_agg_info(bond, &ad_info))
			count = sprintf(buf, "%pM\n", ad_info.partner_system);
	}

	return count;
}
static DEVICE_ATTR(ad_partner_mac, S_IRUGO, bonding_show_ad_partner_mac, NULL);

/*
 * Show the queue_ids of the slaves in the current bond.
 */
static ssize_t bonding_show_queue_id(struct device *d,
				     struct device_attribute *attr,
				     char *buf)
{
	struct bonding *bond = to_bond(d);
	struct list_head *iter;
	struct slave *slave;
	int res = 0;

	if (!rtnl_trylock())
		return restart_syscall();

	bond_for_each_slave(bond, slave, iter) {
		if (res > (PAGE_SIZE - IFNAMSIZ - 6)) {
			/* not enough space for another interface_name:queue_id pair */
			if ((PAGE_SIZE - res) > 10)
				res = PAGE_SIZE - 10;
			res += sprintf(buf + res, "++more++ ");
			break;
		}
		res += sprintf(buf + res, "%s:%d ",
			       slave->dev->name, slave->queue_id);
	}
	if (res)
		buf[res-1] = '\n'; /* eat the leftover space */

	rtnl_unlock();

	return res;
}

/*
 * Set the queue_ids of the  slaves in the current bond.  The bond
 * interface must be enslaved for this to work.
 */
static ssize_t bonding_store_queue_id(struct device *d,
				      struct device_attribute *attr,
				      const char *buffer, size_t count)
{
	struct slave *slave, *update_slave;
	struct bonding *bond = to_bond(d);
	struct list_head *iter;
	u16 qid;
	int ret = count;
	char *delim;
	struct net_device *sdev = NULL;

	if (!rtnl_trylock())
		return restart_syscall();

	/* delim will point to queue id if successful */
	delim = strchr(buffer, ':');
	if (!delim)
		goto err_no_cmd;

	/*
	 * Terminate string that points to device name and bump it
	 * up one, so we can read the queue id there.
	 */
	*delim = '\0';
	if (sscanf(++delim, "%hd\n", &qid) != 1)
		goto err_no_cmd;

	/* Check buffer length, valid ifname and queue id */
	if (strlen(buffer) > IFNAMSIZ ||
	    !dev_valid_name(buffer) ||
	    qid > bond->dev->real_num_tx_queues)
		goto err_no_cmd;

	/* Get the pointer to that interface if it exists */
	sdev = __dev_get_by_name(dev_net(bond->dev), buffer);
	if (!sdev)
		goto err_no_cmd;

	/* Search for thes slave and check for duplicate qids */
	update_slave = NULL;
	bond_for_each_slave(bond, slave, iter) {
		if (sdev == slave->dev)
			/*
			 * We don't need to check the matching
			 * slave for dups, since we're overwriting it
			 */
			update_slave = slave;
		else if (qid && qid == slave->queue_id) {
			goto err_no_cmd;
		}
	}

	if (!update_slave)
		goto err_no_cmd;

	/* Actually set the qids for the slave */
	update_slave->queue_id = qid;

out:
	rtnl_unlock();
	return ret;

err_no_cmd:
	pr_info("invalid input for queue_id set for %s.\n",
		bond->dev->name);
	ret = -EPERM;
	goto out;
}

static DEVICE_ATTR(queue_id, S_IRUGO | S_IWUSR, bonding_show_queue_id,
		   bonding_store_queue_id);


/*
 * Show and set the all_slaves_active flag.
 */
static ssize_t bonding_show_slaves_active(struct device *d,
					  struct device_attribute *attr,
					  char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.all_slaves_active);
}

static ssize_t bonding_store_slaves_active(struct device *d,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int new_value, ret;

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err("%s: no all_slaves_active value specified.\n",
		       bond->dev->name);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_all_slaves_active_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}
static DEVICE_ATTR(all_slaves_active, S_IRUGO | S_IWUSR,
		   bonding_show_slaves_active, bonding_store_slaves_active);

/*
 * Show and set the number of IGMP membership reports to send on link failure
 */
static ssize_t bonding_show_resend_igmp(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.resend_igmp);
}

static ssize_t bonding_store_resend_igmp(struct device *d,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err("%s: no resend_igmp value specified.\n",
		       bond->dev->name);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_resend_igmp_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}

static DEVICE_ATTR(resend_igmp, S_IRUGO | S_IWUSR,
		   bonding_show_resend_igmp, bonding_store_resend_igmp);


static ssize_t bonding_show_lp_interval(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	struct bonding *bond = to_bond(d);
	return sprintf(buf, "%d\n", bond->params.lp_interval);
}

static ssize_t bonding_store_lp_interval(struct device *d,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int new_value, ret;

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err("%s: no lp interval value specified.\n",
			bond->dev->name);
		return -EINVAL;
	}

	if (!rtnl_trylock())
		return restart_syscall();

	ret = bond_option_lp_interval_set(bond, new_value);
	if (!ret)
		ret = count;

	rtnl_unlock();
	return ret;
}

static DEVICE_ATTR(lp_interval, S_IRUGO | S_IWUSR,
		   bonding_show_lp_interval, bonding_store_lp_interval);

static ssize_t bonding_show_packets_per_slave(struct device *d,
					      struct device_attribute *attr,
					      char *buf)
{
	struct bonding *bond = to_bond(d);
	unsigned int packets_per_slave = bond->params.packets_per_slave;
	return sprintf(buf, "%u\n", packets_per_slave);
}

static ssize_t bonding_store_packets_per_slave(struct device *d,
					       struct device_attribute *attr,
					       const char *buf, size_t count)
{
	struct bonding *bond = to_bond(d);
	int ret;

	ret = bond_opt_tryset_rtnl(bond, BOND_OPT_PACKETS_PER_SLAVE,
				   (char *)buf);
	if (!ret)
		ret = count;

	return ret;
}

static DEVICE_ATTR(packets_per_slave, S_IRUGO | S_IWUSR,
		   bonding_show_packets_per_slave,
		   bonding_store_packets_per_slave);

static struct attribute *per_bond_attrs[] = {
	&dev_attr_slaves.attr,
	&dev_attr_mode.attr,
	&dev_attr_fail_over_mac.attr,
	&dev_attr_arp_validate.attr,
	&dev_attr_arp_all_targets.attr,
	&dev_attr_arp_interval.attr,
	&dev_attr_arp_ip_target.attr,
	&dev_attr_downdelay.attr,
	&dev_attr_updelay.attr,
	&dev_attr_lacp_rate.attr,
	&dev_attr_ad_select.attr,
	&dev_attr_xmit_hash_policy.attr,
	&dev_attr_num_grat_arp.attr,
	&dev_attr_num_unsol_na.attr,
	&dev_attr_miimon.attr,
	&dev_attr_primary.attr,
	&dev_attr_primary_reselect.attr,
	&dev_attr_use_carrier.attr,
	&dev_attr_active_slave.attr,
	&dev_attr_mii_status.attr,
	&dev_attr_ad_aggregator.attr,
	&dev_attr_ad_num_ports.attr,
	&dev_attr_ad_actor_key.attr,
	&dev_attr_ad_partner_key.attr,
	&dev_attr_ad_partner_mac.attr,
	&dev_attr_queue_id.attr,
	&dev_attr_all_slaves_active.attr,
	&dev_attr_resend_igmp.attr,
	&dev_attr_min_links.attr,
	&dev_attr_lp_interval.attr,
	&dev_attr_packets_per_slave.attr,
	NULL,
};

static struct attribute_group bonding_group = {
	.name = "bonding",
	.attrs = per_bond_attrs,
};

/*
 * Initialize sysfs.  This sets up the bonding_masters file in
 * /sys/class/net.
 */
int bond_create_sysfs(struct bond_net *bn)
{
	int ret;

	bn->class_attr_bonding_masters = class_attr_bonding_masters;
	sysfs_attr_init(&bn->class_attr_bonding_masters.attr);

	ret = netdev_class_create_file_ns(&bn->class_attr_bonding_masters,
					  bn->net);
	/*
	 * Permit multiple loads of the module by ignoring failures to
	 * create the bonding_masters sysfs file.  Bonding devices
	 * created by second or subsequent loads of the module will
	 * not be listed in, or controllable by, bonding_masters, but
	 * will have the usual "bonding" sysfs directory.
	 *
	 * This is done to preserve backwards compatibility for
	 * initscripts/sysconfig, which load bonding multiple times to
	 * configure multiple bonding devices.
	 */
	if (ret == -EEXIST) {
		/* Is someone being kinky and naming a device bonding_master? */
		if (__dev_get_by_name(bn->net,
				      class_attr_bonding_masters.attr.name))
			pr_err("network device named %s already exists in sysfs",
			       class_attr_bonding_masters.attr.name);
		ret = 0;
	}

	return ret;

}

/*
 * Remove /sys/class/net/bonding_masters.
 */
void bond_destroy_sysfs(struct bond_net *bn)
{
	netdev_class_remove_file_ns(&bn->class_attr_bonding_masters, bn->net);
}

/*
 * Initialize sysfs for each bond.  This sets up and registers
 * the 'bondctl' directory for each individual bond under /sys/class/net.
 */
void bond_prepare_sysfs_group(struct bonding *bond)
{
	bond->dev->sysfs_groups[0] = &bonding_group;
}

