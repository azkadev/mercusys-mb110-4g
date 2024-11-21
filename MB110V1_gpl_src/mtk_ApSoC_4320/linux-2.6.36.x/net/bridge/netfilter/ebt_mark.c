/*
 *  ebt_mark
 *
 *	Authors:
 *	Bart De Schuymer <bdschuym@pandora.be>
 *
 *  July, 2002
 *
 */

/* The mark target can be used in any chain,
 * I believe adding a mangle table just for marking is total overkill.
 * Marking a frame doesn't really change anything in the frame anyway.
 */

#include <linux/module.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_mark_t.h>

/* zl added 2013-1-6 */
#include <linux/ip.h>
#include <linux/ipv6.h>

#define QOS_8021P_ENABLE_MARK_MASK	(0X00100000U)
#define QOS_8021P_MARK_MASK			(0X000E0000U)
#define QOS_8021P_MARK_BEGIN_BIT	17
/* end--added */

/* zl moved from brcm ebt_ftos.c , 2012-7-30 */
#define PPPOE_HLEN   6
#define PPP_TYPE_IPV4   0x0021  /* IPv4 in PPP */
#define PPP_TYPE_IPV6   0x0057  /* IPv6 in PPP */
/* end--moved */


static unsigned int
ebt_mark_tg(struct sk_buff *skb, const struct xt_target_param *par)
{
	const struct ebt_mark_t_info *info = par->targinfo;
	int action = info->target & -16;

	if (action == MARK_SET_VALUE)
		skb->mark = info->mark;
	else if (action == MARK_OR_VALUE)
		skb->mark |= info->mark;
	else if (action == MARK_AND_VALUE)
		skb->mark &= info->mark;
/* zl modified 2013-1-6 */		
#if 0		
	else
		skb->mark ^= info->mark;
#else
    else if (action == MARK_XOR_VALUE)
        skb->mark ^= info->mark;
    else if (action == VLAN_8021P_SET_VALUE)
    {
        skb->mark &= ~(QOS_8021P_MARK_MASK | QOS_8021P_ENABLE_MARK_MASK);
        skb->mark |= (((info->mark & 0xF) << QOS_8021P_MARK_BEGIN_BIT) | QOS_8021P_ENABLE_MARK_MASK);
    }
    else /* if (action == VLAN_8021P_AUTO_SET)*/
    {
        struct iphdr *iph = NULL;
        struct ipv6hdr *ipv6h = NULL;
        
        /* if VLAN frame, we need to point to correct network header */
        if (skb->protocol == __constant_htons(ETH_P_IP))
            iph = (struct iphdr *)(skb->network_header);
        else if ((skb)->protocol == __constant_htons(ETH_P_IPV6))
            ipv6h = (struct ipv6hdr *)(skb->network_header);
        else if (skb->protocol == __constant_htons(ETH_P_8021Q)) 
        {
            if (*(unsigned short *)(skb->network_header + VLAN_HLEN - 2) == __constant_htons(ETH_P_IP))
                iph = (struct iphdr *)(skb->network_header + VLAN_HLEN);
            else if (*(unsigned short *)(skb->network_header + VLAN_HLEN - 2) == __constant_htons(ETH_P_IPV6))
                ipv6h = (struct ipv6hdr *)(skb->network_header + VLAN_HLEN);
        }
        else if (skb->protocol == __constant_htons(ETH_P_PPP_SES)) 
        {
            if (*(unsigned short *)(skb->network_header + PPPOE_HLEN) == PPP_TYPE_IPV4)
                iph = (struct iphdr *)(skb->network_header + PPPOE_HLEN + 2);
            else if (*(unsigned short *)(skb->network_header + PPPOE_HLEN) == PPP_TYPE_IPV6)
                ipv6h = (struct ipv6hdr *)(skb->network_header + PPPOE_HLEN + 2);
        }

        if (NULL != iph) /*IPv4*/
        {
            skb->mark &= ~(QOS_8021P_MARK_MASK | QOS_8021P_ENABLE_MARK_MASK);
            skb->mark |= (((((iph->tos) >> 5) & 0x07 ) << QOS_8021P_MARK_BEGIN_BIT) 
                            | QOS_8021P_ENABLE_MARK_MASK);
        }
        else if (NULL != ipv6h)
        {
            skb->mark &= ~(QOS_8021P_MARK_MASK | QOS_8021P_ENABLE_MARK_MASK);
            skb->mark |= ((((ntohs(*(const __be16 *)ipv6h) & 0x0ff0) >> 5) 
                               << QOS_8021P_MARK_BEGIN_BIT) | QOS_8021P_ENABLE_MARK_MASK);
        }
    }
#endif
/* end--modified */

	return info->target | ~EBT_VERDICT_BITS;
}

static bool ebt_mark_tg_check(const struct xt_tgchk_param *par)
{
	const struct ebt_mark_t_info *info = par->targinfo;
	int tmp;

	unsigned int len;
	len = EBT_ALIGN(sizeof(struct ebt_mark_t_info));

	tmp = info->target | ~EBT_VERDICT_BITS;
	if (BASE_CHAIN && tmp == EBT_RETURN)
		return -EINVAL;
	if (tmp < -NUM_STANDARD_TARGETS || tmp >= 0)
		return -EINVAL;
	tmp = info->target & ~EBT_VERDICT_BITS;
	if (tmp != MARK_SET_VALUE && tmp != MARK_OR_VALUE &&
	    tmp != MARK_AND_VALUE && tmp != MARK_XOR_VALUE 
	    && tmp !=  VLAN_8021P_AUTO_SET && tmp != VLAN_8021P_SET_VALUE /* zl added 2013-1-6 */)
		return -EINVAL;
	return 0;
}
#ifdef CONFIG_COMPAT
struct compat_ebt_mark_t_info {
	compat_ulong_t mark;
	compat_uint_t target;
};

static void mark_tg_compat_from_user(void *dst, const void *src)
{
	const struct compat_ebt_mark_t_info *user = src;
	struct ebt_mark_t_info *kern = dst;

	kern->mark = user->mark;
	kern->target = user->target;
}

static int mark_tg_compat_to_user(void __user *dst, const void *src)
{
	struct compat_ebt_mark_t_info __user *user = dst;
	const struct ebt_mark_t_info *kern = src;

	if (put_user(kern->mark, &user->mark) ||
	    put_user(kern->target, &user->target))
		return -EFAULT;
	return 0;
}
#endif

static struct xt_target ebt_mark_tg_reg __read_mostly = {
	.name		= "mark",
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
	.target		= ebt_mark_tg,
	.checkentry	= ebt_mark_tg_check,
	.targetsize	= XT_ALIGN(sizeof(struct ebt_mark_t_info)),
#ifdef CONFIG_COMPAT
	.compatsize	= sizeof(struct compat_ebt_mark_t_info),
	.compat_from_user = mark_tg_compat_from_user,
	.compat_to_user	= mark_tg_compat_to_user,
#endif
	.me		= THIS_MODULE,
};

static int __init ebt_mark_init(void)
{
	return xt_register_target(&ebt_mark_tg_reg);
}

static void __exit ebt_mark_fini(void)
{
	xt_unregister_target(&ebt_mark_tg_reg);
}

module_init(ebt_mark_init);
module_exit(ebt_mark_fini);
MODULE_DESCRIPTION("Ebtables: Packet mark modification");
MODULE_LICENSE("GPL");
