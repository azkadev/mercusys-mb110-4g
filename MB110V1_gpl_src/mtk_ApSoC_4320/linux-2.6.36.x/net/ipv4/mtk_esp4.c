/************************************************************************
 *
 *	Copyright (C) 2012 MediaTek Technologies, Corp.
 *	All Rights Reserved.
 *
 * MediaTek Confidential; Need to Know only.
 * Protected as an unpublished work.
 *
 * The computer program listings, specifications and documentation
 * herein are the property of MediaTek Technologies, Co. and shall
 * not be reproduced, copied, disclosed, or used in whole or in part
 * for any reason without the prior express written permission of
 * MediaTek Technologeis, Co.
 *
 *************************************************************************/


#include <linux/err.h>
#include <linux/module.h>
#include <linux/version.h>
#include <net/ip.h>
#include <net/xfrm.h>
#include <net/esp.h>
#include <asm/scatterlist.h>
#include <linux/crypto.h>
#include <linux/kernel.h>
#include <linux/pfkeyv2.h>
#include <linux/random.h>
#include <net/icmp.h>
#include <net/protocol.h>
#include <net/udp.h>

#include <net/mtk_esp.h>
#include <linux/netfilter_ipv4.h>

/************************************************************************
*                          C O N S T A N T S
*************************************************************************
*/
#define IPESC_EIP93_ADAPTERS	16
#define HASH_MD5_HMAC			"hmac(md5)"
#define HASH_SHA1_HMAC			"hmac(sha1)"
#define HASH_SHA256_HMAC		"hmac(sha256)"
#define HASH_NULL_HMAC 			"hmac(digest_null)"
#define HASH_IPAD				0x36363636
#define HASH_OPAD				0x5c5c5c5c
#define CIPHER_DES_CBC			"cbc(des)"
#define CIPHER_3DES_CBC			"cbc(des3_ede)"
#define CIPHER_AES_CBC			"cbc(aes)"
#define CIPHER_NULL_ECB			"ecb(cipher_null)"
#define SKB_QUEUE_MAX_SIZE		3000//100

#define RALINK_HWCRYPTO_NAT_T	1
#define FEATURE_AVOID_QUEUE_PACKET	1
/************************************************************************
*      P R I V A T E    S T R U C T U R E    D E F I N I T I O N
*************************************************************************
*/


/************************************************************************
*              P R I V A T E     D A T A
*************************************************************************
*/
static ipsecEip93Adapter_t 	*ipsecEip93AdapterListOut[IPESC_EIP93_ADAPTERS];
static ipsecEip93Adapter_t 	*ipsecEip93AdapterListIn[IPESC_EIP93_ADAPTERS];
static spinlock_t 			cryptoLock;
static spinlock_t				ipsec_adapters_lock;
static eip93DescpHandler_t 	resDescpHandler;

mcrypto_proc_type 			mcrypto_proc;
EXPORT_SYMBOL(mcrypto_proc);

/************************************************************************
*              E X T E R N A L     D A T A
*************************************************************************
*/
int 
(*ipsec_packet_put)(
	eip93DescpHandler_t *descpHandler, 
	struct sk_buff *skb
);
int 
(*ipsec_packet_get)(
	eip93DescpHandler_t *descpHandler
);
bool 
(*ipsec_eip93CmdResCnt_check)(
	void
);
int 
(*ipsec_preComputeIn_cmdDescp_set)(
	ipsecEip93Adapter_t *currAdapterPtr,
	//unsigned int hashAlg, 
	unsigned int direction
);
int 
(*ipsec_preComputeOut_cmdDescp_set)(
	ipsecEip93Adapter_t *currAdapterPtr, 
	//unsigned int hashAlg,
	unsigned int direction
);
int 
(*ipsec_cmdHandler_cmdDescp_set)(
	ipsecEip93Adapter_t *currAdapterPtr, 
	unsigned int direction,
	unsigned int cipherAlg, 
	unsigned int hashAlg, 
	unsigned int digestWord,
	unsigned int cipherMode, 
	unsigned int enHmac, 
	unsigned int aesKeyLen, 
	unsigned int *cipherKey, 
	unsigned int keyLen, 
	unsigned int spi, 
	unsigned int padCrtlStat
);
void 
(*ipsec_espNextHeader_set)(
	eip93DescpHandler_t *cmdHandler, 
	unsigned char protocol	
);
unsigned char 
(*ipsec_espNextHeader_get)(
	eip93DescpHandler_t *resHandler
);
unsigned int 
(*ipsec_pktLength_get)(
	eip93DescpHandler_t *resHandler
);
unsigned int 
(*ipsec_eip93HashFinal_get)(
	eip93DescpHandler_t *resHandler
);
unsigned int 
(*ipsec_eip93UserId_get)(
	eip93DescpHandler_t *resHandler
);

void 
(*ipsec_addrsDigestPreCompute_free)(
	ipsecEip93Adapter_t *currAdapterPtr
);

void 
(*ipsec_cmdHandler_free)(
	eip93DescpHandler_t *cmdHandler
);

void 
(*ipsec_hashDigests_get)(
	ipsecEip93Adapter_t *currAdapterPtr
);

void 
(*ipsec_hashDigests_set)(
	ipsecEip93Adapter_t *currAdapterPtr,
	unsigned int isInOrOut
);

unsigned int 
(*ipsec_espSeqNum_get)(
	eip93DescpHandler_t *resHandler
);

EXPORT_SYMBOL(ipsec_packet_put);
EXPORT_SYMBOL(ipsec_packet_get);
EXPORT_SYMBOL(ipsec_eip93CmdResCnt_check);
EXPORT_SYMBOL(ipsec_preComputeIn_cmdDescp_set);
EXPORT_SYMBOL(ipsec_preComputeOut_cmdDescp_set);
EXPORT_SYMBOL(ipsec_cmdHandler_cmdDescp_set);
EXPORT_SYMBOL(ipsec_espNextHeader_set);
EXPORT_SYMBOL(ipsec_espNextHeader_get);
EXPORT_SYMBOL(ipsec_pktLength_get);
EXPORT_SYMBOL(ipsec_eip93HashFinal_get);
EXPORT_SYMBOL(ipsec_eip93UserId_get);
EXPORT_SYMBOL(ipsec_addrsDigestPreCompute_free);
EXPORT_SYMBOL(ipsec_cmdHandler_free);
EXPORT_SYMBOL(ipsec_hashDigests_get);
EXPORT_SYMBOL(ipsec_hashDigests_set);
EXPORT_SYMBOL(ipsec_espSeqNum_get);

#ifdef MCRYPTO_DBG
#define ra_dbg 	printk
#else
#define ra_dbg(fmt, arg...) do {}while(0)
#endif

#ifdef MCRYPTO_DBG
static void skb_dump(struct sk_buff* sk, const char* func,int line) {
        unsigned int i;

        ra_dbg("(%d)skb_dump: [%s] with len %d (%08X) headroom=%d tailroom=%d\n",
                line,func,sk->len,sk,
                skb_headroom(sk),skb_tailroom(sk));

        for(i=(unsigned int)sk->head;i<=(unsigned int)sk->data + 160;i++) {
                if((i % 16) == 0)
                        ra_dbg("\n");
                if(i==(unsigned int)sk->data) printk("{");
                //if(i==(unsigned int)sk->h.raw) printk("#");
                //if(i==(unsigned int)sk->nh.raw) printk("|");
                //if(i==(unsigned int)sk->mac.raw) printk("*");
                ra_dbg("%02x ",*((unsigned char*)i));
                if(i==(unsigned int)(sk->tail)-1) printk("}");
        }
        ra_dbg("\n");
}
#else
#define skb_dump //skb_dump
#endif
/************************************************************************
*              P R I V A T E     F U N C T I O N S
*************************************************************************
*/
/*_______________________________________________________________________
**function name: ipsec_hashDigest_preCompute
**
**description:
*   EIP93 can only use Hash Digests (not Hash keys) to do authentication!
*	This funtion is to use EIP93 to generate Hash Digests from Hash keys!
*	Only the first packet for a IPSec flow need to do this!
**parameters:
*   x -- point to the structure that stores IPSec SA information
*	currAdapterPtr -- point to the structure that stores needed info
*		for Hash Digest Pre-Compute. After Pre-Compute is done,
*		currAdapterPtr->addrsPreCompute is used to free resource.
*	digestPreComputeDir -- indicate direction for encryption or
*		decryption.
**global:
*   none
**return:
*   -EPERM, -ENOMEM -- failed: the pakcet will be dropped!
*	1 -- success
**call:
*   none
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
static int 
ipsec_hashDigest_preCompute(
	struct xfrm_state *x, 
	ipsecEip93Adapter_t *currAdapterPtr, 
	unsigned int digestPreComputeDir
)
{
	char hashKeyName[32];
	unsigned int blkSize, blkWord, digestWord, hashKeyLen, hashKeyWord;
	unsigned int *ipad, *opad, *hashKey, *hashKeyTank;
	dma_addr_t	ipadPhyAddr, opadPhyAddr;
	unsigned int *pIDigest, *pODigest;
	unsigned int i, j;
	unsigned long flags;
	int errVal;
	
	addrsDigestPreCompute_t* addrsPreCompute;
	
	if (x->aalg)
	{	
	strcpy(hashKeyName, x->aalg->alg_name);
	hashKeyLen = (x->aalg->alg_key_len+7)/8;
	}
	else
	{
		strcpy(hashKeyName, HASH_NULL_HMAC);
		hashKeyLen = 0;
		currAdapterPtr->isHashPreCompute = 3;
		return 1;
	}
	
	hashKeyWord = hashKeyLen >> 2;

	if (strcmp(hashKeyName, HASH_MD5_HMAC) == 0)
	{
		blkSize = 64; //bytes
		digestWord = 4; //words
	}
	else if (strcmp(hashKeyName, HASH_SHA1_HMAC) == 0)
	{
		blkSize = 64; //bytes
		digestWord = 5; //words	
	}
	else if (strcmp(hashKeyName, HASH_SHA256_HMAC) == 0)
	{
		blkSize = 64; //bytes
		digestWord = 8; //words	
	}
	else if (strcmp(hashKeyName, HASH_NULL_HMAC) == 0)
	{
		blkSize = 64; //bytes
		digestWord = 0; //words	
	}
	else
	{
		printk("\n !Unsupported Hash Algorithm by EIP93: %s! \n", hashKeyName);
		return -EPERM;
	}

	
	addrsPreCompute = (addrsDigestPreCompute_t *) kzalloc(sizeof(addrsDigestPreCompute_t), GFP_KERNEL);
	if (unlikely(addrsPreCompute == NULL))
	{
		printk("\n\n !!kmalloc for addrsPreCompute failed!! \n\n");
		return -ENOMEM;
	}
	currAdapterPtr->addrsPreCompute = addrsPreCompute;
	
	hashKeyTank = (unsigned int *) kzalloc(blkSize, GFP_KERNEL);
	if (unlikely(hashKeyTank == NULL))
	{
		printk("\n\n !!kmalloc for hashKeyTank failed!! \n\n");
		errVal = -ENOMEM;
		goto free_addrsPreCompute;
	}
	addrsPreCompute->hashKeyTank = hashKeyTank;
	
	ipad = (unsigned int *) dma_alloc_coherent(NULL, blkSize, &ipadPhyAddr, GFP_DMA);
	if (unlikely(ipad == NULL))
	{
		printk("\n\n !!dma_alloc for ipad failed!! \n\n");
		errVal = -ENOMEM;
		goto free_hashKeyTank;
	}
	addrsPreCompute->ipadHandler.addr = (unsigned int)ipad;
	addrsPreCompute->ipadHandler.phyAddr = ipadPhyAddr;
	addrsPreCompute->blkSize = blkSize;
	
	opad = (unsigned int *) dma_alloc_coherent(NULL, blkSize, &opadPhyAddr, GFP_DMA);
	if (unlikely(opad == NULL))
	{
		printk("\n\n !!dma_alloc for opad failed!! \n\n");
		errVal = -ENOMEM;
		goto free_ipad;
	}
	addrsPreCompute->opadHandler.addr = (unsigned int)opad;
	addrsPreCompute->opadHandler.phyAddr = opadPhyAddr;	

	blkWord = blkSize >> 2;
	if (x->aalg)
	{	
	hashKey = (unsigned int *)x->aalg->alg_key;
	                                     
	if(hashKeyLen <= blkSize)
	{
		for(i = 0; i < hashKeyWord; i++)
		{
			hashKeyTank[i] = hashKey[i];
		}
		for(j = i; j < blkWord; j++)
		{
			hashKeyTank[j] = 0x0;
		}
	}
	else
	{
		// EIP93 supports md5, sha1, sha256. Their hash key length and their function output length should be the same, which are 128, 160, and 256 bits respectively! Their block size are 64 bytes which are always larger than all of their hash key length! 
		printk("\n !Unsupported hashKeyLen:%d by EIP93! \n", hashKeyLen);
		errVal = -EPERM;
		goto free_opad;
	}
	}
	else
		memset(hashKeyTank, 0, blkSize);
	
	for(i=0; i<blkWord; i++)
	{
		ipad[i] = HASH_IPAD;
		opad[i] = HASH_OPAD;
		ipad[i] ^= hashKeyTank[i];
		opad[i] ^= hashKeyTank[i];			
	}

	pIDigest = (unsigned int *) kzalloc(sizeof(unsigned int) << 3, GFP_KERNEL);
	if(pIDigest == NULL)
	{
		printk("\n\n !!kmalloc for Hash Inner Digests failed!! \n\n");
		errVal = -ENOMEM;
		goto free_opad;
	}
	addrsPreCompute->pIDigest = pIDigest;
	
	pODigest = (unsigned int *) kzalloc(sizeof(unsigned int) << 3, GFP_KERNEL);
	if(pODigest == NULL)
	{
		printk("\n\n !!kmalloc for Hash Outer Digests failed!! \n\n");
		errVal = -ENOMEM;
		goto free_pIDigest;
	}
	addrsPreCompute->pODigest = pODigest;
		
	addrsPreCompute->digestWord = digestWord;

	currAdapterPtr->isHashPreCompute = 0; //pre-compute init	

	/* start pre-compute for Hash Inner Digests */
	errVal = ipsec_preComputeIn_cmdDescp_set(currAdapterPtr, digestPreComputeDir);
	if (errVal < 0)
	{
		goto free_pODigest;
	}

	spin_lock(&cryptoLock);
	while (ipsec_eip93CmdResCnt_check())
	{	
	ipsec_packet_put(addrsPreCompute->cmdHandler, NULL); //mtk_packet_put()
		break;
	}
	spin_unlock(&cryptoLock);
	
	/* start pre-compute for Hash Outer Digests */	
	errVal = ipsec_preComputeOut_cmdDescp_set(currAdapterPtr, digestPreComputeDir);
	if (errVal < 0)
	{
		goto free_pODigest;
	}
	
	spin_lock(&cryptoLock);
	while (ipsec_eip93CmdResCnt_check())
	{		
	ipsec_packet_put(addrsPreCompute->cmdHandler, NULL); //mtk_packet_put()
		break;
	}
	spin_unlock(&cryptoLock);

	return 1; //success
	

free_pODigest:
	kfree(pODigest);
free_pIDigest:
	kfree(pIDigest);
free_opad:
	dma_free_coherent(NULL, blkSize, opad, opadPhyAddr);		
free_ipad:
	dma_free_coherent(NULL, blkSize, ipad, ipadPhyAddr);		
free_hashKeyTank:
	kfree(hashKeyTank);
free_addrsPreCompute:
	kfree(addrsPreCompute);
	currAdapterPtr->addrsPreCompute = NULL;	

	return errVal;	
}

/*_______________________________________________________________________
**function name: ipsec_cmdHandler_prepare
**
**description:
*   Prepare a command handler for a IPSec flow. This handler includes 
*	all needed information for EIP93 to do encryption/decryption.
*	Only the first packet for a IPSec flow need to do this!
**parameters:
*   x -- point to the structure that stores IPSec SA information
*	currAdapterPtr -- point to the structure that will stores the
*		command handler
*	cmdHandlerDir -- indicate direction for encryption or decryption.
**global:
*   none
**return:
*   -EPERM, -ENOMEM -- failed: the pakcet will be dropped!
*	1 -- success
**call:
*   none
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
static int 
ipsec_cmdHandler_prepare(
	struct xfrm_state *x, 
	ipsecEip93Adapter_t *currAdapterPtr,
	unsigned int cmdHandlerDir
)
{
	int errVal;
	struct esp_data *esp = x->data;
	int padBoundary = ALIGN(crypto_aead_blocksize(esp->aead), 4);
	unsigned int padCrtlStat, keyLen;
	char nameString[32];
	unsigned int cipherAlg, cipherMode, aesKeyLen = 0, hashAlg, enHmac;
	unsigned int *cipherKey;
	unsigned int addedLen = 0;

	addedLen += 8; //for esp header	

	/* decide pad boundary */
	switch(padBoundary){
		case 1:
			padCrtlStat = 0x1;
			addedLen += 1;
			break;
		case 4:
			padCrtlStat = 0x1 << 1;
			addedLen += 4;
			break;
		case 8:
			padCrtlStat = 0x1 << 2;
			addedLen += 8;
			break;
		case 16:
			padCrtlStat = 0x1 << 3;
			addedLen += 16;
			break;
		case 32:
			padCrtlStat = 0x1 << 4;
			addedLen += 32;
			break;
		case 64:
			padCrtlStat = 0x1 << 5;
			addedLen += 64;
			break;
		case 128:
			padCrtlStat = 0x1 << 6;
			addedLen += 128;
			break;
		case 256:
			padCrtlStat = 0x1 << 7;
			addedLen += 256;
			break;
		default:
			printk("\n !Unsupported pad boundary (%d) by EIP93! \n", padBoundary);
			errVal = -EPERM;
			goto free_addrsPreComputes;
	}
	
	
	/* decide cipher */
	strcpy(nameString, x->ealg->alg_name);

	keyLen = (x->ealg->alg_key_len+7)/8;
	if(strcmp(nameString, CIPHER_DES_CBC) == 0)
	{
		cipherAlg = 0x0; //des
		cipherMode = 0x1; //cbc
		addedLen += (8 + (8 + 1)); //iv + (esp trailer + padding)
	}
	else if(strcmp(nameString, CIPHER_3DES_CBC) == 0)
	{
		cipherAlg = 0x1; //3des
		cipherMode = 0x1; //cbc
		addedLen += (8 + (8 + 1)); //iv + (esp trailer + padding)
	}
	else if(strcmp(nameString, CIPHER_AES_CBC) == 0)
	{
		cipherAlg = 0x3; //aes
		cipherMode = 0x1; //cbc
		addedLen += (16 + (16 + 1)); //iv + (esp trailer + padding)

		switch(keyLen << 3) //keyLen*8
		{ 
			case 128:
				aesKeyLen = 0x2;
				break;
			case 192:
				aesKeyLen = 0x3;
				break;
			case 256:
				aesKeyLen = 0x4;
				break;
			default:
				printk("\n !Unsupported AES key length (%d) by EIP93! \n", keyLen << 3);
				errVal = -EPERM;
				goto free_addrsPreComputes;
		}
	}
	else if(strcmp(nameString, CIPHER_NULL_ECB) == 0)
	{
		cipherAlg = 0xf; //null
		cipherMode = 0x0; //ecb
		addedLen += (8 + (16 + 1) + 16); //iv + (esp trailer + padding) + ICV
	}
	else
	{
		printk("\n !Unsupported Cipher Algorithm (%s) by EIP93! \n", nameString);
		errVal = -EPERM;
		goto free_addrsPreComputes;
	}

	
	/* decide hash */
	if (x->aalg==NULL)
		strcpy(nameString, HASH_NULL_HMAC);	
	else
	strcpy(nameString, x->aalg->alg_name);

	if(strcmp(nameString, HASH_MD5_HMAC) == 0)
	{
		hashAlg = 0x0; //md5
		enHmac = 0x1; //hmac
		addedLen += 12; //ICV
	}
	else if(strcmp(nameString, HASH_SHA1_HMAC) == 0)
	{
		hashAlg = 0x1; //sha1
		enHmac = 0x1; //hmac
		addedLen += 12; //ICV
	}
	else if(strcmp(nameString, HASH_SHA256_HMAC) == 0)
	{
		hashAlg = 0x3; //sha256
		enHmac = 0x1; //hmac
		addedLen += 16; //ICV
	}
	else if(strcmp(nameString, HASH_NULL_HMAC) == 0)
	{
		hashAlg = 0xf; //null
		enHmac = 0x0;//0x1; //hmac
	}
	else
	{
		printk("\n !Unsupported! Hash Algorithm (%s) by EIP93! \n", nameString);
		errVal = -EPERM;
		goto free_addrsPreComputes;
	}

	cipherKey =	(unsigned int *)x->ealg->alg_key;
	currAdapterPtr->addedLen = addedLen;
	errVal = ipsec_cmdHandler_cmdDescp_set(currAdapterPtr, cmdHandlerDir, cipherAlg, hashAlg, crypto_aead_authsize(esp->aead)/sizeof(unsigned int), cipherMode, enHmac, aesKeyLen, cipherKey, keyLen, x->id.spi, padCrtlStat);
	if (errVal < 0)
	{
		goto free_addrsPreComputes;
	}

	return 1; //success

free_addrsPreComputes:
	ipsec_addrsDigestPreCompute_free(currAdapterPtr);

	return errVal;
}

static int 
ipsec_esp_preProcess(
	struct xfrm_state *x, 
	struct sk_buff *skb,
	unsigned int direction
)
{
	ipsecEip93Adapter_t **ipsecEip93AdapterList;
	unsigned int i, usedEntryNum = 0;
	ipsecEip93Adapter_t *currAdapterPtr;
	unsigned int spi = x->id.spi;
	int currAdapterIdx = -1;
	int err = 1;
	struct esp_data *esp = x->data;
	unsigned int *addrCurrAdapter;
	unsigned long flags;

	if (direction == HASH_DIGEST_OUT)
	{
		ipsecEip93AdapterList = &ipsecEip93AdapterListOut[0];
	}
	else
	{
		ipsecEip93AdapterList = &ipsecEip93AdapterListIn[0];
	}

	spin_lock(&ipsec_adapters_lock);
	//try to find the matched ipsecEip93Adapter for the ipsec flow
	for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
	{
		if ((currAdapterPtr = ipsecEip93AdapterList[i]) != NULL)
		{
			if (currAdapterPtr->spi == spi)
			{
				currAdapterIdx = i;
				break;
			}
			usedEntryNum++;
		}
		else
		{	//try to record the first unused entry in ipsecEip93AdapterList
			if (currAdapterIdx == -1)
			{
				currAdapterIdx = i;
			}
		}
	}
	
	if (usedEntryNum == IPESC_EIP93_ADAPTERS)
	{
		printk("\n\n !The ipsecEip93AdapterList table is full! \n\n");
		err = -EPERM;
		spin_unlock(&ipsec_adapters_lock);
		goto EXIT;
	}

	//no ipsecEip93Adapter matched, so create a new one for the ipsec flow. Only the first packet of a ipsec flow will encounter this.
	if (i == IPESC_EIP93_ADAPTERS)
	{
		if (x->aalg == NULL)
		{
			//printk("\n\n !please set a hash algorithm! \n\n");
			//err = -EPERM;
			//goto EXIT;
		}
		else if (x->ealg == NULL)
		{
			printk("\n\n !please set a cipher algorithm! \n\n");
			err = -EPERM;
			goto EXIT;
		}
	
		currAdapterPtr = (ipsecEip93Adapter_t *) kzalloc(sizeof(ipsecEip93Adapter_t), GFP_KERNEL);	
		if(currAdapterPtr == NULL)
		{
			printk("\n\n !!kmalloc for new ipsecEip93Adapter failed!! \n\n");
			err = -ENOMEM;
			goto EXIT;
		}
		
		spin_lock_init(&currAdapterPtr->lock);
		skb_queue_head_init(&currAdapterPtr->skbQueue);	
		spin_lock_irqsave(&currAdapterPtr->lock, flags);
		err = ipsec_hashDigest_preCompute(x, currAdapterPtr, direction);
		if (err < 0)
		{
			printk("\n\n !ipsec_hashDigest_preCompute for direction:%d failed! \n\n", direction);
			kfree(currAdapterPtr);
			spin_unlock_irqrestore(&currAdapterPtr->lock, flags);
			goto EXIT;
		}			
		err = ipsec_cmdHandler_prepare(x, currAdapterPtr, direction);
		if (err < 0)
		{
			printk("\n\n !ipsec_cmdHandler_prepare for direction:%d failed! \n\n", direction);
			kfree(currAdapterPtr);
			spin_unlock_irqrestore(&currAdapterPtr->lock, flags);
			goto EXIT;
		}		
		currAdapterPtr->spi = spi;
		ipsecEip93AdapterList[currAdapterIdx] = currAdapterPtr;
		
		if (direction == HASH_DIGEST_IN)
				currAdapterPtr->isEncryptOrDecrypt = CRYPTO_DECRYPTION;

		else
				currAdapterPtr->isEncryptOrDecrypt = CRYPTO_ENCRYPTION;	
		spin_unlock_irqrestore(&currAdapterPtr->lock, flags);
		
		
	}
	spin_unlock(&ipsec_adapters_lock);
	
	currAdapterPtr = ipsecEip93AdapterList[currAdapterIdx];


	if (direction == HASH_DIGEST_IN)
	{
		currAdapterPtr->x = x;
	}

#if !defined (FEATURE_AVOID_QUEUE_PACKET)
	//Hash Digests are ready
	spin_lock(&currAdapterPtr->lock);
	if (currAdapterPtr->isHashPreCompute == 2)
	{	 		
		ipsec_hashDigests_get(currAdapterPtr);
		currAdapterPtr->isHashPreCompute = 3; //pre-compute done
		ipsec_addrsDigestPreCompute_free(currAdapterPtr);	
	}
	spin_unlock(&currAdapterPtr->lock);
#endif
	//save needed info skb (cryptoDriver will save skb in EIP93's userID), so the needed info can be used by the tasklet which is raised by interrupt.
	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	*addrCurrAdapter = (unsigned int)currAdapterPtr;

EXIT:
	return err;

	
}

static int 
ipsec_esp_pktPut(
	ipsecEip93Adapter_t *currAdapterPtr,
	struct sk_buff *skb
)
{
	eip93DescpHandler_t *cmdHandler;
	struct sk_buff *pSkb;
	unsigned int isQueueFull = 0;
	unsigned int addedLen;
	struct sk_buff *skb2 = NULL;
	struct dst_entry *dst;
	unsigned int *addrCurrAdapter;
	unsigned long flags;
	

	spin_lock_bh(&cryptoLock);
	
	if (currAdapterPtr!=NULL)
	{
		cmdHandler = currAdapterPtr->cmdHandler;
		addedLen = currAdapterPtr->addedLen;
		goto DEQUEUE;
	}		

	dst = skb_dst(skb);
	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);
	cmdHandler = currAdapterPtr->cmdHandler;
	addedLen = currAdapterPtr->addedLen;

	//resemble paged packets if needed
	if (skb_is_nonlinear(skb)) 
	{
		ra_dbg("skb should linearize\n");
		mcrypto_proc.nolinear_count++;
		if (skb_linearize(skb) != 0)
		{
			printk("\n !resembling paged packets failed! \n");
			spin_unlock_bh(&cryptoLock);
			return -EPERM;
		}
		
		//skb_linearize() may return a new skb, so insert currAdapterPtr back to skb!
		addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
		*addrCurrAdapter = (unsigned int)currAdapterPtr;
	}

	//make sure that tailroom is enough for the added length due to encryption		
	if (skb_tailroom(skb) < addedLen)
	{
		skb2 = skb_copy_expand(skb, skb_headroom(skb), addedLen, GFP_ATOMIC);

		kfree_skb(skb); //free old skb

		if (skb2 == NULL)
		{
			printk("\n !skb_copy_expand failed! \n");
			spin_unlock_bh(&cryptoLock);
			return -EPERM;
		}
		
		skb = skb2; //the new skb
		skb_dst_set(skb, dst_clone(dst));
		//skb_dst_set(skb, dst);
		addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
		*addrCurrAdapter = (unsigned int)currAdapterPtr;
		
		mcrypto_proc.copy_expand_count++;
	}


	if (currAdapterPtr->skbQueue.qlen < SKB_QUEUE_MAX_SIZE)
	{
		int i;
		skb_queue_tail(&currAdapterPtr->skbQueue, skb);

		for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
		{
			if (currAdapterPtr == ipsecEip93AdapterListIn[i])
				mcrypto_proc.qlen[i] = currAdapterPtr->skbQueue.qlen;
		}
		for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
		{
			if (currAdapterPtr == ipsecEip93AdapterListOut[i])
				mcrypto_proc.qlen[i] = currAdapterPtr->skbQueue.qlen;
		}
			
	}
	else
	{
		isQueueFull = 1;
	}
DEQUEUE:	
	//ipsec_BH_handler_resultGet has no chance to set isHashPreCompute as 3, so currAdapterPtr->lock is not needed here!
	if (currAdapterPtr->isHashPreCompute == 3) //pre-compute done	
	{		
		//spin_lock(&cryptoLock);	
		while (ipsec_eip93CmdResCnt_check() && ((pSkb = skb_dequeue(&currAdapterPtr->skbQueue)) != NULL))
		{
			ipsec_packet_put(cmdHandler, pSkb); //mtk_packet_put
		}	
		//spin_unlock(&cryptoLock);
	
		if (isQueueFull && (currAdapterPtr->skbQueue.qlen < SKB_QUEUE_MAX_SIZE))
		{
			isQueueFull = 0;
			if (skb)
			skb_queue_tail(&currAdapterPtr->skbQueue, skb);
		}
	}

	if (isQueueFull == 0)
	{
		spin_unlock_bh(&cryptoLock);
		return HWCRYPTO_OK; //success
	}
	else
	{
		ra_dbg("-ENOMEM qlen=%d\n",currAdapterPtr->skbQueue.qlen);
		mcrypto_proc.oom_in_put++;
		if(skb2)
		{	
			kfree_skb(skb2);
			spin_unlock_bh(&cryptoLock);
			return HWCRYPTO_NOMEM;
		}
		else
		{
			spin_unlock_bh(&cryptoLock);
			return -ENOMEM; //drop the packet
	}
}
}

/*_______________________________________________________________________
**function name: ipsec_esp_output_finish
**
**description:
*   Deal with the rest of Linux Kernel's esp_output(). Then,
*	the encrypted packet can do the correct post-routing.
**parameters:
*   resHandler -- point to the result descriptor handler that stores
*		the needed info comming from EIP93's Result Descriptor Ring.
**global:
*   none
**return:
*   none
**call:
*   ip_output()
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
static void 
ipsec_esp_output_finish(
	eip93DescpHandler_t *resHandler
)
{
	struct sk_buff *skb = (struct sk_buff *) ipsec_eip93UserId_get(resHandler);
	struct iphdr *top_iph = ip_hdr(skb);
	unsigned int length;
	//struct dst_entry *dst = skb->dst;
	struct dst_entry *dst = skb_dst(skb);
	struct xfrm_state *x = dst->xfrm;
	
	struct net *net = xs_net(x);
	int err;
	struct ip_esp_hdr *esph = ip_esp_hdr(skb);


	length = ipsec_pktLength_get(resHandler);

	skb_put(skb, length - skb->len); //adjust skb->tail

	length += skb->data - skb_network_header(skb); //IP total length

	__skb_push(skb, -skb_network_offset(skb));
#ifdef RALINK_HWCRYPTO_NAT_T
	//if (x->encap)
	//	skb_push(skb, 8);
#endif			
	esph = ip_esp_hdr(skb);
	*skb_mac_header(skb) = IPPROTO_ESP;	      
#ifdef RALINK_HWCRYPTO_NAT_T
	if (x->encap) {
		struct xfrm_encap_tmpl *encap = x->encap;
		struct udphdr *uh;
		__be32 *udpdata32;
		__be16 sport, dport;
		int encap_type;

		sport = encap->encap_sport;
		dport = encap->encap_dport;
		encap_type = encap->encap_type;

		uh = (struct udphdr *)esph;
		uh->source = sport;
		uh->dest = dport;
		uh->len = htons(skb->len - skb_transport_offset(skb));
		uh->check = 0;
	
		switch (encap_type) {
		default:
		case UDP_ENCAP_ESPINUDP:
			esph = (struct ip_esp_hdr *)(uh + 1);
			break;
		case UDP_ENCAP_ESPINUDP_NON_IKE:
			udpdata32 = (__be32 *)(uh + 1);
			udpdata32[0] = udpdata32[1] = 0;
			esph = (struct ip_esp_hdr *)(udpdata32 + 2);
			break;
		}

		*skb_mac_header(skb) = IPPROTO_UDP;
		//__skb_push(skb, -skb_network_offset(skb));
	}
#endif

	top_iph->tot_len = htons(length);
	ip_send_check(top_iph);
#ifdef 	RALINK_ESP_OUTPUT_POLLING	
	goto out;
#endif	
	/* adjust for IPSec post-routing */
	dst = skb_dst_pop(skb);
	if (!dst) {
		XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTERROR);
		err = -EHOSTUNREACH;
		printk("(%d)ipsec_esp_output_finish EHOSTUNREACH\n",__LINE__);
		kfree_skb(skb);
		return;
	}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)	
	skb_dst_set(skb, dst_clone(dst));
#else
	skb_dst_set(skb, dst);
#endif	

	if (skb_dst(skb)->xfrm)
	{
		x = dst->xfrm;
		if (x->type->proto==IPPROTO_AH)
		{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
			extern int xfrm_skb_check_space(struct sk_buff *skb);
			err = xfrm_skb_check_space(skb);
#else
			extern int xfrm_state_check_space(struct xfrm_state *x, struct sk_buff *skb);
			err = xfrm_state_check_space(x, skb);
#endif			
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTERROR);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTERROR\n",__LINE__);
				kfree_skb(skb);
				return;	
			}
	
			err = x->outer_mode->output(x, skb);
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATEMODEERROR);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATEMODEERROR\n",__LINE__);
				kfree_skb(skb);
				return;	
			}
	
			spin_lock_bh(&x->lock);
			
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
			if (unlikely(x->km.state != XFRM_STATE_VALID)) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATEINVALID);
				spin_unlock_bh(&x->lock);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATEINVALID\n",__LINE__);
				kfree_skb(skb);
				return;	
			}
#endif
			err = xfrm_state_check_expire(x);
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATEEXPIRED);
				spin_unlock_bh(&x->lock);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATEEXPIRED\n",__LINE__);
				kfree_skb(skb);
				return;	
			}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
			err = x->repl->overflow(x, skb);
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATESEQERROR);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATESEQERROR\n",__LINE__);
				kfree_skb(skb);
				return;
			}
#else	
			if (x->type->flags & XFRM_TYPE_REPLAY_PROT) {
				XFRM_SKB_CB(skb)->seq.output = ++x->replay.oseq;
				if (unlikely(x->replay.oseq == 0)) {
					XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATESEQERROR);
					x->replay.oseq--;
					xfrm_audit_state_replay_overflow(x, skb);
					err = -EOVERFLOW;
					spin_unlock_bh(&x->lock);
					printk("(%d)ipsec_esp_output_finish -EOVERFLOW\n",__LINE__);
					kfree_skb(skb);
					return;
				}
				if (xfrm_aevent_is_on(net))
					xfrm_replay_notify(x, XFRM_REPLAY_UPDATE);
			}
#endif	
			x->curlft.bytes += skb->len;
			x->curlft.packets++;
			spin_unlock_bh(&x->lock);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)			
			skb_dst_force(skb);
#endif			
			err = x->type->output(x, skb);
			top_iph = ip_hdr(skb);
			ip_send_check(top_iph);
			dst = skb_dst_pop(skb);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)	
			skb_dst_set(skb, dst_clone(dst));
#else
			skb_dst_set(skb, dst);
#endif
		}	
	}

	nf_reset(skb);

	if (!skb_dst(skb)->xfrm)
	{
		mcrypto_proc.dbg_pt[8]++;
		ip_output(skb);
		return;
	}
		      
out:
	printk("(%d)%s:out\n",__LINE__,__func__);

	return;
}

static void 
ipsec_esp6_output_finish(
	eip93DescpHandler_t *resHandler
)
{
	struct sk_buff *skb = (struct sk_buff *) ipsec_eip93UserId_get(resHandler);
	struct ipv6hdr *top_iph = ipv6_hdr(skb);
	unsigned int length;
	struct dst_entry *dst = skb_dst(skb);
	struct xfrm_state *x = dst->xfrm;
	
	struct net *net = xs_net(x);
	int err;
	struct ip_esp_hdr *esph = ip_esp_hdr(skb);

	length = ipsec_pktLength_get(resHandler);

	skb_put(skb, length - skb->len); //adjust skb->tail

	__skb_push(skb, -skb_network_offset(skb));
			
	esph = ip_esp_hdr(skb);
	*skb_mac_header(skb) = IPPROTO_ESP;	      

	top_iph->payload_len = htons(length);

	/* adjust for IPSec post-routing */
	dst = skb_dst_pop(skb);
	
	if (!dst) {
		XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTERROR);
		err = -EHOSTUNREACH;
		printk("(%d)ipsec_esp_output_finish EHOSTUNREACH\n",__LINE__);
		kfree_skb(skb);
		return;
	}
	
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)	
	skb_dst_set(skb, dst_clone(dst));
#else
	skb_dst_set(skb, dst);
#endif

	if (skb_dst(skb)->xfrm)
	{
		x = dst->xfrm;
		if (x->type->proto==IPPROTO_AH)
		{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
			extern int xfrm_skb_check_space(struct sk_buff *skb);
			err = xfrm_skb_check_space(skb);
#else
			extern int xfrm_state_check_space(struct xfrm_state *x, struct sk_buff *skb);
			err = xfrm_state_check_space(x, skb);
#endif
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTERROR);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTERROR\n",__LINE__);
				kfree_skb(skb);
				return;	
			}
	
			err = x->outer_mode->output(x, skb);
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATEMODEERROR);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATEMODEERROR\n",__LINE__);
				kfree_skb(skb);
				return;	
			}
	
			spin_lock_bh(&x->lock);
			err = xfrm_state_check_expire(x);
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATEEXPIRED);
				spin_unlock_bh(&x->lock);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATEEXPIRED\n",__LINE__);
				kfree_skb(skb);
				return;	
			}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
			err = x->repl->overflow(x, skb);
			if (err) {
				XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATESEQERROR);
				printk("(%d)ipsec_esp_output_finish LINUX_MIB_XFRMOUTSTATESEQERROR\n",__LINE__);
				kfree_skb(skb);
				return;
			}
#else	
			if (x->type->flags & XFRM_TYPE_REPLAY_PROT) {
				XFRM_SKB_CB(skb)->seq.output = ++x->replay.oseq;
				if (unlikely(x->replay.oseq == 0)) {
					XFRM_INC_STATS(net, LINUX_MIB_XFRMOUTSTATESEQERROR);
					x->replay.oseq--;
					xfrm_audit_state_replay_overflow(x, skb);
					err = -EOVERFLOW;
					spin_unlock_bh(&x->lock);
					printk("(%d)ipsec_esp_output_finish -EOVERFLOW\n",__LINE__);
					kfree_skb(skb);
					return;
				}
				if (xfrm_aevent_is_on(net))
					xfrm_replay_notify(x, XFRM_REPLAY_UPDATE);
			}
#endif
	
			x->curlft.bytes += skb->len;
			x->curlft.packets++;
			spin_unlock_bh(&x->lock);
			err = x->type->output(x, skb);
			dst = skb_dst_pop(skb);
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)	
			skb_dst_set(skb, dst_clone(dst));
	#else
			skb_dst_set(skb, dst);
	#endif
		}
	}

	nf_reset(skb);

	if (!skb_dst(skb)->xfrm)
	{
		mcrypto_proc.dbg_pt[8]++;
		dst_output(skb);
		return;
	}
		      
out:
	printk("(%d)%s:out\n",__LINE__,__func__);
	return;
}

/*_______________________________________________________________________
**function name: ipsec_esp_input_finish
**
**description:
*   Deal with the rest of Linux Kernel's esp_input(). Then,
*	the decrypted packet can do the correct post-routing.
**parameters:
*   resHandler -- point to the result descriptor handler that stores
*		the needed info comming from EIP93's Result Descriptor Ring.
*   x -- point to the structure that stores IPSec SA information
**global:
*   none
**return:
*   none
**call:
*   netif_rx() for tunnel mode, or xfrm4_rcv_encap_finish() for transport
*		mode.
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
static void 
ipsec_esp_input_finish(
	eip93DescpHandler_t *resHandler, 
	struct xfrm_state *x
)
{
	struct sk_buff *skb = (struct sk_buff *) ipsec_eip93UserId_get(resHandler);
	struct iphdr *iph;
	unsigned int ihl, pktLen;
	struct esp_data *esp = x->data;
	int xfrm_nr = 0;
	int decaps = 0;
	__be32 spi, seq;
	int err;
	int net;
	int nexthdr = 0;
	struct xfrm_mode *inner_mode = x->inner_mode;
	int async = 0;
	struct ip_esp_hdr *esph = skb->data;
	ipsecEip93Adapter_t *currAdapterPtr;
	unsigned int *addrCurrAdapter;

	
	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);

	esph->seq_no = htonl(ipsec_espSeqNum_get(resHandler));
	esph->spi = currAdapterPtr->spi;

	skb->ip_summed = CHECKSUM_NONE;	
	iph = ip_hdr(skb);
	ihl = iph->ihl << 2; //iph->ihl * 4	
	iph->protocol = ipsec_espNextHeader_get(resHandler);
	nexthdr = iph->protocol;
		
	//adjest skb->tail & skb->len
	pktLen = ipsec_pktLength_get(resHandler);
	
	//*(skb->data-20+9) = 0x32;
#ifdef RALINK_HWCRYPTO_NAT_T	
	if (x->encap) {
		struct xfrm_encap_tmpl *encap = x->encap;
		struct udphdr *uh = (void *)(skb_network_header(skb) + ihl);

		/*
		 * 1) if the NAT-T peer's IP or port changed then
		 *    advertize the change to the keying daemon.
		 *    This is an inbound SA, so just compare
		 *    SRC ports.
		 */
		if (iph->saddr != x->props.saddr.a4 ||
		    uh->source != encap->encap_sport) {
			xfrm_address_t ipaddr;

			ipaddr.a4 = iph->saddr;
			km_new_mapping(x, &ipaddr, uh->source);

			/* XXX: perhaps add an extra
			 * policy check here, to see
			 * if we should allow or
			 * reject a packet from a
			 * different source
			 * address/port.
			 */
		}

		/*
		 * 2) ignore UDP/TCP checksums in case
		 *    of NAT-T in Transport Mode, or
		 *    perform other post-processing fixes
		 *    as per draft-ietf-ipsec-udp-encaps-06,
		 *    section 3.1.2
		 */
		if (x->props.mode == XFRM_MODE_TRANSPORT)
			skb->ip_summed = CHECKSUM_UNNECESSARY;
	}
#endif	
	skb->len = pktLen;
	skb_set_tail_pointer(skb, pktLen);
	__skb_pull(skb, crypto_aead_ivsize(esp->aead));

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)	
	skb_set_transport_header(skb, -ihl);
#else	
	if (x->props.mode == XFRM_MODE_TUNNEL)
		skb_reset_transport_header(skb);
	else
		skb_set_transport_header(skb, -ihl);
#endif		

	/* adjust for IPSec post-routing */
	spin_lock(&x->lock);
	if (nexthdr <= 0) {
		if (nexthdr == -EBADMSG) {
			xfrm_audit_state_icvfail(x, skb, x->type->proto);
			x->stats.integrity_failed++;
		}
		XFRM_INC_STATS(net, LINUX_MIB_XFRMINSTATEPROTOERROR);
		printk("(%d)ipsec_esp_input_finish LINUX_MIB_XFRMINSTATEPROTOERROR\n",__LINE__);
		spin_unlock(&x->lock);
		goto drop;
	}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)
	if (x->props.replay_window)
			xfrm_replay_advance(x, htonl(ipsec_espSeqNum_get(resHandler)));
#else	
	seq = htonl(ipsec_espSeqNum_get(resHandler));
	if (async && x->repl->recheck(x, skb, seq)) {
			XFRM_INC_STATS(net, LINUX_MIB_XFRMINSTATESEQERROR);
			spin_unlock(&x->lock);
			goto drop;
		}
		x->repl->advance(x, seq);
#endif

	x->curlft.bytes += skb->len;
	x->curlft.packets++;
	spin_unlock(&x->lock);

	XFRM_MODE_SKB_CB(skb)->protocol = nexthdr;

	inner_mode = x->inner_mode;

	if (x->sel.family == AF_UNSPEC) {
		inner_mode = xfrm_ip2inner_mode(x, XFRM_MODE_SKB_CB(skb)->protocol);
		if (inner_mode == NULL)
		{
			printk("(%d)ipsec_esp_input_finish inner_mode NULL\n",__LINE__);	
			goto drop;
		}	
	}

	if (inner_mode->input(x, skb)) {
		printk("(%d)ipsec_esp_input_finish LINUX_MIB_XFRMINSTATEMODEERROR\n",__LINE__);
		XFRM_INC_STATS(net, LINUX_MIB_XFRMINSTATEMODEERROR);
		goto drop;
	}

	if (x->outer_mode->flags & XFRM_MODE_FLAG_TUNNEL) {
		decaps = 1;
	}
	else
	{	
		/*
		 * We need the inner address.  However, we only get here for
		 * transport mode so the outer address is identical.
		 */
	
		err = xfrm_parse_spi(skb, nexthdr, &spi, &seq);
		if (err < 0) {
			printk("(%d)ipsec_esp_input_finish LINUX_MIB_XFRMINHDRERROR\n",__LINE__);
			XFRM_INC_STATS(net, LINUX_MIB_XFRMINHDRERROR);
			goto drop;
		}
	}

	nf_reset(skb);
	mcrypto_proc.dbg_pt[9]++;
	if (decaps) {
		skb_dst_drop(skb);
		netif_rx(skb);
		return ;
	} else {
		x->inner_mode->afinfo->transport_finish(skb, async);
		return;
	}	

drop:
	printk("(%d)%s:drop\n",__LINE__,__func__);
	kfree_skb(skb);
	return;
}

static void 
ipsec_esp6_input_finish(
	eip93DescpHandler_t *resHandler, 
	struct xfrm_state *x
)
{
	struct sk_buff *skb = (struct sk_buff *) ipsec_eip93UserId_get(resHandler);
	struct ipv6hdr *iph;
	unsigned int ihl, pktLen;
	struct esp_data *esp = x->data;
	int xfrm_nr = 0;
	int decaps = 0;
	__be32 spi, seq;
	int err;
	int net;
	int nexthdr = 0;
	struct xfrm_mode *inner_mode = x->inner_mode;
	int async = 0;
	struct ip_esp_hdr *esph = skb->data;
	ipsecEip93Adapter_t *currAdapterPtr;
	unsigned int *addrCurrAdapter;

	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);

	esph->seq_no = htonl(ipsec_espSeqNum_get(resHandler));
	esph->spi = currAdapterPtr->spi;

	skb->ip_summed = CHECKSUM_NONE;	
	iph = ipv6_hdr(skb);
	ihl = 40;	
	iph->nexthdr = ipsec_espNextHeader_get(resHandler);
	nexthdr = iph->nexthdr;
		
	//adjest skb->tail & skb->len
	pktLen = ipsec_pktLength_get(resHandler);

	//*(skb->data-20+9) = 0x32;
	skb->len = pktLen;
	skb_set_tail_pointer(skb, pktLen);
	__skb_pull(skb, crypto_aead_ivsize(esp->aead));
	
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)	
	skb_set_transport_header(skb, -ihl);
#else	
	if (x->props.mode == XFRM_MODE_TUNNEL)
		skb_reset_transport_header(skb);
	else
		skb_set_transport_header(skb, -ihl);
#endif
	
	/* adjust for IPSec post-routing */
	spin_lock(&x->lock);
	if (nexthdr <= 0) {
		if (nexthdr == -EBADMSG) {
			xfrm_audit_state_icvfail(x, skb, x->type->proto);
			x->stats.integrity_failed++;
		}
		XFRM_INC_STATS(net, LINUX_MIB_XFRMINSTATEPROTOERROR);
		printk("(%d)ipsec_esp_input_finish LINUX_MIB_XFRMINSTATEPROTOERROR\n",__LINE__);
		spin_unlock(&x->lock);
		goto drop;
	}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)
	if (x->props.replay_window)
			xfrm_replay_advance(x, htonl(ipsec_espSeqNum_get(resHandler)));
#else	
	seq = htonl(ipsec_espSeqNum_get(resHandler));
	if (async && x->repl->recheck(x, skb, seq)) {
			XFRM_INC_STATS(net, LINUX_MIB_XFRMINSTATESEQERROR);
			spin_unlock(&x->lock);
			goto drop;
	}
	x->repl->advance(x, seq);
#endif

	x->curlft.bytes += skb->len;
	x->curlft.packets++;
	spin_unlock(&x->lock);

	XFRM_MODE_SKB_CB(skb)->protocol = nexthdr;

	inner_mode = x->inner_mode;

	if (x->sel.family == AF_UNSPEC) {
		inner_mode = xfrm_ip2inner_mode(x, XFRM_MODE_SKB_CB(skb)->protocol);
		if (inner_mode == NULL)
		{
			printk("(%d)ipsec_esp_input_finish inner_mode NULL\n",__LINE__);	
			goto drop;
		}	
	}

	if (inner_mode->input(x, skb)) {
		printk("(%d)ipsec_esp_input_finish LINUX_MIB_XFRMINSTATEMODEERROR\n",__LINE__);
		XFRM_INC_STATS(net, LINUX_MIB_XFRMINSTATEMODEERROR);
		goto drop;
	}

	if (x->outer_mode->flags & XFRM_MODE_FLAG_TUNNEL) {
		decaps = 1;
	}
	else
	{	
		/*
		 * We need the inner address.  However, we only get here for
		 * transport mode so the outer address is identical.
		 */
	
		err = xfrm_parse_spi(skb, nexthdr, &spi, &seq);
		if (err < 0) {
			printk("(%d)ipsec_esp_input_finish LINUX_MIB_XFRMINHDRERROR\n",__LINE__);
			XFRM_INC_STATS(net, LINUX_MIB_XFRMINHDRERROR);
			goto drop;
		}
	}

	nf_reset(skb);
	mcrypto_proc.dbg_pt[9]++;
	if (decaps) {
		skb_dst_drop(skb);
		netif_rx(skb);
		return ;
	} else {
		x->inner_mode->afinfo->transport_finish(skb, async);
		return;
	}	

drop:
	printk("(%d)%s:drop\n",__LINE__,__func__);
	kfree_skb(skb);
	return;
}
/************************************************************************
*              P U B L I C     F U N C T I O N S
*************************************************************************
*/
void 
ipsec_eip93Adapter_free(
	unsigned int spi
)
{
	unsigned int i;
	ipsecEip93Adapter_t *currAdapterPtr;

	spin_lock(&ipsec_adapters_lock);
	for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
	{
		if ((currAdapterPtr = ipsecEip93AdapterListOut[i]) != NULL)
		{
			if (currAdapterPtr->spi == spi)
			{
				ipsec_cmdHandler_free(currAdapterPtr->cmdHandler);
				kfree(currAdapterPtr);
				ipsecEip93AdapterListOut[i] = NULL;
				spin_unlock(&ipsec_adapters_lock);
				return;
			}
		}
	}
	
	for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
	{
		if ((currAdapterPtr = ipsecEip93AdapterListIn[i]) != NULL)
		{
			if (currAdapterPtr->spi == spi)
			{
				ipsec_cmdHandler_free(currAdapterPtr->cmdHandler);
				kfree(currAdapterPtr);
				ipsecEip93AdapterListIn[i] = NULL;
				spin_unlock(&ipsec_adapters_lock);
				return;
			}
		}
	}
	spin_unlock(&ipsec_adapters_lock);
}

/*_______________________________________________________________________
**function name: ipsec_esp_output
**
**description:
*   Replace Linux Kernel's esp_output(), in order to use EIP93
*	to do encryption for a IPSec ESP flow.
**parameters:
*   x -- point to the structure that stores IPSec SA information
*	skb -- the packet that is going to be encrypted.
**global:
*   none
**return:
*   -EPERM, -ENOMEM -- failed: the pakcet will be dropped!
*	1 -- success: the packet's command decsriptor is put into
*		EIP93's Command Descriptor Ring.
**call:
*   none
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
int 
ipsec_esp_output(
	struct xfrm_state *x, 
	struct sk_buff *skb
)
{
	ipsecEip93Adapter_t *currAdapterPtr;
	int err;
	eip93DescpHandler_t *cmdHandler;
	struct iphdr *top_iph = ip_hdr(skb);
	unsigned int *addrCurrAdapter;
	struct sk_buff *trailer;
	u8 *tail;

	err = ipsec_esp_preProcess(x, skb, HASH_DIGEST_OUT);
	if (err < 0)
	{
		printk("\n\n ipsec_esp_preProcess for HASH_DIGEST_OUT failed! \n\n");
		return err;
	}

	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);
	cmdHandler = currAdapterPtr->cmdHandler;
		
#ifdef RALINK_HWCRYPTO_NAT_T
#else		
	/* this is non-NULL only with UDP Encapsulation for NAT-T */
	if (unlikely(x->encap)) 
	{		
		printk("\n\n NAT-T is not supported yet! \n\n");
		return -EPERM;
	}
#endif	
	/* in case user will change between tunnel and transport mode,
	 * we have to set "padValue" every time before every packet 
	 * goes into EIP93 for esp outbound! */
	ipsec_espNextHeader_set(cmdHandler, top_iph->protocol);
	//let skb->data point to the payload which is going to be encrypted
	if (x->encap==0)	
		__skb_pull(skb, skb_transport_offset(skb));

#if defined (FEATURE_AVOID_QUEUE_PACKET)
	err = ipsec_esp_pktPut(NULL, skb);
	return err;
#else
	return ipsec_esp_pktPut(NULL, skb);
#endif
}

int ipsec_esp6_output(
	struct xfrm_state *x, 
	struct sk_buff *skb
)
{
	ipsecEip93Adapter_t *currAdapterPtr;
	int err;
	eip93DescpHandler_t *cmdHandler;
	struct ipv6hdr *top_iph = ipv6_hdr(skb);
	unsigned int *addrCurrAdapter;
	struct sk_buff *trailer;
	u8 *tail;

	err = ipsec_esp_preProcess(x, skb, HASH_DIGEST_OUT);
	if (err < 0)
	{
		printk("\n\n ipsec_esp_preProcess for HASH_DIGEST_OUT failed! \n\n");
		return err;
	}

	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);
	cmdHandler = currAdapterPtr->cmdHandler;	
#ifdef RALINK_HWCRYPTO_NAT_T
#else		
	/* this is non-NULL only with UDP Encapsulation for NAT-T */
	if (unlikely(x->encap)) 
	{		
		printk("\n\n NAT-T is not supported yet! \n\n");
		return -EPERM;
	}
#endif	
	/* in case user will change between tunnel and transport mode,
	 * we have to set "padValue" every time before every packet 
	 * goes into EIP93 for esp outbound! */
	//top_iph->nexthdr = 50;
	ipsec_espNextHeader_set(cmdHandler, top_iph->nexthdr);
	//let skb->data point to the payload which is going to be encrypted
	if (x->encap==0)	
		__skb_pull(skb, skb_transport_offset(skb));

#if defined (FEATURE_AVOID_QUEUE_PACKET)
	err = ipsec_esp_pktPut(NULL, skb);
	return err;
#else
	return ipsec_esp_pktPut(NULL, skb);
#endif
}
/*_______________________________________________________________________
**function name: ipsec_esp_input
**
**description:
*   Replace Linux Kernel's esp_input(), in order to use EIP93
*	to do decryption for a IPSec ESP flow.
**parameters:
*   x -- point to the structure that stores IPSec SA information
*	skb -- the packet that is going to be decrypted.
**global:
*   none
**return:
*   -EPERM, -ENOMEM -- failed: the pakcet will be dropped!
*	1 -- success: the packet's command decsriptor is put into
*		EIP93's Command Descriptor Ring.
**call:
*   none
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
int 
ipsec_esp_input(
	struct xfrm_state *x, 
	struct sk_buff *skb
)
{
	ipsecEip93Adapter_t *currAdapterPtr;
	unsigned int *addrCurrAdapter;	
	int err;
	struct esp_data *esp = x->data;
	int blksize = ALIGN(crypto_aead_blocksize(esp->aead), 4);
	int alen = crypto_aead_authsize(esp->aead);
	int elen = skb->len - sizeof(struct ip_esp_hdr) - crypto_aead_ivsize(esp->aead) - alen;	

	err = ipsec_esp_preProcess(x, skb, HASH_DIGEST_IN);
	if (err < 0)
	{
		printk("\n\n ipsec_esp_preProcess for HASH_DIGEST_IN failed! \n\n");
		return err;
	}

	if (!pskb_may_pull(skb, sizeof(struct ip_esp_hdr)))
		goto out;
		
	if (elen <= 0 || (elen & (blksize-1)))
		goto out;

#ifdef RALINK_HWCRYPTO_NAT_T
#else
	if (x->encap) 
	{
		printk("\n !NAT-T is not supported! \n");
		goto out;
	}
#endif

#if defined (FEATURE_AVOID_QUEUE_PACKET)	
	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);
	err = ipsec_esp_pktPut(NULL, skb);
	return err;	
#else
	return ipsec_esp_pktPut(NULL, skb);
#endif
out:
	printk("\n Something's wrong! Go out! \n");
	return -EINVAL;
}

int 
ipsec_esp6_input(
	struct xfrm_state *x, 
	struct sk_buff *skb
)
{
	ipsecEip93Adapter_t *currAdapterPtr;
	unsigned int *addrCurrAdapter;	
	int err;
	struct esp_data *esp = x->data;
	int blksize = ALIGN(crypto_aead_blocksize(esp->aead), 4);
	int alen = crypto_aead_authsize(esp->aead);
	int elen = skb->len - sizeof(struct ip_esp_hdr) - crypto_aead_ivsize(esp->aead) - alen;	

	err = ipsec_esp_preProcess(x, skb, HASH_DIGEST_IN);
	if (err < 0)
	{
		printk("\n\n ipsec_esp_preProcess for HASH_DIGEST_IN failed! \n\n");
		return err;
	}

	if (!pskb_may_pull(skb, sizeof(struct ip_esp_hdr)))
	{	
		printk("[%s]pskb_may_pull failed\n",__func__);
		goto out;
	}
		
	if (elen <= 0 || (elen & (blksize-1)))
	{	
		printk("[%s]elen=%d blksize=%d\n",__func__,elen,blksize);
		goto out;
	}
#ifdef RALINK_HWCRYPTO_NAT_T
#else
	if (x->encap) 
	{
		printk("\n !NAT-T is not supported! \n");
		goto out;
	}
#endif

#if defined (FEATURE_AVOID_QUEUE_PACKET)	
	addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
	currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);
	err = ipsec_esp_pktPut(NULL, skb);
	return err;	
#else
	return ipsec_esp_pktPut(NULL, skb);
#endif
out:
	printk("\n[%s] Something's wrong! Go out! \n",__func__);
	return -EINVAL;
}
/************************************************************************
*              E X T E R N E L     F U N C T I O N S
*************************************************************************
*/
/*_______________________________________________________________________
**function name: ipsec_eip93_adapters_init
**
**description:
*   initialize ipsecEip93AdapterListOut[] and ipsecEip93AdapterListIn[]
*	durin EIP93's initialization.
**parameters:
*   none
**global:
*   none
**return:
*   none
**call:
*   none
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
void 
ipsec_eip93_adapters_init(
	void
)
{
	unsigned int i;
	
	for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
	{
		ipsecEip93AdapterListOut[i] = NULL;
		ipsecEip93AdapterListIn[i] = NULL;
	}
}

/*_______________________________________________________________________
**function name: ipsec_cryptoLock_init
**
**description:
*   initialize cryptoLock durin EIP93's initialization. cryptoLock is
*	used to make sure only one process can access EIP93 at a time.
**parameters:
*   none
**global:
*   none
**return:
*   none
**call:
*   none
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
void 
ipsec_cryptoLock_init(
	void
)
{
	spin_lock_init(&cryptoLock);
	spin_lock_init(&ipsec_adapters_lock);
}

EXPORT_SYMBOL(ipsec_eip93_adapters_init);
EXPORT_SYMBOL(ipsec_cryptoLock_init);

/*_______________________________________________________________________
**function name: ipsec_BH_handler_resultGet
**
**description:
*   This tasklet is raised by EIP93's interrupt after EIP93 finishs
*	a command descriptor and puts the result in Result Descriptor Ring.
*	This tasklet gets a result descriptor from EIP93 at a time and do
*	the corresponding atcion until all results from EIP93 are finished.
**parameters:
*   none
**global:
*   none
**return:
*   none
**call:
*   ipsec_esp_output_finish() when the result is for encryption.
*	ipsec_esp_input_finish() when the result is for decryption.
**revision:
*   1.Trey 20120209
**_______________________________________________________________________*/
void 
ipsec_BH_handler_resultGet(
	void
)
{
	int retVal;
	struct sk_buff *skb = NULL;
	ipsecEip93Adapter_t *currAdapterPtr;
	unsigned int *addrCurrAdapter;
	unsigned long flags;

	while (1)
	{
		memset(&resDescpHandler, 0, sizeof(eip93DescpHandler_t));
		retVal = ipsec_packet_get(&resDescpHandler);

		//got the correct result from eip93
		if (likely(retVal == 1))
		{
			//the result is for encrypted or encrypted packet
			if (ipsec_eip93HashFinal_get(&resDescpHandler) == 0x1)
			{				
				skb = (struct sk_buff *) ipsec_eip93UserId_get(&resDescpHandler);
				addrCurrAdapter = (unsigned int *) &(skb->cb[36]);
				currAdapterPtr = (ipsecEip93Adapter_t *)(*addrCurrAdapter);

				if (skb->protocol == htons(ETH_P_IPV6))
				{
					if (currAdapterPtr->isEncryptOrDecrypt == CRYPTO_ENCRYPTION)
					{
						ipsec_esp6_output_finish(&resDescpHandler);
					}
					else if (currAdapterPtr->isEncryptOrDecrypt == CRYPTO_DECRYPTION)
					{			
						ipsec_esp6_input_finish(&resDescpHandler, currAdapterPtr->x);
					}
					else
					{
						printk("\n\n !can't tell encrypt or decrypt! %08X\n\n",currAdapterPtr->isEncryptOrDecrypt);
						return;
					}
				}
				else
				{			
				if (currAdapterPtr->isEncryptOrDecrypt == CRYPTO_ENCRYPTION)
				{
					ipsec_esp_output_finish(&resDescpHandler);
				}
				else if (currAdapterPtr->isEncryptOrDecrypt == CRYPTO_DECRYPTION)
				{			
					ipsec_esp_input_finish(&resDescpHandler, currAdapterPtr->x);
				}
				else
				{
					printk("\n\n !can't tell encrypt or decrypt! %08X\n\n",currAdapterPtr->isEncryptOrDecrypt);
					return;
				}
				}
				//ipsec_esp_pktPut(currAdapterPtr, NULL);
			}
			//the result is for inner and outer hash digest pre-compute
			else
			{
				currAdapterPtr = (ipsecEip93Adapter_t *) ipsec_eip93UserId_get(&resDescpHandler);
				if (currAdapterPtr)
				printk("=== Build IPSec %s Connection===\n",\
							(currAdapterPtr->isEncryptOrDecrypt==CRYPTO_ENCRYPTION) ? "outbound" : " inbound");
				else
				{	
					printk("No connection entry in table\n");
					return;				
				}
				spin_lock(&currAdapterPtr->lock);
				//for the inner digests			
				if (currAdapterPtr->isHashPreCompute == 0)
				{
					//resDescpHandler only has physical addresses, so we have to get saState's virtual address from addrsPreCompute.
					ipsec_hashDigests_set(currAdapterPtr, 1);
					//inner digest done
					currAdapterPtr->isHashPreCompute = 1; 
				}
				//for the outer digests	
				else if (currAdapterPtr->isHashPreCompute == 1)
				{
					addrsDigestPreCompute_t* addrsPreCompute = currAdapterPtr->addrsPreCompute;
					ipsec_hashDigests_set(currAdapterPtr, 2);
					//outer digest done
					currAdapterPtr->isHashPreCompute = 2;				
#if defined (FEATURE_AVOID_QUEUE_PACKET)
					//Hash Digests are ready
					ipsec_hashDigests_get(currAdapterPtr);
					currAdapterPtr->isHashPreCompute = 3; //pre-compute done
					ipsec_esp_pktPut(currAdapterPtr, NULL);
					ipsec_addrsDigestPreCompute_free(currAdapterPtr);
#endif
				}
				else
				{
					printk("\n\n !can't tell inner or outer digests! \n\n");				
					spin_unlock(&currAdapterPtr->lock);
					return;
				}						
				spin_unlock(&currAdapterPtr->lock);
			}
		}
		//if packet is not done, don't wait! (for speeding up)
		else if (retVal == 0)
		{

			int i;
			
			for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
			{
				currAdapterPtr = ipsecEip93AdapterListIn[i];
				if (currAdapterPtr!=NULL)
					ipsec_esp_pktPut(currAdapterPtr, NULL);
			}
			for (i = 0; i < IPESC_EIP93_ADAPTERS; i++)
			{
				currAdapterPtr = ipsecEip93AdapterListOut[i];
				if (currAdapterPtr!=NULL)
					ipsec_esp_pktPut(currAdapterPtr, NULL);	
			}

			break;
		}
	} //end while (1)
	
	return;
}
EXPORT_SYMBOL(ipsec_BH_handler_resultGet);
