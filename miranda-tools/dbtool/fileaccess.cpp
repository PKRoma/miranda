/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

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
#include "dbtool.h"

BOOL CALLBACK FileAccessDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	BOOL bReturn;

	if(DoMyControlProcessing(hdlg,message,wParam,lParam,&bReturn)) return bReturn;
	switch(message) {
		case WM_INITDIALOG:
			CheckDlgButton(hdlg,IDC_CHECKONLY,opts.bCheckOnly);
			CheckDlgButton(hdlg,IDC_BACKUP,opts.bBackup);
			CheckDlgButton(hdlg,IDC_AGGRESSIVE,opts.bAggressive);
			SendMessage(hdlg,WM_COMMAND,MAKEWPARAM(IDC_CHECKONLY,BN_CLICKED),0);
			TranslateDialog(hdlg);
			return TRUE;
		case WZN_PAGECHANGING:
			opts.bCheckOnly=IsDlgButtonChecked(hdlg,IDC_CHECKONLY);
			opts.bAggressive=IsDlgButtonChecked(hdlg,IDC_AGGRESSIVE);
			if(opts.bCheckOnly) opts.bBackup=0;
			else opts.bBackup=IsDlgButtonChecked(hdlg,IDC_BACKUP);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BACK:
					SendMessage(GetParent(hdlg),WZM_GOTOPAGE,IDD_SELECTDB,(LPARAM)SelectDbDlgProc);
					break;
				case IDOK:
					if(opts.bCheckOnly) {
						if (!opts.hFile) {
							opts.hFile = CreateFile( opts.filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
							if ( opts.hFile == INVALID_HANDLE_VALUE ) {
								opts.hFile = NULL;
								opts.error = GetLastError();
								SendMessage( GetParent(hdlg), WZM_GOTOPAGE, IDD_OPENERROR, ( LPARAM )OpenErrorDlgProc );
								break;
							}
						}
						SendMessage(GetParent(hdlg),WZM_GOTOPAGE,IDD_PROGRESS,(LPARAM)ProgressDlgProc);
					}
					else
						SendMessage(GetParent(hdlg),WZM_GOTOPAGE,IDD_CLEANING,(LPARAM)CleaningDlgProc);
					break;
				case IDC_CHECKONLY:
					EnableWindow(GetDlgItem(hdlg,IDC_BACKUP),!IsDlgButtonChecked(hdlg,IDC_CHECKONLY));
					EnableWindow(GetDlgItem(hdlg,IDC_STBACKUP),!IsDlgButtonChecked(hdlg,IDC_CHECKONLY));
					break;
			}
			break;
	}
	return FALSE;
}
