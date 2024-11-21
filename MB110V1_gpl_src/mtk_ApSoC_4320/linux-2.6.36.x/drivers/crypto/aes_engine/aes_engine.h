#ifndef _MTK_AES_ENGINE
#define _MTK_AES_ENGINE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <asm/rt2880/rt_mmap.h>
#endif

#define MOD_VERSION_AES_ENGINE 			"0.1"
#define NUM_AES_RX_DESC     16
#define NUM_AES_TX_DESC    	16

#define phys_to_bus(a) (a & 0x1FFFFFFF)

#define PHYS_TO_K1(physaddr) KSEG1ADDR(physaddr)

extern int aes_dbg_print;

#define DBG_LOW		3
#define DBG_MID		2
#define DBG_HIGH	1

//#define DBG
#ifdef DBG
#define DBGPRINT(level, fmt, args...)      do{ if ((aes_dbg_print>0)&&(aes_dbg_print>=level)) printk(fmt, ## args);}while(0)
#else
#define DBGPRINT(level, fmt, args...)      do {}while(0)
#endif

#define sysRegRead(phys)        \
        (*(volatile unsigned int *)PHYS_TO_K1(phys))

//#ifdef DBG
#if 0
#define sysRegWrite(phys, val)  \
        do {\
        	(*(volatile unsigned int *)PHYS_TO_K1(phys)) = (val);\
        	DBGPRINT("AES[%08X]=%08X\n",(u32)phys,(u32)sysRegRead(phys));\
        }while(0);
#else
#define sysRegWrite(phys, val)  \
        ((*(volatile unsigned int *)PHYS_TO_K1(phys)) = (val))
#endif
	

#define u_long	unsigned long
#define u32	unsigned int
#define u16	unsigned short


/* 1. AES */
#define AES_TX_BASE_PTR0            (RALINK_AES_ENGINE_BASE + 0x000)
#define AES_TX_MAX_CNT0             (RALINK_AES_ENGINE_BASE + 0x004)
#define AES_TX_CTX_IDX0             (RALINK_AES_ENGINE_BASE + 0x008)
#define AES_TX_DTX_IDX0             (RALINK_AES_ENGINE_BASE + 0x00C)

#define AES_RX_BASE_PTR0            (RALINK_AES_ENGINE_BASE + 0x100)
#define AES_RX_MAX_CNT0             (RALINK_AES_ENGINE_BASE + 0x104)
#define AES_RX_CALC_IDX0            (RALINK_AES_ENGINE_BASE + 0x108)
#define AES_RX_DRX_IDX0             (RALINK_AES_ENGINE_BASE + 0x10C)

#define AES_INFO                    (RALINK_AES_ENGINE_BASE + 0x200)
#define AES_GLO_CFG 				(RALINK_AES_ENGINE_BASE + 0x204)
#define AES_RST_IDX            		(RALINK_AES_ENGINE_BASE + 0x208)
#define AES_RST_CFG            		(AES_RST_IDX)
#define AES_DLY_INT_CFG             (RALINK_AES_ENGINE_BASE + 0x20C)
#define HAES_FREEQ_THRES            (RALINK_AES_ENGINE_BASE + 0x210)
#define AES_INT_STATUS              (RALINK_AES_ENGINE_BASE + 0x220)
#define AES_FE_INT_STATUS		    (AES_INT_STATUS)
#define AES_INT_MASK                (RALINK_AES_ENGINE_BASE + 0x228)
//#define FE_INT_ENABLE		(INT_MASK)

/*BUS MATRIX*/
#define OCP_CFG0	  (RALINK_RBUS_MATRIXCTL_BASE+0x000);
#define OCP_CFG1      (RALINK_RBUS_MATRIXCTL_BASE+0x004);
#define DYN_CFG0      (RALINK_RBUS_MATRIXCTL_BASE+0x010);
#define DYN_CFG1	  (RALINK_RBUS_MATRIXCTL_BASE+0x014);
#define DYN_CFG2      (RALINK_RBUS_MATRIXCTL_BASE+0x018);
#define DYN_CFG3      (RALINK_RBUS_MATRIXCTL_BASE+0x01C);
#define IOCU_CFG      (RALINK_RBUS_MATRIXCTL_BASE+0x020);


/* ====================================== */
/* ====================================== */
#define PSE_RESET       (1<<0)
/* ====================================== */
#define AES_PST_DRX_IDX1       (1<<17)
#define AES_PST_DRX_IDX0       (1<<16)
#define AES_PST_DTX_IDX3       (1<<3)
#define AES_PST_DTX_IDX2       (1<<2)
#define AES_PST_DTX_IDX1       (1<<1)
#define AES_PST_DTX_IDX0       (1<<0)

#define AES_RX_2B_OFFSET	    (1<<31)
#define AES_RX_ANYBYTE_ALIGN   (1<<12)
#define AES_DESC_5DW_INFO_EN	(1<<11)
#define AES_MUTI_ISSUE        (1<<10)
#define AES_TWO_BUFFER        (1<<9)
#define AES_TX_WB_DDONE       (1<<6)
#define AES_RX_DMA_BUSY       (1<<3)
#define AES_TX_DMA_BUSY       (1<<1)
#define AES_RX_DMA_EN         (1<<2)
#define AES_TX_DMA_EN         (1<<0)

#define AES_BT_SIZE_4DWORDS     (0<<4)
#define AES_BT_SIZE_8DWORDS     (1<<4)
#define AES_BT_SIZE_16DWORDS    (2<<4)
#define AES_BT_SIZE_32DWORDS    (3<<4)

#define AES_RX_COHERENT      (1<<31)
#define AES_RX_DLY_INT       (1<<30)
#define AES_TX_COHERENT      (1<<29)
#define AES_TX_DLY_INT       (1<<28)
#define AES_RX_DONE_INT0     (1<<16)
#define AES_TX_DONE_INT0     (1)
//#define AES_TX_DONE_BIT      (1<<31)
//#define AES_TX_LS0           (1<<30)
/*
#define FE_INT_ALL		(AES_TX_DONE_INT3 | AES_TX_DONE_INT2 | \
			         AES_TX_DONE_INT1 | AES_TX_DONE_INT0 | \
	                         AES_RX_DONE_INT0 | AES_RX_DONE_INT1)*/
//#define AES_FE_INT_ALL    (AES_TX_DONE_INT0 | AES_RX_DONE_INT0 | AES_RX_COHERENT | AES_TX_COHERENT | AES_RX_DLY_INT | AES_TX_DLY_INT)                    
#define AES_FE_INT_ALL			(AES_RX_DONE_INT0)
#define AES_FE_INT_TX			(AES_TX_DONE_INT0)
#define AES_FE_INT_RX     (AES_RX_DONE_INT0)
#define AES_FE_INT_STATUS_REG (*(volatile unsigned long *)(FE_INT_STATUS))
#define AES_FE_INT_STATUS_CLEAN(reg) (*(volatile unsigned long *)(FE_INT_STATUS)) = reg

// Define Whole FE Reset Register
#define RSTCTRL         (RALINK_SYSCTL_BASE + 0x34)
/*=========================================
      AES AES_RX Descriptor Format define
=========================================*/
//-------------------------------------------------
typedef struct _AES_RXD_INFO2_    AES_RXD_INFO2_T;

struct _AES_RXD_INFO2_
{
	volatile unsigned int    RSV                 	: 16;
	volatile unsigned int    SDL0                  	: 14;
	volatile unsigned int    LS0                   	: 1;
	volatile unsigned int    DDONE	             	: 1;
};
//-------------------------------------------------
typedef struct _AES_RXD_INFO4_    AES_RXD_INFO4_T;

struct _AES_RXD_INFO4_
{
	volatile unsigned int	KEY_LEN				: 2;
	volatile unsigned int	ENC					: 1;
	volatile unsigned int	UDV					: 1;
	volatile unsigned int	CBC					: 1;
	volatile unsigned int	IVR					: 1;
	volatile unsigned int	KIU					: 1;
	volatile unsigned int	RSV					: 25;	
};


struct AES_rxdesc {
	unsigned int SDP0;
	AES_RXD_INFO2_T aes_rxd_info2;
	unsigned int user_data;
	union {
		AES_RXD_INFO4_T bitfield;
		uint32_t value;
	}aes_rxd_info4;
	unsigned int IV[4];
}__attribute__((aligned(32)));


/*=========================================
      AES AES_TX Descriptor Format define
=========================================*/
//-------------------------------------------------
typedef struct _AES_TXD_INFO2_    AES_TXD_INFO2_T;

struct _AES_TXD_INFO2_
{
	volatile unsigned int    SDL1                 	: 14;
	volatile unsigned int    LS1                   	: 1;
	volatile unsigned int    RSV                   	: 1;
	volatile unsigned int    SDL0                  	: 14;
	volatile unsigned int    LS0                   	: 1;
	volatile unsigned int    DDONE             		: 1;
};
//-------------------------------------------------
typedef struct _AES_TXD_INFO4_    AES_TXD_INFO4_T;

struct _AES_TXD_INFO4_
{
	volatile unsigned int	KEY_LEN				: 2;
	volatile unsigned int	ENC					: 1;
	volatile unsigned int	UDV					: 1;
	volatile unsigned int	CBC					: 1;
	volatile unsigned int	IVR					: 1;
	volatile unsigned int	KIU					: 1;
	volatile unsigned int	RSV					: 25;			
};


struct AES_txdesc {
	unsigned int SDP0;
	AES_TXD_INFO2_T aes_txd_info2;
	unsigned int SDP1;
	union {
		AES_TXD_INFO4_T bitfield;
		uint32_t value;
	}aes_txd_info4;
	unsigned int IV[4];
}__attribute__((aligned(32)));

typedef struct AES_userdata_t {
	unsigned int orig_SDP0;
	unsigned int orig_SDL;
	unsigned int new_SDP0;
	unsigned int new_SDL;
}AES_userdata_type;

struct AesReqEntry {

    unsigned int  	aes_tx_full;
    unsigned int	phy_aes_tx_ring0;
    unsigned int	phy_aes_rx_ring0;
    spinlock_t          page_lock;              /* Page register locks */
    struct AES_txdesc *AES_tx_ring0;

	struct AES_rxdesc *AES_rx_ring0;
	int (*DoneIntCallback)(uint32_t, uint32_t);
	struct work_struct  reset_task;
};
		
int AesProcessScatterGather( 
	struct scatterlist* sg_src,
	struct scatterlist* sg_dst,
	uint32_t TransCount,
	uint8_t*	Key,
	uint8_t*	IV,
	uint32_t	aes_mode
	);
void AesRxHandler(unsigned long data);
int AesGenPattern(int tv, int seq, int enc);
int AesResultVerify(int tv, int seq,int enc);
int AesKickEngine(void);

#define AES_128		0
#define AES_192		1
#define AES_256		2

#define ENCRYPTION		(1<<2)
#define CARRY_USERDATA	(1<<3)
#define CBC_MODE		(1<<4)
#define RESTORE_IV		(1<<5)
#define VALID_IV		(1<<6)			
#endif

#define PROCNAME    "aes_engine"

