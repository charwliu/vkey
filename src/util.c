
#include "util.h"
#include "libsodium/include/sodium.h"

/// need free return value
const char* util_getUUID()
{
    unsigned char random[32];
    randombytes_buf(random,	32);

    unsigned char hash[16];
    crypto_generichash(hash, sizeof	hash, random, 16, NULL,	0);


    unsigned char *hex=(unsigned char*)malloc(33);
    return sodium_bin2hex(hex,33,hash,16);
}


cJSON* util_parseBody(struct mg_str* body)
{
    char* temp=malloc(body->len+1);
    memcpy(temp,body->p,body->len);
    temp[body->len]=0;

    cJSON *json = cJSON_Parse(temp);
    memset(temp,0,body->len+1);
    free(temp);
    return json;
}

/*
 * s_query:name=abc&value=123
 * s_key:value
 * return:123
 *
 * */
char* util_getFromQuery(struct mg_str* s_query,const char* s_key)
{
    c_strnstr(s_query->p,s_key,s_query->len);

    return "todo";
}