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
#include "resource.h"
#include "../../miranda32/core/m_system.h"
#include "../../miranda32/protocols/protocols/protomod.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"MSN Protocol",
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"Adds support for communicating with users of the MSN Messenger network",
	"Richard Hughes",
	"miranda@rhughes.net",
	"© 2001 Richard Hughes",
	"http://miranda-icq.sourceforge.net/",
	0,
	DEFMOD_PROTOCOLMSN
};

int LoadMsnServices(void);
SOCKET msnSock;
int msnLoggedIn;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

int __declspec(dllexport) Unload(void)
{
	if(msnLoggedIn) MSN_Logout();
	MSN_WS_CleanUp();
	return 0;
}

static VOID CALLBACK MsnCheckDataTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	MSN_main();
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	PROTOCOLDESCRIPTOR pd;

	ZeroMemory(&pd,sizeof(pd));
	pd.cbSize=sizeof(pd);
	pd.szName=MSNPROTONAME;
	pd.type=PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

	//set all contacts to 'offline'
	{	HANDLE hContact;
		DBVARIANT dbv;
		DWORD uin;
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
		while(hContact!=NULL) {
			if(!lstrcmp(MSNPROTONAME,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0)))
				DBWriteContactSettingWord(hContact,MSNPROTONAME,"Status",ID_STATUS_OFFLINE);
				//icq_ContactAdd(plink,uin);
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		}
	}

	msnSock=0;
	msnLoggedIn=0;
	MSN_WS_Init();
	LoadMsnServices();
	SetTimer(NULL,0,250,MsnCheckDataTimer);
	return 0;
}

BOOL CALLBACK DlgProcMSNOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{/*
	BOOL oldstate;
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				//hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				//hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{		
			SendDlgItemMessage(hwndDlg, IDC_USEMSN, BM_SETCHECK, (opts.MSN.enabled) ? BST_CHECKED : BST_UNCHECKED, 0);		
			SetDlgItemText(hwndDlg, IDC_UHANDLE, opts.MSN.uhandle);
			SetDlgItemText(hwndDlg, IDC_PASSWORD, opts.MSN.password);
			
			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
						oldstate=opts.MSN.enabled;
						GetDlgItemText(hwndDlg, IDC_UHANDLE, opts.MSN.uhandle, MSN_UHANDLE_LEN);
						GetDlgItemText(hwndDlg, IDC_PASSWORD, opts.MSN.password, MSN_PASSWORD_LEN);
					
						opts.MSN.enabled=SendDlgItemMessage(hwndDlg, IDC_USEMSN, BM_GETCHECK, 0, 0);
						
						if (oldstate!=opts.MSN.enabled)
							MessageBox(hwndDlg,"You will have to restart Miranda for your MSN changes to take affect.",MIRANDANAME,MB_OK);

					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}*/
	return FALSE;
}
