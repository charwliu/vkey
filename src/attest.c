


#include "http.h"
#include "eth.h"

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

 POST HOST/attestation
{
    "peer":"as84s8f7a8dfagyerwrg",
    "claim":{},
    "proof":{
        "signature":"sdfadfa",
        ""
    }
}

 * */

static int post_attest(struct mg_connection *nc, struct http_message *hm)
{
    //todo:1 verify signature of the claim by peer vid

    //todo:2 compute proof signature

    //todo:3 send transction to Smart Contract

    eth_register("cn.guoqc.2","vid123","pid123","suk123","vuk123");
    http_response_text(nc,200,"Attestation post ok!");
}



int attest_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,2,nc,hm);
}