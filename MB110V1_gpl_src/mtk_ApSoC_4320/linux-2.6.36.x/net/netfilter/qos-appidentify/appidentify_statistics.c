/*! Copyright(c) 2008-2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_statistics.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     10Feb14
 *
 *\warning
 *
 *\history \arg 0.0.1, 10Feb14, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/spinlock.h>
#include <linux/timer.h>

#include "appprio.h"
#include "appidentify.h"
#include "appidentify_log.h"
#include "appidentify_id.h"
#include "appidentify_utils.h"
#include "appidentify_statistics.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
static DEFINE_SPINLOCK(app_stat_lock);

#define APPIDNTFY_STATCS_ENTRY_MAXNUM       (2048)
#define APPIDNTFY_STATCS_ENTRY_RESNUM       (8)

#define APPIDNTFY_STATCS_SET_MAXNUM         (1024)
#define APPIDNTFY_STATCS_SET_RESNUM         (8)

/* for high speed conn watched */
#define APPIDNTFY_WATCH_ENTRY_MAXNUM         (16)
#define APPIDNTFY_WATCH_ENTRY_RESNUM         (2)

#define APPIDNTFY_WATCH_TUPLE_MAXNUM         (512)
#define APPIDNTFY_WATCH_TUPLE_RESNUM         (8)

#define INVALID_APPID                       (-1)

#define SPEED_THRESHOLD                     (50 * 1024)  /* 50KB/s */

#define WARN_TEST_TIMES                     (5)
#define HALF_BEYOND_TIMES                   (WARN_TEST_TIMES >> 1)

int app_stat_debug = FALSE;

#define     APPIDNTFY_STAT_ERROR(fmt, args...)  printk("STAT_ERROR[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args)

#define     APPIDNTFY_STAT_DEBUG(fmt, args...)                                    \
            do                                                              \
            {                                                               \
                if (TRUE == app_stat_debug)                                    \
                {                                                           \
                    printk("STAT_DEBUG[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args);  \
                }                                                           \
            }while(0)
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
extern bool enable_print;

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/
void *
appidentify_stat_pool_alloc(STAT_POOL_TYPE type);
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
int g_app_stat_enable = TRUE;
static struct timer_list app_stat_timer;

APPIDNTFY_MEMPOOL       l_app_stat_pool;
APPIDNTFY_MEMPOOL       l_stat_set_pool;

APPIDNTFY_MEMPOOL       l_watch_list_pool;
APPIDNTFY_MEMPOOL       l_tuple_list_pool;

struct list_head        l_stat_appid_list;
struct list_head        l_stat_mac_list;

struct list_head        l_appid_list;
struct list_head        l_devip_list;

struct list_head        l_app_watch_list;
struct list_head        l_app_tuple_list;
#if 0
static APPIDNTFY_STATCS prio_stat_entries[APPPRIO_PRIO_TOP - 1];
#endif

int l_app_stat_inited           = FALSE;
//static unsigned int lan_ip      = 0x00000000;
static unsigned int lan_ip      = ( (192 << 24) + (168 << 16) + (3 << 8) + 1 );
static unsigned int lan_mask    = 0xFFFFFF00;
static int stat_period          = 2; /* 2 seconds */
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
int
_free_watch_entry(WATCH_APP_LIST *entry)
{
    WATCH_TUPLE_LIST *tuple_entry;
    WATCH_TUPLE_LIST *tmp_tuple_entry;

    if (!entry)
    {
        return -1;
    }

    list_del(&entry->watch_list_node);
    list_for_each_entry_safe(tuple_entry, tmp_tuple_entry, &entry->tuple_list_head, tuple_list_node)
    {
        list_del(&tuple_entry->tuple_lru_node);
        list_del(&tuple_entry->tuple_list_node);
        appidentify_mempool_free_limit(tuple_entry, &l_tuple_list_pool);
    }
    appidentify_mempool_free_limit(entry, &l_watch_list_pool);

    return 0;
}

int
_check_set_element(struct list_head *set_list, int value)
{
    STATCS_SET  *temp_set;
    int         found_flag = 0;

    if (!set_list)
    {
        APPIDNTFY_STAT_ERROR("mem alloc error.\r\n");
        return -1;
    }

    if (-1 == value)
    {
        return 0;
    }

    list_for_each_entry(temp_set, set_list, list)
    {
        if (temp_set->set.appid == value)   /* appid or ip in union */
        {
            found_flag = 1;
        }
    }
    if (!found_flag)
    {
        temp_set = (STATCS_SET *)appidentify_stat_pool_alloc(STAT_POOL_TYPE_SET);
        if (!temp_set)
        {
            APPIDNTFY_STAT_ERROR("mem alloc error.\r\n");
            return -1;
        }
        temp_set->set.appid = value;
        list_add(&temp_set->list, set_list);   /* re-add to the head. LRU */
    }

    return 0;
}

WATCH_APP_LIST *
_search_watch_entry(unsigned int ip4, int appid)
{
    WATCH_APP_LIST *watch_entry;

    list_for_each_entry(watch_entry, &l_app_watch_list, watch_list_node)
    {
        if (watch_entry->app.appid == appid &&
            watch_entry->app.ip4 == ip4)
        {
            return watch_entry;
        }
    }

    return NULL;
}

int
_add_new_watch_entry(unsigned int ip4, int appid, short prio)
{
    WATCH_APP_LIST *watch_entry;
    WATCH_TUPLE_LIST *tuple_entry;
    WATCH_TUPLE_LIST *tmp_tuple_entry;

    watch_entry = appidentify_mempool_alloc_limit(&l_watch_list_pool, GFP_ATOMIC);
    if(!watch_entry)
    {
        if (!list_empty(&l_app_watch_list))
        {
            /* get the last list entry. LRU */
            watch_entry = list_entry(l_app_watch_list.prev, WATCH_APP_LIST, watch_list_node);
            list_del(&watch_entry->watch_list_node);
            if (!list_empty(&watch_entry->tuple_list_head))
            {
                list_for_each_entry_safe(tuple_entry, tmp_tuple_entry,
                                         &watch_entry->tuple_list_head, tuple_list_node)
                {
                    list_del(&tuple_entry->tuple_list_node);
                    list_del(&tuple_entry->tuple_lru_node);
                    appidentify_mempool_free_limit(tuple_entry, &l_tuple_list_pool);
                }
            }
        }
        else
        {
            return -1;
        }
    }

    if (watch_entry)
    {
        memset(watch_entry, 0, sizeof(WATCH_APP_LIST));
    }

    watch_entry->app.ip4      = ip4;
    watch_entry->app.appid    = appid;
    watch_entry->app.app_prio = prio;

    list_add(&watch_entry->watch_list_node, &l_app_watch_list);
    INIT_LIST_HEAD(&watch_entry->tuple_list_head);

    return 0;
}

WATCH_TUPLE_LIST *
_search_tuple_entry(WATCH_TUPLE *tuple, struct list_head *tuple_list_head)
{
    WATCH_TUPLE_LIST *tuple_entry;

    if (tuple == NULL ||
        tuple_list_head == NULL)
    {
        return NULL;
    }

    list_for_each_entry(tuple_entry, tuple_list_head, tuple_list_node)
    {
        if (tuple_entry->ori_tuple.protonum == tuple->protonum &&
            tuple_entry->ori_tuple.src_ip == tuple->src_ip &&
            tuple_entry->ori_tuple.dst_ip == tuple->dst_ip &&
            tuple_entry->ori_tuple.src_port == tuple->src_port &&
            tuple_entry->ori_tuple.dst_port == tuple->dst_port)
        {
            return tuple_entry;
        }
    }

    return NULL;
}

WATCH_TUPLE_LIST *
_add_new_watch_tuple(WATCH_TUPLE *tuple, struct list_head *tuple_list_head)
{
    WATCH_TUPLE_LIST *tuple_entry;

    tuple_entry = appidentify_mempool_alloc_limit(&l_tuple_list_pool, GFP_ATOMIC);
    if(!tuple_entry)
    {
        if (!list_empty(&l_app_tuple_list))
        {
            /* get the last list entry. LRU */
            tuple_entry = list_entry(l_app_tuple_list.prev, WATCH_TUPLE_LIST, tuple_lru_node);
            list_del(&tuple_entry->tuple_lru_node);
            list_del(&tuple_entry->tuple_list_node);
        }
        else
        {
            return NULL;
        }
    }

    if (tuple_entry)
    {
        memset(tuple_entry, 0, sizeof(WATCH_TUPLE_LIST));
    }

    memcpy(&tuple_entry->ori_tuple, tuple, sizeof(WATCH_TUPLE));
    list_add(&tuple_entry->tuple_lru_node, &l_app_tuple_list); /* add to the LRU list */
    list_add(&tuple_entry->tuple_list_node, tuple_list_head);

    return tuple_entry;
}

void *
appidentify_stat_pool_alloc(STAT_POOL_TYPE type)
{
    APPIDNTFY_MEMPOOL   *mempool;
    struct list_head    *stat_list;
    void                *entry = NULL;
    size_t              data_size;

    if (STAT_POOL_TYPE_ENTRY == type)
    {
        mempool = &l_app_stat_pool;
        if (!list_empty(&l_stat_appid_list))
        {
            stat_list = &l_stat_appid_list;
        }
        else if (!list_empty(&l_stat_mac_list))
        {
            stat_list = &l_stat_mac_list;
        }
        else
        {
            stat_list = NULL;
        }

        data_size = sizeof(APPIDNTFY_STATCS);
    }
    else if (STAT_POOL_TYPE_SET == type)
    {
        mempool = &l_stat_set_pool;
        if (!list_empty(&l_appid_list))
        {
            stat_list = &l_appid_list;
        }
        else if (!list_empty(&l_devip_list))
        {
            stat_list = &l_devip_list;
        }
        else
        {
            stat_list = NULL;
        }

        data_size = sizeof(STATCS_SET);
    }
    else
    {
        APPIDNTFY_STAT_ERROR("mempool type error.\r\n");
        return NULL;
    }

    entry = appidentify_mempool_alloc_limit(mempool, GFP_ATOMIC);
    if (!entry && !stat_list)
    {
        spin_lock_bh(&app_stat_lock);
        stat_list = stat_list->prev;   /* del the tail element. LRU */
        list_del(stat_list);
        spin_unlock_bh(&app_stat_lock);

        entry = (void *)stat_list;
    }
    if (entry != NULL)
    {
        memset(entry, 0, data_size);
    }

    APPIDNTFY_STAT_DEBUG("\r\n");
    return entry;
}


APPIDNTFY_STATCS*
appidentify_stat_entry_find(int appid, unsigned int ip)
{
    APPIDNTFY_STATCS    *stat_entry;

    if (_check_set_element(&l_appid_list, appid) < 0)
    {
        APPIDNTFY_STAT_ERROR("appid set element check error.\r\n");
        return NULL;
    }

    if (_check_set_element(&l_devip_list, ip) < 0)
    {
        APPIDNTFY_STAT_ERROR("ip set element check error.\r\n");
        return NULL;
    }

    list_for_each_entry(stat_entry, &l_stat_appid_list, list)
    {
        if (stat_entry->stat_unit.app_ip_set.appid == appid &&
            stat_entry->stat_unit.app_ip_set.ip4 == ip)
        {
            APPIDNTFY_STAT_DEBUG("find the entry appid %d, ip %pI4\r\n",
                                 appid, &ip);
            list_del(&stat_entry->list); /* take off the list */
            return stat_entry;
        }
    }
    stat_entry = (APPIDNTFY_STATCS*)appidentify_stat_pool_alloc(STAT_POOL_TYPE_ENTRY);
    stat_entry->stat_unit.app_ip_set.appid = appid;
    stat_entry->stat_unit.app_ip_set.ip4 = ip;

    return stat_entry;
}

void appidentify_stat_monitor(unsigned long para)
{
    APPIDNTFY_STATCS    *stat_entry;
    WATCH_TUPLE_LIST    *tuple_entry;
    static unsigned int threshold_cnt;
    WATCH_APP_LIST      *watch_entry;
    WATCH_APP_LIST      *tmp_watch_entry;

    if (!threshold_cnt)
    {
        threshold_cnt = SPEED_THRESHOLD * stat_period;
    }

    list_for_each_entry(stat_entry, &l_stat_appid_list, list)
    {
        spin_lock_bh(&app_stat_lock);
        if (stat_entry->all.bytes != stat_entry->old.bytes)
        {
            stat_entry->recent.bytes = stat_entry->all.bytes - stat_entry->old.bytes;
            stat_entry->old.bytes = stat_entry->all.bytes;

            if (stat_entry->stat_unit.app_ip_set.appid == APP_ID_HTTP)
            {
                if (stat_entry->recent.bytes > threshold_cnt)
                {
                    stat_entry->warn.ex_cnt ++;
                    stat_entry->warn.total_cnt ++;

                    APPID_LOG(APPID_STAT, " #SPEED WARNNING# appid:%d ip:%pI4 [%d/%d]",
                              stat_entry->stat_unit.app_ip_set.appid,
                              &(stat_entry->stat_unit.app_ip_set.ip4),
                              stat_entry->warn.ex_cnt,
                              stat_entry->warn.total_cnt);
                }
                else if (stat_entry->warn.total_cnt)
                {
                    stat_entry->warn.total_cnt ++;
                }

                if (stat_entry->warn.total_cnt == WARN_TEST_TIMES)
                {
                    if (stat_entry->warn.ex_cnt > HALF_BEYOND_TIMES)
                    {
                        APPID_LOG(APPID_STAT, " #SPEED WARNNING# add appid: %d ip:%pI4 to watch list",
                                  stat_entry->stat_unit.app_ip_set.appid,
                                  &(stat_entry->stat_unit.app_ip_set.ip4));

                        watch_entry = _search_watch_entry(stat_entry->stat_unit.app_ip_set.ip4,
                                                          stat_entry->stat_unit.app_ip_set.appid);
                        if (!watch_entry)
                        {
                            if (_add_new_watch_entry(stat_entry->stat_unit.app_ip_set.ip4,
                                                     stat_entry->stat_unit.app_ip_set.appid,
                                                     stat_entry->stat_unit.app_ip_set.app_prio) < 0)
                            {
                                APPIDNTFY_STAT_ERROR("_add_new_watch_entry error.\r\n");
                            }
                        }
                    }
                    /* whether ex_cnt beyond or not, reset the cnt for next stat. */
                    stat_entry->warn.ex_cnt = 0;
                    stat_entry->warn.total_cnt = 0;
                }
            }
        }
        else
        {
            stat_entry->recent.bytes = 0;
        }
        spin_unlock_bh(&app_stat_lock);
    }

    list_for_each_entry_safe(watch_entry, tmp_watch_entry, &l_app_watch_list, watch_list_node)
    {
        spin_lock_bh(&app_stat_lock);
        if (watch_entry->free_flag)
        {
            if (_free_watch_entry(watch_entry) < 0)
            {
                APPIDNTFY_STAT_ERROR("_free_watch_entry error.\r\n");
            }
        }
        spin_unlock_bh(&app_stat_lock);
    }

    list_for_each_entry(tuple_entry, &l_app_tuple_list, tuple_lru_node)
    {
        spin_lock_bh(&app_stat_lock);
        tuple_entry->recent.bytes = tuple_entry->all.bytes - tuple_entry->old.bytes;
        tuple_entry->old.bytes = tuple_entry->all.bytes;

        if (tuple_entry->recent.bytes > threshold_cnt)
        {
            tuple_entry->warn.ex_cnt ++;
            tuple_entry->warn.total_cnt ++;
        }
        else if (tuple_entry->warn.total_cnt)
        {
            tuple_entry->warn.total_cnt ++;
        }

        if (tuple_entry->warn.total_cnt == WARN_TEST_TIMES)
        {
            if (tuple_entry->warn.ex_cnt > HALF_BEYOND_TIMES)
            {
                /* APPID_LOG(APPID_STAT, " #SPEED WARNNING# %pI4:%d => %pI4:%d set warn flag", */
                /*           tuple_entry->ori_tuple.src_ip, */
                /*           tuple_entry->ori_tuple.src_port, */
                /*           tuple_entry->ori_tuple.dst_ip, */
                /*           tuple_entry->ori_tuple.dst_port */
                /*     ); */

                APPID_LOG(APPID_STAT, " #SPEED WARNNING# set warn flag");

                tuple_entry->warn.warn_flag = TRUE;
            }
        }
        spin_unlock_bh(&app_stat_lock);
    }


#if 0
    for(index = 0; index < APPPRIO_PRIO_TOP - 1; index ++)
    {
        spin_lock_bh(&app_stat_lock);
        stat_entry = &prio_stat_entries[index];
        if (stat_entry->all.bytes != stat_entry->old.bytes)
        {
            stat_entry->recent.bytes = stat_entry->all.bytes - stat_entry->old.bytes;
            stat_entry->old.bytes = stat_entry->all.bytes;
        }
        else
        {
            stat_entry->recent.bytes = 0;
        }
        spin_unlock_bh(&app_stat_lock);
    }
#endif

    mod_timer(&app_stat_timer, jiffies + stat_period * HZ);
}
/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
unsigned int appidentify_statistics_hook(unsigned int hook,
                 struct sk_buff *skb,
                 const struct net_device *in,
                 const struct net_device *out,
                 int (*okfn)(struct sk_buff *))
{
    struct nf_conn          *ct         = NULL;
    unsigned int            dir         = IP_CT_DIR_MAX;
    enum ip_conntrack_info  ctinfo;
    struct nf_conntrack_app *appinfo;
    int                     appid       = INVALID_APPID;
    struct iphdr            *iph;
    struct tcphdr           *tcph;
    short                   appIdOri;
    short                   appIdReply;
    short                   normalAppOri;
    short                   basicAppOri;
    short                   normalAppReply;
    short                   basicAppReply;
    unsigned int            ori_srcip;
    unsigned int            ori_dstip;
    unsigned int            client_ip;
    APPIDNTFY_STATCS        *stat_entry;
    unsigned int            pkt_len;
    unsigned int            app_prio;
    WATCH_APP_LIST          *watch_entry;
    WATCH_TUPLE             watch_tuple;
    WATCH_TUPLE_LIST        *tuple_entry;
	
	//printk("APP_STAT: #1\r\n");
	
    if (!l_app_stat_inited)
    {
        return NF_ACCEPT;
    }

    if (FALSE == g_app_stat_enable)
    {
        return NF_ACCEPT;
    }

    if (0 == lan_ip)
    {
        return NF_ACCEPT;
    }
	
	//printk("lan info set ip: %pI4, mask: %pI4", &lan_ip, &lan_mask);

    ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct)
    {
        return NF_ACCEPT;
    }
	
    appinfo = nf_ct_get_app(ct);
    if(NULL == appinfo)
    {
        APPIDNTFY_STAT_ERROR("no appidntf_info.");
        return NF_ACCEPT;
    }

    if (APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER))
    {
        return NF_ACCEPT;
    }

    iph = ip_hdr(skb);
    if (NULL == iph)
    {
        return NF_ACCEPT;
    }

    tcph = (void *)iph + iph->ihl * 4;
    pkt_len = ntohs(iph->tot_len) + 18;
    dir = CTINFO2DIR(ctinfo);
    appIdOri = appinfo->appidntfy_flag[dir].app_id_index;
    appIdReply = appinfo->appidntfy_flag[1 - dir].app_id_index;
    ori_srcip = ntohl(iph->saddr);
    ori_dstip = ntohl(iph->daddr);
    app_prio = APPPRIO_PRI_GET(appinfo->appprio_flag);

    if ((ori_srcip & lan_mask) == (lan_ip & lan_mask))
    {
        client_ip = ori_srcip;
    }
    else if ((ori_dstip & lan_mask) == (lan_ip & lan_mask))
    {
        client_ip = ori_dstip;
    }
    else
    {
        return NF_ACCEPT;
    }

    if ((APP_NORMAL_ID_GET(appIdOri) != APP_NORMAL_ID_CHECKING) &&
        (APP_NORMAL_ID_GET(appIdOri) != APP_NORMAL_ID_UNKNOWN))
    {
        normalAppOri    = APP_NORMAL_ID_GET(appIdOri);
        normalAppReply  = APP_NORMAL_ID_GET(appIdReply);
        if (normalAppReply == normalAppOri)
        {
            appid = normalAppReply;
        }
    }
    else if (INVALID_APPID == appid &&
        (APP_BASIC_ID_FLAG_GET(appIdOri) != APP_BASIC_ID_CHECKING) &&
        (APP_BASIC_ID_FLAG_GET(appIdOri) != APP_BASIC_ID_UNKNOWN))
    {
        basicAppOri     = APP_BASIC_ID_VALUE_GET(appIdOri);
        basicAppReply   = APP_BASIC_ID_VALUE_GET(appIdReply);
        if (basicAppReply == basicAppOri)
        {
            appid = basicAppReply;
        }
    }

    if (client_ip != lan_ip)
    {		
        spin_lock_bh(&app_stat_lock);
        stat_entry = appidentify_stat_entry_find(appid, client_ip);
        stat_entry->all.bytes += pkt_len;
        if (IP_CT_DIR_ORIGINAL == dir)
        {
            stat_entry->tx_cnt.bytes += pkt_len;
        }
        else if (IP_CT_DIR_REPLY == dir)
        {
            stat_entry->rx_cnt.bytes += pkt_len;
        }

        APPIDNTFY_STAT_DEBUG("prio %d.\r\n", app_prio);
        stat_entry->stat_unit.app_ip_set.app_prio = app_prio;
        list_add(&stat_entry->list, &l_stat_appid_list); /* re-add to the head */
		
		if(enable_print)
		{
			printk("APP_STAT: add to list, pkt_len = %uBytes, all bytes = %uBytes\r\n", pkt_len, stat_entry->all.bytes);
		}

        watch_entry = _search_watch_entry(client_ip, appid);
        if (watch_entry)
        {
            watch_tuple.protonum = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
            watch_tuple.src_ip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
            watch_tuple.src_port = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all;
            watch_tuple.dst_ip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip;
            watch_tuple.dst_port = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all;
#if 0
            printk("appid %d, tuple: src %pI4:%d, dst %pI4:%d  in.\r\n",
                           appid,
                           &(watch_tuple.src_ip),
                           watch_tuple.src_port,
                           &(watch_tuple.dst_ip),
                           watch_tuple.dst_port);
#endif

            tuple_entry = _search_tuple_entry(&watch_tuple, &watch_entry->tuple_list_head);
            if (!tuple_entry &&
                appid != APP_ID_HTTPFD)
            {
                tuple_entry = _add_new_watch_tuple(&watch_tuple, &watch_entry->tuple_list_head);
            }

            if (tuple_entry)
            {
                if (tuple_entry->warn.warn_flag)
                {
                    APPID_LOG(APPID_STAT, " #SPEED WARNNING# %pI4:%d => %pI4%d turn to low prio",
                              &(watch_tuple.src_ip),
                              watch_tuple.src_port,
                              &(watch_tuple.dst_ip),
                              watch_tuple.dst_port);
#if 0
                    printk("appid %d, tuple: src %pI4:%d, dst %pI4:%d turned to low prio.\r\n",
                           appid,
                           &(watch_tuple.src_ip),
                           watch_tuple.src_port,
                           &(watch_tuple.dst_ip),
                           watch_tuple.dst_port);

                    if (IPPROTO_UDP == watch_tuple.protonum)
                    {
                        APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_FOURTH_PRIO);
                        APP_NORMAL_ID_SET(appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, APPMNGR_APPID_NORMAL_END);
                        APP_NORMAL_ID_SET(appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, APPMNGR_APPID_NORMAL_END);
                    }
                    else if (IPPROTO_TCP == watch_tuple.protonum)
                    {
                        APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_THIRD_PRIO);
                        APP_NORMAL_ID_SET(appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, APP_ID_HTTPFD);
                        APP_NORMAL_ID_SET(appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, APP_ID_HTTPFD);
                    }
                    list_del(&tuple_entry->tuple_lru_node);
                    list_del(&tuple_entry->tuple_list_node);
                    appidentify_mempool_free_limit(tuple_entry, &l_tuple_list_pool);

                    watch_entry->free_flag = TRUE;
#endif
                }
                else
                {
                    tuple_entry->all.bytes += pkt_len;
                }
            }
        }

        spin_unlock_bh(&app_stat_lock);
    }
    else
    {
        printk("client ip equal lan ip.\r\n");
    }

    return NF_ACCEPT;
}

int appidentify_stat_set(app_stat_msg_t *msg, unsigned short opt)
{
    int ret = 0;

    if (!l_app_stat_inited)
    {
        APPIDNTFY_STAT_ERROR("stat set before stat init.\r\n");
        ret = -EINVAL;
        goto set_over;
    }

    switch (msg->statOpt)
    {
    case APP_STAT_OPT_LANINFO:
        lan_ip = msg->ip;
        lan_mask = msg->mask;
        APPID_LOG(APPID_STAT, "lan info set ip: %pI4, mask: %pI4", &lan_ip, &lan_mask);
		printk("APP STAT: lan info set ip: %pI4, mask: %pI4", &lan_ip, &lan_mask);
        break;
    default:
        APPID_ERR(APPID_STAT, "invalid stat config msg");
        ret = -EINVAL;
        goto set_over;
        break;
    }

set_over:

    return ret;
}

void appidentify_statistics_print(void)
{
    unsigned long long  total_all;
    unsigned long long  total_recent;
    STATCS_SET          *set_entry;
    APPIDNTFY_STATCS    *stat_entry;
    int                 index;
    short               app_prio = -1;
    unsigned long long  prio_total[APPPRIO_PRIO_TOP - 1] = {0};
    unsigned long long  prio_recent[APPPRIO_PRIO_TOP - 1] = {0};


    printk("***********************App Statistics***********************\r\n");
    printk("\r\n");
    printk("appid list:\r\n");
    printk("Appid    Priority    Speed(KBytes/s)    Total(KBytes)\r\n");
    spin_lock_bh(&app_stat_lock);
    list_for_each_entry(set_entry, &l_appid_list, list)
    {
        total_all = 0;
        total_recent = 0;
        if (set_entry->set.appid != INVALID_APPID)
        {
            list_for_each_entry(stat_entry, &l_stat_appid_list, list)
            {
                if (set_entry->set.appid == stat_entry->stat_unit.app_ip_set.appid)
                {
                    app_prio = stat_entry->stat_unit.app_ip_set.app_prio;
                    total_all += stat_entry->all.bytes;
                    total_recent += stat_entry->recent.bytes;
                }
            }
            printk("%-9d%-12d%-19llu%-9llu\r\n",
                   set_entry->set.appid,
                   (app_prio == -1) ? 0 : app_prio,
                   total_recent >> 11,
                   total_all >> 10);
        }
    }

    printk("\r\n");
    printk("client list:\r\n");
    printk("client_IP      Appid    Priority    Speed(KBytes/s)    Total(KBytes)\r\n");
    list_for_each_entry(set_entry, &l_devip_list, list)
    {
        printk("%pI4:\r\n", &(set_entry->set.ip4));

        list_for_each_entry(stat_entry, &l_stat_appid_list, list)
        {
            if (set_entry->set.ip4 == stat_entry->stat_unit.app_ip_set.ip4)
            {
                if (-1 == stat_entry->stat_unit.app_ip_set.appid)
                {
                    printk("               %-9s%-12d%-19u%-5u\r\n",
                       "other",
                       stat_entry->stat_unit.app_ip_set.app_prio,
                       stat_entry->recent.bytes >> 11,
                       stat_entry->all.bytes >> 10);
                }
                else
                {
                    printk("               %-9d%-12d%-19u%-5u\r\n",
                       stat_entry->stat_unit.app_ip_set.appid,
                       stat_entry->stat_unit.app_ip_set.app_prio,
                       stat_entry->recent.bytes >> 11,
                       stat_entry->all.bytes >> 10);
                }
            }
        }
    }

    printk("\r\n");
    printk("prio statistics:\r\n");
    printk("Priority    Speed(KBytes/s)    Total(KBytes)\r\n");
    list_for_each_entry(stat_entry, &l_stat_appid_list, list)
    {
        prio_total[stat_entry->stat_unit.app_ip_set.app_prio - 1] += stat_entry->all.bytes;
        prio_recent[stat_entry->stat_unit.app_ip_set.app_prio - 1] += stat_entry->recent.bytes;
    }

    for (index = 0; index < APPPRIO_PRIO_TOP - 1; index ++)
    {
        printk("%-12d%-19llu%llu\r\n",
               index + 1,
               prio_recent[index] >> 11,
               prio_total[index] >> 10);
    }
    spin_unlock_bh(&app_stat_lock);
    printk("************************************************************\r\n");
}

int appidentify_statistics_init(void)
{
    if (appidentify_mempool_init_limit("app_stat_pool",
                                   &l_app_stat_pool,
                                   sizeof(APPIDNTFY_STATCS),
                                   APPIDNTFY_STATCS_ENTRY_RESNUM,
                                   APPIDNTFY_STATCS_ENTRY_MAXNUM) < 0)
    {
        APPIDNTFY_STAT_ERROR("app statistics entry pool init error.\r\n");
        return -1;
    }

    if (appidentify_mempool_init_limit("stat_set_pool",
                                   &l_stat_set_pool,
                                   sizeof(STATCS_SET),
                                   APPIDNTFY_STATCS_SET_RESNUM,
                                   APPIDNTFY_STATCS_SET_MAXNUM) < 0)
    {
        APPIDNTFY_STAT_ERROR("app statistics set pool init error.\r\n");
        return -1;
    }

    /* for high speed conn watched */
    if (appidentify_mempool_init_limit("watch_list_pool",
                                   &l_watch_list_pool,
                                   sizeof(WATCH_APP_LIST),
                                   APPIDNTFY_WATCH_ENTRY_RESNUM,
                                   APPIDNTFY_WATCH_ENTRY_MAXNUM) < 0)
    {
        APPIDNTFY_STAT_ERROR("app watch entry pool init error.\r\n");
        return -1;
    }

    if (appidentify_mempool_init_limit("tuple_list_pool",
                                   &l_tuple_list_pool,
                                   sizeof(WATCH_TUPLE_LIST),
                                   APPIDNTFY_WATCH_TUPLE_RESNUM,
                                   APPIDNTFY_WATCH_TUPLE_MAXNUM) < 0)
    {
        APPIDNTFY_STAT_ERROR("app watch tuple pool init error.\r\n");
        return -1;
    }

    INIT_LIST_HEAD(&l_appid_list);
    INIT_LIST_HEAD(&l_devip_list);
    INIT_LIST_HEAD(&l_stat_appid_list);
    INIT_LIST_HEAD(&l_stat_mac_list);
    /* for high speed conn watched */
    INIT_LIST_HEAD(&l_app_watch_list);
    INIT_LIST_HEAD(&l_app_tuple_list);

    init_timer(&app_stat_timer);
    app_stat_timer.data     = (unsigned long)0;
    app_stat_timer.function = appidentify_stat_monitor;
    appidentify_stat_monitor(0);

    l_app_stat_inited = TRUE;
	
	printk("STAT INIT, lan info: %pI4, mask: %pI4", &lan_ip, &lan_mask);
	
    return 0;
}

int appidentify_statistics_exit(void)
{
    APPIDNTFY_STATCS    *stat_entry;
    APPIDNTFY_STATCS    *tmp_stat_entry;
    STATCS_SET          *set_entry;
    STATCS_SET          *tmp_set_entry;
    WATCH_APP_LIST      *watch_entry;
    WATCH_APP_LIST      *tmp_watch_entry;
    WATCH_TUPLE_LIST    *tuple_entry;
    WATCH_TUPLE_LIST    *tmp_tuple_entry;

    l_app_stat_inited = FALSE;
    spin_lock_bh(&app_stat_lock);

    list_for_each_entry_safe(set_entry, tmp_set_entry, &l_appid_list, list)
    {
        list_del(&set_entry->list);
        appidentify_mempool_free_limit(set_entry, &l_stat_set_pool);
    }

    list_for_each_entry_safe(set_entry, tmp_set_entry, &l_devip_list, list)
    {
        list_del(&set_entry->list);
        appidentify_mempool_free_limit(set_entry, &l_stat_set_pool);
    }

    list_for_each_entry_safe(stat_entry, tmp_stat_entry, &l_stat_appid_list, list)
    {
        list_del(&stat_entry->list);
        appidentify_mempool_free_limit(stat_entry, &l_app_stat_pool);
    }

    /* for high speed warning */
    list_for_each_entry_safe(tuple_entry, tmp_tuple_entry, &l_app_tuple_list, tuple_lru_node)
    {
        list_del(&tuple_entry->tuple_lru_node);
        appidentify_mempool_free_limit(tuple_entry, &l_tuple_list_pool);
    }

    list_for_each_entry_safe(watch_entry, tmp_watch_entry, &l_app_watch_list, watch_list_node)
    {
        list_del(&watch_entry->watch_list_node);
        appidentify_mempool_free_limit(watch_entry, &l_watch_list_pool);
    }

    /* mac statistic not included temporarily. */
#if 0
    list_for_each_entry_safe(stat_entry, tmp_stat_entry, &l_appid_list, list)
    {
        list_del(&stat_entry->list);
        appidentify_mempool_free_limit(stat_entry, &l_stat_set_pool);
    }
#endif

    del_timer(&app_stat_timer);

    spin_unlock_bh(&app_stat_lock);

    printk("statistics module exit ok!\n");
    return 0;
}

void appidentify_stat_clear(void)
{
    APPIDNTFY_STATCS    *stat_entry;

    APPID_LOG(APPID_STAT, "clear statistics data");
    spin_lock_bh(&app_stat_lock);

    list_for_each_entry(stat_entry, &l_stat_appid_list, list)
    {
        stat_entry->rx_cnt.bytes = 0;
        stat_entry->tx_cnt.bytes = 0;
    }

    spin_unlock_bh(&app_stat_lock);
}

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
