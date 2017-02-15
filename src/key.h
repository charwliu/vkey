#ifndef CS_VKEY_KEY_H_
#define CS_VKEY_KEY_H_

#include "mongoose/mongoose.h"



int key_exist();
int key_route(struct mg_connection *nc, struct http_message *hm );


#endif
