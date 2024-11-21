/************************************************************************
 *
 *	Copyright (C) 2007 Trendchip Technologies, Corp.
 *	All Rights Reserved.
 *
 * Trendchip Confidential; Need to Know only.
 * Protected as an unpublished work.
 *
 * The computer program listings, specifications and documentation
 * herein are the property of Trendchip Technologies, Co. and shall
 * not be reproduced, copied, disclosed, or used in whole or in part
 * for any reason without the prior express written permission of
 * Trendchip Technologeis, Co.
 *
 *************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <asm/addrspace.h>
#include <asm/io.h>
//#include <asm/tc3162/cmdparse.h>
#include "cmdparse.h"
//#include <asm/tc3162/tc3162.h>

extern int subcmd(const cmds_t tab[], int argc, char *argv[], void *p);
extern int cmd_register(cmds_t *cmds_p);

static int doSys(int argc, char *argv[], void *p);
static int doSysMemrl(int argc, char *argv[], void *p);
static int doSysMemwl(int argc, char *argv[], void *p);
static int doSysMemrw(int argc, char *argv[], void *p);
static int doSysMemww(int argc, char *argv[], void *p);
static int doSysMemory(int argc, char *argv[], void *p);
static int doSysMemcpy(int argc, char *argv[], void *p);
static int doSysFillMem(int argc, char *argv[], void *p);
static int doSysMac(int argc, char *argv[], void *p);
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BOOTLOADER_MODIFY_PVNAME)
static int doSysAtsp(int argc, char *argv[], void *p);
static int doSysAtsv(int argc, char *argv[], void *p);
static int doSysAtspv(int argc, char *argv[], void *p);
#endif/*TCSUPPORT_COMPILE*/

#ifdef WAN2LAN
static int doWan2lan(int argc, char *argv[], void *p);
int my_atoi(const char *str);
int masko_on_off = 0;
int masko = 0xF;
EXPORT_SYMBOL(masko_on_off);
EXPORT_SYMBOL(masko);

#if defined(TCSUPPORT_CMDPROMPT)
int print_usage=0;
int print_state=0;
int print_portmask=0;
int print_errormsg=0;
#define WAN2LAN_INFO_USAGE "Usage: wan2lan [on||off] <number> \r\n"
#define WAN2LAN_INFO_ON "Current wan2lan feature status: on \r\n"
#define WAN2LAN_INFO_OFF "Current wan2lan feature status: off \r\n"
#define WAN2LAN_INFO_PORTMASK "Current portmask number is[%d] \r\n"
#define WAN2LAN_INFO_ERRORMSG "input portmask number is out of range(available value is 0~15) \r\n"
#endif

#endif
#ifdef TCSUPPORT_AUTOBENCH_AFE
extern void tc3162wdog_kick(void);
static int waitDmtPowerOn(int argc, char *argv[], void *p);
static int checkReboot(int argc, char *argv[], void *p);
#endif
#if defined(TCSUPPORT_LED_BTN_CHECK) || defined(TCSUPPORT_TEST_LED_ALL) 
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BTN_CHECK)
extern int gButtonCheck; 
#endif/*TCSUPPORT_COMPILE*/
#if defined(TCSUPPORT_LED_CHECK) || defined(TCSUPPORT_TEST_LED_ALL) 
extern int doLedOn(void);
extern int doLedOff(void);
static int doLedCheck(int argc, char *argv[], void *p);
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BTN_CHECK)
static int doButtonCheck(int argc, char *argv[], void *p);
#endif/*TCSUPPORT_COMPILE*/
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C1_NEW_GUI)
int doRomReset(int argc, char *argv[], void *p);
#endif/*TCSUPPORT_COMPILE*/
#if defined(TCSUPPORT_USBHOST)
int doUSBEYE(int argc, char *argv[], void *p);
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C2_TRUE)
int doRomCheck(int argc, char *argv[], void *p);
#endif/*TCSUPPORT_COMPILE*/
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C1_OBM)
int doSysAtsh(int argc, char *argv[], void *p);
#endif/*TCSUPPORT_COMPILE*/

#ifdef L2_AUTOPVC
static cmds_t sysCmds[] = {
#else
static const cmds_t sysCmds[] = {
#endif
	{"memrl",		doSysMemrl,		0x02,  	1,  NULL},
	{"memwl",		doSysMemwl,		0x02,  	2,  NULL},
	{"memrw",		doSysMemrw,		0x02,  	1,  NULL},
	{"memww",		doSysMemww,		0x02,  	2,  NULL},
	{"memory",		doSysMemory,	0x02,  	2,  NULL},
	{"memcpy",		doSysMemcpy,	0x02,  	3,  NULL},
	{"fillmem",		doSysFillMem,	0x02,  	3,  NULL},
	{"mac",	    	doSysMac,	    0x02,  	1,  NULL},
#ifdef L2_AUTOPVC
	{"autopvc",		NULL,			0x12,	1,	NULL},
#endif
#ifdef WAN2LAN
	{"wan2lan",		doWan2lan,		0x02,	0,	NULL},
#endif
#ifdef TCSUPPORT_AUTOBENCH_AFE
    {"waitdmt",     waitDmtPowerOn,      0x02,   0,  NULL},
    {"checkreboot",     checkReboot,      0x02,   0,  NULL},
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C1_NEW_GUI)
	{"romreset",		doRomReset,		0x02,	1,	NULL},
#endif/*TCSUPPORT_COMPILE*/
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BOOTLOADER_MODIFY_PVNAME)
	{"atsp",	doSysAtsp,			0x02,	0,	NULL},
	{"atsv",	doSysAtsv,			0x02,	0,	NULL},
	{"atpvsave",	doSysAtspv,		0x02,	0,	NULL},	
#endif/*TCSUPPORT_COMPILE*/
#if defined(TCSUPPORT_LED_BTN_CHECK) || defined(TCSUPPORT_TEST_LED_ALL) 
#if defined(TCSUPPORT_LED_CHECK) || defined(TCSUPPORT_TEST_LED_ALL) 
	{"led",		doLedCheck,		0x02,	0,	NULL},
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BTN_CHECK)
	{"button",	doButtonCheck,	0x02,	0,	NULL},	
#endif/*TCSUPPORT_COMPILE*/
#endif
#if defined(TCSUPPORT_USBHOST)
	{"usbeye",  doUSBEYE,  0x02,   0,  NULL},
#endif	
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C2_TRUE)
	{"romcheck",	    	doRomCheck,	0x02,	0,	NULL},
#endif/*TCSUPPORT_COMPILE*/
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C1_OBM)
	{"atsh",		doSysAtsh,			0x02,	0,	NULL},
#endif/*TCSUPPORT_COMPILE*/
	{NULL,			NULL,			0x10,	0,	NULL},
};

//#define BOOTLOADER_SIZE 64*1024
extern void setUserSpaceFlag(int num);

int doSys(int argc, char *argv[], void *p)
{
#if defined(TCSUPPORT_CMDPROMPT)
	if(argc==1){
		setUserSpaceFlag(12);
		return -1;
	}else{
		return subcmd(sysCmds, argc, argv, p);
	}
#else
	return subcmd(sysCmds, argc, argv, p);
#endif
	
}

int doSysMemrl(int argc, char *argv[], void *p)
{
	unsigned long *ptr;

	ptr = (unsigned long *)simple_strtoul(argv[1], NULL, 16);
	printk("\r\n<Address>\t<Value>\r\n");
	printk("0x%08lx\t0x%08lx\r\n", (unsigned long)ptr, (unsigned long)*ptr);

	return 0;
}

int doSysMemwl(int argc, char *argv[], void *p)
{
	unsigned long *ptr;
	unsigned long value;

	ptr = (unsigned long *)simple_strtoul(argv[1], NULL, 16);
	value = (unsigned long)simple_strtoul(argv[2], NULL, 16);
	*ptr = value;

	return 0;
}

int doSysMemrw(int argc, char *argv[], void *p)
{
	unsigned short *ptr;

	ptr = (unsigned short *)simple_strtoul(argv[1], NULL, 16);
	printk("\r\n<Address>\t<Value>\r\n");
	printk("0x%08lx\t0x%04x\r\n", (unsigned long) ptr, (unsigned int) (*ptr));

	return 0;
}

int doSysMemww(int argc, char *argv[], void *p)
{
	unsigned short *ptr;
	unsigned short value;

	ptr = (unsigned short *)simple_strtoul(argv[1], NULL, 16);
	value = (unsigned short)simple_strtoul(argv[2], NULL, 16);
	*ptr = value;

	return 0;
}

static int dump(unsigned long addr, unsigned long len)
{
	register int n, m, c, r;
	unsigned char temp[16];

	for( n = len; n > 0; ){
			printk("%.8lx ", addr);
			r = n < 16? n: 16;
			memcpy((void *) temp, (void *) addr, r);
			addr += r;
			for( m = 0; m < r; ++m ){
					printk("%c", (m & 3) == 0 && m > 0? '.': ' ');
					printk("%.2x", temp[m]);
			}
			for(; m < 16; ++m )
					printk("   ");
			printk("  |");
			for( m = 0; m < r; ++m ){
				c = temp[m];
				printk("%c", ' ' <= c && c <= '~'? c: '.');
			}
			n -= r;
			for(; m < 16; ++m )
					printk(" ");
			printk("|\n");
	}

	return 0;
}

int doSysMemory(int argc, char *argv[], void *p)
{
	unsigned long addr, len;

	addr = simple_strtoul(argv[1], NULL, 16);
	len = simple_strtoul(argv[2], NULL, 16);
	dump(addr, len);

	return 0;
}

int doSysMemcpy(int argc, char *argv[], void *p)
{
	unsigned long src, dst, len;

	src = simple_strtoul(argv[1], NULL, 16);
	dst = simple_strtoul(argv[2], NULL, 16);
	len = simple_strtoul(argv[3], NULL, 16);
	memcpy((void *)dst, (void *)src, len);

	return 0;
}

int doSysFillMem(int argc, char *argv[], void *p)
{
	unsigned long addr, len, pattern;

	addr = simple_strtoul(argv[1], NULL, 16);
	len = simple_strtoul(argv[2], NULL, 16);
	pattern = simple_strtoul(argv[3], NULL, 16);
	memset((void *)addr, pattern, len);

	return 0;
}


int doSysMac(int argc, char *argv[], void *p)
{
    setUserSpaceFlag(1);
	return 0;
}
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BOOTLOADER_MODIFY_PVNAME)
static int doSysAtsp(int argc, char *argv[], void *p){
    setUserSpaceFlag(6);
	return 0;	
}

static int doSysAtsv(int argc, char *argv[], void *p){
    setUserSpaceFlag(7);
	return 0;
	
}
  
static int doSysAtspv(int argc, char *argv[], void *p){
    setUserSpaceFlag(8);
	return 0;
}
#endif/*TCSUPPORT_COMPILE*/

#ifdef L2_AUTOPVC
/*_____________________________________________________________________________
**      function name: autopvc_cmd_register_to_sys
**      descriptions:
**         Register autopvc ci-cmd into sys ci-cmd tables.
**
**      parameters:
**             cmds_p: Specify you want to register ci-cmd
**      global:
**             Cmds
**      return:
**              Success:        0
**              Otherwise:     -1
**      call:
**   	None
**      revision:
**      1. krammer 2008/8/13
**____________________________________________________________________________
*/
int
autopvc_cmd_register_to_sys(cmds_t *cmds_p)
{
	printk("register autopvc cmd to sys\n");
	int i=0;
	for(i=0; sysCmds[i].name; i++){
		if(memcmp(sysCmds[i].name,"autopvc",7) == 0){
			memcpy(&sysCmds[i], cmds_p, sizeof(cmds_t));
			return 0;
		}
	}

	printk("Can't find autopvc-cmd\n");
	return -1;
}
EXPORT_SYMBOL(autopvc_cmd_register_to_sys);
#endif
#ifdef WAN2LAN
int my_atoi(const char *str)
{
	int result = 0;
	int signal = 1;
	if( (*str >= '0' && *str <= '9') || *str == '-' || *str == '+' ){
		if( *str == '-' || *str == '+' ){
			if( *str == '-' )
				signal = -1;
			str++;
		}
	}
	else 
		return 0;
	while( *str >= '0' && *str <= '9' )
		result = result*10 + (*str++ - '0');
	return signal*result;
}

//merge from linos,but it dosen't support SW3052
static int doWan2lan(int argc, char *argv[], void *p)
{
#if defined(TCSUPPORT_CMDPROMPT)
    	setUserSpaceFlag(13);
#endif

	if(argc < 2 || argc > 3){
#if !defined(TCSUPPORT_CMDPROMPT)
		printk("Usage: wan2lan [on||off] <number> \r\n");
#else
		print_usage=1;
#endif
		if(masko_on_off == 1){
#if !defined(TCSUPPORT_CMDPROMPT)
			printk("Current wan2lan feature status: on \r\n");
			printk("Current portmask number is[%d] \r\n",masko);
#else
			print_state=1;
			print_portmask=1;
#endif
		}
		else if(masko_on_off == 0){
#if !defined(TCSUPPORT_CMDPROMPT)
			printk("Current wan2lan feature status: off \r\n");
#else
			print_state=1;
#endif
		}
		return -1;
	}
	if(argc == 3){
			if((my_atoi(argv[2]) > 15) || (my_atoi(argv[2]) < 0)){
#if !defined(TCSUPPORT_CMDPROMPT)
			printk("input portmask number is out of range(available value is 0~15) \r\n");
#else
			print_errormsg=1;
#endif
			return -1;
		}
	}
	if (strcmp(argv[1], "on") == 0){
		masko_on_off = 1;
		masko = 0xF;
#if !defined(TCSUPPORT_CMDPROMPT)
		printk("Current wan2lan feature status: on \r\n");
#else
		print_state=1;
#endif
	}	
	else if(strcmp(argv[1], "off") == 0){
		masko_on_off = 0;
#if !defined(TCSUPPORT_CMDPROMPT)
		printk("Current wan2lan feature status: off \r\n");
#else
		print_state=1;
#endif
		return 0;
	 }
	else{
#if !defined(TCSUPPORT_CMDPROMPT)
		printk("Usage: wan2lan [on||off] <number> \r\n");
#else
		print_usage=1;
#endif
		return -1;
	}
	if(argc == 3){
		masko = simple_strtoul(argv[2],NULL, 10);
#if !defined(TCSUPPORT_CMDPROMPT)
		printk("Current portmask number is[%d] \r\n",masko);
#else
		print_portmask=1;
#endif
		return 0;
	}
	return -1;
}
#if defined(TCSUPPORT_CMDPROMPT)
int
getWan2lanInfo(char *wan2lan_print)
{
	uint16	index=0;

	if(print_usage)
		index += sprintf( wan2lan_print+index, WAN2LAN_INFO_USAGE);
	if(print_errormsg)
		index += sprintf( wan2lan_print+index, WAN2LAN_INFO_ERRORMSG);
	if(print_state){
		if(masko_on_off == 1)
			index += sprintf( wan2lan_print+index, WAN2LAN_INFO_ON);
		else if(masko_on_off == 0)
			index += sprintf( wan2lan_print+index, WAN2LAN_INFO_OFF);
	}
	if(print_portmask)
			index += sprintf( wan2lan_print+index, WAN2LAN_INFO_PORTMASK,masko);

	print_usage=0;
	print_errormsg=0;
	print_state=0;
	print_portmask=0;
	
	return index;
}
int sys_proc_wan2lan(char *buf, char **start, off_t off, int count,
                 int *eof, void *data)
{
	int len = getWan2lanInfo(buf);
	if (len <= off+count)
		*eof = 1;
	*start = buf + off;
	len -= off;
	if (len>count)
		len = count;
	if (len<0)
		len = 0;
	return len;
}

#endif
#endif

#if defined(TCSUPPORT_CMDPROMPT)
int
getSysSubcmdsInfo(char *sys_subcmds)
{
	uint16	index=0;
	register const cmds_t *cmdp=NULL;
	char buf[66]={0};
	int i=0;

	index += sprintf( sys_subcmds+index, "valid subcommands:\n");

	memset(buf,' ',sizeof(buf));
	buf[64] = '\n';
	buf[65] = '\0';

	for(i=0,cmdp = sysCmds;cmdp->name != NULL;cmdp++){
			strncpy(&buf[i*16],cmdp->name,strlen(cmdp->name));
			if(i == 3){
				index += sprintf( sys_subcmds+index, buf);
				memset(buf,' ',sizeof(buf));
				buf[64] = '\n';
				buf[65] = '\0';
			}
			i = (i+1)%4;
	}
	if(i != 0)
		index += sprintf( sys_subcmds+index, buf);

	return index;
}
int sys_proc_subcmds(char *buf, char **start, off_t off, int count,
                 int *eof, void *data)
{
	int len = getSysSubcmdsInfo(buf);
	if (len <= off+count)
		*eof = 1;
	*start = buf + off;
	len -= off;
	if (len>count)
		len = count;
	if (len<0)
		len = 0;
	return len;
}
#endif


#ifdef TCSUPPORT_AUTOBENCH_AFE
static int waitDmtPowerOn(int argc, char *argv[], void *p)
{
	unsigned long int value;
	/* wait until dmt3095 is power on */       
        while(1){
		if(isRT63365){
			//printk("GPIO %x\n",(VPint(CR_GPIO_DATA)));
			value = (VPint(CR_GPIO_DATA) & (1<<0));
		}else{
			value = (VPint(CR_GPIO_DATA) & 0x2000000);
		}	
		if(value){
			return 0;
		}else{
			msleep(1000);
		}
	
		tc3162wdog_kick();
        }
}

static int checkReboot(int argc, char *argv[], void *p)
{
    if(VPint(0x8001fffc)==0x1234){
        VPint(0x8001fffc)=0;
        return 0; /* return back to get a console for setting */
    }
    else{
        printk("wait for system to reboot\n");
        while(1);
    }
}
#endif

#if defined(TCSUPPORT_LED_BTN_CHECK) || defined(TCSUPPORT_TEST_LED_ALL) 
#if defined(TCSUPPORT_LED_CHECK) || defined(TCSUPPORT_TEST_LED_ALL) 
static int doLedCheck(int argc, char *argv[], void *p)
{
	setUserSpaceFlag(9);
	if((argc == 2) && ((strcmp(argv[1], "on") == 0) || (strcmp(argv[1], "off") == 0))){
		if(strcmp(argv[1], "off") == 0){
			doLedOff();
			printk("All led is turned off! \r\n");	
		 }	
		else{
			doLedOn();
			printk("All led is turned on! \r\n");
		}
	}
	else{
		printk("Usage: sys led [on||off] \r\n");
		return -1;
	}

	return 0;
}	
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_BTN_CHECK)
static int doButtonCheck(int argc, char *argv[], void *p)
{
	setUserSpaceFlag(10);
	if(argc != 2 || (strcmp(argv[1], "enable") && strcmp(argv[1], "disable"))){
		printk("Usage: sys button [enable||disable]\r\n");
		return -1;
	}
	if (strcmp(argv[1], "disable") == 0){
		gButtonCheck = 1;
		printk("All buttons are disabled! \r\n");
	}	
	else{
		gButtonCheck = 0;
		printk("All buttons are enabled! \r\n");	
	 }

	return 0;
}	
#endif/*TCSUPPORT_COMPILE*/
#endif

#if defined(TCSUPPORT_USBHOST)
int doUSBEYE(int argc, char *argv[], void *p)
{
	printk("USB EYE TEST\n");
	if(isRT63365){
		VPint(0xbfbb0054) = 0x00041000;
		VPint(0xbfbb0058) = 0x00041000;
	}
	else{
		VPint(0xbfb80cf8) = 0x80001b40;
		VPint(0xbfb80cfc) = 0x800000e0;
		VPint(0xbfba1060) = 0x1000000;
		VPint(0xbfba1064) = 0x4000500;
		VPint(0xbfba1068) = 0x4000500;
		VPint(0xbfba1064) = 0x4000400;
		VPint(0xbfba1068) = 0x4000400;
	}
	return 0;
}
#endif
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C1_NEW_GUI)
int doRomReset(int argc, char *argv[], void *p)
{
	setUserSpaceFlag(5);
	return 0;
}
#endif/*TCSUPPORT_COMPILE*/

#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C2_TRUE)
int doRomCheck(int argc, char *argv[], void *p)
{
	setUserSpaceFlag(11);
	return 0;
}
#endif/*TCSUPPORT_COMPILE*/
#if/*TCSUPPORT_COMPILE*/ defined(TCSUPPORT_C1_OBM)
int doSysAtsh(int argc, char *argv[], void *p)
{
	setUserSpaceFlag(14);
	return 0;
}
#endif/*TCSUPPORT_COMPILE*/
int tcsyscmd_init(void)
{
	cmds_t sys_cmd;

#if defined(TCSUPPORT_CMDPROMPT)
	/* sys subcommands  info */
	create_proc_read_entry("tc3162/sys_subcmds", 0, NULL, sys_proc_subcmds, NULL);
#ifdef WAN2LAN
	/* sys wan2lan related info */
	create_proc_read_entry("tc3162/sys_wan2lan", 0, NULL, sys_proc_wan2lan, NULL);
#endif
#endif

	/*Init sys root ci-cmd*/
	sys_cmd.name= "sys";
	sys_cmd.func=doSys;
	sys_cmd.flags=0x12;
	sys_cmd.argcmin=0;
	sys_cmd.argc_errmsg=NULL;

	/*Register sys ci-cmd*/
	cmd_register(&sys_cmd);

	return 0;
}

//static void __exit tcsyscmd_exit(void)
void tcsyscmd_exit(void)
{
#if defined(TCSUPPORT_CMDPROMPT)
	remove_proc_entry("tc3162/sys_subcmds", NULL);
#ifdef WAN2LAN
	remove_proc_entry("tc3162/sys_wan2lan", NULL);
#endif
#endif

}
//module_init (tcsyscmd_init);
//module_exit (tcsyscmd_exit);

