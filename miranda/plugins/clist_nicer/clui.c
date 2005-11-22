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
#include "../../include/m_findadd.h"
#include "m_updater.h"
#include "cluiframes/cluiframes.h"
#include "urlctrl.h"

#define TM_AUTOALPHA  1
#define TIMERID_AUTOSIZE 100
#define MENU_MIRANDAMENU         0xFFFF1234

int g_shutDown = 0;
int g_fading_active = 0;

static RECT g_PreSizeRect, g_SizingRect;
static int g_sizingmethod;
static LONG g_CLUI_x_off, g_CLUI_y_off, g_CLUI_y1_off;
static RECT rcWPC;

static HMODULE hUserDll;
static HIMAGELIST himlMirandaIcon;
HWND hwndContactList = 0;
HWND hwndStatus;
HMENU hMenuMain;
static HANDLE hContactDraggingEvent, hContactDroppedEvent, hContactDragStopEvent;
static HANDLE hStatusBarShowToolTipEvent, hStatusBarHideToolTipEvent;
static int transparentFocus = 1;
static byte oldhideoffline;
static int disableautoupd=1;
HWND hwndContactTree;
HANDLE hFrameContactTree;
extern HIMAGELIST hCListImages;
extern PLUGININFO pluginInfo;
extern HANDLE hClcWindowList;
extern WNDPROC OldStatusBarProc;
extern RECT old_window_rect, new_window_rect;
extern pfnDrawAlpha pDrawAlpha;

extern BOOL g_trayTooltipActive;
extern POINT tray_hover_pos;
extern HWND g_hwndViewModeFrame, g_hwndEventArea;

struct ClcData *g_clcData = NULL;
struct ExtraCache *g_ExtraCache = NULL;
int g_nextExtraCacheEntry = 0;
int g_maxExtraCacheEntry = 0;

extern HANDLE hPreBuildStatusMenuEvent;
extern ImageItem *g_CLUIImageItem;
extern HBRUSH g_CLUISkinnedBkColor;
extern StatusItems_t *StatusItems;
extern HWND g_hwndSFL;

HIMAGELIST himlExtraImages = 0;

RECT cluiPos;
DWORD g_gdiplusToken = 0;

char *szNoevets = "No events...";
    #if defined(_UNICODE)
TCHAR statusNames[12][124];
#else
    TCHAR *statusNames[12];
#endif

BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD) = 0;
BOOL (WINAPI *MyUpdateLayeredWindow)(HWND hwnd, HDC hdcDst, POINT *pptDst,SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags) = 0;

extern LRESULT CALLBACK EventAreaWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HANDLE hNotifyFrame;

//int  (WINAPI *MyMultiByteToWideChar)(UINT uCodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cchMultiByte, LPWSTR lpWideCharStr, int cchWideChar) = 0;
//int  (WINAPI *MyDrawTextW)(HDC hDC, LPCWSTR lpString, int nCount, OUT LPRECT lpRect, UINT uFormat) = 0;

int CluiOptInit(WPARAM wParam, LPARAM lParam);
int SortList(WPARAM wParam, LPARAM lParam);
int LoadCluiServices(void);
void InitGroupMenus();
void InitGdiPlus(void);//, InitImgCache();
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

char *im_clients[] = {
    "MIRANDA","&RQ","TRILLIAN","GAIM","IM2","KOPETE","LICQ","QIP","SIM",
    "ICQ 2000","ICQ 2001B","ICQ 200","ICQ2GO! (FLASH)","ICQ2GO!","ICQ5","ICQ LITE V5","ICQ LITE V4","ICQ LITE",
    "AGILE","GNOMEICU","MOBICQ", "JIMM", "LIBICQ2000", 
    "CENTERICQ", "IMPLUS", "SPAM BOT", "ICQ FOR MAC", 
    "ALICQ", "NICQ", "PSI", "STICQ", "STRICQ", "VICQ", 
    "WEBICQ", "YAMIGO", "YSM", NULL
};

static char *im_clients_descr[] = {
    "Miranda","&RQ","Trillian","GAIM","IM2","Kopete",
    "LICQ","QIP","SIM",
    "ICQ2000","ICQ2001","ICQ2003","ICQ2Go! (Flash)","ICQ2Go! (Java)","icq5","ICQ Lite v5","ICQ Lite v4","ICQ Lite",
    "Agile Messenger","GnomeICU","mobicq / JIMM", "mobicq / JIMM", "libICQ 2000",
    "Center ICQ", "IM+", "Spam Bot", "ICQ for Mac", 
    "Alicq", "Nicq", "Psi", "Sticq", "StrICQ", "vICQ", 
    "WebICQ", "Yamigo", "Ysm", NULL, NULL
};


HICON im_clienthIcons[NR_CLIENTS];
HICON overlayicons[10];

static int clientIcons[] = {
    IDI_CLIENTMIRANDA,IDI_CLIENTRQ,IDI_CLIENTTRILLIAN,IDI_CLIENTGAIM,IDI_CLIENTIM2,IDI_CLIENTKOPETE,IDI_CLIENTLICQ,IDI_CLIENTQIP,IDI_CLIENTSIM,
    IDI_CLIENTICQ2000,IDI_CLIENTICQ2001,IDI_CLIENTICQ2003,IDI_CLIENTICQGOF,IDI_CLIENTICQGOJ,IDI_CLIENTICQLITE5,IDI_CLIENTICQLITE5,IDI_CLIENTICQLITE4,IDI_CLIENTICQLITE,
    IDI_CLIENTAGILE,IDI_CLIENTGNOMEICU,IDI_CLIENTMOBICQ,IDI_CLIENTMOBICQ,IDI_CLIENTLIBICQ,
    IDI_CENTERICQ, IDI_CLIENTIMPLUS, IDI_CLIENTSPAMBOT, IDI_CLIENTICQMAC,
    IDI_CLIENTALICQ, IDI_CLIENTNICQ, IDI_CLIENTPSI, IDI_CLIENTSTICQ, IDI_CLIENTSTRICQ, IDI_CLIENTVICQ,
    IDI_CLIENTWEBICQ, IDI_CLIENTYAMIGO, IDI_CLIENTYSM
};

static struct CluiTopButton top_buttons[] = {
    0, 0, 0, IDC_TBTOPMENU, IDI_CLIENTMIRANDA, 0, "CLN_topmenu", NULL, TOPBUTTON_PUSH, 1, "Show menu", 
    0, 0, 0, IDC_TBHIDEOFFLINE, IDI_HIDEOFFLINE, 0, "CLN_online", NULL, 0, 2, "Show only online contacts", 
    0, 0, 0, IDC_TBHIDEGROUPS, IDI_HIDEGROUPS, 0, "CLN_groups", NULL, 0, 4, "Use contact groups", 
    0, 0, 0, IDC_TBFINDANDADD, IDI_FINDANDADD, 0, "CLN_findadd", NULL, TOPBUTTON_PUSH, 8, "Find and add contacts", 
    0, 0, 0, IDC_TBOPTIONS, IDI_TBOPTIONS, 0, "CLN_options", NULL, TOPBUTTON_PUSH, 16, "Open preferences", 
    0, 0, 0, IDC_TBSOUND, IDI_SOUNDSON, IDI_SOUNDSOFF, "CLN_sound", "CLN_soundsoff", 0, 32, "Toggle sounds", 
    0, 0, 0, IDC_TBMINIMIZE, IDI_MINIMIZE, 0, "CLN_minimize", NULL, TOPBUTTON_PUSH, 64, "Minimize contact list", 
    0, 0, 0, IDC_TBTOPSTATUS, 0, 0, "", NULL, TOPBUTTON_PUSH, 128, "Status menu", 
    0, 0, 0, IDC_TABSRMMSLIST, IDI_TABSRMMSESSIONLIST, 0, "CLN_slist", NULL, TOPBUTTON_PUSH, 256, "tabSRMM session list", 
    0, 0, 0, IDC_TABSRMMMENU, IDI_TABSRMMMENU, 0, "CLN_menu", NULL, TOPBUTTON_PUSH, 512, "tabSRMM Menu", 

    0, 0, 0, IDC_TBSELECTVIEWMODE, IDI_CLVM_SELECT, 0, "CLN_CLVM_select", NULL, TOPBUTTON_PUSH, 1024, "Select view mode", 
    0, 0, 0, IDC_TBCONFIGUREVIEWMODE, IDI_CLVM_OPTIONS, 0, "CLN_CLVM_options", NULL, TOPBUTTON_PUSH, 2048, "Setup view modes", 
    0, 0, 0, IDC_TBCLEARVIEWMODE, IDI_DELETE, 0, "CLN_CLVM_reset", NULL, TOPBUTTON_PUSH, 4096, "Clear view mode", 
    
    0, 0, 0, IDC_TBGLOBALSTATUS, 0, 0, "", NULL, TOPBUTTON_PUSH, 0, "Set status modes", 
    0, 0, 0, IDC_TBMENU, IDI_MINIMIZE, 0, "", NULL, TOPBUTTON_PUSH, 0, "Open main menu", 
    (HWND) - 1, 0, 0, 0, 0, 0, 0, 0, 0
};

#define DEFAULT_TB_VISIBILITY (1 | 2 | 4 | 8 | 16 | 32)

static struct IconDesc myIcons[] = {
    "CLN_online", "Toggle show online/offline", -IDI_HIDEOFFLINE, 
    "CLN_groups", "Toggle groups", -IDI_HIDEGROUPS, 
    "CLN_findadd", "Find contacts", -IDI_FINDANDADD, 
    "CLN_options", "Open preferences", -IDI_TBOPTIONS, 
    "CLN_sound", "Toggle sounds", -IDI_SOUNDSON, 
    "CLN_minimize", "Minimize contact list", -IDI_MINIMIZE, 
    "CLN_slist", "Show tabSRMM session list", -IDI_TABSRMMSESSIONLIST, 
    "CLN_menu", "Show tabSRMM menu", -IDI_TABSRMMMENU, 
    "CLN_soundsoff", "Sounds are off", -IDI_SOUNDSOFF,
    "CLN_email", "Email", -IDI_EMAIL,
    "CLN_web", "Homepage", -IDI_URL,
    "CLN_sms", "SMS", -IDI_SMS,
    "CLN_CLVM_select", "Select view mode", -IDI_CLVM_SELECT,
    "CLN_CLVM_reset", "Reset view mode", -IDI_DELETE,
    "CLN_CLVM_options", "Configure view modes", -IDI_CLVM_OPTIONS,
    "CLN_topmenu", "Show menu", -IDI_CLIENTMIRANDA,
    NULL, NULL, 0
};

static void LayoutButtons(HWND hwnd, HDWP *batch, RECT *rc)
{
    int i;
    RECT rect;
    BYTE rightButton = 1, leftButton = 0;
    BYTE left_offset = g_CluiData.bCLeft - (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN ? 3 : 0);
    BYTE right_offset = g_CluiData.bCRight - (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN ? 3 : 0);
    BYTE delta = left_offset + right_offset;
    
    if(rc == NULL)
        GetClientRect(hwnd, &rect);
    else
        rect = *rc;
    
    rect.bottom -= g_CluiData.bCBottom;
    
    for (i = 0; ; i++) {
        if (top_buttons[i].szTooltip == NULL)
            break;
        if (top_buttons[i].hwnd == 0)
            continue;
        if (top_buttons[i].id == IDC_TBMENU) {
            if(batch)
                *batch = DeferWindowPos(*batch, top_buttons[i].hwnd, 0, 2 + left_offset, rect.bottom - g_CluiData.statusBarHeight - BUTTON_HEIGHT_D - 1,
                                        BUTTON_WIDTH_D * 3, BUTTON_HEIGHT_D + 1, SWP_NOACTIVATE | SWP_NOZORDER);
                                        
        }
        else if (top_buttons[i].id == IDC_TBGLOBALSTATUS) {
            if(batch)
                *batch = DeferWindowPos(*batch, top_buttons[i].hwnd, 0, left_offset + (3 * BUTTON_WIDTH_D) + 3, rect.bottom - g_CluiData.statusBarHeight - BUTTON_HEIGHT_D - 1,
                                        rect.right - delta - (3 * BUTTON_WIDTH_D + 5), BUTTON_HEIGHT_D + 1, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        if(!(top_buttons[i].visibilityOrder & g_CluiData.toolbarVisibility))
            continue;
        if (top_buttons[i].id == IDC_TBTOPSTATUS || top_buttons[i].id == IDC_TBMINIMIZE || top_buttons[i].id == IDC_TABSRMMMENU || top_buttons[i].id == IDC_TABSRMMSLIST) {
            if(batch)
                *batch = DeferWindowPos(*batch, top_buttons[i].hwnd, 0, rect.right - right_offset - 2 - (rightButton * (g_CluiData.dwButtonWidth + 1)), 2 + g_CluiData.bCTop, g_CluiData.dwButtonWidth, g_CluiData.dwButtonHeight - 2, 
                                        SWP_NOACTIVATE | SWP_NOZORDER);
            rightButton++;
            continue;
        }
        else {
            if(batch)
                *batch = DeferWindowPos(*batch, top_buttons[i].hwnd, 0, left_offset + 3 + (leftButton * (g_CluiData.dwButtonWidth + 1)), 2 + g_CluiData.bCTop, g_CluiData.dwButtonWidth, g_CluiData.dwButtonHeight - 2, 
                                        SWP_NOACTIVATE | SWP_NOZORDER);
            leftButton++;
        }
    }
}

static int FS_FontsChanged(WPARAM wParam, LPARAM lParam)
{
    ClcOptionsChanged();
    RedrawWindow(hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);   
    return 0;
}

void CLN_DrawMenuItem(DRAWITEMSTRUCT *dis, HICON hIcon, HICON eventIcon)
{
    if (!IsWinVerXPPlus()) {
        FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
        if (dis->itemState & ODS_HOTLIGHT)
            DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
        else if (dis->itemState & ODS_SELECTED)
            DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);
        if (eventIcon != 0) {
            DrawState(dis->hDC, NULL, NULL, (LPARAM) eventIcon, 0, 2, (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
            DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0, 4 + g_cxsmIcon, (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
        } else
            DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0, (dis->rcItem.right + dis->rcItem.left - g_cxsmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
    } else {
        HBRUSH hBr;
        BOOL bfm = FALSE;
        SystemParametersInfo(SPI_GETFLATMENU, 0, &bfm, 0);
        if (bfm) {
    /* flat menus: fill with COLOR_MENUHILIGHT and outline with COLOR_HIGHLIGHT, otherwise use COLOR_MENUBAR */
            if (dis->itemState & ODS_SELECTED || dis->itemState & ODS_HOTLIGHT) {
    /* selected or hot lighted, no difference */
                hBr = GetSysColorBrush(COLOR_MENUHILIGHT);
                FillRect(dis->hDC, &dis->rcItem, hBr);
                DeleteObject(hBr);
    /* draw the frame */
                hBr = GetSysColorBrush(COLOR_HIGHLIGHT);
                FrameRect(dis->hDC, &dis->rcItem, hBr);
                DeleteObject(hBr);
            } else {
    /* flush the DC with the menu bar colour (only supported on XP) and then draw the icon */
                hBr = GetSysColorBrush(COLOR_MENUBAR);
                FillRect(dis->hDC, &dis->rcItem, hBr);
                DeleteObject(hBr);
            } //if
    /* draw the icon */
            if (eventIcon != 0) {
                DrawState(dis->hDC, NULL, NULL, (LPARAM) eventIcon, 0, 2, (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0, 4 + g_cxsmIcon, (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
            } else
                DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0, (dis->rcItem.right + dis->rcItem.left - g_cxsmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
        } else {
    /* non-flat menus, flush the DC with a normal menu colour */
            FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
            if (dis->itemState & ODS_HOTLIGHT) {
                DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
            } else if (dis->itemState & ODS_SELECTED) {
                DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);
            } //if
            if (eventIcon != 0) {
                DrawState(dis->hDC, NULL, NULL, (LPARAM) eventIcon, 0, 2, (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0, 4 + g_cxsmIcon, (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
            } else
                DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0, (dis->rcItem.right + dis->rcItem.left - g_cxsmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), (dis->rcItem.bottom + dis->rcItem.top - g_cysmIcon) / 2 + (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0, DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
        }
    }
    DestroyIcon(hIcon);
    return;
}

void GetClientID(struct ClcContact *contact, char *client)
{
    int i = 0;
    char szBuffer[128];
    char *_client = _strupr(client);
    
    contact->clientId = -1;

    while (im_clients[i] != NULL) {
        if (strstr(_client, im_clients[i])) {
            contact->clientId = i;
            if(im_clienthIcons[i] == 0) {
                if(g_CluiData.IcoLib_Avail) {
                    mir_snprintf(szBuffer, sizeof(szBuffer), "cln_%s", im_clients[i]);
                    im_clienthIcons[i] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) szBuffer);
                }
                else {
                    im_clienthIcons[i] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(clientIcons[i]), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, 0);
                }
            }
            break;
        }
        i++;
    }
}

/*
 * create the CLC control, but not yet the frame. The frame containing the CLC should be created as the
 * last frame of all.
 */

int PreCreateCLC(HWND parent)
{
    hwndContactTree=CreateWindow(CLISTCONTROL_CLASS,_T(""),
                WS_CHILD|CLS_CONTACTLIST
                |(DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT)?CLS_USEGROUPS:0)
                |CLS_HIDEOFFLINE
                //|(DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT)?CLS_HIDEOFFLINE:0)
                |(DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT)?CLS_HIDEEMPTYGROUPS:0)
                |CLS_MULTICOLUMN
                ,0,0,0,0,parent,NULL,g_hInst,(LPVOID)0xff00ff00);

    g_clcData = (struct ClcData *)GetWindowLong(hwndContactTree, 0);
	return((int)hwndContactTree);
}

/*
 * create internal frames, including the last frame (actual CLC control)
 */

int CreateCLC(HWND parent)
{
    CallService(MS_CLIST_SETHIDEOFFLINE,(WPARAM)oldhideoffline,0);
	{	
        int state=DBGetContactSettingByte(NULL,"CList","State",SETTING_STATE_NORMAL);
		if(state==SETTING_STATE_NORMAL) ShowWindow(hwndContactList, SW_SHOW);
		else if(state==SETTING_STATE_MINIMIZED) ShowWindow(hwndContactList, SW_SHOWMINIMIZED);
	}
	//lastreqh=0;
	disableautoupd=0;

    ReloadExtraIcons();
    {
        CLISTFrame frame = {0};
        frame.cbSize = sizeof(frame);
        frame.name = "EventArea";
        frame.hIcon = 0;
        frame.height = 18;
        frame.TBname = "Event Area";
        frame.Flags=F_VISIBLE|F_SHOWTBTIP;
        frame.align = alBottom;
        frame.hWnd = g_hwndEventArea = CreateWindowExA(0, "EventAreaClass", "evt", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 20, 20, hwndContactList, (HMENU) 0, g_hInst, NULL);
        hNotifyFrame = (HWND)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&frame,(LPARAM)0);
        CallService(MS_CLIST_FRAMES_UPDATEFRAME, (WPARAM)hNotifyFrame, FU_FMPOS);
        if(!g_CluiData.bFirstRun)
            HideShowNotifyFrame();
        CreateViewModeFrame();
    }
    SetButtonToSkinned();

	Sleep(0);
	{
	   // create contact list frame
        DWORD flags;
        
		CLISTFrame Frame;
		memset(&Frame,0,sizeof(Frame));
		Frame.cbSize=sizeof(CLISTFrame);
		Frame.hWnd=hwndContactTree;
		Frame.align=alClient;
		Frame.hIcon=LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
		Frame.Flags=F_VISIBLE|F_SHOWTB|F_SHOWTBTIP|F_NOBORDER;
		Frame.name=(Translate("My Contacts"));
        Frame.height = 200;
		hFrameContactTree=(HWND)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
		//free(Frame.name);
        CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_TBTIPNAME,hFrameContactTree),(LPARAM)Translate("My Contacts"));

        /*
         * ugly, but working hack. Prevent that annoying little scroll bar from appearing in the "My Contacts" title bar
         */
        
        flags = (DWORD)CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hFrameContactTree), 0);
        flags ^= F_NOBORDER;
        CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hFrameContactTree), flags);
        flags ^= F_NOBORDER;
        CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hFrameContactTree), flags);
	}
    return(0);
}

static int CluiModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    MENUITEMINFO mii;
    static Update upd = {0};
#if defined(_UNICODE)
    static char *component = "CList Nicer+ (Unicode)";
    static char szCurrentVersion[30];
    static char *szVersionUrl = "http://miranda.or.at/files/clist_nicer_plus/versionW.txt";
    static char *szUpdateUrl = "http://miranda.or.at/files/clist_nicer_plus/clist_nicer_plusW.zip";
    static char *szFLVersionUrl = "http://www.miranda-im.org/download/details.php?action=viewfile&id=2365";
    static char *szFLUpdateurl = "http://www.miranda-im.org/download/feed.php?dlfile=2365";
#else
    static char *component = "CList Nicer+";
    static char szCurrentVersion[30];
    static char *szVersionUrl = "http://miranda.or.at/files/clist_nicer_plus/version.txt";
    static char *szUpdateUrl = "http://miranda.or.at/files/clist_nicer_plus/clist_nicer_plus.zip";
    static char *szFLVersionUrl = "http://www.miranda-im.org/download/details.php?action=viewfile&id=2189";
    static char *szFLUpdateurl = "http://www.miranda-im.org/download/feed.php?dlfile=2189";
#endif    
    static char *szPrefix = "clist_nicer_plus ";

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = MENUITEMINFO_V4_SIZE;
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);
    SetMenuItemInfo(hMenuMain, 0, TRUE, &mii);
    mii.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
    SetMenuItemInfo(hMenuMain, 1, TRUE, &mii);

    // updater plugin support

    upd.cbSize = sizeof(upd);
    upd.szComponentName = pluginInfo.shortName;
    upd.pbVersion = (BYTE *)CreateVersionStringPlugin(&pluginInfo, szCurrentVersion);
    upd.cpbVersion = strlen((char *)upd.pbVersion);
    upd.szVersionURL = szFLVersionUrl;
    upd.szUpdateURL = szFLUpdateurl;
    upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">Updater ";

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
        FS_RegisterFonts();
        HookEvent(ME_FONT_RELOAD, FS_FontsChanged);
    }
    {
        int state;

        ApplyCLUIBorderStyle(hwndContactList);

        SendMessage(hwndContactList, WM_SIZE, 0, 0);
        SetWindowPos(hwndContactList, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
        RedrawWindow(hwndContactList, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        
        state = DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);
        hMenuMain = GetMenu(hwndContactList);
        if (!DBGetContactSettingByte(NULL, "CLUI", "ShowMainMenu", SETTING_SHOWMAINMENU_DEFAULT))
            SetMenu(hwndContactList, NULL);
        if (state == SETTING_STATE_NORMAL)
            ShowWindow(hwndContactList, SW_SHOW);
        else if (state == SETTING_STATE_MINIMIZED)
            ShowWindow(hwndContactList, SW_SHOWMINIMIZED);
        else if (state == SETTING_STATE_HIDDEN)
            ShowWindow(hwndContactList, SW_HIDE);
        SetWindowPos(hwndContactList, DBGetContactSettingByte(NULL, "CList", "OnTop", SETTING_ONTOP_DEFAULT) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SendMessage(hwndContactList, WM_SIZE, 0, 0);
        PostMessage(hwndContactList, CLUIINTM_REDRAW, 0, 0);
        g_CluiData.forceResize = TRUE;
    }
    return 0;
}

static HICON hIconSaved = 0;

void ClearIcons(int mode)
{
    int i;

    for(i = IDI_CLIENTFIRST; i <= IDI_CLIENTLAST; i++) {
        if(im_clienthIcons[i - IDI_CLIENTFIRST] != 0) {
            if(mode)
                DestroyIcon(im_clienthIcons[i - IDI_CLIENTFIRST]);
            im_clienthIcons[i - IDI_CLIENTFIRST] = 0;
        }
    }

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
    ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_email"));
    ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_web"));
    ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_sms"));
    if(hIconSaved != 0) {
        ImageList_AddIcon(himlExtraImages, hIconSaved);
        DestroyIcon(hIconSaved);
        hIconSaved = 0;
    }
    else
        ImageList_AddIcon(himlExtraImages, (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_sms"));
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

    sid.pszSection = "CList - Nicer/Client Icons";
    i = 0;
    while (im_clients[i] != NULL) {
        mir_snprintf(szBuffer, sizeof(szBuffer), "cln_%s", im_clients[i]);
        sid.pszName = szBuffer;
        sid.pszDescription = im_clients_descr[i];
        sid.iDefaultIndex = -(clientIcons[i]);
        CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
        i++;
    }
    
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
        if(protos[i]->type != PROTOTYPE_PROTOCOL)
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
    int i;
    HICON hIcon;
    
    if (g_CluiData.IcoLib_Avail) {
        if(mode) {
            InitIcoLib();
            hIcoLibChanged = HookEvent(ME_SKIN2_ICONSCHANGED, IcoLibChanged);
            g_CluiData.hIconVisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_visible");
            g_CluiData.hIconInvisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_invisible");
            g_CluiData.hIconChatactive = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_chatactive");
        }
        CacheClientIcons();
    } else {
        if(mode) {
            for (i = IDI_OVL_OFFLINE; i <= IDI_OVL_OUTTOLUNCH; i++)
                overlayicons[i - IDI_OVL_OFFLINE] = LoadImage(g_hInst, MAKEINTRESOURCE(i), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
            g_CluiData.hIconVisible = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_CLVISIBLE), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
            g_CluiData.hIconInvisible = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_CLINVISIBLE), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
            g_CluiData.hIconChatactive = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_OVL_FREEFORCHAT), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
            g_CluiData.hIconConnecting = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PROTOCONNECTING), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
        }
        hIcon = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_EMAIL), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
        ImageList_AddIcon(himlExtraImages, hIcon);
        DestroyIcon(hIcon);
        hIcon = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_URL), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
        ImageList_AddIcon(himlExtraImages, hIcon);
        DestroyIcon(hIcon);
        hIcon = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SMS), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, LR_SHARED);
        ImageList_AddIcon(himlExtraImages, hIcon);
        ImageList_AddIcon(himlExtraImages, hIcon);
        DestroyIcon(hIcon);
    }
}

static int ClcSoundHook(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

void ConfigureEventArea(HWND hwnd)
{
    int iCount = GetMenuItemCount(g_CluiData.hMenuNotify);
    DWORD dwFlags = g_CluiData.dwFlags;
    int oldstate = g_CluiData.notifyActive;

    if (dwFlags & CLUI_FRAME_USEEVENTAREA) {
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
        }
    }
}

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
        SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
        InvalidateRect(top_buttons[i].hwnd, NULL, TRUE);
    }
    g_CluiData.hIconVisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_visible");
    g_CluiData.hIconInvisible = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_invisible");
    g_CluiData.hIconChatactive = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) "CLN_chatactive");
    CacheClientIcons();
    NotifyEventHooks(hPreBuildStatusMenuEvent, 0, 0);
    ReloadExtraIcons();
    WindowList_Broadcast(hClcWindowList, CLM_AUTOREBUILD, 0, 0);
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

void ConfigureCLUIGeometry()
{
    RECT rcStatus;
    DWORD clmargins = DBGetContactSettingDword(NULL, "CLUI", "clmargins", 0);

    g_CluiData.bCLeft = LOBYTE(LOWORD(clmargins));
    g_CluiData.bCRight = HIBYTE(LOWORD(clmargins));
    g_CluiData.bCTop = LOBYTE(HIWORD(clmargins));
    g_CluiData.bCBottom = HIBYTE(HIWORD(clmargins));
    
    g_CluiData.dwButtonWidth = g_CluiData.dwButtonHeight = DBGetContactSettingByte(NULL, "CLUI", "TBSize", 19);

    if (g_CluiData.dwFlags & CLUI_FRAME_SBARSHOW) {
        GetWindowRect(hwndStatus, &rcStatus);
        g_CluiData.statusBarHeight = (rcStatus.bottom - rcStatus.top);
    } else
        g_CluiData.statusBarHeight = 0;

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

void SetButtonStates(HWND hwnd)
{
    BYTE iMode;
    
    CheckDlgButton(hwnd, IDC_TBHIDEGROUPS, DBGetContactSettingByte(NULL, "CList", "UseGroups", 0) ? BST_CHECKED : BST_UNCHECKED);
    iMode = DBGetContactSettingByte(NULL, "CList", "HideOffline", 0);
    CheckDlgButton(hwnd, IDC_TBHIDEOFFLINE, iMode ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_TBSOUND, g_CluiData.soundsOff ? BST_UNCHECKED : BST_CHECKED);
    SendDlgItemMessage(hwnd, IDC_TBSOUND, BM_SETIMAGE, IMAGE_ICON, (LPARAM)(g_CluiData.soundsOff ? top_buttons[5].hAltIcon : top_buttons[5].hIcon));
}
// Restore protocols to the last global status.
// Used to reconnect on restore after standby.

static void RestoreMode()
{
    int nStatus;

    nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
    if (nStatus != ID_STATUS_OFFLINE)
        PostMessage(hwndContactList, WM_COMMAND, nStatus, 0);
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
        pt.y = rc->top;
        ClientToScreen(hwndContactList, &pt);
        GetWindowRect(hwndContactList, &mrect);
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
    g_CluiData.bSkinnedToolbar = DBGetContactSettingByte(NULL, "CLUI", "tb_skinned", 0);
    g_CluiData.bSkinnedStatusBar = DBGetContactSettingByte(NULL, "CLUI", "sb_skinned", 0);
    g_CluiData.bUsePerProto = DBGetContactSettingByte(NULL, "CLCExt", "useperproto", 0);
    g_CluiData.bOverridePerStatusColors = DBGetContactSettingByte(NULL, "CLCExt", "override_status", 0);
    g_CluiData.bRowSpacing = DBGetContactSettingByte(NULL, "CLC", "RowGap", 0);
    g_CluiData.exIconScale = DBGetContactSettingByte(NULL, "CLC", "ExIconScale", 16);
    g_CluiData.bApplyIndentToBg = DBGetContactSettingByte(NULL, "CLCExt", "applyindentbg", 0);
    g_CluiData.bWallpaperMode = DBGetContactSettingByte(NULL, "CLUI", "UseBkSkin", 0);
    g_CluiData.bClipBorder = DBGetContactSettingByte(NULL, "CLUI", "clipborder", 0);
    g_CluiData.cornerRadius = DBGetContactSettingByte(NULL, "CLCExt", "CornerRad", 6);
    g_CluiData.gapBetweenFrames = (BYTE)DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
    g_CluiData.bUseDCMirroring = (BYTE)DBGetContactSettingByte(NULL, "CLC", "MirrorDC", 0);
	if(g_CluiData.hBrushColorKey)
		DeleteObject(g_CluiData.hBrushColorKey);
	g_CluiData.hBrushColorKey = CreateSolidBrush(RGB(255, 0, 255));
	g_CluiData.bUseFloater = (BYTE)DBGetContactSettingByte(NULL, "CLUI", "FloaterMode", 0);
}

static RECT rcWindow = {0};

#define M_CREATECLC  (WM_USER+1)
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    {
        LRESULT result;
        MSG m;
        m.hwnd = hwnd;
        m.message = msg;
        m.wParam = wParam;
        m.lParam = lParam;
        if (CallService(MS_CLIST_DOCKINGPROCESSMESSAGE, (WPARAM) &m, (LPARAM) &result))
            return result;
        if (CallService(MS_CLIST_TRAYICONPROCESSMESSAGE, (WPARAM) &m, (LPARAM) &result))
            return result;
        if (CallService(MS_CLIST_HOTKEYSPROCESSMESSAGE, (WPARAM) &m, (LPARAM) &result))
            return result;
    }

    switch (msg) {
        case WM_CREATE:
            {
                int i;
                HICON hIcon;
                HMENU hMenuButtonList;
                //InstallDialogBoxHook();
                ZeroMemory((void*) &g_CluiData, sizeof(g_CluiData));
                g_CluiData.bMetaAvail = ServiceExists(MS_MC_GETDEFAULTCONTACT) ? TRUE : FALSE;
                if(g_CluiData.bMetaAvail)
                    mir_snprintf(g_CluiData.szMetaName, 256, "%s", (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0));
                else
                    strncpy(g_CluiData.szMetaName, "MetaContacts", 255);

                g_ExtraCache = malloc(sizeof(struct ExtraCache) * EXTRAIMAGECACHESIZE);
                ZeroMemory(g_ExtraCache, sizeof(struct ExtraCache) * EXTRAIMAGECACHESIZE);
                g_nextExtraCacheEntry = 0;
                g_maxExtraCacheEntry = EXTRAIMAGECACHESIZE;
                
                g_CluiData.bMetaEnabled = DBGetContactSettingByte(NULL, "MetaContacts", "Enabled", 1);
                g_CluiData.bAvatarServiceAvail = ServiceExists(MS_AV_GETAVATARBITMAP) ? TRUE : FALSE;
                if(g_CluiData.bAvatarServiceAvail)
                    HookEvent(ME_AV_AVATARCHANGED, AvatarChanged);
                g_CluiData.tabSRMM_Avail = ServiceExists("SRMsg_MOD/GetWindowFlags") ? TRUE : FALSE;
                g_CluiData.IcoLib_Avail = ServiceExists(MS_SKIN2_ADDICON) ? TRUE : FALSE;
                g_CluiData.toolbarVisibility = DBGetContactSettingDword(NULL, "CLUI", "TBVisibility", DEFAULT_TB_VISIBILITY);
                g_CluiData.hMenuButtons = GetSubMenu(LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT)), 3);
                g_CluiData.hMenuNotify = CreatePopupMenu();
                g_CluiData.wNextMenuID = 1;
                g_CluiData.sortTimer = DBGetContactSettingDword(NULL, "CLC", "SortTimer", 150);
                g_CluiData.szNoEvents = Translate(szNoevets);
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
                if(!pDrawAlpha)
                    pDrawAlpha = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS  && g_gdiplusToken) ? GDIp_DrawAlpha : DrawAlpha;

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
                hMenuButtonList = GetSubMenu(g_CluiData.hMenuButtons, 0);
                DeleteMenu(hMenuButtonList, 0, MF_BYPOSITION);
                ZeroMemory((void *)im_clienthIcons, sizeof(HICON) * NR_CLIENTS);
                ZeroMemory((void *)overlayicons, sizeof(HICON) * 10);
                
                CLN_LoadAllIcons(1);
                
                CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) GetMenu(hwnd), 0);
                DrawMenuBar(hwnd);
                g_CluiData.dwFlags = DBGetContactSettingDword(NULL, "CLUI", "Frameflags", CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_USEEVENTAREA | CLUI_FRAME_STATUSICONS | CLUI_FRAME_SHOWBOTTOMBUTTONS);
                g_CluiData.dwFlags |= (DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? CLUI_FRAME_SBARSHOW : 0);
                g_CluiData.soundsOff = DBGetContactSettingByte(NULL, "CLUI", "NoSounds", 0);

                pDrawAlpha = NULL;
                if(!pDrawAlpha) {
                    pDrawAlpha = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS && g_gdiplusToken) ? GDIp_DrawAlpha : DrawAlpha;
                }
                
                if (g_CluiData.soundsOff)
                    hSoundHook = HookEvent(ME_SKIN_PLAYINGSOUND, ClcSoundHook);
                if(DBGetContactSettingByte(NULL, "Skin", "UseSound", 0) != g_CluiData.soundsOff)
                    DBWriteContactSettingByte(NULL, "Skin", "UseSound", g_CluiData.soundsOff ? 0 : 1);

				//create the status wnd
                {
                    int flags = WS_CHILD | CCS_BOTTOM;
                    flags |= DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? WS_VISIBLE : 0;
                    flags |= DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1) ? SBARS_SIZEGRIP : 0;
                    hwndStatus = CreateWindow(STATUSCLASSNAME, NULL, flags, 0, 0, 0, 0, hwnd, NULL, g_hInst, NULL);
                    if(flags & WS_VISIBLE) {
                        ShowWindow(hwndStatus, SW_SHOW);
                        SendMessage(hwndStatus, WM_SIZE, 0, 0);
                    }
                    OldStatusBarProc = (WNDPROC)SetWindowLong(hwndStatus, GWL_WNDPROC, (LONG)NewStatusBarWndProc);
                    SetClassLong(hwndStatus, GCL_STYLE, GetClassLong(hwndStatus, GCL_STYLE) & ~(CS_VREDRAW | CS_HREDRAW));
                }
                if(!g_CluiData.bFirstRun)
                    ConfigureEventArea(hwnd);
                CluiProtocolStatusChanged(0, 0);
                ConfigureCLUIGeometry();
                for (i = 0; ; i++) {
                    if (top_buttons[i].szTooltip == NULL)
                        break;
                    if ((top_buttons[i].id == IDC_TABSRMMMENU || top_buttons[i].id == IDC_TABSRMMSLIST) && !g_CluiData.tabSRMM_Avail)
                        continue;

                    top_buttons[i]. hwnd = CreateWindowExA(0, "CLCButtonClass", "", BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 0, 0, 20, 20, hwnd, (HMENU) top_buttons[i].id, g_hInst, NULL);
                    if (top_buttons[i].id != IDC_TBMENU && top_buttons[i].id != IDC_TBGLOBALSTATUS)
                        AppendMenuA(hMenuButtonList, MF_STRING, 50000 + i, top_buttons[i].szTooltip);
                    if (!g_CluiData.IcoLib_Avail) {
                        hIcon = top_buttons[i].hIcon = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(top_buttons[i].idIcon), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, 0);
                        if(top_buttons[i].idAltIcon)
                            top_buttons[i].hAltIcon = LoadImage(g_hInst, MAKEINTRESOURCE(top_buttons[i].idAltIcon), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, 0);
                    }
                    else {
                        hIcon = top_buttons[i].hIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) top_buttons[i].szIcoLibIcon);
                        if(top_buttons[i].szIcoLibAltIcon)
                            top_buttons[i].hAltIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) top_buttons[i].szIcoLibAltIcon);
                    }
                    if (top_buttons[i].id == IDC_TBMENU) {
                        SetWindowTextA(top_buttons[i].hwnd, "Menu");
                        SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
                    } else
                        SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
                    if (top_buttons[i].id == IDC_TBGLOBALSTATUS) {
                        SetWindowTextA(top_buttons[i].hwnd, "Offline");
                        SendMessage(top_buttons[i].hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadSkinnedIcon(SKINICON_STATUS_OFFLINE));
                    }
                    if (!(top_buttons[i].flags & TOPBUTTON_PUSH))
                        SendMessage(top_buttons[i].hwnd, BUTTONSETASPUSHBTN, 0, 0);
                    if (top_buttons[i].id != IDC_TBGLOBALSTATUS && top_buttons[i].id != IDC_TBMENU)
                        SendMessage(top_buttons[i].hwnd, BUTTONSETASFLATBTN, 0, 0);
                    
                    SendMessage(top_buttons[i].hwnd, BUTTONADDTOOLTIP, (WPARAM) Translate(top_buttons[i].szTooltip), 0);
                }
                SetButtonStyle();
                if(g_CluiData.bSkinnedToolbar)
                    SetTBSKinned(1);
                ConfigureFrame();
                SetButtonStates(hwnd); 
                {
                    MENUITEMINFOA mii;
                    ZeroMemory(&mii, sizeof(mii));
                    mii.cbSize = MENUITEMINFO_V4_SIZE;
                    mii.fMask = MIIM_TYPE | MIIM_DATA;
                    himlMirandaIcon = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 1, 1);
                    ImageList_AddIcon(himlMirandaIcon, LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
                    mii.dwItemData = MENU_MIRANDAMENU;
                    mii.fType = MFT_OWNERDRAW;
                    mii.dwTypeData = NULL;
                    SetMenuItemInfoA(GetMenu(hwnd), 0, TRUE, &mii);
                }
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
                PostMessage(hwnd, M_CREATECLC, 0, 0);
				SFL_Create();
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
                    int nStatus;


                    nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
                    if (nStatus != ID_STATUS_OFFLINE)
                        PostMessage(hwnd, WM_COMMAND, nStatus, 0);
                }
#endif
                return FALSE;
            }
        case M_CREATECLC:
            CreateCLC(hwnd);
            g_clcData = (struct ClcData *)GetWindowLong(hwndContactTree, 0);
            break;

        // Power management
        case WM_POWERBROADCAST:
            {
                switch ((DWORD) wParam) {
                    case PBT_APMSUSPEND:
                // Computer is suspending, disconnect all protocols
                        DisconnectAll();
                        break;

                    case PBT_APMRESUMESUSPEND:
                // Computer is resuming, restore all protocols
                        RestoreMode();
                        break;
                }
            }
            break;

        case WM_SYSCOLORCHANGE:
            SendMessage(hwndContactTree, msg, wParam, lParam);
            SendMessage(hwndStatus, msg, wParam, lParam);
        // XXX: only works with 4.71 with 95, IE4.
            SendMessage(hwndStatus, SB_SETBKCOLOR, 0, GetSysColor(COLOR_3DFACE));
            break;
        case WM_ERASEBKGND:
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

                if(g_shutDown)
                    return 0;
                
                hdc = g_CluiData.hdcBg;

                CopyRect(&rcFrame, &rcClient);
                if(g_CLUISkinnedBkColor)
                    FillRect(hdc, &rcClient, g_CLUISkinnedBkColor);
                
                if(g_CluiData.bClipBorder != 0 || g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME) {
                    int docked = Docking_IsDocked();
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

                FillRect(hdc, &rcClient, g_CluiData.hBrushCLCBk);

                rcFrame.left += (g_CluiData.bCLeft - 1);
                rcFrame.right -= (g_CluiData.bCRight - 1);
                if(!g_CluiData.bSkinnedButtonMode)
                    rcFrame.bottom -= (g_CluiData.bottomOffset);
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

                g_SizingRect = *((RECT *)lParam);
                if(wParam != WMSZ_BOTTOM)
                    szrect->bottom = g_PreSizeRect.bottom;
                if(wParam != WMSZ_RIGHT)
                    szrect->right = g_PreSizeRect.right;
                return TRUE;
            }
        case WM_WINDOWPOSCHANGING:
            {
                WINDOWPOS *wp = (WINDOWPOS *)lParam;
                RECT rcStatus;

                if(wp && wp->flags & SWP_NOSIZE)
                    return FALSE;

                //_DebugPopup(0, "sizing: %d, %d, %d, %d : wp: %d, %d, %d, %d", g_SizingRect.left,
                //            g_SizingRect.top, g_SizingRect.right, g_SizingRect.bottom,
                //            wp->x, wp->y, wp->x + wp->cx, wp->y + wp->cy);
                
                if (g_CluiData.dwFlags & CLUI_FRAME_SBARSHOW) {
                    GetWindowRect(hwndStatus, &rcStatus);
                    g_CluiData.statusBarHeight = (rcStatus.bottom - rcStatus.top);
                } else
                    g_CluiData.statusBarHeight = 0;

                if(hwndContactList != NULL) {
                    RECT rcOld;
                    GetWindowRect(hwnd, &rcOld);
                    if (wp->cx != rcOld.right - rcOld.left || wp->cy != rcOld.bottom - rcOld.top && !(wp->flags & SWP_NOSIZE)) {
                        HDWP PosBatch;
                        int res;

                        during_sizing = 1;
                        new_window_rect.left = 0;
                        new_window_rect.right = wp->cx - (2 * g_CLUI_x_off);
                        new_window_rect.top = 0;
                        new_window_rect.bottom = wp->cy - g_CLUI_y_off - g_CLUI_y1_off;
                        PosBatch = BeginDeferWindowPos(25);
                        SizeFramesByWindowRect(&new_window_rect, &PosBatch);
                        dock_prevent_moving=0;
                        LayoutButtons(hwnd, &PosBatch, &new_window_rect);
                        DeferWindowPos(PosBatch, hwndStatus, 0, 0, new_window_rect.bottom - g_CluiData.statusBarHeight, new_window_rect.right,
                                       g_CluiData.statusBarHeight, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
                        res=EndDeferWindowPos(PosBatch);
                        if(wp->cx != g_oldSize.cx)
                            PostMessage(hwnd, CLUIINTM_STATUSBARUPDATE, 0, 0);
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
            if(wParam == 0 && lParam == 0) {
                RECT rcStatus;
                
                if (IsZoomed(hwnd))
                    ShowWindow(hwnd, SW_SHOWNORMAL);

                if (g_CluiData.dwFlags & CLUI_FRAME_SBARSHOW) {
                    GetWindowRect(hwndStatus, &rcStatus);
                    g_CluiData.statusBarHeight = (rcStatus.bottom - rcStatus.top);
                    SendMessage(hwndStatus, WM_SIZE, 0, 0);
                } else
                    g_CluiData.statusBarHeight = 0;

                if(hwndContactList != NULL) {
                    HDWP PosBatch;
                    int res;
                    RECT rcClient;
                    
                    GetClientRect(hwnd, &rcClient);
                    PosBatch = BeginDeferWindowPos(25);
                    SizeFramesByWindowRect(&rcClient, &PosBatch);
                    LayoutButtons(hwnd, &PosBatch, &rcClient);
                    res=EndDeferWindowPos(PosBatch);
                    if(rcClient.right - rcClient.left != g_oldSize.cx)
                        PostMessage(hwnd, CLUIINTM_STATUSBARUPDATE, 0, 0);
                    g_oldSize.cx = rcClient.right - rcClient.left;
                    g_oldSize.cy = rcClient.bottom - rcClient.top;
                }
            }
            if(g_CluiData.forceResize) {
                g_CluiData.forceResize = FALSE;
                PostMessage(hwnd, CLUIINTM_REDRAW, 0, 0);
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

                if (!CallService(MS_CLIST_DOCKINGISDOCKED, 0, 0)) {
                    cluiPos.bottom = (DWORD) (rc.bottom - rc.top);
                    cluiPos.left = rc.left;
                    cluiPos.top = rc.top;
                }
                cluiPos.right = rc.right - rc.left;
            }
            return TRUE;
        case WM_SETFOCUS:
            SetFocus(hwndContactTree);
            return 0;

        case CLUIINTM_REMOVEFROMTASKBAR:
            RemoveFromTaskBar(hwnd);
            return 0;
        case WM_NCACTIVATE:
            //PostMessage(hwnd, CLUIINTM_REMOVEFROMTASKBAR, 0, 0);
            break;
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
            }
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
                else if(pt.y >= r.top && pt.y <= r.top + 6 && !DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) {
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
                
                if(g_CluiData.forceResize)
                    SendMessage(hwnd, WM_SIZE, 0, 0);
				if(!g_CluiData.fadeinout)
					SFL_SetState();
                if (lParam)
                    break;
                if (noRecurse)
                    break;
				if (!g_CluiData.fadeinout || !IsWinVer2000Plus())
                    break;

                g_fading_active = 1;
                if (wParam) {
                    sourceAlpha = 0;
                    destAlpha = g_CluiData.isTransparent ? g_CluiData.alpha : 255;
                    MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), sourceAlpha, LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
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
                    if (thisTick >= startTick + 200)
                        break;
                    MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), (BYTE) (sourceAlpha + (destAlpha - sourceAlpha) * (int) (thisTick - startTick) / 200), LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
                }
                MySetLayeredWindowAttributes(hwnd, g_CluiData.bFullTransparent ? g_CluiData.colorkey : RGB(0, 0, 0), (BYTE) (destAlpha), LWA_ALPHA | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0));
                g_fading_active = 0;
				SFL_SetState();
                break;
            }
        case WM_MENUSELECT:
            {
                POINT pt;
                int pos;

                if((HMENU)lParam != hMenuMain)
                    break;
                
                GetCursorPos(&pt);
                pos = LOWORD(wParam); //MenuItemFromPoint(hwnd, hMenuMain, pt);
                if ((pos == 0 || pos == 1) && HIWORD(wParam) & MF_POPUP && MenuItemFromPoint(hwnd, hMenuMain, pt) != -1) {
                    MENUITEMINFO mii;
                    
                    ZeroMemory(&mii, sizeof(mii));
                    mii.cbSize = MENUITEMINFO_V4_SIZE;
                    mii.fMask = MIIM_SUBMENU;
                    mii.hSubMenu = (pos == 0) ? (HMENU)CallService(MS_CLIST_MENUGETMAIN, 0, 0) : (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
                    SetMenuItemInfo(hMenuMain, pos, TRUE, &mii);
                }
                break;
            }
        case WM_MENURBUTTONUP:
        /* this API is so badly documented at MSDN!! */ {
                UINT id = 0;

                id = GetMenuItemID((HMENU) lParam, LOWORD(wParam)); /* LOWORD(wParam) contains the menu pos in its parent menu */
                if (id != (-1))
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, 0), 0);
                break;
            }
        case WM_SYSCOMMAND:
            if (wParam == SC_MAXIMIZE)
                return 0;
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_COMMAND:
            {
                DWORD dwOldFlags = g_CluiData.dwFlags;
                if(HIWORD(wParam) == BN_CLICKED && lParam != 0) {
                    switch(LOWORD(wParam)) {
                        case IDC_TBMENU:
                        case IDC_TBTOPMENU:
                        {
                            RECT rc;
                            HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);

                            //HMENU hmenu = GetSubMenu(hMenuMain, 0);
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
                            if (hSoundHook) {
                                UnhookEvent(hSoundHook);
                                hSoundHook = 0;
                            }
                            g_CluiData.soundsOff = !g_CluiData.soundsOff;
                            DBWriteContactSettingByte(NULL, "CLUI", "NoSounds", g_CluiData.soundsOff);
                            if (g_CluiData.soundsOff && hSoundHook == 0)
                                hSoundHook = HookEvent(ME_SKIN_PLAYINGSOUND, ClcSoundHook);
                            DBWriteContactSettingByte(NULL, "Skin", "UseSound", g_CluiData.soundsOff ? 0 : 1);
                            CheckDlgButton(hwndContactList, IDC_TBSOUND, g_CluiData.soundsOff ? BST_UNCHECKED : BST_CHECKED);
                            SetButtonStates(hwnd);
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
                else {
                    if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_MAINMENU), (LPARAM) (HANDLE) NULL))
                        break;
                }
                switch (LOWORD(wParam)) {
                    case ID_TRAY_EXIT:
                    case ID_ICQ_EXIT:
                        g_shutDown = 1;
                        if (CallService(MS_SYSTEM_OKTOEXIT, 0, 0))
                            DestroyWindow(hwnd);
                        break;
                    case IDC_TBMINIMIZE:
                    case ID_TRAY_HIDE:
                        CallService(MS_CLIST_SHOWHIDE, 0, 0);
                        break;
                    case POPUP_NEWGROUP:
                        SendMessage(hwndContactTree, CLM_SETHIDEEMPTYGROUPS, 0, 0);
                        CallService(MS_CLIST_GROUPCREATE, 0, 0);
                        break;
                    case POPUP_HIDEOFFLINE:
                    case IDC_TBHIDEOFFLINE:
                        CallService(MS_CLIST_SETHIDEOFFLINE, (WPARAM) (-1), 0);
                        break;
                    case POPUP_HIDEOFFLINEROOT:
                        SendMessage(hwndContactTree, CLM_SETHIDEOFFLINEROOT, !SendMessage(hwndContactTree, CLM_GETHIDEOFFLINEROOT, 0, 0), 0);
                        break;
                    case POPUP_HIDEEMPTYGROUPS:
                        {
                            int newVal = !(GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_HIDEEMPTYGROUPS);
                            DBWriteContactSettingByte(NULL, "CList", "HideEmptyGroups", (BYTE) newVal);
                            SendMessage(hwndContactTree, CLM_SETHIDEEMPTYGROUPS, newVal, 0);
                            break;
                        }
                    case POPUP_DISABLEGROUPS:
                    case IDC_TBHIDEGROUPS:
                        {
                            int newVal = !(GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_USEGROUPS);
                            DBWriteContactSettingByte(NULL, "CList", "UseGroups", (BYTE) newVal);
                            SendMessage(hwndContactTree, CLM_SETUSEGROUPS, newVal, 0);
                            CheckDlgButton(hwnd, IDC_TBHIDEGROUPS, newVal ? BST_CHECKED : BST_UNCHECKED);
                            break;
                        }
                    case POPUP_HIDEMIRANDA:
                        {
                            CallService(MS_CLIST_SHOWHIDE, 0, 0);
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
                            SendMessage(hwndContactTree, CLM_AUTOREBUILD, 0, 0);
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
                    case POPUP_ADJUSTSIZE:
                        {
                            //AdjustSize(hwnd);
                            break;
                        }
                    case ID_EVENTAREA_AUTOHIDE:
                        {
                            g_CluiData.dwFlags ^= CLUI_FRAME_AUTOHIDENOTIFY;
                            break;
                        }
                    case ID_EVENTAREA_ENABLED:
                        {
                            g_CluiData.dwFlags ^= CLUI_FRAME_USEEVENTAREA;
                            break;
                        }
                    case ID_EVENTAREA_SUNKENFRAME:
                        {
                            g_CluiData.dwFlags ^= CLUI_FRAME_EVENTAREASUNKEN;
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
								SFL_SetState();
							}
							else
								SFL_Destroy();
							DBWriteContactSettingByte(NULL, "CLUI", "FloaterMode", g_CluiData.bUseFloater);
							break;
						}
					case POPUP_FLOATER_AUTOHIDE:
						{
							g_CluiData.bUseFloater ^= CLUI_FLOATER_AUTOHIDE;
							if(g_CluiData.bUseFloater & CLUI_FLOATER_AUTOHIDE)
								SFL_SetState();
							else
								ShowWindow(g_hwndSFL, SW_SHOWNORMAL);
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
                    InvalidateRect(hwndContactTree, NULL, FALSE);
                    DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
                    if ((dwOldFlags & (CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_SHOWBOTTOMBUTTONS | CLUI_FRAME_CLISTSUNKEN)) != (g_CluiData.dwFlags & (CLUI_FRAME_SHOWTOPBUTTONS | CLUI_FRAME_SHOWBOTTOMBUTTONS | CLUI_FRAME_CLISTSUNKEN))) {
                        ConfigureFrame();
                        ConfigureCLUIGeometry();
                    }
                    ConfigureEventArea(hwndContactList);
                    SetButtonStyle();
                    PostMessage(hwndContactList, WM_SIZE, 0, 0);
                    InvalidateRect(hwnd, FALSE, TRUE);
                }
                return FALSE;
            }
        case WM_KEYDOWN:
            CallService(MS_CLIST_MENUPROCESSHOTKEY, wParam, MPCF_MAINMENU | MPCF_CONTACTMENU);
            break;

        case WM_GETMINMAXINFO:
            DefWindowProc(hwnd, msg, wParam, lParam);
            ((LPMINMAXINFO) lParam)->ptMinTrackSize.x = 16 + GetSystemMetrics(SM_CXHTHUMB);
            ((LPMINMAXINFO) lParam)->ptMinTrackSize.y = 16;
            return 0;

        case WM_LBUTTONDOWN:
            {
                if(g_CluiData.dwFlags & CLUI_FRAME_SHOWTOPBUTTONS) {
                    POINT ptMouse, pt;
                    RECT rcClient;

                    GetCursorPos(&ptMouse);
                    pt = ptMouse;
                    ScreenToClient(hwnd, &ptMouse);
                    GetClientRect(hwnd, &rcClient);
                    rcClient.bottom = g_CluiData.topOffset;
                    if(PtInRect(&rcClient, ptMouse))
                        return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
                }
                break;
            }
        case WM_DISPLAYCHANGE:
            SendMessage(hwndContactTree, WM_SIZE, 0, 0);   //forces it to send a cln_listsizechanged
            break;
        case WM_NOTIFY:
            if (((LPNMHDR) lParam)->hwndFrom == hwndContactTree) {
                switch (((LPNMHDR) lParam)->code) {
                    case CLN_EXPANDED:
                        {
                            NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                            CallService(MS_CLIST_GROUPSETEXPANDED, (WPARAM) nmc->hItem, nmc->action);
                            return FALSE;
                        }
                    case CLN_DRAGGING:
                        {
                            NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                            ClientToScreen(hwnd, &nmc->pt);
                            if (!(nmc->flags & CLNF_ISGROUP))
                                if (NotifyEventHooks(hContactDraggingEvent, (WPARAM) nmc->hItem, MAKELPARAM(nmc->pt.x, nmc->pt.y))) {
                                    SetCursor(LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER)));
                                    return TRUE;
                                }
                            break;
                        }
                    case CLN_DRAGSTOP:
                        {
                            NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                            if (!(nmc->flags & CLNF_ISGROUP))
                                NotifyEventHooks(hContactDragStopEvent, (WPARAM) nmc->hItem, 0);
                            break;
                        }
                    case CLN_DROPPED:
                        {
                            NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                            ClientToScreen(hwnd, &nmc->pt);
                            if (!(nmc->flags & CLNF_ISGROUP))
                                if (NotifyEventHooks(hContactDroppedEvent, (WPARAM) nmc->hItem, MAKELPARAM(nmc->pt.x, nmc->pt.y))) {
                                    SetCursor(LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER)));
                                    return TRUE;
                                }
                            break;
                        }
                    case NM_KEYDOWN:
                        {
                            NMKEY *nmkey = (NMKEY *) lParam;
                            return CallService(MS_CLIST_MENUPROCESSHOTKEY, nmkey->nVKey, MPCF_MAINMENU | MPCF_CONTACTMENU);
                        }
                    case CLN_LISTSIZECHANGE:
                        {
                            NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
                            RECT rcTree, rcWorkArea, rcOld;
                            int maxHeight,newHeight;
                            int winstyle;

                            if (disableautoupd)
                                break;

                            if (!DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) 
                                break;

                            if (CallService(MS_CLIST_DOCKINGISDOCKED,0,0)) 
                                break;
                            if (hFrameContactTree == 0)
                                break;
                            
                            maxHeight=DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",75);
                            rcOld=rcWindow;
                            
                            GetWindowRect(hwnd,&rcWindow);
                            GetWindowRect(hwndContactTree,&rcTree);
                            winstyle=GetWindowLong(hwndContactTree,GWL_STYLE);

                            SystemParametersInfo(SPI_GETWORKAREA,0,&rcWorkArea,FALSE);
                            if (nmc->pt.y>(rcWorkArea.bottom-rcWorkArea.top)) {
                                nmc->pt.y=(rcWorkArea.bottom-rcWorkArea.top);
                            }
                            /*
                            if ((nmc->pt.y)==lastreqh) {
                            }
                            lastreqh=nmc->pt.y;*/
                            newHeight=max(nmc->pt.y,3)+1+((winstyle&WS_BORDER)?2:0)+(rcWindow.bottom-rcWindow.top)-(rcTree.bottom-rcTree.top);
                            if (newHeight==(rcWindow.bottom-rcWindow.top)) 
                                break;

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
                            KillTimer(hwnd, TIMERID_AUTOSIZE);
                            SetTimer(hwnd, TIMERID_AUTOSIZE, 100, 0);
                            break;
                        }
                    case NM_CLICK:
                        {
                            NMCLISTCONTROL *nm = (NMCLISTCONTROL *) lParam;
                            DWORD hitFlags;
                            HANDLE hItem;
                            
                            hItem = (HANDLE)SendMessage(hwndContactTree,CLM_HITTEST,(WPARAM)&hitFlags,MAKELPARAM(nm->pt.x,nm->pt.y));

                            if ((hitFlags & (CLCHT_NOWHERE | CLCHT_INLEFTMARGIN | CLCHT_BELOWITEMS)) == 0)
                                break;
                            if (DBGetContactSettingByte(NULL, "CLUI", "ClientAreaDrag", SETTING_CLIENTDRAG_DEFAULT)) {
                                POINT pt;
                                pt = nm->pt;
                                ClientToScreen(hwndContactTree, &pt);
                                return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
                            }
                            break;
                        }
                }
            } else if (((LPNMHDR) lParam)->hwndFrom == hwndStatus) {
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
                            nParts = SendMessage(hwndStatus, SB_GETPARTS, 0, 0);
                            if (nm->dwItemSpec == 0xFFFFFFFE) {
                                nPanel = nParts - 1;
                                SendMessage(hwndStatus, SB_GETRECT, nPanel, (LPARAM) &rc);
                                if (nm->pt.x < rc.left)
                                    return FALSE;
                            } else {
                                nPanel = nm->dwItemSpec;
                            }

                            //if (nParts > 1)
                            //    hMenu = GetSubMenu(hMenu, nPanel);

                            szBuffer[0] = 0;
                            pd = (ProtocolData *)SendMessageA(hwndStatus, SB_GETTEXTA, (WPARAM)nPanel, (LPARAM)szBuffer);
                            if(pd == NULL)
                                break;
                            szBuf = pd->RealName;
                            SendMessage(hwndStatus, SB_GETRECT, nPanel, (LPARAM) &rc);
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
                            ClientToScreen(hwndStatus, &pt);
                            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                        }
                }
            }
            return FALSE;
        case WM_CONTEXTMENU:
            {
                RECT rc;
                POINT pt;

                pt.x = (short) LOWORD(lParam);
                pt.y = (short) HIWORD(lParam);
            // x/y might be -1 if it was generated by a kb click            
                GetWindowRect(hwndContactTree, &rc);
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
                GetWindowRect(hwndStatus, &rc);
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
                    //GetWindowRect(hwndContactTree, &rcClc);
                    GetCursorPos(&pt);
                    ScreenToClient(hwnd, &pt);
                    hMenu = g_CluiData.hMenuButtons;
                    rcHit.bottom = g_CluiData.dwButtonHeight;
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
                            ConfigureCLUIGeometry();
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
                            DBWriteContactSettingByte(NULL, "CLUI", "tb_skinned", g_CluiData.bSkinnedToolbar);
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
                if (dis->hwndItem == hwndStatus) {
                    ProtocolData *pd = (ProtocolData *)dis->itemData;
                    int nParts = SendMessage(hwndStatus, SB_GETPARTS, 0, 0);
                    char *szProto;
                    int status, x;
                    SIZE textSize;
                    BYTE showOpts = DBGetContactSettingByte(NULL, "CLUI", "SBarShow", 1);
                    if(IsBadCodePtr((FARPROC)pd))
                        break;
                    if(g_shutDown)
                        return TRUE;
                    szProto = pd->RealName;
                    status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
                    SetBkMode(dis->hDC, TRANSPARENT);
                    x = dis->rcItem.left;

                    if (showOpts & 1) {
                        HICON hIcon;
                        BYTE xStatusID = 0;
                        BOOL bDestroy = FALSE;
                        
                        if(g_CluiData.bShowXStatusOnSbar && status > ID_STATUS_OFFLINE && DBGetContactSettingByte(NULL, szProto, "XStatusEnabled", 0) && (xStatusID = DBGetContactSettingByte(NULL, szProto, "XStatusId", 0)) > 0) {
                            if(xStatusID > 0 && xStatusID <= 24) {
                                hIcon = ImageList_ExtractIcon(0, himlExtraImages, xStatusID +  3);
                                bDestroy = TRUE;
                            }
                        }
                        else if(status >= ID_STATUS_CONNECTING && status < ID_STATUS_OFFLINE) {
                            if(g_CluiData.IcoLib_Avail) {
                                char szBuffer[128];
                                mir_snprintf(szBuffer, 128, "%s_conn", pd->RealName);
                                hIcon = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)szBuffer);
                            }
                            else
                                hIcon = g_CluiData.hIconConnecting;
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
                        if(bDestroy)
                            DestroyIcon(hIcon);
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
                } else if (dis->CtlType == ODT_MENU) {
                    if (dis->itemData == MENU_MIRANDAMENU) {
                        HICON hIcon = ImageList_GetIcon(himlMirandaIcon, 0, ILD_NORMAL);
                        CLN_DrawMenuItem(dis, hIcon, 0);
                        return TRUE;
                    }
                    return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
                }
                return 0;
            }
        case WM_CLOSE:
            if (DBGetContactSettingByte(NULL, "CList", "ToolWindow", SETTING_TOOLWINDOW_DEFAULT))
                CallService(MS_CLIST_SHOWHIDE, 0, 0);
            else
                SendMessage(hwnd, WM_COMMAND, ID_ICQ_EXIT, 0);

            return FALSE;
        case CLUIINTM_REDRAW:
            RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);   
            return 0;
        case CLUIINTM_STATUSBARUPDATE:
            CluiProtocolStatusChanged(0, 0);
            return 0;
        case WM_DESTROY:
            //RemoveDialogBoxHook();
            if (!IsIconic(hwnd)) {
                RECT rc;
                GetWindowRect(hwnd, &rc);

                if (!CallService(MS_CLIST_DOCKINGISDOCKED, 0, 0)) {
            //if docked, dont remember pos (except for width)
                    DBWriteContactSettingDword(NULL, "CList", "Height", (DWORD) (rc.bottom - rc.top));
                    DBWriteContactSettingDword(NULL, "CList", "x", (DWORD) rc.left);
                    DBWriteContactSettingDword(NULL, "CList", "y", (DWORD) rc.top);
                }
                DBWriteContactSettingDword(NULL, "CList", "Width", (DWORD) (rc.right - rc.left));
            }
            if(g_CluiData.hdcBg) {
                SelectObject(g_CluiData.hdcBg, g_CluiData.hbmBgOld);
                DeleteObject(g_CluiData.hbmBg);
                DeleteDC(g_CluiData.hdcBg);
            }
            if(g_CluiData.bmpBackground) {
                SelectObject(g_CluiData.hdcPic, g_CluiData.hbmPicOld);
                DeleteDC(g_CluiData.hdcPic);
                DeleteObject(g_CluiData.bmpBackground);
            }
            DestroyMenu(g_CluiData.hMenuButtons);
        // Disconnect all protocols
            DisconnectAll();

            ShowWindow(hwnd, SW_HIDE);
            CallService(MS_CLIST_FRAMES_REMOVEFRAME,(WPARAM)hFrameContactTree,(LPARAM)0);
            DestroyWindow(hwndContactTree);
			UnLoadCLUIFramesModule();
            ImageList_Destroy(himlMirandaIcon);
            FreeLibrary(hUserDll);
            free(g_ExtraCache);
            PostQuitMessage(0);
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return TRUE;
}

static int CluiIconsChanged(WPARAM wParam, LPARAM lParam)
{
    ImageList_ReplaceIcon(himlMirandaIcon, 0, LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
    DrawMenuBar(hwndContactList);
    return 0;
}

static HANDLE hRenameMenuItem;

static int MenuItem_PreBuild(WPARAM wParam, LPARAM lParam)
{
	TCHAR cls[128];
	HANDLE hItem;
	HWND hwndClist = GetFocus();
	CLISTMENUITEM mi;

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.flags = CMIM_FLAGS;
	GetClassName(hwndClist, cls, SIZEOF(cls));
	hwndClist = (!lstrcmp(CLISTCONTROL_CLASS, cls)) ? hwndClist : hwndContactList;
	hItem = (HANDLE) SendMessage(hwndClist, CLM_GETSELECTION, 0, 0);
	if (!hItem) {
		mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
	}
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hRenameMenuItem, (LPARAM) & mi);
	return 0;
}

static int MenuItem_RenameContact(WPARAM wParam, LPARAM lParam)
{
	TCHAR cls[128];
	HANDLE hItem;
	HWND hwndClist = GetFocus();
	GetClassName(hwndClist, cls, SIZEOF(cls));
	// worst case scenario, the rename is sent to the main contact list
	hwndClist = (!lstrcmp(CLISTCONTROL_CLASS, cls)) ? hwndClist : hwndContactList;
	hItem = (HANDLE) SendMessage(hwndClist, CLM_GETSELECTION, 0, 0);
	if (hItem) {
		SetFocus(hwndClist);
		SendMessage(hwndClist, CLM_EDITLABEL, (WPARAM) hItem, 0);
	}
	return 0;
}

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000    
#endif

static int MetaChanged(WPARAM wParam, LPARAM lParam)
{
    WindowList_Broadcast(hClcWindowList, INTM_METACHANGEDEVENT, wParam, lParam);
    return 0;
}

static BOOL g_AboutDlgActive = 0;

BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon;
    COLORREF url_visited = RGB(128, 0, 128);
    COLORREF url_unvisited = RGB(0, 0, 255);
    
    switch (msg)
	{
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
#if defined(_UNICODE)
            urlctrl_set(GetDlgItem(hwndDlg, IDC_SUPPORT), _T("http://miranda-im.org/download/details.php?action=viewfile&id=2365"), &url_unvisited, &url_visited, UCF_TXT_HCENTER, RGB(255, 255, 255));
#else            
            urlctrl_set(GetDlgItem(hwndDlg, IDC_SUPPORT), _T("http://miranda-im.org/download/details.php?action=viewfile&id=2189"), &url_unvisited, &url_visited, UCF_TXT_HCENTER, RGB(255, 255, 255));
#endif            
            DestroyIcon(hIcon);
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
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
			{	HFONT hFont;
            
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CLNICER,WM_GETFONT,0,0);
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

int LoadCLUIModule(void)
{
    WNDCLASS wndclass;
    DBVARIANT dbv;
    TCHAR titleText[256];
    BYTE windowStyle;
    
    hUserDll = LoadLibraryA("user32.dll");
    if (hUserDll) {
        MySetLayeredWindowAttributes = (BOOL(WINAPI *)(HWND, COLORREF, BYTE, DWORD))GetProcAddress(hUserDll, "SetLayeredWindowAttributes");
		MyUpdateLayeredWindow = (BOOL (WINAPI *)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD))GetProcAddress(hUserDll, "UpdateLayeredWindow");
    }
    HookEvent(ME_SYSTEM_MODULESLOADED, CluiModulesLoaded);
    HookEvent(ME_SKIN_ICONSCHANGED, CluiIconsChanged);
    HookEvent(ME_OPT_INITIALISE, CluiOptInit);
    HookEvent(ME_MC_DEFAULTTCHANGED, MetaChanged);
    HookEvent(ME_MC_SUBCONTACTSCHANGED, MetaChanged);
    hContactDraggingEvent = CreateHookableEvent(ME_CLUI_CONTACTDRAGGING);
    hContactDroppedEvent = CreateHookableEvent(ME_CLUI_CONTACTDROPPED);
    hContactDragStopEvent = CreateHookableEvent(ME_CLUI_CONTACTDRAGSTOP);

    hStatusBarShowToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
    hStatusBarHideToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_HIDE_TOOLTIP);
    InitGroupMenus();
    LoadCluiServices();
    //InstallDialogBoxHook();
    
    wndclass.style         = ( IsWinVerXPPlus() && DBGetContactSettingByte(NULL,"CList", "WindowShadow",0) == 1 ? CS_DROPSHADOW : 0);
    //wndclass.style = CS_HREDRAW | CS_VREDRAW | (IsWinVerXPPlus() && DBGetContactSettingByte(NULL, "CList", "WindowShadow", 0) == 1 ? CS_DROPSHADOW : 0) ;
    wndclass.lpfnWndProc = ContactListWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = g_hInst;
    wndclass.hIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_CLISTMENU);
    wndclass.lpszClassName = _T(MIRANDACLASS);

    RegisterClass(&wndclass);

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
    
    if (DBGetContactSettingTString(NULL, "CList", "TitleText", &dbv))
        lstrcpyn(titleText, _T(MIRANDANAME), sizeof(titleText));
    else {
        lstrcpyn(titleText, dbv.ptszVal, sizeof(titleText));
        DBFreeVariant(&dbv);
    }
	oldhideoffline=DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT);
    cluiPos.left = DBGetContactSettingDword(NULL, "CList", "x", 600);
    cluiPos.top = DBGetContactSettingDword(NULL, "CList", "y", 200);
    cluiPos.right = DBGetContactSettingDword(NULL, "CList", "Width", 150);
    cluiPos.bottom = DBGetContactSettingDword(NULL, "CList", "Height", 350);

    LoadExtraIconModule();
    windowStyle = DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", SETTING_WINDOWSTYLE_DEFAULT);
	SFL_RegisterWindowClass();
    
    hwndContactList = CreateWindowEx((windowStyle != SETTING_WINDOWSTYLE_DEFAULT ? WS_EX_TOOLWINDOW : 0),
                                     _T(MIRANDACLASS), titleText, 
                                     GetCLUIWindowStyle(windowStyle),
                                     (int) cluiPos.left, (int) cluiPos.top, (int) cluiPos.right, (int) cluiPos.bottom, NULL, NULL, g_hInst, NULL);

    if (DBGetContactSettingByte(NULL, "CList", "OnDesktop", 0)) {
        HWND hProgMan = FindWindowA("Progman", NULL);
        if (IsWindow(hProgMan))
            SetParent(hwndContactList, hProgMan);
    } 

	//laster=GetLastError();
	PreCreateCLC(hwndContactList);
    LoadCLUIFramesModule();
    {
        CLISTMENUITEM mi;
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.flags = 0;
        mi.pszContactOwner = NULL;    //on every contact
        CreateServiceFunction("CList/RenameContactCommand", MenuItem_RenameContact);
        mi.position = 2000050000;
        mi.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_RENAME));
        mi.pszName = Translate("&Rename");
        mi.pszService = "CList/RenameContactCommand";
        hRenameMenuItem = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) &mi);
        HookEvent(ME_CLIST_PREBUILDCONTACTMENU, MenuItem_PreBuild);
    }
    //CreateServiceFunction(MS_CLUI_SHOWMAINMENU,CList_ShowMainMenu);
    CreateServiceFunction("CLN/About",CLN_ShowAbout);
    return 0;
}

static struct {UINT id; char *name;} _tagFSINFO[] = {
    FONTID_STATUS, "Status mode",
    FONTID_FRAMETITLE, "Frame titles",
    FONTID_EVENTAREA, "Event area",
    FONTID_TIMESTAMP, "Contact list local time",
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

UpdateWindowImage()
{
    HDC hdc;
    RECT rc;
    BLENDFUNCTION bf;
    POINT ptDest, ptSrc = {0};
    SIZE szDest;
    HDC screenDC = GetDC(0);
    GetWindowRect(hwndContactList, &rc);
    
    ptDest.x = rc.left;
    ptDest.y = rc.top;
    szDest.cx = rc.right - rc.left;
    szDest.cy = rc.bottom - rc.top;

    if(!g_CluiData.hdcBg) {
        HDC dc = GetDC(hwndContactList);
        g_CluiData.hdcBg = CreateCompatibleDC(dc);
        //g_CluiData.hbmBg = CreateCompatibleBitmap(g_CluiData.hdcBg, rc.right - rc.left, (rc.bottom - rc.top) + g_CluiData.statusBarHeight);
        g_CluiData.hbmBg = CreateCompatibleBitmap(g_CluiData.hdcBg, 2000, 2000 + g_CluiData.statusBarHeight);
        g_CluiData.hbmBgOld = SelectObject(g_CluiData.hdcBg, g_CluiData.hbmBg);
        ReleaseDC(hwndContactList, dc);
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
        ClientToScreen(hwndContactList, &g_CluiData.ptW);
    }
    bf.BlendOp = AC_SRC_OVER;
    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.SourceConstantAlpha = 255;
    SetWindowLong(hwndContactList, GWL_STYLE, GetWindowLong(hwndContactList, GWL_STYLE) & ~WS_EX_LAYERED);
    SetWindowLong(hwndContactList, GWL_STYLE, GetWindowLong(hwndContactList, GWL_STYLE) | WS_EX_LAYERED);
    
    UpdateLayeredWindow(hwndContactList, screenDC, &ptDest, &szDest, hdc, &ptSrc, RGB(224, 224, 224), &bf, ULW_ALPHA);
    ReleaseDC(0, screenDC);
}
