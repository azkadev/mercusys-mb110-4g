/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_dpi_engine.c
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     11Oct13
 *
 *\warning
 *
 *\history \arg 0.0.1, 11Oct13, Yan Wei, Create the file.
 */
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/types.h>
#include <linux/pcre/pcre.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "appidentify.h"
#include "appidentify_id.h"
#include "appidentify_dpi_engine.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define OVECCOUNT       30    /* should be a multiple of 3 */
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
void*   appidentify_pcre_parse(const char* pattern);
int     appidentify_pcre_inspect(struct sk_buff *skb, struct nf_conntrack_app *ct,
                                 void *cookie, unsigned char *data, unsigned int datalen);
/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
APPIDNTFY_DPI_ENGINE  appidentify_dpi_pcre = {
    .parse = appidentify_pcre_parse,
    .inspect = appidentify_pcre_inspect
};
/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
/*!
 *\fn       Ip_bool appidentify_pcre_parse(char* pattern, void** re)
 *\brief ±àÒëPCRE¸ñÊ½×Ö·û´®
 *\details
 *
 *\param[in]
 *\param[out]
 *
 *\return
 *\retval
 *
 *\note
 */
void*
appidentify_pcre_parse(const char* pattern)
{
    char    *error;
    int     erroffset;
    pcre    *re;

    APPIDNTF_PCRE_DEBUG("%s\r\n", pattern);
    re = pcre_compile(
        pattern,              /* the pattern */
        PCRE_DOTALL,          /* default options */
        (const char **)&error,               /* for error message */
        &erroffset,           /* for error offset */
        NULL);                /* use default character tables */

    /* Compilation failed: print the error message and exit */

    if (re == NULL)
    {
        APPIDNTF_ERROR("PCRE compilation failed at offset %d: %s\n", erroffset, error);
    }
    {
        #if 0
        APPIDNTF_PCRE_DEBUG("first_byte %x req_byte %x size %d  %s\r\n", re->first_byte, re->req_byte, re->size,
            re->options & PCRE_ANCHORED?"anchored":"noAnchored");
        #endif
    }
    return re;
}


int
appidentify_pcre_inspect(struct sk_buff *skb, struct nf_conntrack_app *ct, void *cookie, unsigned char *data, unsigned int datalen)
{
    int                     rc;
    int ovector[OVECCOUNT];
    APPIDNTFY_KER_RULE      *rule;
    struct iphdr            *iph;
    unsigned short          fragoff;
    unsigned char           protocol;
    struct nf_conn          *pCt;
    enum ip_conntrack_info  ctinfo;
    int                     ipHeaderLen;
    unsigned int            packetDirection;

    APPIDNTF_DEBUG("in\r\n");
    if (NULL == skb || NULL == ct)
    {
        return APP_ID_UNKNOWN;
    }

    pCt = nf_ct_get(skb, &ctinfo);
    packetDirection = CTINFO2DIR(ctinfo);
    iph = ip_hdr(skb);
    fragoff = ntohs(iph->frag_off) & 0x1fff;
    ipHeaderLen = iph->ihl * 4;
    protocol = iph->protocol;

    rule = (APPIDNTFY_KER_RULE*)cookie;

    rc = pcre_exec(
        rule->feature[packetDirection].pDpiCode,       /* the compiled pattern */
        NULL,                 /* no extra data - we didn't study the pattern */
        (PCRE_SPTR)data,      /* the subject string */
        datalen,       /* the length of the subject */
        0,                    /* start at offset 0 in the subject */
        0,                    /* default options */
        ovector,              /* output vector for substring information */
        OVECCOUNT);           /* number of elements in the output vector */


    if (rc < 0)
    {
        return APP_ID_UNKNOWN;
    }

    return rule->appId;
}

/*!
 *\fn   void appidentify_pcre_free(void *pDpiCode) brief    Free the pcre data.
 */
void appidentify_pcre_free(void *pDpiCode)
{
    if(NULL != pDpiCode)
        kfree(pDpiCode);
}
/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
