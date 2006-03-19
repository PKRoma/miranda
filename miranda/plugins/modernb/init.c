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

#include "commonheaders.h"
#include "commonprototypes.h"

#include <time.h>

#include "skinEngine.h"
extern HANDLE hSkinLoaded;
#include "version.h"

HINSTANCE g_hInst = 0;
PLUGINLINK * pluginLink;
CLIST_INTERFACE *pcli;
struct MM_INTERFACE memoryManagerInterface;
struct LIST_INTERFACE li;
static HANDLE hCListShutdown = 0;
extern int LoadMoveToGroup();
int OnSkinLoad(WPARAM wParam, LPARAM lParam);
void UninitSkinHotKeys();

void  CalcEipPosition( struct ClcData *dat, struct ClcContact *contact, struct ClcGroup *group, POINT *result);
void  CheckPDNCE(pdisplayNameCacheEntry pdnce);
int   CListTrayNotify(MIRANDASYSTRAYNOTIFY *msn);
void  FreeDisplayNameCacheItem( pdisplayNameCacheEntry p );
void  GetDefaultFontSetting(int i,LOGFONT *lf,COLORREF *colour);
int   GetWindowVisibleState(HWND hWnd, int iStepX, int iStepY);
int   HotKeysProcess(HWND hwnd,WPARAM wParam,LPARAM lParam);
int   HotkeysProcessMessage(WPARAM wParam,LPARAM lParam);
int   HotKeysRegister(HWND hwnd);
extern int GetContactIcon(WPARAM wParam,LPARAM lParam);
void  RebuildEntireList(HWND hwnd,struct ClcData *dat);
void  RecalcScrollBar(HWND hwnd,struct ClcData *dat);
int   UnRegistersAllHotkey(HWND hwnd);

extern int GetContactIcon(WPARAM wParam,LPARAM lParam);
extern void TrayIconUpdateBase(char *szChangedProto);
extern void TrayIconSetToBase(char *szPreferredProto);
extern void TrayIconIconsChanged(void);
extern void fnCluiProtocolStatusChanged(int status,const unsigned char * proto);
extern int sfnFindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
extern HMENU BuildGroupPopupMenu(struct ClcGroup *group);
struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);


void (*savedLoadCluiGlobalOpts)(void);
extern void LoadCluiGlobalOpts(void);

int ( *saveAddItemToGroup )( struct ClcGroup *group, int iAboveItem );
int AddItemToGroup(struct ClcGroup *group, int iAboveItem);

int ( *saveAddInfoItemToGroup)(struct ClcGroup *group,int flags,const TCHAR *pszText);
int AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText);

LRESULT ( CALLBACK *saveContactListControlWndProc )( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int (* saveTrayIconProcessMessage) ( WPARAM wParam, LPARAM lParam );

void ( *savedAddContactToTree)(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);

void ( *saveDeleteItemFromTree )(HWND hwnd, HANDLE hItem);
void DeleteItemFromTree(HWND hwnd, HANDLE hItem);

void ( *saveFreeContact )( struct ClcContact* );
extern void FreeContact( struct ClcContact* );

void ( *saveFreeGroup )( struct ClcGroup* );
void FreeGroup( struct ClcGroup* );

void ( *saveChangeContactIcon)(HANDLE hContact,int iIcon,int add);
void ChangeContactIcon(HANDLE hContact,int iIcon,int add);

LRESULT ( *saveProcessExternalMessages )(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);

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
	"Artem Shpynov and Ricardo Pescuma Domenecci, based on clist_mw by Bethoven",
	"shpynov@nm.ru" ,
	"Copyright 2000-2005 Miranda-IM project ["__DATE__" "__TIME__"]",
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

int LoadContactListModule(void);
//int UnLoadContactListModule(void);
int LoadCLCModule(void);
void LoadCLUIModule(void);

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

static struct ClcContact* fnCreateClcContact( void )
{
	return (struct ClcContact*)mir_calloc(1, sizeof( struct ClcContact ) );
}

static ClcCacheEntryBase* fnCreateCacheItem( HANDLE hContact )
{
	pdisplayNameCacheEntry p = (pdisplayNameCacheEntry)mir_calloc( 1, sizeof( displayNameCacheEntry ));
	if ( p )
	{
		p->hContact = hContact;
		InvalidateDisplayNameCacheEntryByPDNE(hContact,p,0); //TODO should be in core
	}
	return (ClcCacheEntryBase*)p;
}

extern TCHAR *parseText(TCHAR *stzText);

int __declspec(dllexport) CListInitialise(PLUGINLINK * link)
{
	int rc=0;
	pluginLink=link;
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	// get the internal malloc/free()
	TRACE("CListInitialise ClistMW\r\n");
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
LBL_Error:
		MessageBoxA( NULL, "This version of plugin requires Miranda 0.4.3 bld#49 or later", "Fatal error", MB_OK );
		return 1;
	}
	if ( pcli->version < 3 )
		goto LBL_Error;

	pcli->pfnCheckCacheItem = (void ( * )( ClcCacheEntryBase* )) CheckPDNCE;
//	pcli->pfnCListTrayNotify = CListTrayNotify;
	pcli->pfnCreateClcContact = fnCreateClcContact;
	pcli->pfnCreateCacheItem = fnCreateCacheItem;
	pcli->pfnFindItem = sfnFindItem;
	pcli->pfnFreeCacheItem = (void( * )( ClcCacheEntryBase* ))FreeDisplayNameCacheItem;
	pcli->pfnGetRowBottomY = RowHeights_GetItemBottomY;
	pcli->pfnGetRowByIndex = GetRowByIndex;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnGetRowTopY = RowHeights_GetItemTopY;
	pcli->pfnGetRowTotalHeight = RowHeights_GetTotalHeight;
	pcli->pfnGetWindowVisibleState = GetWindowVisibleState;
	pcli->pfnInvalidateRect = skinInvalidateRect;
	pcli->pfnOnCreateClc = LoadCLUIModule;
	pcli->pfnHotKeysProcess = HotKeysProcess;
	pcli->pfnHotkeysProcessMessage = HotkeysProcessMessage;
	pcli->pfnHotKeysRegister = HotKeysRegister;
	pcli->pfnHotKeysUnregister = UnRegistersAllHotkey;
	pcli->pfnPaintClc = PaintClc;
	pcli->pfnRebuildEntireList = RebuildEntireList;
	pcli->pfnRecalcScrollBar = RecalcScrollBar;
	pcli->pfnRowHitTest = RowHeights_HitTest;
	pcli->pfnScrollTo = ScrollTo;
	pcli->pfnShowHide = ShowHide;

	pcli->pfnCluiProtocolStatusChanged = fnCluiProtocolStatusChanged;
	pcli->pfnBeginRenameSelection = BeginRenameSelection;
	pcli->pfnHitTest = HitTest;
	pcli->pfnCompareContacts = CompareContacts;
	pcli->pfnBuildGroupPopupMenu = BuildGroupPopupMenu;

	pcli->pfnTrayIconUpdateBase=(void (*)( const char *szChangedProto ))TrayIconUpdateBase;
	pcli->pfnTrayIconUpdateWithImageList=TrayIconUpdateWithImageList;
	pcli->pfnTrayIconSetToBase=TrayIconSetToBase;
	pcli->pfnTrayIconIconsChanged=TrayIconIconsChanged;

	saveAddGroup = pcli->pfnAddGroup; pcli->pfnAddGroup = AddGroup;
	savedAddContactToTree=pcli->pfnAddContactToTree;  pcli->pfnAddContactToTree=AddContactToTree;
	saveAddInfoItemToGroup = pcli->pfnAddInfoItemToGroup; pcli->pfnAddInfoItemToGroup = AddInfoItemToGroup;
	saveAddItemToGroup = pcli->pfnAddItemToGroup; pcli->pfnAddItemToGroup = AddItemToGroup;
	saveChangeContactIcon = pcli->pfnChangeContactIcon; pcli->pfnChangeContactIcon = ChangeContactIcon;
	saveContactListControlWndProc = pcli->pfnContactListControlWndProc; pcli->pfnContactListControlWndProc = ContactListControlWndProc;
	saveContactListWndProc = pcli->pfnContactListWndProc; pcli->pfnContactListWndProc = ContactListWndProc;
	saveDeleteItemFromTree = pcli->pfnDeleteItemFromTree; pcli->pfnDeleteItemFromTree = DeleteItemFromTree;
	saveFreeContact = pcli->pfnFreeContact; pcli->pfnFreeContact = FreeContact;
	saveFreeGroup = pcli->pfnFreeGroup; pcli->pfnFreeGroup = FreeGroup;
	savedLoadCluiGlobalOpts = pcli->pfnLoadCluiGlobalOpts; pcli->pfnLoadCluiGlobalOpts = LoadCluiGlobalOpts;
	saveProcessExternalMessages = pcli->pfnProcessExternalMessages; pcli->pfnProcessExternalMessages = ProcessExternalMessages;
	saveTrayIconProcessMessage = pcli->pfnTrayIconProcessMessage; pcli->pfnTrayIconProcessMessage = TrayIconProcessMessage;

	memset(&SED,0,sizeof(SED));
	CreateServiceFunction(CLUI_SetDrawerService,SetDrawer);

	///test///
	LoadSkinModule();
	rc=LoadContactListModule();
	if (rc==0) rc=LoadCLCModule();

	LoadMoveToGroup();
	//BGModuleLoad();

	//CallTest();
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
	//BGModuleUnload();
	//    UnLoadContactListModule();
	UninitSkinHotKeys();
	UnhookEvent(hSkinLoaded);
	UnloadSkinModule();

	pcli->hwndContactList=0;
	TRACE("***&&& NEED TO UNHOOK ALL EVENTS &&&***\r\n");
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

HANDLE MyHookEvent(char *EventID,MIRANDAHOOK HookProc)
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

int MyUnhookEvent(HANDLE hHook)
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
	return 1;
}

#define HookEvent(a,b)  MyHookEvent(a,b)
#define UnhookEvent(a)  MyUnhookEvent(a)
