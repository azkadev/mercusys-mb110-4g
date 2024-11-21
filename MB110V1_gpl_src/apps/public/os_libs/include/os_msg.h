/*  Copyright(c) 2010-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file	os_msg.h
 * brief	The msg lib declarations. 
 *
 * author	Yang Xv
 * version	1.0.0
 * date	28Apr11
 *
 * history 	\arg 1.0.0, 28Apr11, Yang Xv, Create the file.
 */

#ifndef __OS_MSG_H__
#define __OS_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */




/* 注意这个头文件不要引用<cstd.h>头文件 */
#include <unistd.h>

#include <dm_paramLen.h>

/* do not include <netinet/in.h> */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* 
 * brief Message content size	
 */
#define MSG_CONTENT_SIZE	512


/* 
 * brief Message size	
 */
#define MSG_SIZE			520


/* for all message type */

#define SNTP_DM_SERVER_NUM	5

/* 
 * brief status of DHCP6C_INFO_MSG_BODY
 */
#ifdef INCLUDE_IPV6
//CMSG_IPV6_DHCP6C_STATUS
#define DHCP6C_ASSIGNED_DNS 	0x1
#define DHCP6C_ASSIGNED_ADDR 	0x2
#define DHCP6C_ASSIGNED_PREFIX 	0x4
#define DHCP6C_ASSIGNED_DSLITE_ADDR		0x8  /* Add by YuanRui: support DS-Lite, 21Mar12 */

//CMSG_IPV6_DHCP6S_STATUS
#define DHCP6S_ASSIGNED_PREFIX	0x01		/* add by chenming 2017-6-12 */
#define DHCP6S_RELEASE_PREFIX	0x02
#endif	/* INCLUDE_IPV6 */

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/


#ifdef __LINUX_OS_FC__
#include <sys/un.h>

typedef struct
{
	int fd;
	struct sockaddr_un _localAddr;
	struct sockaddr_un _remoteAddr;
}CMSG_FD;
#endif	/* __LINUX_OS_FC__ */

#ifdef __VXWORKS_OS_FC__
typedef struct
{
	int fd;
}CMSG_FD;
#endif /* __VXWORKS_OS_FC__ */


/* 
 * brief 	Enumeration message type
 * 			Convention:
 *			System message types - 1000 ~ 1999
 *			Common application message types - 2000 ~ 2999
 */
typedef enum
/* 如果一个消息只有一个UINT32的数据，那么可以省略该message type对应的结构体，
 * 只使用CMSG_BUFF结构中的priv成员即可
 */
{
	CMSG_NULL = 0,
	CMSG_REPLY = 1,
	CMSG_CONNECTSTATUS_CHECK = 2,
	CMSG_EXECSYSTEM = 1000,
	CMSG_LOG = 2000,
	CMSG_SNTP_CFG = 2001,
	CMSG_SNTP_STATUS = 2002,	/* only have one word value */
	CMSG_SNTP_START = 2003,

	CMSG_DNS_PROXY_CFG = 2004,
	CMSG_DNS_SERVER	= 2005,		/* only have one word value */
	CMSG_PPP_STATUS = 2006,

 	/* Added by Yang Caiyong, 11-Aug-29.
 	 * For auto PVC.
 	 */ 
	CMSG_AUTO_PVC_SEARCHED = 2007,
 	/* Ended by Yang Caiyong, 11-Aug-29. */
	
	/* Added by whb , 2013-03-27. */
	CMSG_IP_STATUS = 2008,
	/* Ended by whb. */
	
	/* Added by xcl, 2011-06-13.*/
#ifdef INCLUDE_SMART_DHCP
	CMSG_DHCPC_DISCOVER = 2009,
#endif /* INCLUDE_SMART_DHCP */
	CMSG_DHCPS_RELOAD_CFG = 2010, 
	CMSG_DHCPS_WRITE_LEASE_TO_SHM = 2011,
	CMSG_DHCPS_WAN_STATUS_CHANGE = 2012,
	CMSG_DHCPC_STATUS = 2013,
	CMSG_DHCPC_START = 2014,
	CMSG_DHCPC_RENEW = 2015, 
	CMSG_DHCPC_RELEASE = 2016,
	CMSG_DHCPC_SHUTDOWN = 2017,
	CMSG_DHCPC_MAC_CLONE = 2018,
	/* End added by xcl, 2011-06-13.*/

	CMSG_DHCPS_REPLY = 2019, /* liuliming */

	CMSG_DDNS_PH_RT_CHANGED = 2020,
	CMSG_DDNS_PH_CFG_CHANGED = 2021,
	CMSG_DDNS_PH_GET_RT	= 2022,
	/* Added by xcl, for dyndns, 24Nov11 */
	CMSG_DYNDNS_RT_CHANGED = 2023,
	CMSG_DYNDNS_CFG_CHANGED = 2024,
	CMSG_DYNDNS_STATE_CHANGED = 2025,
	/* Added by tpj, for noipdns, 17Jan12 */
	CMSG_NOIPDNS_RT_CHANGED = 2026,
	CMSG_NOIPDNS_CFG_CHANGED = 2027,
	CMSG_NOIPDNS_STATE_CHANGED = 2028,
	/*end (n & n)*/

	CMSG_HTTPD_SERVICE_CFG = 2030,
	CMSG_HTTPD_USER_CFG = 2031,
	CMSG_HTTPD_HOST_CFG = 2032,
	CMSG_HTTPD_CHALLENGE_CFG = 2033,
#ifdef INCLUDE_HTTP_SSL
	CMSG_HTTPD_CERT_UPDATE = 2034,
#endif /*INCLUDE_HTTP_SSL*/

	CMSG_HTTPD_LOGOUT_ALL_USERS		= 2035,
#ifdef INCLUDE_CLOUD_ACCOUNT
	CMSG_TPLINKDNS_CFG_CHANGED = 2036,
#endif /*INCLUDE_CLOUD*/

	CMSG_CLI_USRCFG_UPDATE	= 2040,

	/* Added by zj, for userdefined ddns, 28Jan14 */
	CMSG_DDNS_UD_RT_CHANGED = 2041,
	CMSG_DDNS_UD_CFG_CHANGED = 2042,
	CMSG_DDNS_UD_STATE_CHANGED = 2043,

	CMSG_HTTPD_CERTIFICATE_CHANGE = 2044,
#ifdef INCLUDE_PORTABLE_APP
	CMSG_PORTABLE_APP_USRCFG_UPDATE = 2045,
	CMSG_PORTABLE_APP_WLANCFG_UPDATE = 2046,
#ifdef INCLUDE_CLOUD
	CMSG_PORTABLE_APP_LOGOUT_ALL_USERS = 2047,
	CMSG_PORTABLE_APP_OWNERINFO_UPDATE = 2048,
#endif /*INCLUDE_CLOUD*/
#endif /* INCLUDE_PORTABLE_APP */	
	
 	/* Added  by  Li Chenglong , 11-Jul-31.*/
	CMSG_UPNP_ENABLE = 2050,
	CMSG_DEFAULT_GW_CH = 2051,
 	/* Ended  by  Li Chenglong , 11-Jul-31.*/

	/* Add by chz, 2012-12-24 */
	CMSG_UPNP_DEL_ENTRY = 2052,
	/* end add */
	/* Add by Chen Zexian, 20130116 */
	CMSG_UPNP_LAN_IP_CH = 2053,
	/* End of add */

	/* added by Yuan Shang for diagTool, 2011-08-18 */
	CMSG_DIAG_TOOL_COMMAND = 2060,

	/* added by wuzhiqin, 2011-09-26 */
	CMSG_CWMP_CFG_UPDATE = 2070, 
	CMSG_CWMP_PARAM_VAL_CHANGED = 2071,
	CMSG_CWMP_WAN_CONNECTION_UP = 2072,
	CMSG_CWMP_TIMER = 2073,
	CMSG_CWMP_WAN_CONNECTION_DOWN = 2074,
	/* ended by wuzhiqin, 2011-09-26 */

#ifdef INCLUDE_TR143 /* Added by GuoChubin 2015-05. */
	CMSG_DIAG_COMPLETE = 2080, /* Generic */
	CMSG_DIAG_PARAM    = 2081,
	CMSG_TR143_START = 2082,
#endif /* INCLUDE_TR143 */

#ifdef INCLUDE_TR143_V1_1_0
	CMSG_DIAG_LOAD_MULTI = 2083,
#endif
	/* added by Yuan Shang for usb storage hotplug event, 2011-12-09 */
	/* delete by zjj ,2013.09 */
	/* CMSG_USB_HOTPLUG_EVENT = 2080, */

	/* added by Wang Wenhao for IGMPv3 Proxy, 2011-11-24 */
	CMSG_IGMPD_ADD_LAN_IF = 2100,
	CMSG_IGMPD_ADD_WAN_IF = 2101,
	CMSG_IGMPD_DEL_IF = 2102,

	/* Added by LI CHENGLONG , 2011-Dec-15.*/
	CMSG_DLNA_MEDIA_SERVER_INIT = 2110,
	CMSG_DLNA_MEDIA_SERVER_MANUAL_SCAN = 2111,
	CMSG_DLNA_MEDIA_SERVER_OP_FOLDER = 2112,
	CMSG_DLNA_MEDIA_SERVER_RELOAD = 2113,
	CMSG_DLNA_MEDIA_DEVICE_STATE_CHANGE = 2114,
	/* Ended by LI CHENGLONG , 2011-Dec-15.*/

	/*added by LY to handle printer hotplug msg, in 2014.07.03*/
	CMSG_USB_PRINTER_HANDLE_EVENT = 2200,
	/*end added by LY*/
	
#ifdef INCLUDE_LAN_WLAN
	CMSG_WPS_CFG = 2700,
	CMSG_WLAN_SWITCH = 2701,
	CMSG_WPS_PIN_SWITCH = 2702,
#endif /* INCLUDE_LAN_WLAN */

//#ifdef INCLUDE_RMNET_QMI
	CMSG_QMI_DISCONNECT = 2703,
	CMSG_QMI_RECONNECT = 2704,
//#endif
	CMSG_RESET_BUTTON_PRESSED = 2705,

	CMSG_WPS_SUCCESS_24G = 2706,
	CMSG_WPS_SUCCESS_5G  = 2707,
	CMSG_WLAN_JOIN_5G  = 2708,
	CMSG_WLAN_LEAVE_5G = 2709,
	CMSG_WLAN_JOIN_2G  = 2710,
	CMSG_WLAN_LEAVE_2G = 2711,
	/* 
	 * for voice process. added by zhonglianbo 2011-8-10
	 */
#ifdef INCLUDE_VOIP
	CMSG_VOIP_CMCNXCOUNT 			= 2800,
	CMSG_VOIP_WAN_STS_CHANGED		= 2801,			/* wan口状态改变.
												 * 消息的priv字段值为 RSL_VOIP_INTF_STSCODE
												 * 消息的content为当前的端口ip地址
												 * Added by zhonglianbo, 2011-11-24 
												 */
	CMSG_VOIP_CONFIG_CHANGED 		= 2802,
	CMSG_VOIP_RESTART_CALLMGR 		= 2803,
	CMSG_VOIP_CONFIG_WRITTEN 		= 2804,			/* 非语音进程不能写flash,以此消息通知
												 * 语音进程写flash
												 */
	CMSG_VOIP_FLASH_WRITTEN 		= 2805,			/* 语音进程可以写flash,以此消息通知语
												 * 音进程已经写了flash
												 */
	CMSG_VOIP_STATISTICS_RESET 		= 2806, 
#ifdef INCLUDE_USB_VOICEMAIL	
	CMSG_VOIP_UVM_USING_RECORDED_FILE	= 2807,		/* 获取USB线程是否正在使用recorded
												 * file.	Added by zhonglianbo, 2011-9-19.
												 */
	CMSG_VOIP_USB_MOUNT_NEW = 2808,				/* A new disk that mounted can be used by usbvm  */
	CMSG_VOIP_USB_UMOUNT_CHANGE = 2809,			/* Change to another disk's path that is effective*/
	CMSG_VOIP_USB_UMOUNT_NULL = 2810,			/* There is not any disk can be used by usbvm  */
#endif /* INCLUDE_USB_VOICEMAIL */												 

	CMSG_VOIP_CALLLOG_CLEAR = 2811,				/* Clear call log */

#ifdef INCLUDE_VOICEAPP
	CMSG_VOICEAPP_BASESTATION_UPDATE = 2820,
	CMSG_VOICEAPP_SESSIONCFG_UPDATE = 2821,
	CMSG_VOICEAPP_PHONENAME_UPDATE = 2822,
#endif /* INCLUDE_VOICEAPP */
#endif /* INCLUDE_VOIP */
	/* end of voice process */

    /* Added by xcl, 21Sep11 */
    CMSG_SNMP_CFG_CHANGED 	= 2850,
    CMSG_SNMP_LINK_UP       = 2851,
    CMSG_SNMP_LINK_DOWN     = 2852,
    CMSG_SNMP_WAN_UP		= 2853,
    /* End added by xcl, 21Sep11 */

#ifdef INCLUDE_IPV6	/* Add by CM, 16Nov11 */
	CMSG_IPV6_PPP_STATUS	= 2900,
	CMSG_IPV6_DHCP6C_STATUS	= 2901,
	CMSG_IPV6_STATUS = 2902,

#ifdef INCLUDE_IPV6_MLD	/* Add by HYY: MLDv2 Proxy, 10Jul13 */
	CMSG_MLDPROXY_ADD_LAN_IF	= 2903,
	CMSG_MLDPROXY_ADD_WAN_IF	= 2904,
	CMSG_MLDPROXY_DEL_IF		= 2905,
#endif /* INCLUDE_IPV6_MLD */

	CMSG_IPV6_DHCP6S_STATUS	= 2906,

	//[zhangguosong start] 2018-10-11
	CMSG_DNS6_PROBE_CFG = 2950,
	CMSG_DNS6_SERVER = 2951,
	//[zhangguosong end]

#endif	/* INCLUDE_IPV6 */

#ifdef INCLUDE_IPSEC
	CMSG_IPSEC_CFG_CHANGED = 3000,
	CMSG_IPSEC_WAN_CHANGED = 3001,
#endif

/*Code transplanting by ljn from Wang Yang for DHCP Option66 2017.8.15*/
#ifdef INCLUDE_OPTION66
	CMSG_OPTION66 = 3004,
#endif

#ifdef INCLUDE_OPENVPN_SERVER /* added by CCy, 27Jul2015 */
	CMSG_OVPN_STATUS = 3005,
#endif /*INCLUDE_OPENVPN_SERVER*/

/* Add by zjj, 20120703, for usb 3g handle card event */
#ifdef INCLUDE_USB_3G_DONGLE
	CMSG_USB_3G_HANDLE_EVENT = 3100,
	CMSG_USB_3G_BACKUP		 = 3101,
#endif /* INCLUDE_USB_3G_DONGLE */

#if defined(INCLUDE_3G4G_BACKUP) || defined(INCLUDE_USB_3G_DONGLE)
	CMSG_CHECK_LINK_ALIVE	 = 3102,
#endif

#ifdef INCLUDE_3G4G_BACKUP
    CMSG_3G4G_BACKUP         = 3103,
#endif

#ifdef INCLUDE_EWAN_BACKUP
	CMSG_EWAN_BACKUP	 	 = 3150,
#endif  /* INCLUDE_EWAN_BACKUP */

/* Add by zjj, 20130726, for samba notification when lan ip changed. */
#ifdef INCLUDE_USB_SAMBA_SERVER
	CMSG_USB_SMB_LAN_IP_CHANGED = 3200,
#endif /* INCLUDE_USB_SAMBA_SERVER */

#ifdef INCLUDE_VDSLWAN
	CMSG_DSL_TYPE_CHANGED = 3300,
#endif /* INCLUDE_VDSLWAN */


/*<< BosaZhong@20Sep2012, add, for SMP system. */
#ifdef SOCKET_LOCK 
	CMSG_SOCKET_LOCK_P                  = 4000,
	CMSG_SOCKET_LOCK_V                  = 4001,
	CMSG_SOCKET_LOCK_PROBE_DEAD_PROCESS = 4002,
#endif /* SOCKET_LOCK */
/*>> endof BosaZhong@20Sep2012, add, for SMP system. */

#ifdef INCLUDE_WAN_DETECT
	CMSG_WAN_DETECT_RESULT = 4101,
#endif

#ifdef INCLUDE_DECT
	CMSG_DECT_ALLOW_REGISTER		= 4200,
	CMSG_DECT_BASE_CFG_CHANGE		= 4201,
	CMSG_DECT_HANDSET_PAGING		= 4202,
	CMSG_DECT_HANDSET_UNREGISTER	= 4203,
	CMSG_DECT_HANDSET_DATETIME_SYNC	= 4204,	
	CMSG_DECT_HANDSET_STATUS_CHANGE = 4205,		
	CMSG_DECT_HANDSET_NAME_CHANGE	= 4206,
/* Add by wang haobin, 20140124, for dect cli */
	CMSG_DECT_CLI_DIAG_MODE_SET		= 4207,
	CMSG_DECT_CLI_MODEM_RESET 		= 4208,
	CMSG_DECT_CLI_SET_BMC_REQ 		= 4209,
	CMSG_DECT_CLI_SET_OSC_REQ 		= 4210,
	CMSG_DECT_CLI_SET_TBR6_REQ	 	= 4211,
	CMSG_DECT_CLI_SET_RFPI_REQ 		= 4212,
	CMSG_DECT_CLI_SET_XRAM_REQ 		= 4213,
	CMSG_DECT_CLI_SET_GFSK_REQ 		= 4214,
	CMSG_DECT_CLI_SET_RFMODE_REQ 	= 4215,
	CMSG_DECT_CLI_SET_FREQ_REQ 		= 4216,
	CMSG_DECT_CLI_SET_TPC_REQ 		= 4217,
	CMSG_DECT_CLI_GET_BMC_REQ 		= 4218,
	CMSG_DECT_CLI_GET_XRAM_REQ 		= 4219,
	CMSG_DECT_CLI_GET_TPC_REQ 		= 4220,
	CMSG_DECT_CLI_GET_BMC_REP		= 4221,
	CMSG_DECT_CLI_GET_XRAM_REP 		= 4222,
	CMSG_DECT_CLI_GET_TPC_REP 		= 4223,
	CMSG_DECT_CLI_PROCESS_RESULT	= 4224,
/* end add */
	CMSG_DECT_HANDSET_TEST_START 	= 4225,
	CMSG_DECT_HANDSET_TEST_STOP		= 4226,
	CMSG_VOIP_USBMAIL_UNREAD_COUNT 	= 4227,
	CMSG_DECT_LINE_SETTINGS_CHANGE	= 4228,
	CMSG_DECT_CONTACT_CHANGE		= 4229,

#endif /* INCLUDE_DECT */

#ifdef INCLUDE_CLOUD /* Added by zjj, 20150922, for cloud service message. */
	CMSG_CLOUD_UPGRADE_FIRMWARE		= 4300,
	CMSG_CLOUD_CHECK_FW_UPDATE		= 4301,
	/*notify cloud_client to reconnect cloud server when default gateway changed.*/
	CMSG_CLOUD_NOTIFY_RECONNECT 	= 4302,
	/*get device status*/
	CMSG_CLOUD_GET_DEV_STATUS		= 4303,
	CMSG_CLOUD_NOTIFY_BIND 	= 4304,

	/* bind is not used by now, it will be called when first login */
	CMSG_CLOUD_ACCOUNT_BIND			= 4310,
	CMSG_CLOUD_ACCOUNT_UNBIND		= 4311,
	CMSG_CLOUD_ACCOUNT_LOGIN		= 4312,
	/* for cloud ddns service. */
	CMSG_CLOUD_DDNS_REGISTER		= 4320,
	CMSG_CLOUD_DDNS_BIND			= 4321,
	CMSG_CLOUD_DDNS_UNBIND			= 4322,
	CMSG_CLOUD_DDNS_UNBIND_ALL		= 4323,
	CMSG_CLOUD_DDNS_GET_LIST		= 4324,
	CMSG_CLOUD_DDNS_DELETE			= 4325,
	/*for device management*/
	CMSG_CLOUD_DEVMGMT_GET_TOKEN	= 4330,
	CMSG_CLOUD_DEVMGMT_ADD_DEV_USER = 4331,
	CMSG_CLOUD_DEVMGMT_RM_DEV_USER   = 4332,
	CMSG_CLOUD_DEVMGMT_GET_DEV_USERINFO	= 4333,
	CMSG_CLOUD_DEVMGMT_PASSTHROUGH	= 4334,
	/* for cloud https interface */
	CMSG_CLOUD_HTTPS_DEVMGMT_GET_TOKEN	= 4340,
	CMSG_CLOUD_HTTPS_ACCOUNT_CHECK_BIND	= 4341,
	CMSG_CLOUD_HTTPS_ACCOUNT_BIND		= 4342,
	CMSG_CLOUD_HTTPS_ACCOUNT_UNBIND		= 4343,
	CMSG_CLOUD_HTTPS_DEVMGMT_RM_DEV_USER	= 4344,
	CMSG_CLOUD_HTTPS_CHECK_CONNECT		= 4345,
#endif /* INCLUDE_CLOUD */

#ifdef INCLUDE_WAN_BLOCK
	CMSG_WAN_BLOCK_STOP_BLOCK = 4400,
#endif /*INCLUDE_WAN_BLOCK*/

	CMSG_MOBILE_LTE_DISCONNECTED = 4500,
	CMSG_LTE_FW_TRY_RECOVERY = 4501,

	CMSG_ONEMESH_LIST_DEVICES = 4600,
	CMSG_ONEMESH_LIST_AVAILABLE_DEVICES = 4601,
	CMSG_ONEMESH_CLEAN_DEVICE = 4602,
	CMSG_ONEMESH_PROBE = 4603,
	CMSG_ONEMESH_ADD_SLAVE = 4604,
	CMSG_ONEMESH_ATTACH_MASTER = 4605,
	CMSG_ONEMESH_AUTO_ATTACH_MASTER = 4606,
	CMSG_ONEMESH_SYNC_WIFI = 4607,

#ifdef INCLUDE_SPEEDTEST
	CMSG_SPEEDTEST_SET_STATUS = 9400,
#endif /* INCLUDE_SPEEDTEST */


}CMSG_TYPE;


/* 
 * brief	Message struct
 */
typedef struct
{
	CMSG_TYPE type;		/* specifies what message this is */
	unsigned int priv;		/* private data, one word of user data etc. */
	unsigned char content[MSG_CONTENT_SIZE];
}CMSG_BUFF;


/* 
 * brief	Message type identification	
 */
typedef enum
{
	CMSG_ID_NULL = 5,	/* start from 5 */
	CMSG_ID_COS = 6,
	CMSG_ID_SNTP = 7,
	CMSG_ID_HTTP = 8,
	CMSG_ID_DNS_PROXY = 9,
	CMSG_ID_DHCPS = 10, 	/* Added by xcl, 2011-06-13.*/
	CMSG_ID_DDNS_PH = 11,  	/* addde by tyz, 2011-07-21 */
	CMSG_ID_PH_RT = 12,
	CMSG_ID_CLI = 13,
	CMSG_ID_DHCPC = 14, 
	CMSG_ID_UPNP =15, 	/* Added  by  Li Chenglong , 11-Jul-31.*/
	CMSG_ID_DIAGTOOL =16, /*Added by Yuan Shang, 2011-08-18 */
	CMSG_ID_CWMP = 17, /* add by wuzhiqin, 2011-09-26 */
	CMSG_ID_SNMP = 18, /* Added by xcl, 21Sep11 */
	CMSG_ID_IGMP = 19,	/* Added by Wang Wenhao, 2011-11-18 */

#ifdef INCLUDE_VOIP
	CMSG_ID_VOIP = 20,  /* for voice process, added by zhonglianbo 2011-8-10 */
#endif /* INCLUDE_VOIP */
	CMSG_ID_DYNDNS = 21, /* Added by xcl, 24Nov11 */
	
	/* Added by LI CHENGLONG , 2011-Dec-15.*/
	CMSG_ID_DLNA_MEDIA_SERVER = 22,
	/* Ended by LI CHENGLONG , 2011-Dec-15.*/

	CMSG_ID_NOIPDNS = 23, /*added by tpj, 2012-2-1*/
	
	CMSG_ID_IPSEC = 24,

	CMSG_ID_LOG = 25,	/* added by yangxv, 2012.12.25, log message no longer shared cos_passive */

#ifdef SOCKET_LOCK
	CMSG_ID_SOCKET_LOCK = 26, /* BosaZhong@20Sep2012, add, for SMP system. */
	CMSG_ID_SOCKET_LOCK_ACCEPT = 27, /* BosaZhong@21Sep2012, add, for SMP system. */
#endif /* SOCKET_LOCK */

#ifdef INCLUDE_IPV6_MLD	/* Add by HYY: MLDv2 Proxy, 01Jul13 */
	CMSG_ID_MLD	= 28,
#endif /* INCLUDE_IPV6_MLD */

#ifdef INCLUDE_DECT
	CMSG_ID_VOIP_SERVER = 29,
	CMSG_ID_VOIP_DECTCLI = 30,
#endif /* INCLUDE_DECT */

#ifdef INCLUDE_PORTABLE_APP
	CMSG_ID_PORTABLE_APP = 31,
#endif /* INCLUDE_PORTABLE_APP */

#ifdef INCLUDE_TR143 /* Added by Huang Zhida, 2014-01-15 */
	CMSG_ID_TR143 = 32,
#endif /* INCLUDE_TR143 */

	CMSG_ID_DDNS_UD = 33, /* added by zj, for userdefine ddns, 28May14 */

#ifdef INCLUDE_CLOUD /* Added by zjj, 20150922, for cloud service message. */
	CMSG_ID_CLOUD_CLIENT = 34,
#endif /* INCLUDE_CLOUD */

#ifdef INCLUDE_WAN_BLOCK /*Added by frl, 20151024, for action from wan block web.*/
	CMSG_ID_WAN_BLOCK = 35,
#endif /*INCLUDE_WAN_BLOCK*/

#ifdef INCLUDE_MAIL
	CMSG_ID_MAIL = 36,
#endif /* INCLUDE_MAIL */
#ifdef INCLUDE_CLOUD_ACCOUNT /* Added by pudongfang, 20180905, for cloud service message. */
	CMSG_ID_CLOUD_HTTPS_CLIENT = 37,
#endif /* INCLUDE_CLOUD_HTTPS */

	CMSG_ID_TDPD = 38,

	CMSG_ID_SYSTEM = 39,

	CMSG_ID_MAX,	
}CMSG_ID;


/* for all message type 
 * 注意不要使用UINT8等这种自定义的数据类型
 */

/* 
 * brief	CMSG_SNTP_CFG message type content
 */
#ifdef INCLUDE_IPV6
typedef struct
{
	unsigned char in6Addr[16];
}IN6_ADDR;
#endif

typedef struct
{
	char   	ntpServers[SNTP_DM_SERVER_NUM][TIME_NTPSERVER1_L];
	unsigned int primaryDns;
	unsigned int secondaryDns;
#ifdef INCLUDE_IPV6
	IN6_ADDR primaryDnsv6;
	IN6_ADDR secondaryDnsv6;
#endif
	unsigned int timeZone;
}SNTP_CFG_MSG;

/* Added by LI CHENGLONG , 2011-Dec-15.*/

/* 
 * brief: Added by LI CHENGLONG, 2011-Nov-21.
 *		 厂商相关的信息，在DLNA_MEDIA_SERVER进程启动时通过INIT消息发送给DLNA_MEDIA_SERVER进程，
*        DLNA_MEDIA_SERVER进程在发送ssdp 通告时将通告特定厂商的信息.
 */
typedef struct _MANUFACT_SPEC_INFO
{
	char 	devManufacturerURL[64];
	char 	manufacturer[64];
	char 	modelName[64];
	char 	devModelVersion[16];
	char 	description[256];
}MANUFACT_SPEC_INFO;


/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-15.
 *		  描述一个共享目录的结构.
 */
typedef struct _DMS_FOLDER_INFO
{
	char	dispName[DLNA_MEDIA_SERVER_FOLDER_ALIAS_L];
	char 	path[DLNA_MEDIA_SERVER_FOLDER_NAME_L];
	char	uuid[DLNA_MEDIA_SERVER_FOLDER_VOLUMEUUID_L];
	int enable;	/*added by LY to record whether this item is enabled, in 20141203*/
}DMS_FOLDER_INFO;


/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-16.
 *		  对目录的操作类型.
 */
typedef enum _DMS_FOLDER_OP
{
	DMS_INIT_FOLDER = 0,
	DMS_DEL_FOLDER = 1,
	DMS_ADD_FOLDER = 2
}DMS_FOLDER_OP;

/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-15.
 *		  启动DLNA_MEDIA_SERVER进程后立即发送,将初始化配置信息发送给DLNA_MEDIA_SERVER进程.
 */
typedef struct _DMS_INIT_INFO_MSG
{
	unsigned char		scanFlag;				/*scan*/
	unsigned char		serverState;			/* ServerState */
	unsigned int		folderCnt;			/*how many folde is shared now*/
	int			shareAll;			/*indicate whether share all the volumes, added by LY in 2014.09.05 */
	unsigned int		scanInterval;		/*scan interval*/
	MANUFACT_SPEC_INFO	manuInfo;				/*oem等不同厂商的信息*/
	char			serverName[DLNA_MEDIA_SERVER_SERVERNAME_L];

}DMS_INIT_INFO_MSG;

/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-15.
 *		  上层UI更新了DLNA_MEDIA_MEDIA_SERVER配置后直接将整个配置传给DLNA_MEDIA_SERVER进程,
 *		  不再对各个操作进行分类.
 */
typedef struct _DMS_RELOAD_MSG
{
	unsigned char		serverState;			/* ServerState */
	char				serverName[DLNA_MEDIA_SERVER_SERVERNAME_L];
	unsigned char		scanFlag;				/*scan*/
	unsigned int		scanInterval;		/*scan interval*/	
	int			shareAll;				/*added by LY to indicate whether to share all the volumes*/						
}DMS_RELOAD_MSG;

/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-16.
 *		  操作目录的消息.
 */
typedef struct _DMS_OP_FOLDER_MSG
{
	 DMS_FOLDER_OP			op;
	 DMS_FOLDER_INFO		folder;
}DMS_OP_FOLDER_MSG;

/* Ended by LI CHENGLONG , 2011-Dec-15.*/


/* 
 * brief	CMSG_ID_CLI message type content
 */
typedef struct _CLI_USR_CFG_MSG
{
	char   		rootName[USER_CFG_ROOTNAME_L];	/* RootName */
	char   		rootPwd[USER_CFG_ROOTPWD_L];	/* RootPwd */
	char   		adminName[USER_CFG_ADMINNAME_L];	/* AdminName */
	char   		adminPwd[USER_CFG_ADMINPWD_L];	/* AdminPwd */
	char   		userName[USER_CFG_USERNAME_L];	/* UserName */
	char   		userPwd[USER_CFG_USERPWD_L];	/* UserPwd */
	char		manufact[IGD_DEV_INFO_MANUFACTURER_L];/* Added by Li Chenglong , 2011-Oct-12.*/
}CLI_USR_CFG_MSG;


/* 
 * brief: Added by Li Chenglong, 11-Jul-31.
 *		  UPnP enable message
 */
typedef struct _UPNP_ENABLE_MSG
{
	unsigned int enable; 
}UPNP_ENABLE_MSG;

/* Add by chz, 2012-12-24 */
typedef struct _UPNP_DEL_MSG
{
	unsigned int port;
	char protocol[16];
}UPNP_DEL_MSG;
/* end add */


/* 
 * brief: Added by Li Chenglong, 11-Jul-31.
 *		  默认网关状态改变的消息。
 */
typedef struct _UPNP_DEFAULT_GW_CH_MSG
{
	char gwName[WAN_IP_CONN_X_TP_IFNAME_L];
	char gwAddr[WAN_IP_CONN_EXTERNALIPADDRESS_L];
	unsigned char natEnabled;
	unsigned char upDown;
}UPNP_DEFAULT_GW_CH_MSG;


/* 
 * brief CMSG_DNS_PROXY_CFG	message type content
 */
typedef struct
{
	unsigned int primaryDns;
	unsigned int secondaryDns;
}DNS_PROXY_CFG_MSG;


/* Added by Zeng Yi. 2011-07-08 */
typedef struct
{
	unsigned char isError;
	char iface[LAN_WLAN_NAME_L];
	char SSID[LAN_WLAN_SSID_L];
	int authMode;
	int encryMode;
	char key[LAN_WLAN_X_TP_PRESHAREDKEY_L];
}WPS_CFG_MSG;
/* End added. Zeng Yi. */

/* Added by Yang Caiyong for PPP connection status changed, 2011-07-18 */
typedef struct _PPP_CFG_MSG
{
	unsigned int pppDevUnit;
	char connectionStatus[18];
	unsigned int pppLocalIp;
	unsigned int pppSvrIp;
	unsigned int uptime;
	char lastConnectionError[32];
	unsigned int dnsSvrs[2];
}PPP_CFG_MSG;
/* YCY adds end */

/* Added by whb, 2013-03-27. */
typedef struct _IP_CFG_MSG
{
	char connectionStatus[18];
	char connName[32];
	
}IP_CFG_MSG;

/* Added by xcl, 2011-07-25 */
typedef struct 
{
    unsigned int delLanIp;
    unsigned int delLanMask;
}DHCPS_RELOAD_MSG_BODY;

#ifdef INCLUDE_IPV6
/* Add by HYY: support dynamic 6RD, 20Mar12 */
typedef struct
{
	unsigned char ipv4MaskLen;
	unsigned char sit6rdPrefixLen;
	IN6_ADDR sit6rdPrefix;
	unsigned int sit6rdBRIPv4Addr;
}DHCPC_6RD_INFO;
#endif /* INCLUDE_IPV6 */

#ifdef INCLUDE_IPV6
typedef struct
{
	IN6_ADDR primaryDnsv6;
	IN6_ADDR secondaryDnsv6;
}DNS6_PROBE_CFG_MSG;
#endif

/* Add by chz for L2TP/PPTP, 2013-04-24 */
#undef INCLUDE_L2TP_OR_PPTP

#ifdef INCLUDE_L2TP
#define INCLUDE_L2TP_OR_PPTP 1
#endif

#ifdef INCLUDE_PPTP
#define INCLUDE_L2TP_OR_PPTP 1
#endif

#if defined(INCLUDE_L2TP) || defined(INCLUDE_PPTP)
typedef enum
{
	DHCP_IP = 0,
	DHCP_L2TP  = 1,
	DHCP_PPTP = 2
}IP_CONN_DHCP_TYPE;
#endif
/* end add */

typedef struct 
{
    unsigned char status; /* Have we been assigned an IP address ? */
    char ifName[WAN_IP_CONN_X_TP_IFNAME_L];  
    unsigned int ip;
    unsigned int mask;
    unsigned int gateway;
    unsigned int dns[2];
#ifdef INCLUDE_IPV6	/* Add by HYY: support dynamic 6RD, 20Mar12 */
	DHCPC_6RD_INFO sit6rdInfo;
#endif /* INCLUDE_IPV6 */
/* Add by chz for L2TP/PPTP, 2013-04-24 */
#if defined(INCLUDE_L2TP) || defined(INCLUDE_PPTP)
	IP_CONN_DHCP_TYPE connType;
#endif
/* end add */
#ifdef INCLUDE_OPTION66 /*Code transplanting by ljn from Wang Yang for DHCP Option66 2017.8.15*/
	char tftpIP[16];
#endif
}DHCPC_INFO_MSG_BODY;

typedef struct 
{
    unsigned char unicast;
    char ifName[WAN_IP_CONN_X_TP_IFNAME_L];
    char hostName[WAN_IP_CONN_X_TP_HOSTNAME_L];
#ifdef INCLUDE_IPV6	/* Add by HYY: support dynamic 6RD, 19Mar12 */
	unsigned char sit6rdEnabled;
#endif /* INCLUDE_IPV6 */
/* Add by chz for L2TP/PPTP, 2013-04-24 */
#if defined(INCLUDE_L2TP) || defined(INCLUDE_PPTP)
	IP_CONN_DHCP_TYPE connType;
#endif
/* end add */
/* yanglx, 2015-8-7, port from 9980 */
#ifdef INCLUDE_MER
	unsigned int merEnabled;
    char merString[65];
#endif
/* end, yanglx */
#ifdef INCLUDE_AP_MODE
	int multiMode;
#endif
#ifdef INCLUDE_OPTION66_LAN
	unsigned char initHookOpt66Flag;
#endif /* INCLUDE_OPTION66_LAN */
}DHCPC_CFG_MSG_BODY;
/* End added by xcl, 2011-07-25 */

/* Added by tyz 2011-08-02 (n & n) */
/* the msg of the interface */
typedef struct
{
	int ifUp;
	unsigned int ip;
	unsigned int gateway;
	unsigned int mask;
	unsigned int dns[2];
	char ifName[16];
}DDNS_RT_CHAGED_MSG;

/*
the msg of the ph running time
*/
typedef struct 
{
	unsigned char state;
	unsigned char sevType;
	unsigned short isEnd;
}DDNS_RT_PRIV_MSG;
/*
the msg of the cfg 
*/
typedef struct
{	
	int enabled;
	int reg;
	int userLen;
	char phUserName[PH_DDNS_CFG_PHUSERNAME_L];
	int pwdLen;
	char phPwd[PH_DDNS_CFG_PHPASSWD_L];	
}DDNS_PH_CFG_MSG;

/* Added by xcl, 24Nov11 */
/* dynDns config msg struct */
typedef struct 
{
    unsigned char   enable;
    char            userName[DYN_DNS_CFG_USERNAME_L];
    char            password[DYN_DNS_CFG_PASSWORD_L];
    char            domain[DYN_DNS_CFG_USERDOMAIN_L];
    char            server[DYN_DNS_CFG_SERVER_L];
    unsigned char   login;
}DYN_DNS_CFG_MSG;

typedef struct 
{
    unsigned int state;
}DYN_DNS_STATE_MSG;
/* End added by xcl */

/* Added by tpj, 17Jan12 */
/* noipDns config msg struct */
typedef struct 
{
    unsigned char   enable;
    char            userName[NOIP_DNS_CFG_USERNAME_L];
    char            password[NOIP_DNS_CFG_PASSWORD_L];
    char            domain[NOIP_DNS_CFG_USERDOMAIN_L];
    char            server[NOIP_DNS_CFG_SERVER_L];
    unsigned char   login;
}NOIP_DNS_CFG_MSG;

typedef struct 
{
    unsigned int state;
}NOIP_DNS_STATE_MSG;
/* End added by tpj */

/* Add by ZJ, 28May14 */
typedef struct 
{
	unsigned char   enable;
	unsigned char   login;
	unsigned short	IPStartOffset;
	unsigned short	IPEndOffset;
	char		grabServer[USERDEFINE_DDNS_CFG_GRABSERVER_L];
	char		grabAuth[USERDEFINE_DDNS_CFG_GRABAUTH_L];
	char            grabRequest[USERDEFINE_DDNS_CFG_GRABREQUEST_L];
	char            grabDomain[USERDEFINE_DDNS_CFG_GRABDOMAIN_L];
}DDNS_UD_CFG_MSG;
/* End add */

typedef struct
{
    unsigned int state;
}DDNS_UD_STATE_MSG;
/* end added by hx, 2015.04.13 */
#ifdef INCLUDE_CLOUD
typedef struct
{
	unsigned char   enable;
	signed int status;
	char domain[CLOUD_DDNS_BOUNDDOMAIN_L];
}TPLINKDNS_CFG_MSG;
#endif
typedef struct
{
	unsigned int command;
	char host[16];
#ifdef INCLUDE_IPV6
     char host6[80];
#endif  /*INCLUDE_IPV6*/
	unsigned int result;
}DIAG_COMMAND_MSG;

/* end (n & n) */
typedef struct
{
	unsigned char vpi;
	unsigned short vci;
	char connName[WAN_IP_CONN_NAME_L];
}AUTO_PVC_MSG;

/* Added by xcl, 17Oct11, snmp msg struct */
typedef struct 
{
    unsigned short ifIndex;
}SNMP_LINK_STAUS_CHANGED_MSG;
/* End added */

#ifdef INCLUDE_IPV6	/* Add by HYY: IPv6 support, 16Nov11 */
typedef struct _PPP6_CFG_MSG
{
	unsigned int pppDevUnit;
	unsigned char pppIPv6CPUp;
	unsigned long long remoteID;
	unsigned long long localID;
}PPP6_CFG_MSG;

typedef struct 
{
	IN6_ADDR addr;
	unsigned int pltime;
	unsigned int vltime;
	unsigned char plen;
}DHCP6C_IP_INFO;

typedef struct 
{
	unsigned char status;
	char ifName[WAN_PPP_CONN_X_TP_IFNAME_L];  
	DHCP6C_IP_INFO ip;
	DHCP6C_IP_INFO prefix;
	IN6_ADDR dns[2];
	IN6_ADDR dsliteAddr;   /* Add by YuanRui: support DS-Lite, 21Mar12 */
}DHCP6C_INFO_MSG_BODY;
#endif /* INCLUDE_IPV6 */

#ifdef INCLUDE_IPSEC
typedef struct
{
	unsigned short currDepth;								
	unsigned short numInstance[6];	
}IPSEC_OLD_NUM_STACK;

typedef struct 
{
	int state;
    char default_gw_ip[16];
}IPSEC_WAN_STATE_CHANGED_MSG;

typedef struct
{
	char local_ip[16];
	char local_mask[16];
	unsigned int  local_ip_mode;
	char remote_ip[16];
	char remote_mask[16];
	unsigned int  remote_ip_mode;
	char real_remote_gw_ip[16];
	char spi[16];
	char second_spi[16];
	unsigned int entryID;
	unsigned int  op;
	unsigned char  enable;
	unsigned int key_ex_type; /*Added for vxWorks*/
	IPSEC_OLD_NUM_STACK stack;
}IPSEC_CFG_CHANGED_MSG;
#endif

#ifdef INCLUDE_VDSLWAN
typedef enum
{
	CMSG_DSL_SWITCH_TO_ADSL = 0,
	CMSG_DSL_SWITCH_TO_VDSL = 1,
}CMSG_DSL_TYPE;

typedef struct 
{
	CMSG_DSL_TYPE dslType;
}DSL_TYPE_SWITCH_MSG;
#endif /* INCLUDE_VDSLWAN */

/*added by LY for printer hotplug event msg*/
typedef enum _PRINTER_ACTION
{
	USB_PRINTER_ADD = 0,
	USB_PRINTER_REMOVE,
}PRINTER_ACTION;

typedef struct 
{
	PRINTER_ACTION printerAction;
	char printerName[XTP_PRINT_SERVICE_SERVERNAME_L];
}USB_PRINTER_HOTPLUG_MSG;
/*end added by LY*/

#ifdef INCLUDE_DECT
typedef struct 
{
	unsigned char name[XTP_DECT_BASE_STATION_NAME_L];
    unsigned char enabled;
	char pin[XTP_DECT_BASE_STATION_PIN_L];
	unsigned char greenModeEnabled;
	unsigned char ECOModeEnabled;
	unsigned char SecurityModeEnabled;
	unsigned char clockMaster;
}DECT_BASE_CFG;

typedef struct 
{
	unsigned int endpt;
	char name[VOICE_PROF_LINE_X_TP_PHONENAME_L];
	char interNum[VOICE_PROF_LINE_X_TP_INTERNALNUMBER_L];
	unsigned char widebandEnabled;
	unsigned char status;	/* Registered  status */
	char IPUI[XTP_DECT_HANDSET_INFO_IPUI_L];	/* IPUI */
	char TPUI[XTP_DECT_HANDSET_INFO_TPUI_L];	/* TPUI */
	char authKey[XTP_DECT_HANDSET_INFO_AUTHKEY_L];	/* AuthKey */
	char cipherKey[XTP_DECT_HANDSET_INFO_CIPHERKEY_L];	/* CipherKey */
	unsigned char serviceClass;	/* ServiceClass */
	unsigned char modelId;	/* ModelId */
	unsigned int termCap;	/* TermCap */	
}DECT_HANDSET_INFO;
#endif /* INCLUDE_DECT */

#ifdef INCLUDE_VOICEAPP
typedef struct 
{
	unsigned char name[XTP_VOICEAPP_BASE_STATION_NAME_L];
    unsigned char enabled;
	char pin[XTP_VOICEAPP_BASE_STATION_PIN_L];
}VOICEAPP_BASE_CFG;

typedef struct 
{
	unsigned int endpt;
	unsigned char status;	/* Registered  status */
	unsigned int identification; 
}VOICEAPP_ENDPT_INFO;

typedef struct 
{
	unsigned int endpt;
	char name[VOICE_PROF_LINE_X_TP_PHONENAME_L];
}VOICEAPP_PHONENAME_INFO;

#endif /* INCLUDE_VOICEAPP */

#ifdef INCLUDE_USB_VOICEMAIL

typedef enum
{
	USBVM_SC_NEW = 0,			/* new voice mail, notify dect and voiceApp */
	USBVM_SC_LISTEN = 1, 		/* voice mail listened, notify dect and voiceApp */
	USBVM_SC_DEL_UNREAD = 2,	/* del unread voice mail, notify dect and voiceApp */
	USBVM_SC_DEL_READ = 3,		/* del read voice mail, only notify voiceApp */
} USBVM_STATUC_CHANGE_TYPE;

typedef struct
{
	USBVM_STATUC_CHANGE_TYPE type;
	int unreadCount;
} USBVM_RECORD_STATUS_CHANGE_MSG;

#endif	/* #ifdef INCLUDE_USB_VOICEMAIL */

#ifdef INCLUDE_TR143
typedef struct
{
	int  oid;
	int  moreFragFlag;
	char frag[1]; /* [PARAM_MSG_FRAG_SIZE] */
} DIAG_PARAM_MSG;
#define PARAM_MSG_FRAG_SIZE (MSG_CONTENT_SIZE - ((size_t)&((DIAG_PARAM_MSG *)0)->frag))
#endif /* INCLUDE_TR143 */

#ifdef INCLUDE_OPENVPN_SERVER /* added by CCy for OpenVpn, 27Jul215 */
typedef enum
{
	OVPN_KEY_NOT_GENERATED		= 0,
	OVPN_KEY_GENERATED			= 1,
	OVPN_KEY_GENERATING			= 2,
}OVPN_KEY_GEN_STATUS;

typedef struct _OVPN_STATUS_CHANGED_MSG
{
	OVPN_KEY_GEN_STATUS			keyStatus;
}OVPN_STATUS_CHANGED_MSG;
#endif /*INCLUDE_OPENVPN_SERVER*/

#ifdef INCLUDE_CLOUD_ACCOUNT
typedef struct _CLOUD_ACCOUNT_USER_INFO
{
	char account[OWNER_INFO_EMAIL_L];
} CLOUD_ACCOUNT_USER_INFO;

typedef struct _CLOUD_HTTPS_UNBIND_MSG
{
	char userAccountId[OWNER_INFO_EMAIL_L];
	char accountManagerToken[CURRENT_USER_LOGINTOKEN_L];
} CLOUD_HTTPS_UNBIND_MSG;

typedef struct _CLOUD_UNBIND_MSG
{
	char userName[65];
	char deviceId[41];
} CLOUD_UNBIND_MSG;
#endif /*INCLUDE_CLOUD_ACCOUNT*/

#ifdef INCLUDE_MAIL
/* 
 * brief	mail related configuration types
 */
typedef enum
{
	MAIL_RELATED_CONFIG_LANGUAGE = 1
} MAIL_RELATED_CONFIG_TYPE;

/* 
 * brief	消息体中搭载要修改的配置
 */
typedef struct __MAIL_RELATED_CONFIG__
{
	MAIL_RELATED_CONFIG_TYPE type;
	char data[500];
} MAIL_RELATED_CONFIG;
#endif /* INCLUDE_MAIL */


#if defined(INCLUDE_USB_3G_BACKUP) || defined(INCLUDE_3G4G_BACKUP)
/*
 * brief	传递给cos检查线路是否alive的结构体
 */
typedef struct _CHECK_LINK_ALIVE_CONGIG
{
	char 		type[16];
	unsigned short	oid;
	int 		optCode;
	char 		StaticRouteData[256];
	char   		name[32];	/* Name */
	char   		defaultGateway[16];	/* DefaultGateway */
	char   		X_TP_IfName[16];	/* X_TP_IfName */
	char   		DNSServers[64];	/* DNSServers */

} CHECK_LINK_ALIVE_CONGIG;
#endif //defined(INCLUDE_USB_3G_BACKUP) || defined(INCLUDE_3G4G_BACKUP)

typedef enum
{
	SYNC_ALL = 0,
	SYNC_ONE = 1,
	SYNC_WITH_REPLY = 2,
} SYNC_WIFI_TYPE;

typedef struct _ONEMESH_PROBE_MSG
{
	int count;
	int timeout;
} ONEMESH_PROBE_MSG;

typedef struct _ONEMESH_ADD_SLAVE_MSG
{
	char slaveMac[18];
	int timeout;
} ONEMESH_ADD_SLAVE_MSG;

typedef struct _ONEMESH_ATTACH_MASTER_MSG
{
	char masterMac[18];
	int wantJoin;
	int timeout;
	int autoEnabled;
} ONEMESH_ATTACH_MASTER_MSG;

typedef struct _ONEMESH_SYNC_WIFI_MSG
{
	char mac[18];
	char ip[16];
	SYNC_WIFI_TYPE type;
} ONEMESH_SYNC_WIFI_MSG;

#ifdef INCLUDE_SPEEDTEST
typedef struct _SPEEDTEST_STATUS_MSG_BODY
{
	unsigned int download;
	unsigned int upload;
	unsigned int date;
	unsigned int step; //-1=unknown, 0=stop, 1=uploading, 2=downloading, 3=end
}SPEEDTEST_STATUS_MSG_BODY;
#endif /* INCLUDE_SPEEDTEST */

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

/* 
 * fn		int msg_init(CMSG_FD *pMsgFd)
 * brief	Create an endpoint for msg
 *	
 * param[out]	pMsgFd - return msg descriptor that has been create	
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 *
 * note 	Need call msg_cleanup() when you no longer use this msg which is created by msg_init()
 */
int msg_init(CMSG_FD *pMsgFd);


/* 
 * fn		int msg_srvInit(CMSG_ID msgId, CMSG_FD *pMsgFd)
 * brief	Init an endpoint as a server and bind a name to this endpoint msg	
 *
 * param[in]	msgId - server name	
 * param[in]	pMsgFd - server endpoint msg fd
 *
 * return	-1 is returned if an error occurs, otherwise is 0	
 */
int msg_srvInit(CMSG_ID msgId, CMSG_FD *pMsgFd);



/* 
 * fn		int msg_connSrv(CMSG_ID msgId, CMSG_FD *pMsgFd)
 * brief	Init an endpoint as a client and specify a server name	
 *
 * param[in]		msgId - server name that we want to connect	
 * param[in/out]	pMsgFd - client endpoint msg fd	
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 */
int msg_connSrv(CMSG_ID msgId, CMSG_FD *pMsgFd);


/* 
 * fn		int msg_recv(const CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
 * brief	Receive a message form a msg	
 *
 * param[in]	pMsgFd - msg fd that we want to receive message
 * param[out]	pMsgBuff - return recived message
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 *
 * note		we will clear msg buffer before recv
 */
int msg_recv(const CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff);


/* 
 * fn		int msg_send(const CMSG_FD *pMsgFd, const CMSG_BUFF *pMsgBuff)
 * brief	Send a message from a msg	
 *
 * param[in]	pMsgFd - msg fd that we want to send message	
 * param[in]	pMsgBuff - msg that we wnat to send
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 *
 * note 	This function will while call sendto() if sendto() return ENOENT error
 */
int msg_send(const CMSG_FD *pMsgFd, const CMSG_BUFF *pMsgBuff);


/* 
 * fn		int msg_cleanup(CMSG_FD *pMsgFd)
 * brief	Close a message fd
 * details	
 *
 * param[in]	pMsgFd - message fd that we want to close		
 *
 * return	-1 is returned if an error occurs, otherwise is 0		
 */
int msg_cleanup(CMSG_FD *pMsgFd);


/* 
 * fn		int msg_connCliAndSend(CMSG_ID msgId, CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
 * brief	init a client msg and send msg to server which is specified by msgId	
 *
 * param[in]	msgId -	server ID that we want to send
 * param[in]	pMsgFd - message fd that we want to send
 * param[in]	pMsgBuff - msg that we wnat to send
 *
 * return	-1 is returned if an error occurs, otherwise is 0	
 */
int msg_connCliAndSend(CMSG_ID msgId, CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff);


/* 
 * fn		int msg_sendAndGetReply(CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
 * brief	
 *
 * param[in]	pMsgFd - msg fd that we want to use
 * param[in/out]pMsgBuff - send msg and get reply
 * param[in]	timeSeconds - timeout in second
 *
 * return	-1 is returned if an error occurs, otherwise is 0	
 */
int msg_sendAndGetReplyWithTimeout(CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff, int timeSeconds);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* __OS_MSG_H__ */

