/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering	the source code	is stricitly prohibited, unless	the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	wf_aon.h

	Abstract:
	Ralink Wireless Chip MAC related definition & structures

	Revision History:
	Who			When		  What
	--------	----------	  ----------------------------------------------
*/


#ifndef __WF_AON_H__
#define __WF_AON_H__

#define WF_AON_BASE		0x2D000

#ifdef DUTY_ADMIT_SUPPORT
#define WF_AON_MTR0 WF_AON_BASE + 0X2C
#define MTR0_THERM_ADMIT_TIME(data) ((data & 0xFFFF) << 16)
#define MTR0_THERM_ADMIT_TIME_MASK        (0xFFFF << 16)
#define MTR0_THERM_USED_TIME             0XFFFF
#define MTR0_THERM_USED_TIME_MASK        0XFFFF

#define WF_AON_MTR1 WF_AON_BASE + 0X30
#define MTR1_THERM_AVERAGE_PERIOD(data) ((data & 0x7F) << 8)
#define MTR1_THERM_AVERAGE_PERIOD_MASK    (0X7F << 8)
#define MTR1_THERM_MT_UNIT_64_US            (1 << 1)
#define MTR1_THERM_ADMIT_TIME_EN            (1 << 0)

#endif

#endif /* __WF_AON_H__ */

