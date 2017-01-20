
#include <stddef.h>
#include <sodium.h>
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
#include "util.h"
#include "auth.h"
#include "attest.h"
#include "db.h"
#include "share.h"
#include "encrypt.h"
#include "vkey.h"

static const char *s_user_name = "guoqc";
static const char *s_password = "123";

static struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};
static struct mg_connection *mqtt_conn=NULL;
static int mqtt_ready=0;

static int mqtt_log(const char* s_topic,const char* s_pk,const char* s_sk,time_t t_time,int n_duration,const char* s_data);

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
            char clientId[33];
            util_getUUID(clientId,33);
            mg_send_mqtt_handshake_opt(nc, clientId, opts);
            break;
        }
        case MG_EV_MQTT_CONNACK:
        {
            if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED)
            {
                printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
                exit(1);
            }

            mqtt_ready = 1;
            char topic[256];
            sprintf(topic, "%s.*",key_getAddress());
            s_topic_expr.topic = topic;
            
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
            char* strMessage=malloc(msg->payload.len+1);
            memcpy(strMessage,msg->payload.p,msg->payload.len);
            strMessage[msg->payload.len]=0;


            mqtt_got(msg);
            /*
            if(util_strstr(&msg->topic,"SHARE_SRC")==0)
            {
                share_confirm(msg);
            }

            if(util_strstr(&msg->topic,"SHARE_DES")==0)
            {
                share_got(strMessage);
            }

            if(util_strstr(&msg->topic,"auth")>0)
            {
                auth_got(strMessage);
            }
            if(util_strstr(&msg->topic,"attest")>0)
            {
                attest_got(strMessage);
            }

            free(strMessage);*/
            //mg_mqtt_publish(nc, "/test", 65, MG_MQTT_QOS(0), msg->payload.p, msg->payload.len);
            break;
        }
        case MG_EV_CLOSE:
        {
            printf("MQTT connection closed\n");
            mqtt_ready=0;
            //exit(1);
        }
    }
}

/// connect to mqtt
/// \param mgr
/// \return
int mqtt_connect(struct mg_mgr *mgr)
{
    if(mqtt_conn){
        return 0;
    }
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
/// {
///     from:topic of publish
///     data:un encrypted data
/// }
///
/// \param s_to destinate topic which consist of event/publickey hex
/// \param s_from  source topic which consist of event/publickey hex
/// \param s_sk source secret key hex
/// \param s_data data unecrypted
/// \return
int mqtt_send(const char* s_to,const char* s_event,const char* s_pk,const char* s_sk,const char* s_data )
{
    if(!mqtt_ready) return -1;


    char* strToPK = util_getPk(s_to);

    char to_pk[VKEY_KEY_SIZE];

    size_t len;
    sodium_hex2bin(to_pk,VKEY_KEY_SIZE,strToPK,64,NULL,&len,NULL);


    //generate sharekey from secret key and peer public key
    char shareKey[32];
    encrypt_makeDHShareKey(s_sk,s_pk,to_pk,shareKey);
    //todo: encrypt data with shareKey


    char strPK[65];
    sodium_bin2hex(strPK,65,s_pk,VKEY_KEY_SIZE);

    char strFromTopic[128];
    sprintf(strFromTopic,"%s/%s",s_event,strPK);

    cJSON* jPayload = cJSON_CreateObject();
    cJSON_AddStringToObject(jPayload,"from",strFromTopic);
    cJSON_AddStringToObject(jPayload,"data",s_data);
    char* sPayload = cJSON_PrintUnformatted(jPayload);


    mg_mqtt_publish(mqtt_conn, s_to, 42, MG_MQTT_QOS(0), sPayload, strlen(sPayload));

    free(sPayload);
    cJSON_Delete(jPayload);
}

/// extract data from message, compute sharekey and descrypt data, then call handler of each event
/// \param msg
/// \return
static int mqtt_got(struct mg_mqtt_message *msg)
{
    //get secret key and subscribe data by topic
    char* strTopic = util_getStr(&msg->topic);
    char* strPK = util_getPk(strTopic);

    cJSON* jSubscribe=cJSON_CreateObject();

    if( 0!=mqtt_readTopic(strPK,jSubscribe))
    {
        return -1;
    }

    cJSON* jSubSK = cJSON_GetObjectItem(jSubscribe,"sk");
    cJSON* jSubData = cJSON_GetObjectItem(jSubscribe,"data");
    //cJSON* jClaimIds = cJSON_GetObjectItem(jData,"claimIds");

    //get from public key
    char* strPayload = util_getStr(&msg->payload);
    cJSON* jPayload = cJSON_Parse(strPayload);
    cJSON* jFrom = cJSON_GetObjectItem(jPayload,"from");
    cJSON* jData = cJSON_GetObjectItem(jPayload,"data");


    //compute share key
    char* strMyPK = util_getPk(strTopic);
    char* strFromPK = util_getPk(jFrom->valuestring);

    char my_pk[VKEY_KEY_SIZE];
    char my_sk[VKEY_KEY_SIZE];
    char from_pk[VKEY_KEY_SIZE];
    size_t len;
    sodium_hex2bin(my_pk,VKEY_KEY_SIZE,strMyPK,64,NULL,&len,NULL);
    sodium_hex2bin(my_sk,VKEY_KEY_SIZE,jSubSK->valuestring,64,NULL,&len,NULL);
    sodium_hex2bin(from_pk,VKEY_KEY_SIZE,strFromPK,64,NULL,&len,NULL);

    //generate sharekey from secret key and peer public key
    char shareKey[32];
    encrypt_makeDHShareKey(my_sk,my_pk,from_pk,shareKey);

    //todo:descrypt data
    char* strData;



    if(strstr(strTopic,"SHARE_SRC"))
    {
        share_confirm(jFrom->valuestring,my_pk,my_sk,jSubData,jData->valuestring);
    }

    if(strstr(strTopic,"SHARE_DES"))
    {
        share_got(strTopic,jData->valuestring);
    }

    if(strstr(strTopic,"AUTH_DES")>0)
    {
        auth_got(strTopic,jData->valuestring);
    }
    if(strstr(strTopic,"attest")>0)
    {
        //attest_got(strMessage);
    }

    free(strTopic);
    free(strPayload);

    cJSON_Delete(jPayload);
    cJSON_Delete(jSubscribe);



}

/// subscribe and save the data
/// \param s_event   SHARE/AUTH/ATTEST etc.
/// \param s_pk
/// \param s_sk
/// \param t_time
/// \param n_duration
/// \param s_data
/// \return
int mqtt_subscribe(const char* s_event,const char* s_pk,const char* s_sk,time_t t_time,int n_duration,const char* s_data)
{
    if(!mqtt_ready) return -1;

    char strPK[65];
    char strSK[65];
    sodium_bin2hex(strPK,65,s_pk,VKEY_KEY_SIZE);
    sodium_bin2hex(strSK,65,s_sk,VKEY_KEY_SIZE);


    sprintf(s_topic_expr.topic,"%s/%s",s_event,strPK);


    printf("Subscribing to '%s'\n", s_topic_expr.topic);
    mg_mqtt_subscribe(mqtt_conn, &s_topic_expr, 1, 42);

    mqtt_log(s_event,strPK,strSK,t_time,n_duration,s_data);
    return 0;
}

/// subscribe topic to mqtt
/// \param s_topic
/// \return
int mqtt_unsubscribe(const char* s_topic)
{
    if(!mqtt_ready) return -1;

    s_topic_expr.topic = s_topic;

    printf("Unsubscribed to '%s'\n", s_topic_expr.topic);
    mg_mqtt_unsubscribe(mqtt_conn, &s_topic_expr, 1, 42);

    //todo:remove subscribe record from db file
    return 0;
}

/// save the subscribe data
/// \param s_event ,SHARE/AUTH/ATTEST
/// \param s_pk ,communication public key and the part of mq topic
/// \param s_sk ,communication secret key
/// \param t_time ,subscribe time, since the subscribe will be cancel when the duration is expired
/// \param n_duration , duration of the subscribe
/// \param s_data , additional data with the subscribe
/// \return
static int mqtt_log(const char* s_event,const char* s_pk,const char* s_sk,time_t t_time,int n_duration,const char* s_data)
{
    sqlite3* db = db_get();
    char strSql[1024];
    sprintf(strSql,"INSERT INTO TB_MQTT (TOPIC,PK,SK,TIME,DURATION,DATA) VALUES(?,?,?,?,?,?)");

    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_bind_text(pStmt,1,s_event,strlen(s_event),SQLITE_TRANSIENT);
    sqlite3_bind_text(pStmt,2,s_pk,strlen(s_pk),SQLITE_TRANSIENT);
    //todo: encrypt sk
    sqlite3_bind_text(pStmt,3,s_sk,strlen(s_sk),SQLITE_TRANSIENT);

    sqlite3_bind_int(pStmt,4,t_time );
    sqlite3_bind_int(pStmt,5,n_duration);
    sqlite3_bind_text(pStmt,6,s_data,strlen(s_data),SQLITE_TRANSIENT);

    ret = sqlite3_step(pStmt);
    if( ret != SQLITE_DONE )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }
    sqlite3_finalize(pStmt);

    return 0;
}

int mqtt_readTopic(const char* s_pk,cJSON* j_result)
{
    sqlite3* db = db_get();

    char strSql[256];


    sprintf(strSql,"SELECT SK,TIME,DURATION,DATA FROM TB_MQTT WHERE PK=?;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    sqlite3_bind_text(pStmt, 1, s_pk, strlen(s_pk), SQLITE_TRANSIENT);


    if( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        char *strSK = (char *) sqlite3_column_text(pStmt, 0);
        time_t time = (time_t)sqlite3_column_int(pStmt,1);
        int duration = sqlite3_column_int(pStmt,2);
        char *strData = (char *) sqlite3_column_text(pStmt, 3);

        //todo: descrypt data
        cJSON* jData = cJSON_Parse(strData);
        if(strSK)
        {
            cJSON_AddStringToObject(j_result,"sk",strSK);
        }

        cJSON_AddNumberToObject(j_result,"time",time);
        cJSON_AddNumberToObject(j_result,"duration",duration);

        if(strData)
        {
            cJSON_AddStringToObject(j_result,"data",strData);
        }

        sqlite3_finalize(pStmt);
        return 0;
    }
    return -1;
}