#
# Makefile for USB Host Controller Drivers
#

ifeq ($(CONFIG_USB_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

ifdef CONFIG_USB_MT7621_XHCI_HCD
EXTRA_CFLAGS += -DCONFIG_USB_XHCI_PLATFORM
xhci-hcd-objs := mtk/xhci.o mtk/xhci-mem.o mtk/xhci-plat.o mtk/xhci-ring.o mtk/xhci-hub.o mtk/xhci-dbg.o mtk/mtk-phy.o mtk/xhci-mtk-scheduler.o mtk/xhci-mtk-power.o mtk/xhci-mtk.o mtk/mtk-phy-7621.o mtk/mtk-phy-ahb.o

obj-$(CONFIG_U3_PHY_GPIO_SUPPORT)	+= mtk/mtk-phy-gpio.o
obj-$(CONFIG_C60802_SUPPORT)		+= mtk/mtk-phy-c60802.o
obj-$(CONFIG_D60802_SUPPORT)		+= mtk/mtk-phy-d60802.o
else
xhci-hcd-objs := xhci.o xhci-mem.o xhci-pci.o xhci-ring.o xhci-hub.o xhci-dbg.o
endif

isp1760-objs := isp1760-hcd.o isp1760-if.o
fhci-objs := fhci-hcd.o fhci-hub.o fhci-q.o fhci-mem.o \
	     fhci-tds.o fhci-sched.o
ifeq ($(CONFIG_FHCI_DEBUG),y)
fhci-objs += fhci-dbg.o
endif

obj-$(CONFIG_USB_WHCI_HCD)	+= whci/

obj-$(CONFIG_PCI)		+= pci-quirks.o

obj-$(CONFIG_USB_EHCI_HCD)	+= ehci-hcd.o
obj-$(CONFIG_USB_OXU210HP_HCD)	+= oxu210hp-hcd.o
obj-$(CONFIG_USB_ISP116X_HCD)	+= isp116x-hcd.o
obj-$(CONFIG_USB_ISP1362_HCD)	+= isp1362-hcd.o
obj-$(CONFIG_USB_OHCI_HCD)	+= ohci-hcd.o
obj-$(CONFIG_USB_UHCI_HCD)	+= uhci-hcd.o
obj-$(CONFIG_USB_FHCI_HCD)	+= fhci.o
obj-$(CONFIG_USB_XHCI_HCD)	+= xhci-hcd.o
obj-$(CONFIG_USB_SL811_HCD)	+= sl811-hcd.o
obj-$(CONFIG_USB_SL811_CS)	+= sl811_cs.o
obj-$(CONFIG_USB_U132_HCD)	+= u132-hcd.o
obj-$(CONFIG_USB_R8A66597_HCD)	+= r8a66597-hcd.o
obj-$(CONFIG_USB_ISP1760_HCD)	+= isp1760.o
obj-$(CONFIG_USB_HWA_HCD)	+= hwa-hc.o
obj-$(CONFIG_USB_IMX21_HCD)	+= imx21-hcd.o


