
#include "key.h"
#include "http.h"
#include "sqlite/sqlite3.h"
#include "db.h"
#include "cjson/cJSON.h"
#include "config.h"
#include "encrypt.h"
#include "libsodium/include/sodium.h"
#include "util.h"
#include "vkey.h"

static int post_key(struct mg_connection *nc, struct http_message *hm );

static http_router routers[1]={
        {post_key,"POST","/api/v1/key"}
};


static char key_address[VKEY_KEY_SIZE*2+1]="";



static int write_key(const char* s_address,const char * s_imk,const char* s_ilk,const char* s_tag,int n_time)
{
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_KEY (ADDRESS,IMK,ILK,TAG,TIME) VALUES(?,?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_bind_text(pStmt,1,s_address,-1,SQLITE_TRANSIENT);
    sqlite3_bind_blob(pStmt,2,s_imk,32,SQLITE_TRANSIENT);
    sqlite3_bind_blob(pStmt,3,s_ilk,32,SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,4,s_tag,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,5,n_time);


    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);

    return 0;
}


/*
 {
    "rescure":"rescure code",
    "password":"123"
 }
 * */
/// request by secure code and main password hash , saved ilk,encrytped imk,create address, return encryped iuk,address
static int post_key(struct mg_connection *nc, struct http_message *hm )
{
    if( strlen(key_address)>0 )
    {
        http_response_error(nc,400,"Vkey Service : veky exist");
        return 0;
    }

    cJSON *json = util_parseBody(&hm->body);

    cJSON *rescureCode = cJSON_GetObjectItem(json, "rescure");
    if(!rescureCode)
    {
        http_response_error(nc,400,"Vkey Service : rescure error");
        return 0;
    }
    cJSON *password = cJSON_GetObjectItem(json, "password");
    if(!password)
    {
        http_response_error(nc,400,"Vkey Service : password error");
        return 0;
    }

    char* strSecure = rescureCode->valuestring;
    char* strPassword = password->valuestring;

    //1:generate keys
    unsigned char IUK[VKEY_KEY_SIZE];
    unsigned char ILK[VKEY_KEY_SIZE];
    unsigned char IMK[VKEY_KEY_SIZE];
    unsigned char ISK[VKEY_KEY_SIZE];
    unsigned char ADDRESS[VKEY_KEY_SIZE];
    unsigned char AUTHTAG[VKEY_KEY_SIZE+1];

    //IUK,ILK,IMK
    encrypt_random(IUK);
    encrypt_makeDHPublic(IUK,ILK);
    encrypt_enHash((uint64_t *)IUK,(uint64_t *)IMK);

    encrypt_hmac(IMK,g_config.sde_url,strlen(g_config.sde_url),ISK);
    encrypt_makeSignPublic(ISK,ADDRESS);
    memset(ISK,0,VKEY_KEY_SIZE);
    sodium_bin2hex(key_address,65,ADDRESS,VKEY_KEY_SIZE);


    //IUK使用救援码的HASH进行加密
    unsigned char hashRescure[VKEY_KEY_SIZE];
    encrypt_hash(hashRescure,strSecure,strlen(strSecure));
    unsigned char cipherIUK[VKEY_KEY_SIZE];
    encrypt_encrypt(cipherIUK,IUK,32,hashRescure,NULL,NULL,0,NULL);
    memset(IUK,0,VKEY_KEY_SIZE);

    //IMK使用密码的HASH进行加密，并生成16位校验tag，解密时用于确认密码是否正确
    unsigned char hashPassword[VKEY_KEY_SIZE];
    encrypt_hash(hashPassword,strPassword,strlen(strPassword));
    unsigned char cipherIMK[VKEY_KEY_SIZE];
    unsigned char authTag[16];
    encrypt_encrypt(cipherIMK,IMK,32,hashPassword,NULL,NULL,0,authTag);
    sodium_bin2hex(AUTHTAG,33,authTag,16);

    cJSON_Delete(json);

    //2:write to db
    time_t nTime = time(NULL);
    int ret = write_key(key_address,IMK,ILK,AUTHTAG,nTime);
    if(ret!=0)
    {
        http_response_text(nc,400,"Vkey Service : Write key error");
        return 0;
    }


    //4:send response
    cJSON* res = cJSON_CreateObject();
    char hexIUK[66];

    sodium_bin2hex(hexIUK,66,cipherIUK,VKEY_KEY_SIZE);

    cJSON_AddStringToObject(res,"iuk",hexIUK);

    http_response_json(nc,200,res);

    cJSON_Delete(res);
    return 0;
}

/// load address imk from table TB_KEY
int key_load()
{
    sqlite3* db = db_get();

    char strSql[256];

    sprintf(strSql,"SELECT ADDRESS FROM TB_KEY LIMIT 1;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        printf("No key!");
        sqlite3_finalize(pStmt);
        return -1;
    }


    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_ROW )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    char *strADDRESS = (char *) sqlite3_column_text(pStmt, 0);
    memcpy(key_address,strADDRESS,strlen(strADDRESS));

    printf("Address:%s\n",key_address);

    sqlite3_finalize(pStmt);

    return 0;
}

int key_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}

const char* key_getAddress()
{
    return key_address;
}


const char* key_getAttestSK()
{
    return key_address;
}

const char* key_getAttestPK()
{
    return key_address;
}