/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2010, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

/****************************************************************************

    Abstract:

	All related POWER SAVE function body.

***************************************************************************/

#include "rt_config.h"

#ifdef MT_PS
static VOID MtReadPseRdTabAccessReg(
	RTMP_ADAPTER *pAd, 
	UCHAR wcid, 
	UINT32 *p_rPseRdTabAccessReg)
{
	*p_rPseRdTabAccessReg = PSE_RTA_RD_KICK_BUSY | PSE_RTA_TAG(wcid);
	HW_IO_WRITE32(pAd, PSE_RTA, *p_rPseRdTabAccessReg);
	do
	{
		HW_IO_READ32(pAd,PSE_RTA, p_rPseRdTabAccessReg);
	}
	while ( GET_PSE_RTA_RD_KICK_BUSY(*p_rPseRdTabAccessReg) == 1 );

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: *p_rPseRdTabAccessReg = 0x%x\n",
			__FUNCTION__, *p_rPseRdTabAccessReg));
}
static VOID MtClearPseRdTab(
	RTMP_ADAPTER *pAd, 
	UCHAR wcid )
{
	UINT32 rPseRdTabAccessReg = 0;
	rPseRdTabAccessReg = PSE_RTA_RD_RW | PSE_RTA_RD_KICK_BUSY | PSE_RTA_TAG(wcid);
	HW_IO_WRITE32(pAd, PSE_RTA, rPseRdTabAccessReg);
	do
	{
		HW_IO_READ32(pAd,PSE_RTA, &rPseRdTabAccessReg);
	}
	while ( GET_PSE_RTA_RD_KICK_BUSY(rPseRdTabAccessReg) == 1 );

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s: rPseRdTabAccessReg = 0x%x wcid=%d\n",
			__FUNCTION__, rPseRdTabAccessReg, wcid));
}
VOID MtSetIgnorePsm(
	RTMP_ADAPTER *pAd,
	MAC_TABLE_ENTRY *pEntry,
	UCHAR value)
{
	ULONG IrqFlags = 0;
	struct wtbl_entry tb_entry;
	union WTBL_1_DW3 *dw3 = (union WTBL_1_DW3 *)&tb_entry.wtbl_1.wtbl_1_d3.word;

	NdisZeroMemory(&tb_entry, sizeof(tb_entry));

	if (mt_wtbl_get_entry234(pAd, pEntry->wcid, &tb_entry) == FALSE) 
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s():Cannot found WTBL2/3/4\n",__FUNCTION__));
		return;
	}
	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

	pEntry->i_psm = value;
	HW_IO_READ32(pAd, tb_entry.wtbl_addr[0]+12, &dw3->word);
	dw3->field.du_i_psm = value; /* I_PSM changed by HW automatically is enabled. */
	dw3->field.i_psm = value; /* follow PSM value. */
	HW_IO_WRITE32(pAd, tb_entry.wtbl_addr[0] + 12, dw3->word);
	
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);  
}

VOID CheckSkipTX(
	RTMP_ADAPTER *pAd,
	MAC_TABLE_ENTRY *pEntry)
{
	struct wtbl_entry tb_entry;
	union WTBL_1_DW3 *dw3 = (union WTBL_1_DW3 *)&tb_entry.wtbl_1.wtbl_1_d3.word;
	STA_TR_ENTRY *tr_entry;
	CHAR isChange = FALSE;

	NdisZeroMemory(&tb_entry, sizeof(tb_entry));
	if (mt_wtbl_get_entry234(pAd, pEntry->wcid, &tb_entry) == FALSE) 
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s():Cannot found WTBL2/3/4\n",__FUNCTION__));
		return;
	}
	tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];

	HW_IO_READ32(pAd, tb_entry.wtbl_addr[0]+12, &dw3->word);
	if ((tr_entry->ps_state != APPS_RETRIEVE_START_PS) && (dw3->field.skip_tx == 1))
	{
		dw3->field.skip_tx = 0;
		isChange = TRUE;
	} 

	if ((tr_entry->ps_state < APPS_RETRIEVE_DONE) && (dw3->field.du_i_psm == 1))
	{
		dw3->field.du_i_psm = 0;
		dw3->field.i_psm = 0;

		/* also sync pEntry flag*/
		pEntry->i_psm = 0 ;

		isChange = TRUE;
	}
	
	if (isChange == TRUE) {
		pAd->SkipTxRCount++;		
		HW_IO_WRITE32(pAd, tb_entry.wtbl_addr[0]+12, dw3->word);
	}
	return;
}
#endif /* MT_PS */

#if defined(MT7603) || defined(MT7628)
INT  MtPSDummyCR(RTMP_ADAPTER *pAd)
{
	INT value;
	
	HW_IO_READ32(pAd, 0x817c, &value);
	value &= 0xff;
	return value;
}

#ifdef RTMP_MAC_PCI
VOID MtTriggerMCUINT(RTMP_ADAPTER *pAd)
{
	INT value;
	INT restore_remap_addr;

	HW_IO_READ32(pAd, 0x2504, &restore_remap_addr);
	HW_IO_WRITE32(pAd, 0x2504, 0x81000000);
	HW_IO_READ32(pAd, 0xc00c0, &value);
	value |= 0x100;
	HW_IO_WRITE32(pAd, 0xc00c0, value); //Trigger INT to MCU, 0x810400c0
	HW_IO_WRITE32(pAd, 0x2504, restore_remap_addr);
}
#endif /* RTMP_MAC_PCI */		

BOOLEAN  MtStartPSRetrieve(RTMP_ADAPTER *pAd, USHORT wcid)
{
	if (MtPSDummyCR(pAd) != 0) {
		return FALSE;
	}
	
	HW_IO_WRITE32(pAd, 0x817c, wcid);
		
#ifdef RTMP_MAC_PCI
//	MtTriggerMCUINT(pAd);
#endif /* RTMP_MAC_PCI */		

	return TRUE;
}
#endif /*MT7603 || MT7628 */


/*
  ========================================================================
  Description:
	This routine frees all packets in PSQ that's destined to a specific DA.
	BCAST/MCAST in DTIMCount=0 case is also handled here, just like a PS-POLL 
	is received from a WSTA which has MAC address FF:FF:FF:FF:FF:FF
  ========================================================================
*/
VOID MtHandleRxPsPoll(RTMP_ADAPTER *pAd, UCHAR *pAddr, USHORT wcid, BOOLEAN isActive)
{
#ifdef CONFIG_AP_SUPPORT
	MAC_TABLE_ENTRY *pMacEntry;
	STA_TR_ENTRY *tr_entry;
	BOOLEAN       IsDequeu= FALSE;
	INT           DequeuAC = QID_AC_BE;
	INT           DequeuCOUNT;
	INT i, Total_Packet_Number = 0;

	ASSERT(wcid < MAX_LEN_OF_MAC_TABLE);

	pMacEntry = &pAd->MacTab.Content[wcid];
	tr_entry = &pAd->MacTab.tr_entry[wcid];
	
	if (isActive == FALSE) /* ps poll */
	{
#ifdef MT_PS
		if (tr_entry->ps_state == APPS_RETRIEVE_DONE) /*state is finish(sleep)*/
		{				
			MtSetIgnorePsm(pAd, pMacEntry, I_PSM_ENABLE);
		}

		if(tr_entry->ps_state == APPS_RETRIEVE_DONE || tr_entry->ps_state == APPS_RETRIEVE_IDLE)
		{
			for (i = 0; i < WMM_QUE_NUM; i++)
				Total_Packet_Number = Total_Packet_Number + tr_entry->tx_queue[i].Number;

			if (Total_Packet_Number > 0)
			{
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RtmpHandleRxPsPoll fetch tx queue tr_entry->ps_queue.Number= %x tr_entry->tx_queue[0].Number=%x Total_Packet_Number=%x\n",
						tr_entry->ps_queue.Number, tr_entry->tx_queue[QID_AC_BE].Number, Total_Packet_Number));

					for (i = WMM_QUE_NUM - 1; i >=0; i--)
					{
						if (tr_entry->tx_queue[i].Head)
						{
							if (Total_Packet_Number > 1)
							{
								RTMP_SET_PACKET_MOREDATA(RTPKT_TO_OSPKT(tr_entry->tx_queue[i].Head), TRUE);
							}
							RTMP_SET_PACKET_TXTYPE(RTPKT_TO_OSPKT(tr_entry->tx_queue[i].Head), TX_LEGACY_FRAME);

							DequeuAC = i;
							IsDequeu = TRUE;
							DequeuCOUNT = 1;
							tr_entry->PsQIdleCount = 0;
							break;
						}
					}               
				}
			}
			else /* Recieve ps_poll but no packet==>send NULL Packet */
			{ 
				BOOLEAN bQosNull = FALSE;
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("RtmpHandleRxPsPoll no packet tr_entry->ps_queue.Number= %x tr_entry->tx_queue[0].Number=%x Total_Packet_Number=%x\n"
					,tr_entry->ps_queue.Number, tr_entry->tx_queue[QID_AC_BE].Number, Total_Packet_Number));
				if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))
					bQosNull = TRUE;

				RtmpEnqueueNullFrame(pAd, pMacEntry->Addr, tr_entry->CurrTxRate,
				pMacEntry->Aid, pMacEntry->func_tb_idx,
				bQosNull, TRUE, 0);
			} 
			if (Total_Packet_Number >1)
			{
				WLAN_MR_TIM_BIT_SET(pAd, tr_entry->func_tb_idx, tr_entry->wcid);
			} 
			else 
			{
				WLAN_MR_TIM_BIT_CLEAR(pAd, tr_entry->func_tb_idx, tr_entry->wcid);
			}
		}
#else
		/*
			Need to check !! @20140212
			New architecture has per AC sw-Q for per entry.
			We should check packets by ACs priority --> 1. VO, 2. VI, 3. BE, 4. BK
		*/
		DequeuAC = QID_AC_BE;
		IsDequeu = TRUE;
		DequeuCOUNT = 1;
		tr_entry->PsQIdleCount = 0;
#endif /* Ps_poll and ifndef MT_PS */

	}        
	else /* Receive Power bit 0 frame */ 
	{
		WLAN_MR_TIM_BIT_CLEAR(pAd, tr_entry->func_tb_idx, tr_entry->wcid);
#ifdef MT_PS
		MtSetIgnorePsm(pAd, pMacEntry, I_PSM_DISABLE);
#endif /*Power bit is 1 and ifndef MT_PS */

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("RtmpHandleRxPsPoll null0/1 wcid = %x mt_ps_queue.Number = %d\n",
			tr_entry->wcid,
			tr_entry->ps_queue.Number));

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(%d) tx_queue.Number = BE:%d, BK:%d, VI:%d, VO:%d, ps_state:%x,  tx_queue.TokenCount = BE:%d, BK:%d, VI:%d, VO:%d\n",
			__FUNCTION__, __LINE__,
			tr_entry->tx_queue[QID_AC_BE].Number,
			tr_entry->tx_queue[QID_AC_BK].Number,
			tr_entry->tx_queue[QID_AC_VI].Number,
			tr_entry->tx_queue[QID_AC_VO].Number,
			tr_entry->ps_state,
			tr_entry->TokenCount[QID_AC_BE],
			tr_entry->TokenCount[QID_AC_BK],
			tr_entry->TokenCount[QID_AC_VI],
			tr_entry->TokenCount[QID_AC_VO]));

#ifdef UAPSD_SUPPORT
		/* deliver all queued UAPSD packets */
		UAPSD_AllPacketDeliver(pAd, pMacEntry);

		/* end the SP if exists */
		UAPSD_MR_ENTRY_RESET(pAd, pMacEntry);
#endif /* UAPSD_SUPPORT */

		if (tr_entry->enqCount > 0) 
		{
			IsDequeu = TRUE;
		DequeuAC = NUM_OF_TX_RING;
			if (tr_entry->enqCount > 8 /* MAX_TX_PROCESS */)
			{
				DequeuCOUNT =  8 /* MAX_TX_PROCESS */;
			}
			else
			{
				DequeuCOUNT = tr_entry->enqCount;
			}
		}
	}   

	if (IsDequeu == TRUE)
	{
		RTMPDeQueuePacket(pAd, FALSE, DequeuAC, tr_entry->wcid, DequeuCOUNT);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("RtmpHandleRxPsPoll IsDequeu == TRUE tr_entry->wcid=%x DequeuCOUNT=%d, ps_state=%d\n",tr_entry->wcid, DequeuCOUNT, tr_entry->ps_state));
	}    
	return;
	
#endif /* CONFIG_AP_SUPPORT */
}

#define MT_PS_WATCHDOG_THRES 20

VOID MtPsWatchDog(RTMP_ADAPTER *pAd)
{
	MAC_TABLE_ENTRY *pEntry = NULL;
	STA_TR_ENTRY *tr_entry = NULL;
	INT i = 0;
	UINT32 Value=0;
	struct wtbl_entry tb_entry;
        union WTBL_1_DW3 *dw3 = (union WTBL_1_DW3 *)&tb_entry.wtbl_1.wtbl_1_d3.word;

	if(pAd->PsmWatchDogDisabled)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(): DISABLED THE PSM WATCHDOG BY CMD \n",__FUNCTION__));
		return;
	}		

	for (i = 1; i< MAX_LEN_OF_MAC_TABLE; i++)   
    {
		pEntry = &pAd->MacTab.Content[i];
		tr_entry = &pAd->MacTab.tr_entry[i];

        /* add by lizuli, fix Bug 321712   */
        if ((IS_ENTRY_NONE(pEntry)) && (pEntry->Sst == SST_ASSOC))
        {
            WLAN_MR_TIM_BIT_CLEAR(pAd, tr_entry->func_tb_idx, tr_entry->wcid);
        }
        /*  end add  */

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			if ((pEntry->PsMode == 0) && (pEntry->PsmPktCount == 0))
			{
				NdisZeroMemory(&tb_entry, sizeof(tb_entry));
            	if (mt_wtbl_get_entry234(pAd, i, &tb_entry) == FALSE)
            	{
                    	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Cannot found WTBL2/3/4\n",__FUNCTION__));
                    	continue;
            	}
				RTMP_IO_READ32(pAd, tb_entry.wtbl_addr[0]+12, &dw3->word);

				if (dw3->field.psm == 1)
				{
					pEntry->PsmUpDuration++;
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): HIT PsmPktCount/Dur: %d/%d on Wcid %d (SW_PSM:[%d|%d])\n\n\n\n\n",
                                        	__FUNCTION__, pEntry->PsmPktCount, pEntry->PsmUpDuration, pEntry->wcid, pEntry->PsMode, 
						tr_entry->PsMode));

                    if (pEntry->PsmUpDuration >= MT_PS_WATCHDOG_THRES)
    		        {
                    		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s():Hit the PSM Case > %d x 100ms with Wcid %d (SW_PSM:[%d|%d])\n\n\n\n\n",
                            		__FUNCTION__, MT_PS_WATCHDOG_THRES, pEntry->wcid, pEntry->PsMode, tr_entry->PsMode));


                            MAC_IO_READ32(pAd, WTBL1OR, &Value);
    		                Value |= PSM_W_FLAG;
                    		MAC_IO_WRITE32(pAd, WTBL1OR, Value);

                            dw3->field.psm = 0;
    		                HW_IO_WRITE32(pAd, tb_entry.wtbl_addr[0] + 12, dw3->word);

                    		pEntry->PsMode = 0;
                    		tr_entry->PsMode = 0;

                            MAC_IO_READ32(pAd, WTBL1OR, &Value);
    		                Value &= ~PSM_W_FLAG;
                    		MAC_IO_WRITE32(pAd, WTBL1OR, Value);
                    		pEntry->PsmResetCount++;

    		        }
				}
				else
				{
					pEntry->PsmPktCount = 0;
                	pEntry->PsmUpDuration = 0;
				}

			}
			else
			{
				pEntry->PsmPktCount = 0;
				pEntry->PsmUpDuration = 0;
			}
		}
	}

}

/*
	==========================================================================
	Description:
		Update the station current power save mode. Calling this routine also
		prove the specified client is still alive. Otherwise AP will age-out
		this client once IdleCount exceeds a threshold.
	==========================================================================
 */
BOOLEAN MtPsIndicate(RTMP_ADAPTER *pAd, UCHAR *pAddr, UCHAR wcid, UCHAR Psm)
{
	MAC_TABLE_ENTRY *pEntry;
	UCHAR old_psmode;
	STA_TR_ENTRY *tr_entry;

	if (wcid >= MAX_LEN_OF_MAC_TABLE)
	{
		return PWR_ACTIVE;
	}

	pEntry = &pAd->MacTab.Content[wcid];
	tr_entry = &pAd->MacTab.tr_entry[wcid];

	/*
		Change power save mode first because we will call
		RTMPDeQueuePacket() in RtmpHandleRxPsPoll().

		Or when Psm = PWR_ACTIVE, we will not do Aggregation in
		RTMPDeQueuePacket().
	*/
	old_psmode = pEntry->PsMode;
	pEntry->NoDataIdleCount = 0;
	pEntry->PsMode = Psm;
	tr_entry->PsMode = Psm;

	if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		pEntry->PsmPktCount++;


	if ((old_psmode == PWR_SAVE) && (Psm == PWR_ACTIVE))
	{
		/*
			STA wakes up.
		*/		
		if(tr_entry->ps_state == APPS_RETRIEVE_DONE)
		{
			tr_entry->ps_state = APPS_RETRIEVE_IDLE;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(%d): STA wakes up!\n", __FUNCTION__, __LINE__));
			MtHandleRxPsPoll(pAd, pAddr, wcid, TRUE);
		}
		else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(%d):wcid=%d, old_psmode=%d, now_psmode=%d, wrong ps_state=%d ???\n",
					__FUNCTION__, __LINE__, wcid, old_psmode, Psm, tr_entry->ps_state));
	}
	else if ((old_psmode == PWR_ACTIVE) && (Psm == PWR_SAVE))
	{
		/*
			STA goes to sleep.
		*/

		if (tr_entry->ps_state == APPS_RETRIEVE_IDLE)
		{ 
#ifdef MT_PS
#if defined(MT7603) || defined(MT7628) //Use write CRs replace PS cmd
			if (MtStartPSRetrieve(pAd, wcid) == TRUE) {
			tr_entry->ps_state = APPS_RETRIEVE_START_PS;
			} else {
				struct tx_swq_fifo *ps_fifo_swq;
				INT enq_idx;
				
				ps_fifo_swq = &pAd->apps_cr_q;
				enq_idx = ps_fifo_swq->enqIdx;
				if (ps_fifo_swq->swq[enq_idx] == 0)
				{
					ps_fifo_swq->swq[enq_idx] = wcid;
					INC_RING_INDEX(ps_fifo_swq->enqIdx, TX_SWQ_FIFO_LEN);
					tr_entry->ps_state = APPS_RETRIEVE_CR_PADDING;
				} else {
					INT idx;
					tr_entry->ps_state = APPS_RETRIEVE_DONE;
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ERROR!! ps_fifo_swq->deqIdx=%d, ps_fifo_swq->enqIdx=%d\n", __FUNCTION__,ps_fifo_swq->deqIdx,ps_fifo_swq->enqIdx));
					for (idx =0; idx < TX_SWQ_FIFO_LEN;idx++)
					{
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, (",[%d] =%d\n", idx, ps_fifo_swq->swq[idx]));
						if ((idx % 16) == 0)
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("\n"));
					}
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("\n"));
				}				
			}
#else /* !MT7603 && MT7628 */
			RTEnqueueInternalCmd(pAd, CMDTHREAD_PS_RETRIEVE_START, pEntry, sizeof(MAC_TABLE_ENTRY));
#endif
#else /* MT_PS */
			tr_entry->ps_state = APPS_RETRIEVE_DONE;
#endif /* !MT_PS */
		}
		else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(%d):wcid=%d, old_psmode=%d, now_psmode=%d, wrong ps_state=%d ???\n",
					__FUNCTION__, __LINE__, wcid, old_psmode, Psm, tr_entry->ps_state));
	}
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("%s(%d): ps state is not changed, do nothing here.\n",
			__FUNCTION__, __LINE__));
	}
   
	return old_psmode;
}

#ifdef MT_PS
VOID MtPsRedirectDisableCheck(
	RTMP_ADAPTER *pAd,
	UCHAR wcid)
{
	UINT32 Reg = 0, wlan_idx = 0;
	BOOLEAN pfgForce = 0;
	
	if (pAd->chipCap.hif_type != HIF_MT)
	{
		return;
	}

	MtReadPseRdTabAccessReg(pAd, wcid, &Reg);
	
	pfgForce = ( BOOLEAN ) GET_PSE_RTA_RD_RULE_F(Reg);
	
	if (pfgForce)
	{
		wlan_idx = (UINT32)wcid;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(%d): [wlan_idx=0x%x] PS Redirect mode(pfgForce = %d) is enabled. Send PC Clear command to FW.\n",
			__FUNCTION__, __LINE__, wlan_idx, pfgForce));

		MtClearPseRdTab(pAd, wlan_idx);
	}
}

VOID MtPsSendToken(
	RTMP_ADAPTER *pAd,
	UINT32 WlanIdx)
{
	MAC_TABLE_ENTRY *pEntry = NULL;
	STA_TR_ENTRY *tr_entry;
	NDIS_STATUS token_status;
	struct wtbl_entry tb_entry;
	union WTBL_1_DW3 *dw3;
	UINT32 Reg = 0;
	BOOLEAN pfgForce = 0;
	unsigned char q_idx;
	
	if (pAd->chipCap.hif_type != HIF_MT)
	{
		return;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(wcid=%d): Driver didn't receive PsRetrieveStartRsp from FW.\n",__FUNCTION__, WlanIdx));
	pEntry = &pAd->MacTab.Content[WlanIdx];
	
	tr_entry = &pAd->MacTab.tr_entry[WlanIdx];
	NdisZeroMemory(&tb_entry, sizeof(tb_entry));
	dw3 = (union WTBL_1_DW3 *)&tb_entry.wtbl_1.wtbl_1_d3.word;

	if (mt_wtbl_get_entry234(pAd, WlanIdx, &tb_entry) == FALSE) 
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Cannot found WTBL2/3/4, wcid=%d\n",__FUNCTION__, WlanIdx));
		return;
	}

	HW_IO_READ32(pAd, tb_entry.wtbl_addr[0]+12, &dw3->word);

	MtReadPseRdTabAccessReg(pAd, pEntry->wcid, &Reg);
		
	pfgForce = ( BOOLEAN ) GET_PSE_RTA_RD_RULE_F(Reg);
	
	if (pfgForce == 0)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s(ps_state = %d): PS Redirect mode didn't be enabled. Driver doesn't need to send token.\n",__FUNCTION__, tr_entry->ps_state));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s: Reset ps_state to IDLE.\n",__FUNCTION__));
		tr_entry->ps_state = APPS_RETRIEVE_IDLE;
		return;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("---->%s: Start to send TOKEN frames.\n", __FUNCTION__));

	tr_entry->ps_state = APPS_RETRIEVE_GOING;

	tr_entry->ps_qbitmap = 0;

	for (q_idx = 0; q_idx < NUM_OF_TX_RING; q_idx++)
	{
		token_status = RtmpEnqueueTokenFrame(pAd, &(pEntry->Addr[0]), 0, WlanIdx, 0, q_idx);
		if (!token_status)
			tr_entry->ps_qbitmap |= (1 << q_idx);
		else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s(%d) Fail:  Send TOKEN Frame, AC=%d\n", __FUNCTION__, __LINE__, q_idx));
	}

	if (tr_entry->ps_qbitmap == 0)
	{
		tr_entry->ps_state = APPS_RETRIEVE_WAIT_EVENT;
		RTEnqueueInternalCmd(pAd, CMDTHREAD_PS_CLEAR, (VOID *)&WlanIdx, sizeof(UINT32));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("(ps_state = %d) token enqueue failed for all queues ==> send CMDTHREAD_PS_CLEAR cmd.\n", tr_entry->ps_state));
	}
}


VOID MtPsRecovery(
	RTMP_ADAPTER *pAd)
{
	MAC_TABLE_ENTRY *pMacEntry;
	STA_TR_ENTRY *tr_entry;
	UINT32 i;

	for (i=1; i < MAX_LEN_OF_MAC_TABLE; i++)
	{
		pMacEntry = &pAd->MacTab.Content[i];
		tr_entry = &pAd->MacTab.tr_entry[i];
		if (IS_ENTRY_CLIENT(pMacEntry))
		{
			if (tr_entry->ps_state == APPS_RETRIEVE_CR_PADDING) {
				tr_entry->ps_state = APPS_RETRIEVE_IDLE;
			} else if ((tr_entry->ps_state == APPS_RETRIEVE_START_PS) 
				|| (tr_entry->ps_state == APPS_RETRIEVE_GOING))
			{
				if (tr_entry->ps_queue.Number) {
					MtEnqTxSwqFromPsQueue(pAd, QID_AC_BE, tr_entry);
				}


				 if (pAd->MacTab.tr_entry[i].PsMode == PWR_ACTIVE) {
					tr_entry->ps_state = APPS_RETRIEVE_IDLE;
					 MtHandleRxPsPoll(pAd, &pMacEntry->Addr[0], i, TRUE);
				 } else
					tr_entry->ps_state = APPS_RETRIEVE_DONE;
			} else if(tr_entry->ps_state == APPS_RETRIEVE_WAIT_EVENT)
			{
				RTEnqueueInternalCmd(pAd, CMDTHREAD_PS_CLEAR, (VOID *)&i, sizeof(UINT32));
			}
		}
	}
}


VOID MtEnqTxSwqFromPsQueue(RTMP_ADAPTER *pAd, UCHAR qidx, STA_TR_ENTRY *tr_entry)
{
	ULONG IrqFlags = 0;
	//struct tx_swq_fifo *fifo_swq = &pAd->tx_swq[qidx];
	QUEUE_ENTRY *pQEntry;
	QUEUE_HEADER *pAcPsQue = &tr_entry->ps_queue;
	QUEUE_HEADER *pAcTxQue = &tr_entry->tx_queue[qidx];
#ifdef UAPSD_SUPPORT
	MAC_TABLE_ENTRY *pEntry = NULL;
#endif
	
	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#ifdef UAPSD_SUPPORT
	pEntry = &pAd->MacTab.Content[tr_entry->wcid];
	if (UAPSD_MR_IS_UAPSD_AC(pEntry, qidx))
	{
		while(pAcPsQue->Head)
		{
			pQEntry = RemoveTailQueue(pAcPsQue);
			UAPSD_PacketEnqueue(pAd, pEntry, PACKET_TO_QUEUE_ENTRY(pQEntry), qidx, TRUE);
		}
	}
	else
#endif /* UAPSD_SUPPORT */
	{
		while(pAcPsQue->Head)
		{
			pQEntry = RemoveTailQueue(pAcPsQue);
			if(tr_entry->enqCount > SQ_ENQ_NORMAL_MAX) {
			RELEASE_NDIS_PACKET(pAd, pQEntry, NDIS_STATUS_FAILURE);
			continue;
                }
	
		InsertHeadQueue(pAcTxQue, pQEntry); 			
		TR_ENQ_COUNT_INC(tr_entry);
		}
	}
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

	return;
}
#endif /* MT_PS */

