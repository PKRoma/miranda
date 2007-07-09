////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"

static BOOL CALLBACK gg_mainoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK gg_genoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK gg_confoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK gg_advoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK gg_userutildlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

////////////////////////////////////////////////////////////////////////////////
// SetValue
#define SVS_NORMAL			0
#define SVS_GENDER			1
#define SVS_ZEROISUNSPEC	2
#define SVS_IP				3
#define SVS_COUNTRY 		4
#define SVS_MONTH			5
#define SVS_SIGNED			6
#define SVS_TIMEZONE		7
#define SVS_GGVERSION		9

static void SetValue(HWND hwndDlg,int idCtrl,HANDLE hContact,char *szModule,char *szSetting,int special,int disableIfUndef)
{
	DBVARIANT dbv;
	char str[80],*pstr;
	int unspecified=0;

	dbv.type=DBVT_DELETED;
	if(szModule==NULL) unspecified=1;
	else unspecified=DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(!unspecified) {
		switch(dbv.type) {
			case DBVT_BYTE:
					if(special==SVS_GENDER) {
							if(dbv.cVal=='M') pstr=Translate("Male");
							else if(dbv.cVal=='F') pstr=Translate("Female");
							else unspecified=1;
					}
					else if(special==SVS_MONTH) {
							if(dbv.bVal>0 && dbv.bVal<=12) {
									pstr=str;
									GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SABBREVMONTHNAME1-1+dbv.bVal,str,sizeof(str));
							}
							else unspecified=1;
					}
					else if(special==SVS_TIMEZONE) {
							if(dbv.cVal==-100) unspecified=1;
							else {
									pstr=str;
									mir_snprintf(str,sizeof(str),dbv.cVal?"GMT%+d:%02d":"GMT",-dbv.cVal/2,(dbv.cVal&1)*30);
							}
					}
					else {
							unspecified=(special==SVS_ZEROISUNSPEC && dbv.bVal==0);
							pstr=itoa(special==SVS_SIGNED?dbv.cVal:dbv.bVal,str,10);
					}
					break;
			case DBVT_WORD:
					if(special==SVS_COUNTRY) {
							pstr=(char*)CallService(MS_UTILS_GETCOUNTRYBYNUMBER,dbv.wVal,0);
							unspecified=pstr==NULL;
					}
					else {
							unspecified=(special==SVS_ZEROISUNSPEC && dbv.wVal==0);
							pstr=itoa(special==SVS_SIGNED?dbv.sVal:dbv.wVal,str,10);
					}
					break;
			case DBVT_DWORD:
					unspecified=(special==SVS_ZEROISUNSPEC && dbv.dVal==0);
					if(special==SVS_IP) {
							struct in_addr ia;
							ia.S_un.S_addr=htonl(dbv.dVal);
							pstr=inet_ntoa(ia);
							if(dbv.dVal==0) unspecified=1;
					}
					else if(special == SVS_GGVERSION)
						pstr = (char *)gg_version2string(dbv.dVal);
					else
						pstr = itoa(special==SVS_SIGNED?dbv.lVal:dbv.dVal,str,10);
					break;
			case DBVT_ASCIIZ:
					unspecified=(special==SVS_ZEROISUNSPEC && dbv.pszVal[0]=='\0');
					pstr=dbv.pszVal;
					break;
			default: pstr=str; lstrcpy(str,"???"); break;
		}
	}

	if(disableIfUndef)
	{
		EnableWindow(GetDlgItem(hwndDlg, idCtrl), !unspecified);
		if (unspecified)
			SetDlgItemText(hwndDlg, idCtrl, Translate("<not specified>"));
		else
			SetDlgItemText(hwndDlg, idCtrl, pstr);
	}
	else
	{
		EnableWindow(GetDlgItem(hwndDlg, idCtrl), TRUE);
		if (!unspecified)
			SetDlgItemText(hwndDlg, idCtrl, pstr);
	}
	DBFreeVariant(&dbv);
}

////////////////////////////////////////////////////////////////////////////////
// Options Page : Init

#ifndef ETDT_ENABLETAB
#define ETDT_DISABLE		0x00000001
#define ETDT_ENABLE 		0x00000002
#define ETDT_USETABTEXTURE	0x00000004
#define ETDT_ENABLETAB		(ETDT_ENABLE | ETDT_USETABTEXTURE)
#endif

static HWND hwndGeneral = NULL, hwndConference = NULL, hwndAdvanced = NULL;
static BOOL (WINAPI *pfnEnableThemeDialogTexture)(HANDLE, DWORD) = NULL;

int gg_options_init(WPARAM wParam, LPARAM lParam)
{
	char title[64];
	OPTIONSDIALOGPAGE odp = { 0 };
	HMODULE	hUxTheme = NULL;
	strncpy(title, GG_PROTONAME, sizeof(title));

	if(IsWinVerXPPlus())
	{
		if(hUxTheme = GetModuleHandle("uxtheme.dll"))
			pfnEnableThemeDialogTexture = (BOOL (WINAPI *)(HANDLE, DWORD))GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
	}

	odp.cbSize = sizeof(odp);
	odp.position = 1003000;
	odp.hInstance = hInstance;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_GG_MAIN);
	odp.pszGroup = Translate("Network");
	odp.pszTitle = title;
	odp.pfnDlgProc = gg_mainoptsdlgproc;
	odp.flags = ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Options Page : Proc
static void gg_optsdlgcheck(HWND hwndDlg)
{
	char text[128];
	GetDlgItemText(hwndDlg, IDC_UIN, text, sizeof(text));
	if(strlen(text))
	{
		GetDlgItemText(hwndDlg, IDC_EMAIL, text, sizeof(text));
		if(strlen(text))
			ShowWindow(GetDlgItem(hwndDlg, IDC_CHEMAIL), SW_SHOW);
		else
			ShowWindow(GetDlgItem(hwndDlg, IDC_CHEMAIL), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CHPASS), SW_SHOW);
		ShowWindow(GetDlgItem(hwndDlg, IDC_LOSTPASS), SW_SHOW);
		ShowWindow(GetDlgItem(hwndDlg, IDC_REMOVEACCOUNT), SW_SHOW);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CREATEACCOUNT), SW_HIDE);
	}
	else
	{
		ShowWindow(GetDlgItem(hwndDlg, IDC_REMOVEACCOUNT), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_LOSTPASS), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CHPASS), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CHEMAIL), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CREATEACCOUNT), SW_SHOW);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// Proc: Tabs generator function

static void gg_setoptionsdlgtotype(HWND hwnd, int iExpert)
{
	TCITEM tci;
	RECT rcClient;
	HWND hwndTab = GetDlgItem(hwnd, IDC_OPTIONSTAB);
	int iPages = 0;

	tci.mask = TCIF_PARAM | TCIF_TEXT;

	GetClientRect(hwnd, &rcClient);
	TabCtrl_DeleteAllItems(hwndTab);

	if(!hwndGeneral)
		hwndGeneral = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_OPT_GG_GENERAL), hwnd, gg_genoptsdlgproc);

	tci.lParam = (LPARAM)hwndGeneral;
	tci.pszText = Translate("General");
	TabCtrl_InsertItem(hwndTab, iPages++, &tci);
	MoveWindow((HWND)tci.lParam, 5, 26, rcClient.right - 8,rcClient.bottom - 29, 1);

	if(!hwndConference)
		hwndConference = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_OPT_GG_CONFERENCE), hwnd, gg_confoptsdlgproc);

	tci.lParam = (LPARAM)hwndConference;
	tci.pszText = Translate("Conference");
	TabCtrl_InsertItem(hwndTab, iPages++, &tci);
	MoveWindow((HWND)tci.lParam, 5, 26, rcClient.right - 8, rcClient.bottom - 29, 1);

	if(!hwndAdvanced)
		hwndAdvanced = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_OPT_GG_ADVANCED), hwnd, gg_advoptsdlgproc);

	if(iExpert)
	{
		tci.lParam = (LPARAM)hwndAdvanced;
		tci.pszText = Translate("Advanced");
		TabCtrl_InsertItem(hwndTab, iPages++, &tci);
		MoveWindow((HWND)tci.lParam, 5, 26, rcClient.right - 8, rcClient.bottom - 29, 1);
	}

	if(pfnEnableThemeDialogTexture)
	{
		if(hwndGeneral)
			pfnEnableThemeDialogTexture(hwndGeneral, ETDT_ENABLETAB);
		if(hwndConference)
			pfnEnableThemeDialogTexture(hwndConference, ETDT_ENABLETAB);
		if(hwndAdvanced)
			pfnEnableThemeDialogTexture(hwndAdvanced, ETDT_ENABLETAB);
	}

	ShowWindow(hwndAdvanced, SW_HIDE);
	ShowWindow(hwndConference, SW_HIDE);
	ShowWindow(hwndGeneral, SW_SHOW);

	TabCtrl_SetCurSel(hwndTab, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Proc: Main options dialog
static BOOL CALLBACK gg_mainoptsdlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int iInit = TRUE;

	switch(msg)
	{
		case WM_INITDIALOG:
		{
			int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
			iInit = TRUE;
			gg_setoptionsdlgtotype(hwnd, iExpert);
			iInit = FALSE;
			return FALSE;
		}
		case WM_DESTROY:
			hwndGeneral = hwndConference = hwndAdvanced = NULL;
			break;
		case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
			if(!iInit)
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom)
			{
				case 0:
					switch(((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{
							TCITEM tci;
							int i,count = TabCtrl_GetItemCount(GetDlgItem(hwnd,IDC_OPTIONSTAB));
							tci.mask = TCIF_PARAM;
							for (i=0;i<count;i++)
							{
								TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),i,&tci);
								SendMessage((HWND)tci.lParam,WM_NOTIFY,0,lParam);
							}
							break;
						}
						case PSN_EXPERTCHANGED:
						{
							int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
							gg_setoptionsdlgtotype(hwnd, iExpert);
							break;
						}
					}
					break;
				case IDC_OPTIONSTAB:
					switch (((LPNMHDR)lParam)->code)
					{
						case TCN_SELCHANGING:
						{
							TCITEM tci;
							tci.mask = TCIF_PARAM;
							TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
							ShowWindow((HWND)tci.lParam,SW_HIDE);
							break;
						}
						case TCN_SELCHANGE:
						{
							TCITEM tci;
							tci.mask = TCIF_PARAM;
							TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
							ShowWindow((HWND)tci.lParam,SW_SHOW);
							break;
						}
					}
					break;
			}
			break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Proc: General options dialog
static BOOL CALLBACK gg_genoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			DWORD num;

			TranslateDialogDefault(hwndDlg);
			if (num = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0))
			{
				SetDlgItemText(hwndDlg, IDC_UIN, ditoa(num));
				ShowWindow(GetDlgItem(hwndDlg, IDC_CREATEACCOUNT), SW_HIDE);
			}
			else
			{
				ShowWindow(GetDlgItem(hwndDlg, IDC_CHPASS), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_REMOVEACCOUNT), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_LOSTPASS), SW_HIDE);
			}
			if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_PASSWORD, &dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
				SetDlgItemText(hwndDlg, IDC_PASSWORD, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_EMAIL, &dbv)) {
				SetDlgItemText(hwndDlg, IDC_EMAIL, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else
			{
				ShowWindow(GetDlgItem(hwndDlg, IDC_LOSTPASS), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_CHPASS), SW_HIDE);
			}

			CheckDlgButton(hwndDlg, IDC_FRIENDSONLY, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_FRIENDSONLY, GG_KEYDEF_FRIENDSONLY));
			CheckDlgButton(hwndDlg, IDC_SHOWINVISIBLE, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWINVISIBLE, GG_KEYDEF_SHOWINVISIBLE));
			CheckDlgButton(hwndDlg, IDC_LEAVESTATUSMSG, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_LEAVESTATUSMSG, GG_KEYDEF_LEAVESTATUSMSG));
			if(ggGCEnabled)
				CheckDlgButton(hwndDlg, IDC_IGNORECONF, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_IGNORECONF, GG_KEYDEF_IGNORECONF));
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_IGNORECONF), FALSE);
				CheckDlgButton(hwndDlg, IDC_IGNORECONF, TRUE);
			}
			CheckDlgButton(hwndDlg, IDC_IMGRECEIVE, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_IMGRECEIVE, GG_KEYDEF_IMGRECEIVE));
			CheckDlgButton(hwndDlg, IDC_SHOWNOTONMYLIST, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWNOTONMYLIST, GG_KEYDEF_SHOWNOTONMYLIST));

			EnableWindow(GetDlgItem(hwndDlg, IDC_LEAVESTATUS), IsDlgButtonChecked(hwndDlg, IDC_LEAVESTATUSMSG));
			EnableWindow(GetDlgItem(hwndDlg, IDC_IMGMETHOD), IsDlgButtonChecked(hwndDlg, IDC_IMGRECEIVE));
			SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_ADDSTRING, 0, (LPARAM)Translate("<Last Status>"));	// 0
			SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_ADDSTRING, 0, (LPARAM)Translate("Online")); 		// ID_STATUS_ONLINE
			SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_ADDSTRING, 0, (LPARAM)Translate("Away"));			// ID_STATUS_AWAY
			SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_ADDSTRING, 0, (LPARAM)Translate("Invisible"));		// ID_STATUS_INVISIBLE
			switch(DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_LEAVESTATUS, GG_KEYDEF_LEAVESTATUS))
			{
				case ID_STATUS_ONLINE:
					SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_SETCURSEL, 1, 0);
					break;
				case ID_STATUS_AWAY:
					SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_SETCURSEL, 2, 0);
					break;
				case ID_STATUS_INVISIBLE:
					SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_SETCURSEL, 3, 0);
					break;
				default:
					SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_SETCURSEL, 0, 0);
			}

			SendDlgItemMessage(hwndDlg, IDC_IMGMETHOD, CB_ADDSTRING, 0, (LPARAM)Translate("System tray icon"));
			SendDlgItemMessage(hwndDlg, IDC_IMGMETHOD, CB_ADDSTRING, 0, (LPARAM)Translate("Popup window"));
			SendDlgItemMessage(hwndDlg, IDC_IMGMETHOD, CB_SETCURSEL,
				DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_IMGMETHOD, GG_KEYDEF_IMGMETHOD), 0);
			break;
		}
		case WM_COMMAND:
		{
			if ((LOWORD(wParam) == IDC_UIN || LOWORD(wParam) == IDC_PASSWORD || LOWORD(wParam) == IDC_EMAIL)
				&& (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
				return 0;
			switch (LOWORD(wParam)) {
				case IDC_EMAIL:
				case IDC_UIN:
				{
					gg_optsdlgcheck(hwndDlg);
					break;
				}
				case IDC_LEAVESTATUSMSG:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_LEAVESTATUS), IsDlgButtonChecked(hwndDlg, IDC_LEAVESTATUSMSG));
					break;
				}
				case IDC_IMGRECEIVE:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_IMGMETHOD), IsDlgButtonChecked(hwndDlg, IDC_IMGRECEIVE));
					break;
				}
				case IDC_LOSTPASS:
				{
					char email[128];
					uin_t uin;
					GetDlgItemText(hwndDlg, IDC_UIN, email, sizeof(email));
					uin = atoi(email);
					GetDlgItemText(hwndDlg, IDC_EMAIL, email, sizeof(email));
					if(!strlen(email))
						MessageBox(
							NULL,
							Translate("You need to specify your registration e-mail first."),
							GG_PROTONAME,
							MB_OK | MB_ICONEXCLAMATION);
					else if(MessageBox(
						NULL,
						Translate("Your password will be sent to your registration e-mail.\nDo you want to continue ?"),
						GG_PROTONAME,
						MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
							gg_remindpassword(uin, email);
					return FALSE;
				}
				case IDC_CREATEACCOUNT:
				case IDC_REMOVEACCOUNT:
					if(gg_isonline())
					{
						if(MessageBox(
							NULL,
							Translate("You should disconnect before making any permanent changes with your account.\nDo you want to disconnect now ?"),
							GG_PROTONAME,
							MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
							break;
						else
							gg_disconnect(FALSE);
					}
				case IDC_CHPASS:
				case IDC_CHEMAIL:
					{
						// Readup data
						GGUSERUTILDLGDATA dat;
						int ret;
						char pass[128], email[128];
						GetDlgItemText(hwndDlg, IDC_UIN, pass, sizeof(pass));
						dat.uin = atoi(pass);
						GetDlgItemText(hwndDlg, IDC_PASSWORD, pass, sizeof(pass));
						GetDlgItemText(hwndDlg, IDC_EMAIL, email, sizeof(email));
						dat.pass = pass;
						dat.email = email;
						if(LOWORD(wParam) == IDC_CREATEACCOUNT)
						{
							dat.mode = GG_USERUTIL_CREATE;
							ret = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CREATEACCOUNT), hwndDlg, gg_userutildlgproc, (LPARAM)&dat);
						}
						else if(LOWORD(wParam) == IDC_CHPASS)
						{
							dat.mode = GG_USERUTIL_PASS;
							ret = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CHPASS), hwndDlg, gg_userutildlgproc, (LPARAM)&dat);
						}
						else if(LOWORD(wParam) == IDC_CHEMAIL)
						{
							dat.mode = GG_USERUTIL_EMAIL;
							ret = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CHEMAIL), hwndDlg, gg_userutildlgproc, (LPARAM)&dat);
						}
						else
						{
							dat.mode = GG_USERUTIL_REMOVE;
							ret = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_REMOVEACCOUNT), hwndDlg, gg_userutildlgproc, (LPARAM)&dat);
						}

						if(ret == IDOK)
						{
							DBVARIANT dbv;
							DWORD num;

							// Show reload required window
							ShowWindow(GetDlgItem(hwndDlg, IDC_RELOADREQD), SW_SHOW);

							// Update uin
							if (num = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0))
								SetDlgItemText(hwndDlg, IDC_UIN, ditoa(num));
							else
								SetDlgItemText(hwndDlg, IDC_UIN, "");

							// Update password
							if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_PASSWORD, &dbv)) {
								CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
								SetDlgItemText(hwndDlg, IDC_PASSWORD, dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								SetDlgItemText(hwndDlg, IDC_PASSWORD, "");

							// Update e-mail
							if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_EMAIL, &dbv)) {
								SetDlgItemText(hwndDlg, IDC_EMAIL, dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								SetDlgItemText(hwndDlg, IDC_PASSWORD, "");

							// Update links
							gg_optsdlgcheck(hwndDlg);

							// Remove details
							if(LOWORD(wParam) != IDC_CHPASS && LOWORD(wParam) != IDC_CHEMAIL)
							{
								DBDeleteContactSetting(NULL, GG_PROTO, "Nick");
								DBDeleteContactSetting(NULL, GG_PROTO, "NickName");
								DBDeleteContactSetting(NULL, GG_PROTO, "City");
								DBDeleteContactSetting(NULL, GG_PROTO, "FirstName");
								DBDeleteContactSetting(NULL, GG_PROTO, "LastName");
								DBDeleteContactSetting(NULL, GG_PROTO, "FamilyName");
								DBDeleteContactSetting(NULL, GG_PROTO, "CityOrigin");
								DBDeleteContactSetting(NULL, GG_PROTO, "Age");
								DBDeleteContactSetting(NULL, GG_PROTO, "BirthYear");
								DBDeleteContactSetting(NULL, GG_PROTO, "Gender");
							}
						}
					}
					break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR) lParam)->code) {
				case PSN_APPLY:
				{
					char str[128];

					// Write Gadu-Gadu number
					GetDlgItemText(hwndDlg, IDC_UIN, str, sizeof(str));
					DBWriteContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, atoi(str));
					// Write Gadu-Gadu password
					GetDlgItemText(hwndDlg, IDC_PASSWORD, str, sizeof(str));
					CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
					DBWriteContactSettingString(NULL, GG_PROTO, GG_KEY_PASSWORD, str);
					// Write Gadu-Gadu email
					GetDlgItemText(hwndDlg, IDC_EMAIL, str, sizeof(str));
					DBWriteContactSettingString(NULL, GG_PROTO, GG_KEY_EMAIL, str);

					// Write checkboxes
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_FRIENDSONLY, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FRIENDSONLY));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWINVISIBLE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWINVISIBLE));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_LEAVESTATUSMSG, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LEAVESTATUSMSG));
					if(ggGCEnabled)
						DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_IGNORECONF, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_IGNORECONF));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_IMGRECEIVE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_IMGRECEIVE));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWNOTONMYLIST, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTONMYLIST));

					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_IMGMETHOD,
						(BYTE)SendDlgItemMessage(hwndDlg, IDC_IMGMETHOD, CB_GETCURSEL, 0, 0));

					// Write leave status
					switch(SendDlgItemMessage(hwndDlg, IDC_LEAVESTATUS, CB_GETCURSEL, 0, 0))
					{
						case 1:
							DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_LEAVESTATUS, ID_STATUS_ONLINE);
							break;
						case 2:
							DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_LEAVESTATUS, ID_STATUS_AWAY);
							break;
						case 3:
							DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_LEAVESTATUS, ID_STATUS_INVISIBLE);
							break;
						default:
							DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_LEAVESTATUS, GG_KEYDEF_LEAVESTATUS);
					}
					break;
				}
			}
			break;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Proc: General options dialog
static BOOL CALLBACK gg_confoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
		{
			DWORD num;

			TranslateDialogDefault(hwndDlg);
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_TOTAL, CB_ADDSTRING, 0, (LPARAM)Translate("Allow"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_TOTAL, CB_ADDSTRING, 0, (LPARAM)Translate("Ask"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_TOTAL, CB_ADDSTRING, 0, (LPARAM)Translate("Ignore"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_TOTAL, CB_SETCURSEL,
				DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_TOTAL, GG_KEYDEF_GC_POLICY_TOTAL), 0);

			if (num = DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_TOTAL, GG_KEYDEF_GC_COUNT_TOTAL))
				SetDlgItemText(hwndDlg, IDC_GC_COUNT_TOTAL, ditoa(num));

			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_UNKNOWN, CB_ADDSTRING, 0, (LPARAM)Translate("Allow"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_UNKNOWN, CB_ADDSTRING, 0, (LPARAM)Translate("Ask"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_UNKNOWN, CB_ADDSTRING, 0, (LPARAM)Translate("Ignore"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_UNKNOWN, CB_SETCURSEL,
				DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_UNKNOWN, GG_KEYDEF_GC_POLICY_UNKNOWN), 0);

			if (num = DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_UNKNOWN, GG_KEYDEF_GC_COUNT_UNKNOWN))
				SetDlgItemText(hwndDlg, IDC_GC_COUNT_UNKNOWN, ditoa(num));

			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_DEFAULT, CB_ADDSTRING, 0, (LPARAM)Translate("Allow"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_DEFAULT, CB_ADDSTRING, 0, (LPARAM)Translate("Ask"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_DEFAULT, CB_ADDSTRING, 0, (LPARAM)Translate("Ignore"));
			SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_DEFAULT, CB_SETCURSEL,
				DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_DEFAULT, GG_KEYDEF_GC_POLICY_DEFAULT), 0);
			break;
		}
		case WM_COMMAND:
		{
			if ((LOWORD(wParam) == IDC_GC_COUNT_TOTAL || LOWORD(wParam) == IDC_GC_COUNT_UNKNOWN)
				&& (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
				return 0;
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR) lParam)->code) {
				case PSN_APPLY:
				{
					char str[128];

					// Write groupchat policy
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_TOTAL,
						(WORD)SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_TOTAL, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_UNKNOWN,
						(WORD)SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_UNKNOWN, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_DEFAULT,
						(WORD)SendDlgItemMessage(hwndDlg, IDC_GC_POLICY_DEFAULT, CB_GETCURSEL, 0, 0));

					GetDlgItemText(hwndDlg, IDC_GC_COUNT_TOTAL, str, sizeof(str));
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_TOTAL, atoi(str));
					GetDlgItemText(hwndDlg, IDC_GC_COUNT_UNKNOWN, str, sizeof(str));
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_UNKNOWN, atoi(str));

					break;
				}
			}
			break;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Proc: Advanced options dialog
static BOOL CALLBACK gg_advoptsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			DWORD num;

			TranslateDialogDefault(hwndDlg);
			if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_SERVERHOSTS, &dbv)) {
				SetDlgItemText(hwndDlg, IDC_HOST, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else
				SetDlgItemText(hwndDlg, IDC_HOST, GG_KEYDEF_SERVERHOSTS);

			CheckDlgButton(hwndDlg, IDC_KEEPALIVE, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_KEEPALIVE, GG_KEYDEF_KEEPALIVE));
			CheckDlgButton(hwndDlg, IDC_SHOWCERRORS, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWCERRORS, GG_KEYDEF_SHOWCERRORS));
			CheckDlgButton(hwndDlg, IDC_ARECONNECT, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_ARECONNECT, GG_KEYDEF_ARECONNECT));
			CheckDlgButton(hwndDlg, IDC_MSGACK, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_MSGACK, GG_KEYDEF_MSGACK));
			CheckDlgButton(hwndDlg, IDC_MANUALHOST, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_MANUALHOST, GG_KEYDEF_MANUALHOST));
			if(hLibSSL)
				CheckDlgButton(hwndDlg, IDC_SSLCONN, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SSLCONN, GG_KEYDEF_SSLCONN));
			else
			{
				CheckDlgButton(hwndDlg, IDC_SSLCONN, FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SSLCONN), FALSE);
			}

			EnableWindow(GetDlgItem(hwndDlg, IDC_HOST), IsDlgButtonChecked(hwndDlg, IDC_MANUALHOST));
			EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), IsDlgButtonChecked(hwndDlg, IDC_MANUALHOST));

			CheckDlgButton(hwndDlg, IDC_DIRECTCONNS, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_DIRECTCONNS, GG_KEYDEF_DIRECTCONNS));
			if (num = DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_DIRECTPORT, GG_KEYDEF_DIRECTPORT))
				SetDlgItemText(hwndDlg, IDC_DIRECTPORT, ditoa(num));
			CheckDlgButton(hwndDlg, IDC_FORWARDING, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_FORWARDING, GG_KEYDEF_FORWARDING));
			if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_FORWARDHOST, &dbv)) {
				SetDlgItemText(hwndDlg, IDC_FORWARDHOST, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if (num = DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_FORWARDPORT, GG_KEYDEF_FORWARDPORT))
				SetDlgItemText(hwndDlg, IDC_FORWARDPORT, ditoa(num));

			EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECTPORT), IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
			EnableWindow(GetDlgItem(hwndDlg, IDC_FORWARDING), IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
			EnableWindow(GetDlgItem(hwndDlg, IDC_FORWARDPORT), IsDlgButtonChecked(hwndDlg, IDC_FORWARDING) && IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
			EnableWindow(GetDlgItem(hwndDlg, IDC_FORWARDHOST), IsDlgButtonChecked(hwndDlg, IDC_FORWARDING) && IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
			break;
		}
		case WM_COMMAND:
		{
			if ((LOWORD(wParam) == IDC_DIRECTPORT || LOWORD(wParam) == IDC_FORWARDHOST || LOWORD(wParam) == IDC_FORWARDPORT)
				&& (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
				return 0;
			switch (LOWORD(wParam)) {
				case IDC_MANUALHOST:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_HOST), IsDlgButtonChecked(hwndDlg, IDC_MANUALHOST));
					EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), IsDlgButtonChecked(hwndDlg, IDC_MANUALHOST));
					ShowWindow(GetDlgItem(hwndDlg, IDC_RELOADREQD), SW_SHOW);
					break;
				}
				case IDC_DIRECTCONNS:
				case IDC_FORWARDING:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECTPORT), IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
					EnableWindow(GetDlgItem(hwndDlg, IDC_FORWARDING), IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
					EnableWindow(GetDlgItem(hwndDlg, IDC_FORWARDPORT), IsDlgButtonChecked(hwndDlg, IDC_FORWARDING) && IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
					EnableWindow(GetDlgItem(hwndDlg, IDC_FORWARDHOST), IsDlgButtonChecked(hwndDlg, IDC_FORWARDING) && IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
					ShowWindow(GetDlgItem(hwndDlg, IDC_RELOADREQD), SW_SHOW);
					break;
				}
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR) lParam)->code) {
				case PSN_APPLY:
				{
					char str[512];
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_KEEPALIVE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_KEEPALIVE));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWCERRORS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWCERRORS));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_ARECONNECT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_ARECONNECT));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_MSGACK, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MSGACK));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_MANUALHOST, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MANUALHOST));
					if(hLibSSL)
						DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_SSLCONN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SSLCONN));

					// Transfer settings
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_DIRECTCONNS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DIRECTCONNS));
					DBWriteContactSettingByte(NULL, GG_PROTO, GG_KEY_FORWARDING, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FORWARDING));

					// Write custom servers
					GetDlgItemText(hwndDlg, IDC_HOST, str, sizeof(str));
					DBWriteContactSettingString(NULL, GG_PROTO, GG_KEY_SERVERHOSTS, str);

					// Write direct port
					GetDlgItemText(hwndDlg, IDC_DIRECTPORT, str, sizeof(str));
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_DIRECTPORT, atoi(str));
					// Write forwarding host
					GetDlgItemText(hwndDlg, IDC_FORWARDHOST, str, sizeof(str));
					DBWriteContactSettingString(NULL, GG_PROTO, GG_KEY_FORWARDHOST, str);
					GetDlgItemText(hwndDlg, IDC_FORWARDPORT, str, sizeof(str));
					DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_FORWARDPORT, atoi(str));
					break;
				}
			}
			break;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Info Page : Data
struct GGDETAILSDLGDATA
{
	HINSTANCE hInstIcmp;
	HANDLE hContact;
	int disableUpdate;
	int updating;
};

////////////////////////////////////////////////////////////////////////////////
// Info Page : Proc
static BOOL CALLBACK gg_detailsdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct GGDETAILSDLGDATA *dat;
	dat = (struct GGDETAILSDLGDATA *)GetWindowLong(hwndDlg, GWL_USERDATA);
	switch(msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			dat = (struct GGDETAILSDLGDATA *)malloc(sizeof(struct GGDETAILSDLGDATA));
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
			dat->hContact=(HANDLE)lParam;
			dat->disableUpdate = FALSE;
			dat->updating = FALSE;
			// Add genders
			if(!dat->hContact)
			{
				SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)Translate(""));		// 0
				SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)Translate("Male"));	// 1
				SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)Translate("Female"));	// 2
			}
			return TRUE;

		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->idFrom)
			{
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_INFOCHANGED:
						{
							char *szProto;
							HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;

							// Show updated message
							if(dat && dat->updating)
							{
								MessageBox(
									NULL,
									Translate("Your info has been uploaded to public catalog."),
									GG_PROTONAME,
									MB_OK | MB_ICONINFORMATION
								);
								dat->updating = FALSE;
								break;
							}

							if (hContact == NULL)
								szProto = GG_PROTO;
							else
								szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
							if (szProto == NULL)
									break;

							// Disable when updating
							if(dat) dat->disableUpdate = TRUE;

							SetValue(hwndDlg, IDC_UIN, hContact, szProto, GG_KEY_UIN, 0, (hContact != NULL));
							SetValue(hwndDlg, IDC_REALIP, hContact, szProto, GG_KEY_CLIENTIP, SVS_IP, (hContact != NULL));
							SetValue(hwndDlg, IDC_PORT, hContact, szProto, GG_KEY_CLIENTPORT, SVS_ZEROISUNSPEC, (hContact != NULL));
							SetValue(hwndDlg, IDC_VERSION, hContact, szProto, GG_KEY_CLIENTVERSION, SVS_GGVERSION, (hContact != NULL));

							SetValue(hwndDlg, IDC_FIRSTNAME, hContact, szProto, "FirstName", SVS_NORMAL, (hContact != NULL));
							SetValue(hwndDlg, IDC_LASTNAME, hContact, szProto, "LastName", SVS_NORMAL, (hContact != NULL));
							SetValue(hwndDlg, IDC_NICKNAME, hContact, szProto, "NickName", SVS_NORMAL, (hContact != NULL));
							SetValue(hwndDlg, IDC_BIRTHYEAR, hContact, szProto, "BirthYear", SVS_ZEROISUNSPEC, (hContact != NULL));
							SetValue(hwndDlg, IDC_CITY, hContact, szProto, "City", SVS_NORMAL, (hContact != NULL));
							SetValue(hwndDlg, IDC_FAMILYNAME, hContact, szProto, "FamilyName", SVS_NORMAL, (hContact != NULL));
							SetValue(hwndDlg, IDC_CITYORIGIN, hContact, szProto, "CityOrigin", SVS_NORMAL, (hContact != NULL));

							if(hContact)
							{
								SetValue(hwndDlg, IDC_GENDER, hContact, szProto, "Gender", SVS_GENDER, (hContact != NULL));
								SetValue(hwndDlg, IDC_STATUSDESCR, hContact, "CList", GG_KEY_STATUSDESCR, SVS_NORMAL, (hContact != NULL));
							}
							else switch((char)DBGetContactSettingByte(hContact, GG_PROTO, "Gender", (BYTE)'?'))
							{
								case 'M':
									SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_SETCURSEL, 1, 0);
									break;
								case 'F':
									SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_SETCURSEL, 2, 0);
									break;
								default:
									SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_SETCURSEL, 0, 0);
							}

							// Disable when updating
							if(dat) dat->disableUpdate = FALSE;
							break;
						}
					}
					break;
				}
				break;
		case WM_COMMAND:
			if (dat && !dat->hContact && LOWORD(wParam) == IDC_SAVE && HIWORD(wParam) == BN_CLICKED)
			{
				// Save user data
				char text[256];
				gg_pubdir50_t req;

				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE), FALSE);

				req = gg_pubdir50_new(GG_PUBDIR50_WRITE);

				GetDlgItemText(hwndDlg, IDC_FIRSTNAME, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, text);

				GetDlgItemText(hwndDlg, IDC_LASTNAME, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, text);

				GetDlgItemText(hwndDlg, IDC_NICKNAME, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, text);

				GetDlgItemText(hwndDlg, IDC_CITY, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_CITY, text);

				// Gadu-Gadu Female <-> Male
				switch(SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETCURSEL, 0, 0))
				{
					case 1:
						gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE);
						break;
					case 2:
						gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE);
						break;
					default:
						gg_pubdir50_add(req, GG_PUBDIR50_GENDER, "");
				}

				GetDlgItemText(hwndDlg, IDC_BIRTHYEAR, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_BIRTHYEAR, text);

				GetDlgItemText(hwndDlg, IDC_FAMILYNAME, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_FAMILYNAME, text);

				GetDlgItemText(hwndDlg, IDC_CITYORIGIN, text, sizeof(text));
				if (strlen(text)) gg_pubdir50_add(req, GG_PUBDIR50_FAMILYCITY, text);

				// Run update
				gg_pubdir50_seq_set(req, GG_SEQ_CHINFO);
				gg_pubdir50(ggThread->sess, req);
				dat->updating = TRUE;

				gg_pubdir50_free(req);
			}

			if(dat && !dat->hContact && !dat->disableUpdate && (HIWORD(wParam) == EN_CHANGE && (
				LOWORD(wParam) == IDC_NICKNAME || LOWORD(wParam) == IDC_FIRSTNAME || LOWORD(wParam) == IDC_LASTNAME || LOWORD(wParam) == IDC_FAMILYNAME ||
				LOWORD(wParam) == IDC_CITY || LOWORD(wParam) == IDC_CITYORIGIN || LOWORD(wParam) == IDC_BIRTHYEAR) ||
				HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_GENDER))
				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE), TRUE);

			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
					break;
			}
			break;
		case WM_DESTROY:
			if(dat)
			{
				if(dat->hInstIcmp!=NULL) FreeLibrary(dat->hInstIcmp);
				free(dat);
			}
			break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Info Page : Init
int gg_details_init(WPARAM wParam, LPARAM lParam)
{
	char* szProto;
	szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
	if ((szProto == NULL || strcmp(szProto, GG_PROTO)) && lParam || lParam && DBGetContactSettingByte((HANDLE)lParam, GG_PROTO, "ChatRoom", 0))
			return 0;

	// Here goes init
	{
		OPTIONSDIALOGPAGE odp;

		odp.cbSize = sizeof(odp);
		odp.hIcon = NULL;
		odp.hInstance = hInstance;
		odp.pfnDlgProc = gg_detailsdlgproc;
		odp.position = -1900000000;
		odp.pszTemplate = ((HANDLE)lParam != NULL) ? MAKEINTRESOURCE(IDD_INFO_GG) : MAKEINTRESOURCE(IDD_CHINFO_GG);
		odp.pszTitle = GG_PROTONAME;
		CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
	}

	// Start search for my data
	if((HANDLE)lParam == NULL)
	{
		CCSDATA ccs; ZeroMemory(&ccs, sizeof(ccs));
		gg_getinfo((WPARAM) 0, (LPARAM) &ccs);
	}

	return 0;
}
