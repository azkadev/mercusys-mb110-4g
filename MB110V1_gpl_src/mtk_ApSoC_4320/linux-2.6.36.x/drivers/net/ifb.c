/* drivers/net/ifb.c:

	The purpose of this driver is to provide a device that allows
	for sharing of resources:

	1) qdiscs/policies that are per device as opposed to system wide.
	ifb allows for a device which can be redirected to thus providing
	an impression of sharing.

	2) Allows for queueing incoming traffic for shaping instead of
	dropping.

	The original concept is based on what is known as the IMQ
	driver initially written by Martin Devera, later rewritten
	by Patrick McHardy and then maintained by Andre Correa.

	You need the tc action  mirror or redirect to feed this device
       	packets.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.

  	Authors:	Jamal Hadi Salim (2005)

*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <net/pkt_sched.h>
#include <net/net_namespace.h>
#include <linux/sched.h>
#include <linux/random.h>



#define TX_TIMEOUT  (2*HZ)

#define TX_Q_LIMIT    32

/*in second*/
#define UPTIME_TO_CHECK_CPU_LOAD    90

#define CPU_LOAD_BEGIN_TO_DROP_SKB    3

#define RANDOM_LIMIT_DROP_SKB	3

#define IFB_DS "ifb1"
#define IFB_US "ifb0"


static unsigned int drop_cpu_load = CPU_LOAD_BEGIN_TO_DROP_SKB;

static unsigned int drop_random_limit = RANDOM_LIMIT_DROP_SKB;

static unsigned int drop_random_smoothly = 0;


struct kobject *sys_ifb;	/* sysfs linkage */

struct rate_control {
	unsigned int cpu_load_limit;
	unsigned int cpu_load2_limit;
	int random_drop_limit_deta;
};

struct rate_control rc_table[] = {
/*
		{5,80,16},
		{5,60,10},
		{5,40,14},
		{5,20,12},
		{5,0,13},


		{4,80,16},
		{4,60,12},
		{4,40,18},
		{4,20,10},
		{4,0,13},


		{3,80,13},
		{3,60,12},
		{3,40,14},
		{3,20,10},
		{3,0,15},


		{2,80,13},
		{2,60,12},
		{2,40,15},
		{2,20,10},
		{2,0,13},


		{1,80,14},
		{1,60,15},
		{1,40,16},
		{1,20,10},
		{1,0,14},
*/
		{0,80,1},
		{0,60,1},
		{0,40,0},
		{0,20,-1},
		{0,0,-1}
	};




struct ifb_private {
	struct tasklet_struct   ifb_tasklet;
	int     tasklet_pending;
	/* mostly debug stats leave in for now */
	unsigned long   st_task_enter; /* tasklet entered */
	unsigned long   st_txq_refl_try; /* transmit queue refill attempt */
	unsigned long   st_rxq_enter; /* receive queue entered */
	unsigned long   st_rx2tx_tran; /* receive to trasmit transfers */
	unsigned long   st_rxq_notenter; /*receiveQ not entered, resched */
	unsigned long   st_rx_frm_egr; /* received from egress path */
	unsigned long   st_rx_frm_ing; /* received from ingress path */
	unsigned long   st_rxq_check;
	unsigned long   st_rxq_rsch;
	struct sk_buff_head     rq;
	struct sk_buff_head     tq;
};

static int numifbs = 2;

static void ri_tasklet(unsigned long dev);
static netdev_tx_t ifb_xmit(struct sk_buff *skb, struct net_device *dev);
static int ifb_open(struct net_device *dev);
static int ifb_close(struct net_device *dev);

static void ri_tasklet(unsigned long dev)
{

	struct net_device *_dev = (struct net_device *)dev;
	struct ifb_private *dp = netdev_priv(_dev);
	struct net_device_stats *stats = &_dev->stats;
	struct netdev_queue *txq;
	struct sk_buff *skb;

	txq = netdev_get_tx_queue(_dev, 0);
	dp->st_task_enter++;
	if ((skb = skb_peek(&dp->tq)) == NULL) {
		dp->st_txq_refl_try++;
		if (__netif_tx_trylock(txq)) {
			dp->st_rxq_enter++;
			while ((skb = skb_dequeue(&dp->rq)) != NULL) {
				skb_queue_tail(&dp->tq, skb);
				dp->st_rx2tx_tran++;
			}
			__netif_tx_unlock(txq);
		} else {
			/* reschedule */
			dp->st_rxq_notenter++;
			goto resched;
		}
	}

	while ((skb = skb_dequeue(&dp->tq)) != NULL) {
		u32 from = G_TC_FROM(skb->tc_verd);

		skb->tc_verd = 0;
		skb->tc_verd = SET_TC_NCLS(skb->tc_verd);
		stats->tx_packets++;
		stats->tx_bytes +=skb->len;

		rcu_read_lock();
		skb->dev = dev_get_by_index_rcu(&init_net, skb->skb_iif);
		if (!skb->dev) {
			rcu_read_unlock();
			dev_kfree_skb(skb);
			stats->tx_dropped++;
			break;
		}
		rcu_read_unlock();
		skb->skb_iif = _dev->ifindex;

		if (from & AT_EGRESS) {
			dp->st_rx_frm_egr++;
			dev_queue_xmit(skb);
		} else if (from & AT_INGRESS) {
			dp->st_rx_frm_ing++;
			skb_pull(skb, skb->dev->hard_header_len);
			netif_rx(skb);
		} else
			BUG();
	}

	if (__netif_tx_trylock(txq)) {
		dp->st_rxq_check++;
		if ((skb = skb_peek(&dp->rq)) == NULL) {
			dp->tasklet_pending = 0;
			if (netif_queue_stopped(_dev))
				netif_wake_queue(_dev);
		} else {
			dp->st_rxq_rsch++;
			__netif_tx_unlock(txq);
			goto resched;
		}
		__netif_tx_unlock(txq);
	} else {
resched:
		dp->tasklet_pending = 1;
		tasklet_hi_schedule(&dp->ifb_tasklet);
	}

}

static const struct net_device_ops ifb_netdev_ops = {
	.ndo_open	= ifb_open,
	.ndo_stop	= ifb_close,
	.ndo_start_xmit	= ifb_xmit,
	.ndo_validate_addr = eth_validate_addr,
};

static void ifb_setup(struct net_device *dev)
{
	/* Initialize the device structure. */
	dev->destructor = free_netdev;
	dev->netdev_ops = &ifb_netdev_ops;

	/* Fill in device structure with ethernet-generic values. */
	ether_setup(dev);
	dev->tx_queue_len = TX_Q_LIMIT;

	dev->flags |= IFF_NOARP;
	dev->flags &= ~IFF_MULTICAST;
	dev->priv_flags &= ~IFF_XMIT_DST_RELEASE;
	random_ether_addr(dev->dev_addr);
}

unsigned long getUptime()
{
		struct timespec uptime;
		do_posix_clock_monotonic_gettime(&uptime);
		monotonic_to_bootbased(&uptime);
		return (unsigned long)uptime.tv_sec;

}


unsigned int checkNeedDropPacket(unsigned long cpu_load,unsigned long cpu_load2)
{
	int i = 0;

	//printk("\ncheckNeedDropPacket: %lu.%02lu\n",cpu_load,cpu_load2);
	if(drop_random_smoothly)
	{
		for(i = 0; i < sizeof(rc_table)/sizeof(struct rate_control); i++)
		{
			if(cpu_load >= rc_table[i].cpu_load_limit + drop_cpu_load)
				if(cpu_load2 >= rc_table[i].cpu_load2_limit)
					if((random32() % (rc_table[i].random_drop_limit_deta + drop_random_limit)) == 0)
						//printk("\ncheckNeedDropPacket:i:%d/%d cpu_load_limit:%u  cpu_load2_limit:%u %lu.%02lu random:%u limit:%d\n",i,sizeof(rc_table)/sizeof(struct rate_control),rc_table[i].cpu_load_limit,rc_table[i].cpu_load2_limit,cpu_load,cpu_load2,random,rc_table[i].random_drop_limit);
						return 1;


		}
	}
	else
		{
			/*if(cpu_load >= 3*drop_cpu_load)
				if(random32() %  drop_random_limit/4 == 0)
						return 1;
			else if(cpu_load >= 2*drop_cpu_load)
				if(random32() %  drop_random_limit/2 == 0)
						return 1;
			else*/
			if(cpu_load >= drop_cpu_load)
				if(random32() %  drop_random_limit == 0)
						return 1;

		}

	return 0;
}


static netdev_tx_t ifb_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ifb_private *dp = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	u32 from = G_TC_FROM(skb->tc_verd);

#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

	unsigned long avnrun[3];
	get_avenrun(avnrun, FIXED_1/200, 0);

	stats->rx_packets++;
	stats->rx_bytes+=skb->len;

	if (!(from & (AT_INGRESS|AT_EGRESS)) || !skb->skb_iif) {
		dev_kfree_skb(skb);
		stats->rx_dropped++;
		return NETDEV_TX_OK;
	}

	if (skb_queue_len(&dp->rq) >= dev->tx_queue_len ||
		(getUptime() > UPTIME_TO_CHECK_CPU_LOAD && LOAD_INT(avnrun[0]) >= drop_cpu_load &&
		(from & AT_EGRESS) && strcmp(dev->name,IFB_DS) == 0))
	{
		if(checkNeedDropPacket(LOAD_INT(avnrun[0]),LOAD_FRAC(avnrun[0])))
		{
			//netif_stop_queue(dev);
			dev_kfree_skb(skb);
			stats->rx_dropped++;
			return NETDEV_TX_OK;
		}
	}

	skb_queue_tail(&dp->rq, skb);
	if (!dp->tasklet_pending) {
		dp->tasklet_pending = 1;
		tasklet_hi_schedule(&dp->ifb_tasklet);
	}

	return NETDEV_TX_OK;
}

static int ifb_close(struct net_device *dev)
{
	struct ifb_private *dp = netdev_priv(dev);

	tasklet_kill(&dp->ifb_tasklet);
	netif_stop_queue(dev);
	skb_queue_purge(&dp->rq);
	skb_queue_purge(&dp->tq);
	return 0;
}

static int ifb_open(struct net_device *dev)
{
	struct ifb_private *dp = netdev_priv(dev);

	tasklet_init(&dp->ifb_tasklet, ri_tasklet, (unsigned long)dev);
	skb_queue_head_init(&dp->rq);
	skb_queue_head_init(&dp->tq);
	netif_start_queue(dev);

	return 0;
}

static int ifb_validate(struct nlattr *tb[], struct nlattr *data[])
{
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	return 0;
}

static struct rtnl_link_ops ifb_link_ops __read_mostly = {
	.kind		= "ifb",
	.priv_size	= sizeof(struct ifb_private),
	.setup		= ifb_setup,
	.validate	= ifb_validate,
};

/* Number of ifb devices to be set up by this module. */
module_param(numifbs, int, 0);
MODULE_PARM_DESC(numifbs, "Number of ifb devices");

static int __init ifb_init_one(int index)
{
	struct net_device *dev_ifb;
	int err;

	dev_ifb = alloc_netdev(sizeof(struct ifb_private),
				 "ifb%d", ifb_setup);

	if (!dev_ifb)
		return -ENOMEM;

	err = dev_alloc_name(dev_ifb, dev_ifb->name);
	if (err < 0)
		goto err;

	dev_ifb->rtnl_link_ops = &ifb_link_ops;
	err = register_netdevice(dev_ifb);
	if (err < 0)
		goto err;

	return 0;

err:
	free_netdev(dev_ifb);
	return err;
}


static ssize_t ifb_get_drop_cpu_load(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", drop_cpu_load);
}

static ssize_t ifb_set_drop_cpu_load(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	strict_strtol(buf, 0, (long int *)&drop_cpu_load);

	return size;
}

static ssize_t ifb_get_drop_random_limit(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", drop_random_limit);
}

static ssize_t ifb_set_drop_random_limit(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	strict_strtol(buf, 0, (long int *)&drop_random_limit);

	return size;
}


static ssize_t ifb_get_drop_random_smoothly(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", drop_random_smoothly);
}

static ssize_t ifb_set_drop_random_smoothly(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	strict_strtol(buf, 0, (long int *)&drop_random_smoothly);

	return size;
}



static const struct device_attribute ifb_drop_cpu_load =
	__ATTR(cpu_loadavg_drop, S_IWUSR | S_IRUGO, ifb_get_drop_cpu_load, ifb_set_drop_cpu_load);

static const struct device_attribute ifb_drop_random_limit =
	__ATTR(random_limit_drop, S_IWUSR | S_IRUGO, ifb_get_drop_random_limit, ifb_set_drop_random_limit);

static const struct device_attribute ifb_drop_random_smoothly =
	__ATTR(random_smoothly_drop, S_IWUSR | S_IRUGO, ifb_get_drop_random_smoothly, ifb_set_drop_random_smoothly);



static int __init ifb_init_module(void)
{
	int i, err;

	rtnl_lock();
	err = __rtnl_link_register(&ifb_link_ops);

	for (i = 0; i < numifbs && !err; i++)
		err = ifb_init_one(i);
	if (err)
		__rtnl_link_unregister(&ifb_link_ops);
	rtnl_unlock();


	sys_ifb = kobject_create_and_add("ifb", NULL);

	sysfs_create_file(sys_ifb, &ifb_drop_cpu_load.attr);
	sysfs_create_file(sys_ifb, &ifb_drop_random_limit.attr);
	sysfs_create_file(sys_ifb, &ifb_drop_random_smoothly.attr);

	return err;
}

static void __exit ifb_cleanup_module(void)
{
	sysfs_remove_file(sys_ifb, &ifb_drop_cpu_load.attr);
	sysfs_remove_file(sys_ifb, &ifb_drop_random_limit.attr);
	sysfs_remove_file(sys_ifb, &ifb_drop_random_smoothly.attr);

	kobject_put(sys_ifb);

	rtnl_link_unregister(&ifb_link_ops);
}

module_init(ifb_init_module);
module_exit(ifb_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jamal Hadi Salim");
MODULE_ALIAS_RTNL_LINK("ifb");
