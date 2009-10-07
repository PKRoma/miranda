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

REOLECallback*		mREOLECallback;
NEN_OPTIONS 		nen_options;
extern PLUGININFOEX pluginInfo;
extern HANDLE 		hHookToolBarLoadedEvt;
static HANDLE 		hUserPrefsWindowLis = 0;
HMODULE 			g_hIconDLL = 0;

static void 	UnloadIcons();

extern INT_PTR 	CALLBACK 		DlgProcUserPrefsFrame(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern struct 	MsgLogIcon 		msgLogIcons[NR_LOGICONS * 3];
extern int 		CacheIconToBMP	(struct MsgLogIcon *theIcon, HICON hIcon, COLORREF backgroundColor, int sizeX, int sizeY);
extern void 	DeleteCachedIcon(struct MsgLogIcon *theIcon);

int     Chat_IconsChanged(WPARAM wp, LPARAM lp);
void    Chat_AddIcons(void);
int     Chat_PreShutdown(WPARAM wParam, LPARAM lParam);

/*
 * fired event when user changes IEView plugin options. Apply them to all open tabs
 */

int IEViewOptionsChanged(WPARAM wParam, LPARAM lParam)
{
	M->BroadcastMessage(DM_IEVIEWOPTIONSCHANGED, 0, 0);
	return 0;
}

/*
 * fired event when user changes smileyadd options. Notify all open tabs about the changes
 */

int SmileyAddOptionsChanged(WPARAM wParam, LPARAM lParam)
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
	HWND hWnd = WindowList_Find(PluginConfig.hUserPrefsWindowList, (HANDLE)wParam);
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
			HWND hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
			SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
			SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(char *) lParam);
		}
		SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM) &pContainer);          // ask the message window about its parent...
		ActivateExistingTab(pContainer, hwnd);
	} else {
		TCHAR szName[CONTAINER_NAMELEN + 1];
		GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
		pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, (HANDLE)wParam);
		CreateNewTabForContact(pContainer, (HANDLE) wParam, 0, (const char *) lParam, TRUE, TRUE, FALSE, 0);
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

int SplitmsgShutdown(void)
{
	DestroyCursor(PluginConfig.hCurSplitNS);
	DestroyCursor(PluginConfig.hCurHyperlinkHand);
	DestroyCursor(PluginConfig.hCurSplitWE);
	DeleteObject(PluginConfig.m_hFontWebdings);
	FreeLibrary(GetModuleHandleA("riched20"));
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

	if (Utils::rtf_ctable)
		free(Utils::rtf_ctable);

	UnloadTSButtonModule();

	if (g_hIconDLL) {
		FreeLibrary(g_hIconDLL);
		g_hIconDLL = 0;
	}
	return 0;
}

int MyAvatarChanged(WPARAM wParam, LPARAM lParam)
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

int AvatarChanged(WPARAM wParam, LPARAM lParam)
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
			DM_RecalcPictureSize(dat);
			if (dat->showPic == 0 || dat->showInfoPic == 0)
				GetAvatarVisibility(hwnd, dat);
			if(dat->hwndPanelPic) {
				dat->panelWidth = -1;				// force new size calculations (not for flash avatars)
				SendMessage(dat->hwnd, WM_SIZE, 0, 1);
			}
				dat->panelWidth = -1;				// force new size calculations (not for flash avatars)
				RedrawWindow(dat->hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
				SendMessage(dat->hwnd, WM_SIZE, 0, 1);
			ShowPicture(dat, TRUE);
			dat->dwFlagsEx |= MWF_EX_AVATARCHANGED;
		}
	}
	return 0;
}

int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
	LoadFromIconLib();
	CacheMsgLogIcons();
	if (PluginConfig.m_chat_enabled)
		Chat_IconsChanged(wParam, lParam);
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

/**
 * initialises the internal API, services, events etc...
 */

static struct _svcdef {
	char 			*szName;
	INT_PTR			(*pfnService)(WPARAM wParam, LPARAM lParam);
	HANDLE			*h;
} SERVICES[] = {
	MS_MSG_SENDMESSAGE, 	SendMessageCommand, 	&PluginConfig.hSvc[CGlobals::H_MS_MSG_SENDMESSAGE],
	MS_MSG_GETWINDOWAPI, 	GetWindowAPI, 			&PluginConfig.hSvc[CGlobals::H_MS_MSG_GETWINDOWAPI],
	MS_MSG_GETWINDOWCLASS, 	GetWindowClass,			&PluginConfig.hSvc[CGlobals::H_MS_MSG_GETWINDOWCLASS],
	MS_MSG_GETWINDOWDATA,	GetWindowData,			&PluginConfig.hSvc[CGlobals::H_MS_MSG_GETWINDOWDATA],
	"SRMsg/ReadMessage",	ReadMessageCommand,		&PluginConfig.hSvc[CGlobals::H_MS_MSG_READMESSAGE],
	"SRMsg/TypingMessage",	TypingMessageCommand,	&PluginConfig.hSvc[CGlobals::H_MS_MSG_TYPINGMESSAGE],
	MS_MSG_MOD_MESSAGEDIALOGOPENED, MessageWindowOpened, &PluginConfig.hSvc[CGlobals::H_MS_MSG_MOD_MESSAGEDIALOGOPENED],
	MS_TABMSG_SETUSERPREFS,	SetUserPrefs,			&PluginConfig.hSvc[CGlobals::H_MS_TABMSG_SETUSERPREFS],
	MS_TABMSG_TRAYSUPPORT,	Service_OpenTrayMenu,	&PluginConfig.hSvc[CGlobals::H_MS_TABMSG_TRAYSUPPORT],
	MS_MSG_MOD_GETWINDOWFLAGS, GetMessageWindowFlags, &PluginConfig.hSvc[CGlobals::H_MSG_MOD_GETWINDOWFLAGS]
};

static void TSAPI InitAPI()
{
	int	i;

	ZeroMemory(PluginConfig.hSvc, sizeof(HANDLE) * CGlobals::SERVICE_LAST);

	for(i = 0; i < safe_sizeof(SERVICES); i++)
		*(SERVICES[i].h) = CreateServiceFunction(SERVICES[i].szName, SERVICES[i].pfnService);

#if defined(_UNICODE)
	*(SERVICES[CGlobals::H_MS_MSG_SENDMESSAGEW].h) = CreateServiceFunction(MS_MSG_SENDMESSAGE "W", SendMessageCommand_W);
#endif

	SI_InitStatusIcons();
	CB_InitCustomButtons();

	/*
	 * the event API
	 */

	PluginConfig.m_event_MsgWin = CreateHookableEvent(ME_MSG_WINDOWEVENT);
	PluginConfig.m_event_MsgPopup = CreateHookableEvent(ME_MSG_WINDOWPOPUP);
}

int LoadSendRecvMessageModule(void)
{
	int 					nOffset = 0;
	INITCOMMONCONTROLSEX 	icex;

	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;;
	InitCommonControlsEx(&icex);

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
tzdone:

	LoadLibraryA("riched20");
	OleInitialize(NULL);
	mREOLECallback = new REOLECallback;
	ZeroMemory((void *)&nen_options, sizeof(nen_options));
	M->m_hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	PluginConfig.hUserPrefsWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	sendQueue = new SendQueue;
	Skin = new CSkin;
	InitOptions();

	InitAPI();

	PluginConfig.reloadSystemStartup();
	ReloadTabConfig();
	NEN_ReadOptions(&nen_options);

	M->WriteByte(TEMPLATES_MODULE, "setup", 2);
	LoadDefaultTemplates();

	BuildCodePageList();
	GetDefaultContainerTitleFormat();
	PluginConfig.local_gmt_diff = nOffset;

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

int TSAPI ActivateExistingTab(ContainerWindowData *pContainer, HWND hwndChild)
{
	struct _MessageWindowData *dat = 0;
	NMHDR nmhdr;

	dat = (struct _MessageWindowData *) GetWindowLongPtr(hwndChild, GWLP_USERDATA);	// needed to obtain the hContact for the message window
	if (dat && pContainer) {
		ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
		nmhdr.code = TCN_SELCHANGE;
		if (TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS)) > 1 && !(pContainer->dwFlags & CNT_DEFERREDTABSELECT)) {
			TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetTabIndexFromHWND(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), hwndChild));
			SendMessage(pContainer->hwnd, WM_NOTIFY, 0, (LPARAM) &nmhdr);	// just select the tab and let WM_NOTIFY do the rest
		}
		if (dat->bType == SESSIONTYPE_IM)
			SendMessage(pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
		if (IsIconic(pContainer->hwnd)) {
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

HWND TSAPI CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTab, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent)
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
		Utils::DoubleAmpersands(newcontactname);
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
	/*
	 * switchbar support
	 */
	if(pContainer->dwFlags & CNT_SIDEBAR) {
		_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(hwndNew, GWLP_USERDATA);
		if(dat)
			pContainer->SideBar->addSession(dat, pContainer->iTabIndex);
	}
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

struct ContainerWindowData* TSAPI FindMatchingContainer(const TCHAR *szName, HANDLE hContact)
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

void TSAPI CreateImageList(BOOL bInitial)
{
	HICON hIcon;
	int cxIcon = GetSystemMetrics(SM_CXSMICON);
	int cyIcon = GetSystemMetrics(SM_CYSMICON);

	/*
	 * the imagelist is now a fake. It is still needed to provide the tab control with a
	 * image list handle. This will make sure that the tab control will reserve space for
	 * an icon on each tab. This is a blank and empty icon
	 */

	if (bInitial) {
		PluginConfig.g_hImageList = ImageList_Create(16, 16, PluginConfig.m_bIsXP ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 2, 0);
		hIcon = CreateIcon(g_hInst, 16, 16, 1, 4, NULL, NULL);
		ImageList_AddIcon(PluginConfig.g_hImageList, hIcon);
		DestroyIcon(hIcon);
	}

	PluginConfig.g_IconFileEvent = LoadSkinnedIcon(SKINICON_EVENT_FILE);
	PluginConfig.g_IconMsgEvent = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	PluginConfig.g_IconSend = PluginConfig.g_buttonBarIcons[9];
	PluginConfig.g_IconTypingEvent = PluginConfig.g_buttonBarIcons[5];
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
	return(NotifyEventHooks(PluginConfig.m_event_MsgWin, 0, (LPARAM)&mwe));
}

/*
 * standard icon definitions
 */

static ICONDESC _toolbaricons[] = {
	"tabSRMM_mlog", LPGEN("Message Log Options"), &PluginConfig.g_buttonBarIcons[2], -IDI_MSGLOGOPT, 1,
	"tabSRMM_multi", LPGEN("Image tag"), &PluginConfig.g_buttonBarIcons[3], -IDI_IMAGETAG, 1,
	"tabSRMM_quote", LPGEN("Quote text"), &PluginConfig.g_buttonBarIcons[8], -IDI_QUOTE, 1,
	"tabSRMM_save", LPGEN("Save and close"), &PluginConfig.g_buttonBarIcons[7], -IDI_SAVE, 1,
	"tabSRMM_send", LPGEN("Send message"), &PluginConfig.g_buttonBarIcons[9], -IDI_SEND, 1,
	"tabSRMM_avatar", LPGEN("Edit user notes"), &PluginConfig.g_buttonBarIcons[10], -IDI_CONTACTPIC, 1,
	"tabSRMM_close", LPGEN("Close"), &PluginConfig.g_buttonBarIcons[6], -IDI_CLOSEMSGDLG, 1,
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
	"chat_filter",LPGEN("Event filter"), &PluginConfig.g_buttonBarIcons[33] ,-IDI_FILTER2, 1,
	"chat_shownicklist",LPGEN("Nick list"),&PluginConfig.g_buttonBarIcons[35]  ,-IDI_SHOWNICKLIST, 1,
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
	"tabSRMM_sounds_on", LPGEN("Sounds (status bar)"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_SOUNDS], -IDI_SOUNDSON, 1,
	"tabSRMM_pulldown", LPGEN("Pulldown Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_PULLDOWN], -IDI_PULLDOWNARROW, 1,
	"tabSRMM_Leftarrow", LPGEN("Left Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_LEFT], -IDI_LEFTARROW, 1,
	"tabSRMM_Rightarrow", LPGEN("Right Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_RIGHT], -IDI_RIGHTARROW, 1,
	"tabSRMM_Pulluparrow", LPGEN("Up Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_UP], -IDI_PULLUPARROW, 1,
	"tabSRMM_sb_slist", LPGEN("Session List"), &PluginConfig.g_sideBarIcons[0], -IDI_SESSIONLIST, 1,
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

static int TSAPI SetupIconLibConfig()
{
	SKINICONDESC sid = { 0 };
	char szFilename[MAX_PATH];
	int i = 0, j = 2, version = 0, n = 0;

	strncpy(szFilename, "icons\\tabsrmm_icons.dll", MAX_PATH);
	g_hIconDLL = LoadLibraryA(szFilename);
	if (g_hIconDLL == 0) {
		MessageBox(0, CTranslator::get(CTranslator::GEN_ICONPACK_WARNING), _T("tabSRMM"), MB_OK);
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
			sid.cx = sid.cy = 16;
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
	strncpy(szFilename, "plugins\\tabsrmm.dll", MAX_PATH);
	sid.pszDefaultFile = szFilename;
	sid.pszSection = "TabSRMM/Default";

	sid.pszName = "tabSRMM_overlay_disabled";
	sid.cx = sid.cy = 16;
	sid.pszDescription = "Feature disabled (used as overlay)";
	sid.iDefaultIndex = -IDI_FEATURE_DISABLED;
	CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

	sid.pszName = "tabSRMM_overlay_enabled";
	sid.cx = sid.cy = 16;
	sid.pszDescription = "Feature enabled (used as overlay)";
	sid.iDefaultIndex = -IDI_FEATURE_ENABLED;
	CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

	return 1;
}

// load the icon theme from IconLib - check if it exists...

static int TSAPI LoadFromIconLib()
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
	PluginConfig.g_buttonBarIcons[0] = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_8");
	PluginConfig.g_buttonBarIcons[1] = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_10");
	PluginConfig.g_buttonBarIconHandles[0] = (HICON)CallService(MS_SKIN2_GETICONHANDLE, 0, (LPARAM)"core_main_10");
	PluginConfig.g_buttonBarIconHandles[1] = (HICON)CallService(MS_SKIN2_GETICONHANDLE, 0, (LPARAM)"core_main_8");
	PluginConfig.g_buttonBarIconHandles[20] = (HICON)CallService(MS_SKIN2_GETICONHANDLE, 0, (LPARAM)"core_main_9");

	PluginConfig.g_buttonBarIcons[5] = PluginConfig.g_buttonBarIcons[12] = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_23");
	PluginConfig.g_IconChecked = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_19");
	PluginConfig.g_IconUnchecked = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_20");
	PluginConfig.g_IconFolder = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"core_main_5");

	PluginConfig.g_iconOverlayEnabled = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"tabSRMM_overlay_enabled");
	PluginConfig.g_iconOverlayDisabled = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"tabSRMM_overlay_disabled");

	CacheMsgLogIcons();
	M->BroadcastMessage(DM_LOADBUTTONBARICONS, 0, 0);
	return 0;
}

/*
 * load icon theme from either icon pack or IcoLib
 */

void TSAPI LoadIconTheme()
{
	if (SetupIconLibConfig() == 0)
		return;
	else
		LoadFromIconLib();

	Skin->setupTabCloseBitmap();
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
