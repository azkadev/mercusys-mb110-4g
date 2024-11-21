#ifdef CONFIG_D60802_SUPPORT
#include "mtk-phy.h"
#include "mtk-phy-d60802.h"

PHY_INT32 phy_init_d60802(struct u3phy_info *info){
	PHY_INT8 temp;

	DRV_MDELAY(100);
	/**********u2phy part******************/
	//manual set U2 slew rate ctrl = 4	
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5)+1);
	temp &= ~(0x7<<4);
	temp |= (0x4<<4);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5)+1, temp);
	DRV_MDELAY(100);
	//fine tune SQTH to gain margin in U2 Rx sensitivity compliance test
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr6));
	temp &= ~(0xf);
	temp |= 0x4;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr6), temp);
	DRV_MDELAY(100);
	//disable VBUS CMP to save power since no OTG function
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr6)+2);
	temp &= ~(0x1<<4);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr6)+2, temp);
	DRV_MDELAY(100);

	/*********phyd part********************/
	//shorten Tx drive stable delay time from 82us -> 25us
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_mix1)+1);
	temp &= ~(0x3f);
	temp |= 0x13;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_mix1)+1, temp);
	DRV_MDELAY(100);
	//The same Rx LFPS detect period  rxlfps_upb as A ver
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_lfps0));
	temp &= ~(0x1f);
	temp |= 0x19;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_lfps0), temp);
	DRV_MDELAY(100);
	//No switch to Lock 5g @tx_lfps enable 
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_lfps0)+1);
	temp |= (0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_lfps0)+1, temp);
	DRV_MDELAY(100);
	//disable DFE to improve Rx JT
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_rx0));
	temp &= ~(0x1<<1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_rx0), temp);
	DRV_MDELAY(100);
	//calibrate CDR offset every time enter TSEQ
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_mix2)+2);
	temp |= (0x1<<2);
	temp = U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_mix2)+2, temp);
	DRV_MDELAY(100);
	//Re-Calibration after exit P3 state
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_pll_0));
	temp |= (0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_d->phyd_pll_0), temp);
	DRV_MDELAY(100);

	/**************phyd bank2 part************/
	//Disable E-Idle Low power mode
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_d->b2_phyd_top1)+2);
	temp &= ~(0x3);
	temp |= 0x1;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_d->b2_phyd_top1)+2, temp);
	DRV_MDELAY(100);

	/**************phya part******************/
	//modify Tx det Rx Vth to work around the threshold back to 200mV
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phya_regs_d->reg5)+1);
	temp &= ~(0x3<<2);
	temp |= (0x2<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_regs_d->reg5)+1, temp);
	DRV_MDELAY(100);
	//modify Tx det Rx Vth to work around the threshold back to 200mV
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phya_regs_d->reg5)+1);
	temp &= ~(0x3);
	temp |= 0x2;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_regs_d->reg5)+1, temp);
	DRV_MDELAY(100);

	/*************phya da part*****************/
	//set to pass SSC min in electrical compliance
	temp = 0x47;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_da_regs_d->reg21)+2, temp);
	DRV_MDELAY(100);
	//set R step 1 = 2 to improve Rx JT
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phya_da_regs_d->reg32)+2);
	temp &= ~(0x3<<6);
	temp |= (0x2<<6);
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_da_regs_d->reg32)+2, temp);

	/*************phy chip part******************/
	//Power down bias at P3
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlb)+3);
	temp |= 0x1;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlb)+3, temp);
	DRV_MDELAY(100);
	// PIPE drv = 2
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlc)+2, 0x10);
	DRV_MDELAY(100);
	// PIPE phase
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlc)+3, 0x44);
	DRV_MDELAY(100);
	return PHY_TRUE;
}

#define PHY_DRV_SHIFT	3
#define PHY_PHASE_SHIFT	3
#define PHY_PHASE_DRV_SHIFT	1
PHY_INT32 phy_change_pipe_phase_d60802(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase){
	PHY_INT32 drv_reg_value;
	PHY_INT32 phase_reg_value;
	PHY_INT32 temp;

	drv_reg_value = phy_drv << PHY_DRV_SHIFT;
	phase_reg_value = (pipe_phase << PHY_PHASE_SHIFT) | (phy_drv << PHY_PHASE_DRV_SHIFT);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlc)+2);
	temp &= ~(0x3 << PHY_DRV_SHIFT);
	temp |= drv_reg_value;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlc)+2, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlc)+3);
	temp &= ~((0x3 << PHY_PHASE_DRV_SHIFT) | (0x1f << PHY_PHASE_SHIFT));
	temp |= phase_reg_value;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_d->gpio_ctlc)+3, temp);

	return PHY_TRUE;
}

PHY_INT32 u2_slew_rate_calibration_d60802(struct u3phy_info *info){
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
	temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5));
	temp |= (0x1<<23);
	U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5), temp);

	DRV_MSLEEP(1);

	// => RG_FRCK_EN = 1    
	// Enable free run clock
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmmonr1));
	temp |= (0x1<<8);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmmonr1), temp);

	// => RG_CYCLECNT = 4
	// Setting cyclecnt =4
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmcr0));
	temp &= ~(0xffffff);
	temp |= 0x4;
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmcr0), temp);
	
	// => RG_FREQDET_EN = 1
	// Enable frequency meter
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmcr0));
	temp |= (0x1<<24);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmcr0), temp);

	// wait for FM detection done, set 10ms timeout
	for(i=0; i<10; i++){
		// => u4FmOut = USB_FM_OUT
		// read FM_OUT
		u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmmonr0));
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
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmcr0));
	temp &= ~(0x1<<24);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmcr0), temp);
	// => RG_FRCK_EN = 0
	// disable free run clock
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmmonr1));
	temp &= ~(0x1<<8);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_d->fmmonr1), temp);
	// => RG_USB20_HSTX_SRCAL_EN = 0
	// disable HS TX SR calibration
	temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr0));
	temp &= ~(0x1<<23);
	U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr0), temp);
	DRV_MSLEEP(1);

	if(u4FmOut == 0){
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5));
		temp &= ~(0x7<<16);
		temp |= (0x4<<16);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5), temp);

		fgRet = 1;
	}
	else{
		// set reg = (1024/FM_OUT) * 25 * 0.028 (round to the nearest digits)
		u4Tmp = (((1024 * 25 * U2_SR_COEF_D60802) / u4FmOut) + 500) / 1000;
		printk("SR calibration value u1SrCalVal = %d\n", (PHY_UINT8)u4Tmp);

		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5));
		temp &= ~(0x7<<16);
		temp |= (u4Tmp<<16);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_d->usbphyacr5), temp);
	}
	return fgRet;
}


#endif
