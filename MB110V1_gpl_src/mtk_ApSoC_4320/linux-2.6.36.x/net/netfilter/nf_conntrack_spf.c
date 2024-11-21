/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	nf_conntrack_spf.c
 * Version		:	1.0
 * Abstract		:
 * Author		:	Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date	:	23 Jun 2014
 *
 ***************************************************************/


#include <linux/netfilter.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)
#include <linux/export.h>
#else
#include <linux/module.h>
#endif

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_spf.h>

#ifdef CONFIG_NF_CONNTRACK_TP_APP

static struct nf_ct_ext_type spf_extend __read_mostly = {
    .len   = sizeof(struct nf_conntrack_spf[IP_CT_DIR_MAX]),
    .align = __alignof__(struct nf_conntrack_spf[IP_CT_DIR_MAX]),
    .id    = NF_CT_EXT_SPF,
    .flags		= NF_CT_EXT_F_PREALLOC,
};

int nf_conntrack_spf_init(struct net *net)
{
    int ret;

    if (net_eq(net, &init_net)) {
        ret = nf_ct_extend_register(&spf_extend);
        if (ret < 0) {
            printk(KERN_ERR "nf_conntrack_spf: Unable to register extension\n");
            goto out_extend_register;
        }
    }

    return 0;

out_extend_register:
    return ret;
}

void nf_conntrack_spf_fini(struct net *net)
{
    if (net_eq(net, &init_net))
    {
        nf_ct_extend_unregister(&spf_extend);
    }
}

#endif // CONFIG_NF_CONNTRACK_TP_APP
