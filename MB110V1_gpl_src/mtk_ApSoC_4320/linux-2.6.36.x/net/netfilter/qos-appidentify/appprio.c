/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appprio.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     17Oct13
 *
 *\warning
 *
 *\history \arg 0.0.1, 17Oct13, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/list.h>
#include <linux/mempool.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_app.h>

#include "appprio.h"
#include "appidentify.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
unsigned char *g_appPriDftMap = NULL;

mempool_t     *g_priNode_pool = NULL;
/* temp hardcode */
unsigned int l_appWeb[] = {APP_ID_HTTP, APP_ID_HTTPS, APP_ID_SMTP, APP_ID_POP3, APP_ID_PPTP,
    APP_ID_L2TP, APP_ID_IPSEC, APP_ID_IMAP,
    APPPRIO_APP_ID_END};
unsigned int l_appVoice[] = {APP_ID_SKYPE, APP_ID_SIP, APP_ID_H323,
    APPPRIO_APP_ID_END};
unsigned int l_appIM[] = {APP_ID_QQ, APP_ID_WEBQQ, APP_ID_MSN, APP_ID_FETION, APP_ID_ALIIM, APP_ID_YY,
    APP_ID_TM,
    APPPRIO_APP_ID_END};
unsigned int l_appVideo[] = {APP_ID_YKTD, APP_ID_TXVIDEO, APP_ID_IQIYI, APP_ID_LETV, APP_ID_SOHUVIDEO,
    APP_ID_CNTV, APP_ID_VIDEO_ONLINE,
    APPPRIO_APP_ID_END};
unsigned int l_appP2PVideo[] = {APP_ID_QQLIVE, APP_ID_PPSTREAM, APP_ID_PPTV, APP_ID_PPTC, APP_ID_KUAIBO,
    APP_ID_FX, APP_ID_CLSF_PPS, APP_ID_CLSF_PPTV, APP_ID_CLSF_FLASHP2P,
    APPPRIO_APP_ID_END};
unsigned int l_appMusic[] = {APP_ID_KUGOU, APP_ID_KUWO, APP_ID_QQMUSIC, APP_ID_QQJT, APP_ID_CLSF_KUGOU,
    APPPRIO_APP_ID_END};
unsigned int l_appOther[] = {APP_ID_HTTPFD, APP_ID_FTP, APP_ID_XL_OTHERS,
    APPPRIO_APP_ID_END};
unsigned int l_appP2PDl[] = {APP_ID_THUNDER, APP_ID_XLLX, APP_ID_BITTORRENT, APP_ID_EMULE, APP_ID_QQXF,
    APP_ID_FLASHGET, APP_ID_CLSF_XL, APP_ID_CLSF_BT, APP_ID_CLSF_EMULE, APP_ID_CLSF_FLASHGET,
    APPPRIO_APP_ID_END};

APPPRIO_APP_MAP appIDMap[] = {
        {APP_ID_CLSF_XL, "xunlei"}, {APP_ID_CLSF_EMULE, "emule"}, {APP_ID_CLSF_PPS, "pps"},
        {APP_ID_CLSF_PPTV, "pptv"}, {APP_ID_CLSF_FLASHGET, "flashget"}, {APP_ID_CLSF_XF, "QQDownload"},
        {APP_ID_CLSF_QQMUSIC, "QQMusic"}, {APP_ID_CLSF_KUGOU, "kugou"}, {APP_ID_CLSF_VOICE, "voice"},
        {APP_ID_CLSF_BT, "BT"}
    };
/* temp hardcode pri */
APPPRIO_PRIQ *l_priorQueue = NULL;

APPPRIO_PRIO l_appprio_dftpri = APPPRIO_THIRD_PRIO;

int     l_isAppprioInit = FALSE;

#if  APPPRIO_CNT_DEBUG
int l_appprio_debug = TRUE;
unsigned int *l_debug_cnt_map = NULL;
unsigned int l_pri_cnt[4] = {0};
#endif

int l_appprio_check_debug = FALSE;
float   g_appprio_first_interval = 5.0f;
float   g_appprio_interval = 1.0f;
float   g_appprio_dl_threshold = 20.0f;
unsigned int  g_appprio_cnt_threshold = 11;
unsigned int  g_appprio_suspect_threshold = 11 / 2;


APPPRIO_PROFILE  l_appprio_profile;
int g_count_debug = FALSE;
int g_enablePrio = TRUE;

extern int g_appprio_version;
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
#if 1
static int
_appprio_primap_set(unsigned char *pMap, APPPRIO_PRIQ *pQueue, unsigned char pri)
{
    struct list_head  *pNode = NULL;
    APPPRIO_PRIQ *pSort = NULL;
    int         index = 0;

    if (pMap == NULL || pQueue == NULL)
    {
        APPPRIO_ERROR("primap set args NULL.");
        return -1;
    }

    list_for_each(pNode, &pQueue->list)
    {
        pSort = list_entry(pNode, APPPRIO_PRIQ, list);
        if (pSort->pSort != NULL)
        {
            for(index = 0; pSort->pSort[index] != APPPRIO_APP_ID_END; index++)
            {
                if (pMap[pSort->pSort[index]] != 0)
                {
                    APPPRIO_ERROR("!!!pMap %d elem had been set.\r\n", pSort->pSort[index]);
                }
                pMap[pSort->pSort[index]] = pri;
            }
        }
        else
        {
            APPPRIO_ERROR("primap pSort NULL.");
        }
    }

    return 0;
}
#endif
/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
int
appprio_readParaFromProfile(void)
{
    memset(&l_appprio_profile, 0, sizeof(l_appprio_profile));

    l_appprio_profile.insCnt = 1;
    l_appprio_profile.appmapSize = 4096;
    l_appprio_profile.dbMemSize = 64 * 1024;
    l_appprio_profile.appprioRuleMaxNum = 1;
    l_appprio_profile.appprioCustomappMaxNum = 32;

    return 0;
}

APPPRIO_PRIQ*
appprio_node_alloc(void)
{
    APPPRIO_PRIQ *pNode = NULL;

    if (NULL == g_priNode_pool)
    {
        APPPRIO_ERROR("kernel rule pool not init.");
        return NULL;
    }

    pNode = (APPPRIO_PRIQ *)mempool_alloc(g_priNode_pool, GFP_ATOMIC);
    if (NULL == pNode)
    {
        APPPRIO_ERROR("kernel entry alloc error.");
        return NULL;
    }
    memset((void *)pNode, 0, sizeof(APPPRIO_PRIQ));

    return pNode;
}

int appprio_node_free(APPPRIO_PRIQ *pNode)
{
    if (NULL == pNode)
    {
        APPPRIO_ERROR("args NULL.");
        return -1;
    }

    if (NULL == g_priNode_pool)
    {
        APPPRIO_ERROR("kernel rule pool not init.");
        return -1;
    }

    mempool_free((void *)pNode, g_priNode_pool);
    return 0;
}

#if 1
int
appprio_priq_insert(APPPRIO_PRIQ *priq, unsigned int *sort)
{
    APPPRIO_PRIQ *pNode = NULL;

    if (priq == NULL || sort == NULL)
    {
        APPPRIO_ERROR("priq insert args NULL.");
        return -1;
    }
    pNode = appprio_node_alloc();
    pNode->pSort = sort;

    list_add_tail(&pNode->list, &priq->list);

    return 0;
}

int
appprio_priq_free(void)
{
    int index;
    APPPRIO_PRIQ *pHead = NULL;
    APPPRIO_PRIQ *pNode = NULL;

    if (NULL == l_priorQueue)
    {
        APPPRIO_ERROR("l_priorQueue NULL.");
        return -1;
    }

    for (index = 0; index < APPPRIO_PRIQ_NUM; index ++)
    {
        pHead = &l_priorQueue[index];
        while (!pHead->list.next)
        {
            pNode = list_entry(pHead->list.next, APPPRIO_PRIQ, list);
            list_del(pHead->list.next);
            if (appprio_node_free(pNode))
            {
                APPPRIO_ERROR("free node error.");
                break;
            }
        }
    }

    return 0;
}

int
appprio_init_primap(unsigned char *pMap, APPPRIO_PRIQ *pQueueList)
{
    int qLevel = 0;
    APPPRIO_PRIQ *pQueue = NULL;

    if (pMap == NULL || pQueueList == NULL)
    {
        APPPRIO_ERROR("appprio_init_primap args NULL.");
        return -1;
    }
    for (qLevel = 0; qLevel < APPPRIO_PRIQ_NUM; qLevel++)
    {
        pQueue = &pQueueList[qLevel];
        if (_appprio_primap_set(pMap, pQueue, qLevel + 1))
        {
             APPPRIO_ERROR("_appprio_primap_set error.");
        }
    }
    return 0;
}
#endif

#if  APPPRIO_CNT_DEBUG
void appprio_cnt_reset(void)
{
    memset(l_debug_cnt_map, 0, APPPRIO_APP_MAP_SIZE * sizeof(unsigned int));
    memset(&l_pri_cnt[0], 0, 4 * sizeof(unsigned int));
}

void appprio_debug(void)
{
    int i;

    printk("------------pri cnt------------\r\n");
    for (i = 0; i < 4; i++)
    {
        printk("APP_PRI %u, conn cnt %u\r\n", i+1, l_pri_cnt[i]);
    }
    printk("------------app cnt------------\r\n");
    for(i = 0; i < APPPRIO_APP_MAP_SIZE; i++)
    {
        if (l_debug_cnt_map[i] != 0)
        {
            printk("APP_ID %-4u PRI %d pri conn cnt %u\r\n", i,
                       g_appPriDftMap[i], l_debug_cnt_map[i]);
        }
    }
    printk("------------app prio------------\r\n");
    for(i = 0; i < APPPRIO_APP_MAP_SIZE; i++)
    {
        if (g_appPriDftMap[i] != 0)
        {
            printk("APP_ID %-4u PRI %d\r\n", i, g_appPriDftMap[i]);
        }
    }
}

#if 0
void appprio_debug()
{
    int i = 0, j = 0;

    printf("--------customapp prio---------\r\n");
    appprio_customapp_showHashTbl();
    printf("-----appprio appId <--> pri-----\r\n");
    for(i = 0; i < APPPRIO_APP_MAP_SIZE; i++)
    {
        if (g_appPriDftMap[i] != 0)
        {
            char appName[APPPRIO_NAME_MAX_LEN] = {0};
            char appGbkBuff[512] = {0};
            int iDstLen = APPMNGR_TMP_SHORT_LEN;
            APPPRIO_SORT *pSort = IP_NULL;

            pSort = appprio_getAppInfoByIdFromDB(i, appName, APPPRIO_NAME_MAX_LEN);

            if (pSort != IP_NULL)
            {
                char appGbkBuffTmp[256] = {0};
                int iDstLenTmp = 128;
                appmngr_encodingUtf82Gbk(appName, ipcom_strlen(appName) + 1,
                                         appGbkBuff,&iDstLen);
                appmngr_encodingUtf82Gbk(pSort->sortName, ipcom_strlen(pSort->sortName) + 1,
                                         appGbkBuffTmp, &iDstLenTmp);
                printf("APP_ID %-4u APP_NAME %-15s PRI %-2u SORT %-8s\r\n",
                       i, appGbkBuff, g_appPriDftMap[i], appGbkBuffTmp);
            }
            else
            {
                if (i < 3500 || i > 3600)
                {
                    appmngr_getAppLocaleNameByAppId(i, appName);
                    appmngr_encodingUtf82Gbk(appName, ipcom_strlen(appName)+1, appGbkBuff,&iDstLen);
                }
                else
                {
                    for (j = 0; j < sizeof(appIDMap) / sizeof(APPPRIO_APP_MAP); j++)
                    {
                        if (appIDMap[j].appId == i)
                        {
                            strcpy(appGbkBuff, appIDMap[j].name);
                        }
                    }
                }
                printf("APP_ID %-4u APP_NAME %-15s PRI %-2u\r\n", i, appGbkBuff, g_appPriDftMap[i]);
            }
        }
    }
    printf("------------pri cnt------------\r\n");
    for (i = 0; i < 4; i++)
    {
        /*
        if (i + 1 == l_appprio_dftpri)
        {
            continue ;
        }*/
        printf("APP_PRI %u, conn cnt %u\r\n", i+1, l_pri_cnt[i]);
    }
    printf("------------app cnt------------\r\n");
    for(i = 0; i < APPPRIO_APP_MAP_SIZE; i++)
    {
        if (l_debug_cnt_map[i] != 0)
        {
            char appName[APPPRIO_NAME_MAX_LEN] = {0};
            char appGbkBuff[512] = {0};
            int iDstLen = APPMNGR_TMP_SHORT_LEN;
            APPPRIO_SORT *pSort = IP_NULL;

            pSort = appprio_getAppInfoByIdFromDB(i, appName, APPPRIO_NAME_MAX_LEN);

            if (pSort != IP_NULL)
            {
                char appGbkBuffTmp[256] = {0};
                int iDstLenTmp = 128;
                appmngr_encodingUtf82Gbk(appName, ipcom_strlen(appName) + 1,
                                         appGbkBuff,&iDstLen);
                appmngr_encodingUtf82Gbk(pSort->sortName, ipcom_strlen(pSort->sortName) + 1,
                                         appGbkBuffTmp, &iDstLenTmp);
                printf("APP_ID %-4u APP_NAME %-15s PRI %-2u SORT %-8s pri conn cnt %u\r\n",
                       i, appGbkBuff, g_appPriDftMap[i], appGbkBuffTmp, l_debug_cnt_map[i]);
            }
            else
            {
                if (i < 3500 || i > 3600)
                {
                    appmngr_getAppLocaleNameByAppId(i, appName);
                    appmngr_encodingUtf82Gbk(appName, ipcom_strlen(appName)+1, appGbkBuff,&iDstLen);
                }
                else
                {
                    for (j = 0; j < sizeof(appIDMap) / sizeof(APPPRIO_APP_MAP); j++)
                    {
                        if (appIDMap[j].appId == i)
                        {
                            strcpy(appGbkBuff, appIDMap[j].name);
                        }
                    }
                }
                printf("APP_ID %-4u APP_NAME %-15s PRI %d pri conn cnt %u\r\n", i, appGbkBuff,
                       g_appPriDftMap[i], l_debug_cnt_map[i]);
            }
        }
    }
}
#endif

#endif

int
appprio_prioResetDefault(void)
{
    l_isAppprioInit = FALSE;
    memset(g_appPriDftMap, 0, sizeof(unsigned char) * l_appprio_profile.appmapSize);

    appprio_priq_insert(&l_priorQueue[0], l_appWeb);
    appprio_priq_insert(&l_priorQueue[0], l_appVoice);
    appprio_priq_insert(&l_priorQueue[0], l_appIM);

    appprio_priq_insert(&l_priorQueue[1], l_appMusic);
    appprio_priq_insert(&l_priorQueue[1], l_appVideo);

    appprio_priq_insert(&l_priorQueue[2], l_appOther);

    appprio_priq_insert(&l_priorQueue[3], l_appP2PDl);
    appprio_priq_insert(&l_priorQueue[3], l_appP2PVideo);

    appprio_init_primap(g_appPriDftMap, l_priorQueue);
    l_isAppprioInit = TRUE;

    return 0;
}

#if 0

IP_PUBLIC STATUS
appprio_ruleConfigToKernel(APPPRIO_CFG_CUSTOM_POLICY *pPolicy)
{
    int     mapCnt  = 0;
    int     sortId  = 0;
    int     prioIdx = 0;
    int     bitIdx  = 0;

    if (IP_NULL == pPolicy)
    {
        APPPRIO_ERROR("param error IP_NULL == pPolicy.");
        return ERROR;
    }

    ipcom_memset(g_appPriDftMap, 0, sizeof(Ip_u8) * l_appprio_profile.appmapSize);
    for (prioIdx = 0; prioIdx < APPPRIO_PRIO_TOP - 1; prioIdx++)
    {
        for (mapCnt = 0; mapCnt < APPPRIO_PRIO_MAP_COUNT; mapCnt++)
        {
            if (!pPolicy->prioMap.appPrioMap[prioIdx][mapCnt])
            {
                continue;
            }
            for (bitIdx = 0; bitIdx < 32; bitIdx++)
            {
                if (pPolicy->prioMap.appPrioMap[prioIdx][mapCnt] & (1 << bitIdx))
                {
                    sortId = mapCnt * 32 + bitIdx;
                    if (sortId < APPPRIO_CUSTOMAPP_ID_BASE)
                    {
                        if (OK != appprio_initPrimapWithSort(g_appPriDftMap, sortId, prioIdx + 1))
                        {
                            APPPRIO_ERROR("appprio_initPrimapWithSort error.");
                            return ERROR;
                        }
                    }
                }
            }
        }
    }

    return OK;
}
#endif
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
unsigned int appprio_hook(unsigned int hook,
                 struct sk_buff *skb,
                 const struct net_device *in,
                 const struct net_device *out,
                 int (*okfn)(struct sk_buff *))
{
    struct nf_conn  *ct = NULL;
    unsigned int          dir  = IP_CT_DIR_MAX;
    short          appIdOri;
    short          appIdReply;
    short          normalAppOri;
    short          basicAppOri;
    short          normalAppReply;
    short          basicAppReply;
    enum ip_conntrack_info ctinfo;
    struct nf_conntrack_app *appinfo;
    struct iphdr            *iph;

    if (!g_enablePrio)
    {
        return NF_ACCEPT;
    }

    if (!l_isAppprioInit || !g_appPriDftMap)
    {
        return NF_ACCEPT;
    }

    ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct)
    {
        /*APPIDNTF_ERROR("no nf_conn.");*/
        return NF_ACCEPT;
    }

    iph = ip_hdr(skb);
    if (NULL == iph)
    {
        return NF_ACCEPT;
    }

    appinfo = nf_ct_get_app(ct);
    if(NULL == appinfo)
    {
        APPIDNTF_ERROR("no appidntf_info.");
        return NF_ACCEPT;
    }

    if (APPPRIO_PRI_GET(appinfo->appprio_flag) == APPPRIO_DEFAULT_PRIO)
    {
        if (IPPROTO_TCP == iph->protocol)
        {
            APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_THIRD_PRIO);
        }
        else if (IPPROTO_UDP == iph->protocol)
        {
            APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_FOURTH_PRIO);
        }
        else
        {
            APPPRIO_PRI_SET(appinfo->appprio_flag, APPPRIO_THIRD_PRIO);
        }
    }

    if (APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER) ||
        APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_PRI_SET))
    {
        if (g_count_debug)
        {
            struct nf_conn_counter *counter;
            unsigned int packetDirection;
            counter = nf_conn_acct_find(ct);
            if (NULL == counter)
            {
                APPIDNTF_ERROR("counter null.\r\n");
                return NF_ACCEPT;
            }
            packetDirection = CTINFO2DIR(ctinfo);
	#if 0		
            printk("PRIO SET OR LOCAL stream info:dir %s, counter %l, ori %x:%d---%x:%d, proto %s, reply %x:%d---%x:%d.\r\n",
                               packetDirection == IP_CT_DIR_ORIGINAL ? "ORI" : "REP",
                               counter[packetDirection].packets,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,
                               IPPROTO_TCP == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);
	#endif
		}
        if (!APPPRIO_FLAG_IS_SET(appinfo->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER) &&
            (appinfo->appprio_version != g_appprio_version))
        {
            appinfo->appprio_version = g_appprio_version;
            APPPRIO_FLAG_CLEAR(appinfo->appprio_flag, APPPRIO_FLAG_PRI_SET);
        }
        else
        {
            return NF_ACCEPT;
        }
    }

    if (IPPROTO_GRE == iph->protocol ||
        IPPROTO_ESP == iph->protocol ||
        IPPROTO_AH == iph->protocol)
    {
        APP_NORMAL_ID_SET(appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, APP_ID_PPTP);
        APP_NORMAL_ID_SET(appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, APP_ID_PPTP);
        APPPRIO_FLAG_SET(appinfo->appprio_flag, APPPRIO_FLAG_VPN);
        printk("vpn set over.\r\n");
    }

#if 0
    if (pkt->vpn_flag &&
        !APPPRIO_FLAG_IS_SET(pCtEntry->appprio_flag, APPPRIO_FLAG_PORTDEF))
    {
        if (pkt->vpn_flag < VPN_PROTO_MAX)
        {
            vpnIdx = pkt->vpn_flag;
            if (0 == vpnIdArr[0])
            {
                vpnIdArr[VPN_PROTO_PPTP]  = apppri_getAppIdByDesc("PPTP");
                vpnIdArr[VPN_PROTO_L2TP]  = apppri_getAppIdByDesc("L2TP");
                vpnIdArr[VPN_PROTO_IPSEC] = apppri_getAppIdByDesc("IPSec");
                vpnIdArr[0] = 1;
            }
            APP_NORMAL_ID_SET(pCtEntry->appdist_flag[CT_DIR_ORIGINAL].app_id_index, vpnIdArr[vpnIdx]);
            APP_NORMAL_ID_SET(pCtEntry->appdist_flag[CT_DIR_REPLY].app_id_index, vpnIdArr[vpnIdx]);
            APPPRIO_FLAG_SET(pCtEntry->appprio_flag, APPPRIO_FLAG_VPN);
            APPPRIO_DEBUG("vpn set over.");
        }
    }
#endif

    dir = CTINFO2DIR(ctinfo);
    appIdOri = appinfo->appidntfy_flag[dir].app_id_index;
    appIdReply = appinfo->appidntfy_flag[1 - dir].app_id_index;

    #if 0
    if (appIdOri != APP_ID_CHECKING || appIdReply != APP_ID_CHECKING)
    {
        APPPRIO_DEBUG("appIdOri %x, appIdReply %x.", appIdOri, appIdReply);
    }
    #endif

    if ((APP_BASIC_ID_FLAG_GET(appIdOri) != APP_BASIC_ID_CHECKING)
        || ((APP_NORMAL_ID_GET(appIdOri) != APP_NORMAL_ID_CHECKING)))
    {
        if ((APP_BASIC_ID_FLAG_GET(appIdOri) != APP_BASIC_ID_UNKNOWN)
            || ((APP_NORMAL_ID_GET(appIdOri) != APP_NORMAL_ID_UNKNOWN)))
        {
            basicAppOri     = APP_BASIC_ID_VALUE_GET(appIdOri);
            normalAppOri    = APP_NORMAL_ID_GET(appIdOri);
            basicAppReply   = APP_BASIC_ID_VALUE_GET(appIdReply);
            normalAppReply  = APP_NORMAL_ID_GET(appIdReply);

            if (normalAppOri > APPPRIO_APP_MAP_SIZE - 1 || normalAppOri < 0)
            {
                /*APPPRIO_DEBUG("appId get out of range %d.", normalAppOri);*/
            }
            else
            {
                if (normalAppOri == normalAppReply &&
                    g_appPriDftMap[normalAppOri])
                {
                    APPPRIO_PRI_SET(appinfo->appprio_flag, g_appPriDftMap[normalAppOri]);
                    APPPRIO_FLAG_SET(appinfo->appprio_flag, APPPRIO_FLAG_PRI_SET);

                    #if  APPPRIO_CNT_DEBUG
                    l_pri_cnt[g_appPriDftMap[normalAppOri] - 1]++;
                    l_debug_cnt_map[normalAppOri]++;


                    if (l_appprio_debug)
                    {
                        APPPRIO_DEBUG("normal app stream pri set: appId %u, app_id_index %x, pri %d, prio_flag %08x.",
                                     normalAppOri,
                                     appinfo->appidntfy_flag[dir].app_id_index,
                                     APPPRIO_PRI_GET(appinfo->appprio_flag),
                                     appinfo->appprio_flag);

                        APPPRIO_DEBUG("normal stream info: ori %x:%d---%x:%d, proto %s, reply %x:%d---%x:%d.",
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,
                               IPPROTO_TCP == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);
                    }
                    #endif
                    return NF_ACCEPT;
                }
            }

            if (IS_BASIC_APP(basicAppOri))
            {
                if (basicAppOri == basicAppReply &&
                    g_appPriDftMap[basicAppOri])
                {
                    APPPRIO_PRI_SET(appinfo->appprio_flag, g_appPriDftMap[basicAppOri]);
                    APPPRIO_FLAG_SET(appinfo->appprio_flag, APPPRIO_FLAG_PRI_SET);

                    #if  APPPRIO_CNT_DEBUG
                    l_pri_cnt[g_appPriDftMap[basicAppOri] - 1]++;
                    l_debug_cnt_map[basicAppOri]++;
                    #endif
                }
                #if  APPPRIO_CNT_DEBUG
                if(l_appprio_debug)
                {
                    APPPRIO_DEBUG("basic app stream pri set: appId %u, app_id_index %x, pri %d, prio_flag %08x.",
                                 basicAppOri,
                                 appinfo->appidntfy_flag[dir].app_id_index,
                                 APPPRIO_PRI_GET(appinfo->appprio_flag),
                                 appinfo->appprio_flag);

                        APPPRIO_DEBUG("basic stream info: ori %x:%d---%x:%d, proto %s, reply %x:%d---%x:%d.",
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,
                               IPPROTO_TCP == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip,
                               ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);
                }
                #endif
                return NF_ACCEPT;
            }
        }
        else
        {
            if (g_count_debug)
            {
                struct nf_conn_counter *counter;
                unsigned int packetDirection;
                counter = nf_conn_acct_find(ct);
                if (NULL == counter)
                {
                    APPIDNTF_ERROR("counter null.\r\n");
                    return NF_ACCEPT;
                }
                packetDirection = CTINFO2DIR(ctinfo);
				#if 0
                printk("UNKNOWN stream info:dir %s, counter %llu, ori %x:%d---%x:%d, proto %s, reply %x:%d---%x:%d.\r\n",
                                   packetDirection == IP_CT_DIR_ORIGINAL ? "ORI" : "REP",
                                   counter[packetDirection].packets,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,
                                   IPPROTO_TCP == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all,
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);
				#endif
			}
        }
    }
    else
    {
            if (g_count_debug)
            {
                struct nf_conn_counter *counter;
                unsigned int packetDirection;
                counter = nf_conn_acct_find(ct);
                if (NULL == counter)
                {
                    APPIDNTF_ERROR("counter null.\r\n");
                    return NF_ACCEPT;
                }
                packetDirection = CTINFO2DIR(ctinfo);
				#if 0
                printk("CHECKING stream info:dir %s, counter %llu, ori %x:%d---%x:%d, proto %s, reply %x:%d---%x:%d.\r\n",
                                   packetDirection == IP_CT_DIR_ORIGINAL ? "ORI" : "REP",
                                   counter[packetDirection].packets,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,
                                   IPPROTO_TCP == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all,
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip,
                                   ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);
				#endif
			}
    }

    return NF_ACCEPT;
}

int appprio_update_db(void * pAppMap, int mapSize, int len)
{
    int ret = 0;

    if (len < sizeof(unsigned char) * mapSize)
    {
        APPPRIO_ERROR("the msg recv is too short\n");
        return -1;
    }

    l_isAppprioInit = FALSE;


    if(g_appPriDftMap)
    {
        kfree(g_appPriDftMap);
        g_appPriDftMap = NULL;
    }

    g_appPriDftMap = (unsigned char *)kmalloc(mapSize * sizeof(unsigned char), GFP_ATOMIC);
    if (NULL == g_appPriDftMap)
    {
        ret = -1;
        APPPRIO_ERROR("g_appPriTbl mem kmalloc error.");
        goto  leave;
    }

    l_appprio_profile.appmapSize = mapSize;
    memset(g_appPriDftMap, 0, l_appprio_profile.appmapSize * sizeof(unsigned char));

    memcpy(g_appPriDftMap, pAppMap, l_appprio_profile.appmapSize * sizeof(unsigned char));

    //appprio_debug();

leave:
    l_isAppprioInit = TRUE;
    return ret;

}

int
appprio_init(void)
{
    int     ret = 0;
    unsigned int  index = 0;
    APPPRIO_DEBUG("");
    if (appprio_readParaFromProfile())
    {
        ret = -1;
        APPPRIO_ERROR("read profile error.");
        goto  leave;
    }

    g_appPriDftMap = (unsigned char *)kmalloc(l_appprio_profile.appmapSize * sizeof(unsigned char), GFP_KERNEL);
    if (NULL == g_appPriDftMap)
    {
        ret = -ENOMEM;
        APPPRIO_ERROR("g_appPriTbl mem kmalloc error.");
        goto  leave;
    }
    memset(g_appPriDftMap, 0, l_appprio_profile.appmapSize * sizeof(unsigned char));

#if 1
    l_priorQueue = kmalloc(APPPRIO_PRIQ_NUM * sizeof(APPPRIO_PRIQ), GFP_KERNEL);
    if (NULL == l_priorQueue)
    {
        ret = -ENOMEM;
        APPPRIO_ERROR("l_priorQueue mem malloc error.");
        goto leave;
    }
    memset(l_priorQueue, 0, APPPRIO_PRIQ_NUM * sizeof(APPPRIO_PRIQ));

    for (index = 0; index < APPPRIO_PRIQ_NUM; index++)
    {
        INIT_LIST_HEAD(&l_priorQueue[index].list);
    }

    APPPRIO_DEBUG("");
    g_priNode_pool = mempool_create_kmalloc_pool(APPPRIO_PRIQ_POOL_SIZE, sizeof(APPPRIO_PRIQ));
    if (NULL == g_priNode_pool)
    {
        APPIDNTF_ERROR("mempool create error.");
        ret = -ENOMEM;
        goto leave;
    }
#endif

#if 0
    if (OK != (ret = appprio_database_init()))
    {
        ret = ERROR;
        APPPRIO_ERROR("appprio_database_init error.");
        goto leave;
    }
#endif
    APPPRIO_DEBUG("");
    #if 1
    /* temp hardcode */
    appprio_priq_insert(&l_priorQueue[0], l_appWeb);
    appprio_priq_insert(&l_priorQueue[0], l_appVoice);
    appprio_priq_insert(&l_priorQueue[0], l_appIM);

    appprio_priq_insert(&l_priorQueue[1], l_appMusic);
    appprio_priq_insert(&l_priorQueue[1], l_appVideo);

    appprio_priq_insert(&l_priorQueue[2], l_appOther);

    appprio_priq_insert(&l_priorQueue[3], l_appP2PDl);
    appprio_priq_insert(&l_priorQueue[3], l_appP2PVideo);

    appprio_init_primap(g_appPriDftMap, l_priorQueue);
    #endif
/*
    if (OK != appprio_initPrimapFromDB(g_appPriDftMap))
    {
        ret = ERROR;
        APPPRIO_ERROR("appprio_initPrimapFromDB error.");
        goto leave;
    }
*/
#if 0
    if (OK != appprio_customapp_init())
    {
        ret = ERROR;
        APPPRIO_ERROR("appprio_customapp_init error.");
        goto leave;
    }
#endif
    APPPRIO_DEBUG("");
    l_isAppprioInit = TRUE;


#if  APPPRIO_CNT_DEBUG
    l_debug_cnt_map = (unsigned int *)kmalloc(l_appprio_profile.appmapSize * sizeof(unsigned int), GFP_KERNEL);
    memset(l_debug_cnt_map, 0, l_appprio_profile.appmapSize * sizeof(unsigned int));
#endif

leave:
    return ret;
}

int
appprio_exit(void)
{
    APPPRIO_DEBUG("");

#if 0
    if (appprio_priq_free())
    {
        APPPRIO_ERROR("appprio_priq_free error.");
        return -1;
    }

    if (l_priorQueue)
    {
        kfree(l_priorQueue);
    }
#endif

    if (g_appPriDftMap)
    {
        kfree(g_appPriDftMap);
        g_appPriDftMap = NULL;
    }

    if (l_debug_cnt_map)
    {
        kfree(l_debug_cnt_map);
        l_debug_cnt_map = NULL;
    }

    if (g_priNode_pool)
    {
        mempool_destroy(g_priNode_pool);
    }
    APPPRIO_DEBUG("");
    return 0;
}

#if 0
IP_GLOBAL int
appprio_statistics_cb(CT_ENTRY *ct)
{
    const float first_interval = CT_TCP_STATISTICS_FIRST_INTERVAL;
    const float interval = CT_TCP_STATISTICS_INTERVAL;

    ipnet_timeout_cancel(ct->cntTmo);

    if (APP_BASIC_ID_VALUE_GET(ct->appdist_flag[CT_DIR_ORIGINAL].app_id_index) != APP_ID_HTTP ||
        APP_NORMAL_ID_GET(ct->appdist_flag[CT_DIR_ORIGINAL].app_id_index) != APP_NORMAL_ID_UNKNOWN)
    {
        APPPRIO_CHECK_DEBUG("not http ret basicId %d, normalId %d\r\n"
                       "original %x:%d---%x:%d, proto %s. reply %x:%d---%x:%d ct %p",
                   APP_BASIC_ID_VALUE_GET(ct->appdist_flag[CT_DIR_ORIGINAL].app_id_index),
                   APP_NORMAL_ID_GET(ct->appdist_flag[CT_DIR_ORIGINAL].app_id_index),
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.src.addr.ip4,
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.src.tsp.all,
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.dst.addr.ip4,
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.dst.tsp.all,
                   IP_IPPROTO_TCP == ct->tuplehash[CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                   ct->tuplehash[CT_DIR_REPLY].tuple.src.addr.ip4,
                   ct->tuplehash[CT_DIR_REPLY].tuple.src.tsp.all,
                   ct->tuplehash[CT_DIR_REPLY].tuple.dst.addr.ip4,
                   ct->tuplehash[CT_DIR_REPLY].tuple.dst.tsp.all,
                    ct);
        if (APP_NORMAL_ID_GET(ct->appdist_flag[CT_DIR_ORIGINAL].app_id_index) == APP_ID_HTTPFD)
        {
            APPPRIO_CHECK_DEBUG("first check http, then normal check http fd.\n");
        }
        return 0;
    }

    if (ct->replyRate != 0)
    {
        ct->replyRate = ct->replyBytes / interval / 1024.0f;
    }
    else
    {
        ct->replyRate = ct->replyBytes / first_interval / 1024.0f;
    }

    if (!APPPRIO_CNT_GET(ct->statisticsCnt, 0))
    {

    }
    else
    {
        if (ct->replyRate > APPPRIO_HTTP_DL_THRESHOLD)
        {
            APPPRIO_CNT_INCREASE(ct->statisticsCnt, 8);
        }

    }
    APPPRIO_CNT_INCREASE(ct->statisticsCnt, 0);
    /*ct->statisticsCnt++;*/

    if (APPPRIO_CNT_GET(ct->statisticsCnt, 0) == APPPRIO_CNT_THRESHOLD)
    {
        if (APPPRIO_CNT_GET(ct->statisticsCnt, 8) >= APPPRIO_SUSPECT_THRESHOLD)
        {
            APP_NORMAL_ID_SET(ct->appdist_flag[CT_DIR_ORIGINAL].app_id_index, APP_ID_HTTPFD);
            APP_NORMAL_ID_SET(ct->appdist_flag[CT_DIR_REPLY].app_id_index, APP_ID_HTTPFD);
            APPPRIO_FLAG_CLEAR(ct->appprio_flag, APPPRIO_FLAG_PRI_SET);
            APPPRIO_CHECK_DEBUG("suspect http download.");
        }
        APPPRIO_CHECK_DEBUG("check over.");
        return 0;
    }
    APPPRIO_CHECK_DEBUG("original %x:%d---%x:%d, proto %s. reply %x:%d---%x:%d  ct %p",
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.src.addr.ip4,
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.src.tsp.all,
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.dst.addr.ip4,
                   ct->tuplehash[CT_DIR_ORIGINAL].tuple.dst.tsp.all,
                   IP_IPPROTO_TCP == ct->tuplehash[CT_DIR_ORIGINAL].tuple.dst.protonum ? "TCP":"UDP",
                   ct->tuplehash[CT_DIR_REPLY].tuple.src.addr.ip4,
                   ct->tuplehash[CT_DIR_REPLY].tuple.src.tsp.all,
                   ct->tuplehash[CT_DIR_REPLY].tuple.dst.addr.ip4,
                   ct->tuplehash[CT_DIR_REPLY].tuple.dst.tsp.all,
                    ct);

    APPPRIO_CHECK_DEBUG("HTTP connection reply bytes:%u, rate %.2fKB/s, beyond limit cnt %d, total cnt %d",
                 ct->replyBytes, ct->replyRate,
                 APPPRIO_CNT_GET(ct->statisticsCnt, 8),
                 APPPRIO_CNT_GET(ct->statisticsCnt, 0));
    ct->replyBytes = 0;
    ipnet_timeout_schedule(interval * 1000, appprio_statistics_cb, ct, &ct->cntTmo);

    return 0;
}
#endif
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
