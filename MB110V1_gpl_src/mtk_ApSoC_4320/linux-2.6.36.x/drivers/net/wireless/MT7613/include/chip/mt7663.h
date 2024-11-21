#ifndef __MT7663_H__
#define __MT7663_H__

#include "mcu/andes_core.h"
#include "phy/mt_rf.h"

struct _RTMP_ADAPTER;
struct _RTMP_CHIP_DBG;

#define MAX_RF_ID	127
#define MAC_RF_BANK 7

#define MT7663_MT_WTBL_SIZE	136
#define MT7663_MT_WMM_SIZE	3
#define MT7663_PDA_PORT		0xf800

#define MT7663_CT_PARSE_LEN	0

#define MT7663_BIN_FILE_NAME "WIFI_RAM_CODE_MT7663.bin"
#define MT7663_BIN_FILE_NAME_E2 "WIFI_RAM_CODE_MT7663_E2.bin"
#define MT7663_ROM_PATCH_BIN_FILE_NAME_E1 "mt7663_patch_e1_hdr.bin"
#define MT7663_ROM_PATCH_BIN_FILE_NAME_E2 "mt7663_patch_e2_hdr.bin"

#define MT7663_ROM_PATCH_START_ADDRESS	0x000DC000
#define EFUSE_CONTENT_BUFFER_SIZE 0x580

#if defined(CAL_BIN_FILE_SUPPORT) && defined(MT7615)
#define PA_TRIM_OFFSET        0
#define PA_TRIM_SIZE          16
#endif /* CAL_BIN_FILE_SUPPORT */

/* wilsonl */
/* */
/* Device ID & Vendor ID, these values should match EEPROM value */
/* */


#define MT7663_RO_BAND0_PHYCTRL_STS0_OFFSET 0x4
#define MT7663_RO_BAND0_PHYCTRL_STS5_OFFSET	0xFFFFFFF4 /*-12*/
#define MT7663_PHY_BAND0_PHYMUX_5_OFFSET	0xFFFFFE00 /*-512*/

#ifdef	CONNAC_EFUSE_FORMAT_SUPPORT
typedef struct _EFUSE_INFO_T {
	/* MT7663 as example */
	UINT8 bytes_0x0_0x6F[0x70]; /* 0x00 ~ 0x6F : other purpose */
	/* 0x70 */
	EFUSE_INFO_MODULE_TX_POWER_T rTxPower; /* 59 bytes */
	UINT8 bytes_0xAB_0xAF[0x5]; /* 0x100 ~ 0x1AF : padding */
	/* 0xB0 */
	EFUSE_INFO_MODULE_2G4_COMMON_T r2G4Cmm; /* 48 bytes */
	/* 0xE0 */
	EFUSE_INFO_MODULE_2G4_WIFI_PATH_T ar2G4WFPath[MAX_ANTENNA_NUM]; /* 16*2=32 bytes */
	UINT8 bytes_0x100_0x1AF[0xB0]; /* 0x100 ~ 0x1AF : other purpose */
	/* 0x1B0 */
	EFUSE_INFO_MODULE_SYSTEM_T rSys; /* 16 bytes */
	/* 0x1C0 */
	EFUSE_INFO_MODULE_DELAY_COMP_T rDelayComp; /* 34 bytes */
	UINT8 bytes_0x1E2_0x1EF[0xE]; /* 0x1E2 ~ 0x1EF: padding */
	/* 0x1F0 */
	EFUSE_INFO_MODULE_5G_COMMON_T r5GCmm; /* 80 bytes */
	/* 0x240 */
	EFUSE_INFO_MODULE_5G_WIFI_PATH_T ar5GWFPath[MAX_ANTENNA_NUM]; /* 112*2=224 bytes */
	/* 0x320 */
	EFUSE_INFO_MODULE_IBF_CAL_T rIBfCal; /* 12*9=108 bytes */
	UINT8 bytes_0x38C_0x3FF[0x74]; /* 0x38C ~ 0x3FF: other purpose */
	UINT8 bytes_ADie_0x000_0x17F[0x180]; /* A Die 0x000 ~ 0x17F */
} EFUSE_INFO_T, *P_EFUSE_INFO_T;
#endif /*#ifdef	CONNAC_EFUSE_FORMAT_SUPPORT*/

void mt7663_init(struct _RTMP_ADAPTER *pAd);
void mt7663_get_tx_pwr_per_rate(struct _RTMP_ADAPTER *pAd);
void mt7663_get_tx_pwr_info(struct _RTMP_ADAPTER *pAd);
void mt7663_antenna_sel_ctl(struct _RTMP_ADAPTER *pAd);
void mt7663_pwrOn(struct _RTMP_ADAPTER *pAd);
void mt7663_calibration(struct _RTMP_ADAPTER *pAd, UCHAR channel);
void mt7663_tssi_compensation(struct _RTMP_ADAPTER *pAd, UCHAR channel);
VOID mt7663_chip_dbg_init(struct _RTMP_CHIP_DBG *dbg_ops);

#ifdef MT7663_FPGA
INT mt7663_chk_top_default_cr_setting(struct _RTMP_ADAPTER *pAd);
INT mt7663_chk_hif_default_cr_setting(struct _RTMP_ADAPTER *pAd);
#endif /* MT7663_FPGA */

#ifdef MT7663_RFB_MANUAL_CAL
BOOLEAN mt7663_manual_cal(struct _RTMP_ADAPTER *pAd);
INT set_manual_cal_id(struct _RTMP_ADAPTER *pAd, char *arg);
#endif /* MT7663_RFB_MANUAL_CAL */

#ifdef PRE_CAL_MT7663_SUPPORT

/* Implement MT7663 Pre-K Down-size part-1 : remove redundant Group0 of 5G */
#define CFG_PRE_CAL_MT7663_DOWNSIZE_PART1  1
/* Implement MT7663 Pre-K Down-size part-2 : remove non-used DPD Table/Entry */
#define CFG_PRE_CAL_MT7663_DOWNSIZE_PART2  1

void mt7663_apply_dpd_flatness_data(struct _RTMP_ADAPTER *pAd, struct _MT_SWITCH_CHANNEL_CFG SwChCfg);

enum {
    GBAND = 0,
    ABAND = 1,
};


#ifdef RTMP_FLASH_SUPPORT
#define CAL_PRE_CAL_SIZE_OFFSET		1536	/* DW0 : 0x600 ~ 0x603 Used for save total pre-cal size
											* DW1 : reserved
											* DW2 : reserved
											* DW3 : reserved
											*/
#define CAL_FLASH_OFFSET			1552	/* 0x610 ~ 0x5780 Used for save Group calibration data */
#else
#define CAL_FLASH_OFFSET			0
#endif

/* Group Calibration item */
#define PRE_CAL_2_4G_CR_NUM      352  /* 2.4G total 704*2 = 352*4 = 1408 Bytes */
#if (CFG_PRE_CAL_MT7663_DOWNSIZE_PART1 == 1)
#define PRE_CAL_5G_CR_NUM        4320 /* 5G (+DFS) 1080*2*8 = 2160*8 = 4320*4 = 17280 Bytes */
#else
#define PRE_CAL_5G_CR_NUM        4860 /* 5G (+DFS) 1080*2*9 = 2160*9 = 4860*4 = 19440 Bytes */
#endif

/*
 * TOTAL CR NUM : # of total 4Bytes = 2.4G Group + 5G Group
 * Fullsize     : 352 + 4860 = 5212
 * Downsize p1  : 352 + 4320 = 4672
 */
#define PRE_CAL_TOTAL_CR_NUM     (PRE_CAL_2_4G_CR_NUM + PRE_CAL_5G_CR_NUM)

/*
 * TOTAL SIZE   : 2.4G Group + 5G Group size
 * Fullsize     : 5212 * 4 = 20848 Bytes
 * Downsize p1  : 4672 * 4 = 18688 Bytes
 */
#define PRE_CAL_TOTAL_SIZE       (PRE_CAL_TOTAL_CR_NUM * 4)


/* DPD & Flatness item */
#define PER_CHAN_FLATNESS_CR_NUM ((5 + 1) * 2)
#if (CFG_PRE_CAL_MT7663_DOWNSIZE_PART2 == 1)
#define PER_CHAN_DPD_2G_CR_NUM   ((50 + 1) * 2)
#define PER_CHAN_DPD_5G_CR_NUM   ((34 + 1) * 2)
#define DPD_FLATNESS_2G_CAL_SIZE ((PER_CHAN_DPD_2G_CR_NUM + PER_CHAN_FLATNESS_CR_NUM) * 4) /* 456 Bytes */
#define DPD_FLATNESS_5G_CAL_SIZE ((PER_CHAN_DPD_5G_CR_NUM + PER_CHAN_FLATNESS_CR_NUM) * 4) /* 328 Bytes */
#else
#define PER_CHAN_DPD_CR_NUM      ((130 + 1) * 2)
#define DPD_FLATNESS_CAL_SIZE    ((PER_CHAN_DPD_CR_NUM + PER_CHAN_FLATNESS_CR_NUM) * 4) /* 1096 Bytes */
#endif
#define DPD_FLATNESS_5G_CHAN_NUM 24
#define DPD_FLATNESS_2G_CHAN_NUM 3
#define TOTAL_CHAN_FOR_DPD_CAL   (DPD_FLATNESS_5G_CHAN_NUM + DPD_FLATNESS_2G_CHAN_NUM)
#if (CFG_PRE_CAL_MT7663_DOWNSIZE_PART2 == 1)
#define DPD_CAL_5G_TOTAL_SIZE    (DPD_FLATNESS_5G_CAL_SIZE * DPD_FLATNESS_5G_CHAN_NUM)
#define DPD_CAL_2G_TOTAL_SIZE    (DPD_FLATNESS_2G_CAL_SIZE * DPD_FLATNESS_2G_CHAN_NUM)
#define DPD_CAL_TOTAL_SIZE       (DPD_CAL_5G_TOTAL_SIZE + DPD_CAL_2G_TOTAL_SIZE)
#else
#define DPD_CAL_5G_TOTAL_SIZE    (DPD_FLATNESS_CAL_SIZE * DPD_FLATNESS_5G_CHAN_NUM)
#define DPD_CAL_2G_TOTAL_SIZE    (DPD_FLATNESS_CAL_SIZE * DPD_FLATNESS_2G_CHAN_NUM)
#define DPD_CAL_TOTAL_SIZE       (DPD_FLATNESS_CAL_SIZE * TOTAL_CHAN_FOR_DPD_CAL)
#endif


/* Flast offset */
#define PRE_CAL_FLASH_OFFSET     (CAL_FLASH_OFFSET)
/*
 * DPD FLASH OFFSET : The start address that Flash used to store the DPD data
 * Fullsize         : 1552(0x610) + 20848(0x5170) = 22272(0x5780)
 * Downsize part1   : 1552(0x610) + 18688(0x4900) = 20240(0x4F10)
 */
#define DPD_FLASH_OFFSET         (CAL_FLASH_OFFSET + PRE_CAL_TOTAL_SIZE)


/* Length limitation from Host to Firmware */
#define PRE_CAL_SET_MAX_CR_NUM   800 /* 800*4 = 3200 Bytes, set this value due to Host can't send more than some value to N9 Firmware */
#define PRE_CAL_SET_MAX_LENGTH   (PRE_CAL_SET_MAX_CR_NUM * 4)

extern UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_FREQ[];
extern UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_FREQ[];
extern UINT16 MT7663_DPD_FLATNESS_BW20_FREQ[];
extern UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_SIZE;
extern UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_SIZE;
extern UINT16 MT7663_DPD_FLATNESS_BW20_FREQ_SIZE;

extern UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_CH[];
extern UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_CH[];
extern UINT16 MT7663_DPD_FLATNESS_BW20_CH[];
extern UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_CH_SIZE;
extern UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_CH_SIZE;
extern UINT16 MT7663_DPD_FLATNESS_B20_CH_SIZE;
#endif /*PRE_CAL_MT7663_SUPPORT*/

#endif /* __MT7663_H__ */

