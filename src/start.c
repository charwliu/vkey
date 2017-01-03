
/* vkey start point */

#include "db.h"
#include "encrypt.h"
#include "http.h"
#include "mqtt.h"
#include "config.h"
#include "network.h"
#include "key.h"
#include "start.h"


FN_Notify g_fn_notity;

int g_notify(const char* s_msg)
{
    g_fn_notity(s_msg);
}

int start_vkey(FN_Notify fn_notify,const char* s_dbPath,const char* s_port){

    config_load();
    if(s_port)
        g_config.http_port=s_port;

    //db_init("/home/gqc/vkey/db.vkey");
    db_init(s_dbPath);
    encrypt_init();

    key_load();

    g_fn_notity=fn_notify;
    network_start();

    return 0;
}
