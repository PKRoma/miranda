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

/////////////////////////////////////////////////////////////////////////////////////////
// MSN contact option page dialog procedure.


INT_PTR CALLBACK MsnDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
// MsnOnDetailsInit - initializes user info dialog pages.

int MsnOnDetailsInit( WPARAM wParam, LPARAM lParam )
{
/*
	if ( !MSN_IsMyContact( hContact ))
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
