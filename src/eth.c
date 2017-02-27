
#include "eth.h"
#include <string.h>
#include <sodium.h>
#include "mongoose/mongoose.h"

#define TEMPLATE_PAYLOAD_ETH_CALL "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\", \"gas\": 4712388},\"latest\"],\"id\":1}"
#define ETH_TEMPLATE_PAYLOAD "{\"jsonrpc\":\"2.0\",\"method\":\"personal_sendTransaction\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\", \"gas\": \"0x47E7C4\"},\"%s\"],\"id\":1}"
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

    //stringHex(result, str);

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

static char* eth_buildBytesPayload(char* result, char** values, int size)
{
    for(int i=0;i<size;i++)
    {
        strncat(result,values[i],strlen(values[i]));
    }
}

static char* eth_buildPayload(char* result, char** values, int size)
{

    int segments[size];
    int len,segment;
    for(int i=0;i<size;i++)
    {
        len=(int)strlen(values[i]);
        segment=len/32;//segments, each segment has 32 bytes
        if(len%32>0) segment++;
        segments[i]=segment;
    }


    int param_data_offset=32*size;
    for(int i=0;i<size;i++)
    {
        intEncodingSliceInData(result,param_data_offset);
        param_data_offset+=(32+segments[i]*32);
    }

    for(int i=0;i<size;i++)
    {
        strEncodingSliceInData(result,values[i]);
    }

    return result;
}



int eth_init(struct mg_mgr *mgr)
{
    eth_mgr=mgr;
}

static char* eth_getUrl()
{
    return "http://124.251.62.216:7788";
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
    //build params data
    char data[1024];
    memset(data,0,1024);
    strncpy(data,FUNCTION_CODE_REGISTER,strlen(FUNCTION_CODE_REGISTER));
    char* values[5];
    values[0]=global_name;
    values[1]=vid;
    values[2]=pid;
    values[3]=suk;
    values[4]=vuk;
    eth_buildPayload(data,values,5);

    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    sprintf(payload,ETH_TEMPLATE_PAYLOAD,PUB_ADDRESS,CONTRACT_ADDRESS,data,ETH_ACCOUNT_PASSWORD);

    struct mg_connection *nc;

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


int eth_attest_write(const char* s_sig,const char* s_apk,const char* s_rapk,const char* s_tid)
{
    char* trEthFunctionCodeRegister="0x87a9792a";

    char *strEthPayload= "{\"jsonrpc\":\"2.0\",\"method\":\"personal_sendTransaction\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\", \"gas\": 4712388},\"%s\"],\"id\":1}";
    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0x02a42ce070accce12f4f7e243b65cbcb7867cd38";
    char* strEthPassword="mm1234";

    //build params data
    char data[1024];
    memset(data,0,1024);
    strncpy(data,trEthFunctionCodeRegister,strlen(trEthFunctionCodeRegister));

    char strRAPK[67];
    char strAPK[67];
    sprintf(strRAPK,"0x%s",s_rapk);
    sprintf(strAPK,"0x%s",s_apk);
    char* values[4];
    values[0]="0000000000000000000000000000000000000000000000000000000000000080";
    values[1]="00000000000000000000000000000000000000000000000000000000000000a0";
    values[2]=strRAPK;
    values[3]=strAPK;
    values[4]="0000000000000000000000000000000000000000000000000000000000000040";
    values[5]=s_sig;


    eth_buildBytesPayload(data,values,6);

    strEncodingSliceInData(data,s_tid);

    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    sprintf(payload,strEthPayload,strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Eth sent to: %s\n", eth_getUrl());
    return 0;
}


int eth_register_client(const char* s_pid, const char* s_rpk, const char* s_vuk, const char* s_suk)
{
    char* strEthFunctionCodeRegister="0x231e0750";
    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0x0c386ce9eb6316a60be7d002ca8913a5e5b5e4f9";
    char* strEthPassword="mm1234";

    char data[2024];
    memset(data,0,2024);
    strncpy(data,strEthFunctionCodeRegister,strlen(strEthFunctionCodeRegister));
    char* values[4];
    values[0]=s_pid;
    values[1]=s_vuk;
    values[2]=s_suk;
    values[3]=s_rpk;

    eth_buildBytesPayload(data,values,4);

    //construct json data to post to block chain
    char payload[3048];
    memset(payload,0,3048);
    int ret=sprintf(payload,ETH_TEMPLATE_PAYLOAD, strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    printf(payload);

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Send Smart Contract message to %s\n", eth_getUrl());
    return 0;
}

int eth_recover_client(const char* s_oldPid,const char* s_sigByURSK,const char* s_pid, const char* s_rpk, const char* s_vuk, const char* s_suk)
{
    return 0;
}

/// site submit register
/// \param RID
/// \param sigRID
/// \return
int eth_register_site(char* s_pid,char* s_sigPid,char* s_rpk)
{
    char* strEthFunctionCodeRegister="0x6937e722";
    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0x0c386ce9eb6316a60be7d002ca8913a5e5b5e4f9";
    char* strEthPassword="mm1234";

    char data[2024];
    memset(data,0,2024);
    strncpy(data,strEthFunctionCodeRegister,strlen(strEthFunctionCodeRegister));
    char* values[5];
    values[0]=s_pid;
    values[1]=s_rpk;
    values[2]="0000000000000000000000000000000000000000000000000000000000000060";
    values[3]="0000000000000000000000000000000000000000000000000000000000000040";
    values[4]=s_sigPid;

    eth_buildBytesPayload(data,values,5);

    //construct json data to post to block chain
    char payload[3048];
    memset(payload,0,3048);
    int ret=sprintf(payload,ETH_TEMPLATE_PAYLOAD, strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    printf(payload);

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Send Smart Contract message to %s\n", eth_getUrl());
    return 0;
}