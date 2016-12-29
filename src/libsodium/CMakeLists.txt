cmake_minimum_required(VERSION 3.6)

project(libsodium)


file(GLOB_RECURSE HEADER_LIST "./include/*.h")
file(GLOB_RECURSE SRC_LIST "./*.c")

include_directories(include)
include_directories(include/sodium)

add_library(libsodium  STATIC ${SRC_LIST} ${HEADER_LIST})

 
