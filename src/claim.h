#ifndef CS_VKEY_CLAIM_H_
#define CS_VKEY_CLAIM_H_

#include "mongoose/mongoose.h"
#include "cjson/cJSON.h"

int claim_route(struct mg_connection *nc, struct http_message *hm );

int claim_get_by_tid(const char* s_templateId,unsigned const char* u_msg,cJSON* jClaims);


cJSON* claim_read_by_claimid(const char* s_id);
int claim_get_by_ids(cJSON* jClaimIds,unsigned const char* u_msg,cJSON* jClaims);

#endif