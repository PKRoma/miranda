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

    b) event notify stuff, messages posted from the popups to avoid threading
       issues.

    c) tray icon handling - cares about the events sent by the tray icon, handles
       the menus, doubleclicks and so on.
 *
 */

#include "commonheaders.h"
#pragma hdrstop

extern HICON		hIcons[];
extern INT_PTR		SendMessageCommand(WPARAM wParam, LPARAM lParam);
extern INT_PTR		SendMessageCommand_W(WPARAM wParam, LPARAM lParam);

static UINT 	WM_TASKBARCREATED;
static HANDLE 	hSvcHotkeyProcessor = 0;

#define TIMERID_SENDLATER 12000

static HOTKEYDESC _hotkeydescs[] = {
	0, "tabsrmm_mostrecent", "Most recent unread session", TABSRMM_HK_SECTION_IM, MS_TABMSG_HOTKEYPROCESS, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'R'), TABSRMM_HK_LASTUNREAD,
	0, "tabsrmm_paste_and_send", "Paste and send", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'D'), TABSRMM_HK_PASTEANDSEND,
	0, "tabsrmm_uprefs", "Contact's messaging prefs", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'C'), TABSRMM_HK_SETUSERPREFS,
	0, "tabsrmm_copts", "Container options", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_CONTROL, 'O'), TABSRMM_HK_CONTAINEROPTIONS,
	0, "tabsrmm_nudge", "Send nudge", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_CONTROL, 'N'), TABSRMM_HK_NUDGE,
	0, "tabsrmm_sendfile", "Send a file", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'F'), TABSRMM_HK_SENDFILE,
	0, "tabsrmm_quote", "Quote message", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'Q'), TABSRMM_HK_QUOTEMSG,
	0, "tabsrmm_sendlater", "Toggle send later", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_ALT, 'S'), TABSRMM_HK_TOGGLESENDLATER,

	0, "tabsrmm_send", "Send message", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT, 'S'), TABSRMM_HK_SEND,
	0, "tabsrmm_emot", "Smiley selector", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT, 'E'), TABSRMM_HK_EMOTICONS,
	0, "tabsrmm_hist", "Show message history", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT, 'H'), TABSRMM_HK_HISTORY,
	0, "tabsrmm_umenu", "Show user menu", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'D'), TABSRMM_HK_USERMENU,
	0, "tabsrmm_udet", "Show user details", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'U'), TABSRMM_HK_USERDETAILS,
	0, "tabsrmm_tbar", "Toggle tool bar", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_SHIFT, 'T'), TABSRMM_HK_TOGGLETOOLBAR,
	0, "tabsrmm_ipanel", "Toggle info panel", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_CONTROL, 'I'), TABSRMM_HK_TOGGLEINFOPANEL,
	0, "tabsrmm_rtl", "Toggle text direction", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_CONTROL, 'B'), TABSRMM_HK_TOGGLERTL,
	0, "tabsrmm_msend", "Toggle multi send", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT|HOTKEYF_CONTROL, 'M'), TABSRMM_HK_TOGGLEMULTISEND,
	0, "tabsrmm_clearlog", "Clear message log", TABSRMM_HK_SECTION_GENERIC, 0, HOTKEYCODE(HOTKEYF_CONTROL, 'L'), TABSRMM_HK_CLEARLOG,
	0, "tabsrmm_notes", "Edit user notes", TABSRMM_HK_SECTION_IM, 0, HOTKEYCODE(HOTKEYF_ALT, 'N'), TABSRMM_HK_EDITNOTES
};

std::vector<HANDLE> sendLaterContactList;
std::vector<SendLaterJob *> sendLaterJobList;

typedef std::vector<SendLaterJob *>::iterator SendLaterJobIterator;

#define SENDLATER_AGE_THRESHOLD (86400 * 3)				// 3 days, older messages will be removed from the db.
#define SENDLATER_RESEND_THRESHOLD 180					// timeouted messages should be resent after that many seconds

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

void HandleMenuEntryFromhContact(int iSelection)
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

static void DrawMenuItem(DRAWITEMSTRUCT *dis, HICON hIcon, DWORD dwIdle)
{
	int cx = PluginConfig.m_smcxicon;
	int cy = PluginConfig.m_smcyicon;

	if (PluginConfig.m_bIsXP) {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
		if (dis->itemState & ODS_HOTLIGHT)
			DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
		else if (dis->itemState & ODS_SELECTED)
			DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);
		if (dwIdle) {
			CSkin::DrawDimmedIcon(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - cy) / 2, 16, 16, hIcon, 180);
		} else
			DrawIconEx(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - cy) / 2, hIcon, cx, cy, 0, 0, DI_NORMAL | DI_COMPAT);
	} else {
		BOOL bfm = FALSE;
		SystemParametersInfo(SPI_GETFLATMENU, 0, &bfm, 0);
		if (bfm) {
			/* flat menus: fill with COLOR_MENUHILIGHT and outline with COLOR_HIGHLIGHT, otherwise use COLOR_MENUBAR */
			if (dis->itemState & ODS_SELECTED || dis->itemState & ODS_HOTLIGHT) {
				/* selected or hot lighted, no difference */
				FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENUHILIGHT));
				/* draw the frame */
				FrameRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
			} else {
				/* flush the DC with the menu bar colour (only supported on XP) and then draw the icon */
				FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENUBAR));
			}   //if
			/* draw the icon */
			if (dwIdle) {
				CSkin::DrawDimmedIcon(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - cy) / 2, 16, 16, hIcon, 180);
			} else
				DrawIconEx(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - cy) / 2, hIcon, cx, cy, 0, 0, DI_NORMAL | DI_COMPAT);
		} else {
			/* non-flat menus, flush the DC with a normal menu colour */
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
			if (dis->itemState & ODS_HOTLIGHT) {
				DrawEdge(dis->hDC, &dis->rcItem, BDR_RAISEDINNER, BF_RECT);
			} else if (dis->itemState & ODS_SELECTED) {
				DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENOUTER, BF_RECT);
			}
			if (dwIdle) {
				CSkin::DrawDimmedIcon(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - cy) / 2, 16, 16, hIcon, 180);
			} else
				DrawIconEx(dis->hDC, 2, (dis->rcItem.bottom + dis->rcItem.top - cy) / 2, hIcon, cx, cy, 0, 0, DI_NORMAL | DI_COMPAT);
		}       //if
	}           //if
}

INT_PTR CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
		case WM_INITDIALOG:
			int i;

			for(i = 0; i < safe_sizeof(_hotkeydescs); i++) {
				_hotkeydescs[i].cbSize = sizeof(HOTKEYDESC);
				_hotkeydescs[i].pszSection = Translate(_hotkeydescs[i].pszSection);
				_hotkeydescs[i].pszDescription = Translate(_hotkeydescs[i].pszDescription);
				CallService(MS_HOTKEY_REGISTER, 0, (LPARAM)&_hotkeydescs[i]);
			}
			sendLaterContactList.clear();

			WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");
			ShowWindow(hwndDlg, SW_HIDE);
			hSvcHotkeyProcessor = CreateServiceFunction(MS_TABMSG_HOTKEYPROCESS, HotkeyProcessor);
			SetTimer(hwndDlg, TIMERID_SENDLATER, 60000, NULL);
			return TRUE;
		case WM_LBUTTONDOWN:
			iMousedown = 1;
			GetCursorPos(&ptLast);
			SetCapture(hwndDlg);
			break;
		case WM_LBUTTONUP: {
			iMousedown = 0;
			ReleaseCapture();
			break;
		}
		case WM_MOUSEMOVE: {
			RECT rc;
			POINT pt;

			if (iMousedown) {
				GetWindowRect(hwndDlg, &rc);
				GetCursorPos(&pt);
				MoveWindow(hwndDlg, rc.left - (ptLast.x - pt.x), rc.top - (ptLast.y - pt.y), rc.right - rc.left, rc.bottom - rc.top, TRUE);
				ptLast = pt;
			}
			break;
		}
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
						fNeedFree = TRUE;
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
						if (PluginConfig.m_WinVerMajor < 5)
							break;
						if (PluginConfig.m_TipOwner != 0)
							break;
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

						if (PluginConfig.m_WinVerMajor < 5)
							break;

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
					case NIN_BALLOONUSERCLICK: {
						HandleMenuEntryFromhContact((int)PluginConfig.m_TipOwner);
						break;
					}
					case NIN_BALLOONHIDE:
					case NIN_BALLOONTIMEOUT:
						PluginConfig.m_TipOwner = 0;
						break;
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
			return 0;
#endif
		case DM_SENDMESSAGECOMMAND:
			SendMessageCommand(wParam, lParam);
			if (lParam)
				free((void *)lParam);
			return 0;
			/*
			* sent from the popup to "dismiss" the event. we should do this in the main thread
			*/
		case DM_REMOVECLISTEVENT:
			CallService(MS_CLIST_REMOVEEVENT, wParam, lParam);
			CallService(MS_DB_EVENT_MARKREAD, wParam, lParam);
			break;

		case DM_SETLOCALE: {
			HKL 	hkl = (HKL)lParam;
			HANDLE 	hContact = (HANDLE)wParam;

			HWND	hWnd = M->FindWindow(hContact);

			if(hWnd) {
				_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				if(dat) {
					DBVARIANT  dbv;

					if(hkl)
						ActivateKeyboardLayout(hkl, 0);
					if(0 == DBGetContactSettingString(hContact, SRMSGMOD_T, "locale", &dbv)) {
						dat->hkl = hkl;
						GetLocaleID(dat, dbv.pszVal);
						PostMessage(dat->hwnd, DM_SETLOCALE, 0, 0);
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

			ContainerWindowData *pContainer = pFirstContainer;

			while (pContainer) {
				InvalidateRect(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), NULL, FALSE);
				pContainer = pContainer->pNextContainer;
			}
			break;
		}

		case WM_THEMECHANGED: {
			struct ContainerWindowData *pContainer = pFirstContainer;

			M->getAeroState();
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

			ack.hContact = job->hContact[0];
			ack.hProcess = job->hSendId[0];
			ack.type = ACKTYPE_MESSAGE;
			ack.result = ACKRESULT_SUCCESS;

			if (job->hOwner && job->iAcksNeeded && job->hContact[0] && job->iStatus == SendQueue::SQ_INPROGRESS) {
				if (IsWindow(job->hwndOwner))
					::SendMessage(job->hwndOwner, HM_EVENTSENT, (WPARAM)MAKELONG(wParam, 0), (LPARAM)&ack);
				else
					sendQueue->ackMessage(0, (WPARAM)MAKELONG(wParam, 0), (LPARAM)&ack);
			}
			return 0;
		}
		case WM_POWERBROADCAST:
		case WM_DISPLAYCHANGE: {
			struct ContainerWindowData *pContainer = pFirstContainer;

			while (pContainer) {
				if (pContainer->bSkinned) {             // invalidate cached background DCs for skinned containers
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

		case DM_REMOVEFROMSENDLATER:
			SendLater_Remove((HANDLE)wParam);
			return(0);

		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_ACTIVE)
				SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
			return 0;

		case WM_CLOSE:
			return 0;

		case WM_TIMER:
			if(wParam == TIMERID_SENDLATER && PluginConfig.m_SendLaterAvail) {
				std::vector<HANDLE>::iterator it = sendLaterContactList.begin();
				DWORD  now = time(0);

				while(it != sendLaterContactList.end()) {
					SendLater_Process(*it);
					break;
					it++;
				}
			}
			break;

		case WM_DESTROY: {
			KillTimer(hwndDlg, TIMERID_SENDLATER);
			DestroyServiceFunction(hSvcHotkeyProcessor);
			break;
		}
	}
	return FALSE;
}

/*
 * send later functions
 */

void SendLater_Add(const HANDLE hContact)
{
	if(!PluginConfig.m_SendLaterAvail)
		return;

	std::vector<HANDLE>::iterator it = sendLaterContactList.begin();

	if(sendLaterContactList.empty()) {
		sendLaterContactList.push_back(hContact);
		return;
	}

	/*
	 * this list should not have duplicate entries
	 */

	while(it != sendLaterContactList.end()) {
		if(*it == hContact) {
			//_DebugTraceA("%d already in list", hContact);
			return;
		}
		it++;
	}
	sendLaterContactList.push_back(hContact);
}

static void SendLater_Remove(const HANDLE hContact)
{
	std::vector<HANDLE>::iterator it = sendLaterContactList.begin();

	while(it != sendLaterContactList.end()) {
		if(*it == hContact) {
			//_DebugTraceA("%d deleted from list", hContact);
			sendLaterContactList.erase(it);
			return;
		}
		it++;
	}
}

#if defined(_UNICODE)
	#define U_PREF_UNICODE PREF_UNICODE
#else
	#define U_PREF_UNICODE 0
#endif

static int SendLater_SendIt(const char *szSetting, LPARAM lParam)
{
	HANDLE 	hContact = (HANDLE)lParam;
	HANDLE  hOrigContact = hContact;
	DWORD 	dwTimeStamp;
	DWORD 	now = time(0);
	TCHAR   *msgToSend = 0;
	bool	fResend = false;;

	if(!szSetting || !strcmp(szSetting, "count") || lstrlenA(szSetting) < 8)
		return(0);

	sscanf(szSetting, "%d", &dwTimeStamp);

	std::vector<SendLaterJob *>::const_iterator it = sendLaterJobList.begin();

	//_DebugTraceA("checking for %d", hContact);

	if(!sendLaterJobList.empty()) {
		while(it != sendLaterJobList.end()) {
			if((*it)->hOrigContact == hOrigContact && (*it)->dwOriginalId == dwTimeStamp) {
				if((*it)->dwTime < (now - SENDLATER_RESEND_THRESHOLD)) {
					//_DebugTraceA("the message %d for %d is already in the list and ready for resend", dwTimeStamp, hContact);
					(*it)->dwTime = now;
					fResend = true;
					hOrigContact = (*it)->hOrigContact;
					hContact = hOrigContact;
					dwTimeStamp = (*it)->dwOriginalId;
					mir_free((*it)->sendBuffer);
					(*it)->sendBuffer = 0;
					break;
				}
				else {
					//_DebugTraceA("the message %d for %d is already in the list and NOT ready for resend", dwTimeStamp, hContact);
					return(0);
				}
			}
			it++;
		}
	}

	if(!fResend) {
		if(dwTimeStamp > now || dwTimeStamp < (now - SENDLATER_AGE_THRESHOLD)) {
			// most likely invalid, remove it
			return(0);
		}
	}

	char	*szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	DWORD   dwFlags = 0;
	DBVARIANT dbv = {0};

	bool	fIsMeta = (PluginConfig.g_MetaContactsAvail && !strcmp(szProto, PluginConfig.szMetaName)) ? true : false;

	if(fIsMeta) {
		hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)hContact, 0);
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	}

	if(!hContact || szProto == 0) {
		//_DebugTraceA("contact handle or protocol invalid", szSetting);
		return(0);
	}

	WORD wMyStatus = (WORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	WORD wContactStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);

	if(!(wMyStatus == ID_STATUS_ONLINE || wMyStatus == ID_STATUS_FREECHAT)) {
		//_DebugTraceA("My status does not allow to send a queued message");
		return(0);
	}
	if(wContactStatus == ID_STATUS_OFFLINE) {
		//_DebugTraceA("Contact is not online, sending not possible");
		return(0);
	}

	dwFlags = IsUtfSendAvailable(hContact) ? PREF_UTF : U_PREF_UNICODE;

	char *svcName = SendQueue::MsgServiceName(hContact, 0, dwFlags);

	if(0 == DBGetContactSettingString(hOrigContact, "SendLater", szSetting, &dbv)) {
		TCHAR tszTimestamp[30], tszHeader[150];

		TCHAR *formatTime = _T("%Y.%m.%d - %H:%M");
		_tcsftime(tszTimestamp, 30, formatTime, _localtime32((__time32_t *)&dwTimeStamp));
		tszTimestamp[29] = 0;

		mir_sntprintf(tszHeader, safe_sizeof(tszHeader), CTranslator::get(CTranslator::GEN_SQ_SENDLATER_HEADER), tszTimestamp);

		TCHAR *msg = M->utf8_decodeT((char *)dbv.pszVal);
		mir_free(dbv.pszVal);

		UINT memRequired = (lstrlen(tszHeader) + lstrlen(msg) + 4) * sizeof(TCHAR);

		TCHAR *finalMsg = (TCHAR *)malloc(memRequired);
		mir_sntprintf(finalMsg, memRequired, _T("%s%s"), tszHeader, msg);

		mir_free(msg);

		/*
		 * construct the message for sending
		 */

		if(dwFlags & PREF_UTF)
			msgToSend = reinterpret_cast<TCHAR *>(M->utf8_encodeT(finalMsg));
#if defined(_UNICODE)
		else if(dwFlags & PREF_UNICODE) {
			int iLength = lstrlen(finalMsg);
			memRequired = (lstrlen(finalMsg) + 3) * (sizeof(TCHAR) + 1);
			msgToSend = (TCHAR *)mir_alloc(memRequired);
			mir_sntprintf(msgToSend, lstrlen(finalMsg), _T("%s"), finalMsg);
			WideCharToMultiByte(0, 0, finalMsg, iLength, (char *)&msgToSend[iLength + 1], iLength, 0, 0);
		}
#endif
		else
			msgToSend = reinterpret_cast<TCHAR *>(mir_tstrdup(finalMsg));

		SendLaterJob *job = 0;

		if(fResend)
			job = *it;
		else
			job = new SendLaterJob;

		job->dwOriginalId = dwTimeStamp;
		job->dwTime = now;
		job->hContact = hContact;
		job->hProcess = (HANDLE)CallContactService(hContact, svcName, (WPARAM)dwFlags, (LPARAM)msgToSend);
		job->hOrigContact = hOrigContact;
		job->dwFlags = dwFlags;
		job->sendBuffer = reinterpret_cast<char *>(msgToSend);
		if(!fResend)
			sendLaterJobList.push_back(job);

		free(finalMsg);
	}
	return(0);
}
/**
 * Process a single contact
 *
 * @param hContact HANDLE: contact's handle
 */
static void SendLater_Process(const HANDLE hContact)
{
	int iCount = M->GetDword(hContact, "SendLater", "count", 0);

	if(iCount) {
		DBCONTACTENUMSETTINGS ces = {0};

		ces.pfnEnumProc = SendLater_SendIt;
		ces.szModule = "SendLater";
		ces.lParam = (LPARAM)hContact;

		CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)hContact, (LPARAM)&ces);
	}
}

HANDLE SendLater_ProcessAck(const ACKDATA *ack)
{
	if(sendLaterJobList.empty())
		return(0);

	SendLaterJobIterator it = sendLaterJobList.begin();

	while(it != sendLaterJobList.end()) {
		if((*it)->hProcess == ack->hProcess) {
			char	szSetting[50];
			mir_snprintf(szSetting, 50, "%d", (*it)->dwOriginalId);
			DBDeleteContactSetting((*it)->hOrigContact, "SendLater", szSetting);
			int iCount = M->GetDword((*it)->hOrigContact, "SendLater", "count", 0);
			if(iCount)
				iCount--;
			M->WriteDword((*it)->hOrigContact, "SendLater", "count", iCount);

			if(iCount == 0)
				SendLater_Remove((*it)->hOrigContact);

			DBEVENTINFO dbei = {0};
			dbei.cbSize = sizeof(dbei);
			dbei.eventType = EVENTTYPE_MESSAGE;
			dbei.flags = DBEF_SENT;
			dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)((*it)->hOrigContact), 0);
			dbei.timestamp = time(NULL);
			dbei.cbBlob = lstrlenA((*it)->sendBuffer) + 1;

#if defined( _UNICODE )
			if ((*it)->dwFlags & PREF_UNICODE)
				dbei.cbBlob *= sizeof(TCHAR) + 1;
			if ((*it)->dwFlags & PREF_RTL)
				dbei.flags |= DBEF_RTL;
			if ((*it)->dwFlags & PREF_UTF)
				dbei.flags |= DBEF_UTF;
#endif
			dbei.pBlob = (PBYTE)((*it)->sendBuffer);
			HANDLE hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM)((*it)->hOrigContact), (LPARAM)&dbei);

			mir_free((*it)->sendBuffer);
			delete *it;
			sendLaterJobList.erase(it);
			return(ack->hProcess);
		}
		it++;
	}
	return(0);
}

void SendLater_ClearAll()
{
	if(sendLaterJobList.empty())
		return;

	SendLaterJobIterator it = sendLaterJobList.begin();

	while(it != sendLaterJobList.end()) {
		delete *it;
		it++;
	}
}

