/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"

#include <commdlg.h>

#define STYLE_DEFAULTBGCOLOUR     RGB(173,206,247)

/////////////////////////////////////////////////////////////////////////////////////////
// Icons init

struct _tag_iconList
{
	char*  szDescr;
	char*  szName;
	int    defIconID;
}
static const iconList[] =
{
	{	"Protocol icon",          "main",        IDI_MSN        },
	{	"Hotmail Inbox",          "inbox",       IDI_INBOX      },
	{	"Profile",                "profile",     IDI_PROFILE    },
	{	"MSN Services",           "services",    IDI_SERVICES   },
	{	"Block user",             "block",       IDI_MSNBLOCK   },
	{	"Invite to chat",         "invite",      IDI_INVITE     },
	{	"Start Netmeeting",       "netmeeting",  IDI_NETMEETING },
	{	"Contact list",           "list_fl",     IDI_LIST_FL    },
	{	"Allowed list",           "list_al",     IDI_LIST_AL    },
	{	"Blocked list",           "list_bl",     IDI_LIST_BL    },
	{	"Relative list",          "list_rl",     IDI_LIST_RL    }
};

HANDLE hIconLibItem[SIZEOF(iconList)];


void MsnInitIcons( void )
{
	char szFile[MAX_PATH];
	GetModuleFileNameA(hInst, szFile, MAX_PATH);

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
	sid.pszSection = MSN_Translate( "Protocols/MSN" );

	for ( unsigned i = 0; i < SIZEOF(iconList); i++ ) {
		char szSettingName[100];
		mir_snprintf( szSettingName, sizeof( szSettingName ), "MSN_%s", iconList[i].szName );
		sid.pszName = szSettingName;
		sid.pszDescription = MSN_Translate( iconList[i].szDescr );
		sid.iDefaultIndex = -iconList[i].defIconID;
		hIconLibItem[i] = ( HANDLE )CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
}	}

HICON  LoadIconEx( const char* name )
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%MSN_%s", name );
	return ( HICON )MSN_CallService( MS_SKIN2_GETICON, 0, (LPARAM)szSettingName );
}

HANDLE  GetIconHandle( int iconId )
{
	for ( unsigned i=0; i < SIZEOF(iconList); i++ )
		if ( iconList[i].defIconID == iconId )
			return hIconLibItem[i];

	return NULL;
}

void  ReleaseIconEx( const char* name )
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%MSN_%s", name );
	MSN_CallService( MS_SKIN2_RELEASEICON, 0, (LPARAM)szSettingName );
}

INT_PTR CALLBACK DlgProcMsnServLists(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/////////////////////////////////////////////////////////////////////////////////////////
// MSN Options dialog procedure

static INT_PTR CALLBACK DlgProcMsnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG: {
		TranslateDialogDefault( hwndDlg );

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		CMsnProto* proto = (CMsnProto*)lParam;

		SetDlgItemTextA( hwndDlg, IDC_HANDLE, proto->MyOptions.szEmail );

		char tBuffer[ MAX_PATH ];
		if ( !proto->getStaticString( NULL, "Password", tBuffer, sizeof( tBuffer ))) {
			MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer )+1, ( LPARAM )tBuffer );
			tBuffer[ 16 ] = 0;
			SetDlgItemTextA( hwndDlg, IDC_PASSWORD, tBuffer );
		}
		SendDlgItemMessage( hwndDlg, IDC_PASSWORD, EM_SETLIMITTEXT, 16, 0 );

		HWND wnd = GetDlgItem( hwndDlg, IDC_HANDLE2 );
		DBVARIANT dbv;
		if ( !proto->getTString( "Nick", &dbv )) {
			SetWindowText( wnd, dbv.ptszVal );
			MSN_FreeVariant( &dbv );
		}
		EnableWindow(wnd, proto->msnLoggedIn);
		EnableWindow(GetDlgItem(hwndDlg, IDC_MOBILESEND), proto->msnLoggedIn && 
			proto->getByte( "MobileEnabled", 0) && proto->getByte( "MobileAllowed", 0));

		CheckDlgButton( hwndDlg, IDC_MOBILESEND,        proto->getByte( "MobileAllowed", 0 ));
		CheckDlgButton( hwndDlg, IDC_SENDFONTINFO,      proto->getByte( "SendFontInfo", 1 ));
		CheckDlgButton( hwndDlg, IDC_USE_OWN_NICKNAME,  proto->getByte( "NeverUpdateNickname", 0 ));
		CheckDlgButton( hwndDlg, IDC_AWAY_AS_BRB,       proto->getByte( "AwayAsBrb", 0 ));
		CheckDlgButton( hwndDlg, IDC_MANAGEGROUPS,      proto->getByte( "ManageServer", 1 ));

		int tValue = proto->getByte( "RunMailerOnHotmail", 0 );
		CheckDlgButton( hwndDlg, IDC_RUN_APP_ON_HOTMAIL, tValue );
		EnableWindow( GetDlgItem( hwndDlg, IDC_MAILER_APP ), tValue );
		EnableWindow( GetDlgItem( hwndDlg, IDC_ENTER_MAILER_APP ), tValue );

		if ( !proto->getStaticString( NULL, "MailerPath", tBuffer, sizeof( tBuffer )))
			SetDlgItemTextA( hwndDlg, IDC_MAILER_APP, tBuffer );

		if ( !proto->msnLoggedIn ) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_MANAGEGROUPS ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS ), FALSE );
		}
		else CheckDlgButton( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS, proto->msnOtherContactsBlocked );
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
			case IDC_SENDFONTINFO:
			case IDC_DISABLE_ANOTHER_CONTACTS:	case IDC_USE_OWN_NICKNAME:
			case IDC_AWAY_AS_BRB:
			case IDC_MOBILESEND:
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;

			case IDC_MANAGEGROUPS:
				if ( IsDlgButtonChecked( hwndDlg, IDC_MANAGEGROUPS ))
				{
					if ( IDYES == MessageBox( hwndDlg,
											TranslateT( "Server groups import may change your contact list layout after next login. Do you want to upload your groups to the server?" ),
											TranslateT( "MSN Protocol" ), MB_YESNOCANCEL ))
					{
						CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
						proto->MSN_UploadServerGroups( NULL );
					}
				}
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;

			case IDC_RUN_APP_ON_HOTMAIL: {
				BOOL tIsChosen = IsDlgButtonChecked( hwndDlg, IDC_RUN_APP_ON_HOTMAIL );
				EnableWindow( GetDlgItem( hwndDlg, IDC_MAILER_APP ), tIsChosen );
				EnableWindow( GetDlgItem( hwndDlg, IDC_ENTER_MAILER_APP ), tIsChosen );
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
				break;
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
						memmove(szFile, szFile+1, strlen(szFile));
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
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
				if ( GetOpenFileNameA( &ofn ) != TRUE )
					break;

				if ( strchr( szFile, ' ' ) != NULL ) {
					char tmpBuf[ MAX_PATH+2 ];
					mir_snprintf( tmpBuf, sizeof( tmpBuf ), "\"%s\"", szFile );
					strcpy( szFile, tmpBuf );
				}

				SendMessage( tEditField, EM_SETSEL, 0, tSelectLen );
				SendMessageA( tEditField, EM_REPLACESEL, TRUE, LPARAM( szFile ));
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
		}	}

		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY ) {
			bool reconnectRequired = false;
			TCHAR screenStr[ MAX_PATH ];
			char  password[ 100 ], szEmail[MSN_MAX_EMAIL_LEN];
			DBVARIANT dbv;

			CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);

			GetDlgItemTextA( hwndDlg, IDC_HANDLE, szEmail, sizeof( szEmail ));
			if ( strcmp( szEmail, proto->MyOptions.szEmail )) {
				reconnectRequired = true;
				strcpy( proto->MyOptions.szEmail, szEmail );
				proto->setString( NULL, "e-mail", szEmail );
			}

			GetDlgItemTextA( hwndDlg, IDC_PASSWORD, password, sizeof( password ));
			MSN_CallService( MS_DB_CRYPT_ENCODESTRING, sizeof( password ),( LPARAM )password );
			if ( !proto->getString( NULL, "Password", &dbv )) {
				if ( strcmp( password, dbv.pszVal )) {
					reconnectRequired = true;
					proto->setString( NULL, "Password", password );
				}
				MSN_FreeVariant( &dbv );
			}
			else {
				reconnectRequired = true;
				proto->setString( NULL, "Password", password );
			}

			GetDlgItemText( hwndDlg, IDC_HANDLE2, screenStr, SIZEOF( screenStr ));
			if	( !proto->getTString( "Nick", &dbv )) {
				if ( _tcscmp( dbv.ptszVal, screenStr ))
					proto->MSN_SendNicknameT( screenStr );
				MSN_FreeVariant( &dbv );
			}

			BYTE mblsnd = IsDlgButtonChecked( hwndDlg, IDC_MOBILESEND ) == BST_CHECKED;
			if (mblsnd != proto->getByte( "MobileAllowed", 0))
			{
				proto->msnNsThread->sendPacket( "PRP", "MOB %c", mblsnd ? 'Y' : 'N' );
				proto->MSN_SetServerStatus( proto->m_iStatus );
			}

			unsigned tValue = IsDlgButtonChecked( hwndDlg, IDC_DISABLE_ANOTHER_CONTACTS );
			if ( tValue != proto->msnOtherContactsBlocked && proto->msnLoggedIn ) {
				proto->msnOtherContactsBlocked = tValue;
				proto->msnNsThread->sendPacket( "BLP", tValue ? "BL" : "AL" );
				proto->MSN_ABUpdateAttr(NULL, "MSN.IM.BLP", tValue ? "0" : "1");
				break;
			}

			proto->setByte( "SendFontInfo", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SENDFONTINFO ));
			proto->setByte( "RunMailerOnHotmail", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_RUN_APP_ON_HOTMAIL ));
			proto->setByte( "NeverUpdateNickname", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USE_OWN_NICKNAME ));
			proto->setByte( "AwayAsBrb", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AWAY_AS_BRB ));
			proto->setByte( "ManageServer", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_MANAGEGROUPS ));

			GetDlgItemText( hwndDlg, IDC_MAILER_APP, screenStr, SIZEOF( screenStr ));
			proto->setTString( "MailerPath", screenStr );

			if ( reconnectRequired && proto->msnLoggedIn )
				MessageBox( hwndDlg, TranslateT( "The changes you have made require you to reconnect to the MSN Messenger network before they take effect"), _T("MSN Options"), MB_OK );

			proto->LoadOptions();
			return TRUE;
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN Connection Options dialog procedure

static INT_PTR CALLBACK DlgProcMsnConnOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;

	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			CMsnProto* proto = (CMsnProto*)lParam;
			
			CheckDlgButton( hwndDlg, IDC_USEGATEWAY, proto->MyOptions.UseGateway );
			if ( !proto->getString( NULL, "LoginServer", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, dbv.pszVal );
				MSN_FreeVariant( &dbv );
			}
			else 
				SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, proto->MyOptions.UseGateway ? MSN_DEFAULT_GATEWAY : MSN_DEFAULT_LOGIN_SERVER );
			SetDlgItemInt( hwndDlg, IDC_MSNPORT, proto->MyOptions.UseGateway ? MSN_DEFAULT_GATEWAY_PORT : MSN_DEFAULT_PORT, FALSE );

			CheckDlgButton( hwndDlg, IDC_USEIEPROXY,  proto->getByte( "UseIeProxy",  0 ));
			CheckDlgButton( hwndDlg, IDC_SLOWSEND,    proto->getByte( "SlowSend",    0 ));

			SendDlgItemMessage( hwndDlg, IDC_HOSTOPT, CB_ADDSTRING, 0, (LPARAM)TranslateT("Automatically obtain host/port" ));
			SendDlgItemMessage( hwndDlg, IDC_HOSTOPT, CB_ADDSTRING, 0, (LPARAM)TranslateT("Manually specify host/port" ));
			SendDlgItemMessage( hwndDlg, IDC_HOSTOPT, CB_ADDSTRING, 0, (LPARAM)TranslateT("Disable" ));

			unsigned gethst = proto->getByte( "AutoGetHost", 1 );
			if (gethst < 2) gethst = !gethst;

			char ipaddr[ 256 ] = "";
			if (gethst == 1) 
			{
				if (proto->getStaticString( NULL, "YourHost", ipaddr, sizeof( ipaddr )))
					gethst = 0;
			}
			if (gethst == 0)
			{
				mir_snprintf(ipaddr, sizeof(ipaddr), "%s", proto->msnLoggedIn ? 
					proto->MyConnection.GetMyExtIPStr() : "");
			}
			SendDlgItemMessage( hwndDlg, IDC_HOSTOPT, CB_SETCURSEL, gethst, 0);
			if (ipaddr[0])
				SetDlgItemTextA( hwndDlg, IDC_YOURHOST, ipaddr );
			else
				SetDlgItemText( hwndDlg, IDC_YOURHOST, TranslateT("IP info available only after login" ));
			EnableWindow( GetDlgItem( hwndDlg, IDC_YOURHOST), gethst == 1 );

			EnableWindow( GetDlgItem( hwndDlg, IDC_MSNPORT ), FALSE );
			return TRUE;
		}

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_RESETSERVER:
			if ( IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY )) {
				SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_GATEWAY );
				SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_DEFAULT_GATEWAY_PORT, FALSE );
			}
			else {
				SetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, MSN_DEFAULT_LOGIN_SERVER );
				SetDlgItemInt(  hwndDlg, IDC_MSNPORT,  1863, FALSE );
			}
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		}

		if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus())
			switch( LOWORD( wParam )) {
			case IDC_LOGINSERVER:
			case IDC_YOURHOST:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}

		if ( HIWORD( wParam ) == CBN_SELCHANGE && LOWORD( wParam ) == IDC_HOSTOPT )
		{
			unsigned gethst = SendMessage( (HWND)lParam, CB_GETCURSEL, 0, 0);
			EnableWindow( GetDlgItem( hwndDlg, IDC_YOURHOST), gethst == 1 );
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		}

		if ( HIWORD( wParam ) == BN_CLICKED )
		{
			switch( LOWORD( wParam )) 
			{
				case IDC_USEIEPROXY:		case IDC_SLOWSEND:
					SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
					break;

				case IDC_USEGATEWAY: {
					CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
					bool tValue = !IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY );

					HWND tWindow = GetDlgItem( hwndDlg, IDC_LOGINSERVER );
					if ( !tValue ) {
						SetWindowTextA( tWindow, MSN_DEFAULT_GATEWAY );
						SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_DEFAULT_GATEWAY_PORT, FALSE );
					}
					else {
						if ( !proto->getString( NULL, "LoginServer", &dbv )) {
							SetWindowTextA( tWindow, dbv.pszVal );
							MSN_FreeVariant( &dbv );
						}
						else SetWindowTextA( tWindow, MSN_DEFAULT_LOGIN_SERVER );

						SetDlgItemInt( hwndDlg, IDC_MSNPORT, MSN_DEFAULT_PORT, FALSE );
					}
					SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
					break;
				}
			}	
		}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY ) {
			bool restartRequired = false, reconnectRequired = false;
			char str[ MAX_PATH ];

			CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);

			bool tValue = IsDlgButtonChecked( hwndDlg, IDC_USEGATEWAY ) == BST_CHECKED;
			if (proto->MyOptions.UseGateway != tValue ) {
				proto->setByte( "UseGateway", tValue );
				reconnectRequired = true;
			}
			
			GetDlgItemTextA( hwndDlg, IDC_LOGINSERVER, str, sizeof( str ));
			if (strcmp(str, MSN_DEFAULT_LOGIN_SERVER) && strcmp(str, MSN_DEFAULT_GATEWAY))
				proto->setString( NULL, "LoginServer", str );
			else
				proto->deleteSetting( NULL, "LoginServer" );

			proto->setByte( "UseIeProxy", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEIEPROXY ));
			proto->setByte( "SlowSend",   ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SLOWSEND   ));
			if (proto->getByte( "SlowSend", FALSE ))
			{
				if (DBGetContactSettingDword(NULL, "SRMsg", "MessageTimeout", 60000) < 60000 ||  
					DBGetContactSettingDword(NULL, "SRMM",  "MessageTimeout", 60000) < 60000) 
				{
					MessageBox(NULL, TranslateT("MSN Protocol requires message timeout to be not less then 60 sec. Correct the timeout value."), TranslateT("MSN"), MB_OK|MB_ICONINFORMATION);
				}
			}

			unsigned gethst2 = proto->getByte( "AutoGetHost", 1 );
			unsigned gethst = SendDlgItemMessage(hwndDlg, IDC_HOSTOPT, CB_GETCURSEL, 0, 0);
			if (gethst < 2) gethst = !gethst;
			proto->setByte( "AutoGetHost", ( BYTE )gethst );

			if (gethst == 0)
			{
 				GetDlgItemTextA( hwndDlg, IDC_YOURHOST, str, sizeof( str ));
				proto->setString( NULL, "YourHost", str );
			}
			else
				proto->deleteSetting( NULL, "YourHost" );

			if (gethst != gethst2)
			{
				proto->ForkThread( &CMsnProto::MSNConnDetectThread, NULL );
			}

			if ( restartRequired )
				MessageBox( hwndDlg, TranslateT( "The changes you have made require you to restart Miranda IM before they take effect"), TranslateT( "MSN Options" ), MB_OK );
			else if ( reconnectRequired && proto->msnLoggedIn )
				MessageBox( hwndDlg, TranslateT( "The changes you have made require you to reconnect to the MSN Messenger network before they take effect"), TranslateT( "MSN Options" ), MB_OK );

			proto->LoadOptions();
			return TRUE;
	}	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// PopUp Options Dialog: style, position, color, font...

static INT_PTR CALLBACK DlgProcHotmailPopUpOpts( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static bool bEnabled;

	switch( msg ) {
	case WM_INITDIALOG: {
		TranslateDialogDefault(hwndDlg);
		bEnabled = false;

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		CMsnProto* proto = (CMsnProto*)lParam;

		//Colours. First step is configuring the colours.
		SendDlgItemMessage( hwndDlg, IDC_BGCOLOUR, CPM_SETCOLOUR, 0, proto->MyOptions.BGColour );
		SendDlgItemMessage( hwndDlg, IDC_TEXTCOLOUR, CPM_SETCOLOUR, 0, proto->MyOptions.TextColour);

		//Second step is disabling them if we want to use default Windows ones.
		CheckDlgButton( hwndDlg, IDC_USEWINCOLORS, proto->MyOptions.UseWinColors ? BST_CHECKED : BST_UNCHECKED );
		EnableWindow( GetDlgItem( hwndDlg, IDC_BGCOLOUR), !proto->MyOptions.UseWinColors );
		EnableWindow( GetDlgItem( hwndDlg, IDC_TEXTCOLOUR), !proto->MyOptions.UseWinColors );

		if ( !ServiceExists( MS_POPUP_ADDPOPUPEX ))
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT ), FALSE );
		else
			SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, proto->MyOptions.PopupTimeoutHotmail, FALSE );

		CheckDlgButton( hwndDlg, IDC_DISABLEHOTMAIL,      proto->getByte( "DisableHotmail", 0 ));
		CheckDlgButton( hwndDlg, IDC_DISABLEHOTJUNK,      proto->getByte( "DisableHotmailJunk", 0 ));
		CheckDlgButton( hwndDlg, IDC_NOTIFY_ENDSESSION,   proto->getByte( "EnableSessionPopup", 0 ));
		CheckDlgButton( hwndDlg, IDC_NOTIFY_FIRSTMSG,     proto->getByte( "EnableDeliveryPopup", 0 ));
		CheckDlgButton( hwndDlg, IDC_NOTIFY_CUSTOMSMILEY, proto->getByte( "EnableCustomSmileyPopup", 1 ));
		CheckDlgButton( hwndDlg, IDC_ERRORS_USING_POPUPS, proto->getByte( "ShowErrorsAsPopups", 0 ));

		int tTimeout = proto->getDword( "PopupTimeout", 3 );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, tTimeout, FALSE );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT2, proto->getDword( "PopupTimeoutOther", tTimeout ), FALSE );
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
			bool toSet = SendMessage( HWND( lParam ), BM_GETCHECK, 0, 0 ) != BST_CHECKED;
			if ( !toSet )
				SendMessage( wnd, BM_GETCHECK, BST_CHECKED, 0 );

			EnableWindow( wnd, toSet );
			EnableWindow( GetDlgItem( hwndDlg, IDC_POPUP_TIMEOUT ), toSet );
			EnableWindow( GetDlgItem( hwndDlg, IDC_PREVIEW ), toSet );
		}

		case IDC_NOTIFY_CUSTOMSMILEY:
		case IDC_DISABLEHOTJUNK:
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
				CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
				proto->MyOptions.BGColour = SendDlgItemMessage( hwndDlg, IDC_BGCOLOUR, CPM_GETCOLOUR, 0, 0 );
				proto->MyOptions.TextColour = SendDlgItemMessage( hwndDlg, IDC_TEXTCOLOUR, CPM_GETCOLOUR, 0, 0 );
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_USEWINCOLORS:
			{
				CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
				proto->MyOptions.UseWinColors = IsDlgButtonChecked( hwndDlg, IDC_USEWINCOLORS ) != 0;

				EnableWindow( GetDlgItem( hwndDlg, IDC_BGCOLOUR ), !( proto->MyOptions.UseWinColors ));
				EnableWindow( GetDlgItem( hwndDlg, IDC_TEXTCOLOUR ), !( proto->MyOptions.UseWinColors ));
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			}
			break;

		case IDC_PREVIEW: 
			{
				CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
				proto->MSN_ShowPopup( TranslateT( "A New Hotmail has come!" ), TranslateT( "Test: Arrival Hotmail" ), MSN_HOTMAIL_POPUP );
			}
			break;

		case IDC_PREVIEW2:
			{
				CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
				proto->MSN_ShowPopup( _T("john.doe@hotmail.com"), TranslateT( "Chat session established" ), 0, NULL );
			}
			break;
		}
		break;

	case WM_NOTIFY: //Here we have pressed either the OK or the APPLY button.
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_RESET: {
				CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
				proto->LoadOptions();
				return TRUE;
				}

			case PSN_APPLY: {
				CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);
				
				proto->MyOptions.TextColour = SendDlgItemMessage(hwndDlg,IDC_TEXTCOLOUR,CPM_GETCOLOUR,0,0);
				DBWriteContactSettingDword(NULL, proto->ModuleName, "TextColour", proto->MyOptions.TextColour);

				proto->MyOptions.BGColour = SendDlgItemMessage(hwndDlg,IDC_BGCOLOUR,CPM_GETCOLOUR,0,0);
				DBWriteContactSettingDword(NULL, proto->ModuleName, "BackgroundColour", proto->MyOptions.BGColour);

				if ( ServiceExists( MS_POPUP_ADDPOPUPEX )) {
					proto->MyOptions.PopupTimeoutHotmail = GetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, NULL, FALSE );
					proto->setDword( NULL, "PopupTimeout", proto->MyOptions.PopupTimeoutHotmail );

					proto->MyOptions.PopupTimeoutOther = GetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT2, NULL, FALSE );
					proto->setDword( NULL, "PopupTimeoutOther", proto->MyOptions.PopupTimeoutOther );
				}

				proto->MyOptions.ShowErrorsAsPopups = IsDlgButtonChecked( hwndDlg, IDC_ERRORS_USING_POPUPS ) != 0;
				proto->setByte( "ShowErrorsAsPopups", proto->MyOptions.ShowErrorsAsPopups );

				proto->setByte( "UseWinColors",	( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USEWINCOLORS ));
				proto->setByte( "EnableCustomSmileyPopup", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_NOTIFY_CUSTOMSMILEY ));
				proto->setByte( "DisableHotmail", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLEHOTMAIL ));
				proto->setByte( "DisableHotmailJunk",( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLEHOTJUNK ));
				proto->setByte( "EnableDeliveryPopup", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_NOTIFY_FIRSTMSG ));
				proto->setByte( "EnableSessionPopup", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_NOTIFY_ENDSESSION ));
				}
				return TRUE;
			}
			break;
		}
		break;
	}

	return FALSE;
}

static INT_PTR CALLBACK DlgProcAccMgrUI( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_INITDIALOG: 
		{
			TranslateDialogDefault(hwndDlg);

			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			CMsnProto* proto = (CMsnProto*)lParam;

			SetDlgItemTextA( hwndDlg, IDC_HANDLE, proto->MyOptions.szEmail );

			char tBuffer[ MAX_PATH ];
			if ( !proto->getStaticString( NULL, "Password", tBuffer, sizeof( tBuffer ))) {
				MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer )+1, ( LPARAM )tBuffer );
				tBuffer[ 16 ] = 0;
				SetDlgItemTextA( hwndDlg, IDC_PASSWORD, tBuffer );
			}
			SendDlgItemMessage( hwndDlg, IDC_PASSWORD, EM_SETLIMITTEXT, 16, 0 );
			return TRUE;
		}

	case WM_COMMAND:
		if ( LOWORD( wParam ) == IDC_NEWMSNACCOUNTLINK ) {
			MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )"http://lc2.law13.hotmail.passport.com/cgi-bin/register" );
			return TRUE;
		}

		if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus()) 
		{
			switch( LOWORD( wParam )) {
			case IDC_HANDLE:			case IDC_PASSWORD:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY ) 
		{
			char  password[ 100 ], szEmail[MSN_MAX_EMAIL_LEN];
			DBVARIANT dbv;

			CMsnProto* proto = (CMsnProto*)GetWindowLong(hwndDlg, GWL_USERDATA);

			GetDlgItemTextA( hwndDlg, IDC_HANDLE, szEmail, sizeof( szEmail ));
			if ( strcmp( szEmail, proto->MyOptions.szEmail )) {
				strcpy( proto->MyOptions.szEmail, szEmail );
				proto->setString( NULL, "e-mail", szEmail );
			}

			GetDlgItemTextA( hwndDlg, IDC_PASSWORD, password, sizeof( password ));
			MSN_CallService( MS_DB_CRYPT_ENCODESTRING, sizeof( password ),( LPARAM )password );
			if ( !proto->getString( NULL, "Password", &dbv )) {
				if ( strcmp( password, dbv.pszVal ))
					proto->setString( NULL, "Password", password );
				MSN_FreeVariant( &dbv );
			}
			else 
				proto->setString( NULL, "Password", password );

			return TRUE;
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Initialize options pages

int CMsnProto::OnOptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize      = sizeof(odp);
	odp.position    = -790000000;
	odp.hInstance   = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSN);
	odp.ptszTitle    = m_tszUserName;
	odp.ptszGroup    = LPGENT("Network");
	odp.ptszTab      = LPGENT("Account");
	odp.flags       = ODPF_BOLDGROUPS | ODPF_TCHAR;
	odp.pfnDlgProc  = DlgProcMsnOpts;
	odp.dwInitParam = (LPARAM)this;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	odp.ptszTab      = LPGENT("Connection");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSN_CONN);
	odp.pfnDlgProc  = DlgProcMsnConnOpts;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	odp.ptszTab      = LPGENT("Server list");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_LISTSMGR);
	odp.pfnDlgProc  = DlgProcMsnServLists;
	MSN_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	odp.ptszTab      = LPGENT("Notifications");
	odp.pszTemplate	= MAKEINTRESOURCEA( IDD_HOTMAIL_OPT_POPUP );
	odp.pfnDlgProc	= DlgProcHotmailPopUpOpts;
	MSN_CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	return 0;
}

int CMsnProto::SvcCreateAccMgrUI(WPARAM wParam, LPARAM lParam)
{
	return (int)CreateDialogParam (hInst, MAKEINTRESOURCE( IDD_ACCMGRUI ), 
		 (HWND)lParam, DlgProcAccMgrUI, (LPARAM)this );
}


/////////////////////////////////////////////////////////////////////////////////////////
// Load resident option values into memory

void  CMsnProto::LoadOptions()
{
	memset( &MyOptions, 0, sizeof( MyOptions ));

	//PopUp Options
	MyOptions.BGColour =
		DBGetContactSettingDword( NULL, ModuleName, "BackgroundColour", STYLE_DEFAULTBGCOLOUR );
	MyOptions.TextColour =
		DBGetContactSettingDword( NULL, ModuleName, "TextColour", GetSysColor( COLOR_WINDOWTEXT ));

	MyOptions.AwayAsBrb = getByte( "AwayAsBrb", FALSE ) != 0;
	MyOptions.ManageServer = getByte( "ManageServer", TRUE ) != 0;
	MyOptions.PopupTimeoutHotmail = getDword( "PopupTimeout", 3 );
	MyOptions.PopupTimeoutOther = getDword( "PopupTimeoutOther", MyOptions.PopupTimeoutHotmail );
	MyOptions.ShowErrorsAsPopups = getByte( "ShowErrorsAsPopups", FALSE ) != 0;
	MyOptions.SlowSend = getByte( "SlowSend", FALSE ) != 0;
	MyOptions.UseProxy = getByte( "NLUseProxy", FALSE ) != 0;
	MyOptions.UseGateway = getByte( "UseGateway", FALSE ) != 0;
	MyOptions.UseWinColors = getByte( "UseWinColors", FALSE ) != 0;
	if ( getStaticString( NULL, "e-mail", MyOptions.szEmail, sizeof( MyOptions.szEmail )))
		MyOptions.szEmail[0] = 0;
}
