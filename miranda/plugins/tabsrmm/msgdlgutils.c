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

$Id$
*/

/*
 * utility functions for the message dialog window
 */

#include "commonheaders.h"

#pragma hdrstop
#include "../../include/m_clc.h"
#include "../../include/m_clui.h"
#include "../../include/m_userinfo.h"
#include "../../include/m_history.h"
#include "../../include/m_addcontact.h"

#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "m_metacontacts.h"
#include "msgdlgutils.h"
#include "m_smileyadd.h"
#include "m_ieview.h"
#include "functions.h"

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern LOGFONTA logfonts[MSGDLGFONTCOUNT + 2];
extern COLORREF fontcolors[MSGDLGFONTCOUNT + 2];
extern TemplateSet LTR_Active, RTL_Active;
extern DWORD g_gdiplusToken;
extern PAB MyAlphaBlend;

extern HMODULE g_hInst;
extern HANDLE hMessageWindowList;
void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state);

void WriteThemeToINI(const char *szIniFilename, struct MessageWindowData *dat), ReadThemeFromINI(const char *szIniFilename, struct MessageWindowData *dat, int noAdvanced);
int CheckThemeVersion(const char *szIniFilename);

char *GetThemeFileName(int iMode);
void CacheMsgLogIcons(), CacheLogFonts();
void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc);
int MessageWindowOpened(WPARAM wParam, LPARAM LPARAM);
extern BOOL CALLBACK DlgProcTabConfig(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern StatusItems_t StatusItems[];

extern void DrawWithGDIp(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, DWORD srcWidth, DWORD srcHeight, struct avatarCacheEntry *ace, HBITMAP hbm);

struct RTFColorTable rtf_ctable[] = {
    _T("red"), RGB(255, 0, 0), 0, ID_FONT_RED,
    _T("blue"), RGB(0, 0, 255), 0, ID_FONT_BLUE,
    _T("green"), RGB(0, 255, 0), 0, ID_FONT_GREEN,
    _T("magenta"), RGB(255, 0, 255), 0, ID_FONT_MAGENTA,
    _T("yellow"), RGB(255, 255, 0), 0, ID_FONT_YELLOW,
    _T("black"), 0, 0, ID_FONT_BLACK,
    _T("white"), RGB(255, 255, 255), 0, ID_FONT_WHITE,
    NULL, 0, 0, 0
};

/*
 * draw transparent avatar image. Get around crappy image rescaling quality of the
 * AlphaBlend() API.
 */

void MY_AlphaBlend(HDC hdcDraw, DWORD left, DWORD top,  int width, int height, int bmWidth, int bmHeight, HDC hdcMem)
{
    HDC hdcTemp = CreateCompatibleDC(hdcDraw);
    HBITMAP hbmTemp = CreateCompatibleBitmap(hdcMem, bmWidth, bmHeight);
    HBITMAP hbmOld = SelectObject(hdcTemp, hbmTemp);
    BLENDFUNCTION bf = {0};

    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.BlendOp = AC_SRC_OVER;
    
    SetStretchBltMode(hdcTemp, HALFTONE);
    StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcDraw, left, top, width, height, SRCCOPY);
	if(MyAlphaBlend)
		MyAlphaBlend(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, 0, 0, bmWidth, bmHeight, bf);
	else {
		SetStretchBltMode(hdcTemp, HALFTONE);
		StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, 0, 0, bmWidth, bmHeight, SRCCOPY);
	}
    StretchBlt(hdcDraw, left, top, width, height, hdcTemp, 0, 0, bmWidth, bmHeight, SRCCOPY);
    SelectObject(hdcTemp, hbmOld);
    DeleteObject(hbmTemp);
    DeleteDC(hdcTemp);
}

/*
 * calculates avatar layouting, based on splitter position to find the optimal size
 * for the avatar w/o disturbing the toolbar too much.
 */

void CalcDynamicAvatarSize(HWND hwndDlg, struct MessageWindowData *dat, BITMAP *bminfo)
{
    RECT rc;
    double aspect = 0, newWidth = 0, picAspect = 0;
    double picProjectedWidth = 0;
    
    GetClientRect(hwndDlg, &rc);
    
    if(dat->iAvatarDisplayMode != AVATARMODE_DYNAMIC) {
        int showToolbar = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
        RECT rcContainer;
        int cx;
        if(!dat->showPic)           // don't care if no avatar is visible == leave splitter alone...
            return;
        if(dat->dwFlags & MWF_WASBACKGROUNDCREATE || dat->pContainer->dwFlags & CNT_DEFERREDCONFIGURE || dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED || IsIconic(dat->pContainer->hwnd))
            return;                 // at this stage, the layout is not yet ready...

        GetClientRect(dat->pContainer->hwnd, &rcContainer);
        AdjustTabClientRect(dat->pContainer, &rcContainer);
        cx = rc.right;

        if(dat->iRealAvatarHeight == 0) {               // calc first layout paramaters
            if(bminfo->bmWidth == 0 || bminfo->bmHeight == 0)
                picAspect = 1.0;
            else
                picAspect = (double)(bminfo->bmWidth / (double)bminfo->bmHeight);
            if(myGlobals.m_LimitStaticAvatarHeight > 0 && bminfo->bmHeight > myGlobals.m_LimitStaticAvatarHeight)
                dat->iRealAvatarHeight = myGlobals.m_LimitStaticAvatarHeight;
            else
                dat->iRealAvatarHeight = bminfo->bmHeight;

            picProjectedWidth = (double)dat->iRealAvatarHeight * picAspect;
            dat->pic.cx = (int)picProjectedWidth + 2*1;
            SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
        }
        if(cx - dat->pic.cx > dat->iButtonBarNeeds && !myGlobals.m_AlwaysFullToolbarWidth) {
            if(dat->splitterY <= dat->bottomOffset + (showToolbar ? 0 : 26))
                dat->splitterY = dat->bottomOffset + (showToolbar ? 1 : 26);;
            if(dat->splitterY - 26 > dat->bottomOffset)
                dat->pic.cy = dat->splitterY - 27;
            else
                dat->pic.cy = dat->bottomOffset;
        }
        else {
            if(dat->splitterY <= dat->bottomOffset + 26 + (showToolbar ? 0 : 26))
                dat->splitterY = dat->bottomOffset + 26 + (showToolbar ? 0 : 26);;
                if(dat->splitterY - 27 > dat->bottomOffset)
                    dat->pic.cy = dat->splitterY - 28;
                else
                    dat->pic.cy = dat->bottomOffset;
        }
		dat->pic.cy--;
    }
    else if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
        if(dat->dwFlags & MWF_WASBACKGROUNDCREATE || dat->pContainer->dwFlags & CNT_DEFERREDCONFIGURE || dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED || IsIconic(dat->pContainer->hwnd))
            return;                 // at this stage, the layout is not yet ready...
            
        if(bminfo->bmWidth == 0 || bminfo->bmHeight == 0)
            picAspect = 1.0;
        else
            picAspect = (double)(bminfo->bmWidth / (double)bminfo->bmHeight);
        picProjectedWidth = (double)((dat->dynaSplitter + ((dat->showUIElements != 0) ? 28 : 2))) * picAspect;

        if(((rc.right) - (int)picProjectedWidth) > (dat->iButtonBarNeeds) && !myGlobals.m_AlwaysFullToolbarWidth) {
            dat->iRealAvatarHeight = dat->dynaSplitter + ((dat->showUIElements != 0) ? 32 : 6);
            dat->bottomOffset = dat->dynaSplitter + 100;
        }
        else {
            dat->iRealAvatarHeight = dat->dynaSplitter + 6;
            dat->bottomOffset = -33;
        }
        if(bminfo->bmHeight != 0)
            aspect = (double)dat->iRealAvatarHeight / (double)bminfo->bmHeight;
        else
            aspect = 1;
        newWidth = (double)bminfo->bmWidth * aspect;
        if(newWidth > (double)(rc.right) * 0.8)
            newWidth = (double)(rc.right) * 0.8;
        dat->pic.cy = dat->iRealAvatarHeight + 2*1;
        dat->pic.cx = (int)newWidth + 2*1;
    }
    else
        return;
}

int IsMetaContact(HWND hwndDlg, struct MessageWindowData *dat) 
{
    if(dat->hContact == 0 || dat->szProto == NULL)
       return 0;
    
    return (myGlobals.g_MetaContactsAvail && !strcmp(dat->szProto, "MetaContacts"));
}
char *GetCurrentMetaContactProto(HWND hwndDlg, struct MessageWindowData *dat)
{
    dat->hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)dat->hContact, 0);
    if(dat->hSubContact) {
        dat->szMetaProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hSubContact, 0);
        if(dat->szMetaProto)
            dat->wMetaStatus = DBGetContactSettingWord(dat->hSubContact, dat->szMetaProto, "Status", ID_STATUS_OFFLINE);
        else
            dat->wMetaStatus = ID_STATUS_OFFLINE;
        return dat->szMetaProto;
    }
    else
        return dat->szProto;
}

void WriteStatsOnClose(HWND hwndDlg, struct MessageWindowData *dat)
{
    DBEVENTINFO dbei;
    char buffer[450];
    HANDLE hNewEvent;
    time_t now = time(NULL);
    now = now - dat->stats.started;

    return;
    
    if(dat->hContact != 0 && myGlobals.m_LogStatusChanges != 0 && DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0) {
        mir_snprintf(buffer, sizeof(buffer), "Session close - active for: %d:%02d:%02d, Sent: %d (%d), Rcvd: %d (%d)", now / 3600, now / 60, now % 60, dat->stats.iSent, dat->stats.iSentBytes, dat->stats.iReceived, dat->stats.iReceivedBytes);
        dbei.cbSize = sizeof(dbei);
        dbei.pBlob = (PBYTE) buffer;
        dbei.cbBlob = strlen(buffer) + 1;
        dbei.eventType = EVENTTYPE_STATUSCHANGE;
        dbei.flags = DBEF_READ;
        dbei.timestamp = time(NULL);
        dbei.szModule = dat->szProto;
        hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
        if (dat->hDbEventFirst == NULL) {
            dat->hDbEventFirst = hNewEvent;
            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
        }
    }
}

int MsgWindowUpdateMenu(HWND hwndDlg, struct MessageWindowData *dat, HMENU submenu, int menuID)
{
    if(menuID == MENU_LOGMENU) {
        int iLocalTime = dat->dwEventIsShown & MWF_SHOW_USELOCALTIME ? 1 : 0; // DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0);
        int iRtl = (myGlobals.m_RTLDefault == 0 ? DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) : DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1));
        int iLogStatus = (myGlobals.m_LogStatusChanges != 0) && (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0);

        CheckMenuItem(submenu, ID_LOGMENU_SHOWTIMESTAMP, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWTIME ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_USECONTACTSLOCALTIME, MF_BYCOMMAND | iLocalTime ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWDATE, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWDATES ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWSECONDS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWSECONDS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_INDENTMESSAGEBODY, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_INDENT ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGEICONS_SHOWICONS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWICONS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGEICONS_SYMBOLSINSTEADOFICONS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SYMBOLS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGEICONS_USEINCOMING, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_INOUTICONS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWNICKNAME, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWNICK ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_ACTIVATERTL, MF_BYCOMMAND | iRtl ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGITEMSTOSHOW_LOGSTATUSCHANGES, MF_BYCOMMAND | iLogStatus ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_SHOWGRID, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_GRID ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_USEINDIVIDUALBACKGROUNDCOLORS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_INDIVIDUALBKG ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_GROUPMESSAGES, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_GROUPMODE ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_SIMPLETEXTFORMATTING, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_TEXTFORMAT ? MF_CHECKED : MF_UNCHECKED);
        
        EnableMenuItem(submenu, ID_LOGMENU_SHOWDATE, dat->dwFlags & MWF_LOG_SHOWTIME ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(submenu, ID_LOGMENU_SHOWSECONDS, dat->dwFlags & MWF_LOG_SHOWTIME ? MF_ENABLED : MF_GRAYED);
    }
    else if(menuID == MENU_PICMENU) {
        MENUITEMINFOA mii = {0};
        char *szText;
        
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING;
        
        EnableMenuItem(submenu, ID_PICMENU_TOGGLEAVATARDISPLAY, MF_BYCOMMAND | (myGlobals.m_AvatarMode == 2 ? MF_ENABLED : MF_GRAYED));
        CheckMenuItem(submenu, ID_PICMENU_ALWAYSKEEPTHEBUTTONBARATFULLWIDTH, MF_BYCOMMAND | (myGlobals.m_AlwaysFullToolbarWidth ? MF_CHECKED : MF_UNCHECKED));
        if(!(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)) {
            EnableMenuItem(submenu, ID_PICMENU_SETTINGS, MF_BYCOMMAND | (ServiceExists(MS_AV_GETAVATARBITMAP) ? MF_ENABLED : MF_GRAYED));
            szText = Translate("Contact picture settings...");
        }
		else {
			char szServiceName[100];

			mir_snprintf(szServiceName, 100, "%s/SetAvatar", dat->bIsMeta ? dat->szMetaProto : dat->szProto);
			EnableMenuItem(submenu, ID_PICMENU_SETTINGS, MF_BYCOMMAND | (ServiceExists(szServiceName) ? MF_ENABLED : MF_GRAYED));
            szText = Translate("Set your avatar...");
		}
        mii.dwTypeData = szText;
        mii.cch = lstrlenA(szText) + 1;
        SetMenuItemInfoA(submenu, ID_PICMENU_SETTINGS, FALSE, &mii);
    }
    else if(menuID == MENU_PANELPICMENU) {
        CheckMenuItem(submenu, ID_PANELPICMENU_DISABLEAUTOMATICAVATARUPDATES, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0) == 1) ? MF_CHECKED : MF_UNCHECKED);
        EnableMenuItem(submenu, ID_PICMENU_SETTINGS, MF_BYCOMMAND | (ServiceExists(MS_AV_GETAVATARBITMAP) ? MF_ENABLED : MF_GRAYED));
    }
    return 0;
}

int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId)
{
    if(menuId == MENU_PICMENU || menuId == MENU_PANELPICMENU || menuId == MENU_TABCONTEXT) {
        switch(selection) {
            case ID_TABMENU_SWITCHTONEXTTAB:
                SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, (WPARAM) DM_SELECT_NEXT, 0);
                return 1;
            case ID_TABMENU_SWITCHTOPREVIOUSTAB:
                SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, (WPARAM) DM_SELECT_PREV, 0);
                return 1;
            case ID_TABMENU_ATTACHTOCONTAINER:
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SELECTCONTAINER), hwndDlg, SelectContainerDlgProc, (LPARAM) hwndDlg);
                return 1;
            case ID_TABMENU_CONTAINEROPTIONS:
                if(dat->pContainer->hWndOptions == 0)
                    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM) dat->pContainer);
                return 1;
            case ID_TABMENU_CLOSECONTAINER:
                SendMessage(dat->pContainer->hwnd, WM_CLOSE, 0, 0);
                return 1;
            case ID_TABMENU_CLOSETAB:
                SendMessage(hwndDlg, WM_CLOSE, 1, 0);
                return 1;
            case ID_TABMENU_CONFIGURETABAPPEARANCE:
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TABCONFIG), hwndDlg, DlgProcTabConfig, 0);
                return 1;
            case ID_PICMENU_TOGGLEAVATARDISPLAY:
                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", dat->showPic ? 0 : 1);
                ShowPicture(hwndDlg, dat, FALSE);
                SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
                return 1;
            case ID_PICMENU_ALWAYSKEEPTHEBUTTONBARATFULLWIDTH:
                myGlobals.m_AlwaysFullToolbarWidth = !myGlobals.m_AlwaysFullToolbarWidth;
                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "alwaysfulltoolbar", myGlobals.m_AlwaysFullToolbarWidth);
                WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
                break;
            case ID_PICMENU_CHOOSEBACKGROUNDCOLOR:
            case ID_PANELPICMENU_CHOOSEBACKGROUNDCOLOR:
                {
                    CHOOSECOLOR cc = {0};
                    COLORREF custColors[20];
                    int i;
                    
                    for(i = 0; i < 20; i++)
                        custColors[i] = GetSysColor(COLOR_3DFACE);
                    
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = dat->pContainer->hwnd;
                    cc.lpCustColors = custColors;
                    if(ChooseColor(&cc)) {
                        dat->avatarbg = cc.rgbResult;
                        DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "avbg", dat->avatarbg);
                    }
                    break;
                }
            case ID_PICMENU_SETTINGS:
                {
                    char FileName[MAX_PATH];
                    char Filters[512];
                    OPENFILENAMEA ofn={0};

                    CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(Filters),(LPARAM)(char*)Filters);
                    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                    ofn.hwndOwner=0;
                    ofn.lpstrFile = FileName;
                    ofn.lpstrFilter = Filters;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.nMaxFileTitle = MAX_PATH;
                    ofn.Flags=OFN_HIDEREADONLY;
                    ofn.lpstrInitialDir = ".";
                    *FileName = '\0';
                    ofn.lpstrDefExt="";
                    if(menuId == MENU_PANELPICMENU)
                        CallService(MS_AV_CONTACTOPTIONS, (WPARAM)dat->hContact, 0);
                    else if(menuId == MENU_PICMENU) {
                        if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                            if (GetOpenFileNameA(&ofn)) {
                                if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL && menuId == MENU_PICMENU) {
                                    char szServiceName[100], *szProto;

                                    szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                                    mir_snprintf(szServiceName, sizeof(szServiceName), "%s/SetAvatar", szProto);
                                    if(ServiceExists(szServiceName)) {
                                        if(CallProtoService(szProto, "/SetAvatar", 0, (LPARAM)FileName) != 0)
                                            _DebugMessage(hwndDlg, dat, Translate("Failed to set avatar for %s"), szProto);
                                    }
                                }
                                else {
                                    char szFinalPath[MAX_PATH];
                                    CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)FileName, (LPARAM)szFinalPath);
                                    DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File", szFinalPath);
                                }
                            }
                        }
                        else
                            CallService(MS_AV_CONTACTOPTIONS, (WPARAM)dat->hContact, 0);
                    }
                }
                return 1;
        }
    }
    else if(menuId == MENU_LOGMENU) {
        int iLocalTime = dat->dwEventIsShown & MWF_SHOW_USELOCALTIME ? 1 : 0; // DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0);
        int iRtl = (myGlobals.m_RTLDefault == 0 ? DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) : DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1));
        int iLogStatus = (myGlobals.m_LogStatusChanges != 0) && (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0);

        DWORD dwOldFlags = dat->dwFlags;

        switch(selection) {

            case ID_LOGMENU_SHOWTIMESTAMP:
                dat->dwFlags ^= MWF_LOG_SHOWTIME;
                return 1;
            case ID_LOGMENU_USECONTACTSLOCALTIME:
                iLocalTime ^=1;
                dat->dwEventIsShown ^= MWF_SHOW_USELOCALTIME;
                if(dat->hContact) {
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", (BYTE) iLocalTime);
                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                }
                return 1;
            case ID_LOGMENU_INDENTMESSAGEBODY:
                dat->dwFlags ^= MWF_LOG_INDENT;
                return 1;
            case ID_LOGMENU_ACTIVATERTL:
                iRtl ^= 1;
                dat->dwFlags = iRtl ? dat->dwFlags | MWF_LOG_RTL : dat->dwFlags & ~MWF_LOG_RTL;
                if(dat->hContact) {
                    if(iRtl != myGlobals.m_RTLDefault)
                        DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", (BYTE) iRtl);
                    else
                        DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "RTL");
                    SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
                }
                return 1;
            case ID_LOGMENU_SHOWDATE:
                dat->dwFlags ^= MWF_LOG_SHOWDATES;
                return 1;
            case ID_LOGMENU_SHOWSECONDS:
                dat->dwFlags ^= MWF_LOG_SHOWSECONDS;
                return 1;
            case ID_MESSAGEICONS_SHOWICONS:
                dat->dwFlags ^= MWF_LOG_SHOWICONS;
                //dat->dwFlags = dat->dwFlags & MWF_LOG_SHOWICONS ? dat->dwFlags & ~MWF_LOG_SYMBOLS : dat->dwFlags;
                return 1;
            case ID_MESSAGEICONS_SYMBOLSINSTEADOFICONS:
                dat->dwFlags ^= MWF_LOG_SYMBOLS;
                //dat->dwFlags = dat->dwFlags & MWF_LOG_SYMBOLS ? dat->dwFlags & ~MWF_LOG_SHOWICONS : dat->dwFlags;
                return 1;
            case ID_MESSAGEICONS_USEINCOMING:
                dat->dwFlags ^= MWF_LOG_INOUTICONS;
                return 1;
            case ID_LOGMENU_SHOWNICKNAME:
                dat->dwFlags ^= MWF_LOG_SHOWNICK;
                return 1;
            case ID_LOGITEMSTOSHOW_LOGSTATUSCHANGES:
                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", iLogStatus ? 0 : -1);
                return 1;
            case ID_MESSAGELOGFORMATTING_SHOWGRID:
                dat->dwFlags ^= MWF_LOG_GRID;
                if(dat->dwFlags & MWF_LOG_GRID)
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));     // XXX margins in the log (looks slightly better)
                else
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));     // XXX margins in the log (looks slightly better)
                return 1;
            case ID_MESSAGELOGFORMATTING_USEINDIVIDUALBACKGROUNDCOLORS:
                dat->dwFlags ^= MWF_LOG_INDIVIDUALBKG;
                return 1;
            case ID_MESSAGELOGFORMATTING_GROUPMESSAGES:
                dat->dwFlags ^= MWF_LOG_GROUPMODE;
                return 1;
            case ID_MESSAGELOGFORMATTING_SIMPLETEXTFORMATTING:
                dat->dwFlags ^= MWF_LOG_TEXTFORMAT;
                return 1;
            case ID_MESSAGELOG_EXPORTMESSAGELOGSETTINGS:
                {
                    char *szFilename = GetThemeFileName(1);
                    if(szFilename != NULL)
                        WriteThemeToINI(szFilename, dat);
                    return 1;
                }
            case ID_MESSAGELOG_IMPORTMESSAGELOGSETTINGS:
                {
                    char *szFilename = GetThemeFileName(0);
                    if(szFilename != NULL) {
                        ReadThemeFromINI(szFilename, 0, 0);
                        CacheMsgLogIcons();
                        CacheLogFonts();
                        WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
                        WindowList_Broadcast(hMessageWindowList, DM_FORCEDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                    }
                    return 1;
                }
        }
    }
    return 0;
}

/*
 * sets tooltips for the visible status bar icons
 * 1) secure im icon
 * 2) Sound toggle icon
 * 3) mtn status icon
 */

void UpdateStatusBarTooltips(HWND hwndDlg, struct MessageWindowData *dat, int iSecIMStatus)
{
    time_t now = time(NULL);
    now = now - dat->stats.started;

    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
        char szTipText[256];
        
        if(myGlobals.g_SecureIMAvail && iSecIMStatus >= 0) {
            mir_snprintf(szTipText, sizeof(szTipText), Translate("Secure IM is %s"), iSecIMStatus ? Translate("enabled") : Translate("disabled"));
            SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXTA, 2, (LPARAM)szTipText);
        }
        mir_snprintf(szTipText, sizeof(szTipText), Translate("Sounds are %s (click to toggle, SHIFT-click to apply for all containers)"), dat->pContainer->dwFlags & CNT_NOSOUND ? "off" : "on");
        SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXTA, myGlobals.g_SecureIMAvail ? 3 : 2, (LPARAM)szTipText);
    }
}

void UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat)
{
    if (dat->pContainer->hwndStatus && SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0) >= 3) {
        char buf[128];
        int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));

        _snprintf(buf, sizeof(buf), "%s %d/%d", dat->lcID, dat->iOpenJobs, len);
        SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 1 | SBT_NOBORDERS, (LPARAM) buf);
    }
}

void UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
        int iSecIMStatus = 0;
        
        SetSelftypingIcon(hwndDlg, dat, DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)));
        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
        if(myGlobals.g_SecureIMAvail) {
			SendMessage(dat->pContainer->hwndStatus, SB_SETTEXTA, 2 | SBT_NOBORDERS, (LPARAM)"");
            if((iSecIMStatus = CallService("SecureIM/IsContactSecured", (WPARAM)dat->hContact, 0)) != 0)
                SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 2, (LPARAM)myGlobals.g_buttonBarIcons[14]);
            else
                SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 2, (LPARAM)myGlobals.g_buttonBarIcons[15]);
        }
		SendMessage(dat->pContainer->hwndStatus, SB_SETTEXTA, (myGlobals.g_SecureIMAvail ? 3 : 2) | SBT_NOBORDERS, (LPARAM)"");
        SendMessage(dat->pContainer->hwndStatus, SB_SETICON, (myGlobals.g_SecureIMAvail ? 3 : 2), (LPARAM)(dat->pContainer->dwFlags & CNT_NOSOUND ? myGlobals.g_buttonBarIcons[23] : myGlobals.g_buttonBarIcons[22]));
        UpdateReadChars(hwndDlg, dat);
        UpdateStatusBarTooltips(hwndDlg, dat, iSecIMStatus);
    }
}

void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, HICON iIcon)
{
    TCITEM item = {0};
    HICON iOldIcon = dat->hTabIcon;
    
    if(iIcon == (HICON)-1) {    // restore status image
        if(dat->dwFlags & MWF_ERRORSTATE) {
            dat->hTabIcon = myGlobals.g_iconErr;
        }
        else {
            dat->hTabIcon = dat->hTabStatusIcon;
        }
    }
    else
        dat->hTabIcon = iIcon;
    item.iImage = 0;
    item.mask = TCIF_IMAGE;
    TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
}

/*
 * retrieve the visiblity of the avatar window, depending on the global setting
 * and local mode
 */

int GetAvatarVisibility(HWND hwndDlg, struct MessageWindowData *dat)
{
    BYTE bAvatarMode = myGlobals.m_AvatarMode;
	BYTE bOwnAvatarMode = myGlobals.m_OwnAvatarMode;

	// infopanel visible, consider own avatar display

	dat->showPic = 0;

	if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
		if(bOwnAvatarMode)
			dat->showPic = FALSE;
		else
			dat->showPic = (dat->hOwnPic && dat->hOwnPic != myGlobals.g_hbmUnknown) ? 1 : 0;

		switch(bAvatarMode) {
			case 0:
			case 1:
				dat->showInfoPic = 1;
				break;
			case 4:
				dat->showInfoPic = 0;
				break;
			case 3:
			{
				HBITMAP hbm = ((dat->ace && !(dat->ace->dwFlags & AVS_HIDEONCLIST)) ? dat->ace->hbmPic : 0);
				if(hbm && hbm != myGlobals.g_hbmUnknown)
					dat->showInfoPic = 1;
				else
					dat->showInfoPic = 0;
				break;
			}
			case 2:
	            dat->showInfoPic = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", 0);
		        break;
		}
	}
	else {
		dat->showInfoPic = 0;
		
		switch(bAvatarMode) {
			case 0:             // globally on
				dat->showPic = 1;
				break;
			case 4:             // globally OFF
				dat->showPic = 0;
				break;
			case 3:             // on, if present
			{
				HBITMAP hbm = (dat->ace && !(dat->ace->dwFlags & AVS_HIDEONCLIST)) ? dat->ace->hbmPic : 0;
				if(hbm && hbm != myGlobals.g_hbmUnknown)
					dat->showPic = 1;
				else
					dat->showPic = 0;
				break;
			}
			case 1:             // on for protocols with avatar support
				{
					int pCaps;

					if(dat->szProto) {
						pCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);
						if((pCaps & PF4_AVATARS) && (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown)) {
							dat->showPic = 1;
						}
					}
					break;
				}
			case 2:             // default (per contact, as it always was
				dat->showPic = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", 0);
				break;
		}
	}
    return dat->showPic;
}

void SetSelftypingIcon(HWND dlg, struct MessageWindowData *dat, int iMode)
{
    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == dlg) {
        char szTipText[64];
        int nParts = SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0);

        SendMessage(dat->pContainer->hwndStatus, SB_SETTEXTA, (nParts - 1) | SBT_NOBORDERS, (LPARAM)"");
		if(iMode)
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, (nParts - 1), (LPARAM)myGlobals.g_buttonBarIcons[12]);
        else
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, (nParts - 1), (LPARAM)myGlobals.g_buttonBarIcons[13]);

        mir_snprintf(szTipText, sizeof(szTipText), Translate("Sending typing notifications is: %s"), iMode ? "Enabled" : "Disabled");
        SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXTA, myGlobals.g_SecureIMAvail ? 4 : 3, (LPARAM)szTipText);
        InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
    }
}

/*
 * checks, if there is a valid smileypack installed for the given protocol
 */

int CheckValidSmileyPack(char *szProto, HICON *hButtonIcon)
{
    SMADD_INFO smainfo;

    if(myGlobals.g_SmileyAddAvail) {
        smainfo.cbSize = sizeof(smainfo);
        smainfo.Protocolname = szProto;
        CallService(MS_SMILEYADD_GETINFO2, 0, (LPARAM)&smainfo);
		if(hButtonIcon)
			*hButtonIcon = smainfo.ButtonIcon;
        return smainfo.NumberOfVisibleSmileys;
    }
    else
        return 0;
}

/*
 * return value MUST be free()'d by caller.
 */

TCHAR *QuoteText(TCHAR *text,int charsPerLine,int removeExistingQuotes)
{
    int inChar,outChar,lineChar;
    int justDoneLineBreak,bufSize;
    TCHAR *strout;

#ifdef _UNICODE
    bufSize = lstrlenW(text) + 23;
#else
    bufSize=strlen(text)+23;
#endif
    strout=(TCHAR*)malloc(bufSize * sizeof(TCHAR));
    inChar=0;
    justDoneLineBreak=1;
    for (outChar=0,lineChar=0;text[inChar];) {
        if (outChar>=bufSize-8) {
            bufSize+=20;
            strout=(TCHAR *)realloc(strout, bufSize * sizeof(TCHAR));
        }
        if (justDoneLineBreak && text[inChar]!='\r' && text[inChar]!='\n') {
            if (removeExistingQuotes)
                if (text[inChar]=='>') {
                    while (text[++inChar]!='\n');
                    inChar++;
                    continue;
                }
            strout[outChar++]='>'; strout[outChar++]=' ';
            lineChar=2;
        }
        if (lineChar==charsPerLine && text[inChar]!='\r' && text[inChar]!='\n') {
            int decreasedBy;
            for (decreasedBy=0;lineChar>10;lineChar--,inChar--,outChar--,decreasedBy++)
                if (strout[outChar]==' ' || strout[outChar]=='\t' || strout[outChar]=='-') break;
            if (lineChar<=10) {
                lineChar+=decreasedBy; inChar+=decreasedBy; outChar+=decreasedBy;
            } else inChar++;
            strout[outChar++]='\r'; strout[outChar++]='\n';
            justDoneLineBreak=1;
            continue;
        }
        strout[outChar++]=text[inChar];
        lineChar++;
        if (text[inChar]=='\n' || text[inChar]=='\r') {
            if (text[inChar]=='\r' && text[inChar+1]!='\n')
                strout[outChar++]='\n';
            justDoneLineBreak=1;
            lineChar=0;
        } else justDoneLineBreak=0;
        inChar++;
    }
    strout[outChar++]='\r';
    strout[outChar++]='\n';
    strout[outChar]=0;
    return strout;
}


void AdjustBottomAvatarDisplay(HWND hwndDlg, struct MessageWindowData *dat)
{
    HBITMAP hbm = dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown);
    
    if(dat->iAvatarDisplayMode != AVATARMODE_DYNAMIC)
        dat->iRealAvatarHeight = 0;
    if(hbm) {
        dat->showPic = GetAvatarVisibility(hwndDlg, dat);
        if(dat->dynaSplitter == 0 || dat->splitterY == 0)
            LoadSplitter(hwndDlg, dat);
        dat->dynaSplitter = dat->splitterY - 34;
        if(dat->iAvatarDisplayMode != AVATARMODE_DYNAMIC) {
            BITMAP bm;
            GetObject(hbm, sizeof(bm), &bm);
            CalcDynamicAvatarSize(hwndDlg, dat, &bm);
        }
        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
        SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
    }
    else {
        dat->showPic = GetAvatarVisibility(hwndDlg, dat);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
        dat->pic.cy = dat->pic.cx = 60;
        InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
    }
}

void ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL showNewPic)
{
    DBVARIANT dbv = {0};
    RECT rc;
    
    if(!(dat->dwEventIsShown & MWF_SHOW_INFOPANEL))
        dat->pic.cy = dat->pic.cx = 60;
    
    if (showNewPic) {
        if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELPIC), NULL, FALSE);
            return;
        }
        AdjustBottomAvatarDisplay(hwndDlg, dat);
    } else {
        dat->showPic = dat->showPic ? 0 : 1;
        DBWriteContactSettingByte(dat->hContact,SRMSGMOD_T,"MOD_ShowPic",(BYTE)dat->showPic);
        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
    }

    GetWindowRect(GetDlgItem(hwndDlg,IDC_CONTACTPIC),&rc);
    if (dat->minEditBoxSize.cy+3>dat->splitterY)
        SendMessage(hwndDlg,DM_SPLITTERMOVED,(WPARAM)rc.bottom-dat->minEditBoxSize.cy,(LPARAM)GetDlgItem(hwndDlg,IDC_SPLITTER));
    if (!showNewPic)
        SetDialogToType(hwndDlg);
    else
        SendMessage(hwndDlg,WM_SIZE,0,0);

    SendMessage(hwndDlg, DM_CALCMINHEIGHT, 0, 0);
    SendMessage(dat->pContainer->hwnd, DM_REPORTMINHEIGHT, (WPARAM) hwndDlg, (LPARAM) dat->uMinHeight);
}

void FlashOnClist(HWND hwndDlg, struct MessageWindowData *dat, HANDLE hEvent, DBEVENTINFO *dbei)
{
    CLISTEVENT cle;
    //char toolTip[256];

    dat->dwTickLastEvent = GetTickCount();
    if((GetForegroundWindow() != dat->pContainer->hwnd || dat->pContainer->hwndActive != hwndDlg) && !(dbei->flags & DBEF_SENT) && dbei->eventType == EVENTTYPE_MESSAGE) {
        UpdateTrayMenu(dat, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->szStatus, dat->hContact, FALSE);
        if(nen_options.bTraySupport == TRUE && myGlobals.m_WinVerMajor >= 5)
            return;
    }
    
    if(hEvent == 0)
        return;

    if(!myGlobals.m_FlashOnClist)
        return;
    if((GetForegroundWindow() != dat->pContainer->hwnd || dat->pContainer->hwndActive != hwndDlg) && !(dbei->flags & DBEF_SENT) && dbei->eventType == EVENTTYPE_MESSAGE && !(dat->dwEventIsShown & MWF_SHOW_FLASHCLIST)) {
        ZeroMemory(&cle, sizeof(cle));
        cle.cbSize = sizeof(cle);
        cle.hContact = (HANDLE) dat->hContact;
        cle.hDbEvent = hEvent;
        cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        cle.pszService = "SRMsg/ReadMessage";
        //_snprintf(toolTip, sizeof(toolTip), Translate("Message from %s"), dat->szNickname);
        //cle.pszTooltip = toolTip;
        CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
        dat->dwEventIsShown |= MWF_SHOW_FLASHCLIST;
        dat->hFlashingEvent = hEvent;
    }
}

static DWORD CALLBACK Message_StreamCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	static DWORD dwRead;
    char ** ppText = (char **) dwCookie;

	if (*ppText == NULL)
	{
		*ppText = malloc(cb + 2);
		CopyMemory(*ppText, pbBuff, cb);
		*pcb = cb;
		dwRead = cb;
        *(*ppText + cb) = '\0';
	}
	else
	{
		char  *p = realloc(*ppText, dwRead + cb + 2);
		//memcpy(p, *ppText, dwRead);
		CopyMemory(p+dwRead, pbBuff, cb);
		//free(*ppText);
		*ppText = p;
		*pcb = cb;
		dwRead += cb;
        *(*ppText + dwRead) = '\0';
	}

    return 0;
}

char * Message_GetFromStream(HWND hwndRtf, struct MessageWindowData* dat, DWORD dwPassedFlags)
{
	EDITSTREAM stream;
	char *pszText = NULL;
    DWORD dwFlags = 0;

	if(hwndRtf == 0 || dat == 0)
		return NULL;

	ZeroMemory(&stream, sizeof(stream));
	stream.pfnCallback = Message_StreamCallback;
	stream.dwCookie = (DWORD) &pszText; // pass pointer to pointer
#if defined(_UNICODE)
    if(dwPassedFlags == 0)
        dwFlags = (CP_UTF8 << 16) | (SF_RTFNOOBJS|SFF_PLAINRTF|SF_USECODEPAGE);
    else
        dwFlags = (CP_UTF8 << 16) | dwPassedFlags;
#else
    if(dwPassedFlags == 0)
        dwFlags = SF_RTF|SF_RTFNOOBJS|SFF_PLAINRTF;
    else
        dwFlags = dwPassedFlags;
#endif    
	SendMessage(hwndRtf, EM_STREAMOUT, (WPARAM)dwFlags, (LPARAM) & stream);
	
	return pszText; // pszText contains the text
}

static void CreateColorMap(TCHAR *Text)
{
	TCHAR * pszText = Text;
	TCHAR * p1;
	TCHAR * p2;
	TCHAR * pEnd;
	int iIndex = 1, i = 0;
    COLORREF default_color;
    
//	static const char* lpszFmt = "/red%[^ \x5b,]/red%[^ \x5b,]/red%[^ \x5b,]";
	static const TCHAR *lpszFmt = _T("\\red%[^ \x5b\\]\\green%[^ \x5b\\]\\blue%[^ \x5b;];");
	TCHAR szRed[10], szGreen[10], szBlue[10];

	//pszText = _tcsdup(Text);

	p1 = _tcsstr(pszText, _T("\\colortbl"));
	if(!p1)
		return;

	pEnd = _tcschr(p1, '}');

	p2 = _tcsstr(p1, _T("\\red"));

    while(rtf_ctable[i].szName != NULL)
        rtf_ctable[i++].index = 0;
    
    default_color = (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, "Font16Col", 0);
    
    while(p2 && p2 < pEnd)
	{
		if( _stscanf(p2, lpszFmt, &szRed, &szGreen, &szBlue) > 0 )
		{
			int i;
			for (i = 0;;i ++) {
                if(rtf_ctable[i].szName == NULL)
                    break;
//                if(rtf_ctable[i].clr == default_color)
//                    continue;
				if(rtf_ctable[i].clr == RGB(_ttoi(szRed), _ttoi(szGreen), _ttoi(szBlue)))
					rtf_ctable[i].index = iIndex;
			}
		}
		iIndex++;
		p1 = p2;
		p1 ++;

		p2 = _tcsstr(p1, _T("\\red"));
	}

	//free(pszText);

	return ;
}

int RTFColorToIndex(int iCol)
{
    int i = 0;
    while(rtf_ctable[i].szName != NULL) {
        if(rtf_ctable[i].index == iCol)
            return i + 1;
        i++;
    }
    return 0;
}

BOOL DoRtfToTags(TCHAR * pszText, struct MessageWindowData *dat)
{
	TCHAR * p1;
	//int * pIndex;
	BOOL bJustRemovedRTF = TRUE;
	BOOL bTextHasStarted = FALSE;
    LOGFONTA lf;
    COLORREF color;
    static inColor = 0;
    
	if(!pszText)
		return FALSE;

    /*
     * used to filter out attributes which are already set for the default message input area
     * font
     */

    //lf = logfonts[MSGFONTID_MESSAGEAREA];
    //color = fontcolors[MSGFONTID_MESSAGEAREA];

    lf = dat->theme.logFonts[MSGFONTID_MESSAGEAREA];
    color = dat->theme.fontColors[MSGFONTID_MESSAGEAREA];
    
	// create an index of colors in the module and map them to
	// corresponding colors in the RTF color table

    /*
	pIndex = malloc(sizeof(int) * MM_FindModule(dat->pszModule)->nColorCount);
	for(i = 0; i < MM_FindModule(dat->pszModule)->nColorCount ; i++)
		pIndex[i] = -1;*/
	CreateColorMap(pszText);
    
	// scan the file for rtf commands and remove or parse them
    inColor = 0;
	p1 = _tcsstr(pszText, _T("\\pard"));
	if(p1)
	{
		int iRemoveChars;
		TCHAR InsertThis[50];
		p1 += 5;

		MoveMemory(pszText, p1, (_tcslen(p1) +1) * sizeof(TCHAR));
		p1 = pszText;
		// iterate through all characters, if rtf control character found then take action
		while(*p1 != (TCHAR)'\0')
		{
			_sntprintf(InsertThis, 50, _T(""));
			iRemoveChars = 0;

			switch (*p1)
			{
			case (TCHAR)'\\':
				if(p1 == _tcsstr(p1, _T("\\cf"))) // foreground color
				{
					TCHAR szTemp[20];
					int iCol = _ttoi(p1 + 3);
					int iInd = RTFColorToIndex(iCol);
					bJustRemovedRTF = TRUE;

                    _sntprintf(szTemp, 20, _T("%d"), iCol);
					iRemoveChars = 3 + _tcslen(szTemp);
					if(bTextHasStarted || iCol)
                        _sntprintf(InsertThis, sizeof(InsertThis) / sizeof(TCHAR), ( iInd > 0 ) ? (inColor ? _T("[/color][color=%s]") : _T("[color=%s]")) : (inColor ? _T("[/color]") : _T("")), rtf_ctable[iInd  - 1].szName);
                    inColor = iInd > 0 ? 1 : 0;
				}
				else if(p1 == _tcsstr(p1, _T("\\highlight"))) //background color
				{
                    TCHAR szTemp[20];
					int iCol = _ttoi(p1 + 10);
					//int iInd = RTFColorToIndex(pIndex, iCol, dat);
					bJustRemovedRTF = TRUE;

                    _sntprintf(szTemp, 20, _T("%d"), iCol);
					iRemoveChars = 10 + _tcslen(szTemp);
					//if(bTextHasStarted || iInd >= 0)
					//	_snprintf(InsertThis, sizeof(InsertThis), ( iInd >= 0 ) ? _T("%%f%02u") : _T("%%F"), iInd);
				}
				else if(p1 == _tcsstr(p1, _T("\\par"))) // newline
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = 4;
					//_sntprintf(InsertThis, sizeof(InsertThis), _T("\n"));
				}
                else if(p1 == _tcsstr(p1, _T("\\line")))    // soft line break;
                {
                    bTextHasStarted = TRUE;
                    bJustRemovedRTF = TRUE;
                    iRemoveChars = 5;
                    _sntprintf(InsertThis, safe_sizeof(InsertThis), _T("\n"));
                }
				else if(p1 == _tcsstr(p1, _T("\\b"))) //bold
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = (p1[2] != (TCHAR)'0')?2:3;
                    if(!(lf.lfWeight == FW_BOLD)) {          // only allow bold if the font itself isn't a bold one, otherwise just strip it..
                        if(dat->SendFormat == SENDFORMAT_BBCODE)
                            _sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("[b]") : _T("[/b]"));
                        else
                            _sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("*") : _T("*"));
                    }

				}
				else if(p1 == _tcsstr(p1, _T("\\i"))) // italics
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = (p1[2] != (TCHAR)'0')?2:3;
                    if(!lf.lfItalic) {                       // same as for bold
                        if(dat->SendFormat == SENDFORMAT_BBCODE)
                            _sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("[i]") : _T("[/i]"));
                        else
                            _sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("/") : _T("/"));
                    }

				}
				else if(p1 == _tcsstr(p1, _T("\\ul"))) // underlined
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					if(p1[3] == (TCHAR)'n')
						iRemoveChars = 7;
					else if(p1[3] == (TCHAR)'0')
						iRemoveChars = 4;
					else
						iRemoveChars = 3;
                    if(!lf.lfUnderline)  {                   // same as for bold
                        if(dat->SendFormat == SENDFORMAT_BBCODE)
                            _sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[3] != (TCHAR)'0' && p1[3] != (TCHAR)'n') ? _T("[u]") : _T("[/u]"));
                        else
                            _sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[3] != (TCHAR)'0' && p1[3] != (TCHAR)'n') ? _T("_") : _T("_"));
                    }

				}
				else if(p1 == _tcsstr(p1, _T("\\tab"))) // tab
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = 4;
					_sntprintf(InsertThis, safe_sizeof(InsertThis), _T(" "));

				}
				else if(p1[1] == (TCHAR)'\\' || p1[1] == (TCHAR)'{' || p1[1] == (TCHAR)'}' ) // escaped characters
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = 2;
					_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), p1[1]);

				}
				else if(p1[1] == (TCHAR)'\'' ) // special character
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = FALSE;
					if(p1[2] != (TCHAR)' ' && p1[2] != (TCHAR)'\\')
					{
						int iLame = 0;
						TCHAR * p3;
                        TCHAR *stoppedHere;
                        
						if(p1[3] != (TCHAR)' ' && p1[3] != (TCHAR)'\\')
						{
							_tcsncpy(InsertThis, p1 + 2, 3);
							iRemoveChars = 4;
                            InsertThis[2] = 0;
						}
						else
						{
							_tcsncpy(InsertThis, p1 + 2, 2);
							iRemoveChars = 3;
                            InsertThis[2] = 0;
						}
						// convert string containing char in hex format to int.
						// i'm sure there is a better way than this lame stuff that came out of me
                        
                        p3 = InsertThis;
                        iLame = _tcstol(p3, &stoppedHere, 16);
                        /*
						while (*p3)
						{
							if(*p3 == (TCHAR)'a')
								iLame += 10 * (int)pow(16, _tcslen(p3) -1);
							else if(*p3 == (TCHAR)'b')
								iLame += 11 * (int)pow(16, _tcslen(p3) -1);
							else if(*p3 == (TCHAR)'c')
								iLame += 12 * (int)pow(16, _tcslen(p3) -1);
							else if(*p3 == (TCHAR)'d')
								iLame += 13 * (int)pow(16, _tcslen(p3) -1);
							else if(*p3 == (TCHAR)'e')
								iLame += 14 * (int)pow(16, _tcslen(p3) -1);
							else if(*p3 == (TCHAR)'f')
								iLame += 15 * (int)pow(16, _tcslen(p3) -1);
							else
								iLame += (*p3 - 48) * (int)pow(16, _tcslen(p3) -1);
							p3++;
						}
                        */
						_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), (TCHAR)iLame);
                        
					}
					else
						iRemoveChars = 2;
				}
				else // remove unknown RTF command
				{
					int j = 1;
					bJustRemovedRTF = TRUE;
                    while(!_tcschr(_T(" !$%()#*"), p1[j]) && p1[j] != (TCHAR)'§' && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
//                    while(!_tcschr(_T(" !§$%&()#*"), p1[j]) && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
                          j++;
//					while(p1[j] != (TCHAR)' ' && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
//						j++;
					iRemoveChars = j;
				}
				break;

			case (TCHAR)'{': // other RTF control characters
			case (TCHAR)'}':
				iRemoveChars = 1;
				break;

            /*
			case (TCHAR)'%': // escape chat -> protocol control character
				bTextHasStarted = TRUE;
				bJustRemovedRTF = FALSE;
				iRemoveChars = 1;
				_sntprintf(InsertThis, sizeof(InsertThis), _T("%%%%"));
				break;
            */
			case (TCHAR)' ': // remove spaces following a RTF command
				if(bJustRemovedRTF)
					iRemoveChars = 1;
				bJustRemovedRTF = FALSE;
				bTextHasStarted = TRUE;
				break;

			default: // other text that should not be touched
				bTextHasStarted = TRUE;
				bJustRemovedRTF = FALSE;

				break;
			}

			// move the memory and paste in new commands instead of the old RTF
			if(_tcslen(InsertThis) || iRemoveChars)
			{
				MoveMemory(p1 + _tcslen(InsertThis) , p1 + iRemoveChars, (_tcslen(p1) - iRemoveChars +1) * sizeof(TCHAR));
				CopyMemory(p1, InsertThis, _tcslen(InsertThis) * sizeof(TCHAR));
				p1 += _tcslen(InsertThis);
			}
			else
				p1++;
		}


	}
	else {
		return FALSE;
	}
	return TRUE;
}

/*
 * trims the output from DoRtfToTags(), removes trailing newlines and whitespaces...
 */

void DoTrimMessage(TCHAR *msg)
{
    int iLen = _tcslen(msg) - 1;
    int i = iLen;

    while(i && (msg[i] == '\r' || msg[i] == '\n') || msg[i] == ' ') {
        i--;
    }
    if(i < iLen)
        msg[i+1] = '\0';
}

/*
 * saves message to the input history.
 * its using streamout in UTF8 format - no unicode "issues" and all RTF formatting is saved aswell,
 * so restoring a message from the input history will also restore its formatting
 */

void SaveInputHistory(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
    int iLength = 0, iStreamLength = 0;
    int oldTop = 0;
    char *szFromStream = NULL;

    if(wParam) {
        oldTop = dat->iHistoryTop;
        dat->iHistoryTop = (int)wParam;
    }

    szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS|SFF_PLAINRTF|SF_USECODEPAGE));

    iLength = iStreamLength = (lstrlenA(szFromStream) + 1);

    if(iLength > 0 && dat->history != NULL) {
        if((dat->iHistoryTop == dat->iHistorySize) && oldTop == 0) {          // shift the stack down...
            struct InputHistory ihTemp = dat->history[0];
            dat->iHistoryTop--;
            MoveMemory((void *)&dat->history[0], (void *)&dat->history[1], (dat->iHistorySize - 1) * sizeof(struct InputHistory));
            dat->history[dat->iHistoryTop] = ihTemp;
        }
        if(iLength > dat->history[dat->iHistoryTop].lLen) {
            if(dat->history[dat->iHistoryTop].szText == NULL) {
                if(iLength < HISTORY_INITIAL_ALLOCSIZE)
                    iLength = HISTORY_INITIAL_ALLOCSIZE;
                dat->history[dat->iHistoryTop].szText = (TCHAR *)malloc(iLength);
                dat->history[dat->iHistoryTop].lLen = iLength;
            }
            else {
                if(iLength > dat->history[dat->iHistoryTop].lLen) {
                    dat->history[dat->iHistoryTop].szText = (TCHAR *)realloc(dat->history[dat->iHistoryTop].szText, iLength);
                    dat->history[dat->iHistoryTop].lLen = iLength;
                }
            }
        }
        CopyMemory(dat->history[dat->iHistoryTop].szText, szFromStream, iStreamLength);
        if(!oldTop) {
            if(dat->iHistoryTop < dat->iHistorySize) {
                dat->iHistoryTop++;
                dat->iHistoryCurrent = dat->iHistoryTop;
            }
        }
    }
    if(szFromStream)
        free(szFromStream);

    if(oldTop)
        dat->iHistoryTop = oldTop;
}

/*
 * retrieve both buddys and my own UIN for a message session and store them in the message window *dat
 * respects metacontacts and uses the current protocol if the contact is a MC
 */

void GetContactUIN(HWND hwndDlg, struct MessageWindowData *dat)
{
    CONTACTINFO ci;

    ZeroMemory((void *)&ci, sizeof(ci));

    if(dat->bIsMeta) {
        ci.hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)dat->hContact, 0);
        ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ci.hContact, 0);
    }
    else {
        ci.hContact = dat->hContact;
        ci.szProto = dat->szProto;
    }
    ci.cbSize = sizeof(ci);
    ci.dwFlag = CNF_UNIQUEID;
    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
        switch (ci.type) {
            case CNFT_ASCIIZ:
                mir_snprintf(dat->uin, sizeof(dat->uin), "%s", ci.pszVal);
                mir_free(ci.pszVal);
                break;
            case CNFT_DWORD:
                mir_snprintf(dat->uin, sizeof(dat->uin), "%u", ci.dVal);
                break;
        }
    }
    else
        dat->uin[0] = 0;

    // get own uin...
    ci.hContact = 0;
    
    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
        switch (ci.type) {
            case CNFT_ASCIIZ:
                mir_snprintf(dat->myUin, sizeof(dat->myUin), "%s", ci.pszVal);
                mir_free(ci.pszVal);
                break;
            case CNFT_DWORD:
                mir_snprintf(dat->myUin, sizeof(dat->myUin), "%u", ci.dVal);
                break;
        }
    }
    else
        dat->myUin[0] = 0;
    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
        SetDlgItemTextA(hwndDlg, IDC_PANELUIN, dat->uin);
}

unsigned int GetIEViewMode(HWND hwndDlg, struct MessageWindowData *dat)
{
    int iWantIEView = 0;

    iWantIEView = (myGlobals.g_WantIEView) || (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 1 && ServiceExists(MS_IEVIEW_WINDOW));
    iWantIEView = (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == (BYTE)-1) ? 0 : iWantIEView;

    return iWantIEView;
}
void SetMessageLog(HWND hwndDlg, struct MessageWindowData *dat)
{
    unsigned int iWantIEView = GetIEViewMode(hwndDlg, dat);

    if (iWantIEView && dat->hwndLog == 0) {
        IEVIEWWINDOW ieWindow;
        ieWindow.cbSize = sizeof(IEVIEWWINDOW);
        ieWindow.iType = IEW_CREATE;
        ieWindow.dwFlags = 0;
        ieWindow.dwMode = IEWM_TABSRMM;
        ieWindow.parent = hwndDlg;
        ieWindow.x = 0;
        ieWindow.y = 0;
        ieWindow.cx = 10;
        ieWindow.cy = 10;
        CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
        dat->hwndLog = ieWindow.hwnd;
        ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
    }
    else if(!iWantIEView) {
        if(dat->hwndLog) {
            IEVIEWWINDOW ieWindow;
            ieWindow.cbSize = sizeof(IEVIEWWINDOW);
            ieWindow.iType = IEW_DESTROY;
            ieWindow.hwnd = dat->hwndLog;
            CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
        }
        ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_SHOW);
        EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), TRUE);
        dat->hwndLog = 0;
    }
}

void SwitchMessageLog(HWND hwndDlg, struct MessageWindowData *dat, int iMode)
{
    if(iMode) {            // switch from rtf to IEview
        SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
        EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
        SetMessageLog(hwndDlg, dat);
    }
    else                      // switch from IEView to rtf
        SetMessageLog(hwndDlg, dat);
    SetDialogToType(hwndDlg);
    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    UpdateContainerMenu(hwndDlg, dat);
}

void FindFirstEvent(HWND hwndDlg, struct MessageWindowData *dat)
{
    int historyMode = DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_LOADHISTORY, -1);
    if(historyMode == -1) 
        historyMode = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY);

    dat->hDbEventFirst = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) dat->hContact, 0);
    switch (historyMode) {
        case LOADHISTORY_COUNT:
            {
                int i;
                HANDLE hPrevEvent;
                DBEVENTINFO dbei = { 0};
                dbei.cbSize = sizeof(dbei);
                for (i = DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT); i > 0; i--) {
                    if (dat->hDbEventFirst == NULL)
                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
                    else
                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
                    if (hPrevEvent == NULL)
                        break;
                    dbei.cbBlob = 0;
                    dat->hDbEventFirst = hPrevEvent;
                    CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
                    if (!DbEventIsShown(dat, &dbei))
                        i++;
                }
                break;
            }
        case LOADHISTORY_TIME:
            {
                HANDLE hPrevEvent;
                DBEVENTINFO dbei = { 0};
                DWORD firstTime;

                dbei.cbSize = sizeof(dbei);
                if (dat->hDbEventFirst == NULL)
                    dbei.timestamp = time(NULL);
                else
                    CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
                firstTime = dbei.timestamp - 60 * DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME);
                for (;;) {
                    if (dat->hDbEventFirst == NULL)
                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
                    else
                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
                    if (hPrevEvent == NULL)
                        break;
                    dbei.cbBlob = 0;
                    CallService(MS_DB_EVENT_GET, (WPARAM) hPrevEvent, (LPARAM) & dbei);
                    if (dbei.timestamp < firstTime)
                        break;
                    dat->hDbEventFirst = hPrevEvent;
                }
                break;
            }
        default:
            {
                break;
            }
    }
}

void SaveSplitter(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat->panelHeight < 110 && dat->panelHeight > 50) {           // only save valid panel splitter positions
        if(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE)
            DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "panelheight", dat->panelHeight);
        else {
            myGlobals.m_panelHeight = dat->panelHeight;
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "panelheight", dat->panelHeight);
        }
    }

    if(dat->splitterY < MINSPLITTERY || dat->splitterY < 0)
        return;             // do not save "invalid" splitter values
        
    if(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE)
        DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "splitsplity", dat->splitterY);
    else
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", dat->splitterY);
}

void LoadSplitter(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(!(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE))
        dat->splitterY = (int) DBGetContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", (DWORD) 60);
    else
        dat->splitterY = (int) DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "splitsplity", DBGetContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", (DWORD) 60));

    if(dat->splitterY < MINSPLITTERY || dat->splitterY < 0)
        dat->splitterY = 150;
}

void LoadPanelHeight(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(!(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE))
        dat->panelHeight = myGlobals.m_panelHeight;
    else
        dat->panelHeight = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "panelheight", myGlobals.m_panelHeight);

    if(dat->panelHeight <= 0 || dat->panelHeight > 120)
        dat->panelHeight = 52;
}

void PlayIncomingSound(struct ContainerWindowData *pContainer, HWND hwnd)
{
    int iPlay = 0;
    DWORD dwFlags = pContainer->dwFlags;

    if(nen_options.iNoSounds)
        return;
    
	if(nen_options.bSimpleMode == 0) {
		if(dwFlags & CNT_NOSOUND)
			iPlay = FALSE;
		else if(dwFlags & CNT_SYNCSOUNDS) {
			iPlay = !MessageWindowOpened(0, (LPARAM)hwnd);
		}
		else
			iPlay = TRUE;
	}
	else 
		iPlay = TRUE;
    if (iPlay) {
        if(GetForegroundWindow() == pContainer->hwnd && pContainer->hwndActive == hwnd)
            SkinPlaySound("RecvMsgActive");
        else 
            SkinPlaySound("RecvMsgInactive");
    }
}

/*
 * configures the sidebar (if enabled) when tab changes
 */

void ConfigureSideBar(HWND hwndDlg, struct MessageWindowData *dat)
{
    return;
	if(!dat->pContainer->dwFlags & CNT_SIDEBAR)
        return;
    CheckDlgButton(dat->pContainer->hwnd, IDC_SBAR_TOGGLEFORMAT, dat->SendFormat != 0 ? BST_CHECKED : BST_UNCHECKED);
}

/*
 * reads send format and configures the toolbar buttons
 * if mode == 0, int only configures the buttons and does not change send format
 */

void GetSendFormat(HWND hwndDlg, struct MessageWindowData *dat, int mode)
{
    UINT controls[4] = {IDC_FONTBOLD, IDC_FONTITALIC, IDC_FONTUNDERLINE, IDC_FONTFACE};
    int i;
    
    if(mode) {
        dat->SendFormat = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", myGlobals.m_SendFormat);
        if(dat->SendFormat == -1)           // per contact override to disable it..
            dat->SendFormat = 0;
    }
    for(i = 0; i < 4; i++) {
        EnableWindow(GetDlgItem(hwndDlg, controls[i]), dat->SendFormat != 0 ? TRUE : FALSE);
        ShowWindow(GetDlgItem(hwndDlg, controls[i]), (dat->SendFormat != 0 && !(dat->pContainer->dwFlags & CNT_HIDETOOLBAR)) ? SW_SHOW : SW_HIDE);
    }
    return;
}

/*
 * get 2-digit locale id from current keyboard layout
 */

void GetLocaleID(struct MessageWindowData *dat, char *szKLName)
{
    char szLI[20], *stopped = NULL;
    USHORT langID;

    langID = (USHORT)strtol(szKLName, &stopped, 16);
    dat->lcid = MAKELCID(langID, 0);
    GetLocaleInfoA(dat->lcid, LOCALE_SISO639LANGNAME , szLI, 10);
    dat->lcID[0] = toupper(szLI[0]);
    dat->lcID[1] = toupper(szLI[1]);
    dat->lcID[2] = 0;
}

// Returns true if the unicode buffer only contains 7-bit characters.
BOOL IsUnicodeAscii(const wchar_t* pBuffer, int nSize)
{
	BOOL bResult = TRUE;
	int nIndex;

	for (nIndex = 0; nIndex < nSize; nIndex++) {
		if (pBuffer[nIndex] > 0x7F) {
			bResult = FALSE;
			break;
		}
	}
	return bResult;
}

BYTE GetInfoPanelSetting(HWND hwndDlg, struct MessageWindowData *dat)
{
    BYTE bDefault = dat->pContainer->dwFlags & CNT_INFOPANEL ? 1 : 0;
    BYTE bContact = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "infopanel", 0);
    if(dat->hContact == 0)      // no info panel, if no hcontact
        return 0;
    return bContact == 0 ? bDefault : (bContact == (BYTE)-1 ? 0 : 1);
}

void GetDataDir()
{
    char pszDBPath[MAX_PATH + 1], pszDataPath[MAX_PATH + 1];
    
    CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)pszDBPath);
    mir_snprintf(pszDataPath, MAX_PATH, "%s\\tabSRMM\\", pszDBPath);
    CreateDirectoryA(pszDataPath, NULL);
    strncpy(myGlobals.szDataPath, pszDataPath, MAX_PATH);
    myGlobals.szDataPath[MAX_PATH] = 0;
}

void LoadOwnAvatar(HWND hwndDlg, struct MessageWindowData *dat)
{
	AVATARCACHEENTRY *ace = NULL;
	
	if(ServiceExists(MS_AV_GETMYAVATAR))
		ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETMYAVATAR, 0, (LPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto));

	if(ace)
		dat->hOwnPic = ace->hbmPic;
	else
		dat->hOwnPic = myGlobals.g_hbmUnknown;

    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
        BITMAP bm;
        
        dat->iRealAvatarHeight = 0;
        AdjustBottomAvatarDisplay(hwndDlg, dat);
        GetObject(dat->hOwnPic, sizeof(bm), &bm);
        CalcDynamicAvatarSize(hwndDlg, dat, &bm);
        SendMessage(hwndDlg, WM_SIZE, 0, 0);
    }
}

void UpdateApparentModeDisplay(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat->wApparentMode == ID_STATUS_OFFLINE) {
        CheckDlgButton(hwndDlg, IDC_APPARENTMODE, BST_CHECKED);
        SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadSkinnedIcon(SKINICON_STATUS_OFFLINE));
    }
    else if(dat->wApparentMode == ID_STATUS_ONLINE || dat->wApparentMode == 0) {
        CheckDlgButton(hwndDlg, IDC_APPARENTMODE, BST_UNCHECKED);
        SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BM_SETIMAGE, IMAGE_ICON, (LPARAM)(dat->wApparentMode == ID_STATUS_ONLINE ? LoadSkinnedIcon(SKINICON_STATUS_INVISIBLE) : LoadSkinnedIcon(SKINICON_STATUS_ONLINE)));
		if(!dat->pContainer->bSkinned)
			SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN, 0, (LPARAM)(dat->wApparentMode == ID_STATUS_ONLINE ? 1 : 0));
    }
}


int MY_DBFreeVariant(DBVARIANT *dbv)
{
    return DBFreeVariant(dbv);
}

int MY_DBGetContactSettingTString(HANDLE hContact, char *szModule, char *szSetting, DBVARIANT *dbv)
{
    return(DBGetContactSettingTString(hContact, szModule, szSetting, dbv));
}

// free() the return value

TCHAR *MY_DBGetContactSettingString(HANDLE hContact, char *szModule, char *szSetting)
{
    DBVARIANT dbv;
    TCHAR *szResult = NULL;
    if(!DBGetContactSetting(hContact, szModule, szSetting, &dbv)) {
        if(dbv.type == DBVT_ASCIIZ && lstrlenA(dbv.pszVal) > 0) {
#if defined(_UNICODE)
            szResult = Utf8_Decode(dbv.pszVal);
#else
            szResult = malloc(lstrlenA(dbv.pszVal) + 1);
            strcpy(szResult, dbv.pszVal);
#endif
        }
        DBFreeVariant(&dbv);
        return szResult;
    }
    return NULL;
}

void LoadTimeZone(HWND hwndDlg, struct MessageWindowData *dat)
{
    dat->timezone = (DWORD)(DBGetContactSettingByte(dat->hContact,"UserInfo","Timezone", DBGetContactSettingByte(dat->hContact, dat->szProto,"Timezone",-1)));
    if(dat->timezone != -1) {
        DWORD contact_gmt_diff;
        contact_gmt_diff = dat->timezone > 128 ? 256 - dat->timezone : 0 - dat->timezone;
        dat->timediff = (int)myGlobals.local_gmt_diff - (int)contact_gmt_diff*60*60/2;
    }
}

/*
 * paste contents of the clipboard into the message input area and send it immediately
 */

void HandlePasteAndSend(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(!myGlobals.m_PasteAndSend) {
        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_MESSAGE, (LPARAM)TranslateT("The 'paste and send' feature is disabled. You can enable it on the 'General' options page in the 'Sending Messages' section"));
        return;                                     // feature disabled
    }
    
    SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_PASTESPECIAL, CF_TEXT, 0);
    if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
        SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
}

/*
 * draw various elements of the message window, like avatar(s), info panel fields
 * and the color formatting menu
 */

int MsgWindowDrawHandler(WPARAM wParam, LPARAM lParam, HWND hwndDlg, struct MessageWindowData *dat)
{
    LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

    if(!dat)
        return 0;
    if(dat->dwFlags & MWF_INITMODE)
        return 0;

    /*if(dis->CtlType == ODT_BUTTON && dis->CtlID == IDC_TOGGLESIDEBAR) {
        HICON hIcon;
        DWORD bStyle = 0;
		if(dat->pContainer->bSkinned)
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
		else {
			if(dat->pContainer->dwFlags & CNT_SIDEBAR)
				bStyle = DFCS_PUSHED;
			else
				bStyle = DFCS_FLAT;
			DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | bStyle);
		}
        hIcon = myGlobals.g_buttonBarIcons[25];
        DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                  (dis->rcItem.right + dis->rcItem.left - myGlobals.m_smcxicon) / 2,
                  (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2,
                  myGlobals.m_smcxicon, myGlobals.m_smcyicon,
                  DST_ICON | DSS_NORMAL);
        return TRUE;
    } else */
	if (dis->CtlType == ODT_MENU && dis->hwndItem == (HWND)GetSubMenu(myGlobals.g_hMenuContext, 7)) {
        RECT rc = { 0 };
        HBRUSH old, col;
        COLORREF clr;
        switch(dis->itemID) {
            case ID_FONT_RED:
                clr = RGB(255, 0, 0);
                break;
            case ID_FONT_BLUE:
                clr = RGB(0, 0, 255);
                break;
            case ID_FONT_GREEN:
                clr = RGB(0, 255, 0);
                break;
            case ID_FONT_MAGENTA:
                clr = RGB(255, 0, 255);
                break;
            case ID_FONT_YELLOW:
                clr = RGB(255, 255, 0);
                break;
            case ID_FONT_WHITE:
                clr = RGB(255, 255, 255);
                break;
            case ID_FONT_DEFAULTCOLOR:
                clr = GetSysColor(COLOR_MENU);
                break;
            default:
                clr = 0;
        }
        col = CreateSolidBrush(clr);
        old = SelectObject(dis->hDC, col);
        rc.left = 2; rc.top = dis->rcItem.top - 5;
        rc.right = 18; rc.bottom = dis->rcItem.bottom + 4;
        Rectangle(dis->hDC, rc.left - 1, rc.top - 1, rc.right + 1, rc.bottom + 1);
        FillRect(dis->hDC, &rc, col);
        SelectObject(dis->hDC, old);
        DeleteObject(col);
        return TRUE;
    } else if ((dis->hwndItem == GetDlgItem(hwndDlg, IDC_CONTACTPIC) && (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown) && dat->showPic) || (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELPIC) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL && (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown))) {
        HBRUSH hOldBrush;
        BITMAP bminfo;
        double dAspect = 0, dNewWidth = 0, dNewHeight = 0;
        DWORD iMaxHeight = 0, top, cx, cy;
        RECT rc, rcClient, rcFrame;
        HDC hdcDraw;
        HBITMAP hbmDraw, hbmOld;
        BOOL bPanelPic = dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELPIC);
        DWORD aceFlags = 0;
        HBRUSH bgBrush = 0;
        BYTE borderType = myGlobals.bAvatarBoderType;
		HPEN hPenBorder = CreatePen(PS_SOLID, 1, (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", RGB(0, 0, 0))), hPenOld = 0;
		HRGN clipRgn = 0;

        if(bPanelPic) {
            GetObject(dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown, sizeof(bminfo), &bminfo);
			if(dat->ace && dat->showInfoPic && !(dat->ace->dwFlags & AVS_HIDEONCLIST))
                aceFlags = dat->ace->dwFlags;
			else {
				if(dat->panelWidth) {
					dat->panelWidth = 0;
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
				}
				return TRUE;
			}
        }
        else  {
            if(!(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)) {
                if(dat->ace)
                    aceFlags = dat->ace->dwFlags;
            }
            GetObject(dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown), sizeof(bminfo), &bminfo);
        }
    
        GetClientRect(hwndDlg, &rc);
        GetClientRect(dis->hwndItem, &rcClient);
        cx = rcClient.right;
        cy = rcClient.bottom;
        if(cx < 5 || cy < 5)
            return TRUE;
        hdcDraw = CreateCompatibleDC(dis->hDC);
        hbmDraw = CreateCompatibleBitmap(dis->hDC, cx, cy);
        hbmOld = SelectObject(hdcDraw, hbmDraw);
        
		hPenOld = SelectObject(hdcDraw, hPenBorder);

        if(bPanelPic) {
            if(bminfo.bmHeight > bminfo.bmWidth) {
                if(bminfo.bmHeight > 0)
                    dAspect = (double)(cy - 2) / (double)bminfo.bmHeight;
                else
                    dAspect = 1.0;
                dNewWidth = (double)bminfo.bmWidth * dAspect;
                dNewHeight = cy - 2;
            }
            else {
                if(bminfo.bmWidth > 0)
                    dAspect = (double)(cy - 2) / (double)bminfo.bmWidth;
                else
                    dAspect = 1.0;
                dNewHeight = (double)bminfo.bmHeight * dAspect;
                dNewWidth = cy - 2;
            }
			if(dat->panelWidth == -1) {
				dat->panelWidth = (int)dNewWidth + 2;
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
			}
        }
        else {
            if(bminfo.bmHeight > 0)
                dAspect = (double)dat->iRealAvatarHeight / (double)bminfo.bmHeight;
            else
                dAspect = 1.0;
            dNewWidth = (double)bminfo.bmWidth * dAspect;
            if(dNewWidth > (double)(rc.right) * 0.8)
                dNewWidth = (double)(rc.right) * 0.8;
            iMaxHeight = dat->iRealAvatarHeight;
        }
        bgBrush = CreateSolidBrush(dat->avatarbg);
        hOldBrush = SelectObject(hdcDraw, bgBrush);
		rcFrame = rcClient;
		if(bPanelPic)
			rcFrame.left = rcClient.right - dat->panelWidth;
        if(dat->pContainer->bSkinned)
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &rcFrame, hdcDraw);
		else
			FillRect(hdcDraw, &rcFrame, bgBrush);

		if(borderType == 1)
			borderType = aceFlags & AVS_PREMULTIPLIED ? 2 : 3;

		if(!bPanelPic) {
            if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
				if(borderType) {
					RECT rcEdge = {0, 0, dat->pic.cx, dat->pic.cy};
					if(borderType == 2)
						DrawEdge(hdcDraw, &rcEdge, BDR_SUNKENINNER, BF_RECT);
					else if(borderType == 3)
	                    Rectangle(hdcDraw, 0, 0, dat->pic.cx, dat->pic.cy);
					else if(borderType == 4) {
						clipRgn = CreateRoundRectRgn(0, 0, dat->pic.cx, dat->pic.cy, 4, 4);
						SelectClipRgn(hdcDraw, clipRgn);
					}
				}
            }
            else {
                top = (dat->pic.cy - dat->iRealAvatarHeight) / 2;
                if(borderType) {
                    RECT rcEdge = {0, top - 1, dat->pic.cx, top + dat->iRealAvatarHeight + 1};
					if(borderType == 2)
						DrawEdge(hdcDraw, &rcEdge, BDR_SUNKENINNER, BF_RECT);
					else if(borderType == 3)
						Rectangle(hdcDraw, rcEdge.left, rcEdge.top, rcEdge.right, rcEdge.bottom);
					else if(borderType == 4) {
						clipRgn = CreateRoundRectRgn(rcEdge.left, rcEdge.top, rcEdge.right, rcEdge.bottom, 4, 4);
						SelectClipRgn(hdcDraw, clipRgn);
					}
                }
            }
        }
        if(((dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown)) && dat->showPic) || bPanelPic) {
            HDC hdcMem = CreateCompatibleDC(dis->hDC);
            HBITMAP hbmAvatar = bPanelPic ? (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown) : (dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown));
            HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, hbmAvatar);
            if(bPanelPic) {
				LONG width_off = borderType ? 0 : 2;

                rcFrame = rcClient;
                rcFrame.left = rcFrame.right - ((LONG)dNewWidth + 2);
                rcFrame.bottom = rcFrame.top + (LONG)dNewHeight + 2;
                SetStretchBltMode(hdcDraw, HALFTONE);
                if(aceFlags & AVS_PREMULTIPLIED) {
					if(borderType == 2)
						DrawEdge(hdcDraw, &rcFrame, BDR_SUNKENINNER, BF_RECT);
					else if(borderType == 3)
						Rectangle(hdcDraw, rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom);
					else if(borderType == 4) {
						clipRgn = CreateRoundRectRgn(rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom, 4, 4);
						SelectClipRgn(hdcDraw, clipRgn);
					}
					MY_AlphaBlend(hdcDraw, rcFrame.left + (borderType ? 1 : 0), borderType ? 1 : 0, (int)dNewWidth + width_off, (int)dNewHeight + width_off, bminfo.bmWidth, bminfo.bmHeight, hdcMem);
                }
                else {
					if(borderType == 2)
						DrawEdge(hdcDraw, &rcFrame, BDR_SUNKENINNER, BF_RECT);
					else if(borderType == 3)
						Rectangle(hdcDraw, rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom);
					else if(borderType == 4) {
						clipRgn = CreateRoundRectRgn(rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom, 4, 4);
						SelectClipRgn(hdcDraw, clipRgn);
					}
                    StretchBlt(hdcDraw, rcFrame.left + (borderType ? 1 : 0), borderType ? 1 : 0, (int)dNewWidth + width_off, (int)dNewHeight + width_off, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                }
            }
            else {
				LONG width_off = borderType ? 0 : 2;
				LONG xy_off = borderType ? 1 : 0;

                SetStretchBltMode(hdcDraw, HALFTONE);
                if(aceFlags & AVS_PREMULTIPLIED) {
                    if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                        MY_AlphaBlend(hdcDraw, xy_off, xy_off, (int)dNewWidth + width_off, iMaxHeight + width_off, bminfo.bmWidth, bminfo.bmHeight, hdcMem);
                    else
						MY_AlphaBlend(hdcDraw, xy_off, top - (borderType ? 0 : 1), (int)dNewWidth + width_off, iMaxHeight + width_off, bminfo.bmWidth, bminfo.bmHeight, hdcMem);
                }
                else {
                    if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                        StretchBlt(hdcDraw, xy_off, xy_off, (int)dNewWidth + width_off, iMaxHeight + width_off, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                    else
						StretchBlt(hdcDraw, xy_off, top - (borderType ? 0 : 1), (int)dNewWidth + width_off, iMaxHeight + width_off, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                }
            }
			if(clipRgn) {
				HBRUSH hbr = CreateSolidBrush((COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", RGB(0, 0, 0)));
				FrameRgn(hdcDraw, clipRgn, hbr, 1, 1);
				DeleteObject(hbr);
				DeleteObject(clipRgn);
			}
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
        }
        SelectObject(hdcDraw, hPenOld);
        SelectObject(hdcDraw, hOldBrush);
        DeleteObject(bgBrush);
        DeleteObject(hPenBorder);
        BitBlt(dis->hDC, 0, 0, cx, cy, hdcDraw, 0, 0, SRCCOPY);
        SelectObject(hdcDraw, hbmOld);
        DeleteObject(hbmDraw);
        DeleteDC(hdcDraw);
        return TRUE;
    }
    else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELSTATUS) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
        char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
        SIZE sProto = {0}, sStatus = {0};
        DWORD oldPanelStatusCX = dat->panelStatusCX;
        RECT rc;
        HFONT hOldFont = 0;
        BOOL config = myGlobals.ipConfig.isValid;
        
        if(config)
            hOldFont = SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_STATUS]);

        if(dat->szStatus[0])
            GetTextExtentPoint32A(dis->hDC, dat->szStatus, lstrlenA(dat->szStatus), &sStatus);
        if(szProto) {
            if(config)
                SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_PROTO]);
            GetTextExtentPoint32A(dis->hDC, szProto, lstrlenA(szProto), &sProto);
        }
    
        dat->panelStatusCX = sStatus.cx + sProto.cx + 14;
        
        if(dat->panelStatusCX != oldPanelStatusCX)
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
    
        GetClientRect(dis->hwndItem, &rc);
		if(dat->pContainer->bSkinned) {
			StatusItems_t *item = &StatusItems[ID_EXTBKINFOPANEL];
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &rc, dis->hDC);
			rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
			rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
			DrawAlpha(dis->hDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
					  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
		}
		else
			FillRect(dis->hDC, &rc, myGlobals.ipConfig.bkgBrush);
		SetBkMode(dis->hDC, TRANSPARENT);
        if(myGlobals.ipConfig.borderStyle < IPFIELD_FLAT && !dat->pContainer->bSkinned)
            DrawEdge(dis->hDC, &rc, myGlobals.ipConfig.edgeType, myGlobals.ipConfig.edgeFlags);
        
		rc.left += 3;
        if(dat->szStatus[0]) {
            if(config) {
                SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_STATUS]);
                SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_STATUS]);
            }
            DrawTextA(dis->hDC, dat->szStatus, lstrlenA(dat->szStatus), &rc, DT_SINGLELINE | DT_VCENTER);
        }
        if(szProto) {
            rc.left = rc.right - sProto.cx - 3;
            if(config) {
                SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_PROTO]);
                SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_PROTO]);
            }
            else
                SetTextColor(dis->hDC, GetSysColor(COLOR_HOTLIGHT));
            DrawTextA(dis->hDC, szProto, lstrlenA(szProto), &rc, DT_SINGLELINE | DT_VCENTER);
        }
        if(config && hOldFont)
            SelectObject(dis->hDC, hOldFont);
        return TRUE;
    }
    else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELNICK) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
        RECT rc = dis->rcItem;
		TCHAR *szStatusMsg = NULL;

		if(ServiceExists("CList/GetContactStatusMsg")) {
			szStatusMsg = (TCHAR *)CallService("CList/GetContactStatusMsg", (WPARAM)dat->hContact, 0);
			if(szStatusMsg == NULL)
				szStatusMsg = dat->statusMsg;
		}
		else
			szStatusMsg = dat->statusMsg;

		GetTextExtentPoint32A(dis->hDC, "Name:", 5, &dat->szLabel);
        rc.right = rc.left + dat->szLabel.cx + 3;
        SetBkMode(dis->hDC, TRANSPARENT);
		if(dat->pContainer->bSkinned) {
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
			SetTextColor(dis->hDC, myGlobals.skinDefaultFontColor);
		}
		else {
			FillRect(dis->hDC, &rc, GetSysColorBrush(COLOR_3DFACE));
			SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
		}
        DrawTextA(dis->hDC, "Name:", 5, &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
        dis->rcItem.left += (dat->szLabel.cx + 3);
		if(dat->pContainer->bSkinned) {
			StatusItems_t *item = &StatusItems[ID_EXTBKINFOPANEL];
			RECT rc = dis->rcItem;
			rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
			rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
			DrawAlpha(dis->hDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
					  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
		}
		else
			FillRect(dis->hDC, &dis->rcItem, myGlobals.ipConfig.bkgBrush);
        if(myGlobals.ipConfig.borderStyle < IPFIELD_FLAT && !dat->pContainer->bSkinned)
            DrawEdge(dis->hDC, &dis->rcItem, myGlobals.ipConfig.edgeType, myGlobals.ipConfig.edgeFlags);
        dis->rcItem.left +=2;
        if(dat->szNickname[0]) {
            HFONT hOldFont = 0;
            
            if(dat->xStatus > 0 && dat->xStatus <= 32) {
				char szServiceName[128];
				HICON xIcon;

				mir_snprintf(szServiceName, 128, "%s/GetXStatusIcon", dat->bIsMeta ? dat->szMetaProto : dat->szProto);

				if(ServiceExists(szServiceName) && ((xIcon = (HICON)CallProtoService(dat->bIsMeta ? dat->szMetaProto : dat->szProto, "/GetXStatusIcon", dat->xStatus, 0)) != 0)) {
					DrawIconEx(dis->hDC, dis->rcItem.left, (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2, xIcon, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
					DestroyIcon(xIcon);
					dis->rcItem.left += 21;
				}
            }
            if(myGlobals.ipConfig.isValid) {
                hOldFont = SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_NICK]);
                SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_NICK]);
            }
            DrawText(dis->hDC, dat->szNickname, -1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX);
			if(szStatusMsg && szStatusMsg[0]) {
				SIZE szNick;
				GetTextExtentPoint32(dis->hDC, dat->szNickname, lstrlen(dat->szNickname), &szNick);
				if(myGlobals.ipConfig.isValid) {
					SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_STATUS]);
					SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_STATUS]);
				}
				dis->rcItem.left += szNick.cx;
				if(dis->rcItem.left + 30 < dis->rcItem.right) {
					RECT rc = dis->rcItem;
					DrawText(dis->hDC, _T(" - "), -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_CALCRECT);
					DrawText(dis->hDC, _T(" - "), -1, &rc, DT_SINGLELINE | DT_VCENTER);
					dis->rcItem.left += (rc.right - rc.left);
					DrawText(dis->hDC, szStatusMsg, -1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
				}
			}
            if(hOldFont)
                SelectObject(dis->hDC, hOldFont);
        }
        return TRUE;
    }
    else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELUIN) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
        char szBuf[256];
        BOOL config = myGlobals.ipConfig.isValid;
        HFONT hOldFont = 0;
        RECT rc = dis->rcItem;

		if(dat->pContainer->bSkinned) {
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
			SetTextColor(dis->hDC, myGlobals.skinDefaultFontColor);
		}
		else {
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
			SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
		}

		SetBkMode(dis->hDC, TRANSPARENT);
        DrawTextA(dis->hDC, "UIN:", 4, &dis->rcItem, DT_SINGLELINE | DT_VCENTER);

		rc.right = rc.left + dat->szLabel.cx + 3;
        dis->rcItem.left += (dat->szLabel.cx + 3);
		if(dat->pContainer->bSkinned) {
			StatusItems_t *item = &StatusItems[ID_EXTBKINFOPANEL];
			RECT rc = dis->rcItem;
			rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
			rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
			DrawAlpha(dis->hDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
					  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
		}
		else
			FillRect(dis->hDC, &dis->rcItem, myGlobals.ipConfig.bkgBrush);
        if(myGlobals.ipConfig.borderStyle < IPFIELD_FLAT && !dat->pContainer->bSkinned)
            DrawEdge(dis->hDC, &dis->rcItem, myGlobals.ipConfig.edgeType, myGlobals.ipConfig.edgeFlags);
        dis->rcItem.left +=2;
        if(config) {
            hOldFont = SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_UIN]);
            SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_UIN]);
        }
        if(dat->uin[0]) {
            SIZE sUIN, sTime;
            if(dat->idle) {
                time_t diff = time(NULL) - dat->idle;
                int i_hrs = diff / 3600;
                int i_mins = (diff - i_hrs * 3600) / 60;
                mir_snprintf(szBuf, sizeof(szBuf), "%s    Idle: %dh,%02dm", dat->uin, i_hrs, i_mins);
                GetTextExtentPoint32A(dis->hDC, szBuf, lstrlenA(szBuf), &sUIN);
                DrawTextA(dis->hDC, szBuf, lstrlenA(szBuf), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
            }
            else {
                GetTextExtentPoint32A(dis->hDC, dat->uin, lstrlenA(dat->uin), &sUIN);
                DrawTextA(dis->hDC, dat->uin, lstrlenA(dat->uin), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
            }
            if(dat->timezone != -1) {
                DBTIMETOSTRING dbtts;
                char szResult[80];
                time_t final_time;
                time_t now = time(NULL);
                HFONT oldFont = 0;
                COLORREF oldColor;
                int base_hour;
                char symbolic_time[3];
                
                final_time = now - dat->timediff;
                dbtts.szDest = szResult;
                dbtts.cbDest = 70;
                dbtts.szFormat = "t";
                CallService(MS_DB_TIME_TIMESTAMPTOSTRING, final_time, (LPARAM) & dbtts);
                if(config) {
                    SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_TIME]);
                    SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_TIME]);
                }
                GetTextExtentPoint32A(dis->hDC, szResult, lstrlenA(szResult), &sTime);
                if(sUIN.cx + sTime.cx + 23 < dis->rcItem.right - dis->rcItem.left) {
                    dis->rcItem.left = dis->rcItem.right - sTime.cx - 3 - 16;
                    oldFont = SelectObject(dis->hDC, myGlobals.m_hFontWebdings);
                    oldColor = GetTextColor(dis->hDC);
                    SetTextColor(dis->hDC, myGlobals.ipConfig.clrClockSymbol);
                    base_hour = atoi(szResult);
                    base_hour = base_hour > 11 ? base_hour - 12 : base_hour;
                    symbolic_time[0] = (char)(0xB7 + base_hour);
                    symbolic_time[1] = 0;
                    //DrawIconEx(dis->hDC, dis->rcItem.left, (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2, myGlobals.g_IconClock, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
                    DrawTextA(dis->hDC, symbolic_time, 1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
                    SelectObject(dis->hDC, oldFont);
                    SetTextColor(dis->hDC, oldColor);
                    dis->rcItem.left += 16;
                    DrawTextA(dis->hDC, szResult, lstrlenA(szResult), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
                }
            }
        }
        if(config && hOldFont)
            SelectObject(dis->hDC, hOldFont);
        return TRUE;
	}
	else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_STATICTEXT) || dis->hwndItem == GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT)) {
		TCHAR szWindowText[256];
		if(dat->pContainer->bSkinned) {
			SetTextColor(dis->hDC, myGlobals.skinDefaultFontColor);
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
		}
		else {
			SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
		}
		GetWindowText(dis->hwndItem, szWindowText, 255);
		szWindowText[255] = 0;
		SetBkMode(dis->hDC, TRANSPARENT);
		DrawText(dis->hDC, szWindowText, -1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP | DT_END_ELLIPSIS);
		return TRUE;
	}
	else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_STATICERRORICON) || dis->hwndItem == GetDlgItem(hwndDlg, IDC_MULTIPLEICON)) {
		_DebugPopup(0, "draw icon");
		if(dat->pContainer->bSkinned)
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
		else
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
		DrawIconEx(dis->hDC, (dis->rcItem.right - dis->rcItem.left) / 2 - 8, (dis->rcItem.bottom - dis->rcItem.top) / 2 - 8,
				   (HICON)SendMessage(dis->hwndItem, STM_GETICON, 0, 0), 16, 16, 0, 0, DI_NORMAL);
		return TRUE;
	}
    return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
}

void LoadThemeDefaults(HWND hwndDlg, struct MessageWindowData *dat)
{
    int i;
    char szTemp[40];
    COLORREF colour;
    
    dat->theme.inbg = myGlobals.crIncoming;
    dat->theme.outbg = myGlobals.crOutgoing;
    dat->theme.bg = myGlobals.crDefault;
    dat->theme.hgrid = DBGetContactSettingDword(NULL, FONTMODULE, "hgrid", RGB(224,224,224));
    dat->theme.left_indent = DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0) * 15;
    dat->theme.right_indent = DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0) * 15;
    dat->theme.inputbg = DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", SRMSGDEFSET_BKGCOLOUR);
    
    for(i = 1; i <= 5; i++) {
        _snprintf(szTemp, 10, "cc%d", i);
        colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, RGB(224,224,224));
        if(colour == 0)
            colour = RGB(1,1,1);
        dat->theme.custom_colors[i - 1] = colour;
    }
    dat->theme.logFonts = logfonts;
    dat->theme.fontColors = fontcolors;
    dat->theme.rtfFonts = NULL;
}

void LoadOverrideTheme(HWND hwndDlg, struct MessageWindowData *dat)
{
    BOOL bReadTemplates = ((dat->pContainer->ltr_templates == NULL) || (dat->pContainer->rtl_templates == NULL) ||
                           (dat->pContainer->logFonts == NULL) || (dat->pContainer->fontColors == NULL));
    
    if(lstrlenA(dat->pContainer->szThemeFile) > 1) {
        HANDLE hFile;
        if((hFile = CreateFileA(dat->pContainer->szThemeFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE ) {
            CloseHandle(hFile);
            if(CheckThemeVersion(dat->pContainer->szThemeFile) == 0) {
                _DebugPopup(0, "%s is an incompatible theme.", dat->pContainer->szThemeFile);
                LoadThemeDefaults(hwndDlg, dat);
                return;
            }
            if(dat->pContainer->ltr_templates == NULL) {
                dat->pContainer->ltr_templates = (TemplateSet *)malloc(sizeof(TemplateSet));
                CopyMemory(dat->pContainer->ltr_templates, &LTR_Active, sizeof(TemplateSet));
            }
            if(dat->pContainer->rtl_templates == NULL) {
                dat->pContainer->rtl_templates = (TemplateSet *)malloc(sizeof(TemplateSet));
                CopyMemory(dat->pContainer->rtl_templates, &RTL_Active, sizeof(TemplateSet));
            }
            
            if(dat->pContainer->logFonts == NULL)
                dat->pContainer->logFonts = malloc(sizeof(LOGFONTA) * (MSGDLGFONTCOUNT + 2));
            if(dat->pContainer->fontColors == NULL)
                dat->pContainer->fontColors = malloc(sizeof(COLORREF) * (MSGDLGFONTCOUNT + 2));
            if(dat->pContainer->rtfFonts == NULL)
                dat->pContainer->rtfFonts = malloc((MSGDLGFONTCOUNT + 2) * RTFCACHELINESIZE);
            
            dat->ltr_templates = dat->pContainer->ltr_templates;
            dat->rtl_templates = dat->pContainer->rtl_templates;
            dat->theme.logFonts = dat->pContainer->logFonts;
            dat->theme.fontColors = dat->pContainer->fontColors;
            dat->theme.rtfFonts = dat->pContainer->rtfFonts;
			ReadThemeFromINI(dat->pContainer->szThemeFile, dat, bReadTemplates ? 0 : 1);
            dat->dwFlags = dat->theme.dwFlags;
            dat->theme.left_indent *= 15;
            dat->theme.right_indent *= 15;
            dat->dwFlags |= MWF_SHOW_PRIVATETHEME;
            return;
        }
    }
    LoadThemeDefaults(hwndDlg, dat);
}

void SaveMessageLogFlags(HWND hwndDlg, struct MessageWindowData *dat)
{
    char szBuf[100];
    
    if(!(dat->dwFlags & MWF_SHOW_PRIVATETHEME))
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
    else {
        HANDLE hFile;

        if((hFile = CreateFileA(dat->pContainer->szThemeFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE ) {
            CloseHandle(hFile);
            WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(dat->dwFlags & MWF_LOG_ALL, szBuf, 10), dat->pContainer->szThemeFile);
        }
    }
}

void ConfigureSmileyButton(HWND hwndDlg, struct MessageWindowData *dat)
{
    HICON hButtonIcon = 0;
    int nrSmileys = 0;
    int showToolbar = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;

    if(myGlobals.g_SmileyAddAvail && myGlobals.m_SmileyPluginEnabled && dat->hwndLog == 0) {
        nrSmileys = CheckValidSmileyPack(dat->bIsMeta ? dat->szMetaProto : dat->szProto, &hButtonIcon);

        dat->doSmileys = 1;

        if(hButtonIcon == 0) {
            dat->hSmileyIcon = 0;
            SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[11]);
        }
        else {
            SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hButtonIcon);
            dat->hSmileyIcon = hButtonIcon;
        }
    }
    else if(dat->hwndLog != 0) {
        dat->doSmileys = 1;
        nrSmileys = 1;
        if(dat->hSmileyIcon != 0) {
            DestroyIcon(dat->hSmileyIcon);
            dat->hSmileyIcon = 0;
        }
        SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[11]);
    }

    if(nrSmileys == 0 || dat->hContact == 0)
        dat->doSmileys = 0;

    ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), (dat->doSmileys && showToolbar) ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), dat->doSmileys ? TRUE : FALSE);
}

void SendNudge(struct MessageWindowData *dat, HWND hwndDlg)
{
	char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
	char szServiceName[128];
	HANDLE hContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;

	mir_snprintf(szServiceName, 128, "%s/SendNudge", szProto);
	if(ServiceExists(szServiceName))
		CallProtoService(szProto, "/SendNudge", (WPARAM)hContact, 0);
}

// size in TCHARs
// szwBuf must be large enough to hold at least size wchar_t's
// proto may be NULL
// per contact codepage only used with non-unicode cores (to get "faked" unicode nicknames...)

#if defined(_UNICODE)
int MY_GetContactDisplayNameW(HANDLE hContact, wchar_t *szwBuf, unsigned int size, const char *szProto, UINT codePage)
{
	CONTACTINFO ci;
    char *szBasenick = NULL;
        
    if(myGlobals.bUnicodeBuild) {
    	ZeroMemory(&ci, sizeof(ci));
    	ci.cbSize = sizeof(ci);
    	ci.hContact = hContact;
        ci.szProto = (char *)szProto;
    	ci.dwFlag = CNF_DISPLAY | CNF_UNICODE;
    	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
    		if (ci.type == CNFT_ASCIIZ) {
                size_t len = lstrlenW(ci.pszVal);
                wcsncpy(szwBuf, ci.pszVal, size);
                szwBuf[size - 1] = 0;
                mir_free(ci.pszVal);
				return 0;
    		}
    		if (ci.type == CNFT_DWORD) {
                _ltow(ci.dVal, szwBuf, 10 );
                szwBuf[size - 1] = 0;
                return 0;
    		}
    	}
    }
    szBasenick = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);
    MultiByteToWideChar(codePage, 0, szBasenick, -1, szwBuf, size);
    szwBuf[size - 1] = 0;
    return 0;    
}
#endif
