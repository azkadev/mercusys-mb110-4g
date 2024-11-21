#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "ated_tp.h"

/*#define	 DRIVER_CHANGED	*/

#define DEFAULT_PORT 5000
#define SECONDARY_PORT 5001

static int connfd;
static int ioctlfd = 0;

#ifdef DRIVER_CHANGED
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
#ifdef INCLUDE_WIFI_5G_CHIP_MT7612E
	RTPRIV_IOCTL_ATE_SET_TX_POW_EVAL,
#endif
	RTPRIV_IOCTL_ATE_SET_TX_ANT,	
	RTPRIV_IOCTL_ATE_SET_RX_ANT,
	RTPRIV_IOCTL_ATE_AUTO_ALC,
	RTPRIV_IOCTL_ATE_VCO_CAL,
	RTPRIV_IOCTL_ATE_IPG,
	RTPRIV_IOCTL_ATE_SET_RESETCOUNTER,
	RTPRIV_IOCTL_ATE_SET_RX_FER	,
	RTPRIV_IOCTL_ATE_EFUSE_WRITEBACK,
	RTPRIV_IOCTL_ATE_EFUSE_LOAD,
	RTPRIV_IOCTL_ATE_WRITECAL

};
#endif
void server_addr_init(struct sockaddr_in *server)
{
	server->sin_family = AF_INET;
	server->sin_addr.s_addr = htonl(INADDR_ANY);
	server->sin_port = htons(DEFAULT_PORT);
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
	if (ioctl(ioctlfd, param, (void *)wrq) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	
	return 0;
}

int set_ate(char *data, char *interface)
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
		
		//sprintf(data, "%s", data + 5);
		data += 5;	/* skip "ATE$=" and "ATE&=" */
		
		for (index = 0; index < cmdCount; ++index)
		{
			sscanf(data,"%[^;];%s", param, data);
			wrq.u.data.length = strlen(param);
			wrq.u.data.pointer = param;
			wrq.u.data.flags = cmdFlags[index + 4];

#ifdef INCLUDE_WIFI_5G_CHIP_MT7612E
			if ((0 == strcmp(interface, "rai0")) 
				&& ((cmdFlags[index + 4] == RTPRIV_IOCTL_ATE_SET_TX_POW0) 
			 		|| (cmdFlags[index + 4] == RTPRIV_IOCTL_ATE_SET_TX_POW1)))
			{
				continue; /* MT7612E的5G使用RTPRIV_IOCTL_ATE_SET_TX_POW_EVAL替代了这两项*/
			}

#endif

#ifndef INCLUDE_WIFI_5G_CHIP_MT7612E
			if (cmdFlags[index + 4] == RTPRIV_IOCTL_ATE_SET_TX_POW1 && (0 == strcmp(interface, "rai0")))
			{
				continue; /*5G has only one chain*/
			}
#endif

			strcpy(wrq.ifr_name, interface);
			
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
		wrq.u.data.flags = cmdFlags[index - 5];
		strcpy(wrq.ifr_name, interface);

		if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
		{
			printf("%s, %d\n", __func__, __LINE__);
			return -1;
		}
		//printf("\n%s,%d,%04x\n", param, index, cmdFlags[index - 4]);
	}
#else

	if (0 == strncmp(data, "ATEIBfInstCal", strlen("ATEIBfInstCal")))
	{
		int fd;
		fd = open("/proc/mt7613/value", O_RDWR|O_NONBLOCK);
		if (fd < 0)
		{
			perror("/proc/mt7613/value");
			return -1;
		}
		else
		{
			int ret = write(fd, "clear", 5);
			if(ret < 0)
			{
				perror("clear txbf data");
				//printf("ret : %d\n", ret);
				close(fd);
				return -1;
			}
		}
		close(fd);
	}

	char cmdBuf[1024] = {0};
	sprintf(cmdBuf, "iwpriv %s set %s", interface, data);
	printf("%s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}

int read_eeprom(char *data, char *result, char *interface)
{
	struct iwreq wrq;
	char temp[50] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_E2P) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(result, "%s", temp + 1);
	return 0;
}

int write_eeprom(char *data, char *interface)
{
#ifdef DRIVER_CHANGED
	struct iwreq wrq;
	char temp[50] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);

	printf("asuka: write e2p: %s\n", data);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_E2P) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
#else
	char cmdBuf[1024] = {0};
	sprintf(cmdBuf, "iwpriv %s e2p %s\n", interface, data);
	printf("asuka : %s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}

int read_mac_reg(char *data, char *result, char *interface)
{
	struct iwreq wrq;
	char temp[50] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_MAC) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(result, "%s", temp + 1);
	return 0;
}

int write_mac_reg(char *data, char *interface)
{
#ifdef DRIVER_CHANGED
	struct iwreq wrq;
	char temp[50] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);

	printf("asuka: write e2p: %s\n", data);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_MAC) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
#else
	char cmdBuf[1024] = {0};
	sprintf(cmdBuf, "iwpriv %s mac %s\n", interface, data);
	printf("asuka : %s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}


int read_bbp_reg(char *data, char *result, char *interface)
{
	struct iwreq wrq;
	char temp[50] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_BBP) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(result, "%s", temp + 1);
	return 0;
}

int write_bbp_reg(char *data, char *interface)
{
	#ifdef DRIVER_CHANGED
	struct iwreq wrq;
	char temp[50] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);

	printf("asuka: write e2p: %s\n", data);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_BBP) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	#else
	char cmdBuf[1024] = {0};
	sprintf(cmdBuf, "iwpriv %s bbp %s\n", interface, data);
	printf("asuka : %s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}

/* C2 use this. I can not find WriteCal in 7603 & 7612*/
int write_cal(char *data, char *interface)
{
#ifdef DRIVER_CHANGED 
	struct iwreq wrq;
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(data);
	wrq.u.data.pointer = data;
	wrq.u.data.flags = RTPRIV_IOCTL_ATE_WRITECAL;
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
#else
	char cmdBuf[1024] = {0};
	sprintf(cmdBuf, "iwpriv %s set %s", interface, data);
	printf("%s\n", cmdBuf);
	system(cmdBuf);
#endif
	return 0;
}

int load_efuse(char *data, char *interface)
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
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;	
}

int efuse_writeBack(char *data, char *interface)
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
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_ATE) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;	
}

/* only returned info for "Rx success" */
int get_stat(char *data, char *result, char *interface)
{
	struct iwreq wrq;
	char temp[2048] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_STATISTICS) < 0) 
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(temp, "%s", strstr(temp, "Rx success"));
	sscanf(temp, "%[^\n]", result);
	return 0;
}

/* "False CCA" is part of "stat" */
int get_false_cca(char *data, char *result, char *interface)
{
	struct iwreq wrq;
	char temp[2048] = {0};
	sprintf(temp, "%s", data);
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.length = strlen(temp);
	wrq.u.data.pointer = temp;
	wrq.u.data.flags = 0;
	strcpy(wrq.ifr_name, interface);
	if (ated_ioctl(&wrq, RTPRIV_IOCTL_STATISTICS) < 0)
	{
		printf("%s, %d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(temp, "%s", strstr(temp, "False CCA"));
	sscanf(temp, "%[^\n]", result);
	return 0;
}

/*
to get txbf calibration status in procfs
changed by wanghe
*/

int get_txbf_cal_status(char *result, unsigned long len)
{
	if (result == NULL || len < 64)
	{
		perror("get_txbf_cal_status");
		return -1;
	}
	int fd;
	fd = open("/proc/mt7613/status", O_RDWR|O_NONBLOCK);
	if (fd < 0)
	{
		perror("/proc/mt7613/status");
		return -1;
	}
	int ret = read(fd, result, len);
	if(ret < 0)
	{
		perror("read txbf status /proc/mt7613/status");
		//printf("ret : %d\n", ret);
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/*
to get txbf calibration values in procfs
changed by wanghe
*/
int get_txbf_cal(char *result, unsigned long len)
{
	if (result == NULL || len < 64)
	{
		perror("get_txbf_cal");
		return -1;
	}
	int fd;
	fd = open("/proc/mt7613/value", O_RDWR|O_NONBLOCK);
	if (fd < 0)
	{
		perror("/proc/mt7613/value");
		return -1;
	}
	int ret = read(fd, result, len);
	if(ret < 0)
	{
		perror("read txbf value /proc/mt7613/value");
		//printf("ret : %d\n", ret);
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/* we should consider IPA/EPA and chip diff. */
void parse_cmd(char* cmd)
{
	char *ptr;
	char data[50];
	char* delim = " ";
	char *value;
	char result[2048];
	int cmdlen = strlen(cmd);
	memset(result, 0, sizeof(result));
	
	/*
	ATE:         cmd for tx and rx settings.
	efuse:       cmd for efuse (chip memory), IPA/ILNA only
	WriteCal:    C2 only?
	e2p:		 flash R/W. 
	stat:        cmd for rx statisic
	*/

	printf("cmd : %s\n", cmd);

	/* ??? */
	if (cmd[cmdlen - 1] == '\r' || cmd[cmdlen - 1] == '\n')
	{
		cmd[cmdlen - 1] = '\0';
	}

	if (cmd == strstr(cmd, "ifconfig"))
	{
		printf("%s\n", cmd);
		system(cmd);
		send_message(IWPRIV_OK, "");
	}
	else if (cmd == strstr(cmd, "TxbfCalStatus"))
	{
		printf("%s\n", cmd);
		get_txbf_cal_status(result, sizeof(result));
		send_message(IWPRIV_OK, result);
	}
	else if (cmd == strstr(cmd, "TxbfCalResult"))
	{
		printf("%s\n", cmd);

		get_txbf_cal(result, sizeof(result));
		send_message(IWPRIV_OK, result);
	}/*for test*/
	else
	{
		ptr = strtok(cmd, delim);
		if (0 == strcmp(ptr, "iwpriv"))
		{
			ptr = strtok(NULL, delim);
			if (0 == strcmp(ptr, "ra0"))
			{
				ptr = strtok(NULL, delim);
				if (0 == strcmp(ptr, "set"))
				{
					ptr = strtok(NULL, delim);
					if (0 == strncmp(ptr, "ATE", strlen("ATE"))
						|| (0 == strncmp(ptr, "ResetCounter", strlen("ResetCounter")))
						|| (0 == strncmp(ptr, "SKUEnable", strlen("SKUEnable"))))
					{
						sprintf(data, "%s", ptr);
						if (set_ate(data, "ra0") < 0)
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
						if (load_efuse(data, "ra0") < 0)
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
						if (efuse_writeBack(data,"ra0") < 0)
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
						if (write_cal(data, "ra0") < 0)
						{
							send_message(IWPRIV_ERROR, ptr);
						}
						else
						{
							send_message(IWPRIV_OK, "");
						}
					}
					else if (0 == strncmp(ptr, "bufferWriteBack", strlen("bufferWriteBack")))
					{
						sprintf(data, "%s", ptr);
						if (write_cal(data, "ra0") < 0)
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
						if (read_eeprom(data, result, "ra0") < 0)
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
						if (write_eeprom(data, "ra0") < 0)
						{
							printf("write_eeprom fail\n");
							send_message(IWPRIV_ERROR, ptr);
						}
						else
						{
							printf("write_eeprom success\n");
							send_message(IWPRIV_OK, "");
						}
					}
				}
				else if (0 == strncmp(ptr, "mac", strlen("mac")))
				{
					ptr = strtok(NULL, delim);
					if ((value = strchr(ptr, '=')) != NULL)
						*value++ = 0;

					if (!value || !*value)
					{
						sprintf(data, "%s", ptr);
						if (read_mac_reg(data, result, "ra0") < 0)
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
						if (write_mac_reg(data, "ra0") < 0)
						{
							send_message(IWPRIV_ERROR, ptr);
						}
						else
						{
							send_message(IWPRIV_OK, "");
						}
					}
				}
				else if (0 == strncmp(ptr, "bbp", strlen("bbp")))
				{
					ptr = strtok(NULL, delim);
					if ((value = strchr(ptr, '=')) != NULL)
						*value++ = 0;

					if (!value || !*value)
					{
						sprintf(data, "%s", ptr);
						if (read_bbp_reg(data, result, "ra0") < 0)
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
						if (write_bbp_reg(data, "ra0") < 0)
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
					printf("asuka: ra0 try to get stat\n");
					sprintf(data, "%s", ptr);
					if (get_stat(data, result, "ra0") < 0)
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
			else if (0 == strcmp(ptr, "rai0"))
			{
				ptr = strtok(NULL, delim);
				if (0 == strcmp(ptr, "set"))
				{
					ptr = strtok(NULL, delim);
					if (0 == strncmp(ptr, "ATE", strlen("ATE"))
						|| (0 == strncmp(ptr, "ResetCounter", strlen("ResetCounter")))
						|| (0 == strncmp(ptr, "DyncVgaEnable", strlen("DyncVgaEnable"))) 
						/*  for 7613 */
						|| 0 == strncmp(ptr, "SKUCtrl", strlen("SKUCtrl"))
						|| 0 == strncmp(ptr, "TxBfTxApply", strlen("TxBfTxApply")))
					{

						sprintf(data, "%s", ptr);
						if (set_ate(data, "rai0") < 0)
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
						if (load_efuse(data, "rai0") < 0)
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
						if (efuse_writeBack(data,"rai0") < 0)
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
						if (write_cal(data, "rai0") < 0)
						{
							send_message(IWPRIV_ERROR, ptr);
						}
						else
						{
							send_message(IWPRIV_OK, "");
						}
					}
					else if (0 == strncmp(ptr, "bufferWriteBack", strlen("bufferWriteBack")))
					{
						sprintf(data, "%s", ptr);
						if (write_cal(data, "rai0") < 0)
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
						if (read_eeprom(data, result, "rai0") < 0)
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
						if (write_eeprom(data, "rai0") < 0)
						{
							send_message(IWPRIV_ERROR, ptr);
						}
						else
						{
							send_message(IWPRIV_OK, "");
						}
					}

				}
				else if (0 == strncmp(ptr, "mac", strlen("mac")))
				{
					ptr = strtok(NULL, delim);
					if ((value = strchr(ptr, '=')) != NULL)
						*value++ = 0;

					if (!value || !*value)
					{
						sprintf(data, "%s", ptr);
						if (read_mac_reg(data, result, "rai0") < 0)
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
						if (write_mac_reg(data, "rai0") < 0)
						{
							send_message(IWPRIV_ERROR, ptr);
						}
						else
						{
							send_message(IWPRIV_OK, "");
						}
					}
				}
				else if (0 == strncmp(ptr, "bbp", strlen("bbp")))
				{
					ptr = strtok(NULL, delim);
					if ((value = strchr(ptr, '=')) != NULL)
						*value++ = 0;

					if (!value || !*value)
					{
						sprintf(data, "%s", ptr);
						if (read_bbp_reg(data, result, "rai0") < 0)
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
						if (write_bbp_reg(data, "rai0") < 0)
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
					if (get_stat(data, result, "rai0") < 0)
					{
						send_message(IWPRIV_ERROR, ptr);
					}
					else
					{
						send_message(IWPRIV_OK, result);
					}
				}
				else if (0 == strcmp(ptr, "false_cca"))
				{
					sprintf(data, "%s", ptr);
					if (get_false_cca(data, result, "rai0") < 0)
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
		else if (0 == strcmp(ptr, "uclited"))
		{
			char cmdBuf[1024] = {0};
			sprintf(cmdBuf, "uclited --force_cal");
			printf("%s\n", cmdBuf);
			system(cmdBuf);
		}
		else
		{
			send_message(IWPRIV_ERROR, ptr);
		}
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
	int ret = 0;
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server_addr_init(&server);

	if(daemon(1, 1) != 0)
	{
		printf("daemon error\n");
		exit(1);
	}
	printf("wireless tool : ated_tp open SUCC\r\n");
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
	ret = bind(fd, (struct sockaddr*)&server, sizeof(server));
	if (ret == -1)
	{
		server.sin_port = htons(SECONDARY_PORT);
		ret = bind(fd, (struct sockaddr*)&server, sizeof(server));
		if (ret == -1)
		{
			printf("%s, %d:bind error\n", __func__, __LINE__);
			exit(1);
		}
		printf("using port %d\n", SECONDARY_PORT);
	}
	else
	{
		printf("using port %d\n", DEFAULT_PORT);
	}

	listen(fd, SOCK_MAX_LISTEN);

	/*accept new connection and get descriptor*/
	addr_len = sizeof(struct sockaddr_in);
	connfd = accept(fd, &server, &addr_len);

	while (1)
	{
		memset(cmd, 0, CMD_MAXLEN);
		/* if(recv(connfd, cmd, CMD_MAXLEN, 0)<= 0) */
		if (readLine(connfd, cmd, CMD_MAXLEN) <= 0)
		{
			close(connfd);
			connfd = accept(fd, &server, &addr_len);
		}
		else 
		{
			parse_cmd(cmd);
		}
	}
	
	close(fd);
    close(connfd);
	close(ioctlfd);
	
	return 0;
}

