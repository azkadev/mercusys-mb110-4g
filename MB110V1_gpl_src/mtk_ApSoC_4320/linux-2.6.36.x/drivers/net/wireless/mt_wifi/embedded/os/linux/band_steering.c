/*
 * @@-COPYRIGHT-START-@@
 *
 * Copyright (c) 2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * @@-COPYRIGHT-END-@@
 */
#include "linux/if.h"
#include "linux/socket.h"
#include "linux/netlink.h"
#include <net/sock.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cache.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/version.h>
//#include "sys/queue.h"
#include "ap_band_steering.h"
//#include "ieee80211_var.h"

#if 1
#define WLAN_DEFAULT_NETLINK_PID 0xffffffff
struct band_steering_netlink *band_steering_nl = NULL;
static u_int32_t pid = WLAN_DEFAULT_NETLINK_PID;
//static int32_t ifindex = -1;

void mtk_deliver_band_steering_event(u_int32_t ifindex, int type, int len, u_int8_t* data)
{
	ath_netlink_bsteering_event_t netlink_event;

	if (-1 == ifindex)
	{
		return;
	}
	
	memset(&netlink_event, 0x0, sizeof(netlink_event));
    netlink_event.type = type;
    netlink_event.sys_index = ifindex;
	//printk("%s %d netlink_event.type = %d, netlink_event.sys_index = %d\r\n", __FUNCTION__, __LINE__, netlink_event.type, netlink_event.sys_index);
    if (len && data)
	{
        memcpy(&(netlink_event.data), data, len);
    }
	
	mtk_band_steering_netlink_send(&netlink_event);
}


/**
 * @brief Send a unicast netlink message containing the band steering event.
 *
 * @param event  the structure containing all of the event data
 */
void mtk_band_steering_netlink_send(ath_netlink_bsteering_event_t *event)
{
    struct sk_buff *skb = NULL;
    struct nlmsghdr *nlh;
    u_int8_t *nldata = NULL;
    int len=0;

    if (!band_steering_nl || !event) {
        return;
    }

    if (WLAN_DEFAULT_NETLINK_PID == pid) {
        // No user space daemon has requested events yet, so drop it.
        return;
    }
    len=NLMSG_ALIGN(sizeof(*event));
    skb = nlmsg_new(len, GFP_ATOMIC);

    if (!skb) {
        //QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: No memory, event = %d\n", __func__, event->type);
        return;
    }

    nlh = nlmsg_put(skb, pid, 0, event->type, sizeof(*event), 0);
    if (!nlh) {
        //QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: nlmsg_put() failed, event = %d\n", __func__, event->type);
        kfree_skb(skb);
        return;
    }

    nldata = NLMSG_DATA(nlh);
    memcpy(nldata, event, sizeof(*event));

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0) 
    NETLINK_CB(skb).pid = 0;        /* from kernel */
#else
    //NETLINK_CB(skb).portid = 0;     /* from kernel */
    NETLINK_CB(skb).pid = 0;        /* from kernel */
#endif
	NETLINK_CB(skb).pid = 0;        /* from kernel */

    NETLINK_CB(skb).dst_group = 0;  /* unicast */
    netlink_unicast(band_steering_nl->bsteer_sock, skb, pid, MSG_DONTWAIT);
}

/**
 * @brief Handle a netlink message sent from user space.
 *
 * @param 2
 * @param 6
 * @param struct sk_buff *__skb  the buffer containing the data to receive
 * @param len  the length of the data to receive
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
static void ath_band_steering_netlink_receive(struct sk_buff *__skb)
#else
static void ath_band_steering_netlink_receive(struct sock *sk, int len)
#endif
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh = NULL;
    //u_int32_t pid;
   // int32_t ifindex;
    //struct net_device *dev;
    //osif_dev  *osifp;
    //wlan_if_t vap;

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
    if ((skb = skb_get(__skb)) != NULL){
#else
    if ((skb = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
#endif
        /* process netlink message pointed by skb->data */
        nlh = (struct nlmsghdr *)skb->data;
        pid = nlh->nlmsg_pid;
		#if 0
        ifindex = nlh->nlmsg_flags;

		printk("%s %d ifindex = %d\r\n", __FUNCTION__, __LINE__, ifindex);
        dev = dev_get_by_index(&init_net, ifindex);

        if (!dev) {
            //QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: Invalid interface index:%d\n", __func__, ifindex);
            printk("%s %d dev = null, ruturn\r\n", __FUNCTION__, __LINE__);
            kfree_skb(skb);
            return;
        }
		#endif
	#if 0
        osifp = ath_netdev_priv(dev);

        if (osifp ->is_deleted) {
            //QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: vap[%s] has been deleted\n", __func__, dev->name);
            goto out;
        }

        vap = osifp->os_if;
        if(vap == NULL) {
            goto out;
        }
#endif
		printk("%s %d pid = %d\r\n", __FUNCTION__, __LINE__, pid);
	
        if (!band_steering_nl)
            goto out;

        if(band_steering_nl->bsteer_sock == NULL) {
            goto out;
        }
#if 0
        if (vap->lbd_pid != pid) {
            vap->lbd_pid = pid;
        }
#endif
        //QDF_PRINT_INFO(vap->iv_ic->ic_print_idx, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "[%s] Band steering events being sent to PID:%d\n",dev->name,vap->lbd_pid);
out:
        //dev_put(dev);
        kfree_skb(skb);
    }
}
/**
  * @brief Initialize the netlink socket for the band steering module.
  *
  * @return  0 on success; -ENODEV on failure
  */
int mtk_band_steering_netlink_init(void)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION (3,10,0)
    extern struct net init_net;
    struct netlink_kernel_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.groups = 1;
    cfg.input = &ath_band_steering_netlink_receive;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
    extern struct net init_net;

    struct netlink_kernel_cfg cfg = {
        .groups = 1,
        .input  = ath_band_steering_netlink_receive,
    };
#endif

    if (!band_steering_nl) {
        band_steering_nl = (struct band_steering_netlink *)
            kzalloc(sizeof(struct band_steering_netlink), GFP_KERNEL);
        if (!band_steering_nl) {
            return -ENODEV;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)/* >= (3,10,0) */
        band_steering_nl->bsteer_sock = (struct sock *)netlink_kernel_create(
                                                                              &init_net,
                                                                              NETLINK_BAND_STEERING_EVENT,
                                                                              &cfg);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
        band_steering_nl->bsteer_sock = (struct sock *)netlink_kernel_create(
                                                                              &init_net,
                                                                              NETLINK_BAND_STEERING_EVENT,
                                                                              THIS_MODULE, &cfg);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
        band_steering_nl->bsteer_sock = (struct sock *) netlink_kernel_create(
                                                                              &init_net,
                                                                              NETLINK_BAND_STEERING_EVENT, 1,
                                                                              &ath_band_steering_netlink_receive,
                                                                              NULL, THIS_MODULE);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,22)
        band_steering_nl->bsteer_sock = (struct sock *) netlink_kernel_create(
                                                                              NETLINK_BAND_STEERING_EVENT, 1,
                                                                              &ath_band_steering_netlink_receive,
                                                                              NULL, THIS_MODULE);

#else
        band_steering_nl->bsteer_sock = (struct sock *)netlink_kernel_create(
                                                                             NETLINK_BAND_STEERING_EVENT, 1,
                                                                             &ath_band_steering_netlink_receive,
                                                                             THIS_MODULE);
#endif
        if (!band_steering_nl->bsteer_sock) {
            kfree(band_steering_nl);
            band_steering_nl = NULL;
            //QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s NETLINK_KERNEL_CREATE FAILED\n", __func__);
            return -ENODEV;
        }
        atomic_set(&band_steering_nl->bsteer_refcnt, 1);
    } else {
        atomic_inc(&band_steering_nl->bsteer_refcnt);
    }
	printk("%s %d band steering netlink init ok\r\n", __FUNCTION__, __LINE__);
    return 0;
}
/**
  * @brief Close and destroy the netlink socket for the band steering module.
  *
  * @return 0 on success; non-zero on failure
  */
int mtk_band_steering_netlink_delete(void)
{
    if (!band_steering_nl) {
        //QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s band_steering_nl is NULL\n", __func__);
        return -ENODEV;
    }
    if (!atomic_dec_return(&band_steering_nl->bsteer_refcnt)) {
        if (band_steering_nl->bsteer_sock) {
	    netlink_kernel_release(band_steering_nl->bsteer_sock);
        }
        kfree(band_steering_nl);
        band_steering_nl = NULL;
    }
	//ifindex = -1;
	pid = WLAN_DEFAULT_NETLINK_PID;
    return 0;
}

EXPORT_SYMBOL(mtk_deliver_band_steering_event);
EXPORT_SYMBOL(mtk_band_steering_netlink_send);
EXPORT_SYMBOL(mtk_band_steering_netlink_init);
EXPORT_SYMBOL(mtk_band_steering_netlink_delete);
#endif
