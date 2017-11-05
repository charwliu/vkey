


#include "http.h"
#include "util.h"
#include "attest.h"

/// collection api use attest function, so params are same with attest api
static http_router routers[1]={
        {post_attest,"POST","/api/v1/collection"}
};


int collect_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}