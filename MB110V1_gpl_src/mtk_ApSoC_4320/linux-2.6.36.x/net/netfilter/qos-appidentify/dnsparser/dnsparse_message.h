/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_message.h
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
#ifndef DNSPARSE_MESSAGE_H
#define DNSPARSE_MESSAGE_H


/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/mempool.h>

#include "dnsparse_typedef.h"
#include "dnsparse_config.h"
#include "dnsparse_error.h"
#include "dnsparse_buffer.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define DNSPARSE_DNS_SECTION_MAX         (4)
#define DNSPARSE_DNS_SECTION_QUESTION    (0)
#define DNSPARSE_DNS_SECTION_ANSWER      (1)
#define DNSPARSE_DNS_SECTION_AUTHORITY   (2)
#define DNSPARSE_DNS_SECTION_ADDITIONAL  (3)

#define DNSPARSE_DNS_HEADER_LEN          (12)

/* standard size of a wire format name */
#define DNSPARSE_DNS_NAME_MAXWIRE        (255)

#define DNSPARSE_NAME_BUFFER_SIZE        (256)
#if DNSPARSE_NAME_BUFFER_SIZE < DNSPARSE_DNS_NAME_MAXWIRE
#error "DNSPARSE_NAME_BUFFER_SIZE must not less than DNSPARSE_DNS_NAME_MAXWIRE"
#endif
#define DNSPARSE_NAME_MEMPOOL_BLKSIZE    (sizeof(DNSPARSE_NAME_MCTX) + DNSPARSE_NAME_BUFFER_SIZE)
#define DNSPARSE_NAME_MEMPOOL_SIZE       (4)

#define DNSPARSE_ADDR4_BUFFER_SIZE       (2 * sizeof(DNSPARSE_ADDR4))
#define DNSPARSE_ADDR4_MEMPOOL_BLKSIZE   (sizeof(DNSPARSE_ADDR4_MCTX) + DNSPARSE_ADDR4_BUFFER_SIZE)
#define DNSPARSE_ADDR4_MEMPOOL_SIZE      (4)

#define DNSPARSE_RR_MEMPOOL_BLKSIZE      (sizeof(DNSPARSE_RR_TYPE))
#define DNSPARSE_RR_MEMPOOL_SIZE         (16)

/* flag masks in dns header */
#define DNSPARSE_FLAGS_QR_MASK           (0x8000)
#define DNSPARSE_FLAGS_OPCODE_MASK       (0x7800)
#define DNSPARSE_FLAGS_AA_MASK           (0x0400)
#define DNSPARSE_FLAGS_TC_MASK           (0x0200)
#define DNSPARSE_FLAGS_RD_MASK           (0x0100)
#define DNSPARSE_FLAGS_RA_MASK           (0x0080)
#define DNSPARSE_FLAGS_ZERO_MASK         (0x0070)
#define DNSPARSE_FLAGS_RCODE_MASK        (0x000F)

#define DNSPARSE_GET_FLAG_QR(flags)      (((flags) & DNSPARSE_FLAGS_QR_MASK) >> 15)
#define DNSPARSE_GET_FLAG_OPCODE(flags)  (((flags) & DNSPARSE_FLAGS_OPCODE_MASK) >> 11)
#define DNSPARSE_GET_FLAG_AA(flags)      (((flags) & DNSPARSE_FLAGS_AA_MASK) >> 10)
#define DNSPARSE_GET_FLAG_TC(flags)      (((flags) & DNSPARSE_FLAGS_TC_MASK) >> 9)
#define DNSPARSE_GET_FLAG_RD(flags)      (((flags) & DNSPARSE_FLAGS_RD_MASK) >> 8)
#define DNSPARSE_GET_FLAG_RA(flags)      (((flags) & DNSPARSE_FLAGS_RA_MASK) >> 7)
#define DNSPARSE_GET_FLAG_ZERO(flags)    (((flags) & DNSPARSE_FLAGS_ZERO_MASK) >> 4)
#define DNSPARSE_GET_FLAG_RCODE(flags)    ((flags) & DNSPARSE_FLAGS_RCODE_MASK)


#define DNSPARSE_DNS_RR_TYPE_A           ( 1)
#define DNSPARSE_DNS_RR_TYPE_NS          ( 2)
#define DNSPARSE_DNS_RR_TYPE_CNAME       ( 5)

#define DNSPARSE_DNS_RR_TYPE_A_DATALEN   ( 4)

#define DNSPARSE_DNS_RR_CLASS_IN         ( 1)

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef Ip_u32 DNSPARSE_ADDR4;
typedef char  *DNSPARSE_NAME;

typedef struct _DNSPARSE_MCTX_COMMON
{
    struct list_head listNode;
    /* 4 byte aligned */
    DNSPARSE_BUFFER memBlk;
}DNSPARSE_MCTX_COMMON, DNSPARSE_NAME_MCTX, DNSPARSE_ADDR4_MCTX;

typedef struct _DNSPARSE_DNS_MESSAGE_MCTX
{
    struct list_head  nameCtx;   /* DNSPARSE_NAME_MCTX */
    struct list_head  addr4Ctx; /* DNSPARSE_ADDR4_MCTX */
}DNSPARSE_DNS_MESSAGE_MCTX;

typedef enum _DNSPARSE_MCTXTYPE
{
    DNSPARSE_MCTXTYPE_NAME = 0,
    DNSPARSE_MCTXTYPE_ADDR4,
}DNSPARSE_MCTXTYPE;

typedef struct _DNSPARSE_RR_TYPE
{
    struct list_head listNode;

    DNSPARSE_NAME name;
    Ip_u16        type;
    Ip_u16        klass;    /* avoid use 'class', which is preserved by cpp*/

    /* Note: when is structure is used for Question Section, the following elements is non-sense,
     * and be careful about the pointer RData */
    Ip_u32  ttl;
    /* we don't need RDlength here, because contents in RData is uncompressed, either is a string
     * ended with '\0' or a ip4address, which can be distinguished by 'type' and 'klass' */
    /* Ip_u32  RDlength; */
    void   *pRData;
}DNSPARSE_RR_TYPE;


typedef struct _DNSPARSE_DNS_MESSAGE
{
    /* dns message header elements */
    Ip_u16 id;                                     /* A 16 bit identifier assigned by the program */
    Ip_u16 flags;                                  /* flags which contains QR, opcode, AA, TC ... */
    Ip_u16 counts[DNSPARSE_DNS_SECTION_MAX];       /* Four 16 bit counter */
    /* dns header end */

    /* dns message's 4 sections, i.e., 'Question', 'Answer', 'Authority' and 'Additional' sections,
     * these sections have a common format call RR(resource record) */
    struct list_head sections[DNSPARSE_DNS_SECTION_MAX];  /* DNSPARSE_RR_TYPE */
    /* dns message packet end */

    /* the following elements used to maintain resources used for storing pasing result,
     * 'mctx' here is short for 'memory context' */
    DNSPARSE_DNS_MESSAGE_MCTX mctx;
}DNSPARSE_DNS_MESSAGE;

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
extern mempool_t    *g_dnsparse_name_mempool;
extern mempool_t    *g_dnsparse_addr4_mempool;
extern mempool_t    *g_dnsparse_rr_mempool;
extern Ip_bool      g_dnsparse_mempool_inited;

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
IP_PUBLIC dnsparse_result_t dnsparse_mempool_init(void);
IP_PUBLIC dnsparse_result_t dnsparse_mempool_destroy(void);
IP_PUBLIC dnsparse_result_t dnsparse_message_init(DNSPARSE_DNS_MESSAGE *pMsg);
IP_PUBLIC dnsparse_result_t dnsparse_message_destroy(DNSPARSE_DNS_MESSAGE *pMsg);

IP_GLOBAL dnsparse_result_t dnsparse_new_nameCtx(DNSPARSE_DNS_MESSAGE *pMsg);
IP_GLOBAL dnsparse_result_t dnsparse_new_addr4Ctx(DNSPARSE_DNS_MESSAGE *pMsg);
IP_GLOBAL dnsparse_result_t dnsparse_new_rr(DNSPARSE_DNS_MESSAGE *pMsg, int sectionId);

#endif /* DNSPARSE_MESSAGE_H */
