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

#define _WIN32_IE 0x0500

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "functions.h"

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern struct ContainerWindowData *pFirstContainer;

void CreateSystrayIcon(int create)
{
    NOTIFYICONDATA nim;

    nim.cbSize = sizeof(nim);
    nim.hWnd = myGlobals.g_hwndHotkeyHandler;
    nim.uID = 100;
    nim.uFlags = NIF_ICON | NIF_MESSAGE;
    nim.hIcon = myGlobals.g_iconContainer;
    nim.uCallbackMessage = DM_TRAYICONNOTIFY;
    if(create && !nen_options.bTrayExist) {
        Shell_NotifyIcon(NIM_ADD, &nim);
        myGlobals.g_hMenuTrayUnread = CreatePopupMenu();
        myGlobals.g_hMenuFavorites = CreatePopupMenu();
        myGlobals.g_hMenuRecent = CreatePopupMenu();
        myGlobals.g_hMenuTrayContext = GetSubMenu(myGlobals.g_hMenuContext, 6);
        ModifyMenuA(myGlobals.g_hMenuTrayContext, 0, MF_BYPOSITION | MF_POPUP, (UINT_PTR)myGlobals.g_hMenuFavorites, Translate("Favorites"));
        ModifyMenuA(myGlobals.g_hMenuTrayContext, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)myGlobals.g_hMenuRecent, Translate("Recent Sessions"));
        LoadFavoritesAndRecent();
        nen_options.bTrayExist = TRUE;
        SetTimer(myGlobals.g_hwndHotkeyHandler, 1000, 1000, 0);
    }
    else if(create == FALSE && nen_options.bTrayExist) {
        struct ContainerWindowData *pContainer = pFirstContainer;
        
        Shell_NotifyIcon(NIM_DELETE, &nim);
        nen_options.bTrayExist = FALSE;
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
        /*
         * check if there are containers minimized to the tray, get them back, otherwise the're trapped forever :)
         * need to temporarily re-enable tray support, because the container checks for it.
         */
        nen_options.bTraySupport = TRUE;
        while(pContainer) {
            if(pContainer->bInTray)
                SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            pContainer = pContainer->pNextContainer;
        }
        nen_options.bTraySupport = FALSE;
    }
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

BOOL RemoveTaskbarIcon(HWND hWnd)
{
    SetParent(hWnd, myGlobals.g_hwndHotkeyHandler);
    return TRUE;
}

void MinimiseToTray(HWND hWnd, BOOL bForceAnimation)
{
    if (bForceAnimation) {
        RECT rectFrom, rectTo;

        GetWindowRect(hWnd, &rectFrom);
        GetTrayWindowRect(&rectTo);
        DrawAnimatedRects(hWnd, IDANI_CAPTION, &rectFrom, &rectTo);
    }
    RemoveTaskbarIcon(hWnd);
    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_VISIBLE);
}

void MaximiseFromTray(HWND hWnd, BOOL bForceAnimation, RECT *rectTo)
{
    if (bForceAnimation) {
        RECT rectFrom;

        GetTrayWindowRect(&rectFrom);
        SetParent(hWnd, NULL);
        DrawAnimatedRects(hWnd, IDANI_CAPTION, &rectFrom, rectTo);
    }
    else
        SetParent(hWnd, NULL);

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
 */

void AddContactToFavorites(HANDLE hContact, char *szNickname, char *szProto, char *szStatus, WORD wStatus, HICON hIcon, BOOL mode, HMENU hMenu)
{
    MENUITEMINFOA mii = {0};
    char szMenuEntry[80];

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
    mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s)", szProto, szNickname, szStatus);
    if(mode) {
        if(hMenu == myGlobals.g_hMenuRecent) {
            if(CheckMenuItem(hMenu, (UINT_PTR)hContact, MF_BYCOMMAND | MF_UNCHECKED) == 0)
                return;             // don't dupicate items on the recent menu
            if(GetMenuItemCount(myGlobals.g_hMenuRecent) > 15) {            // throw out oldest entry in the recent menu...
                UINT uid = GetMenuItemID(hMenu, 0);
                if(uid) {
                    DeleteMenu(hMenu, (UINT_PTR)0, MF_BYPOSITION);
                    DBDeleteContactSetting((HANDLE)uid, SRMSGMOD_T, "isRecent");
                }
            }
            DBWriteContactSettingWord(hContact, SRMSGMOD_T, "isRecent", 1);
        }
        AppendMenuA(hMenu, MF_BYCOMMAND, (UINT_PTR)hContact, szMenuEntry);
    }
    mii.fMask = MIIM_BITMAP | MIIM_DATA;
    if(!mode) {
        mii.fMask |= MIIM_STRING;
        mii.dwTypeData = szMenuEntry;
        mii.cch = lstrlenA(szMenuEntry);
    }
    mii.hbmpItem = HBMMENU_CALLBACK;
    mii.dwItemData = (ULONG)hIcon;
    SetMenuItemInfoA(hMenu, (UINT)hContact, FALSE, &mii);
}

void LoadFavoritesAndRecent()
{
    HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while(hContact != 0) {
        if(DBGetContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", 0))
            AddContactToFavorites(hContact, NULL, NULL, NULL, 0, 0, 1, myGlobals.g_hMenuFavorites);
        if(DBGetContactSettingWord(hContact, SRMSGMOD_T, "isRecent", 0))
            AddContactToFavorites(hContact, NULL, NULL, NULL, 0, 0, 1, myGlobals.g_hMenuRecent);
        hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
    }
}

