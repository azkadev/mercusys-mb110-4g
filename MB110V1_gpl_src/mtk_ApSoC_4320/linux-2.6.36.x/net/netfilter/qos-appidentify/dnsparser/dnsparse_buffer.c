/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_buffer.c
 *\brief
 *
 *\author      wangquzhi
 *\version
 *\date        2013/01/30
 *
 *\history     2013/01/30, created this file, wangquzhi
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

#include "dnsparse_error.h"
#include "dnsparse_buffer.h"
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
IP_GLOBAL dnsparse_result_t
dnsparse_buffer_getuint8(Ip_u8 *pDst, DNSPARSE_BUFFER *pbuffer)
{

    DNSPARSE_ASSERT(((pbuffer != NULL) && DNSPARSE_BUFFER_VALID(pbuffer)), DNSPARSE_INVALID_ARG);

    if (!(DNSPARSE_BUFFER_AVALIABLE(pbuffer) >= 0))
    {
        return DNSPARSE_BUFFER_ACCESS_DENIED;
    }
    *pDst = *(Ip_u8 *)DNSPARSE_BUFFER_CURSOR(pbuffer);
    pbuffer->cursor++;

    return DNSPARSE_SUCCESS;
}

IP_GLOBAL dnsparse_result_t
dnsparse_buffer_getuint16(Ip_u16 *pDst, DNSPARSE_BUFFER *pbuffer)
{
    Ip_u8 *pBufCursor = NULL;

    DNSPARSE_ASSERT(((pbuffer != NULL) && DNSPARSE_BUFFER_VALID(pbuffer)), DNSPARSE_INVALID_ARG);

    if (!(DNSPARSE_BUFFER_AVALIABLE(pbuffer) >= 2))
    {
        return DNSPARSE_BUFFER_ACCESS_DENIED;
    }
    pBufCursor = DNSPARSE_BUFFER_CURSOR(pbuffer);
    *pDst = (Ip_u16)(pBufCursor[0]);
    *pDst = *pDst << 8;
    *pDst += (Ip_u16)(pBufCursor[1]);
    pbuffer->cursor += 2;

    return DNSPARSE_SUCCESS;
}

IP_GLOBAL dnsparse_result_t
dnsparse_buffer_getuint32(Ip_u32 *pDst, DNSPARSE_BUFFER *pbuffer)
{
    Ip_u8 *pBufCursor = NULL;

    DNSPARSE_ASSERT(((pbuffer != NULL) && DNSPARSE_BUFFER_VALID(pbuffer)), DNSPARSE_INVALID_ARG);
    if (!(DNSPARSE_BUFFER_AVALIABLE(pbuffer) >= 2))
    {
        return DNSPARSE_BUFFER_ACCESS_DENIED;
    }
    pBufCursor = DNSPARSE_BUFFER_CURSOR(pbuffer);
    *pDst = (Ip_u16)(pBufCursor[0]);
    *pDst = *pDst << 8;
    *pDst += (Ip_u16)(pBufCursor[1]);
    *pDst = *pDst << 8;
    *pDst += (Ip_u16)(pBufCursor[2]);
    *pDst = *pDst << 8;
    *pDst += (Ip_u16)(pBufCursor[3]);
    pbuffer->cursor += 4;

    return DNSPARSE_SUCCESS;
}

IP_GLOBAL dnsparse_result_t
dnsparse_buffer_seek(DNSPARSE_BUFFER *pbuffer, int offset, int pos)
{
    DNSPARSE_ASSERT(pbuffer != NULL, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(DNSPARSE_BUFFER_VALID(pbuffer), DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT((pos >= 0 && pos <=2), DNSPARSE_INVALID_ARG); /* from end not supported */

    if(pos == 0) /* from beginning */
    {
        DNSPARSE_ASSERT(((DNSPARSE_BUFFER_SIZE(pbuffer) > offset) && (offset >= 0)), \
                          DNSPARSE_INVALID_ARG);
        pbuffer->cursor = offset;
    }
    else if (pos == 1) /* from current */
    {
        DNSPARSE_ASSERT(((DNSPARSE_BUFFER_AVALIABLE(pbuffer) > offset) && (offset >= 0)), \
                          DNSPARSE_INVALID_ARG);
        pbuffer->cursor += offset;
    }
    else
    {
        ; /* do nothing */
    }
    return DNSPARSE_SUCCESS;
}
