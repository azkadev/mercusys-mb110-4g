/*  Copyright(c) 2009-2015 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		xt_app.c
 * brief	for iptables match
 *
 * author	Wang Lian
 * version	1.0.0
 * date		23Oct15
 *
 * history 	\arg 1.0.0, 23Oct15, Wang Lian, Create the file.
 */

#include <linux/module.h>
#include <linux/skbuff.h>

#include <linux/netfilter/xt_app.h>
#include <linux/netfilter/x_tables.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_app.h>


MODULE_AUTHOR("WANG LIAN <wanglian@tp-link.net>");
MODULE_DESCRIPTION("Xtables: packet app operations");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_app");
MODULE_ALIAS("ip6t_app");


static bool
app_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_app_info *info = par->matchinfo;
    struct nf_conn  *ct = NULL;
	enum ip_conntrack_info ctinfo;
    struct nf_conntrack_app *appinfo;
    short appid_ori, appid_rep;

	ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct)
    {
        /*APPIDNTF_ERROR("no nf_conn.");*/
        return -1;
    }
    appinfo = nf_ct_get_app(ct);
    if(NULL == appinfo)    {
        
        return -1;
    }
    appid_ori = appinfo->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index & 0x0FFF;
    appid_rep = appinfo->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index & 0x0FFF;	
	
	/*if(appid_ori == (info->id & 0x0FFF) || appid_rep == (info->id & 0x0FFF)){
		printk("APP ID match: appid_ori = %d, appid_rep = %d, info_id = %d\r\n", appid_ori, appid_rep, info->id);
	}*/

	return (appid_ori == (info->id & 0x0FFF) || appid_rep == (info->id & 0x0FFF));
}


static struct xt_match app_mt_reg __read_mostly = {
	.name           = "app",	
	.family         = NFPROTO_UNSPEC,
	.match          = app_mt,
	.matchsize      = sizeof(struct xt_app_info),
	.me             = THIS_MODULE,
};

static int __init app_mt_init(void)
{
	int ret;
	
	ret = xt_register_match(&app_mt_reg);
	if (ret < 0) {		
		return ret;
	}
	return 0;
}

static void __exit app_mt_exit(void)
{
	xt_unregister_match(&app_mt_reg);	
}

module_init(app_mt_init);
module_exit(app_mt_exit);
