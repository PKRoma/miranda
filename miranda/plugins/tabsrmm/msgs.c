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
#include "m_message.h"
#include "msgs.h"
#include "m_smileyadd.h"
#include "m_metacontacts.h"

#ifdef __MATHMOD_SUPPORT
//mathMod begin
#define QUESTIONMathExists "req_Is_MathModule_Installed"
#define ANSWERMathExists "automaticAnswer:MathModule_Is_Installed"
#define REPORTMathModuleInstalled_SERVICENAME "MATHMODULE_SEND_INSTALLED"
#define MTH_GETBITMAP "Math/GetBitmap"
#include <windows.h>
#include "../../SDK/headers_c/m_protomod.h"
#include "../../SDK/headers_c/m_protosvc.h"
//mathMod end
#endif

static void InitREOleCallback(void);

HCURSOR hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
HANDLE hMessageWindowList;
static HANDLE hEventDbEventAdded, hEventDbSettingChange, hEventContactDeleted;
HANDLE *hMsgMenuItem = NULL;
int hMsgMenuItemCount = 0;
HWND g_hwndHotkeyHandler;
HICON g_iconIn, g_iconOut, g_iconErr;
HBITMAP g_hbmUnknown = 0;
int g_isMetaContactsAvail = 0;

extern HINSTANCE g_hInst;
HMODULE hDLL;
PSLWA pSetLayeredWindowAttributes;
extern TCHAR g_szDefaultContainerName[];
extern HMENU g_hMenuContext;
extern struct ContainerWindowData *pFirstContainer;

struct ProtocolData *protoIconData = NULL;
int g_nrProtos = 0;
HIMAGELIST g_hImageList = 0;
int g_IconMsgEvent = 0, g_IconTypingEvent = 0, g_IconError = 0, g_IconEmpty = 0, g_IconFileEvent = 0, g_IconUrlEvent  = 0, g_IconSend = 0;

HANDLE g_hEvent_MsgWin;

int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer);
int GetProtoIconFromList(const char *szProto, int iStatus);
void CreateImageList(BOOL bInitial);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);

// the cached message log icons
void CacheMsgLogIcons();
void UncacheMsgLogIcons();
void CacheLogFonts();
void ConvertAllToUTF8();
void InitAPI();

extern struct MsgLogIcon msgLogIcons[NR_LOGICONS * 3];

#if defined(_STREAMTHREADING)
    // stream thread stuff...
    HANDLE g_hStreamThread = 0;
    int g_StreamThreadRunning = 0;
    extern CRITICAL_SECTION sjcs;
    extern DWORD WINAPI StreamThread(LPVOID param);
#endif

int _log(const char *fmt, ...);
struct ContainerWindowData *CreateContainer(const TCHAR *name, int iTemp, HANDLE hContactFrom);
int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd);
int CutContactName(char *contactName, char *newcontactname, int length);

struct ContainerWindowData *FindContainerByName(const TCHAR *name);
struct ContainerWindowData *FindMatchingContainer(const TCHAR *name, HANDLE hContact);

int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
HMENU BuildContainerMenu();
BOOL CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern HICON g_buttonBarIcons[];
int g_SmileyAddAvail = 0;

HMODULE g_hIconDLL = 0;
// nls stuff...

void BuildCodePageList();

/*
 * service function. retrieves the message window data for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
            * is directly used as the handle for the target window
 */
            
static int GetMessageWindowData(WPARAM wParam, LPARAM lParam)
{
    HWND hwndTarget = (HWND)lParam;

    if(hwndTarget == 0)
        hwndTarget = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
    
    if(hwndTarget)
        return GetWindowLong(hwndTarget, GWL_USERDATA);
    else
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
 * service function finds a message session 
 * wParam = contact handle for which we want the open window
 * thanks to bio for suggestions how to implement this
 * if wParam == 0, then lParam is considered to be a valid window handle and 
 * the function tests the popup mode of the target container

 * returns 1 if there is an open window or tab for the given hcontact (wParam)
 * 0 if there is none.
 */

int MessageWindowOpened(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = 0;
	struct ContainerWindowData *pContainer;

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

static int ReadMessageCommand(WPARAM wParam, LPARAM lParam)
{
    struct NewMessageWindowLParam newData = { 0 };
    HWND hwndExisting;
    HANDLE hContact = ((CLISTEVENT *) lParam)->hContact;
    struct ContainerWindowData *pContainer = 0;
    
	// int usingReadNext = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SPLIT, SRMSGDEFSET_SPLIT) ? 0 : 1;

    hwndExisting = WindowList_Find(hMessageWindowList, hContact);
    
    if (hwndExisting != NULL) {
        SendMessage(hwndExisting, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwndExisting);
        // if (usingReadNext)
           // SendMessage(hwndExisting, WM_COMMAND, MAKEWPARAM(IDC_READNEXT, BN_CLICKED), (LPARAM) GetDlgItem(hwndExisting, IDC_READNEXT));
    }
    else {
        TCHAR szName[CONTAINER_NAMELEN + 1];
        GetContainerNameForContact(hContact, szName, CONTAINER_NAMELEN);
        pContainer = FindContainerByName(szName);
        if (pContainer == NULL)
            pContainer = CreateContainer(szName, FALSE, hContact);
		CreateNewTabForContact (pContainer, hContact, 0, NULL, TRUE, TRUE);
    }
    return 0;
}

static int MessageEventAdded(WPARAM wParam, LPARAM lParam)
{
    CLISTEVENT cle;
    DBEVENTINFO dbei;
    char *contactName;
    char toolTip[256];
    HWND hwnd;
    BYTE bAutoPopup = FALSE, bAutoCreate = FALSE, bAutoContainer = FALSE;
    struct ContainerWindowData *pContainer = 0;
    TCHAR szName[CONTAINER_NAMELEN + 1];
    
    // int split = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SPLIT, SRMSGDEFSET_SPLIT);
    int split = 1;
    
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.cbBlob = 0;
    CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);

    if (dbei.flags & (DBEF_SENT | DBEF_READ) || dbei.eventType != EVENTTYPE_MESSAGE)
        return 0;
	
	CallServiceSync(MS_CLIST_REMOVEEVENT, wParam, (LPARAM) 1);
    /* does a window for the contact exist? */
    hwnd = WindowList_Find(hMessageWindowList, (HANDLE) wParam);
    if (hwnd) {
        struct ContainerWindowData *pTargetContainer = 0;
        int iPlay = 0;
        SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pTargetContainer);
        if (pTargetContainer) {
            DWORD dwFlags = pTargetContainer->dwFlags;
            if(dwFlags & CNT_NOSOUND)
                iPlay = FALSE;
            else if(dwFlags & CNT_SYNCSOUNDS) {
                iPlay = !MessageWindowOpened(0, (LPARAM)hwnd);
            }
            else
                iPlay = TRUE;
        }
        if (iPlay)
            SkinPlaySound("RecvMsg");
    
        // For single mode, the icon is needed or else closing the message window with a new
        // event means the user can't open the new event
        if (!split) {
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
        return 0;
    }
    /* new message */
    SkinPlaySound("AlertMsg");

    GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);

    bAutoPopup = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
    bAutoCreate = DBGetContactSettingByte(NULL, SRMSGMOD_T, "autotabs", 0);
    bAutoContainer = DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocontainer", 0);
    
    if (bAutoPopup || bAutoCreate) {
        BOOL bActivate = TRUE, bPopup = TRUE;
        struct NewMessageWindowLParam newData = { 0 };
        newData.hContact = (HANDLE) wParam;
        newData.isSend = 0;
        if(bAutoPopup) {
            bActivate = bPopup = TRUE;
            pContainer = FindContainerByName(szName);
            if (pContainer == NULL)
                pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
            CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup);
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
                        CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup);
                        return 0;
                    }
                    else if(bAutoContainer) {
                        pContainer = CreateContainer(szName, CNT_CREATEFLAG_MINIMIZED, (HANDLE)wParam);         // 2 means create minimized, don't popup...
                        CreateNewTabForContact(pContainer,  (HANDLE) wParam,  0, NULL, bActivate, bPopup);
                        SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
                        return 0;
                    }
                }
                else {
                    CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup);
                    return 0;
                }
                    
            }
            else {
                if(bAutoContainer) {
                    pContainer = CreateContainer(szName, CNT_CREATEFLAG_MINIMIZED, (HANDLE)wParam);         // 2 means create minimized, don't popup...
                    CreateNewTabForContact(pContainer,  (HANDLE) wParam,  0, NULL, bActivate, bPopup);
                    SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
                    return 0;
                }
            }
        }
    }
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
    return 0;
}

static int SendMessageCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
	char *szProto;
    struct NewMessageWindowLParam newData = { 0 };
    struct ContainerWindowData *pContainer = 0;
    
	// int isSplit = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SPLIT, SRMSGDEFSET_SPLIT);
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
            SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM) (char *) lParam);
        }
// XXX mod tab support (hide current, select the tab for this window...)
        SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwnd);
// XXX mod end
    }
    else {
        TCHAR szName[CONTAINER_NAMELEN + 1];
        GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
        pContainer = FindContainerByName(szName);
        if (pContainer == NULL)
            pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, !isSplit, (const char *) lParam, TRUE, TRUE);
    }
    return 0;
}

static int ForwardMessage(WPARAM wParam, LPARAM lParam)
{
	HWND hwndNew, hwndOld;
	RECT rc;
    struct ContainerWindowData *pContainer = 0;

    pContainer = FindContainerByName(_T("default"));
    if (pContainer == NULL)
        pContainer = CreateContainer(_T("default"), FALSE, 0);
    
	hwndOld = pContainer->hwndActive;
	hwndNew = CreateNewTabForContact(pContainer, 0, 0, (const char *) lParam, TRUE, TRUE);
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
        foundWin = 1;
    }
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
            cle.hIcon = LoadIcon(g_hIconDLL, MAKEINTRESOURCE(IDI_TYPING));
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
    char *szProto;
    HWND hwndTarget = 0;
    
    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
    if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
        return 0;
    
    hwndTarget = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
    if(hwndTarget)
        SendMessage(hwndTarget, DM_UPDATETITLE, 0, 0);
    
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
    // int usingReadNext = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SPLIT, SRMSGDEFSET_SPLIT) ? 0 : 1;
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

                if (autoPopup && !windowAlreadyExists) {
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
    HookEvent(ME_CLIST_DOUBLECLICKED, SendMessageCommand);
    RestoreUnreadMessageAlerts();
    CreateImageList(TRUE);              // XXX tab support, create shared image list for tab icons.
#if defined(_UNICODE)
    ConvertAllToUTF8();
#endif    
	hDLL = LoadLibraryA("user32");
	pSetLayeredWindowAttributes = (PSLWA) GetProcAddress(hDLL,"SetLayeredWindowAttributes");

#if defined(_UNICODE)
    if(!DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW", &dbv)) {
        if(dbv.type == DBVT_ASCIIZ) 
            _tcsncpy(g_szDefaultContainerName, Utf8Decode(dbv.pszVal), CONTAINER_NAMELEN);
#else
    if(!DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainername", &dbv)) {
        if(dbv.type == DBVT_ASCIIZ)
            strncpy(g_szDefaultContainerName, dbv.pszVal, CONTAINER_NAMELEN);
#endif        
        DBFreeVariant(&dbv);
    }
    else
        _tcsncpy(g_szDefaultContainerName, _T("Default"), CONTAINER_NAMELEN);

    g_hMenuContext = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TABCONTEXT));
    CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) g_hMenuContext, 0);   
    BuildContainerMenu();
    
#if defined(_UNICODE)
    #define SHORT_MODULENAME "tabSRMsgW (unicode)"
#else
    #define SHORT_MODULENAME "tabSRMsg"
#endif    
    DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SPLIT, 1);
    if(DBGetContactSetting(NULL, "KnownModules", SHORT_MODULENAME, &dbv))
        DBWriteContactSettingString(NULL, "KnownModules", SHORT_MODULENAME, "SRMsg,Tab_SRMsg,TAB_Containers,TAB_ContainersW");
    else
        DBFreeVariant(&dbv);

    g_hwndHotkeyHandler = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_HOTKEYSLAVE), 0, HotkeyHandlerDlgProc);

#if defined(_STREAMTHREADING)
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "streamthreading", 0)) {
        DWORD dwThreadId;
        InitializeCriticalSection(&sjcs);
        g_StreamThreadRunning = 1;
        g_hStreamThread = CreateThread(NULL, 0, StreamThread, NULL, 0, &dwThreadId);
    }
    else
        g_StreamThreadRunning = 0;
#endif

    if(ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) 
        g_SmileyAddAvail = 1;
    else
        g_SmileyAddAvail = 0;

    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", -1) == -1)
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 2);

    // nls stuff
    CacheLogFonts();
    BuildCodePageList();
    return 0;
}

int PreshutdownSendRecv(WPARAM wParam, LPARAM lParam)
{
    struct ContainerWindowData *pCurrent = 0;
    DestroyWindow(g_hwndHotkeyHandler);
    //WindowList_BroadcastAsync(hMessageSendList, WM_CLOSE, 0, 1);
    //WindowList_BroadcastAsync(hMessageWindowList, WM_CLOSE, 0, 1);
    pCurrent = pFirstContainer;
    while(pCurrent) {
        SendMessage(pCurrent->hwnd, WM_CLOSE, 0, 1);
        pCurrent = pCurrent->pNextContainer;
    }
    return 0;
}

int SplitmsgShutdown(void)
{
    int i;
    
    DestroyCursor(hCurSplitNS);
    DestroyCursor(hCurHyperlinkHand);
    DestroyCursor(hCurSplitWE);
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
    ImageList_RemoveAll(g_hImageList);
    ImageList_Destroy(g_hImageList);
    DestroyMenu(g_hMenuContext);
    UncacheMsgLogIcons();
#if defined(_STREAMTHREADING)
    if(g_StreamThreadRunning != 0) {
        g_StreamThreadRunning = 0;
        ResumeThread(g_hStreamThread);
        CloseHandle(g_hStreamThread);
        DeleteCriticalSection(&sjcs);
    }
#endif    
    DestroyIcon(g_iconIn);
    DestroyIcon(g_iconOut);
    DestroyIcon(g_iconErr);
    for (i = 0; i < NR_BUTTONBARICONS; i++)
        DestroyIcon(g_buttonBarIcons[i]);
    if(g_hbmUnknown)
        DeleteObject(g_hbmUnknown);
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
    //FreeMsgLogIcons();
    //LoadMsgLogIcons();
    WindowList_Broadcast(hMessageWindowList, DM_REMAKELOG, 0, 0);
    // change all the icons
    WindowList_Broadcast(hMessageWindowList, DM_UPDATEWINICON, 0, 0);

    CreateImageList(FALSE);
    UncacheMsgLogIcons();
    CacheMsgLogIcons();
    return 0;
}

#ifdef __MATHMOD_SUPPORT
//mathMod begin
// MathExistsRecvMsg()
// called by Miranda when a message was received
// checks if it was an plugin control code, signals an event if so,
// relays the message to the other protocol plugins if not so.
//
int doReportMathModuleInstalledRecvMsg( WPARAM wParam, LPARAM lParam )
{
	CCSDATA *pccsd = (CCSDATA *)lParam;
	PROTORECVEVENT *ppre = ( PROTORECVEVENT * )pccsd->lParam;

	if(strncmp(ppre->szMessage, QUESTIONMathExists, strlen(QUESTIONMathExists)))
		return CallService( MS_PROTO_CHAINRECV, wParam, lParam );
	if ( ServiceExists(MTH_GETBITMAP) && (CallContactService(pccsd->hContact, PS_GETSTATUS ,0,0) != ID_STATUS_INVISIBLE) )
		CallContactService(pccsd->hContact, PSS_MESSAGE, 0, (LPARAM)ANSWERMathExists);
	return 0;

}
//mathMod end
#endif

int LoadSendRecvMessageModule(void)
{
#ifdef __MATHMOD_SUPPORT
//mathMod begin
   	PROTOCOLDESCRIPTOR pd;
   	HANDLE hContact;
//mathMod end
#endif
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
    hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
    InitOptions();
    hEventDbEventAdded = HookEvent(ME_DB_EVENT_ADDED, MessageEventAdded);
    hEventDbSettingChange = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, MessageSettingChanged);
    hEventContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);
    HookEvent(ME_SYSTEM_MODULESLOADED, SplitmsgModulesLoaded);
    HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);
    HookEvent(ME_PROTO_CONTACTISTYPING, TypingMessage);
    HookEvent(ME_SYSTEM_PRESHUTDOWN, PreshutdownSendRecv);
    InitAPI();
    
#ifdef __MATHMOD_SUPPORT	
    //mathMod begin
     // register callback-function for every contact

    	memset( &pd, 0, sizeof( PROTOCOLDESCRIPTOR ));
    	pd.cbSize = sizeof( PROTOCOLDESCRIPTOR );
    	pd.szName = REPORTMathModuleInstalled_SERVICENAME;
    	pd.type = PROTOTYPE_ENCRYPTION - 9;
    	CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM ) &pd );
    	CreateServiceFunction(REPORTMathModuleInstalled_SERVICENAME PSR_MESSAGE, doReportMathModuleInstalledRecvMsg );
    	hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
    	while( hContact )
    	{
    		if( !CallService( MS_PROTO_ISPROTOONCONTACT, ( WPARAM )hContact, ( LPARAM )REPORTMathModuleInstalled_SERVICENAME ))
    			CallService( MS_PROTO_ADDTOCONTACT, ( WPARAM )hContact, ( LPARAM )REPORTMathModuleInstalled_SERVICENAME );
    		hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
    	}
	//mathMod end
#endif    
    
    SkinAddNewSound("RecvMsg", Translate("Message: Queued Incoming"), "message.wav");
    SkinAddNewSound("AlertMsg", Translate("Message: Incoming"), "messagealert.wav");
    SkinAddNewSound("SendMsg", Translate("Message: Outgoing"), "outgoing.wav");
    hCurSplitNS = LoadCursor(NULL, IDC_SIZENS);
    hCurSplitWE = LoadCursor(NULL, IDC_SIZEWE);
    hCurHyperlinkHand = LoadCursor(NULL, IDC_HAND);
    if (hCurHyperlinkHand == NULL)
        hCurHyperlinkHand = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_HYPERLINKHAND));
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
		TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetTabIndexFromHWND(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), hwndChild));
		SendMessage(pContainer->hwnd, WM_NOTIFY, 0, (LPARAM) &nmhdr);	// just select the tab and let WM_NOTIFY do the rest
		SetForegroundWindow(hwndChild);
		SetActiveWindow(hwndChild);
    	SendMessage(pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
        if(IsIconic(pContainer->hwnd))
            SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		return TRUE;
	} else
		return FALSE;
}

/*
 * this function creates and activates a new tab within the given container.
 * bActivateTab: make the new tab the active one
 * bPopupContainer: restore container if it was minimized, otherwise flash it...
 */

HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTab, BOOL bPopupContainer)
{
	char *contactName, *szProto, *szStatus, tabtitle[128], newcontactname[128];
	WORD wStatus;
    int	newItem;
    HWND hwndNew;
    struct NewMessageWindowLParam newData = {0};
    
    // if we have a max # of tabs/container set and want to open something in the default container...
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0) &&  !_tcsncmp(pContainer->szName, _T("default"), 6)) {
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
    SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
    hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSPLITNEW), GetDlgItem(pContainer->hwnd, IDC_MSGTABS), DlgProcMessage, (LPARAM) &newData);
    // if the container is minimized, then pop it up...
    if(IsIconic(pContainer->hwnd)) {
        if(bPopupContainer) {
            SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            SetFocus(pContainer->hwndActive);
        }
        else {
            if(pContainer->isFlashing)
                pContainer->nFlash = 0;
            else {
                SetTimer(pContainer->hwnd, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                pContainer->isFlashing = TRUE;
            }
        }
    }
    if (bActivateTab) {
        SetFocus(hwndNew);
        RedrawWindow(pContainer->hwnd, NULL, NULL, RDW_INVALIDATE);
        UpdateWindow(pContainer->hwnd);
        PostMessage(hwndNew, DM_SCROLLLOGTOBOTTOM, 0, 0);
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
        if(pDesired)
            return pDesired;
        else {
            return NULL;
        }
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

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &g_nrProtos, (LPARAM) &pProtos);

    if(bInitial) {
        if (g_nrProtos) {
            protoIconData = (struct ProtocolData *) malloc(sizeof(struct ProtocolData) * g_nrProtos);
        } else {
            MessageBoxA(0, "Warning: LoadIcons - no protocols found", "Warning", MB_OK);
        }
    }

    if(bInitial) {
        int cxIcon = GetSystemMetrics(SM_CXSMICON);
        int cyIcon = GetSystemMetrics(SM_CYSMICON);
        
        //g_hIconDLL = LoadLibraryExA("plugins/tabsrmm_icons.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
        g_hIconDLL = LoadLibraryA("plugins/tabsrmm_icons.dll");
        if(g_hIconDLL == NULL)
            MessageBoxA(0, "Critical: cannot load resource DLL (no icons will be shown)", "b", MB_OK);
        else {
            g_hbmUnknown = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDB_UNKNOWNAVATAR), IMAGE_BITMAP, 0, 0, 0);
            
            g_iconIn = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_ICONIN), IMAGE_ICON, 0, 0, 0);
            g_iconOut = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_ICONOUT), IMAGE_ICON, 0, 0, 0);
            g_iconErr = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_MSGERROR), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_hImageList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, (g_nrProtos + 1) * 12 + 8, 0);
            CacheMsgLogIcons();

            // load button bar Icons

            g_buttonBarIcons[0] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_ADDCONTACT), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[1] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_HISTORY), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[2] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_TIMESTAMP), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[3] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_MULTISEND), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[4] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_USERMENU), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[5] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_TYPING), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[6] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_CLOSEMSGDLG), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[7] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_SAVE), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[8] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_QUOTE), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[9] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[10] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_CONTACTPIC), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[11] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_SMILEYICON), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[12] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_SELFTYPING_ON), IMAGE_ICON, cxIcon, cyIcon, 0);
            g_buttonBarIcons[13] = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_SELFTYPING_OFF), IMAGE_ICON, cxIcon, cyIcon, 0);
        }
    }
    else
        ImageList_RemoveAll(g_hImageList);

    // load global status icons...
    for (i = ID_STATUS_OFFLINE; i <= ID_STATUS_OUTTOLUNCH; i++) {
        hIcon = LoadSkinnedProtoIcon(0, i);
        ImageList_AddIcon(g_hImageList, hIcon);
    }
    iCurIcon = i - ID_STATUS_OFFLINE;

    /*
     * build a list of protocols and their associated icon ids
     * load icons into the image list and remember the base indices for
     * each protocol.
     */

    for(i = 0; i < g_nrProtos; i++) {
        if (pProtos[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        strncpy(protoIconData[i].szName, pProtos[i]->szName, sizeof(protoIconData[i].szName));
        protoIconData[i].iFirstIconID = iCurIcon;
        for (j = ID_STATUS_OFFLINE; j <= ID_STATUS_OUTTOLUNCH; j++) {
            hIcon = LoadSkinnedProtoIcon(protoIconData[i].szName, j);
            if (hIcon != 0) {
                ImageList_AddIcon(g_hImageList, hIcon);
                iCurIcon++;
            }
        }
    }

    // message event icon
    hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    ImageList_AddIcon(g_hImageList, hIcon);
    g_IconMsgEvent = iCurIcon++;

    // typing notify icon
    hIcon = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_TYPING), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);    
    ImageList_AddIcon(g_hImageList, hIcon);
    DestroyIcon(hIcon);
    g_IconTypingEvent = iCurIcon++;

    hIcon = LoadSkinnedIcon(SKINICON_EVENT_FILE);
    ImageList_AddIcon(g_hImageList, hIcon);
    g_IconFileEvent = iCurIcon++;

    hIcon = LoadSkinnedIcon(SKINICON_EVENT_URL);
    ImageList_AddIcon(g_hImageList, hIcon);
    g_IconUrlEvent = iCurIcon++;

    hIcon = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_MSGERROR), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    ImageList_AddIcon(g_hImageList, hIcon);
    DestroyIcon(hIcon);
    g_IconError = iCurIcon++;

    hIcon = (HICON) LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    ImageList_AddIcon(g_hImageList, hIcon);
    DestroyIcon(hIcon);
    g_IconSend = iCurIcon++;
    
    ImageList_AddIcon(g_hImageList, 0);             // empty (end of list)
    g_IconEmpty = iCurIcon;

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
            utf8string = Utf8Encode((WCHAR *)dbv.pbVal);
            DBWriteContactSettingString(NULL, "TAB_ContainersW", szCounter, utf8string);
        }
        DBFreeVariant(&dbv);
        counter++;
    } while ( TRUE );

    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

    while(hContact) {
        if(!DBGetContactSetting(hContact, SRMSGMOD_T, "containerW", &dbv)) {
            if(dbv.type == DBVT_BLOB && dbv.cpbVal > 0) {
                utf8string = Utf8Encode((WCHAR *)dbv.pbVal);
                DBDeleteContactSetting(hContact, SRMSGMOD_T, "containerW");
                DBWriteContactSettingString(hContact, SRMSGMOD_T, "containerW", utf8string);
            }
            DBFreeVariant(&dbv);
        }
        hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
    }
    if(!DBGetContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW", &dbv)) {
        if(dbv.type == DBVT_BLOB) {
            DBDeleteContactSetting(NULL, SRMSGMOD_T, "defaultcontainernameW");
            DBWriteContactSettingString(NULL, SRMSGMOD_T, "defaultcontainernameW", Utf8Encode((WCHAR *)dbv.pbVal));
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
    HANDLE hEvent;
    /*
     * common services (SRMM style)
     */
    
    CreateServiceFunction(MS_MSG_SENDMESSAGE, SendMessageCommand);
#if defined(_UNICODE)
    CreateServiceFunction(MS_MSG_SENDMESSAGE "W", SendMessageCommand);
#endif
    CreateServiceFunction(MS_MSG_FORWARDMESSAGE, ForwardMessage);
    CreateServiceFunction("SRMsg/ReadMessage", ReadMessageCommand);
    CreateServiceFunction("SRMsg/TypingMessage", TypingMessageCommand);

    /*
     * tabSRMM - specific services
     */
    
    CreateServiceFunction(MS_MSG_MOD_MESSAGEDIALOGOPENED,MessageWindowOpened); 
    CreateServiceFunction(MS_MSG_MOD_GETWINDOWDATA,GetMessageWindowData); 
    CreateServiceFunction(MS_MSG_MOD_GETWINDOWFLAGS,GetMessageWindowFlags); 

    /*
     * the event API
     */

    g_hEvent_MsgWin = CreateHookableEvent(ME_MSG_WINDOWEVENT);

    if(ServiceExists(MS_MC_GETDEFAULTCONTACT))
        g_isMetaContactsAvail = TRUE;
}

void TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type) {
    MessageWindowEventData mwe = { 0 };

    if (hContact == NULL || hwnd == NULL) return;
    if (!DBGetContactSettingByte(NULL, SRMSGMOD_T, "eventapi", 0))
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
    NotifyEventHooks(g_hEvent_MsgWin, 0, (LPARAM)&mwe);
}

