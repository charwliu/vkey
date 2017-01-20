#ifndef CS_VKEY_SHARE_H_
#define CS_VKEY_SHARE_H_

int share_route(struct mg_connection *nc, struct http_message *hm );
//int share_confirm(struct mg_mqtt_message *msg );

//int share_confirm(const char* s_myTopic,const char* s_peerTopic,const char* s_sk,cJSON* j_subData, const char* s_data );
int share_confirm(const char* s_peerTopic,const char* s_pk,const char* s_sk,cJSON* j_subData, const char* s_data );

int share_got(const char* s_myTopic, const char* s_data );

int share_start(cJSON* j_share,cJSON* j_result);

#endif