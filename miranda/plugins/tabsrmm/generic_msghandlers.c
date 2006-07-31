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
 * also contains various callback functions for custom buttons
 */

#include "commonheaders.h"

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern StatusItems_t StatusItems[];

extern WCHAR    *FilterEventMarkers(WCHAR *wszText);
extern char     *FilterEventMarkersA(char *szText);

extern PITA pfnIsThemeActive;
extern POTD pfnOpenThemeData;
extern PDTB pfnDrawThemeBackground;
extern PCTD pfnCloseThemeData;
extern PDTT pfnDrawThemeText;
extern PITBPT pfnIsThemeBackgroundPartiallyTransparent;
extern PDTPB  pfnDrawThemeParentBackground;
extern PGTBCR pfnGetThemeBackgroundContentRect;

/* 
 * action and callback procedures for the stock button objects
 */

static void BTN_StockAction(ButtonItem *item, HWND hwndDlg, struct MessageWindowData *dat, HWND hwndBtn)
{
    if(item->dwStockFlags & SBI_HANDLEBYCLIENT && IsWindow(hwndDlg) && dat)
        SendMessage(hwndDlg, WM_COMMAND, MAKELONG(item->uId, BN_CLICKED), (LPARAM)hwndBtn);
    else {
        switch(item->uId) {
            case IDC_SBAR_CANCEL:
                PostMessage(hwndDlg, WM_COMMAND, MAKELONG(IDC_SAVE, BN_CLICKED), (LPARAM)hwndBtn);
                break;
            case IDC_SBAR_SLIST:
                SendMessage(myGlobals.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_LBUTTONUP);
                break;
            case IDC_SBAR_FAVORITES:
            {
                POINT pt;
                int iSelection;
                GetCursorPos(&pt);
                iSelection = TrackPopupMenu(myGlobals.g_hMenuFavorites, TPM_RETURNCMD, pt.x, pt.y, 0, myGlobals.g_hwndHotkeyHandler, NULL);
                HandleMenuEntryFromhContact(iSelection);
                break;
            }
            case IDC_SBAR_RECENT:
            {
                POINT pt;
                int iSelection;
                GetCursorPos(&pt);
                iSelection = TrackPopupMenu(myGlobals.g_hMenuRecent, TPM_RETURNCMD, pt.x, pt.y, 0, myGlobals.g_hwndHotkeyHandler, NULL);
                HandleMenuEntryFromhContact(iSelection);
                break;
            }
            case IDC_SBAR_USERPREFS:
            {
                HANDLE hContact = 0;
                SendMessage(hwndDlg, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                if(hContact != 0)
                    CallService(MS_TABMSG_SETUSERPREFS, (WPARAM)hContact, 0);
                break;
            }
            case IDC_SBAR_TOGGLEFORMAT:
            {
                if(dat) {
                    if(IsDlgButtonChecked(hwndDlg, IDC_SBAR_TOGGLEFORMAT) == BST_UNCHECKED) {
                        dat->SendFormat = 0;
                        GetSendFormat(hwndDlg, dat, 0);
                    }
                    else {
                        dat->SendFormat = SENDFORMAT_BBCODE;
                        GetSendFormat(hwndDlg, dat, 0);
                    }
                }
                break;
            }
        }
    }
}

static void BTN_StockCallback(ButtonItem *item, HWND hwndDlg, struct MessageWindowData *dat, HWND hwndBtn)
{
}

/*
 * predefined button objects for customizeable buttons
 */

static struct SIDEBARITEM sbarItems[] = {
    IDC_SBAR_SLIST, SBI_TOP, &myGlobals.g_sideBarIcons[0], &myGlobals.g_sideBarIcons[0], &myGlobals.g_sideBarIcons[0], "t_slist", BTN_StockAction, BTN_StockCallback, _T("Open session list"),
    IDC_SBAR_FAVORITES, SBI_TOP, &myGlobals.g_sideBarIcons[1], &myGlobals.g_sideBarIcons[1], &myGlobals.g_sideBarIcons[1], "t_fav", BTN_StockAction, BTN_StockCallback, _T("Open favorites"),
    IDC_SBAR_RECENT, SBI_TOP, &myGlobals.g_sideBarIcons[2],  &myGlobals.g_sideBarIcons[2], &myGlobals.g_sideBarIcons[2], "t_recent", BTN_StockAction, BTN_StockCallback, _T("Open recent contacts"),
    IDC_SBAR_USERPREFS, SBI_TOP, &myGlobals.g_sideBarIcons[4], &myGlobals.g_sideBarIcons[4], &myGlobals.g_sideBarIcons[4], "t_prefs", BTN_StockAction, BTN_StockCallback, _T("Contact preferences"),
    IDC_SBAR_TOGGLEFORMAT, SBI_TOP | SBI_TOGGLE, &myGlobals.g_buttonBarIcons[20], &myGlobals.g_buttonBarIcons[20], &myGlobals.g_buttonBarIcons[20], "t_tformat", BTN_StockAction, BTN_StockCallback, _T("Formatting"),
    IDC_SBAR_SETUP, SBI_BOTTOM, &myGlobals.g_sideBarIcons[3], &myGlobals.g_sideBarIcons[3], &myGlobals.g_sideBarIcons[3], "t_setup", BTN_StockAction, BTN_StockCallback, _T("Miranda options"),
    IDOK, SBI_TOP | SBI_HANDLEBYCLIENT, &myGlobals.g_buttonBarIcons[9], &myGlobals.g_buttonBarIcons[9], &myGlobals.g_buttonBarIcons[9], "t_send", BTN_StockAction, BTN_StockCallback, _T("Send message"),
    IDC_SBAR_CANCEL, SBI_TOP, &myGlobals.g_buttonBarIcons[6], &myGlobals.g_buttonBarIcons[6], &myGlobals.g_buttonBarIcons[6], "t_close", BTN_StockAction, BTN_StockCallback, _T("Close session"),
    IDC_SMILEYBTN, SBI_TOP | SBI_HANDLEBYCLIENT, &myGlobals.g_buttonBarIcons[11], &myGlobals.g_buttonBarIcons[11], &myGlobals.g_buttonBarIcons[11], "t_emoticon", BTN_StockAction, BTN_StockCallback, _T("Emoticon"),
    IDC_NAME, SBI_TOP | SBI_HANDLEBYCLIENT, &myGlobals.g_buttonBarIcons[16], &myGlobals.g_buttonBarIcons[16], &myGlobals.g_buttonBarIcons[16], "t_menu", BTN_StockAction, BTN_StockCallback, _T("User menu"),
    IDC_PROTOCOL, SBI_TOP | SBI_HANDLEBYCLIENT, &myGlobals.g_buttonBarIcons[4], &myGlobals.g_buttonBarIcons[4], &myGlobals.g_buttonBarIcons[4], "t_details", BTN_StockAction, BTN_StockCallback, _T("User details"),
    0, 0, 0, 0, 0, "", NULL, NULL, _T("")
};

int BTN_GetStockItem(ButtonItem *item, const char *szName)
{
    int i = 0;

    while(sbarItems[i].uId) {
        if(!stricmp(sbarItems[i].szName, szName)) {
            item->uId = sbarItems[i].uId;
            //item->dwFlags |= BUTTON_ISSIDEBAR;
            //myGlobals.m_SideBarEnabled = TRUE;
            if(item->dwFlags & BUTTON_ISSIDEBAR) {
                if(sbarItems[i].dwFlags & SBI_TOP)
                    item->yOff = 0;
                else if(sbarItems[i].dwFlags & SBI_BOTTOM)
                    item->yOff = -1;
            }
            item->dwStockFlags = sbarItems[i].dwFlags;
            item->dwFlags = sbarItems[i].dwFlags & SBI_TOGGLE ? item->dwFlags | BUTTON_ISTOGGLE : item->dwFlags & ~BUTTON_ISTOGGLE;
            item->pfnAction = sbarItems[i].pfnAction;
            item->pfnCallback = sbarItems[i].pfnCallback;
            lstrcpyn(item->szTip, sbarItems[i].tszTip, 256);
            item->szTip[255] = 0;
            if(sbarItems[i].hIcon) {
                item->normalGlyphMetrics[0] = (LONG)sbarItems[i].hIcon;
                item->dwFlags |= BUTTON_NORMALGLYPHISICON;
            }
            if(sbarItems[i].hIconPressed) {
                item->pressedGlyphMetrics[0] = (LONG)sbarItems[i].hIconPressed;
                item->dwFlags |= BUTTON_PRESSEDGLYPHISICON;
            }
            if(sbarItems[i].hIconHover) {
                item->hoverGlyphMetrics[0] = (LONG)sbarItems[i].hIconHover;
                item->dwFlags |= BUTTON_HOVERGLYPHISICON;
            }
            return 1;
        }
        i++;
    }
    return 0;
}

/*                                                              
 * set the states of defined database action buttons (only if button is a toggle)
*/

void DM_SetDBButtonStates(HWND hwndChild, struct MessageWindowData *dat)
{
    ButtonItem *buttonItem = dat->pContainer->buttonItems;
    HANDLE hContact = dat->hContact, hFinalContact = 0;
    char *szModule, *szSetting;
    HWND hwndContainer = dat->pContainer->hwnd;;

    while(buttonItem) {
        BOOL result = FALSE;
        HWND hWnd = GetDlgItem(hwndContainer, buttonItem->uId);

        if(buttonItem->pfnCallback)
            buttonItem->pfnCallback(buttonItem, hwndChild, dat, hWnd);

        if(!(buttonItem->dwFlags & BUTTON_ISTOGGLE && buttonItem->dwFlags & BUTTON_ISDBACTION)) {
            buttonItem = buttonItem->nextItem;
            continue;
        }
        szModule = buttonItem->szModule;
        szSetting = buttonItem->szSetting;
        if(buttonItem->dwFlags & BUTTON_DBACTIONONCONTACT || buttonItem->dwFlags & BUTTON_ISCONTACTDBACTION) {
            if(hContact == 0) {
                SendMessage(hWnd, BM_SETCHECK, BST_UNCHECKED, 0);
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
        SendMessage(hWnd, BM_SETCHECK, (WPARAM)result, 0);
        buttonItem = buttonItem->nextItem;
    }
}

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
            SendMessage(hwndDlg, DM_SCROLLIEVIEW, 0, 0);
            SendMessage(hwndDlg, DM_SCROLLIEVIEW, 0, 0);
            return 0;
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

/*
 * create embedded contact list control
 */

HWND DM_CreateClist(HWND hwndParent, struct MessageWindowData *dat)
{
    HWND hwndClist = CreateWindowExA(0, "CListControl", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD | 0x248, 184, 0, 30, 30, hwndParent, (HMENU)IDC_CLIST, g_hInst, NULL);
    HANDLE hItem = (HANDLE) SendDlgItemMessage(hwndParent, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) dat->hContact, 0);

    SetWindowLong(hwndClist, GWL_EXSTYLE, GetWindowLong(hwndClist, GWL_EXSTYLE) & ~CLS_EX_TRACKSELECT);
    SetWindowLong(hwndClist, GWL_EXSTYLE, GetWindowLong(hwndClist, GWL_EXSTYLE) | (CLS_EX_NOSMOOTHSCROLLING | CLS_EX_NOTRANSLUCENTSEL));
    SetWindowLong(hwndClist, GWL_STYLE, GetWindowLong(hwndClist, GWL_STYLE) | CLS_HIDEOFFLINE);
    if (hItem)
        SendMessage(hwndClist, CLM_SETCHECKMARK, (WPARAM) hItem, 1);

    if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_DISABLEGROUPS && !DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
        SendMessage(hwndClist, CLM_SETUSEGROUPS, (WPARAM) FALSE, 0);
    else
        SendMessage(hwndClist, CLM_SETUSEGROUPS, (WPARAM) TRUE, 0);
    if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_HIDEEMPTYGROUPS && DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_USEGROUPS_DEFAULT))
        SendMessage(hwndClist, CLM_SETHIDEEMPTYGROUPS, (WPARAM) TRUE, 0);
    else
        SendMessage(hwndClist, CLM_SETHIDEEMPTYGROUPS, (WPARAM) FALSE, 0);
    SendMessage(hwndClist, CLM_FIRST + 106, 0, 1);
    //SendMessage(hwndClist, CLM_SETHIDEOFFLINEROOT, TRUE, 0);
    SendMessage(hwndClist, CLM_AUTOREBUILD, 0, 0);

    return hwndClist;
}

LRESULT DM_MouseWheelHandler(HWND hwnd, HWND hwndParent, struct MessageWindowData *mwdat, WPARAM wParam, LPARAM lParam)
{
    RECT rc, rc1;
    POINT pt;
    TCHITTESTINFO hti;
    HWND hwndTab;
    UINT uID = mwdat->bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG;

    GetCursorPos(&pt);
    GetWindowRect(hwnd, &rc);
    if(PtInRect(&rc, pt))
        return 1;
    if(mwdat->pContainer->dwFlags & CNT_SIDEBAR) {
        GetWindowRect(GetDlgItem(mwdat->pContainer->hwnd, IDC_SIDEBARUP), &rc);
        GetWindowRect(GetDlgItem(mwdat->pContainer->hwnd, IDC_SIDEBARDOWN), &rc1);
        rc.bottom = rc1.bottom;
        if(PtInRect(&rc, pt)) {
            short amount = (short)(HIWORD(wParam));
            SendMessage(mwdat->pContainer->hwnd, WM_COMMAND, MAKELONG(amount > 0 ? IDC_SIDEBARUP : IDC_SIDEBARDOWN, 0), 0);
            return 0;
        }
    }
    if(mwdat->hwndIEView)
        GetWindowRect(mwdat->hwndIEView, &rc);
    else
        GetWindowRect(GetDlgItem(hwndParent, uID), &rc);
    if(PtInRect(&rc, pt)) {
        HWND hwnd = mwdat->hwndIEView ? mwdat->hwndIWebBrowserControl : GetDlgItem(hwndParent, uID);
        short wDirection = (short)HIWORD(wParam);

        if(hwnd == 0)
            hwnd = WindowFromPoint(pt);

        if(LOWORD(wParam) & MK_SHIFT || DBGetContactSettingByte(NULL, SRMSGMOD_T, "fastscroll", 0)) {
            if(wDirection < 0)
                SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
            else if(wDirection > 0)
                SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
        }
        else
            SendMessage(hwnd, WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    }
    hwndTab = GetDlgItem(mwdat->pContainer->hwnd, IDC_MSGTABS);
    GetCursorPos(&hti.pt);
    ScreenToClient(hwndTab, &hti.pt);
    hti.flags = 0;
    if(TabCtrl_HitTest(hwndTab, &hti) != -1) {
        SendMessage(hwndTab, WM_MOUSEWHEEL, wParam, -1);
        return 0;
    }
    return 1;
}

LRESULT DM_ThemeChanged(HWND hwnd, struct MessageWindowData *dat)
{
    StatusItems_t *item_log = &StatusItems[ID_EXTBKHISTORY];
    StatusItems_t *item_msg = &StatusItems[ID_EXTBKINPUTAREA];

    dat->bFlatMsgLog = DBGetContactSettingByte(NULL, SRMSGMOD_T, "flatlog", 0);

    if(!dat->bFlatMsgLog)
        dat->hTheme = (myGlobals.m_VSApiEnabled && pfnOpenThemeData) ? pfnOpenThemeData(hwnd, L"EDIT") : 0;
    else
        dat->hTheme = 0;

    if(dat->bType == SESSIONTYPE_IM) {
        if(dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_log->IGNORED))
            SetWindowLong(GetDlgItem(hwnd, IDC_LOG), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwnd, IDC_LOG), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
        if(dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_msg->IGNORED))
            SetWindowLong(GetDlgItem(hwnd, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwnd, IDC_MESSAGE), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
    }
    else {
        if(dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_log->IGNORED)) {
            SetWindowLong(GetDlgItem(hwnd, IDC_CHAT_LOG), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwnd, IDC_CHAT_LOG), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
            SetWindowLong(GetDlgItem(hwnd, IDC_LIST), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwnd, IDC_LIST), GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        }
        if(dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_msg->IGNORED))
            SetWindowLong(GetDlgItem(hwnd, IDC_CHAT_MESSAGE), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwnd, IDC_CHAT_MESSAGE), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
    }
}
