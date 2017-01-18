
#include "auth.h"
#include "http.h"
#include "mqtt.h"
#include "util.h"
#include "claim.h"
#include "jwt.h"

static int post_auth(struct mg_connection *nc, struct http_message *hm);
static int test_auth(struct mg_connection *nc, struct http_message *hm);

static http_router routers[1]={
        {post_auth,"POST","/api/v1/auth"}
};


/*
POST
{
  "to":"as84s8f7a8dfagyerwrg",
  "claims":["CLMT_IDNUMBER","CLMT_SOCIALSECURITY"]
}
 * */
static int post_auth(struct mg_connection *nc, struct http_message *hm) {

    cJSON *json = util_parseBody(&hm->body);
    cJSON *peer = cJSON_GetObjectItem(json, "to");
    if(!peer)
    {
        http_response_error(nc,400,"Vkey Service : peer error");
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
    char* jwt = jwt_create(peer->valuestring,claimsAll);

    char topic[256];
    sprintf(topic,"%s/auth",peer->valuestring);
    //3 publish to mq
    mqtt_send(topic,jwt,strlen(jwt));

    http_response_text(nc,200,"data sent");

    free(jwt);
    cJSON_Delete(json);
    return 0;
}

int auth_got(const char* s_msg)
{

    printf("Got auth data : %s \n", s_msg);

    //todo:1 parse and descrypt jwt


    //todo:2 notify app with json

    return 0;

}


int auth_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}