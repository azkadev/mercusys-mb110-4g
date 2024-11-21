config MT_WIFI
	tristate "MT WIFI Driver"
	select WIFI_BASIC_FUNC if MT_WIFI

config MT_WIFI_PATH
		string
		depends on MT_WIFI
		default "rlt_wifi"

if MT_WIFI
menu "WiFi Generic Feature Options"
choice
		prompt "EEPROM Type of 1st Card"
		depends on ! FIRST_IF_NONE

		config FIRST_IF_EEPROM_FLASH
		bool "FLASH"
endchoice

config RT_FIRST_CARD_EEPROM
		string
		depends on ! FIRST_IF_NONE
		default "flash" if FIRST_IF_EEPROM_FLASH

#choice
#		prompt "EEPROM Type of 2nd Card"
#		depends on ! SECOND_IF_NONE

#		config SECOND_IF_EEPROM_PROM
#		bool "EEPROM"

#		config SECOND_IF_EEPROM_EFUSE
#		bool "EFUSE"

#		config SECOND_IF_EEPROM_FLASH
#		bool "FLASH"
#endchoice

#config RT_SECOND_CARD_EEPROM
#		string
#		depends on ! SECOND_IF_NONE
#		default "prom" if SECOND_IF_EEPROM_PROM
#		default "efuse" if SECOND_IF_EEPROM_EFUSE
#		default "flash" if SECOND_IF_EEPROM_FLASH
		
#config MULTI_INF_SUPPORT
#		bool
#		default y if !FIRST_IF_NONE && !SECOND_IF_NONE
		
config WIFI_BASIC_FUNC
	bool "Basic Functions"
	select WIRELESS_EXT
	select WEXT_SPY
	select WEXT_PRIV
        
config MT_WSC_INCLUDED
	bool "WSC (WiFi Simple Config)"
	depends on WIFI_DRIVER
	default y

config MT_WSC_V2_SUPPORT
	bool "WSC V2(WiFi Simple Config Version 2.0)"
	depends on WIFI_DRIVER
	default y

config MT_DOT11W_PMF_SUPPORT
	bool "PMF"
	depends on WIFI_DRIVER
	default y

config MT_LLTD_SUPPORT
	bool "LLTD (Link Layer Topology Discovery Protocol)"
	depends on WIFI_DRIVER
	default n

config MT_QOS_DLS_SUPPORT
	bool "802.11e DLS ((Direct-Link Setup) Support"
	depends on WIFI_DRIVER
	default n

config MT_WAPI_SUPPORT
	bool "WAPI Support"
	depends on WIFI_DRIVER
	default n

#config CARRIER_DETECTION_SUPPORT
#	bool "Carrier Detect"
#	depends on WIFI_DRIVER
#	default n

config MT_IGMP_SNOOP_SUPPORT
	bool "IGMP snooping"
	depends on WIFI_DRIVER
	default n

config MT_BLOCK_NET_IF
	bool "NETIF Block"
	depends on WIFI_DRIVER
	default n
	help
	Support Net interface block while Tx-Sw queue full

config MT_RATE_ADAPTION
	bool "New Rate Adaptation support"
	depends on WIFI_DRIVER
	default y

config MT_NEW_RATE_ADAPT_SUPPORT
	bool "Intelligent Rate Adaption"
	depends on WIFI_DRIVER && MT_RATE_ADAPTION
	default y

config MT_AGS_SUPPORT
	bool "Adaptive Group Switching"
	depends on WIFI_DRIVER && MT_RATE_ADAPTION
	default n
    
config MT_IDS_SUPPORT
	bool "IDS (Intrusion Detection System) Support"
	depends on WIFI_DRIVER
	default n

config MT_WIFI_WORK_QUEUE
	bool "Work Queue"
	depends on WIFI_DRIVER
	default n

config MT_WIFI_SKB_RECYCLE
	bool "SKB Recycle(Linux)"
	depends on WIFI_DRIVER
	default n

#config RTMP_FLASH_SUPPORT
#	bool "Flash Support"
#	depends on WIFI_DRIVER
#	default y

config MT_SMART_CARRIER_SENSE_SUPPORT
        bool "Smart Carrier Sense"
        depends on WIFI_DRIVER
        default y

config MT_LED_CONTROL_SUPPORT
	bool "LED Support"
	depends on WIFI_DRIVER
	default n

config MT_ATE_SUPPORT
	bool "ATE/QA Support"
	depends on WIFI_DRIVER
	default n

config MT_MEMORY_OPTIMIZATION
	bool "Memory Optimization"
	depends on WIFI_DRIVER
	default n
	
config MT_SINGLE_SKU
	bool "Single SKU Support"
	depends on WIFI_DRIVER
	default n
	
config MT_UAPSD
	bool "UAPSD support"
	depends on WIFI_DRIVER
	default y

#
# Section for chip architectures
#
# "RLT MAC Support"
#
# Section for interfaces
#
config RTMP_PCI_SUPPORT
    	bool

config RTMP_USB_SUPPORT
		bool

config RTMP_RBUS_SUPPORT
    	bool

endmenu

menu "WiFi Operation Modes"
	choice
		prompt "Main Mode"
		default MT_WIFI_MODE_AP

		config MT_WIFI_MODE_AP
				bool "AP"

		config MT_WIFI_MODE_STA
				bool "STA"

		config MT_WIFI_MODE_BOTH
				bool "APSTA"
    endchoice

    if MT_WIFI_MODE_AP || MT_WIFI_MODE_BOTH
		source "drivers/net/wireless/mt_wifi_ap/Kconfig"
    endif

    #if MT_WIFI_MODE_STA || MT_WIFI_MODE_BOTH

		#source "drivers/net/wireless/mt_wifi_sta/Kconfig"

    #endif

endmenu	

endif

#if MT_MAC
if RALINK_MT7603E
	config MT_MAC
		bool
		default y
endif
