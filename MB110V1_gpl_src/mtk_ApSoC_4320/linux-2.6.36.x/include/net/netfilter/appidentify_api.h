/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	appidentify_api.h
 * Version		:	1.0
 * Abstract		:
 * Author		:	Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date	:	2014-07-01
 *
 ***************************************************************/

#ifndef __APPIDENTIFY_API_H__
#define __APPIDENTIFY_API_H__

#include <linux/list.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)
#include <linux/printk.h>
#else
#include <linux/kernel.h>
#endif

#include <linux/ratelimit.h>

#define APPID_DEBUG_ON          // uncomment this to enable log
//#define LOG_RATELIMIT           // comment this to cancel log ratelimit

#define APPID_MODULE_NAME_MAX 16
#define APPID_MODULE_ID_MAX   16

/// module interfaces
struct appid_module
{
    char name[APPID_MODULE_NAME_MAX];
    unsigned int   (*hook) (unsigned int hooknum, struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));
    struct list_head list;
};

int  appid_register_module(char* name, struct appid_module* module);
int  appid_unregister_module(char* name);
int  appid_alloc_id(int prio);
void appid_free_id(int id);

/// hardcode id
#define APP_ID_SPF                17
#define APP_ID_QQ_GAME            18

/// id area
typedef enum _APPMNGR_APPID_AREA
{
    APPMNGR_APPID_VOID          = -1,
    APPMNGR_APPID_NORMAL_BEGIN  = 0,         /* 普通应用的 ID范围 0-889 */
    APPMNGR_APPID_NORMAL_END    = 3999,      /* 普通应用的终点 */

    APPMNGR_APPID_AUTO_BEGIN    = 2000,
    APPMNGR_APPID_AUTO_END      = 2255,      /* 该范围的 ID 保留用于自动分配，勿占用 */

    APPMNGR_APPID_PRI_BEGIN     = 3000,
    APPMNGR_APPID_CLSF_BEGIN    = 3500,

    APPMNGR_PROXY_NONE          = 4000,      /* 分割� 获取代理的ID�加上此�可获取相应的策略. */
    APPMNGR_PROXY_BEGIN         = 4001,      /* 代理起点 */
    APPMNGR_PROXY_END           = 4020,      /* 代理终点:�6�*/

    APPMNGR_APPID_SPECIAL_NONE  = 4020,      /* 分割� 特殊应用基本上全部为0,供识别内部使� */
    APPMNGR_APPID_SPECIAL_BEGIN = 4021,      /* 特殊应用 908-1007, �00�*/
    APPMNGR_APPID_SPECIAL_END   = 4070,      /* 4021-4070 50�*/

    APPMNGR_APPID_BASIC_NONE    = 4070,     /* 分割� 提取出高4位之�加上此值可得到基础应用的ID，若结果为APPMNGR_APPID_BASIC_NONE，则表示无基础应用 */
    APPMNGR_APPID_BASIC_BEGIN   = 4071,     /* 基础应用�ID范围 4071-4085, �5�*/
    APPMNGR_APPID_BASIC_END     = 4083,

}APPMNGR_APPID_AREA;

/// id  interfaces
#define APP_BASIC_ID_UNKNOWN            (short)0xF000
#define APP_BASIC_ID_CHECKING           (short)0xE000
#define APP_NORMAL_ID_UNKNOWN           (short)0x0FFF
#define APP_NORMAL_ID_CHECKING          (short)0x0FFE
#define APP_ID_CHECKING                 (short)(APP_BASIC_ID_CHECKING | APP_NORMAL_ID_CHECKING)

#define APP_BASIC_ID_FLAG_GET(appId)         (short)((appId)&0xF000)
#define APP_BASIC_ID_FLAG_SET(appId, value)  (appId) = (((value) & 0xF000) | ((appId) & 0x0FFF))
#define APP_BASIC_ID_VALUE_GET(appId)        (short)((((appId)&0xF000) >> 12) + APPMNGR_APPID_BASIC_NONE)
#define APP_BASIC_ID_VALUE_SET(appId, value) \
    (appId) = (((((value) - APPMNGR_APPID_BASIC_NONE) << 12) & 0xF000) | ((appId) & 0x0FFF))


#define APP_NORMAL_ID_GET(appId)             (short)((appId)&0x0FFF)
#define APP_NORMAL_ID_SET(appId, value)      (appId) = ((value) & 0x0FFF) | ((appId) & 0xF000)


/// prio interfaces
typedef enum _APPPRIO_PRIO
{
    APPPRIO_DEFAULT_PRIO = 0,
    APPPRIO_FIRST_PRIO,
    APPPRIO_SECOND_PRIO,
    APPPRIO_THIRD_PRIO,
    APPPRIO_FOURTH_PRIO,
    APPPRIO_PRIO_TOP
}APPPRIO_PRIO;

typedef enum _APPPRIO_FLAG
{
    APPPRIO_FLAG_UPNP            = 0x000001,
    APPPRIO_FLAG_DPI             = 0x000002,
    APPPRIO_FLAG_CLSF            = 0x000004,
    APPPRIO_FLAG_PORTDEF         = 0x000008,
    APPPRIO_FLAG_VPN             = 0x000010,
    APPPRIO_FLAG_DNS             = 0x000020,
    APPPRIO_FLAG_PORT_SET        = 0x000040,
    APPPRIO_FLAG_PORT_CNT        = 0x000080,
    APPPRIO_FLAG_PORT_CHECK      = 0x000100,
    APPPRIO_FLAG_SUBMOUDLES      = 0x000200,
    APPPRIO_FLAG_TOFROM_ROUTER   = 0x200000,
    APPPRIO_FLAG_PRI_SET         = 0x400000,
    APPPRIO_FLAG_TOP             = 0x800000
}APPPRIO_FLAG;

#define APPPRIO_FLAG_OFFSET                   (8)
#define APPPRIO_PRI_MASK                      (0xff)
#define APPPRIO_NAME_STR_LEN                  (32)
#define APPPRIO_CUSTOM_PORTS_MAX              (5)

#define APPPRIO_FLAG_SET(priFlag, value)      (priFlag = (priFlag | (value << APPPRIO_FLAG_OFFSET)))
#define APPPRIO_FLAG_CLEAR(priFlag, value)    (priFlag = (priFlag & ~(value << APPPRIO_FLAG_OFFSET)))
#define APPPRIO_FLAG_IS_SET(priFlag, value)   ((priFlag >> APPPRIO_FLAG_OFFSET) & value)
#define APPPRIO_FLAG_GET(priFlag)             ((priFlag & ~(APPPRIO_PRI_MASK)) >> APPPRIO_FLAG_OFFSET)

#define APPPRIO_PRI_GET(priFlag)              (priFlag & APPPRIO_PRI_MASK)
#define APPPRIO_PRI_SET(priFlag, value)       (priFlag = ((priFlag & ~(APPPRIO_PRI_MASK)) | value))

/// proc_path
extern struct proc_dir_entry *appid_proc_path;

/// log interface
#ifdef LOG_RATELIMIT
#define log_printk printk_ratelimited
#else
#define log_printk printk
#endif // LOG_RATELIMIT

#ifdef APPID_DEBUG_ON
#define APPID_LOG(module, fmt, ...) do {                            \
    if (_appid_submodule_debug_is_enabled(module)) {                \
        log_printk(KERN_DEBUG "APPID [%s](%s:%d): "fmt"\n",         \
                   module, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                               \
} while(0)
#else
#define APPID_LOG(module, fmt, ...)
#endif // APPID_DEBUG_ON

#define APPID_ERR(module, fmt, ...) do {                           \
    log_printk(KERN_ERR "APPID ERROR [%s](%s:%d) : "fmt"\n",       \
               module, __FUNCTION__, __LINE__, ##__VA_ARGS__);     \
} while(0)

int _appid_submodule_debug_is_enabled(char* module);

#endif // __APPIDENTIFY_API_H__
