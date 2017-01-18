


#include <sodium.h>
#include "http.h"
#include "eth.h"
#include "util.h"
#include "encrypt.h"
#include "key.h"
#include "mqtt.h"
#include "db.h"
#include "vkey.h"

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


/*
 *
POST HOST/attestation
{
    "claim":{
        "templateId":"CLMT_IDNUMBER",
        "claim":{
            "value":"110021101000000"
        }
    },
    "proof":{
        @"from":"74444211acdaf12345",

        @"claimId":"2323423423",
        @"claimMd":"sdfadfa",

        "result":1,

        "name":"佛山自然人一门式",

        "time":"125545611",
        "expiration":"13545544",
        "link":"https://mysite.com/12323",

        "extra":{
            "staff":"20112313 李敏",
            "docs":"身份证原件，本人",
            "desc":"本人现场认证，无误"
        }
    }
}
 *
 *
 */
static int post_attest(struct mg_connection *nc, struct http_message *hm)
{
    cJSON *json = util_parseBody(&hm->body);
    cJSON *claim = cJSON_GetObjectItem(json, "claim");
    if(!claim)
    {
        http_response_error(nc,400,"Vkey Service : claim could not be parsed");
        return 0;
    }

    cJSON* id=cJSON_GetObjectItem(claim,"id");
    if(!id)
    {
        http_response_error(nc,400,"Vkey Service : claim id could not be parsed");
        return 0;
    }

    cJSON *proof = cJSON_GetObjectItem(json, "proof");
    if(!proof)
    {
        http_response_error(nc,400,"Vkey Service : proof could not be parsed");
        return 0;
    }


    //1: add claim id to proof
    cJSON_AddStringToObject(proof,"claimId",id->valuestring);

    //2: add claim hash to proof
    char* strClaim=cJSON_PrintUnformatted(claim);
    char claimHash[32];
    encrypt_hash(claimHash,strClaim,strlen(strClaim));

    char hexClaimHash[65];
    sodium_bin2hex(hexClaimHash,65,claimHash,32);

    cJSON_AddStringToObject(proof,"claimHash",hexClaimHash);

    //3: add attest public key to proof
    char hexAttestPK[65];
    sodium_bin2hex(hexAttestPK,65,key_getAttestPK(),32);
    cJSON_AddStringToObject(proof,"attestPK",hexAttestPK);

    //4: compute proof signature
    char* strProof=cJSON_PrintUnformatted(proof);
    char sigProof[64];
    encrypt_sign(strProof,strlen(strProof),key_getAttestSK(),sigProof);
    char hexSigProof[129];
    sodium_bin2hex(hexSigProof,129,sigProof,64);

    cJSON *result = cJSON_CreateObject();
    cJSON_AddItemToObject(result,"proof",proof);
    cJSON_AddStringToObject(result,"signature",hexSigProof);


    //todo: 5 send transction

    char topic[256];
//    sprintf(topic,"%s/attest",peer->valuestring);
    //3 publish to mq
  //  mqtt_send(topic,jwt,strlen(jwt));


    free(strProof);
    free(strClaim);

    cJSON_Delete(result);
    cJSON_Delete(json);

    http_response_text(nc,200,"Attestation post ok!");
}


int attest_got(const char* s_msg)
{
    printf("Got attestation data : %s \n", s_msg);

    //todo:1 parse and descrypt jwt


    eth_register("cn.guoqc.3","vid1233","pid1233","suk1233","vuk1233");

    //todo:2 notify app with json

    return 0;

}

cJSON* attest_read_by_claimid(const char* s_claimId)
{
    sqlite3* db = db_get();

    char strSql[256];
    sprintf(strSql,"SELECT ATTEST,RASK,TIME FROM TB_ATTEST WHERE CID=?;");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return NULL;
    }
    sqlite3_bind_text(pStmt, 1, s_claimId, strlen(s_claimId), SQLITE_TRANSIENT);

    cJSON *jResult = cJSON_CreateArray();

    while( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strAttest = (char *) sqlite3_column_text(pStmt, 0);
        char *strRASK = (char *) sqlite3_column_text(pStmt, 1);
        //todo: descrypt data
        cJSON* jData = cJSON_Parse(strAttest);
        if(jData&&strRASK)
        {
            cJSON_AddStringToObject(jData,"rask",strRASK);
        }
        cJSON_AddItemToArray(jResult,jData);
        cJSON_Delete(jData);
    }
    sqlite3_finalize(pStmt);
    return jResult;
}

/* translate
 * {
 *  proof:{},
 *  sig:{},
 *  rask:{}
 * }
 * to
 * {
 *  proof:{},
 *  signature:{},
 *  verify:{}
 * }
 *

 *
 *
 */
int attest_replace_rask_with_verify(cJSON* jAttest,const char* s_msg)
{
    cJSON* jRASK = cJSON_GetObjectItem(jAttest,"rask");
    if(!jRASK)
    {
        return -1;
    }
    char sk[VKEY_KEY_SIZE];
    size_t len;
    sodium_hex2bin(sk,VKEY_KEY_SIZE,jRASK->valuestring,64,NULL,&len,NULL);
    if(len!=VKEY_KEY_SIZE)
    {
        return -1;
    }
    char sig[VKEY_SIG_SIZE];
    if(0!=encrypt_sign(s_msg,strlen(s_msg),sk,sig))
    {
        return -1;
    }
    char sigHex[VKEY_SIG_SIZE*2+1];
    sodium_bin2hex(sigHex,VKEY_SIG_SIZE*2+1,sig,VKEY_SIG_SIZE);

    cJSON_AddStringToObject(jAttest,"verify",sigHex);
    cJSON_DeleteItemFromObject(jAttest,"rask");

    return 0;
}



int attest_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}