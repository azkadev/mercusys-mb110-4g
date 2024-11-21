/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_proc.c
 * Version:      1.0
 * Abstract:     Appidentify proc file
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/inet.h>

#include "appidentify.h"
#include "appidentify_rules.h"
#include "appidentify_node.h"
#include "appidentify_id.h"
#include "appidentify_match_rules.h"
#include "appidentify_hash_table.h"
#include "appidentify_flow.h"
#include "appprio.h"
#include "appidentify_dns.h"
#include "appidentify_statistics.h"
#include "appidentify_log.h"
#include "dnsparser/dnsparse_testsuite.h"
#include "appidentify_scene.h"
#include "appidentify_proc.h"


#define APPID_PROC_DIR_NAME   "appid"
#define APPID_PROC_DEBUG_NAME "debug"
#define APPID_PROC_STAT_NAME  "stat"
#define APPID_PROC_SCENE_NAME "scene"

struct proc_dir_entry *appid_proc_path;
EXPORT_SYMBOL(appid_proc_path);

static struct proc_dir_entry *appid_proc_debug;
static struct proc_dir_entry *appid_proc_stat;
static struct proc_dir_entry *appid_proc_scene;

extern bool g_enablePort;
extern bool g_port_debug;

extern int app_stat_debug;
extern int g_app_stat_enable;
static char msg[128]={0};
static char scene_msg[128]={0};

extern bool appUpnpInitd;
extern bool g_enablePort;
extern bool appUpnpInitd;
extern bool g_enablePort;
extern int  g_enableDns;
extern int  g_enableDpi;
extern int  g_enableClsf;

/* 29Oct15 */
extern int  appidntf_debug; 

extern struct list_head        l_stat_appid_list;

static int appid_debug_read(struct file * file, char *data, size_t len, loff_t *off)
{
    if(*off > 0)
    {
        return 0;
    }
    if(copy_to_user(data,msg,strlen(msg)))
    {
        return -EFAULT;
    }
    *off += strlen(msg);

    return strlen(msg);
}

static int appid_debug_write(struct file *file, const char *data, size_t len, loff_t *off)
{
    if(copy_from_user(msg,(void*)data,len))
    {
        return -EFAULT;
    }

    msg[len]='\0';
    if (!strcmp("prio_reset\n", msg))
    {
        appprio_cnt_reset();
    }
    else if (!strcmp("prio_debug\n", msg))
    {
        appprio_debug();
    }
	/* ADD */
	else if (!strcmp("debug_on\n", msg))
    {
        appidntf_debug = TRUE;
    }
    else if (!strcmp("debug_off\n", msg))
    {
        appidntf_debug = FALSE;
    } /* */
    else if (!strcmp("dpi_debug_on\n", msg))
    {
        appid_enable_debug(APPID_DPI);
    }
    else if (!strcmp("dpi_debug_off\n", msg))
    {
        appid_disable_debug(APPID_DPI);
    }
    else if (!strcmp("dns_print\n", msg))
    {
        appidentify_dns_printall();
    }
    else if (!strcmp("dnskw_print\n", msg))
    {
        appidentify_dnskw_printall();
    }
    else if (strstr(msg, "debug on")) // "dpi debug on"
    {
        char* strElem[16+1] = {NULL};
//        char* token;
        int strNum = 0;
        char* module;
        int  ret;

        if (-1 != (strNum = string_makeSubStrByChar(msg, ' ', 16+1 , strElem)))
        {
            if (3 == strNum)
            {
                module = strElem[0];
                ret = _appid_set_debug(module, 1);
                if (0 == ret)
                {
                    APPID_LOG(APPID_BASE, "enable log output of %s module", module);
                }
            }
        }
    }
    else if (strstr(msg, "debug off")) // "dpi debug off"
    {
        char* strElem[16+1] = {NULL};
//        char* token;
        int strNum = 0;
        char* module;
        int  ret;

        if (-1 != (strNum = string_makeSubStrByChar(msg, ' ', 16+1 , strElem)))
        {
            if (3 == strNum)
            {
                module = strElem[0];
                ret = _appid_set_debug(module, 0);
                if (0 == ret)
                {
                    APPID_LOG(APPID_BASE, "disable log output of %s module", module);
                }
            }
        }
    }
    else if (strstr(msg, "dns add"))
    {
        char *strElem[8 + 1]={NULL};
        int strNum = 0;
        unsigned int ipaddr;
        int appid;

        if (-1 != (strNum = string_makeSubStrByChar(msg, ' ', 8 + 1, strElem)))
        {
            if (5 == strNum)
            {
                printk("str: %s %s %s %s %s.\n", strElem[0], strElem[1], strElem[2], strElem[3], strElem[4]);
                ipaddr = in_aton(strElem[3]);
                appid = simple_strtol(strElem[4], NULL, 10);
                appidentify_dns_testentry_add(strElem[2], ipaddr, appid);
            }
        }
    }
    else if (strstr(msg, "dnskw_add"))
    {
        char* strElem[16+1] = {NULL};
 //       char* token;
        int strNum = 0;
        int appId;
        char* domain;

        if (-1 != (strNum = string_makeSubStrByChar(msg, ' ', 16+1 , strElem)))
        {
            if (3 == strNum)
            {
                domain = strElem[1];
                appId  = simple_strtol(strElem[2], NULL, 10);
                appidentify_add_dnskw(domain, appId);
            }
        }
    }
    else if (!strcmp("app_stat\n", msg))
    {
        appidentify_statistics_print();
    }
    else if (!strcmp("app_typemap_print\n", msg))
    {
        print_app_type_map();
    }
    else if (!strcmp("stat_clear\n", msg))
    {
        appidentify_stat_clear();
    }
    else if (!strcmp("disable_all\n", msg))
    {
        APPID_LOG(APPID_BASE, "disable all identify submodule");
        appUpnpInitd = FALSE;
        g_enablePort = FALSE;
        g_enableDns  = FALSE;
        g_enableDpi  = FALSE;
        g_enableClsf = FALSE;
    }
    else if (!strcmp("enable_all\n", msg))
    {
        APPID_LOG(APPID_BASE, "enable all identify submodule");
        appUpnpInitd = TRUE;
        g_enablePort = TRUE;
        g_enableDns  = TRUE;
        g_enableDpi  = TRUE;
        g_enableClsf = TRUE;
    }
    else
    {
        printk("invalid command: %s\n", msg);
    }
    return len;
}

static struct file_operations appid_debug_ops = {
    .read    = appid_debug_read,
    .write   = appid_debug_write,
};

static void *appid_stat_seq_start(struct seq_file *seq, loff_t *pos)
{
    return seq_list_start(&l_stat_appid_list, *pos);
}

static void *appid_stat_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
    return seq_list_next(v, &l_stat_appid_list, pos);
}

static void appid_stat_seq_stop(struct seq_file *seq, void *v)
{}

static int appid_stat_seq_show(struct seq_file *seq, void *v)
{
    APPIDNTFY_STATCS    *stat_entry;

    stat_entry = list_entry(v, APPIDNTFY_STATCS, list);
    seq_printf(seq, "%u %d %d %u %u\n",
               stat_entry->stat_unit.app_ip_set.ip4,      // client ip
               stat_entry->stat_unit.app_ip_set.appid,    // appid
               stat_entry->stat_unit.app_ip_set.app_prio, // app prio
               stat_entry->tx_cnt.bytes,                  // upload bytes
               stat_entry->rx_cnt.bytes                   // download bytes
        );
	return 0;
}

static struct seq_operations appid_stat_seq_ops = {
    .start = appid_stat_seq_start,
    .next  = appid_stat_seq_next,
    .stop  = appid_stat_seq_stop,
    .show  = appid_stat_seq_show
};

static int appid_stat_seq_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &appid_stat_seq_ops);
}

static struct file_operations appid_stat_ops = {
    .owner   = THIS_MODULE,
    .open    = appid_stat_seq_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};

static int appid_scene_read(struct file * file, char *data, size_t len, loff_t *off)
{
    if(*off > 0)
    {
        return 0;
    }

    if(copy_to_user(data, scene_msg, strlen(scene_msg)))
    {
        return -EFAULT;
    }

    *off += strlen(scene_msg);

    return strlen(scene_msg);
}

static int appid_scene_write(struct file *file, const char *data, size_t len, loff_t *off)
{
    int scene_mode;

    if(copy_from_user(scene_msg, (void*)data, len))
    {
        return -EFAULT;
    }

    scene_msg[len]='\0';
    scene_mode = app_parse_scene(scene_msg);
    app_set_scene(scene_mode);

    return len;
}

static struct file_operations appid_scene_ops = {
    .read    = appid_scene_read,
    .write   = appid_scene_write
};

int app_proc_init(void)
{
    appid_proc_path = proc_mkdir(APPID_PROC_DIR_NAME, NULL);
    if (!appid_proc_path)
    {
        goto failed_out;
    }

    appid_proc_debug = proc_create(APPID_PROC_DEBUG_NAME, 0666, appid_proc_path, &appid_debug_ops);
    if (!appid_proc_debug)
    {
        goto cleanup_debug;
    }

    appid_proc_stat = proc_create(APPID_PROC_STAT_NAME, 0666, appid_proc_path, &appid_stat_ops);
    if (!appid_proc_stat)
    {
        goto cleanup_stat;
    }

    appid_proc_scene = proc_create(APPID_PROC_SCENE_NAME, 0666, appid_proc_path, &appid_scene_ops);
    if (!appid_proc_scene)
    {
        goto cleanup_scene;
    }

    return 0;

cleanup_scene:
    remove_proc_entry(APPID_PROC_STAT_NAME, appid_proc_path);
cleanup_stat:
    remove_proc_entry(APPID_PROC_DEBUG_NAME, appid_proc_path);
cleanup_debug:
    remove_proc_entry(APPID_PROC_DIR_NAME, NULL);
failed_out:
    return -1;
}

int app_proc_exit(void)
{
    remove_proc_entry(APPID_PROC_DEBUG_NAME, appid_proc_path);
    remove_proc_entry(APPID_PROC_SCENE_NAME, appid_proc_path);
    remove_proc_entry(APPID_PROC_STAT_NAME, appid_proc_path);
    remove_proc_entry(APPID_PROC_DIR_NAME, NULL);

    return 0;
}
