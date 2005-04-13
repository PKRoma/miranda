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

#define TM_AUTOALPHA  1
#define MENU_MIRANDAMENU         0xFFFF1234

static HMODULE hUserDll;
static HIMAGELIST himlMirandaIcon;
HWND hwndContactList = NULL;
HWND hwndStatus;
HMENU hMenuMain;
static HANDLE hContactDraggingEvent, hContactDroppedEvent, hContactDragStopEvent;
static int transparentFocus = 1;
HWND hwndContactTree;
UINT uMsgProcessProfile;

BOOL(WINAPI * MySetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
BOOL(WINAPI * MyAnimateWindow) (HWND hWnd, DWORD dwTime, DWORD dwFlags);

typedef struct {
	int showsbar;
	int showgrip;
	int transparent;
	int alpha;
} CluiOpts;
static CluiOpts cluiopt = {0};

int CluiOptInit(WPARAM wParam, LPARAM lParam);
int SortList(WPARAM wParam, LPARAM lParam);
int LoadCluiServices(void);
int CluiProtocolStatusChanged(WPARAM wParam, LPARAM lParam);

void LoadCluiGlobalOpts() {
	cluiopt.showsbar = DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1);
	cluiopt.showgrip = DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1);
	cluiopt.transparent = DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT);
	cluiopt.alpha = DBGetContactSettingByte(NULL, "CList", "Alpha", SETTING_ALPHA_DEFAULT);
}

static int CluiModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = MENUITEMINFO_V4_SIZE;
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);
    SetMenuItemInfo(hMenuMain, 0, TRUE, &mii);
    mii.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
    SetMenuItemInfo(hMenuMain, 1, TRUE, &mii);
    return 0;
}

// Restore protocols to the last global status.
// Used to reconnect on restore after standby.
static void RestoreMode()
{
    int nStatus;

    nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
    if (nStatus != ID_STATUS_OFFLINE)
        PostMessage(hwndContactList, WM_COMMAND, nStatus, 0);
}


// Disconnect all protocols.
// Happens on shutdown and standby.
static void DisconnectAll()
{
    PROTOCOLDESCRIPTOR **ppProtoDesc;
    int nProtoCount;
    int nProto;

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) & nProtoCount, (LPARAM) & ppProtoDesc);
    for (nProto = 0; nProto < nProtoCount; nProto++) {
        if (ppProtoDesc[nProto]->type == PROTOTYPE_PROTOCOL)
            CallProtoService(ppProtoDesc[nProto]->szName, PS_SETSTATUS, ID_STATUS_OFFLINE, 0);
    }
}

#define M_CREATECLC  (WM_USER+1)
LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	MSG m;
	m.hwnd = hwnd;
	m.message = msg;
	m.wParam = wParam;
	m.lParam = lParam;
	if (CallService(MS_CLIST_DOCKINGPROCESSMESSAGE, (WPARAM) & m, (LPARAM) & result))
		return result;
	if (CallService(MS_CLIST_TRAYICONPROCESSMESSAGE, (WPARAM) & m, (LPARAM) & result))
		return result;
	if (CallService(MS_CLIST_HOTKEYSPROCESSMESSAGE, (WPARAM) & m, (LPARAM) & result))
		return result;
    if (msg == uMsgProcessProfile) {
        char profile[MAX_PATH];
        int rc;
        // wParam = (ATOM)hProfileAtom, lParam = 0
        if (GlobalGetAtomName((ATOM) wParam, profile, sizeof(profile))) {
            char path[MAX_PATH];
            char file[MAX_PATH];
            char p[MAX_PATH];
            CallService(MS_DB_GETPROFILEPATH, sizeof(path), (LPARAM) & path);
            CallService(MS_DB_GETPROFILENAME, sizeof(file), (LPARAM) & file);
            _snprintf(p, sizeof(p), "%s\\%s", path, file);
            rc = lstrcmp(profile, p) == 0;
            ReplyMessage(rc);
            if (rc)
                ShowWindowAsync(hwnd, SW_SHOW);
        }
        return 0;
    }

    switch (msg) {
        case WM_CREATE:
            CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) GetMenu(hwnd), 0);
            DrawMenuBar(hwnd);

            //create the status wnd
            {
                int flags = WS_CHILD | CCS_BOTTOM;
                flags |= cluiopt.showsbar ? WS_VISIBLE : 0;
                flags |= cluiopt.showgrip ? SBARS_SIZEGRIP : 0;
                hwndStatus = CreateWindow(STATUSCLASSNAME, NULL, flags, 0, 0, 0, 0, hwnd, NULL, g_hInst, NULL);
            }
            CluiProtocolStatusChanged(0, 0);

            {
                MENUITEMINFO mii;
                ZeroMemory(&mii, sizeof(mii));
                mii.cbSize = MENUITEMINFO_V4_SIZE;
                mii.fMask = MIIM_TYPE | MIIM_DATA;
                himlMirandaIcon = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 1, 1);
                ImageList_AddIcon(himlMirandaIcon, LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
                mii.dwItemData = MENU_MIRANDAMENU;
                mii.fType = MFT_OWNERDRAW;
                mii.dwTypeData = NULL;
                SetMenuItemInfo(GetMenu(hwnd), 0, TRUE, &mii);
            }

            //delay creation of CLC so that it can get the status icons right the first time (needs protocol modules loaded)
            PostMessage(hwnd, M_CREATECLC, 0, 0);

            if (cluiopt.transparent) {
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
                if (MySetLayeredWindowAttributes)
                    MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), (BYTE) cluiopt.alpha,
                                                 LWA_ALPHA);
            }
            transparentFocus = 1;

#ifndef _DEBUG
            // Miranda is starting up! Restore last status mode.
            // This is not done in debug builds because frequent
            // reconnections will get you banned from the servers.
            RestoreMode();
#endif

            return FALSE;

        case M_CREATECLC:
            hwndContactTree = CreateWindow(CLISTCONTROL_CLASS, "",
                                           WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN
                                           | CLS_CONTACTLIST
                                           | (DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT) ? CLS_USEGROUPS : 0)
                                           | (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT) ? CLS_HIDEOFFLINE :
                                              0)
                                           | (DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_HIDEEMPTYGROUPS_DEFAULT) ?
                                              CLS_HIDEEMPTYGROUPS : 0), 0, 0, 0, 0, hwnd, NULL, g_hInst, NULL);
            SendMessage(hwnd, WM_SIZE, 0, 0);
            break;

            // Power management
        case WM_POWERBROADCAST:
        {
            switch ((DWORD) wParam) {

                case PBT_APMSUSPEND:
                    // Computer is suspending, disconnect all protocols
                    DisconnectAll();
                    break;

                case PBT_APMRESUMESUSPEND:
                    // Computer is resuming, restore all protocols
                    RestoreMode();
                    break;

            }
        }
            break;

        case WM_SYSCOLORCHANGE:
            SendMessage(hwndContactTree, msg, wParam, lParam);
            SendMessage(hwndStatus, msg, wParam, lParam);
            // XXX: only works with 4.71 with 95, IE4.
            SendMessage(hwndStatus, SB_SETBKCOLOR, 0, GetSysColor(COLOR_3DFACE));
            break;

        case WM_SIZE:
            if (IsZoomed(hwnd))
                ShowWindow(hwnd, SW_SHOWNORMAL);
            {
                RECT rect, rcStatus;
                GetClientRect(hwnd, &rect);
                if (cluiopt.showsbar) {
                    SetWindowPos(hwndStatus, NULL, 0, rect.bottom - 20, rect.right - rect.left, 20, SWP_NOZORDER);
                    GetWindowRect(hwndStatus, &rcStatus);
                    CluiProtocolStatusChanged(0, 0);
                }
                else
                    rcStatus.top = rcStatus.bottom = 0;
                SetWindowPos(hwndContactTree, NULL, 0, 0, rect.right, rect.bottom - (rcStatus.bottom - rcStatus.top), SWP_NOZORDER);
            }
            if (wParam == SIZE_MINIMIZED) {
                if (DBGetContactSettingByte(NULL, "CList", "Min2Tray", SETTING_MIN2TRAY_DEFAULT)) {
                    ShowWindow(hwnd, SW_HIDE);
                    DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_HIDDEN);
                }
                else
                    DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_MINIMIZED);
            }
            // drop thru
        case WM_MOVE:
            if (!IsIconic(hwnd)) {
                RECT rc;
                GetWindowRect(hwnd, &rc);

                if (!CallService(MS_CLIST_DOCKINGISDOCKED, 0, 0)) {     //if docked, dont remember pos (except for width)
                    DBWriteContactSettingDword(NULL, "CList", "Height", (DWORD) (rc.bottom - rc.top));
                    DBWriteContactSettingDword(NULL, "CList", "x", (DWORD) rc.left);
                    DBWriteContactSettingDword(NULL, "CList", "y", (DWORD) rc.top);
                }
                DBWriteContactSettingDword(NULL, "CList", "Width", (DWORD) (rc.right - rc.left));
            }
            return FALSE;

        case WM_SETFOCUS:
            SetFocus(hwndContactTree);
            return 0;

        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE) {
                if ((HWND) wParam != hwnd)
                    if (cluiopt.transparent)
                        if (transparentFocus)
                            SetTimer(hwnd, TM_AUTOALPHA, 250, NULL);
            }
            else {
                if (cluiopt.transparent) {
                    KillTimer(hwnd, TM_AUTOALPHA);
                    if (MySetLayeredWindowAttributes)
                        MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0),
                                                     (BYTE) cluiopt.alpha, LWA_ALPHA);
                    transparentFocus = 1;
                }
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_SETCURSOR:
            
			if(cluiopt.transparent) {
				if (!transparentFocus && GetForegroundWindow()!=hwnd && MySetLayeredWindowAttributes) {
					MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)cluiopt.alpha, LWA_ALPHA);
					transparentFocus=1;
					SetTimer(hwnd, TM_AUTOALPHA,250,NULL);
				}
			} 
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_NCHITTEST:
        {
            LRESULT result;
            result = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
            if (result == HTSIZE || result == HTTOP || result == HTTOPLEFT || result == HTTOPRIGHT ||
                result == HTBOTTOM || result == HTBOTTOMRIGHT || result == HTBOTTOMLEFT)
                if (DBGetContactSettingByte(NULL, "CLUI", "AutoSize", 0))
                    return HTCLIENT;
            return result;
        }

        case WM_TIMER:
            if ((int) wParam == TM_AUTOALPHA) {
                int inwnd;

                if (GetForegroundWindow() == hwnd) {
                    KillTimer(hwnd, TM_AUTOALPHA);
                    inwnd = 1;
                }
                else {
                    POINT pt;
                    HWND hwndPt;
                    pt.x = (short) LOWORD(GetMessagePos());
                    pt.y = (short) HIWORD(GetMessagePos());
                    hwndPt = WindowFromPoint(pt);
                    inwnd = (hwndPt == hwnd || GetParent(hwndPt) == hwnd);
                }
                if (inwnd != transparentFocus && MySetLayeredWindowAttributes) {        //change
                    transparentFocus = inwnd;
                    if (transparentFocus)
                        MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0),
                                                     (BYTE) cluiopt.alpha, LWA_ALPHA);
                    else
                        MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0),
                                                     (BYTE) DBGetContactSettingByte(NULL, "CList", "AutoAlpha", SETTING_AUTOALPHA_DEFAULT),
                                                     LWA_ALPHA);
                }
                if (!transparentFocus)
                    KillTimer(hwnd, TM_AUTOALPHA);
            }
            return TRUE;

        case WM_SHOWWINDOW:
        {
            static int noRecurse = 0;
            if (lParam)
                break;
            if (noRecurse)
                break;
            if (!DBGetContactSettingByte(NULL, "CLUI", "FadeInOut", 0) || !IsWinVer2000Plus())
                break;
            if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED) {
                DWORD thisTick, startTick;
                int sourceAlpha, destAlpha;
                if (wParam) {
                    sourceAlpha = 0;
                    destAlpha = (BYTE) cluiopt.alpha;
                    MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_ALPHA);
                    noRecurse = 1;
                    ShowWindow(hwnd, SW_SHOW);
                    noRecurse = 0;
                }
                else {
                    sourceAlpha = (BYTE) cluiopt.alpha;
                    destAlpha = 0;
                }
                for (startTick = GetTickCount();;) {
                    thisTick = GetTickCount();
                    if (thisTick >= startTick + 200)
                        break;
                    MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0),
                                                 (BYTE) (sourceAlpha + (destAlpha - sourceAlpha) * (int) (thisTick - startTick) / 200), LWA_ALPHA);
                }
                MySetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), (BYTE) destAlpha, LWA_ALPHA);
            }
            else {
                if (wParam)
                    SetForegroundWindow(hwnd);
                MyAnimateWindow(hwnd, 200, AW_BLEND | (wParam ? 0 : AW_HIDE));
                SetWindowPos(hwndContactTree, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
            }
            break;
        }
        case WM_MENURBUTTONUP: /* this API is so badly documented at MSDN!! */
        {
            UINT id = 0;

            id = GetMenuItemID((HMENU) lParam, LOWORD(wParam)); /* LOWORD(wParam) contains the menu pos in its parent menu */
            if (id != (-1))
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, 0), 0);
            break;
        }
        case WM_SYSCOMMAND:
            if (wParam == SC_MAXIMIZE)
                return 0;
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_COMMAND:
            if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_MAINMENU), (LPARAM) (HANDLE) NULL))
                break;
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                case ID_ICQ_EXIT:
                    if (CallService(MS_SYSTEM_OKTOEXIT, 0, 0))
                        DestroyWindow(hwnd);
                    break;
                case ID_TRAY_HIDE:
                    CallService(MS_CLIST_SHOWHIDE, 0, 0);
                    break;
                case POPUP_NEWGROUP:
                    SendMessage(hwndContactTree, CLM_SETHIDEEMPTYGROUPS, 0, 0);
                    CallService(MS_CLIST_GROUPCREATE, 0, 0);
                    break;
                case POPUP_HIDEOFFLINE:
                    CallService(MS_CLIST_SETHIDEOFFLINE, (WPARAM) (-1), 0);
                    break;
                case POPUP_HIDEOFFLINEROOT:
                    SendMessage(hwndContactTree, CLM_SETHIDEOFFLINEROOT, !SendMessage(hwndContactTree, CLM_GETHIDEOFFLINEROOT, 0, 0), 0);
                    break;
                case POPUP_HIDEEMPTYGROUPS:
                {
                    int newVal = !(GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_HIDEEMPTYGROUPS);
                    DBWriteContactSettingByte(NULL, "CList", "HideEmptyGroups", (BYTE) newVal);
                    SendMessage(hwndContactTree, CLM_SETHIDEEMPTYGROUPS, newVal, 0);
                    break;
                }
                case POPUP_DISABLEGROUPS:
                {
                    int newVal = !(GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_USEGROUPS);
                    DBWriteContactSettingByte(NULL, "CList", "UseGroups", (BYTE) newVal);
                    SendMessage(hwndContactTree, CLM_SETUSEGROUPS, newVal, 0);
                    break;
                }
                case POPUP_HIDEMIRANDA:
                {
                    CallService(MS_CLIST_SHOWHIDE, 0, 0);
                    break;
                }
            }
            return FALSE;
        case WM_KEYDOWN:
            CallService(MS_CLIST_MENUPROCESSHOTKEY, wParam, MPCF_MAINMENU | MPCF_CONTACTMENU);
            break;

        case WM_GETMINMAXINFO:
            DefWindowProc(hwnd, msg, wParam, lParam);
            ((LPMINMAXINFO) lParam)->ptMinTrackSize.x = 16 + GetSystemMetrics(SM_CXHTHUMB);
            ((LPMINMAXINFO) lParam)->ptMinTrackSize.y = 16;
            return 0;

        case WM_DISPLAYCHANGE:
            SendMessage(hwndContactTree, WM_SIZE, 0, 0);        //forces it to send a cln_listsizechanged
            break;

            //MSG FROM CHILD CONTROL
        case WM_NOTIFY:
            if (((LPNMHDR) lParam)->hwndFrom == hwndContactTree) {
                switch (((LPNMHDR) lParam)->code) {
                    case CLN_EXPANDED:
                    {
                        NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                        CallService(MS_CLIST_GROUPSETEXPANDED, (WPARAM) nmc->hItem, nmc->action);
                        return FALSE;
                    }
                    case CLN_DRAGGING:
                    {
                        NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                        ClientToScreen(hwnd, &nmc->pt);
                        if (!(nmc->flags & CLNF_ISGROUP))
                            if (NotifyEventHooks(hContactDraggingEvent, (WPARAM) nmc->hItem, MAKELPARAM(nmc->pt.x, nmc->pt.y))) {
                                SetCursor(LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER)));
                                return TRUE;
                            }
                        break;
                    }
                    case CLN_DRAGSTOP:
                    {
                        NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                        if (!(nmc->flags & CLNF_ISGROUP))
                            NotifyEventHooks(hContactDragStopEvent, (WPARAM) nmc->hItem, 0);
                        break;
                    }
                    case CLN_DROPPED:
                    {
                        NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                        ClientToScreen(hwnd, &nmc->pt);
                        if (!(nmc->flags & CLNF_ISGROUP))
                            if (NotifyEventHooks(hContactDroppedEvent, (WPARAM) nmc->hItem, MAKELPARAM(nmc->pt.x, nmc->pt.y))) {
                                SetCursor(LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER)));
                                return TRUE;
                            }
                        break;
                    }
                    case NM_KEYDOWN:
                    {
                        NMKEY *nmkey = (NMKEY *) lParam;
                        return CallService(MS_CLIST_MENUPROCESSHOTKEY, nmkey->nVKey, MPCF_MAINMENU | MPCF_CONTACTMENU);
                    }
                    case CLN_LISTSIZECHANGE:
                    {
                        NMCLISTCONTROL *nmc = (NMCLISTCONTROL *) lParam;
                        RECT rcWindow, rcTree, rcWorkArea;
                        int maxHeight, newHeight;

                        if (!DBGetContactSettingByte(NULL, "CLUI", "AutoSize", 0))
                            break;
                        if (CallService(MS_CLIST_DOCKINGISDOCKED, 0, 0))
                            break;
                        maxHeight = DBGetContactSettingByte(NULL, "CLUI", "MaxSizeHeight", 75);
                        GetWindowRect(hwnd, &rcWindow);
                        GetWindowRect(hwndContactTree, &rcTree);
                        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, FALSE);
                        newHeight = max(nmc->pt.y, 9) + 1 + (rcWindow.bottom - rcWindow.top) - (rcTree.bottom - rcTree.top);
                        if (newHeight > (rcWorkArea.bottom - rcWorkArea.top) * maxHeight / 100)
                            newHeight = (rcWorkArea.bottom - rcWorkArea.top) * maxHeight / 100;
                        if (DBGetContactSettingByte(NULL, "CLUI", "AutoSizeUpward", 0)) {
                            rcWindow.top = rcWindow.bottom - newHeight;
                            if (rcWindow.top < rcWorkArea.top)
                                rcWindow.top = rcWorkArea.top;
                        }
                        else {
                            rcWindow.bottom = rcWindow.top + newHeight;
                            if (rcWindow.bottom > rcWorkArea.bottom)
                                rcWindow.bottom = rcWorkArea.bottom;
                        }
                        SetWindowPos(hwnd, 0, rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                        break;
                    }
                    case NM_CLICK:
                    {
                        NMCLISTCONTROL *nm = (NMCLISTCONTROL *) lParam;
                        DWORD hitFlags;

                        if (SendMessage(hwndContactTree, CLM_HITTEST, (WPARAM) & hitFlags, MAKELPARAM(nm->pt.x, nm->pt.y)))
                            break;
                        if ((hitFlags & (CLCHT_NOWHERE | CLCHT_INLEFTMARGIN | CLCHT_BELOWITEMS)) == 0)
                            break;
                        if (DBGetContactSettingByte(NULL, "CLUI", "ClientAreaDrag", SETTING_CLIENTDRAG_DEFAULT)) {
                            POINT pt;
                            pt = nm->pt;
                            ClientToScreen(hwndContactTree, &pt);
                            return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
                        }
                        break;
                    }
                }
            }
            else if (((LPNMHDR) lParam)->hwndFrom == hwndStatus) {
                switch (((LPNMHDR) lParam)->code) {
                    case NM_CLICK:
                    {
                        unsigned int nParts, nPanel;
                        NMMOUSE *nm = (NMMOUSE *) lParam;
                        HMENU hMenu;
                        RECT rc;
                        POINT pt;

                        hMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
                        nParts = SendMessage(hwndStatus, SB_GETPARTS, 0, 0);
                        if (nm->dwItemSpec == 0xFFFFFFFE) {
                            nPanel = nParts - 1;
                            SendMessage(hwndStatus, SB_GETRECT, nPanel, (LPARAM) & rc);
                            if (nm->pt.x < rc.left)
                                return FALSE;
                        }
                        else {
                            nPanel = nm->dwItemSpec;
                        }
                        if (nParts > 1)
                            hMenu = GetSubMenu(hMenu, nPanel);
                        SendMessage(hwndStatus, SB_GETRECT, nPanel, (LPARAM) & rc);
                        pt.x = rc.left;
                        pt.y = rc.top;
                        ClientToScreen(hwndStatus, &pt);
                        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                    }
                }
            }
            return FALSE;
        case WM_CONTEXTMENU:
        {
            RECT rc;
            POINT pt;

            pt.x = (short) LOWORD(lParam);
            pt.y = (short) HIWORD(lParam);
            // x/y might be -1 if it was generated by a kb click                    
            GetWindowRect(hwndContactTree, &rc);
            if (pt.x == -1 && pt.y == -1) {
                // all this is done in screen-coords!
                GetCursorPos(&pt);
                // the mouse isnt near the window, so put it in the middle of the window
                if (!PtInRect(&rc, pt)) {
                    pt.x = rc.left + (rc.right - rc.left) / 2;
                    pt.y = rc.top + (rc.bottom - rc.top) / 2;
                }
            }
            if (PtInRect(&rc, pt)) {
                HMENU hMenu;
                hMenu = GetSubMenu(LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT)), 1);
                CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hMenu, 0);
                CheckMenuItem(hMenu, POPUP_HIDEOFFLINE,
                              DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT) ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem(hMenu, POPUP_HIDEOFFLINEROOT, SendMessage(hwndContactTree, CLM_GETHIDEOFFLINEROOT, 0, 0) ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem(hMenu, POPUP_HIDEEMPTYGROUPS,
                              GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_HIDEEMPTYGROUPS ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem(hMenu, POPUP_DISABLEGROUPS, GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_USEGROUPS ? MF_UNCHECKED : MF_CHECKED);
                TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
                return 0;
            }
            GetWindowRect(hwndStatus, &rc);
            if (PtInRect(&rc, pt)) {
                HMENU hMenu;
                if (DBGetContactSettingByte(NULL, "CLUI", "SBarRightClk", 0))
                    hMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);
                else
                    hMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
                TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                return 0;
            }
        }
            break;

        case WM_MEASUREITEM:
            if (((LPMEASUREITEMSTRUCT) lParam)->itemData == MENU_MIRANDAMENU) {
                ((LPMEASUREITEMSTRUCT) lParam)->itemWidth = GetSystemMetrics(SM_CXSMICON) * 4 / 3;
                ((LPMEASUREITEMSTRUCT) lParam)->itemHeight = 0;
                return TRUE;
            }
            return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
            if (dis->hwndItem == hwndStatus) {
                char *szProto = (char *) dis->itemData;
                int status, x;
                SIZE textSize;
                BYTE showOpts = DBGetContactSettingByte(NULL, "CLUI", "SBarShow", 1);
                status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
                SetBkMode(dis->hDC, TRANSPARENT);
                x = dis->rcItem.left;
                if (showOpts & 1) {
                    HICON hIcon;
                    hIcon = LoadSkinnedProtoIcon(szProto, status);
                    DrawIconEx(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - GetSystemMetrics(SM_CYSMICON)) >> 1, hIcon,
                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
                    x += GetSystemMetrics(SM_CXSMICON) + 2;
                }
                else
                    x += 2;
                if (showOpts & 2) {
                    char szName[64];
                    szName[0] = 0;
                    if (CallProtoService(szProto, PS_GETNAME, sizeof(szName), (LPARAM) szName)) {
                        strcpy(szName, szProto);
                    }           //if
                    if (lstrlen(szName) < sizeof(szName) - 1)
                        lstrcat(szName, " ");
                    GetTextExtentPoint32(dis->hDC, szName, lstrlen(szName), &textSize);
                    TextOut(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - textSize.cy) >> 1, szName, lstrlen(szName));
                    x += textSize.cx;
                }
                if (showOpts & 4) {
                    char *szStatus;
                    szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0);
                    if (!szStatus)
                        szStatus = "";
                    GetTextExtentPoint32(dis->hDC, szStatus, lstrlen(szStatus), &textSize);
                    TextOut(dis->hDC, x, (dis->rcItem.top + dis->rcItem.bottom - textSize.cy) >> 1, szStatus, lstrlen(szStatus));
                }
            }
            else if (dis->CtlType == ODT_MENU) {
                if (dis->itemData == MENU_MIRANDAMENU) {
                    HICON hIcon = ImageList_GetIcon(himlMirandaIcon, 0, ILD_NORMAL);
                    if (!IsWinVerXPPlus()) {
                        FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
                        if (dis->itemState & ODS_HOTLIGHT)
                            DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
                        else if (dis->itemState & ODS_SELECTED)
                            DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);
                        DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                                  (dis->rcItem.right + dis->rcItem.left - GetSystemMetrics(SM_CXSMICON)) / 2 +
                                  (dis->itemState & ODS_SELECTED ? 1 : 0),
                                  (dis->rcItem.bottom + dis->rcItem.top - GetSystemMetrics(SM_CYSMICON)) / 2 +
                                  (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0,
                                  DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                    }
                    else {
                        HBRUSH hBr;
                        BOOL bfm = FALSE;
                        SystemParametersInfo(SPI_GETFLATMENU, 0, &bfm, 0);
                        if (bfm) {
                            /* flat menus: fill with COLOR_MENUHILIGHT and outline with COLOR_HIGHLIGHT, otherwise use COLOR_MENUBAR */
                            if (dis->itemState & ODS_SELECTED || dis->itemState & ODS_HOTLIGHT) {
                                /* selected or hot lighted, no difference */
                                hBr = GetSysColorBrush(COLOR_MENUHILIGHT);
                                FillRect(dis->hDC, &dis->rcItem, hBr);
                                DeleteObject(hBr);
                                /* draw the frame */
                                hBr = GetSysColorBrush(COLOR_HIGHLIGHT);
                                FrameRect(dis->hDC, &dis->rcItem, hBr);
                                DeleteObject(hBr);
                            }
                            else {
                                /* flush the DC with the menu bar colour (only supported on XP) and then draw the icon */
                                hBr = GetSysColorBrush(COLOR_MENUBAR);
                                FillRect(dis->hDC, &dis->rcItem, hBr);
                                DeleteObject(hBr);
                            }   //if
                            /* draw the icon */
                            DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                                      (dis->rcItem.right + dis->rcItem.left - GetSystemMetrics(SM_CXSMICON)) / 2 +
                                      (dis->itemState & ODS_SELECTED ? 1 : 0),
                                      (dis->rcItem.bottom + dis->rcItem.top - GetSystemMetrics(SM_CYSMICON)) / 2 +
                                      (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0,
                                      DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                        }
                        else {
                            /* non-flat menus, flush the DC with a normal menu colour */
                            FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
                            if (dis->itemState & ODS_HOTLIGHT) {
                                DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
                            }
                            else if (dis->itemState & ODS_SELECTED) {
                                DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);
                            }   //if
                            DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                                      (dis->rcItem.right + dis->rcItem.left - GetSystemMetrics(SM_CXSMICON)) / 2 +
                                      (dis->itemState & ODS_SELECTED ? 1 : 0),
                                      (dis->rcItem.bottom + dis->rcItem.top - GetSystemMetrics(SM_CYSMICON)) / 2 +
                                      (dis->itemState & ODS_SELECTED ? 1 : 0), 0, 0,
                                      DST_ICON | (dis->itemState & ODS_INACTIVE ? DSS_DISABLED : DSS_NORMAL));
                        }       //if
                    }           //if
                    DestroyIcon(hIcon);
                    return TRUE;
                }
                return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
            }
            return 0;
        }
        case WM_CLOSE:
            if (DBGetContactSettingByte(NULL, "CList", "ToolWindow", SETTING_TOOLWINDOW_DEFAULT))
                CallService(MS_CLIST_SHOWHIDE, 0, 0);
            else
                SendMessage(hwnd, WM_COMMAND, ID_ICQ_EXIT, 0);

            return FALSE;
        case WM_DESTROY:
            if (!IsIconic(hwnd)) {
                RECT rc;
                GetWindowRect(hwnd, &rc);

                if (!CallService(MS_CLIST_DOCKINGISDOCKED, 0, 0)) {     //if docked, dont remember pos (except for width)
                    DBWriteContactSettingDword(NULL, "CList", "Height", (DWORD) (rc.bottom - rc.top));
                    DBWriteContactSettingDword(NULL, "CList", "x", (DWORD) rc.left);
                    DBWriteContactSettingDword(NULL, "CList", "y", (DWORD) rc.top);
                }
                DBWriteContactSettingDword(NULL, "CList", "Width", (DWORD) (rc.right - rc.left));
            }

            // Disconnect all protocols
            DisconnectAll();
			
            ShowWindow(hwnd, SW_HIDE);
            DestroyWindow(hwndContactTree);
            ImageList_Destroy(himlMirandaIcon);
            FreeLibrary(hUserDll);
            PostQuitMessage(0);
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return TRUE;
}

static int CluiIconsChanged(WPARAM wParam, LPARAM lParam)
{
    ImageList_ReplaceIcon(himlMirandaIcon, 0, LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
    DrawMenuBar(hwndContactList);
    return 0;
}

static HANDLE hRenameMenuItem;

static int MenuItem_PreBuild(WPARAM wParam, LPARAM lParam)
{
    char cls[128];
    HANDLE hItem;
    HWND hwndClist = GetFocus();
    CLISTMENUITEM mi;

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.flags = CMIM_FLAGS;
    GetClassName(hwndClist, cls, sizeof(cls));
    hwndClist = (!lstrcmp(CLISTCONTROL_CLASS, cls)) ? hwndClist : hwndContactList;
    hItem = (HANDLE) SendMessage(hwndClist, CLM_GETSELECTION, 0, 0);
    if (!hItem) {
        mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
    }
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hRenameMenuItem, (LPARAM) & mi);
    return 0;
}

static int MenuItem_RenameContact(WPARAM wParam, LPARAM lParam)
{
    char cls[128];
    HANDLE hItem;
    HWND hwndClist = GetFocus();
    GetClassName(hwndClist, cls, sizeof(cls));
    // worst case scenario, the rename is sent to the main contact list
    hwndClist = (!lstrcmp(CLISTCONTROL_CLASS, cls)) ? hwndClist : hwndContactList;
    hItem = (HANDLE) SendMessage(hwndClist, CLM_GETSELECTION, 0, 0);
    if (hItem) {
        SetFocus(hwndClist);
        SendMessage(hwndClist, CLM_EDITLABEL, (WPARAM) hItem, 0);
    }
    return 0;
}

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif

int LoadCLUIModule(void)
{
    WNDCLASS wndclass;
    DBVARIANT dbv;
    char titleText[256];

    uMsgProcessProfile = RegisterWindowMessage("Miranda::ProcessProfile");
	LoadCluiGlobalOpts();
    hUserDll = LoadLibrary("user32.dll");
    if (hUserDll) {
        MySetLayeredWindowAttributes = (BOOL(WINAPI *) (HWND, COLORREF, BYTE, DWORD)) GetProcAddress(hUserDll, "SetLayeredWindowAttributes");
        MyAnimateWindow = (BOOL(WINAPI *) (HWND, DWORD, DWORD)) GetProcAddress(hUserDll, "AnimateWindow");
    }
    HookEvent(ME_SYSTEM_MODULESLOADED, CluiModulesLoaded);
    HookEvent(ME_SKIN_ICONSCHANGED, CluiIconsChanged);
    HookEvent(ME_OPT_INITIALISE, CluiOptInit);
    hContactDraggingEvent = CreateHookableEvent(ME_CLUI_CONTACTDRAGGING);
    hContactDroppedEvent = CreateHookableEvent(ME_CLUI_CONTACTDROPPED);
    hContactDragStopEvent = CreateHookableEvent(ME_CLUI_CONTACTDRAGSTOP);
    LoadCluiServices();

    wndclass.style = CS_HREDRAW | CS_VREDRAW | (IsWinVerXPPlus()
                                                && DBGetContactSettingByte(NULL, "CList", "WindowShadow", 0) == 1 ? CS_DROPSHADOW : 0);
    wndclass.lpfnWndProc = ContactListWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = g_hInst;
    wndclass.hIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_CLISTMENU);
    wndclass.lpszClassName = MIRANDACLASS;

    RegisterClass(&wndclass);

    if (DBGetContactSetting(NULL, "CList", "TitleText", &dbv))
        lstrcpyn(titleText, MIRANDANAME, sizeof(titleText));
    else {
        lstrcpyn(titleText, dbv.pszVal, sizeof(titleText));
        DBFreeVariant(&dbv);
    }
    hwndContactList = CreateWindowEx(DBGetContactSettingByte(NULL, "CList", "ToolWindow", SETTING_TOOLWINDOW_DEFAULT) ? WS_EX_TOOLWINDOW : 0,
                                     MIRANDACLASS,
                                     titleText,
                                     (DBGetContactSettingByte(NULL, "CLUI", "ShowCaption", SETTING_SHOWCAPTION_DEFAULT) ? WS_CAPTION | WS_SYSMENU |
                                      WS_MINIMIZEBOX : 0) | WS_POPUPWINDOW | WS_THICKFRAME | WS_CLIPCHILDREN, (int) DBGetContactSettingDword(NULL,
                                                                                                                                             "CList",
                                                                                                                                             "x",
                                                                                                                                             700),
                                     (int) DBGetContactSettingDword(NULL, "CList", "y", 221), (int) DBGetContactSettingDword(NULL, "CList", "Width",
                                                                                                                             108),
                                     (int) DBGetContactSettingDword(NULL, "CList", "Height", 310), NULL, NULL, g_hInst, NULL);
    if (DBGetContactSettingByte(NULL, "CList", "OnDesktop", 0)) {
        HWND hProgMan = FindWindow("Progman", NULL);
        if (IsWindow(hProgMan))
            SetParent(hwndContactList, hProgMan);
    }

    {
        int state = DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);
        hMenuMain = GetMenu(hwndContactList);
        if (!DBGetContactSettingByte(NULL, "CLUI", "ShowMainMenu", SETTING_SHOWMAINMENU_DEFAULT))
            SetMenu(hwndContactList, NULL);
        if (state == SETTING_STATE_NORMAL)
            ShowWindow(hwndContactList, SW_SHOW);
        else if (state == SETTING_STATE_MINIMIZED)
            ShowWindow(hwndContactList, SW_SHOWMINIMIZED);
        SetWindowPos(hwndContactList, DBGetContactSettingByte(NULL, "CList", "OnTop", SETTING_ONTOP_DEFAULT) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0,
                     0, SWP_NOSIZE | SWP_NOMOVE);
    }
    {
        CLISTMENUITEM mi;
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.flags = 0;
        mi.pszContactOwner = NULL;      //on every contact
        CreateServiceFunction("CList/RenameContactCommand", MenuItem_RenameContact);
        mi.position = 2000050000;
        mi.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_RENAME));
        mi.pszName = Translate("&Rename");
        mi.pszService = "CList/RenameContactCommand";
        hRenameMenuItem = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
        HookEvent(ME_CLIST_PREBUILDCONTACTMENU, MenuItem_PreBuild);
    }
    return 0;
}
