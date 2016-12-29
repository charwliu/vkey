cmake_minimum_required(VERSION 3.6)

add_subdirectory(src/)

project(vkeytest)

set( SRC_LIST test/main.c)

include_directories(src/)
add_executable(vkeytest ${SRC_LIST})


target_link_libraries(vkeytest vkey )
