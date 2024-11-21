/*! Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     app_mod.c
 *\brief
 *\details
 *
 *\author   Weng Kaiping
 *\version
 *\date     17Oct13
 *
 *\warning
 *
 *\history \arg
 */

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/module.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <asm/page.h>
#include <linux/vmstat.h>
#include <linux/mmzone.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_app.h>

#include "appidentify_api.h"
#include "appidentify_log.h"
#include "appidentify_proc.h"
#include "appidentify_utils.h"
#include "appidentify_netlink.h"
#include "appprio.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define ENABLE_DPI                  (TRUE)
#define ENABLE_DFI                  (TRUE)
#define ENABLE_SECNE_MODE           (TRUE)
#define ENABLE_STATS                (TRUE)
#define ENABLE_DNS                  (TRUE)
#define ENABLE_UPNP                 (TRUE)
#define ENABLE_SPF                  (TRUE)
#define ENABLE_PORT                 (FALSE)
#define ENABLE_DEBUG                (TRUE)
#define ENABLE_PRIO                 (TRUE)

bool enable_dpi = 1;
bool enable_dfi = 0;
bool enable_dns = 1;
bool enable_upnp = 0;
bool enable_spf = 0;
bool enable_port = 0;
bool enable_debug = 1;
bool enable_prio = 0;
bool enable_stats = 1;
bool enable_scene_mode = 0;

bool enable_print = 0;

module_param_named(dpi, enable_dpi, bool, 0644);
MODULE_PARM_DESC(dpi, "Enable dpi.");

module_param_named(dfi, enable_dfi, bool, 0644);
MODULE_PARM_DESC(dfi, "Enable dfi.");

module_param_named(dns, enable_dns, bool, 0644);
MODULE_PARM_DESC(dns, "Enable dns.");

module_param_named(upnp, enable_upnp, bool, 0644);
MODULE_PARM_DESC(upnp, "Enable upnp.");

module_param_named(spf, enable_spf, bool, 0644);
MODULE_PARM_DESC(spf, "Enable spf.");

module_param_named(port, enable_port, bool, 0644);
MODULE_PARM_DESC(port, "Enable port.");

module_param_named(debug, enable_debug, bool, 0644);
MODULE_PARM_DESC(debug, "Enable debug.");

module_param_named(prio, enable_prio, bool, 0644);
MODULE_PARM_DESC(prio, "Enable prio.");

module_param_named(stats, enable_stats, bool, 0644);
MODULE_PARM_DESC(stats, "Enable stats.");

module_param_named(scene, enable_scene_mode, bool, 0644);
MODULE_PARM_DESC(scene, "Enable scene mode.");

module_param_named(print, enable_print, bool, 0644);
MODULE_PARM_DESC(print, "Enable print.");

/**************************************************************************************************/
/*                                      CUSTOM_INCLUDE_FILES                                      */
/**************************************************************************************************/

#if ENABLE_DNS
    #include "appidentify_dns.h"
#endif

#if ENABLE_DPI
    #include "appidentify_match_rules.h"
    #include "appidentify.h"
#endif

#if ENABLE_STATS
    #include "appidentify_statistics.h"
#endif

#if ENABLE_SECNE_MODE
    #include "appidentify_scene.h"
#endif

#if ENABLE_DFI
    #include "appidentify_flow.h"
#endif

#if ENABLE_UPNP
    #include "appidentify_upnp.h"
#endif

#if ENABLE_PORT
	#include "appidentify_port.h"
#endif
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

#if ENABLE_PORT
    nf_conntrack_app_handler_t appidentify_handler ={
        .dec_port_counter = dec_port_counter,
    };
#endif

#if ENABLE_SPF
    extern struct list_head appid_module_list;
#endif
/**************************************************************************************************/
/*                                      FUNCTION                                                */
/**************************************************************************************************/

#if ENABLE_SPF
static void appid_sub_modules_callhook(unsigned int hooknum,
                                       struct sk_buff *skb,
                                       const struct net_device *in,
                                       const struct net_device *out,
                                       int (*okfn)(struct sk_buff *))
{
    struct appid_module *entry;
	
	//printk("APPID: entering spf. #0\r\n");
    list_for_each_entry(entry, &appid_module_list, list) {
        entry->hook(hooknum, skb, in, out, okfn);
		//printk("APPID: entering spf. #0.1\r\n");
    }
}
#endif

unsigned int app_set_local(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
    struct nf_conn     *pCtEntry = NULL;
    struct nf_conntrack_app * app_info;
    enum ip_conntrack_info ctinfo;

    pCtEntry = nf_ct_get(skb, &ctinfo);
    if ( NULL == pCtEntry )
    {
        return NF_ACCEPT;
    }

    app_info = nf_ct_get_app(pCtEntry);
    if ( NULL == app_info)
    {
        return NF_ACCEPT;
    }

    APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER);

    return NF_ACCEPT;
}


unsigned int appidntfy_hook(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
#if 1
	struct nf_conn          *ct         = NULL;    
    enum ip_conntrack_info  ctinfo;
    struct nf_conntrack_app *appinfo;    
	short appid_ori, appid_rep;
	
	ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct)
    {
        return NF_ACCEPT;
    }
	
    appinfo = nf_ct_get_app(ct);
    if(NULL == appinfo)
    {
        printk("no appidntf_info.");
        return NF_ACCEPT;
    }
#endif
	
	/*printk("#1 before hooks, appid_ori=0x%x, appid_rep=0x%x\r\n", 
		   appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index,
		   appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index );*/
	
#if ENABLE_UPNP
	if(enable_upnp)
	{
		app_upnp_ct_check(hooknum, skb, in, out, okfn);
	}
#endif

#if ENABLE_PORT
	if(enable_port)
	{
		appidentify_port_hook(hooknum, skb, in, out, okfn);
	}
#endif

#if ENABLE_DNS
	if(enable_dns)
	{
		appidentify_dns_hook(hooknum, skb, in, out, okfn);
	}
#endif

#if ENABLE_DPI
	if(enable_dpi)
	{
		appidentify_match_hook(hooknum, skb, in, out, okfn);
	}
#endif

#if ENABLE_DFI
	if(enable_dfi)
	{
		app_flow_statistic_record(hooknum, skb, in, out, okfn);
	}
#endif

#if ENABLE_SPF
	/*printk("#2 before spf, appid_ori=0x%x, appid_rep=0x%x\r\n", 
		   appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index,
		   appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index );*/
	
	if(enable_spf)
	{
		appid_sub_modules_callhook(hooknum, skb, in, out, okfn);
	}
#endif

#if ENABLE_PRIO
	if(enable_prio)
	{
		appprio_hook(hooknum, skb, in, out, okfn);
	}
#endif

#if 1
	appid_ori = appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index;
	appid_rep = appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index & 0x0FFF; 
	if( enable_print && (APP_NORMAL_ID_GET(appid_ori) != APP_NORMAL_ID_CHECKING)
			&& ( APP_NORMAL_ID_GET(appid_ori) != APP_NORMAL_ID_UNKNOWN ) )
	{		
		printk("APPID: after all hooks, appid_ori = %d, appid_rep = %d\r\n", appid_ori, appid_rep);	
	}
#endif
	
#if ENABLE_STATS
	if(enable_stats &&
			(APP_NORMAL_ID_GET(appid_ori) != APP_NORMAL_ID_CHECKING)
			&& ( APP_NORMAL_ID_GET(appid_ori) != APP_NORMAL_ID_UNKNOWN ) )
	{
		appidentify_statistics_hook(hooknum, skb, in, out, okfn);
	}
#endif

    return NF_ACCEPT;
}

static struct nf_hook_ops appidntfy_ops[] =  {
    {
        .hook       = appidntfy_hook,
        .owner      = THIS_MODULE,
        .pf         = PF_INET,
        .hooknum    = NF_INET_POST_ROUTING,
        .priority   = NF_IP_PRI_CONNTRACK + 1,
    },
    {
        .hook       = app_set_local,
        .owner      = THIS_MODULE,
        .pf         = PF_INET,
        .hooknum    = NF_INET_LOCAL_IN,
        .priority   = NF_IP_PRI_CONNTRACK + 1,
    },
    {
        .hook       = app_set_local,
        .owner      = THIS_MODULE,
        .pf         = PF_INET,
        .hooknum    = NF_INET_LOCAL_OUT,
        .priority   = NF_IP_PRI_CONNTRACK + 1,
    }
};

static int __init app_init(void)
{
    int ret = 0;

#if ENABLE_DEBUG	
	if(enable_debug)
	{
		ret = app_proc_init();
		if (ret < 0)
		{
			APPID_ERR(APPID_BASE, "procfs init failed");
			goto cleanup_proc;
		}
	}
#endif

#if ENABLE_DFI
	if(enable_dfi)
	{
		ret = app_flow_init();
		if (ret < 0)
		{
			printk("app flow int fail\n");
			goto cleanup_flow;
		}
	}
#endif

#if ENABLE_UPNP
	if(enable_upnp)
	{
		ret = app_upnp_init();
		if (ret < 0)
		{
			printk("app upnp int fail\n");
			goto cleanup_upnp;
		}
	}
#endif

#if ENABLE_DNS
	if(enable_dns)
	{
		ret = appidentify_dns_init();
		if (ret < 0)
		{
			printk("app dns int fail\n");
			goto cleanup_dns;
		}
	}
#endif

#if ENABLE_DPI
	if(enable_dpi)
	{
		ret = appidentify_dpi_init();
		if (ret < 0)
		{
			printk("appidentify_dpi_init fail\n");
			goto cleanup_appidnt_dpi;
		}
	}
#endif

#if ENABLE_PRIO
	if(enable_prio)
	{
		ret = appprio_init();
		if (ret < 0)
		{
			printk("appprio_init fail\n");
			goto cleanup_appidnt_prio;
		}
	}
#endif

#if ENABLE_SECNE_MODE
	if(enable_scene_mode)
	{
		ret = app_scene_init();
		if (ret < 0)
		{
			APPID_ERR(APPID_BASE, "scene init failed");
			goto cleanup_scene;
		}
	}
#endif

#if ENABLE_PORT
	if(enable_port)
	{
		nf_ct_app_handler = &appidentify_handler;

		ret = appidentify_port_init();
		if (ret < 0)
		{
			printk("appidentify_port_init fail\n");
			goto cleanup_appport;
		}
	}
#endif

    ret = nf_register_hooks(appidntfy_ops, ARRAY_SIZE(appidntfy_ops));
    if (ret < 0) {
        printk("can't register hooks.\n");
        goto cleanup_hooks;
    }

#if ENABLE_STATS
	if(enable_stats)
	{
		ret = appidentify_statistics_init();
		if (ret < 0)
		{
			printk("appidentify_port_init fail\n");
			goto cleanup_appstat;
		}
	}
#endif

    ret = app_netlink_init();
    if (ret < 0)
    {
        APPID_ERR(APPID_NETLINK, "init failed");
        goto cleanup_nl;
    }

    APPID_LOG(APPID_BASE, "appidentify init success");
    return 0;

cleanup_nl:
    app_netlink_fini();

#if ENABLE_STATS
cleanup_appstat:
    appidentify_statistics_exit();
#endif

cleanup_hooks:
    nf_unregister_hooks(appidntfy_ops, ARRAY_SIZE(appidntfy_ops));

#if ENABLE_PORT
cleanup_appport:
    appidentify_port_exit();
    nf_ct_app_handler= NULL;
#endif

#if ENABLE_SECNE_MODE
cleanup_scene:
    app_scene_exit();
#endif

#if ENABLE_PRIO
cleanup_appidnt_prio:
    appprio_exit();
#endif
	
#if ENABLE_DPI
cleanup_appidnt_dpi:
    appidentify_dpi_fini();
#endif

#if ENABLE_DNS
cleanup_dns:
    appidentify_dns_exit();
#endif

#if ENABLE_UPNP
cleanup_upnp:
    app_upnp_exit();
#endif

#if ENABLE_DFI
cleanup_flow:
    app_flow_exit();
#endif

#if ENABLE_DEBUG
cleanup_proc:
    app_proc_exit();
#endif
    return -1;
}


static void __exit app_exit(void)
{
    nf_unregister_hooks(appidntfy_ops, ARRAY_SIZE(appidntfy_ops));
    app_netlink_fini();

#if ENABLE_PORT
	if(enable_port)
	{
		appidentify_port_exit();
		nf_ct_app_handler = NULL;
	}
#endif

#if ENABLE_STATS
	if(enable_stats)
	{
		appidentify_statistics_exit();
	}
#endif

#if ENABLE_PRIO
	if(enable_prio)
	{
		appprio_exit();
	}
#endif

#if ENABLE_DPI
    appidentify_dpi_fini();
#endif

#if ENABLE_DNS
	if(enable_dns)
	{
		appidentify_dns_exit();
	}
#endif

#if ENABLE_DFI
	if(enable_dfi)
	{
		app_flow_exit();
	}
#endif

#if ENABLE_UPNP
	if(enable_upnp)
	{
		app_upnp_exit();
	}
#endif

#if ENABLE_SECNE_MODE
	if(enable_scene_mode)
	{
		app_scene_exit();
	}
#endif

#if ENABLE_DEBUG
	if(enable_debug)
	{
		app_proc_exit();
	}
#endif

    APPID_LOG(APPID_BASE, "appidentify exit success");
}

module_init(app_init);
module_exit(app_exit);
MODULE_LICENSE("GPL");