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

#define _WIN32_IE 0x501

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "functions.h"

extern struct ContainerWindowData *pFirstContainer;
extern HANDLE hMessageWindowList;
extern struct SendJob sendJobs[NR_SENDJOBS];
extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;

int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
int g_hotkeysEnabled = 0;
HWND g_hotkeyHwnd = 0;

BOOL CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_INITDIALOG:
            SendMessage(hwndDlg, DM_REGISTERHOTKEYS, 0, 0);
            //g_hSettingChanged = HookEventMessage(ME_DB_CONTACT_SETTINGCHANGED, hwndDlg, DM_CONTACTSETTINGCHANGED);
            //g_hAckEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, DM_PROTOACK);
            //g_hNewEvent = HookEventMessage(ME_DB_EVENT_ADDED, hwndDlg, HM_DBEVENTADDED);
            g_hotkeyHwnd = hwndDlg;
            return TRUE;
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
                    DrawState(dis->hDC, NULL, NULL, (LPARAM) dis->itemData, 0,
                              2 + (dis->itemState & ODS_SELECTED ? 1 : 0),
                              (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0,
                              DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                    return TRUE;
                }
                else if(dis->CtlType == ODT_MENU) {
                    HWND hWnd = WindowList_Find(hMessageWindowList, (HANDLE)dis->itemID);
                    if(hWnd)
                        dat = (struct MessageWindowData *)GetWindowLong(hWnd, GWL_USERDATA);
                    
                    if (dis->itemData >= 0) {
                        HICON hIcon;

                        if(dis->itemData > 0)
                            hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
                        else if(dat != NULL)
                            hIcon = LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus);
                        else
                            hIcon = myGlobals.g_iconContainer;
                        
                        DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                                  2 + (dis->itemState & ODS_SELECTED ? 1 : 0),
                                  (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0,
                                  DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));

                        return TRUE;
                    }
                }
            }
            break;
        case DM_TRAYICONNOTIFY:
            {
                int iSelection;
                
                if(wParam == 100) {
                    switch(lParam) {
                        case WM_LBUTTONUP:
                        {
                            POINT pt;
                            GetCursorPos(&pt);
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
        case WM_DESTROY:
            //if (g_hSettingChanged)
            //    UnhookEvent(g_hSettingChanged);
            if(g_hotkeysEnabled)
                SendMessage(hwndDlg, DM_FORCEUNREGISTERHOTKEYS, 0, 0);
            //if(g_hAckEvent)
            //    UnhookEvent(g_hAckEvent);
            //if(g_hNewEvent)
            //    UnhookEvent(g_hNewEvent);
            break;
    }
    return FALSE;
}

