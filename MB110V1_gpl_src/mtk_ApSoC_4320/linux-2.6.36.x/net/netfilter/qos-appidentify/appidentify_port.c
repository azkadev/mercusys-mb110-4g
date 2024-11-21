/*! Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     app_mod.c
 *\brief
 *\details
 *
 *\author   Weng Kaiping
 *\version
 *\date     15Nov13
 *
 *\warning
 *
 *\history \arg
 */

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/
/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/timer.h>
#include <linux/jhash.h>
#include <linux/mutex.h>
#include <linux/version.h>

#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_app.h>

#include "appidentify_id.h"
#include "appidentify_port.h"
#include "appidentify_log.h"
#include "appprio.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define APP_PORT_ERROR(fmt, args...)   printk("[Error](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args)
#define APP_PORT_INFO(fmt, args...)    \
	if(g_port_debug) { printk("[INFO] (%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args);}   
#define APP_PORT_DEBUG(fmt, args...)   \
    if(g_port_debug) { printk("[Debug](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args);}


#define APP_EVEN_NORMAL_ID_GET(appOri,appReply) \
    ((APP_NORMAL_ID_GET(appOri) == APP_NORMAL_ID_GET(appReply))? (APP_NORMAL_ID_GET(appOri)): APP_NORMAL_ID_CHECKING)

#define P2P_PORT_LEVEL   10

#define APP_PORT_SET_APP_FACTOR 3
#define APP_PORT_CHECK_PERIOD   5
#define APP_PORT_HASH_SIZE      256

#define HASH_KEY_BITS           (8)     /* hash table depth is 64 */
#define HASH_KEY_PORT(port)     hash_long(port, HASH_KEY_BITS)

static inline u32 app_port_hash(const __be32 raddr, const __be16 rport,
                 const u32 rnd, const u32 hsize)
{
    return jhash_2words((__force u32)raddr, (__force u32)rport, rnd) & (hsize - 1);
}
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef enum _APP_PORT_PROTO_INDEX
{
    APP_PORT_PROTO_TCP    = 0,
    APP_PORT_PROTO_UDP,
    APP_PORT_PROTO_MAX
} APP_PORT_PROTO_INDEX;

typedef struct _app_port_ct_map
{
    unsigned int   internIp;
    unsigned short internPort;
    unsigned short externPort;
    unsigned int   ctNum;
    unsigned int   newCtNum;
    unsigned char  proto;
    short          normalApp;
    short          factor;
    struct timer_list timeout;

}app_port_ct_map;

typedef struct _app_port_ct_node
{
   struct hlist_node  list;
   app_port_ct_map    map;
}app_port_ct_node;

typedef enum _app_port_ct_handle
{
    app_port_ct_inc,
    app_port_ct_dec
}app_port_ct_handle;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
static bool app_port_initd = false;
static char app_ct_num_per_port[APP_PORT_PROTO_MAX][MAX_PORT_NUM] = {{0}};
static struct kmem_cache * app_port_cache;
static struct hlist_head  app_port_ct_list[APP_PORT_PROTO_MAX][APP_PORT_HASH_SIZE];
static char app_port_check_cnt = 0;

extern void nf_conntrack_ports_lookup(void);

bool g_port_debug = false;
bool g_enablePort = true;

static DEFINE_MUTEX(app_port_mutex);
/**************************************************************************************************/
/*                                      FUNCTION                                                */
/**************************************************************************************************/
app_port_ct_node *
app_port_ct_find_node(unsigned char protoIndex,unsigned int ip, unsigned short internPort)
{
    app_port_ct_node *pNode;
    struct hlist_node *pos;
    struct hlist_node *next;
    unsigned char hash;

    hash = app_port_hash(ip, internPort, 1,APP_PORT_HASH_SIZE);
    hlist_for_each_safe(pos, next, &app_port_ct_list[protoIndex][hash])
    {
        pNode = hlist_entry(pos, app_port_ct_node, list);
        if ( pNode->map.internIp == ip && pNode->map.internPort == internPort)
        {
            return pNode;
        }
    }

    return NULL;
}

app_port_ct_node * app_port_ct_init_node(unsigned char  protoIndex, struct nf_conn *pCtEntry)
{
    app_port_ct_node * pNode = NULL;
    unsigned char hash;
    unsigned int ip = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
    unsigned short internPort = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all;
    unsigned short externPort = pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all;

    hash = app_port_hash(ip, internPort, 1,APP_PORT_HASH_SIZE);

    pNode = kmem_cache_alloc(app_port_cache,GFP_ATOMIC);
    if(NULL == pNode)
    {
        return NULL;
    }
    pNode->map.internIp  = ip;
    pNode->map.internPort= internPort;
    pNode->map.externPort= externPort;
    pNode->map.ctNum     = app_ct_num_per_port[protoIndex][externPort];
    pNode->map.newCtNum  = 0;
    pNode->map.normalApp = APP_NORMAL_ID_UNKNOWN;

    /*setup_timer(&pNode->map.timeout, app_port_ct_timeout ,pNode);
    pNode->map.timeout.expires = jiffies + HZ;
    add_timer(&pNode->map.timeout);*/


    hlist_add_head(&pNode->list,&app_port_ct_list[protoIndex][hash]);

    return pNode;

}

void app_port_debug_add_node(void)
{
    app_port_ct_node * pNode = NULL;
    unsigned char protoIndex = APP_PORT_PROTO_UDP;

    struct nf_conn ct;
    ct.tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip = 0xc0a80064;
    ct.tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all = 10000;
    ct.tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all = 10000;
    app_ct_num_per_port[protoIndex][10000] = 13;

    pNode = app_port_ct_init_node(protoIndex, &ct);

    app_ct_num_per_port[protoIndex][10000] = -1;


}

void app_port_debug_add_node1(void)
{
    app_port_ct_node * pNode = NULL;
    unsigned char protoIndex = APP_PORT_PROTO_UDP;

    struct nf_conn ct;
    ct.tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip = 0xc0a80064;
    ct.tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all = 10001;
    ct.tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all = 10001;
    app_ct_num_per_port[protoIndex][10001] = 13;

    pNode = app_port_ct_init_node(protoIndex, &ct);

    pNode->map.normalApp = APP_ID_PPSTREAM;
    pNode->map.factor    = 4;

    app_ct_num_per_port[protoIndex][10001] = -1;

}

int app_port_ct_del_node(unsigned char  protoIndex,app_port_ct_node *pNode)
{
    if(NULL == pNode)
    {
        APP_PORT_ERROR("can't find the node to del\n");
        return -1;
    }

    app_ct_num_per_port[protoIndex][pNode->map.externPort] = pNode->map.ctNum;
    hlist_del(&pNode->list);
    kmem_cache_free(app_port_cache, pNode);
    return 0;
}

void inc_port_counter(struct nf_conn *pCtEntry)
{
    unsigned short port  = pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all;
    unsigned char  proto = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    unsigned char  protoIndex;
    app_port_ct_node * pNode = NULL;

    if (!app_port_initd || port < 1 || port > 65535)
    {
        return ;
    }

    protoIndex = (proto == IPPROTO_TCP ? APP_PORT_PROTO_TCP : APP_PORT_PROTO_UDP);

    //mutex_lock(&app_port_mutex);
    if (-1 != app_ct_num_per_port[protoIndex][port] && app_ct_num_per_port[protoIndex][port] < (P2P_PORT_LEVEL -1))
    {
        app_ct_num_per_port[protoIndex][port]++;
        APP_PORT_DEBUG("inc port:%d, num:%d\n", port, app_ct_num_per_port[protoIndex][port])
    }
    else if(app_ct_num_per_port[protoIndex][port] >= (P2P_PORT_LEVEL -1))
    {
        pNode = app_port_ct_init_node(protoIndex, pCtEntry);
        if (NULL == pNode)
        {
            APP_PORT_ERROR("init node fail\n");
            app_ct_num_per_port[protoIndex][port]++;
            goto out;
        }
        APP_PORT_INFO("insert node:port(%d), ctnum(%d)\n",port, app_ct_num_per_port[protoIndex][port]);
        app_ct_num_per_port[protoIndex][port] = -1;
        pNode->map.ctNum++;
        pNode->map.newCtNum++;
    }
    else
    {
        pNode = app_port_ct_find_node(protoIndex,
                                      pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                                      pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
        if (NULL == pNode)
        {
            APP_PORT_ERROR("find node fail\n");
            goto out;
        }

        pNode->map.ctNum ++;
        pNode->map.newCtNum ++;
    }
out:
    //mutex_unlock(&app_port_mutex);
    return ;
}
EXPORT_SYMBOL(inc_port_counter);

void dec_port_counter(struct nf_conn *pCtEntry )
{
    struct nf_conntrack_app * app_info;
    app_port_ct_node *pNode = NULL;
    unsigned short port = pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all;
    unsigned char  proto = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    unsigned char  protoIndex;

    if (!app_port_initd || port < 1 || port > 65535)
    {
        return;
    }

    app_info = nf_ct_get_app(pCtEntry);
    if ( NULL == app_info)
    {
        return ;
    }

    if (!APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_CNT))
    {
        return ;
    }

    protoIndex = (proto == IPPROTO_TCP ? APP_PORT_PROTO_TCP : APP_PORT_PROTO_UDP);

    //mutex_lock(&app_port_mutex);
    if (-1 != app_ct_num_per_port[protoIndex][port])
    {
        if(app_ct_num_per_port[protoIndex][port] < 0)
        {
            APP_PORT_ERROR("app_ct_num_per_port[%d] is %d\n", port, app_ct_num_per_port[protoIndex][port]);
        }
        APP_PORT_DEBUG("dec port:%d, num:%d\n", port, app_ct_num_per_port[protoIndex][port]);
        app_ct_num_per_port[protoIndex][port]--;
    }
    else
    {
        pNode = app_port_ct_find_node(protoIndex,
                                      pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                                      pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
        if (NULL == pNode)
        {
            APP_PORT_ERROR("can't find node to del\n");
            goto out;
        }

        pNode->map.ctNum --;
        APP_PORT_DEBUG("dec port:%d, num:%d\n", port, pNode->map.ctNum)
        if(pNode->map.ctNum < 4)
        {
            APP_PORT_INFO("del node:port(%d), ctnum(%d)",port, pNode->map.ctNum);
            app_port_ct_del_node(protoIndex, pNode);
            pNode = NULL;
        }
    }
out:
    //mutex_unlock(&app_port_mutex);

    return;
}
EXPORT_SYMBOL(dec_port_counter);

unsigned int appidentify_port_hook(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
    struct nf_conn     *pCtEntry = NULL;
    struct nf_conntrack_app * app_info;
    enum ip_conntrack_info ctinfo;
    app_port_ct_node *pNode = NULL;
    struct nf_conn_counter *counter;
    unsigned int packetDirection;

    short     normalApp   = APP_NORMAL_ID_CHECKING;

    unsigned char  proto;
    unsigned char  protoIndex;

    if(!g_enablePort)
    {
        return NF_ACCEPT;
    }

    if(!app_port_initd)
    {
        return NF_ACCEPT;
    }

    pCtEntry = nf_ct_get(skb, &ctinfo);
    if ( NULL == pCtEntry )
    {
        return NF_ACCEPT;
    }

    app_info = nf_ct_get_app(pCtEntry);
    if ( NULL == app_info)
    {
        return NF_ACCEPT;
    }

    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER))
    {
        return NF_ACCEPT;
    }

    proto = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    if ( IPPROTO_UDP != proto && IPPROTO_TCP != proto)/**/
    {
        return NF_ACCEPT;
    }

    //mutex_lock(&app_port_mutex);

    /* check from the second packet */
    protoIndex = (proto == IPPROTO_TCP ? APP_PORT_PROTO_TCP : APP_PORT_PROTO_UDP);
    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_CNT)
        && -1 != app_ct_num_per_port[protoIndex][pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all])
    {
        goto out;;
    }

    /*check for all the first packet and part of the next packets*/
    if(!APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_SET))
    {
        protoIndex = (proto == IPPROTO_TCP ? APP_PORT_PROTO_TCP : APP_PORT_PROTO_UDP);
        pNode = app_port_ct_find_node(protoIndex,
                                      pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                                      pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
        if (NULL == pNode)
        {
            goto out;
        }

        normalApp = APP_EVEN_NORMAL_ID_GET(app_info->appidntfy_flag[0].app_id_index,app_info->appidntfy_flag[1].app_id_index);

        if ((normalApp != APP_NORMAL_ID_CHECKING && normalApp != APP_NORMAL_ID_UNKNOWN))
        {
            if( APP_NORMAL_ID_UNKNOWN == pNode->map.normalApp)
            {
                pNode->map.normalApp = normalApp;
                pNode->map.factor = 1;
            }
            else if ( normalApp == pNode->map.normalApp)
            {
                pNode->map.factor++;
            }
            else
            {
                pNode->map.factor = pNode->map.factor - 3;
                if (pNode->map.factor <= 0)
                {
                    pNode->map.normalApp = APP_NORMAL_ID_UNKNOWN;
                    pNode->map.factor = 0;
                }
            }
            APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_SET);

        }
        else if (APP_NORMAL_ID_UNKNOWN != pNode->map.normalApp && pNode->map.factor >= 2)
        {
            if( APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_CHECK))
            {
                goto out;
            }
            else if (APP_PORT_CHECK_PERIOD == app_port_check_cnt)
            {
                app_port_check_cnt = 0;
                APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_CHECK);
                goto out;
            }

            APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, pNode->map.normalApp);
            APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, pNode->map.normalApp);
            APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_SET);
            app_port_check_cnt++;

            counter = nf_conn_acct_find(pCtEntry);
            if (NULL == counter)
            {
                APP_PORT_ERROR("counter null.\r\n");
                goto out;
            }
            packetDirection = CTINFO2DIR(ctinfo);
			#if 0
            APP_PORT_INFO("APP PORT SET:appId %u,dir %s, counter %llu, ori %x:%d---%x:%d, proto %s, reply %x:%d---%x:%d.\r\n",
                               pNode->map.normalApp,
                               packetDirection == IP_CT_DIR_ORIGINAL ? "ORI" : "REP",
                               counter[packetDirection].packets,
                               pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                               pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
                               pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
                               pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,
                               IPPROTO_TCP == pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                               pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip,
                               pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all,
                               pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip,
                               pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);
			#endif
        }
        else
        {
            struct nf_conn_counter *counter;
            counter = nf_conn_acct_find(pCtEntry);
            if (NULL == counter)
            {
                APP_PORT_ERROR("counter null.\r\n");
                goto out;
            }

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
		if(counter[IP_CT_DIR_ORIGINAL].packets + counter[IP_CT_DIR_REPLY].packets > 100)
#else
            if(counter[IP_CT_DIR_ORIGINAL].packets.counter + counter[IP_CT_DIR_REPLY].packets.counter > 100)
#endif
            {
                APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_SET);
            }
        }
    }

out:

    //mutex_unlock(&app_port_mutex);

    return NF_ACCEPT;
}

unsigned int appidentify_port_count(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
    struct nf_conn     *pCtEntry = NULL;
    struct nf_conntrack_app * app_info;
    enum ip_conntrack_info ctinfo;
    unsigned char  proto;

    if(!g_enablePort)
    {
        return NF_ACCEPT;
    }

    if(!app_port_initd)
    {
        return NF_ACCEPT;
    }

    pCtEntry = nf_ct_get(skb, &ctinfo);
    if ( NULL == pCtEntry )
    {
        return NF_ACCEPT;
    }

    app_info = nf_ct_get_app(pCtEntry);
    if ( NULL == app_info)
    {
        return NF_ACCEPT;
    }

    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER))
    {
        return NF_ACCEPT;
    }

    proto = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    if ( IPPROTO_UDP != proto && IPPROTO_TCP != proto)/**/
    {
        return NF_ACCEPT;
    }

    /* check for the first packet*/
    if (!APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_CNT)
        && test_bit(IPS_SRC_NAT_DONE_BIT, &pCtEntry->status))
    {
        inc_port_counter(pCtEntry);
        APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_PORT_CNT);
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops app_port_ops =
{
    .hook       = appidentify_port_count,
    .owner      = THIS_MODULE,
    .pf         = PF_INET,
    .hooknum    = NF_INET_POST_ROUTING,
    .priority   = NF_IP_PRI_NAT_SRC + 1,
};

int appidentify_port_init(void)
{
    int i = 0;
    int protoIndex = APP_PORT_PROTO_TCP;
    int ret = 0;

    for ( protoIndex = APP_PORT_PROTO_TCP; protoIndex < APP_PORT_PROTO_MAX; protoIndex++)
    {
        for ( i = 0; i < MAX_PORT_NUM; i++)
        {
           app_ct_num_per_port[protoIndex][i] = 0;
        }
    }

    for ( protoIndex = APP_PORT_PROTO_TCP; protoIndex < APP_PORT_PROTO_MAX; protoIndex++)
    {
        for( i = 0; i < APP_PORT_HASH_SIZE; i++)
        {
           INIT_HLIST_HEAD(&app_port_ct_list[protoIndex][i]);
        }
    }

    app_port_cache = kmem_cache_create("app_port_cache", sizeof(app_port_ct_node),
                                       0, SLAB_HWCACHE_ALIGN, NULL);

    /*nf_conntrack_ports_lookup();*/
    app_port_debug_add_node();
    app_port_debug_add_node1();

    ret = nf_register_hook(&app_port_ops);
    if (ret < 0) {
        APP_PORT_ERROR("can't register hooks.\n");
        goto cleanup_hooks;
    }

    app_port_initd = true;

    return ret;

cleanup_hooks:
    nf_unregister_hook(&app_port_ops);
    return ret;

}

void appidentify_port_exit(void)
{
    int i = 0;
    int protoIndex ;
    app_port_ct_node *pNode;

    app_port_initd = false;

    nf_unregister_hook(&app_port_ops);

    for ( protoIndex = APP_PORT_PROTO_TCP; protoIndex < APP_PORT_PROTO_MAX; protoIndex++)
    {
        for(i = 0; i < APP_PORT_HASH_SIZE; i++)
        {
            while(!hlist_empty(&app_port_ct_list[protoIndex][i]))
            {
               pNode = hlist_entry(app_port_ct_list[protoIndex][i].first,app_port_ct_node, list);
               hlist_del(&pNode->list);
               kmem_cache_free(app_port_cache, pNode);
            }
        }
    }
    kmem_cache_destroy(app_port_cache);
    printk("appidentify port exit ok!\n");
}
