/**/

#include "commonheaders.h"

extern char szDbPath[MAX_PATH];
extern struct MM_INTERFACE memoryManagerInterface;
extern sqlite3 * sql = NULL;
extern CRITICAL_SECTION csDb;

static UINT hCacheFlushTimer;
void CALLBACK DbCache_Flush(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime);

#define XXX(msg) MessageBox(0,(char*)msg,"",0);

// assumes csDb owned
void DbFlushTableCache()
{
	char buf[] = "CREATE TEMP TABLE settingcache (" \
	"id integer, module varchar(255), setting varchar(255), type integer, val any," \
	"primary key(setting, id, module, type)" \
	");";
	sqlite3_exec(sql, "DROP TABLE settingcache;", NULL, NULL, NULL);
	sqlite3_exec(sql, buf, NULL, NULL, NULL);
}

void InitCache(void)
{
	DbFlushTableCache();
}

void UninitCache(void)
{
	// it will kill the timer itself
	DbCache_Flush(NULL,WM_TIMER,0,0);
}

// given a DBVARIANT return a promoted number or error - returns 1 if successful
int DbCache_GetVariant(DBVARIANT * dbv, int * val)
{
	switch ( dbv->type ) {
		case DBVT_BYTE: 
		{ 
			*val=dbv->bVal;
			break;
		}
		case DBVT_WORD: 
		{ 
			*val=dbv->wVal;
			break;
		}
		case DBVT_DWORD:
		{
			*val=dbv->dVal;
			break;
		}
		default:
			return 0;
	} // switch
	return 1;
}

// assumes main thread, will call csDb
void CALLBACK DbCache_Flush(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
	int rc;
	int value=0;
	sqlite3_stmt * st = NULL;
	EnterCriticalSection(&csDb);
	KillTimer(NULL, hCacheFlushTimer);
	hCacheFlushTimer=0;
	sqlite3_exec(sql, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	rc=sqlite3_prepare(sql, "SELECT * from settingcache;", -1, &st, NULL);
	while ( sqlite3_step(st) == SQLITE_ROW ) {
		sqlite3_stmt * row = NULL;
		if ( sqlite3_prepare(sql, "REPLACE INTO settings VALUES(?,?,?,?,?);", -1, &row, NULL) == SQLITE_OK ) {	
			// bind_* is one based, column_* is zero based, sigh.
			HANDLE hContact=(HANDLE)sqlite3_column_int(st,0);
			const char * szModule=sqlite3_column_text(st,1);			
			const char * szSetting=sqlite3_column_text(st,2);
			const int type=sqlite3_column_int(st,3);
			// convert the select * into an insert/replace using the same data from the result row
			sqlite3_bind_int(row, 1, (int)hContact);
			sqlite3_bind_text(row, 2, szModule, strlen(szModule), SQLITE_STATIC);
			sqlite3_bind_text(row, 3, szSetting, strlen(szSetting), SQLITE_STATIC);
			sqlite3_bind_int(row, 4, type);
			switch ( sqlite3_column_type(st, 4) ) {
				case SQLITE_INTEGER:
				{
					sqlite3_bind_int(row, 5, sqlite3_column_int(st, 4));
					break;
				}
				case SQLITE_TEXT:
				{
					const char * text = sqlite3_column_text(st, 4);
					sqlite3_bind_text(row, 5, text, strlen(text), SQLITE_STATIC);
					break;
				}
				case SQLITE_BLOB:
				{
					const void * blob = sqlite3_column_blob(st, 4);
					const size_t size = sqlite3_column_bytes(st, 4);
					sqlite3_bind_blob(row, 5, blob, size, SQLITE_STATIC);
					break;
				}
			} //switch
			// execute it
			sqlite3_step(row);
			// free query
			sqlite3_finalize(row);
		}
	} //while
	sqlite3_finalize(st);
	sqlite3_exec(sql, "COMMIT;", NULL, NULL, NULL);
	// delete and rebuild the mem table
	DbFlushTableCache();
	LeaveCriticalSection(&csDb);
	OutputDebugString("DbCache_Flush() \n");
}

// assumes caller did some validation, but not on ->value, assumes csDb owned - 0 on success
int DbCache_WriteSetting(HANDLE hContact, const char * szModule, const char * szSetting, DBVARIANT * dbv)
{
	sqlite3_stmt * st = NULL;
	int rc;
	int value=0;
	rc=sqlite3_prepare(sql, "REPLACE INTO settingcache VALUES(?,?,?,?,?);", -1, &st, NULL);
	if ( rc != SQLITE_OK ) return 1;
	// bind the data into the statement
	sqlite3_bind_int(st, 1, (int)hContact);
	sqlite3_bind_text(st, 2, szModule, strlen(szModule), SQLITE_STATIC);
	sqlite3_bind_text(st, 3, szSetting, strlen(szSetting), SQLITE_STATIC);
	sqlite3_bind_int(st, 4, (int)dbv->type);
	// convert the variant
	if ( DbCache_GetVariant(dbv, &value) ) sqlite3_bind_int(st, 5, value);
	else {
		if ( dbv->type == DBVT_ASCIIZ && dbv->pszVal != NULL ) sqlite3_bind_text(st, 5, dbv->pszVal, strlen(dbv->pszVal), SQLITE_STATIC);
		else if ( dbv->type == DBVT_BLOB && dbv->pbVal != NULL ) sqlite3_bind_blob(st, 5, dbv->pbVal, dbv->cpbVal, SQLITE_STATIC);
		else {
			sqlite3_finalize(st);
			return 1;
		}
	}
	// execute
	rc=sqlite3_step(st);
	sqlite3_finalize(st);
	st=NULL;
	if ( rc == SQLITE_DONE) {
		// kill any timer and figure out when to do a real write
		KillTimer(NULL, hCacheFlushTimer);
		hCacheFlushTimer=SetTimer(NULL, 0, 1000 * 5, DbCache_Flush);
	} else {
		OutputDebugString("failed to write.. \n");
	}	
	return rc;
}

// given a sqlite3_stmt* fills reply either dynamically or static
int __inline DbCache_SetVariant(sqlite3_stmt * st, DBVARIANT * dbv, int staticAlloc)
{
	// row should consist of type, val - columns are zero based
	int type = sqlite3_column_int(st, 0);
	dbv->type=type;
	switch ( type ) {
		case DBVT_ASCIIZ:
		{
			const char * p = sqlite3_column_text(st, 1);
			if ( p != NULL ) {
				size_t len = strlen(p) + 1;		
				size_t copylen = staticAlloc ? ( len < dbv->cchVal ? len : dbv->cchVal ) : len;
				if ( !staticAlloc ) dbv->pszVal=mir_alloc( len );
				memmove(dbv->pszVal, p, copylen );
				return 0;
			}
			break;
		}
		case DBVT_BLOB:
		{
			const void * p = sqlite3_column_blob(st, 1);
			if ( p != NULL ) {
				size_t len = sqlite3_column_bytes(st, 1);		
				size_t copylen = staticAlloc ? ( len < dbv->cpbVal ? len : dbv->cpbVal ) : len;
				if ( !staticAlloc ) dbv->pbVal=mir_alloc( len );
				memmove(dbv->pbVal, p, copylen );
				return 0;
			}
			break;
		}
		case DBVT_BYTE:
		{
			dbv->bVal=(BYTE)sqlite3_column_int(st, 1);
			return 0;
		}
		case DBVT_WORD:
		{
			dbv->wVal=(WORD)sqlite3_column_int(st, 1);
			return 0;
		}
		case DBVT_DWORD:
		{
			dbv->dVal=(DWORD)sqlite3_column_int(st, 1);
			return 0;
		}
	}
	return 1;
}

// assumes csDb is owned, if mem cache is empty, will make a disk request
int DbCache_GetSetting(HANDLE hContact, const char * szModule, const char * szSetting, int staticAlloc, 
	DBVARIANT * pValue)
{
	/* First the memory table 'settingcache' if the row is found, return that, if nothing is found
	query the 'settings' table which is on disk.
	*/
	int rc = 1;
	sqlite3_stmt * st = NULL;
	if ( sqlite3_prepare(sql, "SELECT type,val FROM settingcache WHERE setting = ? AND module = ? AND id = ?;", -1, &st, NULL) != SQLITE_OK ) {
		return 1;
	}
	sqlite3_bind_text(st, 1, szSetting, strlen(szSetting), SQLITE_STATIC);
	sqlite3_bind_text(st, 2, szModule, strlen(szModule), SQLITE_STATIC);
	sqlite3_bind_int(st, 3, (int) hContact);
	if ( sqlite3_step(st) == SQLITE_ROW ) {
		rc=DbCache_SetVariant(st, pValue, staticAlloc);
		sqlite3_finalize(st);
		st=NULL;
		return rc;
	}
	sqlite3_finalize(st);
	/* the setting wasnt in the write cache, so now look on disk */
	if ( sqlite3_prepare(sql, "SELECT type,val FROM settings WHERE setting = ? AND module = ? AND id = ?;", -1, &st, NULL) != SQLITE_OK ) {
		return 1;
	}
	sqlite3_bind_text(st, 1, szSetting, strlen(szSetting), SQLITE_STATIC);
	sqlite3_bind_text(st, 2, szModule, strlen(szModule), SQLITE_STATIC);
	sqlite3_bind_int(st, 3, (int) hContact);
	if ( sqlite3_step(st) == SQLITE_ROW ) {
		rc=DbCache_SetVariant(st, pValue, staticAlloc);
	}
	sqlite3_finalize(st);
	return rc;
}

// assumes csDb is owned
int DbCache_DeleteSetting(HANDLE hContact, const char * szModule, const char * szSetting)
{
	int rc=1;
	sqlite3_stmt * st = NULL;
	// delete any buffered write cos it doesnt matter anymore
	if ( sqlite3_prepare(sql, "DELETE FROM settingcache WHERE setting = ? AND module = ? AND id = ?;", -1, &st, NULL) != SQLITE_OK ) {
		return 1;
	}
	sqlite3_bind_text(st, 1, szSetting, strlen(szSetting), SQLITE_STATIC);
	sqlite3_bind_text(st, 2, szModule, strlen(szModule), SQLITE_STATIC);
	sqlite3_bind_int(st, 3, (int) hContact);
	rc=sqlite3_step(st);
	sqlite3_finalize(st);
	// now delete it from disk, success of this operation is what counts
	if ( sqlite3_prepare(sql, "DELETE FROM settings WHERE setting = ? AND module = ? AND id = ?;", -1, &st, NULL) != SQLITE_OK ) {
		return 1;
	}
	// feed in the args
	sqlite3_bind_text(st, 1, szSetting, strlen(szSetting), SQLITE_STATIC);
	sqlite3_bind_text(st, 2, szModule, strlen(szModule), SQLITE_STATIC);
	sqlite3_bind_int(st, 3, (int) hContact);
	// execute
	rc=sqlite3_step(st) == SQLITE_OK ? 0 : 1;
	sqlite3_finalize(st);
	return rc;
}

















