
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
