/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
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

#include <commdlg.h>
#include <direct.h>

#include "resource.h"
#include "msn_md5.h"

#define STYLE_DEFAULTBGCOLOUR     RGB(173,206,247)

/////////////////////////////////////////////////////////////////////////////////////////
// External data declarations

extern unsigned long sl;
extern char *rru;

BOOL CALLBACK DlgProcMsnServLists(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static void __cdecl sttUploadGroups( void* )
{
	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		if ( !lstrcmpA( msnProtocolName, ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 ))) {
			DBVARIANT dbv;
			if ( !DBGetContactSettingStringUtf( hContact, "CList", "Group", &dbv )) {
				MSN_MoveContactToGroup( hContact, dbv.pszVal );
				MSN_FreeVariant( &dbv );
		}	}

		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN Options dialog procedure

static BOOL CALLBACK DlgProcMsnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG: {
		TranslateDialogDefault( hwndDlg );

		char tBuffer[ MAX_PATH ];
		if ( !MSN_GetStaticString( "e-mail", NULL, tBuffer, sizeof( tBuffer )))
			SetDlgItemTextA( hwndDlg, IDC_HANDLE, tBuffer );

		if ( !MSN_GetStaticString( "Password", NULL, tBuffer, sizeof( tBuffer ))) {
			MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer )+1, ( LPARAM )tBuffer );
			tBuffer[ 16 ] = 0;
			SetDlgItemTextA( hwndDlg, IDC_PASSWORD, tBuffer );
		}
		SendDlgItemMessage( hwndDlg, IDC_PASSWORD, EM_SETLIMITTEXT, 16, 0 );

		HWND wnd = GetDlgItem( hwndDlg, IDC_HANDLE2 );
		DBVARIANT dbv;
		if ( !DBGetContactSettingTString( NULL, msnProtocolName, "Nick", &dbv )) {
			SetWindowText( wnd, dbv.ptszVal );
			MSN_FreeVariant( &dbv );
		}
		if ( !msnLoggedIn )
			EnableWindow( wnd, FALSE );

		CheckDlgButton( hwndDlg, IDC_DISABLE_MAIN_MENU, MSN_GetByte( "DisableSetNickname", 0 ));
		CheckDlgButton( hwndDlg, IDC_ENABLE_AVATARS,    MSN_GetByte( "EnableAvatars", 0 ));
		CheckDlgButton( hwndDlg, IDC_SENDFONTINFO,      MSN_GetByte( "SendFontInfo", 1 ));
		CheckDlgButton( hwndDlg, IDC_USE_OWN_NICKNAME,  MSN_GetByte( "NeverUpdateNickname", 0 ));
		CheckDlgButton( hwndDlg, IDC_AWAY_AS_BRB,       MSN_GetByte( "AwayAsBrb", 0 ));
		CheckDlgButton( hwndDlg, IDC_MANAGEGROUPS,      MSN_GetByte( "ManageServer", 0 ));

		int tValue = MSN_GetByte( "RunMailerOnHotmail", 0 );
		CheckDlgButton( hwndDlg, IDC_RUN_APP_ON_HOTMAIL, tValue );
		EnableWindow( GetDlgItem( hwndDlg, IDC_MAILER_APP ), tValue );
		EnableWindow( GetDlgItem( hwndDlg, IDC_ENTER_MAILER_APP ), tValue );

		if ( !MSN_GetStaticString( "MailerPath", NULL, tBuffer, sizeof( tBuffer )))
			SetDlgItemTextA( hwndDlg, IDC_MAILER_APP, tBuffer );

		if ( !msnLoggedIn ) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_MANAGEGROUPS ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS ), FALSE );
		}
		else CheckDlgButton( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS, msnOtherContactsBlocked );
		return TRUE;
	}

	case WM_COMMAND:
		if ( LOWORD( wParam ) == IDC_NEWMSNACCOUNTLINK ) {
			MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )"http://lc2.law13.hotmail.passport.com/cgi-bin/register" );
			return TRUE;
		}

		if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus()) {
			switch( LOWORD( wParam )) {
			case IDC_HANDLE:			case IDC_PASSWORD:			case IDC_LOGINSERVER:
			case IDC_MSNPORT:			case IDC_YOURHOST:			case IDC_HANDLE2:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		}	}

		if ( HIWORD( wParam ) == BN_CLICKED )
			switch( LOWORD( wParam )) {
			case IDC_ENABLE_AVATARS: {
				BYTE tIsChosen = IsDlgButtonChecked( hwndDlg, IDC_ENABLE_AVATARS );
				if ( tIsChosen && MSN_LoadPngModule() == NULL ) {
					CheckDlgButton( hwndDlg, IDC_ENABLE_AVATARS, 0 );
					break;
				}

            EnableWindow( GetDlgItem( hwndDlg, IDC_SETAVATAR ), tIsChosen );
            EnableWindow( GetDlgItem( hwndDlg, IDC_DELETEAVATAR ), tIsChosen );
			}

			case IDC_DISABLE_MAIN_MENU:			case IDC_SENDFONTINFO:
			case IDC_DISABLE_ANOTHER_CONTACTS:	case IDC_USE_OWN_NICKNAME:
			case IDC_AWAY_AS_BRB:
			LBL_Apply:
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;

			case IDC_MANAGEGROUPS:
				if ( IsDlgButtonChecked( hwndDlg, IDC_MANAGEGROUPS ))
					if ( IDYES == MessageBoxA( hwndDlg,
											MSN_Translate( "Server groups import may change your contact list layout after next login. "
																"Do you want to upload your groups to the server?" ),
											MSN_Translate( "MSN Protocol" ), MB_YESNOCANCEL ))
						(new ThreadData())->startThread( sttUploadGroups );
				goto LBL_Apply;

			case IDC_RUN_APP_ON_HOTMAIL: {
				BYTE tIsChosen = IsDlgButtonChecked( hwndDlg, IDC_RUN_APP_ON_HOTMAIL );
				EnableWindow( GetDlgItem( hwndDlg, IDC_MAILER_APP ), tIsChosen );
				EnableWindow( GetDlgItem( hwndDlg, IDC_ENTER_MAILER_APP ), tIsChosen );
				goto LBL_Apply;
			}

			case IDC_ENTER_MAILER_APP: {
				HWND tEditField = GetDlgItem( hwndDlg, IDC_MAILER_APP );

				char szFile[ MAX_PATH+2 ];
				GetWindowTextA( tEditField, szFile, sizeof( szFile ));

				int tSelectLen = 0;

				if ( szFile[0] == '\"' ) {
					char* p = strchr( szFile+1, '\"' );
					if ( p != NULL ) {
						*p = '\0';
						strdel( szFile, 1 );
						tSelectLen += 2;
						goto LBL_Continue;
				}	}

				{	char* p = strchr( szFile, ' ' );
					if ( p != NULL )
						*p = '\0';
				}
LBL_Continue:
				tSelectLen += strlen( szFile );

				OPENFILENAMEA ofn = { 0 };
				ofn.lStructSize = sizeof( ofn );
				ofn.hwndOwner = hwndDlg;
				ofn.nMaxFile = sizeof( szFile );
				ofn.lpstrFile = szFile;
				ofn.Flags = OFN_FILEMUSTEXIST;
				if ( GetOpenFileNameA( &ofn ) != TRUE )
					break;

				if ( strchr( szFile, ' ' ) != NULL ) {
					char tmpBuf[ MAX_PATH+2 ];
					mir_snprintf( tmpBuf, sizeof( tmpBuf ), "\"%s\"", szFile );
					strcpy( szFile, tmpBuf );
				}

				SendMessage( tEditField, EM_SETSEL, 0, tSelectLen );
				SendMessage( tEditField, EM_REPLACESEL, TRUE, LPARAM( szFile ));
				goto LBL_Apply;
		}	}

		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY ) {
			bool reconnectRequired = false, restartRequired = false;
			TCHAR screenStr[ MAX_PATH ], dbStr[ MAX_PATH ];
			char  password[ 100 ];
			DBVARIANT dbv;

			GetDlgItemText( hwndDlg, IDC_HANDLE, screenStr, sizeof( screenStr ));
			if ( DBGetContactSettingTString( "e-mail", msnProtocolName, NULL, &dbv ))
				dbv.ptszVal = NULL;
			if ( lstrcmp( screenStr, dbStr ))
				reconnectRequired = true;
			MSN_SetStringT( NULL, "e-mail", screenStr );
			MSN_FreeVariant( &dbv );

			GetDlgItemTextA( hwndDlg, IDC_PASSWORD, password, sizeof( password ));
			MSN_CallService( MS_DB_CRYPT_ENCODESTRING, sizeof( password ),( LPARAM )password );
			if ( DBGetContactSetting( "Password", msnProtocolName, NULL, &dbv ))
				dbv.pszVal = NULL;
			if ( lstrcmp( screenStr, dbStr ))
				reconnectRequired = true;
			MSN_SetString( NULL, "Password", password );
			MSN_FreeVariant( &dbv );

			GetDlgItemText( hwndDlg, IDC_HANDLE2, screenStr, sizeof( screenStr ));
			if	( !DBGetContactSettingTString( NULL, msnProtocolName, "Nick", &dbv )) {
				if ( lstrcmp( dbv.ptszVal, screenStr ))
					MSN_SendNicknameT( screenStr );
				MSN_FreeVariant( &dbv );
			}
			MSN_SetStringT( NULL, "Nick", screenStr );

			BYTE tValue = IsDlgButtonChecked( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS );
			if ( tValue != msnOtherContactsBlocked && msnLoggedIn ) {
				msnNsThread->sendPacket( "BLP", ( tValue ) ? "BL" : "AL" );
				break;
			}

			MSN_SetByte( "EnableAvatars", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_ENABLE_AVATARS ));
			MSN_SetByte( "SendFontInfo", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SENDFONTINFO ));
			MSN_SetByte( "RunMailerOnHotmail", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_RUN_APP_ON_HOTMAIL ));
			MSN_SetByte( "NeverUpdateNickname", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USE_OWN_NICKNAME ));
			MSN_SetByte( "DisableSetNickname", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLE_MAIN_MENU ));
			MSN_SetByte( "AwayAsBrb", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AWAY_AS_BRB ));
			MSN_SetByte( "ManageServer", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_MANAGEGROUPS ));

			GetDlgItemText( hwndDlg, IDC_MAILER_APP, screenStr, sizeof( screenStr ));
			MSN_SetStringT( NULL, "MailerPath", screenStr );

			if ( reconnectRequired && msnLoggedIn )
				MessageBoxA( hwndDlg, MSN_Translate( "The changes you have made require you to reconnect to the MSN Messenger network before they take effect"), "MSN Options", MB_OK );

			LoadOptions();
			return TRUE;
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN Connection Options dialog procedure

static BOOL CALLBACK DlgProcMsnConnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;

	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{
			int tUseGateway = MSN_GetByte( "UseGateway", 0 );
			CheckDlgButton( hwndDlg, IDC_USEGATEWAY, tUseGateway );
			if ( tUseGateway ) {
				SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_GATEWAY );
				SetDlgItemInt( hwndDlg, IDC_MSNPORT, 80, FALSE );
			}
			else {
				if ( !DBGetContactSetting( NULL, msnProtocolName, "LoginServer", &dbv )) {
					SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, dbv.pszVal );
					MSN_FreeVariant( &dbv );
				}
				else SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_LOGIN_SERVER );

				SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_GetWord( NULL, "MSNMPort", 1863 ), FALSE );
		}	}

		CheckDlgButton( hwndDlg, IDC_KEEPALIVE,	MSN_GetByte( "KeepAlive",   0 ));
		CheckDlgButton( hwndDlg, IDC_AUTOGETHOST,	MSN_GetByte( "AutoGetHost", 1 ));
		CheckDlgButton( hwndDlg, IDC_USEIEPROXY,  MSN_GetByte( "UseIeProxy",  0 ));
		CheckDlgButton( hwndDlg, IDC_SLOWSEND,    MSN_GetByte( "SlowSend",    0 ));
		CheckDlgButton( hwndDlg, IDC_USEMSNP11,   MSN_GetByte( "UseMSNP11",   0 ));

		if ( MyOptions.UseProxy )
			EnableWindow( GetDlgItem( hwndDlg, IDC_USEOPENSSL ), FALSE );
		else
         CheckDlgButton( hwndDlg, IDC_USEOPENSSL, MSN_GetByte( "UseOpenSSL", 0 ));

		if ( !DBGetContactSetting( NULL, msnProtocolName, "YourHost", &dbv )) {
			if ( !MSN_GetByte( "AutoGetHost", 1 ))
				SetDlgItemTextA( hwndDlg, IDC_YOURHOST, dbv.pszVal );
			else {
				if ( msnExternalIP == NULL ) {
					char ipaddr[ 256 ];
					gethostname( ipaddr, sizeof( ipaddr ));
					SetDlgItemTextA( hwndDlg, IDC_YOURHOST, ipaddr );
				}
				else SetDlgItemTextA( hwndDlg, IDC_YOURHOST, msnExternalIP );
			}
			MSN_FreeVariant( &dbv );
		}
		else {
			char ipaddr[256];
			gethostname( ipaddr, sizeof( ipaddr ));
			SetDlgItemTextA( hwndDlg, IDC_YOURHOST, ipaddr );
		}

		if ( MSN_GetByte( "AutoGetHost", 1 ))
			EnableWindow( GetDlgItem( hwndDlg, IDC_YOURHOST), FALSE );

		if ( MyOptions.UseGateway ) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_LOGINSERVER ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_MSNPORT ), FALSE );
		}
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_RESETSERVER:
			SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_LOGIN_SERVER );
			SetDlgItemInt(  hwndDlg, IDC_MSNPORT,  1863, FALSE );
			goto LBL_Apply;
		}

		if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus())
			switch( LOWORD( wParam )) {
			case IDC_LOGINSERVER:		case IDC_MSNPORT:
			case IDC_YOURHOST:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}

		if ( HIWORD( wParam ) == BN_CLICKED )
			switch( LOWORD( wParam )) {
			case IDC_AUTOGETHOST:
			{	int tValue = IsDlgButtonChecked( hwndDlg, IDC_AUTOGETHOST ) ? FALSE : TRUE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_YOURHOST), tValue );
			}

			case IDC_KEEPALIVE:			case IDC_USEIEPROXY:		case IDC_SLOWSEND:
			case IDC_USEMSNP11:        case IDC_USEOPENSSL:
			LBL_Apply:
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;

			case IDC_USEGATEWAY: {
				bool tValue = !IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY );

				HWND tWindow = GetDlgItem( hwndDlg, IDC_LOGINSERVER );
				if ( !tValue ) {
					SetWindowTextA( tWindow, MSN_DEFAULT_GATEWAY );
					SetDlgItemInt( hwndDlg, IDC_MSNPORT, 80, FALSE );
				}
				else {
					if ( !DBGetContactSetting( NULL, msnProtocolName, "LoginServer", &dbv )) {
						SetWindowTextA( tWindow, dbv.pszVal );
						MSN_FreeVariant( &dbv );
					}
					else SetWindowTextA( tWindow, MSN_DEFAULT_LOGIN_SERVER );

					SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_GetWord( NULL, "MSNMPort", 1863 ), FALSE );
				}

				EnableWindow( tWindow, tValue );
				EnableWindow( GetDlgItem( hwndDlg, IDC_MSNPORT ), tValue );
				goto LBL_Apply;
		}	}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY ) {
			bool restartRequired = false, reconnectRequired = false;
			char str[ MAX_PATH ];

			BYTE tValue = ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY );
			if ( MyOptions.UseGateway != tValue ) {
				MSN_SetByte( "UseGateway", tValue );
				restartRequired = true;
			}
			if ( !tValue ) {
				GetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, str, sizeof( str ));
				MSN_SetString( NULL, "LoginServer", str );

				MSN_SetWord( NULL, "MSNMPort", GetDlgItemInt( hwndDlg, IDC_MSNPORT, NULL, FALSE ));
			}

			tValue = ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEMSNP11 );
			if ( MyOptions.UseMSNP11 != tValue ) {
				MSN_SetByte( "UseMSNP11", tValue );
				if ( msnLoggedIn )
					reconnectRequired = true;
			}

			tValue = ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEOPENSSL );
			if ( MSN_GetByte( "UseOpenSSL", 0 ) != tValue ) {
				MSN_SetByte( "UseOpenSSL", tValue );
				if ( msnLoggedIn )
					reconnectRequired = true;
			}

			MSN_SetByte( "UseIeProxy",    ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEIEPROXY    ));
			MSN_SetByte( "KeepAlive",     ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE     ));
			MSN_SetByte( "AutoGetHost",   ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AUTOGETHOST   ));
			MSN_SetByte( "SlowSend",      ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SLOWSEND      ));

			GetDlgItemTextA( hwndDlg, IDC_YOURHOST, str, sizeof( str ));
			MSN_SetString( NULL, "YourHost", str );

			if ( restartRequired )
				MessageBoxA( hwndDlg, MSN_Translate( "The changes you have made require you to restart Miranda IM before they take effect"), "MSN Options", MB_OK );
			if ( reconnectRequired && msnLoggedIn )
				MessageBoxA( hwndDlg, MSN_Translate( "The changes you have made require you to reconnect to the MSN Messenger network before they take effect"), "MSN Options", MB_OK );

			LoadOptions();
			return TRUE;
	}	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// PopUp Options Dialog: style, position, color, font...

static BOOL CALLBACK DlgProcHotmailPopUpOpts( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static bool bEnabled;

	switch( msg ) {
	case WM_INITDIALOG: {
		TranslateDialogDefault(hwndDlg);
		bEnabled = false;

		//Colours. First step is configuring the colours.
		SendDlgItemMessage( hwndDlg, IDC_BGCOLOUR, CPM_SETCOLOUR, 0, MyOptions.BGColour );
		SendDlgItemMessage( hwndDlg, IDC_TEXTCOLOUR, CPM_SETCOLOUR, 0, MyOptions.TextColour);

		//Second step is disabling them if we want to use default Windows ones.
		CheckDlgButton( hwndDlg, IDC_USEWINCOLORS, MyOptions.UseWinColors ? BST_CHECKED : BST_UNCHECKED );
		EnableWindow( GetDlgItem( hwndDlg, IDC_BGCOLOUR), !MyOptions.UseWinColors );
		EnableWindow( GetDlgItem( hwndDlg, IDC_TEXTCOLOUR), !MyOptions.UseWinColors );

		if ( !ServiceExists( MS_POPUP_ADDPOPUPEX ))
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT ), FALSE );
		else
			SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, MyOptions.PopupTimeoutHotmail, FALSE );

		CheckDlgButton( hwndDlg, IDC_DISABLEHOTMAIL,      MSN_GetByte( "DisableHotmail", 0 ));
		CheckDlgButton( hwndDlg, IDC_DISABLEHOTJUNK,	     MSN_GetByte( "DisableHotmailJunk", 0 ));
		CheckDlgButton( hwndDlg, IDC_NOTIFY_USERTYPE,     MSN_GetByte( "DisplayTyping", 0 ));
		CheckDlgButton( hwndDlg, IDC_NOTIFY_ENDSESSION,   MSN_GetByte( "EnableSessionPopup", 0 ));
		CheckDlgButton( hwndDlg, IDC_NOTIFY_FIRSTMSG,     MSN_GetByte( "EnableDeliveryPopup", 1 ));
		CheckDlgButton( hwndDlg, IDC_ERRORS_USING_POPUPS, MSN_GetByte( "ShowErrorsAsPopups", 0 ));

		int tTimeout = MSN_GetDword( NULL, "PopupTimeout", 3 );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, tTimeout, FALSE );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT2, MSN_GetDword( NULL, "PopupTimeoutOther", tTimeout ), FALSE );
		if ( !ServiceExists( MS_POPUP_ADDPOPUPEX )) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT2 ), FALSE );
		}
		bEnabled = true;
		return TRUE;
	}
	case WM_COMMAND:
		switch( LOWORD( wParam )) {
		case IDC_DISABLEHOTMAIL: {
			HWND wnd = GetDlgItem( hwndDlg, IDC_DISABLEHOTJUNK );
			BOOL toSet;
			if ( SendMessage( HWND( lParam ), BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
				SendMessage( wnd, BM_GETCHECK, BST_CHECKED, 0 );
				toSet = FALSE;
			}
			else toSet = TRUE;

			EnableWindow( wnd, toSet );
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT ), toSet );
			EnableWindow( GetDlgItem( hwndDlg, IDC_PREVIEW ), toSet );
		}

		case IDC_DISABLEHOTJUNK:
		case IDC_NOTIFY_USERTYPE:
		case IDC_NOTIFY_ENDSESSION:
		case IDC_POPUP_TIMEOUT:
		case IDC_POPUP_TIMEOUT2:
		case IDC_NOTIFY_FIRSTMSG:
		case IDC_ERRORS_USING_POPUPS:
			if ( bEnabled )
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;

		case IDC_BGCOLOUR: //Fall through
		case IDC_TEXTCOLOUR:
			if ( HIWORD( wParam ) == CPN_COLOURCHANGED ) {
				MyOptions.BGColour = SendDlgItemMessage( hwndDlg, IDC_BGCOLOUR, CPM_GETCOLOUR, 0, 0 );
				MyOptions.TextColour = SendDlgItemMessage( hwndDlg, IDC_TEXTCOLOUR, CPM_GETCOLOUR, 0, 0 );
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_USEWINCOLORS:
			MyOptions.UseWinColors = IsDlgButtonChecked( hwndDlg, IDC_USEWINCOLORS );

			EnableWindow( GetDlgItem( hwndDlg, IDC_BGCOLOUR ), !( MyOptions.UseWinColors ));
			EnableWindow( GetDlgItem( hwndDlg, IDC_TEXTCOLOUR ), !( MyOptions.UseWinColors ));
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;

		case IDC_PREVIEW:
			MSN_ShowPopup( MSN_Translate( "A New Hotmail has come!" ), MSN_Translate( "Test: Arrival Hotmail" ), MSN_HOTMAIL_POPUP );
			break;

		case IDC_PREVIEW2:
			MSN_ShowPopup( "vasya.pupkin@hotmail.com", MSN_Translate( "First message delivered" ), 0 );
			break;
		}
		break;

	case WM_NOTIFY: //Here we have pressed either the OK or the APPLY button.
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_RESET:
				LoadOptions();
				return TRUE;

			case PSN_APPLY:
				MyOptions.TextColour = SendDlgItemMessage(hwndDlg,IDC_TEXTCOLOUR,CPM_GETCOLOUR,0,0);
				DBWriteContactSettingDword(NULL, ModuleName, "TextColour",MyOptions.TextColour);

				MyOptions.BGColour = SendDlgItemMessage(hwndDlg,IDC_BGCOLOUR,CPM_GETCOLOUR,0,0);
				DBWriteContactSettingDword(NULL, ModuleName, "BackgroundColour",MyOptions.BGColour);

				if ( ServiceExists( MS_POPUP_ADDPOPUPEX )) {
					MyOptions.PopupTimeoutHotmail = GetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, NULL, FALSE );
					MSN_SetDword( NULL, "PopupTimeout", MyOptions.PopupTimeoutHotmail );

					MyOptions.PopupTimeoutOther = GetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT2, NULL, FALSE );
					MSN_SetDword( NULL, "PopupTimeoutOther", MyOptions.PopupTimeoutOther );
				}

				MyOptions.ShowErrorsAsPopups = ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_ERRORS_USING_POPUPS );
				MSN_SetByte( "ShowErrorsAsPopups", MyOptions.ShowErrorsAsPopups );

				MSN_SetByte( "UseWinColors", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEWINCOLORS ));
				MSN_SetByte( "DisplayTyping", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_NOTIFY_USERTYPE ));
				MSN_SetByte( "DisableHotmail", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLEHOTMAIL ));
				MSN_SetByte( "DisableHotmailJunk",( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLEHOTJUNK ));
				MSN_SetByte( "EnableDeliveryPopup", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_NOTIFY_FIRSTMSG ));
				MSN_SetByte( "EnableSessionPopup", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_NOTIFY_ENDSESSION ));
				return TRUE;
			}
			break;
		}
		break;
	}

	return FALSE;
}

static HWND hwndAcc = 0, hwndConn = 0, hwndSrvList = 0; 

static void SetOptionsDlgToType(HWND hwnd, int iExpert)
{
	TCITEM tci;
	RECT rcClient;
	HWND hwndTab = GetDlgItem(hwnd, IDC_OPTIONSTAB), hwndEnum;
	int iPages = 0;

	if(!hwndAcc)
		hwndAcc = CreateDialog(hInst,MAKEINTRESOURCE(IDD_OPT_MSN), hwnd, DlgProcMsnOpts);

	hwndEnum = GetWindow(hwndAcc, GW_CHILD);
	
	while(hwndEnum) {
		ShowWindow(hwndEnum, iExpert ? SW_SHOW : SW_HIDE);
		hwndEnum = GetWindow(hwndEnum, GW_HWNDNEXT);
	}
	if(!iExpert) {
		hwndEnum = GetDlgItem(hwndAcc, IDC_STMSNGROUP);
		ShowWindow(hwndEnum, SW_SHOW);
		hwndEnum = GetWindow(hwndEnum, GW_HWNDNEXT);
		do {
			ShowWindow(hwndEnum, SW_SHOW);
			hwndEnum = GetWindow(hwndEnum, GW_HWNDNEXT);
		} while(hwndEnum && hwndEnum != GetDlgItem(hwndAcc, IDC_NEWMSNACCOUNTLINK));
	}
	ShowWindow(hwndEnum, SW_SHOW);
	GetClientRect(hwnd, &rcClient);
	TabCtrl_DeleteAllItems(hwndTab);

	tci.mask = TCIF_PARAM|TCIF_TEXT;
	tci.lParam = (LPARAM)hwndAcc;
	tci.pszText = TranslateT("Account");
	TabCtrl_InsertItem(hwndTab, 0, &tci);
	MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
	iPages++;

	if(!hwndConn)
		hwndConn = CreateDialog(hInst,MAKEINTRESOURCE(IDD_OPT_MSN_CONN),hwnd,DlgProcMsnConnOpts);

	if(!hwndSrvList)
		hwndSrvList = CreateDialog(hInst,MAKEINTRESOURCE(IDD_LISTSMGR),hwnd,DlgProcMsnServLists);

	ShowWindow(hwndConn, SW_HIDE);
	ShowWindow(hwndSrvList, SW_HIDE);
	ShowWindow(hwndAcc, SW_SHOW);

	if(iExpert) {
		tci.lParam = (LPARAM)hwndConn;
		tci.pszText = TranslateT("Connection");
		TabCtrl_InsertItem(hwndTab, iPages++, &tci);
		MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);

		tci.lParam = (LPARAM)hwndSrvList;
		tci.pszText = TranslateT("Server list");
		TabCtrl_InsertItem(hwndTab, iPages++, &tci);
		MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
	}
	TabCtrl_SetCurSel(hwndTab, 0);
}

// handle tabbed options dialog

static BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   
	switch(msg) {
	case WM_INITDIALOG:
		{
			iInit = TRUE;
			int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
			SetOptionsDlgToType(hwnd, iExpert);
			iInit = FALSE;
			return FALSE;
	}
	case WM_DESTROY:
		hwndAcc = hwndConn = hwndSrvList = 0;
		break;
	case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
		if(!iInit)
			SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				{
					TCITEM tci;
					int i,count = TabCtrl_GetItemCount(GetDlgItem(hwnd,IDC_OPTIONSTAB));
					tci.mask = TCIF_PARAM;
					for (i=0;i<count;i++) {
					  TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),i,&tci);
					  SendMessage((HWND)tci.lParam,WM_NOTIFY,0,lParam);
				}	}
				break;
			case PSN_EXPERTCHANGED:
				{
					int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
					SetOptionsDlgToType(hwnd, iExpert);
					break;
			}	}
			break;
		case IDC_OPTIONSTAB:
			switch (((LPNMHDR)lParam)->code) {
			case TCN_SELCHANGING:
				{
					TCITEM tci;
					tci.mask = TCIF_PARAM;
					TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
					ShowWindow((HWND)tci.lParam,SW_HIDE);                     
				  break;
				}
			case TCN_SELCHANGE:
				{
					TCITEM tci;
					tci.mask = TCIF_PARAM;
					TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
					ShowWindow((HWND)tci.lParam,SW_SHOW);                     
					break;
			}	}
			break;
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Initialize options pages

int MsnOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize      = sizeof(odp);
	odp.position    = -790000000;
	odp.hInstance   = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSNMAIN);
	odp.pszTitle    = msnProtocolName;
	odp.pszGroup    = "Network";
	odp.flags       = ODPF_BOLDGROUPS;
	odp.pfnDlgProc  = OptionsDlgProc;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	if ( ServiceExists( MS_POPUP_ADDPOPUP )) {
		memset( &odp, 0, sizeof( odp ));
		odp.cbSize			= sizeof( odp );
		odp.position		= 100000000;
		odp.hInstance		= hInst;
		odp.pszTemplate	= MAKEINTRESOURCEA( IDD_HOTMAIL_OPT_POPUP );
		odp.pszTitle		= msnProtocolName;
		odp.pszGroup		= "Popups";
		odp.groupPosition	= 910000000;
		odp.flags			= ODPF_BOLDGROUPS;
		odp.pfnDlgProc		= DlgProcHotmailPopUpOpts;
		MSN_CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Load resident option values into memory

void __stdcall LoadOptions()
{
	memset( &MyOptions, 0, sizeof( MyOptions ));

	//PopUp Options
	MyOptions.BGColour =
		DBGetContactSettingDword( NULL, ModuleName, "BackgroundColour", STYLE_DEFAULTBGCOLOUR );
	MyOptions.TextColour =
		DBGetContactSettingDword( NULL, ModuleName, "TextColour", GetSysColor( COLOR_WINDOWTEXT ));

	MyOptions.AwayAsBrb = MSN_GetByte( "AwayAsBrb", FALSE );
	MyOptions.DisableMenu = MSN_GetByte( "DisableSetNickname", FALSE );
	MyOptions.EnableAvatars = MSN_GetByte( "EnableAvatars", FALSE );
	MyOptions.KeepConnectionAlive = MSN_GetByte( "KeepAlive", FALSE );
	MyOptions.ManageServer = MSN_GetByte( "ManageServer", FALSE );
	MyOptions.PopupTimeoutHotmail = MSN_GetDword( NULL, "PopupTimeout", 3 );
	MyOptions.PopupTimeoutOther = MSN_GetDword( NULL, "PopupTimeoutOther", MyOptions.PopupTimeoutHotmail );
	MyOptions.ShowErrorsAsPopups = MSN_GetByte( "ShowErrorsAsPopups", FALSE );
	MyOptions.SlowSend = MSN_GetByte( "SlowSend", FALSE );
	MyOptions.UseMSNP11 = MSN_GetByte( "UseMSNP11", FALSE );
	MyOptions.UseProxy = MSN_GetByte( "NLUseProxy", FALSE );
	MyOptions.UseGateway = MSN_GetByte( "UseGateway", FALSE );
	MyOptions.UseWinColors = MSN_GetByte( "UseWinColors", FALSE );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Display Hotmail Inbox thread

DWORD WINAPI MsnShowMailThread( LPVOID )
{
	DBVARIANT dbv;

	DBGetContactSetting( NULL, msnProtocolName, "e-mail", &dbv );
	char* email = ( char* )alloca( strlen( dbv.pszVal )*3 );
	UrlEncode( dbv.pszVal, email, strlen( dbv.pszVal )*3 );
	MSN_FreeVariant( &dbv );

	DBGetContactSetting( NULL, msnProtocolName, "Password", &dbv );
	MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
	char* passwd = ( char* )alloca( strlen( dbv.pszVal )*3 );
	UrlEncode( dbv.pszVal, passwd, strlen( dbv.pszVal )*3 );
	MSN_FreeVariant( &dbv );

	// for hotmail access

	char* Path = getenv( "TEMP" );
	if ( Path == NULL )
		Path = getenv( "TMP" );

	char tPathName[ MAX_PATH ];
	if ( Path == NULL ) {
		MSN_DebugLog( "Temporary file is created in the current directory: %s",
			getcwd( tPathName, sizeof( tPathName )));
	}
	else {
		MSN_DebugLog( "Temporary path found: %s", Path );
		strcpy( tPathName, Path );
	}

	strcat( tPathName, "\\stdtemp.html" );
	MSN_DebugLog( "Opening temporary file '%s'", tPathName );

	FILE* fp = fopen( tPathName, "w" );
	if ( fp == NULL ) {
		MSN_DebugLog( "Opening failed" );
		return 0;
	}

	char hippy[ 2048 ];
	long challen = mir_snprintf( hippy, sizeof( hippy ), "%s%lu%s", MSPAuth, time(NULL) - sl, passwd );

	//Digest it
	unsigned char digest[16];
	MD5_CTX context;
	MD5Init( &context);
	MD5Update( &context, ( BYTE* )hippy, challen );
	MD5Final( digest, &context );

	fprintf( fp, "<html>\n" );
	fprintf( fp, "<head>\n" );
	fprintf( fp, "<noscript>\n" );
	fprintf( fp, "<meta http-equiv=Refresh content=\"0; url=http://www.hotmail.com\">\n" );
	fprintf( fp, "</noscript>\n" );
	fprintf( fp, "</head>\n\n" );

	fprintf( fp, "<body onload=\"document.pform.submit(); \">\n" );
	fprintf( fp, "<form name=\"pform\" action=\"%s\" method=\"POST\">\n\n", passport);
	fprintf( fp, "<input type=\"hidden\" name=\"mode\" value=\"ttl\">\n" );
	fprintf( fp, "<input type=\"hidden\" name=\"login\" value=\"%s\">\n", email);
	fprintf( fp, "<input type=\"hidden\" name=\"username\" value=\"%s\">\n", email);
	fprintf( fp, "<input type=\"hidden\" name=\"sid\" value=\"%s\">\n", sid);
	fprintf( fp, "<input type=\"hidden\" name=\"kv\" value=\"%s\">\n", kv);
	fprintf( fp, "<input type=\"hidden\" name=\"id\" value=\"2\">\n" );
	fprintf( fp, "<input type=\"hidden\" name=\"sl\" value=\"%ld\">\n", time(NULL) - sl);
	fprintf( fp, "<input type=\"hidden\" name=\"rru\" value=\"%s\">\n", rru);
	fprintf( fp, "<input type=\"hidden\" name=\"auth\" value=\"%s\">\n", MSPAuth);
	fprintf( fp, "<input type=\"hidden\" name=\"creds\" value=\"%08x%08x%08x%08x\">\n", htonl(*(PDWORD)(digest+0)),htonl(*(PDWORD)(digest+4)),htonl(*(PDWORD)(digest+8)),htonl(*(PDWORD)(digest+12)));
	fprintf( fp, "<input type=\"hidden\" name=\"svc\" value=\"mail\">\n" );
	fprintf( fp, "<input type=\"hidden\" name=\"js\" value=\"yes\">\n" );
	fprintf( fp, "</form></body>\n" );
	fprintf( fp, "</html>\n" );
	fclose( fp );

	char url[ 256 ];
	if ( rru && passport )
		mir_snprintf( url, sizeof( url ), "file://%s", tPathName );
	else
		strcpy( url, "http://go.msn.com/0/1" );

	MSN_DebugLog( "Starting URL: '%s'", url );
	MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )url );
	Sleep( 15000 );
	DeleteFileA( tPathName );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Popup plugin window proc

LRESULT CALLBACK NullWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_COMMAND: {
			void* tData = PUGetPluginData( hWnd );
			if ( tData != NULL ) {
				DWORD tThreadID;
				CreateThread( NULL, 0, MsnShowMailThread, hWnd, 0, &tThreadID );
				PUDeletePopUp( hWnd );
			}
			break;
		}

		case WM_CONTEXTMENU:
			PUDeletePopUp( hWnd );
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
