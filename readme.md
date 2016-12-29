#Coding standard

## Public function name

model_foo();

## Variable name

string s_name
int    n_name

## Global name
string g_name


## generate xcode project 
mkdir ios
cp iOS_64.cmake ./ios
cd ios

cmake -DCMAKE_TOOLCHAIN_FILE=ios_64.cmake  -DCMAKE_IOS_DEVELOPER_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/ -DCMAKE_IOS_SDK_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/  -GXcode ../src
