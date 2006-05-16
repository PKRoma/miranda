/**/

// db.c
int dbsqlite_LoadDatabase(char * profile);
int dbsqlite_UnloadDatabase(void);


// dbcache.c
void InitCache(void);
void UninitCache(void);
int DbCache_GetSetting(HANDLE hContact, const char * szModule, const char * szSetting, int staticAlloc, DBVARIANT * pValue);
int DbCache_WriteSetting(HANDLE hContact, const char * szModule, const char * szSetting, DBVARIANT * dbv);
int DbCache_DeleteSetting(HANDLE hContact, const char * szModule, const char * szSetting);