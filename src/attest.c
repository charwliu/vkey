


#include <sodium.h>
#include "http.h"
#include "eth.h"
#include "util.h"
#include "encrypt.h"
#include "key.h"
#include "mqtt.h"
#include "db.h"
#include "vkey.h"
#include "start.h"
#include "claim.h"
#include "attest.h"

static int get_attest(struct mg_connection *nc, struct http_message *hm);
static int post_attest_self(struct mg_connection *nc, struct http_message *hm);
static int attest_create(const char* s_cid,const char* s_tid, cJSON* j_proof,cJSON* j_signature);

static http_router routers[3]={
        {get_attest,"GET","/api/v1/attestation"},
        {post_attest,"POST","/api/v1/attestation"},
        {post_attest_self,"POST","/api/v1/attestation/self"},
};


/// GET /api/v1/attestation?psig=xxxxx&apk=yyyy&nonce=11111&nsig=2222
static int get_attest(struct mg_connection *nc, struct http_message *hm)
{


    char strProofSig[180]="";
    char strAPK[80]="";
    char strMessage[80]="";
    char strMessageSig[180]="";
    mg_get_http_var(&hm->query_string, "psig", strProofSig, 180);
    mg_get_http_var(&hm->query_string, "apk", strAPK, 80);
    mg_get_http_var(&hm->query_string, "nonce", strMessage, 80);
    mg_get_http_var(&hm->query_string, "nsig", strMessageSig, 180);

    unsigned char HashMsg[VKEY_KEY_SIZE];
    encrypt_hash(HashMsg,strMessage,strlen(strMessage));
    char strHashMsg[65];
    sodium_bin2hex(strHashMsg,65,HashMsg,32);
    eth_attest_read(strProofSig,strAPK,strHashMsg,strMessageSig);



    cJSON* res = cJSON_CreateObject();

    cJSON_AddStringToObject(res,"attestation","not ready now");

    http_response_json(nc,200,res);

    cJSON_Delete(res);
}


/*
 *
POST HOST/attestation?topic=123455
{
    "claim":{
        "id":"12345"
        "templateId":"CLMT_IDNUMBER",
        "claim":{
            "value":"110021101000000"
        }
    },
    "proof":{
        @"claimId":"2323423423",
        @"claimMd":"sdfadfa",
        @"attestPk":"aacccc",

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
    },
    @"signature":"a12s112"
}
 *
 *
 */
int post_attest(struct mg_connection *nc, struct http_message *hm)
{

    char strSourceTopic[128]="";
    mg_get_http_var(&hm->query_string, "topic", strSourceTopic, 80);



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


    //2: add claim hash to proof
    char* strClaim=cJSON_PrintUnformatted(claim);
    char claimHash[32];
    encrypt_hash(claimHash,strClaim,strlen(strClaim));

    char hexClaimHash[65];
    sodium_bin2hex(hexClaimHash,65,claimHash,32);

    cJSON_AddStringToObject(proof,"md",hexClaimHash);

    //3: add attest public key to proof
    char hexAttestPK[65];
    memset(hexAttestPK,0,65);
    unsigned char AttestSK[VKEY_SIG_SK_SIZE];
    key_get(NULL,NULL,hexAttestPK,AttestSK,NULL);

    //attest_getKeys(hexAttestPK,hexAttestSK);

    cJSON_AddStringToObject(proof,"apk",hexAttestPK);

    //4: compute proof signature
//    char ask[32];
//    size_t len;
//    sodium_hex2bin(ask,32,hexAttestSK,65,NULL,&len,NULL);
    char* strProof=cJSON_PrintUnformatted(proof);
    char sigProof[64];
    encrypt_sign(strProof,strlen(strProof),AttestSK,sigProof);
    memset(AttestSK,0,VKEY_SIG_SK_SIZE);

    char hexSigProof[129];
    sodium_bin2hex(hexSigProof,129,sigProof,64);

    cJSON *result = cJSON_CreateObject();
    cJSON_AddItemToObject(result,"claim",cJSON_Duplicate( claim,1));
    cJSON_AddItemToObject(result,"proof",cJSON_Duplicate(proof,1));
    cJSON_AddStringToObject(result,"signature",hexSigProof);


    //todo: 5 send transction

    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);

//    sprintf(topic,"%s/attest",peer->valuestring);
    //3 publish to mq
  //  mqtt_send(topic,jwt,strlen(jwt));
    char* pData = cJSON_PrintUnformatted(result);
    mqtt_send(strSourceTopic,"ATTEST_SRC",PK,SK,pData);


    free(pData);
    free(strProof);
    free(strClaim);

    cJSON_Delete(result);
    cJSON_Delete(json);

    http_response_text(nc,200,"Attestation post ok!");
}



/*
 {
    "claim":{
        "id":"40d27017ddf91223a7bfaf90b6a21ee2",
        "templateId":"CLMT_IDNUMBER"
    },
    "proof":{
        "md":"1212",
        "apk":"13432435",
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
    },
    "signature":1231231
}
 */
static int post_attest_self(struct mg_connection *nc, struct http_message *hm)
{

    cJSON *json = util_parseBody(&hm->body);
    cJSON *jClaim = cJSON_GetObjectItem(json, "claim");
    cJSON *jProof = cJSON_GetObjectItem(json, "proof");
    cJSON* jSignature = cJSON_GetObjectItem(json,"signature");

    if(!json)
    {
        http_response_error(nc,400,"Vkey Service : claim could not be parsed");
        return 0;
    }

    if(!jClaim)
    {
        http_response_error(nc,400,"Vkey Service : claim could not be parsed");
        return 0;
    }

    if(!jProof)
    {
        http_response_error(nc,400,"Vkey Service : proof could not be parsed");
        return 0;
    }

    if(!jSignature)
    {
        http_response_error(nc,400,"Vkey Service : signature could not be parsed");
        return 0;
    }


    cJSON* jCID = cJSON_GetObjectItem(jClaim,"id");
    cJSON* jTID = cJSON_GetObjectItem(jClaim,"templateId");


    int ret = attest_create(jCID->valuestring,jTID->valuestring,jProof,jSignature);
    if(ret==0)
    {
        http_response_text(nc,200,"Attestation post ok!");
    }
    else if(ret==-1)
    {
        http_response_error(nc,400,"claim is not exist!");
    }
    else if(ret==-2)
    {
        http_response_error(nc,400,"claim md is not match!");
    }

    cJSON_Delete(json);
    return 0;
}

cJSON* attest_read_by_claimid(const char* s_claimId,unsigned char* u_verifyMsg)
{
    sqlite3* db = db_get();

    char strSql[256];
    sprintf(strSql,"SELECT PROOF,SIGNATURE,RASK,TIME FROM TB_ATTEST WHERE CID=?;");

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
        char *strProof = (char *) sqlite3_column_text(pStmt, 0);
        char *strSignature = ( char *) sqlite3_column_text(pStmt, 1);
        unsigned char *RASK = (unsigned char *) sqlite3_column_blob(pStmt, 2);

        char strRASK[65];
        sodium_bin2hex(strRASK,65,RASK,VKEY_KEY_SIZE);

        cJSON* jItem = cJSON_CreateObject();
        //todo: descrypt data
        cJSON* jProof = cJSON_Parse(strProof);
        cJSON_AddItemToObject(jItem,"proof",jProof);
        cJSON_AddStringToObject(jItem,"signature",strSignature);

        if(u_verifyMsg)
        {
            unsigned char VERIFYSIG[VKEY_SIG_SIZE];
            encrypt_sign(u_verifyMsg, VKEY_KEY_SIZE, RASK, VERIFYSIG);

            char strVerifySig[129];
            sodium_bin2hex(strVerifySig, 129, VERIFYSIG, VKEY_SIG_SIZE);

            cJSON_AddStringToObject(jItem, "verify", strVerifySig);
        }
        cJSON_AddItemToArray(jResult,jItem);
    }
    sqlite3_finalize(pStmt);
    return jResult;
}

int attest_remove(const char* s_cid)
{
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"DELETE FROM TB_ATTEST WHERE CID='%s'",s_cid);
    return sqlite3_exec(db,strSql,NULL,NULL,NULL);
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
//int attest_replace_rask_with_verify(cJSON* jAttest,unsigned const char* u_msg)
//{
//    cJSON* jRASK = cJSON_GetObjectItem(jAttest,"rask");
//    if(!jRASK)
//    {
//        return -1;
//    }
//    char sk[VKEY_KEY_SIZE];
//    size_t len;
//    sodium_hex2bin(sk,VKEY_KEY_SIZE,jRASK->valuestring,64,NULL,&len,NULL);
//    if(len!=VKEY_KEY_SIZE)
//    {
//        return -1;
//    }
//    char sig[VKEY_SIG_SIZE];
//    if(0!=encrypt_sign(u_msg,VKEY_KEY_SIZE,sk,sig))
//    {
//        return -1;
//    }
//    char sigHex[VKEY_SIG_SIZE*2+1];
//    sodium_bin2hex(sigHex,VKEY_SIG_SIZE*2+1,sig,VKEY_SIG_SIZE);
//
//    cJSON_AddStringToObject(jAttest,"verify",sigHex);
//    cJSON_DeleteItemFromObject(jAttest,"rask");
//
//    return 0;
//}

///write proof to db
static int attest_write(const char* s_cid,const char* s_proof,const char* s_sig,const unsigned char* u_rask)
{
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_ATTEST (CID,PROOF,SIGNATURE,RASK,TIME) VALUES(?,?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    time_t nTime = time(NULL);
    sqlite3_bind_text(pStmt,1,s_cid,strlen(s_cid),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,s_proof,strlen(s_proof),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,3,s_sig,strlen(s_sig),SQLITE_TRANSIENT);
    sqlite3_bind_blob(pStmt,4,u_rask,VKEY_SIG_SK_SIZE,SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,5,nTime);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);

    return 0;
}

///local handle attest
static int attest_create(const char* s_cid,const char* s_tid, cJSON* j_proof,cJSON* j_signature)
{
    //todo:1 parse and descrypt jwt
    cJSON* jAPK = cJSON_GetObjectItem(j_proof,"apk");
    char* strProof = cJSON_PrintUnformatted(j_proof);


    unsigned char RASK[VKEY_SIG_SK_SIZE],RAPK[VKEY_KEY_SIZE];

    unsigned char RANDOM[32];
    encrypt_random(RANDOM);
    encrypt_makeSignPublic(RANDOM,RASK,RAPK);
    char strRAPK[65];
    sodium_bin2hex(strRAPK,65,RAPK,32);


    cJSON* jLocalClaim = claim_read_by_claimid(s_cid);
    int ret=-1;
    if(jLocalClaim)
    {

        char* strClaim=cJSON_PrintUnformatted(jLocalClaim);
        char claimHash[32];
        encrypt_hash(claimHash,strClaim,strlen(strClaim));

        char hexClaimHash[65];
        sodium_bin2hex(hexClaimHash,65,claimHash,32);
        cJSON* jMD = cJSON_GetObjectItem(j_proof,"md");
        if( 1||strcmp(hexClaimHash,jMD->valuestring)==0)
        {
            attest_write(s_cid,strProof,j_signature->valuestring,RASK);

            eth_attest_write(j_signature->valuestring,jAPK->valuestring,strRAPK,s_tid);
            ret=0;
        }
        else
        {
            ret=-2;
        }
        free(strClaim);
        cJSON_Delete(jLocalClaim);
    }
    else
    {
        ret=-1;
    }
    free(strProof);
    return ret;
}


int attest_got( const char* s_peerTopic, const char* s_data )
{
    printf("Got attestation data : %s \n", s_data);

    //todo:1 parse and descrypt jwt
    cJSON* jData = cJSON_Parse(s_data);
    cJSON* jClaim = cJSON_GetObjectItem(jData,"claim");
    cJSON* jProof = cJSON_GetObjectItem(jData,"proof");
    cJSON* jSignature = cJSON_GetObjectItem(jData,"signature");
    if(jClaim&&jProof&&jSignature)
    {


        cJSON *jCID = cJSON_GetObjectItem(jClaim, "id");
        cJSON *jTID = cJSON_GetObjectItem(jClaim, "templateId");

        int ret = attest_create(jCID->valuestring, jTID->valuestring, jProof, jSignature);
        if (ret == 0)
        {
            cJSON_AddStringToObject(jData, "state", "0:attest confirmed");
        }
        else if (ret == -1)
        {
            cJSON_AddStringToObject(jData, "state", "-1:claim is not exist");

        }
        else if (ret == -2)
        {
            cJSON_AddStringToObject(jData, "state", "-2:claim md is not match");
        }
    }
    else
    {

        cJSON_AddStringToObject(jData, "state", "-3:attest data invalid format");
    }


    char* strData = cJSON_PrintUnformatted(jData);

    g_notify(s_peerTopic,strData);

    free(strData);
    cJSON_Delete(jData);
    return 0;
}



int attest_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,3,nc,hm);
}