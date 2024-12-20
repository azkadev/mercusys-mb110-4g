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
	cmm_txbf_cal_mt.c
*/

#ifdef COMPOS_WIN
#include "MtConfig.h"
#if defined(EVENT_TRACING)
#include "Cmm_txbf_cal_mt.h"
#endif
#elif defined(COMPOS_TESTMODE_WIN)
#include "config.h"
#else
#include "rt_config.h"
#endif

#ifdef TXBF_SUPPORT


extern UINT_8 ibf_value[];
extern UCHAR ibf_status;

UINT_16 au2IBfCalEEPROMOffset[9] = {
	IBF_LNA_PHASE_G0_ADDR,
	IBF_LNA_PHASE_G1_ADDR,
	IBF_LNA_PHASE_G2_ADDR,
	IBF_LNA_PHASE_G3_ADDR,
	IBF_LNA_PHASE_G4_ADDR,
	IBF_LNA_PHASE_G5_ADDR,
	IBF_LNA_PHASE_G6_ADDR,
	IBF_LNA_PHASE_G7_ADDR,
	IBF_LNA_PHASE_G8_ADDR
};


VOID E2pMemWrite(IN PRTMP_ADAPTER pAd,
		 IN UINT16 u2MemAddr,
		 IN UCHAR  ucInput_L,
		 IN UCHAR  ucInput_H)
{
	UINT16 u2Value;

	if ((u2MemAddr & 0x1) != 0)
		u2Value = (ucInput_L << 8) | ucInput_H;
	else
		u2Value = (ucInput_H << 8) | ucInput_L;

	RT28xx_EEPROM_WRITE16(pAd, u2MemAddr, u2Value);
}

#if defined(MT7663)
VOID mt7663_eBFPfmuMemAlloc(IN PRTMP_ADAPTER pAd, IN PCHAR pPfmuMemRow, IN PCHAR pPfmuMemCol)
{
	pPfmuMemRow[0] = 0;
	pPfmuMemRow[1] = 0;
	pPfmuMemRow[2] = 1;
	pPfmuMemRow[3] = 1;

	pPfmuMemCol[0] = 0;
	pPfmuMemCol[1] = 3;
	pPfmuMemCol[2] = 0;
	pPfmuMemCol[3] = 3;
}

VOID mt7663_iBFPfmuMemAlloc(IN PRTMP_ADAPTER pAd, IN PCHAR pPfmuMemRow, IN PCHAR pPfmuMemCol)
{
	pPfmuMemRow[0] = 2;
	pPfmuMemRow[1] = 2;
	pPfmuMemRow[2] = 3;
	pPfmuMemRow[3] = 3;

	pPfmuMemCol[0] = 0;
	pPfmuMemCol[1] = 3;
	pPfmuMemCol[2] = 0;
	pPfmuMemCol[3] = 3;
}
#endif /* MT7663 */

#if defined(MT7663) || defined(MT7626)
VOID mt_iBFPhaseComp(IN PRTMP_ADAPTER pAd, IN UCHAR ucGroup, IN PCHAR pCmdBuf)
{
	UINT_8 u1IbfCalPhaseStructLenG0 = 0;
	UINT_8 u1IbfCalPhaseStructLenGx = 0;

#if defined(MT7663)
	u1IbfCalPhaseStructLenG0 = sizeof(MT7663_IBF_PHASE_G0_T);
	u1IbfCalPhaseStructLenGx = sizeof(MT7663_IBF_PHASE_Gx_T);
#endif

	if (ucGroup == 0)
		os_move_mem(pCmdBuf, pAd->piBfPhaseG0, u1IbfCalPhaseStructLenG0);
	else
		os_move_mem(pCmdBuf, &pAd->piBfPhaseGx[(ucGroup - 1) * u1IbfCalPhaseStructLenGx], u1IbfCalPhaseStructLenGx);

}
#endif /* MT7663 */



#if defined(MT7663) || defined(MT7626)
VOID mt_iBFPhaseFreeMem(IN PRTMP_ADAPTER pAd)
{
	/* Group 0 */
	if (pAd->piBfPhaseG0 != NULL) {
		os_free_mem(pAd->piBfPhaseG0);
	}

	/* Group 1 ~ 8 */
	if (pAd->piBfPhaseGx != NULL) {
		os_free_mem(pAd->piBfPhaseGx);
	}
}
#endif /* MT7663 || MT7626 */






#if defined(MT7663) || defined(MT7626)
VOID mt_iBFPhaseCalInit(IN PRTMP_ADAPTER pAd)
{
	UINT_8 u1IbfCalPhaseStructLenG0 = 0;
	UINT_8 u1IbfCalPhaseStructLenGx = 0;


#if defined(MT7663)
	u1IbfCalPhaseStructLenG0 = sizeof(MT7663_IBF_PHASE_G0_T);
	u1IbfCalPhaseStructLenGx = sizeof(MT7663_IBF_PHASE_Gx_T);
#endif

	/* Free memory allocated by iBF phase calibration */
	mt_iBFPhaseFreeMem(pAd);

	os_alloc_mem(pAd, (PUCHAR *)&pAd->piBfPhaseG0, u1IbfCalPhaseStructLenG0);
	os_alloc_mem(pAd, (PUCHAR *)&pAd->piBfPhaseGx, u1IbfCalPhaseStructLenGx * 8);

	NdisZeroMemory(pAd->piBfPhaseG0, u1IbfCalPhaseStructLenG0);
	NdisZeroMemory(pAd->piBfPhaseGx, u1IbfCalPhaseStructLenGx * 8);

	pAd->fgCalibrationFail = FALSE;
	NdisZeroMemory(&pAd->fgGroupIdPassFailStatus[0], 9);
}
#endif /* MT7663 || MT7626 */






#if defined(MT7663) || defined(MT7626)
VOID mt_iBFPhaseCalE2PInit(IN PRTMP_ADAPTER pAd)
{
	UINT_16 u2BfE2pOccupiedSize;
	UINT_8 u1IbfCalPhaseStructLenG0 = 0;
	UINT_8 u1IbfCalPhaseStructLenGx = 0;


#if defined(MT7663)
	u1IbfCalPhaseStructLenG0 = sizeof(MT7663_IBF_PHASE_G0_T);
	u1IbfCalPhaseStructLenGx = sizeof(MT7663_IBF_PHASE_Gx_T);
#endif

	/* Group 0 */
	u2BfE2pOccupiedSize = u1IbfCalPhaseStructLenG0;
	RT28xx_EEPROM_READ_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_0], u2BfE2pOccupiedSize, pAd->piBfPhaseG0);

	/* Group 1 ~ 7 */
	u2BfE2pOccupiedSize = (u1IbfCalPhaseStructLenGx * 7);
	RT28xx_EEPROM_READ_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_1], u2BfE2pOccupiedSize, pAd->piBfPhaseGx);

	/* Group 8 */
	u2BfE2pOccupiedSize = u1IbfCalPhaseStructLenGx;
	RT28xx_EEPROM_READ_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_8], u2BfE2pOccupiedSize, (PUCHAR)&pAd->piBfPhaseGx[7 * u1IbfCalPhaseStructLenGx]);

	pAd->fgCalibrationFail = FALSE;
	NdisZeroMemory(&pAd->fgGroupIdPassFailStatus[0], 9);
}
#endif /* MT7663 || MT7626 */







#if defined(MT7663) || defined(MT7626)
VOID mt_iBFPhaseCalE2PUpdate(IN PRTMP_ADAPTER pAd,
			  IN UCHAR   ucGroup,
			  IN BOOLEAN fgSX2,
			  IN UCHAR   ucUpdateAllTye)
{
#if defined(MT7663)
	MT7663_IBF_PHASE_Gx_T iBfPhaseGx;
	MT7663_IBF_PHASE_G0_T iBfPhaseG0;
#endif
	UCHAR  ucGroupIdx, ucEndLoop;
	UCHAR  ucCounter;
	UINT_8 u1IbfCalPhaseStructLenG0 = 0;
	UINT_8 u1IbfCalPhaseStructLenGx = 0;


#if defined(MT7663)
	u1IbfCalPhaseStructLenG0 = sizeof(MT7663_IBF_PHASE_G0_T);
	u1IbfCalPhaseStructLenGx = sizeof(MT7663_IBF_PHASE_Gx_T);
#endif

	/* UINT16 u2Value; */
	/* IF phase calibration is for BW20/40/80/160 */
	ucGroupIdx = 0;
	NdisZeroMemory(&iBfPhaseG0, u1IbfCalPhaseStructLenG0);
	NdisZeroMemory(&iBfPhaseGx, u1IbfCalPhaseStructLenGx);

	switch (ucUpdateAllTye) {
	case IBF_PHASE_ONE_GROUP_UPDATE:
		ucGroupIdx = ucGroup - 1;
		ucEndLoop  = 0;

		if (ucGroup == GROUP_0) {
			RT28xx_EEPROM_WRITE_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[ucGroup], u1IbfCalPhaseStructLenG0, pAd->piBfPhaseG0);
			NdisCopyMemory(&iBfPhaseG0, pAd->piBfPhaseG0, u1IbfCalPhaseStructLenG0);
		} else {
			if (pAd->piBfPhaseGx)
			{
				RT28xx_EEPROM_WRITE_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[ucGroup],  u1IbfCalPhaseStructLenGx, &pAd->piBfPhaseGx[ucGroupIdx * u1IbfCalPhaseStructLenGx]);
				NdisCopyMemory(&iBfPhaseGx, &pAd->piBfPhaseGx[ucGroupIdx * u1IbfCalPhaseStructLenGx], u1IbfCalPhaseStructLenGx);
			}
		}

		if (ucGroup == GROUP_0) {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G%d and Group_M\n G%d_M_T0_H = %d\n G%d_M_T1_H = %d\n",
				ucGroup,
				ucGroup, iBfPhaseG0.ucG0_M_T0_H,
				ucGroup, iBfPhaseG0.ucG0_M_T1_H));
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G%d_R0_UH = %d\n G%d_R0_H = %d\n G%d_R0_M = %d\n G%d_R0_L = %d\n G%d_R1_UH = %d\n G%d_R1_H = %d\n G%d_R1_M = %d\n G%d_R1_L = %d\n",
				ucGroup, iBfPhaseG0.ucG0_R0_UH,
				ucGroup, iBfPhaseG0.ucG0_R0_H,
				ucGroup, iBfPhaseG0.ucG0_R0_M,
				ucGroup, iBfPhaseG0.ucG0_R0_L,
				ucGroup, iBfPhaseG0.ucG0_R1_UH,
				ucGroup, iBfPhaseG0.ucG0_R1_H,
				ucGroup, iBfPhaseG0.ucG0_R1_M,
				ucGroup, iBfPhaseG0.ucG0_R1_L));
		} else {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G%d and Group_M\n G%d_M_T0_H = %d\n G%d_M_T1_H = %d\n",
				ucGroup,
				ucGroup, iBfPhaseGx.ucGx_M_T0_H,
				ucGroup, iBfPhaseGx.ucGx_M_T1_H));
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G%d_R0_UH = %d\n G%d_R0_H = %d\n G%d_R0_M = %d\n G%d_R0_L = %d\n G%d_R1_UH = %d\n G%d_R1_H = %d\n G%d_R1_M = %d\n G%d_R1_L = %d\n",
				ucGroup, iBfPhaseGx.ucGx_R0_UH,
				ucGroup, iBfPhaseGx.ucGx_R0_H,
				ucGroup, iBfPhaseGx.ucGx_R0_M,
				ucGroup, iBfPhaseGx.ucGx_R0_L,
				ucGroup, iBfPhaseGx.ucGx_R1_UH,
				ucGroup, iBfPhaseGx.ucGx_R1_H,
				ucGroup, iBfPhaseGx.ucGx_R1_M,
				ucGroup, iBfPhaseGx.ucGx_R1_L));
		}
		break;

	case IBF_PHASE_ALL_GROUP_UPDATE:
		if (pAd->fgCalibrationFail == FALSE) {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("All of groups can pass criterion and calibrated phases can be written into EEPROM\n"));
			if ((pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_ALL) || 
				(pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_2G))
			{
				/* Write Group 0 into EEPROM */
				RT28xx_EEPROM_WRITE_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_0], u1IbfCalPhaseStructLenG0, pAd->piBfPhaseG0);
			}

			if ((pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_ALL) || 
				(pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_5G))
			{
				/* Write Group 1 ~ 8 into EEPROM */
				RT28xx_EEPROM_WRITE_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_1], (u1IbfCalPhaseStructLenGx * 8), pAd->piBfPhaseGx);
			}
		} else {
			MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibrated phases can't be written into EEPROM because some groups can't pass criterion!!!\n"));

			for (ucCounter = GROUP_0; ucCounter <= GROUP_8; ucCounter++) {
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Group%d = %s\n",
						 ucCounter,
						 (pAd->fgGroupIdPassFailStatus[ucCounter] == TRUE) ? "FAIL" : "PASS"));
			}
		}

		break;

	case IBF_PHASE_ALL_GROUP_ERASE:
		NdisZeroMemory(pAd->piBfPhaseG0, u1IbfCalPhaseStructLenG0);
		NdisZeroMemory(pAd->piBfPhaseGx, u1IbfCalPhaseStructLenGx * 8);

		if ((pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_ALL) || 
			(pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_2G))
		{
			RT28xx_EEPROM_WRITE_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_0], u1IbfCalPhaseStructLenG0, pAd->piBfPhaseG0);
		}

		if ((pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_ALL) || 
			(pAd->u1IbfCalPhase2G5GE2pClean == CLEAN_5G))
		{
			RT28xx_EEPROM_WRITE_WITH_RANGE(pAd, au2IBfCalEEPROMOffset[GROUP_1], (u1IbfCalPhaseStructLenGx * 8), pAd->piBfPhaseGx);
		}
		break;

	case IBF_PHASE_ALL_GROUP_READ_FROM_E2P:
		mt_iBFPhaseCalE2PInit(pAd);
		break;

	default:
		ucGroupIdx = ucGroup - 1;
		ucEndLoop  = 0;

		NdisCopyMemory(&iBfPhaseGx, &pAd->piBfPhaseGx[ucGroupIdx * u1IbfCalPhaseStructLenGx], u1IbfCalPhaseStructLenGx);
		NdisCopyMemory(&iBfPhaseG0, pAd->piBfPhaseG0, u1IbfCalPhaseStructLenG0);
		break;
	}
}
#endif /* MT7663 || MT7626 */

#if defined(MT7663) || defined(MT7626)
VOID mt_iBFPhaseCalReport(IN PRTMP_ADAPTER pAd,
		   IN UCHAR   ucGroupL_M_H,
		   IN UCHAR   ucGroup,
		   IN BOOLEAN fgSX2,
		   IN UCHAR   ucStatus,
		   IN UCHAR   ucPhaseCalType,
		   IN PUCHAR  pBuf)
{
#if defined(MT7663)
	MT7663_IBF_PHASE_G0_T   iBfPhaseG0;
	MT7663_IBF_PHASE_Gx_T   iBfPhaseGx;
	P_MT7663_IBF_PHASE_G0_T piBfPhaseG0;
	P_MT7663_IBF_PHASE_Gx_T piBfPhaseGx;
	MT7663_IBF_PHASE_OUT    iBfPhaseOut;
#endif

	UCHAR  ucGroupIdx;
	PUCHAR pucIBfPhaseG;
	UINT_8 u1IbfCalPhaseStructLenG0 = 0;
	UINT_8 u1IbfCalPhaseStructLenGx = 0;
	UINT_8 u1IbfPhaseOutStructLen   = 0;


#if defined(MT7663)
	u1IbfCalPhaseStructLenG0 = sizeof(MT7663_IBF_PHASE_G0_T);
	u1IbfCalPhaseStructLenGx = sizeof(MT7663_IBF_PHASE_Gx_T);
	u1IbfPhaseOutStructLen   = sizeof(MT7663_IBF_PHASE_OUT);
#endif

	MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s :: Calibrated iBF phases\n", __func__));
	pucIBfPhaseG = pBuf + u1IbfPhaseOutStructLen;
	NdisCopyMemory(&iBfPhaseOut, pBuf, u1IbfPhaseOutStructLen);

	switch (ucPhaseCalType) {
	case IBF_PHASE_CAL_NOTHING: /* Do nothing */
		break;

	case IBF_PHASE_CAL_NORMAL_INSTRUMENT:
	case IBF_PHASE_CAL_NORMAL: /* Store calibrated phases with buffer mode */
		/* IF phase calibration is for DBDC or 80+80 */
		if ((fgSX2 == TRUE) && (ucGroup > 0)) {
			ucGroupIdx = ucGroup - 1;
			NdisCopyMemory(&iBfPhaseGx, pucIBfPhaseG, u1IbfCalPhaseStructLenGx);

#if defined(MT7663)
			piBfPhaseGx = (P_MT7663_IBF_PHASE_Gx_T)&pAd->piBfPhaseGx[ucGroupIdx * u1IbfCalPhaseStructLenGx];
#endif


			switch (ucGroupL_M_H) {
			case GROUP_L:
				break;

			case GROUP_M:
				break;

			case GROUP_H:
				break;
			}

			return;
		}

		/* IF phase calibration is for BW20/40/80 */
		if (ucGroup == GROUP_0) {
			NdisCopyMemory(&iBfPhaseG0, pucIBfPhaseG, u1IbfCalPhaseStructLenG0);

#if defined(MT7663)
			piBfPhaseG0 = (P_MT7663_IBF_PHASE_G0_T)pAd->piBfPhaseG0;
#endif

			switch (ucGroupL_M_H) {
			case GROUP_L:
				break;

			case GROUP_M:
				piBfPhaseG0->ucG0_M_T0_H = iBfPhaseG0.ucG0_M_T0_H;
				piBfPhaseG0->ucG0_M_T1_H = iBfPhaseG0.ucG0_M_T1_H;
				piBfPhaseG0->ucG0_R0_UH  = iBfPhaseG0.ucG0_R0_UH;
				piBfPhaseG0->ucG0_R0_H   = iBfPhaseG0.ucG0_R0_H;
				piBfPhaseG0->ucG0_R0_M   = iBfPhaseG0.ucG0_R0_M;
				piBfPhaseG0->ucG0_R0_L   = iBfPhaseG0.ucG0_R0_L;
				piBfPhaseG0->ucG0_R1_UH  = iBfPhaseG0.ucG0_R1_UH;
				piBfPhaseG0->ucG0_R1_H   = iBfPhaseG0.ucG0_R1_H;
				piBfPhaseG0->ucG0_R1_M   = iBfPhaseG0.ucG0_R1_M;
				piBfPhaseG0->ucG0_R1_L   = iBfPhaseG0.ucG0_R1_L;
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G0 and Group_M\n G0_M_T0_H = %d\n G0_M_T1_H = %d\n",
						 iBfPhaseG0.ucG0_M_T0_H,
						 iBfPhaseG0.ucG0_M_T1_H));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G0_R0_UH = %d\n G0_R0_H = %d\n G0_R0_M = %d\n G0_R0_L = %d\n G0_R1_UH = %d\n G0_R1_H = %d\n G0_R1_M = %d\n G0_R1_L = %d\n",
						 iBfPhaseG0.ucG0_R0_UH,
						 iBfPhaseG0.ucG0_R0_H,
						 iBfPhaseG0.ucG0_R0_M,
						 iBfPhaseG0.ucG0_R0_L,
						 iBfPhaseG0.ucG0_R1_UH,
						 iBfPhaseG0.ucG0_R1_H,
						 iBfPhaseG0.ucG0_R1_M,
						 iBfPhaseG0.ucG0_R1_L));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Group : %d\n", 0));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibration == 1? or Verification == 2? : %d\n", ucPhaseCalType));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibrated result = %d\n", ucStatus));

				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("0 : Means failed\n 1: means pass\n 2: means on-going\n"));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("C0_H : %d, C1_H : %d, C2_H : %d\n C0_M : %d, C1_M : %d, C2_M : %d\n C0_L : %d, C1_L : %d, C2_L : %d\n C3_M : %d, C3_L : %d\n",
						 iBfPhaseOut.ucC0_H, iBfPhaseOut.ucC1_H, iBfPhaseOut.ucC2_H,
						 iBfPhaseOut.ucC0_M, iBfPhaseOut.ucC1_M, iBfPhaseOut.ucC2_M,
						 iBfPhaseOut.ucC0_L, iBfPhaseOut.ucC1_L, iBfPhaseOut.ucC2_L,
						 iBfPhaseOut.ucC3_M, iBfPhaseOut.ucC3_L));
				break;
			case GROUP_H:
				break;
			}
		} else {
			ucGroupIdx = ucGroup - 1;
			NdisCopyMemory(&iBfPhaseGx, pucIBfPhaseG, u1IbfCalPhaseStructLenGx);

#if defined(MT7663)
			piBfPhaseGx = (P_MT7663_IBF_PHASE_Gx_T)&pAd->piBfPhaseGx[ucGroupIdx * u1IbfCalPhaseStructLenGx];
#endif

			switch (ucGroupL_M_H) {
			case GROUP_L:
				break;

			case GROUP_M:
				piBfPhaseGx->ucGx_M_T0_H = iBfPhaseGx.ucGx_M_T0_H;
				piBfPhaseGx->ucGx_M_T1_H = iBfPhaseGx.ucGx_M_T1_H;
				piBfPhaseGx->ucGx_R0_UH  = iBfPhaseGx.ucGx_R0_UH;
				piBfPhaseGx->ucGx_R0_H   = iBfPhaseGx.ucGx_R0_H;
				piBfPhaseGx->ucGx_R0_M   = iBfPhaseGx.ucGx_R0_M;
				piBfPhaseGx->ucGx_R0_L   = iBfPhaseGx.ucGx_R0_L;
				piBfPhaseGx->ucGx_R1_UH  = iBfPhaseGx.ucGx_R1_UH;
				piBfPhaseGx->ucGx_R1_H   = iBfPhaseGx.ucGx_R1_H;
				piBfPhaseGx->ucGx_R1_M   = iBfPhaseGx.ucGx_R1_M;
				piBfPhaseGx->ucGx_R1_L   = iBfPhaseGx.ucGx_R1_L;
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G%d and Group_M\n G%d_M_T0_H = %d\n G%d_M_T1_H = %d\n",
						 ucGroup,
						 ucGroup, iBfPhaseGx.ucGx_M_T0_H,
						 ucGroup, iBfPhaseGx.ucGx_M_T1_H));

				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("G%d_R0_UH = %d\n G%d_R0_H = %d\n G%d_R0_M = %d\n G%d_R0_L = %d\n G%d_R1_UH = %d\n G%d_R1_H = %d\n G%d_R1_M = %d\n G%d_R1_L = %d\n",
						 ucGroup, iBfPhaseGx.ucGx_R0_UH,
						 ucGroup, iBfPhaseGx.ucGx_R0_H,
						 ucGroup, iBfPhaseGx.ucGx_R0_M,
						 ucGroup, iBfPhaseGx.ucGx_R0_L,
						 ucGroup, iBfPhaseGx.ucGx_R1_UH,
						 ucGroup, iBfPhaseGx.ucGx_R1_H,
						 ucGroup, iBfPhaseGx.ucGx_R1_M,
						 ucGroup, iBfPhaseGx.ucGx_R1_L));
				/*
				update status and value of ibf calibration to procfs.
				changed by wanghe
				*/
				ibf_status = ucStatus;
				ibf_value[0] = iBfPhaseGx.ucGx_M_T0_H;
				ibf_value[1] = iBfPhaseGx.ucGx_M_T1_H;
				ibf_value[2] = iBfPhaseGx.ucGx_R0_UH;
				ibf_value[3] = iBfPhaseGx.ucGx_R0_H;
				ibf_value[4] = iBfPhaseGx.ucGx_R0_M;
				ibf_value[5] = iBfPhaseGx.ucGx_R0_L;
				ibf_value[6] = iBfPhaseGx.ucGx_R1_UH;
				ibf_value[7] = iBfPhaseGx.ucGx_R1_H;
				ibf_value[8] = iBfPhaseGx.ucGx_R1_M;
				ibf_value[9] = iBfPhaseGx.ucGx_R1_L;

				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Group : %d\n", ucGroup));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibration == 1? or Verification == 2? : %d\n", ucPhaseCalType));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibrated result = %d\n", ucStatus));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("0 : Means failed\n 1: means pass\n 2: means on-going\n"));
				MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("C0_H : %d, C1_H : %d, C2_H : %d\n C0_M : %d, C1_M : %d, C2_M : %d\n C0_L : %d, C1_L : %d, C2_L : %d\n C3_M : %d, C3_L : %d\n",
						 iBfPhaseOut.ucC0_H, iBfPhaseOut.ucC1_H, iBfPhaseOut.ucC2_H,
						 iBfPhaseOut.ucC0_M, iBfPhaseOut.ucC1_M, iBfPhaseOut.ucC2_M,
						 iBfPhaseOut.ucC0_L, iBfPhaseOut.ucC1_L, iBfPhaseOut.ucC2_L,
						 iBfPhaseOut.ucC3_M, iBfPhaseOut.ucC3_L));
				break;

			case GROUP_H:
				break;
			}
		}
	break;

	case IBF_PHASE_CAL_VERIFY: /* Show calibrated result only */
	case IBF_PHASE_CAL_VERIFY_INSTRUMENT:
		NdisCopyMemory(&iBfPhaseOut, pBuf, u1IbfPhaseOutStructLen);

		/* Update calibrated status */
		pAd->fgCalibrationFail |= ((ucStatus == 1) ? FALSE : TRUE);
		pAd->fgGroupIdPassFailStatus[ucGroup] = ((ucStatus == 1) ? FALSE : TRUE);
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Group : %d\n", ucGroup));
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibration == 1? or Verification == 2? : %d\n", ucPhaseCalType));
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Calibrated result = %d\n", ucStatus));
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("0 : Means failed\n 1: means pass\n 2: means on-going\n"));
		MTWF_LOG(DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("C0_H : %d, C1_H : %d, C2_H : %d\n C0_M : %d, C1_M : %d, C2_M : %d\n C0_L : %d, C1_L : %d, C2_L : %d\n C3_M : %d, C3_L : %d\n",
				 iBfPhaseOut.ucC0_H, iBfPhaseOut.ucC1_H, iBfPhaseOut.ucC2_H,
				 iBfPhaseOut.ucC0_M, iBfPhaseOut.ucC1_M, iBfPhaseOut.ucC2_M,
				 iBfPhaseOut.ucC0_L, iBfPhaseOut.ucC1_L, iBfPhaseOut.ucC2_L,
				 iBfPhaseOut.ucC3_M, iBfPhaseOut.ucC3_L));
		//write7613procfs(ucStatus);/*for test*/
		break;

	default:
		break;
	}
}
#endif /* MT7663 */



#endif /* TXBF_SUPPORT */
