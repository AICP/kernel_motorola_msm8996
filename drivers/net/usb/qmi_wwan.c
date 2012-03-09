/*
 * Copyright (c) 2012  Bjørn Mork <bjorn@mork.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/usb/cdc.h>
#include <linux/usb/usbnet.h>
#include <linux/usb/cdc-wdm.h>

/* The name of the CDC Device Management driver */
#define DM_DRIVER "cdc_wdm"

/*
 * This driver supports wwan (3G/LTE/?) devices using a vendor
 * specific management protocol called Qualcomm MSM Interface (QMI) -
 * in addition to the more common AT commands over serial interface
 * management
 *
 * QMI is wrapped in CDC, using CDC encapsulated commands on the
 * control ("master") interface of a two-interface CDC Union
 * resembling standard CDC ECM.  The devices do not use the control
 * interface for any other CDC messages.  Most likely because the
 * management protocol is used in place of the standard CDC
 * notifications NOTIFY_NETWORK_CONNECTION and NOTIFY_SPEED_CHANGE
 *
 * Handling a protocol like QMI is out of the scope for any driver.
 * It can be exported as a character device using the cdc-wdm driver,
 * which will enable userspace applications ("modem managers") to
 * handle it.  This may be required to use the network interface
 * provided by the driver.
 *
 * These devices may alternatively/additionally be configured using AT
 * commands on any of the serial interfaces driven by the option driver
 *
 * This driver binds only to the data ("slave") interface to enable
 * the cdc-wdm driver to bind to the control interface.  It still
 * parses the CDC functional descriptors on the control interface to
 *  a) verify that this is indeed a handled interface (CDC Union
 *     header lists it as slave)
 *  b) get MAC address and other ethernet config from the CDC Ethernet
 *     header
 *  c) enable user bind requests against the control interface, which
 *     is the common way to bind to CDC Ethernet Control Model type
 *     interfaces
 *  d) provide a hint to the user about which interface is the
 *     corresponding management interface
 */

static int qmi_wwan_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int status = -1;
	struct usb_interface *control = NULL;
	u8 *buf = intf->cur_altsetting->extra;
	int len = intf->cur_altsetting->extralen;
	struct usb_interface_descriptor *desc = &intf->cur_altsetting->desc;
	struct usb_cdc_union_desc *cdc_union = NULL;
	struct usb_cdc_ether_desc *cdc_ether = NULL;
	u32 required = 1 << USB_CDC_HEADER_TYPE | 1 << USB_CDC_UNION_TYPE;
	u32 found = 0;
	atomic_t *pmcount = (void *)&dev->data[1];

	atomic_set(pmcount, 0);

	/*
	 * assume a data interface has no additional descriptors and
	 * that the control and data interface are numbered
	 * consecutively - this holds for the Huawei device at least
	 */
	if (len == 0 && desc->bInterfaceNumber > 0) {
		control = usb_ifnum_to_if(dev->udev, desc->bInterfaceNumber - 1);
		if (!control)
			goto err;

		buf = control->cur_altsetting->extra;
		len = control->cur_altsetting->extralen;
		dev_dbg(&intf->dev, "guessing \"control\" => %s, \"data\" => this\n",
			dev_name(&control->dev));
	}

	while (len > 3) {
		struct usb_descriptor_header *h = (void *)buf;

		/* ignore any misplaced descriptors */
		if (h->bDescriptorType != USB_DT_CS_INTERFACE)
			goto next_desc;

		/* buf[2] is CDC descriptor subtype */
		switch (buf[2]) {
		case USB_CDC_HEADER_TYPE:
			if (found & 1 << USB_CDC_HEADER_TYPE) {
				dev_dbg(&intf->dev, "extra CDC header\n");
				goto err;
			}
			if (h->bLength != sizeof(struct usb_cdc_header_desc)) {
				dev_dbg(&intf->dev, "CDC header len %u\n", h->bLength);
				goto err;
			}
			break;
		case USB_CDC_UNION_TYPE:
			if (found & 1 << USB_CDC_UNION_TYPE) {
				dev_dbg(&intf->dev, "extra CDC union\n");
				goto err;
			}
			if (h->bLength != sizeof(struct usb_cdc_union_desc)) {
				dev_dbg(&intf->dev, "CDC union len %u\n", h->bLength);
				goto err;
			}
			cdc_union = (struct usb_cdc_union_desc *)buf;
			break;
		case USB_CDC_ETHERNET_TYPE:
			if (found & 1 << USB_CDC_ETHERNET_TYPE) {
				dev_dbg(&intf->dev, "extra CDC ether\n");
				goto err;
			}
			if (h->bLength != sizeof(struct usb_cdc_ether_desc)) {
				dev_dbg(&intf->dev, "CDC ether len %u\n",  h->bLength);
				goto err;
			}
			cdc_ether = (struct usb_cdc_ether_desc *)buf;
			break;
		}

		/*
		 * Remember which CDC functional descriptors we've seen.  Works
		 * for all types we care about, of which USB_CDC_ETHERNET_TYPE
		 * (0x0f) is the highest numbered
		 */
		if (buf[2] < 32)
			found |= 1 << buf[2];

next_desc:
		len -= h->bLength;
		buf += h->bLength;
	}

	/* did we find all the required ones? */
	if ((found & required) != required) {
		dev_err(&intf->dev, "CDC functional descriptors missing\n");
		goto err;
	}

	/* give the user a helpful hint if trying to bind to the wrong interface */
	if (cdc_union && desc->bInterfaceNumber == cdc_union->bMasterInterface0) {
		dev_err(&intf->dev, "leaving \"control\" interface for " DM_DRIVER " - try binding to %s instead!\n",
			dev_name(&usb_ifnum_to_if(dev->udev, cdc_union->bSlaveInterface0)->dev));
		goto err;
	}

	/* errors aren't fatal - we can live with the dynamic address */
	if (cdc_ether) {
		dev->hard_mtu = le16_to_cpu(cdc_ether->wMaxSegmentSize);
		usbnet_get_ethernet_addr(dev, cdc_ether->iMACAddress);
	}

	/* success! point the user to the management interface */
	if (control)
		dev_info(&intf->dev, "Use \"" DM_DRIVER "\" for QMI interface %s\n",
			dev_name(&control->dev));

	/* XXX: add a sysfs symlink somewhere to help management applications find it? */

	/* collect bulk endpoints now that we know intf == "data" interface */
	status = usbnet_get_endpoints(dev, intf);

err:
	return status;
}

/* using a counter to merge subdriver requests with our own into a combined state */
static int qmi_wwan_manage_power(struct usbnet *dev, int on)
{
	atomic_t *pmcount = (void *)&dev->data[1];
	int rv = 0;

	dev_dbg(&dev->intf->dev, "%s() pmcount=%d, on=%d\n", __func__, atomic_read(pmcount), on);

	if ((on && atomic_add_return(1, pmcount) == 1) || (!on && atomic_dec_and_test(pmcount))) {
		/* need autopm_get/put here to ensure the usbcore sees the new value */
		rv = usb_autopm_get_interface(dev->intf);
		if (rv < 0)
			goto err;
		dev->intf->needs_remote_wakeup = on;
		usb_autopm_put_interface(dev->intf);
	}
err:
	return rv;
}

static int qmi_wwan_cdc_wdm_manage_power(struct usb_interface *intf, int on)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	return qmi_wwan_manage_power(dev, on);
}

/* Some devices combine the "control" and "data" functions into a
 * single interface with all three endpoints: interrupt + bulk in and
 * out
 *
 * Setting up cdc-wdm as a subdriver owning the interrupt endpoint
 * will let it provide userspace access to the encapsulated QMI
 * protocol without interfering with the usbnet operations.
  */
static int qmi_wwan_bind_shared(struct usbnet *dev, struct usb_interface *intf)
{
	int rv;
	struct usb_driver *subdriver = NULL;
	atomic_t *pmcount = (void *)&dev->data[1];

	atomic_set(pmcount, 0);

	/* collect all three endpoints */
	rv = usbnet_get_endpoints(dev, intf);
	if (rv < 0)
		goto err;

	/* require interrupt endpoint for subdriver */
	if (!dev->status) {
		rv = -EINVAL;
		goto err;
	}

	subdriver = usb_cdc_wdm_register(intf, &dev->status->desc, 512, &qmi_wwan_cdc_wdm_manage_power);
	if (IS_ERR(subdriver)) {
		rv = PTR_ERR(subdriver);
		goto err;
	}

	/* can't let usbnet use the interrupt endpoint */
	dev->status = NULL;

	/* save subdriver struct for suspend/resume wrappers */
	dev->data[0] = (unsigned long)subdriver;

err:
	return rv;
}

static void qmi_wwan_unbind_shared(struct usbnet *dev, struct usb_interface *intf)
{
	struct usb_driver *subdriver = (void *)dev->data[0];

	if (subdriver && subdriver->disconnect)
		subdriver->disconnect(intf);

	dev->data[0] = (unsigned long)NULL;
}

/* suspend/resume wrappers calling both usbnet and the cdc-wdm
 * subdriver if present.
 *
 * NOTE: cdc-wdm also supports pre/post_reset, but we cannot provide
 * wrappers for those without adding usbnet reset support first.
 */
static int qmi_wwan_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct usb_driver *subdriver = (void *)dev->data[0];
	int ret;

	ret = usbnet_suspend(intf, message);
	if (ret < 0)
		goto err;

	if (subdriver && subdriver->suspend)
		ret = subdriver->suspend(intf, message);
	if (ret < 0)
		usbnet_resume(intf);
err:
	return ret;
}

static int qmi_wwan_resume(struct usb_interface *intf)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct usb_driver *subdriver = (void *)dev->data[0];
	int ret = 0;

	if (subdriver && subdriver->resume)
		ret = subdriver->resume(intf);
	if (ret < 0)
		goto err;
	ret = usbnet_resume(intf);
	if (ret < 0 && subdriver && subdriver->resume && subdriver->suspend)
		subdriver->suspend(intf, PMSG_SUSPEND);
err:
	return ret;
}


static const struct driver_info	qmi_wwan_info = {
	.description	= "QMI speaking wwan device",
	.flags		= FLAG_WWAN,
	.bind		= qmi_wwan_bind,
	.manage_power	= qmi_wwan_manage_power,
};

static const struct driver_info	qmi_wwan_shared = {
	.description	= "QMI speaking wwan device with combined interface",
	.flags		= FLAG_WWAN,
	.bind		= qmi_wwan_bind_shared,
	.unbind		= qmi_wwan_unbind_shared,
	.manage_power	= qmi_wwan_manage_power,
};

#define HUAWEI_VENDOR_ID	0x12D1

static const struct usb_device_id products[] = {
	{	/* Huawei E392, E398 and possibly others sharing both device id and more... */
		.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = HUAWEI_VENDOR_ID,
		.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
		.bInterfaceSubClass = 1,
		.bInterfaceProtocol = 8, /* NOTE: This is the *slave* interface of the CDC Union! */
		.driver_info        = (unsigned long)&qmi_wwan_info,
	},
	{	/* Huawei E392, E398 and possibly others in "Windows mode"
		 * using a combined control and data interface without any CDC
		 * functional descriptors
		 */
		.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = HUAWEI_VENDOR_ID,
		.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
		.bInterfaceSubClass = 1,
		.bInterfaceProtocol = 17,
		.driver_info        = (unsigned long)&qmi_wwan_shared,
	},
	{ }	/* END */
};
MODULE_DEVICE_TABLE(usb, products);

static struct usb_driver qmi_wwan_driver = {
	.name		      = "qmi_wwan",
	.id_table	      = products,
	.probe		      =	usbnet_probe,
	.disconnect	      = usbnet_disconnect,
	.suspend	      = qmi_wwan_suspend,
	.resume		      =	qmi_wwan_resume,
	.reset_resume         = qmi_wwan_resume,
	.supports_autosuspend = 1,
};

static int __init qmi_wwan_init(void)
{
	return usb_register(&qmi_wwan_driver);
}
module_init(qmi_wwan_init);

static void __exit qmi_wwan_exit(void)
{
	usb_deregister(&qmi_wwan_driver);
}
module_exit(qmi_wwan_exit);

MODULE_AUTHOR("Bjørn Mork <bjorn@mork.no>");
MODULE_DESCRIPTION("Qualcomm MSM Interface (QMI) WWAN driver");
MODULE_LICENSE("GPL");
