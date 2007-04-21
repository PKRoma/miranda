/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

This file uses the 'Webhost sample' code
Copyright(C) 2002 Chris Becke (http://www.mvps.org/user32/webhost.cab)

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

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include "resource.h"

BOOL  MSN_LoadPngModule()
{
	if ( !ServiceExists( MS_AV_SETMYAVATAR )) {
		MessageBox( NULL, TranslateT( "MSN protocol requires the loadavatars plugin to handle avatars"), _T("MSN"), MB_OK );
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN contact option page dialog procedure.

struct MsnDlgProcData
{
	HANDLE hContact;
	HANDLE hEventHook;
};

#define HM_REBIND_AVATAR  ( WM_USER + 1024 )

BOOL CALLBACK MsnDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{
			MsnDlgProcData* pData = new MsnDlgProcData;
			pData->hContact = ( HANDLE )lParam;
			pData->hEventHook = HookEventMessage( ME_PROTO_ACK, hwndDlg, HM_REBIND_AVATAR );
			SetWindowLong( hwndDlg, GWL_USERDATA, LONG( pData ));

			char tBuffer[ MAX_PATH ];
			if ( MSN_GetStaticString( "OnMobile", pData->hContact, tBuffer, sizeof tBuffer ))
				strcpy( tBuffer, "N" );
			SetDlgItemTextA( hwndDlg, IDC_MOBILE, tBuffer );

			if ( MSN_GetStaticString( "OnMsnMobile", pData->hContact, tBuffer, sizeof tBuffer ))
				strcpy( tBuffer, "N" );
			SetDlgItemTextA( hwndDlg, IDC_MSN_MOBILE, tBuffer );

			DWORD dwFlagBits = MSN_GetDword( pData->hContact, "FlagBits", 0 );
			SetDlgItemTextA( hwndDlg, IDC_WEBMESSENGER, ( dwFlagBits & 0x200 ) ? "Y" : "N" );

			if ( MyOptions.EnableAvatars ) {
				MSN_GetAvatarFileName(( HANDLE )lParam, tBuffer, sizeof tBuffer );

				if ( access( tBuffer, 0 )) {
LBL_Reread:		p2p_invite( pData->hContact, MSN_APPID_AVATAR );
					return TRUE;
				}

				char tSavedContext[ 256 ], tNewContext[ 256 ];
				if ( MSN_GetStaticString( "PictContext", pData->hContact, tNewContext, sizeof( tNewContext )) ||
					  MSN_GetStaticString( "PictSavedContext", pData->hContact, tSavedContext, sizeof( tSavedContext )))
					goto LBL_Reread;

				if ( stricmp( tNewContext, tSavedContext ))
					goto LBL_Reread;

				SendDlgItemMessageA( hwndDlg, IDC_MSN_PICT, STM_SETIMAGE, IMAGE_BITMAP,
					( LPARAM )LoadImageA( ::GetModuleHandle(NULL), tBuffer, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE ));
		}	}
		return TRUE;

	case HM_REBIND_AVATAR:
		{	MsnDlgProcData* pData = ( MsnDlgProcData* )GetWindowLong( hwndDlg, GWL_USERDATA );

			ACKDATA* ack = ( ACKDATA* )lParam;
			if ( ack->type == ACKTYPE_AVATAR && ack->hContact == pData->hContact ) {
				if ( ack->result == ACKRESULT_SUCCESS ) {
					PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )ack->hProcess;
					SendDlgItemMessageA( hwndDlg, IDC_MSN_PICT, STM_SETIMAGE, IMAGE_BITMAP,
						( LPARAM )LoadImageA( ::GetModuleHandle(NULL), AI->filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE ));
				}
				else if ( ack->result == ACKRESULT_STATUS )
					p2p_invite( ack->hContact, MSN_APPID_AVATAR );
		}	}
		break;

	case WM_DESTROY:
		MsnDlgProcData* pData = ( MsnDlgProcData* )GetWindowLong( hwndDlg, GWL_USERDATA );
		UnhookEvent( pData->hEventHook );
		delete pData;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// AvatarDlgProc - MSN avatar option page dialog procedure.

static HWND hAvatarDlg = NULL;

int OnSaveMyAvatar( WPARAM wParam, LPARAM lParam )
{
	if ( !lstrcmpA(( char* )wParam, msnProtocolName ) && hAvatarDlg ) {
		AVATARCACHEENTRY* ace = ( AVATARCACHEENTRY* )lParam;
		HBITMAP hAvatar = ( HBITMAP )SendDlgItemMessage( hAvatarDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (WPARAM)ace->hbmPic );
		if ( hAvatar != NULL )
			DeleteObject( hAvatar );
	}

	return 0;
}

BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static RECT r;

	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );

		if ( MyOptions.EnableAvatars ) {
			hAvatarDlg = hwndDlg;
			if ( MSN_LoadPngModule() ) {
				char tBuffer[ MAX_PATH ];
				MSN_GetAvatarFileName( NULL, tBuffer, sizeof tBuffer );
				HBITMAP hAvatar = ( HBITMAP )MSN_CallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )tBuffer );
				if ( hAvatar != NULL )
		         SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (WPARAM)hAvatar );
			}
			else {
				MSN_SetByte( "EnableAvatars", 0 );
				MyOptions.EnableAvatars = FALSE;
		}	}
		return TRUE;

	case WM_COMMAND:
		if ( HIWORD( wParam ) == BN_CLICKED ) {
			switch( LOWORD( wParam )) {
			case IDC_SETAVATAR:
				MSN_CallService( MS_AV_SETMYAVATAR, ( WPARAM )msnProtocolName, 0 );
				break;

			case IDC_DELETEAVATAR:
				{
					HBITMAP hAvatar = ( HBITMAP )SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, 0 );
					if ( hAvatar != NULL )
						DeleteObject( hAvatar );

					char tFileName[ MAX_PATH ];
					MSN_GetAvatarFileName( NULL, tFileName, sizeof tFileName );
					DeleteFileA( tFileName );
					MSN_DeleteSetting( NULL, "PictObject" );
					InvalidateRect( hwndDlg, NULL, TRUE );
					break;
		}	}	}
		break;

	case WM_DESTROY:
		{
			HBITMAP hAvatar = ( HBITMAP )SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, 0 );
			if ( hAvatar != NULL )
				DeleteObject( hAvatar );

			hAvatarDlg = NULL;
		}
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnOnDetailsInit - initializes user info dialog pages.

int MsnOnDetailsInit( WPARAM wParam, LPARAM lParam )
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.hIcon = NULL;
	odp.hInstance = hInst;

	HANDLE hContact = ( HANDLE )lParam;
	if ( hContact == NULL ) {
		if ( MyOptions.EnableAvatars ) {
			char szTitle[256];
			mir_snprintf( szTitle, sizeof( szTitle ), "%s %s", msnProtocolName, MSN_Translate( "Avatar" ));

			odp.pfnDlgProc = AvatarDlgProc;
			odp.position = 1900000000;
			odp.pszTemplate = MAKEINTRESOURCEA(IDD_SETAVATAR);
			odp.pszTitle = szTitle;
			MSN_CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
		}
		return 0;
	}

	char* szProto = ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
	if ( lstrcmpA( szProto, msnProtocolName ))
		return 0;

	if ( MyOptions.EnableAvatars && MSN_GetDword( hContact, "FlagBits", 0 )) {
		odp.pfnDlgProc = MsnDlgProc;
		odp.position = -1900000000;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_USEROPTS);
		odp.pszTitle = MSN_Translate(msnProtocolName);
		MSN_CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
	}
	return 0;
}
