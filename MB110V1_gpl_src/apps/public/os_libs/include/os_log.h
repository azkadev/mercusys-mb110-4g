/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		os_log.h
 * brief		
 * details	
 *
 * author	wangwenhao
 * version	
 * date		16Jun11
 *
 * history \arg	1.0, 16Jun11, wangwenhao, create file
 */
#ifndef __OS_LOG_H__
#define __OS_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define LOG_CONTENT_LEN 256
#define LOG_MSG_SIZE	(268)
#define SYSLOGD_SHM_LOG_LEN	(32 * 1024)

#define SYSLOG_SEM_KEY				(25000 + 4)	/* "sysl" */
#define SYSLOG_SHARED_MEM_KEY		(15000 + 4)

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
typedef enum
{
	LOG_EMERG = 0,
	LOG_ALERT = 1,
	LOG_CRIT = 2,
	LOG_ERROR = 3,
	LOG_WARN = 4,
	LOG_NOTICE = 5,
	LOG_INFORM = 6,
	LOG_DEBUG = 7
} LOG_SEVERITY;

typedef enum
{
	LOG_USER = 1,
	LOG_LOCAL0 = 16,
	LOG_LOCAL1 = 17,
	LOG_LOCAL2 = 18,
	LOG_LOCAL3 = 19,
	LOG_LOCAL4 = 20,
	LOG_LOCAL5 = 21,
	LOG_LOCAL6 = 22,
	LOG_LOCAL7 = 23
} LOG_FACILITY;

typedef enum
{
	LOG_SYSTEM = 0,
	LOG_INTERNET = 1,
	LOG_DHCPD = 2,
	LOG_HTTPD = 3,
	LOG_CMM_PPP = 4,
	LOG_OTHER = 5,
	LOG_DHCPC = 6,
	LOG_DSL	= 7,
	LOG_IGMP = 8,
	LOG_3G = 9,
	LOG_VOIP = 10,
	LOG_KERNEL = 11,
	LOG_LTE = 12,
	LOG_MODULE_MAX
} LOG_MODULE;

typedef enum
{
	LOG_NORM_MSG = 0,
	LOG_CFG_MSG = 1
}LOG_TYPE;

typedef struct _LOG_MSG
{
	LOG_TYPE type;
	LOG_SEVERITY severity;
	LOG_MODULE module;
	char content[LOG_CONTENT_LEN];
} LOG_MSG;

typedef struct _LOG_CFG_BLOCK
{
	unsigned char logToRemote;	/* LogToRemote */
	unsigned int  remoteSeverity;	/* RemoteSeverity */
	char   		  serverIP[16];	/* ServerIP */
	unsigned int  serverPort;	/* ServerPort */
	unsigned int  facility;	/* Facility */
	unsigned char logToLocal;	/* LogToLocal */
	unsigned int  localSeverity;	/* LocalSeverity */
} LOG_CFG_BLOCK;

typedef struct
{
	unsigned int   	logHeadPos;	/* LogHeadPos */
	unsigned int   	logTailPos;	/* LogTailPos */	
}SYSLOGD_SHM_CONTROL_BLOCK;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn			void cmmlog(LOG_SEVERITY severity, LOG_MODULE module, const char *format, ...)
 *															 
 * brief		Recording the log to syslogd.
 *
 * param[in]	  severity   - the level of the log we want to record.
 * param[in]	  module     - which module the log from.
 * param[in]	  format    - the content we want to log.
 *
 * return		N/A
 */
void cmmlog(LOG_SEVERITY severity, LOG_MODULE module, const char *format, ...);

/* 
 * fn			void cmmlogcfg(LOG_CFG_BLOCK *pLogCfg)
 *															 
 * brief		Send the configure of log module to syslogd.
 *
 * param[in]	  pLogCfg   - the configure of the log module we want to send.
 *
 * return		N/A
 */
void cmmlogcfg(LOG_CFG_BLOCK *pLogCfg);

/* 
 * fn			void cmmlog_logRemote(unsigned int ip, unsigned short port, 
 									  const char *content, unsigned int len)
 *															 
 * brief		Recording the log to a remote syslog server.
 *
 * param[in]	  ip     - the ip address of the remote syslog server.
 * param[in]	  port   - the port of the remote syslog server.
 * param[in]	  content   - the content we want to log.
 * param[in]	  len   - the length of the log.
 *
 * return		N/A
 */
int cmmlog_logRemote(unsigned int ip, unsigned short port, const char *content, unsigned int len);

/* 
 * fn			int cmmlog_outputLog(char *buff)
 *															 
 * brief		output the log to a buffer.
 *
 * param[out]	  buff     - the buffer to save log.
 *
 * return		0/-1
 */
int cmmlog_outputLog(char *buff);

/* 
 * fn			int cmmlog_attachLogShm(void **pShmAddr)
 *															 
 * brief		attach to the shared memory of syslogd where stored the logs.
 *
 * param[out]	  pShmAddr     - the address of shared memory.
 *
 * return		0/-1
 */
int cmmlog_attachLogShm(void **pShmAddr);

/* 
 * fn			int cmmlog_detachLogShm(void *pShmAddr)
 *															 
 * brief		detach the shared memory of syslogd where stored the logs.
 *
 * param[in]	  pShmAddr     - the address of shared memory.
 *
 * return		0/-1
 */
int cmmlog_detachLogShm(void *pShmAddr);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif	/* __OS_LOG_H__ */
