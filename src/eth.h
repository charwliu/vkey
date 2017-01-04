#ifndef CS_VKEY_ETH_H_
#define CS_VKEY_ETH_H_

#include "mongoose/mongoose.h"

int eth_init(struct mg_mgr *mgr);

int eth_register(char* global_name, char* vid, char* pid, char* suk, char* vuk);

int eth_attest(char* from, char* to, char* pid, char* claimId, char* signature);

#endif