#ifndef CS_VKEY_KEY_H_
#define CS_VKEY_KEY_H_

#include "mongoose/mongoose.h"



int key_exist();
int key_route(struct mg_connection *nc, struct http_message *hm );
int key_create(const char* s_rescure,const char* s_password,unsigned char* h_random, char* s_ciperIuk );
int key_chechPassword(const char* s_password);

#endif
