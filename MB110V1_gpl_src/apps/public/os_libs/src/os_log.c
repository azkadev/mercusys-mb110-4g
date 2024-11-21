/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		os_log.c
 * brief		
 * details	
 *
 * author	wangwenhao
 * version	
 * date		16Jun11
 *
 * history \arg	1.0, 16Jun11, wangwenhao, create file
 */

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include <os_lib.h>
#include <os_msg.h>
#include <os_log.h>
 
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/
/* 
 * fn			static void escapeChar(char *pBuf, char c, int pos)
 *															 
 * brief		Transform the special character in syslog. (use for acs)
 *
 * param[out]	  pBuf   - the output buffer which is used for saving log.
 * param[in]	  c      - the character we want to transform.
 * param[in]	  pos    - the position of the character in syslog.
 *
 * return		N/A
 */
static void escapeChar(char *pBuf, char c, int pos)
{
	int i = pos;
	
	if (c == '<')
	{
		pBuf[i] = '(';
	}
	else if (c == '>')
	{
		pBuf[i] = ')';
	}
	else if (c == '&')
	{
		pBuf[i] = '*';
	}
	else if (c == '%')
	{
		pBuf[i] = '#';
	}
	else if (c == '\"')
	{
		pBuf[i] = '^';
	}
	else if (c == '\'')
	{
		pBuf[i] = '^';
	}
	else if (c == '\0')
	{
		pBuf[i] = '\n';
	}
	else
	{
		pBuf[i] = c;
	}

}

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
void cmmlog(LOG_SEVERITY severity, LOG_MODULE module, const char *format, ...)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuf;
	LOG_MSG *pLogMsg = (LOG_MSG *)msgBuf.content;
	va_list args;

	if (0 != msg_init(&msgFd))
	{
		return;
	}
	
	if (0 != msg_connSrv(CMSG_ID_LOG, &msgFd))
	{
		goto tail;
	}

	msgBuf.type = CMSG_LOG;

	pLogMsg->severity = severity;
	pLogMsg->module = module;
	pLogMsg->type = LOG_NORM_MSG;

	va_start(args, format);
	vsnprintf(pLogMsg->content, LOG_CONTENT_LEN, format, args);
	va_end(args);

	OS_DEBUG_PRINT("Send log, module %d, severity %d\n", module, severity);
	msg_send(&msgFd, &msgBuf);

tail:
	msg_cleanup(&msgFd);
}

/* 
 * fn			void cmmlogcfg(LOG_CFG_BLOCK *pLogCfg)
 *															 
 * brief		Send the configure of log module to syslogd.
 *
 * param[in]	  pLogCfg   - the configure of the log module we want to send.
 *
 * return		N/A
 */
void cmmlogcfg(LOG_CFG_BLOCK *pLogCfg)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuf;
	LOG_MSG *pLogMsg = (LOG_MSG *)msgBuf.content;

	if (0 != msg_init(&msgFd))
	{
		return;
	}
	
	if (0 != msg_connSrv(CMSG_ID_LOG, &msgFd))
	{
		goto tail;
	}

	msgBuf.type = CMSG_LOG;

	pLogMsg->type = LOG_CFG_MSG;
	memcpy(pLogMsg->content, (char *)pLogCfg, sizeof(LOG_CFG_BLOCK));

	OS_DEBUG_PRINT("Send log cfg...\n");
	msg_send(&msgFd, &msgBuf);

tail:
	msg_cleanup(&msgFd);
}

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
#define RET_FAIL   (-1)
#define RET_SUCC   (0)
int cmmlog_logRemote(unsigned int ip, unsigned short port, const char *content, unsigned int len)
{
	int sock;
	struct sockaddr_in addr;
	int sockOpt;
	int n_send;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		printf("Get socket failed, when do remote log\n");
		return RET_FAIL;
	}
	addr.sin_family = AF_INET;
	/* this may have already changed to network byte-order by inet_addr */
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port); 

	sockOpt = 1;
	if (0 != ioctl(sock, FIONBIO, (int)&sockOpt))
	{
		printf("log ioctl data nonblock fail\n");
	}
	n_send = sendto(sock, (void *)content, len, 0, (struct sockaddr *)&addr, sizeof(addr));

	close(sock);

	return n_send<0?RET_FAIL:RET_SUCC;
}

/* 
 * fn			int cmmlog_outputLog(char *buff)
 *															 
 * brief		output the log to a buffer.
 *
 * param[out]	  buff     - the buffer to save log.
 *
 * return		0/-1
 */
int cmmlog_outputLog(char *buff)
{
	SYSLOGD_SHM_CONTROL_BLOCK *pShmCtrlBlock = NULL;
	char *pLogPool = NULL;
	int ret = 0;
	int index = 0;
	int pos = 0;
	unsigned int headPos = 0;
	unsigned int tailPos = 0;

	if (NULL == buff)
	{
		return -1;
	}
	
	if ((ret = cmmlog_attachLogShm((void *)&pLogPool)) < 0)
	{
		printf("Get log pool failed\n");
		return -1;
	}

	pShmCtrlBlock = (SYSLOGD_SHM_CONTROL_BLOCK *)pLogPool;
	headPos = pShmCtrlBlock->logHeadPos;
	tailPos = pShmCtrlBlock->logTailPos;
	pLogPool = pLogPool + sizeof(SYSLOGD_SHM_CONTROL_BLOCK);

	if (headPos < tailPos)
	{
		for (pos = headPos + 1; pos < tailPos; pos++)
		{
			escapeChar(buff, pLogPool[pos], index);
			index++;
		}
	}
	else
	{
		for (pos = headPos + 1; pos < IGD_DEV_INFO_DEVICELOG_L; pos++)
		{
			escapeChar(buff, pLogPool[pos], index);
			index++;
		}

		for (pos = 0; pos < tailPos; pos++)
		{
			escapeChar(buff, pLogPool[pos], index);
			index++;
		}
	}

	buff[index] = '\0';

	cmmlog_detachLogShm(pShmCtrlBlock);
	return 0;
}

/* 
 * fn			int cmmlog_attachLogShm(void **pShmAddr)
 *															 
 * brief		attach to the shared memory of syslogd where stored the logs.
 *
 * param[out]	  pShmAddr     - the address of shared memory.
 *
 * return		0/-1
 */
int cmmlog_attachLogShm(void **pShmAddr)
{
	int shmId = 0;
	int shmFlg = 0;
	void *pAddr = NULL;
	OS_V_SEM syslogSem;
	int ret = 0;
	if (pShmAddr == NULL)
	{
		return -1;
	}
	
	shmFlg = IPC_CREAT | 0666;
	if ((shmId = os_shmGet(SYSLOG_SHARED_MEM_KEY, 
						  (SYSLOGD_SHM_LOG_LEN + sizeof(SYSLOGD_SHM_CONTROL_BLOCK)), shmFlg)) < 0)
	{
		printf("syslogd : get syslogd shared buffer error.");
		return -1;
	}

	if (NULL == (pAddr = os_shmAt(shmId, NULL, shmFlg)))
	{
		printf("syslogd : attach shared buffer error\n");
		return -1;		
	}

	if ((ret = os_semVGet(SYSLOG_SEM_KEY, &syslogSem)) < 0)
	{
		printf("syslogd : get semaphore fail\n");
		os_shmDt(pAddr);
		return -1;
	}
	
	if ((ret = os_semVTake(syslogSem)) < 0)
	{
		printf("syslogd : os_semVTake fail\n");
		os_shmDt(pAddr);
		return -1;
	}

	*pShmAddr = pAddr;
	return 0;
}

/* 
 * fn			int cmmlog_detachLogShm(void *pShmAddr)
 *															 
 * brief		detach the shared memory of syslogd where stored the logs.
 *
 * param[in]	  pShmAddr     - the address of shared memory.
 *
 * return		0/-1
 */
int cmmlog_detachLogShm(void *pShmAddr)
{
	OS_V_SEM syslogSem;
	int ret = 0;
	
	if (pShmAddr == NULL)
	{
		return -1;
	}
	
	if ((ret = os_semVGet(SYSLOG_SEM_KEY, &syslogSem)) < 0)
	{
		printf("syslogd : get semaphore fail\n");
		return -1;
	}

	os_semVGive(syslogSem);
	
	os_shmDt(pShmAddr);
	
	return 0;
}

