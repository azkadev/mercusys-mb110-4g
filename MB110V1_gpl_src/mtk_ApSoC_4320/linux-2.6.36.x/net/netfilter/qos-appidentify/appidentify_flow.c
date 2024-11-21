/*! Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     app_flow.c
 *\brief
 *\details
 *
 *\author   Weng Kaiping
 *\version
 *\date     10Oct12
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
#include <linux/types.h>
#include <linux/version.h>
#include <linux/ip.h>
#include <linux/mempool.h>
#include <linux/mutex.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/sysctl.h>
#include <linux/ctype.h>
#include <linux/netdevice.h>
#include <linux/rculist.h>
#include <net/route.h>
#include <net/ip.h>

#include <linux/netfilter_ipv4.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_app.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/ipv4/nf_defrag_ipv4.h>

#include "linux/list.h"
#include "linux/in.h"
#include "appidentify_id.h"
#include "appidentify_flow.h"
#include "appidentify_flow_xml.h"
#include "appprio.h"
#include "appidentify_utils.h"
#include "appidentify_log.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define APP_FLOW_STR_PIECE_MAX_NUM  5
#define APP_FLOW_STR_MAX_LEN  128
#define APP_FLOW_NAME_CNT 20



#define APP_FLOW_ERROR(fmt, args...)   printk("[Error](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args)
#define APP_FLOW_DEBUG(fmt, args...)   /* printk("[Debug](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args)  */

#define LF "\r\n"
#define OK     0
#define ERROR -1



/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef enum _CT_FRAG_TYPE
{
    CT_FIRST_FRAG,
    CT_NORMAL_FRAG,
    CT_LAST_FRAG,
    CT_NOT_FRAG,
}CT_FRAG_TYPE;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
enum tcp_bit_set {
    TCP_SYN_SET,
    TCP_SYNACK_SET,
    TCP_FIN_SET,
    TCP_ACK_SET,
    TCP_RST_SET,
    TCP_NONE_SET,
};

static char*
l_attr_lbls[APP_FLOW_ATTR_CNT] =
{
    "fwd_pkt_len1",
    "fwd_pkt_len2",
    "fwd_pkt_len3",
    "fwd_pkt_len4",
    "fwd_pkt_len5",
    "fwd_pkt_min",
    "fwd_pkt_max",
    "fwd_inter_pkt_len1",
    "fwd_inter_pkt_len2",
    "fwd_inter_pkt_len3",
    "fwd_inter_pkt_len4",
    "bwd_pkt_len1",
    "bwd_pkt_len2",
    "bwd_pkt_len3",
    "bwd_pkt_len4",
    "bwd_pkt_len5",
    "bwd_pkt_min",
    "bwd_pkt_max",
    "bwd_inter_pkt_len1",
    "bwd_inter_pkt_len2",
    "bwd_inter_pkt_len3",
    "bwd_inter_pkt_len4"
};

static APP_FLOW_ID_MAP l_id_map[APP_FLOW_NAME_CNT] =
{
    {"other",       APP_ID_UNKNOWN,         IPPROTO_IP},
    {"thunder",     APP_ID_CLSF_XL,         IPPROTO_UDP},
    {"thunder_tcp", APP_ID_CLSF_XL,         IPPROTO_TCP},
    {"bt",          APP_ID_CLSF_BT,         IPPROTO_UDP},
    {"emule",       APP_ID_CLSF_EMULE,      IPPROTO_TCP},
    {"pps",         APP_ID_CLSF_PPS,        IPPROTO_UDP},
    {"pptv",        APP_ID_CLSF_PPTV,       IPPROTO_UDP},
    {"bc",          APP_ID_CLSF_BT,         IPPROTO_UDP},
    {"flashget",    APP_ID_CLSF_FLASHGET,   IPPROTO_UDP},
    {"qqdownload",  APP_ID_CLSF_XF,         IPPROTO_UDP},
    {"qqmusic",     APP_ID_CLSF_QQMUSIC,    IPPROTO_UDP},
    {"kugou",       APP_ID_CLSF_KUGOU,      IPPROTO_UDP},
    {"flash_p2p",   APP_ID_CLSF_FLASHP2P,   IPPROTO_UDP},
    {"sohuyingyin", APP_ID_UNKNOWN,         IPPROTO_UDP},
    {"fengxing",    APP_ID_UNKNOWN,         IPPROTO_UDP},
    {"pipi",        APP_ID_UNKNOWN,         IPPROTO_UDP},
    {"duomi",       APP_ID_UNKNOWN,         IPPROTO_UDP},
    {"kuaibo",      APP_ID_UNKNOWN,         IPPROTO_UDP},
    {"baiduyingyin",APP_ID_UNKNOWN,         IPPROTO_UDP},
    {"aiqiyi",      APP_ID_UNKNOWN,         IPPROTO_UDP}

};
#if 0
static APP_FLOW_INDEX_APPID_MAP l_index_id_map[] =
{
    {APP_FLOW_ID_THUNDER,        APP_ID_CLSF_XL},
    {APP_FLOW_ID_BITTORRENT,     APP_ID_CLSF_BT},
    {APP_FLOW_ID_EMULE,          APP_ID_CLSF_EMULE},
    {APP_FLOW_ID_PPSTREAM,       APP_ID_CLSF_PPS},
    {APP_FLOW_ID_PPTV,           APP_ID_CLSF_PPTV},
    {APP_FLOW_ID_FLASHGET,       APP_ID_CLSF_FLASHGET},
    {APP_FLOW_ID_XUANFENG,       APP_ID_CLSF_XF},
    {APP_FLOW_ID_QQMUSIC,        APP_ID_CLSF_QQMUSIC},
    {APP_FLOW_ID_KUGOU,          APP_ID_CLSF_KUGOU},
    {APP_FLOW_ID_VOICE,          APP_ID_CLSF_VOICE},
    {-1,                        APP_ID_UNKNOWN}
};
#endif

static int            l_valid_tree_num = 6;
static int            l_udp_counter[4]={0};
static int            l_tcp_counter[4]={0};
static APP_FLOW_NODE  *l_app_flow_tree[APP_FLOW_TREE_NUM];
static struct kmem_cache    *app_flow_slabp;
static mempool_t    *g_app_flow_statPool;
int                  g_isAppclsfInit            = false;
int                  g_enableClsf               = true;
unsigned int         g_app_flow_total = 0;
unsigned int         g_app_flow_total_udp = 0;
unsigned int         g_app_flow_total_tcp = 0;
unsigned int         g_app_flow_cnt[APP_FLOW_ID_MAX] = {0};
unsigned int         g_app_flow_cntAlive[APP_FLOW_ID_MAX] = {0};

//EXPORT_SYMBOL(g_isAppclsfInit);

unsigned int  app_flow_debug  = 0;
unsigned int  app_flow_check   = 1;
unsigned int  app_flow_print = 0;
unsigned int  app_flow_print_udp = 0;
unsigned int  app_flow_print_tcp = 0;

extern void nf_conntrack_cleanup_app(void);
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
static int _ct_ipv4_printTuple(const struct nf_conntrack_tuple *tuple)
{
    return printk( "src=%pI4 dst=%pI4 ",
              &tuple->src.u3.ip, &tuple->dst.u3.ip);
}



/*!
 *\fn           CT_FRAG_TYPE ct_frag_getType(Ipcom_pkt *pkt)
 *\brief        Resolve fragment type from packet structure.
 *\details
 *
 *\param[in]    pkt - packet to parse.
 *\param[out]
 *
 *\return       IP-fragment tupe.
 *\retval       CT_FIRST_FRAG - the first fragment of a group.
 *\retval       CT_LAST_FRAG - the last fragment of a group.
 *\retval       CT_NORMAL_FRAG - normal fragment, not first or last.
 *\retval       CT_NOT_FRAG - not a fragment packet.
 *
 *\note
 */
CT_FRAG_TYPE _ct_frag_getType(struct sk_buff *skb)
{

    int more_frag = 0;
    int dont_frag = 0;
    int frag_offset = 0;

    more_frag   = ntohs(ip_hdr(skb)->frag_off) & IP_MF ;
    dont_frag   = ntohs(ip_hdr(skb)->frag_off) & IP_DF ;
    frag_offset = ntohs(ip_hdr(skb)->frag_off) & IP_OFFSET;

    if (dont_frag)
    {
        return CT_NOT_FRAG;
    }

    if (more_frag && !frag_offset)
    {
        return CT_FIRST_FRAG;
    }
    else if (more_frag && frag_offset)
    {
        return CT_NORMAL_FRAG;
    }
    else if (!more_frag && frag_offset)
    {
        return CT_LAST_FRAG;
    }
    else
    {
        return CT_NOT_FRAG;
    }

}


void
set_valid_tree_num(int i)
{
    l_valid_tree_num = i;
}
#if 0
static unsigned short
_get_appdistId(int index)
{
    int i = 0;

    for (i = 0; l_index_id_map[i].appIndex != -1; i++)
    {
        if (index == l_index_id_map[i].appIndex)
        {
            return l_index_id_map[i].appdistId;
        }
    }

    return 0;
}
#endif
#if APP_FLOW_TIME
static int
_getTimeStr(char *timeStr)
{
    struct Ip_timeval tv;
    struct Ip_tm      gmt_tm;

    if (NULL == timeStr)
    {
        APP_FLOW_ERROR("Some parameter is null."LF);
        return ERROR;
    }

    if (!ipsntp_isInitFinish())
    {
        /* Initialization of SNTP is not finished, so we have to use the default time string. */
        strncpy(timeStr, "0000-00-00_00:00:00", strlen("0000-00-00_00:00:00"));
        return OK;
    }

    memset(&tv, 0, sizeof(tv));
    memset(&gmt_tm, 0, sizeof(gmt_tm));

    /* Gets the system time which is set by SNTP. */
    if (IPCOM_SUCCESS != ipcom_gettimeofday(&tv, NULL))
    {
        APP_FLOW_ERROR("ipcom_gettimeofday failed."LF);
        return ERROR;
    }

    ipcom_gmtime_r(&tv.tv_sec, &gmt_tm);
    gmt_tm.tm_year += 1900;
    gmt_tm.tm_mon  += 1;

    timeStr[19] = '\0';

    sprintk(timeStr,
            "%4d-%02d-%02d_%02d:%02d:%02d",
            gmt_tm.tm_year,
            gmt_tm.tm_mon,
            gmt_tm.tm_mday,
            gmt_tm.tm_hour,
            gmt_tm.tm_min,
            gmt_tm.tm_sec);

    return OK;
}
#endif

static int
_getAttrIndex(char *pStr)
{
    int index;

    if (NULL == pStr)
    {
        return ERROR;
    }

    for (index = 0; index < APP_FLOW_ATTR_CNT; index++)
    {
        if (0 == strcmp(l_attr_lbls[index], pStr))
        {
            return index;
        }
    }

    return ERROR;
}

/*
static char *
_getAppName(int id)
{
    int index;
    char *appName = NULL;

    for (index = 0; index < APP_FLOW_NAME_CNT; index++)
    {
        if (l_id_map[index].id == id)
        {
            appName = l_id_map[index].appName;
            break;
        }
    }

    return appName;
}
*/
static int
_getAppId(char *pStr)
{
    int index;

    if (NULL == pStr)
    {
        return ERROR;
    }

    for (index = 0; index < APP_FLOW_NAME_CNT; index++)
    {
        if (0 == strcmp(l_id_map[index].appName, pStr))
        {
            return index;
        }
    }

    return ERROR;
}

static void
_print_ct_info(struct nf_conn *ct)
{
    struct nf_conntrack_tuple *tuple;

    _ct_ipv4_printTuple(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
    tuple = &(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
    printk("port: %-5hu->%-5hu ", ntohs(tuple->src.u.tcp.port), ntohs(tuple->dst.u.tcp.port));

    _ct_ipv4_printTuple(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);
    tuple = &(ct->tuplehash[IP_CT_DIR_REPLY].tuple);
    printk("port: %-5hu->%-5hu ", ntohs(tuple->src.u.tcp.port), ntohs(tuple->dst.u.tcp.port));
}

#if 1
static void
_print_app_flow_result(struct nf_conn *ct, int *flowAttr, char shootTrees, int appNameIndex)
{
    int index;
    int totalCt = 0;
    int iProtocol;
    char *appName;
    struct nf_conntrack_app * app_info;
    struct nf_conn_counter *acct;

    app_info = nf_ct_get_app(ct);
    acct = nf_conn_acct_find(ct);

    appName = l_id_map[appNameIndex].appName;/*_getAppName(appNameIndex);*/
    iProtocol = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    if(IPPROTO_UDP == iProtocol)
    {
        if (!app_flow_print_udp)
        {
            return;
        }
    printk("Proto:UDP    ");
        totalCt = g_app_flow_total_udp;
    }
    else
    {
        if (!app_flow_print_tcp)
        {
            return;
        }
        printk("Proto:TCP    ");
        totalCt = g_app_flow_total_tcp;
    }

#if APP_FLOW_TIME
    printk("time:%s     ", ((APP_FLOW_STAT *)app_info->app_flow_stat)->data->timeStr);
#endif
    printk("App:%s   shootTrees:%d  total:%d poolsize:%d\r\n", appName,
                                                      shootTrees,
                                                      totalCt,
                                                      g_app_flow_statPool->curr_nr);


    _print_ct_info(ct);

//    printk("\r\norigin:%llu\r\n",acct[IP_CT_DIR_ORIGINAL].packets);
    printk("%5d", flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1]);
    for(index = 1; index < APP_FLOW_ATTR_CNT/2; index++)
    {
        printk(",%5d", flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index]);
    }

//  printk("\r\nreply:%llu\r\n",acct[IP_CT_DIR_REPLY].packets);
    printk("%5d", flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1]);
    for(index = 1; index < APP_FLOW_ATTR_CNT/2; index++)
    {
        printk(",%5d", flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index]);
    }

    printk("\r\n\r\n");

}
#endif

static int
_parseStrToNode(char *pStr, APP_FLOW_NODE *pTmp, char *strType)
{
    int strNum = 0;
    int index = 0;
    char *strElem[APP_FLOW_STR_PIECE_MAX_NUM + 1]={NULL};
    APP_FLOW_ATTR_INDEX attrIndex;
    APP_FLOW_APP_INDEX  appIndex;
    char pStr1[APP_FLOW_STR_MAX_LEN];

    if(!pStr || !pTmp || !strType)
    {
        APP_FLOW_ERROR("param is null\n");
        return ERROR;
    }
    strcpy(pStr1, pStr);
    memset(pTmp, 0, sizeof(APP_FLOW_NODE));

    if (ERROR != (strNum = string_makeSubStrByChar(pStr1, ' ', APP_FLOW_STR_PIECE_MAX_NUM + 1, strElem)))
    {
        if (strNum != 3 && strNum != 5)
        {
            APP_FLOW_ERROR("APP_FLOW str paser error.strNum is %d"LF,strNum);
            return ERROR;
        }

        if (!strcmp("<=",strElem[1]))
        {
            *strType = 0;
        }
        else if (!strcmp(">",strElem[1]))
        {
            *strType = 1;
        }
        else
        {
            APP_FLOW_ERROR("APP_FLOW tree string parse error."LF);
            return ERROR;
        }

        if (ERROR != (attrIndex = _getAttrIndex(strElem[0])))
        {
            pTmp->feature = attrIndex;
        }
        else
        {
            APP_FLOW_ERROR("APP_FLOW tree string parse error."LF);
            return ERROR;
        }


        if (ERROR != (index = string_charReplace(strElem[2], ' ', ':')))
        {
            pTmp->boundary = simple_strtol(strElem[2], NULL, 0);
            if (index > 0)
            {
                pTmp->isLeaf = 1;
                if (ERROR != (appIndex = _getAppId(strElem[3])))
                {
                    pTmp->id = appIndex;
                }
                else
                {
                    APP_FLOW_ERROR("APP_FLOW tree string parse error."LF);
                    return ERROR;
                }
            }
        }
        else
        {
            APP_FLOW_ERROR("APP_FLOW tree string parse error."LF);
            return ERROR;
        }


    }
    else
    {
        APP_FLOW_ERROR("APP_FLOW tree string parse error."LF);
        return ERROR;
    }

    return OK;
}


int
app_flow_set_param(unsigned int paramId, unsigned int value)
{
    printk("app_flow set param %d, value:%d\n", paramId, value);
    switch(paramId)
    {
    case APP_FLOW_DEBUG_SWITCH:
        app_flow_debug = value;
        break;

    case APP_FLOW_CHECK_APPID:
        app_flow_check = value;
        break;

    case APP_FLOW_PRINT:
        app_flow_print = value;
        break;

    case APP_FLOW_PRINT_UDP:
        app_flow_print_udp = value;
        break;

    case APP_FLOW_PRINT_TCP:
        app_flow_print_tcp = value;
        break;

    default:
        printk("unknown paramId\n");
        break;

    }

    return 0;
}

void app_flow_new_stat(void ** pStat)
{
    *pStat = mempool_alloc(g_app_flow_statPool, GFP_ATOMIC);
    memset(*pStat, 0, sizeof(APP_FLOW_STAT));
    ((APP_FLOW_STAT *)(*pStat))->pfreeHandler = app_flow_free_stat;
}

void app_flow_free_stat(void * pStat)
{
    if (NULL == pStat)
    {
        return;
    }

    mempool_free(pStat,g_app_flow_statPool);
}
//EXPORT_SYMBOL(app_flow_free_stat);


/*!
 *\fn        Ip_err app_flow_tree_init()
 *\brief
 *\details
 *
 *\param[in]
 *\param[out]
 *
 *\return
 *\retval
 *
 *\note
 */
int app_flow_tree_init(void)
{
    int i = 0;
    int index = 0;
    int treeSize = 0;

    for (index = 0; index < APP_FLOW_TREE_NUM; index++)
    {
        treeSize = g_tree_lbls[index].treeSize;
        l_app_flow_tree[index] = (APP_FLOW_NODE *)kmalloc(treeSize*sizeof(APP_FLOW_NODE),GFP_ATOMIC);
        if (NULL == l_app_flow_tree[index])
        {
            APP_FLOW_ERROR("NO memory."LF);
            return ERROR;
        }

        for (i=0; i<treeSize; i++)
        {
            memset(&l_app_flow_tree[index][i], 0, sizeof(APP_FLOW_NODE));
            l_app_flow_tree[index][i].id = APP_FLOW_ID_UNKNOWN;
        }
    }
    return OK;

}

int app_flow_tree_destroy(void)
{
    int index = 0;

    for (index = 0; index < APP_FLOW_TREE_NUM; index++)
    {
        if(NULL != l_app_flow_tree[index])
        {
            kfree(l_app_flow_tree[index]);
            l_app_flow_tree[index] = NULL;
        }
    }
    return OK;

}


/*!
 *\fn        Ip_err app_flow_tree_construct()
 *\brief
 *\details
 *
 *\param[in]
 *\param[out]
 *
 *\return
 *\retval
 *
 *\note
 */
int app_flow_tree_construct(void)
{
    int i = 0;
    int index = 0;
    int treeSize = 0;
    int usdNode = 0;
    char strType = 0;
    struct list_head treelist[APP_FLOW_TREE_NUM];
    APP_FLOW_NODE parseTmp;
    APP_FLOW_NODE *prev;

    APP_FLOW_DEBUG("BEGIN\n");
    for (index = 0; index < APP_FLOW_TREE_NUM; index++)
    {
        APP_FLOW_DEBUG("index:%d\n", index);
        usdNode = 0;
        treeSize = g_tree_lbls[index].treeSize;
        APP_FLOW_DEBUG("tree[%d] size:%d\n", index, treeSize);
        INIT_LIST_HEAD(&treelist[index]);
        for (i = 0; i < (treeSize - 1); i++)
        {

            if (ERROR == _parseStrToNode(g_tree_lbls[index].treelbls[i], &parseTmp, &strType))
            {
                APP_FLOW_ERROR("TREE %d PARSE STRING[%d] %s ERROR."LF, index, i, g_tree_lbls[index].treelbls[i]);
                return ERROR;
            }


            /*string is like"... <=..." or "....<=:", add node.*/
            if (0 == strType)
            {
                l_app_flow_tree[index][usdNode].isLeaf = 0;
                l_app_flow_tree[index][usdNode].id = APP_FLOW_ID_UNKNOWN;
                l_app_flow_tree[index][usdNode].left  = NULL;
                l_app_flow_tree[index][usdNode].right = NULL;
                l_app_flow_tree[index][usdNode].feature  = parseTmp.feature;
                l_app_flow_tree[index][usdNode].boundary = parseTmp.boundary;

                if (!list_empty_careful(&treelist[index]))
                {
                   prev = (APP_FLOW_NODE *)(treelist[index].prev);
                   if (NULL == prev->left)
                   {
                       prev->left = &(l_app_flow_tree[index][usdNode]);
                   }
                   else if(NULL == prev->right)
                   {
                       prev->right = &(l_app_flow_tree[index][usdNode]);
                   }
                   else
                   {
                       APP_FLOW_ERROR("TREE %d CONSTRUCT ERROR."LF, index);
                       return ERROR;
                   }
                }

                list_add_tail(&(l_app_flow_tree[index][usdNode].list), &treelist[index]);
                usdNode++;
                if(usdNode >= treeSize)
                {
                    APP_FLOW_ERROR("usdNode too large."LF);
                    return ERROR;
                }

                if (parseTmp.isLeaf)
                {
                    l_app_flow_tree[index][usdNode].isLeaf = 1;
                    l_app_flow_tree[index][usdNode].id = parseTmp.id;
                    l_app_flow_tree[index][usdNode].left  = NULL;
                    l_app_flow_tree[index][usdNode].right = NULL;
                    l_app_flow_tree[index][usdNode-1].left = &(l_app_flow_tree[index][usdNode]);
                    usdNode++;
                    if(usdNode >= treeSize)
                    {
                        APP_FLOW_ERROR("usdNode too large."LF);
                        return ERROR;
                    }
                }
            }
            /*string is like"... >..." or "....>:", check node.*/
            else
            {
                if (list_empty_careful(&treelist[index]))
                {
                    APP_FLOW_ERROR("TREE %d CONSTRUCT ERROR at node %d."LF,index,i);
                    return ERROR;
                }
                prev = (APP_FLOW_NODE *)(treelist[index].prev);
                if (prev->feature != parseTmp.feature || prev->boundary != parseTmp.boundary
                    || prev->left == NULL)
                {
                    APP_FLOW_ERROR("TREE %d CONSTRUCT ERROR."LF, index);
                    return ERROR;
                }


                if (parseTmp.isLeaf)
                {
                    l_app_flow_tree[index][usdNode].isLeaf = 1;
                    l_app_flow_tree[index][usdNode].id = parseTmp.id;
                    l_app_flow_tree[index][usdNode].left  = NULL;
                    l_app_flow_tree[index][usdNode].right = NULL;

                    prev->right = &(l_app_flow_tree[index][usdNode]);
                    while (!list_empty_careful(&treelist[index]) && NULL != prev->right)
                    {
                        list_del(&(prev->list));
                        prev = (APP_FLOW_NODE *)(treelist[index].prev);
                    }
                    if (list_empty_careful(&treelist[index]) && i < (treeSize -2))
                    {
                        APP_FLOW_ERROR("TREE %d CONSTRUCT ERROR."LF, index);
                        return ERROR;
                    }

                    usdNode++;
                    if(usdNode >= treeSize && i < (treeSize -2))
                    {
                        APP_FLOW_ERROR("usdNode too large."LF);
                        return ERROR;
                    }
                }
            }
        }


    }

    APP_FLOW_DEBUG("END\n");

    return OK;

}


/*!
 *\fn       IP_PUBLIC unsigned int app_flow_statistic_record( Ipcom_pkt *pkt )
 *\brief    Record the packet length
 *\details
 *
 *\param[in]    pkt
 *\param[out]   N/A
 *
 *\return       The operation result.
 *\retval       OK
 *
 *\note
 */
unsigned int app_flow_statistic_record(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
    int       iProtocol   = 0;
    int       index       = 0;
    short    basicApp    = APP_BASIC_ID_CHECKING;
    short    normalApp   = APP_NORMAL_ID_CHECKING;
    unsigned int  dir     = IP_CT_DIR_MAX;

    APP_FLOW_DATA *pFlowData = NULL;
    struct nf_conn     *pCtEntry = NULL;
    struct nf_conntrack_app *app_info = NULL;
    struct nf_conn_counter *acct = NULL;
    enum ip_conntrack_info ctinfo;
    CT_FRAG_TYPE  frag_type;

    const struct iphdr *iph;
    const struct tcphdr *th;
    struct tcphdr _tcph;
    int dataoff = 0;
    int nhl = 0;

    if (!g_enableClsf)
    {
        return NF_ACCEPT;
    }

    if ( false == g_isAppclsfInit)
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

    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_CLSF) ||
        APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER) ||
        APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PRI_SET))
    {
        goto free_stat;
    }

    dir = CTINFO2DIR(ctinfo);

    basicApp = APP_EVEN_BASIC_ID_FLAG_GET(app_info->appidntfy_flag[0].app_id_index,app_info->appidntfy_flag[1].app_id_index);
    normalApp = APP_EVEN_NORMAL_ID_GET(app_info->appidntfy_flag[0].app_id_index,app_info->appidntfy_flag[1].app_id_index);
#if 1
#ifndef COLLECT_TRAIN_DATA
    if (app_flow_check && (( basicApp != APP_BASIC_ID_CHECKING && basicApp != APP_BASIC_ID_UNKNOWN)
                       || (normalApp != APP_NORMAL_ID_CHECKING && normalApp != APP_NORMAL_ID_UNKNOWN)))
    {
            if (app_flow_print)
        {
                printk("already appdist:basic(%4x),normal(%4x)"LF, basicApp,normalApp);
                _print_ct_info(pCtEntry);
                printk("\r\n\r\n");
        }
            goto free_stat;
    }
#endif
#endif

    iProtocol = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    if ( IPPROTO_UDP != iProtocol && IPPROTO_TCP != iProtocol)/**/
    {
        return NF_ACCEPT;
    }

    if (IPPROTO_TCP == iProtocol && (TCP_CONNTRACK_ESTABLISHED != pCtEntry->proto.tcp.state ))
    {
        return NF_ACCEPT;
    }

    acct = nf_conn_acct_find(pCtEntry);
    if (!acct)
    {
        return NF_ACCEPT;
    }

    if ( NULL == app_info->app_flow_stat )
    {
        app_flow_new_stat(&(app_info->app_flow_stat));
        if ( NULL == app_info->app_flow_stat)
        {
            APP_FLOW_ERROR("alloc mem fail\n");
            goto free_stat;
        }
    }

    pFlowData = &(((APP_FLOW_STAT *)app_info->app_flow_stat)->data);


#if APP_FLOW_TIME
    if (0 == pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] && 0 == pFlowData->recordCnt[IP_CT_DIR_REPLY])
    {
        if (ERROR == _getTimeStr(pFlowData->timeStr))
        {
             strncpy(pFlowData->timeStr, "0000-00-00_00:00:00", strlen("0000-00-00_00:00:00"));
        }

    }

#endif

    /**/
    index = pFlowData->recordCnt[dir];
    frag_type = _ct_frag_getType(skb);

    if (index < APP_FLOW_RECORD_RANGE)
    {

        iph = ip_hdr(skb);
        if ( IPPROTO_UDP == iProtocol )
        {
            nhl = (iph->ihl << 2) + 8;
        }
        else
        {
            dataoff = skb_network_offset(skb) + (iph->ihl << 2);
            th = skb_header_pointer(skb, dataoff, sizeof(_tcph), &_tcph);
            nhl = (iph->ihl << 2) + th->doff * 4;
        }

        switch (frag_type)
        {
        case CT_FIRST_FRAG:
        case CT_NORMAL_FRAG:
            pFlowData->pktLen[dir][index] += (skb->len - skb_network_offset(skb) - nhl);
            break;
        case CT_LAST_FRAG:
            pFlowData->pktLen[dir][index] += (skb->len - skb_network_offset(skb)- nhl);
            pFlowData->recordCnt[dir]++;
            break;
        default:
#ifndef COLLECT_TRAIN_DATA
            if ((pCtEntry->proto.tcp.last_index == TCP_ACK_SET) &&(skb->len - skb_network_offset(skb)-nhl) <= 6)
            {
                return NF_ACCEPT;
            }
#endif
            pFlowData->pktLen[dir][index] += (skb->len - skb_network_offset(skb) - nhl);
            pFlowData->recordCnt[dir]++;

        break;
        }

    }


    //if(acct[IP_CT_DIR_ORIGINAL].packets >=APP_FLOW_RANGE || acct[IP_CT_DIR_REPLY].packets >=APP_FLOW_RANGE)
    if ((app_flow_debug && (pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >= 5 || pFlowData->recordCnt[IP_CT_DIR_REPLY]>= 5))
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
	 ||(acct[IP_CT_DIR_ORIGINAL].packets >=APP_FLOW_RANGE || acct[IP_CT_DIR_REPLY].packets >=APP_FLOW_RANGE))
#else
        ||(acct[IP_CT_DIR_ORIGINAL].packets.counter >=APP_FLOW_RANGE || acct[IP_CT_DIR_REPLY].packets.counter >=APP_FLOW_RANGE))
#endif
    {

#ifndef COLLECT_TRAIN_DATA
        if (app_flow_check &&((basicApp== APP_BASIC_ID_CHECKING)|| (normalApp== APP_NORMAL_ID_CHECKING)))
        {
            if (app_flow_print)
            {
                printk("still in appdist:basic(%4x),normal(%4x)"LF,basicApp,normalApp);
            }/**/
            return NF_ACCEPT;
        }

        if ((app_flow_debug && (pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >= 4 || pFlowData->recordCnt[IP_CT_DIR_REPLY]>= 4))
            || (pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >= 4 && pFlowData->recordCnt[IP_CT_DIR_REPLY]>= 4))
        {
            app_flow_distinguish_hook(pCtEntry);
        }
        goto free_stat;
#else
        if (pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] == APP_FLOW_RECORD_RANGE && pFlowData->recordCnt[IP_CT_DIR_REPLY]== APP_FLOW_RECORD_RANGE)
        {
            app_flow_print_app_flow_train(pCtEntry);
            goto free_stat;
        }

#endif

    }

    return NF_ACCEPT;

free_stat:
    if (! APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_CLSF))
    {
        APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_CLSF);
    }

    if (NULL != app_info && NULL != app_info->app_flow_stat)
    {
        app_info->app_flow_stat = NULL;
    }

    return NF_ACCEPT;
}


/*!
 *\fn       IP_PUBLIC void app_flow_distinguish_hook(struct nf_conn *pCtEntry)
 *\brief    Distinguish application according to the statistical data
 *\details
 *
 *\param[in]    pCtEntry
 *\param[out]   pCtEntry
 *
 *\return       N/A
 *\retval       N/A
 *
 *\note
 */
void app_flow_distinguish_hook(struct nf_conn *pCtEntry)
{
    int iProtocol = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;

    int flowAttr[APP_FLOW_ATTR_CNT] = {0};
    int index = 0;
    int appIndex = 0;
    char shootTrees = 0;
    char shootNum[APP_FLOW_NAME_CNT] = {0};
    APP_FLOW_DATA *pFlowData;
    APP_FLOW_NODE *pNode;
    struct nf_conntrack_app * app_info;

    /* if(IP_IPPROTO_TCP == iProtocol)
    {
        return;
    }*/
    APP_FLOW_DEBUG("hook!!!!"LF);

    app_info = nf_ct_get_app(pCtEntry);
    pFlowData = &(((APP_FLOW_STAT *)app_info->app_flow_stat)->data);

    flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1]=flowAttr[APP_FLOW_ATTR_FWD_PKTMIN]
                                      =flowAttr[APP_FLOW_ATTR_FWD_PKTMAX]
                                      =pFlowData->pktLen[IP_CT_DIR_ORIGINAL][0];
    flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1]=flowAttr[APP_FLOW_ATTR_BWD_PKTMIN]
                                      =flowAttr[APP_FLOW_ATTR_BWD_PKTMAX]
                                      =pFlowData->pktLen[IP_CT_DIR_REPLY][0];

    for (index = 1; index <= APP_FLOW_ATTR_FWD_PKTLEN5; index++)
    {
        /*lan to wan*/
        flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index] = pFlowData->pktLen[IP_CT_DIR_ORIGINAL][index];
        if (flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index] < flowAttr[APP_FLOW_ATTR_FWD_PKTMIN])
        {
            flowAttr[APP_FLOW_ATTR_FWD_PKTMIN] = flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index];
        }
        if (flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index] > flowAttr[APP_FLOW_ATTR_FWD_PKTMAX])
        {
            flowAttr[APP_FLOW_ATTR_FWD_PKTMAX] = flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index];
        }
        flowAttr[APP_FLOW_ATTR_FWD_INTER_PKTLEN1 + index - 1]
         = flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index] - flowAttr[APP_FLOW_ATTR_FWD_PKTLEN1 + index - 1];

        /*wan to lan*/
        flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index] = pFlowData->pktLen[IP_CT_DIR_REPLY][index];
        if (flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index] < flowAttr[APP_FLOW_ATTR_BWD_PKTMIN])
        {
            flowAttr[APP_FLOW_ATTR_BWD_PKTMIN] = flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index];
        }
        if (flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index] > flowAttr[APP_FLOW_ATTR_BWD_PKTMAX])
        {
            flowAttr[APP_FLOW_ATTR_BWD_PKTMAX] = flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index];
        }
        flowAttr[APP_FLOW_ATTR_BWD_INTER_PKTLEN1 + index - 1]
         = flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index] - flowAttr[APP_FLOW_ATTR_BWD_PKTLEN1 + index - 1];
    }

    for (index = 0; index< APP_FLOW_TREE_NUM; index++)
    {
        pNode = &l_app_flow_tree[index][0];
        while (NULL != pNode && !pNode->isLeaf)
        {
            if(flowAttr[pNode->feature] <= pNode->boundary)
            {
                pNode = pNode->left;
            }
            else
            {
                pNode = pNode->right;
            }
        }

        if (NULL == pNode)
        {
            APP_FLOW_ERROR("TREE CONSTRUCT ERROR."LF);
            return;
        }

        shootNum[pNode->id]++;
    }

    /*pCtEntry->app_flow_id = APP_FLOW_ID_UNKNOWN;*/
    for (index = 0; index < APP_FLOW_NAME_CNT; index ++)
    {
        if(shootNum[index] >= l_valid_tree_num)
        {
            if (l_id_map[index].protoNum == IPPROTO_IP || l_id_map[index].protoNum == iProtocol)
            {
                if (app_flow_print)
                {
                    printk("appcls set app id %d.\n", l_id_map[index].id);
                }
                if (APP_ID_UNKNOWN != l_id_map[index].id)
                {
                    APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, l_id_map[index].id);
                    APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, l_id_map[index].id);
                }
                APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_CLSF);
                shootTrees = shootNum[index];
                appIndex = index;
            }
            break;
            }
        }
#if 0
    if ( APP_FLOW_ID_UNKNOWN == app_info->app_flow_id)
    {
        appIndex = 0;
    }
#endif

    g_app_flow_total++;
    if (IPPROTO_UDP == iProtocol)
    {
        g_app_flow_total_udp++;
    }
    else
    {
        g_app_flow_total_tcp++;
    }
#if 0
    g_app_flow_cnt[app_info->app_flow_id]++;
    g_app_flow_cntAlive[app_info->app_flow_id]++;
#endif
    if (app_flow_print)
    {
        _print_app_flow_result(pCtEntry, flowAttr,shootTrees,appIndex);/**/
    }

    return;
}

void
app_flow_print_app_flow_train(struct nf_conn *pCtEntry)
{
    int index;
    int iProtocol;
    int pktNum;
    struct nf_conntrack_tuple *tuple;
    APP_FLOW_DATA *pFlowData;
    struct nf_conntrack_app * app_info;
    struct nf_conn_counter *acct;

    acct = nf_conn_acct_find(pCtEntry);
    app_info = nf_ct_get_app(pCtEntry);
    pFlowData = &(((APP_FLOW_STAT *)app_info->app_flow_stat)->data);

    APP_FLOW_DEBUG("PRINT...."LF);

    if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL]<2 || pFlowData->recordCnt[IP_CT_DIR_REPLY]<1)
    {
        return;
    }

    iProtocol = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
    if(IPPROTO_UDP == iProtocol)
    {
        if(!app_flow_print_udp)
        {
        return;
        }
        if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=5)
        {
            l_udp_counter[0]++;
            if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=10)
            {
                l_udp_counter[1]++;
                if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=15)
                {
                    l_udp_counter[2]++;
                    if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=20)
                        l_udp_counter[3]++;
                }
            }
        }

        printk("Proto:UDP   ");
#if APP_FLOW_TIME
        printk("time:%s  ", pFlowData->timeStr);
#endif
        printk("counter:%6d,%6d,%6d,%6d\r\n", l_udp_counter[0],l_udp_counter[1],l_udp_counter[2],l_udp_counter[3]);
    }
    else if(IPPROTO_TCP == iProtocol)
    {
        if(!app_flow_print_tcp)
        {
            return;
        }
        if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=5)
        {
            l_tcp_counter[0]++;
            if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=10)
            {
                l_tcp_counter[1]++;
                if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=15)
                {
                    l_tcp_counter[2]++;
                    if(pFlowData->recordCnt[IP_CT_DIR_ORIGINAL] >=20)
                        l_tcp_counter[3]++;
                }
            }
        }

        printk("Proto:TCP   ");
#if APP_FLOW_TIME
        printk("time:%s  ", pFlowData->timeStr);
#endif
        printk("counter:%6d,%6d,%6d,%6d\r\n", l_tcp_counter[0],l_tcp_counter[1],l_tcp_counter[2],l_tcp_counter[3]);
    }
    else
    {
        return;
    }

    _ct_ipv4_printTuple(&pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
    tuple = &(pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
    printk("port: %-5hu->%-5hu ", ntohs(tuple->src.u.udp.port), ntohs(tuple->dst.u.udp.port));

    _ct_ipv4_printTuple(&pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple);
    tuple = &(pCtEntry->tuplehash[IP_CT_DIR_REPLY].tuple);
    printk("port: %-5hu->%-5hu ", ntohs(tuple->src.u.udp.port), ntohs(tuple->dst.u.udp.port));

//    printk("\r\norigin:%llu\r\n", acct[IP_CT_DIR_ORIGINAL].packets);
    printk("%5d", pFlowData->pktLen[IP_CT_DIR_ORIGINAL][0]);
    pktNum = pFlowData->recordCnt[IP_CT_DIR_ORIGINAL];
    if(pktNum > APP_FLOW_RECORD_RANGE)
    {
        pktNum = APP_FLOW_RECORD_RANGE;
    }
    for(index = 1; index < pktNum; index++)
    {
        printk(",%5d", pFlowData->pktLen[IP_CT_DIR_ORIGINAL][index]);
    }

//    printk("\r\nreply:%llu\r\n", acct[IP_CT_DIR_REPLY].packets);
    printk("%5d", pFlowData->pktLen[IP_CT_DIR_REPLY][0]);
    pktNum = pFlowData->recordCnt[IP_CT_DIR_REPLY];
    if(pktNum > APP_FLOW_RECORD_RANGE)
    {
        pktNum = APP_FLOW_RECORD_RANGE;
    }
    for(index = 1; index < pktNum; index++)
    {
        printk(",%5d", pFlowData->pktLen[IP_CT_DIR_REPLY][index]);
    }

    printk("\r\n\r\n");

    return;

}

/*!
 *\fn        void app_flow_init()
 *\brief    Appclsf init
 *\details  Memory alloc and construct a tree
 *
 *\return   N/A
 *\retval   N/A
 *
 *\note
 */
int app_flow_init(void)
{
 //   int ret = 0;
    g_isAppclsfInit = false;

    if (OK != app_flow_tree_init())
    {
        APP_FLOW_ERROR("app_flow tree init fail!"LF);
        goto cleanup_tree;
    }
    APP_FLOW_DEBUG("app_flow tree init OK."LF);

    if (OK != app_flow_tree_construct())
    {
        APP_FLOW_ERROR("app_flow tree construct fail!"LF);
        goto cleanup_tree;
    }
    APP_FLOW_DEBUG("app_flow tree construct OK."LF);

    app_flow_slabp = kmem_cache_create("app_flow_stat",
                         sizeof(APP_FLOW_STAT),
                         0, SLAB_HWCACHE_ALIGN,
                         NULL);
    if (!app_flow_slabp)
    {
        APP_FLOW_ERROR("app_flow_slabp create fail."LF);
        goto cleanup_slab;
    }


    g_app_flow_statPool = mempool_create_slab_pool(APP_FLOW_STAT_POOL_SIZE,
                            app_flow_slabp);
    /*g_app_flow_statPool = mempool_create_kmalloc_pool(APP_FLOW_STAT_POOL_SIZE,sizeof(APP_FLOW_STAT));*/
    if (NULL == g_app_flow_statPool)
    {
        APP_FLOW_ERROR("g_app_flow_statPool create fail."LF);
        goto cleanup_pool;
    }
    APP_FLOW_DEBUG("g_app_flow_statPool create OK."LF);

    g_isAppclsfInit = true;

    return OK;
/* cleanup_nl:  */
/*     app_netlink_fini(); */

cleanup_pool:
    if(g_app_flow_statPool)
       mempool_destroy(g_app_flow_statPool);
    g_app_flow_statPool = NULL;
cleanup_slab:
    if(app_flow_slabp)
       kmem_cache_destroy(app_flow_slabp);
    app_flow_slabp = NULL;

cleanup_tree:
    app_flow_tree_destroy();

    return ERROR;
}

void app_flow_exit(void)
{
    g_isAppclsfInit = false;

    /* app_netlink_fini(); */

    nf_conntrack_cleanup_app();

    if(g_app_flow_statPool)
       mempool_destroy(g_app_flow_statPool);

    if(app_flow_slabp)
       kmem_cache_destroy(app_flow_slabp);

    app_flow_tree_destroy();

    printk("module app_flow exit ok\n");
}
