#include <stdint.h>
#include "cjson/cJSON.h"

#ifndef CS_VKEY_JWT_H_
#define CS_VKEY_JWT_H_

char* jwt_create(const uint8_t* to_public,cJSON* json);

#endif
