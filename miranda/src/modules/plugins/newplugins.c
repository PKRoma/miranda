/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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
#include "../../core/commonheaders.h"

// basic export prototypes
typedef int (__cdecl * Miranda_Plugin_Load) ( PLUGINLINK * );
typedef int (__cdecl * Miranda_Plugin_Unload) ( void );
// version control
typedef PLUGININFO * (__cdecl * Miranda_Plugin_Info) ( DWORD mirandaVersion );
// prototype for databases
typedef DATABASELINK * (__cdecl * Database_Plugin_Info) ( void * reserved );
// prototype for protocol plugins?

typedef struct { // can all be NULL
	HINSTANCE hInst;			
	Miranda_Plugin_Load Load;
	Miranda_Plugin_Unload Unload;
	Miranda_Plugin_Info Info;
	Database_Plugin_Info DbInfo;
	PLUGININFO * pluginInfo;
	DATABASELINK * dblink;		 // only valid during module being in memory
} BASIC_PLUGIN_INFO;

#define PCLASS_FAILED	 0x1  // not a valid plugin, or API is invalid, pluginname is valid
#define PCLASS_WHITELIST 0x2  // plugin needs to be checked against the whitelist still
#define PCLASS_BASICAPI  0x4  // has Load, Unload, MirandaPluginInfo() -> PLUGININFO seems valid, this dll is in memory.
#define PCLASS_DB	 	 0x8  // has DatabasePluginInfo() and is valid as can be, and PCLASS_BASICAPI has to be set too
#define PCLASS_LAST		 0x10 // this plugin should be unloaded after everything else
#define PCLASS_OK		 0x20 // if db, then the plugin was loaded, if not then this plugin is okay to load.
#define PCLASS_CONFLICT  0x40 // this plugin was unloaded cos it wants to replace an already replaced module
#define PCLASS_LOADED	 0x80 // Load() has been called, Unload() should be called.

typedef struct pluginEntry {
	struct pluginEntry * prev, * next;
	char pluginname[64];
	unsigned int pclass; // PCLASS_*
	BASIC_PLUGIN_INFO bpi;
} pluginEntry;

typedef struct {
	pluginEntry * first, * last;
} pluginEntryList;

PLUGINLINK pluginCoreLink;
static DWORD mirandaVersion;
static pluginEntryList pluginListHead;
static HANDLE hPluginListHeap = NULL;
static pluginEntry * pluginDefModList[DEFMOD_HIGHEST+1]; // do not free this memory

#define PLUGINWHITELIST "PluginWhiteList"

int LoadDatabaseModule(void);	

static void LL_Insert(pluginEntryList * head, pluginEntry * p)
{
	if ( head->first == NULL ) head->first = p;
	if ( head->last != NULL ) head->last->next = p;
	p->prev = head->last;
	head->last = p;
	p->next = NULL;
}

static void LL_Remove(pluginEntryList * head, pluginEntry * q)
{
	if ( q->prev != NULL ) q->prev->next = q->next;
	if ( q->next != NULL ) q->next->prev = q->prev;
	if ( head->first == q ) head->first = q->next;
	if ( head->last == q ) head->last = q->prev;
		
	q->next = q->prev = NULL;
}

// returns true if the API exports were good, otherwise, passed in data is returned
static int checkAPI(char * plugin, BASIC_PLUGIN_INFO * bpi, DWORD mirandaVersion, int checkDbAPI)
{
	HINSTANCE h = NULL;
	h = LoadLibrary(plugin);
	if ( h == NULL ) return 0;
	// loaded, check for exports
	bpi->Load = (Miranda_Plugin_Load) GetProcAddress(h, "Load");
	bpi->Unload = (Miranda_Plugin_Unload) GetProcAddress(h, "Unload");
	bpi->Info = (Miranda_Plugin_Info) GetProcAddress(h, "MirandaPluginInfo");
	// if they were present
	if ( bpi->Load && bpi->Unload && bpi->Info ) 
	{ 
		PLUGININFO * pi = bpi->Info(mirandaVersion);
		if ( pi && pi->cbSize==sizeof(PLUGININFO) && pi->shortName && pi->description 
				&& pi->author && pi->authorEmail && pi->copyright && pi->homepage
				&& pi->replacesDefaultModule <= DEFMOD_HIGHEST )
		{
			bpi->pluginInfo = pi;
			if ( checkDbAPI ) {
				bpi->DbInfo = (Database_Plugin_Info) GetProcAddress(h, "DatabasePluginInfo");
				if ( bpi->DbInfo ) {
					// fetch internal database function pointers
					bpi->dblink = bpi->DbInfo(NULL);
					// validate returned link structure
					if ( bpi->dblink && bpi->dblink->cbSize==sizeof(DATABASELINK) ) {
						bpi->hInst=h;
						return 1;
					}
				} //if				
			} else { 
				bpi->hInst=h;
				return 1;
			} 
		} //if
	}
	// not found, unload
	FreeLibrary(h);
	return 0;
}

// returns true if the given file is <anything>.dll exactly
static int valid_library_name(char * name)
{
	char * dot = strchr(name, '.');
	if ( dot != NULL && lstrcmpi(dot+1,"dll") == 0) {
		if ( dot[4] == 0 ) return 1;
	}
	return 0;
}

// returns true if the given file matches dbx_*.dll, which is used to LoadLibrary()
static int validguess_db_name(char * name)
{
	int rc=0;	
	// this is ONLY SAFE because name -> ffd.cFileName == MAX_PATH
	char x = name[4];
	name[4]=0;
	rc=lstrcmp(name,"dbx_") == 0;
	name[4]=x;
	return rc;
}

// perform any API related tasks to freeing
static void Plugin_Uninit(pluginEntry * p)
{	
	// if it was an installed database plugin, call its unload
	if ( p->pclass&PCLASS_DB ) p->bpi.dblink->Unload( p->pclass&PCLASS_OK );
	// if the basic API check had passed, call Unload if Load() was ever called
	if ( p->pclass&PCLASS_LOADED ) p->bpi.Unload();
	// release the library
	if ( p->bpi.hInst != NULL ) {
		FreeLibrary(p->bpi.hInst);
		p->bpi.hInst=NULL;
	}
}

// called in the first pass to create pluginEntry* structures and validate database plugins
static void scanPlugins(void)
{
	char exe[MAX_PATH];
	char search[MAX_PATH];
	char * p = 0;	
	// get miranda's exe path
	GetModuleFileName(NULL, exe, sizeof(exe));
	// find the last \ and null it out, this leaves no trailing slash
	p = strrchr(exe, '\\');
	if ( p ) *p=0;
	// create the search filter
	_snprintf(search,sizeof(search),"%s\\Plugins\\*.dll", exe);
	{
		// FFFN will return filenames for things like dot dll+ or dot dllx 
		HANDLE hFind=INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA ffd;
		mirandaVersion = CallService(MS_SYSTEM_GETVERSION, 0, 0);
		hFind = FindFirstFile(search, &ffd);
		if ( hFind != INVALID_HANDLE_VALUE ) {
			do {
				if ( !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && valid_library_name(ffd.cFileName) ) 
				{ 
					// dont know if file is a valid DLL
					// dont know if file has a valid basic API
					// dont know if file has a valid higher API
					// guessing name based on class is only to stop LoadLibrary() per file at this point
					int isdb = validguess_db_name(ffd.cFileName);
					BASIC_PLUGIN_INFO bpi;
					pluginEntry * p = HeapAlloc(hPluginListHeap, HEAP_NO_SERIALIZE+HEAP_ZERO_MEMORY, sizeof(pluginEntry));
					p->next=NULL;					
					strncpy(p->pluginname, ffd.cFileName, sizeof(p->pluginname));
					// plugin name suggests its a db module, load it right now
					if ( isdb ) {
						char buf[MAX_PATH];
						_snprintf(buf,sizeof(buf),"%s\\Plugins\\%s", exe, ffd.cFileName);
						if ( checkAPI(buf, &bpi, mirandaVersion, 1) ) {							
							// db plugin is valid
							p->pclass |= (PCLASS_DB|PCLASS_BASICAPI);
							// copy the dblink stuff
							p->bpi=bpi;
						}  else {							
							// didn't have basic APIs or DB exports - failed.
							p->pclass = PCLASS_FAILED;
						}
					} else {
						// this file has to be loaded later - dont know if it should be loaded, DB isnt up yet.
						p->pclass = PCLASS_WHITELIST;
					}
					// add it to the list
					LL_Insert(&pluginListHead, p);
				} //if
			} while ( FindNextFile(hFind, &ffd) );
			FindClose(hFind);
		} //if	
	}
}

static int PluginsEnum(WPARAM wParam, LPARAM lParam)
{
	PLUGIN_DB_ENUM * de = (PLUGIN_DB_ENUM *) lParam;
	if ( de && de->cbSize==sizeof(PLUGIN_DB_ENUM) && de->pfnEnumCallback ) {
		pluginEntry * p = pluginListHead.first;
		while ( p != NULL ) {
			if ( p->pclass&PCLASS_DB ) {				
				int rc = de->pfnEnumCallback(p->pluginname, p->bpi.dblink, de->lParam);
				if ( rc == DBPE_DONE ) 
				{
					pluginEntry * pp, * q =NULL;
					// remember this db as loaded
					p->pclass |= (PCLASS_LOADED|PCLASS_OK|PCLASS_LAST);
					// remove any other database plugins from memory
					q = pluginListHead.first;
					while ( q != NULL ) {
						pp = q->next;
						if ( q->pclass&PCLASS_DB && p != q ) 
						{
							LL_Remove(&pluginListHead, q);
							Plugin_Uninit(q);							
							HeapFree(hPluginListHeap, HEAP_NO_SERIALIZE, q);
						} //if
						q=pp; 
					} //while
					return 0;
				} // if
				else if ( rc == DBPE_HALT ) return 1;
			} //if
			p = p->next;
		} //while
		return 1;
	}
	return 1;
}

// hooked very late, after all the internal plugins, blah
static int UnloadNewPlugins(WPARAM wParam, LPARAM lParam)
{
	pluginEntry * p = pluginListHead.first;
	// unload everything but the special db/clist plugins
	while ( p != NULL ) {
		pluginEntry * q = p->next;
		if ( !(p->pclass&PCLASS_DB) || !(p->pclass&PCLASS_LAST) ) 
		{
			LL_Remove(&pluginListHead, p);
			Plugin_Uninit(p);
		}
		p = q;
	}
	return 0;
}

// called at the end of module chain unloading, just modular engine left at this point
int UnloadNewPluginsModule(void)
{
	pluginEntry * p = pluginListHead.first;
	// unload everything but the DB
	while ( p != NULL ) {
		pluginEntry * q = p->next;
		LL_Remove(&pluginListHead, p);
		if ( !(p->pclass&PCLASS_DB) ) Plugin_Uninit(p);
		p = q;
	}
	// unload the DB
	p = pluginListHead.first;
	while ( p != NULL ) {
		pluginEntry * q = p->next;
		Plugin_Uninit(p);
		p = q;
	}
	if ( hPluginListHeap ) HeapDestroy(hPluginListHeap);
	hPluginListHeap=0;
	return 0;
}

static int PluginsGetDefaultArray(WPARAM wParam, LPARAM lParam)
{
	return (int) &pluginDefModList;
}

// called before anything real is loaded, incl. database
int LoadNewPluginsModuleInfos(void)
{
	hPluginListHeap=HeapCreate(HEAP_NO_SERIALIZE, 0, 0);	
	CreateServiceFunction(MS_PLUGINS_ENUMDBPLUGINS, PluginsEnum);
	CreateServiceFunction(MS_PLUGINS_GETDISABLEDEFAULTARRAY, PluginsGetDefaultArray);
	// make sure plugins can get internal core APIs
	pluginCoreLink.CallService=CallService;
	pluginCoreLink.ServiceExists=ServiceExists;
	pluginCoreLink.CreateServiceFunction=CreateServiceFunction;
	pluginCoreLink.CreateTransientServiceFunction=CreateServiceFunction;
	pluginCoreLink.DestroyServiceFunction=DestroyServiceFunction;
	pluginCoreLink.CreateHookableEvent=CreateHookableEvent;
	pluginCoreLink.DestroyHookableEvent=DestroyHookableEvent;
	pluginCoreLink.HookEvent=HookEvent;
	pluginCoreLink.HookEventMessage=HookEventMessage;
	pluginCoreLink.UnhookEvent=UnhookEvent;
	pluginCoreLink.NotifyEventHooks=NotifyEventHooks;
	pluginCoreLink.SetHookDefaultForHookableEvent=SetHookDefaultForHookableEvent;
	pluginCoreLink.CallServiceSync=CallServiceSync;
	pluginCoreLink.CallFunctionAsync=CallFunctionAsync;
	// look for all *.dll's
	scanPlugins();
	// the database will select which db plugin to use, or fail if no profile is selected
	if (LoadDatabaseModule()) return 1;
	//  could validate the plugin entries here but internal modules arent loaded so can't call Load() in one pass
	return 0;
}

// returns 1 if the plugin should be enabled within this profile, filename is always lower case
static int isPluginOnWhiteList(char * pluginname)
{
	//DBGetContactSettingByte(NULL, PLUGINWHITELIST, pluginname, 0);
	return 1;
}

// used within LoadNewPluginsModule as an exception handler
int CListFailed(int present)
{
	MessageBox(0,present ? "hey clist.dll is present but failed to load -- I need a UI!":"clist.dll is missing, you need to install it." ,"CList missing!",MB_ICONWARNING);
	return 1;
}

// called after the first pass, to load normal plugins as well as clist, it is all done in
// the second pass since the first pass loads the db and now the whitelist for which plugins
// to actually load is present
int LoadNewPluginsModule(void)
{
	{
		pluginEntry * p = pluginListHead.first;
		pluginEntry * clist = NULL;
		char exe[MAX_PATH];
		char * sz = 0;

		GetModuleFileName(NULL, exe, sizeof(exe));
		sz = strrchr(exe, '\\');
		if ( sz ) *sz=0;
		// so far we have a double link list of plugin names that match *.dll
		// there is at least one db driver loaded, but no clist and that has to load first
		// because most plugins assume it is loaded
		while ( p != NULL ) {
			BASIC_PLUGIN_INFO bpi;
			_snprintf(sz, &exe[sizeof(exe)] - sz, "\\Plugins\\%s", p->pluginname);	
			CharLower(p->pluginname);
			if ( p->pclass&PCLASS_WHITELIST && isPluginOnWhiteList(p->pluginname) ) 
			{
				if ( checkAPI(exe, &bpi, mirandaVersion, 0) ) 
				{					
					int rm = bpi.pluginInfo->replacesDefaultModule;
					// copy the LoadLibrary() handle and GetProcAddress() returns
					p->bpi = bpi;
					// if it didnt replace anything, mark it ok
					if ( rm == 0 ) p->pclass |= PCLASS_OK;
					else {
						p->pclass |= ( pluginDefModList[rm] ? PCLASS_CONFLICT : PCLASS_OK );
						if ( pluginDefModList[rm] ) Plugin_Uninit(p);
						else pluginDefModList[rm] = p;
					} //if					
				}					
				else 
				{
					p->pclass |= PCLASS_FAILED;
				}
			} //if
			p = p->next;
		} //while

		// now all plugins that were supposed to be loaded, have PCLASS_OK -- make sure an external clist exists
		clist = pluginDefModList[DEFMOD_CLISTALL];
		if ( clist != NULL && clist->pclass&PCLASS_OK ) {
			if ( clist->bpi.Load(&pluginCoreLink) == 0 ) clist->pclass |= (PCLASS_LOADED|PCLASS_LAST);
			else return CListFailed(1);
		} 
		else return CListFailed(0);

		// now load all the plugins, a plugin might already have PCLASS_LOADED
		p = pluginListHead.first;
		while ( p != NULL ) {
			if ( p->pclass&PCLASS_OK && !(p->pclass&PCLASS_LOADED) ) 
			{
				if ( p->bpi.Load(&pluginCoreLink) == 0 ) p->pclass |= PCLASS_LOADED;
			} //if
			p = p->next;
		} //while
	}
	// hook shutdown after everything
	HookEvent(ME_SYSTEM_SHUTDOWN, UnloadNewPlugins);
	return 0;
}


