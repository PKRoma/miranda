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

// This file holds functions that are called upon receiving
// certain commands from the server.

#include "irc.h"
#include <algorithm>

using namespace irc;

VOID CALLBACK IdentTimerProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	CIrcProto* ppro = GetTimerOwner( idEvent );
	if ( !ppro )
		return;

	ppro->KillChatTimer( ppro->IdentTimer );
	if ( ppro->m_iDesiredStatus == ID_STATUS_OFFLINE || ppro->m_iDesiredStatus == ID_STATUS_CONNECTING )
		return;

	if ( ppro->IsConnected() && ppro->m_identTimer )
		ppro->KillIdent();
}

VOID CALLBACK TimerProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	CIrcProto* ppro = GetTimerOwner( idEvent );
	if ( !ppro )
		return;

	ppro->KillChatTimer( ppro->InitTimer );
	if ( ppro->m_iDesiredStatus == ID_STATUS_OFFLINE || ppro->m_iDesiredStatus == ID_STATUS_CONNECTING )
		return;

	if ( ppro->m_forceVisible )
		ppro->PostIrcMessage( _T("/MODE %s -i"), ppro->m_info.sNick.c_str());

	if ( lstrlenA( ppro->m_myHost ) == 0 && ppro->IsConnected() )
		ppro->DoUserhostWithReason(2, (_T("S") + ppro->m_info.sNick).c_str(), true, _T("%s"), ppro->m_info.sNick.c_str());
}

VOID CALLBACK KeepAliveTimerProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	CIrcProto* ppro = GetTimerOwner( idEvent );
	if ( !ppro )
		return;

	if ( !ppro->m_sendKeepAlive || ( ppro->m_iDesiredStatus == ID_STATUS_OFFLINE || ppro->m_iDesiredStatus == ID_STATUS_CONNECTING )) {
		ppro->KillChatTimer( ppro->KeepAliveTimer );
		return;
	}

	TCHAR temp2[270];
	if ( !ppro->m_info.sServerName.empty())
		mir_sntprintf(temp2, SIZEOF(temp2), _T("PING %s"), ppro->m_info.sServerName.c_str());
	else
		mir_sntprintf(temp2, SIZEOF(temp2), _T("PING %u"), time(0));

	if ( ppro->IsConnected())
		*ppro << CIrcMessage( ppro, temp2, ppro->getCodepage(), false, false);
}

VOID CALLBACK OnlineNotifTimerProc3( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	CIrcProto* ppro = GetTimerOwner( idEvent );
	if ( !ppro )
		return;

	if ( !ppro->m_channelAwayNotification || 
		  ppro->m_iDesiredStatus == ID_STATUS_OFFLINE || ppro->m_iDesiredStatus == ID_STATUS_CONNECTING || 
		  ( !ppro->m_autoOnlineNotification && !ppro->bTempForceCheck) || ppro->bTempDisableCheck ) {
		ppro->KillChatTimer( ppro->OnlineNotifTimer3 );
		ppro->m_channelsToWho = _T("");
		return;
	}

	TString name = GetWord( ppro->m_channelsToWho.c_str(), 0 );
	if ( name.empty()) {
		ppro->m_channelsToWho = _T("");
		int count = (int)CallServiceSync(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)ppro->m_szModuleName);
		for ( int i = 0; i < count; i++ ) {
			GC_INFO gci = {0};
			gci.Flags = BYINDEX | NAME | TYPE | COUNT;
			gci.iItem = i;
			gci.pszModule = ppro->m_szModuleName;
			if ( !CallServiceSync( MS_GC_GETINFO, 0, (LPARAM)&gci ) && gci.iType == GCW_CHATROOM )
				if ( gci.iCount <= ppro->m_onlineNotificationLimit )
					ppro->m_channelsToWho += (TString)gci.pszName + _T(" ");
	}	}

	if ( ppro->m_channelsToWho.empty()) {
		ppro->SetChatTimer( ppro->OnlineNotifTimer3, 60*1000, OnlineNotifTimerProc3 );
		return;
	}

	name = GetWord( ppro->m_channelsToWho.c_str(), 0 );
	ppro->DoUserhostWithReason(2, _T("S") + name, true, _T("%s"), name.c_str());
	TString temp = GetWordAddress( ppro->m_channelsToWho.c_str(), 1 );
	ppro->m_channelsToWho = temp;
	if ( ppro->m_iTempCheckTime )
		ppro->SetChatTimer( ppro->OnlineNotifTimer3, ppro->m_iTempCheckTime*1000, OnlineNotifTimerProc3 );
	else
		ppro->SetChatTimer( ppro->OnlineNotifTimer3, ppro->m_onlineNotificationTime*1000, OnlineNotifTimerProc3 );
}

VOID CALLBACK OnlineNotifTimerProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	CIrcProto* ppro = GetTimerOwner( idEvent );
	if ( !ppro )
		return;

	if ( ppro->m_iDesiredStatus == ID_STATUS_OFFLINE || ppro->m_iDesiredStatus == ID_STATUS_CONNECTING || 
		  ( !ppro->m_autoOnlineNotification && !ppro->bTempForceCheck) || ppro->bTempDisableCheck ) {
		ppro->KillChatTimer( ppro->OnlineNotifTimer );
		ppro->m_namesToWho = _T("");
		return;
	}

	TString name = GetWord( ppro->m_namesToWho.c_str(), 0);
	TString name2 = GetWord( ppro->m_namesToUserhost.c_str(), 0);

	if ( name.empty() && name2.empty()) {
		DBVARIANT dbv;
		char* szProto;

		HANDLE hContact = (HANDLE) CallService( MS_DB_CONTACT_FINDFIRST, 0, 0);
		while ( hContact ) {
		   szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		   if ( szProto != NULL && !lstrcmpiA( szProto, ppro->m_szModuleName )) {
			   BYTE bRoom = ppro->getByte(hContact, "ChatRoom", 0);
			   if ( bRoom == 0 ) {
				   BYTE bDCC = ppro->getByte(hContact, "DCC", 0);
				   BYTE bHidden = DBGetContactSettingByte(hContact,"CList", "Hidden", 0);
					if ( bDCC == 0 && bHidden == 0 ) {
						if ( !ppro->getTString( hContact, "Default", &dbv )) {
							BYTE bAdvanced = ppro->getByte(hContact, "AdvancedMode", 0) ;
							if ( !bAdvanced ) {
								DBFreeVariant( &dbv );
								if ( !ppro->getTString( hContact, "Nick", &dbv )) {	
									ppro->m_namesToUserhost += (TString)dbv.ptszVal + _T(" ");
									DBFreeVariant( &dbv );
								}
							}
							else {
								DBFreeVariant( &dbv );
								DBVARIANT dbv2;
								
								TCHAR* DBNick = NULL;
								TCHAR* DBWildcard = NULL;
								if ( !ppro->getTString( hContact, "Nick", &dbv ))
									DBNick = dbv.ptszVal;
								if ( !ppro->getTString( hContact, "UWildcard", &dbv2 ))
									DBWildcard = dbv2.ptszVal;

								if ( DBNick && ( !DBWildcard || !WCCmp(CharLower(DBWildcard), CharLower(DBNick)))) 
									ppro->m_namesToWho += (TString)DBNick + _T(" ");
								else if( DBWildcard )
									ppro->m_namesToWho += (TString)DBWildcard + _T(" ");

								if ( DBNick )     DBFreeVariant(&dbv);
                        if ( DBWildcard ) DBFreeVariant(&dbv2);
			}	}	}	}	}

			hContact = (HANDLE) CallService( MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}	}

	if ( ppro->m_namesToWho.empty() && ppro->m_namesToUserhost.empty()) {
		ppro->SetChatTimer( ppro->OnlineNotifTimer, 60*1000, OnlineNotifTimerProc );
		return;
	}

	name = GetWord( ppro->m_namesToWho.c_str(), 0);
	name2 = GetWord( ppro->m_namesToUserhost.c_str(), 0);
	TString temp;
	if ( !name.empty()) {
		ppro->DoUserhostWithReason(2, _T("S") + name, true, _T("%s"), name.c_str());
		temp = GetWordAddress( ppro->m_namesToWho.c_str(), 1 );
		ppro->m_namesToWho = temp;
	}

	if ( !name2.empty()) {
		TString params;
		for ( int i = 0; i < 3; i++ ) {
			params = _T("");
			for ( int j = 0; j < 5; j++ ) 
				params += GetWord( ppro->m_namesToUserhost.c_str(), i *5 + j) + _T(" ");

			if ( params[0] != ' ' )
				ppro->DoUserhostWithReason(1, (TString)_T("S") + params, true, params);
		}
		temp = GetWordAddress( ppro->m_namesToUserhost.c_str(), 15 );
		ppro->m_namesToUserhost = temp;
	}

	if ( ppro->m_iTempCheckTime )
		ppro->SetChatTimer( ppro->OnlineNotifTimer, ppro->m_iTempCheckTime*1000, OnlineNotifTimerProc );
	else
		ppro->SetChatTimer( ppro->OnlineNotifTimer, ppro->m_onlineNotificationTime*1000, OnlineNotifTimerProc );
}

int CIrcProto::AddOutgoingMessageToDB(HANDLE hContact, TCHAR* msg)
{
	if ( m_iDesiredStatus == ID_STATUS_OFFLINE || m_iDesiredStatus == ID_STATUS_CONNECTING )
		return 0;

	TString S = DoColorCodes( msg, TRUE, FALSE );

	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	dbei.szModule = m_szModuleName;
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
	IPRESOLVE* ipr = (IPRESOLVE *) di;

	EnterCriticalSection( &ipr->ppro->m_resolve);

	if ( ipr != NULL && (ipr->iType == IP_AUTO && lstrlenA(ipr->ppro->m_myHost) == 0 || ipr->iType == IP_MANUAL )) {
		hostent* myhost = gethostbyname( ipr->sAddr.c_str() );
		if ( myhost ) {
			IN_ADDR in;
			memcpy( &in, myhost->h_addr, 4 );
			if ( ipr->iType == IP_AUTO )
				mir_snprintf( ipr->ppro->m_myHost, sizeof( ipr->ppro->m_myHost ), "%s", inet_ntoa( in ));
			else
				mir_snprintf( ipr->ppro->m_mySpecifiedHostIP, sizeof( ipr->ppro->m_mySpecifiedHostIP ), "%s", inet_ntoa( in ));
	}	}
	
	LeaveCriticalSection( &ipr->ppro->m_resolve );
	delete ipr;
}

bool CIrcProto::OnIrc_PING(const CIrcMessage* pmsg)
{
	TCHAR szResponse[100];
	mir_sntprintf(szResponse, SIZEOF(szResponse), _T("PONG %s"), pmsg->parameters[0].c_str());
	*this << CIrcMessage( this, szResponse, getCodepage() );
	return false;
}

bool CIrcProto::OnIrc_WELCOME( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters[0] != m_info.sNick )
		m_info.sNick = pmsg->parameters[0];

	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		static TCHAR host[1024];
		int i = 0;
		TString word = GetWord( pmsg->parameters[1].c_str(), i );
		while ( !word.empty()) {
			if ( _tcschr( word.c_str(), '!') && _tcschr( word.c_str(), '@' )) {
				lstrcpyn( host, word.c_str(), SIZEOF(host));
				TCHAR* p1 = _tcschr( host, '@' );
				if ( p1 )
					mir_forkthread( ResolveIPThread, new IPRESOLVE( this, _T2A(p1+1), IP_AUTO ));
			}
			
			word = GetWord(pmsg->parameters[1].c_str(), ++i);
	}	}			
		
	ShowMessage( pmsg ); 
	return true;
}

bool CIrcProto::OnIrc_WHOTOOLONG( const CIrcMessage* pmsg )
{
	TString command = GetNextUserhostReason(2);
	if ( command[0] == 'U' )
		ShowMessage( pmsg ); 

	return true;
}

bool CIrcProto::OnIrc_BACKFROMAWAY( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		int Temp = m_iDesiredStatus;
		m_iDesiredStatus = ID_STATUS_ONLINE;
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_ONLINE);

		if ( m_perform )
			DoPerform( "Event: Available" );
	}			
		
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_SETAWAY( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		int Temp = m_iDesiredStatus;
		m_iDesiredStatus =  ID_STATUS_AWAY;
		ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_AWAY);

		if ( m_perform ) {
			switch ( m_iStatus ) {
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
				m_iStatus = ID_STATUS_AWAY;
				DoPerform( "Event: Away" );
				break;
	}	}	}	
		
	ShowMessage( pmsg ); 
	return true;
}

bool CIrcProto::OnIrc_JOIN( const CIrcMessage* pmsg )
{
	if (pmsg->parameters.size() > 0 && pmsg->m_bIncoming && pmsg->prefix.sNick != m_info.sNick) {
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_JOIN, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), NULL, _T("Normal"), host.c_str(), NULL, true, false); 
		DoEvent(GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[0].c_str(),pmsg->prefix.sNick.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE); 
	}
	else ShowMessage( pmsg ); 

	return true;
}

bool CIrcProto::OnIrc_QUIT( const CIrcMessage* pmsg )
{
	if (pmsg->m_bIncoming) 
	{
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_QUIT, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters.size()>0?pmsg->parameters[0].c_str():NULL, NULL, host.c_str(), NULL, true, false);
		struct CONTACT user = { (LPTSTR)pmsg->prefix.sNick.c_str(), (LPTSTR)pmsg->prefix.sUser.c_str(), (LPTSTR)pmsg->prefix.sHost.c_str(), false, false, false};
		CList_SetOffline( &user );
		if ( pmsg->prefix.sNick == m_info.sNick ) {
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			gce.cbSize = sizeof(GCEVENT);
			gcd.pszID = NULL;
			gcd.pszModule = m_szModuleName;
			gcd.iType = GC_EVENT_CONTROL;
			gce.pDest = &gcd;
			CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
		}
	}
	else ShowMessage( pmsg );

	return true;
}

bool CIrcProto::OnIrc_PART( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters.size() > 0 && pmsg->m_bIncoming ) {
		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_PART, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters.size()>1?pmsg->parameters[1].c_str():NULL, NULL, host.c_str(), NULL, true, false); 
		if ( pmsg->prefix.sNick == m_info.sNick ) {
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			TString S = MakeWndID( pmsg->parameters[0].c_str());
			gce.cbSize = sizeof(GCEVENT);
			gcd.ptszID = ( TCHAR* )S.c_str();
			gce.dwFlags = GC_TCHAR;
			gcd.pszModule = m_szModuleName;
			gcd.iType = GC_EVENT_CONTROL;
			gce.pDest = &gcd;
			CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
		}
	}
	else ShowMessage( pmsg ); 

	return true;
}

bool CIrcProto::OnIrc_KICK( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 )
		DoEvent( GC_EVENT_KICK, pmsg->parameters[0].c_str(), pmsg->parameters[1].c_str(), pmsg->parameters.size()>2?pmsg->parameters[2].c_str():NULL, pmsg->prefix.sNick.c_str(), NULL, NULL, true, false); 
	else
		ShowMessage( pmsg ); 

	if ( pmsg->parameters[1] == m_info.sNick ) {
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		TString S = MakeWndID( pmsg->parameters[0].c_str() );
		gce.cbSize = sizeof(GCEVENT);
		gce.dwFlags = GC_TCHAR;
		gcd.ptszID = ( TCHAR* )S.c_str();
		gcd.pszModule = m_szModuleName;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);

		if ( m_rejoinIfKicked ) {
			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if ( wi && wi->pszPassword )
				PostIrcMessage( _T("/JOIN %s %s"), pmsg->parameters[0].c_str(), wi->pszPassword);
			else
				PostIrcMessage( _T("/JOIN %s"), pmsg->parameters[0].c_str());
	}	}

	return true;
}

bool CIrcProto::OnIrc_MODEQUERY( const CIrcMessage* pmsg )
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

bool CIrcProto::OnIrc_MODE( const CIrcMessage* pmsg )
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
						if ( !_tcscmp(pmsg->parameters[2].c_str(), m_info.sNick.c_str() )) {
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
						DoEvent( bAdd ? GC_EVENT_ADDSTATUS : GC_EVENT_REMOVESTATUS, pmsg->parameters[0].c_str(), pmsg->parameters[iParametercount].c_str(), pmsg->prefix.sNick.c_str(), sStatus.c_str(), NULL, NULL, m_oldStyleModes?false:true, false); 
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

			if ( m_oldStyleModes ) {
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

bool CIrcProto::OnIrc_NICK( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 0 ) {
		bool bIsMe = pmsg->prefix.sNick.c_str() == m_info.sNick ? true : false;

		if (( m_info.sNick == pmsg->prefix.sNick) && (pmsg->parameters.size() > 0 )) {
			m_info.sNick = pmsg->parameters[0];
			DBWriteContactSettingTString(NULL, m_szModuleName,"Nick", m_info.sNick.c_str());
		}

		TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_NICK, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, host.c_str(), NULL, true, bIsMe); 
		DoEvent(GC_EVENT_CHUID, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, NULL, NULL, true, false); 
		
		struct CONTACT user = { (TCHAR*)pmsg->prefix.sNick.c_str(), (TCHAR*)pmsg->prefix.sUser.c_str(), (TCHAR*)pmsg->prefix.sHost.c_str(), false, false, false};
		HANDLE hContact = CList_FindContact(&user);
		if (hContact) {
			if ( getWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
				setWord(hContact, "Status", ID_STATUS_ONLINE);
			setTString(hContact, "Nick", pmsg->parameters[0].c_str());
			setTString(hContact, "User", pmsg->prefix.sUser.c_str());
			setTString(hContact, "Host", pmsg->prefix.sHost.c_str());
		}
	}
	else ShowMessage( pmsg );

	return true;
}

bool CIrcProto::OnIrc_NOTICE( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		if ( IsCTCP( pmsg ))
			return true;

		if ( !m_ignore || !IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'n' )) {
			TString S;
			TString S2;
			TString S3;
			if ( pmsg->prefix.sNick.length() > 0 )
				S = pmsg->prefix.sNick;
			else
				S = m_info.sNetwork;
			S3 = m_info.sNetwork;
			if ( IsChannel( pmsg->parameters[0] ))
				S2 = pmsg->parameters[0].c_str();
			else {
				GC_INFO gci = {0};
				gci.Flags = BYID | TYPE;
				gci.pszModule = m_szModuleName;

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

bool CIrcProto::OnIrc_YOURHOST( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		static const TCHAR* lpszFmt = _T("Your host is %99[^ \x5b,], running version %99s");
		TCHAR szHostName[100], szVersion[100];
		if( _stscanf(pmsg->parameters[1].c_str(), lpszFmt, &szHostName, &szVersion) > 0 )
			m_info.sServerName = szHostName;
		if (pmsg->parameters[0] != m_info.sNick)
			m_info.sNick = pmsg->parameters[0];
	}
	
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_INVITE( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && ( m_ignore && IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'i' )))
		return true;

	if ( pmsg->m_bIncoming && m_joinOnInvite && pmsg->parameters.size() >1 && lstrcmpi(pmsg->parameters[0].c_str(), m_info.sNick.c_str()) == 0 ) 
		PostIrcMessage( _T("/JOIN %s"), pmsg->parameters[1].c_str());

	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_PINGPONG( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->sCommand == _T("PING")) {
		TCHAR szResponse[100];
		mir_sntprintf(szResponse, SIZEOF(szResponse), _T("PONG %s"), pmsg->parameters[0].c_str());
		*this << CIrcMessage( this, szResponse, getCodepage() );
	}

	return true;
}

bool CIrcProto::OnIrc_PRIVMSG( const CIrcMessage* pmsg )
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

			if (( m_ignore && IsIgnored( pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q' ))) {
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
			setTString(ccs.hContact, "User", pmsg->prefix.sUser.c_str());
			setTString(ccs.hContact, "Host", pmsg->prefix.sHost.c_str());
			CallService( MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			#if defined( _UNICODE )
				mir_free( pre.szMessage );
			#endif
			return true;
		}
		
		if ( bIsChannel ) {
			if ( !(pmsg->m_bIncoming && m_ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm' ))) {
				if ( !pmsg->m_bIncoming )
					ReplaceString( mess, _T("%%"), _T("%"));
				DoEvent(GC_EVENT_MESSAGE, pmsg->parameters[0].c_str(), pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():m_info.sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 				
			}
			return true;
	}	}

	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::IsCTCP( const CIrcMessage* pmsg )
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
			DoEvent( GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
			return true;
		}

		// extract the type of ctcp command
		TString ocommand = GetWord(mess.c_str(), 0);
		TString command = GetWord(mess.c_str(), 0);
		transform (command.begin(),command.end(), command.begin(), tolower);

		// should it be ignored?
		if ( m_ignore ) {
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

						DoEvent(GC_EVENT_ACTION, pmsg->parameters[0].c_str(), pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():m_info.sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 
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
				PostIrcMessage( _T("/NOTICE %s \001FINGER %s (%s)\001"), pmsg->prefix.sNick.c_str(), m_name, m_userID);
				
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
				PostIrcMessage( _T("/NOTICE %s \001USERINFO %s\001"), pmsg->prefix.sNick.c_str(), m_userInfo );
				
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
					if ( m_manualHost )
						ulAdr = ConvertIPToInteger( m_mySpecifiedHostIP );
					else
						ulAdr = ConvertIPToInteger( m_IPFromServer ? m_myHost : m_myLocalHost );

					if ( bIsChat && !m_DCCChatEnabled)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: Chat request from %s denied"),pmsg->prefix.sNick.c_str());

					else if(type == _T("send") && !m_DCCFileEnabled)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: File transfer request from %s denied"),pmsg->prefix.sNick.c_str());

					else if(type == _T("send") && !iPort && ulAdr == 0)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: Reverse file transfer request from %s denied [No local IP]"),pmsg->prefix.sNick.c_str());

					if ( sFile.empty() || dwAdr == 0 || dwSize == 0 || iPort == 0 && sToken.empty())
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Malformed CTCP request from %s [%s]"),pmsg->prefix.sNick.c_str(), mess.c_str());
				
					if ( szTemp[0] ) {
						DoEvent( GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
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

					if ( type == _T("resume") && !m_DCCFileEnabled)
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC: File transfer resume request from %s denied"),pmsg->prefix.sNick.c_str());

					if ( sToken.empty() && iPort == 0 || sFile.empty())
						mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("DCC ERROR: Malformed CTCP request from %s [%s]"),pmsg->prefix.sNick.c_str(), mess.c_str());
				
					if ( szTemp[0] ) {
						DoEvent( GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
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
					if ( m_DCCChatIgnore == 1 || 
						m_DCCChatIgnore == 2 && hContact && 
						DBGetContactSettingByte(hContact,"CList", "NotOnList", 0) == 0 && 
						DBGetContactSettingByte(hContact,"CList", "Hidden", 0) == 0)
					{
						TString host = pmsg->prefix.sUser + _T("@") + pmsg->prefix.sHost;
						CList_AddDCCChat(pmsg->prefix.sNick, host, dwAdr, iPort); // add a CHAT event to the clist
					}
					else {
						TCHAR szTemp[512];
						mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("DCC: Chat request from %s denied"),pmsg->prefix.sNick.c_str());
						DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
				}	}

				// remote requested that the file should be resumed
				if ( type == _T("resume")) {
					CDccSession* dcc;
					if ( sToken.empty())
						dcc = FindDCCSendByPort( iPort );
					else
						dcc = FindPassiveDCCSend( _ttoi( sToken.c_str())); // reverse ft

					if ( dcc ) {
						InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME);
						InterlockedExchange(&dcc->dwResumePos, dwSize); // dwSize is the resume position
						PostIrcMessage( _T("/PRIVMSG %s \001DCC ACCEPT %s\001"), pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 2));
				}	}

				// remote accepted your request for a file resume
				if ( type == _T("accept")) {
					CDccSession* dcc;
					if ( sToken.empty())
						dcc = FindDCCRecvByPortAndName(iPort, pmsg->prefix.sNick.c_str());
					else
						dcc = FindPassiveDCCRecv(pmsg->prefix.sNick, sToken); // reverse ft

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
						CDccSession* dcc = FindPassiveDCCSend( _ttoi( sToken.c_str()));
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

							setTString(hContact, "User", pmsg->prefix.sUser.c_str());
							setTString(hContact, "Host", pmsg->prefix.sHost.c_str());

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
					setTString( hContact, "MirVer", DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE)); 
				}

			// if the whois window is visible and the ctcp reply belongs to the user in it, then show the reply in the whois window
			if ( m_whoisDlg && IsWindowVisible( m_whoisDlg->GetHwnd() )) {
				m_whoisDlg->m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
				if ( lstrcmpi(szTemp, pmsg->prefix.sNick.c_str()) == 0 ) {
					if ( pmsg->m_bIncoming && (command == _T("version") || command == _T("userinfo") || command == _T("time"))) {
						SetActiveWindow( m_whoisDlg->GetHwnd() );
						m_whoisDlg->m_Reply.SetText( DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE));
						return true;
					}
					if (pmsg->m_bIncoming && command == _T("ping")) {
						SetActiveWindow( m_whoisDlg->GetHwnd() );
						int s = (int)time(0) - (int)_ttol(GetWordAddress(mess.c_str(), 1));
						TCHAR szTemp[30];
						if ( s == 1 )
							mir_sntprintf( szTemp, SIZEOF(szTemp), _T("%u second"), s );
						else
							mir_sntprintf( szTemp, SIZEOF(szTemp), _T("%u seconds"), s );

						m_whoisDlg->m_Reply.SetText( DoColorCodes( szTemp, TRUE, FALSE ));
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

bool CIrcProto::OnIrc_NAMES( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 3 )
		sNamesList += pmsg->parameters[3] + _T(" ");
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_ENDNAMES( const CIrcMessage* pmsg )
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

				if ( !lstrcmpi( name.substr(index, name.length()).c_str(), m_info.sNick.c_str())) {
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
			gcw.pszModule = m_szModuleName;
			gcw.ptszName = sChanName;
			if ( !CallServiceSync( MS_GC_NEWSESSION, 0, ( LPARAM )&gcw )) {
				DBVARIANT dbv;
				GCDEST gcd = {0};
				GCEVENT gce = {0};
				TString sTemp;
				int i = 0;

				PostIrcMessage( _T("/MODE %s"), sChanName );

				gcd.ptszID = ( TCHAR* )sID.c_str();
				gcd.pszModule = m_szModuleName;
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
					BOOL bIsMe = ( !lstrcmpi( gce.ptszNick, m_info.sNick.c_str())) ? TRUE : FALSE;
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
					wi->codepage = getCodepage();
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
	
				if ( !getTString( "JTemp", &dbv )) {
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
						DBDeleteContactSetting(NULL, m_szModuleName, "JTemp");
					else
						setTString("JTemp", save.c_str());
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

bool CIrcProto::OnIrc_INITIALTOPIC( const CIrcMessage* pmsg )
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

bool CIrcProto::OnIrc_INITIALTOPICNAME( const CIrcMessage* pmsg )
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

bool CIrcProto::OnIrc_TOPIC( const CIrcMessage* pmsg )
{
	if ( pmsg->parameters.size() > 1 && pmsg->m_bIncoming ) {
		DoEvent( GC_EVENT_TOPIC, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str(), NULL, sTopicTime.empty() ? NULL : sTopicTime.c_str(), NULL, true, false);
		AddWindowItemData(pmsg->parameters[0].c_str(), 0, 0, 0, pmsg->parameters[1].c_str());
	}
	ShowMessage( pmsg );
	return true;
}

static void __stdcall sttShowDlgList( void* param )
{
	CIrcProto* ppro = ( CIrcProto* )param;
	if ( ppro->m_listDlg == NULL ) {
		ppro->m_listDlg = new CListDlg( ppro );
		ppro->m_listDlg->Show();
	}
	SetEvent( ppro->m_evWndCreate );
}

bool CIrcProto::OnIrc_LISTSTART( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		CallFunctionAsync( sttShowDlgList, this );
		WaitForSingleObject( m_evWndCreate, INFINITE );
		m_channelNumber = 0;
	}

	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_LIST( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming == 1 && m_listDlg && pmsg->parameters.size() > 2 ) {
		m_channelNumber++;
		LVITEM lvItem;
		HWND hListView = GetDlgItem( m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW );
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
		if ( m_noOfChannels > 0 )
			percent = (int)(m_channelNumber*100) / m_noOfChannels;

		TCHAR text[100];
		if ( percent < 100)
			mir_sntprintf(text, SIZEOF(text), TranslateT("Downloading list (%u%%) - %u channels"), percent, m_channelNumber);
		else
			mir_sntprintf(text, SIZEOF(text), TranslateT("Downloading list - %u channels"), m_channelNumber);
		SetDlgItemText(m_listDlg->GetHwnd(), IDC_TEXT, text);
	}
	
	return true;
}

bool CIrcProto::OnIrc_LISTEND( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && m_listDlg ) {
		EnableWindow(GetDlgItem(m_listDlg->GetHwnd(), IDC_JOIN), true);
		ListView_SetSelectionMark(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 0);		
		ListView_SetColumnWidth(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 1, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 2, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 3, LVSCW_AUTOSIZE);
		m_listDlg->UpdateList();

		TCHAR text[100];
		mir_sntprintf( text, SIZEOF(text), TranslateT("Done: %u channels"), m_channelNumber );
		int percent = 100;
		if ( m_noOfChannels > 0 )
			percent = (int)(m_channelNumber*100) / m_noOfChannels;
		if ( percent < 70 ) {
			lstrcat( text, _T(" "));
			lstrcat( text, TranslateT("(probably truncated by server)"));
		}
		SetDlgItemText( m_listDlg->GetHwnd(), IDC_TEXT, text );
	}
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_BANLIST( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 2 ) {
		if ( m_managerDlg->GetHwnd() && (
			m_managerDlg->m_radio1.GetState() && pmsg->sCommand == _T("367") ||
			m_managerDlg->m_radio2.GetState() && pmsg->sCommand == _T("346") ||
			m_managerDlg->m_radio3.GetState() && pmsg->sCommand == _T("348")) && 
			!m_managerDlg->m_radio1.Enabled() && !m_managerDlg->m_radio2.Enabled() && !m_managerDlg->m_radio3.Enabled())
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

			SendDlgItemMessage(m_managerDlg->GetHwnd(), IDC_LIST, LB_ADDSTRING, 0, (LPARAM)S.c_str());
	}	}

	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_BANLISTEND( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
		if ( m_managerDlg->GetHwnd() && 
			 ( m_managerDlg->m_radio1.GetState() && pmsg->sCommand == _T("368")
			|| m_managerDlg->m_radio2.GetState() && pmsg->sCommand == _T("347")
			|| m_managerDlg->m_radio3.GetState() && pmsg->sCommand == _T("349")) && 
			!m_managerDlg->m_radio1.Enabled() && !m_managerDlg->m_radio2.Enabled() && !m_managerDlg->m_radio3.Enabled())
		{
			if ( strchr( sChannelModes.c_str(), 'b' ))
				m_managerDlg->m_radio1.Enable();
			if ( strchr( sChannelModes.c_str(), 'I' ))
				m_managerDlg->m_radio2.Enable();
			if ( strchr( sChannelModes.c_str(), 'e' ))
				m_managerDlg->m_radio3.Enable();
			if ( !IsDlgButtonChecked(m_managerDlg->GetHwnd(), IDC_NOTOP))
				m_managerDlg->m_add.Enable();
	}	}
	
	ShowMessage( pmsg );
	return true;
}

static void __stdcall sttShowWhoisWnd( void* param )
{
	CIrcMessage* pmsg = ( CIrcMessage* )param;
	CIrcProto* ppro = ( CIrcProto* )pmsg->m_proto;
	if ( ppro->m_whoisDlg == NULL ) {
		ppro->m_whoisDlg = new CWhoisDlg( ppro );
		ppro->m_whoisDlg->Show();
	}
	SetEvent( ppro->m_evWndCreate );

	ppro->m_whoisDlg->ShowMessage( pmsg );
	delete pmsg;
}

bool CIrcProto::OnIrc_WHOIS_NAME( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 5 && m_manualWhoisCount > 0 ) {
		CallFunctionAsync( sttShowWhoisWnd, new CIrcMessage( *pmsg ));
		WaitForSingleObject( m_evWndCreate, INFINITE );
	}
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_CHANNELS( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.size() > 2 && m_manualWhoisCount > 0 )
		m_whoisDlg->m_InfoChannels.SetText( pmsg->parameters[2].c_str());
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_AWAY( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.size() > 2 && m_manualWhoisCount > 0 )
		m_whoisDlg->m_InfoAway2.SetText( pmsg->parameters[2].c_str());
	if ( m_manualWhoisCount < 1 && pmsg->m_bIncoming && pmsg->parameters.size() > 2 )
		WhoisAwayReply = pmsg->parameters[2];
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_OTHER( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.size() > 2 && m_manualWhoisCount > 0 ) {
		TCHAR temp[1024], temp2[1024];
		m_whoisDlg->m_InfoOther.GetText( temp, 1000 );
		lstrcat( temp, _T("%s\r\n"));
		mir_sntprintf( temp2, 1020, temp, pmsg->parameters[2].c_str()); 
		m_whoisDlg->m_InfoOther.SetText( temp2 );
	}
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_END( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 && m_manualWhoisCount < 1 ) {
		CONTACT user = { (TCHAR*)pmsg->parameters[1].c_str(), NULL, NULL, false, false, true};
		HANDLE hContact = CList_FindContact( &user );
		if ( hContact )
			ProtoBroadcastAck( m_szModuleName, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM)( char* )_T2A( WhoisAwayReply.c_str(), getCodepage()));
	}

	m_manualWhoisCount--;
	if (m_manualWhoisCount < 0)
		m_manualWhoisCount = 0;
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_IDLE( const CIrcMessage* pmsg )
{
	int D = 0;
	int H = 0;
	int M = 0;
	int S = 0;
	if ( pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.size() > 2 && m_manualWhoisCount > 0 ) {
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
		else
			temp[0] = 0;

		TCHAR temp3[256];
		TCHAR tTimeBuf[128], *tStopStr;
		time_t ttTime = _tcstol( pmsg->parameters[3].c_str(), &tStopStr, 10);
		_tcsftime(tTimeBuf, 128, _T("%c"), localtime(&ttTime));
		mir_sntprintf( temp3, SIZEOF(temp3), _T("online since %s, idle %s"), tTimeBuf, temp);
		m_whoisDlg->m_AwayTime.SetText( temp3 );	
	}
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_SERVER( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.size() > 2 && m_manualWhoisCount > 0 )
		m_whoisDlg->m_InfoServer.SetText( pmsg->parameters[2].c_str());
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_AUTH( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.size() > 2 && m_manualWhoisCount > 0 ) {
		if ( pmsg->sCommand == _T("330"))
			m_whoisDlg->m_InfoAuth.SetText( pmsg->parameters[2].c_str());
		else if ( pmsg->parameters[2] == _T("is an identified user") || pmsg->parameters[2] == _T("is a registered nick"))
			m_whoisDlg->m_InfoAuth.SetText( pmsg->parameters[2].c_str());
		else
			OnIrc_WHOIS_OTHER( pmsg );
	}
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_WHOIS_NO_USER( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 2 && !IsChannel( pmsg->parameters[1] )) {
		if ( m_whoisDlg )
			m_whoisDlg->ShowMessageNoUser( pmsg );
		
		CONTACT user = { (TCHAR*)pmsg->parameters[1].c_str(), NULL, NULL, false, false, false};
		HANDLE hContact = CList_FindContact( &user );
		if ( hContact ) {
			AddOutgoingMessageToDB( hContact, (TCHAR*)((TString)_T("> ") + pmsg->parameters[2] + (TString)_T(": ") + pmsg->parameters[1]).c_str() );

			DBVARIANT dbv;
			if ( !getTString( hContact, "Default", &dbv )) {
				setTString( hContact, "Nick", dbv.ptszVal );
				
				DBVARIANT dbv2;
				if ( getByte( hContact, "AdvancedMode", 0 ) == 0 )
					DoUserhostWithReason(1, ((TString)_T("S") + dbv.ptszVal).c_str(), true, dbv.ptszVal );
				else {
					if ( !getTString( hContact, "UWildcard", &dbv2 )) {
						DoUserhostWithReason(2, ((TString)_T("S") + dbv2.ptszVal).c_str(), true, dbv2.ptszVal );
						DBFreeVariant(&dbv2);
					}
					else DoUserhostWithReason(2, ((TString)_T("S") + dbv.ptszVal).c_str(), true, dbv.ptszVal );
				}
				setString(hContact, "User", "");
				setString(hContact, "Host", "");
				DBFreeVariant(&dbv);
	}	}	}

	ShowMessage( pmsg );
	return true;
}

static void __stdcall sttShowNickWnd( void* param )
{
	CIrcMessage* pmsg = ( CIrcMessage* )param;
	CIrcProto* ppro = pmsg->m_proto;
	if ( ppro->m_nickDlg == NULL ) {
		ppro->m_nickDlg = new CNickDlg( ppro );
		ppro->m_nickDlg->Show();
	}
	SetEvent( ppro->m_evWndCreate );
	SetWindowText( GetDlgItem( ppro->m_nickDlg->GetHwnd(), IDC_CAPTION ), TranslateT("Change nickname"));
	SetWindowText( GetDlgItem( ppro->m_nickDlg->GetHwnd(), IDC_TEXT    ), pmsg->parameters[2].c_str());
	ppro->m_nickDlg->m_Enick.SetText( pmsg->parameters[1].c_str());
	ppro->m_nickDlg->m_Enick.SendMsg( CB_SETEDITSEL, 0, MAKELPARAM(0,-1));
	delete pmsg;
}

bool CIrcProto::OnIrc_NICK_ERR( const CIrcMessage* pmsg )
{
	if (( nickflag || m_alternativeNick[0] == 0) && pmsg->m_bIncoming && pmsg->parameters.size() > 2 ) {
		CallFunctionAsync( sttShowNickWnd, new CIrcMessage( *pmsg ));
		WaitForSingleObject( m_evWndCreate, INFINITE );
	}
	else if ( pmsg->m_bIncoming ) {
		TCHAR m[200];
		mir_sntprintf( m, SIZEOF(m), _T("NICK %s"), m_alternativeNick );
		if ( IsConnected() )
			*this << irc::CIrcMessage( this, m, getCodepage() );

		nickflag = true;
	}

	ShowMessage( pmsg );
 	return true;
}

bool CIrcProto::OnIrc_JOINERROR( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming ) {
		DBVARIANT dbv;
		if ( !getTString( "JTemp", &dbv )) {
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
				DBDeleteContactSetting( NULL, m_szModuleName, "JTemp" );
			else
				setTString( "JTemp", save.c_str());
	}	}

	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_UNKNOWN( const CIrcMessage* pmsg )
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

bool CIrcProto::OnIrc_ENDMOTD( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && !bPerformDone )
		DoOnConnect( pmsg );
	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_NOOFCHANNELS( const CIrcMessage* pmsg )
{
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 )
		m_noOfChannels = StrToInt( pmsg->parameters[1].c_str());

	if ( pmsg->m_bIncoming && !bPerformDone )
		DoOnConnect( pmsg );

	ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_ERROR( const CIrcMessage* pmsg )
{
	if	( pmsg->m_bIncoming && !m_disableErrorPopups ) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = m_szModuleName;
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

bool CIrcProto::OnIrc_WHO_END( const CIrcMessage* pmsg )
{
	TString command = GetNextUserhostReason(2);
	if ( command[0] == 'S' ) {
		if ( pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) {
			// is it a channel?
			if ( IsChannel( pmsg->parameters[1] )) {
				TString S = _T("");
				TString SS = m_whoReply;
				TString User = GetWord( m_whoReply.c_str(), 0 );
				while ( !User.empty()) {
					if ( GetWord( m_whoReply.c_str(), 3)[0] == 'G' ) {
						S += User;
						S += _T("\t");
						DoEvent( GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[1].c_str(), User.c_str(), NULL, NULL, NULL, ID_STATUS_AWAY, FALSE, FALSE);
					}
					else DoEvent( GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[1].c_str(), User.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE);

					SS = GetWordAddress( m_whoReply.c_str(), 4 );
					if ( SS.empty())
						break;
					m_whoReply = SS;
					User = GetWord(m_whoReply.c_str(), 0);
				}
				
				DoEvent( GC_EVENT_SETSTATUSEX, pmsg->parameters[1].c_str(), NULL, S.empty() ? NULL : S.c_str(), NULL, NULL, GC_SSE_TABDELIMITED, FALSE, FALSE); 
				return true;
			}

			/// if it is not a channel
			TCHAR* UserList = mir_tstrdup( m_whoReply.c_str() );
			TCHAR* p1= UserList;
			m_whoReply = _T("");
			CONTACT user = { (TCHAR*)pmsg->parameters[1].c_str(), NULL, NULL, false, true, false};
			HANDLE hContact = CList_FindContact( &user );

			if ( hContact && getByte( hContact, "AdvancedMode", 0 ) == 1 ) {
				DBVARIANT dbv1, dbv2, dbv3, dbv4, dbv5, dbv6, dbv7;	
				TCHAR *DBDefault = NULL, *DBNick = NULL, *DBWildcard = NULL;
				TCHAR *DBUser = NULL, *DBHost = NULL, *DBManUser = NULL, *DBManHost = NULL;
				if ( !getTString(hContact, "Default", &dbv1)) DBDefault = dbv1.ptszVal;
				if ( !getTString(hContact, "Nick", &dbv2)) DBNick = dbv2.ptszVal;
				if ( !getTString(hContact, "UWildcard", &dbv3)) DBWildcard = dbv3.ptszVal;
				if ( !getTString(hContact, "UUser", &dbv4)) DBUser = dbv4.ptszVal;
				if ( !getTString(hContact, "UHost", &dbv5)) DBHost = dbv5.ptszVal;
				if ( !getTString(hContact, "User", &dbv6)) DBManUser = dbv6.ptszVal;
				if ( !getTString(hContact, "Host", &dbv7)) DBManHost = dbv7.ptszVal;
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
						if (away[0] == 'G' && getWord( hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_AWAY)
							setWord(hContact, "Status", ID_STATUS_AWAY);
						else if (away[0] == 'H' && getWord( hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_ONLINE)
							setWord(hContact, "Status", ID_STATUS_ONLINE);

						if (( DBNick && lstrcmpi( nick.c_str(), DBNick)) || !DBNick )
							setTString( hContact, "Nick", nick.c_str());
						if (( DBManUser && lstrcmpi( user.c_str(), DBManUser)) || !DBManUser )
							setTString( hContact, "User", user.c_str());
						if (( DBManHost && lstrcmpi(host.c_str(), DBManHost)) || !DBManHost )
							setTString(hContact, "Host", host.c_str());

						goto LBL_Exit;
					}
					p1 = GetWordAddress(p1, 4);
					away = GetWord(p1, 3);
				}
				
				if ( DBWildcard && DBNick && !WCCmp( CharLower( DBWildcard ), CharLower( DBNick ))) {
					setTString(hContact, "Nick", DBDefault);
					
					DoUserhostWithReason(2, ((TString)_T("S") + DBWildcard).c_str(), true, DBWildcard);
							
					setString(hContact, "User", "");
					setString(hContact, "Host", "");
					goto LBL_Exit;
				}

				if ( getWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE ) {
					setWord(hContact, "Status", ID_STATUS_OFFLINE);
					setTString(hContact, "Nick", DBDefault);
					setString(hContact, "User", "");
					setString(hContact, "Host", "");
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

bool CIrcProto::OnIrc_WHO_REPLY( const CIrcMessage* pmsg )
{
	TString command = PeekAtReasons(2);
	if ( pmsg->m_bIncoming && pmsg->parameters.size() > 6 && command[0] == 'S' ) {
		m_whoReply += pmsg->parameters[5] + _T(" ") +pmsg->parameters[2] + _T(" ") + pmsg->parameters[3] + _T(" ") + pmsg->parameters[6] + _T(" ");
		if ( lstrcmpi( pmsg->parameters[5].c_str(), m_info.sNick.c_str()) == 0 ) {
			TCHAR host[1024];
			lstrcpyn( host, pmsg->parameters[3].c_str(), 1024 );
			mir_forkthread( ResolveIPThread, new IPRESOLVE( this, _T2A(host), IP_AUTO ));
	}	}

	if ( command[0] == 'U' )
		ShowMessage( pmsg );
	return true;
}

bool CIrcProto::OnIrc_TRYAGAIN( const CIrcMessage* pmsg )
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

bool CIrcProto::OnIrc_USERHOST_REPLY( const CIrcMessage* pmsg )
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
					if ( hContact && getByte( hContact, "AdvancedMode", 0 ) == 0 ) {
						setWord(hContact, "Status", awaystatus == '-'? ID_STATUS_AWAY : ID_STATUS_ONLINE);
						setTString(hContact, "User", user.c_str());
						setTString(hContact, "Host", host.c_str());
						setTString(hContact, "Nick", nick.c_str());

						// If user found, remove from checklist
						for ( i = 0; i < (int)checklist.size(); i++ )
							if ( !lstrcmpi(checklist[i].c_str(), nick.c_str() ))
								checklist.erase(checklist.begin() + i);
					}
					break;
				}
				case 'I':	// m_ignore
					mess = _T("/IGNORE %question=\"");
					mess += TranslateT("Please enter the hostmask (nick!user@host)\nNOTE! Contacts on your contact list are never ignored");
					mess += (TString)_T("\",\"") + TranslateT("Ignore") + _T("\",\"*!*@") + host + _T("\"");
					if ( m_ignoreChannelDefault )
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

bool CIrcProto::OnIrc_SUPPORT( const CIrcMessage* pmsg )
{
	static const TCHAR* lpszFmt = _T("Try server %99[^ ,], port %19s");
	TCHAR szAltServer[100];
	TCHAR szAltPort[20];
	if ( pmsg->parameters.size() > 1 && _stscanf(pmsg->parameters[1].c_str(), lpszFmt, &szAltServer, &szAltPort) == 2 ) {
		ShowMessage( pmsg );
		lstrcpynA( m_serverName, _T2A(szAltServer), 99 );
		lstrcpynA( m_portStart, _T2A(szAltPort), 9 );

		m_noOfChannels = 0;
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

void CIrcProto::OnIrcDefault( const CIrcMessage* pmsg )
{
	ShowMessage( pmsg );
}

void CIrcProto::OnIrcDisconnected()
{
	m_statusMessage = _T("");
	DBDeleteContactSetting(NULL, m_szModuleName, "JTemp");
	bTempDisableCheck = false;
	bTempForceCheck = false;
	m_iTempCheckTime = 0;

	m_myHost[0] = '\0';

	int Temp = m_iDesiredStatus;
	KillChatTimer( OnlineNotifTimer );
	KillChatTimer( OnlineNotifTimer3 );
	KillChatTimer( KeepAliveTimer );
	KillChatTimer( InitTimer );
	KillChatTimer( IdentTimer );
	m_iDesiredStatus = ID_STATUS_OFFLINE;
	ProtoBroadcastAck(m_szModuleName,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp, ID_STATUS_OFFLINE);

	TString sDisconn = _T("\0035\002");
	sDisconn += TranslateT("*Disconnected*");
	DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, sDisconn.c_str(), NULL, NULL, NULL, true, false); 

	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gce.cbSize = sizeof(GCEVENT);
		gcd.pszModule = m_szModuleName;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
	}

	if ( !Miranda_Terminated() )
		CList_SetAllOffline( m_disconnectDCCChats );
	setTString( "Nick", m_nick );
	
	CLISTMENUITEM clmi = {0};
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnConnect

static void __stdcall sttMainThrdOnConnect( void* param )
{
	CIrcProto* ppro = ( CIrcProto* )param;

	ppro->SetChatTimer( ppro->InitTimer, 1*1000, TimerProc );
	if ( ppro->m_identTimer )
		ppro->SetChatTimer( ppro->IdentTimer, 60*1000, IdentTimerProc );
	if ( ppro->m_sendKeepAlive )
		ppro->SetChatTimer( ppro->KeepAliveTimer, 60*1000, KeepAliveTimerProc );
	if ( ppro->m_autoOnlineNotification && !ppro->bTempDisableCheck || ppro->bTempForceCheck ) {
		ppro->SetChatTimer( ppro->OnlineNotifTimer, 1000, OnlineNotifTimerProc );
		if ( ppro->m_channelAwayNotification )
			ppro->SetChatTimer( ppro->OnlineNotifTimer3, 3000, OnlineNotifTimerProc3);
}	}

bool CIrcProto::DoOnConnect( const CIrcMessage* pmsg )
{
	bPerformDone = true;
	nickflag = true;	

	CLISTMENUITEM clmi = {0};
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );

	int Temp = m_iDesiredStatus;
	m_iDesiredStatus = ID_STATUS_ONLINE;
	ProtoBroadcastAck( m_szModuleName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE )Temp, ID_STATUS_ONLINE );

	if ( !m_statusMessage.empty()) {
		TString S = _T("/AWAY ");
		S += m_statusMessage;
		ReplaceString( S, _T("\r\n"), _T(" "));
		PostIrcMessage( S.c_str());
	}
	
	if ( m_perform ) {
		DoPerform( "ALL NETWORKS" );
		if ( IsConnected() )
			DoPerform( _T2A( m_info.sNetwork.c_str()));
	}

	if ( m_rejoinChannels ) {
		int count = CallServiceSync( MS_GC_GETSESSIONCOUNT, 0, (LPARAM)m_szModuleName);
		for ( int i = 0; i < count ; i++ ) {
			GC_INFO gci = {0};
			gci.Flags = BYINDEX | DATA | NAME | TYPE;
			gci.iItem = i;
			gci.pszModule = m_szModuleName;
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
		gcd.pszModule = m_szModuleName;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_ONLINE, (LPARAM)&gce);
	}

	CallFunctionAsync( sttMainThrdOnConnect, this );
	return 0;
}

static void __cdecl AwayWarningThread(LPVOID di)
{
	MessageBox(NULL, TranslateT("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), TranslateT("IRC Error"), MB_OK);
}

int CIrcProto::DoPerform( const char* event )
{
	String sSetting = String("PERFORM:") + event;
	transform( sSetting.begin(), sSetting.end(), sSetting.begin(), toupper );

	DBVARIANT dbv;
	if ( !getTString( sSetting.c_str(), &dbv )) {
		if ( !my_strstri( dbv.ptszVal, _T("/away")))
			PostIrcMessageWnd( NULL, NULL, dbv.ptszVal );
		else
			mir_forkthread( AwayWarningThread, NULL  );
		DBFreeVariant( &dbv );
		return 1;
	}
	return 0;
}

int CIrcProto::IsIgnored( TString nick, TString address, TString host, char type) 
{ 
	return IsIgnored( nick + _T("!") + address + _T("@") + host, type );
}

int CIrcProto::IsIgnored( TString user, char type ) 
{
	for ( size_t i=0; i < m_ignoreItems.size(); i++ ) {
		CIrcIgnoreItem& C = m_ignoreItems[i];

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

			if ( IsConnected() && !lstrcmpi( C.network.c_str(), m_info.sNetwork.c_str()))
				return i+1;
	}	}

	return 0; 
}

bool CIrcProto::AddIgnore( const TCHAR* mask, const TCHAR* flags, const TCHAR* network ) 
{ 
	RemoveIgnore( mask );
	m_ignoreItems.push_back( CIrcIgnoreItem( mask, (_T("+") + TString(flags)).c_str(), network ));
	RewriteIgnoreSettings();

	if ( m_ignoreDlg )
		m_ignoreDlg->RebuildList();
	return true;
}  

bool CIrcProto::RemoveIgnore( const TCHAR* mask ) 
{ 
	int idx;
	while (( idx = IsIgnored( mask, '\0')) != 0 )
		m_ignoreItems.erase( m_ignoreItems.begin()+idx-1 );
	RewriteIgnoreSettings();

	if ( m_ignoreDlg )
		m_ignoreDlg->RebuildList();
	return true; 
} 
