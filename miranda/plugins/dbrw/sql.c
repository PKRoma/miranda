/*
dbRW

Copyright (c) 2005-2007 Robert Rainwater

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "dbrw.h"

static void sql_server_start();
static void sql_server_stop();

static char **sql_prepare_text;
static sqlite3_stmt ***sql_prepare_stmt;
static int sql_prepare_len = 0;

void sql_init() {
	log0("Loading module: sql");
    sql_server_start();
}

void sql_destroy() {
	int i;
	
	log0("Unloading module: sql");
	for(i = 0; i < sql_prepare_len; i++)
		sql_finalize(*sql_prepare_stmt[i]);
	dbrw_free(sql_prepare_text);
	dbrw_free(sql_prepare_stmt);
    sql_server_stop();
}

void sql_prepare_add(char **text, sqlite3_stmt **stmts, int len) {
	int i;

	sql_prepare_text = (char**)dbrw_realloc(sql_prepare_text, (sql_prepare_len+len)*sizeof(char*));
	sql_prepare_stmt = (sqlite3_stmt***)dbrw_realloc(sql_prepare_stmt, (sql_prepare_len+len)*sizeof(sqlite3_stmt**));
	for (i=0; i<len; i++) {
		sql_prepare_text[sql_prepare_len+i] = text[i];
		sql_prepare_stmt[sql_prepare_len+i] = &stmts[i];
	}
	sql_prepare_len += len;
}

void sql_prepare_stmts() {
	int i;
	
	log1("Preparing %d statements", sql_prepare_len);
	for(i = 0; i < sql_prepare_len; i++)
		sql_prepare(g_sqlite, sql_prepare_text[i], sql_prepare_stmt[i]);
	log1("Prepared %d statements", sql_prepare_len);
}

/*  SQLite Server Implementation 
    This code was adapted from the SQLite test_server.c server
    implementation.
*/
typedef struct SqlMessage SqlMessage;
struct SqlMessage {
  int op;                      /* Opcode for the message */
  sqlite3 *pDb;                /* The SQLite connection */
  sqlite3_stmt *pStmt;         /* A specific statement */
  int errCode;                 /* Error code returned */
  const char *zIn;             /* Input filename or SQL statement */
  int nByte;                   /* Size of the zIn parameter for prepare() */
  const char *zOut;            /* Tail of the SQL statement */
  SqlMessage *pNext;           /* Next message in the queue */
  SqlMessage *pPrev;           /* Previous message in the queue */
  CRITICAL_SECTION clientMutex;/* Hold this mutex to access the message */
  HANDLE clientWakeup;         /* Signal to wake up the client */
};

enum {
    MSG_Open=1,   /* sqlite3_open(zIn, &pDb) */
    MSG_Prepare,  /* sqlite3_prepare(pDb, zIn, nByte, &pStmt, &zOut) */
    MSG_Step,     /* sqlite3_step(pStmt) */
    MSG_Reset,    /* sqlite3_reset(pStmt) */
    MSG_Finalize, /* sqlite3_finalize(pStmt) */
    MSG_Close,    /* sqlite3_close(pDb) */
    MSG_Done,     /* Server has finished with this message */
    MSG_Exec,     /* sqlite3_exec() */
};

static CRITICAL_SECTION svrQueueMutex;
static CRITICAL_SECTION svrServerMutex;
static HANDLE svrServerWakeup;
static volatile int svrServerHalt;
static SqlMessage *pSvrQueueHead;
static SqlMessage *pSvrQueueTail;

static void sql_server_send(SqlMessage *pMsg) {
    InitializeCriticalSection(&pMsg->clientMutex);
    pMsg->clientWakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
    EnterCriticalSection(&svrQueueMutex);
    pMsg->pNext = pSvrQueueHead;
    if (pSvrQueueHead==0) {
        pSvrQueueTail = pMsg;
    }
    else {
        pSvrQueueHead->pPrev = pMsg;
    }
    pMsg->pPrev = 0;
    pSvrQueueHead = pMsg;
    LeaveCriticalSection(&svrQueueMutex);
    EnterCriticalSection(&pMsg->clientMutex);
    SetEvent(svrServerWakeup);
    while (pMsg->op!=MSG_Done) {
        LeaveCriticalSection(&pMsg->clientMutex);
        WaitForSingleObjectEx(pMsg->clientWakeup, INFINITE, FALSE);
        EnterCriticalSection(&pMsg->clientMutex);
    }
    LeaveCriticalSection(&pMsg->clientMutex);
    DeleteCriticalSection(&pMsg->clientMutex);
    CloseHandle(pMsg->clientWakeup);
}

static void __cdecl sql_server(void *args){
    sqlite3_enable_shared_cache(1);
    EnterCriticalSection(&svrServerMutex);
    while(!svrServerHalt){
        SqlMessage *pMsg;

        EnterCriticalSection(&svrQueueMutex);
        while (pSvrQueueTail==0&&svrServerHalt==0) {
            LeaveCriticalSection(&svrQueueMutex);
            WaitForSingleObjectEx(svrServerWakeup, INFINITE, FALSE);
            EnterCriticalSection(&svrQueueMutex);
        }
        pMsg = pSvrQueueTail;
        if (pMsg) {
            if (pMsg->pPrev) {
                pMsg->pPrev->pNext = 0;
            }
            else {
                pSvrQueueHead = 0;
            }
            pSvrQueueTail = pMsg->pPrev;
        }
        LeaveCriticalSection(&svrQueueMutex);
        if(pMsg==0) 
            break;
        EnterCriticalSection(&pMsg->clientMutex);
        switch (pMsg->op) {
            case MSG_Open: {
                pMsg->errCode = sqlite3_open(pMsg->zIn, &pMsg->pDb);
                break;
            }
            case MSG_Prepare: {
                pMsg->errCode = sqlite3_prepare(pMsg->pDb, pMsg->zIn, pMsg->nByte, &pMsg->pStmt, &pMsg->zOut);
                break;
            }
            case MSG_Step: {
                pMsg->errCode = sqlite3_step(pMsg->pStmt);
                break;
            }
            case MSG_Reset: {
                pMsg->errCode = sqlite3_reset(pMsg->pStmt);
                break;
            }
            case MSG_Finalize: {
                pMsg->errCode = sqlite3_finalize(pMsg->pStmt);
                break;
            }
            case MSG_Close: {
                pMsg->errCode = sqlite3_close(pMsg->pDb);
                break;
            }
            case MSG_Exec: {
                pMsg->errCode = sqlite3_exec(pMsg->pDb, pMsg->zIn, NULL, NULL, NULL);
                break;
            }
        }
        pMsg->op = MSG_Done;
        LeaveCriticalSection(&pMsg->clientMutex);
        SetEvent(pMsg->clientWakeup);
    }
    LeaveCriticalSection(&svrServerMutex);
    sqlite3_thread_cleanup();
}

static void sql_server_start(){
    svrServerHalt = 0;
    svrServerWakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
    InitializeCriticalSection(&svrServerMutex);
    InitializeCriticalSection(&svrQueueMutex);
    mir_forkthread(sql_server, 0);
}

static void sql_server_stop(){
    svrServerHalt = 1;
    SetEvent(svrServerWakeup);
    CloseHandle(svrServerWakeup);
    DeleteCriticalSection(&svrServerMutex);
    DeleteCriticalSection(&svrQueueMutex);
}

int sql_open(const char *zDatabaseName, sqlite3 **ppDb) {
    SqlMessage msg;
    msg.op = MSG_Open;
    msg.zIn = zDatabaseName;
    sql_server_send(&msg);
    *ppDb = msg.pDb;
    return msg.errCode;
}

int sql_prepare(sqlite3 *pDb, const char *zSql, sqlite3_stmt **ppStmt) {
    SqlMessage msg;
    msg.op = MSG_Prepare;
    msg.pDb = pDb;
    msg.zIn = zSql;
    msg.nByte = -1;
    sql_server_send(&msg);
    *ppStmt = msg.pStmt;
    return msg.errCode;
}

int sql_step(sqlite3_stmt *pStmt) {
    SqlMessage msg;
    msg.op = MSG_Step;
    msg.pStmt = pStmt;
    sql_server_send(&msg);
    return msg.errCode;
}
int sql_reset(sqlite3_stmt *pStmt) {
    SqlMessage msg;
    msg.op = MSG_Reset;
    msg.pStmt = pStmt;
    sql_server_send(&msg);
    return msg.errCode;
}

int sql_finalize(sqlite3_stmt *pStmt) {
    SqlMessage msg;
    msg.op = MSG_Finalize;
    msg.pStmt = pStmt;
    sql_server_send(&msg);
    return msg.errCode;
}

int sql_close(sqlite3 *pDb) {
    SqlMessage msg;
    msg.op = MSG_Close;
    msg.pDb = pDb;
    sql_server_send(&msg);
    return msg.errCode;
}

int sql_exec(sqlite3 *pDb, const char *zSql) {
    SqlMessage msg;
    msg.op = MSG_Exec;
    msg.pDb = pDb;
    msg.zIn = zSql;
    sql_server_send(&msg);
    return msg.errCode;
}
