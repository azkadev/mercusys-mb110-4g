/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_utils.c
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
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/ctype.h>
#include <linux/string.h>

#include "appidentify_utils.h"
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define UCHAR_MAX           (255)

#define APPIDNTFY_UTILS_ERROR(fmt, args...)   printk("[Error](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args)
#define APPIDNTFY_UTILS_DEBUG(fmt, args...)   /* printk("[Debug](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args)  */
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
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/
char *
strtok_r(char *s, const char *st, char **last)
{
    char *start;
    char *stop;

    start = s ? s : *last;
    while (*start != '\0' && strchr(st, *start) != NULL)
        start++;
    stop = start;
    while (*stop != '\0' && strchr(st, *stop) == NULL)
        stop++;
    if (*stop != '\0')
        *stop++ = '\0';
    *last = stop;

    return strlen(start) ? start : NULL;
}

/*!
 *\fn       int _string_makeSubStrByChar(tp_char *string,
 *                                          tp_char delimit,
 *                                          int     maxNum,
 *                                          tp_char *subStrArr[])
 *\brief    Splits a string into a substring array by specified character delimit. Callers needn't
 *          allocate memory for "subStrArr".
 *
 *\param[in]    string      The string to be splitted.
 *\param[in]    delimit     The character delimit.
 *\param[in]    maxNum      Maximum number of substring that callers can accept. It must be bigger
 *                          than actual number of substring by 1, because there will be a null
 *                          pointer as the ending sign of the array.
 *\param[out]   subStrArr   The substring array containing all splitted substrings. Size of the
 *                          array mustn't be smaller than maxNum.
 *
 *\return   the number of substring
 *
 *\note     It returns -1 if there is some error.
 */
int
string_makeSubStrByChar(char *string, char delimit, int maxNum, char *subStrArr[])
{
    char ws[8];
    char *pChar = NULL;
    char *pLast = NULL;
    int     cnt    = 0;

    if ((NULL == string) || (NULL == subStrArr))
    {
        APPIDNTFY_UTILS_ERROR("Some parameter is null.\r\n");
        return -1;
    }

    if (maxNum <= 0)
    {
        APPIDNTFY_UTILS_ERROR("Maximum number is invalid. maxNum = %d\r\n", maxNum);
        return -1;
    }

    memset(ws, 0, sizeof(ws));

    ws[0] = delimit;
    ws[1] = '\0';
    strncat(ws, "\t\r\n", strlen("\t\r\n")+1);

    for (pChar=strtok_r(string,ws,&pLast); pChar; pChar=strtok_r(NULL,ws,&pLast))
    {
        subStrArr[cnt++] = pChar;

        #if 1
        if (cnt >= maxNum)
        {
            APPIDNTFY_UTILS_ERROR("Too many substrings to string provided \r\n");

            return -1;
        }
        #endif
    }

    subStrArr[cnt] = NULL;

    return cnt;
}

int
string_charReplace(char *string, char dstChar, char srcChar)
{
    unsigned int strLen;
    int    i;

    if (NULL == string)
    {
        APPIDNTFY_UTILS_ERROR("Some parameter is null.\r\n");
        return -1;
    }

    strLen = strlen(string);

    for (i=0; i<strLen; i++)
    {
        if (srcChar == string[i])
        {
            string[i] = dstChar;
            i = i + 1;
            return i;
        }
    }

    return 0;
}

void* appidentify_mempool_alloc_limit(APPIDNTFY_MEMPOOL *app_mempool, gfp_t gfp_mask)
{
    void *memblk = NULL;

    if (NULL == app_mempool ||
        NULL == app_mempool->mempool)
    {
        return NULL;
    }

    if (app_mempool->used < app_mempool->maximum)
    {
        memblk = mempool_alloc(app_mempool->mempool, gfp_mask);
        if (memblk)
        {
            app_mempool->used ++;
        }
        return memblk;
    }
    else
    {
        /*printk(" mempool: %s full, can't alloc.\r\n", app_mempool->poolname);*/
        return NULL;
    }
}

void appidentify_mempool_free_limit(void *mem_entry, APPIDNTFY_MEMPOOL *app_mempool)
{
    if (NULL == app_mempool ||
        NULL == mem_entry)
    {
        return;
    }

    mempool_free(mem_entry, app_mempool->mempool);
    app_mempool->used --;
}

int appidentify_mempool_init_limit(char *pool_name, APPIDNTFY_MEMPOOL *app_mempool, size_t blksize,
                                   int min_reserved, unsigned int max_limited)
{
    if (NULL == app_mempool || NULL == pool_name)
    {
        return -1;
    }

    memset(app_mempool, 0, sizeof(APPIDNTFY_MEMPOOL));
    strncpy(app_mempool->poolname, pool_name, 31);
    app_mempool->mempool = mempool_create_kmalloc_pool(min_reserved, blksize);
    if (NULL == app_mempool->mempool)
    {
        return -1;
    }
    app_mempool->maximum = max_limited;

    return 0;
}

/*
 *===========================================================================
 *                    ipfirewall_http_bms_str_search
 *===========================================================================
 * Description: Search using the Boyer-Moore-Sunday (Quick Search) algorithm
 *              The Boyer-Moore-Sunday algorithm is a more efficient simplification of
 *              the Boyer-Moore algorithm. It performs comparisons between a Ip_s8acter
 *              in the pattern and a Ip_s8acter in the text buffer from left to right.
 *              After each mismatch it uses bad Ip_s8acter heuristic to shift the
 *              pattern to the right. For more details on the algorithm, refer to
 *               "A Very Fast Substring Search Algorithm", Daniel M. Sunday, Communications
 *              of the ACM, Vol. 33 No. 8, August 1990, pp. 132-142.
 *              It has a O(Pattern Length x Text Buffer Length) worst-case time complexity.
 *              But empirical results have shown that this algorithm is one of the
 *              fastest in practice.
 * Parameters:  pattern       - pattern to search for
 *              pattern_len    - length of the pattern
 *              buffer        - text buffer to search in
 *              buffer_len     - length of the text buffer
 *              case_sensitive - case-sensitive search
 * Returns:     A pointer to the located pattern, or a IP_NULL pointer if the
 *              pattern is not found
 */
char *
appidentify_bms_str_search(char   *pattern,
                               int     pattern_len,
                               char   *buffer,
                               int     buffer_len,
                               int case_sensitive)
{
    int skip_table[UCHAR_MAX + 1];
    int i;
    unsigned char b;
    char *p_pattern;
    char *p_buffer;
    char *p_buffer_compare;
    char *p_pattern_end = pattern + pattern_len;
    char *p_buffer_end = buffer + buffer_len - pattern_len + 1;

    /* Validation check */
    if ((pattern == NULL) || (pattern_len == 0) ||
        (buffer == NULL) || (buffer_len == 0) ||
        (pattern_len > buffer_len))
    {
        return NULL;
    }

    /* Setup the skip table for the pattern */
    for (i = 0; i < UCHAR_MAX + 1; i++)
        skip_table[(unsigned char)i] = pattern_len + 1;
    for (p_pattern = pattern; p_pattern < p_pattern_end; p_pattern++)
    {
        if (case_sensitive == TRUE)
            skip_table[(unsigned char)*p_pattern] = p_pattern_end - p_pattern;
        else
        {
            b = tolower((int)*p_pattern);
            skip_table[b] = p_pattern_end - p_pattern;
            b = toupper((int)*p_pattern);
            skip_table[b] = p_pattern_end - p_pattern;
        }
    }

    /* Main loop is the fast skip loop */
    for (p_buffer = buffer; p_buffer < p_buffer_end;
         p_buffer += skip_table[(unsigned char)*(p_buffer + pattern_len)])
    {
        /* Comparison loop */
        if (case_sensitive == TRUE)
        {
            for (p_pattern = pattern, p_buffer_compare = p_buffer;
                 (p_pattern < p_pattern_end) && (*p_pattern == *p_buffer_compare);
                 p_pattern++, p_buffer_compare++)
                     ;
        }
        else
        {
            for (p_pattern = pattern, p_buffer_compare = p_buffer;
                 (p_pattern < p_pattern_end) &&
                 (toupper((int)*p_pattern) == toupper((int)*p_buffer_compare));
                 p_pattern++, p_buffer_compare++)
                     ;
        }

        if (p_pattern == p_pattern_end)
            return p_buffer;  /* match */
     }

    /* no match */
    return NULL;
}

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/
