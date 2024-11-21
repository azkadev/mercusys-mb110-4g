/*
 * RTSP extension for IP connection tracking
 * (C) 2003 by Tom Marshall <tmarshall at real.com>
 * based on ip_conntrack_irc.c
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Module load syntax:
 *   insmod nf_conntrack_rtsp.o ports=port1,port2,...port<MAX_PORTS>
 *                              max_outstanding=n setup_timeout=secs
 *
 * If no ports are specified, the default will be port 554.
 *
 * With max_outstanding you can define the maximum number of not yet
 * answered SETUP requests per RTSP session (default 8).
 * With setup_timeout you can specify how long the system waits for
 * an expected data channel (default 300 seconds).
 *
 * 2005-02-13: Harald Welte <laforge at netfilter.org>
 * 	- port to 2.6
 * 	- update to recent post-2.6.11 api changes
 * 2006-09-14: Steven Van Acker <deepstar at singularity.be>
 *      - removed calls to NAT code from conntrack helper: NAT no longer needed to use rtsp-conntrack
 * 2007-04-18: Michael Guntsche <mike at it-loops.com>
 * 			- Port to new NF API
 */

#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <net/tcp.h>

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <linux/netfilter/nf_conntrack_rtsp.h>
#include <linux/inetdevice.h>
#include <net/netfilter/nf_nat_helper.h>

#define NF_NEED_STRNCASECMP
#define NF_NEED_STRTOU16
#define NF_NEED_STRTOU32
#define NF_NEED_NEXTLINE
#include <linux/netfilter_helpers.h>
#define NF_NEED_MIME_NEXTLINE
#include <linux/netfilter_mime.h>

#include <linux/ctype.h>
#define MAX_SIMUL_SETUP 8 /* XXX: use max_outstanding */
#define INFOP(fmt, args...) printk(KERN_INFO "%s: %s: " fmt, __FILE__, __FUNCTION__ , ## args)
#if 0
#define DEBUGP(fmt, args...) printk(KERN_DEBUG "%s: %s: " fmt, __FILE__, __FUNCTION__ , ## args)
#else
#define DEBUGP(fmt, args...)
#endif

#define MAX_PORTS 8
static int ports[MAX_PORTS];
static int num_ports = 0;
static int max_outstanding = 8;
static unsigned int setup_timeout = 300;
static char* lanDevName = "br0";

MODULE_AUTHOR("Tom Marshall <tmarshall at real.com>");
MODULE_DESCRIPTION("RTSP connection tracking module");
MODULE_LICENSE("GPL");
module_param_array(ports, int, &num_ports, 0400);
MODULE_PARM_DESC(ports, "port numbers of RTSP servers");
module_param(max_outstanding, int, 0400);
MODULE_PARM_DESC(max_outstanding, "max number of outstanding SETUP requests per RTSP session");
module_param(setup_timeout, int, 0400);
MODULE_PARM_DESC(setup_timeout, "timeout on for unestablished data channels");
module_param(lanDevName, charp, 0400);
MODULE_PARM_DESC(lanDevName, "the LAN device name");


static char *rtsp_buffer;
static DEFINE_SPINLOCK(rtsp_buffer_lock);

unsigned int (*nf_nat_rtsp_hook)(struct sk_buff *skb,
				 enum ip_conntrack_info ctinfo,
				 unsigned int matchoff, unsigned int matchlen,struct ip_ct_rtsp_expect* prtspexp,
				 struct nf_conntrack_expect *exp,
				 int* sizeDiff);
void (*nf_nat_rtsp_hook_expectfn)(struct nf_conn *ct, struct nf_conntrack_expect *exp);

int isSameLanAddr(unsigned int ip, char* devName);


EXPORT_SYMBOL_GPL(nf_nat_rtsp_hook);
EXPORT_SYMBOL_GPL(isSameLanAddr);

/*
 * Max mappings we will allow for one RTSP connection (for RTP, the number
 * of allocated ports is twice this value).  Note that SMIL burns a lot of
 * ports so keep this reasonably high.  If this is too low, you will see a
 * lot of "no free client map entries" messages.
 */
#define MAX_PORT_MAPS 16

/*** default port list was here in the masq code: 554, 3030, 4040 ***/

#define SKIP_WSPACE(ptr,len,off) while(off < len && isspace(*(ptr+off))) { off++; }

#define RTSP_CONVERT_IP_TO_PORT(ip)	\
	((((ip & 0x00ff0000) >> 16) + ((ip & 0x0000ff00) >> 8) + (ip & 0x000000ff)) * 8)

/*
 * Parse an RTSP packet.
 *
 * Returns zero if parsing failed.
 *
 * Parameters:
 *  IN      ptcp        tcp data pointer
 *  IN      tcplen      tcp data len
 *  IN/OUT  ptcpoff     points to current tcp offset
 *  OUT     phdrsoff    set to offset of rtsp headers
 *  OUT     phdrslen    set to length of rtsp headers
 *  OUT     pcseqoff    set to offset of CSeq header
 *  OUT     pcseqlen    set to length of CSeq header
 */
static int
rtsp_parse_message(char* ptcp, uint tcplen, uint* ptcpoff,
                   uint* phdrsoff, uint* phdrslen,
                   uint* pcseqoff, uint* pcseqlen,
                   uint* transoff, uint* translen)
{
	uint    entitylen = 0;
	uint    lineoff;
	uint    linelen;

	if (!nf_nextline(ptcp, tcplen, ptcpoff, &lineoff, &linelen))
		return 0;

	*phdrsoff = *ptcpoff;
	while (nf_mime_nextline(ptcp, tcplen, ptcpoff, &lineoff, &linelen)) {
		if (linelen == 0) {
			if (entitylen > 0)
				*ptcpoff += min(entitylen, tcplen - *ptcpoff);
			break;
		}
		if (lineoff+linelen > tcplen) {
			INFOP("!! overrun !!\n");
			break;
		}

		if (nf_strncasecmp(ptcp+lineoff, "CSeq:", 5) == 0) {
			*pcseqoff = lineoff;
			*pcseqlen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Transport:", 10) == 0) {
			*transoff = lineoff;
			*translen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Content-Length:", 15) == 0) {
			uint off = lineoff+15;
			SKIP_WSPACE(ptcp+lineoff, linelen, off);
			nf_strtou32(ptcp+off, &entitylen);
		}
	}
	*phdrslen = (*ptcpoff) - (*phdrsoff);

	return 1;
}

/*
 * Parse an RTSP packet which is of reply type.
 *
 * Returns zero if parsing failed.
 *
 * Parameters:
 *  IN      ptcp        tcp data pointer
 *  IN      tcplen      tcp data len
 *  IN/OUT  ptcpoff     points to current tcp offset
 *  OUT     phdrsoff    set to offset of rtsp headers
 *  OUT     phdrslen    set to length of rtsp headers
 *  OUT     pcseqoff    set to offset of CSeq header
 *  OUT     pcseqlen    set to length of CSeq header
 */
static int
rtsp_parse_in_message(char* ptcp, uint tcplen, uint* ptcpoff,
                   uint* phdrsoff, uint* phdrslen,
                   uint* pcseqoff, uint* pcseqlen,
                   uint* transoff, uint* translen,
                   uint* locatoff, uint* locatlen)
{
	uint    entitylen = 0;
	uint    lineoff;
	uint    linelen;

	if (!nf_nextline(ptcp, tcplen, ptcpoff, &lineoff, &linelen))
		return 0;

	*phdrsoff = *ptcpoff;
	while (nf_mime_nextline(ptcp, tcplen, ptcpoff, &lineoff, &linelen)) {
		if (linelen == 0) {
			if (entitylen > 0)
				*ptcpoff += min(entitylen, tcplen - *ptcpoff);
			break;
		}
		if (lineoff+linelen > tcplen) {
			INFOP("!! overrun !!\n");
			break;
		}

		if (nf_strncasecmp(ptcp+lineoff, "CSeq:", 5) == 0) {
			*pcseqoff = lineoff;
			*pcseqlen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Transport:", 10) == 0) {
			*transoff = lineoff;
			*translen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Location:", 9) == 0) {
			*locatoff = lineoff;
			*locatlen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Content-Length:", 15) == 0) {
			uint off = lineoff+15;
			SKIP_WSPACE(ptcp+lineoff, linelen, off);
			nf_strtou32(ptcp+off, &entitylen);
		}
	}
	*phdrslen = (*ptcpoff) - (*phdrsoff);

	return 1;
}

/*
 * Find lo/hi client ports (if any) in transport header
 * In:
 *   ptcp, tcplen = packet
 *   tranoff, tranlen = buffer to search
 *
 * Out:
 *   pport_lo, pport_hi = lo/hi ports (host endian)
 *
 * Returns nonzero if any client ports found
 *
 * Note: it is valid (and expected) for the client to request multiple
 * transports, so we need to parse the entire line.
 */
static int
rtsp_parse_transport(char* ptran, uint tranlen,
                     struct ip_ct_rtsp_expect* prtspexp,
					 struct nf_conn *ct, 
					 /* Added by Wang Wenhu, Sep9th,2014 */
					 enum ip_conntrack_dir pkt_dir)
{
	int     rc = 0;
	uint    off = 0;

	u_int16_t divisor = 0;
	__be32 srcIp = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;

	if(isSameLanAddr(srcIp, lanDevName))
	{
		divisor = (u_int16_t)RTSP_CONVERT_IP_TO_PORT(srcIp);
	}

	/* Added by Wang Wenhu, Sep9th,2014
	 * 
	 * If the packet is in the direction of reply, we
	 * should not change the "Transport" value because
	 * the value had been changed in the "SETUP" packet
	 * which is the peer packet in the direction of original.
	 */
	if(IP_CT_DIR_REPLY == pkt_dir)
	{
		divisor = 0;
	}

	DEBUGP("The original src ip is %u.%u.%u.%u, the original dst ip is %u.%u.%u.%u, the reply src ip is %u.%u.%u.%u, the reply dst ip is %u.%u.%u.%u.\n", 
	NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip), 
	NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip),
	NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip),
	NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip));
	DEBUGP("####the ipoffset is %x.\n", divisor);

	if (tranlen < 10 || !iseol(ptran[tranlen-1]) ||
	    nf_strncasecmp(ptran, "Transport:", 10) != 0) {
		INFOP("sanity check failed\n");
		return 0;
	}

	DEBUGP("tran='%.*s'\n", (int)tranlen, ptran);
	off += 10;
	SKIP_WSPACE(ptran, tranlen, off);

	/* Transport: tran;field;field=val,tran;field;field=val,... */
	while (off < tranlen) {
		const char* pparamend;
		uint        nextparamoff;

		pparamend = memchr(ptran+off, ',', tranlen-off);
		pparamend = (pparamend == NULL) ? ptran+tranlen : pparamend+1;
		nextparamoff = pparamend-ptran;

		while (off < nextparamoff) {
			const char* pfieldend;
			uint        nextfieldoff;

			pfieldend = memchr(ptran+off, ';', nextparamoff-off);
			nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

			if (strncmp(ptran+off, "client_port=", 12) == 0) {
				u_int16_t   port;
				uint        numlen;

				off += 12;
				numlen = nf_strtou16(ptran+off, &port);
				off += numlen;
				if (prtspexp->loport != 0 && prtspexp->loport != port)
					DEBUGP("multiple ports found, port %hu ignored\n", port);
				else {
					DEBUGP("lo port found : %hu\n", port);
					prtspexp->loport = prtspexp->hiport = port + divisor;
					if (ptran[off] == '-') {
						off++;
						numlen = nf_strtou16(ptran+off, &port);
						off += numlen;
						prtspexp->pbtype = pb_range;
						prtspexp->hiport = port + divisor;

						// If we have a range, assume rtp:
						// loport must be even, hiport must be loport+1
						if ((prtspexp->loport & 0x0001) != 0 ||
						    prtspexp->hiport != prtspexp->loport+1) {
							DEBUGP("incorrect range: %hu-%hu, correcting\n",
							       prtspexp->loport, prtspexp->hiport);
							prtspexp->loport &= 0xfffe;
							prtspexp->hiport = prtspexp->loport+1;
						}
					} else if (ptran[off] == '/') {
						off++;
						numlen = nf_strtou16(ptran+off, &port);
						off += numlen;
						prtspexp->pbtype = pb_discon;

						prtspexp->hiport = port + divisor;
					}
					rc = 1;
				}
			}

			/*
			 * Note we don't look for the destination parameter here.
			 * If we are using NAT, the NAT module will handle it.  If not,
			 * and the client is sending packets elsewhere, the expectation
			 * will quietly time out.
			 */

			off = nextfieldoff;
		}

		off = nextparamoff;
	}

	return rc;
}

void expected(struct nf_conn *ct, struct nf_conntrack_expect *exp)
{
    if(nf_nat_rtsp_hook_expectfn) {
        nf_nat_rtsp_hook_expectfn(ct,exp);
    }
}

/*** conntrack functions ***/

/* outbound packet: client->server */

static inline int
help_out(struct sk_buff *skb, unsigned char *rb_ptr, unsigned int datalen,
                struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	struct ip_ct_rtsp_expect expinfo;

	int dir = CTINFO2DIR(ctinfo);   /* = IP_CT_DIR_ORIGINAL */
	//struct  tcphdr* tcph = (void*)iph + iph->ihl * 4;
	//uint    tcplen = pktlen - iph->ihl * 4;
	char*   pdata = rb_ptr;
	//uint    datalen = tcplen - tcph->doff * 4;
	uint    dataoff = 0;
	int ret = NF_ACCEPT;
	int sizeDiff = 0;

	struct nf_conntrack_expect *exp;

	__be16 be_loport;

	memset(&expinfo, 0, sizeof(expinfo));

	while (dataoff < datalen) {
		uint    cmdoff = dataoff;
		uint    hdrsoff = 0;
		uint    hdrslen = 0;
		uint    cseqoff = 0;
		uint    cseqlen = 0;
		uint    transoff = 0;
		uint    translen = 0;
		uint    off;

		if (!rtsp_parse_message(pdata, datalen, &dataoff,
					&hdrsoff, &hdrslen,
					&cseqoff, &cseqlen,
					&transoff, &translen))
			break;      /* not a valid message */

		if(strstr(pdata+cmdoff, "SETUP ") == NULL)
			continue;   /* not a SETUP message */
		DEBUGP("found a setup message\n");

		off = 0;
		if(translen) {
			rtsp_parse_transport(pdata+transoff, translen, &expinfo, ct, dir);
		}

		if (expinfo.loport == 0) {
			DEBUGP("no udp transports found\n");
			continue;   /* no udp transports found */
		}

		DEBUGP("udp transport found, ports=(%d,%hu,%hu)\n",
		       (int)expinfo.pbtype, expinfo.loport, expinfo.hiport);

		exp = nf_ct_expect_alloc(ct);
		if (!exp) {
			ret = NF_DROP;
			goto out;
		}

		//be_loport = htons(expinfo.loport);
		be_loport = (expinfo.loport);
		DEBUGP("be_loport=(%hu)\n", be_loport);

		nf_ct_expect_init(exp, NF_CT_EXPECT_CLASS_DEFAULT,
			ct->tuplehash[!dir].tuple.src.l3num,
			&ct->tuplehash[!dir].tuple.src.u3, &ct->tuplehash[!dir].tuple.dst.u3,
			IPPROTO_UDP, NULL, &be_loport);

		exp->master = ct;

		exp->expectfn = expected;
		exp->flags = 0;

		if (expinfo.pbtype == pb_range) {
			DEBUGP("Changing expectation mask to handle multiple ports\n");
			/* Changed by Wang Wenhu, Sep9th, 2014 */
			exp->mask.src.u.udp.port  = 0;
		}

		DEBUGP("expect_related %u.%u.%u.%u:%u-%u.%u.%u.%u:%u,src mask:%08x\n",
		       NIPQUAD(exp->tuple.src.u3.ip),
		       ntohs(exp->tuple.src.u.udp.port),
		       NIPQUAD(exp->tuple.dst.u3.ip),
		       ntohs(exp->tuple.dst.u.udp.port),
		       ntohs(exp->mask.src.u.udp.port));

		if (nf_nat_rtsp_hook){
			/* pass the request off to the nat helper */
			ret = nf_nat_rtsp_hook(skb, ctinfo, hdrsoff, hdrslen, &expinfo, exp, &sizeDiff);
		}
		else if (nf_ct_expect_related(exp) != 0) {
			INFOP("nf_ct_expect_related failed\n");
				ret = NF_DROP;
			}

		nf_ct_expect_put(exp);
	}

	/* change the TCP seq if the packet's size is diffirent from the old one */
	if(0 != sizeDiff)
	{
		nf_nat_mangle_rtsp_seq(skb, ct, ctinfo, sizeDiff);
		sizeDiff = 0;
	}

out:

	return ret;
}

static inline int
help_response(struct sk_buff *skb, unsigned char *rb_ptr, unsigned int datalen,
                struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	struct ip_ct_rtsp_expect expinfo;

	char*   	pdata = rb_ptr;
	char*  		pTranData;
	char*   	pTemp;
	uint    	dataoff = 0;
	u_int16_t 	loPort = 0;
	u_int16_t 	hiPort = 0;
	uint    	numlen = 0;
	char        rbuf1[16];      /* Replacement buffer (one port) */
    uint        rbuf1len;       /* Replacement len (one port) */
    char        rbufa[16];      /* Replacement buffer (all ports) */
    uint        rbufalen;       /* Replacement len (all ports) */
    uint        origoff;
    uint        origlen;	
	int ret = NF_ACCEPT;

	u_int16_t divisor = 0;
	__be32 srcIp = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
	
	/*in the case that the media server is in the LAN subnet, there's no need to 
	transfer the client_port, modified by Chen Jianfeng, 2013.11.21*/
	if(isSameLanAddr(srcIp, lanDevName))
	{
		divisor = (u_int16_t)RTSP_CONVERT_IP_TO_PORT(srcIp);
	}

	memset(&expinfo, 0, sizeof(expinfo));

	while (dataoff < datalen) 
	{
		uint    cmdoff = dataoff;
		uint    hdrsoff = 0;
		uint    hdrslen = 0;
		uint    cseqoff = 0;
		uint    cseqlen = 0;
		uint    transoff = 0;
		uint    translen = 0;
		uint    off;

		if (!rtsp_parse_message(pdata, datalen, &dataoff,
					&hdrsoff, &hdrslen,
					&cseqoff, &cseqlen,
					&transoff, &translen))
			break;      /* not a valid message */
		
		if (strncmp(pdata+cmdoff, "RTSP/1.0 ", 9) != 0)
			continue;   /* not a SETUP message */
		DEBUGP("found a response message\n");

		if(translen) 
		{
			pTranData = pdata + transoff;
			if (translen < 10 || !iseol(pTranData[translen-1]) ||
	    		nf_strncasecmp(pTranData, "Transport:", 10) != 0)
	    	{
				INFOP("sanity check failed\n");
				return NF_ACCEPT;
		}

			DEBUGP("tran='%.*s'\n", (int)translen, pTranData);
			off += 10;
			SKIP_WSPACE(pTranData, translen, off);

			if((pTemp = strstr(pTranData, "client_port=")))
			{
				off = 12 + (pTemp - pTranData);
				origoff = transoff + off;
				numlen = nf_strtou16(pTranData+off, &loPort);
				off += numlen;
				origlen = numlen;
				
				loPort -= divisor;
 				
				rbuf1len = sprintf(rbuf1, "%hu", loPort);
				
				DEBUGP("the numlen is %d, using port %hu\n", numlen, loPort);
				if (pTranData[off]== '-')
				{			
					off++;
					origlen ++;
					numlen = nf_strtou16(pTranData+off, &hiPort);
					off += numlen;
					origlen += numlen;
					hiPort -= divisor;
					rbufalen = sprintf(rbufa, "%hu-%hu", loPort, hiPort);
					DEBUGP("using port %hu-%hu\n", loPort, hiPort);					
	
				} 
				else if (pTranData[off] == '/')
				{
					off++;
					origlen ++;
					numlen = nf_strtou16(pTranData+off, &hiPort);	
					off += numlen;
					origlen += numlen;
					hiPort -= divisor;
					rbufalen = sprintf(rbufa, "%hu/%hu", loPort, hiPort);
					DEBUGP("using port %hu/%hu\n", loPort, hiPort);
				}

				if(0 == hiPort)
				{
					if(origlen >11 || rbuf1len > 11)
						return NF_ACCEPT;
					ret = nf_nat_mangle_tcp_packet(skb, ct, ctinfo, origoff, origlen, rbuf1, rbuf1len);
				}
				else
				{
					if(origlen >11 || rbufalen > 11)
						return NF_ACCEPT;
					ret = nf_nat_mangle_tcp_packet(skb, ct, ctinfo, origoff, origlen, rbufa, rbufalen);
				}
				}
		}
	}
	return ret;
}

int isSameLanAddr(unsigned int ip, char* devName)
{
	struct net_device *lan_dev = dev_get_by_name(&init_net, devName);
    struct in_device *in_dev = NULL;
    struct in_ifaddr *ifa = NULL;

    unsigned int lanIpaddr = 0;
	unsigned int lanMask = 0;


    if (NULL == lan_dev)
		{
        return 0;
		}
			
    in_dev = (struct in_device*)lan_dev->ip_ptr;    
	if (in_dev) 
    {
        ifa = in_dev->ifa_list;
        if (ifa)
        {
            lanIpaddr = ifa->ifa_local;
			lanMask = ifa->ifa_mask;
        }
	}
	dev_put(lan_dev);
	
    if((ip & lanMask) == (lanIpaddr & lanMask))
    {	
		return 1;	
	}
	else
	{	
		return 0;
	}
}

void longing_future(struct nf_conn *ct, struct nf_conntrack_expect *exp)
{
	struct nf_nat_range range;
	int dir = IP_CT_DIR_REPLY;
//	int ret;

	/* This must be a fresh one. */
	BUG_ON(ct->status & IPS_NAT_DONE_MASK);
	
	/* Change src to where master sends to */
	range.flags = IP_NAT_RANGE_MAP_IPS;
	range.min_ip = range.max_ip
		= ct->master->tuplehash[dir].tuple.dst.u3.ip;
	nf_nat_setup_info(ct, &range, IP_NAT_MANIP_SRC);

	/* For DST manip, map port here to where it's expected. */
	range.flags = (IP_NAT_RANGE_MAP_IPS | IP_NAT_RANGE_PROTO_SPECIFIED);
	range.min = range.max = exp->saved_proto;
	range.min_ip = range.max_ip
		= ct->master->tuplehash[dir].tuple.src.u3.ip;
	nf_nat_setup_info(ct, &range, IP_NAT_MANIP_DST);
}

static inline int
help_in(struct sk_buff *skb, unsigned char *rb_ptr, unsigned int datalen,
 struct nf_conn* ct, enum ip_conntrack_info ctinfo)
{
	struct ip_ct_rtsp_expect expinfo;

	int dir = CTINFO2DIR(ctinfo);   /* = IP_CT_DIR_ORIGINAL */
	char*   pdata = rb_ptr;
	uint    dataoff = 0;
	int ret = NF_ACCEPT;

	char*	transHead;
	int		server_port = 0;

	struct nf_conntrack_expect *exp;

	__be16 be_server_port;

	/* Added by Wang Wenhu, Sep9th,2014
	 *
	 * If there is a "Location" header in the reply
	 * packet, we have to let the data transmitted by
	 * the server given by the "Location" header pass.
	 * So, we build a new expected item here is needed.
	 */
	char*   locatHead;
	char    locatstr[16];
	__be32  srcs_addr   = 0;
	__be16  be_client_port;

	int ccount = 0;
	
	struct nf_conntrack_expect*  exp_link;

	memset(&expinfo, 0, sizeof(expinfo));

	while (dataoff < datalen) {
		uint    cmdoff = dataoff;
		uint    hdrsoff = 0;
		uint    hdrslen = 0;
		uint    cseqoff = 0;
		uint    cseqlen = 0;
		uint    transoff = 0;
		uint    translen = 0;
		uint    locatoff = 0;
		uint    locatlen = 0;

		if (!rtsp_parse_in_message(pdata, datalen, &dataoff,
					&hdrsoff, &hdrslen,
					&cseqoff, &cseqlen,
					&transoff, &translen,
					&locatoff, &locatlen))
			break;      /* not a valid message */

		if(locatlen) 
		{
	    	if(nf_strncasecmp(pdata+locatoff, "Location:", 9) == 0) 
			{
				DEBUGP("Location find\n");
 				locatHead = strstr(pdata+locatoff, "rtsp://");
				if (locatHead == NULL)
					break;
				locatHead+=7;
				memset(locatstr, '\0', 16);
				ccount = 0;
				while(*(locatHead + ccount) != ':')
					ccount++;
				memcpy(locatstr, locatHead, ccount);
				srcs_addr = in_aton(locatstr);
			}
			else
				continue;
		}
		else
		{
			DEBUGP("_____No need to create expection_____\n");
			continue;
		}

		if(translen) {
			DEBUGP("_____Parse transport_____\n");
			rtsp_parse_transport(pdata+transoff, translen, &expinfo, ct, dir);
		}

		if (expinfo.loport == 0) {
			DEBUGP("no udp transports found\n");
			break;   /* no udp transports found */
		}

		
		DEBUGP("udp transport found, ports=(%d,%hu,%hu)\n",
		       (int)expinfo.pbtype, expinfo.loport, expinfo.hiport);

		exp_link = nf_ct_expect_alloc(ct);
		if (!exp_link) {
			ret = NF_DROP;
			goto out;
		}

		be_client_port = htons(expinfo.loport);

		if(srcs_addr)
		{
			nf_ct_expect_init(exp_link, NF_CT_EXPECT_CLASS_DEFAULT,
				ct->tuplehash[dir].tuple.src.l3num,
				&srcs_addr, &ct->tuplehash[dir].tuple.dst.u3,
				IPPROTO_UDP, NULL, &be_client_port);
		}
		else
		{
			nf_ct_expect_init(exp_link, NF_CT_EXPECT_CLASS_DEFAULT,
				ct->tuplehash[dir].tuple.src.l3num,
				&ct->tuplehash[dir].tuple.src.u3, &ct->tuplehash[dir].tuple.dst.u3,
				IPPROTO_UDP, NULL, &be_client_port);
		}


		exp_link->master = ct;

		exp_link->expectfn = expected;
		exp_link->flags = 0;

		if (expinfo.pbtype == pb_range) {
			DEBUGP("Changing expectation mask to handle multiple ports\n");
			exp_link->mask.src.u.udp.port  = 0;
		}

		DEBUGP("expect_related %u.%u.%u.%u:%u-%u.%u.%u.%u:%u,src mask:%08x\n",
		       NIPQUAD(exp_link->tuple.src.u3.ip),
		       ntohs(exp_link->tuple.src.u.udp.port),
		       NIPQUAD(exp_link->tuple.dst.u3.ip),
		       ntohs(exp_link->tuple.dst.u.udp.port),
		       ntohs(exp_link->mask.src.u.udp.port));

		if (nf_ct_expect_related(exp_link) != 0) {
			INFOP("nf_ct_expect_related failed\n");
		}
		
		nf_ct_expect_put(exp_link);
	}

	pdata = rb_ptr;
	dataoff = 0;
	ret = NF_ACCEPT;

	server_port = 0;

	/*End adding */

	memset(&expinfo, 0, sizeof(expinfo));

	while (dataoff < datalen) {
		uint    hdrsoff = 0;
		uint    hdrslen = 0;
		uint    cseqoff = 0;
		uint    cseqlen = 0;
		uint    transoff = 0;
		uint    translen = 0;

		if (!rtsp_parse_message(pdata, datalen, &dataoff,
					&hdrsoff, &hdrslen,
					&cseqoff, &cseqlen,
					&transoff, &translen))
			break;      /* not a valid message */

		if(translen) 
		{
	    	if(nf_strncasecmp(pdata+transoff, "Transport:", 10) != 0) 
			{
				DEBUGP("sanity check failed\n");
 				return NF_ACCEPT;
			}
			transHead = strstr(pdata+transoff, "server_port=");
			if (transHead == NULL)
				continue;
			transHead+=12;
			server_port = simple_strtoul(transHead, NULL, 10);
			expinfo.loport = server_port;
		}

		if (server_port == 0) 
		{
			DEBUGP("no udp transports found\n");
			continue;   /* no udp transports found */
		}
		DEBUGP("server_port = %d.\n",server_port);
		
		exp = nf_ct_expect_alloc(ct);
		if (!exp) {
			ret = NF_DROP;
			goto out;
		}

		be_server_port = htons(server_port);

		nf_ct_expect_init(exp, NF_CT_EXPECT_CLASS_DEFAULT,
			ct->tuplehash[!dir].tuple.src.l3num,
			&ct->tuplehash[!dir].tuple.src.u3, &ct->tuplehash[!dir].tuple.dst.u3,
			IPPROTO_UDP, NULL, &be_server_port);

		exp->master = ct;

		exp->expectfn = longing_future;
		exp->flags = 0;

		/*!< 开放端口，接收UDP包  by Fu Wenzheng 16Sep11 */
		exp->mask.src.u.udp.port = 0;
		
		exp->saved_proto.udp.port = be_server_port;
		
		DEBUGP("expect_related %u.%u.%u.%u:%u-%u.%u.%u.%u:%u\n",
		       NIPQUAD(exp->tuple.src.u3.ip),
		       ntohs(exp->tuple.src.u.udp.port),
		       NIPQUAD(exp->tuple.dst.u3.ip),
		       ntohs(exp->tuple.dst.u.udp.port));


		if (nf_ct_expect_related(exp) != 0) 
		{
			DEBUGP("nf_ct_expect_related failed\n");
			ret  = NF_DROP;
		}

		nf_ct_expect_put(exp);
		goto out;
	}
out:

	return ret;
}

static int help(struct sk_buff *skb, unsigned int protoff,
		struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	struct tcphdr _tcph, *th;
	unsigned int dataoff, datalen;
	char *rb_ptr;
	int ret = NF_DROP;


	/* Until there's been traffic both ways, don't look in packets. */
	if (ctinfo != IP_CT_ESTABLISHED &&
	    ctinfo != IP_CT_ESTABLISHED + IP_CT_IS_REPLY) {
		DEBUGP("conntrackinfo = %u\n", ctinfo);
		return NF_ACCEPT;
	}

	/* Not whole TCP header? */
	th = skb_header_pointer(skb, protoff, sizeof(_tcph), &_tcph);

	if (!th)
		return NF_ACCEPT;

	/* No data ? */
	dataoff = protoff + th->doff*4;
	datalen = skb->len - dataoff;
	if (dataoff >= skb->len)
		return NF_ACCEPT;

	spin_lock_bh(&rtsp_buffer_lock);
	rb_ptr = skb_header_pointer(skb, dataoff,
				    skb->len - dataoff, rtsp_buffer);
	BUG_ON(rb_ptr == NULL);

#if 0
	/* Checksum invalid?  Ignore. */
	/* FIXME: Source route IP option packets --RR */
	if (tcp_v4_check(tcph, tcplen, iph->saddr, iph->daddr,
			 csum_partial((char*)tcph, tcplen, 0)))
	{
		DEBUGP("bad csum: %p %u %u.%u.%u.%u %u.%u.%u.%u\n",
		       tcph, tcplen, NIPQUAD(iph->saddr), NIPQUAD(iph->daddr));
		return NF_ACCEPT;
	}
#endif

	switch (CTINFO2DIR(ctinfo)) {
	case IP_CT_DIR_ORIGINAL:
		DEBUGP("IP_CT_DIR_ORIGINAL");
		ret = help_out(skb, rb_ptr, datalen, ct, ctinfo);
		break;
	case IP_CT_DIR_REPLY:
		DEBUGP("IP_CT_DIR_REPLY\n");
		/*
		 *	Bug 13628 - 【TL-R600VPN_1.0】进行RTSP ALG测试时，从wan口进来的RTP数据包不能正常穿透
		 *	
		 *	Bug说明: 对服务器的返回包中server_port没有作解析。
		 *	
		 *	解决办法: 解析服务器返回包中的server_port进行解析并建立对应的nat条目。
		 *	
		 *	By Fu Wenzheng, 28Sep11
		 */	
		ret = help_in(skb, rb_ptr, datalen, ct, ctinfo);
		if (NF_ACCEPT == ret)
		{
		    ret = help_response(skb, rb_ptr, datalen, ct, ctinfo);
		}

		/* inbound packet: server->client */
		break;
	}

	spin_unlock_bh(&rtsp_buffer_lock);

	return ret;
}

static struct nf_conntrack_helper rtsp_helpers[MAX_PORTS];
static char rtsp_names[MAX_PORTS][10];
static struct nf_conntrack_expect_policy rtsp_expect_policy;

/* This function is intentionally _NOT_ defined as __exit */
static void
fini(void)
{
	int i;
	for (i = 0; i < num_ports; i++) {
		DEBUGP("unregistering port %d\n", ports[i]);
		nf_conntrack_helper_unregister(&rtsp_helpers[i]);
	}
	kfree(rtsp_buffer);
}

static int __init
init(void)
{
	int i, ret;
	struct nf_conntrack_helper *hlpr;
	char *tmpname;

	DEBUGP("nf_conntrack_rtsp v" IP_NF_RTSP_VERSION " loading\n");

	if (max_outstanding < 1) {
		DEBUGP("nf_conntrack_rtsp: max_outstanding must be a positive integer\n");
		return -EBUSY;
	}
	if (setup_timeout < 0) {
		DEBUGP("nf_conntrack_rtsp: setup_timeout must be a positive integer\n");
		return -EBUSY;
	}

	rtsp_expect_policy.max_expected = max_outstanding;
	rtsp_expect_policy.timeout = setup_timeout;

	rtsp_buffer = kmalloc(65536, GFP_KERNEL);
	if (!rtsp_buffer)
		return -ENOMEM;

	/* If no port given, default to standard rtsp port */
	if (ports[0] == 0) {
		ports[0] = RTSP_PORT;
	}

	for (i = 0; (i < MAX_PORTS) && ports[i]; i++) {
		hlpr = &rtsp_helpers[i];
		memset(hlpr, 0, sizeof(struct nf_conntrack_helper));
		/*
		 *	Bug 13628 - 进行RTSP ALG测试时，从wan口进来的RTP数据包不能正常穿透
		 *	
		 *	Bug说明: 未指定协议族，导致HASH值不能匹配。
		 *	
		 *	解决办法: 指定协议族
		 *	
		 *	By Fu Wenzheng, 16Sep11
		 */
		/*!< 增加协议族，匹配hash值，  by Fu Wenzheng 16Sep11 */
		hlpr->tuple.src.l3num = AF_INET;
		hlpr->tuple.src.u.tcp.port = htons(ports[i]);
		hlpr->tuple.dst.protonum = IPPROTO_TCP;
		hlpr->expect_policy = &rtsp_expect_policy;
		hlpr->me = THIS_MODULE;
		hlpr->help = help;

		tmpname = &rtsp_names[i][0];
		if (ports[i] == RTSP_PORT) {
			sprintf(tmpname, "rtsp");
		} else {
			sprintf(tmpname, "rtsp-%d", i);
		}
		hlpr->name = tmpname;

		DEBUGP("port #%d: %d\n", i, ports[i]);

		ret = nf_conntrack_helper_register(hlpr);

		if (ret) {
			printk("nf_conntrack_rtsp: ERROR registering port %d\n", ports[i]);
			fini();
			return -EBUSY;
		}
		num_ports++;
	}
	return 0;
}

module_init(init);
module_exit(fini);

EXPORT_SYMBOL(nf_nat_rtsp_hook_expectfn);

