#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include "frame_engine.h"
#include "mcast_tbl.h"


#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
DEFINE_MUTEX(mtbl_lock);
#else
DECLARE_MUTEX(mtbl_lock);
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
#define UP(x)	 mutex_lock(&x)
#define DOWN(x)  mutex_unlock(&x)
#else
#define UP(x)	 up(&x)
#define DOWN(x)  down(&x)
#endif

int32_t mcast_entry_get(uint16_t vlan_id, uint8_t *dst_mac) 
{
    int i=0;

    for(i=0;i<MAX_MCAST_ENTRY;i++) {
	if((GET_PPE_MCAST_H(i)->mc_vid == vlan_id ) && 
		GET_PPE_MCAST_L(i)->mc_mac_addr[3] == dst_mac[2] &&
		GET_PPE_MCAST_L(i)->mc_mac_addr[2] == dst_mac[3] &&
		GET_PPE_MCAST_L(i)->mc_mac_addr[1] == dst_mac[4] &&
		GET_PPE_MCAST_L(i)->mc_mac_addr[0] == dst_mac[5]) {
	    UP(mtbl_lock);
	    return i;
	}
    }
    return -1;
}

/*
  mc_px_en: enable multicast to port x
  mc_px_qos_en: enable QoS for multicast to port x
  
  - multicast port0 map to PDMA
  - multicast port1 map to GMAC1
  - multicast port2 map to GMAC2
  - multicast port3 map to QDMA
*/
int foe_mcast_entry_ins(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid)
{
    int i=0;
    int entry_num;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;

    DOWN(mtbl_lock);

    printk("%s: vid=%x mac=%x:%x:%x:%x:%x:%x mc_px_en=%x mc_px_qos_en=%x \n", __FUNCTION__, vlan_id, dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5], mc_px_en, mc_px_qos_en);
    //update exist entry
    if((entry_num = mcast_entry_get(vlan_id, dst_mac)) >= 0) {
	mcast_h = GET_PPE_MCAST_H(entry_num);
	mcast_l = GET_PPE_MCAST_L(entry_num);

	mcast_h->mc_px_en |= mc_px_en;
	mcast_h->mc_px_qos_en |= mc_px_qos_en;
	UP(mtbl_lock);
	return 1;
    } else { //create new entry
	    for(i=0;i<MAX_MCAST_ENTRY;i++) {

		    mcast_h = GET_PPE_MCAST_H(i);
		    mcast_l = GET_PPE_MCAST_L(i);

		    if(mcast_h->valid == 0) {
	    
			    mcast_h->mc_vid = vlan_id;
			    mcast_h->mc_px_en = mc_px_en;
			    mcast_h->mc_px_qos_en = mc_px_qos_en;
			    mcast_l->mc_mac_addr[3] = dst_mac[2];
			    mcast_l->mc_mac_addr[2] = dst_mac[3];
			    mcast_l->mc_mac_addr[1] = dst_mac[4];
			    mcast_l->mc_mac_addr[0] = dst_mac[5];
			    mcast_h->valid = 1;
			    UP(mtbl_lock);
			    return 1;
		    }
	    }
    }

    MCAST_PRINT("HNAT: Multicast Table is FULL!!\n");
    UP(mtbl_lock);
    return 0;
}

int foe_mcast_entry_qid(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_qos_qid)
{
    int entry_num;
    ppe_mcast_h *mcast_h;

    DOWN(mtbl_lock);
    printk("%s: vid=%x mac=%x:%x:%x:%x:%x:%x mc_qos_qid=%d\n", __FUNCTION__, vlan_id, dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5], mc_qos_qid);
    //update exist entry
    if((entry_num = mcast_entry_get(vlan_id, dst_mac)) >= 0) {
	mcast_h = GET_PPE_MCAST_H(entry_num);
	mcast_h->mc_qos_qid = mc_qos_qid;
	UP(mtbl_lock);
	return 1;
    } 
    UP(mtbl_lock);
    return 0;
}

/*
 * Return:
 *	    0: entry found
 *	    1: entry not found
 */
int foe_mcast_entry_del(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid)
{
    int entry_num;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;

    DOWN(mtbl_lock);
    printk("%s: vid=%x mac=%x:%x:%x:%x:%x:%x mc_px_en=%x mc_px_qos_en=%x mc_qos_qid=%d\n", __FUNCTION__, vlan_id, dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5], mc_px_en, mc_px_qos_en, mc_qos_qid);
    if((entry_num = mcast_entry_get(vlan_id, dst_mac)) >= 0) {
	mcast_h = GET_PPE_MCAST_H(entry_num);
	mcast_l = GET_PPE_MCAST_L(entry_num);

	mcast_h->mc_px_en &= ~mc_px_en;
	mcast_h->mc_px_qos_en &= ~mc_px_qos_en;
	
	if(mcast_h->mc_px_en == 0 && mcast_h->mc_px_qos_en == 0) {
		mcast_h->valid = 0;
		mcast_h->mc_vid = 0;
		mcast_h->mc_qos_qid = 0;
		memset(&mcast_l->mc_mac_addr, 0, 4);
	}
	UP(mtbl_lock);
	return 0;
    }else { 
	UP(mtbl_lock);
	return 1;
    }
}

void foe_mcast_entry_dump(void)
{
    int i;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;

    DOWN(mtbl_lock);
    printk("MAC | VID | PortMask | QosPortMask \n");
    for(i=0;i<MAX_MCAST_ENTRY;i++) {

	    mcast_h = GET_PPE_MCAST_H(i);
	    mcast_l = GET_PPE_MCAST_L(i);

	    printk("%x:%x:%x:%x  %d  %c%c%c%c %c%c%c%c (QID=%d)\n", 
			    mcast_l->mc_mac_addr[3], 
			    mcast_l->mc_mac_addr[2], 
			    mcast_l->mc_mac_addr[1], 
			    mcast_l->mc_mac_addr[0], 
			    mcast_h->mc_vid, 
			    (mcast_h->mc_px_en & 0x08)?'1':'-',
			    (mcast_h->mc_px_en & 0x04)?'1':'-',
			    (mcast_h->mc_px_en & 0x02)?'1':'-',
			    (mcast_h->mc_px_en & 0x01)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x08)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x04)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x02)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x01)?'1':'-',
			     mcast_h->mc_qos_qid);
    }
    UP(mtbl_lock);
}	

void foe_mcast_entry_del_all(void)
{
    int i;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;

    DOWN(mtbl_lock);
    for(i=0;i<MAX_MCAST_ENTRY;i++) {
	    mcast_h = GET_PPE_MCAST_H(i);
	    mcast_l = GET_PPE_MCAST_L(i);
		mcast_h->mc_px_en = 0;
	        mcast_h->mc_px_qos_en = 0;
		mcast_h->valid = 0;
		mcast_h->mc_vid = 0;
		mcast_h->mc_qos_qid = 0;
		memset(&mcast_l->mc_mac_addr, 0, 4);
    }
    UP(mtbl_lock);
}
