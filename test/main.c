
#include <stdio.h>
#include "start.h"

int notify(const char* s_msg);

int main(int arg,char* args[]) {
    char* db;
    char* port;
    if(arg>1)
        db=args[1];
    if(arg>2)
        port=args[2];

    start_vkey(notify,db,port);
    return 0;
}

int notify(const char* s_msg)
{
    printf("notify from sdk: %s \n",s_msg);

}