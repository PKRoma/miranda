/*
SRMM

Copyright 2000-2005 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"

INT_PTR CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			RECT rc, rcParent;
			char *pszError = (char *) lParam;

			TranslateDialogDefault(hwndDlg);

			if (!pszError||!strlen(pszError))
				SetDlgItemText(hwndDlg, IDC_ERRORTEXT, TranslateT("An unknown error has occured."));
			else {
				TCHAR* ptszError = (TCHAR*)CallService(MS_LANGPACK_PCHARTOTCHAR,0,(LPARAM)pszError);

				SetDlgItemText(hwndDlg, IDC_ERRORTEXT, ptszError);
				mir_free(ptszError);
			}

			GetWindowRect(hwndDlg, &rc);
			GetWindowRect(GetParent(hwndDlg), &rcParent);
			SetWindowPos(hwndDlg, 0, (rcParent.left + rcParent.right - (rc.right - rc.left)) / 2, (rcParent.top + rcParent.bottom - (rc.bottom - rc.top)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SendMessage(GetParent(hwndDlg), DM_ERRORDECIDED, MSGERROR_RETRY, 0);
			DestroyWindow(hwndDlg);
			break;
		case IDCANCEL:
			SendMessage(GetParent(hwndDlg), DM_ERRORDECIDED, MSGERROR_CANCEL, 0);
			DestroyWindow(hwndDlg);
			break;
		}
		break;

	case DM_ERRORDECIDED:
		if (wParam != MSGERROR_DONE) break;
		SendMessage(GetParent(hwndDlg), DM_ERRORDECIDED, MSGERROR_DONE, 0);
		DestroyWindow(hwndDlg);
		break;
	}
	return FALSE;
}
