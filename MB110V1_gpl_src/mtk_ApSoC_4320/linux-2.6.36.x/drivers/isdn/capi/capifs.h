/* $Id: //WIFI_SOC/MP/SDK_4_3_0_0/RT288x_SDK/source/linux-2.6.36.x/drivers/isdn/capi/capifs.h#1 $
 * 
 * Copyright 2000 by Carsten Paeth <calle@calle.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

#include <linux/dcache.h>

#if defined(CONFIG_ISDN_CAPI_CAPIFS) || defined(CONFIG_ISDN_CAPI_CAPIFS_MODULE)

struct dentry *capifs_new_ncci(unsigned int num, dev_t device);
void capifs_free_ncci(struct dentry *dentry);

#else

static inline struct dentry *capifs_new_ncci(unsigned int num, dev_t device)
{
	return NULL;
}

static inline void capifs_free_ncci(struct dentry *dentry)
{
}

#endif
