/*
 * @@-COPYRIGHT-START-@@
 *
 * Copyright (c) 2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * @@-COPYRIGHT-END-@@
 */

//#include <osdep.h>
#include "band_steering_api.h"

#ifndef _ATH_BAND_STEERING__
#define _ATH_BAND_STEERING___

#if 1
struct band_steering_netlink {
    struct sock      *bsteer_sock;
    atomic_t         bsteer_refcnt;
};
#if 0
int mtk_band_steering_netlink_init(void);
int mtk_band_steering_netlink_delete(void);
void mtk_band_steering_netlink_send(ath_netlink_bsteering_event_t *event);
void mtk_deliver_band_steering_event(int type, int len, u_int8_t* data);
#endif
#else
#define ath_band_steering_netlink_init() do {} while (0)
#define ath_band_steering_netlink_delete() do {} while (0)
#define ath_band_steering_netlink_send(ev,pid) do {} while (0)
#endif

#endif /* _ATH_BAND_STEERING___*/
