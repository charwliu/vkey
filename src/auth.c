
#include "auth.h"
#include "http.h"
#include "mqtt.h"
#include "util.h"
#include "claim.h"
#include "jwt.h"

static int post_auth(struct mg_connection *nc, struct http_message *hm);
static int test_auth(struct mg_connection *nc, struct http_message *hm);

static http_router routers[2]={
        {post_auth,"POST","api/v1/auth"},
        {test_auth,"GET","api/v1/auth"}
};

static int test_auth(struct mg_connection *nc, struct http_message *hm) {

    mqtt_send("1000001","hellobaby!",10);
    return 0;
}

/*
POST
{
  "vid":"as84s8f7a8dfagyerwrg",
  "claims":["CLMT_IDNUMBER","CLMT_SOCIALSECURITY"]
}
 * */
static int post_auth(struct mg_connection *nc, struct http_message *hm) {

    cJSON *json = util_parseBody(&hm->body);
    cJSON *vid = cJSON_GetObjectItem(json, "vid");
    if(!vid)
    {
        http_response_error(nc,400,"Vkey Service : vid error");
        return 0;
    }
    cJSON *claimTemplates = cJSON_GetObjectItem(json, "claims");
    if(!claimTemplates)
    {
        http_response_error(nc,400,"Vkey Service : claims error");
        return 0;
    }

    int nClaimCount = cJSON_GetArraySize(claimTemplates);
    cJSON* claimsAll = cJSON_CreateArray();

    //1 read from db
    for(int i=0;i<nClaimCount;i++)
    {
        cJSON* claimT = cJSON_GetArrayItem(claimTemplates,i);
        claim_read(claimT->valuestring,claimsAll);
    }

    //2 generate jwt
    char* jwt = jwt_create(vid->valuestring,claimsAll);

    //3 publish to mq
    mqtt_send(vid->valuestring,jwt,strlen(jwt));

    //todo:7 return ok
    http_response_text(nc,200,"ok");


    free(jwt);
    cJSON_Delete(json);
    return 0;
}

int auth_got(const char* s_msg)
{
    //todo:1 parse and descrypt jwt

    //todo:2 notify app with json

}


int auth_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}