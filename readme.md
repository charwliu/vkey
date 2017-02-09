vkey 2.0

[toc]
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

## Key

### create key
Request
```
POST HOST/key
{
    "rescure":"rescure code",
    "password":"123"
}
```
- rescure :  rescurecode,24 numbers
- password : acturely,it should be hash of user password,32 bytes

Return
```
{
    "iuk":""12554822"
}
```
- iuk   encrypted iuk, vwallet should show qrcode with iuk to user and user must backup the qrcode for recovering the identity
 
### recover key
```
POST HOST/key/recover
{
    "iuk":"111222233",
    "resure":"rescure code"
}

```
- iuk: Identity unlock key encryped by resure code and stored offline by user
- resure: Rescure code stored offline by user

Recover action need the db file recovered from cloud first.

Return
```
{
    
}
```

## 


## Claim

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
- 406 claim would exceed the max count!
````

### update claim

All Attestations of a claim will be removed, when the claim been updated.

Request
````
PUT HOST/claim
{
    "claimId":"344343",
    "claim":
        {
            "value":"1234566"
        }
}
````
All attributes of the claim should be included in the body. It's not possible to update partial attributes of a claim.
If a attribute wasnt included, the old value would be removed.

Return
````
- 200 Claim update ok! 
- 500 Claim write failed!
- 406 claim not found!

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

### delete claim

Request

````
DEL HOST/claim
{
    "claimId":"3424234"
}
````
Return
````
- 200 Claim deleted ok! 
- 406 claim not found!

````

## Auth

Request
```
POST HOST/auth
{
  "to":"as84s8f7a8dfagyerwrg",
  "claims":["CLMT_IDNUMBER","CLMT_SOCIALSECURITY"]
}
```

- to   random topic which be authorized

Return
```
- 200 data sent
```


## Share

Request
```
POST HOST/share
{
  "templateId":"CLMT_IDNUMBER",
  "claimId":"1234",
}
```
- tempateId : claims of this template will be shared all.
- claimId : just this claim will be shared.


Return
```
{
    "vlink":"https://vlink.vnet.com/share/2324242424e2"
}
```
- vlink : address can be used to get shared claim data, client should generate QRCode and show it to others.

## Collection

FLow of collection:
1. user sign in vkey site,  site get his vid and some claims on authorization
2. site orgnize data to claims
3. user select claims on site, and site can use api to create attestation of some of these claims
4. site use collection api send this claims to user
5. user confirm save claims on his mobile phone
6. vwallet call create claim api to save claims selected

Request
```
POST HOST/collection
{
  "to":"1231231241321",
  "claims":[
    {
        "templateId":"CLMT_EDUCATION",
        "claim":{},
        "proof":{}
    },
    {
        "templateId":"CLMT_CAREER",
        "claim":{}
    }
  ]
}
```

- to : reception of the claims
- claims : array of claim to be sent
- proof: site can pre attestat the claim and then send to user, once user accept the claim, it attestatted already.


Return
```
- 200 data sent
```



## Attestation

Attestation data store in block chain:
- from vid
- to vid
- claim id
- proof signature by attestator

### create attestation

Request
```
 POST HOST/attestation
{
    "proof":{
        "from":"74444211acdaf12345",
        "to":"as84s8f7a8dfagyerwrg",
        
        "claimId":"2323423423",
        "claimSignature":"sdfadfa",
        
        "result":1,
    
        "name":"佛山自然人一门式",
        
        "time":"125545611",
        "expiration":"13545544",
        "link":"https://mysite.com/12323",
        
        "extra":{
            "staff":"20112313 李敏",
            "docs":"身份证原件，本人",
            "desc":"本人现场认证，无误"
        }
    }
    "signature":"312sdf1312",
}
```
- proof 
    - from :              vid of attestator
    - to :                vid of claim owner
    - claimId :           claim id
    - claimSignature :    claim signature signed by claim owner
    - result :            result of attestation, signed integer number.
    - name :              name of attestator, provided by attestator
    - time :              time of attestation
    - expiration :        time of expiration
    - link :              address of some page which can be reviewed of this attestation
    - extra :             optional infomation provided by attestator
- Signature :    signature of proof by attestation creator


Return
````
- 200 attest  ok! 
- 406 attest not acceptable!
````

### check attestation
Request
```
 GET HOST/attestation
{
    "claim":{
    },
    "proof":{
    }
    "signature":"312sdf1312",
}
```

Return

````
{
    "attestation":"yes/no"
}
````

- attestation: yes or no

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

- DB
all key saved as hex string.

# Building

## linux

clion open project, select sdk forder

## ios

创建一个ios目录，拷贝ios_64.cmake 到ios目录，进入ios，执行cmake 命令生成xcode项目，用xcode打开，然后编译

mkdir ios

cp iOS_64.cmake ./ios

cd ios

cmake -DCMAKE_TOOLCHAIN_FILE=iOS_64.cmake  -DCMAKE_IOS_DEVELOPER_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/ -DCMAKE_IOS_SDK_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/  -GXcode ../../src
