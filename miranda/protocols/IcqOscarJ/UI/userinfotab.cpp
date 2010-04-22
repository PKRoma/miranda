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
//  Code for User details ICQ specific pages
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

#define SVS_NORMAL        0
#define SVS_ZEROISUNSPEC  2
#define SVS_IP            3
#define SVS_SIGNED        6
#define SVS_ICQVERSION    8
#define SVS_TIMESTAMP     9
#define SVS_STATUSID      10

char* MirandaVersionToString(char* szStr, int bUnicode, int v, int m);

extern char *nameXStatus[];


/////////////////////////////////////////////////////////////////////////////////////////

static void SetValue(CIcqProto* ppro, HWND hwndDlg, int idCtrl, HANDLE hContact, char* szModule, char* szSetting, int special)
{
	DBVARIANT dbv = {0};
	char str[MAX_PATH];
	char* pstr = NULL;
	int unspecified = 0;
	int bUtf = 0, bDbv = 0, bAlloc = 0;

	dbv.type = DBVT_DELETED;

	if ((hContact == NULL) && ((int)szModule<0x100))
	{
		dbv.type = (BYTE)szModule;

		switch((int)szModule) {
		case DBVT_BYTE:
			dbv.cVal = (BYTE)szSetting;
			break;
		case DBVT_WORD:
			dbv.wVal = (WORD)szSetting;
			break;
		case DBVT_DWORD:
			dbv.dVal = (DWORD)szSetting;
			break;
		case DBVT_ASCIIZ:
			dbv.pszVal = pstr = szSetting;
			break;
		default:
			unspecified = 1;
			dbv.type = DBVT_DELETED;
		}
	}
	else
	{
		if (szModule == NULL)
			unspecified = 1;
		else
		{
			unspecified = DBGetContactSetting(hContact, szModule, szSetting, &dbv);
			bDbv = 1;
		}
	}

	if (!unspecified)
	{
		switch (dbv.type) {
		case DBVT_BYTE:
			unspecified = (special == SVS_ZEROISUNSPEC && dbv.bVal == 0);
			pstr = _itoa(special == SVS_SIGNED ? dbv.cVal:dbv.bVal, str, 10);
			break;

		case DBVT_WORD:
			if (special == SVS_ICQVERSION)
			{
				if (dbv.wVal != 0)
				{
					char szExtra[80];

					null_snprintf(str, 250, "%d", dbv.wVal);
					pstr = str;

					if (hContact && ppro->IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 1))
					{
						ICQTranslateUtfStatic(LPGEN(" (DC Established)"), szExtra, 80);
						strcat(str, (char*)szExtra);
						bUtf = 1;
					}
				}
				else
					unspecified = 1;
			}
			else if (special == SVS_STATUSID)
			{
				char *pXName;
				char *pszStatus = MirandaStatusToStringUtf(dbv.wVal);
				BYTE bXStatus = ppro->getContactXStatus(hContact);

				if (bXStatus)
				{
					pXName = ppro->getSettingStringUtf(hContact, DBSETTING_XSTATUS_NAME, NULL);
					if (!strlennull(pXName))
					{ // give default name
						pXName = ICQTranslateUtf(nameXStatus[bXStatus-1]);
					}
					null_snprintf(str, sizeof(str), "%s (%s)", pszStatus, pXName);
					SAFE_FREE((void**)&pXName);
				}
				else
					null_snprintf(str, sizeof(str), pszStatus);

				bUtf = 1;
				SAFE_FREE(&pszStatus);
				pstr = str;
				unspecified = 0;
			}
			else
			{
				unspecified = (special == SVS_ZEROISUNSPEC && dbv.wVal == 0);
				pstr = _itoa(special == SVS_SIGNED ? dbv.sVal:dbv.wVal, str, 10);
			}
			break;

		case DBVT_DWORD:
			unspecified = (special == SVS_ZEROISUNSPEC && dbv.dVal == 0);
			if (special == SVS_IP)
			{
				struct in_addr ia;
				ia.S_un.S_addr = htonl(dbv.dVal);
				pstr = inet_ntoa(ia);
				if (dbv.dVal == 0)
					unspecified=1;
			}
			else if (special == SVS_TIMESTAMP)
			{
				if (dbv.dVal == 0)
					unspecified = 1;
				else
					pstr = time2text(dbv.dVal);
			}
			else
				pstr = _itoa(special == SVS_SIGNED ? dbv.lVal:dbv.dVal, str, 10);
			break;

		case DBVT_ASCIIZ:
		case DBVT_WCHAR:
			unspecified = (special == SVS_ZEROISUNSPEC && dbv.pszVal[0] == '\0');
			if (!unspecified && pstr != szSetting)
			{
				pstr = ppro->getSettingStringUtf(hContact, szModule, szSetting, NULL);
				bUtf = 1;
				bAlloc = 1;
			}
			if (idCtrl == IDC_UIN)
				SetDlgItemTextUtf(hwndDlg, IDC_UINSTATIC, ICQTranslateUtfStatic(LPGEN("ScreenName:"), str, MAX_PATH));
			break;

		default:
			pstr = str;
			strcpy(str,"???");
			break;
		}
	}

	EnableDlgItem(hwndDlg, idCtrl, !unspecified);
	if (unspecified)
		SetDlgItemTextUtf(hwndDlg, idCtrl, ICQTranslateUtfStatic(LPGEN("<not specified>"), str, MAX_PATH));
	else if (bUtf)
		SetDlgItemTextUtf(hwndDlg, idCtrl, pstr);
	else
		SetDlgItemTextA(hwndDlg, idCtrl, pstr);

	if (bDbv)
		ICQFreeVariant(&dbv);

	if (bAlloc)
		SAFE_FREE(&pstr);
}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK IcqDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		ICQTranslateDialog(hwndDlg);
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (( PSHNOTIFY* )lParam )->lParam );
				break;

			case PSN_INFOCHANGED:
				{
					CIcqProto* ppro = (CIcqProto*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

          if (!ppro)
            break;

					char* szProto;
					HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;

					if (hContact == NULL)
						szProto = ppro->m_szModuleName;
					else
						szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

					if (!szProto)
						break;

					SetValue(ppro, hwndDlg, IDC_UIN, hContact, szProto, UNIQUEIDSETTING, SVS_NORMAL);
					SetValue(ppro, hwndDlg, IDC_ONLINESINCE, hContact, szProto, "LogonTS", SVS_TIMESTAMP);
					SetValue(ppro, hwndDlg, IDC_IDLETIME, hContact, szProto, "IdleTS", SVS_TIMESTAMP);
					SetValue(ppro, hwndDlg, IDC_IP, hContact, szProto, "IP", SVS_IP);
					SetValue(ppro, hwndDlg, IDC_REALIP, hContact, szProto, "RealIP", SVS_IP);

					if (hContact)
					{
						SetValue(ppro, hwndDlg, IDC_PORT, hContact, szProto, "UserPort", SVS_ZEROISUNSPEC);
						SetValue(ppro, hwndDlg, IDC_VERSION, hContact, szProto, "Version", SVS_ICQVERSION);
						SetValue(ppro, hwndDlg, IDC_MIRVER, hContact, szProto, "MirVer", SVS_ZEROISUNSPEC);
						if (ppro->getSettingByte(hContact, "ClientID", 0))
							ppro->setSettingDword(hContact, "TickTS", 0);
						SetValue(ppro, hwndDlg, IDC_SYSTEMUPTIME, hContact, szProto, "TickTS", SVS_TIMESTAMP);
						SetValue(ppro, hwndDlg, IDC_STATUS, hContact, szProto, "Status", SVS_STATUSID);
					}
					else
					{
						char str[MAX_PATH];

						SetValue(ppro, hwndDlg, IDC_PORT, hContact, (char*)DBVT_WORD, (char*)ppro->wListenPort, SVS_ZEROISUNSPEC);
						SetValue(ppro, hwndDlg, IDC_VERSION, hContact, (char*)DBVT_WORD, (char*)ICQ_VERSION, SVS_ICQVERSION);
#if defined( _UNICODE )
						SetValue(ppro, hwndDlg, IDC_MIRVER, hContact, (char*)DBVT_ASCIIZ, MirandaVersionToString(str, TRUE, ICQ_PLUG_VERSION, MIRANDA_VERSION), SVS_ZEROISUNSPEC);
#else
						SetValue(ppro, hwndDlg, IDC_MIRVER, hContact, (char*)DBVT_ASCIIZ, MirandaVersionToString(str, FALSE, ICQ_PLUG_VERSION, MIRANDA_VERSION), SVS_ZEROISUNSPEC);
#endif
						SetDlgItemTextUtf(hwndDlg, IDC_SUPTIME, ICQTranslateUtfStatic(LPGEN("Member since:"), str, MAX_PATH));
						SetValue(ppro, hwndDlg, IDC_SYSTEMUPTIME, hContact, szProto, "MemberTS", SVS_TIMESTAMP);
						SetValue(ppro, hwndDlg, IDC_STATUS, hContact, (char*)DBVT_WORD, (char*)ppro->m_iStatus, SVS_STATUSID);
					}
				}
				break;
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDCANCEL:
			SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
			break;
		}
		break;
	}

	return FALSE;  
}

/////////////////////////////////////////////////////////////////////////////////////////

int CIcqProto::OnUserInfoInit(WPARAM wParam, LPARAM lParam)
{
	if ((!IsICQContact((HANDLE)lParam)) && lParam)
		return 0;

	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.flags = ODPF_TCHAR | ODPF_DONTTRANSLATE;
	odp.hInstance = hInst;
	odp.dwInitParam = LPARAM(this);
	odp.pfnDlgProc = IcqDlgProc;
	odp.position = -1900000000;
	odp.ptszTitle = m_tszUserName;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_INFO_ICQ);
	CallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	if (!lParam)
	{
		TCHAR buf[200];
		null_snprintf(buf, SIZEOF(buf), TranslateT("%s Details"), m_tszUserName);
		odp.ptszTitle = buf;

		odp.position = -1899999999;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_INFO_CHANGEINFO);
		odp.pfnDlgProc = ChangeInfoDlgProc;
		CallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );
	}
	return 0;
}
