
#include <stdio.h>
#include <string.h>
#include "start.h"

int notify(const char*s_topic, const char* s_msg);

int main(int arg,char* args[]) {
    char* cmd;
    if(arg>1)
    {
        cmd=args[1];
    }

    if( strcmp(cmd,"create")==0)
    {
        if( arg>5 )
        {
            char *db=args[2];
            char *secure=args[3];
            char *pwd=args[4];
            char *random=args[5];

            char IUK[65];

            init_key();
            if(0==create_key(db,pwd,secure,random,IUK))
            {
                printf("Identidy created! The Encrypted IUK:%s\nPlease store IUK OFFLINE!",IUK);
            }
            else
            {
                printf("Identidy create failed.\n");
            }
            return 0;
        }
    }
    else if(strcmp(cmd,"start")==0)
    {
        if( arg>5 )
        {
            char *db=args[2];
            char *port=args[3];
            char *pwd=args[4];
            char *url=args[5];

            init_key();

            start_vkey(db,pwd,port,notify,url);
            return 0;
        }

    }
    else if(strcmp(cmd,"update")==0)
    {

    }

    printf("Params Not Right. \n\n");
    printf("Command 1: create identity \n");
    printf("./vkeytest create dbfile  securecode password randomdata\n");
    printf("[example]: ./vkeytest create /home/gqc/vkey/db1.vkey  sc123 pwd123 r123456\n\n");

    printf("Command 2: start vkey \n");
    printf("./vkeytest start dbfile serviceport   password callbackurl\n");
    printf("[example]: ./vkeytest start /home/gqc/vkey/db1.vkey 10002 pwd123 localhost:3000/vkey\n\n");


    printf("Command 3: update identity \n");
    printf("not ready!\n");

    return 0;
}

int notify(const char*s_topic,const char* s_msg)
{
    printf("App: Notified from sdk. \n\n");

}