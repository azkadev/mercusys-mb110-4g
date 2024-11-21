/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

/****************************************************************************
	Abstract:

***************************************************************************/

#ifdef DOT11K_RRM_SUPPORT

#include "rt_config.h"
#ifdef TP_NETWORK_ROAMING
#include "band_steering_api.h"
#include "ap_band_steering.h"
#endif

/*
static CHAR ZeroSsid[32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
*/


static VOID RRM_QuietOffsetTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3);

static VOID RRM_QuietTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3);

DECLARE_TIMER_FUNCTION(RRM_QuietOffsetTimeout);
DECLARE_TIMER_FUNCTION(RRM_QuietTimeout);

BUILD_TIMER_FUNCTION(RRM_QuietOffsetTimeout);
BUILD_TIMER_FUNCTION(RRM_QuietTimeout);


static VOID RRM_QuietOffsetTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	INT idx;
	PRRM_CONFIG pRrmCfg = NULL;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;
	PRALINK_TIMER_STRUCT pTimer = (PRALINK_TIMER_STRUCT) SystemSpecific3;

	for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++)
	{
		pRrmCfg = &pAd->ApCfg.MBSSID[idx].RrmCfg;
		if (&pRrmCfg->QuietCB.QuietOffsetTimer == pTimer)
			break;
	}

	/* start Quiet . */
	if (idx == pAd->ApCfg.BssidNum)
		return;

	pRrmCfg = &pAd->ApCfg.MBSSID[idx].RrmCfg;
	pRrmCfg->QuietCB.QuietState = RRM_QUIET_SILENT;
	RTMPSetTimer(&pRrmCfg->QuietCB.QuietTimer,
		pRrmCfg->QuietCB.QuietDuration);

	return;
}

static VOID RRM_QuietTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	INT idx;
	PRRM_CONFIG pRrmCfg = NULL;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;
	PRALINK_TIMER_STRUCT pTimer = (PRALINK_TIMER_STRUCT) SystemSpecific3;

	for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++)
	{
		pRrmCfg = &pAd->ApCfg.MBSSID[idx].RrmCfg;
		if (&pRrmCfg->QuietCB.QuietTimer == pTimer)
			break;
	}

	/* stop Quiet . */
	if (idx == pAd->ApCfg.BssidNum)
		return;

	pRrmCfg = &pAd->ApCfg.MBSSID[idx].RrmCfg;
	pRrmCfg->QuietCB.QuietState = RRM_QUIET_IDLE;

	return;
}

void RRM_ReadParametersFromFile(
	IN PRTMP_ADAPTER pAd,
	RTMP_STRING *tmpbuf,
	RTMP_STRING *buffer)
{
	INT loop;
	RTMP_STRING *macptr;

	/* RRMEnable */
	if (RTMPGetKeyParameter("RRMEnable", tmpbuf, 255, buffer, TRUE))
	{
		for (loop=0, macptr = rstrtok(tmpbuf,";");
				(macptr && loop < MAX_MBSSID_NUM(pAd));
					macptr = rstrtok(NULL,";"), loop++)
		{
			LONG Enable;
			Enable = simple_strtol(macptr, 0, 10);
			pAd->ApCfg.MBSSID[loop].RrmCfg.bDot11kRRMEnable =
				(Enable > 0) ? TRUE : FALSE;
			pAd->ApCfg.MBSSID[loop].RrmCfg.bDot11kRRMEnableSet = TRUE;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s::(bDot11kRRMEnable[%d]=%d)\n",
				__FUNCTION__, loop,
				pAd->ApCfg.MBSSID[loop].RrmCfg.bDot11kRRMEnable));
		}
	}
	else 
	{
		for (loop = 0; loop < MAX_MBSSID_NUM(pAd); loop++)
			pAd->ApCfg.MBSSID[loop].RrmCfg.bDot11kRRMEnable = FALSE;
	}

	/* Regulatory Class */
	if (RTMPGetKeyParameter("RegulatoryClass", tmpbuf, 255, buffer, TRUE))
	{	
		LONG Value;
		for (loop=0, macptr = rstrtok(tmpbuf,";");
				(macptr && loop < MAX_NUM_OF_REGULATORY_CLASS);
					macptr = rstrtok(NULL,";"), loop++)
		{
			Value = simple_strtol(macptr, 0, 10);
			pAd->CommonCfg.RegulatoryClass[loop] = Value;
		}
		
	}

	return;
}


INT Set_Dot11kRRM_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex = pObj->ioctl_if;
	ULONG Value;

	if (ifIndex >= pAd->ApCfg.BssidNum)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Unknow If index (%d)", ifIndex));
		return -1;
	}

	Value = (UINT) simple_strtol(arg, 0, 10);
	pAd->ApCfg.MBSSID[ifIndex].RrmCfg.bDot11kRRMEnable =
		(BOOLEAN)(Value) == 0 ? FALSE : TRUE;

	if(pAd->ApCfg.MBSSID[ifIndex].RrmCfg.bDot11kRRMEnable)
		pAd->ApCfg.MBSSID[ifIndex].CapabilityInfo |= RRM_CAP_BIT;
	else
		pAd->ApCfg.MBSSID[ifIndex].CapabilityInfo &= ~RRM_CAP_BIT;
	APMakeBssBeacon(pAd,ifIndex);

	return 1;
}

INT Set_BeaconReq_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT Loop;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex = pObj->ioctl_if;
	UINT Aid = 1;
	UINT ArgIdx;
	RTMP_STRING *thisChar;

	RRM_MLME_BCN_REQ_INFO BcnReq;

	ArgIdx = 0;
	NdisZeroMemory(&BcnReq, sizeof(RRM_MLME_BCN_REQ_INFO));

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		switch(ArgIdx)
		{
			case 0:	/* Aid. */
				Aid = (UINT8) simple_strtol(thisChar, 0, 16);
				if (!VALID_WCID(Aid))
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: unknow sta of Aid(%d)\n", __FUNCTION__, Aid));
					return TRUE;
				}
				break;

			case 1: /* Meausre Duration. */
				BcnReq.MeasureDuration = (UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 2: /* Regulator Class */
				BcnReq.RegulatoryClass = (UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 3: /* BSSID */
				if(strlen(thisChar) != 17)
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						("%s: invalid value BSSID.\n", 	__FUNCTION__));
					return TRUE;
				}

				if(strlen(thisChar) == 17)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 */
				{
					RTMP_STRING *value;
					for (Loop=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
					{
						if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
							return FALSE;  /*Invalid */

						AtoH(value, &BcnReq.Bssid[Loop++], 1);
					}

					if(Loop != 6)
						return TRUE;
				}
				break;

			case 4: /* SSID */
				BcnReq.pSsid = (PUINT8)thisChar;
				BcnReq.SsidLen = strlen(thisChar);
				break;

			case 5: /* measure channel */
				BcnReq.MeasureCh = (UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 6: /* measure mode. */
				BcnReq.MeasureMode = (UINT8) simple_strtol(thisChar, 0, 10);
				if (BcnReq.MeasureMode > RRM_BCN_REQ_MODE_BCNTAB)
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						("%s: invalid Measure Mode. %d\n", 	__FUNCTION__, BcnReq.MeasureMode));
					return TRUE;
				}
				break;
			case 7: /* regulatory class. */
				{
					RTMP_STRING *RegClassString;
					int RegClassIdx;

					RegClassIdx = 0;
					while ((RegClassString = strsep((char **)&thisChar, "+")) != NULL)
					{
						BcnReq.ChRepRegulatoryClass[RegClassIdx] =
							(UINT8) simple_strtol(RegClassString, 0, 10);
						RegClassIdx++;
					}
				}
				break;
			default:
				break;
			
		}
		ArgIdx++;
	}	

	if (ArgIdx < 7 || ArgIdx > 8)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid args (%d).\n", __FUNCTION__, ArgIdx));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("eg: iwpriv ra0 set BcnReq=<Aid>-<Duration>-<RegulatoryClass>-<BSSID>-<SSID>-<MeasureCh>-<MeasureMode>-<ChRegClass>\n"));
		return TRUE;
	}


	BcnReq.BcnReqCapFlag.field.ReportCondition = TRUE;
	if (BcnReq.MeasureCh == 255)
		BcnReq.BcnReqCapFlag.field.ChannelRep = TRUE;
	else
		BcnReq.BcnReqCapFlag.field.ChannelRep = FALSE;

	RRM_EnqueueBcnReq(pAd, Aid, ifIndex, &BcnReq);

	return TRUE;
}

INT Set_LinkMeasureReq_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex = pObj->ioctl_if;
	UINT Aid = 1;

	Aid = simple_strtol(arg, 0, 10);
	RRM_EnqueueLinkMeasureReq(pAd, Aid, ifIndex);

	return TRUE;
}

INT Set_TxStreamMeasureReq_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex = pObj->ioctl_if;
	UINT Aid = 1;
	UINT ArgIdx;
	RTMP_STRING *thisChar;

	RRM_MLME_TRANSMIT_REQ_INFO TransmitReq;
#ifdef DBG
	PMAC_TABLE_ENTRY pMacEntry;
#endif
	ArgIdx = 0;
	NdisZeroMemory(&TransmitReq, sizeof(RRM_MLME_TRANSMIT_REQ_INFO));

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		switch(ArgIdx)
		{
			case 0:	/* Aid. */
				Aid = (UINT8) simple_strtol(thisChar, 0, 10);
				if (!VALID_WCID(Aid))
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: unknow sta of Aid(%d)\n", __FUNCTION__, Aid));
					return TRUE;
				}
				break;

			case 1: /* DurationMandotory. */
				TransmitReq.bDurationMandatory =
					((UINT16)simple_strtol(thisChar, 0, 10) > 0 ? TRUE : FALSE);
				break;

			case 2: /* Measure Duration */
				TransmitReq.MeasureDuration = (UINT16)simple_strtol(thisChar, 0, 10);
				break;

			case 3: /* TID */
				TransmitReq.Tid = (UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 4: /* Bin 0 Range */
				TransmitReq.BinRange = (UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 5: /* Averange Condition */
				TransmitReq.ArvCondition =
					((UINT8) simple_strtol(thisChar, 0, 10)) > 0 ? 1 : 0;
				break;

			case 6: /* Consecutive Condition */
				TransmitReq.ConsecutiveCondition =
					((UINT8) simple_strtol(thisChar, 0, 10)) > 0 ? 1 : 0;
				break;

			case 7: /* Delay Condition */
				TransmitReq.DelayCondition =
					((UINT8) simple_strtol(thisChar, 0, 10)) > 0 ? 1 : 0;
				break;

			case 8: /* Averange Error Threshold */
				TransmitReq.AvrErrorThreshold =
					(UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 9: /* Consecutive Error Threshold */
				TransmitReq.ConsecutiveErrorThreshold =
					(UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 10: /* Delay Threshold */
				TransmitReq.DelayThreshold =
					(UINT8) simple_strtol(thisChar, 0, 10);
				break;

			case 11: /* Measure counter  */
				TransmitReq.MeasureCnt =
					(UINT8) simple_strtol(thisChar, 0, 10);

				break;

			case 12: /* Trigger time out */
				TransmitReq.TriggerTimeout =
					(UINT8) simple_strtol(thisChar, 0, 10);
				break;
			default:
				break;
			
		}
		ArgIdx++;
	}	

	if ((ArgIdx != 13) && (ArgIdx != 5))
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s: invalid args (%d).\n", __FUNCTION__, ArgIdx));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("eg: iwpriv ra0 set txreq=<Aid>-<DurationMandortory>-<Duration>-<TID>-<BinRange>[-<AvrCond>-<ConsecutiveCond>-<DealyCond>-<AvrErrorThreshold>-<ConsecutiveErrorThreshold>-<DelayThreshold>-<MeasureCnt>-<TriggerTimeout>]\n"));
		return TRUE;
	}

	if (ArgIdx == 5)
		TransmitReq.bTriggerReport = 0;
	else
		TransmitReq.bTriggerReport = 1;

#ifdef DBG
	pMacEntry = &pAd->MacTab.Content[Aid];
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s::Aid=%d, PeerMac=%02x:%02x:%02x:%02x:%02x:%02x\n",
		__FUNCTION__, Aid,	pMacEntry->Addr[0], pMacEntry->Addr[1],
		pMacEntry->Addr[2], pMacEntry->Addr[3], pMacEntry->Addr[4], pMacEntry->Addr[5]));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Duration=%d, Tid=%d, Bin 0 Range=%d\n",
		TransmitReq.MeasureDuration, TransmitReq.Tid, TransmitReq.BinRange));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("ArvCondition=%d, ConsecutiveCondition=%d, DelayCondition=%d\n",
		TransmitReq.ArvCondition, TransmitReq.ConsecutiveCondition, TransmitReq.DelayCondition));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("AvrErrorThreshold=%d, ConsecutiveErrorThreshold=%d\n",
		TransmitReq.AvrErrorThreshold, TransmitReq.ConsecutiveErrorThreshold));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("DelayThreshold=%d\n", TransmitReq.DelayThreshold));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("MeasureCnt=%d, TriggerTimeout=%d\n",
		TransmitReq.MeasureCnt, TransmitReq.TriggerTimeout));
#endif

	RRM_EnqueueTxStreamMeasureReq(pAd, Aid, ifIndex, &TransmitReq);

	return TRUE;
}

INT Set_RRM_Selftest_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UINT Cmd = 1;

	PMAC_TABLE_ENTRY pEntry;
	UCHAR StaAddr[MAC_ADDR_LEN] = {0x00,0x0c,0x43,0x00,0x00,0x00};	
	struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[0/*pEntry->func_tb_idx*/].wdev;

	Cmd = simple_strtol(arg, 0, 10);

	switch(Cmd)
	{
		case 1: /* insert a STA for RRM Beacon Report request testing. */
			pEntry = MacTableInsertEntry(pAd, StaAddr, wdev, ENTRY_CLIENT, OPMODE_AP, TRUE);
			pEntry->Sst = SST_ASSOC;
			pEntry->CapabilityInfo |= RRM_CAP_BIT;
			pEntry->RrmEnCap.field.BeaconActiveMeasureCap = 1;
			break;
	}
	return TRUE;
}

INT RRM_InfoDisplay_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT loop;

	for (loop = 0; loop < pAd->ApCfg.BssidNum; loop++)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%d: bDot11kRRMEnable=%d\n",
			loop, pAd->ApCfg.MBSSID[loop].RrmCfg.bDot11kRRMEnable));
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Country Code=%s\n",
		pAd->CommonCfg.CountryCode));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Power Constraint=%d\n",
		pAd->CommonCfg.PwrConstraint));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Regulator Class="));
	for (loop = 0; loop < MAX_NUM_OF_REGULATORY_CLASS; loop++)
	{
		if (pAd->CommonCfg.RegulatoryClass[loop] == 0)
			break;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%d ",
			pAd->CommonCfg.RegulatoryClass[loop]));
	}
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("\n"));	

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("Regulator TxPowerPercentage=%ld\n",
		pAd->CommonCfg.TxPowerPercentage));
	return TRUE;
}

VOID RRM_CfgInit(
	IN PRTMP_ADAPTER pAd)
{
	INT loop;
	PRRM_CONFIG pRrmCfg;

	for (loop = 0; loop < MAX_MBSSID_NUM(pAd); loop++)
	{
		pRrmCfg = &pAd->ApCfg.MBSSID[loop].RrmCfg;
		pRrmCfg->QuietCB.QuietPeriod = RRM_DEFAULT_QUIET_PERIOD;
		pRrmCfg->QuietCB.QuietDuration = RRM_DEFAULT_QUIET_DURATION;
		pRrmCfg->QuietCB.QuietOffset = RRM_DEFAULT_QUIET_OFFSET;

		RTMPInitTimer(pAd, &pRrmCfg->QuietCB.QuietOffsetTimer, GET_TIMER_FUNCTION(RRM_QuietOffsetTimeout), pAd, FALSE);
		RTMPInitTimer(pAd, &pRrmCfg->QuietCB.QuietTimer, GET_TIMER_FUNCTION(RRM_QuietTimeout), pAd, FALSE);

		pRrmCfg->QuietCB.QuietState = RRM_QUIET_IDLE;
		pRrmCfg->QuietCB.CurAid = 1;
		if (pRrmCfg->bDot11kRRMEnableSet == FALSE)
			pRrmCfg->bDot11kRRMEnable = FALSE; //set to default off
		pRrmCfg->bDot11kRRMNeighborRepTSFEnable = FALSE;
	}

	return;
}

VOID RRM_QuietUpdata(
	IN PRTMP_ADAPTER pAd)
{
	INT loop;
	PRRM_CONFIG pRrmCfg;
	BSS_STRUCT *mbss;
	struct wifi_dev *wdev;

	for (loop = 0; loop < pAd->ApCfg.BssidNum; loop++)
	{
		mbss = &pAd->ApCfg.MBSSID[loop];
		wdev = &mbss->wdev;
	
		if ((wdev->if_dev == NULL) || ((wdev->if_dev != NULL) &&
			!(RTMP_OS_NETDEV_STATE_RUNNING(wdev->if_dev))))
		{
			/* the interface is down */
			continue;
		}

		pRrmCfg = &pAd->ApCfg.MBSSID[loop].RrmCfg;

		if (pRrmCfg->QuietCB.QuietCnt == pRrmCfg->QuietCB.QuietPeriod)
		{
			/* issue Bcn Report Request to STAs supported RRM-Bcn Report. */
			/* Voice-enterpise doesn't require it in testing-event3 so remove it. */
			/*RRM_BcnReortQuery(pAd, loop, pRrmCfg); */
		}

		if ((pRrmCfg->QuietCB.QuietCnt == 0)
			&& (pRrmCfg->QuietCB.QuietState == RRM_QUIET_IDLE))
		{
			if (pRrmCfg->QuietCB.QuietOffset != 0)
			{
				/* Start QuietOffsetTimer. */
				RTMPSetTimer(&pRrmCfg->QuietCB.QuietOffsetTimer,
					pRrmCfg->QuietCB.QuietOffset);
			}
			else
			{
				/* Start Quiet Timer. */
				pRrmCfg->QuietCB.QuietState = RRM_QUIET_SILENT;

				RTMPSetTimer(&pRrmCfg->QuietCB.QuietTimer,
					pRrmCfg->QuietCB.QuietDuration);
			}
		}

		RRM_QUIET_CNT_DEC(pRrmCfg->QuietCB.QuietCnt, pRrmCfg->QuietCB.QuietPeriod);
	}
	return;
}

VOID RRM_PeerNeighborReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	PFRAME_802_11 pFr = (PFRAME_802_11)Elem->Msg;
	PUCHAR pFramePtr = pFr->Octet;
	PMAC_TABLE_ENTRY pEntry;
	UINT8 DialogToken;
	PCHAR pSsid = NULL;
	UINT8 SsidLen = 0;	
	CHAR ssidbuf[MAX_LEN_OF_SSID+1];

	PMEASURE_REQ_ENTRY pMeasure_Entry = NULL;
	PNET_DEV NetDev = NULL;
	UINT32 FrameLen = 0;
	UINT8 NRReqToken = 0, APIndex = 0;

	KV_PRINT(("%s::\n", __FUNCTION__));

	pEntry = MacTableLookup(pAd, pFr->Hdr.Addr2);
	if (!pEntry || (pEntry->Sst != SST_ASSOC))
		return;

	/*whether the up-layer or driver handle the request*/
	if (pAd->ApCfg.HandleNRReqbyUplayer) {
		/*send event to the up-layer */
		for (APIndex = 0; APIndex < MAX_MBSSID_NUM(pAd); APIndex++) {
			if (MAC_ADDR_EQUAL(pFr->Hdr.Addr3, pAd->ApCfg.MBSSID[APIndex].wdev.bssid)) {
				NetDev = pAd->ApCfg.MBSSID[APIndex].wdev.if_dev;
				break;
			}
		}
		if (NetDev == NULL) {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("no valid NetDev! BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
				PRINT_MAC(pFr->Hdr.Addr3)));
			return;
		}
	/* skip Category and action code. */
	pFramePtr += 2;
		NdisMoveMemory(&NRReqToken, pFramePtr, 1);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: NRReqToken = %d\n",
			__func__, NRReqToken));
		pMeasure_Entry = MeasureReqInsert(pAd, NRReqToken);
		RTMP_SEM_LOCK(&pAd->CommonCfg.MeasureReqTabLock);
		pMeasure_Entry->skip_time_check = TRUE;
		pMeasure_Entry->CurrentState = WAIT_NR_RSP;
		pMeasure_Entry->Priv = pAd;
		pMeasure_Entry->ControlIndex = APIndex;
		memcpy(pMeasure_Entry->StaMac, pEntry->Addr, MAC_ADDR_LEN);
		RTMPInitTimer(pAd, &pMeasure_Entry->WaitNRRspTimer,
				GET_TIMER_FUNCTION(WaitNRRspTimeout), pMeasure_Entry, FALSE);
		RTMP_SEM_UNLOCK(&pAd->CommonCfg.MeasureReqTabLock);

		FrameLen = Elem->MsgLen - sizeof(HEADER_802_11) - 2;
		wext_send_nr_req_event(NetDev, pFr->Hdr.Addr2, pFramePtr, FrameLen);

		RTMPSetTimer(&pMeasure_Entry->WaitNRRspTimer, NR_RSP_TIMEOUT_VALUE);
		} else {

			if (RRM_PeerNeighborReqSanity(pAd, Elem->Msg, Elem->MsgLen, &DialogToken, &pSsid, &SsidLen)) {
		KV_PRINT(("DialogToken=%x\n", DialogToken));
		snprintf(ssidbuf, sizeof(ssidbuf),"%s", pSsid);
		ssidbuf[SsidLen]='\0';
		KV_PRINT(("pSsid=%s\n", /*pSsid*/ssidbuf));		
		KV_PRINT(("SsidLen=%d\n", SsidLen));
		RRM_EnqueueNeighborRep(pAd, pEntry, DialogToken, pSsid, SsidLen);
	}
		}
	return;
}

VOID RRM_BeaconReportHandler(
	IN PRTMP_ADAPTER pAd,
	IN PRRM_BEACON_REP_INFO pBcnRepInfo,
	IN LONG Length,
	IN PMEASURE_REQ_ENTRY pDialogEntry)
{
	CHAR Rssi;
	USHORT LenVIE = 0;
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	UCHAR VarIE[MAX_VIE_LEN];
	ULONG Idx = BSS_NOT_FOUND;
	LONG RemainLen = Length;
	PRRM_BEACON_REP_INFO pBcnRep;
	PUINT8 ptr;
	RRM_BEACON_REP_INFO_FIELD BcnReqInfoField;
	UINT32 Ptsf;
	BCN_IE_LIST *ie_list = NULL;
#ifndef TP_NETWORK_ROAMING
	PNET_DEV NetDev = NULL;
#endif
	BOOLEAN bFrameBody = FALSE;

	os_alloc_mem(NULL, (UCHAR **)&ie_list, sizeof(BCN_IE_LIST));
	if (ie_list == NULL) {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(): Alloc ie_list failed!\n", __FUNCTION__));
		return;
	}
	NdisZeroMemory(ie_list, sizeof(BCN_IE_LIST));

	ptr = (PUINT8)pBcnRepInfo;

	pBcnRep = (PRRM_BEACON_REP_INFO)ptr;
#ifdef RT_BIG_ENDIAN
		pBcnRep->ActualMeasureStartTime = le2cpu64(pBcnRep->ActualMeasureStartTime);
		pBcnRep->MeasureDuration = le2cpu16(pBcnRep->MeasureDuration);
		pBcnRep->ParentTSF = le2cpu32(pBcnRep->ParentTSF);
#endif
#ifndef TP_NETWORK_ROAMING
	/*we assemble all bssInfo into one msg and send it to up-layer in RRM_PeerMeasureRepAction()
	   so pass this step*/
	/* send Beacon Response to up-layer */
	NetDev = pAd->ApCfg.MBSSID[pDialogEntry->ControlIndex].wdev.if_dev;
	wext_send_bcn_rsp_event(NetDev, pDialogEntry->StaMac,
		ptr, sizeof(*pBcnRepInfo), pDialogEntry->DialogToken);
#endif
	Ptsf = pBcnRep->ParentTSF;
	BcnReqInfoField.word = pBcnRep->RepFrameInfo; 

	KV_PRINT(("%s:: ReqClass=%d, Channel=%d\n",__FUNCTION__, pBcnRep->RegulatoryClass, pBcnRep->ChNumber));

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
						PRINT_MAC(pBcnRep->Bssid)));

	Rssi = pBcnRep->RCPI + pAd->BbpRssiToDbmDelta;

	RemainLen -= sizeof(RRM_BEACON_REP_INFO);
	ptr += sizeof(RRM_BEACON_REP_INFO);

	/* check option sub element IE. */
	while (RemainLen > 0)
	{
		PRRM_SUBFRAME_INFO pRrmSubFrame;
		pRrmSubFrame = (PRRM_SUBFRAME_INFO)ptr;

		switch(pRrmSubFrame->SubId)
		{
			case 1:
				if (BcnReqInfoField.field.ReportFrameType == 0)
				{
					/* Init Variable IE structure */
					pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
					pVIE->Length = 0;

					bFrameBody = PeerBeaconAndProbeRspSanity(pAd,
								pRrmSubFrame->Oct,
								pRrmSubFrame->Length,
								pBcnRep->ChNumber,
								ie_list,
								&LenVIE,
								pVIE,
									FALSE,
									TRUE);
				}
				break;

			case 221:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:: SubIe: ID=%x, Len=%d\n",
					__FUNCTION__, pRrmSubFrame->SubId, pRrmSubFrame->Length));
				break;
			default:
				break;
		}

		RemainLen -= (pRrmSubFrame->Length + 2);
		ptr += (pRrmSubFrame->Length + 2);

		/* avoid infinite loop. */
		if (pRrmSubFrame->Length == 0)
			break;
	}
	KV_PRINT(("%s(): ie_list->Bssid  is %02X:%02X:%02X:%02X:%02X:%02X!\n", 
	__FUNCTION__,  ie_list->Bssid[0], ie_list->Bssid[1],ie_list->Bssid[2],
	ie_list->Bssid[3],ie_list->Bssid[4],ie_list->Bssid[5]));
	KV_PRINT(("%s(): pBcnRep->Bssid  is %02X:%02X:%02X:%02X:%02X:%02X!\n", 
	__FUNCTION__,  pBcnRep->Bssid[0], pBcnRep->Bssid[1],pBcnRep->Bssid[2],
	pBcnRep->Bssid[3],pBcnRep->Bssid[4],pBcnRep->Bssid[5]));

	if (NdisEqualMemory(pBcnRep->Bssid, ie_list->Bssid, MAC_ADDR_LEN) == FALSE)
	{
		KV_PRINT(("%s():BcnReq->BSSID not equal ie_list->Bssid!\n", __FUNCTION__));
		if (NdisEqualMemory(ie_list->Bssid, ZERO_MAC_ADDR, MAC_ADDR_LEN)) {
			COPY_MAC_ADDR(&ie_list->Addr2[0], pBcnRep->Bssid);
			COPY_MAC_ADDR(&ie_list->Bssid[0], pBcnRep->Bssid);
	}
	}

	if (ie_list->Channel == 0)
		ie_list->Channel = pBcnRep->ChNumber;

#ifdef AP_SCAN_SUPPORT
	if (bFrameBody)
	{
		ie_list->FromBcnReport = TRUE;

	Idx = BssTableSetEntry(pAd, &pAd->ScanTab, ie_list, Rssi, LenVIE, pVIE);
	if (Idx != BSS_NOT_FOUND)
	{
		BSS_ENTRY *pBssEntry = &pAd->ScanTab.BssEntry[Idx];
		NdisMoveMemory(pBssEntry->PTSF, (PUCHAR)&Ptsf, 4);
		pBssEntry->RegulatoryClass = pBcnRep->RegulatoryClass;
		pBssEntry->CondensedPhyType = BcnReqInfoField.field.CondensePhyType;
		pBssEntry->RSNI = pBcnRep->RSNI;
		KV_PRINT(("%s:%d  BssTableSetEntry ok, idx:%lu\n", __FUNCTION__, __LINE__, Idx));

	}
		else
			{
			KV_PRINT(("%s:%d  BssTableSetEntry failed\n", __FUNCTION__, __LINE__));
			}
	}
#endif /* AP_SCAN_SUPPORT */
	if (ie_list != NULL)
		os_free_mem(NULL, ie_list);

	return;
}

VOID RRM_BeaconReportFailReason(
	IN PRTMP_ADAPTER pAd,
	IN MEASURE_REPORT_MODE ReportMode,
	IN UINT8 ReportType,
	IN PMEASURE_REQ_ENTRY pDialogEntry,
	IN UINT8 DialogToken,
	IN PEID_STRUCT eid_ptr,
	IN UCHAR *pAddr)
{
	PNET_DEV NetDev = NULL;

	if (ReportMode.field.Refused) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("STA refuse to do mesurement with type(%d)",
			ReportType));
	} else if (ReportMode.field.Incapable) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("STA do not support mesurement with type(%d)",
			ReportType));
	} else if (ReportMode.field.Late) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("the request to do mesurement with type(%d) too late",
			ReportType));
	} else {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("STA has no beacon report(%02x:%02x:%02x:%02x:%02x:%02x) info to report",
			PRINT_MAC(pAddr)));
	}

	NetDev = pAd->ApCfg.MBSSID[pDialogEntry->ControlIndex].wdev.if_dev;
	wext_send_bcn_rsp_event(NetDev, pDialogEntry->StaMac,
					(PUCHAR)eid_ptr->Octet, 3, DialogToken);
}

#ifdef TP_NETWORK_ROAMING
VOID RRM_Report_IE_Sanity(PRRM_BEACON_REP_INFO pRRM_Beacon_Rep_Info, pieee80211_bcnrpt_t pIEEE80211_Bcnrpt)
{
	NdisMoveMemory(pIEEE80211_Bcnrpt->bssid, pRRM_Beacon_Rep_Info->Bssid, MAC_ADDR_LEN);
	pIEEE80211_Bcnrpt->chnum = pRRM_Beacon_Rep_Info->ChNumber;
	pIEEE80211_Bcnrpt->rcpi = pRRM_Beacon_Rep_Info->RCPI;
	pIEEE80211_Bcnrpt->rsni = pRRM_Beacon_Rep_Info->RSNI;
	/*we did NULL_MAC check in wlanifevent, so mark 'more' as 1 for simplicity*/
	pIEEE80211_Bcnrpt->more = 1;
	KV_PRINT(("pIEEE80211_Bcnrpt->bssid=%02x:%02x:%02x:%02x:%02x:%02x\n, pIEEE80211_Bcnrpt->chnum = %d, pIEEE80211_Bcnrpt->rcpi = %d, pIEEE80211_Bcnrpt->rsni = %d\r\n",
						PRINT_MAC(pIEEE80211_Bcnrpt->bssid), pIEEE80211_Bcnrpt->chnum, pIEEE80211_Bcnrpt->rcpi, pIEEE80211_Bcnrpt->rsni));
}
#endif

VOID RRM_PeerMeasureRepAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	PFRAME_802_11 pFr = (PFRAME_802_11)Elem->Msg;
	PUCHAR pFramePtr = pFr->Octet;
	ULONG MsgLen = Elem->MsgLen;
	PMEASURE_REQ_ENTRY pDialogEntry = NULL;
	PMAC_TABLE_ENTRY pEntry;
	UINT8 DialogToken;
	BOOLEAN Cancelled;
#ifdef TP_NETWORK_ROAMING
	int bcnrptIdx = -1;	
	struct bs_rrm_report_ind rrm_rep_data = {0};
	UCHAR rrmRepMode = -1;
	PNET_DEV NetDev = NULL;
	memset(&rrm_rep_data, 0, sizeof(struct bs_rrm_report_ind));
#endif
	KV_PRINT(("%s::\n", __FUNCTION__));

	/* skip Category and action code. */
	pFramePtr += 2;
	MsgLen -= 2;

	/* get DialogToken. */
	NdisMoveMemory(&DialogToken, pFramePtr, 1);
	pFramePtr += 1;
	MsgLen -= 1;

	/*
		Not a autonomous measure report (non zero DialogToken).
		check the dialog token field.
		drop it if the dialog token doesn't match.
	*/
	if (DialogToken != 0) {
		pDialogEntry = MeasureReqLookUp(pAd, DialogToken);
		if (!pDialogEntry) {
			KV_PRINT(("DialogToken invalid(%d)!\n", DialogToken));
			return;
		}
		RTMPCancelTimer(&pDialogEntry->WaitBCNRepTimer, &Cancelled);
		RTMPReleaseTimer(&pDialogEntry->WaitBCNRepTimer, &Cancelled);
	} else {
		KV_PRINT(("DialogToken auto report(%d)!\n", DialogToken));
		//return;
	}


	do
	{
		PEID_STRUCT eid_ptr;
		MEASURE_REPORT_MODE ReportMode;
		UINT8 ReportType;
		PRRM_BEACON_REP_INFO pMeasureRep;
		LONG BcnRepLen = 0;

		/* Is the STA associated. Dorp the Measure report if it's not. */
		pEntry = MacTableLookup(pAd, pFr->Hdr.Addr2);
		if (!pEntry || (pEntry->Sst != SST_ASSOC))
		{
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("STA not associated. Dorp the Measure report!"));
			break;
		}

		eid_ptr = (PEID_STRUCT)pFramePtr;
		while (((UCHAR*)eid_ptr + eid_ptr->Len + 1) < ((PUCHAR)pFramePtr + MsgLen))
		{
			hex_dump("RRM report IE",(UCHAR *) eid_ptr, eid_ptr->Len);
			switch(eid_ptr->Eid)
			{
				case IE_MEASUREMENT_REPORT:
					{
						if (eid_ptr->Len < 3)
						{
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF,
								("%s, STA(%02x:%02x:%02x:%02x:%02x:%02x) beacon report has a wrong length: %d",
									__FUNCTION__, PRINT_MAC(pFr->Hdr.Addr2), eid_ptr->Len));
							break;
						}

						BcnRepLen = (LONG)eid_ptr->Len - 3;
						NdisMoveMemory(&ReportMode, eid_ptr->Octet + 1, 1);
#ifdef TP_NETWORK_ROAMING
						NdisMoveMemory(&rrmRepMode, eid_ptr->Octet + 1, 1);
#endif
						NdisMoveMemory(&ReportType, eid_ptr->Octet + 2, 1);
						pMeasureRep = (PVOID)(eid_ptr->Octet + 3);
						if (BcnRepLen) {
						if (ReportType == RRM_MEASURE_SUBTYPE_BEACON)
							RRM_BeaconReportHandler(pAd, pMeasureRep,
									BcnRepLen, pDialogEntry);
#ifdef TP_NETWORK_ROAMING
						if (ReportType == RRM_MEASURE_SUBTYPE_BEACON)
						{
							if (bcnrptIdx < IEEE80211_BSTEERING_RRM_NUM_BCNRPT_MAX - 1)
							{
								bcnrptIdx++;
								RRM_Report_IE_Sanity(pMeasureRep, &rrm_rep_data.data.bcnrpt[bcnrptIdx]);
							}
						}

#endif
						} else {
						/*
							RRM_BeaconReportFailReason(pAd, ReportMode,
								ReportType, pDialogEntry, DialogToken,
								eid_ptr, pFr->Hdr.Addr2);
								*/
						}
					}
					break;

				default:
					break;
			}
			eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);
		}
	} while(FALSE);
#ifdef TP_NETWORK_ROAMING
	KV_PRINT(("%s %d bcnrptIdx = %d\r\n", __FUNCTION__, __LINE__, bcnrptIdx));
	if (bcnrptIdx >= 0)
	{
		int i = 0;
		for (i = 0; i <= bcnrptIdx; i++)
		{
			KV_PRINT(("%s %d : bcnrptIdx[%d], values : bcnrpt.bssid=%02x:%02x:%02x:%02x:%02x:%02x, bcnrpt.chnum = %d, bcnrpt.rcpi = %d, bcnrpt.rsni = %d\r\n",
			__FUNCTION__, __LINE__, i, PRINT_MAC(rrm_rep_data.data.bcnrpt[i].bssid), rrm_rep_data.data.bcnrpt[i].chnum, rrm_rep_data.data.bcnrpt[i].rcpi, rrm_rep_data.data.bcnrpt[i].rsni));
		}
		
		rrm_rep_data.rrm_type = BSTEERING_RRM_TYPE_BCNRPT;
		//bs_rrm_req_ind.measrpt_mode = (bcnrptIdx >= 0) ? 0 : 1; /*0: succeed*/
		rrm_rep_data.measrpt_mode = rrmRepMode;
		rrm_rep_data.dialog_token = DialogToken;
		memcpy(rrm_rep_data.macaddr, pFr->Hdr.Addr2, MAC_ADDR_LEN);
		NetDev = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.if_dev;		
		printk("%s %d wext_send_bcn_rsp_event to send ATH_EVENT_BSTEERING_RRM_REPORT, rrm.macaddr = %02x:%02x:%02x:%02x:%02x:%02x\r\n", __FUNCTION__, __LINE__, PRINT_MAC(rrm_rep_data.macaddr));
		wext_send_bcn_rsp_event(NetDev, pFr->Hdr.Addr2,
			(PUINT8)&rrm_rep_data, sizeof(struct bs_rrm_report_ind), DialogToken);
	}
#endif
	if ( DialogToken && pDialogEntry)
		MeasureReqDelete(pAd, pDialogEntry->DialogToken);
}


int rrm_MsgHandle(PRTMP_ADAPTER pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	p_rrm_command_t p_rrm_command = NULL;
	int status = NDIS_STATUS_FAILURE;

	os_alloc_mem(NULL, (UCHAR **)&p_rrm_command, wrq->u.data.length);
	if (p_rrm_command == NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("!!!(%s) : no memory!!!\n", __func__));
		return NDIS_STATUS_FAILURE;
	}

	copy_from_user(p_rrm_command, wrq->u.data.pointer, wrq->u.data.length);

	switch (p_rrm_command->command_id) {
	case OID_802_11_RRM_CMD_ENABLE:
		if (p_rrm_command->command_len == 0x1) {
			status = Set_Dot11kRRM_Enable(pAd, p_rrm_command->command_body[0]);
		} else {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("Unexpected command len (%d)", p_rrm_command->command_len));
			status = NDIS_STATUS_FAILURE;
		}
		break;

	case OID_802_11_RRM_CMD_CAP:
		if (p_rrm_command->command_len == 0x8) {
			status = set_rrm_capabilities(pAd, p_rrm_command->command_body);
		} else {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("Unexpected command len (%d)", p_rrm_command->command_len));
			status = NDIS_STATUS_FAILURE;
		}
		break;
	/*this cmd is used to compose beacon request frame by the up-layer raw data */
	case OID_802_11_RRM_CMD_SEND_BEACON_REQ:
			status = rrm_send_beacon_req(pAd,
				(p_bcn_req_data_t)p_rrm_command->command_body,
				p_rrm_command->command_len);
		break;
	/*this cmd is used to compose beacon request frame by the up-layer parameters*/
	case OID_802_11_RRM_CMD_SET_BEACON_REQ_PARAM:
			status = rrm_send_beacon_req_param(pAd,
				(p_bcn_req_info)p_rrm_command->command_body,
				p_rrm_command->command_len);
		break;
	/*this cmd is used to compose neighbor report frame by the up-layer raw data*/
	case OID_802_11_RRM_CMD_SEND_NEIGHBOR_REPORT:
			status = rrm_send_nr_rsp_ie(pAd,
				(p_nr_rsp_data_t)p_rrm_command->command_body,
				p_rrm_command->command_len);
		break;
	/*this cmd is used to compose neighbor report frame by the up-layer parameters*/
	case OID_802_11_RRM_CMD_SET_NEIGHBOR_REPORT_PARAM:
			status = rrm_send_nr_rsp_param(pAd,
						(p_rrm_nrrsp_info_custom_t)p_rrm_command->command_body,
							p_rrm_command->command_len);
		break;
	/*this cmd is used to config whether the driver or the up-layer handle the neighbor report request*/
	case OID_802_11_RRM_CMD_HANDLE_NEIGHBOR_REQUEST_BY_DAEMON:
			status = rrm_set_handle_nr_req_flag(pAd, p_rrm_command->command_body[0]);
		break;
		}
	os_free_mem(NULL, p_rrm_command);
	return status;
}


int Set_Dot11kRRM_Enable(RTMP_ADAPTER *pAd, UINT8 enable)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex = pObj->ioctl_if;
	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[ifIndex];

	if (ifIndex >= pAd->ApCfg.BssidNum) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("Unknown If index (%d)", ifIndex));
		return NDIS_STATUS_FAILURE;
	}

	pMbss->RrmCfg.bDot11kRRMEnable = (BOOLEAN)(enable) == 0 ? FALSE : TRUE;

	if (pMbss->RrmCfg.bDot11kRRMEnable == TRUE)
		pMbss->CapabilityInfo |= RRM_CAP_BIT;
	else
		pMbss->CapabilityInfo &= ~RRM_CAP_BIT;

	APUpdateCapabilityAndErpIe(pAd);
	APUpdateBeaconFrame(pAd, ifIndex);
	return NDIS_STATUS_SUCCESS;
}

int set_rrm_capabilities(RTMP_ADAPTER *pAd, UINT8 *rrm_capabilities)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR ifIndex = pObj->ioctl_if;
	BSS_STRUCT *pMbss;
	PRRM_CONFIG pRrmCfg;

	if (ifIndex >= pAd->ApCfg.BssidNum) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("Unknown If index (%d)", ifIndex));
		return NDIS_STATUS_FAILURE;
	}

	pMbss = &pAd->ApCfg.MBSSID[ifIndex];
	pRrmCfg = &pMbss->RrmCfg;
	NdisCopyMemory((void *)&pRrmCfg->rrm_capabilities, (void *)rrm_capabilities, sizeof(RRM_EN_CAP_IE));

	pRrmCfg->rrm_capabilities.word &= pRrmCfg->max_rrm_capabilities.word;

	if (pMbss->RrmCfg.bDot11kRRMEnable == TRUE)
		APUpdateBeaconFrame(pAd, ifIndex);

	return NDIS_STATUS_SUCCESS;
}

int rrm_send_beacon_req(
	RTMP_ADAPTER *pAd,
	p_bcn_req_data_t p_bcn_req_data,
	UINT32 bcn_req_len)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	PUCHAR pBuf = NULL;
	PBCN_EVENT_DATA Event;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	BSS_STRUCT *pMbss = NULL;
	UINT32 EventLen = 0, Len = 0;
	NDIS_STATUS NStatus = NDIS_STATUS_SUCCESS;
	UCHAR IfIdx = pObj->ioctl_if;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s\n", __func__));

	/* check the AP supports 11k or not */
	pMbss = &pAd->ApCfg.MBSSID[IfIdx];
	if (pMbss->RrmCfg.bDot11kRRMEnable == FALSE) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() AP(%02x:%02x:%02x:%02x:%02x:%02x) not support rrm\n",
			__func__, PRINT_MAC(pMbss->wdev.bssid)));
		return NDIS_STATUS_FAILURE;
	}

	Len = sizeof(*p_bcn_req_data) + p_bcn_req_data->bcn_req_len;
	if (bcn_req_len != Len) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s, bcn req len check failed len1=%d, len2=%d\n",
			__func__, bcn_req_len, Len));
		return NDIS_STATUS_FAILURE;
	}

	/*send bcn req msg into mlme*/
	EventLen = sizeof(*Event) + bcn_req_len;
	os_alloc_mem(NULL, (UCHAR **)&pBuf, EventLen);
	if (!pBuf) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s, allocate event memory failed\n", __func__));
		return NDIS_STATUS_RESOURCES;
	}
	NdisZeroMemory(pBuf, EventLen);

	/*insert bcn req token into measure table*/
	if (p_bcn_req_data->dialog_token == 0) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s, invalid MesureReq Token(0)!\n",
			__func__));
		return NDIS_STATUS_FAILURE;
	}
	pEntry = MeasureReqInsert(pAd, p_bcn_req_data->dialog_token);
	if (!pEntry) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s, Fail to Insert MesureReq Token(%d)!\n",
			__func__, p_bcn_req_data->dialog_token));
		os_free_mem(NULL, pBuf);
		return NDIS_STATUS_FAILURE;
	}
	pEntry->skip_time_check = TRUE;
	pEntry->CurrentState = WAIT_BCN_REQ;
	pEntry->Priv = pAd;
	pEntry->ControlIndex = IfIdx;

	Event = (BCN_EVENT_DATA *)pBuf;
	Event->ControlIndex = IfIdx;
	Event->MeasureReqToken = p_bcn_req_data->dialog_token;
	RTMPMoveMemory(Event->stamac, p_bcn_req_data->peer_address, MAC_ADDR_LEN);
	Event->DataLen = bcn_req_len;
	NdisMoveMemory(Event->Data, p_bcn_req_data, bcn_req_len);

	if (MlmeEnqueue(pAd, BCN_STATE_MACHINE, BCN_REQ_RAW, EventLen, pBuf, 0)
		== FALSE) {
		NStatus = NDIS_STATUS_FAILURE;
		MeasureReqDelete(pAd, p_bcn_req_data->dialog_token);
	}

	/*free memory*/
	os_free_mem(NULL, pBuf);
	return NStatus;
}

INT rrm_send_beacon_req_param(
	IN PRTMP_ADAPTER pAd,
	IN p_bcn_req_info pBcnReq,
	IN UINT32 BcnReqLen)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BSS_STRUCT *pMbss = NULL;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	PBCN_EVENT_DATA Event = NULL;
	UINT32 Len = 0;
	NDIS_STATUS NStatus = NDIS_STATUS_SUCCESS;
	PUCHAR pBuf = NULL;
	UINT8 IfIdx = pObj->ioctl_if;
	UINT8 MeasureReqToken = 0;
	static UINT8 k;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s\n", __func__));

	/* check the AP supports 11k or not */
	pMbss = &pAd->ApCfg.MBSSID[IfIdx];
	if (pMbss->RrmCfg.bDot11kRRMEnable == FALSE) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() rrm off\n", __func__));
		return NDIS_STATUS_FAILURE;
	}

	if (BcnReqLen != sizeof(*pBcnReq)) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, bcn req len check failed len1=%d, len2=%d\n",
			__func__, BcnReqLen, sizeof(*pBcnReq)));
		return NDIS_STATUS_FAILURE;
	}

	/*send bcn req msg into mlme*/
	Len = sizeof(*Event) + BcnReqLen;
	os_alloc_mem(NULL, (UCHAR **)&pBuf, Len);
	if (!pBuf) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, allocate event memory failed\n", __func__));
		return NDIS_STATUS_RESOURCES;
	}
	NdisZeroMemory(pBuf, Len);

	/*insert bcn req token into measure table*/
	k++;
	if (k == 0)
		k = 1;
	MeasureReqToken = k;
	pEntry = MeasureReqInsert(pAd, MeasureReqToken);
	if (!pEntry) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, Fail to Insert MesureReq Token(%d)!\n",
			__func__, MeasureReqToken));
		os_free_mem(NULL, pBuf);
		return NDIS_STATUS_FAILURE;
	}
	pEntry->skip_time_check = TRUE;
	pEntry->CurrentState = WAIT_BCN_REQ;
	pEntry->Priv = pAd;
	pEntry->ControlIndex = IfIdx;
	COPY_MAC_ADDR(pEntry->StaMac, pBcnReq->peer_address);

	Event = (PBCN_EVENT_DATA)pBuf;
	Event->ControlIndex = IfIdx;
	Event->MeasureReqToken = MeasureReqToken;
	RTMPMoveMemory(Event->stamac, pBcnReq->peer_address, MAC_ADDR_LEN);
	Event->DataLen = BcnReqLen;
	NdisMoveMemory(Event->Data, pBcnReq, BcnReqLen);

	if (MlmeEnqueue(pAd, BCN_STATE_MACHINE, BCN_REQ, Len, pBuf, 0) == FALSE) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, Fail to MlmeEnqueue BCN_REQ! Token(%d)!\n",
			__func__, MeasureReqToken));
		NStatus = NDIS_STATUS_FAILURE;
		MeasureReqDelete(pAd, MeasureReqToken);
	}
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s, MlmeEnqueue BCN_REQ successfully! Token (%d)!\n",
		__func__, MeasureReqToken));

	/*free memory*/
	os_free_mem(NULL, pBuf);
	return NStatus;
}



void RRM_measurement_report_to_host(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	PFRAME_802_11 pFr = (PFRAME_802_11)Elem->Msg;
	PUCHAR pFramePtr = pFr->Octet;
	ULONG MsgLen = Elem->MsgLen;
	UINT8 DialogToken;
	p_rrm_event_t p_rrm_event;
	p_bcn_rsp_data_t p_bcn_rsp_data;

	MsgLen -= sizeof(HEADER_802_11);

	/* skip category and action code. */
	pFramePtr += 2;
	MsgLen -= 2;
	DialogToken = *pFramePtr;

	pFramePtr += 1;
	MsgLen -= 1;
	os_alloc_mem(NULL, (UCHAR **)&p_rrm_event, sizeof(rrm_event_t) + sizeof(bcn_rsp_data_t) + MsgLen);
	if (p_rrm_event == NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("!!!(%s) : no memory!!!\n", __func__));
		return;
	}

	p_rrm_event->event_id = 1;
	p_rrm_event->event_len = sizeof(bcn_rsp_data_t) + MsgLen;
	p_bcn_rsp_data = (p_bcn_rsp_data_t)(&p_rrm_event->event_body[0]);
	p_bcn_rsp_data->dialog_token = DialogToken;
	p_bcn_rsp_data->ifindex = 1;
	p_bcn_rsp_data->bcn_rsp_len = MsgLen;
	COPY_MAC_ADDR(p_bcn_rsp_data->peer_address, pFr->Hdr.Addr2);
	memcpy(&p_bcn_rsp_data->bcn_rsp[0], pFramePtr, MsgLen);
	RtmpOSWrielessEventSend(
				pAd->net_dev,
				RT_WLAN_EVENT_CUSTOM,
				OID_802_11_RRM_EVENT,
				NULL,
				(UCHAR *) p_rrm_event,
				sizeof(rrm_event_t) + sizeof(bcn_rsp_data_t) + MsgLen);
	os_free_mem(NULL, p_rrm_event);
}


/*
*	this function is used to compose the neighbor report response frame with
*	parameters sent by the up-layer
*	return value: error reason or success
*/
int rrm_send_nr_rsp_param(RTMP_ADAPTER *pAd,
	p_rrm_nrrsp_info_custom_t p_nr_rsp_data, UINT32 nr_rsp_data_len)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	PNR_EVENT_DATA Event = NULL;
	BSS_STRUCT *pMbss = NULL;
	PUCHAR pBuf = NULL;
	UINT32 Len = 0;
	UCHAR ifIndex = pObj->ioctl_if;

	pMbss = &pAd->ApCfg.MBSSID[ifIndex];
	if (pMbss->RrmCfg.bDot11kRRMEnable == FALSE) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() rrm off\n", __func__));
		return NDIS_STATUS_FAILURE;
	}

	Len = MAC_ADDR_LEN + 2 +
		p_nr_rsp_data->nrresp_info_count * sizeof(struct nr_info);
	if (nr_rsp_data_len != Len) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() length check failed nr_rsp_data_len=%d, Len=%d\n",
			__func__, nr_rsp_data_len, Len));
		return NDIS_STATUS_FAILURE;
	}

	if (p_nr_rsp_data->nrresp_info_count > MAX_CANDIDATE_NUM) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s the num of candidate(%d) excceed %d\n", __func__,
			  p_nr_rsp_data->nrresp_info_count, MAX_CANDIDATE_NUM));
		return NDIS_STATUS_FAILURE;
	}

	/*send msg to mlme*/
	Len = sizeof(*Event) + nr_rsp_data_len;
	os_alloc_mem(NULL, (UCHAR **)&pBuf, Len);
	if (!pBuf) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s():allocate event memory failed\n", __func__));
		return NDIS_STATUS_RESOURCES;
	}
	NdisZeroMemory(pBuf, Len);
	Event = (PNR_EVENT_DATA)pBuf;
	Event->ControlIndex = ifIndex;
	Event->MeasureReqToken = p_nr_rsp_data->dialogtoken;
	NdisMoveMemory(Event->stamac, p_nr_rsp_data->peer_address,
		MAC_ADDR_LEN);
	Event->DataLen = nr_rsp_data_len;
	NdisMoveMemory(Event->Data, (PUCHAR)p_nr_rsp_data, nr_rsp_data_len);
	MlmeEnqueue(pAd, NEIGHBOR_STATE_MACHINE, NR_RSP_PARAM, Len, pBuf, 0);

	/*free the memory*/
	os_free_mem(NULL, pBuf);

	return NDIS_STATUS_SUCCESS;
}


/*this function is used to check the validity of customer parameters*/
int check_rrm_nrrsp_custom_params(
	RTMP_ADAPTER *pAd,
	p_rrm_nrrsp_info_custom_t p_nr_rsp_data,
	UINT32 nr_rsp_data_len)
{
	struct nr_info *pinfo = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;
	UINT32 Len = 0;
	UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	RRM_EN_CAP_IE Zero_RrmEnCap;

	pEntry = MacTableLookup(pAd, p_nr_rsp_data->peer_address);
	if (!pEntry || !((Ndis802_11AuthModeOpen ==
			pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode) ||
			((pEntry->WpaState == AS_PTKINITDONE) &&
			(pEntry->GTKState == REKEY_ESTABLISHED)))) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) not associates with AP!\n",
			__func__, PRINT_MAC(p_nr_rsp_data->peer_address)));
		goto error;
	}

	memset((PUCHAR)&Zero_RrmEnCap, 0, sizeof(Zero_RrmEnCap));
	if (!memcmp(&Zero_RrmEnCap, &pEntry->RrmEnCap,
			sizeof(RRM_EN_CAP_IE))) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) not support rrm\n",
			__func__, PRINT_MAC(p_nr_rsp_data->peer_address)));
		goto error;
	}

	if (p_nr_rsp_data->nrresp_info_count == 0) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() nr count equals to 0; neighbor report response is meaningless\n",
			__func__));
		return NDIS_STATUS_SUCCESS;
	}

	Len = p_nr_rsp_data->nrresp_info_count * sizeof(struct nr_info);
	pinfo = p_nr_rsp_data->nrresp_info;

/*
*	need check the validity of each neighbor report candidates
*	   check bssid field of each neighbor report candidates
*/
	while (Len > 0) {
		if (MAC_ADDR_EQUAL(pinfo->bssid, ZERO_MAC_ADDR)) {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s() bssid check failed\n", __func__));
			goto error;
		}
		if (pinfo->channum == 0) {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s() channel check failed\n", __func__));
			goto error;
		}

		Len -= sizeof(struct nr_info);
		pinfo++;
	}

	return NDIS_STATUS_SUCCESS;

error:
	return NDIS_STATUS_FAILURE;
}



void compose_rrm_nrrsp_ie(RTMP_ADAPTER *pAd,
	PUCHAR nr_rsp_ie, PUINT32 p_ie_len,
	struct nr_info *p_candidate_info, UINT8 cnt)
{
	UINT i = 0;
	struct nr_info *p_info = p_candidate_info;
	BOOLEAN nr_flag = FALSE;
	ULONG bss_index = BSS_NOT_FOUND;
	BSS_ENTRY *pBssEntry = NULL;
	RRM_NEIGHBOR_REP_INFO NeighborRepInfo;
	RRM_BSSID_INFO BssidInfo;

	for (i = 0; i < cnt; i++, p_info++) {
/*
*	first, check if need get info from scan table to compose
*	neighbor report response ie
*/
		if (!(p_info->phytype &&
			p_info->regclass &&
			p_info->capinfo &&
			p_info->is_ht &&
			p_info->is_vht &&
			p_info->mobility)) {
			nr_flag = TRUE;
		}
/*
*	if nr_flag equals to TRUE, it means that the up-layer need driver to
*	help with the build of neighbor report ie. so need find info to complete
*	the ie from scan table matched by bssid. if do not find info in
*	scan table, just skip this candidate
*/
		if (nr_flag) {
			bss_index =
				BssTableSearch(&pAd->ScanTab, p_info->bssid, p_info->channum);
			if (bss_index == BSS_NOT_FOUND) {
				MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s() bss not found\n", __func__));
				continue;
			}
			pBssEntry = &pAd->ScanTab.BssEntry[bss_index];
			if (p_info->regclass == 0)
				p_info->regclass = pBssEntry->RegulatoryClass;
			if (p_info->capinfo == 0)
				p_info->capinfo = pBssEntry->CapabilityInfo;
			if (p_info->is_ht == 0)
				p_info->is_ht = (pBssEntry->HtCapabilityLen != 0)?1:0;
#ifdef DOT11_VHT_AC
			if (p_info->is_vht == 0)
				p_info->is_vht = (pBssEntry->vht_cap_len != 0)?1:0;
#endif /* DOT11_VHT_AC */
			if (p_info->is_ht == 0)
				p_info->is_ht = (pBssEntry->HtCapabilityLen != 0)?1:0;
			if (p_info->mobility == 0)
				p_info->mobility = (pBssEntry->bHasMDIE)?1:0;

			if (pBssEntry->Channel > 14) {
				if (pBssEntry->HtCapabilityLen != 0) {
#ifdef DOT11_VHT_AC
					if (pBssEntry->vht_cap_len != 0)
						pBssEntry->CondensedPhyType = 9;
					else
#endif /* DOT11_VHT_AC */
						pBssEntry->CondensedPhyType = 7;
				} else { /* OFDM case */
					pBssEntry->CondensedPhyType = 4;
				}
			} else {
				if (pBssEntry->HtCapabilityLen != 0) /* HT case */
					pBssEntry->CondensedPhyType = 7;
				else if (ERP_IS_NON_ERP_PRESENT(pBssEntry->Erp)) /* ERP case */
					pBssEntry->CondensedPhyType = 6;
				else if (pBssEntry->SupRateLen > 4) /* OFDM case (1,2,5.5,11 for CCK 4 Rates) */
					pBssEntry->CondensedPhyType = 4;
				/* no CCK's definition in spec. */
			}
			if (p_info->phytype == 0)
				p_info->phytype = pBssEntry->CondensedPhyType;
		}

		/*compose neighbor report ie buffer*/
		BssidInfo.word = 0;
		BssidInfo.field.APReachAble =
			(p_info->ap_reachability == 0)?3:p_info->ap_reachability;
		BssidInfo.field.Security = p_info->security;
		BssidInfo.field.KeyScope = p_info->key_scope;
		BssidInfo.field.SepctrumMng = (p_info->capinfo & (1 << 8))?1:0;
		BssidInfo.field.Qos = (p_info->capinfo & (1 << 9))?1:0;
		BssidInfo.field.APSD = (p_info->capinfo & (1 << 11))?1:0;
		BssidInfo.field.RRM = (p_info->capinfo & RRM_CAP_BIT)?1:0;
		BssidInfo.field.DelayBlockAck = (p_info->capinfo & (1 << 14))?1:0;
		BssidInfo.field.ImmediateBA = (p_info->capinfo & (1 << 15))?1:0;
		BssidInfo.field.MobilityDomain = p_info->mobility;
		BssidInfo.field.HT = p_info->is_ht;
#ifdef DOT11_VHT_AC
		BssidInfo.field.VHT = p_info->is_vht;
#endif /* DOT11_VHT_AC */
		COPY_MAC_ADDR(NeighborRepInfo.Bssid, p_info->bssid);
		NeighborRepInfo.BssidInfo = cpu2le32(BssidInfo.word);
		NeighborRepInfo.RegulatoryClass = p_info->regclass;
		NeighborRepInfo.ChNum = p_info->channum;
		NeighborRepInfo.PhyType = p_info->phytype;

		RRM_InsertNeighborRepIE_New(pAd, (nr_rsp_ie + *p_ie_len), (PULONG)p_ie_len,
				sizeof(RRM_NEIGHBOR_REP_INFO), &NeighborRepInfo);
	}

}



VOID send_nr_resp_toair(
	IN PRTMP_ADAPTER    pAd,
	IN MLME_QUEUE_ELEM  *Elem)
{
	PNR_EVENT_DATA Event = (PNR_EVENT_DATA)Elem->Msg;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	PMAC_TABLE_ENTRY pMacEntry = NULL;
	PUCHAR Buf = NULL;
	PRRM_FRAME RRMFrame = NULL;
	UINT32 FrameLen = 0, VarLen = Event->DataLen;
	BOOLEAN Cancelled;
	RRM_EN_CAP_IE Zero_RrmEnCap;

	/*check token*/
	pEntry = MeasureReqLookUp(pAd, Event->MeasureReqToken);
	if (pEntry == NULL && Event->MeasureReqToken != 0) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() invalid token(%d)\n", __func__,
			Event->MeasureReqToken));
		return;
	}

	pMacEntry = MacTableLookup(pAd, Event->stamac);
	if (!pMacEntry || !((Ndis802_11AuthModeOpen ==
			pAd->ApCfg.MBSSID[pMacEntry->func_tb_idx].wdev.AuthMode) ||
			((pMacEntry->WpaState == AS_PTKINITDONE) &&
			(pMacEntry->GTKState == REKEY_ESTABLISHED)))) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x not associates with AP!\n",
			__func__, PRINT_MAC(Event->stamac)));
		return;
	}

	memset((PUCHAR)&Zero_RrmEnCap, 0, sizeof(Zero_RrmEnCap));
	if (!memcmp((PUCHAR)&Zero_RrmEnCap, (PUCHAR)&pMacEntry->RrmEnCap,
			sizeof(RRM_EN_CAP_IE))) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) not support rrm\n",
			__func__, PRINT_MAC(Event->stamac)));
		return;
	}

	os_alloc_mem(NULL, (UCHAR **)&Buf, sizeof(*RRMFrame) + VarLen);
	if (!Buf) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s Not available memory for neighbor report frame\n",
			__func__));
		return;
	}

	NdisZeroMemory(Buf, sizeof(*RRMFrame) + VarLen);
	RRMFrame = (PRRM_FRAME)Buf;
	ActHeaderInit(pAd, &RRMFrame->Hdr, Event->stamac,
					pAd->ApCfg.MBSSID[Event->ControlIndex].wdev.bssid,
					pAd->ApCfg.MBSSID[Event->ControlIndex].wdev.bssid);
	FrameLen = sizeof(HEADER_802_11);
	RRMFrame->Category = CATEGORY_RM;
	FrameLen += 1;
	RRMFrame->u.NR_RSP.Action = RRM_NEIGHTBOR_RSP;
	FrameLen += 1;
	RRMFrame->u.NR_RSP.DialogToken = Event->MeasureReqToken;
	FrameLen += 1;
	NdisMoveMemory(RRMFrame->u.NR_RSP.Variable, Event->Data,
					Event->DataLen);
	FrameLen += Event->DataLen;

	/*receive neighbor report response;  cancel timer*/
	if (Event->MeasureReqToken) {
		RTMPCancelTimer(&pEntry->WaitNRRspTimer, &Cancelled);
		RTMPReleaseTimer(&pEntry->WaitNRRspTimer, &Cancelled);
	}
	MeasureReqDelete(pAd, Event->MeasureReqToken);
	MiniportMMRequest(pAd, (MGMT_USE_QUEUE_FLAG | QID_AC_BE), Buf, FrameLen);
	os_free_mem(NULL, Buf);

}



VOID send_nr_rsp_param_toair(
	IN PRTMP_ADAPTER    pAd,
	IN MLME_QUEUE_ELEM  *Elem)
{
	PNR_EVENT_DATA Event = (PNR_EVENT_DATA)Elem->Msg;
	p_rrm_nrrsp_info_custom_t p_nr_rsp_data = NULL;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	PUCHAR pOutBuffer = NULL;
	ULONG FrameLen = 0;
	UINT32 nr_rsp_data_len = Event->DataLen;
	NDIS_STATUS NStatus = NDIS_STATUS_SUCCESS;
	BOOLEAN Cancelled;
	HEADER_802_11 ActHdr;

	p_nr_rsp_data = (p_rrm_nrrsp_info_custom_t)Event->Data;

	/*check token*/
	pEntry = MeasureReqLookUp(pAd, p_nr_rsp_data->dialogtoken);
	if (pEntry == NULL && p_nr_rsp_data->dialogtoken != 0) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s() invalid token(%d)\n", __func__,
				p_nr_rsp_data->dialogtoken));
		return;
	}
	/*check the other parameters*/
	NStatus = check_rrm_nrrsp_custom_params(pAd, p_nr_rsp_data, nr_rsp_data_len);
	if (NStatus != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() check customer params failed\n", __func__));
		return;
	}

	/*allocate a buffer prepare for Neighbor report IE*/
	NStatus = MlmeAllocateMemory(pAd, (PVOID)&pOutBuffer);
	if (NStatus != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() allocate memory for neighbor report frame failed\n",
			__func__));
		return;
	}
	/*compose 80211 header*/
	ActHeaderInit(pAd, &ActHdr, p_nr_rsp_data->peer_address,
					pAd->ApCfg.MBSSID[Event->ControlIndex].wdev.bssid,
					pAd->ApCfg.MBSSID[Event->ControlIndex].wdev.bssid);
	NdisMoveMemory(pOutBuffer, (PCHAR)&ActHdr, sizeof(ActHdr));
	FrameLen = sizeof(ActHdr);

	/*compose the  Neighbor report header
	*   category, action and token
	*/
	InsertActField(pAd, (pOutBuffer + FrameLen), &FrameLen,
		CATEGORY_RM, RRM_NEIGHTBOR_RSP);
	InsertDialogToken(pAd, (pOutBuffer + FrameLen),
		&FrameLen, p_nr_rsp_data->dialogtoken);

	hex_dump("nr rsp header", pOutBuffer, FrameLen);
	/*compose the  Neighbor report ie*/
	compose_rrm_nrrsp_ie(pAd, pOutBuffer, (PUINT32)&FrameLen,
		p_nr_rsp_data->nrresp_info,
		p_nr_rsp_data->nrresp_info_count);
	hex_dump("nr rsp payload", pOutBuffer,
						FrameLen);
	/*receive neighbor report response;
	*   cancel timer
	*/
	if (p_nr_rsp_data->dialogtoken) {
		RTMPCancelTimer(&pEntry->WaitNRRspTimer, &Cancelled);
		RTMPReleaseTimer(&pEntry->WaitNRRspTimer, &Cancelled);
	}
	MeasureReqDelete(pAd, p_nr_rsp_data->dialogtoken);
	MiniportMMRequest(pAd, (MGMT_USE_QUEUE_FLAG | QID_AC_BE), pOutBuffer, FrameLen);
	os_free_mem(NULL, pOutBuffer);

}



int rrm_send_nr_rsp_ie(RTMP_ADAPTER *pAd,
	p_nr_rsp_data_t p_nr_rsp_data,
	UINT32 nr_rsp_data_len)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BSS_STRUCT *pMbss = NULL;
	PNR_EVENT_DATA Event = NULL;
	PUCHAR pBuf = NULL;
	UINT32 FrameLen = p_nr_rsp_data->nr_rsp_len;
	UINT32 Len = 0;
	UINT8 ifIndex = pObj->ioctl_if;


	pMbss = &pAd->ApCfg.MBSSID[ifIndex];
	if (pMbss->RrmCfg.bDot11kRRMEnable == FALSE) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() rrm off\n", __func__));
		return NDIS_STATUS_FAILURE;
	}

	Len = sizeof(*p_nr_rsp_data) + FrameLen;
	if (nr_rsp_data_len != Len) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() length check failed nr_rsp_data_len=%d, Len=%d\n",
			__func__, nr_rsp_data_len, Len));
		return NDIS_STATUS_FAILURE;
	}

	if (FrameLen > 1000) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s nr rsp ie len(%d) is too long", __func__, FrameLen));
		return NDIS_STATUS_FAILURE;
	}

	/*send a msg into mlme*/
	Len = sizeof(*Event) + FrameLen;
	os_alloc_mem(NULL, (UCHAR **)&pBuf, Len);
	if (!pBuf) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() allocate event memory failed\n", __func__));
		return NDIS_STATUS_RESOURCES;
	}
	NdisZeroMemory(pBuf, Len);
	Event = (PNR_EVENT_DATA)pBuf;
	Event->ControlIndex = ifIndex;
	Event->MeasureReqToken = p_nr_rsp_data->dialog_token;
	NdisMoveMemory(Event->stamac, p_nr_rsp_data->peer_address,
		MAC_ADDR_LEN);
	Event->DataLen = FrameLen;
	NdisMoveMemory(Event->Data, p_nr_rsp_data->nr_rsp, FrameLen);
	MlmeEnqueue(pAd, NEIGHBOR_STATE_MACHINE, NR_RSP, Len, pBuf, 0);

	/*free the memory*/
	os_free_mem(NULL, pBuf);

	return NDIS_STATUS_SUCCESS;
}



/*this func is used to config whether the driver or
*  the up-layer handle the neighbor report request
 */
int rrm_set_handle_nr_req_flag(RTMP_ADAPTER *pAd, UINT8 by_daemon)
{
	pAd->ApCfg.HandleNRReqbyUplayer = (by_daemon == 0) ? 0:1;
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_OFF,
			("%s, HandleNRReqbyUplayer(%d)!\n",
			__func__, pAd->ApCfg.HandleNRReqbyUplayer));

	return NDIS_STATUS_SUCCESS;
}



void wext_send_nr_req_event(PNET_DEV net_dev,
	const char *peer_mac_addr,
	const char *nr_req,
	UINT16 nr_req_len)
{
	struct nr_req_data *data = NULL;
	p_rrm_event_t event_data = NULL;
	UINT32 buflen = 0;
	UCHAR *buf;

	buflen = sizeof(*event_data) + sizeof(*data) + nr_req_len;
	os_alloc_mem(NULL, (UCHAR **)&buf, buflen);
	NdisZeroMemory(buf, buflen);

	event_data = (p_rrm_event_t)buf;
	event_data->event_id = OID_802_11_RRM_EVT_NEIGHBOR_REQUEST;
	event_data->event_len = sizeof(*data) + nr_req_len;

	data = (struct nr_req_data *)event_data->event_body;
	data->ifindex = RtmpOsGetNetIfIndex(net_dev);
	memcpy(data->peer_mac_addr, peer_mac_addr, 6);
	data->nr_req_len = nr_req_len;
	memcpy(data->nr_req, nr_req, nr_req_len);

	RtmpOSWrielessEventSend(net_dev, RT_WLAN_EVENT_CUSTOM,
					OID_802_11_RRM_EVENT, NULL, (PUCHAR)buf, buflen);

	os_free_mem(NULL, buf);
}



/*used to check if the up-layer does not send neighbor report response in time*/
VOID  NRRspTimeout(
	IN PRTMP_ADAPTER    pAd,
	IN MLME_QUEUE_ELEM  *Elem)
{
	PNR_EVENT_DATA Event = (PNR_EVENT_DATA)Elem->Msg;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	UINT8 MeasureReqToken = Event->MeasureReqToken;
	BOOLEAN Cancelled;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s StaMac(%02x:%02x:%02x:%02x:%02x:%02x), MeasureReqToken=%d\n",
		__func__, PRINT_MAC(Event->stamac), MeasureReqToken));

	pEntry = MeasureReqLookUp(pAd, MeasureReqToken);

	if (!pEntry) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s nr response entry not founded\n", __func__));
		return;
	}

	RTMPReleaseTimer(&pEntry->WaitNRRspTimer, &Cancelled);
	/*delete measure entry to avoid the full condition of measure table*/
	MeasureReqDelete(pAd, MeasureReqToken);
}



VOID WaitNRRspTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	PMEASURE_REQ_ENTRY pEntry = (PMEASURE_REQ_ENTRY)FunctionContext;
	PRTMP_ADAPTER pAd;
	UINT32 Len = 0;
	NR_EVENT_DATA Event;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __func__));

	if (!pEntry) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s: MeasureReq Entry doesn't exist\n", __func__));
	return;
}

	pAd = pEntry->Priv;
	NdisZeroMemory(&Event, sizeof(Event));
	Event.ControlIndex = pEntry->ControlIndex;
	Event.MeasureReqToken = pEntry->DialogToken;
	memcpy(Event.stamac, pEntry->StaMac, MAC_ADDR_LEN);
	Event.DataLen = 0;
	Len = sizeof(Event);

	MlmeEnqueue(pAd, NEIGHBOR_STATE_MACHINE, NR_RSP_TIMEOUT, Len, &Event, 0);

}
BUILD_TIMER_FUNCTION(WaitNRRspTimeout);


enum NR_STATE NRPeerCurrentState(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	PNR_EVENT_DATA Event = (PNR_EVENT_DATA)Elem->Msg;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	UINT8 MeasureReqToken = Event->MeasureReqToken;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s, MeasureReqToken=%d!\n", __func__, MeasureReqToken));

	 pEntry = MeasureReqLookUp(pAd, MeasureReqToken);

	if (pEntry != NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, CurrentState=%d!\n", __func__, pEntry->CurrentState));
		return pEntry->CurrentState;
	 } else {
		 MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, invalid MeasureReqToken!\n", __func__));

		return NR_UNKNOWN;
	 }
}

int check_rrm_BcnReq_custom_params(
	RTMP_ADAPTER *pAd,
	p_bcn_req_info p_beacon_req)
{
	PMAC_TABLE_ENTRY pEntry = NULL;
	UCHAR ZERO_OpClassList[OP_LEN] = {0};

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s()\n", __func__));

	/* the the peer is connected or not */
	pEntry = MacTableLookup(pAd, p_beacon_req->peer_address);
	if (!pEntry || !((Ndis802_11AuthModeOpen ==
			pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode) ||
			((pEntry->WpaState == AS_PTKINITDONE) &&
			(pEntry->GTKState == REKEY_ESTABLISHED)))) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) not associates with AP!\n",
			__func__, PRINT_MAC(p_beacon_req->peer_address)));
		if ( pEntry )
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("dump:%d, %d, %d\n", pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode,
				pEntry->WpaState , pEntry->GTKState));
		goto error;
	}

	/* check the peer supports 11k or not */
	if (!(pEntry->RrmEnCap.field.BeaconPassiveMeasureCap ||
		pEntry->RrmEnCap.field.BeaconActiveMeasureCap)) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) not support beacon report!\n",
			__func__, PRINT_MAC(p_beacon_req->peer_address)));
		goto error;
	}


	/* operating class; only mandatory when channel is set to 0*/
	if (p_beacon_req->channum == 0 && p_beacon_req->regclass == 0) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() regclass cannot set to 0 when channel is set to 0\n",
			__func__));
		goto error;
	}

	if (p_beacon_req->channum == 255) {
		if (p_beacon_req->op_class_len == 0) {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s() op class must be set when channel is set to 255!\n",
					__func__));
			goto error;
		}
		if (!RTMPCompareMemory(p_beacon_req->op_class_list,
				ZERO_OpClassList, OP_LEN)) {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s() op class list is invalid!\n", __func__));
			goto error;
		}
	}

	return NDIS_STATUS_SUCCESS;

error:
	return NDIS_STATUS_FAILURE;
}



VOID set_rrm_BcnReq_optional_params(
	RTMP_ADAPTER *pAd,
	p_bcn_req_info p_beacon_req)
{
	PMAC_TABLE_ENTRY pEntry = NULL;
	UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN]  = {0};
	UCHAR BROADCAST_ADDR[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	UCHAR DefaultRequest[13] = {0,	/* SSID */
							1,	 /* Support Rate*/
							45, /* HT IE*/
							48, /* RSN IE */
							50, /* Extended Support Rate*/
							54, /* Mobility Domain. */
							61, /* HT ADD*/
							70, /* RRM Capabilities. */
							127, /* Ext Cap*/
							191, /* VHT 1*/
							192, /* VHT 2*/
							195,  /* VHT 3*/
							221  /*vendor ie*/
						   };

	pEntry = MacTableLookup(pAd, p_beacon_req->peer_address);

	if (p_beacon_req->channum != 0 && p_beacon_req->channum != 255) {
		if (p_beacon_req->regclass == 0) {
			p_beacon_req->regclass =
				get_regulatory_class(pAd);
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				("%s() set optional regclass = %d\n",
				__func__, p_beacon_req->regclass));
		}
	}
	if (p_beacon_req->duration == 0) {
		p_beacon_req->duration = 20;
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() set optional duration = %d\n",
			__func__, p_beacon_req->duration));
	}
	if (!RTMPCompareMemory(p_beacon_req->bssid, ZERO_MAC_ADDR, MAC_ADDR_LEN)) {
		RTMPMoveMemory(p_beacon_req->bssid, BROADCAST_ADDR, MAC_ADDR_LEN);
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() set optional bssid = %02x:%02x:%02x:%02x:%02x:%02x\n",
			__func__, PRINT_MAC(p_beacon_req->bssid)));
	}
	if (p_beacon_req->mode == 0) {
		if (pEntry->RrmEnCap.field.BeaconActiveMeasureCap)
			p_beacon_req->mode = 1;
		else
			p_beacon_req->mode = 0;
	} else {
		p_beacon_req->mode--;
	}
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() set optional mode = %d!\n",
			__func__, p_beacon_req->mode));

	if (p_beacon_req->timeout == 0) {
		p_beacon_req->timeout = BCN_REP_TIMEOUT_VALUE;
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() set optional timeout = %d!\n",
			__func__, p_beacon_req->timeout));
	}

	if (p_beacon_req->detail == 0) {
		p_beacon_req->detail = 1;
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() set optional detail = %d\n",
			__func__, p_beacon_req->detail));
		p_beacon_req->request_len = 13;
		RTMPMoveMemory(p_beacon_req->request, DefaultRequest, 13);
	} else {
		p_beacon_req->detail--;
	}
}



void compose_rrm_BcnReq_ie(RTMP_ADAPTER *pAd,
	PUCHAR beacon_req_ie, PUINT32 beacon_req_ie_len,
	p_bcn_req_info p_beacon_req, UINT8 measure_req_token, UCHAR ifidx)
{
	PUCHAR pOutBuffer = beacon_req_ie;
	ULONG FrameLen = 0, HeaderLen = 0;
	UINT32 FramelenTmp = 0;
	UINT16 Repetitions = 0;
	UINT8 MeasureReqType = RRM_MEASURE_SUBTYPE_BEACON;
	UINT8 TotalLen = 0, idx = 0;
	MEASURE_REQ_MODE MeasureReqMode;
	RRM_BEACON_REQ_INFO BcnReq;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s\n", __func__));
	/*
	*	Action header has a field to indicate total length of packet
	*	but the total length is unknown untial whole packet completd.
	*	So skip the action here and fill it late.
	*	1. skip Catgore (1 octect), Action(1 octect).
	*	2. skip dailog token (1 octect).
	*	3. skip Num Of Repetitions field (2 octects)
	*	3. skip MeasureReqIE (2 + sizeof(MEASURE_REQ_INFO)).
	*/
	FrameLen = 7 + sizeof(MEASURE_REQ_INFO);
	TotalLen = sizeof(MEASURE_REQ_INFO);
	/*Insert Beacon Req IE*/
	BcnReq.RegulatoryClass = p_beacon_req->regclass;
	BcnReq.ChNumber = p_beacon_req->channum;
	BcnReq.RandomInterval = cpu2le16(p_beacon_req->random_ivl);
	BcnReq.MeasureDuration = cpu2le16(p_beacon_req->duration);
	BcnReq.MeasureMode = p_beacon_req->mode;
	COPY_MAC_ADDR(BcnReq.Bssid, p_beacon_req->bssid);
	RRM_InsertBcnReqIE(pAd, (pOutBuffer+FrameLen),
		&FrameLen, (PUCHAR)&BcnReq);
	TotalLen += sizeof(RRM_BEACON_REQ_INFO);
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s RegulatoryClass=%d ChNumber=%d RandomInterval=%d\n",
			__func__, BcnReq.RegulatoryClass, BcnReq.ChNumber,
			BcnReq.RandomInterval));
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("MeasureDuration=%d MeasureMode=%d\n",
			BcnReq.MeasureDuration, BcnReq.MeasureMode));
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("Bssid(%02x:%02x:%02x:%02x:%02x:%02x\n",
			PRINT_MAC(BcnReq.Bssid)));


	/* inssert SSID sub field.
	*	Fix Voice Enterprise : Item V-E-4.3,
	*	case2 still need to include the SSID sub filed even SsidLen is 0
	*/
	RRM_InsertBcnReqSsidSubIE(pAd, (pOutBuffer+FrameLen),
		&FrameLen, (PUCHAR)p_beacon_req->req_ssid, p_beacon_req->req_ssid_len);
	TotalLen += (p_beacon_req->req_ssid_len + 2);

	/* inssert beacon reporting information sub field. */
	RRM_InsertBcnReqRepCndSubIE(pAd, (pOutBuffer+FrameLen),
		&FrameLen, p_beacon_req->rep_conditon, p_beacon_req->ref_value);
		TotalLen += (2 + 2);

	/* inssert reporting detail sub field. */
	RRM_InsertBcnReqRepDetailSubIE(pAd, (pOutBuffer+FrameLen),
			&FrameLen, p_beacon_req->detail);
	TotalLen += (2 + 1);

	if (p_beacon_req->detail == 1) {
		RRM_InsertRequestIE_NEW(pAd, (pOutBuffer+FrameLen),
			&FrameLen, p_beacon_req->request, p_beacon_req->request_len);
		TotalLen += (p_beacon_req->request_len + 2);
	}

	/* inssert channel report sub field. */
	if (p_beacon_req->channum == 255) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() inssert channel report sub field\n", __func__));
		idx = 0;
		while (p_beacon_req->op_class_list[idx] != 0) {
			FramelenTmp = FrameLen;
#ifdef TP_NETWORK_ROAMING
			InsertChannelRepIE_UsingGlobalRegualtoryClass(pAd, (pOutBuffer+FrameLen), &FrameLen,
				p_beacon_req->op_class_list[idx], p_beacon_req->ch_list_len, p_beacon_req->ch_list);
#else
			InsertChannelRepIE(pAd, (pOutBuffer+FrameLen), &FrameLen,
				(RTMP_STRING *)pAd->CommonCfg.CountryCode,
				p_beacon_req->op_class_list[idx], p_beacon_req->ch_list);
#endif
			TotalLen += (FrameLen - FramelenTmp);
			idx++;
		}
	}

	if (TotalLen > 250) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() MeasureReq IE len > 255!\n", __func__));
	}

	/* Insert Action header here. */
	MeasureReqMode.word = 0;
	Repetitions = p_beacon_req->num_rpt;
	MakeMeasurementReqFrame(pAd, pOutBuffer, &HeaderLen,
		TotalLen, CATEGORY_RM, RRM_MEASURE_REQ, measure_req_token,
		MeasureReqMode.word, MeasureReqType, Repetitions);

	*beacon_req_ie_len = FrameLen;
}



void wext_send_bcn_rsp_event(PNET_DEV net_dev,
	PUCHAR peer_mac_addr,
	PUCHAR bcn_rsp,
	UINT32 bcn_rsp_len,
	UINT8 dia_token)
{
	p_bcn_rsp_data_t data = NULL;
	p_rrm_event_t event_data = NULL;
	UINT32 buflen = 0;
	UCHAR *buf;

	if ( !net_dev )
		return ;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s\n", __func__));

	buflen = sizeof(*event_data) + sizeof(*data) + bcn_rsp_len;
	os_alloc_mem(NULL, (UCHAR **)&buf, buflen);
	NdisZeroMemory(buf, buflen);

	event_data = (p_rrm_event_t)buf;
	event_data->event_id = OID_802_11_RRM_EVT_BEACON_REPORT;
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s event_data->event_id = %d\n", __func__,
				event_data->event_id));
	event_data->event_len = sizeof(*data) + bcn_rsp_len;

	data = (p_bcn_rsp_data_t)event_data->event_body;
	data->dialog_token = dia_token;
	data->ifindex = RtmpOsGetNetIfIndex(net_dev);
	memcpy(data->peer_address, peer_mac_addr, 6);
	data->bcn_rsp_len = bcn_rsp_len;
	memcpy(data->bcn_rsp, bcn_rsp, bcn_rsp_len);

	hex_dump("data in Driver", buf, buflen);

	RtmpOSWrielessEventSend(net_dev, RT_WLAN_EVENT_CUSTOM,
					OID_802_11_RRM_EVENT, NULL, (PUCHAR)buf, buflen);

	os_free_mem(NULL, buf);

}


static VOID  BcnRepTimeout(
	IN PRTMP_ADAPTER    pAd,
	IN MLME_QUEUE_ELEM  *Elem)
{
	PNET_DEV NetDev = NULL;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	PBCN_EVENT_DATA Event = (BCN_EVENT_DATA *)Elem->Msg;
	UINT8 MeasureReqToken = Event->MeasureReqToken;
	BOOLEAN Cancelled;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s StaMac(%02x:%02x:%02x:%02x:%02x:%02x) MeasureReqToken=%d\n",
		__func__, PRINT_MAC(Event->stamac), MeasureReqToken));


	pEntry = MeasureReqLookUp(pAd, MeasureReqToken);
	if (!pEntry) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s Bcnreq entry not founded\n", __func__));
		return;
	}

	RTMPReleaseTimer(&pEntry->WaitBCNRepTimer, &Cancelled);
	MeasureReqDelete(pAd, MeasureReqToken);

	/* send Beacon Request timeout to up-layer */
	NetDev = pAd->ApCfg.MBSSID[pEntry->ControlIndex].wdev.if_dev;
	if (NetDev == NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("no valid NetDev! IfIndex=%d\n", pEntry->ControlIndex));
		return;
	}

	wext_send_bcn_rsp_event(NetDev, Event->stamac, Event->Data,
		0, MeasureReqToken);
}



VOID WaitPeerBCNRepTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	PMEASURE_REQ_ENTRY pEntry = (PMEASURE_REQ_ENTRY)FunctionContext;
	PRTMP_ADAPTER pAd = NULL;
	UINT32 Len = 0;
	BCN_EVENT_DATA Event;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s\n", __func__));

	pAd = pEntry->Priv;
	Len = sizeof(Event);
	NdisZeroMemory((PUCHAR)&Event, Len);
	Event.MeasureReqToken = pEntry->DialogToken;
	RTMPMoveMemory(Event.stamac, pEntry->StaMac, MAC_ADDR_LEN);
	Event.DataLen = 0;

	MlmeEnqueue(pAd, BCN_STATE_MACHINE, BCN_REP_TIMEOUT,
		Len, (PUCHAR)&Event, 0);
}
BUILD_TIMER_FUNCTION(WaitPeerBCNRepTimeout);


static VOID SendBcnReqToAir_SetParam(
	IN PRTMP_ADAPTER    pAd,
	IN MLME_QUEUE_ELEM  *Elem)
{
	BCN_EVENT_DATA *Event = (BCN_EVENT_DATA *)Elem->Msg;
	p_bcn_req_info pBcnReq = (p_bcn_req_info)Event->Data;
	PMEASURE_REQ_ENTRY pMeasureReqEntry = NULL;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus = NDIS_STATUS_SUCCESS;
	UINT32 FrameLen = 0, TmpLen = 0;
#if 0
	ULONG Timeout = 0;
#endif
	UINT8 MeasureReqToken = Event->MeasureReqToken;
	UINT8 IfIdx = Event->ControlIndex;
	HEADER_802_11 ActHdr;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s()\n", __func__));

	/*check the parameters*/
	NStatus = check_rrm_BcnReq_custom_params(pAd, pBcnReq);
	if (NStatus != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() check customer params failed\n", __func__));
		MeasureReqDelete(pAd, MeasureReqToken);
		return;
	}

	/*set the optional paratmeters when customer don't set*/
	set_rrm_BcnReq_optional_params(pAd, pBcnReq);

	/*compose the Beacon Request frame*/
	NStatus = MlmeAllocateMemory(pAd, (PVOID)&pOutBuffer);
	if (NStatus != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s, MEMORY ALLOC Failed!!\n", __func__));
		MeasureReqDelete(pAd, MeasureReqToken);
		return;
	}
	ActHeaderInit(pAd, &ActHdr, Event->stamac,
					pAd->ApCfg.MBSSID[IfIdx].wdev.bssid,
					pAd->ApCfg.MBSSID[IfIdx].wdev.bssid);
	memcpy(pOutBuffer, (PUCHAR)&ActHdr, sizeof(ActHdr));
	FrameLen = sizeof(ActHdr);

	compose_rrm_BcnReq_ie(pAd, (pOutBuffer+FrameLen), &TmpLen,
		pBcnReq, MeasureReqToken, IfIdx);
	FrameLen += TmpLen;

	hex_dump("bcn req frame", pOutBuffer, FrameLen);

	KV_PRINT(("\nMiniportMMRequest bcn req!!\n"));
	MiniportMMRequest(pAd, (MGMT_USE_QUEUE_FLAG | QID_AC_BE), pOutBuffer, FrameLen);

	pMeasureReqEntry = MeasureReqLookUp(pAd, MeasureReqToken);
	if (pMeasureReqEntry == NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() pMeasureReqEntry is NULL!\n", __func__));
		MlmeFreeMemory(pAd, pOutBuffer);
		return;
	}
	pMeasureReqEntry->CurrentState = WAIT_BCN_REP;
#if 0
	Timeout = pBcnReq->timeout * 1000;
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() Timeout=%lds!\n", __func__, Timeout/1000));
	//RTMPInitTimer(pAd, &pMeasureReqEntry->WaitBCNRepTimer,
		//GET_TIMER_FUNCTION(WaitPeerBCNRepTimeout), pMeasureReqEntry, FALSE);
	//RTMPSetTimer(&pMeasureReqEntry->WaitBCNRepTimer, Timeout);
#endif
	MlmeFreeMemory(pAd, pOutBuffer);

}


static VOID SendBcnReqToAir_Raw(
	IN PRTMP_ADAPTER    pAd,
	IN MLME_QUEUE_ELEM  *Elem)
{
	PBCN_EVENT_DATA Event = (PBCN_EVENT_DATA)Elem->Msg;
	PMEASURE_REQ_ENTRY pMeasureReqEntry = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;
	p_bcn_req_data_t p_bcn_req_data_raw = (p_bcn_req_data_t)Event->Data;
	PUCHAR pbuf = NULL;
	ULONG Timeout = 0, FrameLen = 0;
	NDIS_STATUS NStatus = NDIS_STATUS_SUCCESS;
	UINT8 MeasureReqToken = Event->MeasureReqToken;
	UINT8 IfIdx = Event->ControlIndex;
	HEADER_802_11 ActHdr;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s()\n", __func__));

	/* the the peer is connected or not */
	pEntry = MacTableLookup(pAd, p_bcn_req_data_raw->peer_address);
	if (!pEntry || !((Ndis802_11AuthModeOpen ==
			pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode) ||
			((pEntry->WpaState == AS_PTKINITDONE) &&
			(pEntry->GTKState == REKEY_ESTABLISHED)))) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) not associates with AP!\n",
			__func__, PRINT_MAC(p_bcn_req_data_raw->peer_address)));
		MeasureReqDelete(pAd, MeasureReqToken);
		return;
	}

	/* check the peer supports 11k or not */
	if (!(pEntry->RrmEnCap.field.BeaconPassiveMeasureCap ||
		pEntry->RrmEnCap.field.BeaconActiveMeasureCap)) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() STA(%02x:%02x:%02x:%02x:%02x:%02x) does not support beacon report!\n",
			__func__, PRINT_MAC(p_bcn_req_data_raw->peer_address)));
		MeasureReqDelete(pAd, MeasureReqToken);
		return;
	}

	/*compose the Beacon Request frame*/
	NStatus = MlmeAllocateMemory(pAd, (PVOID)&pbuf);
	if (NStatus != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() allocate memory failed\n", __func__));
		MeasureReqDelete(pAd, MeasureReqToken);
		return;
	}

	ActHeaderInit(pAd, &ActHdr, p_bcn_req_data_raw->peer_address,
					pAd->ApCfg.MBSSID[IfIdx].wdev.bssid,
					pAd->ApCfg.MBSSID[IfIdx].wdev.bssid);
	NdisMoveMemory(pbuf, (PCHAR)&ActHdr, sizeof(HEADER_802_11));
	FrameLen = sizeof(HEADER_802_11);

	InsertActField(pAd, (pbuf + FrameLen), &FrameLen,
		MT2_PEER_RM_CATE, RRM_MEASURE_REQ);
	InsertDialogToken(pAd, (pbuf + FrameLen), &FrameLen,
		p_bcn_req_data_raw->dialog_token);
	memcpy((pbuf + FrameLen), p_bcn_req_data_raw->bcn_req,
		p_bcn_req_data_raw->bcn_req_len);
	FrameLen += p_bcn_req_data_raw->bcn_req_len;

	MiniportMMRequest(pAd, (MGMT_USE_QUEUE_FLAG | QID_AC_BE),
		pbuf, FrameLen);

	pMeasureReqEntry = MeasureReqLookUp(pAd, MeasureReqToken);
	if (pMeasureReqEntry == NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			("%s() pMeasureReqEntry is NULL!\n", __func__));
		MlmeFreeMemory(pAd, pbuf);
		return;
	}
	pMeasureReqEntry->CurrentState = WAIT_BCN_REP;
	Timeout =  BCN_REP_TIMEOUT_VALUE;
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s() Timeout=%lds!\n", __func__, Timeout/1000));

	RTMPInitTimer(pAd, &pMeasureReqEntry->WaitBCNRepTimer,
		GET_TIMER_FUNCTION(WaitPeerBCNRepTimeout), pMeasureReqEntry, FALSE);
	RTMPSetTimer(&pMeasureReqEntry->WaitBCNRepTimer, Timeout);

	MlmeFreeMemory(pAd, pbuf);
	return;
}

enum BCN_STATE BCNPeerCurrentState(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	PBCN_EVENT_DATA Event = (PBCN_EVENT_DATA)Elem->Msg;
	PMEASURE_REQ_ENTRY pEntry = NULL;
	UINT8 MeasureReqToken = Event->MeasureReqToken;

	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			("%s, MeasureReqToken=%d!\n", __func__, MeasureReqToken));

	pEntry = MeasureReqLookUp(pAd, MeasureReqToken);
	if (pEntry != NULL) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			  ("%s, CurrentState=%d!\n", __func__, pEntry->CurrentState));
		  return pEntry->CurrentState;
	} else {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			  ("%s, invalid MeasureReqToken!\n", __func__));
		return BCN_IDLE;
	}
}



VOID RRMBcnReqStateMachineInit(
			IN	PRTMP_ADAPTER pAd,
			IN	STATE_MACHINE * S,
			OUT STATE_MACHINE_FUNC	Trans[])
{
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s\n", __func__));

	StateMachineInit(S, (STATE_MACHINE_FUNC *)Trans, MAX_BCN_STATE, MAX_BCN_MSG,
		(STATE_MACHINE_FUNC)Drop, BCN_IDLE, BCN_MACHINE_BASE);

#ifdef CONFIG_AP_SUPPORT
	StateMachineSetAction(S, WAIT_BCN_REQ, BCN_REQ, (STATE_MACHINE_FUNC)SendBcnReqToAir_SetParam);
	StateMachineSetAction(S, WAIT_BCN_REQ, BCN_REQ_RAW, (STATE_MACHINE_FUNC)SendBcnReqToAir_Raw);
	StateMachineSetAction(S, WAIT_BCN_REP, BCN_REP_TIMEOUT, (STATE_MACHINE_FUNC)BcnRepTimeout);
#endif /* CONFIG_AP_SUPPORT */
}


VOID NRStateMachineInit(
	IN	PRTMP_ADAPTER pAd,
	IN	STATE_MACHINE * S,
	OUT STATE_MACHINE_FUNC	Trans[])
{
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
		("%s\n", __func__));

	StateMachineInit(S, (STATE_MACHINE_FUNC *)Trans, MAX_NR_STATE, MAX_NR_MSG,
		(STATE_MACHINE_FUNC)Drop, NR_UNKNOWN, NR_MACHINE_BASE);

#ifdef CONFIG_AP_SUPPORT
	StateMachineSetAction(S, WAIT_NR_RSP, NR_RSP, (STATE_MACHINE_FUNC)send_nr_resp_toair);
	StateMachineSetAction(S, WAIT_NR_RSP, NR_RSP_PARAM, (STATE_MACHINE_FUNC)send_nr_rsp_param_toair);
	StateMachineSetAction(S, WAIT_NR_RSP, NR_RSP_TIMEOUT, (STATE_MACHINE_FUNC)NRRspTimeout);
#endif /* CONFIG_AP_SUPPORT */
}


#endif /* DOT11K_RRM_SUPPORT */

