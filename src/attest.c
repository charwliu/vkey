


#include "http.h"

static int get_attest(struct mg_connection *nc, struct http_message *hm);
static int post_attest(struct mg_connection *nc, struct http_message *hm);

static http_router routers[2]={
        {get_attest,"GET","/api/v1/attestation"},
        {post_attest,"POST","/api/v1/attestation"}
};


/// /api/v1/claim?templateid=CLMT_NAME
static int get_attest(struct mg_connection *nc, struct http_message *hm)
{

}

/*

 {

 }


 * */

static int post_attest(struct mg_connection *nc, struct http_message *hm)
{

}



int attest_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}