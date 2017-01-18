#ifndef CS_VKEY_ATTEST_H_
#define CS_VKEY_ATTEST_H_

int attest_route(struct mg_connection *nc, struct http_message *hm );

int attest_got(const char* s_msg);

cJSON* attest_read_by_claimid(const char* s_claimId);

int attest_replace_rask_with_verify(cJSON* jAttest,const char* s_msg);

#endif