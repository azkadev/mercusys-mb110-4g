/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		strFile.h
 * brief		
 * details	
 *
 * author	LI CHENGLONG
 * version	
 * date		02Nov11
 *
 * history 	\arg  1.0.0, 02Nov11, LI CHENGLONG, Create the file.	
 */

#ifndef __STRFILE_H__
#define __STRFILE_H__

/**************************************************************************************************/
/*                                          INCLUDES                                              */
/**************************************************************************************************/
#include "globals.h"
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/*xml中用到的一些常量定义, Added by LI CHENGLONG , 2011-Nov-02.*/
#define STRING_URL "http://255.255.255.255:65535"
#define STRING_10 "          "
#define STRING_32 STRING_10 STRING_10 STRING_10 "  "
#define STRING_64 STRING_32 STRING_32
#define PREFIX_presentationURL "<presentationURL>"
#define PREFIX_friendlyName "<friendlyName>"
#define PREFIX_manufacturer "<manufacturer>"
#define PREFIX_manufacturerURL "<manufacturerURL>"
#define PREFIX_modelDescription "<modelDescription>"
#define PREFIX_modelName "<modelName>"
#define PREFIX_modelNumber "<modelNumber>"

#define UPNP_WEB_PATH "/"
#define UPNP_XML_SUFFIX ".xml"
#define UPNP_CTL_URL   "upnp/control/"
#define UPNP_EVENT_URL UPNP_CTL_URL
#define L3F_NAME "dummy"
#define WCIFC_NAME "WANCommonIFC1"
#define WIPC_NAME  "WANIPConn1"

#define L3F_SCPD_NAME L3F_NAME
#define WCIFC_SCPD_NAME "gateicfgSCPD"
#define WIPC_SCPD_NAME "gateconnSCPD"

#define L3F_CTL_URL    UPNP_WEB_PATH UPNP_CTL_URL "%s/" L3F_NAME
#define L3F_EVENT_URL  UPNP_WEB_PATH UPNP_CTL_URL "%s/" L3F_NAME
#define L3F_SCPD_FILE_NAME  UPNP_WEB_PATH L3F_SCPD_NAME UPNP_XML_SUFFIX
#define WCIFC_CTL_URL  UPNP_WEB_PATH UPNP_CTL_URL "%s/" WCIFC_NAME
#define WCIFC_EVENT_URL  UPNP_WEB_PATH UPNP_CTL_URL "%s/" WCIFC_NAME
#define WCIFC_SCPD_FILE_NAME UPNP_WEB_PATH WCIFC_SCPD_NAME UPNP_XML_SUFFIX
#define WIPC_CTL_URL   UPNP_WEB_PATH UPNP_CTL_URL "%s/" WIPC_NAME
#define WIPC_EVENT_URL   UPNP_WEB_PATH UPNP_CTL_URL "%s/" WIPC_NAME
#define WIPC_SCPD_FILE_NAME UPNP_WEB_PATH WIPC_SCPD_NAME UPNP_XML_SUFFIX
/* Ended by LI CHENGLONG , 2011-Nov-02.*/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn		UBOOL32 initConfigFile()
 * brief	初始化upnpd的配置文件.	
 * details	生成配置文件保存到配置文件目录下.
 *
 * return		UBOOL32
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  08Nov11, LI CHENGLONG	
 */
UBOOL32 initConfigFile();


/* 
 * fn		UBOOL32 upnpDescInit(char *devManufacturerUrl,
 *								 char *devManufacturer,
 *								 char *devModelName,
 *								 char *devModelVer,
 *								 char *devName,
 *								 char *description,
 *								 UINT32 webPort,
 *								 UINT32 ip
 *			);
 * brief	根据设备信息更新设备描述文件gatedesc.xml	
 * details	根据设备信息更新设备描述文件gatedesc.xml	
 *
 * param[in]	devManufacturerUrl			upnp设备厂商url
 * param[in]	devManufacturer				upnp设备厂商名
 * param[in]	devModelName				upnp设备model名
 * param[in]	devModelVer					upnp设备model版本
 * param[in]	devName						upnp设备名
 * param[in]   description					upnp设备描述
 * param[in]	webPort						webport default 80
 * param[in]	ip							lan端ip default br0's ip
 *
 * return		UBOOL32
 * retval		TRUE	更新成功
 *				FALSE	更新失败
 *
 * note	written by  02Nov11, LI CHENGLONG	
 */
UBOOL32 upnpDescInit(char *devManufacturerUrl,
					 char *devManufacturer,
					 char *devModelName,
					 char *devModelVer,
					 char *devName,
					 char *description,
					 UINT32 webPort,
					 UINT32 ip
);

#endif	/* __STRFILE_H__ */
