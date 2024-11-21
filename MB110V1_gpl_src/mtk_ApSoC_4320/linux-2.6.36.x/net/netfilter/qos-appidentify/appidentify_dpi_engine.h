/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_dpi_engine.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     29Sep13
 *
 *\warning
 *
 *\history \arg 0.0.1, 29Sep13, Yan Wei, Create the file.
 */
#ifndef __APPDIENTIFY_DPI_ENGINE_H__
#define __APPDIENTIFY_DPI_ENGINE_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include "appidentify.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define     APPIDNTF_PCRE_DEBUG(fmt, args...)   /*printk("[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args)*/                          \

typedef enum _APPIDNTFY_DPI_TYPE
{
    APPIDNTFY_DPI_TYPE_UNKOWN = -1,
    APPIDNTFY_DPI_TYPE_PCRE
}APPIDNTFY_DPI_TYPE;
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct _appidentify_dpi_engine
{
    void*   (*parse)(const char*);
    int     (*inspect)(struct sk_buff *, struct nf_conntrack_app *, void*, unsigned char *, unsigned int);
}APPIDNTFY_DPI_ENGINE;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/*!
 *\fn   void appidentify_pcre_free(void *pDpiCode)
 *\brief    Free the pcre data.
 */
void appidentify_pcre_free(void *pDpiCode);




#endif  /* __APPDIENTIFY_DPI_ENGINE_H__ */
