

#include "config.h"

void config_load()
{
    g_config.mqtt_url="124.251.62.214:1886";
    g_config.mqtt_user = "vnetsdk";
    g_config.mqtt_password = "vnetsdk123456";

//    g_config.mqtt_url="192.168.107.132:1883";
//    g_config.mqtt_user = "guoqc";
//    g_config.mqtt_password = "123";

    g_config.http_port="10001";

    g_config.template_server="localhost:3000/templates";

}
