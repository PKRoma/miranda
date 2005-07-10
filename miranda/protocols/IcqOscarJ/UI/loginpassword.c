// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera
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


BOOL CALLBACK LoginPasswdDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


void RequestPassword()
{
  DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGINPW), NULL, LoginPasswdDlgProc);
}


BOOL CALLBACK LoginPasswdDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {

    case WM_INITDIALOG:
      {
        char pszUIN[128];
        DWORD dwUin;

        ICQTranslateDialog(hwndDlg);
        SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICQ)));
        dwUin = ICQGetContactSettingDword(NULL, UNIQUEIDSETTING, 0);
        null_snprintf(pszUIN, 128, Translate("Enter a password for UIN %u:"), dwUin);
        SetDlgItemText(hwndDlg, IDC_INSTRUCTION, pszUIN);
      }
		  break;

    case WM_CLOSE:

      EndDialog(hwndDlg, 0);
      break;
		
    case WM_COMMAND:
      {
        switch (LOWORD(wParam))
        {
				
          case IDOK:
            {
              char str[128];

              GetDlgItemText(hwndDlg, IDC_LOGINPW, str, sizeof(str));
					    icq_login(str);
					    EndDialog(hwndDlg, IDOK);
            }
				    break;

          case IDCANCEL:
            SetCurrentStatus(ID_STATUS_OFFLINE);

            EndDialog(hwndDlg, IDCANCEL);
            break;
        }
      }
      break;
  }

  return FALSE;
}
