
#include <sodium.h>
#include "register.h"
#include "http.h"
#include "util.h"
#include "db.h"
#include "vkey.h"
#include "encrypt.h"
#include "eth.h"
#include "mqtt.h"
#include "key.h"
#include "start.h"

static int post_verify(struct mg_connection *nc, struct http_message *hm);
static int post_recover(struct mg_connection *nc, struct http_message *hm);
static int register_write(const char* s_rid,const char* s_url,const char* s_rpk,int n_state);
static int register_recover(const unsigned char* u_IUKOLD,const unsigned char* u_IMKOLD,const unsigned char* u_ILKNEW,const unsigned char* u_IMKNEW, const char* s_RID,const char* s_URL,const char* s_RPK);

static int register_build(const unsigned char* u_ILK,const unsigned char* u_IMK,const char* s_url,const char* s_rpk, char* s_ipk,unsigned char* u_isk,char* s_pid,char* s_suk,char* s_vuk);

static http_router routers[2]={
        {post_verify,"POST","/api/v1/register/verify"},
        {post_recover,"POST","/api/v1/register/recover"}
};


/*
{
 "iuk:"as84s8f7a8dfagyerwrg",
 "rescure":"11111"
 }
 */

static int post_verify(struct mg_connection *nc, struct http_message *hm)
{
    cJSON *json = util_parseBody(&hm->body);

    cJSON *rescureCode = cJSON_GetObjectItem(json, "rescure");
    if (!rescureCode)
    {
        http_response_error(nc, 400, "Vkey Service : rescure error");
        return 0;
    }
    cJSON *jCipherIUK = cJSON_GetObjectItem(json, "iuk");
    if (!jCipherIUK)
    {
        http_response_error(nc, 400, "Vkey Service : iuk error");
        return 0;
    }

    char *strSecure = rescureCode->valuestring;
    char *strCipherIUK = jCipherIUK->valuestring;
    if( key_checkIUK(strCipherIUK,strSecure)!=0 )
    {
        http_response_error(nc, 400, "Vkey Service : iuk or rescure wrong!");
    }
    else
    {
        http_response_text(nc, 200, "ok");
    }
    return 0;
}


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
 "rescure":"11111",
}
*/

/// recover for all register sites, the sites should subscribe recover topic
/// \param nc
/// \param hm
/// \return
static int post_recover(struct mg_connection *nc, struct http_message *hm)
{
    //todo 1: check db file

    cJSON *json = util_parseBody(&hm->body);

    //check params
    cJSON *jIUKOld = cJSON_GetObjectItem(json, "iuk");
    if(!jIUKOld)
    {
        http_response_error(nc,400,"Vkey Service : iuk error");
        return 0;
    }

    cJSON *JRescureOld = cJSON_GetObjectItem(json, "rescure");
    if(!JRescureOld)
    {
        http_response_error(nc,400,"Vkey Service : no rescureOld");
        return 0;
    }


    char* strIUKOld = jIUKOld->valuestring;
    char* strRescureOld = JRescureOld->valuestring;

    unsigned char IUKOLD[VKEY_KEY_SIZE];
    if(0!=key_descryptIUK(strIUKOld,strRescureOld,IUKOLD))
    {
        cJSON_Delete(json);

        http_response_error(nc,400,"Vkey Service : WRONG IUK OR RESCURE CODE");
        return 0;
    }

    cJSON_Delete(json);

    //compute OLD ilk,imk
    unsigned char ILKOLD[VKEY_KEY_SIZE];
    unsigned char IMKOLD[VKEY_KEY_SIZE];

    encrypt_makeDHPublic(IUKOLD,ILKOLD);
    encrypt_enHash((uint64_t *)IUKOLD,(uint64_t *)IMKOLD);

    //todo 2: read new IMK&ILK
    unsigned char ILK[VKEY_KEY_SIZE];
    unsigned char IMK[VKEY_KEY_SIZE];

    if(0!=key_get(IMK,ILK,NULL,NULL,NULL))
    {
        http_response_error(nc,400,"Vkey Service : no key");
        return 0;
    }

    //todo 3: read register recordset, descrypt

    sqlite3* db = db_get();
    if(!db)
    {
        http_response_error(nc,400,"vkey:has not db file");
        return -1;
    }
    char strSql[256];
    sprintf(strSql,"SELECT RID,URL,RPK FROM TB_REG_CLIENT WHERE STATE=1;");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return NULL;
    }

    //todo 4: start loop
    while( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strRID = (char *) sqlite3_column_text(pStmt, 0);
        char *strURL = (char *) sqlite3_column_text(pStmt, 1);
        char *strRPK = (char *) sqlite3_column_text(pStmt, 2);
        //todo: descrypt data

        register_recover(IUKOLD,IMKOLD,ILK,IMK,strRID,strURL,strRPK);
    }
    sqlite3_finalize(pStmt);

    //todo 5: finish loop

    //todo 6: start monitor block chain transaction state

    //todo 7: finish

    http_response_text(nc,200,"ok");
    return 0;

}


static int register_disable(const char* s_pid,int n_state)
{
    sqlite3* db = db_get();
    char strSql[256];
    sprintf(strSql,"UPDATE TB_REG_CLIENT SET STATE=? WHERE RID=?");
    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_bind_int(pStmt,1,n_state);
    sqlite3_bind_text(pStmt,2,s_pid,strlen(s_pid),SQLITE_TRANSIENT);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    sqlite3_finalize(pStmt);
    return 0;
}

static int register_recover(const unsigned char* u_IUKOLD,const unsigned char* u_IMKOLD,const unsigned char* u_ILKNEW,const unsigned char* u_IMKNEW, const char* s_PID,const char* s_URL,const char* s_RPK)
{
    //1: compute old ipk,isk,and old sig
    char ISKOLD[VKEY_KEY_SIZE];
    char IPKOLD[VKEY_KEY_SIZE];
    encrypt_hmac(u_IMKOLD,s_URL,strlen(s_URL),ISKOLD);
    encrypt_makeSignPublic(ISKOLD,IPKOLD);
    char strIPKOld[65];
    sodium_bin2hex(strIPKOld,65,IPKOLD,32);

    unsigned char OLDSIG[VKEY_SIG_SIZE];
    encrypt_sign(strIPKOld,strlen(strIPKOld),ISKOLD,OLDSIG);
    char hexOldSig[129];
    sodium_bin2hex(hexOldSig,129,OLDSIG,64);

    char strOldSUK[65];
    unsigned char OldSUK[VKEY_KEY_SIZE];
    //todo 4.1: get suk from bc by pid


    //2: compute ursk by suk & iuk

    unsigned char ILKOLD[VKEY_KEY_SIZE];
    encrypt_makeDHPublic(u_IUKOLD,ILKOLD);

    char URSK[VKEY_KEY_SIZE];
    encrypt_makeDHShareKey(u_IUKOLD,OldSUK,ILKOLD,URSK);

    unsigned char SIG[VKEY_SIG_SIZE];
    encrypt_sign(s_PID,strlen(s_PID),URSK,SIG);
    char hexSig[129];
    sodium_bin2hex(hexSig,129,SIG,64);



    //4: compute new ipk,Pid,vuk,suk

    char ISK[VKEY_KEY_SIZE];
    char strIPK[65];
    char strPID[65];
    char strSUK[65];
    char strVUK[65];
    if(0!=register_build(u_ILKNEW,u_IMKNEW,s_URL,s_RPK,strIPK,ISK,strPID,strSUK,strVUK))
    {
        return -1;
    }

    //4: send old rid,sign(rid,ursk),new rid,new vuk,new suk to recover smart contract
    eth_recover_client(s_PID,hexSig,strPID,s_RPK,strVUK,strSUK);

    //3 save reg info to db with sent state, when confirm info from site arrived, the reg info set to be confirmed
    register_disable(s_PID,1);
    register_write(strPID,s_URL,s_RPK,0);
    //todo 4.6: send old ipk,new ipk, signature of ipk by new isk to site


    unsigned char PK[VKEY_KEY_SIZE];
    unsigned char SK[VKEY_KEY_SIZE];

    encrypt_random(SK);
    encrypt_makeDHPublic(SK,PK);

    cJSON* jToSite = cJSON_CreateObject();
    cJSON_AddStringToObject(jToSite,"ipkNew",strIPK);
    cJSON_AddStringToObject(jToSite,"sigNew",hexSig);
    cJSON_AddStringToObject(jToSite,"ipkOld",strIPKOld);
    cJSON_AddStringToObject(jToSite,"sigOld",hexOldSig);
    char* pData = cJSON_PrintUnformatted(jToSite);

    mqtt_send(s_RPK,"RESTORE_DES",PK,SK,pData);

    free(pData);
    cJSON_Delete(jToSite);

    //todo 4.7: finish one site recover
    return 0;
}


int register_recover_got( const char* s_peerTopic, const char* s_data )
{

    printf("Auth From:%s\n",s_peerTopic);
    printf("Auth Message:%s\n",s_data);

    //handle register message
    cJSON* jData = cJSON_Parse(s_data);
    cJSON* jURL = cJSON_GetObjectItem(jData,"url");
    cJSON* jIpkNew = cJSON_GetObjectItem(jData,"ipkNew");
    cJSON* jSigNew = cJSON_GetObjectItem(jData,"sigNew");
    cJSON* jIpkOld = cJSON_GetObjectItem(jData,"ipkOld");
    cJSON* jSigOld = cJSON_GetObjectItem(jData,"sigOld");

    char strRPK[65];
    char strRSK[65];
    if(jURL && 0==register_getKeys(jURL->valuestring,strRPK,strRSK))
    {
        //todo: verify signature by isk

        char PIDOLD[VKEY_KEY_SIZE];
        char strSigRID[VKEY_SIG_SIZE];
        encrypt_hash(PIDOLD,jIpkOld->valuestring,strlen(jIpkOld->valuestring));
        char strPIDOLD[65];
        sodium_bin2hex(strPIDOLD,65,PIDOLD,VKEY_KEY_SIZE);

        //todo: compute RID and sig
    }

    char *pData = cJSON_PrintUnformatted(jData);

    g_notify(s_peerTopic,pData);

    free(pData);
    cJSON_Delete(jData);
    return 0;
}

static int register_build(const unsigned char* u_ILK,const unsigned char* u_IMK,const char* s_url,const char* s_rpk, char* s_ipk,unsigned char* u_isk,char* s_pid,char* s_suk,char* s_vuk)
{
    if(!u_ILK||!u_IMK||!s_url||!s_rpk||!s_ipk||!u_isk||!s_pid||!s_suk||!s_vuk)
    {
        return -1;
    }

    //ISK,IPK
    char IPK[VKEY_KEY_SIZE];
    encrypt_hmac(u_IMK,s_url,strlen(s_url),u_isk);
    encrypt_makeSignPublic(u_isk,IPK);
    sodium_bin2hex(s_ipk,65,IPK,VKEY_KEY_SIZE);

    //PID
    char PID[VKEY_KEY_SIZE];
    encrypt_hash(PID,IPK,VKEY_KEY_SIZE);
    sodium_bin2hex(s_pid,65,PID,VKEY_KEY_SIZE);

    //SUK
    char RLK[VKEY_KEY_SIZE];
    char SUK[VKEY_KEY_SIZE];
    encrypt_random(RLK);
    encrypt_makeDHPublic(RLK,SUK);
    sodium_bin2hex(s_suk,65,SUK,VKEY_KEY_SIZE);

    //VUK
    // signature by dhkey, can be verified by VUK, when recover identity, dh(iuk,suk),can be used as secret sign key,and vuk will be used as public sign key to verify signature.
    char DHKEY[VKEY_KEY_SIZE];
    char VUK[VKEY_KEY_SIZE];
    encrypt_makeDHShareKey(RLK,u_ILK,s_suk,DHKEY);
    encrypt_makeSignPublic(DHKEY,s_vuk);

    sodium_bin2hex(s_vuk,65,VUK,VKEY_KEY_SIZE);

    return 0;
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
    if(0!=key_get(IMK,ILK,NULL,NULL,NULL))
    {
        return -1;
    }
    char ISK[VKEY_KEY_SIZE];
    char strPID[65];
    char strSUK[65];
    char strVUK[65];
    if(0!=register_build(ILK,IMK,s_url,s_rpk,s_ipk,ISK,strPID,strSUK,strVUK))
    {
        return -1;
    }


    //IPK
//    char ISK[VKEY_KEY_SIZE];
//    char IPK[VKEY_KEY_SIZE];
//    encrypt_hmac(IMK,s_url,strlen(s_url),ISK);
//    encrypt_makeSignPublic(ISK,IPK);
//    memset(ISK,0,VKEY_KEY_SIZE);
//    memset(IMK,0,VKEY_KEY_SIZE);
//
//    //RID
//    char RID[VKEY_KEY_SIZE];
//    encrypt_hash(RID,IPK,VKEY_KEY_SIZE);
//
//    //SUK
//    char RLK[VKEY_KEY_SIZE];
//    char SUK[VKEY_KEY_SIZE];
//    encrypt_random(RLK);
//    encrypt_makeDHPublic(RLK,SUK);
//
//    //VUK
//    // signature by dhkey, can be verified by VUK, when recover identity, dh(iuk,suk),can be used as secret sign key,and vuk will be used as public sign key to verify signature.
//    char VUK[VKEY_KEY_SIZE];
//    char DHKEY[VKEY_KEY_SIZE];
//    encrypt_makeDHShareKey(RLK,ILK,SUK,DHKEY);
//    encrypt_makeSignPublic(DHKEY,VUK);

    //todo:2 SIGN(Nonce，RPK)

    //3 save reg info to db with sent state, when confirm info from site arrived, the reg info set to be confirmed
    register_write(strPID,s_url,s_rpk,0);


    //4 send to BC
    eth_register_client(strPID,s_rpk,strVUK,strSUK);


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
        //s_url has been a register record, nothing to do
    }
    else
    {
        unsigned char RPK[VKEY_KEY_SIZE];
        encrypt_random(RSK);
        encrypt_makeSignPublic(RSK, RPK);

        char strSK[65];
        sodium_bin2hex(strSK, 65, RSK, VKEY_KEY_SIZE);
        char strPK[65];
        sodium_bin2hex(strPK, 65, RPK, VKEY_KEY_SIZE);


        sqlite3 *db = db_get();


        char strSql[1024];
        sprintf(strSql, "INSERT INTO TB_REG_SITE (URL,SK,PK,TIME) VALUES(?,?,?,?)");

        sqlite3_stmt *pStmt;
        const char *strTail = NULL;
        int ret = sqlite3_prepare_v2(db, strSql, -1, &pStmt, &strTail);
        if (ret != SQLITE_OK)
        {
            sqlite3_finalize(pStmt);
            return -1;
        }

        //todo: encrypt data

        time_t nTime = time(NULL);
        sqlite3_bind_text(pStmt, 1, s_url, strlen(s_url), SQLITE_TRANSIENT);
        sqlite3_bind_text(pStmt, 2, strSK, strlen(strSK), SQLITE_TRANSIENT);
        sqlite3_bind_text(pStmt, 3, strPK, strlen(strPK), SQLITE_TRANSIENT);
        sqlite3_bind_int(pStmt, 4, nTime);

        ret = sqlite3_step(pStmt);
        if (ret != SQLITE_DONE)
        {
            sqlite3_finalize(pStmt);
            return -1;
        }
        sqlite3_finalize(pStmt);
        strcpy(s_rpk, strPK);

        mqtt_subscribe("RESTORE_DES",RPK,RSK,nTime,0,"");
    }
    //
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
    return http_routers_handle(routers,2,nc,hm);
}
