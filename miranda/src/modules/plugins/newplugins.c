/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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
// prototype for clists
typedef int (__cdecl * CList_Initialise) ( PLUGINLINK * ); 
// prototype for protocol plugins?

typedef struct { // can all be NULL
	HINSTANCE hInst;			
	Miranda_Plugin_Load Load;
	Miranda_Plugin_Unload Unload;
	Miranda_Plugin_Info Info;
	Database_Plugin_Info DbInfo;
	CList_Initialise clistlink;
	PLUGININFO * pluginInfo;	 // must be freed if hInst==NULL then its a copy
	DATABASELINK * dblink;		 // only valid during module being in memory	
} BASIC_PLUGIN_INFO;

#define PCLASS_FAILED	 0x1  	// not a valid plugin, or API is invalid, pluginname is valid
#define PCLASS_BASICAPI  0x2  	// has Load, Unload, MirandaPluginInfo() -> PLUGININFO seems valid, this dll is in memory.
#define PCLASS_DB	 	 0x4    // has DatabasePluginInfo() and is valid as can be, and PCLASS_BASICAPI has to be set too
#define PCLASS_LAST		 0x8    // this plugin should be unloaded after everything else
#define PCLASS_OK		 0x10   // plugin should be loaded, if DB means nothing
#define PCLASS_LOADED	 0x20   // Load() has been called, Unload() should be called.
#define PCLASS_STOPPED   0x40 	// wasn't loaded cos plugin name not on white list
#define PCLASS_CLIST 	 0x80  // a CList implementation

typedef struct pluginEntry {
	struct pluginEntry * prev, * next;
	char pluginname[64];
	unsigned int pclass; // PCLASS_*
	BASIC_PLUGIN_INFO bpi;
	struct pluginEntry * nextclass; 
} pluginEntry;

typedef struct {
	pluginEntry * first, * last;
} pluginEntryList;

PLUGINLINK pluginCoreLink;
char mirandabootini[MAX_PATH];
static DWORD mirandaVersion;
static pluginEntryList pluginListHead;
static pluginEntry * pluginListDb;
static pluginEntry * pluginListUI;
static HANDLE hPluginListHeap = NULL;
static pluginEntry * pluginDefModList[DEFMOD_HIGHEST+1]; // do not free this memory
static int askAboutIgnoredPlugins;

#define PLUGINDISABLELIST "PluginDisable"

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
#define CHECKAPI_NONE 	0
#define CHECKAPI_DB 	1
#define CHECKAPI_CLIST  2
static int checkAPI(char * plugin, BASIC_PLUGIN_INFO * bpi, DWORD mirandaVersion, int checkTypeAPI, int * exports)
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
			// basic API is present
			if ( checkTypeAPI == CHECKAPI_NONE ) {
				bpi->hInst=h;
				return 1;
			}
			// check for DB?
			if ( checkTypeAPI == CHECKAPI_DB ) {			
				bpi->DbInfo = (Database_Plugin_Info) GetProcAddress(h, "DatabasePluginInfo");
				if ( bpi->DbInfo ) {
					// fetch internal database function pointers
					bpi->dblink = bpi->DbInfo(NULL);
					// validate returned link structure
					if ( bpi->dblink && bpi->dblink->cbSize==sizeof(DATABASELINK) ) {
						bpi->hInst=h;
						return 1;
					}
					// had DB exports
					if ( exports != NULL ) *exports=1;
				} //if				
			} //if 
		
			// check clist ?
			if ( checkTypeAPI == CHECKAPI_CLIST ) {				
				bpi->clistlink = (CList_Initialise) GetProcAddress(h, "CListInitialise");
				if ( bpi->clistlink ) {					
					// nothing more can be done here, this export is a load function
					bpi->hInst=h;
					if ( exports != NULL ) *exports=1;
					return 1;
				} 
			}
		} // if
		if ( exports != NULL ) *exports=1;
	} //if 
	// not found, unload
	FreeLibrary(h);
	return 0;
}

// returns true if the given file is <anything>.dll exactly
static int valid_library_name(char * name)
{
	char * dot = strrchr(name, '.');
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
	rc=lstrcmpi(name,"dbx_") == 0;
	name[4]=x;
	return rc;
}

// returns true if the given file matches clist_*.dll
static int validguess_clist_name(char * name)
{
	int rc=0;	
	// argh evil
	char x = name[6];
	name[6]=0;
	rc=lstrcmpi(name,"clist_") == 0;
	name[6]=x;
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
		ZeroMemory(&p->bpi,sizeof(p->bpi));
	}
}

typedef BOOL (*SCAN_PLUGINS_CALLBACK) ( WIN32_FIND_DATA * fd, char * path, WPARAM wParam, LPARAM lParam );

static void enumPlugins(SCAN_PLUGINS_CALLBACK cb, WPARAM wParam, LPARAM lParam)
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
		hFind = FindFirstFile(search, &ffd);
		if ( hFind != INVALID_HANDLE_VALUE ) {
			do {
				if ( !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && valid_library_name(ffd.cFileName) ) 
				{ 
					cb(&ffd,exe,wParam,lParam);
				} //if
			} while ( FindNextFile(hFind, &ffd) );
			FindClose(hFind);
		} //if	
	}
}

// this is called by the db module to return all DBs plugins, then when it finds the one it likes the others are unloaded
static int PluginsEnum(WPARAM wParam, LPARAM lParam)
{
	PLUGIN_DB_ENUM * de = (PLUGIN_DB_ENUM *) lParam;
	pluginEntry * x = pluginListDb;
	if ( de == NULL || de->cbSize != sizeof(PLUGIN_DB_ENUM) || de->pfnEnumCallback == NULL ) return 1;
	while ( x != NULL ) {
		int rc = de->pfnEnumCallback(x->pluginname, x->bpi.dblink, de->lParam);
		if ( rc == DBPE_DONE ) {
			// this db has been picked, get rid of all the others
			pluginEntry * y = pluginListDb, * n;
			while ( y != NULL ) {
				n = y->nextclass;
				if ( x != y ) { 
					LL_Remove(&pluginListHead, y);
					Plugin_Uninit(y);
				}
				y = n;
			} // while			
			x->pclass |= PCLASS_LOADED | PCLASS_OK | PCLASS_LAST;
			return 0;
		} else if ( rc == DBPE_HALT ) return 1;
		x = x->nextclass;
	} // while
	return pluginListDb != NULL ? 1 : -1;
}


// hooked very late, after all the internal plugins, blah
static int UnloadNewPlugins(WPARAM wParam, LPARAM lParam)
{
	pluginEntry * p = pluginListHead.first;
	// unload everything but the special db/clist plugins
	while ( p != NULL ) {
		pluginEntry * q = p->next;
		if ( !(p->pclass&PCLASS_LAST) && p->pclass&PCLASS_OK ) 
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
		if ( !(p->pclass&PCLASS_DB) ) { 
			LL_Remove(&pluginListHead, p);
			Plugin_Uninit(p);
		}
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

// called in the first pass to create pluginEntry* structures and validate database plugins
static BOOL scanPluginsDir (WIN32_FIND_DATA * fd, char * path, WPARAM wParam, LPARAM lParam )
{
	int isdb = validguess_db_name(fd->cFileName);
	BASIC_PLUGIN_INFO bpi;
	pluginEntry * p = HeapAlloc(hPluginListHeap, HEAP_NO_SERIALIZE|HEAP_ZERO_MEMORY, sizeof(pluginEntry));
	p->next=NULL;					
	strncpy(p->pluginname, fd->cFileName, sizeof(p->pluginname));
	// plugin name suggests its a db module, load it right now
	if ( isdb ) {
		char buf[MAX_PATH];
		_snprintf(buf,sizeof(buf),"%s\\Plugins\\%s", path, fd->cFileName);
		if ( checkAPI(buf, &bpi, mirandaVersion, CHECKAPI_DB, NULL) ) {	
			// db plugin is valid
			p->pclass |= (PCLASS_DB|PCLASS_BASICAPI);
			// copy the dblink stuff
			p->bpi=bpi;
			// keep a faster list.
			if ( pluginListDb != NULL ) p->nextclass=pluginListDb;			
			pluginListDb=p;
		}  else {							
			// didn't have basic APIs or DB exports - failed.
			p->pclass |= PCLASS_FAILED;
		}
	} else if ( validguess_clist_name(fd->cFileName) ) {
		// keep a note of this plugin for later
		if ( pluginListUI != NULL ) p->nextclass=pluginListUI;
		pluginListUI=p;
		p->pclass |= PCLASS_CLIST;
	}
	// add it to the list
	LL_Insert(&pluginListHead, p);
	return TRUE;
}

// called before anything real is loaded, incl. database
int LoadNewPluginsModuleInfos(void)
{
	hPluginListHeap=HeapCreate(HEAP_NO_SERIALIZE, 0, 0);	
	mirandaVersion = (DWORD)CallService(MS_SYSTEM_GETVERSION, 0, 0);
	// 
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
	// remember where the mirandaboot.ini goes
	{
		char exe[MAX_PATH];
		char * slice;
		GetModuleFileName(NULL, exe, sizeof(exe));
		slice=strrchr(exe, '\\');
		if ( slice != NULL ) *slice=0;
		_snprintf(mirandabootini, sizeof(mirandabootini), "%s\\mirandaboot.ini", exe);
	}
	// look for all *.dll's
	enumPlugins(scanPluginsDir, 0, 0);
	// the database will select which db plugin to use, or fail if no profile is selected
	if (LoadDatabaseModule()) return 1;
	//  could validate the plugin entries here but internal modules arent loaded so can't call Load() in one pass
	return 0;
}

static void SetPluginOnWhiteList(char * pluginname, int allow)
{
	DBWriteContactSettingByte(NULL, PLUGINDISABLELIST, pluginname, allow ? 0 : 1);
}

// returns 1 if the plugin should be enabled within this profile, filename is always lower case
static int isPluginOnWhiteList(char * pluginname)
{
	int rc = DBGetContactSettingByte(NULL, PLUGINDISABLELIST, pluginname, 0);
	if ( rc != 0 && askAboutIgnoredPlugins ) {
		char buf[256];
		_snprintf(buf,sizeof(buf),Translate("'%s' is disabled, re-enable?"),pluginname);
		if ( MessageBox(NULL,buf,Translate("Re-enable Miranda plugin?"),MB_YESNO|MB_ICONQUESTION) == IDYES ) {
			SetPluginOnWhiteList(pluginname, 1);
			return 1;
		}
	}
	return rc == 0;
}


// used within LoadNewPluginsModule as an exception handler
int CListFailed(int result)
{
	// result = 0, no clist_* can be found
	MessageBox(0, result ? 
	Translate("Unable to start any of the installed contact list plugins, I even ignored your preferences for which contact list couldn't load any.") 
	: Translate("Can't find a contact list plugin! you need clist_classic or clist_mw.") , "", MB_OK | MB_ICONINFORMATION);
	return 1;
}

static int PluginOptionsInit(WPARAM wParam, LPARAM lParam);

static pluginEntry * getCListModule(char * exe, char * slice, int useWhiteList)
{
	pluginEntry * p = pluginListUI;
	BASIC_PLUGIN_INFO bpi;
	while ( p != NULL )
	{
		_snprintf(slice, &exe[MAX_PATH] - slice, "\\Plugins\\%s", p->pluginname);
		CharLower(p->pluginname);
		if ( useWhiteList ? isPluginOnWhiteList(p->pluginname) : 1 ) {
			if ( checkAPI(exe, &bpi, mirandaVersion, CHECKAPI_CLIST, NULL) ) {
				p->bpi = bpi;
				p->pclass |= PCLASS_LAST | PCLASS_OK | PCLASS_BASICAPI;						
				if ( bpi.clistlink(&pluginCoreLink) == 0 ) {
					p->bpi=bpi;
					p->pclass |= PCLASS_LOADED;
					return p;
					break;
				} else { 
					Plugin_Uninit(p); 
					LL_Remove(&pluginListHead, p);
				}
			} //if
		} //if
		p = p->nextclass;
	}
	return NULL;
}

int LoadNewPluginsModule(void)
{
	char exe[MAX_PATH];
	char * slice;
	pluginEntry * p, * q;
	pluginEntry * clist = NULL;
	int useWhiteList;

	// make full path to the plugin
	GetModuleFileName(NULL, exe, sizeof(exe));
	slice=strrchr(exe, '\\');
	if ( slice != NULL ) *slice=0;
	// remember some useful options
	askAboutIgnoredPlugins=(UINT) GetPrivateProfileInt("PluginLoader", "AskAboutIgnoredPlugins", 0, mirandabootini);
	// first load the clist cos alot of plugins need that to be present at Load()
	for ( useWhiteList = 1; useWhiteList >= 0 && clist == NULL; useWhiteList-- ) 
		clist=getCListModule(exe, slice, useWhiteList);
	/* the loop above will try and get one clist DLL to work, if all fail then just bail now */
	if ( clist == NULL ) 
		return CListFailed( pluginListUI ? 1 : 0 );
	/* enable and disable as needed  */
	p=pluginListUI;
	while ( p != NULL ) {
		SetPluginOnWhiteList(p->pluginname, clist != p ? 0 : 1 );
		p = p->nextclass;
	}
	/* now loop thru and load all the other plugins, do this in one pass */
	p=pluginListHead.first;
	while ( p != NULL ) {
		q = p->next;
		CharLower(p->pluginname);
		if ( !(p->pclass&PCLASS_LOADED) && !(p->pclass&PCLASS_DB) 
			&& !(p->pclass&PCLASS_CLIST) && isPluginOnWhiteList(p->pluginname) ) {
			BASIC_PLUGIN_INFO bpi;
			_snprintf(slice,&exe[sizeof(exe)] - slice, "\\Plugins\\%s", p->pluginname);			
			if ( checkAPI(exe, &bpi, mirandaVersion, CHECKAPI_NONE, NULL) ) {
				int rm = bpi.pluginInfo->replacesDefaultModule;					
				p->bpi = bpi;
				p->pclass |= PCLASS_OK | PCLASS_BASICAPI;
				if ( pluginDefModList[rm] == NULL ) {					
					if ( bpi.Load(&pluginCoreLink) == 0 ) p->pclass |= PCLASS_LOADED;
					else { 
						LL_Remove(&pluginListHead, p);
						Plugin_Uninit(p);
					}
					if ( rm ) pluginDefModList[rm]=p;
				} //if
				else { 
					SetPluginOnWhiteList(p->pluginname, 0);
					LL_Remove(&pluginListHead, p);
					Plugin_Uninit(p);
				}
			} else p->pclass |= PCLASS_FAILED;
		}
		p=q;
	} // while
	// hook shutdown after everything
	HookEvent(ME_SYSTEM_SHUTDOWN, UnloadNewPlugins);
	HookEvent(ME_OPT_INITIALISE, PluginOptionsInit);
	return 0;
}

static BOOL dialogListPlugins(WIN32_FIND_DATA * fd, char * path, WPARAM wParam, LPARAM lParam)
{
	LVITEM it;
	int iRow;
	HWND hwndList=(HWND)lParam;
	BASIC_PLUGIN_INFO pi;
	int exports=0;
	char buf[MAX_PATH];
	int isdb = 0;
	HINSTANCE gModule=NULL;
	_snprintf(buf,sizeof(buf),"%s\\Plugins\\%s",path,fd->cFileName);
	CharLower(fd->cFileName);
	gModule=GetModuleHandle(buf);
	if ( checkAPI(buf, &pi, mirandaVersion, CHECKAPI_NONE, &exports) == 0 ) {
		// failed to load anything, but if exports were good, show some info.
		return TRUE;
	}	
	isdb = pi.pluginInfo->replacesDefaultModule == DEFMOD_DB;
	ZeroMemory(&it, sizeof(it));
	it.mask=LVIF_TEXT | LVIF_PARAM;
	it.pszText=Translate(fd->cFileName);
	it.lParam=(LPARAM)pi.pluginInfo->replacesDefaultModule;
	iRow=ListView_InsertItem(hwndList,&it);
	if ( isPluginOnWhiteList(fd->cFileName) ) ListView_SetItemState(hwndList, iRow, !isdb ? 0x2000 : 0x3000, 0xF000);
	if ( iRow != (-1) ) {
		ListView_SetItemText(hwndList, iRow, 1, pi.pluginInfo->shortName);
		_snprintf(buf,sizeof(buf),"%d.%d.%d.%d", HIBYTE(HIWORD(pi.pluginInfo->version)), LOBYTE(HIWORD(pi.pluginInfo->version)), HIBYTE(LOWORD(pi.pluginInfo->version)), LOBYTE(LOWORD(pi.pluginInfo->version)));
		ListView_SetItemText(hwndList, iRow, 2, buf);
		ListView_SetItemText(hwndList, iRow, 3, Translate( gModule != NULL ? "Yes":"No" ));
		ListView_SetItemText(hwndList, iRow, 4, pi.pluginInfo->author);
		ListView_SetItemText(hwndList, iRow, 5, pi.pluginInfo->authorEmail);
		ListView_SetItemText(hwndList, iRow, 6, pi.pluginInfo->description);
		ListView_SetItemText(hwndList, iRow, 7, pi.pluginInfo->copyright);
		ListView_SetItemText(hwndList, iRow, 8, pi.pluginInfo->homepage);
	}
	FreeLibrary(pi.hInst);
	return TRUE;
}

static LRESULT CALLBACK ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC proc=(WNDPROC)GetWindowLong(hwnd,GWL_USERDATA);
	if ( msg == WM_WINDOWPOSCHANGING ) ShowScrollBar(hwnd,SB_HORZ,FALSE);
	return CallWindowProc(proc,hwnd,msg,wParam,lParam);
}

static char * PluginCutCopyright(char * buf)
{
	/* Some plugins include (C,c)opyright, which is fine but it looks stupid in the UI */
	char tmp[16];
	strncpy(tmp, buf, 9);
	tmp[9]=0;
	if ( lstrcmpi(tmp,"copyright") == 0 ) return buf+10;
	return buf;
}

static BOOL CALLBACK DlgPluginOpt(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
		{
			HWND hwndList=GetDlgItem(hwndDlg,IDC_PLUGLIST);
			LVCOLUMN col;			
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(hwndList,GWL_USERDATA, SetWindowLong(hwndList,GWL_WNDPROC, (LONG)ListView_WndProc));
			col.mask=LVCF_TEXT|LVCF_WIDTH;
			col.pszText=Translate("Plugin");
			col.cx=75;
			ListView_InsertColumn(hwndList,0,&col);
			
			col.pszText=Translate("Name");			
			ListView_InsertColumn(hwndList,1,&col);
		
			col.pszText=Translate("Version");
			col.cx=60;
			ListView_InsertColumn(hwndList,2,&col);
		
			col.pszText=Translate("Running");
			col.cx=1000;
			ListView_InsertColumn(hwndList,3,&col);
		
			col.pszText=Translate("Author");
			ListView_InsertColumn(hwndList,4,&col);		
		
			col.pszText=Translate("e-mail");
			ListView_InsertColumn(hwndList,5,&col);				
			
			col.pszText=Translate("Description");
			ListView_InsertColumn(hwndList,6,&col);				
			
			col.pszText=Translate("Copyright");
			ListView_InsertColumn(hwndList,7,&col);	
			
			col.pszText=Translate("Homepage");
			ListView_InsertColumn(hwndList,8,&col);	
			
			// XXX: Won't work on windows 95 without IE3+ or 4.70
			ListView_SetExtendedListViewStyleEx(hwndList, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT );
			// scan the plugin dir for plugins, cos 
			enumPlugins(dialogListPlugins,(WPARAM)hwndDlg,(LPARAM)hwndList);
			// sort out the headers
			ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE); // dll name
			ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE); // short name
			return TRUE;
		}
		case WM_NOTIFY: 
		{
			NMLISTVIEW * hdr = (NMLISTVIEW *) lParam;
			if ( hdr && hdr->hdr.code == LVN_ITEMCHANGED && hdr->uOldState != 0 
				&& (hdr->uNewState == 0x1000 || hdr->uNewState == 0x2000 ) && IsWindowVisible(hdr->hdr.hwndFrom) ) { 
				HWND hwndList=GetDlgItem(hwndDlg,IDC_PLUGLIST);								
				LVITEM it;
				int iRow;								
				ZeroMemory(&it,sizeof(it));				
				it.mask=LVIF_PARAM | LVIF_STATE;
				it.iItem = hdr->iItem;
				if ( ListView_GetItem(hwndList,&it) && it.lParam == DEFMOD_DB ) {
					ListView_SetItemState(hwndList, hdr->iItem, 0x3000, 0xF000);
					return FALSE;
				}
				// if enabling and replaces, find all other replaces and toggle off
				if ( hdr->uNewState&0x2000 && it.lParam )  {
					for ( iRow=0; iRow != (-1); ) {
						if ( iRow != hdr->iItem ) {
							LVITEM dt;						
							dt.mask = LVIF_PARAM;
							dt.iItem = iRow;
							if ( ListView_GetItem(hwndList,&dt) && dt.lParam == it.lParam ) {
								// the lParam is unset, so when the check is unset the clist block doesnt trigger
								LPARAM lParam = dt.lParam;
								dt.lParam = 0;
								ListView_SetItem(hwndList, &dt);
								ListView_SetItemState(hwndList, iRow, 0x1000, 0xF000);
								dt.lParam = lParam;
								ListView_SetItem(hwndList, &dt);
							}
						}
						iRow=ListView_GetNextItem(hwndList, iRow, LVNI_ALL);
					}
				} 
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}			
			if ( hdr && hdr->hdr.code == LVN_ITEMCHANGED && IsWindowVisible(hdr->hdr.hwndFrom) 
					&& hdr->iItem != (-1) ) {
				char buf[1024];
				int sel = hdr->uNewState&LVIS_SELECTED;
				HWND hwndList = GetDlgItem(hwndDlg, IDC_PLUGLIST);	
				// frame/about
				ListView_GetItemText(hwndList, hdr->iItem, 1, buf, sizeof(buf));				
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGININFOFRAME),sel ? buf : "");
				// author
				ListView_GetItemText(hwndList, hdr->iItem, 4, buf, sizeof(buf));
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINAUTHOR), sel ? buf : "");
				// author email
				ListView_GetItemText(hwndList, hdr->iItem, 5, buf, sizeof(buf));
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINEMAIL), sel ? buf : "");
				// description
				ListView_GetItemText(hwndList, hdr->iItem, 6, buf, sizeof(buf));
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINLONGINFO), sel ? buf : "");
				// copyright
				ListView_GetItemText(hwndList, hdr->iItem, 7, buf, sizeof(buf));
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINCPYR), sel ? PluginCutCopyright(buf) : "");
				// homepage
				ListView_GetItemText(hwndList, hdr->iItem, 8, buf, sizeof(buf));
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINURL), sel ? buf : "");				
			}
			if ( hdr && hdr->hdr.code == PSN_APPLY ) {				
				HWND hwndList=GetDlgItem(hwndDlg,IDC_PLUGLIST);
				int iRow;
				int iState;
				char buf[1024];
				for (iRow=0 ; iRow != (-1) ; ) {					
					ListView_GetItemText(hwndList, iRow, 0, buf, sizeof(buf));
					iState=ListView_GetItemState(hwndList, iRow, LVIS_STATEIMAGEMASK);
					SetPluginOnWhiteList(buf, iState&0x2000 ? 1 : 0);
					iRow=ListView_GetNextItem(hwndList, iRow, LVNI_ALL);					
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			if ( HIWORD(wParam) == STN_CLICKED ) { 
				switch (LOWORD(wParam)) {
					case IDC_PLUGINEMAIL:
					case IDC_PLUGINURL:
					{
						char buf[512];
						char * p = &buf[7];
						lstrcpy(buf,"mailto:");
						if ( GetWindowText(GetDlgItem(hwndDlg, LOWORD(wParam)), p, sizeof(buf) - 7) ) {							
							CallService(MS_UTILS_OPENURL,0,(LPARAM) (LOWORD(wParam)==IDC_PLUGINEMAIL ? buf : p) );
						}
						break;
					}
				} // switch
			}
			break;
		}
	}
	return FALSE;
}

static int PluginOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.hInstance=GetModuleHandle(NULL);
	odp.pfnDlgProc=DlgPluginOpt;
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_PLUGINS);
	odp.position=1300000000;	
	odp.pszTitle=Translate("Plugins");
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}










