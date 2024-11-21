/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_dns.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     11Nov13
 *
 *\warning
 *
 *\history \arg 0.0.1, 12Nov13, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/types.h>
#include <linux/list.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include <net/ip.h>

#include "dnsparser/dnsparse_api.h"

#include "appidentify.h"
#include "appidentify_utils.h"
#include "appidentify_proxy.h"
#include "appidentify_dns.h"
#include "appidentify_log.h"

#include "appprio.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
unsigned int BKDRHash(char *str);

#define IP_HASH(ip)         (((ip >> 16) ^ ip) & (APPIDNTFY_DNS_IP_HASH_SCALE - 1))
#define DNS_HASH(dns)       (BKDRHash(dns) & (APPIDNTFY_DNS_HASH_SCALE - 1))

#define HTTP_PROTO_PORT     (80)
#define HTTPS_PROTO_PORT    (443)

#define APPDNS_CNT_DEBUG    (1)
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
extern int appidntf_debug;

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/
int
appidentify_cleanup_dnskw(void);
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
int appidntf_dns_debug                          = FALSE;
int l_isAppdnsInited                            = FALSE;
int g_enableDns                                 = TRUE;

static APPIDNTFY_MEMPOOL   l_dns_domain_pool;
static APPIDNTFY_MEMPOOL   l_dns_ip_pool;
struct hlist_head   *l_dns_hash                 = NULL;
struct hlist_head   *l_dns_ip_hash              = NULL;
struct list_head    l_lru_list_head;
APPMNGR_DNS         l_dns_keywords;
static LIST_HEAD(l_dnskw_list);
static unsigned int dnskw_cnt;
static DEFINE_SPINLOCK(dnskw_lock);

#if  APPDNS_CNT_DEBUG
unsigned int *l_dns_cnt_map = NULL;
#endif
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
unsigned int BKDRHash(char *str)
{
    unsigned int seed = 131; /* 31 131 1313 13131 131313 etc..*/
    unsigned int hash = 0;


    if(str == NULL)
    {
        return 0;
    }

    while (*str)
    {
        if(*str == '\r' || *str == '\n' || (*str == '.' && *(str + 1) == '\r'))
        {
            break;
        }

        hash = hash * seed + (*str++);
    }

    return ((hash >> 13)^(hash));
}

int _appidentify_dns_entry_init(APPIDNTFY_DNS *entry)
{
    if (NULL == entry)
    {
        return -1;
    }
    memset(entry, 0, sizeof(APPIDNTFY_DNS));
    INIT_HLIST_NODE(&entry->hnode);
    INIT_LIST_HEAD(&entry->lnode);
    INIT_LIST_HEAD(&entry->iplist);
    return 0;
}

int _appidentify_ip_hash_add(APPIDNTFY_DNS_IP *entry)
{
    unsigned int hash_key = 0;
    struct hlist_head *hlhead = NULL;

    if (NULL == entry)
    {
        return -1;
    }
    hash_key = IP_HASH(entry->ip4addr);
    hlhead = &l_dns_ip_hash[hash_key];

    hlist_add_head(&entry->hnode, hlhead);

    return 0;
}

int _appidentify_ip_hash_del(APPIDNTFY_DNS_IP *entry)
{
    if (NULL == entry)
    {
        return -1;
    }
    hlist_del(&entry->hnode);

    return 0;
}

int _appidentify_dns_hash_add(APPIDNTFY_DNS *dns_entry)
{
    unsigned int dns_hashkey = 0;
    struct hlist_head *hlhead = NULL;

    if (NULL == dns_entry)
    {
        return -1;
    }

    dns_hashkey = DNS_HASH(dns_entry->domain);
    hlhead = &l_dns_hash[dns_hashkey];

    hlist_add_head(&dns_entry->hnode, hlhead);

    return 0;
}

APPIDNTFY_DNS* _appidentify_dns_hash_get(char *domain)
{
    unsigned int dns_hashkey = 0;
    struct hlist_head *hlhead = NULL;
    struct hlist_node *hlnode = NULL;
    struct hlist_node *tmp_hlnode = NULL;
    APPIDNTFY_DNS *dns_entry = NULL;

    if (NULL == domain)
    {
        return NULL;
    }
    dns_hashkey = DNS_HASH(domain);
    hlhead = &l_dns_hash[dns_hashkey];

    hlist_for_each_safe(hlnode, tmp_hlnode, hlhead)
    {
        dns_entry = hlist_entry(hlnode, APPIDNTFY_DNS, hnode);
        if (!strncmp(domain, dns_entry->domain, strlen(dns_entry->domain)))
        {
            return dns_entry;
        }
    }

    return NULL;
}
/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
void appidentify_dns_printall(void)
{
    struct list_head    *node;
    APPIDNTFY_DNS       *dns_entry;
    APPIDNTFY_DNS_IP    *ip_entry;
    APPIDNTFY_DNS_IP    *_next_ip_entry;
    int     dns_count = 0;
    int     ip_count = 0;

    printk("dns cache list:\r\n");
    list_for_each(node, &l_lru_list_head)
    {
        dns_entry = list_entry(node, APPIDNTFY_DNS, lnode);
        dns_count++;
        printk("\r\n    domain: %s\r\n", dns_entry->domain);
        printk("    appid: %d\r\n", dns_entry->appid);
        printk("    ip addr count: %d\r\n", dns_entry->ipnum);

        list_for_each_entry_safe(ip_entry, _next_ip_entry, &(dns_entry->iplist), lnode)
        {
            printk("        ip addr: %pI4\r\n", &ip_entry->ip4addr);
            ip_count++;
        }
    }
    printk("\r\ntotal domain:%d, total ip addr:%d, mempool:%d/%d.\r\n",
           dns_count, ip_count, l_dns_domain_pool.used, l_dns_domain_pool.maximum);
}

int appidentify_dns_init(void)
{
    if (appidentify_mempool_init_limit("domain_pool",
                                       &l_dns_domain_pool,
                                       sizeof(APPIDNTFY_DNS),
                                       APPIDNTFY_DNS_RESERVED_NUM,
                                       APPIDNTFY_DNS_MAX_NUM) < 0)
    {
        APPIDNTFY_DNS_ERROR("l_dns_domain_pool init error.\r\n");
        return -1;
    }

    if (appidentify_mempool_init_limit("dns_ip_pool",
                                       &l_dns_ip_pool,
                                       sizeof(APPIDNTFY_DNS_IP),
                                       APPIDNTFY_IP_RESERVED_NUM,
                                       APPIDNTFY_IP_MAX_NUM) < 0)
    {
        APPIDNTFY_DNS_ERROR("l_dns_ip_pool init error.\r\n");
        return -1;
    }

    l_dns_hash = (struct hlist_head *)kmalloc(APPIDNTFY_DNS_HASH_SCALE * sizeof(struct hlist_head), GFP_KERNEL);
    if (NULL == l_dns_hash)
    {
        APPIDNTFY_DNS_ERROR("domain hash kmalloc error.\r\n");
        return -1;
    }
    memset(l_dns_hash, 0, APPIDNTFY_DNS_HASH_SCALE * sizeof(struct hlist_head));

    l_dns_ip_hash = (struct hlist_head *)kmalloc(APPIDNTFY_DNS_IP_HASH_SCALE * sizeof(struct hlist_head), GFP_KERNEL);
    if (NULL == l_dns_ip_hash)
    {
        APPIDNTFY_DNS_ERROR("ip hash kmalloc error.\r\n");
        return -1;
    }
    memset(l_dns_ip_hash, 0, APPIDNTFY_DNS_IP_HASH_SCALE * sizeof(struct hlist_head));

    if (DNSPARSE_SUCCESS != dnsparse_mempool_init())
    {
        APPIDNTFY_DNS_ERROR("dnsparser mempool init error.\r\n");
        return -1;
    }

    INIT_LIST_HEAD(&l_lru_list_head);

#if  APPDNS_CNT_DEBUG
    l_dns_cnt_map = (unsigned int *)vmalloc(4096 * sizeof(unsigned int));
    memset(l_dns_cnt_map, 0, 4096 * sizeof(unsigned int));
#endif

    /* dns keywords list init */
    //memset(&l_dns_keywords, 0, sizeof(APPMNGR_DNS));

    /* init dns keywords list */
    dnskw_cnt = 0;
    INIT_LIST_HEAD(&l_dnskw_list);

    l_isAppdnsInited = TRUE;

    return 0;
}

int appidentify_dns_exit(void)
{

    l_isAppdnsInited = FALSE;

    if (l_dns_domain_pool.mempool)
    {
        mempool_destroy(l_dns_domain_pool.mempool);
    }

    if (l_dns_hash)
    {
        kfree(l_dns_hash);
    }

    if (l_dns_hash)
    {
        kfree(l_dns_ip_hash);
    }

    if (DNSPARSE_SUCCESS != dnsparse_mempool_destroy())
    {
        APPIDNTFY_DNS_ERROR("dnsparser mempool destroy error.\r\n");
        return -1;
    }

    appidentify_cleanup_dnskw();

#if  APPDNS_CNT_DEBUG
    vfree(l_dns_cnt_map);
#endif

    printk("appidentify dns exit ok\n");

    return 0;
}

int appidnetify_dns_ip_add(APPIDNTFY_DNS *dns_entry, DNSPARSE_DNS_MESSAGE *msg)
{
    struct list_head    *an_lhead = NULL;
    struct list_head    *an_lnode = NULL;
    DNSPARSE_RR_TYPE    *rr_entry = NULL;
    APPIDNTFY_DNS_IP    *ip_elem  = NULL;

    if (NULL == dns_entry || NULL == msg)
    {
        return -1;
    }

    if (msg->counts[DNSPARSE_DNS_SECTION_ANSWER] != 0)
    {
        an_lhead = &msg->sections[DNSPARSE_DNS_SECTION_ANSWER];
        list_for_each(an_lnode, an_lhead)
        {
            rr_entry = list_entry(an_lnode, DNSPARSE_RR_TYPE, listNode);
            if (DNSPARSE_DNS_RR_TYPE_A == rr_entry->type &&
                NULL != rr_entry->pRData)
            {
                /* loop overflow for finding a new ip entry */
                if (dns_entry->ipnum < APPIDNTFY_DNS_IP_NUM)
                {
                    ip_elem = (APPIDNTFY_DNS_IP *)appidentify_mempool_alloc_limit(&l_dns_ip_pool, GFP_ATOMIC);
                    if (!ip_elem)
                    {
                        APPIDNTFY_DNS_ERROR("ip_elem alloc error.\r\n");
                        return -1;
                    }
                }
                else
                {
                    ip_elem = list_entry(dns_entry->iplist.next, APPIDNTFY_DNS_IP, lnode);
                    list_del(&ip_elem->lnode);
                    dns_entry->ipnum --;
                    if (ip_elem && _appidentify_ip_hash_del(ip_elem) < 0)
                    {
                        APPIDNTFY_DNS_ERROR("_appidentify_ip_hash_del error.\r\n");
                        return -1;
                    }
                }

                memset(ip_elem, 0, sizeof(APPIDNTFY_DNS_IP));
                ip_elem->ip4addr = *((unsigned int *)rr_entry->pRData);
                ip_elem->appid = dns_entry->appid;

                if (_appidentify_ip_hash_add(ip_elem) < 0)
                {
                    APPIDNTFY_DNS_ERROR("_appidentify_ip_hash_add error.\r\n");
                    return -1;
                }
                /* add to dns_entry -> lru */
                list_add_tail(&ip_elem->lnode, &dns_entry->iplist);
                dns_entry->ipnum ++;
            }
        }
    }

    return 0;
}

int appidentify_dns_node_del(APPIDNTFY_DNS *dns_entry)
{
    APPIDNTFY_DNS_IP  *ip_entry = NULL;
    APPIDNTFY_DNS_IP  *_next_ip_entry = NULL;

    if (NULL == dns_entry)
    {
        APPIDNTFY_DNS_ERROR("param null.\r\n");
        return -1;
    }

    list_for_each_entry_safe(ip_entry, _next_ip_entry, &(dns_entry->iplist), lnode)
    {
        if (!hlist_unhashed(&ip_entry->hnode))
        {
            hlist_del(&ip_entry->hnode);
        }
        list_del(&ip_entry->lnode);
        appidentify_mempool_free_limit((void *)ip_entry, &l_dns_ip_pool);
    }

    /* take off from dns entry hash table */
    hlist_del(&dns_entry->hnode);
    /* take off from the LRU list */
    list_del(&dns_entry->lnode);

    return 0;
}

APPIDNTFY_DNS* appidentify_dns_node_alloc(void)
{
    APPIDNTFY_DNS *dns_entry = NULL;

    dns_entry = (APPIDNTFY_DNS *)appidentify_mempool_alloc_limit(&l_dns_domain_pool, GFP_ATOMIC);
    if (!dns_entry && !list_empty(&l_lru_list_head))
    {
        dns_entry = list_entry(l_lru_list_head.next, APPIDNTFY_DNS, lnode);
        if (appidentify_dns_node_del(dns_entry) < 0)
        {
            APPIDNTFY_DNS_ERROR("appidentify_dns_node_del error.\r\n");
            return NULL;
        }
    }

    return dns_entry;
}

void appidentify_dns_node_free(APPIDNTFY_DNS *dns_entry)
{
    appidentify_mempool_free_limit((void *)dns_entry, &l_dns_domain_pool);
}

int appidentify_dns_query_keywords(APPIDNTFY_DNS *dns_entry)
{
//    int index;
    APPID_DNS_RULE *dns_rule = NULL;

    if (NULL == dns_entry)
    {
        return -1;
    }
    if (!l_isAppdnsInited)
    {
        return -1;
    }

#if 0
    if (!l_dns_keywords.rule)
    {
        return -1;
    }

    dns_rule = l_dns_keywords.rule;
    for (index = 0; index < l_dns_keywords.appDnsCnt; index++)
    {
        if (appidentify_bms_str_search(dns_rule[index].dnsKw.keyword,
                                       strlen(dns_rule[index].dnsKw.keyword),
                                       dns_entry->domain,
                                       strlen(dns_entry->domain),
                                       TRUE))
        {
            dns_entry->appid = dns_rule[index].appId;
            return 0;
        }
    }
#endif

    //spin_lock_bh(&dnskw_lock);
    list_for_each_entry(dns_rule, &l_dnskw_list, list)
    {
        if (appidentify_bms_str_search(dns_rule->dnsKw.keyword,
                                       strlen(dns_rule->dnsKw.keyword),
                                       dns_entry->domain,
                                       strlen(dns_entry->domain),
                                       TRUE))
        {
            dns_entry->appid = dns_rule->appId;
            printk("find a dnskw!\n");
            return 0;
        }
    }
    //spin_unlock_bh(&dnskw_lock);

    return -1;
}

int appidentify_dns_node_add(DNSPARSE_DNS_MESSAGE *msg)
{
    APPIDNTFY_DNS       *dns_entry = NULL;
    DNSPARSE_RR_TYPE    *rr_entry = NULL;
    struct list_head    *qu_lhead = NULL;

    if (msg->counts[DNSPARSE_DNS_SECTION_QUESTION] != 0)
    {
        qu_lhead = &msg->sections[DNSPARSE_DNS_SECTION_QUESTION];
        if (list_empty(qu_lhead))
        {
            APPIDNTFY_DNS_ERROR("dns question name null.\r\n");
            return -1;
        }
        APPIDNTFY_DNS_DEBUG("\r\n");
        dns_entry = appidentify_dns_node_alloc();
        if (!dns_entry)
        {
            APPIDNTFY_DNS_ERROR("appidentify_dns_node_alloc error.\r\n");
            return -1;
        }

        if(_appidentify_dns_entry_init(dns_entry) < 0)
        {
            APPIDNTFY_DNS_ERROR("entry init error.\r\n");
            return -1;
        }

        rr_entry = (DNSPARSE_RR_TYPE *)(qu_lhead->next);
        strcpy(dns_entry->domain, (char *)rr_entry->name);

        /* need a database query for dns keywords. */
        if(!appidentify_dns_query_keywords(dns_entry))
        {
            APPIDNTFY_DNS_DEBUG("domain %s, the keywords found.\r\n", dns_entry->domain);
        }

        if (appidnetify_dns_ip_add(dns_entry, msg) < 0)
        {
            APPIDNTFY_DNS_ERROR("appidnetify_dns_ip_add error.\r\n");
            return -1;
        }

        if (_appidentify_dns_hash_add(dns_entry) < 0)
        {
            APPIDNTFY_DNS_ERROR("_appidentify_dns_hash_add error.\r\n");
            return -1;
        }
        list_add_tail(&dns_entry->lnode, &l_lru_list_head);
    }
    else
    {
        return -1;
    }

    return 0;
}

int appidentify_dns_node_update(APPIDNTFY_DNS *dns_entry, DNSPARSE_DNS_MESSAGE *msg)
{
    struct list_head    *an_lhead = NULL;
    DNSPARSE_RR_TYPE    *rr_entry;
    unsigned int        ipaddr;
    APPIDNTFY_DNS_IP    *ip_entry = NULL;

    if (NULL == msg)
    {
        return -1;
    }

    an_lhead = &msg->sections[DNSPARSE_DNS_SECTION_ANSWER];

    list_for_each_entry(rr_entry, an_lhead, listNode)
    {
        if (DNSPARSE_DNS_RR_TYPE_A == rr_entry->type)
        {

            ipaddr = *((unsigned int *)rr_entry->pRData);

            list_for_each_entry(ip_entry, &dns_entry->iplist, lnode)
            {
                if (ip_entry->ip4addr == ipaddr)
                {
                    /* update the ip lru list */
                    list_del(&ip_entry->lnode);
                    list_add_tail(&ip_entry->lnode, &dns_entry->iplist);
                    goto IP_UPDATE_OVER;
                }
            }

            if (APPIDNTFY_DNS_IP_NUM <= dns_entry->ipnum)
            {
                ip_entry = list_entry(dns_entry->iplist.next, APPIDNTFY_DNS_IP, lnode);
                if (_appidentify_ip_hash_del(ip_entry) < 0)
                {
                    APPIDNTFY_DNS_ERROR("_appidentify_ip_hash_del error.\r\n");
                    return -1;
                }
                list_del(&ip_entry->lnode);
                dns_entry->ipnum--;
            }
            else
            {
                ip_entry = (APPIDNTFY_DNS_IP *)appidentify_mempool_alloc_limit(&l_dns_ip_pool, GFP_ATOMIC);
                if (!ip_entry)
                {
                    APPIDNTFY_DNS_ERROR("ip_elem alloc error.\r\n");
                    return -1;
                }
            }

            memset(ip_entry, 0, sizeof(APPIDNTFY_DNS_IP));
            ip_entry->ip4addr = ipaddr;
            ip_entry->appid = dns_entry->appid;
            if (_appidentify_ip_hash_add(ip_entry) < 0)
            {
                APPIDNTFY_DNS_ERROR("_appidentify_ip_hash_add error.\r\n");
                return -1;
            }
            /* add to dns_entry -> lru */
            list_add_tail(&ip_entry->lnode, &dns_entry->iplist);
            dns_entry->ipnum ++;
        }
    }

IP_UPDATE_OVER:
    /* update the dns LRU list */
    list_del(&dns_entry->lnode);
    list_add_tail(&dns_entry->lnode, &l_lru_list_head);

    return 0;
}

void appidentify_dns_testentry_add(char *domain, unsigned int ipaddr, short appid)
{
    APPIDNTFY_DNS       *dns_entry;
    APPIDNTFY_DNS_IP    *ip_entry;

    if (NULL == domain)
    {
        APPIDNTFY_DNS_ERROR("domain null.\r\n");
        return;
    }

    dns_entry = _appidentify_dns_hash_get(domain);
    if (!dns_entry)
    {
        dns_entry = appidentify_dns_node_alloc();
        if (_appidentify_dns_entry_init(dns_entry) < 0)
        {
            APPIDNTFY_DNS_ERROR("_appidentify_dns_entry_init error.\r\n");
            return;
        }
        strcpy(dns_entry->domain, domain);
        dns_entry->appid = appid;
        if (_appidentify_dns_hash_add(dns_entry) < 0)
        {
            appidentify_dns_node_free(dns_entry);
            APPIDNTFY_DNS_ERROR("_appidentify_dns_hash_add error.\r\n");
            return;
        }

        list_add_tail(&dns_entry->lnode, &l_lru_list_head);

        ip_entry = (APPIDNTFY_DNS_IP *)appidentify_mempool_alloc_limit(&l_dns_ip_pool, GFP_ATOMIC);
        if (!ip_entry)
        {
            APPIDNTFY_DNS_ERROR("ip_elem alloc error.\r\n");
            return;
        }
        ip_entry->ip4addr = ipaddr;
        ip_entry->appid   = appid;
        dns_entry->ipnum  = 1;
        if (_appidentify_ip_hash_add(ip_entry) < 0)
        {
            APPIDNTFY_DNS_ERROR("_appidentify_ip_hash_add error.\r\n");
            return;
        }
    }
    else
    {
        ip_entry = (APPIDNTFY_DNS_IP *)appidentify_mempool_alloc_limit(&l_dns_ip_pool, GFP_ATOMIC);
        if (!ip_entry)
        {
            APPIDNTFY_DNS_ERROR("ip_elem alloc error.\r\n");
            return;
        }
        ip_entry->ip4addr = ipaddr;
        ip_entry->appid   = appid;
        if (_appidentify_ip_hash_add(ip_entry) < 0)
        {
            APPIDNTFY_DNS_ERROR("_appidentify_ip_hash_add error.\r\n");
            return;
        }
        dns_entry->ipnum ++;
    }

    /* update LRU list */
    list_del(&dns_entry->lnode);
    list_add_tail(&dns_entry->lnode, &l_lru_list_head);
}

int appidentify_dns_dispose(DNSPARSE_DNS_MESSAGE *msg)
{
    struct list_head    *qu_list;
    DNSPARSE_RR_TYPE    *rr_entry;
    APPIDNTFY_DNS       *dns_entry;

    if (NULL == msg)
    {
        return -1;
    }

    if (msg->counts[DNSPARSE_DNS_SECTION_QUESTION] != 0)
    {
        qu_list = &msg->sections[DNSPARSE_DNS_SECTION_QUESTION];
        if (!list_empty(qu_list))
        {
            rr_entry = (DNSPARSE_RR_TYPE *)qu_list->next;
            dns_entry = _appidentify_dns_hash_get(rr_entry->name);

            if (dns_entry) /* find a dns entry */
            {
                if (appidentify_dns_node_update(dns_entry, msg) < 0)
                {
                    APPIDNTFY_DNS_ERROR("appidentify_dns_node_add error.\r\n");
                    return -1;
                }
            }
            else /* new dns entry */
            {
                if (appidentify_dns_node_add(msg) < 0)
                {
                    APPIDNTFY_DNS_ERROR("appidentify_dns_node_add error.\r\n");
                    return -1;
                }
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

int appidentify_dns_check(unsigned int ipaddr, unsigned short *appid)
{
    struct hlist_head *ip_list;
    struct hlist_node *ip_node;
    APPIDNTFY_DNS_IP  *ip_entry = NULL;
    unsigned int hash_key;

    if (NULL == appid)
    {
        return FALSE;
    }
    hash_key = IP_HASH(ipaddr);
    ip_list = &l_dns_ip_hash[hash_key];
    APPIDNTFY_DNS_DEBUG("hash key %d, %p, %pI4\r\n", hash_key, ip_list, &ipaddr);
    hlist_for_each(ip_node, ip_list)
    {
        ip_entry = list_entry(ip_node, APPIDNTFY_DNS_IP, hnode);
        if (ipaddr == ip_entry->ip4addr)
        {
            *appid = ip_entry->appid;
#if  APPDNS_CNT_DEBUG
            l_dns_cnt_map[*appid] ++;
#endif
            return TRUE;
        }
    }

    *appid = 0;
    return FALSE;
}

unsigned int appidentify_dns_hook(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
    int                     datalen;
    struct iphdr            *iph;
    struct udphdr           *udph;
    unsigned char           *data_ptr;
    struct nf_conn          *ct;
    enum ip_conntrack_info  ctinfo;
    unsigned char           pktdir = IP_CT_DIR_ORIGINAL;
    DNSPARSE_DNS_MESSAGE    dnsmsg;
    unsigned int            srcip, dstip;
    unsigned short          srcport, dstport;
    unsigned short          ori_appid, rep_appid = 0, appid;
    short                   *ori_flag;
    short                   *rep_flag;
    int                     ret;
    struct nf_conntrack_app *appinfo;

    if (!l_isAppdnsInited)
    {
        return NF_ACCEPT;
    }

    if (!g_enableDns)
    {
        return NF_ACCEPT;
    }

    iph = ip_hdr(skb);
    if (NULL == iph)
    {
        return NF_ACCEPT;
    }

    ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct)
    {
        return NF_ACCEPT;
    }

    appinfo = nf_ct_get_app(ct);
    if (NULL == appinfo)
    {
        APPIDNTFY_DNS_ERROR("no appidntf_info.\r\n");
        return NF_ACCEPT;
    }

    if (APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_DNS)  ||
        APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_UPNP) ||
        APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_CLSF) ||
        APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_DPI))
    {
        return NF_ACCEPT;
    }

	//printk("DNS hook: in dns hook\r\n");

    pktdir = CTINFO2DIR(ctinfo);
    data_ptr = (unsigned char *)iph;

    srcip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
    srcport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all;
    dstip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip;
    dstport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all;

    ori_flag = &(appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index);
    rep_flag = &(appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index);

    if (iph->protocol == IPPROTO_TCP)
    {
        if (TRUE == appidentify_dns_check(srcip, &ori_appid) ||
            TRUE == appidentify_dns_check(dstip, &rep_appid))
        {
            appid = ori_appid > rep_appid ? ori_appid : rep_appid;
            if (appid)
            {
                APP_NORMAL_ID_SET(*ori_flag, appid);
                APP_NORMAL_ID_SET(*rep_flag, appid);
				printk("DNS hook: in dns hook, appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);
            }
            if (HTTP_PROTO_PORT == srcport ||
                HTTP_PROTO_PORT == dstport)
            {
                APP_BASIC_ID_VALUE_SET(*ori_flag, APP_ID_HTTP);
                APP_BASIC_ID_VALUE_SET(*rep_flag, APP_ID_HTTP);
				printk("DNS hook: in dns hook, appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);
            }
            else if (HTTPS_PROTO_PORT == srcport ||
                     HTTPS_PROTO_PORT == dstport)
            {
                APP_BASIC_ID_VALUE_SET(*ori_flag, APP_ID_HTTPS);
                APP_BASIC_ID_VALUE_SET(*rep_flag, APP_ID_HTTPS);
				printk("DNS hook: appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);
            }
        }
    }
    else if (iph->protocol == IPPROTO_UDP)
    {
        udph = (struct udphdr *)(data_ptr + ip_hdrlen(skb));

        if (pktdir == IP_CT_DIR_REPLY &&
            udph->source == DNS_SVR_PORT)
        {
            if (appidentify_locate_payload(skb, &data_ptr, &datalen) < 0)
            {
                return NF_ACCEPT;
            }
            dnsparse_message_init(&dnsmsg);
            ret = dnsparse_message_parse(&dnsmsg, data_ptr, datalen);
            if(ret != DNSPARSE_SUCCESS)
            {
                dnsparse_message_destroy(&dnsmsg);
                return NF_ACCEPT;
            }
            /*
            dnsparse_test_printQuestion(&dnsmsg);
            dnsparse_test_printSection(&dnsmsg, DNSPARSE_DNS_SECTION_ANSWER);
            */
            appidentify_dns_dispose(&dnsmsg);
            dnsparse_message_destroy(&dnsmsg);

            APP_NORMAL_ID_SET(*ori_flag, APP_ID_DNS);
            APP_NORMAL_ID_SET(*rep_flag, APP_ID_DNS);
			APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_FIRST_PRIO); /* ??? @WANG */
            //APPPRIO_PRI_SET(*ori_flag, APPPRIO_FIRST_PRIO);
            //APPPRIO_PRI_SET(*rep_flag, APPPRIO_FIRST_PRIO);
			printk("DNS hook: appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);

            return NF_ACCEPT;
        }
        else if (udph->source == DNS_SVR_PORT ||
                 udph->dest == DNS_SVR_PORT)
        {
            APP_NORMAL_ID_SET(*ori_flag, APP_ID_DNS);
            APP_NORMAL_ID_SET(*rep_flag, APP_ID_DNS);
			APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_FIRST_PRIO); /* ??? @WANG */
            //APPPRIO_PRI_SET(*ori_flag, APPPRIO_FIRST_PRIO);
            //APPPRIO_PRI_SET(*rep_flag, APPPRIO_FIRST_PRIO);
			printk("DNS hook: appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);

            return NF_ACCEPT;
        }

        if (TRUE == appidentify_dns_check(srcip, &ori_appid) ||
            TRUE == appidentify_dns_check(dstip, &rep_appid))
        {
            appid = ori_appid > rep_appid ? ori_appid : rep_appid;
            if (appid)
            {
                APP_NORMAL_ID_SET(*ori_flag, appid);
                APP_NORMAL_ID_SET(*rep_flag, appid);
				printk("DNS hook: appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);
            }
        }
    }

	//printk("APPID: in dns hook, appid_ori = %d, appid_rep = %d\r\n", *ori_flag, *rep_flag);

    APPPRIO_FLAG_SET(appinfo->appprio_flag, APPPRIO_FLAG_DNS);
    return NF_ACCEPT;
}

void
appidentify_dnskw_printall(void)
{
    int index;
	APPID_DNS_RULE    *rule;

#if 1
    printk("----------------dns keywords------------------\r\n");
    for (index = 0; index < l_dns_keywords.appDnsCnt; index ++)
    {
        printk("appId %d, dns keyword %s\r\n",
               l_dns_keywords.rule[index].appId,
               l_dns_keywords.rule[index].dnsKw.keyword);
    }
#if  APPDNS_CNT_DEBUG
    printk("----------------dns kw conn cnt---------------\r\n");
    for(index = 0; index < 4096; index++)
    {
        if (l_dns_cnt_map[index] != 0)
        {
            printk("APP_ID %-4u, conn cnt %u\r\n", index, l_dns_cnt_map[index]);
        }
    }
#endif
#endif



    spin_lock_bh(&dnskw_lock);
    list_for_each_entry(rule, &l_dnskw_list, list)
    {
        printk("appId %d, dns keyword %s\r\n",
               rule->appId, rule->dnsKw.keyword);
    }
    spin_unlock_bh(&dnskw_lock);
}


int
appidentify_cleanup_dnskw(void)
{
    APPID_DNS_RULE    *rule;
    APPID_DNS_RULE    *next;

#if 0
    if (l_dns_keywords.rule)
    {
        vfree(l_dns_keywords.rule);
        l_dns_keywords.rule = NULL;
    }
#endif
    spin_lock_bh(&dnskw_lock);
    dnskw_cnt = 0;
    list_for_each_entry_safe(rule, next, &l_dnskw_list, list)
    {
        list_del(&rule->list);
        kfree(rule);
    }
    spin_unlock_bh(&dnskw_lock);
    return 0;
}

int appidentify_add_dnskw(char *domain, unsigned int appId)
{
    APPID_DNS_RULE     *rule;

    APPID_LOG(APPID_DNS, "add new dnskw: %s, appid: %d", domain, appId);

    rule = (APPID_DNS_RULE*)kmalloc(sizeof(APPID_DNS_RULE), GFP_KERNEL);
    if (NULL == rule)
    {
        APPID_ERR(APPID_DNS, "malloc dnskw failed");
        return -1;
    }

    rule->appId = appId;
    strncpy(rule->dnsKw.keyword, domain, APPMNGR_DNS_KEYWORD_LEN);
    spin_lock_bh(&dnskw_lock);
    dnskw_cnt++;
    list_add(&rule->list, &l_dnskw_list);
    spin_unlock_bh(&dnskw_lock);

	return 0;
}


int appidentify_add_new_dnskw(APPMNGR_DNS_RULE *rule, int ruleNum)
{
#if 0
    if (!l_dns_keywords.rule)
    {
        l_dns_keywords.rule = (APPMNGR_DNS_RULE *)vmalloc(ruleNum * sizeof(APPMNGR_DNS_RULE));
        if (!l_dns_keywords.rule)
        {
            APPIDNTFY_DNS_ERROR("l_dns_keywords.rule mem alloc error.\n");
            return -1;
        }

        l_dns_keywords.appDnsCnt = ruleNum;
        memset(l_dns_keywords.rule, 0, ruleNum * sizeof(APPMNGR_DNS_RULE));

        memcpy((void *)l_dns_keywords.rule, (void *)rule, ruleNum * sizeof(APPMNGR_DNS_RULE));
    }
#endif

    int i;
    APPID_DNS_RULE    *newRule;

    spin_lock_bh(&dnskw_lock);
    for (i = 0; i < ruleNum; ++i)
    {
        newRule = (APPID_DNS_RULE *)kmalloc(sizeof(APPID_DNS_RULE), GFP_KERNEL);
        if (NULL == newRule)
        {
            APPID_ERR(APPID_DNS, "malloc dnskw rule failed");
            break;
        }
        newRule->appId = rule[i].appId;
        strncpy(newRule->dnsKw.keyword, rule[i].dnsKw.keyword, APPMNGR_DNS_KEYWORD_LEN);
        list_add(&newRule->list, &l_dnskw_list);
    }
    dnskw_cnt = ruleNum;
    spin_unlock_bh(&dnskw_lock);

	if (1 == appidntf_debug) {
		appidentify_dnskw_printall();
	}

    return 0;
}

int
appidentify_dnskw_update(void * pRuleBuf, int ruleNum, int len)
{
    int ret;
    APPMNGR_DNS_RULE *pAppRule = (APPMNGR_DNS_RULE *)pRuleBuf;

    if (len < sizeof(APPMNGR_DNS_RULE) * ruleNum)
    {
        APPIDNTFY_DNS_ERROR("the msg recv is too short\n");
        return -1;
    }

    l_isAppdnsInited = FALSE;

    if ((ret = appidentify_cleanup_dnskw()))
    {
        APPIDNTFY_DNS_ERROR("appidentify_cleanup_rules error.\r\n");
        return ret;
    }

    if ((ret = appidentify_add_new_dnskw(pAppRule, ruleNum)))
    {
        APPIDNTFY_DNS_ERROR("appidentify_add_new_rule error.\r\n");
        return ret;
    }

    l_isAppdnsInited = TRUE;
    return 0;
}
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
