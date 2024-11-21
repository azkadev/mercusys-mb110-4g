/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_hardcode.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     03Dec13
 *
 *\warning
 *
 *\history \arg 0.0.1, 03Dec13, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_app.h>
#include <linux/list.h>
#include <linux/vmalloc.h>

#include "appidentify.h"
#include "appidentify_rules.h"
#include "appidentify_utils.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

int app_hard_debug = 1;
#define     APPIDNTFY_HARDCODE_DEBUG(fmt, args...)      \
            do                                          \
            {                                           \
                if (app_hard_debug)                     \
                {                                       \
                    printk("HC_DEBUG[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args);    \
                }                                       \
            }while(0)

#define     APPIDNTFY_HARDCODE_ERROR(fmt, args...)      \
                printk("HC_ERROR[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args)

/**
 * 16位大小端转换
 */
#ifdef IP_LITTLE_ENDIAN
#define APPDIST_GET_16ON8(xxaddr) (xxaddr)
#define APPDIST_GET_32ON8(xxaddr) (xxaddr)
#else
#define APPDIST_GET_16ON8(xxaddr) (((unsigned short)((xxaddr) << 8) & 0xFF00 ) | ((unsigned short)((xxaddr) >> 8) & 0x00FF))
#define APPDIST_GET_32ON8(xxaddr) ((unsigned int)APPDIST_GET_16ON8((unsigned short)((xxaddr) >> 16)) | \
                                  ((unsigned int)APPDIST_GET_16ON8((unsigned short)((xxaddr) & 0xFFFF)) << 16))
#endif

#define         APPIDNTFY_SKYPE_HASH_SCALE          (4096)
#define         APPIDNTFY_SKYPE_MIN_RESERVED        (8)
#define         APPIDNTFY_SKYPE_MAX_LIMITED         (128)

#define SKYPE_HASH(ip, port)    ((((ip >> 16) ^ ip) ^ port) & (APPIDNTFY_SKYPE_HASH_SCALE - 1))
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct _SKYPE_ID_TUPLE
{
    unsigned char         idh;
    unsigned char         idhCnt;
}SKYPE_ID_TUPLE;

typedef struct _SKYPE_CONN
{
    struct hlist_node     hlist;
    struct list_head      lruList;
    unsigned int          keyIp;
    unsigned int          keyPort;
    SKYPE_ID_TUPLE        idFlag[IP_CT_DIR_MAX];
} SKYPE_CONN;
/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/
static int _appidentify_qqlive_udp(struct sk_buff *, struct nf_conntrack_app *,
                           void *, unsigned char *, unsigned int);

static int _appidentify_ppstream_udp(struct sk_buff *, struct nf_conntrack_app *,
                           void *, unsigned char *, unsigned int);

static int _appidentify_skype_udp(struct sk_buff *, struct nf_conntrack_app *,
                           void *, unsigned char *, unsigned int);
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
struct hlist_head   *l_skype_hash                   = NULL;
APPIDNTFY_MEMPOOL   l_skype_pool;
struct list_head    l_skype_lru;


/* 硬编码应用列表 */
FP_HANDLE_INFO appidentify_hardCodeHandles[] =
{
    /* QQLive UDP 8000 */
    {
        APP_ID_QQLIVE,
        {0, 0, 0, 8000},
        IPPROTO_UDP,
        {_appidentify_qqlive_udp, NULL}
    },
    /* PPS UDP 任意端口 */
    {
        APP_ID_PPSTREAM,
        {0, 0, 0, 0},
        IPPROTO_UDP,
        {_appidentify_ppstream_udp, NULL}
    },
    /* Skype UDP 任意端口 */
    {
        APP_ID_SKYPE,
        {0, 0, 0, 0},
        IPPROTO_UDP,
        {_appidentify_skype_udp, _appidentify_skype_udp}
    },
    /* NULL */
    {
        APP_ID_UNKNOWN,
        {0, 0, 0, 0},
        IPPROTO_MAX,
        {NULL, NULL}
    }
};
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
SKYPE_CONN *
_appidentify_skype_hashGet(SKYPE_CONN *conn)
{
    struct hlist_head   *hash_entry     = NULL;
    struct hlist_node   *hash_node      = NULL;
    SKYPE_CONN          *skype_entry    = NULL;

    if (!conn)
    {
        return NULL;
    }

    hash_entry = &l_skype_hash[SKYPE_HASH(conn->keyIp, conn->keyPort)];

    hlist_for_each(hash_node, hash_entry)
    {
        skype_entry = hlist_entry(hash_node, SKYPE_CONN, hlist);
        if (skype_entry->keyIp == conn->keyIp &&
            skype_entry->keyPort == conn->keyPort)
        {
            return skype_entry;
        }
    }

    return NULL;
}

int _appidentify_skype_hashAdd(SKYPE_CONN *conn)
{
    struct hlist_head  *hash_head = NULL;

    if (!conn)
    {
        return -1;
    }
    hash_head = &l_skype_hash[SKYPE_HASH(conn->keyIp, conn->keyPort)];
    hlist_add_head(&conn->hlist, hash_head);

    return 0;
}

SKYPE_CONN* _appidentify_skype_alloc(void)
{
    SKYPE_CONN *skype_entry = NULL;

    skype_entry = (SKYPE_CONN *)appidentify_mempool_alloc_limit(&l_skype_pool, GFP_ATOMIC);
    if (!skype_entry &&
        !list_empty(&l_skype_lru))
    {
        skype_entry = list_entry(l_skype_lru.next, SKYPE_CONN, lruList);
        list_del(&skype_entry->lruList);
        hlist_del(&skype_entry->hlist);
    }

    return skype_entry;
}

void _appidentify_skype_init(SKYPE_CONN *entry)
{
    if (entry)
    {
        memset(entry, 0, sizeof(SKYPE_CONN));
        INIT_LIST_HEAD(&entry->lruList);
    }
}
/*!
 *\fn           static int _appidentify_qqlive_udp(Ipcom_pkt* pkt, CT_ENTRY* ct,
 *void* cookie) \brief        QQLive udp hard code.
 *\detail
 *
 *\param[in]    pkt
 *\param[in]    ct
 *\param[in]    cookie
 *
 *\return       The application ID
 *\retval       APP_ID_QQLIVE
 *\retval       APP_ID_UNKNOWN
 *
 *\note
 */
static int
_appidentify_qqlive_udp(struct sk_buff *skb, struct nf_conntrack_app *appinfo,
                           void *cookie, unsigned char *data, unsigned int datalen)
{
    int                 ret;
    unsigned char       *data_ptr = NULL;
    int                 udp_datalen = 0;
    int                 pktdir;
    struct iphdr        *iph;
    struct udphdr       *udph;
    struct nf_conn      *ct;
    enum ip_conntrack_info ctinfo;
    APPIDNTFY_KER_RULE* pParam = NULL;

    /* check params */
    if (NULL == skb || NULL == appinfo)
    {
        APPIDNTFY_HARDCODE_ERROR("invalid parameters.\r\n");
        return APP_ID_UNKNOWN;
    }

    iph = ip_hdr(skb);
    ct  = nf_ct_get(skb, &ctinfo);
    if (NULL == iph || NULL == ct)
    {
        return APP_ID_UNKNOWN;
    }

    pktdir = CTINFO2DIR(ctinfo);
    /* 处理正则表达式 */
    pParam = (APPIDNTFY_KER_RULE*)cookie;
    if ((NULL != pParam) && (NULL != pParam->feature[pktdir].fpHandle))
    {
        ret = pParam->feature[pktdir].fpHandle(skb, appinfo, cookie, data, datalen);
        if (APP_ID_UNKNOWN == ret)
        {
            return ret;
        }
    }

    /* get ip header and payload */
    data_ptr = skb->data;
    udph = (struct udphdr *)(data_ptr + ip_hdrlen(skb));
    data_ptr = (unsigned char *)udph;
    data_ptr += sizeof(struct udphdr);
    udp_datalen = skb->len - ip_hdrlen(skb) - sizeof(struct udphdr);

    if (NULL != data_ptr)
    {
        if ((udp_datalen >= 3) && (0xfe == data_ptr[0]))
        {
            short sCommand = 0;

            memcpy(&sCommand, data_ptr + 1, 2);
            sCommand = ntohs(sCommand);

            APPIDNTFY_HARDCODE_DEBUG("sCommand:%x  datalen - 3:%x\r\n", sCommand, udp_datalen - 3);
            if (datalen - 3 == sCommand)
            {
                APPIDNTFY_HARDCODE_DEBUG("QQLive UDP srcPort-%d    dstPort-%d\r\n",
                              udph->source,
                              udph->dest);
                return APP_ID_QQLIVE;
            }
        }
    }

    return APP_ID_UNKNOWN;
}

/*!
 *\fn           static int _appdist_ppstream_udp(Ipcom_pkt* pkt, CT_ENTRY* ct, void* cookie)
 *\brief        PPStream udp hard code.
 *\detail
 *
 *\param[in]    pkt
 *\param[in]    ct
 *\param[in]    cookie
 *
 *\return       The application ID
 *\retval       APP_ID_PPSTREAM
 *\retval       APP_ID_UNKNOWN
 *
 *\note
 */
static int
_appidentify_ppstream_udp(struct sk_buff *skb, struct nf_conntrack_app *appinfo,
                           void *cookie, unsigned char *data, unsigned int datalen)
{
    int                 ret;
    unsigned char       *data_ptr = NULL;
    int                 udp_datalen = 0;
    int                 pktdir;
    struct iphdr        *iph;
    struct udphdr       *udph;
    struct nf_conn      *ct;
    enum ip_conntrack_info ctinfo;
    APPIDNTFY_KER_RULE* pParam = NULL;

    /* check params */
    if (NULL == skb || NULL == appinfo)
    {
        APPIDNTFY_HARDCODE_ERROR("invalid parameters.\r\n");
        return APP_ID_UNKNOWN;
    }

    iph = ip_hdr(skb);
    ct  = nf_ct_get(skb, &ctinfo);
    if (NULL == iph || NULL == ct)
    {
        return APP_ID_UNKNOWN;
    }

    pktdir = CTINFO2DIR(ctinfo);
    /* 处理正则表达式 */
    pParam = (APPIDNTFY_KER_RULE*)cookie;
    if ((NULL != pParam) && (NULL != pParam->feature[pktdir].fpHandle))
    {
        ret = pParam->feature[pktdir].fpHandle(skb, appinfo, cookie, data, datalen);
        if (APP_ID_UNKNOWN == ret)
        {
            return ret;
        }
    }

    /* get ip header and payload */
    data_ptr = skb->data;
    udph = (struct udphdr *)(data_ptr + ip_hdrlen(skb));
    data_ptr = data;
    udp_datalen = datalen;

    if (NULL != data_ptr)
    {
        /*
        *  PPSTREAM
        *
        *  By Peng Xiaochuang, 08Dec10
        */

        if ((udp_datalen >= 3) && (data_ptr[2] != 0x65))
        {
            short sCommand = 0;
            memcpy(&sCommand, data_ptr, 2);
            sCommand = APPDIST_GET_16ON8(sCommand);

            switch(data_ptr[3])
            {
            case 0x00:
                {
                    if ((udp_datalen - 4 == sCommand) || (udp_datalen - 6 == sCommand))
                    {
                        APPIDNTFY_HARDCODE_DEBUG("PPStream srcPort-%d    dstPort-%d.\r\n",
                                      udph->source,
                                      udph->dest);
                        printk("ppstream check found.\r\n");
                        return APP_ID_PPSTREAM;
                    }
                }
                break;
            default:
                {
                    if (udp_datalen == sCommand)
                    {
                        APPIDNTFY_HARDCODE_DEBUG("PPStream srcPort-%d    dstPort-%d.\r\n",
                                      udph->source,
                                      udph->dest);
                        printk("ppstream check found.\r\n");
                        return APP_ID_PPSTREAM;
                    }
                }
                break;
            }
        }
        else
        {
            printk("ppstream check found.\r\n");
            return APP_ID_PPSTREAM;
        }
    }

    return APP_ID_UNKNOWN;
}


/*!
 *\fn           static int _appdist_skype_udp_original(Ipcom_pkt* pkt,
 *                                                     CT_ENTRY* ct,
 *                                                     APPDIST_KER_RULE* pParam)
 *\brief        skype udp original direction hard code.
 *\detail
 *
 *\param[in]    pkt
 *\param[in]    ct
 *\param[in]    pParam
 *
 *\return       The application ID
 *\retval       APP_ID_SKYPE_CHECKING
 *\retval       APP_ID_UNKNOWN
 *
 *\note
 */
static int
_appidentify_skype_udp_original(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *cookie,
                            unsigned char *data, unsigned int datalen, APPIDNTFY_KER_RULE *pParam)
{
    int                 ret;
    unsigned char       *data_ptr = NULL;
    int                 pktdir = IP_CT_DIR_ORIGINAL;
    struct iphdr        *iph;
    struct udphdr       *udph;
    struct nf_conn      *ct;
    enum ip_conntrack_info ctinfo;
    struct nf_conn_counter *counter = NULL;

    SKYPE_CONN  tmpConn;
    SKYPE_CONN* conn_ptr = NULL;

    iph = ip_hdr(skb);
    ct  = nf_ct_get(skb, &ctinfo);
    if (NULL == iph || NULL == ct)
    {
        return APP_ID_UNKNOWN;
    }

    counter = nf_conn_acct_find(ct);
    if (NULL == counter)
    {
        return APP_ID_UNKNOWN;
    }

    pktdir = CTINFO2DIR(ctinfo);
    /**
     * 处理正则表达式
     */
    if ((NULL != pParam) && (NULL != pParam->feature[pktdir].fpHandle))
    {
        ret = pParam->feature[pktdir].fpHandle(skb, appinfo, cookie, data, datalen);
        if (APP_ID_UNKNOWN == ret)
        {
            return ret;
        }
    }
    else
    {
        APPIDNTFY_HARDCODE_DEBUG("NULL pParam.");
        return APP_ID_UNKNOWN;
    }

    /* get udp header and payload */
    data_ptr = data;
    udph = (struct udphdr *)(skb->data + ip_hdrlen(skb));

    /**
     * 测试中发现域名服务器有时也会被放入list_skype中,
     * 因此在这里检测
     */
    if (53 == udph->dest)
    {
        APPIDNTFY_HARDCODE_DEBUG("Domain, cannot be skype.");
        return APP_ID_UNKNOWN;
    }

    tmpConn.keyIp = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip;
    tmpConn.keyPort = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all;

    if (0x02 == data_ptr[2])
    {
        /*printf("ori skype flag connection: ori_count %d, rpy_count %d, ori ip:%x port:%d mempool %d.\n",
               ct->counter[CT_DIR_ORIGINAL].pkts,
               ct->counter[CT_DIR_REPLY].pkts,
               tmpConn.keyIp,
               tmpConn.keyPort,
               de_mem_cnt);
        APPDIST_HARDCODE_DEBUG("ori @2.\n");*/

        if (NULL != (conn_ptr = (SKYPE_CONN *)_appidentify_skype_hashGet(&tmpConn)))
        {
            if (conn_ptr->idFlag[pktdir].idh == data_ptr[0])
            {
                conn_ptr->idFlag[pktdir].idhCnt ++;
            }
            else
            {
                return APP_ID_UNKNOWN;
            }

            if (conn_ptr->idFlag[pktdir].idhCnt >= 2 &&
                conn_ptr->idFlag[1 - pktdir].idhCnt >= 2)
            {
                printk("skype hardcode found.\r\n");
                return APP_ID_SKYPE;
            }
        }
        else
        {
            if ((conn_ptr = _appidentify_skype_alloc()))
            {
                _appidentify_skype_init(conn_ptr);
                conn_ptr->keyIp = tmpConn.keyIp;
                conn_ptr->keyPort = tmpConn.keyPort;
                conn_ptr->idFlag[pktdir].idh = data_ptr[0];
                conn_ptr->idFlag[pktdir].idhCnt ++;
                if (_appidentify_skype_hashAdd(conn_ptr) < 0)
                {
                    return APP_ID_UNKNOWN;
                }
                list_add_tail(&conn_ptr->lruList, &l_skype_lru);
            }
        }
    }

    return APP_ID_UNKNOWN;
}

/*!
 *\fn           static int _appdist_skype_udp(Ipcom_pkt* pkt, CT_ENTRY* ct, void* cookie)
 *\brief        Compass udp hard code.
 *\detail
 *
 *\param[in]    pkt
 *\param[in]    ct
 *\param[in]    cookie
 *
 *\return       The application ID
 *\retval       APP_ID_SKYPE_CHECKING
 *\retval       APP_ID_UNKNOWN
 *
 *\note
 */
static int
_appidentify_skype_udp(struct sk_buff *skb, struct nf_conntrack_app *appinfo,
                           void *cookie, unsigned char *data, unsigned int datalen)
{
    int                 pktdir;
    struct iphdr        *iph;
    struct nf_conn      *ct;
    enum ip_conntrack_info ctinfo;
    APPIDNTFY_KER_RULE* pParam = NULL;

    /* check params */
    if (NULL == skb || NULL == appinfo)
    {
        APPIDNTFY_HARDCODE_ERROR("invalid parameters.\r\n");
        return APP_ID_UNKNOWN;
    }

    iph = ip_hdr(skb);
    ct  = nf_ct_get(skb, &ctinfo);
    if (NULL == iph || NULL == ct)
    {
        return APP_ID_UNKNOWN;
    }

    pktdir = CTINFO2DIR(ctinfo);

    pParam = (APPIDNTFY_KER_RULE*)cookie;

    if (IP_CT_DIR_ORIGINAL == pktdir)
    {
        return _appidentify_skype_udp_original(skb, appinfo, cookie, data, datalen, pParam);
    }
    else
    {
        return _appidentify_skype_udp_original(skb, appinfo, cookie, data, datalen, pParam);
    }
}
/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
int
appidentify_hardcode_init(void)
{

    l_skype_hash = (struct hlist_head *)vmalloc(APPIDNTFY_SKYPE_HASH_SCALE * sizeof(struct hlist_head));
    if (NULL == l_skype_hash)
    {
        APPIDNTFY_HARDCODE_ERROR("skype hash kmalloc error.\r\n");
        return -1;
    }
    memset(l_skype_hash, 0, APPIDNTFY_SKYPE_HASH_SCALE * sizeof(struct hlist_head));

    if (appidentify_mempool_init_limit("skype_conn_pool",
                                       &l_skype_pool, sizeof(SKYPE_CONN),
                                       APPIDNTFY_SKYPE_MIN_RESERVED,
                                       APPIDNTFY_SKYPE_MAX_LIMITED) < 0)
    {
        APPIDNTFY_HARDCODE_ERROR("skype mempool alloc error.\r\n");
        return -1;
    }

    INIT_LIST_HEAD(&l_skype_lru);

    return 0;
}
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
