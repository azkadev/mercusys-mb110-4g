################################################################################
#
#  Makefile Settings
#
################################################################################

PREFIX=./
IGD_SRC_PATH=./src
IGD_INC_PATH=./include



INCLUDES= -I$(IGD_INC_PATH) -I$(OS_LIB_PATH)/include -I../upnp_libs/inc
LIBS = -lpthread -lupnp -lixml -lthreadutil -lrt -los -L$(OS_LIB_PATH)/ -L../upnp_libs

LDFLAGS += $(LIBS)
CFLAGS += $(INCLUDES) -Wall -D_GNU_SOURCE -D__LINUX_OS_FC__ -fstrength-reduce -fomit-frame-pointer
CFLAGS += $(UPNP_FLAGS)

IGD_SRCS = $(IGD_SRC_PATH)/main.c $(IGD_SRC_PATH)/gatedevice.c $(IGD_SRC_PATH)/pmlist.c $(IGD_SRC_PATH)/util.c\
		   $(IGD_SRC_PATH)/config.c $(IGD_SRC_PATH)/strFile.c	

IGD_OBJS = $(IGD_SRCS:.c=.o)
IGD_BIN = upnpd

################################################################################
#
#  Makefile Build
#
################################################################################

ALL:$(IGD_BIN)
	
$(IGD_BIN):$(IGD_OBJS)
	$(CC) $(IGD_OBJS) -o $@ $(LDFLAGS) && $(STRIP) $@ && ls -l $@

sinclude $(IGD_SRCS:.c=.d)

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

.PHONY:copy
copy: upnpd
	cp upnpd /mnt/hgfs/F/MyProject/online_Debug/


.PHONY:clean
clean:
	$(RM) $(IGD_SRCS:.c=.d) $(IGD_SRCS:.c=.o) $(IGD_BIN)

################################################################################
#
#  Makefile others
#
################################################################################
