/*!Copyright(c) 2014-2016 Shenzhen TP-Link Technologies Co.Ltd.
 *
 * \file    multicast_vlan.c
 * \brief   This file implements multicast tag management for RU IPTV.
 *
 * \note    Most of the code is extracted from mtk_ApSoc_4120/linux.../rtl8367_api.c
 *
 * \author  Xu Kuohai
 * \version 0.1
 * \date    22Oct2016
 *
 *
 * \history \arg 0.1, 22Oct2016, Xu Kuohai, Create file.
 */

/******************************************************************************/
/*                              CONFIGURATIONS                                */
/******************************************************************************/


/******************************************************************************/
/*                              INCLUDE_FILES                                 */
/******************************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/skbuff.h>

/******************************************************************************/
/*                              DEFINES                                       */
/******************************************************************************/


/******************************************************************************/
/*                              TYPES                                         */
/******************************************************************************/

struct multicastVlanInfo {
    u16 multicastTci;
    u16 iptvTci;
};

struct vlanHdr
{
    u16 tpid;
    u16 tci;
};

/******************************************************************************/
/*                              EXTERN_PROTOTYPES                             */
/******************************************************************************/


/******************************************************************************/
/*                              LOCAL_PROTOTYPES                              */
/******************************************************************************/


/******************************************************************************/
/*                              VARIABLES                                     */
/******************************************************************************/

static struct multicastVlanInfo mcastVlanInfo;

#define multicastTci    (mcastVlanInfo.multicastTci)
#define iptvTci         (mcastVlanInfo.iptvTci)

int (*multicastVlanTxHook)(struct sk_buff *skb) = NULL;
int (*multicastVlanRxHook)(struct sk_buff *skb) = NULL;

/******************************************************************************/
/*                              LOCAL_FUNCTIONS                               */
/******************************************************************************/


/******************************************************************************/
/*                              PUBLIC_FUNCTIONS                              */
/******************************************************************************/

int multicastVlanReplaceTxTag(struct sk_buff *skb)
{
    struct vlanHdr *vlanHdr;

    vlanHdr = (struct vlanHdr *)(skb->data + 12);

    if ((skb->data[0] == 0x01) && (skb->data[1] == 0x00) && (skb->data[2] == 0x5E)/* multicast dst MAC  */
            && ntohs(vlanHdr->tpid) == 0x8100 && ntohs(vlanHdr->tci) == iptvTci)
    {
        vlanHdr->tci = htons(multicastTci);/* replace iptv tag with multicast tag  */
    }

    return 0;
}

int multicastVlanReplaceRxTag(struct sk_buff *skb)
{
    struct vlanHdr *vlanHdr;

    vlanHdr = (struct vlanHdr *)(skb->data - 2);

    if (ntohs(vlanHdr->tpid) == 0x8100 && ((ntohs(vlanHdr->tci) ^ multicastTci) & 0xFFF) == 0)
    {
        vlanHdr->tci = htons(iptvTci);/* replace multicast tag with iptv tag  */
    }
    
    return 0;
}

int multicastVlanSet(struct ifreq *ifr)
{
    copy_from_user(&mcastVlanInfo, ifr->ifr_data, sizeof(struct multicastVlanInfo));

    if ((multicastTci & 0xFFF) != 0) {
        multicastVlanTxHook = multicastVlanReplaceTxTag;
        multicastVlanRxHook = multicastVlanReplaceRxTag;
    } else {
        multicastVlanTxHook = NULL;
        multicastVlanRxHook = NULL;
    }
    return 0;
}

/******************************************************************************/
/*                              GLOBAL_FUNCTIONS                              */
/******************************************************************************/
