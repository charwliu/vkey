
#include "network.h"
#include "http.h"
#include "mqtt.h"
#include "config.h"
#include "eth.h"
#include "template.h"

static int network_finish=0;

int network_start()
{
    network_finish=0;
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    template_load(&mgr);

    http_start(&mgr);

    mqtt_connect(&mgr);

    eth_init(&mgr);

    while (!network_finish)
    {
        mg_mgr_poll(&mgr, 1000);
        time_t now=time(NULL);
        if(now%5==0)
        mqtt_timer(now);
        //printf("time:%d\n",now);
    }

    mg_mgr_free(&mgr);
}

int network_stop()
{
    network_finish=1;
}

int network_isFinish()
{
    return network_finish;
}