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
#include "cluiframes/cluiframes.h"

HINSTANCE g_hInst = 0;
PLUGINLINK *pluginLink;
CLIST_INTERFACE* pcli = NULL;

#define DEFAULT_TB_VISIBILITY (1 | 2 | 4 | 8 | 16 | 32)
TCHAR *szNoevents = _T("No events...");
extern HICON im_clienthIcons[NR_CLIENTS];
extern HICON overlayicons[10];

extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern BOOL (WINAPI *MyUpdateLayeredWindow)(HWND hwnd, HDC hdcDst, POINT *pptDst,SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags);
extern struct CluiData g_CluiData;
extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry;
extern int g_maxExtraCacheEntry;
extern pfnDrawAlpha pDrawAlpha;
extern DWORD g_gdiplusToken;
extern HANDLE hSoundHook, hIcoLibChanged;
extern HIMAGELIST himlExtraImages;
extern DWORD ( WINAPI *pfnSetLayout )(HDC, DWORD);

struct LIST_INTERFACE li;
struct MM_INTERFACE memoryManagerInterface;

HMENU  BuildGroupPopupMenu( struct ClcGroup* group );
int    CListTrayNotify( MIRANDASYSTRAYNOTIFY *msn );
struct ClcContact* CreateClcContact( void );
void   ReloadThemedOptions();
void   TrayIconIconsChanged(void);
void   TrayIconSetToBase(char *szPreferredProto);
void   TrayIconUpdateBase(const char *szChangedProto);
void   TrayIconUpdateWithImageList(int iImage, const TCHAR *szNewTip, char *szPreferredProto);

int fnRemoveEvent(WPARAM wParam, LPARAM lParam);
int fnAddEvent(WPARAM wParam, LPARAM lParam);
int fnGetEvent(WPARAM wParam, LPARAM lParam);

void GetDefaultFontSetting(int i, LOGFONT *lf, COLORREF *colour);

void ( *saveLoadClcOptions )(HWND hwnd,struct ClcData *dat);
void LoadClcOptions(HWND hwnd,struct ClcData *dat);

int ( *saveAddContactToGroup )(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);
int AddContactToGroup(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);

int ( *saveAddInfoItemToGroup )(struct ClcGroup *group, int flags, const TCHAR *pszText);
int AddInfoItemToGroup(struct ClcGroup *group, int flags, const TCHAR *pszText);

struct ClcGroup* ( *saveAddGroup )(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);
struct ClcGroup* AddGroup(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);

int ( *saveIconFromStatusMode )( const char *szProto, int status, HANDLE hContact );

LRESULT ( *saveProcessExternalMessages )(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ProcessExternalMessages(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int ( *saveTrayIconProcessMessage )(WPARAM wParam, LPARAM lParam);
int TrayIconProcessMessage(WPARAM wParam, LPARAM lParam);

void ( *saveRecalcScrollBar )(HWND hwnd, struct ClcData *dat);
void RecalcScrollBar(HWND hwnd, struct ClcData *dat);

PLUGININFO pluginInfo = {
#if defined(_UNICODE)
	sizeof(PLUGININFO), "CList Nicer+ (Unicode)", PLUGIN_MAKE_VERSION(0, 5, 1, 16),
#else
	sizeof(PLUGININFO), "CList Nicer+", PLUGIN_MAKE_VERSION(0, 5, 1, 16),
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

int  LoadContactListModule(void);
int  LoadCLCModule(void);
void LoadCLUIModule( void );
int  InitGdiPlus();

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{
#if defined(_UNICODE)
	if ( !ServiceExists( MS_DB_CONTACT_GETSETTING_STR )) {
		MessageBox(NULL, TranslateT( "This plugin requires miranda database plugin version 0.5.1.0 or later" ), _T("CList Nicer+"), MB_OK );
		return 1;
	}
#endif

	g_CluiData.bMetaAvail = ServiceExists(MS_MC_GETDEFAULTCONTACT) ? TRUE : FALSE;
	if(g_CluiData.bMetaAvail)
		mir_snprintf(g_CluiData.szMetaName, 256, "%s", (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0));
	else
		strncpy(g_CluiData.szMetaName, "MetaContacts", 255);

	if(ServiceExists(MS_MC_DISABLEHIDDENGROUP))
		CallService(MS_MC_DISABLEHIDDENGROUP, 1, 0);

	g_CluiData.bAvatarServiceAvail = ServiceExists(MS_AV_GETAVATARBITMAP) ? TRUE : FALSE;
	if(g_CluiData.bAvatarServiceAvail)
		HookEvent(ME_AV_AVATARCHANGED, AvatarChanged);
	g_CluiData.tabSRMM_Avail = ServiceExists("SRMsg_MOD/GetWindowFlags") ? TRUE : FALSE;
	g_CluiData.IcoLib_Avail = ServiceExists(MS_SKIN2_ADDICON) ? TRUE : FALSE;

	ZeroMemory((void *)im_clienthIcons, sizeof(HICON) * NR_CLIENTS);
	ZeroMemory((void *)overlayicons, sizeof(HICON) * 10);

	CLN_LoadAllIcons(1);
	return 0;
}

int MenuModulesLoaded(WPARAM wParam, LPARAM lParam);

static int fnIconFromStatusMode( const char* szProto, int status, HANDLE hContact )
{	return IconFromStatusMode( szProto, status, hContact, NULL );
}

int __declspec(dllexport) CListInitialise(PLUGINLINK * link)
{
	int rc = 0;
	HMODULE hUserDll;

	pluginLink = link;
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	pfnSetLayout = GetProcAddress( GetModuleHandleA( "GDI32.DLL" ), "SetLayout" );

	LoadExtBkSettingsFromDB();
	InitGdiPlus();

	hUserDll = GetModuleHandleA("user32.dll");
	if (hUserDll) {
		MySetLayeredWindowAttributes = (BOOL(WINAPI *)(HWND, COLORREF, BYTE, DWORD))GetProcAddress(hUserDll, "SetLayeredWindowAttributes");
		MyUpdateLayeredWindow = (BOOL (WINAPI *)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD))GetProcAddress(hUserDll, "UpdateLayeredWindow");
	}

	// get the internal malloc/free()
	memset(&memoryManagerInterface, 0, sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &memoryManagerInterface);

	// get the list control interface
	li.cbSize = sizeof(li);
	CallService(MS_SYSTEM_GET_LI, 0, (LPARAM)&li);

	ZeroMemory((void*) &g_CluiData, sizeof(g_CluiData));
	g_ExtraCache = malloc(sizeof(struct ExtraCache) * EXTRAIMAGECACHESIZE);
	ZeroMemory(g_ExtraCache, sizeof(struct ExtraCache) * EXTRAIMAGECACHESIZE);

	g_nextExtraCacheEntry = 0;
	g_maxExtraCacheEntry = EXTRAIMAGECACHESIZE;

	g_CluiData.bMetaEnabled = DBGetContactSettingByte(NULL, "MetaContacts", "Enabled", 1);
	g_CluiData.toolbarVisibility = DBGetContactSettingDword(NULL, "CLUI", "TBVisibility", DEFAULT_TB_VISIBILITY);
	g_CluiData.hMenuButtons = GetSubMenu(LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT)), 3);
	g_CluiData.hMenuNotify = CreatePopupMenu();
	g_CluiData.wNextMenuID = 1;
	g_CluiData.sortTimer = DBGetContactSettingDword(NULL, "CLC", "SortTimer", 150);
	g_CluiData.szNoEvents = TranslateTS(szNoevents);
	g_CluiData.avatarBorder = (COLORREF)DBGetContactSettingDword(NULL, "CLC", "avatarborder", 0);
	g_CluiData.avatarRadius = (COLORREF)DBGetContactSettingDword(NULL, "CLC", "avatarradius", 4);
	g_CluiData.hBrushAvatarBorder = CreateSolidBrush(g_CluiData.avatarBorder);
	g_CluiData.avatarSize = DBGetContactSettingWord(NULL,"CList", "AvatarSize", 24);
	g_CluiData.dualRowMode = DBGetContactSettingByte(NULL, "CLC", "DualRowMode", 0);
	g_CluiData.avatarPadding = DBGetContactSettingByte(NULL, "CList", "AvatarPadding", 0);
	g_CluiData.isTransparent = DBGetContactSettingByte(NULL, "CList", "Transparent", 0);
	g_CluiData.alpha = DBGetContactSettingByte(NULL, "CList", "Alpha", SETTING_ALPHA_DEFAULT);
	g_CluiData.autoalpha = DBGetContactSettingByte(NULL, "CList", "AutoAlpha", SETTING_ALPHA_DEFAULT);
	g_CluiData.fadeinout = DBGetContactSettingByte(NULL, "CLUI", "FadeInOut", 0);
	g_CluiData.autosize = DBGetContactSettingByte(NULL, "CLUI", "AutoSize", 0);
	g_CluiData.titleBarHeight = DEFAULT_TITLEBAR_HEIGHT;
	g_CluiData.dwExtraImageMask = DBGetContactSettingDword(NULL, "CLUI", "ximgmask", 0);
	g_CluiData.bNoOfflineAvatars = DBGetContactSettingByte(NULL, "CList", "NoOfflineAV", 1);
	g_CluiData.bFullTransparent = DBGetContactSettingByte(NULL, "CLUI", "fulltransparent", 0);
	g_CluiData.bDblClkAvatars = DBGetContactSettingByte(NULL, "CLC", "dblclkav", 0);
	g_CluiData.bEqualSections = DBGetContactSettingByte(NULL, "CLUI", "EqualSections", 0);
	g_CluiData.bCenterStatusIcons = DBGetContactSettingByte(NULL, "CLC", "si_centered", 1);
	g_CluiData.boldHideOffline = (BYTE)-1;
	g_CluiData.bSecIMAvail = ServiceExists("SecureIM/IsContactSecured") ? 1 : 0;
	g_CluiData.bNoTrayTips = DBGetContactSettingByte(NULL, "CList", "NoTrayTips", 0);
	g_CluiData.bShowLocalTime = DBGetContactSettingByte(NULL, "CLC", "ShowLocalTime", 1);
	g_CluiData.bShowLocalTimeSelective = DBGetContactSettingByte(NULL, "CLC", "SelectiveLocalTime", 1);
	g_CluiData.bDontSeparateOffline = DBGetContactSettingByte(NULL, "CList", "DontSeparateOffline", 0);
	g_CluiData.bShowXStatusOnSbar = DBGetContactSettingByte(NULL, "CLUI", "xstatus_sbar", 0);
	g_CluiData.bLayeredHack = DBGetContactSettingByte(NULL, "CLUI", "layeredhack", 1);
	g_CluiData.bFirstRun = DBGetContactSettingByte(NULL, "CLUI", "firstrun", 1);
	if(g_CluiData.bFirstRun)
		DBWriteContactSettingByte(NULL, "CLUI", "firstrun", 0);

	_tzset();
	{
		DWORD now = time(NULL);
		struct tm gmt = *gmtime(&now);
		DWORD gmt_time = mktime(&gmt);
		g_CluiData.local_gmt_diff = (int)difftime(now, gmt_time);

	}
	ReloadThemedOptions();
	Reload3dBevelColors();
	himlExtraImages = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16), 30, 2);
	ImageList_SetIconSize(himlExtraImages, g_CluiData.exIconScale, g_CluiData.exIconScale);

	g_CluiData.dwFlags = DBGetContactSettingDword(NULL, "CLUI", "Frameflags", CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_USEEVENTAREA | CLUI_FRAME_STATUSICONS | CLUI_FRAME_SHOWBOTTOMBUTTONS);
	g_CluiData.dwFlags |= (DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? CLUI_FRAME_SBARSHOW : 0);
	g_CluiData.soundsOff = DBGetContactSettingByte(NULL, "CLUI", "NoSounds", 0);

	pDrawAlpha = NULL;
	if(!pDrawAlpha)
		pDrawAlpha = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS && g_gdiplusToken) ? (pfnDrawAlpha)GDIp_DrawAlpha : (pfnDrawAlpha)DrawAlpha;

	if(DBGetContactSettingByte(NULL, "Skin", "UseSound", 0) != g_CluiData.soundsOff)
		DBWriteContactSettingByte(NULL, "Skin", "UseSound", g_CluiData.soundsOff ? 0 : 1);

	// get the clist interface
	pcli = ( CLIST_INTERFACE* )CallService(MS_CLIST_RETRIEVE_INTERFACE, 0, (LPARAM)g_hInst);
	if ( (int)pcli == CALLSERVICE_NOTFOUND ) {
		MessageBoxA( NULL, "This version of plugin requires Miranda 0.4.3 bld#42 or later", "Fatal error", MB_OK );
		return 1;
	}

	pcli->pfnBuildGroupPopupMenu = BuildGroupPopupMenu;
	pcli->pfnCListTrayNotify = CListTrayNotify;
	pcli->pfnCluiProtocolStatusChanged = CluiProtocolStatusChanged;
	pcli->pfnCompareContacts = CompareContacts;
	pcli->pfnCreateClcContact = CreateClcContact;
	pcli->pfnGetDefaultFontSetting = GetDefaultFontSetting;
	pcli->pfnGetRowBottomY = RowHeights_GetItemBottomY;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnGetRowTopY = RowHeights_GetItemTopY;
	pcli->pfnGetRowTotalHeight = RowHeights_GetTotalHeight;
	pcli->pfnHitTest = HitTest;
	pcli->pfnOnCreateClc = LoadCLUIModule;
	pcli->pfnPaintClc = PaintClc;
	pcli->pfnRebuildEntireList = RebuildEntireList;
	pcli->pfnRowHitTest = RowHeights_HitTest;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnScrollTo = ScrollTo;
	pcli->pfnTrayIconIconsChanged = TrayIconIconsChanged;
	pcli->pfnTrayIconSetToBase = TrayIconSetToBase;
	pcli->pfnTrayIconUpdateBase = TrayIconUpdateBase;
	pcli->pfnTrayIconUpdateWithImageList = TrayIconUpdateWithImageList;

	saveAddContactToGroup = pcli->pfnAddContactToGroup; pcli->pfnAddContactToGroup = AddContactToGroup;
	saveAddGroup = pcli->pfnAddGroup; pcli->pfnAddGroup = AddGroup;
	saveAddInfoItemToGroup = pcli->pfnAddInfoItemToGroup; pcli->pfnAddInfoItemToGroup = AddInfoItemToGroup;
	saveContactListControlWndProc = pcli->pfnContactListControlWndProc; pcli->pfnContactListControlWndProc = ContactListControlWndProc;
	saveContactListWndProc = pcli->pfnContactListWndProc; pcli->pfnContactListWndProc = ContactListWndProc;
	saveIconFromStatusMode = pcli->pfnIconFromStatusMode; pcli->pfnIconFromStatusMode = fnIconFromStatusMode;
	saveLoadClcOptions = pcli->pfnLoadClcOptions; pcli->pfnLoadClcOptions = LoadClcOptions;
	saveProcessExternalMessages = pcli->pfnProcessExternalMessages; pcli->pfnProcessExternalMessages = ProcessExternalMessages;
	saveRecalcScrollBar = pcli->pfnRecalcScrollBar; pcli->pfnRecalcScrollBar = RecalcScrollBar;
	saveTrayIconProcessMessage = pcli->pfnTrayIconProcessMessage; pcli->pfnTrayIconProcessMessage = TrayIconProcessMessage;

	if(ServiceExists(MS_CLIST_ADDEVENT)) {
		DestroyServiceFunction(MS_CLIST_ADDEVENT);
		DestroyServiceFunction(MS_CLIST_REMOVEEVENT);
		DestroyServiceFunction(MS_CLIST_GETEVENT);
	}

	CreateServiceFunction(MS_CLIST_ADDEVENT, fnAddEvent);
	CreateServiceFunction(MS_CLIST_REMOVEEVENT, fnRemoveEvent);
	CreateServiceFunction(MS_CLIST_GETEVENT, fnGetEvent);

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
