/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_error.h
 *\brief
 *
 *\author      wangquzhi
 *\version
 *\date        2013/01/30
 *
 *\history     2013/01/30, wangquzhi create this file
 *
 *
 */


/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/
#ifndef DNSPARSE_ERROR_H
#define DNSPARSE_ERROR_H


/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include "dnsparse_typedef.h"
#include "dnsparse_config.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
/* error type list */
#define DNSPARSE_SUCCESS                             ( 0)
#define DNSPARSE_ERRORMSG_END                        (-1)
#define DNSPARSE_INVALID_ARG                         ( 1)
#define DNSPARSE_NOMEMORY                            ( 2)
#define DNSPARSE_MEMPOOL_REINIT                      ( 3)
#define DNSPARSE_NAMETOOLONG                         ( 4)
#define DNSPARSE_NAMEBUF_FULL                        ( 5)
#define DNSPARSE_BADLABELTYPE                        ( 6)
#define DNSPARSE_BADPOINTER                          ( 7)
#define DNSPARSE_PARSE_FATAL_ERROR                   ( 8)
#define DNSPARSE_UNEXPECTEDEND                       ( 9)
#define DNSPARSE_BADHEAD_ZERO                        (10)
#define DNSPARSE_BADHEAD_QUESTIONMISSING             (11)
#define DNSPARSE_BADHEAD_BROKEN                      (12)
#define DNSPARSE_INVALID_MEMREQUEST                  (13)
#define DNSPARSE_BUFFER_ACCESS_DENIED                (14)
#define DNSPARSE_IMPOSSIBLE                          (15)
#define DNSPARSE_RRCLASS_NOTIN                       (16)
#define DNSPARSE_RRTYPE_NOTSUPPORTED                 (17)
#define DNSPARSE_RR_TYPELENNOTMATCHED                (18)
#define DNSPARSE_QUERYPKT                            (19)
#define DNSPARSE_RCODE_ERROR                         (20)
#define DNSPARSE_MEMPOOL_NOTINITED                   (21)


#define DNSPARSE_ASSERT(xexpr, errortype) do \
    {\
        if ((xexpr) == 0)\
        {\
            _dnsparse_requre(#xexpr, errortype, __FUNCTION__, __LINE__);\
            return errortype; \
        }\
    }while(0)

/* this MACRO can only be used in API function, this is intend to make it sure that
 * the user is invoking the API in a right way. */
#define DNSPARSE_REQURE(xexpr, errortype) do \
    {\
        if ((xexpr) == 0)\
        {\
            _dnsparse_requre(#xexpr, errortype, __FUNCTION__, __LINE__);\
            return errortype; \
        }\
    }while(0)


#ifdef ENABLE_DNSPARSE_DEBUG
#define DNSPARSE_DEBUG(info, args...) \
    printk("[%s][%5d]: "info"\r\n", __FUNCTION__, __LINE__, ##args)
#else
#define DNSPARSE_DEBUG(info, args...)
#endif
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef int dnsparse_result_t;

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/
IP_PUBLIC void
_dnsparse_assert(const char *expr, const char *file, Ip_u32 line);

IP_PUBLIC void
_dnsparse_requre(const char *expr, int errortype, const char *file, Ip_u32 line);

IP_PUBLIC void
dnsparse_print_errormsg(dnsparse_result_t error);

IP_GLOBAL const char *
_dnsparse_error_type2msg(int errortype);
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC FUNCTIONS                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      GLOBAL FUNCTIONS                                          */
/**************************************************************************************************/


#endif /* DNSPARSE_ERROR_H */
