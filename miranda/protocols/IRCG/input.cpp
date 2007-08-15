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

bool bEcho = true;
bool bTempDisableCheck = false;
bool bTempForceCheck = false;

static TString FormatMsg(TString text)
{
	TCHAR temp[30];
	lstrcpyn(temp, GetWord(text.c_str(), 0).c_str(), 29);
	CharLower(temp);
	TString command = temp;
	TString S = _T("");
	if (command == _T("/quit") || command == _T("/away")) 
		S = GetWord(text.c_str(), 0) + _T(" :") + GetWordAddress(text.c_str(), 1);
	else if (command == _T("/privmsg") || command == _T("/part") || command == _T("/topic") || command == _T("/notice")) {
		S = GetWord(text.c_str(), 0) + _T(" ") + GetWord(text.c_str(), 1) ;
		if (!GetWord(text.c_str(), 2).empty())
			S += (TString)_T(" :") + TString(GetWordAddress(text.c_str(), 1) +1 + lstrlen(GetWord(text.c_str(), 1).c_str()));
	}
	else if (command == _T("/kick"))
		S = GetWord(text.c_str(), 0) + _T(" ") + GetWord(text.c_str(), 1) + _T(" ") + GetWord(text.c_str(), 2)+ _T(" :") + GetWordAddress(text.c_str(), 3);
	else 
		S = GetWordAddress(text.c_str(), 0);
	S.erase(0,1);
	return S;
}

static TString AddCR( TString text )
{
	text = ReplaceString( text, _T("\n"), _T("\r\n"));
	text = ReplaceString( text, _T("\r\r"), _T("\r"));
	return text;
}

static TString DoAlias( const TCHAR *text, TCHAR *window)
{
	TString Messageout = _T("");
	const TCHAR* p1 = text;
	const TCHAR* p2 = text;
	bool LinebreakFlag = false;
	p2 = _tcsstr(p1, _T("\r\n"));
	if ( !p2 )
		p2 = _tcschr(p1, '\0');
	if ( p1 == p2 )
		return (TString)text;
	
	do {
		if ( LinebreakFlag )
			Messageout += _T("\r\n");
		
		TCHAR* line = new TCHAR[p2-p1 +1]; 
		lstrcpyn(line, p1, p2-p1+1);
		TCHAR* test = line;
		while ( *test == ' ' )
			test++;
		if ( *test == '/' ) {
			lstrcpyn(line, GetWordAddress(line, 0), p2-p1+1);
			TString S = line;
			delete [] line;
			line = new TCHAR[S.length()+2]; 
			lstrcpyn(line, S.c_str(), S.length()+1);
			TString alias( prefs->Alias );
			const TCHAR* p3 = _tcsstr( alias.c_str(), (GetWord(line, 0)+ _T(" ")).c_str());
			if ( p3 != alias.c_str()) {
				p3 = _tcsstr( alias.c_str(), (TString(_T("\r\n")) + GetWord(line, 0) + _T(" ")).c_str());
				if ( p3 )
					p3 += 2;
			}
			if ( p3 != NULL ) {
				const TCHAR* p4 = _tcsstr( p3, _T("\r\n"));
				if ( !p4 )
					p4 = _tcschr( p3, '\0' );

				*( TCHAR* )p4 = 0;
				TString S = ReplaceString( p3, _T("##"), window );
				S = ReplaceString(S.c_str(), _T("$?"), _T("%question"));

				for ( int index = 1; index < 8; index++ ) {
					TCHAR str[5];
					mir_sntprintf( str, SIZEOF(str), _T("#$%u"), index );
					if ( !GetWord(line, index).empty() && IsChannel( GetWord( line, index )))
						S = ReplaceString( S, str, GetWord(line, index).c_str());
					else
						S = ReplaceString( S, str, (TString(_T("#")) + GetWord( line, index )).c_str());
				}
				for ( int index2 = 1; index2 <8; index2++ ) {
					TCHAR str[5];
					mir_sntprintf( str, SIZEOF(str), _T("$%u-"), index2 );
					S = ReplaceString( S, str, GetWordAddress( line, index2 ));
				}
				for ( int index3 = 1; index3 <8; index3++ ) {
					TCHAR str[5];
					mir_sntprintf( str, SIZEOF(str), _T("$%u"), index3 );
					S = ReplaceString( S, str, GetWord(line, index3).c_str());
				}
				Messageout += GetWordAddress(S.c_str(), 1);
			} 
			else Messageout += line;
		}
		else Messageout += line;

		p1 = p2;
		if ( *p1 == '\r' )
			p1 += 2;
		p2 = _tcsstr( p1, _T("\r\n"));
		if ( !p2 )
			p2 = _tcschr( p1, '\0' );
		delete [] line;
		LinebreakFlag = true;
	}
		while ( *p1 != '\0');
	
	return Messageout;
}

static TString DoIdentifiers( TString text, const TCHAR* window )
{
	SYSTEMTIME time;
	TCHAR str[800];
	int  i = 0;

	GetLocalTime( &time );
	text = ReplaceString( text, _T("%mnick"), prefs->Nick);
	text = ReplaceString( text, _T("%anick"), prefs->AlternativeNick);
	text = ReplaceString( text, _T("%awaymsg"), StatusMessage.c_str());
	text = ReplaceString( text, _T("%module"), _A2T(IRCPROTONAME));
	text = ReplaceString( text, _T("%name"), prefs->Name);
	text = ReplaceString( text, _T("%newl"), _T("\r\n"));
	text = ReplaceString( text, _T("%network"), g_ircSession.GetInfo().sNetwork.c_str());
	text = ReplaceString( text, _T("%me"), g_ircSession.GetInfo().sNick.c_str());

	mir_sntprintf( str, SIZEOF(str), _T("%d.%d.%d.%d"),(mirVersion>>24)&0xFF,(mirVersion>>16)&0xFF,(mirVersion>>8)&0xFF,mirVersion&0xFF);
	text = ReplaceString(text, _T("%mirver"), str);

	mir_sntprintf( str, SIZEOF(str), _T("%d.%d.%d.%d"),(pluginInfo.version>>24)&0xFF,(pluginInfo.version>>16)&0xFF,(pluginInfo.version>>8)&0xFF,pluginInfo.version&0xFF);
	text = ReplaceString(text, _T("%version"), str);

	str[0] = (char)3; str[1] = '\0';
	text = ReplaceString(text, _T("%color"), str);

	str[0] = (char)2;
	text = ReplaceString(text, _T("%bold"), str);

	str[0] = (char)31;
	text = ReplaceString(text, _T("%underline"), str);

	str[0] = (char)22;
	text = ReplaceString(text, _T("%italics"), str);
	return text;
}

static void __stdcall sttSetTimerOn( void* )
{
	DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT(	"The buddy check function is enabled"), NULL, NULL, NULL, true, false); 
	SetChatTimer( OnlineNotifTimer, 500, OnlineNotifTimerProc );
	if ( prefs->ChannelAwayNotification )
		SetChatTimer( OnlineNotifTimer3, 1500, OnlineNotifTimerProc3 );
}

static void __stdcall sttSetTimerOff( void* )
{
	DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("The buddy check function is disabled"), NULL, NULL, NULL, true, false); 
	KillChatTimer( OnlineNotifTimer );
	KillChatTimer( OnlineNotifTimer3 );
}

static BOOL DoHardcodedCommand( TString text, TCHAR* window, HANDLE hContact )
{
	TCHAR temp[30];
	lstrcpyn(temp, GetWord(text.c_str(), 0).c_str(), 29 );
	CharLower(temp);
	TString command = temp;
	TString one = GetWord(text.c_str(), 1);
	TString two = GetWord(text.c_str(), 2);
	TString three = GetWord(text.c_str(), 3);
	TString therest = GetWordAddress(text.c_str(), 4);

	if ( command == _T("/servershow") || command == _T("/serverhide")) {
		if ( prefs->UseServer ) {
			GCEVENT gce = {0};
			GCDEST gcd = {0};
			gce.dwFlags = GC_TCHAR;
			gcd.iType = GC_EVENT_CONTROL;
			gcd.ptszID = _T("Network log");
			gcd.pszModule = IRCPROTONAME;
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			CallChatEvent( command == _T("/servershow") ? WINDOW_VISIBLE : WINDOW_HIDDEN, (LPARAM)&gce);
		}
		return true;
	}

	else if (command == _T("/clear")) {
		TString S;
		if ( !one.empty() ) {
			if ( one == _T("server"))
				S = _T("Network log");
			else
				S = MakeWndID( one.c_str() );
		}
		else if ( lstrcmpi( window, _T("Network log")) == 0 )
			S = window;
		else
			S = MakeWndID( window );
		
		GCEVENT gce = {0};
		GCDEST gcd = {0};
		gce.cbSize = sizeof(GCEVENT);
		gcd.iType = GC_EVENT_CONTROL;
		gcd.pszModule = IRCPROTONAME;
		gce.pDest = &gcd;
		gce.dwFlags = GC_TCHAR;
		gcd.ptszID = (TCHAR*)S.c_str();
		CallChatEvent( WINDOW_CLEARLOG, (LPARAM)&gce);
		return true;
	}
	else if ( command == _T("/ignore")) {
		if ( g_ircSession ) {
			TString IgnoreFlags;
			TCHAR temp[500];
			if ( one.empty() ) {
				if ( prefs->Ignore )
					DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("Ignore system is enabled"), NULL, NULL, NULL, true, false); 
				else
					DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("Ignore system is disabled"), NULL, NULL, NULL, true, false); 
				return true;
			}
			if ( !lstrcmpi( one.c_str(), _T("on"))) {
				prefs->Ignore = 1;
				DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("Ignore system is enabled"), NULL, NULL, NULL, true, false); 
				return true;
			}
			if ( !lstrcmpi( one.c_str(), _T("off"))) {
				prefs->Ignore = 0;
				DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("Ignore system is disabled"), NULL, NULL, NULL, true, false); 
				return true;
			}
			if ( !_tcschr( one.c_str(), '!' ) && !_tcschr( one.c_str(), '@' ))
				one += _T("!*@*");
			
			if ( !two.empty() && two[0] == '+' ) {
				if ( _tcschr( two.c_str(), 'q'))
					IgnoreFlags += 'q';
				if ( _tcschr( two.c_str(), 'n'))
					IgnoreFlags += 'n';
				if ( _tcschr( two.c_str(), 'i'))
					IgnoreFlags += 'i';
				if ( _tcschr( two.c_str(), 'd'))
					IgnoreFlags += 'd';
				if ( _tcschr( two.c_str(), 'c'))
					IgnoreFlags += 'c';
				if ( _tcschr( two.c_str(), 'm'))
					IgnoreFlags += 'm';
			}
			else IgnoreFlags = _T("qnidc");

			TString Network;
			if ( three.empty() )
				Network = g_ircSession.GetInfo().sNetwork;
			else
				Network = three;

			AddIgnore( one.c_str(), IgnoreFlags.c_str(), Network.c_str() );
			
			mir_sntprintf(temp, SIZEOF(temp), TranslateT("%s on %s is now ignored (+%s)"), one.c_str(), Network.c_str(), IgnoreFlags.c_str());
			DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
		}
		return true;
	}
	else if (command == _T("/unignore")) {
		if ( !_tcschr( one.c_str(), '!' ) && !_tcschr(one.c_str(), '@'))
			one += _T("!*@*");
		
		TCHAR temp[500];
		if ( RemoveIgnore( one.c_str()))
			mir_sntprintf(temp, SIZEOF(temp), TranslateT("%s is not ignored now"), one.c_str());
		else
			mir_sntprintf(temp, SIZEOF(temp), TranslateT("%s was not ignored"), one.c_str());
		DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
		return true;
	}

	else if ( command == _T("/userhost")) {
 		if ( one.empty())
			return true;

		DoUserhostWithReason( 1, _T("U"), false, temp );
 		return false;
 	}
 
	else if ( command == _T("/joinx")) {
		if ( one.empty())
			return true;

		AddToJTemp( _T("X")+one );

		PostIrcMessage( _T("/JOIN %s"), GetWordAddress(text.c_str(), 1));
		return true;
	}

	else if ( command == _T("/joinm")) {
		if ( one.empty())
			return true;

		AddToJTemp( _T("M")+one );

		PostIrcMessage( _T("/JOIN %s"), GetWordAddress(text.c_str(), 1));
		return true;
	}
	else if (command == _T("/nusers")) {
		TCHAR szTemp[40];
		TString S = MakeWndID(window);
		GC_INFO gci = {0};
		gci.Flags = BYID|NAME|COUNT;
		gci.pszModule = IRCPROTONAME;
		gci.pszID = (TCHAR*)S.c_str();
		if ( !CallServiceSync( MS_GC_GETINFO, 0, ( LPARAM )&gci ))
			mir_sntprintf( szTemp, SIZEOF(szTemp), _T("users: %u"), gci.iCount);

		DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
		return true;
	}
	else if (command == _T("/echo")) {
		if ( one.empty())
			return true;

		if ( !lstrcmpi( one.c_str(), _T("on"))) {
			bEcho = TRUE;
			DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("Outgoing commands are shown"), NULL, NULL, NULL, true, false); 
		}
		
		if ( !lstrcmpi( one.c_str(), _T("off"))) {
			DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("Outgoing commands are not shown"), NULL, NULL, NULL, true, false); 
			bEcho = FALSE;
		}

		return true;
	}
	
	else if (command == _T("/buddycheck")) {
		if ( one.empty()) {
			if (( prefs->AutoOnlineNotification && !bTempDisableCheck) || bTempForceCheck )
				DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("The buddy check function is enabled"), NULL, NULL, NULL, true, false); 
			else
				DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("The buddy check function is disabled"), NULL, NULL, NULL, true, false); 
			return true;
		}
		if ( !lstrcmpi( one.c_str(), _T("on"))) {
			bTempForceCheck = true;
			bTempDisableCheck = false;
			CallFunctionAsync( sttSetTimerOn, NULL );
		}
		if ( !lstrcmpi( one.c_str(), _T("off"))) {
			bTempForceCheck = false;
			bTempDisableCheck = true;
			CallFunctionAsync( sttSetTimerOff, NULL );
		}
		if ( !lstrcmpi( one.c_str(), _T("time")) && !two.empty()) {
			iTempCheckTime = StrToInt( two.c_str());
			if ( iTempCheckTime < 10 && iTempCheckTime != 0 )
				iTempCheckTime = 10;

			if ( iTempCheckTime == 0 )
				DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), TranslateT("The time interval for the buddy check function is now at default setting"), NULL, NULL, NULL, true, false); 
			else {
				TCHAR temp[200];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("The time interval for the buddy check function is now %u seconds"), iTempCheckTime);
				DoEvent( GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
		}	}
		return true;
	}
	else if (command == _T("/whois")) {
		if ( one.empty())
			return false;
		ManualWhoisCount++;
		return false;
	}

	else if ( command == _T("/channelmanager")) {
		if ( window && !hContact && IsChannel( window )) {
			if ( g_ircSession ) {
				if ( manager_hWnd != NULL ) {
					SetActiveWindow( manager_hWnd );
					SendMessage( manager_hWnd, WM_CLOSE, 0, 0 );
				}
				if ( manager_hWnd == NULL ) {
					manager_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE(IDD_CHANMANAGER), NULL, ManagerWndProc );
					SetWindowText( manager_hWnd, TranslateT( "Channel Manager" ));
					SetWindowText( GetDlgItem( manager_hWnd, IDC_CAPTION ), window );
					SendMessage( manager_hWnd, IRC_INITMANAGER, 1, (LPARAM)window );
		}	}	}
		return true;
	}

	else if ( command == _T("/who")) {
		if ( one.empty())
			return true;
		DoUserhostWithReason( 2, _T("U"), false, _T("%s"), one.c_str());
		return false;
	}

	else if (command == _T("/hop")) {
		if ( !IsChannel( window ))
			return true;

		PostIrcMessage( _T("/PART %s"), window );

		if (( one.empty() || !IsChannel( one ))) {
			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if ( wi && wi->pszPassword )
				PostIrcMessage( _T("/JOIN %s %s"), window, wi->pszPassword);
			else
				PostIrcMessage( _T("/JOIN %s"), window);
			return true;
		}

		GCEVENT gce = {0};
		GCDEST gcd = {0};
		gcd.iType = GC_EVENT_CONTROL;
		TString S = MakeWndID(window);
		gcd.ptszID = (TCHAR*)S.c_str();
		gcd.pszModule = IRCPROTONAME;
		gce.cbSize = sizeof(GCEVENT);
		gce.dwFlags = GC_TCHAR;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);

		PostIrcMessage( _T("/JOIN %s"), GetWordAddress(text.c_str(), 1));
		return true;
	}
	
	else if (command == _T("/list" )) {
		if ( list_hWnd == NULL )
			list_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE(IDD_LIST), NULL, ListWndProc );
		SetActiveWindow( list_hWnd );
		int minutes = ( int )NoOfChannels/4000;
		int minutes2 = ( int )NoOfChannels/9000;
		
		TCHAR text[250];
		mir_sntprintf( text, SIZEOF(temp), TranslateT("This command is not recommended on a network of this size!\r\nIt will probably cause high CPU usage and/or high bandwidth\r\nusage for around %u to %u minute(s).\r\n\r\nDo you want to continue?"), minutes2, minutes);
		if ( NoOfChannels < 4000 || ( NoOfChannels >= 4000 && MessageBox( NULL, text, TranslateT("IRC warning") , MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2) == IDYES)) {
			ListView_DeleteAllItems( GetDlgItem( list_hWnd, IDC_INFO_LISTVIEW ));
			PostIrcMessage( _T("/lusers" ));
			return false;
		}
		SetDlgItemText( list_hWnd, IDC_TEXT, TranslateT("Aborted")); 
		return true;
	}
	
	else if (command == _T("/me")) {
		if ( one.empty())
			return true;
		
		TCHAR szTemp[4000];
		mir_sntprintf(szTemp, SIZEOF(szTemp), _T("\1ACTION %s\1"), GetWordAddress(text.c_str(), 1));
		PostIrcMessageWnd( window, hContact, szTemp );
		return true;
	}

	else if (command == _T("/ame")) {
		if ( one.empty())
			return true;

		TString S = _T("/ME ") + DoIdentifiers(GetWordAddress(text.c_str(), 1), window);
		S = ReplaceString(S, _T("%"), _T("%%"));
		DoEvent( GC_EVENT_SENDMESSAGE, NULL, NULL, S.c_str(), NULL, NULL, NULL, FALSE, FALSE);
		return true;
	}
	
	else if (command == _T("/amsg")) {
		if ( one.empty())
			return true;
		
		TString S = DoIdentifiers( GetWordAddress(text.c_str(), 1), window );
		S = ReplaceString( S, _T("%"), _T("%%"));
		DoEvent( GC_EVENT_SENDMESSAGE, NULL, NULL, S.c_str(), NULL, NULL, NULL, FALSE, FALSE);
		return true;
	}

	else if (command == _T("/msg")) {
		if ( one.empty() || two.empty())
			return true;

		TCHAR szTemp[4000];
		mir_sntprintf(szTemp, SIZEOF(szTemp), _T("/PRIVMSG %s"), GetWordAddress(text.c_str(), 1));

		PostIrcMessageWnd(window, hContact, szTemp);
		return true;
	}

	else if (command == _T("/query")) {
		if ( one.empty() || IsChannel(one.c_str()))
			return true;

		CONTACT user = { (TCHAR*)one.c_str(), NULL, NULL, false, false, false};
		HANDLE hContact2 = CList_AddContact(&user, false, false);
		if ( hContact2 ) {
			if ( DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0 ) == 0 )
				DoUserhostWithReason(1, (_T("S") + one).c_str(), true, one.c_str());
			else {
				DBVARIANT dbv1;
				if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UWildcard", &dbv1 )) {
					DoUserhostWithReason(2, (TString(_T("S")) + dbv1.ptszVal).c_str(), true, dbv1.ptszVal);
					DBFreeVariant(&dbv1);
				}
				else DoUserhostWithReason(2, (TString(_T("S")) + one).c_str(), true, one.c_str());
			}
			
			CallService( MS_MSG_SENDMESSAGE, ( WPARAM )hContact2, 0 );
		}

		if ( !two.empty()) {
			TCHAR szTemp[4000];
			mir_sntprintf( szTemp, SIZEOF(szTemp), _T("/PRIVMSG %s"), GetWordAddress(text.c_str(), 1));
			PostIrcMessageWnd( window, hContact, szTemp );
		}
		return true;
	}	
	else if (command == _T("/ctcp")) {
		if ( one.empty() || two.empty())
			return true;

		TCHAR szTemp[1000];
		unsigned long ulAdr = 0;
		if ( prefs->ManualHost )
			ulAdr = ConvertIPToInteger( prefs->MySpecifiedHostIP );
		else
			ulAdr = ConvertIPToInteger( prefs->IPFromServer ? prefs->MyHost : prefs->MyLocalHost );

		 // if it is not dcc or if it is dcc and a local ip exist
		if ( lstrcmpi( two.c_str(), _T("dcc")) != 0 || ulAdr ) {
			if ( lstrcmpi( two.c_str(), _T("ping")) == 0 )
				mir_sntprintf( szTemp, SIZEOF(szTemp), _T("/PRIVMSG %s \001%s %u\001"), one.c_str(), two.c_str(), time(0));
			else
				mir_sntprintf( szTemp, SIZEOF(szTemp), _T("/PRIVMSG %s \001%s\001"), one.c_str(), GetWordAddress(text.c_str(), 2));
			PostIrcMessageWnd( window, hContact, szTemp );
		}

		if ( lstrcmpi(two.c_str(), _T("dcc")) != 0 ) {
			mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("CTCP %s request sent to %s"), two.c_str(), one.c_str());
			DoEvent(GC_EVENT_INFORMATION, _T("Network Log"), g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
		}

		return true;
	}		
	else if (command == _T("/dcc")) {
		if ( one.empty() || two.empty())
			return true;
		
		if ( lstrcmpi( one.c_str(), _T("send")) == 0 ) {
			TCHAR szTemp[1000];
			unsigned long ulAdr = 0;

			if ( prefs->ManualHost )
				ulAdr = ConvertIPToInteger( prefs->MySpecifiedHostIP );
			else
				ulAdr = ConvertIPToInteger( prefs->IPFromServer ? prefs->MyHost : prefs->MyLocalHost );

			if ( ulAdr ) {
				CONTACT user = { (TCHAR*)two.c_str(), NULL, NULL, false, false, true };
				HANDLE hContact = CList_AddContact( &user, false, false );
				if ( hContact ) {
					TString s;
					
					if ( DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0 ) == 0 )
						DoUserhostWithReason( 1, (_T("S") + two).c_str(), true, two.c_str());
					else {
						DBVARIANT dbv1;
						if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UWildcard", &dbv1 )) {
							DoUserhostWithReason(2, (TString(_T("S")) + dbv1.ptszVal).c_str(), true, dbv1.ptszVal );
							DBFreeVariant( &dbv1 );
						}
						else DoUserhostWithReason( 2, (TString(_T("S")) + two).c_str(), true, two.c_str());
					}
					
					if ( three.empty())
						CallService( MS_FILE_SENDFILE, ( WPARAM )hContact, 0 );
					else {
						TString temp = GetWordAddress(text.c_str(), 3);
						TCHAR* pp[2];
						TCHAR* p = ( TCHAR* )temp.c_str();
						pp[0] = p;
						pp[1] = NULL;
						CallService( MS_FILE_SENDSPECIFICFILES, (WPARAM)hContact, (LPARAM)pp );
				}	}
			}
			else {
				mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Unable to automatically resolve external IP"));
				DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
			}
			return true;
		}

		if ( lstrcmpi( one.c_str(), _T("chat")) == 0 ) {
			TCHAR szTemp[1000];

			unsigned long ulAdr = 0;
			if ( prefs->ManualHost )
				ulAdr = ConvertIPToInteger( prefs->MySpecifiedHostIP );
			else
				ulAdr = ConvertIPToInteger( prefs->IPFromServer ? prefs->MyHost : prefs->MyLocalHost );

			if ( ulAdr ) {
				TString contact = two + _T(DCCSTRING);
				CONTACT user = { (TCHAR*)contact.c_str(), NULL, NULL, false, false, true};
				HANDLE hContact = CList_AddContact( &user, false, false );
				DBWriteContactSettingByte(hContact, IRCPROTONAME, "DCC", 1);

				int iPort = 0;
				if ( hContact ) {
					DCCINFO* dci = new DCCINFO;
					ZeroMemory(dci, sizeof(DCCINFO));
					dci->hContact = hContact;
					dci->sContactName = two;
					dci->iType = DCC_CHAT;
					dci->bSender = true;

					CDccSession* dcc = new CDccSession(dci);
					CDccSession* olddcc = g_ircSession.FindDCCSession(hContact);
					if ( olddcc )
						olddcc->Disconnect();
					g_ircSession.AddDCCSession(hContact, dcc);
					iPort = dcc->Connect();
				}

				if ( iPort != 0 ) {
					PostIrcMessage( _T("/CTCP %s DCC CHAT chat %u %u"), two.c_str(), ulAdr, iPort );
					mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("DCC CHAT request sent to %s"), two.c_str(), one.c_str());
					DoEvent( GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
				}
				else {
					mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Unable to bind port"));
					DoEvent( GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
				}
			}
			else {
				mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Unable to automatically resolve external IP"));
				DoEvent( GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
		}	}
		return true;
	}		
	return false;
}

static void __stdcall DoInputRequestAliasApcStub( void* str )
{
	TCHAR* infotext = NULL;
	TCHAR* title = NULL;
	TCHAR* defaulttext = NULL;
	TString command = ( TCHAR* )str;
	TCHAR* p = _tcsstr(( TCHAR* )str, _T("%question"));
	if ( p[9] == '=' && p[10] == '\"' ) {
		infotext = &p[11];
		p = _tcschr( infotext, '\"' );
		if ( p ) {
			*p = '\0';
			p++;
			if ( *p == ',' && p[1] == '\"' ) {
				p++; p++;
				title = p;
				p = _tcschr( title, '\"' );
				if ( p ) {
					*p = '\0';
					p++;
					if ( *p == ',' && p[1] == '\"' ) {
						p++; p++;
						defaulttext = p;
						p = _tcschr( defaulttext, '\"' );
						if ( p )
							*p = '\0';
	}	}	}	}	}

	HWND question_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE(IDD_QUESTION), NULL, QuestionWndProc );

	if ( title )
		SetDlgItemText( question_hWnd, IDC_CAPTION, title);
	else
		SetDlgItemText( question_hWnd, IDC_CAPTION, TranslateT("Input command"));

	if ( infotext )
		SetWindowText( GetDlgItem( question_hWnd, IDC_TEXT), infotext );
	else
		SetWindowText( GetDlgItem( question_hWnd, IDC_TEXT), TranslateT("Please enter the reply"));

	if ( defaulttext )
		SetWindowText( GetDlgItem( question_hWnd, IDC_EDIT), defaulttext );

	SetDlgItemText( question_hWnd, IDC_HIDDENEDIT, command.c_str());
	PostMessage( question_hWnd, IRC_ACTIVATE, 0, 0);

	mir_free( str );
}

static bool DoInputRequestAlias( TCHAR* text )
{
	TCHAR* p = _tcsstr( text, _T("%question"));
	if ( p == NULL )
		return false;

	CallFunctionAsync( DoInputRequestAliasApcStub, mir_tstrdup( text ));
	return true;
}

bool PostIrcMessage( const TCHAR* fmt, ... )
{
	if ( !fmt || lstrlen(fmt) < 1 || lstrlen(fmt) > 4000 )
		return 0;

	va_list marker;
	va_start( marker, fmt );
	static TCHAR szBuf[4*1024];
	mir_vsntprintf( szBuf, SIZEOF(szBuf), fmt, marker );
	va_end( marker );

	return PostIrcMessageWnd(NULL, NULL, szBuf);
}

bool PostIrcMessageWnd( TCHAR* window, HANDLE hContact, const TCHAR* szBuf )
{
	DBVARIANT dbv;
	TCHAR windowname[256];
	BYTE bDCC = 0;
	
	if ( hContact )
		bDCC = DBGetContactSettingByte( hContact, IRCPROTONAME, "DCC", 0 );

	if ( !g_ircSession && !bDCC || !szBuf || lstrlen(szBuf) < 1 )
		return 0;

	if ( hContact && !DBGetContactSettingTString( hContact, IRCPROTONAME, "Nick", &dbv )) {
		lstrcpyn( windowname, dbv.ptszVal, 255); 
		DBFreeVariant(&dbv);
	}
	else if( window )
		lstrcpyn( windowname, window, 255 );
	else 
		lstrcpyn( windowname, _T("Network log"), 255 );

	if ( lstrcmpi( window, _T("Network log")) != 0 ) {
		TCHAR* p1 = _tcschr( windowname, ' ' );
		if ( p1 )
			*p1 = '\0';
	}

	// remove unecessary linebreaks, and do the aliases
	TString Message = szBuf;
	Message = AddCR( Message );
	Message = RemoveLinebreaks( Message );
	if ( !hContact && g_ircSession ) {
		Message = DoAlias( Message.c_str(), windowname );
		if ( DoInputRequestAlias(( TCHAR* )Message.c_str()))
			return 1;
		Message = ReplaceString( Message, _T("%newl"), _T("\r\n"));
		Message = RemoveLinebreaks( Message );
	}

	if ( Message.empty())
		return 0;
	
	CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, windowname, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	int codepage = ( wi ) ? wi->codepage : g_ircSession.getCodepage();

	// process the message
	while ( !Message.empty()) {
		// split the text into lines, and do an automatic textsplit on long lies as well
		bool flag = false;
		TString DoThis = _T("");
		int index = Message.find( _T("\r\n"), 0 );
		if ( index == string::npos )
			index = Message.length();

		if ( index > 480 )
			index = 480;
		DoThis = Message.substr(0, index);
		Message.erase(0, index);
		if ( Message.find( _T("\r\n"), 0 ) == 0 )
			Message.erase( 0, 2 );

		//do this if it's a /raw
		if ( g_ircSession && ( GetWord(DoThis.c_str(), 0) == _T("/raw") || GetWord(DoThis.c_str(), 0) == _T("/quote"))) {
			if ( GetWord( DoThis.c_str(), 1 ).empty())
				continue;
			
			TString S = GetWordAddress( DoThis.c_str(), 1 );
			g_ircSession << CIrcMessage( S.c_str(), codepage );
			continue;
		}

		// Do this if the message is not a command
		if ( GetWord( DoThis.c_str(), 0)[0] != '/' || hContact ) {
			if ( lstrcmpi(window, _T("Network log")) == 0 && !g_ircSession.GetInfo().sServerName.empty() )
				DoThis = (TString)_T("/PRIVMSG ") + g_ircSession.GetInfo().sServerName + _T(" ") + DoThis;
			else
				DoThis = (TString)_T("/PRIVMSG ") + windowname + _T(" ") + DoThis;
			flag = true;
		}

		// and here we send it unless the command was a hardcoded one that should not be sent
		if ( DoHardcodedCommand( DoThis, windowname, hContact ))
			continue;

		if ( !g_ircSession && !bDCC )
			continue;

		if ( !flag && g_ircSession )
			DoThis = DoIdentifiers(DoThis, windowname);
				
		if ( hContact ) {
			if ( flag && bDCC ) {
				CDccSession* dcc = g_ircSession.FindDCCSession( hContact );
				if ( dcc ) {
					DoThis = FormatMsg( DoThis );
					TString mess = GetWordAddress(DoThis.c_str(), 2);
					if ( mess[0] == ':' )
						mess.erase(0,1);
					mess += '\n';
					dcc->SendStuff( mess.c_str());
				}
			}
			else if( g_ircSession ) {
				DoThis = FormatMsg( DoThis );
				g_ircSession << CIrcMessage( DoThis.c_str(), codepage, false, false );
			}
		}
		else {
			DoThis = FormatMsg( DoThis );
			g_ircSession << CIrcMessage( DoThis.c_str(), codepage, false, true );
	}	}

	return 1;
}
