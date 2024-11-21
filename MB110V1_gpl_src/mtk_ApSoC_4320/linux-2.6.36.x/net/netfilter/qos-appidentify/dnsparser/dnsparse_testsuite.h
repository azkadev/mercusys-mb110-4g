/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_message.h
 *\brief
 *
 *\author      wangquzhi
 *\version
 *\date        2013/02/16
 *
 *\history     2013/02/16, created this file, wangquzhi
 *
 *
 */

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/
#ifndef DNSPARSE_TESTSUITE_H
#define DNSPARSE_TESTSUITE_H


/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include "dnsparse_typedef.h"
#include "dnsparse_message.h"
#include "dnsparse_config.h"
#include "dnsparse_error.h"
#include "dnsparse_buffer.h"

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
IP_PUBLIC void
dnsparse_test_hexdump(Ip_u8 *pData, int size);
IP_PUBLIC void
dnsparse_test_printQuestion(DNSPARSE_DNS_MESSAGE *pMsg);
IP_PUBLIC void
dnsparse_test_printSection(DNSPARSE_DNS_MESSAGE *pMsg, int sectionId);
IP_PUBLIC void
dnsparse_test_poolStatus(void);
IP_PUBLIC void
dnsparse_test_mctxDump(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_MCTXTYPE mctxType);
IP_PUBLIC void
dnsparse_test(void);

#endif /* DNSPARSE_TESTSUITE_H */
