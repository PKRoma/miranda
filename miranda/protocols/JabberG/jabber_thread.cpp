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

#include "jabber_ssl.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_secur.h"
#include "resource.h"
#include "jabber_caps.h"
#include "jabber_privacy.h"
#include "jabber_rc.h"

// <iq/> identification number for various actions
// for JABBER_REGISTER thread
int iqIdRegGetReg;
int iqIdRegSetReg;

static void JabberProcessStreamOpening( XmlNode *node, void *userdata );
static void JabberProcessStreamClosing( XmlNode *node, void *userdata );
static void JabberProcessProtocol( XmlNode *node, void *userdata );

// XML Console
#define JCPF_IN      0x01UL
#define JCPF_OUT     0x02UL
#define JCPF_ERROR   0x04UL
#define JCPF_UTF8    0x08UL

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
	GCDEST gcd = { ppro->m_szProtoName, NULL, GC_EVENT_CONTROL };
	GCEVENT gce = { 0 };
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	CallService( MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Jabber keep-alive thread

static void __cdecl JabberKeepAliveThread( CJabberProto* ppro )
{
	NETLIBSELECT nls = {0};
	nls.cbSize = sizeof( NETLIBSELECT );
	nls.dwTimeout = 60000;	// 60000 millisecond ( 1 minute )
	nls.hExceptConns[0] = ppro->jabberThreadInfo->s;
	for ( ;; ) {
		if ( JCallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls ) != 0 )
			break;
		if ( ppro->jabberSendKeepAlive )
			ppro->jabberThreadInfo->send( " \t " );
	}
	ppro->JabberLog( "Exiting KeepAliveThread" );
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

static XmlState xmlState;
static char *xmlStreamToBeInitialized = 0;

void CJabberProto::xmlStreamInitialize(char *which)
{
	JabberLog("Stream will be initialized %s",which);
	xmlStreamToBeInitialized = strdup(which);
}

void CJabberProto::xmlStreamInitializeNow(ThreadData* info)
{
	JabberLog("Stream is initializing %s",xmlStreamToBeInitialized?xmlStreamToBeInitialized:"after connect");
	if (xmlStreamToBeInitialized) {
		free(xmlStreamToBeInitialized);
		xmlStreamToBeInitialized = NULL;
		JabberXmlDestroyState(&xmlState);
	}
	JabberXmlInitState( &xmlState );
	JabberXmlSetCallback( &xmlState, 1, ELEM_OPEN, JabberProcessStreamOpening, info );
	JabberXmlSetCallback( &xmlState, 1, ELEM_CLOSE, JabberProcessStreamClosing, info );
	JabberXmlSetCallback( &xmlState, 2, ELEM_CLOSE, JabberProcessProtocol, info );
	{
		XmlNode stream( "stream:stream" );
		stream.props = "<?xml version='1.0' encoding='UTF-8'?>";
		stream.addAttr( "to", info->server );
		stream.addAttr( "xmlns", "jabber:client" );
		stream.addAttr( "xmlns:stream", "http://etherx.jabber.org/streams" );
		TCHAR *szXmlLang = JabberGetXmlLang();
		if ( szXmlLang ) {
			stream.addAttr( "xml:lang", szXmlLang );
			mir_free( szXmlLang );
		}
		if ( !JGetByte( "Disable3920auth", 0 ))
			stream.addAttr( "version", "1.0" );
		stream.dirtyHack = true; // this is to keep the node open - do not send </stream:stream>
		info->send( stream );
}	}

void __cdecl JabberServerThread( ThreadData* info )
{
	info->proto->ServerThread( info );
}

void CJabberProto::ServerThread( ThreadData* info )
{
	DBVARIANT dbv;
	char* buffer;
	int datalen;
	int oldStatus;
	PVOID ssl;

	JabberLog( "Thread started: type=%d", info->type );

	info->resolveID = -1;
	info->auth = NULL;
	if ( info->type == JABBER_SESSION_NORMAL ) {

		// Normal server connection, we will fetch all connection parameters
		// e.g. username, password, etc. from the database.

		if ( jabberThreadInfo != NULL ) {
			// Will not start another connection thread if a thread is already running.
			// Make APC call to the main thread. This will immediately wake the thread up
			// in case it is asleep in the reconnect loop so that it will immediately
			// reconnect.
			QueueUserAPC( JabberDummyApcFunc, jabberThreadInfo->hThread, 0 );
			JabberLog( "Thread ended, another normal thread is running" );
LBL_Exit:
			delete info;
			return;
		}

		jabberThreadInfo = info;
		if ( streamId ) mir_free( streamId );
		streamId = NULL;

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
			JabberLog( "Thread ended, login name is not configured" );
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
LBL_FatalError:
			jabberThreadInfo = NULL;
			oldStatus = m_iStatus;
			m_iStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
         goto LBL_Exit;
		}

		if ( !DBGetContactSettingString( NULL, m_szProtoName, "LoginServer", &dbv )) {
			strncpy( info->server, dbv.pszVal, SIZEOF( info->server )-1 );
			JFreeVariant( &dbv );
		}
		else {
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
			JabberLog( "Thread ended, login server is not configured" );
			goto LBL_FatalError;
		}

		if ( JGetByte( "HostNameAsResource", FALSE ) == FALSE )
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

		if ( JGetByte( "SavePassword", TRUE ) == FALSE ) {
			mir_sntprintf( jidStr, SIZEOF( jidStr ), _T("%s@") _T(TCHAR_STR_PARAM), info->username, info->server );

			// Ugly hack: continue logging on only the return value is &( onlinePassword[0] )
			// because if WM_QUIT while dialog box is still visible, p is returned with some
			// exit code which may not be NULL.
			// Should be better with modeless.
			onlinePassword[0] = ( char )-1;
			hEventPasswdDlg = CreateEvent( NULL, FALSE, FALSE, NULL );
			QueueUserAPC( JabberPasswordCreateDialogApcProc, hMainThread, ( DWORD )jidStr );
			WaitForSingleObject( hEventPasswdDlg, INFINITE );
			CloseHandle( hEventPasswdDlg );

			if ( onlinePassword[0] == ( TCHAR ) -1 ) {
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
				JabberLog( "Thread ended, password request dialog was canceled" );
				goto LBL_FatalError;
			}
			strncpy( info->password, onlinePassword, SIZEOF( info->password ));
			info->password[ SIZEOF( info->password )-1] = '\0';
		}
		else {
			if ( DBGetContactSettingString( NULL, m_szProtoName, "Password", &dbv )) {
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
				JabberLog( "Thread ended, password is not configured" );
				goto LBL_FatalError;
			}
			JCallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
			strncpy( info->password, dbv.pszVal, SIZEOF( info->password ));
			info->password[SIZEOF( info->password )-1] = '\0';
			JFreeVariant( &dbv );
		}

		if ( JGetByte( "ManualConnect", FALSE ) == TRUE ) {
			if ( !DBGetContactSettingString( NULL, m_szProtoName, "ManualHost", &dbv )) {
				strncpy( info->manualHost, dbv.pszVal, SIZEOF( info->manualHost ));
				info->manualHost[sizeof( info->manualHost )-1] = '\0';
				JFreeVariant( &dbv );
			}
			info->port = JGetWord( NULL, "ManualPort", JABBER_DEFAULT_PORT );
		}
		else info->port = JGetWord( NULL, "Port", JABBER_DEFAULT_PORT );

		info->useSSL = JGetByte( "UseSSL", FALSE );
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
		JabberLog( "Thread ended, invalid session type" );
		goto LBL_FatalError;
	}

	char connectHost[128];
	if ( info->manualHost[0] == 0 ) {
		int port_temp;
		strncpy( connectHost, info->server, SIZEOF(info->server));
		if ( port_temp = xmpp_client_query( connectHost )) { // port_temp will be > 0 if resolution is successful
			JabberLog("%s%s resolved to %s:%d","_xmpp-client._tcp.",info->server,connectHost,port_temp);
			if (info->port==0 || info->port==5222)
				info->port = port_temp;
		}
		else JabberLog("%s%s not resolved", "_xmpp-client._tcp.", connectHost);
	}
	else strncpy( connectHost, info->manualHost, SIZEOF(connectHost)); // do not resolve if manual host is selected

	JabberLog( "Thread type=%d server='%s' port='%d'", info->type, connectHost, info->port );

	int jabberNetworkBufferSize = 2048;
	if (( buffer=( char* )mir_alloc( jabberNetworkBufferSize+1 )) == NULL ) {	// +1 is for '\0' when debug logging this buffer
		JabberLog( "Cannot allocate network buffer, thread ended" );
		if ( info->type == JABBER_SESSION_NORMAL ) {
			oldStatus = m_iStatus;
			m_iStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
			jabberThreadInfo = NULL;
		}
		else if ( info->type == JABBER_SESSION_REGISTER ) {
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Not enough memory" ));
		}
		JabberLog( "Thread ended, network buffer cannot be allocated" );
		goto LBL_FatalError;
	}

	info->s = JabberWsConnect( connectHost, info->port );
	if ( info->s == NULL ) {
		JabberLog( "Connection failed ( %d )", WSAGetLastError());
		if ( info->type == JABBER_SESSION_NORMAL ) {
			if ( jabberThreadInfo == info ) {
				oldStatus = m_iStatus;
				m_iStatus = ID_STATUS_OFFLINE;
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
				JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
				jabberThreadInfo = NULL;
		}	}
		else if ( info->type == JABBER_SESSION_REGISTER )
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Cannot connect to the server" ));

		JabberLog( "Thread ended, connection failed" );
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
		jabberLocalIP = saddr.sin_addr.S_un.S_addr;
		JabberLog( "Local IP = %s", inet_ntoa( saddr.sin_addr ));
	}

	if ( info->useSSL ) {
		JabberLog( "Intializing SSL connection" );
		if ( JabberSslInit() && socket != INVALID_SOCKET ) {
			JabberLog( "SSL using socket = %d", socket );
			if (( ssl=pfn_SSL_new( jabberSslCtx )) != NULL ) {
				JabberLog( "SSL create context ok" );
				if ( pfn_SSL_set_fd( ssl, socket ) > 0 ) {
					JabberLog( "SSL set fd ok" );
					if ( pfn_SSL_connect( ssl ) > 0 ) {
						JabberLog( "SSL negotiation ok" );
						info->ssl = ssl;	// This make all communication on this handle use SSL
						JabberLog( "SSL enabled for handle = %d", info->s );
					}
					else {
						JabberLog( "SSL negotiation failed" );
						pfn_SSL_free( ssl );
				}	}
				else {
					JabberLog( "SSL set fd failed" );
					pfn_SSL_free( ssl );
		}	}	}

		if ( !info->ssl ) {
			if ( info->type == JABBER_SESSION_NORMAL ) {
				oldStatus = m_iStatus;
				m_iStatus = ID_STATUS_OFFLINE;
				JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
				if ( jabberThreadInfo == info )
					jabberThreadInfo = NULL;
			}
			else if ( info->type == JABBER_SESSION_REGISTER ) {
				SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Cannot connect to the server" ));
			}
			mir_free( buffer );
			if ( !hLibSSL )
				MessageBox( NULL, TranslateT( "The connection requires an OpenSSL library, which is not installed." ), TranslateT( "Jabber Connection Error" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
			JabberLog( "Thread ended, SSL connection failed" );
			goto LBL_FatalError;
	}	}

	// User may change status to OFFLINE while we are connecting above
	if ( m_iDesiredStatus != ID_STATUS_OFFLINE || info->type == JABBER_SESSION_REGISTER ) {

		if ( info->type == JABBER_SESSION_NORMAL ) {
			jabberConnected = TRUE;
			int len = _tcslen( info->username ) + strlen( info->server )+1;
			jabberJID = ( TCHAR* )mir_alloc( sizeof( TCHAR)*( len+1 ));
			mir_sntprintf( jabberJID, len+1, _T("%s@") _T(TCHAR_STR_PARAM), info->username, info->server );
			if ( JGetByte( "KeepAlive", 1 ))
				jabberSendKeepAlive = TRUE;
			else
				jabberSendKeepAlive = FALSE;
			mir_forkthread(( pThreadFunc )JabberKeepAliveThread, this );
		}

		xmlStreamInitializeNow( info );

		JabberLog( "Entering main recv loop" );
		datalen = 0;

		for ( ;; ) {
			int recvResult = info->recv( buffer+datalen, jabberNetworkBufferSize-datalen), bytesParsed;
			JabberLog( "recvResult = %d", recvResult );
			if ( recvResult <= 0 )
				break;
			datalen += recvResult;

			buffer[datalen] = '\0';
			if ( info->ssl && DBGetContactSettingByte( NULL, "Netlib", "DumpRecv", TRUE ) == TRUE ) {
				// Emulate netlib log feature for SSL connection
				char* szLogBuffer = ( char* )mir_alloc( recvResult+128 );
				if ( szLogBuffer != NULL ) {
					strcpy( szLogBuffer, "( SSL ) Data received\n" );
					memcpy( szLogBuffer+strlen( szLogBuffer ), buffer+datalen-recvResult, recvResult+1 /* also copy \0 */ );
					Netlib_Logf( hNetlibUser, "%s", szLogBuffer );	// %s to protect against when fmt tokens are in szLogBuffer causing crash
					mir_free( szLogBuffer );
			}	}

			bytesParsed = JabberXmlParse( &xmlState, buffer );
			JabberLog( "bytesParsed = %d", bytesParsed );
			if ( bytesParsed > 0 ) {
				if ( bytesParsed < datalen )
					memmove( buffer, buffer+bytesParsed, datalen-bytesParsed );
				datalen -= bytesParsed;
			}
			else if ( datalen == jabberNetworkBufferSize ) {
				//jabberNetworkBufferSize += 65536;
				jabberNetworkBufferSize *= 2;
				JabberLog( "Increasing network buffer size to %d", jabberNetworkBufferSize );
				if (( buffer=( char* )mir_realloc( buffer, jabberNetworkBufferSize+1 )) == NULL ) {
					JabberLog( "Cannot reallocate more network buffer, go offline now" );
					break;
			}	}
			else JabberLog( "Unknown state: bytesParsed=%d, datalen=%d, jabberNetworkBufferSize=%d", bytesParsed, datalen, jabberNetworkBufferSize );

			if (xmlStreamToBeInitialized) xmlStreamInitializeNow(info);
		}

		JabberXmlDestroyState(&xmlState);

		if ( info->type == JABBER_SESSION_NORMAL ) {
			m_iqManager.ExpireAll( info );
			jabberOnline = FALSE;
			jabberConnected = FALSE;
			info->zlibUninit();
			JabberEnableMenuItems( FALSE );
			JabberRebuildStatusMenu();
			if ( hwndJabberChangePassword ) {
				//DestroyWindow( hwndJabberChangePassword );
				// Since this is a different thread, simulate the click on the cancel button instead
				SendMessage( hwndJabberChangePassword, WM_COMMAND, MAKEWORD( IDCANCEL, 0 ), 0 );
			}

			if ( jabberChatDllPresent )
				QueueUserAPC(( PAPCFUNC )JabberOfflineChatWindows, hMainThread, ( LPARAM )this );

			JabberListRemoveList( LIST_CHATROOM );
			JabberListRemoveList( LIST_BOOKMARK );
			if ( hwndJabberAgents )
				SendMessage( hwndJabberAgents, WM_JABBER_CHECK_ONLINE, 0, 0 );
			if ( hwndJabberGroupchat )
				SendMessage( hwndJabberGroupchat, WM_JABBER_CHECK_ONLINE, 0, 0 );
			if ( hwndJabberJoinGroupchat )
				SendMessage( hwndJabberJoinGroupchat, WM_JABBER_CHECK_ONLINE, 0, 0 );
			if ( hwndJabberBookmarks )
				SendMessage( hwndJabberBookmarks, WM_JABBER_CHECK_ONLINE, 0, 0 );
			if ( hwndJabberAddBookmark)
				SendMessage( hwndJabberAddBookmark, WM_JABBER_CHECK_ONLINE, 0, 0 );

			// Set status to offline
			oldStatus = m_iStatus;
			m_iStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );

			// Set all contacts to offline
			HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			while ( hContact != NULL ) {
				if ( !lstrcmpA(( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 ), m_szProtoName ))
				{
					JabberSetContactOfflineStatus( hContact );
					JabberMenuHideSrmmIcon( hContact );
				}

				hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
			}

			mir_free( jabberJID );
			jabberJID = NULL;
			jabberLoggedInTime = 0;
			JabberListWipe();
			if ( hwndJabberAgents ) {
				SendMessage( hwndJabberAgents, WM_JABBER_AGENT_REFRESH, 0, ( LPARAM )"" );
				SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
			}
			if ( hwndJabberVcard )
				SendMessage( hwndJabberVcard, WM_JABBER_CHECK_ONLINE, 0, 0 );
		}
		else if ( info->type==JABBER_SESSION_REGISTER && !info->reg_done )
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Connection lost" ));
	}
	else if ( info->type == JABBER_SESSION_NORMAL ) {
		oldStatus = m_iStatus;
		m_iStatus = ID_STATUS_OFFLINE;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
	}

	JabberLog( "Thread ended: type=%d server='%s'", info->type, info->server );

	if ( info->type==JABBER_SESSION_NORMAL && jabberThreadInfo==info ) {
		if ( streamId ) mir_free( streamId );
		streamId = NULL;
		jabberThreadInfo = NULL;
	}

	mir_free( buffer );
	JabberLog( "Exiting ServerThread" );
	goto LBL_Exit;
}

void CJabberProto::JabberPerformRegistration( ThreadData* info )
{
	iqIdRegGetReg = JabberSerialNext();
	XmlNodeIq iq("get",iqIdRegGetReg,(char*)NULL);
	XmlNode* query = iq.addQuery(JABBER_FEAT_REGISTER);
	info->send( iq );
	SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 50, ( LPARAM )TranslateT( "Requesting registration instruction..." ));
}

void CJabberProto::JabberPerformIqAuth( ThreadData* info )
{
	if ( info->type == JABBER_SESSION_NORMAL ) {
		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, &CJabberProto::JabberIqResultGetAuth );
		XmlNodeIq iq( "get", iqId );
		XmlNode* query = iq.addQuery( "jabber:iq:auth" );
		query->addChild( "username", info->username );
		info->send( iq );
	}
	else if ( info->type == JABBER_SESSION_REGISTER )
		JabberPerformRegistration( info );
}

static void JabberProcessStreamOpening( XmlNode *node, void *userdata )
{
	if ( node->name == NULL || strcmp( node->name, "stream:stream" ))
		return;

	ThreadData* info = ( ThreadData* ) userdata;
	if ( info->type == JABBER_SESSION_NORMAL ) {
		TCHAR* sid = JabberXmlGetAttrValue( node, "id" );
		if ( sid != NULL ) {
			char* pszSid = mir_t2a( sid );
			replaceStr( info->proto->streamId, pszSid );
			mir_free( pszSid );
	}	}

	// old server - disable SASL then
	if ( JabberXmlGetAttrValue( node, "version" ) == NULL )
		info->proto->JSetByte( "Disable3920auth", TRUE );

	if ( info->proto->JGetByte( "Disable3920auth", 0 ))
		info->proto->JabberPerformIqAuth( info );
}

static void JabberProcessStreamClosing( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;

	Netlib_CloseHandle( info->s );
	if ( node->name && !strcmp( node->name, "stream:error" ) && node->text )
		MessageBox( NULL, TranslateTS( node->text ), TranslateT( "Jabber Connection Error" ), MB_OK|MB_ICONERROR|MB_SETFOREGROUND );
}

/////////////////////////////////////////////////////////////////////////////////////////

void CJabberProto::JabberProcessFeatures( XmlNode *node, void *userdata )
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

	for ( int i=0; i < node->numChild; i++ ) {
		XmlNode* n = node->child[i];
		if ( !strcmp( n->name, "starttls" )) {
			if ( !info->useSSL && JGetByte( "UseTLS", FALSE )) {
				JabberLog( "Requesting TLS" );
				XmlNode stls( n->name ); stls.addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-tls" );
				info->send( stls );
				return;
		}	}

		if ( !strcmp( n->name, "compression" ) && ( JGetByte( "EnableZlib", FALSE ) == TRUE)) {
			JabberLog("Server compression available");
			for ( int k=0; k < n->numChild; k++ ) {
				XmlNode* c = n->child[k];
				if ( !strcmp( c->name, "method" )) {
					if ( !_tcscmp( c->text, _T("zlib")) && info->zlibInit() == TRUE ) {
						JabberLog("Requesting Zlib compression");
						XmlNode szlib( "compress" ); szlib.addAttr( "xmlns", "http://jabber.org/protocol/compress" );
						szlib.addChild( "method", "zlib" );
						info->send( szlib );
						return;
		}	}	}	}

		if ( !strcmp( n->name, "mechanisms" )) {
			areMechanismsDefined = true;
			//JabberLog("%d mechanisms\n",n->numChild);
			for ( int k=0; k < n->numChild; k++ ) {
				XmlNode* c = n->child[k];
				if ( !strcmp( c->name, "mechanism" ))
					//JabberLog("Mechanism: %s",c->text);
					     if ( !_tcscmp( c->text, _T("PLAIN")))          isPlainAvailable = true;
					else if ( !_tcscmp( c->text, _T("DIGEST-MD5")))     isMd5available = true;
					else if ( !_tcscmp( c->text, _T("NTLM")))           isNtlmAvailable = true;
					else if ( !_tcscmp( c->text, _T("X-GOOGLE-TOKEN"))) isXGoogleTokenAvailable = true;
		}	}
		else if ( !strcmp( n->name, "register" )) isRegisterAvailable = true;
		else if ( !strcmp( n->name, "auth"     )) isAuthAvailable = true;
		else if ( !strcmp( n->name, "session"  )) isSessionAvailable = true;
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
				JabberPerformIqAuth( info );
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
			XmlNode n( "auth", request );
			n.addAttr( "xmlns", _T("urn:ietf:params:xml:ns:xmpp-sasl"));
			n.addAttr( "mechanism", auth->getName() );
			info->send( n );
			mir_free( request );
		}
		else if ( info->type == JABBER_SESSION_REGISTER )
			JabberPerformRegistration( info );
		else
			info->send( "</stream:stream>" );
		return;
	}

	// mechanisms are not defined.
	if ( info->auth ) { //We are already logged-in
		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, &CJabberProto::JabberIqResultBind );
		XmlNodeIq iq("set",iqId);
		XmlNode* bind = iq.addChild( "bind" ); bind->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-bind" );
		bind->addChild( "resource", info->resource );
		info->send( iq );

		if ( isSessionAvailable )
			info->bIsSessionAvailable = TRUE;

		return;
	}

	//mechanisms not available and we are not logged in
	JabberPerformIqAuth( info );
}

void CJabberProto::JabberProcessFailure( XmlNode *node, void *userdata )
{
//	JabberXmlDumpNode( node );
	ThreadData* info = ( ThreadData* ) userdata;
	TCHAR* type;
//failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"
	if (( type=JabberXmlGetAttrValue( node, "xmlns" )) == NULL ) return;
	if ( !_tcscmp( type, _T("urn:ietf:params:xml:ns:xmpp-sasl") )) {
		info->send( "</stream:stream>" );

		TCHAR text[128];
		mir_sntprintf( text, SIZEOF( text ), _T("%s %s@")_T(TCHAR_STR_PARAM)_T("."), TranslateT( "Authentication failed for" ), info->username, info->server );
		MessageBox( NULL, text, TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
		JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
		jabberThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::JabberProcessError( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	TCHAR *buff;
	int i;
	int pos;
//failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"
	if ( !node->numChild ) return;
	buff = (TCHAR *)mir_alloc(1024*sizeof(TCHAR));
	pos=0;
	for (i=0;i<node->numChild;i++) {
		pos += mir_sntprintf(buff+pos,1024-pos,
			_T(TCHAR_STR_PARAM)_T(": %s\n"),
			node->child[i]->name,node->child[i]->text);
		if (!strcmp(node->child[i]->name,"conflict"))
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
	}
	MessageBox( NULL, buff, TranslateT( "Jabber Error" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
	mir_free(buff);
	info->send( "</stream:stream>" );
}

void CJabberProto::JabberProcessSuccess( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* )userdata;
	TCHAR* type;
//	int iqId;
	// RECVED: <success ...
	// ACTION: if successfully logged in, continue by requesting roster list and set my initial status
	if (( type=JabberXmlGetAttrValue( node, "xmlns" )) == NULL ) return;

	if ( !_tcscmp( type, _T("urn:ietf:params:xml:ns:xmpp-sasl") )) {
		DBVARIANT dbv;

		JabberLog( "Success: Logged-in." );
		if ( DBGetContactSettingString( NULL, m_szProtoName, "Nick", &dbv ))
			JSetStringT( NULL, "Nick", info->username );
		else
			JFreeVariant( &dbv );
		xmlStreamInitialize( "after successful sasl" );
	}
	else JabberLog( "Success: unknown action "TCHAR_STR_PARAM".",type);
}

void CJabberProto::JabberProcessChallenge( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* )userdata;
	if ( info->auth == NULL ) {
		JabberLog( "No previous auth have been made, exiting..." );
		return;
	}

	if ( lstrcmp( JabberXmlGetAttrValue( node, "xmlns" ), _T("urn:ietf:params:xml:ns:xmpp-sasl")))
		return;

	char* response = info->auth->getChallenge( node->text );

	XmlNode n( "response", response );
	n.addAttr( "xmlns", _T("urn:ietf:params:xml:ns:xmpp-sasl"));
	info->send( n );

	mir_free( response );
}

void JabberProcessProtocol( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;

	info->proto->JabberConsoleProcessXml(node, JCPF_IN);

	if ( !strcmp( node->name, "proceed" )) {
		info->proto->JabberProcessProceed( node, userdata );
		return;
	}

	if ( !strcmp( node->name, "compressed" )) {
		info->proto->JabberProcessCompressed( node, userdata );
		return;
	}

	if ( !strcmp( node->name, "stream:features" ))
		info->proto->JabberProcessFeatures( node, userdata );
	else if ( !strcmp( node->name, "success"))
		info->proto->JabberProcessSuccess( node, userdata );
	else if ( !strcmp( node->name, "failure"))
		info->proto->JabberProcessFailure( node, userdata );
	else if ( !strcmp( node->name, "stream:error"))
		info->proto->JabberProcessError( node, userdata );
	else if ( !strcmp( node->name, "challenge" ))
		info->proto->JabberProcessChallenge( node, userdata );
	else if ( info->type == JABBER_SESSION_NORMAL ) {
		if ( !strcmp( node->name, "message" ))
			info->proto->JabberProcessMessage( node, userdata );
		else if ( !strcmp( node->name, "presence" ))
			info->proto->JabberProcessPresence( node, userdata );
		else if ( !strcmp( node->name, "iq" ))
			info->proto->JabberProcessIq( node, userdata );
		else
			info->proto->JabberLog( "Invalid top-level tag ( only <message/> <presence/> and <iq/> allowed )" );
	}
	else if ( info->type == JABBER_SESSION_REGISTER ) {
		if ( !strcmp( node->name, "iq" ))
			info->proto->JabberProcessRegIq( node, userdata );
		else
			info->proto->JabberLog( "Invalid top-level tag ( only <iq/> allowed )" );
}	}

void CJabberProto::JabberProcessProceed( XmlNode *node, void *userdata )
{
	ThreadData* info;
	TCHAR* type;
	node = node;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type = JabberXmlGetAttrValue( node, "xmlns" )) != NULL && !lstrcmp( type, _T("error")))
		return;

	if ( !lstrcmp( type, _T("urn:ietf:params:xml:ns:xmpp-tls" ))) {
		JabberLog("Starting TLS...");
		if ( !JabberSslInit() ) {
			JabberLog( "SSL initialization failed" );
			return;
		}

		int socket = JCallService( MS_NETLIB_GETSOCKET, ( WPARAM ) info->s, 0 );
		PVOID ssl;
		if (( ssl=pfn_SSL_new( jabberSslCtx )) != NULL ) {
			JabberLog( "SSL create context ok" );
			if ( pfn_SSL_set_fd( ssl, socket ) > 0 ) {
				JabberLog( "SSL set fd ok" );
				if ( pfn_SSL_connect( ssl ) > 0 ) {
					JabberLog( "SSL negotiation ok" );
					info->ssl = ssl;
					info->useSSL = true;
					JabberLog( "SSL enabled for handle = %d", info->s );
					xmlStreamInitialize( "after successful StartTLS" );
				}
				else {
					JabberLog( "SSL negotiation failed" );
					pfn_SSL_free( ssl );
			}	}
			else {
				JabberLog( "SSL set fd failed" );
				pfn_SSL_free( ssl );
}	}	}	}

void CJabberProto::JabberProcessCompressed( XmlNode *node, void *userdata )
{
	ThreadData* info;
	TCHAR* type;

	JabberLog( "Compression confirmed" );

	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type = JabberXmlGetAttrValue( node, "xmlns" )) != NULL && !lstrcmp( type, _T( "error" )))
		return;
	if ( lstrcmp( type, _T( "http://jabber.org/protocol/compress" )))
		return;

	JabberLog( "Starting Zlib stream compression..." );

	info->useZlib = TRUE;
	info->zRecvData = ( char* )mir_alloc( ZLIB_CHUNK_SIZE );

	xmlStreamInitialize( "after successful Zlib init" );
}

void CJabberProto::JabberProcessPubsubEvent( XmlNode *node )
{
	TCHAR* from = JabberXmlGetAttrValue( node, "from" );
	if ( !from )
		return;

	HANDLE hContact = JabberHContactFromJID( from );
	if ( !hContact )
		return;

	XmlNode* eventNode = JabberXmlGetChildWithGivenAttrValue( node, "event", "xmlns", _T(JABBER_FEAT_PUBSUB_EVENT));
	if ( !eventNode )
		return;

	XmlNode* itemsNode = JabberXmlGetChildWithGivenAttrValue( eventNode, "items", "node", _T(JABBER_FEAT_USER_MOOD));
	if ( itemsNode && JGetByte( "EnableUserMood", TRUE )) {
		// node retract?
		if ( JabberXmlGetChild( itemsNode, "retract" )) {
			JabberSetContactMood( hContact, NULL, NULL );
			return;
		}

		XmlNode* itemNode = JabberXmlGetChild( itemsNode, "item" );
		if ( !itemNode )
			return;

		XmlNode* moodNode = JabberXmlGetChildWithGivenAttrValue( itemNode, "mood", "xmlns", _T(JABBER_FEAT_USER_MOOD));
		if ( !moodNode )
			return;

		char*  moodType = NULL;
		TCHAR* moodText = NULL;
		for ( int i=0; i<moodNode->numChild; i++ ) {
			if ( !strcmp( moodNode->child[i]->name, "text" ))
				moodText = moodNode->child[i]->text;
			else
				moodType = moodNode->child[i]->name;
		}
		JabberSetContactMood( hContact, moodType, moodText );
	}
	else if ( JGetByte( "EnableUserTune", FALSE ) && (itemsNode = JabberXmlGetChildWithGivenAttrValue( eventNode, "items", "node", _T(JABBER_FEAT_USER_TUNE)))) {
		// node retract?
		if ( JabberXmlGetChild( itemsNode, "retract" )) {
			JabberSetContactTune( hContact, NULL, NULL, NULL, NULL, NULL, NULL );
			return;
		}

		XmlNode *itemNode = JabberXmlGetChild( itemsNode, "item" );
		if ( !itemNode )
			return;

		XmlNode *tuneNode = JabberXmlGetChildWithGivenAttrValue( itemNode, "tune", "xmlns", _T(JABBER_FEAT_USER_TUNE));
		if ( !tuneNode )
			return;

		TCHAR *szArtist = NULL, *szLength = NULL, *szSource = NULL;
		TCHAR *szTitle = NULL, *szTrack = NULL, *szUri = NULL;
		XmlNode* childNode;

		childNode = JabberXmlGetChild( tuneNode, "artist" );
		if ( childNode && childNode->text )
			szArtist = childNode->text;
		childNode = JabberXmlGetChild( tuneNode, "length" );
		if ( childNode && childNode->text )
			szLength = childNode->text;
		childNode = JabberXmlGetChild( tuneNode, "source" );
		if ( childNode && childNode->text )
			szSource = childNode->text;
		childNode = JabberXmlGetChild( tuneNode, "title" );
		if ( childNode && childNode->text )
			szTitle = childNode->text;
		childNode = JabberXmlGetChild( tuneNode, "track" );
		if ( childNode && childNode->text )
			szTrack = childNode->text;
		childNode = JabberXmlGetChild( tuneNode, "uri" );
		if ( childNode && childNode->text )
			szUri = childNode->text;

		TCHAR szLengthInTime[32];
		szLengthInTime[0] = _T('\0');
		if ( szLength ) {
			int nLength = _ttoi( szLength );
			mir_sntprintf( szLengthInTime, SIZEOF( szLengthInTime ),  _T("%02d:%02d:%02d"),
				nLength / 3600, (nLength / 60) % 60, nLength % 60 );
		}

		JabberSetContactTune( hContact, szArtist, szLength ? szLengthInTime : NULL, szSource, szTitle, szTrack, szUri );
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

HANDLE CJabberProto::JabberCreateTemporaryContact( TCHAR *szJid, JABBER_LIST_ITEM* chatItem )
{
	HANDLE hContact = NULL;
	if ( chatItem ) {
		TCHAR* p = _tcschr( szJid, '/' );
		if ( p != NULL && p[1] != '\0' )
			p++;
		else
			p = szJid;
		hContact = JabberDBCreateContact( szJid, p, TRUE, FALSE );

		for ( int i=0; i < chatItem->resourceCount; i++ ) {
			if ( !lstrcmp( chatItem->resource[i].resourceName, p )) {
				JSetWord( hContact, "Status", chatItem->resource[i].status );
				break;
			}
		}
	}
	else {
		TCHAR *nick = JabberNickFromJID( szJid );
		hContact = JabberDBCreateContact( szJid, nick, TRUE, TRUE );
		mir_free( nick );
	}
	return hContact;
}

void CJabberProto::JabberProcessMessage( XmlNode *node, void *userdata )
{
	ThreadData* info;
	XmlNode *subjectNode, *xNode, *inviteNode, *idNode, *n;
	TCHAR* from, *type, *idStr, *fromResource;
	HANDLE hContact;

	if ( !node->name || strcmp( node->name, "message" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;

	type = JabberXmlGetAttrValue( node, "type" );
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	hContact = JabberHContactFromJID( from );

	XmlNode* errorNode = JabberXmlGetChild( node, "error" );
	if ( errorNode != NULL || !lstrcmp( type, _T("error"))) {
		// we check if is message delivery failure
		int id = JabberGetPacketID( node );
		JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, from );
		if ( item != NULL ) { // yes, it is
			TCHAR *szErrText = JabberErrorMsg(errorNode);
			char *errText = mir_t2a(szErrText);
			JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE ) id, (LPARAM)errText );
			mir_free(errText);
			mir_free(szErrText);
		}
		return;
	}

	if ( n = JabberXmlGetChildWithGivenAttrValue( node, "data", "xmlns", _T( JABBER_FEAT_IBB ))) {
		BOOL bOk = FALSE;
		TCHAR *sid = JabberXmlGetAttrValue( n, "sid" );
		TCHAR *seq = JabberXmlGetAttrValue( n, "seq" );
		if ( sid && seq && n->text ) {
			bOk = JabberIbbProcessRecvdData( n->text, sid, seq );
		}
		return;
	}

	if ( n = JabberXmlGetChildWithGivenAttrValue( node, "event", "xmlns", _T( "http://jabber.org/protocol/pubsub#event" ))) {
		JabberProcessPubsubEvent( node );
		return;
	}

	JABBER_LIST_ITEM *chatItem = JabberListGetItemPtr( LIST_CHATROOM, from );
	if (!lstrcmp( type, _T("groupchat")))
	{
		if ( chatItem )
		{	// process GC message
			JabberGroupchatProcessMessage( node, userdata );
		} else
		{	// got message from onknown conference... let's leave it :)
			TCHAR *conference = NEWTSTR_ALLOCA(from);
			if (TCHAR *s = _tcschr(conference, _T('/'))) *s = 0;
			XmlNode p( "presence" ); p.addAttr( "to", conference ); p.addAttr( "type", "unavailable" );
			info->send( p );
		}
		return;
	}

	TCHAR* szMessage = NULL;
	XmlNode* bodyNode = JabberXmlGetChild( node, "body" );
	if ( bodyNode != NULL && bodyNode->text )
		szMessage = bodyNode->text;
	if (( subjectNode = JabberXmlGetChild( node, "subject" )) && subjectNode->text && subjectNode->text[0] != _T('\0')) {
		int cbLen = (szMessage ? _tcslen( szMessage ) : 0) + _tcslen( subjectNode->text ) + 128;
		TCHAR* szTmp = ( TCHAR * )alloca( sizeof(TCHAR) * cbLen );
		szTmp[0] = _T('\0');
		if ( szMessage )
			_tcscat( szTmp, _T("Subject: "));
		_tcscat( szTmp, subjectNode->text );
		if ( szMessage ) {
			_tcscat( szTmp, _T("\r\n"));
			_tcscat( szTmp, szMessage );
		}
		szMessage = szTmp;
	}

	if ( szMessage && (n = JabberXmlGetChildWithGivenAttrValue( node, "addresses", "xmlns", _T(JABBER_FEAT_EXT_ADDRESSING)))) {
		XmlNode* addressNode = JabberXmlGetChildWithGivenAttrValue( n, "address", "type", _T("ofrom") );
		if ( addressNode ) {
			TCHAR* szJid = JabberXmlGetAttrValue( addressNode, "jid" );
			if ( szJid ) {
				int cbLen = _tcslen( szMessage ) + 1000;
				TCHAR* p = ( TCHAR* )alloca( sizeof( TCHAR ) * cbLen );
				mir_sntprintf( p, cbLen, TranslateT("Message redirected from: %s\r\n%s"), from, szMessage );
				szMessage = p;
				from = szJid;
				// rewrite hContact
				hContact = JabberHContactFromJID( from );
			}
		}
	}

	// If message is from a stranger ( not in roster ), item is NULL
	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, from );
	if ( !item )
		item = JabberListGetItemPtr( LIST_VCARD_TEMP, from );

	JABBER_RESOURCE_STATUS *resourceStatus = JabberResourceInfoFromJID( from );

	time_t msgTime = 0;
	BOOL  isChatRoomInvitation = FALSE;
	TCHAR* inviteRoomJid = NULL;
	TCHAR* inviteFromJid = NULL;
	TCHAR* inviteReason = NULL;
	TCHAR* invitePassword = NULL;
	BOOL delivered = FALSE;

	// check chatstates availability
	if ( resourceStatus && JabberXmlGetChildWithGivenAttrValue( node, "active", "xmlns", _T( JABBER_FEAT_CHATSTATES )))
		resourceStatus->jcbManualDiscoveredCaps |= JABBER_CAPS_CHATSTATES;

	// chatstates composing event
	if ( hContact && JabberXmlGetChildWithGivenAttrValue( node, "composing", "xmlns", _T( JABBER_FEAT_CHATSTATES )))
		JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM )hContact, 60 );

	// chatstates paused event
	if ( hContact && JabberXmlGetChildWithGivenAttrValue( node, "paused", "xmlns", _T( JABBER_FEAT_CHATSTATES )))
		JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM )hContact, PROTOTYPE_CONTACTTYPING_OFF );

	idStr = JabberXmlGetAttrValue( node, "id" );

	// message receipts delivery request
	if ( JabberXmlGetChildWithGivenAttrValue( node, "request", "xmlns", _T( JABBER_FEAT_MESSAGE_RECEIPTS ))) {
		XmlNode m( "message" ); m.addAttr( "to", from );
		XmlNode* receivedNode = m.addChild( "received" );
		receivedNode->addAttr( "xmlns", JABBER_FEAT_MESSAGE_RECEIPTS );
		if ( idStr )
			m.addAttr( "id", idStr );
		info->send( m );

		if ( resourceStatus )
			resourceStatus->jcbManualDiscoveredCaps |= JABBER_CAPS_MESSAGE_RECEIPTS;
	}

	// message receipts delivery notification
	if ( JabberXmlGetChildWithGivenAttrValue( node, "received", "xmlns", _T( JABBER_FEAT_MESSAGE_RECEIPTS ))) {
		int nPacketId = JabberGetPacketID( node );
		if ( nPacketId != -1)
			JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) nPacketId, 0 );
	}

	// XEP-0203 delay support
	if ( n = JabberXmlGetChildWithGivenAttrValue( node, "delay", "xmlns", _T("urn:xmpp:delay") ) ) {
		TCHAR* ptszTimeStamp = JabberXmlGetAttrValue( n, "stamp" );
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
	if ( JabberXmlGetChildWithGivenAttrValue( node, "attention", "xmlns", _T( JABBER_FEAT_ATTENTION ))) {
		if ( !hContact )
			hContact = JabberCreateTemporaryContact( from, chatItem );
		if ( hContact )
			NotifyEventHooks( heventNudge, (WPARAM)hContact, 0 );
	}

	// chatstates gone event
	if ( hContact && JabberXmlGetChildWithGivenAttrValue( node, "gone", "xmlns", _T( JABBER_FEAT_CHATSTATES )) && JGetByte( "LogChatstates", FALSE )) {
		DBEVENTINFO dbei;
		BYTE bEventType = JABBER_DB_EVENT_CHATSTATES_GONE; // gone event
		dbei.cbSize = sizeof(dbei);
		dbei.pBlob = &bEventType;
		dbei.cbBlob = 1;
		dbei.eventType = JABBER_DB_EVENT_TYPE_CHATSTATES;
		dbei.flags = 0;
		dbei.timestamp = time(NULL);
		dbei.szModule = m_szProtoName;
		CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
	}

	for ( int i = 1; ( xNode = JabberXmlGetNthChild( node, "x", i )) != NULL; i++ ) {
		TCHAR* ptszXmlns = JabberXmlGetAttrValue( xNode, "xmlns" );
		if ( ptszXmlns == NULL )
			continue;

		if ( !_tcscmp( ptszXmlns, _T("jabber:x:encrypted" ))) {
			if ( xNode->text == NULL )
				return;

			TCHAR* prolog = _T("-----BEGIN PGP MESSAGE-----\r\n\r\n");
			TCHAR* epilog = _T("\r\n-----END PGP MESSAGE-----\r\n");
			TCHAR* tempstring = ( TCHAR* )alloca( sizeof( TCHAR ) * ( _tcslen( prolog ) + _tcslen( xNode->text ) + _tcslen( epilog ) + 3 ));
			_tcsncpy( tempstring, prolog, _tcslen( prolog ) + 1 );
			_tcsncpy( tempstring + _tcslen( prolog ), xNode->text, _tcslen( xNode->text ) + 1);
			_tcsncpy( tempstring + _tcslen( prolog ) + _tcslen(xNode->text ), epilog, _tcslen( epilog ) + 1);
			szMessage = tempstring;
      }
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:delay")) && msgTime == 0 ) {
			TCHAR* ptszTimeStamp = JabberXmlGetAttrValue( xNode, "stamp" );
			if ( ptszTimeStamp != NULL )
				msgTime = JabberIsoToUnixTime( ptszTimeStamp );
		}
		else if ( !_tcscmp( ptszXmlns, _T(JABBER_FEAT_MESSAGE_EVENTS))) {
			
			// set events support only if we discovered caps and if events not already set
			JabberCapsBits jcbCaps = JabberGetResourceCapabilites( from, TRUE );
			if ( jcbCaps & JABBER_RESOURCE_CAPS_ERROR )
				jcbCaps = JABBER_RESOURCE_CAPS_NONE;
			// FIXME: disabled due to expired XEP-0022 and problems with bombus delivery checks
//			if ( jcbCaps && resourceStatus && (!(jcbCaps & JABBER_CAPS_MESSAGE_EVENTS)) )
//				resourceStatus->jcbManualDiscoveredCaps |= (JABBER_CAPS_MESSAGE_EVENTS | JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY);

			if ( bodyNode == NULL ) {
				idNode = JabberXmlGetChild( xNode, "id" );
				if ( JabberXmlGetChild( xNode, "delivered" ) != NULL || JabberXmlGetChild( xNode, "offline" ) != NULL ) {
					int id = -1;
					if ( idNode != NULL && idNode->text != NULL )
						if ( !_tcsncmp( idNode->text, _T(JABBER_IQID), strlen( JABBER_IQID )) )
							id = _ttoi(( idNode->text )+strlen( JABBER_IQID ));

					if ( id != -1 )
						JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) id, 0 );
				}

				if ( hContact && JabberXmlGetChild( xNode, "composing" ) != NULL )
					JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, 60 );

				// Maybe a cancel to the previous composing
				if ( hContact && ( xNode->numChild==0 || ( xNode->numChild==1 && idNode != NULL )))
					JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, PROTOTYPE_CONTACTTYPING_OFF );
			}
			else {
				// Check whether any event is requested
				if ( !delivered && ( n=JabberXmlGetChild( xNode, "delivered" )) != NULL ) {
					delivered = TRUE;

					XmlNode m( "message" ); m.addAttr( "to", from );
					XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_MESSAGE_EVENTS ); x->addChild( "delivered" );
					x->addChild( "id", ( idStr != NULL ) ? idStr : NULL );
					info->send( m );
				}
				if ( item != NULL && JabberXmlGetChild( xNode, "composing" ) != NULL ) {
					if ( item->messageEventIdStr )
						mir_free( item->messageEventIdStr );
					item->messageEventIdStr = ( idStr==NULL )?NULL:mir_tstrdup( idStr );
			}	}
		}
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:oob"))) {
			XmlNode* urlNode;
			if ( ((urlNode = JabberXmlGetChild( xNode, "url" )) != NULL) && urlNode->text && urlNode->text[0] != _T('\0')) {
				int cbLen = (szMessage ? _tcslen( szMessage ) : 0) + _tcslen( urlNode->text ) + 32;
				TCHAR* szTmp = ( TCHAR * )alloca( sizeof(TCHAR) * cbLen );
				_tcscpy( szTmp, urlNode->text );
				if ( szMessage ) {
					_tcscat( szTmp, _T("\r\n"));
					_tcscat( szTmp, szMessage );
				}
				szMessage = szTmp;
			}
		}
		else if ( !_tcscmp( ptszXmlns, _T("http://jabber.org/protocol/muc#user"))) {
			if (( inviteNode=JabberXmlGetChild( xNode, "invite" )) != NULL ) {
				inviteFromJid = JabberXmlGetAttrValue( inviteNode, "from" );
				if (( n=JabberXmlGetChild( inviteNode, "reason" )) != NULL )
					inviteReason = n->text;
			}

			if (( n=JabberXmlGetChild( xNode, "password" )) != NULL )
				invitePassword = n->text;
		}
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:conference"))) {
			inviteRoomJid = JabberXmlGetAttrValue( xNode, "jid" );
			if ( inviteReason == NULL )
				inviteReason = xNode->text;
			isChatRoomInvitation = TRUE;
	}	}

	if ( isChatRoomInvitation ) {
		if ( inviteRoomJid != NULL ) {
			if ( JGetByte( "IgnoreMUCInvites", FALSE )) {
				// FIXME: temporary disabled due to MUC inconsistence on server side
				/*
				XmlNode m( "message" ); m.addAttr( "to", from );
				XmlNode* xNode = m.addChild( "x" );
				xNode->addAttr( "xmlns", JABBER_FEAT_MUC_USER );
				XmlNode* declineNode = xNode->addChild( "decline" );
				declineNode->addAttr( "from", inviteRoomJid );
				XmlNode* reasonNode = declineNode->addChild( "reason", "The user has chosen to not accept chat invites" );
				info->send( m );
				*/
			}
			else
				JabberGroupchatProcessInvite( inviteRoomJid, inviteFromJid, inviteReason, invitePassword );
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
								JabberUpdateMirVer(item);
							item->lastSeenResource = i;
							break;
		}	}	}	}	}

		// Create a temporary contact
		if ( hContact == NULL )
			hContact = JabberCreateTemporaryContact( from, chatItem );

		time_t now = time( NULL );
		if ( !msgTime )
			msgTime = now;

		if ( JGetByte( "FixIncorrectTimestamps", TRUE ) && ( msgTime > now || ( msgTime < ( time_t )JabberGetLastContactMessageTime( hContact ))))
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

		mir_free( szMessage );
		mir_free( buf );
}	}

// XEP-0115: Entity Capabilities
void CJabberProto::JabberProcessPresenceCapabilites( XmlNode *node )
{
	TCHAR* from;
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL ) return;

	JabberLog("presence: for jid " TCHAR_STR_PARAM, from);

	JABBER_RESOURCE_STATUS *r = JabberResourceInfoFromJID( from );
	if ( r == NULL ) return;

	XmlNode *n;

	// check XEP-0115 support:
	if (( n = JabberXmlGetChildWithGivenAttrValue( node, "c", "xmlns", _T(JABBER_FEAT_ENTITY_CAPS))) != NULL ) {
		TCHAR *szNode = JabberXmlGetAttrValue( n, "node" );
		TCHAR *szVer = JabberXmlGetAttrValue( n, "ver" );
		TCHAR *szExt = JabberXmlGetAttrValue( n, "ext" );
		if ( szNode && szVer ) {
			replaceStr( r->szCapsNode, szNode );
			replaceStr( r->szCapsVer, szVer );
			replaceStr( r->szCapsExt, szExt );
			HANDLE hContact = JabberHContactFromJID( from );
			if ( hContact )
				JabberUpdateMirVer( hContact, r );
		}
	}

	// update user's caps
	JabberCapsBits jcbCaps = JabberGetResourceCapabilites( from, TRUE );
}

void CJabberProto::JabberUpdateJidDbSettings( TCHAR *jid )
{
	JABBER_LIST_ITEM *item = JabberListGetItemPtr( LIST_ROSTER, jid );
	if ( !item ) return;
	HANDLE hContact = JabberHContactFromJID( jid );
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
	int r = -1, i = 0;
	for ( i=0; i < item->resourceCount; i++ )
		if (( status = JabberCombineStatus( status, item->resource[i].status )) == item->resource[i].status )
			r = i;
	item->itemResource.status = status;
	if ( r != -1 ) {
		JabberLog("JabberUpdateJidDbSettings: updating jid " TCHAR_STR_PARAM " to rc " TCHAR_STR_PARAM, item->jid, item->resource[r].resourceName );
		if ( item->resource[r].statusMessage )
			DBWriteContactSettingTString( hContact, "CList", "StatusMsg", item->resource[r].statusMessage );
		else
			DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
		JabberUpdateMirVer( hContact, &item->resource[r] );
	}

	if ( _tcschr( jid, '@' )!=NULL || JGetByte( "ShowTransport", TRUE )==TRUE )
		if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != status )
			JSetWord( hContact, "Status", ( WORD )status );

	if (status == ID_STATUS_OFFLINE)
	{ // remove xstatus icon
		JDeleteSetting( hContact, DBSETTING_XSTATUSID );
		JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );
		JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );
		JabberUpdateContactExtraIcon(hContact);
	}

	JabberMenuUpdateSrmmIcon( item );
}

void CJabberProto::JabberProcessPresence( XmlNode *node, void *userdata )
{
	ThreadData* info;
	HANDLE hContact;
	XmlNode *showNode, *statusNode, *priorityNode;
	JABBER_LIST_ITEM *item;
	TCHAR* from, *nick, *show;
	TCHAR* p;

	if ( !node || !node->name || strcmp( node->name, "presence" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL ) return;

	if ( JabberListExist( LIST_CHATROOM, from )) {
		JabberGroupchatProcessPresence( node, userdata );
		return;
	}

	BOOL bSelfPresence = FALSE;
	TCHAR szBareFrom[ 512 ];
	JabberStripJid( from, szBareFrom, SIZEOF( szBareFrom ));
	TCHAR szBareOurJid[ 512 ];
	JabberStripJid( info->fullJID, szBareOurJid, SIZEOF( szBareOurJid ));

	if ( !_tcsicmp( szBareFrom, szBareOurJid ))
		bSelfPresence = TRUE;

	TCHAR* type = JabberXmlGetAttrValue( node, "type" );
	if ( type == NULL || !_tcscmp( type, _T("available"))) {
		if (( nick = JabberNickFromJID( from )) == NULL )
			return;

		if (( hContact = JabberHContactFromJID( from )) == NULL ) {
			if ( !_tcsicmp( info->fullJID, from ) || ( !bSelfPresence && !JabberListExist( LIST_ROSTER, from ))) {
				JabberLog("SKIP Receive presence online from "TCHAR_STR_PARAM" ( who is not in my roster and not in list - skiping)", from );
				mir_free( nick );
				return;
			}
			hContact = JabberDBCreateContact( from, nick, TRUE, TRUE );
		}
		if ( !JabberListExist( LIST_ROSTER, from )) {
			JabberLog("Receive presence online from "TCHAR_STR_PARAM" ( who is not in my roster )", from );
			JabberListAdd( LIST_ROSTER, from );
		}
		JabberDBCheckIsTransportedContact(from,hContact);
		int status = ID_STATUS_ONLINE;
		if (( showNode = JabberXmlGetChild( node, "show" )) != NULL ) {
			if (( show = showNode->text ) != NULL ) {
				if ( !_tcscmp( show, _T("away"))) status = ID_STATUS_AWAY;
				else if ( !_tcscmp( show, _T("xa"))) status = ID_STATUS_NA;
				else if ( !_tcscmp( show, _T("dnd"))) status = ID_STATUS_DND;
				else if ( !_tcscmp( show, _T("chat"))) status = ID_STATUS_FREECHAT;
		}	}

		char priority = 0;
		if (( priorityNode = JabberXmlGetChild( node, "priority" )) != NULL && priorityNode->text != NULL )
			priority = (char)_ttoi( priorityNode->text );

		if (( statusNode = JabberXmlGetChild( node, "status" )) != NULL && statusNode->text != NULL )
			p = statusNode->text;
		else
			p = NULL;
		JabberListAddResource( LIST_ROSTER, from, status, p, priority );
		
		// XEP-0115: Entity Capabilities
		JabberProcessPresenceCapabilites( node );

		JabberUpdateJidDbSettings( from );

		if ( _tcschr( from, '@' )==NULL ) {
			if ( hwndJabberAgents )
				SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
			if ( hwndServiceDiscovery )
				SendMessage( hwndServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
		}
		JabberLog( TCHAR_STR_PARAM " ( " TCHAR_STR_PARAM " ) online, set contact status to %s", nick, from, JCallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,(WPARAM)status,0 ));
		mir_free( nick );

		XmlNode* xNode;
		BOOL hasXAvatar = false;
		if ( JGetByte( "EnableAvatars", TRUE )) {
			JabberLog( "Avatar enabled" );
			for ( int i = 1; ( xNode=JabberXmlGetNthChild( node, "x", i )) != NULL; i++ ) {
				if ( !lstrcmp( JabberXmlGetAttrValue( xNode, "xmlns" ), _T("jabber:x:avatar"))) {
					if (( xNode = JabberXmlGetChild( xNode, "hash" )) != NULL && xNode->text != NULL ) {
						JDeleteSetting(hContact,"AvatarXVcard");
						JabberLog( "AvatarXVcard deleted" );
						JSetStringT( hContact, "AvatarHash", xNode->text );
						hasXAvatar = true;
						DBVARIANT dbv = {0};
						int result = JGetStringT( hContact, "AvatarSaved", &dbv );
						if ( !result || lstrcmp( dbv.ptszVal, xNode->text )) {
							JabberLog( "Avatar was changed" );
							JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
						} else JabberLog( "Not broadcasting avatar changed" );
						if ( !result ) JFreeVariant( &dbv );
			}	}	}
			if ( !hasXAvatar ) { //no jabber:x:avatar. try vcard-temp:x:update
				JabberLog( "Not hasXAvatar" );
				for ( int i = 1; ( xNode=JabberXmlGetNthChild( node, "x", i )) != NULL; i++ ) {
					if ( !lstrcmp( JabberXmlGetAttrValue( xNode, "xmlns" ), _T("vcard-temp:x:update"))) {
						if (( xNode = JabberXmlGetChild( xNode, "photo" )) != NULL && xNode->text != NULL ) {
							JSetByte( hContact, "AvatarXVcard", 1 );
							JabberLog( "AvatarXVcard set" );
							JSetStringT( hContact, "AvatarHash", xNode->text );
							DBVARIANT dbv = {0};
							int result = JGetStringT( hContact, "AvatarSaved", &dbv );
							if ( !result || lstrcmp( dbv.ptszVal, xNode->text )) {
								JabberLog( "Avatar was changed. Using vcard-temp:x:update" );
								JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
							}
							else JabberLog( "Not broadcasting avatar changed" );
							if ( !result ) JFreeVariant( &dbv );
		}	}	}	}	}
		return;
	}

	if ( !_tcscmp( type, _T("unavailable"))) {
		int status = ID_STATUS_OFFLINE;
		hContact = JabberHContactFromJID( from );
		if (( item = JabberListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
			JabberListRemoveResource( LIST_ROSTER, from );

			// remove selfcontact, if where is no more another resources
			if ( item->resourceCount == 1 && JabberResourceInfoFromJID( info->fullJID ))
				JabberListRemoveResource( LIST_ROSTER, info->fullJID );

			// set status only if no more available resources
			if ( !item->resourceCount )
			{
				item->itemResource.status = ID_STATUS_OFFLINE;
				if ((( statusNode = JabberXmlGetChild( node, "status" )) != NULL ) && statusNode->text )
					replaceStr( item->itemResource.statusMessage, statusNode->text );
				else
					replaceStr( item->itemResource.statusMessage, NULL );
			}
		}
		else JabberLog( "SKIP Receive presence offline from " TCHAR_STR_PARAM " ( who is not in my roster )", from );

		JabberUpdateJidDbSettings( from );

		if ( _tcschr( from, '@' )==NULL ) {
			if ( hwndJabberAgents )
				SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
			if ( hwndServiceDiscovery )
				SendMessage( hwndServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
		}
		JabberDBCheckIsTransportedContact(from, hContact);
		return;
	}

	if ( !_tcscmp( type, _T("subscribe"))) {
		// automatically send authorization allowed to agent/transport
		if ( _tcschr( from, '@' ) == NULL || JGetByte("AutoAcceptAuthorization", FALSE )) {
			JabberListAdd( LIST_ROSTER, from );
			XmlNode p( "presence" ); p.addAttr( "to", from ); p.addAttr( "type", "subscribed" );
			info->send( p );
			if ( JGetByte( "AutoAdd", TRUE ) == TRUE ) {
				if (( item = JabberListGetItemPtr( LIST_ROSTER, from )) == NULL || ( item->subscription != SUB_BOTH && item->subscription != SUB_TO )) {
					JabberLog( "Try adding contact automatically jid = " TCHAR_STR_PARAM, from );
					if (( hContact=AddToListByJID( from, 0 )) != NULL ) {
						// Trigger actual add by removing the "NotOnList" added by AddToListByJID()
						// See AddToListByJID() and JabberDbSettingChanged().
						DBDeleteContactSetting( hContact, "CList", "NotOnList" );
			}	}	}
		}
		else {
			XmlNode* n = JabberXmlGetChild( node, "nick" );
			nick = ( n == NULL ) ? JabberNickFromJID( from ) : mir_tstrdup( n->text );
			if ( nick != NULL ) {
				JabberLog( TCHAR_STR_PARAM " ( " TCHAR_STR_PARAM " ) requests authorization", nick, from );
				JabberDBAddAuthRequest( from, nick );
				mir_free( nick );
		}	}
		return;
	}

	if ( !_tcscmp( type, _T("subscribed"))) {
		if (( item=JabberListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
			if ( item->subscription == SUB_FROM ) item->subscription = SUB_BOTH;
			else if ( item->subscription == SUB_NONE ) {
				item->subscription = SUB_TO;
				if ( _tcschr( from, '@' )==NULL ) {
					if ( hwndJabberAgents )
						SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
					if ( hwndServiceDiscovery )
						SendMessage( hwndServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
}	}	}	}	}

void CJabberProto::JabberProcessIqResultVersion( XmlNode* node, void* userdata, CJabberIqInfo *pInfo )
{
	JABBER_RESOURCE_STATUS *r = JabberResourceInfoFromJID( pInfo->GetFrom() );
	if ( r == NULL ) return;

	r->dwVersionRequestTime = -1;

	replaceStr( r->software, NULL );
	replaceStr( r->version, NULL );
	replaceStr( r->system, NULL );

	XmlNode* queryNode = pInfo->GetChildNode();

	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT && queryNode) {
		XmlNode* n;
		if (( n=JabberXmlGetChild( queryNode, "name" ))!=NULL && n->text )
			r->software = mir_tstrdup( n->text );
		if (( n=JabberXmlGetChild( queryNode, "version" ))!=NULL && n->text )
			r->version = mir_tstrdup( n->text );
		if (( n=JabberXmlGetChild( queryNode, "os" ))!=NULL && n->text )
			r->system = mir_tstrdup( n->text );
	}

	JabberGetResourceCapabilites( pInfo->GetFrom(), TRUE );
	if ( pInfo->GetHContact() )
		JabberUpdateMirVer( pInfo->GetHContact(), r );

	JabberUserInfoUpdate(pInfo->GetHContact());
}

void CJabberProto::JabberProcessIq( XmlNode *node, void *userdata )
{
	ThreadData* info;
	XmlNode *queryNode;
	TCHAR *type;
	TCHAR *xmlns;
	int i;
	JABBER_IQ_PFUNC pfunc;

	if ( !node->name || strcmp( node->name, "iq" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type=JabberXmlGetAttrValue( node, "type" )) == NULL ) return;

	int id = JabberGetPacketID( node );
	TCHAR* idStr = JabberXmlGetAttrValue( node, "id" );

	queryNode = JabberXmlGetChild( node, "query" );
	xmlns = JabberXmlGetAttrValue( queryNode, "xmlns" );

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
		JabberLog( "Handling iq request for id=%d", id );
		(this->*pfunc)( node, userdata );
		return;
	}
	// RECVED: <iq type='error'> ...
	else if ( !_tcscmp( type, _T("error"))) {
		JabberLog( "XXX on entry" );
		// Check for file transfer deny by comparing idStr with ft->iqId
		i = 0;
		while (( i=JabberListFindNext( LIST_FILE, i )) >= 0 ) {
			JABBER_LIST_ITEM *item = JabberListGetItemPtrFromIndex( i );
			if ( item->ft != NULL && item->ft->state == FT_CONNECTING && !_tcscmp( idStr, item->ft->iqId )) {
				JabberLog( "Denying file sending request" );
				item->ft->state = FT_DENIED;
				if ( item->ft->hFileEvent != NULL )
					SetEvent( item->ft->hFileEvent );	// Simulate the termination of file server connection
			}
			i++;
	}	}
	else if (( !_tcscmp( type, _T("get")) || !_tcscmp( type, _T("set") ))) {
		XmlNodeIq iq( "error", idStr, JabberXmlGetAttrValue( node, "from" ) );

		// FIXME: commented out, because of fucking xml parser engine incoming/outgoing text encoding problems
//		XmlNode *pFirstChild = JabberXmlGetFirstChild( node );
//		if ( pFirstChild )
//			iq.addChild( JabberXmlCopyNode( pFirstChild ) );
		
		XmlNode *errorNode = iq.addChild( "error" );
		errorNode->addAttr( "type", "cancel" );
		XmlNode *serviceNode = errorNode->addChild( "service-unavailable" );
		serviceNode->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
		info->send( iq );
	}
}

void CJabberProto::JabberProcessRegIq( XmlNode *node, void *userdata )
{
	ThreadData* info;
	XmlNode *errorNode;
	TCHAR *type, *str;

	if ( !node->name || strcmp( node->name, "iq" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type=JabberXmlGetAttrValue( node, "type" )) == NULL ) return;

	int id = JabberGetPacketID( node );

	if ( !_tcscmp( type, _T("result"))) {

		// RECVED: result of the request for registration mechanism
		// ACTION: send account registration information
		if ( id == iqIdRegGetReg ) {
			iqIdRegSetReg = JabberSerialNext();

			XmlNodeIq iq( "set", iqIdRegSetReg );
			XmlNode* query = iq.addQuery( JABBER_FEAT_REGISTER );
			query->addChild( "password", info->password );
			query->addChild( "username", info->username );
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
		errorNode = JabberXmlGetChild( node, "error" );
		str = JabberErrorMsg( errorNode );
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

	int result;

	if ( ssl != NULL )
		result = pfn_SSL_read( ssl, buf, len );
	else
		result = proto->JabberWsRecv( s, buf, len, flags );

	return result;
}

int ThreadData::recv( char* buf, size_t len )
{
	if ( useZlib )
		return zlibRecv( buf, len );

	return recvws( buf, len, MSG_DUMPASTEXT );
}

int ThreadData::sendws( char* buffer, size_t bufsize, int flags )
{
	if ( !ssl )
		return proto->JabberWsSend( s, buffer, bufsize, flags );

	if ( flags != MSG_NODUMP && DBGetContactSettingByte( NULL, "Netlib", "DumpSent", TRUE ) == TRUE ) {
		char* szLogBuffer = ( char* )alloca( bufsize+32 );
		strcpy( szLogBuffer, "( SSL ) Data sent\n" );
		memcpy( szLogBuffer+strlen( szLogBuffer ), buffer, bufsize+1  ); // also copy \0
		Netlib_Logf( proto->hNetlibUser, "%s", szLogBuffer );	// %s to protect against when fmt tokens are in szLogBuffer causing crash
	}
	return pfn_SSL_write( ssl, buffer, bufsize );
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
int ThreadData::send( XmlNode& node )
{
	if ( this == NULL )
		return 0;

	proto->JabberConsoleProcessXml(&node, JCPF_OUT|JCPF_UTF8);

	char* str = node.getText();
	int result = send( str, strlen( str ));

	mir_free( str );
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
