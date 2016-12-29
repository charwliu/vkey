
#include "network.h"
#include "http.h"
#include "mqtt.h"
#include "config.h"

int network_start()
{
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    http_start(&mgr);

    mqtt_connect(&mgr);

    while (1)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
}