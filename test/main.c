
#include <stdio.h>
#include "start.h"

int notify(const char* s_msg);

int main(char** args,int arg) {
    char* db;
    char* port;
    if(arg>0)
        db=args[0];
    if(arg>1)
        port=args[1];

    start_vkey(notify,db,port);
    return 0;
}

int notify(const char* s_msg)
{
    printf("notify from sdk: %s \n",s_msg);

}