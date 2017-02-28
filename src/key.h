#ifndef CS_VKEY_KEY_H_
#define CS_VKEY_KEY_H_

#include "mongoose/mongoose.h"



int key_exist();
int key_route(struct mg_connection *nc, struct http_message *hm );
int key_create(const char* s_rescure,const char* s_password,unsigned char* h_random, char* s_ciperIuk );
int key_update(const char* s_ciperOldIUK,const char* s_oldRescure, const char* s_rescure,const char* s_password,const char* s_random ,char* s_ciperIUK);

int key_chechPassword(const char* s_password);
int key_checkIUK(const char* s_ciperIUK,const char* s_rescure);
int key_descryptIUK(const char* s_ciperIUK,const char* s_rescure,unsigned char* u_iuk);
int key_get(unsigned char* u_imk, unsigned char* u_ilk, char* s_apk, unsigned char* u_ask, char* s_tag);

#endif
