#ifndef CS_VKEY_AUTH_H_
#define CS_VKEY_AUTH_H_

#include "mongoose/mongoose.h"
int auth_route(struct mg_connection *nc, struct http_message *hm );

int auth_got( const char* s_myTopic, const char* s_data );

#endif