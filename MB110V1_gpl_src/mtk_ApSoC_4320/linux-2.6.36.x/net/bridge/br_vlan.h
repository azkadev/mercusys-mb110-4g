
#ifndef __BR_VLAN_H__
#define __BR_VLAN_H__

#ifdef CONFIG_BRIDGE_VLAN

#if 0
#define BR_VLAN_PRINT(format, args...)\
	printk("%s(%d): "format"\n", __FUNCTION__, __LINE__, ##args)
#else

#define BR_VLAN_PRINT(format, args...)

#endif

bool br_vlan_forward_hook(const struct net_bridge_port *p, const struct sk_buff *skb);

int br_set_if_vlan(struct net_bridge *br, struct net_device *dev, int vlan_id);

void br_show_vlan_map(struct net_bridge *br);

#endif

#endif
