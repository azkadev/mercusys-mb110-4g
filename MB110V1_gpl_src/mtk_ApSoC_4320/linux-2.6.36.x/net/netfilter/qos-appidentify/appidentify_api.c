/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_api.c
 * Version:      1.0
 * Abstract:     Appidentify submodule api
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/netdevice.h>
#include "appidentify_id.h"
#include "appidentify_rules.h"
#include "appidentify_log.h"
#include "appidentify_api.h"

LIST_HEAD(appid_module_list);
DEFINE_MUTEX(appid_module_mutex);

#ifdef CONFIG_USE_AUTO_ID
#define AUTO_ID_MAX 256
DECLARE_BITMAP(autoid_bitmap, AUTO_ID_MAX);

static int auto_id_cursor = 0;
extern unsigned char *g_appPriDftMap;
#endif

extern struct list_head debug_module_list;

int appid_register_module(char* name, struct appid_module* module)
{
    struct appid_module* entry;
    struct debug_module* debug_entry;

    if (NULL == module->name || NULL == module->hook)
    {
        APPID_ERR(APPID_BASE, "invalid moudle %s", name);
        return -1;
    }

    mutex_lock(&appid_module_mutex);

    list_for_each_entry(entry, &appid_module_list, list)
    {
        if (0 == strcmp(name, entry->name))
        {
            APPID_ERR(APPID_BASE, "moudle %s exist", name);
            return -1;
        }
    }
    list_add_tail(&(module->list), &appid_module_list);
    debug_entry = (struct debug_module*)kmalloc(sizeof(struct debug_module), GFP_KERNEL);
    if (NULL == debug_entry)
    {
        APPID_ERR(APPID_BASE, "malloc debug entry failed, debug log will be disabled");
    }
    else
    {
        strncpy(debug_entry->name, name, DEBUG_MODULE_NAME_MAX);
        debug_entry->flag = 0;
        list_add_tail(&(debug_entry->list), &debug_module_list);
    }

    mutex_unlock(&appid_module_mutex);

    return 0;
}
EXPORT_SYMBOL(appid_register_module);

int appid_unregister_module(char* name)
{
    struct appid_module* entry;
    struct appid_module* next;
    struct debug_module* debug_entry;
    struct debug_module* debug_next;

    mutex_lock(&appid_module_mutex);

    list_for_each_entry_safe(entry, next, &appid_module_list, list)
    {
        if (0 == strcmp(name, entry->name))
        {
            list_del(&(entry->list));
        }
    }

    list_for_each_entry_safe(debug_entry, debug_next, &debug_module_list, list)
    {
        if (0 == strcmp(name, debug_entry->name))
        {
            list_del(&(debug_entry->list));
            kfree(debug_entry);
        }
    }

    mutex_unlock(&appid_module_mutex);

    return 0;
}
EXPORT_SYMBOL(appid_unregister_module);


#ifdef CONFIG_USE_AUTO_ID
int appid_alloc_id(int prio)
{
    int id = -1;
    int i = 0;
//    int flag = 0;

    if (prio < 1 || prio > 4)
    {
        APPID_ERR(APPID_BASE, "invalid prio value: %d", prio);
        return -1;
    }

    if (auto_id_cursor < AUTO_ID_MAX)
    {
        id = APPMNGR_APPID_AUTO_BEGIN + auto_id_cursor;
        auto_id_cursor ++;
        set_bit(id - APPMNGR_APPID_AUTO_BEGIN, autoid_bitmap);
        g_appPriDftMap[id] = prio;

        return id;
    }

    for (i = 0; i < AUTO_ID_MAX; ++i)
    {
        if (!test_bit(i, autoid_bitmap))
        {
            id = APPMNGR_APPID_AUTO_BEGIN + i;
            g_appPriDftMap[id] = prio;
        }
    }

    APPID_ERR(APPID_BASE, "allocate appid failed!\n");
    return -1;
}
EXPORT_SYMBOL(appid_alloc_id);

void appid_free_id(int id)
{
    if (id < APPMNGR_APPID_AUTO_BEGIN || id > APPMNGR_APPID_AUTO_END)
    {
        APPID_ERR(APPID_BASE, "invalid id: %d", id);
    }
    else
    {
        clear_bit(id - APPMNGR_APPID_AUTO_BEGIN, autoid_bitmap);
    }
}
EXPORT_SYMBOL(appid_free_id);
#endif
