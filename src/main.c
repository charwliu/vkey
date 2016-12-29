
#include <stdio.h>
#include "start.h"

int notify(const char* s_msg);

int main() {
    start_vkey(notify);
    return 0;
}

int notify(const char* s_msg)
{
    printf("notify from sdk: %s \n",s_msg);

}