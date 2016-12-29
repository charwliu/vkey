#ifndef CS_VKEY_KEY_H_
#define CS_VKEY_KEY_H_

#include "mongoose/mongoose.h"

int key_load();
int key_route(struct mg_connection *nc, struct http_message *hm );
const char* key_getAddress();

#endif
