/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_hash_table.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     29Sep13
 *
 *\warning
 *
 *\history \arg 0.0.1, 29Sep13, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/types.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/mempool.h>

#include "appidentify.h"
#include "appidentify_id.h"
#include "appidentify_rules.h"
#include "appidentify_node.h"
#include "appidentify_dpi_engine.h"
#include "appidentify_hash_table.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define HASH_DEBUG      APPIDNTF_DEBUG

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
extern mempool_t             *l_kernel_rule_pool;
extern APPIDNTFY_DPI_ENGINE  appidentify_dpi_pcre;
extern APPIDNTFY_HEAD       (*hash_sub_tables)[APPIDNTFY_TUPLE_INDEX_MAX][APPIDNTFY_HASH_TABLE_LEN];
extern FP_HANDLE_INFO       appidentify_hardCodeHandles[];
/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
void*
_appidentify_search_hardCodeHandle(int appID, APPIDNTFY_PKT_TUPLE* tuple, int proto)
{
    int index = 0;

    /* check params */
    if ((appID < APPMNGR_APPID_NORMAL_BEGIN) || (appID > APPMNGR_APPID_BASIC_END) ||
        (proto < 0) || (proto > IPPROTO_MAX) ||
        (NULL == tuple))
    {
        APPIDNTF_ERROR("invalid parameters.appId = %d\n\r", appID);

        return NULL;
    }

    for (; appidentify_hardCodeHandles[index].appId != APP_ID_UNKNOWN; index ++)
    {
        if ((appID == appidentify_hardCodeHandles[index].appId) &&
            (!memcmp(tuple, &appidentify_hardCodeHandles[index].tuple, sizeof(APPIDNTFY_PKT_TUPLE))) &&
            (proto == appidentify_hardCodeHandles[index].proto))
        {
            return &appidentify_hardCodeHandles[index];
        }
    }

    return NULL;
}
/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
APPIDNTFY_KER_RULE*
appidentify_kernel_entry_alloc()
{
    APPIDNTFY_KER_RULE *pKernelEntry = NULL;

    if (NULL == l_kernel_rule_pool)
    {
        APPIDNTF_ERROR("kernel rule pool not init.\r\n");
        return NULL;
    }

    pKernelEntry = (APPIDNTFY_KER_RULE *)mempool_alloc(l_kernel_rule_pool, GFP_ATOMIC);
    if (NULL == pKernelEntry)
    {
        APPIDNTF_ERROR("kernel entry alloc error.\r\n");
        return NULL;
    }
    memset((void *)pKernelEntry, 0, sizeof(APPIDNTFY_KER_RULE));

    return pKernelEntry;
}

int appidentify_kernel_entry_free(APPIDNTFY_KER_RULE *pKernelEntry)
{
    if (NULL == pKernelEntry)
    {
        APPIDNTF_ERROR("args NULL.\r\n");
        return -1;
    }

    if (NULL == l_kernel_rule_pool)
    {
        APPIDNTF_ERROR("kernel rule pool not init.\r\n");
        return -1;
    }

    mempool_free((void *)pKernelEntry, l_kernel_rule_pool);
    return 0;
}

/*!
*\fn           Ip_bool appidentify_hash_add( int iTupleIndex,
*                       APPIDNTFY_NODE *hash_table, APPIDNTFY_NODE *hash_entry )
*\brief        Add a hash entry here.
*\detail
*
*\param[in] iTupleIndex ,
*\param[in] hash_table , Hash table to manager the rules.
*\param[in] hash_entry
*
*\return       The result of the appidentify_hash_add
*\retval       IP_TRUE
*\retval       IP_FALSE
*
*\note
*/
int appidentify_hash_add(int  index, APPIDNTFY_HEAD *hash_table, APPIDNTFY_NODE *hash_entry)
{

    APPIDNTFY_KER_RULE   *pNextEntry      = NULL;
    APPIDNTFY_KER_RULE   *pEntry          = (APPIDNTFY_KER_RULE *)(hash_entry->parent);
    APPIDNTFY_HEAD       *pListHeader     = NULL;
    APPIDNTFY_NODE       *pListNext       = NULL;

    int                 hashKey         = 0;
    APPIDNTFY_TUPLE_INDEX tupleIndex      = index;

    /*!
    *  确定哈希索引KEY值.
    *
    *  [4/25/2010]
    */
    APPIDNTF_DEBUG("\r\n");
    if (NULL == pEntry)
    {
        APPIDNTF_ERROR("pEntry is NULL.\r\n");
        return -1;
    }
    switch (tupleIndex)
    {
    case  APPIDNTFY_SRC_IP:
        hashKey = HASH_KEY_IP(pEntry->tuple.srcip);
        break;
    case  APPIDNTFY_SRC_PORT:
        hashKey = HASH_KEY_PORT(pEntry->tuple.srcport);
        break;
    case  APPIDNTFY_DST_IP:
        hashKey = HASH_KEY_IP(pEntry->tuple.dstip);
        break;
    case  APPIDNTFY_DST_PORT:
        hashKey = HASH_KEY_PORT(pEntry->tuple.dstport);
        break;

        /*
         *  原设计是把APPIDNTFY_BASIC_APP的处理方式与APPIDNTFY_DST_PORT，但是基础应用不基于端口，因此，出现了
         *  无法识别的情况。现用0作为其哈希值。对基础应用进行独立的处理。
         *  其相关的代码详见appidentify_distinguish_hook的代码段
         *
         *  By Peng Xiaochuang, 30Sep11
         */

    case APPIDNTFY_BASIC_APP:
        hashKey = 0;
        break;
    case APPIDNTFY_RANDOM_PORT:
        hashKey = -1;
        break;
    default:
        APPIDNTF_DEBUG("unknown error!\r\n");
        break;
    }
    APPIDNTF_DEBUG("hashKey is %08x.\r\n", hashKey);

    /*!
    *  定位至哈希表的节点.
    *
    *  [4/25/2010]
    */
    if (hashKey < 0)
    {
        pListHeader = hash_table;
    }
    else
    {

        /*!
        *  会出现第一个添加的节点的时候,哈希表里的信息的SIZE为0.
        *
        *  [4/25/2010]
        */
        pListHeader = &hash_table[hashKey];
    }

    pListNext = hlist_entry(pListHeader->head.first, APPIDNTFY_NODE, node);


    while (1)
    {
        /*!
        *  若当前规则是第一个插入的值,或者是count最小的一个,则需要添加到链表的最后.
        *
        *  [4/25/2010]
        */
        if (NULL == pListNext)
        {
            if (!hlist_insert_tail(&(pListHeader->head),
                &(pEntry->hashNode[tupleIndex].node)))
            {
                pListNext = &(pEntry->hashNode[tupleIndex]);
            }
            else
            {
                pListNext = NULL;
            }
            HASH_DEBUG("HASH ADD LAST\r\n");
            break;
        }

        pNextEntry = (APPIDNTFY_KER_RULE *)pListNext->parent;
        /*!
        *  新加的条目的个数大于或者等于当前值,则插入到其前面.同时结束插入操作.
        *
        *  [4/25/2010]
        */
        if (pNextEntry)
        {
            if (pEntry->count >= pNextEntry->count)
            {
                hlist_add_before(&(pEntry->hashNode[tupleIndex].node), &(pListNext->node));
                HASH_DEBUG("HASH INSERT IN MIDDLE\r\n");
                break;
            }
        }

        pListNext = hlist_entry(pListNext->node.next, APPIDNTFY_NODE, node);
    }

    /*!
    *  如果pListNext为空,说明没有插入成功了.
    *
    *  [4/25/2010]
    */
    if (NULL == pListNext)
    {
        APPIDNTF_DEBUG("\r\n");
        return -1;
    }
    else
    {
        APPIDNTF_DEBUG("\r\n");
        return 0;
    }
}

/*!
 *\fn       STATUS appidentify_add_kernal_to_hash(APPIDNTFY_KER_RULE* rule)
 *\brief    把核心规则添加到hash表中
 *\details
 *
 *\param[in]
 *\param[out]
 *
 *\return
 *\retval
 *
 *\note     多掩码的规则不必插入到每一个掩码对应的hash表中，但是需要注意增加、删除
 *          应保持一致，并且最好与搜索时顺序一致。
 */
int
appidentify_add_rule_to_kerHash(APPIDNTFY_KER_RULE* rule)
{
    int ret = 0;
    APPIDNTFY_SUBTABLE_INDEX      subTableIndex   = APPIDNTFY_SUBTABLE_MAX;

    if (NULL == rule || APP_ID_UNKNOWN == rule->appId)
    {
        return -1;
    }

    switch (rule->protocol)
    {
    case IPPROTO_TCP:
        subTableIndex = APPIDNTFY_TCP_SUBTABLE;
        break;
    case  IPPROTO_UDP:
        subTableIndex = APPIDNTFY_UDP_SUBTABLE;
        break;
    default:
        APPIDNTF_ERROR("Unsupported protocol: %d\r\n", rule->protocol);
        return -1;
    }

    APPIDNTF_DEBUG("\r\n");
    /*!
    *   根据已有的TUPLE信息,计算MASK与COUNT
    *
    *  [4/25/2010]
    */
    rule->mask = 0;

    /*!
    *   需要在插入hash之前计算，保证优先序
    *
    *  [8/10/2010]
    */
    rule->count = !!(rule->tuple.srcip)
        + !!(rule->tuple.srcport)
        + !!(rule->tuple.dstip)
        + !!(rule->tuple.dstport);

    /*!
    *   调整插入顺序，并在插入成功后返回，目的是避免多掩码的规则重复插入hash表。
    *   NOTE: 顺序应与删除顺序一致，并尽量符合搜索的顺序。
    *
    *   By lvwg [1/11/2010]
    */

    /* 如果是基础应用，则不能去匹配固定端口的规则 */
    if(IS_BASIC_APP(rule->appId))
    {
        APPIDNTF_DEBUG("\r\n");
        rule->mask |= APPIDNTFY_MASK_BASIC_APP;
        rule->hashNode[APPIDNTFY_BASIC_APP].parent = rule;
    }
    else
    {
        if (rule->tuple.dstport)
        {
            APPIDNTF_DEBUG("\r\n");
            rule->mask |= APPIDNTFY_MASK_DST_PORT;
            rule->hashNode[APPIDNTFY_DST_PORT].parent = rule;
        }
        else
        {
            APPIDNTF_DEBUG("\r\n");
            rule->mask |= APPIDNTFY_MASK_RANDOM_PORT;
            rule->hashNode[APPIDNTFY_RANDOM_PORT].parent = rule;
        }


        if (rule->tuple.dstip)
        {
            APPIDNTF_DEBUG("\r\n");
            rule->mask |= APPIDNTFY_MASK_DST_IP;
            rule->hashNode[APPIDNTFY_DST_IP].parent = rule;
        }


        if (rule->tuple.srcport)
        {
            APPIDNTF_DEBUG("\r\n");
            rule->mask |= APPIDNTFY_MASK_SRC_PORT;
            rule->hashNode[APPIDNTFY_SRC_PORT].parent = rule;
        }

        if (rule->tuple.srcip)
        {
            APPIDNTF_DEBUG("\r\n");
            rule->mask |= APPIDNTFY_MASK_SRC_IP;
            rule->hashNode[APPIDNTFY_SRC_IP].parent = rule;
        }
    }

    if (APPIDNTFY_MASK_BASIC_APP == (rule->mask & APPIDNTFY_MASK_BASIC_APP))
    {
        APPIDNTF_DEBUG("Add dpi rules: APPIDNTFY_BASIC_APP\r\n");
        ret = appidentify_hash_add(APPIDNTFY_BASIC_APP,
                                hash_sub_tables[subTableIndex][APPIDNTFY_BASIC_APP],
                                &rule->hashNode[APPIDNTFY_BASIC_APP]);
    }

    if (APPIDNTFY_MASK_DST_PORT == (rule->mask & APPIDNTFY_MASK_DST_PORT))
    {
        APPIDNTF_DEBUG("Add dpi rules: APPIDNTFY_DST_PORT\r\n");
        ret = appidentify_hash_add(APPIDNTFY_DST_PORT,
                                hash_sub_tables[subTableIndex][APPIDNTFY_DST_PORT],
                                &rule->hashNode[APPIDNTFY_DST_PORT]);
    }

    if (APPIDNTFY_MASK_RANDOM_PORT == (rule->mask & APPIDNTFY_MASK_RANDOM_PORT))
    {
        APPIDNTF_DEBUG("Add dpi rules: APPIDNTFY_RANDOM_PORT\r\n");
        ret = appidentify_hash_add(APPIDNTFY_RANDOM_PORT,
                                hash_sub_tables[subTableIndex][APPIDNTFY_RANDOM_PORT],
                                &rule->hashNode[APPIDNTFY_RANDOM_PORT]);

    }

    if (APPIDNTFY_MASK_DST_IP == (rule->mask & APPIDNTFY_MASK_DST_IP))
    {
        APPIDNTF_DEBUG("Add dpi rules: APPIDNTFY_DST_IP\r\n");
        ret = appidentify_hash_add(APPIDNTFY_DST_IP,
                                hash_sub_tables[subTableIndex][APPIDNTFY_DST_IP],
                                &rule->hashNode[APPIDNTFY_DST_IP]);
    }

    if (APPIDNTFY_MASK_SRC_PORT == (rule->mask & APPIDNTFY_MASK_SRC_PORT))
    {
        APPIDNTF_DEBUG("Add dpi rules: APPIDNTFY_SRC_PORT\r\n");
        ret = appidentify_hash_add(APPIDNTFY_SRC_PORT,
                                hash_sub_tables[subTableIndex][APPIDNTFY_SRC_PORT],
                                &rule->hashNode[APPIDNTFY_SRC_PORT]);
    }

    if (APPIDNTFY_MASK_SRC_IP == (rule->mask & APPIDNTFY_MASK_SRC_IP))
    {
        APPIDNTF_DEBUG("Add dpi rules: APPIDNTFY_SRC_IP\r\n");
        ret = appidentify_hash_add(APPIDNTFY_SRC_IP,
                               hash_sub_tables[subTableIndex][APPIDNTFY_SRC_IP],
                               &rule->hashNode[APPIDNTFY_SRC_IP]);
    }

    return ret;
}

/*!
 *\fn           appidentify_add_new_rule(APPMNGR_RULE *pAppRule, int ruleNum)
 *\brief        从数据库中导入规则
 *\detail
 *
 *\param[in]    APPMNGR_RULE *pAppRule
 *              int ruleNum
 *\param[out]   N/A
 *
 *\return       the result of the operation
 *\retval       OK
 *              ERROR
 *\note
 */
int
appidentify_add_new_rule(APPMNGR_RULE *pAppRule, int ruleNum)
{
    APPIDNTFY_KER_RULE* kRule = NULL;
    int i;
    int ret = -1;
    enum ip_conntrack_dir dir = IP_CT_DIR_ORIGINAL;
//    FP_HANDLE_INFO  *info = NULL;

    APPIDNTF_DEBUG("in.\r\n");

	printk("Add new rules: ruleNum = %d\r\n", ruleNum);

    for (i = 0;i < ruleNum; i++)
    {
        kRule = appidentify_kernel_entry_alloc();
        if (NULL == kRule)
        {
            ret = -1;
            APPIDNTF_ERROR("appidentify_kernel_entry_alloc error.\r\n");
            break;
        }
        memset(kRule, 0, sizeof(APPIDNTFY_KER_RULE));

        kRule->appId = pAppRule[i].id;
        kRule->protocol = pAppRule[i].tuple.proto;

        kRule->tuple.srcip = htonl(pAppRule[i].tuple.srcip);
        kRule->tuple.dstip = htonl(pAppRule[i].tuple.dstip);
        kRule->tuple.srcport = htonl(pAppRule[i].tuple.srcport);
        kRule->tuple.dstport = htonl(pAppRule[i].tuple.dstport);

        for (dir = IP_CT_DIR_ORIGINAL; dir < IP_CT_DIR_MAX; dir++)
        {
            kRule->feature[dir].enable = pAppRule[i].feature[dir].enable;
            kRule->feature[dir].pkt_start = pAppRule[i].feature[dir].pkt_start;
            kRule->feature[dir].pkt_end = pAppRule[i].feature[dir].pkt_end;
            kRule->feature[dir].packetLen = pAppRule[i].feature[dir].packetLen;


            if (0 == memcmp(pAppRule[i].feature[dir].dpiType, "pcre", strlen("pcre")))
            {
                kRule->feature[dir].pDpiCode = appidentify_dpi_pcre.parse(pAppRule[i].feature[dir].dpiCode);
                kRule->feature[dir].fpHandle = appidentify_dpi_pcre.inspect;

                APPIDNTF_DEBUG("pDipCode = %p,kRule->appid = %d\n\r", kRule->feature[dir].pDpiCode, kRule->appId);
            }
            else
            {
                kRule->feature[dir].fpHandle = NULL;
                kRule->feature[dir].pDpiCode = NULL;
                APPIDNTF_DEBUG("Only support pcre engine: %s:dir = %d, i = %d\r\n", pAppRule[i].feature[dir].dpiType, dir, i);
            }
        }

        kRule->feature[IP_CT_DIR_ORIGINAL].hardCodeHandle = NULL;
        kRule->feature[IP_CT_DIR_REPLY].hardCodeHandle = NULL;
#if 0 /* temp not add */
        if ((APP_ID_BITTORRENT != kRule->appId) || (NULL == kRule->feature[IP_CT_DIR_ORIGINAL].fpHandle))
        {
            info = (FP_HANDLE_INFO*)_appidentify_search_hardCodeHandle(kRule->appId, &kRule->tuple, kRule->protocol);
            if (NULL != info)
            {
                if (kRule->feature[IP_CT_DIR_ORIGINAL].enable)
                {
                    kRule->feature[IP_CT_DIR_ORIGINAL].hardCodeHandle = info->hard_code_ptr[IP_CT_DIR_ORIGINAL];
                }

                if (kRule->feature[IP_CT_DIR_REPLY].enable)
                {
                    kRule->feature[IP_CT_DIR_REPLY].hardCodeHandle = info->hard_code_ptr[IP_CT_DIR_REPLY];
                }
            }
        }
#endif
        APPIDNTF_DEBUG("in.\r\n");
        if ((ret = appidentify_add_rule_to_kerHash(kRule)))
        {
            APPIDNTF_DEBUG("Hash add failed.\r\n");
            kRule->appId = APP_ID_UNKNOWN;
            break;
        }

        APPIDNTF_DEBUG("rule %d register success\n\r",i);

		printk("\tAdd appid = %d\r\n", kRule->appId);
    }

    return 0;
}

int appidentify_hash_del(APPIDNTFY_HEAD *hash_head)
{
    APPIDNTFY_NODE *pNodeEntry;
    APPIDNTFY_NODE *pNodeNext;
    APPIDNTFY_KER_RULE *kRule;
    int             index, tupleIdx;
    int             ret;

    if (NULL == hash_head)
    {
        APPIDNTF_ERROR("args head NULL.\r\n");
        return -1;
    }

    if (!hash_head->head.first)
    {
        return 0;
    }
    pNodeEntry = hlist_entry(hash_head->head.first, APPIDNTFY_NODE, node);
    pNodeNext  = pNodeEntry;

    while (pNodeNext)
    {
        kRule = (APPIDNTFY_KER_RULE *)pNodeNext->parent;
        pNodeNext = hlist_entry(pNodeNext->node.next, APPIDNTFY_NODE, node);
        if (kRule)
        {
            for (index = IP_CT_DIR_ORIGINAL; index < IP_CT_DIR_MAX; index ++)
            {
                appidentify_pcre_free(kRule->feature[index].pDpiCode);
            }

            for (tupleIdx = APPIDNTFY_SRC_IP; tupleIdx < APPIDNTFY_TUPLE_INDEX_MAX; tupleIdx ++)
            {
                if (kRule->hashNode[tupleIdx].parent)
                {
                    APPIDNTF_DEBUG("tuple %d del before.\r\n", tupleIdx);
                    hlist_del(&(kRule->hashNode[tupleIdx].node));
                    APPIDNTF_DEBUG("tuple %d del over.\r\n", tupleIdx);
                }
            }
            if ((ret = appidentify_kernel_entry_free(kRule)))
            {
                APPIDNTF_ERROR("APPIDNTFY_KER_RULE free error.\r\n");
                return ret;
            }
        }
    }

    return 0;
}

int
appidentify_cleanup_rules(void)
{
    int subTableIdx, hashIdx, idx;
    APPIDNTFY_HEAD *pHashEntry = NULL;

    if (NULL == hash_sub_tables)
    {
        APPIDNTF_ERROR("hash tbl not init.\r\n");
        return -1;
    }

    for (subTableIdx = APPIDNTFY_TCP_SUBTABLE; subTableIdx < APPIDNTFY_SUBTABLE_MAX; subTableIdx ++)
    {
        for(hashIdx = APPIDNTFY_SRC_IP; hashIdx < APPIDNTFY_TUPLE_INDEX_MAX; hashIdx ++)
        {
            for (idx = 0; idx < APPIDNTFY_HASH_TABLE_LEN; idx ++)
            {
                pHashEntry = &hash_sub_tables[subTableIdx][hashIdx][idx];
                if (pHashEntry->head.first)
                {
                    if(appidentify_hash_del(pHashEntry))
                    {
                        APPIDNTF_ERROR("appidentify_hash_del error.\r\n");
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
