ifndef CFLAGS
CFLAGS = -O2 -Wall -g
endif

# If you want to support multiple radius server for multiple bssid, add following line
CFLAGS +=  -DMULTIPLE_RADIUS

# If you want to debug daemon, add following line
CFLAGS +=  -DDBG 

OBJS =	rtdot1x.o eloop.o eapol_sm.o radius.o md5.o  \
	config.o ieee802_1x.o  \
	sta_info.o   radius_client.o

all: rtdot1xd

rtdot1xd: $(OBJS)
	$(CC) -o rt2860apd $(OBJS)

clean:
	rm -f core *~ *.o rt2860apd *.d

#
# add by YYHuang@Ralink
romfs:
	$(ROMFSINST) rt2860apd /bin/rt2860apd
	ln -sf rt2860apd $(ROMFSDIR)/bin/rtinicapd

-include $(OBJS:%.o=%.d)
