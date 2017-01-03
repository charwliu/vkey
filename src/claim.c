
#include "claim.h"
#include "mongoose/mongoose.h"
#include "auth.h"
#include "http.h"
#include "cjson/cJSON.h"
#include "libsodium/include/sodium.h"
#include "sqlite/sqlite3.h"
#include "db.h"
#include "util.h"

static int get_claim(struct mg_connection *nc, struct http_message *hm);
static int post_claim(struct mg_connection *nc, struct http_message *hm);
static int write_claim(const char* s_id,const char * s_templateId,const char* s_data,int n_timec,int n_timeu);


static http_router routers[2]={
        {get_claim,"GET","/api/v1/claim"},
        {post_claim,"POST","/api/v1/claim"}
};

static void get_claimInfo(struct mg_connection *nc, struct http_message *hm) {

}

/// /api/v1/claim?templateid=CLMT_NAME
static int get_claim(struct mg_connection *nc, struct http_message *hm) {

    char templateId[32]="";
    mg_get_http_var(&hm->query_string, "templateId", templateId, 32);


    cJSON* res = cJSON_CreateArray();

    claim_read(templateId,res);

    http_response_json(nc,200,res);

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

    const char* strId=util_getUUID();
    const char* strTemplateId=templateId->valuestring;

    time_t nTimeC = time(NULL);
    const char* strJson=cJSON_PrintUnformatted(json);


    //todo: write to sqlite
    int ret = write_claim(strId,strTemplateId,strJson,nTimeC,nTimeC);

    if(ret==0)
    {
        http_response_text(nc,200,"Claim post ok!");
    }
    else
    {
        http_response_error(nc,500,"Claim write failed!");
    }
    free((void*)strJson);
    free((void*)strId);
    cJSON_Delete(json);
}



static void del_claim(struct mg_connection *nc, struct http_message *hm) {

}

static int write_claim(const char* s_id,const char * s_templateId,const char* s_data,int n_timec,int n_timeu)
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
    sqlite3_bind_int(pStmt,5,n_timeu);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);

    return 0;
}

/**
 * return value
 * ["data1","data2"]
 * */
int claim_read(const char* s_templateId,cJSON* result)
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
    }
    else
    {
        sqlite3_bind_text(pStmt, 1, s_templateId, strlen(s_templateId), SQLITE_TRANSIENT);
    }

    while( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strID = (char *) sqlite3_column_text(pStmt, 0);
        char *strData = (char *) sqlite3_column_text(pStmt, 1);
        cJSON* jData = cJSON_Parse(strData);
        if(jData&&strID)
        {
            cJSON_AddStringToObject(jData,"id",strID);
        }
        cJSON_AddItemToArray(result,jData);

    }
    sqlite3_finalize(pStmt);
    return 0;
}


int claim_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}