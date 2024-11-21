#ifndef _MCRYPTO_AES_API_H_
#define _MCRYPTO_AES_API_H_

#include <linux/crypto.h>

#define WORDSWAP(a)     	((((a)>>24)&0xff) | (((a)>>8)&0xff00) | (((a)<<8)&0xff0000) | (((a)<<24)&0xff000000))

/* driver logic flags */
#define AES_IV_LENGTH  16
#define AES_KEY_LENGTH 16
#define AES_MIN_BLOCK_SIZE 16

struct mcrypto_ctx {
	/* common */
	int totallen;
	u8 key[32+4];
	u32 keylen;
	u32 mode;
	
	/* cipher */
	u8 iv[AES_IV_LENGTH];
	union {
		struct crypto_blkcipher *blk;
		struct crypto_cipher *cip;
	} fallback;
	void* pData;
};


		
extern int hmac_setkey_base(struct mcrypto_ctx *ctx,
		const u8 *key, unsigned int keylen);

extern int mcrypto_setkey_blk(struct crypto_tfm *tfm, const u8 *key,
		unsigned int len);
extern int fallback_blk_dec(struct blkcipher_desc *desc,
		struct scatterlist *dst, struct scatterlist *src,
		unsigned int nbytes);
extern int fallback_blk_enc(struct blkcipher_desc *desc,
		struct scatterlist *dst, struct scatterlist *src,
		unsigned int nbytes);				
extern int fallback_init_blk(struct crypto_tfm *tfm);
extern void fallback_exit_blk(struct crypto_tfm *tfm);



extern struct crypto_alg mcrypto_cbc_alg;
extern struct crypto_alg mcrypto_aes_alg;

#endif /* !_MCRYPTO_AES_API_H_ */
