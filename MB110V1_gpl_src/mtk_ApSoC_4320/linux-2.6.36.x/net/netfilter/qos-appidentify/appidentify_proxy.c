/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_proxy.c
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
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/ip.h>

#include "appidentify.h"
#include "appidentify_match_rules.h"
#include "appidentify_proxy.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/
void socks5_udp_adjust_payload(struct sk_buff *skb, unsigned char *data);
int appidentify_proxy_http_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                              unsigned char *data, unsigned int datalen);
int appidentify_proxy_http_connect_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                              unsigned char *data, unsigned int datalen);
int
appidentify_proxy_socks4_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                            unsigned char *data, unsigned int datalen);
int
appidentify_proxy_socks5_tcp_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                                unsigned char *data, unsigned int datalen);
int
appidentify_proxy_socks5_udp_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                                unsigned char *data, unsigned int datalen);
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
unsigned short appidntf_proxy_debug = TRUE;
/* ....:...........¼..APPIDNTFY_PROXY_INDEX............ by jyc, 10Nov10 */
APPIDNTFY_PROXY   appidentify_proxy_detector[] =
{
    {
        "http",
        {
            APP_ID_PROXY,
            IPPROTO_TCP,
            {0},
            APPIDNTFY_MASK_DST_PORT,
            1,
            {{{0},NULL}},
            {
                { TRUE, 3, 8, 0, 0, &appidentify_proxy_http_search, NULL},
                { FALSE, 0, 0, 0, 0, NULL, NULL }
            },
        },
        0,
        NULL
    },
    {
        "httpconnect",
        {
            APP_ID_PROXY,
            IPPROTO_TCP,
            {0},
            APPIDNTFY_MASK_DST_PORT,
            1,
            {{{0},NULL}},
            {
                { TRUE, 3, 8, 0, 0, &appidentify_proxy_http_connect_search, NULL},
                { FALSE, 0, 0, 0, 0, NULL, NULL }
            },
        },
        4,
        NULL
    },
    {
        "socks4",
        {
            APP_ID_PROXY,
            IPPROTO_TCP,
            {0},
            APPIDNTFY_MASK_DST_PORT,
            1,
            {{{0},NULL}},
            {
                { TRUE, 3, 8, 0, 0, &appidentify_proxy_socks4_search, NULL },
                { FALSE, 0, 0, 0, 0, NULL, NULL }
            },
        },
        4,
        NULL
    },

    {
        "socks5_tcp",
        {
            APP_ID_PROXY,
            IPPROTO_TCP,
            {0},
            APPIDNTFY_MASK_DST_PORT,
            1,
            {{{0},NULL}},
            {
                {TRUE, 3, 8, 0, 0, &appidentify_proxy_socks5_tcp_search, NULL },
                {FALSE, 0, 0, 0, 0, NULL, NULL }
            },

        },
        4,
        NULL
    },

    {
        "socks5_udp",
        {
            APP_ID_PROXY,
            IPPROTO_UDP,
            {0},
            APPIDNTFY_MASK_DST_PORT,
            1,
            {{{0},NULL}},
            {
                {TRUE, -1, 5, 0, 0, &appidentify_proxy_socks5_udp_search, NULL},
                {FALSE, 0, 0, 0, 0, NULL, NULL }
            },
        },
        0,
        &socks5_udp_adjust_payload
    },
    {
        "",
        {
            APP_ID_PROXY,
            IPPROTO_IP,
            {0},
            APPIDNTFY_MASK_NONE,
            0,
            {{{0},NULL}},
            {
                {FALSE, 0, 0, 0, 0, NULL, NULL },
                {FALSE, 0, 0, 0, 0, NULL, NULL }
            },
        },
        0,
        NULL
    }
};

/*! ................  */
#define APPIDNTFY_CONST_PROXY_RULE_COUNT (sizeof(appidentify_proxy_detector)/sizeof(APPIDNTFY_PROXY)-1)    /* .................. */
const int   APPIDNTFY_PROXY_RULE_COUNT =   APPIDNTFY_CONST_PROXY_RULE_COUNT;
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
unsigned char*
tools_string_strnstr(unsigned char *string, unsigned int strLen, unsigned char *subStr, unsigned int subStrLen)
{
    unsigned char *pCmpStrHead = NULL;
    int     i, j;

    if ((NULL == string) || (NULL == subStr))
    {
        APPIDNTF_ERROR("Some parameter is null.\r\n");
        return NULL;
    }

    if (strLen < subStrLen)
    {
        APPIDNTF_ERROR("strLen is smaller than subStrLen.\r\n");
        return NULL;
    }

    for (i=0; i<(strLen-subStrLen+1); i++)
    {
        pCmpStrHead = string + i;

        for (j=0; j<subStrLen; j++)
        {
            if (*(subStr+j) != *(pCmpStrHead+j))
            {
                break;
            }
            else if (j == (subStrLen-1))
            {
                return pCmpStrHead;
            }
        }
    }

    return NULL;
}

void
appidentify_proxy_tuple(APPIDNTFY_PKT_TUPLE *pktTuple, APPIDNTFY_PKT_TUPLE tuple)
{
    pktTuple->dstip = tuple.dstip;
    pktTuple->dstport = tuple.dstport;
}

int
appidentify_proxy_socks5_tcp_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                                unsigned char *data, unsigned int datalen)
{
    unsigned char       *pPayload = data;
    APPIDNTFY_PKT_TUPLE   *pktTuple = (APPIDNTFY_PKT_TUPLE *)tuple;
    unsigned int        index;
    APPIDNTFY_APP_INDEX   ret = APP_ID_UNKNOWN;
    struct nf_conntrack_app_ext *appidntify_ext;

    if (NULL == skb || NULL == appinfo || NULL == tuple)
    {
        goto socks5_tcp_ret;
    }

    pktTuple->srcip = 0;
    pktTuple->srcport = 0;
    pktTuple->dstip = 0;
    pktTuple->dstport = 0;

    appidntify_ext = (struct nf_conntrack_app_ext *)appinfo->appidntfy_ext;
    if (NULL == appidntify_ext)
    {
        APPIDNTF_ERROR("appidntify_ext null.\r\n");
        return APPIDNTFY_DI_UNKNOWN;
    }

    if (datalen < 8)
    {
        APPIDNTF_DEBUG("too small\r\n");
        ret = APP_ID_UNKNOWN;
        goto socks5_tcp_ret;
    }

    if (pPayload[0] != 0x05         /*! VER */
        || (pPayload[1] != 0x01      /*! CD:CONNECT*/
        && pPayload[1] != 0x02      /*! CD:BIND */
        && pPayload[1] != 0x03)      /*! CD:UDP */
        || pPayload[2] != 0x00)     /*! rsvd */
    {
        APPIDNTF_DEBUG("Incorrect char\r\n");
        ret = APP_ID_UNKNOWN;
        goto socks5_tcp_ret;
    }

    switch(pPayload[3])
    {
    case 1:     /*! ipv4 */
        if (datalen != 10)
        {
            APPIDNTF_DEBUG("Incorrect length %d against 10\r\n",
                datalen);
            ret = APP_ID_UNKNOWN;
        }
        else
        {
            memcpy(&pktTuple->dstip, &pPayload[4], 4);
            memcpy(&pktTuple->dstport, &pPayload[8], 2);
            ret = APP_ID_PROXY;
        }
        break;

    case 3:     /*! domain, ........................ */
        if (pPayload[4] == 0
            || (5 + pPayload[4] + 2) != datalen)
        {
            APPIDNTF_DEBUG("Incorrect length %d against %d by calc\r\n",
                datalen, (5 + pPayload[4] + 2));
            ret = APP_ID_UNKNOWN;
            break;
        }

        for (index = 1; index <= pPayload[4]; index ++)
        {
            if (pPayload[index + 4] != '.'
                && pPayload[index + 4] != ':'
                && pPayload[index + 4] != '/'
                && pPayload[index + 4] != '-'
                && !(pPayload[index + 4] >= '0' && pPayload[index + 4] <= '9')
                && !(pPayload[index + 4] >= 'a' && pPayload[index + 4] <= 'z'))
            {
                APPIDNTF_DEBUG("Incorrect char %c(%d) in domain\r\n", pPayload[index + 4], index);
                ret = APP_ID_UNKNOWN;
                goto socks5_tcp_ret;
            }
        }
        memcpy(&pktTuple->dstport, &pPayload[pPayload[4] + 5], 2);
        ret = APP_ID_PROXY;
        break;

    case 4:     /*! ipv6, no break */
    default:
        ret = APP_ID_UNKNOWN;
        break;
    }

socks5_tcp_ret:
    if (APP_ID_PROXY == ret)
    {
        APPIDNTF_PROXY_DEBUG("HIT TCP SOCKS5!!pktTuple->dstport = %d\n\r",pktTuple->dstport);
        appidntify_ext->proxy_id_index = PROXY_ID_SOCK5_TCP;
    }
    return ret;
}


#define SOCKS5_UDP_DEBUG APPIDNTF_DEBUG/* printf */


int
appidentify_proxy_socks5_udp_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                                unsigned char *data, unsigned int datalen)
{
    unsigned char   *pPayload = data;
    APPIDNTFY_PKT_TUPLE *pktTuple = (APPIDNTFY_PKT_TUPLE *)tuple;
    unsigned short  *reserved = NULL;
    unsigned short index = 0;
    APPIDNTFY_APP_INDEX ret = APP_ID_UNKNOWN;
    struct nf_conntrack_app_ext *appidntify_ext;


    pktTuple->srcip = 0;
    pktTuple->srcport = 0;
    pktTuple->dstip = 0;
    pktTuple->dstport = 0;

    if (NULL == skb || NULL == appinfo || NULL == tuple)
    {
        goto socks5_udp_ret;
    }

    appidntify_ext = (struct nf_conntrack_app_ext *)appinfo->appidntfy_ext;
    if (NULL == appidntify_ext)
    {
        APPIDNTF_ERROR("appidntify_ext null.\r\n");
        return APPIDNTFY_DI_UNKNOWN;
    }

    if (datalen < 8)
    {
        APPIDNTF_DEBUG("too small\r\n");
        ret = APP_ID_UNKNOWN;
        goto socks5_udp_ret;
    }

    reserved = (unsigned short *)pPayload;
    if (0 != *reserved      /*! Reversed, 2bytes */
        || pPayload[2] > 0x80   /* Fragment is between 1~127,the highest bit means the end */
        )
    {
        SOCKS5_UDP_DEBUG("socks5_udp:Incorrect char,*reserved = %d,pPayload[2]= 0x%x\r\n", *reserved, pPayload[2]);
        ret = APP_ID_UNKNOWN;
        goto socks5_udp_ret;
    }

    switch(pPayload[3])
    {
    case 1:     /*! ipv4 */
            memcpy(&pktTuple->dstip, &pPayload[4], 4);
            memcpy(&pktTuple->dstport, &pPayload[8], 2);
            ret = APP_ID_PROXY;
        break;

    case 3:     /*! domain, ........................ */
        if (pPayload[4] == 0)
        {
            SOCKS5_UDP_DEBUG("Incorrect length %d\r\n", pPayload[4]);
            ret = APP_ID_UNKNOWN;
            break;
        }

        for (index = 1; index <= pPayload[4]; index ++)
        {
            if (pPayload[index + 4] != '.'
                && pPayload[index + 4] != ':'
                && pPayload[index + 4] != '/'
                && pPayload[index + 4] != '-'
                && !(pPayload[index + 4] >= '0' && pPayload[index + 4] <= '9')
                && !(pPayload[index + 4] >= 'a' && pPayload[index + 4] <= 'z'))
            {
                SOCKS5_UDP_DEBUG("Incorrect char %c(%d) in domain\r\n", pPayload[index + 4], index);
                ret = APP_ID_UNKNOWN;
                goto socks5_udp_ret;
            }
        }

        memcpy(&pktTuple->dstport, &pPayload[SOCKS5_UDP_PORT_OFFSET(pPayload[4])], 2);
        ret = APP_ID_PROXY;
        break;

    case 4:     /*! ipv6, no break */
    default:
        ret = APP_ID_UNKNOWN;
        break;
    }

socks5_udp_ret:
    if (APP_ID_PROXY == ret)
    {
        APPIDNTF_PROXY_DEBUG("HIT UDP SOCKS5!!pktTuple->dstport = %d\n\r",pktTuple->dstport);
        appidntify_ext->proxy_id_index = PROXY_ID_SOCK5_UDP;
    }
    return ret;
}


#define SOCKS4_DEBUG APPIDNTF_DEBUG/* printf */
int
appidentify_proxy_socks4_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                            unsigned char *data, unsigned int datalen)
{
    unsigned char       *pPayload = data;
    APPIDNTFY_PKT_TUPLE   *pktTuple = (APPIDNTFY_PKT_TUPLE *)tuple;
    unsigned int        pktLen;
    APPIDNTFY_APP_INDEX ret = APP_ID_UNKNOWN;
    struct nf_conntrack_app_ext *appidntify_ext;

    pktTuple->srcip = 0;
    pktTuple->srcport = 0;
    pktTuple->dstip = 0;
    pktTuple->dstport = 0;

    SOCKS4_DEBUG("socks4:searching\r\n");

    if (NULL == skb || NULL == appinfo || NULL == tuple)
    {
        goto socks4_ret;
    }

    appidntify_ext = (struct nf_conntrack_app_ext *)appinfo->appidntfy_ext;
    if (NULL == appidntify_ext)
    {
        APPIDNTF_ERROR("appidntify_ext null.\r\n");
        return APPIDNTFY_DI_UNKNOWN;
    }

    pktLen = datalen;
    if (pktLen < 9)
    {
        SOCKS4_DEBUG("socks4:too small\r\n");
        ret = APP_ID_UNKNOWN;
        goto socks4_ret;
    }

    if (pPayload[0] != 0x04         /*! VER */
        || pPayload[1] != 0x01      /*! CD:CONNECT*/
        || pPayload[pktLen - 1] != 0x00)     /*! usrID */
    {
        SOCKS4_DEBUG("socks4:Incorrect char,pPayload[0] = %d,pPayload[%d] = 0x%x\r\n",
        pPayload[0], pktLen - 1,pPayload[pktLen - 1]);
        ret = APP_ID_UNKNOWN;
        goto socks4_ret;
    }

    memcpy(&pktTuple->dstip, &pPayload[4], 4);
    memcpy(&pktTuple->dstport, &pPayload[2], 2);
    ret = APP_ID_PROXY;

socks4_ret:
    if (APP_ID_PROXY == ret)
    {
        APPIDNTF_PROXY_DEBUG("HIT TCP SOCKS4!!pktTuple->dstport = %d\n\r", pktTuple->dstport);
        appidntify_ext->proxy_id_index = PROXY_ID_SOCK4;
    }

    return ret;
}

#define HTTP_PROXY_DEBUG  APPIDNTF_DEBUG/* printf */
int appidentify_proxy_http_connect_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                              unsigned char *data, unsigned int datalen)
{
    unsigned char  *pPayLoad = data;
    unsigned char  *portBegin = NULL;
    unsigned int    dstPort;

    unsigned int    pktLen;
    APPIDNTFY_APP_INDEX ret = APP_ID_UNKNOWN;
    struct nf_conntrack_app_ext *appidntify_ext;

    APPIDNTFY_PKT_TUPLE *pktTuple = (APPIDNTFY_PKT_TUPLE *)tuple;

    HTTP_PROXY_DEBUG("appidentify_proxy_http_connect\r\n");

    if (NULL == skb || NULL == appinfo || NULL == tuple)
    {
        goto http_connect_ret;
    }

    appidntify_ext = (struct nf_conntrack_app_ext *)appinfo->appidntfy_ext;
    if (NULL == appidntify_ext)
    {
        APPIDNTF_ERROR("appidntify_ext null.\r\n");
        return APPIDNTFY_DI_UNKNOWN;
    }

    memset(pktTuple, sizeof(APPIDNTFY_PKT_TUPLE), 0);

    pktLen = datalen;


    /* IP+.........½.±......"1.1.1.1:1".."255.255.255.255:65535·...........·..±.............¼ by jyc, 24Dec10 */
    if (pktLen < 20)
    {
        HTTP_PROXY_DEBUG("httpConnect:too small.pktLen = %d\n\r", pktLen);
        goto http_connect_ret;
    }

    /* Check whether is "CONNECT" string by jyc, 24Dec10 */
    if (('C' != pPayLoad[0] ) || ('N' != pPayLoad[3])
         || ( 'C' != pPayLoad[5]) || (' ' != pPayLoad[7]))
    {
        HTTP_PROXY_DEBUG("httpConnect: invalid char. NOT CONNECT\n\r");
        goto http_connect_ret;
    }

    /* Confirm "CONNECT" by jyc, 07Nov10 */
    if (memcmp(pPayLoad, "CONNECT", 7) == 0)
    {
        /* if port is unknown, set default value 80 ,by jyc, 07Nov10 */
        pktTuple->dstport = HTTP_DEFAULT_PORT;

        portBegin = appidentify_http_locate_port(&pPayLoad[8]);

        if (NULL != portBegin)
        {
            dstPort = simple_strtol((const char *)(portBegin), NULL, 10);
            if((dstPort > 0) && (dstPort < 65536))
            {
                /* Update dstport in tuple by jyc, 07Nov10 */
                pktTuple->dstport = dstPort;
            }
        }

        ret = APP_ID_PROXY;
    }

http_connect_ret:
    if (APP_ID_PROXY == ret)
    {
        HTTP_PROXY_DEBUG("HIT httpConnect:pktTuple->dstport = %d\n\r", pktTuple->dstport);
        appidntify_ext->proxy_id_index = PROXY_ID_HTTP_CONNECT;
    }

    return ret;
}

int appidentify_proxy_http_search(struct sk_buff *skb, struct nf_conntrack_app *appinfo, void *tuple,
                              unsigned char *data, unsigned int datalen)
{
    unsigned char *pPayLoad = data;
    unsigned char *pPayLoadHttp = NULL;
    unsigned char *portBegin = NULL;
    unsigned int dstPort;

    unsigned int pktLen;
    APPIDNTFY_APP_INDEX ret = APP_ID_UNKNOWN;
    struct nf_conntrack_app_ext *appidntify_ext;

    APPIDNTFY_PKT_TUPLE *pktTuple = (APPIDNTFY_PKT_TUPLE *)tuple;

    HTTP_PROXY_DEBUG("appidentify_proxy_http\r\n");

    if (NULL == skb || NULL == appinfo || NULL == tuple)
    {
        goto http_ret;
    }

    appidntify_ext = (struct nf_conntrack_app_ext *)appinfo->appidntfy_ext;
    if (NULL == appidntify_ext)
    {
        APPIDNTF_ERROR("appidntify_ext null.\r\n");
        return APPIDNTFY_DI_UNKNOWN;
    }

    memset(pktTuple, sizeof(APPIDNTFY_PKT_TUPLE), 0);

    pktLen = datalen;

    if (pktLen < 20)
    {
        HTTP_PROXY_DEBUG("http:too small.\n\r");
        goto http_ret;
    }

    /* ...."POST..GET Http://" by jyc, 07Nov10 */
    if ((('P' == pPayLoad[0])&& ('T' == pPayLoad[3]) && (' ' == pPayLoad[4]))
       )
    {
        pPayLoadHttp = &pPayLoad[5];
    }
    else if (('G' == pPayLoad[0])&& ('T' == pPayLoad[2]) && (' ' == pPayLoad[3]))
    {
        pPayLoadHttp = &pPayLoad[4];
    }
    else
    {
        pPayLoadHttp = NULL;
    }

    if (NULL != pPayLoadHttp)
    {
        if (('h' == pPayLoadHttp[0] || 'H' == pPayLoadHttp[0])
         && (':' == pPayLoadHttp[4]) && ('/' == pPayLoadHttp[5]))
        {
            /* if port is unknown, set default value 80 ,by jyc, 07Nov10 */
            pktTuple->dstport = HTTP_DEFAULT_PORT;

            portBegin = appidentify_http_locate_port(&pPayLoad[11]);

            if (NULL != portBegin)
            {
                dstPort = simple_strtol((const char *)(portBegin), NULL, 10);
                if((dstPort > 0) && (dstPort < 65536))
                {
                    /* Update dstport in tuple by jyc, 07Nov10 */
                    pktTuple->dstport = dstPort;
                }
            }

            ret = APP_ID_PROXY;
        }

    }
    else
    {
        HTTP_PROXY_DEBUG("http:Invalid char.\n\r");
    }

http_ret:
    if (APP_ID_PROXY == ret)
    {
        HTTP_PROXY_DEBUG("HIT HTTP proxy!pktTuple->dstport = %d\n\r", pktTuple->dstport);
        appidntify_ext->proxy_id_index = PROXY_ID_HTTP;
    }

    return ret;

}

void socks5_udp_adjust_payload(struct sk_buff *skb, unsigned char *data)
{
    unsigned char   *pPayload = data;
    unsigned short  adjustLen = 0;

    if (NULL == skb || NULL == data)
    {
        goto adjust_payload_ret;
    }

    /* ............................·.............payload by jyc, 09Nov10 */
    switch(pPayload[3])
    {
    case 1:     /*! ipv4 */
            adjustLen = SOCKS5_UDP_IP_HEADER_LEN;
            SOCKS5_UDP_DEBUG("socks5_udp:ip:adjustLen = %d\r\n", adjustLen);
        break;

    case 3:     /*! domain */
        /*
        The first octet of the address field contains the number of octets of name that
           follow, there is no terminating NUL octet.
        */
        adjustLen = SOCKS5_UDP_DOMAIN_HEADER_LEN(pPayload[4]);
        SOCKS5_UDP_DEBUG("socks5_udp:domain:adjustLen = %d\r\n", adjustLen);

        break;

    case 4:     /*! ipv6, no break */
    default:
        SOCKS5_UDP_DEBUG("socks5_udp:no adjust\r\n");
        break;
    }
adjust_payload_ret:

    /* adjust the udppayload pointer */
    data += adjustLen;

    SOCKS5_UDP_DEBUG("ADJUST UDP SOCKS5!!\r\n");

    return;
}

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
int appidentify_locate_payload(struct sk_buff *skb, unsigned char **pos, unsigned int *len)
{
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    unsigned char *data_ptr;
    int    datalen;

    APPIDNTF_DEBUG("in\r\n");
    iph = ip_hdr(skb);
    if (NULL == iph)
    {
        return -1;
    }

    /* tcp && udp hdr has not init, should point to the iph. skb->len equal
       ip_pkt_len */
    data_ptr = (unsigned char *)iph;
    if (iph->protocol == IPPROTO_TCP)
    {
        tcph = (struct tcphdr *)(data_ptr + ip_hdrlen(skb));
        data_ptr = (unsigned char *)tcph;
        data_ptr += tcph->doff * 4;
        datalen = skb->len - ip_hdrlen(skb) - tcph->doff * 4;
        APPIDNTF_DEBUG("tcp hdr srcport %d, dstport %d, skb->len %d, iphdrlen %d, pro_hdrlen %d.\r\n",
                       tcph->source, tcph->dest,
                       skb->len, ip_hdrlen(skb), tcph->doff * 4);
        if (datalen < 0)
        {
            APPIDNTF_ERROR("datalen %d.\n", datalen);
            return -1;
        }
    }
    else if (iph->protocol == IPPROTO_UDP)
    {
        udph = (struct udphdr *)(data_ptr + ip_hdrlen(skb));
        data_ptr = (unsigned char *)udph;
        data_ptr += sizeof(struct udphdr);
        datalen = skb->len - ip_hdrlen(skb) - sizeof(struct udphdr);
        APPIDNTF_DEBUG("skb->len %d, iphdrlen %d, pro_hdrlen %d.\r\n",
                       skb->len, ip_hdrlen(skb), sizeof(struct udphdr));
        if (datalen < 0)
        {
            APPIDNTF_ERROR("datalen %d.\n", datalen);
            return -1;
        }
    }
    else if (iph->protocol == IPPROTO_ICMP)
    {
        return -1;
    }
    else
    {
        APPIDNTF_ERROR("other protocol return.\r\n");
        return -1;
    }

    *pos = data_ptr;
    *len = datalen;
    return 0;
}


/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
