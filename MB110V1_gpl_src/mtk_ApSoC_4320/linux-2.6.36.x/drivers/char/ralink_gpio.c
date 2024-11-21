/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright, Ralink Technology, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 ***************************************************************************
 *
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/sched.h>
#ifdef CONFIG_RALINK_GPIO_LED
#include <linux/timer.h>
#endif
#include <asm/uaccess.h>
#include "ralink_gpio.h"

#include <asm/rt2880/surfboardint.h>

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
static  devfs_handle_t devfs_handle;
#endif
#include <linux/proc_fs.h>

#include <linux/rtnetlink.h>
#include <net/rtnetlink.h>

//#include <net/netlink.h>
/*added by xieping for mr3020v3*/
#ifdef CONFIG_TP_MODEL_MR3020V3
#define GPIO_LED_POWER  			(37)
#define GPIO_LED_INTERNET_GREEN 	(43)
#define GPIO_LED_WPS  				(2)
#define GPIO_LED_LAN  				(3)
#define GPIO_LED_WLAN_2G4  			(44)

#define GPIO_BTN_WPS_RST  			(38)
#define GPIO_BTN_MODE_C1  			(41)
#define GPIO_BTN_MODE_C2  			(42)

#define MODE_AP						(0x3)
#define MODE_WISP					(0x2)
#define MODE_3G						(0x1)

static char * sys_mode_str[3] = {"3G","WISP","AP"};
static int hwsys_mode = MODE_3G;
#define PROC_FILE_HWSYS_MODE				"tplink/hwsys_mode"
#define PROC_FILE_APCLI_STATUS				"tplink/apcli_status"

u32 apcli_status = 0;
EXPORT_SYMBOL(apcli_status);

extern void machine_restart(char *command);

#define INCLUDE_LAN_BIT_LED			(0)
#define INCLUDE_INTERNET_COLOR_LED	(0)

#define INCLUDE_SYS_MODE_PROC		(1)
#define INTERNET_MODE_USB3G			(1)
#define INTERNET_MODE_ETH			(2)
#define INTERNET_MODE_WISP			(3)


#define USB_MODEM_UNLINK			(0)
#define USB_MODEM_LINK				(1)

#elif defined(CONFIG_TP_MODEL_MR100V1) || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
/* 
 * MB110V1 green used for sys and wifi , and sys first
 * when it is 1, sys using, wifi set ignore
 */
#if defined(CONFIG_TP_MODEL_MB110V1)
#define STATE_SYS_OFF 0
#define STATE_SYS_BOOT_UPGRADE 1
#define STATE_SYS_ON_WIFI_OFF 2
#define STATE_SYS_ON_WIFI_ON 3
#define STATE_SYS_ON_WIFI_WPS 4
u32 sys_wlan_state = 2;	/* 0=sysoff, 1=booting_upgrading, 2=syson_wifioff, 3=syson_wifion, 4=wpsing */
u32 out_atn_main_enable = 0;
u32 out_atn_secd_enable = 0;
#define GPIO_OUT_ATN_MAIN  			(0)
#define GPIO_OUT_ATN_SECD	  		(5)
#endif

/* MR100V1 MR100V2 BEGIN */
#define GPIO_LED_POWER  			(39)
#define GPIO_LED_INTERNET  			(42)	/* means WAN/LAN2 for MB110V1 */

#define GPIO_LED_WPS				(41)	/* means SYS_GREEN for MB110V1 */
#define GPIO_LED_LAN  				(40)
#define GPIO_LED_WLAN_2G4  			(41)

#define GPIO_LED_SIGNAL_S1  		(43)
#define GPIO_LED_SIGNAL_S2  		(44)
#define GPIO_LED_SIGNAL_S3  		(46)

#define GPIO_BTN_WPS_RST  			(38)

#define GPIO_USB_POWER  			(36)

#if defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
#define GPIO_USB_BOOT  				(3)
#define GPIO_USB_RESET  			(4)
#define GPIO_USB_DC	  				(37)
#endif // CONFIG_TP_MODEL_MR100V2

#define INCLUDE_LAN_BIT_LED			(0)
#define INCLUDE_INTERNET_COLOR_LED	(0)
#define INCLUDE_SINGLE_LED			(0)

#define INCLUDE_SYS_MODE_PROC		(1)
#define INTERNET_MODE_LTE			(1)
#define INTERNET_MODE_ETH			(2)
/* MR100V1 MR100V2 END */

/* added by ZC for LTE GATEWAY COMBINED IMAGE*/
#elif defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
extern int pcie0_disable;
extern int pcie1_disable;

static int GPIO_LED_POWER;
static int GPIO_LED_INTERNET;
static int GPIO_LED_WPS;
static int GPIO_LED_WLAN_2G4;
static int GPIO_LED_WLAN_5G;
static int GPIO_LED_LAN;
static int GPIO_LED_SIGNAL_S1;
static int GPIO_LED_SIGNAL_S2;
static int GPIO_LED_SIGNAL_S3;

static int GPIO_BTN_WPS_RST;
static int GPIO_BTN_WIFI;

#define GPIO_MODEL_VER 			(11)

extern int flash_version;

#define INCLUDE_LAN_BIT_LED			(0)
#define INCLUDE_INTERNET_COLOR_LED	(0)
#define INCLUDE_SINGLE_LED			(0)

#define INCLUDE_SYS_MODE_PROC		(1)
#define INTERNET_MODE_LTE			(1)
#define INTERNET_MODE_ETH			(2)
/* end added */

/*add by wzy for mr3420v5*/
#elif defined (CONFIG_TP_MODEL_MR3420V5)
#define GPIO_LED_POWER  			(2)
#define GPIO_LED_USB				(3)
#define GPIO_LED_INTERNET_ORANGE 	(4)
#define GPIO_LED_INTERNET_GREEN 	(5)
#define GPIO_LED_WPS  				(37)
#define GPIO_LED_LAN  				(41)
#define GPIO_LED_WLAN_2G4  			(44)

#define GPIO_BTN_WPS_RST  			(38)
#define GPIO_BTN_WIFI  			    (46)

#define INCLUDE_LAN_BIT_LED			(0)
#define INCLUDE_INTERNET_COLOR_LED	(1)

#define INCLUDE_SYS_MODE_PROC		(1)
#define INTERNET_MODE_USB3G			(1)
#define INTERNET_MODE_ETH			(2)

#define USB_MODEM_UNLINK			(0)
#define USB_MODEM_LINK				(1)

#else
#define GPIO_LED_POWER  			(11)
#define GPIO_LED_USB				(3)
#define GPIO_LED_INTERNET_ORANGE 	(4)
#define GPIO_LED_INTERNET_GREEN 	(5)
#define GPIO_LED_WPS  				(37)
#define GPIO_LED_LAN  				(41)
#define GPIO_LED_WLAN_2G4  			(44)
#define GPIO_LED_WLAN_5G  			(43)

#define GPIO_BTN_WPS_RST  			(38)
#define GPIO_BTN_WIFI  			    (46)

#define INCLUDE_LAN_BIT_LED			(1)
#define INCLUDE_INTERNET_COLOR_LED	(1)
#endif
/*end add*/

unsigned char TotalPinAttackCount = 0;
unsigned char sharedWscLock = 0;
EXPORT_SYMBOL(TotalPinAttackCount);
EXPORT_SYMBOL(sharedWscLock);


#define TP_WSC_LED_START 			0
#define TP_WSC_LED_END				1
#define TP_WSC_LED_PBC_OVERLAPPED	2
#define TP_WSC_LED_ERROR			3
#define TP_WSC_LED_SUCCESS			4
#define TP_WSC_LED_NOP				5

#define GPIO_MODE_FACTORY			(1)
#define GPIO_MODE_NORMAL			(0)


unsigned int TP_WSCLEDStatus_24G = TP_WSC_LED_END;
unsigned int TP_WSCLEDStatus_5G = TP_WSC_LED_END;
EXPORT_SYMBOL(TP_WSCLEDStatus_24G);
EXPORT_SYMBOL(TP_WSCLEDStatus_5G);


#ifdef GPIO_USB_POWER
#define PROC_FILE_USB_POWER				"tplink/usb_power"
#endif
#ifdef GPIO_USB_BOOT
#define PROC_FILE_USB_BOOT				"tplink/usb_boot"
#endif
#ifdef GPIO_USB_DC
#define PROC_FILE_USB_DC				"tplink/usb_dc"
#endif
#ifdef GPIO_USB_RESET
#define PROC_FILE_USB_RESET				"tplink/usb_reset"
#endif
#if defined(CONFIG_TP_MODEL_MB110V1)
#define PROC_FILE_OUT_ATN				"tplink/out_atn"
#endif
#define PROC_FILE_USB_LED				"tplink/led_usb"
#define PROC_FILE_INTERNET_MODE			"tplink/internet_mode"
#define PROC_FILE_MODEM_LINK			"tplink/usb_modem_link"
#define PROC_FILE_INTERNET_LED			"tplink/led_internet"
#define PROC_FILE_WLAN24G_LED			"tplink/led_wlan_24G"
#define PROC_FILE_WLAN5G_LED			"tplink/led_wlan_5G"
#define PROC_FILE_POWER_LED				"tplink/led_power"
#define PROC_FILE_WLAN_LED_STATUS		"tplink/led_wlan_status"
/* added by zc in 2017/11/21 */
#define PROC_FILE_GPIO_MODE			"tplink/gpio_mode"
#define PROC_FILE_GPIO_STATUS			"tplink/gpio_status"
/* end added */
#define PROC_FILE_ENABLE_LED			"tplink/led_enable"
#define PROC_FILE_CONTROL_LED			"tplink/led_control"
#define PROC_FILE_SIGNAL_STRENGTH_LED	"tplink/led_signal_strength"
#define PROC_FILE_LTE_LED				"tplink/led_lte"

#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1) || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
int lte_resetFlag = 0;
int lte_rebootFlag = 0;
int lte_resetCount = 0;
int lte_rebootCount = 0;
int lte_reset_wait = 0;
#endif

#define NAME			"ralink_gpio"
#define RALINK_GPIO_DEVNAME	"gpio"
int ralink_gpio_major = 252;
int ralink_gpio_irqnum = 0;
u32 ralink_gpio_intp = 0;
u32 ralink_gpio_edge = 0;
#if defined (RALINK_GPIO_HAS_2722)
u32 ralink_gpio2722_intp = 0;
u32 ralink_gpio2722_edge = 0;
#elif defined (RALINK_GPIO_HAS_4524)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio4540_intp = 0;
u32 ralink_gpio4540_edge = 0;
#elif defined (RALINK_GPIO_HAS_5124)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio5140_intp = 0;
u32 ralink_gpio5140_edge = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio7140_intp = 0;
u32 ralink_gpio7140_edge = 0;
#if defined (RALINK_GPIO_HAS_7224)
u32 ralink_gpio72_intp = 0;
u32 ralink_gpio72_edge = 0;
#else
u32 ralink_gpio9572_intp = 0;
u32 ralink_gpio9572_edge = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
u32 ralink_gpio6332_intp = 0;
u32 ralink_gpio6332_edge = 0;
u32 ralink_gpio9564_intp = 0;
u32 ralink_gpio9564_edge = 0;
#endif
ralink_gpio_reg_info ralink_gpio_info[RALINK_GPIO_NUMBER];
extern unsigned long volatile jiffies;

#ifdef CONFIG_RALINK_GPIO_LED
#define RALINK_LED_DEBUG 0

#define RALINK_GPIO_CYCLE_UNIT (50) /* 50ms, cycle of gpio polling process */
#define RALINK_GPIO_LED_FREQ (12) /* 12 = RALINK_GPIO_CYCLE_UNIT / (1000 / HZ) */
#define LED_CYCLE_TRIGGER (1)	/* 1 = RALINK_GPIO_CYCLE_UNIT / RALINK_GPIO_CYCLE_UNIT */
#define BTN_CYCLE_TRIGGER	(4) /* 4 = 200 / RALINK_GPIO_CYCLE_UNIT */
#define CONN_CYCLE_TRIGGER (64) /* 64 = 3200 / RALINK_GPIO_CYCLE_UNIT */
#define RESET_TRIGGER (25) /* 25 = (5000 / RALINK_GPIO_CYCLE_UNIT) / BTN_CYCLE_TRIGGER */
#define WLAN_TRIGGER (1) /* 1 = (200 / RALINK_GPIO_CYCLE_UNIT) / BTN_CYCLE_TRIGGER */
#define WLAN_LOCK_TIMES (200) /* 200=40s, 10 = (5000 / RALINK_GPIO_CYCLE_UNIT) / BTN_CYCLE_TRIGGER */
#define MODEL_JUDGE_TRIGGER (10)

struct timer_list ralink_gpio_led_timer;
ralink_gpio_led_info ralink_gpio_led_data[RALINK_GPIO_NUMBER];

u32 ra_gpio_led_set = 0;
u32 ra_gpio_led_clr = 0;
#if defined (RALINK_GPIO_HAS_2722)
u32 ra_gpio2722_led_set = 0;
u32 ra_gpio2722_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_4524)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio4540_led_set = 0;
u32 ra_gpio4540_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_5124)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio5140_led_set = 0;
u32 ra_gpio5140_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio7140_led_set = 0;
u32 ra_gpio7140_led_clr = 0;
#if defined (RALINK_GPIO_HAS_7224)
u32 ra_gpio72_led_set = 0;
u32 ra_gpio72_led_clr = 0;
#else
u32 ra_gpio9572_led_set = 0;
u32 ra_gpio9572_led_clr = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
u32 ra_gpio6332_led_set = 0;
u32 ra_gpio6332_led_clr = 0;
u32 ra_gpio9564_led_set = 0;
u32 ra_gpio9564_led_clr = 0;
#endif
struct ralink_gpio_led_status_t {
	int ticks;
	unsigned int ons;
	unsigned int offs;
	unsigned int resting;
	unsigned int times;
} ralink_gpio_led_stat[RALINK_GPIO_NUMBER];
#endif
void ralink_gpio_notify_user(int usr);
static struct work_struct gpio_event_hold;
static struct work_struct gpio_event_click;

/*added by xieping for factory reset*/
static struct work_struct reset_event_work;

//static struct proc_dir_entry *simple_config_entry = NULL;


MODULE_DESCRIPTION("Ralink SoC GPIO Driver");
MODULE_AUTHOR("Winfred Lu <winfred_lu@ralinktech.com.tw>");
MODULE_LICENSE("GPL");
ralink_gpio_reg_info info;

void send_wlanSwitch_to_user(char *buf, int len);
void sendResetButtonPressed(void);

static int wlan_24G_status = 0;
static int wlan_5G_status = 0;
static int wlan_led_status = 1; //0=start setting, 1=end setting
static int wan_status = 0;
#ifdef CONFIG_TP_MODEL_MR3020V3
int sys_status = 1;
#else
int sys_status = 0;
#endif

int lte_recovering = 0;
EXPORT_SYMBOL(sys_status);
EXPORT_SYMBOL(lte_recovering);

int sd_kicking = 0;
EXPORT_SYMBOL(sd_kicking);


#if INCLUDE_SYS_MODE_PROC
#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1)|| defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
static int internet_mode = INTERNET_MODE_LTE;
#else
static int internet_mode = INTERNET_MODE_USB3G;
static int usb_modem_link = USB_MODEM_UNLINK;
#endif
#endif
/* added by zc in 2017/11/21 */
static int gpio_mode = GPIO_MODE_NORMAL;
static int gpio_status = 0;

enum GPIO_TYPE
{
	GPIO_RESET = 0,
	GPIO_WPS = 1,
	GPIO_LED = 2,
	GPIO_WIFI = 3,
	GPIO_WIFI_5G = 4,
	GPIO_MODE_SWITCH = 5,
	GPIO_RE_ONEKEY = 6,
	GPIO_TYPE_MAX
};

#define SET_GPIO_STATUS(x,n) (x=(x|((1U)<<n)))
#define CLEAR_GPIO_STATUS(x,n) (x=(x&~((1U)<<n)))
#define GET_GPIO_STATUS(x,n) ((1U)&(x>>n))
/* end added */

void gpio_click_notify(struct work_struct *work)
{
    //printk("<hua-dbg> %s, 1\n", __FUNCTION__);
    ralink_gpio_notify_user(1);
}


void gpio_hold_notify(struct work_struct *work)
{
    //printk("<hua-dbg> %s, 2\n", __FUNCTION__);
    ralink_gpio_notify_user(2);
}


int ralink_gpio_led_set(ralink_gpio_led_info led)
{
#ifdef CONFIG_RALINK_GPIO_LED
	unsigned long tmp;
	if (0 <= led.gpio && led.gpio < RALINK_GPIO_NUMBER) {
		if (led.on > RALINK_GPIO_LED_INFINITY)
			led.on = RALINK_GPIO_LED_INFINITY;
		if (led.off > RALINK_GPIO_LED_INFINITY)
			led.off = RALINK_GPIO_LED_INFINITY;
		if (led.blinks > RALINK_GPIO_LED_INFINITY)
			led.blinks = RALINK_GPIO_LED_INFINITY;
		if (led.rests > RALINK_GPIO_LED_INFINITY)
			led.rests = RALINK_GPIO_LED_INFINITY;
		if (led.times > RALINK_GPIO_LED_INFINITY)
			led.times = RALINK_GPIO_LED_INFINITY;
		if (led.on == 0 && led.off == 0 && led.blinks == 0 &&
				led.rests == 0) {
			ralink_gpio_led_data[led.gpio].gpio = -1; //stop it
			return 0;
		}
		//register led data
		ralink_gpio_led_data[led.gpio].gpio = led.gpio;
		ralink_gpio_led_data[led.gpio].on = (led.on == 0)? 1 : led.on;
		ralink_gpio_led_data[led.gpio].off = (led.off == 0)? 1 : led.off;
		ralink_gpio_led_data[led.gpio].blinks = (led.blinks == 0)? 1 : led.blinks;
		ralink_gpio_led_data[led.gpio].rests = (led.rests == 0)? 1 : led.rests;
		ralink_gpio_led_data[led.gpio].times = (led.times == 0)? 1 : led.times;

		//clear previous led status
		ralink_gpio_led_stat[led.gpio].ticks = -1;
		ralink_gpio_led_stat[led.gpio].ons = 0;
		ralink_gpio_led_stat[led.gpio].offs = 0;
		ralink_gpio_led_stat[led.gpio].resting = 0;
		ralink_gpio_led_stat[led.gpio].times = 0;

		printk("led=%d, on=%d, off=%d, blinks,=%d, reset=%d, time=%d\n",
				ralink_gpio_led_data[led.gpio].gpio,
				ralink_gpio_led_data[led.gpio].on,
				ralink_gpio_led_data[led.gpio].off,
				ralink_gpio_led_data[led.gpio].blinks,
				ralink_gpio_led_data[led.gpio].rests,
				ralink_gpio_led_data[led.gpio].times);
		//set gpio direction to 'out'
#if defined (RALINK_GPIO_HAS_2722)
		if (led.gpio <= 21) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
			tmp |= RALINK_GPIO((led.gpio-22));
			*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_9532)
		if (led.gpio <= 31) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		} else if (led.gpio <= 63) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
			tmp |= RALINK_GPIO((led.gpio-32));
			*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		} else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
			tmp |= RALINK_GPIO((led.gpio-64));
			*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		}
#else
		if (led.gpio <= 23) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
#if defined (RALINK_GPIO_HAS_4524)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_5124)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else if (led.gpio <= 71) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		}
		else {
#if defined (RALINK_GPIO_HAS_7224)
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
			tmp |= RALINK_GPIO((led.gpio-72));
			*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
#else
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
			tmp |= RALINK_GPIO((led.gpio-72));
			*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
#endif
		}
#endif
#endif
#if RALINK_LED_DEBUG
		printk("dir_%x gpio_%d - %d %d %d %d %d\n", tmp,
				led.gpio, led.on, led.off, led.blinks,
				led.rests, led.times);
#endif
	}
	else {
		printk(KERN_ERR NAME ": gpio(%d) out of range\n", led.gpio);
		return -1;
	}
	return 0;
#else
	printk(KERN_ERR NAME ": gpio led support not built\n");
	return -1;
#endif
}
EXPORT_SYMBOL(ralink_gpio_led_set);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
long ralink_gpio_ioctl(struct file *file, unsigned int req,
		unsigned long arg)
#else
int ralink_gpio_ioctl(struct inode *inode, struct file *file, unsigned int req,
		unsigned long arg)
#endif
{
	unsigned long tmp;
	ralink_gpio_reg_info info;
#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_info led;
#endif

	req &= RALINK_GPIO_DATA_MASK;

	switch(req) {
	case RALINK_GPIO_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIODIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_READ: //RALINK_GPIO_READ_INT
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO_WRITE: //RALINK_GPIO_WRITE_INT
		*(volatile u32 *)(RALINK_REG_PIODATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET: //RALINK_GPIO_SET_INT
		*(volatile u32 *)(RALINK_REG_PIOSET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_CLEAR: //RALINK_GPIO_CLEAR_INT
		*(volatile u32 *)(RALINK_REG_PIORESET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_ENABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_DISABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_REG_IRQ:
		copy_from_user(&info, (ralink_gpio_reg_info *)arg, sizeof(info));
		if (0 <= info.irq && info.irq < RALINK_GPIO_NUMBER) {
			ralink_gpio_info[info.irq].pid = info.pid;
#if defined (RALINK_GPIO_HAS_2722)
			if (info.irq <= 21) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722RENA));
				tmp |= (0x1 << (info.irq-22));
				*(volatile u32 *)(RALINK_REG_PIO2722RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722FENA));
				tmp |= (0x1 << (info.irq-22));
				*(volatile u32 *)(RALINK_REG_PIO2722FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_9532)
			if (info.irq <= 31) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			} else if (info.irq <= 63) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332RENA));
				tmp |= (0x1 << (info.irq-32));
				*(volatile u32 *)(RALINK_REG_PIO6332RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332FENA));
				tmp |= (0x1 << (info.irq-32));
				*(volatile u32 *)(RALINK_REG_PIO6332FENA) = cpu_to_le32(tmp);
			} else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564RENA));
				tmp |= (0x1 << (info.irq-64));
				*(volatile u32 *)(RALINK_REG_PIO9564RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564FENA));
				tmp |= (0x1 << (info.irq-64));
				*(volatile u32 *)(RALINK_REG_PIO9564FENA) = cpu_to_le32(tmp);
			}
#else
			if (info.irq <= 23) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			}
#if defined (RALINK_GPIO_HAS_4524)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO4540RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO4540FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_5124)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO5140RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO5140FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else if (info.irq <= 71) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO7140RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO7140FENA) = cpu_to_le32(tmp);
			}
			else {
#if defined (RALINK_GPIO_HAS_7224)
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72RENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO72RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72FENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO72FENA) = cpu_to_le32(tmp);
#else
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572RENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO9572RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572FENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO9572FENA) = cpu_to_le32(tmp);
#endif
			}
#endif
#endif
		}
		else
			printk(KERN_ERR NAME ": irq number(%d) out of range\n",
					info.irq);
		break;

#if defined (RALINK_GPIO_HAS_2722)
	case RALINK_GPIO2722_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		break;
	case RALINK_GPIO2722_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		break;
	case RALINK_GPIO2722_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO2722_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO2722DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_SET:
		*(volatile u32 *)(RALINK_REG_PIO2722SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO2722SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_9532)
	case RALINK_GPIO6332_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		break;
	case RALINK_GPIO6332_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		break;
	case RALINK_GPIO6332_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO6332_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO6332DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_SET:
		*(volatile u32 *)(RALINK_REG_PIO6332SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO6332SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		break;
	case RALINK_GPIO9564_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		break;
	case RALINK_GPIO9564_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO9564_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO9564DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET:
		*(volatile u32 *)(RALINK_REG_PIO9564SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO9564SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_4524)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO4540_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		break;
	case RALINK_GPIO4540_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		break;
	case RALINK_GPIO4540_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO4540_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO4540DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_SET:
		*(volatile u32 *)(RALINK_REG_PIO4540SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO4540SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_5124)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO5140_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		break;
	case RALINK_GPIO5140_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		break;
	case RALINK_GPIO5140_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO5140_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO5140DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_SET:
		*(volatile u32 *)(RALINK_REG_PIO5140SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO5140SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO7140_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		break;
	case RALINK_GPIO7140_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		break;
	case RALINK_GPIO7140_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO7140_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO7140DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_SET:
		*(volatile u32 *)(RALINK_REG_PIO7140SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO7140SET) = cpu_to_le32(arg);
		break;
#if defined (RALINK_GPIO_HAS_7224)
	case RALINK_GPIO72_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
		break;
	case RALINK_GPIO72_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
		break;
	case RALINK_GPIO72_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO72_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO72DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_SET:
		*(volatile u32 *)(RALINK_REG_PIO72SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO72SET) = cpu_to_le32(arg);
		break;
#else
	case RALINK_GPIO9572_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
		break;
	case RALINK_GPIO9572_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
		break;
	case RALINK_GPIO9572_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO9572_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO9572DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_SET:
		*(volatile u32 *)(RALINK_REG_PIO9572SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO9572SET) = cpu_to_le32(arg);
		break;
#endif
#endif

	case RALINK_GPIO_LED_SET:
#ifdef CONFIG_RALINK_GPIO_LED
		copy_from_user(&led, (ralink_gpio_led_info *)arg, sizeof(led));
		ralink_gpio_led_set(led);
#else
		printk(KERN_ERR NAME ": gpio led support not built\n");
#endif
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

int ralink_gpio_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_INC_USE_COUNT;
#else
	try_module_get(THIS_MODULE);
#endif
    INIT_WORK(&gpio_event_hold, gpio_hold_notify);
    INIT_WORK(&gpio_event_click, gpio_click_notify);
	return 0;
}

int ralink_gpio_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#else
	module_put(THIS_MODULE);
#endif
	return 0;
}

struct file_operations ralink_gpio_fops =
{
	owner:		THIS_MODULE,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	unlocked_ioctl:	ralink_gpio_ioctl,
#else
	ioctl:		ralink_gpio_ioctl,
#endif
	open:		ralink_gpio_open,
	release:	ralink_gpio_release,
};

#ifdef CONFIG_RALINK_GPIO_LED

#if RALINK_GPIO_LED_LOW_ACT

#define __LED_ON(gpio)      ra_gpio_led_clr |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio)     ra_gpio_led_set |= RALINK_GPIO(gpio);
#define __LED2722_ON(gpio)  ra_gpio2722_led_clr |= RALINK_GPIO((gpio-22));
#define __LED2722_OFF(gpio) ra_gpio2722_led_set |= RALINK_GPIO((gpio-22));
#define __LED3924_ON(gpio)  ra_gpio3924_led_clr |= RALINK_GPIO((gpio-24));
#define __LED3924_OFF(gpio) ra_gpio3924_led_set |= RALINK_GPIO((gpio-24));
#define __LED4540_ON(gpio)  ra_gpio4540_led_clr |= RALINK_GPIO((gpio-40));
#define __LED4540_OFF(gpio) ra_gpio4540_led_set |= RALINK_GPIO((gpio-40));
#define __LED5140_ON(gpio)  ra_gpio5140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED5140_OFF(gpio) ra_gpio5140_led_set |= RALINK_GPIO((gpio-40));
#define __LED7140_ON(gpio)  ra_gpio7140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED7140_OFF(gpio) ra_gpio7140_led_set |= RALINK_GPIO((gpio-40));

#if defined (RALINK_GPIO_HAS_7224)
#define __LED72_ON(gpio)  ra_gpio72_led_clr |= RALINK_GPIO((gpio-72));
#define __LED72_OFF(gpio) ra_gpio72_led_set |= RALINK_GPIO((gpio-72));
#else
#define __LED9572_ON(gpio)  ra_gpio9572_led_clr |= RALINK_GPIO((gpio-72));
#define __LED9572_OFF(gpio) ra_gpio9572_led_set |= RALINK_GPIO((gpio-72));
#endif

#if defined (RALINK_GPIO_HAS_9532)
#define __LED6332_ON(gpio)  ra_gpio6332_led_clr |= RALINK_GPIO((gpio-32));
#define __LED6332_OFF(gpio) ra_gpio6332_led_set |= RALINK_GPIO((gpio-32));
#define __LED9564_ON(gpio)  ra_gpio9564_led_clr |= RALINK_GPIO((gpio-64));
#define __LED9564_OFF(gpio) ra_gpio9564_led_set |= RALINK_GPIO((gpio-64));
#endif

#else

#define __LED_ON(gpio)      ra_gpio_led_set |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio)     ra_gpio_led_clr |= RALINK_GPIO(gpio);
#define __LED2722_ON(gpio)  ra_gpio2722_led_set |= RALINK_GPIO((gpio-22));
#define __LED2722_OFF(gpio) ra_gpio2722_led_clr |= RALINK_GPIO((gpio-22));
#define __LED3924_ON(gpio)  ra_gpio3924_led_set |= RALINK_GPIO((gpio-24));
#define __LED3924_OFF(gpio) ra_gpio3924_led_clr |= RALINK_GPIO((gpio-24));
#define __LED4540_ON(gpio)  ra_gpio4540_led_set |= RALINK_GPIO((gpio-40));
#define __LED4540_OFF(gpio) ra_gpio4540_led_clr |= RALINK_GPIO((gpio-40));
#define __LED5140_ON(gpio)  ra_gpio5140_led_set |= RALINK_GPIO((gpio-40));
#define __LED5140_OFF(gpio) ra_gpio5140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED7140_ON(gpio)  ra_gpio7140_led_set |= RALINK_GPIO((gpio-40));
#define __LED7140_OFF(gpio) ra_gpio7140_led_clr |= RALINK_GPIO((gpio-40));
#if defined (RALINK_GPIO_HAS_7224)
#define __LED72_ON(gpio)  ra_gpio72_led_set |= RALINK_GPIO((gpio-72));
#define __LED72_OFF(gpio) ra_gpio72_led_clr |= RALINK_GPIO((gpio-72));
#else
#define __LED9572_ON(gpio)  ra_gpio9572_led_set |= RALINK_GPIO((gpio-72));
#define __LED9572_OFF(gpio) ra_gpio9572_led_clr |= RALINK_GPIO((gpio-72));
#endif

#if defined (RALINK_GPIO_HAS_9532)
#define __LED6332_ON(gpio)  ra_gpio6332_led_set |= RALINK_GPIO((gpio-32));
#define __LED6332_OFF(gpio) ra_gpio6332_led_clr |= RALINK_GPIO((gpio-32));
#define __LED9564_ON(gpio)  ra_gpio9564_led_set |= RALINK_GPIO((gpio-64));
#define __LED9564_OFF(gpio) ra_gpio9564_led_clr |= RALINK_GPIO((gpio-64));
#endif


#endif
/* add for c2 & c20i, yuanshang, 2013-11-14 */
static void pollingGpio(void);
/* add end */

static void ralink_gpio_led_do_timer(unsigned long unused)
#if 1
	{
		pollingGpio();
		
		init_timer(&ralink_gpio_led_timer);
		ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
		add_timer(&ralink_gpio_led_timer);
	}
#else
{
	int i;
	unsigned int x;

#if defined (RALINK_GPIO_HAS_2722)
	for (i = 0; i < 22; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 22; i <  RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED2722_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED2722_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED2722_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED2722_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED2722_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED2722_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#else
	#if defined (RALINK_GPIO_HAS_9532)
	for (i = 0; i < 31; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1){ //-1 means unused	
			continue;
		}
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);	
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);	
			continue;
		}	
		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}	
	
	
	#else
	for (i = 0; i < 24; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#endif
#if defined (RALINK_GPIO_HAS_4524)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i <  RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED4540_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED4540_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED4540_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED4540_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED4540_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED4540_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_5124)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED5140_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED5140_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED5140_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED5140_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED5140_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED5140_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_9532)
	for (i = 32; i < 64; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED6332_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED6332_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED6332_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED6332_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED6332_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED6332_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 64; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED9564_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED9564_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED9564_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED9564_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED9564_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED9564_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i < 72; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED7140_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED7140_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED7140_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED7140_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED7140_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED7140_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_ON(i);
#else
			__LED9572_ON(i);
#endif
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_ON(i);
#else
			__LED9572_ON(i);
#endif
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
#if defined (RALINK_GPIO_HAS_7224)
				__LED72_OFF(i);
#else
				__LED9572_OFF(i);
#endif
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#endif
#endif

	//always turn the power LED on
#ifdef CONFIG_RALINK_RT2880
	__LED_ON(12);
#elif defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT2883)
	__LED_ON(9);
#endif

	*(volatile u32 *)(RALINK_REG_PIORESET) = ra_gpio_led_clr;
	*(volatile u32 *)(RALINK_REG_PIOSET) = ra_gpio_led_set;
#if defined (RALINK_GPIO_HAS_2722)
	*(volatile u32 *)(RALINK_REG_PIO2722RESET) = ra_gpio2722_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO2722SET) = ra_gpio2722_led_set;
#elif defined (RALINK_GPIO_HAS_4524)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO4540RESET) = ra_gpio4540_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO4540SET) = ra_gpio4540_led_set;
#elif defined (RALINK_GPIO_HAS_5124)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO5140RESET) = ra_gpio5140_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO5140SET) = ra_gpio5140_led_set;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO7140RESET) = ra_gpio7140_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO7140SET) = ra_gpio7140_led_set;
#if defined (RALINK_GPIO_HAS_7224)
	*(volatile u32 *)(RALINK_REG_PIO72RESET) = ra_gpio72_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO72SET) = ra_gpio72_led_set;
#else
	*(volatile u32 *)(RALINK_REG_PIO9572RESET) = ra_gpio9572_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO9572SET) = ra_gpio9572_led_set;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	*(volatile u32 *)(RALINK_REG_PIO6332RESET) = ra_gpio6332_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO6332SET) = ra_gpio6332_led_set;
	*(volatile u32 *)(RALINK_REG_PIO9564RESET) = ra_gpio9564_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO9564SET) = ra_gpio9564_led_set;
#endif

#if RALINK_LED_DEBUG
	printk("led_set= %x, led_clr= %x\n", ra_gpio_led_set, ra_gpio_led_clr);
#if defined (RALINK_GPIO_HAS_2722)
	printk("led2722_set= %x, led2722_clr= %x\n", ra_gpio2722_led_set, ra_gpio2722_led_clr);
#elif defined (RALINK_GPIO_HAS_4524)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led4540_set= %x, led4540_clr= %x\n", ra_gpio4540_led_set, ra_gpio4540_led_set);
#elif defined (RALINK_GPIO_HAS_5124)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led5140_set= %x, led5140_clr= %x\n", ra_gpio5140_led_set, ra_gpio5140_led_set);
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led7140_set= %x, led7140_clr= %x\n", ra_gpio7140_led_set, ra_gpio7140_led_set);
#if defined (RALINK_GPIO_HAS_7224)
	printk("led72_set= %x, led72_clr= %x\n", ra_gpio72_led_set, ra_gpio72_led_set);
#else
	printk("led9572_set= %x, led9572_clr= %x\n", ra_gpio9572_led_set, ra_gpio9572_led_set);
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	printk("led6332_set= %x, led6332_clr= %x\n", ra_gpio6332_led_set, ra_gpio6332_led_clr);
	printk("led9564_set= %x, led9564_clr= %x\n", ra_gpio9564_led_set, ra_gpio9564_led_set);
#endif
#endif

	ra_gpio_led_set = ra_gpio_led_clr = 0;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_set = ra_gpio2722_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio4540_led_set = ra_gpio4540_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio5140_led_set = ra_gpio5140_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio7140_led_set = ra_gpio7140_led_clr = 0;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_set = ra_gpio72_led_clr = 0;
#else
	ra_gpio9572_led_set = ra_gpio9572_led_clr = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_set = ra_gpio6332_led_clr = 0;
	ra_gpio9564_led_set = ra_gpio9564_led_clr = 0;
#endif

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}
#endif

void ralink_gpio_led_init_timer(void)
{
	int i;

	for (i = 0; i < RALINK_GPIO_NUMBER; i++)
		ralink_gpio_led_data[i].gpio = -1; //-1 means unused
#if RALINK_GPIO_LED_LOW_ACT
	ra_gpio_led_set = 0xffffffff;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_set = 0xff;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio4540_led_set = 0xff;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio5140_led_set = 0xfff;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio7140_led_set = 0xffffffff;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_set = 0xffffff;
#else
	ra_gpio9572_led_set = 0xffffff;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_set = 0xffffffff;
	ra_gpio9564_led_set = 0xffffffff;
#endif
#else // RALINK_GPIO_LED_LOW_ACT //
	ra_gpio_led_clr = 0xffffffff;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_clr = 0xff;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio4540_led_clr = 0xff;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio5140_led_clr = 0xfff;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio7140_led_clr = 0xffffffff;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_clr = 0xffffff;
#else
	ra_gpio9572_led_clr = 0xffffff;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_clr = 0xffffffff;
	ra_gpio9564_led_clr = 0xffffffff;
#endif
#endif // RALINK_GPIO_LED_LOW_ACT //

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.function = ralink_gpio_led_do_timer;
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}
#endif

int setGpioData(u32 gpio, u32 data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 95)
	{
		printk(KERN_ERR NAME ": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 31)
	{
		/* RALINK_REG_PIODATA for GPIO 0~31 */
#if defined(CONFIG_TP_MODEL_MR3420V5) || defined(CONFIG_TP_MODEL_MR3020V3)
	if(2 == gpio || 3 == gpio)
		reg = RALINK_SYSCTL_BASE + 0x3c;
	else
#endif
		reg = RALINK_REG_PIODATA;
		bit = (1 << gpio);
	}
	else if (gpio <= 63)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 32~63 */
		reg = RALINK_REG_PIO6332DATA;
		bit = (1 << (gpio - 32));
	}
	else if (gpio <= 95)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 64~95 */
		reg = RALINK_REG_PIO9564DATA;
		bit = (1 << (gpio - 64));
	}

	/* Set to reg base on bit and data */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (0 == data)
	{
		tmp &= ~bit;
	}
	else
	{
		tmp |= bit;
	}
	*(volatile u32 *)(reg) = tmp;
	return 0;
}
int getGpioData(u32 gpio, u32 *data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 95)
	{
		printk(KERN_ERR NAME ": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 31)
	{
		/* RALINK_REG_PIODATA for GPIO 0~31 */
		reg = RALINK_REG_PIODATA;
		bit = (1 << gpio);
	}
	else if (gpio <= 63)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 32~63 */
		reg = RALINK_REG_PIO6332DATA;
		bit = (1 << (gpio - 32));
	}
	else if (gpio <= 95)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 64~95 */
		reg = RALINK_REG_PIO9564DATA;
		bit = (1 << (gpio - 64));
	}

	/* Get to reg base on bit */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (bit & tmp)
	{
		*data = 1;
	}
	else
	{
		*data = 0;
	}
	return 0;
}
enum GPIO_ACT_TYPE
{
	GPIO_ACT_OFF	= 0,
	GPIO_ACT_ON 	= 1,
	GPIO_ACT_BLINK	= 2
};

enum GPIO_PHY_TYPE
{
	GPIO_PHY_ON 	= 0,
	GPIO_PHY_OFF	= 1
};

enum GPIO_FREG_TYPE
{
	GPIO_FREQ_NO	= 0,
	GPIO_FREQ_FAST	= 5, /* 5 = 250 / RALINK_GPIO_CYCLE_UNIT */
	GPIO_FREQ_SLOW = 12 /* 12 = 600 / RALINK_GPIO_CYCLE_UNIT */
};

enum GPIO_FLAG_TYPE
{
	GPIO_FLAG_UNCHANGED = 0,
	GPIO_FLAG_CHANGED = 1
};

enum SIGNAL_STRENGTH_TYPE
{
	SIGNAL_S0 = 0,
	SIGNAL_S1 = 1,
	SIGNAL_S2 = 2,
	SIGNAL_S3 = 3
};
static int signal_strength = 0;

typedef struct _TP_LED_CELL
{
	u32 gpioId;
	u32 gpioAction;
	u32 gpioFreq;
	u32 gpioStatus;
	u32 gpioTimes;
	u32 gpioFlag;
	
}TP_LED_CELL;

#if INCLUDE_SINGLE_LED
enum STATE_FLAG_TYPE
{
	STATE_FLAG_INVALID = 0,
	STATE_FLAG_VALID = 1
};

typedef struct _TP_SINGLE_LED_CELL
{
	u32 stateFlag;
	u32 stateValidTimes; /* 3600 = 180000 / RALINK_GPIO_CYCLE_UNIT = 3min, 0 means valid all the time */
}TP_SINGLE_LED_CELL;

/* should be 1~31 */
enum SINGLE_LED_STATE_TYPE
{
	SINGLE_LED_STATE_UNKOWN = 0,
	SINGLE_LED_STATE_INIT = 1,
	SINGLE_LED_STATE_UPDATE = 2,
	SINGLE_LED_STATE_RESET = 3,
	SINGLE_LED_STATE_WPS = 4,
	SINGLE_LED_STATE_NO_WAN_CONN = 5,
	SINGLE_LED_STATE_NORMAL = 6,
	SINGLE_LED_STATE_MAX
};

typedef struct SINGLE_LED_STATE_ACTION_INFO
{
	u32 paramAction;
	u32 paramFreq;
	u32 paramColor;
	u32 paramValidTimes;
}SINGLE_LED_STATE_ACTION_INFO;

TP_SINGLE_LED_CELL c2_single_led;

const SINGLE_LED_STATE_ACTION_INFO g_stateStartTable[] = {
/* 0 */{NULL, NULL, NULL, NULL},
/* 1 */{GPIO_ACT_BLINK, GPIO_FREQ_SLOW, GPIO_LED_INTERNET_ORANGE, 0},
/* 2 */{GPIO_ACT_BLINK, GPIO_FREQ_SLOW, GPIO_LED_INTERNET_BLUE, 0},
/* 3 */{GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_INTERNET_BLUE, 0},
/* 4 */{GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_INTERNET_BLUE, 0},
/* 5 */{GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE, 0},
/* 6 */{GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_BLUE, 0},
};

const SINGLE_LED_STATE_ACTION_INFO g_stateStopTable[] = {
/* 0 */{NULL, NULL, NULL, NULL},
/* 1 */{GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE, 0},
/* 2 */{GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_BLUE, 0},
/* 3 */{GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_BLUE, 0},
/* 4 */{GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_BLUE, 0},
/* 5 */{GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE, 0},
/* 6 */{GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_BLUE, 0},
};

#define SET_STATE_FLAG(x,n) (x=(x|((1U)<<(n-1))))
#define CLEAR_STATE_FLAG(x,n) (x=(x&~((1U)<<(n-1))))
#define GET_CURR_STATE(x,n) do {	\
								if (x == 0)	\
									n = SINGLE_LED_STATE_NORMAL;	\
								else	\
									for(n=1;((x&((1U)<<(n-1)))==0);n++);	\
							} while(0)
#endif

#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
void initGpioPins(void)
{
	int is_mr6400v5 = 0;
#if defined (CONFIG_TP_MODEL_MR6400V5)
	is_mr6400v5 = 1;
#endif
	if (pcie0_disable && (0 == is_mr6400v5)) /* for MR6400v3 */
	{
		GPIO_LED_POWER = 37;
		GPIO_LED_INTERNET = 39;
		GPIO_LED_WPS = 40;
		GPIO_LED_WLAN_2G4 = 40;
		GPIO_LED_LAN = 41;
		GPIO_LED_SIGNAL_S1 = 42;
		GPIO_LED_SIGNAL_S2 = 43;
		GPIO_LED_SIGNAL_S3 = 44;

		GPIO_BTN_WPS_RST = 38;
		GPIO_BTN_WIFI = 46;
	}
	else /* for MR200v3 & MR400v3 */
	{
		GPIO_LED_POWER = 39;
		GPIO_LED_INTERNET = 40;
		GPIO_LED_WPS = 4;
		GPIO_LED_WLAN_2G4 = 4;
		GPIO_LED_WLAN_5G = 4;
		GPIO_LED_LAN = 5;
		GPIO_LED_SIGNAL_S1 = 41;
		GPIO_LED_SIGNAL_S2 = 42;
		GPIO_LED_SIGNAL_S3 = 43;

		GPIO_BTN_WPS_RST = 38;
		GPIO_BTN_WIFI = 46;
	}
}
#endif

void initLedData(TP_LED_CELL *pGpio, u32 gpio);

/* Led numbers by GPIO */
#if defined(CONFIG_TP_MODEL_WR840NV4)
#define LED_NUM (5)
#elif defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
#define LED_NUM (7)
#elif defined(CONFIG_TP_MODEL_MR100V1)
#define LED_NUM (8)
#elif defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
#define LED_NUM (11)
#elif  defined(CONFIG_TP_MODEL_MR3020V3)
#define LED_NUM (5)
#elif  defined(CONFIG_TP_MODEL_MR3420V5)
#define LED_NUM (7)
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) 
#define LED_NUM (9)
#elif defined(CONFIG_TP_MODEL_C50V4) || defined(CONFIG_TP_MODEL_C20V4) 
#define LED_NUM (7)
#endif
TP_LED_CELL c2_led[LED_NUM];
static u32 c2_led_enable = 1;

#define RESET_BUTTON_GPIO (38)
#define WLAN_BUTTON_GPIO (37)

void initLedData_W8(void)
{
#if defined(CONFIG_TP_MODEL_WR840NV4)
	initLedData(&c2_led[0], 36);
	initLedData(&c2_led[1], 37);
	initLedData(&c2_led[2], 41);
	initLedData(&c2_led[3], 43);
	initLedData(&c2_led[4], 44);
#elif defined(CONFIG_TP_MODEL_MR100V1) || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_INTERNET);
	initLedData(&c2_led[2], GPIO_LED_WPS);
	initLedData(&c2_led[3], GPIO_LED_LAN);
	initLedData(&c2_led[4], GPIO_LED_SIGNAL_S1);
	initLedData(&c2_led[5], GPIO_LED_SIGNAL_S2);
	initLedData(&c2_led[6], GPIO_LED_SIGNAL_S3);
	initLedData(&c2_led[7], GPIO_USB_POWER);
#if defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
	initLedData(&c2_led[8], GPIO_USB_BOOT);
	initLedData(&c2_led[9], GPIO_USB_DC);
	initLedData(&c2_led[10], GPIO_USB_RESET);
#endif
#elif defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_INTERNET);
	initLedData(&c2_led[2], GPIO_LED_WPS);
	initLedData(&c2_led[3], GPIO_LED_LAN);
	initLedData(&c2_led[4], GPIO_LED_SIGNAL_S1);
	initLedData(&c2_led[5], GPIO_LED_SIGNAL_S2);
	initLedData(&c2_led[6], GPIO_LED_SIGNAL_S3);
#elif defined(CONFIG_TP_MODEL_MR3020V3)
//	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_INTERNET_GREEN);
	initLedData(&c2_led[2], GPIO_LED_WPS);
	initLedData(&c2_led[3], GPIO_LED_LAN);
	initLedData(&c2_led[4], GPIO_LED_WLAN_2G4);
#elif defined(CONFIG_TP_MODEL_MR3420V5)
	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_USB);
	initLedData(&c2_led[2], GPIO_LED_INTERNET_ORANGE);
	initLedData(&c2_led[3], GPIO_LED_INTERNET_GREEN);
	initLedData(&c2_led[4], GPIO_LED_WPS);
	initLedData(&c2_led[5], GPIO_LED_LAN);
	initLedData(&c2_led[6], GPIO_LED_WLAN_2G4);
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) || defined(CONFIG_TP_MODEL_C20V4) 
	initLedData(&c2_led[0], 11);
	initLedData(&c2_led[1], 36);
	initLedData(&c2_led[2], 39);
	initLedData(&c2_led[3], 40);
	initLedData(&c2_led[4], 41);
	initLedData(&c2_led[5], 42);
	initLedData(&c2_led[6], 43);
	initLedData(&c2_led[7], 44);
	initLedData(&c2_led[8], 46);
#endif
}

void initGpioDir_W8(u32 flag)
{
	u32 tmp;
#if defined(CONFIG_TP_MODEL_WR840NV4)

	/* OUTPUT GPIO
	 * GPIO36: Power
	 * GPIO37: WPS
	 * GPIO41: LAN
	 * GPIO43: WAN
	 * GPIO44: WLAN 2.4G
	 */
	/* Set Direction to output */

	/* RALINK_REG_PIO6332DIR for GPIO 32~63 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= (1 << (36-32)) | (1 << (37-32)) | (1 << (41-32)) | (1 << (43-32)) | (1 << (44-32));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

	/* INPUT GPIO
	 * GPIO38:RESET/WPS
	 */
	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp &= ~((1 << (38-32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;	

#elif defined(CONFIG_TP_MODEL_MR3020V3)

	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
	tmp |= ((1 << (GPIO_LED_WPS)) | (1 << (GPIO_LED_LAN)));
	*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;

	/* RALINK_REG_PIO6332DIR for GPIO 32~63 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= ((1 << (GPIO_LED_POWER- 32)) | (1 << (GPIO_LED_INTERNET_GREEN - 32)) | (1 << (GPIO_LED_WLAN_2G4- 32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

	/* INPUT GPIO
	 * GPIO46:WLAN ON/OFF (WIFI switch for 3420V5)
	 * GPIO38:RESET Button(WPS switch for 3420V5)
	 */
	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	//tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	tmp &= (~((1 << (GPIO_BTN_WPS_RST- 32)) | (1 << (GPIO_BTN_MODE_C1- 32)) | (1 << (GPIO_BTN_MODE_C2- 32))));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

#elif defined(CONFIG_TP_MODEL_MR100V1)
    /* GPIO CTRL 1 for GPIO 32~63 OUTPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
    tmp |= ((1 << (GPIO_LED_POWER - 32)) | (1 << (GPIO_LED_INTERNET - 32)) | (1 << (GPIO_LED_WPS - 32)) | (1 << (GPIO_LED_LAN - 32)) |
         (1 << (GPIO_LED_SIGNAL_S1 - 32)) | (1 << (GPIO_LED_SIGNAL_S2 - 32)) | (1 << (GPIO_LED_SIGNAL_S3 - 32)) | (1 << (GPIO_USB_POWER - 32)));
    *(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

    /* GPIO CTRL 1 for GPIO 32~63 INPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
    tmp &= (~(1 << (GPIO_BTN_WPS_RST- 32)));
    *(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

#elif defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
	/* GPIO CTRL 0 for GPIO 2 3 4 16 21 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
	tmp |= ((1 << GPIO_USB_BOOT) | (1 << GPIO_USB_RESET) | (1 << GPIO_OUT_ATN_MAIN) | (1 << GPIO_OUT_ATN_SECD));
	*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
    /* GPIO CTRL 1 for GPIO 32~63 OUTPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
    tmp |= ((1 << (GPIO_LED_POWER - 32)) | (1 << (GPIO_LED_INTERNET - 32)) | (1 << (GPIO_LED_WPS - 32)) | (1 << (GPIO_LED_LAN - 32)) |
         (1 << (GPIO_LED_SIGNAL_S1 - 32)) | (1 << (GPIO_LED_SIGNAL_S2 - 32)) | (1 << (GPIO_LED_SIGNAL_S3 - 32)) | (1 << (GPIO_USB_POWER - 32)) |
		 (1 << (GPIO_USB_DC - 32)));
    *(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

    /* GPIO CTRL 1 for GPIO 32~63 INPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
    tmp &= (~(1 << (GPIO_BTN_WPS_RST- 32)));
    *(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

#elif defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)

	int is_mr6400v5 = 0;
#if defined (CONFIG_TP_MODEL_MR6400V5)
	is_mr6400v5 = 1;
#endif
	if (pcie0_disable && (0 == is_mr6400v5)) /* for MR6400v3 */
	{
		/* GPIO CTRL 1 for GPIO 37 & 39~44 OUTPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp |= ((1 << (GPIO_LED_POWER - 32)) | (1 << (GPIO_LED_INTERNET - 32)) | (1 << (GPIO_LED_WPS - 32)) | (1 << (GPIO_LED_LAN - 32)) |
			 (1 << (GPIO_LED_SIGNAL_S1 - 32)) | (1 << (GPIO_LED_SIGNAL_S2 - 32)) | (1 << (GPIO_LED_SIGNAL_S3 - 32)));
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

		/* GPIO CTRL 1 for GPIO 38 & 46 INPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp &= (~(1 << (GPIO_BTN_WPS_RST- 32) | (1 << (GPIO_BTN_WIFI- 32))));
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

		/* GPIO CTRL 0 for GPIO 0 INPUT for model version*/
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp &= (~(1 << GPIO_MODEL_VER));
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
	}
	else /* for MR200v3 & MR400v3 */
	{
		if (1 == flag)
		{
			/* GPIO CTRL 0 for GPIO 4~5 OUTPUT */
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= ((1 << GPIO_LED_WPS) | (1 << GPIO_LED_LAN));
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
		else
		{
			/* GPIO CTRL 0 for GPIO 5 OUTPUT */
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= ((1 << GPIO_LED_LAN));
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;

			/* GPIO CTRL 0 for GPIO 4 INPUT */
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp &= (~(1 << GPIO_LED_WPS));
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}

		/* GPIO CTRL 0 for GPIO#11 INPUT for model version*/
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp &= (~(1 << GPIO_MODEL_VER));
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;

		/* GPIO CTRL 1 for GPIO 39~43 OUTPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp |= ((1 << (GPIO_LED_POWER - 32)) | (1 << (GPIO_LED_INTERNET - 32)) | (1 << (GPIO_LED_SIGNAL_S1 - 32)) |
			(1 << (GPIO_LED_SIGNAL_S2 - 32)) | (1 << (GPIO_LED_SIGNAL_S3 - 32)));
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

		/* GPIO CTRL 1 for GPIO 38 & 46 INPUT */
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp &= (~(1 << (GPIO_BTN_WPS_RST - 32) | (1 << (GPIO_BTN_WIFI - 32))));
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
	}
	//printk("after---GPIO_BTN_WPS_RST=%d GPIO_BTN_WIFI=%d PIO CTRL 1=%08x\n", GPIO_BTN_WPS_RST, GPIO_BTN_WIFI, tmp);

#elif defined(CONFIG_TP_MODEL_MR3420V5)

	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
	tmp |= ((1 << (GPIO_LED_POWER)) | (1 << (GPIO_LED_USB)) | (1 << (GPIO_LED_INTERNET_ORANGE)) | (1 << (GPIO_LED_INTERNET_GREEN)));
	*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;

	/* RALINK_REG_PIO6332DIR for GPIO 32~63 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= ((1 << (GPIO_LED_WPS- 32)) | (1 << (GPIO_LED_LAN - 32)) | (1 << (GPIO_LED_WLAN_2G4- 32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

	/* INPUT GPIO
	 * GPIO46:WLAN ON/OFF (WIFI switch for 3420V5)
	 * GPIO38:RESET Button(WPS switch for 3420V5)
	 */
	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	//tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	tmp &= (~(1 << (GPIO_BTN_WPS_RST- 32) | (1 << (GPIO_BTN_WIFI- 32))));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) || defined(CONFIG_TP_MODEL_C20V4) 
	/* GPIO
	 * GPIO36: POWER
	 * GPIO46:WPS	
	 * GPIO38:RESET Button(WPS switch for 841v13)
 	 * GPIO39/40/41/42:LAN
 	 * GPIO43:INTERNET_GREEN
 	 * GPIO11:INTERNET_ORANGE
	 * GPIO44:WLAN(2.4G)
	 * GPIO37:WLAN ON/OFF (WPS switch for 845v3)
	 */

	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
	
	/* RALINK_REG_PIO6332DIR for GPIO 32~63 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= ((1 << (36 - 32)) | (1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | 
			(1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));

	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

	/* INPUT GPIO
	 * GPIO37:WLAN ON/OFF (WPS switch for 845v3)
	 * GPIO38:RESET Button(WPS switch for 841v13)
	 */
	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp; 

#endif
	return;
}

void initGpioMode_W8(void)
{
	u32 gpiomode;
	u32 gpiomode2;
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
	gpiomode2 = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE2));

	//printk("before---gpiomode=%08x gpiomode2=%08x\n", gpiomode, gpiomode2);
#if defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) || defined(CONFIG_TP_MODEL_C20V4) 
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
#elif defined(CONFIG_TP_MODEL_WR840NV4)
	gpiomode |= (RALINK_GPIOMODE_REFCLK) | (RALINK_GPIOMODE_WDT) | (RALINK_GPIOMODE_PERST);

#elif defined(CONFIG_TP_MODEL_MR100V1)
	/* GPIO1 Mode for GPIO 37 & 38 & 46  */
	gpiomode &= (~((0x3 << 24) | (0x3 << 14)));
	gpiomode |= ((1 << 24) | (1 << 14));

#elif defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
	/* GPIO1 Mode for GPIO 3 & 4 & 37 & 38 & 46  */
	/* GPIO1 I2C b21=0, b20=1, set as common gpio */
	/* GPIO1 I2S b7=0, b6=1, set as common gpio */
	gpiomode &= (~((0x3 << 24) | (0x3 << 14) | (0x3 << 6) | (0x3 << 20)));
	gpiomode |= ((1 << 24) | (1 << 14) | (1 << 6) | (1 << 20));

#elif defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)

	int is_mr6400v5 = 0;
#if defined (CONFIG_TP_MODEL_MR6400V5)
	is_mr6400v5 = 1;
#endif
	if (pcie0_disable && (0 == is_mr6400v5)) /* for MR6400v3 */
	{
		/* GPIO1 Mode for GPIO 37 & 38 & 46  */
		gpiomode &= (~((0x3 << 18) | (0x3 << 24) | (0x3 << 14)));
		gpiomode |= ((1 << 18) | (1 << 24) | (1 << 14));
	}
	else /* for MR200v3 & MR400v3 */
	{
		/* GPIO1 Mode for GPIO 4 & 5 & 38 & 46  */
		gpiomode &= (~((0x3 << 20) | (0x3 << 24) | (0x3 << 14)));
		gpiomode |= ((1 << 20) | (1 << 24) | (1 << 14));
	}

#elif defined(CONFIG_TP_MODEL_MR3420V5) || defined(CONFIG_TP_MODEL_MR3020V3)
	//gpiomode |= (RALINK_GPIOMODE_REFCLK) | (RALINK_GPIOMODE_WDT) | (RALINK_GPIOMODE_PERST);
	gpiomode &= ~((0x3 << 18) | (0x3 << 20));
	gpiomode |= (1 << 18) | (1 << 20);
#endif	
	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);

	gpiomode2 &= ((0xf << 12) | (0xf << 28));
	gpiomode2 |= (0x555 | (0x555 << 16));
	//printk("after---gpiomode=%08x gpiomode2=%08x\n", gpiomode, gpiomode2);
	*(volatile u32 *)(RALINK_REG_GPIOMODE2) = cpu_to_le32(gpiomode2);
	
	return;
}

void initLedData(TP_LED_CELL *pGpio, u32 gpio)
{
	pGpio->gpioId = gpio;
	pGpio->gpioAction = GPIO_ACT_OFF;
	pGpio->gpioFreq = GPIO_FREQ_NO;
	pGpio->gpioStatus = GPIO_ACT_OFF;

#if defined(GPIO_USB_RESET)
	if(gpio == GPIO_USB_RESET)
	{
		pGpio->gpioAction = GPIO_ACT_ON;
		pGpio->gpioStatus = GPIO_ACT_ON;
	}
#else
	setGpioData(gpio, GPIO_PHY_OFF);
#endif // GPIO_USB_RESET

#ifdef GPIO_USB_BOOT
	if(gpio == GPIO_USB_BOOT)
	{
		pGpio->gpioAction = GPIO_ACT_ON;
		pGpio->gpioStatus = GPIO_ACT_ON;
	}
#endif // GPIO_USB_BOOT
	pGpio->gpioTimes = 0;
	pGpio->gpioFlag = GPIO_FLAG_CHANGED;
}

#if defined(CONFIG_TP_MODEL_C50V4) || defined(CONFIG_TP_MODEL_C20V4)
void gpio_common_init(void)
{
	u32 gpiomode, tmp;
	/* GPIO1 Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)RALINK_REG_GPIOMODE);
	/*						 GPIO11 		   GPIO5 */
	gpiomode &= ~((0x3 << 0) | (0x3 << 20));
	/*					  GPIO38		 GPIO5 */
	gpiomode |= (1 << 14) | (1 << 20);
	
	//printk("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)RALINK_REG_GPIOMODE = cpu_to_le32(gpiomode);
	
	/* GPIO2 Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)RALINK_REG_GPIOMODE2);
	/*				   GPIO39 ~ GPIO44 */
	gpiomode &= ~0x0fff;
	/*				   GPIO39 ~ GPIO44 */
	gpiomode |= 0x0555;
	
	//printk("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)RALINK_REG_GPIOMODE2 = cpu_to_le32(gpiomode);	
	
	/* GPIO
	 * GPIO11: POWER LED
	 * GPIO39:WPS LED
	 * GPIO40:WLAN 5G LED
	 * GPIO41:LAN LED
	 * GPIO42:INTERNET ORANGE LED
	 * GPIO43:INTERNET GREEN LED
	 * GPIO44:WLAN 2.4G LED
	 * GPIO38:WPS /RESET BUTTON
	 * GPIO5:WLAN BUTTON
	 */
	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << 11);
	tmp &= ~(1 << 5);
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= 0x3f << (39 - 32);
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~(1 << (38 - 32));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_WPS);
	initLedData(&c2_led[2], GPIO_LED_WLAN_5G);
	initLedData(&c2_led[3], GPIO_LED_LAN);
	initLedData(&c2_led[4], GPIO_LED_INTERNET_ORANGE);
	initLedData(&c2_led[5], GPIO_LED_INTERNET_GREEN);
	initLedData(&c2_led[6], GPIO_LED_WLAN_2G4);

}
#endif

int setLedState(u32 action)
{
	u32 count = 0;
	TP_LED_CELL *pGpio = c2_led;

	if (action != GPIO_ACT_OFF && action != GPIO_ACT_ON)
		return -1;

	for (count = 0; count < LED_NUM; count++)
	{
#if defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
		if (pGpio[count].gpioId == GPIO_USB_RESET || pGpio[count].gpioId == GPIO_USB_BOOT
			|| pGpio[count].gpioId == GPIO_USB_DC)
		{
			continue;
		}
#endif
		pGpio[count].gpioFlag == GPIO_FLAG_CHANGED;
		if (action == GPIO_ACT_OFF)
		{
			setGpioData(pGpio[count].gpioId, GPIO_PHY_OFF);
		}
		else if (action == GPIO_ACT_ON)
		{
			pGpio[count].gpioFlag = GPIO_FLAG_CHANGED;	/* trigger polling once */
		}
	}

	return 0;
}

int setLedCfg(u32 action, u32 freq, u32 gpio)
{
	u32 count = 0;
	TP_LED_CELL *pGpio = c2_led;
	while(count < LED_NUM && (pGpio[count].gpioId != gpio))
	{
		count++;
	}
	if (count >= LED_NUM)
	{
		return -1;
	}

	if (action != pGpio[count].gpioAction || 
		(action == GPIO_ACT_BLINK && pGpio[count].gpioFreq != freq))
	{
		pGpio[count].gpioFlag = GPIO_FLAG_CHANGED;
	}
	else
	{
		// do not set UNCHANGED here
		return 0;	/* UNCHANGED to return directly */
	}

	if (action == GPIO_ACT_OFF)
	{
		pGpio[count].gpioAction = GPIO_ACT_OFF;
		pGpio[count].gpioFreq = GPIO_FREQ_NO;
		
	}
	else if (action == GPIO_ACT_ON )
	{
		pGpio[count].gpioAction = GPIO_ACT_ON;
		pGpio[count].gpioFreq = GPIO_FREQ_NO;
	}
	else if (action == GPIO_ACT_BLINK )
	{
		pGpio[count].gpioAction = GPIO_ACT_BLINK;
		pGpio[count].gpioFreq = freq;
		pGpio[count].gpioTimes = freq;
	}
	return 0;
	
}

#if INCLUDE_SINGLE_LED
void led_singleControl(u32 state, u32 flag)
{
	int currState = 0;
	//printk("newState=%d,flag=%d\n",state,flag);
	if (flag == STATE_FLAG_INVALID)
	{
		GET_CURR_STATE(c2_single_led.stateFlag,currState);
		CLEAR_STATE_FLAG(c2_single_led.stateFlag,state);
		if (state > currState)
			return;
	}
	else if (flag == STATE_FLAG_VALID)
	{
		GET_CURR_STATE(c2_single_led.stateFlag,currState);
		SET_STATE_FLAG(c2_single_led.stateFlag,state);
		if (state >= currState)
			return;
	}
	else
		return;

	//printk("c2_single_led.stateFlag=0x%x, oldState=%d\n",c2_single_led.stateFlag, currState);
	setLedCfg(g_stateStopTable[currState].paramAction, g_stateStopTable[currState].paramFreq, g_stateStopTable[currState].paramColor);
	GET_CURR_STATE(c2_single_led.stateFlag,currState);
	setLedCfg(g_stateStartTable[currState].paramAction, g_stateStartTable[currState].paramFreq, g_stateStartTable[currState].paramColor);
	c2_single_led.stateValidTimes = g_stateStartTable[currState].paramValidTimes;

}
#endif

#ifdef GPIO_LED_USB
void led_setUsbBlink(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_USB);
#endif
}

void led_setUsbOn(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_USB);
#endif
}
void led_setUsbOff(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_USB);
#endif
}
void led_setUsbFlash(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_USB);
#endif
}
#endif

void led_setSignalStrength(u32 strength)
{
#if (!INCLUDE_SINGLE_LED)
	switch (strength)
	{
	case SIGNAL_S0:
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S1);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S2);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S3);
		break;
	case SIGNAL_S1:
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S1);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S2);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S3);
		break;
	case SIGNAL_S2:
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S1);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S2);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S3);
		break;
	case SIGNAL_S3:
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S1);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S2);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S3);
		break;
	default:
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S1);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S2);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_SIGNAL_S3);
		break;
	}
#endif
}


void led_setSysOn(void)
{
#if INCLUDE_SINGLE_LED
	led_singleControl(SINGLE_LED_STATE_NORMAL, STATE_FLAG_VALID);
#else
	if (sys_wlan_state >= STATE_SYS_ON_WIFI_ON)
	{
		sys_wlan_state = STATE_SYS_ON_WIFI_ON;
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_POWER);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS);		/* MB110V1 green on */
	}
	else if (sys_wlan_state >= STATE_SYS_ON_WIFI_OFF)
	{
		sys_wlan_state = STATE_SYS_ON_WIFI_OFF;
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_POWER);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS);		/* MB110V1 orange on */
	}
#endif
}
void led_setSysOff(void)
{
#if INCLUDE_SINGLE_LED
	led_singleControl(SINGLE_LED_STATE_NORMAL, STATE_FLAG_INVALID);
#else
	sys_wlan_state = STATE_SYS_OFF;
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_POWER);
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS);	/* MB110V1 sys off */
#endif
}
void led_setSysFlash(void)
{
#if (!INCLUDE_SINGLE_LED)
	sys_wlan_state = STATE_SYS_BOOT_UPGRADE;
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_POWER);
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_SLOW, GPIO_LED_WPS);	/* MB110V1 green blink and orange off when upgrading */
#endif
}

void led_setWlanOn(void)
{
#if (!INCLUDE_SINGLE_LED)
	if (sys_wlan_state >= STATE_SYS_ON_WIFI_OFF)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WLAN_2G4);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_POWER);
		sys_wlan_state = STATE_SYS_ON_WIFI_ON;
	}
#endif
}
void led_setWlanOff(void)
{
#if (!INCLUDE_SINGLE_LED)
	if (sys_wlan_state >= STATE_SYS_ON_WIFI_OFF)
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WLAN_2G4);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_POWER);
		sys_wlan_state = STATE_SYS_ON_WIFI_OFF;
	}
#endif
}

void led_setWlan5gOn(void)
{
#if (!INCLUDE_SINGLE_LED) && defined(GPIO_LED_WLAN_5G)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WLAN_5G);
#endif
}
void led_setWlan5gOff(void)
{
#if (!INCLUDE_SINGLE_LED) && defined(GPIO_LED_WLAN_5G)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WLAN_5G);
#endif
}

#if (INCLUDE_LAN_BIT_LED && !INCLUDE_SINGLE_LED)
void led_setLanBits(u32 lanBits)
{
	u32 bit = 0;
	u32 maxLanPortNum = 4;
	u32 ledGpioBase = 42; /* GPIO 42~39 LAN(1~4) leds */

	initGpioMode_W8();
	while(bit < maxLanPortNum)
	{
		if(lanBits & (1 << bit))
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, ledGpioBase - bit);
		}
		else
		{
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, ledGpioBase - bit);
		}
		bit ++;
	}
}
#endif


void led_setLanOn(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_LAN);
#endif
}
void led_setLanOff(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_LAN);
#endif
}

void led_setWanOn(u32 internet)
{
#if defined(CONFIG_TP_MODEL_MB110V1)
	if (internet == 1)	/* MB110V1 WAN LED ON */
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET);
	}
	else if (internet == 2) 	/* MB110V1 WAN LED blink */
	{
		setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_INTERNET);
	}
	else 	/* MB110V1 WAN LED off */
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET);
	}
#elif INCLUDE_SINGLE_LED
	if (internet == 1)
	{
		led_singleControl(SINGLE_LED_STATE_NO_WAN_CONN,STATE_FLAG_INVALID);
	}
	else
	{
		if (wan_status == 1)
		{
			led_singleControl(SINGLE_LED_STATE_NO_WAN_CONN,STATE_FLAG_INVALID);
		}
		else
		{
			led_singleControl(SINGLE_LED_STATE_NO_WAN_CONN,STATE_FLAG_VALID);
		}
	}
#else
#if INCLUDE_INTERNET_COLOR_LED
	if (internet == 1)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
	}
	else
	{
		if (wan_status == 1)
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
		}
		else
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);

		}
	}
#else
	if (internet == 1)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET);
	}
	else
	{
		if (wan_status == 1)
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET);
		}
		else
		{
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET);
		}
	}
#endif
#endif
}

void led_setWanOff(u32 internet)
{
#if INCLUDE_SINGLE_LED
    led_singleControl(SINGLE_LED_STATE_NO_WAN_CONN,STATE_FLAG_VALID);
#else
#if INCLUDE_INTERNET_COLOR_LED
	if (internet == 0)/*unlink*/
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);

	}
	else /*link but no internet*/
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
	}
#else
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET);
#endif
#endif
}

void led_setWpsOn(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS);
#endif
}
void led_setWpsOff(void)
{
#if (!INCLUDE_SINGLE_LED)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS);
#endif
}

void led_startWpsConn(void)
{
#if INCLUDE_SINGLE_LED
	led_singleControl(SINGLE_LED_STATE_WPS, STATE_FLAG_VALID);
#endif
}

void led_stopWpsConn(void)
{
#if INCLUDE_SINGLE_LED
	led_singleControl(SINGLE_LED_STATE_WPS, STATE_FLAG_INVALID);
#endif
}
/* added by zengweiji */
void led_WpsFinish(void)
{
#if defined(CONFIG_TP_MODEL_MB110V1)	/* WPS end, Green for sys */
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS);

	/* if WPS finished, then set led on if wlan24g or wlan5g is on */
#elif (!INCLUDE_SINGLE_LED)
	if(wlan_24G_status != GPIO_ACT_OFF || wlan_5G_status != GPIO_ACT_OFF)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS);
	}
#endif
}
/* end added */

#ifdef GPIO_USB_POWER
void flag_setUsbPowerOn(void)
{
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_USB_POWER);
}
void flag_setUsbPowerOff(void)
{
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_USB_POWER);
}
#endif

#ifdef GPIO_USB_BOOT
void flag_setUsbBootOn(void)
{
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_USB_BOOT);
}
void flag_setUsbBootOff(void)
{
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_USB_BOOT);
}
#endif
#ifdef GPIO_USB_DC
void flag_setUsbDcOn(void)
{
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_USB_DC);
}
void flag_setUsbDcOff(void)
{
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_USB_DC);
}
#endif
#ifdef GPIO_USB_RESET
void flag_setUsbResetOn(void)
{
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_USB_RESET);
}
void flag_setUsbResetOff(void)
{
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_USB_RESET);
}
#endif

typedef irqreturn_t(*sc_callback_t)(int, void *, struct pt_regs *);

static sc_callback_t registered_cb = NULL;
static sc_callback_t registered_cb_5G = NULL;
static void *cb_arg;
static void *cb_arg_5G;

void register_simple_config_callback (void *callback, void *arg)
{
    registered_cb = (sc_callback_t) callback;
    cb_arg = arg;
}
void register_simple_config_callback_5G (void *callback, void *arg)
{
    registered_cb_5G = (sc_callback_t) callback;
    cb_arg_5G = arg;
}

EXPORT_SYMBOL(register_simple_config_callback);
EXPORT_SYMBOL(register_simple_config_callback_5G);
#ifdef GPIO_LED_USB
EXPORT_SYMBOL(led_setUsbOn);
EXPORT_SYMBOL(led_setUsbOff);
EXPORT_SYMBOL(led_setUsbFlash);
#endif
EXPORT_SYMBOL(led_setWpsOn);
EXPORT_SYMBOL(led_setWpsOff);
EXPORT_SYMBOL(led_startWpsConn);
EXPORT_SYMBOL(led_stopWpsConn);
EXPORT_SYMBOL(led_WpsFinish);



/* return 0 means Test OK */
u32 (*fp_gpioTestLan)(void) = NULL;
EXPORT_SYMBOL(fp_gpioTestLan);
u32 (*fp_gpioTestLanOffWan)(void) = NULL;
EXPORT_SYMBOL(fp_gpioTestLanOffWan);
u32 (*fp_ethTestWan)(void) = NULL;
EXPORT_SYMBOL(fp_ethTestWan);


int resetCount = 0;
int wlanCount = 0;

extern int spi_flash_erase_config(void);


static void pollingGpio(void)
{
	u32 count = 0;
	TP_LED_CELL *pGpio = c2_led;
	u32 mask;
	static u32 ledCycCount = LED_CYCLE_TRIGGER;
	static u32 btnCycCount = BTN_CYCLE_TRIGGER;
	static u32 connCycCount = CONN_CYCLE_TRIGGER;
	static u32 btnUnprCount = 0;
	u32 buttonStat = 0;
#ifdef CONFIG_TP_MODEL_MR3020V3
	u32 button2ndStat = 0;
	static u32 sysMode = 0;
#endif
	static u32 isReset = 0;
	static u32 isWlan = 0;
	u32 tmpval = 0;
#ifdef INCLUDE_SINGLE_LED
		u32 currState = 0;
#endif
#if INCLUDE_LAN_BIT_LED
	u32 lanBits = 0;
#endif
	/* added by ZC for MR200v3 gpio ports conflicting with i2c ports */
	static u32 isFirst = 0;

	if (!isFirst)
	{
		initGpioDir_W8(1);
		initGpioMode_W8();
		isFirst = 1;
	}
	/* end added */

	/* Part 1, Do LED Display */
	btnCycCount--;
	connCycCount--;

	for (count = 0; count < LED_NUM; count++)
	{
		if (pGpio[count].gpioFlag == GPIO_FLAG_CHANGED)
		{
			pGpio[count].gpioFlag = GPIO_FLAG_UNCHANGED;
			/* printk("GPIO %d, act %d, freq %d, status %d, times %d\n", pGpio[count].gpioId, pGpio[count].gpioAction,
					pGpio[count].gpioFreq, pGpio[count].gpioStatus, pGpio[count].gpioTimes); */

			switch (pGpio[count].gpioAction)
			{
			case GPIO_ACT_OFF:
				/* turn off */
				if (c2_led_enable)
				{
					setGpioData(pGpio[count].gpioId, GPIO_PHY_OFF);
				}
#if defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
				if ((pGpio[count].gpioId == GPIO_USB_RESET || pGpio[count].gpioId == GPIO_USB_BOOT
					|| pGpio[count].gpioId == GPIO_USB_DC) && !c2_led_enable)
				{
					setGpioData(pGpio[count].gpioId, GPIO_PHY_OFF);
				}
#endif
				pGpio[count].gpioStatus = GPIO_ACT_OFF;
				pGpio[count].gpioTimes = 0;
				break;
			case GPIO_ACT_ON:
				/* turn on */
				if (c2_led_enable)
				{
					setGpioData(pGpio[count].gpioId, GPIO_PHY_ON);
				}
#if defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
				if ((pGpio[count].gpioId == GPIO_USB_RESET || pGpio[count].gpioId == GPIO_USB_BOOT
					|| pGpio[count].gpioId == GPIO_USB_DC) && !c2_led_enable)
				{
					setGpioData(pGpio[count].gpioId, GPIO_PHY_ON);
				}
#endif
				pGpio[count].gpioStatus = GPIO_ACT_ON;
				pGpio[count].gpioTimes = 0;
				break;
			case GPIO_ACT_BLINK:
				if (pGpio[count].gpioFreq == 0)
				{
					/* turn off */
					if (c2_led_enable)
					{
						setGpioData(pGpio[count].gpioId, GPIO_PHY_OFF);
					}
					pGpio[count].gpioStatus = GPIO_ACT_OFF;
					pGpio[count].gpioTimes = 0;
				}
				else
				{
					pGpio[count].gpioFlag = GPIO_FLAG_CHANGED;
					if (pGpio[count].gpioTimes == 0)
					{
						pGpio[count].gpioTimes = pGpio[count].gpioFreq;
						//printk("GPIO %d, mask %d, gpioTimes %d\n", pGpio[count].gpioId, mask, pGpio[count].gpioTimes);
						if (c2_led_enable)
						{
							setGpioData(pGpio[count].gpioId, (pGpio[count].gpioStatus == GPIO_ACT_ON ? GPIO_PHY_ON: GPIO_PHY_OFF));
						}
						pGpio[count].gpioStatus = (pGpio[count].gpioStatus == GPIO_ACT_ON ? GPIO_ACT_OFF : GPIO_ACT_ON);
					}
					pGpio[count].gpioTimes--;
				}
				break;
			default:
				/* Turn Off */
				if (c2_led_enable)
				{
					setGpioData(pGpio[count].gpioId, GPIO_PHY_OFF);
				}
				pGpio[count].gpioStatus = GPIO_ACT_OFF;
				break;
			}
		}
	}

#if INCLUDE_SINGLE_LED
	if (c2_single_led.stateValidTimes > 0 && --c2_single_led.stateValidTimes == 0)
	{
		//printk("c2_single_led.stateValidTimes=%d\n",c2_single_led.stateValidTimes);
		GET_CURR_STATE(c2_single_led.stateFlag, currState);
		led_singleControl(currState,STATE_FLAG_INVALID);
	}
#endif

	/* Part 2, Do GPIO Cycle Function */
	if (!connCycCount)
	{
		connCycCount = CONN_CYCLE_TRIGGER;
		//initGpioMode_W8();
#if INCLUDE_LAN_BIT_LED
		/* LAN Led Polling */
		if (NULL != fp_gpioTestLan)
		{
			lanBits = fp_gpioTestLan();
			led_setLanBits(lanBits);
		}

		/* WAN Led Polling */
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
		{
			led_setWanOn(0);
		}
		else /* No func or test failed */
		{
			led_setWanOff(0);
		}
#else
		/* LAN Led Polling */
		if ((NULL != fp_gpioTestLan) && (1 == fp_gpioTestLan())
#if defined(INCLUDE_SYS_MODE_PROC) && defined(INTERNET_MODE_LTE) && !defined(CONFIG_TP_MODEL_MB110V1)
			&& internet_mode == INTERNET_MODE_LTE
#endif
			)
		{
			led_setLanOn();

		}
#if defined(INCLUDE_SYS_MODE_PROC) && defined(INTERNET_MODE_ETH) && !defined(CONFIG_TP_MODEL_MB110V1)
		else if ((NULL != fp_gpioTestLanOffWan) && (1 == fp_gpioTestLanOffWan())
			&& internet_mode == INTERNET_MODE_ETH)
		{
			led_setLanOn();
		}
#endif
		else /* No func or test failed */
		{
			led_setLanOff();
		}

		/* WAN Led Polling */
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan())
#ifdef INCLUDE_SYS_MODE_PROC
			&& internet_mode == INTERNET_MODE_ETH
#endif
			)
		{
			if (wan_status)
			{
				led_setWanOn(1);	/* connected and online */
			}
			else
			{
				led_setWanOn(2);	/* connected but offline */
			}
		}
#ifdef INTERNET_MODE_USB3G
		else if (internet_mode == INTERNET_MODE_USB3G && usb_modem_link == USB_MODEM_LINK)
		{
			led_setWanOn(0);
		}
#endif
#ifdef INTERNET_MODE_WISP
		else if (internet_mode == INTERNET_MODE_WISP)
		{
			//led_setWanOn(0);
		}
#endif
#ifdef INTERNET_MODE_LTE
		else if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()) && internet_mode == INTERNET_MODE_LTE)
		{
			led_setWanOn(1);	/* MB110V1 WAN means LAN2, set it on */
		}
#endif
		else /* No func or test failed */
		{
			led_setWanOff(0);
		}
#endif
	}

	if (!btnCycCount)
	{
		btnCycCount = BTN_CYCLE_TRIGGER;
#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1)  || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
#define RESET_WAIT_TIME					(32*2)
		if (lte_reset_wait != 0)
		{
			lte_reset_wait++;

			if (lte_reset_wait > RESET_WAIT_TIME)
			{
				INIT_WORK(&reset_event_work, (work_func_t)spi_flash_erase_config);
				schedule_work(&reset_event_work);
				lte_reset_wait = 0;
			}
			return;
		}
#endif
		
		/* Reset/WPS Button */
		if (0 == getGpioData(GPIO_BTN_WPS_RST, &buttonStat))
		{
            if (gpio_mode == GPIO_MODE_FACTORY)
            {
                if (1 == buttonStat)
                {
                    if (btnUnprCount)
                    {
                        btnUnprCount--;
                        //printk("factory mode, after pressed btnUnprCount = %d", btnUnprCount);
                        if (GET_GPIO_STATUS(gpio_status,GPIO_WPS) == 0)
                        {
                            SET_GPIO_STATUS(gpio_status,GPIO_WPS);
                            SET_GPIO_STATUS(gpio_status,GPIO_RESET);
                        }
                    }
                    if (!btnUnprCount && GET_GPIO_STATUS(gpio_status,GPIO_WPS))
                    {
                        CLEAR_GPIO_STATUS(gpio_status,GPIO_WPS);
                        CLEAR_GPIO_STATUS(gpio_status,GPIO_RESET);
                    }
                }
                else
                {
                    btnUnprCount = 20;
                }
            }
            else
            {
                /*printk("resetCount %d, isReset %d\n", resetCount, isReset);*/
                if (1 == buttonStat) /* ??Unpressed */
                {
                    /* clean Button stat */
                    if (isReset == 1)
                    {
                        resetCount = 0;
                        isReset = 0;
                    }

                    if ((resetCount != 0) && (isReset == 0))
                    {
                        resetCount = 0;
                        /* Do WPS */
                        printk("Call WPS now\n");
                        if (registered_cb)
                        {
                            printk("wps 2.4G begin\n");
                            registered_cb (0, cb_arg, NULL);
                        }
                        else
                        {
                            printk("register 2.4G func is NULL\n");
                        }
                        if (registered_cb_5G)
                        {
                            printk("wps 5G begin\n");
                            registered_cb_5G (0, cb_arg_5G, NULL);
                        }
                        else
                        {
                            printk("register 5G func is NULL\n");
                        }

                    }

                }
                else /* Pressed */
                {
                    if (resetCount >= RESET_TRIGGER)
                    {
                        c2_led_enable = GPIO_ACT_ON;
                        /* Do Reset */
                        if (0 == isReset)/* Avoid more times */
                        {
                            printk("Factory configuration restored..\n");
#if INCLUDE_SINGLE_LED
                            led_singleControl(SINGLE_LED_STATE_RESET, STATE_FLAG_VALID);
#endif
#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1) || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
                            lte_reset_wait = 1;  // tell userspace to rm sms.db
                            led_setSysFlash();/*flash*/
                            sys_status = GPIO_ACT_BLINK;
                            sendResetButtonPressed();
#else
                            INIT_WORK(&reset_event_work, (work_func_t)spi_flash_erase_config);
                            schedule_work(&reset_event_work);
#endif

                            isReset = 1;
                        }

                    }
                    else
                    {
                        printk("resetCount ++ %d.\n", resetCount);
                        resetCount++;
                    }
                }
			}
		}
#ifdef GPIO_BTN_WIFI
		/* Wlan Button */
		if (0 == getGpioData(GPIO_BTN_WIFI, &buttonStat))
		{
            if (gpio_mode == GPIO_MODE_FACTORY)
            {
                if (1 == buttonStat)
                {
                    if (GET_GPIO_STATUS(gpio_status,GPIO_WIFI))
                    {
                        CLEAR_GPIO_STATUS(gpio_status,GPIO_WIFI);
                    }
                }
                else
                {
                    if (GET_GPIO_STATUS(gpio_status,GPIO_WIFI) == 0)
                    {
                        SET_GPIO_STATUS(gpio_status,GPIO_WIFI);
                    }
                }
            }
            else
            {
                if (1 == isWlan)
                {
                    if (0 == wlanCount)
                    {
                        printk("Switch Wlan Button locked\n");
                    }
                    wlanCount++;
                    if(wlan_led_status == 1 || wlanCount >= WLAN_LOCK_TIMES)
                    {
                        isWlan = 0;
                        wlanCount = 0;
                        wlan_led_status =1;
                        printk("Switch Wlan Button unlock\n");
                    }
                }
                else
                {
                    if (1 == buttonStat) /* ??Unpressed */
                    {
                        if (wlanCount > WLAN_TRIGGER)
                        {
                            c2_led_enable = GPIO_ACT_ON;
                            //if (wlan_button_on)
                            {
                                //wlan_button_on(wlan_radio_dev);
                                char buf[] = "WLAN SWITCH";
                                int len = sizeof(buf);
                                printk("Switch Wlan up now\n");
                                printk("%s\n", buf);
                                send_wlanSwitch_to_user(buf, len);
                            }

                            wlanCount = 0;
                            isWlan = 1;
                            wlan_led_status = 0;
                        }
                        else
                        {
    #if defined(CONFIG_TP_MODEL_WR845NV3) /* WiFi && WPS button */
                            if ((wlanCount != 0) && (isWlan == 0))
                            {
                                /* Do WPS */
                                printk("Call WPS now\n");
                                if (registered_cb)
                                {
                                    printk("wps 2.4G begin\n");
                                    registered_cb (0, cb_arg, NULL);
                                }
                                else
                                {
                                    printk("register 2.4G func is NULL\n");
                                }
                                if (registered_cb_5G)
                                {
                                    printk("wps 5G begin\n");
                                    registered_cb_5G (0, cb_arg_5G, NULL);
                                }
                                else
                                {
                                    printk("register 5G func is NULL\n");
                                }
                            }
#endif
                            /* Should Re-count */
                            wlanCount = 0;
                        }
                    }
                    else /* Pressed */
                    {
                        wlanCount++;
                    }
                }
			}
		}
#endif /* GPIO_BTN_WIFI */

#ifdef CONFIG_TP_MODEL_MR3020V3
		if ((0 == getGpioData(GPIO_BTN_MODE_C1, &buttonStat)) && (0 == getGpioData(GPIO_BTN_MODE_C2, &button2ndStat)))
		{
			c2_led_enable = GPIO_ACT_ON;
			if(sysMode == 0)
			{
				printk("#######GPIO_BTN_MODE_C1:%d , GPIO_BTN_MODE_C2:%d \n",buttonStat,button2ndStat);
				sysMode = (buttonStat << 1) | button2ndStat;
				hwsys_mode = (buttonStat << 1) | button2ndStat;
			}
			else if(((buttonStat << 1) | button2ndStat) != sysMode)
			{
				printk("#######GPIO_BTN_MODE_C1:%d , GPIO_BTN_MODE_C2:%d \n",buttonStat,button2ndStat);
				printk("####### Old Mode :%d , %s Mode\n",sysMode,sys_mode_str[sysMode-1]);
				sysMode = (buttonStat << 1) | button2ndStat;
				hwsys_mode = (buttonStat << 1) | button2ndStat;
				printk("####### New Mode :%d , %s Mode\n",sysMode,sys_mode_str[sysMode-1]);
				printk("####### HW Sys Mode changed , Rebooting......");
				machine_restart(NULL);
			}
		}
#endif
	}
}

#if defined(GPIO_LED_USB)
static int led_usb_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int led_usb_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16] = {0};
	int num = 0;
	int val = 0;
	
	if (count > sizeof(val_string) - 1)
	{
		printk("led_usb_write_proc count(%d) is larger than 16\n",count);
		return -EINVAL;
	}
	if (copy_from_user(val_string, buffer, count))
	{
		printk("led_usb_write_proc copy_from_user error\n");
		return -EFAULT;
	}

	if (sscanf(val_string, "%d %d",&num, &val) != 2)
	{
		printk("usage: <action>\n");
		return count;
	}
	//printk("led_usb_write num(%d), status(%d)\n",num,val);
	if (val == 1)
	{
		led_setUsbOn();
	}
	else if (val == 2)
	{
		led_setUsbBlink();
	}
	else
	{
		led_setUsbOff();
	}
	
	return count;
}
#endif

#ifdef GPIO_USB_POWER
static int usb_power_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int usb_power_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[8] = {0};
	int num = 0;
	int val = 0;

	if (count > sizeof(val_string) - 1)
	{
		printk("usb_power_write_proc count(%d) is larger than 8\n",count);
		return -EINVAL;
	}
	if (copy_from_user(val_string, buffer, count))
	{
		printk("usb_power_write_proc copy_from_user error\n");
		return -EFAULT;
	}

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

	if (val == 1)
	{
		flag_setUsbPowerOff();
		udelay (1000 * 100 * 10); /* 1s */
		flag_setUsbPowerOn();
	}
	else if (val == 0)
	{
		flag_setUsbPowerOff();
	}

	return count;
}
#endif /*GPIO_USB_POWER*/

#ifdef GPIO_USB_BOOT
static int usb_boot_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int usb_boot_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[8] = {0};
	int num = 0;
	int val = 0;

	if (count > sizeof(val_string) - 1)
	{
		printk("usb_boot_write_proc count(%d) is larger than 8\n",count);
		return -EINVAL;
	}
	if (copy_from_user(val_string, buffer, count))
	{
		printk("usb_boot_write_proc copy_from_user error\n");
		return -EFAULT;
	}

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

	if (val == 1)
	{
		flag_setUsbBootOn();
	}
	else if (val == 0)
	{
		flag_setUsbBootOff();
	}

	return count;
}
#endif /*GPIO_USB_BOOT*/
#ifdef GPIO_USB_DC
static int usb_dc_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int usb_dc_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[8] = {0};
	int num = 0;
	int val = 0;

	if (count > sizeof(val_string) - 1)
	{
		printk("usb_dc_write_proc count(%d) is larger than 8\n",count);
		return -EINVAL;
	}
	if (copy_from_user(val_string, buffer, count))
	{
		printk("usb_dc_write_proc copy_from_user error\n");
		return -EFAULT;
	}

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

	if (val == 1)
	{
		flag_setUsbDcOn();
	}
	else if (val == 0)
	{
		flag_setUsbDcOff();
	}

	return count;
}
#endif /*GPIO_USB_DC*/
#ifdef GPIO_USB_RESET
static int usb_reset_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int usb_reset_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[8] = {0};
	int num = 0;
	int val = 0;

	if (count > sizeof(val_string) - 1)
	{
		printk("usb_reset_write_proc count(%d) is larger than 8\n",count);
		return -EINVAL;
	}
	if (copy_from_user(val_string, buffer, count))
	{
		printk("usb_reset_write_proc copy_from_user error\n");
		return -EFAULT;
	}

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

	if (val == 1)
	{
		flag_setUsbResetOn();
	}
	else if (val == 0)
	{
		flag_setUsbResetOff();
	}

	return count;
}
#endif /*GPIO_USB_RESET*/

#if defined(CONFIG_TP_MODEL_MB110V1)
static int out_atn_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;
	//printk("out_atn_read_proc out_atn_main_enable=%d, out_atn_secd_enable=%d\n", out_atn_main_enable, out_atn_secd_enable);
	len += sprintf(page,"%d %d\n", out_atn_main_enable, out_atn_secd_enable);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}
static int out_atn_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;
	int val2 = 0;
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d %d", &val, &val2) != 2)
	{
		printk("usage: <action>\n");
		return count;
	}

	out_atn_main_enable = val;
	out_atn_secd_enable = val2;

	setGpioData(GPIO_OUT_ATN_MAIN, out_atn_main_enable);
	setGpioData(GPIO_OUT_ATN_SECD, out_atn_secd_enable);

	//printk("out_atn_write_proc out_atn_main_enable=%d, out_atn_secd_enable=%d\n", out_atn_main_enable, out_atn_secd_enable);

	return count;
}
#endif

/*add by wuzeyu for wan mode include eth and usb_3gmodem*/
#ifdef INCLUDE_SYS_MODE_PROC
#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1) || defined(CONFIG_TP_MODEL_MR100V2) || defined(CONFIG_TP_MODEL_MB110V1)
static int internet_mode_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", internet_mode);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}
static int internet_mode_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
    internet_mode = val;

	return count;
}
#else
static int internet_mode_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int internet_mode_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;
	//printk("wuzeyu internet_mode_write_proc internet_mode(%d).\n",internet_mode);
	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
    internet_mode = val;
	//printk("wuzeyu internet_mode_write_proc internet_mode(%d).\n",internet_mode);
	return count;
}

static int usb_modem_link_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	return count;
}
static int usb_modem_link_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;
	printk("wuzeyu internet_mode_write_proc val_string(%d),internet_mode(%d).\n",
		val_string,usb_modem_link);
	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
    usb_modem_link = val;
	printk("wuzeyu internet_mode_write_proc internet_mode(%d).\n",
	usb_modem_link);
	return count;
}
#endif
#endif
/*end add*/
static int led_internet_read_proc(char *page, char **start, off_t off,	
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", wan_status);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;

}
static int led_internet_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

    wan_status = val;
	printk("wuzeyu led_internet_write_proc wan_status(%d),internet_mode(%d).\n", wan_status, internet_mode);

	return count;
}


static int led_wlan_24G_read_proc(char *page, char **start, off_t off,	
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", wlan_24G_status);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int led_wlan_24G_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || (val != GPIO_ACT_ON && val != GPIO_ACT_OFF))
	{
		printk("usage: <action>[0:off 1:on]\n");
		return count;
	}

	wlan_24G_status = val;
	if (val)
	{
		led_setWlanOn();
	}
	else if(wlan_5G_status == GPIO_ACT_OFF)
	{
		led_setWlanOff();
	}
	
	return count;
}

static int led_wlan_5G_read_proc(char *page, char **start, off_t off,	
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", wlan_5G_status);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int led_wlan_5G_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || (val != GPIO_ACT_ON && val != GPIO_ACT_OFF))
	{
		printk("usage: <action>[0:off 1:on]\n");
		return count;
	}

	wlan_5G_status = val;
	if (val)
	{
		led_setWlan5gOn();
	}
	else if (wlan_24G_status == GPIO_ACT_OFF)
	{
		led_setWlan5gOff();
	}

	return count;
}


static int led_wlan_status_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
    return wlan_led_status;
}

static int led_wlan_status_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || (val != 0 && val != 1))
	{
		printk("usage: <action>[0:start 1:end]\n");
		return count;
	}

	wlan_led_status = val;

	return count;
}

static int led_sys_read_proc(char *page, char **start, off_t off,	
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", sys_status);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int led_sys_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{

	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || (val != GPIO_ACT_ON && val != GPIO_ACT_OFF && val != GPIO_ACT_BLINK))
	{
		printk("usage: <action>\n");
		return count;
	}

	sys_status = val;
	if (sys_status == GPIO_ACT_OFF)
	{
		led_setSysOff();
	}
	else if (sys_status == GPIO_ACT_ON)
	{
		led_setSysOn();
	}
	else
	{
		led_setSysFlash();/*flash*/
	}

	return count;
}

/* added by zc in 2017 */
static int gpio_mode_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", gpio_mode);
	len -= off;
	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;

    return len;
}
static int gpio_mode_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{

	char val_string[16];
	int val;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: 0:gpio normal mode, 1:gpio factory mode\n");
		return count;
	}

	gpio_mode = val ? GPIO_MODE_FACTORY : GPIO_MODE_NORMAL;

	return count;
}

static int gpio_status_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", gpio_status);
	len -= off;
	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;

    return len;
}
static int gpio_status_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{

	char val_string[16];
	int val;

	return count;
}
/* end added */

#if 0
static int lte_reset_read_proc(char *page, char **start, off_t off,
	                         int count, int *eof, void *data)
{
    int len;

	len = sprintf(page, "%d\n", lte_reset_wait);
	len -= off;
	*start = page + off;

	if (len > count)
		len = count;
	else
		*eof = 1;
	if (len < 0)
		len = 0;

	return len;
}
static int lte_reset_write_proc(struct file *file, const char *buffer,
	                          unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if (sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    if (1 == val)
    {
        lte_rebootFlag = 1;
    }
    else if (2 == val)
    {
        lte_resetFlag = 1;
    }
    else if (3 == val)
    {
        /* setGpioData(LTE_REBOOT_GPIO, 0);*//* power down LTE module */
    }

    return count;
}
#endif

#if  defined (CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1)
static int lte_recovering_read_proc(char *page, char **start, off_t off,
	                         int count, int *eof, void *data)
{
    int len;

	len = sprintf(page, "%d\n", lte_recovering);
	len -= off;
	*start = page + off;

	if (len > count)
		len = count;
	else
		*eof = 1;
	if (len < 0)
		len = 0;

	return len;
}
static int lte_recovering_write_proc(struct file *file, const char *buffer,
	                          unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if (sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    lte_recovering = val;


    return count;
}
#endif

static int led_control_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;
#if INCLUDE_SINGLE_LED
	int currState = 0;
	GET_CURR_STATE(c2_single_led.stateFlag, currState);
	len += sprintf(page,"%d %d\n", currState, STATE_FLAG_VALID);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
#endif
    return len;
}

static int led_control_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
#if INCLUDE_SINGLE_LED
	int val_state = 0;
	int val_valid = 0;
#endif
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

#if INCLUDE_SINGLE_LED
	if (sscanf(val_string, "%d %d", &val_state, &val_valid) != 2)
	{
		printk("usage: [state<1~6> valid<0:invalid 1:valid>]\n");
		return count;
	}
	if (val_state == SINGLE_LED_STATE_UNKOWN || val_state >= SINGLE_LED_STATE_MAX )
		val_state = SINGLE_LED_STATE_NORMAL;
	if (val_valid != STATE_FLAG_INVALID && val_valid != STATE_FLAG_VALID)
		val_valid = STATE_FLAG_VALID;
	led_singleControl(val_state, val_valid);
#endif

	return count;

}

static int led_lte_read_proc(char *page, char **start, off_t off,
	                         int count, int *eof, void *data)
{
    int len = 0;

    return len;
}
static int led_lte_write_proc(struct file *file, const char *buffer,
	                          unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if (sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    return count;
}


static int led_signal_strength_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", signal_strength);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;

}

static int led_signal_strength_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || val < SIGNAL_S0 || val > SIGNAL_S3)
	{
		printk("usage: [0:no signal, 1:[0,50)%, 2:[50,75)%, 3:[75,100]%]\n");
		return count;
	}

	signal_strength = val;
	led_setSignalStrength(val);

	return count;
}

static int led_enable_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", c2_led_enable);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int led_enable_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || (val != GPIO_ACT_ON && val != GPIO_ACT_OFF))
	{
		printk("usage: [0:turn off, 1:turn on]\n");
		return count;
	}

	c2_led_enable = val;
	setLedState(val);

	return count;

}
#if !defined(CONFIG_TP_MODEL_MR100V1) && !defined(CONFIG_TP_MODEL_MR100V2) && !defined(CONFIG_TP_MODEL_MB110V1)
static int led_mr200_version_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", flash_version);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int led_mr200_version_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	return count;
}
#endif
static int led_model_version_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;
#ifdef GPIO_MODEL_VER
	int model_ver = 0;
	getGpioData(GPIO_MODEL_VER, &model_ver);
	len += sprintf(page,"%d\n", model_ver);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
#endif
    return len;
}

static int led_model_version_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	return count;
}


static int led_setGpio_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

    return len;
}

static int led_setGpio_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	u32 gpio_id = 0;
	u32 gpio_val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d %d", &gpio_id, &gpio_val) > 1)
	{
		setGpioData(gpio_id, gpio_val);
		printk("set Gpio %d val %d\n", gpio_id, gpio_val);
	}

	return count;

}

static int led_getGpio_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

    return len;
}

static int led_getGpio_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	u32 gpio_id = 0;
	u32 gpio_val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &gpio_id) > 0)
	{
		getGpioData(gpio_id, &gpio_val);
		printk("===========>Gpio %d val %d\n", gpio_id, gpio_val);
	}

	return count;

}



/*added by xieping for mr3020v3*/
#ifdef CONFIG_TP_MODEL_MR3020V3
static int hwsys_mode_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;
	len += sprintf(page,"%d\n",hwsys_mode);

	len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
	return len;
}
static int hwsys_mode_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	return count;
}
#endif

static int rtnetlink_fill_info(struct sk_buff *skb, int type, char *data, int data_len)
{	
	struct ifinfomsg *r;	
	struct nlmsghdr  *nlh;
	unsigned char	 *b = skb_tail_pointer(skb);
	nlh = NLMSG_PUT(skb, 0, 0, type, sizeof(*r));
	r = NLMSG_DATA(nlh);	
	r->ifi_family = AF_UNSPEC;	
	r->__ifi_pad = 0;	
	r->ifi_type = 0;	
	r->ifi_index = 0;	
	r->ifi_flags = 0;	
	r->ifi_change = 0;	
	/* Wireless changes don't affect those flags */	
	/* Add the wireless events in the netlink packet */	
	RTA_PUT(skb, IFLA_WIRELESS, data_len, data);	
	nlh->nlmsg_len = skb_tail_pointer(skb) - b;	
	return skb->len;
	nlmsg_failure:
	rtattr_failure:	
	//nlmsg_trim(skb, b);
	return -1;
}

#ifndef BUF_SIZE_RTNL
#define BUF_SIZE_RTNL 	256
#endif
void send_wlanSwitch_to_user(char *buf, int len)
{
	unsigned int size = BUF_SIZE_RTNL;
	//int ret = 0;
	struct net *net = NULL;
	struct sk_buff *skb = alloc_skb(size, GFP_ATOMIC);

	if (skb == NULL)
	{
		printk("no enough memory!\n");
		return;
	}

	if (rtnetlink_fill_info(skb, RTM_NEWLINK,
				  buf, len) < 0) 
	{
		printk("fill reset info error!\n");
		kfree_skb(skb);
		return;
	}
	net = dev_net(skb->dev);
	rtnl_notify(skb, net, 0, RTNLGRP_LINK, NULL, GFP_ATOMIC);
	/*ret = rtnl_notify(skb, 0, RTNLGRP_LINK, NULL, GFP_ATOMIC);
	if (ret)
	{
		printk("Err to send\n");
	}*/
	return;
	
}
void sendResetButtonPressed(void)
{
	char buf[] = "RESET BUTTON PRESSED";
	int len = sizeof(buf);
	unsigned int size = BUF_SIZE_RTNL;
	struct net *net = NULL;
	struct sk_buff *skb = alloc_skb(size, GFP_ATOMIC);

	if (skb == NULL)
	{
		printk("no enough memory!\n");
		return;
	}

	if (rtnetlink_fill_info(skb, RTM_NEWLINK,
				  buf, len) < 0)
	{
		printk("fill reset info error!\n");
		kfree_skb(skb);
		return;
	}
	net = dev_net(skb->dev);
	rtnl_notify(skb, net, 0, RTNLGRP_LINK, NULL, GFP_ATOMIC);

}

#ifdef CONFIG_TP_MODEL_MR3020V3
static int apcli_status_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;

	len = sprintf(page, "%d\n", apcli_status);
    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}
static int apcli_status_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	printk("/proc/tplink/apcli_status is read only\n");
	return count;
}
#endif

static int sd_kicking_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page,"%d\n", sd_kicking);
	    len -= off;	*start = page + off;
    if (len > count)
        len = count;
    else
        *eof = 1;
    if (len < 0)
        len = 0;
    return len;
}

static int sd_kicking_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[16];
	int val = 0;

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1 || (val != 0 && val != 1))
	{
		printk("usage: [0:turn off, 1:turn on]\n");
		return count;
	}

	sd_kicking = val;

	return count;

}



void init_gpio_tplink(void)
{
	struct proc_dir_entry *led_proc;
#if 0	
	if (simple_config_entry != NULL) 
	{
		printk("Already have a proc entry for /proc/simple_config!\n");
		return;
	}

	simple_config_entry = proc_mkdir("tplink", NULL);
	if (!simple_config_entry)
	{
		return;
	}
#endif

#if defined(GPIO_LED_USB)
	led_proc = create_proc_entry(PROC_FILE_USB_LED, 0, NULL);
	led_proc->read_proc = led_usb_read_proc;
	led_proc->write_proc = led_usb_write_proc;
#endif

#if INCLUDE_SYS_MODE_PROC
	led_proc = create_proc_entry(PROC_FILE_INTERNET_MODE, 0, NULL);
	led_proc->read_proc = internet_mode_read_proc;
	led_proc->write_proc = internet_mode_write_proc;
#endif

#if INTERNET_MODE_USB3G
	led_proc = create_proc_entry(PROC_FILE_MODEM_LINK, 0, NULL);
	led_proc->read_proc = usb_modem_link_read_proc;
	led_proc->write_proc = usb_modem_link_write_proc;
#endif

#ifdef GPIO_USB_POWER
	led_proc = create_proc_entry(PROC_FILE_USB_POWER, 0, NULL);
    led_proc->read_proc = usb_power_read_proc;
    led_proc->write_proc = usb_power_write_proc;
#endif

#ifdef GPIO_USB_BOOT
	led_proc = create_proc_entry(PROC_FILE_USB_BOOT, 0, NULL);
    led_proc->read_proc = usb_boot_read_proc;
    led_proc->write_proc = usb_boot_write_proc;
#endif
#ifdef GPIO_USB_DC
	led_proc = create_proc_entry(PROC_FILE_USB_DC, 0, NULL);
    led_proc->read_proc = usb_dc_read_proc;
    led_proc->write_proc = usb_dc_write_proc;
#endif
#ifdef GPIO_USB_RESET
	led_proc = create_proc_entry(PROC_FILE_USB_RESET, 0, NULL);
    led_proc->read_proc = usb_reset_read_proc;
    led_proc->write_proc = usb_reset_write_proc;
#endif

#if defined(CONFIG_TP_MODEL_MB110V1)
	led_proc = create_proc_entry(PROC_FILE_OUT_ATN, 0, NULL);
    led_proc->read_proc = out_atn_read_proc;
    led_proc->write_proc = out_atn_write_proc;
#endif
	led_proc = create_proc_entry(PROC_FILE_CONTROL_LED, 0, NULL);
	led_proc->read_proc = led_control_read_proc;
	led_proc->write_proc = led_control_write_proc;

	led_proc = create_proc_entry(PROC_FILE_ENABLE_LED,	0, NULL);
	led_proc->read_proc = led_enable_read_proc;
	led_proc->write_proc = led_enable_write_proc;

	led_proc = create_proc_entry(PROC_FILE_LTE_LED, 0, NULL);
	led_proc->read_proc = led_lte_read_proc;
	led_proc->write_proc = led_lte_write_proc;

	led_proc = create_proc_entry(PROC_FILE_SIGNAL_STRENGTH_LED,	0, NULL);
	led_proc->read_proc = led_signal_strength_read_proc;
	led_proc->write_proc = led_signal_strength_write_proc;

	led_proc = create_proc_entry(PROC_FILE_INTERNET_LED,  0, NULL);
	led_proc->read_proc = led_internet_read_proc;
	led_proc->write_proc = led_internet_write_proc;

	led_proc = create_proc_entry(PROC_FILE_WLAN24G_LED,  0, NULL);
	led_proc->read_proc = led_wlan_24G_read_proc;
	led_proc->write_proc = led_wlan_24G_write_proc;

	led_proc = create_proc_entry(PROC_FILE_WLAN5G_LED, 0, NULL);
	led_proc->read_proc = led_wlan_5G_read_proc;
	led_proc->write_proc = led_wlan_5G_write_proc;
	

	led_proc = create_proc_entry(PROC_FILE_WLAN_LED_STATUS, 0, NULL);
	led_proc->read_proc = led_wlan_status_read_proc;
	led_proc->write_proc = led_wlan_status_write_proc;

	led_proc = create_proc_entry(PROC_FILE_POWER_LED,  0, NULL);
	led_proc->read_proc = led_sys_read_proc;
	led_proc->write_proc = led_sys_write_proc;

#if defined (CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5) || defined(CONFIG_TP_MODEL_MR100V1)
	led_proc = create_proc_entry("tplink/mr200_version", 0, NULL);
	led_proc->read_proc = led_mr200_version_read_proc;
	led_proc->write_proc = led_mr200_version_write_proc;

	led_proc = create_proc_entry("tplink/model_version", 0, NULL);
	led_proc->read_proc = led_model_version_read_proc;
	led_proc->write_proc = led_model_version_write_proc;
#if 0
	led_proc = create_proc_entry("tplink/getGpio", 0, NULL);
	led_proc->read_proc = led_getGpio_read_proc;
	led_proc->write_proc = led_getGpio_write_proc;

	led_proc = create_proc_entry("tplink/setGpio", 0, NULL);
	led_proc->read_proc = led_setGpio_read_proc;
	led_proc->write_proc = led_setGpio_write_proc;
#endif
	led_proc = create_proc_entry("tplink/lte_recovering", 0, NULL);
	led_proc->read_proc = lte_recovering_read_proc;
	led_proc->write_proc = lte_recovering_write_proc;

	led_proc = create_proc_entry("tplink/lte_sd_kicking",	0, NULL);
	led_proc->read_proc = sd_kicking_read_proc;
	led_proc->write_proc = sd_kicking_write_proc;
#endif

#if 0
	led_proc = create_proc_entry("tplink/lte_reset", 0, NULL);
	led_proc->read_proc = lte_reset_read_proc;
	led_proc->write_proc = lte_reset_write_proc;
#endif

#ifdef CONFIG_TP_MODEL_MR3020V3
	led_proc = create_proc_entry(PROC_FILE_HWSYS_MODE,  0, NULL);
	led_proc->read_proc = hwsys_mode_read_proc;
	led_proc->write_proc = hwsys_mode_write_proc;

	led_proc = create_proc_entry(PROC_FILE_APCLI_STATUS, 0, NULL);
	led_proc->read_proc = apcli_status_read_proc;
	led_proc->write_proc = apcli_status_write_proc;
#endif
	led_proc = create_proc_entry(PROC_FILE_GPIO_MODE,  0, NULL);
	led_proc->read_proc = gpio_mode_read_proc;
	led_proc->write_proc = gpio_mode_write_proc;

	led_proc = create_proc_entry(PROC_FILE_GPIO_STATUS,  0, NULL);
	led_proc->read_proc = gpio_status_read_proc;
	led_proc->write_proc = gpio_status_write_proc;




#if defined(CONFIG_TP_MODEL_C50V4) || defined(CONFIG_TP_MODEL_C20V4)
	gpio_common_init();
#else
#if defined(CONFIG_TP_MODEL_MR6400V3) || defined (CONFIG_TP_MODEL_MR400V4) || defined (CONFIG_TP_MODEL_MR6400V5) || defined (CONFIG_TP_MODEL_MR200V5)
	initGpioPins();
#endif
	initGpioMode_W8();
	initGpioDir_W8(1);
	initLedData_W8();
#endif

#if INCLUDE_SINGLE_LED
	led_singleControl(SINGLE_LED_STATE_INIT,STATE_FLAG_VALID);
	led_singleControl(SINGLE_LED_STATE_NO_WAN_CONN,STATE_FLAG_VALID);
	led_singleControl(SINGLE_LED_STATE_NORMAL,STATE_FLAG_VALID);
#else
	if (sys_status == 0)
	{
		sys_wlan_state = STATE_SYS_ON_WIFI_WPS;
		setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_SLOW, GPIO_LED_WPS); /* MB110V1 SYS_GREEN(WPS) blink slow when booting */
	}

#endif
}


int __init ralink_gpio_init(void)
{
	unsigned int i;
#if 0
	u32 gpiomode;
#endif


#ifdef  CONFIG_DEVFS_FS
	if (devfs_register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
				&ralink_gpio_fops)) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return -EIO;
	}
	devfs_handle = devfs_register(NULL, RALINK_GPIO_DEVNAME,
			DEVFS_FL_DEFAULT, ralink_gpio_major, 0,
			S_IFCHR | S_IRUGO | S_IWUGO, &ralink_gpio_fops, NULL);
#else
	int r = 0;
	r = register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
			&ralink_gpio_fops);
	if (r < 0) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return r;
	}
	if (ralink_gpio_major == 0) {
		ralink_gpio_major = r;
		printk(KERN_DEBUG NAME ": got dynamic major %d\n", r);
	}
#endif

#if 0
	//config these pins to gpio mode
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
#if !defined (CONFIG_RALINK_RT2880)
	gpiomode &= ~0x1C;  //clear bit[2:4]UARTF_SHARE_MODE
#endif
#if defined (CONFIG_RALINK_MT7620)
	gpiomode &= ~0x2000;  //clear bit[13] WLAN_LED
#endif
	gpiomode |= RALINK_GPIOMODE_DFT;
	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);
#else
	init_gpio_tplink();
#endif

	//enable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_info[i].irq = i;
		ralink_gpio_info[i].pid = 0;
	}

#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_init_timer();
#endif
	printk("Ralink gpio driver initialized\n");

	return 0;
}

void __exit ralink_gpio_exit(void)
{
#ifdef  CONFIG_DEVFS_FS
	devfs_unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
	devfs_unregister(devfs_handle);
#else
	unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
#endif

	//config these pins to normal mode
	*(volatile u32 *)(RALINK_REG_GPIOMODE) &= ~RALINK_GPIOMODE_DFT;
	//disable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
#ifdef CONFIG_RALINK_GPIO_LED
	del_timer(&ralink_gpio_led_timer);
#endif
	printk("Ralink gpio driver exited\n");
}

/*
 * send a signal(SIGUSR1) to the registered user process whenever any gpio
 * interrupt comes
 * (called by interrupt handler)
 */
void ralink_gpio_notify_user(int usr)
{
	struct task_struct *p = NULL;

	if (ralink_gpio_irqnum < 0 || RALINK_GPIO_NUMBER <= ralink_gpio_irqnum) {
		printk(KERN_ERR NAME ": gpio irq number out of range\n");
		return;
	}

	//don't send any signal if pid is 0 or 1
	if ((int)ralink_gpio_info[ralink_gpio_irqnum].pid < 2)
		return;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	p = find_task_by_vpid(ralink_gpio_info[ralink_gpio_irqnum].pid);
#else
	p = find_task_by_pid(ralink_gpio_info[ralink_gpio_irqnum].pid);
#endif

	if (NULL == p) {
		printk(KERN_ERR NAME ": no registered process to notify\n");
		return;
	}

	if (usr == 1) {
		printk(KERN_NOTICE NAME ": sending a SIGUSR1 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		send_sig(SIGUSR1, p, 0);
	}
	else if (usr == 2) {
		printk(KERN_NOTICE NAME ": sending a SIGUSR2 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		send_sig(SIGUSR2, p, 0);
	}
}

/*
 * 1. save the PIOINT and PIOEDGE value
 * 2. clear PIOINT by writing 1
 * (called by interrupt handler)
 */
void ralink_gpio_save_clear_intp(void)
{
	ralink_gpio_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOINT));
	ralink_gpio_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOEDGE));
	
#if defined (RALINK_GPIO_HAS_9532)	
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0xFFFFFFFF);
#else
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0x00FFFFFF);
#endif
#if defined (RALINK_GPIO_HAS_2722)
	ralink_gpio2722_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722INT));
	ralink_gpio2722_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722EDGE));
	*(volatile u32 *)(RALINK_REG_PIO2722INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO2722EDGE) = cpu_to_le32(0x0000FFFF);
#elif defined (RALINK_GPIO_HAS_4524)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio4540_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540INT));
	ralink_gpio4540_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540EDGE));
	*(volatile u32 *)(RALINK_REG_PIO4540INT) = cpu_to_le32(0x00000FFF);
	*(volatile u32 *)(RALINK_REG_PIO4540EDGE) = cpu_to_le32(0x00000FFF);
#elif defined (RALINK_GPIO_HAS_5124)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio5140_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140INT));
	ralink_gpio5140_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140EDGE));
	*(volatile u32 *)(RALINK_REG_PIO5140INT) = cpu_to_le32(0x00000FFF);
	*(volatile u32 *)(RALINK_REG_PIO5140EDGE) = cpu_to_le32(0x00000FFF);
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio7140_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140INT));
	ralink_gpio7140_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140EDGE));
	*(volatile u32 *)(RALINK_REG_PIO7140INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO7140EDGE) = cpu_to_le32(0xFFFFFFFF);
#if defined (RALINK_GPIO_HAS_7224)
	ralink_gpio72_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72INT));
	ralink_gpio72_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72EDGE));
	*(volatile u32 *)(RALINK_REG_PIO72INT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO72EDGE) = cpu_to_le32(0x00FFFFFF);
#else
	ralink_gpio9572_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572INT));
	ralink_gpio9572_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572EDGE));
	*(volatile u32 *)(RALINK_REG_PIO9572INT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO9572EDGE) = cpu_to_le32(0x00FFFFFF);
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ralink_gpio6332_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332INT));
	ralink_gpio6332_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332EDGE));
	*(volatile u32 *)(RALINK_REG_PIO6332INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO6332EDGE) = cpu_to_le32(0xFFFFFFFF);


	ralink_gpio9564_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564INT));
	ralink_gpio9564_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564EDGE));
	*(volatile u32 *)(RALINK_REG_PIO9564INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO9564EDGE) = cpu_to_le32(0xFFFFFFFF);

#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void ralink_gpio_irq_handler(unsigned int irq, struct irqaction *irqaction)
#else
irqreturn_t ralink_gpio_irq_handler(int irq, void *irqaction)
#endif
{
	struct gpio_time_record {
		unsigned long falling;
		unsigned long rising;
	};
	static struct gpio_time_record record[RALINK_GPIO_NUMBER];
	unsigned long now;
	int i;
	ralink_gpio_save_clear_intp();
	now = jiffies;
#if defined (RALINK_GPIO_HAS_2722)
	for (i = 0; i < 22; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			record[i].falling = now;
		}
		break;
	}
	for (i = 22; i < 28; i++) {
		if (! (ralink_gpio2722_intp & (1 << (i - 22))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio2722_edge & (1 << (i - 22))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_9532)
	for (i = 0; i < 32; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
			ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					printk("one click\n");
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					printk("press for several seconds\n");
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			 record[i].falling = now;
		}
		break;
	}
	for (i = 32; i < 64; i++) {
		if (! (ralink_gpio6332_intp & (1 << (i - 32))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio6332_edge & (1 << (i - 32))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 64; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio9564_intp & (1 << (i - 64))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio9564_edge & (1 << (i - 64))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#else
	for (i = 0; i < 24; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			record[i].falling = now;
		}
		break;
	}
#if defined (RALINK_GPIO_HAS_4524)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio4540_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio4540_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_5124)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio5140_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio5140_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < 72; i++) {
		if (! (ralink_gpio7140_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio7140_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#if defined (RALINK_GPIO_HAS_7224)
	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio72_intp & (1 << (i - 72))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio72_edge & (1 << (i - 72))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#else
	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio9572_intp & (1 << (i - 72))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio9572_edge & (1 << (i - 72))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#endif
#endif
#endif

	return IRQ_HANDLED;
}

struct irqaction ralink_gpio_irqaction = {
	.handler = ralink_gpio_irq_handler,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
	.flags = IRQF_DISABLED,
#else
	.flags = SA_INTERRUPT,
#endif
	.name = "ralink_gpio",
};

void __init ralink_gpio_init_irq(void)
{
	setup_irq(SURFBOARDINT_GPIO, &ralink_gpio_irqaction);
}

module_init(ralink_gpio_init);
module_exit(ralink_gpio_exit);

