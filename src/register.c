
#include <sodium.h>
#include "register.h"
#include "http.h"
#include "util.h"
#include "db.h"
#include "vkey.h"
#include "encrypt.h"
#include "eth.h"

static int post_recover(struct mg_connection *nc, struct http_message *hm);
static int register_write(const char* s_rid,const char* s_url,const char* s_rpk,int n_state);

static http_router routers[1]={
        {post_recover,"POST","/api/v1/register/recover"}
};

//
///*
// {
//  "topic":"as84s8f7a8dfagyerwrg",
//  "reg":{
//        "URL":"www.xxxx.cn",
//        "Nonce":2,
//        "RPK":555556,
//        "SessionId":1234555
//    }
//  }
// */
//static int post_register(struct mg_connection *nc, struct http_message *hm)
//{
//    cJSON *json = util_parseBody(&hm->body);
//    cJSON *jTopic = cJSON_GetObjectItem(json, "topic");
//    cJSON *jReg = cJSON_GetObjectItem(json, "reg");
//    if(!jReg)
//    {
//        http_response_error(nc,400,"Vkey Service : reg error");
//        return 0;
//    }
//    if(!jTopic)
//    {
//        http_response_error(nc,400,"Vkey Service : peer error");
//        return 0;
//    }
//
//    cJSON* jRet=cJSON_CreateObject();
//    if(0== register_create("","",jRet))
//    {
//        //todo: send jRet to site
//
//
//        http_response_text(nc,200,"ok");
//    }
//    else
//    {
//        http_response_error(nc,400,"Vkey Service : register failed");
//    }
//    cJSON_Delete(jRet);
//    cJSON_Delete(json);
//    return 0;
//}


/*
{
"iuk":"as84s8f7a8dfagyerwrg",
"rescure":"11111"
}
 */
/// recover for all register sites, the sites should subscribe recover topic
/// \param nc
/// \param hm
/// \return
static int post_recover(struct mg_connection *nc, struct http_message *hm)
{
    //todo 1: check db file

    //todo 2: descrypt iuk, compute old ilk,imk

    //todo 3: read register recordset, descrypt

    //todo 4: start loop

    //todo 4.1: compute old ipk, rid

    //todo 4.2: get suk from bc by rid

    //todo 4.3: compute ursk by suk & iuk

    //todo 4.4: compute new ipk,rid,vuk,suk

    //todo 4.5: send old rid,sign(rid,ursk),new rid,new vuk,new suk to recover smart contract

    //todo 4.6: send old ipk,new ipk, signature of ipk by new isk to site

    //todo 4.7: finish one site recover

    //todo 5: finish loop

    //todo 6: start monitor block chain transaction state

    //todo 7: finish

}


/// register to a site, compute keys and store register record to db, return ipk
/// \param j_reg
/// \return json to send to site
int register_create(const char* s_url,const char* s_rpk, char* s_ipk)
{
    //1 compute ipk,rid,vuk,suk
    char IMK[VKEY_KEY_SIZE];
    char ILK[VKEY_KEY_SIZE];
    //todo: read IMK & ILK

    //IPK
    char ISK[VKEY_KEY_SIZE];
    char IPK[VKEY_KEY_SIZE];
    encrypt_hmac(IMK,s_url,strlen(s_url),ISK);
    encrypt_makeSignPublic(ISK,IPK);
    memset(ISK,0,VKEY_KEY_SIZE);
    memset(IMK,0,VKEY_KEY_SIZE);

    //RID
    char RID[VKEY_KEY_SIZE];
    encrypt_hash(RID,IPK,VKEY_KEY_SIZE);

    //SUK
    char RLK[VKEY_KEY_SIZE];
    char SUK[VKEY_KEY_SIZE];
    encrypt_random(RLK);
    encrypt_makeDHPublic(RLK,SUK);

    //VUK
    // signature by dhkey, can be verified by VUK, when recover identity, dh(iuk,suk),can be used as secret sign key,and vuk will be used as public sign key to verify signature.
    char VUK[VKEY_KEY_SIZE];
    char DHKEY[VKEY_KEY_SIZE];
    encrypt_makeDHShareKey(RLK,ILK,SUK,DHKEY);
    encrypt_makeSignPublic(DHKEY,VUK);

    //todo:2 SIGN(Nonce，RPK)

    //3 save reg info to db with sent state, when confirm info from site arrived, the reg info set to be confirmed
    char strRID[65];
    sodium_bin2hex(strRID,65,RID,VKEY_KEY_SIZE);
    register_write(strRID,s_url,s_rpk,0);


    //4 send to BC
    char strVUK[65];
    sodium_bin2hex(strVUK,65,VUK,VKEY_KEY_SIZE);
    char strSUK[65];
    sodium_bin2hex(strSUK,65,SUK,VKEY_KEY_SIZE);
    eth_register_client(strRID,s_rpk,strVUK,strSUK);

    //6 return ipk
    sodium_bin2hex(s_ipk,65,IPK,VKEY_KEY_SIZE);

    return 0;
}

/// write register log which as client role to db file
/// \param s_rid
/// \param s_url
/// \param s_rpk
/// \param n_state
/// \return
static int register_write(const char* s_rid,const char* s_url,const char* s_rpk,int n_state)
{
    sqlite3* db = db_get();

    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_REG_CLIENT (RID,URL,RPK,TIME,STATE) VALUES(?,?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    //todo: encrypt data

    time_t nTime = time(NULL);
    sqlite3_bind_text(pStmt,1,s_rid,strlen(s_rid),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,s_url,strlen(s_url),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,3,s_rpk,strlen(s_rpk),SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,4,nTime);
    sqlite3_bind_int(pStmt,5,n_state);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);
    return 0;
}

/// save register info, used by site when site require register
/// \param s_url
/// \param s_rpk return rpk
/// \param s_rsk
/// \return
int register_start(const char* s_url, char* s_rpk)
{
    unsigned char RSK[VKEY_KEY_SIZE];
    if(0==register_getKeys(s_url,s_rpk,RSK))
    {
        return 0;
    }

    unsigned char RPK[VKEY_KEY_SIZE];
    encrypt_random(RSK);
    encrypt_makeSignPublic(RSK,RPK);

    char strSK[65];
    sodium_bin2hex(strSK,65,RSK,VKEY_KEY_SIZE);
    char strPK[65];
    sodium_bin2hex(strPK,65,RPK,VKEY_KEY_SIZE);


    sqlite3* db = db_get();


    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_REG_SITE (URL,SK,PK,TIME) VALUES(?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    //todo: encrypt data

    time_t nTime = time(NULL);
    sqlite3_bind_text(pStmt,1,s_url,strlen(s_url),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,strSK,strlen(strSK),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,3,strPK,strlen(strPK),SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,4,nTime);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);
    strcpy(s_rpk,strPK);

    return 0;
}

int register_getKeys(const char* s_url,char* s_rpk,char* s_rsk)
{
    sqlite3* db = db_get();

    char strSql[256];
    sprintf(strSql,"SELECT PK,SK FROM TB_REG_SITE WHERE URL=?;");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return NULL;
    }
    sqlite3_bind_text(pStmt, 1, s_url, strlen(s_url), SQLITE_TRANSIENT);

    if( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strRPK = (char *) sqlite3_column_text(pStmt, 0);
        char *strRSK = (char *) sqlite3_column_text(pStmt, 1);
        //todo: descrypt data

        strcpy(s_rpk,strRPK);
        strcpy(s_rsk,strRSK);
        sqlite3_finalize(pStmt);
        return 0;

    }
    sqlite3_finalize(pStmt);
    return -1;
}

int register_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}