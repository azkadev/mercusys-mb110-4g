/*
 * sfe-cm.c
 *	Shortcut forwarding engine connection manager.
 *
 * Copyright (c) 2013-2016 The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/skbuff.h>
#include <net/route.h>
#include <net/ip6_route.h>
#include <net/addrconf.h>
#include <net/dsfield.h>
#include <linux/inetdevice.h>
#include <linux/netfilter_bridge.h>
#include <linux/netfilter_ipv6.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <linux/netfilter/xt_dscp.h>
#include <linux/if_bridge.h>

#include <linux/sched.h>
#include <linux/random.h>

#include "sfe.h"
#include "sfe_cm.h"
#include "sfe_backport.h"
/* without or "all" prefix will be tcp and udp,or prefix should be tcp or udp */
/* protocol and direction should be in the prefix*/
/* protocol  prefix: udp or tcp  (none means both)*/
/* direction prefix: w2l or l2w  (none means both)*/
static const char *sfe_cm_rule_string[SFE_CM_RULE_MAX] = {
	"smac",
	"dmac",
	"sip",
	"dip",
	"sip_xlate",
	"dip_xlate",
	"sport",
	"dport",
	"sport_xlate",
	"dport_xlate",
	"appid",
	 NULL
};


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
#include <hashtable.h>
#else
#include "hashtable.h"
#endif


typedef enum sfe_cm_exception {
	SFE_CM_EXCEPTION_PACKET_BROADCAST,
	SFE_CM_EXCEPTION_PACKET_MULTICAST,
	SFE_CM_EXCEPTION_NO_IIF,
	SFE_CM_EXCEPTION_NO_CT,
	SFE_CM_EXCEPTION_CT_NO_TRACK,
	SFE_CM_EXCEPTION_CT_NO_CONFIRM,
	SFE_CM_EXCEPTION_CT_IS_ALG,
	SFE_CM_EXCEPTION_IS_IPV4_MCAST,
	SFE_CM_EXCEPTION_IS_IPV6_MCAST,
	SFE_CM_EXCEPTION_TCP_NOT_ASSURED,
	SFE_CM_EXCEPTION_TCP_NOT_ESTABLISHED,
	SFE_CM_EXCEPTION_UNKNOW_PROTOCOL,
	SFE_CM_EXCEPTION_NO_SRC_DEV,
	SFE_CM_EXCEPTION_NO_SRC_XLATE_DEV,
	SFE_CM_EXCEPTION_NO_DEST_DEV,
	SFE_CM_EXCEPTION_NO_DEST_XLATE_DEV,
	SFE_CM_EXCEPTION_NO_BRIDGE,
	SFE_CM_EXCEPTION_LOCAL_OUT,
	SFE_CM_EXCEPTION_MAX
} sfe_cm_exception_t;

static char *sfe_cm_exception_events_string[SFE_CM_EXCEPTION_MAX] = {
	"PACKET_BROADCAST",
	"PACKET_MULTICAST",
	"NO_IIF",
	"NO_CT",
	"CT_NO_TRACK",
	"CT_NO_CONFIRM",
	"CT_IS_ALG",
	"IS_IPV4_MCAST",
	"IS_IPV6_MCAST",
	"TCP_NOT_ASSURED",
	"TCP_NOT_ESTABLISHED",
	"UNKNOW_PROTOCOL",
	"NO_SRC_DEV",
	"NO_SRC_XLATE_DEV",
	"NO_DEST_DEV",
	"NO_DEST_XLATE_DEV",
	"NO_BRIDGE",
	"LOCAL_OUT"
};

/*
 * Per-module structure.
 */
struct sfe_cm {
	spinlock_t lock;		/* Lock for SMP correctness */

	/*
	 * Control state.
	 */
	struct kobject *sys_sfe_cm;	/* sysfs linkage */

	/*
	 * Callback notifiers.
	 */
	struct notifier_block dev_notifier;
					/* Device notifier */
	struct notifier_block inet_notifier;
					/* IPv4 notifier */
	struct notifier_block inet6_notifier;
					/* IPv6 notifier */
	uint32_t exceptions[SFE_CM_EXCEPTION_MAX];
};

struct sfe_cm __sc;

tm_accel_cb_t accel_cb = NULL;
void tm_register_accel_cb(tm_accel_cb_t cb)
{
   accel_cb = cb;
}
EXPORT_SYMBOL(tm_register_accel_cb);

/*
 * Expose the hook for the receive processing.
 */
extern int (*athrs_fast_nat_recv)(struct sk_buff *skb);

/*
 * Expose what should be a static flag in the TCP connection tracker.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
extern int nf_ct_tcp_no_window_check;
#else
int nf_ct_tcp_no_window_check = 0;
#endif

/* added by ZC in 2017/12/16 */
static DEFINE_SPINLOCK(sfe_connections_lock);

struct sfe_connection {
	struct hlist_node hl;
	struct sfe_connection_create *sic;
	struct nf_conn *ct;
	int hits;
	int offloaded;
	bool is_v4;
	unsigned char smac[ETH_ALEN];
	unsigned char dmac[ETH_ALEN];
};

static int offload_at_pkts = 128;
static unsigned short filter_ports_table[] = {80, 443};


#define CPU_LOAD_BEGIN_TO_DROP_SKB    8

#define RANDOM_LIMIT_DROP_SKB	5
#define MIN_SFE_CLIENT_NUM	5
#define MIN_SFE_SI_NUM	1024

static unsigned int drop_cpu_load = CPU_LOAD_BEGIN_TO_DROP_SKB;

static unsigned int drop_random_limit = RANDOM_LIMIT_DROP_SKB;

static unsigned int drop_random_smoothly = 0;

#define SC_CONN_HASH_ORDER 13
static DEFINE_HASHTABLE(sc_conn_ht, SC_CONN_HASH_ORDER);

static int sfe_cm_match_filter_ports(unsigned short dport)
{
	int i;
	DEBUG_TRACE("MATCH FILTER PORTS: %u\n", ntohs(dport));
	for (i = 0; i < sizeof(filter_ports_table)/sizeof(unsigned short); i++)
	{
		if (ntohs(dport) == filter_ports_table[i])
			return 1;
	}
	return 0;
}
static u32 sc_conn_hash(sfe_ip_addr_t *saddr, sfe_ip_addr_t *daddr,
			unsigned short sport, unsigned short dport, bool is_v4)
{
	uint32_t idx, cnt = (is_v4 ? sizeof(saddr->ip) : sizeof(saddr->ip6))/sizeof(uint32_t);
	uint32_t hash = 0;

	for (idx = 0; idx < cnt; idx++) {
		hash ^= ((uint32_t *)saddr)[idx] ^ ((uint32_t *)daddr)[idx];
	}

	return hash ^ (sport | (dport << 16));
}

/*
 * sfe_cm_find_conn()
 * 	find a connection object in the hash table
 *      @pre the sfe_connections_lock must be held before calling this function
 */
static struct sfe_connection *
sfe_cm_find_conn(sfe_ip_addr_t *saddr, sfe_ip_addr_t *daddr,
			  unsigned short sport, unsigned short dport,
			  unsigned char proto, bool is_v4)
{
	struct sfe_connection_create *p_sic;
	struct sfe_connection *conn;
	uint32_t key;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
	struct hlist_node *node;
#endif

	key = sc_conn_hash(saddr, daddr, sport, dport, is_v4);

	sfe_hash_for_each_possible(sc_conn_ht, conn, node, hl, key) {
		if (conn->is_v4 != is_v4) {
			continue;
		}

		p_sic = conn->sic;

		if (p_sic->protocol == proto &&
		    p_sic->src_port == sport &&
		    p_sic->dest_port == dport &&
		    sfe_addr_equal(&p_sic->src_ip, saddr, is_v4) &&
		    sfe_addr_equal(&p_sic->dest_ip, daddr, is_v4)) {
			return conn;
		}
	}

	DEBUG_TRACE("connection not found\n");
	return NULL;
}

/*
 * sfe_cm_destroy_all_conns()
 * 	Destroy connections in sc_conn_ht that match a particular device.
 *  If we pass dev as NULL then this destroys all connections.
 */
static void sfe_cm_destroy_all_conns(struct net_device *dev)
{
	u32 bkt;
	struct sfe_connection *conn;
	struct hlist_node *tmp;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
	struct hlist_node *node;
#endif

	spin_lock_bh(&sfe_connections_lock);
	sfe_hash_for_each_safe(sc_conn_ht, bkt, node, tmp, conn, hl) {
		if (!dev
		    || (dev == conn->sic->src_dev)
		    || (dev == conn->sic->dest_dev)) {
			kfree(conn->sic);
			hash_del(&conn->hl);
			kfree(conn);
		}
	}
	spin_unlock_bh(&sfe_connections_lock);
}


/*
 * sfe_cm_update_protocol()
 * 	Update sfe_ipv4_create struct with new protocol information before we offload
 */
static int sfe_cm_update_protocol(struct sfe_connection_create *p_sic, struct nf_conn *ct)
{
	switch (p_sic->protocol) {
	case IPPROTO_TCP:
		p_sic->src_td_window_scale = ct->proto.tcp.seen[0].td_scale;
		p_sic->src_td_max_window = ct->proto.tcp.seen[0].td_maxwin;
		p_sic->src_td_end = ct->proto.tcp.seen[0].td_end;
		p_sic->src_td_max_end = ct->proto.tcp.seen[0].td_maxend;
		p_sic->dest_td_window_scale = ct->proto.tcp.seen[1].td_scale;
		p_sic->dest_td_max_window = ct->proto.tcp.seen[1].td_maxwin;
		p_sic->dest_td_end = ct->proto.tcp.seen[1].td_end;
		p_sic->dest_td_max_end = ct->proto.tcp.seen[1].td_maxend;
		if (nf_ct_tcp_no_window_check
		    || (ct->proto.tcp.seen[0].flags & IP_CT_TCP_FLAG_BE_LIBERAL)
		    || (ct->proto.tcp.seen[1].flags & IP_CT_TCP_FLAG_BE_LIBERAL)) {
			p_sic->flags |= SFE_CREATE_FLAG_NO_SEQ_CHECK;
		}

		/*
		 * If the connection is shutting down do not manage it.
		 * state can not be SYN_SENT, SYN_RECV because connection is assured
		 * Not managed states: FIN_WAIT, CLOSE_WAIT, LAST_ACK, TIME_WAIT, CLOSE.
		 */
		spin_lock(&ct->lock);
		if (ct->proto.tcp.state != TCP_CONNTRACK_ESTABLISHED) {
			spin_unlock(&ct->lock);
			DEBUG_TRACE("connection in termination state: %#x, s: %pI4:%u, d: %pI4:%u\n",
				    ct->proto.tcp.state, &p_sic->src_ip, ntohs(p_sic->src_port),
				    &p_sic->dest_ip, ntohs(p_sic->dest_port));
			return 0;
		}
		spin_unlock(&ct->lock);
		break;

	case IPPROTO_UDP:
		break;

	default:
		DEBUG_TRACE("unhandled protocol %d\n", p_sic->protocol);
		return 0;
	}

	return 1;
}

/* end added */

/*
 * sfe_cm_incr_exceptions()
 *	increase an exception counter.
 */
static inline void sfe_cm_incr_exceptions(sfe_cm_exception_t except)
{
	struct sfe_cm *sc = &__sc;

	spin_lock_bh(&sc->lock);
	sc->exceptions[except]++;
	spin_unlock_bh(&sc->lock);
}

/**
 * get_avenrun - get the load average array
 * @loads:	pointer to dest load array
 * @offset:	offset to add
 * @shift:	shift count to shift the result left
 *
 * These values are estimates at best, so no need for locking.
 */
static void sfe_get_avenrun(unsigned long *loads, unsigned long offset, int shift)
{
	loads[0] = (avenrun[0] + offset) << shift;
	loads[1] = (avenrun[1] + offset) << shift;
	loads[2] = (avenrun[2] + offset) << shift;
}

static unsigned int checkNeedDropPacket(void)
{
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)
	unsigned long cpu_load= 0;
	unsigned long avnrun[3];
	sfe_get_avenrun(avnrun, FIXED_1/200, 0);
	cpu_load = LOAD_INT(avnrun[0]);
	//printk("\ncheckNeedDropPacket: %lu.%02lu\n",cpu_load,cpu_load2);
	if(drop_random_smoothly)
	{

	}
	else
		{
			if(cpu_load >= drop_cpu_load)
				if(random32() %  drop_random_limit == 0)
				{
					#if 0
					if (net_ratelimit())
					{
						printk("\ncpu_load: %lu is too high, need to drop packet!\n",cpu_load);
					}
					#endif
					return 1;
				}


		}

	return 0;
}


/*
 * sfe_cm_recv()
 *	Handle packet receives.
 *
 * Returns 1 if the packet is forwarded or 0 if it isn't.
 */
int sfe_cm_recv(struct sk_buff *skb)
{
	struct net_device *dev;

	if((htons(ETH_P_IP) == skb->protocol) || (htons(ETH_P_IPV6) == skb->protocol))
	{
		if((sfe_get_tfs_client_number() >= MIN_SFE_CLIENT_NUM) && (sfe_get_ipv4_si_num() >= MIN_SFE_SI_NUM) && checkNeedDropPacket() )
		{
			dev_kfree_skb(skb);
			return 1;

		}
	}


	/*
	 * We know that for the vast majority of packets we need the transport
	 * layer header so we may as well start to fetch it now!
	 */
	prefetch(skb->data + 32);
	barrier();

	dev = skb->dev;

	/*
	 * We're only interested in IPv4 and IPv6 packets.
	 */
	if (likely(htons(ETH_P_IP) == skb->protocol)) {
#if (SFE_HOOK_ABOVE_BRIDGE)
		struct in_device *in_dev;

		/*
		 * Does our input device support IP processing?
		 */
		in_dev = (struct in_device *)dev->ip_ptr;
		if (unlikely(!in_dev)) {
			DEBUG_TRACE("no IP processing for device: %s\n", dev->name);
			return 0;
		}

		/*
		 * Does it have an IP address?  If it doesn't then we can't do anything
		 * interesting here!
		 */
		if (unlikely(!in_dev->ifa_list)) {
			DEBUG_TRACE("no IP address for device: %s\n", dev->name);
			return 0;
		}
#endif

		return sfe_ipv4_recv(dev, skb);
	}

	if (likely(htons(ETH_P_IPV6) == skb->protocol)) {
#if (SFE_HOOK_ABOVE_BRIDGE)
		struct inet6_dev *in_dev;

		/*
		 * Does our input device support IPv6 processing?
		 */
		in_dev = (struct inet6_dev *)dev->ip6_ptr;
		if (unlikely(!in_dev)) {
			DEBUG_TRACE("no IPv6 processing for device: %s\n", dev->name);
			return 0;
		}

		/*
		 * Does it have an IPv6 address?  If it doesn't then we can't do anything
		 * interesting here!
		 */
		if (unlikely(list_empty(&in_dev->addr_list))) {
			DEBUG_TRACE("no IPv6 address for device: %s\n", dev->name);
			return 0;
		}
#endif

		return sfe_ipv6_recv(dev, skb);
	}

	DEBUG_TRACE("not IP packet\n");
	return 0;
}

/*
 * sfe_cm_find_dev_and_mac_addr()
 *	Find the device and MAC address for a given IPv4/IPv6 address.
 *
 * Returns true if we find the device and MAC address, otherwise false.
 *
 * We look up the rtable entry for the address and, from its neighbour
 * structure, obtain the hardware address.  This means this function also
 * works if the neighbours are routers too.
 */
static bool sfe_cm_find_dev_and_mac_addr(sfe_ip_addr_t *addr, struct net_device **dev, uint8_t *mac_addr, int is_v4)
{
	struct neighbour *neigh;
	struct rtable *rt;
	struct rt6_info *rt6;
	struct dst_entry *dst;
	struct net_device *mac_dev;

	/*
	 * Look up the rtable entry for the IP address then get the hardware
	 * address from its neighbour structure.  This means this work when the
	 * neighbours are routers too.
	 */
	if (likely(is_v4)) {
		rt = sfe_ip_route_output(&init_net, addr->ip, 0, 0, 0);
		if (unlikely(!rt)) {
			goto ret_fail;
		}
		if (unlikely(IS_ERR(rt))) {
			goto ret_fail;
		}

		dst = (struct dst_entry *)rt;
	} else {
		rt6 = rt6_lookup(&init_net, (struct in6_addr *)addr->ip6, 0, 0, 0);
		if (unlikely(!rt6)) {
			goto ret_fail;
		}

		dst = (struct dst_entry *)rt6;
	}

	rcu_read_lock();
	neigh = sfe_dst_neigh_lookup(dst, addr);
	if (unlikely(!neigh)) {
		rcu_read_unlock();
		dst_release(dst);
		goto ret_fail;
	}

	if (unlikely(!(neigh->nud_state & NUD_VALID))) {
		rcu_read_unlock();
		neigh_release(neigh);
		dst_release(dst);
		goto ret_fail;
	}

	mac_dev = neigh->dev;
	if (!mac_dev) {
		rcu_read_unlock();
		neigh_release(neigh);
		dst_release(dst);
		goto ret_fail;
	}

	memcpy(mac_addr, neigh->ha, (size_t)mac_dev->addr_len);

	dev_hold(mac_dev);
	*dev = mac_dev;
	rcu_read_unlock();
	neigh_release(neigh);
	dst_release(dst);

	return true;

ret_fail:
	if (is_v4) {
		DEBUG_TRACE("failed to find MAC address for IP: %pI4\n", &addr->ip);

	} else {
		DEBUG_TRACE("failed to find MAC address for IP: %pI6\n", addr->ip6);
	}

	return false;
}

/*
 * sfe_cm_post_routing()
 *	Called for packets about to leave the box - either locally generated or forwarded from another interface
 */
static unsigned int sfe_cm_post_routing(struct sk_buff *skb, int is_v4)
{
	int ret;
	struct sfe_connection_create *p_sic;
	struct sfe_connection *conn;
	u32 key;
	int isfilter;
	struct sfe_connection_create sic;
	struct net_device *in;
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct net_device *dev;
	struct net_device *src_dev;
	struct net_device *dest_dev;
	struct net_device *src_br_dev = NULL;
	struct net_device *dest_br_dev = NULL;
	struct nf_conntrack_tuple orig_tuple;
	struct nf_conntrack_tuple reply_tuple;
	SFE_NF_CONN_ACCT(acct);
	
	/*
	 * Don't process broadcast or multicast packets.
	 */
	if (unlikely(skb->pkt_type == PACKET_BROADCAST)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_PACKET_BROADCAST);
		DEBUG_TRACE("broadcast, ignoring\n");
		return NF_ACCEPT;
	}
	if (unlikely(skb->pkt_type == PACKET_MULTICAST)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_PACKET_MULTICAST);
		DEBUG_TRACE("multicast, ignoring\n");
		return NF_ACCEPT;
	}

#ifdef CONFIG_XFRM
	/*
	 * Packet to xfrm for encapsulation, we can't process it
	 */
	if (unlikely(skb_dst(skb)->xfrm)) {
		DEBUG_TRACE("packet to xfrm, ignoring\n");
		return NF_ACCEPT;
	}
#endif

	/*
	 * Don't process locally generated packets.
	 */
	if (skb->sk) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_LOCAL_OUT);
		DEBUG_TRACE("skip local out packet\n");
		return NF_ACCEPT;
	}

	/*
	 * Don't process packets that are not being forwarded.
	 */
	in = dev_get_by_index(&init_net, skb->skb_iif);
	if (!in) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_IIF);
		DEBUG_TRACE("packet not forwarding\n");
		return NF_ACCEPT;
	}
	
	dev_put(in);

	/*
	 * Don't process packets that aren't being tracked by conntrack.
	 */
	ct = nf_ct_get(skb, &ctinfo);
	if (unlikely(!ct)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_CT);
		DEBUG_TRACE("no conntrack connection, ignoring\n");
		return NF_ACCEPT;
	}

	/*
	 * Don't process untracked connections.
	 */
	if (unlikely(nf_ct_is_untracked(ct))) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_CT_NO_TRACK);
		DEBUG_TRACE("untracked connection\n");
		return NF_ACCEPT;
	}

	/*
	 * Unconfirmed connection may be dropped by Linux at the final step,
	 * So we don't process unconfirmed connections.
	 */
	if (!nf_ct_is_confirmed(ct)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_CT_NO_CONFIRM);
		DEBUG_TRACE("unconfirmed connection\n");
		return NF_ACCEPT;
	}

	/*
	 * Don't process connections that require support from a 'helper' (typically a NAT ALG).
	 */
	if (unlikely(nfct_help(ct))) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_CT_IS_ALG);
		DEBUG_TRACE("connection has helper\n");
		return NF_ACCEPT;
	}

	if (accel_cb && !accel_cb(ct)) {
	 	/*
		 * The DPI need scan more packets so the ct can't be accelerated.
		 */
		return NF_ACCEPT;
	}

	/*
	 * Check if the acceleration of a flow could be rejected quickly.
	 */
	acct = nf_conn_acct_find(ct);
	if (acct) {
		long long packets = atomic64_read(&SFE_ACCT_COUNTER(acct)[CTINFO2DIR(ctinfo)].packets);
		if ((packets > 0xff) && (packets & 0xff)) {
			/*
			 * Connection hits slow path at least 256 times, so it must be not able to accelerate.
			 * But we also give it a chance to walk through ECM every 256 packets
			 */
			return NF_ACCEPT;
		}
	}

	memset(&sic, 0, sizeof(sic));

	/*
	 * Look up the details of our connection in conntrack.
	 *
	 * Note that the data we get from conntrack is for the "ORIGINAL" direction
	 * but our packet may actually be in the "REPLY" direction.
	 */
	orig_tuple = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
	reply_tuple = ct->tuplehash[IP_CT_DIR_REPLY].tuple;
	sic.protocol = (int32_t)orig_tuple.dst.protonum;

	sic.flags = 0;

	/*
	 * Get addressing information, non-NAT first
	 */
	if (likely(is_v4)) {
		uint32_t dscp;

		sic.src_ip.ip = (__be32)orig_tuple.src.u3.ip;
		sic.dest_ip.ip = (__be32)orig_tuple.dst.u3.ip;

		if (ipv4_is_multicast(sic.src_ip.ip) || ipv4_is_multicast(sic.dest_ip.ip)) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_IS_IPV4_MCAST);
			DEBUG_TRACE("multicast address\n");
			return NF_ACCEPT;
		}

		/*
		 * NAT'ed addresses - note these are as seen from the 'reply' direction
		 * When NAT does not apply to this connection these will be identical to the above.
		 */
		sic.src_ip_xlate.ip = (__be32)reply_tuple.dst.u3.ip;
		sic.dest_ip_xlate.ip = (__be32)reply_tuple.src.u3.ip;

		dscp = ipv4_get_dsfield(ip_hdr(skb)) >> XT_DSCP_SHIFT;
		if (dscp) {
			sic.src_dscp = sic.dest_dscp = dscp;
			sic.flags |= SFE_CREATE_FLAG_REMARK_DSCP;
		}
	} else {
		uint32_t dscp;

		sic.src_ip.ip6[0] = *((struct sfe_ipv6_addr *)&orig_tuple.src.u3.in6);
		sic.dest_ip.ip6[0] = *((struct sfe_ipv6_addr *)&orig_tuple.dst.u3.in6);

		if (ipv6_addr_is_multicast((struct in6_addr *)sic.src_ip.ip6) ||
		    ipv6_addr_is_multicast((struct in6_addr *)sic.dest_ip.ip6)) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_IS_IPV6_MCAST);
			DEBUG_TRACE("multicast address\n");
			return NF_ACCEPT;
		}

		/*
		 * NAT'ed addresses - note these are as seen from the 'reply' direction
		 * When NAT does not apply to this connection these will be identical to the above.
		 */
		sic.src_ip_xlate.ip6[0] = *((struct sfe_ipv6_addr *)&reply_tuple.dst.u3.in6);
		sic.dest_ip_xlate.ip6[0] = *((struct sfe_ipv6_addr *)&reply_tuple.src.u3.in6);

		dscp = ipv6_get_dsfield(ipv6_hdr(skb)) >> XT_DSCP_SHIFT;
		if (dscp) {
			sic.src_dscp = sic.dest_dscp = dscp;
			sic.flags |= SFE_CREATE_FLAG_REMARK_DSCP;
		}
	}

	switch (sic.protocol) {
	case IPPROTO_TCP:
		sic.src_port = orig_tuple.src.u.tcp.port;
		sic.dest_port = orig_tuple.dst.u.tcp.port;
		sic.src_port_xlate = reply_tuple.dst.u.tcp.port;
		sic.dest_port_xlate = reply_tuple.src.u.tcp.port;
		sic.src_td_window_scale = ct->proto.tcp.seen[0].td_scale;
		sic.src_td_max_window = ct->proto.tcp.seen[0].td_maxwin;
		sic.src_td_end = ct->proto.tcp.seen[0].td_end;
		sic.src_td_max_end = ct->proto.tcp.seen[0].td_maxend;
		sic.dest_td_window_scale = ct->proto.tcp.seen[1].td_scale;
		sic.dest_td_max_window = ct->proto.tcp.seen[1].td_maxwin;
		sic.dest_td_end = ct->proto.tcp.seen[1].td_end;
		sic.dest_td_max_end = ct->proto.tcp.seen[1].td_maxend;
		if (nf_ct_tcp_no_window_check
		    || (ct->proto.tcp.seen[0].flags & IP_CT_TCP_FLAG_BE_LIBERAL)
		    || (ct->proto.tcp.seen[1].flags & IP_CT_TCP_FLAG_BE_LIBERAL)) {
			sic.flags |= SFE_CREATE_FLAG_NO_SEQ_CHECK;
		}

		/*
		 * Don't try to manage a non-established connection.
		 */
		if (!test_bit(IPS_ASSURED_BIT, &ct->status)) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_TCP_NOT_ASSURED);
			DEBUG_TRACE("non-established connection\n");
			return NF_ACCEPT;
		}

		/*
		 * If the connection is shutting down do not manage it.
		 * state can not be SYN_SENT, SYN_RECV because connection is assured
		 * Not managed states: FIN_WAIT, CLOSE_WAIT, LAST_ACK, TIME_WAIT, CLOSE.
		 */
		spin_lock_bh(&ct->lock);
		if (ct->proto.tcp.state != TCP_CONNTRACK_ESTABLISHED) {
			spin_unlock_bh(&ct->lock);
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_TCP_NOT_ESTABLISHED);
			DEBUG_TRACE("connection in termination state: %#x, s: %pI4:%u, d: %pI4:%u\n",
				    ct->proto.tcp.state, &sic.src_ip, ntohs(sic.src_port),
				    &sic.dest_ip, ntohs(sic.dest_port));
			return NF_ACCEPT;
		}
		spin_unlock_bh(&ct->lock);
		break;

	case IPPROTO_UDP:
		sic.src_port = orig_tuple.src.u.udp.port;
		sic.dest_port = orig_tuple.dst.u.udp.port;
		sic.src_port_xlate = reply_tuple.dst.u.udp.port;
		sic.dest_port_xlate = reply_tuple.src.u.udp.port;

		if(ntohs(sic.dest_port) == 53)
		{
			DEBUG_TRACE("ignore DNS\n");
			return NF_ACCEPT;
		}

		if(skb_has_frags(skb))
		{
			DEBUG_TRACE("ignore frags\n");
			return NF_ACCEPT;
		}

		if (likely(is_v4))
		{
			struct iphdr* iph = ip_hdr(skb);
			if (iph && ntohl(iph->saddr) != ntohl(sic.src_ip_xlate.ip)
				&& ntohl(iph->saddr) != ntohl(sic.src_ip.ip))
			{
				DEBUG_TRACE("ignore reply");
				return NF_ACCEPT;
			}
		}
		break;

	default:
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_UNKNOW_PROTOCOL);
		DEBUG_TRACE("unhandled protocol %d\n", sic.protocol);
		return NF_ACCEPT;
	}

#ifdef CONFIG_XFRM
	sic.original_accel = 1;
	sic.reply_accel = 1;

	/*
	 * For packets de-capsulated from xfrm, we still can accelerate it
	 * on the direction we just received the packet.
	 */
	if (unlikely(skb->sp)) {
		if (sic.protocol == IPPROTO_TCP &&
			!(sic.flags & SFE_CREATE_FLAG_NO_SEQ_CHECK)) {
			return NF_ACCEPT;
		}

		if (CTINFO2DIR(ctinfo) == IP_CT_DIR_ORIGINAL) {
			sic.reply_accel = 0;
		} else {
			sic.original_accel = 0;
		}
	}
#endif

	/*
	 * Get QoS information
	 */
	if (skb->priority) {
		sic.src_priority = sic.dest_priority = skb->priority;
		sic.flags |= SFE_CREATE_FLAG_REMARK_PRIORITY;
	}

	isfilter = sfe_cm_match_filter_ports(sic.dest_port);

	if (isfilter)
	{
		/*
		 * If we already have this connection in our list, skip it
		 * XXX: this may need to be optimized
		 */
		spin_lock_bh(&sfe_connections_lock);

		conn = sfe_cm_find_conn(&sic.src_ip, &sic.dest_ip, sic.src_port, sic.dest_port, sic.protocol, is_v4);
		if (conn) {
			conn->hits++;

			if (!conn->offloaded) {
				if (conn->hits >= offload_at_pkts) {
					DEBUG_TRACE("OFFLOADING CONNECTION, TOO MANY HITS\n");

					if (sfe_cm_update_protocol(conn->sic, conn->ct) == 0) {
						spin_unlock_bh(&sfe_connections_lock);
						DEBUG_TRACE("UNKNOWN PROTOCOL OR CONNECTION CLOSING, SKIPPING\n");
						return NF_ACCEPT;
					}

					DEBUG_TRACE("INFO: calling sfe rule creation!\n");
					spin_unlock_bh(&sfe_connections_lock);

					ret = is_v4 ? sfe_ipv4_create_rule(conn->sic) : sfe_ipv6_create_rule(conn->sic);
					if ((ret == 0) || (ret == -EADDRINUSE)) {
						conn->offloaded = 1;
					}
					return NF_ACCEPT;
				}
				DEBUG_TRACE("NEED MORE PKTS\n");
			}

			if (conn->offloaded) {
				DEBUG_TRACE("UPDATE RULE\n");
				if (sfe_cm_update_protocol(conn->sic, conn->ct) == 0) {
					spin_unlock_bh(&sfe_connections_lock);
					DEBUG_TRACE("UNKNOWN PROTOCOL OR CONNECTION CLOSING, SKIPPING\n");
					return NF_ACCEPT;
				}
				ret = is_v4 ? sfe_ipv4_update_rule(conn->sic) : sfe_ipv6_update_rule(conn->sic);
				if (ret) {
					DEBUG_TRACE("Free connection (rule not found)\n");
					kfree(conn->sic);
					hash_del(&conn->hl);
					kfree(conn);
				}
			};
			spin_unlock_bh(&sfe_connections_lock);

			DEBUG_TRACE("FOUND, SKIPPING\n");
			return NF_ACCEPT;
		}

		DEBUG_TRACE("TO MAKE NEW CONNTRACK\n");
		spin_unlock_bh(&sfe_connections_lock);
	}

	/*
	 * Get the net device and MAC addresses that correspond to the various source and
	 * destination host addresses.
	 */
	if (!sfe_cm_find_dev_and_mac_addr(&sic.src_ip, &src_dev, sic.src_mac, is_v4)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_SRC_DEV);
		return NF_ACCEPT;
	}

	if (!sfe_cm_find_dev_and_mac_addr(&sic.src_ip_xlate, &dev, sic.src_mac_xlate, is_v4)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_SRC_XLATE_DEV);
		goto done1;
	}

	dev_put(dev);

	if (!sfe_cm_find_dev_and_mac_addr(&sic.dest_ip, &dev, sic.dest_mac, is_v4)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_DEST_DEV);
		goto done1;
	}

	dev_put(dev);

	if (!sfe_cm_find_dev_and_mac_addr(&sic.dest_ip_xlate, &dest_dev, sic.dest_mac_xlate, is_v4)) {
		sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_DEST_XLATE_DEV);
		goto done1;
	}

#if (!SFE_HOOK_ABOVE_BRIDGE)
	/*
	 * Now our devices may actually be a bridge interface.  If that's
	 * the case then we need to hunt down the underlying interface.
	 */
	if (src_dev->priv_flags & IFF_EBRIDGE) {
		src_br_dev = sfe_br_port_dev_get(src_dev, sic.src_mac);
		if (!src_br_dev) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_BRIDGE);
			DEBUG_TRACE("no port found on bridge\n");
			goto done2;
		}

		src_dev = src_br_dev;
	}

	if (dest_dev->priv_flags & IFF_EBRIDGE) {
		dest_br_dev = sfe_br_port_dev_get(dest_dev, sic.dest_mac_xlate);
		if (!dest_br_dev) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_BRIDGE);
			DEBUG_TRACE("no port found on bridge\n");
			goto done3;
		}

		dest_dev = dest_br_dev;
	}
#else
	/*
	 * Our devices may actually be part of a bridge interface.  If that's
	 * the case then find the bridge interface instead.
	 */
	if (src_dev->priv_flags & IFF_BRIDGE_PORT) {
		src_br_dev = sfe_dev_get_master(src_dev);
		if (!src_br_dev) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_BRIDGE);
			DEBUG_TRACE("no bridge found for: %s\n", src_dev->name);
			goto done2;
		}

		src_dev = src_br_dev;
	}

	if (dest_dev->priv_flags & IFF_BRIDGE_PORT) {
		dest_br_dev = sfe_dev_get_master(dest_dev);
		if (!dest_br_dev) {
			sfe_cm_incr_exceptions(SFE_CM_EXCEPTION_NO_BRIDGE);
			DEBUG_TRACE("no bridge found for: %s\n", dest_dev->name);
			goto done3;
		}

		dest_dev = dest_br_dev;
	}
#endif

	sic.src_dev = src_dev;
	sic.dest_dev = dest_dev;

	sic.src_mtu = src_dev->mtu;
	sic.dest_mtu = dest_dev->mtu;

	sic.mark = ct->mark;

	if (isfilter)
	{
		conn = kmalloc(sizeof(struct sfe_connection), GFP_ATOMIC);
		if (conn == NULL) {
			printk(KERN_CRIT "ERROR: no memory for sfe\n");
			goto done3;
		}
		conn->hits = 0;
		conn->offloaded = 0;
		conn->is_v4 = is_v4;
		DEBUG_TRACE("Source MAC=%pM\n", sic.src_mac);
		memcpy(conn->smac, sic.src_mac, ETH_ALEN);
		memcpy(conn->dmac, sic.dest_mac_xlate, ETH_ALEN);
	
		p_sic = kmalloc(sizeof(struct sfe_connection_create), GFP_ATOMIC);
		if (p_sic == NULL) {
			printk(KERN_CRIT "ERROR: no memory for sfe\n");
			kfree(conn);
			goto done3;
		}

		memcpy(p_sic, &sic, sizeof(sic));
		conn->sic = p_sic;
		conn->ct = ct;
		spin_lock_bh(&sfe_connections_lock);
		key = sc_conn_hash(&conn->sic->src_ip,
				   &conn->sic->dest_ip,
				   conn->sic->src_port,
				   conn->sic->dest_port,
				   is_v4);
		hash_add(sc_conn_ht, &conn->hl, key);
		spin_unlock_bh(&sfe_connections_lock);
	}
	else
	{
		if (likely(is_v4)) {
			sfe_ipv4_create_rule(&sic);
		} else {
			sfe_ipv6_create_rule(&sic);
		}
	}

	/*
	 * If we had bridge ports then release them too.
	 */
	if (dest_br_dev) {
		dev_put(dest_br_dev);
	}

done3:
	if (src_br_dev) {
		dev_put(src_br_dev);
	}

done2:
	dev_put(dest_dev);

done1:
	dev_put(src_dev);

	return NF_ACCEPT;
}

/*
 * sfe_cm_ipv4_post_routing_hook()
 *	Called for packets about to leave the box - either locally generated or forwarded from another interface
 */
sfe_cm_ipv4_post_routing_hook(hooknum, ops, skb, in_unused, out, okfn)
{
	return sfe_cm_post_routing(skb, true);
}

/*
 * sfe_cm_ipv6_post_routing_hook()
 *	Called for packets about to leave the box - either locally generated or forwarded from another interface
 */
sfe_cm_ipv6_post_routing_hook(hooknum, ops, skb, in_unused, out, okfn)
{
	return sfe_cm_post_routing(skb, false);
}


#ifdef CONFIG_NF_CONNTRACK_EVENTS
/*
 * sfe_cm_conntrack_event()
 *	Callback event invoked when a conntrack connection's state changes.
 */
#ifdef CONFIG_NF_CONNTRACK_CHAIN_EVENTS
static int sfe_cm_conntrack_event(struct notifier_block *this,
			unsigned long events, void *ptr)
#else
static int sfe_cm_conntrack_event(unsigned int events, struct nf_ct_event *item)
#endif
{
#ifdef CONFIG_NF_CONNTRACK_CHAIN_EVENTS
	struct nf_ct_event *item = ptr;
#endif
	struct sfe_connection_destroy sid;
	struct nf_conn *ct = item->ct;
	struct nf_conntrack_tuple orig_tuple;
	bool is_v4;
	struct sfe_connection *conn;

	/*
	 * If we don't have a conntrack entry then we're done.
	 */
	if (unlikely(!ct)) {
		DEBUG_WARN("no ct in conntrack event callback\n");
		return NOTIFY_DONE;
	}

	/*
	 * If this is an untracked connection then we can't have any state either.
	 */
	if (unlikely(nf_ct_is_untracked(ct))) {
		DEBUG_TRACE("ignoring untracked conn\n");
		return NOTIFY_DONE;
	}

	orig_tuple = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
	sid.protocol = (int32_t)orig_tuple.dst.protonum;

	/*
	 * Extract information from the conntrack connection.  We're only interested
	 * in nominal connection information (i.e. we're ignoring any NAT information).
	 */
	switch (sid.protocol) {
	case IPPROTO_TCP:
		sid.src_port = orig_tuple.src.u.tcp.port;
		sid.dest_port = orig_tuple.dst.u.tcp.port;
		break;

	case IPPROTO_UDP:
		sid.src_port = orig_tuple.src.u.udp.port;
		sid.dest_port = orig_tuple.dst.u.udp.port;
		break;

	default:
		DEBUG_TRACE("unhandled protocol: %d\n", sid.protocol);
		return NOTIFY_DONE;
	}

	if (likely(nf_ct_l3num(ct) == AF_INET)) {
		sid.src_ip.ip = (__be32)orig_tuple.src.u3.ip;
		sid.dest_ip.ip = (__be32)orig_tuple.dst.u3.ip;
		is_v4 = true;
	} else if (likely(nf_ct_l3num(ct) == AF_INET6)) {
		sid.src_ip.ip6[0] = *((struct sfe_ipv6_addr *)&orig_tuple.src.u3.in6);
		sid.dest_ip.ip6[0] = *((struct sfe_ipv6_addr *)&orig_tuple.dst.u3.in6);
		is_v4 = false;
	} else {
		DEBUG_TRACE("ignoring non-IPv4 and non-IPv6 connection\n");
		return NOTIFY_DONE;
	}

	/*
	 * Check for an updated mark
	 */
	if ((events & (1 << IPCT_MARK)) && (ct->mark != 0)) {
		struct sfe_connection_mark mark;

		mark.protocol = sid.protocol;
		mark.src_ip = sid.src_ip;
		mark.dest_ip = sid.dest_ip;
		mark.src_port = sid.src_port;
		mark.dest_port = sid.dest_port;
		mark.mark = ct->mark;

		is_v4 ? sfe_ipv4_mark_rule(&mark) : sfe_ipv6_mark_rule(&mark);
	}

	/*
	 * We're only interested in destroy events.
	 */
	if (unlikely(!(events & (1 << IPCT_DESTROY)))) {
		DEBUG_TRACE("ignoring non-destroy event\n");
		return NOTIFY_DONE;
	}

	spin_lock_bh(&sfe_connections_lock);

	conn = sfe_cm_find_conn(&sid.src_ip, &sid.dest_ip, sid.src_port, sid.dest_port, sid.protocol, is_v4);

	if (conn) {
		DEBUG_TRACE("Free connection: proto: %d src_ip: %pI4 dst_ip: %pI4, src_port: %u, dst_port: %u\n",
			sid.protocol, &sid.src_ip, &sid.dest_ip, ntohs(sid.src_port), ntohs(sid.dest_port));
		kfree(conn->sic);
		hash_del(&conn->hl);
		kfree(conn);
	}

	spin_unlock_bh(&sfe_connections_lock);

	is_v4 ? sfe_ipv4_destroy_rule(&sid) : sfe_ipv6_destroy_rule(&sid);

	return NOTIFY_DONE;
}

/*
 * Netfilter conntrack event system to monitor connection tracking changes
 */
#ifdef CONFIG_NF_CONNTRACK_CHAIN_EVENTS
static struct notifier_block sfe_cm_conntrack_notifier = {
	.notifier_call = sfe_cm_conntrack_event,
};
#else
static struct nf_ct_event_notifier sfe_cm_conntrack_notifier = {
	.fcn = sfe_cm_conntrack_event,
};
#endif
#endif

/*
 * Structure to establish a hook into the post routing netfilter point - this
 * will pick up local outbound and packets going from one interface to another.
 *
 * Note: see include/linux/netfilter_ipv4.h for info related to priority levels.
 * We want to examine packets after NAT translation and any ALG processing.
 */
static struct nf_hook_ops sfe_cm_ops_post_routing[] __read_mostly = {
	SFE_IPV4_NF_POST_ROUTING_HOOK(__sfe_cm_ipv4_post_routing_hook),
#ifdef SFE_SUPPORT_IPV6
	SFE_IPV6_NF_POST_ROUTING_HOOK(__sfe_cm_ipv6_post_routing_hook),
#endif
};

/*
 * sfe_cm_sync_rule()
 *	Synchronize a connection's state.
 */
static void sfe_cm_sync_rule(struct sfe_connection_sync *sis)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conntrack_tuple tuple;
	struct nf_conn *ct;
	SFE_NF_CONN_ACCT(acct);

	tfs_update_client_sync_cb(sis);

	/*
	 * Create a tuple so as to be able to look up a connection
	 */
	memset(&tuple, 0, sizeof(tuple));
	tuple.src.u.all = (__be16)sis->src_port;
	tuple.dst.dir = IP_CT_DIR_ORIGINAL;
	tuple.dst.protonum = (uint8_t)sis->protocol;
	tuple.dst.u.all = (__be16)sis->dest_port;

	if (sis->is_v6) {
		tuple.src.u3.in6 = *((struct in6_addr *)sis->src_ip.ip6);
		tuple.dst.u3.in6 = *((struct in6_addr *)sis->dest_ip.ip6);
		tuple.src.l3num = AF_INET6;

		DEBUG_TRACE("update connection - p: %d, s: %pI6:%u, d: %pI6:%u\n",
			    (int)tuple.dst.protonum,
			    &tuple.src.u3.in6, (unsigned int)ntohs(tuple.src.u.all),
			    &tuple.dst.u3.in6, (unsigned int)ntohs(tuple.dst.u.all));
	} else {
		tuple.src.u3.ip = sis->src_ip.ip;
		tuple.dst.u3.ip = sis->dest_ip.ip;
		tuple.src.l3num = AF_INET;

		DEBUG_TRACE("update connection - p: %d, s: %pI4:%u, d: %pI4:%u\n",
			    (int)tuple.dst.protonum,
			    &tuple.src.u3.ip, (unsigned int)ntohs(tuple.src.u.all),
			    &tuple.dst.u3.ip, (unsigned int)ntohs(tuple.dst.u.all));
	}

	/*
	 * Look up conntrack connection
	 */
	h = nf_conntrack_find_get(&init_net, SFE_NF_CT_DEFAULT_ZONE, &tuple);
	if (unlikely(!h)) {
		DEBUG_TRACE("no connection found\n");
		return;
	}

	ct = nf_ct_tuplehash_to_ctrack(h);
	NF_CT_ASSERT(ct->timeout.data == (unsigned long)ct);

	/*
	 * Only update if this is not a fixed timeout
	 */
	if (!test_bit(IPS_FIXED_TIMEOUT_BIT, &ct->status)) {
		spin_lock_bh(&ct->lock);
		ct->timeout.expires += sis->delta_jiffies;
		spin_unlock_bh(&ct->lock);
	}

	acct = nf_conn_acct_find(ct);
	if (acct) {
		spin_lock_bh(&ct->lock);
		atomic64_set(&SFE_ACCT_COUNTER(acct)[IP_CT_DIR_ORIGINAL].packets, sis->src_packet_count);
		atomic64_set(&SFE_ACCT_COUNTER(acct)[IP_CT_DIR_ORIGINAL].bytes, sis->src_byte_count);
		atomic64_set(&SFE_ACCT_COUNTER(acct)[IP_CT_DIR_REPLY].packets, sis->dest_packet_count);
		atomic64_set(&SFE_ACCT_COUNTER(acct)[IP_CT_DIR_REPLY].bytes, sis->dest_byte_count);
		spin_unlock_bh(&ct->lock);
	}

	switch (sis->protocol) {
	case IPPROTO_TCP:
		spin_lock_bh(&ct->lock);
		if (ct->proto.tcp.seen[0].td_maxwin < sis->src_td_max_window) {
			DEBUG_TRACE("Update ct->proto.tcp.seen[0].td_maxwin: %u -> %u\n", ct->proto.tcp.seen[0].td_maxwin, sis->src_td_max_window);
			ct->proto.tcp.seen[0].td_maxwin = sis->src_td_max_window;
		}
		if ((int32_t)(ct->proto.tcp.seen[0].td_end - sis->src_td_end) < 0) {
			DEBUG_INFO("Update ct->proto.tcp.seen[0].td_end: %u -> %u\n", ct->proto.tcp.seen[0].td_end, sis->src_td_end);
			ct->proto.tcp.seen[0].td_end = sis->src_td_end;
		}
		if ((int32_t)(ct->proto.tcp.seen[0].td_maxend - sis->src_td_max_end) < 0) {
			DEBUG_INFO("Update ct->proto.tcp.seen[0].td_maxend: %u -> %u\n", ct->proto.tcp.seen[0].td_maxend, sis->src_td_max_end);
			ct->proto.tcp.seen[0].td_maxend = sis->src_td_max_end;
		}
		if (ct->proto.tcp.seen[1].td_maxwin < sis->dest_td_max_window) {
			DEBUG_TRACE("Update ct->proto.tcp.seen[1].td_maxwin: %u -> %u\n", ct->proto.tcp.seen[1].td_maxwin, sis->dest_td_max_window);
			ct->proto.tcp.seen[1].td_maxwin = sis->dest_td_max_window;
		}
		if ((int32_t)(ct->proto.tcp.seen[1].td_end - sis->dest_td_end) < 0) {
			DEBUG_INFO("Update ct->proto.tcp.seen[1].td_end: %u -> %u\n", ct->proto.tcp.seen[1].td_end, sis->dest_td_end);
			ct->proto.tcp.seen[1].td_end = sis->dest_td_end;
		}
		if ((int32_t)(ct->proto.tcp.seen[1].td_maxend - sis->dest_td_max_end) < 0) {
			DEBUG_INFO("Update ct->proto.tcp.seen[1].td_maxend: %u -> %u\n", ct->proto.tcp.seen[1].td_maxend, sis->dest_td_max_end);
			ct->proto.tcp.seen[1].td_maxend = sis->dest_td_max_end;
		}
		spin_unlock_bh(&ct->lock);
		break;
	}

	/*
	 * Release connection
	 */
	nf_ct_put(ct);
}

/*
 * sfe_cm_device_event()
 */
int sfe_cm_device_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct net_device *dev = SFE_DEV_EVENT_PTR(ptr);

	switch (event) {
	case NETDEV_DOWN:
		if (dev) {
			sfe_ipv4_destroy_all_rules_for_dev(dev);
			sfe_ipv6_destroy_all_rules_for_dev(dev);
			sfe_cm_destroy_all_conns(dev);
		}
		break;
	}

	return NOTIFY_DONE;
}

/*
 * sfe_cm_inet_event()
 */
static int sfe_cm_inet_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct net_device *dev = ((struct in_ifaddr *)ptr)->ifa_dev->dev;
	return sfe_propagate_dev_event(sfe_cm_device_event, this, event, dev);
}

/*
 * sfe_cm_inet6_event()
 */
static int sfe_cm_inet6_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct net_device *dev = ((struct inet6_ifaddr *)ptr)->idev->dev;
	return sfe_propagate_dev_event(sfe_cm_device_event, this, event, dev);
}

/*
 * sfe_cm_get_exceptions
 * 	dump exception counters
 */
static ssize_t sfe_cm_get_exceptions(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int idx, len;
	struct sfe_cm *sc = &__sc;

	spin_lock_bh(&sc->lock);
	for (len = 0, idx = 0; idx < SFE_CM_EXCEPTION_MAX; idx++) {
		if (sc->exceptions[idx]) {
			len += snprintf(buf + len, (ssize_t)(PAGE_SIZE - len), "%s = %d\n", sfe_cm_exception_events_string[idx], sc->exceptions[idx]);
		}
	}
	spin_unlock_bh(&sc->lock);

	return len;
}

/*
 * sfe_cm_get_no_window_check()
 */
static ssize_t sfe_cm_get_no_window_check(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", nf_ct_tcp_no_window_check);
}

/*
 * sfe_cm_set_no_window_check()
 */
static ssize_t sfe_cm_set_no_window_check(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	long new;
	int ret;

	ret = strict_strtol(buf, 0, &new);
	if (ret == -EINVAL || ((int)new != new))
		return -EINVAL;

	nf_ct_tcp_no_window_check = new ? 1 : 0;

	return size;
}

#define FLAG_CLR  0
#define FLAG_SET  1
#define FLAG_HIG  2
#define FLAG_MID  3
#define FLAG_LOW  4
#define FLAG_TCP  5
#define FLAG_UDP  6
#define FLAG_W2L  7
#define FLAG_L2W  8
#define FLAG_MAX  9

static int parse_rule(ctlrule_t * rule, char *key, char *value)
{
	int  ret = 0;
	char *flags[FLAG_MAX], *max_flag_addr = NULL;
	memset(rule, 0, sizeof(ctlrule_t));
	memset(flags, 0, sizeof(flags));

	flags[FLAG_CLR] = strstr(key,"clr");
	flags[FLAG_SET] = strstr(key,"set");
	if (flags[FLAG_CLR] && flags[FLAG_SET])
	{
		printk("sfe error: clr and set prefixes cannot exist at the same time!");
		return -1;
	}
	flags[FLAG_HIG] = strstr(key,"hig");
	flags[FLAG_MID] = strstr(key,"mid");
	flags[FLAG_LOW] = strstr(key,"low");
	if      (flags[FLAG_HIG] && !flags[FLAG_MID]  && !flags[FLAG_LOW]) rule->flag |= IQOS_CLASS_HIGH;
	else if (!flags[FLAG_HIG] && !flags[FLAG_LOW])                     rule->flag |= IQOS_CLASS_MID;
	else if (!flags[FLAG_HIG] && !flags[FLAG_MID] && flags[FLAG_LOW])  rule->flag |= IQOS_CLASS_LOW;
	else {printk("sfe error: wrong priority!\n"); return -1;}


	flags[FLAG_TCP] = strstr(key,"tcp");
	flags[FLAG_UDP] = strstr(key,"udp");
	flags[FLAG_W2L] = strstr(key,"w2l");
	flags[FLAG_L2W] = strstr(key,"l2w");
	if (flags[FLAG_TCP])               rule->flag |= SFE_CM_CTL_PROTO_TCP;
	if (flags[FLAG_UDP])               rule->flag |= SFE_CM_CTL_PROTO_UDP;
	if (!flags[FLAG_TCP] && !flags[FLAG_UDP]) 	rule->flag |= SFE_CM_CTL_PROTO_ALL;
	if (flags[FLAG_W2L])               rule->flag |= SFE_CM_CTL_DIR_W2L;
	if (flags[FLAG_L2W])               rule->flag |= SFE_CM_CTL_DIR_L2W;
	if (!flags[FLAG_W2L] && !flags[FLAG_L2W]) 	rule->flag |= SFE_CM_CTL_DIR_ALL;

	/* default clear tcp and udp mark(memset all 0), so just process the "set" situation */
	if (flags[FLAG_SET])
	{
		if (rule->flag & SFE_CM_CTL_PROTO_TCP) rule->flag |= SFE_CM_CTL_MARK_TCP;
		if (rule->flag & SFE_CM_CTL_PROTO_UDP) rule->flag |= SFE_CM_CTL_MARK_UDP;
	}
	const char *temp_key = key;
	int k = 0;
	for (; k < FLAG_MAX; ++k)
	{
		if (max_flag_addr < flags[k])
			max_flag_addr = flags[k];
	}
	if (max_flag_addr != NULL)
	{
		temp_key = max_flag_addr + 3;   //tcp udp l2w w2l set clr hig low mid are 3bytes
	}
	while(*temp_key == '_') ++temp_key;  //after prefix,there may be several '_'

	int j = 0;
	for (; j < SFE_CM_RULE_MAX; ++j)
	{
		if (sfe_cm_rule_string[j] && strcmp(temp_key, sfe_cm_rule_string[j]) == 0)
		{
			rule->type = j;
			break;
		}
	}
	if (j >= SFE_CM_RULE_MAX)
	{
		printk("sfe error: cannot parse rule type! (type=%s)\n",temp_key);
		return -1;
	}

	switch(rule->type)
	{
		case SFE_CM_RULE_SMAC:
		case SFE_CM_RULE_DMAC:
			 {
				char *mac_string = value, *mac_temp;
				int   mac_int, mac_count = 0;
				while((mac_temp = strsep(&mac_string,":-.")) != NULL)
				{
					if (*mac_temp != 0)
					{
						mac_int = -1;
						sscanf(mac_temp,"%x",&mac_int);
						if (mac_count >= ETH_ALEN || mac_int < 0 || mac_int > 255)
						{
							printk("sfe error:invalid mac address(mac=%s)\n",mac_temp);
							return -1;
						}
						else
						{
							rule->rule.mac[mac_count++] = mac_int & 0xff;
						}
					}
				}

				printk("sfe parsed mac %x:%x:%x:%x:%x:%x\n",
				       rule->rule.mac[0],rule->rule.mac[1],rule->rule.mac[2],
				       rule->rule.mac[3],rule->rule.mac[4],rule->rule.mac[5]);
			 }
			 break;
		case SFE_CM_RULE_SIP:
		case SFE_CM_RULE_DIP:
		case SFE_CM_RULE_SIP_XLATE:
		case SFE_CM_RULE_DIP_XLATE:
		case SFE_CM_RULE_APPID:
			printk("sfe error: cannot parse %s=%s   not support!", key,value);
			ret = -1;
			break;

		case SFE_CM_RULE_SPORT:
		case SFE_CM_RULE_DPORT:
		case SFE_CM_RULE_SPORT_XLATE:
		case SFE_CM_RULE_DPORT_XLATE:
			{
				 long temp_port_num;
				 uint16_t port_num1 = 0,port_num2 = 0;
				 int   ret;

				 char  *separator = strchr(value,'-');
				 if (separator != NULL)
				 {
				 	*separator++ = 0;
					while(*separator == '-') ++separator;
				 }

				 ret = strict_strtol(value, 0, &temp_port_num);
				 if (ret == -EINVAL || temp_port_num >= 65536 || temp_port_num <= 0)
				 {
					 printk("sfe error: invalid port  %s\n", value);
					 return -1;
				 }
				 port_num1 = (uint16_t)temp_port_num;

				 if (separator)
				 {
					 ret = strict_strtol(separator, 0, &temp_port_num);
					 if (ret == -EINVAL || temp_port_num >= 65536 || temp_port_num <= 0)
					 {
						 printk("sfe error: invalid port  %s\n", separator);
						 return -1;
					 }
					 port_num2 = (uint16_t)temp_port_num;
				 }

				 if (separator == NULL)
				 {
					rule->rule.port.min_port = port_num1;
					rule->rule.port.max_port = port_num1;
				 }
				 else
				 {
					rule->rule.port.min_port = min(port_num1, port_num2);
					rule->rule.port.max_port = max(port_num1, port_num2);
				 }
			}
			break;

		default:
			printk("sfe error: can't parse rule -no such type\n");
			ret = -1;
			break;
	}
	return ret;
}
#undef FLAG_CLR
#undef FLAG_SET
#undef FLAG_HIG
#undef FLAG_MID
#undef FLAG_LOW
#undef FLAG_TCP
#undef FLAG_UDP
#undef FLAG_W2L
#undef FLAG_L2W
#undef FLAG_MAX

/*
 * sfe_cm_display_ctl_conn_help
 * 	show help information about ctl_conn
 */
static ssize_t sfe_cm_display_ctl_conn_help(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int len = 0;

	len = snprintf(buf + len, (ssize_t)(PAGE_SIZE - len),
	"    echo command to this file to set or clear iQoS mark bits.\n"
	"\n"
	"    the command is made up of prefix and network parameter.\n"
	"    the prefix could be any combinatoin of udp,tcp,w2l,l2w,set\n"
	"or clr(except set and clr exist at the same time) and they mean:\n"
	"        tcp,udp--the rule matches tcp or udp,none also means both\n"
	"        w2l,l2w--WAN to LAN or LAN to WAN flow,none also means both\n"
	"        set,clr--set or clear the QoS mark,none means clear\n"
	"        hig,mid,low--priority of the flow,none means middle\n"
	"    the network parameters could be smac,dmac,sip,dip,sip_xlate,\n"
	"dip_xlate,sport,dport,sport_xlate,dport_xlate,appid. \"s\" means \n"
	"\"source\", \"d\" means \"destination\",\"xlate\" means \"after nat\"\n"
	"    rules are separated by \"|\", subrules are separated by \",\",\n"
	"a connection is processed when it matches all subrules of a rule.their\n"
	"relationship is like:\n"
	"\n"
	"    rule0(subrule00)----rule1(subrule10)-----rule2(subrule20)------...\n"
	"          |                    |                   |\n"
	"      subrule01            subrule11           subrule21\n"
	"          |                    |                   |\n"
	"      subrule02            subrule12           subrule22\n"
	"          |                    |                   |\n"
	"         ...                  ...                 ...\n"
	"\n"
	"    examples:\n"
	"    1.smac=11:22:33:44:55:66\n"
	"    2.smac=11:22:33:44:55:66,sport=12345\n"
	"    3.smac=11:22:33:44:55:66|sport=12345\n"
	"    4.set_w2l_tcp_sport=12345,set_w2l_tcp_sip=1.2.3.4|sport=23333\n"
	"\n"
	"    for more information, please read the code.\n"
	"\n"
	"    dev note: smac and dmac do the same operation now(2017.12.18)\n");
	return len;
}

/*
 * sfe_cm_ctl_conn()
 * the parsed rule looks like
 *
 *    rule0(subrule00)----rule1(subrule10)-----rule2(subrule20)------...
 *         |                    |                   |
 *     subrule01            subrule11           subrule21
 *         |                    |                   |
 *     subrule02            subrule12           subrule22
 *         |                    |                   |
 *        ...                  ...                 ...
 */
static ssize_t sfe_cm_ctl_conn(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	size_t i = 0, valid = 0;
	char *input_buf = NULL, *rule = NULL, *sub_rule = NULL, *cur_rule = NULL, *cur_sub_rule = NULL;
	ctlrule_t *ctl_rules = NULL;
	ctlrule_t *temp_ctl_rules, *ctl_subs,*temp_ctl_subs;

	static const char skipchar[] = " \r\n\t";

	if (buf == NULL || size == 0)
		return 0;

	input_buf = (char *)kmalloc(size + 1, GFP_ATOMIC);
	if (input_buf == NULL)
		return -1;

	for (i = 0; i < size && buf[i] != 0; ++i)
	{
		if (strchr(skipchar,buf[i]) == NULL)
			input_buf[valid++] = tolower(buf[i]);
	}
	input_buf[valid] = 0;

	rule = input_buf;
	while((cur_rule = strsep(&rule, "|")) != NULL)
	{
		if (*cur_rule != 0)
		{
			sub_rule = cur_rule;
			ctlrule_t  *temp_rule = NULL;
			uint32_t   ctl_flag = 0;
			while ((cur_sub_rule = strsep(&sub_rule, ",")) != NULL)
			{
				if (*cur_sub_rule	 != 0)
				{
					char *seperator = strchr(cur_sub_rule, '=');
					if (seperator == NULL)
					{
						printk("sfe error: wrong subrule %s\n",cur_sub_rule);
						goto free_rules;
					}
					else
					{
						char *subrule_key = cur_sub_rule, *subrule_value = seperator + 1;
						*seperator = 0;
						while(*subrule_value == '=') ++subrule_value;

						ctlrule_t *temp_sub_rule = (ctlrule_t *)kmalloc(sizeof(ctlrule_t), GFP_ATOMIC);
						if (temp_sub_rule == NULL)
						{
							printk("sfe error: kmalloc can't alloc a ctlrule_t\n");
							goto free_rules;
						}
						else
						{
							if (!parse_rule(temp_sub_rule, subrule_key, subrule_value))
							{
								if (temp_rule == NULL)
								{
									ctl_flag  = temp_sub_rule->flag & SFE_CM_SAME_BITS;
									temp_rule = temp_sub_rule;
								}
								else
								{
									if (ctl_flag != (temp_sub_rule->flag & SFE_CM_SAME_BITS))
									{
										kfree(temp_sub_rule);
										printk("sfe error: all subrules should have the same clr/set and priority flag!\n");
										goto free_rules;
									}
									temp_sub_rule->next_sub = temp_rule;
									temp_rule = temp_sub_rule;
								}
							}
							else
							{
								kfree(temp_sub_rule);
								printk("sfe error: can't parse subrule\n");
								goto free_rules;
							}
						}
					}
				}
			}
			if (ctl_rules == NULL)
			{
				ctl_rules = temp_rule;
			}
			else
			{
				temp_rule->next = ctl_rules;
				ctl_rules = temp_rule;
			}
		}
	}
	if (ctl_rules)
	{
		printk("sfe: process current connections...\n");
		sfe_ipv4_ctl_conns(ctl_rules);
		sfe_ipv6_ctl_conns(ctl_rules);
	}

free_rules:
	kfree(input_buf);
	while(ctl_rules)
    {
        ctl_subs = ctl_rules->next_sub;
        while(ctl_subs)
        {
            temp_ctl_subs = ctl_subs;
            ctl_subs = ctl_subs->next_sub;
            kfree(temp_ctl_subs);
        }
        temp_ctl_rules = ctl_rules;
        ctl_rules = ctl_rules->next;
        kfree(temp_ctl_rules);
    }

	return size;
}

/*
 * sfe_cm_get_offload_at_pkts()
 */
static ssize_t sfe_cm_get_offload_at_pkts(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", offload_at_pkts);
}

/*
 * sfe_cm_set_offload_at_pkts()
 */
static ssize_t sfe_cm_set_offload_at_pkts(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	long new;
	int ret;

	ret = strict_strtol(buf, 0, &new);
	if (ret == -EINVAL || ((int)new != new))
		return -EINVAL;

	offload_at_pkts = new;

	return size;
}


static ssize_t sfe_get_drop_cpu_load(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", drop_cpu_load);
}

static ssize_t sfe_set_drop_cpu_load(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	strict_strtol(buf, 0, (long int *)&drop_cpu_load);

	return size;
}

static ssize_t sfe_get_drop_random_limit(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", drop_random_limit);
}

static ssize_t sfe_set_drop_random_limit(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	strict_strtol(buf, 0, (long int *)&drop_random_limit);

	return size;
}


static ssize_t sfe_get_drop_random_smoothly(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", drop_random_smoothly);
}

static ssize_t sfe_set_drop_random_smoothly(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	strict_strtol(buf, 0, (long int *)&drop_random_smoothly);

	return size;
}



static const struct device_attribute sfe_drop_cpu_load =
	__ATTR(cpu_loadavg_drop, S_IWUSR | S_IRUGO, sfe_get_drop_cpu_load, sfe_set_drop_cpu_load);

static const struct device_attribute sfe_drop_random_limit =
	__ATTR(random_limit_drop, S_IWUSR | S_IRUGO, sfe_get_drop_random_limit, sfe_set_drop_random_limit);

static const struct device_attribute sfe_drop_random_smoothly =
	__ATTR(random_smoothly_drop, S_IWUSR | S_IRUGO, sfe_get_drop_random_smoothly, sfe_set_drop_random_smoothly);

/*
 * sysfs attributes.
 */
static const struct device_attribute sfe_cm_exceptions_attr =
	__ATTR(exceptions, S_IRUGO, sfe_cm_get_exceptions, NULL);
static const struct device_attribute sfe_cm_no_window_check =
	__ATTR(no_window_check, S_IWUSR | S_IRUGO, sfe_cm_get_no_window_check, sfe_cm_set_no_window_check);
static const struct device_attribute sfe_cm_ctl_conn_file =
	__ATTR(ctl_conn, S_IWUSR | S_IRUGO, sfe_cm_display_ctl_conn_help, sfe_cm_ctl_conn);
static const struct device_attribute sfe_cm_offload_at_pkts_attr =
	__ATTR(offload_at_pkts, S_IWUSR | S_IRUGO, sfe_cm_get_offload_at_pkts, sfe_cm_set_offload_at_pkts);


/*
 * sfe_cm_init()
 */
static int __init sfe_cm_init(void)
{
	struct sfe_cm *sc = &__sc;
	int result = -1;

	DEBUG_INFO("SFE CM init\n");

	hash_init(sc_conn_ht);

	/*
	 * Create sys/sfe_cm
	 */
	sc->sys_sfe_cm = kobject_create_and_add("sfe_cm", NULL);
	if (!sc->sys_sfe_cm) {
		DEBUG_ERROR("failed to register sfe_cm\n");
		goto exit1;
	}

	/*
	 * Create sys/sfe_cm/exceptions
	 */
	result = sysfs_create_file(sc->sys_sfe_cm, &sfe_cm_exceptions_attr.attr);
	if (result) {
		DEBUG_ERROR("failed to register exceptions file: %d\n", result);
		goto exit2;
	}

	/*
	 * Create sys/sfe_cm/no_window_check
	 */
	result = sysfs_create_file(sc->sys_sfe_cm, &sfe_cm_no_window_check.attr);
	if (result) {
		DEBUG_ERROR("failed to register no_window_check file: %d\n", result);
		sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_exceptions_attr.attr);
		goto exit2;
	}

	/*
	 * Create sys/sfe_cm/ctl_conn
	 */
	result = sysfs_create_file(sc->sys_sfe_cm, &sfe_cm_ctl_conn_file.attr);
	if (result) {
		DEBUG_ERROR("failed to register ctl_conn file: %d\n", result);
		sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_ctl_conn_file.attr);
		goto exit2;
	}
	/*
	 * Create sys/sfe_cm/offload_at_pkts
	 */
	result = sysfs_create_file(sc->sys_sfe_cm, &sfe_cm_offload_at_pkts_attr.attr);
	if (result) {
		DEBUG_ERROR("failed to register offload_at_pkts file: %d\n", result);
		sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_exceptions_attr.attr);
		sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_no_window_check.attr);
		goto exit2;
	}

	sysfs_create_file(sc->sys_sfe_cm, &sfe_drop_cpu_load.attr);
	sysfs_create_file(sc->sys_sfe_cm, &sfe_drop_random_limit.attr);
	sysfs_create_file(sc->sys_sfe_cm, &sfe_drop_random_smoothly.attr);

	sc->dev_notifier.notifier_call = sfe_cm_device_event;
	sc->dev_notifier.priority = 1;
	register_netdevice_notifier(&sc->dev_notifier);

	sc->inet_notifier.notifier_call = sfe_cm_inet_event;
	sc->inet_notifier.priority = 1;
	register_inetaddr_notifier(&sc->inet_notifier);

	sc->inet6_notifier.notifier_call = sfe_cm_inet6_event;
	sc->inet6_notifier.priority = 1;
	register_inet6addr_notifier(&sc->inet6_notifier);
	/*
	 * Register our netfilter hooks.
	 */
	result = nf_register_hooks(sfe_cm_ops_post_routing, ARRAY_SIZE(sfe_cm_ops_post_routing));
	if (result < 0) {
		DEBUG_ERROR("can't register nf post routing hook: %d\n", result);
		goto exit3;
	}

#ifdef CONFIG_NF_CONNTRACK_EVENTS
	/*
	 * Register a notifier hook to get fast notifications of expired connections.
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	result = nf_conntrack_register_notifier(&init_net, &sfe_cm_conntrack_notifier);
#else
	result = nf_conntrack_register_notifier(&sfe_cm_conntrack_notifier);
#endif
	if (result < 0) {
		DEBUG_ERROR("can't register nf notifier hook: %d\n", result);
		goto exit4;
	}
#endif

	spin_lock_init(&sc->lock);

	/*
	 * Hook the receive path in the network stack.
	 */
	BUG_ON(athrs_fast_nat_recv != NULL);
	RCU_INIT_POINTER(athrs_fast_nat_recv, sfe_cm_recv);

	/*
	 * Hook the shortcut sync callback.
	 */
	sfe_ipv4_register_sync_rule_callback(sfe_cm_sync_rule);
	sfe_ipv6_register_sync_rule_callback(sfe_cm_sync_rule);
	return 0;

#ifdef CONFIG_NF_CONNTRACK_EVENTS
exit4:
#endif
	nf_unregister_hooks(sfe_cm_ops_post_routing, ARRAY_SIZE(sfe_cm_ops_post_routing));

exit3:
	unregister_inet6addr_notifier(&sc->inet6_notifier);
	unregister_inetaddr_notifier(&sc->inet_notifier);
	unregister_netdevice_notifier(&sc->dev_notifier);
exit2:
	kobject_put(sc->sys_sfe_cm);

exit1:
	return result;
}

/*
 * sfe_cm_exit()
 */
static void __exit sfe_cm_exit(void)
{
	struct sfe_cm *sc = &__sc;

	DEBUG_INFO("SFE CM exit\n");

	/*
	 * Unregister our sync callback.
	 */
	sfe_ipv4_register_sync_rule_callback(NULL);
	sfe_ipv6_register_sync_rule_callback(NULL);

	/*
	 * Unregister our receive callback.
	 */
	RCU_INIT_POINTER(athrs_fast_nat_recv, NULL);

	/*
	 * Wait for all callbacks to complete.
	 */
	rcu_barrier();

	/*
	 * Destroy all connections.
	 */
	sfe_ipv4_destroy_all_rules_for_dev(NULL);
	sfe_ipv6_destroy_all_rules_for_dev(NULL);
	sfe_cm_destroy_all_conns(NULL);

#ifdef CONFIG_NF_CONNTRACK_EVENTS
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	nf_conntrack_unregister_notifier(&init_net, &sfe_cm_conntrack_notifier);
#else
	nf_conntrack_unregister_notifier(&sfe_cm_conntrack_notifier);
#endif

#endif
	nf_unregister_hooks(sfe_cm_ops_post_routing, ARRAY_SIZE(sfe_cm_ops_post_routing));

	unregister_inet6addr_notifier(&sc->inet6_notifier);
	unregister_inetaddr_notifier(&sc->inet_notifier);
	unregister_netdevice_notifier(&sc->dev_notifier);

	sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_exceptions_attr.attr);
	sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_no_window_check.attr);
    sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_ctl_conn_file.attr);
	sysfs_remove_file(sc->sys_sfe_cm, &sfe_cm_offload_at_pkts_attr.attr);
	sysfs_remove_file(sc->sys_sfe_cm, &sfe_drop_cpu_load.attr);
	sysfs_remove_file(sc->sys_sfe_cm, &sfe_drop_random_limit.attr);
	sysfs_remove_file(sc->sys_sfe_cm, &sfe_drop_random_smoothly.attr);

	kobject_put(sc->sys_sfe_cm);
}

module_init(sfe_cm_init)
module_exit(sfe_cm_exit)

MODULE_AUTHOR("Qualcomm Atheros Inc.");
MODULE_DESCRIPTION("Shortcut Forwarding Engine - Connection Manager");
MODULE_LICENSE("Dual BSD/GPL");

