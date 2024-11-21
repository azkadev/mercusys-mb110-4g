#ifdef CONFIG_D60802_SUPPORT
#ifndef __MTK_PHY_D60802_H
#define __MTK_PHY_D60802_H

#define U2_SR_COEF_D60802 28

struct u2phy_reg_d {
	//0x0
	PHY_LE32 usbphyacr0;
	PHY_LE32 usbphyacr1;
	PHY_LE32 usbphyacr2;
	PHY_LE32 reserve0;
	//0x10
	PHY_LE32 usbphyacr4;
	PHY_LE32 usbphyacr5;
	PHY_LE32 usbphyacr6;
	PHY_LE32 u2phyacr3;
	//0x20
	PHY_LE32 u2phyacr4;
	PHY_LE32 u2phyamon0;
	PHY_LE32 reserve1[2];
	//0x30~0x50
	PHY_LE32 reserve2[12];
	//0x60
	PHY_LE32 u2phydcr0;
	PHY_LE32 u2phydcr1;
	PHY_LE32 u2phydtm0;
	PHY_LE32 u2phydtm1;
	//0x70
	PHY_LE32 u2phydmon0;
	PHY_LE32 u2phydmon1;
	PHY_LE32 u2phydmon2;
	PHY_LE32 u2phydmon3;
	//0x80
	PHY_LE32 u2phybc12c;
	PHY_LE32 u2phybc12c1;
	PHY_LE32 reserve3[2];
	//0x90~0xd0
	PHY_LE32 reserve4[20];
	//0xe0
	PHY_LE32 regfppc;
	PHY_LE32 reserve5[3];
	//0xf0
	PHY_LE32 versionc;
	PHY_LE32 reserve6[2];
	PHY_LE32 regfcom;
};

struct u3phya_reg_d {
	//0x0
	PHY_LE32 reg0;
	PHY_LE32 reg1;
	PHY_LE32 reg2;
	PHY_LE32 reg3;
	//0x10
	PHY_LE32 reg4;
	PHY_LE32 reg5;
	PHY_LE32 reg6;
	PHY_LE32 reg7;
	//0x20
	PHY_LE32 reg8;
	PHY_LE32 reg9;
	PHY_LE32 rega;
	PHY_LE32 regb;
	//0x30
	PHY_LE32 regc;
};

struct u3phya_da_reg_d {
	//0x0
	PHY_LE32 reg0;
	PHY_LE32 reg1;
	PHY_LE32 reg4;
	PHY_LE32 reg5;
	//0x10
	PHY_LE32 reg6;
	PHY_LE32 reg7;
	PHY_LE32 reg8;
	PHY_LE32 reg9;
	//0x20
	PHY_LE32 reg10;
	PHY_LE32 reg12;
	PHY_LE32 reg13;
	PHY_LE32 reg14;
	//0x30
	PHY_LE32 reg15;
	PHY_LE32 reg16;
	PHY_LE32 reg19;
	PHY_LE32 reg20;
	//0x40
	PHY_LE32 reg21;
	PHY_LE32 reg23;
	PHY_LE32 reg25;
	PHY_LE32 reg26;
	//0x50
	PHY_LE32 reg28;
	PHY_LE32 reg29;
	PHY_LE32 reg30;
	PHY_LE32 reg31;
	//0x60
	PHY_LE32 reg32;
	PHY_LE32 reg33;
};

struct u3phyd_reg_d {
	//0x0
	PHY_LE32 phyd_mix0;
	PHY_LE32 phyd_mix1;
	PHY_LE32 phyd_lfps0;
	PHY_LE32 phyd_lfps1;
	//0x10
	PHY_LE32 phyd_impcal0;
	PHY_LE32 phyd_impcal1;
	PHY_LE32 phyd_txpll0;
	PHY_LE32 phyd_txpll1;
	//0x20
	PHY_LE32 phyd_txpll2;
	PHY_LE32 phyd_fl0;
	PHY_LE32 phyd_mix2;
	PHY_LE32 phyd_rx0;
	//0x30
	PHY_LE32 phyd_t2rlb;
	PHY_LE32 phyd_cppat;
	PHY_LE32 phyd_mix3;
	PHY_LE32 phyd_ebufctl;
	//0x40
	PHY_LE32 phyd_pipe0;
	PHY_LE32 phyd_pipe1;
	PHY_LE32 phyd_mix4;
	PHY_LE32 phyd_ckgen0;
	//0x50
	PHY_LE32 phyd_mix5;
	PHY_LE32 phyd_reserved;
	PHY_LE32 phyd_cdr0;
	PHY_LE32 phyd_cdr1;
	//0x60
	PHY_LE32 phyd_pll_0;
	PHY_LE32 phyd_pll_1;
	PHY_LE32 phyd_bcn_det_1;
	PHY_LE32 phyd_bcn_det_2;
	//0x70
	PHY_LE32 eq0;
	PHY_LE32 eq1;
	PHY_LE32 eq2;
	PHY_LE32 eq3;
	//0x80
	PHY_LE32 eq_eye0;
	PHY_LE32 eq_eye1;
	PHY_LE32 eq_eye2;
	PHY_LE32 eq_dfe0;
	//0x90
	PHY_LE32 eq_dfe1;
	PHY_LE32 eq_dfe2;
	PHY_LE32 eq_dfe3;
	PHY_LE32 reserve0;
	//0xa0
	PHY_LE32 phyd_mon0;
	PHY_LE32 phyd_mon1;
	PHY_LE32 phyd_mon2;
	PHY_LE32 phyd_mon3;
	//0xb0
	PHY_LE32 phyd_mon4;
	PHY_LE32 phyd_mon5;
	PHY_LE32 phyd_mon6;
	PHY_LE32 phyd_mon7;
	//0xc0
	PHY_LE32 phya_rx_mon0;
	PHY_LE32 phya_rx_mon1;
	PHY_LE32 phya_rx_mon2;
	PHY_LE32 phya_rx_mon3;
	//0xd0
	PHY_LE32 phya_rx_mon4;
	PHY_LE32 phya_rx_mon5;
	PHY_LE32 phyd_cppat2;
	PHY_LE32 eq_eye3;
	//0xe0
	PHY_LE32 kband_out;
	PHY_LE32 kband_out1;
};

struct u3phyd_bank2_reg_d {
	//0x0
	PHY_LE32 b2_phyd_top1;
	PHY_LE32 b2_phyd_top2;
	PHY_LE32 b2_phyd_top3;
	PHY_LE32 b2_phyd_top4;
	//0x10
	PHY_LE32 b2_phyd_top5;
	PHY_LE32 b2_phyd_top6;
	PHY_LE32 b2_phyd_top7;
	PHY_LE32 b2_phyd_p_sigdet1;
	//0x20
	PHY_LE32 b2_phyd_p_sigdet2;
	PHY_LE32 b2_phyd_p_sigdet_cal1;
	PHY_LE32 b2_phyd_rxdet1;
	PHY_LE32 b2_phyd_rxdet2;
	//0x30
	PHY_LE32 b2_phyd_misc0;
	PHY_LE32 b2_phyd_misc2;
	PHY_LE32 b2_phyd_misc3;
	PHY_LE32 reserve0;
	//0x40
	PHY_LE32 b2_rosc_0;
	PHY_LE32 b2_rosc_1;
	PHY_LE32 b2_rosc_2;
	PHY_LE32 b2_rosc_3;
	//0x50
	PHY_LE32 b2_rosc_4;
	PHY_LE32 b2_rosc_5;
	PHY_LE32 b2_rosc_6;
	PHY_LE32 b2_rosc_7;
	//0x60
	PHY_LE32 b2_rosc_8;
	PHY_LE32 b2_rosc_9;
	PHY_LE32 b2_rosc_a;
	PHY_LE32 reserve1;
	//0x70~0xd0
	PHY_LE32 reserve2[28];
	//0xe0
	PHY_LE32 phyd_version;
	PHY_LE32 phyd_model;
};

struct sifslv_chip_reg_d {
	//0x0
	PHY_LE32 gpio_ctla;
	PHY_LE32 gpio_ctlb;
	PHY_LE32 gpio_ctlc;
};

struct sifslv_fm_feg_d {
	PHY_LE32 fmcr0;
	PHY_LE32 fmcr1;
	PHY_LE32 fmcr2;
	PHY_LE32 fmmonr0;
	PHY_LE32 fmmonr1;
};

PHY_INT32 phy_init_d60802(struct u3phy_info *info);
PHY_INT32 phy_change_pipe_phase_d60802(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase);
PHY_INT32 phy_eyescan_d60802(struct u3phy_info *info, PHY_INT32 x_t1, PHY_INT32 y_t1, PHY_INT32 x_br, PHY_INT32 y_br, PHY_INT32 delta_x, PHY_INT32 delta_y
		, PHY_INT32 eye_cnt, PHY_INT32 num_cnt, PHY_INT32 PI_cal_en, PHY_INT32 num_ignore_cnt);
PHY_INT32 u2_slew_rate_calibration_d60802(struct u3phy_info *info);
#endif
#endif
