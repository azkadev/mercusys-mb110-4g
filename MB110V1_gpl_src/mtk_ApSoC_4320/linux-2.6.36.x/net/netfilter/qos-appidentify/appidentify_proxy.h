/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_proxy.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     09Oct13
 *
 *\warning
 *
 *\history \arg 0.0.1, 09Oct13, Yan Wei, Create the file.
 */
#ifndef __APPIDENTIFY_PROXY_H__
#define __APPIDENTIFY_PROXY_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include "appidentify.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
extern unsigned short appidntf_proxy_debug;
#define     APPIDNTF_PROXY_ERROR(fmt, args...)  printk("[PROXY:%s:%d] "fmt, __FUNCTION__, __LINE__, ##args)

#if 0
#define     APPIDNTF_PROXY_DEBUG(fmt, args...)                                    \
            do                                                              \
            {                                                               \
                if (1 == appidntf_proxy_debug)                                    \
                {                                                           \
                    printk("[PROXY:%s:%d] "fmt, __FUNCTION__, __LINE__, ##args);  \
                }                                                           \
            }while(0)
#else
#define APPIDNTF_PROXY_DEBUG APPIDNTF_DEBUG
#endif

#define APPIDNTFY_SUPPORT_PROXY   (0)

#define APPIDNTFY_CT_MARK_PROXY   (1 << 14)
#define APPIDNTFY_CT_MARK_STOP_PROXY  (1 << 13)
/* #define APP_ID_PROXY (1) */

#define HTTP_DEFAULT_PORT 80
#define SOCKS5_UDP_IP_HEADER_LEN 10
#define SOCKS5_UDP_PORT_OFFSET(domainLen) (5 + (domainLen)) /* 5.¬socks............ */
#define SOCKS5_UDP_DOMAIN_HEADER_LEN(domainLen) (SOCKS5_UDP_PORT_OFFSET(domainLen) + 2); /* 2........ */
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct appidentify_proxy_s
{
    char                name[16];
    APPIDNTFY_KER_RULE    rule;
    int                 offset;
    void                (*adjust_payload)(struct sk_buff *, unsigned char *);
}APPIDNTFY_PROXY;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
int appidentify_locate_payload(struct sk_buff *skb, unsigned char **pos, unsigned int *len);

void
appidentify_proxy_tuple(APPIDNTFY_PKT_TUPLE *pktTuple, APPIDNTFY_PKT_TUPLE tuple);

unsigned char*
tools_string_strnstr(unsigned char *string, unsigned int strLen, unsigned char *subStr, unsigned int subStrLen);

static inline unsigned char*
appidentify_http_locate_port(unsigned char *httpStr)
{
    unsigned char *portBegin = NULL;
    unsigned char *portEnd;

    /* ............ú¼............ by jyc, 07Nov10 */
    portEnd = (unsigned char *) strstr((const char*)(httpStr + 1), " ");
    if (NULL == portEnd)
    {
        return NULL;
    }

    portBegin = (unsigned char *)tools_string_strnstr((unsigned char *)httpStr, portEnd - httpStr, ":", 1);
    if (NULL != portBegin)
    {
        /* ....URL.¬http://..ú¼........ú¼......http://..............ú¼....":"...... by jyc, 06Nov10 */
        if (*(portBegin+1) < '0' || *(portBegin + 1) > '9')
        {
            portBegin = (unsigned char *) tools_string_strnstr((unsigned char *)portBegin, portEnd - httpStr, ":", 1);
        }
    }

    if (NULL != portBegin)
    {
        /* ........ */
        portBegin++;
    }

    return portBegin;
}




#endif  /* __APPIDENTIFY_PROXY_H__ */
