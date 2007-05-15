/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan

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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_thread.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"

#include <io.h>
#include <windns.h>   // requires Windows Platform SDK

#include "jabber_ssl.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_secur.h"
#include "resource.h"
#include "version.h"
#include "jabber_caps.h"

// <iq/> identification number for various actions
// for JABBER_REGISTER thread
int iqIdRegGetReg;
int iqIdRegSetReg;

static void JabberProcessStreamOpening( XmlNode *node, void *userdata );
static void JabberProcessStreamClosing( XmlNode *node, void *userdata );
static void JabberProcessProtocol( XmlNode *node, void *userdata );
static void JabberProcessMessage( XmlNode *node, void *userdata );
static void JabberProcessPresence( XmlNode *node, void *userdata );
static void JabberProcessIq( XmlNode *node, void *userdata );
static void JabberProcessProceed( XmlNode *node, void *userdata );
static void JabberProcessCompressed( XmlNode *node, void *userdata );
static void JabberProcessRegIq( XmlNode *node, void *userdata );

void JabberMenuHideSrmmIcon(HANDLE hContact);
void JabberMenuUpdateSrmmIcon(JABBER_LIST_ITEM *item);

extern int bSecureIM;
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

static VOID CALLBACK JabberOfflineChatWindows( DWORD )
{
	GCDEST gcd = { jabberProtoName, NULL, GC_EVENT_CONTROL };
	GCEVENT gce = { 0 };
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	CallService( MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Jabber keep-alive thread

static void __cdecl JabberKeepAliveThread( void* )
{
	NETLIBSELECT nls = {0};
	nls.cbSize = sizeof( NETLIBSELECT );
	nls.dwTimeout = 60000;	// 60000 millisecond ( 1 minute )
	nls.hExceptConns[0] = jabberThreadInfo->s;
	for ( ;; ) {
		if ( JCallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls ) != 0 )
			break;
		if ( jabberSendKeepAlive )
			jabberThreadInfo->send( " \t " );
	}
	JabberLog( "Exiting KeepAliveThread" );
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

static void xmlStreamInitialize(char *which)
{
	JabberLog("Stream will be initialized %s",which);
	xmlStreamToBeInitialized = strdup(which);
}

static void xmlStreamInitializeNow(ThreadData* info)
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
		stream.addAttr( "xml:lang", "en" );
		if ( !JGetByte( "Disable3920auth", 0 ))
			stream.addAttr( "version", "1.0" );
		stream.dirtyHack = true; // this is to keep the node open - do not send </stream:stream>
		info->send( stream );
}	}

void __cdecl JabberServerThread( ThreadData* info )
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
			oldStatus = jabberStatus;
			jabberStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );
         goto LBL_Exit;
		}

		if ( !DBGetContactSetting( NULL, jabberProtoName, "LoginServer", &dbv )) {
			strncpy( info->server, dbv.pszVal, SIZEOF( info->server )-1 );
			JFreeVariant( &dbv );
		}
		else {
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
			JabberLog( "Thread ended, login server is not configured" );
			goto LBL_FatalError;
		}

		if ( !JGetStringT( NULL, "Resource", &dbv )) {
			_tcsncpy( info->resource, dbv.ptszVal, SIZEOF( info->resource )-1 );
			JFreeVariant( &dbv );
		}
		else _tcscpy( info->resource, _T("Miranda"));

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
			if ( DBGetContactSetting( NULL, jabberProtoName, "Password", &dbv )) {
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
			if ( !DBGetContactSetting( NULL, jabberProtoName, "ManualHost", &dbv )) {
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
		goto LBL_Exit;
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
			oldStatus = jabberStatus;
			jabberStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );
			jabberThreadInfo = NULL;
		}
		else if ( info->type == JABBER_SESSION_REGISTER ) {
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Not enough memory" ));
		}
		JabberLog( "Thread ended, network buffer cannot be allocated" );
		goto LBL_Exit;
	}

	info->s = JabberWsConnect( connectHost, info->port );
	if ( info->s == NULL ) {
		JabberLog( "Connection failed ( %d )", WSAGetLastError());
		if ( info->type == JABBER_SESSION_NORMAL ) {
			if ( jabberThreadInfo == info ) {
				oldStatus = jabberStatus;
				jabberStatus = ID_STATUS_OFFLINE;
				JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK );
				JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );
				jabberThreadInfo = NULL;
		}	}
		else if ( info->type == JABBER_SESSION_REGISTER )
			SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Error: Cannot connect to the server" ));

		JabberLog( "Thread ended, connection failed" );
		mir_free( buffer );
		goto LBL_Exit;
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
				oldStatus = jabberStatus;
				jabberStatus = ID_STATUS_OFFLINE;
				JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );
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
			goto LBL_Exit;
	}	}

	// User may change status to OFFLINE while we are connecting above
	if ( jabberDesiredStatus != ID_STATUS_OFFLINE || info->type == JABBER_SESSION_REGISTER ) {

		if ( info->type == JABBER_SESSION_NORMAL ) {
			jabberConnected = TRUE;
			int len = _tcslen( info->username ) + strlen( info->server )+1;
			jabberJID = ( TCHAR* )mir_alloc( sizeof( TCHAR)*( len+1 ));
			mir_sntprintf( jabberJID, len+1, _T("%s@") _T(TCHAR_STR_PARAM), info->username, info->server );
			if ( JGetByte( "KeepAlive", 1 ))
				jabberSendKeepAlive = TRUE;
			else
				jabberSendKeepAlive = FALSE;
			mir_forkthread(( pThreadFunc )JabberKeepAliveThread, info->s );
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
			jabberOnline = FALSE;
			jabberConnected = FALSE;
			info->zlibUninit();
			JabberEnableMenuItems( FALSE );
			if ( hwndJabberChangePassword ) {
				//DestroyWindow( hwndJabberChangePassword );
				// Since this is a different thread, simulate the click on the cancel button instead
				SendMessage( hwndJabberChangePassword, WM_COMMAND, MAKEWORD( IDCANCEL, 0 ), 0 );
			}

			if ( jabberChatDllPresent )
				QueueUserAPC( JabberOfflineChatWindows, hMainThread, 0 );

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
			oldStatus = jabberStatus;
			jabberStatus = ID_STATUS_OFFLINE;
			JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );

			// Set all contacts to offline
			HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			while ( hContact != NULL ) {
				if ( !lstrcmpA(( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 ), jabberProtoName ))
				{
					if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
						JSetWord( hContact, "Status", ID_STATUS_OFFLINE );
					JabberMenuHideSrmmIcon(hContact);
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
		oldStatus = jabberStatus;
		jabberStatus = ID_STATUS_OFFLINE;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );
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

static void JabberIqProcessSearch( XmlNode *node, void *userdata )
{
}

static void JabberPerformRegistration( ThreadData* info )
{
	iqIdRegGetReg = JabberSerialNext();
	XmlNodeIq iq("get",iqIdRegGetReg,(char*)NULL);
	XmlNode* query = iq.addQuery(JABBER_FEAT_REGISTER);
	info->send( iq );
	SendMessage( info->reg_hwndDlg, WM_JABBER_REGDLG_UPDATE, 50, ( LPARAM )TranslateT( "Requesting registration instruction..." ));
}

static void JabberPerformIqAuth( ThreadData* info )
{
	if ( info->type == JABBER_SESSION_NORMAL ) {
		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultGetAuth );
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
			char* pszSid = t2a( sid );
			replaceStr( streamId, pszSid );
			mir_free( pszSid );
	}	}

	// old server - disable SASL then
	if ( JabberXmlGetAttrValue( node, "version" ) == NULL )
		JSetByte( "Disable3920auth", TRUE );

	if ( JGetByte( "Disable3920auth", 0 ))
		JabberPerformIqAuth( info );
}

static void JabberProcessStreamClosing( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;

	Netlib_CloseHandle( info->s );
	if ( node->name && !strcmp( node->name, "stream:error" ) && node->text )
		MessageBox( NULL, TranslateTS( node->text ), TranslateT( "Jabber Connection Error" ), MB_OK|MB_ICONERROR|MB_SETFOREGROUND );
}

static void JabberProcessFeatures( XmlNode *node, void *userdata )
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
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultBind );
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

static void __cdecl JabberWaitAndReconnectThread( int unused )
{
	JabberLog("Reconnecting after with new X-GOOGLE-TOKEN");
	Sleep(1000);

	ThreadData* thread = new ThreadData( JABBER_SESSION_NORMAL );
	thread->hThread = ( HANDLE ) mir_forkthread(( pThreadFunc )JabberServerThread, thread );
}

static void JabberProcessFailure( XmlNode *node, void *userdata ) {
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

static void JabberProcessError( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	TCHAR *buff;
	int i;
	int pos;
//failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"
	if ( !node->numChild ) return;
	buff = (TCHAR *)mir_alloc(1024*SIZEOF(buff));
	pos=0;
	for (i=0;i<node->numChild;i++) {
		pos += mir_sntprintf(buff+pos,1024-pos,
			_T(TCHAR_STR_PARAM)_T(": %s\n"),
			node->child[i]->name,node->child[i]->text);
		if (!strcmp(node->child[i]->name,"conflict")) JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
	}
	MessageBox( NULL, buff, TranslateT( "Jabber Error" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
	mir_free(buff);
	info->send( "</stream:stream>" );
}

static void JabberProcessSuccess( XmlNode *node, void *userdata )
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
		if ( DBGetContactSetting( NULL, jabberProtoName, "Nick", &dbv ))
			JSetStringT( NULL, "Nick", info->username );
		else
			JFreeVariant( &dbv );
		xmlStreamInitialize( "after successful sasl" );
	}
	else JabberLog( "Success: unknown action "TCHAR_STR_PARAM".",type);
}

static void JabberProcessChallenge( XmlNode *node, void *userdata )
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

static void JabberProcessProtocol( XmlNode *node, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	if ( !strcmp( node->name, "proceed" )) {
		JabberProcessProceed( node, userdata );
		return;
	}

	if ( !strcmp( node->name, "compressed" )) {
		JabberProcessCompressed( node, userdata );
		return;
	}

	if ( !strcmp( node->name, "stream:features" ))
		JabberProcessFeatures( node, userdata );
	else if ( !strcmp( node->name, "success"))
		JabberProcessSuccess( node, userdata );
	else if ( !strcmp( node->name, "failure"))
		JabberProcessFailure( node, userdata );
	else if ( !strcmp( node->name, "stream:error"))
		JabberProcessError( node, userdata );
	else if ( !strcmp( node->name, "challenge" ))
		JabberProcessChallenge( node, userdata );
	else if ( info->type == JABBER_SESSION_NORMAL ) {
		if ( !strcmp( node->name, "message" ))
			JabberProcessMessage( node, userdata );
		else if ( !strcmp( node->name, "presence" ))
			JabberProcessPresence( node, userdata );
		else if ( !strcmp( node->name, "iq" ))
			JabberProcessIq( node, userdata );
		else
			JabberLog( "Invalid top-level tag ( only <message/> <presence/> and <iq/> allowed )" );
	}
	else if ( info->type == JABBER_SESSION_REGISTER ) {
		if ( !strcmp( node->name, "iq" ))
			JabberProcessRegIq( node, userdata );
		else
			JabberLog( "Invalid top-level tag ( only <iq/> allowed )" );
}	}

static void JabberProcessProceed( XmlNode *node, void *userdata )
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

static void JabberProcessCompressed( XmlNode *node, void *userdata )
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

static void JabberProcessPubsubEvent( XmlNode *node )
{
 	XmlNode *eventNode = JabberXmlGetChildWithGivenAttrValue( node, "event", "xmlns", _T(JABBER_FEAT_PUBSUB_EVENT));
 	XmlNode *itemsNode = JabberXmlGetChildWithGivenAttrValue( eventNode, "items", "node", _T(JABBER_FEAT_USER_MOOD));
 	if ( !itemsNode )
		return;

 	XmlNode *itemNode = JabberXmlGetChild( itemsNode, "item" );
 	if ( !itemNode )
		return;

   XmlNode *moodNode = JabberXmlGetChildWithGivenAttrValue( itemNode, "mood", "xmlns", _T(JABBER_FEAT_USER_MOOD));
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

	if ( moodType || moodText ) {
 		TCHAR* from = JabberXmlGetAttrValue( node, "from" );
 		if ( from == NULL ) 
			return;

 		HANDLE hContact = JabberHContactFromJID( from );
 		if ( hContact )
			JabberSetContactMood( hContact, moodType, moodText );
}	}

static void JabberProcessMessage( XmlNode *node, void *userdata )
{
	ThreadData* info;
	XmlNode *subjectNode, *xNode, *inviteNode, *idNode, *n;
	TCHAR* from, *type, *nick, *idStr, *fromResource;
	HANDLE hContact;

	if ( !node->name || strcmp( node->name, "message" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;

	type = JabberXmlGetAttrValue( node, "type" );
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	XmlNode* errorNode = JabberXmlGetChild( node, "error" );
	if ( errorNode != NULL || !lstrcmp( type, _T("error"))) {
		// we check if is message delivery failure
		int id = JabberGetPacketID( node );
		JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, from );
		if ( item != NULL && id == item->idMsgAckPending ) { // yes, it is
			char *errText = t2a(JabberErrorMsg(errorNode));
			JSendBroadcast( JabberHContactFromJID( from ), ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE ) 1, (LPARAM)errText );
			mir_free(errText);
		}
		return;
	}

	n = JabberXmlGetChildWithGivenAttrValue( node, "data", "xmlns", _T(JABBER_FEAT_IBB) );
	if ( n ) {
		BOOL bOk = FALSE;
		TCHAR *sid = JabberXmlGetAttrValue( n, "sid" );
		TCHAR *seq = JabberXmlGetAttrValue( n, "seq" );
		if ( sid && seq && n->text ) {
			bOk = JabberIbbProcessRecvdData( n->text, sid, seq );
		}
		return;
	}

	n = JabberXmlGetChildWithGivenAttrValue( node, "event", "xmlns", _T("http://jabber.org/protocol/pubsub#event"));
	if ( n ) {
		JabberProcessPubsubEvent( node );
		return;
	}

	JABBER_LIST_ITEM* chatItem = JabberListGetItemPtr( LIST_CHATROOM, from );
	BOOL isChatRoomJid = ( chatItem != NULL );
	if ( isChatRoomJid && !lstrcmp( type, _T("groupchat"))) {
		JabberGroupchatProcessMessage( node, userdata );
		return;
	}
	BOOL isRss = !lstrcmp( type, _T("headline"));

	// If message is from a stranger ( not in roster ), item is NULL
	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, from );

	TCHAR* szMessage = NULL;
	XmlNode* bodyNode = JabberXmlGetChild( node, "body" );
	if ( bodyNode != NULL ) {
		TCHAR* ptszBody = (bodyNode) ? bodyNode->text : _T("");
		if (( subjectNode=JabberXmlGetChild( node, "subject" ))!=NULL && subjectNode->text!=NULL && subjectNode->text[0]!='\0' && !isRss ) {
			int cbLen = _tcslen( subjectNode->text ) + _tcslen( bodyNode->text ) + 24;
			TCHAR* p = ( TCHAR* )alloca( sizeof( TCHAR ) * cbLen );
			mir_sntprintf( p, cbLen, _T("Subject: %s\r\n%s"), subjectNode->text, ptszBody );
			szMessage = p;
		}
		else szMessage = ptszBody;
	}

	time_t msgTime = 0;
	BOOL  isChatRoomInvitation = FALSE;
	TCHAR* inviteRoomJid = NULL;
	TCHAR* inviteFromJid = NULL;
	TCHAR* inviteReason = NULL;
	TCHAR* invitePassword = NULL;
	BOOL delivered = FALSE, composing = FALSE;

	n = JabberXmlGetChild( node, "active" );
	if ( item != NULL && bodyNode != NULL ) {
		if ( n != NULL && !lstrcmp( JabberXmlGetAttrValue( n, "xmlns" ), _T(JABBER_FEAT_CHATSTATES)))
			item->cap |= CLIENT_CAP_CHATSTAT;
		else
			item->cap &= ~CLIENT_CAP_CHATSTAT;
	}

	n = JabberXmlGetChild( node, "composing" );
	if ( n != NULL && !lstrcmp( JabberXmlGetAttrValue( n, "xmlns" ), _T(JABBER_FEAT_CHATSTATES)))
		if (( hContact = JabberHContactFromJID( from )) != NULL )
			JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, 60 );

	n = JabberXmlGetChild( node, "paused" );
	if ( n != NULL && !lstrcmp( JabberXmlGetAttrValue( n, "xmlns" ), _T(JABBER_FEAT_CHATSTATES)))
		if (( hContact = JabberHContactFromJID( from )) != NULL )
			JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, PROTOTYPE_CONTACTTYPING_OFF );

	for ( int i = 1; ( xNode = JabberXmlGetNthChild( node, "x", i )) != NULL; i++ ) {
		TCHAR* ptszXmlns = JabberXmlGetAttrValue( xNode, "xmlns" );
		if ( ptszXmlns == NULL )
			continue;

		if ( !_tcscmp( ptszXmlns, _T("jabber:x:encrypted" ))) {
			if ( xNode->text == NULL )
				return;

			TCHAR* prolog = _T("-----BEGIN PGP MESSAGE-----\r\n\r\n");
			TCHAR* epilog = _T("\r\n-----END PGP MESSAGE-----\r\n");
			TCHAR* tempstring = ( TCHAR* )alloca( sizeof( TCHAR )*( _tcslen( prolog ) + _tcslen( xNode->text ) + _tcslen( epilog )));
			_tcsncpy( tempstring, prolog, _tcslen( prolog )+1 );
			_tcsncpy(tempstring + _tcslen( prolog ), xNode->text, _tcslen( xNode->text )+1);
			_tcsncpy(tempstring + _tcslen( prolog )+_tcslen(xNode->text ), epilog, _tcslen( epilog )+1);
			szMessage = tempstring;
      }
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:delay")) && msgTime == 0 ) {
			TCHAR* ptszTimeStamp = JabberXmlGetAttrValue( xNode, "stamp" );
			if ( ptszTimeStamp != NULL )
				msgTime = JabberIsoToUnixTime( ptszTimeStamp );
		}
		else if ( !_tcscmp( ptszXmlns, _T(JABBER_FEAT_MESSAGE_EVENTS))) {
			if ( bodyNode == NULL ) {
				idNode = JabberXmlGetChild( xNode, "id" );
				if ( JabberXmlGetChild( xNode, "delivered" ) != NULL || JabberXmlGetChild( xNode, "offline" ) != NULL ) {
					int id = -1;
					if ( idNode != NULL && idNode->text != NULL )
						if ( !_tcsncmp( idNode->text, _T(JABBER_IQID), strlen( JABBER_IQID )) )
							id = _ttoi(( idNode->text )+strlen( JABBER_IQID ));

					if ( item != NULL )
						if ( id == item->idMsgAckPending )
							JSendBroadcast( JabberHContactFromJID( from ), ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) 1, 0 );
				}

				if ( JabberXmlGetChild( xNode, "composing" ) != NULL )
					if (( hContact = JabberHContactFromJID( from )) != NULL )
 						JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, 60 );

				if ( xNode->numChild==0 || ( xNode->numChild==1 && idNode != NULL ))
					// Maybe a cancel to the previous composing
					if (( hContact = JabberHContactFromJID( from )) != NULL )
						JCallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, PROTOTYPE_CONTACTTYPING_OFF );
			}
			else {
				// Check whether any event is requested
				if ( !delivered && ( n=JabberXmlGetChild( xNode, "delivered" )) != NULL ) {
					delivered = TRUE;
					idStr = JabberXmlGetAttrValue( node, "id" );

					XmlNode m( "message" ); m.addAttr( "to", from );
					XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_MESSAGE_EVENTS ); x->addChild( "delivered" );
					x->addChild( "id", ( idStr != NULL ) ? idStr : NULL );
					info->send( m );
				}
				if ( item != NULL && JabberXmlGetChild( xNode, "composing" ) != NULL ) {
					composing = TRUE;
					if ( item->messageEventIdStr )
						mir_free( item->messageEventIdStr );
					idStr = JabberXmlGetAttrValue( node, "id" );
					item->messageEventIdStr = ( idStr==NULL )?NULL:mir_tstrdup( idStr );
			}	}
		}
		else if ( !_tcscmp( ptszXmlns, _T("jabber:x:oob")) && isRss) {
			XmlNode* rssUrlNode;
			if ( (rssUrlNode = JabberXmlGetNthChild( xNode, "url", 1 )) != NULL) {
				TCHAR* ptszBody = (bodyNode) ? bodyNode->text : _T("");
				TCHAR* ptszSubject = (subjectNode) ? subjectNode->text : _T("");
				int cbLen = lstrlen( ptszBody ) + lstrlen( ptszSubject ) + lstrlen( rssUrlNode->text ) + 32;
				szMessage = ( TCHAR* )alloca( sizeof(TCHAR) * cbLen );
				mir_sntprintf( szMessage, cbLen, _T("Subject: %s\r\n"), ptszSubject );
				if ( rssUrlNode->text ) {
					_tcscat( szMessage, rssUrlNode->text );
					_tcscat( szMessage, _T("\r\n" ));
				}
				_tcscat( szMessage, ptszBody );
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
		if ( inviteRoomJid != NULL )
			JabberGroupchatProcessInvite( inviteRoomJid, inviteFromJid, inviteReason, invitePassword );
		return;
	}

	if ( bodyNode != NULL ) {
		if ( bodyNode->text == NULL )
			return;

		WCHAR* wszMessage;
		char*  szAnsiMsg;
		int    cbAnsiLen, cbWideLen;

		if (( szMessage = JabberUnixToDosT( szMessage )) == NULL )
			szMessage = mir_tstrdup( _T(""));

		#if defined( _UNICODE )
			wszMessage = szMessage; cbWideLen = wcslen( szMessage );
			cbAnsiLen = WideCharToMultiByte( CP_ACP, 0, wszMessage, cbWideLen, NULL, 0, NULL, NULL );
			szAnsiMsg = ( char* )alloca( cbAnsiLen+1 );
			WideCharToMultiByte( CP_ACP, 0, wszMessage, cbWideLen, szAnsiMsg, cbAnsiLen, NULL, NULL );
			szAnsiMsg[ cbAnsiLen ] = 0;
		#else
			szAnsiMsg = szMessage; cbAnsiLen = strlen( szMessage );
			cbWideLen = MultiByteToWideChar( CP_ACP, 0, szAnsiMsg, cbAnsiLen, NULL, 0 );
			wszMessage = ( WCHAR* )alloca( sizeof(WCHAR)*( cbWideLen+1 ));
			MultiByteToWideChar( CP_ACP, 0, szAnsiMsg, cbAnsiLen, wszMessage, cbWideLen );
			wszMessage[ cbWideLen ] = 0;
		#endif

		char* buf = ( char* )alloca( cbAnsiLen+1 + (cbWideLen+1)*sizeof( WCHAR ));
		memcpy( buf, szAnsiMsg, cbAnsiLen+1 );
		memcpy( buf + cbAnsiLen + 1, wszMessage, (cbWideLen+1)*sizeof( WCHAR ));

		HANDLE hContact = JabberHContactFromJID( from );

		if ( item != NULL ) {
			item->wantComposingEvent = composing;
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

		if ( hContact == NULL ) {
			// Create a temporary contact
			if ( isChatRoomJid ) {
				TCHAR* p = _tcschr( from, '/' );
				if ( p != NULL && p[1] != '\0' )
					p++;
				else
					p = from;
				hContact = JabberDBCreateContact( from, p, TRUE, FALSE );

				for ( int i=0; i < chatItem->resourceCount; i++ ) {
					if ( !lstrcmp( chatItem->resource[i].resourceName, p )) {
						JSetWord( hContact, "Status", chatItem->resource[i].status );
						break;
				}	}
			}
			else {
				nick = JabberNickFromJID( from );
				hContact = JabberDBCreateContact( from, nick, TRUE, TRUE );
				mir_free( nick );
		}	}

		time_t now = time( NULL );
		if ( msgTime == 0 || now - jabberLoggedInTime > 60 )
			msgTime = now;

		PROTORECVEVENT recv;
		recv.flags = PREF_UNICODE;
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
}	}

// XEP-0115: Entity Capabilities
void JabberProcessPresenceCapabilites( XmlNode *node )
{
	TCHAR* from;
	XmlNode *n;

	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL ) return;

	JABBER_LIST_ITEM *item = JabberListGetItemPtr( LIST_ROSTER, from );
	if ( item == NULL ) return;

	JABBER_RESOURCE_STATUS *r = item->resource;
	if ( r == NULL ) return;

	TCHAR* p = _tcschr( from, '/' );
	if ( p == NULL )    return;
	if ( *++p == '\0' ) return;

	int i;
	for ( i=0; i<item->resourceCount && _tcscmp( r->resourceName, p ); i++, r++ );
	if ( i >= item->resourceCount )
		return;

	// check XEP-0115 support:
	if (( n = JabberXmlGetChildWithGivenAttrValue( node, "c", "xmlns", _T(JABBER_FEAT_ENTITY_CAPS))) != NULL ) {
		TCHAR *szNode = JabberXmlGetAttrValue( n, "node" );
		TCHAR *szVer = JabberXmlGetAttrValue( n, "ver" );
		TCHAR *szExt = JabberXmlGetAttrValue( n, "ext" );
		if ( szNode && szVer ) {
			if ( !r->dwVersionRequestTime ) {
				HANDLE hContact = JabberHContactFromJID( from );
				if ( hContact ) {
					TCHAR szMirVer[ 512 ];
					if ( _tcsstr( szNode, _T("miranda-im.org"))) {
						if ( szExt )
							mir_sntprintf( szMirVer, SIZEOF(szMirVer ), _T("Miranda IM %s (Jabber %s) (%s)"), szVer, szVer, szExt );
						else
							mir_sntprintf( szMirVer, SIZEOF(szMirVer ), _T("Miranda IM %s (Jabber %s)"), szVer, szVer );
					}
					else if ( !szExt )
						mir_sntprintf( szMirVer, SIZEOF(szMirVer ), _T("%s#%s"), szNode, szVer );
					else if ( _tcsstr( szExt, _T(JABBER_EXT_SECUREIM) ))
						mir_sntprintf( szMirVer, SIZEOF(szMirVer), _T("%s#%s#%s (SecureIM)"), szNode, szVer, szExt );
					else if ( _tcsstr( szNode, _T("www.google.com") ))
						mir_sntprintf( szMirVer, SIZEOF(szMirVer), _T("%s#%s#%s gtalk"), szNode, szVer, szExt );
					else
						mir_sntprintf( szMirVer, SIZEOF(szMirVer), _T("%s#%s#%s"), szNode, szVer, szExt );
					JSetStringT( hContact, "MirVer", szMirVer );
			}	}

			replaceStr( r->szCapsNode, szNode );
			replaceStr( r->szCapsVer, szVer );
			replaceStr( r->szCapsExt, szExt );
		}
	}

	// update user's caps
	DWORD dwCaps = JabberGetResourceCapabilites( from );
}

static void JabberProcessPresence( XmlNode *node, void *userdata )
{
	ThreadData* info;
	HANDLE hContact;
	XmlNode *showNode, *statusNode;
	JABBER_LIST_ITEM *item;
	TCHAR* from, *nick, *show;
	int i;
	TCHAR* p;

	if ( !node || !node->name || strcmp( node->name, "presence" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL ) return;

	if ( JabberListExist( LIST_CHATROOM, from )) {
		JabberGroupchatProcessPresence( node, userdata );
		return;
	}

	TCHAR* type = JabberXmlGetAttrValue( node, "type" );
	if ( type == NULL || !_tcscmp( type, _T("available"))) {
		if (( nick = JabberNickFromJID( from )) == NULL )
			return;

		if (( hContact = JabberHContactFromJID( from )) == NULL ) {
			if (!JabberListExist( LIST_ROSTER, from )) {
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

		// Send version query if this is the new resource
//		if (( p = _tcschr( from, '@' )) != NULL ) {
//			if (( p = _tcschr( p, '/' ))!=NULL && p[1]!='\0' ) {
//				p++;
//				if (( item = JabberListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
//					JABBER_RESOURCE_STATUS *r = item->resource;
//					for ( i=0; i < item->resourceCount && lstrcmp( r->resourceName, p ); i++, r++ );
//					if ( i >= item->resourceCount || ( r->version == NULL && r->system == NULL && r->software == NULL )) {
//						XmlNodeIq iq( "get", JabberSerialNext(), from );
//						XmlNode* query = iq.addQuery( JABBER_FEAT_VERSION );
//						info->send( iq );
//		}	}	}	}

		if (( statusNode = JabberXmlGetChild( node, "status" )) != NULL && statusNode->text != NULL )
			p = mir_tstrdup( statusNode->text );
		else
			p = NULL;
		JabberListAddResource( LIST_ROSTER, from, status, p );
		if ( p ) {
			DBWriteContactSettingTString( hContact, "CList", "StatusMsg", p );
			mir_free( p );
		}
		else DBDeleteContactSetting( hContact, "CList", "StatusMsg" );

		// Determine status to show for the contact
		if (( item=JabberListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
			for ( i=0; i < item->resourceCount; i++ )
				status = JabberCombineStatus( status, item->resource[i].status );
			item->status = status;
			JabberMenuUpdateSrmmIcon(item);
		}

		if ( _tcschr( from, '@' )!=NULL || JGetByte( "ShowTransport", TRUE )==TRUE )
			if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != status )
				JSetWord( hContact, "Status", ( WORD )status );

		if ( _tcschr( from, '@' )==NULL && hwndJabberAgents )
			SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
		JabberLog( TCHAR_STR_PARAM " ( " TCHAR_STR_PARAM " ) online, set contact status to %s", nick, from, JCallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,(WPARAM)status,0 ));
		mir_free( nick );

		// XEP-0115: Entity Capabilities
		JabberProcessPresenceCapabilites( node );

		XmlNode* xNode;
		BOOL hasXAvatar = false;
		if (JGetByte( "EnableAvatars", TRUE )) {
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
			if (!hasXAvatar) { //no jabber:x:avatar. try vcard-temp:x:update
				JabberLog( "Not hasXAvatar" );
				for ( int i = 1; ( xNode=JabberXmlGetNthChild( node, "x", i )) != NULL; i++ ) {
					if ( !lstrcmp( JabberXmlGetAttrValue( xNode, "xmlns" ), _T("vcard-temp:x:update"))) {
						if (( xNode = JabberXmlGetChild( xNode, "photo" )) != NULL && xNode->text != NULL ) {
							JSetByte(hContact,"AvatarXVcard",1);
							JabberLog( "AvatarXVcard set" );
							JSetStringT( hContact, "AvatarHash", xNode->text );
							DBVARIANT dbv = {0};
							int result = JGetStringT( hContact, "AvatarSaved", &dbv );
							if ( !result || lstrcmp( dbv.ptszVal, xNode->text )) {
								JabberLog( "Avatar was changed. Using vcard-temp:x:update" );
								JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
							} JabberLog( "Not broadcasting avatar changed" );
							if ( !result ) JFreeVariant( &dbv );
		}	}	}	}	}
		return;
	}

	if ( !_tcscmp( type, _T("unavailable"))) {
		if ( !JabberListExist( LIST_ROSTER, from ))
			JabberLog( "SKIP Receive presence offline from " TCHAR_STR_PARAM " ( who is not in my roster )", from );
		else
			JabberListRemoveResource( LIST_ROSTER, from );

		int status = ID_STATUS_OFFLINE;
		if (( statusNode = JabberXmlGetChild( node, "status" )) != NULL ) {
			if ( JGetByte( "OfflineAsInvisible", FALSE ) == TRUE )
				status = ID_STATUS_INVISIBLE;

			if (( hContact = JabberHContactFromJID( from )) != NULL) {
				if ( statusNode->text )
					DBWriteContactSettingTString(hContact, "CList", "StatusMsg", statusNode->text );
				else
					DBDeleteContactSetting(hContact, "CList", "StatusMsg");
		}	}

		// Determine status to show for the contact based on the remaining resources
		if (( item=JabberListGetItemPtr( LIST_ROSTER, from )) != NULL ) {
			status = ID_STATUS_OFFLINE;
			for ( i=0; i < item->resourceCount; i++ )
				status = JabberCombineStatus( status, item->resource[i].status );
			item->status = status;
			JabberMenuUpdateSrmmIcon(item);
		}
		if (( hContact=JabberHContactFromJID( from )) != NULL ) {
			if ( _tcschr( from, '@' )!=NULL || JGetByte( "ShowTransport", TRUE )==TRUE )
				if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != status )
					JSetWord( hContact, "Status", ( WORD )status );

			JabberLog( TCHAR_STR_PARAM " offline, set contact status to %d", from, status );
		}
		if ( _tcschr( from, '@' )==NULL && hwndJabberAgents )
			SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
		JabberDBCheckIsTransportedContact(from, hContact);
		return;
	}

	if ( !_tcscmp( type, _T("subscribe"))) {
		if ( _tcschr( from, '@' ) == NULL ) {
			// automatically send authorization allowed to agent/transport
			XmlNode p( "presence" ); p.addAttr( "to", from ); p.addAttr( "type", "subscribed" );
			info->send( p );
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
				if ( hwndJabberAgents && _tcschr( from, '@' )==NULL )
					SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
}	}	}	}


static int JGetMirandaProductText(WPARAM wParam,LPARAM lParam)
{
	char filename[MAX_PATH],*productName;
	DWORD unused;
	DWORD verInfoSize;
	UINT blockSize;
	PVOID pVerInfo;
	GetModuleFileNameA(NULL,filename,SIZEOF(filename));
	verInfoSize=GetFileVersionInfoSizeA(filename,&unused);
	pVerInfo=mir_alloc(verInfoSize);
	GetFileVersionInfoA(filename,0,verInfoSize,pVerInfo);
	VerQueryValueA(pVerInfo,"\\StringFileInfo\\000004b0\\ProductName",(void**)&productName,&blockSize);
	#if defined( _UNICODE )
		mir_snprintf(( char* )lParam, wParam, "%s", productName );
	#else
		lstrcpynA((char*)lParam,productName,wParam);
	#endif
	mir_free(pVerInfo);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Handles various <iq... requests

static void JabberProcessIqVersion( TCHAR* idStr, XmlNode* node )
{
	TCHAR* from;
	if (( from=JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	char* version = JabberGetVersionText();
	TCHAR* os = NULL;

	OSVERSIONINFO osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if ( GetVersionEx( &osvi )) {
		switch ( osvi.dwPlatformId ) {
		case VER_PLATFORM_WIN32_NT:
			if ( osvi.dwMajorVersion == 6 )
				 os = TranslateT( "Windows Vista" );
			else if ( osvi.dwMajorVersion == 5 ) {
				if ( osvi.dwMinorVersion == 2 ) os = TranslateT( "Windows Server 2003" );
				else if ( osvi.dwMinorVersion == 1 ) os = TranslateT( "Windows XP" );
				else if ( osvi.dwMinorVersion == 0 ) os = TranslateT( "Windows 2000" );
			}
			else if ( osvi.dwMajorVersion <= 4 ) {
				os = TranslateT( "Windows NT" );
			}
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			if ( osvi.dwMajorVersion == 4 ) {
				if ( osvi.dwMinorVersion == 0 ) os = TranslateT( "Windows 95" );
				if ( osvi.dwMinorVersion == 10 ) os = TranslateT( "Windows 98" );
				if ( osvi.dwMinorVersion == 90 ) os = TranslateT( "Windows ME" );
			}
			break;
	}	}

	if ( os == NULL ) os = TranslateT( "Windows" );

	char mversion[100];
	JCallService( MS_SYSTEM_GETVERSIONTEXT, sizeof( mversion ), ( LPARAM )mversion );

	char mproduct[50];
	JGetMirandaProductText( sizeof( mproduct ), ( LPARAM )mproduct );

	TCHAR* fullVer = (TCHAR*)alloca(1000 * sizeof( TCHAR ));
	mir_sntprintf( fullVer, 1000, _T(TCHAR_STR_PARAM) _T(" ") _T(TCHAR_STR_PARAM) _T(" (Jabber v.") _T(TCHAR_STR_PARAM) _T(" [%s])") _T(TCHAR_STR_PARAM),
		mproduct, mversion, __VERSION_STRING, jabberThreadInfo->resource, bSecureIM ? " (SecureIM)":"" );

	XmlNodeIq iq( "result", idStr, from );
	XmlNode* query = iq.addQuery( JABBER_FEAT_VERSION );
	query->addChild( "name", fullVer ); query->addChild( "version", version ); query->addChild( "os", os );
	jabberThreadInfo->send( iq );

	if ( version ) mir_free( version );
}

// last activity (XEP-0012) support
static void JabberProcessIqLast( TCHAR* idStr, XmlNode* node )
{
	TCHAR* from;
	if (( from=JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	XmlNodeIq iq( "result", idStr, from );
	XmlNode* query = iq.addQuery( JABBER_FEAT_LAST_ACTIVITY );
	query->addAttr("seconds", jabberIdleStartTime ? time( 0 ) - jabberIdleStartTime : 0 );
	jabberThreadInfo->send( iq );
}

// XEP-0199: XMPP Ping support
static void JabberProcessIqPing( TCHAR* idStr, XmlNode* node )
{
	TCHAR* from;
	if (( from=JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	XmlNodeIq iq( "result", idStr, from );
	jabberThreadInfo->send( iq );
}

// Returns the current GMT offset in seconds
int GetGMTOffset(void)
{
	TIME_ZONE_INFORMATION tzinfo;
	int nOffset = 0;

	DWORD dwResult= GetTimeZoneInformation(&tzinfo);

	switch(dwResult) {
	case TIME_ZONE_ID_STANDARD:
		nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
		break;
	case TIME_ZONE_ID_DAYLIGHT:
		nOffset = -(tzinfo.Bias + tzinfo.DaylightBias) * 60;
		break;
	case TIME_ZONE_ID_UNKNOWN:
	case TIME_ZONE_ID_INVALID:
	default:
		nOffset = 0;
		break;
	}

	return nOffset;
}

// entity time (XEP-0202) support
static void JabberProcessIqTime202(TCHAR* idStr, XmlNode* node)
{
	TCHAR* from;
	struct tm *gmt;
	time_t ltime;
	char stime[100];
	char szTZ[10];
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	_tzset();
	time(&ltime);
	gmt = gmtime(&ltime);
	sprintf(stime,"%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ", gmt->tm_year + 1900, gmt->tm_mon, gmt->tm_mday,
		gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

	int nGmtOffset = GetGMTOffset();
	ltime = abs(nGmtOffset);

	gmt = gmtime( &ltime );
	sprintf(szTZ, "%s%.2i:%.2i", nGmtOffset > 0 ? "+" : "-", gmt->tm_hour, gmt->tm_min );

	XmlNodeIq iq( "result", idStr, from );
	XmlNode* timeNode = iq.addChild( "time" );
	timeNode->addAttr( "xmlns", JABBER_FEAT_ENTITY_TIME );
	timeNode->addChild( "utc", stime);
	timeNode->addChild( "tzo", szTZ );
	jabberThreadInfo->send(iq);
}

static void JabberProcessIqTime( TCHAR* idStr, XmlNode* node ) //added by Rion (jep-0090)
{
	TCHAR* from;
	struct tm *gmt;
	time_t ltime;
	char stime[20],*dtime;
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	_tzset();
	time( &ltime );
	gmt = gmtime( &ltime );
	sprintf( stime,"%.4i%.2i%.2iT%.2i:%.2i:%.2i",
		gmt->tm_year+1900, gmt->tm_mon, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec );
	dtime = ctime( &ltime );
	dtime[24]=0;

	XmlNodeIq iq( "result", idStr, from );
	XmlNode* query = iq.addQuery( "jabber:iq:time" );
	query->addChild( "utc", stime ); query->addChild( "tz", _tzname[1] ); query->addChild( "display", dtime );
	jabberThreadInfo->send( iq );
}

static void JabberProcessIqAvatar( TCHAR* idStr, XmlNode* node )
{
	if ( !JGetByte( "EnableAvatars", TRUE ))
		return;

	TCHAR* from;
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL )
		return;

	int pictureType = JGetByte( "AvatarType", PA_FORMAT_UNKNOWN );
	if ( pictureType == PA_FORMAT_UNKNOWN )
		return;

	char* szMimeType;
	switch( pictureType ) {
		case PA_FORMAT_JPEG:	 szMimeType = "image/jpeg";   break;
		case PA_FORMAT_GIF:	 szMimeType = "image/gif";    break;
		case PA_FORMAT_PNG:	 szMimeType = "image/png";    break;
		case PA_FORMAT_BMP:	 szMimeType = "image/bmp";    break;
		default:	return;
	}

	char szFileName[ MAX_PATH ];
	JabberGetAvatarFileName( NULL, szFileName, MAX_PATH );

	FILE* in = fopen( szFileName, "rb" );
	if ( in == NULL )
		return;

	long bytes = filelength( fileno( in ));
	char* buffer = ( char* )mir_alloc( bytes*4/3 + bytes + 1000 );
	if ( buffer == NULL ) {
		fclose( in );
		return;
	}

	fread( buffer, bytes, 1, in );
	fclose( in );

	char* str = JabberBase64Encode( buffer, bytes );
	XmlNodeIq iq( "result", idStr, from );
	XmlNode* query = iq.addQuery( JABBER_FEAT_AVATAR );
	XmlNode* data = query->addChild( "data", str ); data->addAttr( "mimetype", szMimeType );
	jabberThreadInfo->send( iq );
	mir_free( str );
	mir_free( buffer );
}

static void JabberProcessIqResultVersion( TCHAR* type, XmlNode* node, XmlNode* queryNode  )
{
	TCHAR* from = JabberXmlGetAttrValue( node, "from" );
	if ( from == NULL ) return;

	JABBER_LIST_ITEM *item = NULL;

	if (( item = JabberListGetItemPtr( LIST_VCARD_TEMP, from )) == NULL)
		item = JabberListGetItemPtr( LIST_ROSTER, from );

	if ( item == NULL ) return;

	JABBER_RESOURCE_STATUS *r = item->resource;
	if ( r == NULL ) return;

	TCHAR* p = _tcschr( from, '/' );
	if ( p == NULL )    return;
	if ( *++p == '\0' ) return;

	int i;
	for ( i=0; i<item->resourceCount && _tcscmp( r->resourceName, p ); i++, r++ );
	if ( i >= item->resourceCount )
		return;

	HANDLE hContact = JabberHContactFromJID( from );
	if ( hContact == NULL )
		return;

	r->dwVersionRequestTime = -1;

	if ( !lstrcmp( type, _T("error"))) {
		if ( r->resourceName != NULL && !r->szCapsNode && !r->szCapsVer )
			JSetStringT( hContact, "MirVer", r->resourceName );
		return;
	}

	XmlNode* n;
	if ( r->software ) mir_free( r->software );
	if (( n=JabberXmlGetChild( queryNode, "name" ))!=NULL && n->text ) {
		if (( hContact=JabberHContactFromJID( item->jid )) != NULL ) {
			if (( p = _tcsstr( n->text, _T("Miranda IM"))) != NULL )
				JSetStringT( hContact, "MirVer", p );
			else
				JSetStringT( hContact, "MirVer", n->text );
		}
		r->software = mir_tstrdup( n->text );
	}
	else r->software = NULL;
	if ( r->version ) mir_free( r->version );
	if (( n=JabberXmlGetChild( queryNode, "version" ))!=NULL && n->text )
		r->version = mir_tstrdup( n->text );
	else
		r->version = NULL;
	if ( r->system ) mir_free( r->system );
	if (( n=JabberXmlGetChild( queryNode, "os" ))!=NULL && n->text )
		r->system = mir_tstrdup( n->text );
	else
		r->system = NULL;

	JabberCapsBits jcb = JabberGetResourceCapabilites(from);

	if ( hwndJabberInfo != NULL )
		PostMessage( hwndJabberInfo, WM_JABBER_REFRESH, 0, 0);
}

static void JabberProcessIq( XmlNode *node, void *userdata )
{
	ThreadData* info;
	HANDLE hContact;
	XmlNode *queryNode, *siNode, *n;
	TCHAR *from, *type, *jid, *nick;
	TCHAR *xmlns, *profile;
	TCHAR *str, *p, *q;
	TCHAR text[256];
	int i;
	JABBER_IQ_PFUNC pfunc;

	if ( !node->name || strcmp( node->name, "iq" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( type=JabberXmlGetAttrValue( node, "type" )) == NULL ) return;
	from = JabberXmlGetAttrValue( node, "from" );

	int id = JabberGetPacketID( node );
	TCHAR* idStr = JabberXmlGetAttrValue( node, "id" );

	queryNode = JabberXmlGetChild( node, "query" );
	xmlns = JabberXmlGetAttrValue( queryNode, "xmlns" );

	/////////////////////////////////////////////////////////////////////////
	// MATCH BY ID
	/////////////////////////////////////////////////////////////////////////

	if (( pfunc=JabberIqFetchFunc( id )) != NULL ) {
		JabberLog( "Handling iq request for id=%d", id );
		pfunc( node, userdata );
	}

	/////////////////////////////////////////////////////////////////////////
	// MORE GENERAL ROUTINES, WHEN ID DOES NOT MATCH
	/////////////////////////////////////////////////////////////////////////

	else if (( pfunc=JabberIqFetchXmlnsFunc( xmlns )) != NULL ) {
		JabberLog( "Handling iq request for xmlns = " TCHAR_STR_PARAM, xmlns );
		pfunc( node, userdata );
	}

	// RECVED: <iq type='set'><query ...
	else if ( !_tcscmp( type, _T("set")) && queryNode!=NULL && xmlns != NULL ) {

		// RECVED: roster push
		// ACTION: similar to iqIdGetRoster above
		if ( !_tcscmp( xmlns, _T("jabber:iq:roster"))) {
			XmlNode *itemNode;
			JABBER_LIST_ITEM *item;
			TCHAR* name;

			JabberLog( "<iq/> Got roster push, query has %d children", queryNode->numChild );
			for ( i=0; i<queryNode->numChild; i++ ) {
				itemNode = queryNode->child[i];
				if ( strcmp( itemNode->name, "item" ) != 0 )
					continue;
				if (( jid = JabberXmlGetAttrValue( itemNode, "jid" )) == NULL )
					continue;
				if (( str = JabberXmlGetAttrValue( itemNode, "subscription" )) == NULL )
					continue;

				// we will not add new account when subscription=remove
				if ( !_tcscmp( str, _T("to")) || !_tcscmp( str, _T("both")) || !_tcscmp( str, _T("from")) || !_tcscmp( str, _T("none"))) {
					if (( name=JabberXmlGetAttrValue( itemNode, "name" )) != NULL )
						nick = mir_tstrdup( name );
					else
						nick = JabberNickFromJID( jid );

					if ( nick != NULL ) {
						if (( item=JabberListAdd( LIST_ROSTER, jid )) != NULL ) {
							replaceStr( item->nick, nick );

							XmlNode* groupNode = JabberXmlGetChild( itemNode, "group" );
							replaceStr( item->group, ( groupNode ) ? groupNode->text : NULL );

							if (( hContact=JabberHContactFromJID( jid )) == NULL ) {
								// Received roster has a new JID.
								// Add the jid ( with empty resource ) to Miranda contact list.
								hContact = JabberDBCreateContact( jid, nick, FALSE, TRUE );
							}
							else JSetStringT( hContact, "jid", jid );

							if ( name != NULL ) {
								DBVARIANT dbnick;
								if ( !JGetStringT( hContact, "Nick", &dbnick )) {
									if ( _tcscmp( nick, dbnick.ptszVal ) != 0 )
										DBWriteContactSettingTString( hContact, "CList", "MyHandle", nick );
									else 
										DBDeleteContactSetting( hContact, "CList", "MyHandle" );

									JFreeVariant( &dbnick );
								}
								else DBWriteContactSettingTString( hContact, "CList", "MyHandle", nick );
							}
							else DBDeleteContactSetting( hContact, "CList", "MyHandle" );

							if ( item->group != NULL ) {
								JabberContactListCreateGroup( item->group );
								DBWriteContactSettingTString( hContact, "CList", "Group", item->group );
							}
							else DBDeleteContactSetting( hContact, "CList", "Group" );
						}
						else mir_free( nick );
				}	}

				if (( item=JabberListGetItemPtr( LIST_ROSTER, jid )) != NULL ) {
					if ( !_tcscmp( str, _T("both"))) item->subscription = SUB_BOTH;
					else if ( !_tcscmp( str, _T("to"))) item->subscription = SUB_TO;
					else if ( !_tcscmp( str, _T("from"))) item->subscription = SUB_FROM;
					else item->subscription = SUB_NONE;
					JabberLog( "Roster push for jid=" TCHAR_STR_PARAM ", set subscription to " TCHAR_STR_PARAM, jid, str );
					// subscription = remove is to remove from roster list
					// but we will just set the contact to offline and not actually
					// remove, so that history will be retained.
					if ( !_tcscmp( str, _T("remove"))) {
						if (( hContact=JabberHContactFromJID( jid )) != NULL ) {
							if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
								JSetWord( hContact, "Status", ID_STATUS_OFFLINE );
							JabberListRemove( LIST_ROSTER, jid );
					}	}
					else if ( JGetByte( hContact, "ChatRoom", 0 ))
						DBDeleteContactSetting( hContact, "CList", "Hidden" );
			}	}

			if ( hwndJabberAgents )
				SendMessage( hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0 );
		}

		// RECVED: file transfer request
		// ACTION: notify Miranda through CHAINRECV
		else if ( !_tcscmp( xmlns, _T(JABBER_FEAT_OOB))) {
			if (( jid=from)!=NULL && ( n=JabberXmlGetChild( queryNode, "url" ))!=NULL && n->text!=NULL ) {
				str = n->text;	// URL of the file to get
				filetransfer* ft = new filetransfer;
				ft->std.totalFiles = 1;
				ft->jid = mir_tstrdup( jid );
				ft->std.hContact = JabberHContactFromJID( jid );
				ft->type = FT_OOB;
				ft->httpHostName = NULL;
				ft->httpPort = 80;
				ft->httpPath = NULL;
				// Parse the URL
				if ( !_tcsnicmp( str, _T("http://"), 7 )) {
					p = str + 7;
					if (( q = _tcschr( p, '/' )) != NULL ) {
						if ( q-p < SIZEOF( text )) {
							_tcsncpy( text, p, q-p );
							text[q-p] = '\0';
							if (( p = _tcschr( text, ':' )) != NULL ) {
								ft->httpPort = ( WORD )_ttoi( p+1 );
								*p = '\0';
							}
							ft->httpHostName = t2a( text );
							ft->httpPath = t2a( ++q );
				}	}	}

				if ( idStr != NULL )
					ft->iqId = mir_tstrdup( idStr );

				if ( ft->httpHostName && ft->httpPath ) {
					CCSDATA ccs;
					PROTORECVEVENT pre;
					char* szBlob, *desc;

					JabberLog( "Host=%s Port=%d Path=%s", ft->httpHostName, ft->httpPort, ft->httpPath );
					if (( n=JabberXmlGetChild( queryNode, "desc" ))!=NULL && n->text!=NULL )
						desc = t2a( n->text );
					else
						desc = mir_strdup( "" );

					if ( desc != NULL ) {
						char* str;
						JabberLog( "description = %s", desc );
						if (( str = strrchr( ft->httpPath, '/' )) != NULL )
							str++;
						else
							str = ft->httpPath;
						str = mir_strdup( str );
						JabberHttpUrlDecode( str );
						szBlob = ( char* )mir_alloc( sizeof( DWORD )+ strlen( str ) + strlen( desc ) + 2 );
						*(( PDWORD ) szBlob ) = ( DWORD )ft;
						strcpy( szBlob + sizeof( DWORD ), str );
						strcpy( szBlob + sizeof( DWORD )+ strlen( str ) + 1, desc );
						pre.flags = 0;
						pre.timestamp = time( NULL );
						pre.szMessage = szBlob;
						pre.lParam = 0;
						ccs.szProtoService = PSR_FILE;
						ccs.hContact = ft->std.hContact;
						ccs.wParam = 0;
						ccs.lParam = ( LPARAM )&pre;
						JCallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
						mir_free( szBlob );
						mir_free( str );
						mir_free( desc );
					}
				}
				else {
					// reject
					XmlNodeIq iq( "error", idStr, ft->jid );
					XmlNode* e = iq.addChild( "error", "File transfer refused" ); e->addAttr( "code", 406 );
					jabberThreadInfo->send( iq );
					delete ft;
		}	}	}

		// RECVED: bytestream initiation request
		// ACTION: check for any stream negotiation that is pending ( now only file transfer is handled )
		else if ( !_tcscmp( xmlns, _T(JABBER_FEAT_BYTESTREAMS)))
			JabberFtHandleBytestreamRequest( node );
	}
	// RECVED: <iq type='get'><query ...
	else if ( !_tcscmp( type, _T( "get" ))) {
		// RECVED: software version query
		// ACTION: return my software version
		if ( queryNode != NULL && xmlns != NULL ) {
			if ( !_tcscmp( xmlns, _T(JABBER_FEAT_VERSION)))
				JabberProcessIqVersion( idStr, node );
			else if ( !_tcscmp( xmlns, _T(JABBER_FEAT_AVATAR)))
				JabberProcessIqAvatar( idStr, node );
			else if ( !_tcscmp( xmlns, _T("jabber:iq:time")))
				JabberProcessIqTime( idStr, node );	
			else if ( !_tcscmp( xmlns, _T(JABBER_FEAT_LAST_ACTIVITY)))
				JabberProcessIqLast( idStr, node );
		}
		else {
			// entity time (XEP-0202) support
			n = JabberXmlGetChildWithGivenAttrValue( node, "time", "xmlns", _T( JABBER_FEAT_ENTITY_TIME ));
			if ( n )
				JabberProcessIqTime202( idStr, node );
			// // XEP-0199: XMPP Ping support
			n = JabberXmlGetChildWithGivenAttrValue( node, "ping", "xmlns", _T( JABBER_FEAT_PING ));
			if ( n )
				JabberProcessIqPing( idStr, node );
		}
	}
	// RECVED: <iq type='result'><query ...
	else if ( !_tcscmp( type, _T("result")) && queryNode != NULL && xmlns != NULL ) {

		// RECVED: software version result
		// ACTION: update version information for the specified jid/resource
		if ( !_tcscmp( xmlns, _T(JABBER_FEAT_VERSION)))
			JabberProcessIqResultVersion( type, node, queryNode );
	}
	// RECVED: <iq type='set'><open xmlns='http://jabber.org/protocol/ibb'
	else if ( !_tcscmp( type, _T("set")) && ( n=JabberXmlGetChildWithGivenAttrValue( node, "open", "xmlns", _T(JABBER_FEAT_IBB)))!=NULL ) {
		JabberFtHandleIbbRequest( node, TRUE );
	}
	// RECVED: <iq type='set'><close xmlns='http://jabber.org/protocol/ibb'
	else if ( !_tcscmp( type, _T("set")) && ( n=JabberXmlGetChildWithGivenAttrValue( node, "close", "xmlns", _T(JABBER_FEAT_IBB)))!=NULL ) {
		JabberFtHandleIbbRequest( node, FALSE );
	}
	// RECVED: <iq type='set'><data xmlns='http://jabber.org/protocol/ibb'
	else if (!_tcscmp( type, _T("set")) && ( n=JabberXmlGetChildWithGivenAttrValue( node, "data", "xmlns", _T(JABBER_FEAT_IBB)))!=NULL ) {
		BOOL bOk = FALSE;
		TCHAR *sid = JabberXmlGetAttrValue( n, "sid" );
		TCHAR *seq = JabberXmlGetAttrValue( n, "seq" );
		if ( sid && seq && n->text ) {
			bOk = JabberIbbProcessRecvdData( n->text, sid, seq );
		}
		if ( from != NULL ) {
			if ( bOk ) {
				XmlNodeIq iq( "result", idStr, from );
				jabberThreadInfo->send( iq );
			}
			else {
				XmlNodeIq iq( "error", idStr, from );
				XmlNode* e = iq.addChild( "error" ); e->addAttr( "code", 404 ); e->addAttr( "type", _T("cancel"));
				XmlNode* na = e->addChild( "item-not-found" ); na->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
				jabberThreadInfo->send( iq );
	}	}	}
	// RECVED: <iq type='set'><si xmlns='http://jabber.org/protocol/si' ...
	else if ( !_tcscmp( type, _T("set")) && ( siNode=JabberXmlGetChildWithGivenAttrValue( node, "si", "xmlns", _T(JABBER_FEAT_SI)))!=NULL && ( profile=JabberXmlGetAttrValue( siNode, "profile" ))!=NULL ) {

		// RECVED: file transfer request
		// ACTION: notify Miranda throuch CHAINRECV
		if ( !_tcscmp( profile, _T(JABBER_FEAT_SI_FT))) {
			JabberFtHandleSiRequest( node );
		}
		// RECVED: unknown profile
		// ACTION: reply with bad-profile
		else {
			if ( from != NULL ) {
				XmlNodeIq iq( "error", idStr, from );
				XmlNode* error = iq.addChild( "error" ); error->addAttr( "code", "400" ); error->addAttr( "type", "cancel" );
				XmlNode* brq = error->addChild( "bad-request" ); brq->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
				XmlNode* bp = error->addChild( "bad-profile" ); brq->addAttr( "xmlns", JABBER_FEAT_SI );
				jabberThreadInfo->send( iq );
		}	}
	}
	// RECVED: <iq type='error'> ...
	else if ( !_tcscmp( type, _T("error"))) {
		if ( !lstrcmp( xmlns, _T(JABBER_FEAT_VERSION))) {
			JabberProcessIqResultVersion( type, node, queryNode );
			return;
		}
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
}	}	}

static void JabberProcessRegIq( XmlNode *node, void *userdata )
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

ThreadData::ThreadData( JABBER_SESSION_TYPE parType )
{
	memset( this, 0, sizeof( *this ));
	type = parType;
	caps = CAPS_BOOKMARK; // assume that the server supports bookmarks
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
		result = JabberWsRecv( s, buf, len, flags );

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
		return JabberWsSend( s, buffer, bufsize, flags );

	if ( flags != MSG_NODUMP && DBGetContactSettingByte( NULL, "Netlib", "DumpSent", TRUE ) == TRUE ) {
		char* szLogBuffer = ( char* )alloca( bufsize+32 );
		strcpy( szLogBuffer, "( SSL ) Data sent\n" );
		memcpy( szLogBuffer+strlen( szLogBuffer ), buffer, bufsize+1  ); // also copy \0
		Netlib_Logf( hNetlibUser, "%s", szLogBuffer );	// %s to protect against when fmt tokens are in szLogBuffer causing crash
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
