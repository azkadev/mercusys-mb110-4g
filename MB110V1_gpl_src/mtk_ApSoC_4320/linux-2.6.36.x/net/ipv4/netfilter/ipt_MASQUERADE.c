/* Masquerade.  Simple mapping which alters range to a local IP address
   (depending on route). */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/types.h>
#include <linux/inetdevice.h>
#include <linux/ip.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <net/protocol.h>
#include <net/ip.h>
#include <net/checksum.h>
#include <net/route.h>
#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter/x_tables.h>

#undef CONFIG_INCLUDE_FULLCONE

#define CONFIG_INCLUDE_FULLCONE

#if defined(CONFIG_INCLUDE_FULLCONE)
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <linux/netfilter_ipv4/lockhelp.h>
#endif /* CONFIG_INCLUDE_FULLCONE */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netfilter Core Team <coreteam@netfilter.org>");
MODULE_DESCRIPTION("Xtables: automatic-address SNAT");

#if defined(CONFIG_INCLUDE_FULLCONE)
/****************************************************************************/
static void bcm_nat_expect(struct nf_conn *ct,
			   struct nf_conntrack_expect *exp)
{
	struct nf_nat_range range;

	/* This must be a fresh one. */
	BUG_ON(ct->status & IPS_NAT_DONE_MASK);

	/* Change src to where new ct comes from */
	range.flags = IP_NAT_RANGE_MAP_IPS;
	range.min_ip = range.max_ip =
		ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
	nf_nat_setup_info(ct, &range, IP_NAT_MANIP_SRC);

	/* For DST manip, map port here to where it's expected. */
	range.flags = (IP_NAT_RANGE_MAP_IPS | IP_NAT_RANGE_PROTO_SPECIFIED);
	range.min = range.max = exp->saved_proto;
	range.min_ip = range.max_ip = exp->saved_ip;
	nf_nat_setup_info(ct, &range, IP_NAT_MANIP_DST);
}

/****************************************************************************/
static int bcm_nat_help(struct sk_buff *pskb, unsigned int protoff,
			struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	int dir = CTINFO2DIR(ctinfo);
	struct nf_conn_help *help = nfct_help(ct);
	struct nf_conntrack_expect *exp;

	if (dir != IP_CT_DIR_ORIGINAL || help->expecting[NF_CT_EXPECT_CLASS_DEFAULT])
		return NF_ACCEPT;

	pr_debug("bcm_nat: packet[%d bytes] %u.%u.%u.%u:%hu->%u.%u.%u.%u:%hu, "
	       "reply: %u.%u.%u.%u:%hu->%u.%u.%u.%u:%hu\n",
	       pskb->len,
	       NIPQUAD(ct->tuplehash[dir].tuple.src.u3.ip),
	       ntohs(ct->tuplehash[dir].tuple.src.u.udp.port),
	       NIPQUAD(ct->tuplehash[dir].tuple.dst.u3.ip),
	       ntohs(ct->tuplehash[dir].tuple.dst.u.udp.port),
	       NIPQUAD(ct->tuplehash[!dir].tuple.src.u3.ip),
	       ntohs(ct->tuplehash[!dir].tuple.src.u.udp.port),
	       NIPQUAD(ct->tuplehash[!dir].tuple.dst.u3.ip),
	       ntohs(ct->tuplehash[!dir].tuple.dst.u.udp.port));

	/* Create expect */
	if ((exp = nf_ct_expect_alloc(ct)) == NULL)
		return NF_ACCEPT;

	nf_ct_expect_init(exp, NF_CT_EXPECT_CLASS_DEFAULT, AF_INET, NULL,
				 &ct->tuplehash[!dir].tuple.dst.u3,
				 IPPROTO_UDP, NULL,
				 &ct->tuplehash[!dir].tuple.dst.u.udp.port);
	exp->flags = NF_CT_EXPECT_PERMANENT;
	exp->saved_ip = ct->tuplehash[dir].tuple.src.u3.ip;
	exp->saved_proto.udp.port = ct->tuplehash[dir].tuple.src.u.udp.port;
	exp->dir = !dir;
	exp->expectfn = bcm_nat_expect;

	/* Setup expect */
	nf_ct_expect_related(exp);
	nf_ct_expect_put(exp);
	pr_debug("bcm_nat: expect setup\n");

	return NF_ACCEPT;
}

/****************************************************************************/
static const struct nf_conntrack_expect_policy fullcone_exp_policy __read_mostly = {
		.max_expected   = 1000,
		.timeout        = 240,
};

static struct nf_conntrack_helper nf_conntrack_helper_bcm_nat __read_mostly = {
	.hnode = {NULL, NULL},
	.name = "BCM-NAT",
	.me = THIS_MODULE,
	.expect_policy = &fullcone_exp_policy,
	.tuple.src.l3num = AF_INET,
	.tuple.dst.protonum = IPPROTO_UDP,
	.help = bcm_nat_help,
};

/****************************************************************************/
static inline int find_exp(u_int32_t ip, u_int16_t port, struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);
	struct nf_conntrack_expect * exp;
	struct nf_conntrack_tuple tuple;

	memset(&tuple, 0, sizeof(tuple));
	tuple.dst.u3.ip = ip;
	tuple.dst.u.tcp.port = port;
	tuple.dst.protonum = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;

	exp = nf_ct_expect_find_get(net, nf_ct_zone(ct), &tuple);
	if (exp && exp->master == ct)
		return 1;
	return 0;
}

/****************************************************************************/
static inline struct nf_conntrack_expect *find_fullcone_exp(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);
	struct nf_conntrack_expect * exp;
	struct hlist_node *n;
	struct nf_conntrack_tuple * tp =
		&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
	unsigned int h;

	for (h = 0; h < nf_ct_expect_hsize; h++)
	{
		hlist_for_each_entry_rcu(exp, n, &net->ct.expect_hash[h], hnode) {
			if (exp->saved_ip == tp->src.u3.ip &&
			    exp->saved_proto.all == tp->src.u.all &&
			    exp->tuple.dst.protonum == tp->dst.protonum &&
			    exp->tuple.src.u3.ip == 0 &&
			    exp->tuple.src.u.udp.port == 0)
				return exp;
		}
	}
	return NULL;
}
#endif /* CONFIG_INCLUDE_FULLCONE */

/* FIXME: Multiple targets. --RR */
static int masquerade_tg_check(const struct xt_tgchk_param *par)
{
	const struct nf_nat_multi_range_compat *mr = par->targinfo;

	if (mr->range[0].flags & IP_NAT_RANGE_MAP_IPS) {
		pr_debug("bad MAP_IPS.\n");
		return -EINVAL;
	}
	if (mr->rangesize != 1) {
		pr_debug("bad rangesize %u\n", mr->rangesize);
		return -EINVAL;
	}
	return 0;
}

static unsigned int
masquerade_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct nf_conn *ct;
	struct nf_conn_nat *nat;
	enum ip_conntrack_info ctinfo;
	struct nf_nat_range newrange;
	const struct nf_nat_multi_range_compat *mr;
	const struct rtable *rt;
	__be32 newsrc;

	NF_CT_ASSERT(par->hooknum == NF_INET_POST_ROUTING);

	ct = nf_ct_get(skb, &ctinfo);
	nat = nfct_nat(ct);

	NF_CT_ASSERT(ct && (ctinfo == IP_CT_NEW || ctinfo == IP_CT_RELATED ||
			    ctinfo == IP_CT_RELATED + IP_CT_IS_REPLY));

	/* Source address is 0.0.0.0 - locally generated packet that is
	 * probably not supposed to be masqueraded.
	 */
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip == 0)
		return NF_ACCEPT;

	mr = par->targinfo;
	rt = skb_rtable(skb);
	newsrc = inet_select_addr(par->out, rt->rt_gateway, RT_SCOPE_UNIVERSE);
	if (!newsrc) {
		pr_info("%s ate my IP address\n", par->out->name);
		return NF_DROP;
	}

	nat->masq_index = par->out->ifindex;

#if defined(CONFIG_INCLUDE_FULLCONE)
	if (mr->range[0].min_ip != 0 /* nat_mode == full cone */
		&& nfct_help(ct) == NULL
	    && ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ==
	    IPPROTO_UDP) {
		unsigned int ret;
		u_int16_t minport;
		u_int16_t maxport;
		struct nf_conntrack_expect *exp;

		pr_debug("bcm_nat: need full cone NAT\n");

		/* Choose port */
		LOCK_BH(&nf_conntrack_lock);
		exp = find_fullcone_exp(ct);
		if (exp) {
			minport = maxport = exp->tuple.dst.u.udp.port;
			pr_debug("bcm_nat: existing mapped port = %hu\n",
			       ntohs(minport));
		} else { /* no previous expect */
			u_int16_t newport, tmpport;

			minport = mr->range[0].min.all == 0?
				ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.
				u.udp.port : mr->range[0].min.all;
			maxport = mr->range[0].max.all == 0?
				htons(65535) : mr->range[0].max.all;
			for (newport = ntohs(minport),tmpport = ntohs(maxport);
			     newport <= tmpport; newport++) {
			     	if (!find_exp(newsrc, htons(newport), ct)) {
					pr_debug("bcm_nat: new mapped port = "
					       "%hu\n", newport);
					minport = maxport = htons(newport);
					break;
				}
			}
		}
		UNLOCK_BH(&nf_conntrack_lock);
#if 0
		newrange = ((struct nf_nat_range)
			{ mr->range[0].flags | IP_NAT_RANGE_MAP_IPS |
			  IP_NAT_RANGE_MAP_IPS, newsrc, newsrc,
			  {.udp = {minport}}, {.udp = {maxport}}});
#endif

		newrange.flags = mr->range[0].flags | IP_NAT_RANGE_MAP_IPS |
			IP_NAT_RANGE_PROTO_SPECIFIED;
		newrange.max_ip = newrange.min_ip = newsrc;
		newrange.min.udp.port = newrange.max.udp.port = minport;

		/* Set ct helper */
		ret = nf_nat_setup_info(ct, &newrange, IP_NAT_MANIP_SRC);
		if (ret == NF_ACCEPT) {
			struct nf_conn_help *help;
			help = nf_ct_helper_ext_add(ct, GFP_ATOMIC);
			if (help)
				rcu_assign_pointer(help->helper, &nf_conntrack_helper_bcm_nat);
			pr_debug("bcm_nat: helper set\n");
		}
		return ret;
	}
#endif /* CONFIG_INCLUDE_FULLCONE */

	/* Transfer from original range. */
	newrange = ((struct nf_nat_range)
		{ mr->range[0].flags | IP_NAT_RANGE_MAP_IPS,
		  newsrc, newsrc,
		  mr->range[0].min, mr->range[0].max });

	/* Hand modified range to generic setup. */
	return nf_nat_setup_info(ct, &newrange, IP_NAT_MANIP_SRC);
}

static int
device_cmp(struct nf_conn *i, void *ifindex)
{
	const struct nf_conn_nat *nat = nfct_nat(i);

	if (!nat)
		return 0;

	return nat->masq_index == (int)(long)ifindex;
}

static int masq_device_event(struct notifier_block *this,
			     unsigned long event,
			     void *ptr)
{
	const struct net_device *dev = ptr;
	struct net *net = dev_net(dev);

	if (event == NETDEV_DOWN) {
		/* Device was downed.  Search entire table for
		   conntracks which were associated with that device,
		   and forget them. */
		NF_CT_ASSERT(dev->ifindex != 0);

		nf_ct_iterate_cleanup(net, device_cmp,
				      (void *)(long)dev->ifindex);
	}

	return NOTIFY_DONE;
}

static int masq_inet_event(struct notifier_block *this,
			   unsigned long event,
			   void *ptr)
{
	struct net_device *dev = ((struct in_ifaddr *)ptr)->ifa_dev->dev;
	return masq_device_event(this, event, dev);
}

static struct notifier_block masq_dev_notifier = {
	.notifier_call	= masq_device_event,
};

static struct notifier_block masq_inet_notifier = {
	.notifier_call	= masq_inet_event,
};

static struct xt_target masquerade_tg_reg __read_mostly = {
	.name		= "MASQUERADE",
	.family		= NFPROTO_IPV4,
	.target		= masquerade_tg,
	.targetsize	= sizeof(struct nf_nat_multi_range_compat),
	.table		= "nat",
	.hooks		= 1 << NF_INET_POST_ROUTING,
	.checkentry	= masquerade_tg_check,
	.me		= THIS_MODULE,
};

static int __init masquerade_tg_init(void)
{
	int ret;

	ret = xt_register_target(&masquerade_tg_reg);

#if defined(CONFIG_INCLUDE_FULLCONE)
	nf_conntrack_helper_register(&nf_conntrack_helper_bcm_nat);
#endif

	if (ret == 0) {
		/* Register for device down reports */
		register_netdevice_notifier(&masq_dev_notifier);
		/* Register IP address change reports */
		register_inetaddr_notifier(&masq_inet_notifier);
	}

	return ret;
}

static void __exit masquerade_tg_exit(void)
{
	xt_unregister_target(&masquerade_tg_reg);
	unregister_netdevice_notifier(&masq_dev_notifier);
	unregister_inetaddr_notifier(&masq_inet_notifier);
}

module_init(masquerade_tg_init);
module_exit(masquerade_tg_exit);
