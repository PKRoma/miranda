// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2010 Joe Kucera
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

#include <win2k.h>

extern BOOL bPopUpService;

static const char* szLogLevelDescr[] = {
	LPGEN("Display all problems"),
	LPGEN("Display problems causing possible loss of data"),
	LPGEN("Display explanations for disconnection"),
	LPGEN("Display problems requiring user intervention"),
	LPGEN("Do not display any problems (not recommended)")
};

static BOOL (WINAPI *pfnEnableThemeDialogTexture)(HANDLE, DWORD) = 0;

static void LoadDBCheckState(CIcqProto* ppro, HWND hwndDlg, int idCtrl, const char* szSetting, BYTE bDef)
{
	CheckDlgButton(hwndDlg, idCtrl, ppro->getSettingByte(NULL, szSetting, bDef));
}

static void StoreDBCheckState(CIcqProto* ppro, HWND hwndDlg, int idCtrl, const char* szSetting)
{
	ppro->setSettingByte(NULL, szSetting, (BYTE)IsDlgButtonChecked(hwndDlg, idCtrl));
}

static void OptDlgChanged(HWND hwndDlg)
{
	SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// standalone option pages

static INT_PTR CALLBACK DlgProcIcqOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CIcqProto* ppro = (CIcqProto*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		ppro = (CIcqProto*)lParam;
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );
		{
			DWORD dwUin = ppro->getContactUin(NULL);
			if (dwUin)
				SetDlgItemInt(hwndDlg, IDC_ICQNUM, dwUin, FALSE);
			else // keep it empty when no UIN entered
				SetDlgItemTextA(hwndDlg, IDC_ICQNUM, "");

			SendDlgItemMessage(hwndDlg, IDC_PASSWORD, EM_LIMITTEXT, PASSWORDMAXLEN - 1, 0);

			char pszPwd[PASSWORDMAXLEN];
			if (ppro->GetUserStoredPassword(pszPwd, sizeof(pszPwd)))
			{
				//bit of a security hole here, since it's easy to extract a password from an edit box
				SetDlgItemTextA(hwndDlg, IDC_PASSWORD, pszPwd);
			}

			LoadDBCheckState(ppro, hwndDlg, IDC_SSL, "SecureConnection", DEFAULT_SECURE_CONNECTION);
			LoadDBCheckState(ppro, hwndDlg, IDC_MD5LOGIN, "SecureLogin", DEFAULT_SECURE_LOGIN);

			char szServer[MAX_PATH];
			if (!ppro->getSettingStringStatic(NULL, "OscarServer", szServer, MAX_PATH))
				SetDlgItemTextA(hwndDlg, IDC_ICQSERVER, szServer);
			else
				SetDlgItemTextA(hwndDlg, IDC_ICQSERVER, IsDlgButtonChecked(hwndDlg, IDC_SSL) ? DEFAULT_SERVER_HOST_SSL : DEFAULT_SERVER_HOST);

			SetDlgItemInt(hwndDlg, IDC_ICQPORT, ppro->getSettingWord(NULL, "OscarPort", IsDlgButtonChecked(hwndDlg, IDC_SSL) ? DEFAULT_SERVER_PORT_SSL : DEFAULT_SERVER_PORT), FALSE);
			LoadDBCheckState(ppro, hwndDlg, IDC_KEEPALIVE, "KeepAlive", DEFAULT_KEEPALIVE_ENABLED);
			SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_SETRANGE, FALSE, MAKELONG(0, 4));
			SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_SETPOS, TRUE, 4-ppro->getSettingByte(NULL, "ShowLogLevel", LOG_WARNING));
			{
				char buf[MAX_PATH];
				SetDlgItemTextUtf(hwndDlg, IDC_LEVELDESCR, ICQTranslateUtfStatic(szLogLevelDescr[4-SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_GETPOS, 0, 0)], buf, MAX_PATH));
			}
			ShowDlgItem(hwndDlg, IDC_RECONNECTREQD, SW_HIDE);
			LoadDBCheckState(ppro, hwndDlg, IDC_NOERRMULTI, "IgnoreMultiErrorBox", 0);
		}
		return TRUE;

	case WM_HSCROLL:
		{
			char str[MAX_PATH];

			SetDlgItemTextUtf(hwndDlg, IDC_LEVELDESCR, ICQTranslateUtfStatic(szLogLevelDescr[4-SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL,TBM_GETPOS, 0, 0)], str, MAX_PATH));
			OptDlgChanged(hwndDlg);
		}
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
			case IDC_LOOKUPLINK:
				CallService(MS_UTILS_OPENURL, 1, (LPARAM)URL_FORGOT_PASSWORD);
				return TRUE;

			case IDC_NEWUINLINK:
				CallService(MS_UTILS_OPENURL, 1, (LPARAM)URL_REGISTER);
				return TRUE;

			case IDC_RESETSERVER:
				SetDlgItemInt(hwndDlg, IDC_ICQPORT, IsDlgButtonChecked(hwndDlg, IDC_SSL) ? DEFAULT_SERVER_PORT_SSL : DEFAULT_SERVER_PORT, FALSE);

			case IDC_SSL:
				SetDlgItemTextA(hwndDlg, IDC_ICQSERVER, IsDlgButtonChecked(hwndDlg, IDC_SSL) ? DEFAULT_SERVER_HOST_SSL : DEFAULT_SERVER_HOST);
				SetDlgItemInt(hwndDlg, IDC_ICQPORT, IsDlgButtonChecked(hwndDlg, IDC_SSL) ? DEFAULT_SERVER_PORT_SSL : DEFAULT_SERVER_PORT, FALSE);
				OptDlgChanged(hwndDlg);
				return TRUE;
			}

			if (ppro->icqOnline() && LOWORD(wParam) != IDC_NOERRMULTI)
			{
				char szClass[80];
				GetClassNameA((HWND)lParam, szClass, sizeof(szClass));

				if (stricmpnull(szClass, "EDIT") || HIWORD(wParam) == EN_CHANGE)
					ShowDlgItem(hwndDlg, IDC_RECONNECTREQD, SW_SHOW);
			}

			if ((LOWORD(wParam)==IDC_ICQNUM || LOWORD(wParam)==IDC_PASSWORD || LOWORD(wParam)==IDC_ICQSERVER || LOWORD(wParam)==IDC_ICQPORT) &&
				(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
			{
				return 0;
			}

			OptDlgChanged(hwndDlg);
			break;
		}

	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{

			case PSN_APPLY:
				{
					char str[128];

					ppro->setSettingDword(NULL, UNIQUEIDSETTING, GetDlgItemInt(hwndDlg, IDC_ICQNUM, NULL, FALSE));
					GetDlgItemTextA(hwndDlg, IDC_PASSWORD, str, sizeof(ppro->m_szPassword));
					if (strlennull(str))
					{
						strcpy(ppro->m_szPassword, str);
						ppro->m_bRememberPwd = TRUE;
					}
					else
						ppro->m_bRememberPwd = ppro->getSettingByte(NULL, "RememberPass", 0);

					CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(ppro->m_szPassword), (LPARAM)str);
					ppro->setSettingString(NULL, "Password", str);
					GetDlgItemTextA(hwndDlg,IDC_ICQSERVER, str, sizeof(str));
					ppro->setSettingString(NULL, "OscarServer", str);
					ppro->setSettingWord(NULL, "OscarPort", (WORD)GetDlgItemInt(hwndDlg, IDC_ICQPORT, NULL, FALSE));
					StoreDBCheckState(ppro, hwndDlg, IDC_KEEPALIVE, "KeepAlive");
					StoreDBCheckState(ppro, hwndDlg, IDC_SSL, "SecureConnection");
					StoreDBCheckState(ppro, hwndDlg, IDC_MD5LOGIN, "SecureLogin");
					StoreDBCheckState(ppro, hwndDlg, IDC_NOERRMULTI, "IgnoreMultiErrorBox");
					ppro->setSettingByte(NULL, "ShowLogLevel", (BYTE)(4-SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_GETPOS, 0, 0)));

					return TRUE;
				}
			}
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static const UINT icqPrivacyControls[] = {
	IDC_DCALLOW_ANY, IDC_DCALLOW_CLIST, IDC_DCALLOW_AUTH, IDC_ADD_ANY, IDC_ADD_AUTH, 
	IDC_WEBAWARE, IDC_PUBLISHPRIMARY, IDC_STATIC_DC1, IDC_STATIC_DC2, IDC_STATIC_CLIST
};

static INT_PTR CALLBACK DlgProcIcqPrivacyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CIcqProto* ppro = (CIcqProto*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		ppro = (CIcqProto*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		{
			int nDcType = ppro->getSettingByte(NULL, "DCType", 0);
			int nAddAuth = ppro->getSettingByte(NULL, "Auth", 1);

			if (!ppro->icqOnline())
			{
				icq_EnableMultipleControls(hwndDlg, icqPrivacyControls, SIZEOF(icqPrivacyControls), FALSE);
				ShowDlgItem(hwndDlg, IDC_STATIC_NOTONLINE, SW_SHOW);
			}
			else 
				ShowDlgItem(hwndDlg, IDC_STATIC_NOTONLINE, SW_HIDE);

			CheckDlgButton(hwndDlg, IDC_DCALLOW_ANY, (nDcType == 0));
			CheckDlgButton(hwndDlg, IDC_DCALLOW_CLIST, (nDcType == 1));
			CheckDlgButton(hwndDlg, IDC_DCALLOW_AUTH, (nDcType == 2));
			CheckDlgButton(hwndDlg, IDC_ADD_ANY, (nAddAuth == 0));
			CheckDlgButton(hwndDlg, IDC_ADD_AUTH, (nAddAuth == 1));
			LoadDBCheckState(ppro, hwndDlg, IDC_WEBAWARE, "WebAware", 0);
			LoadDBCheckState(ppro, hwndDlg, IDC_PUBLISHPRIMARY, "PublishPrimaryEmail", 0);
			LoadDBCheckState(ppro, hwndDlg, IDC_STATUSMSG_CLIST, "StatusMsgReplyCList", 0);
			LoadDBCheckState(ppro, hwndDlg, IDC_STATUSMSG_VISIBLE, "StatusMsgReplyVisible", 0);
			if (!ppro->getSettingByte(NULL, "StatusMsgReplyCList", 0))
				EnableDlgItem(hwndDlg, IDC_STATUSMSG_VISIBLE, FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DCALLOW_ANY:
		case IDC_DCALLOW_CLIST:
		case IDC_DCALLOW_AUTH:
		case IDC_ADD_ANY:
		case IDC_ADD_AUTH:
		case IDC_WEBAWARE:
		case IDC_PUBLISHPRIMARY:
		case IDC_STATUSMSG_VISIBLE:
			if ((HWND)lParam != GetFocus())  return 0;
			break;
		case IDC_STATUSMSG_CLIST:
			if (IsDlgButtonChecked(hwndDlg, IDC_STATUSMSG_CLIST)) 
			{
				EnableDlgItem(hwndDlg, IDC_STATUSMSG_VISIBLE, TRUE);
				LoadDBCheckState(ppro, hwndDlg, IDC_STATUSMSG_VISIBLE, "StatusMsgReplyVisible", 0);
			}
			else 
			{
				EnableDlgItem(hwndDlg, IDC_STATUSMSG_VISIBLE, FALSE);
				CheckDlgButton(hwndDlg, IDC_STATUSMSG_VISIBLE, FALSE);
			}
			break;
		default:
			return 0;
		}
		OptDlgChanged(hwndDlg);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			StoreDBCheckState(ppro, hwndDlg, IDC_WEBAWARE, "WebAware");
			StoreDBCheckState(ppro, hwndDlg, IDC_PUBLISHPRIMARY, "PublishPrimaryEmail");
			StoreDBCheckState(ppro, hwndDlg, IDC_STATUSMSG_CLIST, "StatusMsgReplyCList");
			StoreDBCheckState(ppro, hwndDlg, IDC_STATUSMSG_VISIBLE, "StatusMsgReplyVisible");
			if (IsDlgButtonChecked(hwndDlg, IDC_DCALLOW_AUTH))
				ppro->setSettingByte(NULL, "DCType", 2);
			else if (IsDlgButtonChecked(hwndDlg, IDC_DCALLOW_CLIST))
				ppro->setSettingByte(NULL, "DCType", 1);
			else 
				ppro->setSettingByte(NULL, "DCType", 0);
			StoreDBCheckState(ppro, hwndDlg, IDC_ADD_AUTH, "Auth");

			if (ppro->icqOnline())
			{
				PBYTE buf=NULL;
				int buflen=0;

				ppackTLVWord(&buf, &buflen, 0x19A, !ppro->getSettingByte(NULL, "Auth", 1));
				ppackTLVByte(&buf, &buflen, 0x212, ppro->getSettingByte(NULL, "WebAware", 0));
				ppackTLVWord(&buf, &buflen, 0x1F9, ppro->getSettingByte(NULL, "PrivacyLevel", 1));

				ppro->icq_changeUserDirectoryInfoServ(buf, (WORD)buflen, DIRECTORYREQUEST_UPDATEPRIVACY);

				SAFE_FREE((void**)&buf);

				// Send a status packet to notify the server about the webaware setting
				{
					WORD wStatus = MirandaStatusToIcq(ppro->m_iStatus);

					if (ppro->m_iStatus == ID_STATUS_INVISIBLE)
					{
						if (ppro->m_bSsiEnabled)
							ppro->updateServVisibilityCode(3);
						ppro->icq_setstatus(wStatus, NULL);
					}
					else
					{
						ppro->icq_setstatus(wStatus, NULL);
						if (ppro->m_bSsiEnabled)
							ppro->updateServVisibilityCode(4);
					}
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;  
}

/////////////////////////////////////////////////////////////////////////////////////////

static HWND hCpCombo;

struct CPTABLE {
	WORD cpId;
	char *cpName;
};

struct CPTABLE cpTable[] = {
	{  874,  LPGEN("Thai") },
	{  932,  LPGEN("Japanese") },
	{  936,  LPGEN("Simplified Chinese") },
	{  949,  LPGEN("Korean") },
	{  950,  LPGEN("Traditional Chinese") },
	{  1250, LPGEN("Central European") },
	{  1251, LPGEN("Cyrillic") },
	{  1252, LPGEN("Latin I") },
	{  1253, LPGEN("Greek") },
	{  1254, LPGEN("Turkish") },
	{  1255, LPGEN("Hebrew") },
	{  1256, LPGEN("Arabic") },
	{  1257, LPGEN("Baltic") },
	{  1258, LPGEN("Vietnamese") },
	{  1361, LPGEN("Korean (Johab)") },
	{   -1,  NULL}
};

static BOOL CALLBACK FillCpCombo(LPSTR str)
{
	int i;
	UINT cp;

	cp = atoi(str);
	for (i=0; cpTable[i].cpName != NULL && cpTable[i].cpId!=cp; i++);
	if (cpTable[i].cpName) 
		ComboBoxAddStringUtf(hCpCombo, cpTable[i].cpName, cpTable[i].cpId);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static const UINT icqUnicodeControls[] = {IDC_UTFALL,IDC_UTFSTATIC,IDC_UTFCODEPAGE};
static const UINT icqDCMsgControls[] = {IDC_DCPASSIVE};
static const UINT icqXStatusControls[] = {IDC_XSTATUSAUTO};
static const UINT icqCustomStatusControls[] = {IDC_XSTATUSRESET};
static const UINT icqAimControls[] = {IDC_AIMENABLE};

static INT_PTR CALLBACK DlgProcIcqFeaturesOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CIcqProto* ppro = (CIcqProto*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		ppro = (CIcqProto*)lParam;
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );
		{
			BYTE byData = ppro->getSettingByte(NULL, "UtfEnabled", DEFAULT_UTF_ENABLED);
			CheckDlgButton(hwndDlg, IDC_UTFENABLE, byData?TRUE:FALSE);
			CheckDlgButton(hwndDlg, IDC_UTFALL, byData==2?TRUE:FALSE);
			icq_EnableMultipleControls(hwndDlg, icqUnicodeControls, SIZEOF(icqUnicodeControls), byData?TRUE:FALSE);
			LoadDBCheckState(ppro, hwndDlg, IDC_TEMPVISIBLE, "TempVisListEnabled",DEFAULT_TEMPVIS_ENABLED);
			LoadDBCheckState(ppro, hwndDlg, IDC_SLOWSEND, "SlowSend", DEFAULT_SLOWSEND);
			LoadDBCheckState(ppro, hwndDlg, IDC_ONLYSERVERACKS, "OnlyServerAcks", DEFAULT_ONLYSERVERACKS);
			byData = ppro->getSettingByte(NULL, "DirectMessaging", DEFAULT_DCMSG_ENABLED);
			CheckDlgButton(hwndDlg, IDC_DCENABLE, byData?TRUE:FALSE);
			CheckDlgButton(hwndDlg, IDC_DCPASSIVE, byData==1?TRUE:FALSE);
			icq_EnableMultipleControls(hwndDlg, icqDCMsgControls, SIZEOF(icqDCMsgControls), byData?TRUE:FALSE);
			BYTE byXStatusEnabled = ppro->getSettingByte(NULL, "XStatusEnabled", DEFAULT_XSTATUS_ENABLED);
			CheckDlgButton(hwndDlg, IDC_XSTATUSENABLE, byXStatusEnabled);
      BYTE byMoodsEnabled = ppro->getSettingByte(NULL, "MoodsEnabled", DEFAULT_MOODS_ENABLED);
			CheckDlgButton(hwndDlg, IDC_MOODSENABLE, byMoodsEnabled);
			icq_EnableMultipleControls(hwndDlg, icqXStatusControls, SIZEOF(icqXStatusControls), byXStatusEnabled);
      icq_EnableMultipleControls(hwndDlg, icqCustomStatusControls, SIZEOF(icqCustomStatusControls), byXStatusEnabled || byMoodsEnabled);
			LoadDBCheckState(ppro, hwndDlg, IDC_XSTATUSAUTO, "XStatusAuto", DEFAULT_XSTATUS_AUTO);
			LoadDBCheckState(ppro, hwndDlg, IDC_XSTATUSRESET, "XStatusReset", DEFAULT_XSTATUS_RESET);
			LoadDBCheckState(ppro, hwndDlg, IDC_KILLSPAMBOTS, "KillSpambots", DEFAULT_KILLSPAM_ENABLED);
			LoadDBCheckState(ppro, hwndDlg, IDC_AIMENABLE, "AimEnabled", DEFAULT_AIM_ENABLED);
			icq_EnableMultipleControls(hwndDlg, icqAimControls, SIZEOF(icqAimControls), ppro->icqOnline()?FALSE:TRUE);

			hCpCombo = GetDlgItem(hwndDlg, IDC_UTFCODEPAGE);
			int sCodePage = ppro->getSettingWord(NULL, "AnsiCodePage", CP_ACP);
			ComboBoxAddStringUtf(GetDlgItem(hwndDlg, IDC_UTFCODEPAGE), LPGEN("System default codepage"), 0);
			EnumSystemCodePagesA(FillCpCombo, CP_INSTALLED);
			if(sCodePage == 0)
				SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_SETCURSEL, (WPARAM)0, 0);
			else 
			{
				for (int i = 0; i < SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETCOUNT, 0, 0); i++) 
				{
					if (SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETITEMDATA, (WPARAM)i, 0) == sCodePage)
					{
						SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_SETCURSEL, (WPARAM)i, 0);
						break;
					}
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_UTFENABLE:
			icq_EnableMultipleControls(hwndDlg, icqUnicodeControls, SIZEOF(icqUnicodeControls), IsDlgButtonChecked(hwndDlg, IDC_UTFENABLE));
			OptDlgChanged(hwndDlg);
			break;
		case IDC_UTFCODEPAGE:
			if(HIWORD(wParam)==CBN_SELCHANGE)
				OptDlgChanged(hwndDlg);
			break;
		case IDC_DCENABLE:
			icq_EnableMultipleControls(hwndDlg, icqDCMsgControls, SIZEOF(icqDCMsgControls), IsDlgButtonChecked(hwndDlg, IDC_DCENABLE));
			OptDlgChanged(hwndDlg);
			break;
		case IDC_XSTATUSENABLE:
			icq_EnableMultipleControls(hwndDlg, icqXStatusControls, SIZEOF(icqXStatusControls), IsDlgButtonChecked(hwndDlg, IDC_XSTATUSENABLE));
    case IDC_MOODSENABLE:
      icq_EnableMultipleControls(hwndDlg, icqCustomStatusControls, SIZEOF(icqCustomStatusControls), IsDlgButtonChecked(hwndDlg, IDC_XSTATUSENABLE) || IsDlgButtonChecked(hwndDlg, IDC_MOODSENABLE));
		default:
			OptDlgChanged(hwndDlg);
			break;
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			if (IsDlgButtonChecked(hwndDlg, IDC_UTFENABLE))
				ppro->m_bUtfEnabled = IsDlgButtonChecked(hwndDlg, IDC_UTFALL)?2:1;
			else
				ppro->m_bUtfEnabled = 0;
			{
				int i = SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETCURSEL, 0, 0);
				ppro->m_wAnsiCodepage = (WORD)SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETITEMDATA, (WPARAM)i, 0);
				ppro->setSettingWord(NULL, "AnsiCodePage", ppro->m_wAnsiCodepage);
			}
			ppro->setSettingByte(NULL, "UtfEnabled", ppro->m_bUtfEnabled);
			ppro->m_bTempVisListEnabled = (BYTE)IsDlgButtonChecked(hwndDlg, IDC_TEMPVISIBLE);
			ppro->setSettingByte(NULL, "TempVisListEnabled", ppro->m_bTempVisListEnabled);
			StoreDBCheckState(ppro, hwndDlg, IDC_SLOWSEND, "SlowSend");
			StoreDBCheckState(ppro, hwndDlg, IDC_ONLYSERVERACKS, "OnlyServerAcks");
			if (IsDlgButtonChecked(hwndDlg, IDC_DCENABLE))
				ppro->m_bDCMsgEnabled = IsDlgButtonChecked(hwndDlg, IDC_DCPASSIVE)?1:2;
			else
				ppro->m_bDCMsgEnabled = 0;
			ppro->setSettingByte(NULL, "DirectMessaging", ppro->m_bDCMsgEnabled);
			ppro->m_bXStatusEnabled = (BYTE)IsDlgButtonChecked(hwndDlg, IDC_XSTATUSENABLE);
			ppro->setSettingByte(NULL, "XStatusEnabled", ppro->m_bXStatusEnabled);
      ppro->m_bMoodsEnabled = (BYTE)IsDlgButtonChecked(hwndDlg, IDC_MOODSENABLE);
      ppro->setSettingByte(NULL, "MoodsEnabled", ppro->m_bMoodsEnabled);
			StoreDBCheckState(ppro, hwndDlg, IDC_XSTATUSAUTO, "XStatusAuto");
			StoreDBCheckState(ppro, hwndDlg, IDC_XSTATUSRESET, "XStatusReset");
			StoreDBCheckState(ppro, hwndDlg, IDC_KILLSPAMBOTS , "KillSpambots");
			StoreDBCheckState(ppro, hwndDlg, IDC_AIMENABLE, "AimEnabled");
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static const UINT icqContactsControls[] = {IDC_ADDSERVER,IDC_LOADFROMSERVER,IDC_SAVETOSERVER,IDC_UPLOADNOW};
static const UINT icqAvatarControls[] = {IDC_AUTOLOADAVATARS,IDC_STRICTAVATARCHECK};

static INT_PTR CALLBACK DlgProcIcqContactsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CIcqProto* ppro = (CIcqProto*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		ppro = (CIcqProto*)lParam;
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

		LoadDBCheckState(ppro, hwndDlg, IDC_ENABLE, "UseServerCList", DEFAULT_SS_ENABLED);
		LoadDBCheckState(ppro, hwndDlg, IDC_ADDSERVER, "ServerAddRemove", DEFAULT_SS_ADDSERVER);
		LoadDBCheckState(ppro, hwndDlg, IDC_LOADFROMSERVER, "LoadServerDetails", DEFAULT_SS_LOAD);
		LoadDBCheckState(ppro, hwndDlg, IDC_SAVETOSERVER, "StoreServerDetails", DEFAULT_SS_STORE);
		LoadDBCheckState(ppro, hwndDlg, IDC_ENABLEAVATARS, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED);
		LoadDBCheckState(ppro, hwndDlg, IDC_AUTOLOADAVATARS, "AvatarsAutoLoad", DEFAULT_LOAD_AVATARS);
		LoadDBCheckState(ppro, hwndDlg, IDC_STRICTAVATARCHECK, "StrictAvatarCheck", DEFAULT_AVATARS_CHECK);

		icq_EnableMultipleControls(hwndDlg, icqContactsControls, SIZEOF(icqContactsControls), 
			ppro->getSettingByte(NULL, "UseServerCList", DEFAULT_SS_ENABLED)?TRUE:FALSE);
		icq_EnableMultipleControls(hwndDlg, icqAvatarControls, SIZEOF(icqAvatarControls), 
			ppro->getSettingByte(NULL, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED)?TRUE:FALSE);

		if (ppro->icqOnline())
		{
			ShowDlgItem(hwndDlg, IDC_OFFLINETOENABLE, SW_SHOW);
			EnableDlgItem(hwndDlg, IDC_ENABLE, FALSE);
			EnableDlgItem(hwndDlg, IDC_ENABLEAVATARS, FALSE);
		}
		else
			EnableDlgItem(hwndDlg, IDC_UPLOADNOW, FALSE);

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_UPLOADNOW:
			ppro->ShowUploadContactsDialog();
			return TRUE;
		case IDC_ENABLE:
			icq_EnableMultipleControls(hwndDlg, icqContactsControls, SIZEOF(icqContactsControls), IsDlgButtonChecked(hwndDlg, IDC_ENABLE));
			if (ppro->icqOnline()) 
				ShowDlgItem(hwndDlg, IDC_RECONNECTREQD, SW_SHOW);
			else 
				EnableDlgItem(hwndDlg, IDC_UPLOADNOW, FALSE);
			break;
		case IDC_ENABLEAVATARS:
			icq_EnableMultipleControls(hwndDlg, icqAvatarControls, SIZEOF(icqAvatarControls), IsDlgButtonChecked(hwndDlg, IDC_ENABLEAVATARS));
			break;
		}
		OptDlgChanged(hwndDlg);
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY )
		{
			StoreDBCheckState(ppro, hwndDlg, IDC_ENABLE, "UseServerCList");
			StoreDBCheckState(ppro, hwndDlg, IDC_ADDSERVER, "ServerAddRemove");
			StoreDBCheckState(ppro, hwndDlg, IDC_LOADFROMSERVER, "LoadServerDetails");
			StoreDBCheckState(ppro, hwndDlg, IDC_SAVETOSERVER, "StoreServerDetails");
			StoreDBCheckState(ppro, hwndDlg, IDC_ENABLEAVATARS, "AvatarsEnabled");
			StoreDBCheckState(ppro, hwndDlg, IDC_AUTOLOADAVATARS, "AvatarsAutoLoad");
			StoreDBCheckState(ppro, hwndDlg, IDC_STRICTAVATARCHECK, "StrictAvatarCheck");
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DlgProcIcqPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int CIcqProto::OnOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	HMODULE hUxTheme = 0;

	if (IsWinVerXPPlus())
	{
		hUxTheme = GetModuleHandleA("uxtheme.dll");
		if (hUxTheme) 
			pfnEnableThemeDialogTexture = (BOOL (WINAPI *)(HANDLE, DWORD))GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
	}

	odp.cbSize = sizeof(odp);
	odp.position = -800000000;
	odp.hInstance = hInst;
	odp.ptszGroup = LPGENT("Network");
	odp.dwInitParam = LPARAM(this);
	odp.ptszTitle = m_tszUserName;
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR | ODPF_DONTTRANSLATE;

	odp.ptszTab = LPGENT("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_ICQ);
	odp.pfnDlgProc = DlgProcIcqOpts;
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.ptszTab = LPGENT("Contacts");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_ICQCONTACTS);
	odp.pfnDlgProc = DlgProcIcqContactsOpts;
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.ptszTab = LPGENT("Features");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_ICQFEATURES);
	odp.pfnDlgProc = DlgProcIcqFeaturesOpts;
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.ptszTab = LPGENT("Privacy");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_ICQPRIVACY);
	odp.pfnDlgProc = DlgProcIcqPrivacyOpts;
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	if (bPopUpService)
	{
		odp.position = 100000000;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_POPUPS);
		odp.groupPosition = 900000000;
		odp.pfnDlgProc = DlgProcIcqPopupOpts;
		odp.ptszGroup = LPGENT("Popups");
		odp.ptszTab = NULL;
		CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );
	}
	return 0;
}
