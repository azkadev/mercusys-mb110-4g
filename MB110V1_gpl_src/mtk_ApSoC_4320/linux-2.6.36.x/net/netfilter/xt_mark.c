/*
 *	xt_mark - Netfilter module to match NFMARK value
 *
 *	(C) 1999-2001 Marc Boucher <marc@mbsi.ca>
 *	Copyright © CC Computer Consultants GmbH, 2007 - 2008
 *	Jan Engelhardt <jengelh@medozas.de>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/skbuff.h>

#include <linux/netfilter/xt_mark.h>
#include <linux/netfilter/x_tables.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marc Boucher <marc@mbsi.ca>");
MODULE_DESCRIPTION("Xtables: packet mark operations");
MODULE_ALIAS("ipt_mark");
MODULE_ALIAS("ip6t_mark");
MODULE_ALIAS("ipt_MARK");
MODULE_ALIAS("ip6t_MARK");

static unsigned int
mark_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_mark_tginfo2 *info = par->targinfo;

	skb->mark = (skb->mark & ~info->mask) ^ info->mark;
	return XT_CONTINUE;
}

static bool
mark_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_mark_mtinfo1 *info = par->matchinfo;

	return ((skb->mark & info->mask) == info->mark) ^ info->invert;
}

/* zl moved from linux2.6.30, 2012-5-14 */
static unsigned int
mark_tg_v1(struct sk_buff *skb, const struct xt_action_param *par)
{
        const struct xt_mark_target_info_v1 *markinfo = par->targinfo;
        int mark = 0;

        switch (markinfo->mode) {
        case XT_MARK_SET:
                mark = markinfo->mark;
                break;

        case XT_MARK_AND:
                mark = skb->mark & markinfo->mark;
                break;

        case XT_MARK_OR:
                mark = skb->mark | markinfo->mark;
                break;
        }

        skb->mark = mark;
        return XT_CONTINUE;
}

static bool mark_tg_check_v1(const struct xt_tgchk_param *par)
{
        const struct xt_mark_target_info_v1 *markinfo = par->targinfo;

        if (markinfo->mode != XT_MARK_SET
            && markinfo->mode != XT_MARK_AND
            && markinfo->mode != XT_MARK_OR) {
                printk(KERN_WARNING "MARK: unknown mode %u\n",
                       markinfo->mode);
                return false;
        }
        if (markinfo->mark > 0xffffffff) {
                printk(KERN_WARNING "MARK: Only supports 32bit wide mark\n");
                return false;
        }
        return true;
}

static struct xt_target mark_tg_reg[] __read_mostly = {
	{
        .name           = "MARK",
        .family         = NFPROTO_UNSPEC,
        .revision       = 1,
        .checkentry     = mark_tg_check_v1,
        .target         = mark_tg_v1,
        .targetsize     = sizeof(struct xt_mark_target_info_v1),
        .me             = THIS_MODULE,
    },

	{
	.name           = "MARK",
	.revision       = 2,
	.family         = NFPROTO_UNSPEC,
	.target         = mark_tg,
	.targetsize     = sizeof(struct xt_mark_tginfo2),
	.me             = THIS_MODULE,
	}
};
/* end, 2012-5-14 */

#if 0 /* xcl del, 20130416 */
static struct xt_target mark_tg_reg __read_mostly = {
	.name           = "MARK",
	.revision       = 2,
	.family         = NFPROTO_UNSPEC,
	.target         = mark_tg,
	.targetsize     = sizeof(struct xt_mark_tginfo2),
	.me             = THIS_MODULE,
};
#endif

static struct xt_match mark_mt_reg __read_mostly = {
	.name           = "mark",
	.revision       = 1,
	.family         = NFPROTO_UNSPEC,
	.match          = mark_mt,
	.matchsize      = sizeof(struct xt_mark_mtinfo1),
	.me             = THIS_MODULE,
};

static int __init mark_mt_init(void)
{
	int ret;

#if 0 /* xcl modified, 20130416 */
	ret = xt_register_target(&mark_tg_reg);
#else
	ret = xt_register_targets(mark_tg_reg, ARRAY_SIZE(mark_tg_reg));
#endif
	if (ret < 0)
		return ret;
	ret = xt_register_match(&mark_mt_reg);
	if (ret < 0) {
#if 0 /* xcl modified, 20130416 */
		xt_unregister_target(&mark_tg_reg);
#else
		xt_unregister_targets(mark_tg_reg, ARRAY_SIZE(mark_tg_reg));
#endif
		return ret;
	}
	return 0;
}

static void __exit mark_mt_exit(void)
{
	xt_unregister_match(&mark_mt_reg);
#if 0 /* xcl modified, 20130416 */
	xt_unregister_target(&mark_tg_reg);
#else
	xt_unregister_targets(mark_tg_reg, ARRAY_SIZE(mark_tg_reg));
#endif
}

module_init(mark_mt_init);
module_exit(mark_mt_exit);
