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
#include "m_message.h"

#define SB_CHAR_WIDTH        45

char *szWarnClose = "Do you really want to close this session?";

extern HCURSOR hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
extern HANDLE hMessageWindowList, hMessageSendList;
extern struct CREOleCallback reOleCallback;
extern HINSTANCE g_hInst;

extern  HIMAGELIST g_hImageList;
extern struct ProtocolData *protoIconData;
extern int g_nrProtos;

HMENU g_hMenuContext, g_hMenuContainer = 0, g_hMenuEncoding = 0;

#define DEFAULT_CONTAINER_POS 0x00400040
#define DEFAULT_CONTAINER_SIZE 0x019001f4

struct ContainerWindowData *pFirstContainer = 0;        // the linked list of struct ContainerWindowData

int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd);
int GetTabItemFromMouse(HWND hwndTab, POINT *pt);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
int GetProtoIconFromList(const char *szProto, int iStatus);
void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc);
HMENU BuildContainerMenu();

struct ContainerWindowData *AppendToContainerList(struct ContainerWindowData *pContainer);
struct ContainerWindowData *RemoveContainerFromList(struct ContainerWindowData *pContainer);
void DeleteContainer(int iIndex), RenameContainer(int iIndex, const TCHAR *newName);
void _DBWriteContactSettingWString(HANDLE hContact, char *szKey, char *szSetting, const wchar_t *value);
int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection);

int _log(const char *fmt, ...);

extern BOOL CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static WNDPROC OldTabControlProc;

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
extern TCHAR g_szDefaultContainerName[];

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
                    if (dbv.type == DBVT_BLOB) {
                        if(!_tcsncmp((TCHAR *)dbv.pbVal, name, CONTAINER_NAMELEN)) {
                            pContainer->iContainerIndex = i;
                            iFound = TRUE;
                        }
                        else if (!_tcsncmp((TCHAR *)dbv.pbVal, _T("**free**"), CONTAINER_NAMELEN)) {
                            iFirstFree =  i;
                        }
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
 * need to subclass the tabcontrol to catch the middle button click
 */

BOOL CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_MBUTTONDOWN: {
            POINT pt;
            GetCursorPos(&pt);
            SendMessage(GetParent(hwnd), DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
            return 1;
        }
    }
    return CallWindowProc(OldTabControlProc, hwnd, msg, wParam, lParam); 
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

    pContainer = (struct ContainerWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    hwndTab = GetDlgItem(hwndDlg, IDC_MSGTABS);

    switch (msg) {
        case WM_INITDIALOG:
            {
                DWORD ws;
                HMENU hSysmenu;
                DWORD dwCreateFlags;
                int iMenuItems;
                
                pContainer = (struct ContainerWindowData *) lParam;
                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pContainer);
                pContainer->iLastClick = 0xffffffff;
                dwCreateFlags = pContainer->dwFlags;

                pContainer->isCloned = (pContainer->dwFlags & CNT_CREATE_CLONED);
                
                SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);          // set options...
                pContainer->dwFlags |= dwCreateFlags;

             
                pContainer->hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));
                //pContainer->hToolbar = CreateToolbarEx(hwndDlg, WS_CHILD | TBSTYLE_LIST | 
                SetMenu(hwndDlg, pContainer->hMenu);
				/*
				 * additional system menu items...
				 */

				hSysmenu = GetSystemMenu(hwndDlg, FALSE);
                iMenuItems = GetMenuItemCount(hSysmenu);

                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_SEPARATOR, 0, "");
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_STAYONTOP, Translate("Stay on Top"));
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_NOTITLE, Translate("Hide titlebar"));
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_NOREPORTMIN, Translate("Don't report if minimized"));
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_SEPARATOR, 0, "");
                InsertMenuA(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_MOREOPTIONS, Translate("Container options..."));                 
                
                if(!(dwCreateFlags & CNT_CREATE_MINIMIZED))
                    SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 0);

                /*
                 * make the tab control the controlling parent window for all message dialogs
                 */

                ws = GetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_EXSTYLE);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_EXSTYLE, ws | WS_EX_CONTROLPARENT);

                /*
                 * subclass the tab control
                 */

                OldTabControlProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_WNDPROC, (LONG)TabControlSubclassProc);
                
                /*
                 * assign the global image list...
                 */
                if(g_hImageList) 
                    TabCtrl_SetImageList(GetDlgItem(hwndDlg, IDC_MSGTABS), g_hImageList);

                TabCtrl_SetPadding(GetDlgItem(hwndDlg, IDC_MSGTABS), 5, DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3));
                /*
                 * context menu
                 */
                pContainer->hMenuContext = g_hMenuContext;
                /*
                 * tab tooltips...
                 */
                pContainer->hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT,
                                                     CW_USEDEFAULT, CW_USEDEFAULT, hwndDlg, NULL, g_hInst, (LPVOID) NULL);

                if (pContainer->hwndTip) {
                    SetWindowPos(pContainer->hwndTip, HWND_TOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    TabCtrl_SetToolTips(GetDlgItem(hwndDlg, IDC_MSGTABS), pContainer->hwndTip);
                } else
                    MessageBoxA(0, "Tooltip problem", "tabSRMM", MB_OK);

                
                if(pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
//                    SetWindowPos(dat->pContainer->hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE |SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOREPOSITION);
                    //ShowWindow(hwndDlg, SW_MINIMIZE);
                }
                else {
                    SendMessage(hwndDlg, DM_RESTOREWINDOWPOS, 0, 0);
                    ShowWindow(hwndDlg, SW_SHOWNORMAL);
                    SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                }
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
                            SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER);
                    }
                }
                else {
                    _snprintf(szCName, 40, "%s%d", szSetting, pContainer->iContainerIndex);
                    if (Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, szCName)) {
                        if (Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, szCName))
                            SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER);
                    }
                }
                break;
            }
        case WM_COMMAND: {
            HANDLE hContact;
            int isHandled;
            struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);

            if(dat) {
                if(MsgWindowMenuHandler(pContainer->hwndActive, dat, LOWORD(wParam)) == 1)
                    break;
                else
                    _DebugPopup(NULL, "not handled");
            }
            SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
            if(hContact) {
                if(CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) hContact))
                    break;
            }
            if(LOWORD(wParam) == IDOK)
                SendMessage(pContainer->hwndActive, WM_COMMAND, wParam, lParam);      // pass the IDOK command to the active child - fixes the "enter not working
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
                pContainer->dwFlags |= CNT_SIZINGLOOP;
                break;
            }
        case WM_EXITSIZEMOVE:
            {
                RECT rc;

                GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
                if (!((rc.right - rc.left) == pContainer->oldSize.cx && (rc.bottom - rc.top) == pContainer->oldSize.cy))
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                pContainer->dwFlags &= ~CNT_SIZINGLOOP;
                break;
            }
        case WM_GETMINMAXINFO: {
                RECT rc;
                POINT pt;

                MINMAXINFO *mmi = (MINMAXINFO *) lParam;
                mmi->ptMinTrackSize.y = pContainer->uChildMinHeight;
                mmi->ptMinTrackSize.x = 280;
                GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
                pt.y = rc.top;
                TabCtrl_AdjustRect(GetDlgItem(hwndDlg, IDC_MSGTABS), FALSE, &rc);
                mmi->ptMinTrackSize.y += ((rc.top - pt.y) + 10);
                return 0;
            }

        case WM_SIZE: { 
                RECT rc, rcClient;
                int i = 0;
                TCITEM item = {0};

                if(IsIconic(hwndDlg)) {
                    pContainer->dwFlags |= CNT_DEFERREDSIZEREQUEST;
                    break;
                }
                if(wParam == SIZE_MINIMIZED)
                    break;

                if (pContainer->hwndStatus) {
                    RECT rcs;
                    int iParts = SendMessage(pContainer->hwndStatus, SB_GETPARTS, 0, 0);
                    SendMessage(pContainer->hwndStatus, WM_SIZE, 0, 0);
                    GetWindowRect(pContainer->hwndStatus, &rcs);
                    if (iParts == 4) {
                        int statwidths[4];

                        statwidths[0] = (rcs.right - rcs.left) - (2 * SB_CHAR_WIDTH) - 50;
                        statwidths[1] = rcs.right - rcs.left - SB_CHAR_WIDTH - 35;
                        statwidths[2] = rcs.right - rcs.left - 35;
                        statwidths[3] = -1;
                        SendMessage(pContainer->hwndStatus, SB_SETPARTS, 4, (LPARAM) statwidths);
                    } else if(iParts == 3) {
                        int statwidths[3];
                        
                        statwidths[0] = rcs.right - rcs.left - SB_CHAR_WIDTH - 50;
                        statwidths[1] = rcs.right - rcs.left - 35;
                        statwidths[2] = -1;
                        SendMessage(pContainer->hwndStatus, SB_SETPARTS, 3, (LPARAM) statwidths);
                    }
                    pContainer->statusBarHeight = (rcs.bottom - rcs.top) + 1;
                }
                else
                    pContainer->statusBarHeight = 0;
                
                GetClientRect(hwndDlg, &rcClient);
                if (lParam) {
//                    MoveWindow(hwndTab, 2, 5, (rcClient.right - rcClient.left) - 4, (rcClient.bottom - rcClient.top) - 4, TRUE);
                    MoveWindow(hwndTab, 2, 5, (rcClient.right - rcClient.left) - 4, (rcClient.bottom - rcClient.top) - pContainer->statusBarHeight - 5, TRUE);
                }
                AdjustTabClientRect(pContainer, &rcClient);
                rc = rcClient;
                /*
                 * we care about all client sessions, but we really resize only the active tab (hwndActive)
                 * we tell inactive tabs to resize theirselves later when they get activated (DM_CHECKSIZE
                 * just queues a resize request)
                 */
                for(i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
                    item.mask = TCIF_PARAM;
                    TabCtrl_GetItem(hwndTab, i, &item);
                    if((HWND)item.lParam == pContainer->hwndActive) {
                        MoveWindow((HWND)item.lParam, rcClient.left +1, rcClient.top, (rcClient.right - rcClient.left) - 8, (rcClient.bottom - rcClient.top) -2, TRUE);
                        if(!(pContainer->dwFlags & CNT_SIZINGLOOP)) {
                            SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                        }
                    }
                    else
                        SendMessage((HWND)item.lParam, DM_CHECKSIZE, 0, 0);
                }
                if(wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
                    SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                }
                break; 
            } 
        case DM_UPDATETITLE: {
                HANDLE hContact = (HANDLE) wParam;
                char *szProto = 0, *szStatus = 0, *contactName = 0, temp[200];
                TCHAR newtitle[256], oldtitle[256];
                WORD wStatus = -1;
                TCHAR tTemp[204];
                BYTE showStatus = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON);

                if(pContainer->dwFlags & CNT_TITLE_SHOWNAME) {
                    contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0);
                }
                
                temp[0] = '\0';
                tTemp[0] = '\0';
                
                if((pContainer->dwFlags & CNT_TITLE_SHOWNAME && pContainer->dwFlags & CNT_TITLE_SHOWSTATUS) || showStatus) {
                    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
                    if (szProto) {
                        wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
                        szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, szProto == NULL ? ID_STATUS_OFFLINE : wStatus, 0);
                    }
                    if(szStatus != NULL && pContainer->dwFlags & CNT_TITLE_SHOWSTATUS && pContainer->dwFlags & CNT_TITLE_SHOWNAME)
                        _snprintf(temp, sizeof(temp), "%s (%s)", contactName, szStatus);
                    else if(pContainer->dwFlags & CNT_TITLE_SHOWNAME)
                        _snprintf(temp, sizeof(temp), "%s", contactName);
                }
                else if(pContainer->dwFlags & CNT_TITLE_SHOWNAME) {
                    _snprintf(temp, sizeof(temp), "%s", contactName);
                }
#if defined (_UNICODE)
                MultiByteToWideChar(CP_ACP, 0, temp, -1, tTemp, 200);
#else
                strncpy(tTemp, temp, 200);
#endif                
                if(pContainer->dwFlags & CNT_TITLE_PREFIX) {
                    if(!_tcsncmp(pContainer->szName, _T("default"), CONTAINER_NAMELEN) && _tcslen(pContainer->szName) == 7)
                        _sntprintf(newtitle, 255, _T("[%s] %s"), g_szDefaultContainerName, tTemp);
                    else
                        _sntprintf(newtitle, 255, _T("[%s] %s"), pContainer->szName, tTemp);
                }
                else if(pContainer->dwFlags & CNT_TITLE_SUFFIX) {
                    if(!_tcsncmp(pContainer->szName, _T("default"), CONTAINER_NAMELEN) && _tcslen(pContainer->szName) == 7)
                        _sntprintf(newtitle, 255, _T("%s [%s]"), tTemp, g_szDefaultContainerName);
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
                if (showStatus && wStatus != (WORD)-1)
                    SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadSkinnedProtoIcon(szProto, wStatus));
                break;
            }               
        case WM_TIMER:
            if (wParam == TIMERID_FLASHWND) {
                if (pContainer->nFlash++ <= pContainer->nFlashMax)
                    FlashWindow(hwndDlg, TRUE);
                else {
                    FlashWindow(hwndDlg, FALSE);
                    pContainer->nFlash = 0;
                    KillTimer(hwndDlg, TIMERID_FLASHWND);
                    pContainer->isFlashing = FALSE;
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
				case IDM_NOREPORTMIN:
					CheckMenuItem(GetSystemMenu(hwndDlg, FALSE), IDM_NOREPORTMIN, (pContainer->dwFlags & CNT_DONTREPORT) ? MF_BYCOMMAND | MF_UNCHECKED : MF_BYCOMMAND | MF_CHECKED);
					pContainer->dwFlags ^= CNT_DONTREPORT;
					break;
                case IDM_MOREOPTIONS:
                    if(IsIconic(pContainer->hwnd))
                        SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
					CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM)pContainer);
					break;
                case SC_MAXIMIZE:
                case SC_RESTORE:
                    pContainer->oldSize.cx = pContainer->oldSize.cy = 0;
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
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
                                nPanel = nParts -1;
                                SendMessage(pContainer->hwndStatus, SB_GETRECT, nPanel, (LPARAM)&rc);
                                if (nm->pt.x < rc.left) 
                                    return FALSE;
                            } 
                            else { 
                                nPanel = nm->dwItemSpec; 
                            }
                            if(nPanel == nParts - 1)
                                SendMessage(pContainer->hwndActive, WM_COMMAND, IDC_SELFTYPING, 0);
                        }
                    }
                    break;
                }
                switch (pNMHDR->code) {
                    case TCN_SELCHANGE:
                        ZeroMemory((void *)&item, sizeof(item));
                        iItem = TabCtrl_GetCurSel(hwndTab);
                        item.lParam = 0;
                        item.mask = TCIF_PARAM;
                        if (TabCtrl_GetItem(hwndTab, iItem, &item)) {
                            ShowWindow((HWND)item.lParam, SW_SHOW);
                            if((HWND)item.lParam != pContainer->hwndActive)
                                ShowWindow(pContainer->hwndActive, SW_HIDE);
                            pContainer->hwndActive = (HWND) item.lParam;
                            SetFocus(pContainer->hwndActive);
                        }
                        break;
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
                            WORD iStatus;
#if defined ( _UNICODE )
                            wchar_t w_contactName[80];
#endif

                            if (!DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabtips", 0))
                                break;

                            GetCursorPos(&pt);
                            if ((iItem = GetTabItemFromMouse(hwndTab, &pt)) == -1)
                                break;
                            ZeroMemory((void *)&item, sizeof(item));
                            item.mask = TCIF_PARAM;
                            TabCtrl_GetItem(hwndTab, iItem, &item);
                            if (item.lParam) {
                                cdat = (struct MessageWindowData *) GetWindowLong((HWND)item.lParam, GWL_USERDATA);
                                if (cdat) {
                                    contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) cdat->hContact, 0);
                                    if (contactName) {
                                        if (cdat->szProto) {
                                            iStatus = DBGetContactSettingWord(cdat->hContact, cdat->szProto, "Status", ID_STATUS_OFFLINE);
                                            szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, cdat->szProto == NULL ? ID_STATUS_OFFLINE : iStatus, 0);                                            
                                            nmtt->hinst = NULL;
                                            _snprintf(szTtitle, sizeof(szTtitle), "%s (%s)", contactName, szStatus);
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
                            TCITEM item;

                            GetCursorPos(&pt);
                            pt1 = pt;
                            GetSystemTimeAsFileTime(&ft);
                            if (abs(pt.x - pContainer->pLastPos.x) < 5 && abs(pt.y - pContainer->pLastPos.y) < 5) {
                                if ((ft.dwLowDateTime - pContainer->iLastClick) < GetDoubleClickTime() * 10000) {
                                    if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0)) {
                                        if (MessageBoxA(pContainer->hwnd, Translate(szWarnClose), "Miranda", MB_YESNO | MB_ICONQUESTION) == IDNO)
                                            break;
                                    }
                                    if ((iItem = GetTabItemFromMouse(hwndTab,  &pt)) == -1)
                                        break;
                                    ZeroMemory((void *)&item,  sizeof(item));
                                    item.mask = TCIF_PARAM;
                                    TabCtrl_GetItem(hwndTab, iItem, &item);
                                    if (item.lParam && (HWND) item.lParam == pContainer->hwndActive) {
                                        SendMessage((HWND) item.lParam, WM_CLOSE, 0, 1);
                                    }
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
                                _snprintf(szIndex, 8, "%d", iSelection - IDM_CONTAINERMENU);
                                if(iSelection - IDM_CONTAINERMENU >= 0) {
                                    if(!DBGetContactSetting(NULL, szKey, szIndex, &dbv)) {
#if defined (_UNICODE)
                                        SendMessage((HWND)item.lParam, DM_CONTAINERSELECTED, 0, (LPARAM) dbv.pbVal);
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
                                    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM) pContainer);
                                    break;
                                case ID_TABMENU_CLOSECONTAINER:
                                    SendMessage(hwndDlg, WM_CLOSE, 0, 0);
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
			if (LOWORD(wParam == WA_INACTIVE)) {
				if (pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL)
					pSetLayeredWindowAttributes(hwndDlg, RGB(255,255,255), (BYTE)HIWORD(pContainer->dwTransparency), LWA_ALPHA);
			}
            if (LOWORD(wParam) != WA_ACTIVE)
                break;
        case WM_MOUSEACTIVATE: {
                TCITEM item;

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
                    SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 0);
                    GetClientRect(hwndDlg, &rc);
                    AdjustTabClientRect(pContainer, &rc);
                    pContainer->hwndActive = (HWND) item.lParam;
                    SetWindowPos(pContainer->hwndActive, HWND_TOP, rc.left + 1, rc.top, (rc.right - rc.left) - 8, (rc.bottom - rc.top) - 2, SWP_NOZORDER | SWP_SHOWWINDOW);
                    SetFocus(GetDlgItem(pContainer->hwndActive, IDC_LOG));
                    SetFocus(pContainer->hwndActive);
                    SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
                    SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                    SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                }
                SendMessage((HWND) item.lParam, WM_ACTIVATE, WA_ACTIVE, 0);
            // handle flashing
                if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
                    pContainer->isFlashing = FALSE;
                    pContainer->nFlash = 0;
                    FlashWindow(hwndDlg, FALSE);
                }
                break;
            }
        case WM_ENTERMENULOOP: {
            POINT pt;
            int pos;
            
            GetCursorPos(&pt);
            pos = MenuItemFromPoint(hwndDlg, pContainer->hMenu, pt);
            if(pos == 2) {
                HANDLE hContact;
                HMENU hMenu;

                SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                if(hContact) {
                    hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) hContact, 0);
                    ModifyMenuA(pContainer->hMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR) hMenu, Translate("User"));
                    GetCursorPos(&pt);
                    TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);
                    DestroyMenu(hMenu);
                }
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
                _snprintf(szCname, 40, "%s_Flags", szSetting);
                dwLocalFlags = DBGetContactSettingDword(pContainer->hContactFrom, SRMSGMOD_T, szCname, 0xffffffff);
                _snprintf(szCname, 40, "%s_Trans", szSetting);
                dwLocalTrans = DBGetContactSettingDword(pContainer->hContactFrom, SRMSGMOD_T, szCname, 0xffffffff);
                if(dwLocalFlags == 0xffffffff || dwLocalTrans == 0xffffffff)
                    overridePerContact = 1;
                else
                    overridePerContact = 0;
            }
            if(overridePerContact) {
                _snprintf(szCname, 40, "%s%d_Flags", szSetting, pContainer->iContainerIndex);
                dwLocalFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, szCname, 0xffffffff);
                _snprintf(szCname, 40, "%s%d_Trans", szSetting, pContainer->iContainerIndex);
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
			
            pContainer->nFlashMax = pContainer->dwFlags & CNT_FLASHALWAYS ? 0xffffffff : DBGetContactSettingByte(NULL, SRMSGMOD, "FlashMax", 4);
            break;
		}
        case DM_STATUSBARCHANGED:
            {
                RECT rc;
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                GetWindowRect(hwndDlg, &rc);
                SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, 0);
                SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
                RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
                UpdateWindow(hwndDlg);
                PostMessage(pContainer->hwndActive, DM_STATUSBARCHANGED, 0, 0);
                break;
            }
		case DM_CONFIGURECONTAINER: {
			DWORD ws, wsold, ex = 0, exold = 0;
			HMENU hSysmenu = GetSystemMenu(hwndDlg, FALSE);
			HANDLE hContact;
            
			ws = wsold = GetWindowLong(hwndDlg, GWL_STYLE);
			ws = (pContainer->dwFlags & CNT_NOTITLE) ? ws & ~WS_CAPTION : ws | WS_CAPTION;
			SetWindowLong(hwndDlg, GWL_STYLE, ws);

            pContainer->tBorder = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tborder", 0);
            
			if (LOBYTE(LOWORD(GetVersion())) >= 5  && pSetLayeredWindowAttributes != NULL) {
				ex = GetWindowLong(hwndDlg, GWL_EXSTYLE);
				ex = (pContainer->dwFlags & CNT_TRANSPARENCY) ? ex | WS_EX_LAYERED : ex & ~WS_EX_LAYERED;
				SetWindowLong(hwndDlg , GWL_EXSTYLE , ex);
				if (pContainer->dwFlags & CNT_TRANSPARENCY) {
    				pSetLayeredWindowAttributes(hwndDlg, RGB(255,255,255), (BYTE)LOWORD(pContainer->dwTransparency), LWA_ALPHA);
				}
				
				RedrawWindow(hwndDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
			}

			
			CheckMenuItem(hSysmenu, IDM_NOTITLE, (pContainer->dwFlags & CNT_NOTITLE) ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);

			CheckMenuItem(hSysmenu, IDM_NOREPORTMIN, pContainer->dwFlags & CNT_DONTREPORT ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hSysmenu, IDM_STAYONTOP, pContainer->dwFlags & CNT_STICKY ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
            SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            if (ws != wsold) {
				RECT rc;
				GetWindowRect(hwndDlg, &rc);
				SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, 0);
				SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
				RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
				UpdateWindow(hwndDlg);
				SendMessage(pContainer->hwndActive, DM_SCROLLLOGTOBOTTOM, 0, 0);
            }
            ws = wsold = GetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_STYLE);
            if(pContainer->dwFlags & CNT_SINGLEROWTABCONTROL) {
                ws &= ~TCS_MULTILINE;
                ws |= TCS_SINGLELINE;
            }
            else {
                ws &= ~TCS_SINGLELINE;
                ws |= TCS_MULTILINE;
            }
            
            if(ws != wsold) {
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
            else {
                if(!pContainer->hwndStatus)
                    pContainer->hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndDlg, NULL, g_hInst, NULL);

                if(pContainer->hwndStatus) {
                    RECT rc;
                    GetWindowRect(pContainer->hwndStatus, &rc);

                    if (DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_CHARCOUNT, SRMSGDEFSET_CHARCOUNT)) {
                        int statwidths[4];

                        statwidths[0] = (rc.right - rc.left) - (2 * SB_CHAR_WIDTH) - 50;
                        statwidths[1] = rc.right - rc.left - SB_CHAR_WIDTH - 35;
                        statwidths[2] = rc.right - rc.left - 35;
                        statwidths[3] = -1;
                        SendMessage(pContainer->hwndStatus, SB_SETPARTS, 4, (LPARAM) statwidths);
                    } else {
                        int statwidths[3];
                        statwidths[0] = rc.right - rc.left - SB_CHAR_WIDTH - 50;
                        statwidths[1] = rc.right - rc.left - 35;
                        statwidths[2] = -1;
                        SendMessage(pContainer->hwndStatus, SB_SETPARTS, 3, (LPARAM) statwidths);
                    }
                    ws = GetWindowLong(pContainer->hwndStatus, GWL_STYLE);
                    SetWindowLong(pContainer->hwndStatus, GWL_STYLE, ws & ~SBARS_SIZEGRIP);
                    SendMessage(hwndDlg, DM_STATUSBARCHANGED, 0, 0);
                }
            }
            
            SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
            SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
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
                    POINT pt;
                    HANDLE hContact;
                    HMENU hMenu;
                    
                    SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
                    if(hContact) {
                        hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) hContact, 0);
                        GetCursorPos(&pt);
                        TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);
                        DestroyMenu(hMenu);
                    }
                }
                break;
            }
        case WM_DRAWITEM:
            return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
        case WM_MEASUREITEM:
            return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
        case DM_UPDATEWINICON:
            {
                WORD wStatus;
                HICON hIcon;
                HANDLE hContact;
                char *szProto;
                BYTE showStatus = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON);
                
                SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);

                szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

                if (szProto) {
                    wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);

                    hIcon = LoadSkinnedProtoIcon(szProto, wStatus);

                    if (!showStatus) {
                        SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                        break;
                    }
                    SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hIcon);
                    break;
                }
                break;
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

    			pContainer->hwnd = 0;
    			pContainer->hwndActive = 0;
                pContainer->hMenuContext = 0;
                DestroyMenu(pContainer->hMenu);
                if(pContainer->hwndStatus)
                    DestroyWindow(pContainer->hwndStatus);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_WNDPROC, (LONG)OldTabControlProc);        // un-subclass
    			DestroyWindow(pContainer->hwndTip);
    			RemoveContainerFromList(pContainer);
                if (pContainer)
                    free(pContainer);
                else
                    MessageBoxA(0,"pContainer == 0 in WM_DESTROY", "Warning", MB_OK);
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
    // return -1;      // mouse cursor not over one of the tabs
}

/*
 * cut off contact name to the option value set via Options->Tabbed messaging
 * some people were requesting this, because really long contact list names
 * are causing extraordinary wide tabs and these are looking ugly and wasting
 * screen space.
 */

int CutContactName(char *oldname, char *newname, int size)
{
    int cutMax = (int) DBGetContactSettingWord(NULL, SRMSGMOD_T, "cut_at", 15);
    if ((int)strlen(oldname) <= cutMax)
        strncpy(newname, oldname, size);
    else {
		char fmt[20];
		_snprintf(fmt, 18, "%%%d.%ds...", cutMax, cutMax);
		_snprintf(newname, size - 1, fmt, oldname);
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

// FIXME UNICODE !
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
        pFirstContainer = pContainer->pNextContainer;
        return pFirstContainer;
    }

    do {
        if (pCurrent->pNextContainer == pContainer) {
            pCurrent->pNextContainer = pCurrent->pNextContainer->pNextContainer;
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

int GetProtoIconFromList(const char *szProto, int iStatus)
{
    int i;

    if (szProto == NULL || strlen(szProto) == 0) {
        return 0;
    }
    for (i = 0; i < g_nrProtos; i++) {
        if (!strncmp(protoIconData[i].szName, szProto, sizeof(protoIconData[i].szName))) {
            return protoIconData[i].iFirstIconID + ( iStatus - ID_STATUS_OFFLINE );
        }
    }
    return 0;
}

/*
 * calls the TabCtrl_AdjustRect to calculate the "real" client area of the tab.
 * also checks for the option "hide tabs when only one tab open" and adjusts
 * geometry if necessary
 * rc is the RECT obtained by GetClientRect(hwndTab)
 */

void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc)
{
    HWND hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
    RECT rcTab;

    GetClientRect(hwndTab, &rcTab);
    if (pContainer->iChilds > 1 || !(pContainer->dwFlags & CNT_HIDETABS)) {
        TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);
        rc->top = rcTab.top - 2;
        rc->top += pContainer->tBorder;
        rc->left -= 1;
        rc->right +=3;
        rc->left += pContainer->tBorder;
        rc->right -= pContainer->tBorder;
        rc->bottom -= pContainer->statusBarHeight;
        rc->bottom -= (pContainer->tBorder == 0) ? 3 : 4;
    } else {
        rc->bottom -= pContainer->statusBarHeight;
        rc->bottom -= 3;
        rc->left -= 1;
        rc->right +=3;
    }
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
    if (dbv.type == DBVT_BLOB) {
        _tcsncpy(szName, (TCHAR *)dbv.pbVal, iNameLen);
        DBFreeVariant(&dbv);
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
    char *szKey = "TAB_ContainersW";
#endif    
    HANDLE hhContact = 0;

    do {
        _snprintf(szCounter, 8, "%d", iCounter);
        if (DBGetContactSetting(NULL, szKey, szCounter, &dbv))
            break;      // end of loop...
        else {        // found...
            iCounter++;
            if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_BLOB) {
#if defined(_UNICODE)
                if(!_tcsncmp((TCHAR *)dbv.pbVal, _T("**free**"), lstrlenW((TCHAR *)dbv.pbVal))) {
#else                    
                if(!strncmp(dbv.pszVal, "**free**", strlen(dbv.pszVal))) {
#endif                    
                    DBFreeVariant(&dbv);
                    continue;
                }
                if (dwAction & CNT_ENUM_DELETE) {
#if defined (_UNICODE)
                    if(!_tcsncmp((TCHAR *)dbv.pbVal, szTarget, lstrlenW((TCHAR *)dbv.pbVal)) && lstrlenW((TCHAR *)dbv.pbVal) == lstrlenW(szTarget)) {
#else                        
                    if(!strncmp(dbv.pszVal, szTarget, strlen(dbv.pszVal)) && strlen(dbv.pszVal) == strlen(szTarget)) {
#endif                        
                        DeleteContainer(iCounter - 1);
                        DBFreeVariant(&dbv);
                        break;
                    }
                }
                if (dwAction & CNT_ENUM_RENAME) {
#if defined (_UNICODE)
                    if(!_tcsncmp((TCHAR *)dbv.pbVal, szTarget, lstrlenW((TCHAR *)dbv.pbVal)) && lstrlenW((TCHAR *)dbv.pbVal) == lstrlenW(szTarget)) {
#else                    
                    if(!strncmp(dbv.pszVal, szTarget, strlen(dbv.pszVal)) && strlen(dbv.pszVal) == strlen(szTarget)) {
#endif                        
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
        char *szFlags = "CNTW__FLags";
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
        if(dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_BLOB) {
#if defined (_UNICODE)
            _DBWriteContactSettingWString(NULL, szKey, szIndex, _T("**free**"));
#else
            DBWriteContactSettingString(NULL, szKey, szIndex, "**free**");
#endif            
            hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
            while (hhContact) {
                DBVARIANT dbv_c;
#if defined (_UNICODE)
                if (!DBGetContactSetting(hhContact, SRMSGMOD_T, "containerW", &dbv_c)) {
                    if(_tcscmp((TCHAR *)dbv_c.pbVal, (TCHAR *)dbv.pbVal) && lstrlenW((TCHAR *)dbv_c.pbVal) == lstrlenW((TCHAR *)dbv.pbVal)) {
                        DBDeleteContactSetting(hhContact, SRMSGMOD_T, "containerW");
#else                    
                if (!DBGetContactSetting(hhContact, SRMSGMOD_T, "container", &dbv_c)) {
                    if(!strcmp(dbv_c.pszVal, dbv.pszVal) && strlen(dbv_c.pszVal) == strlen(dbv.pszVal)) {
                        DBDeleteContactSetting(hhContact, SRMSGMOD_T, "container");
#endif                    
                    }
                    DBFreeVariant(&dbv_c);
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
                if(!_tcscmp((TCHAR *)dbv.pbVal, (TCHAR *)dbv_c.pbVal) && lstrlenW((TCHAR *)dbv.pbVal) == lstrlenW((TCHAR *)dbv_c.pbVal)) {
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
            }
            hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hhContact, 0);
        }
        DBFreeVariant(&dbv);
    }
}

void _DBWriteContactSettingWString(HANDLE hContact, char *szKey, char *szSetting, const wchar_t *value)
{
    DBCONTACTWRITESETTING dbcws = { 0 };
    
    dbcws.szModule = szKey;
    dbcws.szSetting = szSetting;
    dbcws.value.type = DBVT_BLOB;
    dbcws.value.pbVal = (BYTE *) value;
    dbcws.value.cpbVal = (WORD)((lstrlenW(value) + 1) * sizeof(TCHAR));
    CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM) hContact, (LPARAM)&dbcws);
}

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

    if(g_hMenuContainer != 0) {
        HMENU submenu = GetSubMenu(g_hMenuContext, 0);
        RemoveMenu(submenu, 5, MF_BYPOSITION);
        DestroyMenu(g_hMenuContainer);
        g_hMenuContainer = 0;
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
        if (dbv.type == DBVT_BLOB) {
            if (_tcsncmp((TCHAR *)dbv.pbVal, _T("**free**"), CONTAINER_NAMELEN)) {
                AppendMenu(hMenu, MF_STRING, IDM_CONTAINERMENU + i, (TCHAR *)dbv.pbVal);
            }
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

    //InsertMenu(g_hMenuContext, ID_TABMENU_ATTACHTOCONTAINER, MF_BYCOMMAND | MF_POPUP, (UINT_PTR) hMenu, _T("Attach to"));
    InsertMenuA(g_hMenuContext, ID_TABMENU_ATTACHTOCONTAINER, MF_BYCOMMAND | MF_POPUP, (UINT_PTR) hMenu, Translate("Attach to"));
    g_hMenuContainer = hMenu;
    return hMenu;
}
