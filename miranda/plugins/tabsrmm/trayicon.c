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

Functions, concerning tabSRMMs system tray support. There is more in eventpopups.c

*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "functions.h"
#include "m_toptoolbar.h"

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern struct ContainerWindowData *pFirstContainer;

void CreateTrayMenus(int mode)
{
    if(mode) {
        myGlobals.g_hMenuTrayUnread = CreatePopupMenu();
        myGlobals.g_hMenuFavorites = CreatePopupMenu();
        myGlobals.g_hMenuRecent = CreatePopupMenu();
        myGlobals.g_hMenuTrayContext = GetSubMenu(myGlobals.g_hMenuContext, 6);
        if(myGlobals.m_WinVerMajor >= 5) {
            ModifyMenuA(myGlobals.g_hMenuTrayContext, 0, MF_BYPOSITION | MF_POPUP, (UINT_PTR)myGlobals.g_hMenuFavorites, Translate("Favorites"));
            ModifyMenuA(myGlobals.g_hMenuTrayContext, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)myGlobals.g_hMenuRecent, Translate("Recent Sessions"));
            LoadFavoritesAndRecent();
        }
        else {
            DeleteMenu(myGlobals.g_hMenuTrayContext, 2, MF_BYPOSITION);
            DeleteMenu(myGlobals.g_hMenuTrayContext, 0, MF_BYPOSITION);
            DeleteMenu(myGlobals.g_hMenuTrayContext, 0, MF_BYPOSITION);
            DeleteMenu(myGlobals.g_hMenuTrayContext, 0, MF_BYPOSITION);
        }
        SetTimer(myGlobals.g_hwndHotkeyHandler, 1000, 1000, 0);
    }
    else {
        if(myGlobals.g_hMenuTrayUnread != 0) {
            DestroyMenu(myGlobals.g_hMenuTrayUnread);
            myGlobals.g_hMenuTrayUnread = 0;
        }
        if(myGlobals.g_hMenuFavorites != 0) {
            DestroyMenu(myGlobals.g_hMenuFavorites);
            myGlobals.g_hMenuFavorites = 0;
        }
        if(myGlobals.g_hMenuRecent != 0) {
            DestroyMenu(myGlobals.g_hMenuRecent);
            myGlobals.g_hMenuRecent = 0;
        }
        KillTimer(myGlobals.g_hwndHotkeyHandler, 1000);
    }
}
/*
 * create a system tray icon, create all necessary submenus
 */
 
void CreateSystrayIcon(int create)
{
    NOTIFYICONDATA nim;

    nim.cbSize = sizeof(nim);
    nim.hWnd = myGlobals.g_hwndHotkeyHandler;
    nim.uID = 100;
    nim.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nim.hIcon = myGlobals.g_iconContainer;
    nim.uCallbackMessage = DM_TRAYICONNOTIFY;
#if defined(_UNICODE)
    mir_snprintfW(nim.szTip, 64, L"%s", L"tabSRMM");
#else
    mir_snprintf(nim.szTip, 64, "%s", "tabSRMM");
#endif    
    if(create && !nen_options.bTrayExist) {
        Shell_NotifyIcon(NIM_ADD, &nim);
        nen_options.bTrayExist = TRUE;
    }
    else if(create == FALSE && nen_options.bTrayExist) {
        Shell_NotifyIcon(NIM_DELETE, &nim);
        nen_options.bTrayExist = FALSE;
    }
    ShowWindow(myGlobals.g_hwndHotkeyHandler, nen_options.floaterMode ? SW_SHOW : SW_HIDE);
}

BOOL CALLBACK FindTrayWnd(HWND hwnd, LPARAM lParam)
{
    TCHAR szClassName[256];
    GetClassName(hwnd, szClassName, 255);

    if (_tcscmp(szClassName, _T("TrayNotifyWnd")) == 0) {
        RECT *pRect = (RECT *) lParam;
        GetWindowRect(hwnd, pRect);
        return TRUE;
    }
    if (_tcscmp(szClassName, _T("TrayClockWClass")) == 0) {
        RECT *pRect = (RECT *) lParam;
        RECT rectClock;
        GetWindowRect(hwnd, &rectClock);
        if (rectClock.bottom < pRect->bottom-5) // 10 = random fudge factor.
            pRect->top = rectClock.bottom;
        else
            pRect->right = rectClock.left;
        return FALSE;
    }
    return TRUE;
}
 
void GetTrayWindowRect(LPRECT lprect)
{
    HWND hShellTrayWnd = FindWindow(_T("Shell_TrayWnd"), NULL);
    if (hShellTrayWnd) {
        GetWindowRect(hShellTrayWnd, lprect);
        EnumChildWindows(hShellTrayWnd, FindTrayWnd, (LPARAM)lprect);
        return;
    }
}

/*
 * this hides the window from the taskbar by re-parenting it to our invisible background
 * window.
 */

BOOL RemoveTaskbarIcon(HWND hWnd)
{
    SetParent(hWnd, GetDlgItem(myGlobals.g_hwndHotkeyHandler, IDC_TRAYCONTAINER));
    return TRUE;
}

/*
 * minimise a window to the system tray, using a simple animation.
 */

void MinimiseToTray(HWND hWnd, BOOL bForceAnimation)
{
    if (bForceAnimation) {
        RECT rectFrom, rectTo;

        GetWindowRect(hWnd, &rectFrom);
        GetTrayWindowRect(&rectTo);
        if(nen_options.floaterMode && IsWindowVisible(myGlobals.g_hwndHotkeyHandler))
            GetWindowRect(myGlobals.g_hwndHotkeyHandler, &rectTo);
        DrawAnimatedRects(hWnd, IDANI_CAPTION, &rectFrom, &rectTo);
    }
    RemoveTaskbarIcon(hWnd);
    ShowWindow(hWnd, SW_HIDE);              // experimental - now works with docks like rklauncher..
    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_VISIBLE);
}

void MaximiseFromTray(HWND hWnd, BOOL bForceAnimation, RECT *rectTo)
{
    if (bForceAnimation) {
        RECT rectFrom;

        GetTrayWindowRect(&rectFrom);
        if(nen_options.floaterMode && IsWindowVisible(myGlobals.g_hwndHotkeyHandler))
            GetWindowRect(myGlobals.g_hwndHotkeyHandler, &rectFrom);
        SetParent(hWnd, NULL);
        DrawAnimatedRects(hWnd, IDANI_CAPTION, &rectFrom, rectTo);
    }
    else
        SetParent(hWnd, NULL);

    ShowWindow(hWnd, SW_SHOW);
    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_VISIBLE);
    RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME |
                                   RDW_INVALIDATE | RDW_ERASE);

    SetActiveWindow(myGlobals.g_hwndHotkeyHandler);
    SetActiveWindow(hWnd);
    SetForegroundWindow(hWnd);
}

/*
 * flash the tray icon
 * mode = 0 - continue to flash
 * mode = 1 - restore the original icon
 */

void FlashTrayIcon(int mode)
{
    NOTIFYICONDATA nim;

    if(myGlobals.m_WinVerMajor < 5)
        return;
    
    if(nen_options.bTraySupport) {
        nim.cbSize = sizeof(nim);
        nim.hWnd = myGlobals.g_hwndHotkeyHandler;
        nim.uID = 100;
        nim.uFlags = NIF_ICON;
        nim.hIcon = mode ? myGlobals.g_iconContainer : (myGlobals.m_TrayFlashState ? 0 : myGlobals.g_iconContainer);
        Shell_NotifyIcon(NIM_MODIFY, &nim);
        myGlobals.m_TrayFlashState = !myGlobals.m_TrayFlashState;
        if(mode)
            myGlobals.m_TrayFlashState = 0;
    }
    else if(IsWindowVisible(myGlobals.g_hwndHotkeyHandler) && !nen_options.bTraySupport) {
        HICON hIcon = mode ? myGlobals.g_iconContainer : (myGlobals.m_TrayFlashState ? 0 : myGlobals.g_iconContainer);
        
        SendDlgItemMessage(myGlobals.g_hwndHotkeyHandler, IDC_TRAYICON, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
        myGlobals.m_TrayFlashState = !myGlobals.m_TrayFlashState;
        if(mode)
            myGlobals.m_TrayFlashState = 0;
    }
}

void RemoveBalloonTip()
{
    NOTIFYICONDATA nim;

    nim.cbSize = sizeof(nim);
    nim.hWnd = myGlobals.g_hwndHotkeyHandler;
    nim.uID = 100;
    nim.uFlags = NIF_INFO;
    nim.szInfo[0] = 0;
    Shell_NotifyIcon(NIM_MODIFY, &nim);
    myGlobals.m_TipOwner = (HANDLE)0;
}

/*
 * add a contact to recent or favorites menu
 * mode = 1, add
 * mode = 0, only modify it..
 * hMenu specifies the menu handle (the menus are identical...)
 * cares about updating the menu entry. It sets the hIcon (proto status icon) in
 * dwItemData of the the menu entry, so that the WM_DRAWITEM handler can retrieve it
 * w/o costly service calls.
 * 
 * Also, the function does housekeeping on the Recent Sessions menu to enforce the
 * maximum number of allowed entries (20 at the moment). The oldest (topmost) entry
 * is deleted, if necessary.
 */

void AddContactToFavorites(HANDLE hContact, char *szNickname, char *szProto, char *szStatus, WORD wStatus, HICON hIcon, BOOL mode, HMENU hMenu, UINT codePage)
{
    MENUITEMINFO mii = {0};
    char szMenuEntry[80];
#if defined(_UNICODE)
    const wchar_t *szMenuEntryW = 0;
#endif
    if(szNickname == NULL)
        szNickname = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);
    if(szProto == NULL)
        szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    if(szProto) {
        if(wStatus == 0)
            wStatus = DBGetContactSettingWord((HANDLE)hContact, szProto, "Status", ID_STATUS_OFFLINE);
        if(szStatus == NULL)
            szStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, wStatus, 0);
    }
    else
        return;

    if(hIcon == 0)
        hIcon = LoadSkinnedProtoIcon(szProto, wStatus);
    
    mii.cbSize = sizeof(mii);
#if defined(_UNICODE)
    mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s)", szProto, "%nick%", szStatus);
    szMenuEntryW = EncodeWithNickname(szMenuEntry, szNickname, codePage);
#else
    mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s)", szProto, szNickname, szStatus);
#endif
    if(mode) {
        if(hMenu == myGlobals.g_hMenuRecent) {
            if(CheckMenuItem(hMenu, (UINT_PTR)hContact, MF_BYCOMMAND | MF_UNCHECKED) == 0) {
                DeleteMenu(hMenu, (UINT_PTR)hContact, MF_BYCOMMAND);
                goto addnew;                                            // move to the end of the menu...
            }
            if(GetMenuItemCount(myGlobals.g_hMenuRecent) > nen_options.wMaxRecent) {            // throw out oldest entry in the recent menu...
                UINT uid = GetMenuItemID(hMenu, 0);
                if(uid) {
                    DeleteMenu(hMenu, (UINT_PTR)0, MF_BYPOSITION);
                    DBWriteContactSettingDword((HANDLE)uid, SRMSGMOD_T, "isRecent", 0);
                }
            }
addnew:
            DBWriteContactSettingDword(hContact, SRMSGMOD_T, "isRecent", time(NULL));
#if defined(_UNICODE)
            AppendMenuW(hMenu, MF_BYCOMMAND, (UINT_PTR)hContact, szMenuEntryW);
#else
            AppendMenuA(hMenu, MF_BYCOMMAND, (UINT_PTR)hContact, szMenuEntry);
#endif
        }
        else if(hMenu == myGlobals.g_hMenuFavorites) {              // insert the item sorted...
            MENUITEMINFO mii2 = {0};
            TCHAR szBuffer[142];
            int i, c = GetMenuItemCount(myGlobals.g_hMenuFavorites);
            mii2.fMask = MIIM_STRING;
            mii2.cbSize = sizeof(mii2);
            if(c == 0)
#if defined(_UNICODE)
                InsertMenuW(myGlobals.g_hMenuFavorites, 0, MF_BYPOSITION, (UINT_PTR)hContact, szMenuEntryW);
#else
                InsertMenuA(myGlobals.g_hMenuFavorites, 0, MF_BYPOSITION, (UINT_PTR)hContact, szMenuEntry);
#endif
            else {
                for(i = 0; i <= c; i++) {
                    mii2.cch = 0;
                    mii2.dwTypeData = NULL;
                    GetMenuItemInfo(myGlobals.g_hMenuFavorites, i, TRUE, &mii2);
                    mii2.cch++;
                    mii2.dwTypeData = szBuffer;
                    GetMenuItemInfo(myGlobals.g_hMenuFavorites, i, TRUE, &mii2);
#if defined(_UNICODE)
                    if(wcsncmp((wchar_t *)mii2.dwTypeData, szMenuEntryW, 140) > 0 || i == c) {
                        InsertMenuW(myGlobals.g_hMenuFavorites, i, MF_BYPOSITION, (UINT_PTR)hContact, szMenuEntryW);
                        break;
                    }
#else
                    if(strncmp((char *)mii2.dwTypeData, szMenuEntry, 140) > 0 || i == c) {
                        InsertMenuA(myGlobals.g_hMenuFavorites, i, MF_BYPOSITION, (UINT_PTR)hContact, szMenuEntry);
                        break;
                    }
#endif
                }
            }
        }
    }
    mii.fMask = MIIM_BITMAP | MIIM_DATA;
    if(!mode) {
        mii.fMask |= MIIM_STRING;
#if defined(_UNICODE)
        mii.dwTypeData = (LPWSTR)szMenuEntryW;
        mii.cch = lstrlenW(szMenuEntryW) + 1;
#else
        mii.dwTypeData = szMenuEntry;
        mii.cch = lstrlenA(szMenuEntry) + 1;
#endif
    }
    mii.hbmpItem = HBMMENU_CALLBACK;
    mii.dwItemData = (ULONG)hIcon;
    SetMenuItemInfo(hMenu, (UINT)hContact, FALSE, &mii);
}

/*
 * called by CreateSysTrayIcon(), usually on startup or when you activate tray support
 * at runtime.
 * scans the contact db for favorites or recent session entries and builds the menus.
 */

typedef struct _recentEntry {
    DWORD dwTimestamp;
    HANDLE hContact;
} RCENTRY;

void LoadFavoritesAndRecent()
{
    RCENTRY *recentEntries, rceTemp;
    DWORD dwRecent;
    int iIndex = 0, i, j;
    HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    recentEntries = (RCENTRY *)malloc((nen_options.wMaxRecent + 1 )* sizeof(RCENTRY));
    if(recentEntries != NULL) {
        while(hContact != 0) {
            if(DBGetContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", 0))
                AddContactToFavorites(hContact, NULL, NULL, NULL, 0, 0, 1, myGlobals.g_hMenuFavorites, DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP));
            if((dwRecent = DBGetContactSettingDword(hContact, SRMSGMOD_T, "isRecent", 0)) != 0 && iIndex < nen_options.wMaxRecent) {
                recentEntries[iIndex].dwTimestamp = dwRecent;
                recentEntries[iIndex++].hContact = hContact;
            }
            hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
        if(iIndex == 0)
            return;

        for(i = 0; i < iIndex - 1; i++) {
            for(j = 0; j < iIndex - 1; j++) {
                if(recentEntries[j].dwTimestamp > recentEntries[j+1].dwTimestamp) {
                    rceTemp = recentEntries[j];
                    recentEntries[j] = recentEntries[j+1];
                    recentEntries[j+1] = rceTemp;
                }
            }
        }
        for(i = 0; i < iIndex; i++)
            AddContactToFavorites(recentEntries[i].hContact, NULL, NULL, NULL, 0, 0, 1, myGlobals.g_hMenuRecent, DBGetContactSettingDword(recentEntries[i].hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP));

        free(recentEntries);
    }
}

