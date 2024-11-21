/*********************************************************************************************
*Copyright (c) 2009-2017 TP-Link Technologies CO.,LTD.  All rights reserved. 

File name	: tp_roaming.h
Version		: v0.1 
Description	: mtk data interface for Network Roaming
 
Author		: Zhou Qiqiu <zhouqiqiu@tp-link.com.cn>
Create date	: 2017/7/27

History		:
01, 2017/7/27 Zhou Qiqiu, Created file.
*
*******************************************************************************************/
#ifndef _TP_ROAMING_H_
#define _TP_ROAMING_H_
#include "rtmp_type.h"

#define ETHER_ADDR_LEN 6
#define	NRD_MAX_SSID_LEN	32
#define IW_MAX_FREQUENCIES	32

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef unsigned long long u_int64_t;
typedef u_int8_t nrd_rssi_t;
typedef u_int16_t nrd_linkCapacity_t;


typedef enum
{
    NRD_FALSE = 0,
    NRD_TRUE = !NRD_FALSE

} NRD_BOOL;

typedef enum wlanif_chwidth_e {
    wlanif_chwidth_20,
    wlanif_chwidth_40,
    wlanif_chwidth_80,
    wlanif_chwidth_160,

    wlanif_chwidth_invalid
} wlanif_chwidth_e;

#if 0
typedef struct wlanif_staStats_t {
    /// The Tx byte count
    u_int64_t txByteCount;
    /// The Rx byte count
    u_int64_t rxByteCount;
    /// The Tx packet count
    LARGE_INTEGER txPacketCount;
    /// The Rx packet count
    LARGE_INTEGER rxPacketCount;
    
    /// Last rate at which packets sent to the STA by this AP were sent.
    nrd_linkCapacity_t lastTxRate;

    /// Last rate at which the packets sent by the STA to this AP were sent.
    nrd_linkCapacity_t lastRxRate;   
	
	/// Uplink RSSI
    nrd_rssi_t rssi;

	/// inactivity timer
	u_int16_t idle;
} wlanif_staStats_t;
#endif
typedef struct wlanif_staStats_t {
    /// The Tx byte count
    u_int64_t txByteCount;
    /// The Rx byte count
    u_int64_t rxByteCount;
    /// The Tx packet count
    u_int32_t txPacketCount;
    /// The Rx packet count
    u_int32_t rxPacketCount;
    
    /// Last rate at which packets sent to the STA by this AP were sent.
    nrd_linkCapacity_t lastTxRate;

    /// Last rate at which the packets sent by the STA to this AP were sent.
    nrd_linkCapacity_t lastRxRate;   
	
	/// Uplink RSSI
    nrd_rssi_t rssi;

	/// inactivity timer
	u_int16_t idle;
} wlanif_staStats_t;


typedef struct _wlanif_staStats_t
{
	u_int8_t mac[ETHER_ADDR_LEN];
	wlanif_staStats_t sta;
}wlanif_stainfo_t;

/**
 * @brief Enumerations for IEEE802.11 PHY mode
 */
typedef enum wlanif_phymode_e {
    wlanif_phymode_basic,
    wlanif_phymode_ht,
    wlanif_phymode_vht,

    wlanif_phymode_invalid
} wlanif_phymode_e;


/**
 * @brief PHY capabilities supported by a VAP or client
 */
typedef struct wlanif_phyCapInfo_t {
    /// Flag indicating if this PHY capability entry is valid or not
    NRD_BOOL valid : 1;

    /// The maximum bandwidth supported by this STA
    wlanif_chwidth_e maxChWidth : 3;

    /// The spatial streams supported by this STA
    u_int8_t numStreams : 4;

    /// The PHY mode supported by this STA
    wlanif_phymode_e phyMode : 8;

    /// The maximum MCS supported by this STA
    u_int8_t maxMCS;

    /// The maximum TX power supporetd by this STA
    u_int8_t maxTxPower;
} wlanif_phyCapInfo_t;


typedef struct wlanif_staInfo_t {
    /// Address of the STA that associated/disassociated.
	u_int8_t mac[ETHER_ADDR_LEN];

    /// Indicate if BTM is supported 
    NRD_BOOL isBTMSupported;

    /// Indicate if RRM is supported 
    NRD_BOOL isRRMSupported;

    /// Uplink RSSI
    nrd_rssi_t rssi;

    /// Last rate at which packets sent to the STA by this AP were sent.
    nrd_linkCapacity_t lastTxRate;

    /// Last rate at which the packets sent by the STA to this AP were sent.
    nrd_linkCapacity_t lastRxRate;

	/// inactivity timer
	u_int16_t idle;

    wlanif_phyCapInfo_t phyCapInfo;
} wlanif_staInfo_t;

typedef struct wlanif_staList_t {
    u_int8_t sta_num;
    wlanif_staInfo_t *staInfo;    
}wlanif_staList_t;

typedef struct _phyCapInfo_ {
	u_int8_t mac[ETHER_ADDR_LEN];
	wlanif_phyCapInfo_t phyCapInfo;	
}phycapInfo;
typedef struct _rssiInfo_ {
	u_int8_t mac[ETHER_ADDR_LEN];
	CHAR rssi;	
}RssiInfo;

typedef struct _wlanif_rrm_req_params
{
	u_int8_t aid;
	u_int16_t MeasureDuration;
	u_int8_t RegulatoryClass;
	u_int8_t bssid[IW_ESSID_MAX_SIZE+1];
	u_int8_t ssid[IW_ESSID_MAX_SIZE+1];
	u_int8_t MeasureCh;
	u_int16_t MeasureMode;	
}wlanif_rrm_req_params;

typedef struct wlanif_bssInfo_t {
    /* SSID */
	u_int8_t essid[NRD_MAX_SSID_LEN + 2];
    
    /* BSSID */
    u_int8_t bssid[MAC_ADDR_LEN];
    
    /* channel number for the BSSID */
    u_int8_t channel;

    /// Uplink RSSI
    nrd_rssi_t rssi;

} wlanif_bssInfo_t;

typedef struct wlanif_bssList_t {
    u_int8_t bss_num;
    wlanif_bssInfo_t *bssInfo;    
} wlanif_bssList_t;

#if 0
struct	iw_freq
{
	__s32		m;		/* Mantissa */
	__s16		e;		/* Exponent */
	__u8		i;		/* List index (when in range struct) */
	__u8		flags;		/* Flags (fixed/auto) */
};


struct	iw_scan_req
{
	__u8		scan_type; /* IW_SCAN_TYPE_{ACTIVE,PASSIVE} */
	__u8		essid_len;
	__u8		num_channels; /* num entries in channel_list;
				       * 0 = scan all allowed channels */
	__u8		flags; /* reserved as padding; use zero, this may
				* be used in the future for adding flags
				* to request different scan behavior */
	struct sockaddr	bssid; /* ff:ff:ff:ff:ff:ff for broadcast BSSID or
				* individual address of a specific BSS */

	/*
	 * Use this ESSID if IW_SCAN_THIS_ESSID flag is used instead of using
	 * the current ESSID. This allows scan requests for specific ESSID
	 * without having to change the current ESSID and potentially breaking
	 * the current association.
	 */
	__u8		essid[IW_ESSID_MAX_SIZE];

	/*
	 * Optional parameters for changing the default scanning behavior.
	 * These are based on the MLME-SCAN.request from IEEE Std 802.11.
	 * TU is 1.024 ms. If these are set to 0, driver is expected to use
	 * reasonable default values. min_channel_time defines the time that
	 * will be used to wait for the first reply on each channel. If no
	 * replies are received, next channel will be scanned after this. If
	 * replies are received, total time waited on the channel is defined by
	 * max_channel_time.
	 */
	__u32		min_channel_time; /* in TU */
	__u32		max_channel_time; /* in TU */

	struct iw_freq	channel_list[IW_MAX_FREQUENCIES];
}
#endif

#endif
