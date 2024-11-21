/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     21Mar13
 *
 *\warning
 *
 *\history \arg 0.0.1, 21Mar13, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mempool.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/netlink.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <net/sock.h>
#include <net/ip.h>
#include <net/netfilter/nf_conntrack.h>

#include "appidentify.h"
#include "appidentify_rules.h"
#include "appidentify_node.h"
#include "appidentify_id.h"
#include "appidentify_match_rules.h"
#include "appidentify_hash_table.h"
#include "appidentify_dpi_xml.h"

#include "appprio.h"
#include "appidentify_dns.h"
#include "appidentify_statistics.h"
#include "appidentify_log.h"
#include "dnsparser/dnsparse_testsuite.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define     NETLINK_TEST    (28)
#define     APPIDNTFY_PROC_NAME     "appidentify"

#define     APPIDNTFY_HASH_ELEM_NUM          \
            (APPIDNTFY_SUBTABLE_MAX * APPIDNTFY_TUPLE_INDEX_MAX * APPIDNTFY_HASH_TABLE_LEN)

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
extern int appidentify_hardcode_init(void);

/// proc_path
extern struct proc_dir_entry *appid_proc_path;

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
int                 appidntf_debug               = FALSE;
unsigned short      g_isAppidentifyInit          = FALSE;

APPIDNTFY_KER_RULE  *l_kernel_rule_tables        = NULL;  /* 核心规则表 */
mempool_t           *l_kernel_rule_pool          = NULL;

APPIDNTFY_HEAD      (*hash_sub_tables)[APPIDNTFY_TUPLE_INDEX_MAX][APPIDNTFY_HASH_TABLE_LEN] = NULL;

unsigned short      g_appidentifyCfgLock         = FALSE;
unsigned short      g_appidentifyHookLock        = FALSE;

static struct proc_dir_entry * appidntfy_proc_entry = NULL;

extern bool g_enablePort;
extern bool g_port_debug;

extern int app_stat_debug;
extern int g_app_stat_enable;
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
static DEFINE_MUTEX(appnl_mutex);
static char msg[128]={0};

static int appidentfy_proc_read(struct file * file, char *data, size_t len, loff_t *off)
{
    if(*off > 0)
    {
        APPIDNTF_ERROR("off is zero.\r\n");
        return 0;
    }
    if(copy_to_user(data,msg,strlen(msg)))
    {
        APPIDNTF_ERROR("copy_to_user error.\r\n");
        return -EFAULT;
    }
    *off += strlen(msg);
    return strlen(msg);
}

static int appidentfy_proc_write(struct file *file, const char *data, size_t len, loff_t *off)
{
    if(copy_from_user(msg,(void*)data,len))
    {
        APPIDNTF_ERROR("copy_from_user error.\r\n");
        return -EFAULT;
    }
    printk("MSG: %s.\r\n", msg);
    msg[len]='\0';
    if (!strcmp("debug_on\n", msg))
    {
        printk("debug_on\r\n");
        appidntf_debug = TRUE;
    }
    else if (!strcmp("debug_off\n", msg))
    {
        printk("debug_off\r\n");
        appidntf_debug = FALSE;
    }
    else if (!strcmp("prio_reset\n", msg))
    {
        appprio_cnt_reset();
    }
    else if (!strcmp("prio_debug\n", msg))
    {
        appprio_debug();
    }
    else if (!strcmp("prio_debug_on\n", msg))
    {
        printk("prio_debug_on\r\n");
        l_appprio_check_debug = TRUE;
    }
    else if (!strcmp("prio_debug_off\n", msg))
    {
        printk("prio_debug_off\r\n");
        l_appprio_check_debug = FALSE;
    }
    else if (!strcmp("prio_count_debug_off\n", msg))
    {
        g_count_debug = FALSE;
    }
    else if (!strcmp("prio_count_debug_on\n", msg))
    {
        g_count_debug = TRUE;
    }
    else if (!strcmp("prio_off\n", msg))
    {
        g_enablePrio = FALSE;
    }
    else if (!strcmp("prio_on\n", msg))
    {
        g_enablePrio = TRUE;
    }
    else if (!strcmp("dpi_off\n", msg))
    {
        g_enableDpi = FALSE;
    }
    else if (!strcmp("dpi_on\n", msg))
    {
        g_enableDpi = TRUE;
    }
    else if (!strcmp("dfi_off\n", msg))
    {
        g_enableClsf = FALSE;
    }
    else if (!strcmp("dfi_on\n", msg))
    {
        g_enableClsf = TRUE;
    }
    else if (!strcmp("port_off\n", msg))
    {
        g_enablePort = FALSE;
    }
    else if (!strcmp("port_on\n", msg))
    {
        g_enablePort = TRUE;
    }
    else if (!strcmp("port_debug_off\n", msg))
    {
        g_port_debug = FALSE;
    }
    else if (!strcmp("port_debug_on\n", msg))
    {
        g_port_debug = TRUE;
    }
    else if (!strcmp("dns_off\n", msg))
    {
        g_enableDns = FALSE;
    }
    else if (!strcmp("dns_on\n", msg))
    {
        g_enableDns = TRUE;
    }
    else if (!strcmp("dns_print\n", msg))
    {
        appidentify_dns_printall();
    }
    else if (!strcmp("dnskw_print\n", msg))
    {
        appidentify_dnskw_printall();
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
                printk("str: %s %s %s %s %s.\r\n", strElem[0], strElem[1], strElem[2], strElem[3], strElem[4]);
                ipaddr = in_aton(strElem[3]);
                appid = simple_strtol(strElem[4], NULL, 10);
                appidentify_dns_testentry_add(strElem[2], ipaddr, appid);
            }
        }
    }
    else if (!strcmp("app_stat\n", msg))
    {
        appidentify_statistics_print();
    }
    else if (!strcmp("app_stat_debug_off\n", msg))
    {
        app_stat_debug = FALSE;
    }
    else if (!strcmp("app_stat_debug_on\n", msg))
    {
        app_stat_debug = TRUE;
    }
    else if (!strcmp("app_stat_enable\n", msg))
    {
        g_app_stat_enable = TRUE;
    }
    else if (!strcmp("app_stat_disable\n", msg))
    {
        g_app_stat_enable = FALSE;
    }
    else
    {
        printk("MSG: %s len %d.\r\n", msg, strlen(msg));
    }
    return len;
}
#if 0
static int nl_data_ready(struct sk_buff *skb, struct nlmsghdr *nlh)
{
    printk("%s: received netlink message payload: %s \n", __FUNCTION__, (char*)NLMSG_DATA(nlh));

    printk("recvied finished!\n");
    return 0;
}
#endif
#if 0
static void appidentify_netlink_rcv(struct sk_buff *skb)
{

    printk("appidentify_netlink_rcv in.\n");
    mutex_lock(&appnl_mutex);
    printk("get lock skb resolving.\n");
    netlink_rcv_skb(skb, &nl_data_ready);
    mutex_unlock(&appnl_mutex);

#if 0
     struct sk_buff *skb;
     struct nlmsghdr *nlh;
     skb = skb_get (_skb);
     if(skb->len >= NLMSG_SPACE(0))
     {

        nlh = nlmsg_hdr(skb);

        if (nlh->nlmsg_len < NLMSG_HDRLEN || skb->len < nlh->nlmsg_len)
            printk("nl msg len %d, skb len %d, NLMSG_HDRLEN %d.\n",
                   nlh->nlmsg_len, skb->len, NLMSG_HDRLEN);

        /* Only requests are handled by the kernel */
        if (!(nlh->nlmsg_flags & NLM_F_REQUEST))
            goto ack;


           printk("Message received:%s\n",(char *)NLMSG_DATA(nlh)) ;

         kfree_skb(skb);
     }
     else
     {
         printk("skb->len < NLMSG_SPACE(0), %d < %d\n",
                skb->len, NLMSG_SPACE(0));
     }
    #endif
}
#endif
static int appidentify_hash_table_free(void)
{
    if (hash_sub_tables)
    {
        kfree(hash_sub_tables);
    }
    return 0;
}

static int appidentify_hash_table_init(void)
{
    int subTableIndex = 0;
    int ruleIndex = 0;
    int tupleIndex = 0;

    APPIDNTF_DEBUG("in.\r\n");

    hash_sub_tables = (APPIDNTFY_HEAD (*)[APPIDNTFY_TUPLE_INDEX_MAX][APPIDNTFY_HASH_TABLE_LEN])kmalloc(APPIDNTFY_HASH_ELEM_NUM
                                                                                                       * sizeof(APPIDNTFY_HEAD), GFP_KERNEL);

    if (NULL == hash_sub_tables)
    {
        return -1;
    }
    /*!
    *   初始化子表对就的哈希表.
    *  即:四个子表中,各四个哈希表,每个哈希表的所有元素.
    *  [4/25/2010]
    */
    for (subTableIndex = 0; subTableIndex < APPIDNTFY_SUBTABLE_MAX; ++subTableIndex)
    {
        for (tupleIndex = 0; tupleIndex < APPIDNTFY_TUPLE_INDEX_MAX; ++tupleIndex)
        {
            for(ruleIndex = 0; ruleIndex < APPIDNTFY_HASH_TABLE_LEN; ++ruleIndex)
            {
                INIT_HLIST_HEAD(&(hash_sub_tables[subTableIndex][tupleIndex][ruleIndex].head));
            }
        }
    }

    return 0;
}

static void appidentify_kernel_pool_destroy(void)
{
    if (l_kernel_rule_pool)
    {
        mempool_destroy(l_kernel_rule_pool);
    }
}


static int appidentify_kernel_pool_init(void)
{
    APPIDNTF_DEBUG("in.\r\n");

    l_kernel_rule_pool = mempool_create_kmalloc_pool(8, sizeof(APPIDNTFY_KER_RULE));
    if (NULL == l_kernel_rule_pool)
    {
        APPIDNTF_ERROR("mempool create error.\r\n");
        return -ENOMEM;
    }

    return 0;
}

int appidentify_update_db(void * pRuleBuf, int ruleNum, int len)
{
    int ret;
    APPMNGR_RULE *pAppRule = (APPMNGR_RULE *)pRuleBuf;

    if (len < sizeof(APPMNGR_RULE) * ruleNum)
    {
        APPID_ERR(APPID_BASE, "received datbase is too short");
        return -1;
    }

    g_isAppidentifyInit = FALSE;

    if ((ret = appidentify_cleanup_rules()))
    {
        APPID_ERR(APPID_BASE, "clear up rules failed!");
        return ret;
    }

    if ((ret = appidentify_add_new_rule(pAppRule, ruleNum)))
    {
        APPID_ERR(APPID_BASE, "add new rule failed!");
        return ret;
    }

    g_isAppidentifyInit = TRUE;
    return 0;

}

static int appidentify_init(void)
{
    int ret;
//    int ruleNum;

    g_isAppidentifyInit = FALSE;

    if ((ret = appidentify_kernel_pool_init()))
    {

        APPIDNTF_ERROR("appidentify_kernel_tbl_init error.\r\n");
        return ret;
    }
    APPIDNTF_DEBUG("appidentify_kernel_tbl_init over.\r\n");

    if ((ret = appidentify_hash_table_init()))
    {
        APPIDNTF_ERROR("appidentify_hash_table_init error.\r\n");
        return ret;
    }
    APPIDNTF_DEBUG("appidentify_hash_table_init over.\r\n");

	/* remove the hardcode identify */
#if 0
    if ((ret = appidentify_hardcode_init()))
    {
        APPIDNTF_ERROR("appidentify_hardcode_init error.\r\n");
        return ret;
    }
#endif

#if 0
    /* waiting for webserver setting to kernel */
    APPIDNTF_DEBUG("sizeof g_appRule %d, sizeof APPMNGR_RULE %d.\r\n", sizeof(g_appRule), sizeof(APPMNGR_RULE));
    ruleNum = sizeof(g_appRule)/sizeof(APPMNGR_RULE);
    if ((ret = appidentify_add_new_rule(g_appRule, ruleNum)))
    {
        APPIDNTF_ERROR("appidentify_add_new_rule error.\r\n");
        return ret;
    }

    g_isAppidentifyInit = TRUE;
#endif

    return 0;
}

static int appidentify_exit(void)
{
    int ret;

    g_isAppidentifyInit = FALSE;

    if ((ret = appidentify_cleanup_rules()))
    {
        APPIDNTF_ERROR("appidentify_cleanup_rules error.\r\n");
        return ret;
    }

    appidentify_hash_table_free();
    appidentify_kernel_pool_destroy();

    return 0;
}
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
#if 0
static struct nf_hook_ops appidentify_ops =
{
  .hook = appidentify_match_hook,
  .owner = THIS_MODULE,
  .pf = PF_INET,
  .hooknum = NF_INET_PRE_ROUTING,
  .priority = NF_IP_PRI_CONNTRACK + 1,
};

static struct nf_hook_ops appprio_ops =
{
  .hook = appprio_hook,
  .owner = THIS_MODULE,
  .pf = PF_INET,
  .hooknum = NF_INET_PRE_ROUTING,
  .priority = NF_IP_PRI_CONNTRACK + 2,
};
#endif

static struct file_operations appidntfy_proc_ops = {
       .read    = appidentfy_proc_read,
       .write   = appidentfy_proc_write,
};

/*int  appidentify_module_init(void)*/
int  appidentify_dpi_init(void)
{
#if 1
    appidntfy_proc_entry = proc_create(APPIDNTFY_PROC_NAME, 0666, appid_proc_path, &appidntfy_proc_ops);
    if(!appidntfy_proc_entry)
    {
        printk(KERN_ERR "can't create /proc/appid/appidentify \n");
        return -EFAULT;
    }
#endif

    if (appidentify_init())
    {
        printk("appidentify_init error.\r\n");
    }

    /* below apppri init
    if (appprio_init())
    {
        APPIDNTF_ERROR("appprio_init error.\r\n");
        return -EFAULT;
    } */

    return 0;
}

/*void  appidentify_module_fini(void)*/
void appidentify_dpi_fini(void)
{
    if (appidentify_exit())
    {
        APPIDNTF_ERROR("appidentify cleanup error.\r\n");
    }

    /*if (appprio_exit())
    {
        APPIDNTF_ERROR("appprio cleanup error.\r\n");
    }*/

    /* proc fs release */
	/* remove_proc_entry(APPIDNTFY_PROC_NAME, NULL); */
    remove_proc_entry(APPIDNTFY_PROC_NAME, appid_proc_path);

    printk("appidentify module exit ok!\n");
}
