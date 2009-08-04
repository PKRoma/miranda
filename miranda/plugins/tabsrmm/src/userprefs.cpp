/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

$Id: userprefs.c 10399 2009-07-23 20:11:21Z silvercircle $

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
#include <uxtheme.h>

/*
 * FIXME (tz stuff needed to compile (taken from m_timezones.h)
 */

#define MIM_TZ_PLF_CB		1				// UI element is assumed to be a combo box
#define MIM_TZ_PLF_LB		2				// UI element is assumed to be a list box
#define MIM_TZ_NAMELEN 64
#define MIM_TZ_DISPLAYLEN 128

typedef struct _tagTimeZone {
	DWORD	cbSize;						// caller must supply this
	TCHAR	tszName[MIM_TZ_NAMELEN];				// windows name for the time zone
	TCHAR	tszDisplay[MIM_TZ_DISPLAYLEN];			// more descriptive display name (that's what usually appears in dialogs)
	LONG	Bias;						// Standardbias (gmt offset)
	LONG	DaylightBias;				// daylight Bias (dst offset, relative to standard bias, -60 for most time zones)
	SYSTEMTIME StandardTime;			// when DST ends (month/dayofweek/time)
	SYSTEMTIME DaylightTime;			// when DST begins (month/dayofweek/time)
	char	GMT_Offset;					// simple GMT offset (+/-, measured in half-hours, may be incorrect for DST timezones)
	LONG	Offset;						// time offset to local time, in seconds. It is relativ to the current local time, NOT GMT
										// the sign is inverted, so you have to subtract it from the current time.
	SYSTEMTIME CurrentTime;				// current system time. only updated when forced by the caller
	time_t	   now;						// same in unix time format (seconds since 1970).
} MIM_TIMEZONE;

typedef struct _tagPrepareList {
	DWORD	cbSize;									// caller must supply this
	HWND	hWnd;									// window handle of the combo or list box
	TCHAR	tszName[MIM_TZ_NAMELEN];				// tz name (for preselecting)
	DWORD	dwFlags;								// flags - if neither PLF_CB or PLF_LB is set, the window class name will be used
													// to figure out the type of control.
	HANDLE	hContact;								// contact handle (for preselecting)
} MIM_TZ_PREPARELIST;
#define MS_TZ_PREPARELIST "TZ/PrepareList"


#define UPREF_ACTION_APPLYOPTIONS 1
#define UPREF_ACTION_REMAKELOG 2
#define UPREF_ACTION_SWITCHLOGVIEWER 4

extern		HANDLE hUserPrefsWindowList;
extern		struct CPTABLE cpTable[];

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

static INT_PTR CALLBACK DlgProcUserPrefs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
		case WM_INITDIALOG: {
#if defined(_UNICODE)
			DWORD sCodePage;
#endif
			DWORD contact_gmt_diff, timediff;
			int i;
			BYTE timezone;
			DWORD maxhist = M->GetDword((HANDLE)lParam, "maxhist", 0);
			BYTE bIEView = M->GetByte((HANDLE)lParam, "ieview", 0);
			BYTE bHPP = M->GetByte((HANDLE)lParam, "hpplog", 0);
			int iLocalFormat = M->GetDword((HANDLE)lParam, "sendformat", 0);
			BYTE bRTL = M->GetByte((HANDLE)lParam, "RTL", 0);
			BYTE bLTR = M->GetByte((HANDLE)lParam, "RTL", 1);
			BYTE bSplit = M->GetByte((HANDLE)lParam, "splitoverride", 0);
			BYTE bInfoPanel = M->GetByte((HANDLE)lParam, "infopanel", 0);
			BYTE bAvatarVisible = M->GetByte((HANDLE)lParam, "hideavatar", -1);
			char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)lParam, 0);
			int  def_log_index = 1, hpp_log_index = 1, ieview_log_index = 1;

			have_ieview = ServiceExists(MS_IEVIEW_WINDOW);
			have_hpp = ServiceExists("History++/ExtGrid/NewWindow");

			hContact = (HANDLE)lParam;

			TranslateDialogDefault(hwndDlg);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);

			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use global Setting"));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always On"));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Off"));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_SETCURSEL, bInfoPanel == 0 ? 0 : (bInfoPanel == 1 ? 1 : 2), 0);

			SendDlgItemMessage(hwndDlg, IDC_SHOWAVATAR, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use global Setting"));
			SendDlgItemMessage(hwndDlg, IDC_SHOWAVATAR, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Show always (if present)"));
			SendDlgItemMessage(hwndDlg, IDC_SHOWAVATAR, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never show it at all"));
			SendDlgItemMessage(hwndDlg, IDC_SHOWAVATAR, CB_SETCURSEL, bAvatarVisible == 0xff ? 0 : (bAvatarVisible == 1 ? 1 : 2), 0);

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

			if (CheckMenuItem(PluginConfig.g_hMenuFavorites, (UINT_PTR)lParam, MF_BYCOMMAND | MF_UNCHECKED) == -1)
				CheckDlgButton(hwndDlg, IDC_ISFAVORITE, FALSE);
			else
				CheckDlgButton(hwndDlg, IDC_ISFAVORITE, TRUE);

			CheckDlgButton(hwndDlg, IDC_PRIVATESPLITTER, bSplit);
			CheckDlgButton(hwndDlg, IDC_TEMPLOVERRIDE, M->GetByte(hContact, TEMPLATES_MODULE, "enabled", 0));
			CheckDlgButton(hwndDlg, IDC_RTLTEMPLOVERRIDE, M->GetByte(hContact, RTLTEMPLATES_MODULE, "enabled", 0));

			//MAD
			CheckDlgButton(hwndDlg, IDC_LOADONLYACTUAL, M->GetByte(hContact, "ActualHistory", 0));
			//
			SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 5));
			SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETPOS, 0, maxhist);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), maxhist != 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), maxhist != 0);
			CheckDlgButton(hwndDlg, IDC_ALWAYSTRIM2, maxhist != 0);

#if defined(_UNICODE)
			hCpCombo = GetDlgItem(hwndDlg, IDC_CODEPAGES);
			sCodePage = M->GetDword(hContact, "ANSIcodepage", 0);
			EnumSystemCodePages((CODEPAGE_ENUMPROC)FillCpCombo, CP_INSTALLED);
			SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_INSERTSTRING, 0, (LPARAM)TranslateT("Use default codepage"));
			if (sCodePage == 0)
				SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_SETCURSEL, (WPARAM)0, 0);
			else {
				for (i = 0; i < SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETCOUNT, 0, 0); i++) {
					if (SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETITEMDATA, (WPARAM)i, 0) == (LRESULT)sCodePage)
						SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_SETCURSEL, (WPARAM)i, 0);
				}
			}
			CheckDlgButton(hwndDlg, IDC_FORCEANSI, M->GetByte(hContact, "forceansi", 0) ? 1 : 0);
#else
			EnableWindow(GetDlgItem(hwndDlg, IDC_CODEPAGES), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_FORCEANSI), FALSE);
#endif
			CheckDlgButton(hwndDlg, IDC_IGNORETIMEOUTS, M->GetByte(hContact, "no_ack", 0));
			timezone = M->GetByte(hContact, "UserInfo", "Timezone", M->GetByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone", -1));

			contact_gmt_diff = timezone > 128 ? 256 - timezone : 0 - timezone;
			timediff = /* (int)myGlobals.local_gmt_diff - */ - (int)contact_gmt_diff * 60 * 60 / 2;

			/*
			 * already populated and prepared
			 */

			if(ServiceExists(MS_TZ_PREPARELIST)) {
				MIM_TZ_PREPARELIST mtzp;

				ZeroMemory(&mtzp, sizeof(MS_TZ_PREPARELIST));
				mtzp.cbSize = sizeof(MIM_TZ_PREPARELIST);
				mtzp.hContact = hContact;
				mtzp.hWnd = GetDlgItem(hwndDlg, IDC_TIMEZONE);
				//mtzp.dwFlags = MIM_TZ_PLF_CB;
				CallService(MS_TZ_PREPARELIST, (WPARAM)0, (LPARAM)&mtzp);
			}
			else
				SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_ADDSTRING, 0, (LPARAM)_T("time zone service is missing"));

			ShowWindow(hwndDlg, SW_SHOW);
			CheckDlgButton(hwndDlg, IDC_NOAUTOCLOSE, M->GetByte(hContact, "NoAutoClose", 0));
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_ALWAYSTRIM2:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					break;
				case WM_USER + 100: {
					struct	_MessageWindowData *dat = 0;
					DWORD	*pdwActionToTake = (DWORD *)lParam;
					int		iIndex = CB_ERR;
					DWORD	newCodePage;
					unsigned int iOldIEView;
					HWND	hWnd = M->FindWindow(hContact);
					DWORD	sCodePage = M->GetDword(hContact, "ANSIcodepage", 0);
					DWORD	oldTZ = (DWORD)M->GetByte(hContact, "UserInfo", "Timezone", M->GetByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone", -1));
					BYTE	bInfoPanel, bOldInfoPanel = M->GetByte(hContact, "infopanel", 0);
					BYTE	bAvatarVisible = 0;

					if (hWnd) {
						dat = (struct _MessageWindowData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
						if (dat)
							iOldIEView = GetIEViewMode(hWnd, dat->hContact);
					}

					if ((iIndex = SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_GETCURSEL, 0, 0)) != CB_ERR) {
						unsigned int iNewIEView;

						switch (iIndex) {
							case 0:
								M->WriteByte(hContact, SRMSGMOD_T, "ieview", 0);
								M->WriteByte(hContact, SRMSGMOD_T, "hpplog", 0);
								break;
							case 1:
								M->WriteByte(hContact, SRMSGMOD_T, "ieview", -1);
								M->WriteByte(hContact, SRMSGMOD_T, "hpplog", 1);
								break;
							case 2:
								M->WriteByte(hContact, SRMSGMOD_T, "ieview", 1);
								M->WriteByte(hContact, SRMSGMOD_T, "hpplog", -1);
								break;
							case 3:
								M->WriteByte(hContact, SRMSGMOD_T, "ieview", -1);
								M->WriteByte(hContact, SRMSGMOD_T, "hpplog", -1);
								break;
							default:
								break;
						}
						if (hWnd && dat) {
							iNewIEView = GetIEViewMode(hWnd, dat->hContact);
							if (iNewIEView != iOldIEView) {
								if(pdwActionToTake)
									*pdwActionToTake |= UPREF_ACTION_SWITCHLOGVIEWER;
							}
						}
					}
					if ((iIndex = SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_GETCURSEL, 0, 0)) != CB_ERR) {
						M->WriteDword(hContact, SRMSGMOD_T, "sendformat", iIndex == 3 ? -1 : iIndex);
						if (iIndex == 0)
							DBDeleteContactSetting(hContact, SRMSGMOD_T, "sendformat");
					}
#if defined(_UNICODE)
					iIndex = SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETCURSEL, 0, 0);
					if ((newCodePage = (DWORD)SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETITEMDATA, (WPARAM)iIndex, 0)) != sCodePage) {
						M->WriteDword(hContact, SRMSGMOD_T, "ANSIcodepage", (DWORD)newCodePage);
						if (hWnd && dat) {
							dat->codePage = newCodePage;
							dat->wOldStatus = 0;
							SendMessage(hWnd, DM_UPDATETITLE, 0, 0);
						}
					}
					if ((IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? 1 : 0) != M->GetByte(hContact, "forceansi", 0)) {
						M->WriteByte(hContact, SRMSGMOD_T, "forceansi", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? 1 : 0));
						if (hWnd && dat)
							dat->sendMode = IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? dat->sendMode | SMODE_FORCEANSI : dat->sendMode & ~SMODE_FORCEANSI;
					}
#else
					newCodePage = CP_ACP;
#endif
					if (IsDlgButtonChecked(hwndDlg, IDC_ISFAVORITE)) {
						if (!DBGetContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", 0))
							AddContactToFavorites(hContact, NULL, NULL, NULL, 0, 0, 1, PluginConfig.g_hMenuFavorites, newCodePage);
					} else
						DeleteMenu(PluginConfig.g_hMenuFavorites, (UINT_PTR)hContact, MF_BYCOMMAND);

					DBWriteContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", (WORD)(IsDlgButtonChecked(hwndDlg, IDC_ISFAVORITE) ? 1 : 0));
					M->WriteByte(hContact, SRMSGMOD_T, "splitoverride", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_PRIVATESPLITTER) ? 1 : 0));

					M->WriteByte(hContact, TEMPLATES_MODULE, "enabled", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_TEMPLOVERRIDE)));
					M->WriteByte(hContact, RTLTEMPLATES_MODULE, "enabled", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_RTLTEMPLOVERRIDE)));

					INT_PTR offset = SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_GETCURSEL, 0, 0);
					if (offset > 0) {
						MIM_TIMEZONE *ptz = (MIM_TIMEZONE *)SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_GETITEMDATA, (WPARAM)offset, 0);
						if(reinterpret_cast<INT_PTR>(ptz) != CB_ERR && ptz != 0) {
							char	*szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
							M->WriteTString(hContact, "UserInfo", "TzName", ptz->tszName);
							M->WriteByte(hContact, "UserInfo", "Timezone", (char)(ptz->GMT_Offset));
							if(szProto)
								M->WriteByte(hContact, szProto, "Timezone", (char)(ptz->GMT_Offset));
						}
					} else {
						DBDeleteContactSetting(hContact, "UserInfo", "Timezone");
						DBDeleteContactSetting(hContact, "UserInfo", "TzName");
					}

					if (hWnd && dat) {
						LoadTimeZone(hWnd, dat);
						InvalidateRect(GetDlgItem(hWnd, IDC_PANELUIN), NULL, FALSE);
					}

					bAvatarVisible = (BYTE)SendDlgItemMessage(hwndDlg, IDC_SHOWAVATAR, CB_GETCURSEL, 0, 0);
					if(bAvatarVisible == 0)
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "hideavatar");
					else
						M->WriteByte(hContact, SRMSGMOD_T, "hideavatar", (BYTE)(bAvatarVisible == 1 ? 1 : 0));

					bInfoPanel = (BYTE)SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_GETCURSEL, 0, 0);
					if (bInfoPanel != bOldInfoPanel) {
						M->WriteByte(hContact, SRMSGMOD_T, "infopanel", (BYTE)(bInfoPanel == 0 ? 0 : (bInfoPanel == 1 ? 1 : -1)));
						if (hWnd && dat)
							SendMessage(hWnd, DM_SETINFOPANEL, 0, 0);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2))
						M->WriteDword(hContact, SRMSGMOD_T, "maxhist", (DWORD)SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_GETPOS, 0, 0));
					else
						M->WriteDword(hContact, SRMSGMOD_T, "maxhist", 0);

					//MAD
					if (IsDlgButtonChecked(hwndDlg, IDC_LOADONLYACTUAL)){
						M->WriteByte(hContact, SRMSGMOD_T, "ActualHistory", 1);
						if (hWnd && dat) dat->bActualHistory=TRUE;
						}else{
							M->WriteByte(hContact, SRMSGMOD_T, "ActualHistory", 0);
						if (hWnd && dat) dat->bActualHistory=FALSE;
						}
					//

					if (IsDlgButtonChecked(hwndDlg, IDC_IGNORETIMEOUTS)) {
						M->WriteByte(hContact, SRMSGMOD_T, "no_ack", 1);
						if (hWnd && dat)
							dat->sendMode |= SMODE_NOACK;
					} else {
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "no_ack");
						if (hWnd && dat)
							dat->sendMode &= ~SMODE_NOACK;
					}
					if (hWnd && dat) {
						SendMessage(hWnd, DM_CONFIGURETOOLBAR, 0, 1);
						dat->panelWidth = -1;
						ShowPicture(hWnd, dat, FALSE);
						SendMessage(hWnd, WM_SIZE, 0, 0);
						DM_ScrollToBottom(hWnd, dat, 0, 1);
					}

					if (IsDlgButtonChecked(hwndDlg, IDC_NOAUTOCLOSE))
						M->WriteByte(hContact, SRMSGMOD_T, "NoAutoClose", 1);
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

/*
 * loads message log and other "per contact" flags
 * it uses the global flag value (0, mwflags) and then merges per contact settings
 * based on the mask value.

 * ALWAYS mask dat->dwFlags with MWF_LOG_ALL to only affect real flag bits and
 * ignore temporary bits.
 */

int LoadLocalFlags(HWND hwnd, struct _MessageWindowData *dat)
{
	int		i = 0;
	DWORD	dwMask = M->GetDword(dat->hContact, "mwmask", 0);
	DWORD	dwLocal = M->GetDword(dat->hContact, "mwflags", 0);
	DWORD	dwGlobal = M->GetDword("mwflags", 0);
	DWORD	maskval;

	if(dat) {
		dat->dwFlags &= ~MWF_LOG_ALL;
		if(dat->dwFlags & MWF_SHOW_PRIVATETHEME)
			dat->dwFlags |= (dat->theme.dwFlags & MWF_LOG_ALL);
		else
			dat->dwFlags |= (dwGlobal & MWF_LOG_ALL);
		while(checkboxes[i].uId) {
			maskval = checkboxes[i].uFlag;
			if(dwMask & maskval)
				dat->dwFlags = (dwLocal & maskval) ? dat->dwFlags | maskval : dat->dwFlags & ~maskval;
			i++;
		}
		return(dat->dwFlags & MWF_LOG_ALL);
	}
	return 0;
}

static INT_PTR CALLBACK DlgProcUserPrefs1(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch(msg) {
		case WM_INITDIALOG: {
			DWORD	dwLocalFlags, dwLocalMask, maskval;
			int		i = 0;

			hContact = (HANDLE)lParam;
			TranslateDialogDefault(hwndDlg);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)hContact);

			dwLocalFlags = M->GetDword(hContact, "mwflags", 0);
			dwLocalMask = M->GetDword(hContact, "mwmask", 0);

			while(checkboxes[i].uId) {
				maskval = checkboxes[i].uFlag;

				if(dwLocalMask & maskval)
					CheckDlgButton(hwndDlg, checkboxes[i].uId, (dwLocalFlags & maskval) ? BST_CHECKED : BST_UNCHECKED);
				else
					CheckDlgButton(hwndDlg, checkboxes[i].uId, BST_INDETERMINATE);
				i++;
			}
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case WM_USER + 100: {
					int i = 0;
					LRESULT state;
					HWND	hwnd = M->FindWindow(hContact);
					struct	_MessageWindowData *dat = NULL;
					DWORD	*dwActionToTake = (DWORD *)lParam, dwMask = 0, dwFlags = 0, maskval;

					if(hwnd)
						dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

					while(checkboxes[i].uId) {
						maskval = checkboxes[i].uFlag;

						state = IsDlgButtonChecked(hwndDlg, checkboxes[i].uId);
						if(state != BST_INDETERMINATE) {
							dwMask |= maskval;
							dwFlags = (state == BST_CHECKED) ? (dwFlags | maskval) : (dwFlags & ~maskval);
						}
						i++;
					}
					if(dwMask) {
						M->WriteDword(hContact, SRMSGMOD_T, "mwmask", dwMask);
						M->WriteDword(hContact, SRMSGMOD_T, "mwflags", dwFlags);
					}
					else {
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "mwmask");
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "mwflags");
					}
					if(hwnd && dat) {
						if(dwMask)
							*dwActionToTake |= (DWORD)UPREF_ACTION_REMAKELOG;
						if((dat->dwFlags & MWF_LOG_RTL) != (dwFlags & MWF_LOG_RTL))
							*dwActionToTake |= (DWORD)UPREF_ACTION_APPLYOPTIONS;
					}
					break;
				}
			}
			break;
	}
	return FALSE;
}

INT_PTR CALLBACK DlgProcUserPrefsFrame(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch(msg) {
		case WM_INITDIALOG: {
			TCITEM tci;
			RECT rcClient;
			TCHAR szBuffer[180];

			hContact = (HANDLE)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)hContact);

			WindowList_Add(hUserPrefsWindowList, hwndDlg, hContact);
			TranslateDialogDefault(hwndDlg);

			GetClientRect(hwndDlg, &rcClient);

			mir_sntprintf(szBuffer, safe_sizeof(szBuffer), TranslateT("Set messaging options for %s"), (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
			SetWindowText(hwndDlg, szBuffer);

			tci.mask = TCIF_PARAM | TCIF_TEXT;
			tci.lParam = (LPARAM)CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS), hwndDlg, DlgProcUserPrefs, (LPARAM)hContact);
			tci.pszText = TranslateT("General");
			TabCtrl_InsertItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), 0, &tci);
			MoveWindow((HWND)tci.lParam, 5, 32, rcClient.right - 10, rcClient.bottom - 80, 1);
			ShowWindow((HWND)tci.lParam, SW_SHOW);
			if (M->m_pfnEnableThemeDialogTexture)
				M->m_pfnEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);


			tci.lParam = (LPARAM)CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS1), hwndDlg, DlgProcUserPrefs1, (LPARAM)hContact);
			tci.pszText = TranslateT("Message Log");
			TabCtrl_InsertItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), 1, &tci);
			MoveWindow((HWND)tci.lParam, 5, 32, rcClient.right - 10, rcClient.bottom - 80, 1);
			ShowWindow((HWND)tci.lParam, SW_HIDE);
			if (M->m_pfnEnableThemeDialogTexture)
				M->m_pfnEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);
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
					HWND	hwnd = M->FindWindow(hContact);

					tci.mask = TCIF_PARAM;

					count = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_OPTIONSTAB));
					for (i = 0;i < count;i++) {
						TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_OPTIONSTAB), i, &tci);
						SendMessage((HWND)tci.lParam, WM_COMMAND, WM_USER + 100, (LPARAM)&dwActionToTake);
					}
					if(hwnd) {
						struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
						if(dat) {
							DWORD	dwOldFlags = (dat->dwFlags & MWF_LOG_ALL);

							SetDialogToType(hwnd);
							if(dwActionToTake & UPREF_ACTION_SWITCHLOGVIEWER) {
								unsigned int mode = GetIEViewMode(hwndDlg, dat->hContact);
								SwitchMessageLog(hwnd, dat, mode);
							}
							LoadLocalFlags(hwnd, dat);
							if((dat->dwFlags & MWF_LOG_ALL) != dwOldFlags) {
								BOOL	fShouldHide = TRUE;

								if(IsIconic(dat->pContainer->hwnd))
									fShouldHide = FALSE;
								else
									ShowWindow(dat->pContainer->hwnd, SW_HIDE);
								SendMessage(hwnd, DM_OPTIONSAPPLIED, 0, 0);
								SendMessage(hwnd, DM_DEFERREDREMAKELOG, (WPARAM)hwnd, 0);
								if(fShouldHide)
									ShowWindow(dat->pContainer->hwnd, SW_SHOWNORMAL);
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
