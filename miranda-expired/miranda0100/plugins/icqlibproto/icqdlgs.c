/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "../../icqlib/icq.h"
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/options/m_options.h"
#include "../../miranda32/random/langpack/m_langpack.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "icqproto.h"

static BOOL CALLBACK DlgProcIcqOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcIcqMsgsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcIcqNetOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern HINSTANCE hInst;

int IcqOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-800000000;
	odp.hInstance=hInst;
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_ICQ);
	odp.pszTitle="ICQ";
	odp.pfnDlgProc=DlgProcIcqOpts;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.groupPosition=odp.position;
	odp.pszGroup="ICQ";
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_ICQMSGS);
	odp.pszTitle=Translate("Messaging");
	odp.pfnDlgProc=DlgProcIcqMsgsOpts;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_ICQNETWORK);
	odp.pszTitle=Translate("Network");
	odp.pfnDlgProc=DlgProcIcqNetOpts;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

static BOOL CALLBACK DlgProcIcqOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{	DBVARIANT dbv;

			TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_USEICQ, DBGetContactSettingByte(NULL,ICQPROTONAME,"Enable",1));

			SetDlgItemInt(hwndDlg,IDC_ICQNUM,DBGetContactSettingDword(NULL,ICQPROTONAME,"UIN",0),FALSE);
			if(!DBGetContactSetting(NULL,ICQPROTONAME,"Password",&dbv)) {
				//bit of a security hole here, since it's easy to extract a password from an edit box
				CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
				SetDlgItemText(hwndDlg,IDC_PASSWORD,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			ShowWindow(GetDlgItem(hwndDlg,IDC_RELOADREQD),SW_HIDE);
			return TRUE;
		}
		case WM_COMMAND:
			{	char szClass[80];
				GetClassName((HWND)lParam,szClass,sizeof(szClass));
				if(lstrcmpi(szClass,"EDIT") || HIWORD(wParam)==EN_CHANGE)
					ShowWindow(GetDlgItem(hwndDlg,IDC_RELOADREQD),SW_SHOW);
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_APPLY:
				{	char str[128];

					DBWriteContactSettingByte(NULL,ICQPROTONAME,"Enable",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USEICQ));
					DBWriteContactSettingDword(NULL,ICQPROTONAME,"UIN",(DWORD)GetDlgItemInt(hwndDlg,IDC_ICQNUM,NULL,FALSE));
					GetDlgItemText(hwndDlg,IDC_PASSWORD,str,sizeof(str));
					CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(str),(LPARAM)str);
					DBWriteContactSettingString(NULL,ICQPROTONAME,"Password",str);
					return TRUE;
				}
			}
			break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcIcqMsgsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{	int sts;

			TranslateDialogDefault(hwndDlg);
			sts=DBGetContactSettingByte(NULL,ICQPROTONAME,"SendThruServer",ICQ_SEND_BESTWAY);
			if(sts==ICQ_SEND_BESTWAY) CheckDlgButton(hwndDlg,IDC_BESTWAY,BST_CHECKED);
			if(sts==ICQ_SEND_THRUSERVER) CheckDlgButton(hwndDlg,IDC_THRUSERVER,BST_CHECKED);
			if(sts==ICQ_SEND_DIRECT) CheckDlgButton(hwndDlg,IDC_DIRECT,BST_CHECKED);
			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_APPLY:
					if(IsDlgButtonChecked(hwndDlg,IDC_BESTWAY)) DBWriteContactSettingByte(NULL,ICQPROTONAME,"SendThruServer",(BYTE)ICQ_SEND_BESTWAY);
					else if(IsDlgButtonChecked(hwndDlg,IDC_DIRECT)) DBWriteContactSettingByte(NULL,ICQPROTONAME,"SendThruServer",(BYTE)ICQ_SEND_DIRECT);
					else if(IsDlgButtonChecked(hwndDlg,IDC_THRUSERVER)) DBWriteContactSettingByte(NULL,ICQPROTONAME,"SendThruServer",(BYTE)ICQ_SEND_THRUSERVER);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

#define DM_REDOCHECKBOXES    (WM_USER+1)
static BOOL CALLBACK DlgProcIcqNetOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{	DBVARIANT dbv;

			TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_PROXYAUTH, DBGetContactSettingByte(NULL,ICQPROTONAME,"ProxyAuth",0));
			CheckDlgButton(hwndDlg, IDC_USEPROXY, DBGetContactSettingByte(NULL,ICQPROTONAME,"UseProxy",0));
			SendMessage(hwndDlg,DM_REDOCHECKBOXES,0,0);

			if(!DBGetContactSetting(NULL,ICQPROTONAME,"Server",&dbv)) {
				SetDlgItemText(hwndDlg,IDC_ICQSERVER,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else SetDlgItemText(hwndDlg,IDC_ICQSERVER,"icq.mirabilis.com");
			SetDlgItemInt(hwndDlg,IDC_ICQPORT,DBGetContactSettingWord(NULL,ICQPROTONAME,"Port",4000),FALSE);
			if(!DBGetContactSetting(NULL,ICQPROTONAME,"ProxyHost",&dbv)) {
				SetDlgItemText(hwndDlg,IDC_PROXYHOST,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			SetDlgItemInt(hwndDlg,IDC_PROXYPORT,DBGetContactSettingWord(NULL,ICQPROTONAME,"ProxyPort",1080),FALSE);
			if(!DBGetContactSetting(NULL,ICQPROTONAME,"ProxyUser",&dbv)) {
				SetDlgItemText(hwndDlg,IDC_PROXYUSER,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if(!DBGetContactSetting(NULL,ICQPROTONAME,"ProxyPassword",&dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
				SetDlgItemText(hwndDlg,IDC_PROXYPASS,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			ShowWindow(GetDlgItem(hwndDlg,IDC_RECONNECTREQD),SW_HIDE);
			return TRUE;
		}
		case DM_REDOCHECKBOXES:
#define en(a,b)  EnableWindow(GetDlgItem(hwndDlg,IDC_##a),b)
		{	int on;
			on=IsDlgButtonChecked(hwndDlg,IDC_USEPROXY);
			en(PROXYHOST,on); en(PROXYPORT,on); en(STATIC21,on); en(STATIC22,on);
			en(PROXYAUTH,on);
			if(on) on=IsDlgButtonChecked(hwndDlg,IDC_PROXYAUTH);
			en(STATIC31,on); en(STATIC32,on); en(PROXYUSER,on); en(PROXYPASS,on);
			break;
		}
#undef en
		case WM_COMMAND:
			if(icqIsOnline) {
				char szClass[80];
				GetClassName((HWND)lParam,szClass,sizeof(szClass));
				if(lstrcmpi(szClass,"EDIT") || HIWORD(wParam)==EN_CHANGE)
					ShowWindow(GetDlgItem(hwndDlg,IDC_RECONNECTREQD),SW_SHOW);
			}
			switch(LOWORD(wParam)) {
				case IDC_USEPROXY:
				case IDC_PROXYAUTH:
					SendMessage(hwndDlg,DM_REDOCHECKBOXES,0,0);
					break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_APPLY:
				{	char str[128];

					GetDlgItemText(hwndDlg,IDC_ICQSERVER,str,sizeof(str));
					DBWriteContactSettingString(NULL,ICQPROTONAME,"Server",str);
					DBWriteContactSettingWord(NULL,ICQPROTONAME,"Port",(WORD)GetDlgItemInt(hwndDlg,IDC_ICQPORT,NULL,FALSE));

					DBWriteContactSettingByte(NULL,ICQPROTONAME,"UseProxy",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USEPROXY));
					GetDlgItemText(hwndDlg,IDC_PROXYHOST,str,sizeof(str));
					DBWriteContactSettingString(NULL,ICQPROTONAME,"ProxyHost",str);
					DBWriteContactSettingWord(NULL,ICQPROTONAME,"ProxyPort",(WORD)GetDlgItemInt(hwndDlg,IDC_PROXYPORT,NULL,FALSE));

					DBWriteContactSettingByte(NULL,ICQPROTONAME,"ProxyAuth",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_PROXYAUTH));
					GetDlgItemText(hwndDlg,IDC_PROXYUSER,str,sizeof(str));
					DBWriteContactSettingString(NULL,ICQPROTONAME,"ProxyUser",str);
					GetDlgItemText(hwndDlg,IDC_PROXYPASS,str,sizeof(str));
					CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(str),(LPARAM)str);
					DBWriteContactSettingString(NULL,ICQPROTONAME,"ProxyPassword",str);

					if(plink!=NULL) {
						icqUsingProxy=DBGetContactSettingByte(NULL,ICQPROTONAME,"UseProxy",0);
						if(icqUsingProxy) {
							char *host,*user,*password;
							WORD port;
							BYTE useAuth;
							DBVARIANT dbvHost={0},dbvUser={0},dbvPass={0};

							if(DBGetContactSetting(NULL,ICQPROTONAME,"ProxyHost",&dbvHost)) {host=NULL; icqUsingProxy=0;}
							else host=dbvHost.pszVal;
							port=DBGetContactSettingWord(NULL,ICQPROTONAME,"ProxyPort",1080);
							useAuth=DBGetContactSettingByte(NULL,ICQPROTONAME,"ProxyAuth",0);
							if(useAuth) {
								if(DBGetContactSetting(NULL,ICQPROTONAME,"ProxyUser",&dbvUser)) {user=""; useAuth=0;}
								else user=dbvUser.pszVal;
								if(DBGetContactSetting(NULL,ICQPROTONAME,"ProxyPassword",&dbvPass)) {password=""; useAuth=0;}
								else {
									CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbvPass.pszVal)+1,(LPARAM)dbvPass.pszVal);
									password=dbvPass.pszVal;
								}
							}
							else {
								user="";
								password="";
							}
							if(icqUsingProxy) icq_SetProxy(plink,host,port,useAuth,user,password);
							DBFreeVariant(&dbvHost);
							DBFreeVariant(&dbvUser);
							DBFreeVariant(&dbvPass);
						}
						if(!icqUsingProxy) icq_UnsetProxy(plink);
					}
					return TRUE;
				}
			}
			break;
	}
	return FALSE;
}
