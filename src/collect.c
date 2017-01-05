


#include "http.h"
#include "util.h"

static int post_collect(struct mg_connection *nc, struct http_message *hm);

static http_router routers[1]={
        {post_collect,"POST","/api/v1/collection"}
};


/*


 * */

static int post_collect(struct mg_connection *nc, struct http_message *hm)
{

    cJSON *json = util_parseBody(&hm->body);

    cJSON *to = cJSON_GetObjectItem(json, "to");
    if(!to)
    {
        http_response_error(nc,400,"Vkey Service : to error");
        return 0;
    }
    cJSON *claims = cJSON_GetObjectItem(json, "claims");
    if(!claims)
    {
        http_response_error(nc,400,"Vkey Service : claims error");
        return 0;
    }



    //todo: share it


    cJSON_Delete(json);

    http_response_text(nc,200,"Collection data send ok!");
}



int collect_route(struct mg_connection *nc, struct http_message *hm )
{
    return http_routers_handle(routers,1,nc,hm);
}