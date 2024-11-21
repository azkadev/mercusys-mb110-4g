/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:   example.c
 * Version		:	1.0
 * Abstract		:   appidentify extention example module
 * Author		:	Li Wenxiang  (liwenxiang@tp-link.net)
 *
 ***************************************************************/

#include "example.h"

// module struct
static struct appid_module example_module = {
    .name = EXAMPLE_MODULE_NAME,
    .hook = appid_example_hook
};


// proc
static struct proc_dir_entry *example_proc;
static struct file_operations example_proc_ops = {
    .read    = example_read,
    .write   = example_write
};

static int example_id;

static int __init example_init(void)
{
    int ret;

    // register module
    ret = appid_register_module(EXAMPLE_MODULE_NAME, &example_module);
    if (-1 == ret)
    {
        printk(KERN_ALERT "register module failed!\n");
        goto failed_out;
    }

    // allocate a appid
    example_id = appid_alloc_id(APPPRIO_FIRST_PRIO);
    if (-1 == example_id)
    {
        printk(KERN_ALERT "allocate appid failed!\n");
        goto alloc_id_failed;
    }

    // create proc failde under /proc/appid/
    example_proc = proc_create(EXAMPLE_PROC_NAME, 0666, appid_proc_path, &example_proc_ops);
    if (!example_proc)
    {
        printk(KERN_ALERT "create proc file failed!\n");
        goto create_proc_failed;
    }

    printk(KERN_ALERT "appid example module init success!\n");
    return 0;

create_proc_failed:
    appid_free_id(example_id);
alloc_id_failed:
    appid_unregister_module(EXAMPLE_MODULE_NAME);
failed_out:
    return -1;
}

static void __exit example_exit(void)
{
    // destory proc file
    remove_proc_entry(EXAMPLE_PROC_NAME, appid_proc_path);

    // free appid
    appid_free_id(example_id);

    // unregister_module
    appid_unregister_module(EXAMPLE_MODULE_NAME);

    printk(KERN_ALERT "appid example module exit success!\n");
}

module_init(example_init);
module_exit(example_exit);


// hook function
unsigned appid_example_hook(unsigned int hooknum, struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    struct nf_conn*          ct_entry = NULL;
    struct nf_conntrack_app* app_info;
    enum ip_conntrack_info   ctinfo;

    // get conntrack info
    ct_entry = nf_ct_get(skb, &ctinfo);
    if (NULL == ct_entry)
    {
        APPID_ERR(EXAMPLE_MODULE_NAME, "can not get ct info");
        return NF_ACCEPT;
    }

    // get app info
    app_info = nf_ct_get_app(ct_entry);
    if ( NULL == app_info)
    {
        APPID_ERR(EXAMPLE_MODULE_NAME, "can not get app info");
        return NF_ACCEPT;
    }

    // check if the flow has already been recongnized
    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PRI_SET) ||
        APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER))
    {
        return NF_ACCEPT;
    }

    if (APP_NORMAL_ID_UNKNOWN == APP_NORMAL_ID_GET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index) &&
        APP_NORMAL_ID_UNKNOWN == APP_NORMAL_ID_GET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index))
    {
        APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, example_id);
        APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, example_id);
        APPID_LOG(EXAMPLE_MODULE_NAME, "set appid to %d", example_id);
    }

    return NF_ACCEPT;
}

int example_read(struct file * file, char *data, size_t len, loff_t *off)
{
    return len;
}

int example_write(struct file *file, const char *data, size_t len, loff_t *off)
{
    return len;
}
