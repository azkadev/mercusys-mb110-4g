/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_scene.c
 * Version:      1.0
 * Abstract:     Appidentify scene mode implement
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/list.h>
#include <linux/mempool.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_app.h>

#include "appprio.h"
#include "appidentify_id.h"
#include "appidentify_log.h"
#include "appidentify_scene.h"

unsigned int l_game_apps[] = {APP_ID_QQ_GAME, APP_ID_SPF, APPPRIO_APP_ID_END};

unsigned int l_video_apps[] = {APP_ID_YKTD, APP_ID_TXVIDEO, APP_ID_IQIYI, \
APP_ID_LETV, APP_ID_SOHUVIDEO, APP_ID_CNTV, APP_ID_VIDEO_ONLINE, APPPRIO_APP_ID_END};

unsigned int l_video_p2p_apps[] = {APP_ID_QQLIVE, APP_ID_PPSTREAM, APP_ID_PPTV, \
APP_ID_PPTC, APP_ID_KUAIBO, APP_ID_FX, APP_ID_CLSF_PPS, APP_ID_CLSF_PPTV,       \
APP_ID_CLSF_FLASHP2P, APPPRIO_APP_ID_END};

unsigned int l_download_apps[] = {APP_ID_THUNDER, APP_ID_XLLX, APP_ID_BITTORRENT, \
APP_ID_EMULE, APP_ID_QQXF, APP_ID_FLASHGET, APP_ID_CLSF_XL, APP_ID_CLSF_BT,       \
APP_ID_CLSF_EMULE, APP_ID_CLSF_FLASHGET, APP_ID_FTP, APP_ID_XL_OTHERS,            \
APPPRIO_APP_ID_END};

unsigned int l_web_apps[] = {APP_ID_HTTP, APP_ID_HTTPS, APP_ID_SMTP, APP_ID_POP3, \
APP_ID_PPTP, APP_ID_L2TP, APP_ID_IPSEC, APP_ID_IMAP, APP_ID_DNS, APPPRIO_APP_ID_END};

// static DEFINE_MUTEX(appprio_version_lock); ????
int g_appprio_version = 0;
unsigned char *g_appTypeMap = NULL;
extern unsigned char *g_appPriDftMap;


int app_scene_init(void)
{
    int i;
    int appid;

    g_appprio_version = 0;
    g_appTypeMap = (unsigned char*)kmalloc(APPPRIO_APP_MAP_SIZE * sizeof(unsigned char), GFP_KERNEL);
    if (NULL == g_appTypeMap)
    {
        return -1;
        APPID_ERR(APPID_BASE, "allocate memory for appTypeMap failed");
    }

    memset(g_appTypeMap, 0, APPPRIO_APP_MAP_SIZE * sizeof(unsigned char));

    // init game types
    i = 0;
    while (l_game_apps[i] != APPPRIO_APP_ID_END)
    {
        appid = l_game_apps[i];
        g_appTypeMap[appid] = APP_TYPE_GAME;
        i++;
    }

    // init video types
    i = 0;
    while (l_video_apps[i] != APPPRIO_APP_ID_END)
    {
        appid = l_video_apps[i];
        g_appTypeMap[appid] = APP_TYPE_VIDEO;
        i++;
    }

    // init p2p video types
    i = 0;
    while (l_video_p2p_apps[i] != APPPRIO_APP_ID_END)
    {
        appid = l_video_p2p_apps[i];
        g_appTypeMap[appid] = APP_TYPE_VIDEO_P2P;
        i++;
    }

    // init download types
    i = 0;
    while (l_download_apps[i] != APPPRIO_APP_ID_END)
    {
        appid = l_download_apps[i];
        g_appTypeMap[appid] = APP_TYPE_DOWNLOAD;
        i++;
    }

    // init web types
    i = 0;
    while (l_web_apps[i] != APPPRIO_APP_ID_END)
    {
        appid = l_web_apps[i];
        g_appTypeMap[appid] = APP_TYPE_WEB;
        i++;
    }

    return 0;
}

int app_scene_exit(void)
{
    if (NULL != g_appTypeMap)
    {
        kfree(g_appTypeMap);
    }
	return 0;
}

void  print_app_type_map(void)
{
    int i;
    char app_type[20] = {0};

    if (NULL == g_appTypeMap || NULL == g_appPriDftMap)
    {
        APPID_ERR(APPID_BASE, "app map is NULL");
        return;
    }

    for (i = 0; i < APPPRIO_APP_MAP_SIZE; ++i)
    {
        if (g_appTypeMap[i] != 0)
        {
            memset(app_type, 0, 20);
            if (g_appTypeMap[i] == APP_TYPE_WEB)
            {
                strcpy(app_type, "web");
            }
            else if (g_appTypeMap[i] == APP_TYPE_GAME)
            {
                strcpy(app_type, "game");
            }
            else if (g_appTypeMap[i] == APP_TYPE_VIDEO)
            {
                strcpy(app_type, "video");
            }
            else if (g_appTypeMap[i] == APP_TYPE_VIDEO_P2P)
            {
                strcpy(app_type, "p2p video");
            }
            else if (g_appTypeMap[i] == APP_TYPE_DOWNLOAD)
            {
                strcpy(app_type, "download");
            }
            else
            {
                strcpy(app_type, "other?");
            }

            printk("AppID = %d,  Type = %s,  Prio = %d\n",
                   i, app_type, g_appPriDftMap[i]);
        }
    }
}

static void app_scene_update_prio(struct appid_scene_config* config)
{
    int i;

    if (NULL == g_appTypeMap || NULL == g_appPriDftMap)
    {
        APPID_ERR(APPID_BASE, "app map is NULL");
        return;
    }

    for (i = 0; i < APPPRIO_APP_MAP_SIZE; ++i)
    {
        if (APP_TYPE_GAME == g_appTypeMap[i])
        {
            g_appPriDftMap[i] = APPPRIO_FIRST_PRIO;
        }
        else if (APP_TYPE_WEB == g_appTypeMap[i])
        {
            g_appPriDftMap[i] = config->web_prio;
        }
        else if (APP_TYPE_VIDEO == g_appTypeMap[i])
        {
            g_appPriDftMap[i] = config->video_prio;
        }
        else if (APP_TYPE_VIDEO_P2P == g_appTypeMap[i])
        {
            g_appPriDftMap[i] = config->video_p2p_prio;
        }
        else if (APP_TYPE_DOWNLOAD == g_appTypeMap[i])
        {
            g_appPriDftMap[i] = config->download_prio;
        }
        else if (i == APP_ID_HTTPFD)
        {
            g_appPriDftMap[i] = config->httpfd_prio;
        }
    }

    appprio_incr_version();
}

static void app_scene_smart_limit(void)
{
    struct appid_scene_config config =
        {1, 2, 3, 4, 3};

    APPID_LOG(APPID_BASE, "set scene mode to smart limit");
    app_scene_update_prio(&config);
}

static void app_scene_fullspeed_download(void)
{
    struct appid_scene_config config =
        {1, 2, 3, 1, 1};

    app_scene_update_prio(&config);
    APPID_LOG(APPID_BASE, "set scene mode to fullspeed download");
}

static void app_scene_web_surf(void)
{
    struct appid_scene_config config =
        {1, 2, 4, 4, 3};

    app_scene_update_prio(&config);
    APPID_LOG(APPID_BASE, "set scene mode to web surf");
}

static void app_scene_online_video(void)
{
    struct appid_scene_config config =
        {1, 1, 2, 4, 2};

    app_scene_update_prio(&config);
    APPID_LOG(APPID_BASE, "set scene mode to online video");
}


int app_parse_scene(char* scene_name)
{
    // default use smart limit
    if (NULL == scene_name)
    {
        return SCENE_SMART_LIMIT;
    }
    else if (0 == strcmp(scene_name, "smart_limit\n"))
    {
        return SCENE_SMART_LIMIT;
    }
    else if (0 == strcmp(scene_name, "download\n"))
    {
        return SCENE_FULLSPEED_DOWNLOAD;
    }
    else if (0 == strcmp(scene_name, "web\n"))
    {
        return SCENE_WEB_SURF;
    }
    else if (0 == strcmp(scene_name, "video\n"))
    {
        return SCENE_ONLINE_VIDEO;
    }
    else
    {
        return SCENE_SMART_LIMIT;
    }
}

void  app_set_scene(int scene)
{
    switch(scene)
    {
    case SCENE_SMART_LIMIT:
        app_scene_smart_limit();
        break;
    case SCENE_FULLSPEED_DOWNLOAD:
        app_scene_fullspeed_download();
        break;
    case SCENE_WEB_SURF:
        app_scene_web_surf();
        break;
    case SCENE_ONLINE_VIDEO:
        app_scene_online_video();
        break;
    default:
        app_scene_smart_limit();
        break;
    }
}
