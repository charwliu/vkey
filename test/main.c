
#include <stdio.h>
#include "start.h"

int notify(const char*s_topic, const char* s_msg);

int main(int arg,char* args[]) {
    char* db;
    char* port;
    if(arg>1)
        db=args[1];
    if(arg>2)
        port=args[2];

    char IUK[65];

    init_key();

    create_key(db,"123","rescure-code","random hex data",IUK);

    printf("Encrypted IUK:%s\n",IUK);

    start_vkey(db,"123",port,notify,"localhost:3000/vkey");
    return 0;
}

int notify(const char*s_topic,const char* s_msg)
{
    printf("App: Notified from sdk. \n\n");

}