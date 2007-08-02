/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_opt.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include "jabber_ssl.h"
#include <commctrl.h>
#include "resource.h"
#include <uxtheme.h>
#include "jabber_caps.h"
#include "jabber_opttree.h"

extern BOOL jabberSendKeepAlive;
extern UINT jabberCodePage;

static BOOL (WINAPI *pfnEnableThemeDialogTexture)(HANDLE, DWORD) = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// JabberRegisterDlgProc - the dialog proc for registering new account

#if defined( _UNICODE )
	#define STR_FORMAT _T("%s %s@%S:%d?")
#else
	#define STR_FORMAT _T("%s %s@%s:%d?")
#endif

static BOOL CALLBACK JabberRegisterDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	ThreadData *thread, *regInfo;

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		TranslateDialogDefault( hwndDlg );
		regInfo = ( ThreadData* ) lParam;
		TCHAR text[256];
		mir_sntprintf( text, SIZEOF(text), STR_FORMAT, TranslateT( "Register" ), regInfo->username, regInfo->server, regInfo->port );
		SetDlgItemText( hwndDlg, IDC_REG_STATUS, text );
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )regInfo );
		return TRUE;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			ShowWindow( GetDlgItem( hwndDlg, IDOK ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), SW_SHOW );
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_SHOW );
			regInfo = ( ThreadData* ) GetWindowLong( hwndDlg, GWL_USERDATA );
			thread = new ThreadData( JABBER_SESSION_REGISTER );
			_tcsncpy( thread->username, regInfo->username, SIZEOF( thread->username ));
			strncpy( thread->password, regInfo->password, SIZEOF( thread->password ));
			strncpy( thread->server, regInfo->server, SIZEOF( thread->server ));
			strncpy( thread->manualHost, regInfo->manualHost, SIZEOF( thread->manualHost ));
			thread->port = regInfo->port;
			thread->useSSL = regInfo->useSSL;
			thread->reg_hwndDlg = hwndDlg;
			mir_forkthread(( pThreadFunc )JabberServerThread, thread );
			return TRUE;
		case IDCANCEL:
		case IDOK2:
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		break;
	case WM_JABBER_REGDLG_UPDATE:	// wParam=progress ( 0-100 ), lparam=status string
		if (( TCHAR* )lParam == NULL )
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, TranslateT( "No message" ));
		else
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, ( TCHAR* )lParam );
		if ( wParam >= 0 )
			SendMessage( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), PBM_SETPOS, wParam, 0 );
		if ( wParam >= 100 ) {
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDOK2 ), SW_SHOW );
		}
		else SetFocus( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ));
		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberOptDlgProc - main options dialog procedure

static HWND msgLangListBox;
static BOOL CALLBACK JabberMsgLangAdd( LPSTR str )
{
	int i, count, index;
	UINT cp;
	static struct { UINT cpId; TCHAR* cpName; } cpTable[] = {
		{	874,	_T("Thai") },
		{	932,	_T("Japanese") },
		{	936,	_T("Simplified Chinese") },
		{	949,	_T("Korean") },
		{	950,	_T("Traditional Chinese") },
		{	1250,	_T("Central European") },
		{	1251,	_T("Cyrillic") },
		{	1252,	_T("Latin I") },
		{	1253,	_T("Greek") },
		{	1254,	_T("Turkish") },
		{	1255,	_T("Hebrew") },
		{	1256,	_T("Arabic") },
		{	1257,	_T("Baltic") },
		{	1258,	_T("Vietnamese") },
		{	1361,	_T("Korean ( Johab )") }
	};

	cp = atoi( str );
	count = sizeof( cpTable )/sizeof( cpTable[0] );
	for ( i=0; i<count && cpTable[i].cpId!=cp; i++ );
	if ( i < count ) {
		if (( index=SendMessage( msgLangListBox, CB_ADDSTRING, 0, ( LPARAM )TranslateTS( cpTable[i].cpName )) ) >= 0 ) {
			SendMessage( msgLangListBox, CB_SETITEMDATA, ( WPARAM ) index, ( LPARAM )cp );
			if ( jabberCodePage == cp )
				SendMessage( msgLangListBox, CB_SETCURSEL, ( WPARAM ) index, 0 );
	}	}

	return TRUE;
}

static LRESULT CALLBACK JabberValidateUsernameWndProc( HWND hwndEdit, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WNDPROC oldProc = ( WNDPROC ) GetWindowLong( hwndEdit, GWL_USERDATA );

	if ( msg == WM_CHAR ) {
		switch( wParam ) {
		case '\"':  case '&':	case '\'':	case '/':
		case ':':	case '<':	case '>':	case '@':
			return 0;
	}	}

	return CallWindowProc( oldProc, hwndEdit, msg, wParam, lParam );
}

static BOOL CALLBACK JabberOptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			BOOL enableRegister = TRUE;

			TranslateDialogDefault( hwndDlg );
			SetDlgItemTextA( hwndDlg, IDC_SIMPLE, jabberModuleName );
			if ( !DBGetContactSetting( NULL, jabberProtoName, "LoginName", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_EDIT_USERNAME, dbv.pszVal );
				if ( !dbv.pszVal[0] ) enableRegister = FALSE;
				JFreeVariant( &dbv );
			}
			if ( !DBGetContactSetting( NULL, jabberProtoName, "Password", &dbv )) {
				JCallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
				SetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, dbv.pszVal );
				if ( !dbv.pszVal[0] ) enableRegister = FALSE;
				JFreeVariant( &dbv );
			}

			// fill predefined resources
			TCHAR* szResources[] = { _T("Home"), _T("Work"), _T("Office"), _T("Miranda") };
			for ( int i = 0; i < SIZEOF(szResources); i++ )
				SendDlgItemMessage( hwndDlg, IDC_COMBO_RESOURCE, CB_ADDSTRING, 0, (LPARAM)szResources[i] );

			if ( !DBGetContactSettingTString( NULL, jabberProtoName, "Resource", &dbv )) {
				SetDlgItemText( hwndDlg, IDC_COMBO_RESOURCE, dbv.ptszVal );
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_COMBO_RESOURCE, "Miranda" );

			SendMessage( GetDlgItem( hwndDlg, IDC_PRIORITY_SPIN ), UDM_SETRANGE, 0, ( LPARAM )MAKELONG( 127, -128 ));

			char text[256];
			sprintf( text, "%d", (short)JGetWord( NULL, "Priority", 5 ));
			SetDlgItemTextA( hwndDlg, IDC_PRIORITY, text );
			CheckDlgButton( hwndDlg, IDC_SAVEPASSWORD, JGetByte( "SavePassword", TRUE ));
			if ( !DBGetContactSetting( NULL, jabberProtoName, "LoginServer", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, dbv.pszVal );
				if ( !dbv.pszVal[0] ) enableRegister = FALSE;
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, "jabber.org" );

			WORD port = ( WORD )JGetWord( NULL, "Port", JABBER_DEFAULT_PORT );
			SetDlgItemInt( hwndDlg, IDC_PORT, port, FALSE );
			if ( port <= 0 ) enableRegister = FALSE;

			CheckDlgButton( hwndDlg, IDC_USE_SSL, JGetByte( "UseSSL", FALSE ));
			CheckDlgButton( hwndDlg, IDC_USE_TLS, JGetByte( "UseTLS", FALSE ));
			if ( !JabberSslInit() ) {
				EnableWindow(GetDlgItem( hwndDlg, IDC_USE_SSL ), FALSE );
				EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), FALSE );
				EnableWindow(GetDlgItem( hwndDlg, IDC_DOWNLOAD_OPENSSL ), TRUE );
			}
			else {
				EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), !JGetByte( "UseSSL", FALSE ));
				EnableWindow(GetDlgItem( hwndDlg, IDC_DOWNLOAD_OPENSSL ), FALSE );
			}

			EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_REGISTER ), enableRegister );
			EnableWindow( GetDlgItem( hwndDlg, IDC_UNREGISTER ), jabberConnected );

			if ( JGetByte( "ManualConnect", FALSE ) == TRUE ) {
				CheckDlgButton( hwndDlg, IDC_MANUAL, TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_HOST ), TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_HOSTPORT ), TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_PORT ), FALSE );
			}
			if ( !DBGetContactSetting( NULL, jabberProtoName, "ManualHost", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_HOST, dbv.pszVal );
				JFreeVariant( &dbv );
			}
			SetDlgItemInt( hwndDlg, IDC_HOSTPORT, JGetWord( NULL, "ManualPort", JABBER_DEFAULT_PORT ), FALSE );

			CheckDlgButton( hwndDlg, IDC_KEEPALIVE, JGetByte( "KeepAlive", TRUE ));
			CheckDlgButton( hwndDlg, IDC_ROSTER_SYNC, JGetByte( "RosterSync", FALSE ));

			if ( !DBGetContactSetting( NULL, jabberProtoName, "Jud", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_JUD, dbv.pszVal );
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_JUD, "users.jabber.org" );

			msgLangListBox = GetDlgItem( hwndDlg, IDC_MSGLANG );
			TCHAR str[ 256 ];
			mir_sntprintf( str, SIZEOF(str), _T("== %s =="), TranslateT( "System default" ));
			SendMessage( msgLangListBox, CB_ADDSTRING, 0, ( LPARAM )str );
			SendMessage( msgLangListBox, CB_SETITEMDATA, 0, CP_ACP );
			SendMessage( msgLangListBox, CB_SETCURSEL, 0, 0 );
			EnumSystemCodePagesA( JabberMsgLangAdd, CP_INSTALLED );

			WNDPROC oldProc = ( WNDPROC ) GetWindowLong( GetDlgItem( hwndDlg, IDC_EDIT_USERNAME ), GWL_WNDPROC );
			SetWindowLong( GetDlgItem( hwndDlg, IDC_EDIT_USERNAME ), GWL_USERDATA, ( LONG ) oldProc );
			SetWindowLong( GetDlgItem( hwndDlg, IDC_EDIT_USERNAME ), GWL_WNDPROC, ( LONG ) JabberValidateUsernameWndProc );
			return TRUE;
		}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_EDIT_USERNAME:
		case IDC_EDIT_PASSWORD:
		case IDC_COMBO_RESOURCE:
		case IDC_EDIT_LOGIN_SERVER:
		case IDC_PORT:
		case IDC_MANUAL:
		case IDC_HOST:
		case IDC_HOSTPORT:
		case IDC_JUD:
		case IDC_PRIORITY:
		{
			if ( LOWORD( wParam ) == IDC_MANUAL ) {
				if ( IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOST ), TRUE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOSTPORT ), TRUE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_PORT ), FALSE );
				}
				else {
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOST ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOSTPORT ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_PORT ), TRUE );
				}
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			}
			else {
				WORD wHiParam = HIWORD( wParam );
				if ( ((HWND)lParam==GetFocus() && wHiParam==EN_CHANGE) || ((HWND)lParam==GetParent(GetFocus()) && (wHiParam == CBN_EDITCHANGE || wHiParam == CBN_SELCHANGE)) )
					SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			}

			ThreadData regInfo( JABBER_SESSION_NORMAL );
			GetDlgItemText( hwndDlg, IDC_EDIT_USERNAME, regInfo.username, SIZEOF( regInfo.username ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, regInfo.password, SIZEOF( regInfo.password ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, regInfo.server, SIZEOF( regInfo.server ));
			if ( IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_HOSTPORT, NULL, FALSE );
				GetDlgItemTextA( hwndDlg, IDC_HOST, regInfo.manualHost, SIZEOF( regInfo.manualHost ));
			}
			else {
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_PORT, NULL, FALSE );
				regInfo.manualHost[0] = '\0';
			}
			if ( regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && ( !IsDlgButtonChecked( hwndDlg, IDC_MANUAL ) || regInfo.manualHost[0] ))
				EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_REGISTER ), TRUE );
			else
				EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_REGISTER ), FALSE );
			break;
		}
		case IDC_LINK_PUBLIC_SERVER:
			ShellExecuteA( hwndDlg, "open", "http://www.jabber.org/user/publicservers.shtml", "", "", SW_SHOW );
			return TRUE;
		case IDC_DOWNLOAD_OPENSSL:
			ShellExecuteA( hwndDlg, "open", "http://www.slproweb.com/products/Win32OpenSSL.html", "", "", SW_SHOW );
			return TRUE;
		case IDC_BUTTON_REGISTER:
		{
			ThreadData regInfo( JABBER_SESSION_NORMAL );
			GetDlgItemText( hwndDlg, IDC_EDIT_USERNAME, regInfo.username, SIZEOF( regInfo.username ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, regInfo.password, SIZEOF( regInfo.password ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, regInfo.server, SIZEOF( regInfo.server ));
			if ( IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
				GetDlgItemTextA( hwndDlg, IDC_HOST, regInfo.manualHost, SIZEOF( regInfo.manualHost ));
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_HOSTPORT, NULL, FALSE );
			}
			else {
				regInfo.manualHost[0] = '\0';
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_PORT, NULL, FALSE );
			}
			regInfo.useSSL = IsDlgButtonChecked( hwndDlg, IDC_USE_SSL );

			if ( regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && ( !IsDlgButtonChecked( hwndDlg, IDC_MANUAL ) || regInfo.manualHost[0] ))
				DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_OPT_REGISTER ), hwndDlg, JabberRegisterDlgProc, ( LPARAM )&regInfo );

			return TRUE;
		}
		case IDC_UNREGISTER:
			if ( MessageBox( NULL, TranslateT( "This operation will kill your account, roster and all another information stored at the server. Are you ready to do that?"),
						TranslateT( "Account removal warning" ), MB_YESNOCANCEL ) == IDYES )
			{
				XmlNodeIq iq( "set", NOID, jabberJID );
				iq.addQuery( JABBER_FEAT_REGISTER )->addChild( "remove" );
				jabberThreadInfo->send( iq );
			}
			break;
		case IDC_MSGLANG:
			if ( HIWORD( wParam ) == CBN_SELCHANGE )
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		case IDC_USE_SSL:
			if ( !IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
				if ( IsDlgButtonChecked( hwndDlg, IDC_USE_SSL )) {
					EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), FALSE );
					SetDlgItemInt( hwndDlg, IDC_PORT, 5223, FALSE );
				}
				else {
					EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), TRUE );
					SetDlgItemInt( hwndDlg, IDC_PORT, 5222, FALSE );
			}	}
			// Fall through
		case IDC_USE_TLS:
		case IDC_SAVEPASSWORD:
		case IDC_KEEPALIVE:
		case IDC_ROSTER_SYNC:
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		default:
			return 0;
		}
		break;
	case WM_NOTIFY:
		if (( ( LPNMHDR ) lParam )->code == PSN_APPLY ) {
			BOOL reconnectRequired = FALSE;
			DBVARIANT dbv;

			char userName[256], text[256];
			TCHAR textT [256];
			GetDlgItemTextA( hwndDlg, IDC_EDIT_USERNAME, userName, sizeof( userName ));
			if ( DBGetContactSetting( NULL, jabberProtoName, "LoginName", &dbv ) || strcmp( userName, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "LoginName", userName );

			if ( IsDlgButtonChecked( hwndDlg, IDC_SAVEPASSWORD )) {
				GetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, text, sizeof( text ));
				JCallService( MS_DB_CRYPT_ENCODESTRING, sizeof( text ), ( LPARAM )text );
				if ( DBGetContactSetting( NULL, jabberProtoName, "Password", &dbv ) || strcmp( text, dbv.pszVal ))
					reconnectRequired = TRUE;
				if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
				JSetString( NULL, "Password", text );
			}
			else JDeleteSetting( NULL, "Password" );

			GetDlgItemText( hwndDlg, IDC_COMBO_RESOURCE, textT, SIZEOF( textT ));
			if ( !JGetStringT( NULL, "Resource", &dbv )) {
				if ( _tcscmp( textT, dbv.ptszVal ))
					reconnectRequired = TRUE;
				JFreeVariant( &dbv );
			}
			else reconnectRequired = TRUE;
			JSetStringT( NULL, "Resource", textT );

			GetDlgItemTextA( hwndDlg, IDC_PRIORITY, text, sizeof( text ));
			int nPriority = atoi( text );
			if ( nPriority > 127) nPriority = 127;
			if ( nPriority < -128) nPriority = -128;
			JSetWord( NULL, "Priority", ( WORD )nPriority );

			JSetByte( "SavePassword", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_SAVEPASSWORD ));

			GetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, text, sizeof( text ));
			rtrim( text );
			if ( DBGetContactSetting( NULL, jabberProtoName, "LoginServer", &dbv ) || strcmp( text, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "LoginServer", text );

			strcat( userName, "@" );
			strncat( userName, text, sizeof( userName ));
			userName[ sizeof(userName)-1 ] = 0;
			JSetString( NULL, "jid", userName );

			WORD port = ( WORD )GetDlgItemInt( hwndDlg, IDC_PORT, NULL, FALSE );
			if ( JGetWord( NULL, "Port", JABBER_DEFAULT_PORT ) != port )
				reconnectRequired = TRUE;
			JSetWord( NULL, "Port", port );

			JSetByte( "UseSSL", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_USE_SSL ));
			JSetByte( "UseTLS", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_USE_TLS ));

			JSetByte( "ManualConnect", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_MANUAL ));

			GetDlgItemTextA( hwndDlg, IDC_HOST, text, sizeof( text ));
			rtrim( text );
			if ( DBGetContactSetting( NULL, jabberProtoName, "ManualHost", &dbv ) || strcmp( text, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "ManualHost", text );

			port = ( WORD )GetDlgItemInt( hwndDlg, IDC_HOSTPORT, NULL, FALSE );
			if ( JGetWord( NULL, "ManualPort", JABBER_DEFAULT_PORT ) != port )
				reconnectRequired = TRUE;
			JSetWord( NULL, "ManualPort", port );

			JSetByte( "KeepAlive", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE ));
			jabberSendKeepAlive = IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE );

			JSetByte( "RosterSync", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_ROSTER_SYNC ));

			GetDlgItemTextA( hwndDlg, IDC_JUD, text, sizeof( text ));
			JSetString( NULL, "Jud", text );

			int index = SendMessage( GetDlgItem( hwndDlg, IDC_MSGLANG ), CB_GETCURSEL, 0, 0 );
			if ( index >= 0 ) {
				jabberCodePage = SendMessage( GetDlgItem( hwndDlg, IDC_MSGLANG ), CB_GETITEMDATA, ( WPARAM ) index, 0 );
				JSetWord( NULL, "CodePage", ( WORD )jabberCodePage );
			}

			if ( reconnectRequired && jabberConnected )
				MessageBox( hwndDlg, TranslateT( "These changes will take effect the next time you connect to the Jabber network." ), TranslateT( "Jabber Protocol Option" ), MB_OK|MB_SETFOREGROUND );

			JabberSendPresence( jabberStatus, true );

			return TRUE;
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberAdvOptDlgProc - advanced options dialog procedure

static BOOL CALLBACK JabberAdvOptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	char text[256];
	BOOL bChecked;

	static OPTTREE_OPTION options[] =
	{
		{0,	LPGENT("Messaging") _T("/") LPGENT("Send messages slower, but with full acknowledgement"),
				OPTTREE_CHECK,	1,	NULL,	"MsgAck"},
		{0,	LPGENT("Messaging") _T("/") LPGENT("Enable avatars"),
				OPTTREE_CHECK,	1,	NULL,	"EnableAvatars"},
		{0,	LPGENT("Messaging") _T("/") LPGENT("Log chat state changes"),
				OPTTREE_CHECK,	1,	NULL,	"LogChatstates"},
		{0,	LPGENT("Messaging") _T("/") LPGENT("Enable user moods receiving"),
				OPTTREE_CHECK,	1,	NULL,	"EnableUserMood"},
		{0,	LPGENT("Messaging") _T("/") LPGENT("Enable user tunes receiving"),
				OPTTREE_CHECK,	1,	NULL,	"EnableUserTune"},

		{0,	LPGENT("Conferences") _T("/") LPGENT("Autoaccept multiuser chat invitations"),
				OPTTREE_CHECK,	1,	NULL,	"AutoAcceptMUC"},
		{0,	LPGENT("Conferences") _T("/") LPGENT("Automatically join Bookmarks on login"),
				OPTTREE_CHECK,	1,	NULL,	"AutoJoinBookmarks"},
		{0,	LPGENT("Conferences") _T("/") LPGENT("Automatically join conferences on login"),
				OPTTREE_CHECK,	1,	NULL,	"AutoJoinConferences"},

		{0,	LPGENT("Server options") _T("/") LPGENT("Disable SASL authentication (for old servers)"),
				OPTTREE_CHECK,	1,	NULL,	"Disable3920auth"},
		{0,	LPGENT("Server options") _T("/") LPGENT("Enable stream compression (if possible)"),
				OPTTREE_CHECK,	1,	NULL,	"EnableZlib"},

		{0,	LPGENT("Other") _T("/") LPGENT("Enable remote controlling (from another resource of same JID only)"),
				OPTTREE_CHECK,	1,	NULL,	"EnableRemoteControl"},
		{0,	LPGENT("Other") _T("/") LPGENT("Show transport agents on contact list"),
				OPTTREE_CHECK,	1,	NULL,	"ShowTransport"},
		{0,	LPGENT("Other") _T("/") LPGENT("Automatically add contact when accept authorization"),
				OPTTREE_CHECK,	1,	NULL,	"AutoAdd"},
	};

	BOOL result;
	if (OptTree_ProcessMessage(hwndDlg, msg, wParam, lParam, &result, IDC_OPTTREE, options, SIZEOF(options)))
		return result;

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		TranslateDialogDefault( hwndDlg );
		OptTree_Translate(GetDlgItem(hwndDlg, IDC_OPTTREE));

		// File transfer options
		BOOL bDirect = JGetByte( "BsDirect", TRUE );
		BOOL bManualDirect = JGetByte( "BsDirectManual", FALSE );
		CheckDlgButton( hwndDlg, IDC_DIRECT, bDirect );
		CheckDlgButton( hwndDlg, IDC_DIRECT_MANUAL, bManualDirect );

		DBVARIANT dbv;
		if ( !DBGetContactSetting( NULL, jabberProtoName, "BsDirectAddr", &dbv )) {
			SetDlgItemTextA( hwndDlg, IDC_DIRECT_ADDR, dbv.pszVal );
			JFreeVariant( &dbv );
		}
		if ( !bDirect )
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_MANUAL ), FALSE );
		if ( !bDirect || !bManualDirect )
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_ADDR ), FALSE );

		BOOL bManualProxy = JGetByte( "BsProxyManual", FALSE );
		CheckDlgButton( hwndDlg, IDC_PROXY_MANUAL, bManualProxy );
		if ( !DBGetContactSetting( NULL, jabberProtoName, "BsProxyServer", &dbv )) {
			SetDlgItemTextA( hwndDlg, IDC_PROXY_ADDR, dbv.pszVal );
			JFreeVariant( &dbv );
		}
		if ( !bManualProxy )
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_ADDR ), FALSE );

		// Miscellaneous options
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("ShowTransport", TRUE)?1:0,		"ShowTransport");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("AutoAdd", TRUE)?1:0,				"AutoAdd");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("MsgAck", FALSE)?1:0,				"MsgAck");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("EnableAvatars", TRUE)?1:0,		"EnableAvatars");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("AutoAcceptMUC", FALSE)?1:0,		"AutoAcceptMUC");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("AutoJoinConferences", FALSE)?1:0, "AutoJoinConferences");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("Disable3920auth", FALSE)?1:0,		"Disable3920auth");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("EnableRemoteControl", FALSE)?1:0, "EnableRemoteControl");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("AutoJoinBookmarks", FALSE)?1:0,	"AutoJoinBookmarks");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("EnableZlib", FALSE)?1:0,			"EnableZlib");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("LogChatstates", FALSE)?1:0,		"LogChatstates");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("EnableUserMood", TRUE)?1:0,		"EnableUserMood");
		OptTree_SetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), 
			JGetByte("EnableUserTune", FALSE)?1:0,		"EnableUserTune");
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch ( LOWORD( wParam )) {
		case IDC_DIRECT_ADDR:
		case IDC_PROXY_ADDR:
			if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
				goto LBL_Apply;
			break;
		case IDC_DIRECT:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_DIRECT );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_MANUAL ), bChecked );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_ADDR ), ( bChecked && IsDlgButtonChecked( hwndDlg, IDC_DIRECT_MANUAL )) );
			goto LBL_Apply;
		case IDC_DIRECT_MANUAL:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_DIRECT_MANUAL );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_ADDR ), bChecked );
			goto LBL_Apply;
		case IDC_PROXY_MANUAL:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_PROXY_MANUAL );
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_ADDR ), bChecked );
		default:
		LBL_Apply:
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		}
		break;
	}
	case WM_NOTIFY:
		if (( ( LPNMHDR ) lParam )->code == PSN_APPLY ) {
			// File transfer options
			JSetByte( "BsDirect", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_DIRECT ));
			JSetByte( "BsDirectManual", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_DIRECT_MANUAL ));
			GetDlgItemTextA( hwndDlg, IDC_DIRECT_ADDR, text, sizeof( text ));
			JSetString( NULL, "BsDirectAddr", text );
			JSetByte( "BsProxyManual", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_PROXY_MANUAL ));
			GetDlgItemTextA( hwndDlg, IDC_PROXY_ADDR, text, sizeof( text ));
			JSetString( NULL, "BsProxyServer", text );

			// Miscellaneous options
			bChecked = (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "ShowTransport");
			JSetByte( "ShowTransport", ( BYTE ) bChecked );
			int index = 0;
			while (( index=JabberListFindNext( LIST_ROSTER, index )) >= 0 ) {
				JABBER_LIST_ITEM* item = JabberListGetItemPtrFromIndex( index );
				if ( item != NULL ) {
					if ( _tcschr( item->jid, '@' ) == NULL ) {
						HANDLE hContact = JabberHContactFromJID( item->jid );
						if ( hContact != NULL ) {
							if ( bChecked ) {
								if ( item->itemResource.status != JGetWord( hContact, "Status", ID_STATUS_OFFLINE )) {
									JSetWord( hContact, "Status", ( WORD )item->itemResource.status );
							}	}
							else if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
								JSetWord( hContact, "Status", ID_STATUS_OFFLINE );
				}	}	}
				index++;
			}

			JSetByte("AutoAdd",             (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "AutoAdd"));
			JSetByte("MsgAck",              (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "MsgAck"));
			JSetByte("Disable3920auth",     (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "Disable3920auth"));
			JSetByte("EnableAvatars",       (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "EnableAvatars"));
			JSetByte("AutoAcceptMUC",       (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "AutoAcceptMUC"));
			JSetByte("AutoJoinConferences", (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "AutoJoinConferences"));
			JSetByte("EnableRemoteControl", (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "EnableRemoteControl"));
			JSetByte("AutoJoinBookmarks",   (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "AutoJoinBookmarks"));
			JSetByte("EnableZlib",          (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "EnableZlib"));
			JSetByte("LogChatstates",       (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "LogChatstates"));
			JSetByte("EnableUserMood",      (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "EnableUserMood"));
			JSetByte("EnableUserTune",      (BYTE)OptTree_GetOptions(hwndDlg, IDC_OPTTREE, options, SIZEOF(options), "EnableUserTune"));
			JabberSendPresence( jabberStatus, true );
			return TRUE;
		}
		break;
	}

	return FALSE;
}



//////////////////////////////////////////////////////////////////////////
// roster editor
//
#include <io.h>
#define JM_STATUSCHANGED WM_USER+0x0001
#ifdef UNICODE
#define fputtc(str, file) fputw(str, file)
#define fopent(name, mode) _wfopen(name, mode)
#else
#define fputtc(str, file) fputc(str, file)
#define fopent(name, mode) fopen(name, mode)
#endif

static void _RosterSendRequest(HWND hwndDlg, BYTE rrAction);

enum {
	RRA_FILLLIST = 0,
	RRA_SYNCROSTER,
	RRA_SYNCDONE
};
typedef struct _tag_RosterRequestUserdata {
	HWND hwndDlg;
	BYTE bRRAction;
	BOOL bReadyToDownload;
	BOOL bReadyToUpload;
} ROSTERREQUSERDATA;
typedef struct _tag_RosterhEditDat{
	WNDPROC OldEditProc;
	HWND hList;
	int index;
	int subindex;
} ROSTEREDITDAT;
static ROSTERREQUSERDATA rrud={0};
static WNDPROC _RosterOldListProc=NULL;

static int	_RosterInsertListItem(HWND hList, TCHAR * jid, TCHAR * nick, TCHAR * group, TCHAR * subscr, BOOL bChecked)
{
	LVITEM item={0};
	int index;
	item.mask=LVIF_TEXT|LVIF_STATE;

	item.iItem=ListView_GetItemCount(hList);
	item.iSubItem=0;
	item.pszText=jid;

	index=ListView_InsertItem(hList, &item);

	if ( index<0 )
		return index;

	ListView_SetCheckState(hList, index, bChecked);

	ListView_SetItemText(hList, index, 0, jid);
	ListView_SetItemText(hList, index, 1, nick);
	ListView_SetItemText(hList, index, 2, group);
	ListView_SetItemText(hList, index, 3, subscr);

	return index;
}

static void _RosterListClear(HWND hwndDlg)
{
	HWND hList=GetDlgItem(hwndDlg, IDC_ROSTER);
	if (!hList)	return;
	ListView_DeleteAllItems(hList);
	while (	ListView_DeleteColumn( hList, 0) );

	LV_COLUMN column={0};
	column.mask=LVCF_TEXT;
	column.cx=500;

	column.pszText=_T("JID");
	int ret=ListView_InsertColumn(hList, 1, &column);

	column.pszText=_T("Nick Name");
	ListView_InsertColumn(hList, 2, &column);

	column.pszText=_T("Group");
	ListView_InsertColumn(hList, 3, &column);

	column.pszText=_T("Subscription");
	ListView_InsertColumn(hList, 4, &column);

	RECT rc;
	GetClientRect(hList, &rc);
	int width=rc.right-rc.left;

	ListView_SetColumnWidth(hList,0,width*40/100);
	ListView_SetColumnWidth(hList,1,width*30/100);
	ListView_SetColumnWidth(hList,2,width*20/100);
	ListView_SetColumnWidth(hList,3,width*10/100);
}


static void _RosterHandleGetRequest( XmlNode* node, void* userdata )
{
	HWND hList=GetDlgItem(rrud.hwndDlg, IDC_ROSTER);
	if (rrud.bRRAction==RRA_FILLLIST)
	{
		_RosterListClear(rrud.hwndDlg);
		XmlNode * query=JabberXmlGetChild(node, "query");
		if (!query) return;
		int i=1;
		while (TRUE)
		{
			XmlNode *item=JabberXmlGetNthChild(query, "item", i++);
			if (!item) break;
			TCHAR *jid=JabberXmlGetAttrValue(item,"jid");
			if (!jid) continue;
			TCHAR *name=JabberXmlGetAttrValue(item,"name");
			TCHAR *subscription=JabberXmlGetAttrValue(item,"subscription");
			TCHAR *group=NULL;
			XmlNode * groupNode=JabberXmlGetChild(item, "group");
			if (groupNode) group=groupNode->text;
			_RosterInsertListItem(hList, jid, name, group, subscription, TRUE);
		}
		// now it is require to process whole contact list to add not in roster contacts
		{
			HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			while ( hContact != NULL )
			{
				char* str = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
				if ( str != NULL && !strcmp( str, jabberProtoName ))
				{
					DBVARIANT dbv;
					if ( !JGetStringT( hContact, "jid", &dbv ))
					{
						LVFINDINFO lvfi={0};
						lvfi.flags=LVFI_STRING | LVFI_PARTIAL;
						lvfi.psz=dbv.ptszVal;
						TCHAR *p=_tcschr(dbv.ptszVal,_T('@'));
						if (p)
						{
							p=_tcschr(dbv.ptszVal,_T('\\'));
							if (p) *p=_T('\0');
						}
						if ( ListView_FindItem(hList, -1, &lvfi) == -1)
						{
							TCHAR *jid=mir_tstrdup(dbv.ptszVal);
							TCHAR *name=NULL;
							TCHAR *group=NULL;
							DBVARIANT dbvtemp;
							if ( !DBGetContactSettingTString( hContact, "CList", "MyHandle", &dbvtemp ) )
							{
								name=mir_tstrdup(dbv.ptszVal);
								DBFreeVariant(&dbvtemp);
							}
							if ( !DBGetContactSettingTString( hContact, "CList", "Group", &dbvtemp ) )
							{
								group=mir_tstrdup(dbv.ptszVal);
								DBFreeVariant(&dbvtemp);
							}
							_RosterInsertListItem(hList, jid, name, group, NULL, FALSE);
							if (jid) mir_free(jid);
							if (name) mir_free(name);
							if (group) mir_free(group);
						}
						DBFreeVariant(&dbv);

					}
				}
				hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
			}
		}
		rrud.bReadyToDownload=FALSE;
		rrud.bReadyToUpload=TRUE;
		SetDlgItemText(rrud.hwndDlg,IDC_DOWNLOAD,TranslateT("Download"));
		SetDlgItemText(rrud.hwndDlg,IDC_UPLOAD,TranslateT("Upload"));
		SendMessage(rrud.hwndDlg, JM_STATUSCHANGED,0,0);
        return;
	}
	else if (rrud.bRRAction==RRA_SYNCROSTER)
	{
		SetDlgItemText(rrud.hwndDlg,IDC_UPLOAD,TranslateT("Uploading..."));
		XmlNode * queryRoster=JabberXmlGetChild(node, "query");
		if (!queryRoster) return;

		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, (JABBER_IQ_PFUNC) _RosterHandleGetRequest );

		XmlNode iq( "iq" );
		iq.addAttr( "type", "set" );
		iq.addAttrID( iqId );
		XmlNode* query = iq.addChild( "query" );
		query->addAttr( "xmlns", "jabber:iq:roster" );
		int itemCount=0;
		int ListItemCount=ListView_GetItemCount(hList);
		for (int index=0; index<ListItemCount; index++)
		{
			TCHAR jid[260]={0};
			TCHAR name[260]={0};
			TCHAR group[260]={0};
			TCHAR subscr[260]={0};
			ListView_GetItemText(hList, index, 0, jid, SIZEOF(jid));
			ListView_GetItemText(hList, index, 1, name, SIZEOF(name));
			ListView_GetItemText(hList, index, 2, group, SIZEOF(group));
			ListView_GetItemText(hList, index, 3, subscr, SIZEOF(subscr));
			XmlNode *itemRoster=JabberXmlGetChildWithGivenAttrValue(queryRoster, "item", "jid", jid);
			BOOL bRemove = !ListView_GetCheckState(hList,index);
			if (itemRoster && bRemove)
			{
				//delete item
				XmlNode* item = query->addChild( "item" );
				item->addAttr( "jid", jid );
				item->addAttr( "subscription","remove");
				itemCount++;
			}
			else if ( !bRemove )
			{
				BOOL bPushed=FALSE;
				{
					TCHAR *rosterName=JabberXmlGetAttrValue(itemRoster,"name");
					if ( (rosterName!=NULL || name[0]!=_T('\0')) && lstrcmpi(rosterName,name) )
						bPushed=TRUE;
					if ( !bPushed)
					{
						rosterName=JabberXmlGetAttrValue(itemRoster,"subscription");
						if ((rosterName!=NULL || subscr[0]!=_T('\0')) && lstrcmpi(rosterName,subscr) )
							bPushed=TRUE;
					}
					if ( !bPushed)
					{
						XmlNode * groupNode=JabberXmlGetChild(itemRoster,"group");
						TCHAR * rosterGroup=NULL;
						if (groupNode!=NULL)
							rosterGroup=groupNode->text;
						if ((rosterGroup!=NULL || group[0]!=_T('\0')) && lstrcmpi(rosterGroup,group) )
							bPushed=TRUE;
					}

				}
				if (bPushed)
				{
					XmlNode* item = query->addChild( "item" );
					if (group)item->addChild("group", group);
					item->addAttr( "name", name );
					item->addAttr( "jid", jid );
					item->addAttr( "subscription",subscr[0] ? subscr : _T("none"));
					itemCount++;
				}
			}
		}
		rrud.bRRAction=RRA_SYNCDONE;
		if (itemCount)
			jabberThreadInfo->send( iq );
		else
			_RosterSendRequest(rrud.hwndDlg,RRA_FILLLIST);
	}
	else
	{
		SetDlgItemText(rrud.hwndDlg,IDC_UPLOAD,TranslateT("Upload"));
		rrud.bReadyToUpload=FALSE;
		rrud.bReadyToDownload=FALSE;
		SendMessage(rrud.hwndDlg, JM_STATUSCHANGED,0,0);
		SetDlgItemText(rrud.hwndDlg,IDC_DOWNLOAD,TranslateT("Downloading..."));
		_RosterSendRequest(rrud.hwndDlg,RRA_FILLLIST);

	}
}

static void _RosterSendRequest(HWND hwndDlg, BYTE rrAction)
{
	rrud.bRRAction=rrAction;
	rrud.hwndDlg=hwndDlg;

	int iqId = JabberSerialNext();
	JabberIqAdd( iqId, IQ_PROC_NONE, (JABBER_IQ_PFUNC) _RosterHandleGetRequest );

	XmlNode iq( "iq" );
	iq.addAttr( "type", "get" );
	iq.addAttrID( iqId );
	XmlNode* query = iq.addChild( "query" );
	query->addAttr( "xmlns", "jabber:iq:roster" );
	jabberThreadInfo->send( iq );
}




static void _RosterItemEditEnd( HWND hEditor, ROSTEREDITDAT * edat, BOOL bCancel )
{
	if (!bCancel)
	{
		TCHAR *buff;
		int len=GetWindowTextLength(hEditor)+1;
		buff=(TCHAR*)malloc(len*sizeof(TCHAR));
		GetWindowText(hEditor,buff,len);
		ListView_SetItemText(edat->hList,edat->index, edat->subindex,buff);
		free(buff);
	}
	DestroyWindow(hEditor);

}

static BOOL CALLBACK _RosterItemNewEditProc( HWND hEditor, UINT msg, WPARAM wParam, LPARAM lParam )
{
	ROSTEREDITDAT * edat = (ROSTEREDITDAT *) GetWindowLong(hEditor,GWL_USERDATA);
	if (!edat) return 0;
	switch(msg)
	{

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_RETURN:
			_RosterItemEditEnd(hEditor, edat, FALSE);
			return 0;
		case VK_ESCAPE:
			_RosterItemEditEnd(hEditor, edat, TRUE);
			return 0;
		}
		break;
	case WM_GETDLGCODE:
		if(lParam)
		{
			MSG *msg=(MSG*)lParam;
			if(msg->message==WM_KEYDOWN && msg->wParam==VK_TAB) return 0;
			if(msg->message==WM_CHAR && msg->wParam=='\t') return 0;
		}
		return DLGC_WANTMESSAGE;
	case WM_KILLFOCUS:
		_RosterItemEditEnd(hEditor, edat, FALSE);
		return 0;
	}

	if (msg==WM_DESTROY)
	{
		SetWindowLong(hEditor, GWL_WNDPROC, (LONG) edat->OldEditProc);
		SetWindowLong(hEditor, GWL_USERDATA, (LONG) 0);
		free(edat);
		return 0;
	}
	else return CallWindowProc( edat->OldEditProc, hEditor, msg, wParam, lParam);
}

#ifdef UNICODE
void fputw( TCHAR ch, FILE * fp)
{
	TCHAR buf[2]={0};
	char utf[10]={0};
	char *str=utf;
	buf[0]=ch;
	WideCharToMultiByte(CP_UTF8,0,buf,-1,utf,sizeof(utf),NULL, NULL);
	while ( *str!='\0' )
	{
		fputc(*str,fp);
		str++;
	}

}
#endif

static void _RosterSaveString(FILE * fp, TCHAR * str, BOOL quotes=FALSE)
{
	if (quotes) fputtc(_T('\"'),fp);
	while ( *str!=_T('\0') )
	{
		fputtc(*str,fp);
		if (quotes && *str==_T('\"')) fputtc(*str,fp);
		str++;
	}
	if (quotes) fputtc(_T('\"'),fp);
}
static void _RosterExportToFile(HWND hwndDlg)
{
	TCHAR filename[MAX_PATH]={0};

	TCHAR *filter=_T("XML for MS Excel (UTF-8 encoded)(*.xml)\0*.xml\0\0");
	OPENFILENAME ofn={0};
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hwndDlg;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.nMaxFile = SIZEOF(filename);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = _T("xml");
	if(!GetSaveFileName(&ofn)) return;

	FILE * fp=fopent(filename,_T("w"));
	if (!fp) return;
	HWND hList=GetDlgItem(hwndDlg, IDC_ROSTER);
	int ListItemCount=ListView_GetItemCount(hList);
	TCHAR *xmlExcelHeader=
		_T(			"<?xml version=\"1.0\"?>	\n"											)
		_T(			"<?mso-application progid=\"Excel.Sheet\"?> \n"							)
		_T(			"<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" \n"	)
		_T("\t\t\txmlns:o=\"urn:schemas-microsoft-com:office:office\"\n"					)
		_T("\t\t\txmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n"					)
		_T("\t\t\txmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"			)
		_T("\t\t\txmlns:html=\"http://www.w3.org/TR/REC-html40\">\n"						)
		_T("\t<ExcelWorkbook xmlns=\"urn:schemas-microsoft-com:office:excel\">\n"	)
		_T("\t</ExcelWorkbook>\n"													)
		_T("\t<Worksheet ss:Name=\"Exported roster\">\n"								)
		_T("\t\t<Table>\n"																);

	TCHAR *xmlExcelFooter=
		_T("\t\t</Table>\n")
		_T("\t</Worksheet>\n")
		_T("</Workbook>\n");

	_RosterSaveString(fp,xmlExcelHeader);
	for (int index=0; index<ListItemCount; index++)
	{
		TCHAR jid[260]={0};
		TCHAR name[260]={0};
		TCHAR group[260]={0};
		TCHAR subscr[260]={0};
		ListView_GetItemText(hList, index, 0, jid, SIZEOF(jid));
		ListView_GetItemText(hList, index, 1, name, SIZEOF(name));
		ListView_GetItemText(hList, index, 2, group, SIZEOF(group));
		ListView_GetItemText(hList, index, 3, subscr, SIZEOF(subscr));
		_RosterSaveString(fp,_T("\t\t\t<Row>\n"));
		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">+</Data></Cell>\n"));
		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,jid);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,name);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,group);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,subscr);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t</Row>\n"));
	}
	_RosterSaveString(fp,xmlExcelFooter);
	fclose(fp);
}

static void _RosterParseXmlWorkbook( XmlNode *node, void *userdata )
{
	HWND hList=(HWND)userdata;
	if (!lstrcmpiA(node->name,"Workbook"))
	{
		XmlNode * Worksheet=JabberXmlGetChild(node,"Worksheet");
		if (Worksheet)
		{
			XmlNode * Table=JabberXmlGetChild(Worksheet,"Table");
			if (Table)
			{
				int index=1;
				XmlNode *Row;
				while (TRUE)
				{
					Row=JabberXmlGetNthChild(Table,"Row",index++);
					if (!Row) break;
					BOOL bAdd=FALSE;
					TCHAR * jid=NULL;
					TCHAR * name=NULL;
					TCHAR * group=NULL;
					TCHAR * subscr=NULL;
					XmlNode * Cell;
					XmlNode * Data;
					Cell=JabberXmlGetNthChild(Row,"Cell",1);
					if (Cell) Data=JabberXmlGetChild(Cell,"Data");
					else Data=NULL;
					if (Data)
					{
						if (!lstrcmpi(Data->text,_T("+"))) bAdd=TRUE;
						else if (lstrcmpi(Data->text,_T("-"))) continue;

						Cell=JabberXmlGetNthChild(Row,"Cell",2);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data)
						{
							jid=Data->text;
							if (!jid || lstrlen(jid)==0) continue;
						}

						Cell=JabberXmlGetNthChild(Row,"Cell",3);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data) name=Data->text;

						Cell=JabberXmlGetNthChild(Row,"Cell",4);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data) group=Data->text;

						Cell=JabberXmlGetNthChild(Row,"Cell",5);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data) subscr=Data->text;
					}
					_RosterInsertListItem(hList,jid,name,group,subscr,bAdd);
				}

			}
		}
	}

}
static void _RosterImportFromFile(HWND hwndDlg)
{
	char filename[MAX_PATH]={0};
	char *filter="XML for MS Excel (UTF-8 encoded)(*.xml)\0*.xml\0\0";
	OPENFILENAMEA ofn={0};
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hwndDlg;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nMaxFile = sizeof(filename);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "xml";
	if(!GetOpenFileNameA(&ofn)) return;

	FILE * fp=fopen(filename,"r");
	if (!fp) return;
	HWND hList=GetDlgItem(hwndDlg, IDC_ROSTER);
	char * buffer;
	DWORD bufsize=filelength(fileno(fp));
	if (bufsize>0)
		buffer=(char*)malloc(bufsize);
	fread(buffer,1,bufsize,fp);
	fclose(fp);
	_RosterListClear(hwndDlg);
	XmlState xmlstate;
	JabberXmlInitState(&xmlstate);
	JabberXmlSetCallback( &xmlstate, 2, ELEM_CLOSE, _RosterParseXmlWorkbook, (void*)hList );
	JabberXmlParse(&xmlstate,buffer);
	xmlstate=xmlstate;
	JabberXmlDestroyState(&xmlstate);
	free(buffer);
	SendMessage(hwndDlg,JM_STATUSCHANGED, 0 , 0);
}


static BOOL CALLBACK _RosterNewListProc( HWND hList, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if (msg==WM_MOUSEWHEEL || msg==WM_NCLBUTTONDOWN || msg==WM_NCRBUTTONDOWN)
	{
		SetFocus(hList);
	}

	if (msg==WM_LBUTTONDOWN)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hList, &pt);

		LVHITTESTINFO lvhti={0};
		lvhti.pt=pt;
		ListView_SubItemHitTest(hList,&lvhti);
		if 	(lvhti.flags&LVHT_ONITEM && lvhti.iSubItem !=0)
		{
			RECT rc;
			TCHAR buff[260];
			ListView_GetSubItemRect(hList, lvhti.iItem, lvhti.iSubItem, LVIR_BOUNDS,&rc);
			ListView_GetItemText(hList, lvhti.iItem, lvhti.iSubItem, buff, SIZEOF(buff) );
			HWND hEditor=CreateWindow(TEXT("EDIT"),buff,WS_CHILD|ES_AUTOHSCROLL,rc.left+3, rc.top+2, rc.right-rc.left-3, rc.bottom - rc.top-3,hList, NULL, hInst, NULL);
			SendMessage(hEditor,WM_SETFONT,(WPARAM)SendMessage(hList,WM_GETFONT,0,0),0);
			ShowWindow(hEditor,SW_SHOW);
			SetWindowText(hEditor, buff);
			ClientToScreen(hList, &pt);
			ScreenToClient(hEditor, &pt);
			INPUT inp[2]={0};
			inp[0].type=INPUT_MOUSE;
			inp[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
			inp[1].type=INPUT_MOUSE;
			inp[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
			SendInput(2, inp, sizeof(inp[0]));


			ROSTEREDITDAT * edat=(ROSTEREDITDAT *)malloc(sizeof(ROSTEREDITDAT));
			edat->OldEditProc=(WNDPROC)GetWindowLong(hEditor, GWL_WNDPROC);
			SetWindowLong(hEditor,GWL_WNDPROC,(LONG)_RosterItemNewEditProc);
			edat->hList=hList;
			edat->index=lvhti.iItem;
			edat->subindex=lvhti.iSubItem;
			SetWindowLong(hEditor,GWL_USERDATA,(LONG)edat);
		}
	}
	return CallWindowProc(_RosterOldListProc, hList, msg, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberRosterOptDlgProc - advanced options dialog procedure

static BOOL CALLBACK JabberRosterOptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case JM_STATUSCHANGED:
		{
			int count=ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_ROSTER));
			EnableWindow( GetDlgItem( hwndDlg, IDC_DOWNLOAD ), rrud.bReadyToDownload && jabberConnected );
			EnableWindow( GetDlgItem( hwndDlg, IDC_UPLOAD ),   rrud.bReadyToUpload && count && jabberConnected );
			EnableWindow( GetDlgItem( hwndDlg, IDC_EXPORT ), count > 0);
			break;
		}
	case WM_DESTROY:
		{
			rrud.hwndDlg=NULL;
			break;
		}
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );
			ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg,IDC_ROSTER),  LVS_EX_CHECKBOXES | LVS_EX_BORDERSELECT /*| LVS_EX_FULLROWSELECT*/ | LVS_EX_GRIDLINES /*| LVS_EX_HEADERDRAGDROP*/ );
			_RosterOldListProc=(WNDPROC) GetWindowLong(GetDlgItem(hwndDlg,IDC_ROSTER), GWL_WNDPROC);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_ROSTER), GWL_WNDPROC, (LONG) _RosterNewListProc);
			_RosterListClear(hwndDlg);
			rrud.hwndDlg=hwndDlg;
			rrud.bReadyToDownload=TRUE;
			rrud.bReadyToUpload=FALSE;
			SendMessage(hwndDlg, JM_STATUSCHANGED,0,0);
			return TRUE;
		}
	case WM_COMMAND:
		{
			switch ( LOWORD( wParam ))
			{
			case IDC_DOWNLOAD:
				{
					rrud.bReadyToUpload=FALSE;
					rrud.bReadyToDownload=FALSE;
					SendMessage(rrud.hwndDlg, JM_STATUSCHANGED,0,0);
					SetDlgItemText(rrud.hwndDlg,IDC_DOWNLOAD,TranslateT("Downloading..."));
					_RosterSendRequest(hwndDlg,RRA_FILLLIST);
					break;
				}
			case IDC_UPLOAD:
				{
					rrud.bReadyToUpload=FALSE;
					SendMessage(rrud.hwndDlg, JM_STATUSCHANGED,0,0);
					SetDlgItemText(rrud.hwndDlg,IDC_UPLOAD,TranslateT("Connecting..."));
					_RosterSendRequest(hwndDlg,RRA_SYNCROSTER);
					break;
				}
			case IDC_EXPORT:
				{
					_RosterExportToFile(hwndDlg);
					break;
				}
			case IDC_IMPORT:
				{
					_RosterImportFromFile(hwndDlg);
					break;
				}

			}
			break;
		}
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberOptInit - initializes all options dialogs

int JabberOptInit( WPARAM wParam, LPARAM lParam )
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize      = sizeof( odp );
	odp.hInstance   = hInst;
	odp.pszGroup    = LPGEN("Network");
	odp.pszTab      = LPGEN("Account");
	odp.pszTemplate = MAKEINTRESOURCEA( IDD_OPT_JABBER );
	odp.pszTitle    = jabberModuleName;
	odp.pfnDlgProc  = JabberOptDlgProc;
	odp.flags       = ODPF_BOLDGROUPS;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pszTab      = LPGEN("Advanced");
	odp.pszTemplate = MAKEINTRESOURCEA( IDD_OPT_JABBER2 );
	odp.pfnDlgProc  = JabberAdvOptDlgProc;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pszTab      = "Roster control";
	odp.flags	   |= ODPF_EXPERTONLY;
	odp.pszTemplate = MAKEINTRESOURCEA( IDD_OPT_JABBER3 );
	odp.pfnDlgProc  = JabberRosterOptDlgProc;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	return 0;
}

void JabberUpdateDialogs( BOOL bEnable )
{
	if (rrud.hwndDlg)
		SendMessage(rrud.hwndDlg, JM_STATUSCHANGED, 0,0);
}
