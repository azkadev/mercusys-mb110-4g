/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		wlnetilnkTool.c
 * brief	For wireless netlink operation.	
 * details	
 *
 * author		Zeng Yi
 * version		1.0.0
 * date			29Nov11
 *
 * history 	\arg 1.0.0, 16Jun11, Zeng Yi, Create the file.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> 
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define WLAN_WPS_COS "WLAN WPS"
#define WLAN_SWITCH_COS "WLAN SWITCH"
#define WLAN_WPS_COS_5G "WLAN 5G WPS"
/*add by xieping*/
#define QMI_RECONNECT_COS "QMI_RECONNECT"
#define QMI_DISCONNECT_COS "QMI_DISCONNECT"
/*end add*/
/*add by wx*/
#define RESET_BUTTON_PRESSED_COS "RESET BUTTON PRESSED"
//addn
/* add by zeng weiji, 2018-04-13 */
#define SUCCESS_WPS "SUCCESS WPS"
#define SUCCESS_WPS_5G "SUCCESS 5G WPS"
/* end add */
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
#ifndef __LINUX_OS_FC__
#define __LINUX_OS_FC__
#endif
#include "os_msg.h"
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#ifndef MAC_ADDR_LENGTH
#define MAC_ADDR_LENGTH 6
#endif
#ifndef NDIS_802_11_LENGTH_SSID
#define NDIS_802_11_LENGTH_SSID 32	
#endif
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

typedef struct _WLAN_NDIS_SSID
{
    unsigned int SsidLength;         // length of SSID field below, in bytes;
                                // this can be zero.
    unsigned char   Ssid[NDIS_802_11_LENGTH_SSID];           // SSID information field
}WLAN_NDIS_SSID;

typedef	struct	_WLAN_WSC_CREDENTIAL
{
	WLAN_NDIS_SSID		SSID;				// mandatory
	unsigned short		AuthType;			// mandatory, 1: open, 2: wpa-psk, 4: shared, 8:wpa, 0x10: wpa2, 0x20: wpa2-psk
	unsigned short		EncrType;			// mandatory, 1: none, 2: wep, 4: tkip, 8: aes
#ifdef INCLUDE_WIFI_5G_CHIP_MT7612E
	unsigned char		Key[64 + 1];			// mandatory, Maximum 64 byte
#else
	unsigned char		Key[64];			// mandatory, Maximum 64 byte
#endif
	unsigned short		KeyLength;
	unsigned char		MacAddr[MAC_ADDR_LENGTH];			// mandatory, AP MAC address
	unsigned char		KeyIndex;			// optional, default is 1
	unsigned char		bFromUPnP;			// TRUE: This credential is from external UPnP registrar
	unsigned char		Rsvd[2];			// Make alignment
}WLAN_WSC_CREDENTIAL, *PWLAN_WSC_CREDENTIAL;

typedef enum _ENUM_WLAN_AUTHMODE
{
	WLAN_AUTHMODE_OPEN = 1,
	WLAN_AUTHMODE_SHARED = 2,
	WLAN_AUTHMODE_OPEN_SHARED = 3,
	WLAN_AUTHMODE_WPA = 4,
	WLAN_AUTHMODE_WPA2 = 5,
	WLAN_AUTHMODE_PSK = 6,
	WLAN_AUTHMODE_PSK2 = 7,
	WLAN_AUTHMODE_WPA_WPA2 = 8,
	WLAN_AUTHMODE_PSK_PSK2 = 9
}ENUM_WLAN_AUTHMODE;

typedef enum _ENUM_WLAN_ENCRYPTMODE
{
	WLAN_ENCRYPTMODE_NONE = 1,
	WLAN_ENCRYPTMODE_WEP = 2,
	WLAN_ENCRYPTMODE_TKIP = 3,
	WLAN_ENCRYPTMODE_AES = 4,
	WLAN_ENCRYPTMODE_TKIP_AES = 5
}ENUM_WLAN_ENCRYPTMODE;

struct rtnl_handle
{
	int			fd;
	struct sockaddr_nl	local;
	struct sockaddr_nl	peer;
	unsigned int			seq;
	unsigned int			dump;
};
/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
static void sendWscCredToCos(unsigned char isErr, 
							 char* ssid, 
							 char* key, 
							 int authMode, 
							 int encryMode,
							 char* ifname);
static void sendWlanSwitchToCos(void);
static void sendQmiReconnectToCos(void);
/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

static void sendWscCredToCos(unsigned char isErr, 
							 char* ssid, 
							 char* key, 
							 int authMode, 
							 int encryMode,
							 char* ifname)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuff;

	WPS_CFG_MSG *pWpsCfgMsg = NULL;
	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msgBuff, 0 , sizeof(CMSG_BUFF));

	pWpsCfgMsg = (WPS_CFG_MSG *)(msgBuff.content);

	pWpsCfgMsg->isError = isErr;
	if (!isErr)
	{
		strcpy(pWpsCfgMsg->SSID, ssid);
		if (authMode != WLAN_AUTHMODE_OPEN && NULL != key)
			strcpy(pWpsCfgMsg->key, key);
		pWpsCfgMsg->authMode = authMode;
		pWpsCfgMsg->encryMode = encryMode;
		if (ifname)
		{
			memcpy(pWpsCfgMsg->iface, ifname, 16);
		}
		msgBuff.type = CMSG_WPS_CFG;
	}
	msg_init(&msgFd);
	msg_connSrv(CMSG_ID_COS, &msgFd);

	msg_send(&msgFd, &msgBuff);

	msg_cleanup(&msgFd);
	return;
}

static void sendWlanSwitchToCos(void)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuff;

	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msgBuff, 0 , sizeof(CMSG_BUFF));
	
	msgBuff.type = CMSG_WLAN_SWITCH;
	msg_init(&msgFd);
	msg_connSrv(CMSG_ID_COS, &msgFd);

	msg_send(&msgFd, &msgBuff);

	msg_cleanup(&msgFd);
	return;
}

static void sendQmiReconnectToCos(void)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuff;

	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msgBuff, 0 , sizeof(CMSG_BUFF));

	msgBuff.type = CMSG_QMI_RECONNECT;
	msg_init(&msgFd);
	msg_connSrv(CMSG_ID_COS, &msgFd);

	msg_send(&msgFd, &msgBuff);

	msg_cleanup(&msgFd);
	return;
}

static void sendQmiDisconnectToCos(void)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuff;

	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msgBuff, 0 , sizeof(CMSG_BUFF));

	msgBuff.type = CMSG_QMI_DISCONNECT;
	msg_init(&msgFd);
	msg_connSrv(CMSG_ID_COS, &msgFd);

	msg_send(&msgFd, &msgBuff);

	msg_cleanup(&msgFd);
	return;
}
static void sendResetButtonPressToCos(void)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuff;

	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msgBuff, 0 , sizeof(CMSG_BUFF));

	msgBuff.type = CMSG_RESET_BUTTON_PRESSED;
	msg_init(&msgFd);
	msg_connSrv(CMSG_ID_COS, &msgFd);

	msg_send(&msgFd, &msgBuff);

	msg_cleanup(&msgFd);
	return;

}
/* band: 0 - 2.4G, 1 - 5G */
static void sendWscSuccessStatusToCos(unsigned char band)
{
	CMSG_FD msgFd;
	CMSG_BUFF msgBuff;

	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msgBuff, 0 , sizeof(CMSG_BUFF));

	if (0 == band)
		msgBuff.type = CMSG_WPS_SUCCESS_24G;
	if (1 == band)
		msgBuff.type = CMSG_WPS_SUCCESS_5G;

	msg_init(&msgFd);
	msg_connSrv(CMSG_ID_COS, &msgFd);

	msg_send(&msgFd, &msgBuff);

	msg_cleanup(&msgFd);
	return;
}

static inline void rtnl_close(struct rtnl_handle *rth)
{
	close(rth->fd);
}

static inline int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions)
{
	int addr_len;

	memset(rth, 0, sizeof(rth));

	rth->fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (rth->fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	memset(&rth->local, 0, sizeof(rth->local));
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_groups = subscriptions;

	if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}
	addr_len = sizeof(rth->local);
	if (getsockname(rth->fd, (struct sockaddr*)&rth->local,
			(socklen_t *) &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(rth->local)) {
		fprintf(stderr, "Wrong address length %d\n", addr_len);
		return -1;
	}
	if (rth->local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", rth->local.nl_family);
		return -1;
	}
	rth->seq = time(NULL);
	return 0;
}

static void proc_wireless_event(int ifindex, char *data, int len)
{
	if (0 == memcmp(data, SUCCESS_WPS, strlen(SUCCESS_WPS)))
	{
		sendWscSuccessStatusToCos(0);
		return;
	}
	else if (0 == memcmp(data, SUCCESS_WPS_5G, strlen(SUCCESS_WPS_5G)))
	{
		sendWscSuccessStatusToCos(1);
		return;
	}
	else if (0 == memcmp(data, QMI_RECONNECT_COS, strlen(QMI_RECONNECT_COS)))
	{
		sendQmiReconnectToCos();
		return;
	}
	else if (0 == memcmp(data, QMI_DISCONNECT_COS, strlen(QMI_DISCONNECT_COS)))
	{
		sendQmiDisconnectToCos();
		return;
	}
	else if (0 == memcmp(data, WLAN_SWITCH_COS, strlen(WLAN_SWITCH_COS)))
	{
		sendWlanSwitchToCos();
		return;
	}
	else if (0 == memcmp(data, RESET_BUTTON_PRESSED_COS, strlen(RESET_BUTTON_PRESSED_COS)))
	{
		sendResetButtonPressToCos();
		return;
	}
	else if (0 == memcmp(data, WLAN_WPS_COS, strlen(WLAN_WPS_COS)) || 0 == memcmp(data, WLAN_WPS_COS_5G, strlen(WLAN_WPS_COS_5G)))
	{
		char ifname[16] = "ra0";
		PWLAN_WSC_CREDENTIAL pWscCredential;
		if (0 == memcmp(data, WLAN_WPS_COS, strlen(WLAN_WPS_COS)))
		{
			memset(ifname, 0, 16);
			sprintf(ifname, "%s", "ra0");
			pWscCredential = (PWLAN_WSC_CREDENTIAL)(data + strlen(WLAN_WPS_COS));
		}
		else if (0 == memcmp(data, WLAN_WPS_COS_5G, strlen(WLAN_WPS_COS_5G)))
		{
			memset(ifname, 0, 16);
			sprintf(ifname, "%s", "rai0");
			pWscCredential = (PWLAN_WSC_CREDENTIAL)(data + strlen(WLAN_WPS_COS_5G));
		}
		//PWLAN_WSC_CREDENTIAL pWscCredential = (PWLAN_WSC_CREDENTIAL)(data + strlen(WLAN_WPS_COS));
		char ssid[33] = {0};
		char wscKey[65] = {0};
		printf("****ssid is %s\n",pWscCredential->SSID.Ssid);
		memcpy(ssid, pWscCredential->SSID.Ssid, pWscCredential->SSID.SsidLength);
		ssid[pWscCredential->SSID.SsidLength] = '\0';
		memcpy(wscKey, pWscCredential->Key, pWscCredential->KeyLength);
		wscKey[pWscCredential->KeyLength] = '\0';
		printf("authtype is %d,encrtype is %d\n",pWscCredential->AuthType,pWscCredential->EncrType);
		switch (pWscCredential->AuthType)
		{
		case WLAN_AUTHMODE_OPEN:
			switch (pWscCredential->EncrType)
			{
			case WLAN_ENCRYPTMODE_NONE:
				sendWscCredToCos(0, ssid, wscKey, pWscCredential->AuthType, pWscCredential->EncrType, ifname);
				break;
			default:
				sendWscCredToCos(1, NULL, NULL, 0, 0, ifname);
			}
			break;
		case WLAN_AUTHMODE_PSK:
		case WLAN_AUTHMODE_PSK2:
		case WLAN_AUTHMODE_PSK_PSK2:
			switch (pWscCredential->EncrType)
			{
			case WLAN_ENCRYPTMODE_TKIP_AES:
			case WLAN_ENCRYPTMODE_AES:
			case WLAN_ENCRYPTMODE_TKIP:
				sendWscCredToCos(0, ssid, wscKey, pWscCredential->AuthType, pWscCredential->EncrType, ifname);
				break;
			default:
				sendWscCredToCos(1, NULL, NULL, 0, 0, ifname);
			}
			break;
		default:
			sendWscCredToCos(1, NULL, NULL, 0, 0, ifname);
			break;
		}
	}
	return;
}

static int LinkCatcher(struct nlmsghdr *nlh)
{
	struct ifinfomsg* ifi;
	ifi = NLMSG_DATA(nlh);

  	/* Only keep add/change events */
  	if(nlh->nlmsg_type != RTM_NEWLINK)
		return 0;

  	/* Check for attributes */
  	if (nlh->nlmsg_len > NLMSG_ALIGN(sizeof(struct ifinfomsg)))
	{
		int attrlen = nlh->nlmsg_len - NLMSG_ALIGN(sizeof(struct ifinfomsg));
      	struct rtattr *attr = (void *) ((char *) ifi + 
			NLMSG_ALIGN(sizeof(struct ifinfomsg)));

      	while (RTA_OK(attr, attrlen))
		{
	  		/* Check if the Wireless kind */
	  		if(attr->rta_type == IFLA_WIRELESS)
	    	{
				proc_wireless_event(ifi->ifi_index, 
					(char *) attr + RTA_ALIGN(sizeof(struct rtattr)), 
					attr->rta_len - RTA_ALIGN(sizeof(struct rtattr)));
	    	}
	  		attr = RTA_NEXT(attr, attrlen);
		}
    }

  	return 0;
}

static inline void handle_netlink_events(struct rtnl_handle *	rth)
{
	while(1)
    {
		struct sockaddr_nl sanl;
      	socklen_t sanllen = sizeof(struct sockaddr_nl);

      	struct nlmsghdr *h;
      	int amt;
      	char buf[8192];
      	amt = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
      	if(amt < 0)
		{
	  		if(errno != EINTR && errno != EAGAIN)
	    	{
	      		//fprintf(stderr, "%s: error reading netlink: %s.\n", 
				//	__PRETTY_FUNCTION__, strerror(errno));
	    	}
	  		return;
		}

		if(amt == 0)
		{
	  		fprintf(stderr, "%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
	  		return;
		}

      	h = (struct nlmsghdr*)buf;
      	while(amt >= (int)sizeof(*h))
		{
	  		int len = h->nlmsg_len;
	  		int l = len - sizeof(*h);

	  		if(l < 0 || len > amt)
	    	{
	      		fprintf(stderr, "%s: malformed netlink message: len=%d\n", __PRETTY_FUNCTION__, len);
	      		break;
	    	}
			
			switch(h->nlmsg_type)
			{
			case RTM_NEWLINK:
				LinkCatcher(h);
	      		break;
	    	default:
	      		break;
	    	}

			len = NLMSG_ALIGN(len);
			amt -= len;
			h = (struct nlmsghdr*)((char*)h + len);
		}

		if(amt > 0)
			fprintf(stderr, "%s: remnant of size %d on netlink\n", __PRETTY_FUNCTION__, amt);
	}
}

static void wait_for_event(struct rtnl_handle *rth)
{
	/* Forever */
	while(1)
    {
		fd_set		rfds;		/* File descriptors for select */
      	int			last_fd;	/* Last fd */
      	int			ret;

		/* Guess what ? We must re-generate rfds each time */
      	FD_ZERO(&rfds);
      	FD_SET(rth->fd, &rfds);
      	last_fd = rth->fd;

		/* Wait until something happens */
      	ret = select(last_fd + 1, &rfds, NULL, NULL, NULL);

		/* Check if there was an error */
		if(ret < 0)
		{
			if(errno == EAGAIN || errno == EINTR)
				continue;
			fprintf(stderr, "Unhandled signal - exiting...\n");
			break;
		}

		/* Check if there was a timeout */
		if(ret == 0)
		{
			continue;
		}

		/* Check for interface discovery events. */
		if(FD_ISSET(rth->fd, &rfds))
		{
			handle_netlink_events(rth);
		}
	}

  return;
}

static void *swWlanChkAhbErr(void *args)
{
	struct rtnl_handle	rth;
  	int opt;
  	/* Open netlink channel */

  	if(rtnl_open(&rth, RTMGRP_LINK) < 0)
    {
      	perror("Can't initialize rtnetlink socket");
		return(1);
    }

  	fprintf(stderr, "Waiting for Wireless Events from interfaces...\n");

  	/* Do what we have to do */
	fprintf(stderr, "%s: netlink to do\n", 
					__PRETTY_FUNCTION__);
  	wait_for_event(&rth);

  	/* Cleanup - only if you are pedantic */
  	rtnl_close(&rth);
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

int main(int argc, char ** argv)
{
	int retVal;
	pthread_t netlinkHandler_thread;
	int sig;
	sigset_t sigs_to_catch;
	
	printf("WLAN-Start wlNetlinkTool\n");
	retVal = pthread_create(&netlinkHandler_thread, NULL, swWlanChkAhbErr, NULL);
	if (0 == retVal)
	{
		pthread_detach(netlinkHandler_thread);
	}
	else
	{
		printf("Error to create pthread\n");
	}
	/* Catch Ctrl-C and properly shutdown */
    sigemptyset(&sigs_to_catch);
    sigaddset(&sigs_to_catch, SIGINT);
    sigwait(&sigs_to_catch, &sig);
	printf("Shutdown wlNetlinkTool\n");
	return 0;
}
