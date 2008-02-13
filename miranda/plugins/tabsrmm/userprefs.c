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

$Id$

Dialog to setup per contact (user) prefs.
Invoked by the contact menu entry, handled by the SetUserPrefs Service
function.

Sets things like:

* global/local message log options
* local (per user) template overrides
* view mode (ieview/default)
* text formatting

*/

#include "commonheaders.h"
#pragma hdrstop
#include "uxtheme.h"

#define UPREF_ACTION_APPLYOPTIONS 1
#define UPREF_ACTION_REMAKELOG 2
#define UPREF_ACTION_SWITCHLOGVIEWER 4

extern		MYGLOBALS myGlobals;
extern		HANDLE hMessageWindowList, hUserPrefsWindowList;
extern		struct CPTABLE cpTable[];
extern		BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD);

static HWND hCpCombo;

static BOOL CALLBACK FillCpCombo(LPCTSTR str)
{
	int i;
	UINT cp;

	cp = _ttoi(str);
	for (i = 0; cpTable[i].cpName != NULL && cpTable[i].cpId != cp; i++);
	if (cpTable[i].cpName != NULL) {
		LRESULT iIndex = SendMessage(hCpCombo, CB_ADDSTRING, -1, (LPARAM) TranslateTS(cpTable[i].cpName));
		SendMessage(hCpCombo, CB_SETITEMDATA, (WPARAM)iIndex, cpTable[i].cpId);
	}
	return TRUE;
}

static int have_ieview = 0, have_hpp = 0;

static BOOL CALLBACK DlgProcUserPrefs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG: {
			TCHAR szBuffer[180];
#if defined(_UNICODE)
			DWORD sCodePage;
#endif
			DWORD contact_gmt_diff;
			int i, offset;
			DWORD maxhist = DBGetContactSettingDword((HANDLE)lParam, SRMSGMOD_T, "maxhist", 0);
			BYTE bIEView = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "ieview", 0);
			BYTE bHPP = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "hpplog", 0);
			int iLocalFormat = DBGetContactSettingDword((HANDLE)lParam, SRMSGMOD_T, "sendformat", 0);
			BYTE bRTL = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "RTL", 0);
			BYTE bLTR = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "RTL", 1);
			BYTE bSplit = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "splitoverride", 0);
			BYTE bInfoPanel = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "infopanel", 0);
			char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)lParam, 0);
			int  def_log_index = 1, hpp_log_index = 1, ieview_log_index = 1;

			have_ieview = ServiceExists(MS_IEVIEW_WINDOW);
			have_hpp = ServiceExists("History++/ExtGrid/NewWindow");
  
			hContact = (HANDLE)lParam;

			TranslateDialogDefault(hwndDlg);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)lParam);

			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use Global Setting"));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always On"));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Off"));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_SETCURSEL, bInfoPanel == 0 ? 0 : (bInfoPanel == 1 ? 1 : 2), 0);

			SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use global Setting"));
			SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)(have_hpp ? TranslateT("Force History++") : TranslateT("Force History++ (plugin missing)")));
			SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)(have_ieview ? TranslateT("Force IEView") : TranslateT("Force IEView (plugin missing)")));
			SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Force Default Message Log"));

			if (bIEView == 0xff && bHPP == 0xff)
				SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_SETCURSEL, 3, 0);
			else if (bIEView == 1)
				SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_SETCURSEL, 2, 0);
			else if (bHPP == 1)
				SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_SETCURSEL, 1, 0);
			else
				SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_SETCURSEL, 0, 0);

			SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Global Setting"));
			SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Simple Tags (*/_)"));
			SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("BBCode"));
			SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Force Off"));
			SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_SETCURSEL, iLocalFormat == 0 ? 0 : (iLocalFormat == -1 ? 3 : (iLocalFormat == SENDFORMAT_BBCODE ? 1 : 2)), 0);

			if (CheckMenuItem(myGlobals.g_hMenuFavorites, (UINT_PTR)lParam, MF_BYCOMMAND | MF_UNCHECKED) == -1)
				CheckDlgButton(hwndDlg, IDC_ISFAVORITE, FALSE);
			else
				CheckDlgButton(hwndDlg, IDC_ISFAVORITE, TRUE);

			CheckDlgButton(hwndDlg, IDC_PRIVATESPLITTER, bSplit);
			CheckDlgButton(hwndDlg, IDC_TEMPLOVERRIDE, DBGetContactSettingByte(hContact, TEMPLATES_MODULE, "enabled", 0));
			CheckDlgButton(hwndDlg, IDC_RTLTEMPLOVERRIDE, DBGetContactSettingByte(hContact, RTLTEMPLATES_MODULE, "enabled", 0));

			SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 5));
			SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETPOS, 0, maxhist);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), maxhist != 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), maxhist != 0);
			CheckDlgButton(hwndDlg, IDC_ALWAYSTRIM2, maxhist != 0);

#if defined(_UNICODE)
			hCpCombo = GetDlgItem(hwndDlg, IDC_CODEPAGES);
			sCodePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", 0);
			EnumSystemCodePages(FillCpCombo, CP_INSTALLED);
			SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_INSERTSTRING, 0, (LPARAM)TranslateT("Use default codepage"));
			if (sCodePage == 0)
				SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_SETCURSEL, (WPARAM)0, 0);
			else {
				for (i = 0; i < SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETCOUNT, 0, 0); i++) {
					if (SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETITEMDATA, (WPARAM)i, 0) == (LRESULT)sCodePage)
						SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_SETCURSEL, (WPARAM)i, 0);
				}
			}
			CheckDlgButton(hwndDlg, IDC_FORCEANSI, DBGetContactSettingByte(hContact, SRMSGMOD_T, "forceansi", 0) ? 1 : 0);
#else
			EnableWindow(GetDlgItem(hwndDlg, IDC_CODEPAGES), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_FORCEANSI), FALSE);
#endif
			CheckDlgButton(hwndDlg, IDC_IGNORETIMEOUTS, DBGetContactSettingByte(hContact, SRMSGMOD_T, "no_ack", 0));
			SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("<default, no change>"));
			timezone = (DWORD)DBGetContactSettingByte(hContact, "UserInfo", "Timezone", DBGetContactSettingByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone", -1));
			for (i = -12; i <= 12; i++) {
				_sntprintf(szBuffer, 20, TranslateT("GMT %c %d"), i < 0 ? '-' : '+', abs(i));
				SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_INSERTSTRING, -1, (LPARAM)szBuffer);
			}
			if (timezone != -1) {
				contact_gmt_diff = timezone > 128 ? 256 - timezone : 0 - timezone;
				offset = 13 + ((int)contact_gmt_diff / 2);
				SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_SETCURSEL, (WPARAM)offset, 0);
			} else
				SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_SETCURSEL, 0, 0);
			ShowWindow(hwndDlg, SW_SHOW);
			CheckDlgButton(hwndDlg, IDC_NOAUTOCLOSE, DBGetContactSettingByte(hContact, SRMSGMOD_T, "NoAutoClose", 0));
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_ALWAYSTRIM2:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					break;
				case WM_USER + 100: {
					struct	MessageWindowData *dat = 0;
					DWORD	*pdwActionToTake = (DWORD *)lParam;
					int		iIndex = CB_ERR;
					DWORD	newCodePage;
					int		offset;
					unsigned int iOldIEView;
					HWND	hWnd = WindowList_Find(hMessageWindowList, hContact);
					DWORD	sCodePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", 0);
					DWORD	oldTZ = (DWORD)DBGetContactSettingByte(hContact, "UserInfo", "Timezone", DBGetContactSettingByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone", -1));
					BYTE	bInfoPanel, bOldInfoPanel = DBGetContactSettingByte(hContact, SRMSGMOD_T, "infopanel", 0);

					if (hWnd) {
						dat = (struct MessageWindowData *)GetWindowLong(hWnd, GWL_USERDATA);
						if (dat)
							iOldIEView = GetIEViewMode(hWnd, dat);
					}

					if ((iIndex = SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_GETCURSEL, 0, 0)) != CB_ERR) {
						unsigned int iNewIEView;

						switch (iIndex) {
							case 0:
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "ieview", 0);
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "hpplog", 0);
								break;
							case 1:
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "ieview", -1);
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "hpplog", 1);
								break;
							case 2:
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "ieview", 1);
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "hpplog", -1);
								break;
							case 3:
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "ieview", -1);
								DBWriteContactSettingByte(hContact, SRMSGMOD_T, "hpplog", -1);
								break;
							default:
								break;
						}
						if (hWnd && dat) {
							iNewIEView = GetIEViewMode(hWnd, dat);
							if (iNewIEView != iOldIEView) {
								if(pdwActionToTake)
									*pdwActionToTake |= UPREF_ACTION_SWITCHLOGVIEWER;
							}
						}
					}
					if ((iIndex = SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_GETCURSEL, 0, 0)) != CB_ERR) {
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "sendformat", iIndex == 3 ? -1 : iIndex);
						if (iIndex == 0)
							DBDeleteContactSetting(hContact, SRMSGMOD_T, "sendformat");
					}
#if defined(_UNICODE)
					iIndex = SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETCURSEL, 0, 0);
					if ((newCodePage = (DWORD)SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETITEMDATA, (WPARAM)iIndex, 0)) != sCodePage) {
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", (DWORD)newCodePage);
						if (hWnd && dat) {
							dat->codePage = newCodePage;
							dat->wOldStatus = 0;
							SendMessage(hWnd, DM_UPDATETITLE, 0, 0);
						}
					}
					if ((IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? 1 : 0) != DBGetContactSettingByte(hContact, SRMSGMOD_T, "forceansi", 0)) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "forceansi", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? 1 : 0));
						if (hWnd && dat)
							dat->sendMode = IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? dat->sendMode | SMODE_FORCEANSI : dat->sendMode & ~SMODE_FORCEANSI;
					}
#else
					newCodePage = CP_ACP;
#endif
					if (IsDlgButtonChecked(hwndDlg, IDC_ISFAVORITE)) {
						if (!DBGetContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", 0))
							AddContactToFavorites(hContact, NULL, NULL, NULL, 0, 0, 1, myGlobals.g_hMenuFavorites, newCodePage);
					} else
						DeleteMenu(myGlobals.g_hMenuFavorites, (UINT_PTR)hContact, MF_BYCOMMAND);

					DBWriteContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", (WORD)(IsDlgButtonChecked(hwndDlg, IDC_ISFAVORITE) ? 1 : 0));
					DBWriteContactSettingByte(hContact, SRMSGMOD_T, "splitoverride", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_PRIVATESPLITTER) ? 1 : 0));

					DBWriteContactSettingByte(hContact, TEMPLATES_MODULE, "enabled", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_TEMPLOVERRIDE)));
					DBWriteContactSettingByte(hContact, RTLTEMPLATES_MODULE, "enabled", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_RTLTEMPLOVERRIDE)));

					offset = SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_GETCURSEL, 0, 0);
					if (offset > 0) {
						BYTE timezone = (13 - offset) * 2;
						if (timezone != (BYTE)oldTZ)
							DBWriteContactSettingByte(hContact, "UserInfo", "Timezone", (BYTE)timezone);
					} else
						DBDeleteContactSetting(hContact, "UserInfo", "Timezone");

					if (hWnd && dat) {
						LoadTimeZone(hWnd, dat);
						InvalidateRect(GetDlgItem(hWnd, IDC_PANELUIN), NULL, FALSE);
					}

					bInfoPanel = (BYTE)SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_GETCURSEL, 0, 0);
					if (bInfoPanel != bOldInfoPanel) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "infopanel", (BYTE)(bInfoPanel == 0 ? 0 : (bInfoPanel == 1 ? 1 : -1)));
						if (hWnd && dat)
							SendMessage(hWnd, DM_SETINFOPANEL, 0, 0);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2))
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "maxhist", (DWORD)SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_GETPOS, 0, 0));
					else
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "maxhist", 0);

					if (IsDlgButtonChecked(hwndDlg, IDC_IGNORETIMEOUTS)) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "no_ack", 1);
						if (hWnd && dat)
							dat->sendMode |= SMODE_NOACK;
					} else {
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "no_ack");
						if (hWnd && dat)
							dat->sendMode &= ~SMODE_NOACK;
					}
					if (hWnd && dat)
						SendMessage(hWnd, DM_CONFIGURETOOLBAR, 0, 1);

					if (IsDlgButtonChecked(hwndDlg, IDC_NOAUTOCLOSE))
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "NoAutoClose", 1);
					else
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "NoAutoClose");

					DestroyWindow(hwndDlg);
					break;
				}
				default:
					break;
			}
			break;
	}
	return FALSE;
}

static struct _checkboxes {
	UINT	uId;
	UINT	uFlag;
} checkboxes[] = {
	IDC_UPREFS_GRID, MWF_LOG_GRID,
	IDC_UPREFS_SHOWICONS, MWF_LOG_SHOWICONS,
	IDC_UPREFS_SHOWSYMBOLS, MWF_LOG_SYMBOLS,
	IDC_UPREFS_INOUTICONS, MWF_LOG_INOUTICONS,
	IDC_UPREFS_SHOWTIMESTAMP, MWF_LOG_SHOWTIME,
	IDC_UPREFS_SHOWDATES, MWF_LOG_SHOWDATES,
	IDC_UPREFS_SHOWSECONDS, MWF_LOG_SHOWSECONDS,
	IDC_UPREFS_LOCALTIME, MWF_LOG_LOCALTIME,
	IDC_UPREFS_INDENT, MWF_LOG_INDENT,
	IDC_UPREFS_GROUPING, MWF_LOG_GROUPMODE,
	IDC_UPREFS_MULTIPLEBG, MWF_LOG_INDIVIDUALBKG,
	IDC_UPREFS_BBCODE, MWF_LOG_BBCODE,
	IDC_UPREFS_RTL, MWF_LOG_RTL,
	IDC_UPREFS_LOGSTATUS, MWF_LOG_STATUSCHANGES,
	IDC_UPREFS_NORMALTEMPLATES, MWF_LOG_NORMALTEMPLATES,
	0, 0
};

static BOOL CALLBACK DlgProcUserPrefs1(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLong(hwndDlg, GWL_USERDATA);
	switch(msg) {
		case WM_INITDIALOG: {
			DWORD	dwGlobalFlags, dwLocalFlags, maskval;
			int		i = 0;

			hContact = (HANDLE)lParam;
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)hContact);

			dwGlobalFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
			dwLocalFlags = DBGetContactSettingDword(hContact, SRMSGMOD_T, "mwflags", dwGlobalFlags);

			while(checkboxes[i].uId) {
				maskval = checkboxes[i].uFlag;

				if((dwGlobalFlags & maskval) == (dwLocalFlags & maskval))
					CheckDlgButton(hwndDlg, checkboxes[i].uId, BST_INDETERMINATE);
				else 
					CheckDlgButton(hwndDlg, checkboxes[i].uId, (dwLocalFlags & maskval) ? BST_CHECKED : BST_UNCHECKED);
				i++;
			}
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case WM_USER + 100: {
					int i = 0;
					DWORD	dwGlobalFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT), dwLocalFlags = 0, maskval = 0;
					LRESULT state;
					HWND	hwnd = WindowList_Find(hMessageWindowList, hContact);
					struct	MessageWindowData *dat = NULL;
					DWORD	*dwActionToTake = (DWORD *)lParam, dwFinalFlags = 0;

					if(hwnd)
						dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);

					while(checkboxes[i].uId) {
						maskval = checkboxes[i].uFlag;

						state = IsDlgButtonChecked(hwndDlg, checkboxes[i].uId);
						if(state == BST_INDETERMINATE)
							dwLocalFlags |= (dwGlobalFlags & maskval);
						else
							dwLocalFlags = (state == BST_CHECKED) ? (dwLocalFlags | maskval) : (dwLocalFlags & ~maskval);
						i++;
					}
					if(dwLocalFlags != dwGlobalFlags) {
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "mwflags", dwLocalFlags & MWF_LOG_ALL);
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "mwoverride", 1);
						dwFinalFlags = dwLocalFlags;
					}
					else {
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "mwoverride");
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "mwflags");
						dwFinalFlags = dwGlobalFlags;
					}
					if(hwnd && dat) {
						if((dwFinalFlags & MWF_LOG_ALL) != (dat->dwFlags & MWF_LOG_ALL))
							*dwActionToTake |= (DWORD)UPREF_ACTION_REMAKELOG;
						if((dwFinalFlags & MWF_LOG_RTL) != (dat->dwFlags & MWF_LOG_RTL))
							*dwActionToTake |= (DWORD)UPREF_ACTION_APPLYOPTIONS;
					}
					break;
				}
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcUserPrefsFrame(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch(msg) {
		case WM_INITDIALOG: {
			TCITEM tci;
			RECT rcClient;
			TCHAR szBuffer[180];

			hContact = (HANDLE)lParam;
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)hContact);

			WindowList_Add(hUserPrefsWindowList, hwndDlg, hContact);
			TranslateDialogDefault(hwndDlg);

			GetClientRect(hwndDlg, &rcClient);

			mir_sntprintf(szBuffer, safe_sizeof(szBuffer), TranslateT("Set options for %s"), (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
			SetWindowText(hwndDlg, szBuffer);

			tci.mask = TCIF_PARAM | TCIF_TEXT;
			tci.lParam = (LPARAM)CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS), hwndDlg, DlgProcUserPrefs, (LPARAM)hContact);
			tci.pszText = TranslateT("General");
			TabCtrl_InsertItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), 0, &tci);
			MoveWindow((HWND)tci.lParam, 5, 32, rcClient.right - 10, rcClient.bottom - 80, 1);
			ShowWindow((HWND)tci.lParam, SW_SHOW);
			if (MyEnableThemeDialogTexture)
				MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);


			tci.lParam = (LPARAM)CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS1), hwndDlg, DlgProcUserPrefs1, (LPARAM)hContact);
			tci.pszText = TranslateT("Message Log");
			TabCtrl_InsertItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), 1, &tci);
			MoveWindow((HWND)tci.lParam, 5, 32, rcClient.right - 10, rcClient.bottom - 80, 1);
			ShowWindow((HWND)tci.lParam, SW_HIDE);
			if (MyEnableThemeDialogTexture)
				MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);
			TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), 0);
			ShowWindow(hwndDlg, SW_SHOW);
			return TRUE;
		}
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->idFrom) {
				case IDC_OPTIONSTAB:
					switch (((LPNMHDR)lParam)->code) {
						case TCN_SELCHANGING: {
							TCITEM tci;
							tci.mask = TCIF_PARAM;

							TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_OPTIONSTAB)), &tci);
							ShowWindow((HWND)tci.lParam, SW_HIDE);
						}
						break;
						case TCN_SELCHANGE: {
							TCITEM tci;
							tci.mask = TCIF_PARAM;

							TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_OPTIONSTAB)), &tci);
							ShowWindow((HWND)tci.lParam, SW_SHOW);
						}
						break;
					}
					break;
			}
			break;
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK: {
					TCITEM	tci;
					int		i, count;
					DWORD	dwActionToTake = 0;			// child pages request which action to take
					HWND	hwnd = WindowList_Find(hMessageWindowList, hContact);

					tci.mask = TCIF_PARAM;
					
					count = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_OPTIONSTAB));
					for (i = 0;i < count;i++) {
						TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), i, &tci);
						SendMessage((HWND)tci.lParam, WM_COMMAND, WM_USER + 100, (LPARAM)&dwActionToTake);
					}
					if(hwnd) {
						struct		MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
						unsigned	int uLogViewer;

						if(dat) {
							DWORD	dwNewFlags = 0, dwOldFlags = (dat->dwFlags & MWF_LOG_ALL);

							SetDialogToType(hwnd);
							if(dwActionToTake & UPREF_ACTION_SWITCHLOGVIEWER) {
								unsigned int mode = GetIEViewMode(hwndDlg, dat);
								SwitchMessageLog(hwnd, dat, mode);
							}

							if(DBGetContactSettingByte(hContact, SRMSGMOD_T, "mwoverride", 0))
								dwNewFlags = DBGetContactSettingDword(hContact, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL;
							else {
								if(dat->dwFlags & MWF_SHOW_PRIVATETHEME)
									dwNewFlags = dat->theme.dwFlags & MWF_LOG_ALL;
								else
									dwNewFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL;
							}
							if(dwNewFlags != dwOldFlags) {
								dat->dwFlags &= ~MWF_LOG_ALL;
								dat->dwFlags |= dwNewFlags;
								SendMessage(hwnd, DM_OPTIONSAPPLIED, 0, 0);
								SendMessage(hwnd, DM_DEFERREDREMAKELOG, (WPARAM)hwnd, 0);
							}
						}
					}
					DestroyWindow(hwndDlg);
					break;
				}
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
			}
			break;
		}
		case WM_DESTROY:
			WindowList_Remove(hUserPrefsWindowList, hwndDlg);
			break;
	}
	return FALSE;
}
