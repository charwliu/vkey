


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

static int post_share(struct mg_connection *nc, struct http_message *hm);
static int get_share(struct mg_connection *nc, struct http_message *hm);

static http_router routers[1]={
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
static int post_share(struct mg_connection *nc, struct http_message *hm)
{

    //1 parse request body
    cJSON *json = util_parseBody(&hm->body);

    cJSON *claimIds = cJSON_GetObjectItem(json, "claimIds");
    cJSON *duration =  cJSON_GetObjectItem(json, "duration");
    cJSON *confirm =  cJSON_GetObjectItem(json, "confirm");
    cJSON *desc =  cJSON_GetObjectItem(json, "desc");

    if(!claimIds )
    {
        http_response_error(nc,400,"Vkey Service : claimIds needed!");
        return 0;
    }

    if(!duration )
    {
        http_response_error(nc,400,"Vkey Service : duration needed!");
        return 0;
    }

    if(!confirm )
    {
        http_response_error(nc,400,"Vkey Service : confirm needed!");
        return 0;
    }


    //todo 1: check if claims exist all

    //2 save shared claim ids and topic to db

    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);
    char strPK[65];
    char strSK[65];
    sodium_bin2hex(strPK,65,PK,VKEY_KEY_SIZE);
    sodium_bin2hex(strSK,65,SK,VKEY_KEY_SIZE);

    time_t nTime = time(NULL);

    //3 subscribe topic
    mqtt_subscribe("SHARE",strPK,strSK,nTime,duration->valueint,claimIds->valuestring);

    //4 build vlink
    cJSON* res = cJSON_CreateObject();

    char strTopic[128];
    sprintf(strTopic,"SHARE/%s",strPK);
    char names[128];
    //todo 2: get claim template names by claim ids

    cJSON_AddStringToObject(res,"topic",strTopic);
    cJSON_AddStringToObject(res,"names",names);
    cJSON_AddStringToObject(res,"desc",desc->valuestring);

    http_response_json(nc,200,res);

    cJSON_Delete(json);
    cJSON_Delete(res);
    return 0;
}

/*
 * GET /api/v1/share?topic=share.123456787
 */
static int get_share(struct mg_connection *nc, struct http_message *hm)
{

    char strSourceTopic[128]="";
    mg_get_http_var(&hm->query_string, "topic", strSourceTopic, 32);

    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);
    char strPK[65];
    char strSK[65];
    sodium_bin2hex(strPK,65,PK,VKEY_KEY_SIZE);
    sodium_bin2hex(strSK,65,SK,VKEY_KEY_SIZE);


    time_t nTime = time(NULL);
    char strTopic[128];
    sprintf(strTopic,"SHARE/%s",strPK);

    //3 subscribe topic
    mqtt_subscribe("SHARE",strPK,strSK,nTime,0,"");
    mqtt_send(strSourceTopic,strTopic,SK,VKEY_KEY_SIZE,"");


    return 0;
}

///
/// \param s_msg ,json format {}
/// \return
int share_confirm(struct mg_mqtt_message *msg )
{
    //get secret key by topic
    char* strTopic = util_getStr(&msg->topic);
    char* strPK = util_getPk(strTopic);

    cJSON* jSubscribe=cJSON_CreateObject();

    if( 0!=mqtt_readTopic(strTopic,jSubscribe))
    {
        return -1;
    }
    cJSON* jSK = cJSON_GetObjectItem(jSubscribe,"sk");
    cJSON* jData = cJSON_GetObjectItem(jSubscribe,"data");
    cJSON* jClaimIds = cJSON_GetObjectItem(jData,"claimIds");

    //get from public key
    char* strPayload = util_getStr(&msg->payload);
    cJSON* jPayload = cJSON_Parse(strPayload);
    cJSON* jFrom = cJSON_GetObjectItem(jPayload,"from");
    char* strFromPK = util_getPk(jFrom->valuestring);


    //generate sharekey from secret key and peer public key
    char shareKey[32];
    encrypt_makeDHShareKey(jSK->valuestring,strPK,strFromPK,shareKey);

    //build data playload from claims
    char** claimIds;
    int nClaimCount;

    //todo: parse multi claims

    cJSON* jSend = cJSON_CreateObject();
    cJSON_AddStringToObject(jSend,"pid","1234");
    cJSON* jClaims = cJSON_CreateArray();

    for( int i=0;i<nClaimCount;i++)
    {
        cJSON* jClaim = claim_read_by_claimid(claimIds[i]);
        cJSON* jAttests = attest_read_by_claimid(claimIds[i]);
        cJSON* jClaimWithAttest = util_assemble(jClaim,jAttests,strFromPK);

        cJSON_AddItemToArray( jClaims,jClaimWithAttest);
        cJSON_Delete(jClaim);
        cJSON_Delete(jAttests);
        cJSON_Delete(jClaimWithAttest);

    }
    cJSON_AddItemToObject(jSend,"claims",jClaims);
    cJSON_Delete(jClaims);
    //encrypt data by sharekey
    //todo: encrypt data
    char* encryptData;

    //send data to des

    //release resource
    free(strPayload);
    free(strTopic);
    cJSON_Delete(jSend);
    cJSON_Delete(jPayload);
    cJSON_Delete(jSubscribe);
    return 0;
}

int share_got(struct mg_mqtt_message *msg )
{
    return 0;
}


int share_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}