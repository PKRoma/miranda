/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright(C) 2002-2004 George Hazan (modification) and Richard Hughes (original)

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

#include "msn_global.h"

#include <io.h>
#include <direct.h>
#include <process.h>
#include <time.h>

#include "resource.h"

#include "msn_md5.h"

void __cdecl MSNServerThread( ThreadData* info );
void __cdecl MSNSendfileThread( ThreadData* info );
int MSN_Auth8( char* authChallengeInfo, char*& parResult );

void mmdecode(char *trg, char *str);

char *sid;
char *kv;
char *MSPAuth;
unsigned long sl;
char *passport;
char *rru;

extern int uniqueEventId;
extern char sttHeaderStart[];

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_ReceiveMessage - receives message or a file from the server
/////////////////////////////////////////////////////////////////////////////////////////

static int sttDivideWords( char* parBuffer, int parMinItems, char** parDest )
{
	for ( int i=0; i < parMinItems; i++ ) {
		parDest[ i ] = parBuffer;

		int tWordLen = strcspn( parBuffer, " \t" );
		if ( tWordLen == 0 )
			return i;

		parBuffer += tWordLen;
		if ( *parBuffer != '\0' ) {
			int tSpaceLen = strspn( parBuffer, " \t" );
			memset( parBuffer, 0, tSpaceLen );
			parBuffer += tSpaceLen;
	}	}

	return i;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetMyHostAsString - retrieves a host address as a string

int __stdcall MSN_GetMyHostAsString( char* parBuf, int parBufSize )
{
	IN_ADDR  in;
	hostent* myhost;

	if ( msnExternalIP != NULL )
		strncpy( parBuf, msnExternalIP, parBufSize );
	else
		gethostname( parBuf, parBufSize );

	if ( MSN_GetByte( "AutoGetHost", 1 ))
		MSN_SetString( NULL, "YourHost", parBuf );
	else
		MSN_GetStaticString( "YourHost", NULL, parBuf, parBufSize );

	long ipaddrlong = inet_addr( parBuf );
	if ( ipaddrlong != INADDR_NONE )
		in.S_un.S_addr = ipaddrlong;
	else
	{	myhost = gethostbyname( parBuf );
		if ( myhost == NULL ) {
			{	TWinErrorCode tError;
				MSN_ShowError( "Unknown or invalid host name was specified (%s). Error %d: %s.",
					parBuf, tError.mErrorCode, tError.getText());
			}

			return 1;
		}
		memcpy( &in, myhost->h_addr, 4 );
	}

	strncpy( parBuf, inet_ntoa( in ), parBufSize );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Starts a file sending thread

void MSN_ConnectionProc( HANDLE hNewConnection, DWORD dwRemoteIP )
{
	MSN_DebugLog( "File transfer connection accepted" );

	WORD localPort = 0;
	SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, ( WPARAM )hNewConnection, 0 );
	if ( s != INVALID_SOCKET) {
		SOCKADDR_IN saddr;
		int len = sizeof( saddr );
		if ( getsockname( s, ( SOCKADDR* )&saddr, &len ) != SOCKET_ERROR )
			localPort = ntohs( saddr.sin_port );
	}

	if ( localPort != 0 ) {
		ThreadData* T = MSN_GetThreadByPort( localPort );
		if ( T != NULL ) {
			T->s = hNewConnection;
			if ( T->mMsnFtp != NULL ) {
				T->mMsnFtp->mIncomingPort = 0;
				SetEvent( T->mMsnFtp->hWaitEvent );
			}
			else {
				T->mP2pSession->mIncomingPort = 0;
				SetEvent( T->mP2pSession->hWaitEvent );
			}
			return;
		}
		MSN_DebugLog( "There's no registered file transfers for incoming port #%d, connection closed", localPort );
	}
	else MSN_DebugLog( "Unable to determine the local port, file server connection closed." );

	Netlib_CloseHandle( hNewConnection );
}

void sttStartFileSend( ThreadData* info, const char* Invcommand, const char* Invcookie )
{
	if ( strcmpi( Invcommand,"ACCEPT" ))
		return;

	filetransfer* ft = info->mMsnFtp; info->mMsnFtp = NULL;
	bool bHasError = false;
	NETLIBBIND nlb = {0};

	char ipaddr[256];
	if ( MSN_GetMyHostAsString( ipaddr, sizeof ipaddr ))
		bHasError = true;
	else {
		nlb.cbSize = sizeof NETLIBBIND;
		nlb.pfnNewConnection = MSN_ConnectionProc;
		nlb.wPort = 0;	// Use user-specified incoming port ranges, if available
		if (( ft->mIncomingBoundPort = (HANDLE) CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, (LPARAM) &nlb)) == NULL ) {
			MSN_DebugLog( "Unable to bind the port for incoming transfers" );
			bHasError = true;
		}
		else ft->mIncomingPort = nlb.wPort;
	}

	char command[ 1024 ];
	int  nBytes = _snprintf( command, sizeof( command ),
		"MIME-Version: 1.0\r\n"
		"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
		"Invitation-Command: %s\r\n"
		"Invitation-Cookie: %s\r\n"
		"IP-Address: %s\r\n"
		"Port: %i\r\n"
		"AuthCookie: %i\r\n"
		"Launch-Application: FALSE\r\n"
		"Request-Data: IP-Address:\r\n\r\n",
		( bHasError ) ? "CANCEL" : "ACCEPT",
		Invcookie, ipaddr, nlb.wPort, WORD((( double )rand() / ( double )RAND_MAX ) * 4294967295 ));
	info->sendPacket( "MSG", "N %d\r\n%s", nBytes, command );

	if ( bHasError ) delete ft;
	else {
		ft->hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_FILETRANS;
		newThread->mCaller = 2;
		newThread->mTotalSend = 0;
		newThread->mMsnFtp = ft;
		newThread->startThread(( pThreadFunc )MSNSendfileThread );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes e-mail notification

static void sttNotificationMessage( char* msgBody, bool isInitial )
{
	char tBuffer[512];
	char tBuffer2[512];
	bool tIsPopup = ServiceExists( MS_POPUP_ADDPOPUP ) != 0;

	int   IncomingMessages = -1;

	char* passportini = "https://loginnet.passport.com/ppsecure/md5auth.srf?lc=%L";
	char* rruini = "/cgi-bin/HoTMaiL";

	if ( passport ) free( passport );
	passport = strdup( passportini );

	if ( rru ) free( rru );
	rru = strdup( rruini );

	MimeHeaders tFileInfo;
	char* msgFile = tFileInfo.readFromBuffer( msgBody );

	const char* From = tFileInfo[ "From" ];
	const char* Subject = tFileInfo[ "Subject" ];
	const char* Fromaddr = tFileInfo[ "From-Addr" ];
	{
		const char* p;
		if (( p = tFileInfo[ "Inbox-Unread" ] ) != NULL )
			IncomingMessages = atoi( p );

		if (( p = tFileInfo[ "Inbox-URL" ] ) != NULL ) {
			if ( rru ) free( rru );
			rru = strdup( p );
		}

		if (( p = tFileInfo[ "Post-URL" ] ) != NULL && !isInitial ) {
			if ( passport ) free( passport );
			passport = strdup( p );
	}	}

	if ( From != NULL && Subject != NULL && Fromaddr != NULL ) {
		char mimeFrom[ 1024 ], mimeSubject[ 1024 ];
		mmdecode( mimeFrom,    ( char* )From );
		mmdecode( mimeSubject, ( char* )Subject );

		if ( !strcmpi( From, Fromaddr )) {
			if ( tIsPopup ) {
				_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate( "Hotmail from %s" ), mimeFrom );
				_snprintf( tBuffer2, sizeof( tBuffer2 ), MSN_Translate( "Subject: %s" ), mimeSubject );
			}
			else _snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate("A new mail has come from %s (title: %s)."), mimeFrom, mimeSubject );
		}
		else {
			if ( tIsPopup ) {
				_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate("Hotmail from %s (%s)"),mimeFrom, Fromaddr );
				_snprintf( tBuffer2, sizeof( tBuffer2 ), MSN_Translate("Subject: %s"), mimeSubject );
			}
			else _snprintf( tBuffer, sizeof( tBuffer ),  MSN_Translate("A new mail has come from %s (%s) (title: %s)."),mimeFrom, Fromaddr, mimeSubject );
	}	}
	else {
		if ( tIsPopup ) {
			_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate("Hotmail" ));
			_snprintf( tBuffer2, sizeof( tBuffer2 ), MSN_Translate("Unread mail is available." ));
		}
		else _snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate("Unread mail is available." ));
	}

	// Disable to notify receiving hotmail
	if ( !MSN_GetByte( "DisableHotmail", 1 )) {
		if ( IncomingMessages != 0 || !MSN_GetByte( "DisableHotmailJunk", 0 )) {
			SkinPlaySound( mailsoundname );
			if ( tIsPopup )
				MSN_ShowPopup( tBuffer, tBuffer2, MSN_ALLOW_ENTER + MSN_ALLOW_MSGBOX + MSN_HOTMAIL_POPUP );
			else
				MessageBox( NULL, tBuffer, "MSN Protocol", MB_OK | MB_ICONINFORMATION );
	}	}

	if ( !MSN_GetByte( "RunMailerOnHotmail", 0 ))
		return;

	if ( !MSN_GetStaticString( "MailerPath", NULL, tBuffer, sizeof( tBuffer ))) {
		if ( tBuffer[0] ) {
			char* tParams = "";
			char* p = tBuffer;

			if ( *p == '\"' ) {
				char* tEndPtr = strchr( p+1, '\"' );
				if ( tEndPtr != NULL ) {
					*tEndPtr = 0;
					strdel( p, 1 );
					tParams = tEndPtr+1;
					goto LBL_Run;
			}	}

			p = strchr( p+1, ' ' );
			if ( p != NULL ) {
				*p = 0;
				tParams = p+1;
			}
LBL_Run:
			while ( *tParams == ' ' )
				tParams++;

			MSN_DebugLog( "Running mailer \"%s\" with params \"%s\"", tBuffer, tParams );
			ShellExecute( NULL, "open", tBuffer, tParams, NULL, TRUE );
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes various invitations

static void sttInviteMessage( ThreadData* info, char* msgBody, char* email, char* nick )
{
	MimeHeaders tFileInfo;
	char* msgFile = tFileInfo.readFromBuffer( msgBody );

	const char* Appname = tFileInfo[ "Application-Name" ];
	const char* Invcommand = tFileInfo[ "Invitation-Command" ];
	const char* Invcookie = tFileInfo[ "Invitation-Cookie" ];
	const char* Appfile = tFileInfo[ "Application-File" ];
	const char* Appfilesize = tFileInfo[ "Application-FileSize" ];
	const char* IPAddress = tFileInfo[ "IP-Address" ];
	const char* Port = tFileInfo[ "Port" ];
	const char* AuthCookie = tFileInfo[ "AuthCookie" ];
	const char* SessionID = tFileInfo[ "Session-ID" ];
	const char* SessionProtocol = tFileInfo[ "Session-Protocol" ];

	if ( Appname != NULL && Appfile != NULL && Appfilesize != NULL ) { // receive first
		filetransfer* ft = info->mMsnFtp = new filetransfer();

		ft->std.hContact = MSN_HContactFromEmail( email, nick, 1, 1 );
		ft->std.currentFile = strdup( Appfile );
		Utf8Decode( ft->std.currentFile, strlen( Appfile ), &ft->wszFileName );
		ft->fileId = -1;
		ft->std.currentFileSize = atol( Appfilesize );
		ft->std.totalBytes = atol( Appfilesize );
		ft->std.totalFiles = 1;
		ft->szInvcookie = strdup( Invcookie );

		int tFileNameLen = strlen( ft->std.currentFile );
		char tComment[ 40 ];
		int tCommentLen = _snprintf( tComment, sizeof( tComment ), "%ld bytes", ft->std.currentFileSize );
		char* szBlob = ( char* )malloc( sizeof( DWORD ) + tFileNameLen + tCommentLen + 2 );
		*( PDWORD )szBlob = ( DWORD )ft;
		strcpy( szBlob + sizeof( DWORD ), ft->std.currentFile );
		strcpy( szBlob + sizeof( DWORD ) + tFileNameLen + 1, tComment );

		PROTORECVEVENT pre;
		pre.flags = 0;
		pre.timestamp = ( DWORD )time( NULL );
		pre.szMessage = ( char* )szBlob;
		pre.lParam = ( LPARAM )( char* )Invcookie;

		CCSDATA ccs;
		ccs.hContact = MSN_HContactFromEmail( email, nick, 1, 1 );
		ccs.szProtoService = PSR_FILE;
		ccs.wParam = 0;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		return;
	}

	if ( IPAddress != NULL && Port != NULL && AuthCookie != NULL ) { // receive Second
		ThreadData* newThread = new ThreadData;
		strcpy( newThread->mServer, IPAddress );
		strcat( newThread->mServer, ":" );
		strcat( newThread->mServer, Port );
		newThread->mType = SERVER_FILETRANS;
		newThread->mMsnFtp = info->mMsnFtp; info->mMsnFtp = NULL;
		strcpy( newThread->mCookie, AuthCookie );

		MSN_DebugLog( "Connecting to '%s'...", newThread->mServer );
		newThread->startThread(( pThreadFunc )MSNServerThread );
		return;
	}

	if ( Invcommand != NULL && Invcookie != NULL && Port == NULL && AuthCookie == NULL && SessionID == NULL ) { // send 1
		sttStartFileSend( info, Invcommand, Invcookie );
		return;
	}

	if ( Appname == NULL && SessionID != NULL && SessionProtocol != NULL ) { // netmeeting send 1
		if ( !strcmpi( Invcommand,"ACCEPT" )) {
			char ipaddr[256];
			MSN_GetMyHostAsString( ipaddr, sizeof( ipaddr ));

			ShellExecute(NULL, "open", "conf.exe", NULL, NULL, SW_SHOW);
			Sleep(3000);

			char command[ 1024 ];
			int  nBytes = _snprintf( command, sizeof( command ),
				"MIME-Version: 1.0\r\n"
				"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
				"Invitation-Command: ACCEPT\r\n"
				"Invitation-Cookie: %s\r\n"
				"Session-ID: {1A879604-D1B8-11D7-9066-0003FF431510}\r\n"
				"Launch-Application: TRUE\r\n"
				"IP-Address: %s\r\n\r\n",
				Invcookie, ipaddr);
			info->sendPacket( "MSG", "N %d\r\n%s", nBytes, command );
		}
		return;
	}

	if ( Appname != NULL && !strcmpi( Appname,"NetMeeting" )) { // netmeeting receive 1
		char command[ 1024 ];
		int nBytes;

		_snprintf( command, sizeof( command ), "Accept NetMeeting request from %s?", email );

		if ( MessageBox( NULL, command, "MSN Protocol", MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
			char ipaddr[256];
			MSN_GetMyHostAsString( ipaddr, sizeof( ipaddr ));

			nBytes = _snprintf( command, sizeof( command ),
				"MIME-Version: 1.0\r\n"
				"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
				"Invitation-Command: ACCEPT\r\n"
				"Invitation-Cookie: %s\r\n"
				"Session-ID: {A2ED5ACF-F784-4B47-A7D4-997CD8F643CC}\r\n"
				"Session-Protocol: SM1\r\n"
				"Launch-Application: TRUE\r\n"
				"Request-Data: IP-Address:\r\n"
				"IP-Address: %s\r\n\r\n",
				Invcookie, ipaddr);
		}
		else {
			nBytes = _snprintf( command, sizeof( command ),
				"MIME-Version: 1.0\r\n"
				"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
				"Invitation-Command: CANCEL\r\n"
				"Invitation-Cookie: %s\r\n"
				"Cancel-Code: REJECT\r\n\r\n",
				Invcookie);
		}
		info->sendPacket( "MSG", "N %d\r\n%s", nBytes, command );
		return;
	}

	if ( IPAddress != NULL && Port == NULL && SessionID != NULL && SessionProtocol == NULL ) { // netmeeting receive 2
		char	ipaddr[256];
		_snprintf( ipaddr, sizeof( ipaddr ), "callto://%s", IPAddress);
		ShellExecute(NULL, "open", ipaddr, NULL, NULL, SW_SHOW);
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes any received MSG

void MSN_ReceiveMessage( ThreadData* info, char* cmdString, char* params )
{
	union {
		char* tWords[ 3 ];
		struct { char *fromEmail, *fromNick, *strMsgBytes; } data;
	};

	if ( sttDivideWords( params, 3, tWords ) != 3 ) {
		MSN_DebugLog( "Invalid %.3s command, ignoring", cmdString );
		return;
	}

	int msgBytes = atol( data.strMsgBytes );

	UrlDecode( data.fromEmail ); UrlDecode( data.fromNick ); Utf8Decode( data.fromNick );

	char* msg = ( char* )alloca( msgBytes+1 );
	int bytesFromData = min( info->mBytesInData, msgBytes );
	memcpy( msg, info->mData, bytesFromData );
	info->mBytesInData -= bytesFromData;
	memmove( info->mData, info->mData + bytesFromData, info->mBytesInData );

	while ( bytesFromData < msgBytes ) {
		int recvResult;
		recvResult = MSN_WS_Recv( info->s, msg + bytesFromData, msgBytes - bytesFromData );
		if ( !recvResult )
			return;

		bytesFromData += recvResult;
	}

	msg[ msgBytes ] = 0;
	MSN_DebugLog( "Message:\n%s", msg );

	MimeHeaders tHeader;
	char* msgBody = ( char* )tHeader.readFromBuffer( msg );

	//message from the server (probably)
	if (( strchr( data.fromEmail, '@' ) == NULL ) && strcmpi( data.fromEmail, "Hotmail" ))
		return;

	char* tContentType = NULL;
	{
		for ( int i=0; i < tHeader.mCount; i++ )
			if ( !strcmpi( tHeader.mVals[ i ].name,"Content-Type" ))
				tContentType = tHeader.mVals[ i ].value;
	}

	if ( tContentType == NULL )
		return;

	if ( !strnicmp( tContentType, "text/plain", 10 )) {
		CCSDATA ccs;
		HANDLE tContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 1, 1 );

		wchar_t* tRealBody = NULL;
		int      tRealBodyLen = 0;
		if ( strstr( tContentType, "charset=UTF-8" ))
		{	Utf8Decode( msgBody, 0, &tRealBody );
			tRealBodyLen = wcslen( tRealBody );
		}
		int tMsgBodyLen = strlen( msgBody );

		char* tPrefix = NULL;
		int   tPrefixLen = 0;

		if ( info->mJoinedCount > 1 && info->mJoinedContacts != NULL && info->mJoinedContacts[0] != tContact ) {
			for ( int j=1; j < info->mJoinedCount; j++ ) {
				if ( info->mJoinedContacts[j] == tContact ) {
					char* tNickName = MSN_GetContactName( info->mJoinedContacts[j] );
					tPrefixLen = strlen( tNickName )+2;
					tPrefix = ( char* )alloca( tPrefixLen+1 );
					strcpy( tPrefix, "<" );
					strcat( tPrefix, tNickName );
					strcat( tPrefix, "> " );
					ccs.hContact = info->mJoinedContacts[ 0 ];
					break;
			}	}
		}
		else ccs.hContact = tContact;

		int tMsgBufLen = tMsgBodyLen+1 + (tRealBodyLen+1)*sizeof( wchar_t ) + tPrefixLen*(sizeof( wchar_t )+1);
		char* tMsgBuf = ( char* )alloca( tMsgBufLen );
		char* p = tMsgBuf;

		if ( tPrefixLen != 0 ) {
			memcpy( p, tPrefix, tPrefixLen );
			p += tPrefixLen;
		}
		strcpy( p, msgBody );
		p += tMsgBodyLen+1;

		if ( tPrefixLen != 0 ) {
			MultiByteToWideChar( CP_ACP, 0, tPrefix, tPrefixLen, ( wchar_t* )p, tPrefixLen );
			p += tPrefixLen*sizeof( wchar_t );
		}
		if ( tRealBodyLen != 0 ) {
			memcpy( p, tRealBody, sizeof( wchar_t )*( tRealBodyLen+1 ));
			free( tRealBody );
		}

		PROTORECVEVENT pre;
		pre.szMessage = ( char* )tMsgBuf;
		pre.flags = PREF_UNICODE;
		pre.timestamp = ( DWORD )time(NULL);
		pre.lParam = 0;

		ccs.szProtoService = PSR_MESSAGE;
		ccs.wParam = 0;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		return;
	}

	if ( !strnicmp( tContentType, "text/x-msmsgsprofile", 20 )) {
		MimeHeaders tFileInfo;
		char* msgFile = tFileInfo.readFromBuffer( msg );

		for ( int j=0, sidLen=0, kvLen=0, MSPAuthLen=0; j < tFileInfo.mCount; j++ ) {
			MimeHeader& H = tFileInfo.mVals[ j ];

			if ( !strcmpi( H.name, "sid" )) {
				if ( sid ) free( sid );
				sid = strdup( H.value );
			}
			else if ( !strcmpi( H.name, "kv" )) {
				if ( kv ) free( kv );
				kv = strdup( H.value );
			}
			else if ( !strcmpi( H.name, "MSPAuth" )) {
				if ( MSPAuth ) free( MSPAuth );
				MSPAuth = strdup( H.value );
			}
			else if ( !strcmpi( H.name, "ClientIP" )) {
				if ( msnExternalIP ) free( msnExternalIP );
				msnExternalIP = strdup( H.value );

				if ( MSN_GetByte( "AutoGetHost", 1 ))
					MSN_SetString( NULL, "YourHost", H.value );
		}	}

		return;
	}

	if ( !strnicmp( tContentType, "text/x-msmsgscontrol", 20 )) {
		MimeHeaders tFileInfo;
		tFileInfo.readFromBuffer( msg );

		char* tTypingUser = NULL;

		for ( int j=0; j < tFileInfo.mCount; j++ )
			if ( !strcmpi( tFileInfo.mVals[ j ].name, "TypingUser" ))
				tTypingUser = tFileInfo.mVals[ j ].value;

		if ( tTypingUser != NULL ) {
			char userNick[ 388 ];
			strcpy( userNick, tTypingUser );

			HANDLE hContact = MSN_HContactFromEmail( tTypingUser, userNick, 1, 0 );
			if ( hContact != NULL )
				strcpy( userNick, MSN_GetContactName( hContact ));

			MSN_CallService( MS_PROTO_CONTACTISTYPING, WPARAM( hContact ), 3 );

			if ( MSN_GetByte( "DisplayTyping", 0 ))
				MSN_ShowPopup( userNick, MSN_Translate( "typing..." ), 0 );
		}

		return;
	}

	if ( !strnicmp( tContentType,"text/x-msmsgsemailnotification", 30 ))
		sttNotificationMessage( msgBody, false );
	else if ( !strnicmp( tContentType, "text/x-msmsgsinitialemailnotification", 37 ))
		sttNotificationMessage( msgBody, true );
	else if ( !strnicmp( tContentType, "text/x-msmsgsinvite", 19 ))
		sttInviteMessage( info, msgBody, data.fromEmail, data.fromNick );
	else if ( !strnicmp( tContentType, "application/x-msnmsgrp2p", 24 ))
		p2p_processMsg( info, msgBody );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Process user addition

HANDLE sttProcessAdd( int trid, int listId, char* userEmail, char* userNick )
{
	HANDLE hContact = NULL;

	if ( trid == msnSearchID ) {
		MSN_SendPacket( msnNSSocket, "REM", "BL %s", userEmail );

		PROTOSEARCHRESULT isr;
		memset( &isr, 0, sizeof( isr ));
		isr.cbSize = sizeof( isr );
		isr.nick = userNick;
		isr.email = userEmail;
		MSN_SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE )msnSearchID, ( LPARAM )&isr );
		MSN_SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE )msnSearchID, 0 );

		msnSearchID = -1;
	}
	else {
		UrlDecode( userEmail ); UrlDecode( userNick );
		if ( !IsValidListCode( listId ))
			return NULL;

		Utf8Decode( userNick );
		int mask = Lists_Add( listId, userEmail, userNick );

		hContact = MSN_HContactFromEmail( userEmail, userNick, 1, 1 );
		if ( listId == LIST_RL && ( mask & ( LIST_FL+LIST_AL+LIST_BL )) == 0 )
			MSN_AddAuthRequest( hContact, userEmail, userNick );
	}

	return hContact;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_HandleCommands - process commands from the server
/////////////////////////////////////////////////////////////////////////////////////////

static bool sttIsSync = false;
static int  sttListNumber = 0;

int MSN_HandleCommands( ThreadData* info, char* cmdString )
{
	char* params = "";
	int trid = -1;
	sttIsSync = false;

	if ( cmdString[3] ) {
		if ( isdigit( cmdString[ 4 ] )) {
			trid = strtol( cmdString+4, &params, 10 );
			switch ( *params ) {
				case ' ':	case '\0':	case '\t':	case '\n':
					while ( *params == ' ' || *params == '\t' )
						params++;
					break;

				default: params = cmdString+4;
		}	}
		else params = cmdString+4;
	}

	switch(( *( PDWORD )cmdString & 0x00FFFFFF ) | 0x20000000 )
	{
		case ' KCA':    //********* ACK: section 8.7 Instant Messages
			if ( info->mJoinedCount > 0 )
				MSN_SendBroadcast( info->mJoinedContacts[0], ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )trid, 0 );
			break;

		case ' CDA':	// ADC - MSN v10 addition command
		{
			char* tWords[ 10 ];
			char *userNick = NULL, *userEmail = NULL, *userID = NULL;
			int nTerms = sttDivideWords( params, 10, tWords );
			if ( nTerms < 2 )
				goto LBL_InvalidCommand;

			for ( int i=1; i < nTerms; i++ ) {
				char* p = tWords[ i ];
				if ( *p == 'F' && p[1] == '=' )
					userNick = p+2;
				else if ( *p == 'N' && p[1] == '=' )
					userEmail = p+2;
				else if ( *p == 'C' && p[1] == '=' )
					userID = p+2;
			}

			if ( userEmail == NULL ) goto LBL_InvalidCommand;
			if ( userNick == NULL ) userNick = userEmail;

			int listId = Lists_NameToCode( tWords[0] );
			HANDLE hContact = sttProcessAdd( trid, listId, userEmail, userNick );
			if ( hContact != NULL && userID != NULL )
				MSN_SetString( hContact, "ID", userID );
			break;
		}
		case ' DDA':    //********* ADD: section 7.8 List Modifications
		{
			union {
				char* tWords[ 4 ];
				struct { char *list, *serial, *userEmail, *userNick; } data;
			};

			if ( sttDivideWords( params, 4, tWords ) != 4 ) {
LBL_InvalidCommand:
				MSN_DebugLog( "Invalid %.3s command, ignoring", cmdString );
				break;
			}

			sttProcessAdd( trid, Lists_NameToCode( data.list ), data.userEmail, data.userNick );
			break;
		}
		case ' SNA':    //********* ANS: section 8.4 Getting Invited to a Switchboard Session
			break;

		case ' PLB':    //********* BLP: section 7.6 List Retrieval And Property Management
		{
			union {
				char* tWords[ 2 ];
				struct { char *junk, *listName; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) == 1 )
				data.listName = data.junk;

			msnOtherContactsBlocked = stricmp( data.listName, "BL" ) == 0;
			break;
		}
		case ' RPB':	//********* BPR:
			break;

		case ' EYB':   //********* BYE: section 8.5 Session Participant Changes
		{
			union {
				char* tWords[ 2 ];
				struct { char *userEmail, *junk; } data;
			};

			sttDivideWords( params, 2, tWords );
			UrlDecode( data.userEmail );
			MSN_DebugLog( "Contact left channel: %s", data.userEmail );

			// nobody left in, we might as well leave too
			if ( MSN_ContactLeft( info, MSN_HContactFromEmail( data.userEmail, NULL, 0, 0 )) == 0 )
				info->sendPacket( "OUT", NULL );
			break;
		}
		case ' LAC':    //********* CAL: section 8.3 Inviting Users to a Switchboard Session
			break;

		case ' GHC':    //********* CHG: section 7.7 Client States
		{
			int oldMode = msnStatusMode;
			msnStatusMode = MSNStatusToMiranda( params );

			MSN_SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,( HANDLE )oldMode, msnStatusMode );
			MSN_DebugLog( "Status change acknowledged: %s", params );
			break;
		}
		case ' LHC':    //********* CHL: Query from Server on MSNP7
		{
			char authChallengeInfo[ 30 ];
			if ( sscanf( params, "%30s", authChallengeInfo ) < 1 )
				goto LBL_InvalidCommand;

			//fill chal info
			char* chalinfo = (char*)alloca( strlen( authChallengeInfo )+16+4 );
			strcpy( chalinfo, authChallengeInfo );
			strcat( chalinfo, msnProtChallenge );

			long challen = strlen( chalinfo );

			//Digest it
			DWORD dig[ 4 ];
			MD5_CTX context;
			MD5Init(&context);
			MD5Update(&context, ( BYTE* )chalinfo, challen );
			MD5Final(( BYTE* )dig, &context);

			info->sendPacket( "QRY", "%s 32\r\n%08x%08x%08x%08x", msnProductID,
				htonl( dig[0] ), htonl( dig[1] ), htonl( dig[2] ), htonl( dig[3] ));   // response for the server
			break;
		}
		case ' RVC':    //********* CVR: MSNP8
		{
			char tEmail[ MSN_MAX_EMAIL_LEN ];
			MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail ));
			info->sendPacket( "USR", "TWN I %s", tEmail );
			break;
		}
		case ' NLF':    //********* FLN: section 7.9 Notification Messages
		{	HANDLE hContact;
			UrlDecode( params );
			if (( hContact = MSN_HContactFromEmail( params, NULL, 0, 0 )) != NULL )
				MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE );
			break;
		}
		case ' CTG':    //********* GTC: section 7.6 List Retrieval And Property Management
			break;

		case ' FNI':	//********* INF: section 7.2 Server Policy Information
		{
			char security1[ 10 ];
			//can be more security packages on the end, comma delimited
			if ( sscanf( params, "%9s", security1 ) < 1 )
				goto LBL_InvalidCommand;

			//SEND USR I packet, section 7.3 Authentication
			if ( !strcmp( security1, "MD5" )) {
				char tEmail[ MSN_MAX_EMAIL_LEN ];
				if ( !MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail )))
					info->sendPacket( "USR", "MD5 I %s", tEmail );
				else
					info->sendPacket( "USR", "MD5 I " );   //this will fail, of course
			}
			else {
				MSN_DebugLog( "Unknown security package '%s'", security1 );
				if ( info->mType == SERVER_NOTIFICATION || info->mType == SERVER_DISPATCH ) {
					MSN_SendBroadcast(NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPROTOCOL);
					MSN_GoOffline();
				}
				return 1;
			}
			break;
		}
		case ' NLI':
		case ' NLN':    //********* ILN/NLN: section 7.9 Notification Messages
		{
			union {
				char* tWords[ 5 ];
				struct { char *userStatus, *userEmail, *userNick, *objid, *cmdstring; } data;
			};

			int tArgs = sttDivideWords( params, 5, tWords );
			if ( tArgs < 3 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userEmail ); UrlDecode( data.userNick ); Utf8Decode( data.userNick );

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, 0, 0 );
			if ( hContact != NULL) {
				// is there an uninitialized switchboard for this contact?
				ThreadData* T = MSN_GetUnconnectedThread( hContact );
				if ( T != NULL )
					if ( hContact == T->mInitialContact )
						T->sendPacket( "CAL", "%s", data.userEmail );

				MSN_SetString( hContact, "Nick", data.userNick );
				MSN_SetWord( hContact, "Status", ( WORD )MSNStatusToMiranda( data.userStatus ));
			}

			if ( tArgs > 3 && tArgs <= 5 ) {
				UrlDecode( data.cmdstring );
				MSN_SetDword( hContact, "FlagBits", atol( data.objid ));

				if ( data.cmdstring[0] ) {
					MSN_SetString( hContact, "PictContext", data.cmdstring );
					if ( hContact != NULL ) {
						char szSavedContext[ 256 ];
						int result = MSN_GetStaticString( "PictSavedContext", hContact, szSavedContext, sizeof szSavedContext );
						if ( result || strcmp( szSavedContext, data.cmdstring ))
							MSN_SendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
				}	}
				else {
					DBDeleteContactSetting( hContact, msnProtocolName, "PictContext" );
					DBDeleteContactSetting( hContact, msnProtocolName, "PictSavedContext" );
					MSN_SendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, NULL );
			}	}

			break;
		}
		case ' ORI':    //********* IRO: section 8.4 Getting Invited to a Switchboard Session
		{
			union {
				char* tWords[ 4 ];
				struct { char *strThisContact, *totalContacts, *userEmail, *userNick; } data;
			};

			if ( sttDivideWords( params, 4, tWords ) != 4 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userEmail );
			UrlDecode( data.userNick ); Utf8Decode( data.userNick );

			MSN_ContactJoined( info, MSN_HContactFromEmail( data.userEmail, data.userNick, 1, 1 ));

			int thisContact = atol( data.strThisContact );
			if ( thisContact != 1 ) {
				char* tContactName = MSN_GetContactName( info->mJoinedContacts[0] );

				char multichatmsg[256];
				_snprintf(
					multichatmsg, sizeof( multichatmsg ),
					MSN_Translate( "%s (%s) has joined the chat with %s" ),
					data.userNick, data.userEmail, tContactName );

				MSN_ShowPopup( tContactName, multichatmsg, MSN_ALLOW_MSGBOX );
			}
			break;
		}
		case ' IOJ':    //********* JOI: section 8.5 Session Participant Changes
		{
			union {
				char* tWords[ 2 ];
				struct { char *userEmail, *userNick; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) != 2 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userEmail ); UrlDecode( data.userNick ); Utf8Decode( data.userNick );

			MSN_DebugLog( "New contact in channel %s %s", data.userEmail, data.userNick );
			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, 1, 1 );

			info->mInitialContact = NULL;
			info->mMessageCount = 0;

			if ( MSN_ContactJoined( info, hContact ) == 1 ) {
				MsgQueueEntry E;
				int tFound = MsgQueue_GetNext( hContact, E );
				if ( tFound != 0 ) {
					do {
						if ( E.msgSize == 0 )
							info->sendMessage( E.message, 0 );
						else
							info->sendRawMessage(( E.msgSize == -1 ) ? 'N' : 'D', E.message, E.msgSize );

						MSN_SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )E.seq, 0 );
						free( E.message );

						if ( E.ft != NULL ) {
							info->mMsnFtp = E.ft;
							info->mMsnFtp->mOwnsThread = true;
						}
					}
						while (( tFound = MsgQueue_GetNext( hContact, E )) != 0 );

					if ( MSN_GetByte( "EnableDeliveryPopup", 1 ))
						MSN_ShowPopup( MSN_GetContactName( hContact ), MSN_Translate( "First message delivered" ), 0 );
			}	}
			else {
				char* tContactName = MSN_GetContactName( info->mJoinedContacts[0] );

				char multichatmsg[ 256 ];
				_snprintf(
					multichatmsg, sizeof( multichatmsg ),
					MSN_Translate( "%s (%s) has joined the chat with %s" ),
					data.userNick, data.userEmail, tContactName );

				MSN_ShowPopup( tContactName, multichatmsg, MSN_ALLOW_MSGBOX );
			}
			return 0;
		}
		case ' GSL':	//********* LSG: something strange ;)
			break;

		case ' TSL':	//********* LST: section 7.6 List Retrieval And Property Management
		{
			int	listId;
			char *userEmail = NULL, *userNick = NULL, *userId = NULL;

			union	{
				char* tWords[ 10 ];
				struct { char *userEmail, *userNick, *list, *smth; } data;
			};

			int tNumTokens = sttDivideWords( params, 10, tWords );

			if ( --sttListNumber == 0 )
				MSN_SetServerStatus( msnDesiredStatus );

			if ( msnProtVersion == 10 ) {
				for ( int i=0; i < tNumTokens; i++ ) {
					char* p = tWords[ i ];
					if ( *p == 'N' && p[1] == '=' )
						userEmail = p+2;
					else if ( *p == 'F' && p[1] == '=' )
						userNick = p+2;
					else if ( *p == 'C' && p[1] == '=' )
						userId = p+2;
					else {
						listId = atol( p );
						break;
				}	}

				if ( userEmail == NULL )
					goto LBL_InvalidCommand;

				if ( userNick == NULL )
					userNick = userEmail;
			}
			else
			{
				switch ( tNumTokens ) {
				case 3:	case 4:	break;
				default:
					goto LBL_InvalidCommand;
				}

				listId = atol( data.list );
				userEmail = data.userEmail;
				userNick = data.userNick;
			}

			UrlDecode( userEmail ); UrlDecode( userNick );
			Utf8Decode( userNick );

			if ( !IsValidListCode( listId ) || !strcmp( userEmail, "messenger@microsoft.com" ))
				break;

			Lists_Add( listId, userEmail, userNick );

			// add user if it wasn't included into a contact list
			HANDLE hContact = MSN_HContactFromEmail( userEmail, userNick, 1, 0 );

			if (( listId & ( LIST_AL +  LIST_BL + LIST_FL )) == LIST_BL ) {
				DBDeleteContactSetting( hContact, "CList", "NotOnList" );
				DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
			}

			if ( msnProtVersion == 10 ) {
				if ( listId & LIST_PL ) {
					if ( !Lists_IsInList( LIST_RL, userEmail )) {
						MSN_AddUser( hContact, userEmail, LIST_PL + LIST_REMOVE );
						MSN_AddUser( hContact, userEmail, LIST_RL );
					}

					if (( listId & ( LIST_AL +  LIST_BL + LIST_FL )) == 0 )
						MSN_AddAuthRequest( hContact, userEmail, userNick );
				}
			}
			else {
				if (( listId & LIST_RL ) && !Lists_IsInList( LIST_AL, userEmail ) && !Lists_IsInList( LIST_BL, userEmail ))
					if ( MSN_GetByte( "EnableRlAnalyze", FALSE ))
						MSN_AddAuthRequest( hContact, userEmail, userNick );
			}

			if ( listId & ( LIST_BL | LIST_AL )) {
				hContact = MSN_HContactFromEmail( userEmail, userNick, 0, 0 );
				if ( hContact != NULL ) {
					WORD tApparentMode = MSN_GetWord( hContact, "ApparentMode", 0 );
					if (( listId & LIST_BL ) && tApparentMode == 0 )
						MSN_SetWord( hContact, "ApparentMode", ID_STATUS_OFFLINE );
					else if (( listId & LIST_AL ) && tApparentMode != 0 )
						MSN_SetWord( hContact, "ApparentMode", 0 );
			}	}

			if ( hContact != NULL && userId != NULL )
				MSN_SetString( hContact, "ID", userId );
			break;
		}
		case ' GSM':   //********* MSG: sections 8.7 Instant Messages, 8.8 Receiving an Instant Message
			MSN_ReceiveMessage( info, cmdString, params );
			break;

		case ' KAN':   //********* NAK: section 8.7 Instant Messages
			MSN_DebugLog( "Message send failed (trid=%d)", trid );
			break;

		case ' TUO':   //********* OUT: sections 7.10 Connection Close, 8.6 Leaving a Switchboard Session
			if ( !stricmp( params, "OTH" )) {
				MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION );
				MSN_DebugLog( "You have been disconnected from the MSN server because you logged on from another location using the same MSN passport." );		
			}

			return 1;

		case ' PRP':	//********* PRP: user property
		{
			union {
				char* tWords[ 2 ];
				struct { char *name, *value; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) != 2 )
				goto LBL_InvalidCommand;

			UrlDecode( data.value ); Utf8Decode( data.value );
			if ( !stricmp( data.name, "MFN" ))
				if ( !sttIsSync || !MSN_GetByte( "NeverUpdateNickname", 0 ))
					MSN_SetString( NULL, "Nick", data.value );
			break;
		}
		case ' YRQ':   //********* QRY:
			break;

		case ' GNQ':	//********* QNG: reply to PNG
			if ( msnGetInfoContact != NULL ) {
				MSN_SendBroadcast( msnGetInfoContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, ( HANDLE )1, 0 );
				msnGetInfoContact = NULL;
			}
			break;

		case ' AER':	//********* REA: get a nickname
		{
			union {
				char* tWords[ 3 ];
				struct { char *id, *userEmail, *userNick; } data;
			};

			if ( sttDivideWords( params, 3, tWords ) != 3 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userEmail );
			UrlDecode( data.userNick ); Utf8Decode( data.userNick );

			char tEmail[ MSN_MAX_EMAIL_LEN ];
			MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail ));
			if ( stricmp( data.userEmail, tEmail ) == 0 )
				MSN_SetString( NULL, "Nick", data.userNick );
			break;
		}
		case ' MER':   //********* REM: section 7.8 List Modifications
		{
			union {
				char* tWords[ 3 ];
				struct { char *list, *serial, *userEmail; } data;
			};

			if ( msnProtVersion < 10 ) {
				if ( sttDivideWords( params, 3, tWords ) != 3 )
					goto LBL_InvalidCommand;
			}
			else {
				if ( sttDivideWords( params, 3, tWords ) == 2 )
					data.userEmail = data.serial;
			}

			UrlDecode( data.userEmail );

			int listId = Lists_NameToCode( data.list );
			if ( IsValidListCode( listId ))
				Lists_Remove( listId, data.userEmail );
			break;
		}
		case ' GNR':    //********* RNG: section 8.4 Getting Invited to a Switchboard Session
			//note: unusual message encoding: trid==sessionid
		{
			union {
				char* tWords[ 5 ];
				struct { char *newServer, *security, *authChallengeInfo, *callerEmail, *callerNick; } data;
			};

			if ( sttDivideWords( params, 5, tWords ) != 5 )
				goto LBL_InvalidCommand;

			UrlDecode( data.newServer ); UrlDecode( data.callerEmail );
			UrlDecode( data.callerNick ); Utf8Decode( data.callerNick );

			if ( strcmp( data.security, "CKI" )) {
				MSN_DebugLog( "Unknown security package in RNG command: %s", data.security );
				break;
			}

			ThreadData* newThread = new ThreadData;
			strcpy( newThread->mServer, data.newServer );
			newThread->mType = SERVER_SWITCHBOARD;
			MSN_ContactJoined( newThread, MSN_HContactFromEmail( data.callerEmail, data.callerNick, 1, 1 ));
			_snprintf( newThread->mCookie, sizeof( newThread->mCookie ), "%s %d", data.authChallengeInfo, trid );

			MSN_DebugLog( "Opening caller's switchboard server '%s'...", data.newServer );
			newThread->startThread(( pThreadFunc )MSNServerThread );
			break;
		}
		case ' NYS':    //********* SYN: section 7.5 Client User Property Synchronization
		{
			char* tWords[ 4 ];
			int nRequiredWords = ( msnProtVersion == 9 ) ? 3 : 4;
			if ( sttDivideWords( params, nRequiredWords, tWords ) != nRequiredWords )
				goto LBL_InvalidCommand;

			Lists_Wipe();
			sttIsSync = true;
			sttListNumber = atol( tWords[ nRequiredWords-2 ] );
			break;
		}
		case ' RSU':	//********* USR: sections 7.3 Authentication, 8.2 Switchboard Connections and Authentication
			if ( info->mType == SERVER_SWITCHBOARD ) { //(section 8.2)
				union {
					char* tWords[ 3 ];
					struct { char *status, *userHandle, *friendlyName; } data;
				};

				if ( sttDivideWords( params, 3, tWords ) != 3 )
					goto LBL_InvalidCommand;

				UrlDecode( data.userHandle ); UrlDecode( data.friendlyName );
				Utf8Decode( data.friendlyName );

				if ( strcmp( data.status, "OK" )) {
					MSN_DebugLog( "Unknown status to USR command (SB): '%s'", data.status );
					break;
				}

				HANDLE hContact = MsgQueue_GetNextRecipient();
				if ( hContact == NULL ) { //can happen if both parties send first message at the same time
					MSN_DebugLog( "USR (SB) internal: thread created for no reason" );
					info->sendPacket( "OUT", NULL );
					break;
				}

				char tEmail[ MSN_MAX_EMAIL_LEN ];
				if ( MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail ))) {
					MSN_DebugLog( "USR (SB) internal: Contact is not MSN" );
					info->sendPacket( "OUT", NULL );
					break;
				}

				info->mInitialContact = hContact;
				info->sendPacket( "CAL", "%s", tEmail );
			}
			else 	   //dispatch or notification server (section 7.3)
			{
				union {
					char* tWords[ 3 ];
					struct { char *security, *sequence, *authChallengeInfo; } data;
				};

				if ( sttDivideWords( params, 3, tWords ) != 3 )
					goto LBL_InvalidCommand;

				if ( !strcmp( data.security, "TWN" )) {
					char* tAuth;
					if ( MSN_Auth8( data.authChallengeInfo, tAuth )) {
						MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
							MSN_GoOffline();
						return 1;
					}

					info->sendPacket( "USR", "TWN S %s", tAuth );
					free( tAuth );
				}
				else if ( !strcmp( data.security, "MD5" )) {
					if ( *data.sequence == 'S' ) {
						//send Md5 pass
						char *chalinfo;

						DBVARIANT dbv;
						if ( !DBGetContactSetting( NULL, msnProtocolName, "Password", &dbv )) {
							MSN_CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
							//fill chal info
							chalinfo = ( char* )alloca( strlen( data.authChallengeInfo) + strlen( dbv.pszVal ) + 4 );
							strcpy( chalinfo, data.authChallengeInfo );
							strcat( chalinfo, dbv.pszVal );
							MSN_FreeVariant( &dbv );
						}
						else chalinfo = "xxxxxxxxxx";

						//Digest it
						unsigned char digest[16];
						MD5_CTX context;
						MD5Init( &context );
						MD5Update( &context, ( BYTE* )chalinfo, strlen( chalinfo ));
						MD5Final( digest, &context );

						info->sendPacket( "USR", "MD5 S %08x%08x%08x%08x", htonl(*(PDWORD)(digest+0)),htonl(*(PDWORD)(digest+4)),htonl(*(PDWORD)(digest+8)),htonl(*(PDWORD)(digest+12)));
					}
					else MSN_DebugLog( "Unknown security sequence code '%c', ignoring command", data.sequence );
				}
				else if ( !strcmp( data.security, "OK" )) {
					UrlDecode( tWords[1] ); UrlDecode( tWords[2] ); Utf8Decode( tWords[2] );

					MSN_SetByte( "EnableRlAnalyze", TRUE );

					if ( MSN_GetByte( "NeverUpdateNickname", 0 )) {
						char tNick[ 130 ];
						MSN_GetStaticString( "Nick", NULL, tNick, sizeof tNick );
						MSN_SendNickname( tWords[1], tNick );

						MSN_DebugLog( "Logged in as '%s', name is '%s'", tWords[1], tNick );
					}
					else {
						if ( msnProtVersion < 10 )
							MSN_SetString( NULL, "Nick", tWords[2] );
						MSN_DebugLog( "Logged in as '%s', name is '%s'", tWords[1], tWords[2] );
					}

					sttListNumber = 0;
					if ( msnProtVersion == 10 ) {
						char tOldDate[ 100 ], tNewDate[ 100 ];

						if ( MSN_GetStaticString( "LastSyncTime", NULL, tOldDate, sizeof tOldDate ))
							strcpy( tOldDate, "2004-07-01T00:00:00.0000000-07:00" );

						SYSTEMTIME T;
						GetSystemTime( &T );
						_snprintf( tNewDate, sizeof tNewDate, "%04d-%02d-%02dT%02d:%02d:%02d.0000000-07:00",
							T.wYear, T.wMonth, T.wDay, T.wHour, T.wMinute, T.wSecond );
						MSN_SetString( NULL, "LastSyncTime", tNewDate );

						info->sendPacket( "SYN", "%s %s", tNewDate, tOldDate );
					}
					else info->sendPacket( "SYN", "0" );
				}
				else {
					MSN_DebugLog( "Unknown security package '%s'", data.security );

					if ( info->mType == SERVER_NOTIFICATION || info->mType == SERVER_DISPATCH ) {
						MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPROTOCOL );
						MSN_GoOffline();
					}

					return 1;
			}	}
			break;

		case ' REV':	//******** VER: section 7.1 Protocol Versioning
		{
			char protocol1[ 7 ];
			if ( sscanf( params, "%6s", protocol1 ) < 1 )
				goto LBL_InvalidCommand;

			if ( !strcmp( protocol1, "MSNP10" ))
				msnProtVersion = 10;
			else if ( !strcmp( protocol1, "MSNP9" ))
				msnProtVersion = 9;
			else {
				MSN_ShowError( "Server has requested an unknown protocol set (%s)", params );

				if ( info->mType == SERVER_NOTIFICATION || info->mType == SERVER_DISPATCH ) {
					MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPROTOCOL );
					MSN_GoOffline();
				}

				return 1;
			}

			char tEmail[ MSN_MAX_EMAIL_LEN ];
			if ( MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail ))) {
				MSN_ShowError( "You must specify your e-mail in Options/Network/MSN" );
				return 1;
			}

			info->sendPacket( "CVR","0x0409 winnt 5.1 i386 MSNMSGR 6.2.0137 MSMSGS %s", tEmail );

			msnProtChallenge = "QHDCY@7R1TB6W?5B";
			msnProductID = "PROD0058#7IL2{QD";
			break;
		}
		case ' RFX':    //******** XFR: sections 7.4 Referral, 8.1 Referral to Switchboard
		{
			union {
				char* tWords[ 4 ];
				struct { char *type, *newServer, *security, *authChallengeInfo; } data;
			};

			if ( sttDivideWords( params, 4, tWords ) < 2 )
				goto LBL_InvalidCommand;

			if ( !strcmp( data.type, "NS" )) { //notification server
				UrlDecode( data.newServer );
				ThreadData* newThread = new ThreadData;
				strcpy( newThread->mServer, data.newServer );
				newThread->mType = SERVER_NOTIFICATION;
				newThread->mTrid = info->mTrid;

				MSN_DebugLog( "Switching to notification server '%s'...", data.newServer );
				newThread->startThread(( pThreadFunc )MSNServerThread );
				sl = time(NULL); //for hotmail
				return 1;  //kill the old thread
			}

			if ( !strcmp( data.type, "SB" )) { //switchboard server
				UrlDecode( data.newServer );

				if ( strcmp( data.security, "CKI" )) {
					MSN_DebugLog( "Unknown XFR SB security package '%s'", data.security );
					break;
				}

				ThreadData* newThread = new ThreadData;
				strcpy( newThread->mServer, data.newServer );
				newThread->mType = SERVER_SWITCHBOARD;
				newThread->mCaller = 1;
				strcpy( newThread->mCookie, data.authChallengeInfo );

				MSN_DebugLog( "Opening switchboard server '%s'...", data.newServer );
				newThread->startThread(( pThreadFunc )MSNServerThread );
			}
			else MSN_DebugLog( "Unknown referral server: %s", data.type );
			break;
		}

		default:
			MSN_DebugLog( "Unrecognised message: %s", cmdString );
			break;
	}

	info->mMessageCount++;
	return 0;
}
