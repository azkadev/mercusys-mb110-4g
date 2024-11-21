# $Id: //WIFI_SOC/MP/SDK_4_3_0_0/RT288x_SDK/source/linux-2.6.36.x/arch/cris/kernel/Makefile#1 $
#
# Makefile for the linux kernel.
#

CPPFLAGS_vmlinux.lds := -DDRAM_VIRTUAL_BASE=0x$(CONFIG_ETRAX_DRAM_VIRTUAL_BASE)
extra-y	:= vmlinux.lds

obj-y   := process.o traps.o irq.o ptrace.o setup.o time.o sys_cris.o

obj-$(CONFIG_MODULES)    += crisksyms.o
obj-$(CONFIG_MODULES)	 += module.o
obj-$(CONFIG_SYSTEM_PROFILER)   += profile.o

clean:

