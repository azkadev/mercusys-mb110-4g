
KBUILD_CFLAGS += -I$(srctree)

obj-$(CONFIG_RALINK_I2S)  += ralink_i2s.o

ralink_i2s-objs       		:= i2s_ctrl.o i2s_debug.o


ifeq ($(CONFIG_I2S_WITH_AEC),m)
EXTRA_CFLAGS += -DCONFIG_I2S_WITH_AEC
endif


ifeq ($(CONFIG_RALINK_RT3052),y)
ralink_i2s-objs += wm8751.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8751
endif

ifeq ($(CONFIG_RALINK_RT2883),y)
ralink_i2s-objs += wm8751.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8751
endif

ifeq ($(CONFIG_RALINK_RT3350),y)
ralink_i2s-objs += wm8751.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8751
endif

ifeq ($(CONFIG_RALINK_RT3883),y)
ralink_i2s-objs += wm8750.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8750
endif

ifeq ($(CONFIG_RALINK_RT3352),y)
ralink_i2s-objs += wm8750.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8750
endif

ifeq ($(CONFIG_RALINK_RT5350),y)
ifeq ($(CONFIG_I2S_WM8960), y)
ralink_i2s-objs += wm8960.o
endif
ifeq ($(CONFIG_I2S_WM8750), y)
ralink_i2s-objs += wm8750.o
endif
endif

ifeq ($(CONFIG_RALINK_RT6855),y)
ralink_i2s-objs += wm8750.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8750
endif

ifeq ($(CONFIG_RALINK_RT6855A),y)
ralink_i2s-objs += wm8750.o
#EXTRA_CFLAGS   +=-DCONFIG_I2S_WM8750
endif

ifeq ($(CONFIG_RALINK_MT7620),y)
ifeq ($(CONFIG_I2S_WM8960), y)
ralink_i2s-objs += wm8960.o
endif
ifeq ($(CONFIG_I2S_WM8750), y)
ralink_i2s-objs += wm8750.o
endif
endif

ifeq ($(CONFIG_RALINK_MT7621),y)
ifeq ($(CONFIG_I2S_WM8960), y)
ralink_i2s-objs += wm8960.o
endif
ifeq ($(CONFIG_I2S_WM8750), y)
ralink_i2s-objs += wm8750.o
endif
endif

ifeq ($(CONFIG_RALINK_MT7628),y)
ifeq ($(CONFIG_I2S_WM8960), y)
ralink_i2s-objs += wm8960.o
endif
ifeq ($(CONFIG_I2S_WM8750), y)
ralink_i2s-objs += wm8750.o
endif
endif

clean:
	rm -rf ./*.mod.* ./*.ko ./*.o ./*.bak ./.*.cmd
	rm -fr modules.order
	rm -ff modules.builtin
