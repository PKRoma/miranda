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
#include "tchar.h"
#include <stdio.h>
#include <algorithm>

UINT_PTR	DCCTimer;	
CRITICAL_SECTION m_resolve;
int i = 0;

#define DCCCHATTIMEOUT 300
#define DCCSENDTIMEOUT 120

using namespace irc;

int CDccSession::nDcc = 0;

////////////////////////////////////////////////////////////////////

CIrcMessage::CIrcMessage() :
	m_bIncoming(false),
	m_bNotify(true)
{
	m_codePage = g_ircSession.getCodepage();
}

CIrcMessage::CIrcMessage(const TCHAR* lpszCmdLine, int codepage, bool bIncoming, bool bNotify) :
	m_bIncoming( bIncoming ),
	m_bNotify( bNotify ),
	m_codePage( codepage )
{
	ParseIrcCommand(lpszCmdLine);
}

CIrcMessage::CIrcMessage(const CIrcMessage& m) :
	sCommand( m.sCommand ),
	parameters( m.parameters ),
	m_bIncoming( m.m_bIncoming ), 
	m_bNotify( m.m_bNotify ),
	m_codePage( m.m_codePage )
{
	prefix.sNick = m.prefix.sNick;
	prefix.sUser = m.prefix.sUser;
	prefix.sHost = m.prefix.sHost;
}

void CIrcMessage::Reset()
{
	prefix.sNick = prefix.sUser = prefix.sHost = sCommand = _T("");
	m_bIncoming = false;
	m_bNotify = true;
	parameters.clear();
}

CIrcMessage& CIrcMessage::operator = (const CIrcMessage& m)
{
	if ( &m != this ) {
		sCommand = m.sCommand;
		parameters = m.parameters;
		prefix.sNick = m.prefix.sNick;
		prefix.sUser = m.prefix.sUser;
		prefix.sHost = m.prefix.sHost;
		m_bIncoming = m.m_bIncoming;
		m_bNotify = m.m_bNotify;
	}
	return *this;
}

CIrcMessage& CIrcMessage::operator = (const TCHAR* lpszCmdLine)
{
	Reset();
	ParseIrcCommand(lpszCmdLine);
	return *this;
}

void CIrcMessage::ParseIrcCommand(const TCHAR* lpszCmdLine)
{
	const TCHAR* p1 = lpszCmdLine;
	const TCHAR* p2 = lpszCmdLine;

	// prefix exists ?
	if( *p1 == ':' ) {
		// break prefix into its components (nick!user@host)
		p2 = ++p1;
		while( *p2 && !strchr(" !", *p2) )
			++p2;
		prefix.sNick.assign(p1, p2 - p1);
		if( *p2 != '!' )
			goto end_of_prefix;
		p1 = ++p2;
		while( *p2 && !strchr(" @", *p2) )
			++p2;
		prefix.sUser.assign(p1, p2 - p1);
		if( *p2 != '@' )
			goto end_of_prefix;
		p1 = ++p2;
		while( *p2 && *p2 != ' ' )
			++p2;
		prefix.sHost.assign(p1, p2 - p1);
end_of_prefix :
		while( *p2 && *p2 == ' ' )
			++p2;
		p1 = p2;
	}

	// get command
	p2 = p1;
	while( *p2 && *p2 != ' ' )
		++p2;

	sCommand.assign(p1, p2 - p1);
	transform( sCommand.begin(), sCommand.end(), sCommand.begin(), toupper );
	while( *p2 && *p2 == ' ' )
		++p2;
	p1 = p2;

	// get parameters
	while( *p1 ) {
		if ( *p1 == ':' ) {
			++p1;

			// seek end-of-message
			while( *p2 )
				++p2;
			parameters.push_back(TString(p1, p2 - p1));
			break;
		}
		else {
			// seek end of parameter
			while( *p2 && *p2 != ' ' )
				++p2;
			parameters.push_back(TString(p1, p2 - p1));
			// see next parameter
			while( *p2 && *p2 == ' ' )
				++p2;
			p1 = p2;
}	}	}

TString CIrcMessage::AsString() const
{
	TString s;

	if ( prefix.sNick.length() ) {
		s += _T(":") + prefix.sNick;
		if( prefix.sUser.length() && prefix.sHost.length() )
			s += _T("!") + prefix.sUser + _T("@") + prefix.sHost;
		s += _T(" ");
	}

	s += sCommand;

	for ( size_t i=0; i < parameters.size(); i++ ) {
		s += _T(" ");
		if( i == parameters.size() - 1 && (_tcschr(parameters[i].c_str(), ' ') || parameters[i][0] == ':' ) )// is last parameter ?
			s += _T(":");
		s += parameters[i];
	}

	s += _T("\r\n");
	return s;
}

////////////////////////////////////////////////////////////////////

tSSL_library_init       pSSL_library_init;
tSSL_CTX_new            pSSL_CTX_new;
tSSL_new                pSSL_new;
tSSL_set_fd             pSSL_set_fd;
tSSL_connect            pSSL_connect;
tSSL_read               pSSL_read;
tSSL_write              pSSL_write;
tSSLv23_method          pSSLv23_method;
tSSL_get_error          pSSL_get_error;
tSSL_load_error_strings pSSL_load_error_strings;
tSSL_shutdown           pSSL_shutdown;
tSSL_CTX_free           pSSL_CTX_free;
tSSL_free               pSSL_free;

int CSSLSession::SSLInit() 
{
	if(m_ssl_ctx) return true;
	if(!m_ssleay32) return false;

	pSSL_library_init			= (tSSL_library_init)			GetProcAddress(m_ssleay32, "SSL_library_init");
	pSSL_CTX_new				= (tSSL_CTX_new)				GetProcAddress(m_ssleay32, "SSL_CTX_new");
	pSSL_new					= (tSSL_new)					GetProcAddress(m_ssleay32, "SSL_new");
	pSSL_set_fd					= (tSSL_set_fd)					GetProcAddress(m_ssleay32, "SSL_set_fd");
	pSSL_connect				= (tSSL_connect)				GetProcAddress(m_ssleay32, "SSL_connect");
	pSSL_read					= (tSSL_read)					GetProcAddress(m_ssleay32, "SSL_read");
	pSSL_write					= (tSSL_write)					GetProcAddress(m_ssleay32, "SSL_write");
	pSSLv23_method				= (tSSLv23_method)				GetProcAddress(m_ssleay32, "SSLv23_method");
	pSSL_get_error				= (tSSL_get_error)				GetProcAddress(m_ssleay32, "SSL_get_error");
	pSSL_load_error_strings		= (tSSL_load_error_strings)		GetProcAddress(m_ssleay32, "SSL_load_error_strings");
	pSSL_shutdown				= (tSSL_shutdown)				GetProcAddress(m_ssleay32, "SSL_shutdown");
	pSSL_CTX_free				= (tSSL_CTX_free)				GetProcAddress(m_ssleay32, "SSL_CTX_free");
	pSSL_free					= (tSSL_free)					GetProcAddress(m_ssleay32, "SSL_free");

	if(	!pSSL_library_init			||
		!pSSL_CTX_new				||
		!pSSL_new					||
		!pSSL_set_fd				||
		!pSSL_connect				||
		!pSSL_read					||
		!pSSL_write					||
		!pSSLv23_method				||
		!pSSL_get_error				||
		!pSSL_load_error_strings	||
		!pSSL_shutdown				||
		!pSSL_CTX_free				||
		!pSSL_free					//||
		)  
	{
		return false;
	}

	if(!pSSL_library_init()) 
	{
		return false;
	}
	m_ssl_ctx=pSSL_CTX_new(pSSLv23_method());
	
	if (!m_ssl_ctx) {
		return false;
	}
	
	pSSL_load_error_strings();
	return true;
}

CSSLSession::~CSSLSession()
{
	if ( m_ssl_ctx )
		pSSL_CTX_free( m_ssl_ctx );
	m_ssl_ctx = 0;
}

int CSSLSession::SSLConnect(HANDLE con)
{
	if ( !SSLInit() ) 
		return false;
	
	SOCKET s = ( SOCKET )CallService( MS_NETLIB_GETSOCKET, (WPARAM) con, (LPARAM) 0 );
	if ( s == INVALID_SOCKET ) 
		return false;
	
	m_ssl = pSSL_new( m_ssl_ctx );
	if ( !m_ssl ) 
		return false;
	
	if ( m_ssl )
		pSSL_set_fd(m_ssl, s);
	
	nSSLConnected = pSSL_connect(m_ssl);
	return ( nSSLConnected == 1 );
}

int CSSLSession::SSLDisconnect(void)
{
	if ( nSSLConnected != 1 )
		return true;
	
	int nSSLret = pSSL_shutdown( m_ssl );
	if ( nSSLret == 0 )
		nSSLret = pSSL_shutdown( m_ssl );
	
	pSSL_free( m_ssl );
	m_ssl = 0;
	nSSLConnected = 0;	
	return nSSLret;
}

////////////////////////////////////////////////////////////////////

CIrcSession::CIrcSession(IIrcSessionMonitor* pMonitor) :
	codepage( IRC_DEFAULT_CODEPAGE )
{
	InitializeCriticalSection(&m_cs);
	InitializeCriticalSection(&m_resolve);
	InitializeCriticalSection(&m_dcc);
}

CIrcSession::~CIrcSession()
{
	//	Disconnect();
	DeleteCriticalSection(&m_cs);
	DeleteCriticalSection(&m_resolve);
	DeleteCriticalSection(&m_dcc);
	KillChatTimer(OnlineNotifTimer);
	KillChatTimer(OnlineNotifTimer3);
}

int CIrcSession::getCodepage() const
{
	return ( con != NULL ) ? codepage : IRC_DEFAULT_CODEPAGE;
}

CIrcSession& CIrcSession::operator << (const CIrcMessage& msg)
{
	if ( this ) {
		char* str = mir_t2a_cp( msg.AsString().c_str(), msg.m_codePage );
		NLSend(( const BYTE* )str, strlen( str ));
		mir_free( str );
		
		if ( !msg.sCommand.empty() && msg.sCommand != _T("QUIT") && msg.m_bNotify )
			Notify( &msg );
	}
	return *this;
}

bool CIrcSession::Connect(const CIrcSessionInfo& info)
{
	codepage = prefs->Codepage;

	try
	{
		NETLIBOPENCONNECTION ncon = { 0 };
		ncon.cbSize = sizeof(ncon);
		ncon.szHost = info.sServer.c_str();
		ncon.wPort = info.iPort;
		con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlib, (LPARAM) & ncon);
	
		if (con == NULL)
			return false;

		FindLocalIP(con); // get the local ip used for filetransfers etc

		if ( info.iSSL > 0 ) {
			sslSession.SSLConnect( con ); // Establish SSL connection
			if ( sslSession.nSSLConnected != 1 && info.iSSL == 2 ) {
				Netlib_CloseHandle( con );
				con = NULL;
				m_info.Reset();
				return false;
		}	}

		if ( Miranda_Terminated() ) {
			Disconnect();
			return false;
		}

		m_info = info;

		// start receiving messages from host
		mir_forkthread( ThreadProc, this );
		Sleep( 100 );
		if ( info.sPassword.length() )
			NLSend( "PASS %s\r\n", info.sPassword.c_str());
		NLSend( _T("NICK %s\r\n"), info.sNick.c_str());

		TString UserID = GetWord(info.sUserID.c_str(), 0);
		TCHAR szHostName[MAX_PATH];
		DWORD cbHostName = sizeof( szHostName );
		GetComputerName(szHostName, &cbHostName);
		TString HostName = GetWord(szHostName, 0);
		if ( UserID.empty() )
			UserID = _T("Miranda");
		if ( HostName.empty())
			HostName= _T("host");
		NLSend( _T("USER %s %s %s :%s\r\n"), UserID.c_str(), HostName.c_str(), _T("server"), info.sFullName.c_str());
	}
	catch( const char* )
	{
		Disconnect();
	}
	catch( ... )
	{
		Disconnect();
	}

	return con!=NULL;
}

void CIrcSession::Disconnect(void)
{
	static const DWORD dwServerTimeout = 5 * 1000;

	if( con == NULL )
		return;

	if ( prefs->QuitMessage && lstrlen(prefs->QuitMessage) > 0 )
		NLSend( _T("QUIT :%s\r\n"), prefs->QuitMessage);
	else
		NLSend( "QUIT \r\n" );

	int i = 0;
	while ( sslSession.nSSLConnected && sslSession.m_ssl || !sslSession.nSSLConnected && con ) {
		Sleep(50);
		if (i == 20)
			break;
		i++;
	}

	sslSession.SSLDisconnect(); // Close SSL connection

	if ( con )
		Netlib_CloseHandle(con);

	con = NULL;

	m_info.Reset();
	return;
}

void CIrcSession::Notify(const CIrcMessage* pmsg)
{
	// forward message to monitor objects
	EnterCriticalSection( &m_cs );
	
	for ( std::set<IIrcSessionMonitor*>::iterator it = m_monitors.begin(); it != m_monitors.end(); it++ )
		(*it)->OnIrcMessage(pmsg);

	LeaveCriticalSection(&m_cs);
}

int CIrcSession::NLSend( const unsigned char* buf, int cbBuf)
{
	if ( bMbotInstalled && prefs->ScriptingEnabled ) {
		int iVal = NULL;
		char * pszTemp = 0;
		pszTemp = ( char* ) mir_alloc( lstrlenA((const char *) buf ) + 1);
		lstrcpynA(pszTemp, (const char *)buf, lstrlenA ((const char *)buf) + 1);

		if ( Scripting_TriggerMSPRawOut(&pszTemp) && pszTemp ) {
			if ( sslSession.nSSLConnected == 1 ) 
				iVal = pSSL_write(sslSession.m_ssl, pszTemp, lstrlenA(pszTemp));	
			else if (con)
				iVal = Netlib_Send(con, (const char*)pszTemp, lstrlenA(pszTemp), MSG_DUMPASTEXT);
		}
		if ( pszTemp )
			mir_free( pszTemp );

		return iVal;
	}
	
	if ( sslSession.nSSLConnected == 1 ) 
		return pSSL_write(sslSession.m_ssl, buf, cbBuf);	

	if (con)
		return Netlib_Send(con, (const char*)buf, cbBuf, MSG_DUMPASTEXT);

	return 0;
}

int CIrcSession::NLSend( const TCHAR* fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);

	TCHAR szBuf[1024*4];
	mir_vsntprintf(szBuf, SIZEOF(szBuf), fmt, marker);
	va_end(marker);

	char* buf = mir_t2a_cp( szBuf, getCodepage());
	int result = NLSend((unsigned char*)buf, strlen(buf));
	mir_free( buf );
	return result;
}

#if defined( _UNICODE )
int CIrcSession::NLSend( const char* fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);

	char szBuf[1024*4];
	int cbLen = mir_vsnprintf(szBuf, SIZEOF(szBuf), fmt, marker);
	va_end(marker);

	return NLSend((unsigned char*)szBuf, cbLen );
}
#endif

int CIrcSession::NLSendNoScript( const unsigned char* buf, int cbBuf)
{
	if ( sslSession.nSSLConnected == 1 ) 
		return pSSL_write(sslSession.m_ssl, buf, cbBuf);	

	if ( con )
		return Netlib_Send(con, (const char*)buf, cbBuf, MSG_DUMPASTEXT);

	return 0;
}

int CIrcSession::NLReceive(unsigned char* buf, int cbBuf)
{
	if ( sslSession.nSSLConnected == 1 ) 
		return pSSL_read( sslSession.m_ssl, buf, cbBuf );

	return Netlib_Recv( con, (char*)buf, cbBuf, MSG_DUMPASTEXT );
}

void CIrcSession::KillIdent()
{
	if ( hBindPort )
		Netlib_CloseHandle( hBindPort );
	return;
}

void CIrcSession::InsertIncomingEvent(TCHAR* pszRaw)
{
	CIrcMessage msg(pszRaw, true);
	Notify( &msg );
	return;
}

void CIrcSession::createMessageFromPchar( const char* p )
{
	TCHAR* ptszMsg;
	if ( codepage != CP_UTF8 && prefs->UtfAutodetect ) {
		if ( mir_utf8decodecp( NEWSTR_ALLOCA(p), codepage, &ptszMsg ) == NULL )
			ptszMsg = mir_a2t_cp( p, codepage );
	}
	else ptszMsg = mir_a2t_cp( p, codepage );
	CIrcMessage msg( ptszMsg, codepage, true );
	Notify( &msg );
	mir_free( ptszMsg );
}

void CIrcSession::DoReceive()
{
	char chBuf[1024*4+1];
	int cbInBuf = 0;
	
	if ( m_info.bIdentServer && m_info.iIdentServerPort != NULL ) {
		NETLIBBIND nb = {0};
		nb.cbSize = sizeof(NETLIBBIND);
		nb.pfnNewConnectionV2 = DoIdent;
		nb.wPort = m_info.iIdentServerPort;
		hBindPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)hNetlib,(LPARAM) &nb);
		if ( !hBindPort || nb.wPort != m_info.iIdentServerPort ) {
			char szTemp[200];
			mir_snprintf(szTemp, sizeof(szTemp), "Error: unable to bind local port %u", m_info.iIdentServerPort);
			CallService(MS_NETLIB_LOG, (WPARAM) hNetlib , (LPARAM) szTemp );
			KillIdent();
	}	}

	while( con ) {
		int nLinesProcessed = 0;

		int cbRead = NLReceive((unsigned char*)chBuf+cbInBuf, sizeof(chBuf)-cbInBuf-1);
		if( cbRead <= 0 )
			break;

		cbInBuf += cbRead;
		chBuf[cbInBuf] = '\0';

		char* pStart = chBuf;
		while( *pStart ) {
			char* pEnd;

			// seek end-of-line
			for(pEnd=pStart; *pEnd && *pEnd != '\r' && *pEnd != '\n'; ++pEnd)
				;
			if( *pEnd == '\0' )
				break; // uncomplete message. stop parsing.

			++nLinesProcessed;

			// replace end-of-line with NULLs and skip
			while( *pEnd == '\r' || *pEnd == '\n' )
				*pEnd++ = '\0';

			// process single message by monitor objects
			if ( *pStart ) {
				if ( bMbotInstalled && prefs->ScriptingEnabled ) {
					char* pszTemp = mir_strdup( pStart ); 

					if ( Scripting_TriggerMSPRawIn( &pszTemp ) && pszTemp ) {
						char* p1 = pszTemp;
						// replace end-of-line with NULLs
						while( *p1 != '\0' ) {
							if( *p1 == '\r' || *p1 == '\n')
								*p1 = '\0';
							*p1++;
						}

						createMessageFromPchar( pszTemp );
					}

					mir_free( pszTemp );
				}
				else createMessageFromPchar( pStart );
			}

			cbInBuf -= pEnd - pStart;
			pStart = pEnd;
		}

		// discard processed messages
		if ( nLinesProcessed != 0 )
			memmove(chBuf, pStart, cbInBuf+1);
	}

	if ( con ) {
		Netlib_CloseHandle(con);
		con = NULL;
	}

	KillIdent();

	// notify monitor objects that the connection has been closed
	Notify(NULL);
}

void __cdecl CIrcSession::ThreadProc(void *pparam)
{
	CIrcSession* pThis = (CIrcSession*)pparam;
	try 
	{ 
		pThis->DoReceive(); 
	}
	catch( ... ) 
	{
	}

	pThis->m_info.Reset();
}

void CIrcSession::AddDCCSession(HANDLE hContact, CDccSession* dcc)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_chats.find(hContact);
	if ( it != m_dcc_chats.end())
		m_dcc_chats.erase(it);

	g_ircSession.m_dcc_chats.insert(DccSessionPair(hContact, dcc));

	LeaveCriticalSection(&m_dcc);
}

void CIrcSession::AddDCCSession(DCCINFO* pdci, CDccSession* dcc)
{
	EnterCriticalSection(&m_dcc);

	g_ircSession.m_dcc_xfers.insert(DccSessionPair(pdci, dcc));

	LeaveCriticalSection(&m_dcc);
}

void CIrcSession::RemoveDCCSession(HANDLE hContact)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_chats.find(hContact);
	if ( it != m_dcc_chats.end())
		m_dcc_chats.erase(it);

	LeaveCriticalSection(&m_dcc);
}

void CIrcSession::RemoveDCCSession(DCCINFO* pdci)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_xfers.find(pdci);
	if(it != m_dcc_xfers.end())
		m_dcc_xfers.erase(it);

	LeaveCriticalSection(&m_dcc);
}

CDccSession* CIrcSession::FindDCCSession(HANDLE hContact)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_chats.find(hContact);
	if ( it != m_dcc_chats.end()) {
		LeaveCriticalSection(&m_dcc);
		return (CDccSession*)it->second;
	}

	LeaveCriticalSection(&m_dcc);
	return 0;
}

CDccSession* CIrcSession::FindDCCSession(DCCINFO* pdci)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_xfers.find(pdci);
	if ( it != m_dcc_xfers.end()) {
		LeaveCriticalSection(&m_dcc);
		return (CDccSession*)it->second;
	}
	LeaveCriticalSection(&m_dcc);
	return 0;
}

CDccSession* CIrcSession::FindDCCSendByPort(int iPort)
{
	EnterCriticalSection(&m_dcc);
	CDccSession* dcc = NULL;

	DccSessionMap::iterator it = m_dcc_xfers.begin();
	while(it != m_dcc_xfers.end()) {
		dcc = it->second;
		if ( dcc->di->iType == DCC_SEND && dcc->di->bSender && iPort == dcc->di->iPort ) {
			LeaveCriticalSection(&m_dcc);
			return (CDccSession*)it->second;
		}
		it++;
	}
	LeaveCriticalSection(&m_dcc);
	return 0;
}

CDccSession* CIrcSession::FindDCCRecvByPortAndName(int iPort, const TCHAR* szName)
{
	EnterCriticalSection(&m_dcc);
	CDccSession* dcc = NULL;

	DccSessionMap::iterator it = m_dcc_xfers.begin();
	while ( it != m_dcc_xfers.end()) {
		dcc = it->second;
		DBVARIANT dbv;

		if ( !DBGetContactSettingTString(dcc->di->hContact, IRCPROTONAME, "Nick", &dbv)) {
			if ( dcc->di->iType == DCC_SEND && !dcc->di->bSender && !lstrcmpi( szName, dbv.ptszVal) && iPort == dcc->di->iPort ) {
				DBFreeVariant( &dbv );
				LeaveCriticalSection( &m_dcc );
				return (CDccSession*)it->second;
			}
			DBFreeVariant(&dbv);
		}
		it++;
	}
	LeaveCriticalSection(&m_dcc);
	return 0;
}

CDccSession* CIrcSession::FindPassiveDCCSend(int iToken)
{
	EnterCriticalSection(&m_dcc);
	CDccSession* dcc = NULL;

	DccSessionMap::iterator it = m_dcc_xfers.begin();
	while ( it != m_dcc_xfers.end()) {
		dcc = it->second;

		if ( iToken == dcc->iToken ) {
			LeaveCriticalSection(&m_dcc);
			return (CDccSession*)it->second;		
		}
		it++;
	}
	LeaveCriticalSection(&m_dcc);
	return 0;
}

CDccSession* CIrcSession::FindPassiveDCCRecv(TString sName, TString sToken)
{
	EnterCriticalSection(&m_dcc);
	CDccSession* dcc = NULL;

	DccSessionMap::iterator it = m_dcc_xfers.begin();
	while ( it != m_dcc_xfers.end()) {
		dcc = it->second;

		if ( sToken == dcc->di->sToken && sName == dcc->di->sContactName) {
			LeaveCriticalSection(&m_dcc);
			return (CDccSession*)it->second;
		}
		it++;
	}
	LeaveCriticalSection(&m_dcc);
	return 0;
}

void CIrcSession::DisconnectAllDCCSessions(bool Shutdown)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_chats.begin();
	CDccSession* dcc;

	while ( it != m_dcc_chats.end()) {
		dcc = it->second;
		it++;
		if ( prefs->DisconnectDCCChats || Shutdown )
			dcc->Disconnect();
	}

	LeaveCriticalSection(&m_dcc);
}

void CIrcSession::CheckDCCTimeout(void)
{
	EnterCriticalSection(&m_dcc);

	DccSessionMap::iterator it = m_dcc_chats.begin();
	CDccSession* dcc;

	while ( it != m_dcc_chats.end()) {
		dcc = it->second;
		it++;
		if ( time(0) > dcc->tLastActivity + DCCCHATTIMEOUT )
			dcc->Disconnect();
	}

	it = m_dcc_xfers.begin();
	while ( it != m_dcc_xfers.end()) {
		dcc = it->second;
		it++;
		if ( time(0) > dcc->tLastActivity + DCCSENDTIMEOUT )
			dcc->Disconnect();
	}

	LeaveCriticalSection(&m_dcc);
	return;
}

void CIrcSession::AddIrcMonitor(IIrcSessionMonitor* pMonitor)
{
	EnterCriticalSection(&m_cs);
	m_monitors.insert( pMonitor );
	LeaveCriticalSection(&m_cs);
}

void CIrcSession::RemoveMonitor(IIrcSessionMonitor* pMonitor)
{
	EnterCriticalSection(&m_cs);
	m_monitors.erase( pMonitor );
	LeaveCriticalSection(&m_cs);
}

////////////////////////////////////////////////////////////////////

CIrcIgnoreItem::CIrcIgnoreItem( const TCHAR* _mask, const TCHAR* _flags, const TCHAR* _network ) :
	mask( _mask ),
	flags( _flags ),
	network( _network )
{
}

#if defined( _UNICODE )
CIrcIgnoreItem::CIrcIgnoreItem( const char* _mask, const char* _flags, const char* _network ) :
	mask( _A2T( _mask, g_ircSession.getCodepage())),
	flags( _A2T( _flags, g_ircSession.getCodepage())),
	network( _A2T( _network, g_ircSession.getCodepage()))
{
}
#endif

CIrcIgnoreItem::~CIrcIgnoreItem()
{
}

////////////////////////////////////////////////////////////////////

CIrcSessionInfo::CIrcSessionInfo() :
	iPort(0),
	bIdentServer(false),
	iIdentServerPort(0)
{
}

CIrcSessionInfo::CIrcSessionInfo(const CIrcSessionInfo& si) :
	sServer(si.sServer),
	sServerName(si.sServerName),
	iPort(si.iPort),
	sNick(si.sNick),
	sUserID(si.sUserID),
	sFullName(si.sFullName),
	sPassword(si.sPassword),
	bIdentServer(si.bIdentServer),
	iSSL(si.iSSL),
	sIdentServerType(si.sIdentServerType),
	sNetwork(si.sNetwork),
	iIdentServerPort(si.iIdentServerPort)
{
}

void CIrcSessionInfo::Reset()
{
	sServer = "";
	sServerName = _T("");
	iPort = 0;
	sNick = _T("");
	sUserID = _T("");
	sFullName = _T("");
	sPassword = "";
	bIdentServer = false;
	bNickFlag = false;
	iSSL = 0;
	sIdentServerType = _T("");
	iIdentServerPort = 0;
	sNetwork = _T("");
}

////////////////////////////////////////////////////////////////////
CIrcMonitor::HandlersMap CIrcMonitor::m_handlers;
CIrcMonitor::IrcCommandsMapsListEntry CIrcMonitor::m_handlersMapsListEntry
	= { &CIrcMonitor::m_handlers, NULL };


CIrcMonitor::CIrcMonitor(CIrcSession& session)
	: m_session(session)
{
}

CIrcMonitor::~CIrcMonitor()
{
}

void CIrcMonitor::OnIrcMessage(const CIrcMessage* pmsg)
{
	if ( pmsg != NULL ) {
		OnIrcAll(pmsg);

		PfnIrcMessageHandler pfn = FindMethod( pmsg->sCommand.c_str() );
		if ( pfn ) {
			// call member function. if it returns 'false',
			// call the default handling
			if ( !(this->*pfn)( pmsg ))
				OnIrcDefault( pmsg );
		}
		else // handler not found. call default handler
			OnIrcDefault( pmsg );
	}
	else OnIrcDisconnected();
}

CIrcMonitor::PfnIrcMessageHandler CIrcMonitor::FindMethod(const TCHAR* lpszName)
{
	// call the recursive version with the most derived map
	return FindMethod(GetIrcCommandsMap(), lpszName);
}

CIrcMonitor::PfnIrcMessageHandler CIrcMonitor::FindMethod(IrcCommandsMapsListEntry* pMapsList, const TCHAR* lpszName)
{
	HandlersMap::iterator it = pMapsList->pHandlersMap->find(lpszName);
	if( it != pMapsList->pHandlersMap->end() )
		return it->second; // found !
	else if( pMapsList->pBaseHandlersMap )
		return FindMethod(pMapsList->pBaseHandlersMap, lpszName); // try at base class
	return NULL; // not found in any map
}

////////////////////////////////////////////////////////////////////

DECLARE_IRC_MAP(CIrcDefaultMonitor, CIrcMonitor)

CIrcDefaultMonitor::CIrcDefaultMonitor(CIrcSession& session) :
	CIrcMonitor( session )
{
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "NICK", OnIrc_NICK)
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "PING", OnIrc_PING)
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "002", OnIrc_YOURHOST)
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "001", OnIrc_WELCOME)
}

bool CIrcDefaultMonitor::OnIrc_NICK(const CIrcMessage* pmsg)
{
	if (( m_session.GetInfo().sNick == pmsg->prefix.sNick) && (pmsg->parameters.size() > 0 )) {
		m_session.m_info.sNick = pmsg->parameters[0];
		DBWriteContactSettingTString(NULL,IRCPROTONAME,"Nick",m_session.m_info.sNick.c_str());
	}
	return false;
}

bool CIrcDefaultMonitor::OnIrc_PING(const CIrcMessage* pmsg)
{
	TCHAR szResponse[100];
	mir_sntprintf(szResponse, SIZEOF(szResponse), _T("PONG %s"), pmsg->parameters[0].c_str());
	m_session << CIrcMessage( szResponse, m_session.getCodepage() );
	return false;
}

bool CIrcDefaultMonitor::OnIrc_YOURHOST(const CIrcMessage* pmsg)
{
	static const TCHAR* lpszFmt = _T("Your host is %99[^ \x5b,], running version %99s");
	TCHAR szHostName[100], szVersion[100];
	if( _stscanf(pmsg->parameters[1].c_str(), lpszFmt, &szHostName, &szVersion) > 0 )
		m_session.m_info.sServerName = szHostName;
	if (pmsg->parameters[0] != m_session.GetInfo().sNick)
		m_session.m_info.sNick = pmsg->parameters[0];
	return false;
}
bool CIrcDefaultMonitor::OnIrc_WELCOME(const CIrcMessage* pmsg)
{
	if (pmsg->parameters[0] != m_session.GetInfo().sNick)
		m_session.m_info.sNick = pmsg->parameters[0];
	return false;
}

////////////////////////////////////////////////////////////////////

#define IPC_ADDR_SIZE				4		/* Size of IP address, change for IPv6. */

char* ConvertIntegerToIP(unsigned long int_ip_addr)
{
	IN_ADDR intemp;
	IN_ADDR in;
	intemp.S_un.S_addr = int_ip_addr;

	in.S_un.S_un_b.s_b1 = intemp.S_un.S_un_b.s_b4;
	in.S_un.S_un_b.s_b2 = intemp.S_un.S_un_b.s_b3;
	in.S_un.S_un_b.s_b3 = intemp.S_un.S_un_b.s_b2;
	in.S_un.S_un_b.s_b4 = intemp.S_un.S_un_b.s_b1;

	return inet_ntoa( in );
}

unsigned long ConvertIPToInteger( char* IP )
{
	IN_ADDR in;
	IN_ADDR intemp;

	if( IP == 0 || lstrlenA(IP) == 0)
		return 0;

	intemp.S_un.S_addr = inet_addr(IP);

	in.S_un.S_un_b.s_b1 = intemp.S_un.S_un_b.s_b4;
	in.S_un.S_un_b.s_b2 = intemp.S_un.S_un_b.s_b3;
	in.S_un.S_un_b.s_b3 = intemp.S_un.S_un_b.s_b2;
	in.S_un.S_un_b.s_b4 = intemp.S_un.S_un_b.s_b1;
	return in.S_un.S_addr;
}

////////////////////////////////////////////////////////////////////

// initialize basic stuff needed for the dcc objects, also start a timer for checking the status of connections (timeouts)
CDccSession::CDccSession( DCCINFO* pdci ) :
	NewFileName(0),
	dwWhatNeedsDoing(0),
	tLastPercentageUpdate(0),
	iTotal(0),
	iGlobalToken(),
	dwResumePos(0),
	hEvent(0),
	con(0),
	hBindPort(0)
{
	tLastActivity = time(0);

	di = pdci; // Setup values passed to the constructor

	ZeroMemory(&pfts, sizeof(PROTOFILETRANSFERSTATUS));
	pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);

	if(di->iType == DCC_SEND && di->bSender == false)
		hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(nDcc == 0)
		SetChatTimer(DCCTimer, 20*1000, DCCTimerProc);

	nDcc++; // increase the count of existing objects

	iGlobalToken++;
	if(iGlobalToken == 1000)
		iGlobalToken = 1;
	iToken = iGlobalToken;

	iPacketSize = DBGetContactSettingWord(NULL,IRCPROTONAME, "PacketSize", 4096);

	if ( di->dwAdr )
		DBWriteContactSettingDword(di->hContact, IRCPROTONAME, "IP", di->dwAdr); // mtooltip stuff

	#if defined( _UNICODE )
		szFullPath = szWorkingDir = NULL;
	#endif
}

CDccSession::~CDccSession() // destroy all that needs destroying
{
	if ( di->iType == DCC_SEND ) {
		// ack SUCCESS or FAILURE
		if (iTotal == di->dwSize )
			ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, (void *)di, 0);
		else
			ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, (void *)di, 0);
	}

	if ( di->iType == DCC_CHAT ) {
		CDccSession* dcc = g_ircSession.FindDCCSession(di->hContact);
		if ( dcc && this == dcc ) {
			g_ircSession.RemoveDCCSession(di->hContact); // objects automatically remove themselves from the list of objects
			DBWriteContactSettingWord(di->hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
	}	}

	if ( di->iType == DCC_SEND )
		g_ircSession.RemoveDCCSession( di ); 

	if ( hEvent != NULL ) {
		SetEvent( hEvent );
		CloseHandle( hEvent );
		hEvent = NULL;
	}

	delete di;
	nDcc--;

	if ( nDcc < 0 )
		nDcc = 0;

	if ( nDcc == 0 )
		KillChatTimer( DCCTimer ); // destroy the timer when no dcc objects remain

	#if defined( _UNICODE )
		mir_free( szFullPath );
		mir_free( szWorkingDir );
	#endif
}

int CDccSession::NLSend(const unsigned char* buf, int cbBuf) 
{
	tLastActivity = time(0);

	if (con)
		return Netlib_Send(con, (const char*)buf, cbBuf, di->iType == DCC_CHAT?MSG_DUMPASTEXT:MSG_NODUMP);

	return 0;
}

int CDccSession::NLReceive(const unsigned char* buf, int cbBuf)
{
	int n = 0;

	if(con)
		n = Netlib_Recv(con, (char*)buf, cbBuf, di->iType == DCC_CHAT?MSG_DUMPASTEXT:MSG_NODUMP);

	tLastActivity = time(0);
	return n;
}

int CDccSession::SendStuff(const TCHAR* fmt)
{
	String buf = _T2A( fmt, g_ircSession.getCodepage() );
	return NLSend(( const unsigned char* )buf.c_str(), buf.size());
}

// called when the user wants to connect/create a new connection given the parameters in the constructor.
int CDccSession::Connect()
{
	if ( !di->bSender || di->bReverse ) {
		if ( !con )
			mir_forkthread(ConnectProc, this  ); // spawn a new thread for time consuming activities, ie when connecting to a remote computer
		return true;
	}

	if ( !con )
		return SetupConnection(); // no need to spawn thread for setting up a listening port locally

	return false; 
}

void __cdecl CDccSession::ConnectProc( void *pparam )
{
	CDccSession* pThis = (CDccSession*)pparam;
	if ( !pThis->con )
		pThis->SetupConnection();
}

// small function to setup the address and port of the remote computer fror passive filetransfers
void CDccSession::SetupPassive(DWORD adress, DWORD port)
{
	di->dwAdr = adress;
	di->iPort = (int)port;

	DBWriteContactSettingDword(di->hContact, IRCPROTONAME, "IP", di->dwAdr); // mtooltip stuff
}

int CDccSession::SetupConnection()
{
	//sets up the connection
	try
	{
		// if it is a dcc chat connection make sure it is "offline" to begoin with, since no connection exists still
		if ( di->iType == DCC_CHAT )
			DBWriteContactSettingWord(di->hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);

		// Set up stuff needed for the filetransfer dialog (if it is a filetransfer)
		if ( di->iType == DCC_SEND ) {
			file[0] = ( char* )di->sFileAndPath.c_str();
			file[1] = 0;

			#if defined( _UNICODE )
				pfts.currentFile = szFullPath = mir_strdup( _T2A( di->sFileAndPath.c_str(), g_ircSession.getCodepage() ));
				pfts.workingDir =	szWorkingDir = mir_strdup( _T2A( di->sPath.c_str(), g_ircSession.getCodepage() ));
			#else
				pfts.currentFile = ( char* )di->sFileAndPath.c_str();
				pfts.workingDir =	( char* )di->sPath.c_str();
			#endif

			pfts.hContact = di->hContact;
			pfts.sending = di->bSender ? true : false;
			pfts.totalFiles =	1;
			pfts.currentFileNumber = 0;
			pfts.totalBytes =	di->dwSize;
			pfts.currentFileSize = pfts.totalBytes;
			pfts.files = file;
			pfts.totalProgress = 0;
			pfts.currentFileProgress =	0;
			pfts.currentFileTime = (unsigned long)time(0);
		}

		// create a listening socket for outgoing chat/send requests. The remote computer connects to this computer. Used for both chat and filetransfer.
		if ( di->bSender && !di->bReverse ) {
			NETLIBBIND nb = {0};
			nb.cbSize = sizeof(NETLIBBIND);
			nb.pfnNewConnectionV2 = DoIncomingDcc; // this is the (helper) function to be called once an incoming connection is made. The 'real' function that is called is IncomingConnection()
			nb.pExtra = (void *)this; 
			nb.wPort = 0;
			hBindPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)hNetlibDCC,(LPARAM) &nb);

			if ( hBindPort == NULL ) {
				delete this; // dcc objects destroy themselves when the connection has been closed or failed for some reasson.
				return 0;
			}

			di->iPort = nb.wPort; // store the port internally so it is possible to search for it (for resuming of filetransfers purposes)
			return nb.wPort; // return the created port so it can be sent to the remote computer in a ctcp/dcc command
		}

		// If a remote computer initiates a chat session this is used to connect to the remote computer (user already accepted at this point). 
		// also used for connecting to a remote computer for remote file transfers
		if ( di->iType == DCC_CHAT && !di->bSender || di->iType == DCC_SEND && di->bSender && di->bReverse ) {
			NETLIBOPENCONNECTION ncon = { 0 };
			ncon.cbSize = sizeof(ncon);
			ncon.szHost = ConvertIntegerToIP(di->dwAdr); 
			ncon.wPort = (WORD) di->iPort;
			con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibDCC, (LPARAM) & ncon);
		}


		// If a remote computer initiates a filetransfer this is used to connect to that computer (the user has chosen to accept but it is possible the file exists/needs to be resumed etc still)
		if ( di->iType == DCC_SEND && !di->bSender ) {

			// this following code is for miranda to be able to show the resume/overwrite etc dialog if the file that we are receiving already exists. 
			// It must be done before we create the connection or else the other party will begin sending packets while the user is still deciding if 
			// s/he wants to resume/cancel or whatever. Just the way dcc is...

			// if the file exists (dialog is shown) WaitForSingleObject() till the dialog is closed and PS_FILERESUME has been processed. 
			// dwWhatNeedsDoing will be set using InterlockedExchange() (from other parts of the code depending on action) before SetEvent() is called.
			// If the user has chosen rename then InterlockedExchange() will be used for setting NewFileName to a string containing the new name.
			// Furthermore dwResumePos will be set using InterlockedExchange() to indicate what the file position to start from is.
			if ( ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, (void *)di, (long)&pfts)) { 
				WaitForSingleObject( hEvent, INFINITE );
				switch( dwWhatNeedsDoing ) {
				case FILERESUME_RENAME:
					// If the user has chosen to rename the file we need to change variables accordingly. NewFileName has been set using
					// InterlockedExchange()
					if ( NewFileName) { // the user has chosen to rename the new incoming file.
						di->sFileAndPath = NewFileName;

						int i = di->sFileAndPath.rfind( _T("\\"), di->sFileAndPath.length());
						if ( i != string::npos ) {
							di->sPath = di->sFileAndPath.substr(0, i+1);
							di->sFile = di->sFileAndPath.substr(i+1, di->sFileAndPath.length());
						}

						#if defined( _UNICODE )
							mir_free( szFullPath );
							pfts.currentFile = szFullPath = mir_strdup( _T2A( di->sFileAndPath.c_str(), g_ircSession.getCodepage() ));
							mir_free( szWorkingDir );
							pfts.workingDir =	szWorkingDir = mir_strdup( _T2A( di->sPath.c_str(), g_ircSession.getCodepage() ));
						#else
							pfts.currentFile = ( char* )di->sFileAndPath.c_str();
							pfts.workingDir =	( char* )di->sPath.c_str();
						#endif
						pfts.totalBytes = di->dwSize;
						pfts.currentFileSize = pfts.totalBytes;
						file[0] = pfts.currentFile;
						
						delete []NewFileName;
						NewFileName = NULL;
					}
					break;

				case FILERESUME_OVERWRITE:	
				case FILERESUME_RESUME	:	
					// no action needed at this point, just break out of the switch statement
					break;

				case FILERESUME_CANCEL	:
					return FALSE; 

				case FILERESUME_SKIP	:
				default:
					delete this; // per usual dcc objects destroy themselves when they fail or when connection is closed
					return FALSE; 
			}	}			

			// hack for passive filetransfers
			if ( di->iType == DCC_SEND && !di->bSender && di->bReverse ) {
				NETLIBBIND nb = {0};
				nb.cbSize = sizeof(NETLIBBIND);
				nb.pfnNewConnectionV2 = DoIncomingDcc; // this is the (helper) function to be called once an incoming connection is made. The 'real' function that is called is IncomingConnection()
				nb.pExtra = (void *)this; 
				nb.wPort = 0;
				hBindPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)hNetlibDCC,(LPARAM) &nb);

				if ( hBindPort == NULL ) {
					DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), LPGENT("DCC ERROR: Unable to bind local port for passive filetransfer"), NULL, NULL, NULL, true, false); 
					delete this; // dcc objects destroy themselves when the connection has been closed or failed for some reasson.
					return 0;
				}

				di->iPort = nb.wPort; // store the port internally so it is possible to search for it (for resuming of filetransfers purposes)

				TString sFileWithQuotes = di->sFile;

				// if spaces in the filename surround with quotes
				if ( sFileWithQuotes.find( ' ', 0 ) != string::npos ) {
					sFileWithQuotes.insert( 0, _T("\""));
					sFileWithQuotes.insert( sFileWithQuotes.length(), _T("\""));
				}

				// send out DCC RECV command for passive filetransfers
				unsigned long ulAdr = 0;
				if ( prefs->ManualHost )
					ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);				
				else
					ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

				if ( di->iPort && ulAdr )
					PostIrcMessage( _T("/CTCP %s DCC SEND %s %u %u %u %s"), di->sContactName.c_str(), sFileWithQuotes.c_str(), ulAdr, di->iPort, di->dwSize, di->sToken.c_str());

				return TRUE; 			
			}

			// connect to the remote computer from which you are receiving the file (now all actions to take (resume/overwrite etc) have been decided
			NETLIBOPENCONNECTION ncon = { 0 };
			ncon.cbSize = sizeof(ncon);
			ncon.szHost = ConvertIntegerToIP(di->dwAdr);
			ncon.wPort = (WORD) di->iPort;

			con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibDCC, (LPARAM) & ncon);
		}

		// if for some reason the plugin has failed to connect to the remote computer the object is destroyed.
		if ( con == NULL ) {
			delete this;
			return FALSE; // failed to connect
		}

		// if it is a chat connection set the user to online now since we now know there is a connection
		if ( di->iType == DCC_CHAT )
			DBWriteContactSettingWord(di->hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE);

		// spawn a new thread to handle receiving/sending of data for the new chat/filetransfer connection to the remote computer
		mir_forkthread(ThreadProc, this  );
	}
	catch( const char* )
	{
		Disconnect();
	}
	catch( ... )
	{
		Disconnect();
	}
	
	return (int)con;
}

// called by netlib for incoming connections on a listening socket (chat/filetransfer)
int CDccSession::IncomingConnection(HANDLE hConnection, DWORD dwIP)
{
	con = hConnection;
	if ( con == NULL ) {
		delete this;
		return false; // failed to connect
	}

	DBWriteContactSettingDword(di->hContact, IRCPROTONAME, "IP", dwIP); // mToolTip stuff

	if ( di->iType == DCC_CHAT )
		DBWriteContactSettingWord(di->hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE); // set chat to online

	// same as above, spawn a new thread to handle receiving/sending of data for the new incoming chat/filetransfer connection  
	mir_forkthread(ThreadProc, this  );
	return true;
}

// here we decide which function to use for communicating with the remote computer, depending on connection type
void __cdecl CDccSession::ThreadProc(void *pparam) 
{
	CDccSession* pThis = (CDccSession*)pparam;

	// if it is an incoming connection on a listening port, then we should close the listenting port so only one can connect (the one you offered
	// the connection to) can connect and not evil IRCopers with haxxored IRCDs
	if ( pThis->hBindPort ) {
		Netlib_CloseHandle(pThis->hBindPort);
		pThis->hBindPort = NULL;
	}

	try 
	{ 
		if ( pThis->di->iType == DCC_CHAT )
			pThis->DoChatReceive(); // dcc chat

		else if( !pThis->di->bSender )
			pThis->DoReceiveFile(); // receive a file

		else if ( pThis->di->bSender )
			pThis->DoSendFile(); // send a file
		
	} 
	catch( ... )
	{
}	}

// this is done when the user is initiating a filetransfer to a remote computer
void CDccSession::DoSendFile() 
{
	// initialize the filetransfer dialog
	ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, (void *)di, 0);
	ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, (void *)di, 0);

	BYTE DCCMode = DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0);
	WORD wPacketSize = DBGetContactSettingWord(NULL, IRCPROTONAME, "DCCPacketSize", 1024*4);

	if ( wPacketSize < 256 )
		wPacketSize = 256;

	if ( wPacketSize > 32 * 1024 )
		wPacketSize = 32 * 1024;

	BYTE* chBuf = new BYTE[wPacketSize+1];

	// is there a connection?
	if ( con ) {
		// open the file for reading
		FILE* hFile = _tfopen( di->sFileAndPath.c_str(), _T("rb"));
		if ( hFile ) {	
			DWORD iLastAck = 0;

			// if the user has chosen to resume a file, dwResumePos will contain a value (set using InterlockedExchange())
			// and then the variables and the file pointer are changed accordingly.
			if ( dwResumePos && dwWhatNeedsDoing == FILERESUME_RESUME ) {
				fseek (hFile,dwResumePos,SEEK_SET);
				iTotal = dwResumePos;
				iLastAck = dwResumePos;
				pfts.totalProgress = dwResumePos;
				pfts.currentFileProgress = dwResumePos;
			}

			// initial ack to set the 'percentage-ready meter' to the correct value
			ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (void *)di, (LPARAM) &pfts);
			
			tLastActivity = time(0);

			// create a packet receiver to handle receiving ack's from the remote computer.
			HANDLE hPackrcver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)con, (LPARAM)sizeof(DWORD));
			NETLIBPACKETRECVER npr;
			npr.cbSize = sizeof(NETLIBPACKETRECVER);
			npr.dwTimeout = 60*1000;
			npr.bufferSize = sizeof(DWORD); 
			npr.bytesUsed = 0;

			// until the connection is dropped it will spin around in this while() loop
			while ( con ) {
				// read a packet
				DWORD iRead = fread(chBuf, 1, wPacketSize, hFile);
				if ( iRead <= 0 )
					break; // break out if everything has already been read

				// send the package
				DWORD cbSent = NLSend((unsigned char*)chBuf, iRead);
				if ( cbSent <= 0 )
					break; // break out if connection is lost or a transmission error has occured

				if ( !con )
					break;

				iTotal += cbSent;

				// block connection and receive ack's from remote computer (if applicable)
				if ( DCCMode == 0 ) {
					DWORD dwRead = 0;
					DWORD dwPacket = NULL;

					do {					
						dwRead = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hPackrcver, (LPARAM)&npr);
						npr.bytesUsed = sizeof(DWORD);          

						if( dwRead <= 0)
							break; // connection closed, or a timeout occurred.

						memcpy(&dwPacket, npr.buffer, 4);
						iLastAck = ntohl((u_long)dwPacket);

					}
						while(con && iTotal != iLastAck);

					if ( !con || dwRead <= 0 )
						goto DCC_STOP;
				}

				if ( DCCMode == 1 ) {
					DWORD dwRead = 0;
					DWORD dwPacket = NULL;

					do {					
						dwRead = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hPackrcver, (LPARAM)&npr);
						npr.bytesUsed = sizeof(DWORD);          
						if ( dwRead <= 0)
							break; // connection closed, or a timeout occurred.

						memcpy(&dwPacket, npr.buffer, 4);
						iLastAck = ntohl((u_long)dwPacket);
					}
						while(con && (di->dwSize != iTotal
						&& iTotal - iLastAck >= 100*1024
						|| di->dwSize == iTotal // get the last packets when the whole file has been sent
						&& iTotal != iLastAck));

					if ( !con || dwRead <= 0 )
						goto DCC_STOP;
				}				

				// update the filetransfer dialog's 'percentage-ready meter' once per second only to save cpu
				if ( tLastPercentageUpdate < time(0)) {
					tLastPercentageUpdate = time(0);
					pfts.totalProgress = iTotal;
					pfts.currentFileProgress = iTotal;
					ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (void *)di, (LPARAM) &pfts);
				}

				// close the connection once the whole file has been sent an completely ack'ed
				if ( iLastAck >= di->dwSize ) {
					Netlib_CloseHandle(con);
					con = NULL;
			}	}

DCC_STOP:
			// need to close the connection if it isn't allready
			if ( con ) {
				Netlib_CloseHandle(con);
				con = NULL;
			}

			// ack the progress one final time
			tLastActivity = time(0);
			pfts.totalProgress = iTotal;
			pfts.currentFileProgress = iTotal;
			ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (void *)di, (LPARAM) &pfts);

			fclose(hFile);
		}
		else // file was not possible to open for reading
		{
			ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, (void *)di, 0);
			if( con ) {
				Netlib_CloseHandle(con);
				con = NULL;
	}	}	}

	delete []chBuf;
	delete this; // ... and hopefully all went right, cuz here the object is deleted in any case
}

// This is called when receiving a file from a remote computer.
void CDccSession::DoReceiveFile() 
{
	// initialize the filetransfer dialog
	ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, (void *)di, 0);

	BYTE chBuf[1024*32+1];
	BYTE DCCMode = DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0); // type of dcc: normal, send-ahead
	WORD wAckRate = DBGetContactSettingWord(NULL, IRCPROTONAME, "DCCAckRate", 1024*4); 

	// do some stupid thing so  the filetransfer dialog shows the right thing
	ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, (void *)di, 0);

	// open the file for writing (and reading in case it is a resume)
	FILE* hFile = _tfopen( di->sFileAndPath.c_str(), dwWhatNeedsDoing == FILERESUME_RESUME ? _T("rb+"): _T("wb"));
	if ( hFile ) {	
		DWORD iLastAck = 0;

		// dwResumePos and dwWhatNeedsDoing has possibly been set using InterlockedExchange()
		// if set it is a resume and we adjust variables and the file pointer accordingly.
		if ( dwResumePos && dwWhatNeedsDoing == FILERESUME_RESUME ) {
			fseek (hFile,dwResumePos,SEEK_SET);
			iTotal = dwResumePos;
			iLastAck = dwResumePos;
			pfts.totalProgress = dwResumePos;
			pfts.currentFileProgress = dwResumePos;
		}

		// send an initial ack for the percentage-ready meter
		ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (void *)di, (LPARAM) &pfts);
		
		// the while loop will spin around till the connection is dropped, locally or by the remote computer.
		while ( con ) {
			// read
			DWORD cbRead = NLReceive((unsigned char*)chBuf, sizeof(chBuf));
			if ( cbRead <= 0 )
				break;
			
			// write it to the file
			fwrite(chBuf, 1, cbRead, hFile);

			iTotal += cbRead;

			// this snippet sends out an ack for every 4 kb received in send ahead
			// or every packet for normal mode
			if ( !di->bTurbo ) {
				DWORD no = htonl(iTotal);
				NLSend((const unsigned char *)&no, sizeof(DWORD));
				iLastAck = iTotal;
			}
			else iLastAck = iTotal;

			// sets the 'last update time' to check for timed out connections, and also make sure we only
			// ack the 'percentage-ready meter' only once a second to save CPU.
			if ( tLastPercentageUpdate < time( 0 )) {
				tLastPercentageUpdate = time (0);
				pfts.totalProgress = iTotal;
				pfts.currentFileProgress = iTotal;
				ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (void *)di, (LPARAM) &pfts);
			}	
			
			// if file size is known and everything is received then disconnect 
			if ( di->dwSize && di->dwSize == iTotal ) {
				Netlib_CloseHandle(con);
				con = NULL;
		}	}
		// receiving loop broken locally or by remote computer, just some cleaning up left....
		
		pfts.totalProgress = iTotal;
		pfts.currentFileProgress = iTotal;
		ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (void *)di, (LPARAM) &pfts);
		fclose(hFile);
	}
	else  {
		ProtoBroadcastAck(IRCPROTONAME, di->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, (void *)di, 0);
		if ( con ) { // file not possible to open for writing so we ack FAILURE and close the handle
			Netlib_CloseHandle(con);
			con = NULL;
	}	}

	delete this; // and finally the object is deleted
}

// this function handles receiving text in dcc chats and then send it in the protochain. very uncomplicated...
// For sending text the SendStuff() function is called (with the help of some function in CIrcSession to find the right
// Dcc object). See CIrcSession for info on how the dcc objects are stored, retreived and deleted.

void CDccSession::DoChatReceive()
{
	char chBuf[1024*4+1]; 
	int cbInBuf = 0;
	
	// loop to spin around while there is a connection
	while( con ) {
		int cbRead;
		int nLinesProcessed = 0;
		
		cbRead = NLReceive((unsigned char*)chBuf+cbInBuf, sizeof(chBuf)-cbInBuf-1);
		if( cbRead <= 0 )
			break;

		cbInBuf += cbRead;
		chBuf[cbInBuf] = '\0';
		
		char* pStart = chBuf;
		while( *pStart ) {
			char* pEnd;
			
			// seek end-of-line
			for(pEnd=pStart; *pEnd && *pEnd != '\r' && *pEnd != '\n'; ++pEnd)
				;
			if( *pEnd == '\0' )
				break; // uncomplete message. stop parsing.
			
			++nLinesProcessed;
			
			// replace end-of-line with NULLs and skip
			while( *pEnd == '\r' || *pEnd == '\n' )
				*pEnd++ = '\0';
			
			if( *pStart ) {
				// send it off to some messaging module
				CCSDATA ccs; 
				PROTORECVEVENT pre;
				ccs.szProtoService = PSR_MESSAGE;
				
				ccs.hContact = di->hContact;
				ccs.wParam = 0;
				ccs.lParam = (LPARAM) &pre;
				pre.flags = 0;
				pre.timestamp = (DWORD)time(NULL);
				pre.szMessage = (char*)DoColorCodes((TCHAR*)pStart, true, false); //!!!! // remove color codes
				pre.lParam = 0;
				CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			}
			
			cbInBuf -= pEnd - pStart;
			pStart = pEnd;
		}
		
		// discard processed messages
		if ( nLinesProcessed != 0 )
			memmove(chBuf, pStart, cbInBuf+1);
	}

	delete this; // delete the object when the connection is dropped
}

// disconnect the stuff
int CDccSession::Disconnect()
{
	if ( hBindPort ) {
		Netlib_CloseHandle(hBindPort);
		hBindPort = NULL;
	}

	// if 'con' exists it is cuz a connection exists. 
	// Terminating 'con' will cause any spawned threads to die and then the object will destroy itself.
	if ( con ) {
		Netlib_CloseHandle(con);
		con = NULL;
	}
	else delete this; // if 'con' do not exist (no connection made so far from the object) the object is destroyed

	return TRUE;
}

////////////////////////////////////////////////////////////////////
// check if the dcc chats should disconnect ( default 5 minute timeout )

VOID CALLBACK DCCTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	g_ircSession.CheckDCCTimeout();
}

// helper function for incoming dcc connections.
void DoIncomingDcc(HANDLE hConnection, DWORD dwRemoteIP, void * p1)
{
	CDccSession* dcc = (CDccSession*)p1;
	dcc->IncomingConnection(hConnection, dwRemoteIP);
}

// ident server
void DoIdent(HANDLE hConnection, DWORD dwRemoteIP, void* extra)
{
	char szBuf[1024];
	int cbRead = Netlib_Recv(hConnection, szBuf, sizeof(szBuf)-1, 0);
	if( cbRead == SOCKET_ERROR || cbRead == 0)
		return ;

	szBuf[cbRead] = '\0';
	rtrim( szBuf );

	char buf[1024*4];
	mir_snprintf(buf, SIZEOF(buf), "%s : USERID : " TCHAR_STR_PARAM " : " TCHAR_STR_PARAM "\r\n", 
		szBuf, g_ircSession.GetInfo().sIdentServerType.c_str() , g_ircSession.GetInfo().sUserID.c_str());
	Netlib_Send(hConnection, (const char*)buf, strlen(buf), 0);
	Netlib_CloseHandle(hConnection);
}
