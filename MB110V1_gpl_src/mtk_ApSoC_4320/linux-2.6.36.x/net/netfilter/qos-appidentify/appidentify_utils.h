/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_utils.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     04Dec13
 *
 *\warning
 *
 *\history \arg 0.0.1, 04Dec13, Yan Wei, Create the file.
 */
#ifndef __APPIDENTIFY_UTILS_H__
#define __APPIDENTIFY_UTILS_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/mempool.h>

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define     TRUE        (1)
#define     FALSE       (0)
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct _APPIDNTFY_MEMPOOL
{
    char            poolname[32];
    mempool_t       *mempool;
    unsigned int    used;
    unsigned int    maximum;
}APPIDNTFY_MEMPOOL;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
void* appidentify_mempool_alloc_limit(APPIDNTFY_MEMPOOL *app_mempool, gfp_t gfp_mask);
void appidentify_mempool_free_limit(void *mem_entry, APPIDNTFY_MEMPOOL *app_mempool);
int appidentify_mempool_init_limit(char *pool_name, APPIDNTFY_MEMPOOL *app_mempool, size_t blksize,
                                   int min_reserved, unsigned int max_limited);
char *
appidentify_bms_str_search(char   *pattern,
                               int     pattern_len,
                               char   *buffer,
                               int     buffer_len,
                               int case_sensitive);

int
string_makeSubStrByChar(char *string, char delimit, int maxNum, char *subStrArr[]);

int
string_charReplace(char *string, char dstChar, char srcChar);



#endif  /* __APPIDENTIFY_UTILS_H__ */
