/*
Miranda SMS Plugin
Copyright (C) 2001-2  Richard Hughes

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
#include <stdio.h>
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

BOOL CALLBACK AddressEditDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ViewTextDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam);
int GetBookEntry(int n,char *strNum,int cbStrNum,char *strName,int cbStrName);
void StartSmsSend(const char *number,const char *text);
int IsSmsAcked(int i);
int IsSmsRcpted(int i);

extern HINSTANCE hInst;
static int nFullDlgHeight,nShortDlgHeight;
static WNDPROC OldEditWndProc;
HWND hwndStatus;

static LRESULT CALLBACK MessageSubclassProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_CHAR:
			if(wParam=='\n' && GetKeyState(VK_CONTROL)&0x8000) {
				PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
				return 0;
			}
			break;
	}
	return CallWindowProc(OldEditWndProc,hwnd,message,wParam,lParam);
}

#define DM_RELOADADDRESS   (WM_USER+10)
#define DM_ADDRESSCHANGED  (WM_USER+11)
#define DM_ADDRESSEDITCLOSED  (WM_USER+100)
BOOL CALLBACK SendSmsDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(hInst,MAKEINTRESOURCE(IDI_SMS)));
			{	RECT rcDlg,rc;
				GetWindowRect(hwndDlg,&rcDlg);
				GetWindowRect(GetDlgItem(hwndDlg,IDC_STATUS),&rc);
				nFullDlgHeight=rcDlg.bottom-rcDlg.top;
				nShortDlgHeight=rc.top-rcDlg.top;
				SetWindowPos(hwndDlg,0,0,0,rcDlg.right-rcDlg.left,nShortDlgHeight,SWP_NOZORDER|SWP_NOMOVE);
			}
			hwndStatus=GetDlgItem(hwndDlg,IDC_STATUS);
			{	RECT rc;
				LVCOLUMN lvc;
				GetClientRect(hwndStatus,&rc);
				rc.right-=GetSystemMetrics(SM_CXVSCROLL);
				lvc.mask=LVCF_WIDTH;
				lvc.cx=rc.right/3;
				ListView_InsertColumn(hwndStatus,0,&lvc);
				lvc.cx=rc.right-lvc.cx;
				ListView_InsertColumn(hwndStatus,1,&lvc);
				ListView_SetExtendedListViewStyleEx(hwndStatus,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
			}
			SendMessage(hwndDlg,DM_RELOADADDRESS,0,0);
			OldEditWndProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_MESSAGE),GWL_WNDPROC,(LONG)MessageSubclassProc);
			{	char str[256];
				int len;

				len=_snprintf(str,sizeof(str),"From %s on ICQ:\r\n\r\n",(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)(HANDLE)NULL,0));
				SetDlgItemText(hwndDlg,IDC_MESSAGE,str);
				SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETSEL,len,len);
				SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_MESSAGE,EN_CHANGE),0);
			}
			return TRUE;
		case DM_RELOADADDRESS:
			SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_RESETCONTENT,0,0);
			{	char number[64],name[256],str[256];
				int i;

				for(i=0;;i++) {
					if(!GetBookEntry(i,number,sizeof(number),name,sizeof(name))) break;
					_snprintf(str,sizeof(str),"%s\t%s",number,name);
					SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_ADDSTRING,0,(LPARAM)str);
				}
			}
			{	HANDLE hContact;
				DBVARIANT dbv;
				char str[256];

				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
				while(hContact!=NULL) {
					if(!DBGetContactSetting(hContact,"ICQ","Cellular",&dbv)) {
						if(strlen(dbv.pszVal)>4 && !strcmp(dbv.pszVal+strlen(dbv.pszVal)-4," SMS")) {
							strcpy(str,dbv.pszVal);
							*strchr(str,' ')=0;
							strcat(str,"\t");
							strcat(str,(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0));
							SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_ADDSTRING,0,(LPARAM)str);
						}
						DBFreeVariant(&dbv);
					}
					hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
				}
			}
			break;
		case DM_ADDRESSEDITCLOSED:
			EnableWindow(hwndDlg,TRUE);
			SetForegroundWindow(hwndDlg);
			SetFocus(hwndDlg);
			SendMessage(hwndDlg,DM_RELOADADDRESS,0,0);
			break;
		case WM_MEASUREITEM:
			{	LPMEASUREITEMSTRUCT mis=(LPMEASUREITEMSTRUCT)lParam;
				SIZE size;
				HFONT hFont,hoFont;
				HDC hdc1;

				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_ADDRESS,WM_GETFONT,0,0);
				hdc1=GetDC(hwndDlg);
				hoFont=(HFONT)SelectObject(hdc1,hFont);
				GetTextExtentPoint32(hdc1,"x",1,&size);
				SelectObject(hdc1,hoFont);
				ReleaseDC(hwndDlg,hdc1);
				mis->itemHeight=size.cy;
				mis->itemWidth=size.cx;
			}
			break;
		case WM_DRAWITEM:
			{	LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
				char str[256],*name;
				if(dis->itemState&ODS_SELECTED) {
					SetTextColor(dis->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
					SetBkColor(dis->hDC,GetSysColor(COLOR_HIGHLIGHT));
				}
				else {
					SetTextColor(dis->hDC,GetSysColor(COLOR_WINDOWTEXT));
					SetBkColor(dis->hDC,GetSysColor(COLOR_WINDOW));
				}
				if(dis->itemID!=-1) {
					SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_GETLBTEXT,dis->itemID,(LPARAM)str);
					name=strchr(str,'\t');
					if(name!=NULL) {*name='\0'; name++;}
					else name=str+strlen(str);
					ExtTextOut(dis->hDC,dis->rcItem.left,dis->rcItem.top,ETO_CLIPPED|ETO_OPAQUE,&dis->rcItem,str,strlen(str),NULL);
					dis->rcItem.left+=100;
					ExtTextOut(dis->hDC,dis->rcItem.left,dis->rcItem.top,ETO_CLIPPED|ETO_OPAQUE,&dis->rcItem,name,strlen(name),NULL);
				}
				if(dis->itemState&ODS_FOCUS)
					DrawFocusRect(dis->hDC,&dis->rcItem);
			}
			break;
		case DM_ADDRESSCHANGED:
			{	char str[256],*str2;
				SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_GETLBTEXT,SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_GETCURSEL,0,0),(LPARAM)str);
				str2=strchr(str,'\t');
				if(str2!=NULL) {
					*str2='\0';
					SetDlgItemText(hwndDlg,IDC_ADDRESS,str);
					SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_SETEDITSEL,0,MAKELPARAM(0,-1));
				}
			}
			break;
		case WM_CONTEXTMENU:
			if((HWND)wParam!=hwndStatus) break;
			{	LVHITTESTINFO lvhti;
				LVITEM lvi;
				int cmd;
				HMENU hMenu;

				lvhti.pt.x=(short)LOWORD(lParam);
				lvhti.pt.y=(short)HIWORD(lParam);
				ScreenToClient(hwndStatus,&lvhti.pt);
				lvi.iItem=ListView_HitTest(hwndStatus,&lvhti);
				if(lvi.iItem==-1) break;
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				ListView_GetItem(hwndStatus,&lvi);
				hMenu=GetSubMenu(LoadMenu(hInst,MAKEINTRESOURCE(IDR_CONTEXT)),0);
				EnableMenuItem(hMenu,IDM_VIEWACK,IsSmsAcked(lvi.lParam)?MF_ENABLED:MF_GRAYED);
				EnableMenuItem(hMenu,IDM_VIEWRCPT,IsSmsRcpted(lvi.lParam)?MF_ENABLED:MF_GRAYED);
				cmd=TrackPopupMenu(hMenu,TPM_RIGHTBUTTON|TPM_RETURNCMD,(short)LOWORD(lParam),(short)HIWORD(lParam),0,hwndDlg,NULL);
				DestroyMenu(hMenu);
				if(cmd) CreateDialogParam(hInst,MAKEINTRESOURCE(IDD_VIEWTEXT),hwndDlg,ViewTextDlgProc,MAKELPARAM(lvi.lParam,cmd));
				return 0;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					{	char number[64];
						char text[161];
						GetDlgItemText(hwndDlg,IDC_ADDRESS,number,sizeof(number));
						GetDlgItemText(hwndDlg,IDC_MESSAGE,text,sizeof(text));
						if(number[0]!='+' || number[1]<'1' || number[1]>'9' || number[2]=='\0') {
							MessageBox(hwndDlg,"Valid phone numbers are of the form \"+(country code)(phone number)\". The contents of the phone number portion is dependent on the national layout of phone numbers, but often omits the leading zero.","Invalid phone number",MB_OK);
							SetFocus(GetDlgItem(hwndDlg,IDC_ADDRESS));
							SendDlgItemMessage(hwndDlg,IDC_ADDRESS,CB_SETEDITSEL,0,MAKELPARAM(0,-1));
							break;
						}
						StartSmsSend(number,text);
					}
					{	RECT rcDlg;
						GetWindowRect(hwndDlg,&rcDlg);
						SetWindowPos(hwndDlg,0,0,0,rcDlg.right-rcDlg.left,nFullDlgHeight,SWP_NOZORDER|SWP_NOMOVE);
					}
					break;
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
				case IDC_ADDRESS:
					PostMessage(hwndDlg,DM_ADDRESSCHANGED,0,0);
					break;
				case IDC_MESSAGE:
					if(HIWORD(wParam)==EN_CHANGE) {
						char str[80];
						int count=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
						wsprintf(str,"%d/160 chars",count);
						SetDlgItemText(hwndDlg,IDC_COUNT,str);
						EnableWindow(GetDlgItem(hwndDlg,IDOK),count);
					}
					break;
				case IDC_EDIT:
					{	char str[256];
						GetDlgItemText(hwndDlg,IDC_ADDRESS,str,sizeof(str));
						EnableWindow(hwndDlg,FALSE);
						CreateDialogParam(hInst,MAKEINTRESOURCE(IDD_ADDRESSEDIT),hwndDlg,AddressEditDlgProc,(LPARAM)str);
					}
					break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hwndDlg);
			break;
	}
	return FALSE;
}