/*
 ***************************************************************************
 * MediaTek Inc.
 *
 * All rights reserved. source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mt_io.c
*/
#include	"rt_config.h"







#ifdef MT7663
UINT32 mt7663_mac_cr_range[] = {
	0x82060000, 0x8000, 0x3FFF, /* WF_PLE */
	0x82068000, 0xc000, 0x450, /* WF_PSE */
	0x8206c000, 0xe000, 0x300, /* PP */
	/*0x820d0000, 0x20000, 0x200, */ /* WF_AON */
	0x820d0000, 0xae000, 0x200, /* WF_AON */
	0x820f0000, 0x20000, 0x10000, /* WF_LMAC */
	0x820fd000, 0x2d000, 0x400, /* WF_MIB */

	0x820e0000, 0x30000, 0x10000, /* WF_WTBL */

	0x80020000, 0xb0000, 0x6000, /* TOP_CFG */
	0x80000000, 0x02000, 0x2000, /* MCU_CFG */
	0x50000000, 0x04000, 0x4000, /* PDMA_CFG */
	0x5000A000, 0x06000, 0x1000, /* DMASHDL_CFG */
	0xA0000000, 0x08000, 0x8000, /* PSE_CFG */
	0x82070000, 0x10000, 0x10000, /* WF_PHY */
	0x81020000, 0xC0000, 0x2000, /* CONN_RGU, for checking FW DL state */
	0x810a0000, 0xaa000, 0x1000, /* CONN_RDD_AHB_WRAP0, for CS6 */
	0x810c0000, 0xac000, 0x1000, /* CONN_RBIST_TOP, for baseband */

	0x0, 0x0, 0x0,
};
#endif /* MT7663 */



BOOLEAN mt_mac_cr_range_mapping(RTMP_ADAPTER *pAd, UINT32 *mac_addr)
{
	UINT32 mac_addr_hif = *mac_addr;
	INT idx = 0;
	BOOLEAN IsFound = 0;
	UINT32 *mac_cr_range = NULL;
#ifdef MT7663
	if (IS_MT7663(pAd))
		mac_cr_range = &mt7663_mac_cr_range[0];
#endif /* MT7663 */

	if (!mac_cr_range) {
		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): NotSupported Chip for this function!\n", __func__));
		return IsFound;
	}

	if (mac_addr_hif >= 0x40000) {
		do {
			if (mac_addr_hif >= mac_cr_range[idx] &&
				mac_addr_hif < (mac_cr_range[idx] + mac_cr_range[idx + 2])) {
				mac_addr_hif -= mac_cr_range[idx];
				mac_addr_hif += mac_cr_range[idx + 1];
				IsFound = 1;
				break;
			}

			idx += 3;
		} while (mac_cr_range[idx] != 0);
	} else
		IsFound = 1;

	*mac_addr = mac_addr_hif;
	return IsFound;
}


UINT32 mt_physical_addr_map(RTMP_ADAPTER *pAd, UINT32 addr)
{
	UINT32 global_addr = 0x0, idx = 1;
	struct _RTMP_CHIP_CAP *cap = hc_get_chip_cap(pAd->hdev_ctrl);
	UINT32 wtbl_2_base = cap->WtblPseAddr;

	if (addr < 0x2000)
		global_addr = 0x80020000 + addr;
	else if ((addr >= 0x2000) && (addr < 0x4000))
		global_addr = 0x80000000 + addr - 0x2000;
	else if ((addr >= 0x4000) && (addr < 0x8000))
		global_addr = 0x50000000 + addr - 0x4000;
	else if ((addr >= 0x8000) && (addr < 0x10000))
		global_addr = 0xa0000000 + addr - 0x8000;
	else if ((addr >= 0x10000) && (addr < 0x20000))
		global_addr = 0x60200000 + addr - 0x10000;
	else if ((addr >= 0x20000) && (addr < 0x40000)) {
		UINT32 *mac_cr_range = NULL;
#ifdef MT7663

		if (IS_MT7663(pAd))
			mac_cr_range = &mt7663_mac_cr_range[0];

#endif /* MT7663 */

		if (!mac_cr_range) {
			MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s(): NotSupported Chip for this function!\n", __func__));
			return global_addr;
		}

		do {
			if ((addr >= mac_cr_range[idx]) && (addr < (mac_cr_range[idx] + mac_cr_range[idx + 1]))) {
				global_addr = mac_cr_range[idx - 1] + (addr - mac_cr_range[idx]);
				break;
			}

			idx += 3;
		} while (mac_cr_range[idx] != 0);

		if (mac_cr_range[idx] == 0)
			MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("unknow addr range = %x\n", addr));
	} else if ((addr >= 0x40000) && (addr < 0x80000)) { /* WTBL Address */
		global_addr = wtbl_2_base + addr - 0x40000;
		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("==>global_addr1=0x%x\n", global_addr));
	} else if ((addr >= 0xc0000) && (addr < 0xc0100)) { /* PSE Client */
		global_addr = 0x800c0000 + addr - 0xc0000;
		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("==>global_addr2=0x%x\n", global_addr));
	} else {
		global_addr = addr;
		MTWF_LOG(DBG_CAT_HIF, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("==>global_addr3=0x%x\n", global_addr));
	}

	return global_addr;
}


/*export io func.*/
VOID hif_io_force_read32(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->hif_io_forec_read32)
		ops->hif_io_forec_read32(hdev_ctrl, reg, val);
}

VOID hif_io_force_write32(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->hif_io_forec_write32)
		ops->hif_io_forec_write32(hdev_ctrl, reg, val);
}

VOID hif_io_read32(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->hif_io_read32)
		ops->hif_io_read32(hdev_ctrl, reg, val);
}

VOID hif_io_write32(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->hif_io_write32)
		ops->hif_io_write32(hdev_ctrl, reg, val);
}

VOID mac_io_read16(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mac_io_read16)
		ops->mac_io_read16(hdev_ctrl, reg, val);
}

VOID mac_io_write16(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mac_io_write16)
		ops->mac_io_write16(hdev_ctrl, reg, val);
}

VOID mac_io_read8(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mac_io_read8)
		ops->mac_io_read8(hdev_ctrl, reg, val);
}

VOID mac_io_write8(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mac_io_write8)
		ops->mac_io_write8(hdev_ctrl, reg, val);
}

VOID sys_io_read32(ULONG reg, UINT32 *val)
{
	*val = readl((void *) reg);
}

VOID sys_io_write32(ULONG reg, UINT32 val)
{
	writel(val, (void *) reg);
}

VOID mac_io_read32(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mac_io_read32)
		ops->mac_io_read32(hdev_ctrl, reg, val);
}

VOID mac_io_write32(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mac_io_write32)
		ops->mac_io_write32(hdev_ctrl, reg, val);
}

VOID phy_io_read32(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->phy_io_read32)
		ops->phy_io_read32(hdev_ctrl, reg, val);
}

VOID phy_io_write32(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->phy_io_write32)
		ops->phy_io_write32(hdev_ctrl, reg, val);
}

VOID mcu_io_read32(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mcu_io_read32)
		ops->mcu_io_read32(hdev_ctrl, reg, val);
}

VOID mcu_io_write32(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->mcu_io_write32)
		ops->mcu_io_write32(hdev_ctrl, reg, val);
}

VOID hw_io_read32(void *hdev_ctrl, UINT32 reg, UINT32 *val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->hw_io_read32)
		ops->hw_io_read32(hdev_ctrl, reg, val);
}

VOID hw_io_write32(void *hdev_ctrl, UINT32 reg, UINT32 val)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	if (ops->hw_io_write32)
		ops->hw_io_write32(hdev_ctrl, reg, val);
}

static VOID hif_io_ops_init(void *hdev_ctrl, INT infType)
{
#ifdef RTMP_MAC_PCI
	if (infType == RTMP_DEV_INF_PCIE || infType == RTMP_DEV_INF_PCI || infType == RTMP_DEV_INF_RBUS)
		pci_io_ops_init(hdev_ctrl);
#endif


}

VOID mt_io_ops_init(void *hdev_ctrl, INT hifType)
{
	struct mt_io_ops *ops = hc_get_io_ops(hdev_ctrl);

	/*hif ops*/
	hif_io_ops_init(hdev_ctrl, hifType);

	/*common ops*/
	ops->mcu_io_read32 = CmdIORead32;
	ops->mcu_io_write32 = CmdIOWrite32;
}

