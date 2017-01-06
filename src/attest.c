


#include "http.h"
#include "eth.h"
#include "util.h"

static int get_attest(struct mg_connection *nc, struct http_message *hm);
static int post_attest(struct mg_connection *nc, struct http_message *hm);

static http_router routers[2]={
        {get_attest,"GET","/api/v1/attestation"},
        {post_attest,"POST","/api/v1/attestation"}
};


static int get_attest(struct mg_connection *nc, struct http_message *hm)
{
    cJSON *json = util_parseBody(&hm->body);

    cJSON *claim = cJSON_GetObjectItem(json, "claim");
    if(!claim)
    {
        http_response_error(nc,400,"Vkey Service : claim error");
        return 0;
    }
    cJSON *proof = cJSON_GetObjectItem(json, "proof");
    if(!proof)
    {
        http_response_error(nc,400,"Vkey Service : proof error");
        return 0;
    }

    cJSON *signature = cJSON_GetObjectItem(json, "signature");
    if(!signature)
    {
        http_response_error(nc,400,"Vkey Service : signature error");
        return 0;
    }

    //todo: check it

    cJSON_Delete(json);

    cJSON* res = cJSON_CreateObject();

    cJSON_AddStringToObject(res,"attestation","yes");

    http_response_json(nc,200,res);

    cJSON_Delete(res);
}

static int post_attest(struct mg_connection *nc, struct http_message *hm)
{
    //todo:1 verify signature of the claim by peer vid

    //todo:2 compute proof signature

    //todo:3 send transction to Smart Contract

    eth_register("cn.guoqc.3","vid1233","pid1233","suk1233","vuk1233");
    http_response_text(nc,200,"Attestation post ok!");
}



int attest_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}