#ifndef CS_VKEY_ETH_H_
#define CS_VKEY_ETH_H_

#include "mongoose/mongoose.h"

int eth_init(struct mg_mgr *mgr);

int eth_register(char* global_name, char* vid, char* pid, char* suk, char* vuk);

int eth_attest(char* from, char* to, char* pid, char* claimId, char* signature);

int eth_attest_write(const char* s_sig,const char* s_apk,const char* s_rapk,const char* s_tid);


int eth_register_client(const char* s_pid, const char* s_rpk, const char* s_vuk, const char* s_suk);
int eth_recover_client(const char* s_oldPid,const char* s_sigByURSK,const char* s_pid, const char* s_rpk, const char* s_vuk, const char* s_suk);

/// site submit register
/// \param RID
/// \param sigRID
/// \return
int eth_register_site(char* RID,char* sigRID);

#endif