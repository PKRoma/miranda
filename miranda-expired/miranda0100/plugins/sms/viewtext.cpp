/*
Miranda SMS Plugin
Copyright (C) 2001  Richard Hughes

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
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

void GetSmsStartTime(int i,FILETIME *ft);
char *GetSmsText(int i,int type);

char *PrettyPrintXML(const char *xml)
{
	char *str;
	int iStrAlloced;
	int iXml,iStr,i;
	int indent=0;

	iStrAlloced=256;
	str=(char*)malloc(iStrAlloced);
	for(iXml=0,iStr=0;xml[iXml];iXml++) {
		if(iStr+indent*2+4>iStrAlloced) {
			iStrAlloced+=256;
			str=(char*)realloc(str,iStrAlloced);
		}
		if(xml[iXml]=='<') {
			if(xml[iXml+1]=='/') indent--;
			if(iXml) {
				str[iStr++]='\r';
				str[iStr++]='\n';
			}
			for(i=0;i<indent*2;i++) str[iStr++]=' ';
			if(xml[iXml+1]!='/') indent++;
		}
		str[iStr++]=xml[iXml];
		if(xml[iXml]=='>' && xml[iXml+1]!='<') {
			str[iStr++]='\r';
			str[iStr++]='\n';
			for(i=0;i<indent*2;i++) str[iStr++]=' ';
		}
	}
	str[iStr]=0;
	return str;
}

BOOL CALLBACK ViewTextDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
			{	int i=LOWORD(lParam);
				int cmd=HIWORD(lParam);
				char *text,*number;
				char descr[256];

				{	char strTime[64];
					char strFormat[128],str[256];
					FILETIME ftStartTime;
					SYSTEMTIME stStartTime;
					GetSmsStartTime(i,&ftStartTime);
					FileTimeToSystemTime(&ftStartTime,&stStartTime);
					GetTimeFormat(LOCALE_USER_DEFAULT,0,&stStartTime,NULL,strTime,sizeof(strTime));
					GetDlgItemText(hwndDlg,IDC_MSGTIME,strFormat,sizeof(strFormat));
					wsprintf(str,strFormat,strTime);
					SetDlgItemText(hwndDlg,IDC_MSGTIME,str);
				}
				number=GetSmsText(i,0);
				switch(cmd) {
					case IDM_VIEWMSG:
						text=_strdup(GetSmsText(i,1));
						wsprintf(descr,"Message to %s:",number);
						break;
					case IDM_VIEWACK:
						text=PrettyPrintXML(GetSmsText(i,2));
						wsprintf(descr,"Ack of message to %s:",number);
						break;
					case IDM_VIEWRCPT:
						text=PrettyPrintXML(GetSmsText(i,3));
						wsprintf(descr,"Receipt of message to %s:",number);
						break;
				}
				SetDlgItemText(hwndDlg,IDC_MSGTYPE,descr);
				SetDlgItemText(hwndDlg,IDC_TEXT,text);
				free(text);
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hwndDlg);
			break;
	}
	return FALSE;
}