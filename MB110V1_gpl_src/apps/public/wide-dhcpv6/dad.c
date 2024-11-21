#include <time.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <linux/sockios.h>

#define MAX_PACKET_SIZE	32	/*mac neighbor solicit or advertise packet size*/
#define MAX_IPPACKET_SIZE	64  /*sizeof(struct mip6_hdr) + sizeof(struct nd_neighbor_solicit) */
#ifndef MAX_IF_LEN
#define MAX_IF_LEN	16
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifdef DEBUG
#define LOG_ERROR	
#define LOG_DHCP6C
#endif

#define DADRETRYTIME	3
#define DADRETRYINTERVAL	100

#if 1
/* the definition in netinet/in.h is valid only if _GNU_SOURCE has been defined, but
 * we can not define _GNU_SOURCE because we are using our dprintf function
 * also, common.h can not be included
 */
struct in6_pktinfo
  {
    struct in6_addr ipi6_addr;	/* src/dst IPv6 address */
    unsigned int ipi6_ifindex;	/* send/recv interface index */
  };
#endif /* IN6_PKTINFO */

struct mip6_hdr
{
        __u8  			version;	/*4bit version, 4bit prioty*/
        __u8            flow_lbl[3];	/*24 bit flow table*/
        unsigned short  payload_len;	/*16 bit playlenth*/
        __u8            nexthdr;	/*8 bit next header addr*/
        __u8            hop_limit;	/*8 bit hop limit*/

        struct  in6_addr        saddr;	/*source addr*/
        struct  in6_addr        daddr;	/*dest addr*/
};
/****************************************************/
/* brief: dad test, check the link addr conflict
 * in:	ifName	the interface to be checked
 * in: 	addr	the address to be checked
 * out: TRUE	DAD pass
 		-1 DAD failed
		-2 createsocket failed
		-3 malloc railed
		-4 SendNsInit failed
		-5 SendNs failed
 */
int dad_start(char *ifName, struct in6_addr addr);

/****************************************************/
/* brief: create the socket for dad test packet send and recv
 * in:	ifName	the interface name to send/recv packet 
 * in:	sendSock 	the socket to send NS
 * in:	recvSock 	the socket to receive ND

 * out: -1 create sendsocket failed
		-2 create recvsocket failed
		-3 set recvsocket failed
 		True	succeed
 */
int createDadSk(int	*sendSock, int	*recvSock);
/****************************************************/
/* brief: initialize the para before send the NS, esp assembled the packet
 * in:	sock 		the socket to send NS
 * in:	ifName		the interface name to send/recv packet 
 * in:	sladdr 		the src addr info
 * in:	toetheraddr 	the des addr info
 * in:	pSendData	the data of the NS
 * out: -1  ioctl failed
		-2  device check failed
		-3  bind failed
		True	succeed
 */
int sendNsInit(int *sock, char *ifName, struct in6_addr addr, struct sockaddr_ll *sladdr, struct sockaddr_ll* toetheraddr, char *pSendData);

/****************************************************/
/* brief: wait and recv a neighbour advertisement packet, or the time out
 * in:	sock		the packet recv socket
 * in:  targetAddr	the target address of the na packet to be recv.
 * in:  timeout		the selecte wait timeout
 * in:  ifName		the recv interface name
 * out: -1 not receive NA
		-2 didn't take the action of recving, failed before receing
		True received NA
 */
int recvNA(int sock, struct in6_addr targetAddr, struct timeval timeout, char *ifName);

/****************************************************/
/* brief: calculate  the checkSum of the ICMPV6 ND
 * in:	addr		the array to be calculated
 * in:  len		the lenth of the array
 * out: unsigned short 	the checkSum
 */
unsigned short checkSum (unsigned short *addr, int len);


void info(void)
{
	printf("usage:\n");
	printf("\tsendNS <ifName> <addr>\n");
	return;
}

/****************************************************/
/* brief: dad test, check the link addr conflict
 * in:	ifName	the interface to be checked
 * in: 	addr	the address to be checked
 * out: TRUE	DAD pass
 		-1 DAD failed
		-2 createsocket failed
		-3 malloc failed
		-4 SendNsInit failed
		-5 SendNs failed
 */
int dad_start(char *ifName, struct in6_addr addr)
{
	int dadSendSk = 0;
	int dadRecvSk = 0;
	int dadCount = 0;		
	int ret = TRUE;
	struct sockaddr_ll  sendslddr;
	struct sockaddr_ll  sendtoddr;
	char *pSendData = NULL;
	struct timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = DADRETRYINTERVAL;
	
/*create the socket for communication*/
	if(createDadSk(&dadSendSk, &dadRecvSk) < 0)
	{
		return -2;
	}
	
/*before send NS, some parameter need to be initialized*/
	if((pSendData = (char *)malloc(MAX_IPPACKET_SIZE)) == NULL)
	{
		close(dadSendSk);
		close(dadRecvSk);
		return -3;
	};

	if (sendNsInit(&dadSendSk, ifName, addr, &sendslddr,  &sendtoddr,  pSendData) < 0)
	{
		free(pSendData);
		close(dadSendSk);
		close(dadRecvSk);
		return -4;
	}
/*send the NS, recv the NA, cycle times*/
	while(dadCount < DADRETRYTIME)
	{
		if(sendto(dadSendSk, pSendData, MAX_IPPACKET_SIZE, 0, (struct sockaddr*)&sendtoddr, sizeof(struct sockaddr_ll)) < 0)
		{
			ret =  -5;
			break;
		}
		if (TRUE == recvNA(dadRecvSk, addr, tm, ifName))
		{
			ret = -1;
			break;
		}
		dadCount++;		
	}
	free(pSendData);
	close(dadRecvSk);
	close(dadSendSk);
	return ret;
}

/****************************************************/
/* brief: create the socket for dad test packet send and recv
 * in:	ifName	the interface name to send/recv packet 
 * in:	sendSock 	the socket to send NS
 * in:	recvSock 	the socket to receive ND

 * out: -1 create sendsocket failed
		-2 create recvsocket failed
		-3 set recvsocket failed
 		True	succeed
 */

int createDadSk(int  *sendSock, int  *recvSock)
{
	struct icmp6_filter filter;
	unsigned int err, val;
	struct ifreq	ifr;
/*create the packet socket to send Ethnet packet*/
   	if((*sendSock = socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_IPV6)))< 0)
	{
		return -1;
	}
/*create the raw socket to receive ICMPV6 packet*/
	if ((*recvSock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
	{
		return -2;
	}
/*set some state of the raw socket*/
	val = 1; 
	if (setsockopt(*recvSock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &val, sizeof(val)) < 0)
	{
		return -3;
	}
#ifdef IPV6_RECVHOPLIMIT
	val = 1;
	if (setsockopt(*recvSock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &val, sizeof(val)) < 0)
	{
		return -3;
	}
#endif

/*setup ICMP filter*/
	ICMP6_FILTER_SETBLOCKALL(&filter);
	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_SOLICIT, &filter);
	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filter);
	if (setsockopt(*recvSock, IPPROTO_ICMPV6, ICMP6_FILTER, &filter,
			 sizeof(filter)) < 0)
	{
		return -3;
	}
	return TRUE;
}

/****************************************************/
/* brief: initialize the para before send the NS, esp assembled the packet
 * in:	sock 		the socket to send NS
 * in:	ifName		the interface name to send/recv packet 
 * in:	sladdr 		the src addr info
 * in:	toetheraddr 	the des addr info
 * in:	pSendData	the data of the NS
 * out: -1  ioctl failed
		-2  device check failed
		-3  bind failed
		True succeed
 */
int sendNsInit(int *sock, char *ifName, struct in6_addr addr, struct sockaddr_ll *sladdr, struct sockaddr_ll* toetheraddr,  char *pSendData)
{
	struct ifreq ethreq; 
	struct mip6_hdr ip6hdr;
	struct icmp6_hdr icmp6hdr;
	struct in6_addr multAddr;
	struct nd_neighbor_solicit  ndSend;
	__u8 macTo[6] = {0x33, 0x33, 0xff, 0xff, 0xff, 0xff}; 
	int nTemp = 0; 
	int nLenth = 0;
		
/*get Dest and Target*/	
	multAddr.s6_addr32[0] = htonl(0xFF020000);
	multAddr.s6_addr32[1] = 0;
	multAddr.s6_addr32[2] = htonl(0x1);
	multAddr.s6_addr32[3] = htonl(0xFF000000) | addr.s6_addr32[3];
/*get the Multi mac*/
	memcpy(&macTo[2], &multAddr.s6_addr32[3], 4);

/*get index*/
	memset((void*)&ethreq,0,sizeof(struct ifreq)); 
	strncpy(ethreq.ifr_name, ifName ,IFNAMSIZ-1); 
	ethreq.ifr_name[IFNAMSIZ-1] = '\0';	
	if(ioctl(*sock,SIOCGIFINDEX,&ethreq) < 0) 
	{ 
		return (-1); 
	} 
	sladdr->sll_ifindex = ethreq.ifr_ifindex;
	toetheraddr->sll_ifindex = ethreq.ifr_ifindex;

/*check the device work state*/
	if (ioctl(*sock, SIOCGIFFLAGS, &ethreq) < 0)
	{
		return (-1);
	}
	if (!(ethreq.ifr_flags & IFF_UP) && !(ethreq.ifr_flags & IFF_RUNNING) && !(ethreq.ifr_flags & IFF_MULTICAST))
	{
		return (-2);
	}
/*fill ip6hdr*/
	ip6hdr.version = 0x60;
	ip6hdr.saddr = in6addr_any;
	ip6hdr.daddr = multAddr;
	ip6hdr.payload_len = 0x1800;
	memset(ip6hdr.flow_lbl, 0x00, 3);
	ip6hdr.nexthdr = 0x3a;
	ip6hdr.hop_limit = 0xff;
	
/*fill nd packet*/
	ndSend.nd_ns_hdr.icmp6_type = 135;
	ndSend.nd_ns_hdr.icmp6_code = 0;
	ndSend.nd_ns_hdr.icmp6_cksum = 0x00;
	ndSend.nd_ns_hdr.icmp6_dataun.icmp6_un_data32[0] = 0;
	ndSend.nd_ns_target = addr;
/*cal icmpv6 checkSum*/
	memset(pSendData, 0, MAX_IPPACKET_SIZE); 
	memcpy(pSendData, &ip6hdr.saddr,  sizeof(struct in6_addr));
	memcpy(pSendData + sizeof(struct in6_addr),  &ip6hdr.daddr,  sizeof(struct in6_addr));
	nLenth = 2*sizeof(struct in6_addr);
	nTemp = htonl(sizeof(struct nd_neighbor_solicit));
	memcpy(pSendData + nLenth, &nTemp,  4);
	nLenth +=  4;
	nTemp = htonl(IPPROTO_ICMPV6);
	memcpy(pSendData + nLenth, &nTemp,  4);
	nLenth +=  4;
	memcpy(pSendData + nLenth, &ndSend,  sizeof(struct nd_neighbor_solicit));
	nLenth +=  sizeof(struct nd_neighbor_solicit);
	ndSend.nd_ns_hdr.icmp6_cksum = checkSum((__u16 *)pSendData,  nLenth);

/*set source addr*/
	 memset((void*)sladdr,0,sizeof(struct sockaddr_ll)); 
	 sladdr->sll_family = AF_PACKET; 
	 sladdr->sll_protocol = htons(ETH_P_IPV6); 

	 sladdr->sll_halen = sizeof(macTo); 
/*set Dest addr*/
	 toetheraddr->sll_family = AF_PACKET; 
	 toetheraddr->sll_protocol = htons(ETH_P_IPV6); 
	 toetheraddr->sll_halen = sizeof(macTo); 
	if (ioctl(*sock, SIOCGIFHWADDR, &ethreq) < 0)
	{
		return -1; 
	}
	 memcpy((void*)sladdr->sll_addr,ethreq.ifr_hwaddr.sa_data, 6); 
	 memcpy((void*)toetheraddr->sll_addr,macTo, sizeof(macTo)); 
	
/*bind the sock and full the packet Data*/
	if (bind(*sock, (struct sockaddr *)sladdr, sizeof(*sladdr)) < 0)
	{
		return -3; 
	}
	memset(pSendData, 0, MAX_IPPACKET_SIZE); 
	memcpy((void*)pSendData, &ip6hdr, sizeof(struct mip6_hdr)); 
	memcpy((void*)&pSendData[sizeof(struct mip6_hdr)], &ndSend, sizeof(struct nd_neighbor_solicit));
	return TRUE;
}
/****************************************************/
/* brief: wait and recv a neighbour advertisement packet, or the time out
 * in:	sock		the packet recv socket
 * in:  targetAddr	the target address of the na packet to be recv.
 * in:  timeout		the selecte wait timeout
 * in:  ifName		the recv interface name
 * out: -1 not receive NA
		-2 didn't take the action of recving, failed before receing
		True received NA
 */
int recvNA(int sock, struct in6_addr targetAddr, struct timeval timeout, char *ifName)
{
	struct msghdr mhdr;
	struct cmsghdr *cmsg = NULL;
	struct iovec iov;
	struct sockaddr_in6 addr;
	struct in6_pktinfo *pktInfo = NULL; 
	struct nd_neighbor_advert *pAdv;
	struct icmp6_hdr *icmph;
	unsigned char *chdr = NULL;
	unsigned int chdrlen = 0;
	fd_set rfds;
	int hoplimit;
	char *msg = NULL;
/*to recv the addrinfo and hoplimit*/
	chdrlen = CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int));
	if ((chdr = malloc(chdrlen)) == NULL)
	{
			return -2;
	}

/*to recv NA*/
	msg = malloc(MAX_PACKET_SIZE * sizeof(char ));
	if (msg == NULL)
	{
		free(chdr);
		return -2;
	}

	while(1)
	{
		pktInfo = NULL;
		FD_ZERO( &rfds );
		FD_SET( sock, &rfds );
/*if recv error or no packet, quit*/
		if( select( sock+1, &rfds, NULL, NULL, &timeout ) <= 0 )
		{
			free(msg);
			free(chdr);
			return -1;
		}

		iov.iov_len = MAX_PACKET_SIZE;
		iov.iov_base = (caddr_t) msg;

		memset(&mhdr, 0, sizeof(mhdr));
		mhdr.msg_name = (caddr_t)&addr;
		mhdr.msg_namelen = sizeof(addr);
		mhdr.msg_iov = &iov;
		mhdr.msg_iovlen = 1;
		mhdr.msg_control = (void *)chdr;
		mhdr.msg_controllen = chdrlen;
/*get the packet and do judgement*/
		if (recvmsg(sock, &mhdr, 0) < 0)
		{
			continue;
		}

/*to identify the packet type, we only want to get NA or NS*/
		for (cmsg = CMSG_FIRSTHDR(&mhdr); cmsg != NULL; cmsg = CMSG_NXTHDR(&mhdr, cmsg))
		{
          	if (cmsg->cmsg_level != IPPROTO_IPV6)
          		continue;
          
          	switch(cmsg->cmsg_type)
         	{
#ifdef IPV6_HOPLIMIT
              case IPV6_HOPLIMIT:
                if ((cmsg->cmsg_len == CMSG_LEN(sizeof(int))) && 
                    (*(int *)CMSG_DATA(cmsg) >= 0) && 
                    (*(int *)CMSG_DATA(cmsg) < 256))
                {
                  hoplimit = *(int *)CMSG_DATA(cmsg);
                }
                break;
#endif /* IPV6_HOPLIMIT */

              case IPV6_PKTINFO:
                if ((cmsg->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo))) &&
                    ((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_ifindex)
                {
                  pktInfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
                }
                break;
          	}
		}

		if (pktInfo == NULL)
			continue;

		icmph = (struct icmp6_hdr *) msg;

/*We just want to listen to NSs and NAs*/
		if ((icmph->icmp6_type != ND_NEIGHBOR_ADVERT) || (icmph->icmp6_code != 0))
			continue;

/*NA packet*/
		pAdv = (struct nd_neighbor_advert *)msg;
		if ( 0 == memcmp(&pAdv->nd_na_target, &targetAddr, sizeof(struct in6_addr)))
		{
			free(msg);
			free(chdr);
			return TRUE;
		}			
	}
	free(msg);
	free(chdr);
	return -1;
}

/****************************************************/
/* brief: calculate  the checkSum of the ICMPV6 ND
 * in:	addr			the array to be calculated
 * in:  len			the lenth of the array
 * out: unsigned short 		the checkSum
 */
unsigned short checkSum (unsigned short  *addr, int len)
	{
	int nleft = len;
	int sum = 0;
	unsigned short  *w = addr;
	unsigned short  answer = 0;

	while (nleft > 1) 
	{
		sum += *w++;
		nleft -= sizeof (unsigned short );
	}

	if (nleft == 1) 
	{
		*(__u8 *) (&answer) = *(__u8  *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}
