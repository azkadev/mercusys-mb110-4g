/*! Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     app_netlink.h
 *\brief
 *\details
 *
 *\author   Weng Kaiping
 *\version
 *\date     16Oct13
 *
 *\warning
 *
 *\history \arg
 */

#ifndef __APP_NETLINK_H__
#define __APP_NETLINK_H__

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/




/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/


#include "appidentify_flow.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
/* Types of messages */
#define APPM_BASE            0x10   /* standard netlink messages below this */
#define APPM_DEBUG_PARAM     (APPM_BASE + 1)
#define APPM_UPNP            (APPM_BASE + 2)
#define APPM_DB              (APPM_BASE + 3)
#define APPM_DB_APPPRI       (APPM_BASE + 4)
#define APPM_DB_DNSKW        (APPM_BASE + 5)
#define APPM_STAT            (APPM_BASE + 6)
#define APPM_MAX             (APPM_BASE + 7)

#define UPNP_MAX_VAL_LEN     32
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef enum _APP_MSG_OPT
{
    APP_OPT_UPDATE = 0,
    APP_OPT_REMOVE,
    APP_OPT_DISPLAY,
    APP_OPT_FLUSH,
    APP_OPT_HELP,
    APP_OPT_MAX
}APP_MSG_OPT;

typedef struct app_upnp_mapping
{
    unsigned short externalPort;
    unsigned short internalPort;
    unsigned char  protocol;
    unsigned char  enabled;
    unsigned int   internalClient;
    char description[UPNP_MAX_VAL_LEN];
} app_upnp_mapping_t;


typedef struct app_param_msg {
    unsigned int  paramId;
    unsigned int  value;
} app_param_msg_t;

typedef struct _APP_DB_MSG_INFO
{
    unsigned int          ruleCount;
    unsigned int          ruleSize;
}APP_DB_MSG_INFO;

typedef struct app_stat_msg {
    unsigned int  statOpt;
    unsigned int  appid;
    unsigned int  ip;
    unsigned int  mask;
} app_stat_msg_t;

typedef struct app_peer_msg {
    unsigned short      opt;
    union {
        app_param_msg_t    param;
        app_upnp_mapping_t upnp;
        APP_DB_MSG_INFO    dbInfo;
        app_stat_msg_t     stat;
    } msg;
} app_peer_msg_t;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
int
app_netlink_init(void);

void
app_netlink_fini(void);



#endif /* __APP_NETLINK_H__ */
