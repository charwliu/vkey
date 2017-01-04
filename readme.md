vkey 2.0

# Interface

## Start

````
int start_vkey(FN_Notify fn_notify,const char* s_dbPath,const char* s_port);
````
params
- fn_notify callback function
- s_dbPath  db file path,each file means a identity
- s_port    service port

return
- 0:start successful
- -1:start failed

## key

### create key
Request
```
POST HOST/key
{
    "rescure":"rescure code",
    "password":"123"
}
```
- rescure   rescurecode,24 numbers
- password  acturely,it should be hash of user password,32 bytes

Return
```
{
    "iuk":""12554822"
}
```
- iuk   encrypted iuk, vwallet should show qrcode with iuk to user and user must backup the qrcode for recovering the identity
 
## claim

### create claim
Request
````
POST HOST/claim
{
    "templateId":"CLMT_IDNUMBER",
    "claim":
        {
            "value":"1234566"
        }
}
````
Return
````
- 200 Claim post ok! 
- 500 Claim write failed!
````

### get claim
Request
````
 GET HOST/claim?templateId=CLMT_NAME
````
- templateId    if not defined,all claims returned.

Return

````
{
    "templateId":"CLMT_IDNUMBER",
    "claim":
        {
            "value":"1234566"
        }
    "id":"3424234"
}
````

## auth
Request
```
POST HOST/auth
{
  "peer":"as84s8f7a8dfagyerwrg",
  "claims":["CLMT_IDNUMBER","CLMT_SOCIALSECURITY"]
}
```

- vid   id of identity which be authorized

Return
```
- 200 data sent
```

## attestation

Attestation data store in block chain:
- from vid
- to vid
- claim id
- proof signature by witness


```
POST HOST/attestation
{
   ...
}
```

# Coding standard

- Public function name

model_foo();

- Params name

string s_name
int    n_name

- Global name
string g_name


- Local name
string strName

# Building

## linux

clion open project, select sdk forder

## ios

创建一个ios目录，拷贝ios_64.cmake 到ios目录，进入ios，执行cmake 命令生成xcode项目，用xcode打开，然后编译

mkdir ios

cp iOS_64.cmake ./ios

cd ios

cmake -DCMAKE_TOOLCHAIN_FILE=ios_64.cmake  -DCMAKE_IOS_DEVELOPER_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/ -DCMAKE_IOS_SDK_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/  -GXcode ../src
