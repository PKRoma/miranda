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
#include "commonheaders.h"
#pragma hdrstop
#include "m_toptoolbar.h"
#include "m_updater.h"
#include "m_avatars.h"
#include "chat/chat.h"

#ifdef __MATHMOD_SUPPORT
    #include "m_MathModule.h"
#endif

static char *relnotes[] = {
    "{\\rtf1\\ansi\\deff0\\pard\\li%u\\fi-%u\\ri%u\\tx%u}",
    "\\par\t\\b\\ul1 Release notes for version 1.1.0.17\\b0\\ul0\\par ",
    "*\tHovering a tab will now show a tipper/mToolTip style tip.\\par",
    "*\t\\b Moved main option page to Options->Message Sessions!\\b0. The old \"Message Window\" entry is now gone.\\par",
    NULL
};

BOOL show_relnotes = FALSE;

MYGLOBALS myGlobals;
NEN_OPTIONS nen_options;
extern PLUGININFO pluginInfo;

static void InitREOleCallback(void);
static void UnloadIcons();
static int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam);
static int AvatarChanged(WPARAM wParam, LPARAM lParam);
static int MyAvatarChanged(WPARAM wParam, LPARAM lParam);

HANDLE hMessageWindowList, hUserPrefsWindowList;
static HANDLE hEventDbEventAdded, hEventDbSettingChange, hEventContactDeleted, hEventDispatch, hEvent_ttbInit, hTTB_Slist, hTTB_Tray;
static HANDLE hModulesLoadedEvent;
static HANDLE hEventSmileyAdd = 0;
HANDLE *hMsgMenuItem = NULL;
int hMsgMenuItemCount = 0;

static HANDLE hSVC[14];
#define H_MS_MSG_SENDMESSAGE 0
#define H_MS_MSG_SENDMESSAGEW 1
#define H_MS_MSG_FORWARDMESSAGE 2
#define H_MS_MSG_GETWINDOWAPI 3
#define H_MS_MSG_GETWINDOWCLASS 4
#define H_MS_MSG_GETWINDOWDATA 5
#define H_MS_MSG_READMESSAGE 6
#define H_MS_MSG_TYPINGMESSAGE 7
#define H_MS_MSG_MOD_MESSAGEDIALOGOPENED 8
#define H_MS_TABMSG_SETUSERPREFS 9
#define H_MS_TABMSG_TRAYSUPPORT 10 
#define H_MSG_MOD_GETWINDOWFLAGS 11

HMODULE hDLL;
PSLWA pSetLayeredWindowAttributes = 0;
PULW pUpdateLayeredWindow = 0;
PFWEX MyFlashWindowEx = 0;
PAB MyAlphaBlend = 0;
PGF MyGradientFill = 0;

extern      struct ContainerWindowData *pFirstContainer;
extern      BOOL CALLBACK DlgProcUserPrefs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      int g_chat_integration_enabled;
extern      struct SendJob *sendJobs;
extern      struct MsgLogIcon msgLogIcons[NR_LOGICONS * 3];
extern      HINSTANCE g_hInst;
extern      BOOL CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      int g_sessionshutdown;
extern      BOOL CALLBACK DlgProcTemplateHelp(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      ICONDESC *g_skinIcons;
extern      int g_nrSkinIcons;
extern      struct RTFColorTable *rtf_ctable;

HANDLE g_hEvent_MsgWin;
HANDLE g_hEvent_MsgPopup;

static struct MsgLogIcon ttb_Slist = {0}, ttb_Traymenu = {0};

HMODULE g_hIconDLL = 0;

int     Chat_IconsChanged(WPARAM wp, LPARAM lp), Chat_ModulesLoaded(WPARAM wp, LPARAM lp);
void    Chat_AddIcons(void);
int     Chat_PreShutdown(WPARAM wParam, LPARAM lParam);

static int IEViewOptionsChanged(WPARAM wParam, LPARAM lParam)
{
	WindowList_Broadcast(hMessageWindowList, DM_IEVIEWOPTIONSCHANGED, 0, 0);
	return 0;
}

static int SmileyAddOptionsChanged(WPARAM wParam, LPARAM lParam)
{
    WindowList_Broadcast(hMessageWindowList, DM_SMILEYOPTIONSCHANGED, 0, 0);
    if(g_chat_integration_enabled) 
        SM_BroadcastMessage(NULL, DM_SMILEYOPTIONSCHANGED, 0, 0, FALSE);
    return 0;
}

/*
static int ContactSecureChanged(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam))
	{
        SendMessage(hwnd, DM_SECURE_CHANGED, 0, 0);
    }
    return 0;
}
*/
/*
 * Message API 0.0.0.3 services
 */

static int GetWindowClass(WPARAM wParam, LPARAM lParam)
{
	char *szBuf = (char*)wParam;
	int size = (int)lParam;
	mir_snprintf(szBuf, size, "tabSRMM");
	return 0;
}

static int GetWindowData(WPARAM wParam, LPARAM lParam)
{
	MessageWindowInputData *mwid = (MessageWindowInputData*)wParam;
	MessageWindowOutputData *mwod = (MessageWindowOutputData*)lParam;
	HWND hwnd;
    SESSION_INFO *si = NULL;
    
	if( mwid == NULL || mwod == NULL) 
        return 1;
	if(mwid->cbSize != sizeof(MessageWindowInputData) || mwod->cbSize != sizeof(MessageWindowOutputData)) 
        return 1;
	if(mwid->hContact == NULL) 
        return 1;
	if(mwid->uFlags != MSG_WINDOW_UFLAG_MSG_BOTH) 
        return 1;
	hwnd = WindowList_Find(hMessageWindowList, mwid->hContact);
	if(hwnd) {
		mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
		mwod->hwndWindow = hwnd;
		mwod->local = GetParent(GetParent(hwnd));
		SendMessage(hwnd, DM_GETWINDOWSTATE, 0, 0);
		mwod->uState = GetWindowLong(hwnd, DWL_MSGRESULT);
        return 0;
	}
    else if((si = SM_FindSessionByHCONTACT(mwid->hContact)) != NULL && si->hWnd != 0) {
        mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
        mwod->hwndWindow = si->hWnd;
        mwod->local = GetParent(GetParent(si->hWnd));
        SendMessage(si->hWnd, DM_GETWINDOWSTATE, 0, 0);
        mwod->uState = GetWindowLong(si->hWnd, DWL_MSGRESULT);
        return 0;
    }
	else {
        mwod->uState = 0;
        mwod->hContact = 0;
        mwod->hwndWindow = 0;
        mwod->uFlags = 0;
    }
		
	return 1;
}

/*
 * service function. retrieves the message window data for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
            * is directly used as the handle for the target window
 */
            
static int SetUserPrefs(WPARAM wParam, LPARAM lParam)
{
    HWND hWnd = WindowList_Find(hUserPrefsWindowList, (HANDLE)wParam);
    if(hWnd) {
        SetFocus(hWnd);
        return 0;
    }
    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS), 0, DlgProcUserPrefs, (LPARAM)wParam);
    return 0;
}

static int TTB_Loaded(WPARAM wParam, LPARAM lParam)
{
    if(ServiceExists(MS_TTB_ADDBUTTON) && myGlobals.m_WinVerMajor >= 5) {
        TTBButton ttb = {0};
        CacheIconToBMP(&ttb_Slist, myGlobals.g_iconContainer, GetSysColor(COLOR_3DFACE), 16, 16);
        CacheIconToBMP(&ttb_Traymenu, myGlobals.g_buttonBarIcons[16], GetSysColor(COLOR_3DFACE), 16, 16);
        ttb.hbBitmapUp = ttb_Slist.hBmp;
        ttb.hbBitmapDown = ttb_Slist.hBmp;
        ttb.cbSize = sizeof(ttb);
        ttb.pszServiceDown = MS_TABMSG_TRAYSUPPORT;
        ttb.pszServiceUp = MS_TABMSG_TRAYSUPPORT;
        ttb.wParamDown = 1;
        ttb.wParamUp = 0;
        ttb.name = "tabSRMM Session List";
        ttb.dwFlags = TTBBF_VISIBLE | TTBBF_SHOWTOOLTIP;
        hTTB_Slist = (HANDLE)CallService(MS_TTB_ADDBUTTON, (WPARAM)&ttb, 0);
        ttb.hbBitmapUp = ttb_Traymenu.hBmp;
        ttb.hbBitmapDown = ttb_Traymenu.hBmp;
        ttb.lParamDown = 1;
        ttb.lParamUp = 1;
        ttb.name = "tabSRMM Tray Menu";
        hTTB_Tray = (HANDLE)CallService(MS_TTB_ADDBUTTON, (WPARAM)&ttb, 0);
    }
    UnhookEvent(hEvent_ttbInit);
    return 0;
}
static int Service_OpenTrayMenu(WPARAM wParam, LPARAM lParam)
{
    if(ServiceExists(MS_TTB_SETBUTTONSTATE))
        CallService(MS_TTB_SETBUTTONSTATE, lParam ? (WPARAM)hTTB_Tray : (WPARAM)hTTB_Slist, TTBST_RELEASED);
        
    SendMessage(myGlobals.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, lParam == 0 ? WM_LBUTTONUP : WM_RBUTTONUP);
    return 0;
}
/*
 * service function. retrieves the message window flags for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
            * is directly used as the handle for the target window
 */
            
static int GetMessageWindowFlags(WPARAM wParam, LPARAM lParam)
{
    HWND hwndTarget = (HWND)lParam;

    if(hwndTarget == 0)
        hwndTarget = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
    
    if(hwndTarget) {
        struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndTarget, GWL_USERDATA);
        if(dat)
            return (dat->dwFlags);
        else
            return 0;
    }
    else
        return 0;
}

/*
 * return the version of the window api supported
 */

static int GetWindowAPI(WPARAM wParam, LPARAM lParam)
{
	return PLUGIN_MAKE_VERSION(0,0,0,2);
}

/*
 * service function finds a message session 
 * wParam = contact handle for which we want the window handle
 * thanks to bio for the suggestion of this service
 * if wParam == 0, then lParam is considered to be a valid window handle and 
 * the function tests the popup mode of the target container

 * returns the hwnd if there is an open window or tab for the given hcontact (wParam),
 * or (if lParam was specified) the hwnd if the window exists.
 * 0 if there is none (or the popup mode of the target container was configured to "hide"
 * the window..
 */

int MessageWindowOpened(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = 0;
	struct ContainerWindowData *pContainer = NULL;

    if(wParam)
        hwnd = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
    else if(lParam)
        hwnd = (HWND) lParam;
    else
        hwnd = NULL;
    
    if (hwnd) {
		SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
		if (pContainer) {
            if (pContainer->dwFlags & CNT_DONTREPORT) {
                if (IsIconic(pContainer->hwnd))
                    return 0;
            }
            if (pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED) {
                if (!IsIconic(pContainer->hwnd) && GetForegroundWindow() != pContainer->hwnd && GetActiveWindow() != pContainer->hwnd)
                    return 0;
            }
            if (pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE) {
                if(pContainer->hwndActive == hwnd)
                    return 1;
                else
                    return 0;
            }
        }
        return 1;
    }
    else
        return 0;
}

        /*
         * this is the global ack dispatcher. It handles both ACKTYPE_MESSAGE and ACKTYPE_AVATAR events
         * for ACKTYPE_MESSAGE it searches the corresponding send job in the queue and, if found, dispatches
         * it to the owners window
         */

static int ProtoAck(WPARAM wParam, LPARAM lParam)
{
    ACKDATA *pAck = (ACKDATA *) lParam;
    HWND hwndDlg = 0;
    int i, j, iFound = NR_SENDJOBS;

    if(lParam == 0)
        return 0;
    
    if(pAck->type == ACKTYPE_MESSAGE) {
        for(j = 0; j < NR_SENDJOBS; j++) {
            for (i = 0; i < sendJobs[j].sendCount; i++) {
                if (pAck->hProcess == sendJobs[j].hSendId[i] && pAck->hContact == sendJobs[j].hContact[i]) {
                    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(sendJobs[j].hwndOwner, GWL_USERDATA);
                    if(dat) {
                        if(dat->hContact == sendJobs[j].hOwner) {
                            iFound = j;
                            break;
                        }
                        
                    }
                }
            }
            if (iFound == NR_SENDJOBS)          // no mathing entry found in this queue entry.. continue
                continue;
            else
                break;
        }
        if(iFound == NR_SENDJOBS)               // no matching send info found in the queue
            return 0;
        else {                                  // the job was found
            SendMessage(sendJobs[iFound].hwndOwner, HM_EVENTSENT, (WPARAM)MAKELONG(iFound, i), lParam);
            return 0;
        }
    }
    /*
     * handle status message - pAck->lParam has the message (char *)
     * only care about requests we have made from a message window, so check the hProcess
     */
    if(pAck->type == ACKTYPE_AWAYMSG) {
        struct MessageWindowData *dat = 0;
        HWND hwnd = WindowList_Find(hMessageWindowList, pAck->hContact);
        if(hwnd)
            dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
        else
            return 0;
        
        if(pAck->result == ACKRESULT_SUCCESS) {
            if(dat && dat->hProcessAwayMsg == pAck->hProcess) {
                dat->hProcessAwayMsg = 0;
                if(pAck->lParam)
#if defined(_UNICODE)
                    MultiByteToWideChar(dat->codePage, 0, (char *)pAck->lParam, -1, dat->statusMsg, safe_sizeof(dat->statusMsg) - 1);
#else
                    strncpy(dat->statusMsg, (char *)pAck->lParam, sizeof(dat->statusMsg) - 1);
#endif                
                else
                    lstrcpyn(dat->statusMsg, myGlobals.m_szNoStatus, safe_sizeof(dat->statusMsg) - 1);
                dat->statusMsg[safe_sizeof(dat->statusMsg) - 1] = 0;
                SendMessage(hwnd, DM_ACTIVATETOOLTIP, 0, 0);
            }
        }
        else if(pAck->result = ACKRESULT_FAILED) {
            if(dat && dat->hProcessAwayMsg == pAck->hProcess) {
                dat->hProcessAwayMsg = 0;
                SendMessage(hwnd, DM_ACTIVATETOOLTIP, 0, (LPARAM)_T("Either there is no status message available, or the protocol could not retrieve it."));
            }
        }
        return 0;
    }
    return 0;
}

/*                                                              
 * this handler is called first in the message window chain - it will handle events for which a message window
 * is already open. if not, it will do nothing and the 2nd handler (MessageEventAdded) will perform all
 * the needed actions.
 *
 * this handler POSTs the event to the message window procedure - so it is fast and can exit quickly which will
 * improve the overall responsiveness when receiving messages.
 */

static int DispatchNewEvent(WPARAM wParam, LPARAM lParam) {
	if (wParam) {
        HWND h = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
		if(h)
            PostMessage(h, HM_DBEVENTADDED, wParam, lParam);            // was SENDMESSAGE !!! XXX
	}
	return 0;
}

static int ReadMessageCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwndExisting;
    HANDLE hContact = ((CLISTEVENT *) lParam)->hContact;
    struct ContainerWindowData *pContainer = 0;
    
    //EnterCriticalSection(&cs_sessions);
    hwndExisting = WindowList_Find(hMessageWindowList, hContact);
    
    if (hwndExisting != NULL) {
        SendMessage(hwndExisting, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
        if(pContainer != NULL)
            ActivateExistingTab(pContainer, hwndExisting);
    }
    else {
        TCHAR szName[CONTAINER_NAMELEN + 1];
        GetContainerNameForContact(hContact, szName, CONTAINER_NAMELEN);
        pContainer = FindContainerByName(szName);
        if (pContainer == NULL)
            pContainer = CreateContainer(szName, FALSE, hContact);
		CreateNewTabForContact (pContainer, hContact, 0, NULL, TRUE, TRUE, FALSE, 0);
    }
    //LeaveCriticalSection(&cs_sessions);
    return 0;
}

static int MessageEventAdded(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
    CLISTEVENT cle;
    DBEVENTINFO dbei;
    BYTE bAutoPopup = FALSE, bAutoCreate = FALSE, bAutoContainer = FALSE, bAllowAutoCreate = 0;
    struct ContainerWindowData *pContainer = 0;
    TCHAR szName[CONTAINER_NAMELEN + 1];
    DWORD dwStatusMask = 0;
    
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.cbBlob = 0;
    CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);

    hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam);
    if (dbei.flags & DBEF_SENT || dbei.eventType != EVENTTYPE_MESSAGE || dbei.flags & DBEF_READ) {
        /*
         * care about popups for non-message events for contacts w/o an openend window
         * if a window is open, the msg window itself will care about showing the popup
         */
        if(dbei.eventType != EVENTTYPE_MESSAGE && !IsStatusEvent(dbei.eventType) && hwnd == 0 && !(dbei.flags & DBEF_SENT)) {
            if(!(dbei.flags & DBEF_READ))
                tabSRMM_ShowPopup(wParam, lParam, dbei.eventType, 0, 0, 0, dbei.szModule, 0);
        }
        return 0;
    }
    
	CallServiceSync(MS_CLIST_REMOVEEVENT, wParam, (LPARAM) 1);

    if (hwnd) {
        /*
        struct ContainerWindowData *pTargetContainer = 0;
        if(dbei.eventType == EVENTTYPE_MESSAGE) {
            SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pTargetContainer);
            if (pTargetContainer)
                PostMessage(hwnd, DM_PLAYINCOMINGSOUND, 0, 0);
        }*/
        return 0;
    }

    /*
     * if no window is open, we are not interested in anything else but unread message events
     */
    
    /* new message */
    if(!nen_options.iNoSounds)
        SkinPlaySound("AlertMsg");

    if(nen_options.iNoAutoPopup)
        goto nowindowcreate;
    
    GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);

    bAutoPopup = DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
    bAutoCreate = DBGetContactSettingByte(NULL, SRMSGMOD_T, "autotabs", 0);
    bAutoContainer = DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocontainer", 0);
    dwStatusMask = DBGetContactSettingDword(NULL, SRMSGMOD_T, "autopopupmask", -1);

    bAllowAutoCreate = FALSE;
    
    if(dwStatusMask == -1)
        bAllowAutoCreate = TRUE;
    else {
        char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)wParam, 0);
        DWORD dwStatus = 0;
        if(szProto) {
            dwStatus = (DWORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
            if(dwStatus == 0 || dwStatus <= ID_STATUS_OFFLINE || ((1<<(dwStatus - ID_STATUS_ONLINE)) & dwStatusMask))              // should never happen, but...
                bAllowAutoCreate = TRUE;
        }
    }
    if (bAllowAutoCreate && (bAutoPopup || bAutoCreate)) {
        BOOL bActivate = TRUE, bPopup = TRUE;
        if(bAutoPopup) {
            bActivate = bPopup = TRUE;
            if((pContainer = FindContainerByName(szName)) == NULL)
                pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
            CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, FALSE, 0);
            return 0;
        }
        else {
            bActivate = FALSE;
            bPopup = (BOOL) DBGetContactSettingByte(NULL, SRMSGMOD_T, "cpopup", 0);
            pContainer = FindContainerByName(szName);
            if (pContainer != NULL) {
                if((pContainer->bInTray || IsIconic(pContainer->hwnd)) && myGlobals.m_AutoSwitchTabs) {
                    bActivate = TRUE;
                    pContainer->dwFlags |= CNT_DEFERREDTABSELECT;
                }
                if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0) &&  !_tcsncmp(pContainer->szName, _T("default"), 6)) {
                    if((pContainer = FindMatchingContainer(_T("default"), (HANDLE)wParam)) != NULL) {
                        CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
                        return 0;
                    }
                    else if(bAutoContainer) {
                        pContainer = CreateContainer(szName, CNT_CREATEFLAG_MINIMIZED, (HANDLE)wParam);         // 2 means create minimized, don't popup...
                        CreateNewTabForContact(pContainer,  (HANDLE) wParam,  0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
                        SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
                        return 0;
                    }
                }
                else {
                    CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
                    return 0;
                }
                    
            }
            else {
                if(bAutoContainer) {
                    pContainer = CreateContainer(szName, CNT_CREATEFLAG_MINIMIZED, (HANDLE)wParam);         // 2 means create minimized, don't popup...
                    CreateNewTabForContact(pContainer,  (HANDLE) wParam,  0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
                    SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
                    return 0;
                }
            }
        }
    }

    /*
     * for tray support, we add the event to the tray menu. otherwise we send it back to
     * the contact list for flashing
     */
nowindowcreate:
    if(!(dbei.flags & DBEF_READ)) {
        UpdateTrayMenu(0, 0, dbei.szModule, NULL, (HANDLE)wParam, 1);
        if(!nen_options.bTraySupport || myGlobals.m_WinVerMajor < 5) {
             TCHAR toolTip[256], *contactName;
            ZeroMemory(&cle, sizeof(cle));
            cle.cbSize = sizeof(cle);
            cle.hContact = (HANDLE) wParam;
            cle.hDbEvent = (HANDLE) lParam;
              cle.flags = CLEF_TCHAR;
            cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
            cle.pszService = "SRMsg/ReadMessage";
            contactName = (TCHAR*) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, wParam, GCDNF_TCHAR);
            mir_sntprintf(toolTip, SIZEOF(toolTip), TranslateT("Message from %s"), contactName);
            cle.ptszTooltip = toolTip;
            CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
        }
        tabSRMM_ShowPopup(wParam, lParam, dbei.eventType, 0, 0, 0, dbei.szModule, 0);
    }
    return 0;
}

#if defined(_UNICODE)
int SendMessageCommand_W(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
	char *szProto;
    struct NewMessageWindowLParam newData = { 0 };
    struct ContainerWindowData *pContainer = 0;
    int isSplit = 1;
    
    if(GetCurrentThreadId() != myGlobals.dwThreadID) {
        //_DebugTraceA("sendmessagecommand_W called from different thread (%d), main thread = %d", GetCurrentThreadId(), myGlobals.dwThreadID);
        if(lParam) {
            unsigned iLen = lstrlenW((wchar_t *)lParam);
            wchar_t *tszText = (wchar_t *)malloc((iLen + 1) * sizeof(wchar_t));
            wcsncpy(tszText, (wchar_t *)lParam, iLen + 1);
            tszText[iLen] = 0;
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMANDW, wParam, (LPARAM)tszText);
        }
        else
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMANDW, wParam, 0);
        return 0;
    }
    
    /* does the HCONTACT's protocol support IM messages? */
    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
    if (szProto) {
		if (!CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND)
			return 0;
    }
    else {
		/* unknown contact */
        return 0;
    }

    //EnterCriticalSection(&cs_sessions);
    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam)) {
        if (lParam) {
            HWND hEdit;
            hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
            SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
            SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM) (TCHAR *) lParam);
        }
        SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwnd);
    }
    else {
        TCHAR szName[CONTAINER_NAMELEN + 1];
        /*
         * attempt to fix "double tabs" opened by MS_MSG_SENDMESSAGE
         * strange problem, maybe related to the window list service in miranda?
         */
        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "trayfix", 0)) {
            if(myGlobals.hLastOpenedContact == (HANDLE)wParam) {
                //LeaveCriticalSection(&cs_sessions);
                return 0;
            }
        }
        myGlobals.hLastOpenedContact = (HANDLE)wParam;
        GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
        pContainer = FindContainerByName(szName);
        if (pContainer == NULL)
            pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, 1, (const char *)lParam, TRUE, TRUE, FALSE, 0);
    }
    //LeaveCriticalSection(&cs_sessions);
    return 0;
}

#endif

int SendMessageCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
	char *szProto;
    struct NewMessageWindowLParam newData = { 0 };
    struct ContainerWindowData *pContainer = 0;
    int isSplit = 1;

    if(GetCurrentThreadId() != myGlobals.dwThreadID) {
        //_DebugTraceA("sendmessagecommand called from different thread (%d), main thread = %d", GetCurrentThreadId(), myGlobals.dwThreadID);
        if(lParam) {
            unsigned iLen = lstrlenA((char *)lParam);
            char *szText = (char *)malloc(iLen + 1);
            strncpy(szText, (char *)lParam, iLen + 1);
            szText[iLen] = 0;
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMAND, wParam, (LPARAM)szText);
        }
        else
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMAND, wParam, 0);
        return 0;
    }

    /* does the HCONTACT's protocol support IM messages? */
    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
    if (szProto) {
		if (!CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND)
			return 0;
    }
    else {
		/* unknown contact */
        return 0;
    }

    //EnterCriticalSection(&cs_sessions);
    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam)) {
        if (lParam) {
            HWND hEdit;
            hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
            SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
            SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM) (char *) lParam);
        }
        SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwnd);
    }
    else {
        TCHAR szName[CONTAINER_NAMELEN + 1];
        /*
         * attempt to fix "double tabs" opened by MS_MSG_SENDMESSAGE
         * strange problem, maybe related to the window list service in miranda?
         */
        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "trayfix", 0)) {
            if(myGlobals.hLastOpenedContact == (HANDLE)wParam) {
                //LeaveCriticalSection(&cs_sessions);
                return 0;
            }
        }
        myGlobals.hLastOpenedContact = (HANDLE)wParam;
        GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
        pContainer = FindContainerByName(szName);
        if (pContainer == NULL)
            pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, (const char *) lParam, TRUE, TRUE, FALSE, 0);
    }
    //LeaveCriticalSection(&cs_sessions);
    return 0;
}

static int ForwardMessage(WPARAM wParam, LPARAM lParam)
{
	HWND hwndNew, hwndOld;
	RECT rc;
    struct ContainerWindowData *pContainer = 0;
    int iS = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0);

    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0);        // temporarily disable single window mode for forwarding...
    pContainer = FindContainerByName(_T("default"));
    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", (BYTE)iS);
    
    if (pContainer == NULL)
        pContainer = CreateContainer(_T("default"), FALSE, 0);
    
	hwndOld = pContainer->hwndActive;
	hwndNew = CreateNewTabForContact(pContainer, 0, 0, (const char *) lParam, TRUE, TRUE, FALSE, 0);
	if(hwndNew) {
		if(hwndOld)
			ShowWindow(hwndOld, SW_HIDE);
		SendMessage(hwndNew, DM_SPLITTERMOVED, 0, (LPARAM) GetDlgItem(hwndNew, IDC_MULTISPLITTER));

		GetWindowRect(pContainer->hwnd, &rc);
		SetWindowPos(pContainer->hwnd, 0, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top) - 1, 0);
		SetWindowPos(pContainer->hwnd, 0, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), SWP_SHOWWINDOW);

		SetFocus(GetDlgItem(hwndNew, IDC_MESSAGE));
	}
    return 0;
}

static int TypingMessageCommand(WPARAM wParam, LPARAM lParam)
{
    CLISTEVENT *cle = (CLISTEVENT *) lParam;

    if (!cle)
        return 0;
    SendMessageCommand((WPARAM) cle->hContact, 0);
    return 0;
}

static int TypingMessage(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
    int issplit = 1, foundWin = 0;
    
    if (!DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING))
        return 0;
    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam)) {
        SendMessage(hwnd, DM_TYPING, 0, lParam);
    }
    // check popup config to decide wheter tray notification should be shown.. even for open windows/tabs
    if(hwnd) {
        foundWin = MessageWindowOpened(0, (LPARAM)hwnd);
    }
    else
        foundWin = 0;
    
    if ((int) lParam && !foundWin && DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN)) {
        char szTip[256];

        _snprintf(szTip, sizeof(szTip), Translate("%s is typing a message"), (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, wParam, 0));
        if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY) && !DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST)) {
            MIRANDASYSTRAYNOTIFY tn;
            tn.szProto = NULL;
            tn.cbSize = sizeof(tn);
            tn.szInfoTitle = Translate("Typing Notification");
            tn.szInfo = szTip;
            tn.dwInfoFlags = NIIF_INFO;
            tn.uTimeout = 1000 * 4;
            CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & tn);
        }
        else {
            CLISTEVENT cle;

            ZeroMemory(&cle, sizeof(cle));
            cle.cbSize = sizeof(cle);
            cle.hContact = (HANDLE) wParam;
			cle.hDbEvent = (HANDLE) 1;
            cle.flags = CLEF_ONLYAFEW;
            cle.hIcon = myGlobals.g_buttonBarIcons[5];
            cle.pszService = "SRMsg/TypingMessage";
            cle.pszTooltip = szTip;
            CallServiceSync(MS_CLIST_REMOVEEVENT, wParam, (LPARAM) 1);
            CallServiceSync(MS_CLIST_ADDEVENT, wParam, (LPARAM) & cle);
        }
    }
    return 0;
}

static int MessageSettingChanged(WPARAM wParam, LPARAM lParam)
{
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
    char *szProto = NULL;

    HANDLE hwnd = WindowList_Find(hMessageWindowList,(HANDLE)wParam);

    if(hwnd == 0 && wParam != 0) {      // we are not interested in this event if there is no open message window/tab
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
        if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
            return 0;                       // filter out settings we aren't interested in...
        if(DBGetContactSettingWord((HANDLE)wParam, SRMSGMOD_T, "isFavorite", 0))
            AddContactToFavorites((HANDLE)wParam, NULL, szProto, NULL, 0, 0, 0, myGlobals.g_hMenuFavorites, DBGetContactSettingDword((HANDLE)wParam, SRMSGMOD_T, "ANSIcodepage", myGlobals.m_LangPackCP));
        if(DBGetContactSettingDword((HANDLE)wParam, SRMSGMOD_T, "isRecent", 0))
            AddContactToFavorites((HANDLE)wParam, NULL, szProto, NULL, 0, 0, 0, myGlobals.g_hMenuRecent, DBGetContactSettingDword((HANDLE)wParam, SRMSGMOD_T, "ANSIcodepage", myGlobals.m_LangPackCP));
        return 0;       // for the hContact.
    }

    if(wParam == 0 && strstr("Nick,yahoo_id", cws->szSetting)) {
        WindowList_Broadcast(hMessageWindowList, DM_OWNNICKCHANGED, 0, (LPARAM)cws->szModule);
        return 0;
    }

    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);

    if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
        return 0;
    
    // metacontacts support

    if(!lstrcmpA(cws->szModule, "MetaContacts") && !lstrcmpA(cws->szSetting, "Nick"))       // filter out this setting to avoid infinite loops while trying to obtain the most online contact
        return 0;
    
    if(hwnd) {
        if(strstr("MyHandle,Status,Nick,ApparentMode,Default,ForceSend,IdleTS,XStatusId", cws->szSetting))
            SendMessage(hwnd, DM_UPDATETITLE, 0, 0);
        else if(lstrlenA(cws->szSetting) > 6 && !strncmp(cws->szSetting, "Status", 6)) {
            SendMessage(hwnd, DM_UPDATETITLE, 0, 1);
        }
        else if(!strcmp(cws->szSetting, "MirVer"))
            SendMessage(hwnd, DM_CLIENTCHANGED, 0, 0);
        else if(strstr("StatusMsg,StatusDescr,XStatusMsg,YMsg", cws->szSetting))
            PostMessage(hwnd, DM_UPDATESTATUSMSG, 0, 0);
    }
    
    return 0;
}

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam)) {
        struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);

        if(dat)
            dat->bWasDeleted = 1;
        SendMessage(hwnd, WM_CLOSE, 0, 1);
    }
    return 0;
}

static void RestoreUnreadMessageAlerts(void)
{
    CLISTEVENT cle = { 0 };
    DBEVENTINFO dbei = { 0 };
    char toolTip[256];
    int windowAlreadyExists;
    int usingReadNext = 0;
    
    int autoPopup = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
    HANDLE hDbEvent, hContact;

    dbei.cbSize = sizeof(dbei);
    cle.cbSize = sizeof(cle);
    cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    cle.pszService = "SRMsg/ReadMessage";

    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) hContact, 0);
        while (hDbEvent) {
            dbei.cbBlob = 0;
            CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) & dbei);
            if (!(dbei.flags & (DBEF_SENT | DBEF_READ)) && dbei.eventType == EVENTTYPE_MESSAGE) {
                windowAlreadyExists = WindowList_Find(hMessageWindowList, hContact) != NULL;
                if (!usingReadNext && windowAlreadyExists)
                    continue;

                if (0) {
                    struct NewMessageWindowLParam newData = { 0 };
                    newData.hContact = hContact;
                    newData.isWchar = 0;
                    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSPLITNEW), NULL, DlgProcMessage, (LPARAM) & newData);
                }
                else {
                    cle.hContact = hContact;
                    cle.hDbEvent = hDbEvent;
                    _snprintf(toolTip, sizeof(toolTip), Translate("Message from %s"), (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0));
                    cle.pszTooltip = toolTip;
                    CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
                }
            }
            hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, (WPARAM) hDbEvent, 0);
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
}

static int SplitmsgModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    CLISTMENUITEM mi;
    PROTOCOLDESCRIPTOR **protocol;
    int protoCount, i;
    DBVARIANT dbv;
    MENUITEMINFOA mii = {0};
    HMENU submenu;
    BOOL bIEView;
    static Update upd = {0};
#if defined(_UNICODE)
    static char szCurrentVersion[30];
    static char *szVersionUrl = "http://miranda.or.at/files/tabsrmm/tabsrmm.txt";
    static char *szUpdateUrl = "http://miranda.or.at/files/tabsrmm/tabsrmmW.zip";
    static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=2457";
    static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=2457";
#else
    static char szCurrentVersion[30];
    static char *szVersionUrl = "http://miranda.or.at/files/tabsrmm/version.txt";
    static char *szUpdateUrl = "http://miranda.or.at/files/tabsrmm/tabsrmm.zip";
    static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=1401";
    static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=1401";
#endif    
    static char *szPrefix = "tabsrmm ";

#if defined(_UNICODE)
    if ( !ServiceExists( MS_DB_CONTACT_GETSETTING_STR )) {
        MessageBox(NULL, TranslateT( "This plugin requires db3x plugin version 0.5.1.0 or later" ), _T("tabSRMM (Unicode)"), MB_OK );
        return 1;
    }
#endif    
    UnhookEvent(hModulesLoadedEvent);
    hEventDbSettingChange = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, MessageSettingChanged);
    hEventContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);

    hEventDispatch = HookEvent(ME_DB_EVENT_ADDED, DispatchNewEvent);
    hEventDbEventAdded = HookEvent(ME_DB_EVENT_ADDED, MessageEventAdded);

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = -2000090000;
    mi.flags = 0;
    mi.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    mi.pszName = Translate("&Message");
    mi.pszService = MS_MSG_SENDMESSAGE;
    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) & protoCount, (LPARAM) & protocol);
    for (i = 0; i < protoCount; i++) {
        if (protocol[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        if (CallProtoService(protocol[i]->szName, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND) {
            mi.pszContactOwner = protocol[i]->szName;
            hMsgMenuItem = realloc(hMsgMenuItem, (hMsgMenuItemCount + 1) * sizeof(HANDLE));
            hMsgMenuItem[hMsgMenuItemCount++] = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
        }
    }
    if(ServiceExists(MS_SKIN2_ADDICON))
        HookEvent(ME_SKIN2_ICONSCHANGED, IcoLibIconsChanged);
	if(ServiceExists(MS_AV_GETAVATARBITMAP)) {
       HookEvent(ME_AV_AVATARCHANGED, AvatarChanged);
	   HookEvent(ME_AV_MYAVATARCHANGED, MyAvatarChanged);
	}
    HookEvent(ME_CLIST_DOUBLECLICKED, SendMessageCommand);
    RestoreUnreadMessageAlerts();
    for(i = 0; i < NR_BUTTONBARICONS; i++)
        myGlobals.g_buttonBarIcons[i] = 0;
    LoadIconTheme();
    CreateImageList(TRUE);

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = HBMMENU_CALLBACK;
    submenu = GetSubMenu(myGlobals.g_hMenuContext, 7);
    for(i = 0; i <= 8; i++)
        SetMenuItemInfoA(submenu, (UINT_PTR)i, TRUE, &mii);

    BuildContainerMenu();
    
#if defined(_UNICODE)
    #define SHORT_MODULENAME "tabSRMsgW (unicode)"
#else
    #define SHORT_MODULENAME "tabSRMsg"
#endif    
    if(DBGetContactSetting(NULL, "KnownModules", SHORT_MODULENAME, &dbv))
        DBWriteContactSettingString(NULL, "KnownModules", SHORT_MODULENAME, "SRMsg,Tab_SRMsg,TAB_Containers,TAB_ContainersW");
    else
        DBFreeVariant(&dbv);

    if(ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) {
        myGlobals.g_SmileyAddAvail = 1;
        hEventSmileyAdd = HookEvent(ME_SMILEYADD_OPTIONSCHANGED, SmileyAddOptionsChanged);
    }

    if(ServiceExists(MS_FAVATAR_GETINFO))
        myGlobals.g_FlashAvatarAvail = 1;
    
    bIEView = ServiceExists(MS_IEVIEW_WINDOW);
    if(bIEView) {
        BOOL bOldIEView = DBGetContactSettingByte(NULL, SRMSGMOD_T, "ieview_installed", 0);
        if(bOldIEView != bIEView)
            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "default_ieview", 1);
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "ieview_installed", 1);
        HookEvent(ME_IEVIEW_OPTIONSCHANGED, IEViewOptionsChanged);
    }
    else
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "ieview_installed", 0);

    myGlobals.m_hwndClist = (HWND)CallService(MS_CLUI_GETHWND, 0, 0);
#ifdef __MATHMOD_SUPPORT    		
     myGlobals.m_MathModAvail = ServiceExists(MATH_RTF_REPLACE_FORMULAE) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantmathmod", 1);
     if(myGlobals.m_MathModAvail) {
         char *szDelim = (char *)CallService(MATH_GET_STARTDELIMITER, 0, 0);
         if(szDelim) {
#if defined(_UNICODE)
             MultiByteToWideChar(CP_ACP, 0, szDelim, -1, myGlobals.m_MathModStartDelimiter, sizeof(myGlobals.m_MathModStartDelimiter));
#else
             strncpy(myGlobals.m_MathModStartDelimiter, szDelim, sizeof(myGlobals.m_MathModStartDelimiter));
#endif
             CallService(MTH_FREE_MATH_BUFFER, 0, (LPARAM)szDelim);
         }
     }
#endif     
    //myGlobals.g_wantSnapping = ServiceExists("Utils/SnapWindowProc") && DBGetContactSettingByte(NULL, SRMSGMOD_T, "usesnapping", 0);
    
    if(ServiceExists(MS_MC_GETDEFAULTCONTACT))
        myGlobals.g_MetaContactsAvail = 1;

    if(ServiceExists(MS_POPUP_ADDPOPUPEX))
        myGlobals.g_PopupAvail = 1;

#if defined(_UNICODE)
    if(ServiceExists(MS_POPUP_ADDPOPUPW))
        myGlobals.g_PopupWAvail = 1;
#endif    
    /*
    if(ServiceExists(MS_FONT_REGISTER)) {
        myGlobals.g_FontServiceAvail = 1;
        FS_RegisterFonts();
        hEvent_FontService = HookEvent(ME_FONT_RELOAD, FS_ReloadFonts);
    }
    */
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", -1) == -1)
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 2);

    myGlobals.g_hwndHotkeyHandler = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_HOTKEYSLAVE), 0, HotkeyHandlerDlgProc);

    CreateTrayMenus(TRUE);
    if(nen_options.bTraySupport)
        CreateSystrayIcon(TRUE);

    ZeroMemory((void *)&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = -500050005;
    mi.hIcon = myGlobals.g_iconContainer;
    mi.pszContactOwner = NULL;
    mi.pszName = Translate( "&tabSRMM settings" );
    mi.pszService = MS_TABMSG_SETUSERPREFS;
    myGlobals.m_UserMenuItem = ( HANDLE )CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );
    PreTranslateDates();
    hEvent_ttbInit = HookEvent("TopToolBar/ModuleLoaded", TTB_Loaded);

    myGlobals.m_hFontWebdings = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Wingdings");

    // updater plugin support

    upd.cbSize = sizeof(upd);
    upd.szComponentName = pluginInfo.shortName;
    upd.pbVersion = (BYTE *)CreateVersionStringPlugin(&pluginInfo, szCurrentVersion);
    upd.cpbVersion = strlen((char *)upd.pbVersion);
    upd.szVersionURL = szFLVersionUrl;
    upd.szUpdateURL = szFLUpdateurl;
#if defined(_UNICODE)
    upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">tabSRMM Unicode ";
#else
    upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">tabSRMM ";
#endif
    upd.cpbVersionPrefix = strlen((char *)upd.pbVersionPrefix);

    upd.szBetaUpdateURL = szUpdateUrl;
    upd.szBetaVersionURL = szVersionUrl;
    upd.pbVersion = szCurrentVersion;
    upd.cpbVersion = lstrlenA(szCurrentVersion);
    upd.pbBetaVersionPrefix = (BYTE *)szPrefix;
    upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);

    
    if(ServiceExists(MS_UPDATE_REGISTER))
        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);

    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "useskin", 0))
        ReloadContainerSkin(1, 1);
    
    CacheLogFonts();
    Chat_ModulesLoaded(wParam, lParam);
    if(DBGetContactSettingDword(NULL, SRMSGMOD_T, "last_relnotes", 0) < pluginInfo.version) {
        ViewReleaseNotes();
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "last_relnotes", pluginInfo.version);
    }
	return 0;
}

static int OkToExit(WPARAM wParam, LPARAM lParam)
{
    CreateSystrayIcon(0);
    CreateTrayMenus(0);
    g_sessionshutdown = 1;
    UnhookEvent(hEventDbEventAdded);
    UnhookEvent(hEventDispatch);
    UnhookEvent(hEventDbSettingChange);
    UnhookEvent(hEventContactDeleted);
    return 0;
}

static int PreshutdownSendRecv(WPARAM wParam, LPARAM lParam)
{

    if(g_chat_integration_enabled)
        Chat_PreShutdown(0, 0);

    //SM_BroadcastMessage(NULL, GC_CLOSEWINDOW, 0, 2, FALSE);         // lParam == 2 -> close at end

    while(pFirstContainer)
        SendMessage(pFirstContainer->hwnd, WM_CLOSE, 0, 1);

    DestroyServiceFunction(hSVC[H_MS_MSG_SENDMESSAGE]);
#if defined(_UNICODE)
    DestroyServiceFunction(hSVC[H_MS_MSG_SENDMESSAGEW]);
#endif
    DestroyServiceFunction(hSVC[H_MS_MSG_FORWARDMESSAGE]);
    DestroyServiceFunction(hSVC[H_MS_MSG_GETWINDOWAPI]);
    DestroyServiceFunction(hSVC[H_MS_MSG_GETWINDOWCLASS]);
    DestroyServiceFunction(hSVC[H_MS_MSG_GETWINDOWDATA]);
    DestroyServiceFunction(hSVC[H_MS_MSG_READMESSAGE]);
    DestroyServiceFunction(hSVC[H_MS_MSG_TYPINGMESSAGE]);

    /*
     * tabSRMM - specific services
     */
    
    DestroyServiceFunction(hSVC[H_MS_MSG_MOD_MESSAGEDIALOGOPENED]);
    DestroyServiceFunction(hSVC[H_MS_TABMSG_SETUSERPREFS]);
    DestroyServiceFunction(hSVC[H_MS_TABMSG_TRAYSUPPORT]);
    DestroyServiceFunction(hSVC[H_MSG_MOD_GETWINDOWFLAGS]);

    SI_DeinitStatusIcons();
    /*
     * the event API
     */

	DestroyHookableEvent(g_hEvent_MsgWin);
    DestroyHookableEvent(g_hEvent_MsgPopup);

	NEN_WriteOptions(&nen_options);
    DestroyWindow(myGlobals.g_hwndHotkeyHandler);

    //UnregisterClass(_T("TabSRMSG_Win"), g_hInst);
    UnregisterClass(_T("TSStatusBarClass"), g_hInst);
    UnregisterClassA("TSTabCtrlClass", g_hInst);
    return 0;
}

int SplitmsgShutdown(void)
{
    int i;
    
	DestroyCursor(myGlobals.hCurSplitNS);
    DestroyCursor(myGlobals.hCurHyperlinkHand);
    DestroyCursor(myGlobals.hCurSplitWE);
    DeleteObject(myGlobals.m_hFontWebdings);
    FreeLibrary(GetModuleHandleA("riched20"));
	FreeLibrary(GetModuleHandleA("user32"));
    FreeVSApi();
    if(g_hIconDLL)
        FreeLibrary(g_hIconDLL);
    
    ImageList_RemoveAll(myGlobals.g_hImageList);
    ImageList_Destroy(myGlobals.g_hImageList);

    OleUninitialize();
    if (hMsgMenuItem) {
        free(hMsgMenuItem);
        hMsgMenuItem = NULL;
        hMsgMenuItemCount = 0;
    }
    DestroyMenu(myGlobals.g_hMenuContext);
    if(myGlobals.g_hMenuContainer)
        DestroyMenu(myGlobals.g_hMenuContainer);
    if(myGlobals.g_hMenuEncoding)
        DestroyMenu(myGlobals.g_hMenuEncoding);

    UnloadIcons();
    FreeTabConfig();

    if(rtf_ctable)
        free(rtf_ctable);

    UnloadTSButtonModule(0, 0);
	if(sendJobs) {
		for(i = 0; i < NR_SENDJOBS; i++) {
		    if(sendJobs[i].sendBuffer != NULL)
			    free(sendJobs[i].sendBuffer);
		}
        free(sendJobs);
	}

    if(ttb_Slist.hBmp)
        DeleteCachedIcon(&ttb_Slist);
    if(ttb_Traymenu.hBmp)
        DeleteCachedIcon(&ttb_Traymenu);

	IMG_DeleteItems();
	if(g_hIconDLL) {
		FreeLibrary(g_hIconDLL);
		g_hIconDLL = 0;
	}
    if(g_skinIcons)
        free(g_skinIcons);

    return 0;
}

static int MyAvatarChanged(WPARAM wParam, LPARAM lParam)
{
	struct ContainerWindowData *pContainer = pFirstContainer;

	if(wParam == 0 || IsBadReadPtr((void *)wParam, 4))
		return 0;

	while(pContainer) {
		BroadCastContainer(pContainer, DM_MYAVATARCHANGED, wParam, lParam);
		pContainer = pContainer->pNextContainer;
	}
	return 0;
}

static int AvatarChanged(WPARAM wParam, LPARAM lParam)
{
    struct avatarCacheEntry *ace = (struct avatarCacheEntry *)lParam;
    HWND hwnd = WindowList_Find(hMessageWindowList, (HANDLE)wParam);

	if(wParam == 0) {			// protocol picture has changed...
		WindowList_Broadcast(hMessageWindowList, DM_PROTOAVATARCHANGED, wParam, lParam);
		return 0;
	}
	if(hwnd) {
        struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
        if(dat) {
            dat->ace = ace;
            if(dat->hwndFlash == 0)
                dat->panelWidth = -1;				// force new size calculations (not for flash avatars)
            DM_RecalcPictureSize(hwnd, dat);
            if(dat->showPic == 0 || dat->showInfoPic == 0)
                GetAvatarVisibility(hwnd, dat);
            ShowPicture(hwnd, dat, TRUE);
            dat->dwFlagsEx |= MWF_EX_AVATARCHANGED;
        }
    }
    return 0;
}

static int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
    LoadFromIconLib();
    CacheMsgLogIcons();
    if(g_chat_integration_enabled)
        Chat_IconsChanged(wParam, lParam);
    return 0;
}

static int IconsChanged(WPARAM wParam, LPARAM lParam)
{
    if (hMsgMenuItem) {
        int j;
        CLISTMENUITEM mi;

        mi.cbSize = sizeof(mi);
        mi.flags = CMIM_ICON;
        mi.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);

        for (j = 0; j < hMsgMenuItemCount; j++) {
            CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMsgMenuItem[j], (LPARAM) & mi);
        }
    }
    CreateImageList(FALSE);
    CacheMsgLogIcons();
    WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
    WindowList_Broadcast(hMessageWindowList, DM_UPDATEWINICON, 0, 0);
    if(g_chat_integration_enabled)
        Chat_IconsChanged(wParam, lParam);
    
    return 0;
}

int LoadSendRecvMessageModule(void)
{
    int nOffset = 0;

    INITCOMMONCONTROLSEX icex;
    
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    {
        TIME_ZONE_INFORMATION tzinfo;
        DWORD dwResult;


        dwResult = GetTimeZoneInformation(&tzinfo);

        nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
        goto tzdone;
        
        switch(dwResult)
        {

        case TIME_ZONE_ID_STANDARD:
          nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
          break;

        case TIME_ZONE_ID_DAYLIGHT:
          nOffset = -(tzinfo.Bias + tzinfo.DaylightBias) * 60;
          break;

        case TIME_ZONE_ID_UNKNOWN:
        case TIME_ZONE_ID_INVALID:
        default:
          nOffset = 0;
          break;

        }

/*
        const time_t now = time(NULL);
        struct tm gmt = *gmtime(&now);
        time_t gmt_time = mktime(&gmt);
        myGlobals.local_gmt_diff = (int)difftime(now, gmt_time);*/
    }

tzdone:
    if (LoadLibraryA("riched20.dll") == NULL) {
        if (IDYES !=
            MessageBoxA(0,
                        Translate
                        ("Miranda could not load the built-in message module, riched20.dll is missing. If you are using Windows 95 or WINE please make sure you have riched20.dll installed. Press 'Yes' to continue loading Miranda."),
                        Translate("Information"), MB_YESNO | MB_ICONINFORMATION))
            return 1;
        return 0;
    }
    OleInitialize(NULL);
    InitREOleCallback();
    ZeroMemory((void *)&myGlobals, sizeof(myGlobals));
    ZeroMemory((void *)&nen_options, sizeof(nen_options));
    
    hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
    hUserPrefsWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

    sendJobs = (struct SendJob *)malloc(NR_SENDJOBS * sizeof(struct SendJob));
    ZeroMemory(sendJobs, NR_SENDJOBS * sizeof(struct SendJob));

    InitOptions();
    //hEventDbSettingChange = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, MessageSettingChanged);
    //hEventContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);
    hModulesLoadedEvent = HookEvent(ME_SYSTEM_MODULESLOADED, SplitmsgModulesLoaded);
    HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);
    HookEvent(ME_PROTO_CONTACTISTYPING, TypingMessage);
    HookEvent(ME_PROTO_ACK, ProtoAck);
    HookEvent(ME_SYSTEM_PRESHUTDOWN, PreshutdownSendRecv);
    HookEvent(ME_SYSTEM_OKTOEXIT, OkToExit);

	//HookEvent("SecureIM/Established",ContactSecureChanged);
	//HookEvent("SecureIM/Disabled",ContactSecureChanged);
    InitAPI();
    
    SkinAddNewSoundEx("RecvMsgActive", Translate("Messages"), Translate("Incoming (Focused Window)"));
    SkinAddNewSoundEx("RecvMsgInactive", Translate("Messages"), Translate("Incoming (Unfocused Window)"));
    SkinAddNewSoundEx("AlertMsg", Translate("Messages"), Translate("Incoming (New Session)"));
    SkinAddNewSoundEx("SendMsg", Translate("Messages"), Translate("Outgoing"));
    SkinAddNewSoundEx("SendError", Translate("Messages"), Translate("Error sending message"));

    myGlobals.hCurSplitNS = LoadCursor(NULL, IDC_SIZENS);
    myGlobals.hCurSplitWE = LoadCursor(NULL, IDC_SIZEWE);
    myGlobals.hCurHyperlinkHand = LoadCursor(NULL, IDC_HAND);
    if (myGlobals.hCurHyperlinkHand == NULL)
        myGlobals.hCurHyperlinkHand = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_HYPERLINKHAND));

    LoadTSButtonModule();
    //RegisterContainer();
    RegisterTabCtrlClass();
    ReloadGlobals();
    myGlobals.dwThreadID = GetCurrentThreadId();
    GetDataDir();
    ReloadTabConfig();
    NEN_ReadOptions(&nen_options);

    myGlobals.g_hMenuContext = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TABCONTEXT));
    CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) myGlobals.g_hMenuContext, 0);   

    DBWriteContactSettingByte(NULL, TEMPLATES_MODULE, "setup", 2);
    LoadDefaultTemplates();

    BuildCodePageList();
    myGlobals.m_VSApiEnabled = InitVSApi();
	GetDefaultContainerTitleFormat();
    myGlobals.m_GlobalContainerFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containerflags", CNT_FLAGS_DEFAULT);
    if(!(myGlobals.m_GlobalContainerFlags & CNT_NEWCONTAINERFLAGS))
        myGlobals.m_GlobalContainerFlags = CNT_FLAGS_DEFAULT;
    myGlobals.m_GlobalContainerTrans = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containertrans", CNT_TRANS_DEFAULT);
    myGlobals.local_gmt_diff = nOffset;

    hDLL = GetModuleHandleA("user32");
    pSetLayeredWindowAttributes = (PSLWA) GetProcAddress(hDLL,"SetLayeredWindowAttributes");
    pUpdateLayeredWindow = (PULW) GetProcAddress(hDLL, "UpdateLayeredWindow");
    MyFlashWindowEx = (PFWEX) GetProcAddress(hDLL, "FlashWindowEx");
    MyAlphaBlend = (PAB) GetProcAddress(GetModuleHandleA("msimg32"), "AlphaBlend");
    MyGradientFill = (PGF) GetProcAddress(GetModuleHandleA("msimg32"), "GradientFill");

    return 0;
}

static IRichEditOleCallbackVtbl reOleCallbackVtbl;
struct CREOleCallback reOleCallback;

static STDMETHODIMP_(ULONG) CREOleCallback_QueryInterface(struct CREOleCallback *lpThis, REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, &IID_IRichEditOleCallback)) {
        *ppvObj = lpThis;
        lpThis->lpVtbl->AddRef((IRichEditOleCallback *) lpThis);
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static STDMETHODIMP_(ULONG) CREOleCallback_AddRef(struct CREOleCallback *lpThis)
{
    if (lpThis->refCount == 0) {
        if (S_OK != StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &lpThis->pictStg))
            lpThis->pictStg = NULL;
        lpThis->nextStgId = 0;
    }
    return ++lpThis->refCount;
}

static STDMETHODIMP_(ULONG) CREOleCallback_Release(struct CREOleCallback *lpThis)
{
    if (--lpThis->refCount == 0) {
        if (lpThis->pictStg)
            lpThis->pictStg->lpVtbl->Release(lpThis->pictStg);
    }
    return lpThis->refCount;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_ContextSensitiveHelp(struct CREOleCallback *lpThis, BOOL fEnterMode)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_DeleteObject(struct CREOleCallback *lpThis, LPOLEOBJECT lpoleobj)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetClipboardData(struct CREOleCallback *lpThis, CHARRANGE * lpchrg, DWORD reco, LPDATAOBJECT * lplpdataobj)
{
    return E_NOTIMPL;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetContextMenu(struct CREOleCallback *lpThis, WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE * lpchrg, HMENU * lphmenu)
{
    return E_INVALIDARG;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetDragDropEffect(struct CREOleCallback *lpThis, BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetInPlaceContext(struct CREOleCallback *lpThis, LPOLEINPLACEFRAME * lplpFrame, LPOLEINPLACEUIWINDOW * lplpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    return E_INVALIDARG;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetNewStorage(struct CREOleCallback *lpThis, LPSTORAGE * lplpstg)
{
    WCHAR szwName[64];
    char szName[64];
    wsprintfA(szName, "s%u", lpThis->nextStgId);
    MultiByteToWideChar(CP_ACP, 0, szName, -1, szwName, sizeof(szwName) / sizeof(szwName[0]));
    if (lpThis->pictStg == NULL)
        return STG_E_MEDIUMFULL;
    return lpThis->pictStg->lpVtbl->CreateStorage(lpThis->pictStg, szwName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, lplpstg);
}

static STDMETHODIMP_(HRESULT) CREOleCallback_QueryAcceptData(struct CREOleCallback *lpThis, LPDATAOBJECT lpdataobj, CLIPFORMAT * lpcfFormat, DWORD reco, BOOL fReally, HGLOBAL hMetaPict)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_QueryInsertObject(struct CREOleCallback *lpThis, LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_ShowContainerUI(struct CREOleCallback *lpThis, BOOL fShow)
{
    return S_OK;
}

static void InitREOleCallback(void)
{
    reOleCallback.lpVtbl = &reOleCallbackVtbl;
    reOleCallback.lpVtbl->AddRef = (ULONG(__stdcall *) (IRichEditOleCallback *)) CREOleCallback_AddRef;
    reOleCallback.lpVtbl->Release = (ULONG(__stdcall *) (IRichEditOleCallback *)) CREOleCallback_Release;
    reOleCallback.lpVtbl->QueryInterface = (ULONG(__stdcall *) (IRichEditOleCallback *, REFIID, PVOID *)) CREOleCallback_QueryInterface;
    reOleCallback.lpVtbl->ContextSensitiveHelp = (HRESULT(__stdcall *) (IRichEditOleCallback *, BOOL)) CREOleCallback_ContextSensitiveHelp;
    reOleCallback.lpVtbl->DeleteObject = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPOLEOBJECT)) CREOleCallback_DeleteObject;
    reOleCallback.lpVtbl->GetClipboardData = (HRESULT(__stdcall *) (IRichEditOleCallback *, CHARRANGE *, DWORD, LPDATAOBJECT *)) CREOleCallback_GetClipboardData;
    reOleCallback.lpVtbl->GetContextMenu = (HRESULT(__stdcall *) (IRichEditOleCallback *, WORD, LPOLEOBJECT, CHARRANGE *, HMENU *)) CREOleCallback_GetContextMenu;
    reOleCallback.lpVtbl->GetDragDropEffect = (HRESULT(__stdcall *) (IRichEditOleCallback *, BOOL, DWORD, LPDWORD)) CREOleCallback_GetDragDropEffect;
    reOleCallback.lpVtbl->GetInPlaceContext = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPOLEINPLACEFRAME *, LPOLEINPLACEUIWINDOW *, LPOLEINPLACEFRAMEINFO))
        CREOleCallback_GetInPlaceContext;
    reOleCallback.lpVtbl->GetNewStorage = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPSTORAGE *)) CREOleCallback_GetNewStorage;
    reOleCallback.lpVtbl->QueryAcceptData = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPDATAOBJECT, CLIPFORMAT *, DWORD, BOOL, HGLOBAL)) CREOleCallback_QueryAcceptData;
    reOleCallback.lpVtbl->QueryInsertObject = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPCLSID, LPSTORAGE, LONG)) CREOleCallback_QueryInsertObject;
    reOleCallback.lpVtbl->ShowContainerUI = (HRESULT(__stdcall *) (IRichEditOleCallback *, BOOL)) CREOleCallback_ShowContainerUI;
    reOleCallback.refCount = 0;
}



/*
 * tabbed mode support functions...
 * (C) by Nightwish
 *
 * this function searches and activates the tab belonging to the given hwnd (which is the
 * hwnd of a message dialog window)
 */

int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild)
{
	struct MessageWindowData *dat = 0;
	NMHDR nmhdr;

	dat = (struct MessageWindowData *) GetWindowLong(hwndChild, GWL_USERDATA);	// needed to obtain the hContact for the message window
	if(dat) {
        ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
		nmhdr.code = TCN_SELCHANGE;
        if(TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS)) > 1 && !(pContainer->dwFlags & CNT_DEFERREDTABSELECT)) {
            TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetTabIndexFromHWND(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), hwndChild));
            SendMessage(pContainer->hwnd, WM_NOTIFY, 0, (LPARAM) &nmhdr);	// just select the tab and let WM_NOTIFY do the rest
        }
        if(dat->bType == SESSIONTYPE_IM)
            SendMessage(pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
        if(IsIconic(pContainer->hwnd)) {
            SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            SetForegroundWindow(pContainer->hwnd);
        }
        else if(GetForegroundWindow() != pContainer->hwnd)
            SetForegroundWindow(pContainer->hwnd);
        if(dat->bType == SESSIONTYPE_IM)
            SetFocus(GetDlgItem(hwndChild, IDC_MESSAGE));
        //if(myGlobals.m_ExtraRedraws)
        //    RedrawWindow(pContainer->hwndActive, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
		return TRUE;
	} else
		return FALSE;
}

/*
 * this function creates and activates a new tab within the given container.
 * bActivateTab: make the new tab the active one
 * bPopupContainer: restore container if it was minimized, otherwise flash it...
 */

HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTab, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent)
{
	TCHAR *contactName = NULL, newcontactname[128];
    char *szProto, *szStatus, tabtitle[128];
	WORD wStatus;
    int	newItem;
    HWND hwndNew = 0;
    HWND hwndTab;
    struct NewMessageWindowLParam newData = {0};
#if defined(_UNICODE)
    WCHAR contactNameW[100];
#endif    
    if(WindowList_Find(hMessageWindowList, hContact) != 0) {
        _DebugPopup(hContact, "Warning: trying to create duplicate window");
        return 0;
    }
    // if we have a max # of tabs/container set and want to open something in the default container...
    if(hContact != 0 && DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0) &&  !_tcsncmp(pContainer->szName, _T("default"), 6)) {
        if((pContainer = FindMatchingContainer(_T("default"), hContact)) == NULL) {
            TCHAR szName[CONTAINER_NAMELEN + 1];

            _sntprintf(szName, CONTAINER_NAMELEN, _T("default"));
            pContainer = CreateContainer(szName, CNT_CREATEFLAG_CLONED, hContact);
        }
    }
    
	newData.hContact = hContact;
    newData.isWchar = isSend;
    newData.szInitialText = pszInitialText;
    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) newData.hContact, 0);

    ZeroMemory((void *)&newData.item, sizeof(newData.item));

	// obtain various status information about the contact
#if defined(_UNICODE)
    contactNameW[0] = 0;
    MY_GetContactDisplayNameW(hContact, contactNameW, 100, szProto, 0);
    contactName = contactNameW;
#else
	contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) newData.hContact, 0);
#endif    

	/*
	 * cut nickname if larger than x chars...
	 */

    if(contactName && lstrlen(contactName) > 0) {
        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0))
            CutContactName(contactName, newcontactname, safe_sizeof(newcontactname));
        else {
            lstrcpyn(newcontactname, contactName, safe_sizeof(newcontactname));
            newcontactname[127] = 0;
        }
    }
    else
        lstrcpyn(newcontactname, _T("_U_"), sizeof(newcontactname) / sizeof(TCHAR));

	wStatus = szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) newData.hContact, szProto, "Status", ID_STATUS_OFFLINE);
	szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE)newData.hContact, szProto, "Status", ID_STATUS_OFFLINE), 0);
    
	if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabstatus", 0))
		_snprintf(tabtitle, sizeof(tabtitle), "%s (%s)   ", newcontactname, szStatus);
	else
		_snprintf(tabtitle, sizeof(tabtitle), "%s   ", newcontactname);

#ifdef _UNICODE
	{
    wchar_t w_tabtitle[256];
    if(MultiByteToWideChar(myGlobals.m_LangPackCP, 0, tabtitle, -1, w_tabtitle, safe_sizeof(w_tabtitle)) != 0)
        newData.item.pszText = w_tabtitle;
	}
#else
	newData.item.pszText = tabtitle;
#endif
    //newData.item.iImage = GetProtoIconFromList(szProto, wStatus);

	newData.item.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
    newData.item.iImage = 0;
    newData.item.cchTextMax = 255;

    hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
	// hide the active tab
	if(pContainer->hwndActive && bActivateTab)
		ShowWindow(pContainer->hwndActive, SW_HIDE);

    {
        int iTabIndex_wanted = DBGetContactSettingDword(hContact, SRMSGMOD_T, "tabindex", pContainer->iChilds * 100);
        int iCount = TabCtrl_GetItemCount(hwndTab);
        TCITEM item = {0};
        HWND hwnd;
        struct MessageWindowData *dat;
        int relPos;
        int i;

        pContainer->iTabIndex = iCount;
        if(iCount > 0) {
            for(i = iCount - 1; i >= 0; i--) {
                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(hwndTab, i, &item);
                hwnd = (HWND)item.lParam;
                dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
                if(dat) {
                    relPos = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "tabindex", i * 100);
                    if(iTabIndex_wanted <= relPos)
                        pContainer->iTabIndex = i;
                }
            }
        }
    }
	newItem = TabCtrl_InsertItem(hwndTab, pContainer->iTabIndex, &newData.item);
    SendMessage(hwndTab, EM_REFRESHWITHOUTCLIP, 0, 0);
	if (bActivateTab)
        TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), newItem);
	newData.iTabID = newItem;
	newData.iTabImage = newData.item.iImage;
	newData.pContainer = pContainer;
    newData.iActivate = (int) bActivateTab;
    pContainer->iChilds++;
    newData.bWantPopup = bWantPopup;
    newData.hdbEvent = hdbEvent;
    hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSPLITNEW), GetDlgItem(pContainer->hwnd, IDC_MSGTABS), DlgProcMessage, (LPARAM) &newData);
    SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
    // if the container is minimized, then pop it up...
    if(IsIconic(pContainer->hwnd)) {
        if(bPopupContainer) {
            SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            SetFocus(pContainer->hwndActive);
        }
        else {
            if(pContainer->dwFlags & CNT_NOFLASH)
                SendMessage(pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
            else
                FlashContainer(pContainer, 1, 0);
        }
    }
    if (bActivateTab) {
        SetFocus(hwndNew);
        RedrawWindow(pContainer->hwnd, NULL, NULL, RDW_INVALIDATE);
        UpdateWindow(pContainer->hwnd);
        if(GetForegroundWindow() != pContainer->hwnd && bPopupContainer == TRUE)
            SetForegroundWindow(pContainer->hwnd);
        //PostMessage(hwndNew, DM_SCROLLLOGTOBOTTOM, 0, 0);
    }
	return hwndNew;		// return handle of the new dialog
}

/* 
 * this is used by the 2nd containermode (limit tabs on default containers).
 * it searches a container with "room" for the new tabs or otherwise creates
 * a new (cloned) one.
 */

struct ContainerWindowData *FindMatchingContainer(const TCHAR *szName, HANDLE hContact)
{
    struct ContainerWindowData *pDesired = 0;
    int iMaxTabs = DBGetContactSettingDword(NULL, SRMSGMOD_T, "maxtabs", 0);

    if(iMaxTabs > 0 && DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0) && !_tcsncmp(szName, _T("default"), 6)) {
        struct ContainerWindowData *pCurrent = pFirstContainer;
        // search a "default" with less than iMaxTabs opened...
        while(pCurrent) {
            if(!_tcsncmp(pCurrent->szName, _T("default"), 6) && pCurrent->iChilds < iMaxTabs) {
                pDesired = pCurrent;
                break;
            }
            pCurrent = pCurrent->pNextContainer;
        }
        return(pDesired != NULL ? pDesired : NULL);
    }
    else
        return FindContainerByName(szName);
}
/*
 * create the image list for the tab control
 * bInitial: TRUE  if list needs to be created (only at module initialisation)
 *           FALSE if we only need to clear and repopulate it (icons changed event)
 */

static void CreateImageList(BOOL bInitial)
{
    HICON hIcon;
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);
    
    if(bInitial) {
        myGlobals.g_hImageList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 2, 0);
        myGlobals.g_IconFolder = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_TREEVIEWEXPAND), IMAGE_ICON, 16, 16, LR_SHARED);
        myGlobals.g_IconUnchecked = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_TREEVIEWUNCHECKED), IMAGE_ICON, 16, 16, LR_SHARED);
        myGlobals.g_IconChecked = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_TREEVIEWCHECKED), IMAGE_ICON, 16, 16, LR_SHARED);
    }
    else
        ImageList_RemoveAll(myGlobals.g_hImageList);
    
    hIcon = CreateIcon(g_hInst, 16, 16, 1, 4, NULL, NULL);
    ImageList_AddIcon(myGlobals.g_hImageList, hIcon);
    ImageList_GetIcon(myGlobals.g_hImageList, 0, 0);
    DestroyIcon(hIcon);

    myGlobals.g_IconFileEvent = LoadSkinnedIcon(SKINICON_EVENT_FILE);
    myGlobals.g_IconUrlEvent = LoadSkinnedIcon(SKINICON_EVENT_URL);
    myGlobals.g_IconMsgEvent = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    myGlobals.g_IconSend = myGlobals.g_buttonBarIcons[9];
    myGlobals.g_IconTypingEvent = myGlobals.g_buttonBarIcons[5];
}


/*
 * initialises the internal API, services, events etc...
 */

static void InitAPI()
{
    /*
     * common services (SRMM style)
     */
    
    hSVC[H_MS_MSG_SENDMESSAGE] = CreateServiceFunction(MS_MSG_SENDMESSAGE, SendMessageCommand);
#if defined(_UNICODE)
    hSVC[H_MS_MSG_SENDMESSAGEW] = CreateServiceFunction(MS_MSG_SENDMESSAGE "W", SendMessageCommand_W);
#endif
    hSVC[H_MS_MSG_FORWARDMESSAGE] = CreateServiceFunction(MS_MSG_FORWARDMESSAGE, ForwardMessage);
    hSVC[H_MS_MSG_GETWINDOWAPI] =  CreateServiceFunction(MS_MSG_GETWINDOWAPI, GetWindowAPI);
    hSVC[H_MS_MSG_GETWINDOWCLASS] = CreateServiceFunction(MS_MSG_GETWINDOWCLASS, GetWindowClass);
    hSVC[H_MS_MSG_GETWINDOWDATA] = CreateServiceFunction(MS_MSG_GETWINDOWDATA, GetWindowData);
    hSVC[H_MS_MSG_READMESSAGE] =  CreateServiceFunction("SRMsg/ReadMessage", ReadMessageCommand);
    hSVC[H_MS_MSG_TYPINGMESSAGE] = CreateServiceFunction("SRMsg/TypingMessage", TypingMessageCommand);

    /*
     * tabSRMM - specific services
     */
    
    hSVC[H_MS_MSG_MOD_MESSAGEDIALOGOPENED] =  CreateServiceFunction(MS_MSG_MOD_MESSAGEDIALOGOPENED,MessageWindowOpened); 
    hSVC[H_MS_TABMSG_SETUSERPREFS] = CreateServiceFunction(MS_TABMSG_SETUSERPREFS, SetUserPrefs);
    hSVC[H_MS_TABMSG_TRAYSUPPORT] =  CreateServiceFunction(MS_TABMSG_TRAYSUPPORT, Service_OpenTrayMenu);
    hSVC[H_MSG_MOD_GETWINDOWFLAGS] =  CreateServiceFunction(MS_MSG_MOD_GETWINDOWFLAGS,GetMessageWindowFlags); 

    SI_InitStatusIcons();

    /*
     * the event API
     */

    g_hEvent_MsgWin = CreateHookableEvent(ME_MSG_WINDOWEVENT);
    g_hEvent_MsgPopup = CreateHookableEvent(ME_MSG_WINDOWPOPUP);
}

int TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type, unsigned int subType)
{
    MessageWindowEventData mwe = { 0 };
    struct TABSRMM_SessionInfo se = { 0 };
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
    BYTE bType = dat ? dat->bType : SESSIONTYPE_IM;
    
    if (hContact == NULL || hwnd == NULL) 
        return 0;

    if (!DBGetContactSettingByte(NULL, SRMSGMOD_T, "eventapi", 1))
        return 0;
    mwe.cbSize = sizeof(mwe);
    mwe.hContact = hContact;
    mwe.hwndWindow = hwnd;
#if defined(_UNICODE)
    mwe.szModule = "tabSRMsgW";
#else
    mwe.szModule = "tabSRMsg";
#endif    
    mwe.uType = type;
	mwe.hwndInput = GetDlgItem(hwnd, bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE);
	mwe.hwndLog = GetDlgItem(hwnd, bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG);

    if(type == MSG_WINDOW_EVT_CUSTOM) {
        se.cbSize = sizeof(se);
        se.evtCode = HIWORD(subType);
        se.hwnd = hwnd;
        se.extraFlags = (unsigned int)(LOWORD(subType));
        se.local = (void *)dat->sendBuffer;
        mwe.local = (void *)&se;
    }
    else
        mwe.local = NULL;
    return(NotifyEventHooks(g_hEvent_MsgWin, 0, (LPARAM)&mwe));
}

static ICONDESC _toolbaricons[] = {
    "tabSRMM_history", "Show History", &myGlobals.g_buttonBarIcons[1], -IDI_HISTORY, 1,
    "tabSRMM_mlog", "Message Log Options", &myGlobals.g_buttonBarIcons[2], -IDI_MSGLOGOPT, 1,
    "tabSRMM_add", "Add contact", &myGlobals.g_buttonBarIcons[0], -IDI_ADDCONTACT, 1,
    "tabSRMM_multi", "Multisend indicator", &myGlobals.g_buttonBarIcons[3], -IDI_MULTISEND, 1,
    "tabSRMM_typing", "Contact is typing", &myGlobals.g_buttonBarIcons[5], -IDI_TYPING, 1,
    "tabSRMM_quote", "Quote text", &myGlobals.g_buttonBarIcons[8], -IDI_QUOTE, 1,
    "tabSRMM_save", "Save and close", &myGlobals.g_buttonBarIcons[7], -IDI_SAVE, 1,
    "tabSRMM_send", "Send message", &myGlobals.g_buttonBarIcons[9], -IDI_SEND, 1,
    "tabSRMM_avatar", "Avatar menu", &myGlobals.g_buttonBarIcons[10], -IDI_CONTACTPIC, 1,
    "tabSRMM_close", "Close", &myGlobals.g_buttonBarIcons[6], -IDI_CLOSEMSGDLG, 1,
    "tabSRMM_usermenu", "User menu", &myGlobals.g_buttonBarIcons[4], -IDI_USERMENU, 1,
    NULL, NULL, NULL, 0, 0
};

static ICONDESC _exttoolbaricons[] = {
    "tabSRMM_emoticon", "Smiley button", &myGlobals.g_buttonBarIcons[11], -IDI_SMILEYICON, 1,
    "tabSRMM_bold", "Format bold", &myGlobals.g_buttonBarIcons[17], -IDI_FONTBOLD, 1,
    "tabSRMM_italic", "Format italic", &myGlobals.g_buttonBarIcons[18], -IDI_FONTITALIC, 1,
    "tabSRMM_underline", "Format underline", &myGlobals.g_buttonBarIcons[19], -IDI_FONTUNDERLINE, 1,
    "tabSRMM_face", "Font face", &myGlobals.g_buttonBarIcons[20], -IDI_FONTFACE, 1,
    "tabSRMM_color", "Font color", &myGlobals.g_buttonBarIcons[21], -IDI_FONTCOLOR, 1,
    NULL, NULL, NULL, 0, 0
};

static ICONDESC _logicons[] = {
    "tabSRMM_error", "Message delivery error", &myGlobals.g_iconErr, -IDI_MSGERROR, 1,
    "tabSRMM_in", "Incoming message", &myGlobals.g_iconIn, -IDI_ICONIN, 0,
    "tabSRMM_out", "Outgoing message", &myGlobals.g_iconOut, -IDI_ICONOUT, 0,
    "tabSRMM_status", "Statuschange", &myGlobals.g_iconStatus, -IDI_STATUSCHANGE, 0,
    NULL, NULL, NULL, 0, 0
};
static ICONDESC _deficons[] = {
    "tabSRMM_container", "Static container icon", &myGlobals.g_iconContainer, -IDI_CONTAINER, 1,
    "tabSRMM_mtn_on", "Sending typing notify is on", &myGlobals.g_buttonBarIcons[12], -IDI_SELFTYPING_ON, 1,
    "tabSRMM_mtn_off", "Sending typing notify is off", &myGlobals.g_buttonBarIcons[13], -IDI_SELFTYPING_OFF, 1,
    "tabSRMM_secureim_on", "RESERVED (currently not in use)", &myGlobals.g_buttonBarIcons[14], -IDI_SECUREIM_ENABLED, 1,
    "tabSRMM_secureim_off", "RESERVED (currently not in use)", &myGlobals.g_buttonBarIcons[15], -IDI_SECUREIM_DISABLED, 1,
    "tabSRMM_sounds_on", "Sounds are On", &myGlobals.g_buttonBarIcons[22], -IDI_SOUNDSON, 1,
    "tabSRMM_sounds_off", "Sounds are off", &myGlobals.g_buttonBarIcons[23], -IDI_SOUNDSOFF, 1,
    "tabSRMM_log_frozen", "Message Log frozen", &myGlobals.g_buttonBarIcons[24], -IDI_MSGERROR, 1,
    "tabSRMM_undefined", "Default", &myGlobals.g_buttonBarIcons[27], -IDI_EMPTY, 1,
    "tabSRMM_pulldown", "Pulldown Arrow", &myGlobals.g_buttonBarIcons[16], -IDI_PULLDOWNARROW, 1,
    "tabSRMM_Leftarrow", "Left Arrow", &myGlobals.g_buttonBarIcons[25], -IDI_LEFTARROW, 1,
    "tabSRMM_Rightarrow", "Right Arrow", &myGlobals.g_buttonBarIcons[28], -IDI_RIGHTARROW, 1,
    "tabSRMM_Pulluparrow", "Up Arrow", &myGlobals.g_buttonBarIcons[26], -IDI_PULLUPARROW, 1,
    "tabSRMM_sb_slist", "Session List", &myGlobals.g_sideBarIcons[0], -IDI_SESSIONLIST, 1,
    "tabSRMM_sb_Favorites", "Favorite Contacts", &myGlobals.g_sideBarIcons[1], -IDI_FAVLIST, 1,
    "tabSRMM_sb_Recent", "Recent Sessions", &myGlobals.g_sideBarIcons[2], -IDI_RECENTLIST, 1,
    "tabSRMM_sb_Setup", "Setup Sidebar", &myGlobals.g_sideBarIcons[3], -IDI_CONFIGSIDEBAR, 1,
    "tabSRMM_sb_Userprefs", "Contact Preferences", &myGlobals.g_sideBarIcons[4], -IDI_USERPREFS, 1,
    NULL, NULL, NULL, 0, 0
};

static ICONDESC _trayIcon[] = {
    "tabSRMM_frame1", "Frame 1", &myGlobals.m_AnimTrayIcons[0], -IDI_TRAYANIM1, 1,
    "tabSRMM_frame2", "Frame 2", &myGlobals.m_AnimTrayIcons[1], -IDI_TRAYANIM2, 1,
    "tabSRMM_frame3", "Frame 3", &myGlobals.m_AnimTrayIcons[2], -IDI_TRAYANIM3, 1,
    "tabSRMM_frame4", "Frame 4", &myGlobals.m_AnimTrayIcons[3], -IDI_TRAYANIM4, 1,
    NULL, NULL, NULL, 0, 0
};

static struct _iconblocks { char *szSection; ICONDESC *idesc; } ICONBLOCKS[] = {
    "TabSRMM/Default", _deficons,
    "TabSRMM/Toolbar", _toolbaricons,
    "TabSRMM/Toolbar", _exttoolbaricons,
    "TabSRMM/Message Log", _logicons,
    "TabSRMM/Animated Tray", _trayIcon,
    NULL, 0
};

static int GetIconPackVersion(HMODULE hDLL)
{
    char szIDString[256];
    int version = 0;
    
    if(LoadStringA(hDLL, IDS_IDENTIFY, szIDString, sizeof(szIDString)) == 0)
        version = 0;
    else {
        if(!strcmp(szIDString, "__tabSRMM_ICONPACK 1.0__"))
            version = 1;
        else if(!strcmp(szIDString, "__tabSRMM_ICONPACK 2.0__"))
            version = 2;
        else if(!strcmp(szIDString, "__tabSRMM_ICONPACK 3.0__"))
            version = 3;
    }
    if(version == 0)
        MessageBox(0, _T("The icon pack is either missing or too old."), _T("tabSRMM warning"), MB_OK | MB_ICONWARNING);
    else if(version > 0 && version < 3)
        MessageBox(0, _T("You are using an old icon pack (tabsrmm_icons.dll version < 3). This can cause missing icons, so please update the icon pack"), _T("tabSRMM warning"), MB_OK | MB_ICONWARNING);
    return version;
}
/*
 * setup default icons for the IcoLib service. This needs to be done every time the plugin is loaded
 * default icons are taken from the icon pack in either \icons or \plugins
 */

static int SetupIconLibConfig()
{
    SKINICONDESC sid;
    char szFilename[MAX_PATH];
    int i = 0, version = 0, n = 0;
    
    strncpy(szFilename, "plugins\\tabsrmm_icons.dll", MAX_PATH);
    g_hIconDLL = LoadLibraryA(szFilename);
    if(g_hIconDLL == 0) {
        strncpy(szFilename, "icons\\tabsrmm_icons.dll", MAX_PATH);
        g_hIconDLL = LoadLibraryA(szFilename);
        if(g_hIconDLL == 0) {
            MessageBoxA(0, "Critical: cannot init IcoLib, no resource DLL found.", "tabSRMM", MB_OK);
            return 0;
        }
    }
    GetModuleFileNameA(g_hIconDLL, szFilename, MAX_PATH);
    if(g_chat_integration_enabled)
        Chat_AddIcons();
    version = GetIconPackVersion(g_hIconDLL);
    myGlobals.g_hbmUnknown = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDB_UNKNOWNAVATAR), IMAGE_BITMAP, 0, 0, 0);
    if(myGlobals.g_hbmUnknown == 0) {
        HDC dc = GetDC(0);
        myGlobals.g_hbmUnknown = CreateCompatibleBitmap(dc, 20, 20);
        ReleaseDC(0, dc);
    }
    FreeLibrary(g_hIconDLL);
    g_hIconDLL = 0;
    
    sid.cbSize = sizeof(SKINICONDESC);
    sid.pszDefaultFile = szFilename;

    while(ICONBLOCKS[n].szSection) {
        i = 0;
        sid.pszSection = Translate(ICONBLOCKS[n].szSection);
        while(ICONBLOCKS[n].idesc[i].szDesc) {
            sid.pszName = ICONBLOCKS[n].idesc[i].szName;
            sid.pszDescription = Translate(ICONBLOCKS[n].idesc[i].szDesc);
            sid.iDefaultIndex = ICONBLOCKS[n].idesc[i].uId == -IDI_HISTORY ? 0 : ICONBLOCKS[n].idesc[i].uId;        // workaround problem /w icoLib and a resource id of 1 (actually, a Windows problem)
            CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
            i++;
        }
        n++;
    }
    return 1;
}

// load the icon theme from IconLib - check if it exists...

static int LoadFromIconLib()
{
    int i = 0, n = 0;

    while(ICONBLOCKS[n].szSection) {
        i = 0;
        while(ICONBLOCKS[n].idesc[i].szDesc) {
            *(ICONBLOCKS[n].idesc[i].phIcon) = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)ICONBLOCKS[n].idesc[i].szName);
            i++;
        }
        n++;
    }
    CacheMsgLogIcons();
    WindowList_Broadcast(hMessageWindowList, DM_LOADBUTTONBARICONS, 0, 0);
    return 0;
}

/*
 * load icon theme from either icon pack or IcoLib
 */

static void LoadIconTheme()
{
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);
    int i = 0, version = 0, n = 0;
    //char szDebug[80];
    
    if(SetupIconLibConfig() == 0)
        return;
    else
        LoadFromIconLib();
    return;
}

static void UnloadIcons()
{
    int i = 0, n = 0;

    while(ICONBLOCKS[n].szSection) {
        i = 0;
        while(ICONBLOCKS[n].idesc[i].szDesc) {
            if(*(ICONBLOCKS[n].idesc[i].phIcon) != 0) {
                DestroyIcon(*(ICONBLOCKS[n].idesc[i].phIcon));
                *(ICONBLOCKS[n].idesc[i].phIcon) = 0;
            }
            i++;
        }
        n++;
    }
    if(myGlobals.g_hbmUnknown)
        DeleteObject(myGlobals.g_hbmUnknown);
    for(i = 0; i < 4; i++) {
        if(myGlobals.m_AnimTrayIcons[i])
            DestroyIcon(myGlobals.m_AnimTrayIcons[i]);
    }
}

void ViewReleaseNotes()
{
    if(show_relnotes)
        return;

    show_relnotes = TRUE;
    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_VARIABLEHELP), 0, DlgProcTemplateHelp, (LPARAM)relnotes);
}

HICON *BTN_GetIcon(char *szIconName)
{
    int n = 0, i;
    while(ICONBLOCKS[n].szSection) {
        i = 0;
        while(ICONBLOCKS[n].idesc[i].szDesc) {
            if(!stricmp(ICONBLOCKS[n].idesc[i].szName, szIconName)) {
                //_DebugTraceA("found icon: %s", szIconName);
                return(ICONBLOCKS[n].idesc[i].phIcon);
            }
            i++;
        }
        n++;
    }
    for(i = 0; i < g_nrSkinIcons; i++) {
        if(!stricmp(g_skinIcons[i].szName, szIconName)) {
            //_DebugTraceA("found custom icon: %s", szIconName);
            return(g_skinIcons[i].phIcon);
        }
    }
    return NULL;
}


