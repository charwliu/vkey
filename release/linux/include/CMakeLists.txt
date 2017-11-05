cmake_minimum_required(VERSION 3.6)

#add_subdirectory(src/libsodium)

project(sdk)

file(GLOB_RECURSE HEADER_LIST "./*.h")
file(GLOB_RECURSE SRC_LIST "./*.c")

include_directories(libsodium/include)
include_directories(libsodium/include/sodium)
#aux_source_directory(SOURCE_FILES *.c)
#set(SOURCE_FILES src/main.c src/mongoose/mongoose.c
#        src/start.c src/sqlite/sqlite3.c src/db.c src/cjson/cJSON.c
#        src/encrypt.c src/http.c src/claim.c src/auth.c src/util.c src/mqtt.c src/gcm/gcm.c src/gcm/aes.c src/config.c src/network.c src/key.c src/jwt.c)
#add_executable(sdk ${SRC_LIST})

add_library(vkey  STATIC ${SRC_LIST} ${HEADER_LIST})

target_link_libraries(vkey -lpthread -ldl -lm )
