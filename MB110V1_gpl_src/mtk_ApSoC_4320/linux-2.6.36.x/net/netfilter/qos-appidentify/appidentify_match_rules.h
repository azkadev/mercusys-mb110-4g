/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_match_rules.h
 *\brief
 *\details
 *
 *\author   Yan Wei
 *\version  0.0.1
 *\date     09Oct13
 *
 *\warning
 *
 *\history \arg 0.0.1, 09Oct13, Yan Wei, Create the file.
 */
#ifndef __APPIDENTIFY_MATCH_RULES_H__
#define __APPIDENTIFY_MATCH_RULES_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
/*!
*\typedef   APPIDNTFY_DI_RET
*\brief 深度检测结果
*/
typedef enum _APPIDNTFY_DI_RET
{
    APPIDNTFY_DI_OVER = 0,
    APPIDNTFY_DI_NOT_REACH,
    APPIDNTFY_DI_DISABLE,
    APPIDNTFY_DI_UNKNOWN,
    APPIDNTFY_DI_KNOWN,
    APPIDNTFY_DI_MAX
}APPIDNTFY_DI_RET;
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
extern int g_enableDpi;

extern int g_debug_0;
extern int g_debug_1;
extern int g_debug_2;
extern int g_debug_3;
extern int g_debug_4;
extern int g_debug_5;
extern int g_debug_6;
extern int g_debug_7;
/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
unsigned int appidentify_match_hook(unsigned int hook,
                 struct sk_buff *skb,
                 const struct net_device *in,
                 const struct net_device *out,
                 int (*okfn)(struct sk_buff *));



#endif  /* __APPIDENTIFY_MATCH_RULES_H__ */
