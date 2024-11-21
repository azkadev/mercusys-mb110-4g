/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     27Sep13
 *
 *\warning
 *
 *\history \arg 0.0.1, 27Sep13, Yan Wei, Create the file.
 */
#ifndef __APPIDENTIFY_H__
#define __APPIDENTIFY_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_app.h>
#include <linux/skbuff.h>

#include "appidentify_id.h"
#include "appidentify_rules.h"
#include "appidentify_node.h"
#include "appidentify_utils.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
extern int appidntf_debug;

#define     APPIDNTF_ERROR(fmt, args...)  printk("ERROR[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args)

#define     APPIDNTF_DEBUG(fmt, args...)                                    \
            do                                                              \
            {                                                               \
                if (1 == appidntf_debug)                                    \
                {                                                           \
                    printk("DEBUG[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args);  \
                }                                                           \
            }while(0)

typedef int (*FP_HANDLE)(struct sk_buff *, struct nf_conntrack_app *,
                           void *, unsigned char *, unsigned int);

/*!
*\struct  _APPIDNTFY_PKT_TUPLE
*\brief   The tuple with four element.
*\details elements:source ip\source port\dest ip\dest port.
*/
typedef struct _APPIDNTFY_PKT_TUPLE
{
    unsigned int   srcip;                  /*! 源IP */
    unsigned short  srcport;                /*! 源端口 */
    unsigned int   dstip;                  /*! 目的IP */
    unsigned short  dstport;                /*! 目的端口 */
}APPIDNTFY_PKT_TUPLE;

/*!
*\typedef FP_HANDLE_INFO
*\brief   Hard code application struct.
*/
typedef struct _FP_HANDLE_INFO
{
    int                     appId;               /*!< application ID */
    APPIDNTFY_PKT_TUPLE     tuple;               /*!< 4-tuple */
    int                     proto;               /*!< protocol */
    FP_HANDLE               hard_code_ptr[IP_CT_DIR_MAX];       /*!< hard code method */
} FP_HANDLE_INFO;

/*!
*\typedef   APPIDNTFY_FEATURE
*\brief 连接的其它特征信息。
*/
typedef struct _APPIDNTFY_FEATURE
{
    int enable;         /*! 此特征码是否生效  */
    int pkt_start;      /*! 特征码起始的包, 此值若为-1时，表示特征码存在于中间某段
                        （pkt_end表示长度）数据包中  */
    int pkt_end;        /*! 特征码结束的包, 若（pkt_start）为-1时，表示特征码存在
                        于中间某段（pkt_end表示长度）数据包中 */
    unsigned short packetLen;      /*! 包负载的最小长度,不确定的时候,设置为0 */
    unsigned short inspLen;    /* new add for  */

    int (*fpHandle)       (struct sk_buff *, struct nf_conntrack_app *,
                           void *, unsigned char *, unsigned int);   /*!< 特征码处理函数 */
    int (*hardCodeHandle) (struct sk_buff *, struct nf_conntrack_app *,
                           void *, unsigned char *, unsigned int);    /*!< 硬编码特征处理 reserved */
    void                    *pDpiCode;
}APPIDNTFY_FEATURE;

/*!
*\struct  _APPIDNTFY_KER_RULE
*\brief   Application define by four elements.
*\details
*/
typedef struct _APPIDNTFY_KER_RULE
{
    int                         appId;                  /*!< 命令,设置的值需要来自ENUM_APP_ID中的定义 */
    int                         protocol;               /*!< 数据包的协议 */
    APPIDNTFY_PKT_TUPLE         tuple;                  /*!< 四元组信息 */
    unsigned short              mask;                   /*!< 生效的掩码 */
    unsigned short              count;                  /*!< 生效的个数 */
    APPIDNTFY_NODE              hashNode[APPIDNTFY_TUPLE_INDEX_MAX];/*!< 哈希列表的节点指针 */
    APPIDNTFY_FEATURE           feature[IP_CT_DIR_MAX];    /*!< 发起方向的其它特征  */
}APPIDNTFY_KER_RULE;
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
int  appidentify_dpi_init(void);
void appidentify_dpi_fini(void);



#endif  /* __APPIDENTIFY_H__ */
