#
# pppd makefile for Linux
# $Id: Makefile,v 1.1.1.1.2.1 2011/01/11 06:13:21 shnwind Exp $
#

# Default installation locations
DESTDIR = $(INSTROOT)/usr/local
BINDIR = $(DESTDIR)/sbin
MANDIR = $(DESTDIR)/share/man/man8
INCDIR = $(DESTDIR)/include

TARGETS = pppd

PPPDSRCS = main.c magic.c fsm.c lcp.c ipcp.c upap.c chap-new.c md5.c ccp.c \
	   ecp.c ipxcp.c auth.c options.c sys-linux.c md4.c chap_ms.c \
	   demand.c utils.c tty.c eap.c chap-md5.c session.c

HEADERS = ccp.h session.h chap-new.h ecp.h fsm.h ipcp.h \
	ipxcp.h lcp.h magic.h md5.h patchlevel.h pathnames.h pppd.h \
	upap.h eap.h

MANPAGES = pppd.8
PPPDOBJS = main.o magic.o fsm.o lcp.o ipcp.o upap.o chap-new.o md5.o ccp.o \
	   ecp.o auth.o options.o demand.o utils.o sys-linux.o ipxcp.o tty.o \
	   eap.o chap-md5.o session.o

#
# include dependencies if present
ifeq (.depend,$(wildcard .depend))
include .depend
endif

#CC = $(TOOLPREFIX)gcc
#STRIP = $(TOOLPREFIX)strip

# CC = gcc
#
#COPTS = -O2 -pipe -Wall -g
COPTS = -Os -pipe -Wall

LIBS =



# Uncomment the next 2 lines to include support for Microsoft's
# MS-CHAP authentication protocol.  Also, edit plugins/radius/Makefile.linux.
CHAPMS=y
USE_CRYPT=y
# Don't use MSLANMAN unless you really know what you're doing.
#MSLANMAN=y
# Uncomment the next line to include support for MPPE.  CHAPMS (above) must
# also be enabled.  Also, edit plugins/radius/Makefile.linux.
#MPPE=y

# Uncomment the next line to include support for PPP packet filtering.
# This requires that the libpcap library and headers be installed
# and that the kernel driver support PPP packet filtering.
#FILTER=y

# Uncomment the next line to enable multilink PPP (enabled by default)
# Linux distributions: Please leave multilink ENABLED in your builds
# of pppd!
#HAVE_MULTILINK=y

# Uncomment the next line to enable the TDB database (enabled by default.)
# If you enable multilink, then TDB is automatically enabled also.
# Linux distributions: Please leave TDB ENABLED in your builds.
#USE_TDB=y

#HAS_SHADOW=y
#USE_PAM=y

# Change by HYY: enable ipv6
#ifneq ($(strip $(TCSUPPORT_IPV6)),)
#ifeq ($(INCLUDE_IPV6_PPPV6), y)
HAVE_INET6=y
#endif

# Enable plugins
PLUGIN=y

# Enable Microsoft proprietary Callback Control Protocol
#CBCP=y

# Enable EAP SRP-SHA1 authentication (requires libsrp)
#USE_SRP=y

MAXOCTETS=n

#added CMM_APPS_PATH by YangCaiyong for cmm include header files.
INCLUDE_DIRS= -I../include -I$(OS_LIB_PATH)/include
LDFLAGS += -L$(OS_LIB_PATH)/ -los -lpthread -lrt


# add by yangxv
PPPOEOBJS = discovery.o if.o common.o debug.o plugin.o
PPPOERCS  = discovery.c if.c common.c debug.c plugin.c
CFLAGSPPPOE = -O2 -fomit-frame-pointer  -I ../include/linux 
CFLAGSPPPOE += $(INCLUDE_DIRS)


# add by yangcaiyong for PPPoA, 2011-11-17 
PPPOAOBJS = ans.o misc.o pppoatm.o text2atm.o text2qos.o
PPPOARCS  = ans.c misc.c pppoatm.c text2atm.c text2qos.c
CFLAGSPPPOA = -O2 -fomit-frame-pointer  -I ../include/linux -I ./ $(INCLUDE_DIR)
CFLAGSPPPOA += $(INCLUDE_DIRS)

# add by chz for PPPoL2TP, 2013-01-23 
PPPOL2TPOBJS = pppol2tp.o
PPPOL2TPRCS  = pppol2tp.c
CFLAGSPPPOL2TP = -O2 -fomit-frame-pointer  -I ../include/linux -I ./ $(INCLUDE_DIR)
CFLAGSPPPOL2TP += $(INCLUDE_DIRS)

# add by chz for PPPoPPTP, 2013-01-24
PPPOPPTPOBJS = dirutil.o orckit_quirks.o pptp.o pptp_callmgr.o pptp_ctrl.o pptp_quirks.o util.o vector.o
PPPOPPTPRCS  = dirutil.c orckit_quirks.c pptp.c pptp_callmgr.c pptp_ctrl.c pptp_quirks.c util.c vector.c
CFLAGSPPPOPPTP = -O2 -fomit-frame-pointer  -I ../include/linux -I ./ $(INCLUDE_DIR)
CFLAGSPPPOPPTP += $(INCLUDE_DIRS)

COMPILE_FLAGS= -DHAVE_PATHS_H -DIPX_CHANGE -DHAVE_MMAP

CFLAGS += $(COPTS) $(COMPILE_FLAGS) $(INCLUDE_DIRS) -D__LINUX_OS_FC__

ifdef CHAPMS
CFLAGS   += -DCHAPMS=1
NEEDDES=y
PPPDOBJS += md4.o chap_ms.o
HEADERS	+= md4.h chap_ms.h
ifdef MSLANMAN
CFLAGS   += -DMSLANMAN=1
endif
ifdef MPPE
CFLAGS   += -DMPPE=1
endif
endif

# EAP SRP-SHA1
ifdef USE_SRP
CFLAGS	+= -DUSE_SRP -DOPENSSL -I/usr/local/ssl/include
LIBS	+= -lsrp -L/usr/local/ssl/lib -lcrypto
TARGETS	+= srp-entry
EXTRAINSTALL = $(INSTALL) -s -c -m 555 srp-entry $(BINDIR)/srp-entry
MANPAGES += srp-entry.8
EXTRACLEAN += srp-entry.o
NEEDDES=y
else
# OpenSSL has an integrated version of SHA-1, and its implementation
# is incompatible with this local SHA-1 implementation.  We must use
# one or the other, not both.
PPPDSRCS += sha1.c
HEADERS += sha1.h
PPPDOBJS += sha1.o
endif

ifdef HAS_SHADOW
CFLAGS   += -DHAS_SHADOW
#LIBS     += -lshadow $(LIBS)
endif

ifneq ($(wildcard /usr/include/crypt.h),)
CFLAGS  += -DHAVE_CRYPT_H=1
LIBS	+= -lcrypt
endif

ifdef NEEDDES
ifndef USE_CRYPT
LIBS     += -ldes $(LIBS)
else
CFLAGS   += -DUSE_CRYPT=1
endif
PPPDOBJS += pppcrypt.o
HEADERS += pppcrypt.h
endif

# For "Pluggable Authentication Modules", see ftp.redhat.com:/pub/pam/.
ifdef USE_PAM
CFLAGS   += -DUSE_PAM
LIBS     += -lpam -ldl
endif

# Multi-linnk
ifdef HAVE_MULTILINK
	# Multilink implies the use of TDB
	USE_TDB=y

	CFLAGS += -DHAVE_MULTILINK
	PPPDSRCS += multilink.c
	PPPDOBJS += multilink.o
endif

# TDB
ifdef USE_TDB
	CFLAGS += -DUSE_TDB=1
	PPPDSRCS += tdb.c spinlock.c
	PPPDOBJS += tdb.o spinlock.o
	HEADERS += tdb.h spinlock.h
endif

# Lock library binary for Linux is included in 'linux' subdirectory.
ifdef LOCKLIB
LIBS     += -llock
CFLAGS   += -DLOCKLIB=1
endif

ifdef PLUGIN
CFLAGS	+= -DPLUGIN
LDFLAGS	+= -Wl,-E
LIBS	+= -ldl
endif

ifdef FILTER
ifneq ($(wildcard /usr/include/pcap-bpf.h),)
LIBS    += -lpcap
CFLAGS  += -DPPP_FILTER
endif
endif

ifdef HAVE_INET6
     PPPDSRCS += ipv6cp.c eui64.c
     HEADERS  += ipv6cp.h eui64.h
     PPPDOBJS += ipv6cp.o eui64.o
     CFLAGS   += -DINET6=1 -DINCLUDE_IPV6
endif

ifdef CBCP
     PPPDSRCS += cbcp.c
     PPPDOBJS += cbcp.o
     CFLAGS += -DCBCP_SUPPORT
     HEADERS += cbcp.h
endif

ifdef MAXOCTETS
     CFLAGS += -DMAXOCTETS
endif

#ifeq ($(TCSUPPORT_DNSEACHPVC),1)
     CFLAGS+= -DTCSUPPORT_DNSEACHPVC
#endif
ifneq ($(TC_CFLAGS), )
     CFLAGS+=$(TC_CFLAGS)
endif

INSTALL= install

all: $(TARGETS)
	$(STRIP) --strip-unneeded pppd
install: pppd
	mkdir -p $(BINDIR) $(MANDIR)
	$(EXTRAINSTALL)
	$(INSTALL) -s -c -m 555 pppd $(BINDIR)/pppd
	if chgrp pppusers $(BINDIR)/pppd 2>/dev/null; then \
	  chmod o-rx,u+s $(BINDIR)/pppd; fi
	$(INSTALL) -c -m 444 pppd.8 $(MANDIR)

pppd: $(PPPOEOBJS) $(PPPDOBJS) $(PPPOAOBJS) $(PPPOL2TPOBJS) $(PPPOPPTPOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o pppd $(PPPOEOBJS) $(PPPDOBJS) $(PPPOAOBJS) $(PPPOL2TPOBJS) $(PPPOPPTPOBJS) $(LIBS)

srp-entry:	srp-entry.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ srp-entry.c $(LIBS)

install-devel:
	mkdir -p $(INCDIR)/pppd
	$(INSTALL) -c -m 644 $(HEADERS) $(INCDIR)/pppd

clean:
	rm -f $(PPPDOBJS) $(EXTRACLEAN) $(TARGETS) $(PPPOEOBJS) $(PPPOAOBJS) $(PPPOL2TPOBJS) $(PPPOPPTPOBJS) *~ #* core

depend:
	$(CPP) -M $(CFLAGS) $(PPPDSRCS) >.depend

	
# add by yangxv
plugin.o:plugin.c
	$(CC) $(CFLAGSPPPOE) '-DRP_VERSION="3.8p"' -c plugin.c

debug.o:debug.c
	$(CC) $(CFLAGSPPPOE) '-DVERSION="3.8p"' -c debug.c

common.o:common.c
	$(CC) $(CFLAGSPPPOE) '-DVERSION="3.8p"' -c common.c

if.o:if.c
	$(CC) $(CFLAGSPPPOE) '-DVERSION="3.8p"' -c if.c

discovery.o:discovery.c
	$(CC) $(CFLAGSPPPOE) '-DVERSION="3.8p"' -c discovery.c


# add by yangcaiyong
ans.o:ans.c
	$(CC) $(CFLAGSPPPOA) '-DRP_VERSION="3.8p"' -c ans.c

misc.o:misc.c
	$(CC) $(CFLAGSPPPOA) '-DVERSION="3.8p"' -c misc.c

pppoatm.o:pppoatm.c
	$(CC) $(CFLAGSPPPOA) '-DVERSION="3.8p"' -c pppoatm.c

text2atm.o:text2atm.c
	$(CC) $(CFLAGSPPPOA) '-DVERSION="3.8p"' -c text2atm.c

text2qos.o:text2qos.c
	$(CC) $(CFLAGSPPPOA) '-DVERSION="3.8p"' -c text2qos.c

# add by chz
pppol2tp.o:pppol2tp.c
	$(CC) $(CFLAGSPPPOL2TP) '-DRP_VERSION="3.8p"' -c pppol2tp.c
	
pptp.o:pptp.c
	$(CC) $(CFLAGSPPPOPPTP) '-DRP_VERSION="3.8p"' -c pptp.c

dirutil.o:dirutil.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c dirutil.c

orckit_quirks.o:orckit_quirks.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c orckit_quirks.c
	
pptp_callmgr.o:pptp_callmgr.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c pptp_callmgr.c

	
pptp_ctrl.o:pptp_ctrl.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c pptp_ctrl.c
	
pptp_quirks.o:pptp_quirks.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c pptp_quirks.c
	
util.o:util.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c util.c
	
vector.o:vector.c
	$(CC) $(CFLAGSPPPOPPTP) '-DVERSION="3.8p"' -c vector.c




