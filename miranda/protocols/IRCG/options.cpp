/*
IRC plugin for Miranda IM

Copyright (C) 2003-2005 Jurgen Persson
Copyright (C) 2007 George Hazan

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
#include <uxtheme.h>
#include <win2k.h>

UINT_PTR OnlineNotifTimer = 0;	
UINT_PTR OnlineNotifTimer3 = 0;	

HWND connect_hWnd = NULL;
HWND addserver_hWnd = NULL;

std::vector<CIrcIgnoreItem> irc::g_ignoreItems;

static bool     ServerlistModified = false;
static WNDPROC  OldProc;
static WNDPROC  OldListViewProc;

static int GetPrefsString(const char *szSetting, char * prefstoset, int n, char * defaulttext)
{
	DBVARIANT dbv;
	if ( !DBGetContactSettingString( NULL, IRCPROTONAME, szSetting, &dbv )) {
		lstrcpynA(prefstoset, dbv.pszVal, n);
		DBFreeVariant(&dbv);
		return TRUE;
	}

	lstrcpynA(prefstoset, defaulttext, n);
	return FALSE;
}

#if defined( _UNICODE )
static int GetPrefsString(const char *szSetting, TCHAR* prefstoset, int n, TCHAR* defaulttext)
{
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, szSetting, &dbv )) {
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
	char* result = new char[size];
	GetDlgItemTextA( hWnd, ctrlID, result, size );
	return result;
}

void InitPrefs(void)
{
	DBVARIANT dbv;

	prefs = new PREFERENCES;
	GetPrefsString( "ServerName", prefs->ServerName, 101, "");
	GetPrefsString( "PortStart", prefs->PortStart, 6, "");
	GetPrefsString( "PortEnd", prefs->PortEnd, 6, "");
	GetPrefsString( "Password", prefs->Password, 499, "");
	CallService( MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)prefs->Password);
	if (!GetPrefsString( "PNick", prefs->Nick, 30, _T(""))) {
		GetPrefsString( "Nick", prefs->Nick, 30, _T(""));
		if ( lstrlen(prefs->Nick) > 0)
			DBWriteContactSettingTString(NULL, IRCPROTONAME, "PNick", prefs->Nick);
	}
	GetPrefsString( "AlernativeNick", prefs->AlternativeNick, 31, _T(""));
	GetPrefsString( "Name", prefs->Name, 199, _T(""));
	GetPrefsString( "UserID", prefs->UserID, 199, _T("Miranda"));
	GetPrefsString( "IdentSystem", prefs->IdentSystem, 10, _T("UNIX"));
	GetPrefsString( "IdentPort", prefs->IdentPort, 6, _T("113"));
	GetPrefsString( "RetryWait", prefs->RetryWait, 4, _T("30"));
	GetPrefsString( "RetryCount", prefs->RetryCount, 4, _T("10"));
	GetPrefsString( "Network", prefs->Network, 31, "");
	GetPrefsString( "QuitMessage", prefs->QuitMessage, 399, _T(STR_QUITMESSAGE));
	GetPrefsString( "UserInfo", prefs->UserInfo, 499, _T(STR_USERINFO));
	GetPrefsString( "SpecHost", prefs->MySpecifiedHost, 499, "");
	GetPrefsString( "MyLocalHost", prefs->MyLocalHost, 49, "");

	lstrcpyA( prefs->MySpecifiedHostIP, "" );

	if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "Alias", &dbv )) {
		prefs->Alias = mir_tstrdup( dbv.ptszVal);
		DBFreeVariant( &dbv );
	}
	else prefs->Alias = mir_tstrdup( _T("/op /mode ## +ooo $1 $2 $3\r\n/dop /mode ## -ooo $1 $2 $3\r\n/voice /mode ## +vvv $1 $2 $3\r\n/dvoice /mode ## -vvv $1 $2 $3\r\n/j /join #$1 $2-\r\n/p /part ## $1-\r\n/w /whois $1\r\n/k /kick ## $1 $2-\r\n/q /query $1\r\n/logon /log on ##\r\n/logoff /log off ##\r\n/save /log buffer $1\r\n/slap /me slaps $1 around a bit with a large trout" ));

	prefs->ScriptingEnabled = DBGetContactSettingByte( NULL, IRCPROTONAME, "ScriptingEnabled", 0);
	prefs->ForceVisible = DBGetContactSettingByte( NULL, IRCPROTONAME, "ForceVisible", 0);
	prefs->DisableErrorPopups = DBGetContactSettingByte( NULL, IRCPROTONAME, "DisableErrorPopups", 0);
	prefs->RejoinChannels = DBGetContactSettingByte( NULL, IRCPROTONAME, "RejoinChannels", 0);
	prefs->RejoinIfKicked = DBGetContactSettingByte( NULL, IRCPROTONAME, "RejoinIfKicked", 1);
	prefs->Ident = DBGetContactSettingByte( NULL, IRCPROTONAME, "Ident", 0);
	prefs->IdentTimer = (int)DBGetContactSettingByte( NULL, IRCPROTONAME, "IdentTimer", 0);
	prefs->Retry = DBGetContactSettingByte( NULL, IRCPROTONAME, "Retry", 0);
	prefs->DisableDefaultServer = DBGetContactSettingByte( NULL, IRCPROTONAME, "DisableDefaultServer", 0);
	prefs->HideServerWindow = DBGetContactSettingByte( NULL, IRCPROTONAME, "HideServerWindow", 1);
	prefs->UseServer = DBGetContactSettingByte( NULL, IRCPROTONAME, "UseServer", 1);
	prefs->JoinOnInvite = DBGetContactSettingByte( NULL, IRCPROTONAME, "JoinOnInvite", 0);
	prefs->Perform = DBGetContactSettingByte( NULL, IRCPROTONAME, "Perform", 0);
	prefs->ShowAddresses = DBGetContactSettingByte( NULL, IRCPROTONAME, "ShowAddresses", 0);
	prefs->AutoOnlineNotification = DBGetContactSettingByte( NULL, IRCPROTONAME, "AutoOnlineNotification", 1);
	prefs->Ignore = DBGetContactSettingByte( NULL, IRCPROTONAME, "Ignore", 0);;
	prefs->IgnoreChannelDefault = DBGetContactSettingByte( NULL, IRCPROTONAME, "IgnoreChannelDefault", 0);;
	prefs->ServerComboSelection = DBGetContactSettingDword( NULL, IRCPROTONAME, "ServerComboSelection", -1);
	prefs->QuickComboSelection = DBGetContactSettingDword( NULL, IRCPROTONAME, "QuickComboSelection", prefs->ServerComboSelection);
	prefs->SendKeepAlive = (int)DBGetContactSettingByte( NULL, IRCPROTONAME, "SendKeepAlive", 0);
	prefs->ListSize.y = DBGetContactSettingDword( NULL, IRCPROTONAME, "SizeOfListBottom", 400);
	prefs->ListSize.x = DBGetContactSettingDword( NULL, IRCPROTONAME, "SizeOfListRight", 600);
	prefs->iSSL = DBGetContactSettingByte( NULL, IRCPROTONAME, "UseSSL", 0);
	prefs->DCCFileEnabled = DBGetContactSettingByte( NULL, IRCPROTONAME, "EnableCtcpFile", 1);
	prefs->DCCChatEnabled = DBGetContactSettingByte( NULL, IRCPROTONAME, "EnableCtcpChat", 1);
	prefs->DCCChatAccept = DBGetContactSettingByte( NULL, IRCPROTONAME, "CtcpChatAccept", 1);
	prefs->DCCChatIgnore = DBGetContactSettingByte( NULL, IRCPROTONAME, "CtcpChatIgnore", 1);
	prefs->DCCPassive = DBGetContactSettingByte( NULL, IRCPROTONAME, "DccPassive", 0);
	prefs->ManualHost = DBGetContactSettingByte( NULL, IRCPROTONAME, "ManualHost", 0);
	prefs->IPFromServer = DBGetContactSettingByte( NULL, IRCPROTONAME, "IPFromServer", 0);
	prefs->DisconnectDCCChats = DBGetContactSettingByte( NULL, IRCPROTONAME, "DisconnectDCCChats", 1);
	prefs->OldStyleModes = DBGetContactSettingByte( NULL, IRCPROTONAME, "OldStyleModes", 0);
	prefs->SendNotice = DBGetContactSettingByte( NULL, IRCPROTONAME, "SendNotice", 1);
	prefs->Codepage = DBGetContactSettingDword( NULL, IRCPROTONAME, "Codepage", CP_ACP );
	prefs->UtfAutodetect = DBGetContactSettingByte( NULL, IRCPROTONAME, "UtfAutodetect", 1);
	prefs->MyHost[0] = '\0';
	prefs->colors[0] = RGB(255,255,255);
	prefs->colors[1] = RGB(0,0,0);
	prefs->colors[2] = RGB(0,0,127);
	prefs->colors[3] = RGB(0,147,0);
	prefs->colors[4] = RGB(255,0,0);
	prefs->colors[5] = RGB(127,0,0);
	prefs->colors[6] = RGB(156,0,156);
	prefs->colors[7] = RGB(252,127,0);
	prefs->colors[8] = RGB(255,255,0);
	prefs->colors[9] = RGB(0,252,0);
	prefs->colors[10] = RGB(0,147,147);
	prefs->colors[11] = RGB(0,255,255);
	prefs->colors[12] = RGB(0,0,252);
	prefs->colors[13] = RGB(255,0,255);
	prefs->colors[14] = RGB(127,127,127); 
	prefs->colors[15] = RGB(210,210,210);
	prefs->OnlineNotificationTime = DBGetContactSettingWord(NULL, IRCPROTONAME, "OnlineNotificationTime", 30);
	prefs->OnlineNotificationLimit = DBGetContactSettingWord(NULL, IRCPROTONAME, "OnlineNotificationLimit", 50);
	prefs->ChannelAwayNotification = DBGetContactSettingByte( NULL, IRCPROTONAME, "ChannelAwayNotification", 1);
}

/////////////////////////////////////////////////////////////////////////////////////////
// add icons to the skinning module

struct
{
	char*  szDescr;
	char*  szName;
	int    iSize;
	int    defIconID;
	HANDLE hIconLibItem;
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

void AddIcons(void)
{
	char szFile[MAX_PATH];
	GetModuleFileNameA(g_hInstance, szFile, MAX_PATH);

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszSection = ALTIRCPROTONAME;
	sid.pszDefaultFile = szFile;

	// add them one by one
	for ( int i=0; i < SIZEOF(iconList); i++ ) {
		char szTemp[255];
		mir_snprintf(szTemp, sizeof(szTemp), "%s_%s", IRCPROTONAME, iconList[i].szName );
		sid.pszName = szTemp;
		sid.pszDescription = iconList[i].szDescr;
		sid.iDefaultIndex = -iconList[i].defIconID;
		sid.cx = sid.cy = iconList[i].iSize;
		iconList[i].hIconLibItem = ( HANDLE )CallService( MS_SKIN2_ADDICON, 0, ( LPARAM )&sid );
}	}

HICON LoadIconEx( int iconId )
{
	for ( int i=0; i < SIZEOF(iconList); i++ )
		if ( iconList[i].defIconID == iconId )
			return ( HICON )CallService( MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)iconList[i].hIconLibItem );

	return NULL;
}

HANDLE GetIconHandle( int iconId )
{
	for ( int i=0; i < SIZEOF(iconList); i++ )
		if ( iconList[i].defIconID == iconId )
			return iconList[i].hIconLibItem;

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
// Callback for the 'Add server' dialog

BOOL CALLBACK AddServerProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg);
			int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index = 0; index <i; index++) {
				SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
				if (SendMessageA(GetDlgItem( hwndDlg, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
					SendMessageA(GetDlgItem( hwndDlg, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
			}

			if (m_ssleay32)
				CheckDlgButton( hwndDlg, IDC_OFF, BST_CHECKED);
			else {
				EnableWindow(GetDlgItem( hwndDlg, IDC_ON), FALSE);
				EnableWindow(GetDlgItem( hwndDlg, IDC_OFF), FALSE);
				EnableWindow(GetDlgItem( hwndDlg, IDC_AUTO), FALSE);
			}

			SetWindowTextA(GetDlgItem( hwndDlg, IDC_ADD_PORT), "6667");
			SetWindowTextA(GetDlgItem( hwndDlg, IDC_ADD_PORT2), "6667");
			SetFocus(GetDlgItem( hwndDlg, IDC_ADD_COMBO));	
		}
		return TRUE;

	case WM_COMMAND:
		{
			/*if (HIWORD(wParam) == BN_CLICKED) */{
				switch (LOWORD(wParam)) {	
				case IDN_ADD_OK:
					if (GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_COMBO))) {
						SERVER_INFO* pData = new SERVER_INFO;
						pData->iSSL = 0;
						if(IsDlgButtonChecked( hwndDlg, IDC_ON))
							pData->iSSL = 2;
						if(IsDlgButtonChecked( hwndDlg, IDC_AUTO))
							pData->iSSL = 1;

						pData->PortEnd = getControlText( hwndDlg, IDC_ADD_PORT2 );
						pData->PortStart = getControlText( hwndDlg, IDC_ADD_PORT );
						pData->Address = getControlText( hwndDlg, IDC_ADD_ADDRESS );
						pData->Group = getControlText( hwndDlg, IDC_ADD_COMBO );
						pData->Name = getControlText( hwndDlg, IDC_ADD_SERVER );

						char temp[255];
						mir_snprintf( temp, sizeof(temp), "%s: %s", pData->Group, pData->Name );
						delete pData->Name;
						strcpy( pData->Name = new char[ strlen(temp)+1 ], temp );

						int iItem = SendMessageA(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Name);
						SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
						SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
						SendMessage(connect_hWnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);						
						PostMessage ( hwndDlg, WM_CLOSE, 0,0);
						if ( SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR) {
							int m = SendMessageA(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
							SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
						}
						ServerlistModified = true;
					}
					else MessageBox( hwndDlg, TranslateT("Please complete all fields"), TranslateT("IRC error"), MB_OK | MB_ICONERROR );
					break;
				
				case IDCANCEL:
				case IDN_ADD_CANCEL:
					PostMessage ( hwndDlg, WM_CLOSE, 0,0);
					break;
		}	}	}
		break;

	case WM_CLOSE:
		EnableWindow(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_ADDSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_EDITSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_DELETESERVER), true);
		DestroyWindow( hwndDlg);
		return true;

	case WM_DESTROY:
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Edit server' dialog

BOOL CALLBACK EditServerProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg);
			int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index = 0; index <i; index++) {
				SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
				if (SendMessage(GetDlgItem( hwndDlg, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
					SendMessageA(GetDlgItem( hwndDlg, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
			}
			int j = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, j, 0);
			SetDlgItemTextA( hwndDlg, IDC_ADD_ADDRESS, pData->Address );
			SetDlgItemTextA( hwndDlg, IDC_ADD_COMBO, pData->Group );
			SetDlgItemTextA( hwndDlg, IDC_ADD_PORT, pData->PortStart );
			SetDlgItemTextA( hwndDlg, IDC_ADD_PORT2, pData->PortEnd );
			
			if ( m_ssleay32 ) {
				if ( pData->iSSL == 0 )
					CheckDlgButton( hwndDlg, IDC_OFF, BST_CHECKED );
				if ( pData->iSSL == 1 )
					CheckDlgButton( hwndDlg, IDC_AUTO, BST_CHECKED );
				if ( pData->iSSL == 2 )
					CheckDlgButton( hwndDlg, IDC_ON, BST_CHECKED );
			}
			else {
				EnableWindow(GetDlgItem( hwndDlg, IDC_ON), FALSE);
				EnableWindow(GetDlgItem( hwndDlg, IDC_OFF), FALSE);
				EnableWindow(GetDlgItem( hwndDlg, IDC_AUTO), FALSE);
			}

			char* p = strchr( pData->Name, ' ' );
			if ( p )
				SetDlgItemTextA( hwndDlg, IDC_ADD_SERVER, p+1);

			SetFocus(GetDlgItem( hwndDlg, IDC_ADD_COMBO));	
		}
		return TRUE;

	case WM_COMMAND:
		//if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {	
			case IDN_ADD_OK:
				if (GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ADD_COMBO))) {
					int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);

					SERVER_INFO* pData1 = ( SERVER_INFO* )SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
					delete []pData1->Name;
					delete []pData1->Address;
					delete []pData1->PortStart;
					delete []pData1->PortEnd;
					delete []pData1->Group;
					delete pData1;	
					SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);

					SERVER_INFO* pData = new SERVER_INFO;
					pData->iSSL = 0;
					if ( IsDlgButtonChecked( hwndDlg, IDC_ON ))
						pData->iSSL = 2;
					if ( IsDlgButtonChecked( hwndDlg, IDC_AUTO ))
						pData->iSSL = 1;

					pData->PortEnd = getControlText( hwndDlg, IDC_ADD_PORT2 );
					pData->PortStart = getControlText( hwndDlg, IDC_ADD_PORT );
					pData->Address = getControlText( hwndDlg, IDC_ADD_ADDRESS );
					pData->Group = getControlText( hwndDlg, IDC_ADD_COMBO );
					pData->Name = getControlText( hwndDlg, IDC_ADD_SERVER );

					char temp[255];
					mir_snprintf( temp, sizeof(temp), "%s: %s", pData->Group, pData->Name );
					delete pData->Name;
					strcpy( pData->Name = new char[ strlen(temp)+1 ], temp );

					int iItem = SendMessageA(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Name);
					SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
					SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
					SendMessage(connect_hWnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
					PostMessage ( hwndDlg, WM_CLOSE, 0,0);
					if ( SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR) {
						int m = SendMessageA(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
						SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
					}
					ServerlistModified = true;
				}
				else MessageBox( hwndDlg, TranslateT("Please complete all fields"), TranslateT("IRC error"), MB_OK | MB_ICONERROR );
				break;

			case IDCANCEL:
			case IDN_ADD_CANCEL:
				PostMessage ( hwndDlg, WM_CLOSE, 0,0);
				break;
			}
		break;

	case WM_CLOSE:
		EnableWindow(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_ADDSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_EDITSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_DELETESERVER), true);
		DestroyWindow( hwndDlg);
		return true;

	case WM_DESTROY:
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'CTCP preferences' dialog

BOOL CALLBACK CtcpPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg);
			SetDlgItemText( hwndDlg,IDC_USERINFO,prefs->UserInfo);

			CheckDlgButton( hwndDlg, IDC_SLOW, DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0)==0?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton( hwndDlg, IDC_FAST, DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0)==1?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton( hwndDlg, IDC_DISC, prefs->DisconnectDCCChats?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton( hwndDlg, IDC_PASSIVE, prefs->DCCPassive?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton( hwndDlg, IDC_SENDNOTICE, prefs->SendNotice?BST_CHECKED:BST_UNCHECKED);

			SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "256");
			SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "512");
			SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "1024");
			SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "2048");
			SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "4096");
			SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "8192");

			int j = DBGetContactSettingWord(NULL, IRCPROTONAME, "DCCPacketSize", 1024*4);
			char szTemp[10];
			mir_snprintf(szTemp, sizeof(szTemp), "%u", j);
			int i = SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) szTemp);
			if ( i == CB_ERR)
				int i = SendDlgItemMessageA( hwndDlg, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) "4096");

			if(prefs->DCCChatAccept == 1)
				CheckDlgButton( hwndDlg, IDC_RADIO1, BST_CHECKED);
			if(prefs->DCCChatAccept == 2)
				CheckDlgButton( hwndDlg, IDC_RADIO2, BST_CHECKED);
			if(prefs->DCCChatAccept == 3)
				CheckDlgButton( hwndDlg, IDC_RADIO3, BST_CHECKED);

			CheckDlgButton( hwndDlg, IDC_FROMSERVER, prefs->IPFromServer?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton( hwndDlg, IDC_ENABLEIP, prefs->ManualHost?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem( hwndDlg, IDC_IP), prefs->ManualHost);
			EnableWindow(GetDlgItem( hwndDlg, IDC_FROMSERVER), !prefs->ManualHost);
			if (prefs->ManualHost)
				SetDlgItemTextA( hwndDlg,IDC_IP,prefs->MySpecifiedHost);
			else {
				if ( prefs->IPFromServer ) {
					if ( prefs->MyHost[0] ) {
						TString s = (TString)TranslateT("<Resolved IP: ") + (TCHAR*)_A2T(prefs->MyHost) + _T(">");
						SetDlgItemText( hwndDlg, IDC_IP, s.c_str());
					}
					else SetDlgItemText( hwndDlg, IDC_IP, TranslateT( "<Automatic>" ));
				}
				else {
					if ( prefs->MyLocalHost[0] ) {
						TString s = ( TString )TranslateT( "<Local IP: " ) + (TCHAR*)_A2T(prefs->MyLocalHost) + _T(">");
						SetDlgItemText( hwndDlg, IDC_IP, s.c_str());
					}
					else SetDlgItemText( hwndDlg, IDC_IP, TranslateT( "<Automatic>" ));
		}	}	}
		return TRUE;

	case WM_COMMAND:
		if ((	LOWORD(wParam) == IDC_USERINFO ||
			   LOWORD(wParam) == IDC_IP ||
				LOWORD(wParam) == IDC_COMBO && HIWORD(wParam) != CBN_SELCHANGE) && (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))
			return true;

		SendMessage( GetParent( hwndDlg), PSM_CHANGED, 0, 0 );

		if ( HIWORD(wParam) == BN_CLICKED ) {
			switch (LOWORD(wParam)) {	
			case IDC_FROMSERVER:
			case IDC_ENABLEIP:
				EnableWindow(GetDlgItem( hwndDlg, IDC_IP), IsDlgButtonChecked( hwndDlg, IDC_ENABLEIP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_FROMSERVER), IsDlgButtonChecked( hwndDlg, IDC_ENABLEIP)== BST_UNCHECKED);

				if ( IsDlgButtonChecked( hwndDlg, IDC_ENABLEIP ) == BST_CHECKED )
					SetDlgItemTextA( hwndDlg, IDC_IP, prefs->MySpecifiedHost );
				else {
					if ( IsDlgButtonChecked( hwndDlg, IDC_FROMSERVER )== BST_CHECKED ) {
						if ( prefs->MyHost[0] ) {
							TString s = (TString)TranslateT( "<Resolved IP: ") + (TCHAR*)_A2T(prefs->MyHost) + _T(">");
							SetDlgItemText( hwndDlg, IDC_IP, s.c_str());
						}
						else SetDlgItemText( hwndDlg, IDC_IP, TranslateT( "<Automatic>" ));
					}
					else {
						if ( prefs->MyLocalHost[0] ) {
							TString s = ( TString )TranslateT( "<Local IP: " ) + (TCHAR*)_A2T(prefs->MyLocalHost) + _T(">");
							SetDlgItemText( hwndDlg, IDC_IP, s.c_str());
						}
						else SetDlgItemText( hwndDlg, IDC_IP, TranslateT( "<Automatic>" ));
				}	}
				break;
		}	}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				{
					GetDlgItemText( hwndDlg,IDC_USERINFO,prefs->UserInfo, 499);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"UserInfo",prefs->UserInfo);

					char szTemp[10];
					GetWindowTextA(GetDlgItem( hwndDlg, IDC_COMBO), szTemp, 10);
					DBWriteContactSettingWord(NULL,IRCPROTONAME,"DCCPacketSize", (WORD)atoi(szTemp));

					prefs->DCCPassive = IsDlgButtonChecked( hwndDlg,IDC_PASSIVE)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DccPassive",prefs->DCCPassive);

					prefs->SendNotice = IsDlgButtonChecked( hwndDlg,IDC_SENDNOTICE)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"SendNotice",prefs->SendNotice);

					if ( IsDlgButtonChecked( hwndDlg,IDC_SLOW)== BST_CHECKED)
						DBWriteContactSettingByte(NULL,IRCPROTONAME,"DCCMode",0);
					else 
						DBWriteContactSettingByte(NULL,IRCPROTONAME,"DCCMode",1);

					prefs->ManualHost = IsDlgButtonChecked( hwndDlg,IDC_ENABLEIP)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ManualHost",prefs->ManualHost);

					prefs->IPFromServer = IsDlgButtonChecked( hwndDlg,IDC_FROMSERVER)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"IPFromServer",prefs->IPFromServer);

					prefs->DisconnectDCCChats = IsDlgButtonChecked( hwndDlg,IDC_DISC)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisconnectDCCChats",prefs->DisconnectDCCChats);

					if ( IsDlgButtonChecked( hwndDlg, IDC_ENABLEIP) == BST_CHECKED) {
						char szTemp[500];
						GetDlgItemTextA( hwndDlg,IDC_IP,szTemp, 499);
						lstrcpynA(prefs->MySpecifiedHost, GetWord(szTemp, 0).c_str(), 499);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"SpecHost",prefs->MySpecifiedHost);
						if ( lstrlenA( prefs->MySpecifiedHost ))
							mir_forkthread( ResolveIPThread, new IPRESOLVE( prefs->MySpecifiedHost, IP_MANUAL ));
					}

					if(IsDlgButtonChecked( hwndDlg, IDC_RADIO1) == BST_CHECKED)
						prefs->DCCChatAccept = 1;
					if(IsDlgButtonChecked( hwndDlg, IDC_RADIO2) == BST_CHECKED)
						prefs->DCCChatAccept = 2;
					if(IsDlgButtonChecked( hwndDlg, IDC_RADIO3) == BST_CHECKED)
						prefs->DCCChatAccept = 3;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"CtcpChatAccept",prefs->DCCChatAccept);
		}	}	}
		break;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Advanced preferences' dialog

bool PerformlistModified = false;

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

static void addPerformComboValue( HWND hWnd, int idx, const char* szValueName )
{
	String sSetting = String("PERFORM:") + szValueName;
	transform( sSetting.begin(), sSetting.end(), sSetting.begin(), toupper );

	PERFORM_INFO* pPref;
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, sSetting.c_str(), &dbv )) {
		pPref = new PERFORM_INFO( sSetting.c_str(), dbv.ptszVal );
		DBFreeVariant( &dbv );
	}
	else pPref = new PERFORM_INFO( sSetting.c_str(), _T(""));
	SendMessage( hWnd, CB_SETITEMDATA, idx, ( LPARAM )pPref );
}

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

BOOL CALLBACK OtherPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg);

			OldProc = (WNDPROC)SetWindowLong(GetDlgItem( hwndDlg, IDC_ALIASEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 
			SetWindowLong(GetDlgItem( hwndDlg, IDC_QUITMESSAGE), GWL_WNDPROC,(LONG)EditSubclassProc); 
			SetWindowLong(GetDlgItem( hwndDlg, IDC_PERFORMEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 

			SendDlgItemMessage( hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_ADD));
			SendDlgItemMessage( hwndDlg,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
			SendMessage(GetDlgItem( hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Click to set commands that will be performed for this event"), 0);
			SendMessage(GetDlgItem( hwndDlg,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Click to delete the commands for this event"), 0);

			SetDlgItemText( hwndDlg, IDC_ALIASEDIT, prefs->Alias );
			SetDlgItemText( hwndDlg, IDC_QUITMESSAGE, prefs->QuitMessage );
			CheckDlgButton( hwndDlg, IDC_PERFORM, ((prefs->Perform) ? (BST_CHECKED) : (BST_UNCHECKED)));
			CheckDlgButton( hwndDlg, IDC_SCRIPT, ((prefs->ScriptingEnabled) ? (BST_CHECKED) : (BST_UNCHECKED)));
			EnableWindow(GetDlgItem( hwndDlg, IDC_SCRIPT), bMbotInstalled);
			EnableWindow(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), prefs->Perform);
			EnableWindow(GetDlgItem( hwndDlg, IDC_PERFORMEDIT), prefs->Perform);
			EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), prefs->Perform);
			EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), prefs->Perform);

			FillCodePageCombo( GetDlgItem( hwndDlg, IDC_CODEPAGE ), prefs->Codepage );
			if ( prefs->Codepage == CP_UTF8 )
				EnableWindow( GetDlgItem( hwndDlg, IDC_UTF_AUTODETECT ), FALSE );

			HWND hwndPerform = GetDlgItem( hwndDlg, IDC_PERFORMCOMBO );

			if ( pszServerFile ) {
				char * p1 = pszServerFile;
				char * p2 = pszServerFile;

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
					int i = SendDlgItemMessageA( hwndDlg, IDC_PERFORMCOMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Group);
					if (i == CB_ERR) {
						int idx = SendMessageA(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_ADDSTRING, 0, (LPARAM) Group);
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
			SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_PERFORMCOMBO, CBN_SELCHANGE), 0);
			CheckDlgButton( hwndDlg, IDC_UTF_AUTODETECT, (prefs->UtfAutodetect) ? BST_CHECKED : BST_UNCHECKED );
			PerformlistModified = false;
		}	
		return TRUE;

	case WM_COMMAND:
		if ((	LOWORD(wParam) == IDC_ALIASEDIT ||
			LOWORD(wParam) == IDC_PERFORMEDIT ||
			LOWORD(wParam) == IDC_QUITMESSAGE ||
			LOWORD(wParam) == IDC_PERFORMCOMBO && HIWORD(wParam) != CBN_SELCHANGE )
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	
			return true;

		if ( HIWORD(wParam) == STN_CLICKED ) 
			if ( LOWORD(wParam) == IDC_CUSTOM )
				CallService( MS_UTILS_OPENURL,0,(LPARAM) "http://members.chello.se/matrix/index.html" );

		if ( HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PERFORMCOMBO) {
			int i = SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
			PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
			if (pPerf == 0)
				SetDlgItemTextA( hwndDlg, IDC_PERFORMEDIT, "");
			else
				SetDlgItemText( hwndDlg, IDC_PERFORMEDIT, pPerf->mText.c_str());
			EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), false);
			if ( GetWindowTextLength(GetDlgItem( hwndDlg, IDC_PERFORMEDIT)) != 0)
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), true );
			else
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), false );
			return false;
		}
		SendMessage(GetParent( hwndDlg), PSM_CHANGED,0,0);

		if ( HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_CODEPAGE) {
			int curSel = SendDlgItemMessage( hwndDlg, IDC_CODEPAGE, CB_GETCURSEL, 0, 0 );
			EnableWindow( GetDlgItem( hwndDlg, IDC_UTF_AUTODETECT ),
				SendDlgItemMessage( hwndDlg, IDC_CODEPAGE, CB_GETITEMDATA, curSel, 0 ) != CP_UTF8 );
		}

		if ( HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_PERFORMEDIT) {
			EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), true);

			if ( GetWindowTextLength(GetDlgItem( hwndDlg, IDC_PERFORMEDIT)) != 0)
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), true);
			else
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), false);
		}

		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {	
			case IDC_PERFORM:
				EnableWindow(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), IsDlgButtonChecked( hwndDlg, IDC_PERFORM)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_PERFORMEDIT), IsDlgButtonChecked( hwndDlg, IDC_PERFORM)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), IsDlgButtonChecked( hwndDlg, IDC_PERFORM)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), IsDlgButtonChecked( hwndDlg, IDC_PERFORM)== BST_CHECKED);
				break;

			case IDC_ADD:
				{
					int j = GetWindowTextLength(GetDlgItem( hwndDlg, IDC_PERFORMEDIT));
					TCHAR* temp = new TCHAR[j+1];
					GetWindowText(GetDlgItem( hwndDlg, IDC_PERFORMEDIT), temp, j+1);

					if ( my_strstri( temp, _T("/away")))
						MessageBox( NULL, TranslateT("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), TranslateT("IRC Error"), MB_OK);
					else {
						int i = ( int )SendMessage( GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0 );
						if ( i != CB_ERR ) {
							PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
							if ( pPerf != NULL )
								pPerf->mText = temp;

							EnableWindow( GetDlgItem( hwndDlg, IDC_ADD), false );
							PerformlistModified = true;
					}	}
					delete []temp;
				}
				break;

			case IDC_DELETE:
				{
					int i = (int) SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
					if ( i == CB_ERR )
						break;

					PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
					if ( pPerf != NULL ) {
						pPerf->mText = _T("");
						SetDlgItemTextA( hwndDlg, IDC_PERFORMEDIT, "");
						EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), false);
					}

					PerformlistModified = true;
				}
				break;
		}	}
		break;

	case WM_DESTROY:
		PerformlistModified = false;
		{
			int i = SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETCOUNT, 0, 0);
			if ( i != CB_ERR && i != 0 ) {
				for (int index = 0; index < i; index++) {
					PERFORM_INFO* pPerf = (PERFORM_INFO*)SendMessage(GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, index, 0);
					if (( const int )pPerf != CB_ERR && pPerf != NULL )
						delete pPerf;
		}	}	}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				mir_free( prefs->Alias );
				{	int len = GetWindowTextLength(GetDlgItem( hwndDlg, IDC_ALIASEDIT));
					prefs->Alias = ( TCHAR* )mir_alloc( sizeof( TCHAR )*( len+1 ));
               GetDlgItemText( hwndDlg, IDC_ALIASEDIT, prefs->Alias, len+1 );
				}
				DBWriteContactSettingTString( NULL, IRCPROTONAME, "Alias", prefs->Alias );

				GetDlgItemText( hwndDlg,IDC_QUITMESSAGE,prefs->QuitMessage, 399 );
				DBWriteContactSettingTString( NULL, IRCPROTONAME, "QuitMessage", prefs->QuitMessage );
				{
					int curSel = SendDlgItemMessage( hwndDlg, IDC_CODEPAGE, CB_GETCURSEL, 0, 0 );
					prefs->Codepage = SendDlgItemMessage( hwndDlg, IDC_CODEPAGE, CB_GETITEMDATA, curSel, 0 );
					DBWriteContactSettingDword( NULL, IRCPROTONAME, "Codepage", prefs->Codepage );
					if ( g_ircSession )
						g_ircSession.setCodepage( prefs->Codepage );
				}

				prefs->UtfAutodetect = IsDlgButtonChecked( hwndDlg,IDC_UTF_AUTODETECT)== BST_CHECKED;
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"UtfAutodetect",prefs->UtfAutodetect);
				prefs->Perform = IsDlgButtonChecked( hwndDlg,IDC_PERFORM)== BST_CHECKED;
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"Perform",prefs->Perform);
				prefs->ScriptingEnabled = IsDlgButtonChecked( hwndDlg,IDC_SCRIPT)== BST_CHECKED;
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"ScriptingEnabled",prefs->ScriptingEnabled);
				if (IsWindowEnabled(GetDlgItem( hwndDlg, IDC_ADD)))
					SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ADD, BN_CLICKED), 0);
            
				if ( PerformlistModified ) {
					PerformlistModified = false;

					int count = SendDlgItemMessage( hwndDlg, IDC_PERFORMCOMBO, CB_GETCOUNT, 0, 0);
					for ( int i = 0; i < count; i++ ) {
						PERFORM_INFO* pPerf = ( PERFORM_INFO* )SendMessage( GetDlgItem( hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0 );
						if (( const int )pPerf == CB_ERR )
							continue;

						if ( !pPerf->mText.empty())
							DBWriteContactSettingTString( NULL, IRCPROTONAME, pPerf->mSetting.c_str(), pPerf->mText.c_str());
						else 
							DBDeleteContactSetting( NULL, IRCPROTONAME, pPerf->mSetting.c_str());
				}	}
				return TRUE;
		}	}
		break;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Connect preferences' dialog

BOOL CALLBACK ConnectPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CHAR:
		SendMessage(GetParent( hwndDlg), PSM_CHANGED,0,0);
		break;

	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);

		SendDlgItemMessage( hwndDlg,IDC_ADDSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_ADD));
		SendDlgItemMessage( hwndDlg,IDC_DELETESERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
		SendDlgItemMessage( hwndDlg,IDC_EDITSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_RENAME));
		SendMessage(GetDlgItem( hwndDlg,IDC_ADDSERVER), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Add a new network"), 0);
		SendMessage(GetDlgItem( hwndDlg,IDC_EDITSERVER), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Edit this network"), 0);
		SendMessage(GetDlgItem( hwndDlg,IDC_DELETESERVER), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Delete this network"), 0);

		connect_hWnd = hwndDlg;

		//	Fill the servers combo box and create SERVER_INFO structures
		if ( pszServerFile ) {
			char* p1 = pszServerFile;
			char* p2 = pszServerFile;
			while (strchr(p2, 'n')) {
				SERVER_INFO* pData = new SERVER_INFO;
				p1 = strchr(p2, '=');
				++p1;
				p2 = strstr(p1, "SERVER:");
				pData->Name=new char[p2-p1+1];
				lstrcpynA(pData->Name, p1, p2-p1+1);

				p1 = strchr(p2, ':');
				++p1;
				pData->iSSL = 0;
				if(strstr(p1, "SSL") == p1) {
					p1 +=3;
					if(*p1 == '1')
						pData->iSSL = 1;
					else if(*p1 == '2')
						pData->iSSL = 2;
					p1++;
				}
				p2 = strchr(p1, ':');
				pData->Address=new char[p2-p1+1];
				lstrcpynA(pData->Address, p1, p2-p1+1);

				p1 = p2;
				p1++;
				while (*p2 !='G' && *p2 != '-')
					p2++;
				pData->PortStart = new char[p2-p1+1];
				lstrcpynA(pData->PortStart, p1, p2-p1+1);

				if (*p2 == 'G'){
					pData->PortEnd = new char[p2-p1+1];
					lstrcpyA(pData->PortEnd, pData->PortStart);
				}
				else {
					p1 = p2;
					p1++;
					p2 = strchr(p1, 'G');
					pData->PortEnd = new char[p2-p1+1];
					lstrcpynA(pData->PortEnd, p1, p2-p1+1);
				}

				p1 = strchr(p2, ':');
				p1++;
				p2 = strchr(p1, '\r');
				if (!p2)
					p2 = strchr(p1, '\n');
				if (!p2)
					p2 = strchr(p1, '\0');
				pData->Group = new char[p2-p1+1];
				lstrcpynA(pData->Group, p1, p2-p1+1);
				int iItem = SendDlgItemMessageA( hwndDlg, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->Name);
				SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);
		}	}

		SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_SETCURSEL, prefs->ServerComboSelection,0);				
		SetDlgItemTextA( hwndDlg,IDC_SERVER, prefs->ServerName);
		SetDlgItemTextA( hwndDlg,IDC_PORT, prefs->PortStart);
		SetDlgItemTextA( hwndDlg,IDC_PORT2, prefs->PortEnd);
		if ( m_ssleay32 ) {
			if ( prefs->iSSL == 0 )
				SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "Off" ));
			if ( prefs->iSSL == 1 )
				SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "Auto" ));
			if ( prefs->iSSL == 2 )
				SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "On" ));
		}
		else SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "N/A" ));

		if ( prefs->ServerComboSelection != -1 ) {
			SERVER_INFO* pData = ( SERVER_INFO* )SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_GETITEMDATA, prefs->ServerComboSelection, 0);
			if ((int)pData != CB_ERR) {
				SetDlgItemTextA( hwndDlg, IDC_SERVER, pData->Address   );
				SetDlgItemTextA( hwndDlg, IDC_PORT,   pData->PortStart );
				SetDlgItemTextA( hwndDlg, IDC_PORT2,  pData->PortEnd   );
		}	}

		SendDlgItemMessage( hwndDlg,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(999,20));
		SendDlgItemMessage( hwndDlg,IDC_SPIN1,UDM_SETPOS,0,MAKELONG(prefs->OnlineNotificationTime,0));
		SendDlgItemMessage( hwndDlg,IDC_SPIN2,UDM_SETRANGE,0,MAKELONG(200,0));
		SendDlgItemMessage( hwndDlg,IDC_SPIN2,UDM_SETPOS,0,MAKELONG(prefs->OnlineNotificationLimit,0));
		SetDlgItemText( hwndDlg,IDC_NICK,prefs->Nick);
		SetDlgItemText( hwndDlg,IDC_NICK2,prefs->AlternativeNick);
		SetDlgItemText( hwndDlg,IDC_USERID,prefs->UserID);
		SetDlgItemText( hwndDlg,IDC_NAME,prefs->Name);
		SetDlgItemTextA( hwndDlg,IDC_PASS,prefs->Password);
		SetDlgItemText( hwndDlg,IDC_IDENTSYSTEM,prefs->IdentSystem);
		SetDlgItemText( hwndDlg,IDC_IDENTPORT,prefs->IdentPort);
		SetDlgItemText( hwndDlg,IDC_RETRYWAIT,prefs->RetryWait);
		SetDlgItemText( hwndDlg,IDC_RETRYCOUNT,prefs->RetryCount);			
		CheckDlgButton( hwndDlg,IDC_ADDRESS, ((prefs->ShowAddresses) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_OLDSTYLE, ((prefs->OldStyleModes) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_CHANNELAWAY, ((prefs->ChannelAwayNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_ONLINENOTIF, ((prefs->AutoOnlineNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( hwndDlg, IDC_ONLINETIMER), prefs->AutoOnlineNotification);
		EnableWindow(GetDlgItem( hwndDlg, IDC_CHANNELAWAY), prefs->AutoOnlineNotification);
		EnableWindow(GetDlgItem( hwndDlg, IDC_SPIN1), prefs->AutoOnlineNotification);
		EnableWindow(GetDlgItem( hwndDlg, IDC_SPIN2), prefs->AutoOnlineNotification && prefs->ChannelAwayNotification);
		EnableWindow(GetDlgItem( hwndDlg, IDC_LIMIT), prefs->AutoOnlineNotification && prefs->ChannelAwayNotification);
		CheckDlgButton( hwndDlg,IDC_IDENT, ((prefs->Ident) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( hwndDlg, IDC_IDENTSYSTEM), prefs->Ident);
		EnableWindow(GetDlgItem( hwndDlg, IDC_IDENTPORT), prefs->Ident);
		CheckDlgButton( hwndDlg,IDC_DISABLEERROR, ((prefs->DisableErrorPopups) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_FORCEVISIBLE, ((prefs->ForceVisible) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_REJOINCHANNELS, ((prefs->RejoinChannels) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_REJOINONKICK, ((prefs->RejoinIfKicked) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton( hwndDlg,IDC_RETRY, ((prefs->Retry) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem( hwndDlg, IDC_RETRYWAIT), prefs->Retry);
		EnableWindow(GetDlgItem( hwndDlg, IDC_RETRYCOUNT), prefs->Retry);
		CheckDlgButton( hwndDlg,IDC_STARTUP, ((!prefs->DisableDefaultServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( hwndDlg,IDC_KEEPALIVE, ((prefs->SendKeepAlive) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( hwndDlg,IDC_IDENT_TIMED, ((prefs->IdentTimer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( hwndDlg,IDC_USESERVER, ((prefs->UseServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( hwndDlg,IDC_SHOWSERVER, ((!prefs->HideServerWindow) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton( hwndDlg,IDC_AUTOJOIN, ((prefs->JoinOnInvite) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		EnableWindow(GetDlgItem( hwndDlg, IDC_SHOWSERVER), prefs->UseServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_ADDSERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_EDITSERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_DELETESERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_SERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_PORT), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_PORT2), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_PASS), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem( hwndDlg, IDC_IDENT_TIMED), prefs->Ident);
		ServerlistModified = false;
		return TRUE;

	case WM_COMMAND:
		if(	(LOWORD(wParam)		  == IDC_SERVER
			|| LOWORD(wParam) == IDC_PORT
			|| LOWORD(wParam) == IDC_PORT2
			|| LOWORD(wParam) == IDC_PASS
			|| LOWORD(wParam) == IDC_NICK
			|| LOWORD(wParam) == IDC_NICK2
			|| LOWORD(wParam) == IDC_NAME
			|| LOWORD(wParam) == IDC_USERID
			|| LOWORD(wParam) == IDC_IDENTSYSTEM
			|| LOWORD(wParam) == IDC_IDENTPORT
			|| LOWORD(wParam) == IDC_ONLINETIMER
			|| LOWORD(wParam) == IDC_LIMIT
			|| LOWORD(wParam) == IDC_RETRYWAIT
			|| LOWORD(wParam) == IDC_RETRYCOUNT
			|| LOWORD(wParam) == IDC_SSL
			|| LOWORD(wParam) == IDC_SERVERCOMBO && HIWORD(wParam) != CBN_SELCHANGE
			)
			&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;

		if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_SERVERCOMBO) {
			int i = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			if (pData && (int)pData != CB_ERR ) {
				SetDlgItemTextA( hwndDlg,IDC_SERVER,pData->Address);
				SetDlgItemTextA( hwndDlg,IDC_PORT,pData->PortStart);
				SetDlgItemTextA( hwndDlg,IDC_PORT2,pData->PortEnd);
				SetDlgItemTextA( hwndDlg,IDC_PASS,"");
				if ( m_ssleay32 ) {
					if ( pData->iSSL == 0 )
						SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "Off" ));
					if ( pData->iSSL == 1 )
						SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "Auto" ));
					if ( pData->iSSL == 2 )
						SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "On" ));
				}
				else SetDlgItemText( hwndDlg, IDC_SSL, TranslateT( "N/A" ));
				SendMessage(GetParent( hwndDlg), PSM_CHANGED,0,0);
			}
			return false;
		}
		SendMessage(GetParent( hwndDlg), PSM_CHANGED,0,0);

		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {
			case IDC_DELETESERVER:
				{
					int i = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					if (i != CB_ERR) {
						EnableWindow(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_ADDSERVER), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_EDITSERVER), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_DELETESERVER), false);
						
						SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
						TCHAR temp[200];
						mir_sntprintf( temp, SIZEOF(temp), TranslateT("Do you want to delete\r\n%s"), (TCHAR*)_A2T(pData->Name));
						if ( MessageBox( hwndDlg, temp, TranslateT("Delete server"), MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
							delete []pData->Name;
							delete []pData->Address;
							delete []pData->PortStart;
							delete []pData->PortEnd;
							delete []pData->Group;
							delete pData;	
							SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);
							if (i >= SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0))
								i--;
							SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_SETCURSEL, i, 0);
							SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
							ServerlistModified = true;
						}

						EnableWindow(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), true);
						EnableWindow(GetDlgItem( hwndDlg, IDC_ADDSERVER), true);
						EnableWindow(GetDlgItem( hwndDlg, IDC_EDITSERVER), true);
						EnableWindow(GetDlgItem( hwndDlg, IDC_DELETESERVER), true);
				}	}
				break;

			case IDC_ADDSERVER:
				EnableWindow(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ADDSERVER), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_EDITSERVER), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETESERVER), false);
				addserver_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_ADDSERVER),hwndDlg,AddServerProc);
				break;

			case IDC_EDITSERVER:
				{
					int i = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					if (i != CB_ERR) {
						EnableWindow(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_ADDSERVER), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_EDITSERVER), false);
						EnableWindow(GetDlgItem( hwndDlg, IDC_DELETESERVER), false);
						addserver_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_ADDSERVER),hwndDlg,EditServerProc);
						SetWindowText( addserver_hWnd, TranslateT( "Edit server" ));
				}	}
				break;

			case IDC_STARTUP:
				EnableWindow(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ADDSERVER), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_EDITSERVER), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_DELETESERVER), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_SERVER), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_PORT), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_PORT2), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_PASS), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_SSL), IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED);
				break;	

			case IDC_IDENT:
				EnableWindow(GetDlgItem( hwndDlg, IDC_IDENTSYSTEM), IsDlgButtonChecked( hwndDlg, IDC_IDENT)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_IDENTPORT), IsDlgButtonChecked( hwndDlg, IDC_IDENT)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_IDENT_TIMED), IsDlgButtonChecked( hwndDlg, IDC_IDENT)== BST_CHECKED);
				break;

			case IDC_USESERVER:
				EnableWindow(GetDlgItem( hwndDlg, IDC_SHOWSERVER), IsDlgButtonChecked( hwndDlg, IDC_USESERVER)== BST_CHECKED);
				break;

			case IDC_ONLINENOTIF:
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHANNELAWAY), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ONLINETIMER), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_SPIN1), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_SPIN2), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_LIMIT), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				break;

			case IDC_CHANNELAWAY:
				EnableWindow(GetDlgItem( hwndDlg, IDC_SPIN2), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_LIMIT), IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked( hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				break;

			case IDC_RETRY:
				EnableWindow(GetDlgItem( hwndDlg, IDC_RETRYWAIT), IsDlgButtonChecked( hwndDlg, IDC_RETRY)== BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_RETRYCOUNT), IsDlgButtonChecked( hwndDlg, IDC_RETRY)== BST_CHECKED);
				break;
		}	}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				{
					//Save the setting in the CONNECT dialog
					if(IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED) {
						GetDlgItemTextA( hwndDlg,IDC_SERVER, prefs->ServerName, 101);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
						GetDlgItemTextA( hwndDlg,IDC_PORT, prefs->PortStart, 6);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
						GetDlgItemTextA( hwndDlg,IDC_PORT2, prefs->PortEnd, 6);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
						GetDlgItemTextA( hwndDlg,IDC_PASS, prefs->Password, 500);
						CallService( MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)prefs->Password);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
						CallService( MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)prefs->Password);
					}
					else {
						lstrcpyA(prefs->ServerName, "");
						DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
						lstrcpyA(prefs->PortStart, "");
						DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
						lstrcpyA(prefs->PortEnd, "");
						DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
						lstrcpyA( prefs->Password, "");
						DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
					}

					prefs->OnlineNotificationTime = SendDlgItemMessage( hwndDlg,IDC_SPIN1,UDM_GETPOS,0,0);
					DBWriteContactSettingWord(NULL, IRCPROTONAME, "OnlineNotificationTime", (BYTE)prefs->OnlineNotificationTime);

					prefs->OnlineNotificationLimit = SendDlgItemMessage( hwndDlg,IDC_SPIN2,UDM_GETPOS,0,0);
					DBWriteContactSettingWord(NULL, IRCPROTONAME, "OnlineNotificationLimit", (BYTE)prefs->OnlineNotificationLimit);

					prefs->ChannelAwayNotification = IsDlgButtonChecked( hwndDlg, IDC_CHANNELAWAY )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ChannelAwayNotification",prefs->ChannelAwayNotification);

					GetDlgItemText( hwndDlg,IDC_NICK, prefs->Nick, 30);
					removeSpaces(prefs->Nick);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"PNick",prefs->Nick);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"Nick",prefs->Nick);
					GetDlgItemText( hwndDlg,IDC_NICK2, prefs->AlternativeNick, 30);
					removeSpaces(prefs->AlternativeNick);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"AlernativeNick",prefs->AlternativeNick);
					GetDlgItemText( hwndDlg,IDC_USERID, prefs->UserID, 199);
					removeSpaces(prefs->UserID);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"UserID",prefs->UserID);
					GetDlgItemText( hwndDlg,IDC_NAME, prefs->Name, 199);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"Name",prefs->Name);
					GetDlgItemText( hwndDlg,IDC_IDENTSYSTEM, prefs->IdentSystem, 10);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"IdentSystem",prefs->IdentSystem);
					GetDlgItemText( hwndDlg,IDC_IDENTPORT, prefs->IdentPort, 6);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"IdentPort",prefs->IdentPort);
					GetDlgItemText( hwndDlg,IDC_RETRYWAIT, prefs->RetryWait, 4);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"RetryWait",prefs->RetryWait);
					GetDlgItemText( hwndDlg,IDC_RETRYCOUNT, prefs->RetryCount, 4);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"RetryCount",prefs->RetryCount);
					prefs->DisableDefaultServer = !IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisableDefaultServer",prefs->DisableDefaultServer);
					prefs->Ident = IsDlgButtonChecked( hwndDlg, IDC_IDENT)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"Ident",prefs->Ident);
					prefs->IdentTimer = IsDlgButtonChecked( hwndDlg, IDC_IDENT_TIMED)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"IdentTimer",prefs->IdentTimer);
					prefs->ForceVisible = IsDlgButtonChecked( hwndDlg, IDC_FORCEVISIBLE)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ForceVisible",prefs->ForceVisible);
					prefs->DisableErrorPopups = IsDlgButtonChecked( hwndDlg, IDC_DISABLEERROR)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisableErrorPopups",prefs->DisableErrorPopups);
					prefs->RejoinChannels = IsDlgButtonChecked( hwndDlg, IDC_REJOINCHANNELS)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"RejoinChannels",prefs->RejoinChannels);
					prefs->RejoinIfKicked = IsDlgButtonChecked( hwndDlg, IDC_REJOINONKICK)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"RejoinIfKicked",prefs->RejoinIfKicked);
					prefs->Retry = IsDlgButtonChecked( hwndDlg, IDC_RETRY)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"Retry",prefs->Retry);
					prefs->ShowAddresses = IsDlgButtonChecked( hwndDlg, IDC_ADDRESS)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ShowAddresses",prefs->ShowAddresses);
					prefs->OldStyleModes = IsDlgButtonChecked( hwndDlg, IDC_OLDSTYLE)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"OldStyleModes",prefs->OldStyleModes);

					prefs->UseServer = IsDlgButtonChecked( hwndDlg, IDC_USESERVER )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseServer",prefs->UseServer);

					CLISTMENUITEM clmi;
					memset( &clmi, 0, sizeof( clmi ));
					clmi.cbSize = sizeof( clmi );
					clmi.flags = CMIM_FLAGS;
					if ( !prefs->UseServer )
						clmi.flags |= CMIF_GRAYED;
					CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuServer, ( LPARAM )&clmi );

					prefs->JoinOnInvite = IsDlgButtonChecked( hwndDlg, IDC_AUTOJOIN )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"JoinOnInvite",prefs->JoinOnInvite);
					prefs->HideServerWindow = IsDlgButtonChecked( hwndDlg, IDC_SHOWSERVER )== BST_UNCHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"HideServerWindow",prefs->HideServerWindow);
					prefs->ServerComboSelection = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					DBWriteContactSettingDword(NULL,IRCPROTONAME,"ServerComboSelection",prefs->ServerComboSelection);
					prefs->SendKeepAlive = IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"SendKeepAlive",prefs->SendKeepAlive);
					if (prefs->SendKeepAlive)
						SetChatTimer(KeepAliveTimer, 60*1000, KeepAliveTimerProc);
					else
						KillChatTimer(KeepAliveTimer);

					prefs->AutoOnlineNotification = IsDlgButtonChecked( hwndDlg, IDC_ONLINENOTIF )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"AutoOnlineNotification",prefs->AutoOnlineNotification);
					if (prefs->AutoOnlineNotification) {
						if(!bTempDisableCheck) {
							SetChatTimer(OnlineNotifTimer, 500, OnlineNotifTimerProc);
							if(prefs->ChannelAwayNotification)
								SetChatTimer(OnlineNotifTimer3, 1500, OnlineNotifTimerProc3);
						}
					}
					else if (!bTempForceCheck) {
						KillChatTimer(OnlineNotifTimer);
						KillChatTimer(OnlineNotifTimer3);
					}

					int i = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
					if ( pData && (int)pData != CB_ERR ) {
						if(IsDlgButtonChecked( hwndDlg, IDC_STARTUP)== BST_CHECKED)
							lstrcpyA(prefs->Network, pData->Group); 
						else
							lstrcpyA(prefs->Network, ""); 
						DBWriteContactSettingString(NULL,IRCPROTONAME,"Network",prefs->Network);
						prefs->iSSL = pData->iSSL;
						DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseSSL",pData->iSSL);			
					}

					if (ServerlistModified) {
						ServerlistModified = false;
						char filepath[MAX_PATH];
						mir_snprintf(filepath, sizeof(filepath), "%s\\%s_servers.ini", mirandapath, IRCPROTONAME);
						FILE *hFile2 = fopen(filepath,"w");
						if (hFile2) {
							int j = (int) SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
							if (j !=CB_ERR && j !=0) {
								for (int index2 = 0; index2 < j; index2++) {
									SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
									if (pData != NULL && (int)pData != CB_ERR) {
										char TextLine[512];
										if(pData->iSSL > 0)
											mir_snprintf(TextLine, sizeof(TextLine), "n%u=%sSERVER:SSL%u%s:%s-%sGROUP:%s\n", index2, pData->Name, pData->iSSL, pData->Address, pData->PortStart, pData->PortEnd, pData->Group);
										else
											mir_snprintf(TextLine, sizeof(TextLine), "n%u=%sSERVER:%s:%s-%sGROUP:%s\n", index2, pData->Name, pData->Address, pData->PortStart, pData->PortEnd, pData->Group);
										fputs(TextLine, hFile2);
							}	}	}

							fclose(hFile2);	
							delete [] pszServerFile;
							pszServerFile = IrcLoadFile(filepath);
				}	}	}
				return TRUE;
		}	}
		break;

	case WM_DESTROY:
		{
			ServerlistModified = false;
			int j = (int) SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
			if (j !=CB_ERR && j !=0 ) {
				for (int index2 = 0; index2 < j; index2++) {
					SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
					if (pData != NULL && (int)pData != CB_ERR) {
						delete []pData->Name;
						delete []pData->Address;
						delete []pData->PortStart;
						delete []pData->PortEnd;
						delete []pData->Group;
						delete pData;	
		}	}	}	}
		break;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Callback for the 'Ignore' preferences dialog

static int CALLBACK IgnoreListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if ( !IgnoreWndHwnd )
		return 1;

	TCHAR temp1[512];
	TCHAR temp2[512];

	LVITEM lvm;
	lvm.mask = LVIF_TEXT;
	lvm.iSubItem = lParamSort;
	lvm.cchTextMax = SIZEOF(temp1);

	lvm.iItem = lParam1;
	lvm.pszText = temp1;
	ListView_GetItem( GetDlgItem(IgnoreWndHwnd, IDC_INFO_LISTVIEW), &lvm );

	lvm.iItem = lParam2;
	lvm.pszText = temp2;
	ListView_GetItem( GetDlgItem(IgnoreWndHwnd, IDC_INFO_LISTVIEW), &lvm );
	
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

void InitIgnore( void )
{
	char szTemp[ MAX_PATH ];
	mir_snprintf(szTemp, sizeof(szTemp), "%s\\%s_ignore.ini", mirandapath, IRCPROTONAME);
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
				g_ignoreItems.push_back( CIrcIgnoreItem( mask.c_str(), flags.c_str(), network.c_str()));

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
		if ( DBGetContactSettingTString( NULL, IRCPROTONAME, settingName, &dbv ))
			break;
		
		TString mask = GetWord( dbv.ptszVal, 0 );
		TString flags = GetWord( dbv.ptszVal, 1 );
		TString network = GetWord( dbv.ptszVal, 2 );
		g_ignoreItems.push_back( CIrcIgnoreItem( mask.c_str(), flags.c_str(), network.c_str()));
		DBFreeVariant( &dbv );
}	}

void RewriteIgnoreSettings( void )
{
	char settingName[ 40 ];

	size_t i=0;
	while ( TRUE ) {
		mir_snprintf( settingName, sizeof(settingName), "IGNORE:%d", i++ );
		if ( DBDeleteContactSetting( NULL, IRCPROTONAME, settingName ))
			break;
	}

	for ( i=0; i < g_ignoreItems.size(); i++ ) {
		mir_snprintf( settingName, sizeof(settingName), "IGNORE:%d", i );

		CIrcIgnoreItem& C = g_ignoreItems[i];
		DBWriteContactSettingTString( NULL, IRCPROTONAME, settingName, ( C.mask + _T(" ") + C.flags + _T(" ") + C.network ).c_str());
}	}

// Callback for the 'Add ignore' dialog
BOOL CALLBACK AddIgnoreWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static TCHAR szOldMask[500];
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg);
			if ( lParam == 0 ) {
				if(g_ircSession)
					SetWindowText(GetDlgItem( hwndDlg, IDC_NETWORK), g_ircSession.GetInfo().sNetwork.c_str());
				CheckDlgButton( hwndDlg, IDC_Q, BST_CHECKED);
				CheckDlgButton( hwndDlg, IDC_N, BST_CHECKED);
				CheckDlgButton( hwndDlg, IDC_I, BST_CHECKED);
				CheckDlgButton( hwndDlg, IDC_D, BST_CHECKED);
				CheckDlgButton( hwndDlg, IDC_C, BST_CHECKED);
				szOldMask[0] = 0;
			}
			else lstrcpyn( szOldMask, (TCHAR*)lParam, SIZEOF(szOldMask));
		}
		return TRUE;

	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED )
			switch ( LOWORD( wParam )) {	
			case IDN_YES:
				{
					TCHAR szMask[500];
					TCHAR szNetwork[500];
					TString flags;
					if ( IsDlgButtonChecked( hwndDlg, IDC_Q ) == BST_CHECKED ) flags += 'q';
					if ( IsDlgButtonChecked( hwndDlg, IDC_N ) == BST_CHECKED ) flags += 'n';
					if ( IsDlgButtonChecked( hwndDlg, IDC_I ) == BST_CHECKED ) flags += 'i';
					if ( IsDlgButtonChecked( hwndDlg, IDC_D ) == BST_CHECKED ) flags += 'd';
					if ( IsDlgButtonChecked( hwndDlg, IDC_C ) == BST_CHECKED ) flags += 'c';
					if ( IsDlgButtonChecked( hwndDlg, IDC_M ) == BST_CHECKED ) flags += 'm';

					GetWindowText( GetDlgItem( hwndDlg, IDC_MASK), szMask, SIZEOF(szMask));
					GetWindowText( GetDlgItem( hwndDlg, IDC_NETWORK), szNetwork, SIZEOF(szNetwork));

					TString Mask = GetWord(szMask, 0);
					if ( Mask.length() != 0 ) {
						if ( !_tcschr(Mask.c_str(), '!') && !_tcschr(Mask.c_str(), '@'))
							Mask += _T("!*@*");

						if ( !flags.empty() ) {
							if ( *szOldMask )
								RemoveIgnore( szOldMask );
							AddIgnore(Mask.c_str(), flags.c_str(), szNetwork);
					}	}

					PostMessage( hwndDlg, WM_CLOSE, 0,0);
				}
				break;

			case IDN_NO:
				PostMessage( hwndDlg, WM_CLOSE, 0,0);
				break;
			}
		break;

	case WM_CLOSE:
		PostMessage(IgnoreWndHwnd, IRC_FIXIGNOREBUTTONS, 0, 0);
		DestroyWindow( hwndDlg);
		return(true);
	}

	return false;
}

BOOL CALLBACK IgnorePrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg);
			OldListViewProc = (WNDPROC)SetWindowLong(GetDlgItem( hwndDlg,IDC_LIST),GWL_WNDPROC,(LONG)ListviewSubclassProc);

			CheckDlgButton( hwndDlg, IDC_ENABLEIGNORE, prefs->Ignore?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton( hwndDlg, IDC_IGNOREFILE, prefs->DCCFileEnabled?BST_UNCHECKED:BST_CHECKED);
			CheckDlgButton( hwndDlg, IDC_IGNORECHAT, prefs->DCCChatEnabled?BST_UNCHECKED:BST_CHECKED);
			CheckDlgButton( hwndDlg, IDC_IGNORECHANNEL, prefs->IgnoreChannelDefault?BST_CHECKED:BST_UNCHECKED);
			if(prefs->DCCChatIgnore == 2)
				CheckDlgButton( hwndDlg, IDC_IGNOREUNKNOWN, BST_CHECKED);

			EnableWindow(GetDlgItem( hwndDlg, IDC_IGNOREUNKNOWN), prefs->DCCChatEnabled);
			EnableWindow(GetDlgItem( hwndDlg, IDC_LIST), prefs->Ignore);
			EnableWindow(GetDlgItem( hwndDlg, IDC_IGNORECHANNEL), prefs->Ignore);
			EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), prefs->Ignore);

			SendDlgItemMessage( hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_ADD));
			SendDlgItemMessage( hwndDlg,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
			SendDlgItemMessage( hwndDlg,IDC_EDIT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_RENAME));
			SendMessage(GetDlgItem( hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Add new ignore"), 0);
			SendMessage(GetDlgItem( hwndDlg,IDC_EDIT), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Edit this ignore"), 0);
			SendMessage(GetDlgItem( hwndDlg,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Delete this ignore"), 0);

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
				ListView_InsertColumn(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW),index,&lvC);
			}

			ListView_SetExtendedListViewStyle(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW), LVS_EX_FULLROWSELECT);
			PostMessage( hwndDlg, IRC_REBUILDIGNORELIST, 0, 0);
		}
		return TRUE;

	case IRC_UPDATEIGNORELIST:
		{
			int j = ListView_GetItemCount(GetDlgItem( hwndDlg, IDC_LIST));
			if (j > 0 ) {
				LVITEM lvm;
				lvm.mask= LVIF_PARAM;
				lvm.iSubItem = 0;
				for( int i =0; i < j; i++) {
					lvm.iItem = i;
					lvm.lParam = i;
					ListView_SetItem(GetDlgItem( hwndDlg, IDC_LIST),&lvm);
		}	}	}
		break;

	case IRC_REBUILDIGNORELIST:
		{
			IgnoreWndHwnd = hwndDlg;
			HWND hListView = GetDlgItem( hwndDlg, IDC_LIST);
			ListView_DeleteAllItems( hListView );

			for ( size_t i=0; i < g_ignoreItems.size(); i++ ) {
				CIrcIgnoreItem& C = g_ignoreItems[i];
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

			SendMessage( hwndDlg, IRC_UPDATEIGNORELIST, 0, 0);
			SendMessage( hListView, LVM_SORTITEMS, (WPARAM)0, (LPARAM)IgnoreListSort);
			SendMessage( hwndDlg, IRC_UPDATEIGNORELIST, 0, 0);

			PostMessage( hwndDlg, IRC_FIXIGNOREBUTTONS, 0, 0);
		}
		break;

	case IRC_FIXIGNOREBUTTONS:
		EnableWindow( GetDlgItem( hwndDlg, IDC_ADD), IsDlgButtonChecked( hwndDlg,IDC_ENABLEIGNORE ));
		if(ListView_GetSelectionMark(GetDlgItem( hwndDlg, IDC_LIST)) != -1) {
			EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), true);
			EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), true);
		}
		else {
			EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_DELETE), false);
		}
		break;

	case WM_COMMAND:
		SendMessage(GetParent( hwndDlg), PSM_CHANGED,0,0);

		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam)) {
			case IDC_ENABLEIGNORE:
				EnableWindow(GetDlgItem( hwndDlg, IDC_IGNORECHANNEL), IsDlgButtonChecked( hwndDlg, IDC_ENABLEIGNORE) == BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_LIST), IsDlgButtonChecked( hwndDlg, IDC_ENABLEIGNORE) == BST_CHECKED);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), IsDlgButtonChecked( hwndDlg, IDC_ENABLEIGNORE) == BST_CHECKED);
				break;

			case IDC_IGNORECHAT:
				EnableWindow(GetDlgItem( hwndDlg, IDC_IGNOREUNKNOWN), IsDlgButtonChecked( hwndDlg, IDC_IGNORECHAT) == BST_UNCHECKED);
				break;

			case IDC_ADD:
				{
					HWND hWnd = CreateDialogParam(g_hInstance,MAKEINTRESOURCE(IDD_ADDIGNORE), hwndDlg,AddIgnoreWndProc, 0);
					SetWindowText( hWnd, TranslateT( "Add Ignore" ));
					EnableWindow(GetDlgItem(( hwndDlg), IDC_ADD), false);
					EnableWindow(GetDlgItem(( hwndDlg), IDC_EDIT), false);
					EnableWindow(GetDlgItem(( hwndDlg), IDC_DELETE), false);
					break;
				}

			case IDC_EDIT:
				if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDC_ADD ))) {
					TCHAR szMask[512];
					TCHAR szFlags[512];
					TCHAR szNetwork[512];
					int i = ListView_GetSelectionMark(GetDlgItem( hwndDlg, IDC_LIST));
					ListView_GetItemText(GetDlgItem( hwndDlg, IDC_LIST), i, 0, szMask, 511); 
					ListView_GetItemText(GetDlgItem( hwndDlg, IDC_LIST), i, 1, szFlags, 511); 
					ListView_GetItemText(GetDlgItem( hwndDlg, IDC_LIST), i, 2, szNetwork, 511); 
					HWND hWnd = CreateDialogParam(g_hInstance,MAKEINTRESOURCE(IDD_ADDIGNORE), hwndDlg,AddIgnoreWndProc, (LPARAM)&szMask);
					SetWindowText(hWnd, TranslateT("Edit Ignore"));
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
					EnableWindow(GetDlgItem(( hwndDlg), IDC_ADD), false);
					EnableWindow(GetDlgItem(( hwndDlg), IDC_EDIT), false);
					EnableWindow(GetDlgItem(( hwndDlg), IDC_DELETE), false);
				}
				break;

			case IDC_DELETE:
				if ( IsWindowEnabled(GetDlgItem( hwndDlg, IDC_DELETE))) {
					TCHAR szMask[512];
					int i = ListView_GetSelectionMark(GetDlgItem( hwndDlg, IDC_LIST ));
					ListView_GetItemText( GetDlgItem( hwndDlg, IDC_LIST), i, 0, szMask, SIZEOF(szMask));
					RemoveIgnore( szMask );
				}
				break;
		}	}
		break;

	case WM_NOTIFY :
		switch(((LPNMHDR)lParam)->idFrom) {
		case IDC_LIST:
			switch (((NMHDR*)lParam)->code) {
			case NM_CLICK:
			case NM_RCLICK:
				if ( ListView_GetSelectionMark(GetDlgItem( hwndDlg, IDC_LIST)) != -1 )
					SendMessage( hwndDlg, IRC_FIXIGNOREBUTTONS, 0, 0);
				break;
				
			case NM_DBLCLK:
				SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
				break;

			case LVN_COLUMNCLICK :
				{
					LPNMLISTVIEW lv;
					lv = (LPNMLISTVIEW)lParam;
					SendMessage(GetDlgItem( hwndDlg, IDC_LIST), LVM_SORTITEMS, (WPARAM)lv->iSubItem, (LPARAM)IgnoreListSort);
					SendMessage( hwndDlg, IRC_UPDATEIGNORELIST, 0, 0);
				}
				break;
			}

		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				prefs->DCCFileEnabled = IsDlgButtonChecked( hwndDlg,IDC_IGNOREFILE)== BST_UNCHECKED?1:0;
				prefs->DCCChatEnabled = IsDlgButtonChecked( hwndDlg,IDC_IGNORECHAT)== BST_UNCHECKED?1:0;
				prefs->Ignore = IsDlgButtonChecked( hwndDlg,IDC_ENABLEIGNORE)== BST_CHECKED?1:0;
				prefs->IgnoreChannelDefault = IsDlgButtonChecked( hwndDlg,IDC_IGNORECHANNEL)== BST_CHECKED?1:0;
				prefs->DCCChatIgnore = IsDlgButtonChecked( hwndDlg, IDC_IGNOREUNKNOWN) == BST_CHECKED?2:1;
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"EnableCtcpFile",prefs->DCCFileEnabled);
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"EnableCtcpChat",prefs->DCCChatEnabled);
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"Ignore",prefs->Ignore);
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"IgnoreChannelDefault",prefs->IgnoreChannelDefault);
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"CtcpChatIgnore",prefs->DCCChatIgnore);
			}
         return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			g_ignoreItems.clear();

			HWND hListView = GetDlgItem( hwndDlg, IDC_LIST );
			int i = ListView_GetItemCount(GetDlgItem( hwndDlg, IDC_LIST));
			for ( int j = 0;  j < i; j++ ) {
				TCHAR szMask[512], szFlags[40], szNetwork[100];
				ListView_GetItemText( hListView, j, 0, szMask, SIZEOF(szMask));
				ListView_GetItemText( hListView, j, 1, szFlags, SIZEOF(szFlags));
				ListView_GetItemText( hListView, j, 2, szNetwork, SIZEOF(szNetwork));
				g_ignoreItems.push_back( CIrcIgnoreItem( szMask, szFlags, szNetwork ));
			}
         
			RewriteIgnoreSettings();
			SetWindowLong(GetDlgItem( hwndDlg,IDC_LIST),GWL_WNDPROC,(LONG)OldListViewProc);
			IgnoreWndHwnd = NULL;
		}
		break;
	}
	return false;
}

int InitOptionsPages(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize = sizeof(odp);
	odp.hInstance = g_hInstance;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_CONNECT);
	odp.pszTitle = ALTIRCPROTONAME;
	odp.pszGroup = LPGEN("Network");
	odp.pszTab = LPGEN("Account");
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = ConnectPrefsProc;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_CTCP);
	odp.pszTab = LPGEN("DCC'n CTCP");
	odp.pfnDlgProc = CtcpPrefsProc;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_OTHER);
	odp.pszTab = LPGEN("Advanced");
	odp.pfnDlgProc = OtherPrefsProc;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_IGNORE);
	odp.pszTab = LPGEN("Ignore");
	odp.pfnDlgProc = IgnorePrefsProc;
	CallService( MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

void UnInitOptions(void)
{
	mir_free( prefs->Alias );
	delete prefs;
	delete []pszServerFile;
}
