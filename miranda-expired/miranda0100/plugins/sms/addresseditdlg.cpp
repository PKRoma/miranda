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
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"

#define DM_ADDRESSEDITCLOSED  (WM_USER+100)

int GetBookEntry(int n,char *strNum,int cbStrNum,char *strName,int cbStrName)
{
	char setting[10];
	DBVARIANT dbv;

	wsprintf(setting,"%d#",n);
	if(DBGetContactSetting(NULL,"SMSBook",setting,&dbv)) return 0;
	lstrcpyn(strNum,dbv.pszVal,cbStrNum);
	DBFreeVariant(&dbv);
	wsprintf(setting,"%dn",n);
	if(!DBGetContactSetting(NULL,"SMSBook",setting,&dbv)) {
		lstrcpyn(strName,dbv.pszVal,cbStrName);
		DBFreeVariant(&dbv);
	}
	else if(cbStrName) *strName='\0';
	return 1;
}

static WNDPROC OldEditWndProc;
static LRESULT CALLBACK EditSubclassProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_SETFOCUS:
			SendMessage(GetParent(hwnd),DM_SETDEFID,IDC_ADD,0);
			InvalidateRect(GetDlgItem(GetParent(hwnd),IDOK),NULL,FALSE);
			InvalidateRect(GetDlgItem(GetParent(hwnd),IDC_ADD),NULL,FALSE);
			break;
		case WM_KILLFOCUS:
			SendMessage(GetParent(hwnd),DM_SETDEFID,IDOK,0);
			InvalidateRect(GetDlgItem(GetParent(hwnd),IDOK),NULL,FALSE);
			InvalidateRect(GetDlgItem(GetParent(hwnd),IDC_ADD),NULL,FALSE);
			break;
	}
	return CallWindowProc(OldEditWndProc,hwnd,message,wParam,lParam);
}

static int CALLBACK ListCompareFunc(int i1,int i2,LPARAM lParamSort)
{
	char name1[256],name2[256];
	ListView_GetItemText((HWND)lParamSort,i1,1,name1,sizeof(name1));
	ListView_GetItemText((HWND)lParamSort,i2,1,name2,sizeof(name2));
	return lstrcmpi(name1,name2);
}

BOOL CALLBACK AddressEditDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
			ListView_SetExtendedListViewStyleEx(GetDlgItem(hwndDlg,IDC_LIST),LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
			{	RECT rc;
				LVCOLUMN lvc;
				GetClientRect(GetDlgItem(hwndDlg,IDC_LIST),&rc);
				rc.right-=GetSystemMetrics(SM_CXVSCROLL);
				lvc.mask=LVCF_WIDTH|LVCF_TEXT;
				lvc.cx=rc.right/3;
				lvc.pszText="Phone#";
				ListView_InsertColumn(GetDlgItem(hwndDlg,IDC_LIST),0,&lvc);
				lvc.cx=rc.right-lvc.cx;
				lvc.pszText="Name";
				ListView_InsertColumn(GetDlgItem(hwndDlg,IDC_LIST),1,&lvc);
			}
			{	LVITEM lvi;
				char number[64],name[256];
				int i;

				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=0;
				for(lvi.iItem=0;;lvi.iItem++) {
					if(!GetBookEntry(lvi.iItem,number,sizeof(number),name,sizeof(name))) break;
					lvi.pszText=number;
					i=ListView_InsertItem(GetDlgItem(hwndDlg,IDC_LIST),&lvi);
					ListView_SetItemText(GetDlgItem(hwndDlg,IDC_LIST),i,1,name);
				}
				ListView_SortItemsEx(GetDlgItem(hwndDlg,IDC_LIST),ListCompareFunc,GetDlgItem(hwndDlg,IDC_LIST));
			}
			OldEditWndProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_NUMBER),GWL_WNDPROC,(LONG)EditSubclassProc);
			OldEditWndProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_NAME),GWL_WNDPROC,(LONG)EditSubclassProc);
			SetDlgItemText(hwndDlg,IDC_NUMBER,(char*)lParam);
			SendDlgItemMessage(hwndDlg,IDC_NUMBER,EM_LIMITTEXT,63,0);
			SendDlgItemMessage(hwndDlg,IDC_NAME,EM_LIMITTEXT,255,0);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					{	int i;
						char setting[10];
						char str[256];
						i=ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_LIST));
						wsprintf(setting,"%d#",i);
						DBDeleteContactSetting(NULL,"SMSBook",setting);
						for(i--;i>=0;i--) {
							ListView_GetItemText(GetDlgItem(hwndDlg,IDC_LIST),i,0,str,sizeof(str));
							wsprintf(setting,"%d#",i);
							DBWriteContactSettingString(NULL,"SMSBook",setting,str);
							ListView_GetItemText(GetDlgItem(hwndDlg,IDC_LIST),i,1,str,sizeof(str));
							wsprintf(setting,"%dn",i);
							if(str[0]) DBWriteContactSettingString(NULL,"SMSBook",setting,str);
							else DBDeleteContactSetting(NULL,"SMSBook",setting);
						}
					}
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
				case IDC_NUMBER:
					if(HIWORD(wParam)==EN_CHANGE) {
						LVFINDINFO lvfi;
						char str[64];

						GetDlgItemText(hwndDlg,IDC_NUMBER,str,sizeof(str));
						lvfi.flags=LVFI_STRING;
						lvfi.psz=str;
						SetDlgItemText(hwndDlg,IDC_ADD,ListView_FindItem(GetDlgItem(hwndDlg,IDC_LIST),-1,&lvfi)==-1?"&Add":"Upd&ate");
						EnableWindow(GetDlgItem(hwndDlg,IDC_ADD),GetWindowTextLength(GetDlgItem(hwndDlg,IDC_NUMBER)));
					}
					break;
				case IDC_ADD:
					if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_ADD))) break;
					{	LVFINDINFO lvfi;
						char number[64],name[256];
						int i;

						GetDlgItemText(hwndDlg,IDC_NUMBER,number,sizeof(number));
						GetDlgItemText(hwndDlg,IDC_NAME,name,sizeof(name));
						if(number[0]!='+' || number[1]<'1' || number[1]>'9' || number[2]=='\0') {
							MessageBox(hwndDlg,"Valid phone numbers are of the form \"+(country code)(phone number)\". The contents of the phone number portion is dependent on the national layout of phone numbers, but often omits the leading zero.","Invalid phone number",MB_OK);
							SetFocus(GetDlgItem(hwndDlg,IDC_NUMBER));
							SendDlgItemMessage(hwndDlg,IDC_NUMBER,EM_SETSEL,0,-1);
							break;
						}
						lvfi.flags=LVFI_STRING;
						lvfi.psz=number;
						i=ListView_FindItem(GetDlgItem(hwndDlg,IDC_LIST),-1,&lvfi);
						if(i==-1) {
							LVITEM lvi;
							lvi.mask=LVIF_TEXT;
							lvi.iItem=0;
							lvi.iSubItem=0;
							lvi.pszText=number;
							i=ListView_InsertItem(GetDlgItem(hwndDlg,IDC_LIST),&lvi);
						}
						ListView_SetItemText(GetDlgItem(hwndDlg,IDC_LIST),i,1,name);
						ListView_SetItemState(GetDlgItem(hwndDlg,IDC_LIST),i,LVIS_SELECTED,LVIS_SELECTED);
						ListView_SortItemsEx(GetDlgItem(hwndDlg,IDC_LIST),ListCompareFunc,GetDlgItem(hwndDlg,IDC_LIST));
						SetFocus(GetDlgItem(hwndDlg,IDC_NUMBER));
						SendDlgItemMessage(hwndDlg,IDC_NUMBER,EM_SETSEL,0,-1);
					}
					break;
				case IDC_DELETE:
					{	int i;
						i=ListView_GetNextItem(GetDlgItem(hwndDlg,IDC_LIST),-1,LVNI_ALL|LVNI_SELECTED);
						if(i!=-1) {
							ListView_DeleteItem(GetDlgItem(hwndDlg,IDC_LIST),i);
							SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_NUMBER,EN_CHANGE),0);
						}
					}
					break;
			}
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case IDC_LIST:
					switch(((LPNMHDR)lParam)->code) {
						case LVN_ITEMCHANGED:
							{	LPNMLISTVIEW nmlv=(LPNMLISTVIEW)lParam;
								if(nmlv->uNewState&LVIS_SELECTED) {
									char str[256];
									ListView_GetItemText(GetDlgItem(hwndDlg,IDC_LIST),nmlv->iItem,0,str,sizeof(str));
									SetDlgItemText(hwndDlg,IDC_NUMBER,str);
									ListView_GetItemText(GetDlgItem(hwndDlg,IDC_LIST),nmlv->iItem,1,str,sizeof(str));
									SetDlgItemText(hwndDlg,IDC_NAME,str);
								}
								EnableWindow(GetDlgItem(hwndDlg,IDC_DELETE),-1!=ListView_GetNextItem(GetDlgItem(hwndDlg,IDC_LIST),-1,LVNI_ALL|LVNI_SELECTED));
							}
							break;
					}
					break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hwndDlg);
			break;
		case WM_DESTROY:
			PostMessage(GetParent(hwndDlg),DM_ADDRESSEDITCLOSED,0,0);
			break;
	}
	return FALSE;
}