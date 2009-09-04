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
 * This implements the event notification module for tabSRMM. The code
 * is largely based on the NewEventNotify plugin for Miranda IM. See
 * notices below.
 *
 *  Name: NewEventNotify - Plugin for Miranda ICQ
 * 	Description: Notifies you when you receive a message
 * 	Author: icebreaker, <icebreaker@newmail.net>
 * 	Date: 18.07.02 13:59 / Update: 16.09.02 17:45
 * 	Copyright: (C) 2002 Starzinger Michael
 *
 */

#include "commonheaders.h"
#pragma hdrstop

#include <m_icq.h>

extern      INT_PTR CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      HIMAGELIST CreateStateImageList();
extern      HANDLE g_hEvent;

PLUGIN_DATAT *PopUpList[20];
static int PopupCount = 0;

BOOL        bWmNotify = TRUE;
extern int  safe_wcslen(wchar_t *msg, int chars);

static void CheckForRemoveMask()
{
	if (!M->GetByte(MODULE, "firsttime", 0) && (nen_options.maskActL & MASK_REMOVE || nen_options.maskActR & MASK_REMOVE || nen_options.maskActTE & MASK_REMOVE)) {
		MessageBoxA(0, Translate("One of your popup actions is set to DISMISS EVENT.\nNote that this options may have unwanted side effects as it REMOVES the event from the unread queue.\nThis may lead to events not showing up as \"new\". If you don't want this behaviour, please review the Event Notifications settings page."), "tabSRMM Warning Message", MB_OK | MB_ICONSTOP);
		M->WriteByte(MODULE, "firsttime", 1);
	}
}


int NEN_ReadOptions(NEN_OPTIONS *options)
{
	options->bPreview = (BOOL)M->GetByte(MODULE, OPT_PREVIEW, TRUE);
	options->bDefaultColorMsg = (BOOL)M->GetByte(MODULE, OPT_COLDEFAULT_MESSAGE, FALSE);
	options->bDefaultColorOthers = (BOOL)M->GetByte(MODULE, OPT_COLDEFAULT_OTHERS, FALSE);
	options->colBackMsg = (COLORREF)M->GetDword(MODULE, OPT_COLBACK_MESSAGE, DEFAULT_COLBACK);
	options->colTextMsg = (COLORREF)M->GetDword(MODULE, OPT_COLTEXT_MESSAGE, DEFAULT_COLTEXT);
	options->colBackOthers = (COLORREF)M->GetDword(MODULE, OPT_COLBACK_OTHERS, DEFAULT_COLBACK);
	options->colTextOthers = (COLORREF)M->GetDword(MODULE, OPT_COLTEXT_OTHERS, DEFAULT_COLTEXT);
	options->maskActL = (UINT)M->GetByte(MODULE, OPT_MASKACTL, DEFAULT_MASKACTL);
	options->maskActR = (UINT)M->GetByte(MODULE, OPT_MASKACTR, DEFAULT_MASKACTR);
	options->maskActTE = (UINT)M->GetByte(MODULE, OPT_MASKACTTE, DEFAULT_MASKACTR) & (MASK_OPEN | MASK_DISMISS);
	options->bMergePopup = (BOOL)M->GetByte(MODULE, OPT_MERGEPOPUP, 1);
	options->iDelayMsg = (int)M->GetDword(MODULE, OPT_DELAY_MESSAGE, (DWORD)DEFAULT_DELAY);
	options->iDelayOthers = (int)M->GetDword(MODULE, OPT_DELAY_OTHERS, (DWORD)DEFAULT_DELAY);
	options->iDelayDefault = (int)DBGetContactSettingRangedWord(NULL, "PopUp", "Seconds", SETTING_LIFETIME_DEFAULT, SETTING_LIFETIME_MIN, SETTING_LIFETIME_MAX);
	options->bShowDate = (BYTE)M->GetByte(MODULE, OPT_SHOW_DATE, FALSE);
	options->bShowTime = (BYTE)M->GetByte(MODULE, OPT_SHOW_TIME, FALSE);
	options->bShowHeaders = (BYTE)M->GetByte(MODULE, OPT_SHOW_HEADERS, FALSE);
	options->iNumberMsg = (BYTE)M->GetByte(MODULE, OPT_NUMBER_MSG, TRUE);
	options->bShowON = (BYTE)M->GetByte(MODULE, OPT_SHOW_ON, TRUE);
	options->bNoRSS = (BOOL)M->GetByte(MODULE, OPT_NORSS, FALSE);
	options->iDisable = (BYTE)M->GetByte(MODULE, OPT_DISABLE, 0);
	options->iMUCDisable = (BYTE)M->GetByte(MODULE, OPT_MUCDISABLE, 0);
	options->dwStatusMask = (DWORD)M->GetDword(MODULE, "statusmask", (DWORD) - 1);
	options->bTraySupport = (BOOL)M->GetByte(MODULE, "traysupport", 0);
	options->iAutoRestore = 0;
	options->bWindowCheck = (BOOL)M->GetByte(MODULE, OPT_WINDOWCHECK, 0);
	options->bNoRSS = (BOOL)M->GetByte(MODULE, OPT_NORSS, 0);
	options->iLimitPreview = (int)M->GetDword(MODULE, OPT_LIMITPREVIEW, 0);
	options->bAnimated = (BOOL)M->GetByte(MODULE, OPT_MINIMIZEANIMATED, 1);
	options->wMaxFavorites = 15;
	options->wMaxRecent = 15;
	options->iAnnounceMethod = (int)M->GetByte(MODULE, OPT_ANNOUNCEMETHOD, 0);
	options->dwRemoveMask = M->GetDword(MODULE, OPT_REMOVEMASK, 0);
	options->bSimpleMode = M->GetByte(MODULE, OPT_SIMPLEOPT, 0);
	CheckForRemoveMask();
	return 0;
}

int NEN_WriteOptions(NEN_OPTIONS *options)
{
	M->WriteByte(MODULE, OPT_PREVIEW, (BYTE)options->bPreview);
	M->WriteByte(MODULE, OPT_COLDEFAULT_MESSAGE, (BYTE)options->bDefaultColorMsg);
	M->WriteByte(MODULE, OPT_COLDEFAULT_OTHERS, (BYTE)options->bDefaultColorOthers);
	M->WriteDword(MODULE, OPT_COLBACK_MESSAGE, (DWORD)options->colBackMsg);
	M->WriteDword(MODULE, OPT_COLTEXT_MESSAGE, (DWORD)options->colTextMsg);
	M->WriteDword(MODULE, OPT_COLBACK_OTHERS, (DWORD)options->colBackOthers);
	M->WriteDword(MODULE, OPT_COLTEXT_OTHERS, (DWORD)options->colTextOthers);
	M->WriteByte(MODULE, OPT_MASKACTL, (BYTE)options->maskActL);
	M->WriteByte(MODULE, OPT_MASKACTR, (BYTE)options->maskActR);
	M->WriteByte(MODULE, OPT_MASKACTTE, (BYTE)options->maskActTE);
	M->WriteByte(MODULE, OPT_MERGEPOPUP, (BYTE)options->bMergePopup);
	M->WriteDword(MODULE, OPT_DELAY_MESSAGE, (DWORD)options->iDelayMsg);
	M->WriteDword(MODULE, OPT_DELAY_OTHERS, (DWORD)options->iDelayOthers);
	M->WriteByte(MODULE, OPT_SHOW_DATE, (BYTE)options->bShowDate);
	M->WriteByte(MODULE, OPT_SHOW_TIME, (BYTE)options->bShowTime);
	M->WriteByte(MODULE, OPT_SHOW_HEADERS, (BYTE)options->bShowHeaders);
	M->WriteByte(MODULE, OPT_NUMBER_MSG, (BYTE)options->iNumberMsg);
	M->WriteByte(MODULE, OPT_SHOW_ON, (BYTE)options->bShowON);
	M->WriteByte(MODULE, OPT_DISABLE, (BYTE)options->iDisable);
	M->WriteByte(MODULE, OPT_MUCDISABLE, (BYTE)options->iMUCDisable);
	M->WriteByte(MODULE, "traysupport", (BYTE)options->bTraySupport);
	M->WriteByte(MODULE, OPT_WINDOWCHECK, (BYTE)options->bWindowCheck);
	M->WriteByte(MODULE, OPT_NORSS, (BYTE)options->bNoRSS);
	M->WriteDword(MODULE, OPT_LIMITPREVIEW, options->iLimitPreview);
	M->WriteByte(MODULE, OPT_MINIMIZEANIMATED, (BYTE)options->bAnimated);
	M->WriteByte(MODULE, OPT_ANNOUNCEMETHOD, (BYTE)options->iAnnounceMethod);
	M->WriteDword(MODULE, OPT_REMOVEMASK, options->dwRemoveMask);
	M->WriteByte(MODULE, OPT_SIMPLEOPT, (BYTE)options->bSimpleMode);
	return 0;
}

INT_PTR CALLBACK DlgProcPopupOpts(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NEN_OPTIONS *options = &nen_options;
	LRESULT iIndex;

	switch (msg) {
		case WM_INITDIALOG: {
			TVINSERTSTRUCT tvi = {0};
			int i = 0;

			SetWindowLongPtr(GetDlgItem(hWnd, IDC_EVENTOPTIONS), GWL_STYLE, GetWindowLongPtr(GetDlgItem(hWnd, IDC_EVENTOPTIONS), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));
			SendDlgItemMessage(hWnd, IDC_EVENTOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
			TranslateDialogDefault(hWnd);

			/*
			* fill the tree view
			*/

			LISTOPTIONSGROUP *lGroups = CTranslator::getGroupTree(CTranslator::TREE_NEN);

			while (lGroups[i].szName != NULL) {
				tvi.hParent = 0;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.mask = TVIF_TEXT | TVIF_STATE;
				tvi.item.pszText = lGroups[i].szName;
				tvi.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
				tvi.item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_EXPANDED | TVIS_BOLD;
				lGroups[i++].handle = (LRESULT)TreeView_InsertItem(GetDlgItem(hWnd, IDC_EVENTOPTIONS), &tvi);
			}
			i = 0;

			LISTOPTIONSITEM *defaultItems = CTranslator::getTree(CTranslator::TREE_NEN);

			while (defaultItems[i].szName != 0) {
				tvi.hParent = (HTREEITEM)lGroups[defaultItems[i].uGroup].handle;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.pszText = defaultItems[i].szName;
				tvi.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
				tvi.item.lParam = i;
				tvi.item.stateMask = TVIS_STATEIMAGEMASK;
				if (defaultItems[i].uType == LOI_TYPE_SETTING)
					tvi.item.state = INDEXTOSTATEIMAGEMASK(*((BOOL *)defaultItems[i].lParam) ? 3 : 2);//2 : 1);
				else if (defaultItems[i].uType == LOI_TYPE_FLAG) {
					UINT uVal = *((UINT *)defaultItems[i].lParam);
					tvi.item.state = INDEXTOSTATEIMAGEMASK(uVal & defaultItems[i].id ? 3 : 2);//2 : 1);
				}
				defaultItems[i].handle = (LRESULT)TreeView_InsertItem(GetDlgItem(hWnd, IDC_EVENTOPTIONS), &tvi);
				i++;
			}
			SendDlgItemMessage(hWnd, IDC_COLBACK_MESSAGE, CPM_SETCOLOUR, 0, options->colBackMsg);
			SendDlgItemMessage(hWnd, IDC_COLTEXT_MESSAGE, CPM_SETCOLOUR, 0, options->colTextMsg);
			SendDlgItemMessage(hWnd, IDC_COLBACK_OTHERS, CPM_SETCOLOUR, 0, options->colBackOthers);
			SendDlgItemMessage(hWnd, IDC_COLTEXT_OTHERS, CPM_SETCOLOUR, 0, options->colTextOthers);
			CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_MESSAGE, options->bDefaultColorMsg ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_OTHERS, options->bDefaultColorOthers ? BST_CHECKED : BST_UNCHECKED);

			SendDlgItemMessage(hWnd, IDC_DELAY_MESSAGE_SPIN, UDM_SETRANGE, 0, MAKELONG(3600, -1));
			SendDlgItemMessage(hWnd, IDC_DELAY_OTHERS_SPIN, UDM_SETRANGE, 0, MAKELONG(3600, -1));

			SendDlgItemMessage(hWnd, IDC_DELAY_MESSAGE_SPIN, UDM_SETPOS, 0, (LPARAM)options->iDelayMsg);
			SendDlgItemMessage(hWnd, IDC_DELAY_OTHERS_SPIN, UDM_SETPOS, 0, (LPARAM)options->iDelayOthers);

			EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_MESSAGE), !options->bDefaultColorMsg);
			EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_MESSAGE), !options->bDefaultColorMsg);
			EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_OTHERS), !options->bDefaultColorOthers);
			EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_OTHERS), !options->bDefaultColorOthers);

			//disable merge messages options when is not using

			EnableWindow(GetDlgItem(hWnd, IDC_DELAY_MESSAGE), options->iDelayMsg != -1);
			EnableWindow(GetDlgItem(hWnd, IDC_DELAY_OTHERS), options->iDelayOthers != -1);

			SetDlgItemInt(hWnd, IDC_MESSAGEPREVIEWLIMIT, options->iLimitPreview, FALSE);
			CheckDlgButton(hWnd, IDC_LIMITPREVIEW, (options->iLimitPreview > 0) ? 1 : 0);
			SendDlgItemMessage(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN, UDM_SETRANGE, 0, MAKELONG(2048, options->iLimitPreview > 0 ? 50 : 0));
			SendDlgItemMessage(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN, UDM_SETPOS, 0, (LPARAM)options->iLimitPreview);
			EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMIT), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));
			EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));

			SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Don't use simple mode"));
			SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Notify always"));
			SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Notify for unfocused sessions"));
			SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Notify only when no window is open"));
			EnableWindow(GetDlgItem(hWnd, IDC_EVENTOPTIONS), options->bSimpleMode == 0);
			SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_SETCURSEL, options->bSimpleMode, 0);

			iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("<None>"));
			SendDlgItemMessageA(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);

			if (ServiceExists(MS_POPUP_ADDPOPUPEX)) {
				iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("Popups"));
				SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETITEMDATA, (WPARAM)iIndex, 1);
				if (options->iAnnounceMethod == 1)
					SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);
			}
			iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("Balloon tooltips"));
			SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETITEMDATA, (WPARAM)iIndex, 2);
			if (options->iAnnounceMethod == 2)
				SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);
			bWmNotify = FALSE;
			return TRUE;
		}
		case DM_STATUSMASKSET:
			M->WriteDword(MODULE, "statusmask", (DWORD)lParam);
			options->dwStatusMask = (int)lParam;
			break;
		case WM_COMMAND:
			if (!bWmNotify) {
				switch (LOWORD(wParam)) {
					case IDC_PREVIEW:
						PopupPreview(options);
						break;
					case IDC_SIMPLEMODE:
						options->bSimpleMode = (BYTE)SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_GETCURSEL, 0, 0);
						EnableWindow(GetDlgItem(hWnd, IDC_EVENTOPTIONS), options->bSimpleMode == 0);
						break;
					case IDC_POPUPSTATUSMODES: {
						HWND hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CHOOSESTATUSMODES), hWnd, DlgProcSetupStatusModes, M->GetDword(MODULE, "statusmask", (DWORD) - 1));
						SendMessage(hwndNew, DM_SETPARENTDIALOG, 0, (LPARAM)hWnd);
						break;
					}
					default: {
						options->bDefaultColorMsg = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_MESSAGE);
						options->bDefaultColorOthers = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_OTHERS);

						options->iDelayMsg = SendDlgItemMessage(hWnd, IDC_DELAY_MESSAGE_SPIN, UDM_GETPOS, 0, 0);
						options->iDelayOthers = SendDlgItemMessage(hWnd, IDC_DELAY_OTHERS_SPIN, UDM_GETPOS, 0, 0);

						options->iAnnounceMethod = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_GETITEMDATA, (WPARAM)SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_GETCURSEL, 0, 0), 0);
						options->bSimpleMode = (BYTE)SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_GETCURSEL, 0, 0);

						if (IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW))
							options->iLimitPreview = GetDlgItemInt(hWnd, IDC_MESSAGEPREVIEWLIMIT, NULL, FALSE);
						else
							options->iLimitPreview = 0;
						EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_MESSAGE), !options->bDefaultColorMsg);
						EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_MESSAGE), !options->bDefaultColorMsg);
						EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_OTHERS), !options->bDefaultColorOthers);
						EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_OTHERS), !options->bDefaultColorOthers);

						EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMIT), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));
						EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));
						//disable delay textbox when infinite is checked
						EnableWindow(GetDlgItem(hWnd, IDC_DELAY_MESSAGE), options->iDelayMsg != -1);
						EnableWindow(GetDlgItem(hWnd, IDC_DELAY_OTHERS), options->iDelayOthers != -1);
						if (HIWORD(wParam) == CPN_COLOURCHANGED) {
							options->colBackMsg = SendDlgItemMessage(hWnd, IDC_COLBACK_MESSAGE, CPM_GETCOLOUR, 0, 0);
							options->colTextMsg = SendDlgItemMessage(hWnd, IDC_COLTEXT_MESSAGE, CPM_GETCOLOUR, 0, 0);
							options->colBackOthers = SendDlgItemMessage(hWnd, IDC_COLBACK_OTHERS, CPM_GETCOLOUR, 0, 0);
							options->colTextOthers = SendDlgItemMessage(hWnd, IDC_COLTEXT_OTHERS, CPM_GETCOLOUR, 0, 0);
						}
						EnableWindow(GetDlgItem(hWnd, IDC_USESHELLNOTIFY), options->bTraySupport);
						SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
						break;
					}
				}
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
				case IDC_EVENTOPTIONS:
					if (((LPNMHDR)lParam)->code == NM_CLICK) {
						TVHITTESTINFO hti;
						TVITEM item = {0};

						item.mask = TVIF_HANDLE | TVIF_STATE;
						item.stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;
						hti.pt.x = (short)LOWORD(GetMessagePos());
						hti.pt.y = (short)HIWORD(GetMessagePos());
						ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
						if (TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti)) {
							item.hItem = (HTREEITEM)hti.hItem;
							SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
							if (item.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
								item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
								SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
							}
							else if (hti.flags&TVHT_ONITEMSTATEICON) {
								if (((item.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
									item.state = INDEXTOSTATEIMAGEMASK(1);
									SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
								}
								SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
							}
						}
					}
					break;
			}
			switch (((LPNMHDR)lParam)->code) {
				case PSN_APPLY: {
					int i = 0;
					TVITEM item = {0};
					struct ContainerWindowData *pContainer = pFirstContainer;

					LISTOPTIONSITEM *defaultItems = CTranslator::getTree(CTranslator::TREE_NEN);

					while (defaultItems[i].szName != NULL) {
						item.mask = TVIF_HANDLE | TVIF_STATE;
						item.hItem = (HTREEITEM)defaultItems[i].handle;
						item.stateMask = TVIS_STATEIMAGEMASK;

						SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
						if (defaultItems[i].uType == LOI_TYPE_SETTING) {
							BOOL *ptr = (BOOL *)defaultItems[i].lParam;
							*ptr = (item.state >> 12) == 3/*2*/ ? TRUE : FALSE;
						}
						else if (defaultItems[i].uType == LOI_TYPE_FLAG) {
							UINT *uVal = (UINT *)defaultItems[i].lParam;
							*uVal = ((item.state >> 12) == 3/*2*/) ? *uVal | defaultItems[i].id : *uVal & ~defaultItems[i].id;
						}
						i++;
					}
					NEN_WriteOptions(&nen_options);
					CheckForRemoveMask();
					CreateSystrayIcon(nen_options.bTraySupport);
					SetEvent(g_hEvent);                                 // wake up the thread which cares about the floater and tray
					break;
				}
				case PSN_RESET:
					NEN_ReadOptions(&nen_options);
					break;
			}
			break;
		case WM_DESTROY: {
			SendDlgItemMessage(hWnd, IDC_EVENTOPTIONS, TVM_GETIMAGELIST, TVSIL_STATE, 0);
			bWmNotify = TRUE;
			break;
		}
		default:
			break;
	}
	return FALSE;
}

static int NumberPopupData(HANDLE hContact)
{
	int n;

	for (n = 0;n < 20;n++) {
		if (!PopUpList[n] && !hContact)
			return n;

		if (PopUpList[n] && PopUpList[n]->hContact == hContact)
			return n;
	}
	return -1;
}

static int PopupAct(HWND hWnd, UINT mask, PLUGIN_DATAT* pdata)
{
	pdata->iActionTaken = TRUE;
	if (mask & MASK_OPEN) {
		int i;

		for (i = 0; i < pdata->nrMerged; i++) {
			if (pdata->eventData[i].hEvent != 0) {
				PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)pdata->hContact, (LPARAM)pdata->eventData[i].hEvent);
				pdata->eventData[i].hEvent = 0;
			}
		}
	}
	if (mask & MASK_REMOVE) {
		int i;

		for (i = 0; i < pdata->nrMerged; i++) {
			if (pdata->eventData[i].hEvent != 0) {
				PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_REMOVECLISTEVENT, (WPARAM)pdata->hContact, (LPARAM)pdata->eventData[i].hEvent);
				pdata->eventData[i].hEvent = 0;
			}
		}
		PopUpList[NumberPopupData(pdata->hContact)] = NULL;
	}
	if (mask & MASK_DISMISS) {
		PopUpList[NumberPopupData(pdata->hContact)] = NULL;
		PUDeletePopUp(hWnd);
	}
	return 0;
}

static BOOL CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PLUGIN_DATAT* pdata = NULL;

	pdata = (PLUGIN_DATAT *)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM)pdata);
	if (!pdata) return FALSE;

	switch (message) {
		case WM_COMMAND:
			PopupAct(hWnd, pdata->pluginOptions->maskActL, pdata);
			break;
		case WM_CONTEXTMENU:
			PopupAct(hWnd, pdata->pluginOptions->maskActR, pdata);
			break;
		case UM_FREEPLUGINDATA:
			PopUpList[NumberPopupData(pdata->hContact)] = NULL;
			PopupCount--;
			if (pdata->eventData)
				free(pdata->eventData);
			free(pdata);
			return TRUE;
		case UM_INITPOPUP:
			pdata->hWnd = hWnd;
			if (pdata->iSeconds > 0)
				SetTimer(hWnd, TIMER_TO_ACTION, pdata->iSeconds * 1000, NULL);
			break;
		case WM_MOUSEWHEEL:
			break;
		case WM_SETCURSOR:
			break;
		case WM_TIMER: {
			POINT	pt;
			RECT	rc;

			if (wParam != TIMER_TO_ACTION)
				break;

			GetCursorPos(&pt);
			GetWindowRect(hWnd, &rc);
			if(PtInRect(&rc, pt))
				break;

			if (pdata->iSeconds > 0)
				KillTimer(hWnd, TIMER_TO_ACTION);
			PopupAct(hWnd, pdata->pluginOptions->maskActTE, pdata);
			break;
		}
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/**
 * Get a preview for the message
 * caller must mir_free() the return value
 *
 * @param eventType the event type
 * @param dbe       DBEVENTINFO *: database event structure
 *
 * @return
 */
static TCHAR *GetPreviewT(WORD eventType, DBEVENTINFO* dbe)
{
	TCHAR	*commentFix = NULL;
	char	*pBlob = (char *)dbe->pBlob;
	bool	fAddEllipsis = false;

	int		iPreviewLimit = nen_options.iLimitPreview;

	if(iPreviewLimit > 500 || iPreviewLimit == 0)
		iPreviewLimit = 500;

	switch (eventType) {
		case EVENTTYPE_MESSAGE:
			if (pBlob) {
				if(nen_options.bPreview) {
					TCHAR* buf = DbGetEventTextT(dbe, CP_ACP);
					if(lstrlen(buf) > iPreviewLimit) {
						fAddEllipsis = true;
						int iIndex = iPreviewLimit;
						int iWordThreshold = 20;
						while(iIndex && buf[iIndex] != ' ' && iWordThreshold--) {
							buf[iIndex--] = 0;
						}
						buf[iIndex] = 0;
					}
					buf = (TCHAR *)mir_realloc(buf, (lstrlen(buf) + 5) * sizeof(TCHAR));
					if(fAddEllipsis)
						_tcscat(buf, _T("..."));
					return(buf);
				}
			}
			commentFix = mir_tstrdup(CTranslator::get(CTranslator::GEN_POPUPS_MESSAGE));
			break;
		default:
			commentFix = mir_tstrdup(CTranslator::get(CTranslator::GEN_POPUPS_UNKNOWN));
			break;
	}
	return commentFix;
}

static int PopupUpdateT(HANDLE hContact, HANDLE hEvent)
{
	PLUGIN_DATAT 	*pdata;
	DBEVENTINFO 	dbe;
	TCHAR 			lpzText[MAX_SECONDLINE] = _T("");
	TCHAR			timestamp[50] = _T("\0");
	TCHAR			formatTime[50] = _T("\0");
	int 			iEvent = 0;
	TCHAR			*p = lpzText;
	int  			available = 0, i;
	TCHAR			*szPreview = NULL;

	pdata = (PLUGIN_DATAT *)PopUpList[NumberPopupData(hContact)];

	ZeroMemory((void *)&dbe, sizeof(dbe));

	if (hEvent) {
		if (pdata->pluginOptions->bShowHeaders) {
			mir_sntprintf(pdata->szHeader, safe_sizeof(pdata->szHeader), _T("[b]%s %d[/b]\n"),
						  CTranslator::get(CTranslator::GEN_POPUPS_NEW), pdata->nrMerged + 1);
			pdata->szHeader[255] = 0;
		}
		ZeroMemory(&dbe, sizeof(dbe));
		dbe.cbSize = sizeof(dbe);
		if (pdata->pluginOptions->bPreview && hContact) {
			dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
			dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
		}
		CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);
		if (pdata->pluginOptions->bShowDate || pdata->pluginOptions->bShowTime) {
			formatTime[0] = 0;
			if (pdata->pluginOptions->bShowDate)
				_tcsncpy(formatTime, _T("%Y.%m.%d "), MAX_DATASIZE);
			if (pdata->pluginOptions->bShowTime)
				_tcsncat(formatTime, _T("%H:%M"), MAX_DATASIZE);
			_tcsftime(timestamp, MAX_DATASIZE, formatTime, _localtime32((__time32_t *)&dbe.timestamp));
			mir_sntprintf(pdata->eventData[pdata->nrMerged].szText, MAX_SECONDLINE, _T("\n[b][i]%s[/i][/b]\n"), timestamp);
		}
		szPreview = GetPreviewT(dbe.eventType, &dbe);
		if (szPreview) {
			_tcsncat(pdata->eventData[pdata->nrMerged].szText, szPreview, MAX_SECONDLINE);
			mir_free(szPreview);
		} else
			_tcsncat(pdata->eventData[pdata->nrMerged].szText, _T(" "), MAX_SECONDLINE);

		pdata->eventData[pdata->nrMerged].szText[MAX_SECONDLINE - 1] = 0;

		/*
		 * now, reassemble the popup text, make sure the *last* event is shown, and then show the most recent events
		 * for which there is enough space in the popup text
		 */

		available = MAX_SECONDLINE - 1;
		if (pdata->pluginOptions->bShowHeaders) {
			_tcsncpy(lpzText, pdata->szHeader, MAX_SECONDLINE);
			available -= lstrlen(pdata->szHeader);
		}
		for (i = pdata->nrMerged; i >= 0; i--) {
			available -= lstrlen(pdata->eventData[i].szText);
			if (available <= 0)
				break;
		}
		i = (available > 0) ? i + 1 : i + 2;
		for (; i <= pdata->nrMerged; i++) {
			_tcsncat(lpzText, pdata->eventData[i].szText, MAX_SECONDLINE);
		}
		pdata->eventData[pdata->nrMerged].hEvent = hEvent;
		pdata->eventData[pdata->nrMerged].timestamp = dbe.timestamp;
		pdata->nrMerged++;
		if (pdata->nrMerged >= pdata->nrEventsAlloced) {
			pdata->nrEventsAlloced += 5;
			pdata->eventData = (EVENT_DATAT *)realloc(pdata->eventData, pdata->nrEventsAlloced * sizeof(EVENT_DATAT));
		}
		if (dbe.pBlob)
			free(dbe.pBlob);

		//SendMessage(pdata->hWnd, WM_SETREDRAW, FALSE, 0);
		CallService(MS_POPUP_CHANGETEXTT, (WPARAM)pdata->hWnd, (LPARAM)lpzText);
		//SendMessage(pdata->hWnd, WM_SETREDRAW, TRUE, 0);
	}
	return 0;
}

static int PopupShowT(NEN_OPTIONS *pluginOptions, HANDLE hContact, HANDLE hEvent, UINT eventType)
{
	POPUPDATAT 		pud;
	PLUGIN_DATAT 	*pdata;
	DBEVENTINFO 	dbe;
	long 			iSeconds = 0;
	TCHAR 			*szPreview = NULL;
	DWORD 			codePage;

	//there has to be a maximum number of popups shown at the same time
	if (PopupCount >= MAX_POPUPS)
		return 2;

	if (!PluginConfig.g_PopupAvail)
		return 0;

	switch (eventType) {
		case EVENTTYPE_MESSAGE:
			pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
			pud.colorBack = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colBackMsg;
			pud.colorText = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colTextMsg;
			iSeconds = pluginOptions->iDelayMsg;
			break;
		default:
			return 1;
	}

	ZeroMemory(&dbe, sizeof(dbe));
	dbe.pBlob = NULL;
	dbe.cbSize = sizeof(dbe);

	// fix for a crash
	if (hEvent && (pluginOptions->bPreview || hContact == 0)) {
		dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
		dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
	} else
		dbe.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);

	if(hEvent == 0 && hContact == 0)
		dbe.szModule = Translate("Unknown module or contact");

	pdata = (PLUGIN_DATAT *)malloc(sizeof(PLUGIN_DATAT));
	ZeroMemory((void *)pdata, sizeof(PLUGIN_DATAT));

	pdata->eventType = eventType;
	pdata->hContact = hContact;
	pdata->pluginOptions = pluginOptions;
	pdata->pud = &pud;
	pdata->iSeconds = iSeconds; // ? iSeconds : pluginOptions->iDelayDefault;
	pud.iSeconds = pdata->iSeconds ? -1 : 0;

	//finally create the popup
	pud.lchContact = hContact;
	pud.PluginWindowProc = (WNDPROC)PopupDlgProc;
	pud.PluginData = pdata;

	codePage = M->GetDword(hContact, "ANSIcodepage", PluginConfig.m_LangPackCP);

	if (hContact)
		mir_sntprintf(pud.lptzContactName, MAX_CONTACTNAME, _T("%s"), (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
	else {
		TCHAR *szModule = mir_a2t(dbe.szModule);
		mir_sntprintf(pud.lptzContactName, MAX_CONTACTNAME, _T("%s"), szModule);
		mir_free(szModule);
	}

	szPreview = GetPreviewT((WORD)eventType, &dbe);
	if (szPreview) {
		mir_sntprintf(pud.lptzText, MAX_SECONDLINE, _T("%s"), szPreview);
		mir_free(szPreview);
	} else
		mir_sntprintf(pud.lptzText, MAX_SECONDLINE, _T(" "));

	pdata->eventData = (EVENT_DATAT *)malloc(NR_MERGED * sizeof(EVENT_DATAT));
	pdata->eventData[0].hEvent = hEvent;
	pdata->eventData[0].timestamp = dbe.timestamp;
	_tcsncpy(pdata->eventData[0].szText, pud.lptzText, MAX_SECONDLINE);
	pdata->eventData[0].szText[MAX_SECONDLINE - 1] = 0;
	pdata->nrEventsAlloced = NR_MERGED;
	pdata->nrMerged = 1;

	PopupCount++;

	PopUpList[NumberPopupData(NULL)] = (PLUGIN_DATAT *)pdata;

	// fix for broken popups -- process failures
	if (CallService(MS_POPUP_ADDPOPUPT, (WPARAM)&pud, 0) < 0) {
		// failed to display, perform cleanup
		PopUpList[NumberPopupData(pdata->hContact)] = NULL;
		PopupCount--;
		if (pdata->eventData)
			free(pdata->eventData);
		free(pdata);
	}

	if (dbe.pBlob)
		free(dbe.pBlob);

	return 0;
}

static int PopupPreview(NEN_OPTIONS *pluginOptions)
{
	PopupShowT(pluginOptions, NULL, NULL, EVENTTYPE_MESSAGE);

	return 0;
}

/*
 * announce via either balloon tooltip or OSD plugin service.
 * wParam = hContact
 * lParam = handle of the database event to announce
 * truncates the announce string at 255 characters (balloon tooltip limitation)
 */

static int tabSRMM_ShowBalloon(WPARAM wParam, LPARAM lParam, UINT eventType)
{
	DBEVENTINFO 	dbei = {0};
	TCHAR 			*szPreview = 0;
	NOTIFYICONDATA 	nim;
	TCHAR 			*nickName = NULL;
	int 			iPreviewLimit = nen_options.iLimitPreview;

	if (iPreviewLimit > 255 || iPreviewLimit == 0)
		iPreviewLimit = 255;

	ZeroMemory((void *)&nim, sizeof(nim));
	nim.cbSize = sizeof(nim);

	nim.hWnd = PluginConfig.g_hwndHotkeyHandler;
	nim.uID = 100;
	nim.uFlags = NIF_ICON | NIF_INFO;
	nim.hIcon = PluginConfig.g_iconContainer;
	nim.uCallbackMessage = DM_TRAYICONNOTIFY;
	nim.uTimeout = 10000;
	nim.dwInfoFlags = NIIF_INFO;

	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) lParam, 0);
	if (dbei.cbBlob == -1)
		return 0;
	dbei.pBlob = (PBYTE) malloc(dbei.cbBlob);
	CallService(MS_DB_EVENT_GET, (WPARAM) lParam, (LPARAM) & dbei);
	szPreview = GetPreviewT((WORD)eventType, &dbei);
	nickName = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)wParam, GCDNF_TCHAR);
	if (nickName) {
		if (lstrlen(nickName) >= 30)
			mir_sntprintf(nim.szInfoTitle, 64, _T("%27.27s..."), nickName);
		else
			mir_sntprintf(nim.szInfoTitle, 64, _T("%s"), nickName);
	} else
		mir_sntprintf(nim.szInfoTitle, 64, _T("No Nickname"));

	if (eventType == EVENTTYPE_MESSAGE) {
		if(szPreview)
			mir_sntprintf(nim.szInfo, 255, _T("%s"), szPreview);
		else
			mir_sntprintf(nim.szInfo, 255, _T("%s"), _T(""));
	} else {
		if(dbei.pBlob)
			free(dbei.pBlob);

		if(szPreview)
			mir_free(szPreview);

		return(0);
	}

	PluginConfig.m_TipOwner = (HANDLE)wParam;
	Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA *)&nim);

	if(dbei.pBlob)
		free(dbei.pBlob);

	if(szPreview)
		mir_free(szPreview);
	return 0;
}

/*
 * updates the menu entry...
 * bForced is used to only update the status, nickname etc. and does NOT update the unread count
 */
void UpdateTrayMenuState(struct _MessageWindowData *dat, BOOL bForced)
{
	MENUITEMINFO	mii = {0};
	TCHAR			szMenuEntry[80];

	if (PluginConfig.g_hMenuTrayUnread == 0)
		return;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_DATA | MIIM_BITMAP;

	if (dat->hContact != 0) {
		TCHAR *tszProto = dat->szAccount;

		assert(tszProto != 0);

		GetMenuItemInfo(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, FALSE, &mii);
		if (!bForced)
			PluginConfig.m_UnreadInTray -= (mii.dwItemData & 0x0000ffff);
		if (mii.dwItemData > 0 || bForced) {
			if (!bForced)
				mii.dwItemData = 0;
			mii.fMask |= MIIM_STRING;
			mir_sntprintf(szMenuEntry, safe_sizeof(szMenuEntry), _T("%s: %s (%s) [%d]"), tszProto, dat->szNickname, dat->szStatus[0] ? dat->szStatus : _T("(undef)"), mii.dwItemData & 0x0000ffff);
			mii.dwTypeData = (LPTSTR)szMenuEntry;
			mii.cch = lstrlen(szMenuEntry) + 1;
		}
		mii.hbmpItem = HBMMENU_CALLBACK;
		SetMenuItemInfo(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, FALSE, &mii);
	}
}
/*
 * if we want tray support, add the contact to the list of unread sessions in the tray menu
 */

int UpdateTrayMenu(const _MessageWindowData *dat, WORD wStatus, const char *szProto, const TCHAR *szStatus, HANDLE hContact, DWORD fromEvent)
{
	if (PluginConfig.g_hMenuTrayUnread != 0 && hContact != 0 && szProto != NULL) {
		TCHAR			szMenuEntry[80], *tszFinalProto = NULL;
		MENUITEMINFO	mii = {0};
		WORD			wMyStatus;
		const TCHAR		*szMyStatus;
		const TCHAR		*szNick = NULL;

		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_DATA | MIIM_ID | MIIM_BITMAP;

		if (szProto == NULL)
			return 0;                                     // should never happen...

		PROTOACCOUNT *acc = (PROTOACCOUNT *)CallService(MS_PROTO_GETACCOUNT, (WPARAM)0, (LPARAM)szProto);

		tszFinalProto = (acc && acc->tszAccountName ? acc->tszAccountName : 0);

		if(tszFinalProto == 0)
			return(0);									// should also NOT happen

		wMyStatus = (wStatus == 0) ? DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE) : wStatus;
		szMyStatus = (szStatus == NULL) ? (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)wMyStatus, GCMDF_TCHAR) : szStatus;
		mii.wID = (UINT)hContact;
		mii.hbmpItem = HBMMENU_CALLBACK;

		if (dat != 0) {
			szNick = dat->szNickname;
			GetMenuItemInfo(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
			mii.dwItemData++;
			if (fromEvent == 2)                         // from chat...
				mii.dwItemData |= 0x10000000;
			DeleteMenu(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)hContact, MF_BYCOMMAND);
			mir_sntprintf(szMenuEntry, safe_sizeof(szMenuEntry), _T("%s: %s (%s) [%d]"), tszFinalProto, szNick, szMyStatus, mii.dwItemData & 0x0000ffff);
			AppendMenu(PluginConfig.g_hMenuTrayUnread, MF_BYCOMMAND | MF_STRING, (UINT_PTR)hContact, szMenuEntry);
			PluginConfig.m_UnreadInTray++;
			if (PluginConfig.m_UnreadInTray)
				SetEvent(g_hEvent);
			SetMenuItemInfo(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
		} else {
			UINT codePage = M->GetDword(hContact, "ANSIcodepage", PluginConfig.m_LangPackCP);
			szNick = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);
			if (CheckMenuItem(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)hContact, MF_BYCOMMAND | MF_UNCHECKED) == -1) {
				mir_sntprintf(szMenuEntry, safe_sizeof(szMenuEntry), _T("%s: %s (%s) [%d]"), tszFinalProto, szNick, szMyStatus, fromEvent ? 1 : 0);
				AppendMenu(PluginConfig.g_hMenuTrayUnread, MF_BYCOMMAND | MF_STRING, (UINT_PTR)hContact, szMenuEntry);
				mii.dwItemData = fromEvent ? 1 : 0;
				PluginConfig.m_UnreadInTray += (mii.dwItemData & 0x0000ffff);
				if (PluginConfig.m_UnreadInTray)
					SetEvent(g_hEvent);
				if (fromEvent == 2)
					mii.dwItemData |= 0x10000000;
			} else {
				GetMenuItemInfo(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
				mii.dwItemData += (fromEvent ? 1 : 0);
				PluginConfig.m_UnreadInTray += (fromEvent ? 1 : 0);
				if (PluginConfig.m_UnreadInTray)
					SetEvent(g_hEvent);
				mii.fMask |= MIIM_STRING;
				if (fromEvent == 2)
					mii.dwItemData |= 0x10000000;
				mir_sntprintf(szMenuEntry, safe_sizeof(szMenuEntry), _T("%s: %s (%s) [%d]"), tszFinalProto, szNick, szMyStatus, mii.dwItemData & 0x0000ffff);
				mii.cch = lstrlen(szMenuEntry) + 1;
				mii.dwTypeData = (LPTSTR)szMenuEntry;
			}
			SetMenuItemInfo(PluginConfig.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
		}
	}
	return 0;
}


int tabSRMM_ShowPopup(WPARAM wParam, LPARAM lParam, WORD eventType, int windowOpen, struct ContainerWindowData *pContainer, HWND hwndChild, char *szProto, struct _MessageWindowData *dat)
{
	int heFlags;

	if (nen_options.iDisable)                         // no popups at all. Period
		return 0;

	heFlags = HistoryEvents_GetFlags(eventType);
	if (heFlags != -1 && !(heFlags & HISTORYEVENTS_FLAG_DEFAULT)) // Filter history events popups
		return 0;

	/*
	 * check the status mode against the status mask
	 */

	if (nen_options.bSimpleMode) {
		if (nen_options.bNoRSS && szProto != NULL && !strncmp(szProto, "RSS", 3))
			return 0;
		switch (nen_options.bSimpleMode) {
			case 1:
				goto passed;
			case 3:
				if (!windowOpen)
					goto passed;
				else
					return 0;
			case 2:
				if (!windowOpen)
					goto passed;
				if (pContainer != NULL) {
					if (IsIconic(pContainer->hwnd) || GetForegroundWindow() != pContainer->hwnd)
						goto passed;
				}
				return 0;
			default:
				return 0;
		}
	}

	if (nen_options.dwStatusMask != -1) {
		DWORD dwStatus = 0;
		if (szProto != NULL) {
			dwStatus = (DWORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
			if (!(dwStatus == 0 || dwStatus <= ID_STATUS_OFFLINE || ((1 << (dwStatus - ID_STATUS_ONLINE)) & nen_options.dwStatusMask)))           // should never happen, but...
				return 0;
		}
	}
	if (nen_options.bNoRSS && szProto != NULL && !strncmp(szProto, "RSS", 3))
		return 0;                                        // filter out RSS popups
	//
	if (windowOpen && pContainer != 0) {               // message window is open, need to check the container config if we want to see a popup nonetheless
		if (nen_options.bWindowCheck)                  // no popups at all for open windows... no exceptions
			return 0;
		if (pContainer->dwFlags & CNT_DONTREPORT && (IsIconic(pContainer->hwnd)))        // in tray counts as "minimised"
			goto passed;
		if (pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED) {
			if (!IsIconic(pContainer->hwnd) && GetForegroundWindow() != pContainer->hwnd && GetActiveWindow() != pContainer->hwnd)
				goto passed;
		}
		if (pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE) {
			if (pContainer->hwndActive == hwndChild)
				return 0;
			else
				goto passed;
		}
		return 0;
	}
passed:
	if ((nen_options.bTraySupport && nen_options.iAnnounceMethod == 2) || nen_options.iAnnounceMethod == 3) {
		tabSRMM_ShowBalloon(wParam, lParam, (UINT)eventType);
		return 0;
	}
	if (NumberPopupData((HANDLE)wParam) != -1 && nen_options.bMergePopup && eventType == EVENTTYPE_MESSAGE)
		PopupUpdateT((HANDLE)wParam, (HANDLE)lParam);
	else
		PopupShowT(&nen_options, (HANDLE)wParam, (HANDLE)lParam, (UINT)eventType);

	return 0;
}

/*
 * remove all popups for hContact, but only if the mask matches the current "mode"
 */

void DeletePopupsForContact(HANDLE hContact, DWORD dwMask)
{
	int i = 0;

	if (!(dwMask & nen_options.dwRemoveMask) || nen_options.iDisable || !PluginConfig.g_PopupAvail)
		return;

	while ((i = NumberPopupData(hContact)) != -1) {
		if (PopUpList[i]->hWnd != 0 && IsWindow(PopUpList[i]->hWnd))
			PUDeletePopUp(PopUpList[i]->hWnd);
		PopUpList[i] = NULL;
	}
}
