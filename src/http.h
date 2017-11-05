#ifndef CS_VKEY_HTTP_H_
#define CS_VKEY_HTTP_H_

#include "mongoose/mongoose.h"
#include "cjson/cJSON.h"

typedef struct S_HTTP_ROUTER
{
    int (*fn_handle)(struct mg_connection *nc, struct http_message *hm);
    const char* method;
    const char* uri;
} http_router;


int http_start(struct mg_mgr *mgr);

int http_response_error(struct mg_connection *nc, int n_error, const char *s_msg);

int http_response_text(struct mg_connection *nc, int n_status, const char *s_msg);

int http_response_json(struct mg_connection *nc, int n_status, cJSON *json);

int http_routers_handle(const http_router *routers, int n_size, struct mg_connection *nc, struct http_message *hm);


int http_post(const char* s_url,const char* s_payload);

#endif