/*
 * Copyright (c) 2012  Bj?rn Mork <bjorn@xxxxxxx>
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
#include "cdc_enc.h"
#include "qmi_proto.h"

static char *apnName  = "default";
module_param(apnName, charp, 0);
MODULE_PARM_DESC(apnName, "User specified APN name");

static char *devName  = "default";
module_param(devName, charp, 0);
MODULE_PARM_DESC(devName, "User specified DEV name");

static int  authType = -1;
module_param(authType, int, 0);
MODULE_PARM_DESC(authType, "User specified Auth type");


static int huawei_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	struct ethhdr *hdr  = (void *)skb->data;
	static unsigned char peer[ETH_ALEN];

	if (hdr->h_proto == htons(ETH_P_IP)){
		memcpy(hdr->h_dest, dev->net->dev_addr, ETH_ALEN);
		return 1;
	}	

	if (hdr->h_proto == htons(ETH_P_ARP)) {
		memcpy(peer, hdr->h_source, ETH_ALEN);
		memcpy(hdr->h_dest, dev->net->dev_addr, ETH_ALEN);
		netdev_dbg(dev->net, "updating peer address to %pM\n", peer);
		return 1;
	}

//	netdev_dbg(dev->net, "hdr->h_proto=%04x\n", ntohs(hdr->h_proto));

	/* add room for ethernet header */
	if (pskb_expand_head(skb, sizeof(*hdr), 0, GFP_ATOMIC) < 0)
		return 0;

	hdr = (void *)skb_push(skb, sizeof(*hdr));
	memcpy(hdr->h_dest, dev->net->dev_addr, ETH_ALEN);
	memcpy(hdr->h_source, peer, ETH_ALEN);
	hdr->h_proto = htons(ETH_P_IP);

	return 1;
}
/*
 * Overloading the cdc_state structure...
 *
 * We use usbnet_cdc_bind() because that suits us well, but it maps
 * the whole struct usbnet "data" field to it's own struct cdc_state.
 * The three functional descriptor pointers are however not used after
 * bind, so we remap the "data" field to our own structure with a
 * pointer to the cdc_enc subdriver instead of the CDC header descriptor
 */
struct qmi_wwan_state {
	/* replacing "struct usb_cdc_header_desc *header" */
	struct cdc_enc_client		*wwan;

	/* keeping these for now */
	struct usb_cdc_union_desc	*u;
	struct usb_cdc_ether_desc	*ether;

	/* the rest *must* be identical to "struct cdc_state" */
	struct usb_interface		*control;
	struct usb_interface		*data;
};

/* callback doing the parsing for us */
static void qmux_received_callback(struct work_struct *work)
{
	struct cdc_enc_client *client = container_of(work, struct cdc_enc_client, work);

	qmi_parse_qmux(client);
}

/* we have two additional requirements for usbnet_cdc_status():
 *  1) handle USB_CDC_NOTIFY_RESPONSE_AVAILABLE
 *  2) resubmit interrupt urb even if the ethernet interface is down
 */
static void qmi_wwan_cdc_status(struct usbnet *dev, struct urb *urb)
{
	struct qmi_wwan_state *info = (void *)&dev->data;
	struct usb_cdc_notification *event;

	if (urb->actual_length < sizeof(*event)) {
		netdev_dbg(dev->net, "short status urb rcvd: %d bytes\n", urb->actual_length);
		return;
	}

	event = urb->transfer_buffer;
	switch (event->bNotificationType) {
	case USB_CDC_NOTIFY_RESPONSE_AVAILABLE:
		/* the device has QMI data for us => submit read URB */
		cdc_enc_submit_readurb(info->wwan->cdc_enc, GFP_ATOMIC);
		break;
	default:
		/* let usbnet_cdc_status() handle the other CDC messages */
		usbnet_cdc_status(dev, urb);
	}

	/* usbnet won't resubmit unless netif is running */
	if (!netif_running(dev->net))
		usb_submit_urb(urb, GFP_ATOMIC);
}

/* we need to do some work after binding, but possibly before usbnet will
 * call any of our other methods
 */
static struct qmi_wwan_delayed_type {
	struct delayed_work dwork;
	struct usbnet *dev;
} qmi_wwan_delayed;

static void qmi_wwan_deferred_bind(struct work_struct *work)
{
	struct qmi_wwan_delayed_type *dtype = container_of(work, struct qmi_wwan_delayed_type, dwork.work);
	struct qmi_wwan_state *info = (void *)&dtype->dev->data;

	/* if not ready yet, then wait and retry */
	if (!dtype->dev->interrupt) {
		schedule_delayed_work(&qmi_wwan_delayed.dwork, 100);
		return;
	}

	/* usbnet will happily kill the interrupt status URB when it
	 * doesn't need it.  Let cdc_enc submit the URB in cases where
	 * it needs the interrupts. Hack alert!
	 */
	cdc_enc_set_interrupt(info->wwan->cdc_enc, dtype->dev->interrupt);
}

static int qmi_wwan_cdc_bind(struct usbnet *dev, struct usb_interface *intf)
{
	struct qmi_wwan_state *info = (void *)&dev->data;
	struct cdc_enc_state *cdc_enc;
	int status;

	status = usbnet_cdc_bind(dev, intf);
	if (status < 0) {
		/* Some Huawei devices have multiple sets of
		 * descriptors, selectable by mode switching.  Some of
		 * these sets, like the one used by Windows, do not
		 * have the CDC functional descriptors required by
		 * usbnet_cdc_bind().  But they still provide the
		 * exact same functionality on a single combined
		 * interface, with the missing MAC address string
		 * descriptor being the only exception.
		 */
		dev_info(&intf->dev, "no CDC descriptors - will generate a MAC address\n");
		status = usbnet_get_endpoints(dev, intf);
	}
	if (status < 0)
		goto err_cdc_bind;

	/* ZTE makes devices where the interface descriptors and endpoint
	 * configurations of two or more interfaces are identical, even
	 * though the functions are completely different.  If set, then
	 * driver_info->data is a bitmap of acceptable interface numbers
	 * allowing us to bind to one such interface without binding to
	 * all of them
	 */
	if (dev->driver_info->data &&
	    !test_bit(intf->cur_altsetting->desc.bInterfaceNumber, &dev->driver_info->data)) {
		dev_info(&intf->dev, "not on our whitelist - ignored");

		 /* added by ZQQ, 20Jun12, MF820D 出现生成多个LTE接口，是因为返回值不对 */
		status = -1;
		/* end added by ZQQ, 20Jun12 */
		goto err_cdc_bind;
	}

	/* allocate and initiate a QMI device with a client */
	cdc_enc = cdc_enc_init_one(intf, "qmi");
	if (!cdc_enc)
		goto err_cdc_enc;

	/* add the wwan client */
	info->wwan = cdc_enc_add_client(cdc_enc, qmux_received_callback);
	if (!info->wwan)
		goto err_wwan;


	if (0 != strcmp(apnName, "default"))
	{
		strcpy(info->wwan->apnName, apnName);
	}
	else
	{
		strcpy(info->wwan->apnName, "default");
	}
	
	if (0 != strcmp(devName, "default"))
	{
		strcpy(info->wwan->devName, apnName);
	}
	else
	{
		strcpy(info->wwan->devName, "default");
	}
	
	if (-1 != authType)
	{
		 info->wwan->authType =(unsigned char)(authType & 0xFF);
	}

	/* need to run this after usbnet has finished initialisation */
	qmi_wwan_delayed.dev = dev;
	INIT_DELAYED_WORK(&qmi_wwan_delayed.dwork, qmi_wwan_deferred_bind);
	schedule_delayed_work(&qmi_wwan_delayed.dwork, 100);

	dev_info(&intf->dev, "Use one of the ttyUSBx devices to configure PIN code, APN or other required settings\n");

	return 0;

err_wwan:
	cdc_enc_free_one(cdc_enc);
err_cdc_enc:
	/* usbnet_cdc_unbind() makes sure that the driver is unbound
	 * from both the control and data interface.  It will be a
	 * harmless noop if usbnet_cdc_bind() failed above.
	 */
	usbnet_cdc_unbind(dev, intf);
	status = -1;
err_cdc_bind:
	return status;
}

static void qmi_wwan_cdc_unbind(struct usbnet *dev, struct usb_interface *intf)
{
	struct qmi_wwan_state *info = (void *)&dev->data;
	struct cdc_enc_state *cdc_enc;

	/* release allocated client ID - no need to wait for the reply */
	qmi_ctl_release_wds_cid(info->wwan);

	/* release our private structure */
	cdc_enc = info->wwan->cdc_enc ;
	cdc_enc_destroy_client(info->wwan);
	cdc_enc_free_one(cdc_enc);
	info->wwan = NULL;

	/* disconnect from data interface as well, if there is one */
	usbnet_cdc_unbind(dev, intf);
}

static int qmi_wwan_reset(struct usbnet *dev)
{
	struct qmi_wwan_state *info = (void *)&dev->data;

	/* reset QMI state to a something sane */
	qmi_state_reset(info->wwan);

	/* trigger client ID allocation if needed */
	qmi_ctl_request_wds_cid(info->wwan);

	return 1;
}

static int qmi_wwan_stop(struct usbnet *dev)
{
	struct qmi_wwan_state *info = (void *)&dev->data;

	/* disconnect from mobile network */
	qmi_wds_stop(info->wwan);
	return 1;
}

static int qmi_wwan_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct qmi_wwan_state *info = (void *)&dev->data;

	/* kill the read URB */
	if (!dev->suspend_count)
		cdc_enc_kill_readurb(info->wwan->cdc_enc);

	return usbnet_suspend(intf, message);
}

static int qmi_wwan_resume(struct usb_interface *intf)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	int ret;

	ret = usbnet_resume(intf);

	/* resume interrupt URB in case usbnet didn't do it */
	if (!dev->suspend_count && dev->interrupt && !test_bit(EVENT_DEV_OPEN, &dev->flags))
		usb_submit_urb(dev->interrupt, GFP_NOIO);

	return ret;
}

/* stolen from cdc_ether.c */
static int qmi_wwan_manage_power(struct usbnet *dev, int on)
{
	dev->intf->needs_remote_wakeup = on;
	return 0;
}

/* abusing check_connect for triggering QMI state transitions */
static int qmi_wwan_check_connect(struct usbnet *dev)
{
	struct qmi_wwan_state *info = (void *)&dev->data;
	struct cdc_enc_client *wwan = info->wwan;
	struct qmi_state *qmi = (void *)&wwan->priv;

	/* reset was supposed to allocate a CID - not way out if that failed! */
	if (qmi->flags & QMI_STATE_FLAG_CIDERR) {
		netdev_info(dev->net, "QMI_WDS client ID allocation failed - cannot continue\n");
		return -1;
	}

	switch (qmi->wds_status) {
	case 0: /* no status set yet - trigger an update */
		qmi_wds_status(wwan);
		break;
	case 1: /* disconnected - trigger a connection */
		qmi_wds_start(wwan);
		break;
	case 2: /* connected */
		return 0;
	/* other states may indicate connection in progress - do nothing */
	}

	/* usbnet_open() will fail unless we kill their URB here */
	if (!test_bit(EVENT_DEV_OPEN, &dev->flags))
		usb_kill_urb(dev->interrupt);

	return 1;  /* "not connected" */
}

static const struct driver_info	qmi_wwan_info = {
	.description	= "QMI speaking wwan device",
	.flags		= FLAG_WWAN,
	.bind		= qmi_wwan_cdc_bind,
	.unbind		= qmi_wwan_cdc_unbind,
	.status		= qmi_wwan_cdc_status,
	.manage_power	= qmi_wwan_manage_power,
	.check_connect	= qmi_wwan_check_connect,
	.stop		= qmi_wwan_stop,
	.reset		= qmi_wwan_reset,
	.rx_fixup =	huawei_rx_fixup,
};
static const struct driver_info	qmi_wwan_force_int4 = {
	.description	= "QMI speaking wwan device",
	.flags		= FLAG_WWAN,
	.bind		= qmi_wwan_cdc_bind,
	.unbind		= qmi_wwan_cdc_unbind,
	.status		= qmi_wwan_cdc_status,
	.manage_power	= qmi_wwan_manage_power,
	.check_connect	= qmi_wwan_check_connect,
	.stop		= qmi_wwan_stop,
	.reset		= qmi_wwan_reset,
	.rx_fixup =	huawei_rx_fixup,
	.data		= BIT(4), /* interface whitelist bitmap */
};
static const struct driver_info	qmi_wwan_force_int2 = {
	.description	= "QMI speaking wwan device",
	.flags		= FLAG_WWAN,
	.bind		= qmi_wwan_cdc_bind,
	.unbind		= qmi_wwan_cdc_unbind,
	.status		= qmi_wwan_cdc_status,
	.manage_power	= qmi_wwan_manage_power,
	.check_connect	= qmi_wwan_check_connect,
	.stop		= qmi_wwan_stop,
	.reset		= qmi_wwan_reset,
	.rx_fixup =	huawei_rx_fixup,
	.data		= BIT(2), /* interface whitelist bitmap */
};

static const struct driver_info	qmi_wwan_force_int3 = {
	.description	= "QMI speaking wwan device",
	.flags		= FLAG_WWAN,
	.bind		= qmi_wwan_cdc_bind,
	.unbind		= qmi_wwan_cdc_unbind,
	.status		= qmi_wwan_cdc_status,
	.manage_power	= qmi_wwan_manage_power,
	.check_connect	= qmi_wwan_check_connect,
	.stop		= qmi_wwan_stop,
	.reset		= qmi_wwan_reset,
	.rx_fixup =	huawei_rx_fixup,
	.data		= BIT(3), /* interface whitelist bitmap */
};

#define HUAWEI_VENDOR_ID	0x12D1

#define QMI_GOBI_DEVICE(vend, prod) \
	USB_DEVICE(vend, prod), \
	.driver_info = (unsigned long)&qmi_wwan_info
	
static const struct usb_device_id products[] = {
{
	/* Qmi_Wwan E392, E398, ++? */
	.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor           = HUAWEI_VENDOR_ID,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 9,
	.driver_info        = (unsigned long)&qmi_wwan_info,
}, {
	/* Qmi_Wwan device id 1413 ++? */
	.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor           = HUAWEI_VENDOR_ID,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 6,
	.bInterfaceProtocol = 255,
	.driver_info        = (unsigned long)&qmi_wwan_info,
}, {
	/* Qmi_Wwan E392, E398, ++? "Windows mode" using a combined
	 * control and data interface without any CDC functional
	 * descriptors */
	.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor           = HUAWEI_VENDOR_ID,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 7,
	.driver_info	    = (unsigned long)&qmi_wwan_info,
},{
	/* Qmi_Wwan E392, E398, ++? "Windows mode" using a combined
	 * control and data interface without any CDC functional
	 * descriptors */
	.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor           = HUAWEI_VENDOR_ID,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 17,
	.driver_info	    = (unsigned long)&qmi_wwan_info,
},{
	/* Qmi_Wwan E392, E398, ++? "Windows mode" using a combined
	 * control and data interface without any CDC functional
	 * descriptors */
	.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor           = HUAWEI_VENDOR_ID,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 11,
	.driver_info	    = (unsigned long)&qmi_wwan_info,
},{
	/* Qmi_Wwan E392, E398, ++? "Windows mode" using a combined
	 * control and data interface without any CDC functional
	 * descriptors */
	.match_flags        = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor           = HUAWEI_VENDOR_ID,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 0x67,
	.driver_info	    = (unsigned long)&qmi_wwan_info,
},{	/* Pantech UML290 */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x106c,
		.idProduct          = 0x3718,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0xf0,
		.bInterfaceProtocol = 0xff,
		.driver_info        = (unsigned long)&qmi_wwan_info,
	},
{	/* Pantech UML290 */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x106c,
		.idProduct          = 0x3718,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0xf1,
		.bInterfaceProtocol = 0xff,
		.driver_info        = (unsigned long)&qmi_wwan_info,
	},
	{	/* ZTE MF820D */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x19d2,
		.idProduct          = 0x0167,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0xff,
		.bInterfaceProtocol = 0xff,
		.driver_info        = (unsigned long)&qmi_wwan_force_int4,
	},
	{	/* ZTE MF821D O2 */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x19d2,
		.idProduct          = 0x0326,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0xff,
		.bInterfaceProtocol = 0xff,
		.driver_info        = (unsigned long)&qmi_wwan_force_int4,
	},
	{	/* Quanta 1K3 */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x0408,
		.idProduct          = 0xea26,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0xff,
		.bInterfaceProtocol = 0xff,
		.driver_info        = (unsigned long)&qmi_wwan_force_int3,
	},
	{	/* ZTE (Vodafone) K5006-Z */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x19d2,
		.idProduct          = 0x1018,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0x06,
		.bInterfaceProtocol = 0x00,
		.driver_info        = (unsigned long)&qmi_wwan_force_int3,
	},
	{	/*  Telekom Speedstick LTE II (Alcatel One Touch L100V LTE) */
		.match_flags	    = USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor           = 0x1bbb,
		.idProduct          = 0x011e,
		.bInterfaceClass    = 0xff,
		.bInterfaceSubClass = 0xff,
		.bInterfaceProtocol = 0xff,
		.driver_info        = (unsigned long)&qmi_wwan_force_int4,
	},


	{QMI_GOBI_DEVICE(0x05c6, 0x9212)},	/* Acer Gobi Modem Device */
	{QMI_GOBI_DEVICE(0x03f0, 0x1f1d)},	/* HP un2400 Gobi Modem Device */
	{QMI_GOBI_DEVICE(0x03f0, 0x371d)},	/* HP un2430 Mobile Broadband Module */
	{QMI_GOBI_DEVICE(0x04da, 0x250d)},	/* Panasonic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x413c, 0x8172)},	/* Dell Gobi Modem device */
	{QMI_GOBI_DEVICE(0x1410, 0xa001)},	/* Novatel Gobi Modem device */
	{QMI_GOBI_DEVICE(0x0b05, 0x1776)},	/* Asus Gobi Modem device */
	{QMI_GOBI_DEVICE(0x19d2, 0xfff3)},	/* ONDA Gobi Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9001)},	/* Generic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9002)},	/* Generic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9202)},	/* Generic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9203)},	/* Generic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9222)},	/* Generic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9009)},	/* Generic Gobi Modem device */
	{QMI_GOBI_DEVICE(0x413c, 0x8186)},	/* Dell Gobi 2000 Modem device (N0218, VU936) */
	{QMI_GOBI_DEVICE(0x05c6, 0x920b)},	/* Generic Gobi 2000 Modem device */
	{QMI_GOBI_DEVICE(0x05c6, 0x9225)},	/* Sony Gobi 2000 Modem device (N0279, VU730) */
	{QMI_GOBI_DEVICE(0x05c6, 0x9245)},	/* Samsung Gobi 2000 Modem device (VL176) */
	{QMI_GOBI_DEVICE(0x03f0, 0x251d)},	/* HP Gobi 2000 Modem device (VP412) */
	{QMI_GOBI_DEVICE(0x05c6, 0x9215)},	/* Acer Gobi 2000 Modem device (VP413) */
	{QMI_GOBI_DEVICE(0x05c6, 0x9265)},	/* Asus Gobi 2000 Modem device (VR305) */
	{QMI_GOBI_DEVICE(0x05c6, 0x9235)},	/* Top Global Gobi 2000 Modem device (VR306) */
	{QMI_GOBI_DEVICE(0x05c6, 0x9275)},	/* iRex Technologies Gobi 2000 Modem device (VR307) */
	{QMI_GOBI_DEVICE(0x1199, 0x9001)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9002)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9003)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9004)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9005)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9006)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9007)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9008)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9009)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x900a)},	/* Sierra Wireless Gobi 2000 Modem device (VT773) */
	{QMI_GOBI_DEVICE(0x1199, 0x9011)},	/* Sierra Wireless Gobi 2000 Modem device (MC8305) */
	{QMI_GOBI_DEVICE(0x16d8, 0x8002)},	/* CMDTech Gobi 2000 Modem device (VU922) */
	{QMI_GOBI_DEVICE(0x05c6, 0x9205)},	/* Gobi 2000 Modem device */
	{QMI_GOBI_DEVICE(0x1199, 0x9013)},	/* Sierra Wireless Gobi 3000 Modem device (MC8355) */
{ },		/* END */
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
	/* we remap struct (cdc_state) so we should be compatible */
	BUILD_BUG_ON(sizeof(struct cdc_state) != sizeof(struct qmi_wwan_state) ||
		offsetof(struct cdc_state, control) != offsetof(struct qmi_wwan_state, control) ||
		offsetof(struct cdc_state, data) != offsetof(struct qmi_wwan_state, data));

	//BUILD_BUG_ON(sizeof(struct qmi_state) > sizeof(unsigned long));

	return usb_register(&qmi_wwan_driver);
}
module_init(qmi_wwan_init);

static void __exit qmi_wwan_exit(void)
{
	usb_deregister(&qmi_wwan_driver);
}
module_exit(qmi_wwan_exit);

MODULE_AUTHOR("Bj?rn Mork <bjorn@xxxxxxx>");
MODULE_DESCRIPTION("Qualcomm MSM Interface (QMI) WWAN driver");
MODULE_LICENSE("GPL");