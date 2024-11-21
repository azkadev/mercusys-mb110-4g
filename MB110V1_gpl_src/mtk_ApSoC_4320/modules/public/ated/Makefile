NVDIR = $(shell pwd)/../../../linux-2.6.36.x/include
CFLAGS += -I$(NVDIR) 
#CC = /opt/buildroot-gcc463/usr/bin/mipsel-linux-gcc
#LDFLAGS += -lnvram
EXEC = ated
SRCS = ated.c

#EXTRA_CFLAGS += -DDBG

all: $(EXEC)

$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) -o $@ $@.c $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.o

