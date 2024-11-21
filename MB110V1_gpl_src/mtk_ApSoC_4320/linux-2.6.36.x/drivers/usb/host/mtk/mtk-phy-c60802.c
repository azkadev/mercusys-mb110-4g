#ifdef CONFIG_C60802_SUPPORT
#include "mtk-phy.h"
#include "mtk-phy-c60802.h"

PHY_INT32 phy_init_c60802(struct u3phy_info *info){
	PHY_UINT8 temp;

	//****** u2phy part *******//
	DRV_MDELAY(100);
	//RG_USB20_BGR_EN = 1
	temp = U3PhyReadReg8((PHY_UINT32)(&info->u2phy_regs_c->u2phyac0));
	temp |= 0x1;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyac0),temp);
	DRV_MDELAY(100);
	//RG_USB20_REF_EN = 1
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyac0)+1);
	temp |= 0x80;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyac0)+1,temp);
	DRV_MDELAY(100);
	//RG_USB20_SW_PLLMODE = 2
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydcr1)+2);
	temp &= ~(0x3<<2);
	temp |= (0x2<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydcr1)+2,temp);
	DRV_MDELAY(100);

	//****** u3phyd part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0));
	temp &= ~0x1F;
	temp |= 0x19;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0), temp);
	DRV_MDELAY(100);
	//turn off phy clock gating, marked in normal case
	//U3PhyWriteReg8(&info->u3phyd_regs_c->phyd_mix1+2,0x0);
	//mdelay(100);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0)+1);
	temp |= 0x80;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0)+1, temp);
	DRV_MDELAY(100);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_pll_0));
	temp |= 0x80;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_pll_0), temp);
	DRV_MDELAY(100);

	//****** u3phyd bank2 part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_phyd_top1)+2);
	temp &= ~0x03;
	temp |= 0x01;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_phyd_top1)+2,temp);
	DRV_MDELAY(100);
	
	//****** u3phya part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phya_regs_c->reg7)+1);
	temp &= ~0x40;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_regs_c->reg7)+1, temp);
	DRV_MDELAY(100);

	//****** u3phya_da part *******//
	// set SSC to pass electrical compliance SSC min
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_da_regs_c->reg21)+2, 0x47);
	DRV_MDELAY(100);

	//****** u3phy chip part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlb)+3);
	temp |= 0x01;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlb)+3, temp);
	//phase set
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+2, 0x10);
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+3, 0x74);
	
	return 0;
}

#define PHY_DRV_SHIFT	3
#define PHY_PHASE_SHIFT	3
#define PHY_PHASE_DRV_SHIFT	1
PHY_INT32 phy_change_pipe_phase_c60802(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase){
	PHY_INT32 drv_reg_value;
	PHY_INT32 phase_reg_value;
	PHY_INT32 temp;

	drv_reg_value = phy_drv << PHY_DRV_SHIFT;
	phase_reg_value = (pipe_phase << PHY_PHASE_SHIFT) | (phy_drv << PHY_PHASE_DRV_SHIFT);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+2);
	temp &= ~(0x3 << PHY_DRV_SHIFT);
	temp |= drv_reg_value;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+2, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+3);
	temp &= ~((0x3 << PHY_PHASE_DRV_SHIFT) | (0x1f << PHY_PHASE_SHIFT));
	temp |= phase_reg_value;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+3, temp);
	return PHY_TRUE;
}

//--------------------------------------------------------
//    Function : fgEyeScanHelper_CheckPtInRegion()
// Description : Check if the test point is in a rectangle region.
//               If it is in the rectangle, also check if this point
//               is on the multiple of deltaX and deltaY.
//   Parameter : strucScanRegion * prEye - the region
//               BYTE bX
//               BYTE bY
//      Return : BYTE - TRUE :  This point needs to be tested
//                      FALSE:  This point will be omitted
//        Note : First check within the rectangle.
//               Secondly, use modulous to check if the point will be tested.
//--------------------------------------------------------
PHY_INT8 fgEyeScanHelper_CheckPtInRegion(struct strucScanRegion * prEye, PHY_INT8 bX, PHY_INT8 bY)
{
  PHY_INT8 fgValid = true;


  /// Be careful, the axis origin is on the TOP-LEFT corner.
  /// Therefore the top-left point has the minimum X and Y
  /// Botton-right point is the maximum X and Y
  if ( (prEye->bX_tl <= bX) && (bX <= prEye->bX_br)
    && (prEye->bY_tl <= bY) && (bY <= prEye->bX_br))
  {
    // With the region, now check whether or not the input test point is
    // on the multiples of X and Y
    // Do not have to worry about negative value, because we have already
    // check the input bX, and bY is within the region.
    if ( ((bX - prEye->bX_tl) % (prEye->bDeltaX))
      || ((bY - prEye->bY_tl) % (prEye->bDeltaY)) )
    {
      // if the division will have remainder, that means
      // the input test point is on the multiples of X and Y
      fgValid = false;
    }
    else
    {
    }
  }
  else
  {
    
    fgValid = false;
  }
  return fgValid;
}

//--------------------------------------------------------
//    Function : EyeScanHelper_RunTest()
// Description : Enable the test, and wait til it is completed
//   Parameter : None
//      Return : None
//        Note : None
//--------------------------------------------------------
void EyeScanHelper_RunTest(struct u3phy_info *info)
{
  // Disable the test
  eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0), 3, 1, 0);	//RG_SSUSB_RX_EYE_CNT_EN = 0

  // Run the test
  eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0), 3, 1, 1);	//RG_SSUSB_RX_EYE_CNT_EN = 1

  // Wait til it's done
  while(!eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->phya_rx_mon4), 7, 1));	//RGS_SSUSB_RX_EYE_CNT_RDY
}

//--------------------------------------------------------
//    Function : fgEyeScanHelper_CalNextPoint()
// Description : Calcualte the test point for the measurement
//   Parameter : None
//      Return : BOOL - TRUE :  the next point is within the
//                              boundaryof HW limit
//                      FALSE:  the next point is out of the HW limit
//        Note : The next point is obtained by calculating
//               from the bottom left of the region rectangle
//               and then scanning up until it reaches the upper
//               limit. At this time, the x will increment, and
//               start scanning downwards until the y hits the
//               zero.
//--------------------------------------------------------
PHY_INT8 fgEyeScanHelper_CalNextPoint(void)
{
  if ( ((_bYcurr == MAX_Y) && (_eScanDir == SCAN_DN))
    || ((_bYcurr == MIN_Y) && (_eScanDir == SCAN_UP))
        )
  {
    /// Reaches the limit of Y axis
    /// Increment X
    _bXcurr++;
    _fgXChged = true;
    _eScanDir = (_eScanDir == SCAN_UP) ? SCAN_DN : SCAN_UP;

    if (_bXcurr > MAX_X)
    {
      return false;
    }
  }
  else
  {
    _bYcurr = (_eScanDir == SCAN_DN) ? _bYcurr + 1 : _bYcurr - 1;
    _fgXChged = false;
  }
  return PHY_TRUE;
}



PHY_INT32 phy_eyescan_c60802(struct u3phy_info *info, PHY_INT32 x_t1, PHY_INT32 y_t1, PHY_INT32 x_br, PHY_INT32 y_br, PHY_INT32 delta_x, PHY_INT32 delta_y
		, PHY_INT32 eye_cnt, PHY_INT32 num_cnt, PHY_INT32 PI_cal_en, PHY_INT32 num_ignore_cnt){
	PHY_INT32 cOfst = 0;
	PHY_INT8 bIdxX = 0;
	PHY_INT8 bIdxY = 0;
	PHY_INT8 bCnt = 0;
	PHY_INT8 bIdxCycCnt = 0;
	PHY_INT8 fgValid;
	PHY_INT8 cX;
	PHY_INT8 cY;
	PHY_INT8 bExtendCnt;
	PHY_INT8 isContinue;
	PHY_INT8 isBreak;
	PHY_UINT16 wErr0 = 0, wErr1 = 0;
	PHY_UINT32 temp;

	PHY_UINT32 pwErrCnt0[CYCLE_COUNT_MAX][ERRCNT_MAX][ERRCNT_MAX];
	PHY_UINT32 pwErrCnt1[CYCLE_COUNT_MAX][ERRCNT_MAX][ERRCNT_MAX];

	_bXcurr = 0;
	_bYcurr = 0;
	_eScanDir = SCAN_DN;
	_fgXChged = false;

	//initial PHY setting
	temp = 0xda;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_regs_c->rega), temp);
	temp = 0x22;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_mix3)+1, temp);
	//force SIGDET to OFF
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_reserved)+1, 3, 1, 1);	//RG_SSUSB_RX_SIGDET_SEL = 1
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_reserved)+1, 4, 1, 0);	//RG_SSUSB_RX_SIGDET_EN = 0
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye1)+3, 0, 7, 0);		//RG_SSUSB_RX_SIGDET = 0
	// RX_TRI_DET_EN to Disable
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq3), 7, 1, 0);				//RG_SSUSB_RX_TRI_DET_EN = 0

	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 0, 1, 1);		//RG_SSUSB_EYE_MON_EN = 1
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 1, 7, 0);		//RG_SSUSB_RX_EYE_XOFFSET = 0
	eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+2, 0);				//RG_SSUSB_RX_EYE0_Y = 0
	eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+1, 0);				//RG_SSUSB_RX_EYE1_Y = 0

	if (PI_cal_en){
		// PI Calibration
		eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_reserved)+1, 1, 1, 1);	//RG_SSUSB_RX_PI_CAL_MANUAL_SEL = 1
		eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_reserved)+1, 2, 1, 0);	//RG_SSUSB_RX_PI_CAL_MANUAL_EN = 0
		eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_reserved)+1, 2, 1, 1);	//RG_SSUSB_RX_PI_CAL_MANUAL_EN = 1

		DRV_UDELAY(20);

		eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_reserved)+1, 2, 1, 0);			//RG_SSUSB_RX_PI_CAL_MANUAL_EN = 0
		_bPIResult = eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->phya_rx_mon4), 0, 7);	//read RGS_SSUSB_RX_PILPO
		printk(KERN_ERR "PI result: %d\n", _bPIResult);
	}
	// Read Initial DAC
	// Set CYCLE
	eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye2)+1, HI_BYTE(eye_cnt));	//RG_SSUSB_RX_EYE_CNT_1
	eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye2), LO_BYTE(eye_cnt));	//RG_SSUSB_RX_EYE_CNT_0
	
	// Eye Monitor Feature
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye1), 7, 1, 0x1);	//RG_SSUSB_RX_EYE_MASK_0 = 0x1
	eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye1)+1, 0xff);			//RG_SSUSB_RX_EYE_MASK_1 = 0xff
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye1)+2, 0, 1, 0x1);	//RG_SSUSB_RX_EYE_MASK_2 = 0x1
	eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 0, 1, 0x1);	//RG_SSUSB_EYE_MON_EN = 1

	// Move X,Y to the top-left corner
	for (cOfst = 0; cOfst >= -64; cOfst--)
	{
		eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 1, 7, cOfst);	//RG_SSUSB_RX_EYE_XOFFSET
	}
	for (cOfst = 0; cOfst < 64; cOfst++)
	{
		eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+2, cOfst);			//RG_SSUSB_RX_EYE0_Y
		eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+1, cOfst);			//RG_SSUSB_RX_EYE1_Y
	}
	//ClearErrorResult
	for(bIdxCycCnt = 0; bIdxCycCnt < CYCLE_COUNT_MAX; bIdxCycCnt++){
		for(bIdxX = 0; bIdxX < ERRCNT_MAX; bIdxX++)
		{
			for(bIdxY = 0; bIdxY < ERRCNT_MAX; bIdxY++){
				pwErrCnt0[bIdxCycCnt][bIdxX][bIdxY] = 0;
				pwErrCnt1[bIdxCycCnt][bIdxX][bIdxY] = 0;
			}
		}
	}
	isContinue = true;
	while(isContinue){
		printk(KERN_ERR ".");
		// The point is within the boundary, then let's check if it is within
	    // the testing region.
	    // The point is only test-able if one of the eye region
	    // includes this point.
	    fgValid = fgEyeScanHelper_CheckPtInRegion(&_rEye1, _bXcurr, _bYcurr)
           || fgEyeScanHelper_CheckPtInRegion(&_rEye2, _bXcurr, _bYcurr);
		// Translate bX and bY to 2's complement from where the origin was on the
		// top left corner.
		// 0x40 and 0x3F needs a bit of thinking!!!! >"<
		cX = (_bXcurr ^ 0x40);
		cY = (_bYcurr ^ 0x3F);

		// Set X if necessary
		if (_fgXChged == true)
		{
			eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 1, 7, cX);	//RG_SSUSB_RX_EYE_XOFFSET
		}
		// Set Y
		eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+2, cY);			//RG_SSUSB_RX_EYE0_Y
		eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+1, cY);			//RG_SSUSB_RX_EYE1_Y

		/// Test this point!
		if (fgValid){
			for (bExtendCnt = 0; bExtendCnt < num_ignore_cnt; bExtendCnt++)
			{
				//run test
				EyeScanHelper_RunTest(info);
			}
			for (bExtendCnt = 0; bExtendCnt < num_cnt; bExtendCnt++)
			{
				EyeScanHelper_RunTest(info);

				wErr0 = eyescan_read_reg8(((PHY_UINT32)&info->u3phyd_regs_c->phya_rx_mon3)+2) + (eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->phya_rx_mon3)+3, 0, 2) << 8);
				wErr1 = eyescan_read_reg8(((PHY_UINT32)&info->u3phyd_regs_c->phya_rx_mon3)) + (eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->phya_rx_mon3)+1, 0, 2) << 8);
				pwErrCnt0[bExtendCnt][_bXcurr][_bYcurr] = wErr0;
				pwErrCnt1[bExtendCnt][_bXcurr][_bYcurr] = wErr1;
#if 0				
				if(_bXcurr == _bYcurr || ((_bXcurr+_bYcurr) == 127)){
					if(isBreak>0){
						break_eyescan(cX, cY, isBreak);
					}
				}
#endif
				//EyeScanHelper_GetResult(&_rRes.pwErrCnt0[bCnt], &_rRes.pwErrCnt1[bCnt]);
//				printk(KERN_ERR "cnt[%d] cur_x,y [0x%x][0x%x], cX,cY [0x%x][0x%x], ErrCnt[%d][%d]\n"
//					, bExtendCnt, _bXcurr, _bYcurr, cX, cY, pwErrCnt0[bExtendCnt][_bXcurr][_bYcurr], pwErrCnt1[bExtendCnt][_bXcurr][_bYcurr]);
			}
			//printk(KERN_ERR "cur_x,y [0x%x][0x%x], cX,cY [0x%x][0x%x], ErrCnt[%d][%d]\n", _bXcurr, _bYcurr, cX, cY, pwErrCnt0[0][_bXcurr][_bYcurr], pwErrCnt1[0][_bXcurr][_bYcurr]);
		}
		else{
			
		}
		if (fgEyeScanHelper_CalNextPoint() == false){
#if 1
			printk(KERN_ERR "Xcurr [0x%x] Ycurr [0x%x]\n", _bXcurr, _bYcurr);
		 	printk(KERN_ERR "XcurrREG [0x%x] YcurrREG [0x%x]\n", cX, cY);
#endif
			printk(KERN_ERR "end of eye scan\n");
		  	isContinue = false;
		}
	}
	printk(KERN_ERR "CurX [0x%x] CurY [0x%x]\n"
		, eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 1, 7)
		, eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+1, 0, 7));
	// Move X,Y to the top-left corner
	for (cOfst = 63; cOfst >= 0; cOfst--)
	{
		eyescan_write_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 1, 7, cOfst);	//RG_SSUSB_RX_EYE_XOFFSET
	}
	for (cOfst = 63; cOfst >= 0; cOfst--)
	{
		eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+2, cOfst);			//RG_SSUSB_RX_EYE0_Y
		eyescan_write_reg8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+1, cOfst);			//RG_SSUSB_RX_EYE1_Y
	}
	printk(KERN_ERR "CurX [0x%x] CurY [0x%x]\n"
		, eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+3, 1, 7)
		, eyescan_read_field8(((PHY_UINT32)&info->u3phyd_regs_c->eq_eye0)+1, 0, 7));
	printk(KERN_ERR "PI result: %d\n", _bPIResult);
	printk(KERN_ERR "pwErrCnt0 addr: 0x%x\n", pwErrCnt0);
	printk(KERN_ERR "pwErrCnt1 addr: 0x%x\n", pwErrCnt1);
	return PHY_TRUE;
}

PHY_INT32 u2_save_cur_en_c60802(struct u3phy_info *info){
	PHY_INT32 temp;
	//****** u2phy part *******//
	//switch to USB function. (system register, force ip into usb mode)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2);
	temp &= ~(0x1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2, temp);
	//Release force suspendm. „³ (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//RG_DPPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp |= 0x1<<6;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DMPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp |= 0x1<<7;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_XCVRSEL[1:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x3<<4);
	temp |= 0x1<<4;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_TERMSEL
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp |= 0x1<<2;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DATAIN[3:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1);
	temp &= ~(0xf<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1, temp);
	//force_dp_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<4;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_dm_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<5;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_xcvrsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<3;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_termsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<1;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_datain
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<7;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//DP/DM BC1.1 path Disable
	//RG_USB20_PHY_REV[7]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3));
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3), temp);
	//OTG Disable
	//wait 800us (1. force_suspendm=1, 2. set wanted utmi value, 3. rg_usb20_pll_stable=1)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3);
	temp &= ~(0x1<<3);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3, temp);
	//(let suspendm=0, set utmi into analog power down )
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<2;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	
	return PHY_TRUE;
}

PHY_INT32 u2_save_cur_re_c60802(struct u3phy_info *info){
	PHY_INT32 temp;
	//****** u2phy part *******//
	//switch to USB function. (system register, force ip into usb mode)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2);
	temp &= ~(0x1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2, temp);
	//Release force suspendm. „³ (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//RG_DPPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x1<<6);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DMPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_XCVRSEL[1:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x3<<4);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_TERMSEL
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DATAIN[3:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1);
	temp &= ~(0xf<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1, temp);
	//force_dp_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<4);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_dm_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<5);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_xcvrsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<3);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_termsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_datain
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//DP/DM BC1.1 path Disable
	//RG_USB20_PHY_REV[7]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3));
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3), temp);
	//OTG Disable
	//wait 800us (1. force_suspendm=1, 2. set wanted utmi value, 3. rg_usb20_pll_stable=1)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3);
	temp |= 0x1<<3;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3, temp);

	return PHY_TRUE;
}

PHY_INT32 u2_slew_rate_calibration_c60802(struct u3phy_info *info){
	PHY_INT32 i=0;
	PHY_INT32 j=0;
	PHY_INT8 u1SrCalVal = 0;
	PHY_INT8 u1Reg_addr_HSTX_SRCAL_EN;
	PHY_INT32 fgRet = 0;	
	PHY_INT32 u4FmOut = 0;	
	PHY_INT32 u4Tmp = 0;
	PHY_INT32 temp;

	// => RG_USB20_HSTX_SRCAL_EN = 1
	// enable HS TX SR calibration
	temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
	temp |= (0x1<<23);
	U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);

	DRV_MSLEEP(1);

	// => RG_FRCK_EN = 1    
	// Enable free run clock
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1));
	temp |= (0x1<<8);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1), temp);

	// => RG_CYCLECNT = 4
	// Setting cyclecnt =4
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0));
	temp &= ~(0xffffff);
	temp |= 0x4;
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0), temp);
	
	// => RG_FREQDET_EN = 1
	// Enable frequency meter
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0));
	temp |= (0x1<<24);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0), temp);

	// wait for FM detection done, set 10ms timeout
	for(i=0; i<10; i++){
		// => u4FmOut = USB_FM_OUT
		// read FM_OUT
		u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr0));
		printk("FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

		// check if FM detection done 
		if (u4FmOut != 0)
		{
			fgRet = 0;
			printk("FM detection done! loop = %d\n", i);
			
			break;
		}

		fgRet = 1;
		DRV_MSLEEP(1);
	}
	// => RG_FREQDET_EN = 0
	// disable frequency meter
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0));
	temp &= ~(0x1<<24);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0), temp);
	// => RG_FRCK_EN = 0
	// disable free run clock
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1));
	temp &= ~(0x1<<8);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1), temp);
	// => RG_USB20_HSTX_SRCAL_EN = 0
	// disable HS TX SR calibration
	temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
	temp &= ~(0x1<<23);
	U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);
	DRV_MSLEEP(1);

	if(u4FmOut == 0){
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
		temp &= ~(0x7<<16);
		temp |= (0x4<<16);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);

		fgRet = 1;
	}
	else{
		// set reg = (1024/FM_OUT) * 25 * 0.028 (round to the nearest digits)
		u4Tmp = (((1024 * 25 * U2_SR_COEF_C60802) / u4FmOut) + 500) / 1000;
		printk("SR calibration value u1SrCalVal = %d\n", (PHY_UINT8)u4Tmp);

		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
		temp &= ~(0x7<<16);
		temp |= (u4Tmp<<16);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);
	}
	return fgRet;
}

#endif
