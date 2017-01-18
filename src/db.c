
#include <stdio.h>
#include <string.h>
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

static int db_exist(const char* s_table)
{
    sqlite3* db = db_get();
    char strSql[256];

    sprintf(strSql,"SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?;");


    sqlite3_stmt* pStmt;
    const char* strTail=NULL;
    int ret = sqlite3_prepare_v2(db,strSql,-1,&pStmt,&strTail);
    if( ret != SQLITE_OK )
    {
        sqlite3_finalize(pStmt);
        return -1;
    }

    sqlite3_bind_text(pStmt, 1, s_table, strlen(s_table), SQLITE_TRANSIENT);

    ret=0;
    if( sqlite3_step(pStmt) == SQLITE_ROW )
    {
        ret=1;
    }

    sqlite3_finalize(pStmt);
    return ret;
}

static int db_checkKeyTable()
{
//    if(1==db_exist("TB_KEY"))
//    {
//        fprintf(stdout, "Table TB_KEY is ready\n");
//        return 0;
//    }
//    char *sErrMsg = 0;
//    /* Create SQL statement */
//    const char* sql = "CREATE TABLE TB_KEY( ADDRESS TEXT PRIMARY KEY NOT NULL, IMK BLOB NOT NULL, ILK BLOB NOT NULL, TAG TEXT NOT NULL,TIME INT );";
//
//    /* Execute SQL statement */
//    int rc = sqlite3_exec(g_db, sql, callback, 0, &sErrMsg);
//    if( rc != SQLITE_OK ){
//        fprintf(stderr, "SQL error: %s\n", sErrMsg);
//        sqlite3_free(sErrMsg);
//        return -1;
//    }else{
//        fprintf(stdout, "Table created successfully\n");
//    }
//    return 0;
    const char* sql = "CREATE TABLE TB_KEY( ADDRESS TEXT PRIMARY KEY NOT NULL, IMK BLOB NOT NULL, ILK BLOB NOT NULL, TAG TEXT NOT NULL,TIME INT );";
    return db_checkTable("TB_KEY",sql);
}


static int db_checkClaimTable()
{
    const char* sql = "CREATE TABLE TB_CLAIM( ID TEXT PRIMARY KEY NOT NULL, TID TEXT NOT NULL, DATA TEXT NOT NULL, TIMEC INT, TIMEU INT );";
    return db_checkTable("TB_CLAIM",sql);
}


static int db_checkAttestTable()
{
    const char* sql = "CREATE TABLE TB_ATTEST( CID TEXT NOT NULL, ATTEST TEXT NOT NULL, RASK TEXT NOT NULL, TIME INT );";
    return db_checkTable("TB_ATTEST",sql);
}


static int db_checkShareTable()
{
    const char* sql = "CREATE TABLE TB_SHARE( TOPIC TEXT PRIMARY KEY NOT NULL, CLAIMS TEXT NOT NULL, TIME INT NOT NULL, DURATION INT, CONFIRM INT );";
    return db_checkTable("TB_SHARE",sql);
}

static int db_checkTable(const char* s_table,const char* s_createSQL)
{
    if(1==db_exist("s_table"))
    {
        fprintf(stdout, "Table %s is ready\n",s_table);
        return 0;
    }
    char *sErrMsg = 0;

    /* Execute SQL statement */
    int rc = sqlite3_exec(g_db, s_createSQL, callback, 0, &sErrMsg);
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

    if(0!=db_checkKeyTable())
    {
        return -1;
    }
    if(0!=db_checkClaimTable())
    {
        return -1;
    }

    if(0!=db_checkAttestTable())
    {
        return -1;
    }

    if(0!=db_checkShareTable())
    {
        return -1;
    }
    return 0;
}
