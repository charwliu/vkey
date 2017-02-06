
#include <sodium.h>
#include "auth.h"
#include "http.h"
#include "mqtt.h"
#include "util.h"
#include "claim.h"
#include "jwt.h"
#include "share.h"
#include "vkey.h"
#include "encrypt.h"
#include "start.h"

static int post_auth(struct mg_connection *nc, struct http_message *hm);
static int req_auth(struct mg_connection *nc, struct http_message *hm);
static int test_auth(struct mg_connection *nc, struct http_message *hm);

static http_router routers[2]={
        {req_auth,"POST","/api/v1/auth/req"},
        {post_auth,"POST","/api/v1/auth"}
};

/*

 {
    share:{
      "claimIds":["1234","5678","9998"],
      "duration":60,
      "confirm":1,
      "desc":"description about the share"
    },
    req:{
        "templateIds":["CLMT_IDNUMBER","CLMT_NAME"]
    },
    attest:{
        "templateIds":["CLMT_IDNUMBER","CLMT_NAME"]"
    }

 }
 * */
int auth_start_req(cJSON* jReq,cJSON* jResult)
{
    //start auth req
    cJSON *templateIds =  cJSON_GetObjectItem(jReq, "templateIds");
    cJSON *duration =  cJSON_GetObjectItem(jReq, "duration");
    cJSON *desc =  cJSON_GetObjectItem(jReq, "desc");


    if(!templateIds )
    {
        cJSON_AddStringToObject(jResult,"error","Vkey Service : claimIds needed!");
        return -1;
    }


    //2 subscribe topic
    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);
    char strPK[65];
    sodium_bin2hex(strPK,65,PK,VKEY_KEY_SIZE);

    time_t nTime = time(NULL);
    char* pData = cJSON_PrintUnformatted(jReq);


    mqtt_subscribe("AUTH_DES",PK,SK,nTime,duration->valueint,pData);
    free(pData);

    //4 build vlink

    char strTopic[128];
    sprintf(strTopic,"AUTH:%s",strPK);
    char names[128]="";
    //todo 2: get claim template names by templateIds

    cJSON* jTids =  cJSON_Duplicate(templateIds,0);

    cJSON_AddStringToObject(jResult,"topic",strTopic);
    cJSON_AddStringToObject(jResult,"names",names);

    cJSON_AddStringToObject(jResult,"claimTemplates",jTids);
    cJSON_AddStringToObject(jResult,"desc",desc->valuestring);

    return 0;

}

static int req_auth(struct mg_connection *nc, struct http_message *hm)
{
    cJSON *json = util_parseBody(&hm->body);
    cJSON* jShare = cJSON_GetObjectItem(json,"share");

    cJSON *jShareRes = cJSON_CreateObject();
    if(0!=share_start(jShare,jShareRes))
    {
        //http_response_json(nc,400,jShareRes);
    }

    cJSON *jReqRes = cJSON_CreateObject();
    cJSON* jReq = cJSON_GetObjectItem(json,"req");
    if(0!=auth_start_req(jReq,jReqRes))
    {
        //http_response_json(nc,400,jReqRes);
    }

    cJSON* jRes=cJSON_CreateObject();
    cJSON_AddItemToObject(jRes,"share",jShareRes);
    cJSON_AddItemToObject(jRes,"req",jReqRes);


    http_response_json(nc,200,jRes);
    cJSON_Delete(json);
    cJSON_Delete(jRes);
    return 0;
}


/*
POST
{
  "topic":"as84s8f7a8dfagyerwrg",
  "claims":["123","456"]
}
 * */
static int post_auth(struct mg_connection *nc, struct http_message *hm) {

    cJSON *json = util_parseBody(&hm->body);
    cJSON *jTopic = cJSON_GetObjectItem(json, "topic");
    cJSON *jClaimIds = cJSON_GetObjectItem(json, "claims");
    if(!jTopic)
    {
        http_response_error(nc,400,"Vkey Service : peer error");
        return 0;
    }

    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);



    cJSON* jSend = cJSON_CreateObject();
    cJSON_AddStringToObject(jSend,"pid","1234");
    cJSON* jClaims = cJSON_CreateArray();

    claim_get_with_proofs( jClaimIds,jTopic->valuestring, jClaims);

    cJSON_AddItemToObject(jSend,"claims",jClaims);

    //send data to des
    char* pData=cJSON_PrintUnformatted(jSend);
    char strTopic[128];
    sprintf(strTopic,"AUTH_DES/%s",jTopic->valuestring);

    time_t nTime = time(NULL);
    mqtt_subscribe("AUTH_SRC",PK,SK,nTime,0,"");
    mqtt_send(strTopic,"AUTH_SRC",PK,SK,pData);

    //release resource
    free(pData);
    cJSON_Delete(json);
    cJSON_Delete(jSend);
    http_response_text(nc,200,"ok");

//
//    cJSON *claimTemplates = cJSON_GetObjectItem(json, "claims");
//    if(!claimTemplates)
//    {
//        http_response_error(nc,400,"Vkey Service : claims error");
//        return 0;
//    }
//
//    int nClaimCount = cJSON_GetArraySize(claimTemplates);
//    cJSON* claimsAll = cJSON_CreateArray();
//
//    //1 read from db
//    for(int i=0;i<nClaimCount;i++)
//    {
//        cJSON* claimT = cJSON_GetArrayItem(claimTemplates,i);
//        claim_read(claimT->valuestring,claimsAll);
//    }
//
//    //2 generate jwt
//    char* jwt = jwt_create(peer->valuestring,claimsAll);
//
//    char topic[256];
//    sprintf(topic,"%s/auth",peer->valuestring);
//    //3 publish to mq
//    //mqtt_send(topic,jwt,strlen(jwt));
//
//    http_response_text(nc,200,"data sent");
//
//    free(jwt);
//    cJSON_Delete(json);
//    return 0;
}

int auth_got( const char* s_peerTopic, const char* s_data )
{
    g_notify(s_peerTopic,s_data);
    return 0;
}


int auth_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}