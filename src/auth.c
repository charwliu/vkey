
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
#include "register.h"
#include "eth.h"

static int post_auth(struct mg_connection *nc, struct http_message *hm);
static int req_auth(struct mg_connection *nc, struct http_message *hm);
static int del_auth(struct mg_connection *nc, struct http_message *hm);
static int test_auth(struct mg_connection *nc, struct http_message *hm);

static http_router routers[3]={
        {req_auth,"POST","/api/v1/auth/req"},
        {del_auth,"DELETE","/api/v1/auth/req"},
        {post_auth,"POST","/api/v1/auth"}
};

/*
  {
      "reg":"www.xxxx.cn",
      "templateIds":["CLMT_IDNUMBER","CLMT_NAME"],
      "duration":60,
      "desc":"description about the share"
 }
 * */
/// submit a auth requirement, reg is optional param, if it exist, sdk will compute register publick key ,secret key , and save them with url
/// \param jReq
/// \param jResult
/// \return
int auth_start(cJSON* jReq,cJSON* jResult)
{
    //start auth req
    cJSON *reg =  cJSON_GetObjectItem(jReq, "reg");
    cJSON *templateIds =  cJSON_GetObjectItem(jReq, "templateIds");
    cJSON *duration =  cJSON_GetObjectItem(jReq, "duration");
    //cJSON *desc =  cJSON_GetObjectItem(jReq, "desc");


    //subscribe topic
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

    //build topic
    char strTopic[128];
    sprintf(strTopic,"AUTH:%s",strPK);
    cJSON_AddStringToObject(jResult,"topic",strTopic);

    //if need register, compute RPK (Register Public Key), and save to db with reg url
    if(reg)
    {

        char strRPK[65];
        if(0!=register_start(reg->valuestring,strRPK))
        {
            return -1;
        }

        cJSON_AddStringToObject(jResult,"reg",reg->valuestring);
        cJSON_AddStringToObject(jResult,"rpk",strRPK);
    }


    //char names[128]="";
    if(templateIds)
    {
        //todo 2: get claim template names by templateIds

        //cJSON_AddStringToObject(jResult, "names", names);

        cJSON *jTids = cJSON_Duplicate(templateIds, 1);
        cJSON_AddItemToObject(jResult, "claimTemplates", jTids);
    }
    //cJSON_AddStringToObject(jResult,"desc",desc->valuestring);

    return 0;

}


/*
 {
    share:{
      "claimIds":["1234","5678","9998"],
      "duration":60,
      "confirm":1,
      "desc":"description about the share"
    },

   req:{
        "reg":"www.xxxx.cn",
        "templateIds":["CLMT_IDNUMBER","CLMT_NAME"],
        "duration":60,
        "desc":"description about the share"
    }

 }
 * */
static int req_auth(struct mg_connection *nc, struct http_message *hm)
{
    cJSON *json = util_parseBody(&hm->body);
    cJSON* jShare = cJSON_GetObjectItem(json,"share");

    cJSON *jShareRes = cJSON_CreateObject();
    if(0!=share_start(jShare,jShareRes))
    {
        //todo: false handle.
        //http_response_json(nc,400,jShareRes);
        cJSON_Delete(jShareRes);
        jShareRes=NULL;
    }

    cJSON *jReqRes = cJSON_CreateObject();
    cJSON* jReq = cJSON_GetObjectItem(json,"req");
    if(0!=auth_start(jReq,jReqRes))
    {
        //todo: false handle.
        //http_response_error(nc,400,"veky:register requirem failed");
    }

    cJSON* jRes=cJSON_CreateObject();
    if(jShareRes)
    {
        cJSON_AddItemToObject(jRes, "share", jShareRes);
    }
    cJSON_AddItemToObject(jRes,"req",jReqRes);

    http_response_json(nc,200,jRes);
    cJSON_Delete(json);
    cJSON_Delete(jRes);
    return 0;
}


/// DEL HOST/api/v1/auth/req?topic=4a321212123134
static int del_auth(struct mg_connection *nc, struct http_message *hm)
{
    char strPK[65]="";
    mg_get_http_var(&hm->query_string, "topic", strPK, 65);
    if( strlen(strPK)==0)
    {
        http_response_error(nc,400,"Vkey Service : no valid topic");
        return 0;
    }
    char strTopic[100];
    sprintf(strTopic,"%s/%s","AUTH_DES",strPK);

    mqtt_unsubscribe(strTopic);

    http_response_text(nc,200,"auth request has been removed");

    return 0;
}


/*
POST
{
  "topic":"as84s8f7a8dfagyerwrg",
  "reg":"www.xxxx.com",
  "rpk":"12334",
  "claims":["123","456"],
  "nonce":"12345",
  "extra":{}
}
 * */
/// reg and rpk are optional attributes,if they exist, client execute register process
/// \param nc
/// \param hm
/// \return
static int post_auth(struct mg_connection *nc, struct http_message *hm) {

    cJSON *json = util_parseBody(&hm->body);
    cJSON *jTopic = cJSON_GetObjectItem(json, "topic");
    cJSON *jReg = cJSON_GetObjectItem(json, "reg");
    cJSON *jRPK = cJSON_GetObjectItem(json, "rpk");
    cJSON *jClaimIds = cJSON_GetObjectItem(json, "claims");
    cJSON *jNonce = cJSON_GetObjectItem(json, "nonce");
    cJSON *jExtra = cJSON_GetObjectItem(json, "extra");

    if(!jTopic)
    {
        http_response_error(nc,400,"Vkey Service : peer error");
        return 0;
    }

    if(!jNonce)
    {
        http_response_error(nc,400,"Vkey Service : nonce error");
        return 0;
    }

    unsigned char NonceHash[VKEY_KEY_SIZE];
    encrypt_hash(NonceHash, jNonce->valuestring, strlen(jNonce->valuestring));


    cJSON* jSend = cJSON_CreateObject();
    char strNonceSig[129];

    if( jReg )
    {
        char strIPK[65];
        unsigned  char ISK[VKEY_SIG_SK_SIZE];
        register_create(jReg->valuestring, jRPK->valuestring, strIPK,ISK);

        cJSON_AddStringToObject(jSend,"ipk",strIPK);
        cJSON_AddStringToObject(jSend,"reg",jReg->valuestring);

        unsigned char SIG[VKEY_SIG_SIZE];
        encrypt_sign(NonceHash,VKEY_KEY_SIZE,ISK,SIG);
        sodium_bin2hex(strNonceSig,129,SIG,VKEY_SIG_SIZE);

    }

    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);

    if(jClaimIds)
    {
        cJSON *jClaims = cJSON_CreateArray();

        claim_get_by_ids(jClaimIds, NonceHash, jClaims);

        cJSON_AddItemToObject(jSend, "claims", jClaims);
    }

    cJSON_AddStringToObject(jSend, "nonce", jNonce->valuestring);
    if(jReg)
    {
        cJSON_AddStringToObject(jSend, "nonceSig", strNonceSig);
    }


    if( jExtra )
    {
        cJSON* jExtraSend = cJSON_Duplicate(jExtra,1);
        cJSON_AddItemToObject(jSend,"extra",jExtraSend);
    }

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

    return 0;

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

/// when site got auth message, maybe include register requirement
/// \param s_peerTopic
/// \param s_data
/// \return
int auth_got( const char* s_peerTopic, const char* s_data )
{

    printf("Auth From:%s\n",s_peerTopic);
    printf("Auth Message:%s\n",s_data);

    //handle register message
    cJSON* jData = cJSON_Parse(s_data);
    cJSON* jReg = cJSON_GetObjectItem(jData,"reg");
    cJSON* jIPK = cJSON_GetObjectItem(jData,"ipk");
    unsigned char RPK[VKEY_KEY_SIZE];
    unsigned char RSK[VKEY_SIG_SK_SIZE];
    if(jReg && 0==register_getKeys(jReg->valuestring,RPK,RSK))
    {
        unsigned char PID[VKEY_KEY_SIZE];
        encrypt_hash(PID,jIPK->valuestring,strlen(jIPK->valuestring));
        char strPID[65];
        sodium_bin2hex(strPID,65,PID,VKEY_KEY_SIZE);


        //todo: compute RID and sig
        unsigned char SIG[VKEY_SIG_SIZE];
        encrypt_sign(PID,VKEY_KEY_SIZE,RSK,SIG);

        char strSigRID[129];
        sodium_bin2hex(strSigRID,129,SIG,VKEY_SIG_SIZE);

        ///test
        //int x=encrypt_recover(PID,VKEY_KEY_SIZE,RPK,SIG);
        ///test

        char strRPK[65];
        sodium_bin2hex(strRPK,65,RPK,VKEY_KEY_SIZE);
        eth_register_site(strPID,strSigRID,strRPK);
        cJSON_AddStringToObject(jData,"pid",strPID);
    }

    char *pData = cJSON_PrintUnformatted(jData);

    g_notify(s_peerTopic,pData);

    free(pData);
    cJSON_Delete(jData);
    return 0;
}


int auth_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,3,nc,hm);
}