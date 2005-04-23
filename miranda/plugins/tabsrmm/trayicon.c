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
        myGlobals.g_hMenuTrayUnread = GetSubMenu(myGlobals.g_hMenuContext, 5);
        nen_options.bTrayExist = TRUE;
        SetTimer(myGlobals.g_hwndHotkeyHandler, 1000, 1000, 0);
    }
    else if(create == FALSE && nen_options.bTrayExist) {
        struct ContainerWindowData *pContainer = pFirstContainer;
        
        Shell_NotifyIcon(NIM_DELETE, &nim);
        nen_options.bTrayExist = FALSE;
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

    // Did we find the Main System Tray? If so, then get its size and keep going
    if (_tcscmp(szClassName, _T("TrayNotifyWnd")) == 0) {
        RECT *pRect = (RECT *) lParam;
        GetWindowRect(hwnd, pRect);
        return TRUE;
    }

    // Did we find the System Clock? If so, then adjust the size of the rectangle
    // we have and quit (clock will be found after the system tray)
    if (_tcscmp(szClassName, _T("TrayClockWClass")) == 0) {
        RECT *pRect = (RECT *) lParam;
        RECT rectClock;
        GetWindowRect(hwnd, &rectClock);
        // if clock is above system tray adjust accordingly
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

    // Move focus away and back again to ensure taskbar icon is recreated
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
