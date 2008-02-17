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

CAddServerDlg::CAddServerDlg( CIrcProto* _pro, CConnectPrefsDlg* _owner ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_ADDSERVER, _owner->GetHwnd() ),
	m_owner( _owner ),
	m_OK( this, IDOK )
{
	m_OK.OnClick = Callback( this, &CAddServerDlg::OnOk );
}

void CAddServerDlg::OnInitDialog()
{
	int i = m_owner->m_serverCombo.GetCount();
	for (int index = 0; index <i; index++) {
		SERVER_INFO* pData = ( SERVER_INFO* )m_owner->m_serverCombo.GetItemData( index );
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

void __cdecl CAddServerDlg::OnOk( CCtrlButton* )
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

		int iItem = m_owner->m_serverCombo.AddStringA( pData->m_name, ( LPARAM )pData );
		m_owner->m_serverCombo.SetCurSel( iItem );
		m_owner->OnServerCombo( NULL );

		//if ( m_owner->m_performCombo.FindStringA( pData->Group, -1, true ) == CB_ERR) !!!!!!!!!!!!!
		//	m_owner->m_performCombo.AddStringA( pData->Group );

		m_proto->m_serverlistModified = true;
	}
	else MessageBox( m_hwnd, TranslateT("Please complete all fields"), TranslateT("IRC error"), MB_OK | MB_ICONERROR );
}

void CAddServerDlg::OnClose()
{
	m_owner->m_serverCombo.Enable();
	m_owner->m_add.Enable();
	m_owner->m_edit.Enable();
	m_owner->m_del.Enable();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Edit server' dialog

CEditServerDlg::CEditServerDlg( CIrcProto* _pro, CConnectPrefsDlg* _owner ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_ADDSERVER, _owner->GetHwnd() ),
	m_owner( _owner ),
	m_OK( this, IDOK )
{
	m_OK.OnClick = Callback( this, &CEditServerDlg::OnOk );
}

void CEditServerDlg::OnInitDialog()
{
	int i = m_owner->m_serverCombo.GetCount();
	for (int index = 0; index <i; index++) {
		SERVER_INFO* pData = ( SERVER_INFO* )m_owner->m_serverCombo.GetItemData( index );
		if (SendMessage(GetDlgItem( m_hwnd, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
			SendMessageA(GetDlgItem( m_hwnd, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
	}
	int j = m_owner->m_serverCombo.GetCurSel();
	SERVER_INFO* pData = ( SERVER_INFO* )m_owner->m_serverCombo.GetItemData( j );
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

void __cdecl CEditServerDlg::OnOk( CCtrlButton* )
{
	if (GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem( m_hwnd, IDC_ADD_COMBO))) {
		int i = m_owner->m_serverCombo.GetCurSel();
		delete ( SERVER_INFO* )m_owner->m_serverCombo.GetItemData( i );
		m_owner->m_serverCombo.DeleteString( i );

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

		int iItem = m_owner->m_serverCombo.AddStringA( pData->m_name, (LPARAM) pData );
		m_owner->m_serverCombo.SetCurSel( iItem );
		m_owner->OnServerCombo( NULL );

		//if ( m_owner->m_performCombo.FindStringA( pData->Group, -1, true ) == CB_ERR) !!!!!!!!!!!!!!!!!!!!!
		//	m_owner->m_performCombo.AddStringA( pData->Group );

		m_proto->m_serverlistModified = true;
	}
	else MessageBox( m_hwnd, TranslateT("Please complete all fields"), TranslateT("IRC error"), MB_OK | MB_ICONERROR );
}

void CEditServerDlg::OnClose()
{
	m_owner->m_serverCombo.Enable();
	m_owner->m_add.Enable();
	m_owner->m_edit.Enable();
	m_owner->m_del.Enable();
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'CTCP preferences' dialog

CCtcpPrefsDlg::CCtcpPrefsDlg( CIrcProto* _pro ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_CTCP, NULL ),
	m_enableIP( this, IDC_ENABLEIP ),
	m_fromServer( this, IDC_FROMSERVER )
{
	m_enableIP.OnClick = Callback( this, &CCtcpPrefsDlg::OnClicked );
	m_fromServer.OnClick = Callback( this, &CCtcpPrefsDlg::OnClicked );
}

void CCtcpPrefsDlg::OnInitDialog()
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

void __cdecl CCtcpPrefsDlg::OnClicked( CCtrlButton* )
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

void CCtcpPrefsDlg::OnApply()
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

COtherPrefsDlg::COtherPrefsDlg( CIrcProto* _pro ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_OTHER, NULL ),
	m_url( this, IDC_CUSTOM ),
	m_performCombo( this, IDC_PERFORMCOMBO ),
	m_codepage( this, IDC_CODEPAGE ),
	m_pertormEdit( this, IDC_PERFORMEDIT ),
	m_perform( this, IDC_PERFORM ),
	m_add( this, IDC_ADD, _pro->LoadIconEx(IDI_ADD), LPGEN("Click to set commands that will be performed for this event")),
	m_delete( this, IDC_DELETE, _pro->LoadIconEx(IDI_DELETE), LPGEN("Click to delete the commands for this event"))
{
	m_url.OnClick = Callback( this, &COtherPrefsDlg::OnUrl ); 
	m_performCombo.OnChange = Callback( this, &COtherPrefsDlg::OnPerformCombo );
	m_codepage.OnChange = Callback( this, &COtherPrefsDlg::OnCodePage );
	m_pertormEdit.OnChange = Callback( this, &COtherPrefsDlg::OnPerformEdit );
	m_perform.OnChange = Callback( this, &COtherPrefsDlg::OnPerform );
	m_add.OnClick = Callback( this, &COtherPrefsDlg::OnAdd );
	m_delete.OnClick = Callback( this, &COtherPrefsDlg::OnDelete );
}

void COtherPrefsDlg::OnInitDialog()
{
	OldProc = (WNDPROC)SetWindowLong(GetDlgItem( m_hwnd, IDC_ALIASEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 
	SetWindowLong(GetDlgItem( m_hwnd, IDC_QUITMESSAGE), GWL_WNDPROC,(LONG)EditSubclassProc); 
	SetWindowLong(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 

	SetDlgItemText( m_hwnd, IDC_ALIASEDIT, m_proto->m_alias );
	SetDlgItemText( m_hwnd, IDC_QUITMESSAGE, m_proto->m_quitMessage );
	CheckDlgButton( m_hwnd, IDC_PERFORM, ((m_proto->m_perform) ? (BST_CHECKED) : (BST_UNCHECKED)));
	CheckDlgButton( m_hwnd, IDC_SCRIPT, ((m_proto->m_scriptingEnabled) ? (BST_CHECKED) : (BST_UNCHECKED)));
	EnableWindow(GetDlgItem( m_hwnd, IDC_SCRIPT), m_bMbotInstalled);
	m_performCombo.Enable( m_proto->m_perform );
	EnableWindow(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), m_proto->m_perform);
	m_add.Enable( m_proto->m_perform );
	m_delete.Enable( m_proto->m_perform );

	FillCodePageCombo( GetDlgItem( m_hwnd, IDC_CODEPAGE ), m_proto->m_codepage );
	if ( m_proto->m_codepage == CP_UTF8 )
		EnableWindow( GetDlgItem( m_hwnd, IDC_UTF_AUTODETECT ), FALSE );

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
			int i = m_performCombo.FindStringA( Group, -1, true );
			if (i == CB_ERR) {
				int idx = m_performCombo.AddStringA( Group );
				addPerformComboValue( idx, Group );
			}

			delete []Group;
	}	}

	{
		for ( int i=0; i < SIZEOF(sttPerformEvents); i++ ) {
			int idx = m_performCombo.InsertString( _A2T( sttPerformEvents[i] ), i );
			addPerformComboValue( idx, sttPerformEvents[i] );
	}	}

	m_performCombo.SetCurSel( 0 );
	OnPerformCombo( NULL );
	CheckDlgButton( m_hwnd, IDC_UTF_AUTODETECT, (m_proto->m_utfAutodetect) ? BST_CHECKED : BST_UNCHECKED );
	m_proto->m_performlistModified = false;
}

void __cdecl COtherPrefsDlg::OnUrl( CCtrlButton* )
{
	CallService( MS_UTILS_OPENURL,0,(LPARAM) "http://members.chello.se/matrix/index.html" );
}

void __cdecl COtherPrefsDlg::OnPerformCombo( CCtrlData* )
{
	int i = m_performCombo.GetCurSel();
	PERFORM_INFO* pPerf = (PERFORM_INFO*)m_performCombo.GetItemData( i );
	if (pPerf == 0)
		SetDlgItemTextA( m_hwnd, IDC_PERFORMEDIT, "");
	else
		SetDlgItemText( m_hwnd, IDC_PERFORMEDIT, pPerf->mText.c_str());
	m_add.Disable();
	if ( GetWindowTextLength(GetDlgItem( m_hwnd, IDC_PERFORMEDIT)) != 0)
		m_delete.Enable();
	else
		m_delete.Disable();
}

void __cdecl COtherPrefsDlg::OnCodePage( CCtrlData* )
{
	int curSel = SendDlgItemMessage( m_hwnd, IDC_CODEPAGE, CB_GETCURSEL, 0, 0 );
	EnableWindow( GetDlgItem( m_hwnd, IDC_UTF_AUTODETECT ),
		SendDlgItemMessage( m_hwnd, IDC_CODEPAGE, CB_GETITEMDATA, curSel, 0 ) != CP_UTF8 );
}

void __cdecl COtherPrefsDlg::OnPerformEdit( CCtrlData* )
{
	m_add.Enable();

	if ( GetWindowTextLength(GetDlgItem( m_hwnd, IDC_PERFORMEDIT)) != 0)
		m_delete.Enable();
	else
		m_delete.Disable();
}

void __cdecl COtherPrefsDlg::OnPerform( CCtrlData* )
{
	m_performCombo.Enable( IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED );
	EnableWindow(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
	m_add.Enable( IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
	m_delete.Enable( IsDlgButtonChecked( m_hwnd, IDC_PERFORM)== BST_CHECKED);
}

void __cdecl COtherPrefsDlg::OnAdd( CCtrlButton* )
{
	int j = GetWindowTextLength(GetDlgItem( m_hwnd, IDC_PERFORMEDIT));
	TCHAR* temp = new TCHAR[j+1];
	GetWindowText(GetDlgItem( m_hwnd, IDC_PERFORMEDIT), temp, j+1);

	if ( my_strstri( temp, _T("/away")))
		MessageBox( NULL, TranslateT("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), TranslateT("IRC Error"), MB_OK);
	else {
		int i = m_performCombo.GetCurSel();
		if ( i != CB_ERR ) {
			PERFORM_INFO* pPerf = (PERFORM_INFO*)m_performCombo.GetItemData( i );
			if ( pPerf != NULL )
				pPerf->mText = temp;

			m_add.Disable();
			m_proto->m_performlistModified = true;
	}	}
	delete []temp;
}

void __cdecl COtherPrefsDlg::OnDelete( CCtrlButton* )
{
	int i = m_performCombo.GetCurSel();
	if ( i != CB_ERR ) {
		PERFORM_INFO* pPerf = (PERFORM_INFO*)m_performCombo.GetItemData( i );
		if ( pPerf != NULL ) {
			pPerf->mText = _T("");
			SetDlgItemTextA( m_hwnd, IDC_PERFORMEDIT, "");
			m_delete.Disable();
			m_add.Disable();
		}

		m_proto->m_performlistModified = true;
}	}

void COtherPrefsDlg::OnDestroy()
{
	m_proto->m_performlistModified = false;

	int i = m_performCombo.GetCount();
	if ( i != CB_ERR && i != 0 ) {
		for (int index = 0; index < i; index++) {
			PERFORM_INFO* pPerf = (PERFORM_INFO*)m_performCombo.GetItemData( index );
			if (( const int )pPerf != CB_ERR && pPerf != NULL )
				delete pPerf;
}	}	}

void COtherPrefsDlg::OnApply()
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
	if ( m_add.Enabled())
		OnAdd( NULL );
   
	if ( m_proto->m_performlistModified ) {
		m_proto->m_performlistModified = false;

		int count = m_performCombo.GetCount();
		for ( int i = 0; i < count; i++ ) {
			PERFORM_INFO* pPerf = ( PERFORM_INFO* )m_performCombo.GetItemData( i );
			if (( const int )pPerf == CB_ERR )
				continue;

			if ( !pPerf->mText.empty())
				m_proto->setTString( pPerf->mSetting.c_str(), pPerf->mText.c_str());
			else 
				DBDeleteContactSetting( NULL, m_proto->m_szModuleName, pPerf->mSetting.c_str());
}	}	}

void COtherPrefsDlg::addPerformComboValue( int idx, const char* szValueName )
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
	m_performCombo.SetItemData( idx, ( LPARAM )pPref );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Connect preferences' dialog

CConnectPrefsDlg::CConnectPrefsDlg( CIrcProto* _pro ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_CONNECT, NULL ),
	m_serverCombo( this, IDC_SERVERCOMBO ),
	m_server( this, IDC_SERVER ),
	m_port( this, IDC_PORT ),
	m_port2( this, IDC_PORT2 ),
	m_pass( this, IDC_PASS ),
	m_add( this, IDC_ADDSERVER, _pro->LoadIconEx(IDI_ADD), LPGEN("Add a new network")),
	m_edit( this, IDC_EDITSERVER, _pro->LoadIconEx(IDI_RENAME), LPGEN("Edit this network")),
	m_del( this, IDC_DELETESERVER, _pro->LoadIconEx(IDI_DELETE), LPGEN("Delete this network")),
	m_nick( this, IDC_NICK ),
	m_nick2( this, IDC_NICK2 ),
	m_name( this, IDC_NAME ),
	m_userID( this, IDC_USERID ),
	m_ident( this, IDC_IDENT ),
	m_identSystem( this, IDC_IDENTSYSTEM ),
	m_identPort( this, IDC_IDENTPORT ),
	m_identTimed( this, IDC_IDENT_TIMED ),
	m_retry( this, IDC_RETRY ),
	m_retryWait( this, IDC_RETRYWAIT ),
	m_retryCount( this, IDC_RETRYCOUNT ),
	m_forceVisible( this, IDC_FORCEVISIBLE ),
	m_rejoinOnKick( this, IDC_REJOINONKICK ),
	m_rejoinChannels( this, IDC_REJOINCHANNELS ),
	m_disableError( this, IDC_DISABLEERROR ),
	m_address( this, IDC_ADDRESS ),
	m_useServer( this, IDC_USESERVER ),
	m_showServer( this, IDC_SHOWSERVER ),
	m_keepAlive( this, IDC_KEEPALIVE ),
	m_autoJoin( this, IDC_AUTOJOIN ),
	m_oldStyle( this, IDC_OLDSTYLE ),
	m_onlineNotif( this, IDC_ONLINENOTIF ),
	m_channelAway( this, IDC_CHANNELAWAY ),
	m_startup( this, IDC_STARTUP ),
	m_onlineTimer( this, IDC_ONLINETIMER ),
	m_limit( this, IDC_LIMIT ),
	m_spin1( this, IDC_SPIN1 ),
	m_spin2( this, IDC_SPIN2 ),
	m_ssl( this, IDC_SSL )
{
	CreateLink( m_autoJoin, "JoinOnInvite", DBVT_BYTE, 0 );

	m_serverCombo.OnChange = Callback( this, &CConnectPrefsDlg::OnServerCombo );
	m_add.OnClick = Callback( this, &CConnectPrefsDlg::OnAddServer );
	m_del.OnClick = Callback( this, &CConnectPrefsDlg::OnDeleteServer );
	m_edit.OnClick = Callback( this, &CConnectPrefsDlg::OnEditServer );
	m_startup.OnChange = Callback( this, &CConnectPrefsDlg::OnStartup );
	m_ident.OnChange = Callback( this, &CConnectPrefsDlg::OnIdent );
	m_useServer.OnChange = Callback( this, &CConnectPrefsDlg::OnUseServer );
	m_onlineNotif.OnChange = Callback( this, &CConnectPrefsDlg::OnOnlineNotif );
	m_channelAway.OnChange = Callback( this, &CConnectPrefsDlg::OnChannelAway );
	m_retry.OnChange = Callback( this, &CConnectPrefsDlg::OnRetry );
}

void CConnectPrefsDlg::OnInitDialog()
{
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
			m_serverCombo.AddStringA( pData->m_name, (LPARAM) pData );
	}	}

	m_serverCombo.SetCurSel( m_proto->m_serverComboSelection-1 );				
	m_server.SetTextA( m_proto->m_serverName);
	m_port.SetTextA( m_proto->m_portStart);
	m_port2.SetTextA( m_proto->m_portEnd);
	if ( m_ssleay32 ) {
		if ( m_proto->m_iSSL == 0 )
			m_ssl.SetText( TranslateT( "Off" ));
		if ( m_proto->m_iSSL == 1 )
			m_ssl.SetText( TranslateT( "Auto" ));
		if ( m_proto->m_iSSL == 2 )
			m_ssl.SetText( TranslateT( "On" ));
	}
	else m_ssl.SetText( TranslateT( "N/A" ));

	if ( m_proto->m_serverComboSelection != -1 ) {
		SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( m_proto->m_serverComboSelection-1 );
		if ((int)pData != CB_ERR) {
			m_server.SetTextA( pData->Address );
			m_port.SetTextA( pData->m_portStart );
			m_port2.SetTextA( pData->m_portEnd );
	}	}

	m_spin1.SendMsg( UDM_SETRANGE,0,MAKELONG(999,20));
	m_spin1.SendMsg( UDM_SETPOS,0,MAKELONG(m_proto->m_onlineNotificationTime,0));
	m_spin2.SendMsg( UDM_SETRANGE,0,MAKELONG(200,0));
	m_spin2.SendMsg( UDM_SETPOS,0,MAKELONG(m_proto->m_onlineNotificationLimit,0));
	m_nick.SetText( m_proto->m_nick);
	m_nick2.SetText( m_proto->m_alternativeNick );
	m_userID.SetText( m_proto->m_userID);
	m_name.SetText( m_proto->m_name);
	m_pass.SetTextA( m_proto->m_password);
	m_identSystem.SetText( m_proto->m_identSystem );
	m_identPort.SetText( m_proto->m_identPort );
	m_retryWait.SetText( m_proto->m_retryWait);
	m_retryCount.SetText( m_proto->m_retryCount);
	m_address.SetState( m_proto->m_showAddresses );
	m_oldStyle.SetState( m_proto->m_oldStyleModes );
	m_channelAway.SetState( m_proto->m_channelAwayNotification );
	m_onlineNotif.SetState( m_proto->m_autoOnlineNotification );
	m_onlineTimer.Enable( m_proto->m_autoOnlineNotification);
	m_channelAway.Enable( m_proto->m_autoOnlineNotification);
	m_spin1.Enable( m_proto->m_autoOnlineNotification );
	m_spin2.Enable( m_proto->m_autoOnlineNotification && m_proto->m_channelAwayNotification );
	m_limit.Enable( m_proto->m_autoOnlineNotification && m_proto->m_channelAwayNotification );
	m_ident.SetState( m_proto->m_ident );
	m_identSystem.Enable( m_proto->m_ident );
	m_identPort.Enable( m_proto->m_ident );
	m_identTimed.SetState( m_proto->IdentTimer );				
	m_disableError.SetState( m_proto->m_disableErrorPopups );
	m_forceVisible.SetState( m_proto->m_forceVisible );
	m_rejoinChannels.SetState( m_proto->m_rejoinChannels );
	m_rejoinOnKick.SetState( m_proto->m_rejoinIfKicked );
	m_retry.SetState( m_proto->m_retry );
	m_retryWait.Enable( m_proto->m_retry );
	m_retryCount.Enable( m_proto->m_retry );
	m_startup.SetState( !m_proto->m_disableDefaultServer );
	m_keepAlive.SetState( m_proto->m_sendKeepAlive );				
	m_useServer.SetState( m_proto->m_useServer );				
	m_showServer.SetState( !m_proto->m_hideServerWindow );				
	m_showServer.Enable( m_proto->m_useServer );
	m_autoJoin.SetState( m_proto->m_joinOnInvite );				
	m_serverCombo.Enable( !m_proto->m_disableDefaultServer );
	m_add.Enable( !m_proto->m_disableDefaultServer );
	m_edit.Enable( !m_proto->m_disableDefaultServer );
	m_del.Enable( !m_proto->m_disableDefaultServer );
	m_server.Enable( !m_proto->m_disableDefaultServer );
	m_port.Enable( !m_proto->m_disableDefaultServer );
	m_port2.Enable( !m_proto->m_disableDefaultServer );
	m_pass.Enable( !m_proto->m_disableDefaultServer );
	m_identTimed.Enable( m_proto->m_ident );
	m_proto->m_serverlistModified = false;
}

void __cdecl CConnectPrefsDlg::OnServerCombo( CCtrlData* )
{
	int i = m_serverCombo.GetCurSel();
	SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( i );
	if (pData && (int)pData != CB_ERR ) {
		m_server.SetTextA( pData->Address );
		m_port.SetTextA( pData->m_portStart );
		m_port2.SetTextA( pData->m_portEnd );
		m_port.SetTextA( "" );
		if ( m_ssleay32 ) {
			if ( pData->m_iSSL == 0 )
				m_ssl.SetText( TranslateT( "Off" ));
			if ( pData->m_iSSL == 1 )
				m_ssl.SetText( TranslateT( "Auto" ));
			if ( pData->m_iSSL == 2 )
				m_ssl.SetText( TranslateT( "On" ));
		}
		else m_ssl.SetText( TranslateT( "N/A" ));
		SendMessage(GetParent( m_hwnd), PSM_CHANGED,0,0);
}	}

void __cdecl CConnectPrefsDlg::OnAddServer( CCtrlButton* )
{
	m_serverCombo.Disable();
	m_add.Disable();
	m_edit.Disable();
	m_del.Disable();
	CAddServerDlg* dlg = new CAddServerDlg( m_proto, this );
	dlg->Show();
}

void __cdecl CConnectPrefsDlg::OnDeleteServer( CCtrlButton* )
{
	int i = m_serverCombo.GetCurSel();
	if ( i == CB_ERR)
		return;

	m_serverCombo.Disable();
	m_add.Disable();
	m_edit.Disable();
	m_del.Disable();
	
	SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( i );
	TCHAR temp[200];
	mir_sntprintf( temp, SIZEOF(temp), TranslateT("Do you want to delete\r\n%s"), (TCHAR*)_A2T(pData->m_name));
	if ( MessageBox( m_hwnd, temp, TranslateT("Delete server"), MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
		delete pData;	

		m_serverCombo.DeleteString( i );
		if ( i >= m_serverCombo.GetCount())
			i--;
		m_serverCombo.SetCurSel( i );
		OnServerCombo( NULL );
		m_proto->m_serverlistModified = true;
	}

	m_serverCombo.Enable();
	m_add.Enable();
	m_edit.Enable();
	m_del.Enable();
}

void __cdecl CConnectPrefsDlg::OnEditServer( CCtrlButton* )
{
	int i = m_serverCombo.GetCurSel();
	if ( i != CB_ERR )
		return;

	m_serverCombo.Disable();
	m_add.Disable();
	m_edit.Disable();
	m_del.Disable();
	CEditServerDlg* dlg = new CEditServerDlg( m_proto, this );
	dlg->Show();
	SetWindowText( dlg->GetHwnd(), TranslateT( "Edit server" ));
}

void __cdecl CConnectPrefsDlg::OnStartup( CCtrlData* )
{
	m_serverCombo.Enable( m_startup.GetState());
	m_add.Enable( m_startup.GetState());
	m_edit.Enable( m_startup.GetState());
	m_del.Enable( m_startup.GetState());
	m_server.Enable( m_startup.GetState());
	m_port.Enable( m_startup.GetState());
	m_port2.Enable( m_startup.GetState());
	m_pass.Enable( m_startup.GetState());
	m_ssl.Enable( m_startup.GetState());
}

void __cdecl CConnectPrefsDlg::OnIdent( CCtrlData* )
{
	m_identSystem.Enable( m_ident.GetState());
	m_identPort.Enable( m_ident.GetState());
	m_identTimed.Enable( m_ident.GetState());
}

void __cdecl CConnectPrefsDlg::OnUseServer( CCtrlData* )
{
	EnableWindow(GetDlgItem( m_hwnd, IDC_SHOWSERVER), m_useServer.GetState());
}

void __cdecl CConnectPrefsDlg::OnOnlineNotif( CCtrlData* )
{
	m_channelAway.Enable( m_onlineNotif.GetState());
	m_onlineTimer.Enable( m_onlineNotif.GetState());
	m_spin1.Enable( m_onlineNotif.GetState());
	m_spin2.Enable( m_onlineNotif.GetState());
	m_limit.Enable( m_onlineNotif.GetState() && m_channelAway.GetState());
}

void __cdecl CConnectPrefsDlg::OnChannelAway( CCtrlData* )
{	
	m_spin2.Enable( m_onlineNotif.GetState() && m_channelAway.GetState());
	m_limit.Enable( m_onlineNotif.GetState() && m_channelAway.GetState());
}

void __cdecl CConnectPrefsDlg::OnRetry( CCtrlData* )
{	
	m_retryWait.Enable( m_retry.GetState());
	m_retryCount.Enable( m_retry.GetState());
}

void CConnectPrefsDlg::OnApply()
{
	//Save the setting in the CONNECT dialog
	if(m_startup.GetState()) {
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
	m_proto->m_disableDefaultServer = !m_startup.GetState();
	m_proto->setByte("DisableDefaultServer",m_proto->m_disableDefaultServer);
	m_proto->m_ident = m_ident.GetState();
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
	m_proto->m_retry = m_retry.GetState();
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

	m_proto->m_joinOnInvite = IsDlgButtonChecked( m_hwnd, IDC_AUTOJOIN )== BST_CHECKED;
	m_proto->setByte("JoinOnInvite",m_proto->m_joinOnInvite);
	m_proto->m_hideServerWindow = IsDlgButtonChecked( m_hwnd, IDC_SHOWSERVER )== BST_UNCHECKED;
	m_proto->setByte("HideServerWindow",m_proto->m_hideServerWindow);
	m_proto->m_serverComboSelection = m_serverCombo.GetCurSel() + 1;
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

	int i = m_serverCombo.GetCurSel();
	SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( i );
	if ( pData && (int)pData != CB_ERR ) {
		if ( m_startup.GetState())
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
			int j = m_serverCombo.GetCount();
			if (j != CB_ERR && j != 0) {
				for (int index2 = 0; index2 < j; index2++) {
					SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( index2 );
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

void CConnectPrefsDlg::OnDestroy()
{
	m_proto->m_serverlistModified = false;
	int j = m_serverCombo.GetCount();
	if (j !=CB_ERR && j !=0 ) {
		for (int index2 = 0; index2 < j; index2++) {
			SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( index2 );
			if ( pData != NULL && (int)pData != CB_ERR )
				delete pData;	
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// 'm_ignore' preferences dialog

CAddIgnoreDlg::CAddIgnoreDlg( CIrcProto* _pro, const TCHAR* mask, HWND parent ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_ADDIGNORE, parent ),
	m_Ok( this, IDOK )
{
	if ( mask == NULL )
		szOldMask[0] = 0;
	else
		_tcsncpy( szOldMask, mask, SIZEOF(szOldMask));

	m_Ok.OnClick = Callback( this, &CAddIgnoreDlg::OnOk );
}

void CAddIgnoreDlg::OnInitDialog()
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

void __cdecl CAddIgnoreDlg::OnOk( CCtrlButton* )
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

void CAddIgnoreDlg::OnClose()
{
	PostMessage( m_proto->m_hwndIgnore, IRC_FIXIGNOREBUTTONS, 0, 0 );
	DestroyWindow( m_hwnd);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Ignore' preferences dialog

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

CIgnorePrefsDlg::CIgnorePrefsDlg( CIrcProto* _pro ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_PREFS_IGNORE, NULL ),
	m_list( this, IDC_LIST ),
	m_add( this, IDC_ADD, _pro->LoadIconEx(IDI_ADD), LPGEN("Add new ignore")),
	m_edit( this, IDC_EDIT, _pro->LoadIconEx(IDI_RENAME), LPGEN("Edit this ignore")),
	m_del( this, IDC_DELETE, _pro->LoadIconEx(IDI_DELETE), LPGEN("Delete this ignore")),
	m_enable( this, IDC_ENABLEIGNORE ),
	m_ignoreChat( this, IDC_IGNORECHAT ),
	m_ignoreFile( this, IDC_IGNOREFILE ),
	m_ignoreChannel( this, IDC_IGNORECHANNEL ),
	m_ignoreUnknown( this, IDC_IGNOREUNKNOWN )
{
	m_enable.OnChange = Callback( this, &CIgnorePrefsDlg::OnEnableIgnore );
	m_ignoreChat.OnChange = Callback( this, &CIgnorePrefsDlg::OnIgnoreChat );
	m_add.OnClick = Callback( this, &CIgnorePrefsDlg::OnAdd );
	m_edit.OnClick = Callback( this, &CIgnorePrefsDlg::OnEdit );
	m_del.OnClick = Callback( this, &CIgnorePrefsDlg::OnDelete );
}

void CIgnorePrefsDlg::OnInitDialog()
{
	OldListViewProc = (WNDPROC)SetWindowLong( m_list.GetHwnd(),GWL_WNDPROC, (LONG)ListviewSubclassProc );

	m_enable.SetState( m_proto->m_ignore );
	m_ignoreFile.SetState( m_proto->m_DCCFileEnabled );
	m_ignoreChat.SetState( m_proto->m_DCCChatEnabled );
	m_ignoreChannel.SetState( m_proto->m_ignoreChannelDefault );
	if ( m_proto->m_DCCChatIgnore == 2 )
		m_ignoreUnknown.SetState( BST_CHECKED );

	m_ignoreUnknown.Enable( m_proto->m_DCCChatEnabled );
	m_list.Enable( m_proto->m_ignore );
	m_ignoreChannel.Enable( m_proto->m_ignore);
	m_add.Enable( m_proto->m_ignore );

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

BOOL CIgnorePrefsDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
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
		m_add.Enable( m_enable.GetState());
		if(ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_LIST)) != -1) {
			m_edit.Enable();
			m_del.Enable();
		}
		else {
			m_edit.Disable();
			m_del.Disable();
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
				OnEdit( NULL );
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
				m_proto->m_DCCFileEnabled = m_ignoreFile.GetState();
				m_proto->m_DCCChatEnabled = m_ignoreChat.GetState();
				m_proto->m_ignore = m_enable.GetState();
				m_proto->m_ignoreChannelDefault = m_ignoreChannel.GetState();
				m_proto->m_DCCChatIgnore = m_ignoreUnknown.GetState() ? 2 : 1;
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

void __cdecl CIgnorePrefsDlg::OnEnableIgnore( CCtrlData* )
{
	m_ignoreChannel.Enable( m_enable.GetState());
	EnableWindow(GetDlgItem( m_hwnd, IDC_LIST), m_enable.GetState());
	m_add.Enable( m_enable.GetState());
}

void __cdecl CIgnorePrefsDlg::OnIgnoreChat( CCtrlData* )
{
	m_ignoreUnknown.Enable( m_ignoreChat.GetState() == BST_UNCHECKED );
}

void __cdecl CIgnorePrefsDlg::OnAdd( CCtrlButton* )
{
	CAddIgnoreDlg* dlg = new CAddIgnoreDlg( m_proto, NULL, m_hwnd );
	dlg->Show();
	HWND hWnd = dlg->GetHwnd();
	SetWindowText( hWnd, TranslateT( "Add m_ignore" ));
	m_add.Disable();
	m_edit.Disable();
	m_del.Disable();
}

void __cdecl CIgnorePrefsDlg::OnEdit( CCtrlButton* )
{
	if ( !m_add.Enabled())
		return;

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
	m_add.Disable();
	m_edit.Disable();
	m_del.Disable();
}

void __cdecl CIgnorePrefsDlg::OnDelete( CCtrlButton* )
{
	if ( !m_del.Enabled())
		return;

	TCHAR szMask[512];
	int i = ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_LIST ));
	ListView_GetItemText( GetDlgItem( m_hwnd, IDC_LIST), i, 0, szMask, SIZEOF(szMask));
	m_proto->RemoveIgnore( szMask );
}

void CIgnorePrefsDlg::OnDestroy()
{
	m_proto->m_ignoreItems.clear();

	HWND hListView = GetDlgItem( m_hwnd, IDC_LIST );
	int i = ListView_GetItemCount(GetDlgItem( m_hwnd, IDC_LIST));
	for ( int j = 0; j < i; j++ ) {
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

/////////////////////////////////////////////////////////////////////////////////////////

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
