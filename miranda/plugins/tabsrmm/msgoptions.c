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
*/
#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "../../include/m_clc.h"
#include "../../include/m_clui.h"

#define DM_GETSTATUSMASK (WM_USER + 10)

extern HANDLE hMessageWindowList;
extern HINSTANCE g_hInst;
extern HWND g_hwndHotkeyHandler;
extern int g_wantSnapping;

HMENU BuildContainerMenu();
void UncacheMsgLogIcons(), CacheMsgLogIcons(), CacheLogFonts();

void _DBWriteContactSettingWString(HANDLE hContact, const char *szKey, const char *szSetting, wchar_t *value);
static BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#define FONTF_BOLD   1
#define FONTF_ITALIC 2
#define FONTF_UNDERLINE 4

struct FontOptionsList
{
    char *szDescr;
    COLORREF defColour;
    char *szDefFace;
    BYTE defCharset, defStyle;
    char defSize;
    COLORREF colour;
    char szFace[LF_FACESIZE];
    BYTE charset, style;
    char size;
}
static fontOptionsList[] = {
    {"Outgoing messages", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -10}};
    
TCHAR g_szDefaultContainerName[CONTAINER_NAMELEN + 1];

void LoadMsgDlgFont(int i, LOGFONTA * lf, COLORREF * colour)
{
    char str[32];
    int style;
    DBVARIANT dbv;

    if (colour) {
        wsprintfA(str, "Font%dCol", i);
        *colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, str, fontOptionsList[0].defColour);
    }
    if (lf) {
        HDC hdc = GetDC(NULL);
        wsprintfA(str, "Font%dSize", i);
        if(i == H_MSGFONTID_DIVIDERS)
            lf->lfHeight = 5;
        else {
            lf->lfHeight = (char) DBGetContactSettingByte(NULL, SRMSGMOD_T, str, fontOptionsList[0].defSize);
            lf->lfHeight=-MulDiv(lf->lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        }

        ReleaseDC(NULL,hdc);				
        
        lf->lfWidth = 0;
        lf->lfEscapement = 0;
        lf->lfOrientation = 0;
        wsprintfA(str, "Font%dSty", i);
        style = DBGetContactSettingByte(NULL, SRMSGMOD_T, str, fontOptionsList[0].defStyle);
        lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
        lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
        lf->lfUnderline = style & FONTF_UNDERLINE ? 1 : 0;
        lf->lfStrikeOut = 0;
        wsprintfA(str, "Font%dSet", i);
        lf->lfCharSet = DBGetContactSettingByte(NULL, SRMSGMOD_T, str, fontOptionsList[0].defCharset);
        lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf->lfQuality = DEFAULT_QUALITY;
        lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        wsprintfA(str, "Font%d", i);
        if (DBGetContactSetting(NULL, SRMSGMOD_T, str, &dbv))
            lstrcpyA(lf->lfFaceName, fontOptionsList[0].szDefFace);
        else {
            lstrcpynA(lf->lfFaceName, dbv.pszVal, sizeof(lf->lfFaceName));
            DBFreeVariant(&dbv);
        }
    }
}

static BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            DWORD msgTimeout;
            BOOL translated;
            
            DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);

            TranslateDialogDefault(hwndDlg);
            
            CheckDlgButton(hwndDlg, IDC_SHOWBUTTONLINE, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE));
            CheckDlgButton(hwndDlg, IDC_SHOWINFOLINE, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE));
            CheckDlgButton(hwndDlg, IDC_AUTOMIN, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN));
            CheckDlgButton(hwndDlg, IDC_SENDONENTER, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER));
            CheckDlgButton(hwndDlg, IDC_SHOWSENDBTN, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON));

            SendDlgItemMessageA(hwndDlg, IDC_NOTIFYTYPE, CB_INSERTSTRING, -1, (LPARAM)Translate("None"));
            SendDlgItemMessageA(hwndDlg, IDC_NOTIFYTYPE, CB_INSERTSTRING, -1, (LPARAM)Translate("Tray notifications"));
            SendDlgItemMessageA(hwndDlg, IDC_NOTIFYTYPE, CB_INSERTSTRING, -1, (LPARAM)Translate("Popups"));

            SendDlgItemMessage(hwndDlg, IDC_NOTIFYTYPE, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "debuginfo", 0), 0);

            CheckDlgButton(hwndDlg, IDC_USEDIVIDERS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "usedividers", 0));
            CheckDlgButton(hwndDlg, IDC_AUTOSWITCHTABS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", 0));
            CheckDlgButton(hwndDlg, IDC_DIVIDERSUSEPOPUPCONFIG, DBGetContactSettingByte(NULL, SRMSGMOD_T, "div_popupconfig", 0));
            CheckDlgButton(hwndDlg, IDC_SENDONSHIFTENTER, DBGetContactSettingByte(NULL, SRMSGMOD_T, "sendonshiftenter", 1));
            CheckDlgButton(hwndDlg, IDC_EVENTAPI, DBGetContactSettingByte(NULL, SRMSGMOD_T, "eventapi", 1));
            CheckDlgButton(hwndDlg, IDC_DELETETEMP, DBGetContactSettingByte(NULL, SRMSGMOD_T, "deletetemp", 0));
            CheckDlgButton(hwndDlg, IDC_FLASHCLIST, DBGetContactSettingByte(NULL, SRMSGMOD_T, "flashcl", 0));
            
            CheckDlgButton(hwndDlg, IDC_LIMITAVATARS, dwFlags & MWF_LOG_LIMITAVATARHEIGHT);

            SendDlgItemMessageA(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)Translate("Globally on"));
            SendDlgItemMessageA(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)Translate("On for protocols with avatar support"));
            SendDlgItemMessageA(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)Translate("Per contact setting"));
            SendDlgItemMessageA(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)Translate("On, if present"));
            SendDlgItemMessageA(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)Translate("Globally OFF"));
            CheckDlgButton(hwndDlg, IDC_AVADYNAMIC, DBGetContactSettingByte(NULL, SRMSGMOD_T, "dynamicavatarsize", 0));
            
            SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 0), 0);
            
#if defined(_STREAMTHREADING)
            CheckDlgButton(hwndDlg, IDC_STREAMTHREADING, DBGetContactSettingByte(NULL, SRMSGMOD_T, "streamthreading", 0));
#else
            EnableWindow(GetDlgItem(hwndDlg, IDC_STREAMTHREADING), FALSE);
#endif            
            msgTimeout = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
            SetDlgItemInt(hwndDlg, IDC_SECONDS, msgTimeout >= SRMSGSET_MSGTIMEOUT_MIN ? msgTimeout / 1000 : SRMSGDEFSET_MSGTIMEOUT / 1000, FALSE);

            SetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", 100), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_AVATARSPIN, UDM_SETRANGE, 0, MAKELONG(150, 50));
            SendDlgItemMessage(hwndDlg, IDC_AVATARSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, &translated, FALSE));

            SetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabautoclose", 0), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_AUTOCLOSETABSPIN, UDM_SETRANGE, 0, MAKELONG(1800, 0));
            SendDlgItemMessage(hwndDlg, IDC_AUTOCLOSETABSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE));

            CheckDlgButton(hwndDlg, IDC_AUTOCLOSELAST, DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocloselast", 0));
            EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCLOSELAST), GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE) > 0);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_AUTOMIN:
                    CheckDlgButton(hwndDlg, IDC_AUTOCLOSE, BST_UNCHECKED);
                    break;
                case IDC_AUTOCLOSE:
                    CheckDlgButton(hwndDlg, IDC_AUTOMIN, BST_UNCHECKED);
                    break;
                case IDC_SENDONENTER:
                    CheckDlgButton(hwndDlg, IDC_SENDONDBLENTER, BST_UNCHECKED);
                    break;
                case IDC_SENDONDBLENTER:
                    CheckDlgButton(hwndDlg, IDC_SENDONENTER, BST_UNCHECKED);
                    break;
                case IDC_SECONDS:
                case IDC_MAXAVATARHEIGHT:
                case IDC_AUTOCLOSETABTIME:
                {
                    BOOL translated;
                    
                    EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCLOSELAST), GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE) > 0);
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return 0;
                    break;
                }
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                        {
                            DWORD msgTimeout;
                            DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
                            BOOL translated;
                            
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWBUTTONLINE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWBUTTONLINE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWINFOLINE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWINFOLINE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOMIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOMIN));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOCLOSE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDONENTER, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SENDONENTER));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDBUTTON, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSENDBTN));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "debuginfo", (BYTE) SendDlgItemMessage(hwndDlg, IDC_NOTIFYTYPE, CB_GETCURSEL, 0, 0));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "usedividers", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_USEDIVIDERS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOSWITCHTABS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "div_popupconfig", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DIVIDERSUSEPOPUPCONFIG));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "streamthreading", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_STREAMTHREADING));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", (BYTE) SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_GETCURSEL, 0, 0));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "dynamicavatarsize", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AVADYNAMIC));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "sendonshiftenter", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SENDONSHIFTENTER));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "eventapi", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_EVENTAPI));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "deletetemp", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DELETETEMP));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "flashcl", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FLASHCLIST));
                            
                            if(IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARS))
                                dwFlags |= MWF_LOG_LIMITAVATARHEIGHT;
                            else
                                dwFlags &= ~MWF_LOG_LIMITAVATARHEIGHT;
                            
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dwFlags);
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", GetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, &translated, FALSE));

                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "tabautoclose", GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "autocloselast", IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSELAST));
                            
                            msgTimeout = GetDlgItemInt(hwndDlg, IDC_SECONDS, NULL, TRUE) >= SRMSGSET_MSGTIMEOUT_MIN / 1000 ? GetDlgItemInt(hwndDlg, IDC_SECONDS, NULL, TRUE) * 1000 : SRMSGDEFSET_MSGTIMEOUT;
                            DBWriteContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, msgTimeout);

                            WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
                            return TRUE;
                        }
                    }
                    break;
            }
            break;
        case WM_DESTROY:
            break;
    }
    return FALSE;
}

static BOOL CALLBACK DlgProcLogOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL translated;
    DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
    
    switch (msg) {
        case WM_INITDIALOG:
            TranslateDialogDefault(hwndDlg);
            switch (DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY)) {
                case LOADHISTORY_UNREAD:
                    CheckDlgButton(hwndDlg, IDC_LOADUNREAD, BST_CHECKED);
                    break;
                case LOADHISTORY_COUNT:
                    CheckDlgButton(hwndDlg, IDC_LOADCOUNT, BST_CHECKED);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTN), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTSPIN), TRUE);
                    break;
                case LOADHISTORY_TIME:
                    CheckDlgButton(hwndDlg, IDC_LOADTIME, BST_CHECKED);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMEN), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMESPIN), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STMINSOLD), TRUE);
                    break;
            }
            SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
            SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT));
            SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_SETRANGE, 0, MAKELONG(12 * 60, 0));
            SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME));

            CheckDlgButton(hwndDlg, IDC_SHOWLOGICONS, dwFlags & MWF_LOG_SHOWICONS);
            CheckDlgButton(hwndDlg, IDC_SHOWNAMES, dwFlags & MWF_LOG_SHOWNICK);
            CheckDlgButton(hwndDlg, IDC_SHOWTIMES, dwFlags & MWF_LOG_SHOWTIME);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWDATES), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
            CheckDlgButton(hwndDlg, IDC_SHOWDATES, dwFlags & MWF_LOG_SHOWDATES);
            CheckDlgButton(hwndDlg, IDC_SHOWURLS, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWURLS, SRMSGDEFSET_SHOWURLS));
            CheckDlgButton(hwndDlg, IDC_SHOWFILES, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWFILES, SRMSGDEFSET_SHOWFILES));
            
			// XXX options: show event in a new line and indent events (x pixel)

			CheckDlgButton(hwndDlg, IDC_SHOW_EVENTS_NL, dwFlags & MWF_LOG_NEWLINE);
			CheckDlgButton(hwndDlg, IDC_INDENT, dwFlags & MWF_LOG_INDENT);
			CheckDlgButton(hwndDlg, IDC_UNDERLINE, dwFlags & MWF_LOG_UNDERLINE);
			CheckDlgButton(hwndDlg, IDC_SHOWSECONDS, dwFlags & MWF_LOG_SHOWSECONDS);
            CheckDlgButton(hwndDlg, IDC_SWAPTIMESTAMP, dwFlags & MWF_LOG_SWAPNICK);
            CheckDlgButton(hwndDlg, IDC_DRAWGRID, dwFlags & MWF_LOG_GRID);
            CheckDlgButton(hwndDlg, IDC_GROUPMODE, dwFlags & MWF_LOG_GROUPMODE);
            CheckDlgButton(hwndDlg, IDC_LONGDATES, dwFlags & MWF_LOG_LONGDATES);
            CheckDlgButton(hwndDlg, IDC_RELATIVEDATES, dwFlags & MWF_LOG_USERELATIVEDATES);
            
            CheckDlgButton(hwndDlg, IDC_EMPTYLINEFIX, DBGetContactSettingByte(NULL, SRMSGMOD_T, "emptylinefix", 1));
            CheckDlgButton(hwndDlg, IDC_MARKFOLLOWUPTIMESTAMP, DBGetContactSettingByte(NULL, SRMSGMOD_T, "followupts", 1));
            EnableWindow(GetDlgItem(hwndDlg, IDC_MARKFOLLOWUPTIMESTAMP), IsDlgButtonChecked(hwndDlg, IDC_GROUPMODE));
            
            CheckDlgButton(hwndDlg, IDC_INOUTICONS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", 0));
            CheckDlgButton(hwndDlg, IDC_LOGSTATUS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "logstatus", 0));

            CheckDlgButton(hwndDlg, IDC_IGNORECONTACTSETTINGS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 1));
            CheckDlgButton(hwndDlg, IDC_LOGHOTKEYS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "hotkeys", 0));
            
            SetDlgItemInt(hwndDlg, IDC_INDENTAMOUNT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 0));
            SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_INDENTAMOUNT, &translated, FALSE));

            SetDlgItemInt(hwndDlg, IDC_RIGHTINDENT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_RINDENTSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 0));
            SendDlgItemMessage(hwndDlg, IDC_RINDENTSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_RIGHTINDENT, &translated, FALSE));
            
            EnableWindow(GetDlgItem(hwndDlg, IDC_INOUTICONS), IsDlgButtonChecked(hwndDlg, IDC_SHOWLOGICONS));
            
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LOADUNREAD:
                case IDC_LOADCOUNT:
                case IDC_LOADTIME:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTN), IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTSPIN), IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMEN), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMESPIN), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STMINSOLD), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
                    break;
                case IDC_SHOWLOGICONS:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INOUTICONS), IsDlgButtonChecked(hwndDlg, IDC_SHOWLOGICONS));
                    break;
                case IDC_SHOWTIMES:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWDATES), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSECONDS), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
                    break;
                case IDC_INDENTAMOUNT:
                case IDC_LOADCOUNTN:
                case IDC_LOADTIMEN:
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return TRUE;
                    break;
                case IDC_GROUPMODE:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MARKFOLLOWUPTIMESTAMP), IsDlgButtonChecked(hwndDlg, IDC_GROUPMODE));
                    break;
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY: {
                            DWORD dwOldFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
                            DWORD dwFlags = 0;
                            
                            if (IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT))
                                DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_COUNT);
                            else if (IsDlgButtonChecked(hwndDlg, IDC_LOADTIME))
                                DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_TIME);
                            else
                                DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_UNREAD);
                            DBWriteContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, (WORD) SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, (WORD) SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_GETPOS, 0, 0));
                            dwFlags = (dwOldFlags & (MWF_LOG_LIMITAVATARHEIGHT | MWF_LOG_INDIVIDUALBKG)) | 
                                      (IsDlgButtonChecked(hwndDlg, IDC_SHOWLOGICONS) ? MWF_LOG_SHOWICONS : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_SHOWNAMES) ? MWF_LOG_SHOWNICK : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES) ? MWF_LOG_SHOWTIME : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_SHOWDATES) ? MWF_LOG_SHOWDATES : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_DRAWGRID) ? MWF_LOG_GRID : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_GROUPMODE) ? MWF_LOG_GROUPMODE : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_RELATIVEDATES) ? MWF_LOG_USERELATIVEDATES : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_LONGDATES) ? MWF_LOG_LONGDATES : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_USEINDIVIDUALBKG) ? MWF_LOG_INDIVIDUALBKG : 0) |
                                      (IsDlgButtonChecked(hwndDlg, IDC_SWAPTIMESTAMP) ? MWF_LOG_SWAPNICK : 0);
                            
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "hotkeys", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LOGHOTKEYS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_IGNORECONTACTSETTINGS));
                            
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWURLS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWURLS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWFILES, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWFILES));

                            // XXX new options (indent, new line)

                            dwFlags |= ((IsDlgButtonChecked(hwndDlg, IDC_SHOW_EVENTS_NL) ? MWF_LOG_NEWLINE : 0) |
                                       (IsDlgButtonChecked(hwndDlg, IDC_INDENT) ? MWF_LOG_INDENT : 0) |
                                       (IsDlgButtonChecked(hwndDlg, IDC_UNDERLINE) ? MWF_LOG_UNDERLINE : 0) |
                                       (IsDlgButtonChecked(hwndDlg, IDC_SHOWSECONDS) ? MWF_LOG_SHOWSECONDS : 0));

                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dwFlags);
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", (DWORD) GetDlgItemInt(hwndDlg, IDC_INDENTAMOUNT, &translated, FALSE));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", (DWORD) GetDlgItemInt(hwndDlg, IDC_RIGHTINDENT, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "logstatus", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LOGSTATUS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_INOUTICONS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "emptylinefix", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_EMPTYLINEFIX));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "followupts", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MARKFOLLOWUPTIMESTAMP));
                            
                            WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
                            return TRUE;
                        }
                    }
                    break;
            }
            break;
        case WM_DESTROY:
            break;
    }
    return FALSE;
}

static ResetCList(HWND hwndDlg)
{
    int i;

    if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_DISABLEGROUPS && !DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETUSEGROUPS, (WPARAM) FALSE, 0);
    else
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETUSEGROUPS, (WPARAM) TRUE, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETHIDEEMPTYGROUPS, 1, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETLEFTMARGIN, 2, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKBITMAP, 0, (LPARAM) (HBITMAP) NULL);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETINDENT, 10, 0);
    for (i = 0; i <= FONTID_MAX; i++)
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETTEXTCOLOR, i, GetSysColor(COLOR_WINDOWTEXT));
}

static void RebuildList(HWND hwndDlg, HANDLE hItemNew, HANDLE hItemUnknown)
{
    HANDLE hContact, hItem;
    BYTE defType = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW);

    if (hItemNew && defType) {
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItemNew, 1);
    }
    if (hItemUnknown && DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN)) {
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItemUnknown, 1);
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
        if (hItem && DBGetContactSettingByte(hContact, SRMSGMOD, SRMSGSET_TYPING, defType)) {
            SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItem, 1);
        }
    } while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
}

static void SaveList(HWND hwndDlg, HANDLE hItemNew, HANDLE hItemUnknown)
{
    HANDLE hContact, hItem;

    if (hItemNew) {
        DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItemNew, 0) ? 1 : 0);
    }
    if (hItemUnknown) {
        DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGUNKNOWN, SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItemUnknown, 0) ? 1 : 0);
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
        if (hItem) {
            DBWriteContactSettingByte(hContact, SRMSGMOD, SRMSGSET_TYPING, SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0) ? 1 : 0);
        }
    } while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
}

static BOOL CALLBACK DlgProcTypeOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hItemNew, hItemUnknown;

    switch (msg) {
        case WM_INITDIALOG:
            TranslateDialogDefault(hwndDlg);
            {
                CLCINFOITEM cii = { 0 };
                cii.cbSize = sizeof(cii);
                cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
                cii.pszText = Translate("** New contacts **");
                hItemNew = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM) & cii);
                cii.pszText = Translate("** Unknown contacts **");
                hItemUnknown = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM) & cii);
            }
            SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE) | (CLS_SHOWHIDDEN));
            ResetCList(hwndDlg);
            RebuildList(hwndDlg, hItemNew, hItemUnknown);
            CheckDlgButton(hwndDlg, IDC_SHOWNOTIFY, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING));
            CheckDlgButton(hwndDlg, IDC_TYPEWIN, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN));
            CheckDlgButton(hwndDlg, IDC_TYPETRAY, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN));
            CheckDlgButton(hwndDlg, IDC_NOTIFYTRAY, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST));
            CheckDlgButton(hwndDlg, IDC_NOTIFYBALLOON, !DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST));
            EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEWIN), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
            EnableWindow(GetDlgItem(hwndDlg, IDC_TYPETRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
			if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), FALSE);
				CheckDlgButton(hwndDlg, IDC_NOTIFYTRAY, BST_CHECKED);
				SetWindowTextA(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), Translate("Show balloon popup (unsupported system)"));
			}
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_TYPETRAY:
					if (IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY)) {
						if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
							EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), TRUE);
						}
						else {
							EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), TRUE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), TRUE);
						}
					}
					else {
						EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), FALSE);
					}
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
                case IDC_SHOWNOTIFY:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEWIN), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TYPETRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY)&&ServiceExists(MS_CLIST_SYSTRAY_NOTIFY));
                    //fall-thru
                case IDC_TYPEWIN:
				case IDC_NOTIFYTRAY:
				case IDC_NOTIFYBALLOON:
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                    break;
            }
            break;
        case WM_NOTIFY:
            switch (((NMHDR *) lParam)->idFrom) {
                case IDC_CLIST:
                    switch (((NMHDR *) lParam)->code) {
                        case CLN_OPTIONSCHANGED:
                            ResetCList(hwndDlg);
                            break;
                        case CLN_CHECKCHANGED:
                            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                            break;
                    }
                    break;
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                        {
                            SaveList(hwndDlg, hItemNew, hItemUnknown);
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPING, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TYPEWIN));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGNOWIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_NOTIFYTRAY));
                            WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
                        }
                    }
                    break;
            }
            break;
    }
    return FALSE;
}

/*
 * options for tabbed messaging got their own page.. finally :)
 */

static BOOL CALLBACK DlgProcTabbedOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL translated;

	switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
 
			// XXX tab support options...
			CheckDlgButton(hwndDlg, IDC_WARNONCLOSE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0));
			CheckDlgButton(hwndDlg, IDC_CUT_TABTITLE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0));
			SetDlgItemInt(hwndDlg, IDC_CUT_TITLEMAX, DBGetContactSettingWord(NULL, SRMSGMOD_T, "cut_at", 15), FALSE);
			CheckDlgButton(hwndDlg, IDC_SHOWTABTIP, DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabtips", 0));
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUSONTAB, DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabstatus", 0));
            CheckDlgButton(hwndDlg, IDC_AUTOCREATETABS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "autotabs", 0));
            CheckDlgButton(hwndDlg, IDC_POPUPCONTAINER, DBGetContactSettingByte(NULL, SRMSGMOD_T, "cpopup", 0));
            CheckDlgButton(hwndDlg, IDC_AUTOCREATECONTAINER, DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocontainer", 0));
            CheckDlgButton(hwndDlg, IDC_AUTOLOCALE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0));
            CheckDlgButton(hwndDlg, IDC_FLATBUTTONS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0));
            CheckDlgButton(hwndDlg, IDC_FULLUSERNAME, DBGetContactSettingByte(NULL, SRMSGMOD_T, "fulluin", 1));
            CheckDlgButton(hwndDlg, IDC_ESC_MINIMIZE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "escmode", 0));
            CheckDlgButton(hwndDlg, IDC_SPLITTERSTATICEDGES, DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", 1));
            CheckDlgButton(hwndDlg, IDC_AUTOPOPUP, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP));

            SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(10, 1));
            SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMSGMOD_T, "y-pad", 3));
            SetDlgItemInt(hwndDlg, IDC_TABPADDING, DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3), FALSE);;

            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "tborder", 0));
            SetDlgItemInt(hwndDlg, IDC_TABBORDER, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "tborder", 0), FALSE);;

            SendDlgItemMessage(hwndDlg, IDC_HISTORYSIZESPIN, UDM_SETRANGE, 0, MAKELONG(255, 5));
            SendDlgItemMessage(hwndDlg, IDC_HISTORYSIZESPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "historysize", 0));
            SetDlgItemInt(hwndDlg, IDC_HISTORYSIZE, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "historysize", 10), FALSE);

            SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_INSERTSTRING, -1, (LPARAM)_T("CTRL-SHIFT"));
            SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_INSERTSTRING, -1, (LPARAM)_T("CTRL-ALT"));
            SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_INSERTSTRING, -1, (LPARAM)_T("ALT-SHIFT"));
            SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "hotkeymodifier", 0), 0);

            CheckDlgButton(hwndDlg, IDC_HOTKEYSAREGLOBAL, DBGetContactSettingByte(NULL, SRMSGMOD_T, "globalhotkeys", 0));

            
         break;
            
        }
        case WM_SHOWWINDOW:
            {
                int iAutoPopup = !DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
                
                EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCREATETABS), iAutoPopup);
                EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCREATECONTAINER), iAutoPopup);
                EnableWindow(GetDlgItem(hwndDlg, IDC_POPUPCONTAINER), iAutoPopup);
                break;
            }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_TABPADDING:
                case IDC_TABBORDER:
                case IDC_HISTORYSIZE:
                case IDC_CUT_TITLEMAX:
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return TRUE;
                    break;
                case IDC_AUTOPOPUP:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCREATECONTAINER), !IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCREATETABS), !IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_POPUPCONTAINER), !IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
                    break;
                case IDC_SETUPAUTOCREATEMODES:
                    CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_CHOOSESTATUSMODES), hwndDlg, DlgProcSetupStatusModes);
                    break;
            }
           
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY:
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_WARNONCLOSE));
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_CUT_TABTITLE));
							DBWriteContactSettingWord(NULL, SRMSGMOD_T, "cut_at", (WORD) GetDlgItemInt(hwndDlg, IDC_CUT_TITLEMAX, &translated, FALSE));
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "tabtips", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWTABTIP));
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "tabstatus", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSONTAB));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "y-pad", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABPADDING, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "autotabs", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOCREATETABS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "cpopup", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_POPUPCONTAINER));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "al", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOLOCALE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "nlflat", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FLATBUTTONS));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "fulluin", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FULLUSERNAME));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "autocontainer", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOCREATECONTAINER));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "tborder", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDER, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "historysize", (BYTE) GetDlgItemInt(hwndDlg, IDC_HISTORYSIZE, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "escmode", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_ESC_MINIMIZE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "hotkeymodifier", (BYTE) SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_GETCURSEL, 0, 0));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "globalhotkeys", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_HOTKEYSAREGLOBAL));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SPLITTERSTATICEDGES));
                            DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOPOPUP, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
                            WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
                            SendMessage(g_hwndHotkeyHandler, DM_FORCEUNREGISTERHOTKEYS, 0, 0);
                            SendMessage(g_hwndHotkeyHandler, DM_REGISTERHOTKEYS, 0, 0);
							return TRUE;
                    }
            }
            break;
        case WM_DESTROY:
            break;
    }
    return FALSE;
}

static BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
        case WM_INITDIALOG:
        {
            DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containerflags", CNT_FLAGS_DEFAULT);
            DBVARIANT dbv;
            BOOL translated;
            
            TranslateDialogDefault(hwndDlg);

            SendDlgItemMessage(hwndDlg, IDC_DEFAULTDISPLAYNAME, EM_SETLIMITTEXT, (WPARAM)CONTAINER_NAMELEN, 0);
            CheckDlgButton(hwndDlg, IDC_CONTAINERGROUPMODE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0));
            CheckDlgButton(hwndDlg, IDC_LIMITTABS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0));

            SendDlgItemMessage(hwndDlg, IDC_TABLIMITSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 1));
            SendDlgItemMessage(hwndDlg, IDC_TABLIMITSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "maxtabs", 1));
            SetDlgItemInt(hwndDlg, IDC_TABLIMIT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "maxtabs", 1), FALSE);

            CheckDlgButton(hwndDlg, IDC_SINGLEWINDOWMODE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0));
            CheckDlgButton(hwndDlg, IDC_DEFAULTCONTAINERMODE, !(IsDlgButtonChecked(hwndDlg, IDC_CONTAINERGROUPMODE) || IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS) || IsDlgButtonChecked(hwndDlg, IDC_SINGLEWINDOWMODE)));

            SetDlgItemInt(hwndDlg, IDC_NRFLASH, DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_NRFLASHSPIN, UDM_SETRANGE, 0, MAKELONG(255, 1));
            SendDlgItemMessage(hwndDlg, IDC_NRFLASHSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4));

            SetDlgItemInt(hwndDlg, IDC_FLASHINTERVAL, DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_FLASHINTERVALSPIN, UDM_SETRANGE, 0, MAKELONG(10000, 500));
            SendDlgItemMessage(hwndDlg, IDC_FLASHINTERVALSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000));
            SendDlgItemMessage(hwndDlg, IDC_FLASHINTERVALSPIN, UDM_SETACCEL, 0, (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000));
            
            if(!ServiceExists("Utils/SnapWindowProc"))
                EnableWindow(GetDlgItem(hwndDlg, IDC_USESNAPPING), FALSE);
            else
                CheckDlgButton(hwndDlg, IDC_USESNAPPING, DBGetContactSettingByte(NULL, SRMSGMOD_T, "usesnapping", 0));
            
#if defined(_UNICODE)
            if(DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW", &dbv))
#else                
            if(DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainername", &dbv))
#endif                
                SetWindowText(GetDlgItem(hwndDlg, IDC_DEFAULTDISPLAYNAME), _T("default"));
            else {
#if defined(_UNICODE)
                if(dbv.type == DBVT_ASCIIZ) {
                    SetWindowText(GetDlgItem(hwndDlg, IDC_DEFAULTDISPLAYNAME), Utf8Decode(dbv.pszVal));
#else
                if(dbv.type == DBVT_ASCIIZ) {
                    SetWindowTextA(GetDlgItem(hwndDlg, IDC_DEFAULTDISPLAYNAME), dbv.pszVal);
#endif                    
                }
                DBFreeVariant(&dbv);
            }
		 return TRUE;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_DEFAULTDISPLAYNAME:
                case IDC_TABLIMIT:
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return TRUE;
                    break;
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY: {
                            BOOL translated;
                            TCHAR szContainerName[CONTAINER_NAMELEN + 1];
                            
                            GetWindowText(GetDlgItem(hwndDlg, IDC_DEFAULTDISPLAYNAME), szContainerName, CONTAINER_NAMELEN * sizeof(TCHAR));
#if defined(_UNICODE)
                            _DBWriteContactSettingWString(NULL, SRMSGMOD_T, "defaultcontainernameW", szContainerName);
#else                            
                            DBWriteContactSettingString(NULL, SRMSGMOD_T, "defaultcontainername", szContainerName);
#endif                            
                            _tcsncpy(g_szDefaultContainerName, szContainerName, CONTAINER_NAMELEN);
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", IsDlgButtonChecked(hwndDlg, IDC_CONTAINERGROUPMODE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "limittabs", IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "maxtabs", GetDlgItemInt(hwndDlg, IDC_TABLIMIT, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", IsDlgButtonChecked(hwndDlg, IDC_SINGLEWINDOWMODE));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", GetDlgItemInt(hwndDlg, IDC_FLASHINTERVAL, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "nrflash", GetDlgItemInt(hwndDlg, IDC_NRFLASH, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "usesnapping", IsDlgButtonChecked(hwndDlg, IDC_USESNAPPING));
                            g_wantSnapping = ServiceExists("Utils/SnapWindowProc") && IsDlgButtonChecked(hwndDlg, IDC_USESNAPPING);
                            BuildContainerMenu();
                            return TRUE;
                        }
                    }
            }
            break;
        case WM_DESTROY:
            break;
    }
    return FALSE;
}

#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

static const char *szFontIdDescr[MSGDLGFONTCOUNT] = {
        ">> Outgoing messages", 
        ">> Outgoing misc events",
        "<< Incoming messages",
        "<< Incoming misc events",
        ">> Outgoing name",
        ">> Outgoing timestamp",
        "<< Incoming name",
        "<< Incoming timestamp",
        ">> Outgoing messages (old)",
        ">> Outgoing misc events (old)",
        "<< Incoming messages (old)",
        "<< Incoming misc events (old)",
        ">> Outgoing name (old)",
        ">> Outgoing time (old)",
        "<< Incoming name (old)",
        "<< Incoming time (old)",
        "* Message Input Area",
        "* Status changes",
        "* Dividers"};

#define FONTS_TO_CONFIG MSGDLGFONTCOUNT

#define SAMEASF_FACE   1
#define SAMEASF_SIZE   2
#define SAMEASF_STYLE  4
#define SAMEASF_COLOUR 8
#include <pshpack1.h>
struct {
	BYTE sameAsFlags,sameAs;
	COLORREF colour;
	char size;
	BYTE style;
	BYTE charset;
	char szFace[LF_FACESIZE];
} static fontSettings[MSGDLGFONTCOUNT + 1];
#include <poppack.h>
static WORD fontSameAsDefault[MSGDLGFONTCOUNT + 2]={0x00FF,0x0B00,0x0F00,0x0700,0x0B00,0x0104,0x0D00,0x0B02,0x00FF,0x0B00,0x0F00,0x0700,0x0B00,0x0104,0x0D00,0x0B02,0x00FF,0x0B00,0x0F00};
static char *fontSizes[]={"8","9","10","11","12","13","14","15","18"};
static int fontListOrder[MSGDLGFONTCOUNT + 1] = {
    MSGFONTID_MYMSG,
    MSGFONTID_MYMISC,
    MSGFONTID_YOURMSG,    
    MSGFONTID_YOURMISC,    
    MSGFONTID_MYNAME,     
    MSGFONTID_MYTIME,     
    MSGFONTID_YOURNAME,   
    MSGFONTID_YOURTIME,   
    H_MSGFONTID_MYMSG,
    H_MSGFONTID_MYMISC,   
    H_MSGFONTID_YOURMSG,  
    H_MSGFONTID_YOURMISC,  
    H_MSGFONTID_MYNAME,
    H_MSGFONTID_MYTIME,
    H_MSGFONTID_YOURNAME, 
    H_MSGFONTID_YOURTIME,
    MSGFONTID_MESSAGEAREA,
    H_MSGFONTID_STATUSCHANGES,
    H_MSGFONTID_DIVIDERS};

#define M_REBUILDFONTGROUP   (WM_USER+10)
#define M_REMAKESAMPLE       (WM_USER+11)
#define M_RECALCONEFONT      (WM_USER+12)
#define M_RECALCOTHERFONTS   (WM_USER+13)
#define M_SAVEFONT           (WM_USER+14)
#define M_REFRESHSAMEASBOXES (WM_USER+15)
#define M_FILLSCRIPTCOMBO    (WM_USER+16)
#define M_REDOROWHEIGHT      (WM_USER+17)
#define M_LOADFONT           (WM_USER+18)
#define M_GUESSSAMEASBOXES   (WM_USER+19)
#define M_SETSAMEASBOXES     (WM_USER+20)

static int CALLBACK EnumFontsProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEXA *lpntme,int FontType,LPARAM lParam)
{
	if(!IsWindow((HWND)lParam)) return FALSE;
	if(SendMessageA((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfLogFont.lfFaceName)==CB_ERR)
		SendMessageA((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfLogFont.lfFaceName);
	return TRUE;
}

void FillFontListThread(HWND hwndDlg)
{
	LOGFONTA lf={0};
	HDC hdc=GetDC(hwndDlg);
	lf.lfCharSet=DEFAULT_CHARSET;
	lf.lfFaceName[0]=0;
	lf.lfPitchAndFamily=0;
	EnumFontFamiliesExA(hdc,&lf,(FONTENUMPROCA)EnumFontsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_TYPEFACE),0);
	ReleaseDC(hwndDlg,hdc);
	return;
}

static int CALLBACK EnumFontScriptsProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEXA *lpntme,int FontType,LPARAM lParam)
{
	if(SendMessageA((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfScript)==CB_ERR) {
		int i=SendMessageA((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfScript);
		SendMessageA((HWND)lParam,CB_SETITEMDATA,i,lpelfe->elfLogFont.lfCharSet);
	}
	return TRUE;
}

static int TextOptsDlgResizer(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static void SwitchTextDlgToMode(HWND hwndDlg,int expert)
{
	ShowWindow(GetDlgItem(hwndDlg,IDC_STSAMETEXT),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMETYPE),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMESIZE),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMESTYLE),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMECOLOUR),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_STSIZETEXT),expert?SW_HIDE:SW_SHOW);
	ShowWindow(GetDlgItem(hwndDlg,IDC_STCOLOURTEXT),expert?SW_HIDE:SW_SHOW);
	SetDlgItemTextA(hwndDlg,IDC_STASTEXT,Translate(expert?"as:":"based on:"));
	{	UTILRESIZEDIALOG urd={0};
		urd.cbSize=sizeof(urd);
		urd.hwndDlg=hwndDlg;
		urd.hInstance=g_hInst;
		urd.lpTemplate=MAKEINTRESOURCEA(IDD_OPT_MSGWINDOWFONTS);
		urd.pfnResizer=TextOptsDlgResizer;
		CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
	}
	//resizer breaks the sizing of the edit box
	SendMessageA(hwndDlg,M_REFRESHSAMEASBOXES,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0),0);
}
    

static void GetDefaultFontSetting(int i,LOGFONTA *lf,COLORREF *colour)
{
	SystemParametersInfoA(SPI_GETICONTITLELOGFONT,sizeof(LOGFONTA),lf,FALSE);
	*colour=GetSysColor(COLOR_WINDOWTEXT);
	switch(i) {
		case FONTID_GROUPS:
			lf->lfWeight=FW_BOLD;
			break;
		case FONTID_GROUPCOUNTS:
			lf->lfHeight=(int)(lf->lfHeight*.75);
			*colour=GetSysColor(COLOR_3DSHADOW);
			break;
		case FONTID_OFFINVIS:
		case FONTID_INVIS:
			lf->lfItalic=!lf->lfItalic;
			break;
		case FONTID_DIVIDERS:
			lf->lfHeight=(int)(lf->lfHeight*.75);
			break;
		case FONTID_NOTONLIST:
			*colour=GetSysColor(COLOR_3DSHADOW);
			break;
	}
}

#define SRFONTSETTINGMODULE "Tab_SRMsg"

void GetFontSetting(int i,LOGFONTA *lf,COLORREF *colour)
{
	DBVARIANT dbv;
	char idstr[10];
	BYTE style;

	GetDefaultFontSetting(i,lf,colour);
	sprintf(idstr,"Font%d",i);
	if(!DBGetContactSetting(NULL,SRFONTSETTINGMODULE,idstr,&dbv)) {
		lstrcpyA(lf->lfFaceName,dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	sprintf(idstr,"Font%dCol",i);
	*colour=DBGetContactSettingDword(NULL,SRFONTSETTINGMODULE,idstr,*colour);
	sprintf(idstr,"Font%dSize",i);
	lf->lfHeight=(char)DBGetContactSettingByte(NULL,SRFONTSETTINGMODULE,idstr,lf->lfHeight);
	sprintf(idstr,"Font%dSty",i);
	style=(BYTE)DBGetContactSettingByte(NULL,SRFONTSETTINGMODULE,idstr,(lf->lfWeight==FW_NORMAL?0:DBFONTF_BOLD)|(lf->lfItalic?DBFONTF_ITALIC:0)|(lf->lfUnderline?DBFONTF_UNDERLINE:0));
	lf->lfWidth=lf->lfEscapement=lf->lfOrientation=0;
	lf->lfWeight=style&DBFONTF_BOLD?FW_BOLD:FW_NORMAL;
	lf->lfItalic=(style&DBFONTF_ITALIC)!=0;
	lf->lfUnderline=(style&DBFONTF_UNDERLINE)!=0;
	lf->lfStrikeOut=0;
	sprintf(idstr,"Font%dSet",i);
	lf->lfCharSet=DBGetContactSettingByte(NULL,SRFONTSETTINGMODULE,idstr,lf->lfCharSet);
	lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf->lfQuality=DEFAULT_QUALITY;
	lf->lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
}

static BOOL CALLBACK DlgProcMsgWindowFonts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFontSample;

	switch (msg)
	{
		case WM_INITDIALOG:
            hFontSample=NULL;
			SetDlgItemTextA(hwndDlg,IDC_SAMPLE,"Sample");
			TranslateDialogDefault(hwndDlg);
			if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0))
				SwitchTextDlgToMode(hwndDlg,0);
				FillFontListThread(hwndDlg);
			{	int i,itemId,fontId;
				LOGFONTA lf;
				COLORREF colour;
				WORD sameAs;
				char str[32];
                DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);

				for(i=0;i<FONTS_TO_CONFIG;i++) {
					fontId=fontListOrder[i];
					GetFontSetting(fontId,&lf,&colour);
					sprintf(str,"Font%dAs",fontId);
					sameAs=DBGetContactSettingWord(NULL,SRFONTSETTINGMODULE,str,fontSameAsDefault[fontId]);
					fontSettings[fontId].sameAsFlags=HIBYTE(sameAs);
					fontSettings[fontId].sameAs=LOBYTE(sameAs);
					fontSettings[fontId].style=(lf.lfWeight==FW_NORMAL?0:DBFONTF_BOLD)|(lf.lfItalic?DBFONTF_ITALIC:0)|(lf.lfUnderline?DBFONTF_UNDERLINE:0);
					if(lf.lfHeight<0) {
						HDC hdc;
						SIZE size;
						HFONT hFont=CreateFontIndirectA(&lf);
						hdc=GetDC(hwndDlg);
						SelectObject(hdc,hFont);
						GetTextExtentPoint32A(hdc,"x",1,&size);
						ReleaseDC(hwndDlg,hdc);
						DeleteObject(hFont);
						fontSettings[fontId].size=(char)size.cy;
					}
					else fontSettings[fontId].size=(char)lf.lfHeight;
					fontSettings[fontId].charset=lf.lfCharSet;
					fontSettings[fontId].colour=colour;
					lstrcpyA(fontSettings[fontId].szFace,lf.lfFaceName);
					itemId=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_ADDSTRING,0,(LPARAM)Translate(szFontIdDescr[fontId]));
					SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_SETITEMDATA,itemId,fontId);
				}
                
                SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_INPUTBKG, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "inputbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
                SendDlgItemMessage(hwndDlg, IDC_INPUTBKG, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);

                SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "outbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "inbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
                SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
                CheckDlgButton(hwndDlg, IDC_USEINDIVIDUALBKG, dwFlags & MWF_LOG_INDIVIDUALBKG);
                EnableWindow(GetDlgItem(hwndDlg, IDC_BKGOUTGOING), dwFlags & MWF_LOG_INDIVIDUALBKG);
                EnableWindow(GetDlgItem(hwndDlg, IDC_BKGINCOMING), dwFlags & MWF_LOG_INDIVIDUALBKG);
                
				SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_SETCURSEL,0,0);
				for(i=0;i<sizeof(fontSizes)/sizeof(fontSizes[0]);i++)
					SendDlgItemMessageA(hwndDlg,IDC_FONTSIZE,CB_ADDSTRING,0,(LPARAM)fontSizes[i]);
			}
            SendMessage(hwndDlg,M_REBUILDFONTGROUP,0,0);
			SendMessage(hwndDlg,M_SAVEFONT,0,0);
			return TRUE;
		case M_REBUILDFONTGROUP:	//remake all the needed controls when the user changes the font selector at the top
		{	int i=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0);
			SendMessage(hwndDlg,M_SETSAMEASBOXES,i,0);
			{	int j,id,itemId;
				char szText[256];
				SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_RESETCONTENT,0,0);
				itemId=SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_ADDSTRING,0,(LPARAM)Translate("<none>"));
				SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETITEMDATA,itemId,0xFF);
				if(0xFF==fontSettings[i].sameAs)
					SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,itemId,0);
				for(j=0;j<FONTS_TO_CONFIG;j++) {
					SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETLBTEXT,j,(LPARAM)szText);
					id=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,j,0);
					if(id==i) continue;
					itemId=SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_ADDSTRING,0,(LPARAM)szText);
					SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETITEMDATA,itemId,id);
					if(id==fontSettings[i].sameAs)
						SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,itemId,0);
				}
			}
			SendMessage(hwndDlg,M_LOADFONT,i,0);
			SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,i,0);
			SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
			break;
		}
		case M_SETSAMEASBOXES:	//set the check mark in the 'same as' boxes to the right value for fontid wParam
			CheckDlgButton(hwndDlg,IDC_SAMETYPE,fontSettings[wParam].sameAsFlags&SAMEASF_FACE?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SAMESIZE,fontSettings[wParam].sameAsFlags&SAMEASF_SIZE?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SAMESTYLE,fontSettings[wParam].sameAsFlags&SAMEASF_STYLE?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SAMECOLOUR,fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR?BST_CHECKED:BST_UNCHECKED);
			break;
		case M_FILLSCRIPTCOMBO:		  //fill the script combo box and set the selection to the value for fontid wParam
		{	LOGFONTA lf={0};
			int i;
			HDC hdc=GetDC(hwndDlg);
			lf.lfCharSet=DEFAULT_CHARSET;
			GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
			lf.lfPitchAndFamily=0;
			SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_RESETCONTENT,0,0);
			EnumFontFamiliesExA(hdc,&lf,(FONTENUMPROCA)EnumFontScriptsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_SCRIPT),0);
			ReleaseDC(hwndDlg,hdc);
			for(i=SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETCOUNT,0,0)-1;i>=0;i--) {
				if(SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,i,0)==fontSettings[wParam].charset) {
					SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_SETCURSEL,i,0);
					break;
				}
			}
			if(i<0) SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_SETCURSEL,0,0);
			break;
		}
		case WM_CTLCOLORSTATIC:
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_SAMPLE)) {
				SetTextColor((HDC)wParam,SendDlgItemMessageA(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
                SetBkColor((HDC)wParam,GetSysColor(COLOR_3DFACE));
				return (BOOL)GetSysColorBrush(COLOR_3DFACE);
			}
			break;
		case M_REFRESHSAMEASBOXES:		  //set the disabled flag on the 'same as' checkboxes to the values for fontid wParam
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMETYPE),fontSettings[wParam].sameAs!=0xFF);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMESIZE),fontSettings[wParam].sameAs!=0xFF);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMESTYLE),fontSettings[wParam].sameAs!=0xFF);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMECOLOUR),fontSettings[wParam].sameAs!=0xFF);
			if(SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
				EnableWindow(GetDlgItem(hwndDlg,IDC_TYPEFACE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_FACE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_SCRIPT),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_FACE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_FONTSIZE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_SIZE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_BOLD),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_ITALIC),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_UNDERLINE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
                EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR));
            }
			else {
				EnableWindow(GetDlgItem(hwndDlg,IDC_TYPEFACE),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SCRIPT),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_FONTSIZE),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_BOLD),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_ITALIC),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_UNDERLINE),TRUE);
                EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),TRUE);
			}
			break;
		case M_REMAKESAMPLE:	//remake the sample edit box font based on the settings in the controls
		{	LOGFONTA lf;
            HDC hdc = GetDC(NULL);
			if(hFontSample) {
				SendDlgItemMessageA(hwndDlg,IDC_SAMPLE,WM_SETFONT,SendDlgItemMessageA(hwndDlg,IDC_FONTID,WM_GETFONT,0,0),0);
				DeleteObject(hFontSample);
			}
			lf.lfHeight=GetDlgItemInt(hwndDlg,IDC_FONTSIZE,NULL,FALSE);
            
            lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ReleaseDC(NULL,hdc);				
			lf.lfWidth=lf.lfEscapement=lf.lfOrientation=0;
			lf.lfWeight=IsDlgButtonChecked(hwndDlg,IDC_BOLD)?FW_BOLD:FW_NORMAL;
			lf.lfItalic=IsDlgButtonChecked(hwndDlg,IDC_ITALIC);
			lf.lfUnderline=IsDlgButtonChecked(hwndDlg,IDC_UNDERLINE);
			lf.lfStrikeOut=0;
			lf.lfCharSet=(BYTE)SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETCURSEL,0,0),0);
			lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
			lf.lfQuality=DEFAULT_QUALITY;
			lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
			GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
			hFontSample=CreateFontIndirectA(&lf);
			SendDlgItemMessage(hwndDlg,IDC_SAMPLE,WM_SETFONT,(WPARAM)hFontSample,TRUE);
			break;
		}
		case M_RECALCONEFONT:	   //copy the 'same as' settings for fontid wParam from their sources
			if(fontSettings[wParam].sameAs==0xFF) break;
			if(fontSettings[wParam].sameAsFlags&SAMEASF_FACE) {
				lstrcpyA(fontSettings[wParam].szFace,fontSettings[fontSettings[wParam].sameAs].szFace);
				fontSettings[wParam].charset=fontSettings[fontSettings[wParam].sameAs].charset;
			}
			if(fontSettings[wParam].sameAsFlags&SAMEASF_SIZE)
				fontSettings[wParam].size=fontSettings[fontSettings[wParam].sameAs].size;
			if(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE)
				fontSettings[wParam].style=fontSettings[fontSettings[wParam].sameAs].style;
			if(fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR)
				fontSettings[wParam].colour=fontSettings[fontSettings[wParam].sameAs].colour;
			break;
		case M_RECALCOTHERFONTS:	//recalculate the 'same as' settings for all fonts but wParam
		{	int i;
			for(i=0;i<FONTS_TO_CONFIG;i++) {
				if(i==(int)wParam) continue;
				SendMessage(hwndDlg,M_RECALCONEFONT,i,0);
			}
			break;
		}
        case M_SAVEFONT:	//save the font settings from the controls to font wParam
			fontSettings[wParam].sameAsFlags=(IsDlgButtonChecked(hwndDlg,IDC_SAMETYPE)?SAMEASF_FACE:0)|(IsDlgButtonChecked(hwndDlg,IDC_SAMESIZE)?SAMEASF_SIZE:0)|(IsDlgButtonChecked(hwndDlg,IDC_SAMESTYLE)?SAMEASF_STYLE:0)|(IsDlgButtonChecked(hwndDlg,IDC_SAMECOLOUR)?SAMEASF_COLOUR:0);
			fontSettings[wParam].sameAs=(BYTE)SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0);
			GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,fontSettings[wParam].szFace,sizeof(fontSettings[wParam].szFace));
			fontSettings[wParam].charset=(BYTE)SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETCURSEL,0,0),0);
			fontSettings[wParam].size=(char)GetDlgItemInt(hwndDlg,IDC_FONTSIZE,NULL,FALSE);
			fontSettings[wParam].style=(IsDlgButtonChecked(hwndDlg,IDC_BOLD)?DBFONTF_BOLD:0)|(IsDlgButtonChecked(hwndDlg,IDC_ITALIC)?DBFONTF_ITALIC:0)|(IsDlgButtonChecked(hwndDlg,IDC_UNDERLINE)?DBFONTF_UNDERLINE:0);
            fontSettings[wParam].colour=SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0);
			SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);
			break;
		case M_REDOROWHEIGHT:	//recalculate the minimum feasible row height
		{	int i;
			int minHeight=1;//GetSystemMetrics(SM_CYSMICON);
			for(i=0;i<FONTS_TO_CONFIG;i++)
				if(fontSettings[i].size+2>minHeight) minHeight=fontSettings[i].size+2;
			break;
		}
		case M_LOADFONT:	//load font wParam into the controls
			SetDlgItemTextA(hwndDlg,IDC_TYPEFACE,fontSettings[wParam].szFace);
			SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,wParam,0);
			SetDlgItemInt(hwndDlg,IDC_FONTSIZE,fontSettings[wParam].size,FALSE);
			CheckDlgButton(hwndDlg,IDC_BOLD,fontSettings[wParam].style&DBFONTF_BOLD?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_ITALIC,fontSettings[wParam].style&DBFONTF_ITALIC?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_UNDERLINE,fontSettings[wParam].style&DBFONTF_UNDERLINE?BST_CHECKED:BST_UNCHECKED);
			{	LOGFONTA lf;
				COLORREF colour;
				GetDefaultFontSetting(wParam,&lf,&colour);
                SendDlgItemMessageA(hwndDlg,IDC_COLOUR,CPM_SETDEFAULTCOLOUR,0,colour);
			}
			SendDlgItemMessageA(hwndDlg,IDC_COLOUR,CPM_SETCOLOUR,0,fontSettings[wParam].colour);
			break;
		case M_GUESSSAMEASBOXES:   //guess suitable values for the 'same as' checkboxes for fontId wParam
			fontSettings[wParam].sameAsFlags=0;
			if(fontSettings[wParam].sameAs==0xFF) break;
			if(!strcmp(fontSettings[wParam].szFace,fontSettings[fontSettings[wParam].sameAs].szFace) &&
			   fontSettings[wParam].charset==fontSettings[fontSettings[wParam].sameAs].charset)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_FACE;
			if(fontSettings[wParam].size==fontSettings[fontSettings[wParam].sameAs].size)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_SIZE;
			if(fontSettings[wParam].style==fontSettings[fontSettings[wParam].sameAs].style)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_STYLE;
			if(fontSettings[wParam].colour==fontSettings[fontSettings[wParam].sameAs].colour)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_COLOUR;
			SendMessage(hwndDlg,M_SETSAMEASBOXES,wParam,0);
			break;
		case WM_VSCROLL:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_COMMAND:
		{	int fontId=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0);
			switch(LOWORD(wParam)) {
				case IDC_FONTID:
					if(HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
					SendMessage(hwndDlg,M_REBUILDFONTGROUP,0,0);
					return 0;
				case IDC_SAMETYPE:
				case IDC_SAMESIZE:
				case IDC_SAMESTYLE:
				case IDC_SAMECOLOUR:
					SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
					SendMessage(hwndDlg,M_RECALCONEFONT,fontId,0);
					SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
					SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
					break;
				case IDC_SAMEAS:
					if(HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
					if(SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0)==fontId)
						SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,0,0);
					if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
						int sameAs=SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0);
						if(sameAs!=0xFF) SendMessage(hwndDlg,M_LOADFONT,sameAs,0);
						SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
						SendMessage(hwndDlg,M_GUESSSAMEASBOXES,fontId,0);
					}
					else SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
					SendMessage(hwndDlg,M_RECALCONEFONT,fontId,0);
					SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,fontId,0);
					SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
					SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
					break;
				case IDC_TYPEFACE:
				case IDC_SCRIPT:
				case IDC_FONTSIZE:
					if(HIWORD(wParam)!=CBN_EDITCHANGE && HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
					if(HIWORD(wParam)==CBN_SELCHANGE) {
						SendDlgItemMessageA(hwndDlg,LOWORD(wParam),CB_SETCURSEL,SendDlgItemMessageA(hwndDlg,LOWORD(wParam),CB_GETCURSEL,0,0),0);
					}
					if(LOWORD(wParam)==IDC_TYPEFACE)
						SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,fontId,0);
					//fall through
				case IDC_BOLD:
				case IDC_ITALIC:
                case IDC_UNDERLINE:
                case IDC_COLOUR:
					SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
					if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
						SendMessage(hwndDlg,M_GUESSSAMEASBOXES,fontId,0);
						SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
					}
					SendMessage(hwndDlg,M_RECALCOTHERFONTS,fontId,0);
					SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
					SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);
					break;
                case IDC_USEINDIVIDUALBKG:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BKGOUTGOING), IsDlgButtonChecked(hwndDlg, IDC_USEINDIVIDUALBKG));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BKGINCOMING), IsDlgButtonChecked(hwndDlg, IDC_USEINDIVIDUALBKG));
                    break;
				case IDC_SAMPLE:
					return 0;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
							{	int i;
								char str[20];
                                DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);

								for(i=0;i<FONTS_TO_CONFIG;i++) {
									sprintf(str,"Font%d",i);
									DBWriteContactSettingString(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].szFace);
									sprintf(str,"Font%dSet",i);
									DBWriteContactSettingByte(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].charset);
									sprintf(str,"Font%dSize",i);
									DBWriteContactSettingByte(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].size);
									sprintf(str,"Font%dSty",i);
									DBWriteContactSettingByte(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].style);
									sprintf(str,"Font%dCol",i);
									DBWriteContactSettingDword(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].colour);
									sprintf(str,"Font%dAs",i);
									DBWriteContactSettingWord(NULL,SRFONTSETTINGMODULE,str,(WORD)((fontSettings[i].sameAsFlags<<8)|fontSettings[i].sameAs));
								}
                                dwFlags = IsDlgButtonChecked(hwndDlg, IDC_USEINDIVIDUALBKG) ? dwFlags | MWF_LOG_INDIVIDUALBKG : dwFlags & ~MWF_LOG_INDIVIDUALBKG;
                                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dwFlags);
                                DBWriteContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "inputbg", SendDlgItemMessage(hwndDlg, IDC_INPUTBKG, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "inbg", SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "outbg", SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_GETCOLOUR, 0, 0));
                                UncacheMsgLogIcons();
                                CacheMsgLogIcons();
                                CacheLogFonts();
                                WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
                         }
							return TRUE;
						case PSN_EXPERTCHANGED:
							SwitchTextDlgToMode(hwndDlg,((PSHNOTIFY*)lParam)->lParam);
							break;
					}
					break;
			}
			break;
		case WM_DESTROY:
			if(hFontSample) {
				SendDlgItemMessageA(hwndDlg,IDC_SAMPLE,WM_SETFONT,SendDlgItemMessageA(hwndDlg,IDC_FONTID,WM_GETFONT,0,0),0);
				DeleteObject(hFontSample);
			}
			break;
	}
	return FALSE;
}
    
static int OptInitialise(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp = { 0 };

    odp.cbSize = sizeof(odp);
    odp.position = 910000000;
    odp.hInstance = g_hInst;
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGDLG);
    odp.pszTitle = Translate("Messaging");
    odp.pszGroup = Translate("Message Sessions");
    odp.pfnDlgProc = DlgProcOptions;
    odp.flags = ODPF_BOLDGROUPS;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGLOG);
    odp.pszTitle = Translate("Messaging Log");
    odp.pfnDlgProc = DlgProcLogOptions;
    odp.nIDBottomSimpleControl = IDC_STMSGLOGGROUP;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGTYPE);
    odp.pszTitle = Translate("Typing Notify");
    odp.pfnDlgProc = DlgProcTypeOptions;
    odp.nIDBottomSimpleControl = 0;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_TABBEDMSG);
	odp.pszTitle = Translate("Message tabs");
	odp.pfnDlgProc = DlgProcTabbedOptions;
	odp.nIDBottomSimpleControl = 0;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_CONTAINERS);
	odp.pszTitle = Translate("Message containers");
	odp.pfnDlgProc = DlgProcContainerOptions;
	odp.nIDBottomSimpleControl = 0;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGWINDOWFONTS);
    odp.pszTitle = Translate("Fonts and colors");
    odp.pfnDlgProc = DlgProcMsgWindowFonts;
    odp.nIDBottomSimpleControl = 0;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
    
    return 0;
}

int InitOptions(void)
{
    HookEvent(ME_OPT_INITIALISE, OptInitialise);
    return 0;
}

static BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStatusMask = DBGetContactSettingDword(NULL, SRMSGMOD_T, "autopopupmask", -1);
    static DWORD dwNewStatusMask = 0;
    
    switch (msg) {
        case WM_INITDIALOG:
        {
            int i;
            
            TranslateDialogDefault(hwndDlg);
            SetWindowTextA(hwndDlg, Translate("Choose status modes"));
            for(i = ID_STATUS_ONLINE; i <= ID_STATUS_OUTTOLUNCH; i++) {
                SetWindowTextA(GetDlgItem(hwndDlg, i), Translate((char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)i, 0)));
                if(dwStatusMask != -1 && (dwStatusMask & (1<<(i - ID_STATUS_ONLINE))))
                    CheckDlgButton(hwndDlg, i, TRUE);
                EnableWindow(GetDlgItem(hwndDlg, i), dwStatusMask != -1);
            }
            if(dwStatusMask == -1)
                CheckDlgButton(hwndDlg, IDC_ALWAYS, TRUE);
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            return TRUE;
        }
        case DM_GETSTATUSMASK:
            {
                if(IsDlgButtonChecked(hwndDlg, IDC_ALWAYS))
                    dwNewStatusMask = -1;
                else {
                    int i;
                    dwNewStatusMask = 0;
                    for(i = ID_STATUS_ONLINE; i <= ID_STATUS_OUTTOLUNCH; i++)
                        dwNewStatusMask |= (IsDlgButtonChecked(hwndDlg, i) ? (1<<(i - ID_STATUS_ONLINE)) : 0);
                }
                break;
            }
        case WM_COMMAND:
            {
                switch (LOWORD(wParam)) {
                    case IDOK:
                    case IDCANCEL:
                        if(LOWORD(wParam) == IDOK) {
                            SendMessage(hwndDlg, DM_GETSTATUSMASK, 0, 0);
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "autopopupmask", dwNewStatusMask);
                        }
                        DestroyWindow(hwndDlg);
                        break;
                    case IDC_ALWAYS:
                        {
                        int i;
                        for(i = ID_STATUS_ONLINE; i <= ID_STATUS_OUTTOLUNCH; i++)
                            EnableWindow(GetDlgItem(hwndDlg, i), !IsDlgButtonChecked(hwndDlg, IDC_ALWAYS));
                        break;
                        }
                    default:
                        break;
                }
            }
        default:
            break;
    }
    return FALSE;
}

