/* 
 *
 * file		libxt_massurl.c
 * brief		
 * details	
 *
 * author	wangwenhao
 * version	
 * date		25Oct11
 *
 * history \arg	1.0, 25Oct11, wangwenhao, create file
 */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <iptables.h>

#include <linux/netfilter/xt_massurl.h>
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define TYPE 0x1
#define URLS 0x2

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
static const struct option opts[] = {
	{ .name = "type", .has_arg = true, .val = 0},
    { .name = "urls", .has_arg = true, .val = 1},
    XT_GETOPT_TABLEEND,
};

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
static void help(void)
{
    printf(
    "massurl v%s set url match index-bits in rules. Options:\n"
    "  --type	with match type, now only 'url', 'http' and 'dns'\n"
    "			http: match req pkt with appinted url and other pkt\n"
    "			url: match only req pkt with url\n"
    "			dns: match dns pkt with appinted url\n"
    "  --urls	with hex-value of url index bits\n"
    , MASSURL_VERSION);
}

static void init(struct xt_entry_match *m)
{
	struct xt_massurl_info *info = (void *)m->data;

	memset(info, 0, sizeof(struct xt_massurl_info));
}

static int parse(int c, char **argv, int invert, unsigned int *flags,
                      const void *entry, struct xt_entry_match **match)
{
	struct xt_massurl_info *info = (void *)(*match)->data;
	char *pcIndex;
	unsigned int index;
	char ch ;
	unsigned int v;

	if (c == 0)
	{
		if (*flags & TYPE)
		{
			xtables_error(PARAMETER_PROBLEM,
						"Cannot specify --type twice");
		}
		
		if (invert)
		{
			xtables_error(PARAMETER_PROBLEM,
						"Unexpected \"!\" with --type");
		}

		if (strcmp(optarg, "http") == 0)
		{
			info->type = MASSURL_TYPE_HTTP;
		}
		else if (strcmp(optarg, "url") == 0)
		{
			info->type = MASSURL_TYPE_URL;
		}
		else if (strcmp(optarg, "dns") == 0)
		{
			info->type = MASSURL_TYPE_DNS;
		}
		else
		{
			xtables_error(PARAMETER_PROBLEM,
						"Only 'http', 'url' or 'dns' available with --type");
		}
		*flags |= TYPE;
	}
	else if (c == 1)
	{
		if (*flags & URLS)
		{
			xtables_error(PARAMETER_PROBLEM,
						"Cannot specify --urls twice");
		}
		
		if (invert)
		{
			xtables_error(PARAMETER_PROBLEM,
						"Unexpected \"!\" with --urls");
		}
		
		if (((strlen(optarg) + 7) >> 3) != MASSURL_INDEX_WWORDS)
		{
			xtables_error(PARAMETER_PROBLEM,
						"Arg length error with --urls, must be %d", MASSURL_INDEX_WWORDS << 3);
		}

		memset(info->urlIndexBits, 0, MASSURL_INDEX_WWORDS * sizeof(unsigned int));
		for (index = 0, pcIndex = optarg; *pcIndex != '\0'; pcIndex++, index++)
		{
			ch = *pcIndex;
			if (ch >= '0' && ch <= '9')
			{
				v = ch - '0'; 
			}
			else if (ch >= 'A' && ch <= 'F')
			{
				v = ch - 'A' + 10; 
			}
			else if (ch >= 'a' && ch <= 'f')
			{
				v = ch - 'a' + 10; 
			}
			else
			{
				xtables_error(PARAMETER_PROBLEM,
							"Arg format error with --urls, must with only 0-9A-Fa-f");
			}
		
			info->urlIndexBits[index >> 3] += (v << ((index & 7) << 2));
		}
		*flags |= URLS;
	}
	else
	{
		return 0;
	}
	
	return 1;
}

static void check(unsigned int flags)
{
	if (!(flags & TYPE))
		xtables_error(PARAMETER_PROBLEM, "Must specify `--type'");
	if (!(flags & URLS))
		xtables_error(PARAMETER_PROBLEM, "Must specify `--urls'");
}

static void print(const void *ip, const struct xt_entry_match *match,
                       int numeric)
{
	printf("MASSURL ");
}

static void save(const void *ip, const struct xt_entry_match *match)
{
	struct xt_massurl_info *info = (void *)match->data;
	int index;

	printf("--type ");
	if (info->type == MASSURL_TYPE_DNS)
	{
		printf("dns ");
	}
	else if (info->type == MASSURL_TYPE_URL)
	{
		printf("url ");
	}
	else
	{
		printf("http ");
	}
	
	printf("--url ");
	
	for (index = 0; index < MASSURL_INDEX_WWORDS; index++)
	{
		printf("%08x", info->urlIndexBits[index]);
	}
	printf(" ");
}

#if 1
static struct xtables_match massurl_match = {
	.name          = "massurl",
	.family        = NFPROTO_UNSPEC,
	.version       = XTABLES_VERSION,
	.size          = XT_ALIGN(sizeof(struct xt_massurl_info)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_massurl_info)),
	.help          = help,
	.init          = init,
	.parse         = parse,
	.final_check   = check,
	.print         = print,
	.save          = save,
	.extra_opts    = opts,
};
#else
static struct xtables_match massurl_match = {
	.name          = "massurl",
	.family        = AF_INET,
	.version       = IPTABLES_VERSION,
	.size          = XT_ALIGN(sizeof(struct xt_massurl_info)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_massurl_info)),
	.help          = help,
	.init          = init,
	.parse         = parse,
	.final_check   = check,
	.print         = print,
	.save          = save,
	.extra_opts    = opts,
};

static struct xtables_match massurl_match6 = {
	.name          = "massurl",
	.family        = AF_INET6,
	.version       = IPTABLES_VERSION,
	.size          = XT_ALIGN(sizeof(struct xt_massurl_info)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_massurl_info)),
	.help          = help,
	.init          = init,
	.parse         = parse,
	.final_check   = check,
	.print         = print,
	.save          = save,
	.extra_opts    = opts,
};
#endif

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/
void _init(void)
{
	xtables_register_match(&massurl_match);
#if 0	
	xtables_register_match(&massurl_match6);
#endif
}

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

