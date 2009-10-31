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
 * The hotkeyhandler is a small, invisible window which cares about a few things:

    a) event notify stuff, messages posted from the popups to avoid threading
       issues.

    b) tray icon handling - cares about the events sent by the tray icon, handles
       the menus, doubleclicks and so on.

    c) send later job management. The send later queue is also used for
       multisend.
 */

#include "commonheaders.h"
#pragma hdrstop

extern HICON		hIcons[];
extern INT_PTR		SendMessageCommand(WPARAM wParam, LPARAM lParam);
extern INT_PTR		SendMessageCommand_W(WPARAM wParam, LPARAM lParam);

static UINT 	WM_TASKBARCREATED;
static HANDLE 	hSvcHotkeyProcessor = 0;

#define TIMERID_SENDLATER 12000
#define TIMERID_SENDLATER_TICK 13000

#define TIMEOUT_SENDLATER 10000
#define TIMEOUT_SENDLATER_TICK 200

static HOTKEYDESC _hotkeydescs[] = {
	0, "tabsrmm_mostrecent", "Most recent unread session", TABSRMM_HK_SECTION_IM, MS_TABMSG_HOTKEYPROCESS, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'R'), TABSRMM_HK_LASTUNREAD,
	0, "tabsrmm_paste_and_send", "Paste and send", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'D'), TABSRMM_HK_PASTEANDSEND,
	0, "tabsrmm_uprefs", "Contact's messaging prefs", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'C'), TABSRMM_HK_SETUSERPREFS,
	0, "tabsrmm_copts", "Container options", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_CONTROL, 'O'), TABSRMM_HK_CONTAINEROPTIONS,
	0, "tabsrmm_nudge", "Send nudge", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_CONTROL, 'N'), TABSRMM_HK_NUDGE,
	0, "tabsrmm_sendfile", "Send a file", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'F'), TABSRMM_HK_SENDFILE,
	0, "tabsrmm_quote", "Quote message", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'Q'), TABSRMM_HK_QUOTEMSG,
	0, "tabsrmm_sendlater", "Toggle send later", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_ALT, 'S'), TABSRMM_HK_TOGGLESENDLATER,

	0, "tabsrmm_send", "Send message", TABSRMM_HK_SECTION_GENERIC, 0, 0, TABSRMM_HK_SEND,
	0, "tabsrmm_emot", "Smiley selector", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT, 'E'), TABSRMM_HK_EMOTICONS,
	0, "tabsrmm_hist", "Show message history", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT, 'H'), TABSRMM_HK_HISTORY,
	0, "tabsrmm_umenu", "Show user menu", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'D'), TABSRMM_HK_USERMENU,
	0, "tabsrmm_udet", "Show user details", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'U'), TABSRMM_HK_USERDETAILS,
	0, "tabsrmm_tbar", "Toggle tool bar", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_SHIFT, 'T'), TABSRMM_HK_TOGGLETOOLBAR,
	0, "tabsrmm_ipanel", "Toggle info panel", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_CONTROL, 'I'), TABSRMM_HK_TOGGLEINFOPANEL,
	0, "tabsrmm_rtl", "Toggle text direction", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_CONTROL, 'B'), TABSRMM_HK_TOGGLERTL,
	0, "tabsrmm_msend", "Toggle multi send", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_CONTROL, 'M'), TABSRMM_HK_TOGGLEMULTISEND,
	0, "tabsrmm_clearlog", "Clear message log", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_CONTROL, 'L'), TABSRMM_HK_CLEARLOG,
	0, "tabsrmm_notes", "Edit user notes", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_SHIFT | HOTKEYF_CONTROL, 'N'), TABSRMM_HK_EDITNOTES,
	0, "tabsrmm_sbar", "Collapse side bar", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(0, VK_F9), TABSRMM_HK_TOGGLESIDEBAR,
	0, "tabsrmm_muc_cmgr", "Channel manager", TABSRMM_HK_SECTION_GC, 0, HOTKEYCODE(HOTKEYF_SHIFT | HOTKEYF_CONTROL, 'C'), TABSRMM_HK_CHANNELMGR,
	0, "tabsrmm_muc_filter", "Toggle filter", TABSRMM_HK_SECTION_GC, 0, HOTKEYCODE(HOTKEYF_SHIFT | HOTKEYF_CONTROL, 'F'), TABSRMM_HK_FILTERTOGGLE,
	0, "tabsrmm_muc_filter", "Toggle nick list", TABSRMM_HK_SECTION_GC, 0, HOTKEYCODE(HOTKEYF_SHIFT | HOTKEYF_CONTROL, 'N'), TABSRMM_HK_LISTTOGGLE

};

std::vector<HANDLE> sendLaterContactList;
std::vector<SendLaterJob *> sendLaterJobList;

typedef std::vector<SendLaterJob *>::iterator SendLaterJobIterator;
static 	SendLaterJobIterator g_jobs;

int TSAPI SendLater_SendIt(SendLaterJob *job);

#define SENDLATER_AGE_THRESHOLD (86400 * 3)				// 3 days, older messages will be removed from the db.
#define SENDLATER_RESEND_THRESHOLD 180					// timeouted messages should be resent after that many seconds
#define SENDLATER_PROCESS_INTERVAL 50					// process the list of waiting job every this many seconds

LRESULT ProcessHotkeysByMsgFilter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR ctrlId)
{
	MSGFILTER  mf;
	mf.nmhdr.code = EN_MSGFILTER;
	mf.nmhdr.hwndFrom = hwnd;
	mf.nmhdr.idFrom = ctrlId;

	mf.lParam = lParam;
	mf.wParam = wParam;
	mf.msg = msg;

	return(SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&mf));
}
static INT_PTR HotkeyProcessor(WPARAM wParam, LPARAM lParam)
{
	switch(lParam) {
		case TABSRMM_HK_LASTUNREAD:
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_MBUTTONDOWN);
			break;
		default:
			break;
	}
	return 0;
}

void TSAPI HandleMenuEntryFromhContact(int iSelection)
{
	HWND hWnd = M->FindWindow((HANDLE)iSelection);
	SESSION_INFO *si = NULL;

	if (iSelection == 0)
		return;

	if (hWnd && IsWindow(hWnd)) {
		struct ContainerWindowData *pContainer = 0;
		SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
		if (pContainer) {
			ActivateExistingTab(pContainer, hWnd);
			pContainer->hwndSaved = 0;
			SetForegroundWindow(pContainer->hwnd);
		} else
			CallService(MS_MSG_SENDMESSAGE, (WPARAM)iSelection, 0);
	} else if ((si = SM_FindSessionByHCONTACT((HANDLE)iSelection)) != NULL) {
		if (si->hWnd) {															// session does exist, but no window is open for it
			struct ContainerWindowData *pContainer = 0;

			SendMessage(si->hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
			if (pContainer) {
				ActivateExistingTab(pContainer, si->hWnd);
				if (GetForegroundWindow() != pContainer->hwnd)
					SetForegroundWindow(pContainer->hwnd);
				SetFocus(GetDlgItem(pContainer->hwndActive, IDC_CHAT_MESSAGE));
			} else
				goto nothing_open;
		} else
			goto nothing_open;
	} else {
nothing_open:
		CallService(MS_CLIST_CONTACTDOUBLECLICKED, (WPARAM)iSelection, 0);
	}
}

void TSAPI DrawMenuItem(DRAWITEMSTRUCT *dis, HICON hIcon, DWORD dwIdle)
{
	FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
	/*if (dis->itemState & ODS_HOTLIGHT)
		DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
	else if (dis->itemState & ODS_SELECTED)
		DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);*/
	if (dwIdle) {
		CSkin::DrawDimmedIcon(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - 16) / 2, 16, 16, hIcon, 180);
	} else
		DrawIconEx(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - 16) / 2, hIcon, 16, 16, 0, 0, DI_NORMAL | DI_COMPAT);
}

static time_t t_last_sendlater_processed = 0;

LONG_PTR CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static POINT ptLast;
	static int iMousedown;

	if (msg == WM_TASKBARCREATED) {
		CreateSystrayIcon(FALSE);
		if (nen_options.bTraySupport)
			CreateSystrayIcon(TRUE);
		return 0;
	}
	switch (msg) {
		case WM_CREATE:
			int i;

			for(i = 0; i < safe_sizeof(_hotkeydescs); i++) {
				_hotkeydescs[i].cbSize = sizeof(HOTKEYDESC);
				_hotkeydescs[i].pszSection = Translate(_hotkeydescs[i].pszSection);
				_hotkeydescs[i].pszDescription = Translate(_hotkeydescs[i].pszDescription);
				CallService(MS_HOTKEY_REGISTER, 0, (LPARAM)&_hotkeydescs[i]);
			}
			t_last_sendlater_processed = time(0);

			sendLaterContactList.clear();

			WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");
			ShowWindow(hwndDlg, SW_HIDE);
			hSvcHotkeyProcessor = CreateServiceFunction(MS_TABMSG_HOTKEYPROCESS, HotkeyProcessor);
			SetTimer(hwndDlg, TIMERID_SENDLATER, TIMEOUT_SENDLATER, NULL);
			break;
		case WM_HOTKEY: {
			CLISTEVENT *cli = 0;

			cli = (CLISTEVENT *)CallService(MS_CLIST_GETEVENT, (WPARAM)INVALID_HANDLE_VALUE, (LPARAM)0);
			if (cli != NULL) {
				if (strncmp(cli->pszService, "SRMsg/TypingMessage", strlen(cli->pszService))) {
					CallService(cli->pszService, 0, (LPARAM)cli);
					break;
				}
			}
			if (wParam == 0xc001)
				SendMessage(hwndDlg, DM_TRAYICONNOTIFY, 101, WM_MBUTTONDOWN);

			break;
		}
		/*
		 * handle the popup menus (session list, favorites, recents...
		 * just draw some icons, nothing more :)
		 */
		case WM_MEASUREITEM: {
			LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT) lParam;
			lpmi->itemHeight = 0;
			lpmi->itemWidth = 6;
			return TRUE;
		}
		case WM_DRAWITEM: {
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
			struct _MessageWindowData *dat = 0;
			if (dis->CtlType == ODT_MENU && (dis->hwndItem == (HWND)PluginConfig.g_hMenuFavorites || dis->hwndItem == (HWND)PluginConfig.g_hMenuRecent)) {
				HICON hIcon = (HICON)dis->itemData;

				DrawMenuItem(dis, hIcon, 0);
				return TRUE;
			} else if (dis->CtlType == ODT_MENU) {
				HWND hWnd = M->FindWindow((HANDLE)dis->itemID);
				DWORD idle = 0;

				if (hWnd == NULL) {
					SESSION_INFO *si = SM_FindSessionByHCONTACT((HANDLE)dis->itemID);

					hWnd = si ? si->hWnd : 0;
				}

				if (hWnd)
					dat = (struct _MessageWindowData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

				if (dis->itemData >= 0) {
					HICON hIcon;
					BOOL fNeedFree = FALSE;

					if (dis->itemData > 0)
						hIcon = dis->itemData & 0x10000000 ? hIcons[ICON_HIGHLIGHT] : PluginConfig.g_IconMsgEvent;
					else if (dat != NULL) {
						hIcon = MY_GetContactIcon(dat);
						idle = dat->idle;
					} else
						hIcon = PluginConfig.g_iconContainer;

					DrawMenuItem(dis, hIcon, idle);
					if (fNeedFree)
						DestroyIcon(hIcon);

					return TRUE;
				}
			}
		}
		break;
		case DM_TRAYICONNOTIFY: {
			int iSelection;

			if (wParam == 100 || wParam == 101) {
				switch (lParam) {
					case WM_LBUTTONUP: {
						POINT pt;
						GetCursorPos(&pt);
						if (wParam == 100)
							SetForegroundWindow(hwndDlg);
						if (GetMenuItemCount(PluginConfig.g_hMenuTrayUnread) > 0) {
							iSelection = TrackPopupMenu(PluginConfig.g_hMenuTrayUnread, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
							HandleMenuEntryFromhContact(iSelection);
						} else
							TrackPopupMenu(GetSubMenu(PluginConfig.g_hMenuContext, 8), TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
						if (wParam == 100)
							PostMessage(hwndDlg, WM_NULL, 0, 0);
						break;
					}
					case WM_MBUTTONDOWN: {
						MENUITEMINFOA mii = {0};
						int i, iCount = GetMenuItemCount(PluginConfig.g_hMenuTrayUnread);

						if (wParam == 100)
							SetForegroundWindow(hwndDlg);

						if (iCount > 0) {
							UINT uid = 0;
							mii.fMask = MIIM_DATA;
							mii.cbSize = sizeof(mii);
							i = iCount - 1;
							do {
								GetMenuItemInfoA(PluginConfig.g_hMenuTrayUnread, i, TRUE, &mii);
								if (mii.dwItemData > 0) {
									uid = GetMenuItemID(PluginConfig.g_hMenuTrayUnread, i);
									HandleMenuEntryFromhContact(uid);
									break;
								}
							} while (--i >= 0);
							if (uid == 0 && pLastActiveContainer != NULL) {                // no session found, restore last active container
								if (IsIconic(pLastActiveContainer->hwnd) || !IsWindowVisible(pLastActiveContainer->hwnd)) {
									SendMessage(pLastActiveContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
									SetForegroundWindow(pLastActiveContainer->hwnd);
								} else {
									if(PluginConfig.m_HideOnClose)
										ShowWindow(pLastActiveContainer->hwnd, SW_HIDE);
									else
										SendMessage(pLastActiveContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
								}
							}
						}
						if (wParam == 100)
							PostMessage(hwndDlg, WM_NULL, 0, 0);
						break;
					}
					case WM_RBUTTONUP: {
						HMENU submenu = PluginConfig.g_hMenuTrayContext;
						POINT pt;

						if (wParam == 100)
							SetForegroundWindow(hwndDlg);
						GetCursorPos(&pt);
						CheckMenuItem(submenu, ID_TRAYCONTEXT_DISABLEALLPOPUPS, MF_BYCOMMAND | (nen_options.iDisable ? MF_CHECKED : MF_UNCHECKED));
						CheckMenuItem(submenu, ID_TRAYCONTEXT_DON40223, MF_BYCOMMAND | (nen_options.iNoSounds ? MF_CHECKED : MF_UNCHECKED));
						CheckMenuItem(submenu, ID_TRAYCONTEXT_DON, MF_BYCOMMAND | (nen_options.iNoAutoPopup ? MF_CHECKED : MF_UNCHECKED));
						EnableMenuItem(submenu, ID_TRAYCONTEXT_HIDEALLMESSAGECONTAINERS, MF_BYCOMMAND | (nen_options.bTraySupport) ? MF_ENABLED : MF_GRAYED);
						CheckMenuItem(submenu, ID_TRAYCONTEXT_SHOWTHETRAYICON, MF_BYCOMMAND | (nen_options.bTraySupport ? MF_CHECKED : MF_UNCHECKED));
						iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);

						if (iSelection) {
							MENUITEMINFO mii = {0};

							mii.cbSize = sizeof(mii);
							mii.fMask = MIIM_DATA | MIIM_ID;
							GetMenuItemInfo(submenu, (UINT_PTR)iSelection, FALSE, &mii);
							if (mii.dwItemData != 0) {                      // this must be an itm of the fav or recent menu
								HandleMenuEntryFromhContact(iSelection);
							} else {
								switch (iSelection) {
									case ID_TRAYCONTEXT_SHOWTHETRAYICON:
										nen_options.bTraySupport = !nen_options.bTraySupport;
										CreateSystrayIcon(nen_options.bTraySupport ? TRUE : FALSE);
										break;
									case ID_TRAYCONTEXT_DISABLEALLPOPUPS:
										nen_options.iDisable ^= 1;
										break;
									case ID_TRAYCONTEXT_DON40223:
										nen_options.iNoSounds ^= 1;
										break;
									case ID_TRAYCONTEXT_DON:
										nen_options.iNoAutoPopup ^= 1;
										break;
									case ID_TRAYCONTEXT_HIDEALLMESSAGECONTAINERS: {
										struct ContainerWindowData *pContainer = pFirstContainer;

										while (pContainer) {
											ShowWindow(pContainer->hwnd, SW_HIDE);
											pContainer = pContainer->pNextContainer;
										}
										break;
									}
									case ID_TRAYCONTEXT_RESTOREALLMESSAGECONTAINERS: {
										struct ContainerWindowData *pContainer = pFirstContainer;

										while (pContainer) {
											ShowWindow(pContainer->hwnd, SW_SHOW);
											pContainer = pContainer->pNextContainer;
										}
										break;
									}
									case ID_TRAYCONTEXT_BE: {
										struct ContainerWindowData *pContainer = pFirstContainer;

										nen_options.iDisable = 1;
										nen_options.iNoSounds = 1;
										nen_options.iNoAutoPopup = 1;

										while (pContainer) {
											SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 1);
											pContainer = pContainer->pNextContainer;
										}
										break;
									}
								}
							}
						}
						if (wParam == 100)
							PostMessage(hwndDlg, WM_NULL, 0, 0);
						break;
					}
					default:
						break;
				}
			}
			break;
		}
		/*
		 * handle an event from the popup module (mostly window activation). Since popups may run in different threads, the message
		 * is posted to our invisible hotkey handler which does always run within the main thread.
		 * wParam is the hContact
		 * lParam the event handle
		 */
		case DM_HANDLECLISTEVENT: {
			CLISTEVENT *cle = (CLISTEVENT *)CallService(MS_CLIST_GETEVENT, wParam, 0);

			/*
			 * if lParam == NULL, don't consider clist events, just open the message tab
			 */

			if(lParam == 0) {
				HandleMenuEntryFromhContact((int)wParam);
				break;
			}

			/*
			 * first try, if the clist returned an event...
			 */
			if (cle) {
				if (ServiceExists(cle->pszService)) {
					CallService(cle->pszService, (WPARAM)NULL, (LPARAM)cle);
					CallService(MS_CLIST_REMOVEEVENT, (WPARAM)cle->hContact, (LPARAM)cle->hDbEvent);
				}
			} else {             // still, we got that message posted.. the event may be waiting in tabSRMMs tray...
				HandleMenuEntryFromhContact((int)wParam);
			}
			break;
		}
		case DM_DOCREATETAB: {
			HWND hWnd = M->FindWindow((HANDLE)lParam);
			if (hWnd && IsWindow(hWnd)) {
				struct ContainerWindowData *pContainer = 0;

				SendMessage(hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
				if (pContainer) {
					int iTabs = TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS));
					if (iTabs == 1)
						SendMessage(pContainer->hwnd, WM_CLOSE, 0, 1);
					else
						SendMessage(hWnd, WM_CLOSE, 0, 1);

					CreateNewTabForContact((struct ContainerWindowData *)wParam, (HANDLE)lParam, 0, NULL, TRUE, TRUE, FALSE, 0);
				}
			}
			break;
		}
		case DM_DOCREATETAB_CHAT: {
			SESSION_INFO *si = SM_FindSessionByHWND((HWND)lParam);

			if (si && IsWindow(si->hWnd)) {
				struct ContainerWindowData *pContainer = 0;

				SendMessage(si->hWnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
				if (pContainer) {
					int iTabs = TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, 1159));
					if (iTabs == 1)
						SendMessage(pContainer->hwnd, WM_CLOSE, 0, 1);
					else
						SendMessage(si->hWnd, WM_CLOSE, 0, 1);

					si->hWnd = CreateNewRoom((struct ContainerWindowData *)wParam, si, TRUE, 0, 0);
				}
			}
			break;
		}
#if defined(_UNICODE)
		case DM_SENDMESSAGECOMMANDW:
			SendMessageCommand_W(wParam, lParam);
			if (lParam)
				free((void *)lParam);
			return(0);
#endif
		case DM_SENDMESSAGECOMMAND:
			SendMessageCommand(wParam, lParam);
			if (lParam)
				free((void *)lParam);
			return(0);
			/*
			* sent from the popup to "dismiss" the event. we should do this in the main thread
			*/
		case DM_REMOVECLISTEVENT:
			CallService(MS_CLIST_REMOVEEVENT, wParam, lParam);
			CallService(MS_DB_EVENT_MARKREAD, wParam, lParam);
			return(0);

		case DM_SETLOCALE: {
			HKL 	hkl = (HKL)lParam;
			HANDLE 	hContact = (HANDLE)wParam;

			HWND	hWnd = M->FindWindow(hContact);

			if(hWnd) {
				_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				if(dat) {
					DBVARIANT  dbv;

					if(hkl) {
						dat->hkl = hkl;
						PostMessage(dat->hwnd, DM_SETLOCALE, 0, 0);
					}
					if(0 == M->GetTString(hContact, SRMSGMOD_T, "locale", &dbv)) {
						GetLocaleID(dat, dbv.ptszVal);
						DBFreeVariant(&dbv);
						UpdateReadChars(dat);
					}
				}
			}
			return(0);
		}

		case WM_DWMCOMPOSITIONCHANGED: {
			bool fNewAero = M->getAeroState();					// refresh dwm state
			SendMessage(hwndDlg, WM_THEMECHANGED, 0, 0);
			ContainerWindowData *pContainer = pFirstContainer;
			CSideBar::unInitBG();
			if(pContainer)
				CSideBar::initBG(pContainer->hwnd);

			while (pContainer) {
				if(fNewAero)
					SetAeroMargins(pContainer);
				else {
					MARGINS m = {0};

					if(M->m_pfnDwmExtendFrameIntoClientArea)
						M->m_pfnDwmExtendFrameIntoClientArea(pContainer->hwnd, &m);
				}
				if(pContainer->SideBar->isActive())
					RedrawWindow(GetDlgItem(pContainer->hwnd, 5000), NULL, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);			// the container for the sidebar buttons
				RedrawWindow(pContainer->hwnd, NULL, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
				pContainer = pContainer->pNextContainer;
			}
			M->BroadcastMessage(WM_DWMCOMPOSITIONCHANGED, 0, 0);
			break;
		}

		case WM_DWMCOLORIZATIONCOLORCHANGED: {
			M->getAeroState();
			Skin->setupAeroSkins();
			CSkin::initAeroEffect();
			break;
		}

		case WM_THEMECHANGED: {
			struct ContainerWindowData *pContainer = pFirstContainer;

			M->getAeroState();
			Skin->setupTabCloseBitmap();
			CSkin::initAeroEffect();
			PluginConfig.m_ncm.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &PluginConfig.m_ncm, 0);
			FreeTabConfig();
			ReloadTabConfig();
			while (pContainer) {
				SendMessage(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), EM_THEMECHANGED, 0, 0);
				BroadCastContainer(pContainer, EM_THEMECHANGED, 0, 0);
				pContainer = pContainer->pNextContainer;
			}
			break;
		}
		case DM_SPLITSENDACK: {
			ACKDATA ack = {0};
			struct SendJob *job = sendQueue->getJobByIndex((int)wParam);

			ack.hContact = job->hOwner;
			ack.hProcess = job->hSendId;
			ack.type = ACKTYPE_MESSAGE;
			ack.result = ACKRESULT_SUCCESS;

			if (job->hOwner && job->iAcksNeeded && job->hOwner && job->iStatus == SendQueue::SQ_INPROGRESS) {
				if (IsWindow(job->hwndOwner))
					::SendMessage(job->hwndOwner, HM_EVENTSENT, (WPARAM)MAKELONG(wParam, 0), (LPARAM)&ack);
				else
					sendQueue->ackMessage(0, (WPARAM)MAKELONG(wParam, 0), (LPARAM)&ack);
			}
			return 0;
		}

		case DM_LOGSTATUSCHANGE:
			CGlobals::logStatusChange(reinterpret_cast<CContactCache *>(lParam));
			return(0);

		case DM_MUCFLASHWORKER: {
			FLASH_PARAMS *p = reinterpret_cast<FLASH_PARAMS*>(lParam);
			DoFlashAndSoundWorker(p);
			return(0);
		}

		case WM_POWERBROADCAST:
		case WM_DISPLAYCHANGE: {
			struct ContainerWindowData *pContainer = pFirstContainer;

			while (pContainer) {
				if (CSkin::m_skinEnabled) {             // invalidate cached background DCs for skinned containers
					pContainer->oldDCSize.cx = pContainer->oldDCSize.cy = 0;
					SelectObject(pContainer->cachedDC, pContainer->oldHBM);
					DeleteObject(pContainer->cachedHBM);
					DeleteDC(pContainer->cachedDC);
					pContainer->cachedDC = 0;
					RedrawWindow(pContainer->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);
				}
				pContainer = pContainer->pNextContainer;
			}
			break;
		}

		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_ACTIVE)
				SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
			return 0;

		case WM_CLOSE:
			return 0;

		case WM_TIMER:
			if(wParam == TIMERID_SENDLATER) {
				/*
				 * send heartbeat to each open container (to manage autoclose
				 * feature)
				 */
				ContainerWindowData *pContainer = pFirstContainer;
				while(pContainer) {
					SendMessage(pContainer->hwnd, WM_TIMER, TIMERID_HEARTBEAT, 0);
					pContainer = pContainer->pNextContainer;
				}
				if(PluginConfig.m_SendLaterAvail && (time(0) - t_last_sendlater_processed) > SENDLATER_PROCESS_INTERVAL) {
					time_t now;
					t_last_sendlater_processed = now = time(0);

					/*
					 * check the list of contacts that may have new send later jobs
					 * (added on user's request)
					 */
					if(!sendLaterContactList.empty()) {
						std::vector<HANDLE>::iterator it = sendLaterContactList.begin();
						while(it != sendLaterContactList.end()) {
							SendLater_Process(*it);
							it++;
						}
						sendLaterContactList.clear();
					}

					/*
					 * start processing the job list
					 */
					if(!sendLaterJobList.empty()) {
						KillTimer(hwndDlg, wParam);
						g_jobs = sendLaterJobList.begin();
						SetTimer(hwndDlg, TIMERID_SENDLATER_TICK, TIMEOUT_SENDLATER_TICK, 0);
					}
				}
			}
			/*
			 * process one entry per tick (default: 200ms)
			 * TODO better timings, possibly slow down when many jobs are in the
			 * queue.
			 */
			else if(wParam == TIMERID_SENDLATER_TICK) {
				if(sendLaterJobList.empty() || g_jobs == sendLaterJobList.end()) {
					KillTimer(hwndDlg, wParam);
					SetTimer(hwndDlg, TIMERID_SENDLATER, TIMEOUT_SENDLATER, 0);
				}
				else {
					if((*g_jobs)->fSuccess || (*g_jobs)->fFailed) {
						//_DebugTraceA("Removing successful job %s", (*g_jobs)->szId);
						delete *g_jobs;
						sendLaterJobList.erase(g_jobs);
					}
					else {
						//_DebugTraceA("Sending job: %s", (*g_jobs)->szId);
						SendLater_SendIt(*g_jobs);
						g_jobs++;
					}
				}
			}
			break;

		case WM_DESTROY: {
			KillTimer(hwndDlg, TIMERID_SENDLATER);
			DestroyServiceFunction(hSvcHotkeyProcessor);
			break;
		}
	}
	return(DefWindowProc(hwndDlg, msg, wParam, lParam));
}

/*
 * send later functions
 */

void TSAPI SendLater_Add(const HANDLE hContact)
{
	if(!PluginConfig.m_SendLaterAvail)
		return;

	std::vector<HANDLE>::iterator it = sendLaterContactList.begin();

	if(sendLaterContactList.empty()) {
		sendLaterContactList.push_back(hContact);
		t_last_sendlater_processed = 0;								// force processing at next tick
		return;
	}

	/*
	 * this list should not have duplicate entries
	 */

	while(it != sendLaterContactList.end()) {
		if(*it == hContact)
			return;
		it++;
	}
	sendLaterContactList.push_back(hContact);
	t_last_sendlater_processed = 0;								// force processing at next tick
}

#if defined(_UNICODE)
	#define U_PREF_UNICODE PREF_UNICODE
#else
	#define U_PREF_UNICODE 0
#endif

/**
 * This function adds a new job to the list of messages to send unattended
 * used by the send later feature and multisend
 *
 * @param 	szSetting is either the name of the database key for a send later
 * 		  	job OR the utf-8 encoded message for a multisend job prefixed with
 * 			a 'M+timestamp'. Send later job ids start with "S".
 *
 * @param 	lParam: a contact handle for which the job should be scheduled
 * @return 	0 on failure, 1 otherwise
 */
int _cdecl SendLater_AddJob(const char *szSetting, LPARAM lParam)
{
	HANDLE 		hContact = (HANDLE)lParam;
	DBVARIANT 	dbv = {0};
	char		*szOrig_Utf = 0;

	if(!szSetting || !strcmp(szSetting, "count") || lstrlenA(szSetting) < 8)
		return(0);

	if(szSetting[0] != 'S' && szSetting[0] != 'M')
		return(0);

	SendLaterJobIterator it = sendLaterJobList.begin();

	/*
	 * check for possible dupes
	 */
	while(it != sendLaterJobList.end()) {
		if((*it)->hContact == hContact && !strcmp((*it)->szId, szSetting)) {
			//_DebugTraceA("%s for %d is already on the job list.", szSetting, hContact);
			return(0);
		}
		it++;
	}

	if(szSetting[0] == 'S') {
		if(0 == DBGetContactSettingString(hContact, "SendLater", szSetting, &dbv))
			szOrig_Utf = dbv.pszVal;
		else
			return(0);
	}
	else if(szSetting[0] == 'M') {
		char *szSep = strchr(const_cast<char *>(szSetting), '|');
		if(!szSep)
			return(0);
		*szSep = 0;
		szOrig_Utf = szSep + 1;
	}
	else
		return(0);

	SendLaterJob *job = new SendLaterJob;

	strncpy(job->szId, szSetting, 20);
	job->szId[19] = 0;
	job->hContact = hContact;
	job->created = atol(&szSetting[1]);

	char	*szAnsi = 0;
	wchar_t *szWchar = 0;
	UINT	required = 0;

	int iLen = lstrlenA(szOrig_Utf);
	job->sendBuffer = reinterpret_cast<char *>(mir_alloc(iLen + 1));
	strncpy(job->sendBuffer, szOrig_Utf, iLen);
	job->sendBuffer[iLen] = 0;

	/*
	 * construct conventional send buffer
	 */

	szAnsi = M->utf8_decodecp(szOrig_Utf, CP_ACP, &szWchar);
	iLen = lstrlenA(szAnsi);
	if(szWchar)
		required = iLen + 1 + ((lstrlenW(szWchar) + 1) * sizeof(wchar_t));
	else
		required = iLen + 1;

	job->pBuf = (PBYTE)mir_calloc(required);

	strncpy((char *)job->pBuf, szAnsi, iLen);
	job->pBuf[iLen] = 0;
	if(szWchar)
		wcsncpy((wchar_t *)&job->pBuf[iLen + 1], szWchar, lstrlenW(szWchar));

	else if(szSetting[0] == 'S')
		DBFreeVariant(&dbv);

	mir_free(szWchar);

	sendLaterJobList.push_back(job);

	return(1);
}

/**
 * Try to send an open job from the job list
 * this is ONLY called from the WM_TIMER handler and should never be executed
 * directly.
 */
static int TSAPI SendLater_SendIt(SendLaterJob *job)
{
	HANDLE 		hContact = job->hContact;
	time_t 		now = time(0);
	DWORD   	dwFlags = 0;
	DBVARIANT 	dbv = {0};
	const char* szProto = 0;


	if(job->fSuccess || job->fFailed || job->lastSent > now)
		return(0);											// this one is frozen or done (will be removed soon), don't process it now.

	if(now - job->created > SENDLATER_AGE_THRESHOLD) {		// too old, this will be discarded and user informed by popup
		job->fFailed = true;
		return(0);
	}

	if(job->iSendCount == 5) {
		job->iSendCount = 0;
		job->lastSent = now + 3600;							// after 5 unsuccessful resends, freeze it for an hour
		return(0);
	}

	if(job->iSendCount > 0 && (now - job->lastSent < SENDLATER_RESEND_THRESHOLD)) {
		//_DebugTraceA("Send it: message %s for %s RESEND but not old enough", job->szId, (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
		return(0);											// this one was sent, but probably failed. Resend it after a while
	}

	CContactCache *c = CContactCache::getContactCache(hContact);
	if(!c)
		return(0);

	hContact = c->getActiveContact();
	szProto = c->getActiveProto();

	if(!hContact || szProto == 0)
		return(0);

	WORD wMyStatus = (WORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	WORD wContactStatus = c->getActiveStatus();

	if(job->szId[0] == 'S') {
		if(!(wMyStatus == ID_STATUS_ONLINE || wMyStatus == ID_STATUS_FREECHAT)) {
			return(0);
		}
	}
	if(wContactStatus == ID_STATUS_OFFLINE)
		return(0);

	dwFlags = IsUtfSendAvailable(hContact) ? PREF_UTF : U_PREF_UNICODE;

	char *svcName = SendQueue::MsgServiceName(hContact, 0, dwFlags);

	job->lastSent = now;
	job->iSendCount++;
	job->hTargetContact = hContact;

	if(dwFlags & PREF_UTF)
		job->hProcess = (HANDLE)CallContactService(hContact, svcName, dwFlags, (LPARAM)job->sendBuffer);
	else
		job->hProcess = (HANDLE)CallContactService(hContact, svcName, dwFlags, (LPARAM)job->pBuf);
	return(0);
}
/**
 * Process a single contact from the list of contacts with open send later jobs
 * enum the "SendLater" module and add all jobs to the list of open jobs.
 * SendLater_AddJob() will deal with possible duplicates
 * @param hContact HANDLE: contact's handle
 */
static void TSAPI SendLater_Process(const HANDLE hContact)
{
	int iCount = M->GetDword(hContact, "SendLater", "count", 0);

	if(iCount) {
		DBCONTACTENUMSETTINGS ces = {0};

		ces.pfnEnumProc = SendLater_AddJob;
		ces.szModule = "SendLater";
		ces.lParam = (LPARAM)hContact;

		CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)hContact, (LPARAM)&ces);
	}
}

/**
 * process ACK messages for the send later job list. Called from the proto ack
 * handler when it does not find a match in the normal send queue
 *
 * Add the message to the database and mark it as successful. The job will be
 * removed later by the job list processing code.
 */
HANDLE TSAPI SendLater_ProcessAck(const ACKDATA *ack)
{
	if(sendLaterJobList.empty())
		return(0);

	SendLaterJobIterator it = sendLaterJobList.begin();

	while(it != sendLaterJobList.end()) {
		if((*it)->hProcess == ack->hProcess && (*it)->hTargetContact == ack->hContact && !((*it)->fSuccess || (*it)->fFailed)) {
			DBEVENTINFO dbei = {0};

			if(!(*it)->fSuccess) {
				//_DebugTraceA("ack for: hProcess: %d, job id: %s, job hProcess: %d", ack->hProcess, (*it)->szId, (*it)->hProcess);
				dbei.cbSize = sizeof(dbei);
				dbei.eventType = EVENTTYPE_MESSAGE;
				dbei.flags = DBEF_SENT;
				dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)((*it)->hContact), 0);
				dbei.timestamp = time(NULL);
				dbei.cbBlob = lstrlenA((*it)->sendBuffer) + 1;

	#if defined( _UNICODE )
				dbei.flags |= DBEF_UTF;
	#endif
				dbei.pBlob = (PBYTE)((*it)->sendBuffer);
				HANDLE hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM)((*it)->hContact), (LPARAM)&dbei);

				if((*it)->szId[0] == 'S') {
					DBDeleteContactSetting((*it)->hContact, "SendLater", (*it)->szId);
					int iCount = M->GetDword((*it)->hContact, "SendLater", "count", 0);
					if(iCount)
						iCount--;
					M->WriteDword((*it)->hContact, "SendLater", "count", iCount);
				}
			}
			(*it)->fSuccess = true;					// mark as successful, job list processing code will remove it later
			(*it)->hProcess = (HANDLE)-1;
			return(0);
		}
		it++;
	}
	return(0);
}

/**
 * clear all open send jobs. Only called on system shutdown to remove
 * the jobs from memory. Must _NOT_ delete any sendlater related stuff from
 * the database.
 */
void TSAPI SendLater_ClearAll()
{
	if(sendLaterJobList.empty())
		return;

	SendLaterJobIterator it = sendLaterJobList.begin();

	while(it != sendLaterJobList.end()) {
		mir_free((*it)->sendBuffer);
		mir_free((*it)->pBuf);
		(*it)->fSuccess = false;					// avoid clearing jobs from the database
		delete *it;
		it++;
	}
}

