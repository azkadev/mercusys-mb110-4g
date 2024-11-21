/*
** $Id: //WIFI_SOC/MP/SDK_4_3_0_0/RT288x_SDK/source/linux-2.6.36.x/arch/mips/kprofile/kprofile.c#1 $
*/
/************************************************************************
 *
 *	Copyright (C) 2010 Trendchip Technologies, Corp.
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
/*
** $Log: kprofile.c,v $
** Revision 1.1.1.1  2012-05-10 13:20:56  roger
** add kprofile in linux-2.3.36.x
**
** Revision 1.3  2012-05-04 07:01:46  roger
** change module_init to late_initcall, because kprofile module init  must be later than tccicmd_init
**
** Revision 1.2  2012-05-03 09:35:09  steven
** fix compile error
**
** Revision 1.1  2012-05-03 09:30:52  steven
** add kprofile
**
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <linux/mii.h>
//--#include <linux/pktflow.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/jhash.h>
#include <linux/random.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <net/ip.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <asm/cache.h>
#include <asm/cacheops.h>
#include <asm/r4kcache.h>
#include <asm/io.h>
//#include <asm/tc3162/tc3162.h>
//--#include <asm/tc3162/cmdparse.h>
#include "cmdparse.h"
//--#include <asm/tc3162/kprofile_hook.h>
#include "kprofile_hook.h"

#include "kprofile.h"

/************************************************************************
*                          C O N S T A N T S
*************************************************************************
*/

/************************************************************************
*                            M A C R O S
*************************************************************************
*/
#define DEF_MAX_LOG_SIZE				4096

#define MAX_PERF_COUNTERS				4

#define M_PERFCTL_EXL					(1UL <<  0)
#define M_PERFCTL_KERNEL				(1UL <<  1)
#define M_PERFCTL_SUPERVISOR			(1UL <<  2)
#define M_PERFCTL_USER					(1UL <<  3)
#define M_PERFCTL_INTERRUPT_ENABLE		(1UL <<  4)
#define M_PERFCTL_EVENT(event)			(((event) & 0x3f) << 5)
#define M_PERFCTL_VPEID(vpe)			((vpe)    << 16)
#define M_PERFCTL_MT_EN(filter)			((filter) << 20)
#define    M_TC_EN_ALL			M_PERFCTL_MT_EN(0)
#define    M_TC_EN_VPE			M_PERFCTL_MT_EN(1)
#define    M_TC_EN_TC			M_PERFCTL_MT_EN(2)
#define M_PERFCTL_TCID(tcid)			((tcid) << 22)
#define M_PERFCTL_MORE					(1UL << 31)

#define M_COUNTER_OVERFLOW				(1UL << 31)

#define WHAT(x)		0
#define vpe_id()	0
#if 0
#define read_tc_c0_perfctrl0()    		mftc0(25, 0)
#define read_tc_c0_perfctrl1()    		mftc0(25, 2)
#define read_tc_c0_perfctrl2()    		mftc0(25, 4)
#define read_tc_c0_perfctrl3()    		mftc0(25, 6)

#define write_tc_c0_perfctrl0(val)    	mttc0(25, 0, val)
#define write_tc_c0_perfctrl1(val)    	mttc0(25, 2, val)
#define write_tc_c0_perfctrl2(val)    	mttc0(25, 4, val)
#define write_tc_c0_perfctrl3(val)    	mttc0(25, 6, val)

#define read_tc_c0_perfcntr0()    		mftc0(25, 1)
#define read_tc_c0_perfcntr1()    		mftc0(25, 3)
#define read_tc_c0_perfcntr2()    		mftc0(25, 5)
#define read_tc_c0_perfcntr3()    		mftc0(25, 7)

#define write_tc_c0_perfcntr0(val)    	mttc0(25, 1, val)
#define write_tc_c0_perfcntr1(val)    	mttc0(25, 3, val)
#define write_tc_c0_perfcntr2(val)    	mttc0(25, 5, val)
#define write_tc_c0_perfcntr3(val)    	mttc0(25, 7, val)
#else

#define read_tc_c0_perfctrl0()    		read_c0_perfctrl0()
#define read_tc_c0_perfctrl1()    		read_c0_perfctrl1()
#define read_tc_c0_perfctrl2()    		read_c0_perfctrl2()
#define read_tc_c0_perfctrl3()    		read_c0_perfctrl3()

#define write_tc_c0_perfctrl0(val)    	write_c0_perfctrl0(val)
#define write_tc_c0_perfctrl1(val)    	write_c0_perfctrl1(val)
#define write_tc_c0_perfctrl2(val)    	write_c0_perfctrl2(val)
#define write_tc_c0_perfctrl3(val)    	write_c0_perfctrl3(val)

#define read_tc_c0_perfcntr0()    		read_c0_perfcntr0()
#define read_tc_c0_perfcntr1()    		read_c0_perfcntr1()
#define read_tc_c0_perfcntr2()    		read_c0_perfcntr2()
#define read_tc_c0_perfcntr3()    		read_c0_perfcntr3()

#define write_tc_c0_perfcntr0(val)    	write_c0_perfcntr0(val)
#define write_tc_c0_perfcntr1(val)    	write_c0_perfcntr1(val)
#define write_tc_c0_perfcntr2(val)    	write_c0_perfcntr2(val)
#define write_tc_c0_perfcntr3(val)    	write_c0_perfcntr3(val)


#endif

#define __define_perf_accessors(r, n, np)				\
									\
static inline unsigned int r_c0_ ## r ## n(void)			\
{									\
	return read_tc_c0_ ## r ## n();				\
}									\
									\
static inline void w_c0_ ## r ## n(unsigned int value)			\
{									\
	write_tc_c0_ ## r ## n(value);				\
	return;							\
}									

__define_perf_accessors(perfcntr, 0, 2)
__define_perf_accessors(perfcntr, 1, 3)
__define_perf_accessors(perfcntr, 2, 0)
__define_perf_accessors(perfcntr, 3, 1)

__define_perf_accessors(perfctrl, 0, 2)
__define_perf_accessors(perfctrl, 1, 3)
__define_perf_accessors(perfctrl, 2, 0)
__define_perf_accessors(perfctrl, 3, 1)

#define M_CONFIG1_PC	(1 << 4)

/************************************************************************
*                         D A T A   T Y P E S
*************************************************************************
*/
struct ctr_config {
	unsigned long enabled;
	unsigned long event;
	unsigned long count;
	unsigned long kernel;
	unsigned long user;
	unsigned long exl;
};

struct register_config {
	unsigned int control[MAX_PERF_COUNTERS];
	unsigned int counter[MAX_PERF_COUNTERS];
};

struct profiling_log {
	unsigned int cp0_count;
	unsigned int event0_cnt;
	unsigned int event1_cnt;
	unsigned int label;
	unsigned int cpu;
	unsigned int usr_data;
};

struct profiling_ctrl {
	unsigned int index;
	unsigned int size;
	unsigned int enabled;
	unsigned int wrap;
};

/************************************************************************
*              F U N C T I O N   D E C L A R A T I O N S
*************************************************************************
*/

/************************************************************************
*                        P U B L I C   D A T A
*************************************************************************
*/

/************************************************************************
*                      E X T E R N A L   D A T A
*************************************************************************
*/

/************************************************************************
*                       P R I V A T E   D A T A
*************************************************************************
*/
/* define counter config */
static struct ctr_config ctr_cfg[MAX_PERF_COUNTERS];

/* define register config */
static struct register_config reg_cfg;

/* define num of counter */
static unsigned char num_counters = 0;

/* profiling log data */
static struct profiling_log *p_log = NULL;

/* profiling control */
static struct profiling_ctrl p_ctrl;

/* log lock */
DEFINE_SPINLOCK(log_lock);

unsigned int tbl_size;
	
volatile unsigned int g_calibrate;
volatile unsigned int g_regs[8];

volatile unsigned int *g_dcache_test = 0x80010000;
volatile unsigned int *g_u_mem_test = 0xa0010000;
volatile unsigned int *g_c_mem_test = 0x80010000;
volatile unsigned int *g_u_sram_test = 0xbc007000;
volatile unsigned int *g_c_sram_test = 0x9c007000;

/************************************************************************
*        F U N C T I O N   D E F I N I T I O N S
*************************************************************************
*/
static inline int __n_counters(void)
{
	if (!(read_c0_config1() & M_CONFIG1_PC))
		return 0;
	if (!(r_c0_perfctrl0() & M_PERFCTL_MORE))
		return 1;
	if (!(r_c0_perfctrl1() & M_PERFCTL_MORE))
		return 2;
	if (!(r_c0_perfctrl2() & M_PERFCTL_MORE))
		return 3;

	return 4;
}

static inline int n_counters(void)
{
	int counters;

	switch (current_cpu_data.cputype) {
	case CPU_R10000:
		counters = 2;
		break;

	case CPU_R12000:
	case CPU_R14000:
		counters = 4;
		break;

	default:
		counters = __n_counters();
	}
	return counters;
}

void init_profiling_ctrl(void)
{
	/* init profiling control */
	p_ctrl.index = 0;
	p_ctrl.size = DEF_MAX_LOG_SIZE;
	tbl_size = DEF_MAX_LOG_SIZE;
	p_ctrl.enabled = 0;
	p_ctrl.wrap = 1;

	p_log = kzalloc(p_ctrl.size * sizeof(struct profiling_log), GFP_ATOMIC);
	if (p_log == NULL) {
		printk("ERR: cannot alloc p_log memory\n");
	}
}

void init_profiling_ctr(void)
{
	int i;

	for (i = 0; i < MAX_PERF_COUNTERS; i++) {
		/* init counter config */
		ctr_cfg[i].enabled = 1;
		ctr_cfg[i].event   = i;
		ctr_cfg[i].count   = 0;
		ctr_cfg[i].kernel  = 1;
		ctr_cfg[i].user    = 1;
		ctr_cfg[i].exl     = 1;
	}
}

static void mips_profiling_setup(void)
{
	int i;

	for (i = 0; i < num_counters; i++) {
		reg_cfg.control[i] = 0;
		reg_cfg.counter[i] = 0;

		if (!ctr_cfg[i].enabled)
			continue;

		reg_cfg.control[i] = M_PERFCTL_EVENT(ctr_cfg[i].event);
		if (ctr_cfg[i].kernel)
			reg_cfg.control[i] |= M_PERFCTL_KERNEL;
		if (ctr_cfg[i].user)
			reg_cfg.control[i] |= M_PERFCTL_USER;
		if (ctr_cfg[i].exl)
			reg_cfg.control[i] |= M_PERFCTL_EXL;
		reg_cfg.counter[i] = 0x80000000 - ctr_cfg[i].count;
	}
}

static void mips_profiling_start(void)
{
	int vpflags;
	int tc;

	//vpflags = dvpe();	
	for (tc = 0; tc < NR_CPUS; tc++) { 
		preempt_disable(); 
		settc(tc); 
		switch (num_counters) { 
		case 4:
			w_c0_perfctrl3(WHAT(tc) | reg_cfg.control[3]);
			w_c0_perfcntr3(reg_cfg.counter[3]);
		case 3:
			w_c0_perfctrl2(WHAT(tc) | reg_cfg.control[2]);
			w_c0_perfcntr2(reg_cfg.counter[2]);
		case 2:
			w_c0_perfctrl1(WHAT(tc) | reg_cfg.control[1]);
			w_c0_perfcntr1(reg_cfg.counter[1]);
		case 1:
			w_c0_perfctrl0(WHAT(tc) | reg_cfg.control[0]);
			w_c0_perfcntr0(reg_cfg.counter[0]);
		}
		preempt_enable();
	}
	//evpe(vpflags);
}

static void mips_profiling_stop(void)
{
	int vpflags;
	int tc;

	//vpflags = dvpe();	
	for (tc = 0; tc < NR_CPUS; tc++) { 
		preempt_disable(); 
		settc(tc); 
		switch (num_counters) { 
		case 4:
			w_c0_perfctrl3(0);
		case 3:
			w_c0_perfctrl2(0);
		case 2:
			w_c0_perfctrl1(0);
		case 1:
			w_c0_perfctrl0(0);
		}
		preempt_enable();
	}
	//evpe(vpflags);
}

/*______________________________________________________________________________
**  profilingSetup
**
**  descriptions: 
**  execution sequence:
**  commands process:
**  parameters:
**  local:
**  global:
**  return:
**  called by:
**  call:
**____________________________________________________________________________*/
void profilingSetup(unsigned int tbl_size)
{
	if (p_ctrl.size != tbl_size) {
		p_ctrl.size = tbl_size;

		kfree(p_log);

		p_log = kzalloc(p_ctrl.size * sizeof(struct profiling_log), GFP_ATOMIC);
		if (p_log == NULL) {
			printk("ERR: cannot alloc p_log memory\n");
		}
	} else {
		if (p_log)
			memset(p_log, 0, p_ctrl.size * sizeof(struct profiling_log));
	}

	if (p_log == NULL) 
		p_ctrl.enabled = 0;
}
EXPORT_SYMBOL(profilingSetup);

/*______________________________________________________________________________
**  profilingEvent
**
**  descriptions: 
**  execution sequence:
**  commands process:
**  parameters:
**  local:
**  global:
**  return:
**  called by:
**  call:
**____________________________________________________________________________*/
void profilingEvent(unsigned int cntr, unsigned int event, unsigned int count, 
						unsigned int kernel, unsigned int user, unsigned int exl)
{
	if (cntr < num_counters) {
		ctr_cfg[cntr].event   = event;
		ctr_cfg[cntr].count   = count;
		ctr_cfg[cntr].kernel  = kernel ? 1 : 0;
		ctr_cfg[cntr].user    = user ? 1 : 0;
		ctr_cfg[cntr].exl     = exl ? 1 : 0;
	}
}
EXPORT_SYMBOL(profilingEvent);

/*______________________________________________________________________________
**  profilingStart
**
**  descriptions: 
**  execution sequence:
**  commands process:
**  parameters:
**  local:
**  global:
**  return:
**  called by:
**  call:
**____________________________________________________________________________*/
void profilingStart(void)
{
	p_ctrl.enabled = 1;

	profilingSetup(tbl_size);
	mips_profiling_setup();
	mips_profiling_start();
}
EXPORT_SYMBOL(profilingStart);

/*______________________________________________________________________________
**  profilingStop
**
**  descriptions: 
**  execution sequence:
**  commands process:
**  parameters:
**  local:
**  global:
**  return:
**  called by:
**  call:
**____________________________________________________________________________*/
void profilingStop(void)
{
	p_ctrl.enabled = 0;

	mips_profiling_stop();
}
EXPORT_SYMBOL(profilingStop);

/*______________________________________________________________________________
**  profilingLog
**
**  descriptions: 
**  execution sequence:
**  commands process:
**  parameters:
**  local:
**  global:
**  return:
**  called by:
**  call:
**____________________________________________________________________________*/
void profilingLog(unsigned int label, unsigned int usr_data)
{
	unsigned long flags;

	if (p_ctrl.enabled == 0)
		return;

	if ((p_ctrl.wrap == 0) && (p_ctrl.index == (p_ctrl.size - 1)))
		return;

	spin_lock_irqsave(&log_lock, flags);
	p_log[p_ctrl.index].cp0_count  = read_c0_count();
	p_log[p_ctrl.index].event0_cnt = r_c0_perfcntr0();
	p_log[p_ctrl.index].event1_cnt = r_c0_perfcntr1();
	p_log[p_ctrl.index].label      = label;
	p_log[p_ctrl.index].cpu        = smp_processor_id();
	p_log[p_ctrl.index].usr_data   = usr_data;
	p_ctrl.index++;
	if (p_ctrl.index >= p_ctrl.size)
		p_ctrl.index = 0;
	spin_unlock_irqrestore(&log_lock, flags);
}
EXPORT_SYMBOL(profilingLog);

/*______________________________________________________________________________
**  profilingDump
**
**  descriptions: 
**  execution sequence:
**  commands process:
**  parameters:
**  local:
**  global:
**  return:
**  called by:
**  call:
**____________________________________________________________________________*/
void profilingDump(void)
{
	unsigned int index = p_ctrl.index;
	unsigned int entry = 0;
	unsigned int i;

	printk("Performance counters\n");
	for (i = 0; i < num_counters; i++) {
		printk("PerfCount%d=%d KERNEL=%d USER=%d EXL=%d\n", 
				i, ctr_cfg[i].event, ctr_cfg[i].kernel, 
					ctr_cfg[i].user, ctr_cfg[i].exl);
	}

	printk("index CP0_Count event0 event1 label cpu user_data\n");
	do {
		if (p_log[index].cp0_count && p_log[index].event0_cnt && p_log[index].event1_cnt) {
			printk("%d %08x %08x %08x %d %d %d\n",
				entry, p_log[index].cp0_count, p_log[index].event0_cnt, p_log[index].event1_cnt,
				p_log[index].label, p_log[index].cpu, p_log[index].usr_data);
			entry++;
		}
		index++;
		if (index == p_ctrl.size)
			index = 0;
	} while (index != p_ctrl.index);
}
EXPORT_SYMBOL(profilingDump);

/************************************************************************
*                    C I     C O M M A N D S 
*************************************************************************
*/
static int do_kprofile(int argc, char *argv[], void *p);
static int do_kprofile_start(int argc, char *argv[], void *p);
static int do_kprofile_stop(int argc, char *argv[], void *p);
static int do_kprofile_dump(int argc, char *argv[], void *p);
static int do_kprofile_event(int argc, char *argv[], void *p);
static int do_kprofile_size(int argc, char *argv[], void *p);
static int do_kprofile_wrap(int argc, char *argv[], void *p);
static int do_kprofile_calibrate(int argc, char *argv[], void *p);
static int do_kprofile_memrl(int argc, char *argv[], void *p);
static int do_kprofile_memwl(int argc, char *argv[], void *p);
static int do_kprofile_memrwl(int argc, char *argv[], void *p);
static int do_kprofile_memrlrg(int argc, char *argv[], void *p);
static int do_kprofile_memwlrg(int argc, char *argv[], void *p);
static int do_kprofile_regtest(int argc, char *argv[], void *p);
static int do_kprofile_test(int argc, char *argv[], void *p);
static int do_kprofile_test2(int argc, char *argv[], void *p);

static const cmds_t kprofile_cmds[] = {
	{"start",		do_kprofile_start,		0x02,	0,	NULL},
	{"stop",		do_kprofile_stop,		0x02,	0,	NULL},
	{"dump",		do_kprofile_dump,		0x02,	0,	NULL},
	{"event",		do_kprofile_event,		0x02,	6,	"<entry> <event> <count> <kernel> <user> <exl>"},
	{"size",		do_kprofile_size,		0x02,	1,	"<size>"},
	{"wrap",		do_kprofile_wrap,		0x02,	1,	"<1|0>"},
	{"calibrate",	do_kprofile_calibrate,	0x02,	0,	NULL},
	{"memrl",		do_kprofile_memrl,		0x02,	1,	NULL},
	{"memwl",		do_kprofile_memwl,		0x02,	2,	NULL},
	{"memrwl",		do_kprofile_memrwl,		0x02,	1,	NULL},
	{"memrlrg",		do_kprofile_memrlrg,	0x02,	2,	NULL},
	{"memwlrg",		do_kprofile_memwlrg,	0x02,	3,	NULL},
	{"regtest",		do_kprofile_regtest,	0x02,	0,	NULL},
	{"test",		do_kprofile_test,		0x02,	0,	NULL},
	{"test2",		do_kprofile_test2,		0x02,	0,	NULL},
	{NULL,			NULL,					0x10,	0,	NULL},
};

static int do_kprofile(int argc, char *argv[], void *p)
{
	return subcmd(kprofile_cmds, argc, argv, p);
}

static int do_kprofile_start(int argc, char *argv[], void *p)
{
	profilingStart();
	return 0;
}

static int do_kprofile_stop(int argc, char *argv[], void *p)
{
	profilingStop();
	return 0;
}

static int do_kprofile_dump(int argc, char *argv[], void *p)
{
	profilingDump();
	return 0;
}

static int do_kprofile_event(int argc, char *argv[], void *p)
{
	unsigned int entry, event, count, kernel, user, exl;

	entry = (unsigned int) simple_strtoul(argv[1], NULL, 10);
	event = (unsigned int) simple_strtoul(argv[2], NULL, 10);
	count = (unsigned int) simple_strtoul(argv[3], NULL, 10);
	kernel = (unsigned int) simple_strtoul(argv[4], NULL, 10);
	user = (unsigned int) simple_strtoul(argv[5], NULL, 10);
	exl = (unsigned int) simple_strtoul(argv[6], NULL, 10);

	profilingEvent(entry, event, count, kernel, user, exl);

	return 0;
}

static int do_kprofile_size(int argc, char *argv[], void *p)
{
	unsigned int size;

	size = (unsigned int) simple_strtoul(argv[1], NULL, 10);

	tbl_size = size;
	return 0;
}

static int do_kprofile_wrap(int argc, char *argv[], void *p)
{
	unsigned int wrap;

	wrap = (unsigned int) simple_strtoul(argv[1], NULL, 10) ? 1 : 0;

	p_ctrl.wrap = wrap;
	return 0;
}

static int do_kprofile_calibrate(int argc, char *argv[], void *p)
{
	unsigned long flags;
	unsigned long vpflags;

	unsigned int start_cycles_cnt;
	unsigned int start_inst_cnt;
	unsigned int end_cycles_cnt;
	unsigned int end_inst_cnt;
	int i; 

	volatile unsigned int val;
	extern void __ICACHE_LOOP(void);

	local_irq_save(flags);
	//vpflags = dvpe();

	profilingStart();
	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		__asm__ __volatile__("nop;"); 
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Normal (nop)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		g_calibrate = *(volatile unsigned int *) 0xbfb50028;
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("AHB Register read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		g_dcache_test[0] = *(volatile unsigned int *) 0xbfb50000;
		g_dcache_test[1] = *(volatile unsigned int *) 0xbfb50004;
		g_dcache_test[2] = *(volatile unsigned int *) 0xbfb50008;
		g_dcache_test[3] = *(volatile unsigned int *) 0xbfb5000c;
		g_dcache_test[4] = *(volatile unsigned int *) 0xbfb50010;
		g_dcache_test[5] = *(volatile unsigned int *) 0xbfb50014;
		g_dcache_test[6] = *(volatile unsigned int *) 0xbfb50018;
		g_dcache_test[7] = *(volatile unsigned int *) 0xbfb5001c;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("AHB Register uncached read (8 regs)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_D, 0x9fb50000);
		g_dcache_test[0] = *(volatile unsigned int *) 0x9fb50000;
		g_dcache_test[1] = *(volatile unsigned int *) 0x9fb50004;
		g_dcache_test[2] = *(volatile unsigned int *) 0x9fb50008;
		g_dcache_test[3] = *(volatile unsigned int *) 0x9fb5000c;
		g_dcache_test[4] = *(volatile unsigned int *) 0x9fb50010;
		g_dcache_test[5] = *(volatile unsigned int *) 0x9fb50014;
		g_dcache_test[6] = *(volatile unsigned int *) 0x9fb50018;
		g_dcache_test[7] = *(volatile unsigned int *) 0x9fb5001c;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("AHB Register cached read (8 regs)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		g_dcache_test[0] = *(volatile unsigned int *) 0xbfb50000;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("AHB Register uncached read (1 regs)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_D, 0x9fb50000);
		g_dcache_test[0] = *(volatile unsigned int *) 0x9fb50000;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("AHB Register cached read (1 regs)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		*(volatile unsigned int *) 0xbfb50028 = 0xffffffff;
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("AHB Register write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		g_calibrate = *(volatile unsigned int *) 0xbfbf012c;
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("APB Register read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		*(volatile unsigned int *) 0xbfbf012c = 0xffffffff;
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("APB Register write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_D, 0x80020000);
		g_dcache_test[1] = g_dcache_test[0];
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("DCache miss penality (w/o miss)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_D, g_dcache_test);
		g_dcache_test[1] = g_dcache_test[0];
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("DCache miss penality (w miss)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_I, __ICACHE_LOOP);
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("ICache miss penality (w/o miss)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_I, __ICACHE_LOOP);
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
__asm__(" 	.globl __ICACHE_LOOP	\n"
		"__ICACHE_LOOP:			\n");
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
		__asm__ __volatile__("nop; nop; nop; nop;"); 
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("ICache miss penality (w miss)\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		val = *g_u_sram_test;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached SRAM read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		*g_u_sram_test = i;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached SRAM write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		val = *g_c_sram_test;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached SRAM read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		*g_c_sram_test = i;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached SRAM write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		val = *g_c_mem_test;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached memory read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		*g_c_mem_test = i;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached memory write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		val = *g_u_mem_test;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached memory read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		*g_u_mem_test = i;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached memory write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_D, 0x80010200);
		g_dcache_test[0] = *(volatile unsigned int *) 0x80010200;
		g_dcache_test[1] = *(volatile unsigned int *) 0x80010204;
		g_dcache_test[2] = *(volatile unsigned int *) 0x80010208;
		g_dcache_test[3] = *(volatile unsigned int *) 0x8001020c;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached descriptor read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		g_dcache_test[0] = *(volatile unsigned int *) 0xa0010200;
		g_dcache_test[1] = *(volatile unsigned int *) 0xa0010204;
		g_dcache_test[2] = *(volatile unsigned int *) 0xa0010208;
		g_dcache_test[3] = *(volatile unsigned int *) 0xa001020c;
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached descriptor read\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		*(volatile unsigned int *) 0x80010200 = g_dcache_test[0];
		*(volatile unsigned int *) 0x80010204 = g_dcache_test[1];
		*(volatile unsigned int *) 0x80010208 = g_dcache_test[2];
		*(volatile unsigned int *) 0x8001020c = g_dcache_test[3];
		cache_op(Hit_Writeback_Inv_D, 0x80010200);
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached descriptor write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		*(volatile unsigned int *) 0xa0010200 = g_dcache_test[0];
		*(volatile unsigned int *) 0xa0010204 = g_dcache_test[1];
		*(volatile unsigned int *) 0xa0010208 = g_dcache_test[2];
		*(volatile unsigned int *) 0xa001020c = g_dcache_test[3];
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached descriptor write\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		cache_op(Hit_Invalidate_D, 0x80010200);
		g_dcache_test[0] = *(volatile unsigned int *) 0x80010200;
		g_dcache_test[1] = *(volatile unsigned int *) 0x80010204;
		g_dcache_test[2] = *(volatile unsigned int *) 0x80010208;
		g_dcache_test[3] = *(volatile unsigned int *) 0x8001020c;
		*(volatile unsigned int *) 0x80010200 = g_dcache_test[0];
		*(volatile unsigned int *) 0x80010204 = g_dcache_test[1];
		*(volatile unsigned int *) 0x80010208 = g_dcache_test[2];
		*(volatile unsigned int *) 0x8001020c = g_dcache_test[3];
		cache_op(Hit_Writeback_Inv_D, 0x80010200);
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Cached descriptor R/W\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);

	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++) {
		g_dcache_test[0] = *(volatile unsigned int *) 0xa0010200;
		g_dcache_test[1] = *(volatile unsigned int *) 0xa0010204;
		g_dcache_test[2] = *(volatile unsigned int *) 0xa0010208;
		g_dcache_test[3] = *(volatile unsigned int *) 0xa001020c;
		*(volatile unsigned int *) 0xa0010200 = g_dcache_test[0];
		*(volatile unsigned int *) 0xa0010204 = g_dcache_test[1];
		*(volatile unsigned int *) 0xa0010208 = g_dcache_test[2];
		*(volatile unsigned int *) 0xa001020c = g_dcache_test[3];
	}
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("Uncached descriptor R/W\n");
	printk("%08x %08x\n", start_cycles_cnt, start_inst_cnt);
	printk("%08x %08x\n", end_cycles_cnt, end_inst_cnt);


	profilingStop();

	//evpe(vpflags);
	local_irq_restore(flags);

	return 0;
}

static unsigned int kprofileMemrl(unsigned int *ptr)
{
	unsigned long flags;
	unsigned long vpflags;

	unsigned int start_cycles_cnt;
	unsigned int start_inst_cnt;
	unsigned int end_cycles_cnt;
	unsigned int end_inst_cnt;
	unsigned int i;

	volatile unsigned int val;

	local_irq_save(flags);
	//vpflags = dvpe();

	profilingStart();
	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		val = *(volatile unsigned int *)ptr;
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("READ  %08x cycles=%d instructions=%d IPC=0.%02d\n", ptr,
			end_cycles_cnt-start_cycles_cnt, end_inst_cnt-start_inst_cnt, 
			100*(end_inst_cnt-start_inst_cnt)/(end_cycles_cnt-start_cycles_cnt));

	profilingStop();

	//evpe(vpflags);
	local_irq_restore(flags);

	return val;
}

static int kprofileMemwl(unsigned int *ptr, unsigned int val)
{
	unsigned long flags;
	unsigned long vpflags;

	unsigned int start_cycles_cnt;
	unsigned int start_inst_cnt;
	unsigned int end_cycles_cnt;
	unsigned int end_inst_cnt;
	unsigned int i;

	local_irq_save(flags);
	//vpflags = dvpe();

	profilingStart();
	start_cycles_cnt = r_c0_perfcntr0();
	start_inst_cnt = r_c0_perfcntr1();
	for (i = 0; i < 1000; i++)
		*(volatile unsigned int *)ptr = val;
	end_cycles_cnt = r_c0_perfcntr0();
	end_inst_cnt = r_c0_perfcntr1();
	printk("WRITE %08x cycles=%d instructions=%d IPC=0.%02d\n", ptr,
			end_cycles_cnt-start_cycles_cnt, end_inst_cnt-start_inst_cnt, 
			100*(end_inst_cnt-start_inst_cnt)/(end_cycles_cnt-start_cycles_cnt));

	profilingStop();

	//evpe(vpflags);
	local_irq_restore(flags);

	return 0;
}

static int do_kprofile_memrl(int argc, char *argv[], void *p)
{
	unsigned int val;
	unsigned int *ptr;

	ptr = (unsigned int *)simple_strtoul(argv[1], NULL, 16);

	val = kprofileMemrl(ptr);

	return 0;
}

static int do_kprofile_memwl(int argc, char *argv[], void *p)
{
	unsigned int val;
	unsigned int *ptr;

	ptr = (unsigned int *)simple_strtoul(argv[1], NULL, 16);
	val = (unsigned int)simple_strtoul(argv[2], NULL, 16);

	kprofileMemwl(ptr, val);

	return 0;
}

static int do_kprofile_memrwl(int argc, char *argv[], void *p)
{
	unsigned int val;
	unsigned int *ptr;

	ptr = (unsigned int *)simple_strtoul(argv[1], NULL, 16);

	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	return 0;
}

static int do_kprofile_memrlrg(int argc, char *argv[], void *p)
{
	unsigned int val;
	unsigned int *ptr;
	unsigned int len;
	unsigned int i = 0;

	ptr = (unsigned int *)simple_strtoul(argv[1], NULL, 16);
	len = (unsigned int)simple_strtoul(argv[2], NULL, 16);

	while (i <= len) {
		val = kprofileMemrl(ptr);
		ptr++;
		i += 4;
	}

	return 0;
}

static int do_kprofile_memwlrg(int argc, char *argv[], void *p)
{
	unsigned int val;
	unsigned int *ptr;
	unsigned int len;
	unsigned int i = 0;

	ptr = (unsigned int *)simple_strtoul(argv[1], NULL, 16);
	val = (unsigned int)simple_strtoul(argv[2], NULL, 16);
	len = (unsigned int)simple_strtoul(argv[3], NULL, 16);

	while (i <= len) {
		kprofileMemwl(ptr, val);
		ptr++;
		i += 4;
	}

	return 0;
}

static int do_kprofile_regtest(int argc, char *argv[], void *p)
{
	unsigned int val;
	unsigned int *ptr;
	unsigned int reg;

	ptr = 0xbfb00004;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb10000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb20008;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb30000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb40000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb50028;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb5800c;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb60008;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb70008;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb80000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfb82000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfba0004;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbb0000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbc0008;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbd0000;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbe0010;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf002c;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf0100;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf0200;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf032c;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf0400;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf8008;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfbf8100;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xb4000040;
	val = kprofileMemrl(ptr);
	kprofileMemwl(ptr, val);

	ptr = 0xbfc00000;
	val = kprofileMemrl(ptr);

	reg = *(volatile unsigned int *) 0xbfb00090;
	reg &= ~((1<<21)|(1<<20));
	*(volatile unsigned int *) 0xbfb00090 = reg;
	ptr = 0xbfc00000;
	val = kprofileMemrl(ptr);
	reg = *(volatile unsigned int *) 0xbfb00090;
	reg |= ((1<<21)|(1<<20));
	*(volatile unsigned int *) 0xbfb00090 = reg;

	return 0;
}

static int do_kprofile_test(int argc, char *argv[], void *p)
{
	int vpflags;
	int tc;

	profilingLog(0, 0);
	//vpflags = dvpe();	
	for (tc = 0; tc < NR_CPUS; tc++) {
		preempt_disable();
		settc(tc);
		printk("tc=%d\n", tc);
		printk("perfcntr0=%08x\n", r_c0_perfcntr0());
		printk("perfctrl0=%08x\n", r_c0_perfctrl0());
		printk("perfcntr1=%08x\n", r_c0_perfcntr1());
		printk("perfctrl1=%08x\n", r_c0_perfctrl1());
		preempt_enable();
	}
	//evpe(vpflags);
	profilingLog(1, 0);

	return 0;
}

static int do_kprofile_test2(int argc, char *argv[], void *p)
{
	int i;

	for (i = 0; i < 8192; i++) {
		profilingLog(0, 0);
		profilingLog(1, 0);
	}

	return 0;
}

static void kprofile_cmd_init(void)
{
	cmds_t kprofile_cmd;

	/* init kprofile root ci-cmd */
	kprofile_cmd.name = "profiling";
	kprofile_cmd.func = do_kprofile;
	kprofile_cmd.flags = 0x12;
	kprofile_cmd.argcmin = 0;
	kprofile_cmd.argc_errmsg = NULL;

	/* register kprofile ci-cmd */
	cmd_register(&kprofile_cmd);
}

static void kprofile_cmd_exit(void)
{
	cmd_unregister("profiling");
}

static int kprofile_start(void)
{
	printk("kprofile 1.0 %s\n",  "("__DATE__"-"__TIME__")");

	kprofile_cmd_init();

	num_counters = n_counters();
	if (num_counters == 0) {
		printk(KERN_ERR "kprofile: CPU has no performance counters\n");
		return -ENODEV;
	}

	init_profiling_ctrl();
	init_profiling_ctr();

	profilingSetupHook = profilingSetup;
	profilingEventHook = profilingEvent;
	profilingStartHook = profilingStart;
	profilingStopHook = profilingStop;
	profilingLogHook = profilingLog;

	return 0;
}

static void kprofile_stop(void)
{
	profilingSetupHook = NULL;
	profilingEventHook = NULL;
	profilingStartHook = NULL;
	profilingStopHook = NULL;
	profilingLogHook = NULL;

	kprofile_cmd_exit();
}

static int __init kprofile_init(void)
{
	return kprofile_start();
}

static void __exit kprofile_exit(void)
{
	kprofile_stop();
}

late_initcall(kprofile_init);
module_exit(kprofile_exit);
MODULE_LICENSE("Proprietary");
