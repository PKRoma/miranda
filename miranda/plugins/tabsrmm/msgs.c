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
#include "msgs.h"
#include "msgdlgutils.h"
#include "m_popup.h"
#include "nen.h"
#include "m_smileyadd.h"
#include "m_ieview.h"
#include "m_metacontacts.h"
#include "IcoLib.h"
#include "functions.h"

#ifdef __MATHMOD_SUPPORT
//mathMod begin
#define QUESTIONMathExists "req_Is_MathModule_Installed"
#define ANSWERMathExists "automaticAnswer:MathModule_Is_Installed"
#define REPORTMathModuleInstalled_SERVICENAME "MATHMODULE_SEND_INSTALLED"
#define MTH_GETBITMAP "Math/GetBitmap"
#include <windows.h>
#include "../../include/m_protomod.h"
#include "../../include/m_protosvc.h"
//mathMod end
#endif

MYGLOBALS myGlobals;
NEN_OPTIONS nen_options;

static void InitREOleCallback(void);
static int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam);

HANDLE hMessageWindowList, hUserPrefsWindowList;
static HANDLE hEventDbEventAdded, hEventDbSettingChange, hEventContactDeleted, hEventDispatch;
HANDLE *hMsgMenuItem = NULL;
int hMsgMenuItemCount = 0;


extern HINSTANCE g_hInst;
HMODULE hDLL;
PSLWA pSetLayeredWindowAttributes;

extern struct ContainerWindowData *pFirstContainer;
extern BOOL CALLBACK DlgProcUserPrefs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// send jobs...

extern struct SendJob sendJobs[NR_SENDJOBS];

struct ProtocolData *protoIconData = NULL;

HANDLE g_hEvent_MsgWin;
#if defined(_UNICODE)
HHOOK g_hMsgHook = 0;
#endif

HICON LoadOneIcon(int iconId, char *szName);

extern struct MsgLogIcon msgLogIcons[NR_LOGICONS * 3];

int _log(const char *fmt, ...);
struct ContainerWindowData *CreateContainer(const TCHAR *name, int iTemp, HANDLE hContactFrom);
int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd);
int CutContactName(char *contactName, char *newcontactname, int length);

struct ContainerWindowData *FindContainerByName(const TCHAR *name);
struct ContainerWindowData *FindMatchingContainer(const TCHAR *name, HANDLE hContact);

int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
HMENU BuildContainerMenu();
BOOL CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

HMODULE g_hIconDLL = 0;
// nls stuff...

void BuildCodePageList();
int tabSRMM_ShowPopup(WPARAM wParam, LPARAM lParam, WORD eventType, int windowOpen, struct ContainerWindowData *pContainer, HWND hwndChild, char *szProto, struct MessageWindowData *dat);

/*
 * installed as a WH_GETMESSAGE hook in order to process unicode messages.
 * without this, the rich edit control does NOT accept input for all languages.
 */

#if defined(_UNICODE) && defined(WANT_UGLY_HOOK)
LRESULT CALLBACK GetMsgHookProc(int iCode, WPARAM wParam, LPARAM lParam)
{
    MSG *msg = (MSG *)lParam;
    if(iCode < 0)
        return CallNextHookEx(g_hMsgHook, iCode, wParam, lParam);

    if(wParam == PM_REMOVE) {
        if(IsWindowUnicode(msg->hwnd)) {
            if(msg->message >= WM_KEYFIRST && msg->message <= WM_KEYLAST) {
                TranslateMessage(msg);
                DispatchMessageW(msg);
                msg->message = WM_NULL;
            }
        }
    }
    return CallNextHookEx(g_hMsgHook, iCode, wParam, lParam);
}
#endif

static int ContactSecureChanged(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam))
	{
        SendMessage(hwnd, DM_SECURE_CHANGED, 0, 0);
    }
    return 0;
}

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

	if( mwid == NULL || mwod == NULL) 
        return 1;
	if(mwid->cbSize != sizeof(MessageWindowInputData) || mwod->cbSize != sizeof(MessageWindowOutputData)) 
        return 1;
	if(mwid->hContact == NULL) 
        return 1;
	if(mwid->uFlags != MSG_WINDOW_UFLAG_MSG_BOTH) 
        return 1;
	hwnd = WindowList_Find(hMessageWindowList, mwid->hContact);
	mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
	mwod->hwndWindow = hwnd;
	mwod->local = 0;
	mwod->uState = SendMessage(hwnd, DM_GETWINDOWSTATE, 0, 0);
	return 0;
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
                if((GetForegroundWindow() == pContainer->hwnd || GetActiveWindow() == pContainer->hwnd) && pContainer->hwndActive == hwnd)
                    return 1;
                else {
                    if(pContainer->hwndActive == hwnd && pContainer->dwFlags & CNT_STICKY)
                        return 1;
                    else
                        return 0;
                }
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
    PROTO_AVATAR_INFORMATION *pai;
    HWND hwndDlg = 0;
    int i, j, iFound = NR_SENDJOBS;

    if(lParam == 0)
        return 0;
    
    pai = (PROTO_AVATAR_INFORMATION *) pAck->hProcess;

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
    if(pAck->type != ACKTYPE_AVATAR)
        return 0;

    hwndDlg = WindowList_Find(hMessageWindowList, (HANDLE)pAck->hContact);
    if(hwndDlg) {
        struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
        if(pAck->hContact == dat->hContact && pAck->type == ACKTYPE_AVATAR && pAck->result == ACKRESULT_STATUS)
            PostMessage(hwndDlg, DM_RETRIEVEAVATAR, 0, 0);
        if(pai == NULL) {
            _DebugPopup(dat->hContact, "pai == 0 in avatar ACK handler");
            return 0;
        }
        if(pAck->hContact == dat->hContact && pAck->type == ACKTYPE_AVATAR && pAck->result == ACKRESULT_SUCCESS) {
            if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0)) {
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic", pai->filename);
                DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File",pai->filename);
                ShowPicture(hwndDlg, dat, FALSE, TRUE, TRUE);
            }
        }
        if(pAck->hContact == dat->hContact && pAck->type == ACKTYPE_AVATAR && pAck->result == ACKRESULT_FAILED) {
            if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0)) {
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic", "");
                DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File", "");
                ShowPicture(hwndDlg, dat, FALSE, TRUE, TRUE);
            }
        }
        return 0;
    }
    return 0;
}

static int DispatchNewEvent(WPARAM wParam, LPARAM lParam) {
	if (wParam) {
        HWND h = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
		if(h)
            SendMessage(h, HM_DBEVENTADDED, wParam, lParam);
	}
	return 0;
}

static int ReadMessageCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwndExisting;
    HANDLE hContact = ((CLISTEVENT *) lParam)->hContact;
    struct ContainerWindowData *pContainer = 0;
    
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
    return 0;
}

static int MessageEventAdded(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
    CLISTEVENT cle;
    DBEVENTINFO dbei;
    char *contactName;
    char toolTip[256];
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
        if(dbei.eventType != EVENTTYPE_MESSAGE && dbei.eventType != EVENTTYPE_STATUSCHANGE && hwnd == NULL && !(dbei.flags & DBEF_SENT))
            tabSRMM_ShowPopup(wParam, lParam, dbei.eventType, 0, 0, 0, dbei.szModule, 0);
        return 0;
    }
    
	CallServiceSync(MS_CLIST_REMOVEEVENT, wParam, (LPARAM) 1);

    if (hwnd) {
        struct ContainerWindowData *pTargetContainer = 0;
        if(dbei.eventType == EVENTTYPE_MESSAGE) {
            SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pTargetContainer);
            if (pTargetContainer) {
                PlayIncomingSound(pTargetContainer, hwnd);
            }
        }
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

    bAutoPopup = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
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
            if((pContainer = FindContainerByName(szName)) == NULL);
                pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
            CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, FALSE, 0);
            return 0;
        }
        else {
            bActivate = FALSE;
            bPopup = (BOOL) DBGetContactSettingByte(NULL, SRMSGMOD_T, "cpopup", 0);
            pContainer = FindContainerByName(szName);
            if (pContainer != NULL) {
                if(IsIconic(pContainer->hwnd) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", 0)) {
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
    if(nen_options.bTraySupport)
        UpdateTrayMenu(0, 0, dbei.szModule, NULL, (HANDLE)wParam, TRUE);
    else {
        ZeroMemory(&cle, sizeof(cle));
        cle.cbSize = sizeof(cle);
        cle.hContact = (HANDLE) wParam;
        cle.hDbEvent = (HANDLE) lParam;
        cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        cle.pszService = "SRMsg/ReadMessage";
        contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, wParam, 0);
        _snprintf(toolTip, sizeof(toolTip), Translate("Message from %s"), contactName);
        cle.pszTooltip = toolTip;
        CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
    }
    tabSRMM_ShowPopup(wParam, lParam, dbei.eventType, 0, 0, 0, dbei.szModule, 0);
    return 0;
}

static int SendMessageCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
	char *szProto;
    struct NewMessageWindowLParam newData = { 0 };
    struct ContainerWindowData *pContainer = 0;
    
    int isSplit = 1;
    
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
        GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
        pContainer = FindContainerByName(szName);
        if (pContainer == NULL)
            pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, !isSplit, (const char *) lParam, TRUE, TRUE, FALSE, 0);
    }
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

    if(hwnd == 0) {      // we are not interested in this event if there is no open message window/tab
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
        if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
            return 0;                       // filter out settings we aren't interested in...
        if(DBGetContactSettingWord((HANDLE)wParam, SRMSGMOD_T, "isFavorite", 0))
            AddContactToFavorites((HANDLE)wParam, NULL, szProto, NULL, 0, 0, 0, myGlobals.g_hMenuFavorites);
        if(DBGetContactSettingWord((HANDLE)wParam, SRMSGMOD_T, "isRecent", 0))
            AddContactToFavorites((HANDLE)wParam, NULL, szProto, NULL, 0, 0, 0, myGlobals.g_hMenuRecent);
        return 0;       // for the hContact.
    }

    if(!strncmp(cws->szModule, "ContactPhoto", 12)) {
        SendMessage(hwnd, DM_PICTURECHANGED, 0, 0);
        return 0;
    }

    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
    if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
        return 0;
    
    // metacontacts support

    if(!lstrcmpA(cws->szModule, "MetaContacts") && !lstrcmpA(cws->szSetting, "Nick"))       // filter out this setting to avoid infinite loops while trying to obtain the most online contact
        return 0;
    
    if(hwnd)
        SendMessage(hwnd, DM_UPDATETITLE, 0, 0);
    
    return 0;
}

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if (hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam)) {
        SendMessage(hwnd, WM_CLOSE, 0, 0);
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
                    newData.isSend = 0;
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
    
    ReloadGlobals();
    NEN_ReadOptions(&nen_options);

    //LoadMsgLogIcons();
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
    HookEvent(ME_CLIST_DOUBLECLICKED, SendMessageCommand);
    RestoreUnreadMessageAlerts();
    for(i = 0; i < NR_BUTTONBARICONS; i++)
        myGlobals.g_buttonBarIcons[i] = 0;
    LoadIconTheme();
    CreateImageList(TRUE);              // XXX tab support, create shared image list for tab icons.
#if defined(_UNICODE)
    ConvertAllToUTF8();
#endif    
	hDLL = LoadLibraryA("user32");
	pSetLayeredWindowAttributes = (PSLWA) GetProcAddress(hDLL,"SetLayeredWindowAttributes");
#if defined(_UNICODE)
    if(!DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW", &dbv)) {
        if(dbv.type == DBVT_ASCIIZ) {
            WCHAR *wszTemp = Utf8_Decode(dbv.pszVal);
            _tcsncpy(myGlobals.g_szDefaultContainerName, wszTemp, CONTAINER_NAMELEN);
            free(wszTemp);
        }
#else
    if(!DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainername", &dbv)) {
        if(dbv.type == DBVT_ASCIIZ)
            strncpy(myGlobals.g_szDefaultContainerName, dbv.pszVal, CONTAINER_NAMELEN);
#endif        
        DBFreeVariant(&dbv);
    }
    else
        _tcsncpy(myGlobals.g_szDefaultContainerName, _T("Default"), CONTAINER_NAMELEN);

    myGlobals.g_hMenuContext = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TABCONTEXT));
    CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) myGlobals.g_hMenuContext, 0);   
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

    myGlobals.g_hwndHotkeyHandler = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_HOTKEYSLAVE), 0, HotkeyHandlerDlgProc);

    if(ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) 
        myGlobals.g_SmileyAddAvail = 1;

    myGlobals.g_wantSnapping = ServiceExists("Utils/SnapWindowProc") && DBGetContactSettingByte(NULL, SRMSGMOD_T, "usesnapping", 0);
    
    if(ServiceExists(MS_MC_GETDEFAULTCONTACT))
        myGlobals.g_MetaContactsAvail = 1;

    if(ServiceExists("SecureIM/IsContactSecured"))
        myGlobals.g_SecureIMAvail = 1;

#if defined(_UNICODE) && defined(WANT_UGLY_HOOK)
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "kbdhook", 0))
        g_hMsgHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgHookProc, 0, GetCurrentThreadId());
#endif    
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", -1) == -1)
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 2);

    // nls stuff
    CacheLogFonts();
    BuildCodePageList();
    ZeroMemory((void *)sendJobs, sizeof(struct SendJob) * NR_SENDJOBS);
    if(nen_options.bTraySupport)
        CreateSystrayIcon(TRUE);
    LoadDefaultTemplates();
    
    ZeroMemory((void *)&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = -500050005;
    mi.hIcon = myGlobals.g_iconContainer;
    mi.pszContactOwner = NULL;
    mi.pszName = Translate( "&tabSRMM settings" );
    mi.pszService = MS_TABMSG_SETUSERPREFS;
    myGlobals.m_UserMenuItem = ( HANDLE )CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );
    PreTranslateDates();
    return 0;
}

int PreshutdownSendRecv(WPARAM wParam, LPARAM lParam)
{
    DestroyWindow(myGlobals.g_hwndHotkeyHandler);
    while(pFirstContainer)
        SendMessage(pFirstContainer->hwnd, WM_CLOSE, 0, 1);
    return 0;
}

int SplitmsgShutdown(void)
{
    int i;

#if defined(_UNICODE) && defined(WANT_UGLY_HOOK)
    if(g_hMsgHook != 0)
        UnhookWindowsHookEx(g_hMsgHook);
#endif    
    DestroyCursor(myGlobals.hCurSplitNS);
    DestroyCursor(myGlobals.hCurHyperlinkHand);
    DestroyCursor(myGlobals.hCurSplitWE);
    UnhookEvent(hEventDbEventAdded);
    UnhookEvent(hEventDbSettingChange);
    UnhookEvent(hEventContactDeleted);
    //FreeMsgLogIcons();
    FreeLibrary(GetModuleHandleA("riched20"));
	FreeLibrary(GetModuleHandleA("user32"));
    if(g_hIconDLL)
        FreeLibrary(g_hIconDLL);
    
    OleUninitialize();
    if (hMsgMenuItem) {
        free(hMsgMenuItem);
        hMsgMenuItem = NULL;
        hMsgMenuItemCount = 0;
    }
    ImageList_RemoveAll(myGlobals.g_hImageList);
    ImageList_Destroy(myGlobals.g_hImageList);
    DestroyMenu(myGlobals.g_hMenuContext);
    if(myGlobals.g_hMenuContainer)
        DestroyMenu(myGlobals.g_hMenuContainer);
    if(myGlobals.g_hMenuEncoding)
        DestroyMenu(myGlobals.g_hMenuEncoding);
    UncacheMsgLogIcons();
    DestroyIcon(myGlobals.g_iconIn);
    DestroyIcon(myGlobals.g_iconOut);
    DestroyIcon(myGlobals.g_iconErr);
    DestroyIcon(myGlobals.g_iconContainer);
    DestroyIcon(myGlobals.g_iconStatus);
    
    for (i = 0; i < NR_BUTTONBARICONS; i++)
        DestroyIcon(myGlobals.g_buttonBarIcons[i]);
    if(myGlobals.g_hbmUnknown)
        DeleteObject(myGlobals.g_hbmUnknown);
    if(myGlobals.m_hbmMsgArea)
        DeleteObject(myGlobals.m_hbmMsgArea);
    if(protoIconData != 0)
        free(protoIconData);
    CreateSystrayIcon(FALSE);
    /*
     * not really needed, but avoid memory leak warnings
     */
    return 0;
}

static int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
    UncacheMsgLogIcons();
    LoadFromIconLib();
    CacheMsgLogIcons();
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
    UncacheMsgLogIcons();
    CreateImageList(FALSE);
    CacheMsgLogIcons();
    WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
    WindowList_Broadcast(hMessageWindowList, DM_UPDATEWINICON, 0, 0);
    return 0;
}

int LoadSendRecvMessageModule(void)
{
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
    
    InitOptions();
    hEventDispatch = HookEvent(ME_DB_EVENT_ADDED, DispatchNewEvent);
    hEventDbEventAdded = HookEvent(ME_DB_EVENT_ADDED, MessageEventAdded);
    hEventDbSettingChange = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, MessageSettingChanged);
    hEventContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);
    HookEvent(ME_SYSTEM_MODULESLOADED, SplitmsgModulesLoaded);
    HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);
    HookEvent(ME_PROTO_CONTACTISTYPING, TypingMessage);
    HookEvent(ME_PROTO_ACK, ProtoAck);
    HookEvent(ME_SYSTEM_PRESHUTDOWN, PreshutdownSendRecv);

	HookEvent("SecureIM/Established",ContactSecureChanged);
	HookEvent("SecureIM/Disabled",ContactSecureChanged);
    
    InitAPI();
    
    SkinAddNewSoundEx("RecvMsgActive", Translate("Messages"), Translate("Incoming (Focused Window)"));
    SkinAddNewSoundEx("RecvMsgInactive", Translate("Messages"), Translate("Incoming (Unfocused Window)"));
    SkinAddNewSoundEx("AlertMsg", Translate("Messages"), Translate("Incoming (New Session)"));
    SkinAddNewSoundEx("SendMsg", Translate("Messages"), Translate("Outgoing"));
    myGlobals.hCurSplitNS = LoadCursor(NULL, IDC_SIZENS);
    myGlobals.hCurSplitWE = LoadCursor(NULL, IDC_SIZEWE);
    myGlobals.hCurHyperlinkHand = LoadCursor(NULL, IDC_HAND);
    if (myGlobals.hCurHyperlinkHand == NULL)
        myGlobals.hCurHyperlinkHand = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_HYPERLINKHAND));

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
	struct MessageWindowData *dat;
	NMHDR nmhdr;

	// if the container is minimized, then pop it up...
	// hide the active message dialog
	dat = (struct MessageWindowData *) GetWindowLong(hwndChild, GWL_USERDATA);	// needed to obtain the hContact for the message window
	if(dat) {
        ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
		nmhdr.code = TCN_SELCHANGE;
        if(TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS)) > 1) {
            TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetTabIndexFromHWND(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), hwndChild));
            SendMessage(pContainer->hwnd, WM_NOTIFY, 0, (LPARAM) &nmhdr);	// just select the tab and let WM_NOTIFY do the rest
        }
        if(IsIconic(pContainer->hwnd))
            SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        SetFocus(hwndChild);
    	SendMessage(pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
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
	char *contactName, *szProto, *szStatus, tabtitle[128], newcontactname[128];
	WORD wStatus;
    int	newItem;
    HWND hwndNew;
    struct NewMessageWindowLParam newData = {0};

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
    newData.isSend = isSend;
    newData.szInitialText = pszInitialText;

    ZeroMemory((void *)&newData.item, sizeof(newData.item));

	// obtain various status information about the contact
	contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) newData.hContact, 0);

	/*
	 * cut nickname if larger than x chars...
	 */

    if(contactName && strlen(contactName) > 0) {
        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0))
            CutContactName(contactName, newcontactname, sizeof(newcontactname));
        else
            strncpy(newcontactname, contactName, sizeof(newcontactname));
    }
    else
        strncpy(newcontactname, "_U_", sizeof(newcontactname));

	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) newData.hContact, 0);
	wStatus = szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) newData.hContact, szProto, "Status", ID_STATUS_OFFLINE);
	szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE)newData.hContact, szProto, "Status", ID_STATUS_OFFLINE), 0);
    
	if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabstatus", 0))
		_snprintf(tabtitle, sizeof(tabtitle), "%s (%s)", newcontactname, szStatus);
	else
		_snprintf(tabtitle, sizeof(tabtitle), "%s", newcontactname);

#ifdef _UNICODE
	{
    wchar_t w_tabtitle[256];
    if(MultiByteToWideChar(CP_ACP, 0, tabtitle, -1, w_tabtitle, sizeof(w_tabtitle)) != 0)
        newData.item.pszText = w_tabtitle;
	}
#else
	newData.item.pszText = tabtitle;
#endif
    newData.item.iImage = GetProtoIconFromList(szProto, wStatus);

	newData.item.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;

	// hide the active tab
	if(pContainer->hwndActive && bActivateTab) {
		ShowWindow(pContainer->hwndActive, SW_HIDE);
	}
	newItem = TabCtrl_InsertItem(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), pContainer->iTabIndex++, &newData.item);
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

void CreateImageList(BOOL bInitial)
{
    PROTOCOLDESCRIPTOR **pProtos;
    int i, j;
    HICON hIcon;
    int iCurIcon = 0;
    
    // enumerate available protocols... full protocol icon support 

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &myGlobals.g_nrProtos, (LPARAM) &pProtos);

    if(bInitial) {
        if (myGlobals.g_nrProtos) {
            protoIconData = (struct ProtocolData *) malloc(sizeof(struct ProtocolData) * myGlobals.g_nrProtos);
        } else {
            MessageBoxA(0, "Warning: LoadIcons - no protocols found", "Warning", MB_OK);
        }
    }

    if(bInitial)
        myGlobals.g_hImageList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, (myGlobals.g_nrProtos + 1) * 12 + 8, 0);
    else
        ImageList_RemoveAll(myGlobals.g_hImageList);

    // load global status icons...
    for (i = ID_STATUS_OFFLINE; i <= ID_STATUS_OUTTOLUNCH; i++) {
        hIcon = LoadSkinnedProtoIcon(0, i);
        ImageList_AddIcon(myGlobals.g_hImageList, hIcon);
    }
    iCurIcon = i - ID_STATUS_OFFLINE;

    /*
     * build a list of protocols and their associated icon ids
     * load icons into the image list and remember the base indices for
     * each protocol.
     */

    for(i = 0; i < myGlobals.g_nrProtos; i++) {
        if (pProtos[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        strncpy(protoIconData[i].szName, pProtos[i]->szName, sizeof(protoIconData[i].szName));
        protoIconData[i].iFirstIconID = iCurIcon;
        for (j = ID_STATUS_OFFLINE; j <= ID_STATUS_OUTTOLUNCH; j++) {
            hIcon = LoadSkinnedProtoIcon(protoIconData[i].szName, j);
            if (hIcon != 0) {
                ImageList_AddIcon(myGlobals.g_hImageList, hIcon);
                iCurIcon++;
            }
        }
    }

    // message event icon
    hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    ImageList_AddIcon(myGlobals.g_hImageList, hIcon);
    myGlobals.g_IconMsgEvent = iCurIcon++;

    // typing notify icon
    ImageList_AddIcon(myGlobals.g_hImageList, myGlobals.g_buttonBarIcons[5]);
    myGlobals.g_IconTypingEvent = iCurIcon++;

    hIcon = LoadSkinnedIcon(SKINICON_EVENT_FILE);
    ImageList_AddIcon(myGlobals.g_hImageList, hIcon);
    myGlobals.g_IconFileEvent = iCurIcon++;

    hIcon = LoadSkinnedIcon(SKINICON_EVENT_URL);
    ImageList_AddIcon(myGlobals.g_hImageList, hIcon);
    myGlobals.g_IconUrlEvent = iCurIcon++;

    ImageList_AddIcon(myGlobals.g_hImageList, myGlobals.g_iconErr);
    myGlobals.g_IconError = iCurIcon++;

    ImageList_AddIcon(myGlobals.g_hImageList, myGlobals.g_buttonBarIcons[9]);
    myGlobals.g_IconSend = iCurIcon++;
    
    ImageList_AddIcon(myGlobals.g_hImageList, 0);             // empty (end of list)
    myGlobals.g_IconEmpty = iCurIcon;

}

#if defined(_UNICODE)

void ConvertAllToUTF8()
{
    DBVARIANT dbv;
    char szCounter[10];
    int counter = 0;
    char *utf8string;
    HANDLE hContact;
    
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "utf8converted", 0))
        return;

    do {
        _snprintf(szCounter, 8, "%d", counter);
        if(DBGetContactSetting(NULL, "TAB_ContainersW", szCounter, &dbv))
            break;
        if(dbv.type == DBVT_BLOB && dbv.cpbVal > 0) {
            utf8string = Utf8_Encode((WCHAR *)dbv.pbVal);
            DBWriteContactSettingString(NULL, "TAB_ContainersW", szCounter, utf8string);
            free(utf8string);
        }
        DBFreeVariant(&dbv);
        counter++;
    } while ( TRUE );

    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

    while(hContact) {
        if(!DBGetContactSetting(hContact, SRMSGMOD_T, "containerW", &dbv)) {
            if(dbv.type == DBVT_BLOB && dbv.cpbVal > 0) {
                utf8string = Utf8_Encode((WCHAR *)dbv.pbVal);
                DBDeleteContactSetting(hContact, SRMSGMOD_T, "containerW");
                DBWriteContactSettingString(hContact, SRMSGMOD_T, "containerW", utf8string);
                free(utf8string);
            }
            DBFreeVariant(&dbv);
        }
        hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
    }
    if(!DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW", &dbv)) {
        if(dbv.type == DBVT_BLOB) {
            char *utf8string = Utf8_Encode((WCHAR *)dbv.pbVal);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW");
            DBWriteContactSettingString(NULL, SRMSGMOD_T, "defaultcontainernameW", utf8string);
            free(utf8string);
        }
        DBFreeVariant(&dbv);
    }
    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "utf8converted", 1);
}
#endif

/*
 * initialises the internal API, services, events etc...
 */

void InitAPI()
{
    /*
     * common services (SRMM style)
     */
    
    CreateServiceFunction(MS_MSG_SENDMESSAGE, SendMessageCommand);
#if defined(_UNICODE)
    CreateServiceFunction(MS_MSG_SENDMESSAGE "W", SendMessageCommand);
#endif
    CreateServiceFunction(MS_MSG_FORWARDMESSAGE, ForwardMessage);
    CreateServiceFunction(MS_MSG_GETWINDOWAPI, GetWindowAPI);
    CreateServiceFunction(MS_MSG_GETWINDOWCLASS, GetWindowClass);
    CreateServiceFunction(MS_MSG_GETWINDOWDATA, GetWindowData);
    CreateServiceFunction("SRMsg/ReadMessage", ReadMessageCommand);
    CreateServiceFunction("SRMsg/TypingMessage", TypingMessageCommand);

    /*
     * tabSRMM - specific services
     */
    
    CreateServiceFunction(MS_MSG_MOD_MESSAGEDIALOGOPENED,MessageWindowOpened); 
    CreateServiceFunction(MS_TABMSG_SETUSERPREFS, SetUserPrefs);
    CreateServiceFunction(MS_MSG_MOD_GETWINDOWFLAGS,GetMessageWindowFlags); 

    /*
     * the event API
     */

    g_hEvent_MsgWin = CreateHookableEvent(ME_MSG_WINDOWEVENT);
}

void TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type, unsigned int subType) {
    MessageWindowEventData mwe = { 0 };
    struct TABSRMM_SessionInfo se = { 0 };
    
    if (hContact == NULL || hwnd == NULL) return;
    if (!DBGetContactSettingByte(NULL, SRMSGMOD_T, "eventapi", 1))
        return;
    mwe.cbSize = sizeof(mwe);
    mwe.hContact = hContact;
    mwe.hwndWindow = hwnd;
#if defined(_UNICODE)
    mwe.szModule = "tabSRMsgW";
#else
    mwe.szModule = "tabSRMsg";
#endif    
    mwe.uType = type;
    if(type = MSG_WINDOW_EVT_CUSTOM) {
        se.cbSize = sizeof(se);
        se.evtCode = subType;
        se.dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
        se.hwnd = hwnd;
        se.hwndInput = GetDlgItem(hwnd, IDC_MESSAGE);
        if(se.dat != NULL) {
            se.hwndContainer = se.dat->pContainer->hwnd;
            se.pContainer = se.dat->pContainer;
        }
        mwe.local = (void *)&se;
    }
    else
        mwe.local = NULL;
    NotifyEventHooks(g_hEvent_MsgWin, 0, (LPARAM)&mwe);
}

void LoadMsgAreaBackground()
{
    char szFilename[MAX_PATH];
    DBVARIANT dbv;
    HBITMAP hbmNew;
    
    if(DBGetContactSetting(NULL, SRMSGMOD_T, "bgimage", &dbv) == 0) {
        CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
        hbmNew = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM) szFilename);
        if(myGlobals.m_hbmMsgArea != 0)
            DeleteObject(myGlobals.m_hbmMsgArea);
        myGlobals.m_hbmMsgArea = hbmNew;
        DBFreeVariant(&dbv);
    }
    else {
        if(myGlobals.m_hbmMsgArea != 0)
            DeleteObject(myGlobals.m_hbmMsgArea);
        myGlobals.m_hbmMsgArea = 0;
    }
        
}
static ICONDESC myIcons[] = {
    "tabSRMM_history", "Show History", &myGlobals.g_buttonBarIcons[1], -IDI_HISTORY, 1,
    "tabSRMM_mlog", "Message Log Options", &myGlobals.g_buttonBarIcons[2], -IDI_TIMESTAMP, 1,
    "tabSRMM_add", "Add contact", &myGlobals.g_buttonBarIcons[0], -IDI_ADDCONTACT, 1,
    "tabSRMM_multi", "Multisend indicator", &myGlobals.g_buttonBarIcons[3], -IDI_MULTISEND, 1,
    "tabSRMM_typing", "Contact is typing", &myGlobals.g_buttonBarIcons[5], -IDI_TYPING, 1,
    "tabSRMM_quote", "Quote text", &myGlobals.g_buttonBarIcons[8], -IDI_QUOTE, 1,
    "tabSRMM_save", "Save and close", &myGlobals.g_buttonBarIcons[7], -IDI_SAVE, 1,
    "tabSRMM_send", "Send message", &myGlobals.g_buttonBarIcons[9], -IDI_CHECK, 1,
    "tabSRMM_avatar", "Avatar menu", &myGlobals.g_buttonBarIcons[10], -IDI_CONTACTPIC, 1,
    "tabSRMM_close", "Close", &myGlobals.g_buttonBarIcons[6], -IDI_CLOSEMSGDLG, 1,
    "tabSRMM_usermenu", "User menu", &myGlobals.g_buttonBarIcons[4], -IDI_USERMENU, 1,
    "tabSRMM_error", "Message delivery error", &myGlobals.g_iconErr, -IDI_MSGERROR, 1,
    "tabSRMM_in", "Incoming message", &myGlobals.g_iconIn, -IDI_ICONIN, 0,
    "tabSRMM_out", "Outgoing message", &myGlobals.g_iconOut, -IDI_ICONOUT, 0,
    "tabSRMM_emoticon", "Smiley button", &myGlobals.g_buttonBarIcons[11], -IDI_SMILEYICON, 1,
    "tabSRMM_mtn_on", "Sending typing notify is on", &myGlobals.g_buttonBarIcons[12], -IDI_SELFTYPING_ON, 1,
    "tabSRMM_mtn_off", "Sending typing notify is off", &myGlobals.g_buttonBarIcons[13], -IDI_SELFTYPING_OFF, 1,
    "tabSRMM_container", "Static container icon", &myGlobals.g_iconContainer, -IDI_CONTAINER, 1,
    "tabSRMM_secureim_on", "SecureIM is on", &myGlobals.g_buttonBarIcons[14], -IDI_SECUREIM_ENABLED, 1,
    "tabSRMM_secureim_off", "SecureIM is off", &myGlobals.g_buttonBarIcons[15], -IDI_SECUREIM_DISABLED, 1,
    "tabSRMM_status", "Statuschange", &myGlobals.g_iconStatus, -IDI_STATUSCHANGE, 0,
    "tabSRMM_bold", "Format bold", &myGlobals.g_buttonBarIcons[17], -IDI_FONTBOLD, 1,
    "tabSRMM_italic", "Format italic", &myGlobals.g_buttonBarIcons[18], -IDI_FONTITALIC, 1,
    "tabSRMM_underline", "Format underline", &myGlobals.g_buttonBarIcons[19], -IDI_FONTUNDERLINE, 1,
    "tabSRMM_face", "Font face", &myGlobals.g_buttonBarIcons[20], -IDI_FONTFACE, 1,
    "tabSRMM_color", "Font color", &myGlobals.g_buttonBarIcons[21], -IDI_FONTCOLOR, 1,
    "tabSRMM_sounds_on", "Sounds are On", &myGlobals.g_buttonBarIcons[22], -IDI_SOUNDSON, 1,
    "tabSRMM_sounds_off", "Sounds are off", &myGlobals.g_buttonBarIcons[23], -IDI_SOUNDSOFF, 1,
    "tabSRMM_log_frozen", "Message Log frozen", &myGlobals.g_buttonBarIcons[24], -IDI_MSGERROR, 1,
    NULL, NULL, NULL, 0
};

/*
 * setup default icons for the IcoLib service. This needs to be done every time the plugin is loaded
 * default icons are taken from the icon pack in either \icons or \plugins
 */

int SetupIconLibConfig()
{
    SKINICONDESC sid;
    char szFilename[MAX_PATH];
    int i = 0;
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
    myGlobals.g_hbmUnknown = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDB_UNKNOWNAVATAR), IMAGE_BITMAP, 0, 0, 0);
    LoadMsgAreaBackground();
    FreeLibrary(g_hIconDLL);
    g_hIconDLL = 0;
    
    sid.cbSize = sizeof(SKINICONDESC);
    sid.pszSection = "TabSRMM";
    sid.pszDefaultFile = szFilename;

    i = 0;
    do {
        if(myIcons[i].szName == NULL)
            break;
        sid.pszName = myIcons[i].szName;
        sid.pszDescription = Translate(myIcons[i].szDesc);
        sid.iDefaultIndex = myIcons[i].uId == -IDI_HISTORY ? 0 : myIcons[i].uId;        // workaround problem /w icoLib and a resource id of 1 (actually, a Windows problem)
        CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
    } while(++i);

    GetModuleFileNameA(g_hInst, szFilename, MAX_PATH);
    sid.pszName = (char *) "tabSRMM_pulldown";
    sid.iDefaultIndex = -IDI_PULLDOWNARROW;
    sid.pszDescription = Translate("Pulldown Button");
    CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
    return 1;
}

// load the icon theme from IconLib - check if it exists...

int LoadFromIconLib()
{
    int i = 0;

    UncacheMsgLogIcons();
    do {
        if(myIcons[i].szName == NULL)
            break;
        *(myIcons[i].phIcon) = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) myIcons[i].szName);
    } while(++i);

    myGlobals.g_buttonBarIcons[16] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"tabSRMM_pulldown");
    
    ImageList_ReplaceIcon(myGlobals.g_hImageList, myGlobals.g_IconError, myGlobals.g_iconErr);
    ImageList_ReplaceIcon(myGlobals.g_hImageList, myGlobals.g_IconSend, myGlobals.g_buttonBarIcons[9]);
    ImageList_ReplaceIcon(myGlobals.g_hImageList, myGlobals.g_IconTypingEvent, myGlobals.g_buttonBarIcons[5]);
    CacheMsgLogIcons();
    WindowList_Broadcast(hMessageWindowList, DM_LOADBUTTONBARICONS, 0, 0);
    return 0;
}

/*
 * load icon theme from either icon pack or IcoLib
 */

void LoadIconTheme()
{
    char szFilename[MAX_PATH];
    char szIDString[256];
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);
    int i = 0;
    //char szDebug[80];
    
    if(ServiceExists(MS_SKIN2_ADDICON)) {               // ico lib present...
        if(SetupIconLibConfig() == 0)
            return;
        else
            LoadFromIconLib();
        return;
    }

    // no icolib present, load it the default way...
    
    if(g_hIconDLL == 0) {                               // first time, load the library...
        strncpy(szFilename, "plugins\\tabsrmm_icons.dll", MAX_PATH);
        g_hIconDLL = LoadLibraryA(szFilename);
        if(g_hIconDLL == 0) {
            strncpy(szFilename, "icons\\tabsrmm_icons.dll", MAX_PATH);
            g_hIconDLL = LoadLibraryA(szFilename);
        }
    }

    if(g_hIconDLL == NULL)
        MessageBoxA(0, "Critical: cannot load resource DLL (no icons will be shown)", "tabSRMM", MB_OK);
    else {
        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "v_check", 1)) {
            if(LoadStringA(g_hIconDLL, IDS_IDENTIFY, szIDString, sizeof(szIDString)) == 0) {
                if(MessageBoxA(0, "ICONPACK: unknown version, load anyway?", "tabSRMM", MB_YESNO) == IDNO)
                    goto failure;
            }
            else {
                if(strcmp(szIDString, "__tabSRMM_ICONPACK 1.0__")) {
                    if(MessageBoxA(0, "ICONPACK: unknown version, load anyway?", "tabSRMM", MB_YESNO) == IDNO)
                        goto failure;
                }
            }
        }
        UncacheMsgLogIcons();

        myGlobals.g_hbmUnknown = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDB_UNKNOWNAVATAR), IMAGE_BITMAP, 0, 0, 0);
        LoadMsgAreaBackground();
        i = 0;
        do {
            if(myIcons[i].szName == NULL)
                break;
            *(myIcons[i].phIcon) = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(abs(myIcons[i].uId)), IMAGE_ICON, myIcons[i].bForceSmall ? cxIcon : 0, myIcons[i].bForceSmall ? cyIcon : 0, 0);
        } while(++i);
        myGlobals.g_buttonBarIcons[16] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PULLDOWNARROW), IMAGE_ICON, cxIcon, cyIcon, 0);

        CacheMsgLogIcons();

        WindowList_Broadcast(hMessageWindowList, DM_LOADBUTTONBARICONS, 0, 0);
        return;
    }
failure:
    if(g_hIconDLL) {
        FreeLibrary(g_hIconDLL);
        g_hIconDLL = 0;    
    }
}

