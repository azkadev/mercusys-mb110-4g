/*
 * sfe_cm.h
 *	Shortcut forwarding engine.
 *
 * Copyright (c) 2013-2015 The Linux Foundation. All rights reserved.
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

/*
 * connection flags.
 */
#define SFE_CREATE_FLAG_NO_SEQ_CHECK (1<<0)
					/* Indicates that we should not check sequence numbers */
#define SFE_CREATE_FLAG_REMARK_PRIORITY (1<<1)
					/* Indicates that we should remark priority of skb */
#define SFE_CREATE_FLAG_REMARK_DSCP (1<<2)
					/* Indicates that we should remark DSCP of packet */

/*
 * IPv6 address structure
 */
struct sfe_ipv6_addr {
	__be32 addr[4];
};

typedef union {
	__be32			ip;
	struct sfe_ipv6_addr	ip6[1];
} sfe_ip_addr_t;

/*
 * connection creation structure.
 */
struct sfe_connection_create {
	int protocol;
	struct net_device *src_dev;
	struct net_device *dest_dev;
	uint32_t flags;
	uint32_t src_mtu;
	uint32_t dest_mtu;
	sfe_ip_addr_t src_ip;
	sfe_ip_addr_t src_ip_xlate;
	sfe_ip_addr_t dest_ip;
	sfe_ip_addr_t dest_ip_xlate;
	__be16 src_port;
	__be16 src_port_xlate;
	__be16 dest_port;
	__be16 dest_port_xlate;
	uint8_t src_mac[ETH_ALEN];
	uint8_t src_mac_xlate[ETH_ALEN];
	uint8_t dest_mac[ETH_ALEN];
	uint8_t dest_mac_xlate[ETH_ALEN];
	uint8_t src_td_window_scale;
	uint32_t src_td_max_window;
	uint32_t src_td_end;
	uint32_t src_td_max_end;
	uint8_t dest_td_window_scale;
	uint32_t dest_td_max_window;
	uint32_t dest_td_end;
	uint32_t dest_td_max_end;
	uint32_t mark;
#ifdef CONFIG_XFRM
	uint32_t original_accel;
	uint32_t reply_accel;
#endif
	uint32_t src_priority;
	uint32_t dest_priority;
	uint32_t src_dscp;
	uint32_t dest_dscp;
};

/*
 * connection destruction structure.
 */
struct sfe_connection_destroy {
	int protocol;
	sfe_ip_addr_t src_ip;
	sfe_ip_addr_t dest_ip;
	__be16 src_port;
	__be16 dest_port;
};

typedef enum sfe_sync_reason {
	SFE_SYNC_REASON_STATS,	/* Sync is to synchronize stats */
	SFE_SYNC_REASON_FLUSH,	/* Sync is to flush a entry */
	SFE_SYNC_REASON_DESTROY	/* Sync is to destroy a entry(requested by connection manager) */
} sfe_sync_reason_t;

/*
 * Structure used to sync connection stats/state back within the system.
 *
 * NOTE: The addresses here are NON-NAT addresses, i.e. the true endpoint addressing.
 * 'src' is the creator of the connection.
 */
struct sfe_connection_sync {
	struct net_device *src_dev;
	struct net_device *dest_dev;
	int is_v6;			/* Is it for ipv6? */
	int protocol;			/* IP protocol number (IPPROTO_...) */
	sfe_ip_addr_t src_ip;		/* Non-NAT source address, i.e. the creator of the connection */
	sfe_ip_addr_t src_ip_xlate;	/* NATed source address */
	__be16 src_port;		/* Non-NAT source port */
	__be16 src_port_xlate;		/* NATed source port */
	sfe_ip_addr_t dest_ip;		/* Non-NAT destination address, i.e. to whom the connection was created */
	sfe_ip_addr_t dest_ip_xlate;	/* NATed destination address */
	__be16 dest_port;		/* Non-NAT destination port */
	__be16 dest_port_xlate;		/* NATed destination port */
	uint32_t src_td_max_window;
	uint32_t src_td_end;
	uint32_t src_td_max_end;
	uint64_t src_packet_count;
	uint64_t src_byte_count;
	uint32_t src_new_packet_count;
	uint32_t src_new_byte_count;
	uint32_t dest_td_max_window;
	uint32_t dest_td_end;
	uint32_t dest_td_max_end;
	uint64_t dest_packet_count;
	uint64_t dest_byte_count;
	uint32_t dest_new_packet_count;
	uint32_t dest_new_byte_count;
	uint32_t reason;		/* reason for stats sync message, i.e. destroy, flush, period sync */
	uint64_t delta_jiffies;		/* Time to be added to the current timeout to keep the connection alive */
	uint16_t rep_dst_mac[ETH_ALEN / 2];
};

/*
 * connection mark structure
 */
struct sfe_connection_mark {
	int protocol;
	sfe_ip_addr_t src_ip;
	sfe_ip_addr_t dest_ip;
	__be16 src_port;
	__be16 dest_port;
	uint32_t mark;
};

/* refer to oal_iqos.h */
#define  MASK_FOR_WAN2LAN        (0X00008000)
#define  IQOS_CLASS_HIGH          4
#define  IQOS_CLASS_MID	          3
#define  IQOS_CLASS_LOW	          2

#define  MASK_FOR_IQOSMARK        (IQOS_CLASS_HIGH|IQOS_CLASS_MID|IQOS_CLASS_LOW)

#define  SFE_CM_CTL_PROTO_TCP    (1<<10)
#define  SFE_CM_CTL_PROTO_UDP    (1<<11)
#define  SFE_CM_CTL_PROTO_ALL    (SFE_CM_CTL_PROTO_TCP|SFE_CM_CTL_PROTO_UDP)
#define  SFE_CM_CTL_DIR_W2L      (1<<12)
#define  SFE_CM_CTL_DIR_L2W      (1<<13)
#define  SFE_CM_CTL_DIR_ALL      (SFE_CM_CTL_DIR_W2L|SFE_CM_CTL_DIR_L2W)

/*   0----clear qos mark    1----set qos mark */
#define  SFE_CM_CTL_MARK_TCP      (1<<14)
#define  SFE_CM_CTL_MARK_UDP      (1<<15)
#define  SFE_CM_CTL_MARK_ALL      (SFE_CM_CTL_MARK_TCP | SFE_CM_CTL_MARK_UDP)

/*  bits below should be same for all subrules */
#define  SFE_CM_SAME_BITS         (MASK_FOR_IQOSMARK|SFE_CM_CTL_MARK_ALL)

#define  SFE_CM_CTL_IPV6          (1<<16)


typedef enum  sfe_cm_rule {
	SFE_CM_RULE_SMAC = 0,
	SFE_CM_RULE_DMAC,
	SFE_CM_RULE_SIP,
	SFE_CM_RULE_DIP,
	SFE_CM_RULE_SIP_XLATE,
	SFE_CM_RULE_DIP_XLATE,
	SFE_CM_RULE_SPORT,
	SFE_CM_RULE_DPORT,
	SFE_CM_RULE_SPORT_XLATE,
	SFE_CM_RULE_DPORT_XLATE,
	SFE_CM_RULE_APPID,
	SFE_CM_RULE_MAX
}sfe_cm_rule_t;

typedef struct ctlrule_entry {
	sfe_cm_rule_t  type;
	uint32_t          flag;
	struct ctlrule_entry   *next;
	struct ctlrule_entry   *next_sub;
  	union {
		uint8_t mac[ETH_ALEN];
		__be32	ip;
		struct  sfe_ipv6_addr	ip6[1];
		struct {
		uint16_t  min_port;
		uint16_t  max_port;
		}port;
		int     appid;
	}rule;
}ctlrule_t;


/*
 * Type used for a sync rule callback.
 */
typedef void (*sfe_sync_rule_callback_t)(struct sfe_connection_sync *);

/*
 * IPv4 APIs used by connection manager
 */
extern int sfe_ipv4_recv(struct net_device *dev, struct sk_buff *skb);
extern int sfe_ipv4_create_rule(struct sfe_connection_create *sic);
extern void sfe_ipv4_destroy_rule(struct sfe_connection_destroy *sid);
extern void sfe_ipv4_ctl_conns(const ctlrule_t *rules);
extern void sfe_ipv4_destroy_all_rules_for_dev(struct net_device *dev);
extern unsigned int sfe_get_tfs_client_number();
extern unsigned int sfe_get_ipv4_si_num(void);
extern void sfe_ipv4_register_sync_rule_callback(sfe_sync_rule_callback_t callback);
extern void tfs_update_client_sync_cb(struct sfe_connection_sync *sis);
extern int sfe_ipv4_update_rule(struct sfe_connection_create *sic);
extern void sfe_ipv4_mark_rule(struct sfe_connection_mark *mark);

typedef int (*tm_accel_cb_t)(struct nf_conn *ct);
extern void tm_register_accel_cb(tm_accel_cb_t cb);

typedef int (*tm_fwd_cb_t)(uint32_t connmark, struct sk_buff *skb);
extern int tm_register_ipv4_fwd_cb(tm_fwd_cb_t cb);
extern int tm_register_ipv6_fwd_cb(tm_fwd_cb_t cb);

#ifdef SFE_SUPPORT_IPV6
/*
 * IPv6 APIs used by connection manager
 */
extern int sfe_ipv6_recv(struct net_device *dev, struct sk_buff *skb);
extern int sfe_ipv6_create_rule(struct sfe_connection_create *sic);
extern void sfe_ipv6_destroy_rule(struct sfe_connection_destroy *sid);
extern void sfe_ipv6_ctl_conns(const ctlrule_t *rules);
extern void sfe_ipv6_destroy_all_rules_for_dev(struct net_device *dev);
extern void sfe_ipv6_register_sync_rule_callback(sfe_sync_rule_callback_t callback);
extern int sfe_ipv6_update_rule(struct sfe_connection_create *sic);
extern void sfe_ipv6_mark_rule(struct sfe_connection_mark *mark);
#else
static inline int sfe_ipv6_recv(struct net_device *dev, struct sk_buff *skb)
{
	return 0;
}

static inline int sfe_ipv6_create_rule(struct sfe_connection_create *sic)
{
	return -1;
}

static inline void sfe_ipv6_destroy_rule(struct sfe_connection_destroy *sid)
{
	return;
}

static inline void sfe_ipv6_destroy_all_rules_for_dev(struct net_device *dev)
{
	return;
}

static inline void sfe_ipv6_register_sync_rule_callback(sfe_sync_rule_callback_t callback)
{
	return;
}

static inline int sfe_ipv6_update_rule(struct sfe_connection_create *sic)
{
	return;
}

static inline void sfe_ipv6_mark_rule(struct sfe_connection_mark *mark)
{
	return;
}
#endif

/*
 * sfe_ipv6_addr_equal()
 *	compare ipv6 address
 *
 * return: 1, equal; 0, no equal
 */
static inline int sfe_ipv6_addr_equal(struct sfe_ipv6_addr *a,
				      struct sfe_ipv6_addr *b)
{
	return a->addr[0] == b->addr[0] &&
	       a->addr[1] == b->addr[1] &&
	       a->addr[2] == b->addr[2] &&
	       a->addr[3] == b->addr[3];
}

/*
 * sfe_ipv4_addr_equal()
 *	compare ipv4 address
 *
 * return: 1, equal; 0, no equal
 */
#define sfe_ipv4_addr_equal(a, b) ((uint32_t)(a) == (uint32_t)(b))

/*
 * sfe_addr_equal()
 *	compare ipv4 or ipv6 address
 *
 * return: 1, equal; 0, no equal
 */
static inline int sfe_addr_equal(sfe_ip_addr_t *a,
				 sfe_ip_addr_t *b, int is_v4)
{
	return is_v4 ? sfe_ipv4_addr_equal(a->ip, b->ip) : sfe_ipv6_addr_equal(a->ip6, b->ip6);
}
