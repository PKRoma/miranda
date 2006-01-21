// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera, Bio
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

  iItem = ComboBoxAddStringUtf(hwndCombo, "", 0);
  SendMessage(hwndCombo, CB_SETCURSEL, iItem, 0);

  for (i = 0; ; i++)
  {
    if (names[i].text == NULL)
      break;
    
    iItem = ComboBoxAddStringUtf(hwndCombo, names[i].text, names[i].code);
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
        HWND hCombo;

        CallService(MS_UTILS_GETCOUNTRYLIST, (WPARAM)&countryCount, (LPARAM)&countries);

        hCombo = GetDlgItem(hwndDlg, IDC_COUNTRY);
        iItem = ComboBoxAddStringUtf(hCombo, "", 0);
        SendMessage(hCombo, CB_SETCURSEL, iItem, 0);
        for (i = 0; i < countryCount; i++)
        {
          if (countries[i].id == 0 || countries[i].id == 0xFFFF)
            continue;
          iItem = ComboBoxAddStringUtf(hCombo, countries[i].szName, countries[i].id);
        }
      }

      InitComboBox(GetDlgItem(hwndDlg, IDC_INTERESTSCAT), interestsField);
      InitComboBox(GetDlgItem(hwndDlg, IDC_PASTCAT), pastField);

      {
        BYTE bTLVSearch = ICQGetContactSettingByte(NULL, "TLVSearch", 1);

        CheckDlgButton(hwndDlg, IDC_TLVSEARCH, bTLVSearch);
        EnableDlgItem(hwndDlg, IDC_KEYWORDS, bTLVSearch);
        EnableDlgItem(hwndDlg, IDC_MARITALSTATUS, bTLVSearch);
      }

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
          
        case IDC_TLVSEARCH:
          {
            BYTE On = IsDlgButtonChecked(hwndDlg, IDC_TLVSEARCH);
            EnableDlgItem(hwndDlg, IDC_KEYWORDS, On);
            EnableDlgItem(hwndDlg, IDC_MARITALSTATUS, On);
          }
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



static void searchPackLNTS(PBYTE *buf, int *buflen, HWND hwndDlg, UINT idControl)
{
  char str[512];

  GetDlgItemText(hwndDlg, idControl, str, sizeof(str));

  ppackLELNTS(buf, buflen, str);
}



static DWORD getCurItemData(HWND hwndDlg, UINT iCtrl)
{
  return SendDlgItemMessage(hwndDlg, iCtrl, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, iCtrl, CB_GETCURSEL, 0, 0), 0);
}



static PBYTE createAdvancedSearchStructureOld(HWND hwndDlg, int *length)
{
  PBYTE buf = NULL;
  int buflen = 0;

  ppackLEWord(&buf, &buflen, META_SEARCH_ADVANCED);

  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_FIRSTNAME);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_LASTNAME);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_NICK);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_EMAIL);
  ppackLEDWord(&buf, &buflen, (DWORD)getCurItemData(hwndDlg, IDC_AGERANGE));
  ppackByte(&buf, &buflen, (BYTE)getCurItemData(hwndDlg, IDC_GENDER));
  ppackByte(&buf, &buflen, (BYTE)getCurItemData(hwndDlg, IDC_LANGUAGE));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_CITY);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_STATE);
  ppackLEWord(&buf, &buflen, (WORD)getCurItemData(hwndDlg, IDC_COUNTRY));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_COMPANY);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_DEPARTMENT);
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_POSITION);
  ppackByte(&buf, &buflen, (BYTE)getCurItemData(hwndDlg, IDC_WORKFIELD));
  ppackLEWord(&buf, &buflen, (WORD)getCurItemData(hwndDlg, IDC_PASTCAT));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_PASTKEY);
  ppackLEWord(&buf, &buflen, (WORD)getCurItemData(hwndDlg, IDC_INTERESTSCAT));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_INTERESTSKEY);
  ppackLEWord(&buf, &buflen, (WORD)getCurItemData(hwndDlg, IDC_ORGANISATION));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_ORGKEYWORDS);
  ppackLEWord(&buf, &buflen, (WORD)getCurItemData(hwndDlg, IDC_HOMEPAGECAT));
  searchPackLNTS(&buf, &buflen, hwndDlg, IDC_HOMEPAGEKEY);
  ppackByte(&buf, &buflen, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ONLINEONLY));

  if (length)
    *length = buflen;

  return buf;
}



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



static PBYTE createAdvancedSearchStructureTLV(HWND hwndDlg, int *length)
{
  PBYTE buf = NULL;
  int buflen = 0;
  WORD w;

  ppackLEWord(&buf, &buflen, META_SEARCH_GENERIC);       /* subtype: full search */

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

  ppackTLVDWord(&buf, &buflen, (DWORD)getCurItemData(hwndDlg, IDC_AGERANGE),      TLV_AGERANGE,  0);
  ppackTLVByte(&buf,  &buflen, (BYTE)getCurItemData(hwndDlg,  IDC_GENDER),        TLV_GENDER,    0);
  ppackTLVByte(&buf,  &buflen, (BYTE)getCurItemData(hwndDlg,  IDC_MARITALSTATUS), TLV_MARITAL,   0);
  ppackTLVWord(&buf,  &buflen, (WORD)getCurItemData(hwndDlg,  IDC_LANGUAGE),      TLV_LANGUAGE,  0);
  ppackTLVWord(&buf,  &buflen, (WORD)getCurItemData(hwndDlg,  IDC_COUNTRY),       TLV_COUNTRY,   0);
  ppackTLVWord(&buf,  &buflen, (WORD)getCurItemData(hwndDlg,  IDC_WORKFIELD),     TLV_OCUPATION, 0);

  w = (WORD)getCurItemData(hwndDlg, IDC_PASTCAT);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_PASTKEY, w, TLV_PASTINFO);

  w = (WORD)getCurItemData(hwndDlg, IDC_INTERESTSCAT);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_INTERESTSKEY, w, TLV_INTERESTS);

  w = (WORD)getCurItemData(hwndDlg, IDC_ORGANISATION);
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_ORGKEYWORDS, w, TLV_AFFILATIONS);

  w = (WORD)getCurItemData(hwndDlg, IDC_HOMEPAGECAT);;
  searchPackTLVWordLNTS(&buf, &buflen, hwndDlg, IDC_HOMEPAGEKEY, w, TLV_HOMEPAGE);

  ppackTLVByte(&buf, &buflen, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ONLINEONLY), TLV_ONLINEONLY, 1);

  if (length)
    *length = buflen;

  return buf;
}



PBYTE createAdvancedSearchStructure(HWND hwndDlg, int *length)
{
  BYTE bTLVSearch;

  if (hwndDlg == NULL)
    return NULL;

  bTLVSearch = (BYTE)IsDlgButtonChecked(hwndDlg, IDC_TLVSEARCH);
  ICQWriteContactSettingByte(NULL, "TLVSearch", bTLVSearch);

  if (bTLVSearch)
    return createAdvancedSearchStructureTLV(hwndDlg, length);
  else
    return createAdvancedSearchStructureOld(hwndDlg, length);
}
