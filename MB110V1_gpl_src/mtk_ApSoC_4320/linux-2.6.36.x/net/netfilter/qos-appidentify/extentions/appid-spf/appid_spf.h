#ifndef __QOS_SPF__
#define __QOS_SPF__

//#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/module.h>
#include <net/netfilter/nf_conntrack_app.h>
#include <net/netfilter/appidentify_api.h>
#include <net/netfilter/nf_conntrack_spf.h>
#include <net/netfilter/nf_conntrack_acct.h>

#define SPF_MODULE_NAME		"spf"
#define SPF_PROC_NAME		"spf"

unsigned appid_spf_hook(unsigned int hooknum, struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));
int spf_read(struct file * file, char *data, size_t len, loff_t *off);
int spf_write(struct file *file, const char *data, size_t len, loff_t *off);


#endif /* end of include guard: __QOS_SPF__ */
