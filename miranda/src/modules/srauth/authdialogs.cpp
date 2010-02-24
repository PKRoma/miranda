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
	HANDLE hDbEvent = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) 
	{
	case WM_INITDIALOG:
		{
			DBEVENTINFO dbei = {0};
			DWORD *uin;
			char *nick, *first, *last, *email;
			HANDLE hContact;

			TranslateDialogDefault(hwndDlg);
			Window_SetIcon_IcoLib(hwndDlg, SKINICON_OTHER_MIRANDA);
			Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, "View User's Details");
			Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, "Add Contact Permanently to List");

			hDbEvent = (HANDLE)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

			//blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ)
			dbei.cbSize = sizeof(dbei);
			dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
			dbei.pBlob  = (PBYTE)alloca(dbei.cbBlob);
			CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);

			uin = (PDWORD)dbei.pBlob;
			hContact = *((PHANDLE)(dbei.pBlob + sizeof(DWORD)));
			if (hContact == INVALID_HANDLE_VALUE || !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
				ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);

			nick  = (char*)(dbei.pBlob + sizeof(DWORD) + sizeof(HANDLE));
			first = nick  + strlen(nick)  + 1;
			last  = first + strlen(first) + 1;
			email = last  + strlen(last)  + 1;

			if (*uin) 
				SetDlgItemInt(hwndDlg, IDC_NAME, *uin, FALSE);
			else {
                if (hContact == INVALID_HANDLE_VALUE)
                    SetDlgItemText(hwndDlg, IDC_UIN, TranslateT("(Unknown)"));
                else {
					CONTACTINFO ci = {0};
                    TCHAR buf[128] = _T("");

					ci.cbSize = sizeof(ci);
                    ci.hContact = hContact;
                    ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
                    ci.dwFlag = CNF_UNIQUEID | CNF_TCHAR;
                    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci))
					{
                        switch (ci.type)
						{
                            case CNFT_ASCIIZ:
                                mir_sntprintf(buf, SIZEOF(buf), _T("%s"), ci.pszVal);
                                mir_free(ci.pszVal);
                                break;
                            case CNFT_DWORD:
                                mir_sntprintf(buf, SIZEOF(buf), _T("%u"), ci.dVal);
                                break;
                        }
                    }
                    SetDlgItemText(hwndDlg, IDC_UIN, buf[0] ? buf : TranslateT("(Unknown)"));
                }
            }
			return TRUE;
		}
	case WM_DRAWITEM:
		{
			if (wParam == IDC_PROTOCOL)
			{
				LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;

				DBEVENTINFO dbei = {0};
				dbei.cbSize = sizeof(dbei);
				CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);

				if (dbei.szModule)
				{
					HICON hIcon = (HICON)CallProtoService(dbei.szModule, PS_LOADICON, PLI_PROTOCOL | PLIF_SMALL, 0);
					if (hIcon) 
					{
						DrawIconEx(dis->hDC, dis->rcItem.left, dis->rcItem.top, hIcon,
							GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						DestroyIcon( hIcon );
					}	
				}
				return TRUE;
			}
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_ADD:
			{	
				ADDCONTACTSTRUCT acs = {0};
				acs.handle = hDbEvent;
				acs.handleType = HANDLE_EVENT;
				acs.szProto = "";
				CallService(MS_ADDCONTACT_SHOW, (WPARAM)hwndDlg, (LPARAM)&acs);

				HANDLE hContact = (HANDLE)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA);
                if ((hContact == INVALID_HANDLE_VALUE) || !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
                    ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
				break;
			}
			case IDC_DETAILS:
			{
				HANDLE hContact = (HANDLE)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA);
				CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
				break;
			}

			case IDOK:
			{	
				ADDCONTACTSTRUCT acs = {0};
				acs.handle = hDbEvent;
				acs.handleType = HANDLE_EVENT;
				acs.szProto = "";
				CallService(MS_ADDCONTACT_SHOW, (WPARAM)hwndDlg, (LPARAM)&acs);
			}
				//fall through
			case IDCANCEL:
				DestroyWindow(hwndDlg);
				break;
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
	switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SendDlgItemMessage(hwndDlg, IDC_REASON, EM_LIMITTEXT, 255, 0);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
				HANDLE hDbEvent = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
				
				DBEVENTINFO dbei = {0};
				dbei.cbSize = sizeof(dbei);
				CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);

				if (LOWORD(wParam) == IDOK)
				{
					char szReason[256];
					GetDlgItemTextA(hwndDlg, IDC_REASON, szReason, SIZEOF(szReason));
					CallProtoService(dbei.szModule, PS_AUTHDENY, (WPARAM)hDbEvent, (LPARAM)szReason);
				}
				else
					CallProtoService(dbei.szModule, PS_AUTHDENY, (WPARAM)hDbEvent, 0);
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
	HANDLE hDbEvent = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) 
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		Window_SetIcon_IcoLib(hwndDlg, SKINICON_OTHER_MIRANDA);
		Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, "View User's Details");
		Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, "Add Contact Permanently to List");
		{
			DBEVENTINFO dbei = {0};
			DWORD uin;
			char *nick,*first,*last,*email,*reason;
			HANDLE hContact;		
			
			hDbEvent = (HANDLE)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

			//blob is: uin(DWORD),hcontact(HANDLE),nick(ASCIIZ),first(ASCIIZ),last(ASCIIZ),email(ASCIIZ),reason(ASCIIZ)
			dbei.cbSize = sizeof(dbei);
			dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0);
			dbei.pBlob  = (PBYTE)alloca(dbei.cbBlob);
			CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);

			uin = *(PDWORD)dbei.pBlob;
			hContact = *(HANDLE*)(dbei.pBlob + sizeof(DWORD));
			nick     = (char *)(dbei.pBlob + sizeof(DWORD) + sizeof(HANDLE));
			first    = nick  + strlen(nick)  + 1;
			last     = first + strlen(first) + 1;
			email    = last  + strlen(last)  + 1;
			reason   = email + strlen(email) + 1;

			TCHAR* nickT = dbei.flags & DBEF_UTF ? Utf8DecodeT(nick) : mir_a2t(nick);
			SetDlgItemText(hwndDlg, IDC_NAME, nickT[0] ? nickT : TranslateT("(Unknown)"));
			mir_free(nickT);

			if (hContact == INVALID_HANDLE_VALUE || !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
				ShowWindow(GetDlgItem(hwndDlg,IDC_ADD), FALSE);
			if (uin)
				SetDlgItemInt(hwndDlg, IDC_UIN, uin, FALSE);
			else 
			{
				if (hContact == INVALID_HANDLE_VALUE)
					SetDlgItemText(hwndDlg, IDC_UIN, TranslateT("(Unknown)"));
				else 
				{
					CONTACTINFO ci = {0};
					TCHAR buf[128] = _T("");

					ci.cbSize = sizeof(ci);
					ci.hContact = hContact;
					ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					ci.dwFlag = CNF_UNIQUEID | CNF_TCHAR;
					if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci))
					{
						switch (ci.type)
						{
						case CNFT_ASCIIZ:
							mir_sntprintf(buf, SIZEOF(buf), _T("%s"), ci.pszVal);
							mir_free(ci.pszVal);
							break;

						case CNFT_DWORD:
							mir_sntprintf(buf, SIZEOF(buf), _T("%u"), ci.dVal);
							break;
						}
					}
					SetDlgItemText(hwndDlg, IDC_UIN, buf[0] ? buf : TranslateT("(Unknown)"));
                    PROTOACCOUNT* acc = ProtoGetAccount(ci.szProto);
					SetDlgItemText(hwndDlg, IDC_PROTONAME, acc->tszAccountName);
				}
			}

			TCHAR* emailT = dbei.flags & DBEF_UTF ? Utf8DecodeT(email) : mir_a2t(email);
			SetDlgItemText(hwndDlg, IDC_MAIL, emailT[0] ? emailT : TranslateT("(Unknown)"));
			mir_free(emailT);

			TCHAR* reasonT = dbei.flags & DBEF_UTF ? Utf8DecodeT(reason) : mir_a2t(reason);
			SetDlgItemText(hwndDlg, IDC_REASON, reasonT);
			mir_free(reasonT);

			SetWindowLongPtr(GetDlgItem(hwndDlg,IDC_DETAILS), GWLP_USERDATA, (LONG_PTR)hContact);
		}
		return TRUE;

	case WM_DRAWITEM:
		if (wParam == IDC_PROTOCOL)
		{
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
			DBEVENTINFO dbei = {0};

			dbei.cbSize = sizeof(dbei);
			CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);

			if (dbei.szModule) 
			{
				HICON hIcon;

				hIcon = (HICON)CallProtoService(dbei.szModule, PS_LOADICON,PLI_PROTOCOL | PLIF_SMALL, 0);
				if (hIcon) 
				{
					DrawIconEx(dis->hDC ,dis->rcItem.left, dis->rcItem.top, hIcon,
						GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
					DestroyIcon(hIcon);
				}	
			}
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_ADD:
			{	
				ADDCONTACTSTRUCT acs = {0};
				acs.handle = hDbEvent;
				acs.handleType = HANDLE_EVENT;
				acs.szProto = "";
				CallService(MS_ADDCONTACT_SHOW, (WPARAM)hwndDlg, (LPARAM)&acs);

				HANDLE hContact = (HANDLE)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA);
				if (hContact == INVALID_HANDLE_VALUE || !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
					ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
			}
			break;

		case IDC_DETAILS:
			{	
				HANDLE hContact = (HANDLE)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
				CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact,0);
			}
			break;

		case IDC_DECIDELATER:
			DestroyWindow(hwndDlg);
			break;

		case IDOK:
			{	
				DBEVENTINFO dbei = {0};
				dbei.cbSize = sizeof(dbei);
				CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);
				CallProtoService(dbei.szModule, PS_AUTHALLOW, (WPARAM)hDbEvent,0);
			}
			DestroyWindow(hwndDlg);
			break;

		case IDCANCEL:
			{
				DBEVENTINFO dbei = {0};
				dbei.cbSize = sizeof(dbei);

				CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);

				DWORD flags = CallProtoService(dbei.szModule, PS_GETCAPS,PFLAGNUM_4, 0);
				if (flags & PF4_NOAUTHDENYREASON)
					CallProtoService(dbei.szModule, PS_AUTHDENY, (WPARAM)hDbEvent, 0);
				else
					DialogBoxParam(hMirandaInst, MAKEINTRESOURCE(IDD_DENYREASON), hwndDlg,
						DenyReasonProc, (LPARAM)hDbEvent);
			}
			DestroyWindow(hwndDlg);
			break;;
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
