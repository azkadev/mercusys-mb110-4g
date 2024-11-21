/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appprio.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     17Oct13
 *
 *\warning
 *
 *\history \arg 0.0.1, 17Oct13, Yan Wei, Create the file.
 */
#ifndef __APPPRIO_H__
#define __APPPRIO_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/list.h>


/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define     APPPRIO_CNT_DEBUG            (1)

#define     APPPRIO_ERROR(fmt, args...) printk("[appprio_error](%s) %d: "fmt"\r\n", __FUNCTION__, __LINE__, ##args)

extern int l_appprio_check_debug;
extern int g_count_debug;
extern int g_enablePrio;

#define     APPPRIO_DEBUG(fmt, args...)              \
            do                                      \
            {                                       \
                if (l_appprio_check_debug)           \
                {                                   \
                    printk("[appprio_check_debug](%s) %d: "fmt"\r\n", __FUNCTION__, __LINE__, ##args); \
                }                                   \
            }while(0)

#define     APPPRIO_FLAG_OFFSET          (8)
#define     APPPRIO_PRI_MASK             (0xff)
#define     APPPRIO_NAME_STR_LEN         (32)
#define     APPPRIO_CUSTOM_PORTS_MAX     (5)

#define     APPPRIO_FLAG_SET(priFlag, value)      (priFlag = (priFlag | (value << APPPRIO_FLAG_OFFSET)))
#define     APPPRIO_FLAG_CLEAR(priFlag, value)    (priFlag = (priFlag & ~(value << APPPRIO_FLAG_OFFSET)))
#define     APPPRIO_FLAG_IS_SET(priFlag, value)   ((priFlag >> APPPRIO_FLAG_OFFSET) & value)
#define     APPPRIO_FLAG_GET(priFlag)             ((priFlag & ~(APPPRIO_PRI_MASK)) >> APPPRIO_FLAG_OFFSET)

#define     APPPRIO_PRI_GET(priFlag)              (priFlag & APPPRIO_PRI_MASK)
#define     APPPRIO_PRI_SET(priFlag, value)       (priFlag = ((priFlag & ~(APPPRIO_PRI_MASK)) | value))


#if APPPRIO_CNT_DEBUG
#define     CT_TCP_STATISTICS_FIRST_INTERVAL      (g_appprio_first_interval)
#define     CT_TCP_STATISTICS_INTERVAL            (g_appprio_interval)
#define     APPPRIO_HTTP_DL_THRESHOLD             (g_appprio_dl_threshold)       /* 20KB/S */
#define     APPPRIO_CNT_THRESHOLD                 (g_appprio_cnt_threshold)
#define     APPPRIO_SUSPECT_THRESHOLD             (g_appprio_suspect_threshold)

#else

#define     CT_TCP_STATISTICS_FIRST_INTERVAL      (5.0f)
#define     CT_TCP_STATISTICS_INTERVAL            (1.0f)
#define     APPPRIO_HTTP_DL_THRESHOLD             (20.0f)       /* 20KB/S */
#define     APPPRIO_CNT_THRESHOLD                 (10 + 1)
#define     APPPRIO_SUSPECT_THRESHOLD             (APPPRIO_CNT_THRESHOLD / 2)
#endif

#define     APPPRIO_CNT_GET(short_cnt, offset)    ((short_cnt & (0xff << offset)) >> offset)
#define     APPPRIO_CNT_CLEAR(short_cnt, offset)  (short_cnt & ~(0xff << offset))
#define     APPPRIO_CNT_INCREASE(short_cnt, offset)  \
            short_cnt = (((APPPRIO_CNT_GET(short_cnt, offset) + 1) & 0xff) << offset) |   \
            APPPRIO_CNT_CLEAR(short_cnt, offset)

#define     APPPRIO_APP_MAP_SIZE         (4096)
#define     APPPRIO_PRIQ_NUM             (4)
#define     APPPRIO_PRIQ_POOL_SIZE       (16)
#define     APPPRIO_APP_ID_END           (APPPRIO_APP_MAP_SIZE - 1)
#define     APPPRIO_DFT_PRI_RANGE        (10)

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct _APPPRIO_PRIQ
{
    struct list_head  list;
    unsigned int      *pSort;
}APPPRIO_PRIQ;

typedef enum _APPPRIO_PRIO
{
    APPPRIO_DEFAULT_PRIO = 0,
    APPPRIO_FIRST_PRIO,
    APPPRIO_SECOND_PRIO,
    APPPRIO_THIRD_PRIO,
    APPPRIO_FOURTH_PRIO,
    APPPRIO_PRIO_TOP
}APPPRIO_PRIO;

/*
  appprio flag total 16 bits: (15 14 13 12 11 10 9 8 7 6)   (5 4 3 2 1 0)
                                        flag                    prio

  prio flag enhanced to 32 bits: (31 ~ 8 bit)      (7 ~ 0 bit)
                                    flag             prio
 */
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
    APPPRIO_FLAG_SUBMOUDLE       = 0x000200,

    APPPRIO_FLAG_TOFROM_ROUTER   = 0x200000,
    APPPRIO_FLAG_PRI_SET         = 0x400000,

    APPPRIO_FLAG_TOP             = 0x800000
}APPPRIO_FLAG;

typedef struct _APPPRIO_APP_MAP
{
    int     appId;
    char    name[APPPRIO_NAME_STR_LEN];
}APPPRIO_APP_MAP;

typedef struct _APPPRIO_PROFILE
{
    int             insCnt;
    unsigned int    appmapSize;
    unsigned int    dbMemSize;
    unsigned int    appprioRuleMaxNum;
    unsigned int    appprioCustomappMaxNum;
}APPPRIO_PROFILE;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
extern float   g_appprio_first_interval;
extern float   g_appprio_interval;
extern float   g_appprio_dl_threshold;
extern unsigned int  g_appprio_cnt_threshold;
extern unsigned int  g_appprio_suspect_threshold;
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
unsigned int appprio_hook(unsigned int hook,
                 struct sk_buff *skb,
                 const struct net_device *in,
                 const struct net_device *out,
                 int (*okfn)(struct sk_buff *));

int
appprio_init(void);

int
appprio_exit(void);

int
appprio_prioResetDefault(void);

#if  APPPRIO_CNT_DEBUG
void appprio_cnt_reset(void);
#endif

void
appprio_debug(void);




#endif  /* __APPPRIO_H__ */
