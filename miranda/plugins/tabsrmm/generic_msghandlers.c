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
 * these are generic message handlers which are used by the message dialog window procedure.
 * calling them directly instead of using SendMessage() is faster.
 */

#include "commonheaders.h"

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;

extern WCHAR *FilterEventMarkers(WCHAR *wszText);
extern char  *FilterEventMarkersA(char *szText);

LRESULT DM_ScrollToBottom(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
    SCROLLINFO si = { 0 };

    if(dat == NULL)
        dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);

    if(dat) {

        if(dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED)
            return 0;

        if(IsIconic(dat->pContainer->hwnd))
            dat->dwFlags |= MWF_DEFERREDSCROLL;

        if(dat->hwndIEView) {
            dat->needIEViewScroll = TRUE;
            PostMessage(hwndDlg, DM_SCROLLIEVIEW, 0, 0);
        }
        else {
            HWND hwnd = GetDlgItem(hwndDlg, dat->bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG);

            if(lParam)
                SendMessage(hwnd, WM_SIZE, 0, 0);

            if(wParam == 1 && lParam == 1) {
                RECT rc;
                int len;

                GetClientRect(hwnd, &rc);
                len = GetWindowTextLengthA(hwnd);
                SendMessage(hwnd, EM_SETSEL, len - 1, len - 1);
            }
            if(wParam)
                SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
            else
                PostMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
            if(lParam)
                InvalidateRect(hwnd, NULL, FALSE);
        }
    }
    return 0;
}

LRESULT DM_LoadLocale(HWND hwndDlg, struct MessageWindowData *dat)
{
    /*
     * set locale if saved to contact
     */

    if(dat) {
        if(dat->dwFlags & MWF_WASBACKGROUNDCREATE)
            return 0;

        if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
            DBVARIANT dbv;
            int res;
            char szKLName[KL_NAMELENGTH+1];
            UINT flags = KLF_ACTIVATE;

            res = DBGetContactSetting(dat->hContact, SRMSGMOD_T, "locale", &dbv);
            if (res == 0 && dbv.type == DBVT_ASCIIZ) {

                dat->hkl = LoadKeyboardLayoutA(dbv.pszVal, KLF_ACTIVATE);
                PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                GetLocaleID(dat, dbv.pszVal);
                DBFreeVariant(&dbv);
            } else {
                GetKeyboardLayoutNameA(szKLName);
                dat->hkl = LoadKeyboardLayoutA(szKLName, 0);
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
                GetLocaleID(dat, szKLName);
            }
            UpdateReadChars(hwndDlg, dat);
        }
    }
    return 0;
}

LRESULT DM_RecalcPictureSize(HWND hwndDlg, struct MessageWindowData *dat)
{
    BITMAP bminfo;
    HBITMAP hbm;

    if(dat) {
        hbm = dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown);

        if(hbm == 0) {
            dat->pic.cy = dat->pic.cx = 60;
            return 0;
        }
        GetObject(hbm, sizeof(bminfo), &bminfo);
        CalcDynamicAvatarSize(hwndDlg, dat, &bminfo);
        if (myGlobals.g_FlashAvatarAvail) {
            RECT rc = { 0, 0, dat->pic.cx, dat->pic.cy };
            if(!(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)) {
                FLASHAVATAR fa = {0}; 

                fa.hContact = !(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) ? dat->hContact : NULL;
                fa.cProto = dat->szProto;
                fa.hWindow = 0;
                fa.id = 25367;
                CallService(MS_FAVATAR_RESIZE, (WPARAM)&fa, (LPARAM)&rc);				}
        }
        SendMessage(hwndDlg, WM_SIZE, 0, 0);
    }
    return 0;
}

LRESULT DM_UpdateLastMessage(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat) {
        if (dat->pContainer->hwndStatus == 0)
            return 0;
        if (dat->pContainer->hwndActive != hwndDlg)
            return 0;
        if(dat->showTyping) {
            TCHAR szBuf[80];

            mir_sntprintf(szBuf, safe_sizeof(szBuf), TranslateT("%s is typing..."), dat->szNickname);
            SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM) szBuf);
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) myGlobals.g_buttonBarIcons[5]);
            if(dat->pContainer->hwndSlist)
                SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[5]);
            return 0;
        }
        if (dat->lastMessage || dat->pContainer->dwFlags & CNT_UINSTATUSBAR) {
            DBTIMETOSTRINGT dbtts;
            TCHAR date[64], time[64];

            if(!(dat->pContainer->dwFlags & CNT_UINSTATUSBAR)) {
                dbtts.szFormat = _T("d");
                dbtts.cbDest = safe_sizeof(date);
                dbtts.szDest = date;
                CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dat->lastMessage, (LPARAM) & dbtts);
                if(dat->pContainer->dwFlags & CNT_UINSTATUSBAR && lstrlen(date) > 6)
                    date[lstrlen(date) - 5] = 0;
                dbtts.szFormat = _T("t");
                dbtts.cbDest = safe_sizeof(time);
                dbtts.szDest = time;
                CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dat->lastMessage, (LPARAM) & dbtts);
            }
            if(dat->pContainer->dwFlags & CNT_UINSTATUSBAR) {
                char fmt[100];
                mir_snprintf(fmt, sizeof(fmt), Translate("UIN: %s"), dat->uin);
                SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) fmt);
            }
            else {
                TCHAR fmt[100];
                mir_sntprintf(fmt, safe_sizeof(fmt), TranslateT("Last received: %s at %s"), date, time);
                SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM) fmt);
            }
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM)(nen_options.bFloaterInWin ? myGlobals.g_buttonBarIcons[16] : 0));
            if(dat->pContainer->hwndSlist)
                SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[16]);
        } else {
            SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) "");
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM)(nen_options.bFloaterInWin ? myGlobals.g_buttonBarIcons[16] : 0));
            if(dat->pContainer->hwndSlist)
                SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[16]);
        }
    }
    return 0;
}

LRESULT DM_SaveLocale(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
    if(dat) {
        if (myGlobals.m_AutoLocaleSupport && dat->hContact && dat->pContainer->hwndActive == hwndDlg) {
            char szKLName[KL_NAMELENGTH + 1];

            if((HKL)lParam != dat->hkl) {
                dat->hkl = (HKL)lParam;
                ActivateKeyboardLayout(dat->hkl, 0);
                GetKeyboardLayoutNameA(szKLName);
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
                GetLocaleID(dat, szKLName);
                UpdateReadChars(hwndDlg, dat);
            }
        }
    }
    return 0;
}

/*
 * generic handler for the WM_COPY message in message log/chat history richedit control(s).
 * it filters out the invisible event boundary markers from the text copied to the clipboard.
 */

LRESULT DM_WMCopyHandler(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = CallWindowProc(oldWndProc, hwnd, WM_COPY, wParam, lParam);

    if(OpenClipboard(hwnd)) {
#if defined(_UNICODE)
        HANDLE hClip = GetClipboardData(CF_UNICODETEXT);
#else
        HANDLE hClip = GetClipboardData(CF_TEXT);
#endif
        if(hClip) {
            HGLOBAL hgbl;
            TCHAR *tszLocked;
            TCHAR *tszText = (TCHAR *)malloc((lstrlen((TCHAR *)hClip) + 2) * sizeof(TCHAR));
    
            lstrcpy(tszText, (TCHAR *)hClip);
#if defined(_UNICODE)
            FilterEventMarkers(tszText);
#else
            FilterEventMarkersA(tszText);
#endif
            EmptyClipboard();
    
            hgbl = GlobalAlloc(GMEM_MOVEABLE, (lstrlen(tszText) + 1) * sizeof(TCHAR));
            tszLocked = GlobalLock(hgbl);
            lstrcpy(tszLocked, tszText);
            GlobalUnlock(hgbl);
#if defined(_UNICODE)
            SetClipboardData(CF_UNICODETEXT, hgbl);
#else
            SetClipboardData(CF_TEXT, hgbl);
#endif
            if(tszText)
                free(tszText);
        }
        CloseClipboard();
    }
    return result;
}

