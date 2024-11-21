#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include "wm8960.h"
#include "i2s_ctrl.h"
#include <linux/delay.h>


extern void i2c_WM8751_write(unsigned int address, unsigned int data);
unsigned long wmcodec_reg_data[56];

void wmcodec_write(int reg_addr, unsigned long reg_data)
{
	wmcodec_reg_data[reg_addr] = reg_data;
	printk("[WM8960(%02X)=0x%08X]\n",(unsigned int)reg_addr,(unsigned int)reg_data);
	i2c_WM8751_write(reg_addr, reg_data);
	return;
}

/* Reset and power up the WM8960 */
void audiohw_preinit(void)
{
	memset(wmcodec_reg_data, 0 , sizeof(unsigned long)*55);

	wmcodec_write(RESET, RESET_RESET);    /* Reset (0x0F) */
	
 	mdelay(50);	
	wmcodec_reg_data[RESET] = 0xFFFF;
	mdelay(50);	
}

void audiohw_set_apll(int srate)
{
	unsigned long data;

	if((srate==8000) || (srate==12000) || (srate==16000) || (srate==24000) || (srate==32000) || (srate==48000))
	{
		/* Provide 12.288MHz SYSCLK */
		data = wmcodec_reg_data[PLL1];	
        	wmcodec_write(PLL1, data | PLL1_OPCLKDIV_1 | PLL1_SDM_FRACTIONAL | PLL1_PLLPRESCALE_1 | PLL1_PLLN(0x8));   /* PLL1 (0x34)*/
        	
		wmcodec_write(PLL2, PLL2_PLLK_23_16(0x31));  /* PLL2 (0x35) */
        	wmcodec_write(PLL3, PLL3_PLLK_15_8(0x26));  /* PLL3 (0x36)*/
		wmcodec_write(PLL4, PLL4_PLLK_7_0(0xe9));  /* PLL4 (0x37)*/
	}
	else if ((srate==11025) || (srate==22050) || (srate==44100))
	{
		/* Provide 11.2896MHz SYSCLK */
		data = wmcodec_reg_data[PLL1];	
        	wmcodec_write(PLL1, data | PLL1_OPCLKDIV_1 | PLL1_SDM_FRACTIONAL | PLL1_PLLPRESCALE_1 | PLL1_PLLN(0x7));   /* PLL1 (0x34)*/
        	
		wmcodec_write(PLL2, PLL2_PLLK_23_16(0x89));  /* PLL2 (0x35) */
        	wmcodec_write(PLL3, PLL3_PLLK_15_8(0xc2));  /* PLL3 (0x36)*/
		wmcodec_write(PLL4, PLL4_PLLK_7_0(0x26));  /* PLL4 (0x37)*/
	}
	else
	{
		printk("Not support this srate\n");
	}
	mdelay(3);
}


void audiohw_set_frequency(int fsel, int pll_en)
{
        MSG("audiohw_set_frequency=0x%08X\n",fsel);

	if (pll_en)
	{
		printk("PLL enable\n");
		wmcodec_write(CLOCKING1, (fsel<<3) | CLOCKING1_SYSCLKDIV_2 | CLOCKING1_CLKSEL_PLL);  /* CLOCKING (0x04)=>0x05 */

	}
	else
	{
		printk("PLL disable\n");
		wmcodec_write(CLOCKING1, (fsel<<3));//| CLOCKING1_SYSCLKDIV_2);  /* CLOCKING (0x04) */
	}
	
}

/* FIXME */
int audiohw_set_lineout_vol(int Aout, int vol_l, int vol_r)
{
	MSG("audiohw_set_lineout_vol\n");
	switch(Aout)
	{
	case 1:
		//wmcodec_write(LOUT1, LOUT1_LO1VU|LOUT1_LO1ZC|LOUT1_LOUT1VOL(0x7f)); /* LOUT1(0x02) */
		//wmcodec_write(ROUT1, ROUT1_RO1VU|ROUT1_RO1ZC|ROUT1_ROUT1VOL(0x7f)); /* ROUT1(0x03) */
		wmcodec_write(LOUT1, LOUT1_LO1VU|LOUT1_LO1ZC|LOUT1_LOUT1VOL(vol_l)); /* LOUT1(0x02) */
		wmcodec_write(ROUT1, ROUT1_RO1VU|ROUT1_RO1ZC|ROUT1_ROUT1VOL(vol_r)); /* ROUT1(0x03) */
		break;
	case 2:
    		wmcodec_write(LSPK, LSPK_SPKLVU|LSPK_SPKLZC| LSPK_SPKLVOL(vol_l));
    		wmcodec_write(RSPK, RSPK_SPKRVU|RSPK_SPKRZC| RSPK_SPKRVOL(vol_r));
		break;
	default:
		break;
	}	
    	return 0;
}

/* FIXME */
int audiohw_set_linein_vol(int vol_l, int vol_r)
{
	MSG("audiohw_set_linein_vol\n");
	
    	wmcodec_write(LINV, LINV_IPVU|LINV_LINVOL(vol_l)); /* LINV(0x00)=>0x12b */
	wmcodec_write(RINV, RINV_IPVU|RINV_RINVOL(vol_r)); /* LINV(0x01)=>0x12b */

    	return 0;
}

/* Set signal path*/
int audiohw_postinit(int bSlave, int AIn, int AOut, int pll_en, int wordLen24b)
{

	int i;
	unsigned long data;

	if(wmcodec_reg_data[RESET]!=0xFFFF)
    	return 0;
	
	if(bSlave)
	{ 
		MSG("WM8960 slave.....\n");
		if(wordLen24b)
		{
			printk("24 bit word length\n");
			wmcodec_write(AINTFCE1, AINTFCE1_WL_24 | AINTFCE1_FORMAT_I2S); /* AINTFCE1(0x07) */
		}
		else
		{
			printk("16 bit word length\n");
			wmcodec_write(AINTFCE1, AINTFCE1_WL_16 | AINTFCE1_FORMAT_I2S); /* AINTFCE1(0x07) */
		}
	}	
	else
	{
		MSG("WM8960 master.....\n");
		wmcodec_write(CLOCKING2, 0x1c4);//CLOCKING2_BCLKDIV(0x1c4));  /* CLOCKING2(0x08) */

		if(wordLen24b)
		{
			printk("24 bit word length\n");
			wmcodec_write(AINTFCE1, AINTFCE1_MS | AINTFCE1_WL_24 | AINTFCE1_FORMAT_I2S); /*AINTFCE1(0x07) */
		}
		else
		{
			printk("16 bit word length\n");
			wmcodec_write(AINTFCE1, AINTFCE1_MS | AINTFCE1_WL_16 | AINTFCE1_FORMAT_I2S); /* AINTFCE1(0x07) */
		}
		mdelay(5);
	}

	
	/* From app notes: allow Vref to stabilize to reduce clicks */
	for(i = 0; i < 1000*HZ; i++);
	
	if(AIn > 0)
	{
       		data = wmcodec_reg_data[PWRMGMT1];
   		wmcodec_write(PWRMGMT1, data|PWRMGMT1_ADCL|PWRMGMT1_ADCR|PWRMGMT1_AINL |PWRMGMT1_AINR);//|PWRMGMT1_MICB);/* PWRMGMT1(0x19) */

		data = wmcodec_reg_data[ADDITIONAL1];
		wmcodec_write(ADDITIONAL1, data|ADDITIONAL1_DATSEL(0x01)); /* ADDITIONAL1(0x17) */
		wmcodec_write(LADCVOL, LADCVOL_LAVU_EN|LADCVOL_LADCVOL(0xc3)); /* LADCVOL(0x15) */
		wmcodec_write(RADCVOL, RADCVOL_RAVU_EN|RADCVOL_RADCVOL(0xc3)); /* RADCVOL(0x16) */
		wmcodec_write(ADCLPATH, ADCLPATH_LMN1|ADCLPATH_LMIC2B);//|ADCLPATH_LMICBOOST_13DB); /* ADCLPATH(0x20)=>(0x108)*/
		wmcodec_write(ADCRPATH, ADCRPATH_RMN1|ADCRPATH_RMIC2B);//|ADCRPATH_RMICBOOST_13DB); /* ADCRPATH(0x21)=>(0x108)*/
		wmcodec_write(PWRMGMT3, PWRMGMT3_LMIC|PWRMGMT3_RMIC); /* PWRMGMT3(0x2f) */
	
		//wmcodec_write(LINBMIX, 0x000); /* LINBMIX(0x2B) */
		
		if (AOut<=0)
		{
			wmcodec_write(AINTFCE2, 0x40); /* FIXME:(0x09) */

			data = wmcodec_reg_data[PWRMGMT2];
			if(pll_en)
			{
				wmcodec_write(PWRMGMT2, data|PWRMGMT2_PLL_EN|PWRMGMT2_DACL|PWRMGMT2_DACR); /* PWRMGMT2(0x1a) */
			}
			else
			{
				wmcodec_write(PWRMGMT2, data|PWRMGMT2_DACL|PWRMGMT2_DACR); /* PWRMGMT2(0x1a) */

			}
		}
	}
	if(AOut>0)
	{
		/* Power management 2 setting */
		data = wmcodec_reg_data[PWRMGMT2];

		if(pll_en)
		{
			wmcodec_write(PWRMGMT2, data|PWRMGMT2_PLL_EN|PWRMGMT2_DACL|PWRMGMT2_DACR|PWRMGMT2_LOUT1|PWRMGMT2_ROUT1|PWRMGMT2_SPKL|PWRMGMT2_SPKR); /* PWRMGMT2(0x1a) */
		}
		else
		{
			wmcodec_write(PWRMGMT2, data|PWRMGMT2_DACL|PWRMGMT2_DACR|PWRMGMT2_LOUT1|PWRMGMT2_ROUT1|PWRMGMT2_SPKL|PWRMGMT2_SPKR); /* PWRMGMT2(0x1a) */

		}
		
		mdelay(10);

		wmcodec_write(AINTFCE2, 0x40); /* FIXME:(0x09) */

		wmcodec_write(LEFTGAIN, LEFTGAIN_LDVU|LEFTGAIN_LDACVOL(0xff)); /* LEFTGAIN(0x0a) */
		wmcodec_write(RIGHTGAIN, RIGHTGAIN_RDVU|RIGHTGAIN_RDACVOL(0xff)); /* RIGHTGAIN(0x0b)*/

		wmcodec_write(LEFTMIX1, 0x100);  /* LEFTMIX1(0x22) */
		wmcodec_write(RIGHTMIX2, 0x100); /* RIGHTMIX2(0x25) */

		data = wmcodec_reg_data[PWRMGMT3]; /*FIXME*/
		wmcodec_write(PWRMGMT3, data|PWRMGMT3_ROMIX|PWRMGMT3_LOMIX); /* PWRMGMT3(0x2f) */

		data = wmcodec_reg_data[CLASSDCTRL1]; /* CLASSDCTRL1(0x31) SPEAKER FIXME*/
		wmcodec_write(CLASSDCTRL1, 0xf7);//data|CLASSDCTRL1_OP_LRSPK);

		data = wmcodec_reg_data[CLASSDCTRL3];	/* CLASSDCTRL3(0x33) */
		wmcodec_write(CLASSDCTRL3, 0xad);//data|(0x1b));
	}

	wmcodec_write(DACCTRL1, 0x000);  /* DACCTRL1(0x05) */

	data = wmcodec_reg_data[PWRMGMT1];
	wmcodec_write(PWRMGMT1, data|0x1c0); /* FIXME:PWRMGMT1(0x19)*/
	

	printk("WM8960 All initial ok!\n");

	return 0;
	
}

void audiohw_micboost(int boostgain)
{
	unsigned long data;

	data =  wmcodec_reg_data[ADCLPATH];
	wmcodec_write(ADCLPATH, data|(boostgain << 4));

	data =  wmcodec_reg_data[ADCRPATH];
	wmcodec_write(ADCRPATH, data|(boostgain << 4));
}

void audiohw_micin(int enableMic)
{
	unsigned long data;
	
	if (enableMic==1)
	{
		data =  wmcodec_reg_data[PWRMGMT1];
		wmcodec_write(PWRMGMT1, data|PWRMGMT1_MICB);
	}
#if 1
	else
	{
		data =  wmcodec_reg_data[PWRMGMT1];
		wmcodec_write(PWRMGMT1, data & (~(PWRMGMT1_MICB)));
	}
#endif
}

void audiohw_mute(bool mute)
{
    /* Mute:   Set DACMU = 1 to soft-mute the audio DACs. */
    /* Unmute: Set DACMU = 0 to soft-un-mute the audio DACs. */
    wmcodec_write(DACCTRL1, mute ? DACCTRL1_DACMU : 0);
}


/* Nice shutdown of WM8960 codec */
void audiohw_close(void)
{
	wmcodec_write(DACCTRL1,DACCTRL1_DACMU); /* 0x05->0x08 */
	wmcodec_write(PWRMGMT1, 0x000); /* 0x19->0x000 */
	mdelay(400);
	wmcodec_write(PWRMGMT2, 0x000); /* 0x1a->0x000 */

}

void audiohw_loopback(void)
{
}

void audiohw_codec_exlbk(void)
{
	memset(wmcodec_reg_data, 0 , sizeof(unsigned long)*55);

	wmcodec_write(LINV, 0x117); /* 0x00->0x117 */
	wmcodec_write(RINV, 0x117); /* 0x01->0x117 */
	wmcodec_write(LOUT1, 0x179); /* 0x02->0x179 */
	wmcodec_write(ROUT1, 0x179); /* 0x03->0x179 */
	wmcodec_write(CLOCKING1, 0x00); /* 0x04->0x00 */
	//wmcodec_write(CLOCKING1, 0x40); /* 0x04->0x00 */
	wmcodec_write(DACCTRL1, 0x00); /* 0x05->0x00 */
	wmcodec_write(AINTFCE2, 0x41); /* 0x09->0x41 */
	wmcodec_write(LADCVOL, 0x1c3); /* 0x15->0x1c3 */
	wmcodec_write(RADCVOL, 0x1c3); /* 0x16->0x1c3 */
	wmcodec_write(PWRMGMT1, 0xfc); /* 0x19->0xfc */
	wmcodec_write(PWRMGMT2, 0x1e0); /* 0x1a->0x1e0 */
	wmcodec_write(ADCLPATH, 0x108); /* 0x20->0x108 */
	wmcodec_write(ADCRPATH, 0x108); /* 0x21->0x108 */
	wmcodec_write(LEFTMIX1, 0x150); /* 0x22->0x150 */
	wmcodec_write(RIGHTMIX2, 0x150); /* 0x25->0x150 */
	wmcodec_write(BYPASS1, 0x00); /* 0x2d->0x00 */
	wmcodec_write(BYPASS2, 0x00); /* 0x2e->0x00 */
	wmcodec_write(PWRMGMT3, 0x3c); /* 0x2f->0x3c */
}

void audiohw_bypass(void)
{
	int i;

	memset(wmcodec_reg_data, 0 , sizeof(unsigned long)*55);
	wmcodec_write(RESET, 0x000);    /* 0x0f(R15)->0x000 */
	
	for(i = 0; i < 1000*HZ; i++);

	wmcodec_write(PWRMGMT1, 0xf0); /* 0x19(R25)->0xf0 */
	wmcodec_write(PWRMGMT2, 0x60); /* 0x1a(R26)->0x60 */
	wmcodec_write(PWRMGMT3, 0x3c); /* 0x2f(R47)->0x3c */
	wmcodec_write(LINV, 0x117); /*  0x00(R0)->0x117 */
	wmcodec_write(RINV, 0x117); /*  0x01(R1)->0x117 */
	wmcodec_write(ADCLPATH, 0x108); /* 0x20(R32)->0x108 */
	wmcodec_write(ADCRPATH, 0x108); /* 0x21(R33)->0x108 */
	wmcodec_write(BYPASS1, 0x80); /* 0x2d(R45)->0x80 */
	wmcodec_write(BYPASS2, 0x80); /* 0x2e(R46)->0x80 */
	wmcodec_write(LOUT1, 0x179); /*  0x02(R2)->0x179 */
	wmcodec_write(ROUT1, 0x179); /*  0x03(R3)->0x179 */
}

EXPORT_SYMBOL(audiohw_set_frequency);
EXPORT_SYMBOL(audiohw_close);
EXPORT_SYMBOL(audiohw_postinit);
EXPORT_SYMBOL(audiohw_preinit);
EXPORT_SYMBOL(audiohw_set_apll);
EXPORT_SYMBOL(audiohw_codec_exlbk);
EXPORT_SYMBOL(audiohw_bypass);
