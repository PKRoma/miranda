/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

 Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "sdk/m_acc.h"

/////////////////////////////////////////////////////////////////////////////////////////
// MSN contact option page dialog procedure.


BOOL CALLBACK MsnDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) 
	{
		case WM_INITDIALOG:
			{
				TranslateDialogDefault( hwndDlg );
				
				const HANDLE hContact = ( HANDLE )lParam;

				char tBuffer[ MAX_PATH ];
				if ( MSN_GetStaticString( "OnMobile", hContact, tBuffer, sizeof( tBuffer )))
					strcpy( tBuffer, "N" );
				SetDlgItemTextA( hwndDlg, IDC_MOBILE, tBuffer );

				if ( MSN_GetStaticString( "OnMsnMobile", hContact, tBuffer, sizeof( tBuffer )))
					strcpy( tBuffer, "N" );
				SetDlgItemTextA( hwndDlg, IDC_MSN_MOBILE, tBuffer );

				DWORD dwFlagBits = MSN_GetDword( hContact, "FlagBits", 0 );
				SetDlgItemTextA( hwndDlg, IDC_WEBMESSENGER, ( dwFlagBits & 0x200 ) ? "Y" : "N" );
			}

			return TRUE;

		case WM_DESTROY:
			break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// AvatarDlgProc - MSN avatar option page dialog procedure.

BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static RECT r;

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );

			HWND hwnd = GetDlgItem(hwndDlg, IDC_AVATAR);

			SendMessage(hwnd, AVATAR_SETPROTOCOL, 0, (LPARAM) msnProtocolName);
			SendMessage(hwnd, AVATAR_SETNOAVATARTEXT, 0, (LPARAM) "Contact has no avatar");
			SendMessage(hwnd, AVATAR_SETBKGCOLOR, 0, (LPARAM) GetSysColor( COLOR_3DFACE ));
			SendMessage(hwnd, AVATAR_SETBORDERCOLOR, 0, (LPARAM) RGB(0,0,0));
		}

		return TRUE;

	case WM_COMMAND:
		if ( HIWORD( wParam ) == BN_CLICKED ) {
			switch( LOWORD( wParam )) {
			case IDC_SETAVATAR:
				MSN_CallService( MS_AV_SETMYAVATAR, ( WPARAM )msnProtocolName, 0 );
				break;

			case IDC_DELETEAVATAR:
				{
					char tFileName[ MAX_PATH ];
					MSN_GetAvatarFileName( NULL, tFileName, sizeof( tFileName ));
					remove( tFileName );
					MSN_DeleteSetting( NULL, "PictObject" );
					MSN_SetServerStatus( msnStatusMode );
					break;
		}	}	}
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnOnDetailsInit - initializes user info dialog pages.

int MsnOnDetailsInit( WPARAM wParam, LPARAM lParam )
{
	if ( !ServiceExists( MS_AV_SETMYAVATAR )) return 0;

	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.hIcon = NULL;
	odp.hInstance = hInst;

	HANDLE hContact = ( HANDLE )lParam;
	if ( hContact == NULL ) {
		char szTitle[256];
		mir_snprintf( szTitle, sizeof( szTitle ), "%s %s", msnProtocolName, MSN_Translate( "Avatar" ));

		odp.pfnDlgProc = AvatarDlgProc;
		odp.position = 1900000000;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_SETAVATAR);
		odp.pszTitle = szTitle;
		MSN_CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
		return 0;
	}

/*
	char* szProto = ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
	if ( lstrcmpA( szProto, msnProtocolName ))
		return 0;

	if ( MSN_GetDword( hContact, "FlagBits", 0 )) {
		odp.pfnDlgProc = MsnDlgProc;
		odp.position = -1900000000;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_USEROPTS);
		odp.pszTitle = MSN_Translate(msnProtocolName);
		MSN_CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
	}
*/	return 0;
}
