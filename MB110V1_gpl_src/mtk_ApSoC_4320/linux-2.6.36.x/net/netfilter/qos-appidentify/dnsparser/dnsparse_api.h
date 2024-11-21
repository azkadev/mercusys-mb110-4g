/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_api.h
 *\brief       include this header file, then all api and data structure provided by
 *             libdnsparse.a will be imported in.
 *
 *\author      wangquzhi
 *\version     0.0.1
 *\date        2013/02/06
 *\details
 *             ## Must be invoked before any operations made
 *             IP_PUBLIC dnsparse_result_t dnsparse_mempool_init();
 *
 *             ## DNSPARSE_DNS_MESSAGE is what you get after the dns packet parsing is finished.
 *             ##+You should prepare a empty DNSPARSE_DNS_MESSAGE to store the parsing result. And
 *             ##+use the following function to make a DNSPARSE_DNS_MESSAGE empty.
 *             IP_PUBLIC dnsparse_result_t dnsparse_message_init(DNSPARSE_DNS_MESSAGE *pMsg);
 *
 *             ## Parsing info from a dns packet, which is the most important API.
 *             IP_PUBLIC dnsparse_result_t dnsparse_message_parse(DNSPARSE_DNS_MESSAGE *pMsg,
 *                                                                const char *pDnsPkt, int dnsPktLen);
 *
 *             ## libdnsparse alloc memories automatically while parsing dns packet, and store parsing
 *             ##+result to the new alloced memories. All memories alloced by libdnsparse is made to
 *             ##+to lists linked to DNSPARSE_DNS_MESSAGE. The following function free these memories.
 *             ## Note: 1. DNSPARSE_DNS_MESSAGE itself is not freed, because libdnsparse doesn't know
 *             ##          how it is alloced.
 *             ##       2. You must free a DNSPARSE_DNS_MESSAGE A.S.A.P. after you got the parsing
 *             ##          result. Because all these memories are alloced from mempools maintained by
 *             ##          libdnsparse, whose size is very limited.
 *             IP_PUBLIC dnsparse_result_t dnsparse_message_destroy(DNSPARSE_DNS_MESSAGE *pMsg);
 *
 *\history     2013/02/06, created this file, wangquzhi
 *
 *
 */

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/
#ifndef DNSPARSE_API_H
#define DNSPARSE_API_H


/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include "dnsparse_typedef.h"
#include "dnsparse_error.h"
#include "dnsparse_buffer.h"
#include "dnsparse_message.h"

#include "dnsparse_testsuite.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/

IP_PUBLIC dnsparse_result_t
dnsparse_message_parse(DNSPARSE_DNS_MESSAGE *pMsg, const char *pDnsPkt, int dnsPktLen);

#endif /* DNSPARSE_API_H */
