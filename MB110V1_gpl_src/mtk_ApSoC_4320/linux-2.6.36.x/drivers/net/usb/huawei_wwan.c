/*
 * Copyright (C) Bj?rn Mork <bjorn@xxxxxxx>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// #define	DEBUG			// error path messages, extra info
// #define	VERBOSE			// more; success messages

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/usb.h>
#include <linux/usb/cdc.h>
#include <linux/usb/usbnet.h>

static int huawei_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	struct ethhdr *hdr  = (void *)skb->data;
	static unsigned char peer[ETH_ALEN];

	if (hdr->h_proto == htons(ETH_P_IP)){
		memcpy(hdr->h_dest, dev->net->dev_addr, ETH_ALEN);
		return 1;
	}
	if (hdr->h_proto == htons(ETH_P_ARP)) {
		memcpy(hdr->h_dest, dev->net->dev_addr, ETH_ALEN);
		memcpy(peer, hdr->h_source, ETH_ALEN);
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


void huawei_cdc_status(struct usbnet *dev, struct urb *urb)
{
	struct usb_cdc_notification	*event;

	if (urb->actual_length < sizeof *event)
		return;

	event = urb->transfer_buffer;
	switch (event->bNotificationType) {
	case USB_CDC_NOTIFY_RESPONSE_AVAILABLE:
		netdev_dbg(dev->net, "received USB_CDC_NOTIFY_RESPONSE_AVAILABLE\n");
		break;
	default:
		usbnet_cdc_status(dev, urb);
	}
}

static const struct driver_info	huawei_info = {
	.description =	"Vendor specific Ethernet Device",
	.flags =	FLAG_ETHER,
	.bind =		usbnet_cdc_bind,
	.unbind =	usbnet_cdc_unbind,
	.status =	huawei_cdc_status,
	.rx_fixup =	huawei_rx_fixup,
};

#define HUAWEI_VENDOR_ID	0x12D1


static const struct usb_device_id products [] = {
{
	/* Huawei E392, E398, ++? */
	.match_flags    =   USB_DEVICE_ID_MATCH_VENDOR
		 | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor               = HUAWEI_VENDOR_ID,
	.bInterfaceClass	= USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass	= 1,
	.bInterfaceProtocol	= 9,
	.driver_info = (unsigned long)&huawei_info,
}, {
	/* Huawei device id 1413 ++? */
	.match_flags    =   USB_DEVICE_ID_MATCH_VENDOR
		 | USB_DEVICE_ID_MATCH_INT_INFO,
	.idVendor               = HUAWEI_VENDOR_ID,
	.bInterfaceClass	= USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass	= 6,
	.bInterfaceProtocol	= 255,
	.driver_info = (unsigned long)&huawei_info,
},
	{ },		// END
};
MODULE_DEVICE_TABLE(usb, products);

static struct usb_driver huawei_driver = {
	.name =		"huawei_wwan",
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
	.suspend =	usbnet_suspend,
	.resume =	usbnet_resume,
	.reset_resume =	usbnet_resume,
	.supports_autosuspend = 1,
};

static int __init huawei_init(void)
{
	return usb_register(&huawei_driver);
}
module_init(huawei_init);

static void __exit huawei_exit(void)
{
	usb_deregister(&huawei_driver);
}
module_exit(huawei_exit);

MODULE_AUTHOR("Bj?rn Mork");
MODULE_DESCRIPTION("Huawei WWAN driver");
MODULE_LICENSE("GPL");     