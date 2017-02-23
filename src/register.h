#ifndef CS_VKEY_REGISTER_H_
#define CS_VKEY_REGISTER_H_

#include "cjson/cJSON.h"
#include "mongoose/mongoose.h"

int register_route(struct mg_connection *nc, struct http_message *hm );

int register_create(const char* s_url,const char* s_rpk, char* s_ipk, unsigned char* u_isk);


int register_start(const char* s_url, char* s_rpk);
int register_getKeys(const char* s_url,char* s_rpk,char* s_rsk);

#endif
