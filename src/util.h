
#include "cjson/cJSON.h"
#include "mongoose/mongoose.h"

#ifndef CS_VKEY_UTIL_H_
#define CS_VKEY_UTIL_H_

const char*  util_getUUID(char* s_uuid,int n_size);
cJSON* util_parseBody(struct mg_str* body);
char* util_getFromQuery(struct mg_str* s_query,const char* s_key);
int util_strstr(struct mg_str* src, const char* des);
char* util_getStr(struct mg_str* msg);
char* util_getPk(const char* s_topic);
char** util_split(char* a_str, const char a_delim);

int util_compareKey(const unsigned char* u_a,const unsigned char* u_b,int n_size);

int util_addStringToSet(char** set,int n_size,char* item);



#endif