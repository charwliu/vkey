


#include "vkey.h"
#include <sodium.h>
#include "http.h"
#include "util.h"
#include "db.h"
#include "mqtt.h"
#include "key.h"
#include "encrypt.h"
#include "claim.h"
#include "attest.h"
#include "start.h"

static int post_share(struct mg_connection *nc, struct http_message *hm);
static int get_share(struct mg_connection *nc, struct http_message *hm);

static http_router routers[2]={
        {post_share,"POST","/api/v1/share"},
        {get_share,"GET","/api/v1/share"}
};


/*
 *
POST HOST/share
{
  "claimIds":"1234,5678,9998",
  "duration":60,
  "confirm":1,
  "desc":"description about the share"
}

 duration:60, one mintue.
 confirm:1, need to confirm

 * */
/// start a share, program will subscribe the topic to listen others' request
/// \param nc
/// \param hm
/// \return

int share_start(cJSON* j_share,cJSON* j_result)
{
    //1 parse j_share body
    cJSON *claimIds = cJSON_GetObjectItem(j_share, "claimIds");
    cJSON *duration =  cJSON_GetObjectItem(j_share, "duration");
    cJSON *confirm =  cJSON_GetObjectItem(j_share, "confirm");
    cJSON *desc =  cJSON_GetObjectItem(j_share, "desc");


    if(!claimIds )
    {
        cJSON_AddStringToObject(j_result,"error","Vkey Service : claimIds needed!");
        return -1;
    }

    if(!duration )
    {
        cJSON_AddStringToObject(j_result,"error","Vkey Service : duration needed!");
        return -1;
    }

    if(!confirm )
    {
        cJSON_AddStringToObject(j_result,"error","Vkey Service : confirm needed!");
        return -1;
    }


    //todo 1: check if claims exist all


    //2 subscribe topic
    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);
    char strPK[65];
    char strSK[65];
    sodium_bin2hex(strPK,65,PK,VKEY_KEY_SIZE);
    sodium_bin2hex(strSK,65,SK,VKEY_KEY_SIZE);

    time_t nTime = time(NULL);


    char* pData = cJSON_PrintUnformatted(j_share);


    mqtt_subscribe("SHARE_SRC",PK,SK,nTime,duration->valueint,pData);
    free(pData);

    //4 build vlink

    char strTopic[128];
    sprintf(strTopic,"SHARE:%s",strPK);
    char names[128];
    //todo 2: get claim template names by claim ids

    cJSON_AddStringToObject(j_result,"topic",strTopic);
    cJSON_AddStringToObject(j_result,"names",names);
    cJSON_AddStringToObject(j_result,"desc",desc->valuestring);


    return 0;
}

static int post_share(struct mg_connection *nc, struct http_message *hm)
{

    //1 parse request body
    cJSON *json = util_parseBody(&hm->body);
    cJSON *jRes = cJSON_CreateObject();
    if(0==share_start(json,jRes))
    {
        http_response_json(nc,200,jRes);
    }
    else
    {
        http_response_json(nc,400,jRes);
    }
    cJSON_Delete(json);
    cJSON_Delete(jRes);
    return 0;


//    cJSON *claimIds = cJSON_GetObjectItem(json, "claimIds");
//    cJSON *duration =  cJSON_GetObjectItem(json, "duration");
//    cJSON *confirm =  cJSON_GetObjectItem(json, "confirm");
//    cJSON *desc =  cJSON_GetObjectItem(json, "desc");
//
//    if(!claimIds )
//    {
//        http_response_error(nc,400,"Vkey Service : claimIds needed!");
//        return 0;
//    }
//
//    if(!duration )
//    {
//        http_response_error(nc,400,"Vkey Service : duration needed!");
//        return 0;
//    }
//
//    if(!confirm )
//    {
//        http_response_error(nc,400,"Vkey Service : confirm needed!");
//        return 0;
//    }
//
//
//    //todo 1: check if claims exist all
//
//
//    //2 subscribe topic
//    unsigned char PK[VKEY_KEY_SIZE];
//    unsigned char SK[VKEY_KEY_SIZE];
//
//    encrypt_random(SK);
//    encrypt_makeDHPublic(SK,PK);
//    char strPK[65];
//    char strSK[65];
//    sodium_bin2hex(strPK,65,PK,VKEY_KEY_SIZE);
//    sodium_bin2hex(strSK,65,SK,VKEY_KEY_SIZE);
//
//    time_t nTime = time(NULL);
//
//
//    char* pData = cJSON_PrintUnformatted(json);
//
//
//    mqtt_subscribe("SHARE_SRC",PK,SK,nTime,duration->valueint,pData);
//    free(pData);
//
//    //4 build vlink
//    cJSON* res = cJSON_CreateObject();
//
//    char strTopic[128];
//    sprintf(strTopic,"SHARE:%s",strPK);
//    char names[128];
//    //todo 2: get claim template names by claim ids
//
//    cJSON_AddStringToObject(res,"topic",strTopic);
//    cJSON_AddStringToObject(res,"names",names);
//    cJSON_AddStringToObject(res,"desc",desc->valuestring);
//
//    http_response_json(nc,200,res);
//
//    cJSON_Delete(json);
//    cJSON_Delete(res);
//    return 0;
}

/*
 * GET /api/v1/share?topic=123456787
 */
static int get_share(struct mg_connection *nc, struct http_message *hm)
{

    char strSource[80]="";
    char strSourceTopic[128]="";
    mg_get_http_var(&hm->query_string, "topic", strSource, 80);
    sprintf(strSourceTopic,"SHARE_SRC/%s",strSource);

    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);

    time_t nTime = time(NULL);
    //3 subscribe topic
    mqtt_subscribe("SHARE_DES",PK,SK,nTime,0,"");
    mqtt_send(strSourceTopic,"SHARE_DES",PK,SK,"");


    http_response_text(nc,200,"ok");
    return 0;
}

///
/// \param s_msg ,json format {}
/// \return
int share_confirm(const char* s_peerTopic,const char* s_pk,const char* s_sk,cJSON* j_subData, const char* s_data )
{

    cJSON* jData = cJSON_Parse(j_subData->valuestring);
    cJSON* jClaimIds = cJSON_GetObjectItem(jData,"claimIds");


    //build data playload from claims

    //todo: parse multi claims

    cJSON* jSend = cJSON_CreateObject();
    cJSON_AddStringToObject(jSend,"pid","1234");
    cJSON* jClaims = cJSON_CreateArray();

    claim_get_with_proofs( jClaimIds,s_peerTopic, jClaims);

    cJSON_AddItemToObject(jSend,"claims",jClaims);

    //send data to des
    char* pData=cJSON_PrintUnformatted(jSend);

    time_t nTime = time(NULL);
    //mqtt_subscribe("SHARE_SRC",s_pk,s_sk,nTime,0,"");
    mqtt_send(s_peerTopic,"SHARE_SRC",s_pk,s_sk,pData);



    //release resource
    free(pData);
    cJSON_Delete(jData);
    cJSON_Delete(jSend);
    return 0;
}

int share_got( const char* s_peerTopic, const char* s_data )
{
    printf("Share Message:%s\n",s_data);
    g_notify(s_peerTopic,s_data);
    return 0;
}


int share_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}