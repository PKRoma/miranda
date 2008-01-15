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

// This file holds functions that are called upon receiving
// certain commands from the server.

#include "irc.h"
#include <algorithm>

using namespace irc;

int            ChannelNumber =0;
TString        WhoReply = _T("");
bool           nickflag;
bool           bPerformDone = false;
int            NoOfChannels = 0;
HWND           whois_hWnd = NULL;
HWND           list_hWnd = NULL;
HWND           nick_hWnd = NULL;
HWND           manager_hWnd = NULL;
UINT_PTR       InitTimer;
UINT_PTR       IdentTimer;
int            ManualWhoisCount = 0;
TString        sNamesList;
String         sUserModes;
TString        sUserModePrefixes;
String         sChannelModes;
TString        sChannelPrefixes;
TString        WhoisAwayReply;
TString        sTopic;
TString        sTopicName;
TString		   sTopicTime;
TString        NamesToWho = _T("");
TString        ChannelsToWho = _T("");
TString        NamesToUserhost = _T("");

extern int     OldStatus;
extern int     GlobalStatus;

extern CRITICAL_SECTION m_resolve;

HWND     IgnoreWndHwnd = NULL;
UINT_PTR	KeepAliveTimer = 0;

static VOID CALLBACK IdentTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	KillChatTimer(IdentTimer);
	if(OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING) 
		return;

	if (g_ircSession && prefs->IdentTimer)
		g_ircSession.KillIdent();

	return;
}

static VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	KillChatTimer(InitTimer);
	if(OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING) 
		return;

	if ( prefs->ForceVisible )
		PostIrcMessage( _T("/MODE %s -i"), g_ircSession.GetInfo().sNick.c_str());

	if ( lstrlenA( prefs->MyHost ) == 0 && g_ircSession )
		DoUserhostWithReason(2, (_T("S")+ g_ircSession.GetInfo().sNick).c_str(), true, _T("%s"), g_ircSession.GetInfo().sNick.c_str());
}

VOID CALLBACK KeepAliveTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	if ( !prefs->SendKeepAlive || (OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING )) {
		KillChatTimer(KeepAliveTimer);
		return;
	}

	TCHAR temp2[270];
	if ( !g_ircSession.GetInfo().sServerName.empty())
		mir_sntprintf(temp2, SIZEOF(temp2), _T("PING %s"), g_ircSession.GetInfo().sServerName.c_str());
	else
		mir_sntprintf(temp2, SIZEOF(temp2), _T("PING %u"), time(0));

	if (g_ircSession)
		g_ircSession << CIrcMessage(temp2, g_ircSession.getCodepage(), false, false);
}

VOID CALLBACK OnlineNotifTimerProc3(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	if ( !prefs->ChannelAwayNotification || OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING || (!prefs->AutoOnlineNotification && !bTempForceCheck) || bTempDisableCheck) 
	{
		KillChatTimer(OnlineNotifTimer3);
		ChannelsToWho = _T("");
		return;
	}

	TString name = GetWord( ChannelsToWho.c_str(), 0 );
	if ( name.empty()) {
		ChannelsToWho = _T("");
		int count = (int)CallServiceSync(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)IRCPROTONAME);
		for ( int i = 0; i < count; i++ ) {
			GC_INFO gci = {0};
			gci.Flags = BYINDEX | NAME | TYPE | COUNT;
			gci.iItem = i;
			gci.pszModule = IRCPROTONAME;
			if ( !CallServiceSync( MS_GC_GETINFO, 0, (LPARAM)&gci ) && gci.iType == GCW_CHATROOM )
				if ( gci.iCount <= prefs->OnlineNotificationLimit )
					ChannelsToWho += (TString)gci.pszName + _T(" ");
	}	}

	if ( ChannelsToWho.empty()) {
		SetChatTimer( OnlineNotifTimer3, 60*1000, OnlineNotifTimerProc3 );
		return;
	}

	name = GetWord( ChannelsToWho.c_str(), 0 );
	DoUserhostWithReason(2, _T("S") + name, true, _T("%s"), name.c_str());
	TString temp = GetWordAddress( ChannelsToWho.c_str(), 1 );
	ChannelsToWho = temp;
	if ( iTempCheckTime )
		SetChatTimer( OnlineNotifTimer3, iTempCheckTime*1000, OnlineNotifTimerProc3 );
	else
		SetChatTimer( OnlineNotifTimer3, prefs->OnlineNotificationTime*1000, OnlineNotifTimerProc3 );
}

VOID CALLBACK OnlineNotifTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	if ( OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING || (!prefs->AutoOnlineNotification && !bTempForceCheck) || bTempDisableCheck ) 
	{
		KillChatTimer(OnlineNotifTimer);
		NamesToWho = _T("");
		return;
	}

	TString name = GetWord(NamesToWho.c_str(), 0);
	TString name2 = GetWord(NamesToUserhost.c_str(), 0);

	if ( name.empty() && name2.empty()) {
		DBVARIANT dbv;
		char* szProto;

		HANDLE hContact = (HANDLE) CallService( MS_DB_CONTACT_FINDFIRST, 0, 0);
		while ( hContact ) {
		   szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		   if ( szProto != NULL && !lstrcmpiA( szProto, IRCPROTONAME )) {
			   BYTE bRoom = DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0);
			   if ( bRoom == 0 ) {
				   BYTE bDCC = DBGetContactSettingByte(hContact, IRCPROTONAME, "DCC", 0);
				   BYTE bHidden = DBGetContactSettingByte(hContact,"CList", "Hidden", 0);
					if ( bDCC == 0 && bHidden == 0 ) {
						if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
							BYTE bAdvanced = DBGetContactSettingByte(hContact,IRCPROTONAME, "AdvancedMode", 0) ;
							if ( !bAdvanced ) {
								DBFreeVariant( &dbv );
								if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Nick", &dbv )) {	
									NamesToUserhost += (TString)dbv.ptszVal + _T(" ");
									DBFreeVariant( &dbv );
								}
							}
							else {
								DBFreeVariant( &dbv );
								DBVARIANT dbv2;
								
								TCHAR* DBNick = NULL;
								TCHAR* DBWildcard = NULL;
								if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Nick", &dbv ))
									DBNick = dbv.ptszVal;
								if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UWildcard", &dbv2 ))
									DBWildcard = dbv2.ptszVal;

								if ( DBNick && ( !DBWildcard || !WCCmp(CharLower(DBWildcard), CharLower(DBNick)))) 
									NamesToWho += (TString)DBNick + _T(" ");
								else if( DBWildcard )
									NamesToWho += (TString)DBWildcard + _T(" ");

								if ( DBNick )     DBFreeVariant(&dbv);
                        if ( DBWildcard ) DBFreeVariant(&dbv2);
			}	}	}	}	}

			hContact = (HANDLE) CallService( MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}	}

	if ( NamesToWho.empty() && NamesToUserhost.empty()) {
		SetChatTimer( OnlineNotifTimer, 60*1000, OnlineNotifTimerProc );
		return;
	}

	name = GetWord( NamesToWho.c_str(), 0);
	name2 = GetWord( NamesToUserhost.c_str(), 0);
	TString temp;
	if ( !name.empty()) {
		DoUserhostWithReason(2, _T("S") + name, true, _T("%s"), name.c_str());
		temp = GetWordAddress( NamesToWho.c_str(), 1 );
		NamesToWho = temp;
	}

	if ( !name2.empty()) {
		TString params;
		for ( int i = 0; i < 3; i++ ) {
			params = _T("");
			for ( int j = 0; j < 5; j++ ) 
				params += GetWord(NamesToUserhost.c_str(), i *5 + j) + _T(" ");

			if ( params[0] != ' ' )
				DoUserhostWithReason(1, (TString)_T("S") + params, true, params);
		}
		temp = GetWordAddress( NamesToUserhost.c_str(), 15 );
		NamesToUserhost = temp;
	}

	if ( iTempCheckTime )
		SetChatTimer( OnlineNotifTimer, iTempCheckTime*1000, OnlineNotifTimerProc );
	else
		SetChatTimer( OnlineNotifTimer, prefs->OnlineNotificationTime*1000, OnlineNotifTimerProc );
}

static int AddOutgoingMessageToDB(HANDLE hContact, TCHAR* msg)
{
	if ( OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING )
		return 0;

	TString S = DoColorCodes( msg, TRUE, FALSE );

	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	dbei.szModule = IRCPROTONAME;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.timestamp = (DWORD)time(NULL);
	#if defined( _UNICODE )
		dbei.flags = DBEF_SENT + DBEF_UTF;
		dbei.pBlob = ( PBYTE )mir_utf8encodeW( S.c_str());
	#else
		dbei.flags = DBEF_SENT;
		dbei.pBlob = ( PBYTE )S.c_str();
	#endif
	dbei.cbBlob = strlen(( char* )dbei.pBlob) + 1;
	CallService( MS_DB_EVENT_ADD, (WPARAM) hContact, (LPARAM) & dbei);
	#if defined( _UNICODE )
		mir_free( dbei.pBlob );
	#endif
	return 1;
}

void __cdecl ResolveIPThread(LPVOID di)
{
	EnterCriticalSection(&m_resolve);

	IPRESOLVE * ipr = (IPRESOLVE *) di;
	if ( ipr != NULL && (ipr->iType == IP_AUTO && lstrlenA(prefs->MyHost) == 0 || ipr->iType == IP_MANUAL )) {
		hostent* myhost = gethostbyname( ipr->sAddr.c_str() );
		if ( myhost ) {
			IN_ADDR in;
			memcpy( &in, myhost->h_addr, 4 );
			if ( ipr->iType == IP_AUTO )
				mir_snprintf( prefs->MyHost, sizeof( prefs->MyHost ), "%s", inet_ntoa( in ));
			else
				mir_snprintf( prefs->MySpecifiedHostIP, sizeof( prefs->MySpecifiedHostIP ), "%s", inet_ntoa( in ));
	}	}
	
	delete ipr;
	LeaveCriticalSection(&m_resolve);
}

DECLARE_IRC_MAP(CMyMonitor, CIrcDefaultMonitor)

CMyMonitor::CMyMonitor() : irc::CIrcDefaultMonitor(g_ircSession)
{
	IRC_MAP_ENTRY(CMyMonitor, "JOIN", OnIrc_JOIN)
	IRC_MAP_ENTRY(CMyMonitor, "QUIT", OnIrc_QUIT)
	IRC_MAP_ENTRY(CMyMonitor, "KICK", OnIrc_KICK)
	IRC_MAP_ENTRY(CMyMonitor, "MODE", OnIrc_MODE)
	IRC_MAP_ENTRY(CMyMonitor, "NICK", OnIrc_NICK)
	IRC_MAP_ENTRY(CMyMonitor, "PART", OnIrc_PART)
	IRC_MAP_ENTRY(CMyMonitor, "PRIVMSG", OnIrc_PRIVMSG)
	IRC_MAP_ENTRY(CMyMonitor, "TOPIC", OnIrc_TOPIC)
	IRC_MAP_ENTRY(CMyMonitor, "NOTICE", OnIrc_NOTICE)
	IRC_MAP_ENTRY(CMyMonitor, "PING", OnIrc_PINGPONG)
	IRC_MAP_ENTRY(CMyMonitor, "PONG", OnIrc_PINGPONG)
	IRC_MAP_ENTRY(CMyMonitor, "INVITE", OnIrc_INVITE)
	IRC_MAP_ENTRY(CMyMonitor, "ERROR", OnIrc_ERROR)
	IRC_MAP_ENTRY(CMyMonitor, "001", OnIrc_WELCOME)
	IRC_MAP_ENTRY(CMyMonitor, "002", OnIrc_YOURHOST)
	IRC_MAP_ENTRY(CMyMonitor, "005", OnIrc_SUPPORT)
	IRC_MAP_ENTRY(CMyMonitor, "223", OnIrc_WHOIS_OTHER)			//CodePage info
	IRC_MAP_ENTRY(CMyMonitor, "254", OnIrc_NOOFCHANNELS)
	IRC_MAP_ENTRY(CMyMonitor, "263", OnIrc_TRYAGAIN)
	IRC_MAP_ENTRY(CMyMonitor, "264", OnIrc_WHOIS_OTHER)			//Encryption info (SSL connect)
	IRC_MAP_ENTRY(CMyMonitor, "301", OnIrc_WHOIS_AWAY)
	IRC_MAP_ENTRY(CMyMonitor, "302", OnIrc_USERHOST_REPLY)
	IRC_MAP_ENTRY(CMyMonitor, "305", OnIrc_BACKFROMAWAY)
	IRC_MAP_ENTRY(CMyMonitor, "306", OnIrc_SETAWAY)
	IRC_MAP_ENTRY(CMyMonitor, "307", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY(CMyMonitor, "310", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY(CMyMonitor, "311", OnIrc_WHOIS_NAME)
	IRC_MAP_ENTRY(CMyMonitor, "312", OnIrc_WHOIS_SERVER)
	IRC_MAP_ENTRY(CMyMonitor, "313", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY(CMyMonitor, "315", OnIrc_WHO_END)
	IRC_MAP_ENTRY(CMyMonitor, "317", OnIrc_WHOIS_IDLE)
	IRC_MAP_ENTRY(CMyMonitor, "318", OnIrc_WHOIS_END)
	IRC_MAP_ENTRY(CMyMonitor, "319", OnIrc_WHOIS_CHANNELS)
	IRC_MAP_ENTRY(CMyMonitor, "320", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY(CMyMonitor, "321", OnIrc_LISTSTART)
	IRC_MAP_ENTRY(CMyMonitor, "322", OnIrc_LIST)
	IRC_MAP_ENTRY(CMyMonitor, "323", OnIrc_LISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "324", OnIrc_MODEQUERY)
	IRC_MAP_ENTRY(CMyMonitor, "330", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY(CMyMonitor, "332", OnIrc_INITIALTOPIC)
	IRC_MAP_ENTRY(CMyMonitor, "333", OnIrc_INITIALTOPICNAME)
	IRC_MAP_ENTRY(CMyMonitor, "352", OnIrc_WHO_REPLY)
	IRC_MAP_ENTRY(CMyMonitor, "353", OnIrc_NAMES)
	IRC_MAP_ENTRY(CMyMonitor, "366", OnIrc_ENDNAMES)
	IRC_MAP_ENTRY(CMyMonitor, "367", OnIrc_BANLIST)
	IRC_MAP_ENTRY(CMyMonitor, "368", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "346", OnIrc_BANLIST)
	IRC_MAP_ENTRY(CMyMonitor, "347", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "348", OnIrc_BANLIST)
	IRC_MAP_ENTRY(CMyMonitor, "349", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "371", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY(CMyMonitor, "376", OnIrc_ENDMOTD)
	IRC_MAP_ENTRY(CMyMonitor, "401", OnIrc_WHOIS_NO_USER)
	IRC_MAP_ENTRY(CMyMonitor, "403", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "416", OnIrc_WHOTOOLONG)
	IRC_MAP_ENTRY(CMyMonitor, "421", OnIrc_UNKNOWN)
	IRC_MAP_ENTRY(CMyMonitor, "422", OnIrc_ENDMOTD)
	IRC_MAP_ENTRY(CMyMonitor, "433", OnIrc_NICK_ERR)
	IRC_MAP_ENTRY(CMyMonitor, "471", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "473", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "474", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "475", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "671", OnIrc_WHOIS_OTHER)			//Encryption info (SSL connect)
}

CMyMonitor::~CMyMonitor()
{
}

bool CMyMonitor::OnIrc_WELCOME( const CIrcMessage* pmsg )
{
	CIrcDefaultMonitor::OnIrc_WELCOME(pmsg);

	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		static TCHAR host[1024];
		int i = 0;
		TString word = GetWord( pmsg->parameters[1].c_str(), i );
		while ( !word.empty()) {
			if ( _tcschr( word.c_str(), '!') && _tcschr( word.c_str(), '@' )) {
				lstrcpyn( host, word.c_str(), SIZEOF(host));
				TCHAR* p1 = _tcschr( host, '@' );
				if ( p1 )
					mir_forkthread( ResolveIPThread, new IPRESOLVE( _T2A(p1+1), IP_AUTO ));
			}
			
			word = GetWord(pmsg->parameters[1].c_str(), ++i);
	}	}			
		
	ShowMessage( pmsg ); 
	return true;
}

bool CMyMonitor::OnIrc_WHOTOOLONG( const CIrcMessage* pmsg )
{
	TString command = GetNextUserhostReason(2);
	if ( command[0] == 'U' )
		ShowMessage( pmsg ); 

	return true;
}

bool CMyMonitor::OnIrc_BACKFROMAWAY( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		int Temp = OldStatus;
		OldStatus = ID_STATUS_ONLINE;
		ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_ONLINE);

		if ( prefs->Perform )
			DoPerform( "Event: Available" );
	}			
		
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_SETAWAY( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		int Temp = OldStatus;
		OldStatus =  ID_STATUS_AWAY;
		ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_AWAY);

		if ( prefs->Perform ) {
			switch ( GlobalStatus ) {
			case ID_STATUS_AWAY:
				DoPerform( "Event: Away" );
				break;
			case ID_STATUS_NA:
				DoPerform( "Event: N/A" );
				break;
			case ID_STATUS_DND:
				DoPerform( "Event: DND" );
				break;
			case ID_STATUS_OCCUPIED:
				DoPerform( "Event: Occupied" );
				break;
			case ID_STATUS_OUTTOLUNCH:
				DoPerform( "Event: Out for lunch" );
				break;
			case ID_STATUS_ONTHEPHONE:
				DoPerform( "Event: On the phone" );
				break;
			default:
				GlobalStatus = ID_STATUS_AWAY;
				DoPerform( "Event: Away" );
				break;
	}	}	}	
		
	ShowMessage( pmsg ); 
	return true;
}

bool CMyMonitor::OnIrc_JOIN( const CIrcMessage* pmsg )
{
	if (pmsg->parameters.size() > 0 && pmsg->m_bIncoming && pmsg->prefix.sNick != g_ircSession.GetInfo().sNick) {
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_JOIN, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), NULL, _T("Normal"), host.c_str(), NULL, true, false); 
		DoEvent(GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[0].c_str(),pmsg->prefix.sNick.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE); 
	}
	else ShowMessage( pmsg ); 

	return true;
}

bool CMyMonitor::OnIrc_QUIT( const CIrcMessage* pmsg )
{
	if (pmsg->m_bIncoming) 
	{
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_QUIT, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters.size()>0?pmsg->parameters[0].c_str():NULL, NULL, host.c_str(), NULL, true, false);
		struct CONTACT user = { (LPTSTR)pmsg->prefix.sNick.c_str(), (LPTSTR)pmsg->prefix.sUser.c_str(), (LPTSTR)pmsg->prefix.sHost.c_str(), false, false, false};
		CList_SetOffline( &user );
		if ( pmsg->prefix.sNick == g_ircSession.GetInfo().sNick ) {
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			gce.cbSize = sizeof(GCEVENT);
			gcd.pszID = NULL;
			gcd.pszModule = IRCPROTONAME;
			gcd.iType = GC_EVENT_CONTROL;
			gce.pDest = &gcd;
			CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
		}
	}
	else ShowMessage( pmsg );

	return true;
}

bool CMyMonitor::OnIrc_PART( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters.size() > 0 && pmsg->m_bIncoming ) {
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_PART, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters.size()>1?pmsg->parameters[1].c_str():NULL, NULL, host.c_str(), NULL, true, false); 
		if ( pmsg->prefix.sNick == g_ircSession.GetInfo().sNick ) {
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			TString S = MakeWndID( pmsg->parameters[0].c_str());
			gce.cbSize = sizeof(GCEVENT);
			gcd.ptszID = ( TCHAR* )S.c_str();
			gce.dwFlags = GC_TCHAR;
			gcd.pszModule = IRCPROTONAME;
			gcd.iType = GC_EVENT_CONTROL;
			gce.pDest = &gcd;
			CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
		}
	}
	else ShowMessage( pmsg ); 

	return true;
}

bool CMyMonitor::OnIrc_KICK( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 )
		DoEvent( GC_EVENT_KICK, pmsg->parameters[0].c_str(), pmsg->parameters[1].c_str(), pmsg->parameters.size()>2?pmsg->parameters[2].c_str():NULL, pmsg->prefix.sNick.c_str(), NULL, NULL, true, false); 
	else
		ShowMessage( pmsg ); 

	if ( pmsg->parameters[1] == g_ircSession.GetInfo().sNick ) {
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		TString S = MakeWndID( pmsg->parameters[0].c_str() );
		gce.cbSize = sizeof(GCEVENT);
		gce.dwFlags = GC_TCHAR;
		gcd.ptszID = ( TCHAR* )S.c_str();
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);

		if ( prefs->RejoinIfKicked ) {
			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if ( wi && wi->pszPassword )
				PostIrcMessage( _T("/JOIN %s %s"), pmsg->parameters[0].c_str(), wi->pszPassword);
			else
				PostIrcMessage( _T("/JOIN %s"), pmsg->parameters[0].c_str());
	}	}

	return true;
}

bool CMyMonitor::OnIrc_MODEQUERY( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters.size() > 2 && pmsg->m_bIncoming && IsChannel( pmsg->parameters[1] )) {
		TString sPassword = _T("");
		TString sLimit = _T("");
		bool bAdd = false;
		int iParametercount = 3;

		LPCTSTR p1 = pmsg->parameters[2].c_str();
		while ( *p1 != '\0' ) {
			if ( *p1 == '+' )
				bAdd = true;
			if ( *p1 == '-' )
				bAdd = false;
			if ( *p1 == 'l' && bAdd ) {
				if (( int )pmsg->parameters.size() > iParametercount )
					sLimit = pmsg->parameters[ iParametercount ];
				iParametercount++;
			}
			if ( *p1 == 'k' && bAdd ) {
				if (( int )pmsg->parameters.size() > iParametercount )
					sPassword = pmsg->parameters[ iParametercount ];
				iParametercount++;
			}

			p1++;
		}

		AddWindowItemData( pmsg->parameters[1].c_str(), sLimit.empty() ? 0 : sLimit.c_str(), pmsg->parameters[2].c_str(), sPassword.empty() ? 0 : sPassword.c_str(), 0 );
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_MODE( const CIrcMessage* pmsg )
{	
	bool flag = false; 
	bool bContainsValidModes = false;
	TString sModes = _T("");
	TString sParams = _T("");

	if ( pmsg->parameters.size() > 1 && pmsg->m_bIncoming ) {
		if ( IsChannel( pmsg->parameters[0] )) {
			bool bAdd = false;
			int  iParametercount = 2;
			LPCTSTR p1 = pmsg->parameters[1].c_str();

			while ( *p1 != '\0' ) {
				if ( *p1 == '+' ) {
					bAdd = true;
					sModes += _T("+");
				}
				if ( *p1 == '-' ) {
					bAdd = false;
					sModes += _T("-");
				}
				if ( *p1 == 'l' && bAdd && iParametercount < (int)pmsg->parameters.size()) {
					bContainsValidModes = true;
					sModes += _T("l");
					sParams += _T(" ") + pmsg->parameters[iParametercount];
					iParametercount++;
				}
				if ( *p1 == 'b' || *p1 == 'k' && iParametercount < (int)pmsg->parameters.size()) {
					bContainsValidModes = true;
					sModes += *p1;
					sParams += _T(" ") + pmsg->parameters[iParametercount];
					iParametercount++;
				}
				if ( strchr( sUserModes.c_str(), (char)*p1 )) {
					TString sStatus = ModeToStatus( *p1 );
					if (( int )pmsg->parameters.size() > iParametercount ) {	
						if ( !_tcscmp(pmsg->parameters[2].c_str(), g_ircSession.GetInfo().sNick.c_str() )) {
							char cModeBit = -1;
							CHANNELINFO* wi = (CHANNELINFO *)DoEvent( GC_EVENT_GETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, NULL, false, false, 0 );
							switch (*p1) {
								case 'v':      cModeBit = 0;       break;
								case 'h':      cModeBit = 1;       break;
								case 'o':      cModeBit = 2;       break;
								case 'a':      cModeBit = 3;       break;
								case 'q':      cModeBit = 4;       break;
							}

							// set bit for own mode on this channel (voice/hop/op/admin/owner)
							if ( bAdd && cModeBit >= 0 )
								wi->OwnMode |= ( 1 << cModeBit );
							else
								wi->OwnMode &= ~( 1 << cModeBit );

							DoEvent( GC_EVENT_SETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, (DWORD)wi, false, false, 0 );
						}
						DoEvent( bAdd ? GC_EVENT_ADDSTATUS : GC_EVENT_REMOVESTATUS, pmsg->parameters[0].c_str(), pmsg->parameters[iParametercount].c_str(), pmsg->prefix.sNick.c_str(), sStatus.c_str(), NULL, NULL, prefs->OldStyleModes?false:true, false); 
						iParametercount++;
					}
				}
				else if (*p1 != 'b' && *p1 != ' ' && *p1 != '+' && *p1 != '-' ) {
					bContainsValidModes = true;
					if(*p1 != 'l' && *p1 != 'k')
						sModes += *p1;
					flag = true;
				}

				p1++;
			}

			if ( prefs->OldStyleModes ) {
				TCHAR temp[256];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("%s sets mode %s"), 
					pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());
				
				TString sMessage = temp;
				for ( int i=2; i < (int)pmsg->parameters.size(); i++ )
					sMessage = sMessage  + _T(" ") + pmsg->parameters[i];

				DoEvent( GC_EVENT_INFORMATION, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), sMessage.c_str(), NULL, NULL, NULL, true, false ); 
			}
			else if ( bContainsValidModes ) {
				for ( int i = iParametercount; i < (int)pmsg->parameters.size(); i++ )
					sParams += _T(" ") + pmsg->parameters[i];

				TCHAR temp[4000];
				mir_sntprintf( temp, 3999, TranslateT(	"%s sets mode %s%s" ), pmsg->prefix.sNick.c_str(), sModes.c_str(), sParams.c_str());
				DoEvent(GC_EVENT_INFORMATION, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
			}

			if ( flag )
				PostIrcMessage( _T("/MODE %s"), pmsg->parameters[0].c_str());
		}
		else {
			TCHAR temp[256];
			mir_sntprintf( temp, SIZEOF(temp), TranslateT("%s sets mode %s"), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());

			TString sMessage = temp;
			for ( int i=2; i < (int)pmsg->parameters.size(); i++ )
				sMessage = sMessage  + _T(" ") + pmsg->parameters[i];

			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, pmsg->prefix.sNick.c_str(), sMessage.c_str(), NULL, NULL, NULL, true, false); 
		}
	}
	else ShowMessage( pmsg ); 

	return true;
}

bool CMyMonitor::OnIrc_NICK( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 0 ) {
		bool bIsMe = pmsg->prefix.sNick.c_str() == g_ircSession.GetInfo().sNick ? true : false;
		CIrcDefaultMonitor::OnIrc_NICK( pmsg );
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_NICK, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, host.c_str(), NULL, true, bIsMe); 
		DoEvent(GC_EVENT_CHUID, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, NULL, NULL, true, false); 
		
		struct CONTACT user = { (TCHAR*)pmsg->prefix.sNick.c_str(), (TCHAR*)pmsg->prefix.sUser.c_str(), (TCHAR*)pmsg->prefix.sHost.c_str(), false, false, false};
		HANDLE hContact = CList_FindContact(&user);
		if (hContact) {
			if (DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
				DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE);
			DBWriteContactSettingTString(hContact, IRCPROTONAME, "Nick", pmsg->parameters[0].c_str());
			DBWriteContactSettingTString(hContact, IRCPROTONAME, "User", pmsg->prefix.sUser.c_str());
			DBWriteContactSettingTString(hContact, IRCPROTONAME, "Host", pmsg->prefix.sHost.c_str());
		}
	}
	else ShowMessage( pmsg );

	return true;
}

bool CMyMonitor::OnIrc_NOTICE( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		if ( IsCTCP( pmsg ))
			return true;

		if ( !prefs->Ignore || !IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'n' )) {
			TString S;
			TString S2;
			TString S3;
			if ( pmsg->prefix.sNick.length() > 0 )
				S = pmsg->prefix.sNick;
			else
				S = m_session.GetInfo().sNetwork;
			S3 = m_session.GetInfo().sNetwork;
			if ( IsChannel( pmsg->parameters[0] ))
				S2 = pmsg->parameters[0].c_str();
			else {
				GC_INFO gci = {0};
				gci.Flags = BYID | TYPE;
				gci.pszModule = IRCPROTONAME;

				TString S3 = GetWord( pmsg->parameters[1].c_str(), 0);
				if ( S3[0] == '[' && S3[1] == '#' && S3[S3.length()-1] == ']' ) {
					S3.erase(S3.length()-1, 1);
					S3.erase(0,1);
					TString Wnd = MakeWndID( S3.c_str());
					gci.pszID = ( TCHAR* )Wnd.c_str();
					if ( !CallServiceSync( MS_GC_GETINFO, 0, (LPARAM)&gci ) && gci.iType == GCW_CHATROOM )
						S2 = GetWord( gci.pszID, 0 );
					else
						S2 = _T("");
				}
				else S2 = _T("");
			}
			DoEvent(GC_EVENT_NOTICE, S2.empty() ? 0 : S2.c_str(), S.c_str(), pmsg->parameters[1].c_str(), NULL, S3.c_str(), NULL, true, false); 
		}
	}
	else ShowMessage( pmsg );

	return true;
}

bool CMyMonitor::OnIrc_YOURHOST( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) 
		CIrcDefaultMonitor::OnIrc_YOURHOST( pmsg );
	
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_INVITE( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && ( prefs->Ignore && IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'i' )))
		return true;

	if ( pmsg->m_bIncoming && prefs->JoinOnInvite && pmsg->parameters.size() >1 && lstrcmpi(pmsg->parameters[0].c_str(), m_session.GetInfo().sNick.c_str()) == 0 ) 
		PostIrcMessage( _T("/JOIN %s"), pmsg->parameters[1].c_str());

	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_PINGPONG( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->sCommand == _T("PING"))
		CIrcDefaultMonitor::OnIrc_PING(pmsg);

	return true;
}

bool CMyMonitor::OnIrc_PRIVMSG( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters.size() > 1 ) {
		if ( IsCTCP( pmsg ))
			return true;

		TString mess = pmsg->parameters[1];
		bool bIsChannel = IsChannel(pmsg->parameters[0]);

		if ( pmsg->m_bIncoming && !bIsChannel ) {
			CCSDATA ccs = {0}; 
			PROTORECVEVENT pre;

			mess = DoColorCodes( mess.c_str(), TRUE, FALSE );
			ccs.szProtoService = PSR_MESSAGE;

			struct CONTACT user = { (TCHAR*)pmsg->prefix.sNick.c_str(), (TCHAR*)pmsg->prefix.sUser.c_str(), (TCHAR*)pmsg->prefix.sHost.c_str(), false, false, false};

			if ( CallService( MS_IGNORE_ISIGNORED, NULL, IGNOREEVENT_MESSAGE )) 
				if ( !CList_FindContact( &user ))
					return true;

			if (( prefs->Ignore && IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q' ))) {
				HANDLE hContact = CList_FindContact( &user );
				if ( !hContact || ( hContact && DBGetContactSettingByte( hContact,"CList", "Hidden", 0) == 1 ))
					return true;
			}

			ccs.hContact = CList_AddContact( &user, false, true );
			ccs.lParam = (LPARAM)&pre;
			pre.timestamp = (DWORD)time(NULL);
			#if defined( _UNICODE )
				pre.flags = PREF_UTF;
				pre.szMessage = mir_utf8encodeW( mess.c_str());
			#else
				pre.szMessage = ( char* )mess.c_str();
			#endif
			DBWriteContactSettingTString(ccs.hContact, IRCPROTONAME, "User", pmsg->prefix.sUser.c_str());
			DBWriteContactSettingTString(ccs.hContact, IRCPROTONAME, "Host", pmsg->prefix.sHost.c_str());
			CallService( MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			#if defined( _UNICODE )
				mir_free( pre.szMessage );
			#endif
			return true;
		}
		
		if ( bIsChannel ) {
			if ( !(pmsg->m_bIncoming && prefs->Ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm' ))) {
				if ( !pmsg->m_bIncoming )
					ReplaceString( mess, _T("%%"), _T("%"));
				DoEvent(GC_EVENT_MESSAGE, pmsg->parameters[0].c_str(), pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():m_session.GetInfo().sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 				
			}
			return true;
	}	}

	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::IsCTCP( const CIrcMessage* pmsg )
{
	// is it a ctcp command, i e is the first and last characer of a PRIVMSG or NOTICE text ASCII 1
	if ( pmsg->parameters[1].length() > 3 && pmsg->parameters[1][0] == 1 && pmsg->parameters[1][pmsg->parameters[1].length()-1] == 1 ) {
		// set mess to contain the ctcp command, excluding the leading and trailing  ASCII 1
		TString mess = pmsg->parameters[1];
		mess.erase(0,1);
		mess.erase(mess.length()-1,1);
		
		// exploit???
		if ( mess.find(1) != string::npos || mess.find( _T("%newl")) != string::npos ) {
			TCHAR temp[4096];
			mir_sntprintf(temp, SIZEOF(temp), TranslateT( "CTCP ERROR: Malformed CTCP command received from %s!%s@%s. Possible attempt to take control of your irc client registered"), pmsg->prefix.sNick.c_str(), pmsg->prefix.sUser.c_str(), pmsg->prefix.sHost.c_str());
			DoEvent( GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
			return true;
		}

		// extract the type of ctcp command
		TString ocommand = GetWord(mess.c_str(), 0);
		TString command = GetWord(mess.c_str(), 0);
		transform (command.begin(),command.end(), command.begin(), tolower);

		// should it be ignored?
		if ( prefs->Ignore ) {
			if ( IsChannel(pmsg->parameters[0] )) {
				if ( command == _T("action") && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm'))
					return true;
			}
			else {
				if ( command == _T("action")) {
					if ( IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q' ))
						return true;
				}
				else if ( command == _T("dcc")) {
					if ( IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'd' ))
						return true;
				}
				else if ( IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'c' ))
					return true;
		}	}

		if ( pmsg->sCommand == _T("PRIVMSG")) {
			// incoming ACTION
			if ( command == _T("action")) {
				mess.erase(0,6);

				if ( IsChannel( pmsg->parameters[0] )) {
					if ( mess.length() > 1 ) {
						mess.erase(0,1);
						if ( !pmsg->m_bIncoming )
							ReplaceString(mess, _T("%%"), _T("%"));

						DoEvent(GC_EVENT_ACTION, pmsg->parameters[0].c_str(), pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():m_session.GetInfo().sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 
					}
				}
				else if (pmsg->m_bIncoming)
				{
					mess.insert(0, pmsg->prefix.sNick.c_str());
					mess.insert(0, _T("* "));
					mess.insert(mess.length(), _T(" *"));
					CIrcMessage msg = *pmsg;
					msg.parameters[1] = mess;
					OnIrc_PRIVMSG(&msg);
				}
			}
			// incoming FINGER
			else if (pmsg->m_bIncoming && command == _T("finger")) {
				PostIrcMessage( _T("/NOTICE %s \001FINGER %s (%s)\001"), pmsg->prefix.sNick.c_str(), prefs->Name, prefs->UserID);
				
				TCHAR temp[300];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("CTCP FINGER requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming VERSION
			else if (pmsg->m_bIncoming && command == _T("version")) {
				PostIrcMessage( _T("/NOTICE %s \001VERSION Miranda IM %s (IRC v.%s%s), (c) 2003-07 J.Persson, G.Hazan\001"), 
					pmsg->prefix.sNick.c_str(), _T("%mirver"), _T("%version"),
					#if defined( _UNICODE )
						_T(" Unicode"));
					#else
						"" );
					#endif

				TCHAR temp[300];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("CTCP VERSION requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming SOURCE
			else if (pmsg->m_bIncoming && command == _T("source")) {
				PostIrcMessage( _T("/NOTICE %s \001SOURCE Get Miranda IRC here: http://miranda-im.org/ \001"), pmsg->prefix.sNick.c_str());
				
				TCHAR temp[300];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("CTCP SOURCE requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming USERINFO
			else if (pmsg->m_bIncoming && command == _T("userinfo")) {
				PostIrcMessage( _T("/NOTICE %s \001USERINFO %s\001"), pmsg->prefix.sNick.c_str(), prefs->UserInfo );
				
				TCHAR temp[300];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("CTCP USERINFO requested by %s") , pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming PING
			else if (pmsg->m_bIncoming && command == _T("ping")) {
				PostIrcMessage( _T("/NOTICE %s \001%s\001"), pmsg->prefix.sNick.c_str(), mess.c_str());
				
				TCHAR temp[300];
				mir_sntprintf( temp, SIZEOF(temp), TranslateT("CTCP PING requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming TIME
			else if (pmsg->m_bIncoming && command == _T("time")) {
				TCHAR temp[300];
				time_t tim = time(NULL);
				lstrcpyn( temp, _tctime( &tim ), 25 );
				PostIrcMessage( _T("/NOTICE %s \001TIME %s\001"), pmsg->prefix.sNick.c_str(), temp);
				
				mir_sntprintf(temp, SIZEOF(temp), TranslateT("CTCP TIME requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming DCC request... lots of stuff happening here...
			else if (pmsg->m_bIncoming && command == _T("dcc")) {
				TString type = GetWord(mess.c_str(), 1);
				transform (type.begin(),type.end(), type.begin(), tolower);

				// components of a dcc message
				TString sFile = _T("");
				DWORD dwAdr = 0;
				int iPort = 0;
				DWORD dwSize = 0;
				TString sToken = _T("");
				bool bIsChat = ( type == _T("chat"));

				// 1. separate the dcc command into the correct pieces
				if ( bIsChat || type == _T("send")) {
					// if the filename is surrounded by quotes, do this
					if ( GetWord(mess.c_str(), 2)[0] == '\"' ) {
						int end = 0;
						int begin = mess.find('\"', 0);
						if ( begin >= 0 ) {
							end = mess.find('\"', begin + 1); 
							if ( end >= 0 ) {
								sFile = mess.substr(begin+1, end-begin-1);

								begin = mess.find(' ', end);
								if ( begin >= 0 ) {
									TString rest = mess.substr(begin, mess.length());
									dwAdr = _ttoi(GetWord(rest.c_str(), 0).c_str());
									iPort = _ttoi(GetWord(rest.c_str(), 1).c_str());
									dwSize = _ttoi(GetWord(rest.c_str(), 2).c_str());
									sToken = GetWord(rest.c_str(), 3);
						}	}	}
					}
					// ... or try another method of separating the dcc command
					else if ( !GetWord(mess.c_str(), (bIsChat) ? 4 : 5 ).empty() ) {
						int index = (bIsChat) ? 4 : 5;
						bool bFlag = false;

						// look for the part of the ctcp command that contains adress, port and size
						while ( !bFlag && !GetWord(mess.c_str(), index).empty() ) {
							TString sTemp;
							
							if ( type == _T("chat"))
								sTemp = GetWord(mess.c_str(), index-1) + GetWord(mess.c_str(), index);
							else	
								sTemp = GetWord(mess.c_str(), index-2) + GetWord(mess.c_str(), index-1) + GetWord(mess.c_str(), index);
							
							// if all characters are number it indicates we have found the adress, port and size parameters
							int ind = 0;
							while ( sTemp[ind] != '\0' ) {
								if( !_istdigit( sTemp[ind] ))
									break;
								ind++;
							}
							
							if ( sTemp[ind] == '\0' && GetWord( mess.c_str(), index + ((bIsChat) ? 1 : 2 )).empty() )
								bFlag = true;
							index++;
						}
						
						if ( bFlag ) {
							TCHAR* p1 = _tcsdup( GetWordAddress(mess.c_str(), 2 ));
							TCHAR* p2 = GetWordAddress( p1, index-5 );
							
							if ( type == _T("send")) {
								if ( p2 > p1 ) {
									p2--;
									while( p2 != p1 && *p2 == ' ' ) {
										*p2 = '\0';
										p2--;
									}
									sFile = p1;
								}
							}
							else sFile = _T("chat");

							free( p1 );

							dwAdr = _ttoi(GetWord(mess.c_str(), index - (bIsChat?2:3)).c_str());
							iPort = _ttoi(GetWord(mess.c_str(), index - (bIsChat?1:2)).c_str());
							dwSize = _ttoi(GetWord(mess.c_str(), index-1).c_str());
							sToken = GetWord(mess.c_str(), index);
					}	} 
				}
				else if (type == _T("accept") || type == _T("resume")) {
					// if the filename is surrounded by quotes, do this
					if ( GetWord(mess.c_str(), 2)[0] == '\"' ) {
						int end = 0;
						int begin = mess.find('\"', 0);
						if ( begin >= 0 ) {
							end = mess.find('\"', begin + 1); 
							if ( end >= 0 ) {
								sFile = mess.substr(begin+1, end);

								begin = mess.find(' ', end);
								if ( begin >= 0 ) {
									TString rest = mess.substr(begin, mess.length());
									iPort = _ttoi(GetWord(rest.c_str(), 0).c_str());
									dwSize = _ttoi(GetWord(rest.c_str(), 1).c_str());
									sToken = GetWord(rest.c_str(), 2);
						}	}	}
					}
					// ... or try another method of separating the dcc command
					else if ( !GetWord(mess.c_str(), 4).empty() ) {
						int index = 4;
						bool bFlag = false;

						// look for the part of the ctcp command that contains adress, port and size
						while ( !bFlag && !GetWord(mess.c_str(), index).empty() ) {
							TString sTemp = GetWord(mess.c_str(), index-1) + GetWord(mess.c_str(), index);
							
							// if all characters are number it indicates we have found the adress, port and size parameters
							int ind = 0;

							while ( sTemp[ind] != '\0' ) {
								if( !_istdigit( sTemp[ind] ))
									break;
								ind++;
							}
							
							if ( sTemp[ind] == '\0' && GetWord(mess.c_str(), index + 2).empty() )
								bFlag = true;
							index++;
						}
						if ( bFlag ) {
							TCHAR* p1 = _tcsdup(GetWordAddress(mess.c_str(), 2));
							TCHAR* p2 = GetWordAddress(p1, index-4);
							
							if ( p2 > p1 ) {
								p2--;
								while( p2 != p1 && *p2 == ' ' ) {
									*p2 = '\0';
									p2--;
								}
								sFile = p1;
							}

							free( p1 );

							iPort = _ttoi(GetWord(mess.c_str(), index-2).c_str());
							dwSize = _ttoi(GetWord(mess.c_str(), index-1).c_str());
							sToken = GetWord(mess.c_str(), index);
				}	}	} 
				// end separating dcc commands

				// 2. Check for malformed dcc commands or other errors
				if ( bIsChat || type == _T("send")) {
					TCHAR szTemp[256];
					szTemp[0] = '\0';

					unsigned long ulAdr = 0;
					if ( prefs->ManualHost )
						ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
					else
						ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

					if ( bIsChat && !prefs->DCCChatEnabled)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: Chat request from %s denied"),pmsg->prefix.sNick.c_str());

					else if(type == _T("send") && !prefs->DCCFileEnabled)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: File transfer request from %s denied"),pmsg->prefix.sNick.c_str());

					else if(type == _T("send") && !iPort && ulAdr == 0)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: Reverse file transfer request from %s denied [No local IP]"),pmsg->prefix.sNick.c_str());

					if ( sFile.empty() || dwAdr == 0 || dwSize == 0 || iPort == 0 && sToken.empty())
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Malformed CTCP request from %s [%s]"),pmsg->prefix.sNick.c_str(), mess.c_str());
				
					if ( szTemp[0] ) {
						DoEvent( GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
						return true;
					}

					// remove path from the filename if the remote client (stupidly) sent it
					TString sFileCorrected = sFile;
					int i = sFile.rfind( _T("\\"), sFile.length());
					if (i != string::npos )
						sFileCorrected = sFile.substr(i+1, sFile.length());
					sFile = sFileCorrected;
				}
				else if ( type == _T("accept") || type == _T("resume")) {
					TCHAR szTemp[256];
					szTemp[0] = '\0';

					if ( type == _T("resume") && !prefs->DCCFileEnabled)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: File transfer resume request from %s denied"),pmsg->prefix.sNick.c_str());

					if ( sToken.empty() && iPort == 0 || sFile.empty())
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Malformed CTCP request from %s [%s]"),pmsg->prefix.sNick.c_str(), mess.c_str());
				
					if ( szTemp[0] ) {
						DoEvent( GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
						return true;
					}

					// remove path from the filename if the remote client (stupidly) sent it
					TString sFileCorrected = sFile;
					int i = sFile.rfind( _T("\\"), sFile.length());
					if ( i != string::npos )
						sFileCorrected = sFile.substr(i+1, sFile.length());
					sFile = sFileCorrected;
				}

				// 3. Take proper actions considering type of command

				// incoming chat request
				if ( bIsChat ) {
					CONTACT user = { (TCHAR*)pmsg->prefix.sNick.c_str(), 0, 0, false, false, true};
					HANDLE hContact = CList_FindContact( &user );

					// check if it should be ignored
					if ( prefs->DCCChatIgnore == 1 || 
						prefs->DCCChatIgnore == 2 && hContact && 
						DBGetContactSettingByte(hContact,"CList", "NotOnList", 0) == 0 && 
						DBGetContactSettingByte(hContact,"CList", "Hidden", 0) == 0)
					{
						TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
						CList_AddDCCChat(pmsg->prefix.sNick, host, dwAdr, iPort); // add a CHAT event to the clist
					}
					else {
						TCHAR szTemp[512];
						mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("DCC: Chat request from %s denied"),pmsg->prefix.sNick.c_str());
						DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
				}	}

				// remote requested that the file should be resumed
				if ( type == _T("resume")) {
					CDccSession* dcc;
					if ( sToken.empty())
						dcc = g_ircSession.FindDCCSendByPort( iPort );
					else
						dcc = g_ircSession.FindPassiveDCCSend( _ttoi( sToken.c_str())); // reverse ft

					if ( dcc ) {
						InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME);
						InterlockedExchange(&dcc->dwResumePos, dwSize); // dwSize is the resume position
						PostIrcMessage( _T("/PRIVMSG %s \001DCC ACCEPT %s\001"), pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 2));
				}	}

				// remote accepted your request for a file resume
				if ( type == _T("accept")) {
					CDccSession* dcc;
					if ( sToken.empty())
						dcc = g_ircSession.FindDCCRecvByPortAndName(iPort, pmsg->prefix.sNick.c_str());
					else
						dcc = g_ircSession.FindPassiveDCCRecv(pmsg->prefix.sNick, sToken); // reverse ft

					if ( dcc ) {
						InterlockedExchange( &dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME );
						InterlockedExchange( &dcc->dwResumePos, dwSize );	// dwSize is the resume position					
						SetEvent( dcc->hEvent );
				}	}

				if ( type == _T("send")) {
					TString sTokenBackup = sToken;
					bool bTurbo = false; // TDCC indicator

					if ( !sToken.empty() && sToken[sToken.length()-1] == 'T' ) {
						bTurbo = true;
						sToken.erase(sToken.length()-1,1);
					}

					// if a token exists and the port is non-zero it is the remote
					// computer telling us that is has accepted to act as server for
					// a reverse filetransfer. The plugin should connect to that computer
					// and start sedning the file (if the token is valid). Compare to DCC RECV
					if ( !sToken.empty() && iPort ) {
						CDccSession* dcc = g_ircSession.FindPassiveDCCSend( _ttoi( sToken.c_str()));
						if ( dcc ) {
							dcc->SetupPassive( dwAdr, iPort );
							dcc->Connect();
						}
					}
					else {
						struct CONTACT user = { (TCHAR*)pmsg->prefix.sNick.c_str(), (TCHAR*)pmsg->prefix.sUser.c_str(), (TCHAR*)pmsg->prefix.sHost.c_str(), false, false, false};
						if ( CallService( MS_IGNORE_ISIGNORED, NULL, IGNOREEVENT_FILE )) 
							if ( !CList_FindContact( &user ))
								return true;

						HANDLE hContact = CList_AddContact( &user, false, true );
						if ( hContact ) {
							char* szFileName = mir_t2a( sFile.c_str() );

							DCCINFO* di = new DCCINFO;
							ZeroMemory(di, sizeof(DCCINFO));
							di->hContact = hContact;
							di->sFile = sFile;
							di->dwSize = dwSize;
							di->sContactName = pmsg->prefix.sNick;
							di->dwAdr = dwAdr;
							di->iPort = iPort;
							di->iType = DCC_SEND;
							di->bSender = false;
							di->bTurbo = bTurbo;
							di->bSSL = false;
							di->bReverse = (iPort == 0 && !sToken.empty() ) ? true : false;
							if( di->bReverse )
								di->sToken = sTokenBackup;

							CCSDATA ccs = {0}; 
							PROTORECVEVENT pre = {0};
							ccs.szProtoService = PSR_FILE;
							pre.flags = 0;
							pre.timestamp = (DWORD)time(NULL);

							char* szBlob = ( char* ) malloc(sizeof(DWORD) + strlen(szFileName) + 3);
							*((PDWORD) szBlob) = (DWORD) di;
							strcpy(szBlob + sizeof(DWORD), szFileName );
							strcpy(szBlob + sizeof(DWORD) + strlen(szFileName) + 1, " ");

							DBWriteContactSettingTString(hContact, IRCPROTONAME, "User", pmsg->prefix.sUser.c_str());
							DBWriteContactSettingTString(hContact, IRCPROTONAME, "Host", pmsg->prefix.sHost.c_str());

							pre.szMessage = szBlob;
							ccs.hContact = hContact;
							ccs.lParam = (LPARAM) & pre;
							CallService( MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs );

							mir_free( szFileName );
				}	}	}
				// end type == "send"
			}
			else if ( pmsg->m_bIncoming ) {
				TCHAR temp[300];
				mir_sntprintf(temp, SIZEOF(temp), TranslateT("CTCP %s requested by %s"), ocommand.c_str(), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false); 
		}	}

		// handle incoming ctcp in notices. This technique is used for replying to CTCP queries
		else if(pmsg->sCommand == _T("NOTICE")) {
			TCHAR szTemp[300]; 
			szTemp[0] = '\0';

			//if we got incoming CTCP Version for contact in CList - then write its as MirVer for that contact!
			if (pmsg->m_bIncoming && command == _T("version"))
				{
				struct CONTACT user = { (TCHAR*)pmsg->prefix.sNick.c_str(), (TCHAR*)pmsg->prefix.sUser.c_str(), (TCHAR*)pmsg->prefix.sHost.c_str(), false, false, false};
				HANDLE hContact = CList_FindContact(&user);
				if (hContact) 
					DBWriteContactSettingTString( hContact, IRCPROTONAME, "MirVer", DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE)); 
				}

			// if the whois window is visible and the ctcp reply belongs to the user in it, then show the reply in the whois window
			if ( whois_hWnd && IsWindowVisible( whois_hWnd )) {
				GetDlgItemText( whois_hWnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
				if ( lstrcmpi(szTemp, pmsg->prefix.sNick.c_str()) == 0 ) {
					if ( pmsg->m_bIncoming && (command == _T("version") || command == _T("userinfo") || command == _T("time"))) {
						SetActiveWindow( whois_hWnd );
						SetDlgItemText( whois_hWnd, IDC_REPLY, DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE));
						return true;
					}
					if (pmsg->m_bIncoming && command == _T("ping")) {
						SetActiveWindow( whois_hWnd );
						int s = (int)time(0) - (int)_ttol(GetWordAddress(mess.c_str(), 1));
						TCHAR szTemp[30];
						if ( s == 1 )
							mir_sntprintf( szTemp, SIZEOF(szTemp), _T("%u second"), s );
						else
							mir_sntprintf( szTemp, SIZEOF(szTemp), _T("%u seconds"), s );

						SetDlgItemText( whois_hWnd, IDC_REPLY, DoColorCodes( szTemp, TRUE, FALSE ));
						return true;
			}	}	}

			//... else show the reply in the current window
			if ( pmsg->m_bIncoming && command == _T("ping")) {
				int s = (int)time(0) - (int)_ttol(GetWordAddress(mess.c_str(), 1));
				mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("CTCP PING reply from %s: %u sec(s)"), pmsg->prefix.sNick.c_str(), s); 
				DoEvent( GC_EVENT_INFORMATION, SERVERWINDOW, NULL, szTemp, NULL, NULL, NULL, true, false ); 
			}
			else {
				mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("CTCP %s reply from %s: %s"), ocommand.c_str(), pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 1));	
				DoEvent( GC_EVENT_INFORMATION, SERVERWINDOW, NULL, szTemp, NULL, NULL, NULL, true, false ); 
		}	}		
		
		return true;
	}
	
	return false;
}

bool CMyMonitor::OnIrc_NAMES( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 3 )
		sNamesList += pmsg->parameters[3] + _T(" ");
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_ENDNAMES( const CIrcMessage* pmsg )
{
	HWND hActiveWindow = GetActiveWindow();
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		TString name = _T("a");
		int i = 0;
		BOOL bFlag = false;

		// Is the user on the names list?
		while ( !name.empty() ) {
			name = GetWord( sNamesList.c_str(), i );
			i++;
			if ( !name.empty() ) {
				int index = 0;
				while ( _tcschr( sUserModePrefixes.c_str(), name[index] ))
					index++;

				if ( !lstrcmpi( name.substr(index, name.length()).c_str(), m_session.GetInfo().sNick.c_str())) {
					bFlag = true;
					break;
		}	}	}

		if ( bFlag ) {
			const TCHAR* sChanName = pmsg->parameters[1].c_str();
			if ( sChanName[0] == '@' || sChanName[0] == '*' || sChanName[0] == '=' )
				sChanName++;

			// Add a new chat window
			GCSESSION gcw = {0};
			TString sID = MakeWndID( sChanName );
			BYTE btOwnMode = 0;
			gcw.cbSize = sizeof(GCSESSION);
			gcw.iType = GCW_CHATROOM;
			gcw.dwFlags = GC_TCHAR;
			gcw.ptszID = sID.c_str();
			gcw.pszModule = IRCPROTONAME;
			gcw.ptszName = sChanName;
			if ( !CallServiceSync( MS_GC_NEWSESSION, 0, ( LPARAM )&gcw )) {
				DBVARIANT dbv;
				GCDEST gcd = {0};
				GCEVENT gce = {0};
				TString sTemp;
				int i = 0;

				PostIrcMessage( _T("/MODE %s"), sChanName );

				gcd.ptszID = ( TCHAR* )sID.c_str();
				gcd.pszModule = IRCPROTONAME;
				gcd.iType = GC_EVENT_ADDGROUP;
				gce.time = 0;
				gce.dwFlags = GC_TCHAR;

				//register the statuses
				gce.cbSize = sizeof(GCEVENT);
				gce.pDest = &gcd;

				gce.ptszStatus = _T("Owner");
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = _T("Admin");
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = _T("Op");
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = _T("Halfop");
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = _T("Voice");
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = _T("Normal");
				CallChatEvent(0, (LPARAM)&gce);

				i = 0;
				sTemp = GetWord(sNamesList.c_str(), i);

				// Fill the nicklist
				while ( !sTemp.empty() ) {
					TString sStat;
					TString sTemp2 = sTemp;
					sStat = PrefixToStatus(sTemp[0]);
					
					// fix for networks like freshirc where they allow more than one prefix
					while ( PrefixToStatus(sTemp[0]) != _T("Normal"))
						sTemp.erase(0,1);
					
					gcd.iType = GC_EVENT_JOIN;
					gce.ptszUID = sTemp.c_str();
					gce.ptszNick = sTemp.c_str();
					gce.ptszStatus = sStat.c_str();
					BOOL bIsMe = ( !lstrcmpi( gce.ptszNick, m_session.GetInfo().sNick.c_str())) ? TRUE : FALSE;
					if ( bIsMe ) {
						char BitNr = -1;
						switch ( sTemp2[0] ) {
							case '+':   BitNr = 0;   break;
							case '%':   BitNr = 1;   break;
							case '@':   BitNr = 2;   break;
							case '!':   BitNr = 3;   break;
							case '*':   BitNr = 4;   break;
						}
						if (BitNr >=0)
							btOwnMode = ( 1 << BitNr );
						else
							btOwnMode = 0;
					}
					gce.dwFlags = GC_TCHAR;
					gce.bIsMe = bIsMe;
					gce.time = bIsMe?time(0):0;
					CallChatEvent(0, (LPARAM)&gce);
					DoEvent( GC_EVENT_SETCONTACTSTATUS, sChanName, sTemp.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE );
					// fix for networks like freshirc where they allow more than one prefix
					if ( PrefixToStatus( sTemp2[0]) != _T("Normal")) {
						sTemp2.erase(0,1);
						sStat = PrefixToStatus(sTemp2[0]);
						while ( sStat != _T("Normal")) {
							DoEvent( GC_EVENT_ADDSTATUS, sID.c_str(), sTemp.c_str(), _T("system"), sStat.c_str(), NULL, NULL, false, false, 0 );
							sTemp2.erase(0,1);
							sStat = PrefixToStatus(sTemp2[0]);
					}	}

					i++;
					sTemp = GetWord(sNamesList.c_str(), i);
				}
				
				//Set the item data for the window
				{
					CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, sChanName, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);					
					if (!wi)
						wi = new CHANNELINFO;
					wi->OwnMode = btOwnMode;
					wi->pszLimit = 0;
					wi->pszMode = 0;
					wi->pszPassword = 0;
					wi->pszTopic = 0;
					wi->codepage = g_ircSession.getCodepage();
					DoEvent(GC_EVENT_SETITEMDATA, sChanName, NULL, NULL, NULL, NULL, (DWORD)wi, false, false, 0);

					if ( !sTopic.empty() && !lstrcmpi(GetWord(sTopic.c_str(), 0).c_str(), sChanName )) {
						DoEvent(GC_EVENT_TOPIC, sChanName, sTopicName.empty() ? NULL : sTopicName.c_str(), GetWordAddress(sTopic.c_str(), 1), NULL, sTopicTime.empty() ? NULL : sTopicTime.c_str(), NULL, true, false);
						AddWindowItemData(sChanName, 0, 0, 0, GetWordAddress(sTopic.c_str(), 1));
						sTopic = _T("");
						sTopicName = _T("");
						sTopicTime = _T("");
				}	}
				
				gcd.ptszID = (TCHAR*)sID.c_str();
				gcd.iType = GC_EVENT_CONTROL;
				gce.cbSize = sizeof(GCEVENT);
				gce.dwFlags = GC_TCHAR;
				gce.bIsMe = false;
				gce.dwItemData = false;
				gce.pszNick = NULL;
				gce.pszStatus = NULL;
				gce.pszText = NULL;
				gce.pszUID = NULL;
				gce.pszUserInfo = NULL;
				gce.time = time(0);
				gce.pDest = &gcd;
	
				if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "JTemp", &dbv )) {
					TString command = _T("a");
					TString save = _T("");
					int i = 0;

					while ( !command.empty() ) {
						command = GetWord( dbv.ptszVal, i );
						i++;
						if ( !command.empty() ) {
							TString S = command.substr(1, command.length());
							if ( !lstrcmpi( sChanName, S.c_str()))
								break;

							save += command;
							save += _T(" ");
					}	}

					if ( !command.empty() ) {
						save += GetWordAddress( dbv.ptszVal, i );
						switch ( command[0] ) {
						case 'M':
							CallChatEvent( WINDOW_HIDDEN, (LPARAM)&gce);
							break;
						case 'X':
							CallChatEvent( WINDOW_MAXIMIZE, (LPARAM)&gce);
							break;
						default:
							CallChatEvent( SESSION_INITDONE, (LPARAM)&gce);
							break;
						}
					}
					else CallChatEvent( SESSION_INITDONE, (LPARAM)&gce);

					if ( save.empty())
						DBDeleteContactSetting(NULL, IRCPROTONAME, "JTemp");
					else
						DBWriteContactSettingTString(NULL, IRCPROTONAME, "JTemp", save.c_str());
					DBFreeVariant(&dbv);
				}
				else CallChatEvent( SESSION_INITDONE, (LPARAM)&gce);

				{	gcd.iType = GC_EVENT_CONTROL;
					gce.pDest = &gcd;
					CallChatEvent( SESSION_ONLINE, (LPARAM)&gce);
	}	}	}	}

	sNamesList = _T("");
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_INITIALTOPIC( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 2 ) {
		AddWindowItemData( pmsg->parameters[1].c_str(), 0, 0, 0, pmsg->parameters[2].c_str());
		sTopic = pmsg->parameters[1] + _T(" ") + pmsg->parameters[2];
		sTopicName = _T("");
		sTopicTime = _T("");
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_INITIALTOPICNAME( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 3 ) {
		TCHAR tTimeBuf[128], *tStopStr;
		time_t ttTopicTime;
		sTopicName = pmsg->parameters[2];
		ttTopicTime = _tcstol( pmsg->parameters[3].c_str(), &tStopStr, 10);
		_tcsftime(tTimeBuf, 128, _T("%#c"), localtime(&ttTopicTime));
		sTopicTime = tTimeBuf;
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_TOPIC( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters.size() > 1 && pmsg->m_bIncoming ) {
		DoEvent( GC_EVENT_TOPIC, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str(), NULL, sTopicTime.empty() ? NULL : sTopicTime.c_str(), NULL, true, false);
		AddWindowItemData(pmsg->parameters[0].c_str(), 0, 0, 0, pmsg->parameters[1].c_str());
	}
	ShowMessage( pmsg );
	return true;
}

static void __stdcall sttShowDlgList( void* )
{
	if ( list_hWnd == NULL )
		list_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE(IDD_LIST), NULL, ListWndProc );
}

bool CMyMonitor::OnIrc_LISTSTART( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		CallFunctionAsync( sttShowDlgList, NULL );
		ChannelNumber = 0;
	}
	
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_LIST( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming == 1 && list_hWnd && pmsg->parameters.size() > 2 ) {
		ChannelNumber++;
		LVITEM lvItem;
		HWND hListView = GetDlgItem( list_hWnd, IDC_INFO_LISTVIEW );
		lvItem.iItem = ListView_GetItemCount( hListView ); 
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iSubItem = 0;
		lvItem.pszText = (TCHAR*)pmsg->parameters[1].c_str();
		lvItem.lParam = lvItem.iItem;
		lvItem.iItem = ListView_InsertItem( hListView, &lvItem );
		lvItem.mask = LVIF_TEXT;
		lvItem.iSubItem =1;
		lvItem.pszText = (TCHAR*)pmsg->parameters[pmsg->parameters.size()-2].c_str();
		ListView_SetItem( hListView, &lvItem );

		TCHAR* temp = mir_tstrdup( pmsg->parameters[pmsg->parameters.size()-1].c_str() );
		TCHAR* find = _tcsstr( temp , _T("[+"));
		TCHAR* find2 = _tcsstr( temp , _T("]"));
		TCHAR* save = temp;
		if ( find == temp && find2 != NULL && find+8 >= find2 ) {
			temp = _tcsstr( temp, _T("]"));
			if ( lstrlen(temp) > 1 ) {
				temp++;
				temp[0] = '\0';
				lvItem.iSubItem =2;
				lvItem.pszText = save;
				ListView_SetItem(hListView,&lvItem);
				temp[0] = ' ';
				temp++;
			}
			else temp =save;
		}
		
		lvItem.iSubItem =3;
		TString S = DoColorCodes(temp, TRUE, FALSE);
		lvItem.pszText = ( TCHAR* )S.c_str();
		ListView_SetItem( hListView, &lvItem );
		temp = save;
		mir_free( temp );
	
		int percent = 100;
		if ( NoOfChannels > 0 )
			percent = (int)(ChannelNumber*100) / NoOfChannels;

		TCHAR text[100];
		if ( percent < 100)
			mir_sntprintf(text, SIZEOF(text), TranslateT("Downloading list (%u%%) - %u channels"), percent, ChannelNumber);
		else
			mir_sntprintf(text, SIZEOF(text), TranslateT("Downloading list - %u channels"), ChannelNumber);
		SetDlgItemText(list_hWnd, IDC_TEXT, text);
	}
	
	return true;
}

bool CMyMonitor::OnIrc_LISTEND( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && list_hWnd ) {
		EnableWindow(GetDlgItem(list_hWnd, IDC_JOIN), true);
		ListView_SetSelectionMark(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 0);		
		ListView_SetColumnWidth(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 1, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 2, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 3, LVSCW_AUTOSIZE);
		SendMessage(list_hWnd, IRC_UPDATELIST, 0, 0);

		TCHAR text[100];
		mir_sntprintf( text, SIZEOF(text), TranslateT("Done: %u channels"), ChannelNumber );
		int percent = 100;
		if ( NoOfChannels > 0 )
			percent = (int)(ChannelNumber*100) / NoOfChannels;
		if ( percent < 70 ) {
			lstrcat( text, _T(" "));
			lstrcat( text, TranslateT("(probably truncated by server)"));
		}
		SetDlgItemText( list_hWnd, IDC_TEXT, text );
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_BANLIST( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 2 ) {
		if ( manager_hWnd && (
			IsDlgButtonChecked(manager_hWnd, IDC_RADIO1) && pmsg->sCommand == _T("367") ||
			IsDlgButtonChecked(manager_hWnd, IDC_RADIO2) && pmsg->sCommand == _T("346") ||
			IsDlgButtonChecked(manager_hWnd, IDC_RADIO3) && pmsg->sCommand == _T("348")) && 
			!IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO1)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO2)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO3)))
		{
			TString S = pmsg->parameters[2];
			if ( pmsg->parameters.size() > 3 ) {
				S += _T("   - ");
				S += pmsg->parameters[3];
				if ( pmsg->parameters.size() > 4 ) {
					S += _T(" -  ( ");
					time_t time = StrToInt( pmsg->parameters[4].c_str());
					S += _tctime( &time );
					ReplaceString( S, _T("\n"), _T(" "));
					S += _T(")");
			}	}

			SendDlgItemMessage(manager_hWnd, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)S.c_str());
	}	}

	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_BANLISTEND( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		if ( manager_hWnd && 
			  (IsDlgButtonChecked(manager_hWnd, IDC_RADIO1) && pmsg->sCommand == _T("368")
			|| IsDlgButtonChecked(manager_hWnd, IDC_RADIO2) && pmsg->sCommand == _T("347")
			|| IsDlgButtonChecked(manager_hWnd, IDC_RADIO3) && pmsg->sCommand == _T("349")) && 
			!IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO1)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO2)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO3)))
		{
			if ( strchr( sChannelModes.c_str(), 'b' ))
				EnableWindow( GetDlgItem(manager_hWnd, IDC_RADIO1), true );
			if ( strchr( sChannelModes.c_str(), 'I' ))
				EnableWindow( GetDlgItem(manager_hWnd, IDC_RADIO2), true );
			if ( strchr( sChannelModes.c_str(), 'e' ))
				EnableWindow( GetDlgItem(manager_hWnd, IDC_RADIO3), true );
			if ( !IsDlgButtonChecked(manager_hWnd, IDC_NOTOP))
				EnableWindow( GetDlgItem(manager_hWnd, IDC_ADD), true );
	}	}
	
	ShowMessage( pmsg );
	return true;
}

static void __stdcall sttShowWhoisWnd( void* param )
{
	CIrcMessage* pmsg = ( CIrcMessage* )param;

	if ( whois_hWnd == NULL )
		whois_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE(IDD_INFO), NULL, InfoWndProc );
	if ( SendMessage( GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_FINDSTRINGEXACT, -1, (LPARAM) pmsg->parameters[1].c_str()) == CB_ERR)	
		SendMessage( GetDlgItem( whois_hWnd, IDC_INFO_NICK), CB_ADDSTRING, 0, (LPARAM) pmsg->parameters[1].c_str());	
	int i = SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_FINDSTRINGEXACT, -1, (LPARAM) pmsg->parameters[1].c_str());
	SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_SETCURSEL, i, 0);	
	SetWindowText(GetDlgItem(whois_hWnd, IDC_CAPTION), pmsg->parameters[1].c_str());	

	SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_NAME), pmsg->parameters[5].c_str());	
	SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_ADDRESS), pmsg->parameters[3].c_str());
	SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_ID), pmsg->parameters[2].c_str());
	SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_CHANNELS), "");
	SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_SERVER), "");
	SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_AWAY2), "");
	SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_AUTH), "");
	SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_OTHER), "");
	SetWindowTextA(GetDlgItem(whois_hWnd, IDC_REPLY), "");
	SetWindowText(whois_hWnd, TranslateT("User information"));
	EnableWindow(GetDlgItem(whois_hWnd, ID_INFO_QUERY), true);
	ShowWindow(whois_hWnd, SW_SHOW);
	if ( IsIconic( whois_hWnd ))
		ShowWindow( whois_hWnd, SW_SHOWNORMAL );
	SendMessage(whois_hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(whois_hWnd, NULL, TRUE);
	delete pmsg;
}

bool CMyMonitor::OnIrc_WHOIS_NAME( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 5 && ManualWhoisCount > 0 )
		CallFunctionAsync( sttShowWhoisWnd, new CIrcMessage( *pmsg ));

	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_CHANNELS( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() > 2 && ManualWhoisCount > 0 )
		SetDlgItemText( whois_hWnd, IDC_INFO_CHANNELS, pmsg->parameters[2].c_str());
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_AWAY( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() > 2 && ManualWhoisCount > 0 )
		SetDlgItemText( whois_hWnd, IDC_INFO_AWAY2, pmsg->parameters[2].c_str());
	if ( ManualWhoisCount < 1 && pmsg->m_bIncoming && pmsg->parameters.size() > 2 )
		WhoisAwayReply = pmsg->parameters[2];
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_OTHER( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() > 2 && ManualWhoisCount > 0 ) {
		TCHAR temp[1024], temp2[1024];
		GetDlgItemText( whois_hWnd, IDC_INFO_OTHER, temp, 1000);
		lstrcat( temp, _T("%s\r\n"));
		mir_sntprintf( temp2, 1020, temp, pmsg->parameters[2].c_str()); 
		SetWindowText( GetDlgItem( whois_hWnd, IDC_INFO_OTHER), temp2 );
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_END( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 && ManualWhoisCount < 1 ) {
		CONTACT user = { (TCHAR*)pmsg->parameters[1].c_str(), NULL, NULL, false, false, true};
		HANDLE hContact = CList_FindContact( &user );
		if ( hContact )
			ProtoBroadcastAck( IRCPROTONAME, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)( char* )_T2A( WhoisAwayReply.c_str(), m_session.getCodepage()));
	}

	ManualWhoisCount--;
	if (ManualWhoisCount < 0)
		ManualWhoisCount = 0;
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_IDLE( const CIrcMessage* pmsg )
{
	int D = 0;
	int H = 0;
	int M = 0;
	int S = 0;
	if ( pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() > 2 && ManualWhoisCount > 0 ) {
		S = StrToInt(pmsg->parameters[2].c_str());
		D = S/(60*60*24);
		S -= (D * 60 * 60 *24);
		H = S/(60*60);
		S -= (H * 60 * 60);
		M = S/60;
		S -= (M * 60 );
		
		TCHAR temp[100];
		if ( D )
			mir_sntprintf(temp, 99, _T("%ud, %uh, %um, %us"), D, H, M, S);
		else if (H)
			mir_sntprintf(temp, 99, _T("%uh, %um, %us"), H, M, S);
		else if (M)
			mir_sntprintf(temp, 99, _T("%um, %us"), M, S);
		else if (S)
			mir_sntprintf(temp, 99, _T("%us"), S);

		TCHAR temp3[256];
		TCHAR tTimeBuf[128], *tStopStr;
		time_t ttTime = _tcstol( pmsg->parameters[3].c_str(), &tStopStr, 10);
		_tcsftime(tTimeBuf, 128, _T("%c"), localtime(&ttTime));
		mir_sntprintf( temp3, SIZEOF(temp3), _T("online since %s, idle %s"), tTimeBuf, temp);
		SetDlgItemText( whois_hWnd, IDC_AWAYTIME, temp3 );	
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_SERVER( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() > 2 && ManualWhoisCount > 0 )
		SetDlgItemText( whois_hWnd, IDC_INFO_SERVER, pmsg->parameters[2].c_str());
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_AUTH( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() > 2 && ManualWhoisCount > 0 ) {
		if ( pmsg->sCommand == _T("330"))
			SetDlgItemText( whois_hWnd, IDC_INFO_AUTH, pmsg->parameters[2].c_str());
		else if ( pmsg->parameters[2] == _T("is an identified user") || pmsg->parameters[2] == _T("is a registered nick"))
			SetDlgItemText( whois_hWnd, IDC_INFO_AUTH, pmsg->parameters[2].c_str());
		else
			OnIrc_WHOIS_OTHER( pmsg );
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_NO_USER( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 2 && !IsChannel( pmsg->parameters[1] )) {
		if ( whois_hWnd ) {
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_NICK), pmsg->parameters[2].c_str());
			SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_SETEDITSEL, 0,MAKELPARAM(0,-1));
			SetWindowText(GetDlgItem(whois_hWnd, IDC_CAPTION), pmsg->parameters[2].c_str());	
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_NAME), "");	
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_ADDRESS), "");
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_ID), "");
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_CHANNELS), "");
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_SERVER), "");
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_AWAY2), "");
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_INFO_AUTH), "");
			SetWindowTextA(GetDlgItem(whois_hWnd, IDC_REPLY), "");
			EnableWindow(GetDlgItem(whois_hWnd, ID_INFO_QUERY), false);
		}
		
		CONTACT user = { (TCHAR*)pmsg->parameters[1].c_str(), NULL, NULL, false, false, false};
		HANDLE hContact = CList_FindContact( &user );
		if ( hContact ) {
			AddOutgoingMessageToDB( hContact, (TCHAR*)((TString)_T("> ") + pmsg->parameters[2] + (TString)_T(": ") + pmsg->parameters[1]).c_str() );

			DBVARIANT dbv;
			if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
				DBWriteContactSettingTString( hContact, IRCPROTONAME, "Nick", dbv.ptszVal );
				
				DBVARIANT dbv2;
				if ( DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0 ) == 0 )
					DoUserhostWithReason(1, ((TString)_T("S") + dbv.ptszVal).c_str(), true, dbv.ptszVal );
				else {
					if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UWildcard", &dbv2 )) {
						DoUserhostWithReason(2, ((TString)_T("S") + dbv2.ptszVal).c_str(), true, dbv2.ptszVal );
						DBFreeVariant(&dbv2);
					}
					else DoUserhostWithReason(2, ((TString)_T("S") + dbv.ptszVal).c_str(), true, dbv.ptszVal );
				}
				DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
				DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
				DBFreeVariant(&dbv);
	}	}	}

	ShowMessage( pmsg );
	return true;
}

static void __stdcall sttShowNickWnd( void* param )
{
	CIrcMessage* pmsg = ( CIrcMessage* )param;

	if ( nick_hWnd == NULL )
		nick_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE( IDD_NICK ), NULL, NickWndProc );

	SetWindowText( GetDlgItem( nick_hWnd, IDC_CAPTION ), TranslateT("Change nickname"));
	SetWindowText( GetDlgItem( nick_hWnd, IDC_TEXT    ), pmsg->parameters[2].c_str());
	SetWindowText( GetDlgItem( nick_hWnd, IDC_ENICK   ), pmsg->parameters[1].c_str());
	SendMessage( GetDlgItem(nick_hWnd, IDC_ENICK), CB_SETEDITSEL, 0,MAKELPARAM(0,-1));
	delete pmsg;
}

bool CMyMonitor::OnIrc_NICK_ERR( const CIrcMessage* pmsg )
{
	if (( nickflag || prefs->AlternativeNick[0] == 0) && pmsg->m_bIncoming && pmsg->parameters.size() > 2 ) {
		CallFunctionAsync( sttShowNickWnd, new CIrcMessage( *pmsg ));
	}
	else if ( pmsg->m_bIncoming ) {
		TCHAR m[200];
		mir_sntprintf( m, SIZEOF(m), _T("NICK %s"), prefs->AlternativeNick );
		if ( g_ircSession )
			g_ircSession << irc::CIrcMessage( m, g_ircSession.getCodepage() );

		nickflag = true;
	}

	ShowMessage( pmsg );
 	return true;
}

bool CMyMonitor::OnIrc_JOINERROR( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		DBVARIANT dbv;
		if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "JTemp", &dbv )) {
			TString command = _T("a");
			TString save = _T("");
			int i = 0;

			while ( !command.empty() ) {
				command = GetWord( dbv.ptszVal, i );
				i++;

				if ( !command.empty() && pmsg->parameters[0] == command.substr(1, command.length())) {
					save += command;
					save += _T(" ");
			}	}
			
			DBFreeVariant(&dbv);

			if ( save.empty())
				DBDeleteContactSetting( NULL, IRCPROTONAME, "JTemp" );
			else
				DBWriteContactSettingTString( NULL, IRCPROTONAME, "JTemp", save.c_str());
	}	}

	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_UNKNOWN( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 0 ) {
		if ( pmsg->parameters[0] == _T("WHO") && GetNextUserhostReason(2) != _T("U"))
			return true;
		if ( pmsg->parameters[0] == _T("USERHOST") && GetNextUserhostReason(1) != _T("U"))
			return true;
	}
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_ENDMOTD( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && !bPerformDone )
		DoOnConnect( pmsg );
	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_NOOFCHANNELS( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 )
		NoOfChannels = StrToInt( pmsg->parameters[1].c_str());

	if ( pmsg->m_bIncoming && !bPerformDone )
		DoOnConnect( pmsg );

	ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_ERROR( const CIrcMessage* pmsg )
{
	if	( pmsg->m_bIncoming && !prefs->DisableErrorPopups ) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = IRCPROTONAME;
		msn.tszInfoTitle = TranslateT("IRC error");
		
		TString S;
		if ( pmsg->parameters.size() > 0 ) 
			S = DoColorCodes( pmsg->parameters[0].c_str(), TRUE, FALSE );
		else
			S = TranslateT( "Unknown" );

		msn.tszInfo = ( TCHAR* )S.c_str();
		msn.dwInfoFlags = NIIF_ERROR | NIIF_INTERN_UNICODE;
		msn.uTimeout = 15000;
		CallService( MS_CLIST_SYSTRAY_NOTIFY, 0, ( LPARAM )&msn );
	}
	ShowMessage( pmsg );
	return true;	
}

bool CMyMonitor::OnIrc_WHO_END( const CIrcMessage* pmsg )
{
	TString command = GetNextUserhostReason(2);
	if ( command[0] == 'S' ) {
		if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
			// is it a channel?
			if ( IsChannel( pmsg->parameters[1] )) {
				TString S = _T("");
				TString SS = WhoReply;
				TString User = GetWord( WhoReply.c_str(), 0 );
				while ( !User.empty()) {
					if ( GetWord( WhoReply.c_str(), 3)[0] == 'G' ) {
						S += User;
						S += _T("\t");
						DoEvent( GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[1].c_str(), User.c_str(), NULL, NULL, NULL, ID_STATUS_AWAY, FALSE, FALSE);
					}
					else DoEvent( GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[1].c_str(), User.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE);

					SS = GetWordAddress( WhoReply.c_str(), 4 );
					if ( SS.empty())
						break;
					WhoReply = SS;
					User = GetWord(WhoReply.c_str(), 0);
				}
				
				DoEvent( GC_EVENT_SETSTATUSEX, pmsg->parameters[1].c_str(), NULL, S.empty() ? NULL : S.c_str(), NULL, NULL, GC_SSE_TABDELIMITED, FALSE, FALSE); 
				return true;
			}

			/// if it is not a channel
			TCHAR* UserList = mir_tstrdup( WhoReply.c_str() );
			TCHAR* p1= UserList;
			WhoReply = _T("");
			CONTACT user = { (TCHAR*)pmsg->parameters[1].c_str(), NULL, NULL, false, true, false};
			HANDLE hContact = CList_FindContact( &user );

			if ( hContact && DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0 ) == 1 ) {
				DBVARIANT dbv1, dbv2, dbv3, dbv4, dbv5, dbv6, dbv7;	
				TCHAR *DBDefault = NULL, *DBNick = NULL, *DBWildcard = NULL;
				TCHAR *DBUser = NULL, *DBHost = NULL, *DBManUser = NULL, *DBManHost = NULL;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "Default", &dbv1)) DBDefault = dbv1.ptszVal;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "Nick", &dbv2)) DBNick = dbv2.ptszVal;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "UWildcard", &dbv3)) DBWildcard = dbv3.ptszVal;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "UUser", &dbv4)) DBUser = dbv4.ptszVal;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "UHost", &dbv5)) DBHost = dbv5.ptszVal;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "User", &dbv6)) DBManUser = dbv6.ptszVal;
				if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "Host", &dbv7)) DBManHost = dbv7.ptszVal;
				if ( DBWildcard )
					CharLower( DBWildcard );

				TString nick;
				TString user;
				TString host;
				TString away = GetWord(p1, 3);

				while ( !away.empty()) {
					nick = GetWord(p1, 0);
					user = GetWord(p1, 1);
					host = GetWord(p1, 2);
					if (( DBWildcard && WCCmp( DBWildcard, nick.c_str()) || DBNick && !lstrcmpi(DBNick, nick.c_str()) || DBDefault && !lstrcmpi(DBDefault, nick.c_str()))
						&& (WCCmp(DBUser, user.c_str()) && WCCmp(DBHost, host.c_str())))
					{
						if (away[0] == 'G' && DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) != ID_STATUS_AWAY)
							DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_AWAY);
						else if (away[0] == 'H' && DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) != ID_STATUS_ONLINE)
							DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE);

						if (( DBNick && lstrcmpi( nick.c_str(), DBNick)) || !DBNick )
							DBWriteContactSettingTString( hContact, IRCPROTONAME, "Nick", nick.c_str());
						if (( DBManUser && lstrcmpi( user.c_str(), DBManUser)) || !DBManUser )
							DBWriteContactSettingTString( hContact, IRCPROTONAME, "User", user.c_str());
						if (( DBManHost && lstrcmpi(host.c_str(), DBManHost)) || !DBManHost )
							DBWriteContactSettingTString(hContact, IRCPROTONAME, "Host", host.c_str());

						goto LBL_Exit;
					}
					p1 = GetWordAddress(p1, 4);
					away = GetWord(p1, 3);
				}
				
				if ( DBWildcard && DBNick && !WCCmp( CharLower( DBWildcard ), CharLower( DBNick ))) {
					DBWriteContactSettingTString(hContact, IRCPROTONAME, "Nick", DBDefault);
					
					DoUserhostWithReason(2, ((TString)_T("S") + DBWildcard).c_str(), true, DBWildcard);
							
					DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
					goto LBL_Exit;
				}

				if ( DBGetContactSettingWord( hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE ) {
					DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
					DBWriteContactSettingTString(hContact, IRCPROTONAME, "Nick", DBDefault);
					DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
				}
LBL_Exit:
				if ( DBDefault )  DBFreeVariant(&dbv1);
				if ( DBNick )     DBFreeVariant(&dbv2);
				if ( DBWildcard ) DBFreeVariant(&dbv3);
				if ( DBUser )     DBFreeVariant(&dbv4);
				if ( DBHost )     DBFreeVariant(&dbv5);
				if ( DBManUser )  DBFreeVariant(&dbv6);
				if ( DBManHost )  DBFreeVariant(&dbv7);
			}
			mir_free( UserList );
		}
	}
	else ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_WHO_REPLY( const CIrcMessage* pmsg )
{
	TString command = PeekAtReasons(2);
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 6 && command[0] == 'S' ) {
		WhoReply += pmsg->parameters[5] + _T(" ") +pmsg->parameters[2] + _T(" ") + pmsg->parameters[3] + _T(" ") + pmsg->parameters[6] + _T(" ");
		if ( lstrcmpi( pmsg->parameters[5].c_str(), m_session.GetInfo().sNick.c_str()) == 0 ) {
			TCHAR host[1024];
			lstrcpyn( host, pmsg->parameters[3].c_str(), 1024 );
			mir_forkthread( ResolveIPThread, new IPRESOLVE( _T2A(host), IP_AUTO ));
	}	}

	if ( command[0] == 'U' )
		ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_TRYAGAIN( const CIrcMessage* pmsg )
{
	TString command = _T("");
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		if ( pmsg->parameters[1] == _T("WHO"))
			command = GetNextUserhostReason(2);

		if ( pmsg->parameters[1] == _T("USERHOST"))
			command = GetNextUserhostReason(1);
	}
	if (command[0] == 'U')
		ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_USERHOST_REPLY( const CIrcMessage* pmsg )
{
	TString command = _T("");
	if ( pmsg->m_bIncoming ) {
		command = GetNextUserhostReason(1);
		if ( !command.empty() && command != _T("U") && pmsg->parameters.size() > 1 ) {
			CONTACT finduser = {NULL, NULL, NULL, false, false, false};
			TCHAR* params = NULL;
			TCHAR* next = NULL;
			TCHAR* p1 = NULL;
			TCHAR* p2 = NULL;
			int awaystatus = 0;
			TString sTemp = _T("");
			TString host = _T("");
			TString user = _T("");
			TString nick = _T("");
			TString mask = _T("");
			TString mess = _T("");
			TString channel;
			int i;
			int j;

			// Status-check pre-processing: Setup check-list
			std::vector<TString> checklist;
			if ( command[0] == 'S' ) {
				j = 0;
				sTemp = GetWord(command.c_str(), 0);
				sTemp.erase(0,1);
				while ( !sTemp.empty()) {
					checklist.push_back(sTemp);
					j++;
					sTemp = GetWord(command.c_str(), j);
			}	}

			// Cycle through results
//			params = new char[pmsg->parameters[1].length()+2];
//			lstrcpynA(params, pmsg->parameters[1].c_str(), pmsg->parameters[1].length()+1);
//			for(p1 = GetWordAddress(params, 0); *p1; p1 = next)
			j = 0;
			sTemp = GetWord( pmsg->parameters[1].c_str(), j );
			while ( !sTemp.empty() ) {
//				p2 = next = GetWordAddress(p1, 1);
//				while(*(p2 - 1) == ' ') p2--;
//				*p2 = '\0';
				p1 = mir_tstrdup( sTemp.c_str() );
				p2 = p1;

				// Pull out host, user and nick
				p2 = _tcschr(p1, '@');
				if ( p2 ) {
					*p2 = '\0';
					p2++;
					host = p2;
				}
				p2 = _tcschr(p1, '=');
				if ( p2 ) {
					if (*(p2-1) == '*')
						*(p2-1) = '\0';  //  remove special char for IRCOps
					*p2 = '\0';
					p2++;
					awaystatus = *p2;
					p2++;
					user = p2;
					nick = p1;
				}
				mess = _T("");
				mask = nick + _T("!") + user + _T("@") + host;
				if ( host.empty() || user.empty() || nick.empty() ) {
					mir_free( p1 );
					continue;
				}

				// Do command
				switch ( command[0] ) {
				case 'S':	// Status check
				{
					finduser.name = (TCHAR*)nick.c_str();
					finduser.host = (TCHAR*)host.c_str();
					finduser.user = (TCHAR*)user.c_str();

					HANDLE hContact = CList_FindContact(&finduser);
					if ( hContact && DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0 ) == 0 ) {
						DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", awaystatus == '-'? ID_STATUS_AWAY : ID_STATUS_ONLINE);
						DBWriteContactSettingTString(hContact, IRCPROTONAME, "User", user.c_str());
						DBWriteContactSettingTString(hContact, IRCPROTONAME, "Host", host.c_str());
						DBWriteContactSettingTString(hContact, IRCPROTONAME, "Nick", nick.c_str());

						// If user found, remove from checklist
						for ( i = 0; i < (int)checklist.size(); i++ )
							if ( !lstrcmpi(checklist[i].c_str(), nick.c_str() ))
								checklist.erase(checklist.begin() + i);
					}
					break;
				}
				case 'I':	// Ignore
					mess = _T("/IGNORE %question=\"");
					mess += TranslateT("Please enter the hostmask (nick!user@host)\nNOTE! Contacts on your contact list are never ignored");
					mess += (TString)_T("\",\"") + TranslateT("Ignore") + _T("\",\"*!*@") + host + _T("\"");
					if ( prefs->IgnoreChannelDefault )
						mess += _T(" +qnidcm");
					else
						mess += _T(" +qnidc");
					break;

				case 'J':	// Unignore
					mess = _T("/UNIGNORE *!*@") + host;
					break;

				case 'B':	// Ban
					channel = (command.c_str() + 1);
					mess = _T("/MODE ") + channel + _T(" +b *!*@") + host;
					break;

				case 'K':	// Ban & Kick
					channel = (command.c_str() + 1);
					mess = _T("/MODE ") + channel + _T(" +b *!*@") + host + _T("%newl/KICK ") + channel + _T(" ") + nick;
					break;

				case 'L':	// Ban & Kick with reason
					channel = (command.c_str() + 1);
					mess = _T("/MODE ") + channel + _T(" +b *!*@") + host + _T("%newl/KICK ") + channel + _T(" ") + nick + _T(" %question=\"");
					mess += (TString)TranslateT("Please enter the reason") + _T("\",\"") + TranslateT("Ban'n Kick") + _T("\",\"") + TranslateT("Jerk") + _T("\"");
					break;
				}
				
				mir_free( p1 );

				// Post message
				if ( !mess.empty())
					PostIrcMessageWnd( NULL, NULL, mess.c_str());
				j++;
				sTemp = GetWord(pmsg->parameters[1].c_str(), j);
			}
			
			// Status-check post-processing: make buddies in ckeck-list offline
			if ( command[0] == 'S' ) {
				for ( i = 0; i < (int)checklist.size(); i++ ) {
					finduser.name = (TCHAR*)checklist[i].c_str();
					finduser.ExactNick = true;
					CList_SetOffline( &finduser );
			}	}
			return true;
	}	}
	
	if ( !pmsg->m_bIncoming || command == _T("U"))
		ShowMessage( pmsg );
	return true;
}

bool CMyMonitor::OnIrc_SUPPORT( const CIrcMessage* pmsg )
{
	static const TCHAR* lpszFmt = _T("Try server %99[^ ,], port %19s");
	TCHAR szAltServer[100];
	TCHAR szAltPort[20];
	if ( pmsg->parameters.size() > 1 && _stscanf(pmsg->parameters[1].c_str(), lpszFmt, &szAltServer, &szAltPort) == 2 ) {
		ShowMessage( pmsg );
		lstrcpynA(prefs->ServerName, _T2A(szAltServer), 99);
		lstrcpynA(prefs->PortStart, _T2A(szAltPort), 9);

		NoOfChannels = 0;
		ConnectToServer();
		return true;
	}

	if ( pmsg->m_bIncoming && !bPerformDone )
		DoOnConnect(pmsg);
	
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 0 ) {
		TString S;
		for ( size_t i = 0; i < pmsg->parameters.size(); i++ ) {
			TCHAR* temp = mir_tstrdup( pmsg->parameters[i].c_str() );
			if ( _tcsstr( temp, _T("CHANTYPES="))) {
				TCHAR* p1 = _tcschr( temp, '=' );
				p1++;
				if ( lstrlen( p1 ) > 0 )
					sChannelPrefixes = p1;
			}
			if ( _tcsstr(temp, _T("CHANMODES="))) {
				TCHAR* p1 = _tcschr( temp, '=' );
				p1++;
				if ( lstrlen( p1 ) > 0)
					sChannelModes = ( char* )_T2A( p1 );
			}
			if ( _tcsstr( temp, _T("PREFIX="))) {
				TCHAR* p1 = _tcschr( temp, '(' );
				TCHAR* p2 = _tcschr( temp, ')' );
				if ( p1 && p2 ) {
					p1++;
					if ( p1 != p2 )
						sUserModes = ( char* )_T2A( p1 );
					sUserModes = sUserModes.substr(0, p2-p1);
					p2++;
					if ( *p2 != '\0' )
						sUserModePrefixes = p2;
				}
				else {
					p1 = _tcschr( temp, '=' );
					p1++;
					sUserModePrefixes = p1;
					for ( size_t i =0; i < sUserModePrefixes.length()+1; i++ ) {
						if ( sUserModePrefixes[i] == '@' )
							sUserModes[i] = 'o';
						else if ( sUserModePrefixes[i] == '+' )
							sUserModes[i] = 'v';
						else if ( sUserModePrefixes[i] == '-' )
							sUserModes[i] = 'u';
						else if ( sUserModePrefixes[i] == '%' )
							sUserModes[i] = 'h';
						else if ( sUserModePrefixes[i] == '!' )
							sUserModes[i] = 'a';
						else if ( sUserModePrefixes[i] == '*' )
							sUserModes[i] = 'q';
						else if ( sUserModePrefixes[i] == '\0' )
							sUserModes[i] = '\0';
						else  
							sUserModes[i] = '_';
			}	}	}

			mir_free( temp );
	}	}

	ShowMessage( pmsg );
	return true;
}

void CMyMonitor::OnIrcDefault( const CIrcMessage* pmsg )
{
	CIrcDefaultMonitor::OnIrcDefault(pmsg);
	ShowMessage( pmsg );
}

void CMyMonitor::OnIrcDisconnected()
{
	CIrcDefaultMonitor::OnIrcDisconnected();
	StatusMessage = _T("");
	DBDeleteContactSetting(NULL, IRCPROTONAME, "JTemp");
	bTempDisableCheck = false;
	bTempForceCheck = false;
	iTempCheckTime = 0;

	prefs->MyHost[0] = '\0';

	int Temp = OldStatus;
	KillChatTimer( OnlineNotifTimer );
	KillChatTimer( OnlineNotifTimer3 );
	KillChatTimer( KeepAliveTimer );
	KillChatTimer( InitTimer );
	KillChatTimer( IdentTimer );
	OldStatus = ID_STATUS_OFFLINE;
	ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp, ID_STATUS_OFFLINE);

	TString sDisconn = _T("\0035\002");
	sDisconn += TranslateT("*Disconnected*");
	DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, sDisconn.c_str(), NULL, NULL, NULL, true, false); 

	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gce.cbSize = sizeof(GCEVENT);
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
	}

	if ( !Miranda_Terminated() )
		CList_SetAllOffline( prefs->DisconnectDCCChats );
	DBWriteContactSettingTString( NULL, IRCPROTONAME, "Nick", prefs->Nick );
	
	CLISTMENUITEM clmi = {0};
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnConnect

static void __stdcall sttMainThrdOnConnect( void* )
{
	SetChatTimer( InitTimer, 1*1000, TimerProc );
	if ( prefs->IdentTimer )
		SetChatTimer( IdentTimer, 60*1000, IdentTimerProc );
	if ( prefs->SendKeepAlive )
		SetChatTimer( KeepAliveTimer, 60*1000, KeepAliveTimerProc );
	if ( prefs->AutoOnlineNotification && !bTempDisableCheck || bTempForceCheck ) {
		SetChatTimer( OnlineNotifTimer, 1000, OnlineNotifTimerProc );
		if ( prefs->ChannelAwayNotification )
			SetChatTimer(OnlineNotifTimer3, 3000, OnlineNotifTimerProc3);
}	}

bool DoOnConnect( const CIrcMessage* pmsg )
{
	bPerformDone = true;
	nickflag = true;	

	CLISTMENUITEM clmi = {0};
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );

	int Temp = OldStatus;
	OldStatus = ID_STATUS_ONLINE;
	ProtoBroadcastAck( IRCPROTONAME, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE )Temp, ID_STATUS_ONLINE );

	if ( !StatusMessage.empty()) {
		TString S = _T("/AWAY ");
		S += StatusMessage;
		ReplaceString( S, _T("\r\n"), _T(" "));
		PostIrcMessage( S.c_str());
	}
	
	if ( prefs->Perform ) {
		DoPerform( "ALL NETWORKS" );
		if ( g_ircSession )
			DoPerform( _T2A( g_ircSession.GetInfo().sNetwork.c_str()));
	}

	if ( prefs->RejoinChannels ) {
		int count = CallServiceSync( MS_GC_GETSESSIONCOUNT, 0, (LPARAM)IRCPROTONAME);
		for ( int i = 0; i < count ; i++ ) {
			GC_INFO gci = {0};
			gci.Flags = BYINDEX | DATA | NAME | TYPE;
			gci.iItem = i;
			gci.pszModule = IRCPROTONAME;
			if ( !CallServiceSync( MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM ) {
				CHANNELINFO* wi = ( CHANNELINFO* )gci.dwItemData;
				if ( wi && wi->pszPassword )
					PostIrcMessage( _T("/JOIN %s %s"), gci.pszName, wi->pszPassword);
				else
					PostIrcMessage( _T("/JOIN %s"), gci.pszName);
	}	}	}

	DoEvent( GC_EVENT_ADDGROUP, SERVERWINDOW, NULL, NULL, _T("Normal"), NULL, NULL, FALSE, TRUE); 
	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gce.dwFlags = GC_TCHAR;
		gce.cbSize = sizeof(GCEVENT);
		gcd.ptszID = SERVERWINDOW;
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_ONLINE, (LPARAM)&gce);
	}

	CallFunctionAsync( sttMainThrdOnConnect, NULL );
	return 0;
}

static void __cdecl AwayWarningThread(LPVOID di)
{
	MessageBox(NULL, TranslateT("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), TranslateT("IRC Error"), MB_OK);
}

int DoPerform( const char* event )
{
	String sSetting = String("PERFORM:") + event;
	transform( sSetting.begin(), sSetting.end(), sSetting.begin(), toupper );

	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, sSetting.c_str(), &dbv )) {
		if ( !my_strstri( dbv.ptszVal, _T("/away")))
			PostIrcMessageWnd( NULL, NULL, dbv.ptszVal );
		else
			mir_forkthread( AwayWarningThread, NULL  );
		DBFreeVariant( &dbv );
		return 1;
	}
	return 0;
}

int IsIgnored( TString nick, TString address, TString host, char type) 
{ 
	return IsIgnored( nick + _T("!") + address + _T("@") + host, type );
}

int IsIgnored( TString user, char type ) 
{
	for ( size_t i=0; i < g_ignoreItems.size(); i++ ) {
		CIrcIgnoreItem& C = g_ignoreItems[i];

      if ( type == '\0' )	
			if ( !lstrcmpi( user.c_str(), C.mask.c_str()))
				return i+1;
			
		bool bUserContainsWild = ( _tcschr( user.c_str(), '*') != NULL || _tcschr( user.c_str(), '?' ) != NULL );
		if ( !bUserContainsWild && WCCmp( C.mask.c_str(), user.c_str()) || 
			  bUserContainsWild && !lstrcmpi( user.c_str(), C.mask.c_str()))
		{
			if ( C.flags.empty() || C.flags[0] != '+' )
				continue;

			if ( !_tcschr( C.flags.c_str(), type ))
				continue;

			if ( C.network.empty() )
				return i+1;

			if ( g_ircSession && !lstrcmpi( C.network.c_str(), g_ircSession.GetInfo().sNetwork.c_str()))
				return i+1;
	}	}

	return 0; 
}

bool AddIgnore( const TCHAR* mask, const TCHAR* flags, const TCHAR* network ) 
{ 
	RemoveIgnore( mask );
	g_ignoreItems.push_back( CIrcIgnoreItem( mask, (_T("+") + TString(flags)).c_str(), network ));
	RewriteIgnoreSettings();

	if ( IgnoreWndHwnd )
		SendMessage(IgnoreWndHwnd, IRC_REBUILDIGNORELIST, 0, 0);
	return true;
}  

bool RemoveIgnore( const TCHAR* mask ) 
{ 
	int idx;
	while (( idx = IsIgnored( mask, '\0')) != 0 )
		g_ignoreItems.erase( g_ignoreItems.begin()+idx-1 );

	RewriteIgnoreSettings();
	if ( IgnoreWndHwnd )
		SendMessage(IgnoreWndHwnd, IRC_REBUILDIGNORELIST, 0, 0);
	return true; 
} 
