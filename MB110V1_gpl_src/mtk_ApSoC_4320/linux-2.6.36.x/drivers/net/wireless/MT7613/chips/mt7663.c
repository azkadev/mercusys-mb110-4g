/*
 ***************************************************************************
 * MediaTek Inc.
 *
 * All rights reserved. source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mt7663.c
*/

#include "rt_config.h"
#include "bgnd_scan_cmm.h"
#include "chip/mt7663_cr.h"
#ifndef REDUCE_KO_SIZE
#include "mcu/mt7663_firmware.h"
#endif
#include "mcu/mt7663_firmware_e2.h"
#ifdef NEED_ROM_PATCH
#ifndef REDUCE_KO_SIZE
#include "mcu/mt7663_rom_patch_e1.h"
#endif
#include "mcu/mt7663_rom_patch_e2.h"
#endif /* NEED_ROM_PATCH */
#ifndef REDUCE_KO_SIZE
#include "eeprom/mt7663_e2p.h"
#endif

#ifdef CONFIG_AP_SUPPORT
#define DEFAULT_BIN_FILE "/etc_ro/wlan/MT7663E_EEPROM1.bin"
#else
#define DEFAULT_BIN_FILE "/etc/MT7663E_EEPROM1.bin"
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_RT_SECOND_CARD
#define SECOND_BIN_FILE "/etc_ro/wlan/MT7663E_EEPROM2.bin"
#endif /* CONFIG_RT_SECOND_CARD */
#ifdef CONFIG_RT_THIRD_CARD
#define THIRD_BIN_FILE "/etc_ro/wlan/MT7663E_EEPROM3.bin"
#endif /* CONFIG_RT_THIRD_CARD */

const struct hif_pci_ring_desc mt7663_tx_ring_layout[] = {
#ifdef MEMORY_OPTIMIZATION
	{0, 0,	CONN_HIF_PDMA_TX_RING0_BASE,	256, TX_RING_DATA}, /* AC0 */
	{0, 1,	CONN_HIF_PDMA_TX_RING1_BASE,	256, TX_RING_DATA}, /* AC1 */
	{0, 2,	CONN_HIF_PDMA_TX_RING2_BASE,	256, TX_RING_DATA}, /* AC2 */
	{0, 3,	CONN_HIF_PDMA_TX_RING3_BASE,	64, TX_RING_FWDL},
	{0, 4,	CONN_HIF_PDMA_TX_RING4_BASE,	256, TX_RING_DATA}, /* AC3 */
	{0, 5,	CONN_HIF_PDMA_TX_RING5_BASE,	128, TX_RING_DATA}, /* ALTX */
#else
	{0, 0,	CONN_HIF_PDMA_TX_RING0_BASE,	512, TX_RING_DATA}, /* AC0 */
	{0, 1,	CONN_HIF_PDMA_TX_RING1_BASE,	512, TX_RING_DATA}, /* AC1 */
	{0, 2,	CONN_HIF_PDMA_TX_RING2_BASE,	512, TX_RING_DATA}, /* AC2 */
	{0, 3,	CONN_HIF_PDMA_TX_RING3_BASE,	128, TX_RING_FWDL},
	{0, 4,	CONN_HIF_PDMA_TX_RING4_BASE,	512, TX_RING_DATA}, /* AC3 */
	{0, 5,	CONN_HIF_PDMA_TX_RING5_BASE,	512, TX_RING_DATA}, /* ALTX */
#endif
#ifdef RANDOM_PKT_GEN
	{0, 6,	CONN_HIF_PDMA_TX_RING6_BASE,	512, TX_RING_DATA},
	{0, 7,	CONN_HIF_PDMA_TX_RING7_BASE,	512, TX_RING_DATA},
	{0, 8,	CONN_HIF_PDMA_TX_RING8_BASE,	512, TX_RING_DATA},
	{0, 9,	CONN_HIF_PDMA_TX_RING9_BASE,	512, TX_RING_DATA},
	{0, 10,	CONN_HIF_PDMA_TX_RING10_BASE,	512, TX_RING_DATA},
	{0, 11,	CONN_HIF_PDMA_TX_RING11_BASE,	512, TX_RING_DATA},
	{0, 12,	CONN_HIF_PDMA_TX_RING12_BASE,	512, TX_RING_DATA},
#endif
#ifdef MEMORY_OPTIMIZATION
	{0, 15, CONN_HIF_PDMA_TX_RING15_BASE,	64, TX_RING_CMD}
#else
	{0, 15, CONN_HIF_PDMA_TX_RING15_BASE,	128, TX_RING_CMD}
#endif
};
#define MT7663_TX_RING_NUM	ARRAY_SIZE(mt7663_tx_ring_layout)

const struct hif_pci_ring_desc mt7663_rx_ring_layout[] = {
#ifdef MEMORY_OPTIMIZATION
	{0, 0, CONN_HIF_PDMA_RX_RING0_BASE, 128, RX_RING_DATA},
	{0, 1, CONN_HIF_PDMA_RX_RING1_BASE, 128, RX_RING_EVENT}
#else
	{0, 0, CONN_HIF_PDMA_RX_RING0_BASE, 512, RX_RING_DATA},
	{0, 1, CONN_HIF_PDMA_RX_RING1_BASE, 512, RX_RING_EVENT}
#endif
};
#define MT7663_RX_RING_NUM	ARRAY_SIZE(mt7663_rx_ring_layout)

#ifdef PRE_CAL_MT7663_SUPPORT
UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_FREQ[] = {
	5180, 5200, 5220, 5240, 5260, 5280, 5300, 5320,
	5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640,
	5660, 5680, 5700, 5745, 5765, 5785, 5805, 5825};
UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_FREQ[] = {2422, 2442, 2462};
UINT16 MT7663_DPD_FLATNESS_BW20_FREQ[] = {
	5180, 5200, 5220, 5240, 5260, 5280, 5300, 5320,
	5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640,
	5660, 5680, 5700, 5745, 5765, 5785, 5805, 5825, 2422, 2442, 2462};

UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_CH[] = {
	 36,  40,  44,  48,  52,  56,  60,  64,
	100, 104, 108, 112, 116, 120, 124, 128,
	132, 136, 140, 149, 153, 157, 161, 165};
UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_CH[] = {3, 7, 11};
UINT16 MT7663_DPD_FLATNESS_BW20_CH[] = {
	 36,  40,  44,  48,  52,  56,  60,  64,
	100, 104, 108, 112, 116, 120, 124, 128,
	132, 136, 140, 149, 153, 157, 161, 165, 3, 7, 11};

UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_SIZE = (sizeof(MT7663_DPD_FLATNESS_ABAND_BW20_FREQ) / sizeof(UINT16));
UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_SIZE = (sizeof(MT7663_DPD_FLATNESS_GBAND_BW20_FREQ) / sizeof(UINT16));
UINT16 MT7663_DPD_FLATNESS_BW20_FREQ_SIZE  = (sizeof(MT7663_DPD_FLATNESS_BW20_FREQ) / sizeof(UINT16));

UINT16 MT7663_DPD_FLATNESS_ABAND_BW20_CH_SIZE = (sizeof(MT7663_DPD_FLATNESS_ABAND_BW20_CH) / sizeof(UINT16));
UINT16 MT7663_DPD_FLATNESS_GBAND_BW20_CH_SIZE = (sizeof(MT7663_DPD_FLATNESS_GBAND_BW20_CH) / sizeof(UINT16));
UINT16 MT7663_DPD_FLATNESS_B20_CH_SIZE        = (sizeof(MT7663_DPD_FLATNESS_BW20_CH) / sizeof(UINT16));
#endif /* PRE_CAL_MT7663_SUPPORT */

#ifdef INTERNAL_CAPTURE_SUPPORT
extern RBIST_DESC_T MT7663_ICAP_DESC[];
extern UINT8 MT7663_ICapBankNum;
#endif /* INTERNAL_CAPTURE_SUPPORT */

#ifdef WIFI_SPECTRUM_SUPPORT
extern RBIST_DESC_T MT7663_SPECTRUM_DESC[];
extern UINT8 MT7663_SpectrumBankNum;
#endif /* WIFI_SPECTRUM_SUPPORT */

#ifdef MT7663_FPGA
REG_CHK_PAIR hif_dft_cr[] = {
	{HIF_BASE + 0x00, 0xffffffff, 0x76030001},
	{HIF_BASE + 0x04, 0xffffffff, 0x1b},
	{HIF_BASE + 0x10, 0xffffffff, 0x3f01},
	{HIF_BASE + 0x20, 0xffffffff, 0xe01001e0},
	{HIF_BASE + 0x24, 0xffffffff, 0x1e00000f},

	{HIF_BASE + 0x200, 0xffffffff, 0x0},
	{HIF_BASE + 0x204, 0xffffffff, 0x0},
	{HIF_BASE + 0x208, 0xffffffff, 0x10001870},
	{HIF_BASE + 0x20c, 0xffffffff, 0x0},
	{HIF_BASE + 0x210, 0xffffffff, 0x0},
	{HIF_BASE + 0x214, 0xffffffff, 0x0},
	{HIF_BASE + 0x218, 0xffffffff, 0x0},
	{HIF_BASE + 0x21c, 0xffffffff, 0x0},
	{HIF_BASE + 0x220, 0xffffffff, 0x0},
	{HIF_BASE + 0x224, 0xffffffff, 0x0},
	{HIF_BASE + 0x234, 0xffffffff, 0x0},
	{HIF_BASE + 0x244, 0xffffffff, 0x0},
	{HIF_BASE + 0x300, 0xffffffff, 0x0},
	{HIF_BASE + 0x304, 0xffffffff, 0x0},
	{HIF_BASE + 0x308, 0xffffffff, 0x0},
	{HIF_BASE + 0x30c, 0xffffffff, 0x0},
	{HIF_BASE + 0x310, 0xffffffff, 0x0},
	{HIF_BASE + 0x314, 0xffffffff, 0x0},
	{HIF_BASE + 0x318, 0xffffffff, 0x0},
	{HIF_BASE + 0x31c, 0xffffffff, 0x0},
	{HIF_BASE + 0x320, 0xffffffff, 0x0},
	{HIF_BASE + 0x324, 0xffffffff, 0x0},
	{HIF_BASE + 0x328, 0xffffffff, 0x0},
	{HIF_BASE + 0x32c, 0xffffffff, 0x0},
	{HIF_BASE + 0x330, 0xffffffff, 0x0},
	{HIF_BASE + 0x334, 0xffffffff, 0x0},
	{HIF_BASE + 0x338, 0xffffffff, 0x0},
	{HIF_BASE + 0x33c, 0xffffffff, 0x0},

	{HIF_BASE + 0x400, 0xffffffff, 0x0},
	{HIF_BASE + 0x404, 0xffffffff, 0x0},
	{HIF_BASE + 0x408, 0xffffffff, 0x0},
	{HIF_BASE + 0x40c, 0xffffffff, 0x0},
	{HIF_BASE + 0x410, 0xffffffff, 0x0},
	{HIF_BASE + 0x414, 0xffffffff, 0x0},
	{HIF_BASE + 0x418, 0xffffffff, 0x0},
	{HIF_BASE + 0x41c, 0xffffffff, 0x0},
};

INT mt7663_chk_hif_default_cr_setting(RTMP_ADAPTER *pAd)
{
	UINT32 val;
	INT i;
	BOOLEAN match = TRUE;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Default CR Setting Checking for HIF!\n", __func__));

	for (i = 0; i < sizeof(hif_dft_cr) / sizeof(REG_CHK_PAIR); i++) {
		RTMP_IO_READ32(pAd->hdev_ctrl, hif_dft_cr[i].Register, &val);
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t Reg(%x): Current=0x%x(0x%x), Default=0x%x, Mask=0x%x, Match=%s\n",
				 hif_dft_cr[i].Register, val, (val & hif_dft_cr[i].Mask),
				 hif_dft_cr[i].Value, hif_dft_cr[i].Mask,
				 ((val & hif_dft_cr[i].Mask) != hif_dft_cr[i].Value) ? "No" : "Yes"));

		if ((val & hif_dft_cr[i].Mask) != hif_dft_cr[i].Value)
			match = FALSE;
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Checking Done, Result=> %s match!\n",
			 __func__, match == TRUE ? "All" : "No"));
	return match;
}


REG_CHK_PAIR top_dft_cr[] = {
	{TOP_CFG_BASE + 0x1000, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1004, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1008, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1010, 0xffffffff, 0x0},

	{TOP_CFG_BASE + 0x1100, 0xffffffff, 0x26110310},
	{TOP_CFG_BASE + 0x1108, 0x0000ff00, 0x1400},
	{TOP_CFG_BASE + 0x110c, 0x00000000, 0x0},
	{TOP_CFG_BASE + 0x1110, 0x0f0f00ff, 0x02090040},
	{TOP_CFG_BASE + 0x1124, 0xf000f00f, 0x00000008},
	{TOP_CFG_BASE + 0x1130, 0x000f0000, 0x0},
	{TOP_CFG_BASE + 0x1134, 0x00000000, 0x0},
	{TOP_CFG_BASE + 0x1140, 0x00ff00ff, 0x0},

	{TOP_CFG_BASE + 0x1200, 0x00000000, 0x0},
	{TOP_CFG_BASE + 0x1204, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1208, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x120c, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1210, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1214, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1218, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x121c, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1220, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1224, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1228, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x122c, 0x000fffff, 0x0},
	{TOP_CFG_BASE + 0x1234, 0x00ffffff, 0x0},
	{TOP_CFG_BASE + 0x1238, 0x00ffffff, 0x0},
	{TOP_CFG_BASE + 0x123c, 0xffffffff, 0x5c1fee80},
	{TOP_CFG_BASE + 0x1240, 0xffffffff, 0x6874ae05},
	{TOP_CFG_BASE + 0x1244, 0xffffffff, 0x00fb89f1},

	{TOP_CFG_BASE + 0x1300, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1304, 0xffffffff, 0x8f020006},
	{TOP_CFG_BASE + 0x1308, 0xffffffff, 0x18010000},
	{TOP_CFG_BASE + 0x130c, 0xffffffff, 0x0130484f},
	{TOP_CFG_BASE + 0x1310, 0xffffffff, 0xff000004},
	{TOP_CFG_BASE + 0x1314, 0xffffffff, 0xf0000084},
	{TOP_CFG_BASE + 0x1318, 0x00000000, 0x0},
	{TOP_CFG_BASE + 0x131c, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1320, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1324, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1328, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x132c, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1330, 0xffffffff, 0x00007800},
	{TOP_CFG_BASE + 0x1334, 0x00000000, 0x0},
	{TOP_CFG_BASE + 0x1338, 0xffffffff, 0x0000000a},
	{TOP_CFG_BASE + 0x1400, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1404, 0xffffffff, 0x00005180},
	{TOP_CFG_BASE + 0x1408, 0xffffffff, 0x00001f00},
	{TOP_CFG_BASE + 0x140c, 0xffffffff, 0x00000020},
	{TOP_CFG_BASE + 0x1410, 0xffffffff, 0x0000003a},
	{TOP_CFG_BASE + 0x141c, 0xffffffff, 0x0},

	{TOP_CFG_BASE + 0x1500, 0xffffffff, 0x0},
	{TOP_CFG_BASE + 0x1504, 0xffffffff, 0x0},
};

INT mt7663_chk_top_default_cr_setting(RTMP_ADAPTER *pAd)
{
	UINT32 val;
	INT i;
	BOOLEAN match = TRUE;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Default CR Setting Checking for TOP!\n", __func__));

	for (i = 0; i < sizeof(top_dft_cr) / sizeof(REG_CHK_PAIR); i++) {
		RTMP_IO_READ32(pAd->hdev_ctrl, top_dft_cr[i].Register, &val);
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\t Reg(%x): Current=0x%x(0x%x), Default=0x%x, Mask=0x%x, Match=%s\n",
				 top_dft_cr[i].Register, val, (val & top_dft_cr[i].Mask),
				 top_dft_cr[i].Value, top_dft_cr[i].Mask,
				 ((val & top_dft_cr[i].Mask) != top_dft_cr[i].Value) ? "No" : "Yes"));

		if ((val & top_dft_cr[i].Mask) != top_dft_cr[i].Value)
			match = FALSE;
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): Checking Done, Result=> %s match!\n",
			 __func__, match == TRUE ? "All" : "No"));
	return match;
}
#endif /* MT7663_FPGA */


static VOID mt7663_bbp_adjust(RTMP_ADAPTER *pAd, UCHAR Channel)
{
	/*do nothing, change to use radio_resource control*/
	/*here should do bbp setting only, bbp is full-offload to fw*/
}

#ifdef PRE_CAL_MT7663_SUPPORT
void mt7663_apply_dpd_flatness_data(RTMP_ADAPTER *pAd, MT_SWITCH_CHANNEL_CFG SwChCfg)
{
	USHORT			doCal1 = 0;
	UINT8			i = 0;
	UINT8			Band = 0;
	UINT16			CentralFreq = 0;

	if (pAd->E2pAccessMode != E2P_FLASH_MODE && pAd->E2pAccessMode != E2P_BIN_MODE) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("%s: Currently not in FLASH or BIN MODE,return.\n", __func__));
		return;
	}

#ifdef RTMP_FLASH_SUPPORT
	if (pAd->E2pAccessMode == E2P_FLASH_MODE) {
		rtmp_ee_flash_read(pAd, 0x52, &doCal1);
	}
#endif

	if (((doCal1 & (1 << 1)) != 0) && ((doCal1 & (1 << 2)) != 0)) {
		if (SwChCfg.CentralChannel == 14) {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
					 ("%s: CH 14 don't need DPD , return!!!\n", __func__));
			return;
		} else if (SwChCfg.CentralChannel < 14) {
			Band = GBAND;

			if (SwChCfg.CentralChannel >= 1 && SwChCfg.CentralChannel <= 4)
				CentralFreq = 2422;
			else if (SwChCfg.CentralChannel >= 5 && SwChCfg.CentralChannel <= 9)
				CentralFreq = 2442;
			else if (SwChCfg.CentralChannel >= 10 && SwChCfg.CentralChannel <= 13)
				CentralFreq = 2462;
			else
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("%s: can't find cent freq for CH %d , should not happen!!!\n",
						  __func__, SwChCfg.CentralChannel));
		} else {
			Band = ABAND;

			if (SwChCfg.Bw == BW_20) {
				CentralFreq = SwChCfg.CentralChannel * 5 + 5000;
			} else if (SwChCfg.Bw == BW_160 || SwChCfg.Bw == BW_8080) {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 ("%s: MT7663 not support BW8080 or BW160. return\n", __func__));
				return;
			} else {
				UINT32 Central = SwChCfg.CentralChannel * 5 + 5000;
				UINT32 CentralMinus10M = (SwChCfg.CentralChannel - 2) * 5 + 5000;

				if (ChannelFreqToGroup(Central) != ChannelFreqToGroup(CentralMinus10M)) {
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
							 ("==== Different Group Central %d @ group %d Central-10 @ group %d !!\n"
							  , Central, ChannelFreqToGroup(Central), ChannelFreqToGroup(CentralMinus10M)));
					CentralFreq = (SwChCfg.CentralChannel + 2) * 5 + 5000;
				} else
					CentralFreq = (SwChCfg.CentralChannel - 2) * 5 + 5000;
			}
		}
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s: eeprom 0x52 bit 0 is 0, do runtime cal , skip TX reload\n", __func__));
		return;
	}

	/* Find if CentralFreq is exist in DPD+Flatness pre-k table */
	for (i = 0; i < MT7663_DPD_FLATNESS_BW20_FREQ_SIZE; i++) {
		if (MT7663_DPD_FLATNESS_BW20_FREQ[i] == CentralFreq) {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s: %d is in DPD-Flatness cal table, index = %d\n", __func__, CentralFreq, i));
			break;
		}
	}

	if (i == MT7663_DPD_FLATNESS_BW20_FREQ_SIZE) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s: Unexpected freq (%d)\n", __func__, CentralFreq));
		return;
	}

#if (CFG_PRE_CAL_MT7663_DOWNSIZE_PART2 == 1)
	if (Band == GBAND) {
		MtCmdSetDpdFlatnessCal_7663(pAd, i, DPD_FLATNESS_2G_CAL_SIZE);
	} else {
		MtCmdSetDpdFlatnessCal_7663(pAd, i, DPD_FLATNESS_5G_CAL_SIZE);
	}
#else
	MtCmdSetDpdFlatnessCal_7663(pAd, i, DPD_FLATNESS_CAL_SIZE);
#endif
}
#endif /* PRE_CAL_MT7663_SUPPORT */

/* TODO: Star */
static void mt7663_switch_channel(RTMP_ADAPTER *pAd, MT_SWITCH_CHANNEL_CFG SwChCfg)
{
	/* update power limit table */
	MtPwrLimitTblChProc(pAd, SwChCfg.BandIdx, SwChCfg.Channel_Band, SwChCfg.ControlChannel, SwChCfg.CentralChannel);

#ifdef PRE_CAL_MT7663_SUPPORT
    mt7663_apply_dpd_flatness_data(pAd, SwChCfg);
#endif

	MtCmdChannelSwitch(pAd, SwChCfg);
	MtCmdSetTxRxPath(pAd, SwChCfg);
	pAd->LatchRfRegs.Channel = SwChCfg.CentralChannel;
}

#ifdef NEW_SET_RX_STREAM
static INT mt7663_set_RxStream(RTMP_ADAPTER *pAd, UINT32 StreamNums, UCHAR BandIdx)
{
	UINT32 path = 0;
	UINT i;

	if (StreamNums > 2) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 ("%s():illegal StreamNums(%d)\n",
				  __func__, StreamNums));
		StreamNums = 2;
	}

	for (i = 0; i < StreamNums; i++)
		path |= 1 << i;

	return MtCmdSetRxPath(pAd, path, BandIdx);
}
#endif

static inline VOID bufferModeFieldSet(RTMP_ADAPTER *pAd, EXT_CMD_EFUSE_BUFFER_MODE_T *pCmd, UINT16 addr)
{
	pCmd->BinContent[addr] = pAd->EEPROMImage[addr];
}


static VOID mt7663_bufferModeEfuseFill(RTMP_ADAPTER *pAd, EXT_CMD_EFUSE_BUFFER_MODE_T *pCmd)
{
	RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT16 i = 0;

	pCmd->ucCount = cap->EFUSE_BUFFER_CONTENT_SIZE;
	os_zero_mem(&pCmd->BinContent[0], cap->EFUSE_BUFFER_CONTENT_SIZE);

	for (i = 0x0; i <= EFUSE_CONTENT_END; i++)
		bufferModeFieldSet(pAd, pCmd, i);
}

#ifdef CAL_FREE_IC_SUPPORT
static UINT32 ICAL[] = {
			/* WF D-die */
			0x4A
			};
static UINT32 ICAL_NUM = (sizeof(ICAL) / sizeof(UINT32));

static UINT32 ICAL_JUST_MERGE[] = {
				/* WF D-die */
				0x4B, 0x4C, 0x4E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x128, 0x129,
				0x12A, 0x12B, 0x12C, 0x12D, 0x12E, 0x12F, 0x1B8, 0x1B9, 0x1BA, /*0x1BB,*/ 0x1BC, 0x1BD,
				/* WF A-die */
				0x42F, 0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437, 0x438,
				/* BT A-Die */
				0x550, 0x551, 0x552, 0x557, 0x558
				};
static UINT32 ICAL_JUST_MERGE_NUM = (sizeof(ICAL_JUST_MERGE) / sizeof(UINT32));

static inline BOOLEAN check_valid(RTMP_ADAPTER *pAd, UINT16 Offset)
{
	UINT16 Value = 0;
	BOOLEAN NotValid;

	if ((Offset % 2) != 0) {
		NotValid = rtmp_ee_efuse_read16(pAd, Offset - 1, &Value);

		if (NotValid)
			return FALSE;

		if (((Value >> 8) & 0xff) == 0x00)
			return FALSE;

		NotValid = rtmp_ee_efuse_read16(pAd, Offset + 1, &Value);

		if (NotValid)
			return FALSE;

		if ((Value & 0xff) == 0x00)
			return FALSE;
	} else {
		NotValid = rtmp_ee_efuse_read16(pAd, Offset, &Value);

		if (NotValid)
			return FALSE;

		if (Value == 0x00)
			return FALSE;
	}

	return TRUE;
}
static BOOLEAN mt7663_is_cal_free_ic(RTMP_ADAPTER *pAd)
{
	UINT32 i;

	for (i = 0; i < ICAL_NUM; i++)
		if (check_valid(pAd, ICAL[i]) == FALSE) {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s():check_valid fail ICAL[%d]=%x!\n", __func__, i, ICAL[i]));
			return FALSE;
		}


	return TRUE;

}

static inline VOID cal_free_data_get_from_addr(RTMP_ADAPTER *ad, UINT16 Offset)
{
	UINT16 value;

	if ((Offset % 2) != 0) {
		rtmp_ee_efuse_read16(ad, Offset - 1, &value);
		ad->EEPROMImage[Offset] = (value >> 8) & 0xFF;
	} else {
		rtmp_ee_efuse_read16(ad, Offset, &value);
		ad->EEPROMImage[Offset] =  value & 0xFF;
	}
}

static VOID mt7663_cal_free_data_get(RTMP_ADAPTER *ad)

{
	UINT32 i;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s\n", __func__));

	for (i = 0; i < ICAL_NUM; i++)
		cal_free_data_get_from_addr(ad, ICAL[i]);

	for (i = 0; i < ICAL_JUST_MERGE_NUM; i++)
		cal_free_data_get_from_addr(ad, ICAL_JUST_MERGE[i]);

}
#endif /* CAL_FREE_IC_SUPPORT */

static INT32 mt7663_dma_shdl_init(RTMP_ADAPTER *pAd)
{
	UINT32 value;
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);

	HIF_DMASHDL_IO_READ32(pAd, MT_HIF_DMASHDL_PKT_MAX_SIZE, &value);
	value &= ~(PLE_PKT_MAX_SIZE_MASK | PSE_PKT_MAX_SIZE_MASK);
	value |= PLE_PKT_MAX_SIZE_NUM(0x1);
	value |= PSE_PKT_MAX_SIZE_NUM(0x8);
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_PKT_MAX_SIZE, value);

	/* Enable refill Control Group 0, 1, 2, 4, 5 */
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_REFILL_CTRL, 0xffc80000);
	/* Group 0, 1, 2, 4, 5, 15 joint the ask round robin */
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_OPTION_CTRL, 0x70068037);
	/*Each group min quota must larger then PLE_PKT_MAX_SIZE_NUM*/
	value = DMASHDL_MIN_QUOTA_NUM(0x40);
	value |= DMASHDL_MAX_QUOTA_NUM(0x800);

	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_GROUP0_CTRL, value);
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_GROUP1_CTRL, value);
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_GROUP2_CTRL, value);
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_GROUP4_CTRL, value);
	value = DMASHDL_MIN_QUOTA_NUM(0x40);
	value |= DMASHDL_MAX_QUOTA_NUM(0x40);
	HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_GROUP5_CTRL, value);

	if ((cap->qm == GENERIC_QM) || (cap->qm == GENERIC_FAIR_QM)) {
		HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_Q_MAP0, 0x42104210);
		HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_Q_MAP1, 0x42104210);
		HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_Q_MAP2, 0x00000005);
		HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_Q_MAP3, 0x0);
		HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_SHDL_SET0, 0x6012345f);
		HIF_DMASHDL_IO_WRITE32(pAd, MT_HIF_DMASHDL_SHDL_SET1, 0xedcba987);
	}
	return TRUE;
}

#ifdef CFG_SUPPORT_MU_MIMO
#ifdef MANUAL_MU
INT mu_update_profile_tb(RTMP_ADAPTER *pAd, INT profile_id, UCHAR wlan_id)
{
}

INT mu_update_grp_table(RTMP_ADAPTER *pAd, INT grp_id)
{
	return TRUE;
}


INT mu_update_cluster_tb(RTMP_ADAPTER *pAd, UCHAR c_id, UINT32 *m_ship, UINT32 *u_pos)
{
	UINT32 entry_base, mac_val, offset;

	ASSERT(c_id <= 31);
	MAC_IO_READ32(pAd->hdev_ctrl, MU_MUCR1, &mac_val);

	if (c_id < 16)
		mac_val &= (~MUCR1_CLUSTER_TAB_REMAP_CTRL_MASK);
	else
		mac_val |= MUCR1_CLUSTER_TAB_REMAP_CTRL_MASK;

	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_MUCR1, mac_val);
	entry_base = MU_CLUSTER_TABLE_BASE  + (c_id & (~0x10)) * 24;
	/* update membership */
	MAC_IO_WRITE32(pAd->hdev_ctrl, entry_base + 0x0, m_ship[0]);
	MAC_IO_WRITE32(pAd->hdev_ctrl, entry_base + 0x4, m_ship[1]);
	/* Update user position */
	MAC_IO_WRITE32(pAd->hdev_ctrl, entry_base + 0x8, u_pos[0]);
	MAC_IO_WRITE32(pAd->hdev_ctrl, entry_base + 0xc, u_pos[1]);
	MAC_IO_WRITE32(pAd->hdev_ctrl, entry_base + 0x10, u_pos[2]);
	MAC_IO_WRITE32(pAd->hdev_ctrl, entry_base + 0x14, u_pos[3]);
	return TRUE;
}


INT mu_get_wlanId_ac_len(RTMP_ADAPTER *pAd, UINT32 wlan_id, UINT ac)
{
	return TRUE;
}


INT mu_get_mu_tx_retry_cnt(RTMP_ADAPTER *pAd)
{
	return TRUE;
}


INT mu_get_pfid_tx_stat(RTMP_ADAPTER *pAd)
{
}

INT mu_get_gpid_rate_per_stat(RTMP_ADAPTER *pAd)
{
	return TRUE;
}


INT mt7663_mu_init(RTMP_ADAPTER *pAd)
{
	UINT32 mac_val;
	/****************************************************************************
		MU Part
	****************************************************************************/
	/* After power on initial setting,  AC legnth clear */
	MAC_IO_READ32(pAd->hdev_ctrl, MU_MUCR4, &mac_val);
	mac_val = 0x1;
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_MUCR4, mac_val); /* 820fe010= 0x0000_0001 */
	/* PFID table */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_PROFILE_TABLE_BASE + 0x0, 0x1e000);  /* 820fe780= 0x0001_e000 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_PROFILE_TABLE_BASE + 0x4, 0x1e103);  /* 820fe784= 0x0001_e103 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_PROFILE_TABLE_BASE + 0x8, 0x1e205);  /* 820fe788= 0x0001_e205 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_PROFILE_TABLE_BASE + 0xc, 0x1e306);  /* 820fe78c= 0x0001_e306 */
	/* Cluster table */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x0, 0x0);  /* 820fe400= 0x0000_0000 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x8, 0x0);  /* 820fe408= 0x0000_0000 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x20, 0x2);  /* 820fe420= 0x0000_0002 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x28, 0x0);  /* 820fe428= 0x0000_0000 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x40, 0x2);  /* 820fe440= 0x0000_0002 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x48, 0x4);  /* 820fe448= 0x0000_0004 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x60, 0x0);  /* 820fe460= 0x0000_0000 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_CLUSTER_TABLE_BASE + 0x68, 0x0);  /* 820fe468= 0x0000_0000 */
	/* Group rate table */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_GRP_TABLE_RATE_MAP + 0x0, 0x4109);  /* 820ff000= 0x0000_4109 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_GRP_TABLE_RATE_MAP + 0x4, 0x99);  /* 820ff004= 0x0000_0099 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_GRP_TABLE_RATE_MAP + 0x8, 0x800000f0);  /* 820ff008= 0x8000_00f0 */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_GRP_TABLE_RATE_MAP + 0xc, 0x99);  /* 820ff00c= 0x0000_0099 */
	/* SU Tx minimum setting */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_MUCR2, 0x10000001);  /* 820fe008= 0x1000_0001 */
	/* MU max group search entry = 1 group entry */
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_MUCR1, 0x0);  /* 820fe004= 0x0000_0000 */
	/* MU enable */
	MAC_IO_READ32(pAd->hdev_ctrl, MU_MUCR0, &mac_val);
	mac_val |= 1;
	MAC_IO_WRITE32(pAd->hdev_ctrl, MU_MUCR0, 0x1);  /* 820fe000= 0x1000_0001 */
	/****************************************************************************
		M2M Part
	****************************************************************************/
	/* Enable M2M MU temp mode */
	MAC_IO_READ32(pAd->hdev_ctrl, RMAC_M2M_BAND_CTRL, &mac_val);
	mac_val |= (1 << 16);
	MAC_IO_WRITE32(pAd->hdev_ctrl, RMAC_M2M_BAND_CTRL, mac_val);
	/****************************************************************************
		AGG Part
	****************************************************************************/
	/* 820f20e0[15] = 1 or 0 all need to be verified, because
	    a). if primary is the fake peer, and peer will not ACK to us, cannot setup the TxOP
	    b). Or can use CTS2Self to setup the TxOP
	*/
	MAC_IO_READ32(pAd->hdev_ctrl, AGG_MUCR, &mac_val);
	mac_val &= (~MUCR_PRIM_BAR_MASK);
	/* mac_val |= (1 << MUCR_PRIM_BAR_BIT); */
	MAC_IO_WRITE32(pAd->hdev_ctrl, AGG_MUCR, mac_val);  /* 820fe000= 0x1000_0001 */
	return TRUE;
}
#endif /* MANUAL_MU */
#endif /* CFG_SUPPORT_MU_MIMO */

#ifndef MAC_INIT_OFFLOAD
#endif /* MAC_INIT_OFFLOAD */

/* need to confirm with DE, wilsonl */
static VOID mt7663_init_mac_cr(RTMP_ADAPTER *pAd)
{
	UINT32 mac_val;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s()-->\n", __func__));
#ifndef MAC_INIT_OFFLOAD
	/* need to confirm with DE, wilsonl */
	/* done, confirmed by Austin */
	/* Set TxFreeEvent packet only go through CR4 */
	HW_IO_READ32(pAd->hdev_ctrl, PLE_HIF_REPORT, &mac_val);
	mac_val |= 0x1;
	HW_IO_WRITE32(pAd->hdev_ctrl, PLE_HIF_REPORT, mac_val);
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s(): Set TxRxEventPkt path 0x%0x = 0x%08x\n",
			  __func__, PLE_HIF_REPORT, mac_val));
	HW_IO_READ32(pAd->hdev_ctrl, PP_PAGECTL_2, &mac_val);
	mac_val &= ~(PAGECTL_2_CUT_PG_CNT_MASK);
	mac_val |= 0x30;
	HW_IO_WRITE32(pAd->hdev_ctrl, PP_PAGECTL_2, mac_val);
#if defined(COMPOS_WIN) || defined(COMPOS_TESTMODE_WIN)
#else
	/* TxS Setting */
	InitTxSTypeTable(pAd);
#endif
	MtAsicSetTxSClassifyFilter(pAd, TXS2HOST, TXS2H_QID1, TXS2HOST_AGGNUMS, 0x00, 0);
#endif /*MAC_INIT_OFFLOAD*/
	/* MAC D0 2x / MAC D0 1x clock enable */
	MAC_IO_READ32(pAd->hdev_ctrl, CFG_CCR, &mac_val);
	mac_val |= (BIT31 | BIT25);
	MAC_IO_WRITE32(pAd->hdev_ctrl, CFG_CCR, mac_val);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("%s: MAC D0 2x 1x initial(val=%x)\n", __func__, mac_val));
	/*  Disable RX Header Translation */
	MAC_IO_READ32(pAd->hdev_ctrl, DMA_DCR0, &mac_val);
	mac_val &= ~(DMA_DCR0_RX_HDR_TRANS_EN_BIT | DMA_DCR0_RX_HDR_TRANS_MODE_BIT |
				 DMA_DCR0_RX_RM_VLAN_BIT | DMA_DCR0_RX_INS_VLAN_BIT |
				 DMA_DCR0_RX_HDR_TRANS_CHK_BSSID);
#ifdef HDR_TRANS_RX_SUPPORT

	if (IS_ASIC_CAP(pAd, fASIC_CAP_RX_HDR_TRANS)) {
		UINT32 mac_val2;

		mac_val |= DMA_DCR0_RX_HDR_TRANS_EN_BIT | DMA_DCR0_RX_RM_VLAN_BIT |
				   DMA_DCR0_RX_HDR_TRANS_CHK_BSSID;
		/* TODO: UnifiedSW, take care about Windows for translation mode! */
		/* mac_val |= DMA_DCR0_RX_HDR_TRANS_MODE_BIT; */
		MAC_IO_READ32(pAd->hdev_ctrl, DMA_DCR1, &mac_val2);
		mac_val2 |= RHTR_AMS_VLAN_EN;
		MAC_IO_WRITE32(pAd->hdev_ctrl, DMA_DCR1, mac_val2);
	}

#endif /* HDR_TRANS_RX_SUPPORT */
	MAC_IO_WRITE32(pAd->hdev_ctrl, DMA_DCR0, mac_val);
	/* CCA Setting */
	MAC_IO_READ32(pAd->hdev_ctrl, TMAC_TRCR0, &mac_val);
	mac_val &= ~CCA_SRC_SEL_MASK;
	mac_val |= CCA_SRC_SEL(0x2);
	mac_val &= ~CCA_SEC_SRC_SEL_MASK;
	mac_val |= CCA_SEC_SRC_SEL(0x0);
	MAC_IO_WRITE32(pAd->hdev_ctrl, TMAC_TRCR0, mac_val);
	MAC_IO_READ32(pAd->hdev_ctrl, TMAC_TRCR0, &mac_val);
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s(): TMAC_TRCR0=0x%x\n", __func__, mac_val));
	/* ---Add by shiang for MT7615 RFB ED issue */
	/* Set BAR rate as 0FDM 6M default, remove after fw set */
	MAC_IO_WRITE32(pAd->hdev_ctrl, AGG_ACR0, 0x04b10496);
	/*Add by Star for zero delimiter*/
	MAC_IO_READ32(pAd->hdev_ctrl, TMAC_CTCR0, &mac_val);
	mac_val &= ~INS_DDLMT_REFTIME_MASK;
	mac_val |= INS_DDLMT_REFTIME(0x3f);
	mac_val |= DUMMY_DELIMIT_INSERTION;
	mac_val |= INS_DDLMT_DENSITY(3);
	MAC_IO_WRITE32(pAd->hdev_ctrl, TMAC_CTCR0, mac_val);
	/* Temporary setting for RTS */
	/*if no protect should enable for CTS-2-Self, WHQA_00025629*/
		MAC_IO_WRITE32(pAd->hdev_ctrl, AGG_PCR1, 0x0400092b);/* sync MT7615 MP2.1, for RvR tail section */
		MAC_IO_READ32(pAd->hdev_ctrl, AGG_SCR, &mac_val);
		mac_val |= NLNAV_MID_PTEC_DIS;
		MAC_IO_WRITE32(pAd->hdev_ctrl, AGG_SCR, mac_val);
	/*Default disable rf low power beacon mode*/
#define WIFI_SYS_PHY 0x10000
#define RF_LOW_BEACON_BAND0 (WIFI_SYS_PHY+0x1900)
#define RF_LOW_BEACON_BAND1 (WIFI_SYS_PHY+0x1d00)
	PHY_IO_READ32(pAd->hdev_ctrl, RF_LOW_BEACON_BAND0, &mac_val);
	mac_val &= ~(0x3 << 8);
	mac_val |= (0x2 << 8);
	PHY_IO_WRITE32(pAd->hdev_ctrl, RF_LOW_BEACON_BAND0, mac_val);
	PHY_IO_READ32(pAd->hdev_ctrl, RF_LOW_BEACON_BAND1, &mac_val);
	mac_val &= ~(0x3 << 8);
	mac_val |= (0x2 << 8);
	PHY_IO_WRITE32(pAd->hdev_ctrl, RF_LOW_BEACON_BAND1, mac_val);
}






static VOID MT7663BBPInit(RTMP_ADAPTER *pAd)
{
	BOOLEAN isDBDC = FALSE, band_vld[2];
	INT idx, cbw[2] = {0};
	INT cent_ch[2] = {0}, prim_ch[2] = {0}, prim_ch_idx[2] = {0};
	INT band[2] = {0};
	INT txStream[2] = {0};
	UCHAR use_bands;

	band_vld[0] = TRUE;
	cbw[0] = RF_BW_20;
	cent_ch[0] = 1;
	prim_ch[0] = 1;
	band[0] = BAND_24G;
	txStream[0] = 2;
#ifdef DOT11_VHT_AC
	prim_ch_idx[0] = vht_prim_ch_idx(cent_ch[0], prim_ch[0], cbw[0]);
#endif /* DOT11_VHT_AC */

#ifdef DBDC_MODE
	if (isDBDC) {
		band_vld[1] = TRUE;
		band[1] = BAND_5G;
		cbw[1] = RF_BW_20;
		cent_ch[1] = 36;
		prim_ch[1] = 36;
#ifdef DOT11_VHT_AC
		prim_ch_idx[1] = vht_prim_ch_idx(cent_ch[1], prim_ch[1], cbw[1]);
#endif /* DOT11_VHT_AC */
		txStream[1] = 2;
		use_bands = 2;
	} else
#endif
	{
		band_vld[1] = FALSE;
		use_bands = 1;
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("%s():BBP Initialization.....\n", __func__));

	for (idx = 0; idx < 2; idx++) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				 ("\tBand %d: valid=%d, isDBDC=%d, Band=%d, CBW=%d, CentCh/PrimCh=%d/%d, prim_ch_idx=%d, txStream=%d\n",
				  idx, band_vld[idx], isDBDC, band[idx], cbw[idx], cent_ch[idx], prim_ch[idx],
				  prim_ch_idx[idx], txStream[idx]));
	}

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s() todo\n", __func__));
}


static void mt7663_init_rf_cr(RTMP_ADAPTER *ad)
{
}

/* Read power per rate */
void mt7663_get_tx_pwr_per_rate(RTMP_ADAPTER *pAd)
{
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s() todo\n", __func__));
}


void mt7663_get_tx_pwr_info(RTMP_ADAPTER *pAd)
{
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s() todo\n", __func__));
}


static void mt7663_antenna_default_reset(
	struct _RTMP_ADAPTER *pAd,
	EEPROM_ANTENNA_STRUC *pAntenna)
{
	/* TODO: shiang-MT7615 */
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s() todo\n", __func__));
	pAntenna->word = 0;
	pAd->RfIcType = RFIC_7663;

	pAntenna->field.TxPath = pAd->EfuseInfoAll.prSys->u1TxPath;
	pAntenna->field.RxPath = pAd->EfuseInfoAll.prSys->u1RxPath;
	/*pAntenna->field.TxPath = (pAd->EEPROMDefaultValue[EEPROM_NIC_CFG1_OFFSET] >> 4) & 0x0F;*/
	/*pAntenna->field.RxPath = pAd->EEPROMDefaultValue[EEPROM_NIC_CFG1_OFFSET] & 0x0F;*/

	/* protection */
	if (pAntenna->field.TxPath == 0)
		pAntenna->field.TxPath = 2;
	if (pAntenna->field.RxPath == 0)
		pAntenna->field.RxPath = 2;

}


static VOID mt7663_fw_prepare(RTMP_ADAPTER *pAd)
{
	struct fwdl_ctrl *ctrl = &pAd->MCUCtrl.fwdl_ctrl;

#ifdef NEED_ROM_PATCH
#ifndef REDUCE_KO_SIZE
	if (IS_MT7663_FW_VER_E1(pAd)) {
		ctrl->patch_profile[WM_CPU].source.header_ptr = mt7663_rom_patch_e1;
		ctrl->patch_profile[WM_CPU].source.header_len = sizeof(mt7663_rom_patch_e1);
		ctrl->patch_profile[WM_CPU].source.bin_name = MT7663_ROM_PATCH_BIN_FILE_NAME_E1;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E1 ROM patch\n", __func__));
	} else if (IS_MT7663_FW_VER_E2(pAd)) {
		ctrl->patch_profile[WM_CPU].source.header_ptr = mt7663_rom_patch_e2;
		ctrl->patch_profile[WM_CPU].source.header_len = sizeof(mt7663_rom_patch_e2);
		ctrl->patch_profile[WM_CPU].source.bin_name = MT7663_ROM_PATCH_BIN_FILE_NAME_E2;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E2 ROM patch\n", __func__));
	} else {
		ctrl->patch_profile[WM_CPU].source.header_ptr = mt7663_rom_patch_e1;
		ctrl->patch_profile[WM_CPU].source.header_len = sizeof(mt7663_rom_patch_e1);
		ctrl->patch_profile[WM_CPU].source.bin_name = MT7663_ROM_PATCH_BIN_FILE_NAME_E1;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E1 ROM patch as default\n", __func__));
	}
#else
    if (IS_MT7663_FW_VER_E2(pAd)) {
		ctrl->patch_profile[WM_CPU].source.header_ptr = mt7663_rom_patch_e2;
		ctrl->patch_profile[WM_CPU].source.header_len = sizeof(mt7663_rom_patch_e2);
		ctrl->patch_profile[WM_CPU].source.bin_name = MT7663_ROM_PATCH_BIN_FILE_NAME_E2;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E2 ROM patch\n", __func__));
	} else {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():E2 ROM patch is not useful\n", __func__));
	}
#endif
#endif /* NEED_ROM_PATCH */

#ifndef REDUCE_KO_SIZE
	if (IS_MT7663_FW_VER_E1(pAd)) {
		ctrl->fw_profile[WM_CPU].source.header_ptr = MT7663_FirmwareImage;
		ctrl->fw_profile[WM_CPU].source.header_len = sizeof(MT7663_FirmwareImage);
		ctrl->fw_profile[WM_CPU].source.bin_name = MT7663_BIN_FILE_NAME;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E1 firmware\n", __func__));
	} else if (IS_MT7663_FW_VER_E2(pAd)) {
		ctrl->fw_profile[WM_CPU].source.header_ptr = MT7663_FirmwareImage_E2;
		ctrl->fw_profile[WM_CPU].source.header_len = sizeof(MT7663_FirmwareImage_E2);
		ctrl->fw_profile[WM_CPU].source.bin_name = MT7663_BIN_FILE_NAME_E2;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E2 firmware\n", __func__));
	} else {
		ctrl->fw_profile[WM_CPU].source.header_ptr = MT7663_FirmwareImage;
		ctrl->fw_profile[WM_CPU].source.header_len = sizeof(MT7663_FirmwareImage);
		ctrl->fw_profile[WM_CPU].source.bin_name = MT7663_BIN_FILE_NAME;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E1 firmware as default\n", __func__));
	}
#else
	if (IS_MT7663_FW_VER_E2(pAd)) {
		ctrl->fw_profile[WM_CPU].source.header_ptr = MT7663_FirmwareImage_E2;
		ctrl->fw_profile[WM_CPU].source.header_len = sizeof(MT7663_FirmwareImage_E2);
		ctrl->fw_profile[WM_CPU].source.bin_name = MT7663_BIN_FILE_NAME_E2;
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():using E2 firmware\n", __func__));
	} else {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():E2 firmware is not useful\n", __func__));
	}
#endif
}

static VOID mt7663_fwdl_datapath_setup(RTMP_ADAPTER *pAd, BOOLEAN init)
{
	WPDMA_GLO_CFG_STRUC GloCfg;
	struct _RTMP_CHIP_OP *ops = hc_get_chip_ops(pAd->hdev_ctrl);

	if (init == TRUE) {
		HIF_IO_READ32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, &GloCfg.word);
		GloCfg.MT7663_field.fw_ring_bp_tx_sch = 1;
		HIF_IO_WRITE32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, GloCfg.word);

		ops->pci_kick_out_cmd_msg = AndesMTPciKickOutCmdMsgFwDlRing;
	} else {
		ops->pci_kick_out_cmd_msg = AndesMTPciKickOutCmdMsg;

		HIF_IO_READ32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, &GloCfg.word);
		GloCfg.MT7663_field.fw_ring_bp_tx_sch = 0;
		HIF_IO_WRITE32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, GloCfg.word);
	}
}

#ifdef CONFIG_STA_SUPPORT
static VOID mt7663_init_dev_nick_name(RTMP_ADAPTER *ad)
{
	snprintf((RTMP_STRING *) ad->nickname, sizeof(ad->nickname), "mt7663_sta");
}
#endif /* CONFIG_STA_SUPPORT */


#ifdef TXBF_SUPPORT
void mt7663_setETxBFCap(
	IN  RTMP_ADAPTER      *pAd,
	IN  TXBF_STATUS_INFO * pTxBfInfo)
{
	HT_BF_CAP *pTxBFCap = pTxBfInfo->pHtTxBFCap;

	if (pTxBfInfo->cmmCfgETxBfEnCond > 0) {
		switch (pTxBfInfo->cmmCfgETxBfEnCond) {
		case SUBF_ALL:
		default:
			pTxBFCap->RxNDPCapable         = TRUE;
			pTxBFCap->TxNDPCapable         = (pTxBfInfo->ucRxPathNum > 1) ? TRUE : FALSE;
			pTxBFCap->ExpNoComSteerCapable = FALSE;
			pTxBFCap->ExpComSteerCapable   = TRUE;/* !pTxBfInfo->cmmCfgETxBfNoncompress; */
			pTxBFCap->ExpNoComBF           = 0; /* HT_ExBF_FB_CAP_IMMEDIATE; */
			pTxBFCap->ExpComBF             =
				HT_ExBF_FB_CAP_IMMEDIATE;/* pTxBfInfo->cmmCfgETxBfNoncompress? HT_ExBF_FB_CAP_NONE: HT_ExBF_FB_CAP_IMMEDIATE; */
			pTxBFCap->MinGrouping          = 3;
			pTxBFCap->NoComSteerBFAntSup   = 0;
			pTxBFCap->ComSteerBFAntSup     = 3;
			pTxBFCap->TxSoundCapable       = FALSE;  /* Support staggered sounding frames */
			pTxBFCap->ChanEstimation       = pTxBfInfo->ucRxPathNum - 1;
			break;

		case SUBF_BFER:
			pTxBFCap->RxNDPCapable         = FALSE;
			pTxBFCap->TxNDPCapable         = (pTxBfInfo->ucRxPathNum > 1) ? TRUE : FALSE;
			pTxBFCap->ExpNoComSteerCapable = FALSE;
			pTxBFCap->ExpComSteerCapable   = TRUE;/* !pTxBfInfo->cmmCfgETxBfNoncompress; */
			pTxBFCap->ExpNoComBF           = 0; /* HT_ExBF_FB_CAP_IMMEDIATE; */
			pTxBFCap->ExpComBF             =
				HT_ExBF_FB_CAP_IMMEDIATE;/* pTxBfInfo->cmmCfgETxBfNoncompress? HT_ExBF_FB_CAP_NONE: HT_ExBF_FB_CAP_IMMEDIATE; */
			pTxBFCap->MinGrouping          = 3;
			pTxBFCap->NoComSteerBFAntSup   = 0;
			pTxBFCap->ComSteerBFAntSup     = 3;
			pTxBFCap->TxSoundCapable       = FALSE;  /* Support staggered sounding frames */
			pTxBFCap->ChanEstimation       = pTxBfInfo->ucRxPathNum - 1;
			break;

		case SUBF_BFEE:
			pTxBFCap->RxNDPCapable         = TRUE;
			pTxBFCap->TxNDPCapable         = FALSE;
			pTxBFCap->ExpNoComSteerCapable = FALSE;
			pTxBFCap->ExpComSteerCapable   = TRUE;/* !pTxBfInfo->cmmCfgETxBfNoncompress; */
			pTxBFCap->ExpNoComBF           = 0; /* HT_ExBF_FB_CAP_IMMEDIATE; */
			pTxBFCap->ExpComBF             =
				HT_ExBF_FB_CAP_IMMEDIATE;/* pTxBfInfo->cmmCfgETxBfNoncompress? HT_ExBF_FB_CAP_NONE: HT_ExBF_FB_CAP_IMMEDIATE; */
			pTxBFCap->MinGrouping          = 3;
			pTxBFCap->NoComSteerBFAntSup   = 0;
			pTxBFCap->ComSteerBFAntSup     = 3;
			pTxBFCap->TxSoundCapable       = FALSE;  /* Support staggered sounding frames */
			pTxBFCap->ChanEstimation       = pTxBfInfo->ucRxPathNum - 1;
			break;
		}
	} else
		memset(pTxBFCap, 0, sizeof(*pTxBFCap));
}

#ifdef VHT_TXBF_SUPPORT
void mt7663_setVHTETxBFCap(
	IN  RTMP_ADAPTER *pAd,
	IN  TXBF_STATUS_INFO * pTxBfInfo)
{
	VHT_CAP_INFO *pTxBFCap = pTxBfInfo->pVhtTxBFCap;

	/*
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		("%s: cmmCfgETxBfEnCond = %d\n", __FUNCTION__, (UCHAR)pTxBfInfo->cmmCfgETxBfEnCond));
	*/

	if (pTxBfInfo->cmmCfgETxBfEnCond > 0) {
		switch (pTxBfInfo->cmmCfgETxBfEnCond) {
		case SUBF_ALL:
		default:
			pTxBFCap->bfee_cap_su       = 1;
			pTxBFCap->bfer_cap_su       = (pTxBfInfo->ucTxPathNum > 1) ? 1 : 0;
#ifdef CFG_SUPPORT_MU_MIMO

			switch (pAd->CommonCfg.MUTxRxEnable) {
			case MUBF_OFF:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_BFER:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = (pTxBfInfo->ucTxPathNum > 1) ? 1 : 0;
				break;

			case MUBF_BFEE:
				pTxBFCap->bfee_cap_mu = 1;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_ALL:
				pTxBFCap->bfee_cap_mu = 1;
				pTxBFCap->bfer_cap_mu = (pTxBfInfo->ucTxPathNum > 1) ? 1 : 0;
				break;

			default:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: set wrong parameters\n", __func__));
				break;
			}

#else
			pTxBFCap->bfee_cap_mu = 0;
			pTxBFCap->bfer_cap_mu = 0;
#endif /* CFG_SUPPORT_MU_MIMO */
			if (pTxBFCap->bfee_sts_cap)
				pTxBFCap->bfee_sts_cap	= (pTxBFCap->bfee_sts_cap < 3) ? pTxBFCap->bfee_sts_cap : 3;
			else
				pTxBFCap->bfee_sts_cap  = 3;
			pTxBFCap->num_snd_dimension = pTxBfInfo->ucTxPathNum - 1;
			break;

		case SUBF_BFER:
			pTxBFCap->bfee_cap_su       = 0;
			pTxBFCap->bfer_cap_su       = (pTxBfInfo->ucTxPathNum > 1) ? 1 : 0;
#ifdef CFG_SUPPORT_MU_MIMO

			switch (pAd->CommonCfg.MUTxRxEnable) {
			case MUBF_OFF:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_BFER:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = (pTxBfInfo->ucTxPathNum > 1) ? 1 : 0;
				break;

			case MUBF_BFEE:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_ALL:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = (pTxBfInfo->ucTxPathNum > 1) ? 1 : 0;
				break;

			default:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: set wrong parameters\n", __func__));
				break;
			}

#else
			pTxBFCap->bfee_cap_mu = 0;
			pTxBFCap->bfer_cap_mu = 0;
#endif /* CFG_SUPPORT_MU_MIMO */
			pTxBFCap->bfee_sts_cap      = 0;
			pTxBFCap->num_snd_dimension = pTxBfInfo->ucTxPathNum - 1;
			break;

		case SUBF_BFEE:
			pTxBFCap->bfee_cap_su       = 1;
			pTxBFCap->bfer_cap_su       = 0;
#ifdef CFG_SUPPORT_MU_MIMO

			switch (pAd->CommonCfg.MUTxRxEnable) {
			case MUBF_OFF:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_BFER:
				pTxBFCap->bfee_cap_mu = 0;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_BFEE:
				pTxBFCap->bfee_cap_mu = 1;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			case MUBF_ALL:
				pTxBFCap->bfee_cap_mu = 1;
				pTxBFCap->bfer_cap_mu = 0;
				break;

			default:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: set wrong parameters\n", __func__));
				break;
			}

#else
			pTxBFCap->bfee_cap_mu = 0;
			pTxBFCap->bfer_cap_mu = 0;
#endif /* CFG_SUPPORT_MU_MIMO */
			if (pTxBFCap->bfee_sts_cap)
				pTxBFCap->bfee_sts_cap	= (pTxBFCap->bfee_sts_cap < 3) ? pTxBFCap->bfee_sts_cap : 3;
			else
				pTxBFCap->bfee_sts_cap	= 3;
			pTxBFCap->num_snd_dimension = pTxBfInfo->ucTxPathNum - 1;
			break;
		}
	} else {
		pTxBFCap->num_snd_dimension = 0;
		pTxBFCap->bfee_cap_mu       = 0;
		pTxBFCap->bfee_cap_su       = 0;
		pTxBFCap->bfer_cap_mu       = 0;
		pTxBFCap->bfer_cap_su       = 0;
		pTxBFCap->bfee_sts_cap      = 0;
	}
}

#endif /* VHT_TXBF_SUPPORT */
#endif /* TXBF_SUPPORT */

UCHAR *mt7663_get_default_bin_image_file(RTMP_ADAPTER *pAd)
{
#ifdef MULTI_INF_SUPPORT
	if (multi_inf_get_idx(pAd) == 0) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("Use %dst %s default bin.\n", multi_inf_get_idx(pAd), DEFAULT_BIN_FILE));
		return DEFAULT_BIN_FILE;
	}
#if defined(MT_SECOND_CARD)
	else if (multi_inf_get_idx(pAd) == 1) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("Use %dst %s default bin.\n", multi_inf_get_idx(pAd), SECOND_BIN_FILE));
		return SECOND_BIN_FILE;
	}
#endif /* MT_SECOND_CARD */
#if defined(MT_THIRD_CARD)
	else if (multi_inf_get_idx(pAd) == 2) {
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("Use %dst %s default bin.\n", multi_inf_get_idx(pAd), THIRD_BIN_FILE));
		return THIRD_BIN_FILE;
	}
#endif /* MT_THIRD_CARD */
	else
#endif /* MULTI_INF_SUPPORT */
	{
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("Use the default %s bin image!\n", DEFAULT_BIN_FILE));

		return DEFAULT_BIN_FILE;
	}

	return NULL;
}

static INT hif_init_WPDMA(RTMP_ADAPTER *pAd)
{
	WPDMA_GLO_CFG_STRUC GloCfg;

	HIF_IO_READ32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, &GloCfg.word);

	GloCfg.word = 0x10001870;

	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, GloCfg.word);

	return TRUE;
}

static INT hif_set_WPDMA(RTMP_ADAPTER *pAd, INT32 TxRx, BOOLEAN enable)
{
	WPDMA_GLO_CFG_STRUC GloCfg;
	struct _RTMP_CHIP_CAP *chip_cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT8 WPDMABurstSIZE;
#ifdef CONFIG_DELAY_INT
	UINT32 Value;
#endif

	WPDMABurstSIZE = chip_cap->WPDMABurstSIZE;
	HIF_IO_READ32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, &GloCfg.word);
#ifdef CONFIG_DELAY_INT
	HIF_IO_READ32(pAd->hdev_ctrl, MT_DELAY_INT_CFG, &Value);
#endif

	switch (TxRx) {
	case PDMA_TX:
		if (enable == TRUE) {
			GloCfg.MT7663_field.EnableTxDMA = 1;
			GloCfg.MT7663_field.EnTXWriteBackDDONE = 1;
			GloCfg.MT7663_field.WPDMABurstSIZE = WPDMABurstSIZE;
			GloCfg.MT7663_field.multi_dma_en = MULTI_DMA_EN_FEATURE_2_PREFETCH;
#ifdef CONFIG_DELAY_INT
			Value |= TX_DLY_INT_EN;
			Value &= ~TX_MAX_PINT_MASK;
			Value |= TX_MAX_PINT(TX_PENDING_INT_NUMS);
			Value &= ~TX_MAX_PTIME_MASK;
			Value |= TX_MAX_PTIME(TX_PENDING_INT_TIME);
#endif
		} else {
			GloCfg.MT7663_field.EnableTxDMA = 0;
#ifdef CONFIG_DELAY_INT
			Value &= ~TX_DLY_INT_EN;
			Value &= ~TX_MAX_PINT_MASK;
			Value &= ~TX_MAX_PTIME_MASK;
#endif
		}

		break;

	case PDMA_RX:
		if (enable == TRUE) {
			GloCfg.MT7663_field.EnableRxDMA = 1;
			GloCfg.MT7663_field.WPDMABurstSIZE = WPDMABurstSIZE;
			GloCfg.MT7663_field.multi_dma_en = MULTI_DMA_EN_FEATURE_2_PREFETCH;
#ifdef CONFIG_DELAY_INT
			Value |= RX_DLY_INT_EN;
			Value &= ~RX_MAX_PINT_MASK;
			Value |= RX_MAX_PINT(RX_PENDING_INT_NUMS);
			Value &= ~RX_MAX_PTIME_MASK;
			Value |= RX_MAX_PTIME(RX_PENDING_INT_TIME);
#endif
		} else {
			GloCfg.MT7663_field.EnableRxDMA = 0;
#ifdef CONFIG_DELAY_INT
			Value &= ~RX_DLY_INT_EN;
			Value &= ~RX_MAX_PINT_MASK;
			Value &= ~RX_MAX_PTIME_MASK;
#endif
		}

		break;

	case PDMA_TX_RX:
		if (enable == TRUE) {
			GloCfg.MT7663_field.EnableTxDMA = 1;
			GloCfg.MT7663_field.EnableRxDMA = 1;
			GloCfg.MT7663_field.EnTXWriteBackDDONE = 1;
			GloCfg.MT7663_field.WPDMABurstSIZE = WPDMABurstSIZE;
			GloCfg.MT7663_field.multi_dma_en = MULTI_DMA_EN_FEATURE_2_PREFETCH;

#ifdef CONFIG_DELAY_INT
			Value |= TX_DLY_INT_EN;
			Value &= ~TX_MAX_PINT_MASK;
			Value |= TX_MAX_PINT(TX_PENDING_INT_NUMS);
			Value &= ~TX_MAX_PTIME_MASK;
			Value |= TX_MAX_PTIME(TX_PENDING_INT_TIME);
			Value |= RX_DLY_INT_EN;
			Value &= ~RX_MAX_PINT_MASK;
			Value |= RX_MAX_PINT(RX_PENDING_INT_NUMS);
			Value &= ~RX_MAX_PTIME_MASK;
			Value |= RX_MAX_PTIME(RX_PENDING_INT_TIME);
#endif
		} else {
			GloCfg.MT7663_field.EnableRxDMA = 0;
			GloCfg.MT7663_field.EnableTxDMA = 0;
#ifdef CONFIG_DELAY_INT
			Value &= ~TX_DLY_INT_EN;
			Value &= ~TX_MAX_PINT_MASK;
			Value &= ~TX_MAX_PTIME_MASK;
			Value &= ~RX_DLY_INT_EN;
			Value &= ~RX_MAX_PINT_MASK;
			Value &= ~RX_MAX_PTIME_MASK;
#endif
		}

		break;

	default:
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Unknown path (%d\n", __func__, TxRx));
		break;
	}

	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_WPDMA_GLO_CFG, GloCfg.word);
#ifdef CONFIG_DELAY_INT
	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_DELAY_INT_CFG, Value);
#endif

#define WPDMA_DISABLE -1

	if (!enable)
		TxRx = WPDMA_DISABLE;

	WLAN_HOOK_CALL(WLAN_HOOK_DMA_SET, pAd, &TxRx);
	return TRUE;
}

static BOOLEAN hif_wait_WPDMA_idle(struct _RTMP_ADAPTER *pAd, INT round, INT wait_us)
{
	INT i = 0;

	UINT32 value = 0;

	do {
		HIF_IO_READ32(pAd->hdev_ctrl, MT_CONN_HIF_BUSY_STATUS, &value);

		if ((value & CONN_HIF_BUSY) == 0) {
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("==>	DMAIdle, CONN_HIF_BUSY_STATUS=0x%x\n", value));
			return TRUE;
		}

		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
			return FALSE;

		RtmpusecDelay(wait_us);
	} while ((i++) < round);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("==>  DMABusy\n"));
	return FALSE;
}

static BOOLEAN hif_reset_WPDMA(RTMP_ADAPTER *pAd)
{
	UINT32 value = 0;

	/* pdma0 hw reset (w/ dma scheduler)
		activate: toggle (active low)
		scope: PDMA + DMASCH + Tx/Rx FIFO
		PDMA:
			logic reset: Y
			register reset: Y
		DMASCH:
			logic reset: Y
			register reset: Y
	*/
	HIF_IO_READ32(pAd->hdev_ctrl, MT_CONN_HIF_RST, &value);
	value &= ~(CONN_HIF_LOGIC_RST_N | DMASHDL_ALL_RST_N);
	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_CONN_HIF_RST, value);
	HIF_IO_READ32(pAd->hdev_ctrl, MT_CONN_HIF_RST, &value);
	value |= (CONN_HIF_LOGIC_RST_N | DMASHDL_ALL_RST_N);
	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_CONN_HIF_RST, value);

	return TRUE;
}
static INT32 get_fw_sync_value(RTMP_ADAPTER *pAd)
{
	UINT32 value;

	RTMP_IO_READ32(pAd->hdev_ctrl, CONN_ON_MISC, &value);
	value = (value & 0x0000000E) >> 1;

	MTWF_LOG(DBG_CAT_FW, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s: current sync CR = 0x%x\n", __func__, value));
	return value;
}

#ifdef CONFIG_FWOWN_SUPPORT

static INT pcie_doorbell_push(struct _RTMP_ADAPTER *ad, UINT32 mode)
{
#define PCIE_DOORBELL_PUSH 0x484  /* 0x7c031484 */

	struct pci_dev *dev = ((POS_COOKIE)ad->OS_Cookie)->pci_dev;

	pci_write_config_byte(dev, PCIE_DOORBELL_PUSH, mode);
	return 0;
}

static VOID fw_own(RTMP_ADAPTER *pAd)
{
	/* Write 1 to MT_CONN_HIF_ON_LPCTL bit 0 to set FW own */
	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): Write 1 to MT_CONN_HIF_ON_LPCTL bit 0 to set FW own\n", __func__));
	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_CONN_HIF_ON_LPCTL, MT_HOST_SET_OWN);
	pAd->bDrvOwn = FALSE;
}

static INT32 driver_own(RTMP_ADAPTER *pAd)
{
	INT32 Ret = NDIS_STATUS_SUCCESS;
	UINT32 retrycnt = 0;
	UINT32 counter = 0;
	UINT32 Value, checkval = 0;
#define MAX_RETRY_CNT 4

	do {
		retrycnt++;
		if (pAd->bDrvOwn == TRUE) {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s()::Return since already in Driver Own...\n", __func__));
			return Ret;
		}

		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s()::Try to Clear FW Own...\n", __func__));

		/* trigger eint to N9 to clear */
		pcie_doorbell_push(pAd, MT_HOST_CLR_OWN);

		counter = 0;
		while (counter < FW_OWN_POLLING_COUNTER) {
			RTMP_IO_READ32(pAd->hdev_ctrl, CONN_ON_HOST_CSR_MISC, &checkval);

			if (!(checkval & MT_HOST_SET_OWN))
				break;

			counter++;
		};

		if (counter == FW_OWN_POLLING_COUNTER)
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s()::Driver Own Status Get Fail!!! Counter(%d)\n", __func__, counter));

		/* Write CR to get driver own */
		HIF_IO_WRITE32(pAd->hdev_ctrl, MT_CONN_HIF_ON_LPCTL, MT_HOST_CLR_OWN);
		/* Poll driver own status */
		counter = 0;
		while (counter < FW_OWN_POLLING_COUNTER) {
			RtmpusecDelay(1000);

			if (pAd->bDrvOwn == TRUE)
				break;
			counter++;
		};

		if (counter == FW_OWN_POLLING_COUNTER) {
			HIF_IO_READ32(pAd->hdev_ctrl, MT_CONN_HIF_ON_LPCTL, &Value);

			if (!(Value & MT_HOST_SET_OWN))
				pAd->bDrvOwn = TRUE;
		}

		if (pAd->bDrvOwn)
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s()::Success to clear FW Own\n", __func__));
		else {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s()::Fail to clear FW Own (%d)\n", __func__, counter));

			if (retrycnt >= MAX_RETRY_CNT)
				Ret = NDIS_STATUS_FAILURE;
		}
	} while (pAd->bDrvOwn == FALSE && retrycnt < MAX_RETRY_CNT);

	return Ret;
}
#endif

static VOID mtd_isr(RTMP_ADAPTER *pAd)
{
	UINT32 IntSource = 0x00000000L;
	POS_COOKIE pObj;
	PCI_HIF_T *pci_hif = hc_get_hif_ctrl(pAd->hdev_ctrl);
	struct tm_ops *tm_ops = pAd->tm_hif_ops;
#ifdef CONFIG_TP_DBG
	struct tp_debug *tp_dbg = &pAd->tr_ctl.tp_dbg;
#endif
	unsigned long flags = 0;

	pObj = (POS_COOKIE)pAd->OS_Cookie;
	pci_hif->bPCIclkOff = FALSE;
	HIF_IO_READ32(pAd->hdev_ctrl, MT_INT_SOURCE_CSR, &IntSource);

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP)) {
		HIF_IO_WRITE32(pAd->hdev_ctrl, MT_INT_SOURCE_CSR, IntSource);
		return;
	}

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) {
		UINT32 reg;

		/* Fix Rx Ring FULL lead DMA Busy, when DUT is in reset stage */
		reg = IntSource & (MT_INT_CMD | MT_INT_RX | MT_INT_RX_DLY |
					WF_MAC_INT_3 | MT_INT_RX_COHE);

		if (!reg) {
			HIF_IO_WRITE32(pAd->hdev_ctrl, MT_INT_SOURCE_CSR, IntSource);
			return;
		}
	}

	/* Do nothing if NIC doesn't exist */
	if (IntSource == 0xffffffff) {
		RTMP_SET_FLAG(pAd, (fRTMP_ADAPTER_NIC_NOT_EXIST | fRTMP_ADAPTER_HALT_IN_PROGRESS));
		HIF_IO_WRITE32(pAd->hdev_ctrl, MT_INT_SOURCE_CSR, IntSource);
		return;
	}

	if (IntSource & MT_TxCoherent)
		MTWF_LOG(DBG_CAT_HIF, CATHIF_PCI, DBG_LVL_ERROR, (">>>TxCoherent<<<\n"));

	if (IntSource & MT_RxCoherent)
		MTWF_LOG(DBG_CAT_HIF, CATHIF_PCI, DBG_LVL_ERROR, (">>>RxCoherent<<<\n"));

	RTMP_INT_LOCK(&pAd->LockInterrupt, flags);

	if (IntSource & MT_INT_TX_DONE) {
		if ((pci_hif->intDisableMask & (IntSource & MT_INT_TX_DONE)) == 0)
			tm_ops->schedule_task(pAd, TX_DONE_TASK);

		pci_hif->IntPending |= (IntSource & MT_INT_TX_DONE);

#ifdef CONFIG_TP_DBG
		tp_dbg->IsrTxCnt++;
#endif
	}

	if (IntSource & MT_INT_RX_DATA) {
		if (!(IntSource & MT_INT_RX_DLY))
			IntSource &= ~MT_INT_RX_DATA;
		else
			 pci_hif->IntPending |= MT_INT_RX_DATA;
#ifdef CONFIG_TP_DBG
		tp_dbg->IsrRxCnt++;
#endif
	}

	if (IntSource & MT_INT_RX_CMD) {
		if (!(IntSource & MT_INT_RX_DLY))
			IntSource &= ~MT_INT_RX_CMD;
		else
			pci_hif->IntPending |= MT_INT_RX_CMD;
#ifdef CONFIG_TP_DBG
		tp_dbg->IsrRx1Cnt++;
#endif
	}

	if (IntSource & MT_INT_RX_DLY) {
		if ((pci_hif->intDisableMask & MT_INT_RX_DLY) == 0)
			tm_ops->schedule_task(pAd, TR_DONE_TASK);

		pci_hif->IntPending |= MT_INT_RX_DLY;

#ifdef CONFIG_TP_DBG
		tp_dbg->IsrRxDlyCnt++;
#endif
	}

	if (IntSource & MT_INT_SUBSYS_INT_STS) {
		if ((pci_hif->intDisableMask & (IntSource & MT_INT_SUBSYS_INT_STS)) == 0)
			tm_ops->schedule_task(pAd, SUBSYS_INT_TASK);
	}

	if (IntSource & MT_INT_MCU2HOST_SW_INT_STS) {
		UINT32 value;

		RTMP_IO_READ32(pAd->hdev_ctrl, MT_MCU2HOST_SW_INT_STA, &value);
#ifdef ERR_RECOVERY
		if (value & MT7663_ERROR_DETECT_MASK) {
			/* updated ErrRecovery Status. */
			pAd->ErrRecoveryCtl.status = value;

			/* Clear MCU CMD status*/
			RTMP_IO_WRITE32(pAd->hdev_ctrl, MT_MCU2HOST_SW_INT_STA, MT7663_ERROR_DETECT_MASK);

			/* Trigger error recovery process with fw reload. */
			tm_ops->schedule_task(pAd, ERROR_RECOVERY_TASK);
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
				("%s,::E  R  , status=0x%08X\n", __func__, value));
			RTMPHandleInterruptSerDump(pAd);
		}
#endif /* ERR_RECOVERY */
	}

	HIF_IO_WRITE32(pAd->hdev_ctrl, MT_INT_SOURCE_CSR, IntSource);
	mt_int_disable(pAd, IntSource);
	RTMP_INT_UNLOCK(&pAd->LockInterrupt, flags);
}

static struct dly_ctl_cfg mt7663_rx_dly_ctl_ul_tbl[] = {
	{0, 0x811c},
	{300, 0xa01c},
	{500, 0xc01f},
};

static struct dly_ctl_cfg mt7663_rx_dly_ctl_dl_tbl[] = {
	{0, 0x811c},
};

/*
*	init sub-layer interrupt enable mask
*/
static VOID mt7663_irq_init(RTMP_ADAPTER *pAd)
{
#ifdef ERR_RECOVERY
	HW_IO_WRITE32(pAd->hdev_ctrl, MT_MCU2HOST_SW_INT_ENA, MT7663_ERROR_DETECT_MASK);
#endif
}

#ifdef	CONNAC_EFUSE_FORMAT_SUPPORT
static void mt7663_eeprom_info_extract(RTMP_ADAPTER *pAd, VOID *eeprom)
{
	UCHAR i;
	P_EFUSE_INFO_T prEfuseInfo = (P_EFUSE_INFO_T)eeprom;

	pAd->EfuseInfoAll.pr2G4Cmm = &prEfuseInfo->r2G4Cmm;
	pAd->EfuseInfoAll.pr2G4IBfParam = &prEfuseInfo->rIBfCal.r2G4IBfParam;
	pAd->EfuseInfoAll.pr2G4RxLatency = &prEfuseInfo->rDelayComp.r2G4RxLatency;
	pAd->EfuseInfoAll.pr2G4TxPower = &prEfuseInfo->rTxPower.r2G4TxPower;
	pAd->EfuseInfoAll.pr5GCmm = &prEfuseInfo->r5GCmm;
	pAd->EfuseInfoAll.pr5GIBfParam = &prEfuseInfo->rIBfCal.r5GIBfParam;
	pAd->EfuseInfoAll.pr5GRxLatency = &prEfuseInfo->rDelayComp.r5GRxLatency;
	pAd->EfuseInfoAll.pr5GTxPower = &prEfuseInfo->rTxPower.r5GTxPower;
	pAd->EfuseInfoAll.prDelayCompCtrl = &prEfuseInfo->rDelayComp.rDelayCompCtrl;

#if (CONNAC_WIFI_TX_POWER_EFUSE_TYPE != EFUSE_INFO_TX_POWER_CUSTOMIZED_TYPE_1)
	pAd->EfuseInfoAll.prNTxPowerOffset = &prEfuseInfo->rTxPower.rNTxPowerOffset;
#endif
	pAd->EfuseInfoAll.prSys = &prEfuseInfo->rSys;

	for (i = 0; i < MAX_ANTENNA_NUM; i++) {
		pAd->EfuseInfoAll.pr2G4WFPath[i] = &prEfuseInfo->ar2G4WFPath[i];
		pAd->EfuseInfoAll.pr5GWFPath[i] = &prEfuseInfo->ar5GWFPath[i];
	}
}
#endif /* CONNAC_EFUSE_FORMAT_SUPPORT */

static RTMP_CHIP_OP MT7663_ChipOp = {0};
static RTMP_CHIP_CAP MT7663_ChipCap = {0};


static VOID mt7663_chipCap_init(void)
{
	MT7663_ChipCap.TXWISize = sizeof(TMAC_TXD_L); /* 32 */
	MT7663_ChipCap.RXWISize = 28;
	MT7663_ChipCap.tx_hw_hdr_len = MT7663_ChipCap.TXWISize;/* + sizeof(CR4_TXP_MSDU_INFO); */
	MT7663_ChipCap.rx_hw_hdr_len = MT7663_ChipCap.RXWISize;
	MT7663_ChipCap.num_of_tx_ring = MT7663_TX_RING_NUM;
	MT7663_ChipCap.num_of_rx_ring = MT7663_RX_RING_NUM;
#ifdef MEMORY_OPTIMIZATION
	MT7663_ChipCap.tx_ring_size = 256;
	MT7663_ChipCap.rx0_ring_size = 128;
	MT7663_ChipCap.rx1_ring_size = 128;
#else
	MT7663_ChipCap.tx_ring_size = 1024;
	MT7663_ChipCap.rx0_ring_size = 512;
	MT7663_ChipCap.rx1_ring_size = 512;
#endif
	MT7663_ChipCap.asic_caps = (fASIC_CAP_PMF_ENC | fASIC_CAP_MCS_LUT
				    | fASIC_CAP_CT | fASIC_CAP_HW_DAMSDU);
#ifdef BCN_V2_SUPPORT /* add bcn v2 support , 1.5k beacon support */
	MT7663_ChipCap.max_v2_bcn_num = 4;
#endif
#ifdef DOT11_VHT_AC
	MT7663_ChipCap.mcs_nss.max_vht_mcs = VHT_MCS_CAP_9;
#ifdef G_BAND_256QAM
	MT7663_ChipCap.mcs_nss.g_band_256_qam = TRUE;
#endif
#endif /* DOT11_VHT_AC */

#ifdef RX_CUT_THROUGH
	MT7663_ChipCap.asic_caps |= fASIC_CAP_BA_OFFLOAD;
#endif
#ifdef HDR_TRANS_TX_SUPPORT
	MT7663_ChipCap.asic_caps |= fASIC_CAP_TX_HDR_TRANS;
#endif
#ifdef HDR_TRANS_RX_SUPPORT
	MT7663_ChipCap.asic_caps |= fASIC_CAP_RX_HDR_TRANS;
#endif
#ifdef RX_SCATTER
	MT7663_ChipCap.asic_caps |= fASIC_CAP_RX_DMA_SCATTER;
#endif

	MT7663_ChipCap.asic_caps |= fASIC_CAP_RX_DLY;

	MT7663_ChipCap.phy_caps = (fPHY_CAP_24G | fPHY_CAP_5G | \
							   fPHY_CAP_HT | fPHY_CAP_VHT | \
							   fPHY_CAP_TXBF | fPHY_CAP_LDPC | fPHY_CAP_MUMIMO | \
							   fPHY_CAP_BW40 | fPHY_CAP_BW80);

	MT7663_ChipCap.hw_ops_ver = HWCTRL_OP_TYPE_V2;
	MT7663_ChipCap.hif_type = HIF_MT;
	MT7663_ChipCap.mac_type = MAC_MT;
	MT7663_ChipCap.MCUType = ANDES;
	MT7663_ChipCap.rf_type = RF_MT;
	MT7663_ChipCap.pRFRegTable = NULL;
	MT7663_ChipCap.pBBPRegTable = NULL;
	MT7663_ChipCap.bbpRegTbSize = 0;
	MT7663_ChipCap.MaxNumOfRfId = MAX_RF_ID;
	MT7663_ChipCap.MaxNumOfBbpId = 200;
	MT7663_ChipCap.WtblHwNum = MT7663_MT_WTBL_SIZE;
	MT7663_ChipCap.FlgIsHwWapiSup = TRUE;
	MT7663_ChipCap.FlgIsHwAntennaDiversitySup = FALSE;
#ifdef STREAM_MODE_SUPPORT
	MT7663_ChipCap.FlgHwStreamMode = FALSE;
#endif
#ifdef TXBF_SUPPORT
	MT7663_ChipCap.FlgHwTxBfCap = TXBF_HW_CAP;
#endif
	MT7663_ChipCap.SnrFormula = SNR_FORMULA4;
	MT7663_ChipCap.mcs_nss.max_nss = 2;
	/* todo Ellis */
#ifdef RTMP_EFUSE_SUPPORT
	MT7663_ChipCap.EFUSE_USAGE_MAP_START = 0x3c0;
	MT7663_ChipCap.EFUSE_USAGE_MAP_END = 0x3fb;
	MT7663_ChipCap.EFUSE_USAGE_MAP_SIZE = 60;
	MT7663_ChipCap.EFUSE_RESERVED_SIZE = 59;	/* Cal-Free is 22 free block */
#endif
#ifndef REDUCE_KO_SIZE
	MT7663_ChipCap.EEPROM_DEFAULT_BIN = MT7663_E2PImage;
	MT7663_ChipCap.EEPROM_DEFAULT_BIN_SIZE = sizeof(MT7663_E2PImage);
#else
	MT7663_ChipCap.EEPROM_DEFAULT_BIN = NULL;
	MT7663_ChipCap.EEPROM_DEFAULT_BIN_SIZE = 0;
#endif
	MT7663_ChipCap.EFUSE_BUFFER_CONTENT_SIZE = 0x600;
#ifdef CARRIER_DETECTION_SUPPORT
	MT7663_ChipCap.carrier_func = TONE_RADAR_V2;
#endif
#ifdef RTMP_MAC_PCI
	MT7663_ChipCap.WPDMABurstSIZE = 3;
#endif
	MT7663_ChipCap.ProbeRspTimes = 2;
#ifdef NEW_MBSSID_MODE
#ifdef ENHANCE_NEW_MBSSID_MODE
	MT7663_ChipCap.MBSSIDMode = MBSSID_MODE4;
#else
	MT7663_ChipCap.MBSSIDMode = MBSSID_MODE1;
#endif /* ENHANCE_NEW_MBSSID_MODE */
#else
	MT7663_ChipCap.MBSSIDMode = MBSSID_MODE0;
#endif /* NEW_MBSSID_MODE */
#ifdef DOT11W_PMF_SUPPORT
	/* sync with Ellis, wilsonl */
	MT7663_ChipCap.FlgPMFEncrtptMode = PMF_ENCRYPT_MODE_2;
#endif /* DOT11W_PMF_SUPPORT */
#ifdef CONFIG_ANDES_SUPPORT
#ifdef NEED_ROM_PATCH
	MT7663_ChipCap.need_load_patch = BIT(WM_CPU);
#else
	MT7663_ChipCap.need_load_patch = 0;
#endif
	MT7663_ChipCap.need_load_fw = BIT(WM_CPU);
	MT7663_ChipCap.load_patch_flow = PATCH_FLOW_V1;
	MT7663_ChipCap.load_fw_flow = FW_FLOW_V1; /*FW_FLOW_V2_COMPRESS_IMG;*/
	MT7663_ChipCap.patch_format = PATCH_FORMAT_V1;
	MT7663_ChipCap.fw_format = FW_FORMAT_V3;
	MT7663_ChipCap.load_patch_method = BIT(HEADER_METHOD);
	MT7663_ChipCap.load_fw_method = BIT(HEADER_METHOD);
	MT7663_ChipCap.rom_patch_offset = MT7663_ROM_PATCH_START_ADDRESS;
	MT7663_ChipCap.decompress_tmp_addr = 0x203b000UL;
#endif
#ifdef UNIFY_FW_CMD /* todo wilsonl */
	MT7663_ChipCap.cmd_header_len = sizeof(FW_TXD) + sizeof(TMAC_TXD_L);
#else
	MT7663_ChipCap.cmd_header_len = 12; /* sizeof(FW_TXD) */
#endif
	MT7663_ChipCap.cmd_padding_len = 0;
	MT7663_ChipCap.ppdu.TxAggLimit = 64;
	MT7663_ChipCap.ppdu.RxBAWinSize = 64;
	MT7663_ChipCap.ppdu.ht_max_ampdu_len_exp = 3;
#ifdef DOT11_VHT_AC
	MT7663_ChipCap.ppdu.max_mpdu_len = MPDU_7991_OCTETS;
	MT7663_ChipCap.ppdu.vht_max_ampdu_len_exp = 7;
#endif /* DOT11_VHT_AC */
#ifdef RACTRL_FW_OFFLOAD_SUPPORT
	/* enabled later, wilsonl */
	MT7663_ChipCap.fgRateAdaptFWOffload = TRUE;
#endif /* RACTRL_FW_OFFLOAD_SUPPORT */
	MT7663_ChipCap.qos.WmmHwNum = MT7663_MT_WMM_SIZE; /* for multi-wmm */
	MT7663_ChipCap.PDA_PORT = MT7663_PDA_PORT;
	MT7663_ChipCap.ppdu.SupportAMSDU = TRUE;
	/* sync with Pat, wilsonl */
	MT7663_ChipCap.APPSMode = APPS_MODE2;
	MT7663_ChipCap.CtParseLen = MT7663_CT_PARSE_LEN;
	MT7663_ChipCap.qm = GENERIC_QM;
	MT7663_ChipCap.qm_tm = TASKLET_METHOD;
	MT7663_ChipCap.hif_tm = TASKLET_METHOD;
	MT7663_ChipCap.qos.wmm_detect_method = WMM_DETECT_METHOD1;
	/* for interrupt enable mask */
	MT7663_ChipCap.int_enable_mask = MT_CoherentInt | MT_MacInt | MT_INT_RX_DLY |
					MT_INT_T3_DONE | MT_INT_T15_DONE | MT_FW_CLR_OWN_INT;

	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX0] = TX_DATA;
	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX1] = TX_DATA;
	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX2] = TX_DATA;
	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX3] = TX_FW_DL;
	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX4] = TX_DATA;
	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX5] = TX_ALTX;
	MT7663_ChipCap.hif_pkt_type[HIF_TX_IDX15] = TX_CMD;
	MT7663_ChipCap.hif_pci_ring_layout.tx_ring_layout = mt7663_tx_ring_layout;
	MT7663_ChipCap.hif_pci_ring_layout.rx_ring_layout = mt7663_rx_ring_layout;

#if defined(ERR_RECOVERY) || defined(CONFIG_FWOWN_SUPPORT)
	MT7663_ChipCap.int_enable_mask |= MT_INT_MCU2HOST_SW_INT_STS;
#endif /* ERR_RECOVERY || CONFIG_FWOWN_SUPPORT */
	MT7663_ChipCap.band_cnt = 1;
#ifdef HW_TX_AMSDU_SUPPORT
	MT7663_ChipCap.asic_caps |= fASIC_CAP_HW_TX_AMSDU;
#endif /* HW_TX_AMSDU_SUPPORT */

#ifdef HW_TX_AMSDU_SUPPORT
	/* doesn't support SW AMSDU on GE QM v2*/
	if (MT7663_ChipCap.asic_caps & fASIC_CAP_HW_TX_AMSDU)
		MT7663_ChipCap.qm_version = QM_V2;
	else
#endif /* HW_TX_AMSDU_SUPPORT */
		MT7663_ChipCap.qm_version = QM_V1;
#if (defined(VOW_SUPPORT) && defined(VOW_DVT)) || defined(RANDOM_PKT_GEN)
	/* For HW function DVT, GE QM v2 need to develop furthermore, using QM v1 directly*/
	MT7663_ChipCap.qm_version = QM_V1;
#endif

#ifdef INTERNAL_CAPTURE_SUPPORT
	MT7663_ChipCap.ICapADCIQCnt = MT7663_ICAP_TWO_WAY_ADC_IQ_DATA_CNT;
	MT7663_ChipCap.ICapIQCIQCnt = MT7663_ICAP_TWO_WAY_IQC_IQ_DATA_CNT;
	MT7663_ChipCap.ICapPackedADC = MT7663_CAP_TWO_WAY_ADC;
	MT7663_ChipCap.ICapMaxIQCnt = MT7663_ICAP_TWO_WAY_ADC_IQ_DATA_CNT;
#endif /* INTERNAL_CAPTURE_SUPPORT */
#ifdef WIFI_SPECTRUM_SUPPORT
	MT7663_ChipCap.SpectrumWF0ADC = MT7663_CAP_WF0_ADC;
	MT7663_ChipCap.SpectrumWF1ADC = MT7663_CAP_WF1_ADC;
	MT7663_ChipCap.SpectrumWF0FIIQ = MT7663_CAP_WF0_FIIQ;
	MT7663_ChipCap.SpectrumWF1FIIQ = MT7663_CAP_WF1_FIIQ;
	MT7663_ChipCap.SpectrumWF0FDIQ = MT7663_CAP_WF0_FDIQ;
	MT7663_ChipCap.SpectrumWF1FDIQ = MT7663_CAP_WF1_FDIQ;
#endif /* WIFI_SPECTRUM_SUPPORT */
	MT7663_ChipCap.txd_type = TXD_V2;
	MT7663_ChipCap.tx_delay_support = FALSE;
	MT7663_ChipCap.tx_delay_mode = TX_DELAY_HW_MODE;
#ifdef OCE_SUPPORT
	MT7663_ChipCap.FdFrameFwOffloadEnabled = TRUE;
#endif /* OCE_SUPPORT */
}


static VOID mt7663_chipOp_init(void)
{
	MT7663_ChipOp.AsicRfInit = mt7663_init_rf_cr;
	MT7663_ChipOp.AsicBbpInit = MT7663BBPInit;
	MT7663_ChipOp.AsicMacInit = mt7663_init_mac_cr;
	MT7663_ChipOp.AsicReverseRfFromSleepMode = NULL;
	MT7663_ChipOp.AsicHaltAction = NULL;
	/* BBP adjust */
	MT7663_ChipOp.ChipBBPAdjust = mt7663_bbp_adjust;
	/* AGC */
	MT7663_ChipOp.ChipSwitchChannel = mt7663_switch_channel;
#ifdef NEW_SET_RX_STREAM
	MT7663_ChipOp.ChipSetRxStream = mt7663_set_RxStream;
#endif
	MT7663_ChipOp.AsicAntennaDefaultReset = mt7663_antenna_default_reset;
#ifdef CONFIG_STA_SUPPORT
	MT7663_ChipOp.NetDevNickNameInit = mt7663_init_dev_nick_name;
#endif
	MT7663_ChipOp.get_bin_image_file = mt7663_get_default_bin_image_file;
#ifdef CAL_FREE_IC_SUPPORT
	/* do not need, turn off compile flag, wilsonl */
	MT7663_ChipOp.is_cal_free_ic = mt7663_is_cal_free_ic;
	MT7663_ChipOp.cal_free_data_get = mt7663_cal_free_data_get;
#endif /* CAL_FREE_IC_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	MT7663_ChipOp.ToneRadarProgram = ToneRadarProgram_v2;
#endif
	MT7663_ChipOp.DisableTxRx = NULL; /* 302 */
#ifdef RTMP_PCI_SUPPORT
	/* sync with Pat, Hammin, wilsonl */
	/* MT7663_ChipOp.AsicRadioOn = RT28xxPciAsicRadioOn; */
	/* MT7663_ChipOp.AsicRadioOff = RT28xxPciAsicRadioOff; */
#endif
#ifdef MT_WOW_SUPPORT
	/* do not need, turn off compile flag, wilsonl */
	MT7663_ChipOp.AsicWOWEnable = MT76xxAndesWOWEnable;
	MT7663_ChipOp.AsicWOWDisable = MT76xxAndesWOWDisable;
	/* MT7663_ChipOp.AsicWOWInit = MT76xxAndesWOWInit, */
#endif /* MT_WOW_SUPPORT */
	MT7663_ChipOp.show_pwr_info = NULL;
	MT7663_ChipOp.bufferModeEfuseFill = mt7663_bufferModeEfuseFill;
	MT7663_ChipOp.MtCmdTx = MtCmdSendMsg;
	MT7663_ChipOp.prepare_fwdl_img = mt7663_fw_prepare;
	MT7663_ChipOp.fwdl_datapath_setup = mt7663_fwdl_datapath_setup;
#ifdef TXBF_SUPPORT
	MT7663_ChipOp.TxBFInit                 = mt_WrapTxBFInit;
	MT7663_ChipOp.TxBFInit                 = mt_WrapTxBFInit;
	MT7663_ChipOp.ClientSupportsETxBF      = mt_WrapClientSupportsETxBF;
	MT7663_ChipOp.iBFPhaseComp             = mt_iBFPhaseComp;
	MT7663_ChipOp.iBFPhaseCalInit          = mt_iBFPhaseCalInit;
	MT7663_ChipOp.iBFPhaseFreeMem          = mt_iBFPhaseFreeMem;
	MT7663_ChipOp.iBFPhaseCalE2PUpdate     = mt_iBFPhaseCalE2PUpdate;
	MT7663_ChipOp.iBFPhaseCalReport        = mt_iBFPhaseCalReport;
	MT7663_ChipOp.setETxBFCap              = mt7663_setETxBFCap;
	MT7663_ChipOp.iBfCaleBfPfmuMemAlloc    = mt7663_eBFPfmuMemAlloc;
	MT7663_ChipOp.iBfCaliBfPfmuMemAlloc    = mt7663_iBFPfmuMemAlloc;
	MT7663_ChipOp.BfStaRecUpdate           = mt_AsicBfStaRecUpdate;
	MT7663_ChipOp.BfStaRecRelease          = mt_AsicBfStaRecRelease;
	MT7663_ChipOp.BfPfmuMemAlloc           = CmdPfmuMemAlloc;
	MT7663_ChipOp.BfPfmuMemRelease         = CmdPfmuMemRelease;
	MT7663_ChipOp.TxBfTxApplyCtrl          = CmdTxBfTxApplyCtrl;
	MT7663_ChipOp.BfApClientCluster        = CmdTxBfApClientCluster;
	MT7663_ChipOp.BfHwEnStatusUpdate       = CmdTxBfHwEnableStatusUpdate;
	MT7663_ChipOp.BfModuleEnCtrl           = NULL;
	MT7663_ChipOp.BfeeHwCtrl               = CmdTxBfeeHwCtrl;
	MT7663_ChipOp.bfee_adaption            = txbf_bfee_adaption;
#ifdef CONFIG_STA_SUPPORT
	MT7663_ChipOp.archSetAid               = MtAsicSetAid;
#endif /* CONFIG_STA_SUPPORT */
#ifdef VHT_TXBF_SUPPORT
	MT7663_ChipOp.ClientSupportsVhtETxBF     = mt_WrapClientSupportsVhtETxBF;
	MT7663_ChipOp.setVHTETxBFCap             = mt7663_setVHTETxBFCap;
#endif /* VHT_TXBF_SUPPORT */
#endif /* TXBF_SUPPORT */
#ifdef INTERNAL_CAPTURE_SUPPORT
	MT7663_ChipOp.ICapStart = MtCmdRfTestICapStart;
	MT7663_ChipOp.ICapStatus = MtCmdRfTestGen2ICapStatus;
	MT7663_ChipOp.ICapCmdRawDataProc = MtCmdRfTestICapRawDataProc;
	MT7663_ChipOp.ICapGetIQData = Get_RBIST_IQ_Data;
	MT7663_ChipOp.ICapEventRawDataHandler = ExtEventICapIQDataHandler;
#endif /* INTERNAL_CAPTURE_SUPPORT */
#ifdef WIFI_SPECTRUM_SUPPORT
	MT7663_ChipOp.SpectrumStart = MtCmdWifiSpectrumStart;
	MT7663_ChipOp.SpectrumStatus = MtCmdWifiSpectrumGen2Status;
	MT7663_ChipOp.SpectrumCmdRawDataProc = MtCmdWifiSpectrumRawDataProc;
	MT7663_ChipOp.SpectrumEventRawDataHandler = ExtEventWifiSpectrumRawDataHandler;
#endif /* WIFI_SPECTRUM_SUPPORT */
#ifdef SMART_CARRIER_SENSE_SUPPORT
	MT7663_ChipOp.SmartCarrierSense = SmartCarrierSense_Gen4;
	MT7663_ChipOp.ChipSetSCS = SetSCS;
#endif /* SMART_CARRIER_SENSE_SUPPORT */
#ifdef GREENAP_SUPPORT
	MT7663_ChipOp.EnableAPMIMOPS = enable_greenap;
	MT7663_ChipOp.DisableAPMIMOPS = disable_greenap;
#endif /* GREENAP_SUPPORT */
	MT7663_ChipOp.dma_shdl_init = mt7663_dma_shdl_init;
#ifdef MT7663_FPGA
	MT7663_ChipOp.chk_hif_default_cr_setting = mt7663_chk_hif_default_cr_setting;
	MT7663_ChipOp.chk_top_default_cr_setting = mt7663_chk_top_default_cr_setting;
#endif
#ifdef CONFIG_ATE
	MT7663_ChipOp.set_ampdu_wtbl = mt_ate_wtbl_cfg_v2;
#endif /* CONFIG_ATE */
	MT7663_ChipOp.irq_init = mt7663_irq_init;
	MT7663_ChipOp.hif_init_dma = hif_init_WPDMA;
	MT7663_ChipOp.hif_set_dma = hif_set_WPDMA;
	MT7663_ChipOp.hif_wait_dma_idle = hif_wait_WPDMA_idle;
	MT7663_ChipOp.hif_reset_dma = hif_reset_WPDMA;
	MT7663_ChipOp.get_fw_sync_value = get_fw_sync_value;
	MT7663_ChipOp.read_chl_pwr = NULL;
	MT7663_ChipOp.parse_RXV_packet = parse_RXV_packet_v2;
	MT7663_ChipOp.txs_handler = txs_handler_v2;
#ifdef CONFIG_FWOWN_SUPPORT
	MT7663_ChipOp.driver_own = driver_own;
	MT7663_ChipOp.fw_own = fw_own;
#endif
	MT7663_ChipOp.hw_isr = mtd_isr;
#ifdef CONNAC_EFUSE_FORMAT_SUPPORT
	MT7663_ChipOp.eeprom_extract = mt7663_eeprom_info_extract;
#endif /* CONNAC_EFUSE_FORMAT_SUPPORT */
}

/*
* Bring out of OFFCHANNEL_SCAN_FEATURE flag,
* to make it generic function, to be used as generic requirement
*/
VOID mt7663_reset_enable_nf_registers(RTMP_ADAPTER *pAd, UCHAR bandidx)
{
	UINT32 Value = 0;
	if (bandidx == DBDC_BAND0) {
		/* Reset band0*/
		MAC_IO_READ32(pAd->hdev_ctrl, PHY_RXTD_12, &Value);
		Value |= (1 << B0IrpiSwCtrlResetOffset);
		Value |= (1 << B0IrpiSwCtrlOnlyOffset);

		MAC_IO_WRITE32(pAd->hdev_ctrl, PHY_RXTD_12, Value);
		MAC_IO_WRITE32(pAd->hdev_ctrl, PHY_RXTD_12, Value);
		/* Enable badn0 IPI control */
		HW_IO_READ32(pAd->hdev_ctrl, RO_BAND1_PHYCTRL_STS2, &Value);
		Value |= (B0IpiEnableCtrlValue << B0IpiEnableCtrlOffset);

		MAC_IO_WRITE32(pAd->hdev_ctrl, RO_BAND1_PHYCTRL_STS2, Value);
	}
}

/*
* Bring out of OFFCHANNEL_SCAN_FEATURE flag,
* to make it generic function, to be used as generic requirement
*/
VOID mt7663_calculate_nf(RTMP_ADAPTER *pAd, UCHAR bandidx)
{
	UINT8 NF_index = 0;
	UINT32 total = 0, value[11] = {0};
	INT32 NF, xNF = 0;
	INT16 NF_Power[] = {-92, -89, -86, -83, -80, -75, -70, -65, -60, -55, -52};
	UINT32 band_offset;

	if ((bandidx != DBDC_BAND0) && (bandidx != DBDC_BAND1))
		return;

	/*RTMP_SEM_LOCK(&pAd->ScanCtrl.NF_Lock); //not needed for now*/
	if (bandidx == DBDC_BAND0) {
		band_offset = PHY_RXTD1_0;
	}

	for (NF_index = 0; NF_index <= 10; NF_index++) {
	    MAC_IO_READ32(pAd->hdev_ctrl, (band_offset + 4 * NF_index), &value[NF_index]);
		total += value[NF_index];
		xNF += (NF_Power[NF_index] * (INT32)value[NF_index]);
	}

	if (xNF && total)
		NF = xNF/(INT32)total;

#ifdef OFFCHANNEL_SCAN_FEATURE
	if (pAd->ScanCtrl[bandidx].OffChScan == FALSE)
#endif
	{
		pAd->Avg_NFx16[bandidx] = (pAd->Avg_NF[bandidx] != 0) ? (NF - pAd->Avg_NF[bandidx] + pAd->Avg_NFx16[bandidx]) : NF << 4;
		pAd->Avg_NF[bandidx] = pAd->Avg_NFx16[bandidx] >> 4;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: Bandidx : %d xNF : %d Total : %u NF : %d pAd->Avg_NFx16[%d] : %d  pAd->Avg_NF[%d] : %d \n", __func__, bandidx, xNF, total, NF, bandidx, pAd->Avg_NFx16[bandidx], bandidx, pAd->Avg_NF[bandidx]));
	}

#ifdef OFFCHANNEL_SCAN_FEATURE
	if ((pAd->ScanCtrl[bandidx].OffChScan == TRUE)) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: Bandidx : %d Before  AvgNF : %d \n", __func__, bandidx, pAd->ChannelInfo.AvgNF));
		pAd->ChannelInfo.AvgNF = NF;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: Bandidx : %d Before  AvgNF : %d \n", __func__, bandidx, pAd->ChannelInfo.AvgNF));
	}

	if (pAd->ScanCtrl[bandidx].OffChScan == FALSE)
#endif
	{
		mt7663_reset_enable_nf_registers(pAd, bandidx);
	}

	/*RTMP_SEM_UNLOCK(&pAd->ScanCtrl.NF_Lock);*/
}

VOID mt7663_enable_nf_support(struct _RTMP_ADAPTER *pAd)
{
	int Value, i;

	for (i = 0; i < DBDC_BAND_NUM; i++) {

		pAd->Avg_NF[i] = 0;
		pAd->Avg_NFx16[i] = 0;
		/*band0*/

		if (i == DBDC_BAND0) {
			/*Turn on band 0 IPI*/
			MAC_IO_READ32(pAd->hdev_ctrl, PHY_RXTD_12, &Value);
			Value |= (1 << B0IrpiSwCtrlResetOffset);
			Value |= (1 << B0IrpiSwCtrlOnlyOffset);

			MAC_IO_WRITE32(pAd->hdev_ctrl, PHY_RXTD_12, Value);
			MAC_IO_WRITE32(pAd->hdev_ctrl, PHY_RXTD_12, Value);

			/* Enable badn0 IPI control */
			MAC_IO_READ32(pAd->hdev_ctrl, RO_BAND1_PHYCTRL_STS2, &Value);
			Value |= (B0IpiEnableCtrlValue << B0IpiEnableCtrlOffset);

			MAC_IO_WRITE32(pAd->hdev_ctrl, RO_BAND1_PHYCTRL_STS2, Value);
		}
	}
}

static INT mt7663AsicArchOpsInit(RTMP_ADAPTER *pAd, RTMP_ARCH_OP *arch_ops)
{
	arch_ops->asic_ampdu_efficiency_on_off = MtAsicAMPDUEfficiencyAdjustbyFW;
	arch_ops->archGetCrcErrCnt = MtAsicGetCrcErrCnt;
	arch_ops->archGetCCACnt = MtAsicGetCCACnt;
	arch_ops->archGetChBusyCnt = MtAsicGetChBusyCnt;
#ifdef CONFIG_STA_SUPPORT
	arch_ops->archEnableIbssSync = NULL;/* MtAsicEnableBssSyncByDriver; */
#endif /* CONFIG_STA_SUPPORT */
	arch_ops->archSetAutoFallBack = MtAsicSetAutoFallBack;
	arch_ops->archAutoFallbackInit = MtAsicAutoFallbackInit;
	arch_ops->archUpdateProtect = MtAsicUpdateProtectByFw;
	arch_ops->archUpdateRtsThld = MtAsicUpdateRtsThldByFw;
	arch_ops->archSwitchChannel = MtAsicSwitchChannel;
	arch_ops->archSetRDG = NULL;
	arch_ops->archSetDevMac = MtAsicSetDevMacByFw;
	arch_ops->archSetBssid = MtAsicSetBssidByFw;
	arch_ops->archSetStaRec = MtAsicSetStaRecByFw;
	arch_ops->archUpdateStaRecBa = MtAsicUpdateStaRecBaByFw;
	arch_ops->asic_rts_on_off = mt_asic_rts_on_off;
#ifdef CONFIG_AP_SUPPORT
	arch_ops->archSetMbssWdevIfAddr = MtAsicSetMbssWdevIfAddrGen2;
	arch_ops->archSetMbssHwCRSetting = MtDmacSetMbssHwCRSetting;
	arch_ops->archSetExtTTTTHwCRSetting = MtDmacSetExtTTTTHwCRSetting;
	arch_ops->archSetExtMbssEnableCR = MtDmacSetExtMbssEnableCR;
#endif /* CONFIG_AP_SUPPORT */
	arch_ops->archDelWcidTab = MtAsicDelWcidTabByFw;
	arch_ops->archAddRemoveKeyTab = MtAsicAddRemoveKeyTabByFw;
#ifdef HTC_DECRYPT_IOT
	arch_ops->archSetWcidAAD_OM = MtAsicUpdateStaRecAadOmByFw;
#endif
#ifdef BCN_OFFLOAD_SUPPORT
	/* sync with Carter, wilsonl */
	arch_ops->archEnableBeacon = NULL;
	arch_ops->archDisableBeacon = NULL;
	arch_ops->archUpdateBeacon = MtUpdateBcnAndTimToMcu;
#else
	arch_ops->archEnableBeacon = MtDmacAsicEnableBeacon;
	arch_ops->archDisableBeacon = MtDmacAsicDisableBeacon;
	arch_ops->archUpdateBeacon = MtUpdateBeaconToAsic;
#endif
#ifdef APCLI_SUPPORT
#ifdef MAC_REPEATER_SUPPORT
	arch_ops->archSetReptFuncEnable = MtAsicSetReptFuncEnableByFw;
	arch_ops->archInsertRepeaterEntry = MtAsicInsertRepeaterEntryByFw;
	arch_ops->archRemoveRepeaterEntry = MtAsicRemoveRepeaterEntryByFw;
	arch_ops->archInsertRepeaterRootEntry = MtAsicInsertRepeaterRootEntryByFw;
#endif /* MAC_REPEATER_SUPPORT */
#endif /* APCLI_SUPPORT */
	arch_ops->archSetPiggyBack = MtAsicSetPiggyBack;
	arch_ops->archSetPreTbtt = NULL;/* offload to BssInfoUpdateByFw */
	arch_ops->archSetGPTimer = MtAsicSetGPTimer;
	arch_ops->archSetChBusyStat = MtAsicSetChBusyStat;
	arch_ops->archGetTsfTime = MtAsicGetTsfTimeByFirmware;
	arch_ops->archDisableSync = NULL;/* MtAsicDisableSyncByDriver; */
	arch_ops->archSetSyncModeAndEnable = NULL;/* MtAsicEnableBssSyncByDriver; */
	arch_ops->archSetWmmParam = MtAsicSetWmmParam;
	arch_ops->archGetWmmParam = MtAsicGetWmmParam;
	arch_ops->archSetEdcaParm = MtAsicSetEdcaParm;
	arch_ops->archSetRetryLimit = MtAsicSetRetryLimit;
	arch_ops->archGetRetryLimit = MtAsicGetRetryLimit;
	arch_ops->archSetSlotTime = MtAsicSetSlotTime;
	arch_ops->archGetTxTsc = MtAsicGetTxTscByDriver;
	arch_ops->archAddSharedKeyEntry = MtAsicAddSharedKeyEntry;
	arch_ops->archRemoveSharedKeyEntry = MtAsicRemoveSharedKeyEntry;
	arch_ops->archAddPairwiseKeyEntry = MtAsicAddPairwiseKeyEntry;
	arch_ops->archSetBW = MtAsicSetBW;
	arch_ops->archSetCtrlCh = mt_mac_set_ctrlch;
	arch_ops->archWaitMacTxRxIdle = MtAsicWaitMacTxRxIdle;
#ifdef MAC_INIT_OFFLOAD
	arch_ops->archSetMacTxRx = MtAsicSetMacTxRxByFw;
	arch_ops->archSetRxvFilter = MtAsicSetRxvFilter;
	arch_ops->archSetMacMaxLen = NULL;
	arch_ops->archSetTxStream = NULL;
	arch_ops->archSetRxFilter = NULL;/* MtAsicSetRxFilter; */
#else
	arch_ops->archSetMacTxRx = MtAsicSetMacTxRx;
	arch_ops->archSetMacMaxLen = MtAsicSetMacMaxLen;
	arch_ops->archSetTxStream = MtAsicSetTxStream;
	arch_ops->archSetRxFilter = MtAsicSetRxFilter;
#endif /*MAC_INIT_OFFLOAD*/
	arch_ops->archSetMacWD = MtAsicSetMacWD;
#ifdef MAC_APCLI_SUPPORT
	arch_ops->archSetApCliBssid = MtAsicSetApCliBssid;
#endif /* MAC_APCLI_SUPPORT */
	arch_ops->archTOPInit = MtAsicTOPInit;
	arch_ops->archSetTmrCR = MtSetTmrCRByFw;
	arch_ops->archUpdateRxWCIDTable = MtAsicUpdateRxWCIDTableByFw;
#ifdef TXBF_SUPPORT
	arch_ops->archUpdateClientBfCap = mt_AsicClientBfCap;
#endif /* TXBF_SUPPORT */
	arch_ops->archUpdateBASession = MtAsicUpdateBASessionOffloadByFw;
	arch_ops->archGetTidSn = MtAsicGetTidSnByDriver;
	arch_ops->archSetSMPS = MtAsicSetSMPSByDriver;
	arch_ops->archRxHeaderTransCtl = MtAsicRxHeaderTransCtl;
	arch_ops->archRxHeaderTaranBLCtl = MtAsicRxHeaderTaranBLCtl;
	arch_ops->rx_pkt_process = mt_rx_pkt_process;
	arch_ops->get_packet_type = mtd_get_packet_type;
	arch_ops->trans_rxd_into_rxblk = mtd_trans_rxd_into_rxblk;
	arch_ops->archSetRxStream = NULL;/* MtAsicSetRxStream; */
#ifdef DOT11_VHT_AC
	arch_ops->archSetRtsSignalTA = MtAsicSetRtsSignalTA;
#endif /*  DOT11_VHT_AC */
#ifdef IGMP_SNOOP_SUPPORT
	arch_ops->archMcastEntryInsert = MulticastFilterTableInsertEntry;
	arch_ops->archMcastEntryDelete = MulticastFilterTableDeleteEntry;
#endif
	arch_ops->write_txp_info = mtd_write_txp_info_by_host_v2;
	arch_ops->write_tmac_info_fixed_rate = mtd_write_tmac_info_fixed_rate;
	arch_ops->write_tmac_info = mtd_write_tmac_info_by_host;
	arch_ops->dump_tmac_info = mtd_dump_tmac_info;
	arch_ops->write_tx_resource = mtd_pci_write_tx_resource;
	arch_ops->write_frag_tx_resource = mt_pci_write_frag_tx_resource;
	arch_ops->kickout_data_tx = pci_kickout_data_tx;
	arch_ops->get_pkt_from_rx_resource = mtd_pci_get_pkt_from_rx_resource;
	arch_ops->get_pkt_from_rx1_resource = mtd_pci_get_pkt_from_rx_resource;
	arch_ops->get_resource_idx = mtd_pci_get_resource_idx;
	arch_ops->build_resource_idx = mtd_pci_build_resource_idx;
	arch_ops->get_tx_resource_free_num = pci_get_tx_resource_free_num;
	arch_ops->check_hw_resource = mt_ct_check_hw_resource;
	arch_ops->inc_resource_full_cnt = pci_inc_resource_full_cnt;
	arch_ops->get_resource_state = pci_get_resource_state;
	arch_ops->set_resource_state = pci_set_resource_state;
	arch_ops->check_resource_state = pci_check_resource_state;
	arch_ops->get_hif_buf = mt_pci_get_hif_buf;
	arch_ops->hw_tx = mt_ct_hw_tx;
	arch_ops->mlme_hw_tx = mt_ct_mlme_hw_tx;
#ifdef CONFIG_ATE
	arch_ops->ate_hw_tx = mt_ct_ate_hw_tx;
#endif
	arch_ops->rx_done_handle = mtd_rx_done_handle;
	arch_ops->cmd_dma_done_handle = mt_cmd_dma_done_handle;
	arch_ops->fwdl_dma_done_handle = mt_fwdl_dma_done_handle;
	arch_ops->tx_dma_done_handle = mtd_tx_dma_done_handle;

#ifdef RED_SUPPORT
	arch_ops->archRedMarkPktDrop = RedMarkPktDrop;
#endif

#if defined(NF_SUPPORT) || defined(OFFCHANNEL_SCAN_FEATURE)
		arch_ops->arch_calculate_nf = mt7663_calculate_nf;
		arch_ops->arch_reset_enable_nf_registers = mt7663_reset_enable_nf_registers;
		arch_ops->arch_enable_nf_support = mt7663_enable_nf_support;
#endif

	return TRUE;
}


VOID mt7663_init(RTMP_ADAPTER *pAd)
{
	RTMP_CHIP_CAP *pChipCap = hc_get_chip_cap(pAd->hdev_ctrl);
	RTMP_ARCH_OP *arch_ops = hc_get_arch_ops(pAd->hdev_ctrl);
	struct _RTMP_CHIP_DBG *chip_dbg = hc_get_chip_dbg(pAd->hdev_ctrl);
	struct tr_delay_control *tr_delay_ctl = &pAd->tr_ctl.tr_delay_ctl;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s()-->\n", __func__));
	mt7663_chipCap_init();
	mt7663_chipOp_init();
	NdisMoveMemory(pChipCap, &MT7663_ChipCap, sizeof(RTMP_CHIP_CAP));
	hc_register_chip_ops(pAd->hdev_ctrl, &MT7663_ChipOp);
	mt7663AsicArchOpsInit(pAd, arch_ops);
	mt7663_chip_dbg_init(chip_dbg);
	mt_phy_probe(pAd);
	RTMP_DRS_ALG_INIT(pAd, RATE_ALG_AGBS);
	/* Following function configure beacon related parameters in pChipCap
	 * FlgIsSupSpecBcnBuf / BcnMaxHwNum / WcidHwRsvNum / BcnMaxHwSize / BcnBase[]
	 */
	/* sync with Cater, wilsonl */
	mt_chip_bcn_parameter_init(pAd);
	/* sync with Cater, wilsonl */
	pChipCap->OmacNums = 5;
	pChipCap->BssNums = 4;
	pChipCap->ExtMbssOmacStartIdx = 0x10;
	pChipCap->RepeaterStartIdx = 0x20;
#ifdef AIR_MONITOR
	pChipCap->MaxRepeaterNum = 16;
#else
	pChipCap->MaxRepeaterNum = 24;
#endif /* AIR_MONITOR */

#ifdef BCN_OFFLOAD_SUPPORT
	pChipCap->fgBcnOffloadSupport = TRUE;
	pChipCap->fgIsNeedPretbttIntEvent = FALSE;
#endif
	/* TMR HW version */
	pChipCap->TmrHwVer = TMR_VER_2_0;

	/* For calibration log buffer size limitation issue */
	pAd->fgQAtoolBatchDumpSupport = TRUE;
#ifdef RED_SUPPORT
	pAd->red_have_cr4 = FALSE;
#endif /* RED_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
	/*VOW CR Address offset - Gen_TALOS*/
	pAd->vow_gen.VOW_GEN = VOW_GEN_TALOS;
#endif /* #ifdef CONFIG_AP_SUPPORT */

	tr_delay_ctl->ul_rx_dly_ctl_tbl = mt7663_rx_dly_ctl_ul_tbl;
	tr_delay_ctl->ul_rx_dly_ctl_tbl_size = (ARRAY_SIZE(mt7663_rx_dly_ctl_ul_tbl));
	tr_delay_ctl->dl_rx_dly_ctl_tbl = mt7663_rx_dly_ctl_dl_tbl;
	tr_delay_ctl->dl_rx_dly_ctl_tbl_size = (ARRAY_SIZE(mt7663_rx_dly_ctl_dl_tbl));

	/* For TP tuning dynamically */
	pChipCap->Ap5GPeakTpTH = 580;
	pChipCap->Ap2GPeakTpTH = 270;
	pChipCap->ApDBDC5GPeakTpTH = 0;
	pChipCap->ApDBDC2GPeakTpTH = 0;

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("<--%s()\n", __func__));
}


