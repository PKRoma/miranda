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



extern HANDLE hInst;
extern int gnCurrentStatus;
extern char gpszICQProtoName[MAX_PATH];

static BOOL CALLBACK AskAuthProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);



int icq_RequestAuthorization(WPARAM wParam, LPARAM lParam)
{

	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ASKAUTH), NULL, AskAuthProc, (LPARAM)wParam);

	return 0;

}



static BOOL CALLBACK AskAuthProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	static char szReason[256];
	static HANDLE hContact;


	switch (msg)
	{

	case WM_INITDIALOG:
		hContact = (HANDLE)lParam;

		if (!hContact || !icqOnline)
			EndDialog(hwndDlg, 0);

		TranslateDialogDefault(hwndDlg);
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		SendDlgItemMessage(hwndDlg, IDC_EDITAUTH, EM_LIMITTEXT, (WPARAM)255, 0);
		SetDlgItemText(hwndDlg, IDC_EDITAUTH, Translate("Please authorize me to add you to my contact list."));
		
		return TRUE;
		
	case WM_COMMAND:
        {
            switch (LOWORD(wParam))
			{

			case IDOK:
                {

					DWORD dwUin;

					
					dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);
					
					if (!dwUin || !icqOnline)
						return TRUE;
					
					GetDlgItemText(hwndDlg, IDC_EDITAUTH, szReason, 255);
					icq_sendAuthReqServ(dwUin, szReason);
					EndDialog(hwndDlg, 0);

					return TRUE;

                }
				break;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
				break;

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
