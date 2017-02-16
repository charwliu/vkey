
#include "config.h"
#include "mongoose/mongoose.h"
#include "cjson/cJSON.h"
#include "util.h"

cJSON* j_tempates;

/// http response
/// \param nc
/// \param ev
/// \param ev_data
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    int connect_status;

    switch (ev) {
        case MG_EV_CONNECT:
            connect_status = *(int *) ev_data;
            if (connect_status != 0) {
                printf("Error connecting to %s: %s\n", g_config.template_server, strerror(connect_status));
            }
            break;
        case MG_EV_HTTP_REPLY:
        {
            char *strBody = util_getStr(&hm->body);
            if (j_tempates)
            {
                cJSON_Delete(j_tempates);
            }

            j_tempates = cJSON_Parse(strBody);
            free(strBody);

            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
        }
        case MG_EV_CLOSE:
            printf("Template Server closed connection\n");
            break;
        default:
            break;
    }
}

int template_load()
{

    return 0;
}

int template_update(struct mg_mgr *mgr)
{
    struct mg_connection *nc;
    nc = mg_connect_http(mgr, ev_handler, g_config.template_server, NULL, NULL);
    mg_set_protocol_http_websocket(nc);
    return 0;
}

