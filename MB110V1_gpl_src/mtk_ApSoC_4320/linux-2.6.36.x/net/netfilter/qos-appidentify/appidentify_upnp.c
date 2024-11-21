/*! Copyright(c) 2008-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     app_upnp.c
 *\brief
 *\details
 *
 *\author   Weng Kaiping
 *\version
 *\date     17Oct13
 *
 *\warning
 *
 *\history \arg
 */

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/
/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <linux/netdevice.h>
#include <linux/netlink.h>
#include <linux/security.h>

#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_app.h>

#include "appprio.h"
#include "appidentify_log.h"
#include "appidentify_netlink.h"
#include "appidentify_upnp.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define APP_UPNP_ERROR(fmt, args...)   printk("[Error](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args)
#define APP_UPNP_DEBUG(fmt, args...)   /*printk("[Debug](%s) %05d: "fmt, __FUNCTION__, __LINE__, ##args) */

#define MAX_PORTMAPPING_NUM 32

static DEFINE_MUTEX(appupnp_mutex);
/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
typedef struct app_upnp_node
{
    struct list_head   list;
    app_upnp_mapping_t mapping;
}app_upnp_entry_t;

typedef struct _UPNP_APP_MAP
{
    char *appName;
    int appId;
}UPNP_APP_MAP;


/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/
bool appUpnpInitd = false;
static app_upnp_entry_t *entryPool = NULL;
static struct list_head entryList;
static struct list_head entryListFree;

static unsigned int maxEntryCount = MAX_PORTMAPPING_NUM;

UPNP_APP_MAP l_upnp_app_check[] =
{
    {"Thunder",     APP_ID_THUNDER},
    {"Skype",       APP_ID_SKYPE},
    {"QQDownload",  APP_ID_QQXF},
    {"emule",       APP_ID_EMULE},

    {NULL,          -1}
};

int app_upnp_debug = 1;
/**************************************************************************************************/
/*                                      FUNCTION                                                */
/**************************************************************************************************/
static int pmFlush(void)
{
    app_upnp_entry_t *entry = NULL;

    mutex_lock(&appupnp_mutex);

    while (!list_empty_careful(&entryList))
    {
        entry = (app_upnp_entry_t *)entryList.prev;
        list_del(&(entry->list));
        memset(entry, 0, sizeof(app_upnp_entry_t));
        list_add_tail(&(entry->list), &entryListFree);
    }

    mutex_unlock(&appupnp_mutex);

    return 0;
}

static int pmDisplay(void)
{
    app_upnp_entry_t *entry = NULL;
    app_upnp_mapping_t *mapping = NULL;
    struct list_head *list;

    mutex_lock(&appupnp_mutex);
    printk("app upnp entry list\n");
    printk("des externport internip internport protocol enabled\n");
    for (list = (entryList.next); list != &entryList; list = list->next)
    {
        entry   = (app_upnp_entry_t *)list;
        mapping = &(entry->mapping);

        printk("%08x %12s extern %5d intern %08x %5d %s %s\n", (unsigned int)entry, mapping->description,
            mapping->externalPort,mapping->internalClient, mapping->internalPort,
           (mapping->protocol == IPPROTO_TCP)? "tcp" : "udp",
            mapping->enabled? "enable":"disable");
    }
    mutex_unlock(&appupnp_mutex);

    return 0;
}
static app_upnp_entry_t *pmFindEntry(unsigned short externalPort,unsigned char proto)
{
    app_upnp_entry_t   *entry = NULL;
    app_upnp_mapping_t *mapping = NULL;
    struct list_head *list;

    for (list = (entryList.next); list != &entryList; list = list->next)
    {
        entry   = (app_upnp_entry_t *)list;
        mapping = &(entry->mapping);
        if ((externalPort == mapping->externalPort) &&
            (proto == mapping->protocol))
        {
            printk("found entry 0x%08x - %s; extern = %d, proto=%d\r\n",
                (unsigned int)entry, mapping ? mapping->description : "none",  externalPort, proto);
            return entry;
        }
    }
    return NULL;
}


static app_upnp_entry_t *pmNewEntry(void)
{
    app_upnp_entry_t *entry = NULL;

    if (!list_empty_careful(&entryListFree))
    {
       entry = (app_upnp_entry_t *)(entryListFree.prev);
       APP_UPNP_DEBUG("new entry:%08x\n", entry);

       list_del(&(entry->list));
       memset(entry, 0, sizeof(app_upnp_entry_t));
       list_add_tail(&(entry->list), &entryList);
       APP_UPNP_DEBUG("The last free entry: %08x",entryListFree.prev);
    }

    return entry;
}


static int pmDeleteEntry( app_upnp_mapping_t * mapping)
{
    app_upnp_entry_t *entry = NULL;

    if (!mapping)
    {
        APP_UPNP_ERROR("mapping is null\n");
        return -1;
    }

    mutex_lock(&appupnp_mutex);

    if (NULL == (entry = pmFindEntry(mapping->externalPort, mapping->protocol)))
    {
       APP_UPNP_ERROR("can't find entry\n");
       mutex_unlock(&appupnp_mutex);
       return -1;
    }

    list_del(&(entry->list));
    memset(entry, 0, sizeof(app_upnp_entry_t));
    list_add_tail(&(entry->list), &entryListFree);

    mutex_unlock(&appupnp_mutex);

    return 0;
}

static int pmUpdateEntry(app_upnp_mapping_t * mapping)
{
    app_upnp_entry_t *entry = NULL;

    APP_UPNP_DEBUG("update entry:%s extern %d intern %x %d %s %s\n",
        mapping->description, mapping->externalPort,
        mapping->internalClient, mapping->internalPort,
        (mapping->protocol == 6)? "tcp" : "udp", mapping->enabled? "enable":"disable");

    mutex_lock(&appupnp_mutex);

    if (NULL == (entry = pmFindEntry(mapping->externalPort, mapping->protocol)))
    {
        if (NULL == (entry = pmNewEntry()))
        {
            APP_UPNP_ERROR("can't get new entry\n");
            mutex_unlock(&appupnp_mutex);
            return -1;
        }
    }
    else
    {

        if ((mapping->externalPort == entry->mapping.externalPort) &&
            (mapping->protocol == entry->mapping.protocol) &&
            (mapping->internalClient != entry->mapping.internalClient))
        {
            APP_UPNP_ERROR("no need to update\n");
            mutex_unlock(&appupnp_mutex);
            return -1;
        }
    }

    memcpy((char *)(&entry->mapping), (char *)mapping, sizeof(app_upnp_mapping_t));

    mutex_unlock(&appupnp_mutex);
    return 0;

}


int
app_netlink_set_upnp(app_upnp_mapping_t * mapping, unsigned short opt)
{
    if (!appUpnpInitd)
    {
        return -1;
    }
    if(mapping == NULL)
    {
        printk("mapping is null \n");
        return -1;
    }

    if (APP_OPT_UPDATE == opt)
    {
        return pmUpdateEntry(mapping);
    }
    else if (APP_OPT_REMOVE == opt)
    {
        return pmDeleteEntry(mapping);
    }
    else if (APP_OPT_DISPLAY == opt)
    {
        return pmDisplay();
    }
    else if (APP_OPT_FLUSH == opt)
    {
        return pmFlush();
    }

    APP_UPNP_ERROR("unsupport opt:%d\n", opt);

    return 0;
}


unsigned int app_upnp_ct_check(unsigned int hooknum,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn)(struct sk_buff *))
{
//    int             index = 0;
    int             appIndex = 0;
    unsigned int    protocol, srcip, dstip, srcport, dstport;
//    unsigned int    dir  = IP_CT_DIR_ORIGINAL;
    app_upnp_mapping_t      * upnpMap = NULL;
    app_upnp_entry_t        * entry = NULL;
    struct list_head        * list;
    struct nf_conn          * pCtEntry = NULL;
    struct nf_conntrack_app * app_info;
    enum ip_conntrack_info ctinfo;

    pCtEntry = nf_ct_get(skb, &ctinfo);
    if ( NULL == pCtEntry )
    {
        return NF_ACCEPT;
    }

    app_info = nf_ct_get_app(pCtEntry);
    if ( NULL == app_info)
    {
        return NF_ACCEPT;
    }

    if (APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_TOFROM_ROUTER)
     || APPPRIO_FLAG_IS_SET(app_info->appprio_flag, APPPRIO_FLAG_UPNP))
    {
        return NF_ACCEPT;
    }


    if ( pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num != AF_INET )
    {
        return NF_ACCEPT;
    }


    if (appUpnpInitd)
    {
        protocol = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
        srcip = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
        dstip = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip;
        srcport = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.tcp.port;
        dstport = pCtEntry->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.tcp.port;


        /*APP_UPNP_DEBUG("upnp_ct_check: protocol %s, srcip %08x srcport %08x, dstip %08x dstport %08x\n",
               protocol == IPPROTO_TCP ? "TCP":"UDP",
               srcip, srcport, dstip, dstport);*/

        for (list = (entryList.next); list != &entryList; list = list->next)
        {
            entry   = (app_upnp_entry_t *)list;
            upnpMap = &(entry->mapping);

            if (!upnpMap->enabled)
            {
                continue;
            }

            if (protocol != upnpMap->protocol ||
               ((srcip != upnpMap->internalClient || srcport != upnpMap->internalPort) &&
                dstport != upnpMap->externalPort))
            {
                continue;
            }

            //APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_UPNP);
            for (appIndex = 0; l_upnp_app_check[appIndex].appName != NULL; appIndex ++)
            {
                if (strstr(upnpMap->description, l_upnp_app_check[appIndex].appName))
                {
                    if (app_upnp_debug)
                    {
                        printk("upnp app proto %s name %s interport %d found.\n", protocol ==
                           IPPROTO_TCP ? "TCP":"UDP", l_upnp_app_check[appIndex].appName,
                           upnpMap->internalPort);
                    }
                    APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, l_upnp_app_check[appIndex].appId);
                    //APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index, l_upnp_app_check[appIndex].appId);
					APP_NORMAL_ID_SET(app_info->appidntfy_flag[IP_CT_DIR_REPLY].app_id_index, l_upnp_app_check[appIndex].appId);
					
					APPPRIO_FLAG_SET(app_info->appprio_flag, APPPRIO_FLAG_UPNP);
					
					printk("upnp app id=%d proto %s name %s interport %d found.\n", 
						   app_info->appidntfy_flag[IP_CT_DIR_ORIGINAL].app_id_index,
						   protocol == IPPROTO_TCP ? "TCP":"UDP", l_upnp_app_check[appIndex].appName,
                           upnpMap->internalPort);
					
                    /* todo more */
                    return NF_ACCEPT;
                }
            }
        }
    }

	return NF_ACCEPT;
}

int app_upnp_init(void)
{
    int i = 0;

    if (!(entryPool = (app_upnp_entry_t *)kmalloc(sizeof(app_upnp_entry_t)*maxEntryCount ,GFP_ATOMIC)))
    {
        printk("unable to alloc memory for UPnP entry pool!\r\n");
        return -1;
    }

    INIT_LIST_HEAD(&entryList);
    INIT_LIST_HEAD(&entryListFree);
    APP_UPNP_DEBUG("entryList: %08x, entryListFree:%08x\n",&entryList,&entryListFree);

    for (i = 0; i < maxEntryCount; i ++)
    {
        list_add_tail(&(entryPool[i].list), &entryListFree);
        APP_UPNP_DEBUG("The last free entry: %08x\n",entryListFree.prev);

    }

    APP_UPNP_DEBUG("app upnp init ok\r\n");
    appUpnpInitd = true;

    return 0;
}

void app_upnp_exit(void)
{
    appUpnpInitd = false;
    INIT_LIST_HEAD(&entryList);
    INIT_LIST_HEAD(&entryListFree);
    kfree(entryPool);


    printk("app upnp exit ok\r\n");
    return ;
}
