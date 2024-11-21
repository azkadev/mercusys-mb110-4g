/*! Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     Appclf.h
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
#ifndef __APP_FLOW_H__
#define __APP_FLOW_H__

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/




/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/


#include <net/netfilter/nf_conntrack.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>
#include "appidentify_id.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define APP_FLOW_STAT_POOL_SIZE 10000
#define APP_FLOW_TREE_NUM       10
#define APP_FLOW_RANGE          20
/*#define COLLECT_TRAIN_DATA */
#define APP_FLOW_TIME           0

#ifdef COLLECT_TRAIN_DATA
#define APP_FLOW_RECORD_RANGE   20
#define APP_FLOW_TIME           1
#else
#define APP_FLOW_RECORD_RANGE   5
#endif

#define APP_FLOW_MAX_APP_NUM        64

#define APP_EVEN_BASIC_ID_FLAG_GET(appOri,appReply) \
    ((APP_BASIC_ID_FLAG_GET(appOri) == APP_BASIC_ID_FLAG_GET(appReply))? (APP_BASIC_ID_FLAG_GET(appOri)):APP_BASIC_ID_CHECKING)

#define APP_EVEN_NORMAL_ID_GET(appOri,appReply) \
    ((APP_NORMAL_ID_GET(appOri) == APP_NORMAL_ID_GET(appReply))? (APP_NORMAL_ID_GET(appOri)): APP_NORMAL_ID_CHECKING)


/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef enum  _APP_FLOW_DEBUG_PARAM
{
    APP_FLOW_DEBUG_SWITCH = 0,
    APP_FLOW_CHECK_APPID,
    APP_FLOW_PRINT,
    APP_FLOW_PRINT_UDP,
    APP_FLOW_PRINT_TCP,
    APP_FLOW_DEBUG_MAX
}APP_FLOW_DEBUG_PARAM;



typedef enum  _APP_FLOW_ATTR_INDEX
{
    APP_FLOW_ATTR_FWD_PKTLEN1 = 0,
    APP_FLOW_ATTR_FWD_PKTLEN2,
    APP_FLOW_ATTR_FWD_PKTLEN3,
    APP_FLOW_ATTR_FWD_PKTLEN4,
    APP_FLOW_ATTR_FWD_PKTLEN5,
    APP_FLOW_ATTR_FWD_PKTMIN,
    APP_FLOW_ATTR_FWD_PKTMAX,
    APP_FLOW_ATTR_FWD_INTER_PKTLEN1,
    APP_FLOW_ATTR_FWD_INTER_PKTLEN2,
    APP_FLOW_ATTR_FWD_INTER_PKTLEN3,
    APP_FLOW_ATTR_FWD_INTER_PKTLEN4,
    APP_FLOW_ATTR_BWD_PKTLEN1,
    APP_FLOW_ATTR_BWD_PKTLEN2,
    APP_FLOW_ATTR_BWD_PKTLEN3,
    APP_FLOW_ATTR_BWD_PKTLEN4,
    APP_FLOW_ATTR_BWD_PKTLEN5,
    APP_FLOW_ATTR_BWD_PKTMIN,
    APP_FLOW_ATTR_BWD_PKTMAX,
    APP_FLOW_ATTR_BWD_INTER_PKTLEN1,
    APP_FLOW_ATTR_BWD_INTER_PKTLEN2,
    APP_FLOW_ATTR_BWD_INTER_PKTLEN3,
    APP_FLOW_ATTR_BWD_INTER_PKTLEN4,
    APP_FLOW_ATTR_CNT
} APP_FLOW_ATTR_INDEX;


typedef enum _APP_FLOW_APP_INDEX
{
    APP_FLOW_ID_CHECKING = (-2),
    APP_FLOW_ID_UNKNOWN = (-1),
    APP_FLOW_ID_THUNDER,
    APP_FLOW_ID_BITTORRENT,
    APP_FLOW_ID_EMULE,
    APP_FLOW_ID_FLASHGET,
    APP_FLOW_ID_XUANFENG,
    APP_FLOW_ID_PPSTREAM,
    APP_FLOW_ID_PPTV,
    APP_FLOW_ID_QQMUSIC,
    APP_FLOW_ID_KUGOU,
    APP_FLOW_ID_VOICE,
    APP_FLOW_ID_OTHER,
    APP_FLOW_ID_MAX
} APP_FLOW_APP_INDEX;

typedef struct _APP_FLOW_INDEX_APPID_MAP
{
    APP_FLOW_APP_INDEX   appIndex;
    unsigned short       appdistId;
}APP_FLOW_INDEX_APPID_MAP;

typedef struct _APP_FLOW_NODE
{
   struct list_head    list;
   unsigned char isLeaf;
   APP_FLOW_ATTR_INDEX   feature;
   int           boundary;
   struct _APP_FLOW_NODE *left;
   struct _APP_FLOW_NODE *right;
   int           id;

}APP_FLOW_NODE;

typedef struct _APP_FLOW_ID_MAP
{
   char *appName;
   int   id;
   unsigned char protoNum;
}APP_FLOW_ID_MAP;

typedef struct _APP_FLOW_DATA
{
#if  APP_FLOW_TIME
    char timeStr[20];
#endif
    short pktLen[IP_CT_DIR_MAX][APP_FLOW_RECORD_RANGE];
    short recordCnt[IP_CT_DIR_MAX];
}APP_FLOW_DATA;

typedef struct _APP_FLOW_STAT
{
   void (*pfreeHandler)(void *);
   APP_FLOW_DATA data;
}APP_FLOW_STAT;



/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
extern int g_enableClsf;
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
int  app_flow_init(void);
void app_flow_exit(void);

int app_flow_set_param(unsigned int paramId, unsigned int value);

void app_flow_distinguish_hook(struct nf_conn *pCtEntry);
void app_flow_free_stat(void * pStat);

/* extern int app_netlink_init(void); */
/* extern void app_netlink_fini(void); */

int
string_makeSubStrByChar(char *string, char delimit, int maxNum, char *subStrArr[]);

unsigned int app_flow_statistic_record(unsigned int hooknum,
									   struct sk_buff *skb,
									   const struct net_device *in,
									   const struct net_device *out,
									   int (*okfn)(struct sk_buff *));


#endif /* __APP_FLOW_H__ */
