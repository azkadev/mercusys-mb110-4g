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
	mt7663_manual_cal.c
*/
#include "rt_config.h"

#if defined(MT7663_RFB_52)
UINT32 cal_id = 52;
#else
#if defined(MT7663_RFB_19)
UINT32 cal_id = 19;
#else
UINT32 cal_id;
#endif
#endif

typedef struct _CAL_PAIR {
	UINT32 Register;
	UINT32 Value;
} CAL_PAIR;

struct _CAL_PAIR mt7663_manual_cal_19[] = {
	/* ------TXA----- */
	/* WF0 Gain imb. */
	{(WF_PHY_BASE + 0x4400), 0x00030003}, /* 0x82074400 */
	{(WF_PHY_BASE + 0x4404), 0x00030003}, /* 0x82074404 */
	{(WF_PHY_BASE + 0x4408), 0x00030003}, /* 0x82074408 */
	{(WF_PHY_BASE + 0x440C), 0x00050005}, /* 0x8207440C */
	{(WF_PHY_BASE + 0x4410), 0x00050005}, /* 0x82074410 */
	{(WF_PHY_BASE + 0x4414), 0x00050005}, /* 0x82074414 */

	/* WF0 Phase imb. */
	{(WF_PHY_BASE + 0x4418), 0x003F3F3F}, /* 0x82074418 */
	{(WF_PHY_BASE + 0x441C), 0x3B3B0000}, /* 0x8207441C */
	{(WF_PHY_BASE + 0x4420), 0x3B3B3B3B}, /* 0x82074420 */

	/* WF0 DC I */
	{(WF_PHY_BASE + 0x4424), 0xF4F7F8F8}, /* 0x82074424 */
	{(WF_PHY_BASE + 0x4428), 0xFCFBF1F2}, /* 0x82074428 */
	{(WF_PHY_BASE + 0x442C), 0xFAFAF9FC}, /* 0x8207442C */

	/* WF0 DC Q */
	{(WF_PHY_BASE + 0x4430), 0x20181614}, /* 0x82074430 */
	{(WF_PHY_BASE + 0x4434), 0x15132723}, /* 0x82074434 */
	{(WF_PHY_BASE + 0x4438), 0x24211E17}, /* 0x82074438 */

	/* WF0 LPFG */
	{(WF_PHY_BASE + 0x444C), 0x00000000}, /* 0x8207444C */
	{(WF_PHY_BASE + 0x4450), 0x00000000}, /* 0x82074450 */

	/* WF1 Gain imb. */
	{(WF_PHY_BASE + 0x5400), 0x00060006}, /* 0x82075400 */
	{(WF_PHY_BASE + 0x5404), 0x00060006}, /* 0x82075404 */
	{(WF_PHY_BASE + 0x5408), 0x00060006}, /* 0x82075408 */
	{(WF_PHY_BASE + 0x540C), 0x00080008}, /* 0x8207540C */
	{(WF_PHY_BASE + 0x5410), 0x00080008}, /* 0x82075410 */
	{(WF_PHY_BASE + 0x5414), 0x00080008}, /* 0x82075414 */

	/* WF1 Phase imb. */
	{(WF_PHY_BASE + 0x5418), 0x01010101}, /* 0x8207540C */
	{(WF_PHY_BASE + 0x541C), 0x3F3F0101}, /* 0x82075410 */
	{(WF_PHY_BASE + 0x5420), 0x3F3F3F3F}, /* 0x82075414 */

	/* WF1 DC I */
	{(WF_PHY_BASE + 0x5424), 0x13100F0E}, /* 0x82075424 */
	{(WF_PHY_BASE + 0x5428), 0x0F0E1514}, /* 0x82075428 */
	{(WF_PHY_BASE + 0x542C), 0x1413120F}, /* 0x82075414 */

	/* WF1 DC Q */
	{(WF_PHY_BASE + 0x5430), 0x07060606}, /* 0x82075430 */
	{(WF_PHY_BASE + 0x5434), 0x08080908}, /* 0x82075434 */
	{(WF_PHY_BASE + 0x5438), 0x0F0D0B09}, /* 0x82075438 */

	/* WF1 LPFG */
	{(WF_PHY_BASE + 0x544C), 0x00000000}, /* 0x8207544C */
	{(WF_PHY_BASE + 0x5450), 0x00000000}, /* 0x82075450 */

	/* ------RX----- */
	/* wf0 */
	{(WF_PHY_BASE + 0x4520), 0x00000200}, /* gain mannual tunning, default 200, range 100~300 */
	{(WF_PHY_BASE + 0x4558), 0x00000000}, /* phase mannual tunning, default 00, range D0 ~ 20 */

	/* wf1 */
	{(WF_PHY_BASE + 0x5520), 0x00000204}, /* gain mannual tunning, default 200, range 100~300 */
	{(WF_PHY_BASE + 0x5558), 0x00000000}, /* phase mannual tunning, default 00, range D0 ~ 20 */

	{0x0, 0x0},
};


struct _CAL_PAIR mt7663_manual_cal_52[] = {
	/* ------TXA----- */
	/* WF0 Gain imb. */
	{(WF_PHY_BASE + 0x4400), 0x00030003}, /* 0x82074400 */
	{(WF_PHY_BASE + 0x4404), 0x00030003}, /* 0x82074404 */
	{(WF_PHY_BASE + 0x4408), 0x00030003}, /* 0x82074408 */
	{(WF_PHY_BASE + 0x440C), 0x00050005}, /* 0x8207440C */
	{(WF_PHY_BASE + 0x4410), 0x00050005}, /* 0x82074410 */
	{(WF_PHY_BASE + 0x4414), 0x00050005}, /* 0x82074414 */

	/* WF0 Phase imb. */
	{(WF_PHY_BASE + 0x4418), 0x3C3C3C3C}, /* 0x82074418 */
	{(WF_PHY_BASE + 0x441C), 0x38383C3C}, /* 0x8207441C */
	{(WF_PHY_BASE + 0x4420), 0x38383838}, /* 0x82074420 */

	/* WF0 DC I */
	{(WF_PHY_BASE + 0x4424), 0x00ff0000}, /* 0x82074424 */
	{(WF_PHY_BASE + 0x4428), 0x0202FEFF}, /* 0x82074428 */
	{(WF_PHY_BASE + 0x442C), 0x03030302}, /* 0x8207442C */

	/* WF0 DC Q */
	{(WF_PHY_BASE + 0x4430), 0xff010101}, /* 0x82074430 */
	{(WF_PHY_BASE + 0x4434), 0x06040000}, /* 0x82074434 */
	{(WF_PHY_BASE + 0x4438), 0x0d0B0707}, /* 0x82074438 */

	/* WF0 LPFG */
	{(WF_PHY_BASE + 0x444C), 0x00000000}, /* 0x8207444C */
	{(WF_PHY_BASE + 0x4450), 0x00000000}, /* 0x82074450 */

	/* WF1 Gain imb. */
	{(WF_PHY_BASE + 0x5400), 0x01F501F5}, /* 0x82075400 */
	{(WF_PHY_BASE + 0x5404), 0x01F501F5}, /* 0x82075404 */
	{(WF_PHY_BASE + 0x5408), 0x01F501F5}, /* 0x82075408 */
	{(WF_PHY_BASE + 0x540C), 0x01F501F5}, /* 0x8207540C */
	{(WF_PHY_BASE + 0x5410), 0x01F501F5}, /* 0x82075410 */
	{(WF_PHY_BASE + 0x5414), 0x01F501F5}, /* 0x82075414 */

	/* WF1 Phase imb. */
	{(WF_PHY_BASE + 0x5418), 0x00000000}, /* 0x8207540C */
	{(WF_PHY_BASE + 0x541C), 0x3E3E0000}, /* 0x82075410 */
	{(WF_PHY_BASE + 0x5420), 0x3e3e3e3e}, /* 0x82075414 */

	/* WF1 DC I */
	{(WF_PHY_BASE + 0x5424), 0x0d0A0A0A}, /* 0x82075424 */
	{(WF_PHY_BASE + 0x5428), 0x0e0d0c0c}, /* 0x82075428 */
	{(WF_PHY_BASE + 0x542C), 0x1413120E}, /* 0x82075414 */

	/* WF1 DC Q */
	{(WF_PHY_BASE + 0x5430), 0x19121212}, /* 0x82075430 */
	{(WF_PHY_BASE + 0x5434), 0x16151919}, /* 0x82075434 */
	{(WF_PHY_BASE + 0x5438), 0x24222018}, /* 0x82075438 */

	/* WF1 LPFG */
	{(WF_PHY_BASE + 0x544C), 0x00000000}, /* 0x8207544C */
	{(WF_PHY_BASE + 0x5450), 0x00000000}, /* 0x82075450 */

	/* ------RX----- */
	/* wf0 */
	{(WF_PHY_BASE + 0x4520), 0x000001F8}, /* gain mannual tunning, default 200, range 100~300 */
	{(WF_PHY_BASE + 0x4558), 0x00000000}, /* phase mannual tunning, default 00, range D0 ~ 20 */

	/* wf1 */
	{(WF_PHY_BASE + 0x5520), 0x00000200}, /* gain mannual tunning, default 200, range 100~300 */
	{(WF_PHY_BASE + 0x5558), 0x00000000}, /* phase mannual tunning, default 00, range D0 ~ 20 */

	{0x0, 0x0},
};

INT set_manual_cal_id(
	PRTMP_ADAPTER pAd,
	char *arg)
{
	cal_id = simple_strtol(arg, 0, 10);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("(%s)\n", __func__));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			 ("cal_id = %u\n", cal_id));
	return TRUE;
}


BOOLEAN mt7663_manual_cal(RTMP_ADAPTER *pAd)
{
	INT i = 0;
	CAL_PAIR *cal_mac_cr_range = NULL;
	CAL_PAIR *cal_pair = NULL;

	INT test = 0;

	if (cal_id == 0) {
		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): cal_id = %u !\n", __func__, cal_id));
		return FALSE;
	}

	if (cal_id == 19) {
		MtCmdRFRegAccessWrite(pAd, 0, 0x60C, 0x0517050F);
		MtCmdRFRegAccessWrite(pAd, 1, 0x60C, 0x05170513);
		cal_mac_cr_range = &mt7663_manual_cal_19[0];
	}

	if (cal_id == 52) {
		MtCmdRFRegAccessWrite(pAd, 0, 0x60C, 0x05170513);
		MtCmdRFRegAccessWrite(pAd, 1, 0x60C, 0x05170511);
		cal_mac_cr_range = &mt7663_manual_cal_52[0];
	}

	if (!cal_mac_cr_range) {
		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): No Need Manual Calibration !\n", __func__));
		return FALSE;
	}

	while (1) {

		cal_pair = cal_mac_cr_range + i;

		if ((cal_pair->Register == 0))
			break;

		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("1.%s(): Idx(%d) Register(0x%x) = 0x%x\n",
									__func__, i, cal_pair->Register, cal_pair->Value));

		MAC_IO_WRITE32(pAd, cal_pair->Register, cal_pair->Value);

		i++;
	};

	return TRUE;

}

