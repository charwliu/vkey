
#include "claim.h"
#include "mongoose/mongoose.h"
#include "auth.h"
#include "http.h"
#include "cjson/cJSON.h"
#include "libsodium/include/sodium.h"
#include "sqlite/sqlite3.h"
#include "db.h"
#include "util.h"
#include "attest.h"
#include "vkey.h"
#include "encrypt.h"

static int get_claim(struct mg_connection *nc, struct http_message *hm);
static int post_claim(struct mg_connection *nc, struct http_message *hm);
static int put_claim(struct mg_connection *nc, struct http_message *hm);
static int del_claim(struct mg_connection *nc, struct http_message *hm);
static int insert_claim(const char* s_id,const char * s_templateId,const char* s_data,int n_timec);
static int update_claim(const char* s_id,const char* s_data,int n_timeu);


static http_router routers[4]={
        {get_claim,"GET","/api/v1/claim"},
        {post_claim,"POST","/api/v1/claim"},
        {put_claim,"PUT","/api/v1/claim"},
        {del_claim,"DELETE","/api/v1/claim"}
};


/// /api/v1/claim?templateid=CLMT_NAME&nonce=123
static int get_claim(struct mg_connection *nc, struct http_message *hm) {

    char templateId[32]="";
    char nonce[64]="";
    mg_get_http_var(&hm->query_string, "templateId", templateId, 32);
    mg_get_http_var(&hm->query_string, "nonce", nonce, 64);

    unsigned char HashMsg[VKEY_KEY_SIZE];
    encrypt_hash(HashMsg,nonce,strlen(nonce));


    cJSON* jSend = cJSON_CreateObject();
    cJSON_AddStringToObject(jSend,"nonce",nonce);
    cJSON* jClaims = cJSON_CreateArray();

    claim_get_by_tid(templateId,HashMsg,jClaims);

    cJSON_AddItemToObject(jSend,"claims",jClaims);


    http_response_json(nc,200,jSend);

    cJSON_Delete(jSend);

}


static int post_claim(struct mg_connection *nc, struct http_message *hm) {

    cJSON *json = util_parseBody(&hm->body);
    if(!json)
    {
        http_response_error(nc,400,"Vkey Service : invalid json data");
        return 0;
    }
    //todo: varify json schema
    cJSON *templateId = cJSON_GetObjectItem(json, "templateId");
    if(!templateId)
    {
        http_response_error(nc,400,"Vkey Service : templateId error");
        return 0;
    }
    cJSON *jId = cJSON_GetObjectItem(json, "id");


    char strId[33];
    if( jId )
    {//has id
        strcpy(strId,jId->valuestring);
    }
    else
    {
        if (NULL == util_getUUID(strId, 33))
        {
            http_response_error(nc, 400, "Vkey Service : Could not generate uuid");
            return 0;
        }
        cJSON_AddStringToObject(json,"id",strId);
    }
    const char* strTemplateId=templateId->valuestring;

    time_t nTimeC = time(NULL);
    const char* strJson=cJSON_PrintUnformatted(json);

    //todo: write to sqlite
    int ret = insert_claim(strId,strTemplateId,strJson,nTimeC);

    if(ret==0)
    {
        http_response_text(nc,200,"Claim post ok!");
    }
    else
    {
        http_response_error(nc,500,"Claim write failed!");
    }
    free((void*)strJson);
    cJSON_Delete(json);
}


static int put_claim(struct mg_connection *nc, struct http_message *hm) {

    cJSON *json = util_parseBody(&hm->body);
    if(!json)
    {
        http_response_error(nc,400,"Vkey Service : invalid json data");
        return 0;
    }

    cJSON *templateId = cJSON_GetObjectItem(json, "templateId");
    if(!templateId)
    {
        http_response_error(nc,400,"Vkey Service : templateId error");
        return 0;
    }

    //todo: varify json schema
    cJSON *claimId = cJSON_GetObjectItem(json, "id");
    if(!claimId)
    {
        http_response_error(nc,400,"Vkey Service : claimId error");
        return 0;
    }

    const char* strClaimId=claimId->valuestring;


    cJSON* jClaim = claim_read_by_claimid(strClaimId);
    if(!jClaim)
    {
        http_response_error(nc,400,"Vkey Service : claim not exist");
        return 0;
    }
    cJSON_Delete(jClaim);

    time_t nTimeU = time(NULL);
    const char* strJson=cJSON_PrintUnformatted(json);


    int ret = update_claim(strClaimId,strJson,nTimeU);

    //todo: clear attestation record


    if(ret==0)
    {
        http_response_text(nc,200,"Claim update ok!");
    }
    else
    {
        http_response_error(nc,500,"Claim update failed!");
    }

    free((void*)strJson);
    cJSON_Delete(json);
}

/// DELETE HOST/claim?id=1233
/// \param nc
/// \param hm
/// \return
static int del_claim(struct mg_connection *nc, struct http_message *hm)
{

    char strID[64]="";

    if(0>=mg_get_http_var(&hm->query_string, "id", strID, 64))
    {
        http_response_error(nc,400,"Vkey Service : invalid id");
        return 0;
    }
    cJSON* jClaim = claim_read_by_claimid(strID);
    if(!jClaim)
    {
        http_response_error(nc,400,"Vkey Service : claim not exist");
        return 0;
    }
    cJSON_Delete(jClaim);

//
//    cJSON *json = util_parseBody(&hm->body);
//    if(!json)
//    {
//        http_response_error(nc,400,"Vkey Service : invalid json data");
//        return 0;
//    }
//    //todo: varify json schema
//    cJSON *claimId = cJSON_GetObjectItem(json, "claimId");
//    if(!claimId)
//    {
//        http_response_error(nc,400,"Vkey Service : claimId error");
//        return 0;
//    }
//    char* strID=claimId->valuestring;
    //todo: 1 check exist

    //todo: 2 delete auth and attestation records
    attest_remove(strID);

    //todo: 3 delete claim record
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"DELETE FROM TB_CLAIM WHERE ID='%s'",strID);
    sqlite3_exec(db,strSql,NULL,NULL,NULL);


//    sqlite3_stmt* pStmt;
//    const char* strTail=NULL;
//    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
//    if( ret != SQLITE_OK )
//    {
//        sqlite3_finalize(pStmt);
//        return -1;
//    }
//    sqlite3_bind_text(pStmt,1,strID,strlen(strID),SQLITE_TRANSIENT);
//
//    ret = sqlite3_step(pStmt);
//    if( ret != SQLITE_DONE )
//    {
//        sqlite3_finalize(pStmt);
//        return -1;
//    }
//    sqlite3_finalize(pStmt);




    http_response_text(nc,200,"veky: Claim deleted!");
}


static int insert_claim(const char* s_id,const char * s_templateId,const char* s_data,int n_timec)
{

    //todo: encrypt s_json

    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_CLAIM (ID,TID,DATA,TIMEC,TIMEU) VALUES(?,?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_bind_text(pStmt,1,s_id,strlen(s_id),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,s_templateId,strlen(s_templateId),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,3,s_data,strlen(s_data),SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,4,n_timec);
    sqlite3_bind_int(pStmt,5,0);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);

    return 0;
}



static int update_claim(const char* s_id,const char* s_data,int n_timeu)
{
    //read claim from db, if not exist ,return failed

    //if exist, check old value and new value, if equals return failed

    //update data and time



    //todo: encrypt s_json

    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"UPDATE TB_CLAIM SET DATA=?,TIMEU=? WHERE ID=?");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_bind_text(pStmt,1,s_data,strlen(s_data),SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,2,n_timeu);
    sqlite3_bind_text(pStmt,3,s_id,strlen(s_id),SQLITE_TRANSIENT);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);


    //remove attestation data

    return 0;
}


cJSON* claim_read_by_claimid(const char* s_id)
{
    sqlite3* db = db_get();

    char strSql[256];
    sprintf(strSql,"SELECT ID,DATA FROM TB_CLAIM WHERE ID=?;");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return NULL;
    }
    sqlite3_bind_text(pStmt, 1, s_id, strlen(s_id), SQLITE_TRANSIENT);

    if( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strID = (char *) sqlite3_column_text(pStmt, 0);
        char *strData = (char *) sqlite3_column_text(pStmt, 1);
        //todo: descrypt data
        cJSON* jData = cJSON_Parse(strData);
//        if(jData&&strID)
//        {
//            cJSON_AddStringToObject(jData,"id",strID);
//        }
        sqlite3_finalize(pStmt);
        return jData;

    }
    sqlite3_finalize(pStmt);
    return NULL;
}

/**
 * return value
 * ["data1","data2"]
 * */
int claim_get_by_tid(const char* s_templateId,unsigned const char* u_msg,cJSON* jClaims)
{
    //todo: descrypt s_json

    sqlite3* db = db_get();

    char strSql[256];

    if(s_templateId==NULL || strlen(s_templateId)==0)
    {
        sprintf(strSql,"SELECT ID,DATA FROM TB_CLAIM;");
    }
    else
    {
        sprintf(strSql,"SELECT ID,DATA FROM TB_CLAIM WHERE TID=?;");
    }

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    if(s_templateId==NULL || strlen(s_templateId)==0)
    {
        //nothing to do
    }
    else
    {
        sqlite3_bind_text(pStmt, 1, s_templateId, strlen(s_templateId), SQLITE_TRANSIENT);
    }

    while( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strID = (char *) sqlite3_column_text(pStmt, 0);


        cJSON* jClaim = claim_read_by_claimid(strID);
        cJSON* jAttests = attest_read_by_claimid(strID,u_msg);

        cJSON* jClaimWithAttest=cJSON_CreateObject();
        cJSON_AddItemToObject(jClaimWithAttest,"claim",jClaim);
        cJSON_AddItemToObject(jClaimWithAttest,"proofs",jAttests);
        cJSON_AddItemToArray( jClaims,jClaimWithAttest);

//
//        char *strData = (char *) sqlite3_column_text(pStmt, 1);
//        //todo: descrypt data
//        cJSON* jData = cJSON_Parse(strData);
//        if(jData&&strID)
//        {
//            cJSON_AddStringToObject(jData,"id",strID);
//        }
//        cJSON_AddItemToArray(result,jData);
        //cJSON_Delete(jData);
    }
    sqlite3_finalize(pStmt);
    return 0;
}


int claim_get_by_ids(cJSON* jClaimIds,unsigned const char* u_msg,cJSON* jClaims)
{
    int nClaimCount=cJSON_GetArraySize(jClaimIds);

    for( int i=0;i<nClaimCount;i++)
    {
        cJSON* jItem=cJSON_GetArrayItem(jClaimIds,i);

        cJSON* jClaim = claim_read_by_claimid(jItem->valuestring);
        cJSON* jAttests = attest_read_by_claimid(jItem->valuestring,u_msg);

        cJSON* jClaimWithAttest=cJSON_CreateObject();
        cJSON_AddItemToObject(jClaimWithAttest,"claim",jClaim);
        cJSON_AddItemToObject(jClaimWithAttest,"proofs",jAttests);
        cJSON_AddItemToArray( jClaims,jClaimWithAttest);
    }
    return 0;
}

int claim_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,4,nc,hm);
}