#ifndef CS_VKEY_CONFIG_H_
#define CS_VKEY_CONFIG_H_

#define uint_8 unsigned char;

typedef struct S_VKEY_CONFIG
{
    char* sde_url;
    char* http_port;
} vkey_config;

vkey_config g_config;

void config_load();

#endif