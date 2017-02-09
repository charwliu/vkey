
#include "http.h"
#include "claim.h"
#include "auth.h"
#include "config.h"
#include "key.h"
#include "attest.h"
#include "share.h"
#include "collect.h"
#include "vlink.h"
#include "util.h"
#include "register.h"

static int test(struct mg_connection *nc, struct http_message *hm);

/// router gateway
static http_router routers[8]={
        {key_route,"*","/api/v1/key"},
        {claim_route,"*","/api/v1/claim"},
        {auth_route,"*","/api/v1/auth"},
        {attest_route,"*","/api/v1/attestation"},
        {share_route,"*","/api/v1/share"},
        {collect_route,"*","/api/v1/collection"},
        {register_route,"*","/api/v1/register"},
        {vlink_route,"*","/api/v1/vlink"},
        {test,"GET","/test"}
};


static int test(struct mg_connection *nc, struct http_message *hm)
{
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_printf_http_chunk(nc, "hello mqtt"); /* Send empty chunk, the end of response */
    mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
    return 0;

}


static void handle_not_found(struct mg_connection *nc, struct http_message *hm)
{
    /* Send headers */
    mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\nTransfer-Encoding: chunked\r\n\r\n");

    /* Compute the result and send it back as a JSON object */
    mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    struct http_message *hm = (struct http_message *) ev_data;

    switch (ev)
    {
        case MG_EV_HTTP_REQUEST:
        {
            if (0 != http_routers_handle(routers, 8, nc, hm))
            {
                handle_not_found(nc, hm);
            }
            break;
        }
        default:
            break;
    }
}


int http_start(struct mg_mgr *mgr)
{
    if (g_config.http_port == NULL)
    {
        return -1;
    }
    struct mg_bind_opts bind_opts;

    const char *err_str = "vkey service error";


    struct mg_connection *nc;


    /* Set HTTP server options */
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err_str;

    nc = mg_bind_opt(mgr, g_config.http_port, ev_handler, bind_opts);

    if (nc == NULL)
    {
        fprintf(stderr, "Error starting server on port : %s\n", g_config.http_port);
        return -1;
    }

    mg_set_protocol_http_websocket(nc);
    //    s_http_server_opts.enable_directory_listing = "no";

    printf("Starting RESTful server on port %s \n", g_config.http_port);
    return 0;

}

int http_routers_handle(const http_router* routers,int n_size, struct mg_connection *nc, struct http_message *hm)
{
    struct mg_str all = mg_mk_str("*");
    for( int i=0;i<n_size;i++)
    {
        http_router router = routers[i];
        //if (mg_vcmp(&hm->uri, router.uri) == 0 &&
        if (util_strstr(&hm->uri, router.uri) == 0 &&
                (mg_vcmp(&hm->method, router.method) == 0 || mg_vcmp(&all, router.method) == 0) )
        {
            int rt = router.fn_handle(nc,hm);
            if(rt==0)
            {
                return 0;
            }
        }
    }
    return -1;
}


int http_response_error(struct mg_connection *nc, int n_error, const char *s_msg)
{
    if (!nc)
    {
        return -1;
    }

    mg_http_send_error(nc,n_error,s_msg);
    return 0;
}

int http_response_text(struct mg_connection *nc,int n_status,const char *s_msg)
{
    if (!nc)
    {
        return -1;
    }
    mg_send_response_line(nc, n_status, "Content-Type:text/plain\r\nTransfer-Encoding: chunked\r\n");
    mg_printf_http_chunk(nc, s_msg);
    mg_send_http_chunk(nc, "", 0);
    return 0;

}

int http_response_json(struct mg_connection *nc,int n_status,cJSON* json)
{
    if (!nc)
    {
        return -1;
    }

    mg_send_response_line(nc, n_status, "Content-Type:application/json\r\nTransfer-Encoding: chunked\r\n");

    char* strJson=cJSON_PrintUnformatted(json);
    mg_printf_http_chunk(nc, strJson);
    mg_send_http_chunk(nc, "", 0);
    free(strJson);
    return 0;

}