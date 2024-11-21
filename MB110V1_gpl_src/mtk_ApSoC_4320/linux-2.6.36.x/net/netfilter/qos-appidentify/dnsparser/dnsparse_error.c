/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_error.c
 *\brief       This is a sample of coding criterion.
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

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/kernel.h>
#include <linux/types.h>

#include "dnsparse_typedef.h"
#include "dnsparse_config.h"
#include "dnsparse_error.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/



/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct _DNSPARSE_ERRORMSG_T
{
    unsigned int type;
    const char  *msg;
}DNSPARSE_ERRORMSG_T;

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
IP_STATIC DNSPARSE_ERRORMSG_T _dnsparse_errormsg[] = {
    {DNSPARSE_INVALID_ARG,    "argument is not valid" },
    {DNSPARSE_NOMEMORY,       "memory runs out"},
    {DNSPARSE_MEMPOOL_REINIT, "mempool has already been inited"},
    {DNSPARSE_NAMEBUF_FULL,   "size of the given buffer is not enough when parsing name"},
    {DNSPARSE_NAMETOOLONG,    "a too long name found in dns packet"},
    {DNSPARSE_BADLABELTYPE,   "bad label type found in dns packet"},
    {DNSPARSE_BADPOINTER,     "bad pointer found in dns packet"},
    {DNSPARSE_PARSE_FATAL_ERROR, "something should't happen happened when parsing name"},
    {DNSPARSE_UNEXPECTEDEND,  "unexpected end met when parsing name"},
    {DNSPARSE_BADHEAD_ZERO,   "bad header found in dns packet(zero flag is dirty)"},
    {DNSPARSE_BADHEAD_QUESTIONMISSING, "bad header found in dns packet(question section missing)"},
    {DNSPARSE_BADHEAD_BROKEN, "dns header is not complete"},
    {DNSPARSE_INVALID_MEMREQUEST, "invalid memory request found when getting free memory"},
    {DNSPARSE_BUFFER_ACCESS_DENIED, "accessed memory which is out of buffer"},
    {DNSPARSE_RRCLASS_NOTIN,  "rr class not IN, not supported or malformed packet"},
    {DNSPARSE_RRTYPE_NOTSUPPORTED, "rr type not supported"},
    {DNSPARSE_RR_TYPELENNOTMATCHED, "rr type and rdata length not matched"},
    {DNSPARSE_QUERYPKT,       "the dns packet doesn't contain query result"},
    {DNSPARSE_RCODE_ERROR,    "dns packet rcode says error"},
    {DNSPARSE_MEMPOOL_NOTINITED, "mempool must be inited first"},

    {DNSPARSE_IMPOSSIBLE,     "this error is impossible"},
    {DNSPARSE_SUCCESS,        "everything is OK!"},
    {DNSPARSE_ERRORMSG_END,   NULL}
};

/**************************************************************************************************/
/*                                      LOCAL FUNCTIONS                                           */
/**************************************************************************************************/



/**************************************************************************************************/
/*                                      PUBLIC FUNCTIONS                                          */
/**************************************************************************************************/
IP_PUBLIC void
_dnsparse_requre(const char *expr, int errortype, const char *file, Ip_u32 line)
{
    const char *msgText;

    msgText = _dnsparse_error_type2msg(errortype);
}

IP_PUBLIC void
_dnsparse_assert(const char *expr, const char *file, Ip_u32 line)
{
#ifdef ENABLE_DNSPARSE_ASSERT
    printk("*** Failed assertion '%s' at %s:%d ***\r\n",
                       expr, file, line);

#else

#endif
}

IP_PUBLIC void
dnsparse_print_errormsg(dnsparse_result_t error)
{
    const char *msgText;

    msgText = _dnsparse_error_type2msg(error);
    printk("ERROR: %s\r\n", msgText);
}
/**************************************************************************************************/
/*                                      GLOBAL FUNCTIONS                                          */
/**************************************************************************************************/
IP_GLOBAL const char *
_dnsparse_error_type2msg(int errortype)
{
    DNSPARSE_ERRORMSG_T *pMsgArray = NULL;

    for (pMsgArray = _dnsparse_errormsg; pMsgArray->msg != NULL; pMsgArray++)
    {
        if (pMsgArray->type == errortype)
        {
            return (pMsgArray->msg);
        }
    }

    /* Cannot go here! */
    return NULL;        /* to make the compile happy. */
}
