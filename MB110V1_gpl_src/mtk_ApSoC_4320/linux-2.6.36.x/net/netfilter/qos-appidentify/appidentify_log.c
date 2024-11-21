/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_log.c
 * Version:      1.0
 * Abstract:     Appidentify log interface
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/string.h>

int _appid_base_debug_enable     = 1;
int _appid_dpi_debug_enable      = 0;
int _appid_dfi_debug_enable      = 0;
int _appid_dns_debug_enable      = 0;
int _appid_upnp_debug_enable     = 0;
int _appid_port_debug_enable     = 0;
int _appid_netlink_debug_enable  = 1;
int _appid_stat_debug_enable     = 0;

#include "appidentify_log.h"

LIST_HEAD(debug_module_list);

static int* _appid_get_debug_flag_by_module(char* module)
{
    int *debug_flag = NULL;

    if (0 == strcmp(APPID_DPI, module)) {
        debug_flag = &_appid_dpi_debug_enable;
    } else if (0 == strcmp(APPID_DFI, module)) {
        debug_flag = &_appid_dfi_debug_enable;
    } else if (0 == strcmp(APPID_DNS, module)) {
        debug_flag = &_appid_dns_debug_enable;
    } else if (0 == strcmp(APPID_UPNP, module)) {
        debug_flag = &_appid_upnp_debug_enable;
    } else if (0 == strcmp(APPID_PORT, module)) {
        debug_flag = &_appid_port_debug_enable;
    } else if (0 == strcmp(APPID_STAT, module)) {
        debug_flag = &_appid_stat_debug_enable;
    } else {
        debug_flag = NULL;
    }

    return debug_flag;
}

int _appid_debug_is_enabled(char* module)
{
    int* debug_flag = NULL;

    if (0 == strcmp(APPID_BASE, module) ||
        0 == strcmp(APPID_OTHER, module) ||
        0 == strcmp(APPID_NETLINK, module))
    {
        return 1;
    }

    debug_flag = _appid_get_debug_flag_by_module(module);
    if (NULL != debug_flag)
    {
        return *debug_flag;
    }

    return 0;
}

int _appid_submodule_debug_is_enabled(char* module)
{
    struct debug_module* debug_entry;

    list_for_each_entry(debug_entry, &debug_module_list, list)
    {
        if (0 == strcmp(module, debug_entry->name))
        {
            return debug_entry->flag;
        }
    }

    return 0;
}
EXPORT_SYMBOL(_appid_submodule_debug_is_enabled);

inline int _appid_set_debug(char* module, int value)
{
    struct debug_module* debug_entry;
    int* debug_flag = NULL;

    debug_flag = _appid_get_debug_flag_by_module(module);

    if (NULL != debug_flag) {
        *debug_flag = value;
        goto out;
    } else {
        list_for_each_entry(debug_entry, &debug_module_list, list)
        {
            if (0 == strcmp(module, debug_entry->name))
            {
                debug_entry->flag = value;
                goto out;
            }
        }
    }

    printk(KERN_ERR "APPID ERROR [base](%s:%d) : invalid debug moudule [%s]\n",
           __FUNCTION__, __LINE__, module);
    return -1;
out:
    return 0;
}
