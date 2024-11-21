/*!Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file        dnsparse_message.c
 *\brief
 *
 *\author      wangquzhi
 *\version
 *\date        2013/01/31
 *
 *\history     2013/01/31, wangquzhi create this file
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
#include <linux/gfp.h>
#include <linux/mempool.h>
#include <linux/list.h>

#include "dnsparse_config.h"
#include "dnsparse_error.h"
#include "dnsparse_message.h"


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
IP_STATIC dnsparse_result_t _dnsparse_section_destroy(struct list_head *pHead);
IP_STATIC dnsparse_result_t _dnsparse_mctx_destroy(DNSPARSE_DNS_MESSAGE_MCTX *pMctx);

IP_PUBLIC dnsparse_result_t dnsparse_mempool_init(void);
IP_PUBLIC dnsparse_result_t dnsparse_message_init(DNSPARSE_DNS_MESSAGE *pMsg);
IP_PUBLIC dnsparse_result_t dnsparse_message_destroy(DNSPARSE_DNS_MESSAGE *pMsg);

IP_GLOBAL dnsparse_result_t dnsparse_new_nameCtx(DNSPARSE_DNS_MESSAGE *pMsg);
IP_GLOBAL dnsparse_result_t dnsparse_new_addr4Ctx(DNSPARSE_DNS_MESSAGE *pMsg);
IP_GLOBAL dnsparse_result_t dnsparse_new_rr(DNSPARSE_DNS_MESSAGE *pMsg, int sectionId);

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
mempool_t   *g_dnsparse_name_mempool     = NULL;
mempool_t   *g_dnsparse_addr4_mempool    = NULL;
mempool_t   *g_dnsparse_rr_mempool       = NULL;
Ip_bool     g_dnsparse_mempool_inited   = IP_FALSE;

Ip_bool     l_dnsparse_disp_flag = IP_FALSE;
/**************************************************************************************************/
/*                                      LOCAL FUNCTIONS                                           */
/**************************************************************************************************/

/*!
 *\fn            IP_STATIC void
 *               _dnsparse_section_destroy(Ipcom_list *pHead)
 *\brief         free section list of a DNSPARSE_DNS_MESSAGE
 *\details
 *
 *\param[io]     Ipcom_list *pHead
 *
 *\return            void
 *
 *\note
 */
IP_STATIC dnsparse_result_t
_dnsparse_section_destroy(struct list_head *pHead)
{
    struct list_head *pTempListNode = NULL;

    DNSPARSE_ASSERT(pHead != NULL, DNSPARSE_INVALID_ARG);
    while (pHead != (pTempListNode = pHead->prev))
    {
        list_del(pTempListNode);
        mempool_free(pTempListNode, g_dnsparse_rr_mempool);
    }

    return DNSPARSE_SUCCESS;
}

/*!
 *\fn            IP_STATIC dnsparse_result_t
 *               _dnsparse_mctx_destroy(DNSPARSE_DNS_MESSAGE_MCTX *pMctx)
 *\brief         free mctx of a DNSPARSE_DNS_MESSAGE
 *\details       free two list: nameCtx list and addr4Ctx list
 *
 *\param[io]     DNSPARSE_DNS_MESSAGE_MCTX *pMctx
 *
 *\return            void
 *
 *\note
 */
IP_STATIC dnsparse_result_t
_dnsparse_mctx_destroy(DNSPARSE_DNS_MESSAGE_MCTX *pMctx)
{
    struct list_head *pHead = NULL;
    struct list_head *pTempListNode = NULL;

    DNSPARSE_ASSERT(pMctx != NULL, DNSPARSE_INVALID_ARG);

    /* destory nameCtx list */
    pHead = &(pMctx->nameCtx);
    while (!list_empty_careful(pHead))
    {
        pTempListNode = pHead->prev;
        list_del(pTempListNode);
        mempool_free(pTempListNode, g_dnsparse_name_mempool);
    }

    /* destory addr4Ctx list */
    pHead = &(pMctx->addr4Ctx);
    while (!list_empty_careful(pHead))
    {
        pTempListNode = pHead->prev;
        list_del(pTempListNode);
        mempool_free(pTempListNode, g_dnsparse_addr4_mempool);
    }

    return DNSPARSE_SUCCESS;
}

/**************************************************************************************************/
/*                                      PUBLIC FUNCTIONS                                          */
/**************************************************************************************************/
/*!
 *\fn            IP_PUBLIC dnsparse_result_t
 *               dnsparse_mempool_init()
 *\brief         create three mempools, which will be used while parsing.
 *\details       g_dnsparse_name_mempool, g_dnsparse_addr4_mempool, g_dnsparse_rr_mempool
 *
 *\param[io]     DNSPARSE_DNS_MESSAGE *pMsg
 *
 *\return            void
 *
 *\note          this function must succeed, if not, it will suspend the task who invoke it.
 *               this function only failed when no memory is avaliable.
 */
IP_PUBLIC dnsparse_result_t
dnsparse_mempool_init()
{
    DNSPARSE_REQURE(g_dnsparse_mempool_inited == IP_FALSE, DNSPARSE_MEMPOOL_REINIT);

    g_dnsparse_name_mempool = mempool_create_kmalloc_pool(DNSPARSE_NAME_MEMPOOL_SIZE,
                                                          DNSPARSE_NAME_MEMPOOL_BLKSIZE);
    DNSPARSE_REQURE(NULL != g_dnsparse_name_mempool, DNSPARSE_NOMEMORY);

    g_dnsparse_addr4_mempool = mempool_create_kmalloc_pool(DNSPARSE_ADDR4_MEMPOOL_SIZE,
                                                          DNSPARSE_ADDR4_MEMPOOL_BLKSIZE);
    DNSPARSE_REQURE(NULL != g_dnsparse_addr4_mempool, DNSPARSE_NOMEMORY);

    g_dnsparse_rr_mempool = mempool_create_kmalloc_pool(DNSPARSE_RR_MEMPOOL_SIZE,
                                                          DNSPARSE_RR_MEMPOOL_BLKSIZE);
    DNSPARSE_REQURE(NULL != g_dnsparse_rr_mempool, DNSPARSE_NOMEMORY);

    g_dnsparse_mempool_inited = IP_TRUE;
    return DNSPARSE_SUCCESS;
}

IP_PUBLIC dnsparse_result_t
dnsparse_mempool_destroy()
{
    if (l_dnsparse_disp_flag)
    {
        return DNSPARSE_ERRORMSG_END;
    }

    g_dnsparse_mempool_inited = IP_FALSE;

    if (g_dnsparse_name_mempool)
    {
        mempool_destroy(g_dnsparse_name_mempool);
    }

    if (g_dnsparse_addr4_mempool)
    {
        mempool_destroy(g_dnsparse_addr4_mempool);
    }

    if (g_dnsparse_rr_mempool)
    {
        mempool_destroy(g_dnsparse_rr_mempool);
    }

    return DNSPARSE_SUCCESS;
}

IP_PUBLIC dnsparse_result_t
dnsparse_message_init(DNSPARSE_DNS_MESSAGE *pMsg)
{
    int idx = 0;

    DNSPARSE_REQURE(pMsg != NULL, DNSPARSE_INVALID_ARG);

    l_dnsparse_disp_flag = IP_TRUE;

    memset(pMsg, 0, sizeof(DNSPARSE_DNS_MESSAGE));

    /* init sections list nodes */
    for (idx = 0; idx < DNSPARSE_DNS_SECTION_MAX; idx++)
    {
        INIT_LIST_HEAD(&(pMsg->sections[idx]));
    }

    /* init mctx list nodes */
    INIT_LIST_HEAD(&(pMsg->mctx.nameCtx));
    INIT_LIST_HEAD(&(pMsg->mctx.addr4Ctx));

    return DNSPARSE_SUCCESS;
}



/*!
 *\fn            IP_PUBLIC void
 *               dnsparse_message_destroy(DNSPARSE_DNS_MESSAGE *pMsg)
 *\brief         free DNSPARSE_DNS_MESSAGE and all memories it alloced.
 *\details
 *
 *\param[io]     DNSPARSE_DNS_MESSAGE *pMsg
 *
 *\return            void
 *
 *\note          pMsg itself is not destoryed. Because we don't know how the memory block
 *               pMsg pointed to is alloced.
 */
IP_PUBLIC dnsparse_result_t
dnsparse_message_destroy(DNSPARSE_DNS_MESSAGE *pMsg)
{
    int idx = 0;

    DNSPARSE_REQURE(pMsg != NULL, DNSPARSE_INVALID_ARG);

    /* free sections first */
    for (idx = 0; idx < DNSPARSE_DNS_SECTION_MAX; idx++)
    {
        _dnsparse_section_destroy(&(pMsg->sections[idx]));
    }

    /* free mctx */
    _dnsparse_mctx_destroy(&(pMsg->mctx));

    l_dnsparse_disp_flag = IP_FALSE;

    return DNSPARSE_SUCCESS;
}
/**************************************************************************************************/
/*                                      GLOBAL FUNCTIONS                                          */
/**************************************************************************************************/

/*!
 *\fn            IP_GLOBAL dnsparse_result_t
 *               dnsparse_new_nameCtx(DNSPARSE_DNS_MESSAGE *pMsg)
 *\brief         free DNSPARSE_DNS_MESSAGE and all memories it alloced.
 *\details
 *
 *\param[io]     DNSPARSE_DNS_MESSAGE *pMsg
 *
 *\return            DNSPARSE_SUCCESS     everything is OK
 *               DNSPARSE_NOMEMORY    memory in pool runs out
 *
 *\note
 */
IP_GLOBAL dnsparse_result_t
dnsparse_new_nameCtx(DNSPARSE_DNS_MESSAGE *pMsg)
{
    DNSPARSE_NAME_MCTX *new_nameCtx = NULL;

    DNSPARSE_ASSERT(NULL != pMsg, DNSPARSE_INVALID_ARG);

    DNSPARSE_DEBUG("tracing.");
    new_nameCtx = (DNSPARSE_NAME_MCTX *)mempool_alloc(g_dnsparse_name_mempool, GFP_ATOMIC);
    DNSPARSE_DEBUG("tracing.");
    if (NULL == new_nameCtx)
    {
        DNSPARSE_DEBUG("tracing.");
        return DNSPARSE_NOMEMORY;
    }
    DNSPARSE_DEBUG("tracing.");
    /* clean and init the new alloced nameCtx */
    memset(new_nameCtx, 0, DNSPARSE_NAME_MEMPOOL_BLKSIZE);
    DNSPARSE_DEBUG("tracing.");
    new_nameCtx->memBlk.size = DNSPARSE_NAME_BUFFER_SIZE;
    DNSPARSE_DEBUG("tracing.");
    new_nameCtx->memBlk.base = new_nameCtx->memBlk.buffer;
    DNSPARSE_DEBUG("tracing.");
    list_add_tail(&(new_nameCtx->listNode), &(pMsg->mctx.nameCtx));
    DNSPARSE_DEBUG("tracing.");

    return DNSPARSE_SUCCESS;
}

/*!
 *\fn            IP_GLOBAL dnsparse_result_t
 *               dnsparse_new_addr4Ctx(DNSPARSE_DNS_MESSAGE *pMsg)
 *\brief         free DNSPARSE_DNS_MESSAGE and all memories it alloced.
 *\details
 *
 *\param[io]     DNSPARSE_DNS_MESSAGE *pMsg
 *
 *\return            DNSPARSE_SUCCESS     everything is OK
 *               DNSPARSE_NOMEMORY    memory in pool runs out
 *
 *\note
 */
IP_GLOBAL dnsparse_result_t
dnsparse_new_addr4Ctx(DNSPARSE_DNS_MESSAGE *pMsg)
{
    DNSPARSE_NAME_MCTX *new_addr4Ctx = NULL;

    DNSPARSE_ASSERT(NULL != pMsg, DNSPARSE_INVALID_ARG);

    new_addr4Ctx = (DNSPARSE_ADDR4_MCTX *)mempool_alloc(g_dnsparse_addr4_mempool, GFP_ATOMIC);
    if (NULL == new_addr4Ctx)
    {
        return DNSPARSE_NOMEMORY;
    }

    /* clean and init the new alloced addr4Ctx */
    memset(new_addr4Ctx, 0, DNSPARSE_ADDR4_MEMPOOL_BLKSIZE);
    new_addr4Ctx->memBlk.size = DNSPARSE_ADDR4_BUFFER_SIZE;
    new_addr4Ctx->memBlk.base = new_addr4Ctx->memBlk.buffer;
    list_add_tail(&(new_addr4Ctx->listNode), &(pMsg->mctx.addr4Ctx));

    return DNSPARSE_SUCCESS;
}


/*!
 *\fn            IP_GLOBAL dnsparse_result_t
 *               dnsparse_new_rr(DNSPARSE_DNS_MESSAGE *pMsg)
 *\brief         free DNSPARSE_DNS_MESSAGE and all memories it alloced.
 *\details
 *
 *\param[io]     DNSPARSE_DNS_MESSAGE *pMsg
 *
 *\return            DNSPARSE_SUCCESS     everything is OK
 *               DNSPARSE_NOMEMORY    memory in pool runs out
 *
 *\note
 */
IP_GLOBAL dnsparse_result_t
dnsparse_new_rr(DNSPARSE_DNS_MESSAGE *pMsg, int sectionId)
{
    DNSPARSE_RR_TYPE *new_rr = NULL;

    DNSPARSE_ASSERT(NULL != pMsg, DNSPARSE_INVALID_ARG);
    DNSPARSE_ASSERT((sectionId >= 0 && sectionId < DNSPARSE_DNS_SECTION_MAX), DNSPARSE_INVALID_ARG);

    new_rr = (DNSPARSE_RR_TYPE *)mempool_alloc(g_dnsparse_rr_mempool, GFP_ATOMIC);
    if (NULL == new_rr)
    {
        return DNSPARSE_NOMEMORY;
    }

    /* clean and init the new alloced rr */
    memset(new_rr, 0, DNSPARSE_RR_MEMPOOL_BLKSIZE);
    list_add_tail(&(new_rr->listNode), &(pMsg->sections[sectionId]));

    return DNSPARSE_SUCCESS;
}
