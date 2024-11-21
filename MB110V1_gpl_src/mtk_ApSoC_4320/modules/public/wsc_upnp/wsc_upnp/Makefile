#############################################################
# Makefile for WSC upnp module
#############################################################

#CROSS_COMPILE = mips-openwrt-linux-uclibc-
TARGET_HOST = mips-linux
#CC = $(CROSS_COMPILE)gcc
#LD = $(CROSS_COMPILE)ld
#STRIP = $(CROSS_COMPILE)strip
#ifeq ($(KERNELPATH),)
#KERNELPATH= ../../../../../lantiq/build_dir/linux-ltqcpe_AC750/linux-2.6.32.42/
#endif
#KERNELPATH= ../../../../../lantiq/build_dir/linux-ltqcpe_AC750/linux-2.6.32.42/
#WIRELESS_H_INCLUDE_PATH = $(ROOTDIR)/linux-2.4.x/include/
#KERNELPATH=../../../mtk_ApSoC_4120/linux-2.6.36.x
WIRELESS_H_INCLUDE_PATH = $(KERNELPATH)/include/

PWD = $(shell pwd)

# Compile build-in libupnp-1.3.1
LIBUPNP_SRC_PATH = ./libupnp-1.3.1

LIBUPNP_LIB_PATH = $(PWD)/$(LIBUPNP_SRC_PATH)

INCLUDE_PATH += -I. -I$(LIBUPNP_SRC_PATH) -I$(LIBUPNP_SRC_PATH)/upnp/inc -I$(LIBUPNP_SRC_PATH)/upnp/src/inc
INCLUDE_PATH += -I$(LIBUPNP_SRC_PATH)/ixml/inc -I$(LIBUPNP_SRC_PATH)/ixml/src/inc  -I$(LIBUPNP_SRC_PATH)/threadutil/inc
INCLUDE_PATH += -I$(WIRELESS_H_INCLUDE_PATH)

#INCLUDE_LIBS := -static -L$(LIBUPNP_LIB_PATH)/upnp/.libs/ -lupnp -L$(LIBUPNP_LIB_PATH)/ixml/.libs/ -lixml -L$(LIBUPNP_LIB_PATH)/threadutil/.libs -lthreadutil -lpthread

# YY
# wscd .a version
#INCLUDE_LIBS := -L$(PWD) -lupnp -lixml -lthreadutil -lpthread
# wscd .so version
#INCLUDE_LIBS := -static -L$(LIB_PATH) -lupnp -lixml -lthreadutil -lpthread
INCLUDE_LIBS := -L$(PWD) -lupnp -lixml -lthreadutil -lpthread

OBJS = wsc_main.o wsc_common.o wsc_ioctl.o wsc_netlink.o wsc_upnp_device.o wsc_upnp_cp.o sample_util.o wsc_msg.o

EXEC = wscd

CFLAGS += -O2 -Wall -D_REENTRANT $(INCLUDE_PATH)
CFLAGS += -DRT_DEBUG
#CFLAGS += -DMULTIPLE_CARD_SUPPORT

all: $(EXEC)

# Compile build-in libupnp-1.3.1
$(LIBUPNP_SRC_PATH)/Makefile:
	make -C $(LIBUPNP_SRC_PATH) -f Makefile.src TARGET_HOST=$(TARGET_HOST)

#$(EXEC):  $(OBJS)
# Compile build-in libupnp-1.3.1
$(EXEC): libupnp $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)  $(INCLUDE_LIBS)
#	$(CC) $(CFLAGS) -o $@ $(OBJS)  $(INCLUDE_LIBS)
	$(STRIP) -s $(EXEC)

# Compile build-in libupnp-1.3.1
libupnp: $(LIBUPNP_SRC_PATH)/Makefile 
	make -C $(LIBUPNP_SRC_PATH)
	cp $(LIBUPNP_LIB_PATH)/upnp/.libs/libupnp.a .
	cp $(LIBUPNP_LIB_PATH)/ixml/.libs/libixml.a .
	cp $(LIBUPNP_LIB_PATH)/threadutil/.libs/libthreadutil.a .
#	cp $(LIBUPNP_LIB_PATH)/upnp/.libs/libupnp.so .
#	cp $(LIBUPNP_LIB_PATH)/ixml/.libs/libixml.so .
#	cp $(LIBUPNP_LIB_PATH)/threadutil/.libs/libthreadutil.so .

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o *~ *.bak $(EXEC)

cleanall: clean
	make -C $(LIBUPNP_SRC_PATH) clean
	rm -f *.a
	rm -f libupnp-1.3.1/Makefile

romfs:
	$(ROMFSINST) wscd /bin/wscd
	$(ROMFSINST) xml/WFADeviceDesc.xml /etc_ro/xml/WFADeviceDesc.xml
	$(ROMFSINST) xml/WFAWLANConfigSCPD.xml /etc_ro/xml/WFAWLANConfigSCPD.xml

