
KDIR ?= $(KERNELPATH)

obj-m += pptp.o
#obj-m += gre.o

default:
	make -C $(KDIR) LANG=C M=`pwd` modules

clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
