/**/

/*

sqlite> create table settings(
   ...> id integer,
   ...> module varchar(255),
   ...> setting varchar(255),
   ...> type integer,
   ...> val any,
   ...> primary key (setting,module,id,type)
   ...> );
sqlite>
*/

#include "commonheaders.h"

char szDbPath[MAX_PATH];
struct MM_INTERFACE memoryManagerInterface;
sqlite3 * sql;
CRITICAL_SECTION csDb;

HANDLE hWriteSettingEvent;

static int DbGetSetting(WPARAM wParam, LPARAM lParam)
{
	int rc=1;
	HANDLE hContact = (HANDLE)wParam;
	DBCONTACTGETSETTING * gs = (DBCONTACTGETSETTING *)lParam;
	EnterCriticalSection(&csDb);
	if ( gs && gs->szModule != NULL && gs->szSetting != NULL && gs->pValue != NULL ) 
		rc=DbCache_GetSetting(hContact, gs->szModule, gs->szSetting, 0, gs->pValue);
	LeaveCriticalSection(&csDb);
	return rc;
}

static int DbGetSetticStatic(WPARAM wParam, LPARAM lParam)
{
	int rc=1;
	HANDLE hContact = (HANDLE)wParam;
	DBCONTACTGETSETTING * gs = (DBCONTACTGETSETTING *)lParam;
	EnterCriticalSection(&csDb);
	if ( gs && gs->szModule != NULL && gs->szSetting != NULL && gs->pValue != NULL ) 
		rc=DbCache_GetSetting(hContact, gs->szModule, gs->szSetting, 1, gs->pValue);
	LeaveCriticalSection(&csDb);
	return rc;
}

// assumed no lock owned, will lock, ws strings might not be safe, ws->value might be unsafe
static int DbWriteSettingWorker(HANDLE hContact, DBCONTACTWRITESETTING * ws)
{
	// write to cache
	int rc;
	EnterCriticalSection(&csDb);
	rc=DbCache_WriteSetting(hContact, ws->szModule, ws->szSetting, &ws->value);
	LeaveCriticalSection(&csDb);
	if ( rc == 0 ) {
		// this will always switch to the main thread
		NotifyEventHooks(hWriteSettingEvent, (WPARAM)hContact, (LPARAM) ws);
	}
	return rc;
}

static int DbWriteSetting(WPARAM wParam, LPARAM lParam)
{
	int rc=1;
	HANDLE hContact=(HANDLE)wParam;
	DBCONTACTWRITESETTING * ws=(DBCONTACTWRITESETTING*)lParam;
	if ( ws && ws->szModule != NULL && ws->szSetting != NULL ) rc=DbWriteSettingWorker(hContact,ws);
	return rc;
}

static int DbEnumSetting(WPARAM wParam, LPARAM lParam)
{
	return 1;
}

static int DbDeleteSetting(WPARAM wParam, LPARAM lParam)
{
	int rc=1;
	HANDLE hContact = (HANDLE)wParam;
	DBCONTACTGETSETTING * gs = (DBCONTACTGETSETTING *)lParam;
	EnterCriticalSection(&csDb);
	if ( gs && gs->szModule != NULL && gs->szSetting != NULL ) 
		rc=DbCache_DeleteSetting(hContact, gs->szModule, gs->szSetting);
	LeaveCriticalSection(&csDb);
	if ( rc == 0 ) {
		DBCONTACTWRITESETTING cws;
		cws.szModule=gs->szModule;
		cws.szSetting=gs->szSetting;
		cws.value.type=DBVT_DELETED;
		NotifyEventHooks(hWriteSettingEvent, (WPARAM) hContact, (LPARAM) &cws);
	}
	return rc;
}

static int DbFreeVariant(WPARAM wParam, LPARAM lParam)
{
	DBVARIANT *dbv=(DBVARIANT*)lParam;
	if ( dbv == 0 ) return 1;
	switch ( dbv->type ) {
		case DBVT_ASCIIZ:
		{
			if ( dbv->pszVal ) mir_free(dbv->pszVal);
			dbv->pszVal=0;
			break;
		}
		case DBVT_BLOB:
		{
			if ( dbv->pbVal ) mir_free(dbv->pbVal);
			dbv->pbVal=0;
			break;
		}
	}
	dbv->type=0;
	return 0;
}

static int DbContactFindFirst(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static int DbContactFindNext(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static int DbContactGetCount(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

// returns 0 if the database gets loaded, otherwise nonzero, only call once
int dbsqlite_LoadDatabase(char * profile)
{
	if ( sqlite3_open(profile,&sql) != SQLITE_OK ) return 1;
	InitializeCriticalSection(&csDb);
	// profile is core memory so make our own copy
	strncpy(szDbPath,profile,sizeof(szDbPath));
	// cache
	InitCache();
	// settings
	CreateServiceFunction(MS_DB_CONTACT_GETSETTING,DbGetSetting);
	CreateServiceFunction(MS_DB_CONTACT_GETSETTINGSTATIC,DbGetSetticStatic);	
	CreateServiceFunction(MS_DB_CONTACT_WRITESETTING,DbWriteSetting);	
	CreateServiceFunction(MS_DB_CONTACT_ENUMSETTINGS,DbEnumSetting);
	CreateServiceFunction(MS_DB_CONTACT_DELETESETTING,DbDeleteSetting);
	CreateServiceFunction(MS_DB_CONTACT_FREEVARIANT,DbFreeVariant);
	// settings events 
	hWriteSettingEvent=CreateHookableEvent(ME_DB_CONTACT_SETTINGCHANGED);
	// contacts
	CreateServiceFunction(MS_DB_CONTACT_FINDFIRST,DbContactFindFirst);
	CreateServiceFunction(MS_DB_CONTACT_FINDNEXT,DbContactFindNext);
	CreateServiceFunction(MS_DB_CONTACT_GETCOUNT,DbContactGetCount);
	// history
	// events
	// create a temp table to store data before flushing it.
	return 0;
}

// only called when the db should unload
int dbsqlite_UnloadDatabase(void)
{
	UninitCache();
	if ( sql != NULL ) sqlite3_close(sql);
	sql=NULL;
	DeleteCriticalSection(&csDb);
	return 0;
}