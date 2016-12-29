#ifndef CS_VKEY_START_H_
#define CS_VKEY_START_H_

// function pointer type for notify when sdk got some message
typedef int (*FN_Notify)(const char*) ;

int start_vkey(FN_Notify fn_notify);

int g_notify(const char* s_msg);

#endif