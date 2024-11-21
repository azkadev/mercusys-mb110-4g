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
	mt_phy.c
*/


#include "rt_config.h"
static INT32 MTBbpInit(RTMP_ADAPTER *pAd)
{
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): Init BBP Registers\n", __FUNCTION__));

	if (pAd->chipOps.AsicBbpInit != NULL)
		pAd->chipOps.AsicBbpInit(pAd);

	return NDIS_STATUS_SUCCESS;
}


INT32 MTShowPartialBBP(RTMP_ADAPTER *pAd, UINT32 Start, UINT32 End)
{
	UINT32 Offset, Value;

	for (Offset = Start; Offset <= End; Offset += 4)
	{
		RTMP_IO_READ32(pAd, Offset, &Value);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s():0x%04x 0x%08x\n", __FUNCTION__, Offset, Value));
	}

	return TRUE;
}


INT32 MTShowAllBBP(RTMP_ADAPTER *pAd)
{
	UINT32 Offset, Value;

	for (Offset = 0x10000; Offset <= 0x20000; Offset += 4)
	{
		RTMP_IO_READ32(pAd, Offset, &Value);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s():0x%04x 0x%08x\n", __FUNCTION__, Offset, Value));
	}

	return TRUE;
}


static INT mt_bbp_set_bw(struct _RTMP_ADAPTER *pAd, UINT8 bw)
{
	INT ret = FALSE;

	pAd->CommonCfg.BBPCurrentBW = bw;

	if ((bw == BW_20) || (bw == BW_40))
	{
		pAd->CommonCfg.BBPCurrentBW = bw;


		ret = TRUE;
	}
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("unknow bw setting = %d\n", bw));
		ret = FALSE;
	}

	return ret;
}

#ifdef SMART_CARRIER_SENSE_SUPPORT
INT MTSmartCarrierSense(RTMP_ADAPTER *pAd)
{
	UINT16 RxRatio;
	UINT32 totaltxrx;
	CHAR l2h = pAd->SCSCtrl.SCSL2H;
	CHAR h2l = pAd->SCSCtrl.SCSH2L;
	BOOLEAN txrx, ratio;

	if (l2h >= 0 || h2l >= 0 || h2l <= l2h)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): invalid RSSI threshhold H_to_L=%d L_to_H=%d\n", __FUNCTION__, h2l, l2h));
		return FALSE;
	}

	totaltxrx = pAd->RalinkCounters.OneSecReceivedByteCount + pAd->RalinkCounters.OneSecTransmittedByteCount;
	RxRatio   = totaltxrx != 0 ? ((pAd->RalinkCounters.OneSecReceivedByteCount) * 100 / totaltxrx) : 0;
	txrx      = pAd->SCSCtrl.SCSTxrx ? totaltxrx > pAd->SCSCtrl.SCSThreshold : TRUE;
	ratio     = pAd->SCSCtrl.SCSTxrx ? RxRatio < 90 : TRUE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():BssNr=%d, SCSRssi=%d, EDCCA=%d RxRatio=%d SCSStatus=%d "
	                                                           "H_to_L=%d L_to_H=%d mode=%s SCSTxrx=%d txrx=%d ratio=%d\n", __FUNCTION__,
		     pAd->SCSCtrl.SCSBssTab.BssNr, pAd->SCSCtrl.SCSMinRssi, pAd->SCSCtrl.EDCCA_Status, RxRatio, pAd->SCSCtrl.SCSStatus,
		     h2l, l2h, pAd->SCSCtrl.SCSMode?"max":"min", pAd->SCSCtrl.SCSTxrx, txrx, ratio));
	
	if (txrx && ((pAd->SCSCtrl.SCSBssTab.BssNr > 5) || (pAd->SCSCtrl.SCSStatus != SCS_STATUS_DEFAULT)) && (pAd->MacTab.Size > 0))
	{
#if 0
		/* Check Mini RSSI for dynamic adjust gain */
		if ((pAd->SCSCtrl.SCSMinRssi < 0) && (pAd ->SCSCtrl.SCSMinRssi > -57) && (pAd->SCSCtrl.EDCCA_Status == 0) && 
			(RxRatio < 90))
		{
			if (pAd->SCSCtrl.SCSStatus != SCS_STATUS_ULTRA_LOW)
			{	
				/* CR */
				RTMP_IO_WRITE32(pAd, CR_AGC_0, 0x7FF6666F);
				RTMP_IO_WRITE32(pAd, CR_AGC_0_RX1, 0x7FF6666F);
				RTMP_IO_WRITE32(pAd, CR_AGC_3, 0x818181E3);
				RTMP_IO_WRITE32(pAd, CR_AGC_3_RX1, 0x818181E3);
				pAd->SCSCtrl.SCSStatus = SCS_STATUS_ULTRA_LOW;
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): CSC=UL\n", __FUNCTION__));
			}			
		}
		else
#endif
		if (ratio && (pAd ->SCSCtrl.SCSMinRssi < 0) && (pAd ->SCSCtrl.SCSMinRssi > h2l))
		{
			if (pAd->SCSCtrl.SCSStatus != SCS_STATUS_LOW)
			{
				/* CR */
				RTMP_IO_WRITE32(pAd, CR_AGC_0, 0x6AF7776F);
				RTMP_IO_WRITE32(pAd, CR_AGC_0_RX1, 0x6AF7776F);
				RTMP_IO_WRITE32(pAd, CR_AGC_3, 0x8181D5E3);
				RTMP_IO_WRITE32(pAd, CR_AGC_3_RX1, 0x8181D5E3);
				pAd->SCSCtrl.SCSStatus = SCS_STATUS_LOW;
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("\n\n\n%s(): CSC=L SCSRssi=%d > %d\n\n\n", __FUNCTION__, pAd ->SCSCtrl.SCSMinRssi, h2l));
			}
		}
		else if (pAd ->SCSCtrl.SCSMinRssi < l2h)
		{
			if (pAd->SCSCtrl.SCSStatus != SCS_STATUS_DEFAULT)
			{
				/* CR */
				RTMP_IO_WRITE32(pAd, CR_AGC_0, pAd->SCSCtrl.CR_AGC_0_default);
				RTMP_IO_WRITE32(pAd, CR_AGC_0_RX1, pAd->SCSCtrl.CR_AGC_0_default);
				RTMP_IO_WRITE32(pAd, CR_AGC_3, pAd->SCSCtrl.CR_AGC_3_default);
				RTMP_IO_WRITE32(pAd, CR_AGC_3_RX1, pAd->SCSCtrl.CR_AGC_3_default);
				pAd->SCSCtrl.SCSStatus = SCS_STATUS_DEFAULT;
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("\n\n\n%s(): CSC=H (Default)  SCSRssi=%d < %d\n\n\n", __FUNCTION__, pAd ->SCSCtrl.SCSMinRssi, l2h));
			}
		}
	}
	else
	{
		if (pAd->SCSCtrl.SCSStatus != SCS_STATUS_DEFAULT)
		{
			RTMP_IO_WRITE32(pAd, CR_AGC_0, pAd->SCSCtrl.CR_AGC_0_default);
			RTMP_IO_WRITE32(pAd, CR_AGC_0_RX1, pAd->SCSCtrl.CR_AGC_0_default);
			RTMP_IO_WRITE32(pAd, CR_AGC_3, pAd->SCSCtrl.CR_AGC_3_default);
			RTMP_IO_WRITE32(pAd, CR_AGC_3_RX1, pAd->SCSCtrl.CR_AGC_3_default);
			pAd->SCSCtrl.SCSStatus = SCS_STATUS_DEFAULT;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("\n\n\n%s(): CSC=H (Default)  \n\n\n", __FUNCTION__));
		}
	}

	return TRUE;
}
#endif /* SMART_CARRIER_SENSE_SUPPORT */

static struct phy_ops mt_phy_ops = {
	.bbp_init = MTBbpInit,
	.bbp_set_bw = mt_bbp_set_bw,
	.ShowPartialBBP = MTShowPartialBBP,
	.ShowAllBBP = MTShowAllBBP,
	.ShowPartialRF = MTShowPartialRF,
	.ShowAllRF = MTShowAllRF,
#ifdef SMART_CARRIER_SENSE_SUPPORT
	.Smart_Carrier_Sense = MTSmartCarrierSense,
#endif /* SMART_CARRIER_SENSE_SUPPORT */
};


INT mt_phy_probe(RTMP_ADAPTER *pAd)
{
	pAd->phy_op = &mt_phy_ops;

	return TRUE;
}
