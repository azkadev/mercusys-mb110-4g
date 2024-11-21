/*
** $Id: //WIFI_SOC/MP/SDK_4_3_0_0/RT288x_SDK/source/linux-2.6.36.x/arch/mips/kprofile/kprofile.h#1 $
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
** $Log: kprofile.h,v $
** Revision 1.1  2012-05-03 09:30:52  steven
** add kprofile
**
 */
#ifndef __KPROFILE_H
#define __KPROFILE_H

/************************************************************************
*        F U N C T I O N   P R O T O T Y P E S
*************************************************************************
*/
extern void profilingSetup(unsigned int tbl_size);
extern void profilingEvent(unsigned int cntr, unsigned int event, unsigned int count, 
						unsigned int kernel, unsigned int user, unsigned int exl);
extern void profilingStart(void);
extern void profilingStop(void);
extern void profilingLog(unsigned int label, unsigned int usr_data);

/************************************************************************
*                      E X T E R N A L   D A T A
*************************************************************************
*/

#endif /* __KPROFILE_H */

