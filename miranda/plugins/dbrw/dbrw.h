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
#ifndef DBRW_H
#define DBRW_H

#define DBRW_VER_MAJOR           1
#define DBRW_VER_MINOR           2
#define DBRW_VER_STRING          "1.2"
#define DBRW_VER_ALPHA           "1"
#define DBRW_SCHEMA_VERSION      "2"
#define DBRW_HEADER_STR          "SQLite format 3"
#define DBRW_ROT                 5
#define DBRW_SETTINGS_FLUSHCACHE 1000*3
#define DBRW_EVENTS_FLUSHCACHE   1000*60
#define DBRW_COMPACT_DAYS        7

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <process.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_utils.h>
#include <m_langpack.h>
#include "sqlite3/sqlite3.h"
#include "resource.h"

extern HINSTANCE g_hInst;
extern sqlite3 *g_sqlite;
extern struct MM_INTERFACE memoryManagerInterface;
extern struct LIST_INTERFACE li;
extern char g_szDbPath[MAX_PATH];
extern HANDLE hSettingChangeEvent;
extern HANDLE hContactDeletedEvent;
extern HANDLE hContactAddedEvent;
extern HANDLE hEventFilterAddedEvent;
extern HANDLE hEventAddedEvent;
extern HANDLE hEventDeletedEvent;

#define dbrw_alloc(n)     utils_mem_alloc(n)
#define dbrw_free(p)      utils_mem_free(p)
#define dbrw_realloc(p,s) utils_mem_realloc(p, s)

#ifdef DBRW_LOGGING
#define log0(s)         utils_log_fmt(__FILE__,__LINE__,s)
#define log1(s,a)       utils_log_fmt(__FILE__,__LINE__,s,a)
#define log2(s,a,b)     utils_log_fmt(__FILE__,__LINE__,s,a,b)
#define log3(s,a,b,c)   utils_log_fmt(__FILE__,__LINE__,s,a,b,c)
#define log4(s,a,b,c,d) utils_log_fmt(__FILE__,__LINE__,s,a,b,c,d)
#else
#define log0(s)         
#define log1(s,a)       
#define log2(s,a,b)     
#define log3(s,a,b,c)   
#define log4(s,a,b,c,d) 
#endif

// Temp define to let dbRW compile on  Miranda < v0.7s
#ifndef MS_DB_SETSETTINGRESIDENT
#define MS_DB_SETSETTINGRESIDENT "DB/SetSettingResident"
#endif

// contacts.c
void contacts_init();
void contacts_destroy();
int contacts_getCount(WPARAM wParam, LPARAM lParam);
int contacts_findFirst(WPARAM wParam, LPARAM lParam);
int contacts_findNext(WPARAM wParam, LPARAM lParam);
int contacts_delete(WPARAM wParam, LPARAM lParam);
int contacts_add(WPARAM wParam, LPARAM lParam);
int contacts_isContact(WPARAM wParam, LPARAM lParam);

// events.c
void events_init();
void events_destroy();
int events_getCount(WPARAM wParam, LPARAM lParam);
int events_add(WPARAM wParam, LPARAM lParam);
int events_delete(WPARAM wParam, LPARAM lParam);
int events_getBlobSize(WPARAM wParam, LPARAM lParam);
int events_get(WPARAM wParam, LPARAM lParam);
int events_markRead(WPARAM wParam, LPARAM lParam);
int events_getContact(WPARAM wParam, LPARAM lParam);
int events_findFirst(WPARAM wParam, LPARAM lParam);
int events_findFirstUnread(WPARAM wParam, LPARAM lParam);
int events_findLast(WPARAM wParam, LPARAM lParam);
int events_findNext(WPARAM wParam, LPARAM lParam);
int events_findPrev(WPARAM wParam, LPARAM lParam);

// settings.c
void settings_init();
void settings_destroy();
int setting_getSetting(WPARAM wParam, LPARAM lParam);
int setting_getSettingStr(WPARAM wParam, LPARAM lParam);
int setting_getSettingStatic(WPARAM wParam, LPARAM lParam);
int setting_freeVariant(WPARAM wParam, LPARAM lParam);
int setting_writeSetting(WPARAM wParam, LPARAM lParam);
int setting_deleteSetting(WPARAM wParam, LPARAM lParam);
int setting_enumSettings(WPARAM wParam, LPARAM lParam);
int setting_modulesEnum(WPARAM wParam, LPARAM lParam);
void settings_emptyContactCache(HANDLE hContact);
int settings_setResident(WPARAM wParam, LPARAM lParam);

// sql.c
void sql_init();
void sql_destroy();
void sql_prepare_add(char **text, sqlite3_stmt **stmts, int len);
void sql_prepare_statements();
int sql_step(sqlite3_stmt *stmt);
int sql_reset(sqlite3_stmt *stmt);
int sql_exec(sqlite3 *sql, const char *query);
int sql_open(const char *path, sqlite3 **sql);
int sql_close(sqlite3 *sql);
int sql_prepare(sqlite3 *sql, const char *query, sqlite3_stmt **stmt);
int sql_finalize(sqlite3_stmt *stmt);

// utf8.c
void utf8_decode(char* str, wchar_t** ucs2);
char* utf8_encode(const char* src);
char* utf8_encodeUsc2(const wchar_t* src);

// utils.c
#ifdef DBRW_LOGGING
void utils_log_init();
void utils_log_destroy();
void utils_log_fmt(const char *file,int line,const char *fmt,...);
#endif
int utils_setSafetyMode(WPARAM wParam, LPARAM lParam);
int utils_getProfileName(WPARAM wParam, LPARAM lParam);
int utils_getProfilePath(WPARAM wParam, LPARAM lParam);
int utils_encodeString(WPARAM wParam,LPARAM lParam);
int utils_decodeString(WPARAM wParam,LPARAM lParam);
DWORD utils_hashString(const char *szStr);
int utils_private_setting_get_int(const char *setting, int defval);
int utils_private_setting_set_int(const char *setting, int val);
void utils_vacuum_check();
void* utils_mem_alloc(size_t size);
void* utils_mem_realloc(void* ptr, size_t size);
void utils_mem_free(void* ptr);

#endif
