#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/rt2880/surfboardint.h>


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
#include <asm/rt2880/rt_mmap.h>
#else
#include <linux/libata-compat.h>
#endif
#include <linux/proc_fs.h>
#include <linux/scatterlist.h>
#include "aes_engine.h"
#include "aes_pattern.h"
#include "mcrypto_aes_api.h"

int (*AesDoneIntCallback[NUM_AES_RX_DESC])(uint32_t); 
AES_userdata_type AES_userdata[NUM_AES_RX_DESC];

static DECLARE_TASKLET( \
AesRxTasklet, AesRxHandler, 0);

typedef struct aes_engine_proc_t {
    int put_count;
    int isr_count;
}aes_engine_proc_type;

aes_engine_proc_type aes_engine_proc;

uint8_t* sbuf[MAX_TEST_SEQ][MAX_SG_BUF];
uint8_t* dbuf[MAX_TEST_SEQ][MAX_SG_BUF];
uint8_t* tv_key[MAX_TEST_SEQ];

struct scatterlist sg_src_test[MAX_TEST_SEQ][MAX_SG_BUF];
struct scatterlist sg_dst_test[MAX_TEST_SEQ][MAX_SG_BUF];
int tv_seq[MAX_TEST_SEQ];
int tv_enc[MAX_TEST_SEQ];
static unsigned long aes_tx_front_idx=0;
static unsigned long aes_rx_front_idx=0;
static unsigned long aes_tx_rear_idx=0;
static unsigned long aes_rx_rear_idx=0;
#ifdef DBG
static unsigned long start_clk = 0, end_clk = 0;
extern u32 mips_cpu_feq;
#endif
static struct proc_dir_entry *pProcDir   = NULL;
static const int aes_key_len[3]={16,24,32};

struct AesReqEntry AES_Entry;
EXPORT_SYMBOL(AES_Entry);
static int AesDescriptorInit(void);
static void set_AES_glo_cfg(void);
static void clear_AES_glo_cfg(void);
static int aes_irq_enabled = 0;
int aes_dbg_print = 0;
static int burstlen = 4;

int AesProcessScatterGather( 
	struct scatterlist* sg_src,
	struct scatterlist* sg_dst,
	uint32_t TransCount,
	uint8_t*	Key,
	uint8_t*	IV,
	uint32_t	aes_mode
	)
{
	int i=1;
	int j=1;
	int bUserData = 0;
	struct scatterlist *next_dst, *next_src;
	u8* Src0, *Dst0, *Dst1;
	struct AES_txdesc* txdesc = NULL;
	struct AES_rxdesc* rxdesc = NULL;
	unsigned int aes_tx_scatter = 0;
	unsigned int aes_rx_gather = 0;
	unsigned int keylen = aes_key_len[aes_mode&0x03];
	unsigned long flags = 0;
	int rx_remain_len = 0;
	int rx_desc_userdata = 0;
	
	next_src = sg_src;
	next_dst = sg_dst;
	
	if (aes_irq_enabled == 0)	
		spin_lock_irqsave(&(AES_Entry.page_lock), flags);
	//start_clk = read_c0_count();
	
	while(next_src->length==0)
	{
		if(!sg_is_last(next_src))
			next_src = sg_next(next_src);
		else
		{	
			printk("last src length=0\n");
			goto EXIT;
		}	
	}
	while(next_dst->length==0)
	{
		if(!sg_is_last(next_dst))
			next_dst = sg_next(next_dst);
		else
		{	
			printk("last dst length=0\n");
			goto EXIT;
		}	
	}
	
	Src0 = sg_virt(next_src);
	Dst0 = sg_virt(next_dst);		

	//RX descriptor
	while(1)
	{
		aes_rx_gather  = (aes_rx_rear_idx + j) % NUM_AES_RX_DESC;

		if (bUserData == 0)
		{	
			rxdesc = &AES_Entry.AES_rx_ring0[aes_rx_gather];
			DBGPRINT(DBG_HIGH, "AES set RX Desc[%u] Dst0=%08X, len=%d\n",(u32)aes_rx_gather, (u32)Dst0, next_dst->length);
			
	        if (((u32)Dst0&0x3) || (next_dst->length&0x3))
			{
				bUserData = 1;
				rx_desc_userdata = aes_rx_gather;
				AES_userdata[aes_rx_gather].orig_SDP0 = (u32)Dst0;
		        AES_userdata[aes_rx_gather].orig_SDL = next_dst->length;
				rx_remain_len += next_dst->length;
			}
			else
				AES_userdata[aes_rx_gather].new_SDP0 = 0;
	
			if (bUserData == 0)
			{	
				rxdesc->SDP0 = dma_map_single(NULL, Dst0, next_dst->length, PCI_DMA_FROMDEVICE);
				rxdesc->aes_rxd_info2.SDL0 = next_dst->length;
				rxdesc->aes_rxd_info2.DDONE = 0;
	
	 		}
		}
		else
		{	
			AES_userdata[aes_rx_gather].orig_SDP0 = (u32)Dst0;
	        AES_userdata[aes_rx_gather].orig_SDL = next_dst->length;
			rx_remain_len += next_dst->length;
		}		
		if (sg_is_last(next_dst))
		{ 
			if (bUserData == 0)
				rxdesc->aes_rxd_info2.LS0 = 1;
			break;
		}
		else
		{
			if (bUserData == 0)
				rxdesc->aes_rxd_info2.LS0 = 0;//1;			
		}
		
		next_dst = sg_next(next_dst);
		Dst0 = sg_virt(next_dst);
		j++;
	}	  
  	
  	if (bUserData == 0)
		aes_rx_rear_idx = aes_rx_gather;
	else
	{
		int new_SDL = rx_remain_len;
        int rx_desc_start = rx_desc_userdata;
        int remain_SDL = new_SDL;

       	while (remain_SDL > 0)
		{
	        int SDL = (remain_SDL > (16384-4)) ? 16380 : remain_SDL;

			Dst1 = kmalloc(SDL, GFP_DMA|GFP_ATOMIC);
			DBGPRINT(DBG_HIGH, "RxDesc[%u] realloc len %d (%08X)\n", (u32)rx_desc_start, (u32)SDL, (u32)Dst1);
			if (Dst1)
			{	
       			rxdesc = &AES_Entry.AES_rx_ring0[rx_desc_start];
				
	        	rxdesc->SDP0 = dma_map_single(NULL, Dst1, SDL, PCI_DMA_FROMDEVICE);
				rxdesc->aes_rxd_info2.SDL0 = SDL;
				rxdesc->aes_rxd_info2.DDONE = 0;
				rxdesc->aes_rxd_info2.LS0 = 0;
	        	
				AES_userdata[rx_desc_start].new_SDP0 = (u32)Dst1;
				AES_userdata[rx_desc_start].new_SDL = SDL;
				aes_rx_rear_idx = rx_desc_start;				

				remain_SDL-=SDL;
				rx_desc_start = (rx_desc_start + 1) % NUM_AES_RX_DESC;
			}
			else
			{
				printk("Can't alloc AES Engine bounce buffer\n");
				AES_userdata[rx_desc_start].new_SDP0 = 0;
				//need free previous kmalloc buffer
				break;
			}
		}
		rxdesc->aes_rxd_info2.LS0 = 1;
	}			
  	
	while(1)
	{	
		aes_tx_scatter = (aes_tx_rear_idx + i) % NUM_AES_TX_DESC;
		DBGPRINT(DBG_HIGH, "AES set TX Desc[%u] Src0=%08X len=%d klen=%d\n",aes_tx_scatter, (u32)Src0, next_src->length,keylen);	
		txdesc = &AES_Entry.AES_tx_ring0[aes_tx_scatter];
		
		txdesc->aes_txd_info4.value = aes_mode;

		{	
			if (aes_mode&CBC_MODE)
			{	
				txdesc->aes_txd_info4.value |= VALID_IV;
				txdesc->aes_txd_info4.value |= RESTORE_IV;
				if (i > 1)
					txdesc->aes_txd_info4.value &= ~VALID_IV;
					
				//first tx, set Key and IV
				if (txdesc->aes_txd_info4.value & VALID_IV)
				{	
					if (IV == NULL)
						memset((void*)txdesc->IV, 0xFF, sizeof(uint32_t)*4);
					else
						memcpy((void*)txdesc->IV, IV, sizeof(uint32_t)*4);
				}	
			}
			
			if (i==1)
			{	
				txdesc->SDP0 = ((uint32_t)Key & 0x1FFFFFFF);
				if (bUserData)
				{	
					txdesc->aes_txd_info4.value |= CARRY_USERDATA;
					*(uint32_t*)&Key[keylen] = 	(uint32_t)AES_userdata;
					txdesc->aes_txd_info2.SDL0 = ((keylen>>2)+1)*sizeof(uint32_t);	/* KEY + USER_DATA */
				}
				else
					txdesc->aes_txd_info2.SDL0 = ((keylen>>2))*sizeof(uint32_t);	/* KEY */
			}
			else
				txdesc->aes_txd_info2.SDL0 = 0;
			
			txdesc->aes_txd_info2.LS0 = 0;//1;
		}

		txdesc->SDP1 = ((uint32_t)(Src0) & 0x1FFFFFFF);
		txdesc->aes_txd_info2.SDL1 = next_src->length;
		txdesc->aes_txd_info2.DDONE = 0;

		dma_cache_wback_inv((unsigned long)(Src0), (next_src->length));
		if (bUserData)
			dma_cache_wback_inv((unsigned long)(Key), ((keylen>>2)+1)*sizeof(uint32_t));
		else
			dma_cache_wback_inv((unsigned long)(Key), ((keylen>>2))*sizeof(uint32_t));

		if (sg_is_last(next_src))
		{ 
			DBGPRINT(DBG_HIGH, "[%d]LS1=1\n",i);
			txdesc->aes_txd_info2.LS1 = 1;
			break;
		}
		else
			txdesc->aes_txd_info2.LS1 = 0;		
		
		next_src = sg_next(next_src);
		Src0 = sg_virt(next_src);
		i++;			
	}

	aes_tx_rear_idx = aes_tx_scatter;

  
  	
  	for (i=0; i < NUM_AES_TX_DESC; i++) {
#ifdef DBG
		uint32_t* ptr = (uint32_t*)&AES_Entry.AES_tx_ring0[i];
#endif		
		DBGPRINT(DBG_LOW, "Tx Ring%d[%08X] [f=%u r=%u]:\n",i,(u32)ptr, (u32)aes_tx_front_idx, (u32)aes_tx_rear_idx);
		DBGPRINT(DBG_LOW, "%08X %08X %08X %08X\n",*ptr, *(ptr+1),*(ptr+2),*(ptr+3));
		DBGPRINT(DBG_LOW, "%08X %08X %08X %08X\n",*(ptr+4), *(ptr+5),*(ptr+6),*(ptr+7));
	}
	DBGPRINT(DBG_LOW, "\n");	
	for (i=0; i < NUM_AES_RX_DESC; i++) {
#ifdef DBG
		uint32_t* ptr = (uint32_t*)&AES_Entry.AES_rx_ring0[i];
#endif		
		DBGPRINT(DBG_LOW, "Rx Ring%d[%08X] [f=%u r=%u]:\n",i,(u32)ptr, (u32)aes_rx_front_idx, (u32)aes_rx_rear_idx);
		DBGPRINT(DBG_LOW, "%08X %08X %08X %08X\n",*ptr, *(ptr+1),*(ptr+2),*(ptr+3));
		DBGPRINT(DBG_LOW, "%08X %08X %08X %08X\n",*(ptr+4), *(ptr+5),*(ptr+6),*(ptr+7));
	}
	DBGPRINT(DBG_LOW, "\n");
	
	DBGPRINT(DBG_MID, "AES_RX_CALC_IDX0 = %d\n",sysRegRead(AES_RX_CALC_IDX0));
	DBGPRINT(DBG_MID, "AES_RX_DRX_IDX0 = %d\n",sysRegRead(AES_RX_DRX_IDX0));	
	DBGPRINT(DBG_MID, "AES_TX_CTX_IDX0 = %d\n",sysRegRead(AES_TX_CTX_IDX0));
	DBGPRINT(DBG_MID, "AES_TX_DTX_IDX0 = %d\n",sysRegRead(AES_TX_DTX_IDX0));
	DBGPRINT(DBG_MID, "AES_INFO = 0x%08X\n",sysRegRead(AES_INFO));
	DBGPRINT(DBG_MID, "AES_GLO_CFG = 0x%08X\n",sysRegRead(AES_GLO_CFG));
	DBGPRINT(DBG_MID, "AES_INT_STATUS = 0x%08X\n",sysRegRead(AES_INT_STATUS));
	DBGPRINT(DBG_MID, "AES_GLO_CFG = 0x%08X\n",sysRegRead(AES_GLO_CFG));
	DBGPRINT(DBG_MID, "AES_RST_CFG = 0x%08X\n",sysRegRead(AES_RST_CFG));

	DBGPRINT(DBG_MID, "[*]TT [front=%u rear=%u]; RR [front=%u rear=%u]\n",(u32)aes_tx_front_idx, (u32)aes_tx_rear_idx,(u32)aes_rx_front_idx,(u32)aes_rx_rear_idx);
#ifdef DBG
	start_clk = read_c0_count();
#endif	
	sysRegWrite(AES_TX_CTX_IDX0, cpu_to_le32((u32)((aes_tx_rear_idx+1)%NUM_AES_TX_DESC)));//tx start to move, 1->2->255->0->1

EXIT:
	DBGPRINT(DBG_MID, "[]<<<<< AES set DONE >>>>>][\n");

	if (aes_irq_enabled == 0)		
		spin_unlock_irqrestore(&(AES_Entry.page_lock), flags);

	if (aes_irq_enabled == 0)
	{		
		AesRxHandler(0);

#if 0
		for (i=0; i < NUM_AES_TX_DESC; i++) {
			memset(&AES_Entry.AES_tx_ring0[i], 0, sizeof(struct AES_txdesc));
			AES_Entry.AES_tx_ring0[i].aes_txd_info2.LS0 = 0;		
			AES_Entry.AES_tx_ring0[i].aes_txd_info2.LS1 = 0;
			AES_Entry.AES_tx_ring0[i].aes_txd_info2.DDONE = 1;
		}
		
		for (i = 0; i < NUM_AES_RX_DESC; i++) {
			memset(&AES_Entry.AES_rx_ring0[i],0,sizeof(struct AES_rxdesc));
			AES_Entry.AES_rx_ring0[i].aes_rxd_info2.DDONE = 0;
			AES_Entry.AES_rx_ring0[i].aes_rxd_info2.LS0 = 0;		
		}
				
		//TX0
		//sysRegWrite(AES_TX_BASE_PTR0, phys_to_bus((u32) AES_Entry.phy_aes_tx_ring0));
		//sysRegWrite(AES_TX_MAX_CNT0, cpu_to_le32((u32) NUM_AES_TX_DESC));	
		sysRegWrite(AES_TX_CTX_IDX0, 0);
		aes_tx_front_idx = 0;
		aes_tx_rear_idx = NUM_AES_TX_DESC-1;
		sysRegWrite(AES_RST_CFG, AES_PST_DTX_IDX0);
		   
		//RX0
		//sysRegWrite(AES_RX_BASE_PTR0, phys_to_bus((u32) AES_Entry.phy_aes_rx_ring0));
		//sysRegWrite(AES_RX_MAX_CNT0,  cpu_to_le32((u32) NUM_AES_RX_DESC));
		
		sysRegWrite(AES_RX_CALC_IDX0, cpu_to_le32((u32) (NUM_AES_RX_DESC - 1)));
		sysRegRead(AES_RX_CALC_IDX0);
		aes_rx_front_idx = 0;
		aes_rx_rear_idx = NUM_AES_RX_DESC-1;
		sysRegWrite(AES_RST_CFG, AES_PST_DRX_IDX0);
#endif		
	}		
	return 0;
}


int AesKickEngine(void)
{
	int i;
	unsigned long flags = 0;
	DBGPRINT(DBG_MID, "AES Kick Off\n");

	if (aes_irq_enabled == 1)
		spin_lock_irqsave(&(AES_Entry.page_lock), flags);
		
	for (i = 0; i < MAX_TEST_SEQ; i++)
	{
		int j;
		int tv;
		uint8_t* src;
		
		if (tv_seq[i] < 0)
			continue;
			
		tv = tv_seq[i];
		DBGPRINT(DBG_LOW, "IV=%08X\n",(u32)pattern[tv].iv);
		for (j = 0; j < 16; j++)
			DBGPRINT(DBG_LOW, "%02X ",pattern[tv].iv[j]);
		DBGPRINT(DBG_LOW, "\n");
		
		DBGPRINT(DBG_LOW, "KEY=%08X seq=%d tv=%u\n",(u32)tv_key[i],i,tv_seq[i]);
		src = tv_key[i];
		for (j = 0; j < aes_key_len[pattern[tv].mode&0x03]; j++, src++)
			DBGPRINT(DBG_LOW, "%02X ",*src);
		DBGPRINT(DBG_LOW, "\n");
		
		if (tv_enc[i]==1)
		{	
			pattern[tv].mode |= ENCRYPTION;
		}
		else
		{
			pattern[tv].mode &= ~ENCRYPTION;
		}	
	
		//start_clk = read_c0_count();
		AesProcessScatterGather(&sg_src_test[i][0], &sg_dst_test[i][0], \
								pattern[tv].totallen, (uint8_t*)tv_key[i], \
								(uint8_t*)pattern[tv].iv, (u32)pattern[tv].mode);
	}
	if (aes_irq_enabled == 1)
		spin_unlock_irqrestore(&(AES_Entry.page_lock), flags);
	return 0;
}
	
void AesRxHandler(unsigned long data)
{
	
	int k,n,m, bUserData = 0;
	unsigned long flags = 0, regVal;
	int try_count = -1;
#ifdef DBG	
	end_clk = 0;
#endif	
	if (aes_irq_enabled == 1)
		spin_lock_irqsave(&(AES_Entry.page_lock), flags);
	
	do {
		regVal = sysRegRead(AES_GLO_CFG);
		if((regVal & AES_RX_DMA_BUSY))
		{
			DBGPRINT(DBG_MID, "Poll RX_DMA_BUSY !!! \n");
			udelay(10);
			continue;		
		}
		if((regVal & AES_TX_DMA_BUSY))
		{
			DBGPRINT(DBG_MID, "Poll TX_DMA_BUSY !!! \n");
			udelay(10);
			continue;			
			
		}
		break;
		
	}while(1);
	
	k = aes_rx_front_idx;
	m = aes_tx_front_idx;
	n = k;
	DBGPRINT(DBG_MID, "TX [front=%u rear=%u]; RX [front=%u rear=%u]\n",\
			(u32)aes_tx_front_idx, (u32)aes_tx_rear_idx, (u32)aes_rx_front_idx, (u32)aes_rx_rear_idx);
	do
	{
		if (AES_Entry.AES_rx_ring0[n].aes_rxd_info2.DDONE ==0)
		{
			if (aes_irq_enabled == 1)
				n = (n+1)%NUM_AES_RX_DESC;
			try_count++;
			if (try_count >= NUM_AES_RX_DESC)
			{
				printk("============ Empty Rx Desc[%d] (try=%d)=============\n",n,try_count);
				if (aes_irq_enabled == 1)	
					break;
			}	
			continue;
		}
		else
		{
#ifdef DBG			
			if (end_clk ==0)	
				end_clk = read_c0_count();
#endif		
		}
		k = n;
		
		if (AES_Entry.AES_rx_ring0[k].aes_rxd_info4.bitfield.UDV)
		{
			bUserData = 1;
		}	
		
		if (bUserData)
		{
			
			if (AES_userdata[k].new_SDP0)
			{
				int first_rx = k;
				int last_rx = k;
				do {
					//int kk;
					int j;
					int pos;
					//u8* pdst = AES_userdata[first_rx].new_SDP0;
					DBGPRINT(DBG_HIGH, "Rx[%d] user data, len=%d\n",first_rx,AES_Entry.AES_rx_ring0[first_rx].aes_rxd_info2.SDL0);
					
					dma_unmap_single (NULL, AES_Entry.AES_rx_ring0[first_rx].SDP0, AES_Entry.AES_rx_ring0[first_rx].aes_rxd_info2.SDL0, PCI_DMA_FROMDEVICE);
					pos = 0;
					j = last_rx;//k;
					do{
						memcpy((void*)AES_userdata[j].orig_SDP0, (void*)AES_userdata[first_rx].new_SDP0+pos, AES_userdata[j].orig_SDL);
						//pdst = AES_userdata[first_rx].new_SDP0+pos;
						//for (kk=0; kk<16; kk++)
						//	printk("%02X ",*pdst++);
						//printk("\n");
						DBGPRINT(DBG_HIGH, "copy to RxBuf[%d][%08X] len=%d\n",j, AES_userdata[j].orig_SDP0,AES_userdata[j].orig_SDL);
						pos+=AES_userdata[j].orig_SDL;
						if (pos >=AES_userdata[first_rx].new_SDL)
							break;
						j = (j+1)%NUM_AES_RX_DESC;
		
					}while(1);
					last_rx = j;
					kfree((void*)AES_userdata[first_rx].new_SDP0);
					if (AES_Entry.AES_rx_ring0[first_rx].aes_rxd_info2.LS0 == 1)
					{
						k = first_rx;	
						break;
					}	
					first_rx = (first_rx+1)%NUM_AES_RX_DESC;	
				}while(1);	
			}		
		}
		else
			dma_unmap_single (NULL, AES_Entry.AES_rx_ring0[k].SDP0, AES_Entry.AES_rx_ring0[k].aes_rxd_info2.SDL0, PCI_DMA_FROMDEVICE);

		
		DBGPRINT(DBG_HIGH, "Rx Desc[%d] Done\n",k);
		if (AES_Entry.AES_rx_ring0[k].aes_rxd_info2.LS0)
		{
			AES_Entry.AES_rx_ring0[k].aes_rxd_info2.LS0 = 0;
			/* last RX, release correspond TX */
			do
			{	
				AES_Entry.AES_tx_ring0[m].aes_txd_info2.DDONE = 1;
				if (AES_Entry.AES_tx_ring0[m].aes_txd_info2.LS1)
				{
					AES_Entry.AES_tx_ring0[m].aes_txd_info2.LS1 = 0;
					m = (m+1)%NUM_AES_TX_DESC;	
					break;
				}
				m = (m+1)%NUM_AES_TX_DESC;
			}while (1);
			if (aes_tx_front_idx==aes_tx_rear_idx)
			{	
				aes_tx_front_idx = m;
				DBGPRINT(DBG_HIGH, "Tx Desc Clean\n");
			}
			else	
				aes_tx_front_idx = m;

			if (k==aes_rx_rear_idx)
			{	
				aes_rx_front_idx = (k+1)%NUM_AES_RX_DESC;
				DBGPRINT(DBG_HIGH, "Rx Desc Clean\n");
				AES_Entry.AES_rx_ring0[k].aes_rxd_info2.DDONE = 0;
				break;
			}	
			else	
				aes_rx_front_idx = (k+1)%NUM_AES_RX_DESC;
			bUserData = 0;	
		}
		AES_Entry.AES_rx_ring0[k].aes_rxd_info2.DDONE = 0; 
		n = (k+1)%NUM_AES_RX_DESC;
		
		
	}while(1);
	
	sysRegWrite(AES_RX_CALC_IDX0, cpu_to_le32((u32) (k)));
	aes_rx_rear_idx = sysRegRead(AES_RX_CALC_IDX0);
	if (aes_irq_enabled == 1)
		spin_unlock_irqrestore(&(AES_Entry.page_lock), flags);
#ifdef DBG
	//if (end_clk ==0)	
	//	end_clk = read_c0_count();
#endif
	return ;
}

irqreturn_t AesEngineIrqHandler(
	int irq, 
	void *irqaction
	)
{
	
	DBGPRINT(DBG_HIGH, "AES INT %08X\n",sysRegRead(AES_INT_STATUS));
	tasklet_hi_schedule(&AesRxTasklet);
	sysRegWrite(AES_INT_STATUS, AES_FE_INT_ALL);  //Write one clear INT_status	
	
	return IRQ_HANDLED;	
}


static int aes_engine_proc_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len;
    if (off > 0)
    {
        return 0;
    }

    len = sprintf(buf, "isr : %d\n", aes_engine_proc.isr_count);
    len += sprintf(buf + len, "put packet : %d\n", aes_engine_proc.put_count);
    return len;
}

static int aes_engine_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char buf[128];
    char* pch[5];
    char* ptr;
    char cmd;
    int len = count, i;
	unsigned long reg_int_mask=0;
	
    if (len >= sizeof(buf))
    {
        len = sizeof(buf) - 1;
    }
	memset(buf, 0x0 ,128);
    if (copy_from_user(buf, buffer, len))
    {
        return -EFAULT;
    }
		
	ptr = buf;	
	i = 0;	
	pch[i] = strsep(&ptr," ");
	cmd = (char)*(pch[0]+0);
	while (pch[i] != NULL)
  	{
    	DBGPRINT (DBG_LOW, "[%d]%s\n",i,pch[i]);
    	i++;
    	pch[i] = strsep (&ptr, " ");
    	if (i >= 5)
    		break;
  	}
	if (cmd == 't')
	{
		memset(sbuf, 0, sizeof(uint32_t)*MAX_TEST_SEQ*MAX_SG_BUF);
		memset(dbuf, 0, sizeof(uint32_t)*MAX_TEST_SEQ*MAX_SG_BUF);
		memset(tv_seq, 0xFF, sizeof(int)*MAX_TEST_SEQ);
		memset(tv_enc, 0xFF, sizeof(int)*MAX_TEST_SEQ);
		memset(tv_key, 0, sizeof(uint32_t)*MAX_TEST_SEQ);
		if (i <= 2)
		{	
			tv_seq[0] = 0;
			tv_enc[0] = 0;
		}
		else
		{
			tv_enc[0] = (*(pch[2]+0)=='e') ? 1 : 0;
			tv_seq[0] = simple_strtol(pch[1], (char **)pch[2], 10);
		}
		AesGenPattern(tv_seq[0], 0, tv_enc[0]);
		AesKickEngine();
	}
	if (cmd == 's')
	{
		memset(sbuf, 0, sizeof(uint32_t)*MAX_TEST_SEQ*MAX_SG_BUF);
		memset(dbuf, 0, sizeof(uint32_t)*MAX_TEST_SEQ*MAX_SG_BUF);
		memset(tv_seq, 0xFF, sizeof(int)*MAX_TEST_SEQ);
		memset(tv_enc, 0xFF, sizeof(int)*MAX_TEST_SEQ);
		memset(tv_key, 0, sizeof(uint32_t)*MAX_TEST_SEQ);
		tv_seq[0] = 0;
		tv_seq[1] = 0;
		tv_seq[2] = 1;
		tv_seq[3] = 2;
		tv_seq[4] = 1;
		tv_seq[5] = 3;
		
		tv_seq[6] = 4;
		tv_seq[7] = 4;
		tv_seq[8] = 5;
		tv_seq[9] = 5;
		
		tv_seq[10] = 6;
		tv_seq[11] = 7;
		
		tv_seq[12] = 8;
		tv_seq[13] = 8;
		
		tv_enc[0] = 1;
		tv_enc[1] = 0;
		tv_enc[2] = 1;
		tv_enc[3] = 1;
		tv_enc[4] = 0;
		tv_enc[5] = 0;
		
		tv_enc[6] = 1;
		tv_enc[7] = 0;
		tv_enc[8] = 1;
		tv_enc[9] = 0;
		
		tv_enc[10] = 1;
		tv_enc[11] = 0;
		
		tv_enc[12] = 1;
		tv_enc[13] = 0;
		
		printk("============================================================\n");
		
		for (i=0; i<MAX_TEST_SEQ;i++)
		{
			if (tv_seq[i]>=0)
			{	
				AesGenPattern(tv_seq[i], i, tv_enc[i]);
				printk("============================================================\n");
			}
			
		}
		AesKickEngine();
		
	}
	if (cmd == 'z')
	{
		for (i=0; i<MAX_TEST_SEQ;i++)
		{
			if (tv_seq[i]>=0)
			{	
				AesResultVerify(tv_seq[i], i, tv_enc[i]);
				printk("============================================================\n");
			}
			
		}
	}			
	if (cmd == 'd')
	{
		for (i=0; i < NUM_AES_TX_DESC; i++) {
			uint32_t* ptr = (uint32_t*)&AES_Entry.AES_tx_ring0[i];
			printk("Tx Ring%d[%08X]:\n",i,(u32)ptr);
			printk("%08X %08X %08X %08X\n",*ptr, *(ptr+1),*(ptr+2),*(ptr+3));
			printk("%08X %08X %08X %08X\n",*(ptr+4), *(ptr+5),*(ptr+6),*(ptr+7));
		}
		printk("\n");
		for (i=0; i < NUM_AES_RX_DESC; i++) {
			uint32_t* ptr = (uint32_t*)&AES_Entry.AES_rx_ring0[i];
			printk("Rx Ring%d[%08X]:\n",i,(u32)ptr);
			printk("%08X %08X %08X %08X\n",*ptr, *(ptr+1),*(ptr+2),*(ptr+3));
			printk("%08X %08X %08X %08X\n",*(ptr+4), *(ptr+5),*(ptr+6),*(ptr+7));
		}
		printk("\n");
	}
	if (cmd == 'p')
	{
		aes_dbg_print = simple_strtol(pch[1], (char **)pch[2], 10);
	}		
	if (cmd == 'v')
	{	
		AesResultVerify(tv_seq[0], 0, tv_enc[0]);
#ifdef DBG
		{
			unsigned long delta;
	
			if(start_clk <= end_clk)
				delta = (end_clk - start_clk)/((mips_cpu_feq>>1)/(1000000));
			else
				delta = (0xFFFFFFFF - (start_clk - end_clk))/((mips_cpu_feq>>1)/(1000000));
	
			printk("Time consume = %u usec\n",(u32)delta);
			printk("Data Rate = %u MB/s\n", (u32)(pattern[tv_seq[0]].totallen/delta));
		}
#endif
	}
	if (cmd == 'c')
	{
		int n, j, seq;
		
		for (seq = 0; seq < MAX_TEST_SEQ; seq++)
		{
			if (tv_key[seq])
				free_page((unsigned long)tv_key[seq]);
			tv_key[seq] =NULL;
			
			if (tv_seq[seq] >= 0)
			{	
				n = pattern[tv_seq[seq]].src_multipage;
				for (j = 0 ; j < n ; j++)
					if (sbuf[seq][j])
					{	
			    		free_page((unsigned long)sbuf[seq][j]);
			    		sbuf[seq][j] = NULL;
					}
		
				n = pattern[tv_seq[seq]].dst_multipage;
				for (j = 0 ; j < n ; j++)
					if (dbuf[seq][j])
					{	
			    		free_page((unsigned long)dbuf[seq][j]);
			    		dbuf[seq][j] = NULL;
		    		}
			}
		}
		
		//new PROM
		clear_AES_glo_cfg();
		msleep(10);
		
		if (AES_Entry.AES_tx_ring0)
			pci_free_consistent(NULL, NUM_AES_TX_DESC*sizeof(struct AES_txdesc), AES_Entry.AES_tx_ring0, AES_Entry.phy_aes_tx_ring0);
		if (AES_Entry.AES_rx_ring0)
			pci_free_consistent(NULL, NUM_AES_RX_DESC*sizeof(struct AES_rxdesc), AES_Entry.AES_rx_ring0, AES_Entry.phy_aes_rx_ring0);
		AES_Entry.AES_tx_ring0 = NULL;
		AES_Entry.AES_rx_ring0 = NULL;
		
		//Disable AES interrupt
		reg_int_mask=sysRegRead(AES_INT_MASK);
		sysRegWrite(AES_INT_MASK,reg_int_mask & ~(AES_FE_INT_ALL) );
		
		AesDescriptorInit();
		set_AES_glo_cfg();
		sysRegWrite(AES_INT_MASK, (reg_int_mask  & ~(AES_FE_INT_TX)) |(AES_FE_INT_RX));
	}
	return len;
}


void set_AES_glo_cfg(void)
{
	int AES_glo_cfg=0;
	int burst_mode = 0;
	//AES_glo_cfg = (AES_TX_WB_DDONE | AES_RX_DMA_EN | AES_TX_DMA_EN | AES_BT_SIZE_32DWORDS | AES_MUTI_ISSUE);
	if (burstlen==4)
		burst_mode = 0;
	else if (burstlen==8)
		burst_mode = 1;
	else if (burstlen==16)
		burst_mode = 2;
	else if (burstlen==32)
		burst_mode = 3;
	else
		burst_mode = 0;				
	printk("set burstmode=%02X\n",burst_mode);
	
	AES_glo_cfg = (AES_TX_WB_DDONE | AES_RX_DMA_EN | AES_TX_DMA_EN |(burst_mode<<4)|AES_DESC_5DW_INFO_EN|AES_RX_ANYBYTE_ALIGN );
	sysRegWrite(AES_GLO_CFG, AES_glo_cfg);
}
void clear_AES_glo_cfg(void)
{
	unsigned int regValue;

	regValue = sysRegRead(AES_GLO_CFG);
	regValue &= ~(AES_TX_WB_DDONE | AES_RX_DMA_EN | AES_TX_DMA_EN |(0x3<<4)|AES_DESC_5DW_INFO_EN|AES_RX_ANYBYTE_ALIGN  );
	sysRegWrite(AES_GLO_CFG, regValue);
	DBGPRINT(DBG_LOW, "Done\n");	
}

static int AesDescriptorInit(void)
{
	int		i;
	unsigned int	regVal;
	int bAESDescInit = 0;
/*	
	while(1)
	{
		regVal = sysRegRead(AES_GLO_CFG);
		if((regVal & AES_RX_DMA_BUSY))
		{
			printk("\n  RX_DMA_BUSY !!! ");
			continue;
		}
		if((regVal & AES_TX_DMA_BUSY))
		{
			printk("\n  TX_DMA_BUSY !!! ");
			continue;
		}
		break;
	}
*/
	udelay(1000);
	regVal = sysRegRead(AES_GLO_CFG);
	if((regVal & AES_RX_DMA_BUSY))
	{
		printk("\n  RX_DMA_BUSY !!! ");
		
	}
	if((regVal & AES_TX_DMA_BUSY))
	{
		printk("\n  TX_DMA_BUSY !!! ");
		
	}

	
	//initial TX ring0
	
	if (AES_Entry.AES_tx_ring0 == NULL)
	{	
		AES_Entry.AES_tx_ring0 = pci_alloc_consistent(NULL, NUM_AES_TX_DESC * sizeof(struct AES_txdesc), &AES_Entry.phy_aes_tx_ring0);
		printk("\nphy_tx_ring0 = 0x%08x, tx_ring0 = 0x%p\n", AES_Entry.phy_aes_tx_ring0, AES_Entry.AES_tx_ring0);
	}
	else
		bAESDescInit = 1;
			
	for (i=0; i < NUM_AES_TX_DESC; i++) {
		memset(&AES_Entry.AES_tx_ring0[i], 0, sizeof(struct AES_txdesc));
#if defined(AES_SCATTER)
		AES_Entry.AES_tx_ring0[i].aes_txd_info2.LS0 = 0;
#else
		AES_Entry.AES_tx_ring0[i].aes_txd_info2.LS0 = 0;
#endif		
		AES_Entry.AES_tx_ring0[i].aes_txd_info2.LS1 = 0;//1;
		AES_Entry.AES_tx_ring0[i].aes_txd_info2.DDONE = 1;
	}

	//initial RX ring0
	
	if (AES_Entry.AES_rx_ring0 == NULL)
	{	
		AES_Entry.AES_rx_ring0 = pci_alloc_consistent(NULL, NUM_AES_RX_DESC * sizeof(struct AES_rxdesc), &AES_Entry.phy_aes_rx_ring0);
		printk("\nphy_rx_ring0 = 0x%08x, rx_ring0 = 0x%p\n",AES_Entry.phy_aes_rx_ring0,AES_Entry.AES_rx_ring0);
	}
	else
		bAESDescInit = 1;
		

	for (i = 0; i < NUM_AES_RX_DESC; i++) {
		memset(&AES_Entry.AES_rx_ring0[i],0,sizeof(struct AES_rxdesc));
		AES_Entry.AES_rx_ring0[i].aes_rxd_info2.DDONE = 0;
#if defined(AES_SCATTER)
		AES_Entry.AES_rx_ring0[i].aes_rxd_info2.LS0 = 0;
#else
		AES_Entry.AES_rx_ring0[i].aes_rxd_info2.LS0 = 0;//1;
#endif		
	}	
	
	if (bAESDescInit == 0)
	{	  
		// AES_GLO_CFG
		regVal = sysRegRead(AES_GLO_CFG);
		regVal &= 0x000000FF;
		sysRegWrite(AES_GLO_CFG, regVal);
		regVal=sysRegRead(AES_GLO_CFG);
		/* Tell the adapter where the TX/RX rings are located. */
		//TX0
		sysRegWrite(AES_TX_BASE_PTR0, phys_to_bus((u32) AES_Entry.phy_aes_tx_ring0));
		sysRegWrite(AES_TX_MAX_CNT0, cpu_to_le32((u32) NUM_AES_TX_DESC));
		sysRegWrite(AES_TX_CTX_IDX0, 0);
		aes_tx_front_idx = 0;
		aes_tx_rear_idx = NUM_AES_TX_DESC-1;
		sysRegWrite(AES_RST_CFG, AES_PST_DTX_IDX0);
		printk("TX_CTX_IDX0 = %x\n", sysRegRead(AES_TX_CTX_IDX0));
		printk("TX_DTX_IDX0 = %x\n", sysRegRead(AES_TX_DTX_IDX0));
	
		    
		//RX0
		sysRegWrite(AES_RX_BASE_PTR0, phys_to_bus((u32) AES_Entry.phy_aes_rx_ring0));
		sysRegWrite(AES_RX_MAX_CNT0,  cpu_to_le32((u32) NUM_AES_RX_DESC));
		sysRegWrite(AES_RX_CALC_IDX0, cpu_to_le32((u32) (NUM_AES_RX_DESC - 1)));
		sysRegRead(AES_RX_CALC_IDX0);
		aes_rx_front_idx = 0;
		aes_rx_rear_idx = NUM_AES_RX_DESC-1;
		sysRegWrite(AES_RST_CFG, AES_PST_DRX_IDX0);
		printk("RX_CRX_IDX0 = %x\n", sysRegRead(AES_RX_CALC_IDX0));
		printk("RX_DRX_IDX0 = %x\n", sysRegRead(AES_RX_DRX_IDX0));
	}
	
	return 1;
}

static int __init AesEngineInit(void)
{
	struct proc_dir_entry *entry;
	uint32_t Ret=0;
	unsigned long reg_int_mask=0;
	printk("Enable MTK AesEngine Module (verson=%08X) \n", sysRegRead(AES_INFO));
	entry = create_proc_entry(PROCNAME, 0666, NULL);
    if (entry == NULL)
    {
        printk("AES Engine : unable to create /proc entry\n");
        return -1;
    }
    entry->read_proc = aes_engine_proc_read;
    entry->write_proc = aes_engine_proc_write;
    memset(&aes_engine_proc, 0, sizeof(aes_engine_proc_type));

	if (aes_irq_enabled)
	{	
		Ret = request_irq(SURFBOARDINT_AESENGINE, AesEngineIrqHandler, \
		    IRQF_DISABLED, "AES_ENGINE", NULL);
		
		if(Ret){
			printk("IRQ %d is not free.\n", SURFBOARDINT_AESENGINE);
			return 1;
		}
		printk("request IRQ=%d sucess\n",SURFBOARDINT_AESENGINE);
	}
		
	spin_lock_init(&(AES_Entry.page_lock));
	printk("reg_int_mask=%lu, INT_MASK= %x \n", reg_int_mask, sysRegRead(AES_INT_MASK));
	AesDescriptorInit();
	set_AES_glo_cfg();
	printk("AES_GLO_CFG = %x\n", sysRegRead(AES_GLO_CFG));
	
	memset(AesDoneIntCallback, 0, NUM_AES_RX_DESC*sizeof(uint32_t));
	memset(AES_userdata, 0, NUM_AES_RX_DESC*sizeof(AES_userdata_type));
	
	tasklet_init(&AesRxTasklet, AesRxHandler , 0);
	
	if (aes_irq_enabled)
	{
		sysRegWrite(AES_INT_MASK, (reg_int_mask  & ~(AES_FE_INT_TX)) |(AES_FE_INT_RX));
	}

	Ret = crypto_register_alg(&mcrypto_cbc_alg);
	printk("register aes-cbc crypto api to kernel\n");
	if (Ret)
	{
		printk("register aes-cbc crypto api to kernel failed !!!\n");	
		crypto_unregister_alg(&mcrypto_cbc_alg);
	}
	Ret = crypto_register_alg(&mcrypto_aes_alg);
	printk("register aes crypto api to kernel\n");
	if (Ret)
	{
		printk("register aes crypto api to kernel failed !!!\n");	
		crypto_unregister_alg(&mcrypto_aes_alg);
	}
	return Ret;
}



static void __exit AesEngineExit(void)
{
	unsigned long reg_int_mask=0;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	cancel_work_sync(&AES_Entry.reset_task);
#endif
	clear_AES_glo_cfg();
	msleep(10);

	if (AES_Entry.AES_tx_ring0 != NULL) {
		pci_free_consistent(NULL, NUM_AES_TX_DESC*sizeof(struct AES_txdesc), AES_Entry.AES_tx_ring0, AES_Entry.phy_aes_tx_ring0);
	}	
													
	pci_free_consistent(NULL, NUM_AES_RX_DESC*sizeof(struct AES_rxdesc), AES_Entry.AES_rx_ring0, AES_Entry.phy_aes_rx_ring0);
	printk("Free TX/RX Ring Memory!\n");
	printk("Disable AES Controller Module\n");
	//Disable AES interrupt
	reg_int_mask=sysRegRead(AES_INT_MASK);
	sysRegWrite(AES_INT_MASK,reg_int_mask & ~(AES_FE_INT_ALL) );
	if (aes_irq_enabled)
		free_irq(SURFBOARDINT_AESENGINE, NULL);
	remove_proc_entry(PROCNAME, pProcDir);

	crypto_unregister_alg(&mcrypto_cbc_alg);
	crypto_unregister_alg(&mcrypto_aes_alg);
}

int AesGenPattern(int tv, int seq, int enc)
{   
	int j, n;
	uint8_t* source;
	int pos = 0;

	if (enc==1)
	{	
		pattern[tv].mode |= ENCRYPTION;
		printk("Gen Encrypt pattern (seq=%d tv=%d klen=%d)\n",seq, tv, (aes_key_len[pattern[tv].mode&0x03]<<3));
	}
	else
	{
		pattern[tv].mode &= ~ENCRYPTION;	
		printk("Gen Decrypt pattern (seq=%d tv=%d klen=%d)\n",seq, tv, (aes_key_len[pattern[tv].mode&0x03]<<3));
	}	
	n = pattern[tv].src_multipage;
	sg_init_table(&sg_src_test[seq][0], n);
	if (enc)
	{	
		source = (uint8_t*)pattern[tv].plaintext;
		pattern[tv].totallen = pattern[tv].plainlen;
	}
	else
	{	
		source = (uint8_t*)pattern[tv].ciphertext;
		pattern[tv].totallen = pattern[tv].cipherlen;
	}	
	tv_key[seq] = (void *)__get_free_page(GFP_KERNEL);
	if (!tv_key[seq])
        goto EXIT;
	memset(tv_key[seq], 0, PAGE_SIZE);
	memcpy(tv_key[seq], pattern[tv].key, aes_key_len[pattern[tv].mode&0x03]);
		
	for (j = 0 ; j < n ; j++)
	{
    	sbuf[seq][j] = (void *)__get_free_page(GFP_KERNEL);
    	if (!sbuf[seq][j])
        	goto EXIT;
    	memset(sbuf[seq][j], 0, PAGE_SIZE);
    	
    	if (j==(n-1))
    	{
    		if ((pos+pattern[tv].src_length[j]-pattern[tv].totallen) > 0)
    		{
    			int npadding = (pos+pattern[tv].src_length[j]-pattern[tv].totallen);
    			memset(sbuf[seq][j]+pattern[tv].src_offset[j]+pattern[tv].src_length[j]-npadding, (uint8_t)npadding, npadding);
    			memcpy(sbuf[seq][j]+pattern[tv].src_offset[j], source+pos, pattern[tv].src_length[j]-npadding);
    		}
    		else
    			memcpy(sbuf[seq][j]+pattern[tv].src_offset[j], source+pos, pattern[tv].src_length[j]);
    	}
    	else
			memcpy(sbuf[seq][j]+pattern[tv].src_offset[j], source+pos, pattern[tv].src_length[j]);
		
		sg_set_buf(&sg_src_test[seq][j], sbuf[seq][j]+pattern[tv].src_offset[j], pattern[tv].src_length[j]);
		//{
		//	int jj;
			//uint8_t* src = sbuf[seq][j]+pattern[tv].src_offset[j];
			//for (jj =0; jj< pattern[tv].src_length[j]; jj++)
		//	{
				//printk("%02X ", *src);
				//src++;
		//	}
			//printk("\n");
		//}
		pos += pattern[tv].src_length[j];
	}
	
	n = pattern[tv].dst_multipage;
	sg_init_table(&sg_dst_test[seq][0], n);
	for (j = 0; j < n; j++) 
	{
		dbuf[seq][j] = (void *)__get_free_page(GFP_KERNEL);
    	if (!dbuf[seq][j])
        	goto EXIT;
		memset(dbuf[seq][j], 0x5A, PAGE_SIZE);
		sg_set_buf(&sg_dst_test[seq][j], dbuf[seq][j]+pattern[tv].dst_offset[j], pattern[tv].dst_length[j]);
	}
	
	return 0;
EXIT:
	printk("%s : OOM\n",__func__);
	if (tv_key[seq])
		free_page((u32)tv_key[seq]);

	n = pattern[tv].src_multipage;
	for (j = 0 ; j < n ; j++)
		if (sbuf[seq][j])
			free_page((unsigned long)sbuf[seq][j]);
			
	
	n = pattern[tv].dst_multipage;
	for (j = 0 ; j < n ; j++)
		if (dbuf[seq][j])
			free_page((unsigned long)dbuf[seq][j]);
        
	printk("AES pattern generation failed\n");		
    return -ENOMEM;

}

int AesResultVerify(int tv, int seq, int enc)
{
	int j, n, ret, totallen, remain;
	int pos = 0;
	uint8_t * source;
	
	if (enc==1)
	{	
		source = (uint8_t*)pattern[tv].ciphertext;
		totallen = pattern[tv].cipherlen;
		printk("Verify Encrypt pattern (seq=%d tv=%d totallen=%d)\n",seq, tv, totallen);
	}
	else
	{	
		source = (uint8_t*)pattern[tv].plaintext;
		totallen = pattern[tv].plainlen;
		printk("Verify Decrypt pattern (seq=%d tv=%d totallen=%d)\n",seq, tv, totallen);
	}
	n = pattern[tv].dst_multipage;
	
	for (j = 0 ; j < n ; j++)
	{
		int jj;
		int len = 16, cmplen = 0;
		
		uint8_t* src, *dst;
		if (totallen < pattern[tv].dst_length[j])
		{	
			cmplen = totallen;
			printk("Padding %d bytes\n",pattern[tv].dst_length[j]-totallen);
		}
		else
			cmplen = pattern[tv].dst_length[j];
			
		totallen-=cmplen;
		printk("compare TestVector[%08X] DST[%08X] len=%d\n", (u32)(source+pos), (u32)(dbuf[seq][j]+pattern[tv].dst_offset[j]), cmplen);
		ret = strncmp(dbuf[seq][j]+pattern[tv].dst_offset[j], source+pos, cmplen);
		printk("Verify TestVector[%d][%d] ret=%d done\n",tv,j,ret);
		dst = dbuf[seq][j]+pattern[tv].dst_offset[j];
		src = source+pos;
		len = (cmplen < 16) ? cmplen : 16;
		remain = cmplen;	
		for (jj = 0; jj < cmplen; jj+=16)
		{	
			int kk;
			uint8_t* psrc, *pdst;
			psrc = src;
			for (kk=0; kk<len; kk++)
				DBGPRINT(DBG_LOW, "%02X ",*psrc++);
			DBGPRINT(DBG_LOW, "\n");	
			pdst = dst;
			for (kk=0; kk<len; kk++)
				DBGPRINT(DBG_LOW, "%02X ",*pdst++);
			DBGPRINT(DBG_LOW, "\n\n");
			if (remain < 16)
				len = remain;	
			for (kk=0; kk<len; kk++,src++,dst++)
			{
				if (*src!=*dst)
				{	
					printk("[==== error ====][%d][%d]\n",j,kk);
					goto EXIT;
				}	
			}
			remain -= 16;
				
			
		}
		pos += cmplen;
	}
EXIT:      		
    return 0;
}
#ifdef DBG
IMPORT_SYMBOL(mips_cpu_feq);
#endif
MODULE_DESCRIPTION("MTK AES Engine Module");
MODULE_AUTHOR("Qwert");
MODULE_LICENSE("GPL");
MODULE_VERSION(MOD_VERSION_AES_ENGINE);
module_param_named(irq, aes_irq_enabled, int, S_IRUGO);
module_param_named(b, burstlen, int, S_IRUGO);
module_init(AesEngineInit);
module_exit(AesEngineExit);
