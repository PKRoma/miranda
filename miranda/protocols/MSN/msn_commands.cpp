/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

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

void __cdecl MSNNudgeThread( ThreadData* info );
void __cdecl MSNServerThread( ThreadData* info );
void __cdecl MSNSendfileThread( ThreadData* info );

int MSN_GetPassportAuth( char* authChallengeInfo, char*& parResult );

void mmdecode(char *trg, char *str);

void MSN_ChatStart(ThreadData* info);

char* sid = NULL;
char* kv = NULL;
char* MSPAuth = NULL;
char* passport = NULL;
char* rru = NULL;
extern HANDLE	 hMSNNudge;

int msnPingTimeout = 50;

unsigned long sl;

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_ReceiveMessage - receives message or a file from the server
/////////////////////////////////////////////////////////////////////////////////////////

static int sttDivideWords( char* parBuffer, int parMinItems, char** parDest )
{
	int i;
	for ( i=0; i < parMinItems; i++ ) {
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

/////////////////////////////////////////////////////////////////////////////////////////
// Processes e-mail notification

static void sttNotificationMessage( const char* msgBody, bool isInitial )
{
	char tBuffer[512];
	char tBuffer2[512];
	bool tIsPopup = ServiceExists( MS_POPUP_ADDPOPUP ) != 0;
	int  UnreadMessages = -1, UnreadJunkEmails = -1;

	replaceStr( passport, "https://loginnet.passport.com/ppsecure/md5auth.srf?lc=1033" );
	replaceStr( rru, "/cgi-bin/HoTMaiL" );

	MimeHeaders tFileInfo;
	tFileInfo.readFromBuffer( msgBody );

	const char* From = tFileInfo[ "From" ];
	const char* Subject = tFileInfo[ "Subject" ];
	const char* Fromaddr = tFileInfo[ "From-Addr" ];
	{
		const char* p;
		if (( p = tFileInfo[ "Inbox-Unread" ] ) != NULL )
			UnreadMessages = atoi( p );
		if (( p = tFileInfo[ "Folders-Unread" ] ) != NULL )
			UnreadJunkEmails = atoi( p );
	}
	replaceStr( rru,      tFileInfo[ "Inbox-URL" ] );
//	replaceStr( passport, tFileInfo[ "Post-URL" ]  );

	if ( From != NULL && Subject != NULL && Fromaddr != NULL ) {
		char mimeFrom[ 1024 ], mimeSubject[ 1024 ];
		mmdecode( mimeFrom,    ( char* )From );
		mmdecode( mimeSubject, ( char* )Subject );

		if ( !strcmpi( From, Fromaddr )) {
			if ( tIsPopup ) {
				mir_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate( "Hotmail from %s" ), mimeFrom );
				mir_snprintf( tBuffer2, sizeof( tBuffer2 ), MSN_Translate( "Subject: %s" ), mimeSubject );
			}
			else mir_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate("A new mail has come from %s (title: %s)."), mimeFrom, mimeSubject );
		}
		else {
			if ( tIsPopup ) {
				mir_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate("Hotmail from %s (%s)"),mimeFrom, Fromaddr );
				mir_snprintf( tBuffer2, sizeof( tBuffer2 ), MSN_Translate("Subject: %s"), mimeSubject );
			}
			else mir_snprintf( tBuffer, sizeof( tBuffer ),  MSN_Translate("A new mail has come from %s (%s) (title: %s)."),mimeFrom, Fromaddr, mimeSubject );
	}	}
	else {
		const char* MailData = tFileInfo[ "Mail-Data" ];
		if ( MailData != NULL ) {
			const char* p = strstr( MailData, "<IU>" );
			if ( p != NULL )
				UnreadMessages = atoi( p+4 );
			if (( p = strstr( MailData, "<OU>" )) != NULL )
				UnreadJunkEmails = atoi( p+4 );
		}

		// nothing to do, a fake notification
		if ( UnreadMessages == 0 && UnreadJunkEmails == 0 )
			return;

		char* dest;
		if ( tIsPopup ) {
			mir_snprintf( tBuffer, sizeof( tBuffer ), MSN_Translate( "Hotmail" ));
			dest = tBuffer2;
		}
		else dest = tBuffer;
		mir_snprintf( dest, sizeof( tBuffer ), MSN_Translate( "Unread mail is available: %d messages (%d junk e-mails)." ),
			UnreadMessages, UnreadJunkEmails );
	}

	// Disable to notify receiving hotmail
	if ( !MSN_GetByte( "DisableHotmail", 1 )) {
		if ( UnreadMessages != 0 || !MSN_GetByte( "DisableHotmailJunk", 0 )) {
			SkinPlaySound( mailsoundname );
			if ( tIsPopup )
				MSN_ShowPopup( tBuffer, tBuffer2, MSN_ALLOW_ENTER + MSN_ALLOW_MSGBOX + MSN_HOTMAIL_POPUP );
			else
				MessageBoxA( NULL, tBuffer, "MSN Protocol", MB_OK | MB_ICONINFORMATION );
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
			ShellExecuteA( NULL, "open", tBuffer, tParams, NULL, TRUE );
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes various invitations

static void sttInviteMessage( ThreadData* info, const char* msgBody, char* email, char* nick )
{
	MimeHeaders tFileInfo;
	tFileInfo.readFromBuffer( msgBody );

	const char* Appname = tFileInfo[ "Application-Name" ];
	const char* AppGUID = tFileInfo[ "Application-GUID" ];
	const char* Invcommand = tFileInfo[ "Invitation-Command" ];
	const char* Invcookie = tFileInfo[ "Invitation-Cookie" ];
	const char* Appfile = tFileInfo[ "Application-File" ];
	const char* Appfilesize = tFileInfo[ "Application-FileSize" ];
	const char* IPAddress = tFileInfo[ "IP-Address" ];
	const char* Port = tFileInfo[ "Port" ];
	const char* AuthCookie = tFileInfo[ "AuthCookie" ];
	const char* SessionID = tFileInfo[ "Session-ID" ];
	const char* SessionProtocol = tFileInfo[ "Session-Protocol" ];

	if ( AppGUID != NULL ) {
		if ( !strcmp( AppGUID, "{02D3C01F-BF30-4825-A83A-DE7AF41648AA}" )) {
			MSN_ShowPopup( MSN_GetContactName( info->mJoinedContacts[0] ),
				MSN_Translate( "Contact tried to open an audio conference (currently not supported)" ), MSN_ALLOW_MSGBOX );
			return;
	}	}

	if ( Appname != NULL && Appfile != NULL && Appfilesize != NULL ) { // receive first
		filetransfer* ft = info->mMsnFtp = new filetransfer();

		ft->mThreadId = info->mUniqueID;
		ft->std.hContact = MSN_HContactFromEmail( email, nick, 1, 1 );
		replaceStr( ft->std.currentFile, Appfile );
		Utf8Decode( ft->std.currentFile, &ft->wszFileName );
		ft->fileId = -1;
		ft->std.currentFileSize = atol( Appfilesize );
		ft->std.totalBytes = atol( Appfilesize );
		ft->std.totalFiles = 1;
		ft->szInvcookie = strdup( Invcookie );

		int tFileNameLen = strlen( ft->std.currentFile );
		char tComment[ 40 ];
		int tCommentLen = mir_snprintf( tComment, sizeof( tComment ), "%ld bytes", ft->std.currentFileSize );
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
		ft_startFileSend( info, Invcommand, Invcookie );
		return;
	}

	if ( Appname == NULL && SessionID != NULL && SessionProtocol != NULL ) { // netmeeting send 1
		if ( !strcmpi( Invcommand,"ACCEPT" )) {
			char ipaddr[256];
			MSN_GetMyHostAsString( ipaddr, sizeof( ipaddr ));

			ShellExecuteA(NULL, "open", "conf.exe", NULL, NULL, SW_SHOW);
			Sleep(3000);

			char command[ 1024 ];
			int  nBytes = mir_snprintf( command, sizeof( command ),
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

		mir_snprintf( command, sizeof( command ), "Accept NetMeeting request from %s?", email );

		if ( MessageBoxA( NULL, command, "MSN Protocol", MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
			char ipaddr[256];
			MSN_GetMyHostAsString( ipaddr, sizeof( ipaddr ));

			nBytes = mir_snprintf( command, sizeof( command ),
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
			nBytes = mir_snprintf( command, sizeof( command ),
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
		mir_snprintf( ipaddr, sizeof( ipaddr ), "callto://%s", IPAddress);
		ShellExecuteA(NULL, "open", ipaddr, NULL, NULL, SW_SHOW);
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

	UrlDecode( data.fromEmail ); UrlDecode( data.fromNick );

	char* msg = ( char* )alloca( msgBytes+1 );

	int bytesFromData = min( info->mBytesInData, msgBytes );
	memcpy( msg, info->mData, bytesFromData );
	info->mBytesInData -= bytesFromData;
	memmove( info->mData, info->mData + bytesFromData, info->mBytesInData );

	while ( bytesFromData < msgBytes ) {
		int recvResult;
		recvResult = info->recv( msg + bytesFromData, msgBytes - bytesFromData );
		if ( recvResult <= 0 )
			return;

		bytesFromData += recvResult;
	}

	msg[ msgBytes ] = 0;
	MSN_DebugLog( "Message:\n%s", msg );

	MimeHeaders tHeader;
	const char* msgBody = tHeader.readFromBuffer( msg );

	// message from the server (probably)
	if (( strchr( data.fromEmail, '@' ) == NULL ) && strcmpi( data.fromEmail, "Hotmail" ))
		return;

	const char* tContentType = tHeader[ "Content-Type" ];
	if ( tContentType == NULL )
		return;

	if ( !strnicmp( tContentType, "text/plain", 10 )) {
		CCSDATA ccs;
		HANDLE tContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 1, 1 );

		wchar_t* tRealBody = NULL;
		int      tRealBodyLen = 0;
		if ( strstr( tContentType, "charset=UTF-8" ))
		{	Utf8Decode(( char* )msgBody, &tRealBody );
			tRealBodyLen = wcslen( tRealBody );
		}
		int tMsgBodyLen = strlen( msgBody );

		char* tPrefix = NULL;
		int   tPrefixLen = 0;

		if ( info->mJoinedCount > 1 && info->mJoinedContacts != NULL ) {
			if ( msnHaveChatDll )
				MSN_ChatStart( info );
			else if ( info->mJoinedContacts[0] != tContact ) {
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
			}	}	}
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

		if ( info->mChatID[0] ) {
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			gcd.pszModule = msnProtocolName;
			gcd.pszID = info->mChatID;
			gcd.iType = GC_EVENT_MESSAGE;
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			gce.pszUID = data.fromEmail;
			gce.pszNick = MSN_GetContactName(MSN_HContactFromEmail(data.fromEmail, NULL, 1, 1));
			gce.time = time(NULL);
			gce.bIsMe = FALSE;
			gce.pszText = (char*)EscapeChatTags(tMsgBuf);
			//gce.pszText = tMsgBuf;
			gce.bAddToLog = TRUE;
			MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);
			free(( void* )gce.pszText);
		}
		else {
			PROTORECVEVENT pre;
			pre.szMessage = ( char* )tMsgBuf;
			pre.flags = PREF_UNICODE;
			pre.timestamp = ( DWORD )time(NULL);
			pre.lParam = 0;

			ccs.szProtoService = PSR_MESSAGE;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		}
		return;
	}

	if ( !strnicmp( tContentType, "text/x-msmsgsprofile", 20 )) {
		MimeHeaders tFileInfo;
		tFileInfo.readFromBuffer( msg );
		replaceStr( sid,           tFileInfo[ "sid" ]      );
		replaceStr( kv,            tFileInfo[ "kv" ]       );
		replaceStr( MSPAuth,       tFileInfo[ "MSPAuth" ]  );
		replaceStr( msnExternalIP, tFileInfo[ "ClientIP" ] );

		if ( msnExternalIP != NULL && MSN_GetByte( "AutoGetHost", 1 ))
			MSN_SetString( NULL, "YourHost", msnExternalIP );

		return;
	}

	if ( !strnicmp( tContentType, "text/x-msmsgscontrol", 20 )) {
		MimeHeaders tFileInfo;
		tFileInfo.readFromBuffer( msg );

		char* tTypingUser = NULL;

		for ( int j=0; j < tFileInfo.mCount; j++ )
			if ( !strcmpi( tFileInfo.mVals[ j ].name, "TypingUser" ))
				tTypingUser = tFileInfo.mVals[ j ].value;

		if ( tTypingUser != NULL && info->mChatID[0] == 0 ) {
			char userNick[ 388 ];
			strcpy( userNick, tTypingUser );

			HANDLE hContact = MSN_HContactFromEmail( tTypingUser, userNick, 1, 0 );
			if ( hContact != NULL )
				strcpy( userNick, MSN_GetContactName( hContact ));

			MSN_CallService( MS_PROTO_CONTACTISTYPING, WPARAM( hContact ), 5 );

			if ( MSN_GetByte( "DisplayTyping", 0 ))
				MSN_ShowPopup( userNick, MSN_Translate( "typing..." ), 0 );
		}

		return;
	}

	//WIZZ
	if ( !strnicmp( tContentType, "text/x-msnmsgr-datacast", 23 )) {
		CCSDATA ccs;
		HANDLE tContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 1, 1 );

		wchar_t* tRealBody = NULL;
		int      tRealBodyLen = 0;
		if ( strstr( tContentType, "charset=UTF-8" ))
		{	Utf8Decode(( char* )msgBody, &tRealBody );
			tRealBodyLen = wcslen( tRealBody );
		}
		int tMsgBodyLen = strlen( msgBody );

		char* tPrefix = NULL;
		int   tPrefixLen = 0;

		if ( info->mJoinedCount > 1 && info->mJoinedContacts != NULL ) {
			if ( msnHaveChatDll )
				MSN_ChatStart( info );
			else if ( info->mJoinedContacts[0] != tContact ) {
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
			}	}	}
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

		if( !strnicmp(tMsgBuf,"ID: 1",5))
			NotifyEventHooks(hMSNNudge,(WPARAM) tContact,0);
		return;
	}

	if ( !strnicmp( tContentType,"text/x-msmsgsemailnotification", 30 ))
		sttNotificationMessage( msgBody, false );
	else if ( !strnicmp( tContentType, "text/x-msmsgsinitialemailnotification", 37 ))
		sttNotificationMessage( msgBody, true );
	else if ( !strnicmp( tContentType, "text/x-msmsgsinitialmdatanotification", 37 ))
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
	if ( trid == msnSearchID ) {
		msnNsThread->sendPacket( "REM", "BL %s", userEmail );

		PROTOSEARCHRESULT isr;
		memset( &isr, 0, sizeof( isr ));
		isr.cbSize = sizeof( isr );
		isr.nick = userNick;
		isr.email = userEmail;
		MSN_SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE )msnSearchID, ( LPARAM )&isr );
		MSN_SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE )msnSearchID, 0 );

		msnSearchID = -1;
		return NULL;
	}

	UrlDecode( userEmail ); UrlDecode( userNick );
	if ( !IsValidListCode( listId ))
		return NULL;

	HANDLE hContact = MSN_HContactFromEmail( userEmail, userNick, 1, 1 );
	Utf8Decode( userNick );
	int mask = Lists_Add( listId, userEmail, userNick );

	if ( listId == LIST_RL && ( mask & ( LIST_FL+LIST_AL+LIST_BL )) == 0 )
		MSN_AddAuthRequest( hContact, userEmail, userNick );

	return hContact;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_HandleCommands - process commands from the server
/////////////////////////////////////////////////////////////////////////////////////////

static bool		sttIsSync = false;
static int		sttListNumber = 0;
static HANDLE	sttListedContact = NULL;
static long		sttListedContactMask;

static void sttDeleteUnusedSetting( long mask, const char* settingName )
{	if (( sttListedContactMask & mask ) == 0 )
		DBDeleteContactSetting( sttListedContact, msnProtocolName, settingName );
}

static void sttProcessListedContactMask()
{
	if ( sttListedContact == NULL )
		return;

	sttDeleteUnusedSetting( 0x0001, "Phone" );
	sttDeleteUnusedSetting( 0x0002, "CompanyPhone" );
	sttDeleteUnusedSetting( 0x0004, "Cellular" );
	sttDeleteUnusedSetting( 0x0008, "OnMobile" );
	sttDeleteUnusedSetting( 0x0010, "OnMsnMobile" );
}

static bool sttAddGroup( char* params, bool isFromBoot )
{
	union {
		char* tWords[ 2 ];
		struct { char *grpName, *grpId; } data;
	};

	if ( sttDivideWords( params, 2, tWords ) != 2 )
		return false;

	UrlDecode( data.grpName );
	MSN_AddGroup( data.grpName, data.grpId );
	if ( hGroupAddEvent != NULL )
		SetEvent( hGroupAddEvent );

	int i;
	char str[ 10 ];

	for ( i=0; true; i++ ) {
		ltoa( i, str, 10 );

		DBVARIANT dbv;
		if ( DBGetContactSettingStringUtf( NULL, "CListGroups", str, &dbv ))
			break;

		bool result = !stricmp( dbv.pszVal+1, data.grpName );
		MSN_FreeVariant( &dbv );
		if ( result ) {
			MSN_SetGroupNumber( data.grpId, i );
			return true;
	}	}

	if ( isFromBoot ) {
		MSN_SetGroupNumber( data.grpId, i );

		if ( MyOptions.ManageServer ) {
			char szNewName[ 128 ];
			mir_snprintf( szNewName, sizeof szNewName, "%c%s",  1 | GROUPF_EXPANDED, data.grpName );
			DBWriteContactSettingStringUtf( NULL, "CListGroups", str, szNewName );
			CallService( MS_CLUI_GROUPADDED, i, 0 );
	}	}
	return true;
}

static void sttSwapInt64( LONGLONG* parValue )
{
	BYTE* p = ( BYTE* )parValue;
	for ( int i=0; i < 4; i++ ) {
		BYTE temp = p[i];
		p[i] = p[7-i];
		p[7-i] = temp;
}	}

int MSN_HandleCommands( ThreadData* info, char* cmdString )
{
	char* params = "";
	int trid = -1;
	sttIsSync = false;

	if ( cmdString[3] ) {
		if ( isdigit(( BYTE )cmdString[ 4 ] )) {
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
	MSN_DebugLog("%S", cmdString);

	switch(( *( PDWORD )cmdString & 0x00FFFFFF ) | 0x20000000 )
	{
		case ' KCA':    //********* ACK: section 8.7 Instant Messages
			if ( info->mP2PInitTrid == trid ) {
				info->mP2PInitTrid = 0;
				p2p_sendViaServer( info->mP2pSession, info );
				info->mP2pSession = NULL;
			}
			else if ( info->mJoinedCount > 0 && MyOptions.SlowSend )
				MSN_SendBroadcast( info->mJoinedContacts[0], ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )trid, 0 );
			break;

		case ' CDA':	// ADC - MSN v10 addition command
		{
			char* tWords[ 10 ];
			char *userNick = NULL, *userEmail = NULL, *userId = NULL, *groupId = NULL;
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
					userId = p+2;
				else
					groupId = p;
			}

			HANDLE hContact;
			if ( userEmail == NULL ) {
				if ( userId == NULL || groupId == NULL )
					goto LBL_InvalidCommand;
				hContact = MSN_HContactById( userId );
			}
			else {
				if ( userNick == NULL )
					userNick = userEmail;

				int listId = Lists_NameToCode( tWords[0] );
				hContact = sttProcessAdd( trid, listId, userEmail, userNick );
			}

			if ( hContact != NULL ) {
				if ( userId  != NULL ) MSN_SetString( hContact, "ID", userId );
				if ( groupId != NULL ) MSN_SetString( hContact, "GroupID", groupId );
			}
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
		case ' GDA':    //********* ADG: group addition
			if ( !sttAddGroup( params, false ))
				goto LBL_InvalidCommand;
			break;

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
		{
			char* tWords[ 2 ];
			if ( sttDivideWords( params, 2, tWords ) != 2 )
				goto LBL_InvalidCommand;

			if ( sttListedContact != NULL ) {
				UrlDecode( tWords[1] );
				if ( !strcmp( tWords[0], "PHH" )) {
					MSN_SetString( sttListedContact, "Phone", tWords[1] );
					sttListedContactMask |= 0x0001;
				}
				else if ( !strcmp( tWords[0], "PHW" )) {
					MSN_SetString( sttListedContact, "CompanyPhone", tWords[1] );
					sttListedContactMask |= 0x0002;
				}
				else if ( !strcmp( tWords[0], "PHM" )) {
					MSN_SetString( sttListedContact, "Cellular", tWords[1] );
					sttListedContactMask |= 0x0004;
				}
				else if ( !strcmp( tWords[0], "MOB" )) {
					MSN_SetString( sttListedContact, "OnMobile", tWords[1] );
					sttListedContactMask |= 0x0008;
				}
				else if ( !strcmp( tWords[0], "MBE" )) {
					MSN_SetString( sttListedContact, "OnMsnMobile", tWords[1] );
					sttListedContactMask |= 0x0010;
			}	}
			break;
		}
		case ' EYB':   //********* BYE: section 8.5 Session Participant Changes
		{
			union {
				char* tWords[ 2 ];
				// modified for chat, orginally param2 = junk
				// param 2: quit due to idle = "1", normal quit = nothing
				struct { char *userEmail, *isIdle; } data;
			};

			sttDivideWords( params, 2, tWords );
			UrlDecode( data.userEmail );

			if ( MSN_GetByte( "EnableSessionPopup", 0 ))
				MSN_ShowPopup( data.userEmail, MSN_Translate( "Contact left channel" ), 0 );

			// modified for chat
			if ( msnHaveChatDll ) {
				GCDEST gcd = {0};
				gcd.pszModule = msnProtocolName;
				gcd.pszID = info->mChatID;
				gcd.iType = GC_EVENT_QUIT;

				GCEVENT gce = {0};
				gce.cbSize = sizeof( GCEVENT );
				gce.pDest = &gcd;
				gce.pszNick = MSN_GetContactName( MSN_HContactFromEmail( data.userEmail, NULL, 1, 1 ));
				gce.pszUID = data.userEmail;
				gce.time = time( NULL );
				gce.bIsMe = FALSE;
				gce.bAddToLog = TRUE;
				MSN_CallService( MS_GC_EVENT, NULL, ( LPARAM )&gce );
			}

			// in here, the first contact is the chat ID, starting from the second will be actual contact
			// if only 1 person left in conversation
			int personleft = MSN_ContactLeft( info, MSN_HContactFromEmail( data.userEmail, NULL, 0, 0 ));
			// see if the session is quit due to idleness
			if ( personleft == 1 && !lstrcmpA( data.isIdle, "1" ) ) {
				GCDEST gcd = {0};
				gcd.pszModule = msnProtocolName;
				gcd.pszID = info->mChatID;
				gcd.iType = GC_EVENT_INFORMATION;

				GCEVENT gce = {0};
				gce.cbSize = sizeof( GCEVENT );
				gce.pDest = &gcd;
				gce.bIsMe = FALSE;
				gce.time = time(NULL);
				gce.bAddToLog = TRUE;
				gce.pszText = Translate("This conversation has been inactive, participants will be removed.");
				MSN_CallService( MS_GC_EVENT, NULL, ( LPARAM )&gce );
				gce.pszText = Translate("To resume the conversation, please quit this session and start a new chat session.");
				MSN_CallService( MS_GC_EVENT, NULL, ( LPARAM )&gce );
			}
			else if ( personleft == 2 && lstrcmpA( data.isIdle, "1" ) ) {
				if ( MessageBoxA( NULL, Translate( "There is only 1 person left in the chat, do you want to switch back to standard message window?"), Translate("MSN Chat"), MB_YESNO|MB_ICONQUESTION) == IDYES) {
					// kill chat dlg and open srmm dialog
					GCEVENT gce = {0};
					GCDEST gcd = {0};
					gce.cbSize = sizeof( GCEVENT );
					gce.pDest = &gcd;

					// a flag to let the kill function know what to do
					// if the value is 1, then it'll open up the srmm window
					info->mJoinedCount--;

					gcd.pszModule = msnProtocolName;
					gcd.pszID = info->mChatID;
					gcd.iType = GC_EVENT_CONTROL;
					MSN_CallService( MS_GC_EVENT, WINDOW_OFFLINE, ( LPARAM )&gce );
					MSN_CallService( MS_GC_EVENT, WINDOW_TERMINATE, ( LPARAM )&gce );
			}	}
			// this is not in chat session, quit the session when everyone left
			else if ( personleft == 0 )
				return 1; // die

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

			//Digest it
			DWORD md5hash[ 4 ];
			MD5_CTX context;
			MD5Init(&context);
			MD5Update(&context, ( BYTE* )authChallengeInfo, strlen( authChallengeInfo ));
			MD5Update(&context, ( BYTE* )msnProtChallenge,  strlen( msnProtChallenge  ));
			MD5Final(( BYTE* )md5hash, &context);

			if ( MyOptions.UseMSNP11 ) {
		      LONGLONG hash1 = *( LONGLONG* )&md5hash[0], hash2 = *( LONGLONG* )&md5hash[2];
				size_t i;
				for ( i=0; i < 4; i++ )
					md5hash[i] &= 0x7FFFFFFF;

				char chlString[128];
				_snprintf( chlString, sizeof chlString, "%s%s00000000", authChallengeInfo, msnProductID );
				chlString[ (strlen(authChallengeInfo)+strlen(msnProductID)+7) & 0xF8 ] = 0;

				LONGLONG high=0, low=0, bskey=0;
				int* chlStringArray = ( int* )chlString;
				for ( i=0; i < strlen( chlString )/4; i += 2) {
					LONGLONG temp = chlStringArray[i];

					temp = (0x0E79A9C1 * temp) % 0x7FFFFFFF;
					temp += high;
					temp = md5hash[0] * temp + md5hash[1];
					temp = temp % 0x7FFFFFFF;

					high = chlStringArray[i + 1];
					high = (high + temp) % 0x7FFFFFFF;
					high = md5hash[2] * high + md5hash[3];
					high = high % 0x7FFFFFFF;

					low = low + high + temp;
				}
				high = (high + md5hash[1]) % 0x7FFFFFFF;
				low = (low + md5hash[3]) % 0x7FFFFFFF;

				LONGLONG key = (low << 32) + high;
				sttSwapInt64( &key );
				sttSwapInt64( &hash1 );
				sttSwapInt64( &hash2 );

				info->sendPacket( "QRY", "%s 32\r\n%016I64x%016I64x", msnProductID, hash1 ^ key, hash2 ^ key );
			}
			else info->sendPacket( "QRY", "%s 32\r\n%08x%08x%08x%08x", msnProductID,
				htonl( md5hash[0] ), htonl( md5hash[1] ), htonl( md5hash[2] ), htonl( md5hash[3] ));
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

			UrlDecode( data.userEmail ); UrlDecode( data.userNick );

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, NULL, 0, 0 );
			if ( hContact != NULL) {
				// is there an uninitialized switchboard for this contact?
				ThreadData* T = MSN_GetUnconnectedThread( hContact );
				if ( T != NULL )
					if ( hContact == T->mInitialContact )
						T->sendPacket( "CAL", "%s", data.userEmail );

				MSN_SetStringUtf( hContact, "Nick", data.userNick );
				MSN_SetWord( hContact, "Status", ( WORD )MSNStatusToMiranda( data.userStatus ));
			}

			MSN_SetString( hContact, "MirVer", "" );

			if ( tArgs > 3 && tArgs <= 5 ) {
				UrlDecode( data.cmdstring );
				DWORD dwValue = ( DWORD )atol( data.objid );
				MSN_SetDword( hContact, "FlagBits", dwValue );
				if ( dwValue & 0x200 )
					MSN_SetString( hContact, "MirVer", "Webmessenger" );

				if ( data.cmdstring[0] ) {
					int temp_status = MSN_GetWord(hContact, "Status", ID_STATUS_OFFLINE);
					if (temp_status == (WORD)ID_STATUS_OFFLINE)
						MSN_SetWord( hContact, "Status", (WORD)ID_STATUS_INVISIBLE);
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
			UrlDecode( data.userNick );

			MSN_ContactJoined( info, MSN_HContactFromEmail( data.userEmail, data.userNick, 1, 1 ));

			int thisContact = atol( data.strThisContact );
			if ( thisContact != 1 ) {
				char* tContactName = MSN_GetContactName( info->mJoinedContacts[0] );
				 Utf8Decode( data.userNick );

				char multichatmsg[256];
				mir_snprintf( multichatmsg, sizeof( multichatmsg ),
					MSN_Translate( "%s (%s) has joined the chat with %s" ),
					data.userNick, data.userEmail, tContactName );

				MSN_ShowPopup( tContactName, multichatmsg, MSN_ALLOW_MSGBOX );
			}

			// only start the chat session after all the IRO messages has been recieved
			if ( msnHaveChatDll && info->mJoinedCount > 1 && !lstrcmpA(data.strThisContact, data.totalContacts) ) {
				if ( info->mChatID[0] == 0 )
					MSN_ChatStart(info);
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

			UrlDecode( data.userEmail ); UrlDecode( data.userNick );
			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, 1, 1 );

			Utf8Decode( data.userNick );
			MSN_DebugLog( "New contact in channel %s %s", data.userEmail, data.userNick );

			info->mInitialContact = NULL;
			info->mMessageCount = 0;

			if ( MSN_ContactJoined( info, hContact ) == 1 ) {
				MsgQueueEntry E;
				int tFound = MsgQueue_GetNext( hContact, E );
				if ( tFound != 0 ) {
					do {
						if ( E.msgSize == 0 ) {
							info->sendMessage( E.msgType, E.message, 0 );
							MSN_SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )E.seq, 0 );
						}
						else info->sendRawMessage( E.msgType, E.message, E.msgSize );

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
				mir_snprintf(
					multichatmsg, sizeof( multichatmsg ),
					MSN_Translate( "%s (%s) has joined the chat with %s" ),
					data.userNick, data.userEmail, tContactName );

				MSN_ShowPopup( tContactName, multichatmsg, MSN_ALLOW_MSGBOX );

				if ( msnHaveChatDll ) {
					GCDEST gcd = {0};
					GCEVENT gce = {0};

					gcd.pszModule = msnProtocolName;
					gcd.pszID = info->mChatID;
					gcd.iType = GC_EVENT_JOIN;

					gce.cbSize = sizeof(GCEVENT);
					gce.pDest = &gcd;
					gce.pszNick = MSN_GetContactName( MSN_HContactFromEmail( data.userEmail, NULL, 1, 1 ));
					gce.pszUID = data.userEmail;
					gce.pszStatus = Translate( "Others" );
					gce.time = time(NULL);
					gce.bIsMe = FALSE;
					gce.bAddToLog = TRUE;
					MSN_CallService( MS_GC_EVENT, NULL, ( LPARAM )&gce );
			}	}
			return 0;
		}

		case ' GSL':    //********* LSG: lists existing groups
			if ( !sttAddGroup( params, true ))
				goto LBL_InvalidCommand;
			break;

		case ' TSL':	//********* LST: section 7.6 List Retrieval And Property Management
		{
			int	listId;
			char *userEmail = NULL, *userNick = NULL, *userId = NULL, *groupId = NULL;

			union	{
				char* tWords[ 10 ];
				struct { char *userEmail, *userNick, *list, *groupid; } data;
			};

			int tNumTokens = sttDivideWords( params, 10, tWords );

			if ( --sttListNumber == 0 )
				MSN_SetServerStatus( msnDesiredStatus );

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
					if ( i < tNumTokens-1 )
						groupId = tWords[i+1];
					break;
			}	}

			if ( userEmail == NULL )
				goto LBL_InvalidCommand;

			if ( userNick == NULL )
				userNick = userEmail;

			UrlDecode( userEmail ); UrlDecode( userNick );

			if ( !IsValidListCode( listId ) || !strcmp( userEmail, "messenger@microsoft.com" ))
				break;

			// add user if it wasn't included into a contact list
			sttProcessListedContactMask();
			sttListedContact = MSN_HContactFromEmail( userEmail, userNick, 1, 0 );

			Utf8Decode( userNick );
			Lists_Add( listId, userEmail, userNick );

			if (( listId & ( LIST_AL +  LIST_BL + LIST_FL )) == LIST_BL ) {
				DBDeleteContactSetting( sttListedContact, "CList", "NotOnList" );
				DBWriteContactSettingByte( sttListedContact, "CList", "Hidden", 1 );
			}

			if ( listId & LIST_PL ) {
				if ( !Lists_IsInList( LIST_RL, userEmail )) {
					MSN_AddUser( sttListedContact, userEmail, LIST_PL + LIST_REMOVE );
					MSN_AddUser( sttListedContact, userEmail, LIST_RL );
				}

				if (( listId & ( LIST_AL +  LIST_BL + LIST_FL )) == 0 )
					MSN_AddAuthRequest( sttListedContact, userEmail, userNick );
			}

			if ( listId & ( LIST_BL | LIST_AL )) {
				WORD tApparentMode = MSN_GetWord( sttListedContact, "ApparentMode", 0 );
				if (( listId & LIST_BL ) && tApparentMode == 0 )
					MSN_SetWord( sttListedContact, "ApparentMode", ID_STATUS_OFFLINE );
				else if (( listId & LIST_AL ) && tApparentMode != 0 )
					MSN_SetWord( sttListedContact, "ApparentMode", 0 );
			}

			if ( sttListedContact != NULL ) {
				if ( userId  != NULL )
					MSN_SetString( sttListedContact, "ID", userId );

				if ( MyOptions.ManageServer ) {
					if ( groupId != NULL ) {
						char* p = strchr( groupId, ',' );
						if ( p != NULL )
							*p = 0;

						MSN_SetString( sttListedContact, "GroupID", groupId );

						if (( p = ( char* )MSN_GetGroupById( groupId )) != NULL ) {
							DBVARIANT dbv;
							if ( !DBGetContactSettingStringUtf( sttListedContact, "CList", "Group", &dbv )) {
								if ( strcmp( dbv.pszVal, p ))
									DBWriteContactSettingStringUtf( sttListedContact, "CList", "Group", p );
								MSN_FreeVariant( &dbv );
							}
							else DBWriteContactSettingStringUtf( sttListedContact, "CList", "Group", p );
					}	}
					else {
						DBDeleteContactSetting( sttListedContact, "CList", "Group" );
						DBDeleteContactSetting( sttListedContact, msnProtocolName, "GroupID" );
			}	}	}
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

			if ( !stricmp( params, "MIG" )) // ignore it
				break;

			return 1;

		case ' PRP':	//********* PRP: user property
		{
			union {
				char* tWords[ 2 ];
				struct { char *name, *value; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) != 2 )
				goto LBL_InvalidCommand;

			UrlDecode( data.value );
			if ( !stricmp( data.name, "MFN" ))
				if ( !sttIsSync || !MSN_GetByte( "NeverUpdateNickname", 0 ))
					MSN_SetStringUtf( NULL, "Nick", data.value );
			break;
		}
		case ' YRQ':   //********* QRY:
			sttProcessListedContactMask();
			break;

		case ' GNQ':	//********* QNG: reply to PNG
			msnPingTimeout = trid;
			if ( msnGetInfoContact != NULL ) {
				MSN_SendBroadcast( msnGetInfoContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, ( HANDLE )1, 0 );
				msnGetInfoContact = NULL;
			}
			break;

		case ' GER':   //********* REG: rename group
		{
			union {
				char* tWords[ 2 ];
				struct { char *id, *groupName; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) != 2 )
				goto LBL_InvalidCommand;

			UrlDecode( data.groupName );
			MSN_SetGroupName( data.id, data.groupName );
			break;
		}
		case ' MER':   //********* REM: section 7.8 List Modifications
		{
			union {
				char* tWords[ 3 ];
				struct { char *list, *serial, *groupId; } data;
			};

			if ( sttDivideWords( params, 3, tWords ) == 3 ) { // remove from a group
				HANDLE hContact = MSN_HContactById( data.serial );
				if ( hContact != NULL )
					DBDeleteContactSetting( hContact, msnProtocolName, "GroupID" );
			}
			else { // remove a user from a list
				int listId = Lists_NameToCode( data.list );
				if ( IsValidListCode( listId )) {
					if ( listId == LIST_FL ) {
						HANDLE hContact = MSN_HContactById( data.serial );
						if ( hContact != NULL ) {
							char tEmail[ MSN_MAX_EMAIL_LEN ];
							if ( !MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof tEmail ))
								Lists_Remove( listId, tEmail );
					}	}
					else {
                  UrlDecode( data.serial );
						Lists_Remove( listId, data.serial );
			}	}	}
			break;
		}
		case ' GMR':    //********* RMG: remove a group
		{
			MSN_DeleteGroup( params );
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
			MSN_ContactJoined( newThread, MSN_HContactFromEmail( data.callerEmail, data.callerNick, false, true ));
			mir_snprintf( newThread->mCookie, sizeof( newThread->mCookie ), "%s %d", data.authChallengeInfo, trid );

			MSN_DebugLog( "Opening caller's switchboard server '%s'...", data.newServer );
			newThread->startThread(( pThreadFunc )MSNServerThread );
			break;
		}
		case ' PBS':    //********* SBP: Server Property was changed
			break;

		case ' NYS':    //********* SYN: section 7.5 Client User Property Synchronization
		{
			char* tWords[ 4 ];
			if ( sttDivideWords( params, 4, tWords ) != 4 )
				goto LBL_InvalidCommand;

			Lists_Wipe();
			sttIsSync = true;
			if (( sttListNumber = atol( tWords[ 2 ] )) == 0 )
				MSN_SetServerStatus( msnDesiredStatus );
			sttListedContact = NULL;
			break;
		}
		case ' XBU':   // UBX : MSNP11+ User Status Message
		{
			union {
				char* tWords[ 2 ];
				struct { char *email, *datalen; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) != 2 )
				goto LBL_InvalidCommand;

			HANDLE hContact = MSN_HContactFromEmail( data.email, data.email, 0, 0 );
			if ( hContact == NULL )
				break;

			int len = atol( data.datalen );
			if ( len < 0 || len > 4000 )
				goto LBL_InvalidCommand;

			char* dataBuf = ( char* )alloca( len+1 ), *p = dataBuf;
			memcpy( dataBuf, HReadBuffer( info, 0 ).surelyRead( len ), len );
			dataBuf[ len ] = 0;

         p = strstr( dataBuf, "<PSM>" );
			if ( p ) {
				p += 5;
				char* p1 = strstr( p, "</PSM>" );
				if ( p1 ) {
					*p1 = 0;
					if ( *p != 0 ) {
						HtmlDecode( p ); Utf8Decode( p );
						DBWriteContactSettingString( hContact, "CList", "StatusMsg", p );
					}
					else DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
			}	}
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
					if ( MSN_GetPassportAuth( data.authChallengeInfo, tAuth )) {
						MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
							MSN_GoOffline();
						return 1;
					}

					info->sendPacket( "USR", "TWN S %s", tAuth );
					free( tAuth );
				}
				else if ( !strcmp( data.security, "OK" )) {
					UrlDecode( tWords[1] ); UrlDecode( tWords[2] );

					if ( MSN_GetByte( "NeverUpdateNickname", 0 )) {
						DBVARIANT dbv;
						if ( !DBGetContactSettingTString( NULL, msnProtocolName, "Nick", &dbv )) {
							MSN_SendNicknameT( dbv.ptszVal );
							MSN_FreeVariant( &dbv );
						}
					}
					else MSN_SetStringUtf( NULL, "Nick", tWords[2] );

					msnLoggedIn = true;
					sttListNumber = 0;
					info->sendPacket( "SYN", "0 0" );
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

		case ' XUU':   // UUX: MSNP11 addition
			break;

		case ' REV':	//******** VER: section 7.1 Protocol Versioning
		{
			char protocol1[ 7 ];
			if ( sscanf( params, "%6s", protocol1 ) < 1 )
				goto LBL_InvalidCommand;

			char tEmail[ MSN_MAX_EMAIL_LEN ];
			if ( MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail ))) {
				MSN_ShowError( "You must specify your e-mail in Options/Network/MSN" );
				return 1;
			}

			if ( !strcmp( protocol1, "MSNP10" )) {
				info->sendPacket( "CVR","0x0409 winnt 5.1 i386 MSNMSGR 6.2.0205 MSMSGS %s", tEmail );
				msnProtChallenge = "Q1P7W2E4J9R8U3S5";
				msnProductID = "msmsgs@msnmsgr.com";
			}
         else if ( !strcmp( protocol1, "MSNP11" )) {
				info->sendPacket( "CVR","0x0409 winnt 5.1 i386 MSNMSGR 7.5.0311 MSMSGS %s", tEmail );
				msnProtChallenge = "YMM8C_H7KCQ2S_KL";
				msnProductID = "PROD0090YUAUV{2B";
			}
			else {
				MSN_ShowError( "Server has requested an unknown protocol set (%s)", params );

				if ( info->mType == SERVER_NOTIFICATION || info->mType == SERVER_DISPATCH ) {
					MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPROTOCOL );
					MSN_GoOffline();
				}

				return 1;
			}
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
