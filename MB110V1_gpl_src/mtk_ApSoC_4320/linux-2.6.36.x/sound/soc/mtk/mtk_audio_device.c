/*
 * mtk_gdma_i2s.c
 *
 *  Created on: 2013/8/20
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
#include <linux/i2c.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include "drivers/char/ralink_gdma.h"
#include "mtk_audio_driver.h"
#include "mtk_audio_device.h"
#if defined(CONFIG_SND_SOC_WM8750)
#include "../codecs/wm8750.h"
#elif defined(CONFIG_SND_SOC_WM8960)
#include "../codecs/wm8960.h"
#endif

#define I2C_AUDIO_DEV_ID	(0)
/****************************/
/*FUNCTION DECLRATION		*/
/****************************/
extern void i2c_WM8751_write(unsigned int address, unsigned int data);
//extern void snd_soc_free_pcms(struct snd_soc_device *socdev);
//extern void snd_soc_dapm_free(struct snd_soc_device *socdev);

static int mtk_codec_hw_params(struct snd_pcm_substream *substream,\
				struct snd_pcm_hw_params *params);
static int mtk_codec_init(struct snd_soc_codec *codec);

/****************************/
/*GLOBAL VARIABLE DEFINITION*/
/****************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
extern struct snd_soc_component_driver mtk_i2s_component;
extern struct snd_soc_dai_driver mtk_audio_drv;
extern struct snd_soc_platform_driver mtk_soc_platform;
#else
extern struct snd_soc_dai mtk_audio_drv_dai;
extern struct snd_soc_platform mtk_soc_platform;
#endif


static struct platform_device *mtk_audio_device;
static struct platform_device *mtk_i2c_device;

#if defined(CONFIG_SND_SOC_WM8750)
extern struct snd_soc_dai wm8750_dai;
extern struct snd_soc_codec_device soc_codec_dev_wm8750;
//static unsigned long i2s_codec_12p288Mhz[11]  = {0x0C,  0x00, 0x10, 0x14,  0x38, 0x38, 0x18,  0x20, 0x00,  0x00, 0x1C};
//static unsigned long i2s_codec_12Mhz[11]      = {0x0C,  0x32, 0x10, 0x14,  0x37, 0x38, 0x18,  0x22, 0x00,  0x3E, 0x1C};
//static unsigned long i2s_codec_24p576Mhz[11]  = {0x4C,  0x00, 0x50, 0x54,  0x00, 0x78, 0x58,  0x00, 0x40,  0x00, 0x5C};
#endif

#if defined(CONFIG_SND_SOC_WM8751)
//static unsigned long i2s_codec_12p288Mhz[11]  = {0x04,  0x00, 0x10, 0x14,  0x38, 0x38, 0x18,  0x20, 0x00,  0x00, 0x1C};
//static unsigned long i2s_codec_12Mhz[11]      = {0x04,  0x32, 0x10, 0x14,  0x37, 0x38, 0x18,  0x22, 0x00,  0x3E, 0x1C};
#endif

#if defined(CONFIG_SND_SOC_WM8960)
extern struct snd_soc_dai wm8960_dai;
extern struct snd_soc_codec_device soc_codec_dev_wm8960;
/*only support 12Mhz*/
//static unsigned long i2s_codec_12p288Mhz[11]  = {0x36,  0x24, 0x24, 0x1b,  0x12, 0x12, 0x09,  0x00, 0x00,  0x00, 0x00};
//static unsigned long i2s_codec_12Mhz[11]      = {0x36,  0x24, 0x24, 0x1b,  0x12, 0x12, 0x09,  0x00, 0x00,  0x00, 0x00};
#endif

/****************************/
/*STRUCTURE DEFINITION		*/
/****************************/
static struct i2c_board_info i2c_board_info[] = {
	{
#if defined(CONFIG_SND_SOC_WM8750)
		I2C_BOARD_INFO("wm8750", 0x18),
#elif defined(CONFIG_SND_SOC_WM8960)
		I2C_BOARD_INFO("wm8960", 0x18),
#endif
		//.platform_data = &uda1380_info,
	},
};

static struct resource i2cdev_resource[] =
{
    [0] =
    {
        .start = (RALINK_I2C_BASE),
        .end = (RALINK_I2C_BASE) + (0x40),
        .flags = IORESOURCE_MEM,
    },
};

static struct snd_soc_ops mtk_audio_ops = {
	.hw_params = mtk_codec_hw_params,
};

static struct snd_soc_dai_link mtk_audio_dai = {
	.name = "mtk_dai",
	.stream_name = "WMserious PCM",
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	.cpu_dai_name	= "soc-audio",
#if defined(CONFIG_SND_SOC_WM8750)
	.codec_dai_name	= "wm8750-hifi",
	.codec_name	= "wm8750-codec",
#elif defined(CONFIG_SND_SOC_WM8960)
	.codec_dai_name	= "wm8960-hifi",
	.codec_name	= "wm8960.0-0018",
#endif
	.platform_name	= "soc-audio",
#else
	.cpu_dai = &mtk_audio_drv_dai,
#if defined(CONFIG_SND_SOC_WM8750)
	.codec_dai = &wm8750_dai,
#elif defined(CONFIG_SND_SOC_WM8960)
	.codec_dai = &wm8960_dai,
#endif
#endif
	.init = mtk_codec_init,
	.ops = &mtk_audio_ops,
};

static struct snd_soc_card mtk_audio_card = {
	.name = "mtk_snd",
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	.owner = THIS_MODULE,
#else
	.platform = &mtk_soc_platform,
#endif
	.dai_link = &mtk_audio_dai,//I2S/Codec
	.num_links = 1,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
#else
/*device init: card,codec,codec data*/
static struct snd_soc_device mtk_audio_devdata = {
	.card = &mtk_audio_card,
#if defined(CONFIG_SND_SOC_WM8750)
	.codec_dev = &soc_codec_dev_wm8750,
#elif defined(CONFIG_SND_SOC_WM8960)
	.codec_dev = &soc_codec_dev_wm8960,
#endif
	.codec_data = NULL,
};
#endif
/****************************/
/*Function Body				*/
/****************************/


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
#else
/*It will be set into i2c custom read/write (soc-cache.c)*/
unsigned int mtk_i2c_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	unsigned int reg_cache_size = codec->reg_cache_size;
	//printk("%s:reg:%x val:%x (limit:%x)\n",__func__,reg,cache[reg],reg_cache_size);
	if (reg >= reg_cache_size)
		return -EIO;
	return cache[reg];
}

int mtk_i2c_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int val)
{
	u16 *cache = codec->reg_cache;
	unsigned int reg_cache_size = codec->reg_cache_size;
	//printk("%s:reg:%x val:%x (limit:%x)\n",__func__,reg,val,reg_cache_size);
	if (reg < (reg_cache_size-1))
		cache[reg] = val;
	i2c_WM8751_write(reg, val);
	return 0;
}
#endif

static int mtk_codec_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *p = substream->private_data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	struct snd_soc_dai *codec_dai = p->codec_dai;
#else
	struct snd_soc_dai *codec_dai = p->dai->codec_dai;
#endif
	struct snd_pcm_runtime *runtime = substream->runtime;
	i2s_config_type* rtd = runtime->private_data;
	unsigned long data,index = 0;
	unsigned long* pTable;
	int mclk,ret,targetClk = 0;

	/*For duplex mode, avoid setting twice.*/
	if((rtd->bRxDMAEnable == GDMA_I2S_EN) || (rtd->bTxDMAEnable == GDMA_I2S_EN))
		return 0;
	//printk("%s:%d -val:%x \n",__func__,__LINE__,params_rate(params));
#if defined(CONFIG_I2S_MCLK_12MHZ)
	mclk = 12000000;
#elif defined(CONFIG_I2S_MCLK_12P288MHZ)
	mclk = 12288000;
#else
	mclk = 12000000;
#endif
	snd_soc_dai_set_sysclk(codec_dai,0,mclk, SND_SOC_CLOCK_IN);



	switch(params_rate(params)){
	case 8000:
		index = 0;
		targetClk = 12288000;
		break;
	case 12000:
		index = 2;
		targetClk = 12288000;
		break;
	case 16000:
		index = 3;
		targetClk = 12288000;
		break;
	case 24000:
		index = 5;
		targetClk = 12288000;
		break;
	case 32000:
		index = 6;
		targetClk = 12288000;
		break;
	case 48000:
		index = 8;
		targetClk = 12288000;
		break;
	case 11025:
		index = 1;
		targetClk = 11289600;
		break;
	case 22050:
		index = 4;
		targetClk = 11289600;
		break;
	case 44100:
		index = 7;
		targetClk = 11289600;
		break;
	case 88200:
		index = 9;
		targetClk = 11289600;
		break;
	case 96000:
		index = 10;
		targetClk = 11289600;
		break;
	default:
		index = 7;
		targetClk = 12288000;
		MSG("audio sampling rate %u should be %d ~ %d Hz\n", (u32)params_rate(params), MIN_SRATE_HZ, MAX_SRATE_HZ);
		break;
	}
#if defined(CONFIG_SND_SOC_WM8960)
	snd_soc_dai_set_clkdiv(codec_dai, WM8960_OPCLKDIV, (0<<6));
	/*
	 * There is a fixed divide by 4 in the PLL and a selectable
	 * divide by N after the PLL which should be set to divide by 2 to meet this requirement.
	 * */
	ret = snd_soc_dai_set_pll(codec_dai, 0, 0,mclk, targetClk*2);
	/* From app notes: allow Vref to stabilize to reduce clicks */
	if(rtd->slave_en){
		ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_DCLKDIV, 0x1c4);
		ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_SYSCLKDIV, 0x5);
	}
#endif
	if(!rtd->slave_en)
		snd_soc_dai_set_fmt(codec_dai,SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_NB_NF);
	else{
		snd_soc_dai_set_fmt(codec_dai,SND_SOC_DAIFMT_CBM_CFM|SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_NB_NF);
	}
	mdelay(5);

#if defined(CONFIG_SND_SOC_WM8960)
#if defined(CONFIG_I2S_MCLK_12MHZ)
	pTable = i2s_codec_12Mhz;
	data = pTable[index];
#else
	pTable = i2s_codec_12p288Mhz;
	data = pTable[index];
#endif
	if(rtd->codec_pll_en)
		ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_DACDIV, (data<<3)|0x5);
	else
		ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_DACDIV, (data<<3)|0x4);
#endif
	//audiohw_postinit(1, 0, 1, 1);

	return 0;
}

static int mtk_codec_init(struct snd_soc_codec *codec)
{
#if 0
	struct snd_soc_dai *codec_dai = &codec->dai[0];
	int mclk = 0;
#if defined(CONFIG_I2S_MCLK_12MHZ)
	mclk = 12000000;
#elif defined(CONFIG_I2S_MCLK_12P288MHZ)
	mclk = 12288000;
#else
	mclk = 12000000;
#endif
	snd_soc_dai_set_sysclk(codec_dai,0,mclk, SND_SOC_CLOCK_IN);
#endif
	return 0;
}

static int __init mtk_soc_device_init(void)
{
	//struct snd_soc_device *socdev = &mtk_audio_devdata;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client = NULL;
	int ret = 0;

	mtk_audio_device = platform_device_alloc("soc-audio",-1);
	if (mtk_audio_device == NULL) {
		ret = -ENOMEM;
		goto err_device_alloc;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	platform_set_drvdata(mtk_audio_device, &mtk_audio_card);
#else
	platform_set_drvdata(mtk_audio_device, &mtk_audio_devdata);
	mtk_audio_devdata.dev = &mtk_audio_device->dev;
#endif

	/*Ralink I2S register process*/
	mtk_i2c_device = platform_device_alloc("Ralink-I2C",0);
	if (mtk_audio_device == NULL) {
		ret = -ENOMEM;
		goto err_device_alloc;
	}
	mtk_i2c_device->resource = i2cdev_resource;
	mtk_i2c_device->id = I2C_AUDIO_DEV_ID;
	mtk_i2c_device->num_resources = ARRAY_SIZE(i2cdev_resource);

	ret = platform_device_add(mtk_i2c_device);
	if (ret) {
		printk("mtk_i2c_device : platform_device_add failed (%d)\n",ret);
		goto err_device_add;
	}
	//mtk_audio_drv_dai.dev = socdev;
	adapter = i2c_get_adapter(I2C_AUDIO_DEV_ID);
	if (!adapter)
		return -ENODEV;
	client = i2c_new_device(adapter, i2c_board_info);
	if (!client)
		return -ENODEV;

	i2c_put_adapter(adapter);
	i2c_get_clientdata(client);
	/*Ralink I2S register process end*/
	ret = platform_device_add(mtk_audio_device);
	if (ret) {
		printk("mtk audio device : platform_device_add failed (%d)\n",ret);
		goto err_device_add;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
#else
	snd_soc_register_dai(&mtk_audio_drv_dai);
#endif

	return 0;

err_device_add:
	if (mtk_audio_device!= NULL) {
		platform_device_put(mtk_audio_device);
		mtk_audio_device = NULL;
	}
err_device_alloc:
	return ret;
}


static void __exit mtk_soc_device_exit(void)
{
	platform_device_unregister(mtk_audio_device);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	snd_soc_unregister_platform(&mtk_audio_device->dev);
#else
	snd_soc_unregister_platform(&mtk_soc_platform);
#endif

	mtk_audio_device = NULL;
}

module_init(mtk_soc_device_init);
module_exit(mtk_soc_device_exit);
//EXPORT_SYMBOL_GPL(mtk_soc_platform);
MODULE_LICENSE("GPL");
