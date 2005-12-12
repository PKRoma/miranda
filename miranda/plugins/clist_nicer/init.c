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

HINSTANCE g_hInst = 0;
PLUGINLINK *pluginLink;
CLIST_INTERFACE* pcli = NULL;

struct LIST_INTERFACE li;
struct MM_INTERFACE memoryManagerInterface;

HMENU BuildGroupPopupMenu( struct ClcGroup* group );
struct ClcContact* CreateClcContact( void );

int AddEvent(WPARAM wParam, LPARAM lParam);
int RemoveEvent(WPARAM wParam, LPARAM lParam);

void GetDefaultFontSetting(int i, LOGFONT *lf, COLORREF *colour);

void ( *saveLoadClcOptions )(HWND hwnd,struct ClcData *dat);
void LoadClcOptions(HWND hwnd,struct ClcData *dat);

int ( *saveAddContactToGroup )(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);
int AddContactToGroup(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);

struct ClcGroup* ( *saveAddGroup )(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);
struct ClcGroup* AddGroup(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);

LRESULT ( *saveProcessExternalMessages )(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ProcessExternalMessages(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

PLUGININFO pluginInfo = {
#if defined(_UNICODE)
	sizeof(PLUGININFO), "CList Nicer+ (Unicode)", PLUGIN_MAKE_VERSION(0, 5, 1, 16), 
#else
	sizeof(PLUGININFO), "CList Nicer+", PLUGIN_MAKE_VERSION(0, 5, 1, 15), 
#endif    
		"Display contacts, event notifications, protocol status", 
		"Pixel, egoDust, cyreve, Nightwish", "", "Copyright 2000-2005 Miranda-IM project", "http://www.miranda-im.org", 
		UNICODE_AWARE, 
		DEFMOD_CLISTALL
};

int _DebugPopup(HANDLE hContact, const char *fmt, ...)
{
	POPUPDATA ppd;
	va_list va;
	char    debug[1024];
	int     ibsize = 1023;

	va_start(va, fmt);
	_vsnprintf(debug, ibsize, fmt, va);

	if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
		ZeroMemory((void *)&ppd, sizeof(ppd));
		ppd.lchContact = hContact;
		ppd.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
		if(hContact != 0)
			strncpy(ppd.lpzContactName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0), MAX_CONTACTNAME);
		else
			strncpy(ppd.lpzContactName, "Message", MAX_CONTACTNAME);
		strcpy(ppd.lpzText, "Debug (CListN+): ");
		strncat(ppd.lpzText, debug, MAX_SECONDLINE - 20);
		ppd.colorText = RGB(0,0,0);
		ppd.colorBack = RGB(255,0,255);
		CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
	}
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
	g_hInst = hInstDLL;
	DisableThreadLibraryCalls(g_hInst);
	return TRUE;
}

__declspec(dllexport) PLUGININFO * MirandaPluginInfo(DWORD mirandaVersion)
{
#if defined(_UNICODE)
	pluginInfo.isTransient |= UNICODE_AWARE;
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 2, 0))
#else
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 1))
#endif        
		return NULL;
	return &pluginInfo;
}

int LoadContactListModule(void);
int LoadCLCModule(void); 
int LoadCLUIModule(); 
int InitGdiPlus();

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{
#if defined(_UNICODE)
	if ( !ServiceExists( MS_DB_CONTACT_GETSETTING_STR )) {
		MessageBox(NULL, TranslateT( "This plugin requires db3x plugin version 0.5.1.0 or later" ), _T("CList Nicer+"), MB_OK );
		return 1;
	}
#endif    
	InitGdiPlus();
	LoadCLUIModule();

	if(ServiceExists(MS_MC_DISABLEHIDDENGROUP))
		CallService(MS_MC_DISABLEHIDDENGROUP, 1, 0);

	return 0;
}

int MenuModulesLoaded(WPARAM wParam, LPARAM lParam);

int __declspec(dllexport) CListInitialise(PLUGINLINK * link)
{
	int rc = 0;
	pluginLink = link;
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif  
	// get the internal malloc/free()
	memset(&memoryManagerInterface, 0, sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &memoryManagerInterface);

	// get the list control interface
	li.cbSize = sizeof(li);
	CallService(MS_SYSTEM_GET_LI, 0, (LPARAM)&li);

	// get the clist interface
	pcli = ( CLIST_INTERFACE* )CallService(MS_CLIST_RETRIEVE_INTERFACE, 0, (LPARAM)g_hInst);
	pcli->pfnBuildGroupPopupMenu = BuildGroupPopupMenu;
	pcli->pfnCluiProtocolStatusChanged = CluiProtocolStatusChanged;
	pcli->pfnCompareContacts = CompareContacts;
	pcli->pfnCreateClcContact = CreateClcContact;
	pcli->pfnGetDefaultFontSetting = GetDefaultFontSetting;
	pcli->pfnGetRowBottomY = RowHeights_GetItemBottomY;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnGetRowTopY = RowHeights_GetItemTopY;
	pcli->pfnGetRowTotalHeight = RowHeights_GetTotalHeight;
	pcli->pfnHitTest = HitTest;
//	pcli->pfnIconFromStatusMode = IconFromStatusMode;
	pcli->pfnPaintClc = PaintClc;
	pcli->pfnRebuildEntireList = RebuildEntireList;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnScrollTo = ScrollTo;

	saveAddContactToGroup = pcli->pfnAddContactToGroup; pcli->pfnAddContactToGroup = AddContactToGroup;
	saveAddGroup = pcli->pfnAddGroup; pcli->pfnAddGroup = AddGroup;
	saveContactListControlWndProc = pcli->pfnContactListControlWndProc; pcli->pfnContactListControlWndProc = ContactListControlWndProc;
	saveContactListWndProc = pcli->pfnContactListWndProc; pcli->pfnContactListWndProc = ContactListWndProc;
	saveLoadClcOptions = pcli->pfnLoadClcOptions; pcli->pfnLoadClcOptions = LoadClcOptions;
	saveProcessExternalMessages = pcli->pfnProcessExternalMessages; pcli->pfnProcessExternalMessages = ProcessExternalMessages;

	rc = LoadContactListModule();
	if (rc == 0)
		rc = LoadCLCModule();
	HookEvent(ME_SYSTEM_MODULESLOADED, systemModulesLoaded);
	HookEvent(ME_SYSTEM_MODULESLOADED, MenuModulesLoaded);
	return rc;
}

// a plugin loader aware of CList exports will never call this.
int __declspec(dllexport) Load(PLUGINLINK * link)
{
	return 1;
}

int __declspec(dllexport) Unload(void)
{
	if (IsWindow(pcli->hwndContactList))
		DestroyWindow(pcli->hwndContactList);
	return 0;
}
