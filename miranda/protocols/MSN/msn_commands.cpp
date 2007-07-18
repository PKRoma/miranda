/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
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

void __cdecl MSNNudgeThread( ThreadData* info );
void __cdecl MSNServerThread( ThreadData* info );
void __cdecl MSNSendfileThread( ThreadData* info );

void MSN_ChatStart(ThreadData* info);
void MSN_KillChatSession(TCHAR* id);

int tridUrlInbox = -1, tridUrlEdit = -1;

char* sid = NULL;
char* MSPAuth = NULL;
char* passport = NULL;
char* urlId = NULL;
char* rru = NULL;
char* profileURL = NULL;
char* profileURLId = NULL;
unsigned langpref;

extern HANDLE	 hMSNNudge;

extern int msnPingTimeout;
extern HANDLE hKeepAliveThreadEvt;

unsigned long sl;

/////////////////////////////////////////////////////////////////////////////////////////
// Starts a file sending thread

void MSN_ConnectionProc( HANDLE hNewConnection, DWORD dwRemoteIP, void* )
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
		if ( T != NULL && T->s == NULL ) {
			T->s = hNewConnection;
			ReleaseSemaphore( T->hWaitEvent, 1, NULL );
			return;
		}
		MSN_DebugLog( "There's no registered file transfers for incoming port #%d, connection closed", localPort );
	}
	else MSN_DebugLog( "Unable to determine the local port, file server connection closed." );

	Netlib_CloseHandle( hNewConnection );
}


void sttSetMirVer( HANDLE hContact, DWORD dwValue )
{
	static const char* MirVerStr[] =
	{
		"MSN 4.x-5.x",
		"MSN 6.0",
		"MSN 6.1",
		"MSN 6.2",
		"MSN 7.0",
		"MSN 7.5",
		"WLM 8.0",
		"WLM 8.1",
		"WLM 8.5",
		"WLM Unknown",
	};

	if ( dwValue & 0x1 )
		MSN_SetString( hContact, "MirVer", "MSN Mobile" );
	else if ( dwValue & 0x200 )
		MSN_SetString( hContact, "MirVer", "Webmessenger" );
	else if ( dwValue == 0x50000000 )
		MSN_SetString( hContact, "MirVer", "Miranda IM 0.5.x (MSN v.0.5.x)" );
	else if ( dwValue == 0x30000024 )
		MSN_SetString( hContact, "MirVer", "Miranda IM 0.4.x (MSN v.0.4.x)" );
	else {
		unsigned wlmId = min(dwValue >> 28 & 0xff, SIZEOF(MirVerStr)-1);
		MSN_SetString( hContact, "MirVer", MirVerStr[ wlmId ] );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes various invitations

static void sttInviteMessage( ThreadData* info, char* msgBody, char* email, char* nick )
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
			MSN_ShowPopup( info->mJoinedContacts[0],
				TranslateT( "Contact tried to open an audio conference (currently not supported)" ),
				MSN_ALLOW_MSGBOX );
			return;
	}	}

	if ( Invcommand && ( strcmp( Invcommand, "CANCEL" ) == 0 )) {
		delete info->mMsnFtp;
		info->mMsnFtp = NULL;
	}

	if ( Appname != NULL && Appfile != NULL && Appfilesize != NULL ) { // receive first
		filetransfer* ft = info->mMsnFtp = new filetransfer();

		ft->std.hContact = MSN_HContactFromEmail( email, nick, 1, 1 );
		replaceStr( ft->std.currentFile, Appfile );
		mir_utf8decode( ft->std.currentFile, &ft->wszFileName );
		ft->fileId = -1;
		ft->std.currentFileSize = atol( Appfilesize );
		ft->std.totalBytes = atol( Appfilesize );
		ft->std.totalFiles = 1;
		ft->szInvcookie = mir_strdup( Invcookie );

		int tFileNameLen = strlen( ft->std.currentFile );
		char tComment[ 40 ];
		int tCommentLen = mir_snprintf( tComment, sizeof( tComment ), "%lu bytes", ft->std.currentFileSize );
		char* szBlob = ( char* )mir_alloc( sizeof( DWORD ) + tFileNameLen + tCommentLen + 2 );
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

		if ( info->mMsnFtp == NULL )
		{
			ThreadData* otherThread = MSN_GetOtherContactThread( info );
			if ( otherThread )
			{
				info->mMsnFtp = otherThread->mMsnFtp;
				otherThread->mMsnFtp = NULL;
			}
		}

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
		if ( !_stricmp( Invcommand,"ACCEPT" )) {
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
				Invcookie, MyConnection.GetMyExtIPStr());
			info->sendPacket( "MSG", "N %d\r\n%s", nBytes, command );
		}
		return;
	}

	if ( Appname != NULL && !_stricmp( Appname,"NetMeeting" )) { // netmeeting receive 1
		char command[ 1024 ];
		int nBytes;

		mir_snprintf( command, sizeof( command ), "Accept NetMeeting request from %s?", email );

		if ( MessageBoxA( NULL, command, "MSN Protocol", MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
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
				Invcookie, MyConnection.GetMyExtIPStr());
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
// Processes custom smiley messages

static void sttCustomSmiley( const char* msgBody, char* email, char* nick, int iSmileyType )
{
	HANDLE hContact = MSN_HContactFromEmail( email, nick, 1, 1 );

	char smileyList[500] = "";

	const char *tok1 = msgBody, *tok2;
	char *smlp = smileyList;
	char lastsml[50];

	unsigned iCount = 0;
	bool parseSmiley = true;

	for(;;)
	{
		tok2 = strchr(tok1, '\t');
		if (tok2 == NULL) break;

		size_t sz = tok2 - tok1;
		if (parseSmiley)
		{
			sz = min(sz, sizeof(lastsml) - 1);
			memcpy(lastsml, tok1, sz);
			lastsml[sz] = 0;

			memcpy(smlp, tok1, sz); smlp += sz;
			*(smlp++) = '\n'; *smlp = 0;
			++iCount;
		}
		else
		{
			filetransfer* ft = new filetransfer();
			ft->std.hContact = hContact;

			ft->p2p_object = (char*)mir_alloc(sz + 1);
			memcpy(ft->p2p_object, tok1, sz);
			ft->p2p_object[sz] = 0;

			size_t slen = strlen(lastsml);
			size_t rlen = Netlib_GetBase64EncodedBufferSize(slen);
			char* buf = (char*)mir_alloc(rlen);

			NETLIBBASE64 nlb = { buf, rlen, (PBYTE)lastsml, slen };
			MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));

			ft->std.currentFile = (char*)mir_alloc(rlen*3);
			UrlEncode(buf, ft->std.currentFile, rlen*3);

			mir_free(buf);

			MSN_DebugLog( "Custom Smiley p2p invite for object : %s", ft->p2p_object );
			p2p_invite( hContact, iSmileyType, ft );
		}
		parseSmiley = !parseSmiley;
		tok1 = tok2 + 1;
	}

	if ( MSN_GetByte( "EnableCustomSmileyPopup", 1 ))
	{
		TCHAR popupMessage[500];

		TCHAR* fmt = iSmileyType == MSN_APPID_CUSTOMSMILEY ?
				TranslateT("sent you %d custom smiley(s):\n%s") :
				TranslateT("sent you %d custom animated smiley(s):\n%s");

		wchar_t* wsml;
		mir_utf8decode(smileyList, &wsml);
#ifdef _UNICODE
		mir_sntprintf( popupMessage, SIZEOF( popupMessage ), fmt, iCount, wsml );
#else
		mir_sntprintf( popupMessage, SIZEOF( popupMessage ), fmt, iCount, smileyList );
#endif
		mir_free(wsml);
		MSN_ShowPopup( hContact, popupMessage, 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_ReceiveMessage - receives message or a file from the server
/////////////////////////////////////////////////////////////////////////////////////////


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

	HReadBuffer buf( info, 0 );
	BYTE* msgb = buf.surelyRead( msgBytes );
	if ( msgb == NULL ) return;

	memcpy( msg, msgb, msgBytes );
	msg[ msgBytes ] = 0;

	MSN_DebugLog( "Message:\n%s", msg );

	MimeHeaders tHeader;
	char* msgBody = tHeader.readFromBuffer( msg );

	const char* tMsgId = tHeader[ "Message-ID" ];

	// Chunked message
	char* newbody = NULL;
	if ( tMsgId )
	{
		int idx;
		const char* tChunks = tHeader[ "Chunks" ];
		if ( tChunks )
			idx = addCachedMsg(tMsgId, msg, 0, msgBytes, atol(tChunks), true);
		else
			idx = addCachedMsg(tMsgId, msgBody, 0, strlen(msgBody), 0, true);

		size_t newsize;
		if ( !getCachedMsg( idx, newbody, newsize )) return;
		msgBody = tHeader.readFromBuffer( newbody );
	}

	// message from the server (probably)
	if (( strchr( data.fromEmail, '@' ) == NULL ) && _stricmp( data.fromEmail, "Hotmail" ))
		return;

	const char* tContentType = tHeader[ "Content-Type" ];
	if ( tContentType == NULL )
		return;

	if ( !_strnicmp( tContentType, "text/x-clientcaps", 17 )) {
		MimeHeaders tFileInfo;
		tFileInfo.readFromBuffer( msgBody );
		info->firstMsgRecv = true;

		HANDLE hContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 0, 0 );
		const char* mirver = tFileInfo[ "Client-Name" ];
		if ( hContact != NULL && mirver != NULL )
			MSN_SetString( hContact, "MirVer", mirver );
	}
	else {
		if ( !info->firstMsgRecv ) {
			info->firstMsgRecv = true;
			HANDLE hContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 0, 0 );
			if ( hContact != NULL )
				sttSetMirVer( hContact, MSN_GetDword( hContact, "FlagBits", 0 ));
	}	}

	if ( !_strnicmp( tContentType, "text/plain", 10 )) {
		CCSDATA ccs = {0};
		HANDLE tContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 1, 1 );

		const char* p = tHeader[ "X-MMS-IM-Format" ];
		bool isRtl =  p != NULL && strstr( p, "RL=1" ) != NULL;

		if ( info->mJoinedCount > 1 && info->mJoinedContacts != NULL ) {
			if ( msnHaveChatDll )
				MSN_ChatStart( info );
			else
				for ( int j=0; j < info->mJoinedCount; j++ ) {
					if ( info->mJoinedContacts[j] == tContact && j != 0 ) {
						ccs.hContact = info->mJoinedContacts[ 0 ];
						break;
				}	}
		}
		else ccs.hContact = tContact;

		MSN_CallService( MS_PROTO_CONTACTISTYPING, WPARAM( tContact ), 0 );

		if ( info->mChatID[0] ) {
			GCDEST gcd = { msnProtocolName, NULL, GC_EVENT_MESSAGE };
			gcd.ptszID = info->mChatID;

			GCEVENT gce = {0};
			gce.cbSize = sizeof(GCEVENT);
			gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
			gce.pDest = &gcd;
			gce.ptszUID = a2t(data.fromEmail);
			gce.ptszNick = MSN_GetContactNameT( tContact );
			gce.time = time( NULL );
			gce.bIsMe = FALSE;

			#if defined( _UNICODE )
				TCHAR* p;
				mir_utf8decode(( char* )msgBody, &p );
				gce.ptszText = EscapeChatTags( p );
				mir_free( p );
			#else
				mir_utf8decode(( char* )msgBody, NULL );
				gce.ptszText = EscapeChatTags( msgBody );
			#endif
			MSN_CallService(MS_GC_EVENT, 0, (LPARAM)&gce);
			mir_free(( void* )gce.pszText);
			mir_free(( void* )gce.ptszUID );
		}
		else {
			PROTORECVEVENT pre;
			pre.szMessage = ( char* )msgBody;
			pre.flags = PREF_UTF + ( isRtl ? PREF_RTL : 0);
			pre.timestamp = ( DWORD )time(NULL);
			pre.lParam = 0;

			ccs.szProtoService = PSR_MESSAGE;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		}
	}
	else if ( !_strnicmp( tContentType, "text/x-msmsgsprofile", 20 )) {
		replaceStr( sid,           tHeader[ "sid" ]      );
		replaceStr( MSPAuth,       tHeader[ "MSPAuth" ]  );
		replaceStr( msnExternalIP, tHeader[ "ClientIP" ] );
		langpref = atol(tHeader[ "lang_preference" ]);
	}
	else if ( !_strnicmp( tContentType, "text/x-msmsgscontrol", 20 )) {
		const char* tTypingUser = tHeader[ "TypingUser" ];

		if ( tTypingUser != NULL && info->mChatID[0] == 0 ) {
			HANDLE hContact = MSN_HContactFromEmail( tTypingUser, tTypingUser, 1, 1 );
			MSN_CallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, 7 );
		}
	}
	else if ( !_strnicmp( tContentType, "text/x-msnmsgr-datacast", 23 )) {
		HANDLE tContact = MSN_HContactFromEmail( data.fromEmail, data.fromNick, 1, 1 );

		MimeHeaders tFileInfo;
		tFileInfo.readFromBuffer( msgBody );

		const char* id = tFileInfo[ "ID" ];
		if (id != NULL)
		{
			switch (atol(id))
			{
				case 1:  // Nudge
					NotifyEventHooks(hMSNNudge,(WPARAM) tContact,0);
					break;

				case 2: // Wink
					break;

				case 4: // Action Message
					break;
			}
		}
	}
	else if ( !_strnicmp( tContentType,"text/x-msmsgsemailnotification", 30 ))
		sttNotificationMessage( msgBody, false );
	else if ( !_strnicmp( tContentType, "text/x-msmsgsinitialemailnotification", 37 ))
		sttNotificationMessage( msgBody, true );
	else if ( !_strnicmp( tContentType, "text/x-msmsgsactivemailnotification", 35 ))
		sttNotificationMessage( msgBody, false );
	else if ( !_strnicmp( tContentType, "text/x-msmsgsinitialmdatanotification", 37 ))
		sttNotificationMessage( msgBody, true );
	else if ( !_strnicmp( tContentType, "text/x-msmsgsoimnotification", 28 ))
		sttNotificationMessage( msgBody, false );
	else if ( !_strnicmp( tContentType, "text/x-msmsgsinvite", 19 ))
		sttInviteMessage( info, msgBody, data.fromEmail, data.fromNick );
	else if ( !_strnicmp( tContentType, "application/x-msnmsgrp2p", 24 ))
		p2p_processMsg( info, msgBody );
	else if ( !_strnicmp( tContentType, "text/x-mms-emoticon", 19 ))
		sttCustomSmiley( msgBody, data.fromEmail, data.fromNick, MSN_APPID_CUSTOMSMILEY );
	else if ( !_strnicmp( tContentType, "text/x-mms-animemoticon", 23 ))
		sttCustomSmiley( msgBody, data.fromEmail, data.fromNick, MSN_APPID_CUSTOMANIMATEDSMILEY );

	mir_free( newbody );
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
	mir_utf8decode( userNick, NULL );
	int mask = Lists_Add( listId, hContact );

	if ( listId == LIST_RL && ( mask & ( LIST_FL+LIST_AL+LIST_BL )) == 0 )
		MSN_AddAuthRequest( hContact, userEmail, userNick );

	if (( mask & ( LIST_AL | LIST_BL | LIST_FL )) == LIST_BL ) {
		DBDeleteContactSetting( hContact, "CList", "NotOnList" );
		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
	}

	if ( listId & LIST_AL ) {
		DBDeleteContactSetting( hContact, "CList", "NotOnList" );
	}

	if ( listId & LIST_FL ) {
		DBDeleteContactSetting( hContact, "CList", "NotOnList" );
		DBDeleteContactSetting( hContact, "CList", "Hidden" );
	}

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
		MSN_DeleteSetting( sttListedContact, settingName );
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

	int i = 0;
	char str[ 10 ];

	for ( ;; ) {
		_ltoa( i, str, 10 );

		DBVARIANT dbv;
		if ( DBGetContactSettingStringUtf( NULL, "CListGroups", str, &dbv ))
			break;

		bool result = !_stricmp( dbv.pszVal+1, data.grpName );
		MSN_FreeVariant( &dbv );
		if ( result ) {
			MSN_SetGroupNumber( data.grpId, i );
			if ( !isFromBoot ) MSN_UploadServerGroups( data.grpName );
			return true;
		}
		 ++i;
	}

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

static void sttProcessStatusMessage( char* buf, unsigned len, HANDLE hContact )
{
	ezxml_t xmli = ezxml_parse_str(buf, len);

	// Process status message info
	const char* szStatMsg = ezxml_txt(ezxml_child(xmli, "PSM"));
	if (*szStatMsg)
		DBWriteContactSettingStringUtf( hContact, "CList", "StatusMsg", szStatMsg );
	else
		DBDeleteContactSetting( hContact, "CList", "StatusMsg" );

	// Process current media info
	const char* szCrntMda = ezxml_txt(ezxml_child(xmli, "CurrentMedia"));
	if (!*szCrntMda)
	{
		MSN_DeleteSetting( hContact, "ListeningTo" );
		ezxml_free(xmli);
		return;
	}

	// Get parts separeted by "\\0"
	char *parts[16];
	unsigned pCount;

	char* p = (char*)szCrntMda;
	for (pCount = 0; pCount < SIZEOF(parts); ++pCount)
	{
		parts[pCount] = p;

		char* p1 = strstr(p, "\\0");
		if (p1 == NULL) break;

		*p1 = '\0';
		p = p1 + 2;
	}

	// Now let's mount the final string
	if ( pCount <= 4 )  {
		MSN_DeleteSetting( hContact, "ListeningTo" );
		ezxml_free(xmli);
		return;
	}

	// Check if there is any info in the string
	BOOL foundUsefullInfo = FALSE;
	for (unsigned i = 4; i < pCount; i++) {
		if ( parts[i][0] != '\0' )  {
			foundUsefullInfo = TRUE;
			break;
		}
	}
	if ( !foundUsefullInfo ) {
		MSN_DeleteSetting( hContact, "ListeningTo" );
		ezxml_free(xmli);
		return;
	}

	if (!ServiceExists(MS_LISTENINGTO_GETPARSEDTEXT) ||
		!ServiceExists(MS_LISTENINGTO_OVERRIDECONTACTOPTION) ||
		!CallService(MS_LISTENINGTO_OVERRIDECONTACTOPTION, 0, (LPARAM) hContact))
	{
		// User contact options
		char *format = mir_strdup( parts[3] );
		char *unknown = NULL;
		if (ServiceExists(MS_LISTENINGTO_GETUNKNOWNTEXT))
			unknown = mir_utf8encodeT((TCHAR *) CallService(MS_LISTENINGTO_GETUNKNOWNTEXT, 0, 0));

		for (unsigned i = 4; i < pCount; i++) {
			char part[16];
			size_t lenPart = mir_snprintf(part, sizeof(part), "{%d}", i - 4);
			if (parts[i][0] == '\0' && unknown != NULL)
				parts[i] = unknown;
			size_t lenPartsI = strlen(parts[i]);
			for (p = strstr(format, part); p; p = strstr(p + lenPartsI, part)) {
				if (lenPart < lenPartsI) {
					int loc = p - format;
					format = (char *)mir_realloc(format, strlen(format) + (lenPartsI - lenPart) + 1);
					p = format + loc;
				}
				memmove(p + lenPartsI, p + lenPart, strlen(p + lenPart) + 1);
				memmove(p, parts[i], lenPartsI);
		}	}

		MSN_SetStringUtf( hContact, "ListeningTo", format );
		mir_free(unknown);
		mir_free(format);
	}
	else
	{
		// Use user options
		LISTENINGTOINFO lti = {0};
		lti.cbSize = sizeof(LISTENINGTOINFO);

		#if defined( _UNICODE )
			mir_utf8decode( parts[4], &lti.ptszTitle );
			if ( pCount > 5 ) mir_utf8decode( parts[5], &lti.ptszArtist );
			if ( pCount > 6 ) mir_utf8decode( parts[6], &lti.ptszAlbum );
			if ( pCount > 7 ) mir_utf8decode( parts[7], &lti.ptszTrack );
			if ( pCount > 8 ) mir_utf8decode( parts[8], &lti.ptszYear );
			if ( pCount > 9 ) mir_utf8decode( parts[9], &lti.ptszGenre );
			if ( pCount > 10 ) mir_utf8decode( parts[10], &lti.ptszLength );
			if ( pCount > 11 ) mir_utf8decode( parts[11], &lti.ptszPlayer );
			else mir_utf8decode( parts[0], &lti.ptszPlayer );
			if ( pCount > 12 ) mir_utf8decode( parts[12], &lti.ptszType );
			else mir_utf8decode( parts[1], &lti.ptszType );
		#else
			lti.ptszTitle = parts[4];
			if ( pCount > 5 ) lti.ptszArtist = parts[5];
			if ( pCount > 6 ) lti.ptszAlbum = parts[6];
			if ( pCount > 7 ) lti.ptszTrack = parts[7];
			if ( pCount > 8 ) lti.ptszYear = parts[8];
			if ( pCount > 9 ) lti.ptszGenre = parts[9];
			if ( pCount > 10 ) lti.ptszLength = parts[10];
			if ( pCount > 11 ) lti.ptszPlayer = parts[11];
			else lti.ptszPlayer = parts[0];
			if ( pCount > 12 ) lti.ptszType = parts[12];
			else lti.ptszType = parts[1];
		#endif

		TCHAR *cm = (TCHAR *) CallService(MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM) _T("%title% - %artist%"), (LPARAM) &lti);
		MSN_SetStringT( hContact, "ListeningTo", cm );

		mir_free( cm );
		#if defined( _UNICODE )
			mir_free( lti.ptszArtist );
			mir_free( lti.ptszAlbum );
			mir_free( lti.ptszTitle );
			mir_free( lti.ptszTrack );
			mir_free( lti.ptszYear );
			mir_free( lti.ptszGenre );
			mir_free( lti.ptszLength );
			mir_free( lti.ptszPlayer );
			mir_free( lti.ptszType );
		#endif
	}
	ezxml_free(xmli);
}


static void sttProcessPage( char* buf, unsigned len )
{
	ezxml_t xmlnot = ezxml_parse_str(buf, len);

	ezxml_t xmlbdy = ezxml_get(xmlnot, "MSG", 0, "BODY", -1);
	const char* szMsg = ezxml_txt(ezxml_child(xmlbdy, "TEXT"));
	const char* szTel = ezxml_txt(ezxml_child(xmlbdy, "TEL"));

	if (*szTel && *szMsg)
	{
		size_t lene = strlen(szTel) + 5;
		char* szEmail = (char*)alloca(lene);
		mir_snprintf(szEmail, lene, "tel:%s", szTel);

		PROTORECVEVENT pre = {0};
		pre.szMessage = (char*)szMsg;
		pre.flags = PREF_UTF /*+ (( isRtl ) ? PREF_RTL : 0)*/;
		pre.timestamp = time(NULL);

		CCSDATA ccs = {0};
		ccs.hContact = MSN_HContactFromEmail( szEmail, szEmail, 1, 1 );
		ccs.szProtoService = PSR_MESSAGE;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

	}
	ezxml_free(xmlnot);
}


static void sttProcessNotificationMessage( char* buf, unsigned len )
{
	ezxml_t xmlnot = ezxml_parse_str(buf, len);

	ezxml_t xmlmsg = ezxml_child(xmlnot, "MSG");
	ezxml_t xmlact = ezxml_child(xmlmsg, "ACTION");
	ezxml_t xmltxt = ezxml_get(xmlmsg, "BODY", 0, "TEXT", -1);

	if (xmltxt != NULL)
	{
		char fullurl[1024];
		mir_snprintf(fullurl, sizeof(fullurl), "%snotification_id=%s&message_id=%s",
			ezxml_attr(xmlact, "url"), ezxml_attr(xmlnot, "id"), ezxml_attr(xmlmsg, "id"));

		wchar_t* alrtu;
		const char* txt = ezxml_txt(xmltxt);
		mir_utf8decode( (char*)txt, &alrtu );
		SkinPlaySound( alertsoundname );
#ifdef _UNICODE
		MSN_ShowPopup(TranslateT("MSN Alert"), alrtu, MSN_ALERT_POPUP | MSN_ALLOW_MSGBOX, fullurl);
#else
		MSN_ShowPopup(TranslateT("MSN Alert"), txt, MSN_ALERT_POPUP | MSN_ALLOW_MSGBOX, fullurl);
#endif
		mir_free(alrtu);
	}
	ezxml_free(xmlnot);
}

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
			ReleaseSemaphore( info->hWaitEvent, 1, NULL );

			if ( info->mJoinedCount > 0 && MyOptions.SlowSend )
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
				else {
					if ( MyOptions.ManageServer && strcmp( tWords[0], "FL" ) == 0 ) {
						DBVARIANT dbv;
						if ( !DBGetContactSettingStringUtf( hContact, "CList", "Group", &dbv )) {
							MSN_MoveContactToGroup( hContact, dbv.pszVal );
							MSN_FreeVariant( &dbv );
			}	}	}	}
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

		case ' SBS':
			break;

		case ' SNA':    //********* ANS: section 8.4 Getting Invited to a Switchboard Session
			if ( strcmp( params, "OK" ) == 0 ) {
				info->mInitialContact = NULL;
				info->sendCaps();
				if ( info->mJoinedCount == 1 ) {
					MsgQueueEntry E;
					bool typing = false;
					HANDLE hContact = info->mJoinedContacts[0];

					while (MsgQueue_GetNext( hContact, E ) != 0 )
					{
						if ( E.msgType == 'X' ) ;
						else if ( E.msgType == 2571 )
							typing = E.flags != 0;
						else if ( E.msgSize == 0 ) {
							info->sendMessage( E.msgType, E.message, E.flags );
							MSN_SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )E.seq, 0 );
						}
						else info->sendRawMessage( E.msgType, E.message, E.msgSize );

						mir_free( E.message );

						if ( E.ft != NULL )
							info->mMsnFtp = E.ft;
					}

					if ( typing )
						MSN_StartStopTyping( info, true );

					if ( MSN_GetByte( "EnableDeliveryPopup", 0 ))
						MSN_ShowPopup( hContact, TranslateT( "Chat session established by contact request" ), 0 );
			}	}

			break;

		case ' PLB':    //********* BLP: section 7.6 List Retrieval And Property Management
		{
			union {
				char* tWords[ 2 ];
				struct { char *junk, *listName; } data;
			};

			if ( sttDivideWords( params, 2, tWords ) == 1 )
				data.listName = data.junk;

			msnOtherContactsBlocked = _stricmp( data.listName, "BL" ) == 0;
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

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, NULL, 0, 0 );

			if ( MSN_GetByte( "EnableSessionPopup", 0 ))
				MSN_ShowPopup( hContact, TranslateT( "Contact left channel" ), 0 );

			// modified for chat
			if ( msnHaveChatDll ) {
				GCDEST gcd = { msnProtocolName, NULL, GC_EVENT_QUIT };
				gcd.ptszID = info->mChatID;

				GCEVENT gce = {0};
				gce.cbSize = sizeof( GCEVENT );
				gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
				gce.pDest = &gcd;
				gce.ptszNick = MSN_GetContactNameT( hContact );
				gce.ptszUID = a2t(data.userEmail);
				gce.time = time( NULL );
				gce.bIsMe = FALSE;
				MSN_CallService( MS_GC_EVENT, 0, ( LPARAM )&gce );
				mir_free(( void* )gce.pszUID );
			}

			// in here, the first contact is the chat ID, starting from the second will be actual contact
			// if only 1 person left in conversation
			int personleft = MSN_ContactLeft( info, hContact );
			// see if the session is quit due to idleness
			if ( personleft == 1 && !lstrcmpA( data.isIdle, "1" ) ) {
				GCDEST gcd = { msnProtocolName, NULL, GC_EVENT_INFORMATION };
				gcd.ptszID = info->mChatID;

				GCEVENT gce = {0};
				gce.cbSize = sizeof( GCEVENT );
				gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
				gce.pDest = &gcd;
				gce.bIsMe = FALSE;
				gce.time = time(NULL);
				gce.ptszText = TranslateT("This conversation has been inactive, participants will be removed.");
				MSN_CallService( MS_GC_EVENT, 0, ( LPARAM )&gce );
				gce.ptszText = TranslateT("To resume the conversation, please quit this session and start a new chat session.");
				MSN_CallService( MS_GC_EVENT, 0, ( LPARAM )&gce );
			}
			else if ( personleft == 2 && lstrcmpA( data.isIdle, "1" ) ) {
				if ( MessageBox( NULL, TranslateT( "There is only 1 person left in the chat, do you want to switch back to standard message window?"), TranslateT("MSN Chat"), MB_YESNO|MB_ICONQUESTION) == IDYES) {
					// a flag to let the kill function know what to do
					// if the value is 1, then it'll open up the srmm window
					info->mJoinedCount--;

					// kill chat dlg and open srmm dialog
					MSN_KillChatSession(info->mChatID);
			}	}
			// this is not in chat session, quit the session when everyone left
			else if ( personleft == 0 )
				return 1;

			break;
		}
		case ' LAC':    //********* CAL: section 8.3 Inviting Users to a Switchboard Session
			break;

		case ' GHC':    //********* CHG: section 7.7 Client States
		{
			int oldMode = msnStatusMode;
			msnStatusMode = MSNStatusToMiranda( params );
			if (oldMode == ID_STATUS_OFFLINE || ( oldMode>=ID_STATUS_CONNECTING && oldMode<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES ))
			{
				HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
				while ( hContact != NULL ) {
					if ( MSN_IsMyContact( hContact )) {
						char tEmail[ MSN_MAX_EMAIL_LEN ];
						if ( MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail )) == 0 && strncmp(tEmail, "tel:", 4) == 0 )
						{
							MSN_SetWord( hContact, "Status", ID_STATUS_ONTHEPHONE );
							MSN_SetString( hContact, "MirVer", "SMS" );
						}
					}
					hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
			}	}

			if ( msnStatusMode != ID_STATUS_IDLE )
			{
				MSN_SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,( HANDLE )oldMode, msnStatusMode );
				MSN_DebugLog( "Status change acknowledged: %s", params );
				MSN_RemoveEmptyGroups();
			}
			break;
		}
		case ' LHC':    //********* CHL: Query from Server on MSNP7
		{
			char* authChallengeInfo;
			if (sttDivideWords( params, 1, &authChallengeInfo ) != 1)
				goto LBL_InvalidCommand;

			char dgst[64];
			MSN_MakeDigest(authChallengeInfo, dgst);
			info->sendPacket( "QRY", "%s 32\r\n%s", msnProductID, dgst );
			break;
		}
		case ' RVC':    //********* CVR: MSNP8
		{
			info->sendPacket( "USR", "TWN I %s", MyOptions.szEmail );
			break;
		}
		case ' NLF':    //********* FLN: section 7.9 Notification Messages
		{	HANDLE hContact;
			UrlDecode( params );
			if (( hContact = MSN_HContactFromEmail( params, NULL, 0, 0 )) != NULL )
			{
				MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE );
				MSN_SetDword( hContact, "IdleTS", 0 );
				MsgQueue_Clear( hContact );
			}
			break;
		}
		case ' CTG':    //********* GTC: section 7.6 List Retrieval And Property Management
			break;

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

			WORD lastStatus = ID_STATUS_OFFLINE;
			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, NULL, 0, 0 );
			if ( hContact != NULL) {
				MSN_SetStringUtf( hContact, "Nick", data.userNick );
				lastStatus = MSN_GetWord( hContact, "Status", ID_STATUS_OFFLINE);
				MSN_SetWord( hContact, "Status", ( WORD )MSNStatusToMiranda( data.userStatus ));
				MSN_SetDword( hContact, "IdleTS", strcmp( data.userStatus, "IDL" ) ? 0 : time( NULL ));
			}

			if ( tArgs > 3 && tArgs <= 5 ) {
				UrlDecode( data.cmdstring );
				DWORD dwValue = strtoul( data.objid, NULL, 10 );
				MSN_SetDword( hContact, "FlagBits", dwValue );

				if ( lastStatus == ID_STATUS_OFFLINE ) {
					DBVARIANT dbv;
					if ( MSN_GetStringT( "MirVer", hContact, &dbv ))
						sttSetMirVer( hContact, dwValue );
					else
						MSN_FreeVariant( &dbv );
				}

				if (( dwValue & 0xf0000000 ) && data.cmdstring[0] && strcmp( data.cmdstring, "0" )) {
					int temp_status = MSN_GetWord(hContact, "Status", ID_STATUS_OFFLINE);
					if (temp_status == (WORD)ID_STATUS_OFFLINE)
						MSN_SetWord( hContact, "Status", (WORD)ID_STATUS_INVISIBLE);

					MSN_SetString( hContact, "PictContext", data.cmdstring );

					char* tmpbuf = NEWSTR_ALLOCA(data.cmdstring);
					ezxml_t xmli = ezxml_parse_str(tmpbuf, strlen(tmpbuf));
					const char* szAvatarHash = ezxml_attr(xmli, "SHA1D");
					if (szAvatarHash == NULL || szAvatarHash[0] == '\0')
						MSN_DeleteSetting( hContact, "AvatarHash" );
					else
						MSN_SetString( hContact, "AvatarHash", szAvatarHash );
					ezxml_free(xmli);

					if ( hContact != NULL ) {
						char szSavedContext[ 256 ];
						int result = MSN_GetStaticString( "PictSavedContext", hContact, szSavedContext, sizeof( szSavedContext ));
						if ( result || strcmp( szSavedContext, data.cmdstring ))
							MSN_SendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0 );
				}	}
				else {
					MSN_DeleteSetting( hContact, "AvatarHash" );
					MSN_DeleteSetting( hContact, "PictContext" );
					MSN_DeleteSetting( hContact, "PictSavedContext" );

					char tFileName[ MAX_PATH ];
					MSN_GetAvatarFileName( hContact, tFileName, sizeof( tFileName ));
					remove( tFileName );

					MSN_SendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0 );
			}	}
			else {
				if ( lastStatus == ID_STATUS_OFFLINE )
					MSN_DeleteSetting( hContact, "MirVer" );
			}

			break;
		}
		case ' ORI':    //********* IRO: section 8.4 Getting Invited to a Switchboard Session
		{
			union {
				char* tWords[ 5 ];
				struct { char *strThisContact, *totalContacts, *userEmail, *userNick, *flags; } data;
			};

			int tNumTokens = sttDivideWords( params, 5, tWords );
			if ( tNumTokens < 4 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userEmail );
			UrlDecode( data.userNick );

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, 1, 1 );
			if ( tNumTokens == 5 )
				MSN_SetDword( hContact, "FlagBits", strtoul( data.flags, NULL, 10 ));

			MSN_ContactJoined( info, hContact );

			int thisContact = atol( data.strThisContact );
			if ( thisContact != 1 )
				mir_utf8decode( data.userNick, NULL );

			// only start the chat session after all the IRO messages has been recieved
			if ( msnHaveChatDll && info->mJoinedCount > 1 && !lstrcmpA(data.strThisContact, data.totalContacts) )
				MSN_ChatStart(info);

			break;
		}
		case ' IOJ':    //********* JOI: section 8.5 Session Participant Changes
		{
			union {
				char* tWords[ 3 ];
				struct { char *userEmail, *userNick, *flags; } data;
			};

			int tNumTokens = sttDivideWords( params, 3, tWords );
			if ( tNumTokens < 2 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userEmail ); UrlDecode( data.userNick );
			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, 1, 1 );
			if ( tNumTokens == 3 )
				MSN_SetDword( hContact, "FlagBits", strtoul( data.flags, NULL, 10 ));


			mir_utf8decode( data.userNick, NULL );
			MSN_DebugLog( "New contact in channel %s %s", data.userEmail, data.userNick );

			info->mInitialContact = NULL;

			if ( MSN_ContactJoined( info, hContact ) == 1 ) {
				info->sendCaps();
				MsgQueueEntry E;

				bool typing = false;

				while (MsgQueue_GetNext( hContact, E ) != 0 )
				{
					if ( E.msgType == 'X' ) ;
					else if ( E.msgType == 2571 )
						typing = E.flags != 0;
					else if ( E.msgSize == 0 ) {
						info->sendMessage( E.msgType, E.message, E.flags );
						MSN_SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )E.seq, 0 );
					}
					else info->sendRawMessage( E.msgType, E.message, E.msgSize );

					mir_free( E.message );

					if ( E.ft != NULL )
						info->mMsnFtp = E.ft;
				}

				if ( typing )
					MSN_StartStopTyping( info, true );

				if ( MSN_GetByte( "EnableDeliveryPopup", 0 ))
					MSN_ShowPopup( hContact, TranslateT( "Chat session established by my request" ), 0 );
			}
			else {
				bool chatCreated = info->mChatID[0] != 0;

				info->sendCaps();

				if ( msnHaveChatDll ) {
					if ( chatCreated ) {
						GCDEST gcd = { msnProtocolName, NULL, GC_EVENT_JOIN };
						gcd.ptszID = info->mChatID;

						GCEVENT gce = {0};
						gce.cbSize = sizeof(GCEVENT);
						gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
						gce.pDest = &gcd;
						gce.ptszNick = MSN_GetContactNameT( hContact );
						gce.ptszUID = a2t(data.userEmail);
						gce.ptszStatus = TranslateT( "Others" );
						gce.time = time(NULL);
						gce.bIsMe = FALSE;
						MSN_CallService( MS_GC_EVENT, 0, ( LPARAM )&gce );
						mir_free(( void* )gce.ptszUID );
					}
					else MSN_ChatStart( info );
			}	}
			return 0;
		}

		case ' GSL':    //********* LSG: lists existing groups
			if ( !sttAddGroup( params, true ))
				goto LBL_InvalidCommand;
			break;

		case ' TSL':	//********* LST: section 7.6 List Retrieval And Property Management
		{
			int	listId = 0;
			char *userEmail = NULL, *userNick = NULL, *userId = NULL, *groupId = NULL;
			char* tWords[ 10 ];

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
					int grOff = i + 2;
					if ( grOff < tNumTokens )
						groupId = tWords[grOff];
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

			mir_utf8decode( userNick, NULL );
			Lists_Add( listId, sttListedContact );

			if (( listId & ( LIST_AL | LIST_BL | LIST_FL )) == LIST_BL ) {
				DBDeleteContactSetting( sttListedContact, "CList", "NotOnList" );
				DBWriteContactSettingByte( sttListedContact, "CList", "Hidden", 1 );
			}

			if ( listId == LIST_RL ) {
				DBWriteContactSettingByte( sttListedContact, "CList", "NotOnList", 1 );
				DBWriteContactSettingByte( sttListedContact, "CList", "Hidden", 1 );
			}

			if ( listId & LIST_PL ) {
				if ( !Lists_IsInList( LIST_RL, sttListedContact )) {
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

				 MSN_SyncContactToServerGroup( sttListedContact, userId, groupId );
			}
			break;
		}
		case ' GSM':   //********* MSG: sections 8.7 Instant Messages, 8.8 Receiving an Instant Message
			MSN_ReceiveMessage( info, cmdString, params );
			break;

		case ' KAN':   //********* NAK: section 8.7 Instant Messages
			MSN_DebugLog( "Message send failed (trid=%d)", trid );
			break;

		case ' TON':   //********* NOT: notification message
			sttProcessNotificationMessage((char*)HReadBuffer( info, 0 ).surelyRead( trid ), trid );
			break;

		case ' GPI':   //********* IPG: mobile page
			sttProcessPage((char*)HReadBuffer( info, 0 ).surelyRead( trid ), trid);
			break;

		case ' TUO':   //********* OUT: sections 7.10 Connection Close, 8.6 Leaving a Switchboard Session
			if ( !_stricmp( params, "OTH" )) {
				MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION );
				MSN_DebugLog( "You have been disconnected from the MSN server because you logged on from another location using the same MSN passport." );
			}

			if ( !_stricmp( params, "MIG" )) // ignore it
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
			if (_stricmp(data.name, "MFN") == 0)
			{
				if ( !sttIsSync || !MSN_GetByte( "NeverUpdateNickname", 0 ))
					MSN_SetStringUtf( NULL, "Nick", data.value );
			}
			else if (_stricmp(data.name, "MBE") == 0)
					MSN_SetByte( "MobileEnabled", *data.value == 'Y');
			else if (_stricmp(data.name, "MOB") == 0)
					MSN_SetByte( "MobileAllowed", *data.value == 'Y');
			break;
		}
		case ' YRQ':   //********* QRY:
			sttProcessListedContactMask();
			break;

		case ' GNQ':	//********* QNG: reply to PNG
			msnPingTimeout = trid;
			if ( info->mType == SERVER_NOTIFICATION && hKeepAliveThreadEvt != NULL )
					SetEvent( hKeepAliveThreadEvt );
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
					MSN_DeleteSetting( hContact, "GroupID" );
				MSN_RemoveEmptyGroups();
			}
			else { // remove a user from a list
				int listId = Lists_NameToCode( data.list );
				if ( IsValidListCode( listId )) {
					if ( listId == LIST_FL ) {
						HANDLE hContact = MSN_HContactById( data.serial );
						if ( hContact != NULL )
							Lists_Remove( listId, hContact );
						MSN_RemoveEmptyGroups();
					}
					else {
						HANDLE hContact = MSN_HContactFromEmail( data.serial, data.serial, 0, 0 );
						if ( hContact != NULL )
							Lists_Remove( listId, hContact );
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
			UrlDecode( data.callerNick ); mir_utf8decode( data.callerNick, NULL );

			if ( strcmp( data.security, "CKI" )) {
				MSN_DebugLog( "Unknown security package in RNG command: %s", data.security );
				break;
			}

			ThreadData* newThread = new ThreadData;
			strcpy( newThread->mServer, data.newServer );
			newThread->mType = SERVER_SWITCHBOARD;
			newThread->mInitialContact = MSN_HContactFromEmail( data.callerEmail, data.callerNick, false, true );
			mir_snprintf( newThread->mCookie, sizeof( newThread->mCookie ), "%s %d", data.authChallengeInfo, trid );

			ReleaseSemaphore( newThread->hWaitEvent, 5, NULL );

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
			tridUrlInbox = msnNsThread->sendPacket( "URL", "INBOX" );
			tridUrlEdit  = msnNsThread->sendPacket( "URL", "PROFILE 0x%04x", langpref );
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

			sttProcessStatusMessage( (char*)HReadBuffer( info, 0 ).surelyRead( len ), len, hContact );
			break;
		}
		case ' LRU':
		{
			union {
				char* tWords[ 3 ];
				struct { char *rru, *passport, *urlID; } data;
			};

			if ( sttDivideWords( params, 3, tWords ) != 3 )
				goto LBL_InvalidCommand;

			if ( trid == tridUrlInbox ) {
				replaceStr( passport, data.passport );
				replaceStr( rru, data.rru );
				replaceStr( urlId, data.urlID );
				tridUrlInbox = -1;
			}
			else if ( trid == tridUrlEdit ) {
				replaceStr( profileURL, data.rru );
				replaceStr( profileURLId, data.urlID );
				tridUrlEdit = -1;
			}
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
				mir_utf8decode( data.friendlyName, NULL );

				if ( strcmp( data.status, "OK" )) {
					MSN_DebugLog( "Unknown status to USR command (SB): '%s'", data.status );
					break;
				}

				HANDLE hContact;
				do {
					hContact = MsgQueue_GetNextRecipient();
				} while ( hContact != NULL && MSN_GetUnconnectedThread( hContact ) != NULL );

				if ( hContact == NULL ) { //can happen if both parties send first message at the same time
					MSN_DebugLog( "USR (SB) internal: thread created for no reason" );
					return 1;
					break;
				}

				char tEmail[ MSN_MAX_EMAIL_LEN ];
				if ( MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail ))) {
					MSN_DebugLog( "USR (SB) internal: Contact is not MSN" );
					return 1;
					break;
				}

				info->mInitialContact = hContact;
				info->sendPacket( "CAL", tEmail );
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
					if ( MSN_GetPassportAuth( data.authChallengeInfo )) {
						MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
							MSN_GoOffline();
						return 1;
					}

					info->sendPacket( "USR", "TWN S t=%s&p=%s", tAuthToken, pAuthToken );
				}
				else if ( !strcmp( data.security, "OK" )) {
					UrlDecode( tWords[1] ); UrlDecode( tWords[2] );

					sl = time(NULL); //for hotmail

					if ( MSN_GetByte( "NeverUpdateNickname", 0 )) {
						DBVARIANT dbv;
						if ( !MSN_GetStringT( "Nick", NULL, &dbv )) {
							MSN_SendNicknameT( dbv.ptszVal );
							MSN_FreeVariant( &dbv );
						}
					}
					else MSN_SetStringUtf( NULL, "Nick", tWords[2] );

					msnLoggedIn = true;
					sttListNumber = 0;

					void __cdecl msn_keepAliveThread( void* );
					mir_forkthread(( pThreadFunc )msn_keepAliveThread, NULL );

					void MSNConnDetectThread( void* );
					mir_forkthread( MSNConnDetectThread, NULL );

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

			char* protocol1;
			if ( sttDivideWords( params, 1, &protocol1 ) != 1 )
				goto LBL_InvalidCommand;

			if ( MyOptions.szEmail[0] == 0 ) {
				MSN_ShowError( "You must specify your e-mail in Options/Network/MSN" );
				return 1;
			}

            if ( !strcmp( protocol1, "MSNP12" )) {
				info->sendPacket( "CVR","0x0409 winnt 5.1 i386 MSNMSGR 7.5.0324 msmsgs %s", MyOptions.szEmail );
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

			int numWords = sttDivideWords( params, 4, tWords );
			if ( numWords < 2 )
				goto LBL_InvalidCommand;

			if ( !strcmp( data.type, "NS" )) { //notification server
				UrlDecode( data.newServer );
				ThreadData* newThread = new ThreadData;
				strcpy( newThread->mServer, data.newServer );
				newThread->mType = SERVER_NOTIFICATION;
				newThread->mTrid = info->mTrid;
				newThread->mIsMainThread = true;
				info->mIsMainThread = false;

				MSN_DebugLog( "Switching to notification server '%s'...", data.newServer );
				newThread->startThread(( pThreadFunc )MSNServerThread );
				return 1;  //kill the old thread
			}

			if ( !strcmp( data.type, "SB" )) { //switchboard server
				UrlDecode( data.newServer );

				if ( numWords < 4 )
					goto LBL_InvalidCommand;

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

	return 0;
}
