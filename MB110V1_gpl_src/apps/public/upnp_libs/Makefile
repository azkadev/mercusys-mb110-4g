################################################################################
#
#  Makefile Settings
#
################################################################################

UPNP_DEBUG ?= n

ifeq ($(strip $(RM)),)
RM = rm
endif

UPNP_INC = -I./inc 
UPNP_SRC = ./src
UPNP_LIB = ./libs

LDFLAGS += -lpthread -lrt
CFLAGS += $(UPNP_INC) -Wall -fstrength-reduce -fomit-frame-pointer -pthread -D_GNU_SOURCE -fPIC #-D_XOPEN_SOURCE=500


ifeq ($(UPNP_DEBUG), y)
CFLAGS += -DDEBUG
endif
ifeq ($(DEBUG), y)
LIBUPNP_SRCS += $(UPNP_SRC)/upnpdebug.c
endif
LIBUPNP_SRCS = $(UPNP_SRC)/upnpapi.c $(UPNP_SRC)/upnptools.c $(UPNP_SRC)/gena_callback2.c\
			   $(UPNP_SRC)/gena_device.c $(UPNP_SRC)/miniserver.c\
			   $(UPNP_SRC)/sock.c $(UPNP_SRC)/httpparser.c $(UPNP_SRC)/httpreadwrite.c $(UPNP_SRC)/parsetools.c\
			   $(UPNP_SRC)/statcodes.c $(UPNP_SRC)/webserver.c $(UPNP_SRC)/uri.c $(UPNP_SRC)/service_table.c\
			   $(UPNP_SRC)/membuffer.c $(UPNP_SRC)/strintmap.c $(UPNP_SRC)/upnp_timeout.c $(UPNP_SRC)/util.c\
			   $(UPNP_SRC)/soap_common.c $(UPNP_SRC)/soap_device.c\
			   $(UPNP_SRC)/ssdp_device.c $(UPNP_SRC)/ssdp_server.c $(UPNP_SRC)/urlconfig.c $(UPNP_SRC)/md5.c\
			   $(UPNP_SRC)/sysdep.c  $(UPNP_SRC)/uuid.c
LIBUPNP_OBJS = $(LIBUPNP_SRCS:.c=.o)
LIBUPNP_FOBJ = libupnp.so

LIBIXML_SRCS = $(UPNP_SRC)/attr.c $(UPNP_SRC)/document.c $(UPNP_SRC)/element.c $(UPNP_SRC)/ixml.c $(UPNP_SRC)/ixmlmembuf.c\
			   $(UPNP_SRC)/ixmlparser.c $(UPNP_SRC)/namedNodeMap.c $(UPNP_SRC)/node.c $(UPNP_SRC)/nodeList.c
LIBIXML_OBJS = $(LIBIXML_SRCS:.c=.o)
LIBIXML_FOBJ = libixml.so

LIBTHREADUTIL_SRCS = $(UPNP_SRC)/FreeList.c $(UPNP_SRC)/LinkedList.c $(UPNP_SRC)/ThreadPool.c $(UPNP_SRC)/TimerThread.c
LIBTHREADUTIL_OBJS = $(LIBTHREADUTIL_SRCS:.c=.o)
LIBTHREADUTIL_FOBJ = libthreadutil.so

################################################################################
#
#  Makefile Build
#
################################################################################

ALL:$(LIBIXML_FOBJ) $(LIBTHREADUTIL_FOBJ) $(LIBUPNP_FOBJ) 
	
$(LIBUPNP_FOBJ):$(LIBUPNP_OBJS)
	$(CC) $(CFLAGS) -shared -fPIC $(LIBUPNP_OBJS) -o $@
	$(STRIP) $@ 
	@cp $@ $(UPNP_LIB)

$(LIBIXML_FOBJ):$(LIBIXML_OBJS)
	test -d libs || mkdir libs
	$(CC) $(CFLAGS) -shared -fPIC $(LIBIXML_OBJS) -o $@
	$(STRIP) $@
	@cp $@ $(UPNP_LIB)

$(LIBTHREADUTIL_FOBJ):$(LIBTHREADUTIL_OBJS)
	$(CC) $(CFLAGS) -shared -fPIC $(LIBTHREADUTIL_OBJS) -o $@
	$(STRIP) $@
	@cp $@ $(UPNP_LIB)

ifeq ($(FULL_COMPILE), y)
sinclude $(LIBUPNP_SRCS:.c=.d)
sinclude $(LIBIXML_SRCS:.c=.d)
sinclude $(LIBTHREADUTIL_SRCS:.c=.d)
endif

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.d:%.c
	@$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[:]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

################################################################################
#
#  Makefile Clear, copy
#
################################################################################
.PHONY:clean
clean:
	$(RM) $(LIBUPNP_FOBJ) $(LIBIXML_FOBJ) $(LIBTHREADUTIL_FOBJ)\
		  $(LIBUPNP_OBJS) $(LIBIXML_OBJS) $(LIBTHREADUTIL_OBJS)\
		  $(LIBUPNP_SRCS:.c=.d) $(LIBIXML_SRCS:.c=.d) $(LIBTHREADUTIL_SRCS:.c=.d) $(UPNP_LIB)/* 
################################################################################
#
#  Makefile others
#
################################################################################
