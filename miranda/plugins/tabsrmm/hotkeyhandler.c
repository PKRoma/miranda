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

    b) the event hook which is fired whenever a contact setting is changed. We need
       this hook for detecting changes to the user picture.
*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"

extern struct ContainerWindowData *pFirstContainer;
extern HANDLE hMessageWindowList;
extern struct SendJob sendJobs[NR_SENDJOBS];

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
                        if(IsIconic(pTargetContainer->hwnd))
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
             * handle an event from the popup module (mostly window activation). Since popups may run in different threads, the message
             * is posted to our invisible hotkey handler which does always run within the main thread.
             * wParam is the hContact
             */
        case DM_HANDLECLISTEVENT:
            {
                CLISTEVENT *cle = (CLISTEVENT *)CallService(MS_CLIST_GETEVENT, wParam, 0);
                if (cle) {
                    if (ServiceExists(cle->pszService)) {
                        CallService(cle->pszService, (WPARAM)NULL, (LPARAM)cle);
                        CallService(MS_CLIST_REMOVEEVENT, (WPARAM)cle->hContact, (LPARAM)cle->hDbEvent);
                    }
                    
                }
                break;
            }
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

            /*
             * this is called by the event hook whenever a contact setting changes...
             * we need to process the request as quickly as possible, otherwise it
             * would slow down things a lot when many settings are commited to the
             * database.
             */
        /*
        case DM_CONTACTSETTINGCHANGED:
        {
            DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *)lParam;

            HANDLE hwnd = WindowList_Find(hMessageWindowList,(HANDLE)wParam);

            if(hwnd == 0)       // we are not interested in this event if there is no open message window/tab
                return 0;       // for the hContact.
            
            if(strlen(dbcws->szModule) != 12)
                return 0;
            if(strncmp(dbcws->szModule, "ContactPhoto", 12))
                return 0;

            SendMessage(hwnd, DM_PICTURECHANGED, 0, 0);
            return 0;
        }
        */
        
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

