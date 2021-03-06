#ifndef CS_VKEY_ATTEST_H_
#define CS_VKEY_ATTEST_H_

#include "cjson/cJSON.h"
int attest_route(struct mg_connection *nc, struct http_message *hm );
int post_attest(struct mg_connection *nc, struct http_message *hm);


int attest_got( const char* s_peerTopic, const char* s_data );

cJSON* attest_read_by_claimid(const char* s_claimId,unsigned char* u_verifyMsg);

int attest_remove(const char* s_cid);

int got_attest_fromBC(int n_result);

//int attest_replace_rask_with_verify(cJSON* jAttest,unsigned const char* u_msg);

#endif