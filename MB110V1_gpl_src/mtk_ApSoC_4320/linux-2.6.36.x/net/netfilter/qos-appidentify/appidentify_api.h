/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_api.h
 * Version:      1.0
 * Abstract:     Appidentify submodule api
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#ifndef __APPIDENTIFY_API_H__
#define __APPIDENTIFY_API_H__

#define CONFIG_USE_AUTO_ID

#define APPID_MODULE_NAME_MAX 16
#define APPID_MODULE_ID_MAX   16

/// module interfaces
struct appid_module
{
    char name[APPID_MODULE_NAME_MAX];
    unsigned int   (*hook) (unsigned int hooknum, struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));
    struct list_head list;
};

int  appid_register_module(char* name, struct appid_module* module);
int  appid_unregister_module(char* name);
int  appid_alloc_id(int prio);
void appid_free_id(int id);

int _appid_submodule_debug_is_enabled(char* module);

#endif // __APPIDENTIFY_API_H__
