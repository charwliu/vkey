
#include <stddef.h>
#include "mqtt.h"
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

#include "mongoose/mongoose.h"
#include "config.h"
#include "start.h"
#include "key.h"

static const char *s_user_name = "guoqc";
static const char *s_password = "123";

static struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};
static struct mg_connection *mqtt_conn=NULL;

/// event handler of mqtt
/// \param nc
/// \param ev
/// \param p
static void ev_handler(struct mg_connection *nc, int ev, void *p) {
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
    (void) nc;

    switch (ev)
    {
        case MG_EV_CONNECT:
        {
            struct mg_send_mqtt_handshake_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.user_name = s_user_name;
            opts.password = s_password;
            opts.keep_alive=600;

            mg_set_protocol_mqtt(nc);
            mg_send_mqtt_handshake_opt(nc, key_getAddress(), opts);
            break;
        }
        case MG_EV_MQTT_CONNACK:
        {
            if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED)
            {
                printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
                exit(1);
            }
            s_topic_expr.topic = key_getAddress();
            printf("Subscribing to '%s'\n", s_topic_expr.topic);
            mg_mqtt_subscribe(nc, &s_topic_expr, 1, 42);
            break;
        }
        case MG_EV_MQTT_PUBACK:
        {
            printf("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
            break;
        }
        case MG_EV_MQTT_SUBACK:
        {
            printf("Subscription acknowledged, forwarding to '/test'\n");
            break;
        }
        case MG_EV_MQTT_PUBLISH:
        {

            printf("Got incoming message %.*s: %.*s\n", (int) msg->topic.len,
                   msg->topic.p, (int) msg->payload.len, msg->payload.p);



            printf("Forwarding to /test\n");
            char* strMessage=malloc(msg->payload.len+1);
            memcpy(strMessage,msg->payload.p,msg->payload.len);
            strMessage[msg->payload.len]=0;
            g_notify(strMessage);
            //mg_mqtt_publish(nc, "/test", 65, MG_MQTT_QOS(0), msg->payload.p, msg->payload.len);
            break;
        }
        case MG_EV_CLOSE:
        {
            printf("MQTT connection closed\n");
            //exit(1);
        }
    }
}

/// connect to mqtt
/// \param mgr
/// \return
int mqtt_connect(struct mg_mgr *mgr)
{
    mqtt_conn = mg_connect(mgr, g_config.sde_url, ev_handler);
    if (mqtt_conn == NULL)
    {
        fprintf(stderr, "MQTT Connect to (%s) failed\n", g_config.sde_url);
        return -1;
    }
    printf("Connected MQTT server on %s \n", g_config.sde_url);
    return 0;
}

/// send data to mqtt
/// \param s_address
/// \param s_data
/// \param n_size
/// \return
int mqtt_send(const char* s_address,const char* s_data, int n_size)
{
    if(!mqtt_conn) return -1;
    char strTopic[256];
    sprintf(strTopic,"%s",s_address);

    mg_mqtt_publish(mqtt_conn, strTopic, 42, MG_MQTT_QOS(0), s_data, n_size);
}
