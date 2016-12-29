#Coding standard

## Public function name

model_foo();

## Variable name

string s_name
int    n_name

## Global name
string g_name

## linux

clion open project, select sdk forder

## generate xcode project 
创建一个ios目录，拷贝ios_64.cmake 到ios目录，进入ios，执行cmake 命令生成xcode项目，用xcode打开，然后编译

mkdir ios

cp iOS_64.cmake ./ios

cd ios

cmake -DCMAKE_TOOLCHAIN_FILE=ios_64.cmake  -DCMAKE_IOS_DEVELOPER_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/ -DCMAKE_IOS_SDK_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/  -GXcode ../src
