#ifndef CS_VKEY_REGISTER_H_
#define CS_VKEY_REGISTER_H_

#include "cjson/cJSON.h"
#include "mongoose/mongoose.h"

int register_route(struct mg_connection *nc, struct http_message *hm );

int register_create(const char* s_url,const char* s_rpk, char* s_ipk, unsigned char* u_isk);


int register_start(const char* s_url, char* s_rpk);
int register_getKeys(const char* s_url,unsigned char* u_rpk,unsigned char* u_rsk);
int register_recover_got( const char* s_peerTopic, const char* s_data );

#endif
