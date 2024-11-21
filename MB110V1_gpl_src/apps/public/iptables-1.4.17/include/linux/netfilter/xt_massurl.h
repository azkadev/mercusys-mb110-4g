/* 
 *
 * file		xt_massurl.h
 * brief		
 * details	
 *
 * author	wangwenhao
 * version	
 * date		24Oct11
 *
 * history \arg	1.0, 24Oct11, wangwenhao, create file
 */
#ifndef __XT_MASSURL_H__
#define __XT_MASSURL_H__

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define MASSURL_VERSION "1.0.0"

#define MASSURL_MAX_ENTRY 200

#define MASSURL_URL_LEN 33

#define MASSURL_INDEX_WWORDS ((MASSURL_MAX_ENTRY + 31) >> 5)

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
enum {
	MASSURL_TYPE_HTTP = 0,
	MASSURL_TYPE_URL = 1,
	MASSURL_TYPE_DNS = 2,
};

struct xt_massurl_info {
	unsigned int type;
	unsigned int urlIndexBits[MASSURL_INDEX_WWORDS];
};

struct massurl_url_info
{
	unsigned short index;
	char url[MASSURL_URL_LEN];
};

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif	/* __XT_MASSURL_H__ */
