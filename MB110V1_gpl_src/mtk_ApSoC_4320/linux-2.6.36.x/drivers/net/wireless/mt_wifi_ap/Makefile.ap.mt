EXTRA_CFLAGS = -Idrivers/net/wireless/mt_wifi/include \
				-Idrivers/net/wireless/mt_wifi/embedded/include \
				-Idrivers/net/wireless/mt_wifi/ate/include

DRV_NAME = mt_wifi
MT_WIFI_DIR = ../mt_wifi/embedded

########################################################
# Common files
########################################################
cmm_objs := $(MT_WIFI_DIR)/common/crypt_md5.o\
						$(MT_WIFI_DIR)/common/crypt_sha2.o\
						$(MT_WIFI_DIR)/common/crypt_hmac.o\
						$(MT_WIFI_DIR)/common/crypt_aes.o\
						$(MT_WIFI_DIR)/common/crypt_arc4.o\
						$(MT_WIFI_DIR)/common/mlme.o\
						$(MT_WIFI_DIR)/common/cmm_wep.o\
						$(MT_WIFI_DIR)/common/action.o\
						$(MT_WIFI_DIR)/common/ba_action.o\
						$(MT_WIFI_DIR)/../hw_ctrl/cmm_asic.o\
						$(MT_WIFI_DIR)/mgmt/mgmt_ht.o\
						$(MT_WIFI_DIR)/common/cmm_data.o\
						$(MT_WIFI_DIR)/common/rtmp_init.o\
						$(MT_WIFI_DIR)/common/rtmp_init_inf.o\
						$(MT_WIFI_DIR)/common/cmm_tkip.o\
						$(MT_WIFI_DIR)/common/cmm_aes.o\
						$(MT_WIFI_DIR)/common/cmm_sync.o\
						$(MT_WIFI_DIR)/common/eeprom.o\
						$(MT_WIFI_DIR)/common/cmm_sanity.o\
						$(MT_WIFI_DIR)/common/cmm_info.o\
						$(MT_WIFI_DIR)/common/cmm_cfg.o\
						$(MT_WIFI_DIR)/common/cmm_wpa.o\
						$(MT_WIFI_DIR)/common/cmm_radar.o\
						$(MT_WIFI_DIR)/common/spectrum.o\
						$(MT_WIFI_DIR)/common/rtmp_timer.o\
						$(MT_WIFI_DIR)/common/rt_channel.o\
						$(MT_WIFI_DIR)/common/rt_os_util.o\
						$(MT_WIFI_DIR)/common/cmm_profile.o\
						$(MT_WIFI_DIR)/common/scan.o\
						$(MT_WIFI_DIR)/common/cmm_cmd.o\
						$(MT_WIFI_DIR)/common/sys_log.o\
						$(MT_WIFI_DIR)/common/txpower.o\
						$(MT_WIFI_DIR)/../chips/rtmp_chip.o\
						$(MT_WIFI_DIR)/mgmt/mgmt_hw.o\
						$(MT_WIFI_DIR)/mgmt/mgmt_entrytb.o\
						$(MT_WIFI_DIR)/tx_rx/wdev.o\
						$(MT_WIFI_DIR)/tx_rx/wdev_tx.o\
						$(MT_WIFI_DIR)/tx_rx/wdev_rx.o\
						$(MT_WIFI_DIR)/os/linux/rt_profile.o

########################################################
# Rate adaptation related files
########################################################
rate_objs := $(MT_WIFI_DIR)/../rate_ctrl/ra_ctrl.o\
						$(MT_WIFI_DIR)/../rate_ctrl/alg_legacy.o

ifeq ($(CONFIG_MT_NEW_RATE_ADAPT_SUPPORT),y)
    EXTRA_CFLAGS += -DNEW_RATE_ADAPT_SUPPORT
    rate_objs += $(MT_WIFI_DIR)/../rate_ctrl/alg_grp.o

    ifeq ($(CONFIG_MT_AGS_SUPPORT),y)
        EXTRA_CFLAGS += -DAGS_ADAPT_SUPPORT
        rate_objs += $(MT_WIFI_DIR)/../rate_ctrl/alg_ags.o
    endif
endif


########################################################
# ASIC related files
########################################################
asic_objs := $(MT_WIFI_DIR)/../phy/phy.o


########################################################
# Spec feature related files
########################################################
spec_objs += $(MT_WIFI_DIR)/common/ps.o

ifeq ($(CONFIG_MT_UAPSD),y)
    EXTRA_CFLAGS += -DUAPSD_SUPPORT -DUAPSD_DEBUG
    spec_objs += $(MT_WIFI_DIR)/common/uapsd.o
endif


ifeq ($(CONFIG_MT_MAC),y)
	EXTRA_CFLAGS += -DMT_PS
	spec_objs += $(MT_WIFI_DIR)/common/mt_ps.o
	spec_objs += $(MT_WIFI_DIR)/common/mt_io.o
	spec_objs += $(MT_WIFI_DIR)/tx_rx/txs.o
endif

# WSC
ifeq ($(CONFIG_MT_WSC_INCLUDED),y)
    EXTRA_CFLAGS += -DWSC_INCLUDED -DWSC_SINGLE_TRIGGER

    ifneq ($(CONFIG_MT_AP_SUPPORT),)
        EXTRA_CFLAGS += -DWSC_AP_SUPPORT
    endif

    ifneq ($(CONFIG_MT_STA_SUPPORT),)
        EXTRA_CFLAGS += -DWSC_STA_SUPPORT
    endif

    spec_objs += $(MT_WIFI_DIR)/common/wsc.o\
                    $(MT_WIFI_DIR)/common/wsc_tlv.o\
                    $(MT_WIFI_DIR)/common/crypt_dh.o\
                    $(MT_WIFI_DIR)/common/crypt_biginteger.o\
                    $(MT_WIFI_DIR)/common/wsc_ufd.o

    ifeq ($(CONFIG_MT_WSC_V2_SUPPORT),y)
        EXTRA_CFLAGS += -DWSC_V2_SUPPORT
        spec_objs += $(MT_WIFI_DIR)/common/wsc_v2.o
    endif
endif

# VHT

# WAPI
#ifeq ($(CONFIG_MT_WAPI_SUPPORT),y)
#    EXTRA_CFLAGS += -DWAPI_SUPPORT
#    ifeq ($(CONFIG_RALINK_RT3052),y)
#        EXTRA_CFLAGS += -DSOFT_ENCRYPT
#    endif
#
#  spec_objs += $(MT_WIFI_DIR)/common/wapi.o\
#                $(MT_WIFI_DIR)/common/wapi_sms4.o\
#		$(MT_WIFI_DIR)/common/wapi_crypt.o
#		
#endif

# ACM
ifeq ($(CONFIG_MT_WMM_ACM_SUPPORT),y)
    EXTRA_CFLAGS += -DWMM_ACM_SUPPORT

    spec_objs += $(MT_WIFI_DIR)/common/acm_edca.o\
            $(MT_WIFI_DIR)/common/acm_comm.o\
            $(MT_WIFI_DIR)/common/acm_iocl.o
endif

#PMF
ifeq ($(CONFIG_MT_DOT11W_PMF_SUPPORT),y)
    EXTRA_CFLAGS += -DDOT11W_PMF_SUPPORT -DSOFT_ENCRYPT

    spec_objs += $(MT_WIFI_DIR)/common/pmf.o
endif

# LLTD
ifeq ($(CONFIG_MT_LLTD_SUPPORT),y)
    EXTRA_CFLAGS += -DLLTD_SUPPORT
endif

ifeq ($(CONFIG_SMART_CARRIER_SENSE_SUPPORT),y)
    EXTRA_CFLAGS += -DSMART_CARRIER_SENSE_SUPPORT
endif

# FT
ifeq ($(CONFIG_MT_DOT11R_FT_SUPPORT),y)
EXTRA_CFLAGS += -DDOT11R_FT_SUPPORT
spec_objs += $(MT_WIFI_DIR)/common/ft.o\
                    $(MT_WIFI_DIR)/common/ft_tlv.o\
                    $(MT_WIFI_DIR)/common/ft_iocl.o\
                    $(MT_WIFI_DIR)/common/ft_rc.o\
                    $(MT_WIFI_DIR)/ap/ap_ftkd.o
endif

# RR
ifeq ($(CONFIG_MT_DOT11K_RRM_SUPPORT),y)
EXTRA_CFLAGS += -DDOT11K_RRM_SUPPORT -DAP_SCAN_SUPPORT -DSCAN_SUPPORT -DAPPLE_11K_IOT
spec_objs += $(MT_WIFI_DIR)/common/rrm_tlv.o\
                    $(MT_WIFI_DIR)/common/rrm_sanity.o\
                    $(MT_WIFI_DIR)/common/rrm.o
endif

#SNIFFER
ifeq ($(CONFIG_MT_SNIFFER_SUPPORT),y)
    EXTRA_CFLAGS += -DCONFIG_SNIFFER_SUPPORT

    spec_objs += $(MT_WIFI_DIR)/sniffer/sniffer_radiotap.o\
                 $(MT_WIFI_DIR)/sniffer/sniffer_prism.o
endif

#CFG
ifeq ($(CONFIG_MT_CFG80211_SUPPORT),y)
    EXTRA_CFLAGS += -DRT_CFG80211_SUPPORT -DEXT_BUILD_CHANNEL_LIST

    spec_objs += $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211_scan.o\
        $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211_rx.o\
        $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211_tx.o\
        $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211_inf.o\
        $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211_ap.o\
        $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211drv.o\
                $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211_util.o\
                $(MT_WIFI_DIR)/os/linux/cfg80211/cfg80211.o
endif

#
# Common Feature related files
#
func_objs :=

ifeq ($(CONFIG_MT_IGMP_SNOOP_SUPPORT),y)
    EXTRA_CFLAGS += -DIGMP_SNOOP_SUPPORT

    func_objs += $(MT_WIFI_DIR)/common/igmp_snoop.o
endif

ifeq ($(CONFIG_MT_BLOCK_NET_IF),y)
    EXTRA_CFLAGS += -DBLOCK_NET_IF

    func_objs += $(MT_WIFI_DIR)/common/netif_block.o
endif

ifeq ($(CONFIG_MT_SINGLE_SKU),y)
        EXTRA_CFLAGS += -DSINGLE_SKU_V2
endif

ifeq ($(CONFIG_RT2860V2_AP_VIDEO_TURBINE),y)
    EXTRA_CFLAGS += -DVIDEO_TURBINE_SUPPORT

    func_objs += $(MT_WIFI_DIR)/common/cmm_video.o
endif


ifeq ($(CONFIG_MT_LED_CONTROL_SUPPORT),y)
    EXTRA_CFLAGS += -DLED_CONTROL_SUPPORT
    ifeq ($(CONFIG_MT_WSC_INCLUDED),y)
        EXTRA_CFLAGS += -DWSC_LED_SUPPORT
    endif

    func_objs += $(MT_WIFI_DIR)/common/rt_led.o
endif

ifeq ($(CONFIG_MT_SMART_CARRIER_SENSE_SUPPORT),y)
    EXTRA_CFLAGS += -DSMART_CARRIER_SENSE_SUPPORT
endif

########################################################
# AP feature related files
########################################################
ap_objs := $(MT_WIFI_DIR)/ap/ap.o\
            $(MT_WIFI_DIR)/ap/ap_assoc.o\
            $(MT_WIFI_DIR)/ap/ap_auth.o\
            $(MT_WIFI_DIR)/ap/ap_connect.o\
            $(MT_WIFI_DIR)/ap/ap_mlme.o\
            $(MT_WIFI_DIR)/ap/ap_sanity.o\
            $(MT_WIFI_DIR)/ap/ap_sync.o\
            $(MT_WIFI_DIR)/ap/ap_wpa.o\
            $(MT_WIFI_DIR)/ap/ap_data.o\
            $(MT_WIFI_DIR)/ap/ap_autoChSel.o\
            $(MT_WIFI_DIR)/ap/ap_qload.o\
            $(MT_WIFI_DIR)/ap/ap_cfg.o\
            $(MT_WIFI_DIR)/ap/ap_nps.o\
            $(MT_WIFI_DIR)/os/linux/ap_ioctl.o            

ifeq ($(CONFIG_MT_QOS_DLS_SUPPORT),y)
    EXTRA_CFLAGS += -DQOS_DLS_SUPPORT
    ap_objs += $(MT_WIFI_DIR)/ap/ap_dls.o
endif

ifeq ($(CONFIG_MT_MBSS_SUPPORT),y)
    EXTRA_CFLAGS += -DMBSS_SUPPORT

    ifeq ($(CONFIG_MT_NEW_MBSSID_MODE),y)
        EXTRA_CFLAGS += -DNEW_MBSSID_MODE
        ifeq ($(CONFIG_MT_ENHANCE_NEW_MBSSID_MODE),y)
            EXTRA_CFLAGS += -DENHANCE_NEW_MBSSID_MODE
        endif
    endif

    ap_objs += $(MT_WIFI_DIR)/ap/ap_mbss.o\
            $(MT_WIFI_DIR)/ap/ap_mbss_inf.o
endif


ifeq ($(CONFIG_MT_WDS_SUPPORT),y)
    EXTRA_CFLAGS += -DWDS_SUPPORT

    ap_objs += $(MT_WIFI_DIR)/ap/ap_wds.o\
            $(MT_WIFI_DIR)/ap/ap_wds_inf.o\
            $(MT_WIFI_DIR)/common/client_wds.o
endif

ifeq ($(CONFIG_MT_APCLI_SUPPORT),y)
    EXTRA_CFLAGS += -DAPCLI_SUPPORT -DMAT_SUPPORT  -DAPCLI_AUTO_CONNECT_SUPPORT -DAPCLI_AUTO_BW_TMP
    #EXTRA_CFLAGS += -DAPCLI_CONNECTION_TRIAL -DMULTI_APCLI_SUPPORT -DAPCLI_CERT_SUPPORT

    ap_objs += $(MT_WIFI_DIR)/ap/ap_apcli.o\
            $(MT_WIFI_DIR)/ap/ap_apcli_inf.o\
            $(MT_WIFI_DIR)/ap/apcli_assoc.o\
            $(MT_WIFI_DIR)/ap/apcli_auth.o\
            $(MT_WIFI_DIR)/ap/apcli_ctrl.o\
            $(MT_WIFI_DIR)/ap/apcli_sync.o\
            $(MT_WIFI_DIR)/common/cmm_mat.o\
            $(MT_WIFI_DIR)/common/cmm_mat_iparp.o\
            $(MT_WIFI_DIR)/common/cmm_mat_pppoe.o\
            $(MT_WIFI_DIR)/common/cmm_mat_ipv6.o

    ifeq ($(CONFIG_MT_MAC_REPEATER_SUPPORT),y)
        EXTRA_CFLAGS += -DMAC_REPEATER_SUPPORT

        ap_objs += $(MT_WIFI_DIR)/ap/ap_repeater.o
    endif

    ifeq ($(CONFIG_MT_APCLI_CERT_SUPPORT),y) 		
	EXTRA_CFLAGS += -DAPCLI_CERT_SUPPORT
    endif
    
    ifeq ($(CONFIG_MT_APCLI_AUTO_BW_SUPPORT),y) 		
	EXTRA_CFLAGS += -DAPCLI_AUTO_BW_SUPPORT
	ap_objs += $(MT_WIFI_DIR)/ap/apcli_autobw.o
    endif	

endif

ifeq ($(CONFIG_MT_IDS_SUPPORT),y)
    EXTRA_CFLAGS += -DIDS_SUPPORT

    ap_objs += $(MT_WIFI_DIR)/ap/ap_ids.o
endif

ifeq ($(CONFIG_MT_NINTENDO_AP),y)
    EXTRA_CFLAGS += -DNINTENDO_AP

    ap_objs += $(MT_WIFI_DIR)/ap/ap_nintendo.o
endif

#ifeq ($(CONFIG_MT_COC_SUPPORT),y)
#    EXTRA_CFLAGS += -DGREENAP_SUPPORT -DCOC_SUPPORT
#
#    ap_objs += $(MT_WIFI_DIR)/../hw_ctrl/greenap.o
#endif

ifeq ($(CONFIG_MT_ATE_SUPPORT),y)
    EXTRA_CFLAGS += -DCONFIG_ATE -DCONFIG_QA -DCONFIG_RT2880_ATE_CMD_NEW
endif

ifeq ($(CONFIG_MT_RTMP_FLASH_SUPPORT),y)
    EXTRA_CFLAGS += -DRTMP_FLASH_SUPPORT
endif

ifeq ($(CONFIG_MT_RTMP_TEMPERATURE_TX_ALC),y)
    EXTRA_CFLAGS += -DRTMP_TEMPERATURE_TX_ALC
endif

ifeq ($(CONFIG_MT_TXBF_SUPPORT),y)
    EXTRA_CFLAGS += -DMCS_LUT_SUPPORT -DTXBF_SUPPORT -DVHT_TXBF_SUPPORT
    ap_objs += $(MT_WIFI_DIR)/common/cmm_txbf.o\
                    $(MT_WIFI_DIR)/common/cmm_txbf_cal.o
endif

ifeq ($(CONFIG_CON_WPS_SUPPORT), y)
    EXTRA_CFLAGS += -DCON_WPS -DCON_WPS_AP_SAME_UUID
endif

ifeq ($(CONFIG_MT_DFS_SUPPORT),y)
    EXTRA_CFLAGS += -DDFS_SUPPORT
    EXTRA_CFLAGS += -DDFS_ATP_SUPPORT
    ap_objs += $(MT_WIFI_DIR)/common/cmm_dfs.o
endif

ifeq ($(CONFIG_RT2860V2_AP_CARRIER),y)
    EXTRA_CFLAGS += -DCARRIER_DETECTION_SUPPORT

    ap_objs += $(MT_WIFI_DIR)/common/cmm_cs.o
endif

ifeq ($(CONFIG_RT2860V2_MCAST_RATE_SPECIFIC),y)
    EXTRA_CFLAGS += -DMCAST_RATE_SPECIFIC
endif

ifeq ($(CONFIG_AIRPLAY_SUPPORT),y)
    EXTRA_CFLAGS += -DAIRPLAY_SUPPORT
endif
  
ifeq ($(CONFIG_BAND_STEERING),y)
    EXTRA_CFLAGS += -DBAND_STEERING
    ap_objs +=  $(MT_WIFI_DIR)/ap/ap_band_steering.o
endif

########################################################
# Linux system related files
########################################################
os_objs := $(MT_WIFI_DIR)/os/linux/rt_proc.o\
            $(MT_WIFI_DIR)/os/linux/rt_linux.o\
            $(MT_WIFI_DIR)/os/linux/rt_profile.o\
            $(MT_WIFI_DIR)/os/linux/rt_txrx_hook.o\
            $(MT_WIFI_DIR)/os/linux/rt_main_dev.o

#for MT7628 + MT7637E autobuild symbols conflict
ifeq ($(CONFIG_MT_TXRX_HOOK),y)
os_objs += $(SRC_DIR)/os/linux/rt_txrx_hook.o
endif

ifeq ($(CONFIG_MT_WIFI_WORK_QUEUE_BH),y)
    EXTRA_CFLAGS += -DWORKQUEUE_BH
endif

ifeq ($(CONFIG_MT_KTHREAD),y)
    EXTRA_CFLAGS += -DKTHREAD_SUPPORT
endif


########################################################
# chip related files
########################################################
ifeq ($(CONFIG_RALINK_MT7628),y)
EXTRA_CFLAGS += -DMT7628 -DMT_BBP -DMT_RF -DRTMP_RBUS_SUPPORT -DRTMP_RF_RW_SUPPORT -DMT_MAC -DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT
EXTRA_CFLAGS += -DRTMP_FLASH_SUPPORT -DDMA_SCH_SUPPORT -DRTMP_EFUSE_SUPPORT
EXTRA_CFLAGS += -DCONFIG_ANDES_SUPPORT
EXTRA_CFLAGS += -DRESOURCE_PRE_ALLOC
EXTRA_CFLAGS += -DNEW_MBSSID_MODE -DENHANCE_NEW_MBSSID_MODE
EXTRA_CFLAGS += -DENHANCED_STAT_DISPLAY
EXTRA_CFLAGS += -DFIFO_EXT_SUPPORT
EXTRA_CFLAGS += -DMCS_LUT_SUPPORT
EXTRA_CFLAGS += -DUSE_BMC -DTHERMAL_PROTECT_SUPPORT -DCAL_FREE_IC_SUPPORT -DTRAFFIC_BASED_TXOP
EXTRA_CFLAGS += -DDATA_QUEUE_RESERVE -DEAPOL_QUEUE_SUPPORT -DDMA_RESET_SUPPORT -DWTBL_RATE_DEBUG

chip_objs += $(MT_WIFI_DIR)/../chips/mt7628.o\
		$(MT_WIFI_DIR)/../hw_ctrl/cmm_asic_mt.o\
		$(MT_WIFI_DIR)/../hw_ctrl/cmm_chip_mt.o\
		$(MT_WIFI_DIR)/../mac/mt_mac.o\
		$(MT_WIFI_DIR)/mcu/mcu.o\
		$(MT_WIFI_DIR)/mcu/andes_core.o\
		$(MT_WIFI_DIR)/mcu/andes_mt.o\
		$(MT_WIFI_DIR)/../phy/mt_rf.o\
		$(MT_WIFI_DIR)/../phy/rf.o\
		$(MT_WIFI_DIR)/../phy/mt_phy.o\
		$(MT_WIFI_DIR)/common/cmm_mac_pci.o\
		$(MT_WIFI_DIR)/common/cmm_data_pci.o\
		$(MT_WIFI_DIR)/common/ee_prom.o\
		$(MT_WIFI_DIR)/os/linux/rt_rbus_pci_drv.o\
		$(MT_WIFI_DIR)/hif/hif_pci.o\
		$(MT_WIFI_DIR)/os/linux/rbus_main_dev.o
endif


#
# Root 
#
obj-$(CONFIG_MT_AP_SUPPORT) += $(DRV_NAME).o

$(DRV_NAME)-objs += $(ap_objs) $(cmm_objs) $(asic_objs) $(chip_objs) $(rate_objs)\
                    $(spec_objs) $(func_objs) $(os_objs)

$(DRV_NAME)-objs += $(MT_WIFI_DIR)/common/eeprom.o\
					$(MT_WIFI_DIR)/common/ee_flash.o\
					$(MT_WIFI_DIR)/common/ee_efuse.o

$(DRV_NAME)-objs += $(MT_WIFI_DIR)/common/cmm_mac_pci.o
$(DRV_NAME)-objs += $(MT_WIFI_DIR)/common/cmm_data_pci.o

$(DRV_NAME)-objs += $(MT_WIFI_DIR)/os/linux/rt_pci_rbus.o\
                    $(MT_WIFI_DIR)/os/linux/rt_rbus_pci_drv.o\
                    $(MT_WIFI_DIR)/os/linux/rt_rbus_pci_util.o\
                    #$(MT_WIFI_DIR)/os/linux/rbus_main_dev.o

ifeq ($(CONFIG_MT_ATE_SUPPORT),y)
$(DRV_NAME)-objs += $(MT_WIFI_DIR)/../ate/ate_agent.o\
                    $(MT_WIFI_DIR)/../ate/qa_agent.o\
                    $(MT_WIFI_DIR)/../ate/mt_mac/mt_ate.o
endif

###################
#  CFLAGS
##################
EXTRA_CFLAGS += -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX \
               -Wall -Wstrict-prototypes -Wno-trigraphs -Werror -Wframe-larger-than=4096
#-DDBG_DIAGNOSE -DDBG_RX_MCS -DDBG_TX_MCS

EXTRA_CFLAGS += -DCONFIG_AP_SUPPORT -DSCAN_SUPPORT -DAP_SCAN_SUPPORT
EXTRA_CFLAGS += -DDOT11_N_SUPPORT -DDOT11N_DRAFT3 -DSTATS_COUNT_SUPPORT -DIAPP_SUPPORT -DDOT1X_SUPPORT
#EXTRA_CFLAGS += -DRALINK_ATE -DRALINK_QA -DCONFIG_RT2880_ATE_CMD_NEW
EXTRA_CFLAGS += -DCONFIG_RA_NAT_NONE

#provide busy time statistics for every TBTT */
#EXTRA_CFLAGS += -DQLOAD_FUNC_BUSY_TIME_STATS 

# provide busy time alarm mechanism 
# use the function to avoid to locate in some noise environments 
#EXTRA_CFLAGS += -DQLOAD_FUNC_BUSY_TIME_ALARM

ifeq ($(CONFIG_MT_RT2860V2_AUTO_CH_SELECT_ENCANCE),y)
EXTRA_CFLAGS += -DAUTO_CH_SELECT_ENHANCE
endif

ifeq ($(CONFIG_MT_RT2860V2_SNMP),y)
EXTRA_CFLAGS += -DSNMP_SUPPORT
endif

ifeq ($(CONFIG_MT_RT2860V2_AP_32B_DESC),y)
EXTRA_CFLAGS += -DDESC_32B_SUPPORT
endif

ifeq ($(CONFIG_MT_RT2860V2_HW_ANTENNA_DIVERSITY),y)
EXTRA_CFLAGS += -DHW_ANTENNA_DIVERSITY_SUPPORT
endif

ifeq ($(CONFIG_MT_RT2860V2_EXT_CHANNEL_LIST),y)
EXTRA_CFLAGS += -DEXT_BUILD_CHANNEL_LIST
endif

ifeq ($(CONFIG_MT_MEMORY_OPTIMIZATION),y)
EXTRA_CFLAGS += -DMEMORY_OPTIMIZATION
else
#EXTRA_CFLAGS += -DDBG
endif
EXTRA_CFLAGS += -DDBG

ifeq ($(CONFIG_MT_RTMP_INTERNAL_TX_ALC),y)
EXTRA_CFLAGS += -DRTMP_INTERNAL_TX_ALC
endif

EXTRA_CFLAGS += -DIP_ASSEMBLY

MODULE_FLAGS=$(EXTRA_CFLAGS)
export MODULE_FLAGS
obj-m+=$(MT_WIFI_DIR)/tools/plug_in/

