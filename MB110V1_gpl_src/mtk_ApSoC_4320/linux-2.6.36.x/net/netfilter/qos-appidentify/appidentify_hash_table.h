/*! Copyright(c) 2008-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     appidentify_hash_table.h
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
#ifndef __APPIDENTIFY_HASH_TABLE_H__
#define __APPIDENTIFY_HASH_TABLE_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/hash.h>

#include "appidentify.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define HASH_KEY_BITS           (6)     /* hash table depth is 64, equal to APPIDNTFY_HASH_TABLE_LEN */
#define HASH_KEY_IP(ip)         hash_long(ip, HASH_KEY_BITS)
#define HASH_KEY_PORT(port)     hash_long(port, HASH_KEY_BITS)

#define APPIDNTFY_HASH_TABLE_LEN            (2 << HASH_KEY_BITS)

#define APPMNGR_MAX_DPI_TYPE_LEN            (16)
#define APPMNGR_MAX_DPI_CODE_LEN            (256)
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
/*!
 *\typedef  APPMNGR_FEATURE
 *\brief    The feature of the application on the connection and further on the payload.
 *
 *\warning  由于定义的成员实在太大了，因此，禁止在栈中声明一个对象，而只能声明一个指针。
 */
typedef struct _APPMNGR_FEATURE
{
    int     enable;     /*!< The feature of the rule is enable or not  */
    int     pkt_start;  /*!< The start packet that include feature on the connection, if the value
                        is -1, it means the feature exists in the middle of the connection, in the
                        mean time , the pkt_end is the number of the packet that include feature.*/

    int     pkt_end;    /*!< The end packet that include feature on the connection. if the pkt_start is
                        -1, and it will be the number of the packet that include feature.*/
    int     packetLen;  /*!< The payload min length, set it to be 0 when unknown.*/
    char    dpiType[APPMNGR_MAX_DPI_TYPE_LEN]; /*!< The dpi type */
    char    dpiCode[APPMNGR_MAX_DPI_CODE_LEN]; /*!< The dpi code, regular expresion */
}APPMNGR_FEATURE;


/*!
 *\typedef  APPMNGR_TUPLE
 *\brief    The tuple of the connection.
 */
typedef struct _APPMNGR_TUPLE
{
    unsigned int  srcip;                  /*!< source ip address */
    unsigned int  dstip;                  /*!< destination ip address */
    unsigned short  srcport;                /*!< source port number */
    unsigned short  dstport;                /*!< destination port number */
    unsigned char   proto;                  /*!< the protocol number */
} APPMNGR_TUPLE;

/*!
 *\typedef  APPMNGR_RULE
 *\brief    The structure of the rule.
 */
typedef struct _APPMNGR_RULE
{
    int             id;                     /*!< The application id number. */
    APPMNGR_TUPLE   tuple;                  /*!< The tuple of the rule */
    APPMNGR_FEATURE feature[IP_CT_DIR_MAX];    /*!< The feature of the fule.  */
}APPMNGR_RULE, *P_APPMNGR_RULE __attribute__((aligned));
/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/
int
appidentify_add_new_rule(APPMNGR_RULE *pAppRule, int ruleNum);

APPIDNTFY_KER_RULE*
appidentify_kernel_entry_alloc(void);

int
appidentify_kernel_entry_free(APPIDNTFY_KER_RULE *pKernelEntry);

int
appidentify_cleanup_rules(void);




#endif  /* __APPIDENTIFY_HASH_TABLE_H__ */
