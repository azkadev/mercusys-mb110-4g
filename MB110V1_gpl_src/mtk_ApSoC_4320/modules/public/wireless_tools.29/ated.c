#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <netinet/in.h>

#include "ated.h"

#define	 DRIVER_CHANGED	

static int connfd;
static int ioctlfd = 0;
static int cmdFlags[] = {
	RTPRIV_IOCTL_ATE_SET,
	RTPRIV_IOCTL_ATE_SET_DA,
	RTPRIV_IOCTL_ATE_SET_SA,
	RTPRIV_IOCTL_ATE_SET_BSSID,
	RTPRIV_IOCTL_ATE_SET_TX_CHANNEL,
	RTPRIV_IOCTL_ATE_SET_TX_MODE,
	RTPRIV_IOCTL_ATE_SET_TX_MCS,
	RTPRIV_IOCTL_ATE_SET_TX_BW,
	RTPRIV_IOCTL_ATE_SET_TX_GI,
	RTPRIV_IOCTL_ATE_SET_TX_LENGTH,
	RTPRIV_IOCTL_ATE_SET_TX_POW0,
	RTPRIV_IOCTL_ATE_SET_TX_POW1,
	RTPRIV_IOCTL_ATE_SET_TX_COUNT,
	RTPRIV_IOCTL_ATE_SET_TX_FREQOFFSET,
	RTPRIV_IOCTL_ATE_SET_TX_ANT,	
	RTPRIV_IOCTL_ATE_SET_RX_ANT,
	RTPRIV_IOCTL_ATE_AUTO_ALC,
	RTPRIV_IOCTL_ATE_VCO_CAL,
	RTPRIV_IOCTL_ATE_SET_RESETCOUNTER,
	RTPRIV_IOCTL_ATE_SET_RX_FER	,
	RTPRIV_IOCTL_ATE_EFUSE_WRITEBACK,
	RTPRIV_IOCTL_ATE_EFUSE_LOAD,
	RTPRIV_IOCTL_ATE_WRITECAL

};

void server_addr_init(struct sockaddr_in *server)
{
	server->sin_family = AF_INET;
	server->sin_addr.s_addr = htonl(INADDR_ANY);
	server->sin_port = htons(5000);
}

void send_message(int sendCode, char *info)
{
	char *msg;
	msg = (char *)malloc(sizeof(char)*20 + sizeof(info));
	if (IWPRIV_ERROR == sendCode)
	{	
		sprintf(msg, "set %s error!\r\n# ", info);
	}
	else if (IWPRIV_OK == sendCode)
	{
		sprintf(msg, "%s\r\n# ", info);
	}
	send(connfd, msg, strlen(msg), 0);

	//printf("msg : %s\n", msg);
	
	free(msg);	
}

int  ated_ioctl(struct iwreq *wrq, int param)
{

	char name[10];
	sprintf(name, "%s", "ra0");

	strcpy(wrq->ifr_name, name);
	
	if (ioctl(ioctlfd, param, (void *)wrq) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	
	return 0;
}

int set_ate(char *data)
{
#ifdef DRIVER_CHANGED
	struct iwreq wrq;
	char param[20]; 
	int index = 0;
	int cmdCount = 10;
	memset(&wrq, 0, sizeof(wrq));
	if (data[3] == '$' || data[3] == '&')
	{
		if (data[3] == '&')
		{
			cmdCount = 4;
		}
		sprintf(data, "%s", data + 5);
		for (index = 0; index < cmdCount; ++index)
		{
			sscanf(data,"%[^;];%s", param, data);
			wrq.u.data.length = strlen(param);
			wrq.u.data.pointer = param;
			wrq.u.data.flags = cmdFlags[index + 4];
			
			if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
			{
				printf("%s, %d\n", __func__, __LINE__);
				return -1;
			}
		}
	}
	else
	{
		sscanf(data, "%*[^=]=%[^+]+%d", param, &index);
		wrq.u.data.length = strlen(param);
		wrq.u.data.pointer = param;
		wrq.u.data.flags = cmdFlags[index - 4];

		if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
		{
			printf("%s, %d\n", __func__, __LINE__);
			return -1;
		}
		//printf("\n%s,%d,%04x\n", param, index, cmdFlags[index - 4]);
	}
#else
	char cmdBuf[1024];
	sprintf(cmdBuf, "iwpriv ra0 set %s", data);
	printf("%s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}

int read_eeprom(char *data, char *result)
{
	struct iwreq wrq;
	char temp[50];
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_E2P) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(result, "%s", temp + 1);
	return 0;
}

int write_eeprom(char *data)
{
#if 1
	struct iwreq wrq;
	char temp[50];
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_E2P) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
#else
	util_execSystem(__FUNCTION__, "iwpriv ra0 e2p %s", data);
	printf("exe : iwpriv ra0 e2p %s\n", data);
#endif
	return 0;
}

int write_cal(char *data)
{
#ifdef DRIVER_CHANGED 
	struct iwreq wrq;
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(data);
	wrq.u.data.pointer = data;
	wrq.u.data.flags = RTPRIV_IOCTL_ATE_WRITECAL;
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
#else
	char cmdBuf[1024];
	sprintf(cmdBuf, "iwpriv ra0 set %s", data);
	printf("%s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}

int load_efuse(char *data)
{
	struct iwreq wrq;
	char *value;
	memset(&wrq, 0, sizeof(wrq));
	if ((value = strchr(data, '=')) == NULL)
		return -1;
	value++;
	wrq.u.data.length = strlen(value);
	wrq.u.data.pointer = value;
	wrq.u.data.flags = RTPRIV_IOCTL_ATE_EFUSE_LOAD;
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;	
}

int efuse_writeBack(char *data)
{
	struct iwreq wrq;
	char *value;
	memset(&wrq, 0, sizeof(wrq));
#if 1
	if ((value = strchr(data, '=')) == NULL)
		return -1;
	value++;
	wrq.u.data.length = strlen(value);
	wrq.u.data.pointer = value;
#else
	wrq.u.data.length = strlen(data);
	wrq.u.data.pointer = data;
#endif
	wrq.u.data.flags = RTPRIV_IOCTL_ATE_EFUSE_WRITEBACK;
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;	
}

int get_stat(char *data, char *result)
{
	struct iwreq wrq;
	char temp[2048];
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_STATISTICS) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(temp, "%s", strstr(temp, "Rx success"));
	sscanf(temp, "%[^\n]", result);	
	return 0;
}

void parse_cmd(char* cmd)
{
	char *ptr;
	char data[50];
	char* delim = " ";
	char *value;
	char result[2048];
	int cmdlen = strlen(cmd);
	memset(result, 0, sizeof(result));
	//printf("cmd : %s\n", cmd);
	if (cmd[cmdlen - 1] == '\r' || cmd[cmdlen - 1] == '\n')
	{
		cmd[cmdlen - 1] = '\0';
	}

	ptr = strtok(cmd, delim);

	if(0 == strcmp(ptr, "iwpriv"))
	{
		ptr = strtok(NULL, delim);
		if (0 == strcmp(ptr, "ra0"))
		{
			ptr = strtok(NULL, delim);
			if (0 == strcmp(ptr, "set"))
			{
				ptr = strtok(NULL, delim);
				if (0 == strncmp(ptr, "ATE", strlen("ATE"))
					|| (0 == strncmp(ptr, "ResetCounter", strlen("ResetCounter"))))
				{
					sprintf(data, "%s", ptr);
					if (set_ate(data) < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, "");
					}
				}
				else if (0 == strncmp(ptr, "efuseLoadFromBin", strlen("efuseLoadFromBin")))
				{
					sprintf(data, "%s", ptr);
					if (load_efuse(data) < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, "");
					}					
				}
				else if (0 == strncmp(ptr, "efuseBufferModeWrite", strlen("efuseBufferModeWrite")))
				{
					sprintf(data, "%s", ptr);
					if (efuse_writeBack(data) < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, "");
					}					
				}
				else if (0 == strncmp(ptr, "WriteCal", strlen("WriteCal")))
				{
					sprintf(data, "%s", ptr);
					if (write_cal(data) < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, "");
					}					
				}
			}
			else if (0 == strncmp(ptr, "e2p", strlen("e2p")))
			{
				ptr = strtok(NULL, delim);
				if ((value = strchr(ptr, '=')) != NULL)
					*value++ = 0;
				
				if (!value || !*value)
				{
					sprintf(data, "%s", ptr);
					if (read_eeprom(data, result) < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, result);
					}
				}
				else
				{
					sprintf(data, "%s=%s", ptr, value);
					if (write_eeprom(data) < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, "");
					}
				}
				
			}
			else if (0 == strcmp(ptr, "stat"))
			{
				sprintf(data, "%s", ptr);
				if (get_stat(data, result) < 0)
				{
					send_message(IWPRIV_ERROR, ptr);
				}
				else
				{
					send_message(IWPRIV_OK, result);
				}				
			}
			else
			{
				send_message(IWPRIV_ERROR, ptr);
			}
		}
		else
		{
			send_message(IWPRIV_ERROR, ptr);
		}
	}
	else
	{
		send_message(IWPRIV_ERROR, ptr);
	}
	
}

int readLine(int fd, void*vptr, unsigned int maxlen)
{
	int read_len, rc;
	unsigned char c,*ptr;
	ptr=vptr;
	for(read_len = 1; read_len < maxlen; read_len++)
	{
again:

		/*get one char*/
		if((rc = read(fd,&c,1)) == 1)
		{
			*ptr=c;
			ptr++;
			if(c == '\n')
				break;
		}
		/*read socket over*/
		else if(0 == rc)
		{
			*ptr=0;
			return (read_len - 1);
		}
		/*read socket error*/
		else
		{
			if(EINTR==errno)
				goto again;
			return (-1);
		}
	}
	if(strcmp(vptr, "\r\n") != 0)
	{
		*(--ptr)=0;
		//*(--ptr)=0;
	}
	return read_len;
}

int main()
{
	int fd;
	int addr_len;
	char cmd[CMD_MAXLEN];
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server_addr_init(&server);

	printf("wireless tool : ated open SUCC\r\n");
	/*create socket*/
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	ioctlfd = socket(AF_INET, SOCK_STREAM, 0);
	if(ioctlfd < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}

	/*bind to a sockaddr*/
	bind(fd, (struct sockaddr*)&server, sizeof(server));

	listen(fd, SOCK_MAX_LISTEN);

	/*accept new connection and get descriptor*/
	addr_len = sizeof(struct sockaddr_in);
	connfd = accept(fd, &server, &addr_len);
	
	/*receive command*/
#if 0
	while(1)
	{
		memset(cmd, 0, CMD_MAXLEN);
		if(recv(connfd, cmd, CMD_MAXLEN, 0)<= 0)
		{
			close(connfd);
			connfd = accept(fd, &server, &addr_len);
		}
		else 
		{
			parse_cmd(cmd);
		}
	}
#else
	send_message(IWPRIV_OK, "");
	while(1)
	{
		memset(cmd, 0, CMD_MAXLEN);
		if(readLine(connfd, cmd, CMD_MAXLEN)<= 0)
		{
			close(connfd);
			connfd = accept(fd, &server, &addr_len);
		}
		else 
			parse_cmd(cmd);
	}
#endif	
	
	close(fd);
    close(connfd);
	close(ioctlfd);
	
	return 0;
}

