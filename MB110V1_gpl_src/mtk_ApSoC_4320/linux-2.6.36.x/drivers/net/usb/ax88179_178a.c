/*
 * ASIX AX88179 based USB 3.0 Ethernet Devices
 * Copyright (C) 2003-2005 David Hollis <dhollis@davehollis.com>
 * Copyright (C) 2005 Phil Chang <pchang23@sbcglobal.net>
 * Copyright (c) 2002-2003 TiVo Inc.
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

//#define	DEBUG			// debug messages, extra info

#include <linux/version.h>
//#include <linux/config.h>
#ifdef	CONFIG_USB_DEBUG
#define DEBUG
#endif
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/crc32.h>
#include <linux/if_vlan.h>
#include <linux/usb/usbnet.h>

#include "asix.h"

#define DRV_VERSION	"1.3.0"

static char version[] =
KERN_INFO "ASIX USB Ethernet Adapter:v" DRV_VERSION 
	" " __TIME__ " " __DATE__ "\n"
"    		http://www.asix.com.tw\n";

static int msg_enable = 0;
module_param (msg_enable, int, 0);
MODULE_PARM_DESC (msg_enable, "usbnet msg_enable");


/* ASIX AX88179/178A based USB 3.0/2.0 Gigabit Ethernet Devices */

static int ax88179_read_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			    u16 size, void *data)
{
	return usb_control_msg(
		dev->udev,
		usb_rcvctrlpipe(dev->udev, 0),
		cmd,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		value,
		index,
		data,
		size,
		USB_CTRL_GET_TIMEOUT);
}

static int ax88179_write_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			     u16 size, void *data)
{
	return usb_control_msg(
		dev->udev,
		usb_sndctrlpipe(dev->udev, 0),
		cmd,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		value,
		index,
		data,
		size,
		USB_CTRL_SET_TIMEOUT);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static void ax88179_async_cmd_callback(struct urb *urb, struct pt_regs *regs)
#else
static void ax88179_async_cmd_callback(struct urb *urb)
#endif
{
	struct usb_ctrlrequest *req = (struct usb_ctrlrequest *)urb->context;

	if (urb->status < 0)
		printk(KERN_ERR "ax8817x_async_cmd_callback() failed with %d",
			urb->status);
	kfree(req);
	usb_free_urb(urb);
}

static void
ax88179_write_cmd_async(struct usbnet *dev, u8 cmd, u16 value, u16 index,
				    u16 size, void *data)
{
	struct usb_ctrlrequest *req;
	int status;
	struct urb *urb;

	if ((urb = usb_alloc_urb(0, GFP_ATOMIC)) == NULL) {
		netdev_err(dev->net, "Error allocating URB in write_cmd_async!");
		return;
	}

	if ((req = kmalloc (sizeof (struct usb_ctrlrequest),
			    GFP_ATOMIC)) == NULL) {
		netdev_err(dev->net, "Failed to allocate memory for control request");
		usb_free_urb(urb);
		return;
	}

	req->bRequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	req->bRequest = cmd;
	req->wValue = cpu_to_le16(value);
	req->wIndex = cpu_to_le16(index);
	req->wLength = cpu_to_le16(size);

	usb_fill_control_urb(urb, dev->udev,
			     usb_sndctrlpipe(dev->udev, 0),
			     (void *)req, data, size,
			     ax88179_async_cmd_callback, req);

	if((status = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
		netdev_err(dev->net, "Error submitting the control message: status=%d",
				status);
		kfree(req);
		usb_free_urb(urb);
	}
}


static void ax88179_status(struct usbnet *dev, struct urb *urb)
{
	struct ax88179_int_data *event;
	struct ax88179_data *data = (struct ax88179_data *)&dev->driver_priv;
	int link;

	if (urb->actual_length < 8)
		return;

	event = urb->transfer_buffer;
	link = event->link & AX_INT_PPLS_LINK;
	if (netif_carrier_ok(dev->net) != link) {
		if (link) {
			data->linkup = 1;
	  		   usbnet_defer_kevent (dev, EVENT_LINK_RESET );
		} else {			
			data->linkup = 0;
			netif_carrier_off(dev->net);
		}
		netdev_info(dev->net, "ax8817X - Link status is: %d\n", link);
	}
}

static int ax88179_mdio_read(struct net_device *netdev, int phy_id, int loc)
{
	struct usbnet *dev = netdev_priv(netdev);
	u16 *res;
	u16 ret;

	res = kmalloc (2, GFP_ATOMIC);
	if (!res)
		return 0;

	ax88179_read_cmd(dev, AX_ACCESS_PHY, phy_id, (__u16)loc, 2, res);

	ret = *res & 0xffff;
	kfree (res);

	return ret;
}

/* same as above, but converts resulting value to cpu byte order */

static void
ax88179_mdio_write(struct net_device *netdev, int phy_id, int loc, int val)
{
	struct usbnet *dev = netdev_priv(netdev);
	u16 *res;

	res = kmalloc (2, GFP_ATOMIC);
	if (!res)
		return;
	*res = val;

	ax88179_write_cmd(dev, AX_ACCESS_PHY, phy_id, (__u16)loc, 2, res);

	kfree (res);
}

/* same as above, but converts new value to le16 byte order before writing */

static void
ax88179_mdio_write_le(struct net_device *netdev, int phy_id, int loc, int val)
{
	ax88179_mdio_write( netdev, phy_id, loc, cpu_to_le16(val) );
}

static int ax88179_mdio_read_le(struct net_device *netdev, int phy_id, int loc)
{
	return le16_to_cpu(ax88179_mdio_read(netdev, phy_id, loc));
}

static int ax88179_suspend (struct usb_interface *intf,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10)
			pm_message_t message)
#else
			u32 message)
#endif
{
	struct usbnet *dev = usb_get_intfdata(intf);
	u16 *tmp16;

	usbnet_suspend (intf, message);

	tmp16 = kmalloc (2, GFP_ATOMIC);
	if (!tmp16)
		return 0;

	/* Disable RX path */
	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE, 2, 2, tmp16);
	*tmp16 &= ~AX_MEDIUM_RECEIVE_EN;
	ax88179_write_cmd(dev, AX_ACCESS_MAC,  AX_MEDIUM_STATUS_MODE, 2, 2, tmp16);

	/* Force bz */
	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	*tmp16 |= AX_PHYPWR_RSTCTL_BZ | AX_PHYPWR_RSTCTL_IPRL;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);

	/* change clock */
	*tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, tmp16);

	/* Configure RX control register => stop operation */
	*tmp16 = AX_RX_CTL_STOP;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, tmp16);

	*tmp16 = 0;
	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, tmp16);
	netdev_dbg(dev->net, "Monitor mode 0x%x\n", *tmp16);

	kfree (tmp16);
	return 0;
}

static int ax88179_resume (struct usb_interface *intf)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	u16 *tmp16;

	netif_carrier_off (dev->net);

	tmp16 = kmalloc (2, GFP_ATOMIC);
	if (!tmp16)
		return usbnet_resume (intf);

	/* Power up ethernet PHY */
	*tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	msleep (1);
	*tmp16 = AX_PHYPWR_RSTCTL_IPRL;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	msleep (200);

	/* Ethernet PHY Auto Detach*/
	ax88179_AutoDetach(dev);


	/* change clock */
	ax88179_read_cmd(dev, AX_ACCESS_MAC,  AX_CLK_SELECT, 1, 1, tmp16);
	*tmp16 |= AX_CLK_SELECT_ACS | AX_CLK_SELECT_BCS;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, tmp16);
	msleep(100);

	/* Configure RX control register => start operation */
	*tmp16 = AX_RX_CTL_DROPCRCERR | AX_RX_CTL_IPE | AX_RX_CTL_START |
				AX_RX_CTL_AP | AX_RX_CTL_AMALL | AX_RX_CTL_AB;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, tmp16);

	kfree (tmp16);
	return usbnet_resume (intf);
}

static void
ax88179_get_wol(struct net_device *net, struct ethtool_wolinfo *wolinfo)
{

	struct usbnet *dev = netdev_priv(net);
	u8 *opt;

	wolinfo->supported = 0;
	wolinfo->wolopts = 0;

	opt = kmalloc (1, GFP_KERNEL);
	if (!opt)
		return;

	if (ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, opt) < 0)
		return;

	wolinfo->supported = WAKE_PHY | WAKE_MAGIC;

	if (*opt & AX_MONITOR_MODE_RWLC)
		wolinfo->wolopts |= WAKE_PHY;
	if (*opt & AX_MONITOR_MODE_RWMP)
		wolinfo->wolopts |= WAKE_MAGIC;

	kfree (opt);
}

static int
ax88179_set_wol(struct net_device *net, struct ethtool_wolinfo *wolinfo)
{
	struct usbnet *dev = netdev_priv(net);
	u8 *opt;

	opt = kmalloc (1, GFP_KERNEL);
	if (!opt)
		return -ENOMEM;

	*opt = 0;
	if (ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, opt) < 0)
		return 0;

	if (wolinfo->wolopts & WAKE_PHY)
		*opt |= AX_MONITOR_MODE_RWLC;
	else 
		*opt &= ~AX_MONITOR_MODE_RWLC;

	if (wolinfo->wolopts & WAKE_MAGIC)
		*opt |= AX_MONITOR_MODE_RWMP;
	else 
		*opt &= ~AX_MONITOR_MODE_RWMP;

	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, opt);

	kfree (opt);

	return 0;
}

static int ax88179_get_eeprom_len(struct net_device *net)
{
	return AX_EEPROM_LEN;
}

static int ax88179_get_eeprom(struct net_device *net,
			      struct ethtool_eeprom *eeprom, u8 *data)
{
	struct usbnet *dev = netdev_priv(net);
	u16 *ebuf = (u16 *)data;
	int i;

	/* Crude hack to ensure that we don't overwrite memory
	 * if an odd length is supplied
	 */
	if (eeprom->len % 2)
		return -EINVAL;

	/* ax8817x returns 2 bytes from eeprom on read */
	for (i=0; i < eeprom->len / 2; i++) {
		if (ax88179_read_cmd(dev, AX_ACCESS_EEPROM,
			eeprom->offset + i, 1, 2, &ebuf[i]) < 0)
			return -EINVAL;
	}
	return 0;
}

static void ax88179_get_drvinfo (struct net_device *net,
				 struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	usbnet_get_drvinfo(net, info);
	info->eedump_len = 0x3e;
}

static int ax88179_get_settings(struct net_device *net, struct ethtool_cmd *cmd)
{
	struct usbnet *dev = netdev_priv(net);
	return mii_ethtool_gset(&dev->mii,cmd);
}

static int ax88179_set_settings(struct net_device *net, struct ethtool_cmd *cmd)
{
	struct usbnet *dev = netdev_priv(net);
	return mii_ethtool_sset(&dev->mii,cmd);
}

static int ax88179_ioctl (struct net_device *net, struct ifreq *rq, int cmd)
{
	struct usbnet *dev = netdev_priv(net);
	
	return  generic_mii_ioctl(&dev->mii, if_mii(rq), cmd, NULL);
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
static int ax88179_set_csums(struct usbnet *dev)
{
	struct ax88179_data *ax179_data = (struct ax88179_data *)dev->driver_priv;
	u16 checksum;

	if (ax179_data->checksum & AX_RX_CHECKSUM)
		checksum = AX_RXCOE_DEF_CSUM;
	else
		checksum = 0;

	ax88179_write_cmd (dev, AX_ACCESS_MAC, AX_RXCOE_CTL, 1, 1, &checksum);

	if (ax179_data->checksum & AX_TX_CHECKSUM)
		checksum = AX_TXCOE_DEF_CSUM;
	else
		checksum = 0;

	ax88179_write_cmd (dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, &checksum);

	return 0;
}

static u32 ax88179_get_tx_csum(struct net_device *netdev)
{
	struct usbnet *dev = netdev_priv(netdev);
	struct ax88179_data *ax179_data = (struct ax88179_data *)dev->driver_priv;
	return (ax179_data->checksum & AX_TX_CHECKSUM);
}

static u32 ax88179_get_rx_csum(struct net_device *netdev)
{
	struct usbnet *dev = netdev_priv(netdev);
	struct ax88179_data *ax179_data = (struct ax88179_data *)dev->driver_priv;
	return (ax179_data->checksum & AX_RX_CHECKSUM);
}

static int ax88179_set_rx_csum(struct net_device *netdev, u32 val)
{
	struct usbnet *dev = netdev_priv(netdev);
	struct ax88179_data *ax179_data = (struct ax88179_data *)dev->driver_priv;

	if (val)
		ax179_data->checksum |= AX_RX_CHECKSUM;
	else
		ax179_data->checksum &= ~AX_RX_CHECKSUM;
	return ax88179_set_csums(dev);
}

static int ax88179_set_tx_csum(struct net_device *netdev, u32 val)
{
	struct usbnet *dev = netdev_priv(netdev);
	struct ax88179_data *ax179_data = (struct ax88179_data *)dev->driver_priv;

	if (val)
		ax179_data->checksum |= AX_TX_CHECKSUM;
	else
		ax179_data->checksum &= ~AX_TX_CHECKSUM;

	ethtool_op_set_tx_csum(netdev, val);

	return ax88179_set_csums(dev);
}

static int ax88179_set_tso(struct net_device *netdev, u32 data)
{
	if (data) {
		netdev->features |= NETIF_F_TSO;
		netdev->features |= NETIF_F_TSO6;
	} else {
		netdev->features &= ~NETIF_F_TSO;
		netdev->features &= ~NETIF_F_TSO6;
	}
	
	return 0;
}
#endif
static struct ethtool_ops ax88179_ethtool_ops = {
	.get_drvinfo		= ax88179_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_msglevel		= usbnet_get_msglevel,
	.set_msglevel		= usbnet_set_msglevel,
	.get_wol		= ax88179_get_wol,
	.set_wol		= ax88179_set_wol,
	.get_eeprom_len		= ax88179_get_eeprom_len,
	.get_eeprom		= ax88179_get_eeprom,
	.get_settings		= ax88179_get_settings,
	.set_settings		= ax88179_set_settings,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
	.set_tx_csum		= ax88179_set_tx_csum,
	.get_tx_csum		= ax88179_get_tx_csum,
	.get_rx_csum		= ax88179_get_rx_csum,
	.set_rx_csum		= ax88179_set_rx_csum,
	.get_tso		= ethtool_op_get_tso,
	.set_tso		= ax88179_set_tso,
#endif
};


static void ax88179_set_multicast(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);
	struct ax88179_data *data = (struct ax88179_data *)&dev->driver_priv;
	u8 *m_filter = (u8*) dev->data;
	u16 rx_ctl = (AX_RX_CTL_START | AX_RX_CTL_AB | AX_RX_CTL_IPE);
	int mc_count;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
	mc_count = net->mc_count;
#else
	mc_count = netdev_mc_count (net);
#endif

	if (net->flags & IFF_PROMISC) {
		rx_ctl |= AX_RX_CTL_PRO;
	} else if (net->flags & IFF_ALLMULTI
		   || mc_count > AX_MAX_MCAST) {
		rx_ctl |= AX_RX_CTL_AMALL;
	} else if (mc_count == 0) {
		/* just broadcast and directed */
	} else {
		/* We use the 20 byte dev->data
		 * for our 8 byte filter buffer
		 * to avoid allocating memory that
		 * is tricky to free later */
		u32 crc_bits;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
		struct dev_mc_list *mc_list = net->mc_list;
		int i;

		memset(m_filter, 0, AX_MCAST_FILTER_SIZE);

		/* Build the multicast hash filter. */
		for (i = 0; i < net->mc_count; i++) {
			crc_bits =
			    ether_crc(ETH_ALEN,
				      mc_list->dmi_addr) >> 26;
			*(m_filter + (crc_bits >> 3)) |=
				1 << (crc_bits & 7);
			mc_list = mc_list->next;
		} 
#else
		struct netdev_hw_addr *ha;
		memset(m_filter, 0, AX_MCAST_FILTER_SIZE);
		netdev_for_each_mc_addr (ha, net) {
			crc_bits = ether_crc(ETH_ALEN, ha->addr) >> 26;
			*(m_filter + (crc_bits >> 3)) |=
				1 << (crc_bits & 7);
		}
#endif
		ax88179_write_cmd_async(dev, AX_ACCESS_MAC, AX_MULTI_FILTER_ARRY, 
			AX_MCAST_FILTER_SIZE, AX_MCAST_FILTER_SIZE, m_filter);

		rx_ctl |= AX_RX_CTL_AM;
	}

	data->rxctl = rx_ctl;
	ax88179_write_cmd_async(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, &data->rxctl);
}

static struct net_device_stats *ax88179_get_stats (struct net_device *net)
{	
	return &net->stats;
}

static int ax88179_set_mac_addr (struct net_device *net, void *p)
{
	struct usbnet *dev = netdev_priv(net);
	struct sockaddr *addr = p;

	memcpy (net->dev_addr, addr->sa_data, ETH_ALEN);

	/* Set the MAC address */
	return ax88179_write_cmd (dev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN,
						ETH_ALEN, net->dev_addr);

}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,29)
static const struct net_device_ops ax88179_netdev_ops = {
	.ndo_open			= usbnet_open,
	.ndo_stop			= usbnet_stop,
	.ndo_start_xmit		= usbnet_start_xmit,
	.ndo_tx_timeout		= usbnet_tx_timeout,
	.ndo_change_mtu		= usbnet_change_mtu,
	.ndo_do_ioctl			= ax88179_ioctl,
	.ndo_get_stats		= ax88179_get_stats,
	.ndo_set_mac_address 	= ax88179_set_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,2,0)
	.ndo_set_multicast_list	= ax88179_set_multicast,
#else
	.ndo_set_rx_mode	= ax88179_set_multicast,
#endif
};
#endif

static int ax88179_check_eeprom(struct usbnet *dev)
{
	u8 i;
	u8 buf[2];
	u8 eeprom[20];
	u16 csum;	
			
	/* Read EEPROM content */
	for (i=0; i < 6; i++) {
	
		buf[0] = i;
		if (ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_SROM_ADDR, 1, 1, buf) < 0) 
			return -EINVAL;
		
		buf[0] = EEP_RD;
		if (ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_SROM_CMD, 1, 1, buf) < 0) 
			return -EINVAL;

		do
		{
			ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_SROM_CMD, 1, 1, buf);
		} while (buf[0] & EEP_BUSY);


		ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_SROM_DATA_LOW, 2, 2, &eeprom[i*2]);
			
		if ((i == 0) && (eeprom[0] == 0xFF))
			return -EINVAL;
	}

	csum = eeprom[6] + eeprom[7] + eeprom[8] +eeprom[9];
	csum = (csum >> 8) + (csum & 0xff);

	if ((csum + eeprom[10]) == 0xff)
		return AX_EEP_EFUSE_CORRECT;
	else
		return -EINVAL;
}

static int ax88179_check_ua2_efuse(struct usbnet *dev, void *ledMode)
{
	u8	i;
	u8	efuse[64] = {0x00};
	u16	csum = 0;

	if (ax88179_read_cmd(dev, AX_ACCESS_EFUSE, 0, 64, 64, efuse) < 0)
		return -EINVAL;
			
	if (efuse[0] == 0xFF)
		return -EINVAL;

	for(i = 0; i < 64; i++)
		csum = csum + efuse[i];
		
	while (csum > 255)
		csum = (csum & 0x00FF) + ((csum >> 8) & 0x00FF);

	if (csum == 0xFF)
	{
		memcpy((u8*)ledMode,&efuse[51],2);
		return AX_EEP_EFUSE_CORRECT;
	}
	else
		return -EINVAL;
}

static int ax88179_convert_old_led(struct usbnet *dev, u8 eFuse, void *ledValue)
{
	u8 LedMode;
	u16 tmp;	
	u16 Led;	

	if (eFuse)	//loaded the old eFuse LED Mode
	{
		if (ax88179_read_cmd(dev, AX_ACCESS_EFUSE, 0x18, 1, 2, &tmp) < 0)
			return -EINVAL;
		LedMode = (u8)(tmp & 0xFF);			
	}
	else		//loaded the old EEprom LED Mode
	{
		if (ax88179_read_cmd(dev, AX_ACCESS_EEPROM, 0x3C, 1, 2, &tmp) < 0) 
			return -EINVAL;
		LedMode = (u8) (tmp >> 8);		
	}
	
	netdev_dbg(dev->net, "Old LED Mode = %02X \n", LedMode);	
	
	switch (LedMode)
	{
	case 0xFF:
		Led = LED0_ACTIVE | LED1_LINK_10 | LED1_LINK_100 | LED1_LINK_1000 |
			 LED2_ACTIVE | LED2_LINK_10 | LED2_LINK_100 | LED2_LINK_1000 | LED_VALID;
		break;
	case 0xFE:
		Led = LED0_ACTIVE | LED1_LINK_1000 | LED2_LINK_100 | LED_VALID;
		break;
	case 0xFD:
		Led = LED0_ACTIVE | LED1_LINK_1000 | LED2_LINK_100 | LED2_LINK_10 | LED_VALID;
		break;
	case 0xFC:
		Led = LED0_ACTIVE | LED1_ACTIVE | LED1_LINK_1000 | LED2_ACTIVE | LED2_LINK_100 | LED2_LINK_10 | LED_VALID;
		break;
	default:
		Led = LED0_ACTIVE | LED1_LINK_10 | LED1_LINK_100 | LED1_LINK_1000 |
			 LED2_ACTIVE | LED2_LINK_10 | LED2_LINK_100 | LED2_LINK_1000 | LED_VALID;
		break;
	}	

	memcpy((u8*)ledValue,&Led,2);	
	
	return 0;
}

static int ax88179_led_setting(struct usbnet *dev)
{
	u8 LedFD;
	u8 value = 0;
	u16 tmp;
	u16 LedAct;
	u16 LedLink;	
	u16 LedValue = 0;
	
	/* Check AX88179 version. UA1 or UA2*/
	ax88179_read_cmd(dev, AX_ACCESS_MAC, GENERAL_STATUS, 1, 1, &value);
		
	if (!(value & AX_SECLD)) //UA1
	{
		value = AX_GPIO_CTRL_GPIO3EN | AX_GPIO_CTRL_GPIO2EN | AX_GPIO_CTRL_GPIO1EN;
		if (ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_GPIO_CTRL, 1, 1, &value) < 0)
			return -EINVAL;		
	}
	
	
	// check EEprom
	if (ax88179_check_eeprom(dev) == AX_EEP_EFUSE_CORRECT)
	{
		value = 0x42;
		if (ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_SROM_ADDR, 1, 1, &value) < 0) 
			return -EINVAL;
		
		value = EEP_RD;
		if (ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_SROM_CMD, 1, 1, &value) < 0) 
			return -EINVAL;

		do
		{
			ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_SROM_CMD, 1, 1, &value);
		} while (value & EEP_BUSY);


		ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_SROM_DATA_HIGH, 1, 1, &value);
		LedValue = (value << 8);
		ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_SROM_DATA_LOW, 1, 1, &value);
		LedValue |= value;

		if ((LedValue == 0xFFFF) || ((LedValue & LED_VALID) == 0))//load internal ROM for defaule setting
			ax88179_convert_old_led(dev,0,&LedValue);	
	}
	else if (ax88179_check_ua2_efuse(dev, &LedValue) == AX_EEP_EFUSE_CORRECT) //check eFuse
	{
		if ((LedValue == 0xFFFF) || ((LedValue & LED_VALID) == 0))
			ax88179_convert_old_led(dev,0,&LedValue);	
	}
	else
		ax88179_convert_old_led(dev,0,&LedValue);	

	tmp = GMII_PHY_PAGE_SELECT_EXT;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_PAGE_SELECT, 2, &tmp);

	tmp = 0x2c;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHYPAGE, 2, &tmp);

	ax88179_read_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_LED_ACTIVE, 2, &LedAct);

	ax88179_read_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_LED_LINK, 2, &LedLink);

	LedAct &= GMII_LED_ACTIVE_MASK;
	LedLink &= GMII_LED_LINK_MASK;
		
	
	if (LedValue & LED0_ACTIVE)
		LedAct |= GMII_LED0_ACTIVE;

	if (LedValue & LED1_ACTIVE)
		LedAct |= GMII_LED1_ACTIVE;

	if (LedValue & LED2_ACTIVE)
		LedAct |= GMII_LED2_ACTIVE;


	
	if (LedValue & LED0_LINK_10)
		LedLink |= GMII_LED0_LINK_10;

	if (LedValue & LED1_LINK_10)
		LedLink |= GMII_LED1_LINK_10;
		
	if (LedValue & LED2_LINK_10)
		LedLink |= GMII_LED2_LINK_10;
	

	if (LedValue & LED0_LINK_100)
		LedLink |= GMII_LED0_LINK_100;
	
	if (LedValue & LED1_LINK_100)
		LedLink |= GMII_LED1_LINK_100;
	
	if (LedValue & LED2_LINK_100)
		LedLink |= GMII_LED2_LINK_100;
	

	if (LedValue & LED0_LINK_1000)
		LedLink |= GMII_LED0_LINK_1000;
	
	if (LedValue & LED1_LINK_1000)
		LedLink |= GMII_LED1_LINK_1000;
	
	if (LedValue & LED2_LINK_1000)
		LedLink |= GMII_LED2_LINK_1000;

	tmp = LedAct;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_LED_ACTIVE, 2, &tmp);

	tmp = LedLink;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_LED_LINK, 2, &tmp);

	tmp = GMII_PHY_PAGE_SELECT_PAGE0;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_PAGE_SELECT, 2, &tmp);
	
	
	// LED full duplex setting
	LedFD = 0;
	if (LedValue & LED0_FD)
		LedFD |= 0x01;
	else if ((LedValue & LED0_USB3_MASK) == 0)
		LedFD |= 0x02;


	if (LedValue & LED1_FD)
		LedFD |= 0x04;
	else if ((LedValue & LED1_USB3_MASK) == 0)
		LedFD |= 0x08;

		
	if (LedValue & LED2_FD) //LED2_FD
		LedFD |= 0x10;
	else if ((LedValue & LED2_USB3_MASK) == 0) //LED2_USB3
		LedFD |= 0x20;

	ax88179_write_cmd(dev, AX_ACCESS_MAC, 0x73, 1, 1, &LedFD);
	
	return 0;	
}

static int ax88179_AutoDetach(struct usbnet *dev)
{
	u16 tmp;
	
	if (ax88179_read_cmd(dev, AX_ACCESS_EEPROM, 0x43, 1, 2, &tmp) < 0) 
		return 0;

	if((tmp == 0xFFFF) || (!(tmp & 0x0100)))
		return 0;

	/* Enable Auto Detach bit */
	tmp=0;
	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, &tmp);
	tmp |= AX_CLK_SELECT_ULR;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, &tmp);

	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp);
	tmp |= AX_PHYPWR_RSTCTL_AUTODETACH;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp);

	return 0;
}

static int ax88179_bind(struct usbnet *dev, struct usb_interface *intf)
{
	void *buf;
	u16 *tmp16;
	u8 *tmp;

	struct ax88179_data *ax179_data = (struct ax88179_data *)dev->driver_priv;

	usbnet_get_endpoints(dev,intf);

	if (msg_enable != 0) {
		dev->msg_enable = msg_enable;
	}
	
	buf = kmalloc (6, GFP_KERNEL);
	if (!buf) {
		printk(KERN_ERR "Cannot allocate memory for buffer");
		return -ENOMEM;
	}
	tmp16 = (u16 *)buf;
	tmp = (u8 *)buf;

	ax179_data = (struct ax88179_data *) kmalloc(sizeof(struct ax88179_data), GFP_KERNEL);
	if (!ax179_data) {
		printk(KERN_ERR "Cannot allocate memory for AX8817X data");
		kfree (buf);
		return -ENOMEM;
	}
	memset (ax179_data, 0, sizeof(*ax179_data));
	dev->driver_priv = ax179_data;



	/* Power up ethernet PHY */
	*tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	*tmp16 = AX_PHYPWR_RSTCTL_IPRL;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	msleep (200);

	*tmp = AX_CLK_SELECT_ACS | AX_CLK_SELECT_BCS;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, tmp);
	msleep (100);



	ax88179_read_cmd (dev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN, ETH_ALEN, dev->net->dev_addr);
	printk(KERN_DEBUG "MAC [%02x-%02x-%02x-%02x-%02x-%02x]\n",
		dev->net->dev_addr[0], dev->net->dev_addr[1],
		dev->net->dev_addr[2], dev->net->dev_addr[3],
		dev->net->dev_addr[4], dev->net->dev_addr[5]);

	/* RX bulk configuration */
	*tmp = AX_RX_BULKIN_QCTRL_TIME | AX_RX_BULKIN_QCTRL_IFG |
						AX_RX_BULKIN_QCTRL_SIZE;

	// default for USB3.0 to Giga
	// Bulk in timer
	*(tmp + 1) = 0x41;
	*(tmp + 2) = 0x00;
	// Bulk in size			
	*(tmp + 3) = 0x14;
	// Bulk in IFG
	*(tmp + 4) = 0x40;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_BULKIN_QCTRL, 5, 5, tmp);
	dev->rx_urb_size = (1024 * 20);

	tmp[0] = 0x34;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_LOW, 1, 1, tmp);

	tmp[0] = 0x52;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_HIGH, 1, 1, tmp);

	dev->net->hard_header_len += 12;
	dev->net->netdev_ops = &ax88179_netdev_ops;
	dev->net->ethtool_ops = &ax88179_ethtool_ops;

	/* Initialize MII structure */
	dev->mii.dev = dev->net;
	dev->mii.mdio_read = ax88179_mdio_read_le;
	dev->mii.mdio_write = ax88179_mdio_write_le;
	dev->mii.phy_id_mask = 0xff;
	dev->mii.reg_num_mask = 0xff;
	dev->mii.phy_id = 0x03;
	dev->mii.supports_gmii = 1;


	dev->net->features |= NETIF_F_IP_CSUM;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
	dev->net->features |= NETIF_F_IPV6_CSUM;
#endif
	dev->net->features |= NETIF_F_SG | NETIF_F_TSO | NETIF_F_TSO6;

	/* Register suspend and resume functions */
	ax179_data->suspend = ax88179_suspend;
	ax179_data->resume = ax88179_resume;

#if 1
	/* Enable checksum offload */
	*tmp16 = AX_RXCOE_IP | AX_RXCOE_TCP | AX_RXCOE_UDP |
				AX_RXCOE_TCPV6 | AX_RXCOE_UDPV6;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RXCOE_CTL, 1, 1, tmp16);

	*tmp16 = AX_TXCOE_IP | AX_TXCOE_TCP | AX_TXCOE_UDP |
				AX_TXCOE_TCPV6 | AX_TXCOE_UDPV6;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, tmp16);

	ax179_data->checksum |= AX_RX_CHECKSUM |AX_TX_CHECKSUM;
#endif

	/* Configure RX control register => start operation */
	*tmp16 = AX_RX_CTL_DROPCRCERR | AX_RX_CTL_IPE | AX_RX_CTL_START |
			AX_RX_CTL_AP | AX_RX_CTL_AMALL | AX_RX_CTL_AB;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, 0x0b, 2, 2, tmp16);

	*tmp = AX_MONITOR_MODE_PMETYPE | AX_MONITOR_MODE_PMEPOL |
						AX_MONITOR_MODE_RWMP;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, tmp);

	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, tmp);
	printk(KERN_DEBUG "Monitor mode = 0x%02x\n", *tmp);

	/* Configure default medium type => giga */
	*tmp16 = AX_MEDIUM_RECEIVE_EN 	 | AX_MEDIUM_TXFLOW_CTRLEN |
		 AX_MEDIUM_RXFLOW_CTRLEN | AX_MEDIUM_ALWAYS_ONE    | 
		 AX_MEDIUM_FULL_DUPLEX 	 | AX_MEDIUM_GIGAMODE;

	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE, 2, 2, tmp16);

	ax88179_led_setting(dev);

	/* Restart autoneg */
	*tmp16 = GMII_CONTROL_1000MB | GMII_CONTROL_ENABLE_AUTO |
		GMII_CONTROL_START_AUTO | GMII_CONTROL_FULL_DUPLEX;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_CONTROL, 2, tmp16);

	netif_carrier_off(dev->net);

	kfree (buf);
	printk (version);
	printk(KERN_INFO "mtu %d\n", dev->net->mtu);

	return 0;

}

static void ax88179_unbind(struct usbnet *dev, struct usb_interface *intf)
{
	u16 tmp16;
	struct ax88179_data *ax179_data = (struct ax88179_data *) dev->driver_priv;

	if (ax179_data) {
		/* Configure RX control register => stop operation */
		tmp16 = AX_RX_CTL_STOP;
		ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, &tmp16);

		tmp16 = 0x0;
		ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, &tmp16);

		/* Power down ethernet PHY */
		tmp16 = AX_PHYPWR_RSTCTL_BZ | AX_PHYPWR_RSTCTL_IPRL;
		ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp16);
		dev->driver_priv = NULL;
		kfree (ax179_data);
		msleep (200);
	}	
	
}

static void
ax88179_rx_checksum (struct sk_buff *skb, struct ax88179_rx_pkt_header *pkt_hdr)
{
	skb->ip_summed = CHECKSUM_NONE;

	/* checksum error bit is set */
	if (pkt_hdr->l3_csum_err || pkt_hdr->l4_csum_err) {
		//printk(KERN_ERR "checksum error (0x%08x)\n", *((u32 *)pkt_hdr));
		return;
	}

	/* It must be a TCP or UDP packet with a valid checksum */
	if ((pkt_hdr->l4_type == AX_RXHDR_L4_TYPE_TCP) ||
	    (pkt_hdr->l4_type == AX_RXHDR_L4_TYPE_UDP)) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	}
}

static int ax88179_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	struct sk_buff *ax_skb;
	int pkt_cnt;
	u32 rx_hdr;
	u16 hdr_off;
	struct ax88179_rx_pkt_header *pkt_hdr;

	skb_trim(skb, skb->len - 4);
	memcpy(&rx_hdr, skb_tail_pointer(skb), sizeof(rx_hdr));
	le32_to_cpus (&rx_hdr);

	pkt_cnt = (u16)rx_hdr;
	hdr_off = (u16)(rx_hdr >> 16);
	pkt_hdr = (struct ax88179_rx_pkt_header *)(skb->data + hdr_off);

	while (pkt_cnt--) {

		u16 pkt_len;

		le32_to_cpus ((u32 *)pkt_hdr);
		pkt_len = pkt_hdr->len;

		/* CRC or MII error */
		if (pkt_hdr->crc  || pkt_hdr->drop) {
			skb_pull(skb, (pkt_len + 7) & 0xFFF8);
			pkt_hdr++;
			continue;
		}

		if (pkt_cnt == 0) {
			/* Skip psudo header */
			skb_pull(skb, 2);

			skb->len = pkt_len;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
			skb->tail = skb->data + pkt_len;
#else
			skb_set_tail_pointer(skb, pkt_len);
#endif
			skb->truesize = pkt_len + sizeof(struct sk_buff);

			ax88179_rx_checksum (skb, pkt_hdr);

			return 2;
		}

		ax_skb = skb_clone(skb, GFP_ATOMIC);
		if (ax_skb) {
			ax_skb->len = pkt_len;
			ax_skb->data = skb->data + 2;
			//ax_skb->data = skb->data;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
			ax_skb->tail = ax_skb->data + pkt_len;
#else
			skb_set_tail_pointer(ax_skb, pkt_len);
#endif
			ax_skb->truesize = pkt_len + sizeof(struct sk_buff);

			ax88179_rx_checksum (ax_skb, pkt_hdr);
			usbnet_skb_return(dev, ax_skb);

		} else {
			return 0;
		}

		skb_pull(skb, (pkt_len + 7) & 0xFFF8);
	
		pkt_hdr++;
	}
	return 0;
}

static struct sk_buff *
ax88179_tx_fixup(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	u32 tx_hdr1, tx_hdr2;
	int frame_size = dev->maxpacket;
	int mss = skb_shinfo(skb)->gso_size;
	int headroom;
	int tailroom;

	tx_hdr1 = skb->len;
	tx_hdr2 = mss;
	if (((skb->len + 8) % frame_size) == 0)
	{
		tx_hdr2 |= 0x80008000;	/* Enable padding */
		skb->len += 2;
	}

	skb_linearize (skb);

	headroom = skb_headroom(skb);
	tailroom = skb_tailroom(skb);

	if ((headroom + tailroom) >= 8) {
		if (headroom < 8) {
			skb->data = memmove(skb->head + 8, skb->data, skb->len);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
			skb->tail = skb->data + skb->len;
#else
			skb_set_tail_pointer(skb, skb->len);
#endif
		}
	} else {
		struct sk_buff *skb2;
		skb2 = skb_copy_expand(skb, 8, 0, flags);
		dev_kfree_skb_any(skb);
		skb = skb2;
		if (!skb)
			return NULL;
	}

	skb_push(skb, 4);
	cpu_to_le32s (&tx_hdr2);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
	memcpy(skb->data, &tx_hdr2, 4);
#else
	skb_copy_to_linear_data(skb, &tx_hdr2, 4);
#endif

	skb_push(skb, 4);
	cpu_to_le32s (&tx_hdr1);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
	memcpy(skb->data, &tx_hdr1, 4);
#else
	skb_copy_to_linear_data(skb, &tx_hdr1, 4);
#endif

	return skb;
}

static int ax88179_unlink_urbs (struct usbnet *dev, struct sk_buff_head *q)
{
	unsigned long		flags;
	struct sk_buff		*skb, *skbnext;
	int			count = 0;

	spin_lock_irqsave (&q->lock, flags);
	skb_queue_walk_safe(q, skb, skbnext) {
		struct skb_data		*entry;
		struct urb		*urb;
		int			retval;

		entry = (struct skb_data *) skb->cb;
		urb = entry->urb;

		//*
		 //* Get reference count of the URB to avoid it to be
		 //* freed during usb_unlink_urb, which may trigger
		 //* use-after-free problem inside usb_unlink_urb since
		 //* usb_unlink_urb is always racing with .complete
		 //* handler(include defer_bh).
		 //*
		usb_get_urb(urb);
		spin_unlock_irqrestore(&q->lock, flags);
		// during some PM-driven resume scenarios,
		// these (async) unlinks complete immediately
		retval = usb_unlink_urb (urb);
		if (retval != -EINPROGRESS && retval != 0)
			netdev_dbg(dev->net, "unlink urb err, %d\n", retval);
		else
			count++;
		usb_put_urb(urb);
		spin_lock_irqsave(&q->lock, flags);
	}
	spin_unlock_irqrestore (&q->lock, flags);
	return count;
}

static int ax88179_link_reset (struct usbnet *dev)
{
	struct ax88179_data *data = (struct ax88179_data *)&dev->driver_priv;
	u8 tmp[5], link_sts;
	u16 mode, tmp16, delay = HZ/10;
	u32 tmp32 = 0x40000000;
	unsigned long jTimeout;

	if (!data->linkup) {
		ax88179_unlink_urbs (dev, &dev->txq);
		return 0;
	}

	jTimeout = jiffies + delay;

	while (tmp32 & 0x40000000) {
		mode = 0;
		ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, &mode);		
		ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, &data->rxctl);
	
		// link up, check the usb device control TX FIFO full or empty
		ax88179_read_cmd(dev,0x81, 0x8c, 0, 4,&tmp32);

		if (time_after(jiffies, jTimeout))
			return 0;
	}

	mode = AX_MEDIUM_RECEIVE_EN    | AX_MEDIUM_TXFLOW_CTRLEN |
		   AX_MEDIUM_RXFLOW_CTRLEN | AX_MEDIUM_ALWAYS_ONE;

	ax88179_read_cmd (dev, AX_ACCESS_MAC, PHYSICAL_LINK_STATUS, 1, 1, &link_sts);
	ax88179_read_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_PHYSR, 2, &tmp16);

	if (!(tmp16 & GMII_PHY_PHYSR_LINK))
		return 0;
	else if (GMII_PHY_PHYSR_GIGA == (tmp16 & GMII_PHY_PHYSR_SMASK)) {
		mode |= AX_MEDIUM_GIGAMODE;	/* Bit 0 : GM */
		if (link_sts & AX_USB_SS)
			memcpy(tmp, &AX88179_BULKIN_SIZE[0], 5);
		else if (link_sts & AX_USB_HS)
			memcpy(tmp, &AX88179_BULKIN_SIZE[1], 5);
		else
			memcpy(tmp, &AX88179_BULKIN_SIZE[3], 5);
	}
	else if (GMII_PHY_PHYSR_100 == (tmp16 & GMII_PHY_PHYSR_SMASK)) {
		mode |= AX_MEDIUM_PS;	/* Bit 9 : PS */
		if (link_sts & (AX_USB_SS | AX_USB_HS))
			memcpy(tmp, &AX88179_BULKIN_SIZE[2], 5);
		else
			memcpy(tmp, &AX88179_BULKIN_SIZE[3], 5);
	} else
		memcpy(tmp, &AX88179_BULKIN_SIZE[3], 5);

	/* RX bulk configuration */
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_BULKIN_QCTRL, 5, 5, tmp);

	if (tmp16 & GMII_PHY_PHYSR_FULL)
		mode |= AX_MEDIUM_FULL_DUPLEX;	/* Bit 1 : FD */
 
	dev->rx_urb_size = (1024 * tmp[3]);

	mode |= 0x08;
	printk(KERN_INFO "Write medium type: 0x%04x\n", mode);
	/* Configure default medium type => giga */
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE, 2, 2, &mode);
	mii_check_media (&dev->mii, 1, 1);

	return 0;
}

static int ax88179_reset (struct usbnet *dev)
{
	void *buf;
	u16 *tmp16;
	u8 *tmp;
	struct ax88179_data *ax179_data = (struct ax88179_data *) dev->driver_priv;
	buf = kmalloc (6, GFP_KERNEL);

	tmp16 = (u16 *)buf;
	tmp = (u8 *)buf;

	ax179_data->linkup = 0;
	/* Power up ethernet PHY */
	*tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	*tmp16 = AX_PHYPWR_RSTCTL_IPRL;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, tmp16);
	msleep (200);

	*tmp = AX_CLK_SELECT_ACS | AX_CLK_SELECT_BCS;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, tmp);
	msleep (100);

	/* Ethernet PHY Auto Detach*/
	ax88179_AutoDetach(dev);

	ax88179_read_cmd (dev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN, ETH_ALEN, dev->net->dev_addr);
	netdev_dbg(dev->net, "MAC [%02x-%02x-%02x-%02x-%02x-%02x]\n",
		dev->net->dev_addr[0], dev->net->dev_addr[1],
		dev->net->dev_addr[2], dev->net->dev_addr[3],
		dev->net->dev_addr[4], dev->net->dev_addr[5]);

	/* RX bulk configuration */
	*tmp = AX_RX_BULKIN_QCTRL_TIME | AX_RX_BULKIN_QCTRL_IFG |
						AX_RX_BULKIN_QCTRL_SIZE;
	// Bulk in timer
	*(tmp + 1) = 0x41;
	*(tmp + 2) = 0x00;
	// Bulk in size			
	*(tmp + 3) = 0x14;
	// Bulk in IFG
	*(tmp + 4) = 0x40;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_BULKIN_QCTRL, 5, 5, tmp);
	dev->rx_urb_size = (1024 * 20);

	tmp[0] = 0x34;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_LOW, 1, 1, tmp);

	tmp[0] = 0x52;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_HIGH, 1, 1, tmp);

#if 1
	/* Enable checksum offload */
	*tmp16 = AX_RXCOE_IP | AX_RXCOE_TCP | AX_RXCOE_UDP |
				AX_RXCOE_TCPV6 | AX_RXCOE_UDPV6;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RXCOE_CTL, 1, 1, tmp16);

	*tmp16 = AX_TXCOE_IP | AX_TXCOE_TCP | AX_TXCOE_UDP |
				AX_TXCOE_TCPV6 | AX_TXCOE_UDPV6;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, tmp16);

	ax179_data->checksum |= AX_RX_CHECKSUM |AX_TX_CHECKSUM;
#endif

	/* Configure RX control register => start operation */
	*tmp16 = AX_RX_CTL_DROPCRCERR | AX_RX_CTL_IPE | AX_RX_CTL_START |
			AX_RX_CTL_AP | AX_RX_CTL_AMALL | AX_RX_CTL_AB;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, 0x0b, 2, 2, tmp16);


	*tmp = AX_MONITOR_MODE_PMETYPE | AX_MONITOR_MODE_PMEPOL |
						AX_MONITOR_MODE_RWMP;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, tmp);

	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, tmp);
	netdev_dbg(dev->net, "Monitor mode = 0x%02x\n", *tmp);

	/* Configure default medium type => giga */
	*tmp16 = AX_MEDIUM_RECEIVE_EN 	 | AX_MEDIUM_TXFLOW_CTRLEN |
		 AX_MEDIUM_RXFLOW_CTRLEN | AX_MEDIUM_ALWAYS_ONE    | 
		 AX_MEDIUM_FULL_DUPLEX 	 | AX_MEDIUM_GIGAMODE;

	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE, 2, 2, tmp16);

	ax88179_led_setting(dev);

	/* Restart autoneg */
	*tmp16 = GMII_CONTROL_1000MB | GMII_CONTROL_ENABLE_AUTO |
		GMII_CONTROL_START_AUTO | GMII_CONTROL_FULL_DUPLEX;
	ax88179_write_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID, GMII_PHY_CONTROL, 2, tmp16);

	netif_carrier_off(dev->net);

	kfree (buf);
	netdev_dbg(dev->net, "mtu %d\n", dev->net->mtu);

	return 0;

}


static int ax88179_stop (struct usbnet *dev)
{
	u16 tmp16;

	ax88179_read_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE, 2, 2, &tmp16);
	tmp16 &= ~AX_MEDIUM_RECEIVE_EN;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE, 2, 2, &tmp16);
	return 0;
}
static int ax_suspend (struct usb_interface *intf,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10)
			pm_message_t message)
#else
			u32 message)
#endif
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct ax88179_data *data = (struct ax88179_data *)dev->driver_priv;
	return data->suspend (intf, message);
}

static int ax_resume (struct usb_interface *intf)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct ax88179_data *data = (struct ax88179_data *)dev->driver_priv;

	return data->resume (intf);
}

static const struct driver_info ax88179_info = {
	.description = "ASIX AX88179 USB 3.0 Gigibit Ethernet",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.status = ax88179_status,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.stop = ax88179_stop,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};


static const struct driver_info ax88178a_info = {
	.description = "ASIX AX88178A USB 2.0 Gigibit Ethernet",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.status = ax88179_status,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.stop = ax88179_stop,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info sitecom_info = {
	.description = "Sitecom USB 3.0 to Gigabit Adapter",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.status = ax88179_status,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.stop = ax88179_stop,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct usb_device_id	products [] = {
{
	/* ASIX AX88179 10/100/1000 */
        USB_DEVICE (0x0b95, 0x1790),
        .driver_info = (unsigned long) &ax88179_info,
},{
	/* ASIX AX88178A 10/100/1000 */
        USB_DEVICE (0x0b95, 0x178a),
        .driver_info = (unsigned long) &ax88178a_info,
},{
	/* Sitecom USB 3.0 to Gigabit Adapter */
        USB_DEVICE (0x0df6, 0x0072),
        .driver_info = (unsigned long) &sitecom_info,
},
	{ },		/* END */
};
MODULE_DEVICE_TABLE(usb, products);

static struct usb_driver asix_driver = {
	.name =		"ax88179_178a",
	.id_table =	products,
	.probe =	usbnet_probe,
	.suspend =	ax_suspend,
	.resume =	ax_resume,
	.disconnect =	usbnet_disconnect,
};


static int __init asix_init(void)
{
 	return usb_register(&asix_driver);
}
module_init(asix_init);

static void __exit asix_exit(void)
{
 	usb_deregister(&asix_driver);
}
module_exit(asix_exit);

MODULE_AUTHOR("David Hollis");
MODULE_DESCRIPTION("ASIX AX88179_178A based USB 2.0/3.0 Gigabit Ethernet Devices");
MODULE_LICENSE("GPL");

