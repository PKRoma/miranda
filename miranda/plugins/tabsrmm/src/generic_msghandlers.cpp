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
 * these are generic message handlers which are used by the message dialog window procedure.
 * calling them directly instead of using SendMessage() is faster.
 * also contains various callback functions for custom buttons
 */


#include "commonheaders.h"

extern RECT	  			rcLastStatusBarClick;
extern RTFColorTable*	rtf_ctable;

/*
* action and callback procedures for the stock button objects
*/

static void BTN_StockAction(ButtonItem *item, HWND hwndDlg, struct _MessageWindowData *dat, HWND hwndBtn)
{
	if (item->dwStockFlags & SBI_HANDLEBYCLIENT && IsWindow(hwndDlg) && dat)
		SendMessage(hwndDlg, WM_COMMAND, MAKELONG(item->uId, BN_CLICKED), (LPARAM)hwndBtn);
	else {
		switch (item->uId) {
			case IDC_SBAR_CANCEL:
				PostMessage(hwndDlg, WM_COMMAND, MAKELONG(IDC_SAVE, BN_CLICKED), (LPARAM)hwndBtn);
				break;
			case IDC_SBAR_SLIST:
				SendMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_LBUTTONUP);
				break;
			case IDC_SBAR_FAVORITES: {
				POINT pt;
				int iSelection;
				GetCursorPos(&pt);
				iSelection = TrackPopupMenu(PluginConfig.g_hMenuFavorites, TPM_RETURNCMD, pt.x, pt.y, 0, PluginConfig.g_hwndHotkeyHandler, NULL);
				HandleMenuEntryFromhContact(iSelection);
				break;
			}
			case IDC_SBAR_RECENT: {
				POINT pt;
				int iSelection;
				GetCursorPos(&pt);
				iSelection = TrackPopupMenu(PluginConfig.g_hMenuRecent, TPM_RETURNCMD, pt.x, pt.y, 0, PluginConfig.g_hwndHotkeyHandler, NULL);
				HandleMenuEntryFromhContact(iSelection);
				break;
			}
			case IDC_SBAR_USERPREFS: {
				HANDLE hContact = 0;
				SendMessage(hwndDlg, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
				if (hContact != 0)
					CallService(MS_TABMSG_SETUSERPREFS, (WPARAM)hContact, 0);
				break;
			}
			case IDC_SBAR_TOGGLEFORMAT: {
				if (dat) {
					if (IsDlgButtonChecked(hwndDlg, IDC_SBAR_TOGGLEFORMAT) == BST_UNCHECKED) {
						dat->SendFormat = 0;
						GetSendFormat(hwndDlg, dat, 0);
					} else {
						dat->SendFormat = SENDFORMAT_BBCODE;
						GetSendFormat(hwndDlg, dat, 0);
					}
				}
				break;
			}
		}
	}
}

static void BTN_StockCallback(ButtonItem *item, HWND hwndDlg, struct _MessageWindowData *dat, HWND hwndBtn)
{
}

/*
* predefined button objects for customizeable buttons
*/

static struct SIDEBARITEM sbarItems[] = {
	0, 0, 0, 0, 0, _T(""), NULL, NULL, _T("")
};

int BTN_GetStockItem(ButtonItem *item, const TCHAR *szName)
{
	int i = 0;

	while (sbarItems[i].uId) {
		if (!_tcsicmp(sbarItems[i].szName, szName)) {
			item->uId = sbarItems[i].uId;
			//item->dwFlags |= BUTTON_ISSIDEBAR;
			//myGlobals.m_SideBarEnabled = TRUE;
			if (item->dwFlags & BUTTON_ISSIDEBAR) {
				if (sbarItems[i].dwFlags & SBI_TOP)
					item->yOff = 0;
				else if (sbarItems[i].dwFlags & SBI_BOTTOM)
					item->yOff = -1;
			}
			item->dwStockFlags = sbarItems[i].dwFlags;
			item->dwFlags = sbarItems[i].dwFlags & SBI_TOGGLE ? item->dwFlags | BUTTON_ISTOGGLE : item->dwFlags & ~BUTTON_ISTOGGLE;
			item->pfnAction = sbarItems[i].pfnAction;
			item->pfnCallback = sbarItems[i].pfnCallback;
			lstrcpyn(item->szTip, sbarItems[i].tszTip, 256);
			item->szTip[255] = 0;
			if (sbarItems[i].hIcon) {
				item->normalGlyphMetrics[0] = (LONG_PTR)sbarItems[i].hIcon;
				item->dwFlags |= BUTTON_NORMALGLYPHISICON;
			}
			if (sbarItems[i].hIconPressed) {
				item->pressedGlyphMetrics[0] = (LONG_PTR)sbarItems[i].hIconPressed;
				item->dwFlags |= BUTTON_PRESSEDGLYPHISICON;
			}
			if (sbarItems[i].hIconHover) {
				item->hoverGlyphMetrics[0] = (LONG_PTR)sbarItems[i].hIconHover;
				item->dwFlags |= BUTTON_HOVERGLYPHISICON;
			}
			return 1;
		}
		i++;
	}
	return 0;
}

/*
* set the states of defined database action buttons (only if button is a toggle)
*/

void DM_SetDBButtonStates(HWND hwndChild, struct _MessageWindowData *dat)
{
	ButtonItem *buttonItem = dat->pContainer->buttonItems;
	HANDLE hContact = dat->hContact, hFinalContact = 0;
	char *szModule, *szSetting;
	HWND hwndContainer = dat->pContainer->hwnd;;

	while (buttonItem) {
		BOOL result = FALSE;
		HWND hWnd = GetDlgItem(hwndContainer, buttonItem->uId);

		if (buttonItem->pfnCallback)
			buttonItem->pfnCallback(buttonItem, hwndChild, dat, hWnd);

		if (!(buttonItem->dwFlags & BUTTON_ISTOGGLE && buttonItem->dwFlags & BUTTON_ISDBACTION)) {
			buttonItem = buttonItem->nextItem;
			continue;
		}
		szModule = buttonItem->szModule;
		szSetting = buttonItem->szSetting;
		if (buttonItem->dwFlags & BUTTON_DBACTIONONCONTACT || buttonItem->dwFlags & BUTTON_ISCONTACTDBACTION) {
			if (hContact == 0) {
				SendMessage(hWnd, BM_SETCHECK, BST_UNCHECKED, 0);
				buttonItem = buttonItem->nextItem;
				continue;
			}
			if (buttonItem->dwFlags & BUTTON_ISCONTACTDBACTION)
				szModule = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			hFinalContact = hContact;
		} else
			hFinalContact = 0;

		if (buttonItem->type == DBVT_ASCIIZ) {
			DBVARIANT dbv = {0};

			if (!DBGetContactSettingString(hFinalContact, szModule, szSetting, &dbv)) {
				result = !strcmp((char *)buttonItem->bValuePush, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
		} else {
			switch (buttonItem->type) {
				case DBVT_BYTE: {
					BYTE val = M->GetByte(hFinalContact, szModule, szSetting, 0);
					result = (val == buttonItem->bValuePush[0]);
					break;
				}
				case DBVT_WORD: {
					WORD val = DBGetContactSettingWord(hFinalContact, szModule, szSetting, 0);
					result = (val == *((WORD *) & buttonItem->bValuePush));
					break;
				}
				case DBVT_DWORD: {
					DWORD val = M->GetDword(hFinalContact, szModule, szSetting, 0);
					result = (val == *((DWORD *) & buttonItem->bValuePush));
					break;
				}
			}
		}
		SendMessage(hWnd, BM_SETCHECK, (WPARAM)result, 0);
		buttonItem = buttonItem->nextItem;
	}
}

LRESULT DM_ScrollToBottom(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	SCROLLINFO si = { 0 };

	if (dat == 0)
		return(0);

	if (dat) {

		if (dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED)
			return 0;

		if (IsIconic(dat->pContainer->hwnd))
			dat->dwFlags |= MWF_DEFERREDSCROLL;

		if (dat->hwndIEView) {
			PostMessage(dat->hwnd, DM_SCROLLIEVIEW, 0, 0);
			return 0;
		} else if (dat->hwndHPP) {
			SendMessage(dat->hwnd, DM_SCROLLIEVIEW, 0, 0);
			return 0;
		} else {
			HWND hwnd = GetDlgItem(dat->hwnd, dat->bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG);

			if (lParam)
				SendMessage(hwnd, WM_SIZE, 0, 0);

			if (wParam == 1 && lParam == 1) {
				RECT rc;
				int len;

				GetClientRect(hwnd, &rc);
				len = GetWindowTextLengthA(hwnd);
				SendMessage(hwnd, EM_SETSEL, len - 1, len - 1);
			}
			if (wParam)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			else
				PostMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			if (lParam)
				InvalidateRect(hwnd, NULL, FALSE);
		}
	}
	return 0;
}

static unsigned __stdcall LoadKLThread(LPVOID vParam)
{
	HANDLE 		hContact = reinterpret_cast<HANDLE>(vParam);
	DBVARIANT 	dbv = {0};

	LRESULT res = DBGetContactSettingString(hContact, SRMSGMOD_T, "locale", &dbv);
	if (res == 0) {
		HKL hkl = LoadKeyboardLayoutA(dbv.pszVal, KLF_SETFORPROCESS | KLF_ACTIVATE);//KLF_ACTIVATE);
		PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SETLOCALE, (WPARAM)hContact, (LPARAM)hkl);
		DBFreeVariant(&dbv);
	}
	return(0);
}

LRESULT DM_LoadLocale(_MessageWindowData *dat)
{
	/*
	* set locale if saved to contact
	*/

	if (dat) {
		if (dat->dwFlags & MWF_WASBACKGROUNDCREATE)
			return 0;

		if (PluginConfig.m_AutoLocaleSupport && dat->hContact != 0) {
			DBVARIANT dbv;
			int res;
			char szKLName[KL_NAMELENGTH+1];
			UINT flags = KLF_ACTIVATE;

			res = DBGetContactSettingString(dat->hContact, SRMSGMOD_T, "locale", &dbv);
			if (res == 0) {
				DBFreeVariant(&dbv);
				CloseHandle((HANDLE)mir_forkthreadex(LoadKLThread, reinterpret_cast<void *>(dat->hContact), 16000, NULL));
			} else {
				GetKeyboardLayoutNameA(szKLName);
				dat->hkl = LoadKeyboardLayoutA(szKLName, 0);
				DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
				GetLocaleID(dat, szKLName);
				UpdateReadChars(dat);
			}
		}
	}
	return 0;
}

LRESULT __stdcall DM_RecalcPictureSize(_MessageWindowData *dat)
{
	BITMAP bminfo;
	HBITMAP hbm;

	if (dat) {
		hbm = ((dat->Panel->isActive()) && PluginConfig.m_AvatarMode != 5) ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : PluginConfig.g_hbmUnknown);

		if (hbm == 0) {
			dat->pic.cy = dat->pic.cx = 60;
			return 0;
		}
		GetObject(hbm, sizeof(bminfo), &bminfo);
		CalcDynamicAvatarSize(dat, &bminfo);
		SendMessage(dat->hwnd, WM_SIZE, 0, 0);
	}
	return 0;
}

LRESULT DM_UpdateLastMessage(const _MessageWindowData *dat)
{
	if (dat) {
		if (dat->pContainer->hwndStatus == 0)
			return 0;
		if (dat->pContainer->hwndActive != dat->hwnd)
			return 0;
		if (dat->showTyping) {
			TCHAR szBuf[80];

			mir_sntprintf(szBuf, safe_sizeof(szBuf), CTranslator::get(CTranslator::GEN_MTN_STARTWITHNICK), dat->szNickname);
			SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM) szBuf);
			SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) PluginConfig.g_buttonBarIcons[5]);
			return 0;
		}
		if (dat->lastMessage || dat->pContainer->dwFlags & CNT_UINSTATUSBAR) {
			DBTIMETOSTRINGT dbtts;
			TCHAR date[64], time[64];

			if (!(dat->pContainer->dwFlags & CNT_UINSTATUSBAR)) {
				dbtts.szFormat = _T("d");
				dbtts.cbDest = safe_sizeof(date);
				dbtts.szDest = date;
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dat->lastMessage, (LPARAM) & dbtts);
				if (dat->pContainer->dwFlags & CNT_UINSTATUSBAR && lstrlen(date) > 6)
					date[lstrlen(date) - 5] = 0;
				dbtts.szFormat = _T("t");
				dbtts.cbDest = safe_sizeof(time);
				dbtts.szDest = time;
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dat->lastMessage, (LPARAM) & dbtts);
			}
			if (dat->pContainer->dwFlags & CNT_UINSTATUSBAR) {
				TCHAR fmt[100];
				TCHAR *uidName = _T("UIN");
				mir_sntprintf(fmt, safe_sizeof(fmt), _T("%s: %s"), uidName, dat->uin);
				SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM)fmt);
			} else {
				TCHAR fmt[100];
				mir_sntprintf(fmt, safe_sizeof(fmt), CTranslator::get(CTranslator::GEN_SBAR_LASTRECEIVED), date, time);
				SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM) fmt);
			}
		} else
			SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) "");
	}
	return 0;
}

/*
* save current keyboard layout for the given contact
*/

LRESULT DM_SaveLocale(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	if (dat) {
		if (PluginConfig.m_AutoLocaleSupport && dat->hContact && dat->pContainer->hwndActive == dat->hwnd) {
			char szKLName[KL_NAMELENGTH + 1];
			if ((HKL)lParam != dat->hkl) {
				dat->hkl = (HKL)lParam;
				ActivateKeyboardLayout(dat->hkl, 0);
				GetKeyboardLayoutNameA(szKLName);
				DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
				GetLocaleID(dat, szKLName);
				UpdateReadChars(dat);
			}
		}
	}
	return 0;
}

/*
* generic handler for the WM_COPY message in message log/chat history richedit control(s).
* it filters out the invisible event boundary markers from the text copied to the clipboard.
*/

LRESULT DM_WMCopyHandler(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = CallWindowProc(oldWndProc, hwnd, WM_COPY, wParam, lParam);

	if (OpenClipboard(hwnd)) {
#if defined(_UNICODE)
		HANDLE hClip = GetClipboardData(CF_UNICODETEXT);
#else
		HANDLE hClip = GetClipboardData(CF_TEXT);
#endif
		if (hClip) {
			HGLOBAL hgbl;
			TCHAR *tszLocked;
			TCHAR *tszText = (TCHAR *)malloc((lstrlen((TCHAR *)hClip) + 2) * sizeof(TCHAR));

			lstrcpy(tszText, (TCHAR *)hClip);
			FilterEventMarkers(tszText);
			EmptyClipboard();

			hgbl = GlobalAlloc(GMEM_MOVEABLE, (lstrlen(tszText) + 1) * sizeof(TCHAR));
			tszLocked = (TCHAR *)GlobalLock(hgbl);
			lstrcpy(tszLocked, tszText);
			GlobalUnlock(hgbl);
#if defined(_UNICODE)
			SetClipboardData(CF_UNICODETEXT, hgbl);
#else
			SetClipboardData(CF_TEXT, hgbl);
#endif
			if (tszText)
				free(tszText);
		}
		CloseClipboard();
	}
	return result;
}

/*
* create embedded contact list control
*/

HWND DM_CreateClist(HWND hwndParent, struct _MessageWindowData *dat)
{
	HWND hwndClist = CreateWindowExA(0, "CListControl", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD | 0x248, 184, 0, 30, 30, hwndParent, (HMENU)IDC_CLIST, g_hInst, NULL);

	//MAD: fix for little bug, when following code didn't work (another hack :) )
	HANDLE hItem;
	SendMessage(hwndClist, WM_TIMER, 14, 0);
	//
	hItem = (HANDLE) SendDlgItemMessage(hwndParent, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) dat->hContact, 0);

	SetWindowLongPtr(hwndClist, GWL_EXSTYLE, GetWindowLongPtr(hwndClist, GWL_EXSTYLE) & ~CLS_EX_TRACKSELECT);
	SetWindowLongPtr(hwndClist, GWL_EXSTYLE, GetWindowLongPtr(hwndClist, GWL_EXSTYLE) | (CLS_EX_NOSMOOTHSCROLLING | CLS_EX_NOTRANSLUCENTSEL));
	//MAD: show offline contacts in multi-send
	if (!PluginConfig.m_AllowOfflineMultisend)
		SetWindowLongPtr(hwndClist, GWL_STYLE, GetWindowLongPtr(hwndClist, GWL_STYLE) | CLS_HIDEOFFLINE);
	//
	if (hItem)
		SendMessage(hwndClist, CLM_SETCHECKMARK, (WPARAM) hItem, 1);

	if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_DISABLEGROUPS && !M->GetByte("CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
		SendMessage(hwndClist, CLM_SETUSEGROUPS, (WPARAM) FALSE, 0);
	else
		SendMessage(hwndClist, CLM_SETUSEGROUPS, (WPARAM) TRUE, 0);
	if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_HIDEEMPTYGROUPS && M->GetByte("CList", "HideEmptyGroups", SETTING_USEGROUPS_DEFAULT))
		SendMessage(hwndClist, CLM_SETHIDEEMPTYGROUPS, (WPARAM) TRUE, 0);
	else
		SendMessage(hwndClist, CLM_SETHIDEEMPTYGROUPS, (WPARAM) FALSE, 0);
	SendMessage(hwndClist, CLM_FIRST + 106, 0, 1);
	SendMessage(hwndClist, CLM_AUTOREBUILD, 0, 0);

	return hwndClist;
}

LRESULT DM_MouseWheelHandler(HWND hwnd, HWND hwndParent, struct _MessageWindowData *mwdat, WPARAM wParam, LPARAM lParam)
{
	RECT rc, rc1;
	POINT pt;
	TCHITTESTINFO hti;
	HWND hwndTab;
	UINT uID = mwdat->bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG;
	UINT uIDMsg = mwdat->bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE;

	GetCursorPos(&pt);
	GetWindowRect(hwnd, &rc);
	if (PtInRect(&rc, pt))
		return 1;
	if (mwdat->pContainer->dwFlags & CNT_SIDEBAR) {
		GetWindowRect(GetDlgItem(mwdat->pContainer->hwnd, IDC_SIDEBARUP), &rc);
		GetWindowRect(GetDlgItem(mwdat->pContainer->hwnd, IDC_SIDEBARDOWN), &rc1);
		rc.bottom = rc1.bottom;
		if (PtInRect(&rc, pt)) {
			short amount = (short)(HIWORD(wParam));
			SendMessage(mwdat->pContainer->hwnd, WM_COMMAND, MAKELONG(amount > 0 ? IDC_SIDEBARUP : IDC_SIDEBARDOWN, 0), (LPARAM)uIDMsg);
			return 0;
		}
	}
	if (mwdat->hwndIEView)
		GetWindowRect(mwdat->hwndIEView, &rc);
	else if (mwdat->hwndHPP)
		GetWindowRect(mwdat->hwndHPP, &rc);
	else
		GetWindowRect(GetDlgItem(hwndParent, uID), &rc);
	if (PtInRect(&rc, pt)) {
		HWND hwnd = (mwdat->hwndIEView || mwdat->hwndHPP) ? mwdat->hwndIWebBrowserControl : GetDlgItem(hwndParent, uID);
		short wDirection = (short)HIWORD(wParam);

		if (hwnd == 0)
			hwnd = WindowFromPoint(pt);

		if (LOWORD(wParam) & MK_SHIFT || M->GetByte("fastscroll", 0)) {
			if (wDirection < 0)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
			else if (wDirection > 0)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
		} else
			SendMessage(hwnd, WM_MOUSEWHEEL, wParam, lParam);
		return 0;
	}
	hwndTab = GetDlgItem(mwdat->pContainer->hwnd, IDC_MSGTABS);
	GetCursorPos(&hti.pt);
	ScreenToClient(hwndTab, &hti.pt);
	hti.flags = 0;
	if (TabCtrl_HitTest(hwndTab, &hti) != -1) {
		SendMessage(hwndTab, WM_MOUSEWHEEL, wParam, -1);
		return 0;
	}
	return 1;
}

void DM_FreeTheme(_MessageWindowData *dat)
{
	if(dat) {
		if (CMimAPI::m_pfnCloseThemeData) {
			if(dat->hTheme) {
				CMimAPI::m_pfnCloseThemeData(dat->hTheme);
				dat->hTheme = 0;
			}
			if(dat->hThemeIP) {
				CMimAPI::m_pfnCloseThemeData(dat->hThemeIP);
				dat->hThemeIP = 0;
			}
			if(dat->hThemeToolbar) {
				CMimAPI::m_pfnCloseThemeData(dat->hThemeToolbar);
				dat->hThemeToolbar = 0;
			}
		}
	}
}

LRESULT DM_ThemeChanged(_MessageWindowData *dat)
{
	CSkinItem *item_log = &SkinItems[ID_EXTBKHISTORY];
	CSkinItem *item_msg = &SkinItems[ID_EXTBKINPUTAREA];

	dat->bFlatMsgLog = M->GetByte("flatlog", 1);

	HWND	hwnd = dat->hwnd;

	if (!dat->bFlatMsgLog)
		dat->hTheme = (M->isVSAPIState() && CMimAPI::m_pfnOpenThemeData) ? CMimAPI::m_pfnOpenThemeData(hwnd, L"EDIT") : 0;
	else
		dat->hTheme = 0;

	if (dat->bType == SESSIONTYPE_IM) {
		if (dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_log->IGNORED))
			SetWindowLongPtr(GetDlgItem(hwnd, IDC_LOG), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwnd, IDC_LOG), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
		if (dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_msg->IGNORED))
			SetWindowLongPtr(GetDlgItem(hwnd, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwnd, IDC_MESSAGE), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
	} else {
		if (dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_log->IGNORED)) {
			SetWindowLongPtr(GetDlgItem(hwnd, IDC_CHAT_LOG), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwnd, IDC_CHAT_LOG), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
			SetWindowLongPtr(GetDlgItem(hwnd, IDC_LIST), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwnd, IDC_LIST), GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
		}
		if (dat->bFlatMsgLog || dat->hTheme != 0 || (dat->pContainer->bSkinned && !item_msg->IGNORED))
			SetWindowLongPtr(GetDlgItem(hwnd, IDC_CHAT_MESSAGE), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwnd, IDC_CHAT_MESSAGE), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
	}
	dat->hThemeIP = M->isAero() ? CMimAPI::m_pfnOpenThemeData(hwnd, L"ButtonStyle") : 0;
	dat->hThemeToolbar = (M->isAero() || (!CSkin::m_skinEnabled && M->isVSThemed())) ? (PluginConfig.m_bIsVista ? CMimAPI::m_pfnOpenThemeData(hwnd, L"STARTPANEL") : CMimAPI::m_pfnOpenThemeData(hwnd, L"REBAR")) : 0;

	return 0;
}

void DM_NotifyTyping(struct _MessageWindowData *dat, int mode)
{
	DWORD protoStatus;
	DWORD protoCaps;
	DWORD typeCaps;

	if (dat && dat->hContact) {
		DeletePopupsForContact(dat->hContact, PU_REMOVE_ON_TYPE);

		// Don't send to protocols who don't support typing
		// Don't send to users who are unchecked in the typing notification options
		// Don't send to protocols that are offline
		// Don't send to users who are not visible and
		// Don't send to users who are not on the visible list when you are in invisible mode.

		if(dat->fEditNotesActive)							// don't send them when editing user notes, pretty scary, huh? :)
			return;

		if (!M->GetByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, M->GetByte(SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)))
			return;
		if (!dat->szProto)
			return;
		protoStatus = CallProtoService(dat->szProto, PS_GETSTATUS, 0, 0);
		protoCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_1, 0);
		typeCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);

		if (!(typeCaps & PF4_SUPPORTTYPING))
			return;
		if (protoStatus < ID_STATUS_ONLINE)
			return;
		if (protoCaps & PF1_VISLIST && DBGetContactSettingWord(dat->hContact, dat->szProto, "ApparentMode", 0) == ID_STATUS_OFFLINE)
			return;
		if (protoCaps & PF1_INVISLIST && protoStatus == ID_STATUS_INVISIBLE && DBGetContactSettingWord(dat->hContact, dat->szProto, "ApparentMode", 0) != ID_STATUS_ONLINE)
			return;
		if (M->GetByte(dat->hContact, "CList", "NotOnList", 0)
				&& !M->GetByte(SRMSGMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN))
			return;
		// End user check
		dat->nTypeMode = mode;
		CallService(MS_PROTO_SELFISTYPING, (WPARAM) dat->hContact, dat->nTypeMode);
	}
}

void DM_OptionsApplied(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	if(dat == 0)
		return;

	HWND 				hwndDlg = dat->hwnd;
	ContainerWindowData *m_pContainer = dat->pContainer;

	dat->szMicroLf[0] = 0;
	if (wParam == 1)      // 1 means, the message came from message log options page, so reload the defaults...
		LoadLocalFlags(hwndDlg, dat);

	if (!(dat->dwFlags & MWF_SHOW_PRIVATETHEME))
		LoadThemeDefaults(hwndDlg, dat);

	if (dat->hContact) {
		dat->dwIsFavoritOrRecent = MAKELONG((WORD)DBGetContactSettingWord(dat->hContact, SRMSGMOD_T, "isFavorite", 0),
											(WORD)M->GetDword(dat->hContact, "isRecent", 0));
		LoadTimeZone(hwndDlg, dat);
	}

	if (dat->hContact && dat->szProto != NULL && dat->bIsMeta) {
		DWORD dwForcedContactNum = 0;
		CallService(MS_MC_GETFORCESTATE, (WPARAM)dat->hContact, (LPARAM)&dwForcedContactNum);
		M->WriteDword(dat->hContact, SRMSGMOD_T, "tabSRMM_forced", dwForcedContactNum);
	}

	dat->showUIElements = m_pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;

	dat->dwFlagsEx = M->GetByte(SRMSGSET_SHOWURLS, SRMSGDEFSET_SHOWURLS) ? MWF_SHOW_URLEVENTS : 0;
	dat->dwFlagsEx |= M->GetByte(SRMSGSET_SHOWFILES, SRMSGDEFSET_SHOWFILES) ? MWF_SHOW_FILEEVENTS : 0;
	dat->dwFlagsEx |= M->GetByte(dat->hContact, "splitoverride", 0) ? MWF_SHOW_SPLITTEROVERRIDE : 0;
	dat->Panel->getVisibility();

	// small inner margins (padding) for the text areas

	SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));
	SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));

	GetSendFormat(hwndDlg, dat, 1);
	SetDialogToType(hwndDlg);
	SendMessage(hwndDlg, DM_CONFIGURETOOLBAR, 0, 0);

	if (dat->hBkgBrush)
		DeleteObject(dat->hBkgBrush);
	if (dat->hInputBkgBrush)
		DeleteObject(dat->hInputBkgBrush);
	{
		char *szStreamOut = NULL;
		SETTEXTEX stx = {ST_DEFAULT, CP_UTF8};
		COLORREF colour = M->GetDword(FONTMODULE, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
		COLORREF inputcharcolor;
		CHARFORMAT2A cf2;
		LOGFONTA lf;
		int i = 0;

		ZeroMemory(&cf2, sizeof(CHARFORMAT2A));
		dat->inputbg = dat->theme.inputbg;
		if (GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
			szStreamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS | SFF_PLAINRTF | SF_USECODEPAGE));

		dat->hBkgBrush = CreateSolidBrush(colour);
		dat->hInputBkgBrush = CreateSolidBrush(dat->inputbg);
		SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
		SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETBKGNDCOLOR, 0, dat->inputbg);
		lf = dat->theme.logFonts[MSGFONTID_MESSAGEAREA];
		inputcharcolor = dat->theme.fontColors[MSGFONTID_MESSAGEAREA];
		/*
		 * correct the input area text color to avoid a color from the table of usable bbcode colors
		 */
		for (i = 0; i < PluginConfig.rtf_ctablesize; i++) {
			if (rtf_ctable[i].clr == inputcharcolor)
				inputcharcolor = RGB(GetRValue(inputcharcolor), GetGValue(inputcharcolor), GetBValue(inputcharcolor) == 0 ? GetBValue(inputcharcolor) + 1 : GetBValue(inputcharcolor) - 1);
		}
		cf2.dwMask = CFM_COLOR | CFM_FACE | CFM_CHARSET | CFM_SIZE | CFM_WEIGHT | CFM_BOLD | CFM_ITALIC;
		cf2.cbSize = sizeof(cf2);
		cf2.crTextColor = inputcharcolor;
		cf2.bCharSet = lf.lfCharSet;
		strncpy(cf2.szFaceName, lf.lfFaceName, LF_FACESIZE);
		cf2.dwEffects = ((lf.lfWeight >= FW_BOLD) ? CFE_BOLD : 0) | (lf.lfItalic ? CFE_ITALIC : 0)|(lf.lfUnderline ? CFE_UNDERLINE : 0)|(lf.lfStrikeOut ? CFE_STRIKEOUT : 0);
		cf2.wWeight = (WORD)lf.lfWeight;
		cf2.bPitchAndFamily = lf.lfPitchAndFamily;
		cf2.yHeight = abs(lf.lfHeight) * 15;
		SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);

		/*
		 * setup the rich edit control(s)
		 * LOG is always set to RTL, because this is needed for proper bidirectional operation later.
		 * The real text direction is then enforced by the streaming code which adds appropiate paragraph
		 * and textflow formatting commands to the
		 */
		{
			_PARAFORMAT2 pf2;
			ZeroMemory((void *)&pf2, sizeof(pf2));
			pf2.cbSize = sizeof(pf2);

			pf2.wEffects = PFE_RTLPARA;// dat->dwFlags & MWF_LOG_RTL ? PFE_RTLPARA : 0;
			pf2.dwMask = PFM_RTLPARA;
			if (FindRTLLocale(dat))
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
			if (!(dat->dwFlags & MWF_LOG_RTL)) {
				pf2.wEffects = 0;
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
			}
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);
			pf2.wEffects = PFE_RTLPARA;
			pf2.dwMask |= PFM_OFFSET;
			if (dat->dwFlags & MWF_INITMODE) {
				pf2.dwMask |= (PFM_RIGHTINDENT | PFM_OFFSETINDENT);
				pf2.dxStartIndent = 30;
				pf2.dxRightIndent = 30;
			}
			pf2.dxOffset = dat->theme.left_indent + 30;
			SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);
		}

		/*
		 * set the scrollbars etc to RTL/LTR (only for manual RTL mode)
		 */

		if (dat->dwFlags & MWF_LOG_RTL) {
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LOG), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LOG), GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
		} else {
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE) &~(WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LOG), GWL_EXSTYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LOG), GWL_EXSTYLE) &~(WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
		}
		SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
		if (szStreamOut != NULL) {
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szStreamOut);
			free(szStreamOut);
		}
	}
	if (hwndDlg == m_pContainer->hwndActive)
		SendMessage(m_pContainer->hwnd, WM_SIZE, 0, 0);
	InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
	if (!lParam)
		SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);

	SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
}


void DM_Typing(_MessageWindowData *dat)
{
	if(dat == 0)
		return;

	HWND	hwndDlg = dat->hwnd;
	HWND    hwndContainer = dat->pContainer->hwnd;
	HWND	hwndStatus = dat->pContainer->hwndStatus;

	if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON && GetTickCount() - dat->nLastTyping > TIMEOUT_TYPEOFF) {
		DM_NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
	}
	if (dat->showTyping == 1) {
		if (dat->nTypeSecs > 0) {
			dat->nTypeSecs--;
			if (GetForegroundWindow() == hwndContainer)
				SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
		} else {
			struct _MessageWindowData *dat_active = NULL;
			dat->showTyping = 2;
			dat->nTypeSecs = 10;

			mir_sntprintf(dat->szStatusBar, safe_sizeof(dat->szStatusBar),
						  CTranslator::get(CTranslator::GEN_MTN_STOPPED), dat->szNickname);
			if(hwndStatus && dat->pContainer->hwndActive == hwndDlg)
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) dat->szStatusBar);

			SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
			HandleIconFeedback(dat, (HICON) - 1);
			dat_active = (struct _MessageWindowData *)GetWindowLongPtr(dat->pContainer->hwndActive, GWLP_USERDATA);
			if (dat_active && dat_active->bType == SESSIONTYPE_IM)
				SendMessage(hwndContainer, DM_UPDATETITLE, 0, 0);
			else
				SendMessage(hwndContainer, DM_UPDATETITLE, (WPARAM)dat->pContainer->hwndActive, (LPARAM)1);
			if (!(dat->pContainer->dwFlags & CNT_NOFLASH) && PluginConfig.m_FlashOnMTN)
				ReflashContainer(dat->pContainer);
		}
	} else if(dat->showTyping == 2) {
		if (dat->nTypeSecs > 0)
			dat->nTypeSecs--;
		else {
			dat->szStatusBar[0] = 0;
			dat->showTyping = 0;
		}
		UpdateStatusBar(dat);
	}
	else {
		if (dat->nTypeSecs > 0) {
			mir_sntprintf(dat->szStatusBar, safe_sizeof(dat->szStatusBar), CTranslator::get(CTranslator::GEN_MTN_STARTWITHNICK), dat->szNickname);

			dat->nTypeSecs--;
			if (hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) dat->szStatusBar);
				SendMessage(hwndStatus, SB_SETICON, 0, (LPARAM) PluginConfig.g_buttonBarIcons[5]);
			}
			if (IsIconic(hwndContainer) || GetForegroundWindow() != hwndContainer || GetActiveWindow() != hwndContainer) {
				SetWindowText(hwndContainer, dat->szStatusBar);
				dat->pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
				if (!(dat->pContainer->dwFlags & CNT_NOFLASH) && PluginConfig.m_FlashOnMTN)
					ReflashContainer(dat->pContainer);
			}
			if (dat->pContainer->hwndActive != hwndDlg) {
				if (dat->mayFlashTab)
					dat->iFlashIcon = PluginConfig.g_IconTypingEvent;
				HandleIconFeedback(dat, PluginConfig.g_IconTypingEvent);
			} else {         // active tab may show icon if status bar is disabled
				if (!hwndStatus) {
					if (TabCtrl_GetItemCount(GetParent(hwndDlg)) > 1 || !(dat->pContainer->dwFlags & CNT_HIDETABS)) {
						HandleIconFeedback(dat, PluginConfig.g_IconTypingEvent);
					}
				}
			}
			if ((GetForegroundWindow() != hwndContainer) || (dat->pContainer->hwndStatus == 0))
				SendMessage(hwndContainer, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) PluginConfig.g_buttonBarIcons[5]);
			dat->showTyping = 1;
		}
	}
}

void DM_UpdateTitle(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	TCHAR 					newtitle[128];
	char*					szProto = 0, *szOldMetaProto = 0;
	TCHAR*					pszNewTitleEnd;
	TCHAR 					newcontactname[128];
	TCHAR*					temp;
	TCITEM 					item;
	int    					iHash = 0;
	WORD 					wOldApparentMode;
	DWORD 					dwOldIdle = dat->idle;
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) wParam;
	char*					szActProto = 0;
	HANDLE 					hActContact = 0;
	BYTE 					oldXStatus = dat->xStatus;

	HWND 					hwndDlg = dat->hwnd;
	HWND					hwndTab = GetParent(hwndDlg);
	HWND					hwndContainer = dat->pContainer->hwnd;
	ContainerWindowData*	m_pContainer = dat->pContainer;

	ZeroMemory((void *)newcontactname,  sizeof(newcontactname));
	dat->szNickname[0] = 0;
	dat->szStatus[0] = 0;

	pszNewTitleEnd = _T("Message Session");

	if (dat->iTabID == -1)
		return;

	ZeroMemory((void *)&item, sizeof(item));
	if (dat->hContact) {
		int iHasName;
		TCHAR fulluin[256];
		if (dat->szProto) {
			TCHAR	*wUIN = NULL;

			GetContactUIN(hwndDlg, dat);

			if (dat->bIsMeta) {
				szOldMetaProto = dat->szMetaProto;
				szProto = GetCurrentMetaContactProto(dat);
			}
			szActProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
			hActContact = /* dat->bIsMeta ? dat->hSubContact :*/ dat->hContact;

			mir_sntprintf(dat->szNickname, 80, _T("%s"), (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hActContact, GCDNF_TCHAR));
			iHasName = (int)dat->uin[0];        // dat->uin[0] == 0 if there is no valid UIN
			dat->idle = M->GetDword(dat->hContact, dat->szProto, "IdleTS", 0);
			dat->dwFlagsEx =  dat->idle ? dat->dwFlagsEx | MWF_SHOW_ISIDLE : dat->dwFlagsEx & ~MWF_SHOW_ISIDLE;
			dat->xStatus = M->GetByte(hActContact, szActProto, "XStatusId", 0);

			/*
			 * cut nickname on tabs...
			 */
			temp = dat->szNickname;
			while (*temp)
				iHash += (*(temp++) * (int)(temp - dat->szNickname + 1));

			dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
			mir_sntprintf(dat->szStatus, safe_sizeof(dat->szStatus), _T("%s"), (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, GCMDF_TCHAR));
			wOldApparentMode = dat->wApparentMode;
			dat->wApparentMode = DBGetContactSettingWord(hActContact, szActProto, "ApparentMode", 0);

			if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || dat->xStatus != oldXStatus || lParam != 0) {
				if (PluginConfig.m_CutContactNameOnTabs)
					CutContactName(dat->szNickname, newcontactname, safe_sizeof(newcontactname));
				else
					lstrcpyn(newcontactname, dat->szNickname, safe_sizeof(newcontactname));
				//Mad: to fix tab width for nicknames with ampersands
				DoubleAmpersands(newcontactname);

				if (lstrlen(newcontactname) != 0 && dat->szStatus != NULL) {
					if (PluginConfig.m_StatusOnTabs)
						mir_sntprintf(newtitle, 127, _T("%s (%s)"), newcontactname, dat->szStatus);
					else
						mir_sntprintf(newtitle, 127, _T("%s"), newcontactname);
				} else
					mir_sntprintf(newtitle, 127, _T("%s"), _T("Forward"));

				item.mask |= TCIF_TEXT;
			}
			SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
			wUIN = a2tf((TCHAR *)dat->uin, 0, 0);
			if (dat->bIsMeta)
				mir_sntprintf(fulluin, safe_sizeof(fulluin),
							  CTranslator::get(CTranslator::GEN_MSG_UINCOPY),
							  iHasName ? wUIN : CTranslator::get(CTranslator::GEN_MSG_NOUIN));
			else
				mir_sntprintf(fulluin, safe_sizeof(fulluin),
							  CTranslator::get(CTranslator::GEN_MSG_UINCOPY_NOMC),
							  iHasName ? wUIN : CTranslator::get(CTranslator::GEN_MSG_NOUIN));

			SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONADDTOOLTIP, /*iHasName ?*/ (WPARAM)fulluin /*: (WPARAM)_T("")*/, 0);
			if(wUIN)
				mir_free(wUIN);
		}
	} else
		lstrcpyn(newtitle, pszNewTitleEnd, safe_sizeof(newtitle));

	if (dat->xStatus != oldXStatus || dat->idle != dwOldIdle || iHash != dat->iOldHash || dat->wApparentMode != wOldApparentMode || dat->wStatus != dat->wOldStatus || lParam != 0 || (dat->bIsMeta && dat->szMetaProto != szOldMetaProto)) {
		if (dat->hContact != 0 &&(PluginConfig.m_LogStatusChanges != 0)&& dat->dwFlags & MWF_LOG_STATUSCHANGES) {
			if (dat->wStatus != dat->wOldStatus && dat->hContact != 0 && dat->wOldStatus != (WORD) - 1 && !(dat->dwFlags & MWF_INITMODE)) {          // log status changes to message log
				DBEVENTINFO dbei;
				TCHAR buffer[450];
				HANDLE hNewEvent;

				TCHAR *szOldStatus = (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wOldStatus, GCMDF_TCHAR);
				TCHAR *szNewStatus = (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wStatus, GCMDF_TCHAR);

				if (dat->szProto != NULL) {
					if (dat->wStatus == ID_STATUS_OFFLINE)
						mir_sntprintf(buffer, safe_sizeof(buffer), CTranslator::get(CTranslator::GEN_MSG_SIGNEDOFF));
					else if (dat->wOldStatus == ID_STATUS_OFFLINE)
						mir_sntprintf(buffer, safe_sizeof(buffer), CTranslator::get(CTranslator::GEN_MSG_SIGNEDON), szNewStatus);
					else
						mir_sntprintf(buffer, safe_sizeof(buffer), CTranslator::get(CTranslator::GEN_MSG_CHANGEDSTATUS), szOldStatus, szNewStatus);
				}
#if defined(_UNICODE)
				dbei.pBlob = (PBYTE)M->utf8_encodeW(buffer);
				dbei.cbBlob = lstrlenA((char *)dbei.pBlob) + 1;
				dbei.flags = DBEF_UTF | DBEF_READ;
#else
				dbei.cbBlob = lstrlenA(buffer) + 1;
				dbei.pBlob = (PBYTE)buffer;
				dbei.flags = DBEF_READ;
#endif
				dbei.cbSize = sizeof(dbei);
				dbei.eventType = EVENTTYPE_STATUSCHANGE;
				dbei.timestamp = time(NULL);
				dbei.szModule = dat->szProto;
				hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
				if (dat->hDbEventFirst == NULL) {
					dat->hDbEventFirst = hNewEvent;
					SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
				}
#if defined(_UNICODE)
				mir_free((void *)dbei.pBlob);
#endif
			}
		}

		/*
		 * update readable account name (the subcontact may have changed
		 */

		if(dat->bIsMeta) {
			PROTOACCOUNT *acc = (PROTOACCOUNT *)CallService(MS_PROTO_GETACCOUNT, (WPARAM)0, (LPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto));
			if(acc && acc->tszAccountName)
				mir_sntprintf(dat->szAccount, 128, acc->tszAccountName);
		}

		if (item.mask & TCIF_TEXT) {
			item.pszText = newtitle;
			_tcsncpy(dat->newtitle, newtitle, safe_sizeof(dat->newtitle));
			dat->newtitle[127] = 0;
			item.cchTextMax = 127;;
		}
		if (dat->iTabID  >= 0) {
			TabCtrl_SetItem(hwndTab, dat->iTabID, &item);
			if(m_pContainer->dwFlags & CNT_SIDEBAR)
				m_pContainer->SideBar->updateSession(dat);
		}
		if (m_pContainer->hwndActive == hwndDlg && (dat->iOldHash != iHash || dat->wOldStatus != dat->wStatus || dat->xStatus != oldXStatus))
			SendMessage(hwndContainer, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);

		dat->iOldHash = iHash;
		dat->wOldStatus = dat->wStatus;

		UpdateTrayMenuState(dat, TRUE);
		if (LOWORD(dat->dwIsFavoritOrRecent))
			AddContactToFavorites(dat->hContact, dat->szNickname, szActProto, dat->szStatus, dat->wStatus, LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), 0, PluginConfig.g_hMenuFavorites, dat->codePage);
		if (M->GetDword(dat->hContact, "isRecent", 0)) {
			dat->dwIsFavoritOrRecent |= 0x00010000;
			AddContactToFavorites(dat->hContact, dat->szNickname, szActProto, dat->szStatus, dat->wStatus, LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), 0, PluginConfig.g_hMenuRecent, dat->codePage);
		} else
			dat->dwIsFavoritOrRecent &= 0x0000ffff;

		dat->Panel->Invalidate();

		if (PluginConfig.g_FlashAvatarAvail) {
			FLASHAVATAR fa = {0};

			fa.hContact = dat->hContact;
			fa.hWindow = 0;
			fa.id = 25367;
			fa.cProto = dat->szProto;

			CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
			dat->hwndFlash = fa.hWindow;
			if (dat->hwndFlash) {
				bool isInfoPanel = dat->Panel->isActive();
				SetParent(dat->hwndFlash, GetDlgItem(hwndDlg, isInfoPanel ? IDC_PANELPIC : IDC_CONTACTPIC));
			}
		}
	}
	// care about MetaContacts and update the statusbar icon with the currently "most online" contact...
	if (dat->bIsMeta) {
		PostMessage(hwndDlg, DM_UPDATEMETACONTACTINFO, 0, 0);
		PostMessage(hwndDlg, DM_OWNNICKCHANGED, 0, 0);
		if (m_pContainer->dwFlags & CNT_UINSTATUSBAR)
			DM_UpdateLastMessage(dat);
	}
}

/*
* status icon stuff (by sje, used for indicating encryption status in the status bar
* this is now part of the message window api
*/

static HANDLE hHookIconPressedEvt;
struct StatusIconListNode *status_icon_list = 0;
int status_icon_list_size = 0;

static INT_PTR SI_AddStatusIcon(WPARAM wParam, LPARAM lParam)
{
	StatusIconData *sid = (StatusIconData *)lParam;
	struct StatusIconListNode *siln = (struct StatusIconListNode *)mir_alloc(sizeof(struct StatusIconListNode));

	siln->sid.cbSize = sid->cbSize;
	siln->sid.szModule = mir_strdup(sid->szModule);
	siln->sid.dwId = sid->dwId;
	siln->sid.hIcon = sid->hIcon;
	siln->sid.hIconDisabled = sid->hIconDisabled;
	siln->sid.flags = sid->flags;
	if (sid->szTooltip) siln->sid.szTooltip = mir_strdup(sid->szTooltip);
	else siln->sid.szTooltip = 0;

	siln->next = status_icon_list;
	status_icon_list = siln;
	status_icon_list_size++;

	M->BroadcastMessage(DM_STATUSICONCHANGE, 0, 0);
	return 0;
}

static INT_PTR SI_RemoveStatusIcon(WPARAM wParam, LPARAM lParam)
{
	StatusIconData *sid = (StatusIconData *)lParam;
	struct StatusIconListNode *current = status_icon_list, *prev = 0;

	while (current) {
		if (strcmp(current->sid.szModule, sid->szModule) == 0 && current->sid.dwId == sid->dwId) {
			if (prev) prev->next = current->next;
			else status_icon_list = current->next;

			status_icon_list_size--;

			mir_free(current->sid.szModule);
			DestroyIcon(current->sid.hIcon);
			if (current->sid.hIconDisabled) DestroyIcon(current->sid.hIconDisabled);
			if (current->sid.szTooltip) mir_free(current->sid.szTooltip);
			mir_free(current);
			M->BroadcastMessage(DM_STATUSICONCHANGE, 0, 0);
			return 0;
		}

		prev = current;
		current = current->next;
	}
	return 1;
}

static void SI_RemoveAllStatusIcons(void)
{
	struct StatusIconListNode *current;

	while (status_icon_list) {
		current = status_icon_list;
		status_icon_list = status_icon_list->next;
		status_icon_list_size--;

		mir_free(current->sid.szModule);
		DestroyIcon(current->sid.hIcon);
		if (current->sid.hIconDisabled) DestroyIcon(current->sid.hIconDisabled);
		if (current->sid.szTooltip) mir_free(current->sid.szTooltip);
		mir_free(current);
	}
	M->BroadcastMessage(DM_STATUSICONCHANGE, 0, 0);
}

static INT_PTR SI_ModifyStatusIcon(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;

	StatusIconData *sid = (StatusIconData *)lParam;
	struct StatusIconListNode *current = status_icon_list;

	while (current) {
		if (strcmp(current->sid.szModule, sid->szModule) == 0 && current->sid.dwId == sid->dwId) {
			if (!hContact) {
				current->sid.flags = sid->flags;
				if (sid->hIcon) {
					DestroyIcon(current->sid.hIcon);
					current->sid.hIcon = sid->hIcon;
				}
				if (sid->hIconDisabled) {
					DestroyIcon(current->sid.hIconDisabled);
					current->sid.hIconDisabled = sid->hIconDisabled;
				}
				if (sid->szTooltip) {
					if (current->sid.szTooltip) mir_free(current->sid.szTooltip);
					current->sid.szTooltip = mir_strdup(sid->szTooltip);
				}

				M->BroadcastMessage(DM_STATUSICONCHANGE, 0, 0);
			} else {
				char buff[256];
				HWND hwnd;
				if (!(sid->flags&MBF_OWNERSTATE)) {
					sprintf(buff, "SRMMStatusIconFlags%d", (int)sid->dwId);
					M->WriteByte(hContact, sid->szModule, buff, (BYTE)sid->flags);
				}
				if ((hwnd = M->FindWindow(hContact))) {
					if (sid->flags&MBF_OWNERSTATE) {

						struct StatusIconListNode *siln = NULL;
						struct _MessageWindowData *dat = (struct _MessageWindowData *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
						struct StatusIconListNode *psi = dat->pSINod;
						while (psi) {
							if (strcmp(psi->sid.szModule, sid->szModule) == 0 && psi->sid.dwId == sid->dwId) {
								siln = psi;
								break;
							}
							psi = psi->next;
						}
						if (!siln) {
							siln = (struct StatusIconListNode *)mir_alloc(sizeof(struct StatusIconListNode));
							siln->sid.szModule = mir_strdup(sid->szModule);
							siln->sid.dwId = sid->dwId;
							siln->sid.hIcon = sid->hIcon;
							siln->sid.hIconDisabled = sid->hIconDisabled;
							siln->sid.flags = sid->flags;

							if (sid->szTooltip) siln->sid.szTooltip = mir_strdup(sid->szTooltip);
							else siln->sid.szTooltip = 0;

							siln->next = dat->pSINod;
							dat->pSINod = siln;
						} else {
							siln->sid.hIcon = sid->hIcon;
							siln->sid.hIconDisabled = sid->hIconDisabled;
							siln->sid.flags = sid->flags;
							if (siln->sid.szTooltip) mir_free(siln->sid.szTooltip);

							if (sid->szTooltip) siln->sid.szTooltip = mir_strdup(sid->szTooltip);
							else siln->sid.szTooltip = 0;

						}


						PostMessage(hwnd, DM_STATUSICONCHANGE, 0, 0);
					} else
						PostMessage(hwnd, DM_STATUSICONCHANGE, 0, 0);
				}
			}
			return 0;
		}
		current = current->next;
	}

	return 1;
}

void DrawStatusIcons(struct _MessageWindowData *dat, HDC hDC, RECT r, int gap)
{
	struct StatusIconListNode *current = status_icon_list;
	HICON hIcon = NULL;
	char buff[256];
	int flags;
	int x = r.left;
	SetBkMode(hDC, TRANSPARENT);
	while (current) {
		if (current->sid.flags&MBF_OWNERSTATE) {
			struct StatusIconListNode *currentSIN = dat->pSINod;
			flags = current->sid.flags;
			hIcon = current->sid.hIcon;
			while (currentSIN) {
				if (strcmp(currentSIN->sid.szModule, current->sid.szModule) == 0 && currentSIN->sid.dwId == current->sid.dwId) {
					flags = currentSIN->sid.flags;
					hIcon = currentSIN->sid.hIcon;
					break;
				}
				currentSIN = currentSIN->next;
			}
		} else {
			sprintf(buff, "SRMMStatusIconFlags%d", (int)current->sid.dwId);
			flags = M->GetByte(dat->hContact, current->sid.szModule, buff, current->sid.flags);
		}

		if (!(flags & MBF_HIDDEN)) {
			if (!(flags&MBF_OWNERSTATE) && (flags & MBF_DISABLED) && current->sid.hIconDisabled)
				hIcon = current->sid.hIconDisabled;
			else if (!(flags&MBF_OWNERSTATE))
				hIcon = current->sid.hIcon;

			if (flags & MBF_DISABLED && current->sid.hIconDisabled == (HICON)0)
				CSkin::DrawDimmedIcon(hDC, x, (r.top + r.bottom - PluginConfig.m_smcxicon) >> 1, PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, hIcon, 50);
			else
				DrawIconEx(hDC, x, (r.top + r.bottom - 16) >> 1, hIcon, 16, 16, 0, NULL, DI_NORMAL);

			x += 16 + gap;
		}
		current = current->next;
	}
	DrawIconEx(hDC, x, (r.top + r.bottom - 16) >> 1, dat->pContainer->dwFlags & CNT_NOSOUND ? PluginConfig.g_buttonBarIcons[23] : PluginConfig.g_buttonBarIcons[22], PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, 0, NULL, DI_NORMAL);
	x += PluginConfig.m_smcxicon + gap;
	if (dat->bType == SESSIONTYPE_IM)
		DrawIconEx(hDC, x, (r.top + r.bottom - PluginConfig.m_smcxicon) >> 1, M->GetByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, M->GetByte(SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)) ? PluginConfig.g_buttonBarIcons[12] : PluginConfig.g_buttonBarIcons[13], PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, 0, NULL, DI_NORMAL);
	else
		CSkin::DrawDimmedIcon(hDC, x, (r.top + r.bottom - PluginConfig.m_smcxicon) >> 1, PluginConfig.m_smcxicon, PluginConfig.m_smcyicon,
					   M->GetByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, M->GetByte(SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)) ? PluginConfig.g_buttonBarIcons[12] : PluginConfig.g_buttonBarIcons[13], 50);
	x += PluginConfig.m_smcxicon + gap;
	DrawIconEx(hDC, x, (r.top + r.bottom - PluginConfig.m_smcxicon) >> 1, PluginConfig.g_sideBarIcons[3], PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, 0, NULL, DI_NORMAL);
}

void SI_CheckStatusIconClick(struct _MessageWindowData *dat, HWND hwndFrom, POINT pt, RECT r, int gap, int code)
{
	StatusIconClickData sicd;
	struct StatusIconListNode *current = status_icon_list;
	struct StatusIconListNode *clicked = NULL;

	unsigned int iconNum = (pt.x - (r.left + 0)) / (PluginConfig.m_smcxicon + gap);
	unsigned int list_icons = 0;
	char         buff[100];
	DWORD		 flags;

	if (dat && (code == NM_CLICK || code == NM_RCLICK)) {
		POINT	ptScreen;

		GetCursorPos(&ptScreen);
		if (!PtInRect(&rcLastStatusBarClick, ptScreen))
			return;
	}
	while (current && dat) {
		if (current->sid.flags&MBF_OWNERSTATE) {
			struct StatusIconListNode *currentSIN = dat->pSINod;
			flags = current->sid.flags;
			while (currentSIN) {
				if (strcmp(currentSIN->sid.szModule, current->sid.szModule) == 0 && currentSIN->sid.dwId == current->sid.dwId) {
					flags = currentSIN->sid.flags;
					break;
				}
				currentSIN = currentSIN->next;
			}
		} else  {
			sprintf(buff, "SRMMStatusIconFlags%d", (int)current->sid.dwId);
			flags = M->GetByte(dat->hContact, current->sid.szModule, buff, current->sid.flags);
		}
		if (!(flags & MBF_HIDDEN)) {
			if (list_icons++ == iconNum)
				clicked = current;
		}
		current = current->next;
	}

	if ((int)iconNum == list_icons && code != NM_RCLICK) {
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			struct ContainerWindowData *piContainer = pFirstContainer;

			while (piContainer) {
				piContainer->dwFlags = ((dat->pContainer->dwFlags & CNT_NOSOUND) ? piContainer->dwFlags | CNT_NOSOUND : piContainer->dwFlags & ~CNT_NOSOUND);
				InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
				piContainer = piContainer->pNextContainer;
			}
		} else {
			dat->pContainer->dwFlags ^= CNT_NOSOUND;
			InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
		}
	} else if ((int)iconNum == list_icons + 1 && code != NM_RCLICK && dat->bType == SESSIONTYPE_IM) {
		SendMessage(dat->pContainer->hwndActive, WM_COMMAND, IDC_SELFTYPING, 0);
		InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
	} else if ((int)iconNum == list_icons + 2) {
		if(code == NM_CLICK)
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_LBUTTONUP);
		else if(code == NM_RCLICK)
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_RBUTTONUP);
	} else if (clicked) {
		sicd.cbSize = sizeof(StatusIconClickData);
		GetCursorPos(&sicd.clickLocation);
		sicd.dwId = clicked->sid.dwId;
		sicd.szModule = clicked->sid.szModule;
		sicd.flags = (code == NM_RCLICK ? MBCF_RIGHTBUTTON : 0);
		NotifyEventHooks(hHookIconPressedEvt, (WPARAM)dat->hContact, (LPARAM)&sicd);
		InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
	}
}

static HANDLE SI_hServiceIcon[3];

int SI_InitStatusIcons()
{
	SI_hServiceIcon[0] = CreateServiceFunction(MS_MSG_ADDICON, SI_AddStatusIcon);
	SI_hServiceIcon[1] = CreateServiceFunction(MS_MSG_REMOVEICON, SI_RemoveStatusIcon);
	SI_hServiceIcon[2] = CreateServiceFunction(MS_MSG_MODIFYICON, SI_ModifyStatusIcon);
	hHookIconPressedEvt = CreateHookableEvent(ME_MSG_ICONPRESSED);

	return 0;
}


int SI_DeinitStatusIcons()
{
	int i;
	DestroyHookableEvent(hHookIconPressedEvt);
	for (i = 0; i < 3; i++)
		DestroyServiceFunction(SI_hServiceIcon[i]);
	SI_RemoveAllStatusIcons();
	return 0;
}

int SI_GetStatusIconsCount()
{
	return status_icon_list_size;
}
