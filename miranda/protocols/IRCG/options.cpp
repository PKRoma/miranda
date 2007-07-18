/*
IRC plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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
#include <uxtheme.h>
#include <win2k.h>

HANDLE					OptionsInitHook = NULL;	
extern UINT_PTR			KeepAliveTimer;	
UINT_PTR				OnlineNotifTimer = 0;	
UINT_PTR				OnlineNotifTimer3 = 0;	


extern PREFERENCES *	prefs;
extern char *			IRCPROTONAME;
extern char *			ALTIRCPROTONAME;
extern char *			pszServerFile;
extern char *			pszIgnoreFile;
extern char *			pszPerformFile;
extern char				mirandapath[MAX_PATH];
HWND					connect_hWnd = NULL;
HWND					addserver_hWnd = NULL;
extern HWND				IgnoreWndHwnd;
bool					ServerlistModified = false;
bool					PerformlistModified = false;
extern bool				bMbotInstalled;
extern bool				bTempDisableCheck ;
extern bool				bTempForceCheck ;
extern int				iTempCheckTime ;
extern HMODULE			m_ssleay32;
extern HANDLE			hMenuServer;
static WNDPROC			OldProc;
static WNDPROC			OldListViewProc;

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

static int GetSetting(const char *szSetting, DBVARIANT *dbv)
{
	int rv = !DBGetContactSettingTString(NULL, IRCPROTONAME, szSetting, dbv );
	return rv;
}

static void removeSpaces( TCHAR* p )
{
	while ( *p ) {
		if ( *p == ' ' )
			memmove( p, p+1, sizeof(TCHAR)*lstrlen(p));
		p++;
}	}

void InitPrefs(void)
{
	DBVARIANT dbv;

	prefs = new PREFERENCES;
	GetPrefsString("ServerName", prefs->ServerName, 101, "");
	GetPrefsString("PortStart", prefs->PortStart, 6, "");
	GetPrefsString("PortEnd", prefs->PortEnd, 6, "");
	GetPrefsString("Password", prefs->Password, 499, "");
	CallService(MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)prefs->Password);
	if (!GetPrefsString("PNick", prefs->Nick, 30, _T(""))) {
		GetPrefsString("Nick", prefs->Nick, 30, _T(""));
		if ( lstrlen(prefs->Nick) > 0)
			DBWriteContactSettingTString(NULL, IRCPROTONAME, "PNick", prefs->Nick);
	}
	GetPrefsString("AlernativeNick", prefs->AlternativeNick, 31, _T(""));
	GetPrefsString("Name", prefs->Name, 199, _T(""));
	GetPrefsString("UserID", prefs->UserID, 199, _T("Miranda"));
	GetPrefsString("IdentSystem", prefs->IdentSystem, 10, _T("UNIX"));
	GetPrefsString("IdentPort", prefs->IdentPort, 6, _T("113"));
	GetPrefsString("RetryWait", prefs->RetryWait, 4, _T("30"));
	GetPrefsString("RetryCount", prefs->RetryCount, 4, _T("10"));
	GetPrefsString("Network", prefs->Network, 31, "");
	GetPrefsString("QuitMessage", prefs->QuitMessage, 399, _T(STR_QUITMESSAGE));
	GetPrefsString("UserInfo", prefs->UserInfo, 499, _T(STR_USERINFO));
	GetPrefsString("SpecHost", prefs->MySpecifiedHost, 499, "");
	GetPrefsString("MyLocalHost", prefs->MyLocalHost, 49, "");

	lstrcpyA(prefs->MySpecifiedHostIP, "");

	if ( GetSetting("Alias", &dbv )) {
		prefs->Alias = mir_strdup( dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else prefs->Alias = mir_strdup( "/op /mode ## +ooo $1 $2 $3\r\n/dop /mode ## -ooo $1 $2 $3\r\n/voice /mode ## +vvv $1 $2 $3\r\n/dvoice /mode ## -vvv $1 $2 $3\r\n/j /join #$1 $2-\r\n/p /part ## $1-\r\n/w /whois $1\r\n/k /kick ## $1 $2-\r\n/q /query $1\r\n/logon /log on ##\r\n/logoff /log off ##\r\n/save /log buffer $1\r\n/slap /me slaps $1 around a bit with a large trout" );

	prefs->ScriptingEnabled = DBGetContactSettingByte(NULL,IRCPROTONAME, "ScriptingEnabled", 0);
	prefs->ForceVisible = DBGetContactSettingByte(NULL,IRCPROTONAME, "ForceVisible", 0);
	prefs->DisableErrorPopups = DBGetContactSettingByte(NULL,IRCPROTONAME, "DisableErrorPopups", 0);
	prefs->RejoinChannels = DBGetContactSettingByte(NULL,IRCPROTONAME, "RejoinChannels", 0);
	prefs->RejoinIfKicked = DBGetContactSettingByte(NULL,IRCPROTONAME, "RejoinIfKicked", 1);
	prefs->Ident = DBGetContactSettingByte(NULL,IRCPROTONAME, "Ident", 0);
	prefs->IdentTimer = (int)DBGetContactSettingByte(NULL,IRCPROTONAME, "IdentTimer", 0);
	prefs->Retry = DBGetContactSettingByte(NULL,IRCPROTONAME, "Retry", 0);
	prefs->DisableDefaultServer = DBGetContactSettingByte(NULL,IRCPROTONAME, "DisableDefaultServer", 0);
	prefs->HideServerWindow = DBGetContactSettingByte(NULL,IRCPROTONAME, "HideServerWindow", 1);
	prefs->UseServer = DBGetContactSettingByte(NULL,IRCPROTONAME, "UseServer", 1);
	prefs->JoinOnInvite = DBGetContactSettingByte(NULL,IRCPROTONAME, "JoinOnInvite", 0);
	prefs->Perform = DBGetContactSettingByte(NULL,IRCPROTONAME, "Perform", 0);
	prefs->ShowAddresses = DBGetContactSettingByte(NULL,IRCPROTONAME, "ShowAddresses", 0);
	prefs->AutoOnlineNotification = DBGetContactSettingByte(NULL,IRCPROTONAME, "AutoOnlineNotification", 1);
	prefs->Ignore = DBGetContactSettingByte(NULL,IRCPROTONAME, "Ignore", 0);;
	prefs->IgnoreChannelDefault = DBGetContactSettingByte(NULL,IRCPROTONAME, "IgnoreChannelDefault", 0);;
	prefs->ServerComboSelection = DBGetContactSettingDword(NULL,IRCPROTONAME, "ServerComboSelection", -1);
	prefs->QuickComboSelection = DBGetContactSettingDword(NULL,IRCPROTONAME, "QuickComboSelection", prefs->ServerComboSelection);
	prefs->SendKeepAlive = (int)DBGetContactSettingByte(NULL,IRCPROTONAME, "SendKeepAlive", 0);
	prefs->ListSize.y = DBGetContactSettingDword(NULL,IRCPROTONAME, "SizeOfListBottom", 400);
	prefs->ListSize.x = DBGetContactSettingDword(NULL,IRCPROTONAME, "SizeOfListRight", 600);
	prefs->iSSL = DBGetContactSettingByte(NULL,IRCPROTONAME, "UseSSL", 0);
	prefs->DCCFileEnabled = DBGetContactSettingByte(NULL,IRCPROTONAME, "EnableCtcpFile", 1);
	prefs->DCCChatEnabled = DBGetContactSettingByte(NULL,IRCPROTONAME, "EnableCtcpChat", 1);
	prefs->DCCChatAccept = DBGetContactSettingByte(NULL,IRCPROTONAME, "CtcpChatAccept", 1);
	prefs->DCCChatIgnore = DBGetContactSettingByte(NULL,IRCPROTONAME, "CtcpChatIgnore", 1);
	prefs->DCCPassive = DBGetContactSettingByte(NULL,IRCPROTONAME, "DccPassive", 0);
	prefs->ManualHost = DBGetContactSettingByte(NULL,IRCPROTONAME, "ManualHost", 0);
	prefs->IPFromServer = DBGetContactSettingByte(NULL,IRCPROTONAME, "IPFromServer", 0);
	prefs->DisconnectDCCChats = DBGetContactSettingByte(NULL,IRCPROTONAME, "DisconnectDCCChats", 1);
	prefs->OldStyleModes = DBGetContactSettingByte(NULL,IRCPROTONAME, "OldStyleModes", 0);
	prefs->SendNotice = DBGetContactSettingByte(NULL,IRCPROTONAME, "SendNotice", 1);
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
	prefs->ChannelAwayNotification = DBGetContactSettingByte(NULL,IRCPROTONAME, "ChannelAwayNotification", 1);

	//	DBFreeVariant(&dbv);
	return;
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
			return ( HICON )CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)iconList[i].hIconLibItem );

	return NULL;
}

HANDLE GetIconHandle( int iconId )
{
	for ( int i=0; i < SIZEOF(iconList); i++ )
		if ( iconList[i].defIconID == iconId )
			return iconList[i].hIconLibItem;

	return NULL;
}

// Callback for the 'Add server' dialog
BOOL CALLBACK AddServerProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index = 0; index <i; index++) {
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
				if (SendMessage(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
					SendMessageA(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
			}

			if (m_ssleay32)
				CheckDlgButton(hwndDlg, IDC_OFF, BST_CHECKED);
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_ON), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_OFF), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO), FALSE);
			}

			SetWindowTextA(GetDlgItem(hwndDlg, IDC_ADD_PORT), "6667");
			SetWindowTextA(GetDlgItem(hwndDlg, IDC_ADD_PORT2), "6667");
			SetFocus(GetDlgItem(hwndDlg, IDC_ADD_COMBO));	
		}
		return TRUE;

	case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED) {
				switch (LOWORD(wParam)) {	
				case IDN_ADD_OK:
					if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))) {
						SERVER_INFO * pData = new SERVER_INFO;
						pData->iSSL = 0;
						if(IsDlgButtonChecked(hwndDlg, IDC_ON))
							pData->iSSL = 2;
						if(IsDlgButtonChecked(hwndDlg, IDC_AUTO))
							pData->iSSL = 1;
						pData->Address=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1];
						GetDlgItemTextA(hwndDlg,IDC_ADD_ADDRESS, pData->Address, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1);
						pData->Group=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1];
						GetDlgItemTextA(hwndDlg,IDC_ADD_COMBO, pData->Group, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1);
						pData->Name=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+3];
						lstrcpyA(pData->Name, pData->Group);
						lstrcatA(pData->Name, ": ");
						char temp[255];
						GetDlgItemTextA(hwndDlg,IDC_ADD_SERVER, temp, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+1);
						lstrcatA(pData->Name, temp);			
						pData->PortEnd=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1];
						GetDlgItemTextA(hwndDlg,IDC_ADD_PORT2, pData->PortEnd, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1);
						pData->PortStart=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1];
						GetDlgItemTextA(hwndDlg,IDC_ADD_PORT, pData->PortStart, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1);
						int iItem = SendMessageA(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Name);
						SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
						SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
						SendMessage(connect_hWnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);						
						PostMessage (hwndDlg, WM_CLOSE, 0,0);
						if ( SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR) {
							int m = SendMessageA(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
							SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
						}
						ServerlistModified = true;
					}
					else MessageBoxA(hwndDlg, Translate(	"Please complete all fields"	), Translate(	"IRC error"	), MB_OK|MB_ICONERROR);
					break;

				case IDN_ADD_CANCEL:
					PostMessage (hwndDlg, WM_CLOSE, 0,0);
					break;
		}	}	}
		break;

	case WM_CLOSE:
		EnableWindow(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_ADDSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_EDITSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_DELETESERVER), true);
		DestroyWindow(hwndDlg);
		return true;

	case WM_DESTROY:
		return true;
	}

	return false;
}

// Callback for the 'Edit server' dialog
BOOL CALLBACK EditServerProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index = 0; index <i; index++) {
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
				if (SendMessage(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
					SendMessageA(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
			}
			int j = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, j, 0);
			SetDlgItemTextA(hwndDlg, IDC_ADD_ADDRESS, pData->Address);
			SetDlgItemTextA(hwndDlg, IDC_ADD_COMBO, pData->Group);
			SetDlgItemTextA(hwndDlg, IDC_ADD_PORT, pData->PortStart);
			SetDlgItemTextA(hwndDlg, IDC_ADD_PORT2, pData->PortEnd);
			char tempchar[255];
			strcpy (tempchar, pData->Name);
			int temp = strchr(tempchar, ' ') -tempchar;
			for (int index2 = temp+1; index2 < lstrlenA(tempchar)+1;index2++)
				tempchar[index2-temp-1] = tempchar[index2];

			if(m_ssleay32) {
				if(pData->iSSL == 0)
					CheckDlgButton(hwndDlg, IDC_OFF, BST_CHECKED);
				if(pData->iSSL == 1)
					CheckDlgButton(hwndDlg, IDC_AUTO, BST_CHECKED);
				if(pData->iSSL == 2)
					CheckDlgButton(hwndDlg, IDC_ON, BST_CHECKED);
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_ON), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_OFF), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO), FALSE);
			}

			SetDlgItemTextA(hwndDlg, IDC_ADD_SERVER, tempchar);
			SetFocus(GetDlgItem(hwndDlg, IDC_ADD_COMBO));	
		}
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {	
			case IDN_ADD_OK:
				if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))) {
					int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);

					SERVER_INFO * pData1 = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
					delete []pData1->Name;
					delete []pData1->Address;
					delete []pData1->PortStart;
					delete []pData1->PortEnd;
					delete []pData1->Group;
					delete pData1;	
					SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);

					SERVER_INFO * pData = new SERVER_INFO;
					pData->iSSL = 0;
					if(IsDlgButtonChecked(hwndDlg, IDC_ON))
						pData->iSSL = 2;
					if(IsDlgButtonChecked(hwndDlg, IDC_AUTO))
						pData->iSSL = 1;
					pData->Address=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1];
					GetDlgItemTextA(hwndDlg,IDC_ADD_ADDRESS, pData->Address, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1);
					pData->Group=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1];
					GetDlgItemTextA(hwndDlg,IDC_ADD_COMBO, pData->Group, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1);
					pData->Name=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+3];
					lstrcpyA(pData->Name, pData->Group);
					lstrcatA(pData->Name, ": ");
					char temp[255];
					GetDlgItemTextA(hwndDlg,IDC_ADD_SERVER, temp, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+1);
					lstrcatA(pData->Name, temp);			
					pData->PortEnd=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1];
					GetDlgItemTextA(hwndDlg,IDC_ADD_PORT2, pData->PortEnd, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1);
					pData->PortStart=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1];
					GetDlgItemTextA(hwndDlg,IDC_ADD_PORT, pData->PortStart, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1);
					int iItem = SendMessageA(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Name);
					SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
					SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
					SendMessage(connect_hWnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
					PostMessage (hwndDlg, WM_CLOSE, 0,0);
					if ( SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR) {
						int m = SendMessageA(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
						SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
					}
					ServerlistModified = true;
				}
				else MessageBoxA(hwndDlg, Translate(	"Please complete all fields"	), Translate(	"IRC error"	), MB_OK|MB_ICONERROR);
				break;

			case IDN_ADD_CANCEL:
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
				break;
			}
		break;

	case WM_CLOSE:
		EnableWindow(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_ADDSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_EDITSERVER), true);
		EnableWindow(GetDlgItem(connect_hWnd, IDC_DELETESERVER), true);
		DestroyWindow(hwndDlg);
		return true;

	case WM_DESTROY:
		return true;
	}

	return false;
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
			SendMessage(hwndDlg, EM_REPLACESEL, false, (LPARAM) w);
			SendMessage(hwndDlg, EM_SCROLLCARET, 0,0);
			return 0;
		}
		break;
	} 

	return CallWindowProc(OldProc, hwndDlg, msg, wParam, lParam); 
} 

// Callback for the 'CTCP preferences' dialog
BOOL CALLBACK CtcpPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetDlgItemText(hwndDlg,IDC_USERINFO,prefs->UserInfo);

			CheckDlgButton(hwndDlg, IDC_SLOW, DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0)==0?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_FAST, DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0)==1?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_DISC, prefs->DisconnectDCCChats?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_PASSIVE, prefs->DCCPassive?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SENDNOTICE, prefs->SendNotice?BST_CHECKED:BST_UNCHECKED);

			SendDlgItemMessageA(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "256");
			SendDlgItemMessageA(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "512");
			SendDlgItemMessageA(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "1024");
			SendDlgItemMessageA(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "2048");
			SendDlgItemMessageA(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "4096");
			SendDlgItemMessageA(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "8192");

			int j = DBGetContactSettingWord(NULL, IRCPROTONAME, "DCCPacketSize", 1024*4);
			char szTemp[10];
			mir_snprintf(szTemp, sizeof(szTemp), "%u", j);
			int i = SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) szTemp);
			if ( i == CB_ERR)
				int i = SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) "4096");

			if(prefs->DCCChatAccept == 1)
				CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
			if(prefs->DCCChatAccept == 2)
				CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
			if(prefs->DCCChatAccept == 3)
				CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);

			CheckDlgButton(hwndDlg, IDC_FROMSERVER, prefs->IPFromServer?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ENABLEIP, prefs->ManualHost?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_IP), prefs->ManualHost);
			EnableWindow(GetDlgItem(hwndDlg, IDC_FROMSERVER), !prefs->ManualHost);
			if (prefs->ManualHost)
				SetDlgItemTextA(hwndDlg,IDC_IP,prefs->MySpecifiedHost);
			else {
				if ( prefs->IPFromServer ) {
					if ( lstrlenA(prefs->MyHost )) {
						String s = (String)Translate("<Resolved IP: ") + (String)prefs->MyHost+ (String)">";
						SetDlgItemTextA(hwndDlg,IDC_IP,s.c_str());
					}
					else SetDlgItemTextA(hwndDlg,IDC_IP,Translate("<Automatic>"));
				}
				else {
					if ( lstrlenA(prefs->MyLocalHost)) {
						String s = (String)Translate("<Local IP: ") + (String)prefs->MyLocalHost+ (String)">";
						SetDlgItemTextA(hwndDlg,IDC_IP,s.c_str());
					}
					else SetDlgItemTextA(hwndDlg,IDC_IP,Translate("<Automatic>"));
		}	}	}
		return TRUE;

	case WM_COMMAND:
		if((	LOWORD(wParam) == IDC_USERINFO	
			|| LOWORD(wParam) == IDC_IP
			|| LOWORD(wParam) == IDC_COMBO && HIWORD(wParam) != CBN_SELCHANGE)
			&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;

		SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);

		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {	
			case IDC_FROMSERVER:
			case IDC_ENABLEIP:
				EnableWindow(GetDlgItem(hwndDlg, IDC_IP), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FROMSERVER), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP)== BST_UNCHECKED);

				if ( IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP)== BST_CHECKED )
					SetDlgItemTextA(hwndDlg,IDC_IP,prefs->MySpecifiedHost);
				else {
					if ( IsDlgButtonChecked(hwndDlg, IDC_FROMSERVER)== BST_CHECKED ) {
						if ( lstrlenA(prefs->MyHost)) {
							String s = (String)Translate("<Resolved IP: ") + (String)prefs->MyHost+ (String)">";
							SetDlgItemTextA(hwndDlg,IDC_IP,s.c_str());
						}
						else SetDlgItemTextA(hwndDlg,IDC_IP,Translate("<Automatic>"));
					}
					else {
						if(lstrlenA(prefs->MyLocalHost)) {
							String s = (String)Translate("<Local IP: ") + (String)prefs->MyLocalHost+ (String)">";
							SetDlgItemTextA(hwndDlg,IDC_IP,s.c_str());
						}
						else SetDlgItemTextA(hwndDlg,IDC_IP,Translate("<Automatic>"));
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
					GetDlgItemText(hwndDlg,IDC_USERINFO,prefs->UserInfo, 499);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"UserInfo",prefs->UserInfo);

					char szTemp[10];
					GetWindowTextA(GetDlgItem(hwndDlg, IDC_COMBO), szTemp, 10);
					DBWriteContactSettingWord(NULL,IRCPROTONAME,"DCCPacketSize", (WORD)atoi(szTemp));

					prefs->DCCPassive = IsDlgButtonChecked(hwndDlg,IDC_PASSIVE)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DccPassive",prefs->DCCPassive);

					prefs->SendNotice = IsDlgButtonChecked(hwndDlg,IDC_SENDNOTICE)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"SendNotice",prefs->SendNotice);

					if(IsDlgButtonChecked(hwndDlg,IDC_SLOW)== BST_CHECKED)
						DBWriteContactSettingByte(NULL,IRCPROTONAME,"DCCMode",0);
					else 
						DBWriteContactSettingByte(NULL,IRCPROTONAME,"DCCMode",1);

					prefs->ManualHost = IsDlgButtonChecked(hwndDlg,IDC_ENABLEIP)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ManualHost",prefs->ManualHost);

					prefs->IPFromServer = IsDlgButtonChecked(hwndDlg,IDC_FROMSERVER)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"IPFromServer",prefs->IPFromServer);

					prefs->DisconnectDCCChats = IsDlgButtonChecked(hwndDlg,IDC_DISC)== BST_CHECKED?1:0;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisconnectDCCChats",prefs->DisconnectDCCChats);

					if(IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP) == BST_CHECKED) {
						char szTemp[500];
						GetDlgItemTextA(hwndDlg,IDC_IP,szTemp, 499);
						lstrcpynA(prefs->MySpecifiedHost, GetWord(szTemp, 0).c_str(), 499);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"SpecHost",prefs->MySpecifiedHost);
						if ( lstrlenA( prefs->MySpecifiedHost )) {
							IPRESOLVE * ipr = new IPRESOLVE;
							ipr->iType = IP_MANUAL;
							ipr->pszAdr = prefs->MySpecifiedHost;
							mir_forkthread(ResolveIPThread, ipr);
					}	}

					if(IsDlgButtonChecked(hwndDlg, IDC_RADIO1) == BST_CHECKED)
						prefs->DCCChatAccept = 1;
					if(IsDlgButtonChecked(hwndDlg, IDC_RADIO2) == BST_CHECKED)
						prefs->DCCChatAccept = 2;
					if(IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED)
						prefs->DCCChatAccept = 3;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"CtcpChatAccept",prefs->DCCChatAccept);
		}	}	}
		break;
	}

	return false;
}

// Callback for the 'Advanced preferences' dialog
BOOL CALLBACK OtherPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			OldProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_ALIASEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 
			SetWindowLong(GetDlgItem(hwndDlg, IDC_QUITMESSAGE), GWL_WNDPROC,(LONG)EditSubclassProc); 
			SetWindowLong(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 

			SendDlgItemMessage(hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_ADD));
			SendDlgItemMessage(hwndDlg,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
			SendMessage(GetDlgItem(hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)Translate("Click to set commands that will be performed for this event"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)Translate("Click to delete the commands for this event"), 0);

			SetDlgItemTextA(hwndDlg,IDC_ALIASEDIT,prefs->Alias);
			SetDlgItemText(hwndDlg,IDC_QUITMESSAGE,prefs->QuitMessage);
			CheckDlgButton(hwndDlg,IDC_PERFORM, ((prefs->Perform) ? (BST_CHECKED) : (BST_UNCHECKED)));
			CheckDlgButton(hwndDlg,IDC_SCRIPT, ((prefs->ScriptingEnabled) ? (BST_CHECKED) : (BST_UNCHECKED)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SCRIPT), bMbotInstalled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), prefs->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), prefs->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), prefs->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), prefs->Perform);
			if ( pszServerFile ) {
				char * p1 = pszServerFile;
				char * p2 = pszServerFile;

				while(strchr(p2, 'n')) {
					p1 = strstr(p2, "GROUP:");
					p1 =p1+ 6;
					p2 = strchr(p1, '\r');
					if (!p2)
						p2 = strchr(p1, '\n');
					if (!p2)
						p2 = strchr(p1, '\0');

					char * Group = new char[p2-p1+1];
					lstrcpynA(Group, p1, p2-p1+1);
					int i = SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Group);
					if (i ==CB_ERR)
						int index = SendMessageA(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_ADDSTRING, 0, (LPARAM) Group);

					delete []Group;
			}	}

			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 0, (LPARAM)"Event: Available"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 1, (LPARAM)"Event: Away"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 2, (LPARAM)"Event: N/A"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 3, (LPARAM)"Event: Occupied"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 4, (LPARAM)"Event: DND"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 5, (LPARAM)"Event: Free for chat"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 6, (LPARAM)"Event: On the phone"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 7, (LPARAM)"Event: Out for lunch"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 8, (LPARAM)"Event: Disconnect"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 9, (LPARAM)"ALL NETWORKS"	);
			SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_SETITEMDATA, -1, 0);

			if ( pszPerformFile ) {
				char* p1 = pszPerformFile;
				char* p2 = pszPerformFile;

				while(strstr(p2, "NETWORK: ")) {
					p1 = strstr(p2, "NETWORK: ");
					p1 = p1+9;
					p2 = strchr(p1, '\n');
					char * szNetwork = new char[p2-p1];
					lstrcpynA(szNetwork, p1, p2-p1);
					p1 = p2;
					p1++;
					p2 = strstr(p1, "\nNETWORK: ");
					if ( !p2 )
						p2= p1 + lstrlenA(p1)-1;
					if ( p1 == p2 )
						break;

					int index = SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_FINDSTRINGEXACT, -1, (LPARAM) szNetwork);
					if (index != CB_ERR) {
						*p2 = 0;
						PERFORM_INFO * pPref = new PERFORM_INFO;
						pPref->Perform = mir_strdup( p1 );
						SendDlgItemMessageA(hwndDlg, IDC_PERFORMCOMBO, CB_SETITEMDATA, index, (LPARAM)pPref);
					}

					delete [] szNetwork;
			}	}

			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_SETCURSEL, 0, 0);				
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_PERFORMCOMBO, CBN_SELCHANGE), 0);
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
				CallService(MS_UTILS_OPENURL,0,(LPARAM) "http://members.chello.se/matrix/index.html" );

		if ( HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PERFORMCOMBO) {
			int i = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
			PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
			if (pPerf == 0)
				SetDlgItemTextA(hwndDlg, IDC_PERFORMEDIT, "");
			else
				SetDlgItemTextA(hwndDlg, IDC_PERFORMEDIT, pPerf->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);
			if ( GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PERFORMEDIT)) != 0)
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), true);
			else
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
			return false;
		}
		SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);

		if ( HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_PERFORMEDIT) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), true);

			if ( GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PERFORMEDIT)) != 0)
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), true);
			else
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
		}

		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {	
			case IDC_PERFORM:
				EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
				break;

			case IDC_ADD:
				{
					int j = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PERFORMEDIT));
					char * temp = new char[j+1];
					GetWindowTextA(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), temp, j+1);

					if ( my_strstri(temp, "/away"))
						MessageBoxA(NULL, Translate("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), Translate("IRC Error"), MB_OK);
					else {
						int i = (int) SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
						if ( i != CB_ERR ) {
							PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
							if (pPerf != 0) {
								mir_free( pPerf->Perform );
								delete pPerf;
							}
							pPerf = new PERFORM_INFO;
							pPerf->Perform = mir_strdup( temp );
							SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_SETITEMDATA, i, (LPARAM) pPerf);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);

							PerformlistModified = true;
						}
					}
					delete []temp;
				}
				break;

			case IDC_DELETE:
				{
					int i = (int) SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
					if ( i == CB_ERR )
						break;

					PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
					if (pPerf != 0) {
						delete []pPerf->Perform;
						delete pPerf;
					}
					SetDlgItemTextA(hwndDlg, IDC_PERFORMEDIT, "");
					SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_SETITEMDATA, i, 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);

					PerformlistModified = true;
				}
				break;
		}	}
		break;

	case WM_DESTROY:
		{
			PerformlistModified = false;
			int i = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCOUNT, 0, 0);
			if ( i != CB_ERR && i != 0 ) {
				for (int index = 0; index < i; index++) {
					PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, index, 0);
					if ( (const int)pPerf != CB_ERR && pPerf != 0) {
						delete []pPerf->Perform;
						delete pPerf;
		}	}	}	}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				mir_free( prefs->Alias );
				{	int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ALIASEDIT));
					prefs->Alias = ( char* )mir_alloc( len+1 );
               GetDlgItemTextA( hwndDlg, IDC_ALIASEDIT, prefs->Alias, len+1 );
				}
				DBWriteContactSettingString( NULL, IRCPROTONAME, "Alias", prefs->Alias );

				GetDlgItemText(hwndDlg,IDC_QUITMESSAGE,prefs->QuitMessage, 399);
				DBWriteContactSettingTString(NULL,IRCPROTONAME,"QuitMessage",prefs->QuitMessage);

				prefs->Perform = IsDlgButtonChecked(hwndDlg,IDC_PERFORM)== BST_CHECKED;
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"Perform",prefs->Perform);
				prefs->ScriptingEnabled = IsDlgButtonChecked(hwndDlg,IDC_SCRIPT)== BST_CHECKED;
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"ScriptingEnabled",prefs->ScriptingEnabled);
				if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_ADD)))
					SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ADD, BN_CLICKED), 0);

				if (PerformlistModified) {
					PerformlistModified = false;
					char filepath[MAX_PATH];
					mir_snprintf(filepath, sizeof(filepath), "%s\\%s_perform.ini", mirandapath, IRCPROTONAME);
					int i = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCOUNT, 0, 0);
					FILE *hFile = fopen(filepath,"wb");
					if (hFile && i != CB_ERR && i != 0) {
						for (int index = 0; index < i; index++) {
							PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, index, 0);
							if ( (const int)pPerf != CB_ERR && pPerf != 0) {
								int k = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETLBTEXTLEN, index, 0);
								char * temp = new char[k+1];
								SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETLBTEXT, index, (LPARAM)temp);
								fputs("NETWORK: ", hFile);
								fputs(temp, hFile);
								fputs("\r\n", hFile);
								fputs(pPerf->Perform, hFile);
								fputs("\r\n", hFile);
								delete []temp;
						}	}

						fclose(hFile);
						delete [] pszPerformFile;
						pszPerformFile = IrcLoadFile(filepath);
				}	}
				return TRUE;
		}	}
		break;
	}

	return false;
}

// Callback for the 'Connect preferences' dialog
BOOL CALLBACK ConnectPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CHAR:
		SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);
		break;

	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SendDlgItemMessage(hwndDlg,IDC_ADDSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_ADD));
		SendDlgItemMessage(hwndDlg,IDC_DELETESERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
		SendDlgItemMessage(hwndDlg,IDC_EDITSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_RENAME));
		SendMessage(GetDlgItem(hwndDlg,IDC_ADDSERVER), BUTTONADDTOOLTIP, (WPARAM)Translate("Add a new network"), 0);
		SendMessage(GetDlgItem(hwndDlg,IDC_EDITSERVER), BUTTONADDTOOLTIP, (WPARAM)Translate("Edit this network"), 0);
		SendMessage(GetDlgItem(hwndDlg,IDC_DELETESERVER), BUTTONADDTOOLTIP, (WPARAM)Translate("Delete this network"), 0);

		connect_hWnd = hwndDlg;

		//	Fill the servers combo box and create SERVER_INFO structures
		if ( pszServerFile ) {
			char* p1 = pszServerFile;
			char* p2 = pszServerFile;
			while (strchr(p2, 'n')) {
				SERVER_INFO * pData = new SERVER_INFO;
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
				int iItem = SendDlgItemMessageA(hwndDlg, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->Name);
				SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);
		}	}

		SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_SETCURSEL, prefs->ServerComboSelection,0);				
		SetDlgItemTextA(hwndDlg,IDC_SERVER, prefs->ServerName);
		SetDlgItemTextA(hwndDlg,IDC_PORT, prefs->PortStart);
		SetDlgItemTextA(hwndDlg,IDC_PORT2, prefs->PortEnd);
		if ( m_ssleay32 ) {
			if(prefs->iSSL == 0)
				SetDlgItemTextA(hwndDlg,IDC_SSL,Translate("Off") );
			if(prefs->iSSL == 1)
				SetDlgItemTextA(hwndDlg,IDC_SSL,Translate("Auto") );
			if(prefs->iSSL == 2)
				SetDlgItemTextA(hwndDlg,IDC_SSL,Translate("On") );
		}
		else SetDlgItemTextA(hwndDlg,IDC_SSL, Translate("N/A"));

		if ( prefs->ServerComboSelection != -1 ) {
			SERVER_INFO * pData = (SERVER_INFO *)SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_GETITEMDATA, prefs->ServerComboSelection, 0);
			if ((int)pData != CB_ERR) {
				SetDlgItemTextA(hwndDlg,IDC_SERVER,pData->Address);
				SetDlgItemTextA(hwndDlg,IDC_PORT,pData->PortStart);
				SetDlgItemTextA(hwndDlg,IDC_PORT2,pData->PortEnd);
		}	}

		SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(999,20));
		SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETPOS,0,MAKELONG(prefs->OnlineNotificationTime,0));
		SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_SETRANGE,0,MAKELONG(200,0));
		SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_SETPOS,0,MAKELONG(prefs->OnlineNotificationLimit,0));
		SetDlgItemText(hwndDlg,IDC_NICK,prefs->Nick);
		SetDlgItemText(hwndDlg,IDC_NICK2,prefs->AlternativeNick);
		SetDlgItemText(hwndDlg,IDC_USERID,prefs->UserID);
		SetDlgItemText(hwndDlg,IDC_NAME,prefs->Name);
		SetDlgItemTextA(hwndDlg,IDC_PASS,prefs->Password);
		SetDlgItemText(hwndDlg,IDC_IDENTSYSTEM,prefs->IdentSystem);
		SetDlgItemText(hwndDlg,IDC_IDENTPORT,prefs->IdentPort);
		SetDlgItemText(hwndDlg,IDC_RETRYWAIT,prefs->RetryWait);
		SetDlgItemText(hwndDlg,IDC_RETRYCOUNT,prefs->RetryCount);			
		CheckDlgButton(hwndDlg,IDC_ADDRESS, ((prefs->ShowAddresses) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_OLDSTYLE, ((prefs->OldStyleModes) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_CHANNELAWAY, ((prefs->ChannelAwayNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_ONLINENOTIF, ((prefs->AutoOnlineNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ONLINETIMER), prefs->AutoOnlineNotification);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHANNELAWAY), prefs->AutoOnlineNotification);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN1), prefs->AutoOnlineNotification);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN2), prefs->AutoOnlineNotification && prefs->ChannelAwayNotification);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), prefs->AutoOnlineNotification && prefs->ChannelAwayNotification);
		CheckDlgButton(hwndDlg,IDC_IDENT, ((prefs->Ident) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTSYSTEM), prefs->Ident);
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTPORT), prefs->Ident);
		CheckDlgButton(hwndDlg,IDC_DISABLEERROR, ((prefs->DisableErrorPopups) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_FORCEVISIBLE, ((prefs->ForceVisible) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_REJOINCHANNELS, ((prefs->RejoinChannels) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_REJOINONKICK, ((prefs->RejoinIfKicked) ? (BST_CHECKED) : (BST_UNCHECKED)));
		CheckDlgButton(hwndDlg,IDC_RETRY, ((prefs->Retry) ? (BST_CHECKED) : (BST_UNCHECKED)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYWAIT), prefs->Retry);
		EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYCOUNT), prefs->Retry);
		CheckDlgButton(hwndDlg,IDC_STARTUP, ((!prefs->DisableDefaultServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton(hwndDlg,IDC_KEEPALIVE, ((prefs->SendKeepAlive) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton(hwndDlg,IDC_IDENT_TIMED, ((prefs->IdentTimer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton(hwndDlg,IDC_USESERVER, ((prefs->UseServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton(hwndDlg,IDC_SHOWSERVER, ((!prefs->HideServerWindow) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		CheckDlgButton(hwndDlg,IDC_AUTOJOIN, ((prefs->JoinOnInvite) ? (BST_CHECKED) : (BST_UNCHECKED)));				
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSERVER), prefs->UseServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SERVER), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PORT2), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PASS), !prefs->DisableDefaultServer);
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDENT_TIMED), prefs->Ident);
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
			int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			if (pData && (int)pData != CB_ERR ) {
				SetDlgItemTextA(hwndDlg,IDC_SERVER,pData->Address);
				SetDlgItemTextA(hwndDlg,IDC_PORT,pData->PortStart);
				SetDlgItemTextA(hwndDlg,IDC_PORT2,pData->PortEnd);
				SetDlgItemTextA(hwndDlg,IDC_PASS,"");
				if ( m_ssleay32 ) {
					if(pData->iSSL == 0)
						SetDlgItemTextA(hwndDlg,IDC_SSL,Translate("Off") );
					if(pData->iSSL == 1)
						SetDlgItemTextA(hwndDlg,IDC_SSL,Translate("Auto") );
					if(pData->iSSL == 2)
						SetDlgItemTextA(hwndDlg,IDC_SSL,Translate("On") );
				}
				else SetDlgItemTextA(hwndDlg,IDC_SSL, Translate("N/A"));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);
			}
			return false;
		}
		SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);

		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {
			case IDC_DELETESERVER:
				{
					int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					if (i != CB_ERR) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), false);
						SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
						char * temp = new char [lstrlenA(pData->Name)+24];
						mir_snprintf(temp, sizeof(temp), Translate("Do you want to delete\r\n%s"), pData->Name);
						if (MessageBoxA(hwndDlg, temp, Translate(	"Delete server"	), MB_YESNO|MB_ICONQUESTION) == IDYES) {
							delete []pData->Name;
							delete []pData->Address;
							delete []pData->PortStart;
							delete []pData->PortEnd;
							delete []pData->Group;
							delete pData;	
							SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);
							if (i >= SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0))
								i--;
							SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_SETCURSEL, i, 0);
							SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
							ServerlistModified = true;
						}
						delete []temp;
						EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), true);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), true);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), true);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), true);
				}	}
				break;

			case IDC_ADDSERVER:
				EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), false);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), false);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), false);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), false);
				addserver_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_ADDSERVER),hwndDlg,AddServerProc);
				break;

			case IDC_EDITSERVER:
				{
					int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					if (i != CB_ERR) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), false);
						addserver_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_ADDSERVER),hwndDlg,EditServerProc);
						SetWindowTextA(addserver_hWnd, Translate("Edit server"));
				}	}
				break;

			case IDC_STARTUP:
				EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PORT2), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PASS), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SSL), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
				break;	

			case IDC_IDENT:
				EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTSYSTEM), IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTPORT), IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_IDENT_TIMED), IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED);
				break;

			case IDC_USESERVER:
				EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSERVER), IsDlgButtonChecked(hwndDlg, IDC_USESERVER)== BST_CHECKED);
				break;

			case IDC_ONLINENOTIF:
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHANNELAWAY), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ONLINETIMER), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN1), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN2), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked(hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked(hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				break;

			case IDC_CHANNELAWAY:
				EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN2), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked(hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED && IsDlgButtonChecked(hwndDlg, IDC_CHANNELAWAY)== BST_CHECKED);
				break;

			case IDC_RETRY:
				EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYWAIT), IsDlgButtonChecked(hwndDlg, IDC_RETRY)== BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYCOUNT), IsDlgButtonChecked(hwndDlg, IDC_RETRY)== BST_CHECKED);
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
					if(IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED) {
						GetDlgItemTextA(hwndDlg,IDC_SERVER, prefs->ServerName, 101);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
						GetDlgItemTextA(hwndDlg,IDC_PORT, prefs->PortStart, 6);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
						GetDlgItemTextA(hwndDlg,IDC_PORT2, prefs->PortEnd, 6);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
						GetDlgItemTextA(hwndDlg,IDC_PASS, prefs->Password, 500);
						CallService(MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)prefs->Password);
						DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
						CallService(MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)prefs->Password);
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

					prefs->OnlineNotificationTime = SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_GETPOS,0,0);
					DBWriteContactSettingWord(NULL, IRCPROTONAME, "OnlineNotificationTime", (BYTE)prefs->OnlineNotificationTime);

					prefs->OnlineNotificationLimit = SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_GETPOS,0,0);
					DBWriteContactSettingWord(NULL, IRCPROTONAME, "OnlineNotificationLimit", (BYTE)prefs->OnlineNotificationLimit);

					prefs->ChannelAwayNotification = IsDlgButtonChecked(hwndDlg, IDC_CHANNELAWAY )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ChannelAwayNotification",prefs->ChannelAwayNotification);

					GetDlgItemText(hwndDlg,IDC_NICK, prefs->Nick, 30);
					removeSpaces(prefs->Nick);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"PNick",prefs->Nick);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"Nick",prefs->Nick);
					GetDlgItemText(hwndDlg,IDC_NICK2, prefs->AlternativeNick, 30);
					removeSpaces(prefs->AlternativeNick);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"AlernativeNick",prefs->AlternativeNick);
					GetDlgItemText(hwndDlg,IDC_USERID, prefs->UserID, 199);
					removeSpaces(prefs->UserID);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"UserID",prefs->UserID);
					GetDlgItemText(hwndDlg,IDC_NAME, prefs->Name, 199);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"Name",prefs->Name);
					GetDlgItemText(hwndDlg,IDC_IDENTSYSTEM, prefs->IdentSystem, 10);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"IdentSystem",prefs->IdentSystem);
					GetDlgItemText(hwndDlg,IDC_IDENTPORT, prefs->IdentPort, 6);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"IdentPort",prefs->IdentPort);
					GetDlgItemText(hwndDlg,IDC_RETRYWAIT, prefs->RetryWait, 4);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"RetryWait",prefs->RetryWait);
					GetDlgItemText(hwndDlg,IDC_RETRYCOUNT, prefs->RetryCount, 4);
					DBWriteContactSettingTString(NULL,IRCPROTONAME,"RetryCount",prefs->RetryCount);
					prefs->DisableDefaultServer = !IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisableDefaultServer",prefs->DisableDefaultServer);
					prefs->Ident = IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"Ident",prefs->Ident);
					prefs->IdentTimer = IsDlgButtonChecked(hwndDlg, IDC_IDENT_TIMED)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"IdentTimer",prefs->IdentTimer);
					prefs->ForceVisible = IsDlgButtonChecked(hwndDlg, IDC_FORCEVISIBLE)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ForceVisible",prefs->ForceVisible);
					prefs->DisableErrorPopups = IsDlgButtonChecked(hwndDlg, IDC_DISABLEERROR)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisableErrorPopups",prefs->DisableErrorPopups);
					prefs->RejoinChannels = IsDlgButtonChecked(hwndDlg, IDC_REJOINCHANNELS)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"RejoinChannels",prefs->RejoinChannels);
					prefs->RejoinIfKicked = IsDlgButtonChecked(hwndDlg, IDC_REJOINONKICK)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"RejoinIfKicked",prefs->RejoinIfKicked);
					prefs->Retry = IsDlgButtonChecked(hwndDlg, IDC_RETRY)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"Retry",prefs->Retry);
					prefs->ShowAddresses = IsDlgButtonChecked(hwndDlg, IDC_ADDRESS)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"ShowAddresses",prefs->ShowAddresses);
					prefs->OldStyleModes = IsDlgButtonChecked(hwndDlg, IDC_OLDSTYLE)== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"OldStyleModes",prefs->OldStyleModes);

					prefs->UseServer = IsDlgButtonChecked(hwndDlg, IDC_USESERVER )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseServer",prefs->UseServer);

					CLISTMENUITEM clmi;
					memset( &clmi, 0, sizeof( clmi ));
					clmi.cbSize = sizeof( clmi );
					clmi.flags = CMIM_FLAGS;
					if ( !prefs->UseServer )
						clmi.flags |= CMIF_GRAYED;
					CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuServer, ( LPARAM )&clmi );

					prefs->JoinOnInvite = IsDlgButtonChecked(hwndDlg, IDC_AUTOJOIN )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"JoinOnInvite",prefs->JoinOnInvite);
					prefs->HideServerWindow = IsDlgButtonChecked(hwndDlg, IDC_SHOWSERVER )== BST_UNCHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"HideServerWindow",prefs->HideServerWindow);
					prefs->ServerComboSelection = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					DBWriteContactSettingDword(NULL,IRCPROTONAME,"ServerComboSelection",prefs->ServerComboSelection);
					prefs->SendKeepAlive = IsDlgButtonChecked(hwndDlg, IDC_KEEPALIVE )== BST_CHECKED;
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"SendKeepAlive",prefs->SendKeepAlive);
					if (prefs->SendKeepAlive)
						SetChatTimer(KeepAliveTimer, 60*1000, KeepAliveTimerProc);
					else
						KillChatTimer(KeepAliveTimer);

					prefs->AutoOnlineNotification = IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF )== BST_CHECKED;
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

					int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
					if ( pData && (int)pData != CB_ERR ) {
						if(IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED)
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
							int j = (int) SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
							if (j !=CB_ERR && j !=0) {
								for (int index2 = 0; index2 < j; index2++) {
									SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
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
			int j = (int) SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
			if (j !=CB_ERR && j !=0 ) {
				for (int index2 = 0; index2 < j; index2++) {
					SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
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

static int CALLBACK IgnoreListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if ( !IgnoreWndHwnd )
		return 1;

	char temp1[512];
	char temp2[512];
	LVITEMA lvm;
	lvm.mask = LVIF_TEXT;
	lvm.iItem = lParam1;
	lvm.iSubItem = lParamSort;
	lvm.pszText = temp1;
	lvm.cchTextMax = 511;
	SendMessage(GetDlgItem(IgnoreWndHwnd, IDC_INFO_LISTVIEW), LVM_GETITEM, 0, (LPARAM)&lvm);
	lvm.iItem = lParam2;
	lvm.pszText = temp2;
	SendMessage(GetDlgItem(IgnoreWndHwnd, IDC_INFO_LISTVIEW), LVM_GETITEM, 0, (LPARAM)&lvm);
	if ( lstrlenA(temp1) !=0 && lstrlenA(temp2) != 0 )
		return lstrcmpiA(temp1, temp2);

	if ( lstrlenA(temp1) == 0 )
		return 1;
	else
		return -1;
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

// Callback for the 'Add server' dialog
BOOL CALLBACK AddIgnoreWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static char szOldMask[500];
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			if ( lParam == 0 ) {
				if(g_ircSession)
					SetWindowText(GetDlgItem(hwndDlg, IDC_NETWORK), g_ircSession.GetInfo().sNetwork.c_str());
				CheckDlgButton(hwndDlg, IDC_Q, BST_CHECKED);
				CheckDlgButton(hwndDlg, IDC_N, BST_CHECKED);
				CheckDlgButton(hwndDlg, IDC_I, BST_CHECKED);
				CheckDlgButton(hwndDlg, IDC_D, BST_CHECKED);
				CheckDlgButton(hwndDlg, IDC_C, BST_CHECKED);
				lstrcpynA(szOldMask, (char *) "\0", 499);
			}
			else lstrcpynA(szOldMask, (char *) lParam, 499);
		}
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {	
			case IDN_YES:
				{
					char szMask[500];
					char szNetwork[500];
					String flags = "";
					if(IsDlgButtonChecked(hwndDlg, IDC_Q) == BST_CHECKED)
						flags += "q";
					if(IsDlgButtonChecked(hwndDlg, IDC_N) == BST_CHECKED)
						flags += "n";
					if(IsDlgButtonChecked(hwndDlg, IDC_I) == BST_CHECKED)
						flags += "i";
					if(IsDlgButtonChecked(hwndDlg, IDC_D) == BST_CHECKED)
						flags += "d";
					if(IsDlgButtonChecked(hwndDlg, IDC_C) == BST_CHECKED)
						flags += "c";
					if(IsDlgButtonChecked(hwndDlg, IDC_M) == BST_CHECKED)
						flags += "m";

					GetWindowTextA(GetDlgItem(hwndDlg, IDC_MASK), szMask, 499);
					GetWindowTextA(GetDlgItem(hwndDlg, IDC_NETWORK), szNetwork, 499);

					String Mask = GetWord(szMask, 0);
					if(Mask.length() != 0) {
						if (!strchr(Mask.c_str(), '!') && !strchr(Mask.c_str(), '@'))
							Mask += "!*@*";

						if ( flags != "" ) {
							if ( *szOldMask )
								RemoveIgnore(szOldMask);
							AddIgnore(Mask, flags, szNetwork);
					}	}

					PostMessage (hwndDlg, WM_CLOSE, 0,0);
				}
				break;

			case IDN_NO:
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
				break;
			}
		break;

	case WM_CLOSE:
		PostMessage(IgnoreWndHwnd, IRC_FIXIGNOREBUTTONS, 0, 0);
		DestroyWindow(hwndDlg);
		return(true);
	}

	return false;
}

// Callback for the 'Connect preferences' dialog
BOOL CALLBACK IgnorePrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			OldListViewProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_LIST),GWL_WNDPROC,(LONG)ListviewSubclassProc);

			CheckDlgButton(hwndDlg, IDC_ENABLEIGNORE, prefs->Ignore?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_IGNOREFILE, prefs->DCCFileEnabled?BST_UNCHECKED:BST_CHECKED);
			CheckDlgButton(hwndDlg, IDC_IGNORECHAT, prefs->DCCChatEnabled?BST_UNCHECKED:BST_CHECKED);
			CheckDlgButton(hwndDlg, IDC_IGNORECHANNEL, prefs->IgnoreChannelDefault?BST_CHECKED:BST_UNCHECKED);
			if(prefs->DCCChatIgnore == 2)
				CheckDlgButton(hwndDlg, IDC_IGNOREUNKNOWN, BST_CHECKED);

			EnableWindow(GetDlgItem(hwndDlg, IDC_IGNOREUNKNOWN), prefs->DCCChatEnabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIST), prefs->Ignore);
			EnableWindow(GetDlgItem(hwndDlg, IDC_IGNORECHANNEL), prefs->Ignore);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), prefs->Ignore);

			SendDlgItemMessage(hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_ADD));
			SendDlgItemMessage(hwndDlg,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
			SendDlgItemMessage(hwndDlg,IDC_EDIT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_RENAME));
			SendMessage(GetDlgItem(hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)Translate("Add new ignore"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_EDIT), BUTTONADDTOOLTIP, (WPARAM)Translate("Edit this ignore"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)Translate("Delete this ignore"), 0);

			LV_COLUMN lvC;
			int COLUMNS_SIZES[3] ={195, 60,80};
			TCHAR* text;

			lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvC.fmt = LVCFMT_LEFT;
			for ( int index=0; index < 3; index++ ) {
				lvC.iSubItem = index;
				lvC.cx = COLUMNS_SIZES[index];

				switch (index) {
					case 0: text = TranslateT("Ignore mask"); break;
					case 1: text = TranslateT("Flags"); break;
					case 2: text = TranslateT("Network"); break;
				}
				lvC.pszText = text;
				ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW),index,&lvC);
			}

			ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW), LVS_EX_FULLROWSELECT);
			PostMessage(hwndDlg, IRC_REBUILDIGNORELIST, 0, 0);
		}
		return TRUE;

	case IRC_UPDATEIGNORELIST:
		{
			int j = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_LIST));
			if (j > 0 ) {
				LVITEM lvm;
				lvm.mask= LVIF_PARAM;
				lvm.iSubItem = 0;
				for( int i =0; i < j; i++) {
					lvm.iItem = i;
					lvm.lParam = i;
					ListView_SetItem(GetDlgItem(hwndDlg, IDC_LIST),&lvm);
		}	}	}
		break;

	case IRC_REBUILDIGNORELIST:
		{
			IgnoreWndHwnd = hwndDlg;
			ListView_DeleteAllItems	(GetDlgItem(hwndDlg, IDC_LIST));

			if (pszIgnoreFile) {
				char * p1 = pszIgnoreFile;
				char * p2 = NULL;
				char * pTemp = NULL;
				while(*p1 != '\0')
				{
					while(*p1 == '\r' || *p1 == '\n')
						p1++;
					if (*p1 == '\0')
						break;
					p2 = strstr(p1, "\r\n");
					if (!p2)
						p2 = strchr(p1, '\0');

					pTemp = p2;

					while (pTemp > p1 && (*pTemp == '\r' || *pTemp == '\n' ||*pTemp == '\0' || *pTemp == ' '))
						pTemp--;
					pTemp++;

					*pTemp = 0;
					TCHAR* p3 = mir_a2t( p1 );
					if ( GetWord(p3, 0) != _T("") && GetWord(p3, 0) != _T("")) {
						TString mask = GetWord(p3, 0);
						TString flags = GetWord(p3, 1);
						TString network = GetWordAddress(p3, 2);
						if ( flags[0] == '+' ) {
							LVITEM lvItem;
							HWND hListView = GetDlgItem(hwndDlg, IDC_LIST);
							lvItem.iItem = ListView_GetItemCount(hListView); 
							lvItem.mask = LVIF_TEXT|LVIF_PARAM ;
							lvItem.iSubItem = 0;
							lvItem.lParam = lvItem.iItem;
							lvItem.pszText = (TCHAR*)mask.c_str();
							lvItem.iItem = ListView_InsertItem(hListView,&lvItem);

							lvItem.mask = LVIF_TEXT;
							lvItem.iSubItem =1;
							lvItem.pszText = (TCHAR*)flags.c_str();
							ListView_SetItem(hListView,&lvItem);

							lvItem.mask = LVIF_TEXT;
							lvItem.iSubItem =2;
							lvItem.pszText = (TCHAR*)network.c_str();
							ListView_SetItem(hListView,&lvItem);
					}	}

					mir_free( p3 );
					p1 = p2;
			}	}

			SendMessage(hwndDlg, IRC_UPDATEIGNORELIST, 0, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LVM_SORTITEMS, (WPARAM)0, (LPARAM)IgnoreListSort);
			SendMessage(hwndDlg, IRC_UPDATEIGNORELIST, 0, 0);

			PostMessage(hwndDlg, IRC_FIXIGNOREBUTTONS, 0, 0);
		}
		break;

	case IRC_FIXIGNOREBUTTONS:
		EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), true);
		if(ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_LIST)) != -1) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), true);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), true);
		}
		else {
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), false);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
		}
		break;

	case WM_COMMAND:
		SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);

		if ( HIWORD(wParam) == STN_CLICKED ) 
		{ 
			if ( LOWORD(wParam) == IDC_CUSTOM )
				CallService(MS_UTILS_OPENURL,0,(LPARAM) "http://members.chello.se/matrix/troubleshooting.html" );
		}

		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam)) {
			case IDC_ENABLEIGNORE:
				EnableWindow(GetDlgItem(hwndDlg, IDC_IGNORECHANNEL), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIGNORE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIST), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIGNORE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIGNORE) == BST_CHECKED);
				break;

			case IDC_IGNORECHAT:
				EnableWindow(GetDlgItem(hwndDlg, IDC_IGNOREUNKNOWN), IsDlgButtonChecked(hwndDlg, IDC_IGNORECHAT) == BST_UNCHECKED);
				break;

			case IDC_ADD:
				{
					HWND hWnd = CreateDialogParam(g_hInstance,MAKEINTRESOURCE(IDD_ADDIGNORE), hwndDlg,AddIgnoreWndProc, 0);
					SetWindowTextA(hWnd, Translate("Add Ignore"));
					EnableWindow(GetDlgItem((hwndDlg), IDC_ADD), false);
					EnableWindow(GetDlgItem((hwndDlg), IDC_EDIT), false);
					EnableWindow(GetDlgItem((hwndDlg), IDC_DELETE), false);
					break;
				}

			case IDC_EDIT:
				if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDC_ADD ))) {
					TCHAR szMask[512];
					TCHAR szFlags[512];
					TCHAR szNetwork[512];
					int i = ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_LIST));
					ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), i, 0, szMask, 511); 
					ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), i, 1, szFlags, 511); 
					ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), i, 2, szNetwork, 511); 
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
					EnableWindow(GetDlgItem((hwndDlg), IDC_ADD), false);
					EnableWindow(GetDlgItem((hwndDlg), IDC_EDIT), false);
					EnableWindow(GetDlgItem((hwndDlg), IDC_DELETE), false);
				}
				break;

			case IDC_DELETE:
				if ( IsWindowEnabled(GetDlgItem(hwndDlg, IDC_DELETE))) {
					char szMask[512];
					int i = ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_LIST));
					ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), i, 0, (TCHAR*)szMask, 511); // !!!! 
					RemoveIgnore(szMask);
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
				if ( ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_LIST)) != -1 )
					SendMessage(hwndDlg, IRC_FIXIGNOREBUTTONS, 0, 0);
				break;
				
			case NM_DBLCLK:
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
				break;

			case LVN_COLUMNCLICK :
				{
					LPNMLISTVIEW lv;
					lv = (LPNMLISTVIEW)lParam;
					SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LVM_SORTITEMS, (WPARAM)lv->iSubItem, (LPARAM)IgnoreListSort);
					SendMessage(hwndDlg, IRC_UPDATEIGNORELIST, 0, 0);
				}
				break;
			}

		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				prefs->DCCFileEnabled = IsDlgButtonChecked(hwndDlg,IDC_IGNOREFILE)== BST_UNCHECKED?1:0;
				prefs->DCCChatEnabled = IsDlgButtonChecked(hwndDlg,IDC_IGNORECHAT)== BST_UNCHECKED?1:0;
				prefs->Ignore = IsDlgButtonChecked(hwndDlg,IDC_ENABLEIGNORE)== BST_CHECKED?1:0;
				prefs->IgnoreChannelDefault = IsDlgButtonChecked(hwndDlg,IDC_IGNORECHANNEL)== BST_CHECKED?1:0;
				prefs->DCCChatIgnore = IsDlgButtonChecked(hwndDlg, IDC_IGNOREUNKNOWN) == BST_CHECKED?2:1;
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
			int i = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_LIST));
			String S = "";
			for (int j = 0; j<i; j++) {
				char szItem[512];
				ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), j, 0, (TCHAR*)szItem, 511); // !!!! 
				S += szItem;
				S += " ";
				ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), j, 1, (TCHAR*)szItem, 511); // !!!!
				S += szItem;
				S += " ";
				ListView_GetItemText(GetDlgItem(hwndDlg, IDC_LIST), j, 2, (TCHAR*)szItem, 511); // !!!!
				S += szItem;
				S += "\r\n";
			}
			char filepath[MAX_PATH];
			mir_snprintf(filepath, sizeof(filepath), "%s\\%s_ignore.ini", mirandapath, IRCPROTONAME);
			FILE *hFile = fopen(filepath,"wb");
			if ( hFile ) {
				fputs(S.c_str(), hFile);
				fclose(hFile);
			}
			if (pszIgnoreFile)
				delete [] pszIgnoreFile;
			pszIgnoreFile = IrcLoadFile(filepath);

			SetWindowLong(GetDlgItem(hwndDlg,IDC_LIST),GWL_WNDPROC,(LONG)OldListViewProc);
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
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_CTCP);
	odp.pszTab = LPGEN("DCC'n CTCP");
	odp.pfnDlgProc = CtcpPrefsProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_OTHER);
	odp.pszTab = LPGEN("Advanced");
	odp.pfnDlgProc = OtherPrefsProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_PREFS_IGNORE);
	odp.pszTab = LPGEN("Ignore");
	odp.pfnDlgProc = IgnorePrefsProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

void UnInitOptions(void)
{
	mir_free(prefs->Alias);
	delete prefs;
	delete []pszServerFile;
	delete []pszPerformFile;
	delete []pszIgnoreFile;
}
