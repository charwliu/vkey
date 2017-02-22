
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
static int post_password(struct mg_connection *nc, struct http_message *hm );

static http_router routers[2]={
        {post_password,"POST","/api/v1/key/password"},
        {post_key,"POST","/api/v1/key"}
};


//static char key_address[VKEY_KEY_SIZE*2+1]="";



static int write_key(const char* s_apk,const char* s_ask, const char * s_imk,const char* s_ilk,const char* s_tag,int n_time)
{
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_KEY (APK,ASK,IMK,ILK,TAG,TIME) VALUES(?,?,?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_bind_text(pStmt,1,s_apk,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,s_ask,-1,SQLITE_TRANSIENT);
    sqlite3_bind_blob(pStmt,3,s_imk,32,SQLITE_TRANSIENT);
    sqlite3_bind_blob(pStmt,4,s_ilk,32,SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,5,s_tag,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(pStmt,6,n_time);


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
    "old":"oldpassword",
    "new":"newpassword"
 }
 * */
/// request by secure code and main password hash , saved ilk,encrytped imk,create address, return encryped iuk,address
static int post_password(struct mg_connection *nc, struct http_message *hm )
{
    if( key_exist()==0 )
    {
        http_response_error(nc,400,"Vkey Service : veky not exist");
        return 0;
    }

    cJSON *json = util_parseBody(&hm->body);

    cJSON *jOld = cJSON_GetObjectItem(json, "old");
    if(!jOld)
    {
        http_response_error(nc,400,"Vkey Service : old password error");
        return 0;
    }

    cJSON *jNew = cJSON_GetObjectItem(json, "new");
    if(!jNew)
    {
        http_response_error(nc,400,"Vkey Service : new password error");
        return 0;
    }


    //read old and check
    sqlite3* db = db_get();

    char strSql[256];

    sprintf(strSql,"SELECT IMK,TAG FROM TB_KEY LIMIT 1;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        printf("No key!");
        sqlite3_finalize(pStmt);
        return 0;
    }


    ret = sqlite3_step(pStmt);
    int result=-1;
    unsigned char IMK[VKEY_KEY_SIZE];
    unsigned char* cipherIMK ;
    if( ret == SQLITE_ROW )
    {
        int size = sqlite3_column_bytes(pStmt,0);
        cipherIMK = sqlite3_column_text(pStmt,0);
        char* strTag = sqlite3_column_text(pStmt,1);

        unsigned char hashPassword[VKEY_KEY_SIZE];
        encrypt_hash(hashPassword,jOld->valuestring,strlen(jOld->valuestring));

        unsigned char authTag[16];
        size_t len;
        sodium_hex2bin(authTag,16,strTag,32,NULL,&len,NULL);
        result = encrypt_decrypt(IMK,cipherIMK,32,hashPassword,NULL,NULL,0,authTag);
    }

    sqlite3_finalize(pStmt);
    if( result!=0 )
    {
        cJSON_Delete(json);
        http_response_error(nc,400,"Vkey Service : wrong old password");
        return 0;
    }


    //write new ciper imk and tag to db
    unsigned char hashPassword[VKEY_KEY_SIZE];
    encrypt_enScrypt(hashPassword,jNew->valuestring,strlen(jNew->valuestring),"salt");
    unsigned char cipherNewIMK[VKEY_KEY_SIZE];
    unsigned char authTag[16];
    unsigned char AUTHTAG[VKEY_KEY_SIZE+1];
    encrypt_encrypt(cipherNewIMK,IMK,32,hashPassword,NULL,NULL,0,authTag);
    sodium_bin2hex(AUTHTAG,33,authTag,16);

    cJSON_Delete(json);

    sprintf(strSql,"UPDATE TB_KEY SET IMK=?,TAG=? WHERE 1=1");

    ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        http_response_error(nc,400,"Vkey Service : write new password error");
        return 0;
    }
    sqlite3_bind_blob(pStmt,1,cipherNewIMK,32,SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,AUTHTAG,strlen(AUTHTAG),SQLITE_TRANSIENT);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        http_response_error(nc,400,"Vkey Service : write new password error");
        return 0;
    }

    sqlite3_finalize(pStmt);
    http_response_error(nc,200,"ok");
    return 0;

}

int key_descryptIUK(const char* s_ciperIUK,const char* s_rescure,unsigned char* u_iuk)
{
    //descrypt IUK
    unsigned char CIPERIUK[VKEY_KEY_SIZE];
    size_t len;
    sodium_hex2bin(CIPERIUK,32,s_ciperIUK,64,NULL,&len,NULL);

    unsigned char hashPassword[VKEY_KEY_SIZE];
    encrypt_enScrypt(hashPassword,s_rescure,strlen(s_rescure),"salt");

    unsigned char IUK[VKEY_KEY_SIZE];
    int ret = encrypt_decrypt(IUK,CIPERIUK,32,hashPassword,NULL,NULL,0,NULL);
    if( ret !=0 )
    {
        return -1;
    }
    if(u_iuk)
    {
        memcpy(u_iuk,IUK,VKEY_KEY_SIZE);
    }
    return 0;
}


int key_checkIUK(const char* s_ciperIUK,const char* s_rescure)
{
    unsigned char IUK[VKEY_KEY_SIZE];

    if(0!=key_descryptIUK(s_ciperIUK,s_rescure,IUK))
    {
        return -1;
    }


    unsigned char ILK[VKEY_KEY_SIZE];
    encrypt_makeDHPublic(IUK,ILK);

    //read old ILK

    sqlite3* db = db_get();
    char strSql[256];

    sprintf(strSql,"SELECT ILK FROM TB_KEY LIMIT 1;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    unsigned char* ILKOLD=NULL;
    ret = sqlite3_step(pStmt);

    if( ret == SQLITE_ROW )
    {
        int size = sqlite3_column_bytes(pStmt,0);
        ILKOLD = sqlite3_column_text(pStmt,0);
    }

    ret = (util_compareKey(ILK,ILKOLD,VKEY_KEY_SIZE)==1)?0:-1;
    sqlite3_finalize(pStmt);

    return ret;
}

/*
 {
    "rescure":"rescure code",
    "password":"123",
    "random":"hexdata"
 }
 * */
/// request by secure code and main password hash , saved ilk,encrytped imk,create address, return encryped iuk,address
static int post_key(struct mg_connection *nc, struct http_message *hm )
{
    if( key_exist()==1 )
    {
        http_response_error(nc,400,"Vkey Service : veky exist");
        return 0;
    }

    cJSON *json = util_parseBody(&hm->body);

    cJSON *jRescureCode = cJSON_GetObjectItem(json, "rescure");
    if(!jRescureCode)
    {
        http_response_error(nc,400,"Vkey Service : rescure error");
        return 0;
    }
    cJSON *jPassword = cJSON_GetObjectItem(json, "password");
    if(!jPassword)
    {
        http_response_error(nc,400,"Vkey Service : password error");
        return 0;
    }

    cJSON *jRandom = cJSON_GetObjectItem(json, "random");
    if(!jRandom)
    {
        http_response_error(nc,400,"Vkey Service : random error");
        return 0;
    }
    char ciperIUK[65];

    if(0==key_create(jRescureCode->valuestring,jPassword->valuestring,jRandom->valuestring,ciperIUK))
    {
        cJSON* res = cJSON_CreateObject();
        cJSON_AddStringToObject(res,"iuk",ciperIUK);

        http_response_json(nc,200,res);

        cJSON_Delete(res);

    }
    else
    {
        http_response_error(nc,400,"Vkey Service : create key error");
    }
    return 0;
}

int key_create(const char* s_rescure,const char* s_password,unsigned char* h_random, char* s_ciperIuk )
{
    if( key_exist()==1 )
    {
        return -1;
    }


    if(!s_rescure||!s_password||!h_random||!s_ciperIuk)
    {
        return -1;
    }


    //1:generate keys
    unsigned char IUK[VKEY_KEY_SIZE];
    unsigned char ILK[VKEY_KEY_SIZE];
    unsigned char IMK[VKEY_KEY_SIZE];
    unsigned char ASK[VKEY_KEY_SIZE];
    unsigned char APK[VKEY_KEY_SIZE];
    unsigned char AUTHTAG[VKEY_KEY_SIZE+1];

    //IUK,ILK,IMK

    //IUK=hash(random+salt)
    unsigned char RAND_SALT[VKEY_KEY_SIZE];

    encrypt_random(RAND_SALT);
    char strRandSalt[65];
    sodium_bin2hex(strRandSalt,65,RAND_SALT,VKEY_KEY_SIZE);
    char *strRandom=malloc(strlen(h_random)+64+1);
    strcpy(strRandom,h_random);
    strcat(strRandom,strRandSalt);

    encrypt_hash(IUK,strRandom,strlen(strRandom));
    free(strRandom);

    //ILK
    encrypt_makeDHPublic(IUK,ILK);

    //IMK
    encrypt_enHash((uint64_t *)IUK,(uint64_t *)IMK);

    //APK,ASK
    encrypt_hmac(IMK,g_config.mqtt_url,strlen(g_config.mqtt_url),ASK);
    encrypt_makeSignPublic(ASK,APK);

    char strAPK[65];
    char strASK[65];
    sodium_bin2hex(strAPK,65,APK,VKEY_KEY_SIZE);
    sodium_bin2hex(strASK,65,ASK,VKEY_KEY_SIZE);


    //IUK使用救援码的enScrypt进行加密
    unsigned char hashRescure[VKEY_KEY_SIZE];
    encrypt_enScrypt(hashRescure,s_rescure,strlen(s_rescure),"salt");

    unsigned char cipherIUK[VKEY_KEY_SIZE];
    encrypt_encrypt(cipherIUK,IUK,32,hashRescure,NULL,NULL,0,NULL);
    memset(IUK,0,VKEY_KEY_SIZE);

    //IMK使用密码的enCrypt进行加密，并生成16位校验tag，解密时用于确认密码是否正确
    unsigned char hashPassword[VKEY_KEY_SIZE];
    encrypt_enScrypt(hashPassword,s_password,strlen(s_password),"salt");
    unsigned char cipherIMK[VKEY_KEY_SIZE];
    unsigned char authTag[16];
    encrypt_encrypt(cipherIMK,IMK,32,hashPassword,NULL,NULL,0,authTag);
    sodium_bin2hex(AUTHTAG,33,authTag,16);


    //2:write to db
    time_t nTime = time(NULL);
    int ret = write_key(strAPK,strASK,cipherIMK,ILK,AUTHTAG,nTime);
    if(ret!=0)
    {
        return -1;
    }

    sodium_bin2hex(s_ciperIuk,65,cipherIUK,VKEY_KEY_SIZE);

    return 0;
}

static int key_remove()
{
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"DELETE FROM TB_KEY WHERE 1=1");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);

    return 0;

}

static int key_read(unsigned char* u_imk, unsigned char* u_ilk)
{
    //TODO:
    return 0;
}

int key_update(const char* s_ciperOldIUK,const char* s_oldRescure, const char* s_rescure,const char* s_password,const char* s_random ,char* s_ciperIUK)
{
    //checkIUK
    if (0 != key_checkIUK(s_ciperOldIUK,s_oldRescure))
    {
        return -1;
    }

    //read old imk
    unsigned char ILKOLD[VKEY_KEY_SIZE];
    unsigned char IMKOLD[VKEY_KEY_SIZE];


    if(0!=key_read(IMKOLD,ILKOLD))
    {
        return -1;
    }

    //remove old key data
    if(0!=key_remove())
    {
        return -1;
    }
    //create new key data
    key_create(s_rescure,s_password,s_random,s_ciperIUK);

    unsigned char ILKNEW[VKEY_KEY_SIZE];
    unsigned char IMKNEW[VKEY_KEY_SIZE];

    if(0!=key_read(IMKNEW,ILKNEW))
    {
        return -1;
    }

    //todo: descrypt tables data with old imk and reencrypt them with new imk
    db_reEncrypt(IMKOLD,IMKNEW);

    return 0;
}

/// load address imk from table TB_KEY
int key_exist()
{
    sqlite3* db = db_get();

    char strSql[256];

    sprintf(strSql,"SELECT APK FROM TB_KEY LIMIT 1;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        printf("No key!");
        sqlite3_finalize(pStmt);
        return 0;
    }


    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_ROW )
    {
        sqlite3_finalize(pStmt);
        return 0;
    }

    sqlite3_finalize(pStmt);

    return 1;
}

int key_chechPassword(const char* s_password)
{
    sqlite3* db = db_get();

    char strSql[256];

    sprintf(strSql,"SELECT IMK,TAG FROM TB_KEY LIMIT 1;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        printf("No key!");
        sqlite3_finalize(pStmt);
        return 0;
    }


    ret = sqlite3_step(pStmt);
    int result=-1;
    if( ret == SQLITE_ROW )
    {
        int size = sqlite3_column_bytes(pStmt,0);
        unsigned char* cipherIMK = sqlite3_column_text(pStmt,0);
        char* strTag = sqlite3_column_text(pStmt,1);

        unsigned char hashPassword[VKEY_KEY_SIZE];
        encrypt_hash(hashPassword,s_password,strlen(s_password));

        unsigned char authTag[16];
        size_t len;
        sodium_hex2bin(authTag,16,strTag,32,NULL,&len,NULL);
        unsigned char IMK[VKEY_KEY_SIZE];
        result = encrypt_decrypt(IMK,cipherIMK,32,hashPassword,NULL,NULL,0,authTag);
        memset(IMK,0,32);
    }

    sqlite3_finalize(pStmt);

    return result;
}

int key_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}
