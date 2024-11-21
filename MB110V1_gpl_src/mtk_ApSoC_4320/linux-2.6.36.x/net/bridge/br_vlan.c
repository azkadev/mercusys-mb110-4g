/*
 *	Linux ethernet bridge
 *
 *	Copyright (C) 1992 Linus Torvalds
 *
 *	Distribute under GPLv2.
 *
 */

#ifdef CONFIG_BRIDGE_VLAN

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include "br_private.h"
#include "br_vlan.h"



void br_show_vlan_map(struct net_bridge *br)
{
    struct net_bridge_port *p;
    printk("-------------------%s-------------------\n",br->dev->name);
	list_for_each_entry_rcu(p, &br->port_list, list) {
		printk("dev=%s, vlan_id=%x\n",p->dev->name ,p->vlan_id);
	}
}

bool br_vlan_forward_hook(const struct net_bridge_port *p, const struct sk_buff *skb)
{
	struct net_bridge_port *psrc;

	psrc = br_port_get_rcu(skb->dev);

	if(!psrc)
	{
		return 1;
	}

	if((!psrc->vlan_id) || (!p->vlan_id) )
	{
		BR_VLAN_PRINT("accept %d->%d", psrc->vlan_id , p->vlan_id);
		return 1;
	}

	if(psrc->vlan_id == p->vlan_id)
	{
		BR_VLAN_PRINT("accept %d->%d", psrc->vlan_id , p->vlan_id);
		return 1;
	}
	else
	{
		BR_VLAN_PRINT("deny %d->%d", psrc->vlan_id , p->vlan_id);
		return 0;
	}
}

int br_set_if_vlan(struct net_bridge *br, struct net_device *dev, int vlan_id)
{
	struct net_bridge_port *p;

	p = br_port_exists(dev) ? br_port_get(dev) : NULL;

	if (!p || p->br != br)
		return -EINVAL;

	p->vlan_id = vlan_id;

	return 0;
}

#endif
