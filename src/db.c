
#include <stdio.h>
#include "db.h"
#include "sqlite/sqlite3.h"
#include "cjson/cJSON.h"

sqlite3* g_db;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

sqlite3* db_get()
{
    return g_db;
}

static int db_checkKeyTable()
{
    char *sErrMsg = 0;
    /* Create SQL statement */
    const char* sql = "CREATE TABLE TB_KEY( ADDRESS TEXT PRIMARY KEY NOT NULL, IMK BLOB NOT NULL, ILK BLOB NOT NULL, TAG TEXT NOT NULL,TIME INT );";

    /* Execute SQL statement */
    int rc = sqlite3_exec(g_db, sql, callback, 0, &sErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", sErrMsg);
        sqlite3_free(sErrMsg);
        return -1;
    }else{
        fprintf(stdout, "Table created successfully\n");
    }
    return 0;

}


static int db_checkClaimTable()
{
    char *sErrMsg = 0;
    /* Create SQL statement */
    const char* sql = "CREATE TABLE TB_CLAIM( ID TEXT PRIMARY KEY NOT NULL, TID TEXT NOT NULL, DATA TEXT NOT NULL, TIMEC INT, TIMEU INT );";

    /* Execute SQL statement */
    int rc = sqlite3_exec(g_db, sql, callback, 0, &sErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", sErrMsg);
        sqlite3_free(sErrMsg);
        return -1;
    }else{
        fprintf(stdout, "Table created successfully\n");
    }
    return 0;

}

int db_init(const char *s_path)
{
    sqlite3_open_v2(s_path,&g_db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,NULL);

    db_checkKeyTable();
    db_checkClaimTable();
    return 0;
}
