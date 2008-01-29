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

#define MS_PROTO_SHOWACCMGR "Protos/ShowAccountManager" 

#define LBN_MY_CHECK	0x1000
#define WM_MY_REFRESH (WM_USER+0x1000)

BOOL IsProtocolLoaded(char* pszProtocolName);
int  Proto_EnumProtocols( WPARAM, LPARAM );

const char errMsg[] =
	"WARNING! The account is goind to be deleted. It means that all its "
	"settings, contacts and histories will be also erased.\n\n"
	"Are you absolutely sure?";

static HWND hAccMgr = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Account edit form
// Gets PROTOACCOUNT* as a parameter, or NULL to edit a new one

static BOOL CALLBACK AccFormDlgProc(HWND hwndDlg,UINT message, WPARAM wParam, LPARAM lParam)
{
	switch( message ) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			PROTOCOLDESCRIPTOR** proto;
			int protoCount, i;
			Proto_EnumProtocols(( WPARAM )&protoCount, ( LPARAM )&proto );
			for ( i=0; i < protoCount; i++ )
				if ( proto[i]->type == PROTOTYPE_PROTOCOL )
					SendDlgItemMessageA( hwndDlg, IDC_PROTOTYPECOMBO, CB_ADDSTRING, 0, (LPARAM)proto[i]->szName );
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
				PROTOACCOUNT* pa = ( PROTOACCOUNT* )GetWindowLong( hwndDlg, GWL_USERDATA );
				if ( pa == NULL ) {
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
						pa->szModuleName = mir_strdup( buf );
					}
					List_InsertPtr(( SortedList* )&accounts, pa );
				}
				{
					TCHAR buf[256];
					GetDlgItemText( hwndDlg, IDC_ACCNAME, buf, sizeof( buf )); buf[SIZEOF(buf)-1] = 0;
					mir_free(pa->tszAccountName);
					pa->tszAccountName = mir_tstrdup( buf );
				}
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
};

static LRESULT CALLBACK AccListWndProc(HWND hwnd,UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct TAccListData *dat = (struct TAccListData *)GetWindowLong(hwnd, GWL_USERDATA);
	if ( !dat )
		return DefWindowProc(hwnd, msg, wParam, lParam);

	if (msg == WM_LBUTTONDOWN)
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
	else if (msg == WM_LBUTTONUP)
	{
		POINT pt = {LOWORD(lParam), HIWORD(lParam)};
		if ((dat->iItem >= 0) && PtInRect(&dat->rcCheck, pt))
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetWindowLong(hwnd, GWL_ID), LBN_MY_CHECK), (LPARAM)dat->iItem);
		dat->iItem = -1;
	}
	else if ((msg == WM_CHAR) && (wParam == ' '))
	{
		int iItem = ListBox_GetCurSel(hwnd);
		if (iItem >= 0)
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetWindowLong(hwnd, GWL_ID), LBN_MY_CHECK), (LPARAM)iItem);
	}

	return CallWindowProc(dat->oldWndProc, hwnd, msg, wParam, lParam);
}

static void sttSubclassAccList(HWND hwnd, BOOL subclass)
{
	if (subclass)
	{
		struct TAccListData *dat = (struct TAccListData *)mir_alloc(sizeof(struct TAccListData));
		dat->iItem = -1;
		dat->oldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)dat);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)AccListWndProc);
	} else
	{
		struct TAccListData *dat = (struct TAccListData *)GetWindowLong(hwnd, GWL_USERDATA);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)dat->oldWndProc);
		SetWindowLong(hwnd, GWL_USERDATA, 0);
		mir_free(dat);
	}
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
		return TRUE;
	}

	case WM_MEASUREITEM:
	{
		LPMEASUREITEMSTRUCT lps = (LPMEASUREITEMSTRUCT)lParam;
		PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;

		if ((lps->CtlID != IDC_ACCLIST) || !acc)
			break;

		lps->itemWidth = 10;
		lps->itemHeight = dat->normalHeight;

		return TRUE;
	}

	case WM_DRAWITEM:
	{
		int tmp, size, length;
		TCHAR *text;
		HICON hIcon;
		SIZE sz;

		int cxIcon = GetSystemMetrics(SM_CXSMICON);
		int cyIcon = GetSystemMetrics(SM_CYSMICON);

		LPDRAWITEMSTRUCT lps = (LPDRAWITEMSTRUCT)lParam;
		PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;

		if ((lps->CtlID != IDC_ACCLIST) || (lps->itemID == -1) || !acc)
			break;

		SetBkMode(lps->hDC, TRANSPARENT);
		if (lps->itemState & ODS_SELECTED)
		{
			FillRect(lps->hDC, &lps->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
			SetTextColor(lps->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		} else
		{
			FillRect(lps->hDC, &lps->rcItem, GetSysColorBrush(COLOR_WINDOW));
			SetTextColor(lps->hDC, GetSysColor(COLOR_WINDOWTEXT));
		}

		lps->rcItem.left += 2;
		lps->rcItem.top += 2;
		lps->rcItem.bottom -= 2;

		if (CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)acc->szProtoName))
		{
			DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top,
				LoadSkinnedIcon(acc->bIsEnabled ? SKINICON_OTHER_TICK : SKINICON_OTHER_NOTICK),
				cxIcon, cyIcon, 0, NULL, DI_NORMAL);
		}
		lps->rcItem.left += cxIcon + 2;

		if (acc->ppro)
		{
			hIcon = acc->ppro->vtbl->GetIcon(acc->ppro, PLI_PROTOCOL);
			DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top, hIcon, cxIcon, cyIcon, 0, NULL, DI_NORMAL);
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

		if (lps->itemID == dat->iSelected)
		{
			SelectObject(lps->hDC, dat->hfntText);
			mir_sntprintf(text, size, _T("%s: ") _T(TCHAR_STR_PARAM), TranslateT("Protocol"), acc->szProtoName);
			length = lstrlen(text);
			DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			GetTextExtentPoint32(lps->hDC, text, length, &sz);
			lps->rcItem.top += sz.cy + 2;

			if (acc->ppro && CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)acc->szProtoName))
			{
				char *szIdName;
				TCHAR *tszIdName;
				DBVARIANT dbv = {DBVT_TCHAR};
				DBCONTACTGETSETTING dbcgs = {acc->szModuleName, (char *)acc->ppro->vtbl->GetCaps(acc->ppro, PFLAG_UNIQUEIDSETTING), &dbv};

				szIdName = (char *)acc->ppro->vtbl->GetCaps(acc->ppro, PFLAG_UNIQUEIDTEXT);
				tszIdName = szIdName ? mir_a2t(szIdName) : mir_tstrdup(TranslateT("Account ID"));
				if (!CallService(MS_DB_CONTACT_GETSETTING_STR, 0, (LPARAM)&dbcgs))
				{
					switch (dbv.type)
					{
					case DBVT_DWORD:
						mir_sntprintf(text, size, _T("%s: %d"), tszIdName, dbv.dVal);
						break;
					case DBVT_TCHAR:
						mir_sntprintf(text, size, _T("%s: %s"), tszIdName, dbv.ptszVal);
						break;
					default:
						mir_sntprintf(text, size, _T("%s: %s"), tszIdName, TranslateT("<unknown>"));
						break;
					}
					DBFreeVariant(&dbv);
				} else
				{
					mir_sntprintf(text, size, _T("%s: %s"), tszIdName, TranslateT("<unknown>"));
				}
				mir_free(tszIdName);
			} else
			{
				mir_sntprintf(text, size, _T("Protocol is not loaded."));
			}

			length = lstrlen(text);
			DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			GetTextExtentPoint32(lps->hDC, text, length, &sz);
			lps->rcItem.top += sz.cy + 2;
		}

		return TRUE;
	}

	case WM_MY_REFRESH:
	{
		HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
		int i;
		for (i = 0; i < accounts.count; ++i)
			SendMessage(hList, LB_SETITEMDATA, SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)accounts.items[i]->tszAccountName), (LPARAM)accounts.items[i]);
		break;
	}

	case WM_COMMAND:
		switch( LOWORD(wParam)) {
		case IDC_ACCLIST: {
			HWND hwndList = GetDlgItem(hwndDlg, IDC_ACCLIST);
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				{
					if (dat->iSelected >= 0)
						ListBox_SetItemHeight(hwndList, dat->iSelected, dat->normalHeight);
					dat->iSelected = ListBox_GetCurSel(hwndList);
					ListBox_SetItemHeight(hwndList, dat->iSelected, dat->selectedHeight);
					RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
					break;
				}
			case LBN_MY_CHECK:
				{
					PROTOACCOUNT *acc = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, lParam);
					if (acc) {
						acc->bIsEnabled = !acc->bIsEnabled;
						RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
					}
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
					OPENOPTIONSDIALOG ood;
					int len = SendMessageA( hList, LB_GETTEXTLEN, idx, 0 );
					char* str = (char*)alloca( len + 2 );
					if ( SendMessageA( hList, LB_GETTEXT, idx, (LPARAM)str ) == LB_ERR ) break;
					ood.cbSize = sizeof(ood);
					ood.pszGroup = "Network";
					ood.pszPage = str;
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

int LoadProtoOptions( void )
{
	CreateServiceFunction( MS_PROTO_SHOWACCMGR, OptProtosShow );

	HookEvent( ME_SYSTEM_MODULESLOADED, OptProtosLoaded );
	return 0;
}
