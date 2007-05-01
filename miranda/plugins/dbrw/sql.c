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

static char **sql_prepare_text;
static sqlite3_stmt ***sql_prepare_stmt;
static int sql_prepare_len = 0;
static unsigned sqlThreadId;
static HANDLE hSqlThread;
static HWND hAPCWindow = NULL;
static HANDLE hDummyEvent = NULL;

static unsigned __stdcall sql_threadProc(void *arg);
static DWORD CALLBACK sql_apcproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef struct {
	HANDLE hDoneEvent;
	sqlite3_stmt *stmt;
	int result;
} TStatementToMainThreadItem;

typedef struct {
	HANDLE hDoneEvent;
	sqlite3 *sql;
    const char *query;
	int result;
} TExecToMainThreadItem;

typedef struct {
	HANDLE hDoneEvent;
	sqlite3 **sql;
    const char *path;
	int result;
} TSQLOpenToMainThreadItem;

typedef struct {
	HANDLE hDoneEvent;
	sqlite3 *sql;
	int result;
} TSQLCloseToMainThreadItem;

typedef struct {
	HANDLE hDoneEvent;
	sqlite3 *sql;
    const char *query;
    sqlite3_stmt **stmt;
	int result;
} TPrepareToMainThreadItem;

typedef struct {
	HANDLE hDoneEvent;
	sqlite3_stmt *stmt;
	int result;
} TFinalizeToMainThreadItem;

void sql_init() {
	log0("Loading module: sql");
    hDummyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hSqlThread = (HANDLE)mir_forkthreadex(sql_threadProc, 0, 0, &sqlThreadId);
    hAPCWindow = CreateWindowEx(0, _T("STATIC"), NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    SetWindowLong(hAPCWindow, GWL_WNDPROC, (LONG)sql_apcproc);
}

void sql_destroy() {
	int i;
	
	log0("Unloading module: sql");
	for(i = 0; i < sql_prepare_len; i++)
		sql_finalize(*sql_prepare_stmt[i]);
	dbrw_free(sql_prepare_text);
	dbrw_free(sql_prepare_stmt);
    SetEvent(hDummyEvent); // kill off the sql thread
    CloseHandle(hSqlThread);
    DestroyWindow(hAPCWindow);
}

static unsigned __stdcall sql_threadProc(void *arg) {
    while (WaitForSingleObjectEx(hDummyEvent, INFINITE, TRUE)!=WAIT_OBJECT_0);
    CloseHandle(hDummyEvent);
    return 0;
}

static DWORD CALLBACK sql_apcproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg==WM_NULL) 
        SleepEx(0, TRUE);
	return DefWindowProc(hwnd, msg, wParam, lParam);
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

static void CALLBACK sql_step_sync(DWORD dwParam) {
	TStatementToMainThreadItem *item = (TStatementToMainThreadItem*)dwParam;
	item->result = sqlite3_step(item->stmt);
	SetEvent(item->hDoneEvent);
}

int sql_step(sqlite3_stmt *stmt) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TStatementToMainThreadItem item;
		item.stmt = stmt;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_step_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_step(stmt);
    }
}

static void CALLBACK sql_reset_sync(DWORD dwParam) {
	TStatementToMainThreadItem *item = (TStatementToMainThreadItem*)dwParam;
	item->result = sqlite3_reset(item->stmt);
	SetEvent(item->hDoneEvent);
}

int sql_reset(sqlite3_stmt *stmt) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TStatementToMainThreadItem item;
		item.stmt = stmt;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_reset_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_reset(stmt);   
    }
}

static void CALLBACK sql_exec_sync(DWORD dwParam) {
	TExecToMainThreadItem *item = (TExecToMainThreadItem*)dwParam;
	item->result = sqlite3_exec(item->sql, item->query, NULL, NULL, NULL);
	SetEvent(item->hDoneEvent);
}

int sql_exec(sqlite3 *sql, const char *query) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TExecToMainThreadItem item;
		item.sql = sql;
        item.query = query;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_exec_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_exec(sql, query, NULL, NULL, NULL);   
    }
}

static void CALLBACK sql_open_sync(DWORD dwParam) {
	TSQLOpenToMainThreadItem *item = (TSQLOpenToMainThreadItem*)dwParam;
	item->result = sqlite3_open(item->path, item->sql);
	SetEvent(item->hDoneEvent);
}

int sql_open(const char *path, sqlite3 **sql) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TSQLOpenToMainThreadItem item;
		item.sql = sql;
        item.path = path;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_open_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_open(path, sql);   
    } 
}

static void CALLBACK sql_close_sync(DWORD dwParam) {
	TSQLCloseToMainThreadItem *item = (TSQLCloseToMainThreadItem*)dwParam;
	item->result = sqlite3_close(item->sql);
	SetEvent(item->hDoneEvent);
}

int sql_close(sqlite3 *sql) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TSQLCloseToMainThreadItem item;
		item.sql = sql;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_close_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_close(sql);   
    }
}

static void CALLBACK sql_prepare_sync(DWORD dwParam) {
	TPrepareToMainThreadItem *item = (TPrepareToMainThreadItem*)dwParam;
	item->result = sqlite3_prepare_v2(item->sql, item->query, -1, item->stmt, NULL);   
	SetEvent(item->hDoneEvent);
}

int sql_prepare(sqlite3 *sql, const char *query, sqlite3_stmt **stmt) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TPrepareToMainThreadItem item;
		item.sql = sql;
        item.query = query;
        item.stmt = stmt;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_prepare_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_prepare_v2(sql, query, -1, stmt, NULL);
    }
}

static void CALLBACK sql_finalize_sync(DWORD dwParam) {
	TFinalizeToMainThreadItem *item = (TFinalizeToMainThreadItem*)dwParam;
	item->result = sqlite3_finalize(item->stmt);   
	SetEvent(item->hDoneEvent);
}

int sql_finalize(sqlite3_stmt *stmt) {
    if (GetCurrentThreadId()!=sqlThreadId) {
        TFinalizeToMainThreadItem item;
		item.stmt = stmt;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(sql_finalize_sync, hSqlThread, (DWORD)&item);
		PostMessage(hAPCWindow, WM_NULL, 0, 0);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
    }
    else {
        return sqlite3_finalize(stmt);
    } 
}

