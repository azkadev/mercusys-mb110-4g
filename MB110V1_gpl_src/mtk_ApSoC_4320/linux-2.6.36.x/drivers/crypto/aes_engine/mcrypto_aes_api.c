#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/crypto.h>
#include <linux/spinlock.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <linux/spinlock_types.h>
#include <linux/io.h>
#include <linux/delay.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
#include <linux/sched.h>
#endif

#include "mcrypto_aes_api.h"
#include "aes_engine.h"


//#define LINUX_CRYPTO_API	1

int mcrypto_setkey_blk(struct crypto_tfm *tfm, const u8 *key,
		unsigned int len)
{
	struct mcrypto_ctx *ctx = crypto_tfm_ctx(tfm);
	unsigned int ret;

	ctx->keylen = len;
	if (!ctx->pData)
		ctx->pData = (void*)tfm->__crt_alg;
	
	memcpy(ctx->key, key, len);

	{
		int i;
		DBGPRINT(DBG_LOW, "key[] = [");
		for(i=0;i<len;i++)
			DBGPRINT(DBG_LOW, "%02X ",ctx->key[i]);
		DBGPRINT(DBG_LOW, "]\n");
	}

	ctx->fallback.blk->base.crt_flags &= ~CRYPTO_TFM_REQ_MASK;
	ctx->fallback.blk->base.crt_flags |= (tfm->crt_flags & CRYPTO_TFM_REQ_MASK);

	ret = crypto_blkcipher_setkey(ctx->fallback.blk, key, len);
	if (ret) {
		tfm->crt_flags &= ~CRYPTO_TFM_RES_MASK;
		tfm->crt_flags |= (ctx->fallback.blk->base.crt_flags & CRYPTO_TFM_RES_MASK);
	}
	return ret;
}

int fallback_blk_dec(struct blkcipher_desc *desc,
		struct scatterlist *dst, struct scatterlist *src,
		unsigned int nbytes)
{
	unsigned int ret;
	struct crypto_blkcipher *tfm;
	struct mcrypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);

	tfm = desc->tfm;
	desc->tfm = ctx->fallback.blk;

	ret = crypto_blkcipher_decrypt_iv(desc, dst, src, nbytes);

	desc->tfm = tfm;
	return ret;
}

int fallback_blk_enc(struct blkcipher_desc *desc,
		struct scatterlist *dst, struct scatterlist *src,
		unsigned int nbytes)
{
	unsigned int ret;
	struct crypto_blkcipher *tfm;
	struct mcrypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);

	tfm = desc->tfm;
	desc->tfm = ctx->fallback.blk;

	ret = crypto_blkcipher_encrypt_iv(desc, dst, src, nbytes);

	desc->tfm = tfm;

	return ret;
}

static int
mcrypto_cbc_decrypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes)
{
	struct mcrypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	int err;
	
#ifdef LINUX_CRYPTO_API
	return fallback_blk_dec(desc, dst, src, nbytes);
#endif
	//if (nbytes < 256)
	//	return fallback_blk_dec(desc, dst, src, nbytes);
		
	DBGPRINT(DBG_LOW, "(%d)%s nbytes=%d\n",__LINE__,__func__,nbytes);
	
	memcpy(ctx->iv, desc->info, AES_IV_LENGTH);
	if (ctx->keylen == 32)
		ctx->mode = AES_256;
	else if (ctx->keylen == 24)
		ctx->mode = AES_192;
	else
		ctx->mode = AES_128;
	ctx->mode |= (CBC_MODE);
	err = AesProcessScatterGather(src, dst, nbytes, ctx->key, ctx->iv, ctx->mode);

	return err;
}


static int
mcrypto_cbc_encrypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes)
{
	struct mcrypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	int err;
		
#ifdef LINUX_CRYPTO_API
	return fallback_blk_enc(desc, dst, src, nbytes);
#endif
	
	DBGPRINT(DBG_LOW, "(%d)%s keylen=%d nbytes=%d\n",__LINE__,__func__,ctx->keylen, nbytes);

	//if (nbytes < 256)
	//	return fallback_blk_enc(desc, dst, src, nbytes);

	memcpy(ctx->iv, desc->info, AES_IV_LENGTH);
	if (ctx->keylen == 32)
		ctx->mode = AES_256;
	else if (ctx->keylen == 24)
		ctx->mode = AES_192;
	else
		ctx->mode = AES_128;
	ctx->mode |= (CBC_MODE | ENCRYPTION);
	err = AesProcessScatterGather(src, dst, nbytes, ctx->key, ctx->iv, ctx->mode);

	return err;
}

static int
mcrypto_aes_decrypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes)
{
	struct mcrypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	int err;
	
#ifdef LINUX_CRYPTO_API
	return fallback_blk_dec(desc, dst, src, nbytes);
#endif
	//if (nbytes < 256)
	//	return fallback_blk_dec(desc, dst, src, nbytes);
		
	DBGPRINT(DBG_LOW, "(%d)%s keylen=%d nbytes=%d\n",__LINE__,__func__,ctx->keylen, nbytes);

	if (ctx->keylen == 32)
		ctx->mode = AES_256;
	else if (ctx->keylen == 24)
		ctx->mode = AES_192;
	else
		ctx->mode = AES_128;

	err = AesProcessScatterGather(src, dst, nbytes, ctx->key, ctx->iv, ctx->mode);
	
	return err;
}

static int
mcrypto_aes_encrypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes)
{
	struct mcrypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	int err;
		
#ifdef LINUX_CRYPTO_API
	return fallback_blk_enc(desc, dst, src, nbytes);
#endif
	
	DBGPRINT(DBG_LOW, "(%d)%s keylen=%d nbytes=%d\n",__LINE__,__func__,ctx->keylen, nbytes);

	//if (nbytes < 256)
	//	return fallback_blk_enc(desc, dst, src, nbytes);

	if (ctx->keylen == 32)
		ctx->mode = AES_256;
	else if (ctx->keylen == 24)
		ctx->mode = AES_192;
	else
		ctx->mode = AES_128;
	ctx->mode |= ENCRYPTION;
	err = AesProcessScatterGather(src, dst, nbytes, ctx->key, ctx->iv, ctx->mode);

	return err;
}

int fallback_init_blk(struct crypto_tfm *tfm)
{
	const char *name = tfm->__crt_alg->cra_name;
	struct mcrypto_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->fallback.blk = crypto_alloc_blkcipher(name, 0,
			CRYPTO_ALG_ASYNC | CRYPTO_ALG_NEED_FALLBACK);

	ctx->pData = (void*)tfm->__crt_alg;
	if (IS_ERR(ctx->fallback.blk)) {
		printk(KERN_ERR "Error allocating blk fallback algo %s\n", name);
		return PTR_ERR(ctx->fallback.blk);
	}

	return 0;
}

void fallback_exit_blk(struct crypto_tfm *tfm)
{
	struct mcrypto_ctx *ctx = crypto_tfm_ctx(tfm);

	crypto_free_blkcipher(ctx->fallback.blk);
	ctx->fallback.blk = NULL;
}



struct crypto_alg mcrypto_cbc_alg = {
	.cra_name		=	"cbc(aes)",
	.cra_driver_name	=	"cbc-aes-mcrypto",
	.cra_priority		=	400,
	.cra_flags			=	CRYPTO_ALG_TYPE_BLKCIPHER |
							CRYPTO_ALG_NEED_FALLBACK,
	.cra_init			=	fallback_init_blk,
	.cra_exit			=	fallback_exit_blk,
	.cra_blocksize		=	AES_MIN_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct mcrypto_ctx),
	.cra_alignmask		=	15,
	.cra_type			=	&crypto_blkcipher_type,
	.cra_module			=	THIS_MODULE,
	.cra_list			=	LIST_HEAD_INIT(mcrypto_cbc_alg.cra_list),
	.cra_u				=	{
		.blkcipher	=	{
			.min_keysize	=	AES_MIN_KEY_SIZE,
			.max_keysize	=	AES_MAX_KEY_SIZE,
			.setkey			=	mcrypto_setkey_blk,
			.encrypt		=	mcrypto_cbc_encrypt,
			.decrypt		=	mcrypto_cbc_decrypt,
			.ivsize			=	AES_IV_LENGTH,
		}
	}
};

struct crypto_alg mcrypto_aes_alg = {
	.cra_name		=	"aes",
	.cra_driver_name	=	"aes-mcrypto",
	.cra_priority		=	400,
	.cra_flags			=	CRYPTO_ALG_TYPE_BLKCIPHER |
							CRYPTO_ALG_NEED_FALLBACK,
	.cra_init			=	fallback_init_blk,
	.cra_exit			=	fallback_exit_blk,
	.cra_blocksize		=	AES_MIN_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct mcrypto_ctx),
	.cra_alignmask		=	15,
	.cra_type			=	&crypto_blkcipher_type,
	.cra_module			=	THIS_MODULE,
	.cra_list			=	LIST_HEAD_INIT(mcrypto_aes_alg.cra_list),
	.cra_u				=	{
		.blkcipher	=	{
			.min_keysize	=	AES_MIN_KEY_SIZE,
			.max_keysize	=	AES_MAX_KEY_SIZE,
			.setkey			=	mcrypto_setkey_blk,
			.encrypt		=	mcrypto_aes_encrypt,
			.decrypt		=	mcrypto_aes_decrypt,
			.ivsize			=	0,
		}
	}
};
