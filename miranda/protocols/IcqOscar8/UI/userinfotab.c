// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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
//
// -----------------------------------------------------------------------------
//
// File name      : $Source$
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



extern char gpszICQProtoName[MAX_PATH];
extern HANDLE hInst;

static BOOL CALLBACK IcqDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void SetValue(HWND hwndDlg, int idCtrl, HANDLE hContact, char *szModule, char *szSetting, int special);

#define SVS_NORMAL        0
#define SVS_GENDER        1
#define SVS_ZEROISUNSPEC  2
#define SVS_IP            3
#define SVS_COUNTRY       4
#define SVS_MONTH         5
#define SVS_SIGNED        6
#define SVS_TIMEZONE      7
#define SVS_ICQVERSION    8
#define SVS_TIMESTAMP     9



int OnDetailsInit(WPARAM wParam, LPARAM lParam)
{

	char* szProto;
	OPTIONSDIALOGPAGE odp;


	szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
    if ((szProto == NULL || strcmp(szProto, gpszICQProtoName)) && lParam)
		return 0;

		
	odp.cbSize = sizeof(odp);
	odp.hIcon = NULL;
	odp.hInstance = hInst;
	odp.pfnDlgProc = IcqDlgProc;
	odp.position = -1900000000;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_ICQ);
	odp.pszTitle = Translate(gpszICQProtoName);

	CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
	
	return 0;
	
}



static BOOL CALLBACK IcqDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	switch (msg)
	{

	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		if ((HANDLE)lParam == NULL)
			ShowWindow(GetDlgItem(hwndDlg, IDC_CHANGEDETAILS), SW_SHOW);
		return TRUE;
		
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom)
			{
				
			case 0:
				{
					
					switch (((LPNMHDR)lParam)->code)
					{
						
					case PSN_INFOCHANGED:
						{

							char* szProto;
							HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;
							
							
							if (hContact == NULL)
								szProto = gpszICQProtoName;
							else
								szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
							
							if (szProto == NULL)
								break;
							
							SetValue(hwndDlg, IDC_UIN, hContact, szProto, UNIQUEIDSETTING, 0);
							SetValue(hwndDlg, IDC_IP, hContact, szProto, "IP", SVS_IP);
							SetValue(hwndDlg, IDC_REALIP, hContact, szProto, "RealIP", SVS_IP);
							SetValue(hwndDlg, IDC_PORT, hContact, szProto, "UserPort", SVS_ZEROISUNSPEC);
							SetValue(hwndDlg, IDC_VERSION, hContact, szProto, "Version", SVS_ICQVERSION);
							SetValue(hwndDlg, IDC_MIRVER, hContact, szProto, "MirVer", SVS_ZEROISUNSPEC);
							SetValue(hwndDlg, IDC_ONLINESINCE, hContact, szProto, "LogonTS", SVS_TIMESTAMP);
							if (DBGetContactSettingDword(hContact, szProto, "ClientID", 0) == 1)
								DBWriteContactSettingDword(hContact, szProto, "TickTS", 0);
							SetValue(hwndDlg, IDC_SYSTEMUPTIME, hContact, szProto, "TickTS", SVS_TIMESTAMP);
							
						}
						break;
						
					}
				}
				break;
			}
		}
		break;
		
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			
		case IDC_CHANGEDETAILS:
			{
				
				CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://www.icq.com/whitepages/user_details.php");

			}
			break;
			
		case IDCANCEL:
			SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
			break;
			
		}
		break;
	}

	
	return FALSE;
	
}



static void SetValue(HWND hwndDlg, int idCtrl, HANDLE hContact, char* szModule, char* szSetting, int special)
{
	
	DBVARIANT dbv;
	char str[80];
	char* pstr;
	int unspecified = 0;

	
	dbv.type = DBVT_DELETED;
	
	if (szModule == NULL)
		unspecified = 1;
	else
		unspecified = DBGetContactSetting(hContact, szModule, szSetting, &dbv);
	
	if (!unspecified)
	{
		switch (dbv.type)
		{
			
		case DBVT_BYTE:
			if (special == SVS_GENDER)
			{
				if (dbv.cVal == 'M')
					pstr = Translate("Male");
				else if (dbv.cVal == 'F')
					pstr = Translate("Female");
				else
					unspecified = 1;
			}
			else if (special == SVS_MONTH)
			{
				if (dbv.bVal>0 && dbv.bVal<=12)
				{
					pstr = str;
					GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVMONTHNAME1-1+dbv.bVal, str,sizeof(str));
				}
				else
					unspecified = 1;
			}
			else if (special == SVS_TIMEZONE)
			{
				if (dbv.cVal == -100)
					unspecified = 1;
				else
				{
					pstr = str;
					_snprintf(str, 80, dbv.cVal ? "GMT%+d:%02d":"GMT", -dbv.cVal/2, (dbv.cVal&1)*30);
				}
			}
			else
			{
				unspecified = (special == SVS_ZEROISUNSPEC && dbv.bVal == 0);
				pstr = itoa(special == SVS_SIGNED ? dbv.cVal:dbv.bVal, str, 10);
			}
			break;
			
		case DBVT_WORD:
			if (special == SVS_COUNTRY)
			{
				pstr = (char*)CallService(MS_UTILS_GETCOUNTRYBYNUMBER, dbv.wVal, 0);
				unspecified = pstr == NULL;
			}
			else if (special == SVS_ICQVERSION)
			{
				if (dbv.wVal != 0)
				{
					static char *szVersionDescr[] = {"", "ICQ 1.x", "ICQ 2.x", "Unknown", "ICQ98", "Unknown", "ICQ99 / licq", "Icq2Go or ICQ2000", "ICQ2001-2003a, Miranda or Trillian", "ICQ Lite", "ICQ 2003b"};
					pstr = str;
					_snprintf(str, 80, "%d: %s", dbv.wVal, dbv.wVal > 10 ? Translate("Unknown") : Translate(szVersionDescr[dbv.wVal]));
				}
				else
					unspecified = 1;
			}
			else
			{
				unspecified = (special == SVS_ZEROISUNSPEC && dbv.wVal == 0);
				pstr = itoa(special == SVS_SIGNED ? dbv.sVal:dbv.wVal, str, 10);
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
				{
					pstr = asctime(localtime(&dbv.dVal));
					pstr[24] = '\0'; // Remove newline
				}
			}
			else
				pstr = itoa(special == SVS_SIGNED ? dbv.lVal:dbv.dVal, str, 10);
			break;
			
		case DBVT_ASCIIZ:
			unspecified = (special == SVS_ZEROISUNSPEC && dbv.pszVal[0] == '\0');
			pstr = dbv.pszVal;
			break;
			
		default:
			pstr = str;
			lstrcpy(str,"???");
			break;
			
		}
	}
	
	EnableWindow(GetDlgItem(hwndDlg, idCtrl), !unspecified);
	if (unspecified)
		SetDlgItemText(hwndDlg, idCtrl, Translate("<not specified>"));
	else
		SetDlgItemText(hwndDlg, idCtrl, pstr);
	
	DBFreeVariant(&dbv);
	
}
