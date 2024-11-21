/*
 * RTSP extension for TCP NAT alteration
 * (C) 2003 by Tom Marshall <tmarshall at real.com>
 * based on ip_nat_irc.c
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Module load syntax:
 *      insmod nf_nat_rtsp.o ports=port1,port2,...port<MAX_PORTS>
 *                           stunaddr=<address>
 *                           destaction=[auto|strip|none]
 *
 * If no ports are specified, the default will be port 554 only.
 *
 * stunaddr specifies the address used to detect that a client is using STUN.
 * If this address is seen in the destination parameter, it is assumed that
 * the client has already punched a UDP hole in the firewall, so we don't
 * mangle the client_port.  If none is specified, it is autodetected.  It
 * only needs to be set if you have multiple levels of NAT.  It should be
 * set to the external address that the STUN clients detect.  Note that in
 * this case, it will not be possible for clients to use UDP with servers
 * between the NATs.
 *
 * If no destaction is specified, auto is used.
 *   destaction=auto:  strip destination parameter if it is not stunaddr.
 *   destaction=strip: always strip destination parameter (not recommended).
 *   destaction=none:  do not touch destination parameter (not recommended).
 */

#include <linux/module.h>
#include <net/tcp.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter/nf_conntrack_rtsp.h>
#include <net/netfilter/nf_conntrack_expect.h>

#include <linux/inet.h>
#include <linux/ctype.h>
#define NF_NEED_STRNCASECMP
#define NF_NEED_STRTOU16
#include <linux/netfilter_helpers.h>
#define NF_NEED_MIME_NEXTLINE
#include <linux/netfilter_mime.h>

#define INFOP(fmt, args...) printk(KERN_INFO "%s: %s: " fmt, __FILE__, __FUNCTION__ , ## args)
#if 0
#define DEBUGP(fmt, args...) printk(KERN_DEBUG "%s: %s: " fmt, __FILE__, __FUNCTION__ , ## args)
#else
#define DEBUGP(fmt, args...)
#endif

#define MAX_PORTS       8
#define DSTACT_AUTO     0
#define DSTACT_STRIP    1
#define DSTACT_NONE     2

static char*    stunaddr = NULL;
static char*    destaction = NULL;
static char*    lanDevName = "br0";

static u_int32_t extip = 0;
static int       dstact = 0;

MODULE_AUTHOR("Tom Marshall <tmarshall at real.com>");
MODULE_DESCRIPTION("RTSP network address translation module");
MODULE_LICENSE("GPL");
module_param(stunaddr, charp, 0644);
MODULE_PARM_DESC(stunaddr, "Address for detecting STUN");
module_param(destaction, charp, 0644);
MODULE_PARM_DESC(destaction, "Action for destination parameter (auto/strip/none)");
module_param(lanDevName, charp, 0400);
MODULE_PARM_DESC(lanDevName, "the LAN device name");


#define SKIP_WSPACE(ptr,len,off) while(off < len && isspace(*(ptr+off))) { off++; }

#define RTSP_CONVERT_IP_TO_PORT(ip)	\
	((((ip & 0x00ff0000) >> 16) + ((ip & 0x0000ff00) >> 8) + (ip & 0x000000ff)) * 8);

/*** helper functions ***/

int 
mangle_tran_buf(char * buf,
					unsigned int buflen,
					unsigned int diff_offset,
					unsigned int match_offset,
					unsigned int match_len,
					const char *rep_buffer,
					unsigned int rep_len)
{
	memmove(buf+match_offset+rep_len+diff_offset,  
		buf+match_offset+match_len+diff_offset,  buflen-match_offset-match_len);
	
	memcpy(buf+match_offset+diff_offset,  rep_buffer, rep_len);
	return 1;
}

static void
get_skb_tcpdata(struct sk_buff* skb, char** pptcpdata, uint* ptcpdatalen)
{
    struct iphdr*   iph  = ip_hdr(skb);
    struct tcphdr*  tcph = (void *)iph + ip_hdrlen(skb);

    *pptcpdata = (char*)tcph +  tcph->doff*4;
    *ptcpdatalen = ((char*)skb_transport_header(skb) + skb->len) - *pptcpdata;
}

/*** nat functions ***/

/*
 * Mangle the "Transport:" header:
 *   - Replace all occurences of "client_port=<spec>"
 *   - Handle destination parameter
 *
 * In:
 *   ct, ctinfo = conntrack context
 *   skb        = packet
 *   tranoff    = Transport header offset from TCP data
 *   tranlen    = Transport header length (incl. CRLF)
 *   rport_lo   = replacement low  port (host endian)
 *   rport_hi   = replacement high port (host endian)
 *
 * Returns packet size difference.
 *
 * Assumes that a complete transport header is present, ending with CR or LF
 */
static int
rtsp_mangle_tran(enum ip_conntrack_info ctinfo,
                 struct nf_conntrack_expect* exp,
		 		 struct ip_ct_rtsp_expect* prtspexp,
		 		 struct sk_buff* skb, uint tranoff, uint tranlen)
{
	char*  ptcp;
	uint   tcplen;
	char*  ptran;
	char   rbuf1[16];	  /* Replacement buffer (one port) */
	uint   rbuf1len;	  /* Replacement len (one port) */
	char   rbufa[16];	  /* Replacement buffer (all ports) */
	uint   rbufalen;	  /* Replacement len (all ports) */
	u_int32_t  newip;
	u_int16_t loport, hiport;
	uint	  off = 0;
	uint	  diff; 	   /* Number of bytes we removed */

    struct nf_conn *ct = exp->master;
    struct nf_conntrack_tuple *t;

	/* Add 2012-02-24  Rugemeng*/
	char tran_buf[2048];
	uint  tranbuf_len = tranlen;
	int tranbuf_diff = 0;
	int dir = CTINFO2DIR(ctinfo);

    char    szextaddr[15+1];
	uint extaddrlen;
	int  is_stun;

	get_skb_tcpdata(skb, &ptcp, &tcplen);
	ptran = ptcp+tranoff;

	if (tranoff+tranlen > tcplen || tcplen-tranoff < tranlen ||
		tranlen < 10 || !iseol(ptran[tranlen-1]) ||
        nf_strncasecmp(ptran, "Transport:", 10) != 0)
    {
        INFOP("sanity check failed\n");
		return 0;
	}
	
	/* Add 2012-02-24  Rugemeng*/
	memcpy(tran_buf, ptran, tranlen);
	
	off += 10;
	SKIP_WSPACE(ptcp+tranoff, tranlen, off);

	newip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip;
    t = &exp->tuple;
    t->dst.u3.ip = newip;
	
    extaddrlen = extip ? sprintf(szextaddr, "%u.%u.%u.%u", NIPQUAD(extip))
                       : sprintf(szextaddr, "%u.%u.%u.%u", NIPQUAD(newip));
	DEBUGP("stunaddr=%s (%s)\n", szextaddr, (extip?"forced":"auto"));

	rbuf1len = rbufalen = 0;
    switch (prtspexp->pbtype)
    {
	case pb_single:
        for (loport = prtspexp->loport; loport != 0; loport++) /* XXX: improper wrap? */
        {
            t->dst.u.udp.port = htons(loport);
            if (nf_ct_expect_related(exp) == 0)
            {
				DEBUGP("using port %hu\n", loport);
				break;
			}
		}
        if (loport != 0)
        {
			rbuf1len = sprintf(rbuf1, "%hu", loport);
			rbufalen = sprintf(rbufa, "%hu", loport);
		}
		break;
	case pb_range:
        for (loport = prtspexp->loport; loport != 0; loport += 2) /* XXX: improper wrap? */
        {
            t->dst.u.udp.port = htons(loport);
			DEBUGP("expect_related %u.%u.%u.%u:%u-%u.%u.%u.%u:%u,src mask:%08x\n",
		       NIPQUAD(exp->tuple.src.u3.ip),
		       ntohs(exp->tuple.src.u.udp.port),
		       NIPQUAD(exp->tuple.dst.u3.ip),
		       ntohs(exp->tuple.dst.u.udp.port),
		       ntohs(exp->mask.src.u.udp.port));
            if (nf_ct_expect_related(exp) == 0)
            {
                hiport = loport + 1;//~exp->mask.src.u.udp.port;
                DEBUGP("using ports %hu-%hu\n", loport, hiport);
                break;
			}
		}
        if (loport != 0)
        {
			rbuf1len = sprintf(rbuf1, "%hu", loport);
            rbufalen = sprintf(rbufa, "%hu-%hu", loport, loport+1);
		}
		break;
	case pb_discon:
        for (loport = prtspexp->loport; loport != 0; loport++) /* XXX: improper wrap? */
        {
            t->dst.u.udp.port = htons(loport);
            if (nf_ct_expect_related(exp) == 0)
            {
				DEBUGP("using port %hu (1 of 2)\n", loport);
				break;
			}
		}
        for (hiport = prtspexp->hiport; hiport != 0; hiport++) /* XXX: improper wrap? */
        {
            t->dst.u.udp.port = htons(hiport);
            if (nf_ct_expect_related(exp) == 0)
            {
				DEBUGP("using port %hu (2 of 2)\n", hiport);
				break;
			}
		}
        if (loport != 0 && hiport != 0)
        {
			rbuf1len = sprintf(rbuf1, "%hu", loport);
            if (hiport == loport+1)
            {
                rbufalen = sprintf(rbufa, "%hu-%hu", loport, hiport);
            }
            else
            {
                rbufalen = sprintf(rbufa, "%hu/%hu", loport, hiport);
            }
		}
		break;
	}

	if (rbuf1len == 0)
    {
		return 0;	/* cannot get replacement port(s) */
    }

    /* Transport: tran;field;field=val,tran;field;field=val,... */
    while (off < tranlen)
    {
		uint		saveoff;
		const char* pparamend;
		uint		nextparamoff;

		pparamend = memchr(ptran+off, ',', tranlen-off);
		pparamend = (pparamend == NULL) ? ptran+tranlen : pparamend+1;
		nextparamoff = pparamend-ptran;

		/*
		 * We pass over each param twice.  On the first pass, we look for a
		 * destination= field.	It is handled by the security policy.  If it
		 * is present, allowed, and equal to our external address, we assume
		 * that STUN is being used and we leave the client_port= field alone.
		 */
		is_stun = 0;
		saveoff = off;
        while (off < nextparamoff)/* replace all the content of destination field */
        {
			const char* pfieldend;
			uint		nextfieldoff;

			pfieldend = memchr(ptran+off, ';', nextparamoff-off);
			nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

            if (dstact != DSTACT_NONE && strncmp(ptran+off, "destination=", 12) == 0)
            {
				/* Add 2012-2-21 Rugemeng; modify the destination */
				char        temaddr[15+1];
				uint  	tmplen;
				u_int32_t   tmpip;

				if (strncmp(ptran+off+12, szextaddr, extaddrlen) == 0)
                {
					is_stun = 1;
                }

				if(dir == IP_CT_DIR_ORIGINAL)
				{
					tmpip = ct->tuplehash[!dir].tuple.dst.u3.ip;
				}
				else
				{
					tmpip = ct->tuplehash[!dir].tuple.src.u3.ip;
					}
				tmplen = sprintf(temaddr, "%u.%u.%u.%u", NIPQUAD(tmpip));

				diff = nextfieldoff-off;
				//ip_nat_mangle_tcp_packet(pskb, ct, ctinfo, (ptran-ptcp)+off+12, diff-12-1,  temaddr, tmplen);
				mangle_tran_buf(tran_buf, tranbuf_len, tranbuf_diff, 
									off+12, diff-12-1, temaddr, tmplen);
				tranbuf_diff = tranbuf_diff + tmplen - (diff -12 -1);

				/* end add */
			}

			off = nextfieldoff;
		}
		if (is_stun)
        {
			continue;
        }
		off = saveoff;
        while (off < nextparamoff)
        {
			const char* pfieldend;
			uint		nextfieldoff;

			pfieldend = memchr(ptran+off, ';', nextparamoff-off);
			nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

            if (strncmp(ptran+off, "client_port=", 12) == 0)
            {
				u_int16_t port;
				uint	  numlen;
				uint	  origoff;
				uint	  origlen;
				char*	  rbuf = rbuf1;
				uint	  rbuflen = rbuf1len;

				off += 12;
                //origoff = (ptran-ptcp)+off;
                origoff = off;
				origlen = 0;
				numlen = nf_strtou16(ptran+off, &port);
				off += numlen;
				origlen += numlen;

				/*
				if (port != prtspexp->loport)
                 {
					DEBUGP("multiple ports found, port %hu ignored\n", port);
                 }
                else
				*/
                {
                    if (ptran[off] == '-' || ptran[off] == '/')
                    {
						off++;
						origlen++;
						numlen = nf_strtou16(ptran+off, &port);
						off += numlen;
						origlen += numlen;
						rbuf = rbufa;
						rbuflen = rbufalen;
					}

					/*
					 * note we cannot just memcpy() if the sizes are the same.
					 * the mangle function does skb resizing, checks for a
					 * cloned skb, and updates the checksums.
					 *
					 * parameter 4 below is offset from start of tcp data.
					 */
					diff = origlen-rbuflen;

					/* Add 2012-02-24  Rugemeng*/
					mangle_tran_buf(tran_buf, tranbuf_len,  tranbuf_diff, 
											origoff, origlen, rbuf, rbuflen);
					tranbuf_diff = tranbuf_diff + rbuflen - origlen;
			
					get_skb_tcpdata(skb, &ptcp, &tcplen);
					ptran = ptcp+tranoff;
					tranlen -= diff;
					nextparamoff -= diff;
					nextfieldoff -= diff;
				}
			}

			off = nextfieldoff;
		}

		off = nextparamoff;
	}

	nf_nat_mangle_rtsp_packet(skb, ct, ctinfo, tranoff, tranbuf_len, 
								tran_buf, tranbuf_len + tranbuf_diff);

	return 1;
}

static uint
help_out(struct sk_buff *skb, enum ip_conntrack_info ctinfo,
	 unsigned int matchoff, unsigned int matchlen, struct ip_ct_rtsp_expect* prtspexp,
	 struct nf_conntrack_expect* exp,
	 int *sizeDiff)
{
	char* ptcp;
	uint  tcplen;
	uint  hdrsoff;
	uint  hdrslen;
	uint  lineoff;
	uint  linelen;
	uint  off;

    //struct iphdr* iph = (struct iphdr*)skb->nh.iph;
	//struct tcphdr* tcph = (struct tcphdr*)((void*)iph + iph->ihl*4);

	get_skb_tcpdata(skb, &ptcp, &tcplen);
	hdrsoff = matchoff;//exp->seq - ntohl(tcph->seq);
	hdrslen = matchlen;
	off = hdrsoff;
	DEBUGP("NAT rtsp help_out\n");

    while (nf_mime_nextline(ptcp, hdrsoff+hdrslen, &off, &lineoff, &linelen))
    {
		if (linelen == 0)
        {
			break;
        }
        if (off > hdrsoff+hdrslen)
        {
            INFOP("!! overrun !!");
			break;
		}
		DEBUGP("hdr: len=%u, %.*s", linelen, (int)linelen, ptcp+lineoff);

        if (nf_strncasecmp(ptcp+lineoff, "Transport:", 10) == 0)
        {
			uint oldtcplen = tcplen;
			DEBUGP("hdr: Transport\n");
            if (!rtsp_mangle_tran(ctinfo, exp, prtspexp, skb, lineoff, linelen))
            {
				DEBUGP("hdr: Transport mangle failed");
				break;
			}
			get_skb_tcpdata(skb, &ptcp, &tcplen);
			hdrslen -= (oldtcplen-tcplen);
			off -= (oldtcplen-tcplen);
			lineoff -= (oldtcplen-tcplen);
			linelen -= (oldtcplen-tcplen);
			*sizeDiff += tcplen - oldtcplen;
			DEBUGP("rep: len=%u, %.*s", linelen, (int)linelen, ptcp+lineoff);
		}
	}

	return NF_ACCEPT;
}

static unsigned int
help(struct sk_buff *skb, enum ip_conntrack_info ctinfo,
     unsigned int matchoff, unsigned int matchlen, struct ip_ct_rtsp_expect* prtspexp,
     struct nf_conntrack_expect* exp,
	 int *sizeDiff)
{
    int dir = CTINFO2DIR(ctinfo);
    int rc = NF_ACCEPT;

    switch (dir)
    {
    case IP_CT_DIR_ORIGINAL:
		rc = help_out(skb, ctinfo, matchoff, matchlen, prtspexp, exp, sizeDiff);

        break;
    case IP_CT_DIR_REPLY:
	DEBUGP("unmangle ! %u\n", ctinfo);
    	/* XXX: unmangle */
	rc = NF_ACCEPT;
        break;
    }
    //UNLOCK_BH(&ip_rtsp_lock);

    return rc;
}

static void expected(struct nf_conn* ct, struct nf_conntrack_expect *exp)
{
    struct nf_nat_multi_range_compat mr;
    u_int32_t newdstip, newsrcip, newip;

    struct nf_conn *master = ct->master;

	u_int16_t divisor = 0;

    newdstip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
    newsrcip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
    //FIXME (how to port that ?)
    //code from 2.4 : newip = (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) ? newsrcip : newdstip;
    newip = newdstip;

	/*	
	in the case that the media server is in the LAN subnet, there's no need to 
	transfer the client_port, modified by Chen Jianfeng, 2013.11.21
	*/
	if(isSameLanAddr(newdstip, lanDevName))
	{
		divisor = (u_int16_t)RTSP_CONVERT_IP_TO_PORT(newdstip);
	}

	DEBUGP("divisor=%u, ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all=%u \n",
           divisor, ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all);

	ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all = htons(htons(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all) - divisor);

    DEBUGP("newsrcip=%u.%u.%u.%u, newdstip=%u.%u.%u.%u, newip=%u.%u.%u.%u\n",
           NIPQUAD(newsrcip), NIPQUAD(newdstip), NIPQUAD(newip));
	DEBUGP("divisor=%u, ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all=%u \n",
           divisor, ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all);

    mr.rangesize = 1;
    // We don't want to manip the per-protocol, just the IPs.
    mr.range[0].flags = IP_NAT_RANGE_MAP_IPS;
    mr.range[0].min_ip = mr.range[0].max_ip = newip;

    nf_nat_setup_info(ct, &mr.range[0], IP_NAT_MANIP_DST);
}


static void __exit fini(void)
{
	nf_nat_rtsp_hook = NULL;
       nf_nat_rtsp_hook_expectfn = NULL;
	synchronize_net();
}

static int __init init(void)
{
	printk("nf_nat_rtsp v" IP_NF_RTSP_VERSION " loading\n");

	BUG_ON(nf_nat_rtsp_hook);
	nf_nat_rtsp_hook = help;
	   nf_nat_rtsp_hook_expectfn = &expected; 

	if (stunaddr != NULL)
		extip = in_aton(stunaddr);

	if (destaction != NULL) {
	        if (strcmp(destaction, "auto") == 0)
			dstact = DSTACT_AUTO;

		if (strcmp(destaction, "strip") == 0)
			dstact = DSTACT_STRIP;

		if (strcmp(destaction, "none") == 0)
			dstact = DSTACT_NONE;
	}

	return 0;
}

module_init(init);
module_exit(fini);
