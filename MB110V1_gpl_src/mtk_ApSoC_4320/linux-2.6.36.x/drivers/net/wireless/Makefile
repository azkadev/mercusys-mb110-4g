#
# Makefile for the Linux Wireless network device drivers.
#

obj-$(CONFIG_IPW2100) += ipw2x00/
obj-$(CONFIG_IPW2200) += ipw2x00/

obj-$(CONFIG_HERMES)		+= orinoco/

obj-$(CONFIG_AIRO)		+= airo.o
obj-$(CONFIG_AIRO_CS)		+= airo_cs.o airo.o

obj-$(CONFIG_ATMEL)             += atmel.o
obj-$(CONFIG_PCI_ATMEL)         += atmel_pci.o 
obj-$(CONFIG_PCMCIA_ATMEL)      += atmel_cs.o

obj-$(CONFIG_AT76C50X_USB)      += at76c50x-usb.o

obj-$(CONFIG_PRISM54)		+= prism54/

obj-$(CONFIG_HOSTAP)		+= hostap/
obj-$(CONFIG_B43)		+= b43/
obj-$(CONFIG_B43LEGACY)		+= b43legacy/
obj-$(CONFIG_ZD1211RW)		+= zd1211rw/
obj-$(CONFIG_RTL8180)		+= rtl818x/
obj-$(CONFIG_RTL8187)		+= rtl818x/

# 16-bit wireless PCMCIA client drivers
obj-$(CONFIG_PCMCIA_RAYCS)	+= ray_cs.o
obj-$(CONFIG_PCMCIA_WL3501)	+= wl3501_cs.o

obj-$(CONFIG_USB_NET_RNDIS_WLAN)	+= rndis_wlan.o

obj-$(CONFIG_USB_ZD1201)	+= zd1201.o
obj-$(CONFIG_LIBERTAS)		+= libertas/

obj-$(CONFIG_LIBERTAS_THINFIRM)	+= libertas_tf/

obj-$(CONFIG_ADM8211)	+= adm8211.o

obj-$(CONFIG_MWL8K)	+= mwl8k.o

obj-$(CONFIG_IWLWIFI)	+= iwlwifi/
obj-$(CONFIG_RT2X00)	+= rt2x00/

obj-$(CONFIG_P54_COMMON)	+= p54/

obj-$(CONFIG_ATH_COMMON)	+= ath/

obj-$(CONFIG_MAC80211_HWSIM)	+= mac80211_hwsim.o

obj-$(CONFIG_WL12XX)	+= wl12xx/

obj-$(CONFIG_IWM)	+= iwmc3200wifi/

ifneq ($(CONFIG_RT2860V2_AP),)
obj-$(CONFIG_RT2860V2_AP)       += rt2860v2_ap/
endif
ifneq ($(CONFIG_RT2860V2_STA),)
obj-$(CONFIG_RT2860V2_STA)      += rt2860v2_sta/
endif
ifneq ($(CONFIG_RLT_AP_SUPPORT),)
obj-$(CONFIG_RLT_AP_SUPPORT)       += rlt_wifi_ap/
endif
ifneq ($(CONFIG_RLT_STA_SUPPORT),)
obj-$(CONFIG_RLT_STA_SUPPORT)      += rlt_wifi_sta/
endif
ifneq ($(CONFIG_MT_AP_SUPPORT),)
obj-$(CONFIG_MT_AP_SUPPORT)       += mt_wifi_ap/
endif
ifneq ($(CONFIG_MT_STA_SUPPORT),)
obj-$(CONFIG_MT_STA_SUPPORT)      += mt_wifi_sta/
endif
ifneq ($(CONFIG_RTPCI_AP),)
obj-$(CONFIG_RTPCI_AP)      	+= RTPCI_ap/
endif
ifneq ($(CONFIG_RT3090_AP),)
obj-$(CONFIG_RT3090_AP)      	+= RT3090_ap/
endif
ifneq ($(CONFIG_RT5392_AP),)
obj-$(CONFIG_RT5392_AP)      	+= RT5392_ap/
endif
ifneq ($(CONFIG_RT5592_AP),)
obj-$(CONFIG_RT5592_AP)      	+= RT5592_ap/
endif
ifneq ($(CONFIG_RT3593_AP),)
obj-$(CONFIG_RT3593_AP)      	+= RT3593_ap/
endif
ifneq ($(CONFIG_RT3572_AP),)
obj-$(CONFIG_RT3572_AP)      	+= RT3572_ap/
endif
ifneq ($(CONFIG_RT5572_AP),)
obj-$(CONFIG_RT5572_AP)      	+= RT5572_ap/
endif
ifneq ($(CONFIG_RT305x_INIC_MII),)
obj-$(CONFIG_RT2880v2_INIC_MII) += iNIC/mii/
endif
ifneq ($(CONFIG_RT305x_INIC_MII),)
obj-$(CONFIG_RT2880v2_INIC_PCI) += iNIC/pci/
endif
ifneq ($(CONFIG_RT305x_INIC_MII),)
obj-$(CONFIG_RT305x_INIC_MII) += iNIC_RT305x/mii/
endif
ifneq ($(CONFIG_RT305x_INIC_USB),)
obj-$(CONFIG_RT305x_INIC_USB) += iNIC_RT305x/usb/
endif
ifneq ($(CONFIG_MT7610_AP),)
obj-$(CONFIG_MT7610_AP)      	+= MT7610_ap/
endif
ifneq ($(CONFIG_MT7612_AP),)
obj-$(CONFIG_MT7612_AP)      	+= MT7612_ap/
endif
ifneq ($(CONFIG_MT7613_MT_AP_SUPPORT),)
obj-$(CONFIG_MT7613_MT_AP_SUPPORT) += MT7613_ap/
endif