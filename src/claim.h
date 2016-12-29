#ifndef CS_VKEY_CLAIM_H_
#define CS_VKEY_CLAIM_H_

#include "mongoose/mongoose.h"
#include "cjson/cJSON.h"

int claim_route(struct mg_connection *nc, struct http_message *hm );

int claim_read(const char* s_templateId,cJSON* result);

#endif