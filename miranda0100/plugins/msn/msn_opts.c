/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "msn_global.h"
#include "../../miranda32/ui/options/m_options.h"
#include "resource.h"

static BOOL CALLBACK DlgProcMsnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern volatile LONG msnLoggedIn;
extern HINSTANCE hInst;

int MsnOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-790000000;
	odp.hInstance=hInst;
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_MSN);
	odp.pszTitle="MSN Messenger";
	odp.pfnDlgProc=DlgProcMsnOpts;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

static BOOL CALLBACK DlgProcMsnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{	DBVARIANT dbv;

			if(!DBGetContactSetting(NULL,MSNPROTONAME,"e-mail",&dbv)) {
				SetDlgItemText(hwndDlg,IDC_HANDLE,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if(!DBGetContactSetting(NULL,MSNPROTONAME,"Password",&dbv)) {
				//bit of a security hole here, since it's easy to extract a password from an edit box
				CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
				SetDlgItemText(hwndDlg,IDC_PASSWORD,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if(!DBGetContactSetting(NULL,MSNPROTONAME,"LoginServer",&dbv)) {
				SetDlgItemText(hwndDlg,IDC_LOGINSERVER,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else SetDlgItemText(hwndDlg,IDC_LOGINSERVER,MSN_DEFAULT_LOGIN_SERVER);
			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_APPLY:
				{	int reconnectRequired=0;
					char str[128];
					DBVARIANT dbv;

					GetDlgItemText(hwndDlg,IDC_HANDLE,str,sizeof(str));
					dbv.pszVal=NULL;
					if(DBGetContactSetting(NULL,MSNPROTONAME,"e-mail",&dbv) || strcmp(str,dbv.pszVal))
						reconnectRequired=1;
					if(dbv.pszVal!=NULL) DBFreeVariant(&dbv);
					DBWriteContactSettingString(NULL,MSNPROTONAME,"e-mail",str);

					GetDlgItemText(hwndDlg,IDC_PASSWORD,str,sizeof(str));
					CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(str),(LPARAM)str);
					dbv.pszVal=NULL;
					if(DBGetContactSetting(NULL,MSNPROTONAME,"Password",&dbv) || strcmp(str,dbv.pszVal))
						reconnectRequired=1;
					if(dbv.pszVal!=NULL) DBFreeVariant(&dbv);
					DBWriteContactSettingString(NULL,MSNPROTONAME,"Password",str);

					GetDlgItemText(hwndDlg,IDC_LOGINSERVER,str,sizeof(str));
					DBWriteContactSettingString(NULL,MSNPROTONAME,"LoginServer",str);
					if(reconnectRequired && msnLoggedIn) MessageBox(hwndDlg,"The changes you have made require you to reconnect to the MSN Messenger network before they take effect","MSN Options",MB_OK);
					return TRUE;
				}
			}
			break;
	}
	return FALSE;
}