/* Kernel module to match the port-ranges, trigger related port-ranges,
 * and alters the destination to a local IP address.
 *
 * Copyright (C) 2003, CyberTAN Corporation
 * All Rights Reserved.
 *
 * Description:
 *   This is kernel module for port-triggering.
 *
 *   The module follows the Netfilter framework, called extended packet 
 *   matching modules. 
 */

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/inetdevice.h>
#include <net/protocol.h>
#include <net/checksum.h>

#include <linux/netfilter_ipv4.h>
#include <linux/netfilter/x_tables.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_tuple.h>
#include <linux/netfilter_ipv4/lockhelp.h> 

#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter/xt_TRIGGER.h>

#include <linux/spinlock.h>

#define MUST_BE_READ_LOCKED(l)
#define MUST_BE_WRITE_LOCKED(l)

#define READ_LOCK(l) read_lock_bh(l)
#define WRITE_LOCK(l) write_lock_bh(l)
#define READ_UNLOCK(l) read_unlock_bh(l)
#define WRITE_UNLOCK(l) write_unlock_bh(l)


/* This rwlock protects the main hash table, protocol/helper/expected
 *    registrations, conntrack timers*/
#define ASSERT_READ_LOCK(x) MUST_BE_READ_LOCKED(&nf_conntrack_lock)
#define ASSERT_WRITE_LOCK(x) MUST_BE_WRITE_LOCKED(&nf_conntrack_lock)

/* Return pointer to first true entry, if any, or NULL.  A macro
   required to allow inlining of cmpfn. */
#define LIST_FIND(head, cmpfn, type, args...)		\
({							\
	const struct list_head *__i, *__j = NULL;	\
							\
	ASSERT_READ_LOCK(head);				\
	list_for_each(__i, (head))			\
		if (cmpfn((const type)__i , ## args)) {	\
			__j = __i;			\
			break;				\
		}					\
	(type)__j;					\
})


MODULE_AUTHOR("LSZ <www.tplink.com.cn>");
MODULE_DESCRIPTION("Target for port trigger");
MODULE_LICENSE("GPL");


#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

struct xt_trigger {
	struct list_head list;		/* Trigger list */
	struct timer_list timeout;	/* Timer for list destroying */
	u_int32_t srcip;		/* Outgoing source address */
	u_int32_t dstip;		/* Outgoing destination address */
	u_int16_t mproto;		/* Trigger protocol */
	u_int16_t rproto;		/* Related protocol */
	struct xt_trigger_ports ports;	/* Trigger and related ports */
	u_int8_t reply;			/* Confirm a reply connection */
};

LIST_HEAD(trigger_list);
//DECLARE_LOCK(ip_trigger_lock);

static void trigger_refresh(struct xt_trigger *trig, unsigned long extra_jiffies)
{
    DEBUGP("%s: \n", __FUNCTION__);
    NF_CT_ASSERT(trig);

	/*King(20120808): replace WRITE_LOCK with LOCK_BH, because nf_conntrack_lock is 
		defined as spin lock in Linux kernel-2.6.36 but rw lock in 2.6.22.15*/
#if 0 /* xcl modify, 201303 */
    WRITE_LOCK(&nf_conntrack_lock);
#else
	LOCK_BH(&nf_conntrack_lock);
#endif

    /* Need del_timer for race avoidance (may already be dying). */
    if (del_timer(&trig->timeout)) {
		trig->timeout.expires = jiffies + extra_jiffies;
		add_timer(&trig->timeout);
    }

#if 0
    WRITE_UNLOCK(&nf_conntrack_lock);
#else
	UNLOCK_BH(&nf_conntrack_lock);
#endif
}

static void __del_trigger(struct xt_trigger *trig)
{
    DEBUGP("%s: \n", __FUNCTION__);
    NF_CT_ASSERT(trig);
    MUST_BE_WRITE_LOCKED(&nf_conntrack_lock);

     /* delete from 'trigger_list' */
    list_del(&trig->list);
    kfree(trig);
}

static void trigger_timeout(unsigned long ul_trig)
{
    struct xt_trigger *trig= (void *) ul_trig;

    DEBUGP("trigger list %p timed out\n", trig);
	
#if 0
    WRITE_LOCK(&nf_conntrack_lock);
#else
	LOCK_BH(&nf_conntrack_lock);
#endif

    __del_trigger(trig);

#if 0
    WRITE_UNLOCK(&nf_conntrack_lock);
#else
	UNLOCK_BH(&nf_conntrack_lock);
#endif
}

static unsigned int
add_new_trigger(struct xt_trigger *trig)
{
    struct xt_trigger *new;

    DEBUGP("!!!!!!!!!!!! %s !!!!!!!!!!!\n", __FUNCTION__);

#if 0
    WRITE_LOCK(&nf_conntrack_lock);
#else
	LOCK_BH(&nf_conntrack_lock);
#endif

    new = (struct xt_trigger *)
		kmalloc(sizeof(struct xt_trigger), GFP_ATOMIC);

    if (!new) {
#if 0
    WRITE_UNLOCK(&nf_conntrack_lock);
#else
	UNLOCK_BH(&nf_conntrack_lock);
#endif

	DEBUGP("%s: OOM allocating trigger list\n", __FUNCTION__);
	return -ENOMEM;
    }

    memset(new, 0, sizeof(*trig));
    INIT_LIST_HEAD(&new->list);
    memcpy(new, trig, sizeof(*trig));

    /* add to global table of trigger */
	list_add(&new->list, &trigger_list);
    /* add and start timer if required */
    init_timer(&new->timeout);
    new->timeout.data = (unsigned long)new;
    new->timeout.function = trigger_timeout;
    new->timeout.expires = jiffies + (TRIGGER_TIMEOUT * HZ);
    add_timer(&new->timeout);
	    
#if 0
    WRITE_UNLOCK(&nf_conntrack_lock);
#else
	UNLOCK_BH(&nf_conntrack_lock);
#endif

    return 0;
}

static inline int trigger_out_matched(const struct xt_trigger *i,
	const u_int16_t proto, const u_int16_t dport, const u_int16_t *rport)
{
    /* DEBUGP("%s: i=%p, proto= %d, dport=%d.\n", __FUNCTION__, i, proto, dport);
    DEBUGP("%s: Got one, mproto= %d, mport[0..1]=%d, %d.\n", __FUNCTION__, 
	    i->mproto, i->ports.mport[0], i->ports.mport[1]); */
	
    return ((i->mproto == proto) 			/* proto is rignt*/
    	&& (i->ports.mport[0] <= dport) 	/* dst port is between the match ports */
	    && (i->ports.mport[1] >= dport) 
	    && (i->ports.rport[0] <= rport[0])	/* related ports is contained by old ones */
	    && (i->ports.rport[1] >= rport[1]));
}

static unsigned int
trigger_out(struct sk_buff *skb, const struct xt_action_param *par) 
{
    const struct xt_trigger_info *info = par->targinfo;
    struct xt_trigger trig, *found;
    const struct iphdr *iph = ip_hdr(skb);
    struct tcphdr *tcph = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */
	
    DEBUGP("############# %s ############\n", __FUNCTION__);
    /* Check if the trigger range has already existed in 'trigger_list'. */
    found = LIST_FIND(&trigger_list, trigger_out_matched,
	    struct xt_trigger *, iph->protocol, ntohs(tcph->dest), info->ports.rport);

	/*list_for_each_entry(km, &gre_keymap_list, list) {
		if (gre_key_cmpfn(km, t)) {
			key = km->tuple.src.u.gre.key;
			break;
		}
	}
	
	km = LIST_FIND(&gre_keymap_list, gre_key_cmpfn,
			struct ip_ct_gre_keymap *, t);
	if (km)
		key = km->tuple.src.u.gre.key;*/

	/*list_for_each_entry(found, &trigger_list, list) {
		if (trigger_out_matched(found, iph->protocol, ntohs(tcph->dest), info->ports.rport)) {
			break;
		}
	}*/
	
    if (found) {
		/* Yeah, it exists. We need to update(delay) the destroying timer. */
		trigger_refresh(found, TRIGGER_TIMEOUT * HZ);
		/* In order to allow multiple hosts use the same port range, we update
		   the 'saddr' after previous trigger has a reply connection. */
		if (found->reply)
		    found->srcip = iph->saddr;
    }
    else {
		/* Create new trigger */
		memset(&trig, 0, sizeof(trig));
		trig.srcip = iph->saddr;
		trig.mproto = iph->protocol;
		trig.rproto = info->proto;
		#if 0
		printk("<1>new trig: srcip =%X mproto =%d rproto =%d mport %d-%d rport %d-%d\n"
			, trig.srcip, trig.mproto, trig.rproto, info->ports.mport[0], info->ports.mport[1],
			info->ports.rport[0], info->ports.rport[1]);
		#endif
		memcpy(&trig.ports, &info->ports, sizeof(struct xt_trigger_ports));

		// added by lishaozhang 
		// add new trig only when the packet's dst port is in the range of mports
		if (info->ports.mport[0] <= ntohs(tcph->dest) &&
			info->ports.mport[1] >= ntohs(tcph->dest))
		// end of added code
			add_new_trigger(&trig);	/* Add the new 'trig' to list 'trigger_list'. */
    }

    return XT_CONTINUE;	/* We don't block any packet. */
}

static inline int trigger_in_matched(const struct xt_trigger *i,
	const u_int16_t proto, const u_int16_t dport)
{
    u_int16_t rproto = i->rproto;

    DEBUGP("%s: i=%p, proto= %d, dport=%d.\n", __FUNCTION__, i, proto, dport);
    DEBUGP("%s: Got one, rproto= %d, rport[0..1]=%d, %d.\n", __FUNCTION__, 
	    i->rproto, i->ports.rport[0], i->ports.rport[1]); 

    if (!rproto)// relate proto is all(0)
		rproto = proto;

    return ((rproto == proto) && (i->ports.rport[0] <= dport) 
	    && (i->ports.rport[1] >= dport));
}

static unsigned int
trigger_in(struct sk_buff *skb, const struct xt_action_param *par)
{
    struct xt_trigger *found;
    const struct iphdr *iph = ip_hdr(skb);
    struct tcphdr *tcph = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */

    /* Check if the trigger-ed range has already existed in 'trigger_list'. */
    found = LIST_FIND(&trigger_list, trigger_in_matched,
	    struct xt_trigger *, iph->protocol, ntohs(tcph->dest));
	/*list_for_each_entry(found, &trigger_list, list) {
		if (trigger_in_matched(found, iph->protocol, ntohs(tcph->dest))) {
			break;
		}
	}*/
	
    if (found)
	{
		DEBUGP("############# %s ############\n", __FUNCTION__);
		/* Yeah, it exists. We need to update(delay) the destroying timer. */
		trigger_refresh(found, TRIGGER_TIMEOUT * HZ);
		return NF_ACCEPT;	/* Accept it, or the imcoming packet could be 
				   dropped in the FORWARD chain */
    }
 
    return XT_CONTINUE;	/* Our job is the interception. */
}

static unsigned int
trigger_dnat(struct sk_buff *skb, const struct xt_action_param *par)
{
    struct xt_trigger *found;
    const struct iphdr *iph = ip_hdr(skb);
    struct tcphdr *tcph = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */
    struct nf_conn *ct;
    enum ip_conntrack_info ctinfo;

	/* modified according to ip_nat.h, by lsz 16May07 */
	//#ifdef __KERNEL__
		struct nf_nat_multi_range_compat newrange;
	//#else
    //	struct ip_nat_multi_range newrange;
	//#endif

    NF_CT_ASSERT(par->hooknum == NF_INET_PRE_ROUTING);
    /* Check if the trigger-ed range has already existed in 'trigger_list'. */
    found = LIST_FIND(&trigger_list, trigger_in_matched,
	    struct xt_trigger *, iph->protocol, ntohs(tcph->dest));
	/*list_for_each_entry(found, &trigger_list, list) {
		if (trigger_in_matched(found, iph->protocol, ntohs(tcph->dest))) {
			break;
		}
	}*/

    if (!found || !found->srcip)
		return XT_CONTINUE;	/* We don't block any packet. */

    DEBUGP("############# %s ############\n", __FUNCTION__);
    found->reply = 1;	/* Confirm there has been a reply connection. */
    ct = nf_ct_get(skb, &ctinfo);
    NF_CT_ASSERT(ct && (ctinfo == IP_CT_NEW));

    DEBUGP("%s: got ", __FUNCTION__);
    //DUMP_TUPLE(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
    nf_ct_dump_tuple(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);

    /* Alter the destination of imcoming packet. */
    newrange = ((struct nf_nat_multi_range_compat)
	    { 1, { { IP_NAT_RANGE_MAP_IPS,
	             found->srcip, found->srcip,
	             { 0 }, { 0 }
	           } } });

    /* Hand modified range to generic setup. */
    return nf_nat_setup_info(ct, newrange.range, HOOK2MANIP(par->hooknum));
}

static unsigned int
trigger_target(struct sk_buff *skb, const struct xt_action_param *par)
{
    const struct xt_trigger_info *info = (const struct xt_trigger_info *)par->targinfo;
    const struct iphdr *iph = ip_hdr(skb);

    DEBUGP("%s: type = %s\n", __FUNCTION__, 
	    (info->type == XT_TRIGGER_DNAT) ? "dnat" :
	    (info->type == XT_TRIGGER_IN) ? "in" : "out"); 

    /* The Port-trigger only supports TCP and UDP. */
    if ((iph->protocol != IPPROTO_TCP) && (iph->protocol != IPPROTO_UDP))
	return XT_CONTINUE;

    if (info->type == XT_TRIGGER_OUT)
		return trigger_out(skb, par);
    else if (info->type == XT_TRIGGER_IN)
		return trigger_in(skb, par);
    else if (info->type == XT_TRIGGER_DNAT)
    	return trigger_dnat(skb, par);

    return XT_CONTINUE;
}

static int
trigger_check(const struct xt_tgchk_param *par)
{
	const struct xt_trigger_info *info = (const struct xt_trigger_info *)par->targinfo;
	struct list_head *cur_item, *tmp_item;

	if ((strcmp(par->table, "mangle") == 0)) {
		DEBUGP("trigger_check: bad table `%s'.\n", par->table);
		return -EINVAL;
	}
	
	if (par->hook_mask & ~((1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_FORWARD))) {
		DEBUGP("trigger_check: bad hooks %x.\n", par->hook_mask);
		return -EINVAL;
	}
	
	if (nf_ct_l3proto_try_module_get(par->target->family) < 0) {
		printk(KERN_WARNING "can't load conntrack support for "
				    "proto=%d\n", par->target->family);
		return -EINVAL;
	}
	
	if (info->type == XT_TRIGGER_OUT) {
	    if (!info->ports.mport[0] || !info->ports.rport[0]) {
		DEBUGP("trigger_check: Try 'iptbles -j TRIGGER -h' for help.\n");
		return -EINVAL;
	    }
	}

	/* Empty the 'trigger_list' */
	list_for_each_safe(cur_item, tmp_item, &trigger_list) {
	    struct xt_trigger *trig = (void *)cur_item;

	    DEBUGP("%s: list_for_each_safe(): %p.\n", __FUNCTION__, trig);
	    del_timer(&trig->timeout);
	    __del_trigger(trig);
	}

	return 0;
}

static struct xt_target xt_trigger_target[] = {
	{
		.name		= "TRIGGER", 
		.family		= AF_INET,
		.target		= trigger_target, 
		.checkentry	= trigger_check, 
		.targetsize	= sizeof(struct xt_trigger_info),
	    .me			= THIS_MODULE 
	},
};

static int __init xt_trigger_init(void)
{
	return xt_register_targets(xt_trigger_target, ARRAY_SIZE(xt_trigger_target));
}

static void __exit xt_trigger_fini(void)
{
	xt_unregister_targets(xt_trigger_target, ARRAY_SIZE(xt_trigger_target));
}

module_init(xt_trigger_init);
module_exit(xt_trigger_fini);
