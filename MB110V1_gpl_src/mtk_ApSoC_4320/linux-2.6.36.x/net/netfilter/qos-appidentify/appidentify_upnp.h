/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_upnp.h
 * Version:      1.0
 * Abstract:     Appidentify upnp file
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#ifndef __APPIDENTIFY_UPNP_H__
#define __APPIDENTIFY_UPNP_H__

int  app_upnp_init(void);
void app_upnp_exit(void);
unsigned int app_upnp_ct_check(unsigned int hooknum,
                             struct sk_buff *skb,
                             const struct net_device *in,
                             const struct net_device *out,
                             int (*okfn)(struct sk_buff *));

#endif // c__APPIDENTIFY_UPNP_H__
