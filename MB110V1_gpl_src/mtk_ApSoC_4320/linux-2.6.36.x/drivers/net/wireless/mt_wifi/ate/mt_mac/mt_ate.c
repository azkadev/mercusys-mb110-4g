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
	mt_ate.c
*/

#include "rt_config.h"

static VOID MtATEWTBL2Update(RTMP_ADAPTER *pAd, UCHAR wcid)
{	
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	union  WTBL_2_DW9 wtbl_2_d9 = {.word = 0};
	UINT32 rate[8]; // reg_val;
	UCHAR stbc, bw, nss, preamble; //, wait_cnt = 0;

	bw = 2;
	wtbl_2_d9.field.fcap = bw;
	wtbl_2_d9.field.ccbw_sel = bw;
	wtbl_2_d9.field.cbrn = 7; // change bw as (fcap/2) if rate_idx > 7, temporary code

	if (ATECtrl->Sgi) {
		wtbl_2_d9.field.g2 = 1;
		wtbl_2_d9.field.g4 = 1;
		wtbl_2_d9.field.g8 = 1;
		wtbl_2_d9.field.g16 = 1;
	} else {
		wtbl_2_d9.field.g2 = 0;
		wtbl_2_d9.field.g4 = 0;
		wtbl_2_d9.field.g8 = 0;
		wtbl_2_d9.field.g16 = 0;
	}
	wtbl_2_d9.field.rate_idx = 0;
	
	if (ATECtrl->PhyMode == MODE_CCK)
		preamble = SHORT_PREAMBLE;
	else
		preamble = LONG_PREAMBLE;

    stbc = ATECtrl->Stbc;
	nss = 1;

	rate[0] = tx_rate_to_tmi_rate(ATECtrl->PhyMode,
											ATECtrl->Mcs,
											nss,
											stbc,
											preamble);
	rate[0] &= 0xfff;

	rate[1] = rate[2] = rate[3] = rate[4] = rate[5] = rate[6] = rate[7] = rate[0];

	Wtbl2RateTableUpdate(pAd, wcid, wtbl_2_d9.word, rate);
}

static INT32 MT_ATERestoreInit(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, Rx receive for 10000us workaround\n", __FUNCTION__));
	/* Workaround CR Restore */
	/* Tx/Rx Antenna Setting Restore */	
	MtAsicSetRxPath(pAd, 0);
	/* TxRx switch workaround */
	AsicSetMacTxRx(pAd, ASIC_MAC_RX, TRUE);
	RtmpusecDelay(10000);

	AsicSetMacTxRx(pAd, ASIC_MAC_RX, FALSE);

	/* Flag Resotre */
	ATECtrl->did_tx = 0;
	ATECtrl->did_rx = 0;
	return Ret;
}

static INT32 MT_ATESetTxPower0(RTMP_ADAPTER *pAd, CHAR Value)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
	ATECtrl->TxPower0 = Value;
	ATECtrl->TxPower1 = pAd->EEPROMImage[TX1_G_BAND_TARGET_PWR];
	CmdSetTxPowerCtrl(pAd, ATECtrl->Channel);

	return Ret;
}

static INT32 MT_ATESetTxPower1(RTMP_ADAPTER *pAd, CHAR Value)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;

	ATECtrl->TxPower1 = Value;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

	/* Same as Power0 */
    if ( ATECtrl->TxPower0 != ATECtrl->TxPower1)
	{
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: power1 do not same as power0\n", __FUNCTION__));
        Ret = -1;
	}

	return Ret;
}


static INT32 MT_ATEStart(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	ATE_IF_OPERATION *if_ops = ATECtrl->ATEIfOps;
	INT32 Ret = 0;
#ifdef CONFIG_AP_SUPPORT
	INT32 IdBss, MaxNumBss = pAd->ApCfg.BssidNum;
#endif

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

	/*   Stop send TX packets */
	RTMP_OS_NETDEV_STOP_QUEUE(pAd->net_dev);

    /* Reset ATE TX/RX Counter */
	ATECtrl->tx_coherent = 0;
    ATECtrl->TxDoneCount = 0;
    ATECtrl->RxTotalCnt = 0;
	ATECtrl->QID = QID_AC_BE;
	ATECtrl->cmd_expire = RTMPMsecsToJiffies(3000);
	RTMP_OS_INIT_COMPLETION(&ATECtrl->cmd_done);
	{
		HQA_MPS_CB *mps_cb = &ATECtrl->mps_cb;
		NdisZeroMemory(mps_cb, sizeof(*mps_cb));
		NdisAllocateSpinLock(pAd, &mps_cb->mps_lock);
	}
	ATECtrl->TxPower0 = pAd->EEPROMImage[TX0_G_BAND_TARGET_PWR];
	ATECtrl->TxPower1 = pAd->EEPROMImage[TX1_G_BAND_TARGET_PWR];
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, Init Txpower, Tx0:%x, Tx1:%x\n", __FUNCTION__, ATECtrl->TxPower0, ATECtrl->TxPower1));
#ifdef CONFIG_QA
	MtAsicGetRxStat(pAd, HQA_RX_RESET_PHY_COUNT);
	ATECtrl->RxMacMdrdyCount = 0;
	ATECtrl->RxMacFCSErrCount = 0;
#endif /* CONFIG_QA */

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MaxNumBss > MAX_MBSSID_NUM(pAd))
			MaxNumBss = MAX_MBSSID_NUM(pAd);

		/*  first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
		for (IdBss = FIRST_MBSSID; IdBss < MAX_MBSSID_NUM(pAd); IdBss++)
		{
			if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev)
				RTMP_OS_NETDEV_STOP_QUEUE(pAd->ApCfg.MBSSID[IdBss].wdev.if_dev);
		}
	}
#endif

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET);

	AsicSetMacTxRx(pAd, ASIC_MAC_RX, FALSE);

	/*  Disable TX PDMA */
	AsicSetWPDMA(pAd, PDMA_TX, 0);
	
#ifdef CONFIG_AP_SUPPORT
	APStop(pAd);
#endif /* CONFIG_AP_SUPPORT */

#ifdef RTMP_MAC_SDIO
	if_ops->init(pAd);
	if_ops->clean_trx_q(pAd);
#endif /* RTMP_MAC_SDIO */

#ifdef RTMP_MAC_PCI
	if_ops->init(pAd);
	if_ops->clean_trx_q(pAd);
#endif /* RTMP_MAC_PCI */




	AsicSetWPDMA(pAd, PDMA_TX_RX, 1);
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);

	if(ATECtrl->Mode & fATE_TXCONT_ENABLE){
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s ,Stop Continuous Tx\n",__FUNCTION__));	
		MtAsicStopContinousTx(pAd);
	}

	if(ATECtrl->Mode & fATE_TXCARRSUPP_ENABLE){
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s ,Stop Carrier Suppression Test\n",__FUNCTION__));
    	MtAsicSetTxToneTest(pAd, 0, 0);
	}

	ATECtrl->Mode = ATE_START;
	ATECtrl->Mode &= ATE_RXSTOP;
    ATECtrl->TxDoneCount = 0;
    ATECtrl->RxTotalCnt = 0;
	RTMP_OS_INIT_COMPLETION(&ATECtrl->tx_wait);
#ifdef CONFIG_QA
	MtAsicGetRxStat(pAd, HQA_RX_RESET_PHY_COUNT); 
	MtAsicGetRxStat(pAd, HQA_RX_RESET_MAC_COUNT); 
    ATECtrl->RxMacFCSErrCount = 0;
    ATECtrl->RxMacMdrdyCount = 0;
#endif        
	{
		UINT32 Value = 0;
		ATECtrl->rmac_pcr1 = 0;
		MAC_IO_READ32(pAd, AGG_PCR1, &ATECtrl->rmac_pcr1);
		Value = ATECtrl->rmac_pcr1;
		Value &= 0x0FFFFFFF;
		Value |= 0x10000000;
		MAC_IO_WRITE32(pAd, AGG_PCR1, Value);
	}

	{
		UINT32 Value = 0;
		ATECtrl->rmac_pcr = 0;
		MAC_IO_READ32(pAd, AGG_PCR, &ATECtrl->rmac_pcr);
		MAC_IO_WRITE32(pAd, AGG_PCR, Value);
	}

	MtATEWTBL2Update(pAd, 0);

	return Ret;
}


static INT32 MT_ATEStop(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	ATE_IF_OPERATION *if_ops = ATECtrl->ATEIfOps;
	INT32 Ret = 0;
#ifdef CONFIG_AP_SUPPORT
	INT32 IdBss, MaxNumBss = pAd->ApCfg.BssidNum;
#endif


	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
	
	if(	ATECtrl->Mode & ATE_FFT){
		ATE_OPERATION *ATEOp = ATECtrl->ATEOp;
		ATEOp->SetFFTMode(pAd, 0);
		CmdRfTest(pAd, ACTION_SWITCH_TO_RFTEST, OPERATION_NORMAL_MODE, 0);
		mdelay(2000); /* For FW to switch back to normal mode stable time */
	}

#ifdef RTMP_MAC_PCI
	/* Polling TX/RX path until packets empty */
	if(if_ops->clean_trx_q)
		if_ops->clean_trx_q(pAd);
#endif
#ifdef RTMP_MAC_SDIO
	if(if_ops->clean_trx_q)
		if_ops->clean_trx_q(pAd);
#endif /* RTMP_MAC_SDIO */


	AsicSetMacTxRx(pAd, ASIC_MAC_RXV, FALSE);

	NICInitializeAdapter(pAd, TRUE);
#ifdef RTMP_MAC_PCI
	if(if_ops->clean_test_rx_frame)
		if_ops->clean_test_rx_frame(pAd);
#endif /* RTMP_MAC_PCI */

/* if usb  call this two function , FW will hang~~ */
#ifdef RTMP_MAC_PCI
	if_ops->ate_leave(pAd);
#endif /* RTMP_MAC_PCI */

	AsicSetRxFilter(pAd);

	RTMPEnableRxTx(pAd);

	AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
#ifdef CONFIG_AP_SUPPORT
	APStartUp(pAd);
#endif /* CONFIG_AP_SUPPROT  */
	RTMP_OS_NETDEV_START_QUEUE(pAd->net_dev);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MaxNumBss > MAX_MBSSID_NUM(pAd))
			MaxNumBss = MAX_MBSSID_NUM(pAd);

		/*  first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
		for (IdBss = FIRST_MBSSID; IdBss < MAX_MBSSID_NUM(pAd); IdBss++)
		{
			if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev)
				RTMP_OS_NETDEV_START_QUEUE(pAd->ApCfg.MBSSID[IdBss].wdev.if_dev);
		}
	}
#endif

	RTMP_OS_EXIT_COMPLETION(&ATECtrl->cmd_done);

	if ((MTK_REV_GTE(pAd, MT7603, MT7603E1)) ||
		(MTK_REV_GTE(pAd, MT7628, MT7628E1)))
	{
		UINT32 Value;
		RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
		Value &= ~CR_RFINTF_CAL_NSS_MASK;
		Value |= CR_RFINTF_CAL_NSS(0x0);
		RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);  
	}
	{	
		HQA_MPS_CB *mps_cb = &ATECtrl->mps_cb;
		if(mps_cb) NdisFreeSpinLock(&mps_cb->mps_lock);
		RTMP_OS_EXIT_COMPLETION(&ATECtrl->tx_wait);
	}
	MAC_IO_WRITE32(pAd, AGG_PCR1, ATECtrl->rmac_pcr1);
	MAC_IO_WRITE32(pAd, AGG_PCR, ATECtrl->rmac_pcr);
	
	ATECtrl->Mode = ATE_STOP;
	return Ret;
}


#ifdef RTMP_PCI_SUPPORT
static INT32 MT_ATESetupFrame(RTMP_ADAPTER *pAd, UINT32 TxIdx)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QID_AC_BE];
	PUCHAR pDMAHeaderBufVA = (PUCHAR)pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	TXD_STRUC *pTxD;
	MAC_TX_INFO Info;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	PNDIS_PACKET pPacket = NULL;
	HTTRANSMIT_SETTING Transmit;
	TX_BLK TxBlk;
	INT32 Ret = 0;
#ifdef RT_BIG_ENDIAN
	TXD_STRUC *pDestTxD;
	UCHAR TxHwInfo[TXD_SIZE];
#endif /* RT_BIG_ENDIAN */
	ATECtrl->HLen = LENGTH_802_11;

	NdisZeroMemory(&Transmit, sizeof(Transmit));
	NdisZeroMemory(&TxBlk, sizeof(TxBlk));

	/* Fill Mac Tx info */
	NdisZeroMemory(&Info, sizeof(Info));

	/* LMAC queue index (AC0) */
 	Info.q_idx = 1;

	Info.WCID = 0;
	Info.hdr_len = ATECtrl->HLen;
	Info.hdr_pad = 0;

	Info.BM = IS_BM_MAC_ADDR(ATECtrl->Addr1);

	/* no ack */
	Info.Ack = 0;

	Info.bss_idx = 0;

	/*  no frag */
	Info.FRAG = 0;

	/* no protection */
	Info.prot = 0;

	Info.Length = ATECtrl->TxLength;

    /* TX Path setting */
    Info.AntPri = 0;
    Info.SpeEn = 0;
    switch (ATECtrl->TxAntennaSel) {
        case 0: /* Both */
           Info.AntPri = 0;
           Info.SpeEn = 1;
           break;
        case 1: /* TX0 */
           Info.AntPri = 0;
           Info.SpeEn = 0;
           break;
        case 2: /* TX1 */
           Info.AntPri = 2; //b'010
           Info.SpeEn = 0;
           break;
    }

	/* Fill Transmit setting */
	Transmit.field.MCS = ATECtrl->Mcs;
	Transmit.field.BW = ATECtrl->BW;
	Transmit.field.ShortGI = ATECtrl->Sgi;
	Transmit.field.STBC = ATECtrl->Stbc;
	Transmit.field.MODE = ATECtrl->PhyMode;

	if (ATECtrl->PhyMode == MODE_CCK)
	{
		Info.Preamble = LONG_PREAMBLE;

		if (ATECtrl->Mcs == 9)
		{
			Transmit.field.MCS = 0;
			Info.Preamble = SHORT_PREAMBLE;	
		}
		else if (ATECtrl->Mcs == 10)
		{
			Transmit.field.MCS = 1;
			Info.Preamble = SHORT_PREAMBLE;	
		}
		else if (ATECtrl->Mcs == 11)
		{
			Transmit.field.MCS = 2;
			Info.Preamble = SHORT_PREAMBLE;	
		}
	} 

	write_tmac_info(pAd, pDMAHeaderBufVA, &Info, &Transmit);

	NdisMoveMemory(pDMAHeaderBufVA + TXWISize, ATECtrl->TemplateFrame, ATECtrl->HLen);
	NdisMoveMemory(pDMAHeaderBufVA + TXWISize + 4, ATECtrl->Addr1, MAC_ADDR_LEN);
	NdisMoveMemory(pDMAHeaderBufVA + TXWISize + 10, ATECtrl->Addr2, MAC_ADDR_LEN);
	NdisMoveMemory(pDMAHeaderBufVA + TXWISize + 16, ATECtrl->Addr3, MAC_ADDR_LEN);

#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, (((PUCHAR)pDMAHeaderBufVA) + TXWISize), DIR_READ, FALSE);
#endif /* RT_BIG_ENDIAN */

	pPacket = ATEPayloadInit(pAd, TxIdx);

	if (pPacket == NULL)
	{
		ATECtrl->TxCount = 0;
		DBGPRINT_ERR(("%s : fail to init frame payload.\n", __FUNCTION__));
		return -1;
	}
	pTxRing->Cell[TxIdx].pNdisPacket = pPacket;

	pTxD = (TXD_STRUC *)pTxRing->Cell[TxIdx].AllocVa;
#ifndef RT_BIG_ENDIAN
	pTxD = (TXD_STRUC *)pTxRing->Cell[TxIdx].AllocVa;
#else
    pDestTxD  = (TXD_STRUC *)pTxRing->Cell[TxIdx].AllocVa;
	NdisMoveMemory(&TxHwInfo[0], (UCHAR *)pDestTxD, TXD_SIZE);
	pTxD = (TXD_STRUC *)&TxHwInfo[0];
#endif
	TxBlk.SrcBufLen = GET_OS_PKT_LEN(ATECtrl->pAtePacket[TxIdx]);
	TxBlk.pSrcBufData = (PUCHAR)ATECtrl->AteAllocVa[TxIdx];

	NdisZeroMemory(pTxD, TXD_SIZE);
	/* build Tx descriptor */
	pTxD->SDPtr0 = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);
	pTxD->SDLen0 = TXWISize + ATECtrl->HLen;
	pTxD->LastSec0 = 0;
	pTxD->SDPtr1 = PCI_MAP_SINGLE(pAd, &TxBlk, 0, 1, RTMP_PCI_DMA_TODEVICE);
	pTxD->SDLen1 = GET_OS_PKT_LEN(ATECtrl->pAtePacket[TxIdx]);
	pTxD->LastSec1 = 1;
	pTxD->DMADONE = 0;

#ifdef RT_BIG_ENDIAN
	MTMacInfoEndianChange(pAd, pDMAHeaderBufVA, TYPE_TMACINFO, sizeof(TMAC_TXD_L));
	RTMPFrameEndianChange(pAd, (((PUCHAR)pDMAHeaderBufVA) + TXWISize), DIR_WRITE, FALSE);
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
	WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif

	return Ret;
}
#endif




static INT32 MT_ATEStartTx(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	ATE_IF_OPERATION *if_ops = ATECtrl->ATEIfOps;
	UINT32 Value = 0;
	INT32 Ret = 0;
#ifdef CONFIG_AP_SUPPORT
	INT32 IdBss, MaxNumBss = pAd->ApCfg.BssidNum;
#endif
#ifdef RTMP_FLASH_SUPPORT
	USHORT doCal1 = 0, doReload = 0;
#endif /*RTMP_FLASH_SUPPORT*/

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
	/* TxRx switch workaround */
	if(1 == ATECtrl->did_rx){
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(), DID Rx Before\n", __FUNCTION__));
		MT_ATERestoreInit(pAd);
	}
	CmdChannelSwitch(pAd, ATECtrl->ControlChl, ATECtrl->Channel, ATECtrl->BW,
							pAd->CommonCfg.TxStream, pAd->CommonCfg.RxStream, FALSE);
#ifdef RTMP_FLASH_SUPPORT
	rtmp_ee_flash_read(pAd, 0x9F, &doCal1);	
	doReload = (doCal1 & (0x1 << 7)) >> 7;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("reload DPD from flash , 0x9F = [%04x] doReload bit7[%x]\n", doCal1, doReload));
	/* reload DPD cal data from flash  , follow primary channel -by CSD */
	CmdLoadDPDDataFromFlash(pAd, ATECtrl->ControlChl, doReload);  
#endif /* RTMP_FLASH_SUPPORT */

	AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, FALSE);

	/*   Stop send TX packets */
	RTMP_OS_NETDEV_STOP_QUEUE(pAd->net_dev);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MaxNumBss > MAX_MBSSID_NUM(pAd))
			MaxNumBss = MAX_MBSSID_NUM(pAd);

		/*  first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
		for (IdBss = FIRST_MBSSID; IdBss < MAX_MBSSID_NUM(pAd); IdBss++)
		{
			if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev)
				RTMP_OS_NETDEV_STOP_QUEUE(pAd->ApCfg.MBSSID[IdBss].wdev.if_dev);
		}
	}
#endif

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET);

	/*  Disable PDMA */
	AsicSetWPDMA(pAd, PDMA_TX, 0);

	/* Polling TX/RX path until packets empty */
	if(if_ops->clean_trx_q)
		if_ops->clean_trx_q(pAd);

	/* Turn on RX again if set before */
	if (ATECtrl->Mode & ATE_RXFRAME)
		AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, TRUE);

	RTMP_OS_NETDEV_START_QUEUE(pAd->net_dev);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MaxNumBss > MAX_MBSSID_NUM(pAd))
			MaxNumBss = MAX_MBSSID_NUM(pAd);

		/*  first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
		for (IdBss = FIRST_MBSSID; IdBss < MAX_MBSSID_NUM(pAd); IdBss++)
		{
			if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev)
				RTMP_OS_NETDEV_START_QUEUE(pAd->ApCfg.MBSSID[IdBss].wdev.if_dev);
		}
	}
#endif

	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET);
	if(if_ops->setup_frame)
		if_ops->setup_frame(pAd, QID_AC_BE);
	if(if_ops->test_frame_tx){
		if_ops->test_frame_tx(pAd);
		if(ATECtrl->TxCount!=0xFFFFFFFF)
			ATECtrl->TxCount += ATECtrl->TxDoneCount;
		ATECtrl->Mode |= ATE_TXFRAME;
	}else{
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s, no tx test frame callback function\n", __FUNCTION__));
	}

	/* Low temperature high rate EVM degrade Patch v2 */	
	if ((MTK_REV_GTE(pAd, MT7603, MT7603E1)) ||
		(MTK_REV_GTE(pAd, MT7628, MT7628E1)))
	{
		if (ATECtrl->TxAntennaSel == 0)
		{
			RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
			Value &= ~CR_RFINTF_CAL_NSS_MASK;
			Value |= CR_RFINTF_CAL_NSS(0x0);
			RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);  
		}
		else if (ATECtrl->TxAntennaSel == 1)
		{
			RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
			Value &= ~CR_RFINTF_CAL_NSS_MASK;
			Value |= CR_RFINTF_CAL_NSS(0x0);
			RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);  
		}
		else if (ATECtrl->TxAntennaSel == 2)
		{
			RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
			Value &= ~CR_RFINTF_CAL_NSS_MASK;
			Value |= CR_RFINTF_CAL_NSS(0x1);
			RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);  
		}
	}

	ATECtrl->did_tx = 1;
	return Ret;
}



static INT32 MT_ATEStartRx(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	ATE_IF_OPERATION *if_ops = ATECtrl->ATEIfOps;
	INT32 Ret = 0, Value;
	UINT32 reg;
#ifdef CONFIG_AP_SUPPORT
	INT32 IdBss, MaxNumBss = pAd->ApCfg.BssidNum;
#endif

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
	RTMP_IO_READ32(pAd, ARB_SCR,&reg);
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, ARB_SCR:%x\n",__FUNCTION__, reg));
	reg &= ~MT_ARB_SCR_RXDIS;
	RTMP_IO_WRITE32(pAd, ARB_SCR, reg);

	AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, FALSE);



	/*   Stop send TX packets */
	RTMP_OS_NETDEV_STOP_QUEUE(pAd->net_dev);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MaxNumBss > MAX_MBSSID_NUM(pAd))
			MaxNumBss = MAX_MBSSID_NUM(pAd);

		/*  first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
		for (IdBss = FIRST_MBSSID; IdBss < MAX_MBSSID_NUM(pAd); IdBss++)
		{
			if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev)
				RTMP_OS_NETDEV_STOP_QUEUE(pAd->ApCfg.MBSSID[IdBss].wdev.if_dev);
		}
	}
#endif

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET);

	AsicSetWPDMA(pAd, PDMA_TX, 0);

	if(if_ops->clean_trx_q)
		if_ops->clean_trx_q(pAd);

	RTMP_OS_NETDEV_START_QUEUE(pAd->net_dev);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MaxNumBss > MAX_MBSSID_NUM(pAd))
			MaxNumBss = MAX_MBSSID_NUM(pAd);

		/*  first IdBss must not be 0 (BSS0), must be 1 (BSS1) */
		for (IdBss = FIRST_MBSSID; IdBss < MAX_MBSSID_NUM(pAd); IdBss++)
		{
			if (pAd->ApCfg.MBSSID[IdBss].wdev.if_dev)
				RTMP_OS_NETDEV_START_QUEUE(pAd->ApCfg.MBSSID[IdBss].wdev.if_dev);
		}
	}
#endif

	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET);


	/* Turn on TX again if set before */
	if (ATECtrl->Mode & ATE_TXFRAME)
	{
		AsicSetMacTxRx(pAd, ASIC_MAC_TX, TRUE);
	}
    /* reset counter when iwpriv only */
    if (ATECtrl->bQAEnabled != TRUE)
    {
        ATECtrl->RxTotalCnt = 0;
    }
	pAd->WlanCounters.FCSErrorCount.u.LowPart = 0;

	RTMP_IO_READ32(pAd, RMAC_RFCR, &Value);
	Value |= RM_FRAME_REPORT_EN;
	RTMP_IO_WRITE32(pAd, RMAC_RFCR, Value);

	AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, TRUE);

	/* Enable PDMA */
	AsicSetWPDMA(pAd, PDMA_TX_RX, 1);


	ATECtrl->Mode |= ATE_RXFRAME;


	ATECtrl->did_rx = 1;
	return Ret;
}


static INT32 MT_ATEStopTx(RTMP_ADAPTER *pAd, UINT32 Mode)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	ATE_IF_OPERATION *if_ops = ATECtrl->ATEIfOps;
	INT32 Ret = 0;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

	if (Mode == ATE_TXCARR)
	{



	}
	else if (Mode == ATE_TXCARRSUPP)
	{


	}
	else if ((Mode & ATE_TXFRAME) || (Mode == ATE_STOP))
	{
		if (Mode == ATE_TXCONT)
		{


		}

		/*  Disable PDMA */
		AsicSetWPDMA(pAd, PDMA_TX_RX, 0);
	if(if_ops->clean_trx_q)
		if_ops->clean_trx_q(pAd);
	ATECtrl->Mode &= ~ATE_TXFRAME;


#ifdef RTMP_MAC_SDIO
	if(if_ops->clean_trx_q)
		if_ops->clean_trx_q(pAd);
#endif /* RTMP_MAC_SDIO */

		ATECtrl->Mode &= ~ATE_TXFRAME;

		/* Enable PDMA */
		AsicSetWPDMA(pAd, PDMA_TX_RX, 1);
	}

	return Ret;
}


static INT32 MT_ATEStopRx(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;
	UINT32 reg;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

	Ret = AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, FALSE);

	RTMP_IO_READ32(pAd, ARB_SCR,&reg);
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, ARB_SCR:%x\n",__FUNCTION__, reg));
	reg |= MT_ARB_SCR_RXDIS;
	RTMP_IO_WRITE32(pAd, ARB_SCR, reg);

    ATECtrl->Mode &= ~ATE_RXFRAME;


	return Ret;
}


static INT32 MT_ATESetTxAntenna(RTMP_ADAPTER *pAd, CHAR Ant)
{

	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;
	UINT32 Value;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
    
	/* 0: All 1:TX0 2:TX1 */
    ATECtrl->TxAntennaSel = Ant;

	if ((MTK_REV_GTE(pAd, MT7603, MT7603E1)) ||
		(MTK_REV_GTE(pAd, MT7628, MT7628E1)))
	{
		if (ATECtrl->TxAntennaSel == 0)
		{
			RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
			Value &= ~CR_RFINTF_CAL_NSS_MASK;
			Value |= CR_RFINTF_CAL_NSS(0x0);
			RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);
			/* Tx both patch, ePA/eLNA/, iPA/eLNA, iPA/iLNA */
			{

				UINT32 RemapBase, RemapOffset;
				UINT32 RestoreValue;
				UINT32 value;
				RTMP_IO_READ32(pAd, MCU_PCIE_REMAP_2, &RestoreValue);
				RemapBase = GET_REMAP_2_BASE(0x81060008) << 19;
				RemapOffset = GET_REMAP_2_OFFSET(0x81060008);
				RTMP_IO_WRITE32(pAd, MCU_PCIE_REMAP_2, RemapBase);
				if ((MTK_REV_GTE(pAd, MT7603, MT7603E1))||
					(MTK_REV_GTE(pAd, MT7603, MT7603E2))){
					value = 0x04852390;
					RTMP_IO_WRITE32(pAd, 0x80000 + RemapOffset, value);
				}else if(MTK_REV_GTE(pAd, MT7628, MT7628E1)){
					value = 0x00489523;
					RTMP_IO_WRITE32(pAd, 0x80000 + RemapOffset, value);
				}
				RTMP_IO_WRITE32(pAd, MCU_PCIE_REMAP_2, RestoreValue);

			}
		}
		else if (ATECtrl->TxAntennaSel == 1)
		{
			RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
			Value &= ~CR_RFINTF_CAL_NSS_MASK;
			Value |= CR_RFINTF_CAL_NSS(0x0);
			RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);

			/* Tx0 patch, ePA/eLNA/, iPA/eLNA, iPA/iLNA */
			{
				UINT32 RemapBase, RemapOffset;
				UINT32 RestoreValue;
				UINT32 value;
				RTMP_IO_READ32(pAd, MCU_PCIE_REMAP_2, &RestoreValue);
				RemapBase = GET_REMAP_2_BASE(0x81060008) << 19;
				RemapOffset = GET_REMAP_2_OFFSET(0x81060008);
				RTMP_IO_WRITE32(pAd, MCU_PCIE_REMAP_2, RemapBase);
				if ((MTK_REV_GTE(pAd, MT7603, MT7603E1))||
					(MTK_REV_GTE(pAd, MT7603, MT7603E2))){
					value = 0x04852390;
					RTMP_IO_WRITE32(pAd, 0x80000 + RemapOffset, value);
				}else if(MTK_REV_GTE(pAd, MT7628, MT7628E1)){
					value = 0x00489523;
					RTMP_IO_WRITE32(pAd, 0x80000 + RemapOffset, value);
				}
				RTMP_IO_WRITE32(pAd, MCU_PCIE_REMAP_2, RestoreValue);
			}
		}
		else if (ATECtrl->TxAntennaSel == 2)
		{
			RTMP_IO_READ32(pAd, CR_RFINTF_00, &Value);
			Value &= ~CR_RFINTF_CAL_NSS_MASK;
			Value |= CR_RFINTF_CAL_NSS(0x1);
			RTMP_IO_WRITE32(pAd, CR_RFINTF_00, Value);
			/* Tx1 patch, ePA/eLNA/, iPA/eLNA, iPA/iLNA */
			{
				UINT32 RemapBase, RemapOffset;
				UINT32 RestoreValue;
				UINT32 value;
				RTMP_IO_READ32(pAd, MCU_PCIE_REMAP_2, &RestoreValue);
				RemapBase = GET_REMAP_2_BASE(0x81060008) << 19;
				RemapOffset = GET_REMAP_2_OFFSET(0x81060008);
				RTMP_IO_WRITE32(pAd, MCU_PCIE_REMAP_2, RemapBase);
				if ((MTK_REV_GTE(pAd, MT7603, MT7603E1))||
					(MTK_REV_GTE(pAd, MT7603, MT7603E2))){
					value = 0x04856790;
					RTMP_IO_WRITE32(pAd, 0x80000 + RemapOffset, value);
				}else if(MTK_REV_GTE(pAd, MT7628, MT7628E1)){
					value = 0x00489567;
					RTMP_IO_WRITE32(pAd, 0x80000 + RemapOffset, value);
				}	
				RTMP_IO_WRITE32(pAd, MCU_PCIE_REMAP_2, RestoreValue);
			}
		}
	}

	return Ret;
}


static INT32 MT_ATESetRxAntenna(RTMP_ADAPTER *pAd, CHAR Ant)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
    ATECtrl->RxAntennaSel = Ant;
    /* set RX path */
    MtAsicSetRxPath(pAd, (UINT32)ATECtrl->RxAntennaSel);
	return Ret;
}


static INT32 MT_ATESetTxFreqOffset(RTMP_ADAPTER *pAd, UINT32 FreqOffset)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;
    ATECtrl->RFFreqOffset = FreqOffset;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

    MtAsicSetRfFreqOffset(pAd, ATECtrl->RFFreqOffset);

	return Ret;
}

static INT32 MT_ATESetChannel(RTMP_ADAPTER *pAd, INT16 Value)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;
#ifdef RTMP_FLASH_SUPPORT
	USHORT doCal1 = 0, doReload = 0;
#endif /*RTMP_FLASH_SUPPORT*/

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

	CmdChannelSwitch(pAd, ATECtrl->ControlChl, ATECtrl->Channel, ATECtrl->BW,
							pAd->CommonCfg.TxStream, pAd->CommonCfg.RxStream, FALSE);
#ifdef RTMP_FLASH_SUPPORT
	rtmp_ee_flash_read(pAd, 0x9F, &doCal1);	
	doReload = (doCal1 & (0x1 << 7)) >> 7;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("reload DPD from flash , 0x9F = [%04x] doReload bit7[%x]\n", doCal1, doReload));
	/* reload DPD cal data from flash  , follow primary channel -by CSD */
	CmdLoadDPDDataFromFlash(pAd, ATECtrl->ControlChl, doReload);  
#endif /* RTMP_FLASH_SUPPORT */

	return Ret;
}


static INT32 MT_ATESetBW(RTMP_ADAPTER *pAd, INT16 Value)
{
	INT32 Ret = 0;
    UINT32 val = 0;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));

	RTMP_IO_READ32(pAd, AGG_BWCR, &val);
	val &= (~0x0c);
	switch (Value)
	{
		case BW_20:
			val |= (0);
			break;
		case BW_40:
			val |= (0x1 << 2);
			break;
		case BW_80:
			val |= (0x2 << 2);
			break;
	}
	RTMP_IO_WRITE32(pAd, AGG_BWCR, val);
    /* TODO: check CMD_CH_PRIV_ACTION_BW_REQ */
	//CmdChPrivilege(pAd, CMD_CH_PRIV_ACTION_BW_REQ, ATECtrl->ControlChl, ATECtrl->Channel,
	//					ATECtrl->BW, pAd->CommonCfg.TxStream, pAd->CommonCfg.RxStream);


	return Ret;
}


static INT32 MT_ATESampleRssi(RTMP_ADAPTER *pAd, RX_BLK *RxBlk)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;

	if (RxBlk->rx_signal.raw_rssi[0] != 0)
	{
		ATECtrl->LastRssi0	= ConvertToRssi(pAd,
			(struct raw_rssi_info *)(&RxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_0);

		ATECtrl->AvgRssi0X8 = (ATECtrl->AvgRssi0X8 - ATECtrl->AvgRssi0)
									+ ATECtrl->LastRssi0;
		ATECtrl->AvgRssi0 = ATECtrl->AvgRssi0X8 >> 3;
	}

	if (RxBlk->rx_signal.raw_rssi[1] != 0)
	{
		ATECtrl->LastRssi1 = ConvertToRssi(pAd,
			(struct raw_rssi_info *)(&RxBlk->rx_signal.raw_rssi[0]), RSSI_IDX_1);

		ATECtrl->AvgRssi1X8 = (ATECtrl->AvgRssi1X8 - ATECtrl->AvgRssi1)
									+ ATECtrl->LastRssi1;
		ATECtrl->AvgRssi1 = ATECtrl->AvgRssi1X8 >> 3;
	}

	ATECtrl->LastSNR0 = RxBlk->rx_signal.raw_snr[0];;
	ATECtrl->LastSNR1 = RxBlk->rx_signal.raw_snr[1];

	ATECtrl->NumOfAvgRssiSample++;

	return Ret;
}

static INT32 MT_ATESetAIFS(RTMP_ADAPTER *pAd, CHAR Value)
{
	INT32 Ret = 0;
	UINT val = Value & 0x000000ff;
    /* Test mode use AC0 for TX */
    MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, Value:%x\n",__FUNCTION__, val));
	AsicSetWmmParam(pAd, WMM_PARAM_AC_0, WMM_PARAM_AIFSN, val);
    return Ret;
}

static INT32 MT_ATESetTSSI(RTMP_ADAPTER *pAd, CHAR WFSel, CHAR Setting)
{
	INT32 Ret = 0;
    Ret = MtAsicSetTSSI(pAd, Setting, WFSel);
	return Ret;
}

static INT32 MT_ATESetDPD(RTMP_ADAPTER *pAd, CHAR WFSel, CHAR Setting)
{
    /* !!TEST MODE ONLY!! Normal Mode control by FW and Never disable */
    /* WF0 = 0, WF1 = 1, WF ALL = 2 */
	INT32 Ret = 0;
    Ret = MtAsicSetDPD(pAd, Setting, WFSel);
    return Ret;
}

static INT32 MT_ATEStartTxTone(RTMP_ADAPTER *pAd, UINT32 Mode)
{
	INT32 Ret = 0;
    MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
	MtAsicSetTxToneTest(pAd, 1, Mode);
	return Ret;
}

static INT32 MT_ATESetTxTonePower(RTMP_ADAPTER *pAd, INT32 pwr1, INT32 pwr2)
{
	INT32 Ret = 0;
    MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, pwr1:%d, pwr2:%d\n", __FUNCTION__, pwr1, pwr2));
	MtAsicSetTxTonePower(pAd, pwr1, pwr2);
	return Ret;
}

static INT32 MT_ATEStopTxTone(RTMP_ADAPTER *pAd)
{
	INT32 Ret = 0;
    MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
	MtAsicSetTxToneTest(pAd, 0, 0);
    return Ret;
}

static INT32 MT_ATEStartContinousTx(RTMP_ADAPTER *pAd, CHAR WFSel)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	INT32 Ret = 0;
    MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s\n", __FUNCTION__));
    MtAsicStartContinousTx(pAd, ATECtrl->PhyMode, ATECtrl->BW, ATECtrl->ControlChl, ATECtrl->Mcs, WFSel);
    return Ret;
}


static INT32 MT_RfRegWrite(RTMP_ADAPTER *pAd, UINT32 WFSel, UINT32 Offset, UINT32 Value)
{
	INT32 Ret = 0;

	Ret = CmdRFRegAccessWrite(pAd, WFSel, Offset, Value);

	return Ret;
}


static INT32 MT_RfRegRead(RTMP_ADAPTER *pAd, UINT32 WFSel, UINT32 Offset, UINT32 *Value)
{
	INT32 Ret = 0;

	Ret = CmdRFRegAccessRead(pAd, WFSel, Offset, Value);

	return Ret;
}


static INT32 MT_GetFWInfo(RTMP_ADAPTER *pAd, UCHAR *FWInfo)
{

	RTMP_CHIP_CAP *cap = &pAd->chipCap;

	memcpy(FWInfo, cap->FWImageName + cap->fw_len - 36, 36);

	return 0;
}

static INT32 MT_SetFFTMode(struct _RTMP_ADAPTER *pAd, UINT32 mode)
{
	ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	INT32 Ret = 0;
	UINT32 Value = 0;
	static UINT32 RFCR, OMA0R0, OMA0R1, OMA1R0, OMA1R1, OMA2R0, OMA2R1, OMA3R0, OMA3R1, OMA4R0, OMA4R1; 

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s: %d\n", __FUNCTION__, mode));

	if (mode == 0)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("mode == 0 @@@@@@@@\n"));
		RTMP_IO_WRITE32(pAd, RMAC_RFCR, RFCR);		
		RTMP_IO_WRITE32(pAd, RMAC_OMA0R0, OMA0R0);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA0R1, OMA0R1);	

		RTMP_IO_WRITE32(pAd, RMAC_OMA1R0, OMA1R0);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA1R1, OMA1R1);

		RTMP_IO_WRITE32(pAd, RMAC_OMA2R0, OMA2R0);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA2R1, OMA2R1);

		RTMP_IO_WRITE32(pAd, RMAC_OMA3R0, OMA3R0);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA3R1, OMA3R1);

		RTMP_IO_WRITE32(pAd, RMAC_OMA4R0, OMA4R0);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA4R1, OMA4R1);

		AsicSetWPDMA(pAd, PDMA_TX_RX, 1);
		AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, FALSE);
	}
	else
	{
		//backup CR value
		RTMP_IO_READ32(pAd, RMAC_RFCR, &RFCR);		
		RTMP_IO_READ32(pAd, RMAC_OMA0R0, &OMA0R0);	
		RTMP_IO_READ32(pAd, RMAC_OMA0R1, &OMA0R1);	

		RTMP_IO_READ32(pAd, RMAC_OMA1R0, &OMA1R0);	
		RTMP_IO_READ32(pAd, RMAC_OMA1R1, &OMA1R1);

		RTMP_IO_READ32(pAd, RMAC_OMA2R0, &OMA2R0);	
		RTMP_IO_READ32(pAd, RMAC_OMA2R1, &OMA2R1);

		RTMP_IO_READ32(pAd, RMAC_OMA3R0, &OMA3R0);	
		RTMP_IO_READ32(pAd, RMAC_OMA3R1, &OMA3R1);

		RTMP_IO_READ32(pAd, RMAC_OMA4R0, &OMA4R0);	
		RTMP_IO_READ32(pAd, RMAC_OMA4R1, &OMA4R1);

		RTMP_IO_READ32(pAd, RMAC_RFCR, &Value);
		Value |= RM_FRAME_REPORT_EN;
		RTMP_IO_WRITE32(pAd, RMAC_RFCR, Value);

		//set RMAC don't let packet go up
		RTMP_IO_WRITE32(pAd, RMAC_RFCR, 0x001FEFFB);		
		RTMP_IO_WRITE32(pAd, RMAC_OMA0R0, 0x00000000);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA0R1, 0x00000000);	

		RTMP_IO_WRITE32(pAd, RMAC_OMA1R0, 0x00000000);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA1R1, 0x00000000);

		RTMP_IO_WRITE32(pAd, RMAC_OMA2R0, 0x00000000);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA2R1, 0x00000000);

		RTMP_IO_WRITE32(pAd, RMAC_OMA3R0, 0x00000000);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA3R1, 0x00000000);

		RTMP_IO_WRITE32(pAd, RMAC_OMA4R0, 0x00000000);	
		RTMP_IO_WRITE32(pAd, RMAC_OMA4R1, 0x00000000);
		if(!(ATECtrl->Mode&ATE_FFT)){
			if (IS_MT7603(pAd) || IS_MT7628(pAd))
			{
				CmdIcapOverLap(pAd, 80);
			}
			if (IS_MT7636(pAd))
			{
				CmdIcapOverLap(pAd, 120);
			}
			ATECtrl->Mode |= ATE_FFT;
		}
		AsicSetWPDMA(pAd, PDMA_TX_RX, 0);

		AsicSetMacTxRx(pAd, ASIC_MAC_RX_RXV, TRUE);
	}
	return Ret;
}

static INT32 MT_MPSSetParm(RTMP_ADAPTER *pAd, enum _MPS_PARAM_TYPE type ,UINT32 band_idx, INT32 items, UINT32 *data)
{
	INT32 ret = 0;
	INT32 i = 0;
	ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	HQA_MPS_CB *mps_cb = &ATECtrl->mps_cb;
	HQA_MPS_SETTING *mps_setting = NULL;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s, band_idx:%u, items:%d, Mode:%x, mps_cb:%p, mps_set:%p\n", __FUNCTION__, band_idx, items, ATECtrl->Mode, mps_cb, mps_setting));
	
	if((items>1024) || (items == 0))
		goto MT_MPSSetParm_RET_FAIL;
	if(pAd->ATECtrl.Mode & fATE_MPS)
		goto MT_MPSSetParm_RET_FAIL;
	if(!mps_cb->mps_setting && !mps_cb->mps_cnt){
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s, alloc mps param, itmes:%u, mps_set_addr:%p\n", __FUNCTION__, items, &mps_cb->mps_setting));
		mps_cb->mps_cnt = items;
		ret = os_alloc_mem(pAd, (UCHAR **)&mps_cb->mps_setting, sizeof(HQA_MPS_SETTING)*(items+1));
		if(ret == NDIS_STATUS_FAILURE)
			goto MT_MPSSetParm_RET_FAIL;
		NdisZeroMemory(mps_cb->mps_setting, sizeof(HQA_MPS_SETTING)*(items+1));
	}
	
	mps_setting = mps_cb->mps_setting;

	switch(type){
	case MPS_SEQDATA:
 		mps_setting[0].phy = 1;
		for(i=0;i<items;i++)
    		mps_setting[i+1].phy = data[i];
		break;
	case MPS_PHYMODE:
 		mps_setting[0].phy = 1;
		for(i=0;i<items;i++){
			mps_setting[i+1].phy &= 0xf0ffffff;
    		mps_setting[i+1].phy |= (data[i]<<24)&0xf0ffffff;
		}
		break;
	case MPS_PATH:
 		mps_setting[0].phy = 1;
		for(i=0;i<items;i++){
			mps_setting[i+1].phy &= 0xff0000ff;
    		mps_setting[i+1].phy |= (data[i]<<8)&0xff0000ff;
		}
		break;
	case MPS_RATE:
 		mps_setting[0].phy = 1;
		for(i=0;i<items;i++){
			mps_setting[i+1].phy &= 0xffffff00;
    		mps_setting[i+1].phy |= (0xffffff00&data[i]);
		}
		break;
	case MPS_PAYLOAD_LEN:
 		mps_setting[0].pkt_len = 1;
		for(i=0;i<items;i++){
			if(data[i] > MAX_TEST_PKT_LEN)
				data[i] = MAX_TEST_PKT_LEN;
			else if(data[i] < MIN_TEST_PKT_LEN)
				data[i] = MIN_TEST_PKT_LEN;
    		mps_setting[i+1].pkt_len = data[i];
		}
		break;
	case MPS_TX_COUNT:
 		mps_setting[0].pkt_cnt = 1;
		for(i=0;i<items;i++)
    		mps_setting[i+1].pkt_cnt = data[i];
		break;
	case MPS_PWR_GAIN:
 		mps_setting[0].pwr = 1;
		for(i=0;i<items;i++)
    		mps_setting[i+1].pwr = data[i];
		break;
	default:
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s, unknown setting type\n", __FUNCTION__));
		break;
	}

	return ret; 
MT_MPSSetParm_RET_FAIL:
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR,("%s-fail, band_idx:%u, items:%d, Mode:%x\n", __FUNCTION__, band_idx, items, ATECtrl->Mode));
	return NDIS_STATUS_FAILURE;
}

static INT MtMPSThread(IN ULONG Context)
{

	RTMP_OS_TASK *pTask = (RTMP_OS_TASK *)Context;
	RTMP_ADAPTER *pAd = (PRTMP_ADAPTER)RTMP_OS_TASK_DATA_GET(pTask);
	ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	ATE_OPERATION *ATEOp = ATECtrl->ATEOp;
	HQA_MPS_CB *mps_cb = &ATECtrl->mps_cb;
	HQA_MPS_SETTING *mps_setting = mps_cb->mps_setting;
	UINT32 mps_cnt = mps_cb->mps_cnt;
	INT32 i = 1; /* 0: for init sanity check, 1: start from 1 */	
	INT32 Ret = 0;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s:\n", __FUNCTION__));
	
	if(!mps_cnt || !mps_setting ||
		!((mps_setting[0].phy==1)&&(mps_setting[0].pkt_len==1)&&(mps_setting[0].pkt_cnt==1)&&(mps_setting[0].pwr==1))) {
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s:mps_cnt: %u, %u, %u, %u, %u, error occur when init parameter\n", __FUNCTION__, mps_cnt, mps_setting[0].phy, mps_setting[0].pkt_len, mps_setting[0].pkt_cnt, mps_setting[0].pwr));
		goto MPS_THREAD_END;
	}
	while((i <= mps_cnt) && (ATECtrl->Mode & fATE_MPS)){
		/* Set Paramter */
		ATECtrl->PhyMode = (mps_setting[i].phy&0x0f000000)>>24;
		ATECtrl->TxAntennaSel = (mps_setting[i].phy&0x00ffff00)>>8;
		ATECtrl->Mcs = (mps_setting[i].phy&0x000000ff);
		ATECtrl->TxLength = mps_setting[i].pkt_len;
		ATECtrl->TxCount = mps_setting[i].pkt_cnt;
		Ret = ATEOp->SetTxPower0(pAd, mps_setting[i].pwr);
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%d: PhyMode:%x, TxAnt:0x%x, Mcs:0x%x ", i, ATECtrl->PhyMode, ATECtrl->TxAntennaSel, ATECtrl->Mcs));
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("cnt:%u, len:%u, pwr:0x%x\n", mps_setting[i].pkt_cnt, mps_setting[i].pkt_len, mps_setting[i].pwr));
		/* Start Tx */
		Ret = ATEOp->StartTx(pAd);
		/* Wait */
		RTMP_OS_WAIT_FOR_COMPLETION_TIMEOUT(&ATECtrl->tx_wait, 3000);
		/* No need stop Tx */
		ATEOp->StopTx(pAd, ATECtrl->Mode);
		i++;
	}
MPS_THREAD_END:
	if(mps_setting){
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_OFF,("%s:mps_cnt: %u, finished:%d, free mps_setting\n", __FUNCTION__, mps_cnt, i-1));
		os_free_mem(pAd, mps_setting);
	}
	mps_cb->mps_setting = NULL;
	mps_cb->mps_cnt = 0;
	mps_cb->setting_inuse = FALSE;
	ATECtrl->Mode = ATE_START;
	return Ret;
}

static INT32 MT_MPSTxStart(RTMP_ADAPTER *pAd, UINT32 band_idx)
{
	ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	HQA_MPS_CB *mps_cb = &ATECtrl->mps_cb;
	HQA_MPS_SETTING *mps_setting = mps_cb->mps_setting;
	RTMP_OS_TASK *pTask = &mps_cb->mps_task;
	UINT32 mps_cnt = mps_cb->mps_cnt;
	INT32 Ret = 0;

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s, band_idx:%u, items:%u\n", __FUNCTION__,band_idx, mps_cb->mps_cnt));
	
	if(!mps_setting || !mps_cnt ||(pAd->ATECtrl.Mode & ATE_MPS))
		goto MPS_START_ERR;
	if(mps_cb->setting_inuse)
		goto MPS_START_ERR;

	ATECtrl->Mode |= fATE_MPS;
	mps_cb->setting_inuse = TRUE;
	mps_cb->band_idx = band_idx;
	Ret = SetATEMPSDump(pAd, "1");
	RTMP_OS_TASK_INIT(pTask, "MPSThread", pAd);
	Ret = RtmpOSTaskAttach(pTask, MtMPSThread, (ULONG)pTask);
	if (Ret == NDIS_STATUS_FAILURE) {
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: create mps thread failed\n", __FUNCTION__));
		goto MPS_START_ERR;
	}
	return Ret;
MPS_START_ERR:
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR,("%s error, mode:0x%x, cnt:%u, MPS_SETTING: %p, inuse:%x\n", __FUNCTION__,	pAd->ATECtrl.Mode, mps_cnt, mps_setting, mps_cb->setting_inuse));
	return Ret;
}

static INT32 MT_MPSTxStop(RTMP_ADAPTER *pAd, UINT32 band_idx)
{
	INT32 ret = 0;
	ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	HQA_MPS_CB *mps_cb = &ATECtrl->mps_cb;
	HQA_MPS_SETTING *mps_setting = mps_cb->mps_setting;
	
	if(!(ATECtrl->Mode&ATE_MPS) && mps_setting){
		mps_cb->mps_cnt = 0;
		mps_cb->setting_inuse = FALSE;
		os_free_mem(pAd, mps_setting);
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_OFF,("%s, free mem\n", __FUNCTION__));
		ATECtrl->Mode &= ~ATE_MPS;
	}

	ATECtrl->Mode &= ~ATE_TXFRAME;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s, band_idx:%u, Mode:%x\n", __FUNCTION__,band_idx, ATECtrl->Mode));
	return ret;
}

static INT32 MT_ATESetAutoResp(RTMP_ADAPTER *pAd, UINT32 band_idx, UCHAR *mac, UCHAR mode)
{
	INT32 ret = 0;
	ATE_CTRL *ATECtrl = &(pAd->ATECtrl);
	UCHAR *sa = NULL;

#ifdef CONFIG_AP_SUPPORT
	sa = ATECtrl->Addr3;
#elif defined(CONFIG_STA_SUPPORT) 
	sa = ATECtrl->Addr2;
#endif

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s\n", __FUNCTION__));
	if(mode){
		if(sa)
			NdisMoveMemory(sa, mac, MAC_ADDR_LEN);
		AsicSetDevMac(pAd, mac, 0);	
	}else
		AsicSetDevMac(pAd, pAd->CurrentAddress, 0x0);
	return ret;
}

#ifdef RTMP_MAC_SDIO
static INT32 sdio_ate_init(RTMP_ADAPTER *pAd){
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UINT32 reg;
	
	RTMP_SDIO_WRITE32(pAd, WHLPCR, W_INT_EN_SET);
	RTMP_SDIO_READ32(pAd, WHLPCR, &reg);
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<-- %s, own back 0x%08x\n", __FUNCTION__,reg));
	os_alloc_mem(pAd, (PUCHAR *)&ATECtrl->pAtePacket, (2048));
	
	return NDIS_STATUS_SUCCESS;
}

static INT32 sdio_clean_q (RTMP_ADAPTER *pAd){
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
    P_TX_CTRL_T prTxCtrl = &pAd->rTxCtrl;
	UCHAR *pucOutputBuf = NULL;
	UINT32 reg;

	/* Polling TX/RX path until packets empty usb need ??*/
	ATECtrl->tx_pg = 0;
	rtmp_tx_swq_exit(pAd, WCID_ALL);
    pucOutputBuf = prTxCtrl->pucTxCoalescingBufPtr;
	NdisZeroMemory(pucOutputBuf, MAX_AGGREGATION_SIZE);
	if(ATECtrl->pAtePacket)
		RTMPZeroMemory(ATECtrl->pAtePacket, 2048);
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s -->\n", __FUNCTION__));
	return NDIS_STATUS_SUCCESS;
}

static INT32 sdio_setup_frame(RTMP_ADAPTER *pAd, UINT32 q_idx){
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	ATE_IF_OPERATION *if_ops = ATECtrl->ATEIfOps;
	TXINFO_STRUC *pTxInfo;
	TXWI_STRUC *pTxWI;
	UCHAR *buf;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	MAC_TX_INFO mac_info;
	HTTRANSMIT_SETTING Transmit;
	ATECtrl->HLen = LENGTH_802_11;
	//ATECtrl->TxLength = 24;

	NdisZeroMemory(&pAd->NullFrame, 24);
	NdisZeroMemory(&Transmit, sizeof(Transmit));
	pAd->NullFrame.FC.Type = FC_TYPE_DATA;
	pAd->NullFrame.FC.SubType = SUBTYPE_DATA_NULL;
	pAd->NullFrame.FC.ToDs = 1;
	
	COPY_MAC_ADDR(pAd->NullFrame.Addr1, ATECtrl->Addr1);
	COPY_MAC_ADDR(pAd->NullFrame.Addr2, ATECtrl->Addr2);
	COPY_MAC_ADDR(pAd->NullFrame.Addr3, ATECtrl->Addr3);

		
	buf = ATECtrl->pAtePacket;
	RTMPZeroMemory(buf, 2048);
	
	pTxInfo = (TXINFO_STRUC *)buf;
	NdisZeroMemory((UCHAR *)&mac_info, sizeof(mac_info));
	mac_info.FRAG = FALSE;
	mac_info.Ack = FALSE;
	mac_info.NSeq = FALSE;
	mac_info.hdr_len = ATECtrl->HLen;
	mac_info.hdr_pad = 0;
	mac_info.WCID = 0;
	mac_info.Length = ATECtrl->TxLength;//24;//ATECtrl->TxLength;
	mac_info.PID = 0;
	mac_info.bss_idx = 0;
	mac_info.TID = 0;
	mac_info.prot = 0;
	switch (ATECtrl->TxAntennaSel) {
	case 0: /* Both */
		mac_info.AntPri = 0;
	    mac_info.SpeEn = 1;
		break;
	case 1: /* TX0 */
		mac_info.AntPri = 0;
		mac_info.SpeEn = 0;
		break;
	case 2: /* TX1 */
		mac_info.AntPri = 2; //b'010
		mac_info.SpeEn = 0;
		break;
	}
	Transmit.field.MCS = ATECtrl->Mcs;
	Transmit.field.BW = ATECtrl->BW;
	Transmit.field.ShortGI = ATECtrl->Sgi;
	Transmit.field.STBC = ATECtrl->Stbc;
	Transmit.field.MODE = ATECtrl->PhyMode;
		
	mac_info.q_idx = Q_IDX_AC0;
	mac_info.SpeEn = 1;
	
	if (ATECtrl->PhyMode == MODE_CCK){
		mac_info.Preamble = LONG_PREAMBLE;
		if (ATECtrl->Mcs == 9){
			Transmit.field.MCS = 0;
			mac_info.Preamble = SHORT_PREAMBLE;	
		}else if (ATECtrl->Mcs == 10){
			Transmit.field.MCS = 1;
			mac_info.Preamble = SHORT_PREAMBLE;	
		}else if (ATECtrl->Mcs == 11){
			Transmit.field.MCS = 2;
			mac_info.Preamble = SHORT_PREAMBLE;	
		}
	}
	write_tmac_info(pAd, (UCHAR *)buf, &mac_info, &Transmit);
	RTMPMoveMemory((VOID *)&buf[sizeof(TMAC_TXD_L)], (VOID *)&pAd->NullFrame, ATECtrl->TxLength);
	return NDIS_STATUS_SUCCESS;
}
static INT32 sdio_test_frame_tx(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
    P_TX_CTRL_T prTxCtrl = &pAd->rTxCtrl;
	UINT32 pkt_len = sizeof(TMAC_TXD_L) + ATECtrl->TxLength;
	UCHAR *pPacket = ATECtrl->pAtePacket;
	UCHAR *pucOutputBuf = NULL;
	UINT32 reg = 0;
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<-- %s, pkt_len:%u, TMAC_TXD_L:%u, TxDoneCount:%u\n", __FUNCTION__,pkt_len,sizeof(TMAC_TXD_L),pAd->ATECtrl.TxDoneCount));
	pAd->ATECtrl.TxDoneCount = 0;
		if(!pPacket && !prTxCtrl->pucTxCoalescingBufPtr)
		goto sdio_test_frame_tx_err;
	
	pucOutputBuf = prTxCtrl->pucTxCoalescingBufPtr;
	NdisMoveMemory(pucOutputBuf, pPacket, pkt_len);
	pucOutputBuf += pkt_len;

	{	/* TODO: Fix it after TxDone from HIF OK*/
		INT32 ret = 0;
		UINT16 u2PgCnt;
		u2PgCnt = MTSDIOTxGetPageCount((pAd->ATECtrl.TxLength + TXWISize), TRUE);
        MTSDIOTxAcquireResource(pAd, QID_AC_BK, u2PgCnt);
		ATECtrl->tx_pg = u2PgCnt;
		ret = MTSDIOMultiWrite(pAd, WTDR1, prTxCtrl->pucTxCoalescingBufPtr,pkt_len);

		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Response from bus driver:%d!!\n",ret));
		//MTSDIOWorkerThreadEventNotify(pAd, SDIO_EVENT_INT_BIT);
	}
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s -->, \n", __FUNCTION__));

sdio_test_frame_tx_err:
	return NDIS_STATUS_SUCCESS;
}

static INT32 sdio_clean_test_rx_frame(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	PNDIS_PACKET prRxPacket = pAd->SDIORxPacket;
	if(prRxPacket)
		RELEASE_NDIS_PACKET(pAd, prRxPacket, NDIS_STATUS_SUCCESS);
	return NDIS_STATUS_SUCCESS;
}

#endif
#ifdef RTMP_MAC_PCI
static INT32 pci_ate_init(RTMP_ADAPTER *pAd){
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UINT32 Index, i;
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QID_AC_BE];
	PNDIS_PACKET pPacket;

	MTWF_LOG(DBG_CAT_TEST,  DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s, \n", __FUNCTION__));
	RTMP_IO_READ32(pAd, pTxRing->hw_didx_addr, &pTxRing->TxDmaIdx);
	pTxRing->TxSwFreeIdx = pTxRing->TxDmaIdx;
	pTxRing->TxCpuIdx = pTxRing->TxDmaIdx;
	RTMP_IO_WRITE32(pAd, pTxRing->hw_cidx_addr, pTxRing->TxCpuIdx);

	if (ATECtrl->allocated == 0){
		for (Index = 0; Index < TX_RING_SIZE; Index++) {
			if (ATEPayloadAlloc(pAd, Index) != (NDIS_STATUS_SUCCESS)) {
				ATECtrl->allocated = 0;
				goto pci_ate_init_err;
			}
			pTxRing->Cell[Index].pNdisPacket = ATECtrl->pAtePacket[Index];
		}
		ATECtrl->allocated = 1;
	}
	RTMP_ASIC_INTERRUPT_ENABLE(pAd);
	return NDIS_STATUS_SUCCESS;
pci_ate_init_err:
	MTWF_LOG(DBG_CAT_TEST,  DBG_SUBCAT_ALL, DBG_LVL_ERROR,("%s, Allocate test packet fail at pakcet%d\n", __FUNCTION__, Index));
	for (i = 0; i < Index; i++) {
		pPacket = pTxRing->Cell[Index].pNdisPacket;
		if (pPacket)
		{
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			pTxRing->Cell[Index].pNdisPacket = NULL;
		}
	}
	return NDIS_STATUS_FAILURE;
}

static INT32 pci_clean_q (RTMP_ADAPTER *pAd){
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UINT32 Index;
	TXD_STRUC *pTxD = NULL;
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QID_AC_BE];
#ifdef RT_BIG_ENDIAN
	TXD_STRUC *pDestTxD = NULL;
	UCHAR TxHwInfo[TXD_SIZE];
#endif
	PNDIS_PACKET  pPacket = NULL;

	MTWF_LOG(DBG_CAT_TEST,  DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s -->\n", __FUNCTION__));

	/* Polling TX/RX path until packets empty */
	if(ATECtrl->tx_coherent == 0)
		MTPciPollTxRxEmpty(pAd);
	for (Index = 0; Index < TX_RING_SIZE; Index++)
	{
#ifdef RT_BIG_ENDIAN
		pDestTxD  = (TXD_STRUC *)pTxRing->Cell[Index].AllocVa;
		NdisMoveMemory(&TxHwInfo[0], (UCHAR *)pDestTxD, TXD_SIZE);
		pTxD = (TXD_STRUC *)&TxHwInfo[0];
#else
		pTxD = (TXD_STRUC *)pTxRing->Cell[Index].AllocVa;
#endif
		if(pTxD)
		{
			pPacket = pTxRing->Cell[Index].pNextNdisPacket;
			if (pPacket)
			{
				PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, RTMP_PCI_DMA_TODEVICE);
			}
#ifdef RT_BIG_ENDIAN
			RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
			if(pDestTxD) 
				WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif
		}
	}
	return NDIS_STATUS_SUCCESS;
}

static INT32 pci_setup_frame(RTMP_ADAPTER *pAd, UINT32 q_idx){
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UINT32 Index;
	UINT32 TxIdx = 0;
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QID_AC_BE];

	MTWF_LOG(DBG_CAT_TEST,  DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s -->, \n", __FUNCTION__));
	if(ATECtrl->allocated == 0)
		goto pci_setup_frame_err;
	
	RTMP_IO_READ32(pAd, pTxRing->hw_didx_addr, &pTxRing->TxDmaIdx);
	pTxRing->TxSwFreeIdx = pTxRing->TxDmaIdx;
	pTxRing->TxCpuIdx = pTxRing->TxDmaIdx;
	RTMP_IO_WRITE32(pAd, pTxRing->hw_cidx_addr, pTxRing->TxCpuIdx);

    if (ATECtrl->bQAEnabled != TRUE) /* reset in start tx when iwpriv */
	    ATECtrl->TxDoneCount = 0;
	for (Index = 0; Index < TX_RING_SIZE; Index++)
		pTxRing->Cell[Index].pNdisPacket = ATECtrl->pAtePacket[Index];
	
	for (Index = 0; (Index < TX_RING_SIZE) && (Index < ATECtrl->TxCount); Index++)
	{
		MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Index = %d/%d, ATECtrl->TxCount = %u\n", Index, TX_RING_SIZE,ATECtrl->TxCount));
		TxIdx = pTxRing->TxCpuIdx;
		if (MT_ATESetupFrame(pAd, TxIdx) != 0)
		{
			return (NDIS_STATUS_FAILURE);
		}

		if (((Index + 1) < TX_RING_SIZE) && (Index < ATECtrl->TxCount))
		{
			INC_RING_INDEX(pTxRing->TxCpuIdx, TX_RING_SIZE);
		}
	}

	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s, TxCpuIdx: %u, TxDmaIdx: %u\n", __FUNCTION__,pTxRing->TxCpuIdx, pTxRing->TxDmaIdx));
	return NDIS_STATUS_SUCCESS;

pci_setup_frame_err:
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s, Setup frame fail\n", __FUNCTION__));
	return NDIS_STATUS_FAILURE;
}

static INT32 pci_test_frame_tx(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QID_AC_BE];
	if(ATECtrl->allocated == 0)
		goto pci_tx_frame_err;
	/* Enable PDMA */
	AsicSetWPDMA(pAd, PDMA_TX_RX, 1);
	ATECtrl->Mode |= ATE_TXFRAME;
	RTMP_IO_WRITE32(pAd, pTxRing->hw_cidx_addr, pTxRing->TxCpuIdx);

	return NDIS_STATUS_SUCCESS;
pci_tx_frame_err:
	MTWF_LOG(DBG_CAT_TEST, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s, tx frame fail\n", __FUNCTION__));
	return NDIS_STATUS_FAILURE;
}

static INT32 pci_clean_test_rx_frame(RTMP_ADAPTER *pAd)
{
	UINT32 Index, RingNum;

	RXD_STRUC *pRxD = NULL;
#ifdef RT_BIG_ENDIAN
	RXD_STRUC *pDestRxD;
	UCHAR RxHwInfo[RXD_SIZE];
#endif

	for (RingNum = 0; RingNum < NUM_OF_RX_RING; RingNum++)
	{
		for (Index = 0; Index < RX_RING_SIZE; Index++)
		{
#ifdef RT_BIG_ENDIAN
			pDestRxD = (RXD_STRUC *)pAd->RxRing[0].Cell[Index].AllocVa;
			NdisMoveMemory(&RxHwInfo[0], pDestRxD, RXD_SIZE);
			pRxD = (RXD_STRUC *)&RxHwInfo[0];
			RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#else
			/* Point to Rx indexed rx ring descriptor */
			pRxD = (RXD_STRUC *)pAd->RxRing[0].Cell[Index].AllocVa;
#endif
			pRxD->DDONE = 0;

#ifdef RT_BIG_ENDIAN
			RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
			WriteBackToDescriptor((PUCHAR)pDestRxD, (PUCHAR)pRxD, FALSE, TYPE_RXD);
#endif
		}
	}
	return NDIS_STATUS_SUCCESS;
}

static INT32 pci_ate_leave(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;
	UINT32 Index;
	TXD_STRUC *pTxD = NULL;
	RTMP_TX_RING *pTxRing = &pAd->TxRing[QID_AC_BE];
#ifdef RT_BIG_ENDIAN
    TXD_STRUC *pDestTxD = NULL;
	UCHAR tx_hw_info[TXD_SIZE];
#endif /* RT_BIG_ENDIAN */
	
	MTWF_LOG(DBG_CAT_TEST,  DBG_SUBCAT_ALL, DBG_LVL_TRACE,("%s -->, \n", __FUNCTION__));

	NICReadEEPROMParameters(pAd, NULL);
	NICInitAsicFromEEPROM(pAd);
	if(ATECtrl->allocated == 1){
		/*  Disable PDMA */
		AsicSetWPDMA(pAd, PDMA_TX_RX, 0);

		for (Index = 0; Index < TX_RING_SIZE; Index++)
		{
			PNDIS_PACKET pPacket;
#ifndef RT_BIG_ENDIAN
		    pTxD = (TXD_STRUC *)pAd->TxRing[QID_AC_BE].Cell[Index].AllocVa;
#else
    		pDestTxD = (TXD_STRUC *)pAd->TxRing[QID_AC_BE].Cell[Index].AllocVa;
			NdisMoveMemory(&tx_hw_info[0], (UCHAR *)pDestTxD, TXD_SIZE);
			pTxD = (TXD_STRUC *)&tx_hw_info[0];
    		RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif /* !RT_BIG_ENDIAN */
			pTxD->DMADONE = 0;
			pPacket = pTxRing->Cell[Index].pNdisPacket;
			if (pPacket)
			{
				PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr0, pTxD->SDLen0, RTMP_PCI_DMA_TODEVICE);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			}
			/* Always assign pNdisPacket as NULL after clear */
			pTxRing->Cell[Index].pNdisPacket = NULL;

			pPacket = pTxRing->Cell[Index].pNextNdisPacket;
			
			if (pPacket)
			{
				PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, RTMP_PCI_DMA_TODEVICE);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			}

			/* Always assign pNextNdisPacket as NULL after clear */
			pTxRing->Cell[Index].pNextNdisPacket = NULL;
#ifdef RT_BIG_ENDIAN
			RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
			WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif /* RT_BIG_ENDIAN */
		}
		ATECtrl->allocated = 0;
	}
	return NDIS_STATUS_SUCCESS;
}
#endif

static ATE_OPERATION MT_ATEOp = {
	.ATEStart = MT_ATEStart,
	.ATEStop = MT_ATEStop,
	.StartTx = MT_ATEStartTx,
	.StartRx = MT_ATEStartRx,
	.StopTx = MT_ATEStopTx,
	.StopRx = MT_ATEStopRx,
	.SetTxPower0 = MT_ATESetTxPower0,
	.SetTxPower1 = MT_ATESetTxPower1,
	.SetTxAntenna = MT_ATESetTxAntenna,
	.SetRxAntenna = MT_ATESetRxAntenna,
	.SetTxFreqOffset = MT_ATESetTxFreqOffset,
	.SetChannel = MT_ATESetChannel,
	.SetBW = MT_ATESetBW,
	.SampleRssi = MT_ATESampleRssi,
	.SetAIFS = MT_ATESetAIFS,
	.SetTSSI = MT_ATESetTSSI,
	.SetDPD = MT_ATESetDPD,
	.StartTxTone = MT_ATEStartTxTone,
	.SetTxTonePower = MT_ATESetTxTonePower,
	.StopTxTone = MT_ATEStopTxTone,
	.StartContinousTx = MT_ATEStartContinousTx,
	.RfRegWrite = MT_RfRegWrite,
	.RfRegRead = MT_RfRegRead,
	.GetFWInfo = MT_GetFWInfo,
	.SetFFTMode = MT_SetFFTMode,
	.MPSSetParm = MT_MPSSetParm,
	.MPSTxStart = MT_MPSTxStart,
	.MPSTxStop = MT_MPSTxStop,
	.SetAutoResp = MT_ATESetAutoResp,
};

#ifdef RTMP_MAC_SDIO
static ATE_IF_OPERATION ate_if_ops = {
	.init = sdio_ate_init,
	.clean_trx_q = sdio_clean_q,
	.clean_test_rx_frame = sdio_clean_test_rx_frame,
	.setup_frame = sdio_setup_frame,
	.test_frame_tx = sdio_test_frame_tx,
	.ate_leave = sdio_ate_leave,
};
#else
#ifdef RTMP_MAC_PCI
 static ATE_IF_OPERATION ate_if_ops = {
	.init = pci_ate_init,
	.clean_trx_q = pci_clean_q,
	.clean_test_rx_frame = pci_clean_test_rx_frame,
	.setup_frame = pci_setup_frame,
	.test_frame_tx = pci_test_frame_tx,
	.ate_leave = pci_ate_leave,
};
#else
static ATE_IF_OPERATION ate_if_ops = {
	.init = NULL,
	.clean_trx_q = NULL,
	.setup_frame = NULL,
	.test_frame_tx = NULL,
	.ate_leave = NULL,
};
#endif /* RTMP_MAC_PCI */
#endif /* RTMP_MAC_SDIO */

INT32 MT_ATEInit(RTMP_ADAPTER *pAd)
{
	ATE_CTRL *ATECtrl = &pAd->ATECtrl;

	ATECtrl->ATEOp = &MT_ATEOp;
	ATECtrl->ATEIfOps = &ate_if_ops;
	return 0;
}

#ifdef RTMP_FLASH_SUPPORT
INT Set_CalWrite_Proc(
	RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{

	printk("Write Caldata to flash\n");

	rtmp_ee_write_flash(pAd, arg);

	return TRUE;
}
#endif

