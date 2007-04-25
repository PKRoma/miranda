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

/* Always enable logging for now
   Disable by setting DisableLogging/1 in dbrw_core table */
#ifndef DBRW_LOGGING
#define DBRW_LOGGING
#endif

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <process.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_utils.h>
#include "sqlite3/sqlite3.h"
#include "contacts.h"
#include "events.h"
#include "settings.h"
#include "sql.h"
#include "utf8.h"
#include "utils.h"

extern sqlite3 *g_sqlite;
extern struct LIST_INTERFACE li;
extern char g_szDbPath[MAX_PATH];
extern CRITICAL_SECTION csContactsDb;
extern CRITICAL_SECTION csEventsDb;
extern CRITICAL_SECTION csSettingsDb;
extern HANDLE hSettingChangeEvent;
extern HANDLE hContactDeletedEvent;
extern HANDLE hContactAddedEvent;
extern HANDLE hEventFilterAddedEvent;
extern HANDLE hEventAddedEvent;
extern HANDLE hEventDeletedEvent;
extern struct MM_INTERFACE memoryManagerInterface;

#define dbrw_alloc(n)            utils_mem_alloc(n) /*memoryManagerInterface.mmi_malloc(n)*/
#define dbrw_free(ptr)           utils_mem_free(ptr) /*memoryManagerInterface.mmi_free(ptr)*/
#define dbrw_realloc(ptr,size)   utils_mem_realloc(ptr, size) /*memoryManagerInterface.mmi_realloc(ptr,size)*/

#define DBRW_VER_MAJOR           1
#define DBRW_VER_MINOR           0
#define DBRW_VER_STRING          "1.0"
#define DBRW_VER_ALPHA           "5"
#define DBRW_SCHEMA_VERSION      "2"
#define DBRW_HEADER_STR          "SQLite format 3"
#define DBRW_ROT                 5
#define DBRW_SETTINGS_FLUSHCACHE 1000*10
#define DBRW_EVENTS_FLUSHCACHE   1000*30

// Temp define to let dbRW compile on  Miranda < v0.7s
#ifndef MS_DB_SETSETTINGRESIDENT
#define MS_DB_SETSETTINGRESIDENT "DB/SetSettingResident"
#endif

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

#endif
