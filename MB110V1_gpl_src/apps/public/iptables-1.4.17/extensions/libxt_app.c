/*  Copyright(c) 2009-2015 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		libxt_app.c
 * brief	for iptables match
 *
 * author	Wang Lian
 * version	1.0.0
 * date		23Oct15
 *
 * history 	\arg 1.0.0, 23Oct15, Wang Lian, Create the file.
 */

#include <stdbool.h>
#include <stdio.h>
#include <xtables.h>
#include <linux/netfilter/xt_app.h>

/*
struct xt_app_info {
	unsigned short id;	
};*/

enum {
	O_ID = 0,
};

static void app_mt_help(void)
{
	printf(
"app match options:\n"
"[!] --id value    Match appid value (0-4096)\n");
}

static const struct xt_option_entry app_mt_opts[] = {
	{.name = "id", .id = O_ID, .type = XTTYPE_UINT16,
	 .min = 0, .max = XT_APPID_MAX, .flags = XTOPT_MAND,
	XTOPT_POINTER(struct xt_app_info, id) },
	XTOPT_TABLEEND,
};

static void app_mt_parse(struct xt_option_call *cb)
{
	struct xt_app_info *info = cb->data;

	xtables_option_parse(cb);
	info->id = cb->val.u16; 
}

static void app_mt_check(struct xt_fcheck_call *cb)
{
	if (cb->xflags == 0)
		xtables_error(PARAMETER_PROBLEM,
		           "APPID match: Parameter --id is required");
}

static void
app_mt_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_app_info *info =
		(const struct xt_app_info *)match->data;
	printf(" APPID match %d", info->id);
}

static void app_mt_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_app_info *info =
		(const struct xt_app_info *)match->data;

	printf("--id %d", info->id);
}

static struct xtables_match app_match = {
	.family		= NFPROTO_UNSPEC,
	.name 		= "app",
	.version 	= XTABLES_VERSION,
	.size 		= XT_ALIGN(sizeof(struct xt_app_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_app_info)),
	.help		= app_mt_help,
	.print		= app_mt_print,
	.save		= app_mt_save,
	.x6_parse	= app_mt_parse,
	.x6_fcheck	= app_mt_check,
	.x6_options	= app_mt_opts,
};

void _init(void)
{
	xtables_register_match(&app_match);
}
