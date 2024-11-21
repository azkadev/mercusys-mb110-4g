/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse.c
 *\brief
 *
 *\author      wangquzhi
 *\version
 *\date        2013/01/29
 *
 *\history     2013/01/29, created this file, wangquzhi
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
#include <linux/mempool.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/list.h>
#include <linux/string.h>
 
#include "dnsparse_message.h"
#include "dnsparse_buffer.h"
#include "dnsparse_error.h"
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
IP_STATIC dnsparse_result_t
_dnsparse_getFreeMem(DNSPARSE_DNS_MESSAGE*pMsg, DNSPARSE_MCTXTYPE mctxType,
                     int memSize, Ip_u8 **ppNameStoredTo);
IP_STATIC dnsparse_result_t
_dnsparse_getname(char *pNameBuf, int bufLen,  DNSPARSE_BUFFER *pDnsPktBuf);
IP_STATIC dnsparse_result_t
_dnsparse_getquestion(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_BUFFER *pDnsPktBuf);
IP_STATIC dnsparse_result_t
_dnsparse_getsection(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_BUFFER *pDnsPktBuf, int sectionId);

IP_PUBLIC dnsparse_result_t
dnsparse_message_parse(DNSPARSE_DNS_MESSAGE *pMsg, const char *pDnsPkt, int dnsPktLen);
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL FUNCTIONS                                           */
/**************************************************************************************************/
/*!
 *\fn            IP_STATIC dnsparse_result_t
 *               _dnsparse_getFreeMem(DNSPARSE_DNS_MESSAGE*pMsg, DNSPARSE_MCTXTYPE mctxType,
 *                                    int memSize, Ip_u8 **ppNameStoredTo)
 *\brief
 *\details       we need memory to store name or ip addr got from dns packet.
 *               first we try to get a memory whose size is not less than 'memSize' in mctx list.
 *               if this doesn't succeed, try to alloc new memmory block from mctxType pool.
 *
 *\param[in]     DNSPARSE_DNS_MESSAGE*pMsg
 *\param[in]     DNSPARSE_MCTXTYPE mctxType
 *\param[in]     int memSize
 *\param[out]    char **ppNameStoredTo
 *
 *\return        DNSPARSE_SUCCESS                everything is OK
 *               DNSPARSE_INVALID_MEMREQUEST     shouldn't happen
 *               retcodes got from functions invoked by this function
 *
 *\note
 */
IP_STATIC dnsparse_result_t
_dnsparse_getFreeMem(DNSPARSE_DNS_MESSAGE*pMsg, DNSPARSE_MCTXTYPE mctxType,
                     int memSize, Ip_u8 **ppNameStoredTo)
{
    dnsparse_result_t ret = DNSPARSE_SUCCESS;
    struct list_head *pMemListHead = NULL;
    struct list_head *pMemListNode = NULL;
    dnsparse_result_t (*newMemHandle)(DNSPARSE_DNS_MESSAGE *pMsg) = NULL;
    DNSPARSE_MCTX_COMMON  *pMctx = NULL;
    DNSPARSE_BUFFER *pBuffer = NULL;
    int freeSize = 0;

    DNSPARSE_ASSERT(pMsg != NULL, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(memSize > 0, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(ppNameStoredTo != NULL, DNSPARSE_INVALID_ARG);
    switch (mctxType)
    {
        case DNSPARSE_MCTXTYPE_NAME:
            pMemListHead = &(pMsg->mctx.nameCtx);
            newMemHandle = dnsparse_new_nameCtx;
            break;
        case DNSPARSE_MCTXTYPE_ADDR4:
            pMemListHead = &(pMsg->mctx.addr4Ctx);
            newMemHandle = dnsparse_new_addr4Ctx;
            break;
        default:
            /* this cannot happen! */
            return DNSPARSE_INVALID_MEMREQUEST;
    }

    DNSPARSE_DEBUG("searching for free block in list");
    list_for_each(pMemListNode, pMemListHead)
    {
        pMctx   = list_entry(pMemListNode, DNSPARSE_MCTX_COMMON, listNode);
        pBuffer = &(pMctx->memBlk);
        freeSize = DNSPARSE_BUFFER_FREE(pBuffer);

        if (freeSize >= memSize)
        {
            break;
        }
    }

    if (pMemListNode != pMemListHead)  /* case that we found a free block */
    {
        DNSPARSE_DEBUG("a free block found");
        *ppNameStoredTo = DNSPARSE_BUFFER_USED(pBuffer);
        pBuffer->used += memSize;
        return DNSPARSE_SUCCESS;
    }
    else /* no memory avaliable, alloc it now */
    {
        DNSPARSE_DEBUG("no free block, alloc now");
        ret = newMemHandle(pMsg);
        if (ret != DNSPARSE_SUCCESS)
        {
            return ret;
        }
        DNSPARSE_DEBUG("tracing");
        pMemListNode = pMemListHead->prev;
        DNSPARSE_DEBUG("tracing");
        pBuffer  = &(((DNSPARSE_MCTX_COMMON *)pMemListNode)->memBlk);
        DNSPARSE_DEBUG("tracing");
        *ppNameStoredTo = DNSPARSE_BUFFER_USED(pBuffer);
        DNSPARSE_DEBUG("tracing");
        pBuffer->used += memSize;
        DNSPARSE_DEBUG("tracing");
        return DNSPARSE_SUCCESS;
    }
}


/*!
 *\fn            IP_STATIC dnsparse_result_t
 *               _dnsparse_getname(char *pNameBuff, int bufLen,  DNSPARSE_BUFFER *pDnsPktBuf)
 *\brief         get domain from a raw dns packet
 *\details
 *
 *\param[out]    char *pNameBuff
 *\param[in]     int bufLen
 *\param[io]     DNSPARSE_BUFFER *pDnsPktBuf
 *
 *\return        DNSPARSE_SUCCESS       everything is OK
 *               DNSPARSE_NAMEBUF_FULL
 *               DNSPARSE_NAMETOOLONG
 *               DNSPARSE_BADLABELTYPE
 *               DNSPARSE_BADPOINTER
 *               DNSPARSE_UNEXPECTEDEND
 *               DNSPARSE_PARSE_FATAL_ERROR
 *
 *
 *\note          when this function is invoked, it must be made sure that the 'cursor' has
 *               been set to pointed to the beginning of a name field(either the beginning
 *               of a RR, or the beginning of RData).
 *               after the name is parsed, the cursor will be set to the end of the name field.
 */

IP_STATIC dnsparse_result_t
_dnsparse_getname(char *pNameBuf, int bufLen,  DNSPARSE_BUFFER *pDnsPktBuf)
{
    Ip_bool done         = IP_FALSE;
    Ip_bool seen_pointer = IP_FALSE;
    Ip_bool downcase     = IP_TRUE;
    enum {
        getname_start = 0,
        getname_ordinary,
        getname_copy,
        getname_newcurrent
    }getname_state = getname_start;
    unsigned char *BUFFER_BASE = DNSPARSE_BUFFER_BASE(pDnsPktBuf);
    unsigned char *BUFFER_END  = DNSPARSE_BUFFER_END(pDnsPktBuf);
    unsigned char *pCurrent     = DNSPARSE_BUFFER_CURSOR(pDnsPktBuf);
    unsigned char *biggest_pointer = pCurrent;
    unsigned int   new_current_offset = 0;
    unsigned char *new_current = NULL;
    /*unsigned char *cData       = NULL;*/
    unsigned char *nData       = (unsigned char *)pNameBuf;

    unsigned int cUsed = 0;    /* bytes of compressed name data used */
    unsigned int nUsed = 0;
    unsigned int n     = 0;    /* number of char to copy */
    unsigned int nMax  = bufLen;
    unsigned int c     = 0;    /* char read from pkt each time */

    DNSPARSE_ASSERT(pNameBuf != NULL, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT((nMax > 0 && nMax <= DNSPARSE_DNS_NAME_MAXWIRE), DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(DNSPARSE_BUFFER_VALID(pDnsPktBuf), DNSPARSE_INVALID_ARG);

    /* loop prevention is performed by checking the range of the pointer */
    while (pCurrent < BUFFER_END && !done)
    {
        c = *pCurrent++;
        DNSPARSE_DEBUG("New data read: 0x%02x", c);
        if (!seen_pointer)
        {
            cUsed++;
        }

        switch (getname_state)
        {
            case getname_start:
                if (c < 64) /* 'c' is the length of the name, 0~63(c is unsigned, always >= 0) */
                {
                    DNSPARSE_DEBUG("Found name length: %d.", c);
                    if (nUsed + c + 1 > nMax)  /* buff full */
                    {
                        DNSPARSE_DEBUG("Buffer full>used: %d max: %d.", nUsed, nMax);
                        goto full;
                    }
                    nUsed += c + 1;
                    if (c == 0) /* this is EOF of the name */
                    {
                        DNSPARSE_DEBUG("EOF of name detected.");
                        *nData++ = c;  /* '\0' */
                        done = IP_TRUE;
                        break;
                    }
                    n = c;
                    DNSPARSE_DEBUG("Change state from 'start' to 'ordinary'.");
                    getname_state = getname_ordinary;
                }
                else if (c > 128 && c < 192) /* 14 bit local compression pointer. */
                {                            /* Local compression is no longer an IETF draft.*/
                    DNSPARSE_DEBUG("14 bit local compression pointer found(not supported)");
                    return DNSPARSE_BADLABELTYPE;
                }
                else if (c >=192)            /* 'c' is a compress pointer */
                {
                    DNSPARSE_DEBUG("Compression pointer found.");
                    new_current_offset = c & 0x3F;/* now new_current_offset is only part of offset*/
                    n = 1;
                    DNSPARSE_DEBUG("Change state from 'start' to 'newcurrent'.");
                    getname_state = getname_newcurrent;
                }
                else /* DNS parser don't know what is this. */
                {
                    DNSPARSE_DEBUG("DNS bad label found.");
                    return DNSPARSE_BADLABELTYPE;
                }
                break;
            case getname_ordinary:
                DNSPARSE_DEBUG("In state 'ordinary.'");
                if (downcase)
                {
                    c = tolower(c);
                }
                /* GO DOWN!!! NOT BREAKED!!! */
            case getname_copy:
                DNSPARSE_DEBUG("Copy data '%c'(0x%02x)", c, c);
                *nData++ = c;
                n--;
                DNSPARSE_DEBUG("%d data left.", n);
                if (0 == n)
                {
                   DNSPARSE_DEBUG("Writing point '.'");
                   *nData++ = '.';
                   DNSPARSE_DEBUG("Change state from 'ordinary(in copy)' to 'start'");
                   getname_state = getname_start;
                }
                break;
            case getname_newcurrent:
                new_current_offset <<= 8;
                new_current_offset += c;
                DNSPARSE_DEBUG("Got new pointer offset: 0x%04x", new_current_offset);
                new_current = BUFFER_BASE + new_current_offset;
                n--;
                if (n != 0) /* ?? */
                {
                    DNSPARSE_DEBUG("?? n != 0 ?? in state new current");
                    break;
                }
                if (new_current >= biggest_pointer) /* we only refer to data before us. */
                {
                    DNSPARSE_DEBUG("*** Pointer go further, this should not happen!");
                    return DNSPARSE_BADPOINTER;
                }
                biggest_pointer = new_current;
                pCurrent = new_current;
                seen_pointer = IP_TRUE;
                DNSPARSE_DEBUG("Change state from 'newcurrent' to 'start'");
                getname_state = getname_start;
                break;
            default:   /* can not happen!!!, but      this really happened, we cannot suspend the
                        * the task. Just give a gentle error message. */
                DNSPARSE_DEBUG("*** Switch default branch. This Shouldn't happen!");
                return DNSPARSE_PARSE_FATAL_ERROR;
        }
    }

    if (!done)
    {
        return DNSPARSE_UNEXPECTEDEND;
    }
    DNSPARSE_DEBUG("Length of name: %d", strlen(pNameBuf));
    DNSPARSE_DEBUG("cUsed value   : %d", cUsed);
    DNSPARSE_DEBUG("name          : %s", pNameBuf);
    DNSPARSE_DEBUG("Leaving getname");

    dnsparse_buffer_seek(pDnsPktBuf, cUsed, 1);  /* seek from current position */

    return DNSPARSE_SUCCESS;

full:
    if (nMax == DNSPARSE_DNS_NAME_MAXWIRE)
    {
        /* it's not my fault, the name is too long than exceed the standard size,
         * we cannot handle this name even though we had a buffer big enough.*/
        return DNSPARSE_NAMETOOLONG;
    }
    else
    {
        /* give me more memory, I will handle it gracefully. */
        return DNSPARSE_NAMEBUF_FULL;
    }
}

/*!
 *\fn            IP_STATIC dnsparse_result_t
 *               _dnsparse_getquestion(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_BUFFER *pDnsPktBuf)
 *\brief         get question section from a raw dns packet, and save result to DNSPARSE_DNS_MESSAGE
 *\details
 *
 *\param[out]    DNSPARSE_DNS_MESSAGE *pMsg
 *\param[io]     DNSPARSE_BUFFER *pDnsPktBuf
 *
 *\return        DNSPARSE_SUCCESS       everything is OK
 *
 *
 *
 *\note          when this function is invoked, it must be made sure that the 'cursor' has
 *               been set to pointed to the beginning of a question section field.
 *               after the name is parsed, the cursor will be set to the end of the name field.
 */
IP_STATIC dnsparse_result_t
_dnsparse_getquestion(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_BUFFER *pDnsPktBuf)
{
    dnsparse_result_t ret = DNSPARSE_SUCCESS;
    int    count = 0;          /* number quesions in question section */
    char   nameBuf[DNSPARSE_DNS_NAME_MAXWIRE] = {0};
    int    nameLength = 0;     /* length of the name got by parsing dns packet */
    Ip_u8 *pNameStoredTo = NULL;
    DNSPARSE_RR_TYPE *pNewGotRREntry = NULL;

    DNSPARSE_ASSERT(NULL != pMsg, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(NULL != pDnsPktBuf, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(DNSPARSE_BUFFER_VALID(pDnsPktBuf), DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT((pDnsPktBuf->cursor == DNSPARSE_DNS_HEADER_LEN), DNSPARSE_INVALID_ARG);

    for (count = 0; count < pMsg->counts[DNSPARSE_DNS_SECTION_QUESTION]; count++)
    {
        /* Firstly, add a new RR entry to QUESTION section */
        ret = dnsparse_new_rr(pMsg, DNSPARSE_DNS_SECTION_QUESTION);
        if (DNSPARSE_SUCCESS != ret)
        {
            DNSPARSE_DEBUG("Processing Name :%d", count);
            DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
            return ret;
        }

        /* Get the name to our local buffer */
        ret =  _dnsparse_getname(nameBuf, DNSPARSE_DNS_NAME_MAXWIRE, pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
            return ret;
        }

        /* get memory from mctx.nameCtx, store name to it, and link this address to rr entry */
        nameLength = strlen(nameBuf);
        ret = _dnsparse_getFreeMem(pMsg, DNSPARSE_MCTXTYPE_NAME, nameLength + 1, &pNameStoredTo);
        if (DNSPARSE_SUCCESS != ret)
        {
            DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
            return ret;
        }
        pNewGotRREntry = (DNSPARSE_RR_TYPE *)(pMsg->sections[DNSPARSE_DNS_SECTION_QUESTION].prev);
        strcpy((char *)pNameStoredTo, (const char *)nameBuf);
        pNewGotRREntry->name = (DNSPARSE_NAME)pNameStoredTo;

        /* get type and class info */
        ret = dnsparse_buffer_getuint16(&(pNewGotRREntry->type), pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        ret = dnsparse_buffer_getuint16(&(pNewGotRREntry->klass), pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
    }

    return DNSPARSE_SUCCESS;
}


/*!
 *\fn            IP_STATIC dnsparse_result_t
 *               _dnsparse_getsection(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_BUFFER *pDnsPktBuf)
 *\brief         get a section from a raw dns packet, and save result to DNSPARSE_DNS_MESSAGE
 *\details
 *
 *\param[out]    DNSPARSE_DNS_MESSAGE *pMsg
 *\param[io]     DNSPARSE_BUFFER *pDnsPktBuf
 *
 *\return        DNSPARSE_SUCCESS       everything is OK
 *               DNSPARSE_IMPOSSIBLE
 *
 *
 *\note          when this function is invoked, it must be made sure that the 'cursor' has
 *               been set to pointed to the beginning of a question section field.
 *               after the name is parsed, the cursor will be set to the end of the name field.
 */
IP_STATIC dnsparse_result_t
_dnsparse_getsection(DNSPARSE_DNS_MESSAGE *pMsg, DNSPARSE_BUFFER *pDnsPktBuf, int sectionId)
{
    dnsparse_result_t ret = DNSPARSE_SUCCESS;
    int    count = 0;
    char   nameBuf[DNSPARSE_DNS_NAME_MAXWIRE] = {0};
    int    nameLength = 0;
    Ip_u8 *pNameStoredTo  = NULL;
    Ip_u8 *pAddr4StoredTo = NULL;
    DNSPARSE_RR_TYPE *pNewGotRREntry = NULL;
    Ip_u16 RDataLength = 0;
    Ip_u32 ipaddr_A = 0;

    DNSPARSE_ASSERT(NULL != pMsg, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(NULL != pDnsPktBuf, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT(DNSPARSE_BUFFER_VALID(pDnsPktBuf), DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT((sectionId >=0 && sectionId < DNSPARSE_DNS_SECTION_MAX), DNSPARSE_INVALID_ARG);

    for (count = 0; count < pMsg->counts[sectionId]; count++)
    {
        /* Firstly, add a new RR entry to section */
        ret = dnsparse_new_rr(pMsg, sectionId);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        pNewGotRREntry = (DNSPARSE_RR_TYPE *)(pMsg->sections[sectionId].prev);

        /* Get the name to our local buffer */
        DNSPARSE_DEBUG("count: %d, begin parsing name", count);
        ret = _dnsparse_getname(nameBuf, DNSPARSE_DNS_NAME_MAXWIRE, pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        /* get memory from mctx.nameCtx, store name to it, and link this address to rr entry */
        nameLength = strlen(nameBuf);
        ret = _dnsparse_getFreeMem(pMsg, DNSPARSE_MCTXTYPE_NAME, nameLength + 1, &pNameStoredTo);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        DNSPARSE_DEBUG("tracing");
        strcpy((char *)pNameStoredTo, (const char *)nameBuf);
        DNSPARSE_DEBUG("tracing");
        pNewGotRREntry->name = (DNSPARSE_NAME)pNameStoredTo;
        DNSPARSE_DEBUG("tracing");

        /* get type and class info */
        ret = dnsparse_buffer_getuint16(&(pNewGotRREntry->type), pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        DNSPARSE_DEBUG("tracing");
        ret = dnsparse_buffer_getuint16(&(pNewGotRREntry->klass), pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        DNSPARSE_DEBUG("tracing");
        ret = dnsparse_buffer_getuint32(&(pNewGotRREntry->ttl), pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }
        DNSPARSE_DEBUG("tracing");
        ret = dnsparse_buffer_getuint16(&RDataLength, pDnsPktBuf);
        if (DNSPARSE_SUCCESS != ret)
        {
            return ret;
        }

        DNSPARSE_DEBUG("count: %d, begin parsing Rdata", count);

        /* which type the data is? */
        /* We only parse contents we are interested in(Now ONLY answer section). Sorry! */
        if (DNSPARSE_DNS_RR_CLASS_IN != pNewGotRREntry->klass)
        {
            return DNSPARSE_RRCLASS_NOTIN;
        }

        /* we have make sure class is IN, now consider type */
        switch (pNewGotRREntry->type)
        {
            case DNSPARSE_DNS_RR_TYPE_A:
                if (RDataLength != DNSPARSE_DNS_RR_TYPE_A_DATALEN)
                {
                    return DNSPARSE_RR_TYPELENNOTMATCHED;
                }
                /* get ip address of type A */
                ret = dnsparse_buffer_getuint32(&ipaddr_A, pDnsPktBuf);
                if (DNSPARSE_SUCCESS != ret)
                {
                    return ret;
                }
                /* get free memory to store it */
                ret = _dnsparse_getFreeMem(pMsg, DNSPARSE_MCTXTYPE_ADDR4,
                                           DNSPARSE_DNS_RR_TYPE_A_DATALEN, &pAddr4StoredTo);
                if (DNSPARSE_SUCCESS != ret)
                {
                    return ret;
                }
                DNSPARSE_DEBUG("ipaddr %08x.\r\n", ipaddr_A);
                *(Ip_u32 *)pAddr4StoredTo = ipaddr_A;  /* pAddr4StoredTo should be 4byte aligned. */
                pNewGotRREntry->pRData = (void *)pAddr4StoredTo;
                break;
            case DNSPARSE_DNS_RR_TYPE_CNAME:

                /* Get the name to our local buffer */
                ret =  _dnsparse_getname(nameBuf, DNSPARSE_DNS_NAME_MAXWIRE, pDnsPktBuf);
                if (DNSPARSE_SUCCESS != ret)
                {
                    DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
                    return ret;
                }
                /* get memory from mctx.nameCtx, store name to it, and link this address to rr entry */
                nameLength = strlen(nameBuf);
                ret = _dnsparse_getFreeMem(pMsg, DNSPARSE_MCTXTYPE_NAME, nameLength + 1, &pNameStoredTo);
                if (DNSPARSE_SUCCESS != ret)
                {
                    DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
                    return ret;
                }

                DNSPARSE_DEBUG("copying cname to buffer in pool");
                strcpy((char *)pNameStoredTo, (char *)nameBuf);
                DNSPARSE_DEBUG("make link of cname");
                pNewGotRREntry->pRData = (DNSPARSE_NAME)pNameStoredTo;
                break;
            case DNSPARSE_DNS_RR_TYPE_NS:
            default:
                return DNSPARSE_RRTYPE_NOTSUPPORTED;
        }
    }
    return DNSPARSE_SUCCESS;
}





/**************************************************************************************************/
/*                                      PUBLIC FUNCTIONS                                          */
/**************************************************************************************************/

/*!
 *\fn            IP_PUBLIC dnsparse_result_t
 *               dnsparse_message_parse(DNSPARSE_DNS_MESSAGE *pMsg, const char *pDnsPkt, int dnsPktLen)
 *\brief         parse a dns message from raw dns packet
 *\details
 *
 *\param[out]    DNSPARSE_DNS_MESSAGE *pMsg
 *\param[in]     const char *pDnsPkt
 *\param[in]     int dnsPktLen
 *
 *\return        DNSPARSE_SUCCESS          everything is OK
 *               DNSPARSE_BADHEAD_ZERO     bad dns header(zero flag not clean)
 *               DNSPARSE_BADHEAD_BROKEN
 *               DNSPARSE_BADHEAD_ZERO
 *               DNSPARSE_BADHEAD_QUESTIONMISSING
 *
 *
 *
 *
 *\note       1. when this function is invoked, it must be made sure that the 'cursor' has
 *               been set to pointed to the beginning of a question section field.
 *               after the name is parsed, the cursor will be set to the end of the name field.
 *            2. about the ip4addr:
 *               ip4addr usually represented in 'Dotted Demical notation', and we currently use
 *               a 4-byte var to store a ip4addr. The first(most left) demical in a ip4addr is
 *               placed in the highest byte of the var, and the last(most right) byte of the var.
 */
IP_PUBLIC dnsparse_result_t
dnsparse_message_parse(DNSPARSE_DNS_MESSAGE *pMsg, const char *pDnsPkt, int dnsPktLen)
{
    DNSPARSE_BUFFER dnsPktBuf;
    dnsparse_result_t ret;
    /* char nameBuf[DNSPARSE_DNS_NAME_MAXWIRE] = {0}; */

    DNSPARSE_REQURE((pMsg    != NULL), DNSPARSE_INVALID_ARG);
    DNSPARSE_REQURE((pDnsPkt != NULL), DNSPARSE_INVALID_ARG);
    DNSPARSE_REQURE(g_dnsparse_mempool_inited == IP_TRUE, DNSPARSE_MEMPOOL_NOTINITED);

    if (dnsPktLen <= DNSPARSE_DNS_HEADER_LEN)
    {
        return DNSPARSE_BADHEAD_BROKEN;
    }

    DNSPARSE_BUFFER_INIT(&dnsPktBuf, pDnsPkt, dnsPktLen);

    ret = dnsparse_buffer_getuint16(&(pMsg->id), &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    ret = dnsparse_buffer_getuint16(&(pMsg->flags), &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    ret = dnsparse_buffer_getuint16(&(pMsg->counts[DNSPARSE_DNS_SECTION_QUESTION]), &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    ret = dnsparse_buffer_getuint16(&(pMsg->counts[DNSPARSE_DNS_SECTION_ANSWER]), &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    ret = dnsparse_buffer_getuint16(&(pMsg->counts[DNSPARSE_DNS_SECTION_AUTHORITY]), &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    ret = dnsparse_buffer_getuint16(&(pMsg->counts[DNSPARSE_DNS_SECTION_ADDITIONAL]), &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    /* end of dns packet header parsing, now checking for its correctness. */
    if (0 != DNSPARSE_GET_FLAG_ZERO(pMsg->flags))
    {
        return DNSPARSE_BADHEAD_ZERO;
    }
    if (0 == DNSPARSE_GET_FLAG_QR(pMsg->flags))
    {
        return DNSPARSE_QUERYPKT;    /* this is a query pkt, we only interested in answers. */
    }
    if (0 != DNSPARSE_GET_FLAG_RCODE(pMsg->flags))
    {
        return DNSPARSE_RCODE_ERROR;  /* something wrong while the server is answering the query */
    }
    if (0 == pMsg->counts[DNSPARSE_DNS_SECTION_QUESTION])
    {
        return DNSPARSE_BADHEAD_QUESTIONMISSING;
    }
    /* header check OK */

    ret = _dnsparse_getquestion(pMsg, &dnsPktBuf);
    if (DNSPARSE_SUCCESS != ret)
    {
        DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
        return ret;
    }
    DNSPARSE_DEBUG("get question end");

    DNSPARSE_DEBUG("dnsPktBuf Status> size: %d cursor: %d", dnsPktBuf.size, dnsPktBuf.cursor);
    ret = _dnsparse_getsection(pMsg, &dnsPktBuf, DNSPARSE_DNS_SECTION_ANSWER);
    if (DNSPARSE_SUCCESS != ret)
    {
        DNSPARSE_DEBUG("%s", _dnsparse_error_type2msg(ret));
        return ret;
    }
    DNSPARSE_DEBUG("get answer section end");

    #if 0
    ret = _dnsparse_getsection(pMsg, DNSPARSE_DNS_SECTION_AUTHORITY);
         (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    ret = _dnsparse_getsection(pMsg, DNSPARSE_DNS_SECTION_ADDITIONAL);
         (DNSPARSE_SUCCESS != ret)
    {
        return ret;
    }
    #endif

    return DNSPARSE_SUCCESS;
}


/**************************************************************************************************/
/*                                      GLOBAL FUNCTIONS                                          */
/**************************************************************************************************/
