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
#include "m_smileyadd.h"
#include "m_metacontacts.h"
#include "msgdlgutils.h"
#include "m_ieview.h"
#include "functions.h"

#include <math.h>

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;

extern HMODULE g_hInst;
extern HANDLE hMessageWindowList;
void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state);

void WriteThemeToINI(const char *szIniFilename), ReadThemeFromINI(const char *szIniFilename);
char *GetThemeFileName(int iMode);
void UncacheMsgLogIcons(), CacheMsgLogIcons(), CacheLogFonts();
void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc);
int MessageWindowOpened(WPARAM wParam, LPARAM LPARAM);

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
        //cx = rcContainer.right - rcContainer.left - 8;
        cx = rc.right - rc.left;

        if(dat->iRealAvatarHeight == 0) {               // calc first layout paramaters
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
            if(dat->splitterY <= dat->bottomOffset + (showToolbar ? 0 : 27))
                dat->splitterY = dat->bottomOffset + (showToolbar ? 1 : 27);;
            if(dat->splitterY - 27 > dat->bottomOffset)
                dat->pic.cy = dat->splitterY - 28;
            else
                dat->pic.cy = dat->bottomOffset;
        }
        else {
            if(dat->splitterY <= dat->bottomOffset + 27 + (showToolbar ? 0 : 27))
                dat->splitterY = dat->bottomOffset + 27 + (showToolbar ? 0 : 27);;
                if(dat->splitterY - 27 > dat->bottomOffset)
                    dat->pic.cy = dat->splitterY - 28;
                else
                    dat->pic.cy = dat->bottomOffset;
        }
    }
    else if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
        picAspect = (double)(bminfo->bmWidth / (double)bminfo->bmHeight);
        picProjectedWidth = (double)((dat->dynaSplitter + ((dat->showUIElements != 0) ? 28 : 2))) * picAspect;

        if(((rc.right - rc.left) - (int)picProjectedWidth) > (dat->iButtonBarNeeds) && !myGlobals.m_AlwaysFullToolbarWidth) {
            dat->iRealAvatarHeight = dat->dynaSplitter + ((dat->showUIElements != 0) ? 30 : 4);
            dat->bottomOffset = dat->dynaSplitter + 100;
        }
        else {
            dat->iRealAvatarHeight = dat->dynaSplitter + 4;
            dat->bottomOffset = -33;
        }
        aspect = (double)dat->iRealAvatarHeight / (double)bminfo->bmHeight;
        newWidth = (double)bminfo->bmWidth * aspect;
        if(newWidth > (double)(rc.right - rc.left) * 0.8)
            newWidth = (double)(rc.right - rc.left) * 0.8;
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
        _snprintf(buffer, sizeof(buffer), "Session close - active for: %d:%02d:%02d, Sent: %d (%d), Rcvd: %d (%d)", now / 3600, now / 60, now % 60, dat->stats.iSent, dat->stats.iSentBytes, dat->stats.iReceived, dat->stats.iReceivedBytes);
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
        int iLocalTime = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0);
        int iRtl = (myGlobals.m_RTLDefault == 0 ? DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) : DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1));
        int iLogStatus = (myGlobals.m_LogStatusChanges != 0) && (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0);

        CheckMenuItem(submenu, ID_LOGMENU_SHOWTIMESTAMP, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWTIME ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_USECONTACTSLOCALTIME, MF_BYCOMMAND | iLocalTime ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWDATE, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWDATES ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWSECONDS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWSECONDS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_INDENTMESSAGEBODY, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_INDENT ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGEICONS_SHOWICONS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWICONS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGEICONS_SYMBOLSINSTEADOFICONS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SYMBOLS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGEICONS_USEINCOMING, MF_BYCOMMAND | dat->dwEventIsShown & MWF_SHOW_INOUTICONS ? MF_CHECKED : MF_UNCHECKED);
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
        EnableMenuItem(submenu, ID_PICMENU_RESETTHEAVATAR, MF_BYCOMMAND | ( dat->showPic ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(submenu, ID_PICMENU_DISABLEAUTOMATICAVATARUPDATES, MF_BYCOMMAND | ( dat->showPic ? MF_ENABLED : MF_GRAYED));
        CheckMenuItem(submenu, ID_PICMENU_DISABLEAUTOMATICAVATARUPDATES, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0) == 1) ? MF_CHECKED : MF_UNCHECKED);
        EnableMenuItem(submenu, ID_PICMENU_TOGGLEAVATARDISPLAY, MF_BYCOMMAND | (myGlobals.m_AvatarMode == 2 ? MF_ENABLED : MF_GRAYED));
        CheckMenuItem(submenu, ID_PICMENU_ALWAYSKEEPTHEBUTTONBARATFULLWIDTH, MF_BYCOMMAND | (myGlobals.m_AlwaysFullToolbarWidth ? MF_CHECKED : MF_UNCHECKED));
    }
    return 0;
}

int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId)
{
    if(menuId == MENU_PICMENU || menuId == MENU_TABCONTEXT) {
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
            case ID_PICMENU_TOGGLEAVATARDISPLAY:
                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", dat->showPic ? 0 : 1);
                ShowPicture(hwndDlg,dat,FALSE,FALSE, FALSE);
                SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
                return 1;
            case ID_PICMENU_RESETTHEAVATAR:
                DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic");
                if(dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown)
                    DeleteObject(dat->hContactPic);
                dat->hContactPic = myGlobals.g_hbmUnknown;
                DBDeleteContactSetting(dat->hContact, "ContactPhoto", "File");
                SendMessage(hwndDlg, DM_RETRIEVEAVATAR, 0, 0);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
                SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                return 1;
            case ID_PICMENU_DISABLEAUTOMATICAVATARUPDATES:
                {
                    int iState = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0);
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", !iState);
                    return 1;
                }
            case ID_PICMENU_ALWAYSKEEPTHEBUTTONBARATFULLWIDTH:
                myGlobals.m_AlwaysFullToolbarWidth = !myGlobals.m_AlwaysFullToolbarWidth;
                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "alwaysfulltoolbar", myGlobals.m_AlwaysFullToolbarWidth);
                WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
                break;
            case ID_PICMENU_LOADALOCALPICTUREASAVATAR:
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
                    if (GetOpenFileNameA(&ofn))
                        DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File",FileName);
                    else
                        return 1;
                }
                return 1;
        }
    }
    else if(menuId == MENU_LOGMENU) {
        int iLocalTime = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0);
        int iRtl = (myGlobals.m_RTLDefault == 0 ? DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) : DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1));
        int iLogStatus = (myGlobals.m_LogStatusChanges != 0) && (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0);

        DWORD dwOldFlags = dat->dwFlags;

        switch(selection) {

            case ID_LOGMENU_SHOWTIMESTAMP:
                dat->dwFlags ^= MWF_LOG_SHOWTIME;
                return 1;
            case ID_LOGMENU_USECONTACTSLOCALTIME:
                iLocalTime ^=1;
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
                dat->dwFlags = dat->dwFlags & MWF_LOG_SHOWICONS ? dat->dwFlags & ~MWF_LOG_SYMBOLS : dat->dwFlags;
                return 1;
            case ID_MESSAGEICONS_SYMBOLSINSTEADOFICONS:
                dat->dwFlags ^= MWF_LOG_SYMBOLS;
                dat->dwFlags = dat->dwFlags & MWF_LOG_SYMBOLS ? dat->dwFlags & ~MWF_LOG_SHOWICONS : dat->dwFlags;
                return 1;
            case ID_MESSAGEICONS_USEINCOMING:
                dat->dwEventIsShown ^= MWF_SHOW_INOUTICONS;
                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", dat->dwEventIsShown & MWF_SHOW_INOUTICONS ? 1 : 0);
                return 1;
            case ID_LOGMENU_SHOWNICKNAME:
                dat->dwFlags ^= MWF_LOG_SHOWNICK;
                return 1;
            case ID_LOGITEMSTOSHOW_LOGSTATUSCHANGES:
                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", iLogStatus ? 0 : -1);
                return 1;
            case ID_LOGMENU_SAVETHESESETTINGSASDEFAULTVALUES:
                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
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
                        WriteThemeToINI(szFilename);
                    return 1;
                }
            case ID_MESSAGELOG_IMPORTMESSAGELOGSETTINGS:
                {
                    char *szFilename = GetThemeFileName(0);
                    if(szFilename != NULL) {
                        ReadThemeFromINI(szFilename);
                        UncacheMsgLogIcons();
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

void UpdateStatusBarTooltips(HWND hwndDlg, struct MessageWindowData *dat, int iSecIMStatus)
{
    time_t now = time(NULL);
    now = now - dat->stats.started;

    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
        char szTipText[256];

        if(myGlobals.g_SecureIMAvail && iSecIMStatus >= 0) {
            _snprintf(szTipText, sizeof(szTipText), Translate("Secure IM is %s"), iSecIMStatus ? Translate("enabled") : Translate("disabled"));
            SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXTA, 2, (LPARAM)szTipText);
        }
        //_snprintf(szTipText, sizeof(szTipText), "Session stats: Active for: %d:%02d:%02d, Sent: %d (%d), Rcvd: %d (%d)", now / 3600, now / 60, now % 60, dat->stats.iSent, dat->stats.iSentBytes, dat->stats.iReceived, dat->stats.iReceivedBytes);
        //SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXTA, 0, (LPARAM)szTipText);
        _snprintf(szTipText, sizeof(szTipText), Translate("Sounds are %s (click to toggle, SHIFT-click to apply for all containers)"), dat->pContainer->dwFlags & CNT_NOSOUND ? "off" : "on");
        SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXTA, myGlobals.g_SecureIMAvail ? 3 : 2, (LPARAM)szTipText);
    }
}

void UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat)
{
    if (dat->pContainer->hwndStatus && SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0) >= 3) {
        TCHAR buf[128];
        int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));

        _sntprintf(buf, sizeof(buf), _T("%d/%d"), dat->iOpenJobs, len);
        SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 1, (LPARAM) buf);
    }
}

void UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
        int iSecIMStatus = 0;
        SetSelftypingIcon(hwndDlg, dat, DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)));
        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
        if(myGlobals.g_SecureIMAvail) {
            if((iSecIMStatus = CallService("SecureIM/IsContactSecured", (WPARAM)dat->hContact, 0)) != 0)
                SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 2, (LPARAM)myGlobals.g_buttonBarIcons[14]);
            else
                SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 2, (LPARAM)myGlobals.g_buttonBarIcons[15]);
        }
        SendMessage(dat->pContainer->hwndStatus, SB_SETICON, myGlobals.g_SecureIMAvail ? 3 : 2, (LPARAM)(dat->pContainer->dwFlags & CNT_NOSOUND ? myGlobals.g_buttonBarIcons[23] : myGlobals.g_buttonBarIcons[22]));
        UpdateReadChars(hwndDlg, dat);
        UpdateStatusBarTooltips(hwndDlg, dat, iSecIMStatus);
    }
}

void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, int iIcon)
{
    TCITEM item = {0};
    int iOldIcon;
    
    item.mask = TCIF_IMAGE;
    TabCtrl_GetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
    iOldIcon = item.iImage;

    if(iIcon == -1) {    // restore status image
        if(dat->dwFlags & MWF_ERRORSTATE) {
            iIcon = myGlobals.g_IconError;
        }
        else {
            iIcon = dat->iTabImage;
        }
    }
        
    item.iImage = iIcon;
    if (dat->pContainer->iChilds > 1 || !(dat->pContainer->dwFlags & CNT_HIDETABS)) {       // we have tabs
    }
    TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
}

/*
 * retrieve the visiblity of the avatar window, depending on the global setting
 * and local mode
 */

int GetAvatarVisibility(HWND hwndDlg, struct MessageWindowData *dat)
{
    BYTE bAvatarMode = myGlobals.m_AvatarMode;

    dat->showPic = 0;
    switch(bAvatarMode) {
        case 0:             // globally on
            dat->showPic = 1;
            break;
        case 4:             // globally OFF
            dat->showPic = 0;
            break;
        case 3:             // on, if present
            if(dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown)
                dat->showPic = 1;
            else
                dat->showPic = 0;
            break;
        case 1:             // on for protocols with avatar support
            {
                int pCaps;

                if(dat->szProto) {
                    pCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);
                    if((pCaps & PF4_AVATARS) && dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown) {
                        dat->showPic = 1;
                    }
                }
                break;
            }
        case 2:             // default (per contact, as it always was
            dat->showPic = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", 0);
            break;
    }
    return dat->showPic;
}

void SetSelftypingIcon(HWND dlg, struct MessageWindowData *dat, int iMode)
{
    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == dlg) {
        int nParts = SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0);

        if(iMode)
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, nParts - 1, (LPARAM)myGlobals.g_buttonBarIcons[12]);
        else
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, nParts - 1, (LPARAM)myGlobals.g_buttonBarIcons[13]);

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
        CallService(MS_SMILEYADD_GETINFO, 0, (LPARAM)&smainfo);
        *hButtonIcon = smainfo.ButtonIcon;
        return smainfo.NumberOfVisibleSmileys;
    }
    else
        return 0;
}

/*
 * resize the default :) smiley to fit on the button (smileys may be bigger than 16x16.. so it needs to be done)
 */

void CreateSmileyIcon(struct MessageWindowData *dat, HICON hIcon)
{
    HBITMAP hBmp, hoBmp;
    HDC hdc, hdcMem;
    BITMAPINFOHEADER bih = {0};
    int widthBytes;
    RECT rc;
    HBRUSH hBkgBrush;
    ICONINFO ii;
    BITMAP bm;

    int sizex = 0;
    int sizey = 0;
    
    GetIconInfo(hIcon, &ii);
    GetObject(ii.hbmColor, sizeof(bm), &bm);
    sizex = GetSystemMetrics(SM_CXSMICON);
    sizey = GetSystemMetrics(SM_CYSMICON);
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);

    hBkgBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    bih.biSize = sizeof(bih);
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    
    bih.biHeight = sizex;
    bih.biPlanes = 1;
    bih.biWidth = sizey;
    widthBytes = ((bih.biWidth*bih.biBitCount + 31) >> 5) * 4;
    rc.top = rc.left = 0;
    rc.right = bih.biWidth;
    rc.bottom = bih.biHeight;
    hdc = GetDC(NULL);
    hBmp = CreateCompatibleBitmap(hdc, bih.biWidth, bih.biHeight);
    hdcMem = CreateCompatibleDC(hdc);
    hoBmp = (HBITMAP)SelectObject(hdcMem, hBmp);
    FillRect(hdcMem, &rc, hBkgBrush);
    DrawIconEx(hdcMem, 0, 0, hIcon, bih.biWidth, bih.biHeight, 0, NULL, DI_NORMAL);
    SelectObject(hdcMem, hoBmp);

    DeleteDC(hdcMem);
    DeleteObject(hoBmp);
    ReleaseDC(NULL, hdc);
    DeleteObject(hBkgBrush);
    dat->hSmileyIcon = hBmp;
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

DWORD WINAPI LoadPictureThread(LPVOID param)
{
    HBITMAP hBm;
    HWND hwndDlg = (HWND)param;
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
    DBVARIANT dbv;

    if(dat) {
        if (!DBGetContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic",&dbv)) {
            hBm = (HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
            DBFreeVariant(&dbv);
            if(hBm == 0)
                return 0;
            if(IsWindow(hwndDlg)) {
                dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
                if(dat) {
                    dat->hContactPic = hBm;
                    SendMessage((HWND)param, DM_PICTHREADCOMPLETE, 0, (LPARAM)hBm);
                }
                else
                    DeleteObject(hBm);
            }
            else
                DeleteObject(hBm);
        }
    }
    return 0;
}

void ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL changePic, BOOL showNewPic, BOOL startThread)
{
    DBVARIANT dbv;
    RECT rc;
    int picFailed = FALSE;
    int iUnknown = FALSE;
    
    dat->pic.cy = dat->pic.cx = 60;                        
    if(changePic && startThread) {
        if (dat->hContactPic) {
            if(dat->hContactPic != myGlobals.g_hbmUnknown)
                DeleteObject(dat->hContactPic);
            dat->hContactPic=NULL;
        }
        DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic");
    }
    
    if (showNewPic) {
        if (startThread && dat->hContactPic) {
            if(dat->hContactPic != myGlobals.g_hbmUnknown)
                DeleteObject(dat->hContactPic);
            dat->hContactPic=NULL;
        }
        if (DBGetContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic",&dbv)) {
            if (!DBGetContactSetting(dat->hContact,"ContactPhoto","File",&dbv)) {
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic" ,dbv.pszVal);
                DBFreeVariant(&dbv);
            } else {
                if (!DBGetContactSetting(dat->hContact, dat->szProto, "Photo", &dbv)) {
                    DBWriteContactSettingString(dat->hContact, SRMSGMOD_T,"MOD_Pic",dbv.pszVal);
                    DBFreeVariant(&dbv);
                }
                else
                    iUnknown = TRUE;
            }
        }
        if (!DBGetContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic",&dbv) || iUnknown) {
            BITMAP bminfo;
            BOOL isNoPic = FALSE;
            int maxImageSizeX=500;
            int maxImageSizeY=300;
            DWORD dwThreadId;
            HANDLE hFile;
            
            /*
             * use non-blocking (in its own thread) avatar loading only if its not a local file (i.e. ICQ web photo).
             * note that remote avatars via MSN are considered to be local files, because downloading these is a
             * 2-stage process (notificaton about the change and a further notification when the file is ready
             */
            
            if(iUnknown) {
                dat->hContactPic = myGlobals.g_hbmUnknown;
            }
            else {
                if((hFile = CreateFileA(dbv.pszVal, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
                    if(startThread && dat->hThread == 0) {
                        dat->showPic = 0;
                        ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), SW_HIDE);
                        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                        SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                        SendMessage(hwndDlg, WM_SIZE, 0, 0);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
                        dat->hThread = CreateThread(NULL, 0, LoadPictureThread, (LPVOID)hwndDlg, 0, &dwThreadId);
                        DBFreeVariant(&dbv);
                        return;
                    }
                }
                else {
                    CloseHandle(hFile);
                    dat->hContactPic=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
                }
            }

            if(dat->hContactPic == 0)
                dat->hContactPic = myGlobals.g_hbmUnknown;
            else {
                GetObject(dat->hContactPic,sizeof(bminfo),&bminfo);
                if (bminfo.bmWidth <= 0 || bminfo.bmHeight <= 0) 
                    isNoPic=TRUE;
            }
            if (isNoPic) {
                _DebugPopup(dat->hContact, "%s %s", dbv.pszVal, Translate("has either a wrong size (max 150 x 150) or is not a recognized image file"));
                if(dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown)
                    DeleteObject(dat->hContactPic);
                dat->hContactPic = myGlobals.g_hbmUnknown;
            }
            DBFreeVariant(&dbv);
        } else {
            if(dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown)
                DeleteObject(dat->hContactPic);
            dat->hContactPic = myGlobals.g_hbmUnknown;
        }
        if(dat->iAvatarDisplayMode != AVATARMODE_DYNAMIC)
            dat->iRealAvatarHeight = 0;
        if(dat->hContactPic) {
            dat->showPic = GetAvatarVisibility(hwndDlg, dat);
            if(dat->dynaSplitter == 0 || dat->splitterY == 0)
                LoadSplitter(hwndDlg, dat);
            dat->dynaSplitter = dat->splitterY - 34;
            if(dat->iAvatarDisplayMode != AVATARMODE_DYNAMIC) {
                BITMAP bm;
                GetObject(dat->hContactPic, sizeof(bm), &bm);
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
    char toolTip[256];

    dat->dwTickLastEvent = GetTickCount();
    
    if(!myGlobals.m_FlashOnClist && nen_options.bTraySupport == FALSE)
        return;

    if(hEvent == 0)
        return;

    if(nen_options.bTraySupport == TRUE) {
        if((GetForegroundWindow() != dat->pContainer->hwnd || dat->pContainer->hwndActive != hwndDlg) && !(dbei->flags & DBEF_SENT) && dbei->eventType == EVENTTYPE_MESSAGE)
            UpdateTrayMenu(dat, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->szStatus, dat->hContact, FALSE);
        return;
    }
    
    if((GetForegroundWindow() != dat->pContainer->hwnd || dat->pContainer->hwndActive != hwndDlg) && !(dbei->flags & DBEF_SENT) && dbei->eventType == EVENTTYPE_MESSAGE && !(dat->dwEventIsShown & MWF_SHOW_FLASHCLIST)) {
        ZeroMemory(&cle, sizeof(cle));
        cle.cbSize = sizeof(cle);
        cle.hContact = (HANDLE) dat->hContact;
        cle.hDbEvent = hEvent;
        cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        cle.pszService = "SRMsg/ReadMessage";
        _snprintf(toolTip, sizeof(toolTip), Translate("Message from %s"), dat->szNickname);
        cle.pszTooltip = toolTip;
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
    
    while(p2 && p2 < pEnd)
	{
		if( _stscanf(p2, lpszFmt, &szRed, &szGreen, &szBlue) > 0 )
		{
			int i;
			for (i = 0;;i ++) {
                if(rtf_ctable[i].szName == NULL)
                    break;
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

    LoadMsgDlgFont(MSGFONTID_MESSAGEAREA, &lf, &color);

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
			_sntprintf(InsertThis, sizeof(InsertThis), _T(""));
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

                    //MessageBoxW(0, p1, L"foo", MB_OK);
                    _sntprintf(szTemp, sizeof(szTemp), _T("%d"), iCol);
                    //MessageBoxW(0, pszText, szTemp, MB_OK);
					iRemoveChars = 3 + _tcslen(szTemp);
					if(bTextHasStarted || iCol)
					//	_sntprintf(InsertThis, sizeof(InsertThis), ( iCol > 1 ) ? (inColor ? _T("[/color][color=%s]") : _T("[color=%s]")) : (inColor ? _T("[/color]") : _T("")), rtf_ctable[iInd].szName);
                        _sntprintf(InsertThis, sizeof(InsertThis), ( iInd > 0 ) ? (inColor ? _T("[/color][color=%s]") : _T("[color=%s]")) : (inColor ? _T("[/color]") : _T("")), rtf_ctable[iInd  - 1].szName);
                    inColor = iInd > 0 ? 1 : 0;
				}
				else if(p1 == _tcsstr(p1, _T("\\highlight"))) //background color
				{
                    TCHAR szTemp[20];
					int iCol = _ttoi(p1 + 10);
					//int iInd = RTFColorToIndex(pIndex, iCol, dat);
					bJustRemovedRTF = TRUE;

                    _sntprintf(szTemp, sizeof(szTemp), _T("%d"), iCol);
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
                    _sntprintf(InsertThis, sizeof(InsertThis), _T("\n"));
                }
				else if(p1 == _tcsstr(p1, _T("\\b"))) //bold
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = (p1[2] != (TCHAR)'0')?2:3;
                    if(!(lf.lfWeight == FW_BOLD)) {          // only allow bold if the font itself isn't a bold one, otherwise just strip it..
                        if(dat->SendFormat == SENDFORMAT_BBCODE)
                            _sntprintf(InsertThis, sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("[b]") : _T("[/b]"));
                        else
                            _sntprintf(InsertThis, sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("*") : _T("*"));
                    }

				}
				else if(p1 == _tcsstr(p1, _T("\\i"))) // italics
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = (p1[2] != (TCHAR)'0')?2:3;
                    if(!lf.lfItalic) {                       // same as for bold
                        if(dat->SendFormat == SENDFORMAT_BBCODE)
                            _sntprintf(InsertThis, sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("[i]") : _T("[/i]"));
                        else
                            _sntprintf(InsertThis, sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("/") : _T("/"));
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
                            _sntprintf(InsertThis, sizeof(InsertThis), (p1[3] != (TCHAR)'0' && p1[3] != (TCHAR)'n') ? _T("[u]") : _T("[/u]"));
                        else
                            _sntprintf(InsertThis, sizeof(InsertThis), (p1[3] != (TCHAR)'0' && p1[3] != (TCHAR)'n') ? _T("_") : _T("_"));
                    }

				}
				else if(p1 == _tcsstr(p1, _T("\\tab"))) // tab
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = 4;
					_sntprintf(InsertThis, sizeof(InsertThis), _T(" "));

				}
				else if(p1[1] == (TCHAR)'\\' || p1[1] == (TCHAR)'{' || p1[1] == (TCHAR)'}' ) // escaped characters
				{
					bTextHasStarted = TRUE;
					bJustRemovedRTF = TRUE;
					iRemoveChars = 2;
					_sntprintf(InsertThis, sizeof(InsertThis), _T("%c"), p1[1]);

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
						_sntprintf(InsertThis, sizeof(InsertThis), _T("%c"), (TCHAR)iLame);
                        
					}
					else
						iRemoveChars = 2;
				}
				else // remove unknown RTF command
				{
					int j = 1;
					bJustRemovedRTF = TRUE;
					while(p1[j] != (TCHAR)' ' && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
						j++;
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
	else // error
	{
		//free(pIndex);
		return FALSE;
	}

	//free(pIndex);
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

    iLength = iStreamLength = lstrlenA(szFromStream) + 1;

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
                miranda_sys_free(ci.pszVal);
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
                miranda_sys_free(ci.pszVal);
                break;
            case CNFT_DWORD:
                mir_snprintf(dat->myUin, sizeof(dat->myUin), "%u", ci.dVal);
                break;
        }
    }
    else
        dat->myUin[0] = 0;
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
        ieWindow.cx = 200;
        ieWindow.cy = 300;
        CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
        dat->hwndLog = ieWindow.hwnd;
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
//    int historyMode = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY);
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
        dat->splitterY = (int) DBGetContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", (DWORD) 150);
    else
        dat->splitterY = (int) DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "splitsplity", DBGetContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", (DWORD) 150));

    if(dat->splitterY < MINSPLITTERY || dat->splitterY < 0)
        dat->splitterY = 150;
}

void PlayIncomingSound(struct ContainerWindowData *pContainer, HWND hwnd)
{
    int iPlay = 0;
    DWORD dwFlags = pContainer->dwFlags;

    if(nen_options.iNoSounds)
        return;
    
    if(dwFlags & CNT_NOSOUND)
        iPlay = FALSE;
    else if(dwFlags & CNT_SYNCSOUNDS) {
        iPlay = !MessageWindowOpened(0, (LPARAM)hwnd);
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
