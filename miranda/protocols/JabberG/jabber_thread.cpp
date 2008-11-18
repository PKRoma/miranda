/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"

#include <windns.h>   // requires Windows Platform SDK

#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_secur.h"
#include "jabber_caps.h"
#include "jabber_privacy.h"
#include "jabber_rc.h"

#ifndef DNS_TYPE_SRV
#define DNS_TYPE_SRV 0x0021
#endif

// <iq/> identification number for various actions
// for JABBER_REGISTER thread
int iqIdRegGetReg;
int iqIdRegSetReg;

// XML Console
#define JCPF_IN      0x01UL
#define JCPF_OUT     0x02UL
#define JCPF_ERROR   0x04UL

//extern int bSecureIM;
static VOID CALLBACK JabberDummyApcFunc( DWORD param )
{
	return;
}

static char onlinePassword[128];
static HANDLE hEventPasswdDlg;

static BOOL CALLBACK JabberPasswordDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{	TCHAR text[128];
			mir_sntprintf( text, SIZEOF(text), _T("%s %s"), TranslateT( "Enter password for" ), ( TCHAR* )lParam );
			SetDlgItemText( hwndDlg, IDC_JID, text );
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			GetDlgItemTextA( hwndDlg, IDC_PASSWORD, onlinePassword, SIZEOF( onlinePassword ));
			//EndDialog( hwndDlg, ( int ) onlinePassword );
			//return TRUE;
			// Fall through
		case IDCANCEL:
			//EndDialog( hwndDlg, 0 );
			SetEvent( hEventPasswdDlg );
			DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static VOID CALLBACK JabberPasswordCreateDialogApcProc( DWORD param )
{
	CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_PASSWORD ), NULL, JabberPasswordDlgProc, ( LPARAM )param );
}

static VOID CALLBACK JabberOfflineChatWindows( CJabberProto* ppro )
{
	GCDEST gcd = { ppro->m_szModuleName, NULL, GC_EVENT_CONTROL };
	GCEVENT gce = { 0 };
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	CallService( MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Jabber keep-alive thread

void CJabberProto::OnPingReply( HXML node, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo )
		return;
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_FAIL ) {
		// disconnect because of timeout
		SetStatus(ID_STATUS_OFFLINE);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

typedef DNS_STATUS (WINAPI *DNSQUERYA)(IN PCSTR pszName, IN WORD wType, IN DWORD Options, IN PIP4_ARRAY aipServers OPTIONAL, IN OUT PDNS_RECORD *ppQueryResults OPTIONAL, IN OUT PVOID *pReserved OPTIONAL);
typedef void (WINAPI *DNSFREELIST)(IN OUT PDNS_RECORD pRecordList, IN DNS_FREE_TYPE FreeType);

static int xmpp_client_query( char* domain )
{
	HINSTANCE hDnsapi = LoadLibraryA( "dnsapi.dll" );
	if ( hDnsapi == NULL )
		return 0;

	DNSQUERYA pDnsQuery = (DNSQUERYA)GetProcAddress(hDnsapi, "DnsQuery_A");
	DNSFREELIST pDnsRecordListFree = (DNSFREELIST)GetProcAddress(hDnsapi, "DnsRecordListFree");
	if ( pDnsQuery == NULL ) {
		//dnsapi.dll is not the needed dnsapi ;)
		FreeLibrary( hDnsapi );
		return 0;
	}

   char temp[256];
	mir_snprintf( temp, SIZEOF(temp), "_xmpp-client._tcp.%s", domain );

	DNS_RECORD *results = NULL;
	DNS_STATUS status = pDnsQuery(temp, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &results, NULL);
	if (FAILED(status)||!results || results[0].Data.Srv.pNameTarget == 0||results[0].wType != DNS_TYPE_SRV) {
		FreeLibrary(hDnsapi);
		return NULL;
	}

	strncpy(domain, (char*)results[0].Data.Srv.pNameTarget, 127);
	int port = results[0].Data.Srv.wPort;
	pDnsRecordListFree(results, DnsFreeRecordList);
	FreeLibrary(hDnsapi);
	return port;
}

void CJabberProto::xmlStreamInitialize(char *szWhich)
{
	Log("Stream will be initialized %s", szWhich);
	if ( m_szXmlStreamToBeInitialized )
		free( m_szXmlStreamToBeInitialized );
	m_szXmlStreamToBeInitialized = _strdup( szWhich );
}

void CJabberProto::xmlStreamInitializeNow(ThreadData* info)
{
	Log( "Stream is initializing %s",
		m_szXmlStreamToBeInitialized ? m_szXmlStreamToBeInitialized : "after connect" );
	if (m_szXmlStreamToBeInitialized) {
		free( m_szXmlStreamToBeInitialized );
		m_szXmlStreamToBeInitialized = NULL;
	}

	HXML n = xi.createNode( _T("xml"), NULL, 1 ) << XATTR( _T("version"), _T("1.0")) << XATTR( _T("encoding"), _T("UTF-8"));
	
	HXML stream = n << XCHILDNS( _T("stream:stream" ), _T("jabber:client")) << XATTR( _T("to"), _A2T(info->server))
		<< XATTR( _T("xmlns:stream"), _T("http://etherx.jabber.org/streams"));

	TCHAR *szXmlLang = GetXmlLang();
	if ( szXmlLang ) {
		xmlAddAttr( stream, _T("xml:lang"), szXmlLang );
		mir_free( szXmlLang );
	}
	if ( !m_options.Disable3920auth )
		xmlAddAttr( stream, _T("version"), _T("1.0"));

	LPTSTR xmlQuery = xi.toString( n, NULL );
	char* buf = mir_utf8encodeT( xmlQuery );
	int bufLen = strlen( buf );
	if ( bufLen > 2 ) {
		strdel( buf + bufLen - 2, 1 );
		bufLen--;
	}

	info->send( buf, bufLen );
	mir_free( buf );
	xi.freeMem( xmlQuery );
	xi.destroyNode( n );
}

static int utfLen( TCHAR* p, size_t len )
{
	int result = 0;
	for ( size_t i=0; i < len && *p; i++ ) {
		WORD w = *p++;
		if ( w < 0x80 )
			result++;
		else if ( w < 0x800 )
			result += 2;
		else
			result += 3;
	}

	return result;
}

void CJabberProto::ServerThread( ThreadData* info )
{
	DBVARIANT dbv;
	char* buffer;
	int datalen;
	int oldStatus;

	Log( "Thread started: type=%d", info->type );

	info->resolveID = -1;
	info->auth = NULL;
	if ( info->type == JABBER_SESSION_NORMAL ) {

		// Normal server connection, we will fetch all connection parameters
		// e.g. username, password, etc. from the database.

		if ( m_ThreadInfo != NULL ) {
			// Will not start another connection thread if a thread is already running.
			// Make APC call to the main thread. This will immediately wake the thread up
			// in case it is asleep in the reconnect loop so that it will immediately
			// reconnect.
			QueueUserAPC( JabberDummyApcFunc, m_ThreadInfo->hThread, 0 );
			Log( "Thread ended, another normal thread is running" );
LBL_Exit:
			delete info;
			return;
		}

		m_ThreadInfo = info;
		if ( m_szStreamId ) mir_free( m_szStreamId );
		m_szStreamId = NULL;

		if ( !JGetStringT( NULL, "LoginName", &dbv )) {
			_tcsncpy( info->username, dbv.ptszVal, SIZEOF( info->username )-1 );
			JFreeVariant( &dbv );
		}

		if ( *rtrim(info->username) == '\0' ) {
			DWORD dwSize = SIZEOF( info->username );
			if ( GetUserName( info->username, &dwSize ))
				JSetStringT( NULL, "LoginName", info->username );
			else
				info->username[0] = 0;
		}

		if ( *rtrim(info->username) == '\0' ) {
			Log( "Thread ended, login name is not configured" );
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
LBL_FatalError:
			m_ThreadInfo = NULL;
			oldStatus = m_iStatus;
			m_iStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
         goto LBL_Exit;
		}

		if ( !DBGetContactSettingString( NULL, m_szModuleName, "LoginServer", &dbv )) {
			strncpy( info->server, dbv.pszVal, SIZEOF( info->server )-1 );
			JFreeVariant( &dbv );
		}
		else {
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
			Log( "Thread ended, login server is not configured" );
			goto LBL_FatalError;
		}

		if ( m_options.HostNameAsResource == FALSE )
			if ( !JGetStringT( NULL, "Resource", &dbv )) {
				_tcsncpy( info->resource, dbv.ptszVal, SIZEOF( info->resource ) - 1 );
				JFreeVariant( &dbv );
			}
			else _tcscpy( info->resource, _T("Miranda"));
		else {
			DWORD dwCompNameLen = SIZEOF( info->resource ) - 1;
			if ( !GetComputerName( info->resource, &dwCompNameLen ))
				_tcscpy( info->resource, _T( "Miranda" ));
		}

		TCHAR jidStr[128];
		mir_sntprintf( jidStr, SIZEOF( jidStr ), _T("%s@") _T(TCHAR_STR_PARAM) _T("/%s"), info->username, info->server, info->resource );
		_tcsncpy( info->fullJID, jidStr, SIZEOF( info->fullJID )-1 );

		if ( m_options.SavePassword == FALSE ) {
			mir_sntprintf( jidStr, SIZEOF( jidStr ), _T("%s@") _T(TCHAR_STR_PARAM), info->username, info->server );

			// Ugly hack: continue logging on only the return value is &( onlinePassword[0] )
			// because if WM_QUIT while dialog box is still visible, p is returned with some
			// exit code which may not be NULL.
			// Should be better with modeless.
			onlinePassword[0] = ( char ) -1;
			hEventPasswdDlg = CreateEvent( NULL, FALSE, FALSE, NULL );
			QueueUserAPC( JabberPasswordCreateDialogApcProc, hMainThread, ( DWORD )jidStr );
			WaitForSingleObject( hEventPasswdDlg, INFINITE );
			CloseHandle( hEventPasswdDlg );

			if ( onlinePassword[0] == ( char ) -1 ) {
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
				Log( "Thread ended, password request dialog was canceled" );
				goto LBL_FatalError;
			}
			strncpy( info->password, onlinePassword, SIZEOF( info->password ));
			info->password[ SIZEOF( info->password )-1] = '\0';
		}
		else {
			if ( DBGetContactSettingString( NULL, m_szModuleName, "Password", &dbv )) {
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
				Log( "Thread ended, password is not configured" );
				goto LBL_FatalError;
			}
			JCallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
			strncpy( info->password, dbv.pszVal, SIZEOF( info->password ));
			info->password[SIZEOF( info->password )-1] = '\0';
			JFreeVariant( &dbv );
		}

		if ( m_options.ManualConnect == TRUE ) {
			if ( !DBGetContactSettingString( NULL, m_szModuleName, "ManualHost", &dbv )) {
				strncpy( info->manualHost, dbv.pszVal, SIZEOF( info->manualHost ));
				info->manualHost[sizeof( info->manualHost )-1] = '\0';
				JFreeVariant( &dbv );
			}
			info->port = JGetWord( NULL, "ManualPort", JABBER_DEFAULT_PORT );
		}
		else info->port = JGetWord( NULL, "Port", JABBER_DEFAULT_PORT );

		info->useSSL = m_options.UseSSL;
	}

	else if ( info->type == JABBER_SESSION_REGISTER ) {
		// Register new user connection, all connection parameters are already filled-in.
		// Multiple thread allowed, although not possible : )
		// thinking again.. multiple thread should not be allowed
		info->reg_done = FALSE;
		SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 25, ( LPARAM )TranslateT( "Connecting..." ));
		iqIdRegGetReg = -1;
		iqIdRegSetReg = -1;
	}
	else {
		Log( "Thread ended, invalid session type" );
		goto LBL_FatalError;
	}

	char connectHost[128];
	if ( info->manualHost[0] == 0 ) {
		int port_temp;
		strncpy( connectHost, info->server, SIZEOF(info->server));
		if ( port_temp = xmpp_client_query( connectHost )) { // port_temp will be > 0 if resolution is successful
			Log("%s%s resolved to %s:%d","_xmpp-client._tcp.",info->server,connectHost,port_temp);
			if (info->port==0 || info->port==5222)
				info->port = port_temp;
		}
		else Log("%s%s not resolved", "_xmpp-client._tcp.", connectHost);
	}
	else strncpy( connectHost, info->manualHost, SIZEOF(connectHost)); // do not resolve if manual host is selected

	Log( "Thread type=%d server='%s' port='%d'", info->type, connectHost, info->port );

	int jabberNetworkBufferSize = 2048;
	if (( buffer=( char* )mir_alloc( jabberNetworkBufferSize+1 )) == NULL ) {	// +1 is for '\0' when debug logging this buffer
		Log( "Cannot allocate network buffer, thread ended" );
		if ( info->type == JABBER_SESSION_NORMAL ) {
			oldStatus = m_iStatus;
			m_iStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
			m_ThreadInfo = NULL;
		}
		else if ( info->type == JABBER_SESSION_REGISTER ) {
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Not enough memory" ));
		}
		Log( "Thread ended, network buffer cannot be allocated" );
		goto LBL_FatalError;
	}

	info->s = WsConnect( connectHost, info->port );
	if ( info->s == NULL ) {
		Log( "Connection failed ( %d )", WSAGetLastError());
		if ( info->type == JABBER_SESSION_NORMAL ) {
			if ( m_ThreadInfo == info ) {
				oldStatus = m_iStatus;
				m_iStatus = ID_STATUS_OFFLINE;
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
				JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
				m_ThreadInfo = NULL;
		}	}
		else if ( info->type == JABBER_SESSION_REGISTER )
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Cannot connect to the server" ));

		Log( "Thread ended, connection failed" );
		mir_free( buffer );
		goto LBL_FatalError;
	}

	// Determine local IP
	int socket = JCallService( MS_NETLIB_GETSOCKET, ( WPARAM ) info->s, 0 );
	if ( info->type==JABBER_SESSION_NORMAL && socket!=INVALID_SOCKET ) {
		struct sockaddr_in saddr;
		int len;

		len = sizeof( saddr );
		getsockname( socket, ( struct sockaddr * ) &saddr, &len );
		m_dwJabberLocalIP = saddr.sin_addr.S_un.S_addr;
		Log( "Local IP = %s", inet_ntoa( saddr.sin_addr ));
	}

	if ( info->useSSL ) {
		Log( "Intializing SSL connection" );
		if (!JCallService( MS_NETLIB_STARTSSL, ( WPARAM )info->s, 0)) {
			Log( "SSL intialization failed" );
			if ( info->type == JABBER_SESSION_NORMAL ) {
				oldStatus = m_iStatus;
				m_iStatus = ID_STATUS_OFFLINE;
				JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
				if ( m_ThreadInfo == info )
					m_ThreadInfo = NULL;
			}
			else if ( info->type == JABBER_SESSION_REGISTER ) {
				SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Cannot connect to the server" ));
			}
			mir_free( buffer );
			Log( "Thread ended, SSL connection failed" );
			goto LBL_FatalError;
	}	}

	// User may change status to OFFLINE while we are connecting above
	if ( m_iDesiredStatus != ID_STATUS_OFFLINE || info->type == JABBER_SESSION_REGISTER ) {

		if ( info->type == JABBER_SESSION_NORMAL ) {
			m_bJabberConnected = TRUE;
			int len = _tcslen( info->username ) + strlen( info->server )+1;
			m_szJabberJID = ( TCHAR* )mir_alloc( sizeof( TCHAR)*( len+1 ));
			mir_sntprintf( m_szJabberJID, len+1, _T("%s@") _T(TCHAR_STR_PARAM), info->username, info->server );
			if ( m_options.KeepAlive )
				m_bSendKeepAlive = TRUE;
			else
				m_bSendKeepAlive = FALSE;
			JSetStringT(NULL, "jid", m_szJabberJID); // store jid in database
		}

		xmlStreamInitializeNow( info );
		TCHAR* tag = _T("stream:stream");

		Log( "Entering main recv loop" );
		datalen = 0;

		for ( ;; ) {
			if ( !info->useZlib || info->zRecvReady ) {
				NETLIBSELECT nls = {0};
				nls.cbSize = sizeof( NETLIBSELECT );
				nls.dwTimeout = 60000; // 60 seconds
				nls.hReadConns[0] = info->s;
				int nSelRes = JCallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls );
				if ( nSelRes == -1 ) // error
					break;
				else if ( nSelRes == 0 ) {
					if ( m_options.EnableServerXMPPPing )
						info->send( 
							XmlNodeIq( m_iqManager.AddHandler( &CJabberProto::OnPingReply, JABBER_IQ_TYPE_GET, NULL, 0, -1, this, 0, 45000 ))
								<< XQUERY( _T(JABBER_FEAT_PING)));
					else if ( m_bSendKeepAlive )
						info->send( " \t " );
					continue;
			}	}

			int recvResult = info->recv( buffer+datalen, jabberNetworkBufferSize-datalen), bytesParsed;
			Log( "recvResult = %d", recvResult );
			if ( recvResult <= 0 )
				break;
			datalen += recvResult;

			buffer[datalen] = '\0';

			TCHAR* str;
			#if defined( _UNICODE )
				str = mir_utf8decodeW( buffer );
			#else
				char* bufCopy = NEWSTR_ALLOCA(buffer);
				str = mir_utf8decode( bufCopy, 0 );
			#endif

			bytesParsed = 0;
			XmlNode root( str, &bytesParsed, tag );
			bytesParsed = ( root ) ? utfLen( str, bytesParsed ) : 0;
			Log( "bytesParsed = %d", bytesParsed );
			mir_free(str);

			tag = NULL;

			if ( xmlGetName( root ) == NULL ) {
				for ( int i=0; ; i++ ) {
					HXML n = xmlGetChild( root , i );
					if ( !n )
						break;
					OnProcessProtocol( n, info );
				}	
			}
			else OnProcessProtocol( root, info );

			if ( bytesParsed > 0 ) {
				if ( bytesParsed < datalen )
					memmove( buffer, buffer+bytesParsed, datalen-bytesParsed );
				datalen -= bytesParsed;
			}
			else if ( datalen == jabberNetworkBufferSize ) {
				//jabberNetworkBufferSize += 65536;
				jabberNetworkBufferSize *= 2;
				Log( "Increasing network buffer size to %d", jabberNetworkBufferSize );
				if (( buffer=( char* )mir_realloc( buffer, jabberNetworkBufferSize+1 )) == NULL ) {
					Log( "Cannot reallocate more network buffer, go offline now" );
					break;
			}	}
			else Log( "Unknown state: bytesParsed=%d, datalen=%d, jabberNetworkBufferSize=%d", bytesParsed, datalen, jabberNetworkBufferSize );

			if ( m_szXmlStreamToBeInitialized ) {
				xmlStreamInitializeNow( info );
				tag = _T("stream:stream");
		}	}

		if ( info->type == JABBER_SESSION_NORMAL ) {
			m_iqManager.ExpireAll( info );
			m_bJabberOnline = FALSE;
			m_bJabberConnected = FALSE;
			info->zlibUninit();
			EnableMenuItems( FALSE );
			RebuildStatusMenu();
			RebuildInfoFrame();
			if ( m_hwndJabberChangePassword ) {
				//DestroyWindow( hwndJabberChangePassword );
				// Since this is a different thread, simulate the click on the cancel button instead
				SendMessage( m_hwndJabberChangePassword, WM_COMMAND, MAKEWORD( IDCANCEL, 0 ), 0 );
			}

			if ( jabberChatDllPresent )
				QueueUserAPC(( PAPCFUNC )JabberOfflineChatWindows, hMainThread, ( LPARAM )this );

			ListRemoveList( LIST_CHATROOM );
			ListRemoveList( LIST_BOOKMARK );
			//UI_SAFE_NOTIFY(m_pDlgJabberJoinGroupchat, WM_JABBER_CHECK_ONLINE);
			UI_SAFE_NOTIFY_HWND(m_hwndJabberAddBookmark, WM_JABBER_CHECK_ONLINE);
			//UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_CHECK_ONLINE);
			WindowNotify(WM_JABBER_CHECK_ONLINE);

			// Set status to offline
			oldStatus = m_iStatus;
			m_iStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );

			// Set all contacts to offline
			HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			while ( hContact != NULL ) {
				if ( !lstrcmpA(( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 ), m_szModuleName ))
				{
					SetContactOfflineStatus( hContact );
					MenuHideSrmmIcon( hContact );
				}

				hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
			}

			mir_free( m_szJabberJID );
			m_szJabberJID = NULL;
			m_tmJabberLoggedInTime = 0;
			ListWipe();

			WindowNotify(WM_JABBER_REFRESH_VCARD);
		}
		else if ( info->type==JABBER_SESSION_REGISTER && !info->reg_done )
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Connection lost" ));
	}
	else if ( info->type == JABBER_SESSION_NORMAL ) {
		oldStatus = m_iStatus;
		m_iStatus = ID_STATUS_OFFLINE;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
	}

	Log( "Thread ended: type=%d server='%s'", info->type, info->server );

	if ( info->type==JABBER_SESSION_NORMAL && m_ThreadInfo==info ) {
		if ( m_szStreamId ) mir_free( m_szStreamId );
		m_szStreamId = NULL;
		m_ThreadInfo = NULL;
	}

	mir_free( buffer );
	Log( "Exiting ServerThread" );
	goto LBL_Exit;
}

void CJabberProto::PerformRegistration( ThreadData* info )
{
	iqIdRegGetReg = SerialNext();
	info->send( XmlNodeIq( _T("get"), iqIdRegGetReg, NULL) << XQUERY( _T(JABBER_FEAT_REGISTER )));

	SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 50, ( LPARAM )TranslateT( "Requesting registration instruction..." ));
}

void CJabberProto::PerformIqAuth( ThreadData* info )
{
	if ( info->type == JABBER_SESSION_NORMAL ) {
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetAuth );
		info->send( XmlNodeIq( _T("get"), iqId ) << XQUERY( _T("jabber:iq:auth" )) << XCHILD( _T("username"), info->username ));
	}
	else if ( info->type == JABBER_SESSION_REGISTER )
		PerformRegistration( info );
}

/////////////////////////////////////////////////////////////////////////////////////////

void CJabberProto::OnProcessStreamOpening( HXML node, ThreadData *info )
{
	if ( lstrcmp( xmlGetName( node ), _T("stream:stream")))
		return;

	if ( info->type == JABBER_SESSION_NORMAL ) {
		const TCHAR* sid = xmlGetAttrValue( node, _T("id"));
		if ( sid != NULL ) {
			char* pszSid = mir_t2a( sid );
			replaceStr( info->proto->m_szStreamId, pszSid );
			mir_free( pszSid );
	}	}

	// old server - disable SASL then
	if ( xmlGetAttrValue( node, _T("version")) == NULL )
		info->proto->m_options.Disable3920auth = TRUE;

	if ( info->proto->m_options.Disable3920auth )
		info->proto->PerformIqAuth( info );

	HXML features = xmlGetChild( node ,0);
	if ( features )
		if ( !lstrcmp( xmlGetName( features ), _T("stream:features")))
			OnProcessFeatures( features, info );
}

void CJabberProto::OnProcessStreamClosing( HXML node, ThreadData *info )
{
	Netlib_CloseHandle( info->s );
	if ( !lstrcmp( xmlGetName( node ), _T("stream:error")) && xmlGetText( node ) )
		MessageBox( NULL, TranslateTS( xmlGetText( node ) ), TranslateT( "Jabber Connection Error" ), MB_OK|MB_ICONERROR|MB_SETFOREGROUND );
}

/////////////////////////////////////////////////////////////////////////////////////////

void CJabberProto::OnProcessFeatures( HXML node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	bool isPlainAvailable = false;
	bool isMd5available = false;
	bool isNtlmAvailable = false;
	bool isAuthAvailable = false;
	bool isXGoogleTokenAvailable = false;
	bool isRegisterAvailable = false;
	bool areMechanismsDefined = false;
	bool isSessionAvailable = false;

	for ( int i=0; ;i++ ) {
		HXML n = xmlGetChild( node ,i);
		if ( !n )
			break;

		if ( !_tcscmp( xmlGetName( n ), _T("starttls"))) {
			if ( !info->useSSL && m_options.UseTLS ) {
				Log( "Requesting TLS" );
				info->send( XmlNode( xmlGetName( n )) << XATTR( _T("xmlns"), _T("urn:ietf:params:xml:ns:xmpp-tls" )));
				return;
		}	}

		if ( !_tcscmp( xmlGetName( n ), _T("compression")) && m_options.EnableZlib == TRUE ) {
			Log("Server compression available");
			for ( int k=0; ; k++ ) {
				HXML c = xmlGetChild( n ,k);
				if ( !c )
					break;

				if ( !_tcscmp( xmlGetName( c ), _T("method"))) {
					if ( !_tcscmp( xmlGetText( c ), _T("zlib")) && info->zlibInit() == TRUE ) {
						Log("Requesting Zlib compression");
						info->send( XmlNode( _T("compress")) << XATTR( _T("xmlns"), _T("http://jabber.org/protocol/compress")) 
							<< XCHILD( _T("method"), _T("zlib")));
						return;
		}	}	}	}

		if ( !_tcscmp( xmlGetName( n ), _T("mechanisms"))) {
			areMechanismsDefined = true;
			//JabberLog("%d mechanisms\n",n->numChild);
			for ( int k=0; ; k++ ) {
				HXML c = xmlGetChild( n ,k);
				if ( !c )
					break;

				if ( !_tcscmp( xmlGetName( c ), _T("mechanism")))
					//JabberLog("Mechanism: %s",xmlGetText( c ));
					     if ( !_tcscmp( xmlGetText( c ), _T("PLAIN")))          isPlainAvailable = true;
					else if ( !_tcscmp( xmlGetText( c ), _T("DIGEST-MD5")))     isMd5available = true;
					else if ( !_tcscmp( xmlGetText( c ), _T("NTLM")))           isNtlmAvailable = true;
					else if ( !_tcscmp( xmlGetText( c ), _T("X-GOOGLE-TOKEN"))) isXGoogleTokenAvailable = true;
		}	}
		else if ( !_tcscmp( xmlGetName( n ), _T("register" ))) isRegisterAvailable = true;
		else if ( !_tcscmp( xmlGetName( n ), _T("auth"     ))) isAuthAvailable = true;
		else if ( !_tcscmp( xmlGetName( n ), _T("session"  ))) isSessionAvailable = true;
	}

	if ( areMechanismsDefined ) {
		char *PLAIN = NULL;
		TJabberAuth* auth = NULL;

		if ( isNtlmAvailable ) {
			auth = new TNtlmAuth( info );
			if ( !auth->isValid() ) {
				delete auth;
				auth = NULL;
		}	}

		if ( auth == NULL && isMd5available )
			auth = new TMD5Auth( info );

		if ( auth == NULL && isPlainAvailable )
			auth = new TPlainAuth( info );

		if ( auth == NULL ) {
			if ( isAuthAvailable ) { // no known mechanisms but iq_auth is available
				PerformIqAuth( info );
				return;
			}

			MessageBox( NULL, TranslateT("No known auth methods available. Giving up."), TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
			info->send( "</stream:stream>" );
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
			return;
		}

		if ( info->type == JABBER_SESSION_NORMAL ) {
			info->auth = auth;

			char* request = auth->getInitialRequest();
			info->send( XmlNode( _T("auth"), _A2T(request)) << XATTR( _T("xmlns"), _T("urn:ietf:params:xml:ns:xmpp-sasl")) 
				<< XATTR( _T("mechanism"), _A2T(auth->getName() )));
			mir_free( request );
		}
		else if ( info->type == JABBER_SESSION_REGISTER )
			PerformRegistration( info );
		else
			info->send( "</stream:stream>" );
		return;
	}

	// mechanisms are not defined.
	if ( info->auth ) { //We are already logged-in
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultBind );
		info->send( XmlNodeIq( _T("set"), iqId ) << XCHILDNS( _T("bind"), _T("urn:ietf:params:xml:ns:xmpp-bind" )) 
			<< XCHILD( _T("resource"), info->resource ));

		if ( isSessionAvailable )
			info->bIsSessionAvailable = TRUE;

		return;
	}

	//mechanisms not available and we are not logged in
	PerformIqAuth( info );
}

void CJabberProto::OnProcessFailure( HXML node, ThreadData* info )
{
	const TCHAR* type;
//failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"
	if (( type = xmlGetAttrValue( node, _T("xmlns"))) == NULL ) return;
	if ( !_tcscmp( type, _T("urn:ietf:params:xml:ns:xmpp-sasl") )) {
		info->send( "</stream:stream>" );

		TCHAR text[128];
		mir_sntprintf( text, SIZEOF( text ), _T("%s %s@")_T(TCHAR_STR_PARAM)_T("."), TranslateT( "Authentication failed for" ), info->username, info->server );
		MessageBox( NULL, text, TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
		JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnProcessError( HXML node, ThreadData* info )
{
	TCHAR *buff;
	int i;
	int pos;

	//failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"
	if ( !xmlGetChild( node ,0))
		return;

	buff = (TCHAR *)mir_alloc(1024*sizeof(TCHAR));
	pos=0;
	for ( i=0; ; i++ ) {
		HXML n = xmlGetChild( node , i );
		if ( !n )
			break;

		pos += mir_sntprintf( buff+pos, 1024-pos, _T(TCHAR_STR_PARAM)_T(": %s\n"), xmlGetName( n ), xmlGetText( n ));
		if ( !_tcscmp( xmlGetName( n ), _T("conflict")))
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
	}
	MessageBox( NULL, buff, TranslateT( "Jabber Error" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
	mir_free(buff);
	info->send( "</stream:stream>" );
}

void CJabberProto::OnProcessSuccess( HXML node, ThreadData* info )
{
	const TCHAR* type;
//	int iqId;
	// RECVED: <success ...
	// ACTION: if successfully logged in, continue by requesting roster list and set my initial status
	if (( type = xmlGetAttrValue( node, _T("xmlns"))) == NULL )
		return;

	if ( !_tcscmp( type, _T("urn:ietf:params:xml:ns:xmpp-sasl") )) {
		DBVARIANT dbv;

		Log( "Success: Logged-in." );
		if ( DBGetContactSettingString( NULL, m_szModuleName, "Nick", &dbv ))
			JSetStringT( NULL, "Nick", info->username );
		else
			JFreeVariant( &dbv );
		xmlStreamInitialize( "after successful sasl" );
	}
	else Log( "Success: unknown action "TCHAR_STR_PARAM".",type);
}

void CJabberProto::OnProcessChallenge( HXML node, ThreadData* info )
{
	if ( info->auth == NULL ) {
		Log( "No previous auth have been made, exiting..." );
		return;
	}

	if ( lstrcmp( xmlGetAttrValue( node, _T("xmlns")), _T("urn:ietf:params:xml:ns:xmpp-sasl")))
		return;

	char* challenge = info->auth->getChallenge( xmlGetText( node ));
	info->send( XmlNode( _T("response"), _A2T(challenge)) << XATTR( _T("xmlns"), _T("urn:ietf:params:xml:ns:xmpp-sasl")));
	mir_free( challenge );
}

void CJabberProto::OnProcessProtocol( HXML node, ThreadData* info )
{
	OnConsoleProcessXml(node, JCPF_IN);

	if ( !lstrcmp( xmlGetName( node ), _T("proceed")))
		OnProcessProceed( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("compressed")))
		OnProcessCompressed( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("stream:features")))
		OnProcessFeatures( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("stream:stream")))
		OnProcessStreamOpening( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("success")))
		OnProcessSuccess( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("failure")))
		OnProcessFailure( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("stream:error")))
		OnProcessError( node, info );
	else if ( !lstrcmp( xmlGetName( node ), _T("challenge")))
		OnProcessChallenge( node, info );
	else if ( info->type == JABBER_SESSION_NORMAL ) {
		if ( !lstrcmp( xmlGetName( node ), _T("message")))
			OnProcessMessage( node, info );
		else if ( !lstrcmp( xmlGetName( node ), _T("presence")))
			OnProcessPresence( node, info );
		else if ( !lstrcmp( xmlGetName( node ), _T("iq")))
			OnProcessIq( node, info );
		else
			Log( "Invalid top-level tag ( only <message/> <presence/> and <iq/> allowed )" );
	}
	else if ( info->type == JABBER_SESSION_REGISTER ) {
		if ( !lstrcmp( xmlGetName( node ), _T("iq")))
			OnProcessRegIq( node, info );
		else
			Log( "Invalid top-level tag ( only <iq/> allowed )" );
}	}

void CJabberProto::OnProcessProceed( HXML node, ThreadData* info )
{
	const TCHAR* type;
	if (( type = xmlGetAttrValue( node, _T("xmlns"))) != NULL && !lstrcmp( type, _T("error")))
		return;

	if ( !lstrcmp( type, _T("urn:ietf:params:xml:ns:xmpp-tls" ))) {
		Log("Starting TLS...");
		if (!JCallService( MS_NETLIB_STARTSSL, ( WPARAM )info->s, 0)) {
			Log( "SSL initialization failed" );
			SetStatus(ID_STATUS_OFFLINE);
		}
		else
			xmlStreamInitialize( "after successful StartTLS" );
}	}

void CJabberProto::OnProcessCompressed( HXML node, ThreadData* info )
{
	const TCHAR* type;

	Log( "Compression confirmed" );

	if (( type = xmlGetAttrValue( node, _T("xmlns"))) != NULL && !lstrcmp( type, _T( "error" )))
		return;
	if ( lstrcmp( type, _T( "http://jabber.org/protocol/compress" )))
		return;

	Log( "Starting Zlib stream compression..." );

	info->useZlib = TRUE;
	info->zRecvData = ( char* )mir_alloc( ZLIB_CHUNK_SIZE );

	xmlStreamInitialize( "after successful Zlib init" );
}

void CJabberProto::OnProcessPubsubEvent( HXML node )
{
	const TCHAR* from = xmlGetAttrValue( node, _T("from"));
	if ( !from )
		return;

	HXML eventNode = xmlGetChildByTag( node, "event", "xmlns", _T(JABBER_FEAT_PUBSUB_EVENT));
	if ( !eventNode )
		return;

	m_pepServices.ProcessEvent(from, eventNode);

	HANDLE hContact = HContactFromJID( from );
	if ( !hContact )
		return;

	HXML itemsNode;
	if ( m_options.EnableUserTune && (itemsNode = xmlGetChildByTag( eventNode, "items", "node", _T(JABBER_FEAT_USER_TUNE)))) {
		// node retract?
		if ( xmlGetChild( itemsNode , "retract" )) {
			SetContactTune( hContact, NULL, NULL, NULL, NULL, NULL, NULL );
			return;
		}

		HXML tuneNode = XPath( itemsNode, _T("item/tune[@xmlns='") _T(JABBER_FEAT_USER_TUNE) _T("']") );
		if ( !tuneNode )
			return;

		const TCHAR *szArtist = XPathT( tuneNode, "artist" );
		const TCHAR *szLength = XPathT( tuneNode, "length" );
		const TCHAR *szSource = XPathT( tuneNode, "source" );
		const TCHAR *szTitle = XPathT( tuneNode, "title" );
		const TCHAR *szTrack = XPathT( tuneNode, "track" );
		const TCHAR *szUri = XPathT( tuneNode, "uri" );
	
		TCHAR szLengthInTime[32];
		szLengthInTime[0] = _T('\0');
		if ( szLength ) {
			int nLength = _ttoi( szLength );
			mir_sntprintf( szLengthInTime, SIZEOF( szLengthInTime ),  _T("%02d:%02d:%02d"),
				nLength / 3600, (nLength / 60) % 60, nLength % 60 );
		}

		SetContactTune( hContact, szArtist, szLength ? szLengthInTime : NULL, szSource, szTitle, szTrack, szUri );
	}
}

// returns 0, if error or no events
DWORD JabberGetLastContactMessageTime( HANDLE hContact )
{
	// TODO: time cache can improve performance
	HANDLE hDbEvent = (HANDLE)JCallService( MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0 );
	if ( !hDbEvent )
		return 0;

	DWORD dwTime = 0;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = CallService( MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0 );
	if ( dbei.cbBlob != -1 ) {
		dbei.pBlob = (PBYTE)mir_alloc( dbei.cbBlob + 1 );
		int nGetTextResult = JCallService( MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei );
		if ( !nGetTextResult )
			dwTime = dbei.timestamp;
		mir_free( dbei.pBlob );
	}
	return dwTime;
}

HANDLE CJabberProto::CreateTemporaryContact( const TCHAR *szJid, JABBER_LIST_ITEM* chatItem )
{
	HANDLE hContact = NULL;
	if ( chatItem ) {
		const TCHAR* p = _tcschr( szJid, '/' );
		if ( p != NULL && p[1] != '\0' )
			p++;
		else
			p = szJid;
		hContact = DBCreateContact( szJid, p, TRUE, FALSE );

		for ( int i=0; i < chatItem->resourceCount; i++ ) {
			if ( !lstrcmp( chatItem->resource[i].resourceName, p )) {
				JSetWord( hContact, "Status", chatItem->resource[i].status );
				break;
			}
		}
	}
	else {
		TCHAR *nick = JabberNickFromJID( szJid );
		hContact = DBCreateContact( szJid, nick, TRUE, TRUE );
		mir_free( nick );
	}
	return hContact;
}

void CJabberProto::OnProcessMessage( HXML node, ThreadData* info )
{
	HXML subjectNode, xNode, inviteNode, idNode, n;
	LPCTSTR from, type, idStr, fromResource;
	HANDLE hContact;

	if ( !xmlGetName( node ) || _tcscmp( xmlGetName( node ), _T("message"))) 
		return;

	type = xmlGetAttrValue( node, _T("type"));
	if (( from = xmlGetAttrValue( node, _T("from"))) == NULL )
		return;

	hContact = HContactFromJID( from );

	HXML errorNode = xmlGetChild( node , "error" );
	if ( errorNode != NULL || !lstrcmp( type, _T("error"))) {
		// we check if is message delivery failure
		int id = JabberGetPacketID( node );
		JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, from );
		if ( item != NULL ) { // yes, it is
			TCHAR *szErrText = JabberErrorMsg(errorNode);
			char *errText = mir_t2a(szErrText);
			JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE ) id, (LPARAM)errText );
			mir_free(errText);
			mir_free(szErrText);
		}
		return;
	}

	if ( n = xmlGetChildByTag( node, "data", "xmlns", _T( JABBER_FEAT_IBB ))) {
		BOOL bOk = FALSE;
		const TCHAR *sid = xmlGetAttrValue( n, _T("sid"));
		const TCHAR *seq = xmlGetAttrValue( n, _T("seq"));
		if ( sid && seq && xmlGetText( n ) ) {
			bOk = OnIbbRecvdData( xmlGetText( n ), sid, seq );
		}
		return;
	}

	if ( n = xmlGetChildByTag( node, "event", "xmlns", _T( "http://jabber.org/protocol/pubsub#event" ))) {
		OnProcessPubsubEvent( node );
		return;
	}

	JABBER_LIST_ITEM *chatItem = ListGetItemPtr( LIST_CHATROOM, from );
	if (!lstrcmp( type, _T("groupchat")))
	{
		if ( chatItem )
		{	// process GC message
			GroupchatProcessMessage( node, info );
		} else
		{	// got message from onknown conference... let's leave it :)
//			TCHAR *conference = NEWTSTR_ALLOCA(from);
//			if (TCHAR *s = _tcschr(conference, _T('/'))) *s = 0;
//			XmlNode p( "presence" ); xmlAddAttr( p, "to", conference ); xmlAddAttr( p, "type", "unavailable" );
//			info->send( p );
		}
		return;
	}

	const TCHAR* szMessage = NULL;
	HXML bodyNode = xmlGetChild( node , "body" );
	if ( bodyNode != NULL && xmlGetText( bodyNode ) )
		szMessage = xmlGetText( bodyNode );
	if (( subjectNode = xmlGetChild( node , "subject" )) && xmlGetText( subjectNode ) && xmlGetText( subjectNode )[0] != _T('\0')) {
		int cbLen = (szMessage ? _tcslen( szMessage ) : 0) + _tcslen( xmlGetText( subjectNode ) ) + 128;
		TCHAR* szTmp = ( TCHAR * )alloca( sizeof(TCHAR) * cbLen );
		szTmp[0] = _T('\0');
		if ( szMessage )
			_tcscat( szTmp, _T("Subject: "));
		_tcscat( szTmp, xmlGetText( subjectNode ) );
		if ( szMessage ) {
			_tcscat( szTmp, _T("\r\n"));
			_tcscat( szTmp, szMessage );
		}
		szMessage = szTmp;
	}

	if ( szMessage && (n = xmlGetChildByTag( node, "addresses", "xmlns", _T(JABBER_FEAT_EXT_ADDRESSING)))) {
		HXML addressNode = xmlGetChildByTag( n, "address", "type", _T("ofrom") );
		if ( addressNode ) {
			const TCHAR* szJid = xmlGetAttrValue( addressNode, _T("jid"));
			if ( szJid ) {
				int cbLen = _tcslen( szMessage ) + 1000;
				TCHAR* p = ( TCHAR* )alloca( sizeof( TCHAR ) * cbLen );
				mir_sntprintf( p, cbLen, TranslateT("Message redirected from: %s\r\n%s"), from, szMessage );
				szMessage = p;
				from = szJid;
				// rewrite hContact
				hContact = HContactFromJID( from );
			}
		}
	}

	// If message is from a stranger ( not in roster ), item is NULL
	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, from );
	if ( !item )
		item = ListGetItemPtr( LIST_VCARD_TEMP, from );

	JABBER_RESOURCE_STATUS *resourceStatus = ResourceInfoFromJID( from );

	time_t msgTime = 0;
	BOOL  isChatRoomInvitation = FALSE;
	const TCHAR* inviteRoomJid = NULL;
	const TCHAR* inviteFromJid = NULL;
	const TCHAR* inviteReason = NULL;
	const TCHAR* invitePassword = NULL;
	BOOL delivered = FALSE;

	// check chatstates availability
	if ( resourceStatus && xmlGetChildByTag( node, "active", "xmlns", _T( JABBER_FEAT_CHATSTATES )))
		resourceStatus->jcbManualDiscoveredCaps |= JABBER_CAPS_CHATSTATES;

	// chatstates composing event
	if ( hContact && xmlGetChildByTag( node, "composing", "xmlns", _T( JABBER_FEAT_CHATSTATES )))
		JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM )hContact, 60 );

	// chatstates paused event
	if ( hContact && xmlGetChildByTag( node, "paused", "xmlns", _T( JABBER_FEAT_CHATSTATES )))
		JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM )hContact, PROTOTYPE_CONTACTTYPING_OFF );

	idStr = xmlGetAttrValue( node, _T("id"));

	// message receipts delivery request
	if ( xmlGetChildByTag( node, "request", "xmlns", _T( JABBER_FEAT_MESSAGE_RECEIPTS ))) {
		info->send(
			XmlNode( _T("message")) << XATTR( _T("to"), from ) << XATTR( _T("id"), idStr )
				<< XCHILDNS( _T("received"), _T(JABBER_FEAT_MESSAGE_RECEIPTS)));

		if ( resourceStatus )
			resourceStatus->jcbManualDiscoveredCaps |= JABBER_CAPS_MESSAGE_RECEIPTS;
	}

	// message receipts delivery notification
	if ( xmlGetChildByTag( node, "received", "xmlns", _T( JABBER_FEAT_MESSAGE_RECEIPTS ))) {
		int nPacketId = JabberGetPacketID( node );
		if ( nPacketId != -1)
			JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) nPacketId, 0 );
	}

	// XEP-0203 delay support
	if ( n = xmlGetChildByTag( node, "delay", "xmlns", _T("urn:xmpp:delay") ) ) {
		const TCHAR* ptszTimeStamp = xmlGetAttrValue( n, _T("stamp"));
		if ( ptszTimeStamp != NULL ) {
			// skip '-' chars
			TCHAR* szStamp = mir_tstrdup( ptszTimeStamp );
			int si = 0, sj = 0;
			while (1) {
				if ( szStamp[si] == _T('-') )
					si++;
				else
					if ( !( szStamp[sj++] = szStamp[si++] ))
						break;
			};
			msgTime = JabberIsoToUnixTime( szStamp );
			mir_free( szStamp );
		}
	}

	// XEP-0224 support (Attention/Nudge)
	if ( xmlGetChildByTag( node, "attention", "xmlns", _T( JABBER_FEAT_ATTENTION ))) {
		if ( !hContact )
			hContact = CreateTemporaryContact( from, chatItem );
		if ( hContact )
			NotifyEventHooks( m_hEventNudge, (WPARAM)hContact, 0 );
	}

	// chatstates gone event
	if ( hContact && xmlGetChildByTag( node, "gone", "xmlns", _T( JABBER_FEAT_CHATSTATES )) && m_options.LogChatstates ) {
		DBEVENTINFO dbei;
		BYTE bEventType = JABBER_DB_EVENT_CHATSTATES_GONE; // gone event
		dbei.cbSize = sizeof(dbei);
		dbei.pBlob = &bEventType;
		dbei.cbBlob = 1;
		dbei.eventType = JABBER_DB_EVENT_TYPE_CHATSTATES;
		dbei.flags = DBEF_READ;
		dbei.timestamp = time(NULL);
		dbei.szModule = m_szModuleName;
		CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
	}

	if (( n = xmlGetChildByTag( node, "confirm", "xmlns", _T( JABBER_FEAT_HTTP_AUTH ))) && m_options.AcceptHttpAuth ) {
		const TCHAR *szId = xmlGetAttrValue( n, _T("id"));
		const TCHAR *szMethod = xmlGetAttrValue( n, _T("method"));
		const TCHAR *szUrl = xmlGetAttrValue( n, _T("url"));
		if ( !szId || !szMethod || !szUrl )
			return;

		CJabberHttpAuthParams *pParams = (CJabberHttpAuthParams *)mir_alloc( sizeof( CJabberHttpAuthParams ));
		if ( !pParams )
			return;

		ZeroMemory( pParams, sizeof( CJabberHttpAuthParams ));
		pParams->m_nType = CJabberHttpAuthParams::MSG;
		pParams->m_szFrom = mir_tstrdup( from );
		HXML pThreadNode = xmlGetChild( node , "thread" );
		if ( pThreadNode && xmlGetText( pThreadNode ) && xmlGetText( pThreadNode )[0] )
			pParams->m_szThreadId = mir_tstrdup( xmlGetText( pThreadNode ) );
		pParams->m_szId = mir_tstrdup( szId );
		pParams->m_szMethod = mir_tstrdup( szMethod );
		pParams->m_szUrl = mir_tstrdup( szUrl );

		AddClistHttpAuthEvent( pParams );
		return;
	}

	for ( int i = 1; ( xNode = xmlGetNthChild( node, _T("x"), i )) != NULL; i++ ) {
		const TCHAR* ptszXmlns = xmlGetAttrValue( xNode, _T("xmlns"));
		if ( ptszXmlns == NULL )
			continue;

		if ( !_tcscmp( ptszXmlns, _T("jabber:x:encrypted" ))) {
			if ( xmlGetText( xNode ) == NULL )
				return;

			TCHAR* prolog = _T("-----BEGIN PGP MESSAGE-----\r\n\r\n");
			TCHAR* epilog = _T("\r\n-----END PGP MESSAGE-----\r\n");
			TCHAR* tempstring = ( TCHAR* )alloca( sizeof( TCHAR ) * ( _tcslen( prolog ) + _tcslen( xmlGetText( xNode ) ) + _tcslen( epilog ) + 3 ));
			_tcsncpy( tempstring, prolog, _tcslen( prolog ) + 1 );
			_tcsncpy( tempstring + _tcslen( prolog ), xmlGetText( xNode ), _tcslen( xmlGetText( xNode ) ) + 1);
			_tcsncpy( tempstring + _tcslen( prolog ) + _tcslen(xmlGetText( xNode ) ), epilog, _tcslen( epilog ) + 1);
			szMessage = tempstring;
      }
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:delay")) && msgTime == 0 ) {
			const TCHAR* ptszTimeStamp = xmlGetAttrValue( xNode, _T("stamp"));
			if ( ptszTimeStamp != NULL )
				msgTime = JabberIsoToUnixTime( ptszTimeStamp );
		}
		else if ( !_tcscmp( ptszXmlns, _T(JABBER_FEAT_MESSAGE_EVENTS))) {
			
			// set events support only if we discovered caps and if events not already set
			JabberCapsBits jcbCaps = GetResourceCapabilites( from, TRUE );
			if ( jcbCaps & JABBER_RESOURCE_CAPS_ERROR )
				jcbCaps = JABBER_RESOURCE_CAPS_NONE;
			// FIXME: disabled due to expired XEP-0022 and problems with bombus delivery checks
//			if ( jcbCaps && resourceStatus && (!(jcbCaps & JABBER_CAPS_MESSAGE_EVENTS)) )
//				resourceStatus->jcbManualDiscoveredCaps |= (JABBER_CAPS_MESSAGE_EVENTS | JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY);

			if ( bodyNode == NULL ) {
				idNode = xmlGetChild( xNode , "id" );
				if ( xmlGetChild( xNode , "delivered" ) != NULL || xmlGetChild( xNode , "offline" ) != NULL ) {
					int id = -1;
					if ( idNode != NULL && xmlGetText( idNode ) != NULL )
						if ( !_tcsncmp( xmlGetText( idNode ), _T(JABBER_IQID), strlen( JABBER_IQID )) )
							id = _ttoi(( xmlGetText( idNode ) )+strlen( JABBER_IQID ));

					if ( id != -1 )
						JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) id, 0 );
				}

				if ( hContact && xmlGetChild( xNode , "composing" ) != NULL )
					JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, 60 );

				// Maybe a cancel to the previous composing
				HXML child = xmlGetChild( xNode ,0);
				if ( hContact && ( !child || ( child && idNode != NULL )))
					JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, PROTOTYPE_CONTACTTYPING_OFF );
			}
			else {
				// Check whether any event is requested
				if ( !delivered && ( n = xmlGetChild( xNode , "delivered" )) != NULL ) {
					delivered = TRUE;

					XmlNode m( _T("message" )); m << XATTR( _T("to"), from );
					HXML x = m << XCHILDNS( _T("x"), _T(JABBER_FEAT_MESSAGE_EVENTS));
					x << XCHILD( _T("delivered"));
					x << XCHILD( _T("id"), idStr );
					info->send( m );
				}
				if ( item != NULL && xmlGetChild( xNode , "composing" ) != NULL ) {
					if ( item->messageEventIdStr )
						mir_free( item->messageEventIdStr );
					item->messageEventIdStr = ( idStr==NULL )?NULL:mir_tstrdup( idStr );
			}	}
		}
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:oob"))) {
			HXML urlNode;
			if ( ((urlNode = xmlGetChild( xNode , "url" )) != NULL) && xmlGetText( urlNode ) && xmlGetText( urlNode )[0] != _T('\0')) {
				int cbLen = (szMessage ? _tcslen( szMessage ) : 0) + _tcslen( xmlGetText( urlNode ) ) + 32;
				TCHAR* szTmp = ( TCHAR * )alloca( sizeof(TCHAR) * cbLen );
				_tcscpy( szTmp, xmlGetText( urlNode ) );
				if ( szMessage ) {
					_tcscat( szTmp, _T("\r\n"));
					_tcscat( szTmp, szMessage );
				}
				szMessage = szTmp;
			}
		}
		else if ( !_tcscmp( ptszXmlns, _T("http://jabber.org/protocol/muc#user"))) {
			if (( inviteNode = xmlGetChild( xNode , "invite" )) != NULL ) {
				inviteFromJid = xmlGetAttrValue( inviteNode, _T("from"));
				if (( n = xmlGetChild( inviteNode , "reason" )) != NULL )
					inviteReason = xmlGetText( n );
			}

			if (( n = xmlGetChild( xNode , "password" )) != NULL )
				invitePassword = xmlGetText( n );
		}
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:conference"))) {
			inviteRoomJid = xmlGetAttrValue( xNode, _T("jid"));
			if ( inviteReason == NULL )
				inviteReason = xmlGetText( xNode );
			isChatRoomInvitation = TRUE;
	}	}

	if ( isChatRoomInvitation ) {
		if ( inviteRoomJid != NULL ) {
			if ( m_options.IgnoreMUCInvites ) {
				// FIXME: temporary disabled due to MUC inconsistence on server side
				/*
				XmlNode m( "message" ); xmlAddAttr( m, "to", from );
				XmlNode xNode = xmlAddChild( m, "x" );
				xmlAddAttr( xNode, "xmlns", JABBER_FEAT_MUC_USER );
				XmlNode declineNode = xmlAddChild( xNode, "decline" );
				xmlAddAttr( declineNode, "from", inviteRoomJid );
				XmlNode reasonNode = xmlAddChild( declineNode, "reason", "The user has chosen to not accept chat invites" );
				info->send( m );
				*/
			}
			else
				GroupchatProcessInvite( inviteRoomJid, inviteFromJid, inviteReason, invitePassword );
		}
		return;
	}

	if ( szMessage ) {
		if (( szMessage = JabberUnixToDosT( szMessage )) == NULL )
			szMessage = mir_tstrdup( _T(""));

		#if defined( _UNICODE )
			char* buf = mir_utf8encodeW( szMessage );
		#else
			char* buf = mir_utf8encode( szMessage );
		#endif

		if ( item != NULL ) {
			if ( resourceStatus ) resourceStatus->bMessageSessionActive = TRUE;
			if ( hContact != NULL )
				JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, PROTOTYPE_CONTACTTYPING_OFF );

			// no we will monitor last resource in all modes
			if ( /*item->resourceMode==RSMODE_LASTSEEN &&*/ ( fromResource = _tcschr( from, '/' ))!=NULL ) {
				fromResource++;
				if ( *fromResource != '\0' ) {
					for ( int i=0; i<item->resourceCount; i++ ) {
						if ( !lstrcmp( item->resource[i].resourceName, fromResource )) {
							if ((item->resourceMode==RSMODE_LASTSEEN) && (i != item->lastSeenResource))
								UpdateMirVer(item);
							item->lastSeenResource = i;
							break;
		}	}	}	}	}

		// Create a temporary contact
		if ( hContact == NULL )
			hContact = CreateTemporaryContact( from, chatItem );

		time_t now = time( NULL );
		if ( !msgTime )
			msgTime = now;

		if ( m_options.FixIncorrectTimestamps && ( msgTime > now || ( msgTime < ( time_t )JabberGetLastContactMessageTime( hContact ))))
			msgTime = now;

		PROTORECVEVENT recv;
		recv.flags = PREF_UTF;
		recv.timestamp = ( DWORD )msgTime;
		recv.szMessage = buf;
		recv.lParam = 0;

		CCSDATA ccs;
		ccs.hContact = hContact;
		ccs.wParam = 0;
		ccs.szProtoService = PSR_MESSAGE;
		ccs.lParam = ( LPARAM )&recv;
		JCallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

		mir_free(( void* )szMessage );
		mir_free( buf );
}	}

// XEP-0115: Entity Capabilities
void CJabberProto::OnProcessPresenceCapabilites( HXML node )
{
	const TCHAR* from;
	if (( from = xmlGetAttrValue( node, _T("from"))) == NULL )
		return;

	Log("presence: for jid " TCHAR_STR_PARAM, from);

	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( from );
	if ( r == NULL ) return;

	HXML n;

	// check XEP-0115 support:
	if (( n = xmlGetChildByTag( node, "c", "xmlns", _T(JABBER_FEAT_ENTITY_CAPS))) != NULL ) {
		const TCHAR *szNode = xmlGetAttrValue( n, _T("node"));
		const TCHAR *szVer = xmlGetAttrValue( n, _T("ver"));
		const TCHAR *szExt = xmlGetAttrValue( n, _T("ext"));
		if ( szNode && szVer ) {
			replaceStr( r->szCapsNode, szNode );
			replaceStr( r->szCapsVer, szVer );
			replaceStr( r->szCapsExt, szExt );
			HANDLE hContact = HContactFromJID( from );
			if ( hContact )
				UpdateMirVer( hContact, r );
		}
	}

	// update user's caps
	JabberCapsBits jcbCaps = GetResourceCapabilites( from, TRUE );
}

void CJabberProto::UpdateJidDbSettings( const TCHAR *jid )
{
	JABBER_LIST_ITEM *item = ListGetItemPtr( LIST_ROSTER, jid );
	if ( !item ) return;
	HANDLE hContact = HContactFromJID( jid );
	if ( !hContact ) return;

	int status = ID_STATUS_OFFLINE;
	if ( !item->resourceCount ) {
		// set offline only if jid has resources
		if ( _tcschr( jid, '/' )==NULL )
			status = item->itemResource.status;
		if ( item->itemResource.statusMessage )
			DBWriteContactSettingTString( hContact, "CList", "StatusMsg", item->itemResource.statusMessage );
		else
			DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
	}

	// Determine status to show for the contact based on the remaining resources
	int nSelectedResource = -1, i = 0;
	int nMaxPriority = -999; // -128...+127 valid range
	for ( i = 0; i < item->resourceCount; i++ )
	{
		if ( item->resource[i].priority > nMaxPriority ) {
			nMaxPriority = item->resource[i].priority;
			status = item->resource[i].status;
			nSelectedResource = i;
		}
		else if ( item->resource[i].priority == nMaxPriority) {
			if (( status = JabberCombineStatus( status, item->resource[i].status )) == item->resource[i].status )
				nSelectedResource = i;
		}
	}
	item->itemResource.status = status;
	if ( nSelectedResource != -1 ) {
		Log("JabberUpdateJidDbSettings: updating jid " TCHAR_STR_PARAM " to rc " TCHAR_STR_PARAM, item->jid, item->resource[nSelectedResource].resourceName );
		if ( item->resource[nSelectedResource].statusMessage )
			DBWriteContactSettingTString( hContact, "CList", "StatusMsg", item->resource[nSelectedResource].statusMessage );
		else
			DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
		UpdateMirVer( hContact, &item->resource[nSelectedResource] );
	}

	if ( _tcschr( jid, '@' )!=NULL || m_options.ShowTransport==TRUE )
		if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != status )
			JSetWord( hContact, "Status", ( WORD )status );

	if (status == ID_STATUS_OFFLINE)
	{ // remove x-status icon
		JDeleteSetting( hContact, DBSETTING_XSTATUSID );
		JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );
		JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );
		//JabberUpdateContactExtraIcon(hContact);
	}

	MenuUpdateSrmmIcon( item );
}

void CJabberProto::OnProcessPresence( HXML node, ThreadData* info )
{
	HANDLE hContact;
	HXML showNode, statusNode, priorityNode;
	JABBER_LIST_ITEM *item;
	LPCTSTR from, show, p;
	TCHAR *nick;

	if ( !node || !xmlGetName( node ) ||_tcscmp( xmlGetName( node ), _T("presence"))) return;
	if (( from = xmlGetAttrValue( node, _T("from"))) == NULL ) return;

	if ( ListExist( LIST_CHATROOM, from )) {
		GroupchatProcessPresence( node, info );
		return;
	}

	BOOL bSelfPresence = FALSE;
	TCHAR szBareFrom[ 512 ];
	JabberStripJid( from, szBareFrom, SIZEOF( szBareFrom ));
	TCHAR szBareOurJid[ 512 ];
	JabberStripJid( info->fullJID, szBareOurJid, SIZEOF( szBareOurJid ));

	if ( !_tcsicmp( szBareFrom, szBareOurJid ))
		bSelfPresence = TRUE;

	LPCTSTR type = xmlGetAttrValue( node, _T("type"));
	if ( type == NULL || !_tcscmp( type, _T("available"))) {
		if (( nick = JabberNickFromJID( from )) == NULL )
			return;

		if (( hContact = HContactFromJID( from )) == NULL ) {
			if ( !_tcsicmp( info->fullJID, from ) || ( !bSelfPresence && !ListExist( LIST_ROSTER, from ))) {
				Log("SKIP Receive presence online from "TCHAR_STR_PARAM" ( who is not in my roster and not in list - skiping)", from );
				mir_free( nick );
				return;
			}
			hContact = DBCreateContact( from, nick, TRUE, TRUE );
		}
		if ( !ListExist( LIST_ROSTER, from )) {
			Log("Receive presence online from "TCHAR_STR_PARAM" ( who is not in my roster )", from );
			ListAdd( LIST_ROSTER, from );
		}
		DBCheckIsTransportedContact( from, hContact );
		int status = ID_STATUS_ONLINE;
		if (( showNode = xmlGetChild( node , "show" )) != NULL ) {
			if (( show = xmlGetText( showNode ) ) != NULL ) {
				if ( !_tcscmp( show, _T("away"))) status = ID_STATUS_AWAY;
				else if ( !_tcscmp( show, _T("xa"))) status = ID_STATUS_NA;
				else if ( !_tcscmp( show, _T("dnd"))) status = ID_STATUS_DND;
				else if ( !_tcscmp( show, _T("chat"))) status = ID_STATUS_FREECHAT;
		}	}

		char priority = 0;
		if (( priorityNode = xmlGetChild( node , "priority" )) != NULL && xmlGetText( priorityNode ) != NULL )
			priority = (char)_ttoi( xmlGetText( priorityNode ) );

		if (( statusNode = xmlGetChild( node , "status" )) != NULL && xmlGetText( statusNode ) != NULL )
			p = xmlGetText( statusNode );
		else
			p = NULL;
		ListAddResource( LIST_ROSTER, from, status, p, priority );

		// XEP-0115: Entity Capabilities
		OnProcessPresenceCapabilites( node );

		UpdateJidDbSettings( from );

		if ( _tcschr( from, '@' )==NULL ) {
			UI_SAFE_NOTIFY(m_pDlgServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH);
		}
		Log( TCHAR_STR_PARAM " ( " TCHAR_STR_PARAM " ) online, set contact status to %s", nick, from, JCallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,(WPARAM)status,0 ));
		mir_free( nick );

		HXML xNode;
		if ( m_options.EnableAvatars ) {
			BOOL hasAvatar = false;
			BOOL removedAvatar = false;

			Log( "Avatar enabled" );
			for ( int i = 1; ( xNode=xmlGetNthChild( node, _T("x"), i )) != NULL; i++ ) {
				if ( !lstrcmp( xmlGetAttrValue( xNode, _T("xmlns")), _T("jabber:x:avatar"))) {
					if (( xNode = xmlGetChild( xNode , "hash" )) != NULL && xmlGetText( xNode ) != NULL ) {
						JDeleteSetting(hContact,"AvatarXVcard");
						Log( "AvatarXVcard deleted" );
						JSetStringT( hContact, "AvatarHash", xmlGetText( xNode ) );
						hasAvatar = true;
						DBVARIANT dbv = {0};
						int result = JGetStringT( hContact, "AvatarSaved", &dbv );
						if ( !result || lstrcmp( dbv.ptszVal, xmlGetText( xNode ) )) {
							Log( "Avatar was changed" );
							JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
						} else Log( "Not broadcasting avatar changed" );
						if ( !result ) JFreeVariant( &dbv );
					} else {
						removedAvatar = true;
					}
			}	}
			if ( !hasAvatar ) { //no jabber:x:avatar. try vcard-temp:x:update
				Log( "Not hasXAvatar" );
				for ( int i = 1; ( xNode=xmlGetNthChild( node, _T("x"), i )) != NULL; i++ ) {
					if ( !lstrcmp( xmlGetAttrValue( xNode, _T("xmlns")), _T("vcard-temp:x:update"))) {
						if (( xNode = xmlGetChild( xNode , "photo" )) != NULL ){
							LPCTSTR txt = xmlGetText( xNode );
							if ( txt != NULL && txt[0] != 0) {
								JSetByte( hContact, "AvatarXVcard", 1 );
								Log( "AvatarXVcard set" );
								JSetStringT( hContact, "AvatarHash", txt );
								hasAvatar = true;
								DBVARIANT dbv = {0};
								int result = JGetStringT( hContact, "AvatarSaved", &dbv );
								if ( !result || lstrcmp( dbv.ptszVal, xmlGetText( xNode ) )) {
									Log( "Avatar was changed. Using vcard-temp:x:update" );
									JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
								}
								else Log( "Not broadcasting avatar changed" );
								if ( !result ) JFreeVariant( &dbv );
							} else {
								removedAvatar = true;
							}
			}	}	}	}
			if ( !hasAvatar && removedAvatar ) {
				Log( "Has no avatar" );
				JDeleteSetting( hContact, "AvatarHash" );
				DBVARIANT dbv = {0};
				if ( !JGetStringT( hContact, "AvatarSaved", &dbv ) ) {
					JFreeVariant( &dbv );
					JDeleteSetting( hContact, "AvatarSaved" );
					JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, NULL, NULL );
		}	}	}
		return;
	}

	if ( !_tcscmp( type, _T("unavailable"))) {
		int status = ID_STATUS_OFFLINE;
		hContact = HContactFromJID( from );
		if (( item = ListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
			ListRemoveResource( LIST_ROSTER, from );

			hContact = HContactFromJID( from );
			if ( hContact && DBGetContactSettingByte( hContact, "CList", "NotOnList", 0) == 1 ) {
				// remove selfcontact, if where is no more another resources
				if ( item->resourceCount == 1 && ResourceInfoFromJID( info->fullJID ))
					ListRemoveResource( LIST_ROSTER, info->fullJID );
			}


			// set status only if no more available resources
			if ( !item->resourceCount )
			{
				item->itemResource.status = ID_STATUS_OFFLINE;
				if ((( statusNode = xmlGetChild( node , "status" )) != NULL ) && xmlGetText( statusNode ) )
					replaceStr( item->itemResource.statusMessage, xmlGetText( statusNode ) );
				else
					replaceStr( item->itemResource.statusMessage, NULL );
			}
		}
		else Log( "SKIP Receive presence offline from " TCHAR_STR_PARAM " ( who is not in my roster )", from );

		UpdateJidDbSettings( from );

		if ( _tcschr( from, '@' )==NULL ) {
			UI_SAFE_NOTIFY(m_pDlgServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH);
		}
		DBCheckIsTransportedContact(from, hContact);
		return;
	}

	if ( !_tcscmp( type, _T("subscribe"))) {
		if (hContact = HContactFromJID( from ))
			AddDbPresenceEvent( hContact, JABBER_DB_EVENT_PRESENCE_SUBSCRIBE );

		// automatically send authorization allowed to agent/transport
		if ( _tcschr( from, '@' ) == NULL || m_options.AutoAcceptAuthorization ) {
			ListAdd( LIST_ROSTER, from );
			info->send( XmlNode( _T("presence")) << XATTR( _T("to"), from ) << XATTR( _T("type"), _T("subscribed")));

			if ( m_options.AutoAdd == TRUE ) {
				if (( item = ListGetItemPtr( LIST_ROSTER, from )) == NULL || ( item->subscription != SUB_BOTH && item->subscription != SUB_TO )) {
					Log( "Try adding contact automatically jid = " TCHAR_STR_PARAM, from );
					if (( hContact=AddToListByJID( from, 0 )) != NULL ) {
						// Trigger actual add by removing the "NotOnList" added by AddToListByJID()
						// See AddToListByJID() and JabberDbSettingChanged().
						DBDeleteContactSetting( hContact, "CList", "NotOnList" );
			}	}	}
			RebuildInfoFrame();
		}
		else {
			HXML n = xmlGetChild( node , "nick" );
			nick = ( n == NULL ) ? JabberNickFromJID( from ) : mir_tstrdup( xmlGetText( n ) );
			if ( nick != NULL ) {
				Log( TCHAR_STR_PARAM " ( " TCHAR_STR_PARAM " ) requests authorization", nick, from );
				DBAddAuthRequest( from, nick );
				mir_free( nick );
		}	}
		return;
	}

	if ( !_tcscmp( type, _T("unsubscribe"))) {
		if (hContact = HContactFromJID( from ))
			AddDbPresenceEvent( hContact, JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBE );
	}

	if ( !_tcscmp( type, _T("unsubscribed"))) {
		if (hContact = HContactFromJID( from ))
			AddDbPresenceEvent( hContact, JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBED );
	}

	if ( !_tcscmp( type, _T("error"))) {
		if (hContact = HContactFromJID( from )) {
			AddDbPresenceEvent( hContact, JABBER_DB_EVENT_PRESENCE_ERROR );
		}
	}

	if ( !_tcscmp( type, _T("subscribed"))) {

		if (hContact = HContactFromJID( from ))
			AddDbPresenceEvent( hContact, JABBER_DB_EVENT_PRESENCE_SUBSCRIBED );

		if (( item=ListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
			if ( item->subscription == SUB_FROM ) item->subscription = SUB_BOTH;
			else if ( item->subscription == SUB_NONE ) {
				item->subscription = SUB_TO;
				if ( _tcschr( from, '@' )==NULL ) {
					UI_SAFE_NOTIFY(m_pDlgServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH);
				}
			}
		}
	}	
}

void CJabberProto::OnIqResultVersion( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( pInfo->GetFrom() );
	if ( r == NULL ) return;

	r->dwVersionRequestTime = -1;

	replaceStr( r->software, NULL );
	replaceStr( r->version, NULL );
	replaceStr( r->system, NULL );

	HXML queryNode = pInfo->GetChildNode();

	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT && queryNode) {
		HXML n;
		if (( n = xmlGetChild( queryNode , "name" ))!=NULL && xmlGetText( n ) )
			r->software = mir_tstrdup( xmlGetText( n ) );
		if (( n = xmlGetChild( queryNode , "version" ))!=NULL && xmlGetText( n ) )
			r->version = mir_tstrdup( xmlGetText( n ) );
		if (( n = xmlGetChild( queryNode , "os" ))!=NULL && xmlGetText( n ) )
			r->system = mir_tstrdup( xmlGetText( n ) );
	}

	GetResourceCapabilites( pInfo->GetFrom(), TRUE );
	if ( pInfo->GetHContact() )
		UpdateMirVer( pInfo->GetHContact(), r );

	JabberUserInfoUpdate(pInfo->GetHContact());
}

void CJabberProto::OnProcessIq( HXML node, void *userdata )
{
	ThreadData* info;
	HXML queryNode;
	const TCHAR *type, *xmlns;
	int i;
	JABBER_IQ_PFUNC pfunc;

	if ( !xmlGetName( node ) || _tcscmp( xmlGetName( node ), _T("iq"))) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type=xmlGetAttrValue( node, _T("type"))) == NULL ) return;

	int id = JabberGetPacketID( node );
	const TCHAR* idStr = xmlGetAttrValue( node, _T("id"));

	queryNode = xmlGetChild( node , "query" );
	xmlns = xmlGetAttrValue( queryNode, _T("xmlns"));

	// new match by id
	if ( m_iqManager.HandleIq( id, node, userdata ))
		return;

	// new iq handler engine
	if ( m_iqManager.HandleIqPermanent( node, userdata ))
		return;

	/////////////////////////////////////////////////////////////////////////
	// OLD MATCH BY ID
	/////////////////////////////////////////////////////////////////////////
	if ( ( !_tcscmp( type, _T("result")) || !_tcscmp( type, _T("error")) ) && (( pfunc=JabberIqFetchFunc( id )) != NULL )) {
		Log( "Handling iq request for id=%d", id );
		(this->*pfunc)( node, userdata );
		return;
	}
	// RECVED: <iq type='error'> ...
	else if ( !_tcscmp( type, _T("error"))) {
		Log( "XXX on entry" );
		// Check for file transfer deny by comparing idStr with ft->iqId
		i = 0;
		while (( i=ListFindNext( LIST_FILE, i )) >= 0 ) {
			JABBER_LIST_ITEM *item = ListGetItemPtrFromIndex( i );
			if ( item->ft != NULL && item->ft->state == FT_CONNECTING && !_tcscmp( idStr, item->ft->iqId )) {
				Log( "Denying file sending request" );
				item->ft->state = FT_DENIED;
				if ( item->ft->hFileEvent != NULL )
					SetEvent( item->ft->hFileEvent );	// Simulate the termination of file server connection
			}
			i++;
	}	}
	else if (( !_tcscmp( type, _T("get")) || !_tcscmp( type, _T("set") ))) {
		XmlNodeIq iq( _T("error"), idStr, xmlGetAttrValue( node, _T("from")) );

		HXML pFirstChild = xmlGetChild( node , 0 );
		if ( pFirstChild )
			xmlAddChild( iq, pFirstChild );
		
		iq << XCHILD( _T("error")) << XATTR( _T("type"), _T("cancel"))
				<< XCHILDNS( _T("service-unavailable"), _T("urn:ietf:params:xml:ns:xmpp-stanzas"));
		info->send( iq );
	}
}

void CJabberProto::OnProcessRegIq( HXML node, void *userdata )
{
	ThreadData* info;
	HXML errorNode;
	const TCHAR *type;

	if ( !xmlGetName( node ) || _tcscmp( xmlGetName( node ), _T("iq"))) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type=xmlGetAttrValue( node, _T("type"))) == NULL ) return;

	int id = JabberGetPacketID( node );

	if ( !_tcscmp( type, _T("result"))) {

		// RECVED: result of the request for registration mechanism
		// ACTION: send account registration information
		if ( id == iqIdRegGetReg ) {
			iqIdRegSetReg = SerialNext();

			XmlNodeIq iq( _T("set"), iqIdRegSetReg );
			HXML query = iq << XQUERY( _T(JABBER_FEAT_REGISTER));
			query << XCHILD( _T("password"), _A2T(info->password));
			query << XCHILD( _T("username"), info->username );
			info->send( iq );

			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 75, ( LPARAM )TranslateT( "Sending registration information..." ));
		}
		// RECVED: result of the registration process
		// ACTION: account registration successful
		else if ( id == iqIdRegSetReg ) {
			info->send( "</stream:stream>" );
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Registration successful" ));
			info->reg_done = TRUE;
	}	}

	else if ( !_tcscmp( type, _T("error"))) {
		errorNode = xmlGetChild( node , "error" );
		TCHAR* str = JabberErrorMsg( errorNode );
		SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )str );
		mir_free( str );
		info->reg_done = TRUE;
		info->send( "</stream:stream>" );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// ThreadData constructor & destructor

ThreadData::ThreadData( CJabberProto* aproto, JABBER_SESSION_TYPE parType )
{
	memset( this, 0, sizeof( *this ));
	type = parType;
	proto = aproto;
	InitializeCriticalSection( &iomutex );
}

ThreadData::~ThreadData()
{
	delete auth;
	mir_free( zRecvData );
	DeleteCriticalSection( &iomutex );
}

void ThreadData::close()
{
	if ( s ) {
		Netlib_CloseHandle(s);
		s = NULL;
}	}

int ThreadData::recvws( char* buf, size_t len, int flags )
{
	if ( this == NULL )
		return 0;

	return proto->WsRecv( s, buf, len, flags );
}

int ThreadData::recv( char* buf, size_t len )
{
	if ( useZlib )
		return zlibRecv( buf, len );

	return recvws( buf, len, MSG_DUMPASTEXT );
}

int ThreadData::sendws( char* buffer, size_t bufsize, int flags )
{
	return proto->WsSend( s, buffer, bufsize, flags );
}

int ThreadData::send( char* buffer, int bufsize )
{
	if ( this == NULL )
		return 0;

	int result;

	EnterCriticalSection( &iomutex );

	if ( useZlib )
		result = zlibSend( buffer, bufsize );
	else
		result = sendws( buffer, bufsize, MSG_DUMPASTEXT );

	LeaveCriticalSection( &iomutex );
	return result;
}

// Caution: DO NOT use ->send() to send binary ( non-string ) data
int ThreadData::send( HXML node )
{
	if ( this == NULL )
		return 0;

	while ( HXML parent = xi.getParent( node ))
		node = parent;

	proto->OnConsoleProcessXml(node, JCPF_OUT);

	TCHAR* str = xi.toString( node, NULL );
	char* utfStr = mir_utf8encodeT( str );
	int result = send( utfStr, strlen( utfStr ));
	mir_free( utfStr );
	xi.freeMem( str );
	return result;
}

int ThreadData::send( const char* fmt, ... )
{
	if ( this == NULL )
		return 0;

	va_list vararg;
	va_start( vararg, fmt );
	int size = 512;
	char* str = ( char* )mir_alloc( size );
	while ( _vsnprintf( str, size, fmt, vararg ) == -1 ) {
		size += 512;
		str = ( char* )mir_realloc( str, size );
	}
	va_end( vararg );

	int result = send( str, strlen( str ));

	mir_free( str );
	return result;
}
