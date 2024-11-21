/*
 * mtk_audio_device.h
 *
 *  Created on: 2013/10/23
 *      Author: MTK04880
 */

#ifndef MTK_AUDIO_DEVICE_H_
#define MTK_AUDIO_DEVICE_H_
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#if defined(CONFIG_I2S_WM8750)
#undef CONFIG_I2S_WM8750
#elif defined(CONFIG_I2S_WM8751)
#undef CONFIG_I2S_WM8751
#elif defined(CONFIG_I2S_WM8960)
#undef CONFIG_I2S_WM8960
#else
#error "IIS with Unknown Codec?"
#endif

#ifdef CONFIG_I2S_MMAP
#undef CONFIG_I2S_MMAP
#endif

#if 0
void audiohw_preinit(void);
void audiohw_postinit(int bSlave, int Ain, int Aout);
void audiohw_close(void);
void audiohw_set_frequency(int fsel);
void audiohw_set_MCLK(unsigned int bUsb);
void audiohw_mute(bool mute);
#else
#define audiohw_preinit()
#define audiohw_postinit(a,b,c)
#define audiohw_close()
#define audiohw_set_frequency(a)
#define audiohw_set_MCLK(a);
#define audiohw_mute(a)
#define audiohw_set_frequency(a)
#define audiohw_set_lineout_vol(a,b,c);
#define audiohw_set_linein_vol(a,b)
#if 1
#if defined(CONFIG_SND_SOC_WM8750)
static unsigned long i2s_codec_12p288Mhz[11]  = {0x0C,  0x00, 0x10, 0x14,  0x38, 0x38, 0x18,  0x20, 0x00,  0x00, 0x1C};
static unsigned long i2s_codec_12Mhz[11]      = {0x0C,  0x32, 0x10, 0x14,  0x37, 0x38, 0x18,  0x22, 0x00,  0x3E, 0x1C};
static unsigned long i2s_codec_24p576Mhz[11]  = {0x4C,  0x00, 0x50, 0x54,  0x00, 0x78, 0x58,  0x00, 0x40,  0x00, 0x5C};
#endif

#if defined(CONFIG_SND_SOC_WM8751)
static unsigned long i2s_codec_12p288Mhz[11]  = {0x04,  0x00, 0x10, 0x14,  0x38, 0x38, 0x18,  0x20, 0x00,  0x00, 0x1C};
static unsigned long i2s_codec_12Mhz[11]      = {0x04,  0x32, 0x10, 0x14,  0x37, 0x38, 0x18,  0x22, 0x00,  0x3E, 0x1C};
#endif

#if defined(CONFIG_SND_SOC_WM8960)
static unsigned long i2s_codec_12p288Mhz[11]  = {0x36,  0x24, 0x24, 0x1b,  0x12, 0x12, 0x09,  0x00, 0x00,  0x00, 0x00};
static unsigned long i2s_codec_12Mhz[11]      = {0x36,  0x24, 0x24, 0x1b,  0x12, 0x12, 0x09,  0x00, 0x00,  0x00, 0x00};
#endif
#endif


#endif


#endif /* MTK_AUDIO_DEVICE_H_ */
