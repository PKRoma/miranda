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

UNICODE done

*/
#include "commonheaders.h"
#include <m_findadd.h>
#include <m_icq.h>
#include "m_updater.h"
#include "cluiframes/cluiframes.h"
#include "coolsb/coolscroll.h"

#define TM_AUTOALPHA  1
#define TIMERID_AUTOSIZE 100
#define MENU_MIRANDAMENU         0xFFFF1234

int g_shutDown = 0;
int g_fading_active = 0;

static RECT g_PreSizeRect, g_SizingRect;
static int g_sizingmethod;
static LONG g_CLUI_x_off, g_CLUI_y_off, g_CLUI_y1_off, g_CLUI_x1_off;
static RECT rcWPC;

static HMODULE hUserDll;
static int transparentFocus = 1;
static byte oldhideoffline;
static int disableautoupd=1;
HANDLE hFrameContactTree;
extern HIMAGELIST hCListImages;
extern PLUGININFOEX pluginInfo;
extern WNDPROC OldStatusBarProc;
extern RECT old_window_rect, new_window_rect;
extern pfnDrawAlpha pDrawAlpha;

extern BOOL g_trayTooltipActive;
extern POINT tray_hover_pos;
extern HWND g_hwndViewModeFrame, g_hwndEventArea;

struct ClcData *g_clcData = NULL;
struct ExtraCache *g_ExtraCache = NULL;
int g_nextExtraCacheEntry = 0;

extern ImageItem *g_CLUIImageItem;
extern HBRUSH g_CLUISkinnedBkColor;
extern StatusItems_t *StatusItems;
extern HWND g_hwndSFL;
extern ButtonItem *g_ButtonItems;
extern COLORREF g_CLUISkinnedBkColorRGB;
extern wndFrame *wndFrameCLC;

HIMAGELIST himlExtraImages = 0;

static BYTE old_cliststate, show_on_first_autosize = FALSE;

RECT cluiPos;
DWORD g_gdiplusToken = 0;

#if defined(_UNICODE)
TCHAR statusNames[12][124];
#else
TCHAR *statusNames[12];
#endif

BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD) = 0;
BOOL (WINAPI *MyUpdateLayeredWindow)(HWND hwnd, HDC hdcDst, POINT *pptDst,SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags) = 0;

extern LRESULT CALLBACK EventAreaWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HANDLE hNotifyFrame;

int SortList(WPARAM wParam, LPARAM lParam);
int LoadCluiServices(void);
void InitGroupMenus();
void ReloadExtraIcons();
void FS_RegisterFonts();
void LoadExtraIconModule();
int MTG_OnmodulesLoad(WPARAM wParam,LPARAM lParam);
extern void RemoveFromTaskBar(HWND hWnd);

extern LONG g_cxsmIcon, g_cysmIcon;
struct CluiData g_CluiData;
extern HANDLE hSoundHook;
extern HANDLE hIcoLibChanged;
extern HANDLE hExtraImageListRebuilding, hExtraImageApplying;

SIZE g_oldSize = {0};
POINT g_oldPos = {0};
int during_sizing = 0;
extern int dock_prevent_moving;

static HDC hdcLockedPoint = 0;
static HBITMAP hbmLockedPoint = 0, hbmOldLockedPoint = 0;

HICON overlayicons[10];

struct CluiTopButton top_buttons[] = {
	    0, 0, 0, IDC_TBTOPMENU, IDI_TBTOPMENU, 0,           "CLN_topmenu", NULL, TOPBUTTON_PUSH | TOPBUTTON_SENDONDOWN, 1, LPGENT("Show menu"), 
		0, 0, 0, IDC_TBHIDEOFFLINE, IDI_HIDEOFFLINE, 0,     "CLN_online", NULL, 0, 2, LPGENT("Show / hide offline contacts"),
		0, 0, 0, IDC_TBHIDEGROUPS, IDI_HIDEGROUPS, 0,       "CLN_groups", NULL, 0, 4, LPGENT("Toggle group mode"),
		0, 0, 0, IDC_TBFINDANDADD, IDI_FINDANDADD, 0,       "CLN_findadd", NULL, TOPBUTTON_PUSH, 8, LPGENT("Find and add contacts"),
		0, 0, 0, IDC_TBOPTIONS, IDI_TBOPTIONS, 0,           "CLN_options", NULL, TOPBUTTON_PUSH, 16, LPGENT("Open preferences"),
		0, 0, 0, IDC_TBSOUND, IDI_SOUNDSON, IDI_SOUNDSOFF,  "CLN_sound", "CLN_soundsoff", 0, 32, LPGENT("Toggle sounds"),
		0, 0, 0, IDC_TBMINIMIZE, IDI_MINIMIZE, 0,           "CLN_minimize", NULL, TOPBUTTON_PUSH, 64, LPGENT("Minimize contact list"),
		0, 0, 0, IDC_TBTOPSTATUS, 0, 0,                     "CLN_topstatus", NULL, TOPBUTTON_PUSH  | TOPBUTTON_SENDONDOWN, 128, LPGENT("Status menu"),
		0, 0, 0, IDC_TABSRMMSLIST, IDI_TABSRMMSESSIONLIST, 0, "CLN_slist", NULL, TOPBUTTON_PUSH | TOPBUTTON_SENDONDOWN, 256, LPGENT("tabSRMM session list"),
		0, 0, 0, IDC_TABSRMMMENU, IDI_TABSRMMMENU, 0,       "CLN_menu", NULL, TOPBUTTON_PUSH | TOPBUTTON_SENDONDOWN, 512, LPGENT("tabSRMM Menu"),
		
		0, 0, 0, IDC_TBSELECTVIEWMODE, IDI_CLVM_SELECT, 0,  "CLN_CLVM_select", NULL, TOPBUTTON_PUSH | TOPBUTTON_SENDONDOWN, 1024, LPGENT("Select view mode"),
		0, 0, 0, IDC_TBCONFIGUREVIEWMODE, IDI_CLVM_OPTIONS, 0, "CLN_CLVM_options", NULL, TOPBUTTON_PUSH, 2048, LPGENT("Setup view modes"),
		0, 0, 0, IDC_TBCLEARVIEWMODE, IDI_DELETE, 0,        "CLN_CLVM_reset", NULL, TOPBUTTON_PUSH, 4096, LPGENT("Clear view mode"),

		0, 0, 0, IDC_TBGLOBALSTATUS, 0, 0, "", NULL, TOPBUTTON_PUSH | TOPBUTTON_SENDONDOWN, 0, LPGENT("Set status modes"),
		0, 0, 0, IDC_TBMENU, IDI_MINIMIZE, 0, "", NULL, TOPBUTTON_PUSH | TOPBUTTON_SENDONDOWN, 0, LPGENT("Open main menu"),
		(HWND) - 1, 0, 0, 0, 0, 0, 0, 0, 0
};

static struct IconDesc myIcons[] = {
	"CLN_online", LPGEN("Toggle show online/offline"), -IDI_HIDEOFFLINE,
	"CLN_groups", LPGEN("Toggle groups"), -IDI_HIDEGROUPS,
	"CLN_findadd", LPGEN("Find contacts"), -IDI_FINDANDADD,
	"CLN_options", LPGEN("Open preferences"), -IDI_TBOPTIONS,
	"CLN_sound", LPGEN("Toggle sounds"), -IDI_SOUNDSON,
	"CLN_minimize", LPGEN("Minimize contact list"), -IDI_MINIMIZE,
	"CLN_slist", LPGEN("Show tabSRMM session list"), -IDI_TABSRMMSESSIONLIST,
	"CLN_menu", LPGEN("Show tabSRMM menu"), -IDI_TABSRMMMENU,
	"CLN_soundsoff", LPGEN("Sounds are off"), -IDI_SOUNDSOFF,
	"CLN_CLVM_select", LPGEN("Select view mode"), -IDI_CLVM_SELECT,
	"CLN_CLVM_reset", LPGEN("Reset view mode"), -IDI_DELETE,
	"CLN_CLVM_options", LPGEN("Configure view modes"), -IDI_CLVM_OPTIONS,
	"CLN_topmenu", LPGEN("Show menu"), -IDI_TBTOPMENU,
	NULL, NULL, 0
};

static void Tweak_It(COLORREF clr)
{
	SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);
	MySetLayeredWindowAttributes(pcli->hwndContactList, clr, 0, LWA_COLORKEY);
	g_CluiData.colorkey = clr;
}

static void LayoutButtons(HWND hwnd, RECT *rc)
{
	int i;
	RECT rect;
	BYTE rightButton = 1, leftButton = 0;
	BYTE left_offset = g_CluiData.bCLeft - (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN ? 3 : 0);
	BYTE right_offset = g_CluiData.bCRight - (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN ? 3 : 0);
	BYTE delta = left_offset + right_offset;
    ButtonItem *btnItems = g_ButtonItems;

	if(rc == NULL)
		GetClientRect(hwnd, &rect);
	else
		rect = *rc;

	rect.bottom -= g_CluiData.bCBottom;

    if(g_ButtonItems) {
        while(btnItems) {
            LONG x = (btnItems->xOff >= 0) ? rect.left + btnItems->xOff : rect.right - abs(btnItems->xOff);
            LONG y = (btnItems->yOff >= 0) ? rect.top + btnItems->yOff : rect.bottom - g_CluiData.statusBarHeight;

            SetWindowPos(btnItems->hWnd, 0, x, y, btnItems->width, btnItems->height,
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
            btnItems = btnItems->nextItem;
        }
        SetWindowPos(top_buttons[14].hwnd, 0, 2 + left_offset, rect.bottom - g_CluiData.statusBarHeight - BUTTON_HEIGHT_D - 1,
                              BUTTON_WIDTH_D * 3, BUTTON_HEIGHT_D + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
        SetWindowPos(top_buttons[13].hwnd, 0, left_offset + (3 * BUTTON_WIDTH_D) + 3, rect.bottom - g_CluiData.statusBarHeight - BUTTON_HEIGHT_D - 1,
                              rect.right - delta - (3 * BUTTON_WIDTH_D + 5), BUTTON_HEIGHT_D + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
        return;
    }

    for (i = 0; ; i++) {
		if (top_buttons[i].szTooltip == NULL)
			break;
		if (top_buttons[i].hwnd == 0)
			continue;
		if (top_buttons[i].id == IDC_TBMENU) {
			SetWindowPos(top_buttons[i].hwnd, 0, 2 + left_offset, rect.bottom - g_CluiData.statusBarHeight - BUTTON_HEIGHT_D - 1,
                                  BUTTON_WIDTH_D * 3, BUTTON_HEIGHT_D + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);

		}
		else if (top_buttons[i].id == IDC_TBGLOBALSTATUS) {
			SetWindowPos(top_buttons[i].hwnd, 0, left_offset + (3 * BUTTON_WIDTH_D) + 3, rect.bottom - g_CluiData.statusBarHeight - BUTTON_HEIGHT_D - 1,
                                  rect.right - delta - (3 * BUTTON_WIDTH_D + 5), BUTTON_HEIGHT_D + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
		}
		if(!(top_buttons[i].visibilityOrder & g_CluiData.toolbarVisibility))
			continue;
		if (top_buttons[i].id == IDC_TBTOPSTATUS || top_buttons[i].id == IDC_TBMINIMIZE || top_buttons[i].id == IDC_TABSRMMMENU || top_buttons[i].id == IDC_TABSRMMSLIST) {
			SetWindowPos(top_buttons[i].hwnd, 0, rect.right - right_offset - 2 - (rightButton * (g_CluiData.dwButtonWidth + 1)), 2 + g_CluiData.bCTop, g_CluiData.dwButtonWidth, g_CluiData.dwButtonHeight - 2,
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
			rightButton++;
			continue;
		}
		else {
			SetWindowPos(top_buttons[i].hwnd, 0, left_offset + 3 + (leftButton * (g_CluiData.dwButtonWidth + 1)), 2 + g_CluiData.bCTop, g_CluiData.dwButtonWidth, g_CluiData.dwButtonHeight - 2,
                         SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
			leftButton++;
		}
	}
}

static int FS_FontsChanged(WPARAM wParam, LPARAM lParam)
{
	pcli->pfnClcOptionsChanged();
	RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
	return 0;
}

/*
* create the CLC control, but not yet the frame. The frame containing the CLC should be created as the
* last frame of all.
*/

static int PreCreateCLC(HWND parent)
{
    pcli->hwndContactTree=CreateWindow(CLISTCONTROL_CLASS,_T(""),
		WS_CHILD|CLS_CONTACTLIST
		|(DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT)?CLS_USEGROUPS:0)
		|CLS_HIDEOFFLINE
		//|(DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT)?CLS_HIDEOFFLINE:0)
		|(DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT)?CLS_HIDEEMPTYGROUPS:0)
		|CLS_MULTICOLUMN
		,0,0,0,0,parent,NULL,g_hInst,(LPVOID)0xff00ff00);

	g_clcData = (struct ClcData *)GetWindowLong(pcli->hwndContactTree, 0);
	return((int)pcli->hwndContactTree);
}

/*
* create internal frames, including the last frame (actual CLC control)
*/

static int CreateCLC(HWND parent)
{
    ReloadExtraIcons();
	CallService(MS_CLIST_SETHIDEOFFLINE,(WPARAM)oldhideoffline,0);
	disableautoupd=0;

	{
		CLISTFrame frame = {0};
		frame.cbSize = sizeof(frame);
		frame.tname = _T("EventArea");
		frame.TBtname = TranslateT("Event Area");
		frame.hIcon = 0;
		frame.height = 20;
		frame.Flags=F_VISIBLE|F_SHOWTBTIP|F_NOBORDER|F_TCHAR;
		frame.align = alBottom;
		frame.hWnd = CreateWindowExA(0, "EventAreaClass", "evt", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 20, 20, pcli->hwndContactList, (HMENU) 0, g_hInst, NULL);
        g_hwndEventArea = frame.hWnd;
		hNotifyFrame = (HWND)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&frame,(LPARAM)0);
		CallService(MS_CLIST_FRAMES_UPDATEFRAME, (WPARAM)hNotifyFrame, FU_FMPOS);
		HideShowNotifyFrame();
		CreateViewModeFrame();
	}
	SetButtonToSkinned();

	{
        DWORD flags;
        CLISTFrame Frame;
		memset(&Frame,0,sizeof(Frame));
		Frame.cbSize=sizeof(CLISTFrame);
		Frame.hWnd=pcli->hwndContactTree;
		Frame.align=alClient;
		Frame.hIcon=LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
		Frame.Flags=F_VISIBLE|F_SHOWTB|F_SHOWTBTIP|F_NOBORDER|F_TCHAR;
		Frame.tname=_T("My Contacts");
		Frame.TBtname=TranslateT("My Contacts");
		Frame.height = 200;
		hFrameContactTree=(HWND)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
		//free(Frame.name);
		CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_TBTIPNAME,hFrameContactTree),(LPARAM)Translate("My Contacts"));

		/*
		* ugly, but working hack. Prevent that annoying little scroll bar from appearing in the "My Contacts" title bar
        */

		flags = (DWORD)CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hFrameContactTree), 0);
		flags |= F_VISIBLE;
		CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hFrameContactTree), flags);
	}
	return(0);
}

static int CluiModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	static Update upd = {0};
    static char *szPrefix = "clist_nicer_plus ";
#if defined(_UNICODE)
	static char *component = "CList Nicer+ (Unicode)";
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://miranda.or.at/files/clist_nicer/versionW.txt";
	static char *szUpdateUrl = "http://miranda.or.at/files/clist_nicer/clist_nicer_plusW.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=2601";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=2601";
    upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">CList Nicer+ (NEW) - Unicode ";
#else
	static char *component = "CList Nicer+";
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://miranda.or.at/files/clist_nicer/version.txt";
	static char *szUpdateUrl = "http://miranda.or.at/files/clist_nicer/clist_nicer_plus.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=2602";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=2602";
    upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">CList Nicer+ (NEW) ";
#endif

	// updater plugin support

	upd.cbSize = sizeof(upd);
	upd.szComponentName = pluginInfo.shortName;
	upd.pbVersion = (BYTE *)CreateVersionStringPlugin(&pluginInfo, szCurrentVersion);
	upd.cpbVersion = strlen((char *)upd.pbVersion);
	upd.szVersionURL = szFLVersionUrl;
	upd.szUpdateURL = szFLUpdateurl;

	upd.szBetaUpdateURL = szUpdateUrl;
	upd.szBetaVersionURL = szVersionUrl;
	upd.pbBetaVersionPrefix = (BYTE *)szPrefix;
	upd.pbVersion = szCurrentVersion;
	upd.cpbVersion = lstrlenA(szCurrentVersion);

	upd.cpbVersionPrefix = strlen((char *)upd.pbVersionPrefix);
	upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);

	if(ServiceExists(MS_UPDATE_REGISTER))
		CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
	MTG_OnmodulesLoad(wParam, lParam);
	if(ServiceExists(MS_FONT_REGISTER)) {
        g_CluiData.bFontServiceAvail = TRUE;
		FS_RegisterFonts();
		HookEvent(ME_FONT_RELOAD, FS_FontsChanged);
	}
	return 0;
}

static HICON hIconSaved = 0;

void ClearIcons(int mode)
{
	int i;

	for(i = IDI_OVL_OFFLINE; i <= IDI_OVL_OUTTOLUNCH; i++) {
		if(overlayicons[i - IDI_OVL_OFFLINE] != 0) {
			if(mode)
				DestroyIcon(overlayicons[i - IDI_OVL_OFFLINE]);
			overlayicons[i - IDI_OVL_OFFLINE] = 0;
		}
	}
	hIconSaved = ImageList_GetIcon(himlExtraImages, 3, ILD_NORMAL);
	ImageList_RemoveAll(himlExtraImages);
}

static void CacheClientIcons()
{
	int i = 0;
	char szBuffer[128];

	ClearIcons(0);

	for(i = IDI_OVL_OFFLINE; i <= IDI_OVL_OUTTOLUNCH; i++) {
		mir_snprintf(szBuffer, sizeof(szBuffer), "cln_ovl_%d", ID_STATUS_OFFLINE + (i - IDI_OVL_OFFLINE));
		overlayicons[i - IDI_OVL_OFFLINE] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) szBuffer);
	}
	ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "core_main_14"));
	//ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_web"));
    ImageList_AddIcon(himlExtraImages, (HICON)LoadSkinnedIcon(SKINICON_EVENT_URL));
	ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "core_main_17"));
	if(hIconSaved != 0) {
		ImageList_AddIcon(himlExtraImages, hIconSaved);
		DestroyIcon(hIconSaved);
		hIconSaved = 0;
	}
	else
		ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "core_main_17"));
}

static void InitIcoLib()
{
	SKINICONDESC2 sid;
	char szFilename[MAX_PATH];
	int i = 0, version = 0;
	char szBuffer[128];
	int p_count = 0;
	PROTOCOLDESCRIPTOR **protos = NULL;

	GetModuleFileNameA(g_hInst, szFilename, MAX_PATH);

	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszSection = "CList - Nicer/Default";
	sid.pszDefaultFile = szFilename;
	i = 0;
	do {
		if (myIcons[i].szName == NULL)
			break;
		sid.pszName = myIcons[i].szName;
		sid.pszDescription = Translate(myIcons[i].szDesc);
		sid.iDefaultIndex = myIcons[i].uId;
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
	} while (++i);

	sid.pszName = "CLN_visible";
	sid.pszDescription = "Contact on visible list";
	sid.iDefaultIndex = -IDI_CLVISIBLE;
	CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
	sid.pszName = "CLN_invisible";
	sid.pszDescription = "Contact on invisible list or blocked";
	sid.iDefaultIndex = -IDI_CLINVISIBLE;
	CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
	sid.pszName = "CLN_chatactive";
	sid.pszDescription = "Chat room/IRC channel activity";
	sid.iDefaultIndex = -IDI_OVL_FREEFORCHAT;
	CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);

	sid.pszSection = "CList - Nicer/Overlay Icons";
	for (i = IDI_OVL_OFFLINE; i <= IDI_OVL_OUTTOLUNCH; i++) {
		mir_snprintf(szBuffer, sizeof(szBuffer), "cln_ovl_%d", ID_STATUS_OFFLINE + (i - IDI_OVL_OFFLINE));
		sid.pszName = szBuffer;
		sid.pszDescription = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, ID_STATUS_OFFLINE + (i - IDI_OVL_OFFLINE), 0);
		sid.iDefaultIndex = -i;
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
	}
	sid.pszSection = "CList - Nicer/Connecting Icons";
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&p_count, (LPARAM)&protos);
	for(i = 0; i < p_count; i++) {
		char szDescr[128];
		if(protos[i]->type != PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName, PS_GETCAPS, PFLAGNUM_2, 0) == 0)
			continue;
		mir_snprintf(szBuffer, 128, "%s_conn", protos[i]->szName);
		sid.pszName = szBuffer;
		mir_snprintf(szDescr, 128, "%s Connecting", protos[i]->szName);
		sid.pszDescription = szDescr;
		sid.iDefaultIndex = -IDI_PROTOCONNECTING;
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
	}
}

static int IcoLibChanged(WPARAM wParam, LPARAM lParam)
{
    IcoLibReloadIcons();
	return 0;
}

/*
* if mode != 0 we do first time init, otherwise only reload the extra icon stuff
*/

void CLN_LoadAllIcons(BOOL mode)
{
    if(mode) {
        InitIcoLib();
        hIcoLibChanged = HookEvent(ME_SKIN2_ICONSCHANGED, IcoLibChanged);
        g_CluiData.hIconVisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_visible");
        g_CluiData.hIconInvisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_invisible");
        g_CluiData.hIconChatactive = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_chatactive");
    }
    CacheClientIcons();
}

void ConfigureEventArea(HWND hwnd)
{
	int iCount = GetMenuItemCount(g_CluiData.hMenuNotify);
	DWORD dwFlags = g_CluiData.dwFlags;
	int oldstate = g_CluiData.notifyActive;
    int dwVisible = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hNotifyFrame), 0) & F_VISIBLE;

	if (dwVisible) {
		if (dwFlags & CLUI_FRAME_AUTOHIDENOTIFY)
			g_CluiData.notifyActive = iCount > 0 ? 1 : 0;
		else
			g_CluiData.notifyActive = 1;
	} else
		g_CluiData.notifyActive = 0;

	if (oldstate != g_CluiData.notifyActive)
		HideShowNotifyFrame();
}

void ConfigureFrame()
{
	int i;
	int showCmd;

	for (i = 0; ; i++) {
		if (top_buttons[i].szTooltip == NULL)
			break;
		if (top_buttons[i].hwnd == 0)
			continue;
		switch (top_buttons[i].id) {
		case IDC_TBMENU:
		case IDC_TBGLOBALSTATUS:
			ShowWindow(top_buttons[i].hwnd, g_CluiData.dwFlags & CLUI_FRAME_SHOWBOTTOMBUTTONS ? SW_SHOW : SW_HIDE);
			break;
		default:
			if (g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS) {
				showCmd = (top_buttons[i].visibilityOrder & g_CluiData.toolbarVisibility) ? SW_SHOW : SW_HIDE;
				CheckMenuItem(g_CluiData.hMenuButtons, 50000 + i, MF_BYCOMMAND | (showCmd == SW_SHOW ? MF_CHECKED : MF_UNCHECKED));
			} else
				showCmd = SW_HIDE;
			ShowWindow(top_buttons[i].hwnd, showCmd);
			break;
}	}	}

void IcoLibReloadIcons()
{
	int i;
	HICON hIcon;

	for (i = 0; ; i++) {
		if (top_buttons[i].szTooltip == NULL)
			break;

		if ((top_buttons[i].id == IDC_TABSRMMMENU || top_buttons[i].id == IDC_TABSRMMSLIST) && !g_CluiData.tabSRMM_Avail)
			continue;

		if (top_buttons[i].id == IDC_TBMENU || top_buttons[i].id == IDC_TBGLOBALSTATUS || top_buttons[i].id == IDC_TBTOPSTATUS)
			continue;

		hIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) top_buttons[i].szIcoLibIcon);
        if(top_buttons[i].hwnd && IsWindow(top_buttons[i].hwnd)) {
            SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
            InvalidateRect(top_buttons[i].hwnd, NULL, TRUE);
        }
	}
	g_CluiData.hIconVisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_visible");
	g_CluiData.hIconInvisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_invisible");
	g_CluiData.hIconChatactive = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_chatactive");
	CacheClientIcons();
	ReloadExtraIcons();

    // force client icons reload
    {
        int i;

        for(i = 0; i < g_nextExtraCacheEntry; i++) {
            if(g_ExtraCache[i].hContact)
                NotifyEventHooks(hExtraImageApplying, (WPARAM)g_ExtraCache[i].hContact, 0);
        }
    }
    //
	pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
	pcli->pfnReloadProtoMenus();
	//FYR: Not necessary. It is already notified in pfnReloadProtoMenus
    //NotifyEventHooks(pcli->hPreBuildStatusMenuEvent, 0, 0);
	SendMessage(g_hwndViewModeFrame, WM_USER + 100, 0, 0);
}


static void SetButtonStyle()
{
	int i;

	for (i = 0; ; i++) {
		if (top_buttons[i].szTooltip == NULL)
			break;
		if (top_buttons[i].hwnd == 0 || top_buttons[i].id == IDC_TBGLOBALSTATUS || top_buttons[i].id == IDC_TBMENU)
			continue;
		SendMessage(top_buttons[i].hwnd, BUTTONSETASFLATBTN, 0, g_CluiData.dwFlags & CLUI_FRAME_BUTTONSFLAT ? 0 : 1);
		SendMessage(top_buttons[i].hwnd, BUTTONSETASFLATBTN + 10, 0, g_CluiData.dwFlags & CLUI_FRAME_BUTTONSCLASSIC ? 0 : 1);
	}
}

void CreateButtonBar(HWND hWnd)
{
    int i;
    HICON hIcon;
    HMENU hMenuButtonList = GetSubMenu(g_CluiData.hMenuButtons, 0);

    DeleteMenu(hMenuButtonList, 0, MF_BYPOSITION);

    for (i = 0; ; i++) {
        if (top_buttons[i].szTooltip == NULL)
            break;
        if (top_buttons[i].hwnd)
            continue;

        if (g_ButtonItems && top_buttons[i].id != IDC_TBGLOBALSTATUS && top_buttons[i].id != IDC_TBMENU)
            continue;

        if ((top_buttons[i].id == IDC_TABSRMMMENU || top_buttons[i].id == IDC_TABSRMMSLIST) && !g_CluiData.tabSRMM_Avail)
            continue;

        top_buttons[i].hwnd = CreateWindowEx(0, _T("CLCButtonClass"), _T(""), BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 0, 0, 20, 20, hWnd, (HMENU) top_buttons[i].id, g_hInst, NULL);
        if (top_buttons[i].id != IDC_TBMENU && top_buttons[i].id != IDC_TBGLOBALSTATUS)
            AppendMenu(hMenuButtonList, MF_STRING, 50000 + i, TranslateTS(top_buttons[i].szTooltip));
        if (!g_CluiData.IcoLib_Avail) {
            hIcon = top_buttons[i].hIcon = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(top_buttons[i].idIcon), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
            if(top_buttons[i].idAltIcon)
                top_buttons[i].hAltIcon = LoadImage(g_hInst, MAKEINTRESOURCE(top_buttons[i].idAltIcon), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
        }
        else {
            hIcon = top_buttons[i].hIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) top_buttons[i].szIcoLibIcon);
            if(top_buttons[i].szIcoLibAltIcon)
                top_buttons[i].hAltIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) top_buttons[i].szIcoLibAltIcon);
        }
        if (top_buttons[i].id == IDC_TBMENU) {
            SetWindowText(top_buttons[i].hwnd, TranslateT("Menu"));
            SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
        } else
            SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
        if (top_buttons[i].id == IDC_TBGLOBALSTATUS) {
            SetWindowText(top_buttons[i].hwnd, TranslateT("Offline"));
            SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadSkinnedIcon(SKINICON_STATUS_OFFLINE));
        }
        if (!(top_buttons[i].flags & TOPBUTTON_PUSH))
            SendMessage(top_buttons[i].hwnd, BUTTONSETASPUSHBTN, 0, 0);
        if (top_buttons[i].id != IDC_TBGLOBALSTATUS && top_buttons[i].id != IDC_TBMENU)
            SendMessage(top_buttons[i].hwnd, BUTTONSETASFLATBTN, 0, 0);

        if(top_buttons[i].flags & TOPBUTTON_SENDONDOWN)
            SendMessage(top_buttons[i].hwnd, BM_SETASMENUACTION, 1, 0);

        SendMessage(top_buttons[i].hwnd, BUTTONADDTOOLTIP, (WPARAM) TranslateTS(top_buttons[i].szTooltip), 0);
    }
    SetButtonStyle();
}

void SetTBSKinned(int mode)
{
	int i;

	for (i = 0; ; i++) {
		if (top_buttons[i].szTooltip == NULL)
			break;
		if (top_buttons[i].hwnd == 0 || top_buttons[i].id == IDC_TBGLOBALSTATUS || top_buttons[i].id == IDC_TBMENU)
			continue;
		SendMessage(top_buttons[i].hwnd, BUTTONSETASFLATBTN, 0, 0);
		SendMessage(top_buttons[i].hwnd, BUTTONSETASFLATBTN + 10, 0, 0);
		SendMessage(top_buttons[i].hwnd, BM_SETSKINNED, 0, mode ? MAKELONG(mode, 1) : 0);
	}
	if(!mode)
		SetButtonStyle();           // restore old style
}

void ConfigureCLUIGeometry(int mode)
{
	RECT rcStatus;
	DWORD clmargins = DBGetContactSettingDword(NULL, "CLUI", "clmargins", 0);

	g_CluiData.bCLeft = LOBYTE(LOWORD(clmargins));
	g_CluiData.bCRight = HIBYTE(LOWORD(clmargins));
	g_CluiData.bCTop = LOBYTE(HIWORD(clmargins));
	g_CluiData.bCBottom = HIBYTE(HIWORD(clmargins));

	g_CluiData.dwButtonWidth = g_CluiData.dwButtonHeight = DBGetContactSettingByte(NULL, "CLUI", "TBSize", 19);

    if(mode) {
        if (g_CluiData.dwFlags & CLUI_FRAME_SBARSHOW) {
            SendMessage(pcli->hwndStatus, WM_SIZE, 0, 0);
            GetWindowRect(pcli->hwndStatus, &rcStatus);
            g_CluiData.statusBarHeight = (rcStatus.bottom - rcStatus.top);
        } else
            g_CluiData.statusBarHeight = 0;
    }

	g_CluiData.topOffset = (g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS ? 2 + g_CluiData.dwButtonHeight : 0) + g_CluiData.bCTop;
	g_CluiData.bottomOffset = (g_CluiData.dwFlags & CLUI_FRAME_SHOWBOTTOMBUTTONS ? 2 + BUTTON_HEIGHT_D : 0) + g_CluiData.bCBottom;

	if (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN) {
		g_CluiData.topOffset += 2;
		g_CluiData.bottomOffset += 2;
		g_CluiData.bCLeft += 3;
		g_CluiData.bCRight += 3;
	}
}

void RefreshButtons()
{
	int i;

	for (i = 0; ; i++) {
		if (top_buttons[i].szTooltip == NULL)
			break;
		if (top_buttons[i].hwnd == 0 || top_buttons[i].id == IDC_TBGLOBALSTATUS || top_buttons[i].id == IDC_TBMENU)
			continue;
		InvalidateRect(top_buttons[i].hwnd, NULL, FALSE);
	}
}

/*                                                              
 * set the states of defined database action buttons (only if button is a toggle)
*/

void SetDBButtonStates(HANDLE hPassedContact)
{
    ButtonItem *buttonItem = g_ButtonItems;
    HANDLE hContact = 0, hFinalContact = 0;
    char *szModule, *szSetting;
    int sel = g_clcData ? g_clcData->selection : -1;
    struct ClcContact *contact = 0;

    if(sel != -1 && hPassedContact == 0) {
        sel = pcli->pfnGetRowByIndex(g_clcData, g_clcData->selection, &contact, NULL);
        if(contact && contact->type == CLCIT_CONTACT) {
            hContact = contact->hContact;
        }
    }

    while(buttonItem) {
        BOOL result = FALSE;

        if(!(buttonItem->dwFlags & BUTTON_ISTOGGLE && buttonItem->dwFlags & BUTTON_ISDBACTION)) {
            buttonItem = buttonItem->nextItem;
            continue;
        }
        szModule = buttonItem->szModule;
        szSetting = buttonItem->szSetting;
        if(buttonItem->dwFlags & BUTTON_DBACTIONONCONTACT || buttonItem->dwFlags & BUTTON_ISCONTACTDBACTION) {
            if(hContact == 0) {
                SendMessage(buttonItem->hWnd, BM_SETCHECK, BST_UNCHECKED, 0);
                buttonItem = buttonItem->nextItem;
                continue;
            }
            if(buttonItem->dwFlags & BUTTON_ISCONTACTDBACTION)
                szModule = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
            hFinalContact = hContact;
        }
        else
            hFinalContact = 0;

        if(buttonItem->type == DBVT_ASCIIZ) {
            DBVARIANT dbv = {0};

            if(!DBGetContactSetting(hFinalContact, szModule, szSetting, &dbv)) {
                result = !strcmp((char *)buttonItem->bValuePush, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
        } else {
            switch(buttonItem->type) {
                case DBVT_BYTE:
                {
                    BYTE val = DBGetContactSettingByte(hFinalContact, szModule, szSetting, 0);
                    result = (val == buttonItem->bValuePush[0]);
                    break;
                }
                case DBVT_WORD:
                {
                    WORD val = DBGetContactSettingWord(hFinalContact, szModule, szSetting, 0);
                    result = (val == *((WORD *)&buttonItem->bValuePush));
                    break;
                }
                case DBVT_DWORD:
                {
                    DWORD val = DBGetContactSettingDword(hFinalContact, szModule, szSetting, 0);
                    result = (val == *((DWORD *)&buttonItem->bValuePush));
                    break;
                }
            }
        }
        SendMessage(buttonItem->hWnd, BM_SETCHECK, (WPARAM)result, 0);
        buttonItem = buttonItem->nextItem;
    }
}

/* 
 * set states of standard buttons (pressed/unpressed
 */
void SetButtonStates(HWND hwnd)
{
	BYTE iMode;
    ButtonItem *buttonItem = g_ButtonItems;

	iMode = DBGetContactSettingByte(NULL, "CList", "HideOffline", 0);
    if(!g_ButtonItems) {
        SendDlgItemMessage(hwnd, IDC_TBSOUND, BM_SETIMAGE, IMAGE_ICON, (LPARAM)(g_CluiData.soundsOff ? top_buttons[5].hAltIcon : top_buttons[5].hIcon));
        CheckDlgButton(hwnd, IDC_TBHIDEGROUPS, DBGetContactSettingByte(NULL, "CList", "UseGroups", 0) ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_TBHIDEOFFLINE, iMode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_TBSOUND, g_CluiData.soundsOff ? BST_UNCHECKED : BST_CHECKED);
    }
    else {
        while(buttonItem) {
            if(buttonItem->dwFlags & BUTTON_ISINTERNAL) {
                switch(buttonItem->uId) {
                    case IDC_TBSOUND:
                        SendMessage(buttonItem->hWnd, BM_SETCHECK, g_CluiData.soundsOff ? BST_UNCHECKED : BST_CHECKED, 0);
                        break;
                    case IDC_TBHIDEOFFLINE:
                        SendMessage(buttonItem->hWnd, BM_SETCHECK, iMode ? BST_CHECKED : BST_UNCHECKED, 0);
                        break;
                    case IDC_TBHIDEGROUPS:
                        SendMessage(buttonItem->hWnd, BM_SETCHECK, DBGetContactSettingByte(NULL, "CList", "UseGroups", 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                        break;
                }
            }
            buttonItem = buttonItem->nextItem;
        }
    }
}
// Restore protocols to the last global status.
// Used to reconnect on restore after standby.

static void RestoreMode()
{
	int nStatus;

	nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
	if (nStatus != ID_STATUS_OFFLINE)
		PostMessage(pcli->hwndContactList, WM_COMMAND, nStatus, 0);
}



// Disconnect all protocols.
// Happens on shutdown and standby.
static void DisconnectAll()
{
	PROTOCOLDESCRIPTOR **ppProtoDesc;
	int nProtoCount;
	int nProto;


	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &nProtoCount, (LPARAM) &ppProtoDesc);
	for (nProto = 0; nProto < nProtoCount; nProto++) {
		if (ppProtoDesc[nProto]->type == PROTOTYPE_PROTOCOL)
			CallProtoService(ppProtoDesc[nProto]->szName, PS_SETSTATUS, ID_STATUS_OFFLINE, 0);
	}
}

void BlitWallpaper(HDC hdc, RECT *rc, RECT *rcPaint, struct ClcData *dat)
{
	int x, y;
	int bitx, bity;
	int maxx, maxy;
	int destw, desth, height, width;
	BITMAP *bmp = &g_CluiData.bminfoBg;
	HRGN my_rgn = 0;
	LONG clip = g_CluiData.bClipBorder;

	if(dat == 0)
		return;

	SetStretchBltMode(hdc, HALFTONE);

	y = rc->top;

	rc->left = max(rc->left, clip);
	rc->right = min(rc->right - clip, rc->right);
	rc->top = max(rc->top, clip);
	rc->bottom = min(rc->bottom - clip, rc->bottom);

	width = rc->right - rc->left;
	height = rc->bottom - rc->top;
	my_rgn = CreateRectRgn(rc->left, rc->top, rc->right, rc->bottom);
	SelectClipRgn(hdc, my_rgn);
	maxx = dat->backgroundBmpUse & CLBF_TILEH ? rc->right : rc->left + 1;
	maxy = dat->backgroundBmpUse & CLBF_TILEV ? maxy = rc->bottom : y + 1;
	if (!DBGetContactSettingByte(NULL, "CLCExt", "EXBK_FillWallpaper", 0)) {
		switch (dat->backgroundBmpUse & CLBM_TYPE) {
		case CLB_STRETCH:
			if (dat->backgroundBmpUse & CLBF_PROPORTIONAL) {
				if (width * bmp->bmHeight < height * bmp->bmWidth) {
					desth = height;
					destw = desth * bmp->bmWidth / bmp->bmHeight;
				} else {
					destw = width;
					desth = destw * bmp->bmHeight / bmp->bmWidth;
				}
			} else {
				destw = width;
				desth = height;
			}
			break;
		case CLB_STRETCHH:
			if (dat->backgroundBmpUse & CLBF_PROPORTIONAL) {
				destw = width;
				desth = destw * bmp->bmHeight / bmp->bmWidth;
			} else {
				destw = width;
				desth = bmp->bmHeight;
			}
			break;
		case CLB_STRETCHV:
			if (dat->backgroundBmpUse & CLBF_PROPORTIONAL) {
				desth = height;
				destw = desth * bmp->bmWidth / bmp->bmHeight;
			} else {
				destw = bmp->bmWidth;
				desth = height;
			}
			break;
		default:
			//clb_topleft
			destw = bmp->bmWidth;
			desth = bmp->bmHeight;
			break;
		}
	} else {
		destw = bmp->bmWidth;
		desth = bmp->bmHeight;
	}

	if (DBGetContactSettingByte(NULL, "CLCExt", "EXBK_FillWallpaper", 0)) {
		RECT mrect;
		POINT pt;
		pt.x = rc->left;
		pt.y = rc->top - g_CluiData.bClipBorder;
		ClientToScreen(pcli->hwndContactList, &pt);
		GetWindowRect(pcli->hwndContactList, &mrect);
		bitx = pt.x;
		bity = pt.y;
	} else {
		bitx = 0;
		bity = 0;
	}
	for (; y < maxy; y += desth) {
		//if (y< rc->top - desth)
		//    continue;
		for (x = rc->left; x < maxx; x += destw) {
			//_w = (x + destw < maxx) ? destw : maxx - x;
			//_h = (y + desth < maxy) ? desth : maxy - y;
			StretchBlt(hdc, x, y, destw, desth, g_CluiData.hdcPic, bitx, bity, bmp->bmWidth, bmp->bmHeight, SRCCOPY);
		}
	}
	SelectClipRgn(hdc, NULL);
	DeleteObject(my_rgn);
}

void ReloadThemedOptions()
{
	g_CluiData.bSkinnedToolbar = DBGetContactSettingByte(NULL, "CLUI", "tb_skinned", 1);
	g_CluiData.bSkinnedStatusBar = DBGetContactSettingByte(NULL, "CLUI", "sb_skinned", 0);
	g_CluiData.bUsePerProto = DBGetContactSettingByte(NULL, "CLCExt", "useperproto", 0);
	g_CluiData.bOverridePerStatusColors = DBGetContactSettingByte(NULL, "CLCExt", "override_status", 0);
	g_CluiData.bRowSpacing = DBGetContactSettingByte(NULL, "CLC", "RowGap", 0);
	g_CluiData.exIconScale = DBGetContactSettingByte(NULL, "CLC", "ExIconScale", 16);
	g_CluiData.bApplyIndentToBg = DBGetContactSettingByte(NULL, "CLCExt", "applyindentbg", 0);
	g_CluiData.bWallpaperMode = DBGetContactSettingByte(NULL, "CLUI", "UseBkSkin", 1);
	g_CluiData.bClipBorder = DBGetContactSettingByte(NULL, "CLUI", "clipborder", 0);
	g_CluiData.cornerRadius = DBGetContactSettingByte(NULL, "CLCExt", "CornerRad", 6);
	g_CluiData.gapBetweenFrames = (BYTE)DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
	g_CluiData.bUseDCMirroring = (BYTE)DBGetContactSettingByte(NULL, "CLC", "MirrorDC", 0);
	g_CluiData.bGroupAlign = (BYTE)DBGetContactSettingByte(NULL, "CLC", "GroupAlign", 0);
	if(g_CluiData.hBrushColorKey)
		DeleteObject(g_CluiData.hBrushColorKey);
	g_CluiData.hBrushColorKey = CreateSolidBrush(RGB(255, 0, 255));
	g_CluiData.bUseFloater = (BYTE)DBGetContactSettingByte(NULL, "CLUI", "FloaterMode", 0);
	g_CluiData.bWantFastGradients = (BYTE)DBGetContactSettingByte(NULL, "CLCExt", "FastGradients", 0);
    g_CluiData.titleBarHeight = DBGetContactSettingByte(NULL, "CLCExt", "frame_height", DEFAULT_TITLEBAR_HEIGHT);
    g_CluiData.group_padding = DBGetContactSettingDword(NULL, "CLCExt", "grp_padding", 0);
}

static RECT rcWindow = {0};

static void sttProcessResize(HWND hwnd, NMCLISTCONTROL *nmc)
{
	RECT rcTree, rcWorkArea, rcOld;
	int maxHeight,newHeight;
	int winstyle, skinHeight = 0;

	if (disableautoupd)
		return;

	if (!DBGetContactSettingByte(NULL,"CLUI","AutoSize",0))
		return;

	if (Docking_IsDocked(0, 0))
		return;
	if (hFrameContactTree == 0)
		return;

	maxHeight=DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",75);
	rcOld=rcWindow;

	GetWindowRect(hwnd,&rcWindow);
	GetWindowRect(pcli->hwndContactTree,&rcTree);
	winstyle=GetWindowLong(pcli->hwndContactTree,GWL_STYLE);

	SystemParametersInfo(SPI_GETWORKAREA,0,&rcWorkArea,FALSE);
	if (nmc->pt.y>(rcWorkArea.bottom-rcWorkArea.top)) {
		nmc->pt.y=(rcWorkArea.bottom-rcWorkArea.top);
	}

    if(winstyle & CLS_SKINNEDFRAME) {
        BOOL hasTitleBar = wndFrameCLC ? wndFrameCLC->TitleBar.ShowTitleBar : 0;
        StatusItems_t *item = &StatusItems[(hasTitleBar ? ID_EXTBKOWNEDFRAMEBORDERTB : ID_EXTBKOWNEDFRAMEBORDER) - ID_STATUS_OFFLINE];
        skinHeight = item->IGNORED ? 0 : item->MARGIN_BOTTOM + item->MARGIN_TOP;
    }

	newHeight=max(nmc->pt.y,3)+1+((winstyle&WS_BORDER)?2:0) + skinHeight + (rcWindow.bottom-rcWindow.top)-(rcTree.bottom-rcTree.top);
	if (newHeight==(rcWindow.bottom-rcWindow.top) && show_on_first_autosize == FALSE)
		return;

	if (newHeight>(rcWorkArea.bottom-rcWorkArea.top)*maxHeight/100)
		newHeight=(rcWorkArea.bottom-rcWorkArea.top)*maxHeight/100;
	if (DBGetContactSettingByte(NULL,"CLUI","AutoSizeUpward",0)) {
		rcWindow.top=rcWindow.bottom-newHeight;
		if (rcWindow.top<rcWorkArea.top) rcWindow.top=rcWorkArea.top;
	}
	else {
		rcWindow.bottom=rcWindow.top+newHeight;
		if (rcWindow.bottom>rcWorkArea.bottom) rcWindow.bottom=rcWorkArea.bottom;
	}
	if(g_CluiData.szOldCTreeSize.cx != rcTree.right - rcTree.left) {
		g_CluiData.szOldCTreeSize.cx = rcTree.right - rcTree.left;
		return;
	}
	KillTimer(hwnd, TIMERID_AUTOSIZE);
	SetTimer(hwnd, TIMERID_AUTOSIZE, 100, 0);
}

int CustomDrawScrollBars(NMCSBCUSTOMDRAW *nmcsbcd)
{
    switch(nmcsbcd->hdr.code) {
        case NM_COOLSB_CUSTOMDRAW:
        {
            static HDC hdcScroll = 0;
            static HBITMAP hbmScroll, hbmScrollOld;
            static LONG scrollLeft, scrollRight, scrollHeight, scrollYmin, scrollYmax;

            switch(nmcsbcd->dwDrawStage) {
                case CDDS_PREPAINT:
                    if(g_CluiData.bSkinnedScrollbar)                                              // XXX fix (verify skin items to be complete, otherwise don't draw
                        return CDRF_SKIPDEFAULT;
                    else
                        return CDRF_DODEFAULT;
                case CDDS_POSTPAINT:
                    //BitBlt(nmcsbcd->hdc, scrollLeft, scrollYmin, scrollRight - scrollLeft, scrollYmax - scrollYmin, 
                    //       hdcScroll, scrollLeft, scrollYmin, SRCCOPY);
                    /*SelectObject(hdcScroll, hbmScrollOld);
                    DeleteObject(hbmScroll);
                    DeleteDC(hdcScroll);
                    hdcScroll = 0;*/
                    return 0;
                case CDDS_ITEMPREPAINT:
                {
                    HDC hdc = nmcsbcd->hdc;
                    StatusItems_t *item = 0, *arrowItem = 0;
                    UINT uItemID = ID_EXTBKSCROLLBACK;
                    RECT rcWindow;
                    POINT pt;
                    DWORD dfcFlags;
                    HRGN rgn = 0;
                    GetWindowRect(pcli->hwndContactTree, &rcWindow);
                    pt.x = rcWindow.left; pt.y = rcWindow.top;
                    ScreenToClient(pcli->hwndContactList, &pt);

                    /*
                    if(hdcScroll == 0) {
                        hdcScroll = CreateCompatibleDC(hdc);
                        scrollHeight = rcWindow.bottom - rcWindow.top;
                        scrollYmax = 0;
                        scrollYmin = scrollHeight;
                        hbmScroll = CreateCompatibleBitmap(hdc, rcWindow.right - rcWindow.left, scrollHeight);
                        hbmScrollOld = SelectObject(hdcScroll, hbmScroll);
                        scrollLeft = nmcsbcd->rect.left;
                        scrollRight = nmcsbcd->rect.right;
                    }
                    scrollYmax = max(nmcsbcd->rect.bottom, scrollYmax);
                    scrollYmin = min(nmcsbcd->rect.top, scrollYmin);*/
                    hdcScroll = hdc;
                    BitBlt(hdcScroll, nmcsbcd->rect.left, nmcsbcd->rect.top, nmcsbcd->rect.right - nmcsbcd->rect.left,
                           nmcsbcd->rect.bottom - nmcsbcd->rect.top, g_CluiData.hdcBg, pt.x + nmcsbcd->rect.left, pt.y + nmcsbcd->rect.top, SRCCOPY);

                    switch(nmcsbcd->uItem) {
                        case HTSCROLL_UP:
                        case HTSCROLL_DOWN:
                            uItemID = (nmcsbcd->uState == CDIS_DEFAULT || nmcsbcd->uState == CDIS_DISABLED) ? ID_EXTBKSCROLLBUTTON :
                                (nmcsbcd->uState == CDIS_HOT ? ID_EXTBKSCROLLBUTTONHOVER : ID_EXTBKSCROLLBUTTONPRESSED);
                            break;
                        case HTSCROLL_PAGEGDOWN:
                        case HTSCROLL_PAGEGUP:
                            uItemID = nmcsbcd->uItem == HTSCROLL_PAGEGUP ? ID_EXTBKSCROLLBACK : ID_EXTBKSCROLLBACKLOWER;;
                            rgn = CreateRectRgn(nmcsbcd->rect.left, nmcsbcd->rect.top, nmcsbcd->rect.right, nmcsbcd->rect.bottom);
                            SelectClipRgn(hdcScroll, rgn);
                            break;
                        case HTSCROLL_THUMB:
                            uItemID = nmcsbcd->uState == CDIS_HOT ? ID_EXTBKSCROLLTHUMBHOVER : ID_EXTBKSCROLLTHUMB;
                            uItemID = nmcsbcd->uState == CDIS_SELECTED ? ID_EXTBKSCROLLTHUMBPRESSED : ID_EXTBKSCROLLTHUMB;
                            break;
                        default:
                            break;
                    }

                    uItemID -= ID_STATUS_OFFLINE;
                    item = &StatusItems[uItemID];
                    if(!item->IGNORED) {
                        int alpha = nmcsbcd->uState == CDIS_DISABLED ? item->ALPHA - 50 : item->ALPHA;
                        DrawAlpha(hdcScroll, &nmcsbcd->rect, item->COLOR, alpha, item->COLOR2, item->COLOR2_TRANSPARENT,
                                  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
                    }
                    dfcFlags = DFCS_FLAT | (nmcsbcd->uState == CDIS_DISABLED ? DFCS_INACTIVE : 
                                           (nmcsbcd->uState == CDIS_HOT ? DFCS_HOT : 
                                           (nmcsbcd->uState == CDIS_SELECTED ? DFCS_PUSHED : 0)));

                    if(nmcsbcd->uItem == HTSCROLL_UP)
                        arrowItem = &StatusItems[ID_EXTBKSCROLLARROWUP - ID_STATUS_OFFLINE];
                    if(nmcsbcd->uItem == HTSCROLL_DOWN)
                        arrowItem = &StatusItems[ID_EXTBKSCROLLARROWDOWN - ID_STATUS_OFFLINE];
                    if(arrowItem && !arrowItem->IGNORED)
                        DrawAlpha(hdcScroll, &nmcsbcd->rect, arrowItem->COLOR, arrowItem->ALPHA, arrowItem->COLOR2, arrowItem->COLOR2_TRANSPARENT,
                                  arrowItem->GRADIENT, arrowItem->CORNER, arrowItem->BORDERSTYLE, arrowItem->imageItem);
                    else if(arrowItem)
                        DrawFrameControl(hdcScroll, &nmcsbcd->rect, DFC_SCROLL, (nmcsbcd->uItem == HTSCROLL_UP ? DFCS_SCROLLUP : DFCS_SCROLLDOWN) | dfcFlags);

                    if(rgn) {
                        SelectClipRgn(hdcScroll, NULL);
                        DeleteObject(rgn);
                    }
                }
                default:
                    break;
            }
        }
        return 0;
    }
    return 0;
}

extern LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static int ServiceParamsOK(ButtonItem *item, WPARAM *wParam, LPARAM *lParam, HANDLE hContact)
{
    if(item->dwFlags & BUTTON_PASSHCONTACTW || item->dwFlags & BUTTON_PASSHCONTACTL || item->dwFlags & BUTTON_ISCONTACTDBACTION) {
        if(hContact == 0)
            return 0;
        if(item->dwFlags & BUTTON_PASSHCONTACTW)
            *wParam = (WPARAM)hContact;
        else if(item->dwFlags & BUTTON_PASSHCONTACTL)
            *lParam = (LPARAM)hContact;
        return 1;
    }
    return 1;                                       // doesn't need a paramter
}
static void ShowCLUI(HWND hwnd)
{
	int state = old_cliststate;
	int onTop = DBGetContactSettingByte(NULL, "CList", "OnTop", SETTING_ONTOP_DEFAULT);

	SendMessage(hwnd, WM_SETREDRAW, FALSE, FALSE);
	if (!DBGetContactSettingByte(NULL, "CLUI", "ShowMainMenu", SETTING_SHOWMAINMENU_DEFAULT))
		SetMenu(pcli->hwndContactList, NULL);
	if (state == SETTING_STATE_NORMAL) {
		SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
		ShowWindow(pcli->hwndContactList, SW_SHOWNORMAL);
		SendMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
	}
	else if (state == SETTING_STATE_MINIMIZED) {
		g_CluiData.forceResize = TRUE;
		ShowWindow(pcli->hwndContactList, SW_HIDE);
	}
	else if (state == SETTING_STATE_HIDDEN) {
		g_CluiData.forceResize = TRUE;
		ShowWindow(pcli->hwndContactList, SW_HIDE);
	}
	SetWindowPos(pcli->hwndContactList, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSENDCHANGING);
	DrawMenuBar(hwnd);
	if(g_CluiData.autosize) {
		SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
		SendMessage(pcli->hwndContactTree, WM_SIZE, 0, 0);
	}				
	SFL_Create();
	SFL_SetState(g_CluiData.bUseFloater & CLUI_FLOATER_AUTOHIDE ? (old_cliststate == SETTING_STATE_NORMAL ? 0 : 1) : 1);
}

#define M_CREATECLC  (WM_USER+1)
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:
		{
			int i;
			{
				int flags = WS_CHILD | CCS_BOTTOM;
				flags |= DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? WS_VISIBLE : 0;
				flags |= DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1) ? SBARS_SIZEGRIP : 0;
				pcli->hwndStatus = CreateWindow(STATUSCLASSNAME, NULL, flags, 0, 0, 0, 0, hwnd, NULL, g_hInst, NULL);
				if(flags & WS_VISIBLE) {
					ShowWindow(pcli->hwndStatus, SW_SHOW);
					SendMessage(pcli->hwndStatus, WM_SIZE, 0, 0);
				}
				OldStatusBarProc = (WNDPROC)SetWindowLong(pcli->hwndStatus, GWL_WNDPROC, (LONG)NewStatusBarWndProc);
				SetClassLong(pcli->hwndStatus, GCL_STYLE, GetClassLong(pcli->hwndStatus, GCL_STYLE) & ~(CS_VREDRAW | CS_HREDRAW));
			}
            g_oldSize.cx = g_oldSize.cy = 0;
			old_cliststate = DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);
			DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_HIDDEN);
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_VISIBLE);
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_CLIPCHILDREN);
			if(!g_CluiData.bFirstRun)
				ConfigureEventArea(hwnd);
			CluiProtocolStatusChanged(0, 0);
			ConfigureCLUIGeometry(0);
			for(i = ID_STATUS_OFFLINE; i <= ID_STATUS_OUTTOLUNCH; i++) {
#if defined(_UNICODE)
				char *szTemp = Translate((char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)i, 0));
				MultiByteToWideChar(CP_ACP, 0, szTemp, -1, &statusNames[i - ID_STATUS_OFFLINE][0], 100);
				statusNames[i - ID_STATUS_OFFLINE][100] = 0;
#else
				statusNames[i - ID_STATUS_OFFLINE] = Translate(((char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)i, 0)));
#endif
			}
			//delay creation of CLC so that it can get the status icons right the first time (needs protocol modules loaded)
			if (MySetLayeredWindowAttributes && g_CluiData.bLayeredHack) {
				SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | (WS_EX_LAYERED));
				MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
			}

			if (g_CluiData.isTransparent && MySetLayeredWindowAttributes != NULL) {
				SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
				if (MySetLayeredWindowAttributes)
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), g_CluiData.alpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
			}
			transparentFocus = 1;

#ifndef _DEBUG
			// Miranda is starting up! Restore last status mode.
			// This is not done in debug builds because frequent
			// reconnections will get you banned from the servers.
			{
				int nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
				if (nStatus != ID_STATUS_OFFLINE)
					PostMessage(hwnd, WM_COMMAND, nStatus, 0);
			}
#endif
			CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) GetMenu(hwnd), 0);
			PostMessage(hwnd, M_CREATECLC, 0, 0);
			return FALSE;
		}
	case WM_NCCREATE:
		{
			LPCREATESTRUCT p = (LPCREATESTRUCT)lParam;
			p->style &= ~(CS_HREDRAW | CS_VREDRAW);
		}
		break;
	case M_CREATECLC:
		{
			if(DBGetContactSettingByte(NULL, "CLUI", "useskin", 0))
				IMG_LoadItems();
			CreateButtonBar(hwnd);
            //FYR: to be checked: otherwise it raises double xStatus items
            //NotifyEventHooks(pcli->hPreBuildStatusMenuEvent, 0, 0);
			SendMessage(hwnd, WM_SETREDRAW, FALSE, FALSE);
			{
				BYTE windowStyle = DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", 0);
				ShowWindow(pcli->hwndContactList, SW_HIDE);
				SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, windowStyle != SETTING_WINDOWSTYLE_DEFAULT ? GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | (WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE) : GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_TOOLWINDOW);
				ApplyCLUIBorderStyle(pcli->hwndContactList);

				SetWindowPos(pcli->hwndContactList, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
			}

			if (g_CluiData.soundsOff)
				hSoundHook = HookEvent(ME_SKIN_PLAYINGSOUND, ClcSoundHook);
			if(g_CluiData.bSkinnedToolbar)
				SetTBSKinned(1);
			ConfigureFrame();
			SetButtonStates(hwnd);

			CreateCLC(hwnd);
			g_clcData = (struct ClcData *)GetWindowLong(pcli->hwndContactTree, 0);

            if(MySetLayeredWindowAttributes != 0 && g_CluiData.bFullTransparent) {
                if(g_CLUISkinnedBkColorRGB)
                    Tweak_It(g_CLUISkinnedBkColorRGB);
                else if(g_CluiData.bClipBorder || (g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME))
                    Tweak_It(RGB(255, 0, 255));
                else
                    Tweak_It(g_clcData->bkColour);
            }

			DBWriteContactSettingByte(NULL, "CList", "State", old_cliststate);

            if(DBGetContactSettingByte(NULL, "CList", "AutoApplyLastViewMode", 0)) {
                DBVARIANT dbv = {0};
                if(!DBGetContactSetting(NULL, "CList", "LastViewMode", &dbv)) {
                    if(lstrlenA(dbv.pszVal) > 2) {
                        if(DBGetContactSettingDword(NULL, CLVM_MODULE, dbv.pszVal, -1) != 0xffffffff)
                            ApplyViewMode((char *)dbv.pszVal);
                    }
                    DBFreeVariant(&dbv);
                }
            }
			if(!g_CluiData.autosize)
				ShowCLUI(hwnd);
			else {
				show_on_first_autosize = TRUE;
				RecalcScrollBar(pcli->hwndContactTree, g_clcData);
			}
			return 0;
		}
	case WM_ERASEBKGND:
		return TRUE;
		if(g_CluiData.bSkinnedButtonMode)
			return TRUE;
		return DefWindowProc(hwnd, msg, wParam, lParam);
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc, rcFrame, rcClient;
			HDC hdc;
			HRGN rgn = 0;
			HDC hdcReal = BeginPaint(hwnd, &ps);

			if(during_sizing)
				rcClient = rcWPC;
			else
				GetClientRect(hwnd, &rcClient);
			CopyRect(&rc, &rcClient);

			if(!g_CluiData.hdcBg || rc.right > g_CluiData.dcSize.cx || rc.bottom + g_CluiData.statusBarHeight > g_CluiData.dcSize.cy) {
				RECT rcWorkArea;

				SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, FALSE);
				g_CluiData.dcSize.cy = max(rc.bottom + g_CluiData.statusBarHeight, rcWorkArea.bottom - rcWorkArea.top);
				g_CluiData.dcSize.cx = max(rc.right, (rcWorkArea.right - rcWorkArea.left) / 2);

				if(g_CluiData.hdcBg) {
					SelectObject(g_CluiData.hdcBg, g_CluiData.hbmBgOld);
					DeleteObject(g_CluiData.hbmBg);
					DeleteDC(g_CluiData.hdcBg);
				}
				g_CluiData.hdcBg = CreateCompatibleDC(hdcReal);
				g_CluiData.hbmBg = CreateCompatibleBitmap(hdcReal, g_CluiData.dcSize.cx, g_CluiData.dcSize.cy);
				g_CluiData.hbmBgOld = SelectObject(g_CluiData.hdcBg, g_CluiData.hbmBg);
			}

			if(g_shutDown) {
				EndPaint(hwnd, &ps);
            return 0;
			}

			hdc = g_CluiData.hdcBg;

			CopyRect(&rcFrame, &rcClient);
			if(g_CLUISkinnedBkColor) {
                if(g_CluiData.fOnDesktop) {
                    HDC dc = GetDC(0);
                    RECT rcWin;

                    GetWindowRect(hwnd, &rcWin);
                    BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, dc, rcWin.left, rcWin.top, SRCCOPY);
                }
                else
                    FillRect(hdc, &rcClient, g_CLUISkinnedBkColor);
            }

			if(g_CluiData.bClipBorder != 0 || g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME) {
				int docked = CallService(MS_CLIST_DOCKINGISDOCKED,0,0);
				int clip = g_CluiData.bClipBorder;

				if(!g_CLUISkinnedBkColor)
					FillRect(hdc, &rcClient, g_CluiData.hBrushColorKey);
				if(g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME)
					rgn = CreateRoundRectRgn(clip, docked ? 0 : clip, rcClient.right - clip + 1, rcClient.bottom - (docked ? 0 : clip - 1), 8 + clip, 8 + clip);
				else
					rgn = CreateRectRgn(clip, docked ? 0 : clip, rcClient.right - clip, rcClient.bottom - (docked ? 0 : clip));
				SelectClipRgn(hdc, rgn);
			}

			if(g_CLUIImageItem) {
				IMG_RenderImageItem(hdc, g_CLUIImageItem, &rcFrame);
				g_CluiData.ptW.x = g_CluiData.ptW.y = 0;
				ClientToScreen(hwnd, &g_CluiData.ptW);
				goto skipbg;
			}

			if(g_CluiData.bWallpaperMode)
				FillRect(hdc, &rcClient, g_CluiData.hBrushCLCBk);
			else
				FillRect(hdc, &rcClient, GetSysColorBrush(COLOR_3DFACE));

			rcFrame.left += (g_CluiData.bCLeft - 1);
			rcFrame.right -= (g_CluiData.bCRight - 1);
			//if(!g_CluiData.bSkinnedButtonMode)
			//	rcFrame.bottom -= (g_CluiData.bottomOffset);
			rcFrame.bottom++;
			rcFrame.bottom -= g_CluiData.statusBarHeight;
			if (g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS && g_CluiData.dwFlags & CLUI_FRAME_BUTTONBARSUNKEN) {
				rc.top = g_CluiData.bCTop;;
				rc.bottom = g_CluiData.dwButtonHeight + 2 + g_CluiData.bCTop;
				rc.left++;
				rc.right--;
				DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
			}
			if(g_CluiData.bSkinnedToolbar && !(g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN))
				rcFrame.top = 0;
			else
				rcFrame.top += (g_CluiData.topOffset - 1);

			//if(g_CluiData.neeedSnap)
			//    goto skipbg;
			if (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN) {
				if(g_CluiData.bWallpaperMode && g_clcData != NULL) {
					InflateRect(&rcFrame, -1, -1);
					if(g_CluiData.bmpBackground)
						BlitWallpaper(hdc, &rcFrame, &ps.rcPaint, g_clcData);
					g_CluiData.ptW.x = g_CluiData.ptW.y = 0;
					ClientToScreen(hwnd, &g_CluiData.ptW);
				}
				InflateRect(&rcFrame, 1, 1);
				if(g_CluiData.bSkinnedButtonMode)
					rcFrame.bottom -= (g_CluiData.bottomOffset);
				DrawEdge(hdc, &rcFrame, BDR_SUNKENOUTER, BF_RECT);
			}
			else if(g_CluiData.bWallpaperMode && g_clcData != NULL) {
				if(g_CluiData.bmpBackground)
					BlitWallpaper(hdc, &rcFrame, &ps.rcPaint, g_clcData);
				g_CluiData.ptW.x = g_CluiData.ptW.y = 0;
				ClientToScreen(hwnd, &g_CluiData.ptW);
			}
	skipbg:
			if(g_CluiData.bSkinnedToolbar && g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS) {
				StatusItems_t *item = &StatusItems[ID_EXTBKBUTTONBAR - ID_STATUS_OFFLINE];
				RECT rc = {rcClient.left, 0, rcClient.right, g_CluiData.dwButtonHeight + 2};

				if(!item->IGNORED) {
					rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
					rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
					DrawAlpha(hdc, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
						item->CORNER, item->BORDERSTYLE, item->imageItem);
				}
			}
			BitBlt(hdcReal, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdc, 0, 0, SRCCOPY);
			if(rgn) {
				SelectClipRgn(hdc, NULL);
				DeleteObject(rgn);
			}
			EndPaint(hwnd, &ps);
			return 0;
		}
    case WM_ENTERSIZEMOVE:
        {
            RECT rc;
            POINT pt = {0};

			GetWindowRect(hwnd, &g_PreSizeRect);
            GetClientRect(hwnd, &rc);
            ClientToScreen(hwnd, &pt);
            g_CLUI_x_off = pt.x - g_PreSizeRect.left;
            g_CLUI_y_off = pt.y - g_PreSizeRect.top;
			pt.x = rc.right;
			ClientToScreen(hwnd, &pt);
			g_CLUI_x1_off = g_PreSizeRect.right - pt.x;
			pt.x = 0;
            pt.y = rc.bottom;
            ClientToScreen(hwnd, &pt);
            g_CLUI_y1_off = g_PreSizeRect.bottom - pt.y;
            //g_CluiData.neeedSnap = TRUE;
            break;
        }
	case WM_EXITSIZEMOVE:
		//g_CluiData.neeedSnap = FALSE;
		PostMessage(hwnd, CLUIINTM_REDRAW, 0, 0);
		//RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
		break;
	case WM_SIZING:
		{
			RECT *szrect = (RECT *)lParam;

            break;
            if(Docking_IsDocked(0, 0))
				break;
			g_SizingRect = *((RECT *)lParam);
			if(wParam != WMSZ_BOTTOM && wParam != WMSZ_BOTTOMRIGHT && wParam != WMSZ_BOTTOMLEFT)
				szrect->bottom = g_PreSizeRect.bottom;
			if(wParam != WMSZ_RIGHT && wParam != WMSZ_BOTTOMRIGHT && wParam != WMSZ_TOPRIGHT)
				szrect->right = g_PreSizeRect.right;
			return TRUE;
		}
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *wp = (WINDOWPOS *)lParam;

			if(wp && wp->flags & SWP_NOSIZE)
				return FALSE;

			if(Docking_IsDocked(0, 0))
				break;

			if(pcli->hwndContactList != NULL) {
				RECT rcOld;
				GetWindowRect(hwnd, &rcOld);
				if ((wp->cx != rcOld.right - rcOld.left || wp->cy != rcOld.bottom - rcOld.top) && !(wp->flags & SWP_NOSIZE)) {
                    during_sizing = 1;
                    new_window_rect.left = 0;
                    new_window_rect.right = wp->cx - (g_CLUI_x_off + g_CLUI_x1_off);
                    new_window_rect.top = 0;
                    new_window_rect.bottom = wp->cy - g_CLUI_y_off - g_CLUI_y1_off;
					SizeFramesByWindowRect(&new_window_rect);
					dock_prevent_moving=0;
					LayoutButtons(hwnd, &new_window_rect);
					//SetWindowPos(pcli->hwndStatus, 0, 0, new_window_rect.bottom - g_CluiData.statusBarHeight, new_window_rect.right,
					//	g_CluiData.statusBarHeight, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOCOPYBITS);
					if(wp->cx != g_oldSize.cx)
						SendMessage(hwnd, CLUIINTM_STATUSBARUPDATE, 0, 0);
					dock_prevent_moving=1;
					g_oldPos.x = wp->x;
					g_oldPos.y = wp->y;
					g_oldSize.cx = wp->cx;
					g_oldSize.cy = wp->cy;
					rcWPC = new_window_rect;
					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW);

					during_sizing = 0;
					return(SendMessage(hwnd, WM_WINDOWPOSCHANGED, wParam, (LPARAM)wp));
				}
			}
			during_sizing = 0;
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	case WM_SIZE:

        if (g_CluiData.dwFlags & CLUI_FRAME_SBARSHOW) {
            RECT rcStatus;
            SendMessage(pcli->hwndStatus, WM_SIZE, 0, 0);
            GetWindowRect(pcli->hwndStatus, &rcStatus);
            g_CluiData.statusBarHeight = (rcStatus.bottom - rcStatus.top);
        } else
            g_CluiData.statusBarHeight = 0;


        if((wParam == 0 && lParam == 0) || Docking_IsDocked(0, 0)) {

			if (IsZoomed(hwnd))
				ShowWindow(hwnd, SW_SHOWNORMAL);

			if(pcli->hwndContactList != NULL) {
				RECT rcClient;

				GetClientRect(hwnd, &rcClient);
				SizeFramesByWindowRect(&rcClient);
				LayoutButtons(hwnd, &rcClient);
				if(rcClient.right != g_oldSize.cx)
					PostMessage(hwnd, CLUIINTM_STATUSBARUPDATE, 0, 0);
				g_oldSize.cx = rcClient.right;
				g_oldSize.cy = rcClient.bottom;
			}
		}
		if (wParam == SIZE_MINIMIZED) {
			if (DBGetContactSettingByte(NULL, "CList", "Min2Tray", SETTING_MIN2TRAY_DEFAULT)) {
				ShowWindow(hwnd, SW_HIDE);
				DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_HIDDEN);
			} else
				DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_MINIMIZED);
		}
	case WM_MOVE:
		if (!IsIconic(hwnd)) {
			RECT rc;
			GetWindowRect(hwnd, &rc);

			if (!Docking_IsDocked(0, 0)) {
				cluiPos.bottom = (DWORD) (rc.bottom - rc.top);
				cluiPos.left = rc.left;
				cluiPos.top = rc.top;
			}
			cluiPos.right = rc.right - rc.left;
		}
		return TRUE;
	case WM_SETFOCUS:
		SetFocus(pcli->hwndContactTree);
		return 0;
    case CLUIINTM_REMOVEFROMTASKBAR:
		{
			BYTE windowStyle = DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", SETTING_WINDOWSTYLE_DEFAULT);
			if(windowStyle == SETTING_WINDOWSTYLE_DEFAULT)
				RemoveFromTaskBar(hwnd);
			return 0;
		}
	case WM_ACTIVATE:
		if(g_fading_active) {
			if(wParam != WA_INACTIVE && g_CluiData.isTransparent)
				transparentFocus = 1;
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		if (wParam == WA_INACTIVE) {
			if ((HWND) wParam != hwnd)
				if (g_CluiData.isTransparent)
					if (transparentFocus)
						SetTimer(hwnd, TM_AUTOALPHA, 250, NULL);
		} else {
			if (g_CluiData.isTransparent) {
				KillTimer(hwnd, TM_AUTOALPHA);
				if (MySetLayeredWindowAttributes)
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), g_CluiData.alpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
				transparentFocus = 1;
			}
			SetWindowPos(pcli->hwndContactList, DBGetContactSettingByte(NULL, "CList", "OnTop", SETTING_ONTOP_DEFAULT) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOSENDCHANGING);
		}
		PostMessage(hwnd, CLUIINTM_REMOVEFROMTASKBAR, 0, 0);
		return DefWindowProc(hwnd, msg, wParam, lParam);

	case WM_SETCURSOR:
		if(g_CluiData.isTransparent) {
			if (!transparentFocus && GetForegroundWindow()!=hwnd && MySetLayeredWindowAttributes) {
				MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), g_CluiData.alpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
				transparentFocus=1;
				SetTimer(hwnd, TM_AUTOALPHA,250,NULL);
			}
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	case WM_NCHITTEST:
		{
			LRESULT result;
			RECT r;
			POINT pt;
			int k = 0;
			int clip = g_CluiData.bClipBorder;

			GetWindowRect(hwnd,&r);
			GetCursorPos(&pt);
			if(pt.y <= r.bottom && pt.y >= r.bottom - clip - 6 && !DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) {
				if(pt.x > r.left + clip + 10 && pt.x < r.right - clip - 10)
					return HTBOTTOM;
				if(pt.x < r.left + clip + 10)
					return HTBOTTOMLEFT;
				if(pt.x > r.right - clip - 10)
					return HTBOTTOMRIGHT;

			}
			else if(pt.y >= r.top && pt.y <= r.top + 3 && !DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) {
				if(pt.x > r.left + clip + 10 && pt.x < r.right - clip - 10)
					return HTTOP;
				if(pt.x < r.left + clip + 10)
					return HTTOPLEFT;
				if(pt.x > r.right - clip - 10)
					return HTTOPRIGHT;
			}
			else if(pt.x >= r.left && pt.x <= r.left + clip + 6)
				return HTLEFT;
			else if(pt.x >= r.right - clip - 6 && pt.x <= r.right)
				return HTRIGHT;

			result = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
			if (result == HTSIZE || result == HTTOP || result == HTTOPLEFT || result == HTTOPRIGHT || result == HTBOTTOM || result == HTBOTTOMRIGHT || result == HTBOTTOMLEFT)
				if (g_CluiData.autosize)
					return HTCLIENT;
			return result;
		}

	case WM_TIMER:
		if ((int) wParam == TM_AUTOALPHA) {
			int inwnd;

			if (GetForegroundWindow() == hwnd) {
				KillTimer(hwnd, TM_AUTOALPHA);
				inwnd = 1;
			} else {
				POINT pt;
				HWND hwndPt;
				pt.x = (short) LOWORD(GetMessagePos());
				pt.y = (short) HIWORD(GetMessagePos());
				hwndPt = WindowFromPoint(pt);
				inwnd = (hwndPt == hwnd || GetParent(hwndPt) == hwnd);
			}
			if (inwnd != transparentFocus && MySetLayeredWindowAttributes) {
				//change
				transparentFocus = inwnd;
				if (transparentFocus)
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), g_CluiData.alpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
				else
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), g_CluiData.autoalpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
			}
			if (!transparentFocus)
				KillTimer(hwnd, TM_AUTOALPHA);
		}
		else if(wParam == TIMERID_AUTOSIZE) {
			KillTimer(hwnd, wParam);
			SetWindowPos(hwnd,0,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,rcWindow.bottom-rcWindow.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
			PostMessage(hwnd, WM_SIZE, 0, 0);
			PostMessage(hwnd, CLUIINTM_REDRAW, 0, 0);
		}
		return TRUE;
	case WM_SHOWWINDOW:
		{
			static int noRecurse = 0;
			DWORD thisTick, startTick;
			int sourceAlpha, destAlpha;

			if(g_CluiData.forceResize && wParam != SW_HIDE) {
				g_CluiData.forceResize = FALSE;
				if(0) { //!g_CluiData.fadeinout && MySetLayeredWindowAttributes && g_CluiData.bLayeredHack) {
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), 0, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
					SendMessage(hwnd, WM_SIZE, 0, 0);
					ShowWindow(hwnd, SW_SHOW);
					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), 255, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
				}
				else {
					SendMessage(hwnd, WM_SIZE, 0, 0);
					PostMessage(hwnd, CLUIINTM_REDRAW, 0, 0);
				}
			}
			PostMessage(hwnd, CLUIINTM_REMOVEFROMTASKBAR, 0, 0);
			if(!g_CluiData.fadeinout)
				SFL_SetState(-1);
			if (lParam)
				return DefWindowProc(hwnd, msg, wParam, lParam);
			if (noRecurse)
				return DefWindowProc(hwnd, msg, wParam, lParam);
			if (!g_CluiData.fadeinout || !IsWinVer2000Plus())
				return DefWindowProc(hwnd, msg, wParam, lParam);

			g_fading_active = 1;

			if (wParam) {
				sourceAlpha = 0;
				destAlpha = g_CluiData.isTransparent ? g_CluiData.alpha : 255;
				MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? (COLORREF)g_CluiData.colorkey : RGB(0, 0, 0), (BYTE)sourceAlpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
				noRecurse = 1;
				ShowWindow(hwnd, SW_SHOW);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
				noRecurse = 0;
			} else {
				sourceAlpha = g_CluiData.isTransparent ? (transparentFocus ? g_CluiData.alpha : g_CluiData.autoalpha) : 255;
				destAlpha = 0;
			}
			for (startTick = GetTickCount(); ;) {
				thisTick = GetTickCount();
				if (thisTick >= startTick + 200) {
					SFL_SetState(-1);
					MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), (BYTE) (destAlpha), LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
					g_fading_active = 0;
					return DefWindowProc(hwnd, msg, wParam, lParam);
				}
				MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), (BYTE) (sourceAlpha + (destAlpha - sourceAlpha) * (int) (thisTick - startTick) / 200), LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
			}
			MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), (BYTE) (destAlpha), LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
    case WM_SYSCOMMAND:
        if (wParam == SC_MAXIMIZE)
            return 0;
		else if(wParam == SC_MINIMIZE) {
			//if(DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", SETTING_WINDOWSTYLE_DEFAULT) == SETTING_WINDOWSTYLE_DEFAULT) {
				pcli->pfnShowHide(0, 0);
				return 0;
			//}
		}
        return DefWindowProc(hwnd, msg, wParam, lParam);
	case WM_COMMAND:
		{
			DWORD dwOldFlags = g_CluiData.dwFlags;
			if(HIWORD(wParam) == BN_CLICKED && lParam != 0) {
                if(LOWORD(wParam) == IDC_TBFIRSTUID - 1)
                    break;
                else if(LOWORD(wParam) >= IDC_TBFIRSTUID) {                     // skinnable buttons handling
                    ButtonItem *item = g_ButtonItems;
                    WPARAM wwParam = 0;
                    LPARAM llParam = 0;
                    HANDLE hContact = 0;
                    struct ClcContact *contact = 0;
                    int sel = g_clcData ? g_clcData->selection : -1;
                    int serviceFailure = FALSE;

                    if(sel != -1) {
                        sel = pcli->pfnGetRowByIndex(g_clcData, g_clcData->selection, &contact, NULL);
                        if(contact && contact->type == CLCIT_CONTACT) {
                            hContact = contact->hContact;
                        }
                    }
                    while(item) {
                        if(item->uId == (DWORD)LOWORD(wParam)) {
                            int contactOK = ServiceParamsOK(item, &wwParam, &llParam, hContact);

                            if(item->dwFlags & BUTTON_ISSERVICE) {
                                if(ServiceExists(item->szService) && contactOK)
                                    CallService(item->szService, wwParam, llParam);
                                else if(contactOK)
                                    serviceFailure = TRUE;
                            }
                            else if(item->dwFlags & BUTTON_ISPROTOSERVICE && g_clcData) {
                                if(contactOK) {
                                    char szFinalService[512];

                                    mir_snprintf(szFinalService, 512, "%s/%s", (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), item->szService);
                                    if(ServiceExists(szFinalService))
                                        CallService(szFinalService, wwParam, llParam);
                                    else
                                        serviceFailure = TRUE;
                                }
                            }
                            else if(item->dwFlags & BUTTON_ISDBACTION) {
                                BYTE *pValue;
                                char *szModule = item->szModule;
                                char *szSetting = item->szSetting;
                                HANDLE finalhContact = 0;

                                if(item->dwFlags & BUTTON_ISCONTACTDBACTION || item->dwFlags & BUTTON_DBACTIONONCONTACT) {
                                    contactOK = ServiceParamsOK(item, &wwParam, &llParam, hContact);
                                    if(contactOK && item->dwFlags & BUTTON_ISCONTACTDBACTION)
                                        szModule = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
                                    finalhContact = hContact;
                                }
                                else
                                    contactOK = 1;

                                if(contactOK) {
                                    BOOL fDelete = FALSE;

                                    if(item->dwFlags & BUTTON_ISTOGGLE) {
                                        BOOL fChecked = (SendMessage(item->hWnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED);

                                        pValue = fChecked ? item->bValueRelease : item->bValuePush;
                                        if(fChecked && pValue[0] == 0)
                                            fDelete = TRUE;
                                    }
                                    else
                                        pValue = item->bValuePush;

                                    if(fDelete) {
                                        //_DebugTraceA("delete value: %s, %s ON %d", szModule, szSetting, finalhContact);
                                        DBDeleteContactSetting(finalhContact, szModule, szSetting);
                                    }
                                    else {
                                        switch(item->type) {
                                            case DBVT_BYTE:
                                                DBWriteContactSettingByte(finalhContact, szModule, szSetting, pValue[0]);
                                                //_DebugTraceA("set byte value: %s, %s, %d ON %d", szModule, item->szSetting, pValue[0], finalhContact);
                                                break;
                                            case DBVT_WORD:
                                                DBWriteContactSettingWord(finalhContact, szModule, szSetting, *((WORD *)&pValue[0]));
                                                //_DebugTraceA("set WORD value: %s, %s, %d ON %d", szModule, item->szSetting, *((WORD *)&pValue[0]), finalhContact);
                                                break;
                                            case DBVT_DWORD:
                                                DBWriteContactSettingDword(finalhContact, szModule, szSetting, *((DWORD *)&pValue[0]));
                                                //_DebugTraceA("set DWORD value: %s, %s, %d ON %d", szModule, item->szSetting, *((DWORD *)&pValue[0]), finalhContact);
                                                break;
                                            case DBVT_ASCIIZ:
                                                DBWriteContactSettingString(finalhContact, szModule, szSetting, (char *)pValue);
                                                //_DebugTraceA("set string value: %s, %s, %s ON %d", szModule, item->szSetting, (char *)pValue, finalhContact);
                                                break;
                                        }
                                    }
                                } else if(item->dwFlags & BUTTON_ISTOGGLE)
                                    SendMessage(item->hWnd, BM_SETCHECK, 0, 0);
                            }
                            if(!contactOK)
                                MessageBox(0, _T("The requested action requires a valid contact selection. Please select a contact from the contact list and repeat"), _T("Parameter mismatch"), MB_OK);
                            if(serviceFailure) {
                                char szError[512];

                                mir_snprintf(szError, 512, "The service %s specified by the %s button definition was not found. You may need to install additional plugins", item->szService, item->szName);
                                MessageBoxA(0, szError, "Service failure", MB_OK);
                            }
                            break;
                        }
                        item = item->nextItem;
                    }
                    goto buttons_done;
                }
				switch(LOWORD(wParam)) {
				case IDC_TBMENU:
				case IDC_TBTOPMENU:
					{
						RECT rc;
						HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);

						GetWindowRect(GetDlgItem(hwnd, LOWORD(wParam)), &rc);
						TrackPopupMenu(hMenu, TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON, rc.left, LOWORD(wParam) == IDC_TBMENU ? rc.top : rc.bottom, 0, hwnd, NULL);
						return 0;
					}
				case IDC_TBGLOBALSTATUS:
				case IDC_TBTOPSTATUS:
					{
						RECT rc;
						HMENU hmenu = (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
						GetWindowRect(GetDlgItem(hwnd, LOWORD(wParam)), &rc);
						TrackPopupMenu(hmenu, TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON, rc.left, rc.top, 0, hwnd, NULL);
						return 0;
					}
				case IDC_TABSRMMSLIST:
				case IDC_TABSRMMMENU:
					{
						//CLVM_TestEnum();
						if (ServiceExists("SRMsg_MOD/GetWindowFlags")) {
							CallService("SRMsg_MOD/Show_TrayMenu", 0, LOWORD(wParam) == IDC_TABSRMMSLIST ? 0 : 1);
						}
						return 0;
					}
				case IDC_TBSOUND:
					{
						g_CluiData.soundsOff = !g_CluiData.soundsOff;
						DBWriteContactSettingByte(NULL, "CLUI", "NoSounds", (BYTE)g_CluiData.soundsOff);
						DBWriteContactSettingByte(NULL, "Skin", "UseSound", (BYTE)(g_CluiData.soundsOff ? 0 : 1));
						return 0;
					}
				case IDC_TBSELECTVIEWMODE:
					SendMessage(g_hwndViewModeFrame, WM_COMMAND, IDC_SELECTMODE, lParam);
					break;
				case IDC_TBCLEARVIEWMODE:
					SendMessage(g_hwndViewModeFrame, WM_COMMAND, IDC_RESETMODES, lParam);
					break;
				case IDC_TBCONFIGUREVIEWMODE:
					SendMessage(g_hwndViewModeFrame, WM_COMMAND, IDC_CONFIGUREMODES, lParam);
					break;
				case IDC_TBFINDANDADD:
					CallService(MS_FINDADD_FINDADD, 0, 0);
					return 0;
				case IDC_TBOPTIONS:
					{
						OPENOPTIONSDIALOG ood = {0};

						ood.cbSize = sizeof(ood);
						CallService(MS_OPT_OPENOPTIONS, 0, (LPARAM) &ood);
						return 0;
					}
				default:
					break;
				}
			}
			else if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_MAINMENU), (LPARAM) (HANDLE) NULL))
					return 0;

buttons_done:
			switch (LOWORD(wParam)) {
			case ID_TRAY_EXIT:
			case ID_ICQ_EXIT:
				g_shutDown = 1;
				if (CallService(MS_SYSTEM_OKTOEXIT, 0, 0))
					DestroyWindow(hwnd);
				break;
			case IDC_TBMINIMIZE:
			case ID_TRAY_HIDE:
				pcli->pfnShowHide(0, 0);
				break;
			case POPUP_NEWGROUP:
				SendMessage(pcli->hwndContactTree, CLM_SETHIDEEMPTYGROUPS, 0, 0);
				CallService(MS_CLIST_GROUPCREATE, 0, 0);
				break;
			case POPUP_HIDEOFFLINE:
			case IDC_TBHIDEOFFLINE:
				CallService(MS_CLIST_SETHIDEOFFLINE, (WPARAM) (-1), 0);
				break;
			case POPUP_HIDEOFFLINEROOT:
				SendMessage(pcli->hwndContactTree, CLM_SETHIDEOFFLINEROOT, !SendMessage(pcli->hwndContactTree, CLM_GETHIDEOFFLINEROOT, 0, 0), 0);
				break;
			case POPUP_HIDEEMPTYGROUPS:
				{
					int newVal = !(GetWindowLong(pcli->hwndContactTree, GWL_STYLE) & CLS_HIDEEMPTYGROUPS);
					DBWriteContactSettingByte(NULL, "CList", "HideEmptyGroups", (BYTE) newVal);
					SendMessage(pcli->hwndContactTree, CLM_SETHIDEEMPTYGROUPS, newVal, 0);
					break;
				}
			case POPUP_DISABLEGROUPS:
			case IDC_TBHIDEGROUPS:
				{
					int newVal = !(GetWindowLong(pcli->hwndContactTree, GWL_STYLE) & CLS_USEGROUPS);
					DBWriteContactSettingByte(NULL, "CList", "UseGroups", (BYTE) newVal);
					SendMessage(pcli->hwndContactTree, CLM_SETUSEGROUPS, newVal, 0);
					CheckDlgButton(hwnd, IDC_TBHIDEGROUPS, newVal ? BST_CHECKED : BST_UNCHECKED);
					break;
				}
			case POPUP_HIDEMIRANDA:
				{
					pcli->pfnShowHide(0, 0);
					break;
				}
			case POPUP_VISIBILITY:
				{
					g_CluiData.dwFlags ^= CLUI_SHOWVISI;
					break;
				}
			case POPUP_SHOWMETAICONS:
				{
					g_CluiData.dwFlags ^= CLUI_USEMETAICONS;
					SendMessage(pcli->hwndContactTree, CLM_AUTOREBUILD, 0, 0);
					break;
				}
			case POPUP_FRAME:
				{
					g_CluiData.dwFlags ^= CLUI_FRAME_CLISTSUNKEN;
					break;
				}
			case POPUP_TOOLBAR:
				{
					g_CluiData.dwFlags ^= CLUI_FRAME_SHOWTOPBUTTONS;
					break;
				}
			case POPUP_BUTTONS:
				{
					g_CluiData.dwFlags ^= CLUI_FRAME_SHOWBOTTOMBUTTONS;
					break;
				}
			case POPUP_SHOWSTATUSICONS:
				{
					g_CluiData.dwFlags ^= CLUI_FRAME_STATUSICONS;
					break;
				}
			case POPUP_FLOATER:
				{
					g_CluiData.bUseFloater ^= CLUI_USE_FLOATER;
					if(g_CluiData.bUseFloater & CLUI_USE_FLOATER) {
						SFL_Create();
						SFL_SetState(-1);
					}
					else
						SFL_Destroy();
					DBWriteContactSettingByte(NULL, "CLUI", "FloaterMode", g_CluiData.bUseFloater);
					break;
				}
			case POPUP_FLOATER_AUTOHIDE:
				{
					g_CluiData.bUseFloater ^= CLUI_FLOATER_AUTOHIDE;
					SFL_SetState(g_CluiData.bUseFloater & CLUI_FLOATER_AUTOHIDE ? (DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL) == SETTING_STATE_NORMAL ? 0 : 1) : 1);
					DBWriteContactSettingByte(NULL, "CLUI", "FloaterMode", g_CluiData.bUseFloater);
					break;
				}
			case POPUP_FLOATER_EVENTS:
				{
					g_CluiData.bUseFloater ^= CLUI_FLOATER_EVENTS;
					SFL_SetSize();
					SFL_Update(0, 0, 0, NULL, FALSE);
					DBWriteContactSettingByte(NULL, "CLUI", "FloaterMode", g_CluiData.bUseFloater);
					break;
				}
			}
			if (dwOldFlags != g_CluiData.dwFlags) {
				InvalidateRect(pcli->hwndContactTree, NULL, FALSE);
				DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
				if ((dwOldFlags & (CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_SHOWBOTTOMBUTTONS | CLUI_FRAME_CLISTSUNKEN)) != (g_CluiData.dwFlags & (CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_SHOWBOTTOMBUTTONS | CLUI_FRAME_CLISTSUNKEN))) {
					ConfigureFrame();
					ConfigureCLUIGeometry(1);
				}
				ConfigureEventArea(pcli->hwndContactList);
				SetButtonStyle();
				PostMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
				PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
			}
			return FALSE;
		}
	case WM_LBUTTONDOWN:
		{
			if(g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS || g_ButtonItems) {
				POINT ptMouse, pt;
				RECT rcClient;

				GetCursorPos(&ptMouse);
				pt = ptMouse;
                if(g_ButtonItems)
                    return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
				ScreenToClient(hwnd, &ptMouse);
				GetClientRect(hwnd, &rcClient);
				rcClient.bottom = g_CluiData.topOffset;
				if(PtInRect(&rcClient, ptMouse))
					return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
			}
			break;
		}
	case WM_DISPLAYCHANGE:
		SendMessage(pcli->hwndContactTree, WM_SIZE, 0, 0);   //forces it to send a cln_listsizechanged
		break;
	case WM_NOTIFY:
		if (((LPNMHDR) lParam)->hwndFrom == pcli->hwndContactTree) {
			switch (((LPNMHDR) lParam)->code) {
			case CLN_LISTSIZECHANGE:
				sttProcessResize(hwnd, (NMCLISTCONTROL*)lParam );
				return FALSE;

			case NM_CLICK:
				{
					NMCLISTCONTROL *nm = (NMCLISTCONTROL *) lParam;
					DWORD hitFlags;
					HANDLE hItem;

					hItem = (HANDLE)SendMessage(pcli->hwndContactTree,CLM_HITTEST,(WPARAM)&hitFlags,MAKELPARAM(nm->pt.x,nm->pt.y));

					if ((hitFlags & (CLCHT_NOWHERE | CLCHT_INLEFTMARGIN | CLCHT_BELOWITEMS)) == 0)
						break;
					if (DBGetContactSettingByte(NULL, "CLUI", "ClientAreaDrag", SETTING_CLIENTDRAG_DEFAULT)) {
						POINT pt;
						pt = nm->pt;
						ClientToScreen(pcli->hwndContactTree, &pt);
						return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
				}	}
				return FALSE;
			}
		}
		else if (((LPNMHDR) lParam)->hwndFrom == pcli->hwndStatus) {
			switch (((LPNMHDR) lParam)->code) {
			case NM_CLICK:
				{
					unsigned int nParts, nPanel;
					NMMOUSE *nm = (NMMOUSE *) lParam;
					HMENU hMenu;
					RECT rc;
					POINT pt;
					char *szBuf = NULL, szBuffer[200];
					int pos = 0;
					ProtocolData *pd = 0;

					hMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
					nParts = SendMessage(pcli->hwndStatus, SB_GETPARTS, 0, 0);
					if (nm->dwItemSpec == 0xFFFFFFFE) {
						nPanel = nParts - 1;
						SendMessage(pcli->hwndStatus, SB_GETRECT, nPanel, (LPARAM) &rc);
						if (nm->pt.x < rc.left)
							return FALSE;
					} else {
						nPanel = nm->dwItemSpec;
					}

					//if (nParts > 1)
					//    hMenu = GetSubMenu(hMenu, nPanel);

					szBuffer[0] = 0;
					pd = (ProtocolData *)SendMessageA(pcli->hwndStatus, SB_GETTEXTA, (WPARAM)nPanel, (LPARAM)szBuffer);
					if(pd == NULL)
						break;
					szBuf = pd->RealName;
					SendMessage(pcli->hwndStatus, SB_GETRECT, nPanel, (LPARAM) &rc);
					if(szBuf && lstrlenA(szBuf)) {
						MENUITEMINFOA mii = {0};
						char szName[102];
						int i;
						HMENU hSubmenu = 0;
						char szProtoName[80];

						szProtoName[0] = 0;
						if(!CallProtoService(szBuf, PS_GETNAME, 75, (LPARAM)szProtoName))
							szBuf = szProtoName;

						mii.cbSize = sizeof(mii);
						mii.fMask = MIIM_STRING;
						mii.dwTypeData = szName;
						mii.cch = 100;

						for(i = 0; i < GetMenuItemCount(hMenu); i++) {
							if((hSubmenu = GetSubMenu(hMenu, i)) == 0)
								break;
							mii.dwTypeData = szName;
							mii.cch = 100;
							GetMenuItemInfoA(hSubmenu, 0, TRUE, &mii);
							if(mii.dwTypeData && !strcmp(szBuf, mii.dwTypeData)) {
								hMenu = hSubmenu;
								break;
							}
						}
					}
					pt.x = rc.left;
					pt.y = rc.top;
					ClientToScreen(pcli->hwndStatus, &pt);
					TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
					return FALSE;
				}
			}
		}
		break;
	case WM_CONTEXTMENU:
		{
			RECT rc;
			POINT pt;

			pt.x = (short) LOWORD(lParam);
			pt.y = (short) HIWORD(lParam);
			// x/y might be -1 if it was generated by a kb click
			GetWindowRect(pcli->hwndContactTree, &rc);
			if (pt.x == -1 && pt.y == -1) {
				// all this is done in screen-coords!
				GetCursorPos(&pt);
				// the mouse isnt near the window, so put it in the middle of the window
				if (!PtInRect(&rc, pt)) {
					pt.x = rc.left + (rc.right - rc.left) / 2;
					pt.y = rc.top + (rc.bottom - rc.top) / 2;
				}
			}
			if (PtInRect(&rc, pt)) {
				HMENU hMenu;
				hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDGROUP,0,0);
				TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
				return 0;
			}
			GetWindowRect(pcli->hwndStatus, &rc);
			if (PtInRect(&rc, pt)) {
				HMENU hMenu;
				if (DBGetContactSettingByte(NULL, "CLUI", "SBarRightClk", 0))
					hMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);
				else
					hMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
				TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
				return 0;
			}
			if (g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS) {
				HMENU hMenu;
				int iSelection;
				RECT rcHit;

				GetClientRect(hwnd, &rcHit);
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				hMenu = g_CluiData.hMenuButtons;
				rcHit.bottom = g_CluiData.dwButtonHeight + g_CluiData.bCTop;
				if (!PtInRect(&rcHit, pt))
					break;
				ClientToScreen(hwnd, &pt);
				EnableMenuItem(hMenu, ID_BUTTONBAR_DECREASEBUTTONSIZE, MF_BYCOMMAND | (g_CluiData.dwButtonHeight <= 17 ? MF_GRAYED : MF_ENABLED));
				EnableMenuItem(hMenu, ID_BUTTONBAR_INCREASEBUTTONSIZE, MF_BYCOMMAND | (g_CluiData.dwButtonHeight >= 24 ? MF_GRAYED : MF_ENABLED));
				CheckMenuItem(hMenu, ID_BUTTONBAR_FLATBUTTONS, MF_BYCOMMAND | ((g_CluiData.dwFlags & CLUI_FRAME_BUTTONSFLAT) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem(hMenu, ID_BUTTONBAR_NOVISUALSTYLES, MF_BYCOMMAND | ((g_CluiData.dwFlags & CLUI_FRAME_BUTTONSCLASSIC) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem(hMenu, ID_BUTTONBAR_DRAWSUNKENFRAME, MF_BYCOMMAND | ((g_CluiData.dwFlags & CLUI_FRAME_BUTTONBARSUNKEN) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem(hMenu, ID_BUTTONBAR_SKINNEDTOOLBAR, MF_BYCOMMAND | (g_CluiData.bSkinnedToolbar ? MF_CHECKED : MF_UNCHECKED));

				iSelection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
				if (iSelection >= 50000) {
					int iIndex = iSelection - 50000;
					g_CluiData.toolbarVisibility ^= top_buttons[iIndex].visibilityOrder;
					DBWriteContactSettingDword(NULL, "CLUI", "TBVisibility", g_CluiData.toolbarVisibility);
					ConfigureFrame();
					SendMessage(hwnd, WM_SIZE, 0, 0);
					InvalidateRect(hwnd, NULL, TRUE);
					break;
				}
				switch (iSelection) {
				case ID_BUTTONBAR_DECREASEBUTTONSIZE:
				case ID_BUTTONBAR_INCREASEBUTTONSIZE:
					g_CluiData.dwButtonHeight += (iSelection == ID_BUTTONBAR_DECREASEBUTTONSIZE ? -1 : 1);
					g_CluiData.dwButtonWidth = g_CluiData.dwButtonHeight;
					DBWriteContactSettingByte(NULL, "CLUI", "TBSize", (BYTE) g_CluiData.dwButtonHeight);
					ConfigureCLUIGeometry(1);
					SendMessage(hwnd, WM_SIZE, 0, 0);
					InvalidateRect(hwnd, NULL, TRUE);
					break;
				case ID_BUTTONBAR_NOVISUALSTYLES:
					g_CluiData.dwFlags ^= CLUI_FRAME_BUTTONSCLASSIC;
					SetButtonStyle();
					break;
				case ID_BUTTONBAR_FLATBUTTONS:
					g_CluiData.dwFlags ^= CLUI_FRAME_BUTTONSFLAT;
					SetButtonStyle();
					break;
				case ID_BUTTONBAR_DRAWSUNKENFRAME:
					g_CluiData.dwFlags ^= CLUI_FRAME_BUTTONBARSUNKEN;
					InvalidateRect(hwnd, NULL, FALSE);
					break;
				case ID_BUTTONBAR_SKINNEDTOOLBAR:
					g_CluiData.bSkinnedToolbar = !g_CluiData.bSkinnedToolbar;
					SetTBSKinned(g_CluiData.bSkinnedToolbar);
					DBWriteContactSettingByte(NULL, "CLUI", "tb_skinned", (BYTE)g_CluiData.bSkinnedToolbar);
					PostMessage(hwnd, CLUIINTM_REDRAW, 0, 0);
					break;
				}
				DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
				return 0;
			}
		}
		break;

	case WM_MEASUREITEM:
		if (((LPMEASUREITEMSTRUCT) lParam)->itemData == MENU_MIRANDAMENU) {
			((LPMEASUREITEMSTRUCT) lParam)->itemWidth = g_cxsmIcon * 4 / 3;
			((LPMEASUREITEMSTRUCT) lParam)->itemHeight = 0;
			return TRUE;
		}
		return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

            if(hbmLockedPoint == 0) {
                RECT rc = {0,0,5,5};

                hdcLockedPoint = CreateCompatibleDC(dis->hDC);
                hbmLockedPoint = CreateCompatibleBitmap(dis->hDC, 5, 5);
                hbmOldLockedPoint = SelectObject(hdcLockedPoint, hbmLockedPoint);
            }
			if (dis->hwndItem == pcli->hwndStatus) {
				ProtocolData *pd = (ProtocolData *)dis->itemData;
				int nParts = SendMessage(pcli->hwndStatus, SB_GETPARTS, 0, 0);
				char *szProto;
				int status, x;
				SIZE textSize;
				BYTE showOpts = DBGetContactSettingByte(NULL, "CLUI", "SBarShow", 1);
				if(IsBadCodePtr((FARPROC)pd))
					return TRUE;
				if(g_shutDown)
					return TRUE;
				szProto = pd->RealName;
				status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
				SetBkMode(dis->hDC, TRANSPARENT);
				x = dis->rcItem.left;

				if (showOpts & 1) {
					HICON hIcon;

					if(status >= ID_STATUS_CONNECTING && status < ID_STATUS_OFFLINE) {
						if(g_CluiData.IcoLib_Avail) {
							char szBuffer[128];
							mir_snprintf(szBuffer, 128, "%s_conn", pd->RealName);
							hIcon = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)szBuffer);
						}
						else
							hIcon = g_CluiData.hIconConnecting;
					}
					else if(g_CluiData.bShowXStatusOnSbar && status > ID_STATUS_OFFLINE) {
						ICQ_CUSTOM_STATUS cst = {0};
						char szServiceName[128];
						int xStatus;

						mir_snprintf(szServiceName, 128, "%s%s", pd->RealName, PS_ICQ_GETCUSTOMSTATUSEX);
						cst.cbSize = sizeof(ICQ_CUSTOM_STATUS);
						cst.flags = CSSF_MASK_STATUS;
						cst.status = &xStatus;
						if(ServiceExists(szServiceName) && !CallService(szServiceName, 0, (LPARAM)&cst) && xStatus > 0) {
							hIcon = (HICON)CallProtoService(pd->RealName, PS_ICQ_GETCUSTOMSTATUSICON, 0, LR_SHARED);	// get OWN xStatus icon (if set)
						}
						else
							hIcon = LoadSkinnedProtoIcon(szProto, status);
					}
					else
						hIcon = LoadSkinnedProtoIcon(szProto, status);

					if(!(showOpts & 6) && g_CluiData.bEqualSections)
						x = (dis->rcItem.left + dis->rcItem.right - 16) >> 1;
					if(pd->statusbarpos == 0)
						x += (g_CluiData.bEqualSections ? (g_CluiData.bCLeft/2) : g_CluiData.bCLeft);
					else if(pd->statusbarpos == nParts - 1)
						x -= (g_CluiData.bCRight/2);
					DrawIconEx(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - 16) >> 1, hIcon, 16, 16, 0, NULL, DI_NORMAL);
                    if(DBGetContactSettingByte(NULL, "CLUI", "sbar_showlocked", 1)) {
                        if(DBGetContactSettingByte(NULL, szProto, "LockMainStatus", 0)) {
                            HBRUSH hbr, hbrOld;
                            /*BLENDFUNCTION bf;

                            bf.SourceConstantAlpha = 20;
                            bf.AlphaFormat = 0;
                            bf.BlendFlags = 0;
                            bf.BlendOp = AC_SRC_OVER;
                            AlphaBlend(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - 16) >> 1, 16, 16, hdcLockedPoint,
                                       0, 0, 3, 3, bf);*/
                            BitBlt(hdcLockedPoint, 0, 0, 5, 5, dis->hDC, x + 12, ((dis->rcItem.top + dis->rcItem.bottom - 16) >> 1) + 11, SRCCOPY);
                            hbr = CreateSolidBrush(RGB(255, 0, 0));
                            hbrOld = SelectObject(hdcLockedPoint, hbr);
                            Ellipse(hdcLockedPoint, 0, 0, 5, 5);
                            BitBlt(dis->hDC, x + 12, ((dis->rcItem.top + dis->rcItem.bottom - 16) >> 1) + 11, 5, 5, hdcLockedPoint, 0, 0, SRCCOPY);
                            SelectObject(hdcLockedPoint, hbrOld);
                            DeleteObject(hbr);
                        }
                    }
					x += 18;
				} else {
					x += 2;
					if(pd->statusbarpos == 0)
						x += (g_CluiData.bEqualSections ? (g_CluiData.bCLeft/2) : g_CluiData.bCLeft);
					else if(pd->statusbarpos == nParts - 1)
						x -= (g_CluiData.bCRight/2);
				}
				if (showOpts & 2) {
					char szName[64];
					szName[0] = 0;
					if (CallProtoService(szProto, PS_GETNAME, sizeof(szName), (LPARAM) szName)) {
						strcpy(szName, szProto);
					} //if
					if (lstrlenA(szName) < sizeof(szName) - 1)
						lstrcatA(szName, " ");
					GetTextExtentPoint32A(dis->hDC, szName, lstrlenA(szName), &textSize);
					TextOutA(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - textSize.cy) >> 1, szName, lstrlenA(szName));
					x += textSize.cx;
				}
				if (showOpts & 4) {
					char *szStatus;
					szStatus = (char*) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0);
					if (!szStatus)
						szStatus = "";
					GetTextExtentPoint32A(dis->hDC, szStatus, lstrlenA(szStatus), &textSize);
					TextOutA(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - textSize.cy) >> 1, szStatus, lstrlenA(szStatus));
				}
			}
			else if (dis->CtlType == ODT_MENU) {
				if (dis->itemData == MENU_MIRANDAMENU)
					break;
				return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
			}
			return 0;
		}
	case WM_CLOSE:
        if (DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", SETTING_WINDOWSTYLE_DEFAULT) != SETTING_WINDOWSTYLE_DEFAULT ||
            DBGetContactSettingByte(NULL, "CList", "Min2Tray", SETTING_MIN2TRAY_DEFAULT))
				pcli->pfnShowHide(0, 0);
        else
				SendMessage(hwnd, WM_COMMAND, ID_ICQ_EXIT, 0);
        return FALSE;
	case CLUIINTM_REDRAW:
		if(show_on_first_autosize) {
			show_on_first_autosize = FALSE;
			ShowCLUI(hwnd);
		}
		RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
		return 0;
	case CLUIINTM_STATUSBARUPDATE:
		CluiProtocolStatusChanged(0, 0);
		return 0;
	case WM_DESTROY:
		if(g_CluiData.hdcBg) {
			SelectObject(g_CluiData.hdcBg, g_CluiData.hbmBgOld);
			DeleteObject(g_CluiData.hbmBg);
			DeleteDC(g_CluiData.hdcBg);
			g_CluiData.hdcBg = NULL;
		}
		if(g_CluiData.bmpBackground) {
			SelectObject(g_CluiData.hdcPic, g_CluiData.hbmPicOld);
			DeleteDC(g_CluiData.hdcPic);
			DeleteObject(g_CluiData.bmpBackground);
			g_CluiData.bmpBackground = NULL;
		}
		DestroyMenu(g_CluiData.hMenuButtons);
		FreeProtocolData();
        if(hdcLockedPoint) {
            SelectObject(hdcLockedPoint, hbmOldLockedPoint);
            DeleteObject(hbmLockedPoint);
            DeleteDC(hdcLockedPoint);
        }
		CallService(MS_CLIST_FRAMES_REMOVEFRAME,(WPARAM)hFrameContactTree,(LPARAM)0);
		break;
	}
	return saveContactListWndProc(hwnd, msg, wParam, lParam);
}

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif

static int MetaChanged(WPARAM wParam, LPARAM lParam)
{
	pcli->pfnClcBroadcast(INTM_METACHANGEDEVENT, wParam, lParam);
	return 0;
}

static BOOL g_AboutDlgActive = 0;

BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HICON hIcon;
	COLORREF url_visited = RGB(128, 0, 128);
	COLORREF url_unvisited = RGB(0, 0, 255);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{	int h;
		HFONT hFont;
		LOGFONT lf;

		g_AboutDlgActive = TRUE;
		hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CLNICER,WM_GETFONT,0,0);
		GetObject(hFont,sizeof(lf),&lf);
		h=lf.lfHeight;
		lf.lfHeight=(int)(lf.lfHeight*1.5);
		lf.lfWeight=FW_BOLD;
		hFont=CreateFontIndirect(&lf);
		SendDlgItemMessage(hwndDlg,IDC_CLNICER,WM_SETFONT,(WPARAM)hFont,0);
		lf.lfHeight=h;
		hFont=CreateFontIndirect(&lf);
		SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,(WPARAM)hFont,0);
		}
		{	char str[64];
		DWORD v = pluginInfo.version;
#if defined(_UNICODE)
		mir_snprintf(str,sizeof(str),"%s %d.%d.%d.%d (Unicode)", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)));
#else
		mir_snprintf(str,sizeof(str),"%s %d.%d.%d.%d", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)));
#endif
		SetDlgItemTextA(hwndDlg,IDC_VERSION,str);
		mir_snprintf(str,sizeof(str),Translate("Built %s %s"),__DATE__,__TIME__);
		SetDlgItemTextA(hwndDlg,IDC_BUILDTIME,str);
		}
		hIcon = LoadIcon(GetModuleHandleA("miranda32.exe"), MAKEINTRESOURCE(102));
		//hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_PUMPKIN));
		SendDlgItemMessage(hwndDlg, IDC_LOGO, STM_SETICON, (WPARAM)hIcon, 0);
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		DestroyIcon(hIcon);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDC_SUPPORT:
#if defined(_UNICODE)
		CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://miranda-im.org/download/details.php?action=viewfile&id=2365");
#else
		CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://miranda-im.org/download/details.php?action=viewfile&id=2189");
#endif
		break;
		}
		break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CLNICER)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_VERSION)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_BUILDTIME)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_COPYRIGHT)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_SUPPORT)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO)) {
				if((HWND)lParam==GetDlgItem(hwndDlg,IDC_CLNICER))
					SetTextColor((HDC)wParam,RGB(180,10,10));
				else if((HWND)lParam==GetDlgItem(hwndDlg,IDC_VERSION))
					SetTextColor((HDC)wParam,RGB(70,70,70));
				else
					SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
			break;
	case WM_DESTROY:
		{	HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg,IDC_CLNICER,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CLNICER,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);
			g_AboutDlgActive = FALSE;
		}
		break;
	}
	return FALSE;
}

static int CLN_ShowAbout(WPARAM wParam, LPARAM lParam)
{
	if(!g_AboutDlgActive)
		CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CLNABOUT), 0, DlgProcAbout, 0);
	return 0;
}

static int CLN_ShowMainMenu (WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	POINT pt;

	hMenu = (HMENU)CallService(MS_CLIST_MENUGETMAIN, 0, 0);
	GetCursorPos(&pt);
	TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, pcli->hwndContactList, NULL);
	return 0;
}

static int CLN_ShowStatusMenu(WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	POINT pt;

	hMenu = (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
	GetCursorPos(&pt);
	TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, pcli->hwndContactList, NULL);
	return 0;
}

#define MS_CLUI_SHOWMAINMENU    "CList/ShowMainMenu"
#define MS_CLUI_SHOWSTATUSMENU  "CList/ShowStatusMenu"

void LoadCLUIModule(void)
{
	WNDCLASS wndclass;

	HookEvent(ME_SYSTEM_MODULESLOADED, CluiModulesLoaded);
	HookEvent(ME_MC_DEFAULTTCHANGED, MetaChanged);
	HookEvent(ME_MC_SUBCONTACTSCHANGED, MetaChanged);

	InitGroupMenus();

	wndclass.style = 0;
	wndclass.lpfnWndProc = EventAreaWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = g_hInst;
	wndclass.hIcon = 0;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) (COLOR_3DFACE);
	wndclass.lpszMenuName = 0;
	wndclass.lpszClassName = _T("EventAreaClass");
	RegisterClass(&wndclass);

	oldhideoffline=DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT);
	cluiPos.left = DBGetContactSettingDword(NULL, "CList", "x", 600);
	cluiPos.top = DBGetContactSettingDword(NULL, "CList", "y", 200);
	cluiPos.right = DBGetContactSettingDword(NULL, "CList", "Width", 150);
	cluiPos.bottom = DBGetContactSettingDword(NULL, "CList", "Height", 350);

	LoadExtraIconModule();
	SFL_RegisterWindowClass();

	//laster=GetLastError();
	PreCreateCLC(pcli->hwndContactList);
	LoadCLUIFramesModule();
	CreateServiceFunction("CLN/About",CLN_ShowAbout);
	CreateServiceFunction(MS_CLUI_SHOWMAINMENU,CLN_ShowMainMenu);
	CreateServiceFunction(MS_CLUI_SHOWSTATUSMENU,CLN_ShowStatusMenu);
}

static struct {UINT id; char *name;} _tagFSINFO[] = {
		FONTID_STATUS, LPGEN("Status mode"),
		FONTID_FRAMETITLE, LPGEN("Frame titles"),
		FONTID_EVENTAREA, LPGEN("Event area"),
		FONTID_TIMESTAMP, LPGEN("Contact list local time"),
		0, NULL
};

void FS_RegisterFonts()
{
	FontID fid = {0};
	char szTemp[50];
	DBVARIANT dbv;
	int j = 0;

	fid.cbSize = sizeof(fid);
	strncpy(fid.group, "Contact List", sizeof(fid.group));
	strncpy(fid.dbSettingsGroup, "CLC", 5);
	fid.flags = FIDF_DEFAULTVALID | FIDF_ALLOWEFFECTS | FIDF_APPENDNAME | FIDF_SAVEPOINTSIZE;

	while(_tagFSINFO[j].id != 0) {
		_snprintf(szTemp, sizeof(szTemp), "Font%d", _tagFSINFO[j].id);
		strncpy(fid.prefix, szTemp, sizeof(fid.prefix));
		fid.order = _tagFSINFO[j].id;
		strncpy(fid.name, Translate(_tagFSINFO[j].name), 60);
		_snprintf(szTemp, sizeof(szTemp), "Font%dCol", _tagFSINFO[j].id);
		fid.deffontsettings.colour = (COLORREF)DBGetContactSettingDword(NULL, "CLC", szTemp, GetSysColor(COLOR_WINDOWTEXT));

		_snprintf(szTemp, sizeof(szTemp), "Font%dSize", _tagFSINFO[j].id);
		fid.deffontsettings.size = (BYTE)DBGetContactSettingByte(NULL, "CLC", szTemp, 8);

		_snprintf(szTemp, sizeof(szTemp), "Font%dSty", _tagFSINFO[j].id);
		fid.deffontsettings.style = DBGetContactSettingByte(NULL, "CLC", szTemp, 0);
		_snprintf(szTemp, sizeof(szTemp), "Font%dSet", _tagFSINFO[j].id);
		fid.deffontsettings.charset = DBGetContactSettingByte(NULL, "CLC", szTemp, DEFAULT_CHARSET);
		_snprintf(szTemp, sizeof(szTemp), "Font%dName", _tagFSINFO[j].id);
		if (DBGetContactSetting(NULL, "CLC", szTemp, &dbv))
			lstrcpynA(fid.deffontsettings.szFace, "Arial", LF_FACESIZE);
		else {
			lstrcpynA(fid.deffontsettings.szFace, dbv.pszVal, LF_FACESIZE);
			mir_free(dbv.pszVal);
		}
		CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);
		j++;
	}
}

#ifdef __LXX
UpdateWindowImage()
{
	HDC hdc;
	RECT rc;
	BLENDFUNCTION bf;
	POINT ptDest, ptSrc = {0};
	SIZE szDest;
	HDC screenDC = GetDC(0);
	GetWindowRect(pcli->hwndContactList, &rc);

	ptDest.x = rc.left;
	ptDest.y = rc.top;
	szDest.cx = rc.right - rc.left;
	szDest.cy = rc.bottom - rc.top;

	if(!g_CluiData.hdcBg) {
		HDC dc = GetDC(pcli->hwndContactList);
		g_CluiData.hdcBg = CreateCompatibleDC(dc);
		//g_CluiData.hbmBg = CreateCompatibleBitmap(g_CluiData.hdcBg, rc.right - rc.left, (rc.bottom - rc.top) + g_CluiData.statusBarHeight);
		g_CluiData.hbmBg = CreateCompatibleBitmap(g_CluiData.hdcBg, 2000, 2000 + g_CluiData.statusBarHeight);
		g_CluiData.hbmBgOld = SelectObject(g_CluiData.hdcBg, g_CluiData.hbmBg);
		ReleaseDC(pcli->hwndContactList, dc);
	}
	hdc = g_CluiData.hdcBg;

	if(g_CLUIImageItem) {
		RECT rcDraw;
		rcDraw.left = rcDraw.top = 0;
		rcDraw.right = szDest.cx;
		rcDraw.bottom = szDest.cy;
		//FillRect(hdc, &rc, g_CLUISkinnedBkColor);
		IMG_RenderImageItem(hdc, g_CLUIImageItem, &rcDraw);
		g_CluiData.ptW.x = g_CluiData.ptW.y = 0;
		ClientToScreen(pcli->hwndContactList, &g_CluiData.ptW);
	}
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 255;
	SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) & ~WS_EX_LAYERED);
	SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) | WS_EX_LAYERED);

	MyUpdateLayeredWindow(pcli->hwndContactList, screenDC, &ptDest, &szDest, hdc, &ptSrc, RGB(224, 224, 224), &bf, ULW_ALPHA);
	ReleaseDC(0, screenDC);
}
#endif

