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
//extern void SortCLC( HWND hwnd, struct ClcData *dat, int useInsertionSort );

extern int sfnFindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
extern HMENU BuildGroupPopupMenu(struct ClcGroup *group);
struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);


void (*savedLoadCluiGlobalOpts)(void);
extern void LoadCluiGlobalOpts(void);
void (*saveSortCLC) (HWND hwnd, struct ClcData *dat, int useInsertionSort );
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

void (*saveSaveStateAndRebuildList)(HWND hwnd, struct ClcData *dat);


char* (*saveGetGroupCountsText)(struct ClcData *dat, struct ClcContact *contact);
char* GetGroupCountsText(struct ClcData *dat, struct ClcContact *contact)
{
	char * res;
	lockdat;
	res=saveGetGroupCountsText(dat, contact);
	ulockdat;
	return res;
}


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
extern ClcCacheEntryBase* fnGetCacheEntry(HANDLE hContact);
static struct ClcContact* fnCreateClcContact( void )
{
	return (struct ClcContact*)mir_calloc(1, sizeof( struct ClcContact ) );
}

static ClcCacheEntryBase* fnCreateCacheItem( HANDLE hContact )
{
	pdisplayNameCacheEntry p = (pdisplayNameCacheEntry)mir_calloc( 1, sizeof( displayNameCacheEntry ));
	
	if ( p )
	{
		memset(p,0,sizeof( displayNameCacheEntry ));
		p->hContact = hContact;
		InvalidateDisplayNameCacheEntryByPDNE(hContact,p,0); //TODO should be in core
		p->szSecondLineText=NULL;
		p->szThirdLineText=NULL;
		p->plSecondLineText=NULL;
		p->plThirdLineText=NULL;
	}
	return (ClcCacheEntryBase*)p;
}



void InvalidateDisplayNameCacheEntry(HANDLE hContact)
{	
	pdisplayNameCacheEntry p;
	//if (IsBadWritePtr((void*)hContact,sizeof(displayNameCacheEntry)))
		p = (pdisplayNameCacheEntry) pcli->pfnGetCacheEntry(hContact);
	//else 
	//	p=(pdisplayNameCacheEntry)hContact; //handle give us incorrect hContact on GetCacheEntry;
	if (p)
		InvalidateDisplayNameCacheEntryByPDNE(hContact,p,0);
	return;
}

extern TCHAR *parseText(TCHAR *stzText);
extern int LoadModernButtonModule();
extern void SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat);
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

	pcli->pfnCheckCacheItem = (void ( * )( ClcCacheEntryBase* )) CheckPDNCE;
	pcli->pfnCListTrayNotify = CListTrayNotify;
	pcli->pfnCreateClcContact = fnCreateClcContact;
	pcli->pfnCreateCacheItem = fnCreateCacheItem;
	pcli->pfnFreeCacheItem = (void( * )( ClcCacheEntryBase* ))FreeDisplayNameCacheItem;
	pcli->pfnGetRowBottomY = RowHeights_GetItemBottomY;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnGetRowTopY = RowHeights_GetItemTopY;
	pcli->pfnGetRowTotalHeight = RowHeights_GetTotalHeight;
	pcli->pfnInvalidateRect = skinInvalidateRect;
	savedLoadCluiGlobalOpts=pcli->pfnLoadCluiGlobalOpts; pcli->pfnLoadCluiGlobalOpts=LoadCluiGlobalOpts;
	pcli->pfnGetCacheEntry=fnGetCacheEntry;

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

	pcli->pfnCluiProtocolStatusChanged=fnCluiProtocolStatusChanged;
	pcli->pfnBeginRenameSelection=BeginRenameSelection;
	pcli->pfnHitTest=HitTest;
	pcli->pfnCompareContacts=CompareContacts;

	pcli->pfnBuildGroupPopupMenu=BuildGroupPopupMenu;
	pcli->pfnInvalidateDisplayNameCacheEntry=InvalidateDisplayNameCacheEntry;

	saveTrayIconProcessMessage=pcli->pfnTrayIconProcessMessage; pcli->pfnTrayIconProcessMessage=TrayIconProcessMessage;
	pcli->pfnTrayIconUpdateBase=(void (*)( const char *szChangedProto ))TrayIconUpdateBase;
	pcli->pfnTrayIconUpdateWithImageList=TrayIconUpdateWithImageList;
	pcli->pfnTrayIconSetToBase=TrayIconSetToBase;
	pcli->pfnTrayIconIconsChanged=TrayIconIconsChanged;

    pcli->pfnFindItem=sfnFindItem;

	pcli->pfnGetRowByIndex=GetRowByIndex;

	saveSaveStateAndRebuildList=pcli->pfnSaveStateAndRebuildList;
	pcli->pfnSaveStateAndRebuildList=SaveStateAndRebuildList;

	saveSortCLC=pcli->pfnSortCLC;	pcli->pfnSortCLC=SortCLC;
	saveAddGroup = pcli->pfnAddGroup; pcli->pfnAddGroup = AddGroup;
	saveGetGroupCountsText=pcli->pfnGetGroupCountsText;
	pcli->pfnGetGroupCountsText=GetGroupCountsText;
    savedAddContactToTree=pcli->pfnAddContactToTree;  pcli->pfnAddContactToTree=AddContactToTree;
	saveAddInfoItemToGroup = pcli->pfnAddInfoItemToGroup; pcli->pfnAddInfoItemToGroup = AddInfoItemToGroup;
	saveAddItemToGroup = pcli->pfnAddItemToGroup; pcli->pfnAddItemToGroup = AddItemToGroup;
	saveContactListControlWndProc = pcli->pfnContactListControlWndProc; pcli->pfnContactListControlWndProc = ContactListControlWndProc;
	saveContactListWndProc = pcli->pfnContactListWndProc; pcli->pfnContactListWndProc = ContactListWndProc;
	saveDeleteItemFromTree = pcli->pfnDeleteItemFromTree; pcli->pfnDeleteItemFromTree = DeleteItemFromTree;
	saveFreeContact = pcli->pfnFreeContact; pcli->pfnFreeContact = FreeContact;
	saveFreeGroup = pcli->pfnFreeGroup; pcli->pfnFreeGroup = FreeGroup;
	saveProcessExternalMessages = pcli->pfnProcessExternalMessages; pcli->pfnProcessExternalMessages = ProcessExternalMessages;

	saveChangeContactIcon = pcli->pfnChangeContactIcon; pcli->pfnChangeContactIcon = ChangeContactIcon;

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
extern void UnloadAvatarOverlayIcon();
extern void FreeRowCell ();
extern void UninitCustomMenus(void);
int __declspec(dllexport) Unload(void)
{
	TRACE("Unloading ClistMW\r\n");	
	if (IsWindow(pcli->hwndContactList)) DestroyWindow(pcli->hwndContactList);
	UninitCustomMenus();
	UnloadAvatarOverlayIcon();
	UninitSkinHotKeys();
	UnhookEvent(hSkinLoaded);
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

#define HookEvent(a,b)  MyHookEvent(a,b)
#define UnhookEvent(a)  MyUnhookEvent(a)
