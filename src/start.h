#ifndef CS_VKEY_START_H_
#define CS_VKEY_START_H_

// function pointer type for notify when sdk got some message
typedef int (*FN_Notify)(const char*,const char*) ;

///
/// \param s_dbPath  db file path,each file means a identity
/// \param s_port    service port
/// \param fn_notify callback function
/// \param s_callbackUrl callback url
/// \return          0:start successful, -1:start failed
//int start_vkey(FN_Notify fn_notify,const char* s_dbPath,const char* s_port);
int start_vkey(const char* s_dbPath,const char* s_port,FN_Notify fn_notify,const char* s_callbackUrl);

int g_notify(const char* s_topic,const char* s_msg);

#endif