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

#ifdef DBRW_LOGGING
static FILE *fp = NULL;
static int disableLogging = 0;

void utils_log_init() {
	char szPath[MAX_PATH], szFullPath[MAX_PATH];
	char *str2;
    
    disableLogging = utils_private_setting_get_int("DisableLogging", 0);
    if (disableLogging) 
        return;
    GetModuleFileName(GetModuleHandle(NULL), szPath, sizeof(szPath)-1);
    str2 = strrchr(szPath,'\\');
    if(str2!=NULL) 
        *str2 = 0;
    mir_snprintf(szFullPath, sizeof(szFullPath), "%s\\dbx_rw.log", szPath);
    fp = fopen(szFullPath,"wt");
}

void utils_log_destroy() {
	if (fp)	
		fclose(fp);
}

void utils_log_fmt(const char *file,int line,const char *fmt,...) {
	if (fp) {
        va_list vararg;
        char str[1024];
    
		va_start(vararg,fmt);
		mir_vsnprintf(str,sizeof(str),fmt,vararg);
		va_end(vararg);
		fprintf(fp,"%u: %s %d: %s\n",(unsigned int)GetTickCount(),file,line,str);
	}
}
#endif

int utils_setSafetyMode(WPARAM wParam, LPARAM lParam) {
    int safeMode = (int)wParam;

	log0("db_setSafetyMode");
    if (safeMode) 
        sql_exec(g_sqlite, "PRAGMA synchronous = NORMAL;");
    else
        sql_exec(g_sqlite, "PRAGMA synchronous = OFF;");
	return 0;
}

int utils_getProfileName(WPARAM wParam, LPARAM lParam) {
	char *p = 0;

	p = strrchr(g_szDbPath, '\\');
	if (p == 0) 
		return 1;
	p++;
	strncpy((char*)lParam, p, (size_t)wParam);
	return 0;
}

int utils_getProfilePath(WPARAM wParam, LPARAM lParam) {
	char *dst = (char*)lParam;
	char *p = 0;

	strncpy(dst, g_szDbPath, wParam);
	p = strrchr(dst, '\\');
	if (p == NULL) 
		return 1;
	*p = 0;
	return 0;
}

int utils_encodeString(WPARAM wParam,LPARAM lParam) {
	char *msg = (char*)lParam;
	int i;

	for (i=0; msg[i]; i++) {
		msg[i] = msg[i] + DBRW_ROT;
	}
	return 0;
}

int utils_decodeString(WPARAM wParam,LPARAM lParam) {
	char *msg = (char*)lParam;
	int i;

	for (i=0; msg[i]; i++) {
		msg[i] = msg[i] - DBRW_ROT;
	}
	return 0;
}

DWORD utils_hashString(const char *szStr) {
	DWORD hash = 0;
	int i;
	int shift = 0;

	for(i=0; szStr[i]; i++) {
		hash ^= szStr[i]<<shift;
		if(shift>24) 
			hash ^= (szStr[i]>>(32-shift))&0x7F;
		shift = (shift+5)&0x1F;
	}
	return hash;
}

struct utils_thread_arg {
    HANDLE hEvent;
    void (*threadcode) (void *);
    void *arg;
};

static void utils_thread_r(struct utils_thread_arg *fa) {
    void (*callercode) (void *) = fa->threadcode;
    void *arg = fa->arg;

    CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
    SetEvent(fa->hEvent);
    __try {
        callercode(arg);
    }
    __finally {
        CallService(MS_SYSTEM_THREAD_POP, 0, 0);
    }
}

unsigned long utils_thread_create(void (*threadcode) (void *), void *arg) {
    unsigned long rc;
    struct utils_thread_arg fa;

    fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    fa.threadcode = threadcode;
    fa.arg = arg;
    rc = _beginthread((void *) (void *) utils_thread_r, 0, &fa);
    if ((unsigned long) -1L != rc) {
        WaitForSingleObject(fa.hEvent, INFINITE);
    }
    CloseHandle(fa.hEvent);
    return rc;
}

int utils_private_setting_get_int(const char *setting, int defval) {
    sqlite3_stmt *stmt;
    int rc = defval;
    
    if (setting==NULL) 
        return rc;
    sql_prepare(g_sqlite, "SELECT val FROM dbrw_core where setting = ?;", &stmt);
    sqlite3_bind_text(stmt, 1, setting, (int)strlen(setting), SQLITE_STATIC);
    if (sql_step(stmt)==SQLITE_ROW) {
        rc = sqlite3_column_int(stmt, 0);
    }
    sql_finalize(stmt);
    return rc;
}

int utils_private_setting_set_int(const char *setting, int val) {
    sqlite3_stmt *stmt;
    int rc = 0;
    
    if (setting==NULL)
        return rc;
    sql_prepare(g_sqlite, "REPLACE INTO dbrw_core VALUES(?,?);", &stmt);
    sqlite3_bind_text(stmt, 1, setting, (int)strlen(setting), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, val);
    if (sql_step(stmt)==SQLITE_DONE) {
        rc = 1;
    }
    sql_finalize(stmt);
    return rc;
}

time_t utils_private_setting_get_time(const char *setting, time_t defval) {
    sqlite3_stmt *stmt;
    time_t rc = defval;
    
    if (setting==NULL) 
        return rc;
    sql_prepare(g_sqlite, "SELECT val FROM dbrw_core where setting = ?;", &stmt);
    sqlite3_bind_text(stmt, 1, setting, (int)strlen(setting), SQLITE_STATIC);
    if (sql_step(stmt)==SQLITE_ROW) {
        rc = (time_t)sqlite3_column_int64(stmt, 0);
    }
    sql_finalize(stmt);
    return rc;
}

int utils_private_setting_set_time(const char *setting, time_t val) {
    sqlite3_stmt *stmt;
    int rc = 0;
    
    if (setting==NULL)
        return rc;
    sql_prepare(g_sqlite, "REPLACE INTO dbrw_core VALUES(?,?);", &stmt);
    sqlite3_bind_text(stmt, 1, setting, (int)strlen(setting), SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, val);
    if (sql_step(stmt)==SQLITE_DONE) {
        rc = 1;
    }
    sql_finalize(stmt);
    return rc;
}

#define DM_VACUUM (WM_USER+1)
static BOOL CALLBACK utils_vacuum_proc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
        SetDlgItemTextA(hwndDlg, IDC_TEXT, Translate("Your Miranda IM profile is being compacted.  Please wait..."));
        PostMessage(hwndDlg, DM_VACUUM, 0, 0);
        break;
    case DM_VACUUM:
        sql_exec(g_sqlite, "VACUUM;");
        DestroyWindow(hwndDlg);
        break;
    }
    return FALSE;
}

void utils_vacuum_check() {
    time_t lastC = utils_private_setting_get_time("LastCompact", 0);
    time_t now = time(NULL);
    
    if (lastC) {
        if (lastC>now) {
            log0("System time changed.  Fixing compact key.");
            utils_private_setting_set_time("LastCompact", now);
        }
        else if ((now-lastC)>(60*60*24*DBRW_COMPACT_DAYS)) {
            log0("Compacting database");
            DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_INFODLG), 0, utils_vacuum_proc,(LPARAM)0);
            utils_private_setting_set_time("LastCompact", now);
        }
    }
    else {
        log0("Compact never run before.  Setting up key.");
        utils_private_setting_set_time("LastCompact", now);
    }
    
}

/* The follow memory functions are needed so a single function
   can be used even if the memory manager hasn't been initialized
   from the core.  This happens when the db is being created (core
   isn't completely loaded yet). */
void* utils_mem_alloc(size_t size) {
    if (memoryManagerInterface.mmi_malloc) /* check for mmi initialization */
        return memoryManagerInterface.mmi_malloc(size);
    return malloc(size); /* fall back if no mmi */
}

void* utils_mem_realloc(void* ptr, size_t size) {
    if (memoryManagerInterface.mmi_realloc) /* check for mmi initialization */
        return memoryManagerInterface.mmi_realloc(ptr, size);
    return realloc(ptr, size);  /* fall back if no mmi */
}

void utils_mem_free(void* ptr) {
    if (memoryManagerInterface.mmi_free) /* check for mmi initialization */
        memoryManagerInterface.mmi_free(ptr);
    else
		free(ptr);  /* fall back if no mmi */
}
