// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
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



static void InitComboBox(HWND hwndCombo,struct fieldnames_t *names)
{

	int iItem;
	int i;


	iItem = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)"");
	SendMessage(hwndCombo, CB_SETITEMDATA, iItem, 0);
	SendMessage(hwndCombo, CB_SETCURSEL, iItem, 0);

	for (i = 0; ; i++)
	{
		if (names[i].text == NULL)
			break;
		iItem = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)Translate(names[i].text));
		SendMessage(hwndCombo, CB_SETITEMDATA, iItem,names[i].code);
	}

}



BOOL CALLBACK AdvancedSearchDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch(message)
	{

		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			InitComboBox(GetDlgItem(hwndDlg, IDC_GENDER), genderField);
			InitComboBox(GetDlgItem(hwndDlg, IDC_AGERANGE), agesField);
			InitComboBox(GetDlgItem(hwndDlg, IDC_WORKFIELD), workField);
			InitComboBox(GetDlgItem(hwndDlg, IDC_ORGANISATION), affiliationField);
			InitComboBox(GetDlgItem(hwndDlg, IDC_LANGUAGE), languageField);

			{

				struct CountryListEntry *countries;
				int countryCount;
				int i;
				int iItem;


				CallService(MS_UTILS_GETCOUNTRYLIST, (WPARAM)&countryCount, (LPARAM)&countries);

				iItem = SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_ADDSTRING, 0, (LPARAM)"");
				SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_SETITEMDATA, iItem, 0);
				SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_SETCURSEL, iItem, 0);
				for (i = 0; i < countryCount; i++)
				{
					if (countries[i].id == 0 || countries[i].id == 0xFFFF)
						continue;
					iItem = SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_ADDSTRING, 0, (LPARAM)Translate(countries[i].szName));
					SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_SETITEMDATA, iItem, countries[i].id);
				}

			}

			InitComboBox(GetDlgItem(hwndDlg, IDC_INTERESTSCAT), interestsField);
			InitComboBox(GetDlgItem(hwndDlg, IDC_PASTCAT), pastField);
			//InitComboBox(GetDlgItem(hwndDlg,IDC_HOMEPAGECAT),homepageField);
			//no field decodes known
			SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_ADDSTRING, 0, (LPARAM)"");
			SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_SETITEMDATA, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_ADDSTRING, 0,(LPARAM)"<help wanted>");
			SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_SETITEMDATA, 1, 0);
			SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_SETCURSEL, 0, 0);

			return TRUE;
			
		case WM_COMMAND:
			{

				switch(LOWORD(wParam))
				{
					
				case IDOK:
					SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(GetParent(hwndDlg), IDOK));
					break;
					
				case IDCANCEL:
					//					CheckDlgButton(GetParent(hwndDlg),IDC_ADVANCED,BST_UNCHECKED);
					//					SendMessage(GetParent(hwndDlg),WM_COMMAND,MAKEWPARAM(IDC_ADVANCED,BN_CLICKED),(LPARAM)GetDlgItem(GetParent(hwndDlg),IDC_ADVANCED));
					break;
					
				default:
					break;
					
				}
				break;
			}

		default:
			break;

	}

	return FALSE;

}



static void searchPackByte(PBYTE *buf, int *buflen, BYTE b)
{

	*buf = (PBYTE)realloc(*buf, 1 + *buflen);
	*(*buf + *buflen) = b;
	++*buflen;

}



static void searchPackWord(PBYTE *buf, int *buflen, WORD w)
{

	*buf = (PBYTE)realloc(*buf, 2 + *buflen);
	*(PWORD)(*buf + *buflen) = w;
	*buflen += 2;

}



static void searchPackDWord(PBYTE *buf, int *buflen, DWORD d)
{

	*buf = (PBYTE)realloc(*buf, 4 + *buflen);
	*(PDWORD)(*buf + *buflen) = d;
	*buflen += 4;

}



static void searchPackLNTS(PBYTE *buf, int *buflen, HWND hwndDlg, UINT idControl)
{

	char str[512];
	int len;


	GetDlgItemText(hwndDlg, idControl, str, sizeof(str));
	len = lstrlen(str);
	searchPackWord(buf, buflen, (WORD)len);
	*buf = (PBYTE)realloc(*buf, *buflen + len);
	memcpy(*buf + *buflen, str, len);
	*buflen += len;

}



PBYTE createAdvancedSearchStructure(HWND hwndDlg, int *length)
{

	PBYTE buf = NULL;
	int buflen = 0;


	if (hwndDlg == NULL)
		return NULL;

	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_FIRSTNAME);
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_LASTNAME);
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_NICK);
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_EMAIL);
	searchPackDWord(&buf, &buflen, (DWORD)SendDlgItemMessage(hwndDlg, IDC_AGERANGE, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_AGERANGE, CB_GETCURSEL, 0, 0), 0));
	searchPackByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETCURSEL, 0, 0), 0));
	searchPackByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETCURSEL, 0, 0), 0));
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_CITY);
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_STATE);
	searchPackWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_GETCURSEL, 0, 0), 0));
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_COMPANY);
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_DEPARTMENT);
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_POSITION);
	searchPackByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_WORKFIELD, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_WORKFIELD, CB_GETCURSEL, 0, 0), 0));
	searchPackWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_PASTCAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_PASTCAT, CB_GETCURSEL, 0, 0), 0));
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_PASTKEY);
	searchPackWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_INTERESTSCAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_INTERESTSCAT, CB_GETCURSEL, 0, 0), 0));
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_INTERESTSKEY);
	searchPackWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_ORGANISATION, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_ORGANISATION, CB_GETCURSEL, 0, 0), 0));
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_ORGKEYWORDS);
	searchPackWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_GETCURSEL, 0, 0), 0));
	searchPackLNTS(&buf, &buflen, hwndDlg, IDC_HOMEPAGEKEY);
	searchPackByte(&buf, &buflen, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ONLINEONLY));

	if (length)
		*length = buflen;

	return buf;

}
