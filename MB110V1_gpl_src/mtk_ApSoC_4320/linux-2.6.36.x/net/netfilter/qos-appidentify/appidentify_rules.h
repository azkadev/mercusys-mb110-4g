/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_rules.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     27Sep13
 *
 *\warning
 *
 *\history \arg 0.0.1, 26Sep13, Yan Wei, Create the file.
 */
#ifndef __APPIDENTIFY_RULE_H__
#define __APPIDENTIFY_RULE_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define APPIDNTFY_MASK_NONE               0X0000              /*!  所有无效  */
#define APPIDNTFY_MASK_SRC_IP             0X0001              /*!  源IP  */
#define APPIDNTFY_MASK_SRC_PORT           0X0002              /*!  源端口  */
#define APPIDNTFY_MASK_DST_IP             0X0004              /*!  目的IP  */
#define APPIDNTFY_MASK_DST_PORT           0X0008              /*!  目的端口  */
#define APPIDNTFY_MASK_RANDOM_PORT        0X0010              /*!  不定端口  */
#define APPIDNTFY_MASK_BASIC_APP          0x0020              /*!  基础应用  */
#define APPIDNTFY_MASK_ALL                0X000F              /*!  所有有效  */


/* HTTP 80port search feature */
#define HTTP_HEADER_END_STR             "\r\n\r\n"
#define HTTP_PAYLOAD_SEARCH_OFFSET_L    116
#define HTTP_PAYLOAD_SEARCH_OFFSET_S    40
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
/*!
*\typedef   APPIDNTFY_TUPLE_INDEX
*\brief 四元组的索引
*/
typedef enum _APPIDNTFY_TUPLE_INDEX
{
    APPIDNTFY_SRC_IP  = 0,
    APPIDNTFY_SRC_PORT,
    APPIDNTFY_DST_IP,
    APPIDNTFY_DST_PORT,
    APPIDNTFY_RANDOM_PORT,
    APPIDNTFY_BASIC_APP,
    APPIDNTFY_TUPLE_INDEX_MAX
} APPIDNTFY_TUPLE_INDEX;

/*!
*\typedef   APPIDNTFY_SUBTABLE_INDEX
*\brief 由协议确定的二元组
*/
typedef enum _APPIDNTFY_SUBTABLE_INDEX
{
    APPIDNTFY_TCP_SUBTABLE    = 0,
    APPIDNTFY_UDP_SUBTABLE,
    APPIDNTFY_SUBTABLE_MAX
} APPIDNTFY_SUBTABLE_INDEX;

/*!
 *\typedef  APPMNGR_APPID_AREA
 *\brief    定义应用ID的使用区域。
 */
typedef enum _APPMNGR_APPID_AREA
{
    APPMNGR_APPID_VOID          = -1,
    APPMNGR_APPID_NORMAL_BEGIN  = 0,        /* 普通应用的 ID范围 0-889 */
    APPMNGR_APPID_NORMAL_END    = 3999,      /* 普通应用的终点 */

    APPMNGR_APPID_AUTO_BEGIN    = 2000,
    APPMNGR_APPID_AUTO_END      = 2255,      /* 该范围的 ID 保留用于自动分配，勿占用 */

    APPMNGR_APPID_PRI_BEGIN     = 3000,
    APPMNGR_APPID_CLSF_BEGIN    = 3500,

    APPMNGR_PROXY_NONE          = 4000,      /* 分割线: 获取代理的ID后,加上此值,可获取相应的策略. */
    APPMNGR_PROXY_BEGIN         = 4001,      /* 代理起点 */
    APPMNGR_PROXY_END           = 4020,      /* 代理终点:共16个 */

    APPMNGR_APPID_SPECIAL_NONE  = 4020,      /* 分割线: 特殊应用基本上全部为0,供识别内部使用. */
    APPMNGR_APPID_SPECIAL_BEGIN = 4021,      /* 特殊应用 908-1007, 共100个 */
    APPMNGR_APPID_SPECIAL_END   = 4070,      /* 4021-4070 50个 */

    APPMNGR_APPID_BASIC_NONE    = 4070,     /* 分割线: 提取出高4位之后,加上此值可得到基础应用的ID，若结果为APPMNGR_APPID_BASIC_NONE，则表示无基础应用 */
    APPMNGR_APPID_BASIC_BEGIN   = 4071,     /* 基础应用的 ID范围 4071-4085, 共15个 */
    APPMNGR_APPID_BASIC_END     = 4083,

}APPMNGR_APPID_AREA;

typedef struct _APP_ID_OP_FUNC
{
    void   (*app_id_value_set)(short *appId, short value);
    short   (*app_id_value_get)(short appId);
    void   (*app_id_flag_set)(short *appId, short value);
    short  (*app_id_flag_get)(short appId);
    int   (*is_app_id_valid)(short appId);
}APP_ID_OP_FUNC;

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/




#endif  /* __APPIDENTIFY_RULE_H__ */
