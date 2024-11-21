/*
 * connection tracking appdist.
 */

#ifndef _NF_CONNTRACK_APP_H
#define _NF_CONNTRACK_APP_H

#include <linux/rtnetlink.h>
#include <net/netfilter/nf_conntrack_extend.h>


#ifdef CONFIG_NF_CONNTRACK_TP_APP

struct nf_conntrack_appidntfy
{
    short app_id_index;         /* store identify result */
};

struct nf_conntrack_appclsf
{
   void (*fpFreeHandler)(void *);
   void  *data;
};

struct nf_conntrack_app_ext
{
    void  (*fpFreeHandler)(const void *);
    short proxy_id_index;
};

#define APP_BASIC_ID_UNKNOWN            (short)0xF000
#define APP_BASIC_ID_CHECKING           (short)0xE000

#define APP_NORMAL_ID_UNKNOWN           (short)0x0FFF
#define APP_NORMAL_ID_CHECKING          (short)0x0FFE
#define APP_ID_CHECKING (short)(APP_BASIC_ID_CHECKING | APP_NORMAL_ID_CHECKING)

#define APP_CT_NORMAL_ID_GET(appId)        (short)((appId)&0x0FFF)
#define APP_CT_BASIC_ID_VALUE_GET(appId)   (short)((((appId)&0xF000) >> 12) + 4070)

struct nf_conntrack_app
{
    struct nf_conntrack_appidntfy appidntfy_flag[IP_CT_DIR_MAX];              /* 4 */
    void *appidntfy_fast_entry;                                /* 4 */
    void *appidntfy_ext;                                       /* 4 */
    void *app_flow_stat;                                       /* 4 */
    int  appprio_flag;                                         /* 4 */
    int  appprio_version;                                      /* 4 */
};

typedef struct {
    void (*dec_port_counter)(struct nf_conn *ct);
}nf_conntrack_app_handler_t;

extern nf_conntrack_app_handler_t *nf_ct_app_handler;

static inline struct nf_conntrack_app * nf_ct_get_app(const struct nf_conn *ct)
{
//#ifdef CONFIG_NF_CONNTRACK_APPDIST
    struct nf_conntrack_app *nf_ct_app;
    nf_ct_app = nf_ct_ext_find(ct, NF_CT_EXT_APP);

    if (nf_ct_app)
    {
        return nf_ct_app;
    }
//#endif
    return NULL;
}

static inline int nf_ct_set_app(const struct nf_conn *ct, struct nf_conntrack_app *app)
{
//#ifdef CONFIG_NF_CONNTRACK_APPDIST
    struct nf_conntrack_app *nf_ct_app;
    nf_ct_app = nf_ct_ext_find(ct, NF_CT_EXT_APP);

    if (nf_ct_app)
    {
        return 0;
    }
//#endif
    return -1;
}

static inline void nf_ct_appidntfy_ext_init(struct nf_conntrack_app *nf_ct_app)
{
    struct nf_conntrack_app_ext *appidntfy_ext;

    if (nf_ct_app->appidntfy_ext)
    {
        appidntfy_ext = (struct nf_conntrack_app_ext *)nf_ct_app->appidntfy_ext;
        appidntfy_ext->proxy_id_index = -2;
        appidntfy_ext->fpFreeHandler = kfree;
    }
}

static inline void nf_ct_appidntfy_ext_free(const struct nf_conn *ct)
{
    struct nf_conntrack_app *nfct_app;
    struct nf_conntrack_app_ext *appidntfy_ext;

    nfct_app = nf_ct_get_app(ct);
    if (nfct_app)
    {
        appidntfy_ext = (struct nf_conntrack_app_ext *)nfct_app->appidntfy_ext;
        if (appidntfy_ext)
        {
            if (appidntfy_ext->fpFreeHandler)
            {
                appidntfy_ext->fpFreeHandler(nfct_app->appidntfy_ext);
                nfct_app->appidntfy_ext = NULL;
            }
        }
    }
}


extern spinlock_t nf_conntrack_lock ;

static inline void nf_ct_appidntfy_clsf_free(const struct nf_conn *ct)
{
    struct nf_conntrack_app *nfct_app;
    struct nf_conntrack_appclsf *nfct_appclsf;
    void (*fpFreeHandler)(void * );

    nfct_app = nf_ct_get_app(ct);

    if (nfct_app)
    {
        nfct_appclsf = nfct_app->app_flow_stat;
        if (nfct_appclsf)
        {
            fpFreeHandler = nfct_appclsf->fpFreeHandler;
            if (fpFreeHandler)
            {
                fpFreeHandler(nfct_app->app_flow_stat);
                nfct_app->app_flow_stat = NULL;
            }
        }
    }
}

static inline void nf_ct_appidntfy_destroy(struct nf_conn *ct)
{
    nf_ct_appidntfy_ext_free(ct);
    nf_ct_appidntfy_clsf_free(ct);
    if(nf_ct_app_handler && nf_ct_app_handler->dec_port_counter  )
    {
       nf_ct_app_handler->dec_port_counter(ct);
    }
}


static inline void __nf_conntrack_cleanup_app(struct net *net)
{
    unsigned int i = 0;
    struct nf_conntrack_tuple_hash *h;
    struct nf_conn *ct;
    struct hlist_nulls_node *n;

    for (i = 0; i < nf_conntrack_htable_size; i++) {
        hlist_nulls_for_each_entry(h, n, &net->ct.hash[i], hnnode) {
            ct = nf_ct_tuplehash_to_ctrack(h);
            nf_ct_appidntfy_clsf_free(ct);
        }
    }
    hlist_nulls_for_each_entry(h, n, &net->ct.unconfirmed, hnnode) {
        ct = nf_ct_tuplehash_to_ctrack(h);
        nf_ct_appidntfy_clsf_free(ct);
    }
    return;
}

static inline int nf_ct_appidntfy_show(struct seq_file *s, struct nf_conn *ct)
{
    struct nf_conntrack_app *nfct_app;
    unsigned short          appid_index;

    nfct_app = nf_ct_get_app(ct);

    if (nfct_app)
    {
        appid_index = nfct_app->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index;
        seq_printf(s, "[TP_APP]basicId %u, normalId %u ",
               APP_CT_BASIC_ID_VALUE_GET(appid_index),
               APP_CT_NORMAL_ID_GET(appid_index));

    }
    else
    {
        return -1;
    }

    return 0;
}

#if 1 /* for app_analysis */
typedef struct _nf_app_log
{
    int   conn_status;
    void* connection_ext_info;
    void (*free_info)(void*);
}nf_app_log;


static inline nf_app_log* nf_ct_app_find(const struct nf_conn *ct)
{
    nf_app_log    *nf_ct_app;
    nf_ct_app = (nf_app_log *)nf_ct_ext_find(ct, NF_CT_EXT_APP_ANALYSIS);
    if (nf_ct_app)
        return nf_ct_app;

    return NULL;
}
static inline int nf_ct_app_conn_clear(const struct nf_conn *ct)
{
    nf_app_log    *nf_ct_app = NULL;
    nf_ct_app = (nf_app_log *)nf_ct_ext_find(ct, NF_CT_EXT_APP_ANALYSIS);
    if(!nf_ct_app)
    {
        return 0;
    }
    if(!nf_ct_app->free_info)
    {
        return 0;
    }
    //intk("nf_ct_app_conn_clear\n");
    nf_ct_app->free_info((void*)nf_ct_app->connection_ext_info);
    nf_ct_app->connection_ext_info = NULL;
    nf_ct_app->free_info = NULL;
    nf_ct_app->conn_status = 2; /* NO_RULE */
    return 1;
}
#endif

#endif /* CONFIG_NF_CONNTRACK_TP_APP */

#endif /*_NF_CONNTRACK_APPDIST_H*/

