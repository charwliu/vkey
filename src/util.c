
#include "util.h"
#include "libsodium/include/sodium.h"
#include "attest.h"


const char* util_getUUID(char* s_uuid,int n_size)
{
    if(n_size!=33 )
    {
        return NULL;
    }
    unsigned char random[32];
    randombytes_buf(random,	32);

    unsigned char hash[16];
    crypto_generichash(hash, sizeof	hash, random, 16, NULL,	0);


    //unsigned char *hex=(unsigned char*)malloc(33);
    return sodium_bin2hex(s_uuid,n_size,hash,16);
}


cJSON* util_parseBody(struct mg_str* body)
{
    char* temp=malloc(body->len+1);
    memcpy(temp,body->p,body->len);
    temp[body->len]=0;

    cJSON *json = cJSON_Parse(temp);
    memset(temp,0,body->len+1);
    free(temp);
    return json;
}

/*
 * s_query:name=abc&value=123
 * s_key:value
 * return:123
 *
 * */
char* util_getFromQuery(struct mg_str* s_query,const char* s_key)
{
    c_strnstr(s_query->p,s_key,s_query->len);

    return "todo";
}

int util_strstr(struct mg_str* src, const char* s2)
{
    int n,i=0;
    char* s1=src->p;
    if (*s2)
    {
        while (i<src->len)
        {
            for (n = 0; *(s1 + n) == *(s2 + n); n ++)
            {
                if (!*(s2 + n + 1))
                    return i;
            }
            s1++;
            i++;
        }
        return -1;
    }
    else
        return 0;
}

char* util_getStr(struct mg_str* msg)
{
    char *topic=malloc(msg->len + 1);
    memcpy(topic, msg->p, msg->len);
    topic[msg->len] = 0;
    return topic;
}


char* util_getPk(const char* s_topic)
{
    char* result = strstr(s_topic,"/");
    if( result )
    {
        return result+1;
    }
    return NULL;

}

char** util_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


int util_compareKey(const unsigned char* u_a,const unsigned char* u_b,int n_size)
{
    for(int i=0;i<n_size;i++)
    {
        if(u_a[i]!=u_b[i])
        {
            return 0;
        }
    }
    return 1;
}


int util_addStringToSet(char** set,int n_size,char* item)
{
    for(int i=0;i<n_size;i++)
    {
        if(set[i]==NULL)
        {
            set[i]=item;
            return 0;
        }
        else
        {
            if(strcmp(set[i],item)==0)
            {
                return -1;
            }
        }
    }
    return -1;
}