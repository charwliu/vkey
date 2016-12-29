
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

int start_vkey(FN_Notify fn_notify){

    config_load();
    db_init("/home/gqc/vkey/db.vkey");
    encrypt_init();

    key_load();

    g_fn_notity=fn_notify;
    network_start();

    return 0;
}
