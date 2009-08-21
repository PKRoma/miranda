/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * Load, setup and shutdown the plugin
 * core plugin messaging services (single IM chats only).
 *
 */

#include "commonheaders.h"
#pragma hdrstop
#include "sendqueue.h"

/*
static SKINDESC my_default_skin[] = {
	IDR_SKIN_GLYPH, _T("glyph.png"),
	IDR_SKIN_TSK, _T("default.tsk"),
	IDR_SKIN_TABSRMM, _T("default.tabsrmm"),
	IDR_SKIN_ICO_CLOSE, _T("close.ico"),
	IDR_SKIN_ICO_MAX, _T("maximize.ico"),
	IDR_SKIN_ICO_MIN, _T("minimize.ico")
};
*/

REOLECallback *mREOLECallback;

NEN_OPTIONS nen_options;
extern PLUGININFOEX pluginInfo;
//mad
extern	BOOL newapi, g_bClientInStatusBar;
extern	HANDLE hHookToolBarLoadedEvt;
//

static void UnloadIcons();
static int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam);
static int AvatarChanged(WPARAM wParam, LPARAM lParam);
static int MyAvatarChanged(WPARAM wParam, LPARAM lParam);

HANDLE hUserPrefsWindowList, hEventDbEventAdded;
static HANDLE hEventDbSettingChange, hEventContactDeleted, hEventDispatch, hEventPrebuildMenu;
static HANDLE hModulesLoadedEvent;
static HANDLE hEventSmileyAdd = 0, hMenuItem;

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

extern      INT_PTR CALLBACK DlgProcUserPrefsFrame(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      struct MsgLogIcon msgLogIcons[NR_LOGICONS * 3];
extern      INT_PTR CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      struct RTFColorTable *rtf_ctable;
extern		const TCHAR *DoubleAmpersands(TCHAR *pszText);
extern      void RegisterFontServiceFonts();
extern      int FontServiceFontsChanged(WPARAM wParam, LPARAM lParam);
extern		int ModPlus_PreShutdown(WPARAM wparam, LPARAM lparam);
extern		int ModPlus_Init(WPARAM wparam, LPARAM lparam);
extern		HBITMAP IMG_LoadLogo(const char *szName);
extern 		int CacheIconToBMP(struct MsgLogIcon *theIcon, HICON hIcon, COLORREF backgroundColor, int sizeX, int sizeY);
extern		void DeleteCachedIcon(struct MsgLogIcon *theIcon);

HANDLE g_hEvent_MsgWin;
HANDLE g_hEvent_MsgPopup;

HMODULE g_hIconDLL = 0;

int     Chat_IconsChanged(WPARAM wp, LPARAM lp), Chat_ModulesLoaded(WPARAM wp, LPARAM lp);
void    Chat_AddIcons(void);
int     Chat_PreShutdown(WPARAM wParam, LPARAM lParam);

/*
 * fired event when user changes IEView plugin options. Apply them to all open tabs
 */

static int IEViewOptionsChanged(WPARAM wParam, LPARAM lParam)
{
	M->BroadcastMessage(DM_IEVIEWOPTIONSCHANGED, 0, 0);
	return 0;
}

/*
 * fired event when user changes smileyadd options. Notify all open tabs about the changes
 */

static int SmileyAddOptionsChanged(WPARAM wParam, LPARAM lParam)
{
	M->BroadcastMessage(DM_SMILEYOPTIONSCHANGED, 0, 0);
	if (PluginConfig.m_chat_enabled)
		SM_BroadcastMessage(NULL, DM_SMILEYOPTIONSCHANGED, 0, 0, FALSE);
	return 0;
}

/*
 * Message API 0.0.0.3 services
 */

static INT_PTR GetWindowClass(WPARAM wParam, LPARAM lParam)

{
	char *szBuf = (char*)wParam;
	int size = (int)lParam;
	mir_snprintf(szBuf, size, "tabSRMM");
	return 0;
}

/*
 * service function. retrieves the message window data for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
 * is directly used as the handle for the target window
 */

static INT_PTR GetWindowData(WPARAM wParam, LPARAM lParam)
{
	MessageWindowInputData *mwid = (MessageWindowInputData*)wParam;
	MessageWindowOutputData *mwod = (MessageWindowOutputData*)lParam;
	HWND hwnd;
	SESSION_INFO *si = NULL;

	if (mwid == NULL || mwod == NULL)
		return 1;
	if (mwid->cbSize != sizeof(MessageWindowInputData) || mwod->cbSize != sizeof(MessageWindowOutputData))
		return 1;
	if (mwid->hContact == NULL)
		return 1;
	if (mwid->uFlags != MSG_WINDOW_UFLAG_MSG_BOTH)
		return 1;
	hwnd = M->FindWindow(mwid->hContact);
	if (hwnd) {
		mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
		mwod->hwndWindow = hwnd;
		mwod->local = GetParent(GetParent(hwnd));
		SendMessage(hwnd, DM_GETWINDOWSTATE, 0, 0);
		mwod->uState = (int)GetWindowLongPtr(hwnd, DWLP_MSGRESULT);
		return 0;
	}
	else if ((si = SM_FindSessionByHCONTACT(mwid->hContact)) != NULL && si->hWnd != 0) {
		mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
		mwod->hwndWindow = si->hWnd;
		mwod->local = GetParent(GetParent(si->hWnd));
		SendMessage(si->hWnd, DM_GETWINDOWSTATE, 0, 0);
		mwod->uState = (int)GetWindowLongPtr(si->hWnd, DWLP_MSGRESULT);
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
 * service function. Invoke the user preferences dialog for the contact given (by handle) in wParam
 */

static INT_PTR SetUserPrefs(WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = WindowList_Find(hUserPrefsWindowList, (HANDLE)wParam);
	if (hWnd) {
		SetForegroundWindow(hWnd);			// already open, bring it to front
		return 0;
	}
	CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS_FRAME), 0, DlgProcUserPrefsFrame, (LPARAM)wParam);
	return 0;
}

/*
 * service function - open the tray menu from the TTB button
 */

static INT_PTR Service_OpenTrayMenu(WPARAM wParam, LPARAM lParam)
{
	SendMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, lParam == 0 ? WM_LBUTTONUP : WM_RBUTTONUP);
	return 0;
}

/*
 * service function. retrieves the message window flags for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
 * is directly used as the handle for the target window
 */

static INT_PTR GetMessageWindowFlags(WPARAM wParam, LPARAM lParam)
{
	HWND hwndTarget = (HWND)lParam;

	if (hwndTarget == 0)
		hwndTarget = M->FindWindow((HANDLE)wParam);

	if (hwndTarget) {
		struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwndTarget, GWLP_USERDATA);
		if (dat)
			return (dat->dwFlags);
		else
			return 0;
	} else
		return 0;
}

/*
 * return the version of the window api supported
 */

static INT_PTR GetWindowAPI(WPARAM wParam, LPARAM lParam)
{
	return PLUGIN_MAKE_VERSION(0, 0, 0, 2);
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

INT_PTR MessageWindowOpened(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = 0;
	struct ContainerWindowData *pContainer = NULL;

	if (wParam)
		hwnd = M->FindWindow((HANDLE)wParam);
	else if (lParam)
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
				if (pContainer->hwndActive == hwnd)
					return 1;
				else
					return 0;
			}
		}
		return 1;
	} else
		return 0;
}

/*
 * this is the global ack dispatcher. It handles both ACKTYPE_MESSAGE and ACKTYPE_AVATAR events
 * for ACKTYPE_MESSAGE it searches the corresponding send job in the queue and, if found, dispatches
 * it to the owners window
 *
 * ACKTYPE_AVATAR no longer handled here, because we have avs services now.
 */

static int ProtoAck(WPARAM wParam, LPARAM lParam)
{
	ACKDATA *pAck = (ACKDATA *) lParam;
	HWND hwndDlg = 0;
	int i, j, iFound = SendQueue::NR_SENDJOBS;

	if (lParam == 0)
		return 0;

	SendJob *jobs = sendQueue->getJobByIndex(0);

	if (pAck->type == ACKTYPE_MESSAGE) {
		for (j = 0; j < SendQueue::NR_SENDJOBS; j++) {
			for (i = 0; i < jobs[j].sendCount; i++) {
				if (pAck->hProcess == jobs[j].hSendId[i] && pAck->hContact == jobs[j].hContact[i]) {
					_MessageWindowData *dat = jobs[j].hwndOwner ? (_MessageWindowData *)GetWindowLongPtr(jobs[j].hwndOwner, GWLP_USERDATA) : NULL;
					if (dat) {
						if (dat->hContact == jobs[j].hOwner) {
							iFound = j;
							break;
						}
					} else {      // ack message w/o an open window...
						sendQueue->ackMessage(NULL, (WPARAM)MAKELONG(j, i), lParam);
						return 0;
					}
				}
			}
			if (iFound == SendQueue::NR_SENDJOBS)          // no mathing entry found in this queue entry.. continue
				continue;
			else
				break;
		}
		if (iFound == SendQueue::NR_SENDJOBS)              // no matching send info found in the queue
			return 0;
		else {                                  // the job was found
			SendMessage(jobs[iFound].hwndOwner, HM_EVENTSENT, (WPARAM)MAKELONG(iFound, i), lParam);
			return 0;
		}
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

static int DispatchNewEvent(WPARAM wParam, LPARAM lParam)
{
	if (wParam) {
		HWND h = M->FindWindow((HANDLE)wParam);
		if (h)
			PostMessage(h, HM_DBEVENTADDED, wParam, lParam);            // was SENDMESSAGE !!! XXX
	}
	return 0;
}

/*
 * ReadMessageCommand is executed whenever the user wants to manually open a window.
 * This can happen when double clicking a contact on the clist OR when opening a new
 * message (clicking on a popup, clicking the flashing tray icon and so on).
 */

static INT_PTR ReadMessageCommand(WPARAM wParam, LPARAM lParam)
{
	HWND hwndExisting;
	HANDLE hContact = ((CLISTEVENT *) lParam)->hContact;
	struct ContainerWindowData *pContainer = 0;

	hwndExisting = M->FindWindow(hContact);

	if (hwndExisting != 0) {
		SendMessage(hwndExisting, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		if (pContainer != NULL)
			ActivateExistingTab(pContainer, hwndExisting);
	} else {
		TCHAR szName[CONTAINER_NAMELEN + 1];
		GetContainerNameForContact(hContact, szName, CONTAINER_NAMELEN);
		pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, hContact);
		CreateNewTabForContact(pContainer, hContact, 0, NULL, TRUE, TRUE, FALSE, 0);
	}
	return 0;
}

/*
 * Message event added is called when a new message is added to the database
 * if no session is open for the contact, this function will determine if and how a new message
 * session (tab) must be created.
 *
 * if a session is already created, it just does nothing and DispatchNewEvent() will take care.
 */

static int MessageEventAdded(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	CLISTEVENT cle;
	DBEVENTINFO dbei;
	UINT mesCount=0;
	BYTE bAutoPopup = FALSE, bAutoCreate = FALSE, bAutoContainer = FALSE, bAllowAutoCreate = 0;
	struct ContainerWindowData *pContainer = 0;
	TCHAR szName[CONTAINER_NAMELEN + 1];
	DWORD dwStatusMask = 0;
	struct _MessageWindowData *mwdat=NULL;

	ZeroMemory(&dbei, sizeof(dbei));
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);

	hwnd = M->FindWindow((HANDLE) wParam);
	//mad:mod for actual history
	if (hwnd) {
		mwdat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (mwdat && mwdat->bActualHistory)
			mwdat->messageCount++;
	//mad_
	}
	if (dbei.flags & DBEF_SENT || dbei.eventType != EVENTTYPE_MESSAGE || dbei.flags & DBEF_READ)
		return 0;

	CallServiceSync(MS_CLIST_REMOVEEVENT, wParam, (LPARAM) 1);
		//MaD: hide on close mod, simulating standard behavior for hidden container
	if (hwnd) {
		struct ContainerWindowData *pTargetContainer = 0;
		WINDOWPLACEMENT wp={0};
		wp.length = sizeof(wp);
		SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pTargetContainer);

		if(pTargetContainer && PluginConfig.m_HideOnClose && !IsWindowVisible(pTargetContainer->hwnd))	{
			GetWindowPlacement(pTargetContainer->hwnd, &wp);
			GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);

			bAutoPopup = M->GetByte(SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
			bAutoCreate = M->GetByte("autotabs", 1);
			bAutoContainer = M->GetByte("autocontainer", 1);
			dwStatusMask = M->GetDword("autopopupmask", -1);

			bAllowAutoCreate = FALSE;

			if (bAutoPopup || bAutoCreate) {
				BOOL bActivate = TRUE, bPopup = TRUE;
				if(bAutoPopup) {
					if(wp.showCmd == SW_SHOWMAXIMIZED)
						ShowWindow(pTargetContainer->hwnd, SW_SHOWMAXIMIZED);
					else
						ShowWindow(pTargetContainer->hwnd, SW_SHOWNOACTIVATE);
					return 0;
				}
				else {
					bActivate = FALSE;
					bPopup = (BOOL) M->GetByte("cpopup", 0);
					pContainer = FindContainerByName(szName);
					if (pContainer != NULL) {
						if(bAutoContainer) {
							ShowWindow(pTargetContainer->hwnd, SW_SHOWMINNOACTIVE);
							return 0;
						}
						else goto nowindowcreate;
					}
					else {
						if(bAutoContainer) {
							ShowWindow(pTargetContainer->hwnd, SW_SHOWMINNOACTIVE);
							return 0;
						}
					}
				}
			}
		}
		else
			return 0;
	}			/*  do not process it here, when a session window is open for this contact.
        						DispatchNewEvent() will handle this */

	/*
	 * if no window is open, we are not interested in anything else but unread message events
	 */

	/* new message */
	if (!nen_options.iNoSounds)
		SkinPlaySound("AlertMsg");

	if (nen_options.iNoAutoPopup)
		goto nowindowcreate;

	//MAD
	if (M->GetByte((HANDLE) wParam, "ActualHistory", 0)) {
		mesCount=(UINT)M->GetDword((HANDLE) wParam, "messagecount", 0);
		M->WriteDword((HANDLE) wParam, SRMSGMOD_T, "messagecount", (DWORD)mesCount++);
	}
	//

	GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);

	bAutoPopup = M->GetByte(SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
	bAutoCreate = M->GetByte("autotabs", 1);
	bAutoContainer = M->GetByte("autocontainer", 1);
	dwStatusMask = M->GetDword("autopopupmask", -1);

	bAllowAutoCreate = FALSE;

	if (dwStatusMask == -1)
		bAllowAutoCreate = TRUE;
	else {
		char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)wParam, 0);
		DWORD dwStatus = 0;

		if (PluginConfig.g_MetaContactsAvail && szProto && !strcmp(szProto, (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0))) {
			HANDLE hSubconttact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, wParam, 0);

			szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hSubconttact, 0);
		}
		if (szProto) {
			dwStatus = (DWORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
			if (dwStatus == 0 || dwStatus <= ID_STATUS_OFFLINE || ((1 << (dwStatus - ID_STATUS_ONLINE)) & dwStatusMask))           // should never happen, but...
				bAllowAutoCreate = TRUE;
		}
	}
	if (bAllowAutoCreate && (bAutoPopup || bAutoCreate)) {
		BOOL bActivate = TRUE, bPopup = TRUE;
		if (bAutoPopup) {
			bActivate = bPopup = TRUE;
			if ((pContainer = FindContainerByName(szName)) == NULL)
				pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
			CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, FALSE, 0);
			return 0;
		} else {
			bActivate = FALSE;
			bPopup = (BOOL) M->GetByte("cpopup", 0);
			pContainer = FindContainerByName(szName);
			if (pContainer != NULL) {
				if ((IsIconic(pContainer->hwnd)) && PluginConfig.m_AutoSwitchTabs) {
					bActivate = TRUE;
					pContainer->dwFlags |= CNT_DEFERREDTABSELECT;
				}
				if (M->GetByte("limittabs", 0) &&  !_tcsncmp(pContainer->szName, _T("default"), 6)) {
					if ((pContainer = FindMatchingContainer(_T("default"), (HANDLE)wParam)) != NULL) {
						CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
						return 0;
					} else if (bAutoContainer) {
						pContainer = CreateContainer(szName, CNT_CREATEFLAG_MINIMIZED, (HANDLE)wParam);         // 2 means create minimized, don't popup...
						CreateNewTabForContact(pContainer, (HANDLE) wParam,  0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
						SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
						return 0;
					}
				} else {
					CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
					return 0;
				}

			} else {
				if (bAutoContainer) {
					pContainer = CreateContainer(szName, CNT_CREATEFLAG_MINIMIZED, (HANDLE)wParam);         // 2 means create minimized, don't popup...
					CreateNewTabForContact(pContainer, (HANDLE) wParam,  0, NULL, bActivate, bPopup, TRUE, (HANDLE)lParam);
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
	if (!(dbei.flags & DBEF_READ)) {
		UpdateTrayMenu(0, 0, dbei.szModule, NULL, (HANDLE)wParam, 1);
		if (!nen_options.bTraySupport || PluginConfig.m_WinVerMajor < 5) {
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


/*
 * this is the Unicode version of the SendMessageCommand handler. It accepts wchar_t strings
 * for filling the message input box with a passed message
 */

#if defined(_UNICODE)

INT_PTR SendMessageCommand_W(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	char *szProto;
	struct NewMessageWindowLParam newData = {
		0
	};
	struct ContainerWindowData *pContainer = 0;
	int isSplit = 1;

	/*
	 * make sure that only the main UI thread will handle window creation
     */
	if (GetCurrentThreadId() != PluginConfig.dwThreadID) {
		if (lParam) {
			unsigned iLen = lstrlenW((wchar_t *)lParam);
			wchar_t *tszText = (wchar_t *)malloc((iLen + 1) * sizeof(wchar_t));
			wcsncpy(tszText, (wchar_t *)lParam, iLen + 1);
			tszText[iLen] = 0;
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMANDW, wParam, (LPARAM)tszText);
		} else
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMANDW, wParam, 0);
		return 0;
	}

	/* does the HCONTACT's protocol support IM messages? */
	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	if (szProto) {
		if (!CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND)
			return 0;
	} else {
		/* unknown contact */
		return 0;
	}

	if (hwnd = M->FindWindow((HANDLE) wParam)) {
		if (lParam) {
			HWND hEdit;
			hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
			SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
			SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(TCHAR *) lParam);
		}
		SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwnd);
	} else {
		TCHAR szName[CONTAINER_NAMELEN + 1];
		/*
		 * attempt to fix "double tabs" opened by MS_MSG_SENDMESSAGE
		 * strange problem, maybe related to the window list service in miranda?
		 */
		if (M->GetByte("trayfix", 0)) {
			if (PluginConfig.hLastOpenedContact == (HANDLE)wParam) {
				//LeaveCriticalSection(&cs_sessions);
				return 0;
			}
		}
		PluginConfig.hLastOpenedContact = (HANDLE)wParam;
		GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
		pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, 1, (const char *)lParam, TRUE, TRUE, FALSE, 0);
	}
	return 0;
}

#endif

/*
 * the SendMessageCommand() invokes a message session window for the given contact.
 * e.g. it is called when user double clicks a contact on the contact list
 * it is implemented as a service, so external plugins can use it to open a message window.
 * contacts handle must be passed in wParam.
 */

INT_PTR SendMessageCommand(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	char *szProto;
	struct NewMessageWindowLParam newData = {
		0
	};
	struct ContainerWindowData *pContainer = 0;
	int isSplit = 1;

	if (GetCurrentThreadId() != PluginConfig.dwThreadID) {
		if (lParam) {
			unsigned iLen = lstrlenA((char *)lParam);
			char *szText = (char *)malloc(iLen + 1);
			strncpy(szText, (char *)lParam, iLen + 1);
			szText[iLen] = 0;
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMAND, wParam, (LPARAM)szText);
		} else
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMAND, wParam, 0);
		return 0;
	}

	/* does the HCONTACT's protocol support IM messages? */
	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	if (szProto) {
		if (!CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND)
			return 0;
	} else {
		/* unknown contact */
		return 0;
	}

	if (hwnd = M->FindWindow((HANDLE) wParam)) {
		if (lParam) {
			HWND hEdit;
			hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
			SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
			SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(char *) lParam);
		}
		SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwnd);
	} else {
		TCHAR szName[CONTAINER_NAMELEN + 1];
		/*
		 * attempt to fix "double tabs" opened by MS_MSG_SENDMESSAGE
		 * strange problem, maybe related to the window list service in miranda?
		 */
		if (M->GetByte("trayfix", 0)) {
			if (PluginConfig.hLastOpenedContact == (HANDLE)wParam) {
				//LeaveCriticalSection(&cs_sessions);
				return 0;
			}
		}
		PluginConfig.hLastOpenedContact = (HANDLE)wParam;
		GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
		pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, (const char *) lParam, TRUE, TRUE, FALSE, 0);
	}
	return 0;
}

/*
 * forwarding a message - this service is obsolete but still left intact
 */

static INT_PTR ForwardMessage(WPARAM wParam, LPARAM lParam)
{
	HWND hwndNew, hwndOld;
	RECT rc;
	struct ContainerWindowData *pContainer = 0;
	int iS = (int)M->GetByte("singlewinmode", 0);

	M->WriteByte(SRMSGMOD_T, "singlewinmode", 0);        // temporarily disable single window mode for forwarding...
	pContainer = FindContainerByName(_T("default"));
	M->WriteByte(SRMSGMOD_T, "singlewinmode", (BYTE)iS);

	if (pContainer == NULL)
		pContainer = CreateContainer(_T("default"), FALSE, 0);

	hwndOld = pContainer->hwndActive;
	hwndNew = CreateNewTabForContact(pContainer, 0, 0, (const char *) lParam, TRUE, TRUE, FALSE, 0);
	if (hwndNew) {
		if (hwndOld)
			ShowWindow(hwndOld, SW_HIDE);
		SendMessage(hwndNew, DM_SPLITTERMOVED, 0, (LPARAM) GetDlgItem(hwndNew, IDC_MULTISPLITTER));

		GetWindowRect(pContainer->hwnd, &rc);
		SetWindowPos(pContainer->hwnd, 0, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top) - 1, 0);
		SetWindowPos(pContainer->hwnd, 0, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), SWP_SHOWWINDOW);

		SetFocus(GetDlgItem(hwndNew, IDC_MESSAGE));
	}
	return 0;
}

/*
 * open a window when user clicks on the flashing "typing message" tray icon.
 * just calls SendMessageCommand() for the given contact.
 */
static INT_PTR TypingMessageCommand(WPARAM wParam, LPARAM lParam)
{
	CLISTEVENT *cle = (CLISTEVENT *) lParam;

	if (!cle)
		return 0;
	SendMessageCommand((WPARAM) cle->hContact, 0);
	return 0;
}

/*
 * displays typing notifications (MTN) in various ways.
 * this event is fired when a protocol has to notify the user about a typing buddy.
 * this fucntion distributes the typing event, based on the MTN configuration of the                                                                               .
 * message window plugin.                                                                                                                                                                .
 */
static int TypingMessage(WPARAM wParam, LPARAM lParam)
{
	HWND	hwnd = 0;
	int		issplit = 1, foundWin = 0, preTyping = 0;
	struct	ContainerWindowData *pContainer = NULL;
	BOOL	fShowOnClist = TRUE;

	if ((hwnd = M->FindWindow((HANDLE) wParam)) && M->GetByte(SRMSGMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING))
		preTyping = SendMessage(hwnd, DM_TYPING, 0, lParam);

	if (hwnd && IsWindowVisible(hwnd))
		foundWin = MessageWindowOpened(0, (LPARAM)hwnd);
	else
		foundWin = 0;


	if(hwnd) {
		SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
		if(pContainer == NULL)
			return 0;					// should never happen
	}

	if(M->GetByte(SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST)) {
		if(!hwnd && !M->GetByte(SRMSGMOD, SRMSGSET_SHOWTYPINGNOWINOPEN, 1))
			fShowOnClist = FALSE;
		if(hwnd && !M->GetByte(SRMSGMOD, SRMSGSET_SHOWTYPINGWINOPEN, 1))
			fShowOnClist = FALSE;
	}
	else
		fShowOnClist = FALSE;

	if((!foundWin || !(pContainer->dwFlags&CNT_NOSOUND)) && preTyping != (lParam != 0)){
		if (lParam)
			SkinPlaySound("TNStart");
		else
			SkinPlaySound("TNStop");
	}

	if(M->GetByte(SRMSGMOD, "ShowTypingPopup", 0)) {
		BOOL	fShow = FALSE;
		int		iMode = M->GetByte("MTN_PopupMode", 0);

		switch(iMode) {
			case 0:
				fShow = TRUE;
				break;
			case 1:
				if(!foundWin || !(pContainer && pContainer->hwndActive == hwnd && GetForegroundWindow() == pContainer->hwnd))
					fShow = TRUE;
				break;
			case 2:
				if(hwnd == 0)
					fShow = TRUE;
				else {
					if(PluginConfig.m_HideOnClose) {
						struct	ContainerWindowData *pContainer = 0;
						SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
						if(pContainer) {
							if(!IsWindowVisible(pContainer->hwnd) && !IsIconic(pContainer->hwnd))
								fShow = TRUE;
						}
					}
				}
				break;
		}
		if(fShow)
			TN_TypingMessage(wParam, lParam);
	}

	if ((int) lParam) {
		TCHAR szTip[256];

		_sntprintf(szTip, SIZEOF(szTip), TranslateT("%s is typing a message"), (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, wParam, GCDNF_TCHAR));
		if (fShowOnClist && ServiceExists(MS_CLIST_SYSTRAY_NOTIFY) && M->GetByte(SRMSGMOD, "ShowTypingBalloon", 0)) {
			MIRANDASYSTRAYNOTIFY tn;
			tn.szProto = NULL;
			tn.cbSize = sizeof(tn);
			tn.tszInfoTitle = TranslateT("Typing Notification");
			tn.tszInfo = szTip;
#ifdef UNICODE
			tn.dwInfoFlags = NIIF_INFO | NIIF_INTERN_UNICODE;
#else
			tn.dwInfoFlags = NIIF_INFO;
#endif
			tn.uTimeout = 1000 * 4;
			CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & tn);
		}
		if(fShowOnClist) {
			CLISTEVENT cle;

			ZeroMemory(&cle, sizeof(cle));
			cle.cbSize = sizeof(cle);
			cle.hContact = (HANDLE) wParam;
			cle.hDbEvent = (HANDLE) 1;
			cle.flags = CLEF_ONLYAFEW | CLEF_TCHAR;
			cle.hIcon = PluginConfig.g_buttonBarIcons[5];
			cle.pszService = "SRMsg/TypingMessage";
			cle.ptszTooltip = szTip;
			CallServiceSync(MS_CLIST_REMOVEEVENT, wParam, (LPARAM) 1);
			CallServiceSync(MS_CLIST_ADDEVENT, wParam, (LPARAM) & cle);
		}
	}
	return 0;
}

/*
 * watches various important database settings and reacts accordingly
 * needed to catch status, nickname and other changes in order to update open message
 * sessions.
 */

static int MessageSettingChanged(WPARAM wParam, LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
	char *szProto = NULL;

	HWND hwnd = M->FindWindow((HANDLE)wParam);

	if (hwnd == 0 && wParam != 0) {     // we are not interested in this event if there is no open message window/tab
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
		if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
			return 0;                       // filter out settings we aren't interested in...
		if (DBGetContactSettingWord((HANDLE)wParam, SRMSGMOD_T, "isFavorite", 0))
			AddContactToFavorites((HANDLE)wParam, NULL, szProto, NULL, 0, 0, 0, PluginConfig.g_hMenuFavorites, M->GetDword((HANDLE)wParam, "ANSIcodepage", PluginConfig.m_LangPackCP));
		if (M->GetDword((HANDLE)wParam, "isRecent", 0))
			AddContactToFavorites((HANDLE)wParam, NULL, szProto, NULL, 0, 0, 0, PluginConfig.g_hMenuRecent, M->GetDword((HANDLE)wParam, "ANSIcodepage", PluginConfig.m_LangPackCP));
		return 0;       // for the hContact.
	}

	if (wParam == 0 && strstr("Nick,yahoo_id", cws->szSetting)) {
		M->BroadcastMessage(DM_OWNNICKCHANGED, 0, (LPARAM)cws->szModule);
		return 0;
	}

	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);

	if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
		return 0;

	// metacontacts support

	if (!lstrcmpA(cws->szModule, PluginConfig.szMetaName) && !lstrcmpA(cws->szSetting, "Nick"))      // filter out this setting to avoid infinite loops while trying to obtain the most online contact
		return 0;

	if (hwnd) {
		if (strstr("MyHandle,Status,Nick,ApparentMode,Default,ForceSend,IdleTS,XStatusId,display_uid", cws->szSetting)) {
			if (!strcmp(cws->szSetting, "XStatusId"))
				PostMessage(hwnd, DM_UPDATESTATUSMSG, 0, 0);
			PostMessage(hwnd, DM_UPDATETITLE, 0, 0);
			if (!strcmp(cws->szSetting, "StatusMsg"))
				PostMessage(hwnd, DM_UPDATESTATUSMSG, 0, 0);
		} else if (lstrlenA(cws->szSetting) > 6 && lstrlenA(cws->szSetting) < 9 && !strncmp(cws->szSetting, "Status", 6))
			PostMessage(hwnd, DM_UPDATETITLE, 0, 1);
		else if (!strcmp(cws->szSetting, "MirVer")) {
			PostMessage(hwnd, DM_CLIENTCHANGED, 0, 0);
			if(g_bClientInStatusBar)
				ChangeClientIconInStatusBar(wParam,0);
		}
		else if (strstr("StatusMsg,StatusDescr,XStatusMsg,XStatusName,YMsg", cws->szSetting))
			PostMessage(hwnd, DM_UPDATESTATUSMSG, 0, 0);
	}
	return 0;
}

/*
 * event fired when a contact has been deleted. Make sure to close its message session
 */

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;

	if (hwnd = M->FindWindow((HANDLE) wParam)) {
		struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (dat)
			dat->bWasDeleted = 1;				// indicate a deleted contact. The WM_CLOSE handler will "fast close" the session and skip housekeeping.
		SendMessage(hwnd, WM_CLOSE, 0, 1);
	}
	return 0;
}

/*
 * used on startup to restore flashing tray icon if one or more messages are
 * still "unread"
 */

static void RestoreUnreadMessageAlerts(void)
{
	CLISTEVENT cle = { 0 };
	DBEVENTINFO dbei = { 0 };
	char toolTip[256];
	int windowAlreadyExists;
	int usingReadNext = 0;

	int autoPopup = M->GetByte(SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
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
				windowAlreadyExists = M->FindWindow(hContact) != NULL;
				if (!usingReadNext && windowAlreadyExists)
					continue;

				cle.hContact = hContact;
				cle.hDbEvent = hDbEvent;
				_snprintf(toolTip, sizeof(toolTip), Translate("Message from %s"), (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0));
				cle.pszTooltip = toolTip;
				CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
			}
			hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, (WPARAM) hDbEvent, 0);
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

/*
 * these globals are used in formatting.cpp to determine whether mathmod is available
 * it is needed to control the formatting of incoming messages
 */

int    haveMathMod = 0;
TCHAR  *mathModDelimiter = NULL;

/*
 * second part of the startup initialisation. All plugins are now fully loaded
 */

static int SplitmsgModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	int i;
	DBVARIANT dbv;
	MENUITEMINFOA mii = {0};
	HMENU submenu;
	BOOL bIEView;
	static Update upd = {0};

#if defined(_UNICODE)
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://download.miranda.or.at/tabsrmm/2/version.txt";
	static char *szUpdateUrl = "http://download.miranda.or.at/tabsrmm/2/tabsrmmW.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=3699";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=3699";
#else
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://download.miranda.or.at/tabsrmm/2/version.txt";
	static char *szUpdateUrl = "http://download.miranda.or.at/tabsrmm/2/tabsrmm.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=3698";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=3698";
#endif
	static char *szPrefix = "tabsrmm ";

#if defined(_UNICODE)
	if (!ServiceExists(MS_DB_CONTACT_GETSETTING_STR)) {
		MessageBox(NULL, TranslateT("This plugin requires db3x plugin version 0.5.1.0 or later"), _T("tabSRMM (Unicode)"), MB_OK);
		return 1;
	}
#endif
	UnhookEvent(hModulesLoadedEvent);
	hEventDbSettingChange = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, MessageSettingChanged);
	hEventContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);

	hEventDispatch = HookEvent(ME_DB_EVENT_ADDED, DispatchNewEvent);
	hEventDbEventAdded = HookEvent(ME_DB_EVENT_ADDED, MessageEventAdded);
	{
		CLISTMENUITEM mi = { 0 };
		mi.cbSize = sizeof(mi);
		mi.position = -2000090000;
		if (ServiceExists(MS_SKIN2_GETICONBYHANDLE)) {
			mi.flags = CMIF_ICONFROMICOLIB | CMIF_DEFAULT;
			mi.icolibItem = LoadSkinnedIconHandle(SKINICON_EVENT_MESSAGE);
		} else {
			mi.flags = CMIF_DEFAULT;
			mi.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
		}
		mi.pszName = LPGEN("&Message");
		mi.pszService = MS_MSG_SENDMESSAGE;
		hMenuItem = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
	}

	if (ServiceExists(MS_SKIN2_ADDICON))
		HookEvent(ME_SKIN2_ICONSCHANGED, IcoLibIconsChanged);
	if (ServiceExists(MS_AV_GETAVATARBITMAP)) {
		HookEvent(ME_AV_AVATARCHANGED, AvatarChanged);
		HookEvent(ME_AV_MYAVATARCHANGED, MyAvatarChanged);
	}
	HookEvent(ME_FONT_RELOAD, FontServiceFontsChanged);
	RestoreUnreadMessageAlerts();
	for (i = 0; i < NR_BUTTONBARICONS; i++)
		PluginConfig.g_buttonBarIcons[i] = 0;
	LoadIconTheme();

	CreateImageList(TRUE);

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = HBMMENU_CALLBACK;
	submenu = GetSubMenu(PluginConfig.g_hMenuContext, 7);
	for (i = 0; i <= 8; i++)
		SetMenuItemInfoA(submenu, (UINT_PTR)i, TRUE, &mii);

	BuildContainerMenu();

// #if defined(_UNICODE)
// #define SHORT_MODULENAME "tabSRMsgW (unicode)"
// #else
// #define SHORT_MODULENAME "tabSRMsg"
// #endif
#ifdef __GNUWIN32__
		#define SHORT_MODULENAME	"TabSRMM (MINGW32)"
#else
		#define	SHORT_MODULENAME	"TabSRMM"
#endif

	if (DBGetContactSettingString(NULL, "KnownModules", SHORT_MODULENAME, &dbv))
		DBWriteContactSettingString(NULL, "KnownModules", SHORT_MODULENAME, "SRMsg,Tab_SRMsg,TAB_Containers,TAB_ContainersW");
	else
		DBFreeVariant(&dbv);

	if (ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) {
		PluginConfig.g_SmileyAddAvail = 1;
		hEventSmileyAdd = HookEvent(ME_SMILEYADD_OPTIONSCHANGED, SmileyAddOptionsChanged);
	}

	//mad
	CB_InitDefaultButtons();
	ModPlus_Init(wParam, lParam);
	NotifyEventHooks(hHookToolBarLoadedEvt, (WPARAM)0, (LPARAM)0);
	//

	if (ServiceExists(MS_FAVATAR_GETINFO))
		PluginConfig.g_FlashAvatarAvail = 1;

	bIEView = ServiceExists(MS_IEVIEW_WINDOW);
	if (bIEView) {
		BOOL bOldIEView = M->GetByte("ieview_installed", 0);
		if (bOldIEView != bIEView)
			M->WriteByte(SRMSGMOD_T, "default_ieview", 1);
		M->WriteByte(SRMSGMOD_T, "ieview_installed", 1);
		HookEvent(ME_IEVIEW_OPTIONSCHANGED, IEViewOptionsChanged);
	} else
		M->WriteByte(SRMSGMOD_T, "ieview_installed", 0);
   //MAD
	PluginConfig.g_bDisableAniAvatars=M->GetByte("adv_DisableAniAvatars", 0);
	PluginConfig.g_iButtonsBarGap=M->GetByte("ButtonsBarGap", 1);
	//
	PluginConfig.m_hwndClist = (HWND)CallService(MS_CLUI_GETHWND, 0, 0);
	PluginConfig.m_MathModAvail = ServiceExists(MATH_RTF_REPLACE_FORMULAE);
	if (PluginConfig.m_MathModAvail) {
		char *szDelim = (char *)CallService(MATH_GET_STARTDELIMITER, 0, 0);
		if (szDelim) {
#if defined(_UNICODE)
			MultiByteToWideChar(CP_ACP, 0, szDelim, -1, PluginConfig.m_MathModStartDelimiter, sizeof(PluginConfig.m_MathModStartDelimiter));
#else
			strncpy(PluginConfig.m_MathModStartDelimiter, szDelim, sizeof(PluginConfig.m_MathModStartDelimiter));
#endif
			CallService(MTH_FREE_MATH_BUFFER, 0, (LPARAM)szDelim);
		}
	}

	haveMathMod = PluginConfig.m_MathModAvail;
	if (haveMathMod)
		mathModDelimiter = PluginConfig.m_MathModStartDelimiter;

	if (ServiceExists(MS_MC_GETDEFAULTCONTACT))
		PluginConfig.g_MetaContactsAvail = 1;

	if (PluginConfig.g_MetaContactsAvail)
		mir_snprintf(PluginConfig.szMetaName, 256, "%s", (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0));
	else
		PluginConfig.szMetaName[0] = 0;

	if (ServiceExists(MS_POPUP_ADDPOPUPEX))
		PluginConfig.g_PopupAvail = 1;

#if defined(_UNICODE)
	if (ServiceExists(MS_POPUP_ADDPOPUPW))
		PluginConfig.g_PopupWAvail = 1;
#endif

	if (M->GetByte("avatarmode", -1) == -1)
		M->WriteByte(SRMSGMOD_T, "avatarmode", 2);

	PluginConfig.g_hwndHotkeyHandler = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_HOTKEYSLAVE), 0, HotkeyHandlerDlgProc);

	CreateTrayMenus(TRUE);
	if (nen_options.bTraySupport)
		CreateSystrayIcon(TRUE);

	{
		CLISTMENUITEM mi = { 0 };
		mi.cbSize = sizeof(mi);
		mi.position = -500050005;
		mi.hIcon = PluginConfig.g_iconContainer;
		mi.pszContactOwner = NULL;
		mi.pszName = LPGEN("&Messaging settings...");
		mi.pszService = MS_TABMSG_SETUSERPREFS;
		PluginConfig.m_UserMenuItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
	}
	PreTranslateDates();
	PluginConfig.m_hFontWebdings = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Wingdings");

	// updater plugin support

 	upd.cbSize = sizeof(upd);
 	upd.szComponentName = pluginInfo.shortName;
 	upd.pbVersion = (BYTE *)/*CreateVersionStringPlugin*/CreateVersionString(pluginInfo.version, szCurrentVersion);
 	upd.cpbVersion = (int)(strlen((char *)upd.pbVersion));
	upd.szVersionURL = szFLVersionUrl;
	upd.szUpdateURL = szFLUpdateurl;
#if defined(_UNICODE)
	upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">tabSRMM Unicode 2.0 ";
#else
	upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">tabSRMM 2.0 ";
#endif
	upd.cpbVersionPrefix = (int)(strlen((char *)upd.pbVersionPrefix));

 	upd.szBetaUpdateURL = szUpdateUrl;
 	upd.szBetaVersionURL = szVersionUrl;
	upd.pbVersion = (unsigned char *)szCurrentVersion;
	upd.cpbVersion = lstrlenA(szCurrentVersion);
 	upd.pbBetaVersionPrefix = (BYTE *)szPrefix;
 	upd.cpbBetaVersionPrefix = (int)(strlen((char *)upd.pbBetaVersionPrefix));
 	upd.szBetaChangelogURL   ="http://miranda.radicaled.ru/public/tabsrmm/chglogeng.txt";

 	if (ServiceExists(MS_UPDATE_REGISTER))
 		CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);

	//if (M->GetByte("useskin", 0))
	//	ReloadContainerSkin(1, 1);

	RegisterFontServiceFonts();
	CacheLogFonts();
	Chat_ModulesLoaded(wParam, lParam);
	if(PluginConfig.g_PopupWAvail||PluginConfig.g_PopupAvail)
		TN_ModuleInit();

	return 0;
}

static int OkToExit(WPARAM wParam, LPARAM lParam)
{
	CreateSystrayIcon(0);
	CreateTrayMenus(0);
	CMimAPI::m_shutDown = true;
	UnhookEvent(hEventDbEventAdded);
	UnhookEvent(hEventDispatch);
	UnhookEvent(hEventPrebuildMenu);
	UnhookEvent(hEventDbSettingChange);
	UnhookEvent(hEventContactDeleted);
	ModPlus_PreShutdown(wParam, lParam);
	return 0;
}

static int PreshutdownSendRecv(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;

	if (PluginConfig.m_chat_enabled)
		Chat_PreShutdown(0, 0);

	TN_ModuleDeInit();

	while(pFirstContainer){
		//MaD: fix for correct closing hidden contacts
		if (PluginConfig.m_HideOnClose)
			PluginConfig.m_HideOnClose = FALSE;
		//
		SendMessage(pFirstContainer->hwnd, WM_CLOSE, 0, 1);
	}
	//MaD: to clean "actual history" messages count
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		M->WriteDword(hContact, SRMSGMOD_T, "messagecount", 0);
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	//
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
	CB_DeInitCustomButtons();
	/*
	 * the event API
	 */

	DestroyHookableEvent(g_hEvent_MsgWin);
	DestroyHookableEvent(g_hEvent_MsgPopup);

	NEN_WriteOptions(&nen_options);
	DestroyWindow(PluginConfig.g_hwndHotkeyHandler);

	//UnregisterClass(_T("TabSRMSG_Win"), g_hInst);
	UnregisterClass(_T("TSStatusBarClass"), g_hInst);
	UnregisterClassA("TSTabCtrlClass", g_hInst);
	return 0;
}

int SplitmsgShutdown(void)
{
	DestroyCursor(PluginConfig.hCurSplitNS);
	DestroyCursor(PluginConfig.hCurHyperlinkHand);
	DestroyCursor(PluginConfig.hCurSplitWE);
	DeleteObject(PluginConfig.m_hFontWebdings);
	FreeLibrary(GetModuleHandleA("riched20"));
	FreeLibrary(GetModuleHandleA("user32"));
	if (g_hIconDLL)
		FreeLibrary(g_hIconDLL);

	ImageList_RemoveAll(PluginConfig.g_hImageList);
	ImageList_Destroy(PluginConfig.g_hImageList);

	OleUninitialize();
	DestroyMenu(PluginConfig.g_hMenuContext);
	if (PluginConfig.g_hMenuContainer)
		DestroyMenu(PluginConfig.g_hMenuContainer);
	if (PluginConfig.g_hMenuEncoding)
		DestroyMenu(PluginConfig.g_hMenuEncoding);

	UnloadIcons();
	FreeTabConfig();

	if (rtf_ctable)
		free(rtf_ctable);

	UnloadTSButtonModule(0, 0);

	if (g_hIconDLL) {
		FreeLibrary(g_hIconDLL);
		g_hIconDLL = 0;
	}

	if(PluginConfig.hbmLogo)
		DeleteObject(PluginConfig.hbmLogo);

	return 0;
}

static int MyAvatarChanged(WPARAM wParam, LPARAM lParam)
{
	struct ContainerWindowData *pContainer = pFirstContainer;

	if (wParam == 0 || IsBadReadPtr((void *)wParam, 4))
		return 0;

	while (pContainer) {
		BroadCastContainer(pContainer, DM_MYAVATARCHANGED, wParam, lParam);
		pContainer = pContainer->pNextContainer;
	}
	return 0;
}

static int AvatarChanged(WPARAM wParam, LPARAM lParam)
{
	struct avatarCacheEntry *ace = (struct avatarCacheEntry *)lParam;
	HWND hwnd = M->FindWindow((HANDLE)wParam);

	if (wParam == 0) {			// protocol picture has changed...
		M->BroadcastMessage(DM_PROTOAVATARCHANGED, wParam, lParam);
		return 0;
	}
	if (hwnd) {
		struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (dat) {
			dat->ace = ace;
			if (dat->hwndFlash == 0)
				dat->panelWidth = -1;				// force new size calculations (not for flash avatars)
			DM_RecalcPictureSize(dat);
			if (dat->showPic == 0 || dat->showInfoPic == 0)
				GetAvatarVisibility(hwnd, dat);
			ShowPicture(dat, TRUE);
			dat->dwFlagsEx |= MWF_EX_AVATARCHANGED;
		}
	}
	return 0;
}

static int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
	LoadFromIconLib();
	CacheMsgLogIcons();
	if (PluginConfig.m_chat_enabled)
		Chat_IconsChanged(wParam, lParam);
	return 0;
}

static int PrebuildContactMenu(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	if ( hContact ) {
		char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

		CLISTMENUITEM clmi = {0};
		clmi.cbSize = sizeof( CLISTMENUITEM );
		clmi.flags = CMIM_FLAGS | CMIF_DEFAULT | CMIF_HIDDEN;

		if ( szProto ) {
			// leave this menu item hidden for chats
			if ( !M->GetByte(hContact, szProto, "ChatRoom", 0 ))
				if ( CallProtoService( szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND )
					clmi.flags &= ~CMIF_HIDDEN;
		}

		CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuItem, ( LPARAM )&clmi );
	}
	return 0;
}

int IconsChanged(WPARAM wParam, LPARAM lParam)
{
	CreateImageList(FALSE);
	CacheMsgLogIcons();
	M->BroadcastMessage(DM_OPTIONSAPPLIED, 0, 0);
	M->BroadcastMessage(DM_UPDATEWINICON, 0, 0);
	if (PluginConfig.m_chat_enabled)
		Chat_IconsChanged(wParam, lParam);

	return 0;
}

int LoadSendRecvMessageModule(void)
{
	int 	nOffset = 0;
	HDC 	hScrnDC;

	INITCOMMONCONTROLSEX icex;

	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;;
	InitCommonControlsEx(&icex);

	{
		TIME_ZONE_INFORMATION tzinfo;
		DWORD dwResult;


		dwResult = GetTimeZoneInformation(&tzinfo);

		nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
		goto tzdone;

		switch (dwResult) {

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
	}

tzdone:
	mREOLECallback = new REOLECallback;

	ZeroMemory((void *)&nen_options, sizeof(nen_options));

	M->m_hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	hUserPrefsWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	sendQueue = new SendQueue;
	Skin = new CSkin();

	InitOptions();
	hModulesLoadedEvent = HookEvent(ME_SYSTEM_MODULESLOADED, SplitmsgModulesLoaded);
	HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);
	HookEvent(ME_PROTO_CONTACTISTYPING, TypingMessage);
	HookEvent(ME_PROTO_ACK, ProtoAck);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreshutdownSendRecv);
	HookEvent(ME_SYSTEM_OKTOEXIT, OkToExit);

	hEventPrebuildMenu = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, PrebuildContactMenu);

	InitAPI();

	SkinAddNewSoundEx("RecvMsgActive", Translate("Messages"), Translate("Incoming (Focused Window)"));
	SkinAddNewSoundEx("RecvMsgInactive", Translate("Messages"), Translate("Incoming (Unfocused Window)"));
	SkinAddNewSoundEx("AlertMsg", Translate("Messages"), Translate("Incoming (New Session)"));
	SkinAddNewSoundEx("SendMsg", Translate("Messages"), Translate("Outgoing"));
	SkinAddNewSoundEx("SendError", Translate("Messages"), Translate("Error sending message"));
	//MAD: sound on typing...
	if(PluginConfig.g_bSoundOnTyping = M->GetByte("adv_soundontyping", 0))
		SkinAddNewSoundEx("SoundOnTyping", Translate("Other"), Translate("TABSRMM: Typing"));
	//
	PluginConfig.hCurSplitNS = LoadCursor(NULL, IDC_SIZENS);
	PluginConfig.hCurSplitWE = LoadCursor(NULL, IDC_SIZEWE);
	PluginConfig.hCurHyperlinkHand = LoadCursor(NULL, IDC_HAND);
	if (PluginConfig.hCurHyperlinkHand == NULL)
		PluginConfig.hCurHyperlinkHand = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_HYPERLINKHAND));

	LoadTSButtonModule();
	RegisterTabCtrlClass();
	PluginConfig.Reload();
	PluginConfig.dwThreadID = GetCurrentThreadId();

	M->configureCustomFolders();

	/*
	 * extract the default skin
	 */

	/*
	if(PluginConfig.m_WinVerMajor >=5 && M->GetDword("def_skin_installed", -1) != SKIN_VERSION) {
		M->WriteDword(SRMSGMOD_T, "def_skin_installed", SKIN_VERSION);

		for(i = 0; i < SKIN_NR_ELEMENTS; i++) {
			HRSRC 	hRes;
			HGLOBAL	hResource;

			hRes = FindResource(g_hInst, MAKEINTRESOURCE(my_default_skin[i].ulID), _T("SKIN_GLYPH"));

			if(hRes) {
				hResource = LoadResource(g_hInst, hRes);
				if(hResource) {
					TCHAR	szFilename[MAX_PATH];
					HANDLE  hFile;
					char 	*pData = (char *)LockResource(hResource);
					DWORD	dwSize = SizeofResource(g_hInst, hRes), written = 0;
					mir_sntprintf(szFilename, MAX_PATH, _T("%s\\default"), M->getSkinPath());
					if(!PathFileExists(szFilename))
						CreateDirectory(szFilename, NULL);
					mir_sntprintf(szFilename, MAX_PATH, _T("%s\\default\\%s"), M->getSkinPath(), (TCHAR *)my_default_skin[i].tszName);
					if((hFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE) {
						WriteFile(hFile, (void *)pData, dwSize, &written, NULL);
						CloseHandle(hFile);
					}
				}
			}
		}
	}
	*/
	/*
	 * load the logo
	 */
	char	szFilenameA[MAX_PATH];

	mir_snprintf(szFilenameA, MAX_PATH, "%s\\logo.png", M->getDataPathA());
	if(!PathFileExistsA(szFilenameA)) {
		HRSRC	hRes;
		HGLOBAL hResource;
		char	*pData = NULL;

		hRes = FindResource(g_hInst, MAKEINTRESOURCE(IDR_SKIN_LOGO), _T("SKIN_GLYPH"));
		if(hRes) {
			hResource = LoadResource(g_hInst, hRes);
			if(hResource) {
				DWORD written = 0, dwSize;
				HANDLE hFile;

				pData = (char *)LockResource(hResource);
				dwSize = SizeofResource(g_hInst, hRes);
				if((hFile = CreateFileA(szFilenameA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE) {
					WriteFile(hFile, (void *)pData, dwSize, &written, NULL);
					CloseHandle(hFile);
				}
			}
		}
	}
	PluginConfig.hbmLogo = IMG_LoadLogo(szFilenameA);

	ReloadTabConfig();
	NEN_ReadOptions(&nen_options);

	PluginConfig.g_hMenuContext = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TABCONTEXT));
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) PluginConfig.g_hMenuContext, 0);

	M->WriteByte(TEMPLATES_MODULE, "setup", 2);
	LoadDefaultTemplates();

	BuildCodePageList();
	GetDefaultContainerTitleFormat();
	PluginConfig.m_GlobalContainerFlags = M->GetDword("containerflags", CNT_FLAGS_DEFAULT);
	if (!(PluginConfig.m_GlobalContainerFlags & CNT_NEWCONTAINERFLAGS))
		PluginConfig.m_GlobalContainerFlags = CNT_FLAGS_DEFAULT;
	PluginConfig.m_GlobalContainerTrans = M->GetDword("containertrans", CNT_TRANS_DEFAULT);
	PluginConfig.local_gmt_diff = nOffset;

	hScrnDC = GetDC(0);
	PluginConfig.g_DPIscaleX = GetDeviceCaps(hScrnDC, LOGPIXELSX) / 96.0;
	PluginConfig.g_DPIscaleY = GetDeviceCaps(hScrnDC, LOGPIXELSY) / 96.0;
	ReleaseDC(0, hScrnDC);

	return 0;
}

STDMETHODIMP REOLECallback::GetNewStorage(LPSTORAGE FAR *lplpstg)
{
	LPLOCKBYTES lpLockBytes = NULL;
    SCODE sc  = ::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes);
    if (sc != S_OK)
		return sc;
    sc = ::StgCreateDocfileOnILockBytes(lpLockBytes, STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, lplpstg);
    if (sc != S_OK)
		lpLockBytes->Release();
    return sc;
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
	struct _MessageWindowData *dat = 0;
	NMHDR nmhdr;

	dat = (struct _MessageWindowData *) GetWindowLongPtr(hwndChild, GWLP_USERDATA);	// needed to obtain the hContact for the message window
	if (dat) {
		ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
		nmhdr.code = TCN_SELCHANGE;
		if (TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS)) > 1 && !(pContainer->dwFlags & CNT_DEFERREDTABSELECT)) {
			TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetTabIndexFromHWND(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), hwndChild));
			SendMessage(pContainer->hwnd, WM_NOTIFY, 0, (LPARAM) &nmhdr);	// just select the tab and let WM_NOTIFY do the rest
		}
		if (dat->bType == SESSIONTYPE_IM)
			SendMessage(pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
		if (IsIconic(pContainer->hwnd)) {
			//ShowWindow(pContainer->hwnd, SW_RESTORE);
			SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			SetForegroundWindow(pContainer->hwnd);
		}
		//MaD - hide on close feature
		if (!IsWindowVisible(pContainer->hwnd)) {
			WINDOWPLACEMENT wp={0};
			wp.length = sizeof(wp);
			GetWindowPlacement(pContainer->hwnd, &wp);

			/*
			 * all tabs must re-check the layout on activation because adding a tab while
			 * the container was hidden can make this necessary
             */
			BroadCastContainer(pContainer, DM_CHECKSIZE, 0, 0);
			if(wp.showCmd == SW_SHOWMAXIMIZED)
				ShowWindow(pContainer->hwnd, SW_SHOWMAXIMIZED);
			else {
				ShowWindow(pContainer->hwnd, SW_SHOWNA);
				SetForegroundWindow(pContainer->hwnd);
			}
			SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);			// make sure the active tab resizes its layout properly
		}
		//MaD_
		else if (GetForegroundWindow() != pContainer->hwnd)
			SetForegroundWindow(pContainer->hwnd);
		if (dat->bType == SESSIONTYPE_IM)
			SetFocus(GetDlgItem(hwndChild, IDC_MESSAGE));
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
	TCHAR 	*contactName = NULL, newcontactname[128], *szStatus, tabtitle[128];
	char 	*szProto = NULL;
	WORD 	wStatus;
	int		newItem;
	HWND 	hwndNew = 0;
	HWND 	hwndTab;
	struct 	NewMessageWindowLParam newData = {0};
	DBVARIANT dbv = {0};

	if (M->FindWindow(hContact) != 0) {
		_DebugPopup(hContact, _T("Warning: trying to create duplicate window"));
		return 0;
	}
	// if we have a max # of tabs/container set and want to open something in the default container...
	if (hContact != 0 && M->GetByte("limittabs", 0) &&  !_tcsncmp(pContainer->szName, _T("default"), 6)) {
		if ((pContainer = FindMatchingContainer(_T("default"), hContact)) == NULL) {
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
	contactName = (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) newData.hContact, GCDNF_TCHAR);

	/*
	 * cut nickname if larger than x chars...
	 */

	if (contactName && lstrlen(contactName) > 0) {
		if (M->GetByte("cuttitle", 0))
			CutContactName(contactName, newcontactname, safe_sizeof(newcontactname));
		else {
			lstrcpyn(newcontactname, contactName, safe_sizeof(newcontactname));
			newcontactname[127] = 0;
		}
		//Mad: to fix tab width for nicknames with ampersands
		DoubleAmpersands(newcontactname);
	} else
		lstrcpyn(newcontactname, _T("_U_"), safe_sizeof(newcontactname));

	wStatus = szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) newData.hContact, szProto, "Status", ID_STATUS_OFFLINE);
	szStatus = (TCHAR *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE)newData.hContact, szProto, "Status", ID_STATUS_OFFLINE), GCMDF_TCHAR);

	if (M->GetByte("tabstatus", 1))
		mir_sntprintf(tabtitle, safe_sizeof(tabtitle), _T("%s (%s)  "), newcontactname, szStatus);
	else
		mir_sntprintf(tabtitle, safe_sizeof(tabtitle), _T("%s   "), newcontactname);

	newData.item.pszText = tabtitle;
	newData.item.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	newData.item.iImage = 0;
	newData.item.cchTextMax = 255;

	hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
	// hide the active tab
	if (pContainer->hwndActive && bActivateTab)
		ShowWindow(pContainer->hwndActive, SW_HIDE);

	{
		int iTabIndex_wanted = M->GetDword(hContact, "tabindex", pContainer->iChilds * 100);
		int iCount = TabCtrl_GetItemCount(hwndTab);
		TCITEM item = {0};
		HWND hwnd;
		struct _MessageWindowData *dat;
		int relPos;
		int i;

		pContainer->iTabIndex = iCount;
		if (iCount > 0) {
			for (i = iCount - 1; i >= 0; i--) {
				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab, i, &item);
				hwnd = (HWND)item.lParam;
				dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
				if (dat) {
					relPos = M->GetDword(dat->hContact, "tabindex", i * 100);
					if (iTabIndex_wanted <= relPos)
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
	hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSPLITNEW), GetDlgItem(pContainer->hwnd, IDC_MSGTABS), DlgProcMessage, (LPARAM) & newData);
	SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);

	// if the container is minimized, then pop it up...

	if (IsIconic(pContainer->hwnd)) {
		if (bPopupContainer) {
			SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			SetFocus(pContainer->hwndActive);
		} else {
			if (pContainer->dwFlags & CNT_NOFLASH)
				SendMessage(pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
			else
				FlashContainer(pContainer, 1, 0);
		}
	}
	if (bActivateTab) {
		ActivateExistingTab(pContainer, hwndNew);
		SetFocus(hwndNew);
		RedrawWindow(pContainer->hwnd, NULL, NULL, RDW_INVALIDATE);
		UpdateWindow(pContainer->hwnd);
		if (GetForegroundWindow() != pContainer->hwnd && bPopupContainer == TRUE)
			SetForegroundWindow(pContainer->hwnd);
	}
	//MaD
	if (PluginConfig.m_HideOnClose&&!IsWindowVisible(pContainer->hwnd)){
		WINDOWPLACEMENT wp={0};
		wp.length = sizeof(wp);
		GetWindowPlacement(pContainer->hwnd, &wp);

		BroadCastContainer(pContainer, DM_CHECKSIZE, 0, 0);			// make sure all tabs will re-check layout on activation
		if(wp.showCmd == SW_SHOWMAXIMIZED)
			ShowWindow(pContainer->hwnd, SW_SHOWMAXIMIZED);
		else {
			if(bPopupContainer)
				ShowWindow(pContainer->hwnd, SW_SHOWNORMAL);
			else
				ShowWindow(pContainer->hwnd, SW_SHOWMINNOACTIVE);
		}
		SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);
	}
	//MaD_
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
	int iMaxTabs = M->GetDword("maxtabs", 0);

	if (iMaxTabs > 0 && M->GetByte("limittabs", 0) && !_tcsncmp(szName, _T("default"), 6)) {
		struct ContainerWindowData *pCurrent = pFirstContainer;
		// search a "default" with less than iMaxTabs opened...
		while (pCurrent) {
			if (!_tcsncmp(pCurrent->szName, _T("default"), 6) && pCurrent->iChilds < iMaxTabs) {
				pDesired = pCurrent;
				break;
			}
			pCurrent = pCurrent->pNextContainer;
		}
		return(pDesired != NULL ? pDesired : NULL);
	} else
		return FindContainerByName(szName);
}
/*
 * load some global icons.
 */

static void CreateImageList(BOOL bInitial)
{
	HICON hIcon;
	int cxIcon = GetSystemMetrics(SM_CXSMICON);
	int cyIcon = GetSystemMetrics(SM_CYSMICON);

	/*
	 * the imagelist is now a fake. It is still needed to provide the tab control with a
	 * image list handle. This will make sure that the tab control will reserve space for
	 * an icon on each tab.
	 */

	if (bInitial) {
		PluginConfig.g_hImageList = ImageList_Create(16, 16, PluginConfig.m_bIsXP ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 2, 0);
		PluginConfig.g_IconFolder = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_TREEVIEWEXPAND), IMAGE_ICON, 16, 16, LR_SHARED);
		PluginConfig.g_IconUnchecked = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_TREEVIEWUNCHECKED), IMAGE_ICON, 16, 16, LR_SHARED);
		PluginConfig.g_IconChecked = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_TREEVIEWCHECKED), IMAGE_ICON, 16, 16, LR_SHARED);
	} else
		ImageList_RemoveAll(PluginConfig.g_hImageList);

	hIcon = CreateIcon(g_hInst, 16, 16, 1, 4, NULL, NULL);
	ImageList_AddIcon(PluginConfig.g_hImageList, hIcon);
	//ImageList_GetIcon(myGlobals.g_hImageList, 0, 0);
	DestroyIcon(hIcon);

	PluginConfig.g_IconFileEvent = LoadSkinnedIcon(SKINICON_EVENT_FILE);
	PluginConfig.g_IconUrlEvent = LoadSkinnedIcon(SKINICON_EVENT_URL);
	PluginConfig.g_IconMsgEvent = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	PluginConfig.g_IconSend = PluginConfig.g_buttonBarIcons[9];
	PluginConfig.g_IconTypingEvent = PluginConfig.g_buttonBarIcons[5];
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

	hSVC[H_MS_MSG_MOD_MESSAGEDIALOGOPENED] =  CreateServiceFunction(MS_MSG_MOD_MESSAGEDIALOGOPENED, MessageWindowOpened);
	hSVC[H_MS_TABMSG_SETUSERPREFS] = CreateServiceFunction(MS_TABMSG_SETUSERPREFS, SetUserPrefs);
	hSVC[H_MS_TABMSG_TRAYSUPPORT] =  CreateServiceFunction(MS_TABMSG_TRAYSUPPORT, Service_OpenTrayMenu);
	hSVC[H_MSG_MOD_GETWINDOWFLAGS] =  CreateServiceFunction(MS_MSG_MOD_GETWINDOWFLAGS, GetMessageWindowFlags);

	SI_InitStatusIcons();

	//mad
	CB_InitCustomButtons();
	//

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
	struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	BYTE bType = dat ? dat->bType : SESSIONTYPE_IM;

	if (hContact == NULL || hwnd == NULL)
		return 0;

	if (!M->GetByte("eventapi", 1))
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

	if (type == MSG_WINDOW_EVT_CUSTOM) {
		se.cbSize = sizeof(se);
		se.evtCode = HIWORD(subType);
		se.hwnd = hwnd;
		se.extraFlags = (unsigned int)(LOWORD(subType));
		se.local = (void *)dat->sendBuffer;
		mwe.local = (void *) & se;
	} else
		mwe.local = NULL;
	return(NotifyEventHooks(g_hEvent_MsgWin, 0, (LPARAM)&mwe));
}

/*
 * standard icon definitions
 */

static ICONDESC _toolbaricons[] = {
	"tabSRMM_history", LPGEN("Show History"), &PluginConfig.g_buttonBarIcons[1], -IDI_HISTORY, 1,
	"tabSRMM_mlog", LPGEN("Message Log Options"), &PluginConfig.g_buttonBarIcons[2], -IDI_MSGLOGOPT, 1,
	"tabSRMM_add", LPGEN("Add contact"), &PluginConfig.g_buttonBarIcons[0], -IDI_ADDCONTACT, 1,
	"tabSRMM_multi", LPGEN("Multisend indicator"), &PluginConfig.g_buttonBarIcons[3], -IDI_MULTISEND, 1,
	"tabSRMM_quote", LPGEN("Quote text"), &PluginConfig.g_buttonBarIcons[8], -IDI_QUOTE, 1,
	"tabSRMM_save", LPGEN("Save and close"), &PluginConfig.g_buttonBarIcons[7], -IDI_SAVE, 1,
	"tabSRMM_send", LPGEN("Send message"), &PluginConfig.g_buttonBarIcons[9], -IDI_SEND, 1,
	"tabSRMM_avatar", LPGEN("Avatar menu"), &PluginConfig.g_buttonBarIcons[10], -IDI_CONTACTPIC, 1,
	"tabSRMM_close", LPGEN("Close"), &PluginConfig.g_buttonBarIcons[6], -IDI_CLOSEMSGDLG, 1,
	"tabSRMM_usermenu", LPGEN("User menu"), &PluginConfig.g_buttonBarIcons[4], -IDI_USERMENU, 1,
	NULL, NULL, NULL, 0, 0
};

static ICONDESC _exttoolbaricons[] = {
	"tabSRMM_emoticon", LPGEN("Smiley button"), &PluginConfig.g_buttonBarIcons[11], -IDI_SMILEYICON, 1,
	"tabSRMM_bold", LPGEN("Format bold"), &PluginConfig.g_buttonBarIcons[17], -IDI_FONTBOLD, 1,
	"tabSRMM_italic", LPGEN("Format italic"), &PluginConfig.g_buttonBarIcons[18], -IDI_FONTITALIC, 1,
	"tabSRMM_underline", LPGEN("Format underline"), &PluginConfig.g_buttonBarIcons[19], -IDI_FONTUNDERLINE, 1,
	"tabSRMM_face", LPGEN("Font face"), &PluginConfig.g_buttonBarIcons[20], -IDI_FONTFACE, 1,
	"tabSRMM_color", LPGEN("Font color"), &PluginConfig.g_buttonBarIcons[21], -IDI_FONTCOLOR, 1,
	"tabSRMM_strikeout", LPGEN("Format strike-through"), &PluginConfig.g_buttonBarIcons[30], -IDI_STRIKEOUT, 1,
	NULL, NULL, NULL, 0, 0
};
//MAD
static ICONDESC _chattoolbaricons[] = {
	"chat_bkgcol",LPGEN("Background colour"), &PluginConfig.g_buttonBarIcons[31] ,-IDI_BKGCOLOR, 1,
	"chat_settings",LPGEN("Room settings"),  &PluginConfig.g_buttonBarIcons[32],-IDI_TOPICBUT, 1,
	"chat_filter",LPGEN("Event filter disabled"), &PluginConfig.g_buttonBarIcons[33] ,-IDI_FILTER, 1,
	"chat_filter2",LPGEN("Event filter enabled"), &PluginConfig.g_buttonBarIcons[34] ,-IDI_FILTER2, 1,
	"chat_shownicklist",LPGEN("Show nicklist"),&PluginConfig.g_buttonBarIcons[35]  ,-IDI_SHOWNICKLIST, 1,
	"chat_hidenicklist",LPGEN("Hide nicklist"), &PluginConfig.g_buttonBarIcons[36] ,-IDI_HIDENICKLIST, 1,
	NULL, NULL, NULL, 0, 0
	};
//
static ICONDESC _logicons[] = {
	"tabSRMM_error", LPGEN("Message delivery error"), &PluginConfig.g_iconErr, -IDI_MSGERROR, 1,
	"tabSRMM_in", LPGEN("Incoming message"), &PluginConfig.g_iconIn, -IDI_ICONIN, 0,
	"tabSRMM_out", LPGEN("Outgoing message"), &PluginConfig.g_iconOut, -IDI_ICONOUT, 0,
	"tabSRMM_status", LPGEN("Statuschange"), &PluginConfig.g_iconStatus, -IDI_STATUSCHANGE, 0,
	NULL, NULL, NULL, 0, 0
};
static ICONDESC _deficons[] = {
	"tabSRMM_container", LPGEN("Static container icon"), &PluginConfig.g_iconContainer, -IDI_CONTAINER, 1,
	"tabSRMM_mtn_off", LPGEN("Sending typing notify is off"), &PluginConfig.g_buttonBarIcons[13], -IDI_SELFTYPING_OFF, 1,
	"tabSRMM_secureim_on", LPGEN("RESERVED (currently not in use)"), &PluginConfig.g_buttonBarIcons[14], -IDI_SECUREIM_ENABLED, 1,
	"tabSRMM_secureim_off", LPGEN("RESERVED (currently not in use)"), &PluginConfig.g_buttonBarIcons[15], -IDI_SECUREIM_DISABLED, 1,
	"tabSRMM_sounds_on", LPGEN("Sounds are On"), &PluginConfig.g_buttonBarIcons[22], -IDI_SOUNDSON, 1,
	"tabSRMM_sounds_off", LPGEN("Sounds are off"), &PluginConfig.g_buttonBarIcons[23], -IDI_SOUNDSOFF, 1,
	"tabSRMM_log_frozen", LPGEN("Message Log frozen"), &PluginConfig.g_buttonBarIcons[24], -IDI_MSGERROR, 1,
	"tabSRMM_undefined", LPGEN("Default"), &PluginConfig.g_buttonBarIcons[27], -IDI_EMPTY, 1,
	"tabSRMM_pulldown", LPGEN("Pulldown Arrow"), &PluginConfig.g_buttonBarIcons[16], -IDI_PULLDOWNARROW, 1,
	"tabSRMM_Leftarrow", LPGEN("Left Arrow"), &PluginConfig.g_buttonBarIcons[25], -IDI_LEFTARROW, 1,
	"tabSRMM_Rightarrow", LPGEN("Right Arrow"), &PluginConfig.g_buttonBarIcons[28], -IDI_RIGHTARROW, 1,
	"tabSRMM_Pulluparrow", LPGEN("Up Arrow"), &PluginConfig.g_buttonBarIcons[26], -IDI_PULLUPARROW, 1,
	"tabSRMM_sb_slist", LPGEN("Session List"), &PluginConfig.g_sideBarIcons[0], -IDI_SESSIONLIST, 1,
	"tabSRMM_sb_Favorites", LPGEN("Favorite Contacts"), &PluginConfig.g_sideBarIcons[1], -IDI_FAVLIST, 1,
	"tabSRMM_sb_Recent", LPGEN("Recent Sessions"), &PluginConfig.g_sideBarIcons[2], -IDI_RECENTLIST, 1,
	"tabSRMM_sb_Setup", LPGEN("Setup Sidebar"), &PluginConfig.g_sideBarIcons[3], -IDI_CONFIGSIDEBAR, 1,
	"tabSRMM_sb_Userprefs", LPGEN("Contact Preferences"), &PluginConfig.g_sideBarIcons[4], -IDI_USERPREFS, 1,
	NULL, NULL, NULL, 0, 0
};
static ICONDESC _trayIcon[] = {
	"tabSRMM_frame1", LPGEN("Frame 1"), &PluginConfig.m_AnimTrayIcons[0], -IDI_TRAYANIM1, 1,
	"tabSRMM_frame2", LPGEN("Frame 2"), &PluginConfig.m_AnimTrayIcons[1], -IDI_TRAYANIM2, 1,
	"tabSRMM_frame3", LPGEN("Frame 3"), &PluginConfig.m_AnimTrayIcons[2], -IDI_TRAYANIM3, 1,
	"tabSRMM_frame4", LPGEN("Frame 4"), &PluginConfig.m_AnimTrayIcons[3], -IDI_TRAYANIM4, 1,
	NULL, NULL, NULL, 0, 0
};

static struct _iconblocks {
	char *szSection;
	ICONDESC *idesc;
} ICONBLOCKS[] = {
	"TabSRMM/Default", _deficons,
	"TabSRMM/Toolbar", _toolbaricons,
	"TabSRMM/Toolbar", _exttoolbaricons,
	"TabSRMM/Toolbar", _chattoolbaricons,
	"TabSRMM/Message Log", _logicons,
	"TabSRMM/Animated Tray", _trayIcon,
	NULL, 0
};

static int GetIconPackVersion(HMODULE hDLL)
{
	char szIDString[256];
	int version = 0;

	if (LoadStringA(hDLL, IDS_IDENTIFY, szIDString, sizeof(szIDString)) == 0)
		version = 0;
	else {
		if (!strcmp(szIDString, "__tabSRMM_ICONPACK 1.0__"))
			version = 1;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 2.0__"))
			version = 2;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 3.0__"))
			version = 3;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 3.5__"))
			version = 4;
	}

	/*
	 * user may disable warnings about incompatible icon packs
	 */

	if(!M->GetByte("adv_IconpackWarning", 1))
		return version;

	if (version == 0)
		MessageBox(0, _T("The icon pack is either missing or too old."), _T("tabSRMM warning"), MB_OK | MB_ICONWARNING);
	else if (version > 0 && version < 4)
		MessageBox(0, _T("You are using an old icon pack (tabsrmm_icons.dll version < 3.5). This can cause missing icons, so please update the icon pack"), _T("tabSRMM warning"), MB_OK | MB_ICONWARNING);
	return version;
}
/*
 * setup default icons for the IcoLib service. This needs to be done every time the plugin is loaded
 * default icons are taken from the icon pack in either \icons or \plugins
 */

static int SetupIconLibConfig()
{
	SKINICONDESC sid = { 0 };
	char szFilename[MAX_PATH];
	int i = 0,j = 0, version = 0, n = 0;

	strncpy(szFilename, "icons\\tabsrmm_icons.dll", MAX_PATH);
	g_hIconDLL = LoadLibraryA(szFilename);
	if (g_hIconDLL == 0) {
		MessageBox(0, TranslateT("Icon pack missing. Please install it in the /icons subfolder."), _T("tabSRMM"), MB_OK);
		return 0;
	}

	GetModuleFileNameA(g_hIconDLL, szFilename, MAX_PATH);
	if (PluginConfig.m_chat_enabled)
		Chat_AddIcons();
	version = GetIconPackVersion(g_hIconDLL);
	PluginConfig.g_hbmUnknown = (HBITMAP)LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDB_UNKNOWNAVATAR), IMAGE_BITMAP, 0, 0, 0);
	if (PluginConfig.g_hbmUnknown == 0) {
		HDC dc = GetDC(0);
		PluginConfig.g_hbmUnknown = CreateCompatibleBitmap(dc, 20, 20);
		ReleaseDC(0, dc);
	}
	FreeLibrary(g_hIconDLL);
	g_hIconDLL = 0;

	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFilename;

	while (ICONBLOCKS[n].szSection) {
		i = 0;
		sid.pszSection = ICONBLOCKS[n].szSection;
		while (ICONBLOCKS[n].idesc[i].szDesc) {
			sid.pszName = ICONBLOCKS[n].idesc[i].szName;
			sid.pszDescription = ICONBLOCKS[n].idesc[i].szDesc;
			sid.iDefaultIndex = ICONBLOCKS[n].idesc[i].uId == -IDI_HISTORY ? 0 : ICONBLOCKS[n].idesc[i].uId;        // workaround problem /w icoLib and a resource id of 1 (actually, a Windows problem)
			i++;
			if(n>0&&n<4)
				PluginConfig.g_buttonBarIconHandles[j++]=(HANDLE)CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
			else
				CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		}
		n++;
	}
	return 1;
}

// load the icon theme from IconLib - check if it exists...

static int LoadFromIconLib()
{
	int i = 0, n = 0;

	while (ICONBLOCKS[n].szSection) {
		i = 0;
		while (ICONBLOCKS[n].idesc[i].szDesc) {
			*(ICONBLOCKS[n].idesc[i].phIcon) = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)ICONBLOCKS[n].idesc[i].szName);
			i++;
		}
		n++;
	}
	PluginConfig.g_buttonBarIcons[5] = PluginConfig.g_buttonBarIcons[12] = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_23");

	CacheMsgLogIcons();
	M->BroadcastMessage(DM_LOADBUTTONBARICONS, 0, 0);
	return 0;
}

/*
 * load icon theme from either icon pack or IcoLib
 */

static void LoadIconTheme()
{
	if (SetupIconLibConfig() == 0)
		return;
	else
		LoadFromIconLib();
	return;
}

static void UnloadIcons()
{
	int i = 0, n = 0;

	while (ICONBLOCKS[n].szSection) {
		i = 0;
		while (ICONBLOCKS[n].idesc[i].szDesc) {
			if (*(ICONBLOCKS[n].idesc[i].phIcon) != 0) {
				DestroyIcon(*(ICONBLOCKS[n].idesc[i].phIcon));
				*(ICONBLOCKS[n].idesc[i].phIcon) = 0;
			}
			i++;
		}
		n++;
	}
	if (PluginConfig.g_hbmUnknown)
		DeleteObject(PluginConfig.g_hbmUnknown);
	for (i = 0; i < 4; i++) {
		if (PluginConfig.m_AnimTrayIcons[i])
			DestroyIcon(PluginConfig.m_AnimTrayIcons[i]);
	}
}

/*
 * for the custom buttons
 * search a (named) icon from the default icon configuration and return a pointer
 * to its stored handle
 */
HICON *BTN_GetIcon(const TCHAR *szIconName)
{
#if defined(_UNICODE)
	char *szIconNameA = mir_u2a(szIconName);
#else
	const char *szIconNameA = szIconName;
#endif
	int n = 0, i;
	while (ICONBLOCKS[n].szSection) {
		i = 0;
		while (ICONBLOCKS[n].idesc[i].szDesc) {
			if (!_stricmp(ICONBLOCKS[n].idesc[i].szName, szIconNameA)) {
#if defined(_UNICODE)
				mir_free(szIconNameA);
#endif
				return(ICONBLOCKS[n].idesc[i].phIcon);
			}
			i++;
		}
		n++;
	}
	for (i = 0; i < Skin->getNrIcons(); i++) {
		const ICONDESCW *idesc = Skin->getIconDesc(i);
		if (!_tcsicmp(idesc->szName, szIconName)) {
#if defined(_UNICODE)
			mir_free(szIconNameA);
#endif
			return(idesc->phIcon);
		}
	}
#if defined(_UNICODE)
	mir_free(szIconNameA);
#endif
	return NULL;
}
