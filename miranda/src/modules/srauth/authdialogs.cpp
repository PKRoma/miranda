/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project, 
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

INT_PTR CALLBACK DlgProcAdded(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			DBEVENTINFO dbei;
			DWORD *uin;
			char *nick,*first,*last,*email;
			HANDLE hDbEvent,hcontact;

			TranslateDialogDefault(hwndDlg);
			Window_SetIcon_IcoLib(hwndDlg, SKINICON_OTHER_MIRANDA);
			Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, "View User's Details");
			Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, "Add Contact Permanently to List");
			hDbEvent=(HANDLE)lParam;
			//blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ)
			ZeroMemory(&dbei,sizeof(dbei));
			dbei.cbSize=sizeof(dbei);
			dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
			dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
			CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
			uin=(PDWORD)dbei.pBlob;
			hcontact=*((PHANDLE)(dbei.pBlob+sizeof(DWORD)));
			if ((hcontact == INVALID_HANDLE_VALUE) || !DBGetContactSettingByte(hcontact, "CList", "NotOnList", 0))
				ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
			nick=(char*)(dbei.pBlob+sizeof(DWORD)+sizeof(HANDLE));
			first=nick+strlen(nick)+1;
			last=first+strlen(first)+1;
			email=last+strlen(last)+1;
			if (*uin) 
				SetDlgItemInt(hwndDlg,IDC_NAME,*uin,FALSE);
			else {
                if (hcontact == INVALID_HANDLE_VALUE)
                    SetDlgItemText(hwndDlg,IDC_UIN,TranslateT("(Unknown)"));
                else {
                    CONTACTINFO ci;
                    char buf[128];
                    buf[0] = 0;
                    ZeroMemory(&ci, sizeof(ci));
                    ci.cbSize = sizeof(ci);
                    ci.hContact = hcontact;
                    ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hcontact, 0);
                    ci.dwFlag = CNF_UNIQUEID;
                    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
                        switch (ci.type) {
                            case CNFT_ASCIIZ:
                                mir_snprintf(buf, SIZEOF(buf), "%s", ci.pszVal);
                                mir_free(ci.pszVal);
                                break;
                            case CNFT_DWORD:
                                mir_snprintf(buf, SIZEOF(buf), "%u", ci.dVal);
                                break;
                        }
                    }
                    SetDlgItemTextA(hwndDlg,IDC_UIN,buf[0]?buf:Translate("(Unknown)"));
                }
            }
			SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
			SetWindowLongPtr(GetDlgItem(hwndDlg,IDC_DETAILS),GWLP_USERDATA,(LONG)hcontact);
			mir_free(dbei.pBlob);
			return TRUE;
		}
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT dis = ( LPDRAWITEMSTRUCT )lParam;
			if ( dis->hwndItem == GetDlgItem(hwndDlg, IDC_PROTOCOL)) {
				DBEVENTINFO dbei;
				char *szProto;
				HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);

				ZeroMemory(&dbei,sizeof(dbei));
				dbei.cbSize=sizeof(dbei);
				dbei.cbBlob=0;
				CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);

				szProto = dbei.szModule;
				if ( szProto ) {
					HICON hIcon = ( HICON )CallProtoService(szProto,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
					if ( hIcon ) {
						DrawIconEx(dis->hDC,dis->rcItem.left,dis->rcItem.top,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
						DestroyIcon( hIcon );
				}	}
				return TRUE;
			}
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_ADD:
			{	HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
				ADDCONTACTSTRUCT acs={0};

				acs.handle=hDbEvent;
				acs.handleType=HANDLE_EVENT;
				acs.szProto="";
				CallService(MS_ADDCONTACT_SHOW,(WPARAM)hwndDlg,(LPARAM)&acs);
                {
                    DBEVENTINFO dbei;
                    HANDLE hcontact;
                    
                    ZeroMemory(&dbei,sizeof(dbei));
                    dbei.cbSize=sizeof(dbei);
                    dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
                    dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
                    CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
                    hcontact=*((PHANDLE)(dbei.pBlob+sizeof(DWORD)));
                    mir_free(dbei.pBlob);
                    if ((hcontact == INVALID_HANDLE_VALUE) || !DBGetContactSettingByte(hcontact, "CList", "NotOnList", 0))
                        ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
                }
				return TRUE;
			}
			case IDC_DETAILS:
				CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)(HANDLE)GetWindowLongPtr((HWND)lParam,GWLP_USERDATA),0);
				return TRUE;
			case IDOK:
			{	HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
				ADDCONTACTSTRUCT acs={0};

				acs.handle=hDbEvent;
				acs.handleType=HANDLE_EVENT;
				acs.szProto="";
				CallService(MS_ADDCONTACT_SHOW,(WPARAM)hwndDlg,(LPARAM)&acs);
			}
				//fall through
			case IDCANCEL:
				DestroyWindow(hwndDlg);
				return TRUE;
		}
		break;
	case WM_DESTROY:
		Button_FreeIcon_IcoLib(hwndDlg,IDC_ADD);
		Button_FreeIcon_IcoLib(hwndDlg,IDC_DETAILS);
		Window_FreeIcon_IcoLib(hwndDlg);
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK DenyReasonProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static char szReason[256];
	switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
			SendDlgItemMessage(hwndDlg,IDC_REASON,EM_LIMITTEXT,(WPARAM)256,0);
			return TRUE;

		case WM_COMMAND:
			if(LOWORD(wParam)!=IDOK) break;
            {
				DBEVENTINFO dbei;

				ZeroMemory(&dbei,sizeof(dbei));
				dbei.cbSize=sizeof(dbei);
				dbei.cbBlob=0;
				CallService(MS_DB_EVENT_GET,(WPARAM)GetWindowLongPtr(hwndDlg,GWLP_USERDATA),(LPARAM)&dbei);
				GetDlgItemTextA(hwndDlg,IDC_REASON,szReason,256);
				CallProtoService(dbei.szModule,PS_AUTHDENY,(WPARAM)GetWindowLongPtr(hwndDlg,GWLP_USERDATA),(LPARAM)szReason);
			}
            // fall through
		case WM_CLOSE:
			EndDialog(hwndDlg,0);
			return TRUE;
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcAuthReq(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		Window_SetIcon_IcoLib(hwndDlg, SKINICON_OTHER_MIRANDA);
		Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, "View User's Details");
		Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, "Add Contact Permanently to List");
		{
			DBEVENTINFO dbei;
			DWORD uin;
			char *nick,*first,*last,*email,*reason;
			HANDLE hDbEvent, hcontact;		hDbEvent=(HANDLE)lParam;
			//blob is: uin(DWORD),hcontact(HANDLE),nick(ASCIIZ),first(ASCIIZ),last(ASCIIZ),email(ASCIIZ),reason(ASCIIZ)
			ZeroMemory(&dbei,sizeof(dbei));
			dbei.cbSize=sizeof(dbei);
			dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
			dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
			CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
			uin=*(PDWORD)dbei.pBlob;
			hcontact=*(HANDLE*)(dbei.pBlob+sizeof(DWORD));
			nick=(char *)(dbei.pBlob+sizeof(DWORD)+sizeof(HANDLE));
			first=nick+strlen(nick)+1;
			last=first+strlen(first)+1;
			email=last+strlen(last)+1;
			reason=email+strlen(email)+1;
			SetDlgItemTextA(hwndDlg,IDC_NAME,nick[0]?nick:Translate("(Unknown)"));
			if (hcontact == INVALID_HANDLE_VALUE || !DBGetContactSettingByte(hcontact, "CList", "NotOnList", 0))
				ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
			if (uin)
				SetDlgItemInt(hwndDlg,IDC_UIN,uin,FALSE);
			else {
				if (hcontact == INVALID_HANDLE_VALUE)
					SetDlgItemText(hwndDlg,IDC_UIN,TranslateT("(Unknown)"));
				else {
					CONTACTINFO ci;
					char buf[128];
					buf[0] = 0;
					ZeroMemory(&ci, sizeof(ci));
					ci.cbSize = sizeof(ci);
					ci.hContact = hcontact;
					ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hcontact, 0);
					ci.dwFlag = CNF_UNIQUEID;
					if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
						switch (ci.type) {
						case CNFT_ASCIIZ:
							mir_snprintf(buf, SIZEOF(buf), "%s", ci.pszVal);
							mir_free(ci.pszVal);
							break;
						case CNFT_DWORD:
							mir_snprintf(buf, SIZEOF(buf), "%u", ci.dVal);
							break;
						}
					}
					SetDlgItemTextA(hwndDlg,IDC_UIN,buf[0]?buf:Translate("(Unknown)"));
                    PROTOACCOUNT* acc =  ProtoGetAccount(ci.szProto);
					SetDlgItemText(hwndDlg,IDC_PROTONAME,acc->tszAccountName);
				}
			}
			SetDlgItemTextA(hwndDlg,IDC_MAIL,email[0]?email:Translate("(Unknown)"));
			SetDlgItemTextA(hwndDlg,IDC_REASON,reason);
			SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
			SetWindowLongPtr(GetDlgItem(hwndDlg,IDC_DETAILS),GWLP_USERDATA,(LONG)hcontact);
			mir_free(dbei.pBlob);
		}
		return TRUE;

	case WM_DRAWITEM:
	{	LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
		if(dis->hwndItem==GetDlgItem(hwndDlg, IDC_PROTOCOL)) {
			DBEVENTINFO dbei;
			char *szProto;
			HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);

			ZeroMemory(&dbei,sizeof(dbei));
			dbei.cbSize=sizeof(dbei);
			dbei.cbBlob=0;
			CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);

			szProto=dbei.szModule;
			if (szProto) {
				HICON hIcon;

				hIcon = ( HICON )CallProtoService(szProto,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
				if (hIcon) {
					DrawIconEx(dis->hDC,dis->rcItem.left,dis->rcItem.top,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
					DestroyIcon( hIcon );
			}	}
			return TRUE;
		}
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADD:
			{	
				HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
				ADDCONTACTSTRUCT acs={0};

				acs.handle=hDbEvent;
				acs.handleType=HANDLE_EVENT;
				acs.szProto="";
				CallService(MS_ADDCONTACT_SHOW,(WPARAM)hwndDlg,(LPARAM)&acs);
				{
					DBEVENTINFO dbei;
					HANDLE hcontact;

					ZeroMemory(&dbei,sizeof(dbei));
					dbei.cbSize=sizeof(dbei);
					dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
					dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
					CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
					hcontact=*((PHANDLE)(dbei.pBlob+sizeof(DWORD)));
					mir_free(dbei.pBlob);
					if ((hcontact == INVALID_HANDLE_VALUE) || !DBGetContactSettingByte(hcontact, "CList", "NotOnList", 0))
						ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
				}
			}
			return TRUE;
		case IDC_DETAILS:
			{	
				HANDLE hcontact=(HANDLE)GetWindowLongPtr((HWND)lParam,GWLP_USERDATA);
				CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)hcontact,0);
			}
			return TRUE;
		case IDOK:
			{	
				HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
				DBEVENTINFO dbei;
				ZeroMemory(&dbei,sizeof(dbei));
				dbei.cbSize=sizeof(dbei);
				dbei.cbBlob=0;
				CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
				CallProtoService(dbei.szModule,PS_AUTHALLOW,(WPARAM)hDbEvent,0);
			}
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDCANCEL:
			{	
				HANDLE hDbEvent=(HANDLE)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
				DialogBoxParam(hMirandaInst,MAKEINTRESOURCE(IDD_DENYREASON),hwndDlg,DenyReasonProc,(LPARAM)hDbEvent);
			}
			DestroyWindow(hwndDlg);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		Button_FreeIcon_IcoLib(hwndDlg,IDC_ADD);
		Button_FreeIcon_IcoLib(hwndDlg,IDC_DETAILS);
		Window_FreeIcon_IcoLib(hwndDlg);
		break;
	}
	return FALSE;
}
