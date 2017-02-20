#ifndef CS_VKEY_DB_H_
#define CS_VKEY_DB_H_

#include "sqlite/sqlite3.h"

int db_init(const char *s_path,const char* s_password);

int db_exist(const char* s_path);

int db_open(const char *s_path,const char* s_password);

int db_close();

sqlite3* db_get();

int db_write(const char *s_table,const char* s_type,const unsigned char* s_data);

#endif