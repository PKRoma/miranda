/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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

selectcontainer.c  dialog procedure for the "select/create container" dialog
                   box..
                   (C) 2004 by Nightwish, this file ispart of tabSRMM, a tabbed
                   message module for Miranda 0.3.3 and higher.
$Id: selectcontainer.c 9205 2009-03-24 05:00:43Z nightwish2004 $
*/

#include "commonheaders.h"
#pragma hdrstop

extern      struct ContainerWindowData *pFirstContainer;

INT_PTR CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndMsgDlg = 0;

	hwndMsgDlg = (HWND) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
		case WM_INITDIALOG: {
			TCHAR szNewTitle[128];
			RECT rc, rcParent;
			struct ContainerWindowData *pContainer = 0;

			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) lParam);
			hwndMsgDlg = (HWND) lParam;

			TranslateDialogDefault(hwndDlg);

			if (lParam) {
				struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
				if (dat) {
					mir_sntprintf(szNewTitle, safe_sizeof(szNewTitle), TranslateT("Select container for %s"), dat->szNickname);
					SetWindowText(hwndDlg, szNewTitle);
				}
			}

			SendMessage(hwndDlg, DM_SC_BUILDLIST, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_NEWCONTAINERNAME, EM_LIMITTEXT, (WPARAM)CONTAINER_NAMELEN, 0);
			SendDlgItemMessage(hwndDlg, IDC_NEWCONTAINER, EM_LIMITTEXT, (WPARAM)CONTAINER_NAMELEN, 0);

			GetWindowRect(hwndDlg, &rc);
			GetWindowRect(GetParent(hwndDlg), &rcParent);
			SetWindowPos(hwndDlg, 0, (rcParent.left + rcParent.right - (rc.right - rc.left)) / 2, (rcParent.top + rcParent.bottom - (rc.bottom - rc.top)) / 2, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					TCHAR szName[CONTAINER_NAMELEN];
					LRESULT iItem;

					if ((iItem = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETCURSEL, 0, 0)) != LB_ERR) {
						SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETTEXT, (WPARAM) iItem, (LPARAM) szName);
						if (IsWindow(hwndMsgDlg))
							SendMessage(hwndMsgDlg, DM_CONTAINERSELECTED, 0, (LPARAM) szName);
					}
					if (IsWindow(hwndDlg))
						DestroyWindow(hwndDlg);
					break;
				}
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
				case IDC_DELETECONTAINER: {
					TCHAR szName[CONTAINER_NAMELEN + 1];
					LRESULT iItem;

					if ((iItem = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETCURSEL, 0, 0)) != LB_ERR) {
						SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETTEXT, (WPARAM) iItem, (LPARAM) szName);
						if (!_tcsncmp(szName, _T("default"), CONTAINER_NAMELEN))
							MessageBoxA(hwndDlg, Translate("You cannot delete the default container"), "Error", MB_OK);
						else {
							int iIndex = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETITEMDATA, (WPARAM)iItem, 0);
							DeleteContainer(iIndex);
							SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_RESETCONTENT, 0, 0);
							SendMessage(hwndDlg, DM_SC_BUILDLIST, 0, 0);
							BuildContainerMenu();
						}
					}
					break;
				}
				case IDC_RENAMECONTAINER: {
					TCHAR szNewName[CONTAINER_NAMELEN], szName[CONTAINER_NAMELEN + 1];
					int iLen, iItem;
					struct ContainerWindowData *pCurrent = pFirstContainer;

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NEWCONTAINERNAME));
					if (iLen) {
						GetWindowText(GetDlgItem(hwndDlg, IDC_NEWCONTAINERNAME), szNewName, CONTAINER_NAMELEN * sizeof(TCHAR));
						iItem = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_FINDSTRING, (WPARAM) - 1, (LPARAM) szNewName);
						if (iItem != LB_ERR) {
							TCHAR szOldName[CONTAINER_NAMELEN + 1];
							SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETTEXT, (WPARAM) iItem, (LPARAM) szOldName);
							if (_tcslen(szOldName) == _tcslen(szNewName)) {
								MessageBoxA(0, Translate("This name is already in use"), "Error", MB_OK);
								SetFocus(GetDlgItem(hwndDlg, IDC_NEWCONTAINERNAME));
								break;
							}
						}
						if ((iItem = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETCURSEL, 0, 0)) != LB_ERR) {
							SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETTEXT, (WPARAM) iItem, (LPARAM) szName);
							if (!_tcsncmp(szName, _T("default"), CONTAINER_NAMELEN))
								MessageBoxA(hwndDlg, Translate("You cannot rename the default container"), "Error", MB_OK);
							else {
								int iIndex = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETITEMDATA, (WPARAM)iItem, 0);
								RenameContainer(iIndex, szNewName);
								SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_RESETCONTENT, 0, 0);
								while (pCurrent) {
									if (!_tcsncmp(pCurrent->szName, szName, CONTAINER_NAMELEN) && _tcslen(pCurrent->szName) == _tcslen(szName)) {
										_tcsncpy(pCurrent->szName, szNewName, CONTAINER_NAMELEN);
										SendMessage(pCurrent->hwnd, DM_CONFIGURECONTAINER, 0, 0);
									}
									pCurrent = pCurrent->pNextContainer;
								}
								SendMessage(hwndDlg, DM_SC_BUILDLIST, 0, 0);
								BuildContainerMenu();
							}
						}
					}
					break;
				}
				case IDC_CREATENEW: {
					int iLen, iItem;
					TCHAR szNewName[CONTAINER_NAMELEN], szName[CONTAINER_NAMELEN + 1];

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NEWCONTAINER));
					if (iLen) {
						GetWindowText(GetDlgItem(hwndDlg, IDC_NEWCONTAINER), szNewName, CONTAINER_NAMELEN * sizeof(TCHAR));
						iItem = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_FINDSTRING, (WPARAM) - 1, (LPARAM) szNewName);
						if (iItem != LB_ERR) {
							SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_GETTEXT, (WPARAM)iItem, (LPARAM)szName);
							if (_tcslen(szName) == _tcslen(szNewName)) {
								MessageBoxA(0, Translate("This name is already in use"), "Error", MB_OK);
								SetFocus(GetDlgItem(hwndDlg, IDC_NEWCONTAINER));
								break;
							}
						}
						if (IsWindow(hwndMsgDlg)) {
							SendMessage(hwndMsgDlg, DM_CONTAINERSELECTED, 0, (LPARAM) szNewName);
							if (IsWindow(hwndDlg))
								DestroyWindow(hwndDlg);
						}
					}
					break;
				}
				case IDC_CNTLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
					break;
			}
			break;
			/*
			 * fill the list box...
			 */
		case DM_SC_BUILDLIST: {
			DBVARIANT dbv;
			int iCounter = 0, iItemNew;
#if defined(_UNICODE)
			char *szKey = "TAB_ContainersW";
#else
			char *szKey = "TAB_Containers";
#endif
			char szValue[10];
			struct ContainerWindowData *pContainer = 0;
			do {
				_snprintf(szValue, 8, "%d", iCounter);
				if (DBGetContactSettingTString(NULL, szKey, szValue, &dbv))
					break;          // end of list
				if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR) {
					if (_tcsncmp(dbv.ptszVal, _T("**free**"), CONTAINER_NAMELEN)) {
						iItemNew = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_ADDSTRING, 0, (LPARAM)dbv.ptszVal);
						if (iItemNew != LB_ERR)
							SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_SETITEMDATA, (WPARAM)iItemNew, (LPARAM)iCounter);
					}
					DBFreeVariant(&dbv);
				}
			} while (++iCounter);

			/*
			 * highlight the name of the container to which the message window currently is assigned
			 */

			SendMessage(hwndMsgDlg, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
			if (pContainer) {
				LRESULT iItem;

				iItem = SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_FINDSTRING, (WPARAM) - 1, (LPARAM) pContainer->szName);
				if (iItem != LB_ERR)
					SendDlgItemMessage(hwndDlg, IDC_CNTLIST, LB_SETCURSEL, (WPARAM) iItem, 0);
			}
		}
		break;
	}
	return FALSE;
}

