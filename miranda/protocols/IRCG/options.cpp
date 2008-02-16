/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

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

#include "irc.h"
#include <algorithm>
#include <win2k.h>

#include "ui_utils.h"

static WNDPROC  OldProc;
static WNDPROC  OldListViewProc;

int CIrcProto::GetPrefsString(const char *szSetting, char * prefstoset, int n, char * defaulttext)
{
	DBVARIANT dbv;
	if ( !getString( szSetting, &dbv )) {
		lstrcpynA(prefstoset, dbv.pszVal, n);
		DBFreeVariant(&dbv);
		return TRUE;
	}

	lstrcpynA(prefstoset, defaulttext, n);
	return FALSE;
}

#if defined( _UNICODE )
int CIrcProto::GetPrefsString(const char *szSetting, TCHAR* prefstoset, int n, TCHAR* defaulttext)
{
	DBVARIANT dbv;
	if ( !getTString( szSetting, &dbv )) {
		lstrcpyn(prefstoset, dbv.ptszVal, n);
		DBFreeVariant(&dbv);
		return TRUE;
	}

	lstrcpyn(prefstoset, defaulttext, n);
	return FALSE;
}
#endif

static void removeSpaces( TCHAR* p )
{
	while ( *p ) {
		if ( *p == ' ' )
			memmove( p, p+1, sizeof(TCHAR)*lstrlen(p));
		p++;
}	}

static char* getControlText( HWND hWnd, int ctrlID )
{
	size_t size = GetWindowTextLength( GetDlgItem( hWnd, ctrlID ))+1;
	char* result = ( char* )mir_alloc( size );
	GetDlgItemTextA( hWnd, ctrlID, result, size );
	return result;
}

void CIrcProto::InitPrefs(void)
{
	DBVARIANT dbv;

	GetPrefsString( "ServerName", m_serverName, 101, "");
	GetPrefsString( "PortStart", m_portStart, 6, "");
	GetPrefsString( "PortEnd", m_portEnd, 6, "");
	GetPrefsString( "Password", m_password, 499, "");
	CallService( MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)m_password);
	if (!GetPrefsString( "PNick", m_nick, 30, _T(""))) {
		GetPrefsString( "Nick", m_nick, 30, _T(""));
		if ( lstrlen(m_nick) > 0)
			setTString("PNick", m_nick);
	}
	GetPrefsString( "AlernativeNick", m_alternativeNick, 31, _T(""));
	GetPrefsString( "Name", m_name, 199, _T(""));
	GetPrefsString( "UserID", m_userID, 199, _T("Miranda"));
	GetPrefsString( "IdentSystem", m_identSystem, 10, _T("UNIX"));
	GetPrefsString( "IdentPort", m_identPort, 6, _T("113"));
	GetPrefsString( "RetryWait", m_retryWait, 4, _T("30"));
	GetPrefsString( "RetryCount", m_retryCount, 4, _T("10"));
	GetPrefsString( "Network", m_network, 31, "");
	GetPrefsString( "QuitMessage", m_quitMessage, 399, _T(STR_QUITMESSAGE));
	GetPrefsString( "UserInfo", m_userInfo, 499, _T(STR_USERINFO));
	GetPrefsString( "SpecHost", m_mySpecifiedHost, 499, "");
	GetPrefsString( "MyLocalHost", m_myLocalHost, 49, "");

	lstrcpyA( m_mySpecifiedHostIP, "" );

	if ( !getTString( "Alias", &dbv )) {
		m_alias = mir_tstrdup( dbv.ptszVal);
		DBFreeVariant( &dbv );
	}
	else m_alias = mir_tstrdup( _T("/op /mode ## +ooo $1 $2 $3\r\n/dop /mode ## -ooo $1 $2 $3\r\n/voice /mode ## +vvv $1 $2 $3\r\n/dvoice /mode ## -vvv $1 $2 $3\r\n/j /join #$1 $2-\r\n/p /part ## $1-\r\n/w /whois $1\r\n/k /kick ## $1 $2-\r\n/q /query $1\r\n/logon /log on ##\r\n/logoff /log off ##\r\n/save /log buffer $1\r\n/slap /me slaps $1 around a bit with a large trout" ));

	m_scriptingEnabled = getByte( "ScriptingEnabled", 0);
	m_forceVisible = getByte( "ForceVisible", 0);
	m_disableErrorPopups = getByte( "DisableErrorPopups", 0);
	m_rejoinChannels = getByte( "RejoinChannels", 0);
	m_rejoinIfKicked = getByte( "RejoinIfKicked", 1);
	m_ident = getByte( "Ident", 0);
	IdentTimer = (int)getByte( "IdentTimer", 0);
	m_retry = getByte( "Retry", 0);
	m_disableDefaultServer = getByte( "DisableDefaultServer", 0);
	m_hideServerWindow = getByte( "HideServerWindow", 1);
	m_useServer = getByte( "UseServer", 1);
	m_joinOnInvite = getByte( "JoinOnInvite", 0);
	m_perform = getByte( "Perform", 0);
	m_showAddresses = getByte( "ShowAddresses", 0);
	m_autoOnlineNotification = getByte( "AutoOnlineNotification", 1);
	m_ignore = getByte( "Ignore", 0);;
	m_ignoreChannelDefault = getByte( "IgnoreChannelDefault", 0);;
	m_serverComboSelection = getDword( "ServerComboSelection", -1);
	m_quickComboSelection = getDword( "QuickComboSelection", m_serverComboSelection);
	m_sendKeepAlive = (int)getByte( "SendKeepAlive", 0);
	m_listSize.y = getDword( "SizeOfListBottom", 400);
	m_listSize.x = getDword( "SizeOfListRight", 600);
	m_iSSL = getByte( "UseSSL", 0);
	m_DCCFileEnabled = getByte( "EnableCtcpFile", 1);
	m_DCCChatEnabled = getByte( "EnableCtcpChat", 1);
	m_DCCChatAccept = getByte( "CtcpChatAccept", 1);
	m_DCCChatIgnore = getByte( "CtcpChatIgnore", 1);
	m_DCCPassive = getByte( "DccPassive", 0);
	m_manualHost = getByte( "ManualHost", 0);
	m_IPFromServer = getByte( "IPFromServer", 0);
	m_disconnectDCCChats = getByte( "DisconnectDCCChats", 1);
	m_oldStyleModes = getByte( "OldStyleModes", 0);
	m_sendNotice = getByte( "SendNotice", 1);
	m_codepage = getDword( "Codepage", CP_ACP );
	m_utfAutodetect = getByte( "UtfAutodetect", 1);
	m_myHost[0] = '\0';
	colors[0] = RGB(255,255,255);
	colors[1] = RGB(0,0,0);
	colors[2] = RGB(0,0,127);
	colors[3] = RGB(0,147,0);
	colors[4] = RGB(255,0,0);
	colors[5] = RGB(127,0,0);
	colors[6] = RGB(156,0,156);
	colors[7] = RGB(252,127,0);
	colors[8] = RGB(255,255,0);
	colors[9] = RGB(0,252,0);
	colors[10] = RGB(0,147,147);
	colors[11] = RGB(0,255,255);
	colors[12] = RGB(0,0,252);
	colors[13] = RGB(255,0,255);
	colors[14] = RGB(127,127,127); 
	colors[15] = RGB(210,210,210);
	m_onlineNotificationTime = getWord( "OnlineNotificationTime", 30 );
	m_onlineNotificationLimit = getWord( "OnlineNotificationLimit", 50 );
	m_channelAwayNotification = getByte( "ChannelAwayNotification", 1 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// add icons to the skinning module

struct
{
	char*  szDescr;
	char*  szName;
	int    iSize;
	int    defIconID;
}
static iconList[] =
{
	{ LPGEN("Main"),              "main",    16, IDI_MAIN    },
	{ LPGEN("Add"),               "add",     16, IDI_ADD     },
	{ LPGEN("Apply"),             "go",      16, IDI_GO      },
	{ LPGEN("Edit"),              "rename",  16, IDI_RENAME  },
	{ LPGEN("Cancel"),            "delete",  16, IDI_DELETE  },
	{ LPGEN("Ignore"),            "block",   16, IDI_BLOCK   },
	{ LPGEN("Channel list"),      "list",    16, IDI_LIST    },
	{ LPGEN("Channel manager"),   "manager", 16, IDI_MANAGER },
	{ LPGEN("Quick connect"),     "quick",   16, IDI_QUICK   },
	{ LPGEN("Server window"),     "server",  16, IDI_SERVER  },
	{ LPGEN("Show channel"),      "show",    16, IDI_SHOW    },
	{ LPGEN("Join channel"),      "join",    16, IDI_JOIN    },
	{ LPGEN("Leave Channel"),     "part",    16, IDI_PART    },
	{ LPGEN("Question"),          "whois",   16, IDI_WHOIS   },
	{ LPGEN("Incoming DCC Chat"), "dcc",     16, IDI_DCC     },
	{ LPGEN("Logo (48x48)"),      "logo",    48, IDI_LOGO    }
};

void CIrcProto::AddIcons(void)
{
	char szFile[MAX_PATH];
	GetModuleFileNameA(hInst, szFile, MAX_PATH);

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszSection = m_szModuleName;
	sid.pszDefaultFile = szFile;

	hIconLibItems = new HANDLE[ SIZEOF(iconList) ];

	// add them one by one
	for ( int i=0; i < SIZEOF(iconList); i++ ) {
		char szTemp[255];
		mir_snprintf(szTemp, sizeof(szTemp), "%s_%s", m_szModuleName, iconList[i].szName );
		sid.pszName = szTemp;
		sid.pszDescription = iconList[i].szDescr;
		sid.iDefaultIndex = -iconList[i].defIconID;
		sid.cx = sid.cy = iconList[i].iSize;
		hIconLibItems[i] = ( HANDLE )CallService( MS_SKIN2_ADDICON, 0, ( LPARAM )&sid );
}	}

HICON CIrcProto::LoadIconEx( int iconId )
{
	for ( int i=0; i < SIZEOF(iconList); i++ )
		if ( iconList[i].defIconID == iconId )
			return ( HICON )CallService( MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)hIconLibItems[i] );

	return NULL;
}

HANDLE CIrcProto::GetIconHandle( int iconId )
{
	for ( int i=0; i < SIZEOF(iconList); i++ )
		if ( iconList[i].defIconID == iconId )
			return hIconLibItems[i];

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// code page handler

struct { UINT cpId; TCHAR *cpName; } static cpTable[] =
{
	{	  874, LPGENT("Thai") },
	{	  932, LPGENT("Japanese") },
	{	  936, LPGENT("Simplified Chinese") },
	{	  949, LPGENT("Korean") },
	{	  950, LPGENT("Traditional Chinese") },
	{	 1250, LPGENT("Central European") },
	{	 1251, LPGENT("Cyrillic (Windows)") },
	{	20866, LPGENT("Cyrillic (KOI8R)") },
	{	 1252, LPGENT("Latin I") },
	{	 1253, LPGENT("Greek") },
	{	 1254, LPGENT("Turkish") },
	{	 1255, LPGENT("Hebrew") },
	{	 1256, LPGENT("Arabic") },
	{	 1257, LPGENT("Baltic") },
	{	 1258, LPGENT("Vietnamese") },
	{	 1361, LPGENT("Korean (Johab)") }
};

static HWND sttCombo;

static BOOL CALLBACK sttLangAddCallback( CHAR* str )
{
	UINT cp = atoi(str);
	int i;
	for ( i=0; i < SIZEOF(cpTable) && cpTable[i].cpId != cp; i++ );
	if ( i < SIZEOF(cpTable)) {
		int idx = SendMessage( sttCombo, CB_ADDSTRING, 0, (LPARAM)TranslateTS( cpTable[i].cpName ));
		SendMessage( sttCombo, CB_SETITEMDATA, idx, cp );
	}
	return TRUE;
}

static void FillCodePageCombo( HWND hWnd, int defValue )
{
	int idx = SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM)TranslateT("Default ANSI codepage"));
	SendMessage( hWnd, CB_SETITEMDATA, idx, CP_ACP );

	idx = SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM)TranslateT("UTF-8"));
	SendMessage( hWnd, CB_SETITEMDATA, idx, CP_UTF8 );

	sttCombo = hWnd;
	EnumSystemCodePagesA(sttLangAddCallback, CP_INSTALLED);

	for ( int i = SendMessage( hWnd, CB_GETCOUNT, 0, 0 )-1; i >= 0; i-- ) {
		if ( SendMessage( hWnd, CB_GETITEMDATA, i, 0 ) == defValue ) {
			SendMessage( hWnd, CB_SETCURSEL, i, 0 );
			break;
	}	}

	#if !defined( _UNICODE )
		EnableWindow( hWnd, FALSE );
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Add server' dialog

struct CAddServerDlg : public CProtoDlgBase<CIrcProto>
{
	CAddServerDlg( CIrcProto* _pro, HWND parent ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_ADDSERVER, parent )
	{
		SetControlHandler( IDOK, &CAddServerDlg::OnCommand_Yes );
	}

	virtual void OnInitDialog()
	{
		int i = SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
		for (int index = 0; index <i; index++) {
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
			if (SendMessageA(GetDlgItem( m_hwnd, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
				SendMessageA(GetDlgItem( m_hwnd, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
		}

		if (m_ssleay32)
			CheckDlgButton( m_hwnd, IDC_OFF, BST_CHECKED);
		else {
			EnableWindow(GetDlgItem( m_hwnd, IDC_ON), FALSE);
			EnableWindow(GetDlgItem( m_hwnd, IDC_OFF), FALSE);
			EnableWindow(GetDlgItem( m_hwnd, IDC_AUTO), FALSE);
		}

		SetWindowTextA(GetDlgItem( m_hwnd, IDC_ADD_PORT), "6667");
		SetWindowTextA(GetDlgItem( m_hwnd, IDC_ADD_PORT2), "6667");
		SetFocus(GetDlgItem( m_hwnd, IDC_ADD_COMBO));
	}

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if (GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_COMBO))) {
			SERVER_INFO* pData = new SERVER_INFO;
			pData->m_iSSL = 0;
			if(IsDlgButtonChecked( m_hwnd, IDC_ON))
				pData->m_iSSL = 2;
			if(IsDlgButtonChecked( m_hwnd, IDC_AUTO))
				pData->m_iSSL = 1;

			pData->m_portEnd = getControlText( m_hwnd, IDC_ADD_PORT2 );
			pData->m_portStart = getControlText( m_hwnd, IDC_ADD_PORT );
			pData->Address = getControlText( m_hwnd, IDC_ADD_ADDRESS );
			pData->Group = getControlText( m_hwnd, IDC_ADD_COMBO );
			pData->m_name = getControlText( m_hwnd, IDC_ADD_SERVER );

			char temp[255];
			mir_snprintf( temp, sizeof(temp), "%s: %s", pData->Group, pData->m_name );
			mir_free( pData->m_name );
			pData->m_name = mir_strdup( temp );

			int iItem = SendMessageA(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->m_name);
			SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
			SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
			SendMessage(m_proto->m_hwndConnect, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);						
			Close();
			if ( SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR) {
				int m = SendMessageA(GetDlgItem(m_proto->m_hwndConnect, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
				SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
			}
			m_proto->m_serverlistModified = true;
		}
		else MessageBox( m_hwnd, TranslateT("Please complete all fields"), TranslateT("IRC error"), MB_OK | MB_ICONERROR );
	}

	virtual void OnClose()
	{
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), true);
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_ADDSERVER), true);
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_EDITSERVER), true);
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_DELETESERVER), true);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Edit server' dialog

struct CEditServerDlg : public CProtoDlgBase<CIrcProto>
{
	CEditServerDlg( CIrcProto* _pro, HWND parent ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_ADDSERVER, parent )
	{
		SetControlHandler( IDOK, &CEditServerDlg::OnCommand_Yes );
	}

	virtual void OnInitDialog()
	{
		int i = SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
		for (int index = 0; index <i; index++) {
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
			if (SendMessage(GetDlgItem( m_hwnd, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
				SendMessageA(GetDlgItem( m_hwnd, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
		}
		int j = SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
		SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETITEMDATA, j, 0);
		SetDlgItemTextA( m_hwnd, IDC_ADD_ADDRESS, pData->Address );
		SetDlgItemTextA( m_hwnd, IDC_ADD_COMBO, pData->Group );
		SetDlgItemTextA( m_hwnd, IDC_ADD_PORT, pData->m_portStart );
		SetDlgItemTextA( m_hwnd, IDC_ADD_PORT2, pData->m_portEnd );
		
		if ( m_ssleay32 ) {
			if ( pData->m_iSSL == 0 )
				CheckDlgButton( m_hwnd, IDC_OFF, BST_CHECKED );
			if ( pData->m_iSSL == 1 )
				CheckDlgButton( m_hwnd, IDC_AUTO, BST_CHECKED );
			if ( pData->m_iSSL == 2 )
				CheckDlgButton( m_hwnd, IDC_ON, BST_CHECKED );
		}
		else {
			EnableWindow(GetDlgItem( m_hwnd, IDC_ON), FALSE);
			EnableWindow(GetDlgItem( m_hwnd, IDC_OFF), FALSE);
			EnableWindow(GetDlgItem( m_hwnd, IDC_AUTO), FALSE);
		}

		char* p = strchr( pData->m_name, ' ' );
		if ( p )
			SetDlgItemTextA( m_hwnd, IDC_ADD_SERVER, p+1);

		SetFocus(GetDlgItem( m_hwnd, IDC_ADD_COMBO));	
	}

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if (GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_COMBO))) {
			int i = SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);

			delete ( SERVER_INFO* )SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);

			SERVER_INFO* pData = new SERVER_INFO;
			pData->m_iSSL = 0;
			if ( IsDlgButtonChecked( m_hwnd, IDC_ON ))
				pData->m_iSSL = 2;
			if ( IsDlgButtonChecked( m_hwnd, IDC_AUTO ))
				pData->m_iSSL = 1;

			pData->m_portEnd = getControlText( m_hwnd, IDC_ADD_PORT2 );
			pData->m_portStart = getControlText( m_hwnd, IDC_ADD_PORT );
			pData->Address = getControlText( m_hwnd, IDC_ADD_ADDRESS );
			pData->Group = getControlText( m_hwnd, IDC_ADD_COMBO );
			pData->m_name = getControlText( m_hwnd, IDC_ADD_SERVER );

			char temp[255];
			mir_snprintf( temp, sizeof(temp), "%s: %s", pData->Group, pData->m_name );
			mir_free( pData->m_name );
			pData->m_name = mir_strdup( temp );

			int iItem = SendMessageA(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->m_name);
			SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
			SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
			SendMessage(m_proto->m_hwndConnect, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
			Close();
			if ( SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR) {
				int m = SendMessageA(GetDlgItem(m_proto->m_hwndConnect, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
				SendMessage(GetDlgItem(m_proto->m_hwndConnect, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
			}
			m_proto->m_serverlistModified = true;
		}
		else MessageBox( m_hwnd, TranslateT("Please complete all fields"), TranslateT("IRC error"), MB_OK | MB_ICONERROR );
	}

	virtual void OnClose()
	{
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_SERVERCOMBO), true);
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_ADDSERVER), true);
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_EDITSERVER), true);
		EnableWindow(GetDlgItem(m_proto->m_hwndConnect, IDC_DELETESERVER), true);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// 'CTCP preferences' dialog

struct CCtcpPrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CCtcpPrefsDlg( CIrcProto* _pro ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_CTCP, NULL )
	{
		SetControlHandler( IDC_ENABLEIP, &CCtcpPrefsDlg::OnClicked );
		SetControlHandler( IDC_FROMSERVER, &CCtcpPrefsDlg::OnClicked );
	}

	static CDlgBase* Create( void* param ) { return new CCtcpPrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog()
	{
		SetDlgItemText( m_hwnd, IDC_USERINFO, m_proto->m_userInfo);

		CheckDlgButton( m_hwnd, IDC_SLOW, m_proto->getByte( "DCCMode", 0 ) == 0 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( m_hwnd, IDC_FAST, m_proto->getByte( "DCCMode", 0 ) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( m_hwnd, IDC_DISC, m_proto->m_disconnectDCCChats?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton( m_hwnd, IDC_PASSIVE, m_proto->m_DCCPassive?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton( m_hwnd, IDC_SENDNOTICE, m_proto->m_sendNotice?BST_CHECKED:BST_UNCHECKED);

		SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "256");
		SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "512");
		SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "1024");
		SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "2048");
		SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "4096");
		SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "8192");

		int j = m_proto->getWord( "DCCPacketSize", 1024*4 );
		char szTemp[10];
		mir_snprintf(szTemp, sizeof(szTemp), "%u", j);
		int i = SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) szTemp);
		if ( i == CB_ERR)
			int i = SendDlgItemMessageA( m_hwnd, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) "4096");

		if(m_proto->m_DCCChatAccept == 1)
			CheckDlgButton( m_hwnd, IDC_RADIO1, BST_CHECKED);
		if(m_proto->m_DCCChatAccept == 2)
			CheckDlgButton( m_hwnd, IDC_RADIO2, BST_CHECKED);
		if(m_proto->m_DCCChatAccept == 3)
			CheckDlgButton( m_hwnd, IDC_RADIO3, BST_CHECKED);

		CheckDlgButton( m_hwnd, IDC_FROMSERVER, m_proto->m_IPFromServer?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton( m_hwnd, IDC_ENABLEIP, m_proto->m_manualHost?BST_CHECKED:BST_UNCHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_IP), m_proto->m_manualHost);
		EnableWindow(GetDlgItem( m_hwnd, IDC_FROMSERVER), !m_proto->m_manualHost);
		if (m_proto->m_manualHost)
			SetDlgItemTextA( m_hwnd,IDC_IP,m_proto->m_mySpecifiedHost);
		else {
			if ( m_proto->m_IPFromServer ) {
				if ( m_proto->m_myHost[0] ) {
					TString s = (TString)TranslateT("<Resolved IP: ") + (TCHAR*)_A2T(m_proto->m_myHost) + _T(">");
					SetDlgItemText( m_hwnd, IDC_IP, s.c_str());
				}
				else SetDlgItemText( m_hwnd, IDC_IP, TranslateT( "<Automatic>" ));
			}
			else {
				if ( m_proto->m_myLocalHost[0] ) {
					TString s = ( TString )TranslateT( "<Local IP: " ) + (TCHAR*)_A2T(m_proto->m_myLocalHost) + _T(">");
					SetDlgItemText( m_hwnd, IDC_IP, s.c_str());
				}
				else SetDlgItemText( m_hwnd, IDC_IP, TranslateT( "<Automatic>" ));
	}	}	}

	void OnClicked(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_IP), IsDlgButtonChecked( m_hwnd, IDC_ENABLEIP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_FROMSERVER), IsDlgButtonChecked( m_hwnd, IDC_ENABLEIP)== BST_UNCHECKED);

		if ( IsDlgButtonChecked( m_hwnd, IDC_ENABLEIP ) == BST_CHECKED )
			SetDlgItemTextA( m_hwnd, IDC_IP, m_proto->m_mySpecifiedHost );
		else {
			if ( IsDlgButtonChecked( m_hwnd, IDC_FROMSERVER )== BST_CHECKED ) {
				if ( m_proto->m_myHost[0] ) {
					TString s = (TString)TranslateT( "<Resolved IP: ") + (TCHAR*)_A2T(m_proto->m_myHost) + _T(">");
					SetDlgItemText( m_hwnd, IDC_IP, s.c_str());
				}
				else SetDlgItemText( m_hwnd, IDC_IP, TranslateT( "<Automatic>" ));
			}
			else {
				if ( m_proto->m_myLocalHost[0] ) {
					TString s = ( TString )TranslateT( "<Local IP: " ) + (TCHAR*)_A2T(m_proto->m_myLocalHost) + _T(">");
					SetDlgItemText( m_hwnd, IDC_IP, s.c_str());
				}
				else SetDlgItemText( m_hwnd, IDC_IP, TranslateT( "<Automatic>" ));
	}	}	}

	virtual void OnApply()
	{
		GetDlgItemText( m_hwnd,IDC_USERINFO, m_proto->m_userInfo, 499);
		m_proto->setTString("UserInfo",m_proto->m_userInfo);

		char szTemp[10];
		GetWindowTextA(GetDlgItem( m_hwnd, IDC_COMBO), szTemp, 10);
		m_proto->setWord("DCCPacketSize", (WORD)atoi(szTemp));

		m_proto->m_DCCPassive = IsDlgButtonChecked( m_hwnd,IDC_PASSIVE)== BST_CHECKED?1:0;
		m_proto->setByte("DccPassive",m_proto->m_DCCPassive);

		m_proto->m_sendNotice = IsDlgButtonChecked( m_hwnd,IDC_SENDNOTICE)== BST_CHECKED?1:0;
		m_proto->setByte("SendNotice",m_proto->m_sendNotice);

		if ( IsDlgButtonChecked( m_hwnd,IDC_SLOW)== BST_CHECKED)
			m_proto->setByte("DCCMode",0);
		else 
			m_proto->setByte("DCCMode",1);

		m_proto->m_manualHost = IsDlgButtonChecked( m_hwnd,IDC_ENABLEIP)== BST_CHECKED?1:0;
		m_proto->setByte("ManualHost",m_proto->m_manualHost);

		m_proto->m_IPFromServer = IsDlgButtonChecked( m_hwnd,IDC_FROMSERVER)== BST_CHECKED?1:0;
		m_proto->setByte("IPFromServer",m_proto->m_IPFromServer);

		m_proto->m_disconnectDCCChats = IsDlgButtonChecked( m_hwnd,IDC_DISC)== BST_CHECKED?1:0;
		m_proto->setByte("DisconnectDCCChats",m_proto->m_disconnectDCCChats);

		if ( IsDlgButtonChecked( m_hwnd, IDC_ENABLEIP) == BST_CHECKED) {
			char szTemp[500];
			GetDlgItemTextA( m_hwnd,IDC_IP,szTemp, 499);
			lstrcpynA(m_proto->m_mySpecifiedHost, GetWord(szTemp, 0).c_str(), 499);
			m_proto->setString( "SpecHost", m_proto->m_mySpecifiedHost );
			if ( lstrlenA( m_proto->m_mySpecifiedHost ))
				mir_forkthread( ResolveIPThread, new IPRESOLVE( m_proto, m_proto->m_mySpecifiedHost, IP_MANUAL ));
		}

		if(IsDlgButtonChecked( m_hwnd, IDC_RADIO1) == BST_CHECKED)
			m_proto->m_DCCChatAccept = 1;
		if(IsDlgButtonChecked( m_hwnd, IDC_RADIO2) == BST_CHECKED)
			m_proto->m_DCCChatAccept = 2;
		if(IsDlgButtonChecked( m_hwnd, IDC_RADIO3) == BST_CHECKED)
			m_proto->m_DCCChatAccept = 3;
		m_proto->setByte("CtcpChatAccept",m_proto->m_DCCChatAccept);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// 'Advanced preferences' dialog

static char* sttPerformEvents[] = {
	"Event: Available",
	"Event: Away",
	"Event: N/A",
	"Event: Occupied",
	"Event: DND",
	"Event: Free for chat",
	"Event: On the phone",
	"Event: Out for lunch",
	"Event: Disconnect",
	"ALL NETWORKS"
};

static LRESULT CALLBACK EditSubclassProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
	switch(msg) { 
	case WM_CHAR :
		if (wParam == 21 || wParam == 11 || wParam == 2) {
			char w[2];
			w[1] = '\0';
			if (wParam == 11)
				w[0] = 3;
			if (wParam == 2)
				w[0] = 2;
			if (wParam == 21)
				w[0] = 31;
			SendMessage( hwndDlg, EM_REPLACESEL, false, (LPARAM) w);
			SendMessage( hwndDlg, EM_SCROLLCARET, 0,0);
			return 0;
		}
		break;
	} 

	return CallWindowProc(OldProc, hwndDlg, msg, wParam, lParam); 
} 

struct COtherPrefsDlg : public CProtoDlgBase<CIrcProto>
{
	COtherPrefsDlg( CIrcProto* _pro ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_OTHER, NULL )
	{
		SetControlHandler( IDC_CUSTOM, &COtherPrefsDlg::OnUrl );
		SetControlHandler( IDC_PERFORMCOMBO, &COtherPrefsDlg::OnPerformCombo );
		SetControlHandler( IDC_CODEPAGE, &COtherPrefsDlg::OnCodePage );
		SetControlHandler( IDC_PERFORMEDIT, &COtherPrefsDlg::OnPerformEdit );
		SetControlHandler( IDC_PERFORM, &COtherPrefsDlg::OnPerform );
		SetControlHandler( IDC_ADD, &COtherPrefsDlg::OnAdd );
		SetControlHandler( IDC_DELETE, &COtherPrefsDlg::OnDelete );
	}

	static CDlgBase* Create( void* param ) { return new COtherPrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog()
	{
		OldProc = (WNDPROC)SetWindowLong(GetDlgItem( m_hwnd, IDC_ALIASEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 
		SetWindowLong(GetDlgItem( m_hwnd, IDC_QUITMESSAGE), GWL_WNDPROC,(LONG)EditSubclassProc); 
		SetWindowLong(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 

		SendDlgItemMessage( m_hwnd,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_ADD));
		SendDlgItemMessage( m_hwnd,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_DELETE));
		SendMessage(GetDlgItem( m_hwnd,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Click to set commands that will be performed for this event"), 0);
		SendMessage(GetDlgItem( m_hwnd,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Click to delete the commands for this event"), 0);

		SetDlgItemText( m_hwnd, IDC_ALIASEDIT, m_proto->m_alias );
		SetDlgItemText( m_hwnd, IDC_QUITMESSAGE, m_proto->m_quitMessage );
		CheckDlgButton( m_hwnd, IDC_PERFORM, ((m_proto->m_perform) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd, IDC_SCRIPT, ((m_proto->m_scriptingEnabled) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( m_hwnd, IDC_SCRIPT), m_bMbotInstalled);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), m_proto->m_perform);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), m_proto->m_perform);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), m_proto->m_perform);
		EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), m_proto->m_perform);

		FillCodePageCombo( GetDlgItem( m_hwnd, IDC_CODEPAGE ), m_proto->m_codepage );
		if ( m_proto->m_codepage == CP_UTF8 )
			EnableWindow( GetDlgItem( m_hwnd, IDC_UTF_AUTODETECT ), FALSE );

		HWND hwndPerform = GetDlgItem( m_hwnd, IDC_PERFORMCOMBO );

		if ( m_proto->m_pszServerFile ) {
			char * p1 = m_proto->m_pszServerFile;
			char * p2 = m_proto->m_pszServerFile;

			while(strchr(p2, 'n')) {
				p1 = strstr(p2, "GROUP:");
				p1 = p1+ 6;
				p2 = strchr(p1, '\r');
				if (!p2)
					p2 = strchr(p1, '\n');
				if (!p2)
					p2 = strchr(p1, '\0');

				char * Group = new char[p2-p1+1];
				lstrcpynA(Group, p1, p2-p1+1);
				int i = SendDlgItemMessageA( m_hwnd, IDC_PERFORMCOMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Group);
				if (i == CB_ERR) {
					int idx = SendMessageA(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_ADDSTRING, 0, (LPARAM) Group);
					addPerformComboValue( hwndPerform, idx, Group );
				}

				delete []Group;
		}	}

		{
			for ( int i=0; i < SIZEOF(sttPerformEvents); i++ ) {
				int idx = SendMessage( hwndPerform, CB_INSERTSTRING, i, (LPARAM)TranslateTS(( TCHAR* )_A2T( sttPerformEvents[i] )));
				addPerformComboValue( hwndPerform, idx, sttPerformEvents[i] );
		}	}

		SendMessage( hwndPerform, CB_SETCURSEL, 0, 0);				
		SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_PERFORMCOMBO, CBN_SELCHANGE), 0);
		CheckDlgButton( m_hwnd, IDC_UTF_AUTODETECT, (m_proto->m_utfAutodetect) ? BST_CHECKED : BST_UNCHECKED );
		m_proto->m_performlistModified = false;
	}

	void OnUrl(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( idCode == STN_CLICKED )
			CallService( MS_UTILS_OPENURL,0,(LPARAM) "http://members.chello.se/matrix/index.html" );
	}

	void OnPerformCombo(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( idCode == CBN_SELCHANGE ) {
			int i = SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
			PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
			if (pPerf == 0)
				SetDlgItemTextA( m_hwnd, IDC_PERFORMEDIT, "");
			else
				SetDlgItemText( m_hwnd, IDC_PERFORMEDIT, pPerf->mText.c_str());
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), false);
			if ( GetWindowTextLength(GetDlgItem( m_hwnd, IDC_PERFORMEDIT)) != 0)
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), true );
			else
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), false );
	}	}

	void OnCodePage(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( idCode == CBN_SELCHANGE ) {
			int curSel = SendDlgItemMessage( m_hwnd, IDC_CODEPAGE, CB_GETCURSEL, 0, 0 );
			EnableWindow( GetDlgItem( m_hwnd, IDC_UTF_AUTODETECT ),
				SendDlgItemMessage( m_hwnd, IDC_CODEPAGE, CB_GETITEMDATA, curSel, 0 ) != CP_UTF8 );
	}	}

	void OnPerformEdit(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( idCode == EN_CHANGE ) {
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), true);

			if ( GetWindowTextLength(GetDlgItem( m_hwnd, IDC_PERFORMEDIT)) != 0)
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), true);
			else
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), false);
	}	}

	void OnPerform(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
	}

	void OnAdd(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		int j = GetWindowTextLength(GetDlgItem( m_hwnd, IDC_PERFORMEDIT));
		TCHAR* temp = new TCHAR[j+1];
		GetWindowText(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), temp, j+1);

		if ( my_strstri( temp, _T("/away")))
			MessageBox( NULL, TranslateT("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), TranslateT("IRC Error"), MB_OK);
		else {
			int i = ( int )SendMessage( GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0 );
			if ( i != CB_ERR ) {
				PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
				if ( pPerf != NULL )
					pPerf->mText = temp;

				EnableWindow( GetDlgItem( m_hwnd, IDC_ADD), false );
				m_proto->m_performlistModified = true;
		}	}
		delete []temp;
	}

	void OnDelete(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		int i = (int) SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
		if ( i != CB_ERR ) {
			PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
			if ( pPerf != NULL ) {
				pPerf->mText = _T("");
				SetDlgItemTextA( m_hwnd, IDC_PERFORMEDIT, "");
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), false);
				EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), false);
			}

			m_proto->m_performlistModified = true;
	}	}

	virtual void OnDestroy()
	{
		m_proto->m_performlistModified = false;

		int i = SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETCOUNT, 0, 0);
		if ( i != CB_ERR && i != 0 ) {
			for (int index = 0; index < i; index++) {
				PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETITEMDATA, index, 0);
				if (( const int )pPerf != CB_ERR && pPerf != NULL )
					delete pPerf;
	}	}	}

	virtual void OnApply()
	{
		mir_free( m_proto->m_alias );
		{	int len = GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ALIASEDIT));
			m_proto->m_alias = ( TCHAR* )mir_alloc( sizeof( TCHAR )*( len+1 ));
         GetDlgItemText( m_hwnd, IDC_ALIASEDIT, m_proto->m_alias, len+1 );
		}
		m_proto->setTString( "Alias", m_proto->m_alias );

		GetDlgItemText( m_hwnd,IDC_QUITMESSAGE,m_proto->m_quitMessage, 399 );
		m_proto->setTString( "QuitMessage", m_proto->m_quitMessage );
		{
			int curSel = SendDlgItemMessage( m_hwnd, IDC_CODEPAGE, CB_GETCURSEL, 0, 0 );
			m_proto->m_codepage = SendDlgItemMessage( m_hwnd, IDC_CODEPAGE, CB_GETITEMDATA, curSel, 0 );
			m_proto->setDword( "Codepage", m_proto->m_codepage );
			if ( m_proto->IsConnected() )
				m_proto->setCodepage( m_proto->m_codepage );
		}

		m_proto->m_utfAutodetect = IsDlgButtonChecked( m_hwnd,IDC_UTF_AUTODETECT)== BST_CHECKED;
		m_proto->setByte("UtfAutodetect",m_proto->m_utfAutodetect);
		m_proto->m_perform = IsDlgButtonChecked( m_hwnd,IDC_PERFORM)== BST_CHECKED;
		m_proto->setByte("Perform",m_proto->m_perform);
		m_proto->m_scriptingEnabled = IsDlgButtonChecked( m_hwnd,IDC_SCRIPT)== BST_CHECKED;
		m_proto->setByte("ScriptingEnabled",m_proto->m_scriptingEnabled);
		if (IsWindowEnabled(GetDlgItem( m_hwnd, IDC_ADD)))
			SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_ADD, BN_CLICKED), 0);
      
		if ( m_proto->m_performlistModified ) {
			m_proto->m_performlistModified = false;

			int count = SendDlgItemMessage( m_hwnd, IDC_PERFORMCOMBO, CB_GETCOUNT, 0, 0);
			for ( int i = 0; i < count; i++ ) {
				PERFORM_INFO* pPerf = ( PERFORM_INFO* )SendMessage( GetDlgItem( m_hwnd, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0 );
				if (( const int )pPerf == CB_ERR )
					continue;

				if ( !pPerf->mText.empty())
					m_proto->setTString( pPerf->mSetting.c_str(), pPerf->mText.c_str());
				else 
					DBDeleteContactSetting( NULL, m_proto->m_szModuleName, pPerf->mSetting.c_str());
	}	}	}

	void addPerformComboValue( HWND hWnd, int idx, const char* szValueName )
	{
		String sSetting = String("PERFORM:") + szValueName;
		transform( sSetting.begin(), sSetting.end(), sSetting.begin(), toupper );

		PERFORM_INFO* pPref;
		DBVARIANT dbv;
		if ( !m_proto->getTString( sSetting.c_str(), &dbv )) {
			pPref = new PERFORM_INFO( sSetting.c_str(), dbv.ptszVal );
			DBFreeVariant( &dbv );
		}
		else pPref = new PERFORM_INFO( sSetting.c_str(), _T(""));
		SendMessage( hWnd, CB_SETITEMDATA, idx, ( LPARAM )pPref );
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Connect preferences' dialog

struct CConnectPrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlCheck m_JoinOnInvite;

	CConnectPrefsDlg( CIrcProto* _pro ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_CONNECT, NULL ),
		m_JoinOnInvite( this, IDC_AUTOJOIN )
	{
		CreateLink( m_JoinOnInvite, "JoinOnInvite", DBVT_BYTE, 0 );

		SetControlHandler( IDC_SERVERCOMBO, &CConnectPrefsDlg::OnServerCombo );
		SetControlHandler( IDC_ADDSERVER, &CConnectPrefsDlg::OnAddServer );
		SetControlHandler( IDC_DELETESERVER, &CConnectPrefsDlg::OnDeleteServer );
		SetControlHandler( IDC_EDITSERVER, &CConnectPrefsDlg::OnEditServer );
		SetControlHandler( IDC_STARTUP, &CConnectPrefsDlg::OnStartup );
		SetControlHandler( IDC_IDENT, &CConnectPrefsDlg::OnIdent );
		SetControlHandler( IDC_USESERVER, &CConnectPrefsDlg::OnUseServer );
		SetControlHandler( IDC_ONLINENOTIF, &CConnectPrefsDlg::OnOnlineNotif );
		SetControlHandler( IDC_CHANNELAWAY, &CConnectPrefsDlg::OnChannelAway );
		SetControlHandler( IDC_RETRY, &CConnectPrefsDlg::OnRetry );
	}

	static CDlgBase* Create( void* param ) { return new CConnectPrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog()
	{
		SendDlgItemMessage( m_hwnd,IDC_ADDSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_ADD));
		SendDlgItemMessage( m_hwnd,IDC_DELETESERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_DELETE));
		SendDlgItemMessage( m_hwnd,IDC_EDITSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_RENAME));
		SendMessage(GetDlgItem( m_hwnd,IDC_ADDSERVER), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Add a new network"), 0);
		SendMessage(GetDlgItem( m_hwnd,IDC_EDITSERVER), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Edit this network"), 0);
		SendMessage(GetDlgItem( m_hwnd,IDC_DELETESERVER), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Delete this network"), 0);

		m_proto->m_hwndConnect = m_hwnd;

		//	Fill the servers combo box and create SERVER_INFO structures
		if ( m_proto->m_pszServerFile ) {
			char* p1 = m_proto->m_pszServerFile;
			char* p2 = m_proto->m_pszServerFile;
			while (strchr(p2, 'n')) {
				SERVER_INFO* pData = new SERVER_INFO;
				p1 = strchr(p2, '=');
				++p1;
				p2 = strstr(p1, "SERVER:");
				pData->m_name = ( char* )mir_alloc( p2-p1+1 );
				lstrcpynA(pData->m_name, p1, p2-p1+1);

				p1 = strchr(p2, ':');
				++p1;
				pData->m_iSSL = 0;
				if(strstr(p1, "SSL") == p1) {
					p1 +=3;
					if(*p1 == '1')
						pData->m_iSSL = 1;
					else if(*p1 == '2')
						pData->m_iSSL = 2;
					p1++;
				}
				p2 = strchr(p1, ':');
				pData->Address = ( char* )mir_alloc( p2-p1+1 );
				lstrcpynA( pData->Address, p1, p2-p1+1 );

				p1 = p2;
				p1++;
				while (*p2 !='G' && *p2 != '-')
					p2++;
				pData->m_portStart = ( char* )mir_alloc( p2-p1+1 );
				lstrcpynA( pData->m_portStart, p1, p2-p1+1 );

				if (*p2 == 'G'){
					pData->m_portEnd = ( char* )mir_alloc( p2-p1+1 );
					lstrcpyA( pData->m_portEnd, pData->m_portStart );
				}
				else {
					p1 = p2;
					p1++;
					p2 = strchr(p1, 'G');
					pData->m_portEnd = ( char* )mir_alloc( p2-p1+1 );
					lstrcpynA(pData->m_portEnd, p1, p2-p1+1);
				}

				p1 = strchr(p2, ':');
				p1++;
				p2 = strchr(p1, '\r');
				if (!p2)
					p2 = strchr(p1, '\n');
				if (!p2)
					p2 = strchr(p1, '\0');
				pData->Group = ( char* )mir_alloc( p2-p1+1 );
				lstrcpynA(pData->Group, p1, p2-p1+1);
				int iItem = SendDlgItemMessageA( m_hwnd, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->m_name);
				SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);
		}	}

		SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_SETCURSEL, m_proto->m_serverComboSelection - 1,0);				
		SetDlgItemTextA( m_hwnd,IDC_SERVER, m_proto->m_serverName);
		SetDlgItemTextA( m_hwnd,IDC_PORT, m_proto->m_portStart);
		SetDlgItemTextA( m_hwnd,IDC_PORT2, m_proto->m_portEnd);
		if ( m_ssleay32 ) {
			if ( m_proto->m_iSSL == 0 )
				SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "Off" ));
			if ( m_proto->m_iSSL == 1 )
				SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "Auto" ));
			if ( m_proto->m_iSSL == 2 )
				SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "On" ));
		}
		else SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "N/A" ));

		if ( m_proto->m_serverComboSelection != -1 ) {
			SERVER_INFO* pData = ( SERVER_INFO* )SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETITEMDATA, m_proto->m_serverComboSelection - 1, 0);
			if ((int)pData != CB_ERR) {
				SetDlgItemTextA( m_hwnd, IDC_SERVER, pData->Address   );
				SetDlgItemTextA( m_hwnd, IDC_PORT,   pData->m_portStart );
				SetDlgItemTextA( m_hwnd, IDC_PORT2,  pData->m_portEnd   );
		}	}

		SendDlgItemMessage( m_hwnd,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(999,20));
		SendDlgItemMessage( m_hwnd,IDC_SPIN1,UDM_SETPOS,0,MAKELONG(m_proto->m_onlineNotificationTime,0));
		SendDlgItemMessage( m_hwnd,IDC_SPIN2,UDM_SETRANGE,0,MAKELONG(200,0));
		SendDlgItemMessage( m_hwnd,IDC_SPIN2,UDM_SETPOS,0,MAKELONG(m_proto->m_onlineNotificationLimit,0));
		SetDlgItemText( m_hwnd, IDC_NICK, m_proto->m_nick);
		SetDlgItemText( m_hwnd, IDC_NICK2, m_proto->m_alternativeNick);
		SetDlgItemText( m_hwnd, IDC_USERID, m_proto->m_userID);
		SetDlgItemText( m_hwnd, IDC_NAME, m_proto->m_name);
		SetDlgItemTextA( m_hwnd, IDC_PASS, m_proto->m_password);
		SetDlgItemText( m_hwnd, IDC_IDENTSYSTEM, m_proto->m_identSystem);
		SetDlgItemText( m_hwnd, IDC_IDENTPORT, m_proto->m_identPort);
		SetDlgItemText( m_hwnd, IDC_RETRYWAIT, m_proto->m_retryWait);
		SetDlgItemText( m_hwnd, IDC_RETRYCOUNT, m_proto->m_retryCount);			
		CheckDlgButton( m_hwnd, IDC_ADDRESS, (( m_proto->m_showAddresses) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd, IDC_OLDSTYLE, (( m_proto->m_oldStyleModes) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd, IDC_CHANNELAWAY, (( m_proto->m_channelAwayNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd, IDC_ONLINENOTIF, (( m_proto->m_autoOnlineNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( m_hwnd, IDC_ONLINETIMER), m_proto->m_autoOnlineNotification);
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHANNELAWAY), m_proto->m_autoOnlineNotification);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SPIN1), m_proto->m_autoOnlineNotification);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SPIN2), m_proto->m_autoOnlineNotification && m_proto->m_channelAwayNotification);
		EnableWindow(GetDlgItem( m_hwnd, IDC_LIMIT), m_proto->m_autoOnlineNotification && m_proto->m_channelAwayNotification);
		CheckDlgButton( m_hwnd,IDC_IDENT, ((m_proto->m_ident) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( m_hwnd, IDC_IDENTSYSTEM), m_proto->m_ident);
		EnableWindow(GetDlgItem( m_hwnd, IDC_IDENTPORT), m_proto->m_ident);
		CheckDlgButton( m_hwnd,IDC_DISABLEERROR, ((m_proto->m_disableErrorPopups) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd,IDC_FORCEVISIBLE, ((m_proto->m_forceVisible) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd,IDC_REJOINCHANNELS, ((m_proto->m_rejoinChannels) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd,IDC_REJOINONKICK, ((m_proto->m_rejoinIfKicked) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( m_hwnd,IDC_RETRY, ((m_proto->m_retry) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( m_hwnd, IDC_RETRYWAIT), m_proto->m_retry);
		EnableWindow(GetDlgItem( m_hwnd, IDC_RETRYCOUNT), m_proto->m_retry);
		CheckDlgButton( m_hwnd,IDC_STARTUP, ((!m_proto->m_disableDefaultServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( m_hwnd,IDC_KEEPALIVE, ((m_proto->m_sendKeepAlive) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( m_hwnd,IDC_IDENT_TIMED, ((m_proto->IdentTimer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( m_hwnd,IDC_USESERVER, ((m_proto->m_useServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( m_hwnd,IDC_SHOWSERVER, ((!m_proto->m_hideServerWindow) ? (BST_CHECKED) : (BST_UNCHECKED)));				
//		CheckDlgButton( m_hwnd,IDC_AUTOJOIN, ((m_proto->m_joinOnInvite) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		EnableWindow(GetDlgItem( m_hwnd, IDC_SHOWSERVER), m_proto->m_useServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADDSERVER), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_EDITSERVER), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_DELETESERVER), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SERVER), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PORT), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PORT2), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PASS), !m_proto->m_disableDefaultServer);
		EnableWindow(GetDlgItem( m_hwnd, IDC_IDENT_TIMED), m_proto->m_ident);
		m_proto->m_serverlistModified = false;
	}

	void OnServerCombo(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( idCode == CBN_SELCHANGE ) {
			int i = SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			if (pData && (int)pData != CB_ERR ) {
				SetDlgItemTextA( m_hwnd,IDC_SERVER, pData->Address );
				SetDlgItemTextA( m_hwnd,IDC_PORT,   pData->m_portStart );
				SetDlgItemTextA( m_hwnd,IDC_PORT2,  pData->m_portEnd );
				SetDlgItemTextA( m_hwnd,IDC_PASS,   "" );
				if ( m_ssleay32 ) {
					if ( pData->m_iSSL == 0 )
						SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "Off" ));
					if ( pData->m_iSSL == 1 )
						SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "Auto" ));
					if ( pData->m_iSSL == 2 )
						SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "On" ));
				}
				else SetDlgItemText( m_hwnd, IDC_SSL, TranslateT( "N/A" ));
				SendMessage(GetParent( m_hwnd), PSM_CHANGED,0,0);
	}	}	}

	void OnAddServer(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), false);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADDSERVER), false);
		EnableWindow(GetDlgItem( m_hwnd, IDC_EDITSERVER), false);
		EnableWindow(GetDlgItem( m_hwnd, IDC_DELETESERVER), false);
		CAddServerDlg* dlg = new CAddServerDlg( m_proto, m_hwnd );
		dlg->Show();
		m_proto->m_hwndAddServer = dlg->GetHwnd();
	}

	void OnDeleteServer(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		int i = SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
		if (i != CB_ERR) {
			EnableWindow(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADDSERVER), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDITSERVER), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_DELETESERVER), false);
			
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			TCHAR temp[200];
			mir_sntprintf( temp, SIZEOF(temp), TranslateT("Do you want to delete\r\n%s"), (TCHAR*)_A2T(pData->m_name));
			if ( MessageBox( m_hwnd, temp, TranslateT("Delete server"), MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
				delete pData;	

				SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);
				if (i >= SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0))
					i--;
				SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_SETCURSEL, i, 0);
				SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
				m_proto->m_serverlistModified = true;
			}

			EnableWindow(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), true);
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADDSERVER), true);
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDITSERVER), true);
			EnableWindow(GetDlgItem( m_hwnd, IDC_DELETESERVER), true);
	}	}

	void OnEditServer(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		int i = SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
		if (i != CB_ERR) {
			EnableWindow(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADDSERVER), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDITSERVER), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_DELETESERVER), false);
			CEditServerDlg* dlg = new CEditServerDlg( m_proto, m_hwnd );
			dlg->Show();
			m_proto->m_hwndAddServer = dlg->GetHwnd();
			SetWindowText( m_proto->m_hwndAddServer, TranslateT( "Edit server" ));
	}	}

	void OnStartup(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADDSERVER), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_EDITSERVER), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_DELETESERVER), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SERVER), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PORT), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PORT2), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_PASS), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL), IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED);
	}

	void OnIdent(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_IDENTSYSTEM), IsDlgButtonChecked( m_hwnd, IDC_IDENT)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_IDENTPORT), IsDlgButtonChecked( m_hwnd, IDC_IDENT)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_IDENT_TIMED), IsDlgButtonChecked( m_hwnd, IDC_IDENT)== BST_CHECKED);
	}

	void OnUseServer(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_SHOWSERVER), IsDlgButtonChecked( m_hwnd, IDC_USESERVER)== BST_CHECKED);
	}

	void OnOnlineNotif(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHANNELAWAY), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ONLINETIMER), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SPIN1), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SPIN2), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( m_hwnd, IDC_CHANNELAWAY)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_LIMIT), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( m_hwnd, IDC_CHANNELAWAY)== BST_CHECKED);
	}

	void OnChannelAway(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{	
		EnableWindow(GetDlgItem( m_hwnd, IDC_SPIN2), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( m_hwnd, IDC_CHANNELAWAY)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_LIMIT), IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( m_hwnd, IDC_CHANNELAWAY)== BST_CHECKED);
	}

	void OnRetry(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{	
		EnableWindow(GetDlgItem( m_hwnd, IDC_RETRYWAIT), IsDlgButtonChecked( m_hwnd, IDC_RETRY)== BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_RETRYCOUNT), IsDlgButtonChecked( m_hwnd, IDC_RETRY)== BST_CHECKED);
	}

	virtual void OnApply()
	{
		//Save the setting in the CONNECT dialog
		if(IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED) {
			GetDlgItemTextA( m_hwnd,IDC_SERVER, m_proto->m_serverName, 99);
			m_proto->setString("ServerName",m_proto->m_serverName);
			GetDlgItemTextA( m_hwnd,IDC_PORT, m_proto->m_portStart, 6);
			m_proto->setString("PortStart",m_proto->m_portStart);
			GetDlgItemTextA( m_hwnd,IDC_PORT2, m_proto->m_portEnd, 6);
			m_proto->setString("PortEnd",m_proto->m_portEnd);
			GetDlgItemTextA( m_hwnd,IDC_PASS, m_proto->m_password, 500);
			CallService( MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)m_proto->m_password);
			m_proto->setString("Password",m_proto->m_password);
			CallService( MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)m_proto->m_password);
		}
		else {
			lstrcpyA(m_proto->m_serverName, "");
			m_proto->setString("ServerName",m_proto->m_serverName);
			lstrcpyA(m_proto->m_portStart, "");
			m_proto->setString("PortStart",m_proto->m_portStart);
			lstrcpyA(m_proto->m_portEnd, "");
			m_proto->setString("PortEnd",m_proto->m_portEnd);
			lstrcpyA( m_proto->m_password, "");
			m_proto->setString("Password",m_proto->m_password);
		}

		m_proto->m_onlineNotificationTime = SendDlgItemMessage( m_hwnd,IDC_SPIN1,UDM_GETPOS,0,0);
		m_proto->setWord("OnlineNotificationTime", (BYTE)m_proto->m_onlineNotificationTime);

		m_proto->m_onlineNotificationLimit = SendDlgItemMessage( m_hwnd,IDC_SPIN2,UDM_GETPOS,0,0);
		m_proto->setWord("OnlineNotificationLimit", (BYTE)m_proto->m_onlineNotificationLimit);

		m_proto->m_channelAwayNotification = IsDlgButtonChecked( m_hwnd, IDC_CHANNELAWAY )== BST_CHECKED;
		m_proto->setByte("ChannelAwayNotification",m_proto->m_channelAwayNotification);

		GetDlgItemText( m_hwnd,IDC_NICK, m_proto->m_nick, 30);
		removeSpaces(m_proto->m_nick);
		m_proto->setTString("PNick",m_proto->m_nick);
		m_proto->setTString("Nick",m_proto->m_nick);
		GetDlgItemText( m_hwnd,IDC_NICK2, m_proto->m_alternativeNick, 30);
		removeSpaces(m_proto->m_alternativeNick);
		m_proto->setTString("AlernativeNick",m_proto->m_alternativeNick);
		GetDlgItemText( m_hwnd,IDC_USERID, m_proto->m_userID, 199);
		removeSpaces(m_proto->m_userID);
		m_proto->setTString("UserID",m_proto->m_userID);
		GetDlgItemText( m_hwnd,IDC_NAME, m_proto->m_name, 199);
		m_proto->setTString("Name",m_proto->m_name);
		GetDlgItemText( m_hwnd,IDC_IDENTSYSTEM, m_proto->m_identSystem, 10);
		m_proto->setTString("IdentSystem",m_proto->m_identSystem);
		GetDlgItemText( m_hwnd,IDC_IDENTPORT, m_proto->m_identPort, 6);
		m_proto->setTString("IdentPort",m_proto->m_identPort);
		GetDlgItemText( m_hwnd,IDC_RETRYWAIT, m_proto->m_retryWait, 4);
		m_proto->setTString("RetryWait",m_proto->m_retryWait);
		GetDlgItemText( m_hwnd,IDC_RETRYCOUNT, m_proto->m_retryCount, 4);
		m_proto->setTString("RetryCount",m_proto->m_retryCount);
		m_proto->m_disableDefaultServer = !IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED;
		m_proto->setByte("DisableDefaultServer",m_proto->m_disableDefaultServer);
		m_proto->m_ident = IsDlgButtonChecked( m_hwnd, IDC_IDENT)== BST_CHECKED;
		m_proto->setByte("Ident",m_proto->m_ident);
		m_proto->IdentTimer = IsDlgButtonChecked( m_hwnd, IDC_IDENT_TIMED)== BST_CHECKED;
		m_proto->setByte("IdentTimer",m_proto->IdentTimer);
		m_proto->m_forceVisible = IsDlgButtonChecked( m_hwnd, IDC_FORCEVISIBLE)== BST_CHECKED;
		m_proto->setByte("ForceVisible",m_proto->m_forceVisible);
		m_proto->m_disableErrorPopups = IsDlgButtonChecked( m_hwnd, IDC_DISABLEERROR)== BST_CHECKED;
		m_proto->setByte("DisableErrorPopups",m_proto->m_disableErrorPopups);
		m_proto->m_rejoinChannels = IsDlgButtonChecked( m_hwnd, IDC_REJOINCHANNELS)== BST_CHECKED;
		m_proto->setByte("RejoinChannels",m_proto->m_rejoinChannels);
		m_proto->m_rejoinIfKicked = IsDlgButtonChecked( m_hwnd, IDC_REJOINONKICK)== BST_CHECKED;
		m_proto->setByte("RejoinIfKicked",m_proto->m_rejoinIfKicked);
		m_proto->m_retry = IsDlgButtonChecked( m_hwnd, IDC_RETRY)== BST_CHECKED;
		m_proto->setByte("Retry",m_proto->m_retry);
		m_proto->m_showAddresses = IsDlgButtonChecked( m_hwnd, IDC_ADDRESS)== BST_CHECKED;
		m_proto->setByte("ShowAddresses",m_proto->m_showAddresses);
		m_proto->m_oldStyleModes = IsDlgButtonChecked( m_hwnd, IDC_OLDSTYLE)== BST_CHECKED;
		m_proto->setByte("OldStyleModes",m_proto->m_oldStyleModes);

		m_proto->m_useServer = IsDlgButtonChecked( m_hwnd, IDC_USESERVER )== BST_CHECKED;
		m_proto->setByte("UseServer",m_proto->m_useServer);

		CLISTMENUITEM clmi;
		memset( &clmi, 0, sizeof( clmi ));
		clmi.cbSize = sizeof( clmi );
		clmi.flags = CMIM_FLAGS;
		if ( !m_proto->m_useServer )
			clmi.flags |= CMIF_GRAYED;
		CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_proto->hMenuServer, ( LPARAM )&clmi );

//		m_proto->m_joinOnInvite = IsDlgButtonChecked( m_hwnd, IDC_AUTOJOIN )== BST_CHECKED;
//		m_proto->setByte("JoinOnInvite",m_proto->m_joinOnInvite);
		m_proto->m_hideServerWindow = IsDlgButtonChecked( m_hwnd, IDC_SHOWSERVER )== BST_UNCHECKED;
		m_proto->setByte("HideServerWindow",m_proto->m_hideServerWindow);
		m_proto->m_serverComboSelection = SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0) + 1;
		m_proto->setDword("ServerComboSelection",m_proto->m_serverComboSelection);
		m_proto->m_sendKeepAlive = IsDlgButtonChecked( m_hwnd, IDC_KEEPALIVE )== BST_CHECKED;
		m_proto->setByte("SendKeepAlive",m_proto->m_sendKeepAlive);
		if (m_proto->m_sendKeepAlive)
			m_proto->SetChatTimer(m_proto->KeepAliveTimer, 60*1000, KeepAliveTimerProc);
		else
			m_proto->KillChatTimer(m_proto->KeepAliveTimer);

		m_proto->m_autoOnlineNotification = IsDlgButtonChecked( m_hwnd, IDC_ONLINENOTIF )== BST_CHECKED;
		m_proto->setByte("AutoOnlineNotification",m_proto->m_autoOnlineNotification);
		if (m_proto->m_autoOnlineNotification) {
			if( !m_proto->bTempDisableCheck) {
				m_proto->SetChatTimer(m_proto->OnlineNotifTimer, 500, OnlineNotifTimerProc);
				if(m_proto->m_channelAwayNotification)
					m_proto->SetChatTimer(m_proto->OnlineNotifTimer3, 1500, OnlineNotifTimerProc3);
			}
		}
		else if (!m_proto->bTempForceCheck) {
			m_proto->KillChatTimer(m_proto->OnlineNotifTimer);
			m_proto->KillChatTimer(m_proto->OnlineNotifTimer3);
		}

		int i = SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
		SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
		if ( pData && (int)pData != CB_ERR ) {
			if(IsDlgButtonChecked( m_hwnd, IDC_STARTUP)== BST_CHECKED)
				lstrcpyA(m_proto->m_network, pData->Group); 
			else
				lstrcpyA(m_proto->m_network, ""); 
			m_proto->setString("Network",m_proto->m_network);
			m_proto->m_iSSL = pData->m_iSSL;
			m_proto->setByte("UseSSL",pData->m_iSSL);			
		}

		if (m_proto->m_serverlistModified) {
			m_proto->m_serverlistModified = false;
			char filepath[MAX_PATH];
			mir_snprintf(filepath, sizeof(filepath), "%s\\%s_servers.ini", mirandapath, m_proto->m_szModuleName);
			FILE *hFile2 = fopen(filepath,"w");
			if (hFile2) {
				int j = (int) SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
				if (j !=CB_ERR && j !=0) {
					for (int index2 = 0; index2 < j; index2++) {
						SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
						if (pData != NULL && (int)pData != CB_ERR) {
							char TextLine[512];
							if(pData->m_iSSL > 0)
								mir_snprintf(TextLine, sizeof(TextLine), "n%u=%sSERVER:SSL%u%s:%s-%sGROUP:%s\n", index2, pData->m_name, pData->m_iSSL, pData->Address, pData->m_portStart, pData->m_portEnd, pData->Group);
							else
								mir_snprintf(TextLine, sizeof(TextLine), "n%u=%sSERVER:%s:%s-%sGROUP:%s\n", index2, pData->m_name, pData->Address, pData->m_portStart, pData->m_portEnd, pData->Group);
							fputs(TextLine, hFile2);
				}	}	}

				fclose(hFile2);	
				delete [] m_proto->m_pszServerFile;
				m_proto->m_pszServerFile = IrcLoadFile(filepath);
	}	}	}

	virtual void OnDestroy()
	{
		m_proto->m_serverlistModified = false;
		int j = (int) SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
		if (j !=CB_ERR && j !=0 ) {
			for (int index2 = 0; index2 < j; index2++) {
				SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( m_hwnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
				if ( pData != NULL && (int)pData != CB_ERR )
					delete pData;	
	}	}	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// 'm_ignore' preferences dialog

struct CAddIgnoreDlg : public CProtoDlgBase<CIrcProto>
{
	TCHAR szOldMask[500];

	CAddIgnoreDlg( CIrcProto* _pro, const TCHAR* mask, HWND parent ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_ADDIGNORE, parent )
	{
		if ( mask == NULL )
			szOldMask[0] = 0;
		else
			_tcsncpy( szOldMask, mask, SIZEOF(szOldMask));

		SetControlHandler( IDOK, &CAddIgnoreDlg::OnOk );
	}

	virtual void OnInitDialog()
	{
		if ( szOldMask[0] == 0 ) {
			if ( m_proto->IsConnected())
				SetWindowText(GetDlgItem( m_hwnd, IDC_NETWORK), m_proto->m_info.sNetwork.c_str());
			CheckDlgButton( m_hwnd, IDC_Q, BST_CHECKED);
			CheckDlgButton( m_hwnd, IDC_N, BST_CHECKED);
			CheckDlgButton( m_hwnd, IDC_I, BST_CHECKED);
			CheckDlgButton( m_hwnd, IDC_D, BST_CHECKED);
			CheckDlgButton( m_hwnd, IDC_C, BST_CHECKED);
	}	}

	void OnOk(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		TCHAR szMask[500];
		TCHAR szNetwork[500];
		TString flags;
		if ( IsDlgButtonChecked( m_hwnd, IDC_Q ) == BST_CHECKED ) flags += 'q';
		if ( IsDlgButtonChecked( m_hwnd, IDC_N ) == BST_CHECKED ) flags += 'n';
		if ( IsDlgButtonChecked( m_hwnd, IDC_I ) == BST_CHECKED ) flags += 'i';
		if ( IsDlgButtonChecked( m_hwnd, IDC_D ) == BST_CHECKED ) flags += 'd';
		if ( IsDlgButtonChecked( m_hwnd, IDC_C ) == BST_CHECKED ) flags += 'c';
		if ( IsDlgButtonChecked( m_hwnd, IDC_M ) == BST_CHECKED ) flags += 'm';

		GetWindowText( GetDlgItem( m_hwnd, IDC_MASK), szMask, SIZEOF(szMask));
		GetWindowText( GetDlgItem( m_hwnd, IDC_NETWORK), szNetwork, SIZEOF(szNetwork));

		TString Mask = GetWord(szMask, 0);
		if ( Mask.length() != 0 ) {
			if ( !_tcschr(Mask.c_str(), '!') && !_tcschr(Mask.c_str(), '@'))
				Mask += _T("!*@*");

			if ( !flags.empty() ) {
				if ( *szOldMask )
					m_proto->RemoveIgnore( szOldMask );
				m_proto->AddIgnore(Mask.c_str(), flags.c_str(), szNetwork);
	}	}	}

	virtual void OnClose()
	{
		PostMessage( m_proto->m_hwndIgnore, IRC_FIXIGNOREBUTTONS, 0, 0 );
		DestroyWindow( m_hwnd);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// 'm_ignore' preferences dialog

static int CALLBACK IgnoreListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CIrcProto* ppro = ( CIrcProto* )lParamSort;
	if ( !ppro->m_hwndIgnore )
		return 1;

	TCHAR temp1[512];
	TCHAR temp2[512];

	LVITEM lvm;
	lvm.mask = LVIF_TEXT;
	lvm.iSubItem = 0;
	lvm.cchTextMax = SIZEOF(temp1);

	lvm.iItem = lParam1;
	lvm.pszText = temp1;
	ListView_GetItem( GetDlgItem(ppro->m_hwndIgnore, IDC_INFO_LISTVIEW), &lvm );

	lvm.iItem = lParam2;
	lvm.pszText = temp2;
	ListView_GetItem( GetDlgItem(ppro->m_hwndIgnore, IDC_INFO_LISTVIEW), &lvm );
	
	if ( temp1[0] && temp2[0] )
		return lstrcmpi( temp1, temp2 );

	return ( temp1[0] == 0 ) ? 1 : -1;
}

static LRESULT CALLBACK ListviewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
	switch (msg) {
	case WM_KEYUP :
		if ( ListView_GetSelectionMark(GetDlgItem(GetParent(hwnd), IDC_LIST)) != -1) {
			EnableWindow(GetDlgItem(GetParent(hwnd), IDC_EDIT), true);
			EnableWindow(GetDlgItem(GetParent(hwnd), IDC_DELETE), true);
		}
		else {
			EnableWindow(GetDlgItem(GetParent(hwnd), IDC_EDIT), false);
			EnableWindow(GetDlgItem(GetParent(hwnd), IDC_DELETE), false);
		}

		if (wParam == VK_DELETE)
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_DELETE, BN_CLICKED), 0);

		if (wParam == VK_SPACE)
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
		break;
	} 

	return CallWindowProc(OldListViewProc, hwnd, msg, wParam, lParam); 
}

// Callback for the 'Add ignore' dialog

void CIrcProto::InitIgnore( void )
{
	char szTemp[ MAX_PATH ];
	mir_snprintf(szTemp, sizeof(szTemp), "%s\\%s_ignore.ini", mirandapath, m_szModuleName);
	char* pszIgnoreData = IrcLoadFile(szTemp);
	if ( pszIgnoreData != NULL ) {
		char *p1 = pszIgnoreData;
		while ( *p1 != '\0' ) {
			while ( *p1 == '\r' || *p1 == '\n' )
				p1++;
			if ( *p1 == '\0' )
				break;

			char* p2 = strstr( p1, "\r\n" );
			if ( !p2 )
				p2 = strchr( p1, '\0' );

			char* pTemp = p2;
			while ( pTemp > p1 && (*pTemp == '\r' || *pTemp == '\n' ||*pTemp == '\0' || *pTemp == ' ' ))
				pTemp--;
			*++pTemp = 0;

			String mask = GetWord(p1, 0);
			String flags = GetWord(p1, 1);
			String network = GetWord(p1, 2);
			if ( !mask.empty() )
				m_ignoreItems.push_back( CIrcIgnoreItem( getCodepage(), mask.c_str(), flags.c_str(), network.c_str()));

			p1 = p2;
		}

		RewriteIgnoreSettings();
		delete[] pszIgnoreData;
		::remove( szTemp );
	}

	int idx = 0;
	char settingName[40];
	while ( TRUE ) {
		mir_snprintf( settingName, sizeof(settingName), "IGNORE:%d", idx++ );

		DBVARIANT dbv;
		if ( getTString( settingName, &dbv ))
			break;
		
		TString mask = GetWord( dbv.ptszVal, 0 );
		TString flags = GetWord( dbv.ptszVal, 1 );
		TString network = GetWord( dbv.ptszVal, 2 );
		m_ignoreItems.push_back( CIrcIgnoreItem( mask.c_str(), flags.c_str(), network.c_str()));
		DBFreeVariant( &dbv );
}	}

void CIrcProto::RewriteIgnoreSettings( void )
{
	char settingName[ 40 ];

	size_t i=0;
	while ( TRUE ) {
		mir_snprintf( settingName, sizeof(settingName), "IGNORE:%d", i++ );
		if ( DBDeleteContactSetting( NULL, m_szModuleName, settingName ))
			break;
	}

	for ( i=0; i < m_ignoreItems.size(); i++ ) {
		mir_snprintf( settingName, sizeof(settingName), "IGNORE:%d", i );

		CIrcIgnoreItem& C = m_ignoreItems[i];
		setTString( settingName, ( C.mask + _T(" ") + C.flags + _T(" ") + C.network ).c_str());
}	}

struct CIgnorePrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CIgnorePrefsDlg( CIrcProto* _pro ) :
		CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_IGNORE, NULL )
	{
		SetControlHandler( IDC_ENABLEIGNORE, &CIgnorePrefsDlg::OnEnableIgnore );
		SetControlHandler( IDC_IGNORECHAT, &CIgnorePrefsDlg::OnIgnoreChat );
		SetControlHandler( IDC_ADD, &CIgnorePrefsDlg::OnAdd );
		SetControlHandler( IDC_EDIT, &CIgnorePrefsDlg::OnEdit );
		SetControlHandler( IDC_DELETE, &CIgnorePrefsDlg::OnDelete );
	}

	static CDlgBase* Create( void* param ) { return new CIgnorePrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog()
	{
		OldListViewProc = (WNDPROC)SetWindowLong(GetDlgItem( m_hwnd,IDC_LIST),GWL_WNDPROC,(LONG)ListviewSubclassProc);

		CheckDlgButton( m_hwnd, IDC_ENABLEIGNORE, m_proto->m_ignore?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton( m_hwnd, IDC_IGNOREFILE, m_proto->m_DCCFileEnabled?BST_UNCHECKED:BST_CHECKED);
		CheckDlgButton( m_hwnd, IDC_IGNORECHAT, m_proto->m_DCCChatEnabled?BST_UNCHECKED:BST_CHECKED);
		CheckDlgButton( m_hwnd, IDC_IGNORECHANNEL, m_proto->m_ignoreChannelDefault?BST_CHECKED:BST_UNCHECKED);
		if(m_proto->m_DCCChatIgnore == 2)
			CheckDlgButton( m_hwnd, IDC_IGNOREUNKNOWN, BST_CHECKED);

		EnableWindow(GetDlgItem( m_hwnd, IDC_IGNOREUNKNOWN), m_proto->m_DCCChatEnabled);
		EnableWindow(GetDlgItem( m_hwnd, IDC_LIST), m_proto->m_ignore);
		EnableWindow(GetDlgItem( m_hwnd, IDC_IGNORECHANNEL), m_proto->m_ignore);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), m_proto->m_ignore);

		SendDlgItemMessage( m_hwnd,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_ADD));
		SendDlgItemMessage( m_hwnd,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_DELETE));
		SendDlgItemMessage( m_hwnd,IDC_EDIT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_RENAME));
		SendMessage(GetDlgItem( m_hwnd,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Add new ignore"), 0);
		SendMessage(GetDlgItem( m_hwnd,IDC_EDIT), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Edit this ignore"), 0);
		SendMessage(GetDlgItem( m_hwnd,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Delete this ignore"), 0);

		static int COLUMNS_SIZES[3] = {195, 60, 80};
		LV_COLUMN lvC;
		lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvC.fmt = LVCFMT_LEFT;
		for ( int index=0; index < 3; index++ ) {
			lvC.iSubItem = index;
			lvC.cx = COLUMNS_SIZES[index];

			TCHAR* text;
			switch (index) {
				case 0: text = TranslateT("Ignore mask"); break;
				case 1: text = TranslateT("Flags"); break;
				case 2: text = TranslateT("Network"); break;
			}
			lvC.pszText = text;
			ListView_InsertColumn(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW),index,&lvC);
		}

		ListView_SetExtendedListViewStyle(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW), LVS_EX_FULLROWSELECT);
		PostMessage( m_hwnd, IRC_REBUILDIGNORELIST, 0, 0);
	}

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch( msg ) {
		case IRC_UPDATEIGNORELIST:
			{
				int j = ListView_GetItemCount(GetDlgItem( m_hwnd, IDC_LIST));
				if (j > 0 ) {
					LVITEM lvm;
					lvm.mask= LVIF_PARAM;
					lvm.iSubItem = 0;
					for( int i =0; i < j; i++) {
						lvm.iItem = i;
						lvm.lParam = i;
						ListView_SetItem(GetDlgItem( m_hwnd, IDC_LIST),&lvm);
			}	}	}
			break;

		case IRC_REBUILDIGNORELIST:
			{
				m_proto->m_hwndIgnore = m_hwnd;
				HWND hListView = GetDlgItem( m_hwnd, IDC_LIST);
				ListView_DeleteAllItems( hListView );

				for ( size_t i=0; i < m_proto->m_ignoreItems.size(); i++ ) {
					CIrcIgnoreItem& C = m_proto->m_ignoreItems[i];
					if ( C.mask.empty() || C.flags[0] != '+' )
						continue;

					LVITEM lvItem;
					lvItem.iItem = ListView_GetItemCount(hListView); 
					lvItem.mask = LVIF_TEXT|LVIF_PARAM ;
					lvItem.iSubItem = 0;
					lvItem.lParam = lvItem.iItem;
					lvItem.pszText = (TCHAR*)C.mask.c_str();
					lvItem.iItem = ListView_InsertItem(hListView,&lvItem);

					lvItem.mask = LVIF_TEXT;
					lvItem.iSubItem =1;
					lvItem.pszText = (TCHAR*)C.flags.c_str();
					ListView_SetItem(hListView,&lvItem);

					lvItem.mask = LVIF_TEXT;
					lvItem.iSubItem =2;
					lvItem.pszText = (TCHAR*)C.network.c_str();
					ListView_SetItem(hListView,&lvItem);
				}

				SendMessage( m_hwnd, IRC_UPDATEIGNORELIST, 0, 0);
				SendMessage( hListView, LVM_SORTITEMS, (WPARAM)m_proto, (LPARAM)IgnoreListSort);
				SendMessage( m_hwnd, IRC_UPDATEIGNORELIST, 0, 0);

				PostMessage( m_hwnd, IRC_FIXIGNOREBUTTONS, 0, 0);
			}
			break;

		case IRC_FIXIGNOREBUTTONS:
			EnableWindow( GetDlgItem( m_hwnd, IDC_ADD), IsDlgButtonChecked( m_hwnd,IDC_ENABLEIGNORE ));
			if(ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_LIST)) != -1) {
				EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), true);
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), true);
			}
			else {
				EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), false);
				EnableWindow(GetDlgItem( m_hwnd, IDC_DELETE), false);
			}
			break;

		case WM_NOTIFY :
			switch(((LPNMHDR)lParam)->idFrom) {
			case IDC_LIST:
				switch (((NMHDR*)lParam)->code) {
				case NM_CLICK:
				case NM_RCLICK:
					if ( ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_LIST)) != -1 )
						SendMessage( m_hwnd, IRC_FIXIGNOREBUTTONS, 0, 0);
					break;
					
				case NM_DBLCLK:
					SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
					break;

				case LVN_COLUMNCLICK :
					{
						LPNMLISTVIEW lv;
						lv = (LPNMLISTVIEW)lParam;
						SendMessage(GetDlgItem( m_hwnd, IDC_LIST), LVM_SORTITEMS, (WPARAM)lv->iSubItem, (LPARAM)IgnoreListSort);
						SendMessage( m_hwnd, IRC_UPDATEIGNORELIST, 0, 0);
					}
					break;
				}

			case 0:
				switch (((LPNMHDR)lParam)->code) {
				case PSN_APPLY:
					m_proto->m_DCCFileEnabled = IsDlgButtonChecked( m_hwnd,IDC_IGNOREFILE)== BST_UNCHECKED?1:0;
					m_proto->m_DCCChatEnabled = IsDlgButtonChecked( m_hwnd,IDC_IGNORECHAT)== BST_UNCHECKED?1:0;
					m_proto->m_ignore = IsDlgButtonChecked( m_hwnd,IDC_ENABLEIGNORE)== BST_CHECKED?1:0;
					m_proto->m_ignoreChannelDefault = IsDlgButtonChecked( m_hwnd,IDC_IGNORECHANNEL)== BST_CHECKED?1:0;
					m_proto->m_DCCChatIgnore = IsDlgButtonChecked( m_hwnd, IDC_IGNOREUNKNOWN) == BST_CHECKED?2:1;
					m_proto->setByte("EnableCtcpFile",m_proto->m_DCCFileEnabled);
					m_proto->setByte("EnableCtcpChat",m_proto->m_DCCChatEnabled);
					m_proto->setByte("Ignore",m_proto->m_ignore);
					m_proto->setByte("IgnoreChannelDefault",m_proto->m_ignoreChannelDefault);
					m_proto->setByte("CtcpChatIgnore",m_proto->m_DCCChatIgnore);
				}
				return TRUE;
			}
			break;
		}
		return CDlgBase::DlgProc(msg, wParam, lParam);
	}

	void OnEnableIgnore(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_IGNORECHANNEL), IsDlgButtonChecked( m_hwnd, IDC_ENABLEIGNORE) == BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_LIST), IsDlgButtonChecked( m_hwnd, IDC_ENABLEIGNORE) == BST_CHECKED);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), IsDlgButtonChecked( m_hwnd, IDC_ENABLEIGNORE) == BST_CHECKED);
	}

	void OnIgnoreChat(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		EnableWindow(GetDlgItem( m_hwnd, IDC_IGNOREUNKNOWN), IsDlgButtonChecked( m_hwnd, IDC_IGNORECHAT) == BST_UNCHECKED);
	}

	void OnAdd(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		CAddIgnoreDlg* dlg = new CAddIgnoreDlg( m_proto, NULL, m_hwnd );
		dlg->Show();
		HWND hWnd = dlg->GetHwnd();
		SetWindowText( hWnd, TranslateT( "Add m_ignore" ));
		EnableWindow(GetDlgItem(( m_hwnd), IDC_ADD), false);
		EnableWindow(GetDlgItem(( m_hwnd), IDC_EDIT), false);
		EnableWindow(GetDlgItem(( m_hwnd), IDC_DELETE), false);
	}

	void OnEdit(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( IsWindowEnabled( GetDlgItem( m_hwnd, IDC_ADD ))) {
			TCHAR szMask[512];
			TCHAR szFlags[512];
			TCHAR szNetwork[512];
			int i = ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_LIST));
			ListView_GetItemText(GetDlgItem( m_hwnd, IDC_LIST), i, 0, szMask, 511); 
			ListView_GetItemText(GetDlgItem( m_hwnd, IDC_LIST), i, 1, szFlags, 511); 
			ListView_GetItemText(GetDlgItem( m_hwnd, IDC_LIST), i, 2, szNetwork, 511); 
			CAddIgnoreDlg* dlg = new CAddIgnoreDlg( m_proto, szMask, m_hwnd );
			dlg->Show();
			HWND hWnd = dlg->GetHwnd();
			SetWindowText(hWnd, TranslateT("Edit m_ignore"));
			if ( szFlags[0] ) {
				if( _tcschr(szFlags, 'q'))
					CheckDlgButton(hWnd, IDC_Q, BST_CHECKED);
				if( _tcschr(szFlags, 'n'))
					CheckDlgButton(hWnd, IDC_N, BST_CHECKED);
				if( _tcschr(szFlags, 'i'))
					CheckDlgButton(hWnd, IDC_I, BST_CHECKED);
				if( _tcschr(szFlags, 'd'))
					CheckDlgButton(hWnd, IDC_D, BST_CHECKED);
				if( _tcschr(szFlags, 'c'))
					CheckDlgButton(hWnd, IDC_C, BST_CHECKED);
				if( _tcschr(szFlags, 'm'))
					CheckDlgButton(hWnd, IDC_M, BST_CHECKED);
			}
			SetWindowText(GetDlgItem(hWnd, IDC_MASK), szMask);
			SetWindowText(GetDlgItem(hWnd, IDC_NETWORK), szNetwork);
			EnableWindow(GetDlgItem(( m_hwnd), IDC_ADD), false);
			EnableWindow(GetDlgItem(( m_hwnd), IDC_EDIT), false);
			EnableWindow(GetDlgItem(( m_hwnd), IDC_DELETE), false);
	}	}

	void OnDelete(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if ( IsWindowEnabled(GetDlgItem( m_hwnd, IDC_DELETE))) {
			TCHAR szMask[512];
			int i = ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_LIST ));
			ListView_GetItemText( GetDlgItem( m_hwnd, IDC_LIST), i, 0, szMask, SIZEOF(szMask));
			m_proto->RemoveIgnore( szMask );
	}	}

	virtual void OnDestroy()
	{
		m_proto->m_ignoreItems.clear();

		HWND hListView = GetDlgItem( m_hwnd, IDC_LIST );
		int i = ListView_GetItemCount(GetDlgItem( m_hwnd, IDC_LIST));
		for ( int j = 0;  j < i; j++ ) {
			TCHAR szMask[512], szFlags[40], szNetwork[100];
			ListView_GetItemText( hListView, j, 0, szMask, SIZEOF(szMask));
			ListView_GetItemText( hListView, j, 1, szFlags, SIZEOF(szFlags));
			ListView_GetItemText( hListView, j, 2, szNetwork, SIZEOF(szNetwork));
			m_proto->m_ignoreItems.push_back( CIrcIgnoreItem( szMask, szFlags, szNetwork ));
		}

		m_proto->RewriteIgnoreSettings();
		SetWindowLong(GetDlgItem( m_hwnd,IDC_LIST),GWL_WNDPROC,(LONG)OldListViewProc);
		m_proto->m_hwndIgnore = NULL;
	}
};

int __cdecl CIrcProto::OnInitOptionsPages(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_CONNECT);
	odp.pszTitle = m_szModuleName;
	odp.pszGroup = LPGEN("Network");
	odp.pszTab = LPGEN("Account");
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = CDlgBase::DynamicDlgProc;
	odp.dwInitParam = (LPARAM)&OptCreateAccount;
	OptCreateAccount.create = CConnectPrefsDlg::Create;
	OptCreateAccount.param = this;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_CTCP);
	odp.pszTab = LPGEN("DCC'n CTCP");
	odp.dwInitParam = (LPARAM)&OptCreateConn;
	OptCreateConn.create = CCtcpPrefsDlg::Create;
	OptCreateConn.param = this;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_OTHER);
	odp.pszTab = LPGEN("Advanced");
	odp.dwInitParam = (LPARAM)&OptCreateOther;
	OptCreateOther.create = COtherPrefsDlg::Create;
	OptCreateOther.param = this;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_IGNORE);
	odp.pszTab = LPGEN("Ignore");
	odp.dwInitParam = (LPARAM)&OptCreateIgnore;
	OptCreateIgnore.create = CIgnorePrefsDlg::Create;
	OptCreateIgnore.param = this;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}
