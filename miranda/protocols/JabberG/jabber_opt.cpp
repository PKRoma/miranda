/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-04  Santithorn Bunchua
Copyright (C) 2005     George Hazan

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

*/

#include "jabber.h"
#include "jabber_list.h"
#include <commctrl.h>
#include "resource.h"

extern BOOL jabberSendKeepAlive;
extern UINT jabberCodePage;

static BOOL CALLBACK JabberOptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK JabberRegisterDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK JabberAdvOptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int JabberOptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	char str[33];

	ZeroMemory(&odp, sizeof(odp));
	odp.cbSize = sizeof(odp);
	odp.position = 0;
	odp.hInstance = hInst;
	odp.pszGroup = Translate("Network");
	odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_JABBER);
	odp.pszTitle = jabberModuleName;
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = JabberOptDlgProc;
	odp.nIDBottomSimpleControl = IDC_SIMPLE;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

	odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_JABBER2);
	_snprintf(str, sizeof(str), "%s %s", jabberModuleName, Translate("Advanced"));
	str[sizeof(str)-1] = '\0';
	odp.pszTitle = str;
	odp.pfnDlgProc = JabberAdvOptDlgProc;
	odp.flags = ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
	odp.nIDBottomSimpleControl = 0;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

	return 0;
}

static BOOL CALLBACK JabberRegisterDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ThreadData *thread, *regInfo;
	char text[128];

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		regInfo = (struct ThreadData *) lParam;
		wsprintf(text, "%s %s@%s:%d ?", Translate("Register"), regInfo->username, regInfo->server, regInfo->port);
		SetDlgItemText(hwndDlg, IDC_REG_STATUS, text);
		SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) regInfo);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			ShowWindow(GetDlgItem(hwndDlg, IDOK), SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg, IDCANCEL), SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg, IDC_PROGRESS_REG), SW_SHOW);
			ShowWindow(GetDlgItem(hwndDlg, IDCANCEL2), SW_SHOW);
			regInfo = (struct ThreadData *) GetWindowLong(hwndDlg, GWL_USERDATA);
			thread = (struct ThreadData *) malloc(sizeof(struct ThreadData));
			thread->type = JABBER_SESSION_REGISTER;
			strncpy(thread->username, regInfo->username, sizeof(thread->username));
			strncpy(thread->password, regInfo->password, sizeof(thread->password));
			strncpy(thread->server, regInfo->server, sizeof(thread->server));
			strncpy(thread->manualHost, regInfo->manualHost, sizeof(thread->manualHost));
			thread->port = regInfo->port;
			thread->useSSL = regInfo->useSSL;
			thread->reg_hwndDlg = hwndDlg;
			JabberForkThread(( JABBER_THREAD_FUNC )JabberServerThread, 0, thread);
			return TRUE;
		case IDCANCEL:
		case IDOK2:
			EndDialog(hwndDlg, 0);
			return TRUE;
/*
		case IDCANCEL2:
			//if (jabberRegConnected)
			//	JabberSend(jabberRegSocket, "</stream:stream>");
			EndDialog(hwndDlg, 0);
			return TRUE;
*/
		}
		break;
	case WM_JABBER_REGDLG_UPDATE:	// wParam=progress (0-100), lparam=status string
		if ((char *) lParam == NULL)
			SetDlgItemText(hwndDlg, IDC_REG_STATUS, Translate("No message"));
		else
			SetDlgItemText(hwndDlg, IDC_REG_STATUS, (char *) lParam);
		if (wParam >= 0)
			SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS_REG), PBM_SETPOS, wParam, 0);
		if (wParam >= 100) {
			ShowWindow(GetDlgItem(hwndDlg, IDCANCEL2), SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg, IDOK2), SW_SHOW);
		}
		else
			SetFocus(GetDlgItem(hwndDlg, IDC_PROGRESS_REG));
		return TRUE;
	}

	return FALSE;
}

static HWND msgLangListBox;
static BOOL CALLBACK JabberMsgLangAdd(LPTSTR str)
{
	int i, count, index;
	UINT cp;
	static struct { UINT cpId; char *cpName; } cpTable[] = {
		{	874,	"Thai" },
		{	932,	"Japanese" },
		{	936,	"Simplified Chinese" },
		{	949,	"Korean" },
		{	950,	"Traditional Chinese" },
		{	1250,	"Central European" },
		{	1251,	"Cyrillic" },
		{	1252,	"Latin I" },
		{	1253,	"Greek" },
		{	1254,	"Turkish" },
		{	1255,	"Hebrew" },
		{	1256,	"Arabic" },
		{	1257,	"Baltic" },
		{	1258,	"Vietnamese" },
		{	1361,	"Korean (Johab)" }
	};

	cp = atoi(str);
	count = sizeof(cpTable)/sizeof(cpTable[0]);
	for (i=0; i<count && cpTable[i].cpId!=cp; i++);
	if (i < count) {
		if ((index=SendMessage(msgLangListBox, CB_ADDSTRING, 0, (LPARAM) Translate(cpTable[i].cpName))) >= 0) {
			SendMessage(msgLangListBox, CB_SETITEMDATA, (WPARAM) index, (LPARAM) cp);
			if (jabberCodePage == cp)
				SendMessage(msgLangListBox, CB_SETCURSEL, (WPARAM) index, 0);
		}
	}
	return TRUE;
}

static LRESULT CALLBACK JabberValidateUsernameWndProc(HWND hwndEdit, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC oldProc = (WNDPROC) GetWindowLong(hwndEdit, GWL_USERDATA);

	switch (msg) {
	case WM_CHAR:
		if (strchr("\"&'/:<>@", wParam&0xff) != NULL)
			return 0;
		break;
	}
	return CallWindowProc(oldProc, hwndEdit, msg, wParam, lParam);
}

static BOOL CALLBACK JabberOptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char text[256];
	WORD port;
	WNDPROC oldProc;
	struct ThreadData regInfo;
	int index;

	switch (msg) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			BOOL enableRegister = TRUE;

			TranslateDialogDefault(hwndDlg);
			SetDlgItemText(hwndDlg, IDC_SIMPLE, jabberModuleName);
			if (!DBGetContactSetting(NULL, jabberProtoName, "LoginName", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_EDIT_USERNAME, dbv.pszVal);
				if (!dbv.pszVal[0]) enableRegister = FALSE;
				DBFreeVariant(&dbv);
			}
			if (!DBGetContactSetting(NULL, jabberProtoName, "Password", &dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal)+1, (LPARAM) dbv.pszVal);
				SetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, dbv.pszVal);
				if (!dbv.pszVal[0]) enableRegister = FALSE;
				DBFreeVariant(&dbv);
			}
			if (!DBGetContactSetting(NULL, jabberProtoName, "Resource", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_EDIT_RESOURCE, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else {
				SetDlgItemText(hwndDlg, IDC_EDIT_RESOURCE, "Miranda");
			}
			SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY_SPIN), UDM_SETRANGE, 0, (LPARAM) MAKELONG(100, 0));
			sprintf(text, "%d", DBGetContactSettingWord(NULL, jabberProtoName, "Priority", 0));
			SetDlgItemText(hwndDlg, IDC_PRIORITY, text);
			CheckDlgButton(hwndDlg, IDC_SAVEPASSWORD, DBGetContactSettingByte(NULL, jabberProtoName, "SavePassword", TRUE));
			if (!DBGetContactSetting(NULL, jabberProtoName, "LoginServer", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_EDIT_LOGIN_SERVER, dbv.pszVal);
				if (!dbv.pszVal[0]) enableRegister = FALSE;
				DBFreeVariant(&dbv);
			}
			else {
				SetDlgItemText(hwndDlg, IDC_EDIT_LOGIN_SERVER, "jabber.org");
			}
			port = (WORD) DBGetContactSettingWord(NULL, jabberProtoName, "Port", JABBER_DEFAULT_PORT);
			SetDlgItemInt(hwndDlg, IDC_PORT, port, FALSE);
			if (port <= 0) enableRegister = FALSE;

			CheckDlgButton(hwndDlg, IDC_USE_SSL, DBGetContactSettingByte(NULL, jabberProtoName, "UseSSL", FALSE));

			EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_REGISTER), enableRegister);

			if (DBGetContactSettingByte(NULL, jabberProtoName, "ManualConnect", FALSE) == TRUE) {
				CheckDlgButton(hwndDlg, IDC_MANUAL, TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_HOST), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_HOSTPORT), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), FALSE);
			}
			if (!DBGetContactSetting(NULL, jabberProtoName, "ManualHost", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_HOST, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			SetDlgItemInt(hwndDlg, IDC_HOSTPORT, DBGetContactSettingWord(NULL, jabberProtoName, "ManualPort", JABBER_DEFAULT_PORT), FALSE);

			CheckDlgButton(hwndDlg, IDC_KEEPALIVE, DBGetContactSettingByte(NULL, jabberProtoName, "KeepAlive", TRUE));
			CheckDlgButton(hwndDlg, IDC_RECONNECT, DBGetContactSettingByte(NULL, jabberProtoName, "Reconnect", FALSE));
			CheckDlgButton(hwndDlg, IDC_ROSTER_SYNC, DBGetContactSettingByte(NULL, jabberProtoName, "RosterSync", FALSE));

			if (!DBGetContactSetting(NULL, jabberProtoName, "Jud", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_JUD, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else {
				SetDlgItemText(hwndDlg, IDC_JUD, "users.jabber.org");
			}

			msgLangListBox = GetDlgItem(hwndDlg, IDC_MSGLANG);
			wsprintf(text, "== %s ==", Translate("System default"));
			SendMessage(msgLangListBox, CB_ADDSTRING, 0, (LPARAM) text);
			SendMessage(msgLangListBox, CB_SETITEMDATA, 0, CP_ACP);
			SendMessage(msgLangListBox, CB_SETCURSEL, 0, 0);
			EnumSystemCodePages(JabberMsgLangAdd, CP_INSTALLED);

			oldProc = (WNDPROC) GetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT_USERNAME), GWL_WNDPROC);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT_USERNAME), GWL_USERDATA, (LONG) oldProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT_USERNAME), GWL_WNDPROC, (LONG) JabberValidateUsernameWndProc);

			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EDIT_USERNAME:
		case IDC_EDIT_PASSWORD:
		case IDC_EDIT_RESOURCE:
		case IDC_EDIT_LOGIN_SERVER:
		case IDC_PORT:
		case IDC_MANUAL:
		case IDC_HOST:
		case IDC_HOSTPORT:
		case IDC_JUD:
		case IDC_PRIORITY:
			if (LOWORD(wParam) == IDC_MANUAL) {
				if (IsDlgButtonChecked(hwndDlg, IDC_MANUAL)) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_HOST), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_HOSTPORT), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), FALSE);
				}
				else {
					EnableWindow(GetDlgItem(hwndDlg, IDC_HOST), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_HOSTPORT), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), TRUE);
				}
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			else {
				if ((HWND)lParam==GetFocus() && HIWORD(wParam)==EN_CHANGE)
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			GetDlgItemText(hwndDlg, IDC_EDIT_USERNAME, regInfo.username, sizeof(regInfo.username));
			GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, regInfo.password, sizeof(regInfo.password));
			GetDlgItemText(hwndDlg, IDC_EDIT_LOGIN_SERVER, regInfo.server, sizeof(regInfo.server));
			if (IsDlgButtonChecked(hwndDlg, IDC_MANUAL)) {
				regInfo.port = (WORD) GetDlgItemInt(hwndDlg, IDC_HOSTPORT, NULL, FALSE);
				GetDlgItemText(hwndDlg, IDC_HOST, regInfo.manualHost, sizeof(regInfo.manualHost));
			}
			else {
				regInfo.port = (WORD) GetDlgItemInt(hwndDlg, IDC_PORT, NULL, FALSE);
				regInfo.manualHost[0] = '\0';
			}
			if (regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && (!IsDlgButtonChecked(hwndDlg, IDC_MANUAL) || regInfo.manualHost[0]))
				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_REGISTER), TRUE);
			else
				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_REGISTER), FALSE);
			break;
		case IDC_LINK_PUBLIC_SERVER:
			ShellExecute(hwndDlg, "open", "http://www.jabber.org/user/publicservers.php", "", "", SW_SHOW);
			return TRUE;
		case IDC_BUTTON_REGISTER:
			GetDlgItemText(hwndDlg, IDC_EDIT_USERNAME, regInfo.username, sizeof(regInfo.username));
			GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, regInfo.password, sizeof(regInfo.password));
			GetDlgItemText(hwndDlg, IDC_EDIT_LOGIN_SERVER, regInfo.server, sizeof(regInfo.server));
			if (IsDlgButtonChecked(hwndDlg, IDC_MANUAL)) {
				GetDlgItemText(hwndDlg, IDC_HOST, regInfo.manualHost, sizeof(regInfo.manualHost));
				regInfo.port = (WORD) GetDlgItemInt(hwndDlg, IDC_HOSTPORT, NULL, FALSE);
			}
			else {
				regInfo.manualHost[0] = '\0';
				regInfo.port = (WORD) GetDlgItemInt(hwndDlg, IDC_PORT, NULL, FALSE);
			}
			regInfo.useSSL = IsDlgButtonChecked(hwndDlg, IDC_USE_SSL);

			if (regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && (!IsDlgButtonChecked(hwndDlg, IDC_MANUAL) || regInfo.manualHost[0])) {
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_OPT_REGISTER), hwndDlg, JabberRegisterDlgProc, (LPARAM) &regInfo);
			}
			return TRUE;
		case IDC_MSGLANG:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_USE_SSL:
			if (!IsDlgButtonChecked(hwndDlg, IDC_MANUAL)) {
				if (IsDlgButtonChecked(hwndDlg, IDC_USE_SSL))
					SetDlgItemInt(hwndDlg, IDC_PORT, 5223, FALSE);
				else
					SetDlgItemInt(hwndDlg, IDC_PORT, 5222, FALSE);
			}
			// Fall through
		case IDC_SAVEPASSWORD:
		case IDC_KEEPALIVE:
		case IDC_RECONNECT:
		case IDC_ROSTER_SYNC:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		default:
			return 0;
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				BOOL reconnectRequired = FALSE;
				DBVARIANT dbv;

				GetDlgItemText(hwndDlg, IDC_EDIT_USERNAME, text, sizeof(text));
				if (DBGetContactSetting(NULL, jabberProtoName, "LoginName", &dbv) || strcmp(text, dbv.pszVal))
					reconnectRequired = TRUE;
				if (dbv.pszVal != NULL)	DBFreeVariant(&dbv);
				DBWriteContactSettingString(NULL, jabberProtoName, "LoginName", text);

				if (IsDlgButtonChecked(hwndDlg, IDC_SAVEPASSWORD)) {
					GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, text, sizeof(text));
					CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(text), (LPARAM) text);
					if (DBGetContactSetting(NULL, jabberProtoName, "Password", &dbv) || strcmp(text, dbv.pszVal))
						reconnectRequired = TRUE;
					if (dbv.pszVal != NULL)	DBFreeVariant(&dbv);
					DBWriteContactSettingString(NULL, jabberProtoName, "Password", text);
				}
				else
					DBDeleteContactSetting(NULL, jabberProtoName, "Password");

				GetDlgItemText(hwndDlg, IDC_EDIT_RESOURCE, text, sizeof(text));
				if (DBGetContactSetting(NULL, jabberProtoName, "Resource", &dbv) || strcmp(text, dbv.pszVal))
					reconnectRequired = TRUE;
				if (dbv.pszVal != NULL)	DBFreeVariant(&dbv);
				DBWriteContactSettingString(NULL, jabberProtoName, "Resource", text);

				GetDlgItemText(hwndDlg, IDC_PRIORITY, text, sizeof(text));
				port = (WORD) atoi(text);
				if (port > 100) port = 100;
				if (port < 0) port = 0;
				if (DBGetContactSettingWord(NULL, jabberProtoName, "Priority", 0) != port)
					reconnectRequired = TRUE;
				DBWriteContactSettingWord(NULL, jabberProtoName, "Priority", (WORD) port);

				DBWriteContactSettingByte(NULL, jabberProtoName, "SavePassword", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SAVEPASSWORD));

				GetDlgItemText(hwndDlg, IDC_EDIT_LOGIN_SERVER, text, sizeof(text));
				if (DBGetContactSetting(NULL, jabberProtoName, "LoginServer", &dbv) || strcmp(text, dbv.pszVal))
					reconnectRequired = TRUE;
				if (dbv.pszVal != NULL)	DBFreeVariant(&dbv);
				DBWriteContactSettingString(NULL, jabberProtoName, "LoginServer", text);

				port = (WORD) GetDlgItemInt(hwndDlg, IDC_PORT, NULL, FALSE);
				if (DBGetContactSettingWord(NULL, jabberProtoName, "Port", JABBER_DEFAULT_PORT) != port)
					reconnectRequired = TRUE;
				DBWriteContactSettingWord(NULL, jabberProtoName, "Port", port);

				DBWriteContactSettingByte(NULL, jabberProtoName, "UseSSL", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_USE_SSL));

				DBWriteContactSettingByte(NULL, jabberProtoName, "ManualConnect", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MANUAL));

				GetDlgItemText(hwndDlg, IDC_HOST, text, sizeof(text));
				if (DBGetContactSetting(NULL, jabberProtoName, "ManualHost", &dbv) || strcmp(text, dbv.pszVal))
					reconnectRequired = TRUE;
				if (dbv.pszVal != NULL)	DBFreeVariant(&dbv);
				DBWriteContactSettingString(NULL, jabberProtoName, "ManualHost", text);

				port = (WORD) GetDlgItemInt(hwndDlg, IDC_HOSTPORT, NULL, FALSE);
				if (DBGetContactSettingWord(NULL, jabberProtoName, "ManualPort", JABBER_DEFAULT_PORT) != port)
					reconnectRequired = TRUE;
				DBWriteContactSettingWord(NULL, jabberProtoName, "ManualPort", port);

				DBWriteContactSettingByte(NULL, jabberProtoName, "KeepAlive", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_KEEPALIVE));
				jabberSendKeepAlive = IsDlgButtonChecked(hwndDlg, IDC_KEEPALIVE);

				DBWriteContactSettingByte(NULL, jabberProtoName, "Reconnect", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_RECONNECT));
				DBWriteContactSettingByte(NULL, jabberProtoName, "RosterSync", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_ROSTER_SYNC));

				GetDlgItemText(hwndDlg, IDC_JUD, text, sizeof(text));
				DBWriteContactSettingString(NULL, jabberProtoName, "Jud", text);

				index = SendMessage(GetDlgItem(hwndDlg, IDC_MSGLANG), CB_GETCURSEL, 0, 0);
				if (index >= 0) {
					jabberCodePage = SendMessage(GetDlgItem(hwndDlg, IDC_MSGLANG), CB_GETITEMDATA, (WPARAM) index, 0);
					DBWriteContactSettingWord(NULL, jabberProtoName, "CodePage", (WORD) jabberCodePage);
				}

				if (reconnectRequired && jabberConnected)
					MessageBox(hwndDlg, Translate("These changes will take effect the next time you connect to the Jabber network."), Translate("Jabber Protocol Option"), MB_OK|MB_SETFOREGROUND);

				return TRUE;
			}
		}
		break;
	}

	return FALSE;
}

static BOOL CALLBACK JabberAdvOptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char text[256];
	BOOL bChecked;
	BYTE fontStyle;
	static BOOL fontChanged;
	static JABBER_GCLOG_FONT gcLogFont[JABBER_GCLOG_NUM_FONT];
	static LOGFONT lf;
	static HFONT hFont;

	switch (msg) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			BOOL bDirect, bManualDirect;
			BOOL bProxy, bManualProxy;

			TranslateDialogDefault(hwndDlg);

			// Multi-user conference options
			fontChanged = FALSE;
			hFont = NULL;
			JabberGcLogLoadFont(gcLogFont);
			if (gcLogFont[0].face[0] != '\0') {
				strncpy(lf.lfFaceName, gcLogFont[0].face, sizeof(lf.lfFaceName));
				lf.lfHeight = -MulDiv(gcLogFont[0].size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
				lf.lfCharSet = gcLogFont[0].charset;
				lf.lfWeight = (gcLogFont[0].style & JABBER_FONT_BOLD) ? FW_BOLD : FW_NORMAL;
				lf.lfItalic = (gcLogFont[0].style & JABBER_FONT_ITALIC) ? TRUE : FALSE;
				hFont = CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg, IDC_FONTSHOW, WM_SETFONT, (WPARAM) hFont, FALSE);
				SetDlgItemText(hwndDlg, IDC_FONTSHOW, lf.lfFaceName);
			}
			if (DBGetContactSettingByte(NULL, jabberProtoName, "GcLogSendOnEnter", TRUE) == TRUE)
				CheckRadioButton(hwndDlg, IDC_ENTER, IDC_CTRLENTER, IDC_ENTER);
			else
				CheckRadioButton(hwndDlg, IDC_ENTER, IDC_CTRLENTER, IDC_CTRLENTER);
			CheckDlgButton(hwndDlg, IDC_FLASH, DBGetContactSettingByte(NULL, jabberProtoName, "GcLogFlash", TRUE));
			if (DBGetContactSettingByte(NULL, jabberProtoName, "GcLogTime", TRUE) == TRUE)
				CheckDlgButton(hwndDlg, IDC_TIME, TRUE);
			else
				EnableWindow(GetDlgItem(hwndDlg, IDC_DATE), FALSE);
			CheckDlgButton(hwndDlg, IDC_DATE, DBGetContactSettingByte(NULL, jabberProtoName, "GcLogDate", FALSE));

			// File transfer options
			bDirect = DBGetContactSettingByte(NULL, jabberProtoName, "BsDirect", TRUE);
			bManualDirect = DBGetContactSettingByte(NULL, jabberProtoName, "BsDirectManual", FALSE);
			CheckDlgButton(hwndDlg, IDC_DIRECT, bDirect);
			CheckDlgButton(hwndDlg, IDC_DIRECT_MANUAL, bManualDirect);
			if (!DBGetContactSetting(NULL, jabberProtoName, "BsDirectAddr", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_DIRECT_ADDR, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if (!bDirect)
				EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECT_MANUAL), FALSE);
			if (!bDirect || !bManualDirect)
				EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECT_ADDR), FALSE);

			bProxy = DBGetContactSettingByte(NULL, jabberProtoName, "BsProxy", FALSE);
			bManualProxy = DBGetContactSettingByte(NULL, jabberProtoName, "BsProxyManual", FALSE);
			CheckDlgButton(hwndDlg, IDC_PROXY, bProxy);
			CheckDlgButton(hwndDlg, IDC_PROXY_MANUAL, bManualProxy);
			if (!DBGetContactSetting(NULL, jabberProtoName, "BsProxyServer", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_PROXY_ADDR, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if (!bProxy)
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROXY_MANUAL), FALSE);
			if (!bProxy || !bManualProxy)
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROXY_ADDR), FALSE);

			// Miscellaneous options
			CheckDlgButton(hwndDlg, IDC_SHOW_TRANSPORT, DBGetContactSettingByte(NULL, jabberProtoName, "ShowTransport", TRUE));
			CheckDlgButton(hwndDlg, IDC_AUTO_ADD, DBGetContactSettingByte(NULL, jabberProtoName, "AutoAdd", TRUE));
			CheckDlgButton(hwndDlg, IDC_MSG_ACK, DBGetContactSettingByte(NULL, jabberProtoName, "MsgAck", FALSE));

			return TRUE;
		}
	case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
			case IDC_DIRECT_ADDR:
			case IDC_PROXY_ADDR:
				if ((HWND)lParam==GetFocus() && HIWORD(wParam)==EN_CHANGE)
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			case IDC_DIRECT:
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_DIRECT);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECT_MANUAL), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECT_ADDR), (bChecked && IsDlgButtonChecked(hwndDlg, IDC_DIRECT_MANUAL)));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			case IDC_DIRECT_MANUAL:
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_DIRECT_MANUAL);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DIRECT_ADDR), bChecked);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			case IDC_PROXY:
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_PROXY);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROXY_MANUAL), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROXY_ADDR), (bChecked && IsDlgButtonChecked(hwndDlg, IDC_PROXY_MANUAL)));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			case IDC_PROXY_MANUAL:
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_PROXY_MANUAL);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROXY_ADDR), bChecked);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			case IDC_FONT:
				{
					CHOOSEFONT cf = {0};

					cf.lStructSize = sizeof(CHOOSEFONT);
					cf.hwndOwner = hwndDlg;
					cf.lpLogFont = &lf;
					cf.Flags = CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS;
					if (ChooseFont(&cf)) {
						fontChanged = TRUE;
						if (hFont) DeleteObject(hFont);
						hFont = CreateFontIndirect(&lf);
						SendDlgItemMessage(hwndDlg, IDC_FONTSHOW, WM_SETFONT, (WPARAM) hFont, FALSE);
						SetDlgItemText(hwndDlg, IDC_FONTSHOW, lf.lfFaceName);
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
				}
				break;
			case IDC_TIME:
				EnableWindow(GetDlgItem(hwndDlg, IDC_DATE), IsDlgButtonChecked(hwndDlg, IDC_TIME));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			default:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}
		}
		break;
	case WM_NOTIFY:
		{
			JABBER_LIST_ITEM *item;
			HANDLE hContact;
			int index, i;
			char key[64];

			switch (((LPNMHDR) lParam)->code) {
			case PSN_APPLY:
				// Multi-user conference options
				if (fontChanged) {
					for (i=0; i<JABBER_GCLOG_NUM_FONT; i++) {
						sprintf(key, "GcLogFont%dFace", i);
						DBWriteContactSettingString(NULL, jabberProtoName, key, lf.lfFaceName);
						sprintf(key, "GcLogFont%dSize", i);
						DBWriteContactSettingByte(NULL, jabberProtoName, key, (BYTE) MulDiv(abs(lf.lfHeight), 72, GetDeviceCaps(GetDC(NULL), LOGPIXELSY)));
						sprintf(key, "GcLogFont%dCharset", i);
						DBWriteContactSettingByte(NULL, jabberProtoName, key, lf.lfCharSet);
						fontStyle = (lf.lfWeight>=FW_BOLD) ? JABBER_FONT_BOLD : 0;
						if (lf.lfItalic == TRUE)
							fontStyle |= JABBER_FONT_ITALIC;
						sprintf(key, "GcLogFont%dStyle", i);
						DBWriteContactSettingByte(NULL, jabberProtoName, key, fontStyle);
						// color is missing here!!!
						// this is intentional so that we will use default color
					}
					if (jabberOnline) {
						WindowList_Broadcast(hWndListGcLog, WM_JABBER_SET_FONT, 0, 0);
					}
				}
				DBWriteContactSettingByte(NULL, jabberProtoName, "GcLogSendOnEnter", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_ENTER));
				DBWriteContactSettingByte(NULL, jabberProtoName, "GcLogFlash", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FLASH));
				DBWriteContactSettingByte(NULL, jabberProtoName, "GcLogTime", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TIME));
				DBWriteContactSettingByte(NULL, jabberProtoName, "GcLogDate", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DATE));

				// File transfer options
				DBWriteContactSettingByte(NULL, jabberProtoName, "BsDirect", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DIRECT));
				DBWriteContactSettingByte(NULL, jabberProtoName, "BsDirectManual", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DIRECT_MANUAL));
				GetDlgItemText(hwndDlg, IDC_DIRECT_ADDR, text, sizeof(text));
				DBWriteContactSettingString(NULL, jabberProtoName, "BsDirectAddr", text);
				DBWriteContactSettingByte(NULL, jabberProtoName, "BsProxy", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_PROXY));
				DBWriteContactSettingByte(NULL, jabberProtoName, "BsProxyManual", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_PROXY_MANUAL));
				GetDlgItemText(hwndDlg, IDC_PROXY_ADDR, text, sizeof(text));
				DBWriteContactSettingString(NULL, jabberProtoName, "BsProxyAddr", text);

				// Miscellaneous options
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_SHOW_TRANSPORT);
				DBWriteContactSettingByte(NULL, jabberProtoName, "ShowTransport", (BYTE) bChecked);
				index = 0;
				while ((index=JabberListFindNext(LIST_ROSTER, index)) >= 0) {
					if ((item=JabberListGetItemPtrFromIndex(index)) != NULL) {
						if (strchr(item->jid, '@') == NULL) {
							if ((hContact=JabberHContactFromJID(item->jid)) != NULL) {
								if (bChecked) {
									if (item->status != DBGetContactSettingWord(hContact, jabberProtoName, "Status", ID_STATUS_OFFLINE)) {
										DBWriteContactSettingWord(hContact, jabberProtoName, "Status", (WORD) item->status);
									}
								}
								else {
									if (DBGetContactSettingWord(hContact, jabberProtoName, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
										DBWriteContactSettingWord(hContact, jabberProtoName, "Status", ID_STATUS_OFFLINE);
									}
								}
							}
						}
					}
					index++;
				}
				DBWriteContactSettingByte(NULL, jabberProtoName, "AutoAdd", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTO_ADD));
				DBWriteContactSettingByte(NULL, jabberProtoName, "MsgAck", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MSG_ACK));

				return TRUE;
			}
		}
		break;
	case WM_DESTROY:
		if (hFont) DeleteObject(hFont);
		break;
	}

	return FALSE;
}
