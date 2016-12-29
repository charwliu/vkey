#ifndef CS_VKEY_MQTT_H_
#define CS_VKEY_MQTT_H_

#include "mongoose/mongoose.h"

int mqtt_connect(struct mg_mgr *mgr);

int mqtt_send(const char* s_address,const char* s_data, int n_size);

#endif
