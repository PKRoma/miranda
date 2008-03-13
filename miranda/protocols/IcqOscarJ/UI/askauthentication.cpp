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


static BOOL CALLBACK AskAuthProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


int icq_RequestAuthorization(WPARAM wParam, LPARAM lParam)
{
  DialogBoxUtf(TRUE, hInst, MAKEINTRESOURCEA(IDD_ASKAUTH), NULL, AskAuthProc, (LPARAM)wParam);

  return 0;
}



static BOOL CALLBACK AskAuthProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HANDLE hContact;

  switch (msg)
  {

  case WM_INITDIALOG:
    {
      char str[MAX_PATH];

      hContact = (HANDLE)lParam;

      if (!hContact || !icqOnline)
        EndDialog(hwndDlg, 0);

      ICQTranslateDialog(hwndDlg);
      SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
      SendDlgItemMessage(hwndDlg, IDC_EDITAUTH, EM_LIMITTEXT, (WPARAM)255, 0);
      SetDlgItemTextUtf(hwndDlg, IDC_EDITAUTH, ICQTranslateUtfStatic("Please authorize me to add you to my contact list.", str, MAX_PATH));

      return TRUE;
    }

  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDOK:
        {
          DWORD dwUin;
          uid_str szUid;
          char* szReason;

          hContact = (HANDLE)GetWindowLong(hwndDlg, GWL_USERDATA);

          if (!icqOnline)
            return TRUE;

          if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
            return TRUE; // Invalid contact

          szReason = GetDlgItemTextUtf(hwndDlg, IDC_EDITAUTH);
          icq_sendAuthReqServ(dwUin, szUid, szReason);
          SAFE_FREE((void**)&szReason);

          if (gbSsiEnabled && dwUin)
          { // auth bug fix (thx Bio)
            resetServContactAuthState(hContact, dwUin);
          }

          EndDialog(hwndDlg, 0);

          return TRUE;
        }
        break;

      case IDCANCEL:
        EndDialog(hwndDlg, 0);
        return TRUE;

      default:
        break;

      }
    }
    break;

  case WM_CLOSE:
    EndDialog(hwndDlg,0);
    return TRUE;
  }
  
  return FALSE;
}
