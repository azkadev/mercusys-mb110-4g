/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_buffer.h
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
#ifndef DNSPARSE_BUFFER_H
#define DNSPARSE_BUFFER_H
/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include "dnsparse_typedef.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
/* tools */
#define dnsparse_ntohl(x) ntohl(x)
#define dnsparse_ntohs(x) ntohs(x)

/* Caution: all arguments of the following MACROs are not varified!!!  */
/* MACROs to get properties of DNSPARSE_BUFFER */
#define DNSPARSE_BUFFER_SIZE(pbuffer)      ((pbuffer)->size)
#define DNSPARSE_BUFFER_FREE(pbuffer)      ((pbuffer)->size - (pbuffer)->used)
#define DNSPARSE_BUFFER_AVALIABLE(pbuffer) ((pbuffer)->size - (pbuffer)->cursor)
#define DNSPARSE_BUFFER_BASE(pbuffer)      ((pbuffer)->base)
#define DNSPARSE_BUFFER_CURSOR(pbuffer)    ((pbuffer)->base + (pbuffer)->cursor)
#define DNSPARSE_BUFFER_USED(pbuffer)      ((pbuffer)->base + (pbuffer)->used)
#define DNSPARSE_BUFFER_END(pbuffer)       ((pbuffer)->base + (pbuffer)->size)
#define DNSPARSE_BUFFER_VALID(pbuffer)     (((pbuffer)->base != NULL) \
                                         && ((pbuffer)->size >= 0)\
                                         && ((pbuffer)->cursor >=0)\
                                         && ((pbuffer)->used >=0))
/* MACROs to set properties of DNSPARSE_BUFFER */
#define DNSPARSE_BUFFER_CLEAR(pbuffer)   do \
                                         {\
                                             (pbuffer)->base = NULL;\
                                             (pbuffer)->size = 0;\
                                             (pbuffer)->used = 0;\
                                             (pbuffer)->cursor= 0;\
                                         }while(0)
#define DNSPARSE_BUFFER_INIT(pbuffer, bufferbase, buffersize)   do \
                                         {\
                                             (pbuffer)->base = (void *)(bufferbase);\
                                             (pbuffer)->size = (buffersize);\
                                             (pbuffer)->used = 0;\
                                             (pbuffer)->cursor = 0;\
                                         }while(0)

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/* directly access to this structure is not permitted. Please access it via
 * DNSPARSE_BUFFER_XXX MACROs defined above. */
typedef struct _DNSPARSE_BUFFER
{
    int    size;
    int    used;         /* used when storing(writing) */
    int    cursor;    /* used when parsing(reading) */
    Ip_u8 *base;
    Ip_u8  buffer[0];
}DNSPARSE_BUFFER;


/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
IP_GLOBAL dnsparse_result_t
dnsparse_buffer_getuint8 (Ip_u8 *pDst, DNSPARSE_BUFFER *pbuffer);
IP_GLOBAL dnsparse_result_t
dnsparse_buffer_getuint16(Ip_u16 *pDst, DNSPARSE_BUFFER *pbuffer);
IP_GLOBAL dnsparse_result_t
dnsparse_buffer_getuint32(Ip_u32 *pDst, DNSPARSE_BUFFER *pbuffer);
IP_GLOBAL dnsparse_result_t
dnsparse_buffer_seek(DNSPARSE_BUFFER *pbuffer, int offset, int pos);


#endif /* DNSPARSE_BUFFER_H */
