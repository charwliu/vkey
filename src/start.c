
/* vkey start point */

#include "db.h"
#include "encrypt.h"
#include "http.h"
#include "mqtt.h"
#include "config.h"
#include "network.h"
#include "key.h"
#include "start.h"


FN_Notify g_fn_notity;
const char *g_callback_url;

static int post_stop(struct mg_connection *nc, struct http_message *hm );

static http_router routers[1]={
        {post_stop,"POST","/api/v1/stop"},
};


int g_notify(const char *s_topic, const char *s_msg)
{
    if (g_fn_notity)
    {
        g_fn_notity(s_topic, s_msg);
    }

    if (g_callback_url)
    {
        cJSON* jPayload=cJSON_CreateObject();
        cJSON_AddStringToObject(jPayload,"topic",s_topic);
        cJSON_AddStringToObject(jPayload,"msg",s_msg);
        char* pPayload = cJSON_PrintUnformatted(jPayload);
        http_post(g_callback_url,pPayload);
        free(pPayload);
        cJSON_Delete(jPayload);
    }
}

int start_vkey(const char *s_dbPath, const char *s_port, FN_Notify fn_notify, const char *s_callbackUrl)
{
    g_fn_notity = NULL;
    g_callback_url = NULL;

    config_load();

    if (s_port)
    {
        g_config.http_port = s_port;
    }

    //db_init("/home/gqc/vkey/db.vkey");
    if (-1 == db_init(s_dbPath))
    {
        return -1;
    }

    if (-1 == encrypt_init())
    {
        return -1;
    }

    if (fn_notify)
    {
        g_fn_notity = fn_notify;
    }

    if (s_callbackUrl)
    {
        g_callback_url = s_callbackUrl;
    }

    network_start();

    return 0;
}


static int post_stop(struct mg_connection *nc, struct http_message *hm )
{
    //todo: release resource and stop vkey
    //todo:1 unsubsribe topic from mqtt

    //todo:2 close db file

    //todo:3
    return 0;
}


int start_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}
