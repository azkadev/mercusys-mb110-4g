/**
 * Copyright (C), 2014, TP-LINK TECHNOLOGIES CO., LTD.
 *
 * @brief    QoS - Small Packets First
 * @author   Li Zheng  <lizheng_w5625@tp-link.net>
 * @date     2014-06-24
 */
/**
 * INCLUDE_FILES
 */
#include "appid_spf.h"
//#include <linux/module.h>
//#include <linux/version.h>
//#include <linux/kmod.h>
//#include <linux/spinlock.h>
//#include <linux/semaphore.h>
//#include <linux/fs.h>
//
//#include <net/sock.h>
//#include <net/ip.h>
//#include <net/dsfield.h>
//#include <net/netfilter/nf_conntrack.h>

/**
 * DEFINES
 */
#define PROC_MSG_LEN        (128)

/**
 * TYPES
 */
/**
 * EXTERN_PROTOTYPES
 */
/**
 * LOCAL_PROTOTYPES
 */
//static void DEBUG_PRINT_SPF_TUPLE(SPF_TUPLE *tuple);

/* netlink */
static void spf_get(void);
static void spf_set(const char *msg);

/**
 * VARIABLES
 */
static bool g_spf_enable = true;
static bool g_spf_inited = false;

static unsigned int spf_period          = 10; /* 10 seconds */
static unsigned int threshold_packets   = 100;
static unsigned int threshold_avg_pkt   = 80;
static unsigned int big_avg_pkt_factor  = 4;
static unsigned int trust_factor        = 5;

static char msg[PROC_MSG_LEN] = {0};

static struct appid_module spf_module = {
    .name = SPF_MODULE_NAME,
    .hook = appid_spf_hook
};

static struct proc_dir_entry *spf_proc;
static struct file_operations spf_proc_ops = {
    .read    = spf_read,
    .write   = spf_write
};

/**
 * LOCAL_FUNCTIONS
 */
/*
 * This function did not handle the lock.
 */
//inline static void DEBUG_PRINT_SPF_TUPLE(SPF_TUPLE *tuple)
//{
//	SPF_DEBUG("tuple: %d.%d.%d.%d:%d---%d.%d.%d.%d:%d, proto %d.\n",
//			(tuple->src_ip >> 24) & 0xFF, (tuple->src_ip >> 16) & 0xFF,
//			(tuple->src_ip >> 8) & 0xFF,   tuple->src_ip & 0xFF,
//			tuple->src_port,
//			(tuple->dst_ip >> 24) & 0xFF, (tuple->dst_ip >> 16) & 0xFF,
//			(tuple->dst_ip >> 8) & 0xFF,   tuple->dst_ip & 0xFF,
//			tuple->dst_port, tuple->protonum);
//}

static void spf_get(void)
{
    printk("*************SPF************\n");
    printk("threshold_packets: %d\n", threshold_packets);
    printk("threshold_avg_pkt: %d\n", threshold_avg_pkt);
    printk("big_avg_pkt_factor: %d\n", big_avg_pkt_factor);
    printk("trust_factor: %d\n", trust_factor);
    printk("spf_period: %d\n", spf_period);
    printk("\n");

    printk("Set with input:\n");
    printk("spf_set_[threshold_packets]_[threshold_avg_pkt]_[big_avg_pkt_factor]_[trust_factor]_[spf_period]\n");
    printk("Fixed sequence, varied args, the number base is 10\n");
    printk("like this: spf_set_100_80_4_5_10\n");
    printk("or like this: spf_set_100_80\n");
    printk("\n");
}

static void spf_set(const char *msg)
{
    int val[5];
    int i;

    i = 0;
    while ((*msg) != '\n' && (*msg) != '\0')
    {
        if (i > 4)
            break;

        if (isdigit(*msg))
        {
            val[i] = simple_strtoul(msg, (char **)&msg, 10);
            i++;
        }
        else
        {
            msg++;
        }
    }

    switch (i)
    {
        case 5:
            spf_period = val[4];
        case 4:
            trust_factor = val[3];
        case 3:
            big_avg_pkt_factor = val[2];
        case 2:
            threshold_avg_pkt = val[1];
        case 1:
            threshold_packets = val[0];
            printk("SPF set OK!\n");
            break;
        default:
            printk("SPF set error.\n");
            break;
    }
}


static int __init spf_init(void)
{
    int ret;

    /* register module */
    ret = appid_register_module(SPF_MODULE_NAME, &spf_module);
    if (-1 == ret)
    {
        printk(KERN_ALERT "register module failed!\n");
        goto failed_out;
    }

    /* create proc failde under /proc/appid/ */
    spf_proc = proc_create(SPF_PROC_NAME, 0666, appid_proc_path, &spf_proc_ops);
    if (!spf_proc)
    {
        printk(KERN_ALERT "create proc file failed!\n");
        goto create_proc_failed;
    }

    g_spf_inited = true;
    printk(KERN_ALERT "appid spf module init success!\n");
    return 0;

create_proc_failed:
    appid_unregister_module(SPF_MODULE_NAME);
failed_out:
    return -1;
}

static void __exit spf_exit(void)
{
    g_spf_inited = false;

    // destory proc file
    remove_proc_entry(SPF_MODULE_NAME, appid_proc_path);

    // unregister_module
    appid_unregister_module(SPF_MODULE_NAME);

    printk(KERN_ALERT "appid example module exit success!\n");
}

module_init(spf_init);
module_exit(spf_exit);

MODULE_AUTHOR("Li Zheng <lizheng_w5625@tp-link.net>");
MODULE_DESCRIPTION("QoS - Small Packets First");
MODULE_LICENSE("GPL v2");

/**
 * PUBLIC_FUNCTIONS
 */
unsigned int appid_spf_hook(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    enum ip_conntrack_dir		dir;
    enum ip_conntrack_info		ctinfo;
    struct nf_conn				*ct = NULL;
    struct nf_conn_counter		*acct = NULL;
    struct nf_conntrack_app		*app_info = NULL;
    struct nf_conntrack_spf		*spf_info = NULL;

    unsigned long long			recent_packets;
    unsigned long long			avg_len_pkt;

	//printk("in spf hook #1\r\n");
	
    if (!g_spf_inited)
    {
        return NF_ACCEPT;
    }
	
    if (false == g_spf_enable)
    {
        return NF_ACCEPT;
    }
		
    ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct)
    {
        return NF_ACCEPT;
    }

    app_info = nf_ct_get_app(ct);
    if (NULL == app_info)
    {
        APPID_ERR(SPF_MODULE_NAME, "can not get app info");
        return NF_ACCEPT;
    }
	
    /* check if the flow has already been recongnized */
    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_PRI_SET) ||
        APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER))
    {		
        return NF_ACCEPT;
    }
	
	//printk("in spf hook prio flag #5\r\n");
	
    dir = CTINFO2DIR(ctinfo);
	
	/*printk("SPF appid_ori=0x%x, appid_rep=0x%x\r\n", 
		app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index,
		app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index );*/
	
    /* check if the flow is processing */
    if (APP_NORMAL_ID_UNKNOWN != APP_NORMAL_ID_GET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index) ||
        APP_NORMAL_ID_UNKNOWN != APP_NORMAL_ID_GET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index))
    {
        //return NF_ACCEPT;
    }
	
	//printk("in spf hook app id #6\r\n");
	
    acct = nf_conn_acct_find(ct);
    if (NULL == acct)
    {
        return NF_ACCEPT;
    }
	
    spf_info = nf_conn_spf_find(ct);
    if (NULL == spf_info)
    {
        return NF_ACCEPT;
    }
	
    /* init spf */
    if (0 == spf_info[dir].time)
    {
        spf_info[dir].time = jiffies;
        atomic64_set(&spf_info[dir].packets,
                atomic64_read(&acct[dir].packets));
        atomic64_set(&spf_info[dir].bytes,
                atomic64_read(&acct[dir].bytes));

        return NF_ACCEPT;
    }
	
    if (spf_info[dir].factor >= trust_factor)
    {
        return NF_ACCEPT;
    }
	
    if ((jiffies - spf_info[dir].time) > (spf_period * HZ))
    {
		//printk("in spf hook checking #11\r\n");
		
        recent_packets = atomic64_read(&acct[dir].packets) -
            atomic64_read(&spf_info[dir].packets);

        if (recent_packets > threshold_packets)
        {
            avg_len_pkt = atomic64_read(&acct[dir].bytes) -
                atomic64_read(&spf_info[dir].bytes);
            do_div(avg_len_pkt, recent_packets);

            APPID_LOG(SPF_MODULE_NAME, "%lu, recent pkts: %llu, avg_len: %llu, dir: %s",
                    jiffies, recent_packets, avg_len_pkt, (!dir)?"ori":"reply");

            if (avg_len_pkt < threshold_avg_pkt)
            {
                spf_info[dir].factor++;
                APPID_LOG(SPF_MODULE_NAME, "factor: %u", spf_info[dir].factor);
                if (spf_info[dir].factor >= trust_factor)
                {
                    if (spf_info[!dir].factor >= trust_factor)
                    {
                        APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, APP_ID_SPF);
                        APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, APP_ID_SPF);
                        APPID_LOG(SPF_MODULE_NAME, "set appid to %d", APP_ID_SPF);
                    }
                }
            }
            else if (spf_info[!dir].factor >= trust_factor)
            {
                if (avg_len_pkt < threshold_avg_pkt * big_avg_pkt_factor)
                {
                    spf_info[dir].factor++;
                    APPID_LOG(SPF_MODULE_NAME, "big factor: %u", spf_info[dir].factor);
                    if (spf_info[dir].factor >= trust_factor)
                    {
                        APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, APP_ID_SPF);
                        APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, APP_ID_SPF);
                        APPID_LOG(SPF_MODULE_NAME, "set appid to %d", APP_ID_SPF);
                    }
                }
            }
            else
            {
                spf_info[dir].factor = 0;
            }
        }
        else
        {
            spf_info[dir].factor = 0;
        }

        spf_info[dir].time = jiffies;
        atomic64_set(&spf_info[dir].packets,
                atomic64_read(&acct[dir].packets));
        atomic64_set(&spf_info[dir].bytes,
                atomic64_read(&acct[dir].bytes));
    }
	
	//printk("in spf hook end #12\r\n");
	
    return NF_ACCEPT;
}

int spf_read(struct file * file, char *data, size_t len, loff_t *off)
{
    if(*off > 0)
    {
        APPID_LOG(SPF_MODULE_NAME, "off is zero");
        return 0;
    }
    if(copy_to_user(data, msg, strlen(msg)))
    {
        APPID_LOG(SPF_MODULE_NAME, "copy_to_user error");
        return -EFAULT;
    }
    *off += strlen(msg);
    return strlen(msg);
}

int spf_write(struct file *file, const char *data, size_t len, loff_t *off)
{
    if(copy_from_user(msg, (void*)data, len))
    {
        APPID_LOG(SPF_MODULE_NAME, "copy_to_user error");
        return -EFAULT;
    }
    printk("MSG: %s.\n", msg);
    msg[len]='\0';

    if (!strcmp("spf_get\n", msg))
    {
        spf_get();
    }
    else if (!strncmp("spf_set\n", msg, 7))
    {
        spf_set(msg);
    }
    else if (!strcmp("spf_enable\n", msg))
    {
        g_spf_enable = true;
    }
    else if (!strcmp("spf_disable\n", msg))
    {
        g_spf_enable = false;
    }
    else
    {
        printk("MSG: %s len %d.\n", msg, strlen(msg));
    }

    return len;
}
