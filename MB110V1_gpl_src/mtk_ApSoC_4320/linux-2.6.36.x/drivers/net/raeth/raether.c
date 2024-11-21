#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/rt2880/surfboardint.h>
#if defined (CONFIG_RAETH_TSO)
#include <linux/tcp.h>
#include <net/ipv6.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/in.h>
#include <linux/ppp_defs.h>
#include <linux/if_pppox.h>
#endif
#if defined (CONFIG_RAETH_LRO)
#include <linux/inet_lro.h>
#endif
#include <linux/delay.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
#include <linux/sched.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
#include <asm/rt2880/rt_mmap.h>
#else
#include <linux/libata-compat.h>
#endif
 
#include "ra2882ethreg.h"
#include "raether.h"
#include "ra_mac.h"
#include "ra_ioctl.h"
#include "ra_rfrw.h"
#ifdef CONFIG_RAETH_NETLINK
#include "ra_netlink.h"
#endif
#if defined (CONFIG_RAETH_QOS)
#include "ra_qos.h"
#endif

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
#include "../../../net/nat/hw_nat/ra_nat.h"
#endif

#ifdef CONFIG_TP_SUPER_DMZ
#include "../../../net/nat/hw_nat/ra_nat.h"
#include <net/ip.h>
#if 0
#define LQ_DEBUG(fmt, args...) printk("[%s:%d]"fmt"\n", __FUNCTION__,__LINE__,##args)
#else
#define LQ_DEBUG(fmt, args...)
#endif
#endif


#if defined (TASKLET_WORKQUEUE_SW)
int init_schedule;
int working_schedule;
#endif

#ifdef CONFIG_RAETH_NAPI
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
static int raeth_clean(struct napi_struct *napi, int budget);
#else
static int raeth_clean(struct net_device *dev, int *budget);
#endif

static int rt2880_eth_recv(struct net_device* dev, int *work_done, int work_to_do);
#else
static int rt2880_eth_recv(struct net_device* dev);
#endif

#if !defined(CONFIG_RA_NAT_NONE)
/* bruce+
 */
extern int (*ra_sw_nat_hook_rx)(struct sk_buff *skb);
extern int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no);
#endif

#if defined(CONFIG_RA_CLASSIFIER)||defined(CONFIG_RA_CLASSIFIER_MODULE)
/* Qwert+
 */
#include <asm/mipsregs.h>
extern int (*ra_classifier_hook_tx)(struct sk_buff *skb, unsigned long cur_cycle);
extern int (*ra_classifier_hook_rx)(struct sk_buff *skb, unsigned long cur_cycle);
#endif /* CONFIG_RA_CLASSIFIER */

#ifdef CONFIG_MULTICAST_VLAN
extern int (*multicastVlanRxHook)(struct sk_buff *skb);
int multicastVlanSet(struct ifreq *ifr);
#endif

#if defined (CONFIG_RALINK_RT3052_MP2)
int32_t mcast_rx(struct sk_buff * skb);
int32_t mcast_tx(struct sk_buff * skb);
#endif

#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
int ra_mtd_read_nm(char *name, loff_t from, size_t len, u_char *buf) 
{
	/* TODO */
	return 0;
}
#else
#ifdef RA_MTD_RW_BY_NUM
int ra_mtd_read(int num, loff_t from, size_t len, u_char *buf);
#else
int ra_mtd_read_nm(char *name, loff_t from, size_t len, u_char *buf);
#endif
#endif

#if defined (CONFIG_RALINK_MT7621) || defined (CONFIG_P5_RGMII_TO_MT7530_MODE)
void setup_internal_gsw(void);
#if defined (CONFIG_GE1_TRGMII_FORCE_1200)
void apll_xtal_enable(void);
#define REGBIT(x, n)              (x << n)
#endif
#endif

/* gmac driver feature set config */
#if defined (CONFIG_RAETH_NAPI) || defined (CONFIG_RAETH_QOS)
#undef DELAY_INT
#else
#define DELAY_INT	1
#endif

//#define CONFIG_UNH_TEST
/* end of config */

#if defined (CONFIG_RAETH_JUMBOFRAME)
#define	MAX_RX_LENGTH	4096
#else
#define	MAX_RX_LENGTH	1536
#endif

struct net_device		*dev_raether;

static int rx_dma_owner_idx; 
static int rx_dma_owner_idx0;
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
static int rx_dma_owner_idx1;
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
int rx_calc_idx1;
#endif
#endif
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
int rx_calc_idx0;
#endif
static int pending_recv;
static struct PDMA_rxdesc	*rx_ring;
unsigned long tx_ring_full=0;

#if defined(CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
    defined(CONFIG_RALINK_MT7620)
unsigned short p0_rx_good_cnt = 0;
unsigned short p1_rx_good_cnt = 0;
unsigned short p2_rx_good_cnt = 0;
unsigned short p3_rx_good_cnt = 0;
unsigned short p4_rx_good_cnt = 0;
unsigned short p5_rx_good_cnt = 0;
unsigned short p6_rx_good_cnt = 0;
unsigned short p0_tx_good_cnt = 0;
unsigned short p1_tx_good_cnt = 0;
unsigned short p2_tx_good_cnt = 0;
unsigned short p3_tx_good_cnt = 0;
unsigned short p4_tx_good_cnt = 0;
unsigned short p5_tx_good_cnt = 0;
unsigned short p6_tx_good_cnt = 0;

unsigned short p0_rx_byte_cnt = 0;
unsigned short p1_rx_byte_cnt = 0;
unsigned short p2_rx_byte_cnt = 0;
unsigned short p3_rx_byte_cnt = 0;
unsigned short p4_rx_byte_cnt = 0;
unsigned short p5_rx_byte_cnt = 0;
unsigned short p6_rx_byte_cnt = 0;
unsigned short p0_tx_byte_cnt = 0;
unsigned short p1_tx_byte_cnt = 0;
unsigned short p2_tx_byte_cnt = 0;
unsigned short p3_tx_byte_cnt = 0;
unsigned short p4_tx_byte_cnt = 0;
unsigned short p5_tx_byte_cnt = 0;
unsigned short p6_tx_byte_cnt = 0;

#if defined(CONFIG_RALINK_MT7620)
unsigned short p7_rx_good_cnt = 0;
unsigned short p7_tx_good_cnt = 0;

unsigned short p7_rx_byte_cnt = 0;
unsigned short p7_tx_byte_cnt = 0;
#endif
#endif




#if defined (CONFIG_ETHTOOL) /*&& defined (CONFIG_RAETH_ROUTER)*/
#include "ra_ethtool.h"
extern struct ethtool_ops	ra_ethtool_ops;
#ifdef CONFIG_PSEUDO_SUPPORT
extern struct ethtool_ops	ra_virt_ethtool_ops;
#endif // CONFIG_PSEUDO_SUPPORT //
#endif // (CONFIG_ETHTOOL //

#ifdef CONFIG_RALINK_VISTA_BASIC
int is_switch_175c = 1;
#endif

unsigned int M2Q_table[64] = {0};
EXPORT_SYMBOL(M2Q_table);
#if defined (CONFIG_RAETH_LRO)
unsigned int lan_ip;
struct lro_para_struct lro_para; 
int lro_flush_needed;
extern char const *nvram_get(int index, char *name);
#endif

#define KSEG1                   0xa0000000
#define PHYS_TO_VIRT(x)         ((void *)((x) | KSEG1))
#define VIRT_TO_PHYS(x)         ((unsigned long)(x) & ~KSEG1)

#if defined (CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
#define		TP_MACADDR_OFFSET	0x7EF100
#elif defined (CONFIG_TP_10M_FLASH)
#define		TP_MACADDR_OFFSET	0x9EF100
#elif defined (CONFIG_TP_MODEL_MR400V4)
#define		TP_MACADDR_OFFSET	0xFCF100
#elif defined (CONFIG_TP_4M_FLASH)
#define		TP_MACADDR_OFFSET	0x3FF900
#else
#define		TP_MACADDR_OFFSET	0x7DF100
#endif

extern int fe_dma_init(struct net_device *dev);
extern int ei_start_xmit(struct sk_buff* skb, struct net_device *dev, int gmac_no);
extern void ei_xmit_housekeeping(unsigned long unused);
extern inline int rt2880_eth_send(struct net_device* dev, struct sk_buff *skb, int gmac_no);

#if 0 
void skb_dump(struct sk_buff* sk) {
        unsigned int i;

        printk("skb_dump: from %s with len %d (%d) headroom=%d tailroom=%d\n",
                sk->dev?sk->dev->name:"ip stack",sk->len,sk->truesize,
                skb_headroom(sk),skb_tailroom(sk));

        //for(i=(unsigned int)sk->head;i<=(unsigned int)sk->tail;i++) {
        for(i=(unsigned int)sk->head;i<=(unsigned int)sk->data+20;i++) {
                if((i % 20) == 0)
                        printk("\n");
                if(i==(unsigned int)sk->data) printk("{");
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,21)
                if(i==(unsigned int)sk->transport_header) printk("#");
                if(i==(unsigned int)sk->network_header) printk("|");
                if(i==(unsigned int)sk->mac_header) printk("*");
#else
                if(i==(unsigned int)sk->h.raw) printk("#");
                if(i==(unsigned int)sk->nh.raw) printk("|");
                if(i==(unsigned int)sk->mac.raw) printk("*");
#endif
                printk("%02X-",*((unsigned char*)i));
                if(i==(unsigned int)sk->tail) printk("}");
        }
        printk("\n");
}
#endif

#ifdef CONFIG_TP_SUPER_DMZ
struct K_SDMZ_OBJ
{
	unsigned char enable;
	unsigned char mac[6];
	unsigned long fakeIP;
	unsigned long hostIP;
	unsigned long lanIP;
	unsigned long mask;
};
#endif


#if defined (CONFIG_GIGAPHY) || defined (CONFIG_P5_MAC_TO_PHY_MODE) ||  defined (CONFIG_P4_MAC_TO_PHY_MODE)
int isICPlusGigaPHY(int ge)
{
	u32 phy_id0 = 0, phy_id1 = 0;

#if defined (CONFIG_GE2_RGMII_AN) || defined (CONFIG_P4_MAC_TO_PHY_MODE)
	if (ge == 2) {
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 2, &phy_id0)) {
			printk("\n Read PhyID 1 is Fail!!\n");
			phy_id0 =0;
		}
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 3, &phy_id1)) {
			printk("\n Read PhyID 1 is Fail!!\n");
			phy_id1 = 0;
		}
	}
	else
#endif
#if defined (CONFIG_GE1_RGMII_AN) || defined (CONFIG_P5_MAC_TO_PHY_MODE)
	{
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 2, &phy_id0)) {
			printk("\n Read PhyID 0 is Fail!!\n");
			phy_id0 =0;
		}
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 3, &phy_id1)) {
			printk("\n Read PhyID 0 is Fail!!\n");
			phy_id1 = 0;
		}
	}
#endif

	if ((phy_id0 == EV_ICPLUS_PHY_ID0) && ((phy_id1 & 0xfff0) == EV_ICPLUS_PHY_ID1))
		return 1;
	return 0;
}


int isMarvellGigaPHY(int ge)
{
	u32 phy_id0 = 0, phy_id1 = 0;

#if defined (CONFIG_GE2_RGMII_AN) || defined (CONFIG_P4_MAC_TO_PHY_MODE)
	if (ge == 2) {
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 2, &phy_id0)) {
			printk("\n Read PhyID 1 is Fail!!\n");
			phy_id0 =0;
		}
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 3, &phy_id1)) {
			printk("\n Read PhyID 1 is Fail!!\n");
			phy_id1 = 0;
		}
	}
	else
#endif
#if defined (CONFIG_GE1_RGMII_AN) || defined (CONFIG_P5_MAC_TO_PHY_MODE)
	{
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 2, &phy_id0)) {
			printk("\n Read PhyID 0 is Fail!!\n");
			phy_id0 =0;
		}
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 3, &phy_id1)) {
			printk("\n Read PhyID 0 is Fail!!\n");
			phy_id1 = 0;
		}
	}
#endif
		;
	if ((phy_id0 == EV_MARVELL_PHY_ID0) && (phy_id1 == EV_MARVELL_PHY_ID1))
		return 1;
	return 0;
}

int isVtssGigaPHY(int ge)
{
	u32 phy_id0 = 0, phy_id1 = 0;

#if defined (CONFIG_GE2_RGMII_AN) || defined (CONFIG_P4_MAC_TO_PHY_MODE)
	if (ge == 2) {
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 2, &phy_id0)) {
			printk("\n Read PhyID 1 is Fail!!\n");
			phy_id0 =0;
		}
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 3, &phy_id1)) {
			printk("\n Read PhyID 1 is Fail!!\n");
			phy_id1 = 0;
		}
	}
	else
#endif
#if defined (CONFIG_GE1_RGMII_AN) || defined (CONFIG_P5_MAC_TO_PHY_MODE)
	{
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 2, &phy_id0)) {
			printk("\n Read PhyID 0 is Fail!!\n");
			phy_id0 =0;
		}
		if (!mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 3, &phy_id1)) {
			printk("\n Read PhyID 0 is Fail!!\n");
			phy_id1 = 0;
		}
	}
#endif
		;
	if ((phy_id0 == EV_VTSS_PHY_ID0) && (phy_id1 == EV_VTSS_PHY_ID1))
		return 1;
	return 0;
}
#endif

/*
 * Set the hardware MAC address.
 */
static int ei_set_mac_addr(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	if(netif_running(dev))
		return -EBUSY;

        ra2880MacAddressSet(addr->sa_data);
	return 0;
}

#ifdef CONFIG_PSEUDO_SUPPORT
static int ei_set_mac2_addr(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	if(netif_running(dev))
		return -EBUSY;

        ra2880Mac2AddressSet(addr->sa_data);
	return 0;
}
#endif

void set_fe_dma_glo_cfg(void)
{
        int dma_glo_cfg=0;
#if defined (CONFIG_RALINK_RT2880) || defined(CONFIG_RALINK_RT2883) || \
    defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT3883)
        int fe_glo_cfg=0;
#endif

#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) 
	dma_glo_cfg = (TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN | PDMA_BT_SIZE_32DWORDS);
#elif defined (CONFIG_RALINK_MT7620) || defined (CONFIG_RALINK_MT7621)
	dma_glo_cfg = (TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN | PDMA_BT_SIZE_16DWORDS);
#else 
	dma_glo_cfg = (TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN | PDMA_BT_SIZE_4DWORDS);
#endif

#if defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
	dma_glo_cfg |= (RX_2B_OFFSET);
#endif

#if defined (CONFIG_32B_DESC)
	dma_glo_cfg |= (DESC_32B_EN);
#endif
	sysRegWrite(DMA_GLO_CFG, dma_glo_cfg);
#ifdef CONFIG_RAETH_QDMA	
	sysRegWrite(QDMA_GLO_CFG, dma_glo_cfg);
#endif

	/* only the following chipset need to set it */
#if defined (CONFIG_RALINK_RT2880) || defined(CONFIG_RALINK_RT2883) || \
    defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT3883)
	//set 1us timer count in unit of clock cycle
	fe_glo_cfg = sysRegRead(FE_GLO_CFG);
	fe_glo_cfg &= ~(0xff << 8); //clear bit8-bit15
	fe_glo_cfg |= (((get_surfboard_sysclk()/1000000)) << 8);
	sysRegWrite(FE_GLO_CFG, fe_glo_cfg);
#endif
}

int forward_config(struct net_device *dev)
{
	
#if defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_MT7628)

	/* RT5350: No GDMA, PSE, CDMA, PPE */
	unsigned int sdmVal;
	sdmVal = sysRegRead(SDM_CON);

#ifdef CONFIG_RAETH_CHECKSUM_OFFLOAD
	sdmVal |= 0x7<<16; // UDPCS, TCPCS, IPCS=1
#endif // CONFIG_RAETH_CHECKSUM_OFFLOAD //

#if defined (CONFIG_RAETH_SPECIAL_TAG)
	sdmVal |= 0x1<<20; // TCI_81XX
#endif // CONFIG_RAETH_SPECIAL_TAG //

	sysRegWrite(SDM_CON, sdmVal);

#else //Non RT5350 chipset

	unsigned int	regVal, regCsg;

#ifdef CONFIG_PSEUDO_SUPPORT
	unsigned int	regVal2;
#endif

#ifdef CONFIG_RAETH_HW_VLAN_TX
#if defined(CONFIG_RALINK_MT7620)
	/* frame engine will push VLAN tag regarding to VIDX feild in Tx desc. */
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x430) = 0x00010000;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x434) = 0x00030002;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x438) = 0x00050004;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x43C) = 0x00070006;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x440) = 0x00090008;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x444) = 0x000b000a;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x448) = 0x000d000c;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0x44C) = 0x000f000e;
#else
	/* 
	 * VLAN_IDX 0 = VLAN_ID 0
	 * .........
	 * VLAN_IDX 15 = VLAN ID 15
	 *
	 */
	/* frame engine will push VLAN tag regarding to VIDX feild in Tx desc. */
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xa8) = 0x00010000;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xac) = 0x00030002;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xb0) = 0x00050004;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xb4) = 0x00070006;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xb8) = 0x00090008;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xbc) = 0x000b000a;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xc0) = 0x000d000c;
	*(unsigned long *)(RALINK_FRAME_ENGINE_BASE + 0xc4) = 0x000f000e;
#endif
#endif

	regVal = sysRegRead(GDMA1_FWD_CFG);
	regCsg = sysRegRead(CDMA_CSG_CFG);

#ifdef CONFIG_PSEUDO_SUPPORT
	regVal2 = sysRegRead(GDMA2_FWD_CFG);
#endif

	//set unicast/multicast/broadcast frame to cpu
#if defined (CONFIG_RALINK_MT7620)
	/* GDMA1 frames destination port is port0 CPU*/
	regVal &= ~0x7;
#else
	regVal &= ~0xFFFF;
	regVal |= GDMA1_FWD_PORT;
#endif
	regCsg &= ~0x7;

#if defined (CONFIG_RAETH_SPECIAL_TAG)
	regVal |= (1 << 24); //GDM1_TCI_81xx
#endif


#ifdef CONFIG_RAETH_HW_VLAN_TX
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	dev->features |= NETIF_F_HW_VLAN_TX;
#else
	dev->features |= NETIF_F_HW_VLAN_CTAG_TX;
#endif
#endif
#ifdef CONFIG_RAETH_HW_VLAN_RX
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	dev->features |= NETIF_F_HW_VLAN_RX;
#else
	dev->features |= NETIF_F_HW_VLAN_CTAG_RX;
#endif
#endif

#ifdef CONFIG_RAETH_CHECKSUM_OFFLOAD
	//enable ipv4 header checksum check
	regVal |= GDM1_ICS_EN;
	regCsg |= ICS_GEN_EN;

	//enable tcp checksum check
	regVal |= GDM1_TCS_EN;
	regCsg |= TCS_GEN_EN;

	//enable udp checksum check
	regVal |= GDM1_UCS_EN;
	regCsg |= UCS_GEN_EN;

#ifdef CONFIG_PSEUDO_SUPPORT
	regVal2 &= ~0xFFFF;
	regVal2 |= GDMA2_FWD_PORT;
  
	regVal2 |= GDM1_ICS_EN;
	regVal2 |= GDM1_TCS_EN;
	regVal2 |= GDM1_UCS_EN;
#endif

	dev->features |= NETIF_F_IP_CSUM; /* Can checksum TCP/UDP over IPv4 */
//#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
//	dev->vlan_features |= NETIF_F_IP_CSUM;
//#endif

#if defined(CONFIG_RALINK_MT7620)
#if defined (CONFIG_RAETH_TSO)
	if ((sysRegRead(0xB000000C) & 0xf) >= 0x5) {
		dev->features |= NETIF_F_SG;
		dev->features |= NETIF_F_TSO;
	}
#endif // CONFIG_RAETH_TSO //

#if defined (CONFIG_RAETH_TSOV6)
	if ((sysRegRead(0xB000000C) & 0xf) >= 0x5) {
		dev->features |= NETIF_F_TSO6;
		dev->features |= NETIF_F_IPV6_CSUM; /* Can checksum TCP/UDP over IPv6 */
	}
#endif // CONFIG_RAETH_TSOV6 //
#else
#if defined (CONFIG_RAETH_TSO)
	dev->features |= NETIF_F_SG;
	dev->features |= NETIF_F_TSO;
#endif // CONFIG_RAETH_TSO //

#if defined (CONFIG_RAETH_TSOV6)
	dev->features |= NETIF_F_TSO6;
	dev->features |= NETIF_F_IPV6_CSUM; /* Can checksum TCP/UDP over IPv6 */
#endif // CONFIG_RAETH_TSOV6 //
#endif // CONFIG_RALINK_MT7620 //
#else // Checksum offload disabled

	//disable ipv4 header checksum check
	regVal &= ~GDM1_ICS_EN;
	regCsg &= ~ICS_GEN_EN;

	//disable tcp checksum check
	regVal &= ~GDM1_TCS_EN;
	regCsg &= ~TCS_GEN_EN;

	//disable udp checksum check
	regVal &= ~GDM1_UCS_EN;
	regCsg &= ~UCS_GEN_EN;

#ifdef CONFIG_PSEUDO_SUPPORT
	regVal2 &= ~GDM1_ICS_EN;
	regVal2 &= ~GDM1_TCS_EN;
	regVal2 &= ~GDM1_UCS_EN;
#endif

	dev->features &= ~NETIF_F_IP_CSUM; /* disable checksum TCP/UDP over IPv4 */
#endif // CONFIG_RAETH_CHECKSUM_OFFLOAD //

#ifdef CONFIG_RAETH_JUMBOFRAME
	regVal |= GDM1_JMB_EN;
#ifdef CONFIG_PSEUDO_SUPPORT
	regVal2 |= GDM1_JMB_EN;
#endif
#endif

	sysRegWrite(GDMA1_FWD_CFG, regVal);
	sysRegWrite(CDMA_CSG_CFG, regCsg);
#ifdef CONFIG_PSEUDO_SUPPORT
	sysRegWrite(GDMA2_FWD_CFG, regVal2);
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
        dev->vlan_features = dev->features;
#endif

/*
 * 	PSE_FQ_CFG register definition -
 *
 * 	Define max free queue page count in PSE. (31:24)
 *	RT2883/RT3883 - 0xff908000 (255 pages)
 *	RT3052 - 0x80504000 (128 pages)
 *	RT2880 - 0x80504000 (128 pages)
 *
 * 	In each page, there are 128 bytes in each page.
 *
 *	23:16 - free queue flow control release threshold
 *	15:8  - free queue flow control assertion threshold
 *	7:0   - free queue empty threshold
 *
 *	The register affects QOS correctness in frame engine!
 */

#if defined(CONFIG_RALINK_RT2883) || defined(CONFIG_RALINK_RT3883)
	sysRegWrite(PSE_FQ_CFG, cpu_to_le32(INIT_VALUE_OF_RT2883_PSE_FQ_CFG));
#elif defined(CONFIG_RALINK_RT3352) || defined(CONFIG_RALINK_RT5350) ||  \
      defined(CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
      defined(CONFIG_RALINK_MT7620) || defined(CONFIG_RALINK_MT7621) || \
      defined (CONFIG_RALINK_MT7628)
        /*use default value*/
#else
	sysRegWrite(PSE_FQ_CFG, cpu_to_le32(INIT_VALUE_OF_PSE_FQFC_CFG));
#endif

	/*
	 *FE_RST_GLO register definition -
	 *Bit 0: PSE Rest
	 *Reset PSE after re-programming PSE_FQ_CFG.
	 */
	regVal = 0x1;
	sysRegWrite(FE_RST_GL, regVal);
	sysRegWrite(FE_RST_GL, 0);	// update for RSTCTL issue

	regCsg = sysRegRead(CDMA_CSG_CFG);
	printk("CDMA_CSG_CFG = %0X\n",regCsg);
	regVal = sysRegRead(GDMA1_FWD_CFG);
	printk("GDMA1_FWD_CFG = %0X\n",regVal);

#ifdef CONFIG_PSEUDO_SUPPORT
	regVal = sysRegRead(GDMA2_FWD_CFG);
	printk("GDMA2_FWD_CFG = %0X\n",regVal);
#endif
#endif
	return 1;
}

#ifdef CONFIG_RAETH_LRO
static int
rt_get_skb_header(struct sk_buff *skb, void **iphdr, void **tcph,
                       u64 *hdr_flags, void *priv)
{
        struct iphdr *iph = NULL;
	int    vhdr_len = 0;

        /*
         * Make sure that this packet is Ethernet II, is not VLAN
         * tagged, is IPv4, has a valid IP header, and is TCP.
         */
	if (skb->protocol == 0x0081) {
		vhdr_len = VLAN_HLEN;
	}

	iph = (struct iphdr *)(skb->data + vhdr_len);
	if (iph->daddr != lro_para.lan_ip1) {
		return -1;
	}

	if(iph->protocol != IPPROTO_TCP) {
		return -1;
	} else {
		*iphdr = iph;
		*tcph = skb->data + (iph->ihl << 2) + vhdr_len;
		*hdr_flags = LRO_IPV4 | LRO_TCP;

		lro_flush_needed = 1;
		return 0;
	}
}
#endif // CONFIG_RAETH_LRO //

#ifdef CONFIG_TP_SUPER_DMZ
unsigned short csum_update(unsigned short old_csum,
		unsigned short old_field,
		unsigned short new_field)
{
	unsigned long csum = old_csum + old_field + (~new_field & 0xFFFF);
	csum = (csum >> 16) + (csum & 0xFFFF);
	csum += (csum >> 16);
	return csum;
}

unsigned short csum_update_ip(unsigned short old_check,
		unsigned long old_ip,
		unsigned long new_ip)
{
	unsigned short tmp = old_check;
	tmp = csum_update(tmp,
			(unsigned short)(old_ip >> 16),
			(unsigned short)(new_ip >> 16));
	tmp = csum_update(tmp,
			(unsigned short)(old_ip << 16 >> 16),
			(unsigned short)(new_ip << 16 >> 16));
	return tmp;
}
#endif

#ifdef CONFIG_RAETH_NAPI
static int rt2880_eth_recv(struct net_device* dev, int *work_done, int work_to_do)
#else
static int rt2880_eth_recv(struct net_device* dev)
#endif
{
	struct sk_buff	*skb, *rx_skb;
	unsigned int	length = 0;
	unsigned long	RxProcessed;

	int bReschedule = 0;
	END_DEVICE* 	ei_local = netdev_priv(dev);
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
	int rx_ring_no=0;
#endif

#if defined (CONFIG_RAETH_SPECIAL_TAG)
	struct vlan_ethhdr *veth=NULL;
#endif

#ifdef CONFIG_PSEUDO_SUPPORT
	PSEUDO_ADAPTER *pAd;
#endif

	RxProcessed = 0;
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	rx_dma_owner_idx0 = (rx_calc_idx0 + 1) % NUM_RX_DESC;
#else
	rx_dma_owner_idx0 = (sysRegRead(RAETH_RX_CALC_IDX0) + 1) % NUM_RX_DESC;
#endif

#if defined (CONFIG_32B_DESC)
#if defined (CONFIG_MIPS)
	dma_cache_sync(NULL, &ei_local->rx_ring0[rx_dma_owner_idx0], sizeof(struct PDMA_rxdesc), DMA_FROM_DEVICE);
#else
	dma_sync_single_for_cpu(NULL, &ei_local->rx_ring0[rx_dma_owner_idx0], sizeof(struct PDMA_rxdesc), DMA_FROM_DEVICE);
#endif
#endif
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
	rx_dma_owner_idx1 = (rx_calc_idx1 + 1) % NUM_RX_DESC;
#else
	rx_dma_owner_idx1 = (sysRegRead(RX_CALC_IDX1) + 1) % NUM_RX_DESC;
#endif
#if defined (CONFIG_32B_DESC)
#if defined (CONFIG_MIPS)
	dma_cache_sync(NULL, &ei_local->rx_ring1[rx_dma_owner_idx1], sizeof(struct PDMA_rxdesc), DMA_FROM_DEVICE);
#else
	dma_sync_single_for_cpu(NULL, &ei_local->rx_ring0[rx_dma_owner_idx0], sizeof(struct PDMA_rxdesc), DMA_FROM_DEVICE);
#endif
#endif
#endif
	for ( ; ; ) {

#ifdef CONFIG_RAETH_NAPI
                if(*work_done >= work_to_do)
                        break;
                (*work_done)++;
#else
		if (RxProcessed++ > NUM_RX_MAX_PROCESS)
                {
                        // need to reschedule rx handle
                        bReschedule = 1;
                        break;
                }
#endif


#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
		if (ei_local->rx_ring1[rx_dma_owner_idx1].rxd_info2.DDONE_bit == 1)  {
		    rx_ring = ei_local->rx_ring1;
		    rx_dma_owner_idx = rx_dma_owner_idx1;
		//    printk("rx_dma_owner_idx1=%x\n",rx_dma_owner_idx1);
		    rx_ring_no=1;
		} else if (ei_local->rx_ring0[rx_dma_owner_idx0].rxd_info2.DDONE_bit == 1)  {
		    rx_ring = ei_local->rx_ring0;
		    rx_dma_owner_idx = rx_dma_owner_idx0;
		 //   printk("rx_dma_owner_idx0=%x\n",rx_dma_owner_idx0);
		    rx_ring_no=0;
		} else {
		    break;
		}
#else

		if (ei_local->rx_ring0[rx_dma_owner_idx0].rxd_info2.DDONE_bit == 1)  {
		    rx_ring = ei_local->rx_ring0;
		    rx_dma_owner_idx = rx_dma_owner_idx0;
		} else {
		    break;
		}
#endif

#if defined (CONFIG_32B_DESC)
		prefetch(&rx_ring[(rx_dma_owner_idx + 1) % NUM_RX_DESC]);
#endif
		/* skb processing */
		length = rx_ring[rx_dma_owner_idx].rxd_info2.PLEN0;
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
		if(rx_ring_no==1) {
		    rx_skb = ei_local->netrx1_skbuf[rx_dma_owner_idx];
		    rx_skb->data = ei_local->netrx1_skbuf[rx_dma_owner_idx]->data;
		} else {
		    rx_skb = ei_local->netrx0_skbuf[rx_dma_owner_idx];
		    rx_skb->data = ei_local->netrx0_skbuf[rx_dma_owner_idx]->data;
		}
#else
		rx_skb = ei_local->netrx0_skbuf[rx_dma_owner_idx];
		rx_skb->data = ei_local->netrx0_skbuf[rx_dma_owner_idx]->data;
#endif
		rx_skb->len 	= length;

#if defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		rx_skb->data += NET_IP_ALIGN;
#endif

		rx_skb->tail 	= rx_skb->data + length;

#ifdef CONFIG_PSEUDO_SUPPORT
		if(rx_ring[rx_dma_owner_idx].rxd_info4.SP == 2) {
		    if(ei_local->PseudoDev!=NULL) {
			rx_skb->dev 	  = ei_local->PseudoDev;
			rx_skb->protocol  = eth_type_trans(rx_skb,ei_local->PseudoDev);
		    }else {
			printk("ERROR: PseudoDev is still not initialize but receive packet from GMAC2\n");
		    }
		}else{
		    rx_skb->dev 	  = dev;
		    rx_skb->protocol	  = eth_type_trans(rx_skb,dev);
		}
#else
		rx_skb->dev 	  = dev;
		rx_skb->protocol  = eth_type_trans(rx_skb,dev);
#endif

#ifdef CONFIG_RAETH_CHECKSUM_OFFLOAD
#if defined (CONFIG_PDMA_NEW)
		if(rx_ring[rx_dma_owner_idx].rxd_info4.L4VLD) {
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
		}else {
		    rx_skb->ip_summed = CHECKSUM_NONE;
		}
#else
		if(rx_ring[rx_dma_owner_idx].rxd_info4.IPFVLD_bit) {
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
		}else { 
		    rx_skb->ip_summed = CHECKSUM_NONE;
		}
#endif
#else
		    rx_skb->ip_summed = CHECKSUM_NONE;
#endif

#if defined(CONFIG_RA_CLASSIFIER)||defined(CONFIG_RA_CLASSIFIER_MODULE)
		/* Qwert+
		 */
		if(ra_classifier_hook_rx!= NULL)
		{
#if defined(CONFIG_RALINK_EXTERNAL_TIMER)
			ra_classifier_hook_rx(rx_skb, (*((volatile u32 *)(0xB0000D08))&0x0FFFF));
#else			
			ra_classifier_hook_rx(rx_skb, read_c0_count());
#endif			
		}
#endif /* CONFIG_RA_CLASSIFIER */

#if defined (CONFIG_RA_HW_NAT)  || defined (CONFIG_RA_HW_NAT_MODULE)
		if(ra_sw_nat_hook_rx != NULL) {
		    FOE_MAGIC_TAG(rx_skb)= FOE_MAGIC_GE;
		    *(uint32_t *)(FOE_INFO_START_ADDR(rx_skb)+2) = *(uint32_t *)&rx_ring[rx_dma_owner_idx].rxd_info4;
		    FOE_ALG(rx_skb) = 0;
		}
#endif

#ifdef CONFIG_TP_SUPER_DMZ
		extern struct K_SDMZ_OBJ kSdmzObj;
		struct vlan_ethhdr *vlan_eth;
		struct iphdr *iphs;
		struct udphdr *uhs;
		struct tcphdr *ths;

		LQ_DEBUG("kSdmzObj.enable %d \n", kSdmzObj.enable);
		if (kSdmzObj.enable && (FOE_AI(rx_skb) != HIT_BIND_FORCE_TO_CPU))
		{
			LQ_DEBUG(" FOE_AI(rx_skb): %d \n", FOE_AI(rx_skb));
			
			vlan_eth = vlan_eth_hdr(rx_skb);

			if(vlan_eth->h_vlan_encapsulated_proto == htons(ETH_P_IP))
			{
				rx_skb->network_header = rx_skb->mac_header + VLAN_ETH_HLEN;
				iphs = ip_hdr(rx_skb);
				rx_skb->transport_header = rx_skb->network_header + iphs->ihl * 4;

				LQ_DEBUG("iphs->saddr: %0x , kSdmzObj.hostIP: %0x ", iphs->saddr, kSdmzObj.hostIP);
				LQ_DEBUG("htonl(iphs->saddr): %0x , kSdmzObj.hostIP: %0x ", htons(iphs->saddr), kSdmzObj.hostIP);
				
				LQ_DEBUG("iphs->saddr: %0x , ntohl(kSdmzObj.hostIP): %0x ",iphs->saddr, ntohs(kSdmzObj.hostIP));
				
				if (iphs->saddr == kSdmzObj.hostIP)
				{
					iphs->check = csum_update_ip(iphs->check,
							kSdmzObj.hostIP,
							kSdmzObj.fakeIP);
					if ((iphs->frag_off & htons(IP_OFFSET)) == 0)
					{
						if (iphs->protocol == IPPROTO_TCP)
						{
							ths = tcp_hdr(rx_skb);
							ths->check = csum_update_ip(ths->check,
									kSdmzObj.hostIP,
									kSdmzObj.fakeIP);
						}
						else if (iphs->protocol == IPPROTO_UDP)
						{
							uhs = udp_hdr(rx_skb);
							uhs->check = csum_update_ip(uhs->check,
									kSdmzObj.hostIP,
									kSdmzObj.fakeIP);
						}
					}
					iphs->saddr = kSdmzObj.fakeIP;
				}
			}
		}
#endif
		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu*/
#if defined (CONFIG_RAETH_SKB_RECYCLE_2K)
                skb = skbmgr_dev_alloc_skb2k();
#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
        skb = alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN, GFP_ATOMIC);
#else
		skb = __dev_alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN, GFP_ATOMIC);
#endif
#endif

		if (unlikely(skb == NULL))
		{
			printk(KERN_ERR "skb not available...\n");
#ifdef CONFIG_PSEUDO_SUPPORT
			if (rx_ring[rx_dma_owner_idx].rxd_info4.SP == 2) {
				if (ei_local->PseudoDev != NULL) {
					pAd = netdev_priv(ei_local->PseudoDev);
					pAd->stat.rx_dropped++;
				}
			} else
#endif
				ei_local->stat.rx_dropped++;
                        bReschedule = 1;
			break;
		}
#if !defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		skb_reserve(skb, NET_IP_ALIGN);
#endif

#if defined (CONFIG_RAETH_SPECIAL_TAG)
		// port0: 0x8100 => 0x8100 0001
		// port1: 0x8101 => 0x8100 0002
		// port2: 0x8102 => 0x8100 0003
		// port3: 0x8103 => 0x8100 0004
		// port4: 0x8104 => 0x8100 0005
		// port5: 0x8105 => 0x8100 0006
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,21)
		veth = (struct vlan_ethhdr *)(rx_skb->mac_header);
#else
		veth = (struct vlan_ethhdr *)(rx_skb->mac.raw);
#endif
		/*donot check 0x81 due to MT7530 SPEC*/
		//if((veth->h_vlan_proto & 0xFF) == 0x81) 
		{
		    veth->h_vlan_TCI = htons( (((veth->h_vlan_proto >> 8) & 0xF) + 1) );
		    rx_skb->protocol = veth->h_vlan_proto = htons(ETH_P_8021Q);
		}
#endif

#ifdef CONFIG_MULTICAST_VLAN
        if (multicastVlanRxHook != NULL) {
            multicastVlanRxHook(rx_skb);
        }
#endif

/* ra_sw_nat_hook_rx return 1 --> continue
 * ra_sw_nat_hook_rx return 0 --> FWD & without netif_rx
 */
#if !defined(CONFIG_RA_NAT_NONE)
         if((ra_sw_nat_hook_rx == NULL) || 
	    (ra_sw_nat_hook_rx!= NULL && ra_sw_nat_hook_rx(rx_skb)))
#endif
         {
#if defined (CONFIG_RALINK_RT3052_MP2)
	       if(mcast_rx(rx_skb)==0) {
		   kfree_skb(rx_skb);
	       }else
#endif
#if defined (CONFIG_RAETH_LRO)
	       if (rx_skb->ip_summed == CHECKSUM_UNNECESSARY) {
		       lro_receive_skb(&ei_local->lro_mgr, rx_skb, NULL);
		       //LroStatsUpdate(&ei_local->lro_mgr,0);
	       } else
#endif
#ifdef CONFIG_RAETH_NAPI
                netif_receive_skb(rx_skb);
#else
#ifdef CONFIG_RAETH_HW_VLAN_RX
	        if(ei_local->vlgrp && rx_ring[rx_dma_owner_idx].rxd_info2.TAG) {
			vlan_hwaccel_rx(rx_skb, ei_local->vlgrp, rx_ring[rx_dma_owner_idx].rxd_info3.VID);
		} else {
			netif_rx(rx_skb);
		}
#else
#ifdef CONFIG_RAETH_CPU_LOOPBACK
                skb_push(rx_skb,ETH_HLEN);
                ei_start_xmit(rx_skb, dev, 1);
#else		
                netif_rx(rx_skb);
#endif
#endif
#endif
         }

#ifdef CONFIG_PSEUDO_SUPPORT
		if (rx_ring[rx_dma_owner_idx].rxd_info4.SP == 2) {
			if (ei_local->PseudoDev != NULL) {
				pAd = netdev_priv(ei_local->PseudoDev);
				pAd->stat.rx_packets++;
				pAd->stat.rx_bytes += length;
			}
		} else
#endif
		{
			ei_local->stat.rx_packets++;
			ei_local->stat.rx_bytes += length;
		}


#if defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		rx_ring[rx_dma_owner_idx].rxd_info2.PLEN0 = MAX_RX_LENGTH;
		rx_ring[rx_dma_owner_idx].rxd_info2.LS0 = 0;
#endif
		rx_ring[rx_dma_owner_idx].rxd_info2.DDONE_bit = 0;
		rx_ring[rx_dma_owner_idx].rxd_info1.PDP0 = dma_map_single(NULL, skb->data, MAX_RX_LENGTH, PCI_DMA_FROMDEVICE);
#ifdef CONFIG_32B_DESC
		dma_cache_sync(NULL, &rx_ring[rx_dma_owner_idx], sizeof(struct PDMA_rxdesc), DMA_TO_DEVICE);
#endif

		/*  Move point to next RXD which wants to alloc*/
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
		if(rx_ring_no==0) {
		    sysRegWrite(RAETH_RX_CALC_IDX0, rx_dma_owner_idx);
		    ei_local->netrx0_skbuf[rx_dma_owner_idx] = skb;
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
		    rx_calc_idx0 = rx_dma_owner_idx;
#endif
		}else {
		    sysRegWrite(RAETH_RX_CALC_IDX1, rx_dma_owner_idx);
		    ei_local->netrx1_skbuf[rx_dma_owner_idx] = skb;
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
		    rx_calc_idx1 = rx_dma_owner_idx;
#endif
		}
#else
		sysRegWrite(RAETH_RX_CALC_IDX0, rx_dma_owner_idx);
		ei_local->netrx0_skbuf[rx_dma_owner_idx] = skb;
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
		rx_calc_idx0 = rx_dma_owner_idx;
#endif
#endif

		
		/* Update to Next packet point that was received.
		 */
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
		if(rx_ring_no==0) {
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
			rx_dma_owner_idx0 = (rx_dma_owner_idx + 1) % NUM_RX_DESC;
#else
			rx_dma_owner_idx0 = (sysRegRead(RAETH_RX_CALC_IDX0) + 1) % NUM_RX_DESC;
#endif
		}else {
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
			rx_dma_owner_idx1 = (rx_dma_owner_idx + 1) % NUM_RX_DESC;
#else
			rx_dma_owner_idx1 = (sysRegRead(RAETH_RX_CALC_IDX1) + 1) % NUM_RX_DESC;
#endif
		}
#else
#ifdef CONFIG_RAETH_RW_PDMAPTR_FROM_VAR
		rx_dma_owner_idx0 = (rx_dma_owner_idx + 1) % NUM_RX_DESC;
#else
		rx_dma_owner_idx0 = (sysRegRead(RAETH_RX_CALC_IDX0) + 1) % NUM_RX_DESC;
#endif
#endif
	}	/* for */

#if defined (CONFIG_RAETH_LRO)
	if (lro_flush_needed) {
		//LroStatsUpdate(&ei_local->lro_mgr,1);
		lro_flush_all(&ei_local->lro_mgr);
		lro_flush_needed = 0;
	}
#endif
	return bReschedule;
}


///////////////////////////////////////////////////////////////////
/////
///// ra_get_stats - gather packet information for management plane
/////
///// Pass net_device_stats to the upper layer.
/////
/////
///// RETURNS: pointer to net_device_stats
///////////////////////////////////////////////////////////////////

struct net_device_stats *ra_get_stats(struct net_device *dev)
{
	END_DEVICE *ei_local = netdev_priv(dev);
	return &ei_local->stat;
}

#if defined (CONFIG_RT_3052_ESW)
void kill_sig_workq(struct work_struct *work)
{
	struct file *fp;
	char pid[8];
	struct task_struct *p = NULL;

	//read udhcpc pid from file, and send signal USR2,USR1 to get a new IP
	fp = filp_open("/var/run/udhcpc.pid", O_RDONLY, 0);
	if (IS_ERR(fp))
	    return;

	if (fp->f_op && fp->f_op->read) {
	    if (fp->f_op->read(fp, pid, 8, &fp->f_pos) > 0) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		p = pid_task(find_get_pid(simple_strtoul(pid, NULL, 10)),  PIDTYPE_PID);
#else
		p = find_task_by_pid(simple_strtoul(pid, NULL, 10));
#endif

		if (NULL != p) {
		    send_sig(SIGUSR2, p, 0);
		    send_sig(SIGUSR1, p, 0);
		}
	    }
	}
	filp_close(fp, NULL);

}
#endif


///////////////////////////////////////////////////////////////////
/////
///// ra2880Recv - process the next incoming packet
/////
///// Handle one incoming packet.  The packet is checked for errors and sent
///// to the upper layer.
/////
///// RETURNS: OK on success or ERROR.
///////////////////////////////////////////////////////////////////

#ifndef CONFIG_RAETH_NAPI
#if defined WORKQUEUE_BH || defined (TASKLET_WORKQUEUE_SW)
void ei_receive_workq(struct work_struct *work)
#else
void ei_receive(unsigned long unused)  // device structure
#endif // WORKQUEUE_BH //
{
	struct net_device *dev = dev_raether;
	END_DEVICE *ei_local = netdev_priv(dev);
	unsigned long reg_int_mask=0;
	int bReschedule=0;


	if(tx_ring_full==0){
		bReschedule = rt2880_eth_recv(dev);
		if(bReschedule)
		{
#ifdef WORKQUEUE_BH
			schedule_work(&ei_local->rx_wq);
#else
#if defined (TASKLET_WORKQUEUE_SW)
			if (working_schedule == 1)
				schedule_work(&ei_local->rx_wq);
			else
#endif
			tasklet_hi_schedule(&ei_local->rx_tasklet);
#endif // WORKQUEUE_BH //
		}else{
			reg_int_mask=sysRegRead(RAETH_FE_INT_ENABLE);
#if defined(DELAY_INT)
			sysRegWrite(RAETH_FE_INT_ENABLE, reg_int_mask| RX_DLY_INT);
#else
			sysRegWrite(RAETH_FE_INT_ENABLE, (reg_int_mask | RX_DONE_INT0 | RX_DONE_INT1));
#endif
#ifdef CONFIG_RAETH_QDMA
			reg_int_mask=sysRegRead(QFE_INT_ENABLE);
#if defined(DELAY_INT)
			sysRegWrite(QFE_INT_ENABLE, reg_int_mask| RX_DLY_INT);
#else
			sysRegWrite(QFE_INT_ENABLE, (reg_int_mask | RX_DONE_INT0 | RX_DONE_INT1));
#endif

#endif			
			
		}
	}else{
#ifdef WORKQUEUE_BH
                schedule_work(&ei_local->rx_wq);
#else
#if defined (TASKLET_WORKQUEUE_SW)
		if (working_schedule == 1)
			schedule_work(&ei_local->rx_wq);
		else
#endif
                tasklet_schedule(&ei_local->rx_tasklet);
#endif // WORKQUEUE_BH //
	}
}
#endif

#ifdef CONFIG_RAETH_NAPI
static int
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
raeth_clean(struct napi_struct *napi, int budget)
#else
raeth_clean(struct net_device *netdev, int *budget)
#endif
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	struct net_device *netdev=dev_raether;
        int work_to_do = budget;
#else
        int work_to_do = min(*budget, netdev->quota);
#endif
	END_DEVICE *ei_local =netdev_priv(netdev);
        int work_done = 0;
	unsigned long reg_int_mask=0;

	ei_xmit_housekeeping(0);

	rt2880_eth_recv(netdev, &work_done, work_to_do);

        /* this could control when to re-enable interrupt, 0-> mean never enable interrupt*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
        *budget -= work_done;
        netdev->quota -= work_done;
#endif
        /* if no Tx and not enough Rx work done, exit the polling mode */
        if(( (work_done < work_to_do)) || !netif_running(netdev)) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		napi_complete(&ei_local->napi);
#else
                netif_rx_complete(netdev);
#endif
		atomic_dec_and_test(&ei_local->irq_sem);

		sysRegWrite(RAETH_FE_INT_STATUS, RAETH_FE_INT_ALL);		// ack all fe interrupts
    		reg_int_mask=sysRegRead(RAETH_FE_INT_ENABLE);

#ifdef DELAY_INT
		sysRegWrite(RAETH_FE_INT_ENABLE, reg_int_mask |RAETH_FE_INT_DLY_INIT);  // init delay interrupt only
#else
		sysRegWrite(RAETH_FE_INT_ENABLE,reg_int_mask | RAETH_FE_INT_SETTING);
#endif

#ifdef CONFIG_RAETH_QDMA
		sysRegWrite(QFE_INT_STATUS, QFE_INT_ALL);
		reg_int_mask=sysRegRead(QFE_INT_ENABLE);
#ifdef DELAY_INT
                sysRegWrite(QFE_INT_ENABLE, reg_int_mask |QFE_INT_DLY_INIT);  // init delay interrupt only
#else
                sysRegWrite(QFE_INT_ENABLE,reg_int_mask | (RX_DONE_INT0 | RX_DONE_INT1 | RLS_DONE_INT));
#endif
#endif // CONFIG_RAETH_QDMA //

                return 0;
        }

        return 1;
}

#endif


void gsw_delay_setting(void) 
{
#if defined (CONFIG_GE_RGMII_INTERNAL_P0_AN) || defined (CONFIG_GE_RGMII_INTERNAL_P4_AN) 
	END_DEVICE *ei_local = netdev_priv(dev_raether);
	int reg_int_val = 0;
	int link_speed = 0;

	reg_int_val = sysRegRead(FE_INT_STATUS2);
	if( reg_int_val & BIT(25))
	{
		if(sysRegRead(RALINK_ETH_SW_BASE+0x0208) & 0x1) // link up
		{
			link_speed = (sysRegRead(RALINK_ETH_SW_BASE+0x0208)>>2 & 0x3);
			if(link_speed == 1)
			{
				// delay setting for 100M
				if((sysRegRead(0xbe00000c)&0xFFFF)==0x0101)	
					mii_mgr_write(31, 0x7b00, 8);
				printk("MT7621 GE2 link rate to 100M\n");
			} else 
			{
				//delay setting for 10/1000M
				if((sysRegRead(0xbe00000c)&0xFFFF)==0x0101)
					mii_mgr_write(31, 0x7b00, 0x102);
				printk("MT7621 GE2 link rate to 10M/1G\n");
			}
			schedule_work(&ei_local->kill_sig_wq);
		}
	}
	sysRegWrite(FE_INT_STATUS2, reg_int_val);
#endif
}

/**
 * ei_interrupt - handle controler interrupt
 *
 * This routine is called at interrupt level in response to an interrupt from
 * the controller.
 *
 * RETURNS: N/A.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
static irqreturn_t ei_interrupt(int irq, void *dev_id)
#else
static irqreturn_t ei_interrupt(int irq, void *dev_id, struct pt_regs * regs)
#endif
{
#if !defined(CONFIG_RAETH_NAPI)
	unsigned long reg_int_val;
	unsigned long reg_int_mask=0;
	unsigned int recv = 0;
	unsigned int transmit __maybe_unused = 0;
	unsigned long flags;
#endif

	struct net_device *dev = (struct net_device *) dev_id;
	END_DEVICE *ei_local = netdev_priv(dev);

	//Qwert
	/*
	unsigned long old,cur,dcycle;
	static int cnt = 0;
	static unsigned long max_dcycle = 0,tcycle = 0;
	old = read_c0_count();
	*/
	if (dev == NULL)
	{
		printk (KERN_ERR "net_interrupt(): irq %x for unknown device.\n", IRQ_ENET0);
		return IRQ_NONE;
	}

#ifdef CONFIG_RAETH_NAPI
	gsw_delay_setting();
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
        if(napi_schedule_prep(&ei_local->napi)) {
#else
        if(netif_rx_schedule_prep(dev)) {
#endif
                atomic_inc(&ei_local->irq_sem);
		sysRegWrite(RAETH_FE_INT_ENABLE, 0);
#ifdef CONFIG_RAETH_QDMA		
		sysRegWrite(QFE_INT_ENABLE, 0);
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		__napi_schedule(&ei_local->napi);
#else
                __netif_rx_schedule(dev);
#endif
        }
#else

	spin_lock_irqsave(&(ei_local->page_lock), flags);
	reg_int_val = sysRegRead(RAETH_FE_INT_STATUS);
#ifdef CONFIG_RAETH_QDMA	
	reg_int_val |= sysRegRead(QFE_INT_STATUS);
#endif

#if defined (DELAY_INT)
	if((reg_int_val & RX_DLY_INT))
		recv = 1;
	
	if (reg_int_val & RAETH_TX_DLY_INT)
		transmit = 1;
#else
	if((reg_int_val & RX_DONE_INT0))
		recv = 1;

#if defined (CONFIG_RAETH_MULTIPLE_RX_RING) 
	if((reg_int_val & RX_DONE_INT1))
		recv = 1;
#endif

	if (reg_int_val & RAETH_TX_DONE_INT0)
		transmit |= RAETH_TX_DONE_INT0;
#if defined (CONFIG_RAETH_QOS)
	if (reg_int_val & TX_DONE_INT1)
		transmit |= TX_DONE_INT1;
	if (reg_int_val & TX_DONE_INT2)
		transmit |= TX_DONE_INT2;
	if (reg_int_val & TX_DONE_INT3)
		transmit |= TX_DONE_INT3;
#endif //CONFIG_RAETH_QOS

#endif //DELAY_INT

#if defined (DELAY_INT)
	sysRegWrite(RAETH_FE_INT_STATUS, RAETH_FE_INT_DLY_INIT);
#else
	sysRegWrite(RAETH_FE_INT_STATUS, RAETH_FE_INT_ALL);
#endif
#ifdef CONFIG_RAETH_QDMA
#if defined (DELAY_INT)
	sysRegWrite(QFE_INT_STATUS, QFE_INT_DLY_INIT);
#else
	sysRegWrite(QFE_INT_STATUS, QFE_INT_ALL);
#endif
#endif	

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
	if(transmit)
		ei_xmit_housekeeping(0);
#else
		ei_xmit_housekeeping(0);
#endif

	if (((recv == 1) || (pending_recv ==1)) && (tx_ring_full==0))
	{
		reg_int_mask = sysRegRead(RAETH_FE_INT_ENABLE);
#if defined (DELAY_INT)
		sysRegWrite(RAETH_FE_INT_ENABLE, reg_int_mask & ~(RX_DLY_INT));
#else
		sysRegWrite(RAETH_FE_INT_ENABLE, reg_int_mask & ~(RX_DONE_INT0 | RX_DONE_INT1));
#endif //DELAY_INT
#ifdef CONFIG_RAETH_QDMA		
		reg_int_mask = sysRegRead(QFE_INT_ENABLE);
#if defined (DELAY_INT)
		sysRegWrite(QFE_INT_ENABLE, reg_int_mask & ~(RX_DLY_INT));
#else
		sysRegWrite(QFE_INT_ENABLE, reg_int_mask & ~(RX_DONE_INT0 | RX_DONE_INT1));
#endif //DELAY_INT
#endif

		pending_recv=0;
#ifdef WORKQUEUE_BH
		schedule_work(&ei_local->rx_wq);
#else
#if defined (TASKLET_WORKQUEUE_SW)
		if (working_schedule == 1)
			schedule_work(&ei_local->rx_wq);
		else
#endif
		tasklet_hi_schedule(&ei_local->rx_tasklet);
#endif // WORKQUEUE_BH //
	} 
	else if (recv == 1 && tx_ring_full==1) 
	{
		pending_recv=1;
	}
	else if((recv == 0) && (transmit == 0))
	{
		gsw_delay_setting();
	}
	spin_unlock_irqrestore(&(ei_local->page_lock), flags);
#endif

	return IRQ_HANDLED;
}

#if defined (CONFIG_RALINK_RT6855) || defined (CONFIG_RALINK_RT6855A) || \
    defined (CONFIG_RALINK_MT7620)|| defined (CONFIG_RALINK_MT7621)
static void esw_link_status_changed(int port_no, void *dev_id)
{
    unsigned int reg_val;
    struct net_device *dev = (struct net_device *) dev_id;
    END_DEVICE *ei_local = netdev_priv(dev);

#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
    defined (CONFIG_RALINK_MT7620)
    reg_val = *((volatile u32 *)(RALINK_ETH_SW_BASE+ 0x3008 + (port_no*0x100)));
#elif defined (CONFIG_RALINK_MT7621)
    mii_mgr_read(31, (0x3008 + (port_no*0x100)), &reg_val);
#endif    
    if(reg_val & 0x1) {
	printk("ESW: Link Status Changed - Port%d Link UP\n", port_no);
#if defined (CONFIG_RALINK_MT7621) && defined (CONFIG_RAETH_8023AZ_EEE)
	mii_mgr_write(port_no, 31, 0x52b5);
	mii_mgr_write(port_no, 16, 0xb780);
	mii_mgr_write(port_no, 17, 0x00e0);
	mii_mgr_write(port_no, 16, 0x9780);
#endif
#if defined (CONFIG_WAN_AT_P0)
	if(port_no==0) {
	    schedule_work(&ei_local->kill_sig_wq);
	}
#elif defined (CONFIG_WAN_AT_P4)
#if defined (CONFIG_TP_MODEL_MR400V4) || defined(CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
	if(port_no==3) {
	    schedule_work(&ei_local->kill_sig_wq);
	}
#else
	if(port_no==4) {
	    schedule_work(&ei_local->kill_sig_wq);
	}
#endif
#endif
    } else {	    
	printk("ESW: Link Status Changed - Port%d Link Down\n", port_no);
#if defined (CONFIG_RALINK_MT7621) && defined (CONFIG_RAETH_8023AZ_EEE)
	mii_mgr_write(port_no, 31, 0x52b5);
	mii_mgr_write(port_no, 16, 0xb780);
	mii_mgr_write(port_no, 17, 0x0000);
	mii_mgr_write(port_no, 16, 0x9780);
#endif
    }
}
#endif

#if defined (CONFIG_RT_3052_ESW) && ! defined(CONFIG_RALINK_MT7621)
static irqreturn_t esw_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	unsigned long reg_int_val;
#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
    defined(CONFIG_RALINK_MT7620)
	unsigned long acl_int_val;
	unsigned long mib_int_val;
#else
	static unsigned long stat;
	unsigned long stat_curr;
#endif
	
	struct net_device *dev = (struct net_device *) dev_id;
	END_DEVICE *ei_local = netdev_priv(dev);


	spin_lock_irqsave(&(ei_local->page_lock), flags);
	reg_int_val = (*((volatile u32 *)(ESW_ISR))); //Interrupt Status Register

#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
    defined(CONFIG_RALINK_MT7620)
	if (reg_int_val & P5_LINK_CH) {
	    esw_link_status_changed(5, dev_id);
	}
	if (reg_int_val & P4_LINK_CH) {
	    esw_link_status_changed(4, dev_id);
	}
	if (reg_int_val & P3_LINK_CH) {
	    esw_link_status_changed(3, dev_id);
	}
	if (reg_int_val & P2_LINK_CH) {
	    esw_link_status_changed(2, dev_id);
	}
	if (reg_int_val & P1_LINK_CH) {
	    esw_link_status_changed(1, dev_id);
	}
	if (reg_int_val & P0_LINK_CH) {
	    esw_link_status_changed(0, dev_id);
	}
	if (reg_int_val & ACL_INT) {
	    acl_int_val = sysRegRead(ESW_AISR);
	    sysRegWrite(ESW_AISR, acl_int_val);
	}
	if (reg_int_val & MIB_INT) {

	    mib_int_val = sysRegRead(ESW_P0_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P0_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p0_rx_good_cnt ++;	
		if(mib_int_val & TX_GOOD_CNT)
			p0_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p0_rx_byte_cnt ++;
		if(mib_int_val & TX_GOCT_CNT)
			p0_tx_byte_cnt ++;
	    }

	    mib_int_val = sysRegRead(ESW_P1_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P1_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p1_rx_good_cnt ++;		
		if(mib_int_val & TX_GOOD_CNT)
			p1_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p1_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p1_tx_byte_cnt ++;	
	    }

	    mib_int_val = sysRegRead(ESW_P2_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P2_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p2_rx_good_cnt ++;		
		if(mib_int_val & TX_GOOD_CNT)
			p2_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p2_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p2_tx_byte_cnt ++;	
	    }


	    mib_int_val = sysRegRead(ESW_P3_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P3_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p3_rx_good_cnt ++;		
		if(mib_int_val & TX_GOOD_CNT)
			p3_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p3_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p3_tx_byte_cnt ++;	
	    }

	    mib_int_val = sysRegRead(ESW_P4_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P4_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p4_rx_good_cnt ++;	
		if(mib_int_val & TX_GOOD_CNT)
			p4_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p4_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p4_tx_byte_cnt ++;	
	    }	

	    mib_int_val = sysRegRead(ESW_P5_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P5_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p5_rx_good_cnt ++;		
		if(mib_int_val & TX_GOOD_CNT)
			p5_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p5_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p5_tx_byte_cnt ++;	
	    }

	    mib_int_val = sysRegRead(ESW_P6_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P6_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p6_rx_good_cnt ++;		
		if(mib_int_val & TX_GOOD_CNT)
			p6_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p6_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p6_tx_byte_cnt ++;	
	    }
#if defined (CONFIG_RALINK_MT7620)
	    mib_int_val = sysRegRead(ESW_P7_IntSn);
	    if(mib_int_val){
		sysRegWrite(ESW_P7_IntSn, mib_int_val);
		if(mib_int_val & RX_GOOD_CNT)
			p7_rx_good_cnt ++;		
		if(mib_int_val & TX_GOOD_CNT)
			p7_tx_good_cnt ++;	
		if(mib_int_val & RX_GOCT_CNT)
			p7_rx_byte_cnt ++;	
		if(mib_int_val & TX_GOCT_CNT)
			p7_tx_byte_cnt ++;	
  
	    }
#endif	    
	}

#else // not RT6855
	if (reg_int_val & PORT_ST_CHG) {
		printk("RT305x_ESW: Link Status Changed\n");

		stat_curr = *((volatile u32 *)(RALINK_ETH_SW_BASE+0x80));
#ifdef CONFIG_WAN_AT_P0
		//link down --> link up : send signal to user application
		//link up --> link down : ignore
		if ((stat & (1<<25)) || !(stat_curr & (1<<25)))
#else
#if defined (CONFIG_TP_MODEL_MR400V4) || defined(CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
		if ((stat & (1<<28)) || !(stat_curr & (1<<28)))
#else
		if ((stat & (1<<29)) || !(stat_curr & (1<<29)))
#endif
#endif
			goto out;

		schedule_work(&ei_local->kill_sig_wq);
out:
		stat = stat_curr;
	}

#endif // defined(CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A)//

	sysRegWrite(ESW_ISR, reg_int_val);

	spin_unlock_irqrestore(&(ei_local->page_lock), flags);
	return IRQ_HANDLED;
}



#elif defined (CONFIG_RT_3052_ESW) && defined(CONFIG_RALINK_MT7621)

static irqreturn_t esw_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	unsigned int reg_int_val;
	struct net_device *dev = (struct net_device *) dev_id;
	END_DEVICE *ei_local = netdev_priv(dev);

	spin_lock_irqsave(&(ei_local->page_lock), flags);
        mii_mgr_read(31, 0x700c, &reg_int_val);

	if (reg_int_val & P4_LINK_CH) {
	    esw_link_status_changed(4, dev_id);
	}

	if (reg_int_val & P3_LINK_CH) {
	    esw_link_status_changed(3, dev_id);
	}
	if (reg_int_val & P2_LINK_CH) {
	    esw_link_status_changed(2, dev_id);
	}
	if (reg_int_val & P1_LINK_CH) {
	    esw_link_status_changed(1, dev_id);
	}
	if (reg_int_val & P0_LINK_CH) {
	    esw_link_status_changed(0, dev_id);
	}

        mii_mgr_write(31, 0x700c, 0x1f); //ack switch link change
	
	spin_unlock_irqrestore(&(ei_local->page_lock), flags);
	return IRQ_HANDLED;
}

#endif


static int ei_start_xmit_fake(struct sk_buff* skb, struct net_device *dev)
{
	return ei_start_xmit(skb, dev, 1);
}


#if defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT3352) || defined (CONFIG_RALINK_RT5350) 
void dump_phy_reg(int port_no, int from, int to, int is_local)
{
        u32 i=0;
        u32 temp=0;

        if(is_local==0) {
            printk("Global Register\n");
            printk("===============");
            mii_mgr_write(0, 31, 0); //select global register
            for(i=from;i<=to;i++) {
                if(i%8==0) {
                    printk("\n");
                }
                mii_mgr_read(port_no,i, &temp);
                printk("%02d: %04X ",i, temp);
            }
        } else {
            mii_mgr_write(0, 31, 0x8000); //select local register
                printk("\n\nLocal Register Port %d\n",port_no);
                printk("===============");
                for(i=from;i<=to;i++) {
                    if(i%8==0) {
                        printk("\n");
                    }
                    mii_mgr_read(port_no,i, &temp);
                    printk("%02d: %04X ",i, temp);
                }
        }
        printk("\n");
}
#else
void dump_phy_reg(int port_no, int from, int to, int is_local, int page_no)
{

        u32 i=0;
        u32 temp=0;
        u32 r31=0;


        if(is_local==0) {

            printk("\n\nGlobal Register Page %d\n",page_no);
            printk("===============");
            r31 |= 0 << 15; //global
            r31 |= ((page_no&0x7) << 12); //page no
            mii_mgr_write(port_no, 31, r31); //select global page x
            for(i=16;i<32;i++) {
                if(i%8==0) {
                    printk("\n");
                }
                mii_mgr_read(port_no,i, &temp);
                printk("%02d: %04X ",i, temp);
            }
        }else {
            printk("\n\nLocal Register Port %d Page %d\n",port_no, page_no);
            printk("===============");
            r31 |= 1 << 15; //local
            r31 |= ((page_no&0x7) << 12); //page no
            mii_mgr_write(port_no, 31, r31); //select local page x
            for(i=16;i<32;i++) {
                if(i%8==0) {
                    printk("\n");
                }
                mii_mgr_read(port_no,i, &temp);
                printk("%02d: %04X ",i, temp);
            }
        }
        printk("\n");
}

#endif

int ei_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
#if defined(CONFIG_RT_3052_ESW) || defined(CONFIG_RAETH_QDMA)
		esw_reg reg;
#endif
#if defined(CONFIG_RALINK_RT3352) || defined(CONFIG_RALINK_RT5350) || \
    defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
    defined(CONFIG_RALINK_MT7620) || defined(CONFIG_RALINK_MT7621) || defined (CONFIG_RALINK_MT7628)
        esw_rate ratelimit;
#endif
#if defined(CONFIG_RT_3052_ESW)
	unsigned int offset = 0;
	unsigned int value = 0;
#endif

	ra_mii_ioctl_data mii;
	switch (cmd) {
#if defined(CONFIG_RAETH_QDMA)		
#define _HQOS_REG(x)	(*((volatile u32 *)(RALINK_FRAME_ENGINE_BASE + QDMA_RELATED + x)))
		case RAETH_QDMA_REG_READ:
			copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
			if (reg.off > REG_HQOS_MAX)
				return -EINVAL;
			reg.val = _HQOS_REG(reg.off);
			//printk("read reg off:%x val:%x\n", reg.off, reg.val);
			copy_to_user(ifr->ifr_data, &reg, sizeof(reg));
			break;
		case RAETH_QDMA_REG_WRITE:
			copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
			if (reg.off > REG_HQOS_MAX)
				return -EINVAL;
			_HQOS_REG(reg.off) = reg.val;
			//printk("write reg off:%x val:%x\n", reg.off, reg.val);
			break;
                case RAETH_QDMA_READ_CPU_CLK:
                        copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
                        reg.val = get_surfboard_sysclk();
                        //printk("read reg off:%x val:%x\n", reg.off, reg.val);
			copy_to_user(ifr->ifr_data, &reg, sizeof(reg));
			break;
			
        	case RAETH_QDMA_QUEUE_MAPPING:
			copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
			M2Q_table[reg.off] = reg.val;
			//printk(" M2Q_table[%d] is %d.\n", reg.off, M2Q_table[reg.off]);
	  	break;
#endif	  	
		case RAETH_MII_READ:
			copy_from_user(&mii, ifr->ifr_data, sizeof(mii));
			mii_mgr_read(mii.phy_id, mii.reg_num, &mii.val_out);
			//printk("phy %d, reg %d, val 0x%x\n", mii.phy_id, mii.reg_num, mii.val_out);
			copy_to_user(ifr->ifr_data, &mii, sizeof(mii));
			break;

		case RAETH_MII_WRITE:
			copy_from_user(&mii, ifr->ifr_data, sizeof(mii));
			//printk("phy %d, reg %d, val 0x%x\n", mii.phy_id, mii.reg_num, mii.val_in);
			mii_mgr_write(mii.phy_id, mii.reg_num, mii.val_in);
			break;
#if defined (CONFIG_RALINK_MT7621) || defined (CONFIG_RALINK_MT7620) || defined (CONFIG_ARCH_MT8590)
		case RAETH_MII_READ_CL45:
			copy_from_user(&mii, ifr->ifr_data, sizeof(mii));
			//mii_mgr_cl45_set_address(mii.port_num, mii.dev_addr, mii.reg_addr);
			mii_mgr_read_cl45(mii.port_num, mii.dev_addr, mii.reg_addr, &mii.val_out);
			copy_to_user(ifr->ifr_data, &mii, sizeof(mii));
			break;
		case RAETH_MII_WRITE_CL45:
			copy_from_user(&mii, ifr->ifr_data, sizeof(mii));
			//mii_mgr_cl45_set_address(mii.port_num, mii.dev_addr, mii.reg_addr);
			mii_mgr_write_cl45(mii.port_num, mii.dev_addr, mii.reg_addr, mii.val_in);
			break;
#endif
			
#if defined(CONFIG_RT_3052_ESW)
#define _ESW_REG(x)	(*((volatile u32 *)(RALINK_ETH_SW_BASE + x)))
		case RAETH_ESW_REG_READ:
			copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
			if (reg.off > REG_ESW_MAX)
				return -EINVAL;
			reg.val = _ESW_REG(reg.off);
			//printk("read reg off:%x val:%x\n", reg.off, reg.val);
			copy_to_user(ifr->ifr_data, &reg, sizeof(reg));
			break;
		case RAETH_ESW_REG_WRITE:
			copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
			if (reg.off > REG_ESW_MAX)
				return -EINVAL;
			_ESW_REG(reg.off) = reg.val;
			//printk("write reg off:%x val:%x\n", reg.off, reg.val);
			break;
		case RAETH_ESW_PHY_DUMP:
			copy_from_user(&reg, ifr->ifr_data, sizeof(reg));
#if defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT3352) || defined (CONFIG_RALINK_RT5350)
			if (reg.val ==32 ) {//dump all phy register
			    /* Global Register 0~31
			     * Local Register 0~31
			     */
			    dump_phy_reg(0, 0, 31, 0); //dump global register
			    for(offset=0;offset<5;offset++) {
				dump_phy_reg(offset, 0, 31, 1); //dump local register
			    }
			} else {
			    dump_phy_reg(reg.val, 0, 31, 0); //dump global register
			    dump_phy_reg(reg.val, 0, 31, 1); //dump local register
			}
#else
			/* SPEC defined Register 0~15
			 * Global Register 16~31 for each page
			 * Local Register 16~31 for each page
			 */
			printk("SPEC defined Register");
			if (reg.val ==32 ) {//dump all phy register
			    int i = 0;
			    for(i=0; i<5; i++){	
				printk("\n[Port %d]===============",i);
				for(offset=0;offset<16;offset++) {
				    if(offset%8==0) {
					printk("\n");
				}
				mii_mgr_read(i,offset, &value);
				printk("%02d: %04X ",offset, value);
				}
			    }	
			}
			else{
				printk("\n[Port %d]===============",reg.val);
				for(offset=0;offset<16;offset++) {
				    if(offset%8==0) {
					printk("\n");
				}
				mii_mgr_read(reg.val,offset, &value);
				printk("%02d: %04X ",offset, value);
				}
			}

#if defined (CONFIG_RALINK_MT7628)
			for(offset=0;offset<7;offset++) { //global register  page 0~6
#else
			for(offset=0;offset<5;offset++) { //global register  page 0~4
#endif
			    if(reg.val == 32) //dump all phy register
			        dump_phy_reg(0, 16, 31, 0, offset);
			    else
			        dump_phy_reg(reg.val, 16, 31, 0, offset);
			}

			if (reg.val == 32) {//dump all phy register
#if !defined (CONFIG_RAETH_HAS_PORT4)
				for(offset=0;offset<5;offset++) { //local register port 0-port4
#else
				for(offset=0;offset<4;offset++) { //local register port 0-port3
#endif	    
				    	dump_phy_reg(offset, 16, 31, 1, 0); //dump local page 0
					dump_phy_reg(offset, 16, 31, 1, 1); //dump local page 1
					dump_phy_reg(offset, 16, 31, 1, 2); //dump local page 2
					dump_phy_reg(offset, 16, 31, 1, 3); //dump local page 3
				}
			}else {
				dump_phy_reg(reg.val, 16, 31, 1, 0); //dump local page 0
				dump_phy_reg(reg.val, 16, 31, 1, 1); //dump local page 1
				dump_phy_reg(reg.val, 16, 31, 1, 2); //dump local page 2
				dump_phy_reg(reg.val, 16, 31, 1, 3); //dump local page 3
			}
#endif
			break;

#if defined (CONFIG_RALINK_RT3352) || defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_MT7628)
#define _ESW_REG(x)	(*((volatile u32 *)(RALINK_ETH_SW_BASE + x)))
		case RAETH_ESW_INGRESS_RATE:
			copy_from_user(&ratelimit, ifr->ifr_data, sizeof(ratelimit));
			offset = 0x11c + (4 * (ratelimit.port / 2));
                        value = _ESW_REG(offset);

			if((ratelimit.port % 2) == 0)
			{
				value &= 0xffff0000;
				if(ratelimit.on_off == 1)
				{
					value |= (ratelimit.on_off << 14);
					value |= (0x07 << 10);
					value |= ratelimit.bw;
				}
			}
			else if((ratelimit.port % 2) == 1)
			{
				value &= 0x0000ffff;
				if(ratelimit.on_off == 1)
				{
					value |= (ratelimit.on_off << 30);
					value |= (0x07 << 26);
					value |= (ratelimit.bw << 16);
				}
			}
			printk("offset = 0x%4x value=0x%x\n\r", offset, value);

			_ESW_REG(offset) = value;
			break;

		case RAETH_ESW_EGRESS_RATE:
			copy_from_user(&ratelimit, ifr->ifr_data, sizeof(ratelimit));
			offset = 0x140 + (4 * (ratelimit.port / 2));
                        value = _ESW_REG(offset);

			if((ratelimit.port % 2) == 0)
			{
				value &= 0xffff0000;
				if(ratelimit.on_off == 1)
				{
					value |= (ratelimit.on_off << 12);
					value |= (0x03 << 10);
					value |= ratelimit.bw;
				}
			}
			else if((ratelimit.port % 2) == 1)
			{
				value &= 0x0000ffff;
				if(ratelimit.on_off == 1)
				{
					value |= (ratelimit.on_off << 28);
					value |= (0x03 << 26);
					value |= (ratelimit.bw << 16);
				}
			}
			printk("offset = 0x%4x value=0x%x\n\r", offset, value);
			_ESW_REG(offset) = value;
			break;
#elif  defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
       defined(CONFIG_RALINK_MT7620) || defined(CONFIG_RALINK_MT7621)
#define _ESW_REG(x)	(*((volatile u32 *)(RALINK_ETH_SW_BASE + x)))
		case RAETH_ESW_INGRESS_RATE:
			copy_from_user(&ratelimit, ifr->ifr_data, sizeof(ratelimit));
#if defined(CONFIG_RALINK_RT6855A) || defined(CONFIG_RALINK_MT7621)
			offset = 0x1800 + (0x100 * ratelimit.port);
#else
			offset = 0x1080 + (0x100 * ratelimit.port);
#endif
                        value = _ESW_REG(offset);

			value &= 0xffff0000;
			if(ratelimit.on_off == 1)
			{
				value |= (ratelimit.on_off << 15);
				if (ratelimit.bw < 100)
				{
					value |= (0x0 << 8);
					value |= ratelimit.bw;
				}else if(ratelimit.bw < 1000)
				{
					value |= (0x1 << 8);
					value |= ratelimit.bw/10;
				}else if(ratelimit.bw < 10000)
				{
					value |= (0x2 << 8);
					value |= ratelimit.bw/100;
				}else if(ratelimit.bw < 100000)
				{
					value |= (0x3 << 8);
					value |= ratelimit.bw/1000;
				}else 
				{
					value |= (0x4 << 8);
					value |= ratelimit.bw/10000;
				}
			}
			printk("offset = 0x%4x value=0x%x\n\r", offset, value);
#ifndef CONFIG_RALINK_MT7621			
			_ESW_REG(offset) = value;
#else			
			mii_mgr_write(0x1f, offset, value);
#endif			
			break;

		case RAETH_ESW_EGRESS_RATE:
			copy_from_user(&ratelimit, ifr->ifr_data, sizeof(ratelimit));
			offset = 0x1040 + (0x100 * ratelimit.port);
                        value = _ESW_REG(offset);

			value &= 0xffff0000;
			if(ratelimit.on_off == 1)
			{
				value |= (ratelimit.on_off << 15);
				if (ratelimit.bw < 100)
				{
					value |= (0x0 << 8);
					value |= ratelimit.bw;
				}else if(ratelimit.bw < 1000)
				{
					value |= (0x1 << 8);
					value |= ratelimit.bw/10;
				}else if(ratelimit.bw < 10000)
				{
					value |= (0x2 << 8);
					value |= ratelimit.bw/100;
				}else if(ratelimit.bw < 100000)
				{
					value |= (0x3 << 8);
					value |= ratelimit.bw/1000;
				}else 
				{
					value |= (0x4 << 8);
					value |= ratelimit.bw/10000;
				}
			}
			printk("offset = 0x%4x value=0x%x\n\r", offset, value);
#ifndef CONFIG_RALINK_MT7621			
			_ESW_REG(offset) = value;
#else
			mii_mgr_write(0x1f, offset, value);
#endif
			break;
#endif
#endif // CONFIG_RT_3052_ESW
#ifdef CONFIG_MULTICAST_VLAN
        case TPLINK_SET_MULCIAST_VLAN:
            multicastVlanSet(ifr);
            break;
#endif
		default:
			return -EOPNOTSUPP;

	}

	return 0;
}

/*
 * Set new MTU size
 * Change the mtu of Raeth Ethernet Device
 */
static int ei_change_mtu(struct net_device *dev, int new_mtu)
{
	END_DEVICE *ei_local = netdev_priv(dev);  // get priv ei_local pointer from net_dev structure

	if ( ei_local == NULL ) {
		printk(KERN_EMERG "%s: ei_change_mtu passed a non-existent private pointer from net_dev!\n", dev->name);
		return -ENXIO;
	}


	if ( (new_mtu > 4096) || (new_mtu < 64)) {
		return -EINVAL;
	}

#ifndef CONFIG_RAETH_JUMBOFRAME
	if ( new_mtu > 1500 ) {
		return -EINVAL;
	}
#endif

	dev->mtu = new_mtu;

	return 0;
}

#ifdef CONFIG_RAETH_HW_VLAN_RX
static void ei_vlan_rx_register(struct net_device *dev, struct vlan_group *grp)
{
	END_DEVICE *ei_local = netdev_priv(dev);
	
	ei_local->vlgrp = grp;

	/* enable HW VLAN RX */
	sysRegWrite(CDMP_EG_CTRL, 1);

}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
static const struct net_device_ops ei_netdev_ops = {
        .ndo_init               = rather_probe,
        .ndo_open               = ei_open,
        .ndo_stop               = ei_close,
        .ndo_start_xmit         = ei_start_xmit_fake,
        .ndo_get_stats          = ra_get_stats,
        .ndo_set_mac_address    = eth_mac_addr,
        .ndo_change_mtu         = ei_change_mtu,
        .ndo_do_ioctl           = ei_ioctl,
        .ndo_validate_addr      = eth_validate_addr,
#ifdef CONFIG_RAETH_HW_VLAN_RX
	.ndo_vlan_rx_register   = ei_vlan_rx_register,
#endif
#ifdef CONFIG_NET_POLL_CONTROLLER
        .ndo_poll_controller    = raeth_clean,
#endif
//	.ndo_tx_timeout		= ei_tx_timeout,
};
#endif

void ra2880_setup_dev_fptable(struct net_device *dev)
{
	RAETH_PRINT(__FUNCTION__ "is called!\n");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	dev->netdev_ops		= &ei_netdev_ops;
#else
	dev->open		= ei_open;
	dev->stop		= ei_close;
	dev->hard_start_xmit	= ei_start_xmit_fake;
	dev->get_stats		= ra_get_stats;
	dev->set_mac_address	= ei_set_mac_addr;
	dev->change_mtu		= ei_change_mtu;
	dev->mtu		= 1500;
	dev->do_ioctl		= ei_ioctl;
//	dev->tx_timeout		= ei_tx_timeout;

#ifdef CONFIG_RAETH_NAPI
        dev->poll = &raeth_clean;
#if defined (CONFIG_RAETH_ROUTER)
	dev->weight = 32;
#elif defined (CONFIG_RT_3052_ESW)
	dev->weight = 32;
#else
	dev->weight = 128;
#endif
#endif
#endif
#if defined (CONFIG_ETHTOOL) /*&& defined (CONFIG_RAETH_ROUTER)*/
	dev->ethtool_ops	= &ra_ethtool_ops;
#endif
#define TX_TIMEOUT (5*HZ)
	dev->watchdog_timeo = TX_TIMEOUT;

}

/* reset frame engine */
void fe_reset(void)
{
#if defined (CONFIG_RALINK_RT6855A)
	/* FIXME */
#else
	u32 val;
	val = sysRegRead(RSTCTRL);

// RT5350 need to reset ESW and FE at the same to avoid PDMA panic //	
#if defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_MT7628)
	val = val | RALINK_FE_RST | RALINK_ESW_RST ;
#else
	val = val | RALINK_FE_RST;
#endif
	sysRegWrite(RSTCTRL, val);
#if defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_MT7620) || defined (CONFIG_RALINK_MT7628)
	val = val & ~(RALINK_FE_RST | RALINK_ESW_RST);
#else
	val = val & ~(RALINK_FE_RST);
#endif
	sysRegWrite(RSTCTRL, val);
#endif
}

/* set TRGMII */
#if defined (CONFIG_GE1_TRGMII_FORCE_1200) && defined (CONFIG_MT7621_ASIC)
void trgmii_set_7621(void)
{
	u32 val = 0;
	u32 val_0 = 0;

	val = sysRegRead(RSTCTRL);
// MT7621 need to reset GMAC and FE first //	
	val = val | RALINK_FE_RST | RALINK_ETH_RST ;
	sysRegWrite(RSTCTRL, val);

//set TRGMII clock//
	val_0 = sysRegRead(CLK_CFG_0);
	val_0 &= 0xffffff9f;
	val_0 |= (0x1 << 5);
	sysRegWrite(CLK_CFG_0, val_0);
	mdelay(1);
	val_0 = sysRegRead(CLK_CFG_0);
	printk("set CLK_CFG_0 = 0x%x!!!!!!!!!!!!!!!!!!1\n",val_0);
	val = val & ~(RALINK_FE_RST | RALINK_ETH_RST);
	sysRegWrite(RSTCTRL, val);
}

void trgmii_set_7530(void)
{
// set MT7530 //
#if 0 
	
	mii_mgr_write(31, 103, 0x0020);


	//disable EEE
	mii_mgr_write(0, 0x16, 0);
	mii_mgr_write(1, 0x16, 0);
	mii_mgr_write(2, 0x16, 0);
	mii_mgr_write(3, 0x16, 0);
	mii_mgr_write(4, 0x16, 0);


	//PLL reset for E2
	mii_mgr_write(31, 104, 0x0608);
	mii_mgr_write(31, 104, 0x2608);
	
	mii_mgr_write(31, 0x7808, 0x0);
	mdelay(1);
	mii_mgr_write(31, 0x7804, 0x01017e8f);
	mdelay(1);
	mii_mgr_write(31, 0x7808, 0x1);
	mdelay(1);

#endif
#if 1
	//CL45 command
	//PLL to 150Mhz	
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x404);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x0780);//40Mhz XTAL for 150Mhz CLK
	//mii_mgr_write(0, 14, 0x0C00);//ori
	mdelay(1);

	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x409);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x57);
	mdelay(1);

	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x40a);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x57);

//PLL BIAS en
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x403);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x1800);
	mdelay(1);

//BIAS LPF en
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x403);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x1c00);

//sys PLL en
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x401);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0xc020);

//LCDDDS PWDS
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x406);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0xa030);
	mdelay(1);

//GSW_2X_CLK
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x410);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x0003);

//enable P6
	mii_mgr_write(31, 0x3600, 0x5e33b);

//enable TRGMII
	mii_mgr_write(31, 0x7830, 0x1);
#endif	

}
#endif

void ei_reset_task(struct work_struct *work)
{
	struct net_device *dev = dev_raether;

	ei_close(dev);
	ei_open(dev);

	return;
}

void ei_tx_timeout(struct net_device *dev)
{
        END_DEVICE *ei_local = netdev_priv(dev);

        schedule_work(&ei_local->reset_task);
}

void setup_statistics(END_DEVICE* ei_local)
{
	ei_local->stat.tx_packets	= 0;
	ei_local->stat.tx_bytes 	= 0;
	ei_local->stat.tx_dropped 	= 0;
	ei_local->stat.tx_errors	= 0;
	ei_local->stat.tx_aborted_errors= 0;
	ei_local->stat.tx_carrier_errors= 0;
	ei_local->stat.tx_fifo_errors	= 0;
	ei_local->stat.tx_heartbeat_errors = 0;
	ei_local->stat.tx_window_errors	= 0;

	ei_local->stat.rx_packets	= 0;
	ei_local->stat.rx_bytes 	= 0;
	ei_local->stat.rx_dropped 	= 0;
	ei_local->stat.rx_errors	= 0;
	ei_local->stat.rx_length_errors = 0;
	ei_local->stat.rx_over_errors	= 0;
	ei_local->stat.rx_crc_errors	= 0;
	ei_local->stat.rx_frame_errors	= 0;
	ei_local->stat.rx_fifo_errors	= 0;
	ei_local->stat.rx_missed_errors	= 0;

	ei_local->stat.collisions	= 0;
#if defined (CONFIG_RAETH_QOS)
	ei_local->tx3_full = 0;
	ei_local->tx2_full = 0;
	ei_local->tx1_full = 0;
	ei_local->tx0_full = 0;
#else
	ei_local->tx_full = 0;
#endif
#ifdef CONFIG_RAETH_NAPI
	atomic_set(&ei_local->irq_sem, 1);
#endif

}

/**
 * rather_probe - pick up ethernet port at boot time
 * @dev: network device to probe
 *
 * This routine probe the ethernet port at boot time.
 *
 *
 */
extern int spiflash_ioctl_read(u_char *buf, u32 from, u32 len);

int __init rather_probe(struct net_device *dev)
{
	int i;
	END_DEVICE *ei_local = netdev_priv(dev);
	struct sockaddr addr;
	unsigned char zero1[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	unsigned char zero2[6]={0x00,0x00,0x00,0x00,0x00,0x00};
	char mac[6] = {0};

	fe_reset();

#if 0
	//Get mac0 address from flash
#ifdef RA_MTD_RW_BY_NUM
	i = ra_mtd_read(2, GMAC0_OFFSET, 6, addr.sa_data);
#else
	i = ra_mtd_read_nm("Factory", GMAC0_OFFSET, 6, addr.sa_data);
#endif
#else
/* Get MAC from flash, yuanshang, 2013-11-11 */

	spiflash_ioctl_read(addr.sa_data, TP_MACADDR_OFFSET, 6);

	printk("Read MAC from flash(0x%X) %02x-%02x-%02x-%02x-%02x-%02x\n",
		TP_MACADDR_OFFSET, addr.sa_data[0], addr.sa_data[1], addr.sa_data[2], addr.sa_data[3], addr.sa_data[4], addr.sa_data[5]);

#endif

	//If reading mtd failed or mac0 is empty, generate a mac address
	if (i < 0 || ((memcmp(addr.sa_data, zero1, 6) == 0) || (addr.sa_data[0] & 0x1)) || 
	    (memcmp(addr.sa_data, zero2, 6) == 0)) {
		unsigned char mac_addr01234[5] = {0x00, 0x0C, 0x43, 0x28, 0x80};
		net_srandom(jiffies);
		memcpy(addr.sa_data, mac_addr01234, 5);
		addr.sa_data[5] = 0x02;/*net_random()&0xFF;*/
		printk("Read MAC NULL, set default MAC: %02x-%02x-%02x-%02x-%02x-%02x\n", 
		addr.sa_data[0], addr.sa_data[1], addr.sa_data[2], addr.sa_data[3], addr.sa_data[4], addr.sa_data[5]);		
	}

#ifdef CONFIG_RAETH_NAPI
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	netif_napi_add(dev, &ei_local->napi, raeth_clean, 128);
#endif
#endif

	ei_set_mac_addr(dev, &addr);
	spin_lock_init(&ei_local->page_lock);
	ether_setup(dev);

#ifdef CONFIG_RAETH_LRO
	ei_local->lro_mgr.dev = dev;
        memset(&ei_local->lro_mgr.stats, 0, sizeof(ei_local->lro_mgr.stats));
        ei_local->lro_mgr.features = LRO_F_NAPI;
        ei_local->lro_mgr.ip_summed = CHECKSUM_UNNECESSARY;
        ei_local->lro_mgr.ip_summed_aggr = CHECKSUM_UNNECESSARY;
        ei_local->lro_mgr.max_desc = ARRAY_SIZE(ei_local->lro_arr);
        ei_local->lro_mgr.max_aggr = 64;
        ei_local->lro_mgr.frag_align_pad = 0;
        ei_local->lro_mgr.lro_arr = ei_local->lro_arr;
        ei_local->lro_mgr.get_skb_header = rt_get_skb_header;
#endif

	setup_statistics(ei_local);

	return 0;
}

#ifdef CONFIG_PSEUDO_SUPPORT
int VirtualIF_ioctl(struct net_device * net_dev,
		    struct ifreq * ifr, int cmd)
{
	ra_mii_ioctl_data mii;

	switch (cmd) {
		case RAETH_MII_READ:
			copy_from_user(&mii, ifr->ifr_data, sizeof(mii));
			mii_mgr_read(mii.phy_id, mii.reg_num, &mii.val_out);
			//printk("phy %d, reg %d, val 0x%x\n", mii.phy_id, mii.reg_num, mii.val_out);
			copy_to_user(ifr->ifr_data, &mii, sizeof(mii));
			break;

		case RAETH_MII_WRITE:
			copy_from_user(&mii, ifr->ifr_data, sizeof(mii));
			//printk("phy %d, reg %d, val 0x%x\n", mii.phy_id, mii.reg_num, mii.val_in);
			mii_mgr_write(mii.phy_id, mii.reg_num, mii.val_in);
			break;
		default:
			return -EOPNOTSUPP;
	}

	return 0;
}

struct net_device_stats *VirtualIF_get_stats(struct net_device *dev)
{
	PSEUDO_ADAPTER *pAd = netdev_priv(dev);
	return &pAd->stat;
}

int VirtualIF_open(struct net_device * dev)
{
    PSEUDO_ADAPTER *pPesueoAd = netdev_priv(dev);

    printk("%s: ===> VirtualIF_open\n", dev->name);
 
#if defined (CONFIG_GE_RGMII_INTERNAL_P0_AN) || defined (CONFIG_GE_RGMII_INTERNAL_P4_AN)
    *((volatile u32 *)(FE_INT_ENABLE2)) |= (1<<25); //enable GE2 link change intr for MT7530 delay setting
#endif

    netif_start_queue(pPesueoAd->PseudoDev);

    return 0;
}

int VirtualIF_close(struct net_device * dev)
{
    PSEUDO_ADAPTER *pPesueoAd = netdev_priv(dev);

    printk("%s: ===> VirtualIF_close\n", dev->name);

    netif_stop_queue(pPesueoAd->PseudoDev);

    return 0;
}

int VirtualIFSendPackets(struct sk_buff * pSkb,
			 struct net_device * dev)
{
    PSEUDO_ADAPTER *pPesueoAd = netdev_priv(dev);
    END_DEVICE *ei_local __maybe_unused;


    //printk("VirtualIFSendPackets --->\n");

    ei_local = netdev_priv(dev);
    if (!(pPesueoAd->RaethDev->flags & IFF_UP)) {
	dev_kfree_skb_any(pSkb);
	return 0;
    }
    //pSkb->cb[40]=0x5a;
    pSkb->dev = pPesueoAd->RaethDev;
    ei_start_xmit(pSkb, pPesueoAd->RaethDev, 2);
    return 0;
}

void virtif_setup_statistics(PSEUDO_ADAPTER* pAd)
{
	pAd->stat.tx_packets	= 0;
	pAd->stat.tx_bytes 	= 0;
	pAd->stat.tx_dropped 	= 0;
	pAd->stat.tx_errors	= 0;
	pAd->stat.tx_aborted_errors= 0;
	pAd->stat.tx_carrier_errors= 0;
	pAd->stat.tx_fifo_errors	= 0;
	pAd->stat.tx_heartbeat_errors = 0;
	pAd->stat.tx_window_errors	= 0;

	pAd->stat.rx_packets	= 0;
	pAd->stat.rx_bytes 	= 0;
	pAd->stat.rx_dropped 	= 0;
	pAd->stat.rx_errors	= 0;
	pAd->stat.rx_length_errors = 0;
	pAd->stat.rx_over_errors	= 0;
	pAd->stat.rx_crc_errors	= 0;
	pAd->stat.rx_frame_errors	= 0;
	pAd->stat.rx_fifo_errors	= 0;
	pAd->stat.rx_missed_errors	= 0;

	pAd->stat.collisions	= 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
static const struct net_device_ops VirtualIF_netdev_ops = {
        .ndo_open               = VirtualIF_open,
        .ndo_stop               = VirtualIF_close,
        .ndo_start_xmit         = VirtualIFSendPackets,
        .ndo_get_stats          = VirtualIF_get_stats,
        .ndo_set_mac_address    = ei_set_mac2_addr,
        .ndo_change_mtu         = ei_change_mtu,
        .ndo_do_ioctl           = VirtualIF_ioctl,
        .ndo_validate_addr      = eth_validate_addr,
};
#endif
// Register pseudo interface
void RAETH_Init_PSEUDO(pEND_DEVICE pAd, struct net_device *net_dev)
{
    int index;
    struct net_device *dev;
    PSEUDO_ADAPTER *pPseudoAd;
    int i = 0;
    struct sockaddr addr;
    unsigned char zero1[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    unsigned char zero2[6]={0x00,0x00,0x00,0x00,0x00,0x00};

    for (index = 0; index < MAX_PSEUDO_ENTRY; index++) {

	dev = alloc_etherdev(sizeof(PSEUDO_ADAPTER));
	if (NULL == dev)
	{
		printk(" alloc_etherdev for PSEUDO_ADAPTER failed.\n");
		return;
	}
	strcpy(dev->name, DEV2_NAME);

	//Get mac2 address from flash
#ifdef RA_MTD_RW_BY_NUM
	i = ra_mtd_read(2, GMAC2_OFFSET, 6, addr.sa_data);
#else
	i = ra_mtd_read_nm("Factory", GMAC2_OFFSET, 6, addr.sa_data);
#endif

	//If reading mtd failed or mac0 is empty, generate a mac address
	if (i < 0 || ((memcmp(addr.sa_data, zero1, 6) == 0) || (addr.sa_data[0] & 0x1)) || 
	    (memcmp(addr.sa_data, zero2, 6) == 0)) {
		unsigned char mac_addr01234[5] = {0x00, 0x0C, 0x43, 0x28, 0x80};
		net_srandom(jiffies);
		memcpy(addr.sa_data, mac_addr01234, 5);
		addr.sa_data[5] = net_random()&0xFF;
	}

	ei_set_mac2_addr(dev, &addr);
	ether_setup(dev);
	pPseudoAd = netdev_priv(dev);

	pPseudoAd->PseudoDev = dev;
	pPseudoAd->RaethDev = net_dev;
	virtif_setup_statistics(pPseudoAd);
	pAd->PseudoDev = dev;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	dev->netdev_ops		= &VirtualIF_netdev_ops;
#else
	dev->hard_start_xmit = VirtualIFSendPackets;
	dev->stop = VirtualIF_close;
	dev->open = VirtualIF_open;
	dev->do_ioctl = VirtualIF_ioctl;
	dev->set_mac_address = ei_set_mac2_addr;
	dev->get_stats = VirtualIF_get_stats;
	dev->change_mtu = ei_change_mtu;
	dev->mtu = 1500;
#endif

	dev->features |= NETIF_F_IP_CSUM; /* Can checksum TCP/UDP over IPv4 */

#if defined(CONFIG_RALINK_MT7620)
#if defined (CONFIG_RAETH_TSO)
	if ((sysRegRead(0xB000000C) & 0xf) >= 0x5) {
		dev->features |= NETIF_F_SG;
		dev->features |= NETIF_F_TSO;
	}
#endif // CONFIG_RAETH_TSO //

#if defined (CONFIG_RAETH_TSOV6)
	if ((sysRegRead(0xB000000C) & 0xf) >= 0x5) {
		dev->features |= NETIF_F_TSO6;
		dev->features |= NETIF_F_IPV6_CSUM; /* Can checksum TCP/UDP over IPv6 */
	}
#endif 
#else
#if defined (CONFIG_RAETH_TSO)
        dev->features |= NETIF_F_SG;
        dev->features |= NETIF_F_TSO;
#endif // CONFIG_RAETH_TSO //

#if defined (CONFIG_RAETH_TSOV6)
        dev->features |= NETIF_F_TSO6;
        dev->features |= NETIF_F_IPV6_CSUM; /* Can checksum TCP/UDP over IPv6 */
#endif 
#endif // CONFIG_RALINK_MT7620 //

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
	dev->vlan_features = dev->features;
#endif


#if defined (CONFIG_ETHTOOL) /*&& defined (CONFIG_RAETH_ROUTER)*/
	dev->ethtool_ops = &ra_virt_ethtool_ops;
    // init mii structure
	pPseudoAd->mii_info.dev = dev;
	pPseudoAd->mii_info.mdio_read = mdio_virt_read;
	pPseudoAd->mii_info.mdio_write = mdio_virt_write;
	pPseudoAd->mii_info.phy_id_mask = 0x1f;
	pPseudoAd->mii_info.reg_num_mask = 0x1f;
	pPseudoAd->mii_info.phy_id = 0x1e;
	pPseudoAd->mii_info.supports_gmii = mii_check_gmii_support(&pPseudoAd->mii_info);
#endif

	// Register this device
	register_netdevice(dev);
    }
}
#endif

/**
 * ei_open - Open/Initialize the ethernet port.
 * @dev: network device to initialize
 *
 * This routine goes all-out, setting everything
 * up a new at each open, even though many of these registers should only need to be set once at boot.
 */
int ei_open(struct net_device *dev)
{
	int i, err;
	unsigned long flags;
	END_DEVICE *ei_local;

#ifdef CONFIG_RAETH_LRO
	const char *lan_ip_tmp; 
#ifdef CONFIG_DUAL_IMAGE
#define RT2860_NVRAM	1
#else
#define RT2860_NVRAM	0
#endif
#endif // CONFIG_RAETH_LRO //

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	if (!try_module_get(THIS_MODULE))
	{
		printk("%s: Cannot reserve module\n", __FUNCTION__);
		return -1;
	}
#else
	MOD_INC_USE_COUNT;
#endif

	printk("Raeth %s (",RAETH_VERSION);
#if defined (CONFIG_RAETH_NAPI)
	printk("NAPI\n");
#elif defined (CONFIG_RA_NETWORK_TASKLET_BH)
	printk("Tasklet");
#elif defined (CONFIG_RA_NETWORK_WORKQUEUE_BH)
	printk("Workqueue");
#endif

#if defined (CONFIG_RAETH_SKB_RECYCLE_2K)
	printk(",SkbRecycle");
#endif
	printk(")\n");


  	ei_local = netdev_priv(dev); // get device pointer from System
	// unsigned int flags;

	if (ei_local == NULL)
	{
		printk(KERN_EMERG "%s: ei_open passed a non-existent device!\n", dev->name);
		return -ENXIO;
	}

        /* receiving packet buffer allocation - NUM_RX_DESC x MAX_RX_LENGTH */
        for ( i = 0; i < NUM_RX_DESC; i++)
        {
#if defined (CONFIG_RAETH_SKB_RECYCLE_2K)
                ei_local->netrx0_skbuf[i] = skbmgr_dev_alloc_skb2k();
#else
                ei_local->netrx0_skbuf[i] = dev_alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN);
#endif
                if (ei_local->netrx0_skbuf[i] == NULL ) {
                        printk("rx skbuff buffer allocation failed!");
		} else {
#if !defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		    skb_reserve(ei_local->netrx0_skbuf[i], NET_IP_ALIGN);
#endif
		}
		

#if defined (CONFIG_RAETH_MULTIPLE_RX_RING) 
		ei_local->netrx1_skbuf[i] = dev_alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN);
                if (ei_local->netrx1_skbuf[i] == NULL ) {
                        printk("rx1 skbuff buffer allocation failed!");
		} else {
#if !defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		    skb_reserve(ei_local->netrx1_skbuf[i], NET_IP_ALIGN);
#endif
		}
#endif
        }


#if defined (CONFIG_GE1_TRGMII_FORCE_1200) && defined (CONFIG_MT7621_ASIC)
	trgmii_set_7621(); //reset FE/GMAC in this function
#endif
	
        fe_dma_init(dev);
	fe_sw_init(); //initialize fe and switch register
	err = request_irq( dev->irq, ei_interrupt, IRQF_DISABLED, dev->name, dev);	// try to fix irq in open
	
	if (err)
	    return err;

	if ( dev->dev_addr != NULL) {
	    ra2880MacAddressSet((void *)(dev->dev_addr));
	} else {
	    printk("dev->dev_addr is empty !\n");
	} 
#if defined (CONFIG_RT_3052_ESW)
	err = request_irq(SURFBOARDINT_ESW, esw_interrupt, IRQF_DISABLED, "Ralink_ESW", dev);
	if (err)
		return err;
	INIT_WORK(&ei_local->kill_sig_wq, kill_sig_workq);
#if defined (CONFIG_RALINK_MT7621)
        mii_mgr_write(31, 0x7008, 0x1f); //enable switch link change intr
	
#else
	*((volatile u32 *)(RALINK_INTCL_BASE + 0x34)) = (1<<17);
	*((volatile u32 *)(ESW_IMR)) &= ~(ESW_INT_ALL);

#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_RT6855A) || \
    defined(CONFIG_RALINK_MT7620)
	*((volatile u32 *)(ESW_P0_IntMn)) &= ~(MSK_CNT_INT_ALL);
	*((volatile u32 *)(ESW_P1_IntMn)) &= ~(MSK_CNT_INT_ALL);
	*((volatile u32 *)(ESW_P2_IntMn)) &= ~(MSK_CNT_INT_ALL);
	*((volatile u32 *)(ESW_P3_IntMn)) &= ~(MSK_CNT_INT_ALL);
	*((volatile u32 *)(ESW_P4_IntMn)) &= ~(MSK_CNT_INT_ALL);
	*((volatile u32 *)(ESW_P5_IntMn)) &= ~(MSK_CNT_INT_ALL);
	*((volatile u32 *)(ESW_P6_IntMn)) &= ~(MSK_CNT_INT_ALL);
#endif
#if    defined(CONFIG_RALINK_MT7620)
	*((volatile u32 *)(ESW_P7_IntMn)) &= ~(MSK_CNT_INT_ALL);
#endif

#endif	
#endif // CONFIG_RT_3052_ESW //

	spin_lock_irqsave(&(ei_local->page_lock), flags);

#ifdef DELAY_INT
        sysRegWrite(RAETH_DLY_INT_CFG, DELAY_INT_INIT);
    	sysRegWrite(RAETH_FE_INT_ENABLE, RAETH_FE_INT_DLY_INIT);
#else
    	sysRegWrite(RAETH_FE_INT_ENABLE, RAETH_FE_INT_ALL);
#endif
#ifdef CONFIG_RAETH_QDMA
#ifdef DELAY_INT
        sysRegWrite(QDMA_DELAY_INT, DELAY_INT_INIT);
    	sysRegWrite(QFE_INT_ENABLE, QFE_INT_DLY_INIT);
#else
    	sysRegWrite(QFE_INT_ENABLE, QFE_INT_ALL);

#endif
#endif

 	INIT_WORK(&ei_local->reset_task, ei_reset_task);

#ifdef WORKQUEUE_BH
#ifndef CONFIG_RAETH_NAPI
 	INIT_WORK(&ei_local->rx_wq, ei_receive_workq);
#endif // CONFIG_RAETH_NAPI //
#else
#ifndef CONFIG_RAETH_NAPI
#if defined (TASKLET_WORKQUEUE_SW)
	working_schedule = init_schedule;
 	INIT_WORK(&ei_local->rx_wq, ei_receive_workq);
	tasklet_init(&ei_local->rx_tasklet, ei_receive_workq, 0);
#else
	tasklet_init(&ei_local->rx_tasklet, ei_receive, 0);
#endif
#endif // CONFIG_RAETH_NAPI //
#endif // WORKQUEUE_BH //

	netif_start_queue(dev);

#ifdef CONFIG_RAETH_NAPI
	atomic_dec(&ei_local->irq_sem);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
        napi_enable(&ei_local->napi);
#else
        netif_poll_enable(dev);
#endif
#endif

	spin_unlock_irqrestore(&(ei_local->page_lock), flags);
#ifdef CONFIG_PSEUDO_SUPPORT
	if(ei_local->PseudoDev == NULL) {
	    RAETH_Init_PSEUDO(ei_local, dev);
	}
 
	if(ei_local->PseudoDev == NULL) 
		printk("Open PseudoDev failed.\n");
	else
		VirtualIF_open(ei_local->PseudoDev);

#endif

#ifdef CONFIG_RAETH_LRO
	lan_ip_tmp = nvram_get(RT2860_NVRAM, "lan_ipaddr");
	str_to_ip(&lan_ip, lan_ip_tmp);
	lro_para.lan_ip1 = lan_ip = htonl(lan_ip);
#endif // CONFIG_RAETH_LRO //

	forward_config(dev);
	return 0;
}

/**
 * ei_close - shut down network device
 * @dev: network device to clear
 *
 * This routine shut down network device.
 *
 *
 */
int ei_close(struct net_device *dev)
{
	int i;
	END_DEVICE *ei_local = netdev_priv(dev);	// device pointer

	netif_stop_queue(dev);
        ra2880stop(ei_local);
	free_irq(dev->irq, dev);
#if defined (CONFIG_RT_3052_ESW)
	free_irq(SURFBOARDINT_ESW, dev);
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	cancel_work_sync(&ei_local->reset_task);
#endif

#ifdef CONFIG_PSEUDO_SUPPORT
	VirtualIF_close(ei_local->PseudoDev);
#endif

#ifdef WORKQUEUE_BH
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	cancel_work_sync(&ei_local->rx_wq);
#endif
#else
#if defined (TASKLET_WORKQUEUE_SW)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	cancel_work_sync(&ei_local->rx_wq);
#endif
#endif
	tasklet_kill(&ei_local->tx_tasklet);
	tasklet_kill(&ei_local->rx_tasklet);
#endif // WORKQUEUE_BH //

        for ( i = 0; i < NUM_RX_DESC; i++)
        {
                if (ei_local->netrx0_skbuf[i] != NULL) {
                        dev_kfree_skb(ei_local->netrx0_skbuf[i]);
			ei_local->netrx0_skbuf[i] = NULL;
		}
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
                if (ei_local->netrx1_skbuf[i] != NULL) {
                        dev_kfree_skb(ei_local->netrx1_skbuf[i]);
			ei_local->netrx1_skbuf[i] = NULL;
		}
#endif
        }

	for ( i = 0; i < NUM_TX_DESC; i++)
	{
		if((ei_local->skb_free[i]!=(struct  sk_buff *)0xFFFFFFFF) && (ei_local->skb_free[i]!= 0))
		{
			dev_kfree_skb_any(ei_local->skb_free[i]);
		}
	}	
	/* TX Ring */
#ifdef CONFIG_RAETH_QDMA
       if (ei_local->txd_pool != NULL) {
	   pci_free_consistent(NULL, NUM_TX_DESC*sizeof(struct QDMA_txdesc), ei_local->txd_pool, ei_local->phy_txd_pool);
       }
       if (ei_local->free_head != NULL){
	       pci_free_consistent(NULL, NUM_QDMA_PAGE * sizeof(struct QDMA_txdesc), ei_local->free_head, ei_local->phy_free_head);
       }
       if (ei_local->free_page_head != NULL){
	       pci_free_consistent(NULL, NUM_QDMA_PAGE * QDMA_PAGE_SIZE, ei_local->free_page_head, ei_local->phy_free_page_head);
       }
#else	
       if (ei_local->tx_ring0 != NULL) {
	   pci_free_consistent(NULL, NUM_TX_DESC*sizeof(struct PDMA_txdesc), ei_local->tx_ring0, ei_local->phy_tx_ring0);
       }
#endif       

#if defined (CONFIG_RAETH_QOS)
       if (ei_local->tx_ring1 != NULL) {
	   pci_free_consistent(NULL, NUM_TX_DESC*sizeof(struct PDMA_txdesc), ei_local->tx_ring1, ei_local->phy_tx_ring1);
       }

#if !defined (CONFIG_RALINK_RT2880)
       if (ei_local->tx_ring2 != NULL) {
	   pci_free_consistent(NULL, NUM_TX_DESC*sizeof(struct PDMA_txdesc), ei_local->tx_ring2, ei_local->phy_tx_ring2);
       }

       if (ei_local->tx_ring3 != NULL) {
	   pci_free_consistent(NULL, NUM_TX_DESC*sizeof(struct PDMA_txdesc), ei_local->tx_ring3, ei_local->phy_tx_ring3);
       }
#endif
#endif
	/* RX Ring */
#ifdef CONFIG_32B_DESC
       kfree(ei_local->rx_ring0);
#else
        pci_free_consistent(NULL, NUM_RX_DESC*sizeof(struct PDMA_rxdesc), ei_local->rx_ring0, ei_local->phy_rx_ring0);
#endif
#ifdef CONFIG_RAETH_QDMA
#ifdef CONFIG_32B_DESC
	kfree(ei_local->qrx_ring);
#else
	pci_free_consistent(NULL, NUM_QRX_DESC*sizeof(struct PDMA_rxdesc), ei_local->qrx_ring, ei_local->phy_qrx_ring);
#endif
#endif	
#if defined (CONFIG_RAETH_MULTIPLE_RX_RING)
#ifdef CONFIG_32B_DESC
	kfree(ei_local->rx_ring1);
#else
        pci_free_consistent(NULL, NUM_RX_DESC*sizeof(struct PDMA_rxdesc), ei_local->rx_ring1, ei_local->phy_rx_ring1);
#endif
#endif

	printk("Free TX/RX Ring Memory!\n");

#ifdef CONFIG_RAETH_NAPI
	atomic_inc(&ei_local->irq_sem);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
        napi_disable(&ei_local->napi);
#else
        netif_poll_disable(dev);
#endif
#endif

	fe_reset();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	module_put(THIS_MODULE);
#else
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

#if defined (CONFIG_RT6855A_FPGA)
void rt6855A_eth_gpio_reset(void)
{
	u8 ether_gpio = 12;

	/* Load the ethernet gpio value to reset Ethernet PHY */
	*(unsigned long *)(RALINK_PIO_BASE + 0x00) |= 1<<(ether_gpio<<1);
	*(unsigned long *)(RALINK_PIO_BASE + 0x14) |= 1<<(ether_gpio);
	*(unsigned long *)(RALINK_PIO_BASE + 0x04) &= ~(1<<ether_gpio);

	udelay(100000);

	*(unsigned long *)(RALINK_PIO_BASE + 0x04) |= (1<<ether_gpio);

	/* must wait for 0.6 seconds after reset*/
	udelay(600000);
}
#endif

#if defined(CONFIG_RALINK_RT6855A)
void rt6855A_gsw_init(void)
{
	u32 phy_val=0;
	u32 rev=0;

#if defined (CONFIG_RT6855A_FPGA)
    /*keep dump switch mode */
    rt6855A_eth_gpio_reset();

    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3000) = 0x5e353;//(P0,Force mode,Link Up,100Mbps,Full-Duplex,FC ON)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3100) = 0x5e353;//(P1,Force mode,Link Up,100Mbps,Full-Duplex,FC ON)
    //*(unsigned long *)(RALINK_ETH_SW_BASE+0x3000) = 0x5e333;//(P0,Force mode,Link Up,10Mbps,Full-Duplex,FC ON)
    //*(unsigned long *)(RALINK_ETH_SW_BASE+0x3100) = 0x5e333;//(P1,Force mode,Link Up,10Mbps,Full-Duplex,FC ON)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3200) = 0x8000;//link down
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3300) = 0x8000;//link down
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3400) = 0x8000;//link down
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x8000;//link down
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3600) = 0x5e33b;//CPU Port6 Force Link 1G, FC ON

    *(unsigned long *)(RALINK_ETH_SW_BASE+0x0010) = 0xffffffe0;//Set Port6 CPU Port

    /* In order to use 10M/Full on FPGA board. We configure phy capable to
     * 10M Full/Half duplex, so we can use auto-negotiation on PC side */
    for(i=6;i<8;i++){
	mii_mgr_write(i, 4, 0x07e1);   //Capable of 10M&100M Full/Half Duplex, flow control on/off
	//mii_mgr_write(i, 4, 0x0461);   //Capable of 10M Full/Half Duplex, flow control on/off
	mii_mgr_write(i, 0, 0xB100);   //reset all digital logic, except phy_reg
	mii_mgr_read(i, 9, &phy_val);
	phy_val &= ~(3<<8); //turn off 1000Base-T Advertisement  (9.9=1000Full, 9.8=1000Half)
	mii_mgr_write(i, 9, phy_val);
    }
#elif defined (CONFIG_RT6855A_ASIC)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3600) = 0x5e33b;//CPU Port6 Force Link 1G, FC ON
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x0010) = 0xffffffe0;//Set Port6 CPU Port

    *(unsigned long *)(RALINK_FRAME_ENGINE_BASE+0x1ec) = 0x0fffffff;//Set PSE should pause 4 tx ring as default
    *(unsigned long *)(RALINK_FRAME_ENGINE_BASE+0x1f0) = 0x0fffffff;//switch IOT more stable
    
    *(unsigned long *)(CKGCR) &= ~(0x3 << 4); //keep rx/tx port clock ticking, disable internal clock-gating to avoid switch stuck 
  
    /*
     *Reg 31: Page Control
     * Bit 15     => PortPageSel, 1=local, 0=global
     * Bit 14:12  => PageSel, local:0~3, global:0~4
     *
     *Reg16~30:Local/Global registers
     *
    */
    /*correct  PHY  setting J8.0*/
    mii_mgr_read(0, 31, &rev);
    rev &= (0x0f);

    mii_mgr_write(1, 31, 0x4000); //global, page 4
  
    mii_mgr_write(1, 16, 0xd4cc);
    mii_mgr_write(1, 17, 0x7444);
    mii_mgr_write(1, 19, 0x0112);
    mii_mgr_write(1, 21, 0x7160);
    mii_mgr_write(1, 22, 0x10cf);
    mii_mgr_write(1, 26, 0x0777);
    
    if(rev == 0){
	    mii_mgr_write(1, 25, 0x0102);
	    mii_mgr_write(1, 29, 0x8641);
    }
    else{
            mii_mgr_write(1, 25, 0x0212);
	    mii_mgr_write(1, 29, 0x4640);
    }

    mii_mgr_write(1, 31, 0x2000); //global, page 2
    mii_mgr_write(1, 21, 0x0655);
    mii_mgr_write(1, 22, 0x0fd3);
    mii_mgr_write(1, 23, 0x003d);
    mii_mgr_write(1, 24, 0x096e);
    mii_mgr_write(1, 25, 0x0fed);
    mii_mgr_write(1, 26, 0x0fc4);
    
    mii_mgr_write(1, 31, 0x1000); //global, page 1
    mii_mgr_write(1, 17, 0xe7f8);
    
    
    mii_mgr_write(1, 31, 0xa000); //local, page 2

    mii_mgr_write(0, 16, 0x0e0e);
    mii_mgr_write(1, 16, 0x0c0c);
    mii_mgr_write(2, 16, 0x0f0f);
    mii_mgr_write(3, 16, 0x1010);
    mii_mgr_write(4, 16, 0x0909);

    mii_mgr_write(0, 17, 0x0000);
    mii_mgr_write(1, 17, 0x0000);
    mii_mgr_write(2, 17, 0x0000);
    mii_mgr_write(3, 17, 0x0000);
    mii_mgr_write(4, 17, 0x0000);
#endif

#if defined (CONFIG_RT6855A_ASIC)

#if defined (CONFIG_P5_RGMII_TO_MAC_MODE)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e33b;//(P5, Force mode, Link Up, 1000Mbps, Full-Duplex, FC ON)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x7014) = 0x1f0c000c;//disable port0-port4 internal phy, set phy base address to 12 
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x250c) = 0x000fff10;//disable port5 mac learning
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x260c) = 0x000fff10;//disable port6 mac learning

#elif defined (CONFIG_P5_MII_TO_MAC_MODE)
    	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e337;//(P5, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
#elif defined (CONFIG_P5_MAC_TO_PHY_MODE)
	//rt6855/6 need to modify TX/RX phase
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x7014) = 0xc;//TX/RX CLOCK Phase select
	
	enable_auto_negotiate(1);

 	if (isICPlusGigaPHY(1)) {
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 4, &phy_val);
		phy_val |= 1<<10; //enable pause ability
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 4, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &phy_val);
		phy_val |= 1<<9; //restart AN
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, phy_val);
	}

 	if (isMarvellGigaPHY(1)) {
		printk("Reset MARVELL phy1\n");
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, &phy_val);
		phy_val |= 1<<7; //Add delay to RX_CLK for RXD Outputs
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &phy_val);
		phy_val |= 1<<15; //PHY Software Reset
	 	mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, phy_val);
        }
	if (isVtssGigaPHY(1)) {
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0x0001); //extended page
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, &phy_val);
		printk("Vitesse phy skew: %x --> ", phy_val);
		phy_val |= (0x3<<12); // RGMII RX skew compensation= 2.0 ns
		phy_val &= ~(0x3<<14);// RGMII TX skew compensation= 0 ns
		printk("%x\n", phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0x0000); //main registers
        }
#elif defined (CONFIG_P5_RMII_TO_MAC_MODE)
    	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e337;//(P5, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
#else // Port 5 Disabled //
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x8000;//link down
#endif
#endif
}
#endif

#if defined (CONFIG_RALINK_MT7621)

void setup_external_gsw(void)
{
	u32	regValue;

	/* reduce RGMII2 PAD driving strength */
	*(volatile u_long *)(PAD_RGMII2_MDIO_CFG) &= ~(0x3 << 4);
	//enable MDIO 
	regValue = le32_to_cpu(*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60));
	regValue &= ~(0x3 << 12);
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) = regValue;

	//RGMII1=Normal mode
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 14);
	//GMAC1= RGMII mode
	*(volatile u_long *)(SYSCFG1) &= ~(0x3 << 12);
	
	sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x00008000);//(GE1, Link down)

	//RGMII2=Normal mode
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 15);
	//GMAC2= RGMII mode
	*(volatile u_long *)(SYSCFG1) &= ~(0x3 << 14);

	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x2005e33b);//(GE2, Force 1000M/FD, FC ON)

}







void IsSwitchVlanTableBusy(void)
{
	int j = 0;
	unsigned int value = 0;

	for (j = 0; j < 20; j++) {
	    mii_mgr_read(31, 0x90, &value);
	    if ((value & 0x80000000) == 0 ){ //table busy
		break;
	    }
	    udelay(70000);
	}
	if (j == 20)
	    printk("set vlan timeout value=0x%x.\n", value);
}

void LANWANPartition(void)
{
/*Set  MT7530 */
#ifdef CONFIG_WAN_AT_P0
	printk("set LAN/WAN WLLLL\n");
	//WLLLL, wan at P0
	//LAN/WAN ports as security mode
	mii_mgr_write(31, 0x2004, 0xff0003);//port0
	mii_mgr_write(31, 0x2104, 0xff0003);//port1
	mii_mgr_write(31, 0x2204, 0xff0003);//port2
	mii_mgr_write(31, 0x2304, 0xff0003);//port3
	mii_mgr_write(31, 0x2404, 0xff0003);//port4

	//set PVID
	mii_mgr_write(31, 0x2014, 0x10002);//port0
	mii_mgr_write(31, 0x2114, 0x10001);//port1
	mii_mgr_write(31, 0x2214, 0x10001);//port2
	mii_mgr_write(31, 0x2314, 0x10001);//port3
	mii_mgr_write(31, 0x2414, 0x10001);//port4
	/*port6 */
	//VLAN member
	IsSwitchVlanTableBusy();
	mii_mgr_write(31, 0x94, 0x407e0001);//VAWD1
	mii_mgr_write(31, 0x90, 0x80001001);//VTCR, VID=1
	IsSwitchVlanTableBusy();

	mii_mgr_write(31, 0x94, 0x40610001);//VAWD1
	mii_mgr_write(31, 0x90, 0x80001002);//VTCR, VID=2
	IsSwitchVlanTableBusy();
#endif
#ifdef CONFIG_WAN_AT_P4
	printk("set LAN/WAN LLLLW\n");
	//LLLLW, wan at P4
	//LAN/WAN ports as security mode
	mii_mgr_write(31, 0x2004, 0xff0003);//port0
	mii_mgr_write(31, 0x2104, 0xff0003);//port1
	mii_mgr_write(31, 0x2204, 0xff0003);//port2
	mii_mgr_write(31, 0x2304, 0xff0003);//port3
	mii_mgr_write(31, 0x2404, 0xff0003);//port4

	//set PVID
	mii_mgr_write(31, 0x2014, 0x10001);//port0
	mii_mgr_write(31, 0x2114, 0x10001);//port1
	mii_mgr_write(31, 0x2214, 0x10001);//port2
	mii_mgr_write(31, 0x2314, 0x10001);//port3
	mii_mgr_write(31, 0x2414, 0x10002);//port4

	//VLAN member
	IsSwitchVlanTableBusy();
	mii_mgr_write(31, 0x94, 0x404f0001);//VAWD1
	mii_mgr_write(31, 0x90, 0x80001001);//VTCR, VID=1
	IsSwitchVlanTableBusy();
	mii_mgr_write(31, 0x94, 0x40500001);//VAWD1
	mii_mgr_write(31, 0x90, 0x80001002);//VTCR, VID=2
	IsSwitchVlanTableBusy();
#endif
}


#if defined (CONFIG_RAETH_8023AZ_EEE) && defined (CONFIG_RALINK_MT7621)
void mt7621_eee_patch(void)
{
	u32 i;

	for(i=0;i<5;i++)
	{
		/* Enable EEE */
		mii_mgr_write(i, 13, 0x07);
		mii_mgr_write(i, 14, 0x3c);
		mii_mgr_write(i, 13, 0x4007);
		mii_mgr_write(i, 14, 0x6);

		/* Forced Slave mode */
		mii_mgr_write(i, 31, 0x0);
		mii_mgr_write(i, 9, 0x1600);
		/* Increase SlvDPSready time */
		mii_mgr_write(i, 31, 0x52b5);
		mii_mgr_write(i, 16, 0xafae);
		mii_mgr_write(i, 18, 0x2f);
		mii_mgr_write(i, 16, 0x8fae);
		/* Incease post_update_timer */
		mii_mgr_write(i, 31, 0x3);
		mii_mgr_write(i, 17, 0x4b);
		/* Adjust 100_mse_threshold */
		mii_mgr_write(i, 13, 0x1e);
		mii_mgr_write(i, 14, 0x123);
		mii_mgr_write(i, 13, 0x401e);
		mii_mgr_write(i, 14, 0xffff);
		/* Disable mcc
		   mii_mgr_write(i, 13, 0x1e);
		   mii_mgr_write(i, 14, 0xa6);
		   mii_mgr_write(i, 13, 0x401e);
		   mii_mgr_write(i, 14, 0x300);
	        */
	}

}
#endif

void setup_internal_gsw(void)
{
	u32	i;
	u32	regValue;

#if defined (CONFIG_GE1_RGMII_FORCE_1000) || defined (CONFIG_GE1_TRGMII_FORCE_1200)
	/*Hardware reset Switch*/
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x34) |= (0x1 << 2);
        udelay(1000);
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x34) &= ~(0x1 << 2);
        udelay(10000);

	/* reduce RGMII2 PAD driving strength */
	*(volatile u_long *)(PAD_RGMII2_MDIO_CFG) &= ~(0x3 << 4);

	//RGMII1=Normal mode
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 14);

	//GMAC1= RGMII mode
	*(volatile u_long *)(SYSCFG1) &= ~(0x3 << 12);

	//enable MDIO to control MT7530
	regValue = le32_to_cpu(*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60));
	regValue &= ~(0x3 << 12);
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) = regValue;

	for(i=0;i<=4;i++)
        {
		//turn off PHY
               mii_mgr_read(i, 0x0 ,&regValue);
	       regValue |= (0x1<<11);
	       mii_mgr_write(i, 0x0, regValue);	
	}
        mii_mgr_write(31, 0x7000, 0x3); //reset switch
        udelay(100);
			                       

#if defined (CONFIG_GE1_TRGMII_FORCE_1200) && defined (CONFIG_MT7621_ASIC)
	trgmii_set_7530();   //reset FE, config MDIO again

	//enable MDIO to control MT7530
	regValue = le32_to_cpu(*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60));
	regValue &= ~(0x3 << 12);
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) = regValue;

	// switch to APLL if TRGMII and DDR2
	if ((sysRegRead(0xBE000010)>>4)&0x1)
	{
		apll_xtal_enable();
	}
#endif

#if defined (CONFIG_MT7621_ASIC)
	if((sysRegRead(0xbe00000c)&0xFFFF)==0x0101) {
		sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x2005e30b);//(GE1, Force 1000M/FD, FC ON)
		mii_mgr_write(31, 0x3600, 0x5e30b);
	} else {
		sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x2005e33b);//(GE1, Force 1000M/FD, FC ON)
		mii_mgr_write(31, 0x3600, 0x5e33b);
	}
#elif defined (CONFIG_MT7621_FPGA)
	sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x2005e337);//(GE1, Force 100M/FD, FC ON)
	mii_mgr_write(31, 0x3600, 0x5e337);
#endif

	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x00008000);//(GE2, Link down)
#endif

#if defined (CONFIG_GE1_RGMII_FORCE_1000) || defined (CONFIG_GE1_TRGMII_FORCE_1200)
	//regValue = 0x117ccf; //Enable Port 6, P5 as GMAC5, P5 disable*/
	mii_mgr_read(31, 0x7804 ,&regValue);
	regValue &= ~(1<<8); //Enable Port 6
	regValue |= (1<<6); //Disable Port 5
	regValue |= (1<<13); //Port 5 as GMAC, no Internal PHY

#if defined (CONFIG_RAETH_GMAC2)
	//RGMII2=Normal mode
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 15);

	//GMAC2= RGMII mode
	*(volatile u_long *)(SYSCFG1) &= ~(0x3 << 14);
#if !defined (CONFIG_RAETH_8023AZ_EEE)	
	mii_mgr_write(31, 0x3500, 0x56300); //MT7530 P5 AN, we can ignore this setting??????
	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x20056300);//(GE2, auto-polling)

	enable_auto_negotiate(0);//set polling address
#endif
#if defined (CONFIG_RAETH_8023AZ_EEE)	
	mii_mgr_write(31, 0x3500, 0x5e33b); //MT7530 P5 Force 1000, we can ignore this setting??????
	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x2005e33b);//(GE2, Force 1000)
#endif



	/* set MT7530 Port 5 to PHY 0/4 mode */
#if defined (CONFIG_GE_RGMII_INTERNAL_P0_AN)
	regValue &= ~((1<<13)|(1<<6));
	regValue |= ((1<<7)|(1<<16)|(1<<20));
#elif defined (CONFIG_GE_RGMII_INTERNAL_P4_AN)
	regValue &= ~((1<<13)|(1<<6)|(1<<20));
	regValue |= ((1<<7)|(1<<16));
#endif
	
#if defined (CONFIG_RAETH_8023AZ_EEE)	
	regValue |= (1<<13); //Port 5 as GMAC, no Internal PHY
#endif
	//sysRegWrite(GDMA2_FWD_CFG, 0x20710000);
#endif
	regValue |= (1<<16);//change HW-TRAP
	printk("change HW-TRAP to 0x%x\n",regValue);
	mii_mgr_write(31, 0x7804 ,regValue);
#endif
	regValue = *(volatile u_long *)(RALINK_SYSCTL_BASE + 0x10);
	regValue = (regValue >> 6) & 0x7;
	if(regValue >= 6) { //25Mhz Xtal
		/* do nothing */
	} else if(regValue >=3) { //40Mhz

	    mii_mgr_write(0, 13, 0x1f);  // disable MT7530 core clock
	    mii_mgr_write(0, 14, 0x410);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x0);

	    mii_mgr_write(0, 13, 0x1f);  // disable MT7530 PLL
	    mii_mgr_write(0, 14, 0x40d);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x2020);

	    mii_mgr_write(0, 13, 0x1f);  // for MT7530 core clock = 500Mhz
	    mii_mgr_write(0, 14, 0x40e);  
	    mii_mgr_write(0, 13, 0x401f);  
	    mii_mgr_write(0, 14, 0x119);   

	    mii_mgr_write(0, 13, 0x1f);  // enable MT7530 PLL
	    mii_mgr_write(0, 14, 0x40d);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x2820);

	    udelay(20); //suggest by CD

	    mii_mgr_write(0, 13, 0x1f);  // enable MT7530 core clock
	    mii_mgr_write(0, 14, 0x410);
	    mii_mgr_write(0, 13, 0x401f);
	}else { //20Mhz Xtal

		/* TODO */

	}
#if defined (CONFIG_GE1_TRGMII_FORCE_1200) && defined (CONFIG_MT7621_ASIC)
	mii_mgr_write(0, 14, 0x3); /*TRGMII*/
#else
	mii_mgr_write(0, 14, 0x1);  /*RGMII*/
/* set MT7530 central align */
        mii_mgr_read(31, 0x7830, &regValue);
        regValue &= ~1;
        regValue |= 1<<1;
        mii_mgr_write(31, 0x7830, regValue);

        mii_mgr_read(31, 0x7a40, &regValue);
        regValue &= ~(1<<30);
        mii_mgr_write(31, 0x7a40, regValue);

        regValue = 0x855;
        mii_mgr_write(31, 0x7a78, regValue);

#endif
#if !defined (CONFIG_RAETH_8023AZ_EEE)	
	mii_mgr_write(31, 0x7b00, 0x102);  //delay setting for 10/1000M
	mii_mgr_write(31, 0x7b04, 0x14);  //delay setting for 10/1000M
#endif
#if 0 
	for(i=0;i<=4;i++) {	
		mii_mgr_read(i, 4, &regValue);
                regValue |= (3<<7); //turn on 100Base-T Advertisement
                //regValue &= ~(3<<7); //turn off 100Base-T Advertisement
		mii_mgr_write(i, 4, regValue);
	
		mii_mgr_read(i, 9, &regValue);
                regValue |= (3<<8); //turn on 1000Base-T Advertisement
		//regValue &= ~(3<<8); //turn off 1000Base-T Advertisement
                mii_mgr_write(i, 9, regValue);

		//restart AN
		mii_mgr_read(i, 0, &regValue);
		regValue |= (1 << 9);
		mii_mgr_write(i, 0, regValue);
	}
#endif

	/*Tx Driving*/
	mii_mgr_write(31, 0x7a54, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a5c, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a64, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a6c, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a74, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a7c, 0x44);  //lower driving


	LANWANPartition();

#if !defined (CONFIG_RAETH_8023AZ_EEE)	
	//disable EEE
	for(i=0;i<=4;i++)
	{
	    mii_mgr_write(i, 13, 0x7);
	    mii_mgr_write(i, 14, 0x3C);
	    mii_mgr_write(i, 13, 0x4007);
	    mii_mgr_write(i, 14, 0x0);
	}

	//Disable EEE 10Base-Te:
	for(i=0;i<=4;i++)
	{
	    mii_mgr_write(i, 13, 0x1f);
	    mii_mgr_write(i, 14, 0x027b);
	    mii_mgr_write(i, 13, 0x401f);
	    mii_mgr_write(i, 14, 0x1177);
	}
#endif

	for(i=0;i<=4;i++)
        {
	//turn on PHY
                mii_mgr_read(i, 0x0 ,&regValue);
	        regValue &= ~(0x1<<11);
	        mii_mgr_write(i, 0x0, regValue);	
	}
	
	mii_mgr_read(31, 0x7808 ,&regValue);
        regValue |= (3<<16); //Enable INTR
	mii_mgr_write(31, 0x7808 ,regValue);
#if defined (CONFIG_RAETH_8023AZ_EEE) && defined (CONFIG_RALINK_MT7621)
	mt7621_eee_patch();
#endif

}

#if defined (CONFIG_GE1_TRGMII_FORCE_1200)
void apll_xtal_enable(void)
{
        unsigned long data = 0;
        unsigned long regValue = 0;

        /* Firstly, reset all required register to default value */
        sysRegWrite(RALINK_ANA_CTRL_BASE, 0x00008000);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x0014, 0x01401d61);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x0018, 0x38233d0e);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, 0x80120004);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x0020, 0x1c7dbf48);

        /* toggle RG_XPTL_CHG */
        sysRegWrite(RALINK_ANA_CTRL_BASE, 0x00008800);
        sysRegWrite(RALINK_ANA_CTRL_BASE, 0x00008c00);

        data = sysRegRead(RALINK_ANA_CTRL_BASE+0x0014);
        data &= ~(0x0000ffc0);

	regValue = *(volatile u_long *)(RALINK_SYSCTL_BASE + 0x10);
	regValue = (regValue >> 6) & 0x7;
	if(regValue < 6) { //20/40Mhz Xtal
		data |= REGBIT(0x1d, 8);
	}else {
		data |= REGBIT(0x17, 8);
	}
	
	if(regValue < 6) { //20/40Mhz Xtal
		data |= REGBIT(0x1, 6);
	}
        
	sysRegWrite(RALINK_ANA_CTRL_BASE+0x0014, data);

        data = sysRegRead(RALINK_ANA_CTRL_BASE+0x0018);
        data &= ~(0xf0773f00);
        data |= REGBIT(0x3, 28);
        data |= REGBIT(0x2, 20);
	if(regValue < 6) { //20/40Mhz Xtal
		data |= REGBIT(0x3, 16);
	}else {
		data |= REGBIT(0x2, 16);
	}
        data |= REGBIT(0x3, 12);

	if(regValue < 6) { //20/40Mhz Xtal
		data |= REGBIT(0xd, 8);
	}else {
		data |= REGBIT(0x7, 8);
	}
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x0018, data);

	if(regValue < 6) { //20/40Mhz Xtal
		sysRegWrite(RALINK_ANA_CTRL_BASE+0x0020, 0x1c7dbf48);
	}else {
		sysRegWrite(RALINK_ANA_CTRL_BASE+0x0020, 0x1697cc39);
	}
	//*Common setting - Set PLLGP_CTRL_4 *//
	///* 1. Bit 31 */
        data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data &= ~(REGBIT(0x1, 31));
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);

	/* 2. Bit 0 */
	data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data |= REGBIT(0x1, 0);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);

	/* 3. Bit 3 */
	data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data |= REGBIT(0x1, 3);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);

	/* 4. Bit 8 */
	data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data |= REGBIT(0x1, 8);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);

	/* 5. Bit 6 */
        data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data |= REGBIT(0x1, 6);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);

	/* 6. Bit 7 */
        data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data |= REGBIT(0x1, 5);
        data |= REGBIT(0x1, 7);
        sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);

	/* 7. Bit 17 */
        data = sysRegRead(RALINK_ANA_CTRL_BASE+0x001c);
        data &= ~REGBIT(0x1, 17);
	sysRegWrite(RALINK_ANA_CTRL_BASE+0x001c, data);
        
	/* 8. TRGMII TX CLK SEL APLL */
	data = sysRegRead(0xbe00002c);
	data &= 0xffffff9f;
	data |= 0x40;
	sysRegWrite(0xbe00002c, data);

}
#endif

#endif
#if defined(CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_MT7620)
void rt_gsw_init(void)
{
#if defined (CONFIG_P4_MAC_TO_PHY_MODE) || defined (CONFIG_P5_MAC_TO_PHY_MODE)
	u32 phy_val=0;
#endif
#if defined (CONFIG_RT6855_FPGA) || defined (CONFIG_MT7620_FPGA)
	u32 i=0;
#elif defined (CONFIG_MT7620_ASIC)
	u32 is_BGA=0;
#endif
#if defined (CONFIG_P5_RGMII_TO_MT7530_MODE)
        unsigned int regValue = 0;
#endif
#if defined (CONFIG_RT6855_FPGA) || defined (CONFIG_MT7620_FPGA)
    /*keep dump switch mode */
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3000) = 0x5e333;//(P0, Force mode, Link Up, 10Mbps, Full-Duplex, FC ON)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3100) = 0x5e333;//(P1, Force mode, Link Up, 10Mbps, Full-Duplex, FC ON)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3200) = 0x5e333;//(P2, Force mode, Link Up, 10Mbps, Full-Duplex, FC ON)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3300) = 0x5e333;//(P3, Force mode, Link Up, 10Mbps, Full-Duplex, FC ON)
#if defined (CONFIG_RAETH_HAS_PORT4)
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3400) = 0x5e337;//(P4, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
#else
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3400) = 0x5e333;//(P4, Force mode, Link Up, 10Mbps, Full-Duplex, FC ON)
#endif
    *(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e337;//(P5, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)

    /* In order to use 10M/Full on FPGA board. We configure phy capable to
     * 10M Full/Half duplex, so we can use auto-negotiation on PC side */
#if defined (CONFIG_RAETH_HAS_PORT4)
    for(i=0;i<4;i++){
#else
    for(i=0;i<5;i++){
#endif
	mii_mgr_write(i, 4, 0x0461);   //Capable of 10M Full/Half Duplex, flow control on/off
	mii_mgr_write(i, 0, 0xB100);   //reset all digital logic, except phy_reg
    }

#endif

#if defined (CONFIG_PDMA_NEW)
    *(unsigned long *)(SYSCFG1) |= (0x1 << 8); //PCIE_RC_MODE=1
#endif


#if defined (CONFIG_MT7620_ASIC) && !defined (CONFIG_P5_RGMII_TO_MT7530_MODE)
    is_BGA = (sysRegRead(RALINK_SYSCTL_BASE + 0xc) >> 16) & 0x1;
    /*
    * Reg 31: Page Control
    * Bit 15     => PortPageSel, 1=local, 0=global
    * Bit 14:12  => PageSel, local:0~3, global:0~4
    *
    * Reg16~30:Local/Global registers
    *
    */
    /*correct  PHY  setting L3.0 BGA*/
    mii_mgr_write(1, 31, 0x4000); //global, page 4
  
    mii_mgr_write(1, 17, 0x7444);
    if(is_BGA){
	mii_mgr_write(1, 19, 0x0114);
    }else{
	mii_mgr_write(1, 19, 0x0117);
    }

    mii_mgr_write(1, 22, 0x10cf);
    mii_mgr_write(1, 25, 0x6212);
    mii_mgr_write(1, 26, 0x0777);
    mii_mgr_write(1, 29, 0x4000);
    mii_mgr_write(1, 28, 0xc077);
    mii_mgr_write(1, 24, 0x0000);
    
    mii_mgr_write(1, 31, 0x3000); //global, page 3
    mii_mgr_write(1, 17, 0x4838);

    mii_mgr_write(1, 31, 0x2000); //global, page 2
    if(is_BGA){
	mii_mgr_write(1, 21, 0x0515);
	mii_mgr_write(1, 22, 0x0053);
	mii_mgr_write(1, 23, 0x00bf);
	mii_mgr_write(1, 24, 0x0aaf);
	mii_mgr_write(1, 25, 0x0fad);
	mii_mgr_write(1, 26, 0x0fc1);
    }else{
	mii_mgr_write(1, 21, 0x0517);
	mii_mgr_write(1, 22, 0x0fd2);
	mii_mgr_write(1, 23, 0x00bf);
	mii_mgr_write(1, 24, 0x0aab);
	mii_mgr_write(1, 25, 0x00ae);
	mii_mgr_write(1, 26, 0x0fff);
    }
    mii_mgr_write(1, 31, 0x1000); //global, page 1
    mii_mgr_write(1, 17, 0xe7f8);
    
    mii_mgr_write(1, 31, 0x8000); //local, page 0
    mii_mgr_write(0, 30, 0xa000);
    mii_mgr_write(1, 30, 0xa000);
    mii_mgr_write(2, 30, 0xa000);
    mii_mgr_write(3, 30, 0xa000);
#if !defined (CONFIG_RAETH_HAS_PORT4)   
    mii_mgr_write(4, 30, 0xa000);
#endif

    mii_mgr_write(0, 4, 0x05e1);
    mii_mgr_write(1, 4, 0x05e1);
    mii_mgr_write(2, 4, 0x05e1);
    mii_mgr_write(3, 4, 0x05e1);
#if !defined (CONFIG_RAETH_HAS_PORT4)   
    mii_mgr_write(4, 4, 0x05e1);
#endif

    mii_mgr_write(1, 31, 0xa000); //local, page 2
    mii_mgr_write(0, 16, 0x1111);
    mii_mgr_write(1, 16, 0x1010);
    mii_mgr_write(2, 16, 0x1515);
    mii_mgr_write(3, 16, 0x0f0f);
#if !defined (CONFIG_RAETH_HAS_PORT4)   
    mii_mgr_write(4, 16, 0x1313);
#endif


#if !defined (CONFIG_RAETH_8023AZ_EEE)	
    mii_mgr_write(1, 31, 0xb000); //local, page 3
    mii_mgr_write(0, 17, 0x0);
    mii_mgr_write(1, 17, 0x0);
    mii_mgr_write(2, 17, 0x0);
    mii_mgr_write(3, 17, 0x0);
#if !defined (CONFIG_RAETH_HAS_PORT4)
    mii_mgr_write(4, 17, 0x0);
#endif
#endif


#endif

#if defined(CONFIG_RALINK_MT7620)
	if ((sysRegRead(0xB000000C) & 0xf) >= 0x5) {
		*(unsigned long *)(RALINK_ETH_SW_BASE+0x701c) = 0x800000c; //enlarge FE2SW_IPG
	}
#endif // CONFIG_RAETH_7620 //



#if defined (CONFIG_MT7620_FPGA)|| defined (CONFIG_MT7620_ASIC)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3600) = 0x5e33b;//CPU Port6 Force Link 1G, FC ON
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x0010) = 0x7f7f7fe0;//Set Port6 CPU Port

#if defined (CONFIG_P5_RGMII_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MT7530_MODE)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e33b;//(P5, Force mode, Link Up, 1000Mbps, Full-Duplex, FC ON)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x7014) = 0x1f0c000c; //disable port 0 ~ 4 internal phy, set phy base address to 12
	/*MT7620 need mac learning for PPE*/
	//*(unsigned long *)(RALINK_ETH_SW_BASE+0x250c) = 0x000fff10;//disable port5 mac learning
	//*(unsigned long *)(RALINK_ETH_SW_BASE+0x260c) = 0x000fff10;//disable port6 mac learning
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
	//rxclk_skew, txclk_skew = 0
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 12); //GE1_MODE=RGMii Mode
#if defined (CONFIG_P5_RGMII_TO_MT7530_MODE)

	*(unsigned long *)(0xb0000060) &= ~(3 << 7); //set MDIO to Normal mode

	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3400) = 0x56330;//(P4, AN)
	*(unsigned long *)(0xb0000060) &= ~(1 << 10); //set GE2 to Normal mode
	//rxclk_skew, txclk_skew = 0
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 14); //GE2_MODE=RGMii Mode


	/* set MT7530 Port 0 to PHY mode */
	mii_mgr_read(31, 0x7804 ,&regValue);
#if defined (CONFIG_GE_RGMII_MT7530_P0_AN)
	regValue &= ~((1<<13)|(1<<6)|(1<<5)|(1<<15));
	regValue |= ((1<<7)|(1<<16)|(1<<20)|(1<<24));
	//mii_mgr_write(31, 0x7804 ,0x115c8f);
#elif defined (CONFIG_GE_RGMII_MT7530_P4_AN)
	regValue &= ~((1<<13)|(1<<6)|(1<<20)|(1<<5)|(1<<15));
	regValue |= ((1<<7)|(1<<16)|(1<<24));
#endif
	regValue &= ~(1<<8); //Enable Port 6
	mii_mgr_write(31, 0x7804 ,regValue); //bit 24 standalone switch

/* set MT7530 central align */
        mii_mgr_read(31, 0x7830, &regValue);
        regValue &= ~1;
        regValue |= 1<<1;
        mii_mgr_write(31, 0x7830, regValue);

        mii_mgr_read(31, 0x7a40, &regValue);
        regValue &= ~(1<<30);
        mii_mgr_write(31, 0x7a40, regValue);

        regValue = 0x855;
        mii_mgr_write(31, 0x7a78, regValue);

	/*AN should be set after MT7530 HWSTRAP*/
#if defined (CONFIG_GE_RGMII_MT7530_P0_AN)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x7000) = 0xc5000100;//(P0, AN polling)
#elif defined (CONFIG_GE_RGMII_MT7530_P4_AN)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x7000) = 0xc5000504;//(P4, AN polling)
#endif
#endif

#elif defined (CONFIG_P5_MII_TO_MAC_MODE)
    	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e337;//(P5, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 12); //GE1_MODE=Mii Mode
	*(unsigned long *)(SYSCFG1) |= (0x1 << 12);

#elif defined (CONFIG_P5_MAC_TO_PHY_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
	*(unsigned long *)(0xb0000060) &= ~(3 << 7); //set MDIO to Normal mode
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 12); //GE1_MODE=RGMii Mode
	
	enable_auto_negotiate(1);

 	if (isICPlusGigaPHY(1)) {
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 4, &phy_val);
		phy_val |= 1<<10; //enable pause ability
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 4, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &phy_val);
		phy_val |= 1<<9; //restart AN
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, phy_val);
	}else if (isMarvellGigaPHY(1)) {
#if defined (CONFIG_MT7620_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, &phy_val);
		phy_val &= ~(3<<8); //turn off 1000Base-T Advertisement  (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, phy_val);
#endif
		printk("Reset MARVELL phy1\n");
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, &phy_val);
		phy_val |= 1<<7; //Add delay to RX_CLK for RXD Outputs
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &phy_val);
		phy_val |= 1<<15; //PHY Software Reset
	 	mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, phy_val);
        }else if (isVtssGigaPHY(1)) {
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0x0001); //extended page
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, &phy_val);
		printk("Vitesse phy skew: %x --> ", phy_val);
		phy_val |= (0x3<<12); // RGMII RX skew compensation= 2.0 ns
		phy_val &= ~(0x3<<14);// RGMII TX skew compensation= 0 ns
		printk("%x\n", phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0x0000); //main registers
        }


#elif defined (CONFIG_P5_RMII_TO_MAC_MODE)
    	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x5e337;//(P5, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 12); //GE1_MODE=RvMii Mode
	*(unsigned long *)(SYSCFG1) |= (0x2 << 12);

#else // Port 5 Disabled //
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x3500) = 0x8000;//link down
	*(unsigned long *)(0xb0000060) |= (1 << 9); //set RGMII to GPIO mode
#endif
#endif

#if defined (CONFIG_P4_RGMII_TO_MAC_MODE)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3400) = 0x5e33b;//(P4, Force mode, Link Up, 1000Mbps, Full-Duplex, FC ON)
	*(unsigned long *)(0xb0000060) &= ~(1 << 10); //set GE2 to Normal mode
	//rxclk_skew, txclk_skew = 0
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 14); //GE2_MODE=RGMii Mode

#elif defined (CONFIG_P4_MII_TO_MAC_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 10); //set GE2 to Normal mode
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 14); //GE2_MODE=Mii Mode
	*(unsigned long *)(SYSCFG1) |= (0x1 << 14);

#elif defined (CONFIG_P4_MAC_TO_PHY_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 10); //set GE2 to Normal mode
	*(unsigned long *)(0xb0000060) &= ~(3 << 7); //set MDIO to Normal mode
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 14); //GE2_MODE=RGMii Mode

	enable_auto_negotiate(1);
 
	if (isICPlusGigaPHY(2)) {
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 4, &phy_val);
		phy_val |= 1<<10; //enable pause ability
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 4, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, &phy_val);
		phy_val |= 1<<9; //restart AN
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, phy_val);
	}else if (isMarvellGigaPHY(2)) {
#if defined (CONFIG_MT7620_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 9, &phy_val);
		phy_val &= ~(3<<8); //turn off 1000Base-T Advertisement  (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 9, phy_val);
#endif
		printk("Reset MARVELL phy2\n");
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 20, &phy_val);
		phy_val |= 1<<7; //Add delay to RX_CLK for RXD Outputs
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 20, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, &phy_val);
		phy_val |= 1<<15; //PHY Software Reset
	 	mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, phy_val);
        }else if (isVtssGigaPHY(2)) {
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 31, 0x0001); //extended page
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 28, &phy_val);
		printk("Vitesse phy skew: %x --> ", phy_val);
		phy_val |= (0x3<<12); // RGMII RX skew compensation= 2.0 ns
		phy_val &= ~(0x3<<14);// RGMII TX skew compensation= 0 ns
		printk("%x\n", phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 28, phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 31, 0x0000); //main registers
        }

#elif defined (CONFIG_P4_RMII_TO_MAC_MODE)
    	*(unsigned long *)(RALINK_ETH_SW_BASE+0x3400) = 0x5e337;//(P5, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
	*(unsigned long *)(0xb0000060) &= ~(1 << 10); //set GE2 to Normal mode
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 14); //GE1_MODE=RvMii Mode
	*(unsigned long *)(SYSCFG1) |= (0x2 << 14);
#elif defined (CONFIG_GE_RGMII_MT7530_P0_AN) || defined (CONFIG_GE_RGMII_MT7530_P4_AN)
#else // Port 4 Disabled //
        *(unsigned long *)(SYSCFG1) |= (0x3 << 14); //GE2_MODE=RJ45 Mode
	*(unsigned long *)(0xb0000060) |= (1 << 10); //set RGMII2 to GPIO mode
#endif

}
#endif

#if defined (CONFIG_RALINK_MT7628)

void mt7628_ephy_init(void)
{
	int i;
	u32 phy_val;
	mii_mgr_write(0, 31, 0x2000); //change G2 page
	mii_mgr_write(0, 26, 0x0000);

	for(i=0; i<5; i++){
		mii_mgr_write(i, 31, 0x8000); //change L0 page
		mii_mgr_write(i,  0, 0x3100);

#if defined (CONFIG_RAETH_8023AZ_EEE)	
		mii_mgr_read(i, 26, &phy_val);// EEE setting
		phy_val |= (1 << 5);
		mii_mgr_write(i, 26, phy_val);
#else
		//disable EEE
		mii_mgr_write(i, 13, 0x7);
		mii_mgr_write(i, 14, 0x3C);
		mii_mgr_write(i, 13, 0x4007);
		mii_mgr_write(i, 14, 0x0);
#endif
		mii_mgr_write(i, 30, 0xa000);
		mii_mgr_write(i, 31, 0xa000); // change L2 page
		mii_mgr_write(i, 16, 0x0606);
		mii_mgr_write(i, 23, 0x0f0e);
		mii_mgr_write(i, 24, 0x1610);
		mii_mgr_write(i, 30, 0x1f15);
		mii_mgr_write(i, 28, 0x6111);

		mii_mgr_read(i, 4, &phy_val);
		phy_val |= (1 << 10);
		mii_mgr_write(i, 4, phy_val);
	}

        //100Base AOI setting
	mii_mgr_write(0, 31, 0x5000);  //change G5 page
	mii_mgr_write(0, 19, 0x004a);
	mii_mgr_write(0, 20, 0x015a);
	mii_mgr_write(0, 21, 0x00ee);
	mii_mgr_write(0, 22, 0x0033);
	mii_mgr_write(0, 23, 0x020a);
	mii_mgr_write(0, 24, 0x0000);
	mii_mgr_write(0, 25, 0x024a);
	mii_mgr_write(0, 26, 0x035a);
	mii_mgr_write(0, 27, 0x02ee);
	mii_mgr_write(0, 28, 0x0233);
	mii_mgr_write(0, 29, 0x000a);
	mii_mgr_write(0, 30, 0x0000);
	/* Fix EPHY idle state abnormal behavior */	
	mii_mgr_write(0, 31, 0x4000); //change G4 page
	mii_mgr_write(0, 29, 0x000d);
	mii_mgr_write(0, 30, 0x0500);
	
}

#endif


#if defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT3352) || defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_MT7628)
void rt305x_esw_init(void)
{
	int i=0;
	u32 phy_val=0, val=0;
#if defined (CONFIG_RT3052_ASIC)
	u32 phy_val2;
#endif

#if defined (CONFIG_RT5350_ASIC)
	*(unsigned long *)(RALINK_ETH_SW_BASE+0x0168) = 0x17;
#endif

	/*
	 * FC_RLS_TH=200, FC_SET_TH=160
	 * DROP_RLS=120, DROP_SET_TH=80
	 */
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0008) = 0xC8A07850;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00E4) = 0x00000000;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0014) = 0x00405555;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0050) = 0x00002001;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0090) = 0x00007f7f;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0098) = 0x00007f3f; //disable VLAN
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00CC) = 0x0002500c;
#ifndef CONFIG_UNH_TEST
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x009C) = 0x0008a301; //hashing algorithm=XOR48, aging interval=300sec
#else
	/*
	 * bit[30]:1	Backoff Algorithm Option: The latest one to pass UNH test
	 * bit[29]:1	Length of Received Frame Check Enable
	 * bit[8]:0	Enable collision 16 packet abort and late collision abort
	 * bit[7:6]:01	Maximum Packet Length: 1518
	 */
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x009C) = 0x6008a241;
#endif
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x008C) = 0x02404040;
#if defined (CONFIG_RT3052_ASIC) || defined (CONFIG_RT3352_ASIC) || defined (CONFIG_RT5350_ASIC) || defined (CONFIG_MT7628_ASIC)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) = 0x3f502b28; //Change polling Ext PHY Addr=0x1F
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0084) = 0x00000000;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0110) = 0x7d000000; //1us cycle number=125 (FE's clock=125Mhz)
#elif defined (CONFIG_RT3052_FPGA) || defined (CONFIG_RT3352_FPGA) || defined (CONFIG_RT5350_FPGA) || defined (CONFIG_MT7628_FPGA)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) = 0x00f03ff9; //polling Ext PHY Addr=0x0, force port5 as 100F/D (disable auto-polling)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0084) = 0xffdf1f00;
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x0110) = 0x0d000000; //1us cycle number=13 (FE's clock=12.5Mhz)

	/* In order to use 10M/Full on FPGA board. We configure phy capable to
	 * 10M Full/Half duplex, so we can use auto-negotiation on PC side */
        for(i=0;i<5;i++){
	    mii_mgr_write(i, 4, 0x0461);   //Capable of 10M Full/Half Duplex, flow control on/off
	    mii_mgr_write(i, 0, 0xB100);   //reset all digital logic, except phy_reg
	}
#endif
	
	/*
	 * set port 5 force to 1000M/Full when connecting to switch or iNIC
	 */
#if defined (CONFIG_P5_RGMII_TO_MAC_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(1<<29); //disable port 5 auto-polling
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) |= 0x3fff; //force 1000M full duplex
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(0xf<<20); //rxclk_skew, txclk_skew = 0
#elif defined (CONFIG_P5_MII_TO_MAC_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(1<<29); //disable port 5 auto-polling
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(0x3fff); 
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) |= 0x3ffd; //force 100M full duplex

#if defined (CONFIG_RALINK_RT3352)
        *(unsigned long *)(SYSCFG1) &= ~(0x3 << 12); //GE1_MODE=Mii Mode
        *(unsigned long *)(SYSCFG1) |= (0x1 << 12);
#endif

#elif defined (CONFIG_P5_MAC_TO_PHY_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
	*(unsigned long *)(0xb0000060) &= ~(1 << 7); //set MDIO to Normal mode
#if defined (CONFIG_RT3052_ASIC) || defined (CONFIG_RT3352_ASIC)
	enable_auto_negotiate(1);
#endif
        if (isMarvellGigaPHY(1)) {
#if defined (CONFIG_RT3052_FPGA) || defined (CONFIG_RT3352_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, &phy_val);
		phy_val &= ~(3<<8); //turn off 1000Base-T Advertisement  (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, phy_val);
#endif
		printk("\n Reset MARVELL phy\n");
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, &phy_val);
		phy_val |= 1<<7; //Add delay to RX_CLK for RXD Outputs
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, phy_val);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &phy_val);
		phy_val |= 1<<15; //PHY Software Reset
	 	mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, phy_val);
        }
	if (isVtssGigaPHY(1)) {
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0x0001); //extended page
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, &phy_val);
		printk("Vitesse phy skew: %x --> ", phy_val);
		phy_val |= (0x3<<12); // RGMII RX skew compensation= 2.0 ns
		phy_val &= ~(0x3<<14);// RGMII TX skew compensation= 0 ns
		printk("%x\n", phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, phy_val);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0x0000); //main registers
        }
       
#elif defined (CONFIG_P5_RMII_TO_MAC_MODE)
	*(unsigned long *)(0xb0000060) &= ~(1 << 9); //set RGMII to Normal mode
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(1<<29); //disable port 5 auto-polling
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(0x3fff); 
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) |= 0x3ffd; //force 100M full duplex
        
#if defined (CONFIG_RALINK_RT3352)
	*(unsigned long *)(SYSCFG1) &= ~(0x3 << 12); //GE1_MODE=RvMii Mode
        *(unsigned long *)(SYSCFG1) |= (0x2 << 12);
#endif
#else // Port 5 Disabled //

#if defined (CONFIG_RALINK_RT3052)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(1 << 29); //port5 auto polling disable
        *(unsigned long *)(0xb0000060) |= (1 << 7); //set MDIO to GPIO mode (GPIO22-GPIO23)
        *(unsigned long *)(0xb0000060) |= (1 << 9); //set RGMII to GPIO mode (GPIO41-GPIO50)
        *(unsigned long *)(0xb0000674) = 0xFFF; //GPIO41-GPIO50 output mode
        *(unsigned long *)(0xb000067C) = 0x0; //GPIO41-GPIO50 output low
#elif defined (CONFIG_RALINK_RT3352)
        *(unsigned long *)(RALINK_ETH_SW_BASE+0x00C8) &= ~(1 << 29); //port5 auto polling disable
	*(unsigned long *)(0xb0000060) |= (1 << 7); //set MDIO to GPIO mode (GPIO22-GPIO23)
        *(unsigned long *)(0xb0000624) = 0xC0000000; //GPIO22-GPIO23 output mode
        *(unsigned long *)(0xb000062C) = 0xC0000000; //GPIO22-GPIO23 output high
        
        *(unsigned long *)(0xb0000060) |= (1 << 9); //set RGMII to GPIO mode (GPIO24-GPIO35)
	*(unsigned long *)(0xb000064C) = 0xFFF; //GPIO24-GPIO35 output mode
        *(unsigned long *)(0xb0000654) = 0xFFF; //GPIO24-GPIO35 output high
#elif defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_MT7628)
	/* do nothing */
#endif
#endif // CONFIG_P5_RGMII_TO_MAC_MODE //


#if defined (CONFIG_RT3052_ASIC)
	rw_rf_reg(0, 0, &phy_val);
        phy_val = phy_val >> 4;

        if(phy_val > 0x5) {

            rw_rf_reg(0, 26, &phy_val);
            phy_val2 = (phy_val | (0x3 << 5));
            rw_rf_reg(1, 26, &phy_val2);

			// reset EPHY
			val = sysRegRead(RSTCTRL);
			val = val | RALINK_EPHY_RST;
			sysRegWrite(RSTCTRL, val);
			val = val & ~(RALINK_EPHY_RST);
			sysRegWrite(RSTCTRL, val);

            rw_rf_reg(1, 26, &phy_val);

            //select local register
            mii_mgr_write(0, 31, 0x8000);
            for(i=0;i<5;i++){
                mii_mgr_write(i, 26, 0x1600);   //TX10 waveform coefficient //LSB=0 disable PHY
                mii_mgr_write(i, 29, 0x7058);   //TX100/TX10 AD/DA current bias
                mii_mgr_write(i, 30, 0x0018);   //TX100 slew rate control
            }

            //select global register
            mii_mgr_write(0, 31, 0x0);
            mii_mgr_write(0,  1, 0x4a40); //enlarge agcsel threshold 3 and threshold 2
            mii_mgr_write(0,  2, 0x6254); //enlarge agcsel threshold 5 and threshold 4
            mii_mgr_write(0,  3, 0xa17f); //enlarge agcsel threshold 6
//#define ENABLE_LDPS
#if defined (ENABLE_LDPS)
            mii_mgr_write(0, 12, 0x7eaa);
            mii_mgr_write(0, 22, 0x252f); //tune TP_IDL tail and head waveform, enable power down slew rate control
#else
            mii_mgr_write(0, 12, 0x0);
            mii_mgr_write(0, 22, 0x052f);
#endif
            mii_mgr_write(0, 14, 0x65);   //longer TP_IDL tail length
            mii_mgr_write(0, 16, 0x0684); //increased squelch pulse count threshold.
            mii_mgr_write(0, 17, 0x0fe0); //set TX10 signal amplitude threshold to minimum
            mii_mgr_write(0, 18, 0x40ba); //set squelch amplitude to higher threshold
            mii_mgr_write(0, 27, 0x2fce); //set PLL/Receive bias current are calibrated
            mii_mgr_write(0, 28, 0xc410); //change PLL/Receive bias current to internal(RT3350)
            mii_mgr_write(0, 29, 0x598b); //change PLL bias current to internal(RT3052_MP3)
	    mii_mgr_write(0, 31, 0x8000); //select local register

            for(i=0;i<5;i++){
                //LSB=1 enable PHY
                mii_mgr_read(i, 26, &phy_val);
                phy_val |= 0x0001;
                mii_mgr_write(i, 26, phy_val);
            }
	} else {
	    //select local register
            mii_mgr_write(0, 31, 0x8000);
            for(i=0;i<5;i++){
                mii_mgr_write(i, 26, 0x1600);   //TX10 waveform coefficient //LSB=0 disable PHY
                mii_mgr_write(i, 29, 0x7058);   //TX100/TX10 AD/DA current bias
                mii_mgr_write(i, 30, 0x0018);   //TX100 slew rate control
            }

            //select global register
            mii_mgr_write(0, 31, 0x0);
            mii_mgr_write(0,  1, 0x4a40); //enlarge agcsel threshold 3 and threshold 2
            mii_mgr_write(0,  2, 0x6254); //enlarge agcsel threshold 5 and threshold 4
            mii_mgr_write(0,  3, 0xa17f); //enlarge agcsel threshold 6
            mii_mgr_write(0, 14, 0x65);   //longer TP_IDL tail length
            mii_mgr_write(0, 16, 0x0684); //increased squelch pulse count threshold.
            mii_mgr_write(0, 17, 0x0fe0); //set TX10 signal amplitude threshold to minimum
            mii_mgr_write(0, 18, 0x40ba); //set squelch amplitude to higher threshold
            mii_mgr_write(0, 22, 0x052f); //tune TP_IDL tail and head waveform
            mii_mgr_write(0, 27, 0x2fce); //set PLL/Receive bias current are calibrated
            mii_mgr_write(0, 28, 0xc410); //change PLL/Receive bias current to internal(RT3350)
	    mii_mgr_write(0, 29, 0x598b); //change PLL bias current to internal(RT3052_MP3)
	    mii_mgr_write(0, 31, 0x8000); //select local register

            for(i=0;i<5;i++){
                //LSB=1 enable PHY
                mii_mgr_read(i, 26, &phy_val);
                phy_val |= 0x0001;
                mii_mgr_write(i, 26, phy_val);
            }
	}
#elif defined (CONFIG_RT3352_ASIC)
	//PHY IOT
	// reset EPHY
	val = sysRegRead(RSTCTRL);
	val = val | RALINK_EPHY_RST;
	sysRegWrite(RSTCTRL, val);
	val = val & ~(RALINK_EPHY_RST);
	sysRegWrite(RSTCTRL, val);

	//select local register
        mii_mgr_write(0, 31, 0x8000);
        for(i=0;i<5;i++){
            mii_mgr_write(i, 26, 0x1600);   //TX10 waveform coefficient //LSB=0 disable PHY
            mii_mgr_write(i, 29, 0x7016);   //TX100/TX10 AD/DA current bias
            mii_mgr_write(i, 30, 0x0038);   //TX100 slew rate control
        }

        //select global register
        mii_mgr_write(0, 31, 0x0);
        mii_mgr_write(0,  1, 0x4a40); //enlarge agcsel threshold 3 and threshold 2
        mii_mgr_write(0,  2, 0x6254); //enlarge agcsel threshold 5 and threshold 4
        mii_mgr_write(0,  3, 0xa17f); //enlarge agcsel threshold 6
        mii_mgr_write(0, 12, 0x7eaa);
        mii_mgr_write(0, 14, 0x65);   //longer TP_IDL tail length
        mii_mgr_write(0, 16, 0x0684); //increased squelch pulse count threshold.
        mii_mgr_write(0, 17, 0x0fe0); //set TX10 signal amplitude threshold to minimum
        mii_mgr_write(0, 18, 0x40ba); //set squelch amplitude to higher threshold
        mii_mgr_write(0, 22, 0x253f); //tune TP_IDL tail and head waveform, enable power down slew rate control
        mii_mgr_write(0, 27, 0x2fda); //set PLL/Receive bias current are calibrated
        mii_mgr_write(0, 28, 0xc410); //change PLL/Receive bias current to internal(RT3350)
        mii_mgr_write(0, 29, 0x598b); //change PLL bias current to internal(RT3052_MP3)
        mii_mgr_write(0, 31, 0x8000); //select local register

        for(i=0;i<5;i++){
            //LSB=1 enable PHY
            mii_mgr_read(i, 26, &phy_val);
            phy_val |= 0x0001;
            mii_mgr_write(i, 26, phy_val);
        }

#elif defined (CONFIG_RT5350_ASIC)
	//PHY IOT
	// reset EPHY
	val = sysRegRead(RSTCTRL);
	val = val | RALINK_EPHY_RST;
	sysRegWrite(RSTCTRL, val);
	val = val & ~(RALINK_EPHY_RST);
	sysRegWrite(RSTCTRL, val);

	//select local register
        mii_mgr_write(0, 31, 0x8000);
        for(i=0;i<5;i++){
            mii_mgr_write(i, 26, 0x1600);   //TX10 waveform coefficient //LSB=0 disable PHY
            mii_mgr_write(i, 29, 0x7015);   //TX100/TX10 AD/DA current bias
            mii_mgr_write(i, 30, 0x0038);   //TX100 slew rate control
        }

        //select global register
        mii_mgr_write(0, 31, 0x0);
        mii_mgr_write(0,  1, 0x4a40); //enlarge agcsel threshold 3 and threshold 2
        mii_mgr_write(0,  2, 0x6254); //enlarge agcsel threshold 5 and threshold 4
        mii_mgr_write(0,  3, 0xa17f); //enlarge agcsel threshold 6
        mii_mgr_write(0, 12, 0x7eaa);
        mii_mgr_write(0, 14, 0x65);   //longer TP_IDL tail length
        mii_mgr_write(0, 16, 0x0684); //increased squelch pulse count threshold.
        mii_mgr_write(0, 17, 0x0fe0); //set TX10 signal amplitude threshold to minimum
        mii_mgr_write(0, 18, 0x40ba); //set squelch amplitude to higher threshold
        mii_mgr_write(0, 22, 0x253f); //tune TP_IDL tail and head waveform, enable power down slew rate control
        mii_mgr_write(0, 27, 0x2fda); //set PLL/Receive bias current are calibrated
        mii_mgr_write(0, 28, 0xc410); //change PLL/Receive bias current to internal(RT3350)
        mii_mgr_write(0, 29, 0x598b); //change PLL bias current to internal(RT3052_MP3)
        mii_mgr_write(0, 31, 0x8000); //select local register

        for(i=0;i<5;i++){
            //LSB=1 enable PHY
            mii_mgr_read(i, 26, &phy_val);
            phy_val |= 0x0001;
            mii_mgr_write(i, 26, phy_val);
        }
#elif defined (CONFIG_MT7628_ASIC)
/*INIT MT7628 PHY HERE*/
	val = sysRegRead(RT2880_AGPIOCFG_REG);
#if defined (CONFIG_ETH_ONE_PORT_ONLY)
	val |= (MT7628_P0_EPHY_AIO_EN | MT7628_P1_EPHY_AIO_EN | MT7628_P2_EPHY_AIO_EN | MT7628_P3_EPHY_AIO_EN | MT7628_P4_EPHY_AIO_EN);
	val = val & ~(MT7628_P0_EPHY_AIO_EN);
#else
	val = val & ~(MT7628_P0_EPHY_AIO_EN | MT7628_P1_EPHY_AIO_EN | MT7628_P2_EPHY_AIO_EN | MT7628_P3_EPHY_AIO_EN | MT7628_P4_EPHY_AIO_EN);
#endif
        if ((*((volatile u32 *)(RALINK_SYSCTL_BASE + 0x8))) & 0x10000)
		val &= ~0x1f0000;
	sysRegWrite(RT2880_AGPIOCFG_REG, val);

	val = sysRegRead(RSTCTRL);
	val = val | RALINK_EPHY_RST;
	sysRegWrite(RSTCTRL, val);
	val = val & ~(RALINK_EPHY_RST);
	sysRegWrite(RSTCTRL, val);

#if !defined(CONFIG_TP_MODEL_C50V4) && !defined(CONFIG_TP_MODEL_MR6400V3) && !defined(CONFIG_TP_MODEL_MR400V4) && !defined (CONFIG_TP_MODEL_MR6400V5) &&!defined (CONFIG_TP_MODEL_MR200V5) && !defined(CONFIG_TP_MODEL_MR100V1) && !defined(CONFIG_TP_MODEL_MR100V2) && !defined(CONFIG_TP_MODEL_MB110V1)
	val = sysRegRead(RALINK_SYSCTL_BASE + 0x64);
	//printk("before---rt305x_esw_init() val=0x%08x", val);
#if defined (CONFIG_ETH_ONE_PORT_ONLY)
	val &= 0xf003f003;
	val |= 0x05540554;
	sysRegWrite(RALINK_SYSCTL_BASE + 0x64, val); // set P0 EPHY LED mode
#else
	val &= 0xf003f003;
	sysRegWrite(RALINK_SYSCTL_BASE + 0x64, val); // set P0~P4 EPHY LED mode
#endif
#endif
	//val = sysRegRead(RALINK_SYSCTL_BASE + 0x64);
	//printk("after---rt305x_esw_init() val=0x%08x", val);
	udelay(5000);
	mt7628_ephy_init();

#endif
}
#endif

/* define in ralink_gpio, yuanshang, 2013-11-16 */
extern int (*fp_gpioTestLan)(void);
extern int (*fp_ethTestWan)(void);
extern int (*fp_gpioTestLanOffWan)(void);

void enableEthForward(void)
{
	u32 portControl0 = 0x90;
	int regVal = 0;

	regVal = *(unsigned long *)(RALINK_ETH_SW_BASE + portControl0);
	regVal &= ~(0x1f << 23); //enable 23~27 bit for 5 ports

	*(unsigned long *)(RALINK_ETH_SW_BASE + portControl0) = regVal;

	printk("enable switch phyport...\n");

}

void disableEthForward(void)
{
	u32 portControl0 = 0x90;
	int regVal = 0;

	regVal = *(unsigned long *)(RALINK_ETH_SW_BASE + portControl0);
	regVal |= (0x1f << 23); //disable 23~27 bit for 5 ports

	*(unsigned long *)(RALINK_ETH_SW_BASE + portControl0) = regVal;


	printk("disable switch phyport...\n");

}

int isEthForwardEnable()
{
	u32 portControl0 = 0x90;

	u32 data = 0;
	u32 portData = 0;
	u32 portBit = 0;

	data = *(unsigned long *)(RALINK_ETH_SW_BASE + portControl0);
	portData = data >> 23;

	while (portBit < 5)  // 5 ports
	{
		if (portData & (1 << portBit))
		{
			return 0;
		}
		portBit ++;
	}
	
	return 1;

}


static int eth_forward_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;
	if (isEthForwardEnable())
	{
		len = sprintf(page, "%d\n", 1);
	}
	else
	{
    	len = sprintf(page, "%d\n", 0);
	}

    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int eth_forward_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
	if (val)
	{
		enableEthForward();
	}
	else
	{
		disableEthForward();
	}
	

	return count;
}

static int eth_wanStatus_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;
    int stat;
    if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
	{
		len = sprintf(page, "%d\n", 1);
	}
    else
	{
		len = sprintf(page, "%d\n", 0);
	}
    
    
    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}
static int eth_wanStatus_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	printk("/proc/tplink/eth_wanPort_status is read only\n");
	return count;
}


void init_raeth_tplink(void)
{

	struct proc_dir_entry *eth_proc;

	eth_proc = create_proc_entry("tplink/eth_forward", 0, NULL);
	eth_proc->read_proc = eth_forward_read_proc;
	eth_proc->write_proc = eth_forward_write_proc;

	eth_proc = create_proc_entry("tplink/eth_wanPort_status", 0, NULL);
	eth_proc->read_proc = eth_wanStatus_read_proc;
	eth_proc->write_proc = eth_wanStatus_write_proc;

	return;
}

#if defined(CONFIG_TP_MODEL_MR6400V3) || defined(CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1) || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
u32 rt_mt7628_chkLan(void)	/* MB110V1 means LAN1*/
{
	u32 regData = 0;
	int count = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	//printk("checkLan---regData=0x%08x", regData);
#if defined (CONFIG_TP_MODEL_MR400V4) || defined(CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
	if (regData & (0xF << 25))
#else
	if (regData & (0x1 << 28))	/* MB110V1 LAN2/WAN at bit24 */
#endif
	{
		return 1;
	}

	return 0;
}

u32 rt_mt7628_chkLanOffWan(void)
{
	u32 regData = 0;
	int count = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	//printk("checkLanOffWan---regData=0x%08x", regData);
#if defined (CONFIG_TP_MODEL_MR400V4) || defined(CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
	if (regData & (0x7 << 25))
#else
	if (regData & (0x7 << 26))
#endif
	{
		return 1;
	}

	return 0;
}


u32 rt_mt7628_chkWan(void)	/* MB110V1 means LAN2/WAN */
{
	u32 regData = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	//printk("checkWan---regData=0x%08x", regData);
#ifdef CONFIG_WAN_AT_P0
	if (regData & (0x1 << 25))
#else
#if defined (CONFIG_TP_MODEL_MR400V4) || defined(CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
	if (regData & (0x1 << 28))
#else
	if (regData & (0x1 << 29))	/* MB110V1 LAN2/WAN at bit25 */
#endif
#endif
	{
		return 1;
	}
	return 0;
}
#endif

#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_C50V4) || defined(CONFIG_TP_MODEL_MR3420V5) || defined(CONFIG_TP_MODEL_MR3020V3)

/* For C20i port1~4 is LAN */
u32 rt_mt7628_chkLan(void)
{
	u32 regData = 0;
	int count = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	//printk("[checkLan---regData=0x%08x", regData);
	if (regData & (0xF << 26))
	{
		return 1;
	}

	return 0;
}
/* For C20i port0 is WAN */
u32 rt_mt7628_chkWan(void)
{
	u32 regData = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	//printk("checkWan---regData=0x%08x", regData);
	if (regData & (0x1 << 25))
	{
		return 1;
	}
	return 0;
}
#endif /* CONFIG_TP_MODEL_WR840NV4 */

#if defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)  || defined(CONFIG_TP_MODEL_C20V4) 

/* For C20i port1~4 is LAN */
u32 rt_mt7628_chkLan(void)
{
	u32 regData = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	regData = (regData & (0xF << 26)) >> 26;

	return regData;
}
/* For C20i port0 is WAN */
u32 rt_mt7628_chkWan(void)
{
	u32 regData = 0;

	regData = *(unsigned long *)(RALINK_ETH_SW_BASE + 0x80);
	if (regData & (0x1 << 25))
	{
		return 1;
	}
	return 0;
}
#endif /* CONFIG_TP_MODEL_WR840NV4 */


/**
 * ra2882eth_init - Module Init code
 *
 * Called by kernel to register net_device
 *
 */
int __init ra2882eth_init(void)
{
	int ret;
	struct net_device *dev = alloc_etherdev(sizeof(END_DEVICE));

#ifdef CONFIG_RALINK_VISTA_BASIC
	int sw_id=0;
	mii_mgr_read(29, 31, &sw_id);
	is_switch_175c = (sw_id == 0x175c) ? 1:0;
#endif 

	if (!dev)
		return -ENOMEM;

	strcpy(dev->name, DEV_NAME);
	dev->irq  = IRQ_ENET0;
	dev->addr_len = 6;
	dev->base_addr = RALINK_FRAME_ENGINE_BASE;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	rather_probe(dev);
#else
	dev->init =  rather_probe;
#endif
	ra2880_setup_dev_fptable(dev);

	/* net_device structure Init */
	ethtool_init(dev);
	printk("Ralink APSoC Ethernet Driver Initilization. %s  %d rx/tx descriptors allocated, mtu = %d!\n", RAETH_VERSION, NUM_RX_DESC, dev->mtu);
#ifdef CONFIG_RAETH_NAPI
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	printk("NAPI enable, Tx Ring = %d, Rx Ring = %d\n", NUM_TX_DESC, NUM_RX_DESC);
#else
	printk("NAPI enable, weight = %d, Tx Ring = %d, Rx Ring = %d\n", dev->weight, NUM_TX_DESC, NUM_RX_DESC);
#endif
#endif

	/* Register net device for the driver */
	if ( register_netdev(dev) != 0) {
		printk(KERN_WARNING " " __FILE__ ": No ethernet port found.\n");
		return -ENXIO;
	}	

#ifdef CONFIG_RAETH_NETLINK
	csr_netlink_init();
#endif
	ret = debug_proc_init();

	dev_raether = dev;
	return ret;
}

void fe_sw_init(void)
{
#if defined (CONFIG_GIGAPHY) || defined (CONFIG_RAETH_ROUTER) || defined (CONFIG_100PHY)
        unsigned int regValue = 0;
#endif

	// Case1: RT288x/RT3883/MT7621 GE1 + GigaPhy
#if defined (CONFIG_GE1_RGMII_AN)
	enable_auto_negotiate(1);
	if (isMarvellGigaPHY(1)) {
#if defined (CONFIG_RT3883_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, &regValue);
		regValue &= ~(3<<8); //turn off 1000Base-T Advertisement  (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, regValue);
		
		printk("\n Reset MARVELL phy\n");
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, &regValue);
		regValue |= 1<<7; //Add delay to RX_CLK for RXD Outputs
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 20, regValue);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &regValue);
		regValue |= 1<<15; //PHY Software Reset
	 	mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, regValue);
#elif defined (CONFIG_MT7621_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, &regValue);
		regValue &= ~(3<<8); //turn off 1000Base-T Advertisement  (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 9, regValue);
		
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, &regValue);
		regValue |= 1<<9; //restart AN
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 0, regValue);
#endif

	}
	if (isVtssGigaPHY(1)) {
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 1);
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, &regValue);
		printk("Vitesse phy skew: %x --> ", regValue);
		regValue |= (0x3<<12);
		regValue &= ~(0x3<<14);
		printk("%x\n", regValue);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 28, regValue);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR, 31, 0);
        }
#if defined (CONFIG_RALINK_MT7621)
	sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x20056300);//(P0, Auto mode)
#endif
#endif // CONFIG_GE1_RGMII_AN //

	// Case2: RT3883/MT7621 GE2 + GigaPhy
#if defined (CONFIG_GE2_RGMII_AN)
	enable_auto_negotiate(2);
	if (isMarvellGigaPHY(2)) {
#if defined (CONFIG_RT3883_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 9, &regValue);
		regValue &= ~(3<<8); //turn off 1000Base-T Advertisement (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 9, regValue);
		
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 20, &regValue);
		regValue |= 1<<7; //Add delay to RX_CLK for RXD Outputs
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 20, regValue);

		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, &regValue);
		regValue |= 1<<15; //PHY Software Reset
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, regValue);
#elif defined (CONFIG_MT7621_FPGA)
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 9, &regValue);
		regValue &= ~(3<<8); //turn off 1000Base-T Advertisement (9.9=1000Full, 9.8=1000Half)
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 9, regValue);
		
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, &regValue);
		regValue |= 1<<9; //restart AN
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 0, regValue);
#endif

	}
	if (isVtssGigaPHY(2)) {
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 31, 1);
		mii_mgr_read(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 28, &regValue);
		printk("Vitesse phy skew: %x --> ", regValue);
		regValue |= (0x3<<12);
		regValue &= ~(0x3<<14);
		printk("%x\n", regValue);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 28, regValue);
		mii_mgr_write(CONFIG_MAC_TO_GIGAPHY_MODE_ADDR2, 31, 0);
	}
#if defined (CONFIG_RALINK_MT7621)
	//RGMII2=Normal mode
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 15);
	//GMAC2= RGMII mode
	*(volatile u_long *)(SYSCFG1) &= ~(0x3 << 14);

	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x20056300);//(P1, Auto mode)
#endif
#endif // CONFIG_GE2_RGMII_AN //

	// Case3: RT305x/RT335x/RT6855/RT6855A/MT7620 + EmbeddedSW
#if defined (CONFIG_RT_3052_ESW) && !defined(CONFIG_RALINK_MT7621)
#if defined (CONFIG_RALINK_RT6855) || defined(CONFIG_RALINK_MT7620)
	rt_gsw_init();
#elif defined(CONFIG_RALINK_RT6855A)
	rt6855A_gsw_init();
#else
	printk("[%s:%d]rt305x_esw_init.\n", __FUNCTION__, __LINE__);
	rt305x_esw_init();
	/* add by wanghao */
	//rt_rtl8367_init();
#if defined(CONFIG_TP_MODEL_MR3020V3)
	fp_gpioTestLan = rt_mt7628_chkWan;
#else
	fp_gpioTestLan = rt_mt7628_chkLan;
	fp_gpioTestLanOffWan = rt_mt7628_chkLanOffWan;

#endif
    fp_ethTestWan = rt_mt7628_chkWan;
		
	disableEthForward();
    init_raeth_tplink();	
	/* add end	*/	
#endif
#endif 
	// Case4:  RT288x/RT388x/MT7621 GE1 + Internal GigaSW
#if defined (CONFIG_GE1_RGMII_FORCE_1000) || defined (CONFIG_GE1_TRGMII_FORCE_1200) 
#if defined (CONFIG_RALINK_MT7621)
	/*MT7530 Init*/
	setup_internal_gsw();
#else
	sysRegWrite(MDIO_CFG, INIT_VALUE_OF_FORCE_1000_FD);
#endif
#endif 

	// Case5: RT388x/MT7621 GE2 + GigaSW
#if defined (CONFIG_GE2_RGMII_FORCE_1000)
#if defined (CONFIG_RALINK_MT7621)
	setup_external_gsw();
#else
	sysRegWrite(MDIO_CFG2, INIT_VALUE_OF_FORCE_1000_FD);
#endif
#endif 

	// Case6: RT288x GE1 /RT388x,MT7621 GE1/GE2 + (10/100 Switch or 100PHY)
#if defined (CONFIG_RAETH_ROUTER) || defined (CONFIG_100PHY)

	//set GMAC to MII or RvMII mode
#if defined (CONFIG_RALINK_RT3883)
	regValue = sysRegRead(SYSCFG1);
#if defined (CONFIG_GE1_MII_FORCE_100) || defined (CONFIG_GE1_MII_AN)
	regValue &= ~(0x3 << 12);
	regValue |= 0x1 << 12; // GE1 MII Mode
#elif defined (CONFIG_GE1_RVMII_FORCE_100)
	regValue &= ~(0x3 << 12);
	regValue |= 0x2 << 12; // GE1 RvMII Mode
#endif 

#if defined (CONFIG_GE2_MII_FORCE_100) || defined (CONFIG_GE2_MII_AN) 
	regValue &= ~(0x3 << 14);
	regValue |= 0x1 << 14; // GE2 MII Mode
#elif defined (CONFIG_GE2_RVMII_FORCE_100)
	regValue &= ~(0x3 << 14);
	regValue |= 0x2 << 14; // GE2 RvMII Mode
#endif 
	sysRegWrite(SYSCFG1, regValue);
#endif // CONFIG_RALINK_RT3883 //

#elif defined (CONFIG_RALINK_MT7621)

#if defined (CONFIG_GE1_MII_FORCE_100)
	sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x5e337);//(P0, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
#endif
#if defined (CONFIG_GE2_MII_FORCE_100)
	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x5e337);//(P1, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
#endif
#if defined (CONFIG_GE1_MII_AN) || defined (CONFIG_GE1_RGMII_AN)
	enable_auto_negotiate(1);
#if defined (CONFIG_RALINK_MT7621)
	sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x20056300);//(P0, Auto mode)
#endif
#endif
#if defined (CONFIG_GE2_MII_AN) || defined (CONFIG_GE1_RGMII_AN)
	enable_auto_negotiate(2);
#if defined (CONFIG_RALINK_MT7621)
	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x20056300);//(P1, Auto mode)
#endif
#endif

#else
#if defined (CONFIG_GE1_MII_FORCE_100)
#if defined (CONFIG_RALINK_MT7621)
#else
	sysRegWrite(MDIO_CFG, INIT_VALUE_OF_FORCE_100_FD);
#endif
#endif
#if defined (CONFIG_GE2_MII_FORCE_100)
#if defined (CONFIG_RALINK_MT7621)
#else
	sysRegWrite(MDIO_CFG2, INIT_VALUE_OF_FORCE_100_FD);
#endif
#endif
	//add switch configuration here for other switch chips.
#if defined (CONFIG_GE1_MII_FORCE_100) ||  defined (CONFIG_GE2_MII_FORCE_100)
	// IC+ 175x: force IC+ switch cpu port is 100/FD
	mii_mgr_write(29, 22, 0x8420);
#endif


#endif // defined (CONFIG_RAETH_ROUTER) || defined (CONFIG_100PHY) //

}


/**
 * ra2882eth_cleanup_module - Module Exit code
 *
 * Cmd 'rmmod' will invode the routine to exit the module
 *
 */
void ra2882eth_cleanup_module(void)
{
	struct net_device *dev = dev_raether;
	END_DEVICE *ei_local;

	ei_local = netdev_priv(dev);

#ifdef CONFIG_PSEUDO_SUPPORT
	unregister_netdev(ei_local->PseudoDev);
	free_netdev(ei_local->PseudoDev);
#endif
	unregister_netdev(dev);
	RAETH_PRINT("Free ei_local and unregister netdev...\n");

	free_netdev(dev);
	debug_proc_exit();
#ifdef CONFIG_RAETH_NETLINK
	csr_netlink_end();
#endif
}
EXPORT_SYMBOL(set_fe_dma_glo_cfg);
module_init(ra2882eth_init);
module_exit(ra2882eth_cleanup_module);
MODULE_LICENSE("GPL");
