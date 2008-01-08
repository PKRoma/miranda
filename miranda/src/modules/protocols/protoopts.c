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

static HWND hAccMgr = NULL;

static BOOL CALLBACK AccMgrDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		Utils_RestoreWindowPositionNoSize(hdlg, NULL, "AccMgr", "");

		Window_SetIcon_IcoLib( hdlg, SKINICON_OTHER_ACCMGR );
		SendDlgItemMessage( hdlg, IDC_ADD, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_ADDCONTACT ));
		SendDlgItemMessage( hdlg, IDC_EDIT, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_RENAME ));
		SendDlgItemMessage( hdlg, IDC_REMOVE, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_DELETE ));
		{
			int i = 0, idx;
			HWND hList = GetDlgItem( hdlg, IDC_ACCLIST );
			SendMessage( hList, LB_SETITEMHEIGHT, 0, 17 );

			while( TRUE ) {
				DBVARIANT dbv;
				char buf[30];
				itoa( i++, buf, 10 );
				if ( DBGetContactSettingString( NULL, "Protocols", buf, &dbv ))
					break;

				idx = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM)dbv.pszVal );
				ListBox_SetItemData( hList, idx, (LPARAM)mir_strdup(dbv.pszVal));
				DBFreeVariant( &dbv );
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam)) {
		case IDC_ADD:
		case IDC_REMOVE:
			break;

		case IDC_EDIT:
			{
				HWND hList = GetDlgItem( hdlg, IDC_ACCLIST );
				int idx = ListBox_GetCurSel( hList );
				if ( idx != -1 ) {
					OPENOPTIONSDIALOG ood;
					ood.cbSize = sizeof(ood);
					ood.pszGroup = "Network";
					ood.pszPage = (char*)ListBox_GetItemData( hList, idx );
					ood.pszTab = NULL;
					CallService( MS_OPT_OPENOPTIONS, 0, (LPARAM)&ood );
			}	}
			break;

		case IDOK:
		case IDCANCEL:
			EndDialog(hdlg,LOWORD(wParam));
			break;
		}
		break;

	case WM_DESTROY:
		Window_FreeIcon_IcoLib( hdlg );
		Button_FreeIcon_IcoLib( hdlg, IDC_ADD );
		Button_FreeIcon_IcoLib( hdlg, IDC_EDIT );
		Button_FreeIcon_IcoLib( hdlg, IDC_REMOVE );
		Utils_SaveWindowPosition( hdlg, NULL, "AccMgr", "");
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
