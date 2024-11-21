/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
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

#define MASSURL_INDEX_WWRODS ((MASSURL_MAX_ENTRY + 31) >> 5)

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
	unsigned int urlIndexBits[MASSURL_INDEX_WWRODS];
};

struct massurl_url_info
{
	unsigned short index;
	char url[MASSURL_URL_LEN];
};

//[zhangguosong start] 2019-05-10
enum contentRestrictionType
{
	CONTENTRESTRICTION_DISABLED = 0,		//Content Restriction is disabled
	CONTENTRESTRICTION_BLACKLIST = 1,		//Content Restriction is enabled and it is BlackList mode
	CONTENTRESTRICTION_WHITELIST = 2,		//Content Restriction is enabled and it is whilteList mode
	CONTENTRESTRICTION_MAX
};
//[zhangguosong end]

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
