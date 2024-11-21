/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_log.h
 * Version:      1.0
 * Abstract:     Appidentify log interface
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#ifndef __APPIDENTIFY_LOG_H__
#define __APPIDENTIFY_LOG_H__

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
#include <linux/printk.h>
#endif
#include <linux/ratelimit.h>

#define APPID_DEBUG_ON          // comment this line for disable log
#define LOG_RATELIMIT           // comment this to cancel log rate limit

#define APPID_BASE            "base"
#define APPID_OTHER           "other"
#define APPID_DPI             "dpi"
#define APPID_DFI             "dfi"
#define APPID_DNS             "dns"
#define APPID_UPNP            "upnp"
#define APPID_PORT            "root"
#define APPID_NETLINK         "netlink"
#define APPID_STAT            "stat"

#define DEBUG_MODULE_NAME_MAX 16
struct debug_module {
    char name[DEBUG_MODULE_NAME_MAX];
    int  flag;
    struct list_head list;
};

#ifdef LOG_RATELIMIT
#define log_printk printk_ratelimited
#else
#define log_printk printk
#endif

#ifdef APPID_DEBUG_ON
#define APPID_LOG(module, fmt, ...) do {                            \
    if (_appid_debug_is_enabled(module)) {                          \
        log_printk(KERN_DEBUG "APPID [%s](%s:%d): "fmt"\n",         \
                   module, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                               \
} while(0)
#else
#define APPID_LOG(module, fmt, ...)
#endif

#define APPID_ERR(module, fmt, ...) do {                            \
    log_printk(KERN_ERR "APPID ERROR [%s](%s:%d) : "fmt"\n",        \
               module, __FUNCTION__, __LINE__, ##__VA_ARGS__);      \
} while(0)


#define appid_enable_debug(module) _appid_set_debug(module, 1)
#define appid_disable_debug(module) _appid_set_debug(module, 0)

extern int _appid_dpi_debug_enable;
extern int _appid_dfi_debug_enable;
extern int _appid_dns_debug_enable;
extern int _appid_upnp_debug_enable;
extern int _appid_port_debug_enable;
extern int _appid_stat_debug_enable;

int _appid_set_debug(char* module, int value);
int _appid_debug_is_enabled(char* module);
int _appid_submodule_debug_is_enabled(char* module);


#endif  // __APPIDENTIFY_LOG_H__
