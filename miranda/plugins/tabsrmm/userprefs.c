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

extern MYGLOBALS myGlobals;
extern HANDLE hMessageWindowList, hUserPrefsWindowList;
extern struct CPTABLE cpTable[];

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

BOOL CALLBACK DlgProcUserPrefs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
			BYTE bOverride = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "mwoverride", 0);
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

			WindowList_Add(hUserPrefsWindowList, hwndDlg, hContact);
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

			SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use Default"));
			SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always LTR"));
			SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always RTL"));
			SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_SETCURSEL, (bLTR == 1 && bRTL == 0) ? 0 : (bLTR == 0 ? 1 : 2), 0);

			if (CheckMenuItem(myGlobals.g_hMenuFavorites, (UINT_PTR)lParam, MF_BYCOMMAND | MF_UNCHECKED) == -1)
				CheckDlgButton(hwndDlg, IDC_ISFAVORITE, FALSE);
			else
				CheckDlgButton(hwndDlg, IDC_ISFAVORITE, TRUE);

			CheckDlgButton(hwndDlg, IDC_LOGISGLOBAL, bOverride == 0);
			CheckDlgButton(hwndDlg, IDC_LOGISPRIVATE, bOverride != 0);
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
			mir_sntprintf(szBuffer, safe_sizeof(szBuffer), TranslateT("Set options for %s"), (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
			SetWindowText(hwndDlg, szBuffer);
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
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
				case IDC_ALWAYSTRIM2:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					break;
				case IDOK: {
					struct MessageWindowData *dat = 0;
					int iIndex = CB_ERR;
					DWORD newCodePage;
					int offset;
					unsigned int iOldIEView;
					HWND hWnd = WindowList_Find(hMessageWindowList, hContact);
					DWORD sCodePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", 0);
					DWORD oldTZ = (DWORD)DBGetContactSettingByte(hContact, "UserInfo", "Timezone", DBGetContactSettingByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone", -1));
					BYTE bInfoPanel, bOldInfoPanel = DBGetContactSettingByte(hContact, SRMSGMOD_T, "infopanel", 0);

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
							if (iNewIEView != iOldIEView)
								SwitchMessageLog(hWnd, dat, iNewIEView);
						}
					}
					if ((iIndex = SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_GETCURSEL, 0, 0)) != CB_ERR) {
						if (iIndex == 0)
							DBDeleteContactSetting(hContact, SRMSGMOD_T, "RTL");
						else
							DBWriteContactSettingByte(hContact, SRMSGMOD_T, "RTL", (BYTE)(iIndex == 1 ? 0 : 1));
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_LOGISGLOBAL)) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "mwoverride", 0);
						if (hWnd && dat) {
							SendMessage(hWnd, DM_OPTIONSAPPLIED, 1, 0);
						}
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_LOGISPRIVATE)) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "mwoverride", 1);
						if (hWnd && dat) {
							dat->dwFlags &= ~(MWF_LOG_ALL);
							dat->dwFlags = DBGetContactSettingDword(hContact, SRMSGMOD_T, "mwflags", DBGetContactSettingDword(0, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT));
							SendMessage(hWnd, DM_OPTIONSAPPLIED, 0, 0);
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
		case WM_DESTROY:
			WindowList_Remove(hUserPrefsWindowList, hwndDlg);
			break;
	}
	return FALSE;
}

