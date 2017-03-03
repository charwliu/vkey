
#include "eth.h"
#include <string.h>
#include <sodium.h>
#include "mongoose/mongoose.h"
#include "attest.h"

#define ETH_TEMPLATE_CALL_PAYLOAD "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\"},\"latest\"],\"id\":1}"
#define ETH_TEMPLATE_TRANS_PAYLOAD "{\"jsonrpc\":\"2.0\",\"method\":\"personal_sendTransaction\",\"params\":[{\"from\": \"%s\", \"to\": \"%s\", \"data\": \"%s\", \"gas\": \"0x47E7C4\"},\"%s\"],\"id\":1}"
#define TEMPLATE_PAYLOAD_ETH_GETTRANSACTIONRECEIPT "{\"jsonrpc\":\"2.0\",\"method\": \"eth_getTransactionReceipt\", \"params\": [\"%s\"], \"id\": 1}"
#define CONTRACT_ADDRESS "0x5c706f505b4405f80ea4bd25dde5f27458abca8d"
#define PUB_ADDRESS "0xf0bdee862720d62ca659e47819e8e2bcff1eb7d8"
#define ETH_ACCOUNT_PASSWORD "mm1234"
#define FUNCTION_CODE_IS_AVAILABLE "0x965306aa"
#define FUNCTION_CODE_REGISTER "0x5a09007b"

static struct mg_mgr *eth_mgr;
static char* eth_header="Content-Type: application/json\r\n";
static int eth_get_attest=0;


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
    char result_tmp[64*segments-strlen(result)+1];
    memset(result_tmp,0x30,64*segments-strlen(result));
    result_tmp[64*segments-strlen(result)]='\0';
    strncat(result,result_tmp,strlen(result_tmp));
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
            printf("Got reply from smart contract:\n%.*s\n", (int) hm->body.len, hm->body.p);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            if(eth_get_attest)
            {
                int ret = (hm->body.p[hm->body.len-1]=='1')?1:0;
                got_attest_fromBC(ret);
                eth_get_attest=0;
            }
            break;
        case MG_EV_CLOSE:
                printf("Server closed connection\n");
            break;
        default:
            break;
    }
}



///
/// \param s_psig
/// \param s_apk
/// \param s_msg hex hash of nonce
/// \param s_msig hex sig 128
/// \return
int eth_attest_read(const char* s_psig, const char* s_apk, const char* s_msg, const char* s_msig )
{

    char* trEthFunctionCodeRegister="0xa75caa05";

    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0xf3757d870986de2b7bba622b6edda91f2a7d6f60";
    char* strEthPassword="mm1234";



    //build params data
    char data[1024];
    memset(data,0,1024);
    strncpy(data,trEthFunctionCodeRegister,strlen(trEthFunctionCodeRegister));

    char* values[7];
    values[0]="0000000000000000000000000000000000000000000000000000000000000060";
    values[1]="00000000000000000000000000000000000000000000000000000000000000c0";
    values[2]=s_msg;
    values[3]="0000000000000000000000000000000000000000000000000000000000000040";
    values[4]=s_psig;
    values[5]="0000000000000000000000000000000000000000000000000000000000000040";
    values[6]=s_msig;


    eth_buildBytesPayload(data,values,7);


    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    sprintf(payload,ETH_TEMPLATE_CALL_PAYLOAD,strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    eth_get_attest=1;
    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Eth sent to: %s\n", payload);
    return 0;
}


int eth_attest_write(const char* s_sig,const char* s_apk,const char* s_rapk,const char* s_tid)
{
    char* trEthFunctionCodeRegister="0x08dfbf77";

    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0xf3757d870986de2b7bba622b6edda91f2a7d6f60";
    char* strEthPassword="mm1234";

    //build params data
    char data[1024];
    memset(data,0,1024);
    strncpy(data,trEthFunctionCodeRegister,strlen(trEthFunctionCodeRegister));

    char* values[6];
    values[0]="0000000000000000000000000000000000000000000000000000000000000080";
    values[1]="00000000000000000000000000000000000000000000000000000000000000e0";
    values[2]=s_rapk;
    values[3]=s_apk;
    values[4]="0000000000000000000000000000000000000000000000000000000000000040";
    values[5]=s_sig;


    eth_buildBytesPayload(data,values,6);

    strEncodingSliceInData(data,s_tid);

    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    sprintf(payload,ETH_TEMPLATE_TRANS_PAYLOAD,strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Eth sent to: %s\n", payload);
    return 0;
}


int eth_register_client(const char* s_pid, const char* s_rpk, const char* s_vuk, const char* s_suk)
{
    char* strEthFunctionCodeRegister="0x231e0750";
    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0x866219a29c5b2f600ad8365175bcda8321c73bc0";
    char* strEthPassword="mm1234";

    char data[1024];
    memset(data,0,1024);
    strncpy(data,strEthFunctionCodeRegister,strlen(strEthFunctionCodeRegister));
    char* values[4];
    values[0]=s_pid;
    values[1]=s_vuk;
    values[2]=s_suk;
    values[3]=s_rpk;

    eth_buildBytesPayload(data,values,4);

    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    int ret=sprintf(payload,ETH_TEMPLATE_TRANS_PAYLOAD, strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    printf(payload);

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Send Smart Contract message to %s\n", eth_getUrl());
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
    char* strEthContractAddress="0x866219a29c5b2f600ad8365175bcda8321c73bc0";
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
    int ret=sprintf(payload,ETH_TEMPLATE_TRANS_PAYLOAD, strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    printf(payload);

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Send Smart Contract message to %s\n", eth_getUrl());
    return 0;
}


int eth_recover_client(const char* s_oldPid,const char* s_sigByURSK,const char* s_pid, const char* s_rpk, const char* s_vuk, const char* s_suk)
{
    char* strEthFunctionCodeRegister="0x566ad2e6";
    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0x866219a29c5b2f600ad8365175bcda8321c73bc0";
    char* strEthPassword="mm1234";

    char data[2024];
    memset(data,0,2024);
    strncpy(data,strEthFunctionCodeRegister,strlen(strEthFunctionCodeRegister));
    char* values[8];
    values[0]=s_oldPid;
    values[1]=s_pid;
    values[2]=s_suk;
    values[3]=s_vuk;
    values[4]=s_rpk;
    values[5]="00000000000000000000000000000000000000000000000000000000000000c0";
    values[6]="0000000000000000000000000000000000000000000000000000000000000040";
    values[7]=s_sigByURSK;

    eth_buildBytesPayload(data,values,8);

    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    int ret=sprintf(payload,ETH_TEMPLATE_TRANS_PAYLOAD, strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    printf(payload);

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Send Smart Contract message to %s\n", eth_getUrl());
    return 0;
}

int eth_recover_site(char* s_pidOld,char* s_pidNew,char* s_sigPidNew)
{
    char* strEthFunctionCodeRegister="0x73a408f1";
    char *strEthPubAddress="0x88df0d4a83b54ff939565551784bc8c486f55642";
    char* strEthContractAddress="0x866219a29c5b2f600ad8365175bcda8321c73bc0";
    char* strEthPassword="mm1234";

    char data[2024];
    memset(data,0,2024);
    strncpy(data,strEthFunctionCodeRegister,strlen(strEthFunctionCodeRegister));
    char* values[5];
    values[0]=s_pidOld;
    values[1]=s_pidNew;
    values[2]="0000000000000000000000000000000000000000000000000000000000000060";
    values[3]="0000000000000000000000000000000000000000000000000000000000000040";
    values[4]=s_sigPidNew;

    eth_buildBytesPayload(data,values,5);

    //construct json data to post to block chain
    char payload[2048];
    memset(payload,0,2048);
    int ret=sprintf(payload,ETH_TEMPLATE_TRANS_PAYLOAD, strEthPubAddress,strEthContractAddress,data,strEthPassword);

    struct mg_connection *nc;

    printf(payload);

    nc = mg_connect_http(eth_mgr, ev_handler, eth_getUrl(), eth_header, payload);
    mg_set_protocol_http_websocket(nc);

    printf("Send Smart Contract message to %s\n", eth_getUrl());
    return 0;
}

