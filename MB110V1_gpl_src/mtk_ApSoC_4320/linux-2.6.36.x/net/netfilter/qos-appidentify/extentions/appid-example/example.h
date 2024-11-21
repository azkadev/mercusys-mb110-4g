/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:   example.h
 * Version		:	1.0
 * Abstract		:   appidentify extention example module
 * Author		:	Li Wenxiang  (liwenxiang@tp-link.net)
 *
 ***************************************************************/

#ifndef __APPID_EXAMPLE_H__
#define __APPID_EXAMPLE_H__

#include <linux/module.h>
#include <net/netfilter/nf_conntrack_app.h>
#include <net/netfilter/appidentify_api.h>

#define EXAMPLE_MODULE_NAME "example"
#define EXAMPLE_PROC_NAME   "example"

unsigned appid_example_hook(unsigned int hooknum, struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));
int example_read(struct file * file, char *data, size_t len, loff_t *off);
int example_write(struct file *file, const char *data, size_t len, loff_t *off);


#endif // __APPID_EXAMPLE_H__
