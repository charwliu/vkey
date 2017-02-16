#ifndef CS_VKEY_CONFIG_H_
#define CS_VKEY_CONFIG_H_

#define uint_8 unsigned char;

typedef struct S_VKEY_CONFIG
{
    char* mqtt_url;
    char* mqtt_user;
    char* mqtt_password;
    char* http_port;
    char* template_server;
} vkey_config;

vkey_config g_config;

void config_load();

#endif