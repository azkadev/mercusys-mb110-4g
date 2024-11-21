/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_dns.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     11Nov13
 *
 *\warning
 *
 *\history \arg 0.0.1, 12Nov13, Yan Wei, Create the file.
 */
#ifndef __APPIDENTIFY_DNS_H__
#define __APPIDENTIFY_DNS_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
extern int appidntf_dns_debug;

#define     APPIDNTFY_DNS_ERROR(fmt, args...)  printk("DNS_ERROR[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args)

#define     APPIDNTFY_DNS_DEBUG(fmt, args...)                                   \
            do                                                                  \
            {                                                                   \
                if (1 == appidntf_dns_debug)                                    \
                {                                                               \
                    printk("DNS_DEBUG[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args);  \
                }                                                               \
            }while(0)

#define         DNS_SVR_PORT                        (53)
#define         APPIDNTFY_DNS_IP_NUM                (16)
#define         APPIDNTFY_DNS_DOMAIN_MAX_LEN        (128)

#define         APPIDNTFY_DNS_RESERVED_NUM          (16)
#define         APPIDNTFY_DNS_MAX_NUM               (1024)
#define         APPIDNTFY_IP_RESERVED_NUM           (16)
#define         APPIDNTFY_IP_MAX_NUM                (8192)

#define         APPIDNTFY_DNS_HASH_SCALE            (4096)
#define         APPIDNTFY_DNS_IP_HASH_SCALE         (16384)

#define         APPMNGR_DNS_KEYWORD_LEN             (16)
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct _APPIDNTFY_DNS_IP
{
    struct hlist_node           hnode;      /* ip addr hash */
    struct list_head            lnode;      /* ip lru */
    unsigned int                ip4addr;
    unsigned short              appid;
}APPIDNTFY_DNS_IP;


typedef struct _APPIDNTFY_DNS
{
    struct hlist_node           hnode;      /* domain hash */
    struct list_head            lnode;      /* domain lru */
    char                        domain[APPIDNTFY_DNS_DOMAIN_MAX_LEN];
    struct list_head            iplist;
    unsigned short              appid;
    unsigned char               ipnum;
}APPIDNTFY_DNS;


typedef struct _APPMNGR_DNS_KEYWORD
{
    char        keyword[APPMNGR_DNS_KEYWORD_LEN];
}APPMNGR_DNS_KEYWORD;

typedef struct _APPMNGR_DNS_RULE
{
    int                     appId;
    APPMNGR_DNS_KEYWORD     dnsKw;
}APPMNGR_DNS_RULE;

typedef struct _APPID_DNS_RULE
{
    int                     appId;
    APPMNGR_DNS_KEYWORD     dnsKw;
    struct list_head        list;
}APPID_DNS_RULE;

typedef struct _APPMNGR_DNS
{
    unsigned int        appDnsCnt;
    APPMNGR_DNS_RULE    *rule;
}APPMNGR_DNS;

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
extern int g_enableDns;
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
unsigned int appidentify_dns_hook(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *));

int appidentify_dns_init(void);
int appidentify_dns_exit(void);
void appidentify_dns_printall(void);
void appidentify_dns_testentry_add(char *domain, unsigned int ipaddr, short appid);
void appidentify_dnskw_printall(void);
int appidentify_add_dnskw(char *domain, unsigned int appId);



#endif  /* __APPIDENTIFY_DNS_H__ */
