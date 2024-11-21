/* Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     app_netlink.c
 *\brief
 *\details
 *
 *\author   Weng Kaiping
 *\version
 *\date     14Oct13
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
#include <linux/netlink.h>
#include <linux/security.h>

#include "appidentify_flow.h"
#include "appidentify_statistics.h"
#include "appidentify_netlink.h"
#include "appidentify_log.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

static struct sock *appnl __read_mostly;
static DEFINE_MUTEX(appnl_mutex);

extern int app_netlink_set_upnp(app_upnp_mapping_t * entry, unsigned short opt);
extern int appidentify_update_db(void *pRuleBuf, int ruleNum, int len);
extern int appprio_update_db(void * pAppMap, int appNum, int len);
extern int appidentify_dnskw_update(void * pRuleBuf, int ruleNum, int len);
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
static int
app_receive_peer(struct app_peer_msg *pmsg,
         unsigned char type, unsigned int len)
{
    int status = 0;

    APPID_LOG(APPID_NETLINK, "handle msg from userspace: %d", type);
    if (len < sizeof(*pmsg))
        return -EINVAL;

    switch (type) {
    case APPM_DEBUG_PARAM:
        status = app_flow_set_param(pmsg->msg.param.paramId,
                     pmsg->msg.param.value);
        break;
    case APPM_UPNP:
        status = app_netlink_set_upnp(&(pmsg->msg.upnp), pmsg->opt);
        break;
    case APPM_DB:
        status = appidentify_update_db((void *)((char*)pmsg + sizeof(struct app_peer_msg)),
                                       pmsg->msg.dbInfo.ruleCount, (len - sizeof(struct app_peer_msg)));
        if (0 == status) {
            APPID_LOG(APPID_NETLINK, "load database success");
        } else {
            APPID_ERR(APPID_NETLINK, "load database failed");
        }

        break;
    case APPM_DB_APPPRI:
        status = appprio_update_db((void *)((char*)pmsg + sizeof(struct app_peer_msg)),
                                   pmsg->msg.dbInfo.ruleCount, (len - sizeof(struct app_peer_msg)));
        if (0 == status) {
            APPID_LOG(APPID_NETLINK, "load app priomap success");
        } else {
            APPID_ERR(APPID_NETLINK, "load app priomap failed");
        }


        break;
    case APPM_DB_DNSKW:
        status = appidentify_dnskw_update((void *)((char*)pmsg + sizeof(struct app_peer_msg)),
                                          pmsg->msg.dbInfo.ruleCount, (len - sizeof(struct app_peer_msg)));
        if (0 == status) {
            APPID_LOG(APPID_NETLINK, "load dns keywords success");
        } else {
            APPID_ERR(APPID_NETLINK, "load dns keywords failed");
        }
        break;
    case APPM_STAT:
        status = appidentify_stat_set(&(pmsg->msg.stat), pmsg->opt);
        if (0 == status) {
            APPID_LOG(APPID_NETLINK, "set statistic module success");
        } else {
            APPID_ERR(APPID_NETLINK, "set statistic module success");
        }
        break;

    default:
        status = -EINVAL;
    }
    return status;
}



#define RCV_SKB_FAIL(err) do { netlink_ack(skb, nlh, (err)); return; } while (0)

static inline void
__app_rcv_skb(struct sk_buff *skb)
{
    int status, type, pid, flags, nlmsglen, skblen;
    struct nlmsghdr *nlh;

    APPID_LOG(APPID_NETLINK, "receive netlink data");

    skblen = skb->len;
    if (skblen < sizeof(*nlh)) {
        APPID_ERR(APPID_NETLINK, "skb len is too short");
        return;
    }

    nlh = nlmsg_hdr(skb);
    nlmsglen = nlh->nlmsg_len;
    if (nlmsglen < sizeof(*nlh) || skblen < nlmsglen) {
        APPID_ERR(APPID_NETLINK, "netlink msg len is invalid");
        return;
    }

    pid = nlh->nlmsg_pid;
    flags = nlh->nlmsg_flags;

    if(pid <= 0 || !(flags & NLM_F_REQUEST) || flags & NLM_F_MULTI) {
        APPID_ERR(APPID_NETLINK, "wrong pid, reply error msg");
        RCV_SKB_FAIL(-EINVAL);
    }

    if (flags & MSG_TRUNC) {
        APPID_ERR(APPID_NETLINK, "msg trunc, reply error msg");
        RCV_SKB_FAIL(-ECOMM);
    }

    type = nlh->nlmsg_type;
    if (type < NLMSG_NOOP || type >= APPM_MAX) {
        APPID_ERR(APPID_NETLINK, "invalid type, reply error msg");
        RCV_SKB_FAIL(-EINVAL);
    }

    if (type <= APPM_BASE) {
        APPID_ERR(APPID_NETLINK, "wrong type");
        return;
    }

#if 0 //+TODO: who does this do?
    if (security_netlink_recv(skb, CAP_NET_ADMIN))
        RCV_SKB_FAIL(-EPERM);
#endif

    status = app_receive_peer(NLMSG_DATA(nlh), type,
                  nlmsglen - NLMSG_LENGTH(0));
    if (status < 0)
        RCV_SKB_FAIL(status);

    if (flags & NLM_F_ACK)
        netlink_ack(skb, nlh, 0);
    return;
}

static void
app_rcv_skb(struct sk_buff *skb)
{
    mutex_lock(&appnl_mutex);
    __app_rcv_skb(skb);
    mutex_unlock(&appnl_mutex);
}


int app_netlink_init()
{
    appnl = netlink_kernel_create(&init_net, NETLINK_APPDIST, 0,
                                  app_rcv_skb, NULL, THIS_MODULE);

    if (appnl == NULL) {
        APPID_ERR(APPID_NETLINK, "create netlink socket failed");
        return -1;
    }

    return 0;
}

void app_netlink_fini()
{
    mutex_lock(&appnl_mutex);
    netlink_kernel_release(appnl);
    mutex_unlock(&appnl_mutex);
}
