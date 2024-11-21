/*
 * mtk_audio_pcm.c
 *
 *  Created on: 2013/9/6
 *      Author: MTK04880
 */

#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
#include <linux/sched.h>
#endif
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)
#include <asm/system.h> /* cli(), *_flags */
#endif
#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <linux/pci.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include "drivers/char/ralink_gdma.h"
#include "mtk_audio_driver.h"

#define GDMA_PAGE_SZ (I2S_PAGE_SIZE)
#define GDMA_PAGE_NUM MAX_I2S_PAGE

dma_addr_t i2s_txdma_addr, i2s_rxdma_addr;
dma_addr_t i2s_mmap_addr[GDMA_PAGE_NUM*2];

extern struct tasklet_struct i2s_tx_tasklet;
extern struct tasklet_struct i2s_rx_tasklet;


static int mtk_audio_pcm_open(struct snd_pcm_substream *substream);
static int mtk_pcm_new(struct snd_card *card,\
	struct snd_soc_dai *dai, struct snd_pcm *pcm);
static void mtk_pcm_free(struct snd_pcm *pcm);
static int mtk_audio_pcm_close(struct snd_pcm_substream *substream);
static snd_pcm_uframes_t mtk_audio_pcm_pointer(struct snd_pcm_substream *substream);
static int mtk_audio_pcm_trigger(struct snd_pcm_substream *substream, int cmd);
static int mtk_audio_pcm_prepare(struct snd_pcm_substream *substream);
static int mtk_audio_pcm_hw_params(struct snd_pcm_substream *substream,\
				 struct snd_pcm_hw_params *hw_params);
static int mtk_audio_pcm_copy(struct snd_pcm_substream *substream, int channel,\
		snd_pcm_uframes_t pos,void __user *buf, snd_pcm_uframes_t count);
static int mtk_audio_pcm_hw_free(struct snd_pcm_substream *substream);

static int mtk_pcm_free_dma_buffer(struct snd_pcm_substream *substream,int stream);
static int mtk_pcm_allocate_dma_buffer(struct snd_pcm_substream *substream,int stream);

static const struct snd_pcm_hardware mtk_audio_hwparam = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |SNDRV_PCM_INFO_PAUSE,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min	= GDMA_PAGE_SZ,
	.period_bytes_max	= GDMA_PAGE_SZ,
	.periods_min		= 2,
	.periods_max		= 1024,
	.buffer_bytes_max	= GDMA_PAGE_SZ*GDMA_PAGE_NUM,
};

static struct snd_pcm_ops mtk_pcm_ops = {

	.open = 	mtk_audio_pcm_open,
	.ioctl = 	snd_pcm_lib_ioctl,
	.hw_params = mtk_audio_pcm_hw_params,
	.hw_free = 	mtk_audio_pcm_hw_free,
	.trigger =	mtk_audio_pcm_trigger,
	.prepare = 	mtk_audio_pcm_prepare,
	.pointer = 	mtk_audio_pcm_pointer,
	.close = 	mtk_audio_pcm_close,
	.copy = mtk_audio_pcm_copy,
};
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
struct snd_soc_platform_driver mtk_soc_platform = {
	.ops		= &mtk_pcm_ops,
	.pcm_new	= mtk_pcm_new,
	.pcm_free	= mtk_pcm_free,
};
#else
struct snd_soc_platform mtk_soc_platform = {
	.name		= "mtk-dma",
	.pcm_ops	= &mtk_pcm_ops,
	.pcm_new	= mtk_pcm_new,
	.pcm_free	= mtk_pcm_free,
};
#endif

static int mtk_audio_pcm_close(struct snd_pcm_substream *substream){
	return 0;
}

static snd_pcm_uframes_t mtk_audio_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	i2s_config_type* rtd = runtime->private_data;
	unsigned int offset = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		offset = bytes_to_frames(runtime, GDMA_PAGE_SZ*rtd->tx_r_idx);
	}
	else{
		offset = bytes_to_frames(runtime, GDMA_PAGE_SZ*rtd->rx_w_idx);
	}
	return offset;
}

#if 0
static int gdma_ctrl_start(struct snd_pcm_substream *substream){

	struct snd_pcm_runtime *runtime= substream->runtime;
	i2s_config_type* rtd = runtime->private_data;

	//printk("%s:%d \n",__func__,__LINE__);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
#if defined(I2S_FIFO_MODE)
#else
	GdmaI2sTx((u32) rtd->pPage0TxBuf8ptr, I2S_FIFO_WREG, 0, GDMA_PAGE_SZ, i2s_dma_tx_handler, i2s_dma_unmask_handler);
	GdmaI2sTx((u32) rtd->pPage1TxBuf8ptr, I2S_FIFO_WREG, 1, GDMA_PAGE_SZ, i2s_dma_tx_handler, i2s_dma_unmask_handler);
#endif

#if defined(I2S_FIFO_MODE)
#else
	GdmaUnMaskChannel(GDMA_I2S_TX0);
#endif
		rtd->bTxDMAEnable = 1;
	}
	else{
#if defined(I2S_FIFO_MODE)
#else
		GdmaI2sRx(I2S_RX_FIFO_RREG, (u32)rtd->pPage0RxBuf8ptr, 0, GDMA_PAGE_SZ, i2s_dma_rx_handler, i2s_dma_unmask_handler);
		GdmaI2sRx(I2S_RX_FIFO_RREG, (u32)rtd->pPage1RxBuf8ptr, 1, GDMA_PAGE_SZ, i2s_dma_rx_handler, i2s_dma_unmask_handler);
#endif

#if defined(I2S_FIFO_MODE)
#else
		GdmaUnMaskChannel(GDMA_I2S_RX0);
#endif
		rtd->bRxDMAEnable = 1;
		return 0;
	}
	return 0;
}

static int gdma_ctrl_stop(struct snd_pcm_substream *substream){

	struct snd_pcm_runtime *runtime= substream->runtime;
	i2s_config_type* rtd = runtime->private_data;

	//printk("%s:%d \n",__func__,__LINE__);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		rtd->bTxDMAEnable = 0;
	}
	else{
		rtd->bRxDMAEnable = 0;
	}
	return 0;
}
#endif

static int mtk_audio_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	i2s_config_type* rtd = (i2s_config_type*)substream->runtime->private_data;
	MSG("trigger cmd:%s\n",(cmd==SNDRV_PCM_TRIGGER_START)?"START":\
			(cmd==SNDRV_PCM_TRIGGER_RESUME)?"RESUME":\
					(cmd==SNDRV_PCM_TRIGGER_PAUSE_RELEASE)?"PAUSE_RELEASE":\
							(cmd==SNDRV_PCM_TRIGGER_STOP)?"STOP":\
									(cmd==SNDRV_PCM_TRIGGER_SUSPEND)?"SUSPEND":\
											(cmd==SNDRV_PCM_TRIGGER_PAUSE_PUSH)?"PAUSE_PUSH":"default");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_STOP:
		break;

	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#if 0
		ret = gdma_ctrl_start(substream);
#else
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){

			rtd->tx_pause_en = 0;
			//gdma_En_Switch(rtd,STREAM_PLAYBACK,GDMA_I2S_EN);
			//gdma_unmask_handler(GDMA_I2S_TX0);
		}
		else{
			rtd->rx_pause_en = 0;
			//gdma_En_Switch(rtd,STREAM_CAPTURE,GDMA_I2S_EN);
			//gdma_unmask_handler(GDMA_I2S_RX0);
		}
#endif
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#if 0
		ret =gdma_ctrl_stop(substream);
#else
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
			//gdma_En_Switch(rtd,STREAM_PLAYBACK,GDMA_I2S_DIS);
			rtd->tx_pause_en = 1;
		}
		else{
			rtd->rx_pause_en = 1;
			//gdma_En_Switch(rtd,STREAM_CAPTURE,GDMA_I2S_DIS);
		}
#endif
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mtk_audio_pcm_copy(struct snd_pcm_substream *substream, int channel,\
		snd_pcm_uframes_t pos,void __user *buf, snd_pcm_uframes_t count){
	struct snd_pcm_runtime *runtime= substream->runtime;
	i2s_config_type* rtd = runtime->private_data;
	char *hwbuf = NULL;
#if 0
	do{
		hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
			//printk("%s:%d dma:%x hwoff:%d f2b:%d copy to user's frame:%d \n",__func__,__LINE__,(unsigned int)runtime->dma_area,pos,frames_to_bytes(runtime, pos),frames_to_bytes(runtime, count));
			if(((rtd->tx_w_idx+4)%MAX_I2S_PAGE)!=rtd->tx_r_idx){
				if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime,count)))
					return -EFAULT;
				rtd->tx_w_idx = (rtd->tx_w_idx+1)%MAX_I2S_PAGE;
				break;
			}
			else{
					interruptible_sleep_on(&(rtd->i2s_tx_qh));
			}
		}
		else{
			if(rtd->rx_r_idx!=rtd->rx_w_idx)
			{
				if (copy_to_user(buf, hwbuf, frames_to_bytes(runtime, count)))
					return -EFAULT;
				rtd->rx_r_idx = (rtd->rx_r_idx+1)%MAX_I2S_PAGE;
				break;
			}
			else{
					interruptible_sleep_on(&(rtd->i2s_rx_qh));
			}
		}
	}while(1);
#else

	hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);
	//MSG("%s bur:%x\n",__func__,hwbuf);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		i2s_audio_exchange(rtd,STREAM_PLAYBACK,(unsigned long)buf);
	}
	else{
		i2s_audio_exchange(rtd,STREAM_CAPTURE,(unsigned long)buf);
	}
#endif

	return 0;
}

static int mtk_audio_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime= substream->runtime;
	i2s_config_type *rtd = (i2s_config_type*)runtime->private_data;
	//runtime->stop_threshold = runtime->buffer_size = GDMA_PAGE_NUM*GDMA_PAGE_SZ;
	runtime->boundary = GDMA_PAGE_NUM*GDMA_PAGE_SZ;
#if 0
	gdma_ctrl_start(substream);
#else
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if(rtd->bTxDMAEnable != GDMA_I2S_EN){
			tasklet_init(&i2s_tx_tasklet, i2s_tx_task, (u32)rtd);
			gdma_En_Switch(rtd,STREAM_PLAYBACK,GDMA_I2S_EN);
			gdma_unmask_handler(GDMA_I2S_TX0);
		}
	}
	else{
		if(rtd->bRxDMAEnable != GDMA_I2S_EN){
			tasklet_init(&i2s_rx_tasklet, i2s_rx_task, (u32)rtd);
			gdma_En_Switch(rtd,STREAM_CAPTURE,GDMA_I2S_EN);
			gdma_unmask_handler(GDMA_I2S_RX0);
		}
	}

#endif

	return 0;
}


static int mtk_audio_pcm_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	i2s_config_type *rtd = (i2s_config_type*)runtime->private_data;
	int ret,i;
	ret = i = 0;
	//printk("%s %d \n",__func__,__LINE__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
#if 0
		/* allocate tx buffer */
		rtd->pPage0TxBuf8ptr = (u8*)pci_alloc_consistent(NULL, GDMA_PAGE_SZ*2 , &i2s_txdma_addr);
		if( rtd->pPage0TxBuf8ptr==NULL){
			MSG("Allocate Tx Page Buffer Failed\n");
			return -1;
		}
		rtd->pPage1TxBuf8ptr =  rtd->pPage0TxBuf8ptr + GDMA_PAGE_SZ;
		for(i = 0;i< GDMA_PAGE_NUM;i++)
			rtd->pMMAPTxBufPtr[i] =(char *)(buf->area +i*GDMA_PAGE_SZ);
#else
		i2s_page_prepare(rtd,STREAM_PLAYBACK);
#endif
	}
	else{
#if 0
		/* allocate rx buffer */
		rtd->pPage0RxBuf8ptr = (u8*)pci_alloc_consistent(NULL, GDMA_PAGE_SZ*2 , &i2s_rxdma_addr);
		if(rtd->pPage0RxBuf8ptr==NULL)
		{
			MSG("Allocate Rx Page Buffer Failed\n");
			return -1;
		}
		rtd->pPage1RxBuf8ptr = rtd->pPage0RxBuf8ptr + GDMA_PAGE_SZ;
		for(i = 0;i< GDMA_PAGE_NUM;i++)
			rtd->pMMAPRxBufPtr[i] =(char *)(buf->area +i*GDMA_PAGE_SZ);
#else
		i2s_page_prepare(rtd,STREAM_CAPTURE);
#endif
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	return ret;
}

static int mtk_audio_pcm_hw_free(struct snd_pcm_substream *substream)
{
	i2s_config_type* rtd = (i2s_config_type*)substream->runtime->private_data;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if(rtd->bTxDMAEnable==1){
#if 0
			rtd->bTxDMAEnable = 0;
			if (rtd->nTxDMAStopped<4)
				interruptible_sleep_on(&(rtd->i2s_tx_qh));
			printk("%s:%d \n",__func__,__LINE__);
			pci_free_consistent(NULL, I2S_PAGE_SIZE*2,rtd->pPage0TxBuf8ptr, i2s_txdma_addr);
			rtd->pPage0TxBuf8ptr = NULL;
#else
			tasklet_kill(&i2s_tx_tasklet);
			i2s_dma_tx_end_handle(rtd);
			gdma_En_Switch(rtd,STREAM_PLAYBACK,GDMA_I2S_DIS);
			/* Stop dma trnsfer data */
			i2s_dma_mask_handler(GDMA_I2S_TX0);
			i2s_dma_mask_handler(GDMA_I2S_TX1);
			i2s_page_release(rtd,STREAM_PLAYBACK);
#endif
		}
	}
	else{
		if(rtd->bRxDMAEnable==1){
#if 0
			rtd->bRxDMAEnable = 0;
			if(rtd->nRxDMAStopped<2)
				interruptible_sleep_on(&(rtd->i2s_rx_qh));
			pci_free_consistent(NULL, I2S_PAGE_SIZE*2,rtd->pPage0RxBuf8ptr, i2s_txdma_addr);
			rtd->pPage0RxBuf8ptr = NULL;
#else
			tasklet_kill(&i2s_rx_tasklet);
			gdma_En_Switch(rtd,STREAM_CAPTURE,GDMA_I2S_DIS);
			/* Stop dma transfer */
			i2s_dma_mask_handler(GDMA_I2S_RX0);
			i2s_dma_mask_handler(GDMA_I2S_RX1);
			i2s_page_release(rtd,STREAM_CAPTURE);
#endif
		}
	}
	mtk_pcm_free_dma_buffer(substream,substream->stream);
	return 0;
}

static int mtk_pcm_free_dma_buffer(struct snd_pcm_substream *substream,
	int stream)
{

	//struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	i2s_config_type* rtd = (i2s_config_type*)substream->runtime->private_data;

	if (!buf->area)
		return 0;
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		i2s_memPool_free(rtd,STREAM_PLAYBACK);
	else
		i2s_memPool_free(rtd,STREAM_CAPTURE);
	buf->area = NULL;
	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int mtk_pcm_allocate_dma_buffer(struct snd_pcm_substream *substream,
	int stream)
{

	//struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	i2s_config_type* rtd = (i2s_config_type*)substream->runtime->private_data;

	if(!buf->area){
		buf->dev.type = SNDRV_DMA_TYPE_UNKNOWN;
		buf->dev.dev = NULL;
		buf->private_data = NULL;
		if(stream == SNDRV_PCM_STREAM_PLAYBACK)
			buf->area = i2s_memPool_Alloc(rtd,STREAM_PLAYBACK);
		else
			buf->area = i2s_memPool_Alloc(rtd,STREAM_CAPTURE);

		printk("%s:%d -%s\n",__func__,__LINE__,(stream == SNDRV_PCM_STREAM_PLAYBACK)?"PB":"CP");
		if (!buf->area)
			return -ENOMEM;
		buf->bytes = (GDMA_PAGE_SZ*GDMA_PAGE_NUM);
	}
	return 0;
}

static int mtk_audio_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime= substream->runtime;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	int stream = substream->stream;
	int ret = 0;

	snd_soc_set_runtime_hwparams(substream, &mtk_audio_hwparam);
	/* ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,
						SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto out;
#if 1
	if(stream == SNDRV_PCM_STREAM_PLAYBACK){
		ret = mtk_pcm_allocate_dma_buffer(substream,
				SNDRV_PCM_STREAM_PLAYBACK);
	}
	else{
		ret = mtk_pcm_allocate_dma_buffer(substream,
				SNDRV_PCM_STREAM_CAPTURE);
	}
	if (ret)
		goto out;
#endif
	if(buf)
		memset(buf->area,0,sizeof(I2S_PAGE_SIZE*MAX_I2S_PAGE));

 out:
	return ret;
	return 0;
}



static int mtk_pcm_new(struct snd_card *card,
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;
	printk("%s:%d \n",__func__,__LINE__);

	return 0;
}

static void mtk_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	i2s_config_type* rtd;
	int stream;
	printk("%s:%d \n",__func__,__LINE__);
	return 0;
}


static int __init mtk_audio_pcm_init(void)
{

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
	return 0;
#else
	return snd_soc_register_platform(&mtk_soc_platform);
#endif

}

static void __exit mtk_audio_pcm_exit(void)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
	return;
#else
	snd_soc_unregister_platform(&mtk_soc_platform);
#endif
}

module_init(mtk_audio_pcm_init);
module_exit(mtk_audio_pcm_exit);

MODULE_AUTHOR("Atsushi Nemoto <anemo@mba.ocn.ne.jp>");
MODULE_DESCRIPTION("TXx9 ACLC Audio DMA driver");
MODULE_LICENSE("GPL");

