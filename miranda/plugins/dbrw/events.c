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

enum {
	SQL_EVT_STMT_COUNT=0,
	SQL_EVT_STMT_ADD,
	SQL_EVT_STMT_DELETE,
	SQL_EVT_STMT_BLOBSIZE,
	SQL_EVT_STMT_BLOBSIZE_CACHE,
	SQL_EVT_STMT_GET,
	SQL_EVT_STMT_GET_CACHE,
	SQL_EVT_STMT_GETFLAGS,
	SQL_EVT_STMT_SETFLAGS,
	SQL_EVT_STMT_GETCONTACT,
	SQL_EVT_STMT_GETCONTACT_CACHE,
	SQL_EVT_STMT_FINDFIRST,
	SQL_EVT_STMT_FINDFIRSTUNREAD,
	SQL_EVT_STMT_FINDLAST,
	SQL_EVT_STMT_FINDNEXT,
	SQL_EVT_STMT_FINDPREV,
	SQL_EVT_STMT_NUM
};
static char *evt_stmts[SQL_EVT_STMT_NUM] = {
	"SELECT count(*) FROM dbrw_events where contactid = ?;",
	"INSERT INTO dbrw_events VALUES(NULL,?,?,?,?,?,?,?,?);",
	"DELETE FROM dbrw_events where id = ? AND contactid = ?;",
	"SELECT blobsize FROM dbrw_events where id = ? LIMIT 1;",
	"SELECT blobsize FROM temp_dbrw_events where id = ? LIMIT 1;",
	"SELECT * FROM dbrw_events where id = ? LIMIT 1;",
    "SELECT * FROM temp_dbrw_events where id = ? LIMIT 1;",
	"SELECT flags FROM dbrw_events where id = ? LIMIT 1;",
	"UPDATE dbrw_events SET flags = ? WHERE id = ?;",
	"SELECT contactid FROM dbrw_events where id = ? LIMIT 1;",
	"SELECT contactid FROM temp_dbrw_events where id = ? LIMIT 1;",
	"SELECT id FROM dbrw_events where contactid = ? ORDER by id;",
	"SELECT flags,id FROM dbrw_events where contactid = ? ORDER by id;",
	"SELECT id FROM dbrw_events where contactid = ? ORDER by id DESC;",
	"SELECT id FROM dbrw_events where contactid = ? AND id > ? ORDER by id LIMIT 1;",
	"SELECT id FROM dbrw_events where contactid = ? AND id < ? ORDER by id DESC LIMIT 1;"
};
static sqlite3_stmt *evt_stmts_prep[SQL_EVT_STMT_NUM] = {0};

static void CALLBACK events_timerProc(HWND hwnd, UINT umsg, UINT idEvent, DWORD dwTime);

static int eventsTimerId = 0;

void events_init() {
	log0("Loading module: events");
	sql_prepare_add(evt_stmts, evt_stmts_prep, SQL_EVT_STMT_NUM);
    sql_exec(g_sqlite, "BEGIN TRANSACTION;");
    sql_exec(g_sqlite, "create temp table temp_dbrw_events (id integer primary key,eventtime integer,flags integer,eventtype integer, blob any, blobsize integer, contactid integer,modulename varchar(255),inserttime integer);");
    sql_exec(g_sqlite, "create temp trigger insert_new_temp_event1 after insert on dbrw_events begin replace into temp_dbrw_events values(new.id,new.eventtime,new.flags,new.eventtype,new.blob,new.blobsize,new.contactid,new.modulename,new.inserttime); end;");
    sql_exec(g_sqlite, "create temp trigger insert_new_temp_event2 after update on dbrw_events begin replace into temp_dbrw_events values(new.id,new.eventtime,new.flags,new.eventtype,new.blob,new.blobsize,new.contactid,new.modulename,new.inserttime); end;");
    sql_exec(g_sqlite, "create temp trigger delete_temp_event after delete on dbrw_events begin delete from temp_dbrw_events where id=old.id and contactid=old.id; end;");
    sql_exec(g_sqlite, "END TRANSACTION;");
    eventsTimerId = SetTimer(NULL, 0, DBRW_EVENTS_FLUSHCACHE, events_timerProc);
}

void events_destroy() {
	log0("Unloading module: events");
	if (eventsTimerId)
		KillTimer(NULL, eventsTimerId);
}

static void __cdecl events_timerProcThread(void *arg) {
	EnterCriticalSection(&csEventsDb);
    log0("Flushing events cache table");
    sql_exec(g_sqlite, "BEGIN TRANSACTION;");
    sql_exec(g_sqlite, "drop trigger insert_new_temp_event1;");      
    sql_exec(g_sqlite, "drop trigger insert_new_temp_event2;");  
    sql_exec(g_sqlite, "drop trigger delete_temp_event;");
    sql_exec(g_sqlite, "drop table temp_dbrw_events;");
    log0("Creating events cache table");
    sql_exec(g_sqlite, "create temp table temp_dbrw_events (id integer primary key,eventtime integer,flags integer,eventtype integer, blob any, blobsize integer, contactid integer,modulename varchar(255),inserttime integer);");
    sql_exec(g_sqlite, "create temp trigger insert_new_temp_event1 after insert on dbrw_events begin replace into temp_dbrw_events values(new.id,new.eventtime,new.flags,new.eventtype,new.blob,new.blobsize,new.contactid,new.modulename,new.inserttime); end;");
    sql_exec(g_sqlite, "create temp trigger insert_new_temp_event2 after update on dbrw_events begin replace into temp_dbrw_events values(new.id,new.eventtime,new.flags,new.eventtype,new.blob,new.blobsize,new.contactid,new.modulename,new.inserttime); end;");
    sql_exec(g_sqlite, "create temp trigger delete_temp_event after delete on dbrw_events begin delete from temp_dbrw_events where id=old.id and contactid=old.id; end;");
    sql_exec(g_sqlite, "END TRANSACTION;");
    LeaveCriticalSection(&csEventsDb);
}

static void CALLBACK events_timerProc(HWND hwnd, UINT umsg, UINT idEvent, DWORD dwTime) {
	mir_forkthread(events_timerProcThread, 0);
}

int events_getCount(WPARAM wParam, LPARAM lParam) {
	int rc = 0;
	
	EnterCriticalSection(&csEventsDb);
    if (!contacts_isContact(wParam, 0)) {
        LeaveCriticalSection(&csEventsDb);
        return -1;
    }  
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_COUNT], 1, (int)wParam);
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_COUNT])==SQLITE_ROW)
		rc = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_COUNT], 0);
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_COUNT]);
	LeaveCriticalSection(&csEventsDb);
	log2("Got event count(%d) for #%d", rc, (int)wParam);
	return rc;
}

int events_add(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;
	DBEVENTINFO *dbei = (DBEVENTINFO*)lParam;
	int rc = 0;

	if (dbei==NULL||
			dbei->cbSize!=sizeof(DBEVENTINFO)||
			dbei->timestamp==0||
			NotifyEventHooks(hEventFilterAddedEvent, wParam, lParam)) {
		log1("Attempt to add invalid event for contact(%d)", (int)hContact);
		return 0;
	}
	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int64(evt_stmts_prep[SQL_EVT_STMT_ADD], 1, (__int64)dbei->timestamp);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_ADD], 2, (int)dbei->flags);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_ADD], 3, (int)dbei->eventType);
	sqlite3_bind_blob(evt_stmts_prep[SQL_EVT_STMT_ADD], 4, dbei->pBlob, (int)dbei->cbBlob, SQLITE_STATIC);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_ADD], 5, (int)dbei->cbBlob);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_ADD], 6, (int)hContact);
	sqlite3_bind_text(evt_stmts_prep[SQL_EVT_STMT_ADD], 7, dbei->szModule?dbei->szModule:NULL, dbei->szModule?(int)strlen(dbei->szModule):0, SQLITE_STATIC);
	sqlite3_bind_int64(evt_stmts_prep[SQL_EVT_STMT_ADD], 8, (__int64)time(NULL));
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_ADD])==SQLITE_DONE) {
		rc = (int)sqlite3_last_insert_rowid(g_sqlite);
		log1("Added event(%d)", rc);
	}
	else {
		log2("Error adding event(%d,%s)", rc, sqlite3_errmsg(g_sqlite));
		log1("timestamp(%d)", dbei->timestamp);
		log1("flags(%d)", dbei->flags);
		log1("type(%d)", dbei->eventType);
		log1("hcontact(%d)", hContact);
		log1("module(%s)", dbei->szModule);
	}
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_ADD]);
	LeaveCriticalSection(&csEventsDb);
	if (rc) {
		log1("Firing event(%d)", rc);
		NotifyEventHooks(hEventAddedEvent,wParam,(LPARAM)rc);
	}
	return rc;
}

int events_delete(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;
	HANDLE hDbEvent = (HANDLE)lParam;
	int rc = 1;
	
	// TODO: Verify the event id before sending event notification
	log1("Notify event(%d) to be deleted [verify me]", (int)hDbEvent);
	NotifyEventHooks(hEventDeletedEvent, wParam, lParam);
	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_DELETE], 1, (int)hDbEvent);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_DELETE], 2, (int)hContact);
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_DELETE])==SQLITE_DONE) {
		log1("Deleted event(%d)", (int)hDbEvent);
		rc = 0;
	}
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_DELETE]);
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

static int events_getBlobSizeConditional(HANDLE hDbEvent, int cache) {
	int rc = -1;
    sqlite3_stmt* stmt;
    
    EnterCriticalSection(&csEventsDb);
    stmt = cache?evt_stmts_prep[SQL_EVT_STMT_BLOBSIZE_CACHE]:evt_stmts_prep[SQL_EVT_STMT_BLOBSIZE];
	sqlite3_bind_int(stmt, 1, (int)hDbEvent);
    if (sql_step(stmt)==SQLITE_ROW) {
		rc = sqlite3_column_int(stmt, 0);
        if (cache)
            log2("Got blob size from cache for event %d (%d)", (int)hDbEvent, rc);
        else
            log2("Got blob size event %d (%d)", (int)hDbEvent, rc);
    }
	sql_reset(stmt);
    LeaveCriticalSection(&csEventsDb);
    return rc;
}

int events_getBlobSize(WPARAM wParam, LPARAM lParam) {
	HANDLE hDbEvent = (HANDLE)wParam;
	int rc = events_getBlobSizeConditional(hDbEvent, 1);

	if (rc!=-1)
        return rc;
	return events_getBlobSizeConditional(hDbEvent, 0);;
}

static int events_getConditional(HANDLE hDbEvent, DBEVENTINFO *dbei, int cache) {
	int rc = 1;
    sqlite3_stmt* stmt;
    
	if(dbei==NULL||dbei->cbSize!=sizeof(DBEVENTINFO)) 
		return 1;
	EnterCriticalSection(&csEventsDb);
    stmt = cache?evt_stmts_prep[SQL_EVT_STMT_GET_CACHE]:evt_stmts_prep[SQL_EVT_STMT_GET];
	sqlite3_bind_int(stmt, 1, (int)hDbEvent);
	if (sql_step(stmt)==SQLITE_ROW) {
		size_t copySize = 0;
		const void *blob = sqlite3_column_blob(stmt, 4);
		const size_t size = sqlite3_column_int(stmt, 5);
		
		dbei->timestamp = (DWORD)sqlite3_column_int64(stmt, 1);
		dbei->flags = (DWORD)sqlite3_column_int(stmt, 2);
		dbei->eventType = (WORD)sqlite3_column_int(stmt, 3);
		dbei->szModule = (char*)sqlite3_column_text(stmt, 6);
		copySize = size<dbei->cbBlob ? size : dbei->cbBlob;
		CopyMemory(dbei->pBlob, blob, copySize);
		dbei->cbBlob = copySize;
		rc = 0;
		log2("Get event(%d)%s", (int)hDbEvent, cache?" from cache":"");
	}
	sql_reset(stmt);
	LeaveCriticalSection(&csEventsDb);
	return rc; 
}

int events_get(WPARAM wParam, LPARAM lParam) {
	HANDLE hDbEvent = (HANDLE)wParam;
	DBEVENTINFO *dbei = (DBEVENTINFO*)lParam;
    int rc = events_getConditional(hDbEvent, dbei, 1);
    
    if (!rc)
        return 0;
    return events_getConditional(hDbEvent, dbei, 0);
}

int events_markRead(WPARAM wParam, LPARAM lParam) {
	HANDLE hDbEvent = (HANDLE)lParam;
	int rc = -1;
	
	log1("Marking event read(%d)", (int)hDbEvent);
	EnterCriticalSection(&csEventsDb);
    sql_exec(g_sqlite, "BEGIN TRANSACTION;");
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_GETFLAGS], 1, (int)hDbEvent);
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_GETFLAGS])==SQLITE_ROW) {
		DWORD flags = (DWORD)sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_GETFLAGS], 0);
		
		sql_reset(evt_stmts_prep[SQL_EVT_STMT_GETFLAGS]);
        rc = flags;
        if (!(flags&DBEF_READ)) {
            flags|=DBEF_READ;
            sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_SETFLAGS], 1, (int)flags);
            sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_SETFLAGS], 2, (int)hDbEvent);
            if (sql_step(evt_stmts_prep[SQL_EVT_STMT_SETFLAGS])!=SQLITE_DONE) {
                rc = -1;   
            }
            sql_reset(evt_stmts_prep[SQL_EVT_STMT_SETFLAGS]);
        }
	}
	else sql_reset(evt_stmts_prep[SQL_EVT_STMT_GETFLAGS]);
    sql_exec(g_sqlite, "END TRANSACTION;");
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

static int events_getContactConditional(HANDLE hDbEvent, int cache) {
    int rc = -1;
    sqlite3_stmt* stmt;
    
    EnterCriticalSection(&csEventsDb);
    stmt = cache?evt_stmts_prep[SQL_EVT_STMT_GETCONTACT_CACHE]:evt_stmts_prep[SQL_EVT_STMT_GETCONTACT];
    sqlite3_bind_int(stmt, 1, (int)hDbEvent);
	if (sql_step(stmt)==SQLITE_ROW) {
		rc = sqlite3_column_int(stmt, 0);
        if (cache)
            log2("Got contact from event cache id %d (%d)", (int)hDbEvent, rc);
        else
            log2("Got contact from event id %d (%d)", (int)hDbEvent, rc);
	}
    sql_reset(stmt);
    LeaveCriticalSection(&csEventsDb);
    return rc;
}

int events_getContact(WPARAM wParam, LPARAM lParam) {
	HANDLE hDbEvent = (HANDLE)wParam;
	int rc = events_getContactConditional(hDbEvent, 1);
    
    if (rc!=-1)
        return rc;
	return events_getContactConditional(hDbEvent, 0);
}

int events_findFirst(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;
	int rc = 0;

	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDFIRST], 1, (int)hContact);
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_FINDFIRST])==SQLITE_ROW) {
		rc = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_FINDFIRST], 0);
		log1("Found first event(%d)", rc);
	}
	else {
		log1("Could not find first event(%d)", rc);
	}
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_FINDFIRST]);
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

int events_findFirstUnread(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;
	int rc = 0;
	DWORD flags = 0;

	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDFIRSTUNREAD], 1, (int)hContact);
	while (sql_step(evt_stmts_prep[SQL_EVT_STMT_FINDFIRSTUNREAD])==SQLITE_ROW) {
		flags = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_FINDFIRSTUNREAD], 0);
		if(!(flags&(DBEF_READ|DBEF_SENT))) {
			rc = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_FINDFIRSTUNREAD], 1);
			log2("Found first unread event(%d,%d)", (int)hContact, rc);
			sql_reset(evt_stmts_prep[SQL_EVT_STMT_FINDFIRSTUNREAD]);
			LeaveCriticalSection(&csEventsDb);
			return rc;
		}
	}
	log2("Could not find an unread event(%d,%d)", (int)hContact, rc);
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_FINDFIRSTUNREAD]);
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

int events_findLast(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;
	int rc = 0;

	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDLAST], 1, (int)hContact);
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_FINDLAST])==SQLITE_ROW) {
		rc = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_FINDLAST], 0);
		log1("Found last event(%d)", rc);
	}
	else {
		log1("Could not find last event(%d)", rc);
	}
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_FINDLAST]);
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

int events_findNext(WPARAM wParam, LPARAM lParam) {
	HANDLE hDbEvent = (HANDLE)wParam;
    int hContact = -1, rc = 0;
	
	if (hDbEvent==NULL) {
		return 0;
	}
    hContact = events_getContact(wParam, 0);
    if (hContact==-1) {
        return 0;
    }
	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDNEXT], 1, hContact);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDNEXT], 2, (int)hDbEvent);
	if (sql_step(evt_stmts_prep[SQL_EVT_STMT_FINDNEXT])==SQLITE_ROW) {
		rc = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_FINDNEXT], 0);
		log2("Found next event(%d,%d)", (int)hDbEvent, rc);
	}
	else {
		log2("Could not find next event(%d,%d)", (int)hDbEvent, rc);
	}
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_FINDNEXT]);
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

int events_findPrev(WPARAM wParam, LPARAM lParam) {
	HANDLE hDbEvent = (HANDLE)wParam;
	int hContact = -1, rc = 0;

	if (hDbEvent==NULL) {
		return 0;
	}
    hContact = events_getContact(wParam, 0);
    if (hContact==-1) {
        return 0;
    }
	EnterCriticalSection(&csEventsDb);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDPREV], 1, hContact);
	sqlite3_bind_int(evt_stmts_prep[SQL_EVT_STMT_FINDPREV], 2, (int)hDbEvent);
    rc = sql_step(evt_stmts_prep[SQL_EVT_STMT_FINDPREV]);
	if (rc==SQLITE_ROW) {
		rc = sqlite3_column_int(evt_stmts_prep[SQL_EVT_STMT_FINDPREV], 0);
		log1("Found previous event(%d)", rc);
	}
	else {
		log1("Could not find previous event(%d)", rc);
        rc = 0;
	}
	sql_reset(evt_stmts_prep[SQL_EVT_STMT_FINDPREV]);
	LeaveCriticalSection(&csEventsDb);
	return rc;
}

