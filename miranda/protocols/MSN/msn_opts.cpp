/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright(C) 2002-2004 George Hazan (modification) and Richard Hughes (original)

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

#include <Commdlg.h>
#include <direct.h>

#include "resource.h"

#include "msn_md5.h"

#define STYLE_DEFAULTBGCOLOUR     RGB(173,206,247)

/////////////////////////////////////////////////////////////////////////////////////////
// External data declarations

extern unsigned long sl;
extern char *rru;

BOOL CALLBACK DlgProcMsnServLists(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/////////////////////////////////////////////////////////////////////////////////////////
// MSN Options dialog procedure

static BOOL CALLBACK DlgProcMsnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG: {
		TranslateDialogDefault( hwndDlg );

		char tBuffer[ 256 ];
		if ( !MSN_GetStaticString( "e-mail", NULL, tBuffer, sizeof( tBuffer )))
			SetDlgItemText( hwndDlg, IDC_HANDLE, tBuffer );

		if ( !MSN_GetStaticString( "Password", NULL, tBuffer, sizeof( tBuffer ))) {
			MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer )+1, ( LPARAM )tBuffer );
			SetDlgItemText( hwndDlg, IDC_PASSWORD, tBuffer );
		}

		if ( !MSN_GetStaticString( "Nick", NULL, tBuffer, sizeof( tBuffer )))
			SetDlgItemText( hwndDlg, IDC_HANDLE2, tBuffer );

		CheckDlgButton( hwndDlg, IDC_DISABLE_MAIN_MENU,	MSN_GetByte( "DisableSetNickname", 0 ));
		CheckDlgButton( hwndDlg, IDC_SENDFONTINFO,		MSN_GetByte( "SendFontInfo", 1 ));
		CheckDlgButton( hwndDlg, IDC_USE_OWN_NICKNAME,	MSN_GetByte( "NeverUpdateNickname", 0 ));
		CheckDlgButton( hwndDlg, IDC_ENABLE_AVATARS,		MSN_GetByte( "EnableAvatars", 0 ));

		int tValue = MSN_GetByte( "RunMailerOnHotmail", 0 );
		CheckDlgButton( hwndDlg, IDC_RUN_APP_ON_HOTMAIL, tValue );
		EnableWindow( GetDlgItem( hwndDlg, IDC_MAILER_APP ), tValue );
		EnableWindow( GetDlgItem( hwndDlg, IDC_ENTER_MAILER_APP ), tValue );

		if ( !MSN_GetStaticString( "MailerPath", NULL, tBuffer, sizeof( tBuffer )))
			SetDlgItemText( hwndDlg, IDC_MAILER_APP, tBuffer );

		if ( !msnLoggedIn )
			EnableWindow( GetDlgItem( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS ), FALSE );
		else
			CheckDlgButton( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS, msnOtherContactsBlocked );
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
			case IDC_ENABLE_AVATARS:
				if ( IsDlgButtonChecked( hwndDlg, IDC_ENABLE_AVATARS ))
					if ( MSN_LoadPngModule() == NULL )
						CheckDlgButton( hwndDlg, IDC_ENABLE_AVATARS, 0 );

			case IDC_DISABLE_MAIN_MENU:			case IDC_SENDFONTINFO:		
			case IDC_DISABLE_ANOTHER_CONTACTS:	case IDC_USE_OWN_NICKNAME:
			LBL_Apply:
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;

			case IDC_RUN_APP_ON_HOTMAIL: {
				BYTE tIsChosen = IsDlgButtonChecked( hwndDlg, IDC_RUN_APP_ON_HOTMAIL );
				EnableWindow( GetDlgItem( hwndDlg, IDC_MAILER_APP ), tIsChosen );
				EnableWindow( GetDlgItem( hwndDlg, IDC_ENTER_MAILER_APP ), tIsChosen );
				goto LBL_Apply;
			}

			case IDC_ENTER_MAILER_APP: {
				HWND tEditField = GetDlgItem( hwndDlg, IDC_MAILER_APP );

				char szFile[ MAX_PATH+2 ];
				GetWindowText( tEditField, szFile, sizeof( szFile ));

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

				OPENFILENAME ofn = { 0 };
				ofn.lStructSize = sizeof( ofn );
				ofn.hwndOwner = hwndDlg;
				ofn.nMaxFile = sizeof( szFile );
				ofn.lpstrFile = szFile;
				ofn.Flags = OFN_FILEMUSTEXIST;
				if ( GetOpenFileName( &ofn ) != TRUE )
					break;

				if ( strchr( szFile, ' ' ) != NULL ) {
					char tmpBuf[ MAX_PATH+2 ];
					_snprintf( tmpBuf, sizeof( tmpBuf ), "\"%s\"", szFile );
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
			char screenStr[ MAX_PATH ], dbStr[ MAX_PATH ];
			char tEmail[ MSN_MAX_EMAIL_LEN ];

			GetDlgItemText( hwndDlg, IDC_HANDLE, tEmail, sizeof( tEmail ));
			if ( !MSN_GetStaticString( "e-mail", NULL, dbStr, sizeof( dbStr )))
				if ( strcmp( tEmail, dbStr ))
					reconnectRequired = true;
			MSN_SetString( NULL, "e-mail", tEmail );

			GetDlgItemText( hwndDlg, IDC_PASSWORD, screenStr, sizeof( screenStr ));
			MSN_CallService( MS_DB_CRYPT_ENCODESTRING, sizeof( screenStr ),( LPARAM )screenStr );
			if ( !MSN_GetStaticString( "Password", NULL, dbStr, sizeof( dbStr )))
				if ( strcmp( screenStr, dbStr ))
					reconnectRequired = true;
			MSN_SetString( NULL, "Password", screenStr );

			GetDlgItemText( hwndDlg, IDC_HANDLE2, screenStr, sizeof( screenStr ));
			if ( !MSN_GetStaticString( "Nick", NULL, dbStr, sizeof( dbStr ))) {
				if ( strcmp( screenStr, dbStr )) {
					reconnectRequired = true;

					if ( msnLoggedIn )
						MSN_SendNickname( tEmail, screenStr );
			}	}
			MSN_SetString( NULL, "Nick", screenStr );

			BYTE tValue = ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLE_MAIN_MENU );
			if ( MyOptions.DisableMenu != tValue )
				MSN_SetByte( "DisableSetNickname", tValue );

			tValue  = IsDlgButtonChecked( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS );
			if ( tValue != msnOtherContactsBlocked && msnLoggedIn ) {
				MSN_SendPacket( msnNSSocket, "BLP", ( tValue ) ? "BL" : "AL" );
				break;
			}

			MSN_SetByte( "SendFontInfo", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SENDFONTINFO ));
			MSN_SetByte( "RunMailerOnHotmail", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_RUN_APP_ON_HOTMAIL ));
			MSN_SetByte( "NeverUpdateNickname", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USE_OWN_NICKNAME ));
			MSN_SetByte( "EnableAvatars", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_ENABLE_AVATARS ));

			GetDlgItemText( hwndDlg, IDC_MAILER_APP, screenStr, sizeof( screenStr ));
			MSN_SetString( NULL, "MailerPath", screenStr );

			if ( reconnectRequired && msnLoggedIn )
				MessageBox( hwndDlg, MSN_Translate( "The changes you have made require you to reconnect to the MSN Messenger network before they take effect"), "MSN Options", MB_OK );

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
				SetDlgItemText( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_GATEWAY );
				SetDlgItemInt( hwndDlg, IDC_MSNPORT, 80, FALSE );
			}
			else {
				if ( !DBGetContactSetting( NULL, msnProtocolName, "LoginServer", &dbv )) {
					SetDlgItemText( hwndDlg, IDC_LOGINSERVER, dbv.pszVal );
					MSN_FreeVariant( &dbv );
				}
				else SetDlgItemText( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_LOGIN_SERVER );

				SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_GetWord( NULL, "MSNMPort", 1863 ), FALSE );
		}	}

		CheckDlgButton( hwndDlg, IDC_KEEPALIVE,	MSN_GetByte( "KeepAlive",   0 ));
		CheckDlgButton( hwndDlg, IDC_AUTOGETHOST,	MSN_GetByte( "AutoGetHost", 1 ));
		CheckDlgButton( hwndDlg, IDC_USEIEPROXY,  MSN_GetByte( "UseIeProxy",  0 ));

		if ( MSN_CallService( MS_SYSTEM_GETVERSION, 0, 0 ) < 0x00030300 )
			EnableWindow( GetDlgItem( hwndDlg, IDC_USEGATEWAY ), FALSE );

		if ( !DBGetContactSetting( NULL, msnProtocolName, "YourHost", &dbv )) {
			if ( !MSN_GetByte( "AutoGetHost", 1 ))
				SetDlgItemText( hwndDlg, IDC_YOURHOST, dbv.pszVal );
			else {
				if ( msnExternalIP == NULL ) {
					char ipaddr[ 256 ];
					gethostname( ipaddr, sizeof( ipaddr ));
					SetDlgItemText( hwndDlg, IDC_YOURHOST, ipaddr );
				}
				else SetDlgItemText( hwndDlg, IDC_YOURHOST, msnExternalIP );
			}
			MSN_FreeVariant( &dbv );
		}
		else {
			char ipaddr[256];
			gethostname( ipaddr, sizeof( ipaddr ));
			SetDlgItemText( hwndDlg, IDC_YOURHOST, ipaddr );
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
			SetDlgItemText( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_LOGIN_SERVER );
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

			case IDC_KEEPALIVE:			case IDC_USEIEPROXY:
			LBL_Apply:
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;

			case IDC_USEGATEWAY: {
				bool tValue = !IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY );

				HWND tWindow = GetDlgItem( hwndDlg, IDC_LOGINSERVER );
				if ( !tValue ) {
					SetWindowText( tWindow, MSN_DEFAULT_GATEWAY );
					SetDlgItemInt( hwndDlg, IDC_MSNPORT, 80, FALSE );
				}
				else {
					if ( !DBGetContactSetting( NULL, msnProtocolName, "LoginServer", &dbv )) {
						SetWindowText( tWindow, dbv.pszVal );
						MSN_FreeVariant( &dbv );
					}
					else SetWindowText( tWindow, MSN_DEFAULT_LOGIN_SERVER );

					SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_GetWord( NULL, "MSNMPort", 1863 ), FALSE );
				}

				EnableWindow( tWindow, tValue );
				EnableWindow( GetDlgItem( hwndDlg, IDC_MSNPORT ), tValue );
				goto LBL_Apply;
		}	}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY ) {
			bool restartRequired = false;
			char str[ MAX_PATH ];

			BYTE tValue = ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY );
			if ( MyOptions.UseGateway != tValue ) {
				MSN_SetByte( "UseGateway", tValue );
				restartRequired = true;
			}
			if ( !tValue ) {
				GetDlgItemText( hwndDlg, IDC_LOGINSERVER, str, sizeof( str ));
				MSN_SetString( NULL, "LoginServer", str );

				MSN_SetWord( NULL, "MSNMPort", GetDlgItemInt( hwndDlg, IDC_MSNPORT, NULL, FALSE ));
			}

			MSN_SetByte( "UseIeProxy",  ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEIEPROXY  ));
			MSN_SetByte( "KeepAlive",   ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE   ));
			MSN_SetByte( "AutoGetHost", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AUTOGETHOST ));

			GetDlgItemText( hwndDlg, IDC_YOURHOST, str, sizeof( str ));
			MSN_SetString( NULL, "YourHost", str );

			if ( restartRequired )
				MessageBox( hwndDlg, MSN_Translate( "The changes you have made require you to restart Miranda IM before they take effect"), "MSN Options", MB_OK );

			LoadOptions();
			return TRUE;
	}	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// PopUp Options Dialog: style, position, color, font...

static BOOL CALLBACK DlgProcHotmailPopUpOpts( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) {
	case WM_INITDIALOG: {
		TranslateDialogDefault(hwndDlg);

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
		CheckDlgButton( hwndDlg, IDC_NOTIFY_FIRSTMSG,     MSN_GetByte( "EnableDeliveryPopup", 1 ));
		CheckDlgButton( hwndDlg, IDC_ERRORS_USING_POPUPS, MSN_GetByte( "ShowErrorsAsPopups", 0 ));

		int tTimeout = MSN_GetDword( NULL, "PopupTimeout", 3 );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, tTimeout, FALSE );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT2, MSN_GetDword( NULL, "PopupTimeoutOther", tTimeout ), FALSE );
		if ( !ServiceExists( MS_POPUP_ADDPOPUPEX )) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT2 ), FALSE );
		}
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
		case IDC_POPUP_TIMEOUT:
		case IDC_POPUP_TIMEOUT2:
		case IDC_NOTIFY_FIRSTMSG:
		case IDC_ERRORS_USING_POPUPS:
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
				return TRUE;
			}
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
	odp.cbSize						= sizeof(odp);
	odp.position					= -790000000;
	odp.hInstance					= hInst;
	odp.pszTemplate				= MAKEINTRESOURCE(IDD_OPT_MSN);
	odp.pszTitle					= msnProtocolName;
	odp.pszGroup					= MSN_Translate("Network");
	odp.flags						= ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl = IDC_STMSNGROUP;
	odp.pfnDlgProc					= DlgProcMsnOpts;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	char szTitle[200];
	_snprintf( szTitle, sizeof( szTitle ), "%s %s", msnProtocolName, MSN_Translate( "Network" ));

	odp.position		= -790000001;
	odp.pszTemplate	= MAKEINTRESOURCE(IDD_OPT_MSN_CONN);
	odp.pszTitle		= szTitle;
	odp.pszGroup		= MSN_Translate("Network");
	odp.flags         = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pfnDlgProc		= DlgProcMsnConnOpts;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	_snprintf( szTitle, sizeof( szTitle ), "%s %s", msnProtocolName, MSN_Translate( "Server Lists" ));

	odp.position		= -790000002;
	odp.pszTemplate	= MAKEINTRESOURCE(IDD_LISTSMGR);
	odp.pszTitle		= szTitle;
	odp.pszGroup		= MSN_Translate("Network");
	odp.flags         = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pfnDlgProc		= DlgProcMsnServLists;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	if ( ServiceExists( MS_POPUP_ADDPOPUP )) {
		memset( &odp, 0, sizeof( odp ));
		odp.cbSize			= sizeof( odp );
		odp.position		= 100000000;
		odp.hInstance		= hInst;
		odp.pszTemplate	= MAKEINTRESOURCE( IDD_HOTMAIL_OPT_POPUP );
		odp.pszTitle		= MSN_Translate( msnProtocolName );
		odp.pszGroup		= MSN_Translate("Popups");
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

	MyOptions.UseWinColors = MSN_GetByte( "UseWinColors", FALSE );
	MyOptions.PopupTimeoutHotmail = MSN_GetDword( NULL, "PopupTimeout", 3 );
	MyOptions.PopupTimeoutOther = MSN_GetDword( NULL, "PopupTimeoutOther", MyOptions.PopupTimeoutHotmail );
	MyOptions.UseGateway = MSN_GetByte( "UseGateway", FALSE );
	MyOptions.UseProxy = MSN_GetByte( "NLUseProxy", FALSE );
	MyOptions.DisableMenu = MSN_GetByte( "DisableSetNickname", FALSE );
	MyOptions.ShowErrorsAsPopups = MSN_GetByte( "ShowErrorsAsPopups", FALSE );
	MyOptions.KeepConnectionAlive = MSN_GetByte( "KeepAlive", FALSE );
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
	long challen = _snprintf( hippy, sizeof( hippy ), "%s%lu%s", MSPAuth, time(NULL) - sl, passwd );

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
		_snprintf( url, sizeof( url ), "file://%s", tPathName );
	else
		strcpy( url, "http://go.msn.com/0/1" );

	MSN_DebugLog( "Starting URL: '%s'", url );
	MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )url );
	Sleep( 15000 );
	DeleteFile( tPathName );
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
