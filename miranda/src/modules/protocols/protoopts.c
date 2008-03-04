/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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

#include "m_protoint.h"

#define LBN_MY_CHECK	0x1000
#define LBN_MY_RENAME	0x1001

#define WM_MY_REFRESH	(WM_USER+0x1000)
#define WM_MY_RENAME	(WM_USER+0x1001)

BOOL IsProtocolLoaded(char* pszProtocolName);
int  Proto_EnumProtocols( WPARAM, LPARAM );

const TCHAR errMsg[] =
	_T("WARNING! The account is goind to be deleted. It means that all its ")
	_T("settings, contacts and histories will be also erased.\n\n")
	_T("Are you absolutely sure?");

static HWND hAccMgr = NULL;

extern HANDLE hAccListChanged;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Account edit form
// Gets PROTOACCOUNT* as a parameter, or NULL to edit a new one

static char* rtrim( char* string )
{
   char* p = string + strlen( string ) - 1;

   while ( p >= string ) {
		if ( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
         break;

		*p-- = 0;
   }
   return string;
}

static BOOL CALLBACK AccFormDlgProc(HWND hwndDlg,UINT message, WPARAM wParam, LPARAM lParam)
{
	switch( message ) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			PROTOCOLDESCRIPTOR** proto;
			int protoCount, i;
			Proto_EnumProtocols(( WPARAM )&protoCount, ( LPARAM )&proto );
			for ( i=0; i < protoCount; i++ ) {
				PROTOCOLDESCRIPTOR* pd = proto[i];
				if ( pd->type == PROTOTYPE_PROTOCOL && pd->cbSize == sizeof( *pd ))
					SendDlgItemMessageA( hwndDlg, IDC_PROTOTYPECOMBO, CB_ADDSTRING, 0, (LPARAM)proto[i]->szName );
			}
			SendDlgItemMessage( hwndDlg, IDC_PROTOTYPECOMBO, CB_SETCURSEL, 0, 0 );
		}
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
		{
			PROTOACCOUNT* pa = ( PROTOACCOUNT* )lParam;
			if ( pa == NULL ) // new account
				SetWindowText( hwndDlg, TranslateT( "Create new account" ));
			else {
				TCHAR str[200];
				mir_sntprintf( str, SIZEOF(str), _T("%s: %s"), TranslateT( "Editing account" ), pa->tszAccountName );
				SetWindowText( hwndDlg, str );

				SetDlgItemText( hwndDlg, IDC_ACCNAME, pa->tszAccountName );
				SetDlgItemTextA( hwndDlg, IDC_ACCINTERNALNAME, pa->szModuleName );

				SendDlgItemMessageA( hwndDlg, IDC_PROTOTYPECOMBO, CB_SELECTSTRING, -1, (LPARAM)pa->szProtoName );
				EnableWindow( GetDlgItem( hwndDlg, IDC_PROTOTYPECOMBO ), FALSE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_ACCINTERNALNAME ), FALSE );
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam)) {
		case IDOK:
			{
				int iAction = 2;
				PROTOACCOUNT* pa = ( PROTOACCOUNT* )GetWindowLong( hwndDlg, GWL_USERDATA );
				if ( pa == NULL ) {
					iAction = 1;
					pa = (PROTOACCOUNT*)mir_calloc( sizeof( PROTOACCOUNT ));
					pa->cbSize = sizeof( PROTOACCOUNT );
					pa->bIsEnabled = pa->bIsVisible = TRUE;
					pa->iOrder = accounts.count;
					pa->type = PROTOTYPE_PROTOCOL;
					{
						char buf[200];
						GetDlgItemTextA( hwndDlg, IDC_PROTOTYPECOMBO, buf, sizeof( buf )); buf[sizeof(buf)-1] = 0;
						pa->szProtoName = mir_strdup( buf );
						GetDlgItemTextA( hwndDlg, IDC_ACCINTERNALNAME, buf, sizeof( buf )); buf[sizeof(buf)-1] = 0;
						rtrim( buf );
						if ( buf[0] == 0 ) {
							int count = 1;
							while( TRUE ) {
								DBVARIANT dbv;
								sprintf( buf, "%s_%d", pa->szProtoName, count++ );
								if ( DBGetContactSettingString( NULL, buf, "AM_BaseProto", &dbv ))
									break;
								DBFreeVariant( &dbv );
						}	}
						pa->szModuleName = mir_strdup( buf );
					}
					List_InsertPtr(( SortedList* )&accounts, pa );
					DBWriteContactSettingString( NULL, pa->szModuleName, "AM_BaseProto", pa->szProtoName );
					if ( ActivateAccount( pa ))
						pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONLOAD, 0, 0 );
				}
				{
					TCHAR buf[256];
					GetDlgItemText( hwndDlg, IDC_ACCNAME, buf, sizeof( buf )); buf[SIZEOF(buf)-1] = 0;
					mir_free(pa->tszAccountName);
					pa->tszAccountName = mir_tstrdup( buf );
				}
				
				NotifyEventHooks( hAccListChanged, iAction, ( LPARAM )pa );

				WriteDbAccounts();
			}
			EndDialog( hwndDlg, TRUE );
			break;

		case IDCANCEL:
			EndDialog( hwndDlg, FALSE );
			break;
		}
	}
	
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Accounts manager

struct TAccMgrData
{
	HFONT hfntTitle, hfntText;
	int titleHeight, textHeight;
	int selectedHeight, normalHeight;
	int iSelected;
};

struct TAccListData
{
	WNDPROC oldWndProc;
	int iItem;
	RECT rcCheck;

	HWND hwndEdit;
	WNDPROC oldEditProc;
};

static void sttClickButton(HWND hwndDlg, int idcButton)
{
	if (IsWindowEnabled(GetDlgItem(hwndDlg, idcButton)))
		PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(idcButton, BN_CLICKED), (LPARAM)GetDlgItem(hwndDlg, idcButton));
}

static LRESULT CALLBACK sttEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			{
				int length = GetWindowTextLength(hwnd) + 1;
				TCHAR *str = mir_alloc(sizeof(TCHAR) * length);
				GetWindowText(hwnd, str, length);
				SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, MAKEWPARAM(GetWindowLong(GetParent(hwnd), GWL_ID), LBN_MY_RENAME), (LPARAM)str);
				DestroyWindow(hwnd);
			}
			return 0;

		case VK_ESCAPE:
			SetFocus(GetParent(hwnd));
			DestroyWindow(hwnd);
			return 0;
		}
		break;
	case WM_GETDLGCODE:
		if ((wParam == VK_RETURN) || (wParam == VK_ESCAPE))
			return DLGC_WANTMESSAGE;
		break;
	case WM_KILLFOCUS:
		DestroyWindow(hwnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)GetWindowLong(hwnd, GWL_USERDATA), hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK AccListWndProc(HWND hwnd,UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct TAccListData *dat = (struct TAccListData *)GetWindowLong(hwnd, GWL_USERDATA);
	if ( !dat )
		return DefWindowProc(hwnd, msg, wParam, lParam);

	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			int iItem = LOWORD(SendMessage(hwnd, LB_ITEMFROMPOINT, 0, lParam));
			ListBox_GetItemRect(hwnd, iItem, &dat->rcCheck);

			dat->rcCheck.right = dat->rcCheck.left + GetSystemMetrics(SM_CXSMICON) + 4;
			dat->rcCheck.bottom = dat->rcCheck.top + GetSystemMetrics(SM_CYSMICON) + 4;
			if (PtInRect(&dat->rcCheck, pt))
				dat->iItem = iItem;
			else
				dat->iItem = -1;
		}
		break;

	case WM_LBUTTONUP:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			if ((dat->iItem >= 0) && PtInRect(&dat->rcCheck, pt))
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetWindowLong(hwnd, GWL_ID), LBN_MY_CHECK), (LPARAM)dat->iItem);
			dat->iItem = -1;
		}
		break;

	case WM_CHAR:
		if (wParam == ' ') {
			int iItem = ListBox_GetCurSel(hwnd);
			if (iItem >= 0)
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetWindowLong(hwnd, GWL_ID), LBN_MY_CHECK), (LPARAM)iItem);
			return 0;
		}

		if (wParam == 10 /* enter */)
			return 0;

		break;

	case WM_GETDLGCODE:
		if (wParam == VK_RETURN)
			return DLGC_WANTMESSAGE;
		break;

	case WM_MY_RENAME:
		{
			RECT rc;
			struct TAccMgrData *parentDat = (struct TAccMgrData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);
			PROTOACCOUNT *acc = (PROTOACCOUNT *)ListBox_GetItemData(hwnd, ListBox_GetCurSel(hwnd));
			if (!acc)
				return 0;

			ListBox_GetItemRect(hwnd, ListBox_GetCurSel(hwnd), &rc);
			rc.left += 2*GetSystemMetrics(SM_CXSMICON) + 4;
			rc.bottom = rc.top + max(GetSystemMetrics(SM_CXSMICON), parentDat->titleHeight) + 4 - 1;
			++rc.top; --rc.right;

			dat->hwndEdit = CreateWindow(_T("EDIT"), acc->tszAccountName, WS_CHILD|WS_BORDER|ES_AUTOHSCROLL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, hwnd, NULL, GetModuleHandle(NULL), NULL);
			SetWindowLong(dat->hwndEdit, GWL_USERDATA, SetWindowLong(dat->hwndEdit, GWL_WNDPROC, (LONG)sttEditSubclassProc));
			SendMessage(dat->hwndEdit, WM_SETFONT, (WPARAM)parentDat->hfntTitle, 0);
			SendMessage(dat->hwndEdit, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN|EC_USEFONTINFO, 0);
			SendMessage(dat->hwndEdit, EM_SETSEL, 0, (LPARAM) (-1));
			ShowWindow(dat->hwndEdit, SW_SHOW);
		}
		SetFocus(dat->hwndEdit);
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_F2:
			SendMessage(hwnd, WM_MY_RENAME, 0, 0);
			return 0;

		case VK_INSERT:
			sttClickButton(GetParent(hwnd), IDC_ADD);
			return 0;

		case VK_DELETE:
			sttClickButton(GetParent(hwnd), IDC_REMOVE);
			return 0;

		case VK_RETURN:
			if (GetAsyncKeyState(VK_CONTROL))
				sttClickButton(GetParent(hwnd), IDC_EDIT);
			else
				sttClickButton(GetParent(hwnd), IDOK);
			return 0;
		}
		break;
	}

	return CallWindowProc(dat->oldWndProc, hwnd, msg, wParam, lParam);
}

static void sttSubclassAccList(HWND hwnd, BOOL subclass)
{
	if (subclass) {
		struct TAccListData *dat = (struct TAccListData *)mir_alloc(sizeof(struct TAccListData));
		dat->iItem = -1;
		dat->oldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)dat);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)AccListWndProc);
	}
	else {
		struct TAccListData *dat = (struct TAccListData *)GetWindowLong(hwnd, GWL_USERDATA);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)dat->oldWndProc);
		SetWindowLong(hwnd, GWL_USERDATA, 0);
		mir_free(dat);
}	}

static void sttSelectItem(struct TAccMgrData *dat, HWND hwndList, int iItem)
{
	if (dat->iSelected >= 0)
		ListBox_SetItemHeight(hwndList, dat->iSelected, dat->normalHeight);
	dat->iSelected = ListBox_GetCurSel(hwndList);
	ListBox_SetItemHeight(hwndList, dat->iSelected, dat->selectedHeight);
	RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
}

static BOOL CALLBACK AccMgrDlgProc(HWND hwndDlg,UINT message, WPARAM wParam, LPARAM lParam)
{
	struct TAccMgrData *dat = (struct TAccMgrData *)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch(message) {
	case WM_INITDIALOG:
		{
			struct TAccMgrData *dat = (struct TAccMgrData *)mir_alloc(sizeof(struct TAccMgrData));
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);

			TranslateDialogDefault(hwndDlg);
			Utils_RestoreWindowPositionNoSize(hwndDlg, NULL, "AccMgr", "");

			Window_SetIcon_IcoLib( hwndDlg, SKINICON_OTHER_ACCMGR );
			SendDlgItemMessage( hwndDlg, IDC_ADD, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_ADDCONTACT ));
			SendDlgItemMessage( hwndDlg, IDC_EDIT, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_RENAME ));
			SendDlgItemMessage( hwndDlg, IDC_REMOVE, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_DELETE ));
			SendDlgItemMessage( hwndDlg, IDC_OPTIONS, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_OPTIONS ));

			{
				LOGFONT lf;
				HDC hdc;
				HFONT hfnt;
				TEXTMETRIC tm;

				GetObject((HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0), sizeof(lf), &lf);
				dat->hfntText = CreateFontIndirect(&lf);

				GetObject((HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0), sizeof(lf), &lf);
				lf.lfWeight = FW_BOLD;
				dat->hfntTitle = CreateFontIndirect(&lf);

				hdc = GetDC(hwndDlg);
				hfnt = SelectObject(hdc, dat->hfntTitle);
				GetTextMetrics(hdc, &tm);
				dat->titleHeight = tm.tmHeight;
				SelectObject(hdc, dat->hfntText);
				GetTextMetrics(hdc, &tm);
				dat->textHeight = tm.tmHeight;
				SelectObject(hdc, hfnt);
				ReleaseDC(hwndDlg, hdc);

				dat->normalHeight = 4 + max(dat->titleHeight, GetSystemMetrics(SM_CYSMICON));
				dat->selectedHeight = dat->normalHeight + 4 + 2 * dat->textHeight;
			}

			dat->iSelected = -1;
			sttSubclassAccList(GetDlgItem(hwndDlg, IDC_ACCLIST), TRUE);
			SendMessage( hwndDlg, WM_MY_REFRESH, 0, 0 );
		}
		return TRUE;

	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lps = (LPMEASUREITEMSTRUCT)lParam;
			PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;

			if ((lps->CtlID != IDC_ACCLIST) || !acc)
				break;

			lps->itemWidth = 10;
			lps->itemHeight = dat->normalHeight;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			int tmp, size, length;
			TCHAR *text;
			HICON hIcon;
			HBRUSH hbrBack;
			SIZE sz;

			int cxIcon = GetSystemMetrics(SM_CXSMICON);
			int cyIcon = GetSystemMetrics(SM_CYSMICON);

			LPDRAWITEMSTRUCT lps = (LPDRAWITEMSTRUCT)lParam;
			PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;

			if ((lps->CtlID != IDC_ACCLIST) || (lps->itemID == -1) || !acc)
				break;

			SetBkMode(lps->hDC, TRANSPARENT);
			if (lps->itemState & ODS_SELECTED) {
				hbrBack = GetSysColorBrush(COLOR_HIGHLIGHT);
				SetTextColor(lps->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			}
			else {
				hbrBack = GetSysColorBrush(COLOR_WINDOW);
				SetTextColor(lps->hDC, GetSysColor(COLOR_WINDOWTEXT));
			}
			FillRect(lps->hDC, &lps->rcItem, hbrBack);

			lps->rcItem.left += 2;
			lps->rcItem.top += 2;
			lps->rcItem.bottom -= 2;

			if (CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)acc->szProtoName))
				DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top,
					LoadSkinnedIcon(acc->bIsEnabled ? SKINICON_OTHER_TICK : SKINICON_OTHER_NOTICK),
					cxIcon, cyIcon, 0, hbrBack, DI_NORMAL);

			lps->rcItem.left += cxIcon + 2;

			if (acc->ppro) {
				hIcon = acc->ppro->vtbl->GetIcon(acc->ppro, PLI_PROTOCOL);
				DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top, hIcon, cxIcon, cyIcon, 0, hbrBack, DI_NORMAL);
				DestroyIcon(hIcon);
			}
			lps->rcItem.left += cxIcon + 2;

			length = SendDlgItemMessage(hwndDlg, IDC_ACCLIST, LB_GETTEXTLEN, lps->itemID, 0);
			size = max(length+1, 256);
			text = (TCHAR *)_alloca(sizeof(TCHAR) * size);
			SendDlgItemMessage(hwndDlg, IDC_ACCLIST, LB_GETTEXT, lps->itemID, (LPARAM)text);

			SelectObject(lps->hDC, dat->hfntTitle);
			tmp = lps->rcItem.bottom;
			lps->rcItem.bottom = lps->rcItem.top + max(cyIcon, dat->titleHeight);
			DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS|DT_VCENTER);
			lps->rcItem.bottom = tmp;
			GetTextExtentPoint32(lps->hDC, text, length, &sz);
			lps->rcItem.top += max(cxIcon, sz.cy) + 2;

			if (lps->itemID == (unsigned)dat->iSelected) {
				SelectObject(lps->hDC, dat->hfntText);
				mir_sntprintf(text, size, _T("%s: ") _T(TCHAR_STR_PARAM), TranslateT("Protocol"), acc->szProtoName);
				length = lstrlen(text);
				DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
				GetTextExtentPoint32(lps->hDC, text, length, &sz);
				lps->rcItem.top += sz.cy + 2;

				if (acc->ppro && CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)acc->szProtoName)) {
					char *szIdName;
					TCHAR *tszIdName;
					char *uniqueIdSetting = (char *)acc->ppro->vtbl->GetCaps(acc->ppro, PFLAG_UNIQUEIDSETTING);

					szIdName = (char *)acc->ppro->vtbl->GetCaps(acc->ppro, PFLAG_UNIQUEIDTEXT);
					tszIdName = szIdName ? mir_a2t(szIdName) : mir_tstrdup(TranslateT("Account ID"));
					if ( PF1_NUMERICUSERID & acc->ppro->vtbl->GetCaps(acc->ppro, PFLAGNUM_1)) {
						DWORD uid = DBGetContactSettingDword( NULL, acc->szModuleName, uniqueIdSetting, -1 );
						mir_sntprintf(text, size, _T("%s: %d"), tszIdName, uid);
					}
					else {
						DBVARIANT dbv = { 0 };
						if ( !DBGetContactSettingTString( NULL, acc->szModuleName, uniqueIdSetting, &dbv )) {
							mir_sntprintf(text, size, _T("%s: %s"), tszIdName, dbv.ptszVal );
							DBFreeVariant(&dbv);
						}
						else mir_sntprintf(text, size, _T("%s: %s"), tszIdName, TranslateT("<unknown>"));
					} 
					mir_free(tszIdName);
				}
				else mir_sntprintf(text, size, _T("Protocol is not loaded."));

				length = lstrlen(text);
				DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
				GetTextExtentPoint32(lps->hDC, text, length, &sz);
				lps->rcItem.top += sz.cy + 2;
		}	}
		return TRUE;

	case WM_MY_REFRESH:
		{
			HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
			int i;
			for (i = 0; i < accounts.count; ++i)
				SendMessage(hList, LB_SETITEMDATA, SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)accounts.items[i]->tszAccountName), (LPARAM)accounts.items[i]);
		}
		break;

	case WM_CONTEXTMENU:
		if ( GetWindowLong(( HWND )wParam, GWL_ID ) == IDC_ACCLIST ) {
			HWND hwndList = GetDlgItem( hwndDlg, IDC_ACCLIST );
			POINT pt = { (signed short)LOWORD( lParam ), (signed short)HIWORD( lParam ) };
			int iItem = ListBox_GetCurSel( hwndList );

			if (( pt.x == -1 ) && ( pt.y == -1 )) {
				if (iItem != -1) {
					RECT rc;
					ListBox_GetItemRect( hwndList, iItem, &rc );
					pt.x = rc.left + GetSystemMetrics(SM_CXSMICON) + 4;
					pt.y = rc.top + 4 + max(GetSystemMetrics(SM_CXSMICON), dat->titleHeight);
					ClientToScreen( hwndList, &pt );
				}
			}
			else {
				// menu was activated with mouse => find item under cursor & set focus to our control.
				POINT ptItem = pt;
				ScreenToClient( hwndList, &ptItem );
				iItem = LOWORD(SendMessage(hwndList, LB_ITEMFROMPOINT, 0, MAKELPARAM(ptItem.x, ptItem.y)));
				ListBox_SetCurSel(hwndList, iItem);
				sttSelectItem(dat, hwndList, iItem);
				SetFocus(hwndList);
			}

			if ( iItem != -1 ) {
				HMENU hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)1, TranslateT("Rename"));
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)2, TranslateT("Edit"));
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)3, TranslateT("Delete"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)0, TranslateT("Cancel"));
				switch (TrackPopupMenu( hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL )) {
				case 1:
					SendMessage(hwndList, WM_MY_RENAME, 0, 0);
					break;
				case 2:
					sttClickButton(hwndDlg, IDC_EDIT);
					break;
				case 3:
					sttClickButton(hwndDlg, IDC_REMOVE);
					break;
				}
				DestroyMenu( hMenu );
		}	}
		break;

	case WM_COMMAND:
		switch( LOWORD(wParam)) {
		case IDC_ACCLIST: {
			HWND hwndList = GetDlgItem(hwndDlg, IDC_ACCLIST);
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				sttSelectItem(dat, hwndList, ListBox_GetCurSel(hwndList));
				break;
			case LBN_MY_CHECK:
				{
					PROTOACCOUNT *pa = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, lParam);
					if ( pa ) {
						pa->bIsEnabled = !pa->bIsEnabled;
						if ( pa->bIsEnabled ) {
							if ( ActivateAccount( pa ))
								pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONLOAD, 0, 0 );
						}
						else DeactivateAccount( pa, TRUE );
						WriteDbAccounts();
						NotifyEventHooks( hAccListChanged, 5, ( LPARAM )pa );
						RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
					}
					break;
				}
			case LBN_MY_RENAME:
				{
					int iItem = ListBox_GetCurSel(hwndList);
					PROTOACCOUNT *acc = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, iItem);
					if (acc) {
						mir_free(acc->tszAccountName);
						acc->tszAccountName = (TCHAR *)lParam;
						NotifyEventHooks( hAccListChanged, 2, ( LPARAM )acc );

						ListBox_DeleteString(hwndList, iItem);
						iItem = ListBox_AddString(hwndList, acc->tszAccountName);
						ListBox_SetItemData(hwndList, iItem, (LPARAM)acc);
						ListBox_SetCurSel(hwndList, iItem);

						sttSelectItem(dat, hwndList, iItem);

						RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
					}
					break;
				}
			}
			break;
		}

		case IDC_ADD:
			if ( IDOK == DialogBoxParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ACCFORM), hwndDlg, AccFormDlgProc, 0 )) {
				SendDlgItemMessage( hwndDlg, IDC_ACCLIST, LB_RESETCONTENT, 0, 0 );
				SendMessage( hwndDlg, WM_MY_REFRESH, 0, 0 );
			}
			break;

		case IDC_EDIT:
			{
				HWND hList = GetDlgItem( hwndDlg, IDC_ACCLIST );
				int idx = ListBox_GetCurSel( hList );
				if ( idx != -1 ) {
					LPARAM lParam = ListBox_GetItemData( hList, idx );
					if ( IDOK == DialogBoxParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ACCFORM), hwndDlg, AccFormDlgProc, lParam )) {
						SendDlgItemMessage( hwndDlg, IDC_ACCLIST, LB_RESETCONTENT, 0, 0 );
						SendMessage( hwndDlg, WM_MY_REFRESH, 0, 0 );
			}	}	}
			break;

		case IDC_REMOVE:
			{
				HWND hList = GetDlgItem( hwndDlg, IDC_ACCLIST );
				int idx = ListBox_GetCurSel( hList );
				if ( idx != -1 ) {
					PROTOACCOUNT* pa = ( PROTOACCOUNT* )ListBox_GetItemData( hList, idx );
					TCHAR buf[ 200 ];
					mir_sntprintf( buf, SIZEOF(buf), TranslateT( "Account %s is being deleted" ));
					if ( IDYES == MessageBox( NULL, TranslateTS( errMsg ), buf, MB_ICONSTOP | MB_DEFBUTTON2 | MB_YESNO )) {
						List_RemovePtr(( SortedList* )&accounts, pa );
						NotifyEventHooks( hAccListChanged, 3, ( LPARAM )pa );
						EraseAccount( pa );
						UnloadAccount( pa, TRUE );
						WriteDbAccounts();
						SendDlgItemMessage( hwndDlg, IDC_ACCLIST, LB_RESETCONTENT, 0, 0 );
						SendMessage( hwndDlg, WM_MY_REFRESH, 0, 0 );
			}	}	}
			break;

		case IDC_OPTIONS:
			{
				HWND hList = GetDlgItem( hwndDlg, IDC_ACCLIST );
				int idx = ListBox_GetCurSel( hList );
				if ( idx != -1 ) {
					PROTOACCOUNT* pa = ( PROTOACCOUNT* )ListBox_GetItemData( hList, idx );
					OPENOPTIONSDIALOG ood;
					ood.cbSize = sizeof(ood);
					ood.pszGroup = "Network";
					ood.pszPage = pa->szModuleName;
					ood.pszTab = NULL;
					CallService( MS_OPT_OPENOPTIONS, 0, (LPARAM)&ood );
			}	}
			break;

		case IDOK:
		case IDCANCEL:
			EndDialog(hwndDlg,LOWORD(wParam));
			break;
		}
		break;

	case WM_DESTROY:
		Window_FreeIcon_IcoLib( hwndDlg );
		Button_FreeIcon_IcoLib( hwndDlg, IDC_ADD );
		Button_FreeIcon_IcoLib( hwndDlg, IDC_EDIT );
		Button_FreeIcon_IcoLib( hwndDlg, IDC_REMOVE );
		Button_FreeIcon_IcoLib( hwndDlg, IDC_OPTIONS );
		Utils_SaveWindowPosition( hwndDlg, NULL, "AccMgr", "");
		sttSubclassAccList(GetDlgItem(hwndDlg, IDC_ACCLIST), FALSE);
		DeleteObject(dat->hfntTitle);
		DeleteObject(dat->hfntText);
		mir_free(dat);
		break;
	}

	return FALSE;
}

static int OptProtosShow(WPARAM wParam,LPARAM lParam)
{
	if ( !hAccMgr )
		hAccMgr = CreateDialogParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ACCMGR), NULL, AccMgrDlgProc, 0 );

	ShowWindow( hAccMgr, SW_RESTORE );
	SetForegroundWindow( hAccMgr );
	return 0;
}

int OptProtosLoaded(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.icolibItem = GetSkinIconHandle( SKINICON_OTHER_ACCMGR );
	mi.position = 1900000000;
	mi.pszName = LPGEN("&Accounts...");
	mi.pszService = MS_PROTO_SHOWACCMGR;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	return 0;
}

static int OnAccListChanged( WPARAM eventCode, LPARAM lParam )
{
	PROTOACCOUNT* pa = (PROTOACCOUNT*)lParam;

	switch( eventCode ) {
	case PRAC_CHANGED:
		if ( pa->ppro ) {
			mir_free( pa->ppro->m_tszUserName );
			pa->ppro->m_tszUserName = mir_tstrdup( pa->tszAccountName );
			pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONRENAME, 0, lParam );
		}
		// fall through

	case PRAC_ADDED:
	case PRAC_REMOVED:
	case PRAC_CHECKED:
		cli.pfnReloadProtoMenus();
		cli.pfnTrayIconIconsChanged();
		cli.pfnClcBroadcast( INTM_RELOADOPTIONS, 0, 0 );
		cli.pfnClcBroadcast( INTM_INVALIDATE, 0, 0 );
		break;
	}

	return 0;
}

int LoadProtoOptions( void )
{
	CreateServiceFunction( MS_PROTO_SHOWACCMGR, OptProtosShow );

	HookEvent( ME_SYSTEM_MODULESLOADED, OptProtosLoaded );
	HookEvent( ME_PROTO_ACCLISTCHANGED, OnAccListChanged );
	return 0;
}
