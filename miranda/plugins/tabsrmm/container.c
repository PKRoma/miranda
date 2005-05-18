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
---------------------------------------------------------------------------

message window container implementation
Written by Nightwish (bof@hell.at.eu.org), Jul 2004

License: GPL

Just a few notes, how the container "works".
--------------------------------------------

The container has only one child window, a tab control with the id IDC_MSGTABS.
Each tab is assigned to a message window - the message windows are shown or hidden,
depending on the current tab selection. They are never disabled, just sent to the
background if not active.

Each tab "knows" the hwnd handle of his message dialog child, because at creation,
this handle is saved in the lparam member of the TCITEM structure assigned to
this tab.

Tab icons are stored in a global image list (g_hImageList). They are loaded at
plugin startup, or after a icon change event. Multiple containers can share the
same image list, because the list is read-only.

$Id$
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
#include "m_metacontacts.h"
#include "msgdlgutils.h"
#include "m_snapping_windows.h"
#include "functions.h"

#ifdef __MATHMOD_SUPPORT
//mathMod begin
#include "m_MathModule.h"
//mathMod end
#endif

#define SB_CHAR_WIDTH        45
#define SIDEBARWIDTH         30

char *szWarnClose = "Do you really want to close this session?";

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern HWND floaterOwner;

extern HANDLE hMessageWindowList;
extern struct CREOleCallback reOleCallback;
extern HINSTANCE g_hInst;

#define DEFAULT_CONTAINER_POS 0x00400040
#define DEFAULT_CONTAINER_SIZE 0x019001f4

struct ContainerWindowData *pFirstContainer = 0;        // the linked list of struct ContainerWindowData

int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd);
int GetTabItemFromMouse(HWND hwndTab, POINT *pt);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
int GetProtoIconFromList(const char *szProto, int iStatus);
void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc);
HMENU BuildContainerMenu();
void WriteStatsOnClose(HWND hwndDlg, struct MessageWindowData *dat);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount);
void ReflashContainer(struct ContainerWindowData *pContainer);
void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);

struct ContainerWindowData *AppendToContainerList(struct ContainerWindowData *pContainer);
struct ContainerWindowData *RemoveContainerFromList(struct ContainerWindowData *pContainer);

int EnumContainers(HANDLE hContact, DWORD dwAction, const TCHAR *szTarget, const TCHAR *szNew, DWORD dwExtinfo, DWORD dwExtinfoEx);
void DeleteContainer(int iIndex), RenameContainer(int iIndex, const TCHAR *newName);
#if defined(_UNICODE)
    void _DBWriteContactSettingWString(HANDLE hContact, char *szKey, char *szSetting, const wchar_t *value);
#endif    

static struct SIDEBARITEM sbarItems[] = {
    IDC_SBAR_SLIST, SBI_TOP, &myGlobals.g_sideBarIcons[0], "Session List",
    IDC_SBAR_FAVORITES, SBI_TOP, &myGlobals.g_sideBarIcons[1], "Favorite Contacts",
    IDC_SBAR_RECENT, SBI_TOP, &myGlobals.g_sideBarIcons[2], "Recent Sessions",
    IDC_SBAR_USERPREFS, SBI_TOP, &myGlobals.g_sideBarIcons[4], "Contact Preferences",
    IDC_SBAR_TOGGLEFORMAT, SBI_TOP | SBI_TOGGLE, &myGlobals.g_buttonBarIcons[20], "Quick toggle text formatting",
    1118, SBI_TOP, &myGlobals.g_buttonBarIcons[27], "Test",
    1119, SBI_TOP, &myGlobals.g_buttonBarIcons[27], "Test",
    1120, SBI_TOP, &myGlobals.g_buttonBarIcons[27], "Test",
    IDC_SBAR_SETUP, SBI_BOTTOM, &myGlobals.g_sideBarIcons[3], "Setup Sidebar",
    0, 0, 0, ""
};
extern BOOL CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcTabConfig(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

WNDPROC OldTabControlProc;

HMENU BuildMCProtocolMenu(HWND hwndDlg);
int IsMetaContact(HWND hwndDlg, struct MessageWindowData *dat);

struct ContainerWindowData *pLastActiveContainer = NULL;

/*
 * CreateContainer MUST malloc() a struct ContainerWindowData and pass its address
 * to CreateDialogParam() via the LPARAM. It also adds the struct to the linked list
 * of containers.
 *
 * The WM_DESTROY handler of the container DlgProc is responsible for free()'ing the
 * pointer and for removing the struct from the linked list.
 */

extern HMODULE hDLL;
extern PSLWA pSetLayeredWindowAttributes;

struct ContainerWindowData *CreateContainer(const TCHAR *name, int iTemp, HANDLE hContactFrom) {
    DBVARIANT dbv;
    char szCounter[10];
#if defined (_UNICODE)
    char *szKey = "TAB_ContainersW";
#else    
    char *szKey = "TAB_Containers";
#endif    
    int i, iFirstFree = -1, iFound = FALSE;

    struct ContainerWindowData *pContainer = (struct ContainerWindowData *)malloc(sizeof(struct ContainerWindowData));
    if (pContainer) {
        ZeroMemory((void *)pContainer, sizeof(struct ContainerWindowData));
        _tcsncpy(pContainer->szName, name, CONTAINER_NAMELEN + 1);
        AppendToContainerList(pContainer);
        
        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0) && !_tcscmp(name, _T("default")))
            iTemp |= CNT_CREATEFLAG_CLONED;
        /*
         * save container name to the db
         */
        i = 0;
        if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0)) {
            do {
                _snprintf(szCounter, 8, "%d", i);
                if (DBGetContactSetting(NULL, szKey, szCounter, &dbv)) {
                    if (iFirstFree != -1) {
                        pContainer->iContainerIndex = iFirstFree;
                        _snprintf(szCounter, 8, "%d", iFirstFree);
                    }
                    else {
                        pContainer->iContainerIndex = i;
                    }
    #if defined (_UNICODE)
                    _DBWriteContactSettingWString(NULL, szKey, szCounter, name);
    #else
                    DBWriteContactSettingString(NULL, szKey, szCounter, name);
    #endif                
                    BuildContainerMenu();
                    break;
                } else {
    #if defined (_UNICODE)
                    if (dbv.type == DBVT_ASCIIZ) {
                        WCHAR *wszString = Utf8_Decode(dbv.pszVal);
                        if(!_tcsncmp(wszString, name, CONTAINER_NAMELEN)) {
                            pContainer->iContainerIndex = i;
                            iFound = TRUE;
                        }
                        else if (!_tcsncmp(wszString, _T("**free**"), CONTAINER_NAMELEN)) {
                            iFirstFree =  i;
                        }
                        free(wszString);
                    }
    #else
                    if (dbv.type == DBVT_ASCIIZ) {
                        if (!strncmp(dbv.pszVal, name, CONTAINER_NAMELEN)) {        // found the name...
                            pContainer->iContainerIndex = i;
                            iFound = TRUE;
                        }
                        else if (!strncmp(dbv.pszVal, "**free**", CONTAINER_NAMELEN)) {
                            iFirstFree = i;
                        }
                    }
    #endif                
                    DBFreeVariant(&dbv);
                }
            } while ( ++i && iFound == FALSE);
        }
        else {
            iTemp |= CNT_CREATEFLAG_CLONED;
            pContainer->iContainerIndex = 1;
        }
        
        if(iTemp & CNT_CREATEFLAG_MINIMIZED)
            pContainer->dwFlags = CNT_CREATE_MINIMIZED;
        if(iTemp & CNT_CREATEFLAG_CLONED) {
            pContainer->dwFlags |= CNT_CREATE_CLONED;
            pContainer->hContactFrom = hContactFrom;
        }
        pContainer->hwnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGCONTAINER), NULL, DlgProcContainer, (LPARAM) pContainer);
        return pContainer;
    }
    return NULL;
}

/*
 * draws the sidebar, arranges the buttons, cares about hiding buttons if space
 * is not sufficient.
 * also, updates the scroll buttons on top and bottom of the sidebar
 */

void DrawSideBar(HWND hwndDlg, struct ContainerWindowData *pContainer, RECT *rc, int menuSep)
{
    int i, topCount = 0, bottomCount = 0, j = 0;
    int iSpaceAvail = (rc->bottom - rc->top) - menuSep - pContainer->statusBarHeight - 24;
    int iTopSpaceAvail = iSpaceAvail - (pContainer->sb_NrBottomButtons * 23);
    BOOL topEnabled = FALSE, bottomEnabled = FALSE;
    
    if(pContainer->sb_NrTopButtons * 23 <= iTopSpaceAvail)
        i = pContainer->sb_FirstButton = 0;
    else {
        if(pContainer->sb_FirstButton == 0)
            i = 0;
        else {
            i = pContainer->sb_NrTopButtons - (iTopSpaceAvail / 23);
            i = (i > pContainer->sb_FirstButton) ? pContainer->sb_FirstButton : i;
        }
    }
    while(sbarItems[j].uId != 0) {
        if(j < i) {
            ShowWindow(GetDlgItem(hwndDlg, sbarItems[j].uId), SW_HIDE);
            j++;
            continue;
        }
        if(sbarItems[j].dwFlags & SBI_TOP) {
            if(iTopSpaceAvail >= 23) {
                SetWindowPos(GetDlgItem(hwndDlg, sbarItems[j].uId), 0, 3, (topCount++ * 23) + 12, 22, 22, SWP_SHOWWINDOW | SWP_NOZORDER);
                iTopSpaceAvail -= 23;
            }
            else {
                ShowWindow(GetDlgItem(hwndDlg, sbarItems[j].uId), SW_HIDE);
                bottomEnabled = TRUE;
            }
        }
        else
            SetWindowPos(GetDlgItem(hwndDlg, sbarItems[j].uId), 0, 3, (rc->bottom - rc->top) - menuSep - pContainer->statusBarHeight - ((bottomCount++ * 23) + 34), 22, 22, SWP_NOZORDER);
        j++;
    }
    topEnabled = pContainer->sb_FirstButton ? TRUE : FALSE;
    MoveWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), 2, 2, SIDEBARWIDTH - 6, 10, TRUE);
    MoveWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), 2, (rc->bottom - rc->top) - 12 - menuSep - pContainer->statusBarHeight, SIDEBARWIDTH - 6, 10, TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), topEnabled);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), bottomEnabled);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), NULL, FALSE);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_SIDEBARUP), NULL, FALSE);
    //SetWindowPos(GetDlgItem(hwndDlg, IDC_SIDEBAR), HWND_BOTTOM, 0, 0, SIDEBARWIDTH - 2, (rc->bottom - rc->top) - menuSep - pContainer->statusBarHeight, SWP_DRAWFRAME);
}

/*
 * container window procedure...
 */

BOOL CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct ContainerWindowData *pContainer = 0;        // pointer to our struct ContainerWindowData
    int iItem = 0;
    TCITEM item;
    HWND  hwndTab;

    if(myGlobals.g_wantSnapping)
        CallSnappingWindowProc(hwndDlg, msg, wParam, lParam);
    
    pContainer = (struct ContainerWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    hwndTab = GetDlgItem(hwndDlg, IDC_MSGTABS);
    switch (msg) {
        case WM_INITDIALOG:
            {
                DWORD ws;
                HMENU hSysmenu;
                DWORD dwCreateFlags;
                int iMenuItems;
                int i = 0;
                
                //SetClassLong(hwndTab, GCL_STYLE, GetClassLong(hwndTab, GCL_STYLE) & ~(CS_HREDRAW | CS_VREDRAW));
                if(myGlobals.m_TabAppearance & TCF_FLAT) {
                    SetWindowLong(hwndTab, GWL_STYLE, GetWindowLong(hwndTab, GWL_STYLE) | (TCS_BUTTONS | TCS_FLATBUTTONS));
                    SendMessage(hwndTab, TCM_SETEXTENDEDSTYLE, TCS_EX_FLATSEPARATORS, 0);
                }
                pContainer = (struct ContainerWindowData *) lParam;
                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pContainer);
                pContainer->iLastClick = 0xffffffff;
                dwCreateFlags = pContainer->dwFlags;

                pContainer->isCloned = (pContainer->dwFlags & CNT_CREATE_CLONED);
                
                SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);          // set options...
                pContainer->dwFlags |= dwCreateFlags;

               
                /*
                 * create the sidebar buttons
                 */

                for(i = 0; sbarItems[i].uId != 0; i++) {
                    CreateWindowExA(0, MIRANDABUTTONCLASS, "", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 20, 20, hwndDlg, (HMENU)sbarItems[i].uId, g_hInst, NULL);
                    pContainer->sb_NrTopButtons += (sbarItems[i].dwFlags & SBI_TOP) ? 1 : 0;
                    pContainer->sb_NrBottomButtons += (sbarItems[i].dwFlags & SBI_BOTTOM) ? 1 : 0;
                }
                pContainer->sb_FirstButton = 0;
				/*
				 * additional system menu items...
				 */

				hSysmenu = GetSystemMenu(hwndDlg, FALSE);
                iMenuItems = GetMenuItemCount(hSysmenu);

                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_SEPARATOR, 0, "");
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_STAYONTOP, Translate("Stay on Top"));
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_NOTITLE, Translate("Hide titlebar"));
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_SEPARATOR, 0, "");
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_MOREOPTIONS, Translate("Container options..."));                 
                
                if(!(dwCreateFlags & CNT_CREATE_MINIMIZED))
                    SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 10);

                /*
                 * make the tab control the controlling parent window for all message dialogs
                 */

                ws = GetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_EXSTYLE);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_EXSTYLE, ws | WS_EX_CONTROLPARENT);

                /*
                 * subclass the tab control
                 */

                ws = GetWindowLong(hwndTab, GWL_STYLE);
                if(pContainer->dwFlags & CNT_SINGLEROWTABCONTROL) {
                    ws &= ~TCS_MULTILINE;
                    ws |= TCS_SINGLELINE;
                }
                else {
                    ws &= ~TCS_SINGLELINE;
                    ws |= TCS_MULTILINE;
                }
                SetWindowLong(hwndTab, GWL_STYLE, ws);
                OldTabControlProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_WNDPROC, (LONG)TabControlSubclassProc);
                SendMessage(hwndTab, EM_SUBCLASSED, 0, 0);
                
                /*
                 * assign the global image list...
                 */
                TabCtrl_SetImageList(GetDlgItem(hwndDlg, IDC_MSGTABS), myGlobals.g_hImageList);

                TabCtrl_SetPadding(GetDlgItem(hwndDlg, IDC_MSGTABS), DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 3), DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3));
                /*
                 * context menu
                 */
                pContainer->hMenuContext = myGlobals.g_hMenuContext;
                /*
                 * tab tooltips...
                 */
                pContainer->hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT,
                                                     CW_USEDEFAULT, CW_USEDEFAULT, hwndDlg, NULL, g_hInst, (LPVOID) NULL);

                if (pContainer->hwndTip) {
                    SetWindowPos(pContainer->hwndTip, HWND_TOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    TabCtrl_SetToolTips(GetDlgItem(hwndDlg, IDC_MSGTABS), pContainer->hwndTip);
                }
                
                if(pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
//                    SetWindowPos(dat->pContainer->hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE |SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOREPOSITION);
                    ShowWindow(hwndDlg, SW_SHOWMINNOACTIVE);
                }
                else {
                    SendMessage(hwndDlg, DM_RESTOREWINDOWPOS, 0, 0);
                    ShowWindow(hwndDlg, SW_SHOWNORMAL);
                    SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                }
                SetTimer(hwndDlg, TIMERID_HEARTBEAT, TIMEOUT_HEARTBEAT, NULL);
                SendMessage(hwndDlg, DM_SETSIDEBARBUTTONS, 0, 0);
                return TRUE;
            }

        case DM_RESTOREWINDOWPOS:
            {
#if defined (_UNICODE)
                char *szSetting = "CNTW_";
#else
                char *szSetting = "CNT_";
#endif                
                char szCName[CONTAINER_NAMELEN + 10];
                /*
                 * retrieve the container window geometry information from the database.
                 */

                if(pContainer->isCloned && pContainer->hContactFrom != 0) {
                    if (Utils_RestoreWindowPosition(hwndDlg, pContainer->hContactFrom, SRMSGMOD_T, "split")) {
                        if (Utils_RestoreWindowPositionNoMove(hwndDlg, pContainer->hContactFrom, SRMSGMOD_T, "split"))
                            if(Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, "split"))
                                if(Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, "split"))
                                    SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER);
                    }
                }
                else {
                    mir_snprintf(szCName, 40, "%s%d", szSetting, pContainer->iContainerIndex);
                    if (Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, szCName)) {
                        if (Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, szCName))
                            if(Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, "split"))
                                if(Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, "split"))
                                    SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER);
                    }
                }
                break;
            }
        case WM_COMMAND: {
            HANDLE hContact;
            struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
            DWORD dwOldFlags = pContainer->dwFlags;
            int i = 0;
            
            if(dat) {
                DWORD dwOldMsgWindowFlags = dat->dwFlags;
                DWORD dwOldEventIsShown = dat->dwEventIsShown;
                
                if(MsgWindowMenuHandler(pContainer->hwndActive, dat, LOWORD(wParam), MENU_PICMENU) == 1)
                    break;
                if(MsgWindowMenuHandler(pContainer->hwndActive, dat, LOWORD(wParam), MENU_LOGMENU) == 1) {
                    if(dat->dwFlags != dwOldFlags || dat->dwEventIsShown != dwOldEventIsShown) {
                        if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0)) {
                            WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                        }
                        else
                            SendMessage(hwndDlg, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, 0);
                    }
                    break;
                }
            }
            SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
            if(hContact) {
                if(CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) hContact))
                    break;
            }

            for(i = 0; sbarItems[i].uId != 0; i++) {
                if(LOWORD(wParam) == sbarItems[i].uId) {
                    switch(LOWORD(wParam)) {
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
                            SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                            if(hContact != 0)
                                CallService(MS_TABMSG_SETUSERPREFS, (WPARAM)hContact, 0);
                            break;
                        }
                        case IDC_SBAR_TOGGLEFORMAT:
                        {
                            struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
                            if(dat) {
                                if(IsDlgButtonChecked(hwndDlg, IDC_SBAR_TOGGLEFORMAT) == BST_UNCHECKED) {
                                    dat->SendFormat = 0;
                                    GetSendFormat(pContainer->hwndActive, dat, 0);
                                }
                                else {
                                    dat->SendFormat = SENDFORMAT_BBCODE;
                                    GetSendFormat(pContainer->hwndActive, dat, 0);
                                }
                                ConfigureSideBar(pContainer->hwndActive, dat);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            switch(LOWORD(wParam)) {
                case IDC_SIDEBARDOWN:
                case IDC_SIDEBARUP:
                {
                    RECT rc;

                    pContainer->sb_Sizecheck.cx = pContainer->sb_Sizecheck.cy = 0;
                    GetClientRect(hwndDlg, &rc);
                    if(LOWORD(wParam) == IDC_SIDEBARUP)
                        pContainer->sb_FirstButton = pContainer->sb_FirstButton > 0 ? --pContainer->sb_FirstButton : 0;
                    else
                        pContainer->sb_FirstButton += IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN)) ? 1 : 0;
                    DrawSideBar(hwndDlg, pContainer, &rc, 0);
                    SetFocus(GetDlgItem(pContainer->hwndActive, IDC_MESSAGE));
                    break;
                }
                case IDOK:
                    SendMessage(pContainer->hwndActive, WM_COMMAND, wParam, lParam);      // pass the IDOK command to the active child - fixes the "enter not working
                    break;
                case ID_FILE_SAVEMESSAGELOGAS:
                    SendMessage(pContainer->hwndActive, DM_SAVEMESSAGELOG, 0, 0);
                    break;
                case ID_FILE_CLOSEMESSAGESESSION:
                    PostMessage(pContainer->hwndActive, WM_CLOSE, 0, 1);
                    break;
                case ID_FILE_CLOSE:
                    PostMessage(hwndDlg, WM_CLOSE, 0, 1);
                    break;
                case ID_VIEW_SHOWSTATUSBAR:
                    pContainer->dwFlags ^= CNT_NOSTATUSBAR;
                    break;
                case ID_VIEW_VERTICALMAXIMIZE:
                    pContainer->dwFlags ^= CNT_VERTICALMAX;
                    break;
                case ID_VIEW_SHOWTOOLBAR:
                    pContainer->dwFlags ^= CNT_HIDETOOLBAR;
                    WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
                    return 0;
                case ID_VIEW_SHOWMENUBAR:
                    pContainer->dwFlags ^= CNT_NOMENUBAR;
                    break;
                case ID_VIEW_SHOWTITLEBAR:
                    pContainer->dwFlags ^= CNT_NOTITLE;
                    break;
                case ID_VIEW_TABSATBOTTOM:
                    pContainer->dwFlags ^= CNT_TABSBOTTOM;
                    break;
                case ID_TITLEBAR_USESTATICCONTAINERICON:
                    pContainer->dwFlags ^= CNT_STATICICON;
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    break;
                case ID_TITLEBAR_SHOWNICNAME:
                    pContainer->dwFlags ^= CNT_TITLE_SHOWNAME;
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return 0;
                case ID_TITLEBAR_SHOWSTATUS:
                    pContainer->dwFlags ^= CNT_TITLE_SHOWSTATUS;
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return 0;
                case ID_TITLEBAR_SHOWCONTAINERNAME:
                    pContainer->dwFlags |= CNT_TITLE_PREFIX;
                    pContainer->dwFlags &= ~CNT_TITLE_SUFFIX;
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return 0;
                case ID_TITLEBAR_SHOWCONTAINERNAMEASSUFFIX:
                    pContainer->dwFlags &= ~CNT_TITLE_PREFIX;
                    pContainer->dwFlags |= CNT_TITLE_SUFFIX;
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return 0;
                case ID_TITLEBAR_DONOTSHOWCONTAINERNAME:
                    pContainer->dwFlags &= ~(CNT_TITLE_PREFIX | CNT_TITLE_SUFFIX);
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return 0;
                case ID_VIEW_SHOWMULTISENDCONTACTLIST:
                    SendMessage(pContainer->hwndActive, WM_COMMAND, MAKEWPARAM(IDC_SENDMENU, ID_SENDMENU_SENDTOMULTIPLEUSERS), 0);
                    break;
                case ID_VIEW_STAYONTOP:
                    SendMessage(hwndDlg, WM_SYSCOMMAND, IDM_STAYONTOP, 0);
                    break;
                case ID_CONTAINER_CONTAINEROPTIONS:
                    SendMessage(hwndDlg, WM_SYSCOMMAND, IDM_MOREOPTIONS, 0);
                    break;
                case ID_EVENTPOPUPS_DISABLEALLEVENTPOPUPS:
                    pContainer->dwFlags &= ~(CNT_DONTREPORT | CNT_DONTREPORTUNFOCUSED | CNT_ALWAYSREPORTINACTIVE);
                    return 0;
                case ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISMINIMIZED:
                    pContainer->dwFlags ^= CNT_DONTREPORT;
                    return 0;
                case ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISUNFOCUSED:
                    pContainer->dwFlags ^= CNT_DONTREPORTUNFOCUSED;
                    return 0;
                case ID_EVENTPOPUPS_SHOWPOPUPSFORALLINACTIVESESSIONS:
                    pContainer->dwFlags ^= CNT_ALWAYSREPORTINACTIVE;
                    return 0;
                case ID_WINDOWFLASHING_DISABLEFLASHING:
                    pContainer->dwFlags |= CNT_NOFLASH;
                    pContainer->dwFlags &= ~CNT_FLASHALWAYS;
                    return 0;
                case ID_WINDOWFLASHING_FLASHUNTILFOCUSED:
                    pContainer->dwFlags |= CNT_FLASHALWAYS;
                    pContainer->dwFlags &= ~CNT_NOFLASH;
                    return 0;
                case ID_WINDOWFLASHING_USEDEFAULTVALUES:
                    pContainer->dwFlags &= ~(CNT_NOFLASH | CNT_FLASHALWAYS);
                    return 0;
                case ID_OPTIONS_APPLYCONTAINEROPTIONSGLOBALLY:
                {
                    struct ContainerWindowData *pCurrent = pFirstContainer;
                    while(pCurrent) {
                        if(pCurrent != pContainer) {
                            pCurrent->dwFlags = pContainer->dwFlags;
                            pCurrent->dwTransparency = pContainer->dwTransparency;
                            SendMessage(pCurrent->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                        }
                        pCurrent = pCurrent->pNextContainer;
                    }
                    EnumContainers(NULL, CNT_ENUM_WRITEFLAGS, NULL, NULL, pContainer->dwFlags, pContainer->dwTransparency);
                    return 0;
                }
                case ID_OPTIONS_SETCURRENTOPTIONSASDEFAULTVALUES:
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containerflags", pContainer->dwFlags);
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containertrans", pContainer->dwTransparency);
                    return 0;
                case ID_OPTIONS_SAVECURRENTWINDOWPOSITIONASDEFAULT:
                {
                    WINDOWPLACEMENT wp = {0};

                    wp.length = sizeof(wp);
                    if(GetWindowPlacement(hwndDlg, &wp)) {
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitx", wp.rcNormalPosition.left);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splity", wp.rcNormalPosition.top);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
                    }
                    return 0;
                }
            }
            if(pContainer->dwFlags != dwOldFlags)
                SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 0);
            break;
        }
        case WM_ENTERSIZEMOVE:
            {
                RECT rc;
                SIZE sz;
                GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
                sz.cx = rc.right - rc.left;
                sz.cy = rc.bottom - rc.top;
                pContainer->oldSize = sz;
                pContainer->bSizingLoop = TRUE;
                break;
            }
        case WM_EXITSIZEMOVE:
            {
                RECT rc;

                GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
                if (!((rc.right - rc.left) == pContainer->oldSize.cx && (rc.bottom - rc.top) == pContainer->oldSize.cy))
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                pContainer->bSizingLoop = FALSE;
                break;
            }
        case WM_GETMINMAXINFO: {
                RECT rc;
                POINT pt;
                MINMAXINFO *mmi = (MINMAXINFO *) lParam;

                mmi->ptMinTrackSize.x = 275;
                GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
                pt.y = rc.top;
                TabCtrl_AdjustRect(GetDlgItem(hwndDlg, IDC_MSGTABS), FALSE, &rc);
                if(pContainer->dwFlags & CNT_VERTICALMAX) {
                    WINDOWPLACEMENT wp;
                    RECT rcDesktop;

                    wp.length = sizeof(wp);
                    GetWindowPlacement(hwndDlg, &wp);
                    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);
                    mmi->ptMaxSize.x = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
                    mmi->ptMaxSize.y = rcDesktop.bottom - rcDesktop.top;
                    mmi->ptMaxPosition.x = wp.rcNormalPosition.left;
                    if(IsIconic(hwndDlg))
                        mmi->ptMaxPosition.y = rcDesktop.top;
                    else
                        mmi->ptMaxPosition.y = 0;
#if defined(__MATHMOD_SUPPORT)
                    if(myGlobals.m_MathModAvail) {
                        if(CallService(MTH_GET_PREVIEW_SHOWN, 0, 0)) {
                            RECT rc;
                            HWND hwndMath = FindWindowA("TfrmPreview", "Preview");
                            GetWindowRect(hwndMath, &rc);
                            mmi->ptMaxSize.y -= (rc.bottom - rc.top);
                        }
                    }
#endif
                }
                return 0;
            }
#ifdef __MATHMOD_SUPPORT    		
            //mathMod begin
    		case WM_MOVE:
    		if(myGlobals.m_MathModAvail) {
                TMathWindowInfo mathWndInfo;
    			RECT windRect;
    			GetWindowRect(hwndDlg, &windRect);
    			mathWndInfo.top=windRect.top;
    			mathWndInfo.left=windRect.left;
    			mathWndInfo.right=windRect.right - 5;
    			mathWndInfo.bottom=windRect.bottom - 5;
                CallService(MTH_RESIZE,0,(LPARAM) &mathWndInfo);
    		}
            break;
    		//mathMod end
#endif            
        case WM_SIZE: { 
                RECT rcClient, rcUnadjusted;
                int i = 0;
                TCITEM item = {0};
                POINT pt = {0};
                DWORD menuSep, sidebarwidth;;
                
                if(IsIconic(hwndDlg)) {
                    pContainer->dwFlags |= CNT_DEFERREDSIZEREQUEST;
                    break;
                }
                if(wParam == SIZE_MINIMIZED)
                    break;

                if(LOWORD(lParam) == pContainer->sb_Sizecheck.cx && HIWORD(lParam) == pContainer->sb_Sizecheck.cy && lParam != 0)
                    break;

                if(lParam != 0) {
                    pContainer->sb_Sizecheck.cx = LOWORD(lParam);
                    pContainer->sb_Sizecheck.cy = HIWORD(lParam);
                }
                //_DebugPopup(0, "size: %d, %d", LOWORD(lParam), HIWORD(lParam));
                SendMessage(hwndTab, EM_SEARCHSCROLLER, 0, 0);
                if (pContainer->hwndStatus) {
                    RECT rcs;
                    int statwidths[5];
                    
                    SendMessage(pContainer->hwndStatus, WM_SIZE, 0, 0);
                    GetWindowRect(pContainer->hwndStatus, &rcs);
                    statwidths[0] = (rcs.right - rcs.left) - (2 * SB_CHAR_WIDTH) - 24 - (myGlobals.g_SecureIMAvail ? 24 : 0);
                    statwidths[1] = (rcs.right - rcs.left) - (SB_CHAR_WIDTH - 8) - 24 - (myGlobals.g_SecureIMAvail ? 24 : 0);
                    statwidths[2] = (rcs.right - rcs.left) - (myGlobals.g_SecureIMAvail ? (38 + 24) : 38);
                    statwidths[3] = myGlobals.g_SecureIMAvail ? (rcs.right - rcs.left) - 38 : -1;
                    statwidths[4] = -1;
                    SendMessage(pContainer->hwndStatus, SB_SETPARTS, myGlobals.g_SecureIMAvail ? 5 : 4, (LPARAM) statwidths);
                    pContainer->statusBarHeight = (rcs.bottom - rcs.top) + 1;
                }
                else
                    pContainer->statusBarHeight = 0;
                
                sidebarwidth = pContainer->dwFlags & CNT_SIDEBAR ? SIDEBARWIDTH : 0;
                
                GetClientRect(hwndDlg, &rcClient);
                rcUnadjusted = rcClient;
                menuSep = 0;
                if (lParam) {
                    if(pContainer->dwFlags & CNT_TABSBOTTOM)
                        MoveWindow(hwndTab, pContainer->tBorder_outer + sidebarwidth, pContainer->tBorder_outer + menuSep, (rcClient.right - rcClient.left) - (2 * pContainer->tBorder_outer) - sidebarwidth, (rcClient.bottom - rcClient.top) - menuSep - pContainer->statusBarHeight - ( 2 * pContainer->tBorder_outer), FALSE);
                    else
                        MoveWindow(hwndTab, pContainer->tBorder_outer + sidebarwidth, pContainer->tBorder_outer + menuSep, (rcClient.right - rcClient.left) - (2 * pContainer->tBorder_outer) - sidebarwidth, (rcClient.bottom - rcClient.top) - menuSep - pContainer->statusBarHeight - (pContainer->tBorder_outer * 2), FALSE);
                }
                AdjustTabClientRect(pContainer, &rcClient);

                /*
                 * we care about all client sessions, but we really resize only the active tab (hwndActive)
                 * we tell inactive tabs to resize theirselves later when they get activated (DM_CHECKSIZE
                 * just queues a resize request)
                 */
                for(i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
                    item.mask = TCIF_PARAM;
                    TabCtrl_GetItem(hwndTab, i, &item);
                    if((HWND)item.lParam == pContainer->hwndActive) {
                        MoveWindow((HWND)item.lParam, rcClient.left, rcClient.top, (rcClient.right - rcClient.left), (rcClient.bottom - rcClient.top), TRUE);
                        //XXX RedrawWindow(GetDlgItem((HWND)item.lParam, IDC_LOG), NULL, NULL, RDW_INVALIDATE);
                        if(!pContainer->bSizingLoop) {
                            SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                        }
                    }
                    else
                        SendMessage((HWND)item.lParam, DM_CHECKSIZE, 0, 0);
                }
                if(wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 1);
                
                if(lParam != 0) {
                    RedrawWindow(hwndTab, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | (pContainer->bSizingLoop ? RDW_ERASE : 0));
                    RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | (pContainer->bSizingLoop ? RDW_ERASE : 0));
                }
                
                if (pContainer->hwndStatus && pContainer->bSizingLoop) {
                    InvalidateRect(pContainer->hwndStatus, NULL, FALSE);
                }
                
                if(pContainer->dwFlags & CNT_SIDEBAR)
                    DrawSideBar(hwndDlg, pContainer, &rcUnadjusted, menuSep);
#ifdef __MATHMOD_SUPPORT    			
                //mathMod begin
    			if(myGlobals.m_MathModAvail) {  //start scope
    						TMathWindowInfo mathWndInfo;
    												
    						RECT windRect;
    						// größe ermitteln:
    						GetWindowRect(hwndDlg,&windRect);
    						mathWndInfo.top=windRect.top;
    						mathWndInfo.left=windRect.left;
    						mathWndInfo.right=windRect.right - 5;
    						mathWndInfo.bottom=windRect.bottom - 5;
    						CallService(MTH_RESIZE,0,(LPARAM) &mathWndInfo);
    			}  // end scope
    			//mathMod end
#endif                
            break;
            } 
        case DM_UPDATETITLE: {
                HANDLE hContact = (HANDLE) wParam;
                char *szProto = 0, *szStatus = 0, *contactName = NULL, temp[200];
                TCHAR newtitle[256], oldtitle[256];
                WORD wStatus = -1;
                TCHAR tTemp[204];
                char *dummy = "", *uin;
                struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);

                if(dat) {
                    if(dat->uin && lstrlenA(dat->uin) > 0)
                        uin = dat->uin;
                    else
                        uin = dummy;
                    szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                }
                else
                    uin = dummy;
                
                if(wParam == 0)             // no hContact given - obtain the hContact for the active tab
                    SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                
                if(pContainer->dwFlags & CNT_TITLE_SHOWNAME)
                    contactName = dat->szNickname;
                
                temp[0] = '\0';
                tTemp[0] = '\0';
                
                if (szProto)
                    SendMessage(pContainer->hwndActive, DM_QUERYSTATUS, 0, (LPARAM)&wStatus);
                else
                    break;
                if(pContainer->dwFlags & CNT_TITLE_SHOWNAME && pContainer->dwFlags & CNT_TITLE_SHOWUIN)
                    mir_snprintf(temp, sizeof(temp), "%s - %s", contactName, uin);
                else if(pContainer->dwFlags & CNT_TITLE_SHOWNAME)
                    mir_snprintf(temp, sizeof(temp), "%s", contactName);
                else if(pContainer->dwFlags & CNT_TITLE_SHOWUIN)
                    mir_snprintf(temp, sizeof(temp), "%s", uin);
                    
                if((pContainer->dwFlags & CNT_TITLE_SHOWNAME || pContainer->dwFlags & CNT_TITLE_SHOWUIN) && pContainer->dwFlags & CNT_TITLE_SHOWSTATUS) {
                    szStatus = dat->szStatus;
                    if(szStatus != NULL) {
                        strncat(temp, " (", sizeof(temp));
                        strncat(temp, szStatus, sizeof(temp));
                        strncat(temp, ")", sizeof(temp));
                    }
                }
#if defined (_UNICODE)
                MultiByteToWideChar(CP_ACP, 0, temp, -1, tTemp, 200);
#else
                strncpy(tTemp, temp, 200);
#endif                
                if(pContainer->dwFlags & CNT_TITLE_PREFIX) {
                    if(!_tcsncmp(pContainer->szName, _T("default"), CONTAINER_NAMELEN) && _tcslen(pContainer->szName) == 7)
                        _sntprintf(newtitle, 255, _T("[%s] %s"), myGlobals.g_szDefaultContainerName, tTemp);
                    else
                        _sntprintf(newtitle, 255, _T("[%s] %s"), pContainer->szName, tTemp);
                }
                else if(pContainer->dwFlags & CNT_TITLE_SUFFIX) {
                    if(!_tcsncmp(pContainer->szName, _T("default"), CONTAINER_NAMELEN) && _tcslen(pContainer->szName) == 7)
                        _sntprintf(newtitle, 255, _T("%s [%s]"), tTemp, myGlobals.g_szDefaultContainerName);
                    else
                        _sntprintf(newtitle, 255, _T("%s [%s]"), tTemp, pContainer->szName);
                }
                else
                    _tcsncpy(newtitle, tTemp, 255);
                
                GetWindowText(hwndDlg, oldtitle, 255);
#if defined _UNICODE
                if(lstrcmpW(newtitle, oldtitle))
                    SetWindowTextW(hwndDlg, (LPCWSTR) newtitle);
#else
                if(lstrcmpA(newtitle, oldtitle))
                   SetWindowTextA(hwndDlg, (LPCSTR) newtitle);
#endif                
                if (wStatus != (WORD)-1)
                    SendMessage(hwndDlg, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadSkinnedProtoIcon(szProto, wStatus));
                break;
            }               
        case WM_TIMER:
            if(wParam == TIMERID_HEARTBEAT) {
                int i;
                TCITEM item = {0};
                DWORD dwTimeout;
                
                item.mask = TCIF_PARAM;
                if((dwTimeout = myGlobals.m_TabAutoClose) > 0) {
                    int clients = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_MSGTABS));
                    HWND *hwndClients = (HWND *)malloc(sizeof(HWND) * ( clients + 1));
                    for(i = 0; i < clients; i++) {
                        TabCtrl_GetItem(hwndTab, i, &item);
                        hwndClients[i] = (HWND)item.lParam;
                    }
                    for(i = 0; i < clients; i++) {
                        if(IsWindow(hwndClients[i])) {
                            if((HWND)hwndClients[i] != pContainer->hwndActive)
                                pContainer->bDontSmartClose = TRUE;
                            SendMessage((HWND)hwndClients[i], DM_CHECKAUTOCLOSE, (WPARAM)(dwTimeout * 60), 0);
                            pContainer->bDontSmartClose = FALSE;
                        }
                    }
                    free(hwndClients);
                }
            }
            break;
        case WM_SYSCOMMAND:
            switch (wParam) {
                case IDM_STAYONTOP:
                    SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    CheckMenuItem(GetSystemMenu(hwndDlg, FALSE), IDM_STAYONTOP, (pContainer->dwFlags & CNT_STICKY) ? MF_BYCOMMAND | MF_UNCHECKED : MF_BYCOMMAND | MF_CHECKED);
                    pContainer->dwFlags ^= CNT_STICKY;
                    break;
                case IDM_NOTITLE: {
                        DWORD ws;
                        RECT rc;

                        pContainer->oldSize.cx = 0;
                        pContainer->oldSize.cy = 0;

                        CheckMenuItem(GetSystemMenu(hwndDlg, FALSE), IDM_NOTITLE, (pContainer->dwFlags & CNT_NOTITLE) ? MF_BYCOMMAND | MF_UNCHECKED : MF_BYCOMMAND | MF_CHECKED);
                        ws = GetWindowLong(hwndDlg, GWL_STYLE);
                        ws = (pContainer->dwFlags & CNT_NOTITLE) ? ws | WS_CAPTION : ws & ~WS_CAPTION; 
                        SetWindowLong(hwndDlg, GWL_STYLE, ws);
                        pContainer->dwFlags ^= CNT_NOTITLE;
                        GetWindowRect(hwndDlg, &rc);
                        MoveWindow(hwndDlg, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, TRUE);
                        MoveWindow(hwndDlg, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
                        RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
                        SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                        break;
                    }
                case IDM_MOREOPTIONS:
                    if(IsIconic(pContainer->hwnd))
                        SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                    if(pContainer->hWndOptions == 0)
                        CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM)pContainer);
					break;
                case SC_MAXIMIZE:
                    pContainer->oldSize.cx = pContainer->oldSize.cy = 0;
                    break;
                case SC_RESTORE:
                    pContainer->oldSize.cx = pContainer->oldSize.cy = 0;
                    if(nen_options.bMinimizeToTray && pContainer->bInTray) {
                        if(pContainer->bInTray == 2) {
                            MaximiseFromTray(hwndDlg, FALSE, &pContainer->restoreRect);
                            PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
                        }
                        else
                            MaximiseFromTray(hwndDlg, nen_options.bAnimated, &pContainer->restoreRect);
                        DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)pContainer->iContainerIndex + 1, MF_BYCOMMAND);
                        pContainer->bInTray = 0;
                        return 0;
                    }
                    break;
                case SC_MINIMIZE:
                    if(nen_options.bMinimizeToTray && (nen_options.bTraySupport || nen_options.floaterMode) && !(nen_options.bFloaterOnlyMin) && myGlobals.m_WinVerMajor >= 5) {
                        pContainer->bInTray = IsZoomed(hwndDlg) ? 2 : 1;
                        GetWindowRect(hwndDlg, &pContainer->restoreRect);
                        MinimiseToTray(hwndDlg, nen_options.bAnimated);
                        return 0;
                    }
                    break;
            }
            break;
        case DM_SELECTTAB:
            {
                switch (wParam) {
                    int iItems, iCurrent, iNewTab;
                    TCITEM item;

                    case DM_SELECT_BY_HWND:
                        ActivateTabFromHWND(hwndTab, (HWND) lParam);
                        break;
                    case DM_SELECT_NEXT:
                    case DM_SELECT_PREV:
                    case DM_SELECT_BY_INDEX:
                        iItems = TabCtrl_GetItemCount(hwndTab);
                        iCurrent = TabCtrl_GetCurSel(hwndTab);

                        if (iItems == 1)
                            break;
                        if (wParam == DM_SELECT_PREV)
                            iNewTab = iCurrent ? iCurrent - 1 : iItems - 1;     // cycle if current is already the leftmost tab..
                        else if(wParam == DM_SELECT_NEXT)
                            iNewTab = (iCurrent == (iItems - 1)) ? 0 : iCurrent + 1;
                        else if(wParam == DM_SELECT_BY_INDEX) {
                            if((int)lParam > iItems)
                                break;
                            iNewTab = lParam - 1;
                        }

                        if (iNewTab != iCurrent) {
                            ZeroMemory((void *)&item, sizeof(item));
                            item.mask = TCIF_PARAM;
                            if (TabCtrl_GetItem(hwndTab, iNewTab, &item)) {
                                TabCtrl_SetCurSel(hwndTab, iNewTab);
                                ShowWindow(pContainer->hwndActive, SW_HIDE);
                                pContainer->hwndActive = (HWND) item.lParam;
                                ShowWindow((HWND)item.lParam, SW_SHOW);
                                SetFocus(pContainer->hwndActive);
                            }
                        }
                        break;
                }
                break;
            }
            case WM_NOTIFY: {
                NMHDR* pNMHDR = (NMHDR*) lParam;
                if(pContainer != NULL && pContainer->hwndStatus != 0 && ((LPNMHDR)lParam)->hwndFrom == pContainer->hwndStatus) {
                    switch (((LPNMHDR)lParam)->code) {
                        case NM_CLICK:
                        {
                            unsigned int nParts, nPanel;
                            NMMOUSE *nm=(NMMOUSE*)lParam;
                            RECT rc;

                            nParts = SendMessage(pContainer->hwndStatus, SB_GETPARTS, 0, 0);
                            if (nm->dwItemSpec == 0xFFFFFFFE) {
                                nPanel = nParts -2;
                                SendMessage(pContainer->hwndStatus, SB_GETRECT, nPanel, (LPARAM)&rc);
                                if (nm->pt.x > rc.left && nm->pt.x < rc.right) 
                                    goto panel_found;
                                else {
                                    nPanel = nParts -1;
                                    SendMessage(pContainer->hwndStatus, SB_GETRECT, nPanel, (LPARAM)&rc);
                                    if (nm->pt.x < rc.left) 
                                        return FALSE;
                                }
                            } 
                            else { 
                                nPanel = nm->dwItemSpec; 
                            }
panel_found:                            
                            if(nPanel == 0) {
                                SendMessage(pContainer->hwndStatus, SB_GETRECT, 0, (LPARAM)&rc);
                                if(nm->pt.x > rc.left && nm->pt.x < rc.left + 18) {
                                    if(((LPNMHDR)lParam)->code == NM_CLICK)
                                        SendMessage(myGlobals.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_LBUTTONUP);
                                    break;
                                }
                            }
                            else if(nPanel == nParts - 1)
                                SendMessage(pContainer->hwndActive, WM_COMMAND, IDC_SELFTYPING, 0);
                            else if(nPanel == nParts - 2) {
                                if(GetKeyState(VK_SHIFT) & 0x8000) {
                                    struct ContainerWindowData *piContainer = pFirstContainer;
                                    while(piContainer) {
                                        piContainer->dwFlags = ((pContainer->dwFlags & CNT_NOSOUND) ? piContainer->dwFlags | CNT_NOSOUND : piContainer->dwFlags & ~CNT_NOSOUND);
                                        SendMessage(piContainer->hwndActive, DM_STATUSBARCHANGED, 0, 0);
                                        piContainer = piContainer->pNextContainer;
                                    }
                                    break;
                                }
                                pContainer->dwFlags ^= CNT_NOSOUND;
                                SendMessage(pContainer->hwndActive, DM_STATUSBARCHANGED, 0, 0);
                            }
                        }
                    }
                    break;
                }
                switch (pNMHDR->code) {
                    case TCN_SELCHANGE: 
                    {
                        ZeroMemory((void *)&item, sizeof(item));
                        iItem = TabCtrl_GetCurSel(hwndTab);
                        item.lParam = 0;
                        item.mask = TCIF_PARAM;
                        if (TabCtrl_GetItem(hwndTab, iItem, &item)) {
                            ShowWindow((HWND)item.lParam, SW_SHOW);
                            if((HWND)item.lParam != pContainer->hwndActive)
                                ShowWindow(pContainer->hwndActive, SW_HIDE);
                            pContainer->hwndActive = (HWND) item.lParam;
                            if(!IsIconic(hwndDlg))
                                SetFocus(pContainer->hwndActive);
                        }
                        break;
                    }
                        /*
                         * tooltips
                         */
                    case TTN_GETDISPINFO: {
                            POINT pt;
                            int   iItem;
                            TCITEM item;
                            NMTTDISPINFO *nmtt = (NMTTDISPINFO *) lParam;
                            struct MessageWindowData *cdat = 0;
                            char *contactName = 0, *szStatus = 0;
                            char szTtitle[80];
#if defined ( _UNICODE )
                            wchar_t w_contactName[80];
#endif

                            GetCursorPos(&pt);
                            if ((iItem = GetTabItemFromMouse(hwndTab, &pt)) == -1)
                                break;
                            ZeroMemory((void *)&item, sizeof(item));
                            item.mask = TCIF_PARAM;
                            TabCtrl_GetItem(hwndTab, iItem, &item);
                            if (item.lParam) {
                                cdat = (struct MessageWindowData *) GetWindowLong((HWND)item.lParam, GWL_USERDATA);
                                if (cdat) {
                                    contactName = cdat->szNickname;
                                    if (contactName) {
                                        if (cdat->szProto) {
                                            szStatus = cdat->szStatus;
                                            nmtt->hinst = NULL;
                                            mir_snprintf(szTtitle, sizeof(szTtitle), "%s (%s)", contactName, szStatus);
#if defined ( _UNICODE )
                                            MultiByteToWideChar(CP_ACP, 0, szTtitle, -1, w_contactName, sizeof(w_contactName));
                                            lstrcpynW(nmtt->szText, w_contactName, 80);
                                            nmtt->szText[79] = (wchar_t) '\0';
#else
                                            lstrcpynA(nmtt->szText, szTtitle, 80);
                                            nmtt->szText[79] = '\0';
#endif
                                            nmtt->lpszText = nmtt->szText;
                                            nmtt->uFlags = TTF_IDISHWND;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    /*
                     * double click
                     */
                    case NM_CLICK: {
                            FILETIME ft;
                            POINT pt, pt1;

                            GetCursorPos(&pt);
                            pt1 = pt;
                            GetSystemTimeAsFileTime(&ft);
                            if (abs(pt.x - pContainer->pLastPos.x) < 5 && abs(pt.y - pContainer->pLastPos.y) < 5) {
                                if ((ft.dwLowDateTime - pContainer->iLastClick) < (GetDoubleClickTime() * 10000)) {
                                    SendMessage(hwndDlg, DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
                                }
                            }
                            pContainer->iLastClick = ft.dwLowDateTime;
                            pContainer->pLastPos = pt1;
                            break;
                        }
                        /* 
                         * context menu...
                         */
                    case NM_RCLICK: {
                            HMENU subMenu;
                            POINT pt;
                            int iSelection, iItem;

                            GetCursorPos(&pt);
                            subMenu = GetSubMenu(pContainer->hMenuContext, 0);
                            
                            EnableMenuItem(subMenu, ID_TABMENU_ATTACHTOCONTAINER, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0) || DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0) ? MF_GRAYED : MF_ENABLED);
                            
                            iSelection = TrackPopupMenu(subMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                            if(iSelection >= IDM_CONTAINERMENU) {
                                DBVARIANT dbv = {0};
                                TCITEM item = {0};
                                char szIndex[10];
#if defined (_UNICODE)
                                char *szKey = "TAB_ContainersW";
#else    
                                char *szKey = "TAB_Containers";
#endif    
                                if ((iItem = GetTabItemFromMouse(hwndTab, &pt)) == -1)
                                    break;
                                item.mask = TCIF_PARAM;
                                TabCtrl_GetItem(hwndTab, iItem, &item);
                                mir_snprintf(szIndex, 8, "%d", iSelection - IDM_CONTAINERMENU);
                                if(iSelection - IDM_CONTAINERMENU >= 0) {
                                    if(!DBGetContactSetting(NULL, szKey, szIndex, &dbv)) {
#if defined (_UNICODE)
                                        WCHAR *wszTemp = Utf8_Decode(dbv.pszVal);
                                        SendMessage((HWND)item.lParam, DM_CONTAINERSELECTED, 0, (LPARAM) wszTemp);
                                        free(wszTemp);
#else
                                        SendMessage((HWND)item.lParam, DM_CONTAINERSELECTED, 0, (LPARAM) dbv.pszVal);
#endif
                                        DBFreeVariant(&dbv);
                                    }
                                }
                                return 1;
                            }
                            switch (iSelection) {
                                case ID_TABMENU_CLOSETAB:
                                    SendMessage(hwndDlg, DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
                                    break;
                                case ID_TABMENU_SWITCHTONEXTTAB:
                                    SendMessage(hwndDlg, DM_SELECTTAB, (WPARAM) DM_SELECT_NEXT, 0);
                                    break;
                                case ID_TABMENU_SWITCHTOPREVIOUSTAB:
                                    SendMessage(hwndDlg, DM_SELECTTAB, (WPARAM) DM_SELECT_PREV, 0);
                                    break;
                                case ID_TABMENU_ATTACHTOCONTAINER:
                                    if ((iItem = GetTabItemFromMouse(hwndTab, &pt)) == -1)
                                        break;
                                    ZeroMemory((void *)&item, sizeof(item));
                                    item.mask = TCIF_PARAM;
                                    TabCtrl_GetItem(hwndTab, iItem, &item);
                                    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SELECTCONTAINER), hwndDlg, SelectContainerDlgProc, (LPARAM) item.lParam);
                                    break;
                                case ID_TABMENU_CONTAINEROPTIONS: {
                                    if(pContainer->hWndOptions == 0)
                                        CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM) pContainer);
                                    break;
                                case ID_TABMENU_CLOSECONTAINER:
                                    SendMessage(hwndDlg, WM_CLOSE, 0, 0);
                                    break;
                                case ID_TABMENU_CONFIGURETABAPPEARANCE:
                                    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TABCONFIG), hwndDlg, DlgProcTabConfig, NULL);
                                    break;
                                }
                            }
                            return 1;
                        }
                }
                break;
            }
        /*
         * some kind of "easy drag" feature
         * fake a "caption drag event" if the mouse cursur is within the client
         * area of the container
         */
        case WM_LBUTTONDOWN: {
            pContainer->dwFlags |= CNT_MOUSEDOWN;
            GetCursorPos(&pContainer->ptLast);
            SetCapture(hwndDlg);
            break;
        }
        case WM_LBUTTONUP:
            pContainer->dwFlags &= ~CNT_MOUSEDOWN;
            ReleaseCapture();
            break;
        case WM_MOUSEMOVE: {
                POINT pt;
                RECT  rc;
                if (pContainer->dwFlags & CNT_NOTITLE && pContainer->dwFlags & CNT_MOUSEDOWN) {
                    GetCursorPos(&pt);
                    GetWindowRect(hwndDlg, &rc);
                    MoveWindow(hwndDlg, rc.left - (pContainer->ptLast.x - pt.x), rc.top - (pContainer->ptLast.y - pt.y), rc.right - rc.left, rc.bottom - rc.top, TRUE);
                    pContainer->ptLast = pt;
                }
                break;
            }
       /*
        * pass the WM_ACTIVATE msg to the active message dialog child
        */
        case WM_ACTIVATE:
			if (LOWORD(wParam == WA_INACTIVE) && (HWND)lParam != myGlobals.g_hwndHotkeyHandler) {
				if (pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL)
					pSetLayeredWindowAttributes(hwndDlg, RGB(255,255,255), (BYTE)HIWORD(pContainer->dwTransparency), LWA_ALPHA);
			}
            if (LOWORD(wParam) != WA_ACTIVE)
                break;
        case WM_MOUSEACTIVATE: {
                TCITEM item;

                FlashContainer(pContainer, 0, 0);
                pContainer->dwFlashingStarted = 0;
                pLastActiveContainer = pContainer;
                if(pContainer->dwFlags & CNT_DEFERREDTABSELECT) {
                    NMHDR nmhdr;

                    pContainer->dwFlags &= ~CNT_DEFERREDTABSELECT;
                    SendMessage(hwndDlg, WM_SYSCOMMAND, SC_RESTORE, 0);
                    ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
                    nmhdr.code = TCN_SELCHANGE;
                    SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmhdr);     // do it via a WM_NOTIFY / TCN_SELCHANGE to simulate user-activation
                }
                if(pContainer->dwFlags & CNT_DEFERREDSIZEREQUEST) {
                    pContainer->dwFlags &= ~CNT_DEFERREDSIZEREQUEST;
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                }
                
                if (pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL)
					pSetLayeredWindowAttributes(hwndDlg, RGB(255,255,255), (BYTE)LOWORD(pContainer->dwTransparency), LWA_ALPHA);

                if(pContainer->dwFlags & CNT_NEED_UPDATETITLE) {
                    HANDLE hContact;
                    pContainer->dwFlags &= ~CNT_NEED_UPDATETITLE;
                    SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
                }
                ZeroMemory((void *)&item, sizeof(item));
                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &item);
                if (pContainer->dwFlags & CNT_DEFERREDCONFIGURE) {
                    RECT rc;
                    HANDLE hContact;
                    
                    pContainer->dwFlags &= ~CNT_DEFERREDCONFIGURE;
                    item.mask = TCIF_PARAM;
                    TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &item);
                    SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 10);
                    GetClientRect(hwndDlg, &rc);
                    AdjustTabClientRect(pContainer, &rc);
                    pContainer->hwndActive = (HWND) item.lParam;
                    SetWindowPos(pContainer->hwndActive, HWND_TOP, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), SWP_NOZORDER | SWP_SHOWWINDOW);
                    SetFocus(GetDlgItem(pContainer->hwndActive, IDC_LOG));
                    SetFocus(pContainer->hwndActive);
                    SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                }
                SendMessage((HWND) item.lParam, WM_ACTIVATE, WA_ACTIVE, 0);
                if(GetMenu(hwndDlg) != 0)
                    DrawMenuBar(hwndDlg);
                break;
            }
        case WM_ENTERMENULOOP: {
            POINT pt;
            int pos;
            HMENU hMenu, submenu;
            struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
            
            GetCursorPos(&pt);
            pos = MenuItemFromPoint(hwndDlg, pContainer->hMenu, pt);
            if(pos == 2) {
                HANDLE hContact;

                SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                if(hContact) {
                    hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) hContact, 0);
                    ModifyMenuA(pContainer->hMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR) hMenu, Translate("&User"));
                    DrawMenuBar(hwndDlg);
                    GetCursorPos(&pt);
                    TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);
                    DestroyMenu(hMenu);
                }
            }
            hMenu = pContainer->hMenu;
            CheckMenuItem(hMenu, ID_VIEW_SHOWMENUBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOMENUBAR ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem(hMenu, ID_VIEW_SHOWSTATUSBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOSTATUSBAR ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem(hMenu, ID_VIEW_SHOWAVATAR, MF_BYCOMMAND | dat->showPic ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_VIEW_SHOWTITLEBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOTITLE ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem(hMenu, ID_VIEW_TABSATBOTTOM, MF_BYCOMMAND | pContainer->dwFlags & CNT_TABSBOTTOM ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_VIEW_VERTICALMAXIMIZE, MF_BYCOMMAND | pContainer->dwFlags & CNT_VERTICALMAX ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_TITLEBAR_SHOWNICNAME, MF_BYCOMMAND | pContainer->dwFlags & CNT_TITLE_SHOWNAME ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_TITLEBAR_SHOWSTATUS, MF_BYCOMMAND | pContainer->dwFlags & CNT_TITLE_SHOWSTATUS ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_TITLEBAR_DONOTSHOWCONTAINERNAME, MF_BYCOMMAND | (pContainer->dwFlags & (CNT_TITLE_PREFIX | CNT_TITLE_SUFFIX)) ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem(hMenu, ID_TITLEBAR_SHOWCONTAINERNAME, MF_BYCOMMAND | pContainer->dwFlags & CNT_TITLE_PREFIX ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_TITLEBAR_SHOWCONTAINERNAMEASSUFFIX, MF_BYCOMMAND | pContainer->dwFlags & CNT_TITLE_SUFFIX ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_TITLEBAR_USESTATICCONTAINERICON, MF_BYCOMMAND | pContainer->dwFlags & CNT_STATICICON ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_VIEW_SHOWTOOLBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_HIDETOOLBAR ? MF_UNCHECKED : MF_CHECKED);
            
            EnableMenuItem(hMenu, ID_TITLEBAR_SHOWSTATUS, pContainer->dwFlags & CNT_TITLE_SHOWNAME);
            CheckMenuItem(hMenu, ID_VIEW_SHOWMULTISENDCONTACTLIST, MF_BYCOMMAND | (dat->sendMode & SMODE_MULTIPLE) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_VIEW_STAYONTOP, MF_BYCOMMAND | pContainer->dwFlags & CNT_STICKY ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem(hMenu, ID_EVENTPOPUPS_DISABLEALLEVENTPOPUPS, MF_BYCOMMAND | pContainer->dwFlags & (CNT_DONTREPORT | CNT_DONTREPORTUNFOCUSED | CNT_ALWAYSREPORTINACTIVE) ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISMINIMIZED, MF_BYCOMMAND | pContainer->dwFlags & CNT_DONTREPORT ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSFORALLINACTIVESESSIONS, MF_BYCOMMAND | pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISUNFOCUSED, MF_BYCOMMAND | pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem(hMenu, ID_WINDOWFLASHING_USEDEFAULTVALUES, MF_BYCOMMAND | (pContainer->dwFlags & (CNT_NOFLASH | CNT_FLASHALWAYS)) ? MF_UNCHECKED : MF_CHECKED);
            CheckMenuItem(hMenu, ID_WINDOWFLASHING_DISABLEFLASHING, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOFLASH ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_WINDOWFLASHING_FLASHUNTILFOCUSED, MF_BYCOMMAND | pContainer->dwFlags & CNT_FLASHALWAYS ? MF_CHECKED : MF_UNCHECKED);
            
            submenu = GetSubMenu(hMenu, 3);
            if(dat && submenu) {
                MsgWindowUpdateMenu(pContainer->hwndActive, dat, submenu, MENU_LOGMENU);
                submenu = GetSubMenu(hMenu, 1);
                MsgWindowUpdateMenu(pContainer->hwndActive, dat, submenu, MENU_PICMENU);
            }
            break;
        }
        case DM_CLOSETABATMOUSE:
            {
                HWND hwndCurrent;
                POINT *pt = (POINT *)lParam;
                int iItem;
                TCITEM item = {0};
                
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0)) {
                    if (MessageBoxA(pContainer->hwnd, Translate(szWarnClose), "Miranda", MB_YESNO | MB_ICONQUESTION) == IDNO)
                        break;
                }
                hwndCurrent = pContainer->hwndActive;
                if ((iItem = GetTabItemFromMouse(hwndTab, pt)) == -1)
                    break;
                ZeroMemory((void *)&item, sizeof(item));
                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(hwndTab, iItem, &item);
                if (item.lParam) {
                    if ((HWND) item.lParam != hwndCurrent) {
                        pContainer->bDontSmartClose = TRUE;
                        SendMessage((HWND) item.lParam, WM_CLOSE, 0, 1);
                        RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE);
                        pContainer->bDontSmartClose = FALSE;
                    } else
                        SendMessage((HWND) item.lParam, WM_CLOSE, 0, 1);
                }
                break;
            }
		case DM_OPTIONSAPPLIED: {
			DWORD dwGlobalFlags = 0, dwLocalFlags = 0;
			DWORD dwGlobalTrans = 0, dwLocalTrans = 0;
			char szCname[40];
            int overridePerContact = 1;
#if defined (_UNICODE)
            char *szSetting = "CNTW_";
#else
            char *szSetting = "CNT_";
#endif                
			
            dwGlobalFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containerflags", CNT_FLAGS_DEFAULT);
            
            if(pContainer->isCloned && pContainer->hContactFrom != 0) {
                mir_snprintf(szCname, 40, "%s_Flags", szSetting);
                dwLocalFlags = DBGetContactSettingDword(pContainer->hContactFrom, SRMSGMOD_T, szCname, 0xffffffff);
                mir_snprintf(szCname, 40, "%s_Trans", szSetting);
                dwLocalTrans = DBGetContactSettingDword(pContainer->hContactFrom, SRMSGMOD_T, szCname, 0xffffffff);
                if(dwLocalFlags == 0xffffffff || dwLocalTrans == 0xffffffff)
                    overridePerContact = 1;
                else
                    overridePerContact = 0;
            }
            if(overridePerContact) {
                mir_snprintf(szCname, 40, "%s%d_Flags", szSetting, pContainer->iContainerIndex);
                dwLocalFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, szCname, 0xffffffff);
                mir_snprintf(szCname, 40, "%s%d_Trans", szSetting, pContainer->iContainerIndex);
                dwLocalTrans = DBGetContactSettingDword(NULL, SRMSGMOD_T, szCname, 0xffffffff);
            }

			dwGlobalTrans = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containertrans", CNT_TRANS_DEFAULT);
			
			if (dwLocalFlags == 0xffffffff) {
				pContainer->dwFlags = dwGlobalFlags;
			}
			else {
				pContainer->dwFlags = dwLocalFlags;
			}

			if (dwLocalTrans == 0xffffffff)
				pContainer->dwTransparency = dwGlobalTrans;
			else
				pContainer->dwTransparency = dwLocalTrans;

			if (LOWORD(pContainer->dwTransparency) < 50)
				pContainer->dwTransparency = MAKELONG(50, (WORD)HIWORD(pContainer->dwTransparency));
			if (HIWORD(pContainer->dwTransparency) < 50)
				pContainer->dwTransparency = MAKELONG((WORD)LOWORD(pContainer->dwTransparency), 50);
			
            break;
		}
        case DM_STATUSBARCHANGED:
            {
                RECT rc;
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                GetWindowRect(hwndDlg, &rc);
                SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, 0);
                SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
                RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
                UpdateWindow(hwndDlg);
                PostMessage(pContainer->hwndActive, DM_STATUSBARCHANGED, 0, 0);
                break;
            }
		case DM_CONFIGURECONTAINER: {
			DWORD ws, wsold, ex = 0, exold = 0;
			HMENU hSysmenu = GetSystemMenu(hwndDlg, FALSE);
			HANDLE hContact;
            int i = 0;
            BYTE bFlat = DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0);

            ShowWindow(GetDlgItem(hwndDlg, IDC_STATICCONTROL), SW_HIDE);
            
            if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0))
                pContainer->dwFlags &= ~(CNT_TITLE_PREFIX | CNT_TITLE_SUFFIX);      // hide container name in single window mode (not needed)

            if(!myGlobals.m_SideBarEnabled)
                pContainer->dwFlags &= ~CNT_SIDEBAR;
            
            //ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBAR), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
            for(i = 0; sbarItems[i].uId != 0; i++) {
                if(bFlat)
                    SendMessage(GetDlgItem(hwndDlg, sbarItems[i].uId), BUTTONSETASFLATBTN, 0, 0);
                ShowWindow(GetDlgItem(hwndDlg, sbarItems[i].uId), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
            }
            ws = wsold = GetWindowLong(hwndDlg, GWL_STYLE);
			ws = (pContainer->dwFlags & CNT_NOTITLE) ? ws & ~WS_CAPTION : ws | WS_CAPTION;
			SetWindowLong(hwndDlg, GWL_STYLE, ws);

            pContainer->tBorder = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tborder", 0);
            pContainer->tBorder_outer = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tborder_outer", 2);
            
			if (LOBYTE(LOWORD(GetVersion())) >= 5  && pSetLayeredWindowAttributes != NULL) {
                DWORD exold;
				ex = exold = GetWindowLong(hwndDlg, GWL_EXSTYLE);
				ex = (pContainer->dwFlags & CNT_TRANSPARENCY) ? ex | WS_EX_LAYERED : ex & ~WS_EX_LAYERED;
				SetWindowLong(hwndDlg , GWL_EXSTYLE , ex);
				if (pContainer->dwFlags & CNT_TRANSPARENCY)
    				pSetLayeredWindowAttributes(hwndDlg, RGB(255,255,255), (BYTE)LOWORD(pContainer->dwTransparency), LWA_ALPHA);
				
                if((exold & WS_EX_LAYERED) != (ex & WS_EX_LAYERED))
                    RedrawWindow(hwndDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
			}

			CheckMenuItem(hSysmenu, IDM_NOTITLE, (pContainer->dwFlags & CNT_NOTITLE) ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hSysmenu, IDM_STAYONTOP, pContainer->dwFlags & CNT_STICKY ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
            SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            if (ws != wsold) {
				RECT rc;
				GetWindowRect(hwndDlg, &rc);
                if((ws & WS_CAPTION) != (wsold & WS_CAPTION)) {
                    SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, 0);
                    SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
                    RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
                    UpdateWindow(hwndDlg);
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                }
            }
            ws = wsold = GetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_STYLE);
            if(pContainer->dwFlags & CNT_TABSBOTTOM) {
                ws |= (TCS_BOTTOM);
            }
            else {
                ws &= ~(TCS_BOTTOM);
            }
            if((ws & (TCS_BOTTOM | TCS_MULTILINE)) != (wsold & (TCS_BOTTOM | TCS_MULTILINE))) {
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_STYLE, ws);
                RedrawWindow(GetDlgItem(hwndDlg, IDC_MSGTABS), NULL, NULL, RDW_INVALIDATE);
            }

            if(pContainer->dwFlags & CNT_NOSTATUSBAR) {
                if(pContainer->hwndStatus) {
                    DestroyWindow(pContainer->hwndStatus);
                    pContainer->hwndStatus = 0;
                    pContainer->statusBarHeight = 0;
                    SendMessage(hwndDlg, DM_STATUSBARCHANGED, 0, 0);
                }
            }
            else if(pContainer->hwndStatus == 0) {
                if(!pContainer->hwndStatus)
                    pContainer->hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, SBT_TOOLTIPS | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndDlg, NULL, g_hInst, NULL);

                if(pContainer->hwndStatus) {
                    ws = GetWindowLong(pContainer->hwndStatus, GWL_STYLE);
                    SetWindowLong(pContainer->hwndStatus, GWL_STYLE, ws & ~SBARS_SIZEGRIP);
                    if(lParam != 10)
                        SendMessage(hwndDlg, DM_STATUSBARCHANGED, 0, 0);
                }
            }
            if(pContainer->dwFlags & CNT_NOMENUBAR) {
                if(pContainer->hMenu) {
                    SetMenu(hwndDlg, NULL);
                    //SendMessage(hwndDlg, WM_SIZE, 0, 0);
                }
                ShowWindow(GetDlgItem(hwndDlg, IDC_STATICCONTROL), SW_HIDE);
            }
            else {
                if(pContainer->hMenu == 0) {
                    pContainer->hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));
                    CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) pContainer->hMenu, 0);
                }
                SetMenu(hwndDlg, pContainer->hMenu);
                //SendMessage(hwndDlg, WM_SIZE, 0, 0);
                DrawMenuBar(hwndDlg);
            }

            SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
            SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
            BroadCastContainer(pContainer, DM_CONFIGURETOOLBAR, 0, 1);
			break;
		}
        case DM_SETSIDEBARBUTTONS:
        {
            int i = 0;
            for(i = 0; sbarItems[i].uId != 0; i++) {
                SendDlgItemMessage(hwndDlg, sbarItems[i].uId, BM_SETIMAGE, IMAGE_ICON, (LPARAM)*(sbarItems[i].hIcon));
                SendDlgItemMessage(hwndDlg, sbarItems[i].uId, BUTTONADDTOOLTIP, (WPARAM)sbarItems[i].szTip, 0);
                if(sbarItems[i].dwFlags & SBI_TOGGLE)
                    SendDlgItemMessage(hwndDlg, sbarItems[i].uId, BUTTONSETASPUSHBTN, 0, 0);
            }
            break;
        }
        /*
         * search the first and most recent unread events in all client tabs...
         * return all information via a RECENTINFO structure (tab indices, 
         * window handles and timestamps).
         */
        
        case DM_QUERYRECENT:
        {
            int i;
            int iItems = TabCtrl_GetItemCount(hwndTab);
            RECENTINFO *ri = (RECENTINFO *)lParam;
            TCITEM item = {0};
            
            DWORD dwTimestamp, dwMostRecent = 0;

            ri->iFirstIndex = ri->iMostRecent = -1;
            ri->dwFirst = ri->dwMostRecent = 0;
            ri->hwndFirst = ri->hwndMostRecent = 0;
            
            for (i = 0; i < iItems; i++) {
                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(hwndTab,  i, &item);
                SendMessage((HWND) item.lParam, DM_QUERYLASTUNREAD, 0, (LPARAM)&dwTimestamp);
                if (dwTimestamp > ri->dwMostRecent) {
                    ri->dwMostRecent = dwTimestamp;
                    ri->iMostRecent = i;
                    ri->hwndMostRecent = (HWND) item.lParam;
                    if(ri->iFirstIndex == -1) {
                        ri->iFirstIndex = i;
                        ri->dwFirst = dwTimestamp;
                        ri->hwndFirst = (HWND) item.lParam;
                    }
                }
            }
            break;
        }
        /*
         * search tab with either next or most recent unread message and select it
         */
        case DM_QUERYPENDING: {
                NMHDR nmhdr;
                RECENTINFO ri;
                
                SendMessage(hwndDlg, DM_QUERYRECENT, 0, (LPARAM)&ri);
                nmhdr.code = TCN_SELCHANGE;

                if (wParam == DM_QUERY_NEXT && ri.iFirstIndex != -1) {
                    TabCtrl_SetCurSel(hwndTab,  ri.iFirstIndex);
                    SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmhdr);
                }
                if (wParam == DM_QUERY_MOSTRECENT && ri.iMostRecent != -1) {
                    TabCtrl_SetCurSel(hwndTab, ri.iMostRecent);
                    SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmhdr);
                }
                break;
            }
        /*
         * msg dialog children are required to report their minimum height requirements
         */
        case DM_REPORTMINHEIGHT:
            pContainer->uChildMinHeight = ((UINT) lParam > pContainer->uChildMinHeight) ? (UINT) lParam : pContainer->uChildMinHeight;
            return 0;

        case WM_LBUTTONDBLCLK:
            SendMessage(hwndDlg, WM_SYSCOMMAND, IDM_NOTITLE, 0);
            break;
        case WM_CONTEXTMENU:
            {
                if (pContainer->hwndStatus && pContainer->hwndStatus == (HWND) wParam) {
                    POINT pt, pt1;
                    HANDLE hContact;
                    HMENU hMenu;

                    SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                    if(hContact) {
                        RECT rc;
                        GetCursorPos(&pt);
                        pt1 = pt;
                        ScreenToClient(hwndDlg, &pt1);
                        SendMessage(pContainer->hwndStatus, SB_GETRECT, 0, (LPARAM)&rc);
                        if(pt1.x > rc.left && pt1.x < rc.left + 18) {
                            SendMessage(myGlobals.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 100, WM_RBUTTONUP);
                            break;
                        }
                        hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) hContact, 0);
                        TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);
                        DestroyMenu(hMenu);
                    }
                }
                break;
            }
        case DM_SETICON:
            {
                HICON hIconMsg = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
                if((HICON)lParam == myGlobals.g_buttonBarIcons[5]) {               // always set typing icon, but don't save it...
                    SendMessage(hwndDlg, WM_SETICON, wParam, lParam);
                    break;
                }
                
                if((HICON)lParam != hIconMsg && pContainer->dwFlags & CNT_STATICICON && myGlobals.g_iconContainer != 0)
                    lParam = (LPARAM)myGlobals.g_iconContainer;
                
                if(pContainer->hIcon == STICK_ICON_MSG && (HICON)lParam != hIconMsg && pContainer->dwFlags & CNT_NEED_UPDATETITLE)
                    lParam = (LPARAM)hIconMsg;
                    //break;          // don't overwrite the new message indicator flag
                SendMessage(hwndDlg, WM_SETICON, wParam, lParam);
                pContainer->hIcon = (lParam == (LPARAM)hIconMsg) ? STICK_ICON_MSG : 0;
                break;
            }
        /*
        case WM_CTLCOLORSTATIC:
            if((HWND)lParam == GetDlgItem(hwndDlg, IDC_SIDEBAR))
                return (BOOL)GetSysColorBrush(COLOR_3DFACE); */
        case WM_DRAWITEM:
        {
            int cx = GetSystemMetrics(SM_CXSMICON);
            int cy = GetSystemMetrics(SM_CYSMICON);
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
            if(dis->CtlType == ODT_BUTTON && (dis->CtlID == IDC_SIDEBARUP || dis->CtlID == IDC_SIDEBARDOWN)) {
                HICON hIcon;
                DWORD bStyle = 0;
                int cx = GetSystemMetrics(SM_CXSMICON);
                int cy = GetSystemMetrics(SM_CYSMICON);
                if(dis->itemState & ODS_DISABLED)
                    bStyle = DFCS_FLAT;
                else
                    bStyle = DFCS_MONO;
                DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | (dis->itemState & ODS_SELECTED ? DFCS_PUSHED : 0) | bStyle);
                hIcon = dis->CtlID == IDC_SIDEBARUP ? myGlobals.g_buttonBarIcons[26] : myGlobals.g_buttonBarIcons[16];
                DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                          (dis->rcItem.right + dis->rcItem.left - cx) / 2,
                          (dis->rcItem.bottom + dis->rcItem.top - cy) / 2,
                          cx, cy,
                          DST_ICON | (dis->itemState & ODS_DISABLED ? DSS_DISABLED : DSS_NORMAL));
                return TRUE;
            }
            return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
        }
        case WM_MEASUREITEM:
        {
            return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
        }
        case DM_QUERYCLIENTAREA:
            {
                RECT *rc = (RECT *)lParam;

                GetClientRect(hwndDlg, rc);
                AdjustTabClientRect(pContainer, rc);
                break;
            }
        case WM_DESTROY:
            {
                int i = 0;
                TCITEM item;
                
                DestroyWindow(hwndTab);
                ZeroMemory((void *)&item, sizeof(item));
                for(i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
                    item.mask = TCIF_PARAM;
                    TabCtrl_GetItem(hwndTab, i, &item);
                    if(IsWindow((HWND)item.lParam))
                        DestroyWindow((HWND) item.lParam);
                }

                KillTimer(hwndDlg, TIMERID_HEARTBEAT);
    			pContainer->hwnd = 0;
    			pContainer->hwndActive = 0;
                pContainer->hMenuContext = 0;
                SetMenu(hwndDlg, NULL);
                if(pContainer->hwndStatus)
                    DestroyWindow(pContainer->hwndStatus);
                if(pContainer->hMenu)
                    DestroyMenu(pContainer->hMenu);
                SendMessage(hwndTab, EM_UNSUBCLASSED, 0, 0);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_WNDPROC, (LONG)OldTabControlProc);        // un-subclass
    			DestroyWindow(pContainer->hwndTip);
    			RemoveContainerFromList(pContainer);
#if defined(__MATHMOD_SUPPORT)
                if(myGlobals.m_MathModAvail)
                    CallService(MTH_HIDE, 0, 0);
#endif                
                if (pContainer)
                    free(pContainer);
    			SetWindowLong(hwndDlg, GWL_USERDATA, 0);
                return 0;
            }
        case WM_CLOSE: {
                WINDOWPLACEMENT wp;
                char szCName[40];
 #if defined (_UNICODE)
                char *szSetting = "CNTW_";
#else
                char *szSetting = "CNT_";
#endif                
#if defined(_STREAMTHREADING)
                if(pContainer->pendingStream)
                    return TRUE;
#endif                
                if (lParam == 0 && TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_MSGTABS)) > 0) {    // dont ask if container is empty (no tabs)
                    if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0)) {
                        if (MessageBoxA(hwndDlg, Translate(szWarnClose), "Miranda", MB_YESNO | MB_ICONQUESTION) == IDNO)
                            return TRUE;
                    }
                }

                ZeroMemory((void *)&wp, sizeof(wp));
                wp.length = sizeof(wp);
            /*
            * save geometry information to the database...
            */
                if (GetWindowPlacement(hwndDlg, &wp)) {
                    if(pContainer->isCloned && pContainer->hContactFrom != 0) {
                        HANDLE hContact;
                        int i;
                        TCITEM item = {0};
                        
                        item.mask = TCIF_PARAM;
                        for(i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
                            if(TabCtrl_GetItem(hwndTab, i, &item)) {
                                SendMessage((HWND)item.lParam, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                                DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splitx", wp.rcNormalPosition.left);
                                DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splity", wp.rcNormalPosition.top);
                                DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splitwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
                                DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splitheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
                            }
                        }
                    }
                    else {
                        _snprintf(szCName, 40, "%s%dx", szSetting, pContainer->iContainerIndex);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.left);
                        _snprintf(szCName, 40, "%s%dy", szSetting, pContainer->iContainerIndex);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.top);
                        _snprintf(szCName, 40, "%s%dwidth", szSetting, pContainer->iContainerIndex);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.right - wp.rcNormalPosition.left);
                        _snprintf(szCName, 40, "%s%dheight", szSetting, pContainer->iContainerIndex);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
                    }
                } else
                    MessageBoxA(0, "GetWindowPlacement() failed", "Error", MB_OK);

                // clear temp flags which should NEVER be saved...

                if(pContainer->isCloned && pContainer->hContactFrom != 0) {
                    HANDLE hContact;
                    int i;
                    TCITEM item = {0};
                    
                    item.mask = TCIF_PARAM;
                    pContainer->dwFlags &= ~(CNT_DEFERREDCONFIGURE | CNT_CREATE_MINIMIZED | CNT_DEFERREDSIZEREQUEST | CNT_CREATE_CLONED);
                    for(i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
                        if(TabCtrl_GetItem(hwndTab, i, &item)) {
                            SendMessage((HWND)item.lParam, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                            _snprintf(szCName, 40, "%s_Flags", szSetting);
                            DBWriteContactSettingDword(hContact, SRMSGMOD_T, szCName, pContainer->dwFlags);
                            _snprintf(szCName, 40, "%s_Trans", szSetting);
                            DBWriteContactSettingDword(hContact, SRMSGMOD_T, szCName, pContainer->dwTransparency);
                        }
                    }
                }
                else {
                    pContainer->dwFlags &= ~(CNT_DEFERREDCONFIGURE | CNT_CREATE_MINIMIZED | CNT_DEFERREDSIZEREQUEST | CNT_CREATE_CLONED);
                    _snprintf(szCName, 40, "%s%d_Flags", szSetting, pContainer->iContainerIndex);
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, pContainer->dwFlags);
                    _snprintf(szCName, 40, "%s%d_Trans", szSetting, pContainer->iContainerIndex);
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, pContainer->dwTransparency);
                }
                DestroyWindow(hwndDlg);
                break;
            }
        default:
            return FALSE;
    }
    return FALSE;
}

/*
 * write a log entry to the log file.
 */

int _log(const char *fmt, ...)
{
#ifdef _LOGGING
    va_list va;
    FILE    *f;
    char    debug[1024];
    int     ibsize = 1023;

    va_start(va, fmt);

    f = fopen("srmm_mod.log", "a+");
    if (f) {
        _vsnprintf(debug, ibsize, fmt, va);
        strncat(debug, "\n", ibsize);
        fwrite((const void *) debug, strlen(debug), 1, f);
        fclose(f);
    }
#endif
    return 0;
}

/*
 * search the list of tabs and return the tab (by index) which "belongs" to the given
 * hwnd. The hwnd is the handle of a message dialog childwindow. At creation,
 * the dialog handle is stored in the TCITEM.lParam field, because we need
 * to know the owner of the tab.
 *
 * hwndTab: handle of the tab control itself.
 * hwnd: handle of a message dialog.
 *
 * returns the tab index (zero based), -1 if no tab is found (which SHOULD not
 * really happen, but who knows... ;) )
 */

int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd)
{
    TCITEM item;
    int i = 0;

    if(!IsWindow(hwndTab) || hwndTab == 0)
        MessageBoxA(0, "hwndTab invalid", "Error", MB_OK);
    
    ZeroMemory((void *)&item, sizeof(item));
    item.mask = TCIF_PARAM;

    for (i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
        TabCtrl_GetItem(hwndTab, i, &item);
        if ((HWND) item.lParam == hwnd) {
            return i;
        }
    }
    return -1;
}

/*
 * activates the tab belonging to the given client HWND (handle of the actual
 * message window.
 */

int ActivateTabFromHWND(HWND hwndTab, HWND hwnd)
{
    NMHDR nmhdr;

    int iItem = GetTabIndexFromHWND(hwndTab, hwnd);
    if (iItem >= 0) {
        TabCtrl_SetCurSel(hwndTab, iItem);
        ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
        nmhdr.code = TCN_SELCHANGE;
        SendMessage(GetParent(hwndTab), WM_NOTIFY, 0, (LPARAM) &nmhdr);     // do it via a WM_NOTIFY / TCN_SELCHANGE to simulate user-activation
        return iItem;
    }
    return -1;
}

/*
 * returns the index of the tab under the mouse pointer. Used for
 * context menu popup and tooltips
 * pt: mouse coordinates, obtained from GetCursorPos()
 */

int GetTabItemFromMouse(HWND hwndTab, POINT *pt)
{
    TCHITTESTINFO tch;

    ScreenToClient(hwndTab, pt);
    tch.pt = *pt;
    tch.flags = 0;
    return TabCtrl_HitTest(hwndTab, &tch);
}

/*
 * cut off contact name to the option value set via Options->Tabbed messaging
 * some people were requesting this, because really long contact list names
 * are causing extraordinary wide tabs and these are looking ugly and wasting
 * screen space.
 */

int CutContactName(char *oldname, char *newname, int size)
{
    int cutMax = myGlobals.m_CutContactNameTo;
    if ((int)strlen(oldname) <= cutMax)
        strncpy(newname, oldname, size);
    else {
		char fmt[20];
		mir_snprintf(fmt, 18, "%%%d.%ds...", cutMax, cutMax);
		mir_snprintf(newname, size - 1, fmt, oldname);
    }
    return 0;
}

/*
 * functions for handling the linked list of struct ContainerWindowData *foo
 */

struct ContainerWindowData *AppendToContainerList(struct ContainerWindowData *pContainer) {
    struct ContainerWindowData *pCurrent = 0;

    if (!pFirstContainer) {
        pFirstContainer = pContainer;
        pFirstContainer->pNextContainer = NULL;
        return pFirstContainer;
    } else {
        pCurrent = pFirstContainer;
        while (pCurrent->pNextContainer != 0)
            pCurrent = pCurrent->pNextContainer;
        pCurrent->pNextContainer = pContainer;
        pContainer->pNextContainer = NULL;
        return pCurrent;
    }
}

struct ContainerWindowData *FindContainerByName(const TCHAR *name) {
    struct ContainerWindowData *pCurrent = pFirstContainer;

    if (name == NULL || _tcslen(name) == 0)
        return 0;

    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0)) {             // single window mode - always return 0 and force a new container
        return NULL;
    }
    
    while (pCurrent) {
        if (!_tcsncmp(pCurrent->szName, name, CONTAINER_NAMELEN))
            return pCurrent;
        pCurrent = pCurrent->pNextContainer;
    }
     // error, didn't find it.
    return NULL;
}

struct ContainerWindowData *RemoveContainerFromList(struct ContainerWindowData *pContainer) {
    struct ContainerWindowData *pCurrent = pFirstContainer;

    if (pContainer == pFirstContainer) {
        if(pContainer->pNextContainer != NULL)
            pFirstContainer = pContainer->pNextContainer;
        else
            pFirstContainer = NULL;

        if(pLastActiveContainer == pContainer)      // make sure, we don't reference this container anymore
            pLastActiveContainer = pFirstContainer;

        return pFirstContainer;
    }

    do {
        if (pCurrent->pNextContainer == pContainer) {
            pCurrent->pNextContainer = pCurrent->pNextContainer->pNextContainer;

            if(pLastActiveContainer == pContainer)      // make sure, we don't reference this container anymore
                pLastActiveContainer = pFirstContainer;
            
            return 0;
        }
    } while (pCurrent = pCurrent->pNextContainer);
    return NULL;
}

/*
 * get the image list index for the specified protocol icon...
 * *szProto: protocol name.
 * iStatus: protocol status
 * returns the image list index for the icon, or 0 if not found
 * 0 is actually the global "offline" icon.
 */

/*
int GetProtoIconFromList(const char *szProto, int iStatus)
{
    int i;

    if (szProto == NULL || strlen(szProto) == 0) {
        return 0;
    }
    for (i = 0; i < myGlobals.g_nrProtos; i++) {
        if (!strncmp(protoIconData[i].szName, szProto, sizeof(protoIconData[i].szName))) {
            return protoIconData[i].iFirstIconID + ( iStatus - ID_STATUS_OFFLINE );
        }
    }
    return 0;
}
*/
/*
 * calls the TabCtrl_AdjustRect to calculate the "real" client area of the tab.
 * also checks for the option "hide tabs when only one tab open" and adjusts
 * geometry if necessary
 * rc is the RECT obtained by GetClientRect(hwndTab)
 */

void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc)
{
    HWND hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
    RECT rcTab, rcTabOrig;
    DWORD dwBottom, dwTop;
    DWORD tBorder = pContainer->tBorder;
    DWORD dwStyle = GetWindowLong(hwndTab, GWL_STYLE);
    
    GetClientRect(hwndTab, &rcTab);
    dwBottom = rcTab.bottom;
    dwTop = rcTab.top;
    if (pContainer->iChilds > 1 || !(pContainer->dwFlags & CNT_HIDETABS)) {
        DWORD dwTopPad;
        rcTabOrig = rcTab;
        TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);
        dwTopPad = rcTab.top - rcTabOrig.top;
        
        rc->left += tBorder;
        rc->right -= tBorder;

        if(dwStyle & TCS_BUTTONS) {
            if(pContainer->dwFlags & CNT_TABSBOTTOM) {
                int rows = TabCtrl_GetRowCount(hwndTab);
                RECT rcItem;
                TabCtrl_GetItemRect(hwndTab, 0, &rcItem);
                rc->top = 0;
                rc->bottom -= (rows * (rcItem.bottom - rcItem.top) + pContainer->statusBarHeight);
            }
            else {
                rc->top += (dwTopPad - 2);;
                rc->bottom = rcTabOrig.bottom;
            }
        }
        else {
            if(pContainer->dwFlags & CNT_TABSBOTTOM)
                rc->bottom = rcTab.bottom + 2;
            else {
                rc->top += (dwTopPad - 2);;
                rc->bottom = rcTabOrig.bottom;
            }
        }
        
        rc->top += tBorder;
        rc->bottom -= tBorder;
    } else {
        rc->bottom -= pContainer->statusBarHeight;
        //rc->bottom -= (pContainer->dwFlags & CNT_NOMENUBAR ? 0 : 3);
        rc->bottom -= (2 * pContainer->tBorder_outer);
    }
    rc->right -= (2 * pContainer->tBorder_outer);
    if(pContainer->dwFlags & CNT_SIDEBAR)
      rc->right -= SIDEBARWIDTH;
}

/*
 * retrieve the container name for the given contact handle. 
 * if none is assigned, return the name of the default container
 */

int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen)
{
    DBVARIANT dbv;
    
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0)) {            // single window mode using cloned (temporary) containers
        _tcsncpy(szName, _T("Message Session"), iNameLen);
        return 0;
    }
    
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0)) {        // use clist group names for containers...
        if(DBGetContactSetting(hContact, "CList", "Group", &dbv)) {
            _tcsncpy(szName, _T("default"), iNameLen);
            return 0;
        }
        else {
            if(strlen(dbv.pszVal) > CONTAINER_NAMELEN)
                dbv.pszVal[CONTAINER_NAMELEN] = '\0';
#if defined (_UNICODE)
            MultiByteToWideChar(CP_ACP, 0, dbv.pszVal, -1, szName, iNameLen);
            szName[iNameLen] = '\0';
#else
            strncpy(szName, dbv.pszVal, iNameLen);
            szName[iNameLen] = '\0';
#endif
            DBFreeVariant(&dbv);
            return dbv.cchVal;
        }
    }
#if defined (_UNICODE)
    if (DBGetContactSetting(hContact, SRMSGMOD_T, "containerW", &dbv)) {
#else        
    if (DBGetContactSetting(hContact, SRMSGMOD_T, "container", &dbv)) {
#endif        
        _tcsncpy(szName, _T("default"), iNameLen);
        return 0;
    }
#if defined (_UNICODE)
    if (dbv.type == DBVT_ASCIIZ) {
        WCHAR *wszString = Utf8_Decode(dbv.pszVal);
        _tcsncpy(szName, wszString, iNameLen);
        DBFreeVariant(&dbv);
        free(wszString);
        return dbv.cpbVal;
    }
#else
    if (dbv.type == DBVT_ASCIIZ) {
        strncpy(szName, dbv.pszVal, iNameLen);
        DBFreeVariant(&dbv);
        return dbv.cchVal;
    }
#endif    
    return 0;
}

/*
 * this function iterates over all containers defined under TAB_Containers in the DB. It allows
 * to delete them (marking them as **free** actually) and will care about all references to a
 * deleted or renamed container.
 * It also allows for setting the Flags value for all containers
 */

int EnumContainers(HANDLE hContact, DWORD dwAction, const TCHAR *szTarget, const TCHAR *szNew, DWORD dwExtinfo, DWORD dwExtinfoEx)
{
    DBVARIANT dbv;
    int iCounter = 0;
    char szCounter[20];
#if defined (_UNICODE)
    char *szKey = "TAB_ContainersW";
    char *szSettingP = "CNTW_";
#else    
    char *szSettingP = "CNT_";
    char *szKey = "TAB_Containers";
#endif    
    HANDLE hhContact = 0;

    do {
        _snprintf(szCounter, 8, "%d", iCounter);
        if (DBGetContactSetting(NULL, szKey, szCounter, &dbv))
            break;      // end of loop...
        else {        // found...
            iCounter++;
            if (dbv.type == DBVT_ASCIIZ) {
#if defined(_UNICODE)
                WCHAR wszTemp[CONTAINER_NAMELEN + 10];
                WCHAR *wszTemp1 = Utf8_Decode(dbv.pszVal);
                wcsncpy(wszTemp, wszTemp1, CONTAINER_NAMELEN);
                free(wszTemp1);
#else                    
                char *wszTemp = dbv.pszVal;
#endif                    
                if(!_tcsncmp(wszTemp, _T("**free**"), _tcslen(wszTemp))) {
                    DBFreeVariant(&dbv);
                    continue;
                }
                if (dwAction & CNT_ENUM_DELETE) {
                    if(!_tcsncmp(wszTemp, szTarget, _tcslen(wszTemp)) && _tcslen(wszTemp) == _tcslen(szTarget)) {
                        DeleteContainer(iCounter - 1);
                        DBFreeVariant(&dbv);
                        break;
                    }
                }
                if (dwAction & CNT_ENUM_RENAME) {
                    if(!_tcsncmp(wszTemp, szTarget, _tcslen(wszTemp)) && _tcslen(wszTemp) == _tcslen(szTarget)) {
                        RenameContainer(iCounter - 1, szNew);
                        DBFreeVariant(&dbv);
                        break;
                    }
                }
                if (dwAction & CNT_ENUM_WRITEFLAGS) {
                    char szSetting[CONTAINER_NAMELEN + 15];
                    _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%d_Flags", szSettingP, iCounter - 1);
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, szSetting, dwExtinfo);
                    _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%d_Trans", szSettingP, iCounter - 1);
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, szSetting, dwExtinfoEx);
                }
            }
            DBFreeVariant(&dbv);
        }
    } while ( TRUE );
    
    if(dwAction & CNT_ENUM_WRITEFLAGS) {        // write the flags to the *local* container settings for each hContact
        HANDLE hContact;
#if defined(_UNICODE)
        char *szFlags = "CNTW__Flags";
        char *szTrans = "CNTW__Trans";
#else
        char *szFlags = "CNT__Flags";
        char *szTrans = "CNT__Trans";
#endif        
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while (hContact) {
            if(DBGetContactSettingDword(hContact, SRMSGMOD_T, szFlags, 0xffffffff) != 0xffffffff)
                DBWriteContactSettingDword(hContact, SRMSGMOD_T, szFlags, dwExtinfo);
            if(DBGetContactSettingDword(hContact, SRMSGMOD_T, szTrans, 0xffffffff) != 0xffffffff)
                DBWriteContactSettingDword(hContact, SRMSGMOD_T, szTrans, dwExtinfoEx);

            hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
    }
    
    return 0;
}

void DeleteContainer(int iIndex)
{
    DBVARIANT dbv;
    char szIndex[10], szSetting[CONTAINER_NAMELEN + 30];
#if defined (_UNICODE)
    char *szKey = "TAB_ContainersW";
    char *szSettingP = "CNTW_";
#else    
    char *szSettingP = "CNT_";
    char *szKey = "TAB_ContainersW";
#endif    
    HANDLE hhContact;
    _snprintf(szIndex, 8, "%d", iIndex);
    
    
    if(!DBGetContactSetting(NULL, szKey, szIndex, &dbv)) {
        if(dbv.type == DBVT_ASCIIZ) {
#if defined (_UNICODE)
            WCHAR wszContainerName[CONTAINER_NAMELEN + 2];
            WCHAR *wszTemp = Utf8_Decode(dbv.pszVal);
            _tcsncpy(wszContainerName, wszTemp, CONTAINER_NAMELEN + 1);
            free(wszTemp);
            _DBWriteContactSettingWString(NULL, szKey, szIndex, _T("**free**"));
#else
            DBWriteContactSettingString(NULL, szKey, szIndex, "**free**");
#endif            
            hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
            while (hhContact) {
                DBVARIANT dbv_c;
#if defined (_UNICODE)
                if (!DBGetContactSetting(hhContact, SRMSGMOD_T, "containerW", &dbv_c)) {
                    WCHAR *wszString = Utf8_Decode(dbv_c.pszVal);
                    if(_tcscmp(wszString, wszContainerName) && lstrlenW(wszString) == lstrlenW(wszContainerName)) {
                        DBDeleteContactSetting(hhContact, SRMSGMOD_T, "containerW");
#else                    
                if (!DBGetContactSetting(hhContact, SRMSGMOD_T, "container", &dbv_c)) {
                    if(!strcmp(dbv_c.pszVal, dbv.pszVal) && strlen(dbv_c.pszVal) == strlen(dbv.pszVal)) {
                        DBDeleteContactSetting(hhContact, SRMSGMOD_T, "container");
#endif                    
                    }
                    DBFreeVariant(&dbv_c);
#if defined(_UNICODE)
                    free(wszString);
#endif                    
                }
                hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hhContact, 0);
            }
            _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%d_Flags", szSettingP, iIndex);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
            _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%d_Trans", szSettingP, iIndex);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
            _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dwidth", szSettingP, iIndex);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
            _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dheight", szSettingP, iIndex);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
            _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dx", szSettingP, iIndex);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
            _snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dy", szSettingP, iIndex);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
        }
        DBFreeVariant(&dbv);
    }
}

void RenameContainer(int iIndex, const TCHAR *szNew)
{
    DBVARIANT dbv;
#if defined (_UNICODE)
    char *szKey = "TAB_ContainersW";
    char *szSettingP = "CNTW_";
#else    
    char *szSettingP = "CNT_";
    char *szKey = "TAB_ContainersW";
#endif    
    char szIndex[10];
    HANDLE hhContact;
    // DWORD dwFlags, dwTrans, dwWidth, dwHeight, dwX, dwY;

    _snprintf(szIndex, 8, "%d", iIndex);
     if(!DBGetContactSetting(NULL, szKey, szIndex, &dbv)) {
#if defined(_UNICODE)
        WCHAR wszContainerName[CONTAINER_NAMELEN + 2];
        WCHAR *wszString = Utf8_Decode(dbv.pszVal);
        _tcsncpy(wszContainerName, wszString, CONTAINER_NAMELEN + 1);
        free(wszString);
#endif        
        if (szNew != NULL) {
            if (_tcslen(szNew) != 0)
#if defined(_UNICODE)
                _DBWriteContactSettingWString(NULL, szKey, szIndex, (wchar_t *)szNew);
#else            
                DBWriteContactSettingString(NULL, szKey, szIndex, szNew);
#endif                
        }
        hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while (hhContact) {
            DBVARIANT dbv_c;
#if defined(_UNICODE)
            if (!DBGetContactSetting(hhContact, SRMSGMOD_T, "containerW", &dbv_c)) {
                WCHAR *wszTemp = Utf8_Decode(dbv_c.pszVal);
                if(!_tcscmp(wszContainerName, wszTemp) && lstrlenW(wszContainerName) == lstrlenW(wszTemp)) {
                    if(szNew != NULL) {
                        if(lstrlenW(szNew) != 0)
                            _DBWriteContactSettingWString(hhContact, SRMSGMOD_T, "containerW", (wchar_t *)szNew);
#else                
            if (!DBGetContactSetting(hhContact, SRMSGMOD_T, "container", &dbv_c)) {
                if(!strcmp(dbv.pszVal, dbv_c.pszVal) && strlen(dbv_c.pszVal) == strlen(dbv.pszVal)) {
                    if (szNew != NULL) {
                        if (strlen(szNew) != 0)
                            DBWriteContactSettingString(hhContact, SRMSGMOD_T, "container", szNew);
#endif                        
                    }
                }
                DBFreeVariant(&dbv_c);
#if defined(_UNICODE)
                free(wszTemp);
#endif                
            }
            hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hhContact, 0);
        }
        DBFreeVariant(&dbv);
    }
}

#if defined(_UNICODE)
void _DBWriteContactSettingWString(HANDLE hContact, char *szKey, char *szSetting, const wchar_t *value)
{
    char *utf8string = Utf8_Encode(value);
    DBWriteContactSettingString(hContact, szKey, szSetting, utf8string);
    free(utf8string);
}
#endif

HMENU BuildContainerMenu()
{
#if defined (_UNICODE)
    char *szKey = "TAB_ContainersW";
#else    
    char *szKey = "TAB_Containers";
#endif    
    char szCounter[10];
    int i = 0;
    DBVARIANT dbv = { 0 };
    HMENU hMenu;
    MENUITEMINFO mii = {0};

    if(myGlobals.g_hMenuContainer != 0) {
        HMENU submenu = GetSubMenu(myGlobals.g_hMenuContext, 0);
        RemoveMenu(submenu, 5, MF_BYPOSITION);
        DestroyMenu(myGlobals.g_hMenuContainer);
        myGlobals.g_hMenuContainer = 0;
    }

    // no container attach menu, if we are using the "clist group mode"
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0) || DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0))
        return NULL;

    hMenu = CreateMenu();
    do {
        _snprintf(szCounter, 8, "%d", i);
        if(DBGetContactSetting(NULL, szKey, szCounter, &dbv))
            break;

#if defined (_UNICODE)
        if (dbv.type == DBVT_ASCIIZ) {
            WCHAR *wszString = Utf8_Decode(dbv.pszVal);
            if (_tcsncmp(wszString, _T("**free**"), CONTAINER_NAMELEN)) {
                AppendMenu(hMenu, MF_STRING, IDM_CONTAINERMENU + i, wszString);
            }
            free(wszString);
        }
#else
        if (dbv.type == DBVT_ASCIIZ) {
            if (strncmp(dbv.pszVal, "**free**", CONTAINER_NAMELEN)) {
                AppendMenu(hMenu, MF_STRING, IDM_CONTAINERMENU + i, dbv.pszVal);
            }
        }
#endif                
        DBFreeVariant(&dbv);
        i++;
    } while ( TRUE );

    InsertMenuA(myGlobals.g_hMenuContext, ID_TABMENU_ATTACHTOCONTAINER, MF_BYCOMMAND | MF_POPUP, (UINT_PTR) hMenu, Translate("Attach to"));
    myGlobals.g_hMenuContainer = hMenu;
    return hMenu;
}

HMENU BuildMCProtocolMenu(HWND hwndDlg)
{
    HMENU hMCContextMenu = 0, hMCSubForce = 0, hMCSubDefault = 0, hMenu = 0;
    DBVARIANT dbv;
    int iNumProtos = 0, i = 0, iDefaultProtoByNum = 0;
    char szTemp[50], *szProtoMostOnline = NULL, szMenuLine[128], *nick = NULL, *szStatusText = NULL;
    HANDLE hContactMostOnline, handle;
    DWORD iChecked, isForced;
    WORD wStatus;
    
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
    if(dat == NULL)
        return (HMENU) 0;
    
    if(!IsMetaContact(hwndDlg, dat))
        return (HMENU) 0;

    hMenu = CreatePopupMenu();
    hMCContextMenu = GetSubMenu(hMenu, 0);
    hMCSubForce = CreatePopupMenu();
    hMCSubDefault = CreatePopupMenu();

    AppendMenuA(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED | MF_CHECKED, 1, Translate("Meta Contact"));
    AppendMenuA(hMenu, MF_SEPARATOR, 1, "");

    iNumProtos = (int)CallService(MS_MC_GETNUMCONTACTS, (WPARAM)dat->hContact, 0);
    iDefaultProtoByNum = (int)CallService(MS_MC_GETDEFAULTCONTACTNUM, (WPARAM)dat->hContact, 0);
    hContactMostOnline = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)dat->hContact, 0);
    szProtoMostOnline = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContactMostOnline, 0);
    isForced = DBGetContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1);
    
    for(i = 0; i < iNumProtos; i++) {
        mir_snprintf(szTemp, sizeof(szTemp), "Protocol%d", i);
        if(DBGetContactSetting(dat->hContact, "MetaContacts", szTemp, &dbv))
            continue;
        mir_snprintf(szTemp, sizeof(szTemp), "Handle%d", i);
        if((handle = (HANDLE)DBGetContactSettingDword(dat->hContact, "MetaContacts", szTemp, 0)) != 0) {
            nick = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)handle, 0);
            mir_snprintf(szTemp, sizeof(szTemp), "Status%d", i);
            wStatus = (WORD)DBGetContactSettingWord(dat->hContact, "MetaContacts", szTemp, 0);
            szStatusText = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, wStatus, 0);
        }
        mir_snprintf(szMenuLine, sizeof(szMenuLine), "%s: %s [%s] %s", dbv.pszVal, nick, szStatusText, i == isForced ? Translate("(Forced)") : "");
        iChecked = MF_UNCHECKED;
        if(hContactMostOnline != 0 && hContactMostOnline == handle)
            iChecked = MF_CHECKED;
        AppendMenuA(hMCSubForce, MF_STRING | iChecked, 100 + i, szMenuLine);
        AppendMenuA(hMCSubDefault, MF_STRING | (i == iDefaultProtoByNum ? MF_CHECKED : MF_UNCHECKED), 1000 + i, szMenuLine);
        DBFreeVariant(&dbv);
    }
    AppendMenuA(hMCSubForce, MF_SEPARATOR, 900, "");
    AppendMenuA(hMCSubForce, MF_STRING | ((isForced == -1) ? MF_CHECKED : MF_UNCHECKED), 999, Translate("Autoselect"));
    InsertMenuA(hMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR) hMCSubForce, Translate("Use Protocol"));
    InsertMenuA(hMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR) hMCSubDefault, Translate("Set Default Protocol"));
    
    return hMenu;
}

/*
 * flashes the container
 * iMode != 0: turn on flashing
 * iMode == 0: turn off flashing
 */
 
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount)
{
    FLASHWINFO fwi;
    
    if(pContainer->bInTray && iMode != 0 && nen_options.iAutoRestore > 0) {
        BOOL old = nen_options.bMinimizeToTray;
        nen_options.bMinimizeToTray = FALSE;
        ShowWindow(pContainer->hwnd, SW_HIDE);
        SetParent(pContainer->hwnd, NULL);
        SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        nen_options.bMinimizeToTray = old;
        SetWindowLong(pContainer->hwnd, GWL_STYLE, GetWindowLong(pContainer->hwnd, GWL_STYLE) | WS_VISIBLE);
        DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)pContainer->iContainerIndex + 1, MF_BYCOMMAND);
        pContainer->bInTray = 0;
    }
    
    if(pContainer->dwFlags & CNT_NOFLASH)                   // container should never flash
        return;

    fwi.cbSize = sizeof(fwi);
    fwi.uCount = 0;

    if(iMode) {
        fwi.dwFlags = FLASHW_ALL;
        if(pContainer->dwFlags & CNT_FLASHALWAYS)
            fwi.dwFlags |= FLASHW_TIMER;
        else
            fwi.uCount = (iCount == 0) ? DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4) : iCount;
        fwi.dwTimeout = DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000);
        
    }
    else
        fwi.dwFlags = FLASHW_STOP;

    fwi.hwnd = pContainer->hwnd;
    pContainer->dwFlashingStarted = GetTickCount();
    FlashWindowEx(&fwi);
}

void ReflashContainer(struct ContainerWindowData *pContainer)
{
    DWORD dwStartTime = pContainer->dwFlashingStarted;
    
    if(GetForegroundWindow() == pContainer->hwnd || GetActiveWindow() == pContainer->hwnd)        // dont care about active windows
        return;

    if(pContainer->dwFlags & CNT_NOFLASH || pContainer->dwFlashingStarted == 0)
        return;                                                                                 // dont care about containers which should never flash
    
    if(pContainer->dwFlags & CNT_FLASHALWAYS)
        FlashContainer(pContainer, 1, 0);
    else {
        // recalc the remaining flashes
        DWORD dwInterval = DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000);
        int iFlashesElapsed = (GetTickCount() - dwStartTime) / dwInterval;
        DWORD dwFlashesDesired = DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4);
        if(iFlashesElapsed < (int)dwFlashesDesired)
            FlashContainer(pContainer, 1, dwFlashesDesired - iFlashesElapsed);
        else {
            BOOL isFlashed = FlashWindow(pContainer->hwnd, TRUE);
            if(!isFlashed)
                FlashWindow(pContainer->hwnd, TRUE);
        }
    }
    pContainer->dwFlashingStarted = dwStartTime;
}

/*
 * broadcasts a message to all child windows (tabs/sessions)
 */

void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;
    TCITEM item;
    
    HWND hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
    ZeroMemory((void *)&item, sizeof(item));

    item.mask = TCIF_PARAM;
    
    for(i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
        TabCtrl_GetItem(hwndTab, i, &item);
        if(IsWindow((HWND)item.lParam))
            SendMessage((HWND)item.lParam, message, wParam, lParam);
    }
}

void UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat)
{
    return;
    if(dat->pContainer->hMenu == 0)
        return;
    
    if(dat->hwndLog != 0 && (DBGetContactSettingDword(NULL, "IEVIEW", "TemplatesFlags", 0) & 0x01))
        EnableMenuItem(dat->pContainer->hMenu, 3, MF_BYPOSITION | MF_GRAYED);
    else
        EnableMenuItem(dat->pContainer->hMenu, 3, MF_BYPOSITION | MF_ENABLED);
}

void SetFloater(struct ContainerWindowData *pContainer)
{
    RECT rc;
    POINT pt, pt1;
    int x;
    GetClientRect(pContainer->hwnd, &rc);
    pt.x = rc.left;
    pt.y = rc.top;
    pt1.x = rc.right;
    pt1.y = rc.bottom;
    ClientToScreen(pContainer->hwnd, &pt);
    ClientToScreen(pContainer->hwnd, &pt1);
    x = pt1.x - (pContainer->dwFlags & CNT_NOMENUBAR ? 110 : 52);
    floaterOwner = pContainer->hwnd;
    SetWindowPos(myGlobals.g_hwndHotkeyHandler, HWND_TOP, x, pt.y - (pContainer->dwFlags & CNT_NOMENUBAR ? 20 : 18), 50, 17, SWP_NOACTIVATE);
}
