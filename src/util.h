
#include "cjson/cJSON.h"
#include "mongoose/mongoose.h"

#ifndef CS_VKEY_UTIL_H_
#define CS_VKEY_UTIL_H_

const char* util_getUUID();
cJSON* util_parseBody(struct mg_str* body);
char* util_getFromQuery(struct mg_str* s_query,const char* s_key);
#endif