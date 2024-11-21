/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rt_proc.c

    Abstract:
    Create and register proc file system for ralink device

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "rt_config.h"

int wl_proc_init(void);
int wl_proc_exit(void);

#ifdef CONFIG_RALINK_RT2880
#define PROCREG_DIR             "rt2880"
#endif /* CONFIG_RALINK_RT2880 */


/*
add /mt7613/value and /mt7613/status to /proc
changed by wanghe
*/
#define PROCREG_DIR "mt7613"

static struct proc_dir_entry *mt7613_entry = NULL;
static struct proc_dir_entry *mt7613_entry_ibf_status = NULL;
static struct proc_dir_entry *mt7613_entry_ibf_value = NULL;
UINT_8 ibf_value[10] = {0};
UCHAR ibf_status = 0;

static int ibf_read_status(char *page,char **start, off_t off, int count, int *eof, void *data)
{
	char buff[64];
	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), "Calibrated result = %d\n", ibf_status);
	int len = strlen(buff);
	if (off >= len)return 0;
	if (count > len - off)
	{
		count = len - off;
		//*eof = 1;
	}
	memcpy(page + off, buff + off, count);
	return off + count;
}

static int ibf_read_value(char *page,char **start, off_t off, int count, int *eof, void *data)
{
	char buff[64];
	memset(buff, 0, sizeof(buff));
	int len = snprintf(buff, sizeof(buff), "%d\n %d\n %d\n %d\n %d\n %d\n %d\n %d\n %d\n %d",
		 				ibf_value[0],
		 				ibf_value[1],
		 				ibf_value[2],
		 				ibf_value[3],
		 				ibf_value[4],
		 				ibf_value[5],
		 				ibf_value[6],
		 				ibf_value[7],
		 				ibf_value[8],
						ibf_value[9]);
	if (len >= sizeof(buff) || len <= 0)
	{
		printk("ibf_read_value error: out of range");
		return -1;
	}
	if (off >= len)return 0;
	if (count > len - off)
	{
		count = len - off;
		//*eof = 1;
	}
	memcpy(page + off, buff + off, count);
	return off + count;
}

static int ibf_clear(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	memset(ibf_value, 0, sizeof(ibf_value));
	ibf_status = 0;
	printk("ibf data cleared!\n");
	return 0;
}

int wl_proc_init_mt7613(void)
{
	int rv = 0;
	mt7613_entry = proc_mkdir(PROCREG_DIR, NULL);

	if (mt7613_entry == NULL)
	{
		rv = -ENOMEM;
		goto out;
	}

	mt7613_entry_ibf_status = create_proc_entry("status", 0, mt7613_entry);
	if(mt7613_entry_ibf_status == NULL)
	{
		rv = -ENOMEM;
		goto no_status;
	}
	mt7613_entry_ibf_status->read_proc = ibf_read_status;
	mt7613_entry_ibf_status->write_proc = ibf_clear;

	mt7613_entry_ibf_value = create_proc_entry("value", 0, mt7613_entry);
	if(mt7613_entry_ibf_value == NULL)
	{
		rv = -ENOMEM;
		goto no_value;
	}
	mt7613_entry_ibf_value->read_proc = ibf_read_value;
	mt7613_entry_ibf_value->write_proc = ibf_clear;

	return 0;
no_value:
	remove_proc_entry("status", mt7613_entry);
no_status:
	remove_proc_entry("mt7613", NULL);
out:
	return rv;
}

int wl_proc_exit_mt7613(void)
{
	if (mt7613_entry_ibf_status)
		remove_proc_entry("status", mt7613_entry);
	if (mt7613_entry_ibf_value)
		remove_proc_entry("value", mt7613_entry);
	if (mt7613_entry)
		remove_proc_entry("mt7613", NULL);
	return 0;
}


#ifdef CONFIG_RALINK_RT3052
#define PROCREG_DIR             "rt3052"
#endif /* CONFIG_RALINK_RT3052 */

#ifdef CONFIG_RALINK_RT2883
#define PROCREG_DIR             "rt2883"
#endif /* CONFIG_RALINK_RT2883 */

#ifdef CONFIG_RALINK_RT3883
#define PROCREG_DIR             "rt3883"
#endif /* CONFIG_RALINK_RT3883 */

#ifdef CONFIG_RALINK_RT5350
#define PROCREG_DIR             "rt5350"
#endif /* CONFIG_RALINK_RT5350 */

#ifndef PROCREG_DIR
#define PROCREG_DIR             "rt2880"
#endif /* PROCREG_DIR */

#if defined(CONFIG_PROC_FS)
extern struct proc_dir_entry *procRegDir;

#if defined(VIDEO_TURBINE_SUPPORT)
extern BOOLEAN UpdateFromGlobal;
AP_VIDEO_STRUCT GLOBAL_AP_VIDEO_CONFIG;
/*struct proc_dir_entry *proc_ralink_platform, *proc_ralink_wl, *proc_ralink_wl_video; */
struct proc_dir_entry *proc_ralink_wl, *proc_ralink_wl_video;
static struct proc_dir_entry *entry_wl_video_Update, *entry_wl_video_Enable, *entry_wl_video_ClassifierEnable, *entry_wl_video_HighTxMode, *entry_wl_video_TxPwr, *entry_wl_video_VideoMCSEnable;
static struct proc_dir_entry *entry_wl_video_VideoMCS, *entry_wl_video_TxBASize, *entry_wl_video_TxLifeTimeMode, *entry_wl_video_TxLifeTime, *entry_wl_video_TxRetryLimit;


ssize_t video_Update_get(char *page, char **start, off_t off, int count,
						 int *eof, void *data_unused)
{
	sprintf(page, "%d\n", UpdateFromGlobal);
	*eof = 1;
	return strlen(page);
}

ssize_t video_Update_set(struct file *file, const char __user *buffer,
						 size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		UpdateFromGlobal = val;
	}

	return count;
}

ssize_t video_Enable_get(char *page, char **start, off_t off, int count,
						 int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.Enable);
	*eof = 1;
	return strlen(page);
}

ssize_t video_Enable_set(struct file *file, const char __user *buffer,
						 size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.Enable = val;
	}

	return count;
}

ssize_t video_ClassifierEnable_get(char *page, char **start, off_t off, int count,
								   int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.ClassifierEnable);
	*eof = 1;
	return strlen(page);
}

ssize_t video_ClassifierEnable_set(struct file *file, const char __user *buffer,
								   size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.ClassifierEnable = val;
	}

	return count;
}

ssize_t video_HighTxMode_get(char *page, char **start, off_t off, int count,
							 int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.HighTxMode);
	*eof = 1;
	return strlen(page);
}

ssize_t video_HighTxMode_set(struct file *file, const char __user *buffer,
							 size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.HighTxMode = val;
	}

	return count;
}

ssize_t video_TxPwr_get(char *page, char **start, off_t off, int count,
						int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.TxPwr);
	*eof = 1;
	return strlen(page);
}

ssize_t video_TxPwr_set(struct file *file, const char __user *buffer,
						size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.TxPwr = val;
	}

	return count;
}

ssize_t video_VideoMCSEnable_get(char *page, char **start, off_t off, int count,
								 int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.VideoMCSEnable);
	*eof = 1;
	return strlen(page);
}

ssize_t video_VideoMCSEnable_set(struct file *file, const char __user *buffer,
								 size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.VideoMCSEnable = val;
	}

	return count;
}

ssize_t video_VideoMCS_get(char *page, char **start, off_t off, int count,
						   int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.VideoMCS);
	*eof = 1;
	return strlen(page);
}

ssize_t video_VideoMCS_set(struct file *file, const char __user *buffer,
						   size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.VideoMCS = val;
	}

	return count;
}

ssize_t video_TxBASize_get(char *page, char **start, off_t off, int count,
						   int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.TxBASize);
	*eof = 1;
	return strlen(page);
}

ssize_t video_TxBASize_set(struct file *file, const char __user *buffer,
						   size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.TxBASize = val;
	}

	return count;
}

ssize_t video_TxLifeTimeMode_get(char *page, char **start, off_t off, int count,
								 int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.TxLifeTimeMode);
	*eof = 1;
	return strlen(page);
}

ssize_t video_TxLifeTimeMode_set(struct file *file, const char __user *buffer,
								 size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.TxLifeTimeMode = val;
	}

	return count;
}

ssize_t video_TxLifeTime_get(char *page, char **start, off_t off, int count,
							 int *eof, void *data_unused)
{
	sprintf(page, "%d\n", GLOBAL_AP_VIDEO_CONFIG.TxLifeTime);
	*eof = 1;
	return strlen(page);
}

ssize_t video_TxLifeTime_set(struct file *file, const char __user *buffer,
							 size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 10);

		GLOBAL_AP_VIDEO_CONFIG.TxLifeTime = val;
	}

	return count;
}

ssize_t video_TxRetryLimit_get(char *page, char **start, off_t off, int count,
							   int *eof, void *data_unused)
{
	sprintf(page, "0x%x\n", GLOBAL_AP_VIDEO_CONFIG.TxRetryLimit);
	*eof = 1;
	return strlen(page);
}

ssize_t video_TxRetryLimit_set(struct file *file, const char __user *buffer,
							   size_t count, loff_t *ppos)
{
	char *buf = kmalloc(count, GFP_KERNEL);

	if (buf) {
		unsigned long val;

		if (copy_from_user(buf, buffer, count))
			return -EFAULT;

		if (buf)
			val = os_str_toul(buf, NULL, 16);

		GLOBAL_AP_VIDEO_CONFIG.TxRetryLimit = val;
	}

	return count;
}

int wl_video_proc_init(void)
{
	GLOBAL_AP_VIDEO_CONFIG.Enable = FALSE;
	GLOBAL_AP_VIDEO_CONFIG.ClassifierEnable = FALSE;
	GLOBAL_AP_VIDEO_CONFIG.HighTxMode = FALSE;
	GLOBAL_AP_VIDEO_CONFIG.TxPwr = 0;
	GLOBAL_AP_VIDEO_CONFIG.VideoMCSEnable = FALSE;
	GLOBAL_AP_VIDEO_CONFIG.VideoMCS = 0;
	GLOBAL_AP_VIDEO_CONFIG.TxBASize = 0;
	GLOBAL_AP_VIDEO_CONFIG.TxLifeTimeMode = FALSE;
	GLOBAL_AP_VIDEO_CONFIG.TxLifeTime = 0;
	GLOBAL_AP_VIDEO_CONFIG.TxRetryLimit = 0;
	proc_ralink_wl = proc_mkdir("wl", procRegDir);

	if (proc_ralink_wl)
		proc_ralink_wl_video = proc_mkdir("VideoTurbine", proc_ralink_wl);

	if (proc_ralink_wl_video) {
		entry_wl_video_Update = create_proc_entry("UpdateFromGlobal", 0, proc_ralink_wl_video);

		if (entry_wl_video_Update) {
			entry_wl_video_Update->read_proc = (read_proc_t *)&video_Update_get;
			entry_wl_video_Update->write_proc = (write_proc_t *)&video_Update_set;
		}

		entry_wl_video_Enable = create_proc_entry("Enable", 0, proc_ralink_wl_video);

		if (entry_wl_video_Enable) {
			entry_wl_video_Enable->read_proc = (read_proc_t *)&video_Enable_get;
			entry_wl_video_Enable->write_proc = (write_proc_t *)&video_Enable_set;
		}

		entry_wl_video_ClassifierEnable = create_proc_entry("ClassifierEnable", 0, proc_ralink_wl_video);

		if (entry_wl_video_ClassifierEnable) {
			entry_wl_video_ClassifierEnable->read_proc = (read_proc_t *)&video_ClassifierEnable_get;
			entry_wl_video_ClassifierEnable->write_proc = (write_proc_t *)&video_ClassifierEnable_set;
		}

		entry_wl_video_HighTxMode = create_proc_entry("HighTxMode", 0, proc_ralink_wl_video);

		if (entry_wl_video_HighTxMode) {
			entry_wl_video_HighTxMode->read_proc = (read_proc_t *)&video_HighTxMode_get;
			entry_wl_video_HighTxMode->write_proc = (write_proc_t *)&video_HighTxMode_set;
		}

		entry_wl_video_TxPwr = create_proc_entry("TxPwr", 0, proc_ralink_wl_video);

		if (entry_wl_video_TxPwr) {
			entry_wl_video_TxPwr->read_proc = (read_proc_t *)&video_TxPwr_get;
			entry_wl_video_TxPwr->write_proc = (write_proc_t *)&video_TxPwr_set;
		}

		entry_wl_video_VideoMCSEnable = create_proc_entry("VideoMCSEnable", 0, proc_ralink_wl_video);

		if (entry_wl_video_VideoMCSEnable) {
			entry_wl_video_VideoMCSEnable->read_proc = (read_proc_t *)&video_VideoMCSEnable_get;
			entry_wl_video_VideoMCSEnable->write_proc = (write_proc_t *)&video_VideoMCSEnable_set;
		}

		entry_wl_video_VideoMCS = create_proc_entry("VideoMCS", 0, proc_ralink_wl_video);

		if (entry_wl_video_VideoMCS) {
			entry_wl_video_VideoMCS->read_proc = (read_proc_t *)&video_VideoMCS_get;
			entry_wl_video_VideoMCS->write_proc = (write_proc_t *)&video_VideoMCS_set;
		}

		entry_wl_video_TxBASize = create_proc_entry("TxBASize", 0, proc_ralink_wl_video);

		if (entry_wl_video_TxBASize) {
			entry_wl_video_TxBASize->read_proc = (read_proc_t *)&video_TxBASize_get;
			entry_wl_video_TxBASize->write_proc = (write_proc_t *)&video_TxBASize_set;
		}

		entry_wl_video_TxLifeTimeMode = create_proc_entry("TxLifeTimeMode", 0, proc_ralink_wl_video);

		if (entry_wl_video_TxLifeTimeMode) {
			entry_wl_video_TxLifeTimeMode->read_proc = (read_proc_t *)&video_TxLifeTimeMode_get;
			entry_wl_video_TxLifeTimeMode->write_proc = (write_proc_t *)&video_TxLifeTimeMode_set;
		}

		entry_wl_video_TxLifeTime = create_proc_entry("TxLifeTime", 0, proc_ralink_wl_video);

		if (entry_wl_video_TxLifeTime) {
			entry_wl_video_TxLifeTime->read_proc = (read_proc_t *)&video_TxLifeTime_get;
			entry_wl_video_TxLifeTime->write_proc = (write_proc_t *)&video_TxLifeTime_set;
		}

		entry_wl_video_TxRetryLimit = create_proc_entry("TxRetryLimit", 0, proc_ralink_wl_video);

		if (entry_wl_video_TxRetryLimit) {
			entry_wl_video_TxRetryLimit->read_proc = (read_proc_t *)&video_TxRetryLimit_get;
			entry_wl_video_TxRetryLimit->write_proc = (write_proc_t *)&video_TxRetryLimit_set;
		}
	}

	return 0;
}

int wl_video_proc_exit(void)
{
	if (entry_wl_video_Enable)
		remove_proc_entry("Enable", proc_ralink_wl_video);

	if (entry_wl_video_ClassifierEnable)
		remove_proc_entry("ClassifierEnabl", proc_ralink_wl_video);

	if (entry_wl_video_HighTxMode)
		remove_proc_entry("HighTxMode", proc_ralink_wl_video);

	if (entry_wl_video_TxPwr)
		remove_proc_entry("TxPwr", proc_ralink_wl_video);

	if (entry_wl_video_VideoMCSEnable)
		remove_proc_entry("VideoMCSEnable", proc_ralink_wl_video);

	if (entry_wl_video_VideoMCS)
		remove_proc_entry("VideoMCS", proc_ralink_wl_video);

	if (entry_wl_video_TxBASize)
		remove_proc_entry("TxBASize", proc_ralink_wl_video);

	if (entry_wl_video_TxLifeTimeMode)
		remove_proc_entry("TxLifeTimeMode", proc_ralink_wl_video);

	if (entry_wl_video_TxLifeTime)
		remove_proc_entry("TxLifeTime", proc_ralink_wl_video);

	if (entry_wl_video_TxRetryLimit)
		remove_proc_entry("TxRetryLimit", proc_ralink_wl_video);

	if (proc_ralink_wl_video)
		remove_proc_entry("Video", proc_ralink_wl);

	return 0;
}

#endif /* VIDEO_TURBINE_SUPPORT */

int wl_proc_init(void)
{
	wl_proc_init_mt7613(); /*for test*/
	printk("init mt7613 proc.\n");
/*
	if (procRegDir == NULL)
		procRegDir = proc_mkdir(PROCREG_DIR, NULL);
		//printk("init mt7613 proc.\n");
*/
	if (procRegDir) {
#ifdef VIDEO_TURBINE_SUPPORT
		wl_video_proc_init();
#endif /* VIDEO_TURBINE_SUPPORT */
	}

	return 0;
}

int wl_proc_exit(void)
{
	wl_proc_exit_mt7613();/*for test*/
#ifdef VIDEO_TURBINE_SUPPORT

	if (proc_ralink_wl_video) {
		wl_video_proc_exit();
		remove_proc_entry("Video", proc_ralink_wl);
	}

	if (proc_ralink_wl)
		remove_proc_entry("wl", procRegDir);

#endif /* VIDEO_TURBINE_SUPPORT */
	return 0;
}
#else
int wl_proc_init(void)
{
	wl_proc_init_mt7613(); /*for test*/
	printk("init mt7613 proc.\n");
	return 0;
}

int wl_proc_exit(void)
{
	wl_proc_exit_mt7613();/*for test*/
	return 0;
}
#endif /* CONFIG_PROC_FS */





