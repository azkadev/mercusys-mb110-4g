#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <net/if.h>
#include "os_msg.h"


#define EXCLUDE_G_EXPIRATIONTHREADPOOL 1



#define CHAIN_NAME_LEN 32
#define BITRATE_LEN 32
#define PATH_LEN 64
#define RESULT_LEN 512
#define NUM_LEN 32

/*used for length type, Added by LI CHENGLONG , 2011-Nov-02.*/
#define BUFLEN_8 8
#define BUFLEN_16 16
#define BUFLEN_32 32
#define BUFLEN_64 64
#define BUFLEN_128 128
#define BUFLEN_256 256
#define BUFLEN_512 512
#define BUFLEN_1024 1024
#define BUFLEN_2048 2048
#define BUFLEN_4096 4096
/* Ended by LI CHENGLONG , 2011-Nov-02.*/

/*default configs,Added by LI CHENGLONG , 2011-Nov-02.*/
#define MAPPING_DATA_BASE_SYNC_PERIOD	3
#define IGD_DEFAULT_WEB_PORT	80
#define IGD_DEFAULT_SUBCOUNT	16
#define IGD_DEFAULT_PMCOUNT     64
#define UPNP_PID_FILE_NAME "upnp.pid"
#define PID_PATH "/var/run"
#define UPNPD_CONFIG_FILE_PATH	"/var/tmp/upnpd"
#define UPNPD_PM_LIST_DB_FILE  "/var/tmp/upnpd/pm.db"
/* Ended by LI CHENGLONG , 2011-Nov-02.*/

#define DESC_DOC_URL_LENGTH (7 + 15 + 1 + 5 + 1 + 6 + 1 + PATH_LEN) /* http://ipaddr:port/randomPrefix/docName */

/*for some function use, Added by LI CHENGLONG , 2011-Nov-11.*/
#define UBOOL32 unsigned int
#define FALSE 0
#define TRUE 1

#define UINT32  unsigned int
#define UINT16	unsigned short
/* Ended by LI CHENGLONG , 2011-Nov-11.*/


#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

struct GLOBALS {
	char extInterfaceName[IFNAMSIZ]; // The name of the external interface, picked up from the
	                               // command line
	char intInterfaceName[IFNAMSIZ]; // The name of the internal interface, picked from command line

	char extIntfAddr[WAN_IP_CONN_EXTERNALIPADDRESS_L];

	/*配置数据, Added by LI CHENGLONG , 2011-Nov-02.*/
	UBOOL32 natEnabled;
	char manufacterUrl[BUFLEN_128];
	char manufacter[BUFLEN_64];
	char modelName[BUFLEN_64];
	char modelVer[BUFLEN_16];
	char description[BUFLEN_256];
	char deviceVersion[BUFLEN_16];
	char deviceName[BUFLEN_512];
	int  webPort;
 	/* Ended by LI CHENGLONG , 2011-Nov-02.*/
	

	// All vars below are read from /etc/upnpd.conf in main.c
	int debug;  // 1 - print debug messages to syslog
	           // 0 - no debug messages
	char iptables[PATH_LEN];  // The full name and path of the iptables executable, used in pmlist.c
	char upstreamBitrate[BITRATE_LEN];  // The upstream bitrate reported by the daemon
	char downstreamBitrate[BITRATE_LEN]; // The downstream bitrate reported by the daemon
	char forwardChainName[CHAIN_NAME_LEN];  // The name of the iptables chain to put FORWARD rules in
	char preroutingChainName[CHAIN_NAME_LEN]; // The name of the chain to put PREROUTING rules in
	int forwardRules;     // 1 - forward rules are inserted
	                      // 0 - no forward rules inserted
	long int duration;    // 0 - no duration
	                      // >0 - duration in seconds
	                      // <0 - expiration time 
	char descDocName[PATH_LEN];
	char xmlPath[PATH_LEN];
	char randomPrefix[BUFLEN_8];
  
};

typedef struct GLOBALS* globals_p;
extern struct GLOBALS g_vars;

/* upnp default config setting, Added by LI CHENGLONG , 2011-Nov-11.*/
#define CONF_FILE "/var/tmp/upnpd/upnpd.conf"
#define CONF_FILE_NAME "upnpd.conf"
#define MAX_CONFIG_LINE 256
#define IPTABLES_DEFAULT_FORWARD_CHAIN "FORWARD"
#define IPTABLES_DEFAULT_PREROUTING_CHAIN "PREROUTING"
#define DEFAULT_DURATION 0
#define DEFAULT_UPSTREAM_BITRATE "0"
#define DEFAULT_DOWNSTREAM_BITRATE "0"
#define DESC_DOC_DEFAULT "gatedesc.xml"
#define XML_PATH_DEFAULT "/var/tmp/upnpd"


//#define IGD_PORT 49378
/* Ended by LI CHENGLONG , 2011-Nov-11.*/


/* 
 * brief: Added by LI CHENGLONG, 2011-Nov-11.
 *		  开启关闭upnp的消息数据结构.
 */
typedef struct _IGD_UPNP_CTL
{
	unsigned int enable;
}IGD_UPNP_CTL;


#endif // _GLOBALS_H_
