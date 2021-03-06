#ifndef CS_VKEY_START_H_
#define CS_VKEY_START_H_

// function pointer type for notify when sdk got some message
typedef int (*FN_Notify)(const char*,const char*) ;

/// init vkey,must be called first
/// \return
int init_key();

/// create key
/// \param s_dbPath  db file path,each file means a identity
/// \param s_rescure    rescure code
/// \param s_password   main password of key
/// \param s_random     random to generate iuk
/// \param s_ciperIUK   out,return encrypted IUK, client should store it offline
/// \return 0:success,-1:failed
int create_key(const char* s_dbPath,const char* s_password, const char* s_rescure,const char* s_random ,char* s_ciperIUK);

///
/// \param s_dbPath
/// \param s_ciperOldIUK    iuk backuped offline
/// \param s_oldRescure     rescure code backuped offline
/// \param s_password
/// \param s_rescure
/// \param s_random
/// \param s_ciperIUK
/// \return
int update_key(const char* s_dbPath,const char* s_ciperOldIUK,const char* s_oldRescure,const char* s_password, const char* s_rescure,const char* s_random ,char* s_ciperIUK);

///
/// \param s_dbPath
/// \param s_ciperOldIUK
/// \param s_oldRescure
/// \return
int verify_iuk(const char* s_dbPath,const char* s_ciperOldIUK,const char* s_oldRescure);

///
/// \param s_dbPath  db file path,each file means a identity
/// \param s_password  key main password, same as input param password in create_key
/// \param s_port    service port
/// \param fn_notify callback function
/// \param s_callbackUrl callback url
/// \return          0:start successful, -1:start failed
int start_vkey(const char *s_dbPath,const char* s_password, const char *s_port,  FN_Notify fn_notify, const char *s_callbackUrl);


int g_notify(const char* s_topic,const char* s_msg);


int start_route(struct mg_connection *nc, struct http_message *hm );

#endif