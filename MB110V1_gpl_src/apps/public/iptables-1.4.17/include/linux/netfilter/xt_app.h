/*  Copyright(c) 2009-2015 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		xt_app.h
 * brief	for iptables match
 *
 * author	Wang Lian
 * version	1.0.0
 * date		23Oct15
 *
 * history 	\arg 1.0.0, 23Oct15, Wang Lian, Create the file.
 */
 
#ifndef _XT_APP_H
#define _XT_APP_H

#include <linux/types.h>

#define XT_APPID_MAX 4096

/* match info */
struct xt_app_info {
	__u16 id;	
};

#endif /*_XT_APP_H*/
