


#include "http.h"
#include "util.h"

static int post_share(struct mg_connection *nc, struct http_message *hm);

static http_router routers[1]={
        {post_share,"POST","/api/v1/share"}
};


/*
POST HOST/share
{
  "templateId":"CLMT_IDNUMBER",
  "claimId":"1234",
}

 * */

static int post_share(struct mg_connection *nc, struct http_message *hm)
{

    cJSON *json = util_parseBody(&hm->body);

    cJSON *templateId = cJSON_GetObjectItem(json, "templateId");
    cJSON *claimId = cJSON_GetObjectItem(json, "claimId");

    if(!templateId && !claimId)
    {
        http_response_error(nc,400,"Vkey Service : templateId or claimId needed!");
        return 0;
    }

    //todo: share it


    cJSON_Delete(json);

    cJSON* res = cJSON_CreateObject();
    char hexLink[66]="https://share.vnet.com/12345";


    cJSON_AddStringToObject(res,"link",hexLink);

    http_response_json(nc,200,res);

    cJSON_Delete(res);
}



int share_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}