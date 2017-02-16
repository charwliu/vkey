
#include "network.h"
#include "http.h"
#include "mqtt.h"
#include "config.h"
#include "eth.h"
#include "template.h"

int network_start()
{
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    template_update(&mgr);

    http_start(&mgr);

    mqtt_connect(&mgr);

    eth_init(&mgr);

    while (1)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
}