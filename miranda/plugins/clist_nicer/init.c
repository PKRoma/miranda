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
extern CRITICAL_SECTION cs_extcache;

#define DEFAULT_TB_VISIBILITY (1 | 2 | 4 | 8 | 16 | 32 | 64)
TCHAR *szNoevents = _T("No events...");
//extern HICON im_clienthIcons[NR_CLIENTS];
extern HICON overlayicons[10];

extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern BOOL (WINAPI *MyUpdateLayeredWindow)(HWND hwnd, HDC hdcDst, POINT *pptDst,SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags);
extern PGF MyGradientFill;
extern int Docking_ProcessWindowMessage(WPARAM wParam, LPARAM lParam);
extern int SetHideOffline(WPARAM wParam, LPARAM lParam);

extern struct CluiData g_CluiData;
extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry;
int g_maxExtraCacheEntry = 0;
extern pfnDrawAlpha pDrawAlpha;
extern DWORD g_gdiplusToken;
extern HIMAGELIST himlExtraImages;
extern DWORD ( WINAPI *pfnSetLayout )(HDC, DWORD);

struct LIST_INTERFACE li;
struct MM_INTERFACE memoryManagerInterface;

HMENU  BuildGroupPopupMenu( struct ClcGroup* group );
struct ClcContact* CreateClcContact( void );
struct CListEvent* fnCreateEvent( void );
void   ReloadThemedOptions();
void   TrayIconUpdateBase(const char *szChangedProto);
void    RegisterCLUIFrameClasses();

void GetDefaultFontSetting(int i, LOGFONT *lf, COLORREF *colour);
int  GetWindowVisibleState(HWND hWnd, int iStepX, int iStepY);
int  ShowHide(WPARAM wParam, LPARAM lParam);
int  ClcShutdown(WPARAM wParam, LPARAM lParam);

void ( *saveLoadClcOptions )(HWND hwnd,struct ClcData *dat);
void LoadClcOptions(HWND hwnd,struct ClcData *dat);

int ( *saveAddContactToGroup )(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);
int AddContactToGroup(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);

struct ClcGroup* ( *saveRemoveItemFromGroup )(HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount);
struct ClcGroup* RemoveItemFromGroup(HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount);

struct CListEvent* ( *saveAddEvent )(CLISTEVENT *cle);
struct CListEvent* AddEvent(CLISTEVENT *cle);

int ( *saveAddInfoItemToGroup )(struct ClcGroup *group, int flags, const TCHAR *pszText);
int AddInfoItemToGroup(struct ClcGroup *group, int flags, const TCHAR *pszText);

struct ClcGroup* ( *saveAddGroup )(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);
struct ClcGroup* AddGroup(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);

LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int ( *saveIconFromStatusMode )( const char *szProto, int status, HANDLE hContact );

LRESULT ( *saveProcessExternalMessages )(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ProcessExternalMessages(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);

int ( *saveRemoveEvent )(HANDLE hContact, HANDLE hDbEvent);
int RemoveEvent(HANDLE hContact, HANDLE hDbEvent);

int ( *saveTrayIconProcessMessage )(WPARAM wParam, LPARAM lParam);
int TrayIconProcessMessage(WPARAM wParam, LPARAM lParam);

void ( *saveRecalcScrollBar )(HWND hwnd, struct ClcData *dat);
void RecalcScrollBar(HWND hwnd, struct ClcData *dat);

PLUGININFOEX pluginInfo = {
#if defined(_UNICODE)
		sizeof(PLUGININFOEX), "CList Nicer+ (Unicode)", PLUGIN_MAKE_VERSION(0, 7, 1, 1),
#else
		sizeof(PLUGININFOEX), "CList Nicer+", PLUGIN_MAKE_VERSION(0, 7, 1, 1),
#endif
		"Display contacts, event notifications, protocol status",
		"Pixel, egoDust, cyreve, Nightwish", "", "Copyright 2000-2006 Miranda-IM project", "http://www.miranda-im.org",
		UNICODE_AWARE,
		DEFMOD_CLISTALL,
#if defined(_UNICODE)
		{0x8f79b4ee, 0xeb48, 0x4a03, { 0x87, 0x3e, 0x27, 0xbe, 0x6b, 0x7e, 0x9a, 0x25 }} //{8F79B4EE-EB48-4a03-873E-27BE6B7E9A25}
#else
		{0x5a070cec, 0xb2ab, 0x4bbe, { 0x8e, 0x48, 0x9c, 0x8d, 0xcd, 0xda, 0x14, 0xc3 }} //{5A070CEC-B2AB-4bbe-8E48-9C8DCDDA14C3}
#endif
};

#if defined(_UNICODE)
void __forceinline _DebugTraceW(const wchar_t *fmt, ...)
{
#ifdef _DEBUG
    wchar_t debug[2048];
    int     ibsize = 2047;
    va_list va;
    va_start(va, fmt);

	lstrcpyW(debug, L"CLN: ");

    _vsnwprintf(&debug[5], ibsize - 10, fmt, va);
    OutputDebugStringW(debug);
#endif    
}
#endif

void __forceinline _DebugTraceA(const char *fmt, ...)
{
    char    debug[2048];
    int     ibsize = 2047;
    va_list va;
    va_start(va, fmt);

	lstrcpyA(debug, "CLN: ");
	_vsnprintf(&debug[5], ibsize - 10, fmt, va);
#ifdef _DEBUG
    OutputDebugStringA(debug);
#else
    {
        char szLogFileName[MAX_PATH], szDataPath[MAX_PATH];
        FILE *f;

        CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)szDataPath);
        mir_snprintf(szLogFileName, MAX_PATH, "%s\\%s", szDataPath, "clist_nicer.log");
        f = fopen(szLogFileName, "a+");
        if(f) {
            fputs(debug, f);
            fputs("\n", f);
            fclose(f);
        }
    }
#endif
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
	g_hInst = hInstDLL;
	DisableThreadLibraryCalls(g_hInst);
	return TRUE;
}

__declspec(dllexport) PLUGININFOEX * MirandaPluginInfoEx(DWORD mirandaVersion)
{
#if defined(_UNICODE)
	pluginInfo.flags |= UNICODE_AWARE;
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 2, 0))
#else
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 1))
#endif
		return NULL;
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_CLIST, MIID_LAST};
__declspec(dllexport) const MUUID * MirandaPluginInterfaces(void)
{
	return interfaces;
}

int  LoadContactListModule(void);
int  LoadCLCModule(void);
void LoadCLUIModule( void );
int  InitGdiPlus();

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{
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

	//ZeroMemory((void *)im_clienthIcons, sizeof(HICON) * NR_CLIENTS);
	ZeroMemory((void *)overlayicons, sizeof(HICON) * 10);

	CLN_LoadAllIcons(1);
	LoadExtBkSettingsFromDB();
	return 0;
}

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

	pfnSetLayout = (DWORD ( WINAPI *)(HDC, DWORD))GetProcAddress( GetModuleHandleA( "GDI32.DLL" ), "SetLayout" );

	InitGdiPlus();
	LoadCLCButtonModule();
	RegisterCLUIFrameClasses();
	hUserDll = GetModuleHandleA("user32.dll");
	if (hUserDll) {
		MySetLayeredWindowAttributes = (BOOL(WINAPI *)(HWND, COLORREF, BYTE, DWORD))GetProcAddress(hUserDll, "SetLayeredWindowAttributes");
		MyUpdateLayeredWindow = (BOOL (WINAPI *)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD))GetProcAddress(hUserDll, "UpdateLayeredWindow");
	}
	MyGradientFill = (PGF)GetProcAddress(GetModuleHandleA("msimg32"), "GradientFill");

	// get the internal malloc/free()
	memset(&memoryManagerInterface, 0, sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &memoryManagerInterface);

	// get the list control interface
	li.cbSize = sizeof(li);
	CallService(MS_SYSTEM_GET_LI, 0, (LPARAM)&li);

	ZeroMemory((void*) &g_CluiData, sizeof(g_CluiData));
	{
		int iCount = CallService(MS_DB_CONTACT_GETCOUNT, 0, 0);
		
		iCount += 20;
        if(iCount < 300)
            iCount = 300;

		g_ExtraCache = malloc(sizeof(struct ExtraCache) * iCount);
		ZeroMemory(g_ExtraCache, sizeof(struct ExtraCache) * iCount);
		g_nextExtraCacheEntry = 0;
		g_maxExtraCacheEntry = iCount;
		InitializeCriticalSection(&cs_extcache);
	}

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
	g_CluiData.langPackCP = CallService(MS_LANGPACK_GETCODEPAGE, 0, 0);
	{
		DWORD sortOrder = DBGetContactSettingDword(NULL, "CList", "SortOrder", SORTBY_NAME);
		g_CluiData.sortOrder[0] = LOBYTE(LOWORD(sortOrder));
		g_CluiData.sortOrder[1] = HIBYTE(LOWORD(sortOrder));
		g_CluiData.sortOrder[2] = LOBYTE(HIWORD(sortOrder));
	}
	if(g_CluiData.bFirstRun)
		DBWriteContactSettingByte(NULL, "CLUI", "firstrun", 0);

	_tzset();
	{
		time_t now = time(NULL);
		struct tm gmt = *gmtime(&now);
		time_t gmt_time = mktime(&gmt);
		g_CluiData.local_gmt_diff = (int)difftime(now, gmt_time);

	}
	ReloadThemedOptions();
	FLT_ReadOptions();
	Reload3dBevelColors();
	himlExtraImages = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16), 30, 2);
	ImageList_SetIconSize(himlExtraImages, g_CluiData.exIconScale, g_CluiData.exIconScale);

	g_CluiData.dwFlags = DBGetContactSettingDword(NULL, "CLUI", "Frameflags", CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_STATUSICONS | 
                                                  CLUI_FRAME_SHOWBOTTOMBUTTONS | CLUI_FRAME_BUTTONSFLAT | CLUI_FRAME_CLISTSUNKEN);
	g_CluiData.dwFlags |= (DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? CLUI_FRAME_SBARSHOW : 0);
	g_CluiData.soundsOff = DBGetContactSettingByte(NULL, "CLUI", "NoSounds", 0);

	pDrawAlpha = NULL;
	if(!pDrawAlpha)
		pDrawAlpha = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS && g_gdiplusToken) ? (pfnDrawAlpha)GDIp_DrawAlpha : (pfnDrawAlpha)DrawAlpha;

	if(DBGetContactSettingByte(NULL, "Skin", "UseSound", 0) != g_CluiData.soundsOff)
		DBWriteContactSettingByte(NULL, "Skin", "UseSound", (BYTE)(g_CluiData.soundsOff ? 0 : 1));

	// get the clist interface
	pcli = ( CLIST_INTERFACE* )CallService(MS_CLIST_RETRIEVE_INTERFACE, 0, (LPARAM)g_hInst);
	if ( (int)pcli == CALLSERVICE_NOTFOUND ) {
LBL_Error:
		MessageBoxA( NULL, "This plugin requires Miranda IM 0.7.0.8 or later", "Fatal error", MB_OK );
		return 1;
	}
	if ( pcli->version < 4 ) // don't join it with the previous if()
		goto LBL_Error;

	pcli->pfnBuildGroupPopupMenu = BuildGroupPopupMenu;
	pcli->pfnCluiProtocolStatusChanged = CluiProtocolStatusChanged;
	pcli->pfnCompareContacts = CompareContacts;
	pcli->pfnCreateClcContact = CreateClcContact;
	pcli->pfnCreateEvent = fnCreateEvent;
	pcli->pfnDocking_ProcessWindowMessage = Docking_ProcessWindowMessage;
	pcli->pfnGetDefaultFontSetting = GetDefaultFontSetting;
	pcli->pfnGetRowBottomY = RowHeights_GetItemBottomY;
	pcli->pfnGetRowHeight = RowHeights_GetHeight;
	pcli->pfnGetRowTopY = RowHeights_GetItemTopY;
	pcli->pfnGetRowTotalHeight = RowHeights_GetTotalHeight;
	pcli->pfnGetWindowVisibleState = GetWindowVisibleState;
	pcli->pfnHitTest = HitTest;
	pcli->pfnLoadContactTree = LoadContactTree;
	pcli->pfnOnCreateClc = LoadCLUIModule;
	pcli->pfnPaintClc = PaintClc;
	pcli->pfnRebuildEntireList = RebuildEntireList;
	pcli->pfnRowHitTest = RowHeights_HitTest;
	pcli->pfnScrollTo = ScrollTo;
	pcli->pfnTrayIconUpdateBase = TrayIconUpdateBase;
	pcli->pfnSetHideOffline = SetHideOffline;
	pcli->pfnShowHide = ShowHide;

	saveAddContactToGroup = pcli->pfnAddContactToGroup; pcli->pfnAddContactToGroup = AddContactToGroup;
	saveRemoveItemFromGroup = pcli->pfnRemoveItemFromGroup; pcli->pfnRemoveItemFromGroup = RemoveItemFromGroup;

	saveAddEvent = pcli->pfnAddEvent; pcli->pfnAddEvent = AddEvent;
	saveRemoveEvent = pcli->pfnRemoveEvent; pcli->pfnRemoveEvent = RemoveEvent;

	saveAddGroup = pcli->pfnAddGroup; pcli->pfnAddGroup = AddGroup;
	saveAddInfoItemToGroup = pcli->pfnAddInfoItemToGroup; pcli->pfnAddInfoItemToGroup = AddInfoItemToGroup;
	saveContactListControlWndProc = pcli->pfnContactListControlWndProc; pcli->pfnContactListControlWndProc = ContactListControlWndProc;
	saveContactListWndProc = pcli->pfnContactListWndProc; pcli->pfnContactListWndProc = ContactListWndProc;
	saveIconFromStatusMode = pcli->pfnIconFromStatusMode; pcli->pfnIconFromStatusMode = fnIconFromStatusMode;
	saveLoadClcOptions = pcli->pfnLoadClcOptions; pcli->pfnLoadClcOptions = LoadClcOptions;
	saveProcessExternalMessages = pcli->pfnProcessExternalMessages; pcli->pfnProcessExternalMessages = ProcessExternalMessages;
	saveRecalcScrollBar = pcli->pfnRecalcScrollBar; pcli->pfnRecalcScrollBar = RecalcScrollBar;
	saveTrayIconProcessMessage = pcli->pfnTrayIconProcessMessage; pcli->pfnTrayIconProcessMessage = TrayIconProcessMessage;

	rc = LoadContactListModule();
	if (rc == 0)
		rc = LoadCLCModule();
	HookEvent(ME_SYSTEM_MODULESLOADED, systemModulesLoaded);
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
	ImageList_Destroy(himlExtraImages);
	ClcShutdown(0, 0);
	UnLoadCLUIFramesModule();
	return 0;
}

