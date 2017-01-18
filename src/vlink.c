
#include "vlink.h"
#include "http.h"
#include "util.h"
#include "mqtt.h"

static int get_vlink(struct mg_connection *nc, struct http_message *hm);
static int del_vlink(struct mg_connection *nc, struct http_message *hm);

static http_router routers[1]={
        {get_vlink,"GET","/api/v1/vlink"},
        {del_vlink,"DEL","/api/v1/vlink"}
};



int vlink_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}

static int get_vlink_auth(struct mg_connection *nc, struct http_message *hm)
{

    char strTopic[33];
    if(NULL==util_getUUID(strTopic,33))
    {
        http_response_error(nc,400,"Vkey Service : Could not generate random topic");
        return -1;
    }

    cJSON* res = cJSON_CreateObject();
    char strLink[1024];
    sprintf(strLink,"vlink://vnet.cn/auth/%s",strTopic);
    cJSON_AddStringToObject(res,"vlink",strLink);

    mqtt_subscribe(strTopic);

    http_response_json(nc,200,res);

    cJSON_Delete(res);
    return 0;
}

static int get_vlink(struct mg_connection *nc, struct http_message *hm)
{
    char strType[32]="";
    mg_get_http_var(&hm->query_string, "type", strType, 32);
    if( strcmp(strType,"auth")==0)
    {
        return get_vlink_auth(nc,hm);
    }


    http_response_error(nc,400,"Vkey Service : vlink type unsupported");
    return 0;
}

/// remove topic
/// DEL HOST/api/v1/vlink?topic=4a321212123134
/// \param nc
/// \param hm
/// \return
static int del_vlink(struct mg_connection *nc, struct http_message *hm)
{
    char strTopic[32]="";
    mg_get_http_var(&hm->query_string, "topic", strTopic, 32);
    if( strlen(strTopic)==0)
    {
        http_response_error(nc,400,"Vkey Service : no valid topic");
        return 0;
    }

    mqtt_unsubscribe(strTopic);

    http_response_text(nc,200,"vlink has been removed");

    return 0;

}
