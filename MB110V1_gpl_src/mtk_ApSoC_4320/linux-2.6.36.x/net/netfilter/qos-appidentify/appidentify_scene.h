/***************************************************************
 *
 * Copyright(c) 2014 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename:     appidentify_scene.h
 * Version:      1.0
 * Abstract:     Appidentify scene mode implement
 * Author:       Li Wenxiang  (liwenxiang@tp-link.net)
 * Created Date: 2014-07-01
 *
 ***************************************************************/

#ifndef __APPIDENTIFY_SCENE_H__
#define __APPIDENTIFY_SCENE_H__

typedef enum _APPIDNTFY_SCENE {
    SCENE_SMART_LIMIT = 0,
    SCENE_FULLSPEED_DOWNLOAD,
    SCENE_WEB_SURF,
    SCENE_ONLINE_VIDEO
}APPIDNTFY_SCENE;


typedef enum _APPIDNTFY_APP_TYPE
{
    APP_TYPE_OTHER = 0,
    APP_TYPE_WEB,
    APP_TYPE_VIDEO,
    APP_TYPE_VIDEO_P2P,
    APP_TYPE_MUSIC,
    APP_TYPE_GAME,
    APP_TYPE_P2P,
    APP_TYPE_DOWNLOAD
}APPIDNTFY_APP_TYPE;

struct appid_scene_config
{
    int web_prio;
    int video_prio;
    int video_p2p_prio;
    int download_prio;
    int httpfd_prio;
};

#define appprio_incr_version() g_appprio_version++

int   app_scene_init(void);
int   app_scene_exit(void);
int   app_parse_scene(char* scene_name);
void  app_set_scene(int scene);
void  print_app_type_map(void);


#endif // __APPIDENTIFY_SCENE_H__
