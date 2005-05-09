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

The Hotkey-Handler is a hidden dialog window which needs to be in place for
handling the global hotkeys registered by tabSRMM.

$Id$

The hotkeyhandler is a small, invisible window which cares about a few things:

    a) global hotkeys (they need to work even if NO message window is open, that's
       why we handle it here.

    b) event notify stuff, messages posted from the popups to avoid threading
       issues.

    c) tray icon handling - cares about the events sent by the tray icon, handles
       the menus, doubleclicks and so on.
*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "functions.h"
#include "m_Snapping_windows.h"

extern struct ContainerWindowData *pFirstContainer;
extern HANDLE hMessageWindowList;
extern struct SendJob sendJobs[NR_SENDJOBS];
extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern PSLWA pSetLayeredWindowAttributes;

int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
int g_hotkeysEnabled = 0;
HWND g_hotkeyHwnd = 0;
static UINT WM_TASKBARCREATED;
HWND floaterOwner;

BOOL CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static POINT ptLast;
    static int iMousedown;
    static RECT rcLast;
    
    if(myGlobals.g_wantSnapping)
        CallSnappingWindowProc(hwndDlg, msg, wParam, lParam);

    if(msg == WM_TASKBARCREATED) {
        CreateSystrayIcon(FALSE);
        CreateSystrayIcon(TRUE);
        return 0;
    }
    switch(msg) {
        case WM_INITDIALOG:
            SendMessage(hwndDlg, DM_REGISTERHOTKEYS, 0, 0);
            g_hotkeyHwnd = hwndDlg;
            WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");
            SendMessage(GetDlgItem(hwndDlg, IDC_SLIST), BUTTONSETASFLATBTN, 0, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_TRAYICON), BUTTONSETASFLATBTN, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_SLIST, BUTTONADDTOOLTIP, (WPARAM) Translate("tabSRMM Quick Menu"), 0);
            SendDlgItemMessage(hwndDlg, IDC_TRAYICON, BUTTONADDTOOLTIP, (WPARAM) Translate("Session List"), 0);
            SendDlgItemMessage(hwndDlg, IDC_SLIST, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_iconPulldown);
            SendDlgItemMessage(hwndDlg, IDC_TRAYICON, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_iconContainer);
            ShowWindow(GetDlgItem(hwndDlg, IDC_TRAYCONTAINER), SW_HIDE);
            if (pSetLayeredWindowAttributes != NULL) 
                SetWindowLong(hwndDlg, GWL_EXSTYLE, GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_LAYERED);

            if(Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, "hkh")) {
                if(Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, "hkh"))
                    SetWindowPos(hwndDlg, 0, 50, 50, 50, 17, SWP_NOZORDER);

            }
            SendMessage(hwndDlg, DM_HKSAVESIZE, 0, 0);
            SendMessage(hwndDlg, DM_HKDETACH, 0, 0);
            ShowWindow(hwndDlg, nen_options.floaterMode ? SW_SHOW : SW_HIDE);
            return TRUE;
        case WM_COMMAND:
            SetFocus(floaterOwner);
            switch(LOWORD(wParam)) {
                case IDC_TRAYICON:
                    SendMessage(hwndDlg, DM_TRAYICONNOTIFY, 101, WM_LBUTTONUP);
                    break;
                case IDC_SLIST:
                    SendMessage(hwndDlg, DM_TRAYICONNOTIFY, 101, WM_RBUTTONUP);
                    break;
            }
            break;
        case WM_LBUTTONDOWN:
            iMousedown = 1;              
            GetCursorPos(&ptLast);
            SetCapture(hwndDlg);
            break;
        case WM_LBUTTONUP:
        {
            iMousedown = 0;
            ReleaseCapture();
            SendMessage(hwndDlg, DM_HKSAVESIZE, 0, 0);
            break;
        }
        case WM_MOUSEMOVE:
            {
                RECT rc;
                POINT pt;

                if (iMousedown) {
                    GetWindowRect(hwndDlg, &rc);
                    GetCursorPos(&pt);
                    MoveWindow(hwndDlg, rc.left - (ptLast.x - pt.x), rc.top - (ptLast.y - pt.y), rc.right - rc.left, rc.bottom - rc.top, TRUE);
                    ptLast = pt;
                }
                break;
            }
        case WM_HOTKEY:
            {
                struct ContainerWindowData *pCurrent = pFirstContainer, *pTargetContainer = 0;
                RECENTINFO ri = {0};
                HWND hwndTarget = 0;
                CLISTEVENT *cli = 0;
                TCITEM item = {0};
                DWORD dwTimestamp = 0;
                int i;
                
                ZeroMemory((void *)&ri, sizeof(ri));
                ri.iFirstIndex = ri.iMostRecent = -1;
                cli = (CLISTEVENT *)CallService(MS_CLIST_GETEVENT, (WPARAM)INVALID_HANDLE_VALUE, (LPARAM)0);
                if(cli != NULL) {
                    if(strncmp(cli->pszService, "SRMsg/TypingMessage", strlen(cli->pszService))) {
                        CallService(cli->pszService, 0, (LPARAM)cli);
                        break;
                    }
                }
                item.mask = TCIF_PARAM;
                while(pCurrent) {
                    for(i = 0; i < TabCtrl_GetItemCount(GetDlgItem(pCurrent->hwnd, IDC_MSGTABS)); i++) {
                        TabCtrl_GetItem(GetDlgItem(pCurrent->hwnd, IDC_MSGTABS), i, &item);
                        SendMessage((HWND)item.lParam, DM_QUERYLASTUNREAD, 0, (LPARAM)&dwTimestamp);
                        if (dwTimestamp > ri.dwMostRecent) {
                            ri.dwMostRecent = dwTimestamp;
                            ri.iMostRecent = i;
                            ri.hwndMostRecent = (HWND) item.lParam;
                            if(ri.dwFirst == 0 || (ri.dwFirst != 0 && dwTimestamp < ri.dwFirst)) {
                                ri.iFirstIndex = i;
                                ri.dwFirst = dwTimestamp;
                                ri.hwndFirst = (HWND) item.lParam;
                            }
                        }
                    }
                    pCurrent = pCurrent->pNextContainer;
                }
                if(wParam == 0xc002 && ri.iFirstIndex != -1) {
                    hwndTarget = ri.hwndFirst;
                }
                if(wParam == 0xc001 && ri.iMostRecent != -1) {
                    hwndTarget = ri.hwndMostRecent;
                }

                if(hwndTarget && IsWindow(hwndTarget)) {
                    SendMessage(hwndTarget, DM_QUERYCONTAINER, 0, (LPARAM)&pTargetContainer);
                    if(pTargetContainer) {
                        if(pTargetContainer->hwndActive != hwndTarget) {            // we need to switch tabs...
                            SendMessage(pTargetContainer->hwndActive, DM_QUERYLASTUNREAD, 0, (LPARAM)&dwTimestamp);
                            if(dwTimestamp == 0)                                    // dont switch if the active tab has unread events
                                ActivateTabFromHWND(GetParent(hwndTarget), hwndTarget);
                        }
                        if(IsIconic(pTargetContainer->hwnd) || pTargetContainer->bInTray != 0)
                            SendMessage(pTargetContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                        if(GetForegroundWindow() != pTargetContainer->hwnd || GetActiveWindow() != pTargetContainer->hwnd) {
                            SetActiveWindow(pTargetContainer->hwnd);
                            SetForegroundWindow(pTargetContainer->hwnd);
                        }
                    }
                }
                break;
            }
            /*
             * handle the popup menus (session list, favorites, recents...
             * just draw some icons, nothing more :)
             */
        case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT) lParam;
                lpmi->itemHeight = 0;
                lpmi->itemWidth = 6; //GetSystemMetrics(SM_CXSMICON);
                return TRUE;
            }
        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
                struct MessageWindowData *dat = 0;
                if(dis->CtlType == ODT_MENU && (dis->hwndItem == (HWND)myGlobals.g_hMenuFavorites || dis->hwndItem == (HWND)myGlobals.g_hMenuRecent)) {
                    ICONINFO ii;
                    BITMAP bm;
                    int cx = GetSystemMetrics(SM_CXSMICON);
                    int cy = GetSystemMetrics(SM_CYSMICON);
                    GetIconInfo((HICON)dis->itemData, &ii);
                    GetObject(ii.hbmColor, sizeof(bm), &bm);
                    DrawState(dis->hDC, NULL, NULL, (LPARAM) dis->itemData, 0,
                              2 + (dis->itemState & ODS_SELECTED ? 1 : 0),
                              (dis->itemState & ODS_SELECTED ? 1 : 0), bm.bmWidth != cx ? cx : 0, bm.bmHeight != cy ? cy : 0,
                              DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                    return TRUE;
                }
                else if(dis->CtlType == ODT_MENU) {
                    HWND hWnd = WindowList_Find(hMessageWindowList, (HANDLE)dis->itemID);
                    if(hWnd)
                        dat = (struct MessageWindowData *)GetWindowLong(hWnd, GWL_USERDATA);
                    
                    if (dis->itemData >= 0) {
                        HICON hIcon;
                        ICONINFO ii;
                        BITMAP bm;
                        int cx = GetSystemMetrics(SM_CXSMICON);
                        int cy = GetSystemMetrics(SM_CYSMICON);
                        if(dis->itemData > 0)
                            hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
                        else if(dat != NULL)
                            hIcon = LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus);
                        else
                            hIcon = myGlobals.g_iconContainer;
                        
                        GetIconInfo((HICON)dis->itemData, &ii);
                        GetObject(ii.hbmColor, sizeof(bm), &bm);

                        DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                                  2 + (dis->itemState & ODS_SELECTED ? 1 : 0),
                                  (dis->itemState & ODS_SELECTED ? 1 : 0), bm.bmWidth != cx ? cx : 0, bm.bmHeight != cy ? cy : 0,
                                  DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));

                        return TRUE;
                    }
                }
            }
            break;
        case DM_TRAYICONNOTIFY:
            {
                int iSelection;
                
                if(wParam == 100 || wParam == 101) {
                    switch(lParam) {
                        case WM_LBUTTONUP:
                        {
                            POINT pt;
                            GetCursorPos(&pt);
                            if(wParam == 100)
                                SetForegroundWindow(hwndDlg);
                            if(GetMenuItemCount(myGlobals.g_hMenuTrayUnread) > 0) {
                                HWND hWnd;
                                iSelection = TrackPopupMenu(myGlobals.g_hMenuTrayUnread, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                                hWnd = WindowList_Find(hMessageWindowList, (HANDLE)iSelection);
                                if(hWnd) {
                                    struct ContainerWindowData *pContainer = 0;
                                    SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
                                    if(pContainer)
                                        ActivateExistingTab(pContainer, hWnd);
                                    SetForegroundWindow(pContainer->hwnd);
                                }
                                else
                                    CallService(MS_MSG_SENDMESSAGE, (WPARAM)iSelection, 0);
                            }
                            if(wParam == 100)
                                PostMessage(hwndDlg, WM_NULL, 0, 0);
                            break;
                        }
                        case WM_MBUTTONDOWN:
                        {
                            int iCount = GetMenuItemCount(myGlobals.g_hMenuTrayUnread);
                            SetForegroundWindow(hwndDlg);
                            if(iCount > 0) {
                                UINT uid = GetMenuItemID(myGlobals.g_hMenuTrayUnread, iCount - 1);
                                HWND hWnd = WindowList_Find(hMessageWindowList, (HANDLE)uid);
                                if(hWnd) {
                                    struct ContainerWindowData *pContainer = 0;
                                    SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
                                    if(pContainer)
                                        ActivateExistingTab(pContainer, hWnd);
                                    SetFocus(hWnd);
                                    SetForegroundWindow(pContainer->hwnd);
                                }
                                else
                                    CallService(MS_MSG_SENDMESSAGE, (WPARAM)uid, 0);
                            }
                            PostMessage(hwndDlg, WM_NULL, 0, 0);
                            break;
                        }
                        case WM_RBUTTONUP:
                        {
                            HMENU submenu = myGlobals.g_hMenuTrayContext;
                            POINT pt;

                            if(wParam == 100)
                                SetForegroundWindow(hwndDlg);
                            GetCursorPos(&pt);
                            CheckMenuItem(submenu, ID_TRAYCONTEXT_DISABLEALLPOPUPS, MF_BYCOMMAND | (nen_options.iDisable ? MF_CHECKED : MF_UNCHECKED));
                            CheckMenuItem(submenu, ID_TRAYCONTEXT_DON40223, MF_BYCOMMAND | (nen_options.iNoSounds ? MF_CHECKED : MF_UNCHECKED));
                            CheckMenuItem(submenu, ID_TRAYCONTEXT_DON, MF_BYCOMMAND | (nen_options.iNoAutoPopup ? MF_CHECKED : MF_UNCHECKED));
                            
                            iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                            
                            if(iSelection) {
                                MENUITEMINFO mii = {0};

                                mii.cbSize = sizeof(mii);
                                mii.fMask = MIIM_DATA | MIIM_ID;
                                GetMenuItemInfo(submenu, (UINT_PTR)iSelection, FALSE, &mii);
                                if(mii.dwItemData != 0) {                       // this must be an itm of the fav or recent menu
                                    HWND hWnd = WindowList_Find(hMessageWindowList, (HANDLE)iSelection);
                                    if(hWnd) {
                                        struct ContainerWindowData *pContainer = 0;
                                        SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
                                        if(pContainer)
                                            ActivateExistingTab(pContainer, hWnd);
                                    }
                                    else
                                        CallService(MS_MSG_SENDMESSAGE, (WPARAM)iSelection, 0);
                                }
                                else {
                                    switch(iSelection) {
                                        case ID_TRAYCONTEXT_DISABLEALLPOPUPS:
                                            nen_options.iDisable ^= 1;
                                            break;
                                        case ID_TRAYCONTEXT_DON40223:
                                            nen_options.iNoSounds ^= 1;
                                            break;
                                        case ID_TRAYCONTEXT_DON:
                                            nen_options.iNoAutoPopup ^= 1;
                                            break;
                                        case ID_TRAYCONTEXT_HIDEALLMESSAGECONTAINERS:
                                        {
                                            struct ContainerWindowData *pContainer = pFirstContainer;
                                            BOOL bAnimatedState = nen_options.bAnimated;

                                            nen_options.bAnimated = FALSE;
                                            while(pContainer) {
                                                if(!pContainer->bInTray)
                                                    SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 1);
                                                pContainer = pContainer->pNextContainer;
                                            }
                                            nen_options.bAnimated = bAnimatedState;
                                            break;
                                        }
                                        case ID_TRAYCONTEXT_RESTOREALLMESSAGECONTAINERS:
                                        {
                                            struct ContainerWindowData *pContainer = pFirstContainer;
                                            BOOL bAnimatedState = nen_options.bAnimated;

                                            nen_options.bAnimated = FALSE;
                                            while(pContainer) {
                                                if(pContainer->bInTray)
                                                    SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                                                pContainer = pContainer->pNextContainer;
                                            }
                                            nen_options.bAnimated = bAnimatedState;
                                            break;
                                        }
                                    }
                                }
                            }
                            if(wParam == 100)
                                PostMessage(hwndDlg, WM_NULL, 0, 0);
                            break;
                        }
                        case NIN_BALLOONUSERCLICK:
                        {
                            HWND hWnd = WindowList_Find(hMessageWindowList, myGlobals.m_TipOwner);

                            if(hWnd) {
                                struct ContainerWindowData *pContainer = 0;
                                SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
                                if(pContainer)
                                    ActivateExistingTab(pContainer, hWnd);
                            }
                            else
                                CallService(MS_MSG_SENDMESSAGE, (WPARAM)hWnd, 0);
                        }
                        default:
                            break;
                    }
                }
                break;
            }
            /*
             * handle an event from the popup module (mostly window activation). Since popups may run in different threads, the message
             * is posted to our invisible hotkey handler which does always run within the main thread.
             * wParam is the hContact
             */
        case DM_HANDLECLISTEVENT:
            {
                /*
                 * if tray support is on, no clist events will be created when a message arrives for a contact
                 * w/o an open window. So we just open it manually...
                 */
                if(nen_options.bTraySupport)
                    CallService(MS_MSG_SENDMESSAGE, (WPARAM)wParam, 0);
                else {
                    CLISTEVENT *cle = (CLISTEVENT *)CallService(MS_CLIST_GETEVENT, wParam, 0);
                    if (cle) {
                        if (ServiceExists(cle->pszService)) {
                            CallService(cle->pszService, (WPARAM)NULL, (LPARAM)cle);
                            CallService(MS_CLIST_REMOVEEVENT, (WPARAM)cle->hContact, (LPARAM)cle->hDbEvent);
                        }
                    }
                }
                break;
            }
        case DM_DOCREATETAB:
        {
            HWND hWnd = WindowList_Find(hMessageWindowList, (HANDLE)lParam);
            if(hWnd && IsWindow(hWnd)) {
                struct ContainerWindowData *pContainer = 0;
                
                SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
                if(pContainer) {
                    int iTabs = TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS));
                    if(iTabs == 1)
                        SendMessage(pContainer->hwnd, WM_CLOSE, 0, 1);
                    else
                        SendMessage(hWnd, WM_CLOSE, 0, 1);
                    
                    CreateNewTabForContact((struct ContainerWindowData *)wParam, (HANDLE)lParam, 0, NULL, TRUE, TRUE, FALSE, 0);
                }
            }
            break;
        }
            /*
             * sent from the popup to "dismiss" the event. we should do this in the main thread
             */
        case DM_REMOVECLISTEVENT:
            CallService(MS_CLIST_REMOVEEVENT, wParam, lParam);
            CallService(MS_DB_EVENT_MARKREAD, wParam, lParam);
            break;
        case DM_REGISTERHOTKEYS:
            {
                int iWantHotkeys = DBGetContactSettingByte(NULL, SRMSGMOD_T, "globalhotkeys", 0);
                
                if(iWantHotkeys && !g_hotkeysEnabled) {
                    int mod = MOD_CONTROL | MOD_SHIFT;
                    
                    switch(DBGetContactSettingByte(NULL, SRMSGMOD_T, "hotkeymodifier", 0)) {
                        case HOTKEY_MODIFIERS_CTRLSHIFT:
                            mod = MOD_CONTROL | MOD_SHIFT;
                            break;
                        case HOTKEY_MODIFIERS_ALTSHIFT:
                            mod = MOD_ALT | MOD_SHIFT;
                            break;
                        case HOTKEY_MODIFIERS_CTRLALT:
                            mod = MOD_CONTROL | MOD_ALT;
                            break;
                    }
                    RegisterHotKey(hwndDlg, 0xc001, mod, 0x52);
                    RegisterHotKey(hwndDlg, 0xc002, mod, 0x55);         // ctrl-shift-u
                    g_hotkeysEnabled = TRUE;
                }
                else {
                    if(g_hotkeysEnabled) {
                        SendMessage(hwndDlg, DM_FORCEUNREGISTERHOTKEYS, 0, 0);
                    }
                }
                break;
            }
        case WM_TIMER:
            if(wParam == 1000) {
                if((myGlobals.m_TrayFlashes = myGlobals.m_UnreadInTray > 0 ? 1 : 0) != 0)
                    FlashTrayIcon(0);
                else
                    FlashTrayIcon(1);
            }
            break;
        case DM_FORCEUNREGISTERHOTKEYS:
            UnregisterHotKey(hwndDlg, 0xc001);
            UnregisterHotKey(hwndDlg, 0xc002);
            g_hotkeysEnabled = FALSE;
            break;
        case DM_HKDETACH:
            SetWindowPos(hwndDlg, HWND_TOPMOST, rcLast.left, rcLast.top, rcLast.right - rcLast.left, rcLast.bottom - rcLast.top, SWP_NOACTIVATE);
            if (pSetLayeredWindowAttributes != NULL)
                pSetLayeredWindowAttributes(hwndDlg, RGB(50, 250, 250), (BYTE)220, LWA_ALPHA);
            break;
        case DM_HKSAVESIZE:
        {
            WINDOWPLACEMENT wp = {0};

            wp.length = sizeof(wp);
            GetWindowPlacement(hwndDlg, &wp);
            rcLast = wp.rcNormalPosition;
            break;
        }
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_ACTIVE)
                SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
            return 0;
        case WM_CLOSE:
            return 0;
        case WM_DESTROY:
        {
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "hkhx", rcLast.left);
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "hkhy", rcLast.top);
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "hkhwidth", rcLast.right - rcLast.left);
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "hkhheight", rcLast.bottom - rcLast.top);
            if(g_hotkeysEnabled)
                SendMessage(hwndDlg, DM_FORCEUNREGISTERHOTKEYS, 0, 0);
            break;
        }
    }
    return FALSE;
}

