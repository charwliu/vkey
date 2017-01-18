#ifndef CS_VKEY_MQTT_H_
#define CS_VKEY_MQTT_H_

#include "mongoose/mongoose.h"
#include "cjson/cJSON.h"

int mqtt_connect(struct mg_mgr *mgr);

//int mqtt_send(const char* s_address,const char* s_data, int n_size);
int mqtt_send(const char* s_to,const char* s_from,const char* s_sk, int n_sksize,const char* s_data );

//int mqtt_subscribe(const char* s_topic);
int mqtt_subscribe(const char* s_topic,const char* s_pk,const char* s_sk,time_t t_time,int n_duration,const char* s_data);


int mqtt_unsubscribe(const char* s_topic);

int mqtt_readTopic(const char* s_topic,cJSON* j_result);

#endif
