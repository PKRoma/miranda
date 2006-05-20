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

BOOL CALLBACK SelectDbDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);

#define WM_LAUNCHMIRANDA  (WM_USER+1)
BOOL CALLBACK FinishedDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	BOOL bReturn;

	if(DoMyControlProcessing(hdlg,message,wParam,lParam,&bReturn)) return bReturn;
	switch(message) {
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(GetParent(hdlg),IDC_BACK),FALSE);
			SetDlgItemText(GetParent(hdlg),IDCANCEL,"&Finish");
			SetWindowLong(GetDlgItem(hdlg,IDC_DBFILE),GWL_STYLE,GetWindowLong(GetDlgItem(hdlg,IDC_DBFILE),GWL_STYLE)|SS_PATHELLIPSIS);
			SetDlgItemText(hdlg,IDC_DBFILE,opts.filename);
			if(opts.bBackup) {
				ShowWindow(GetDlgItem(hdlg,IDC_STBACKUP),TRUE);
				SetWindowLong(GetDlgItem(hdlg,IDC_BACKUPFILE),GWL_STYLE,GetWindowLong(GetDlgItem(hdlg,IDC_BACKUPFILE),GWL_STYLE)|SS_PATHELLIPSIS);
				SetDlgItemText(hdlg,IDC_BACKUPFILE,opts.backupFilename);
			}
			else ShowWindow(GetDlgItem(hdlg,IDC_STBACKUP),FALSE);
			return TRUE;
		case WM_LAUNCHMIRANDA:
			if(IsDlgButtonChecked(hdlg,IDC_LAUNCHMIRANDA)) {
				char dbFile[MAX_PATH],dbPath[MAX_PATH],*str2;
				strcpy(dbPath,opts.filename);
				str2=strrchr(dbPath,'\\');
				if(str2==NULL) {
					strcpy(dbFile,dbPath);
					dbPath[0]=0;
				}
				else {
					strcpy(dbFile,str2+1);
					*str2=0;
				}
				str2=strrchr(dbFile,'.');
				if(str2!=NULL) *str2=0;
				strcat(dbPath,"\\miranda32.exe");
				ShellExecute(hdlg,NULL,dbPath,dbFile,"",0);
			}
			break;
		case WZN_CANCELCLICKED:
			SendMessage(hdlg,WM_LAUNCHMIRANDA,0,0);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					SendMessage(hdlg,WM_LAUNCHMIRANDA,0,0);
					SendMessage(GetParent(hdlg),WZM_GOTOPAGE,IDD_SELECTDB,(LPARAM)SelectDbDlgProc);
					break;
			}
			break;
	}
	return FALSE;
}