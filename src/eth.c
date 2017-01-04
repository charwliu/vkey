
#include "eth.h"
#include <string.h>
#include <sodium.h>
#include "mongoose/mongoose.h"

#define TEMPLATE_PAYLOAD_ETH_CALL "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\", \"gas\": 4712388},\"latest\"],\"id\":1}"
#define TEMPLATE_PAYLOAD_ETH_SENDTRANSACTION "{\"jsonrpc\":\"2.0\",\"method\":\"personal_sendTransaction\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\", \"gas\": 4712388},\"%s\"],\"id\":1}"
#define TEMPLATE_PAYLOAD_ETH_GETTRANSACTIONRECEIPT "{\"jsonrpc\":\"2.0\",\"method\": \"eth_getTransactionReceipt\", \"params\": [\"%s\"], \"id\": 1}"
#define CONTRACT_ADDRESS "0x5c706f505b4405f80ea4bd25dde5f27458abca8d"
#define PUB_ADDRESS "0xf0bdee862720d62ca659e47819e8e2bcff1eb7d8"
#define ETH_ACCOUNT_PASSWORD "mm1234"
#define FUNCTION_CODE_IS_AVAILABLE "0x965306aa"
#define FUNCTION_CODE_REGISTER "0x5a09007b"

static struct mg_mgr *eth_mgr;
static char* eth_header="Content-Type: application/json\r\n";


//
char* eth_encodeParams1(char* s_result, const char* s_param)
{
//    int len = strlen(s_param);
//    int segments=len/32;
//    if(len%32>0) segments++;
//    char hexLen[65];
//    sodium_bin2hex(hexLen,65,len,sizeof(len));
//    char *hexParam = malloc(segments*64);
//    sodium_bin2hex(hexParam,segments*64,s_param,len);
//
//    strcat(s_result,hexLen);
//    strcat(s_result,hexParam);
//
//    return s_result;
}


static char hex [] = { '0', '1', '2', '3', '4', '5', '6', '7','8', '9' ,'a', 'b', 'c', 'd', 'e', 'f' };

//无符号整形转成16进制字符串
static int uintToHexStr(char* buff, int num)
{
    int len=0,k=0;
    do//for every 4 bits
    {
        //get the equivalent hex digit
        buff[len] = hex[num&0xF];
        len++;
        num>>=4;
    }while(num!=0);
    //since we get the digits in the wrong order reverse the digits in the buffer
    for(;k<len/2;k++)
    {//xor swapping
        buff[k]^=buff[len-k-1];
        buff[len-k-1]^=buff[k];
        buff[k]^=buff[len-k-1];
    }
    //null terminate the buffer and return the length in digits
    buff[len]='\0';
    return len;
}

//字符串转成16进制字符串
static char* stringHex(char* result, char* str)
{
    int len = strlen(str);
    int i;
    char tmp[3]={'\0','\0','\0'};
    for (i=0; i<len; i++) {
        sprintf(tmp,"%x", str[i] & 0xff);
        result=strncat(result,tmp,strlen(tmp));
    }

    return result;
}

//字符串编码。检查长度限制？
static char* genStringEncodedValue(char* result, char* str)
{
    uint16_t len=(uint16_t)strlen(str);
    int segments=len/32;
    if(len%32>0) segments++;

    stringHex(result, str);

    memset(result+strlen(result),'0',segments*64-strlen(result));
    result[segments*64]=0;
//
//    char result_tmp[64*segments-strlen(result)+1];
//    memset(result_tmp,0x30,64*segments-strlen(result));
//    result_tmp[64*segments-strlen(result)]='\0';
//    strncat(result,result_tmp,strlen(result_tmp));
    return result;
}

//无符号整形编码
static char* getUintEncodedValue(char* result, int num)
{
    char str[20];
    uintToHexStr(str, num);
    int len = strlen(str);

    memset(result,'0',64);
    memcpy(result+64-len,str,len);

    result[64]=0;
    return result;
}



static char* intEncodingSliceInData(char* data, int num)
{
    char tmp[65];
    getUintEncodedValue(tmp, num);

    strncat(data,tmp,strlen(tmp));
    return data;
}

static char* strEncodingSliceInData(char* data, char* str)
{
    uint16_t len=(uint16_t)strlen(str);
    int segments=len/32;
    if(len%32>0) segments++;
    intEncodingSliceInData(data,len);

    char tmpgn[64*segments+1];
    tmpgn[0]=0;
    genStringEncodedValue(tmpgn, str);
    strncat(data,tmpgn,strlen(tmpgn));
    return data;
}


//合约函数 - register
static char* eth_payloadRegister(char* result, char* global_name, char* vid, char* pid, char* suk, char* vuk)
{
    char data[4096];
    memset(data,'\0',4096);
    strncpy(data,FUNCTION_CODE_REGISTER,strlen(FUNCTION_CODE_REGISTER));

    int gn_len=(int)strlen(global_name);
    int gn_segments=gn_len/32;//segments, each segment has 32 bytes
    if(gn_len%32>0) gn_segments++;

    int vid_len=(int)strlen(vid);
    int vid_segments=vid_len/32;
    if(vid_len%32>0) vid_segments++;

    int pid_len=(int)strlen(pid);
    int pid_segments=pid_len/32;
    if(pid_len%32>0) pid_segments++;

    int suk_len=(int)strlen(suk);
    int suk_segments=suk_len/32;
    if(suk_len%32>0) suk_segments++;

    int vuk_len=(int)strlen(vuk);
    int vuk_segments=vuk_len/32;
    if(vuk_len%32>0) vuk_segments++;

    int param_data_offset=32*5;
    //gn
    intEncodingSliceInData(data,param_data_offset);
    //vid
    param_data_offset+=(32+gn_segments*32);
    intEncodingSliceInData(data,param_data_offset);

    //pid
    param_data_offset+=(32+vid_segments*32);
    intEncodingSliceInData(data,param_data_offset);

    //suk
    param_data_offset+=(32+pid_segments*32);
    intEncodingSliceInData(data,param_data_offset);

    //vuk
    param_data_offset+=(32+suk_segments*32);
    intEncodingSliceInData(data,param_data_offset);

    strEncodingSliceInData(data,global_name);
    strEncodingSliceInData(data,vid);
    strEncodingSliceInData(data,pid);
    strEncodingSliceInData(data,suk);
    strEncodingSliceInData(data,vuk);

    sprintf(result,TEMPLATE_PAYLOAD_ETH_SENDTRANSACTION,PUB_ADDRESS,CONTRACT_ADDRESS,data,ETH_ACCOUNT_PASSWORD);
    return result;
}



int eth_init(struct mg_mgr *mgr)
{
    eth_mgr=mgr;
}

static char* eth_getUrl()
{
    return "http://124.251.62.214:7788";
}

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
                printf("Error connecting to %s: %s\n", eth_getUrl(), strerror(connect_status));
            }
            break;
        case MG_EV_HTTP_REPLY:
            printf("Got reply:\n%.*s\n", (int) hm->body.len, hm->body.p);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
        case MG_EV_CLOSE:
                printf("Server closed connection\n");
            break;
        default:
            break;
    }
}

/// post rpc request to ethereum smart contract
/// \param global_name
/// \param vid
/// \param pid
/// \param suk
/// \param vuk
/// \return
int eth_register(char* global_name, char* vid, char* pid, char* suk, char* vuk)
{
    char payload[2048];
    eth_payloadRegister(payload,global_name,vid,pid,suk,vuk);

    struct mg_connection *nc;

    //mg_mgr_init(eth_mgr, NULL);
    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Starting RESTful client against %s\n", eth_getUrl());
}


///
/// \param from
/// \param to
/// \param pid
/// \param claimId
/// \param signature
/// \return
int eth_attest(char* from, char* to, char* pid, char* claimId, char* signature)
{
    return -1;
}