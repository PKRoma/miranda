// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
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


BOOL CALLBACK icq_FirstRunDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


void icq_FirstRunCheck()
{
  if (getByte(NULL, "FirstRun", 0))
    return;

  DialogBoxParam(TRUE, hInst, MAKEINTRESOURCE(IDD_ICQACCOUNT), NULL, icq_FirstRunDlgProc, 0);
}



BOOL CALLBACK icq_FirstRunDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {

  case WM_INITDIALOG:
    {
      char* pszPwd;
      DWORD dwUIN;
      char pszUIN[20];


      ICQTranslateDialog(hwndDlg);
      SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICQ)));

      dwUIN = getUin(NULL);

      if (dwUIN)
      {
        null_snprintf(pszUIN, 20, "%u", dwUIN);
        SetDlgItemText(hwndDlg, IDC_UIN, pszUIN);
      }

      SendDlgItemMessage(hwndDlg, IDC_PW, EM_LIMITTEXT, 10, 0);
      pszPwd = GetUserPassword(FALSE);
      if (pszPwd)
      {
        SetDlgItemText(hwndDlg, IDC_PW, pszPwd);
      }
    }
    break;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    break;

  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {

      case IDC_REGISTER:
        {
          CallService(MS_UTILS_OPENURL, 1, (LPARAM)URL_REGISTER);
          break;
        }

      case IDOK:
        {
          char str[128];
          DWORD dwUIN;

          GetDlgItemText(hwndDlg, IDC_UIN, str, sizeof(str));
          dwUIN = atoi(str);
          setDword(NULL, UNIQUEIDSETTING, dwUIN);
          GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(m_szPassword));
          strcpy(m_szPassword, str);
          CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(m_szPassword), (LPARAM) str);
          setString(NULL, "Password", str);
        }
        // fall through

      case IDCANCEL:
        {
          // Mark first run as completed
          setByte(NULL, "FirstRun", 1);
          EndDialog(hwndDlg, IDCANCEL);
        }
        break;

      }
    }
    break;
  }

  return FALSE;
}
