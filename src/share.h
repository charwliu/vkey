#ifndef CS_VKEY_SHARE_H_
#define CS_VKEY_SHARE_H_

int share_route(struct mg_connection *nc, struct http_message *hm );
int share_confirm(struct mg_mqtt_message *msg );
int share_got(struct mg_mqtt_message *msg);

#endif