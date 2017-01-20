#ifndef CS_VKEY_CLAIM_H_
#define CS_VKEY_CLAIM_H_

#include "mongoose/mongoose.h"
#include "cjson/cJSON.h"

int claim_route(struct mg_connection *nc, struct http_message *hm );

int claim_read(const char* s_templateId,cJSON* result);


cJSON* claim_read_by_claimid(const char* s_id);
int claim_get_with_proofs(cJSON* jClaimIds,const char* sTopic,cJSON* jClaims);

#endif