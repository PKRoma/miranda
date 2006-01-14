// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera, Bio
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

	 /* Perversion... needs the special Unicode build of the whole plugin */
	 if ( gbUnicodeAPI ) {
		WCHAR* p = (WCHAR*)CallService(MS_LANGPACK_PCHARTOTCHAR, 0, (LPARAM)names[i].text);
		if (( int )p != CALLSERVICE_NOTFOUND ) {
  			iItem = SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)p);
			free(p);
		}
		else iItem = SendMessageA(hwndCombo, CB_ADDSTRING, 0, (LPARAM)TranslateT(names[i].text));
	 }
	 else iItem = SendMessageA(hwndCombo, CB_ADDSTRING, 0, (LPARAM)TranslateT(names[i].text));
    SendMessage(hwndCombo, CB_SETITEMDATA, iItem,names[i].code);
  }
}



BOOL CALLBACK AdvancedSearchDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case WM_INITDIALOG:
      ICQTranslateDialog(hwndDlg);
      InitComboBox(GetDlgItem(hwndDlg, IDC_GENDER), genderField);
      InitComboBox(GetDlgItem(hwndDlg, IDC_AGERANGE), agesField);
      InitComboBox(GetDlgItem(hwndDlg, IDC_MARITALSTATUS), maritalField);
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
          iItem = SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_ADDSTRING, 0, (LPARAM)ICQTranslate(countries[i].szName));
          SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_SETITEMDATA, iItem, countries[i].id);
        }
      }

      InitComboBox(GetDlgItem(hwndDlg, IDC_INTERESTSCAT), interestsField);
      InitComboBox(GetDlgItem(hwndDlg, IDC_PASTCAT), pastField);

      return TRUE;
      
    case WM_COMMAND:
      {
        switch(LOWORD(wParam))
        {
          
        case IDOK:
          SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(GetParent(hwndDlg), IDOK));
          break;
          
        case IDCANCEL:
          //          CheckDlgButton(GetParent(hwndDlg),IDC_ADVANCED,BST_UNCHECKED);
          //          SendMessage(GetParent(hwndDlg),WM_COMMAND,MAKEWPARAM(IDC_ADVANCED,BN_CLICKED),(LPARAM)GetDlgItem(GetParent(hwndDlg),IDC_ADVANCED));
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



/*static void searchPackLNTS(PBYTE *buf, int *buflen, HWND hwndDlg, UINT idControl)
{
  char str[512];

  GetDlgItemText(hwndDlg, idControl, str, sizeof(str));

  ppackLELNTS(buf, buflen, str);
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
  ppackLEDWord(&buf, &buflen, (DWORD)SendDlgItemMessage(hwndDlg, IDC_AGERANGE, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_AGERANGE, CB_GETCURSEL, 0, 0), 0));
  ppackByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETCURSEL, 0, 0), 0));
  ppackByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETCURSEL, 0, 0), 0));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_CITY);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_STATE);
  ppackLEWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_GETCURSEL, 0, 0), 0));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_COMPANY);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_DEPARTMENT);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_POSITION);
  ppackByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_WORKFIELD, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_WORKFIELD, CB_GETCURSEL, 0, 0), 0));
  ppackLEWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_PASTCAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_PASTCAT, CB_GETCURSEL, 0, 0), 0));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_PASTKEY);
  ppackLEWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_INTERESTSCAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_INTERESTSCAT, CB_GETCURSEL, 0, 0), 0));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_INTERESTSKEY);
  ppackLEWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_ORGANISATION, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_ORGANISATION, CB_GETCURSEL, 0, 0), 0));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_ORGKEYWORDS);
  ppackLEWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_GETCURSEL, 0, 0), 0));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_HOMEPAGEKEY);
  ppackByte(&buf, &buflen, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ONLINEONLY));

  if (length)
    *length = buflen;

  return buf;
}*/


static void searchPackTLVLNTS(PBYTE *buf, int *buflen, HWND hwndDlg, UINT idControl, WORD wType)
{
  char str[512];

  GetDlgItemText(hwndDlg, idControl, str, sizeof(str));

  ppackTLVLNTS(buf, buflen, str, wType, 0);
}


static void searchPackTLVWordLNTS(PBYTE *buf, int *buflen, HWND hwndDlg, UINT idControl, WORD w, WORD wType)
{
  char str[512];

  GetDlgItemText(hwndDlg, idControl, str, sizeof(str));

  ppackTLVWordLNTS(buf, buflen, w, str, wType, 0);
}


PBYTE createAdvancedSearchStructure(HWND hwndDlg, int *length)
{
  PBYTE buf = NULL;
  int buflen = 0;
  WORD w;

  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_FIRSTNAME, TLV_FIRSTNAME);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_LASTNAME, TLV_LASTNAME);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_NICK, TLV_NICKNAME);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_EMAIL, TLV_EMAIL);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_CITY, TLV_CITY);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_STATE, TLV_STATE);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_COMPANY, TLV_COMPANY);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_DEPARTMENT, TLV_DEPARTMENT);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_POSITION, TLV_POSITION);
  searchPackTLVLNTS(&buf, &buflen, hwndDlg, IDC_KEYWORDS, TLV_KEYWORDS);

  ppackTLVDWord(&buf, &buflen, (DWORD)SendDlgItemMessage(hwndDlg, IDC_AGERANGE, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_AGERANGE, CB_GETCURSEL, 0, 0), 0), TLV_AGERANGE, 0);
  ppackTLVByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETCURSEL, 0, 0), 0), TLV_GENDER, 0);
  ppackTLVByte(&buf, &buflen, (BYTE)SendDlgItemMessage(hwndDlg, IDC_MARITALSTATUS, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_MARITALSTATUS, CB_GETCURSEL, 0, 0), 0), TLV_MARITAL, 0);
  ppackTLVWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETCURSEL, 0, 0), 0), TLV_LANGUAGE, 0);
  ppackTLVWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_COUNTRY, CB_GETCURSEL, 0, 0), 0), TLV_COUNTRY, 0);
  ppackTLVWord(&buf, &buflen, (WORD)SendDlgItemMessage(hwndDlg, IDC_WORKFIELD, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_WORKFIELD, CB_GETCURSEL, 0, 0), 0), TLV_OCUPATION, 0);

  w = (WORD)SendDlgItemMessage(hwndDlg, IDC_PASTCAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_PASTCAT, CB_GETCURSEL, 0, 0), 0);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_PASTKEY, w, TLV_PASTINFO);

  w = (WORD)SendDlgItemMessage(hwndDlg, IDC_INTERESTSCAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_INTERESTSCAT, CB_GETCURSEL, 0, 0), 0);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_INTERESTSKEY, w, TLV_INTERESTS);

  w = (WORD)SendDlgItemMessage(hwndDlg, IDC_ORGANISATION, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_ORGANISATION, CB_GETCURSEL, 0, 0), 0);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_ORGKEYWORDS, w, TLV_AFFILATIONS);

  w = (WORD)SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_HOMEPAGECAT, CB_GETCURSEL, 0, 0), 0);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_HOMEPAGEKEY, w, TLV_HOMEPAGE);

  ppackTLVByte(&buf, &buflen, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ONLINEONLY), TLV_ONLINEONLY, 1);

  if (length)
    *length = buflen;

  return buf;
}
