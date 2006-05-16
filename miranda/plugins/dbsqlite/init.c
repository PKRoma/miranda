/*

*/

#include "commonheaders.h"

extern struct MM_INTERFACE memoryManagerInterface;
PLUGINLINK *pluginLink;

int dbsqlite_getCaps(int flags)
{
	return 0;
}

int dbsqlite_getFriendlyName(char * buf, size_t cch, int shortName)
{
	strncpy(buf,shortName ? "sqlite profile":"Database support for an sqlite powered profile", cch);
	return 0;
}

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
int dbsqlite_makeDatabase(char * profile, int * error)
{
	char sql_make_database[] = "create table settings (" 
	"id integer,module varchar(255), setting varchar(255), " 
	"type integer, val any, primary key(setting,module,id,type));" ;
	sqlite3 * sql = NULL;
	int rc;
	rc=sqlite3_open(profile, &sql);
	if ( rc == SQLITE_OK ) {
		rc=sqlite3_exec(sql, sql_make_database, NULL, NULL, NULL);
	}
	sqlite3_close(sql);
	rc = ( rc == SQLITE_OK ) ? 0 : 1;
	if ( error != NULL ) *error=rc;
	return rc;
}

//static const char zMagicHeader[] = "SQLite format 3";
int dbsqlite_grokHeader(char * profile, int * error)
{
	/* sqlite docs say:  "If the database file does not exist, then a new database will be created as needed".
	but there is  szMagicHeader but its static, anyway we'll just detect the header some evil way for now.*/
	HANDLE hFile=CreateFile(profile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	int rc=1;
	int err=EGROKPRF_CANTREAD;
	if ( hFile != INVALID_HANDLE_VALUE ) {
		BOOL r;
		char buf[32];
		DWORD dwRead;
		ZeroMemory(buf,sizeof(buf));
		r=ReadFile(hFile,buf,sizeof(buf),&dwRead,NULL);
		CloseHandle(hFile);
		if ( r && memcmp(buf,"SQLite format 3", 16)==0 ) rc=0;
		else err= r ? EGROKPRF_UNKHEADER : EGROKPRF_CANTREAD;
	} 
	if ( error != NULL) *error=err;	
	return rc;
}
#include <stdio.h>
int dbsqlite_Load(char * profile, void * link)
{
	pluginLink=(PLUGINLINK*)link;
	// grab mmi interface
	ZeroMemory(&memoryManagerInterface,sizeof(memoryManagerInterface));
	CallService(MS_SYSTEM_GET_MMI,0,(LPARAM)&memoryManagerInterface);
	// now load the db and inject the APIs, this might fail though
	return dbsqlite_LoadDatabase(profile);;
}

int dbsqlite_Unload(int wasLoaded)
{
	if ( !wasLoaded ) return 0;
	// only unload if loaded
	return dbsqlite_UnloadDatabase();
}

DATABASELINK dblink = {
	sizeof(DATABASELINK),
	dbsqlite_getCaps,
	dbsqlite_getFriendlyName,
	dbsqlite_makeDatabase,
	dbsqlite_grokHeader,
	dbsqlite_Load,
	dbsqlite_Unload
};

PLUGININFO plugInfo = {
	sizeof(PLUGININFO),
	"dbsqlite",
	PLUGIN_MAKE_VERSION(0,2,0,0),
	"Provides profile support for sqlite databases to be used as profiles: global settings, contacts, history, etc",
	"egodust",
	"egodust@users.sourceforge.net",
	"Miranda-IM project, powered by sqlite (http://sqlite.org)",
	"",
	0,
	DEFMOD_DB
};

__declspec(dllexport) DATABASELINK* DatabasePluginInfo(void * reserved)
{
	return &dblink;
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &plugInfo;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
	return 1;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}


