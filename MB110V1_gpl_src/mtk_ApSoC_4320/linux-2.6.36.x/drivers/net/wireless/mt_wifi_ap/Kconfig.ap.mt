config MT_AP_SUPPORT
	tristate "Ralink RT2860 802.11n AP support"
#	depends on NET_RADIO 
	select WIRELESS_EXT
	select WEXT_SPY
	select WEXT_PRIV

config MT_WDS_SUPPORT
	bool "WDS"
	depends on MT_AP_SUPPORT

config MT_MBSS_SUPPORT
	bool "MBSSID"
	depends on MT_AP_SUPPORT
	default y

config MT_NEW_MBSSID_MODE
	bool "New MBSSID MODE"
	depends on MT_AP_SUPPORT && MT_MBSS_SUPPORT
	depends on RALINK_RT3883 || RALINK_RT3352 || RALINK_RT5350 || RALINK_RT6352 || RALINK_MT7620
	default y

config MT_ENHANCE_NEW_MBSSID_MODE
	bool "Enhanced MBSSID mode"
	depends on MT_NEW_MBSSID_MODE
	default y

config MT_APCLI_SUPPORT
	bool "AP-Client Support"
	depends on MT_AP_SUPPORT

config MT_APCLI_AUTO_BW_SUPPORT
	bool "AP-Client Auto Bandwidth Support"
	depends on MT_AP_SUPPORT
        depends on MT_APCLI_SUPPORT

config MT_APCLI_CERT_SUPPORT
	bool "AP-Client TGN Cert Support"
	depends on MT_AP_SUPPORT
        depends on MT_APCLI_SUPPORT
        
config MT_MAC_REPEATER_SUPPORT
	bool "MAC Repeater Support"
	depends on MT_AP_SUPPORT
	depends on MT_APCLI_SUPPORT
	depends on RALINK_RT6352 || RALINK_MT7620 || RALINK_MT7603E || MT_AP_SUPPORT
	default n

config MT_DOT11R_FT_SUPPORT
	bool "802.11r Fast BSS Transition"
	depends on MT_AP_SUPPORT

#config DOT11K_RRM_SUPPORT
#	bool "802.11k Radio Resource Management"
#	depends on MT_AP_SUPPORT

#config MT_SNIFFER_SUPPORT
#	bool "SNIFFER"
#	depends on MT_AP_SUPPORT
#	default n

#config MT_CFG80211_SUPPORT
#	bool "CFG80211"
#	depends on MT_AP_SUPPORT
#	default n

#config CON_WPS_SUPPORT
#	bool "Concurrent WPS Support"
#	depends on MT_AP_SUPPORT
#	depends on MT_APCLI_SUPPORT
#	depends on MT_WSC_INCLUDED
#	depends on MT_WSC_V2_SUPPORT
#	default n
	
config MT_LLTD_SUPPORT
	bool "LLTD (Link Layer Topology Discovery Protocol)"
	depends on MT_AP_SUPPORT

#config MT_COC_SUPPORT
#	bool "CoC Support"
#	depends on MT_AP_SUPPORT
#	default n

#config  RT2860V2_SNMP
#	bool "Net-SNMP Support"
#	depends on MT_AP_SUPPORT

#config MCAST_RATE_SPECIFIC
#	bool "User specific tx rate of mcast pkt"
#	depends on MT_AP_SUPPORT

#config EXT_BUILD_CHANNEL_LIST
#	bool "Extension Channel List"
#	depends on MT_AP_SUPPORT

#config AUTO_CH_SELECT_ENHANCE
#	bool "Auto Channel Selection Enhancement"
#	depends on MT_AP_SUPPORT

config AIRPLAY_SUPPORT
	bool "AIRPLAY Support"
	depends on MT_AP_SUPPORT
	default n
	
config BAND_STEERING
	bool "Band Steering"
	depends on MT_AP_SUPPORT
	default n

