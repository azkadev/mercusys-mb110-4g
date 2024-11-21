/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	nf_conntrack_spf.h
 * Version		:	1.0
 * Abstract		:
 * Author		:	Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date	:	23 Jun 2014
 *
 ***************************************************************/

#ifndef __NF_CONNTRACK_SPF_H__
#define __NF_CONNTRACK_SPF_H__

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <linux/types.h>

#ifdef CONFIG_NF_CONNTRACK_TP_APP

struct nf_conntrack_spf
{
    u32        factor;
    time_t     time;
    atomic64_t packets;
    atomic64_t bytes;
};

static inline
struct nf_conntrack_spf* nf_conn_spf_find(const struct nf_conn* ct)
{
    return nf_ct_ext_find(ct, NF_CT_EXT_SPF);
}

static inline
struct nf_conntrack_spf* nf_ct_spf_ext_add(struct nf_conn* ct, gfp_t gfp)
{
    struct nf_conntrack_spf* spf_ext;

    spf_ext = nf_ct_ext_add(ct, NF_CT_EXT_SPF, gfp);
    if (!spf_ext)
        pr_debug("failed to add spf extentions!");

    return spf_ext;
}

extern int  nf_conntrack_spf_init(struct net* net);
extern void nf_conntrack_spf_fini(struct net* net);

#endif // CONFIG_NF_CONNTRACK_TP_APP
#endif // __NF_CONNTRACK_SPF_H__
