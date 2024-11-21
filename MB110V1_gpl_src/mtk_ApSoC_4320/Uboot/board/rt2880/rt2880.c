/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
//#include "LzmaDecode.h"

//#define MAX_SDRAM_SIZE	(64*1024*1024)
//#define MIN_SDRAM_SIZE	(8*1024*1024)
#define MAX_SDRAM_SIZE	(256*1024*1024)
#define MIN_SDRAM_SIZE	(8*1024*1024)

#ifdef SDRAM_CFG_USE_16BIT
#define MIN_RT2880_SDRAM_SIZE	(16*1024*1024)
#else
#define MIN_RT2880_SDRAM_SIZE	(32*1024*1024)
#endif

#if defined(TP_MODEL_MR100V1) || defined(TP_MODEL_MR100V2) || defined(TP_MODEL_MB110V1)
#define POWER_LED_GPIO				(39)
#define INTERNET_LED_GPIO			(40)
#define WPS_LED_GPIO				(41)
#define LAN_LED_GPIO				(42)
#define SIGNAL_S1_LED_GPIO			(43)
#define SIGNAL_S2_LED_GPIO			(44)
#define SIGNAL_S3_LED_GPIO			(46)

#define WPS_RST_BUTTON_GPIO  		(38)
#define WIFI_BUTTON_GPIO  			(46)

#define USB_POWER_GPIO  			(36)
#endif

#if defined(TP_MODEL_MR100V2) ||  defined(TP_MODEL_MB110V1)

#define USB_BOOT_GPIO  				(3)
#define USB_RESET_GPIO  			(4)
#define USB_DC_GPIO	  				(37)
#endif
#if 0
#if defined(TP_MODEL_MR6400V3)

#define POWER_LED_GPIO 				(37)
#define INTERNET_LED_GPIO			(39)
#define WPS_LED_GPIO				(40)
#define LAN_LED_GPIO				(41)
#define SIGNAL_S1_LED_GPIO			(42)
#define SIGNAL_S2_LED_GPIO			(43)
#define SIGNAL_S3_LED_GPIO			(44)

#define WPS_RST_BUTTON_GPIO  		(38)
#define WIFI_BUTTON_GPIO  			(46)
#endif

#if defined(TP_MODEL_MR200V3)
#define POWER_LED_GPIO 				(39)
#define INTERNET_LED_GPIO			(40)
#define WPS_LED_GPIO				(4)
#define LAN_LED_GPIO				(5)
#define SIGNAL_S1_LED_GPIO			(41)
#define SIGNAL_S2_LED_GPIO			(42)
#define SIGNAL_S3_LED_GPIO			(43)

#define WPS_RST_BUTTON_GPIO  		(38)
#define WIFI_BUTTON_GPIO  			(46)
#endif

#endif

#if defined(INCLUDE_COMBINED_IMAGE)
int POWER_LED_GPIO;
int INTERNET_LED_GPIO;
int WPS_LED_GPIO;
int LAN_LED_GPIO;
int SIGNAL_S1_LED_GPIO;
int SIGNAL_S2_LED_GPIO;
int SIGNAL_S3_LED_GPIO;

int WPS_RST_BUTTON_GPIO;
int WIFI_BUTTON_GPIO;
#endif

/* GPIO
* GPIO36: POWER
* GPIO37:WPS
* GPIO38:WPS/RESET Button
* GPIO41:LAN
* GPIO43:INTERNET
* GPIO44:WLAN(2.4G)
*/

//#ifdef TP_MODEL_MR3420V5
#if defined(TP_MODEL_MR3420V5)
#define POWER_LED_GPIO  			(2)
#define USB_LED_GPIO  				(3)
#define INTERNET_LED_ORANGE_GPIO 	(4)
#define INTERNET_LED_GREEN_GPIO 	(5)
#define WPS_LED_GPIO  				(37)
#define LAN_LED_GPIO  				(41)
#define WLAN_LED_GPIO  				(44)

#define WPS_RST_BUTTON_GPIO  		(38)
#define WIFI_BUTTON_GPIO  			(46)
#endif

#ifdef TP_MODEL_MR3020V3
#define POWER_LED_GPIO  			(37)
#define INTERNET_LED_GREEN_GPIO 	(43)
#define WPS_LED_GPIO  				(2)
#define LAN_LED_GPIO  				(3)
#define WLAN_LED_GPIO  				(44)

#define WPS_RST_BUTTON_GPIO  		(38)
#endif

#ifdef TP_MODEL_WR840NV4
#define POWER_LED_GPIO  			(36)
#define WPS_LED_GPIO  				(37)
#define RESET_LED_GPIO  			(38)
#define LAN_LED_GPIO  				(41)
#define INTERNET_LED_GPIO   		(43)
#define WLAN_LED_GPIO  				(44)
#endif


/*
 * Check memory range for valid RAM. A simple memory test determines
 * the actually available RAM size between addresses `base' and
 * `base + maxsize'.
 */
long get_ram_size(volatile long *base, long maxsize)
{
	volatile long *addr;
	long           save[32];
	long           cnt;
	long           val;
	long           size;
	int            i = 0;

	for (cnt = (maxsize / sizeof (long)) >> 1; cnt > 0; cnt >>= 1) {
		addr = base + cnt;	/* pointer arith! */
		save[i++] = *addr;
		
		*addr = ~cnt;

		
	}

	addr = base;
	save[i] = *addr;

	*addr = 0;

	
	if ((val = *addr) != 0) {
		/* Restore the original data before leaving the function.
		 */
		*addr = save[i];
		for (cnt = 1; cnt < maxsize / sizeof(long); cnt <<= 1) {
			addr  = base + cnt;
			*addr = save[--i];
		}
		return (0);
	}

	for (cnt = 1; cnt < maxsize / sizeof (long); cnt <<= 1) {
		addr = base + cnt;	/* pointer arith! */

	//	printf("\n retrieve addr=%08X \n",addr);
			val = *addr;
		*addr = save[--i];
		if (val != ~cnt) {
			size = cnt * sizeof (long);
			
		//	printf("\n The Addr[%08X],do back ring  \n",addr);
			
			/* Restore the original data before leaving the function.
			 */
			for (cnt <<= 1; cnt < maxsize / sizeof (long); cnt <<= 1) {
				addr  = base + cnt;
				*addr = save[--i];
			}
			return (size);
		}
	}

	return (maxsize);
}



long int initdram(int board_type)
{
	ulong size, max_size       = MAX_SDRAM_SIZE;
	ulong our_address;
#ifndef CONFIG_MIPS16
	asm volatile ("move %0, $25" : "=r" (our_address) :);
#endif
	/* Can't probe for RAM size unless we are running from Flash.
	 */
#if 0	 
	#if defined(CFG_RUN_CODE_IN_RAM)

	printf("\n In RAM run \n"); 
    return MIN_SDRAM_SIZE;
	#else

	printf("\n In FLASH run \n"); 
    return MIN_RT2880_SDRAM_SIZE;
	#endif
#endif 
    
#if defined (RT2880_FPGA_BOARD) || defined (RT2880_ASIC_BOARD)
	if (PHYSADDR(our_address) < PHYSADDR(PHYS_FLASH_1))
	{
	    
		//return MIN_SDRAM_SIZE;
		//fixed to 32MB
		printf("\n In RAM run \n");
		return MIN_SDRAM_SIZE;
	}
#endif
	 


	size = get_ram_size((ulong *)CFG_SDRAM_BASE, MAX_SDRAM_SIZE);
	if (size > max_size)
	{
		max_size = size;
	//	printf("\n Return MAX size!! \n");
		return max_size;
	}
//	printf("\n Return Real size =%d !! \n",size);
	return size;
	
}

int checkboard (void)
{
	puts ("Board: Ralink APSoC ");
	return 0;
}

#include <rt_mmap.h>

#define u32 u_long

int setGpioData(u32 gpio, u32 data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 95)
	{
		puts("Boot: setGpioData() Unsupport GPIO\n");
		return -1;
	}
	if (gpio <= 31)
	{
		/* RALINK_REG_PIODATA for GPIO 0~31 */
#if defined(TP_MODEL_C20V4) || defined(TP_MODEL_MR3420V5) || defined(TP_MODEL_MR3020V3)
	if(2 == gpio || 3 == gpio)
		reg = RALINK_SYSCTL_BASE + 0x3c;
	else
#endif
		reg = RALINK_PIO_BASE + 0x20;
		bit = (1 << gpio);
	}
	else if (gpio <= 63)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 32~63 */
		reg = RALINK_PIO_BASE + 0x24;
		bit = (1 << (gpio - 32));
	}
	else if (gpio <= 95)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 64~95 */
		reg = RALINK_PIO_BASE + 0x28;
		bit = (1 << (gpio - 64));
	}

	/* Set to reg base on bit and data */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (0 == data)
	{
		tmp &= ~bit;
	}
	else
	{
		tmp |= bit;
	}
	*(volatile u32 *)(reg) = tmp;
	return 0;
}


int initTpProduct(void)
{
	u32 gpiomode;
	u32 tmp;

#if defined(INCLUDE_COMBINED_IMAGE)
	u32 cpuType = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x08));
	printf("\ncpuType %08x.\n", cpuType);
	cpuType = (cpuType & 0x10000);
	printf("cpuType %08x.\n", cpuType);
	if(cpuType == 0x10000)
	{   /*MT7628NN*/
		POWER_LED_GPIO = 37;
		INTERNET_LED_GPIO = 39;
		WPS_LED_GPIO = 40;
		LAN_LED_GPIO = 41;
		SIGNAL_S1_LED_GPIO = 42;
		SIGNAL_S2_LED_GPIO = 43;
		SIGNAL_S3_LED_GPIO = 44;

		WPS_RST_BUTTON_GPIO = 38;
		WIFI_BUTTON_GPIO = 46;
	}
	else
	/* Only support MR400V4/MR6400V5/MR200V5 */
	{
		POWER_LED_GPIO = 39;
		INTERNET_LED_GPIO = 40;
		WPS_LED_GPIO = 4;
		LAN_LED_GPIO = 5;
		SIGNAL_S1_LED_GPIO = 41;
		SIGNAL_S2_LED_GPIO = 42;
		SIGNAL_S3_LED_GPIO = 43;

		WPS_RST_BUTTON_GPIO = 38;
		WIFI_BUTTON_GPIO = 46;
	}
#endif

#if defined(TP_MODEL_MR3020V3)
	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	printf("gpiomode1 %08x.\n", gpiomode);
	gpiomode &= ~((0x3 << 18) | (0x3 << 20));
	gpiomode |= (1 << 18) | (1 << 20);
	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);

	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT
	* POWER_LED_GPIO,USB_LED_GPIO,
	* INTERNET_LED_GREEN_GPIO,INTERNET_LED_ORANGE_GPIO
	*/
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	printf("######GPIO CTRL 0 for GPIO 0~32 OUTPUT tmp(0x%08x)#####\n",tmp);

	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT
	*/
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (POWER_LED_GPIO - 32)) | (1 << (INTERNET_LED_GREEN_GPIO - 32)) | (1 << (WLAN_LED_GPIO - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT
	*/
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (WPS_RST_BUTTON_GPIO - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/*set led*/
	printf("##########Led gpio info: power(%d),wps(%d),lan(%d),wan_green(%d),wlan(%d)#########\n",
	POWER_LED_GPIO,WPS_LED_GPIO,LAN_LED_GPIO,INTERNET_LED_GREEN_GPIO,WLAN_LED_GPIO);
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 0);
	setGpioData(LAN_LED_GPIO, 0);
	setGpioData(WLAN_LED_GPIO, 0);
	setGpioData(INTERNET_LED_GREEN_GPIO, 0);
	udelay (1000 * 100 * 10); /* 1s */
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 1);
	setGpioData(LAN_LED_GPIO, 1);
	setGpioData(INTERNET_LED_GREEN_GPIO, 1);
	setGpioData(WLAN_LED_GPIO, 1);

#elif defined(INCLUDE_COMBINED_IMAGE)
	if(cpuType == 0x10000)
	{
		/*MT7628NN*/
		/*MR6400V3*/
		/* GPIO1 Mode for GPIO 37 & 38 & 46  */
		gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
		//printf("gpiomode1 %08x.\n", gpiomode);
		gpiomode &= (~((0x3 << 18) | (0x3 << 24) | (0x3 << 14)));
		gpiomode |= ((1 << 18) | (1 << 24) | (1 << 14));
		printf("gpiomode1 %08x.\n", gpiomode);
		*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

		/* GPIO2 Mode for GPIO 39~44 */
		gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
		//printf("gpiomode2 %08x.\n", gpiomode);
		gpiomode &= ((0xf << 12) | (0xf << 28));
		gpiomode |= (0x555 | (0x555 << 16));
		printf("gpiomode2 %08x.\n", gpiomode);
		*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);

		/* GPIO CTRL 1 for GPIO 37 & 39~44 OUTPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
		tmp |= ((1 << (POWER_LED_GPIO - 32)) | (1 << (INTERNET_LED_GPIO - 32)) | (1 << (WPS_LED_GPIO - 32))  | (1 << (LAN_LED_GPIO - 32)) |
			(1 << (SIGNAL_S1_LED_GPIO - 32)) | (1 << (SIGNAL_S2_LED_GPIO - 32)) | (1 << (SIGNAL_S3_LED_GPIO - 32)));
		*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
		printf("######GPIO CTRL 1 for GPIO 32~64 OUTPUT tmp(0x%08x)#####\n",tmp);

		/* GPIO CTRL 1 for GPIO 38 & 46 INPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
		tmp &= (~(1 << (WPS_RST_BUTTON_GPIO - 32) | (1 << (WIFI_BUTTON_GPIO - 32))));
		*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
		printf("######GPIO CTRL 1 for GPIO 32~64 INPUT tmp(0x%08x)#####\n",tmp);

		/* set led */
		setGpioData(POWER_LED_GPIO, 0);
		setGpioData(INTERNET_LED_GPIO, 0);
		setGpioData(WPS_LED_GPIO, 0);
		setGpioData(LAN_LED_GPIO, 0);
		setGpioData(SIGNAL_S1_LED_GPIO, 0);
		setGpioData(SIGNAL_S2_LED_GPIO, 0);
		setGpioData(SIGNAL_S3_LED_GPIO, 0);
		udelay (1000 * 100 * 10); /* 1s */
		setGpioData(POWER_LED_GPIO, 0);
		setGpioData(INTERNET_LED_GPIO, 1);
		setGpioData(WPS_LED_GPIO, 1);
		setGpioData(LAN_LED_GPIO, 1);
		setGpioData(SIGNAL_S1_LED_GPIO, 1);
		setGpioData(SIGNAL_S2_LED_GPIO, 1);
		setGpioData(SIGNAL_S3_LED_GPIO, 1);

	}
	else
	{
		/* Only support MR400V4/MR6400V5/MR200V5 */
		/* GPIO1 Mode for GPIO 4 & 5 & 38 & 46  */
		gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
		//printf("gpiomode1 %08x.\n", gpiomode);
		gpiomode &= (~((0x3 << 20) | (0x3 << 24) | (0x3 << 14)));
		gpiomode |= ((1 << 20) | (1 << 24) | (1 << 14));
		printf("gpiomode1 %08x.\n", gpiomode);
		*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

		/* GPIO2 Mode for GPIO 39~43 */
		gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
		//printf("gpiomode2 %08x.\n", gpiomode);
		gpiomode &= ((0xf << 12) | (0xf << 28));
		gpiomode |= (0x555 | (0x555 << 16));
		printf("gpiomode2 %08x.\n", gpiomode);
		*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);

		/* GPIO CTRL 0 for GPIO 4~5 OUTPUT */
		tmp = le32_to_cpu(*(volatile u32 *)RALINK_PIO_BASE);
		tmp |= ((1 << WPS_LED_GPIO) | (1 << LAN_LED_GPIO));
		*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
		printf("######GPIO CTRL 0 for GPIO 0~31 OUTPUT tmp(0x%08x)#####\n",tmp);

		/* GPIO CTRL 1 for GPIO 39~43 OUTPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
		tmp |= ((1 << (POWER_LED_GPIO - 32)) | (1 << (INTERNET_LED_GPIO - 32)) | (1 << (SIGNAL_S1_LED_GPIO - 32)) |
			(1 << (SIGNAL_S2_LED_GPIO - 32)) | (1 << (SIGNAL_S3_LED_GPIO - 32)));
		*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
		printf("######GPIO CTRL 1 for GPIO 32~64 OUTPUT tmp(0x%08x)#####\n",tmp);

		/* GPIO CTRL 1 for GPIO 38 & 46 INPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
		tmp &= (~(1 << (WPS_RST_BUTTON_GPIO - 32) | (1 << (WIFI_BUTTON_GPIO - 32))));
		*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
		printf("######GPIO CTRL 1 for GPIO 32~64 INPUT tmp(0x%08x)#####\n",tmp);

		/* set led */
		setGpioData(POWER_LED_GPIO, 0);
		setGpioData(INTERNET_LED_GPIO, 0);
		setGpioData(WPS_LED_GPIO, 0);
		setGpioData(LAN_LED_GPIO, 0);
		setGpioData(SIGNAL_S1_LED_GPIO, 0);
		setGpioData(SIGNAL_S2_LED_GPIO, 0);
		setGpioData(SIGNAL_S3_LED_GPIO, 0);
		udelay (1000 * 100 * 10); /* 1s */
		setGpioData(POWER_LED_GPIO, 0);
		setGpioData(INTERNET_LED_GPIO, 1);
		setGpioData(WPS_LED_GPIO, 1);
		setGpioData(LAN_LED_GPIO, 1);
		setGpioData(SIGNAL_S1_LED_GPIO, 1);
		setGpioData(SIGNAL_S2_LED_GPIO, 1);
		setGpioData(SIGNAL_S3_LED_GPIO, 1);

	}
#elif defined(TP_MODEL_MR100V1) || defined(TP_MODEL_MR100V2) || defined(TP_MODEL_MB110V1)

	/*MT7628NN*/
	/* GPIO1 Mode for GPIO 38 46  */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	//printf("gpiomode1 %08x.\n", gpiomode);
//	gpiomode &= (~((0x3 << 24) | (0x3 << 14)) | (0x3 << 16));
//	gpiomode |= ((1 << 24) | (1 << 14) | (1 << 16));
	gpiomode &= (~((0x3 << 24) | (0x3 << 14) |(0x3 << 16) | (0x3 << 6) | (0x3 << 20)));
	gpiomode |= ((1 << 24) | (1 << 14) |  (1 << 16)| (1 << 6) | (1 << 20));

	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	/* GPIO2 Mode for GPIO 39~44 */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	//printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);
#if defined(TP_MODEL_MR100V2) || defined(TP_MODEL_MB110V1)
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= ( (1 << USB_RESET_GPIO) );
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
#endif
	/* GPIO CTRL 1 for GPIO 37 & 39~44 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (POWER_LED_GPIO - 32)) | (1 << (INTERNET_LED_GPIO - 32)) | (1 << (WPS_LED_GPIO - 32))  | (1 << (LAN_LED_GPIO - 32)) |
		(1 << (SIGNAL_S1_LED_GPIO - 32)) | (1 << (SIGNAL_S2_LED_GPIO - 32)) | (1 << (SIGNAL_S3_LED_GPIO - 32)) | (1 << (USB_POWER_GPIO - 32))  | (1 << (USB_DC_GPIO - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	printf("######GPIO CTRL 1 for GPIO 32~64 OUTPUT tmp(0x%08x)#####\n",tmp);

	/* GPIO CTRL 1 for GPIO 38 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (WPS_RST_BUTTON_GPIO - 32) ));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	printf("######GPIO CTRL 1 for GPIO 32~64 INPUT tmp(0x%08x)#####\n",tmp);

	/* set led */
	setGpioData(POWER_LED_GPIO, 1);
	setGpioData(INTERNET_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 0);
	setGpioData(LAN_LED_GPIO, 0);
	setGpioData(SIGNAL_S1_LED_GPIO, 0);
	setGpioData(SIGNAL_S2_LED_GPIO, 0);
	setGpioData(SIGNAL_S3_LED_GPIO, 0);
	//setGpioData(USB_POWER_GPIO, 0);
#if defined(TP_MODEL_MR100V2) || defined(TP_MODEL_MB110V1)
	setGpioData(USB_RESET_GPIO, 1);
#endif
	udelay (1000 * 100 * 5); /* 0.5s */
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 1);
	udelay (1000 * 100 * 5); /* 0.5s */

#if defined(TP_MODEL_MR100V2) || defined(TP_MODEL_MB110V1)
	setGpioData(USB_RESET_GPIO, 0);
#endif

	setGpioData(POWER_LED_GPIO, 1);
	setGpioData(INTERNET_LED_GPIO, 1);
	setGpioData(WPS_LED_GPIO, 0);
	setGpioData(LAN_LED_GPIO, 1);
	setGpioData(SIGNAL_S1_LED_GPIO, 1);
	setGpioData(SIGNAL_S2_LED_GPIO, 1);
	setGpioData(SIGNAL_S3_LED_GPIO, 1);
    //set USB power on
	setGpioData(USB_POWER_GPIO, 1);
#elif defined(TP_MODEL_MR3420V5)
	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	printf("gpiomode1 %08x.\n", gpiomode);
	gpiomode &= ~((0x3 << 18) | (0x3 << 20));
	gpiomode |= (1 << 18) | (1 << 20);
	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);

	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT
	* POWER_LED_GPIO,USB_LED_GPIO,
	* INTERNET_LED_GREEN_GPIO,INTERNET_LED_ORANGE_GPIO
	*/
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= ((1 << (POWER_LED_GPIO)) | (1 << (USB_LED_GPIO)) | (1 << (INTERNET_LED_GREEN_GPIO)) | (1 << (INTERNET_LED_ORANGE_GPIO)));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	printf("######GPIO CTRL 0 for GPIO 0~32 OUTPUT tmp(0x%08x)#####\n",tmp);

	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT
	*/
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (WPS_LED_GPIO - 32)) | (1 << (LAN_LED_GPIO - 32)) | (1 << (WLAN_LED_GPIO - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT
	*/
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (WPS_RST_BUTTON_GPIO - 32) | (1 << (WIFI_BUTTON_GPIO - 32))));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/*set led*/
	printf("##########Led gpio info: power(%d),wps(%d),lan(%d),usb(%d),wan_green(%d),wan_orange(%d),wlan(%d)#########\n",
	POWER_LED_GPIO,WPS_LED_GPIO,LAN_LED_GPIO,USB_LED_GPIO,INTERNET_LED_GREEN_GPIO,INTERNET_LED_ORANGE_GPIO,WLAN_LED_GPIO);
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 0);
	setGpioData(LAN_LED_GPIO, 0);
	setGpioData(USB_LED_GPIO, 0);
	setGpioData(WLAN_LED_GPIO, 0);
	setGpioData(INTERNET_LED_ORANGE_GPIO, 1);
	setGpioData(INTERNET_LED_GREEN_GPIO, 0);
	udelay (1000 * 100 * 10); /* 1s */
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 1);
	setGpioData(LAN_LED_GPIO, 1);
	setGpioData(USB_LED_GPIO, 1);
	setGpioData(INTERNET_LED_GREEN_GPIO, 1);
	setGpioData(INTERNET_LED_ORANGE_GPIO, 1);
	setGpioData(WLAN_LED_GPIO, 1);

#elif  defined(TP_MODEL_WR840NV4)
	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18);
	printf("gpiomode1 %08x.\n", gpiomode);

	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

	/* GPIO
	 * GPIO36: POWER
	 * GPIO37:WPS
	 * GPIO38:WPS/RESET Button
	 * GPIO41:LAN
	 * GPIO43:INTERNET
	 * GPIO44:WLAN(2.4G)
	 */
	/* Set Direction to output */
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */



	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (37 - 32)) | (1 << (41 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 0);
	setGpioData(LAN_LED_GPIO, 0);
	setGpioData(INTERNET_LED_GPIO, 0);
	setGpioData(WLAN_LED_GPIO, 0);
	udelay (1000 * 100 * 10); /* 1s */
	setGpioData(POWER_LED_GPIO, 0);
	setGpioData(WPS_LED_GPIO, 1);
	setGpioData(LAN_LED_GPIO, 1);
	setGpioData(INTERNET_LED_GPIO, 1);
	setGpioData(WLAN_LED_GPIO, 1);
	
#elif defined(TP_MODEL_WR841NV13) || defined(TP_MODEL_WR845NV3)

	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

	/* GPIO
	 * GPIO36: POWER
	 * GPIO46:WPS	
	 * GPIO38:RESET Button(WPS switch for 841v13)
 	 * GPIO39/40/41/42:LAN
 	 * GPIO43:INTERNET_GREEN
 	 * GPIO11:INTERNET_ORANGE
	 * GPIO44:WLAN(2.4G)
	 * GPIO37:WLAN ON/OFF (WPS switch for 845v3)
	 */
	/* Set Direction to output */
	
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | 
			(1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(11, 0);
	setGpioData(36, 0);
	setGpioData(39, 0);
	setGpioData(40, 0);
	setGpioData(41, 0);
	setGpioData(42, 0);
	setGpioData(43, 1);
	setGpioData(44, 0);
	setGpioData(46, 0);
	udelay (500 * 100 * 10); /* 0.5s */
	setGpioData(11, 1);		/* wan color switch */
	setGpioData(43, 0);
	udelay (500 * 100 * 10); /* 0.5s */
	setGpioData(36, 0);
	setGpioData(39, 1);
	setGpioData(40, 1);
	setGpioData(41, 1);
	setGpioData(42, 1);
	setGpioData(43, 1);
	setGpioData(44, 1);
	setGpioData(46, 1);	

#elif defined(TP_MODEL_C20V4)

	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

	/* GPIO
	 ****************LED***************
	 * GPIO19:	POWER
	 * GPIO48:	WPS LED
	 * GPIO42:	LAN1
	 * GPIO41:	LAN2
	 * GPIO40:	LAN3
	 * GPIO39:	LAN4
	 * GPIO43:	INTERNET_GREEN
	 * GPIO29:	INTERNET_ORANGE
	 * GPIO44:	WLAN(2.4G)
	 * GPIO18:	WLAN(5G)
	 *
	 ****************BTN***************
	 * GPIO36:	WLAN ON/OFF
	 * GPIO37:	RESET/WPS
	 */
	 
	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | (1 << (42 - 32)) | 
			(1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(2,  0);
	setGpioData(3,  0);
	setGpioData(11, 1);
	setGpioData(39, 0);
	setGpioData(40, 0);
	setGpioData(41, 0);
	setGpioData(42, 0);
	setGpioData(43, 0);
	setGpioData(44, 0);
	setGpioData(46, 0);
	udelay (500 * 100 * 10);
	setGpioData(11, 0);
	setGpioData(43, 1);
	udelay (500 * 100 * 10);
	setGpioData(2,  1);
	setGpioData(3,  0);
	setGpioData(11, 1);
	setGpioData(39, 1);
	setGpioData(40, 1);
	setGpioData(41, 1);
	setGpioData(42, 1);
	setGpioData(43, 1);
	setGpioData(44, 1);
	setGpioData(46, 1);

#elif defined(TP_MODEL_C50V4)

/* GPIO1 Mode */
gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
/*                       GPIO11            GPIO5 */
gpiomode &= ~((0x3 << 0) | (0x3 << 20));
/*                    GPIO38         GPIO5 */
gpiomode |= (1 << 14) | (1 << 20);

printf("gpiomode1 %08x.\n", gpiomode);
*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

/* GPIO2 Mode */
gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
/*                 GPIO39 ~ GPIO44 */
gpiomode &= ~0x0fff;
/*                 GPIO39 ~ GPIO44 */
gpiomode |= 0x0555;

printf("gpiomode2 %08x.\n", gpiomode);
*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

/* GPIO
 * GPIO11: POWER LED
 * GPIO39:WPS LED
 * GPIO40:WLAN 5G LED
 * GPIO41:LAN LED
 * GPIO42:INTERNET ORANGE LED
 * GPIO43:INTERNET GREEN LED
 * GPIO44:WLAN 2.4G LED
 * GPIO38:WPS /RESET BUTTON
 * GPIO5:WLAN BUTTON
 */
/* Set Direction to output */
/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
tmp |= (1 << 11);
tmp &= ~(1 << 5);
*(volatile u32 *)(RALINK_PIO_BASE) = tmp;

/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
tmp |= 0x3f << (39 - 32);
*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

/* GPIO CTRL 1 for GPIO 32~64 INPUT */
tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
tmp &= ~(1 << (38 - 32));
*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

/* Led */
setGpioData(11, 0);
setGpioData(39, 0);
setGpioData(40, 0);
setGpioData(41, 0);
setGpioData(42, 0);
setGpioData(43, 0);
setGpioData(44, 0);
udelay (1000 * 100 * 10); /* 1s */
setGpioData(11, 0);
setGpioData(39, 1);
setGpioData(40, 1);
setGpioData(41, 1);
setGpioData(42, 1);
setGpioData(43, 1);
setGpioData(44, 1);
#endif

	return 0;
}

/* port from kernel by wanghao  */
#define RALINK_PRGIO_ADDR		RALINK_PIO_BASE // Programmable I/O
#define RALINK_REG_PIO3100DATA		(RALINK_PRGIO_ADDR + 0x20)
#define RALINK_REG_PIO6332DATA		(RALINK_PRGIO_ADDR + 0x24)
#define RALINK_REG_PIO9564DATA		(RALINK_PRGIO_ADDR + 0x28)

#define RALINK_REG_PIODATA		(RALINK_PRGIO_ADDR + 0x20)
int getGpioData(u32 gpio, u32 *data)
{
	u32 tmp = 0;

	/* INPUT GPIO
	 * GPIO38:Switch IRQ
	 */
	
	/* Get to reg base on bit */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DATA));
	if (( 1 << (38 - 32)) & tmp)
	{
		*data = 1;
	}
	else
	{
		*data = 0;
	}
	return 0;
}
/* port end  */

#if 0
value = le32_to_cpu(*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x0034));
u32 gpiomode;
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
	
	/* C2, yuanshang, 2013-11-14
	 * GPIO1,GPIO2: I2C GPIO Mode(bit0)
	 * GPIO11,GPIO13:	UART Full(bit4:2)
	 * GPIO39:	SPI GPIO(bit11) & SPI Ref(bit12) [no need to set bit 1] 
	 * GPIO40,GPIO41,GPIO42:	EPHY LED(bit15)
	 * GPIO72:	WLED GPIO(bit13)
	 */
	/*gpiomode |= RALINK_GPIOMODE_DFT;*/
	gpiomode |= (RALINK_GPIOMODE_I2C) | (RALINK_GPIOMODE_UARTF) | (RALINK_GPIOMODE_EPHY) | (RALINK_GPIOMODE_WLED);
	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);


#endif

