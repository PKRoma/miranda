/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project,
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

//definitions
#define MS_CLIST_GETVERSION "CList/GetVersionType"

//include
#include "commonheaders.h"
#include "commonprototypes.h"
#include <time.h>
#include "skinEngine.h"
#include "version.h"

//current module prototypes
void  UninitSkinHotKeys();
void  GetDefaultFontSetting(int i,LOGFONT *lf,COLORREF *colour);
int   OnSkinLoad(WPARAM wParam, LPARAM lParam);
int	  LoadContactListModule(void);
int   LoadCLCModule(void);

void	cliCheckCacheItem(pdisplayNameCacheEntry pdnce);
void	cliFreeCacheItem( pdisplayNameCacheEntry p );
void	cliRebuildEntireList(HWND hwnd,struct ClcData *dat);
void	cliRecalcScrollBar(HWND hwnd,struct ClcData *dat);
int		cliHotKeysProcess(HWND hwnd,WPARAM wParam,LPARAM lParam);
int		cliHotkeysProcessMessage(WPARAM wParam,LPARAM lParam);
int		cliHotKeysRegister(HWND hwnd);
int		cliHotKeysUnregister(HWND hwnd);
void	cliOnCreateClc(void);
int		cli_AddItemToGroup(struct ClcGroup *group, int iAboveItem);
int		cli_AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText);

//current module global variables
HINSTANCE g_hInst = 0;
PLUGINLINK * pluginLink;
CLIST_INTERFACE *pcli;
struct MM_INTERFACE memoryManagerInterface;
struct LIST_INTERFACE li;

//current module private variables
static HANDLE hCListShutdown = 0;

//stored core interfaces

struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);
void (*saveLoadCluiGlobalOpts)(void);
void (*saveSortCLC) (HWND hwnd, struct ClcData *dat, int useInsertionSort );
int  (*saveAddItemToGroup)( struct ClcGroup *group, int iAboveItem );
int  (*saveAddInfoItemToGroup)(struct ClcGroup *group,int flags,const TCHAR *pszText);

LRESULT (CALLBACK *saveContactListControlWndProc )( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK cli_ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK cli_ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int (* saveTrayIconProcessMessage) ( WPARAM wParam, LPARAM lParam );

void ( *saveAddContactToTree)(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void cli_AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);

void ( *saveDeleteItemFromTree )(HWND hwnd, HANDLE hItem);
void cli_DeleteItemFromTree(HWND hwnd, HANDLE hItem);

void ( *saveFreeContact )( struct ClcContact* );
extern void cli_FreeContact( struct ClcContact* );

void ( *saveFreeGroup )( struct ClcGroup* );
void cli_FreeGroup( struct ClcGroup* );

void (*saveSaveStateAndRebuildList)(HWND hwnd, struct ClcData *dat);



char* cli_GetGroupCountsText(struct ClcData *dat, struct ClcContact *contact);
char* (*saveGetGroupCountsText)(struct ClcData *dat, struct ClcContact *contact);


void ( *saveChangeContactIcon)(HANDLE hContact,int iIcon,int add);
void cli_ChangeContactIcon(HANDLE hContact,int iIcon,int add);

LRESULT ( *saveProcessExternalMessages )(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT cli_ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);


PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
#ifndef _DEBUG
	#ifdef UNICODE
		"Modern Contact List (UNICODE)",
	#else
		"Modern Contact List",
	#endif

#else
	#ifdef UNICODE
			"Debug of Modern Contact List (UNICODE)",
	#else
			"Debug of Modern Contact List",
	#endif
#endif
	0,                              //will initiate later in MirandaPluginInfo
	"Display contacts, event notifications, protocol status with advantage visual modifications. Supported MW modifications, enchanced metacontact cooperation.",
	"Artem Shpynov, Anton Senko and Ricardo Pescuma Domenecci, based on clist_mw by Bethoven",
	"shpynov@nm.ru" ,
	"Copyright 2000-2006 Miranda-IM project ["__DATE__" "__TIME__"]",
	"http://miranda-im.org/download/details.php?action=viewfile&id=2103",
	UNICODE_AWARE,
	DEFMOD_CLISTALL
};

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
	g_hInst = hInstDLL;
	DisableThreadLibraryCalls(g_hInst);
	return TRUE;
}

int MakeVer(a,b,c,d)
{
    return (((((DWORD)(a))&0xFF)<<24)|((((DWORD)(b))&0xFF)<<16)|((((DWORD)(c))&0xFF)<<8)|(((DWORD)(d))&0xFF));
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION(0,4,3,42) )
	{
		return NULL;
	}
    pluginInfo.version=MakeVer(PRODUCT_VERSION);
	return &pluginInfo;
}



int SetDrawer(WPARAM wParam,LPARAM lParam)
{
	pDrawerServiceStruct DSS=(pDrawerServiceStruct)wParam;
	if (DSS->cbSize!=sizeof(*DSS)) return -1;
	if (DSS->PluginName==NULL) return -1;
	if (DSS->PluginName==NULL) return -1;
	if (!ServiceExists(DSS->GetDrawFuncsServiceName)) return -1;


	SED.cbSize=sizeof(SED);
	SED.PaintClc=(void (__cdecl *)(HWND,struct ClcData *,HDC,RECT *,int ,ClcProtoStatus *,HIMAGELIST))CallService(DSS->GetDrawFuncsServiceName,CLUI_EXT_FUNC_PAINTCLC,0);
	if (!SED.PaintClc) return -1;
	return 0;
}

int __declspec(dllexport) CListInitialise(PLUGINLINK * link)
{
	int rc=0;
	pluginLink=link;
	memset(&memoryManagerInterface,0,sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM)&memoryManagerInterface);

	// get the lists manager interface
	li.cbSize = sizeof(li);
	CallService(MS_SYSTEM_GET_LI, 0, (LPARAM)&li);

	CreateServiceFunction(MS_CLIST_GETCONTACTICON,GetContactIcon);

	// get the contact list interface
	pcli = ( CLIST_INTERFACE* )CallService(MS_CLIST_RETRIEVE_INTERFACE, 0, (LPARAM)g_hInst);
	if ( (int)pcli == CALLSERVICE_NOTFOUND ) {
		MessageBoxA( NULL, "This version of plugin requires Miranda 0.4.3 bld#42 or later", "Fatal error", MB_OK );
		return 1;
	}

	// OVERLOAD CLIST INTERFACE FUNCTIONS
	//
	//	Naming convention is:
	//  'cli*'  - new handler without default core service calling 
	//  'save*' - pointer to stored default parent handle
	//	'cli_*'	- new handler with default core service calling

	pcli->pfnCheckCacheItem		= (void (*)(ClcCacheEntryBase*)) cliCheckCacheItem;
	pcli->pfnFreeCacheItem		= (void(*)(ClcCacheEntryBase*)) cliFreeCacheItem;
	pcli->pfnTrayIconUpdateBase	= (void (*)(const char *szChangedProto)) cliTrayIconUpdateBase;	
	
	pcli->pfnInvalidateDisplayNameCacheEntry	= cliInvalidateDisplayNameCacheEntry;
	pcli->pfnTrayIconUpdateWithImageList		= cliTrayIconUpdateWithImageList;
	pcli->pfnCluiProtocolStatusChanged	= cliCluiProtocolStatusChanged;
	pcli->pfnHotkeysProcessMessage		= cliHotkeysProcessMessage;
	pcli->pfnBeginRenameSelection		= cliBeginRenameSelection;
	pcli->pfnTrayIconIconsChanged		= cliTrayIconIconsChanged;
	pcli->pfnCListTrayNotify	= cliCListTrayNotify;
	pcli->pfnCreateClcContact	= cliCreateClcContact;
	pcli->pfnCreateCacheItem	= cliCreateCacheItem;
	pcli->pfnGetRowBottomY		= cliGetRowBottomY;
	pcli->pfnGetRowHeight		= cliGetRowHeight;
	pcli->pfnGetRowTopY			= cliGetRowTopY;
	pcli->pfnGetRowTotalHeight	= cliGetRowTotalHeight;
	pcli->pfnInvalidateRect		= cliInvalidateRect;
	pcli->pfnGetCacheEntry		= cliGetCacheEntry;
	pcli->pfnOnCreateClc		= cliOnCreateClc;
	pcli->pfnHotKeysProcess		= cliHotKeysProcess;
	pcli->pfnHotKeysRegister	= cliHotKeysRegister;
	pcli->pfnHotKeysUnregister	= cliHotKeysUnregister;
	pcli->pfnPaintClc			= cliPaintClc;
	pcli->pfnRebuildEntireList	= cliRebuildEntireList;
	pcli->pfnRecalcScrollBar	= cliRecalcScrollBar;
	pcli->pfnRowHitTest			= cliRowHitTest;
	pcli->pfnScrollTo			= cliScrollTo;
	pcli->pfnShowHide			= cliShowHide;
	pcli->pfnHitTest			= cliHitTest;
	pcli->pfnCompareContacts	= cliCompareContacts;
	pcli->pfnBuildGroupPopupMenu= cliBuildGroupPopupMenu;
	pcli->pfnTrayIconSetToBase	= cliTrayIconSetToBase;
	pcli->pfnFindItem			= cliFindItem;
	pcli->pfnGetRowByIndex		= cliGetRowByIndex;

	//partialy overloaded - call default handlers from inside
	saveLoadCluiGlobalOpts		= pcli->pfnLoadCluiGlobalOpts;
	pcli->pfnLoadCluiGlobalOpts = cli_LoadCluiGlobalOpts;

	saveSortCLC					= pcli->pfnSortCLC;	
	pcli->pfnSortCLC			= cli_SortCLC;
	
	saveAddGroup				= pcli->pfnAddGroup; 
	pcli->pfnAddGroup			= cli_AddGroup;
	
	saveGetGroupCountsText		= pcli->pfnGetGroupCountsText;
	pcli->pfnGetGroupCountsText	= cli_GetGroupCountsText;

    saveAddContactToTree		= pcli->pfnAddContactToTree;  
	pcli->pfnAddContactToTree	= cli_AddContactToTree;

	saveAddInfoItemToGroup		= pcli->pfnAddInfoItemToGroup; 
	pcli->pfnAddInfoItemToGroup = cli_AddInfoItemToGroup;

	saveAddItemToGroup			= pcli->pfnAddItemToGroup; 
	pcli->pfnAddItemToGroup		= cli_AddItemToGroup;

	saveContactListWndProc		= pcli->pfnContactListWndProc; 
	pcli->pfnContactListWndProc = cli_ContactListWndProc;

	saveDeleteItemFromTree		= pcli->pfnDeleteItemFromTree; 
	pcli->pfnDeleteItemFromTree = cli_DeleteItemFromTree;

	saveFreeContact				= pcli->pfnFreeContact; 
	pcli->pfnFreeContact		= cli_FreeContact;

	saveFreeGroup				= pcli->pfnFreeGroup; 
	pcli->pfnFreeGroup			= cli_FreeGroup;
	
	saveChangeContactIcon		= pcli->pfnChangeContactIcon;
	pcli->pfnChangeContactIcon	= cli_ChangeContactIcon;
	
	saveTrayIconProcessMessage		= pcli->pfnTrayIconProcessMessage; 
	pcli->pfnTrayIconProcessMessage	= cli_TrayIconProcessMessage;
	
	saveSaveStateAndRebuildList		= pcli->pfnSaveStateAndRebuildList;
	pcli->pfnSaveStateAndRebuildList= cli_SaveStateAndRebuildList;

	saveContactListControlWndProc		= pcli->pfnContactListControlWndProc;
	pcli->pfnContactListControlWndProc	= cli_ContactListControlWndProc;

	saveProcessExternalMessages			= pcli->pfnProcessExternalMessages; 
	pcli->pfnProcessExternalMessages	= cli_ProcessExternalMessages;	

	memset(&SED,0,sizeof(SED));
	CreateServiceFunction(CLUI_SetDrawerService,SetDrawer);

	///test///
	LoadModernButtonModule();
	LoadSkinModule();
	rc=LoadContactListModule();
	if (rc==0) rc=LoadCLCModule();
	LoadMoveToGroup();
	TRACE("CListInitialise ClistMW...Done\r\n");
	return rc;
}

// never called by a newer plugin loader.
int __declspec(dllexport) Load(PLUGINLINK * link)
{
	TRACE("Load ClistMW\r\n");
	MessageBoxA(0,"You Running Old Miranda, use >30-10-2004 version!","Modern Clist",0);
	CListInitialise(link);
	return 1;
}
int __declspec(dllexport) Unload(void)
{
	TRACE("Unloading ClistMW\r\n");	
	if (IsWindow(pcli->hwndContactList)) DestroyWindow(pcli->hwndContactList);
	UninitCustomMenus();
	UnloadAvatarOverlayIcon();
	UninitSkinHotKeys();
	UnhookEvent(gl_event_hSkinLoaded);
	UnhookAll();
	UnloadSkinModule();
	FreeRowCell();
	pcli->hwndContactList=0;
	UnhookAll();
	TRACE("Unloading ClistMW COMPLETE\r\n");
	return 0;
}

typedef struct _HookRec
{
  HANDLE hHook;
  char * HookStr;
} HookRec;
//UnhookAll();

HookRec * hooksrec=NULL;
DWORD hooksRecAlloced=0;

#undef HookEvent
#undef UnhookEvent

HANDLE mod_HookEvent(char *EventID,MIRANDAHOOK HookProc)
{
	HookRec * hr=NULL;
	DWORD i;
	//1. Find free
	for (i=0;i<hooksRecAlloced;i++)
	{
		if (hooksrec[i].hHook==NULL)
		{
			hr=&(hooksrec[i]);
			break;
		}
	}
	if (hr==NULL)
	{
		//2. Need realloc
		hooksrec=(HookRec*)mir_realloc(hooksrec,sizeof(HookRec)*(hooksRecAlloced+1));
		hr=&(hooksrec[hooksRecAlloced]);
		hooksRecAlloced++;
	}
	//3. Hook and rec
	hr->hHook=pluginLink->HookEvent(EventID,HookProc);
	hr->HookStr=NULL;
#ifdef _DEBUG
	if (hr->hHook) hr->HookStr=mir_strdup(EventID);
#endif
	//3. Hook and rec
	return hr->hHook;
}

int mod_UnhookEvent(HANDLE hHook)
{
	DWORD i;
	//1. Find free
	
	for (i=0;i<hooksRecAlloced;i++)
	{
		if (hooksrec[i].hHook==hHook)
		{
			pluginLink->UnhookEvent(hHook);
			hooksrec[i].hHook=NULL;
			if (hooksrec[i].HookStr) mir_free(hooksrec[i].HookStr);
			return 1;
		}
	}
	return 0;
}

int UnhookAll()
{
	DWORD i;
	TRACE("Unhooked Events:\n");
	if (!hooksrec) return 0;
	for (i=0;i<hooksRecAlloced;i++)
	{
		if (hooksrec[i].hHook!=NULL)
		{
			pluginLink->UnhookEvent(hooksrec[i].hHook);
			hooksrec[i].hHook=NULL;
			if (hooksrec[i].HookStr)
			{
				TRACE(hooksrec[i].HookStr);
				TRACE("\n");
				mir_free(hooksrec[i].HookStr);
			}
		}
	}
	mir_free(hooksrec);
	hooksRecAlloced=0;
	return 1;
}

#define HookEvent(a,b)  mod_HookEvent(a,b)
#define UnhookEvent(a)  mod_UnhookEvent(a)
