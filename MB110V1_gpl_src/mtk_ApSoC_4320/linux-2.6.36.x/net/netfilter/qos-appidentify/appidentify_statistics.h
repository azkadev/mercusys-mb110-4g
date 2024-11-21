/*! Copyright(c) 2008-2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_statistics.h
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
#ifndef __APPIDENTIFY_STATISTICS_H__
#define __APPIDENTIFY_STATISTICS_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/if_ether.h>
#include <linux/list.h>

#include "appidentify_netlink.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef enum _STAT_POOL_TYPE
{
    STAT_POOL_TYPE_ENTRY,
    STAT_POOL_TYPE_SET,
    STAT_POOL_TYPE_END
}STAT_POOL_TYPE;

typedef enum _APP_STAT_OPT
{
    APP_STAT_OPT_LANINFO,
    APP_STAT_OPT_END
}APP_STAT_OPT;

typedef struct _STAT_ENTRY
{
    unsigned int        bytes;
}STAT_ENTRY;

typedef struct _APP_IP_SET
{
    unsigned int        ip4;
    int                 appid;
    short               app_prio;
}APP_IP_SET;

typedef struct _STAT_WARN_CNT
{
    short               ex_cnt;
    short               total_cnt;
    short               warn_flag;
}STAT_WARN_CNT;

typedef struct _APPIDNTFY_STATCS
{
    struct list_head    list;

    STAT_ENTRY          all;

    STAT_ENTRY          tx_cnt;

    STAT_ENTRY          rx_cnt;

    STAT_ENTRY          recent;

    STAT_ENTRY          old;

    STAT_WARN_CNT       warn;

    union
    {
        APP_IP_SET      app_ip_set;
        unsigned char   mac[ETH_ALEN];
    }stat_unit;
}APPIDNTFY_STATCS;

typedef struct _STATCS_SET
{
    struct list_head    list;

    union
    {
        int             appid;
        unsigned int    ip4;
    }set;
}STATCS_SET;


/* for high speed conn watched */
typedef struct _WATCH_TUPLE
{
    unsigned int        src_ip;
    unsigned int        dst_ip;
    unsigned short      src_port;
    unsigned short      dst_port;
    unsigned char       protonum;
}WATCH_TUPLE;

typedef struct _WATCH_APP_LIST
{
    struct list_head    watch_list_node;
    struct list_head    tuple_list_head;
    APP_IP_SET          app;
    int                 free_flag;
}WATCH_APP_LIST;

typedef struct _WATCH_TUPLE_LIST
{
    struct list_head    tuple_list_node;

    struct list_head    tuple_lru_node;

    WATCH_TUPLE         ori_tuple;

    STAT_ENTRY          all;

    STAT_ENTRY          recent;

    STAT_ENTRY          old;

    STAT_WARN_CNT       warn;
}WATCH_TUPLE_LIST;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
int appidentify_statistics_init(void);
int appidentify_statistics_exit(void);

unsigned int appidentify_statistics_hook(unsigned int hook,
                 struct sk_buff *skb,
                 const struct net_device *in,
                 const struct net_device *out,
                 int (*okfn)(struct sk_buff *));

int appidentify_stat_set(app_stat_msg_t *msg, unsigned short opt);

void appidentify_statistics_print(void);
void appidentify_stat_clear(void);




#endif  /* __APPIDENTIFY_STATISTICS_H__ */
