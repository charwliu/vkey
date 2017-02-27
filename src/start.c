
/* vkey start point */

#include "db.h"
#include "encrypt.h"
#include "http.h"
#include "mqtt.h"
#include "config.h"
#include "network.h"
#include "key.h"
#include "start.h"
#include "template.h"


FN_Notify g_fn_notity;
const char *g_callback_url;

static int vkey_init=0;
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



int start_vkey(const char *s_dbPath,const char* s_password, const char *s_port,  FN_Notify fn_notify, const char *s_callbackUrl)
{
    if(!vkey_init)
    {
        return -1;
    }
    g_fn_notity = NULL;
    g_callback_url = NULL;


    if (s_port)
    {
        g_config.http_port = s_port;
    }


    if (-1 == db_open(s_dbPath,s_password))
    {
        return -1;
    }

    if(0!=key_chechPassword(s_password))
    {
        db_close();
        printf("vkey:wrong password!");
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

int init_key()
{
    config_load();
    if (-1 == encrypt_init())
    {
        return -1;
    }
    vkey_init=1;
    return 0;
}

int create_key(const char* s_dbPath,const char* s_password, const char* s_rescure,const char* s_random ,char* s_ciperIUK)
{

    if(!vkey_init)
    {
        return -1;
    }
    if (-1 == db_init(s_dbPath))
    {
        return -1;
    }


    int ret = key_create(s_rescure,s_password,s_random,s_ciperIUK);

    db_close();

    return ret;
}

int update_key(const char* s_dbPath, const char* s_ciperOldIUK,const char* s_oldRescure,const char* s_password, const char* s_rescure,const char* s_random ,char* s_ciperIUK)
{
    if(!vkey_init)
    {
        return -1;
    }
    if (-1 == db_init(s_dbPath))
    {
        return -1;
    }

    int ret=key_update(s_ciperOldIUK,s_oldRescure,s_rescure,s_password,s_random,s_ciperIUK);
    db_close();
    return ret;

}

int verify_iuk(const char* s_dbPath, const char* s_ciperOldIUK,const char* s_oldRescure)
{
    if(!vkey_init)
    {
        return -1;
    }
    if (-1 == db_init(s_dbPath))
    {
        return -1;
    }


    int ret = key_checkIUK(s_ciperOldIUK,s_oldRescure);

    db_close();

    return ret;
}

static int post_stop(struct mg_connection *nc, struct http_message *hm )
{
    //1 unsubsribe topic from mqtt
    mqtt_close();

    //2 close db file
    db_close();

    //3 stop network
    network_stop();

    template_clear();

    http_response_text(nc,200,"stoped!");
    return 0;
}


int start_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}
