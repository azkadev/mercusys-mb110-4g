
ifeq ($(CONFIG_SUPPORT_OPENWRT),y)
EXTRA_CFLAGS = -I$(src)/../src/include -I$(src)/../src/ate/include
DRV_NAME = mt7610e
SRC_DIR = ../src
obj-m += $(DRV_NAME).o
else
EXTRA_CFLAGS = -Idrivers/net/wireless/MT7610_ap/include #-Idrivers/net/wireless/MT7610_ap/ate/include
DRV_NAME = MT7610_ap
SRC_DIR = .
obj-$(CONFIG_MT7610_AP) += $(DRV_NAME).o
endif

obj-$(CONFIG_MT7610_AP) += MT7610_ap.o

$(DRV_NAME)-objs += $(SRC_DIR)/rate_ctrl/ra_ctrl.o
$(DRV_NAME)-objs += $(SRC_DIR)/rate_ctrl/alg_grp.o
$(DRV_NAME)-objs += $(SRC_DIR)/rate_ctrl/alg_legacy.o
$(DRV_NAME)-objs += $(SRC_DIR)/mgmt/mgmt_entrytb.o
$(DRV_NAME)-objs += $(SRC_DIR)/mgmt/mgmt_hw.o
$(DRV_NAME)-objs += $(SRC_DIR)/mgmt/mgmt_vht.o
$(DRV_NAME)-objs += $(SRC_DIR)/mgmt/mgmt_ht.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/ap_ioctl.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_rbus_pci_drv.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_pci_rbus.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_rbus_pci_util.o
#$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/br_ftph.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/pci_main_dev.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_profile.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_linux.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_proc.o
$(DRV_NAME)-objs += $(SRC_DIR)/os/linux/rt_main_dev.o
#$(DRV_NAME)-objs += $(SRC_DIR)/mcu/rtmp_and.o
$(DRV_NAME)-objs += $(SRC_DIR)/mcu/mcu.o
$(DRV_NAME)-objs += $(SRC_DIR)/mcu/mcu_and.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/vendor.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_data_pci.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/eeprom.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_profile.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_sync.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/rtmp_init_inf.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_cfg.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/rt_rf.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/action.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/rtmp_init.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/txpower.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_cmd.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/rtmp_timer.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_info.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/ba_action.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_mac_pci.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_tkip.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_asic.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/vht.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_aes.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_md5.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_wep.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_arc4.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_hmac.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_wpa.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/ps.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_sanity.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_radar.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_aes.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/ee_efuse.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_data.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/spectrum.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_sha2.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/mlme.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/ee_prom.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/rt_os_util.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/rt_channel.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/scan.o
$(DRV_NAME)-objs += $(SRC_DIR)/common/uapsd.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_qload.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_auth.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_sync.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_mlme.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_wpa.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_assoc.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_cfg.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_autoChSel.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_sanity.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_data.o
$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_connect.o
$(DRV_NAME)-objs += $(SRC_DIR)/chips/rtmp_chip.o
#$(DRV_NAME)-objs += $(SRC_DIR)/chips/rt30xx.o
$(DRV_NAME)-objs += $(SRC_DIR)/chips/rt65xx.o
$(DRV_NAME)-objs += $(SRC_DIR)/chips/mt76x0.o
#$(DRV_NAME)-objs += $(SRC_DIR)/FPGA/verf.o
$(DRV_NAME)-objs += $(SRC_DIR)/phy/rtmp_phy.o
$(DRV_NAME)-objs += $(SRC_DIR)/phy/rlt_rf.o
$(DRV_NAME)-objs += $(SRC_DIR)/phy/rlt_phy.o
$(DRV_NAME)-objs += $(SRC_DIR)/mac/ral_nmac.o
$(DRV_NAME)-objs += $(SRC_DIR)/mac/rtmp_mac.o

EXTRA_CFLAGS += -DDROP_MASK_SUPPORT

ifeq ($(CONFIG_MT7610_AP_FLASH),y)
	EXTRA_CFLAGS += -DRTMP_FLASH_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/ee_flash.o
endif

ifeq ($(CONFIG_MT7610_AP_WMM_ACM),y)
	EXTRA_CFLAGS += -DWMM_ACM_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/acm_edca.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/acm_comm.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/acm_iocl.o
endif

ifeq ($(CONFIG_MT7610_AP_LED),y)
	EXTRA_CFLAGS += -DWLAN_LED -DLED_CONTROL_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/rt_led.o
ifeq ($(CONFIG_MT7610_AP_WSC),y)
	EXTRA_CFLAGS += -DWSC_LED_SUPPORT
endif
endif

ifeq ($(CONFIG_MT7610_AP_WSC),y)
	EXTRA_CFLAGS += -DWSC_AP_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wsc.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wsc_tlv.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_dh.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/crypt_biginteger.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wsc_ufd.o
ifeq ($(CONFIG_MT7610_AP_WSC_V2),y)
	EXTRA_CFLAGS += -DWSC_V2_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wsc_v2.o
endif
endif

ifeq ($(CONFIG_MT7610_AP_WDS),y)
	EXTRA_CFLAGS += -DWDS_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_wds.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_wds_inf.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/client_wds.o
endif

ifeq ($(CONFIG_MT7610_AP_MBSS),y)
	EXTRA_CFLAGS += -DMBSS_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_mbss.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_mbss_inf.o
endif

ifeq ($(CONFIG_MT7610_AP_APCLI),y)
	EXTRA_CFLAGS += -DAPCLI_SUPPORT
	EXTRA_CFLAGS += -DMAT_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_apcli.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_apcli_inf.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/apcli_assoc.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/apcli_auth.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/apcli_ctrl.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/apcli_sync.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_mat.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_mat_iparp.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_mat_pppoe.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_mat_ipv6.o
endif

ifeq ($(CONFIG_MT7610_AP_IGMP_SNOOP),y)
	EXTRA_CFLAGS += -DIGMP_SNOOP_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/igmp_snoop.o
endif

ifeq ($(CONFIG_MT7610_AP_NETIF_BLOCK),y)
	EXTRA_CFLAGS += -DBLOCK_NET_IF
	$(DRV_NAME)-objs += $(SRC_DIR)/common/netif_block.o
endif

ifeq ($(CONFIG_MT7610_AP_DFS),y)
	EXTRA_CFLAGS += -DDFS_SUPPORT
	EXTRA_CFLAGS += -DDFS_HARDWARE_SUPPORT -DDFS_FCC_BW40_FIX -DDFS_DEBUG 
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_dfs.o
endif

ifeq ($(CONFIG_MT7610_AP_CARRIER),y)
	EXTRA_CFLAGS += -DCARRIER_DETECTION_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_cs.o
endif

ifeq ($(CONFIG_MT7610_AP_DLS),y)
	EXTRA_CFLAGS += -DQOS_DLS_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_dls.o
endif

ifeq ($(CONFIG_MT7610_AP_IDS),y)
	EXTRA_CFLAGS += -DIDS_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_ids.o
endif

ifeq ($(CONFIG_MT7610_AP_WAPI),y)
	EXTRA_CFLAGS   += -DWAPI_SUPPORT -DSOFT_ENCRYPT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wapi.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wapi_sms4.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/wapi_crypt.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/libwapi.a
endif

ifeq ($(CONFIG_MT7610_80211R_FT),y)
	$(DRV_NAME)-objs += $(SRC_DIR)/common/ft.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/ft_iocl.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/ft_tlv.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/ft_rc.o
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_ftkd.o
endif

ifeq ($(CONFIG_MT7610_80211K_RR),y)
	$(DRV_NAME)-objs += $(SRC_DIR)/common/rrm_tlv.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/rrm_sanity.o
	$(DRV_NAME)-objs += $(SRC_DIR)/common/rrm.o
endif

ifeq ($(CONFIG_MT7610_AP_VIDEO_TURBINE),y)
	EXTRA_CFLAGS   += -DVIDEO_TURBINE_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/common/cmm_video.o
endif

ifeq ($(CONFIG_MT7610_AP_CSO),y)
	EXTRA_CFLAGS   += -DCONFIG_CSO_SUPPORT -DCONFIG_TSO_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/naf/net_acc.o
endif

#ifeq ($(CONFIG_MT7610_AP_MESH),y)
#EXTRA_CFLAGS += -DMESH_SUPPORT -DINTEL_CMPC -DSCAN_SUPPOR
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_link_mng.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_ctrl.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_forwarding.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_bmpkt.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_inf.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_tlv.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_sanity.o
#$(DRV_NAME)-objs += $(SRC_DIR)/common/mesh_path_mng.o
#endif

###################
#  CFLAGS
###################
EXTRA_CFLAGS += -Wall -Wstrict-prototypes -Wno-trigraphs -Wframe-larger-than=4096 -Werror
EXTRA_CFLAGS += -DDOT11_N_SUPPORT -DDOT11_VHT_AC -DSTATS_COUNT_SUPPORT -DNEW_RATE_ADAPT_SUPPORT
EXTRA_CFLAGS += -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -DSYSTEM_LOG_SUPPORT -DRESOURCE_PRE_ALLOC -DFPGA_MODE -DENHANCED_STAT_DISPLAY
EXTRA_CFLAGS += -DCONFIG_AP_SUPPORT  -DUAPSD_SUPPORT -DIAPP_SUPPORT -DDOT1X_SUPPORT -DAP_SCAN_SUPPORT -DSCAN_SUPPORT
#EXTRA_CFLAGS += -DRALINK_ATE -DCONFIG_RT2880_ATE_CMD_NEW -DRALINK_QA
EXTRA_CFLAGS += -DMT76x0 -DRT65xx -DMT7610 -DMT7630 -DMT7650 -DRLT_BBP -DRLT_MAC -DRLT_RF -DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DA_BAND_SUPPORT -DRX_DMA_SCATTER -DRTMP_EFUSE_SUPPORT -DRA_NAT_SUPPORT -DCONFIG_ANDES_SUPPORT
EXTRA_CFLAGS += -DVCORECAL_SUPPORT -DFIFO_EXT_SUPPORT
EXTRA_CFLAGS += -DMCS_LUT_SUPPORT -DPEER_DELBA_TX_ADAPT -DDYNAMIC_VGA_SUPPORT
#EXTRA_CFLAGS += -DDBG_DIAGNOSE -DDBG_RX_MCS -DDBG_TX_MCS

ifeq ($(CONFIG_MT7610_AP_ATE),y)
EXTRA_CFLAGS += -Idrivers/net/wireless/MT7610_ap/ate/include
$(DRV_NAME)-objs += $(SRC_DIR)/ate/common/rt_ate.o
$(DRV_NAME)-objs += $(SRC_DIR)/ate/common/ate_pci.o
$(DRV_NAME)-objs += $(SRC_DIR)/ate/chips/rt6590_ate.o
#$(DRV_NAME)-objs += $(SRC_DIR)/ate/chips/rt85592_ate.o
EXTRA_CFLAGS += -DRALINK_ATE
EXTRA_CFLAGS += -DCONFIG_RT2880_ATE_CMD_NEW
#EXTRA_CFLAGS += -DRTMP_RF_RW_SUPPORT
#EXTRA_CFLAGS += -DRF_BANK
endif

ifeq ($(CONFIG_MT7610_AP_QA),y)
$(DRV_NAME)-objs += $(SRC_DIR)/ate/common/rt_qa.o
EXTRA_CFLAGS += -DRALINK_QA
EXTRA_CFLAGS += -DENHANCED_STAT_DISPLAY
endif

ifneq ($(CONFIG_MT7610_Acceleration),y)
EXTRA_CFLAGS += -DCONFIG_RA_NAT_NONE
endif

EXTRA_CFLAGS += -DRESOURCE_PRE_ALLOC

ifeq ($(CONFIG_MT7610_AUTO_CH_SELECT_ENCANCE),y)
	EXTRA_CFLAGS += -DAUTO_CH_SELECT_ENHANCE
endif

ifeq ($(CONFIG_MT7610_AP_80211N_DRAFT3),y)
	EXTRA_CFLAGS += -DDOT11N_DRAFT3
endif

ifeq ($(CONFIG_SINGLE_SKU),y)
	EXTRA_CFLAGS += -DSINGLE_SKU
endif

ifeq ($(CONFIG_MT7610_SNMP),y)
	EXTRA_CFLAGS += -DSNMP_SUPPORT
endif

ifeq ($(CONFIG_MT7610_AP_LLTD),y)
	EXTRA_CFLAGS += -DLLTD_SUPPORT
endif

ifeq ($(CONFIG_NEW_MBSSID_MODE),y)
	EXTRA_CFLAGS += -DNEW_MBSSID_MODE
#ifeq ($(CONFIG_RALINK_MT7620),y)
	EXTRA_CFLAGS += -DENHANCE_NEW_MBSSID_MODE
#	endif
endif

ifeq ($(CONFIG_MT7610_AP_COC),y)
	EXTRA_CFLAGS   += -DGREENAP_SUPPORT -DCOC_SUPPORT
endif

ifeq ($(CONFIG_MT7610_CON_WPS_SUPPORT),y)
EXTRA_CFLAGS   += -DCON_WPS
endif

ifeq ($(CONFIG_BAND_STEERING),y)
EXTRA_CFLAGS += -DBAND_STEERING
$(DRV_NAME)-objs += $(SRC_DIR)/ ap/ap_band_steering.o
endif

ifeq ($(CONFIG_MT7610_EXT_CHANNEL_LIST),y)
	EXTRA_CFLAGS   += -DEXT_BUILD_CHANNEL_LIST
endif

ifeq ($(CONFIG_KTHREAD),y)
	EXTRA_CFLAGS   += -DKTHREAD_SUPPORT
endif

ifeq ($(CONFIG_MT7610_AP_MEMORY_OPTIMIZATION),y)
	EXTRA_CFLAGS   += -DMEMORY_OPTIMIZATION
else
#	EXTRA_CFLAGS   += -DDBG
endif

ifeq ($(CONFIG_RA_NETWORK_WORKQUEUE_BH),y)
	EXTRA_CFLAGS   += -DWORKQUEUE_BH
endif

ifeq ($(CONFIG_MT7610_AP_MAC_REPEATER),y)
	EXTRA_CFLAGS += -DMAC_APCLI_SUPPORT
	EXTRA_CFLAGS += -DMAC_REPEATER_SUPPORT
#	EXTRA_CFLAGS += -DAPCLI_CONNECTION_TRIAL
	EXTRA_CFLAGS += -DAPCLI_AUTO_CONNECT_SUPPORT
	$(DRV_NAME)-objs += $(SRC_DIR)/ap/ap_repeater.o
endif

#ifeq ($(CONFIG_MT7610_AP_HDR_TRANS),y)
#	EXTRA_CFLAGS += -DHDR_TRANS_SUPPORT
#endif

#ifeq ($(CONFIG_MT7610_AP_BIG_ENDIAN),y)
#	EXTRA_CFLAGS += -DRT_BIG_ENDIAN
#endif

# RR
ifeq ($(CONFIG_DOT11K_RRM_SUPPORT),y)
	EXTRA_CFLAGS += -DDOT11K_RRM_SUPPORT -DAP_SCAN_SUPPORT -DSCAN_SUPPORT -DAPPLE_11K_IOT
MT7610_ap-objs += common/rrm_tlv.o common/rrm_sanity.o common/rrm.o
endif

ifeq ($(CONFIG_DOT11V_WNM_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_DOT11V_WNM
	MT7610_ap-objs += common/wnm.o
endif

ifeq ($(CONFIG_TP_NETWORK_ROAMING),y)
	EXTRA_CFLAGS += -DTP_NETWORK_ROAMING
endif

ifeq ($(CONFIG_MT7610_AP_TSSI_COMPENSATION),y)
	EXTRA_CFLAGS += -DMT76x0_TSSI_CAL_COMPENSATION
endif

ifeq ($(CONFIG_RTMP_TEMPERATURE_COMPENSATION),y)
	EXTRA_CFLAGS += -DRTMP_TEMPERATURE_COMPENSATION
endif

ifeq ($(CONFIG_MT7610_AP_SINGLE_SKU),y)
	EXTRA_CFLAGS += -DSINGLE_SKU_V2
	EXTRA_CFLAGS += -DENABLE_SKU_VHT20_VHT40
endif

ifeq ($(CONFIG_MT7610_ED_MONITOR),y)
	EXTRA_CFLAGS += -DED_MONITOR -DED_SMART
endif

ifeq ($(CONFIG_TP_IE_SUPPORT),y)
	EXTRA_CFLAGS += -DTP_IE_SUPPORT
	EXTRA_CFLAGS += -DONEMESH_BACKHAUL_4ADDR
endif

ifeq ($(CONFIG_MWDS_SUPPORT),y)
	EXTRA_CFLAGS += -DMWDS -DROUTING_TAB_SUPPORT
	MT7610_ap-objs += common/mwds.o
	MT7610_ap-objs += common/routing_tab.o
endif
 
ifeq ($(CONFIG_RT_SECOND_CARD),7610)
	EXTRA_CFLAGS += -DCONFIG_RT_SECOND_CARD=7610
endif


EXTRA_CFLAGS   += -DTP_WSC_LED_SUPPORT
EXTRA_CFLAGS   += -DTP_NO_TPC_IE

PHONY := clean install uninstall
clean:
	rm -f ./common/*.o
	rm -f ./common/.*.{cmd,flags,d}
	rm -f ./os/linux/*.{o,ko,mod.{o,c}}
	rm -f ./os/linux/.*.{cmd,flags,d}
	rm -fr ./os/linux/.tmp_versions
#Must clean Module.symvers; or you will suffer symbol version not match
#when OS_ABL = YES.
	rm -f ./os/linux/Module.symvers
	rm -f ./os/linux/Module.markers
	rm -f ./os/linux/modules.order
	rm -f ./chips/*.o
	rm -f ./chips/.*.{cmd,flags,d}
	rm -f ./ap/*.o
	rm -f ./ap/.*.{cmd,flags,d}
	rm -f ./rate_ctl/*.o
	rm -f ./rate_ctl/.*.{cmd,flags,d}
	rm -f ./chips/*.o
	rm -f ./chips/.*.{cmd,flags,d}
	rm -f ./phy/*.o
	rm -f ./phy/.*.{cmd,flags,d}
	rm -f ./mgmt/*.o
	rm -f ./mgmt/.*.{cmd,flags,d}
	rm -f ./mcu/*.o
	rm -f ./mcu/.*.{cmd,flags,d}
