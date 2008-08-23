/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Starts a file sending thread

void MSN_ConnectionProc( HANDLE hNewConnection, DWORD dwRemoteIP, void* extra)
{
	CMsnProto *proto = (CMsnProto*)extra;
	
	proto->MSN_DebugLog( "File transfer connection accepted" );

	WORD localPort = 0;
	SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, ( WPARAM )hNewConnection, 0 );
	if ( s != INVALID_SOCKET) {
		SOCKADDR_IN saddr;
		int len = sizeof( saddr );
		if ( getsockname( s, ( SOCKADDR* )&saddr, &len ) != SOCKET_ERROR )
			localPort = ntohs( saddr.sin_port );
	}

	if ( localPort != 0 ) {
		ThreadData* T = proto->MSN_GetThreadByPort( localPort );
		if ( T != NULL && T->s == NULL ) {
			T->s = hNewConnection;
			ReleaseSemaphore( T->hWaitEvent, 1, NULL );
			return;
		}
		proto->MSN_DebugLog( "There's no registered file transfers for incoming port #%d, connection closed", localPort );
	}
	else proto->MSN_DebugLog( "Unable to determine the local port, file server connection closed." );

	Netlib_CloseHandle( hNewConnection );
}


void CMsnProto::sttSetMirVer( HANDLE hContact, DWORD dwValue )
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
		"WLM 9.0",
		"WLM Unknown",
	};

	if ( dwValue & 0x1 )
		setString( hContact, "MirVer", "MSN Mobile" );
	else if ( dwValue & 0x200 )
		setString( hContact, "MirVer", "Webmessenger" );
	else if ( dwValue == 0x800800 )
		setString( hContact, "MirVer", "Yahoo" );
	else if ( dwValue == 0x800 )
		setString( hContact, "MirVer", "LCS" );
	else if ( dwValue == 0x50000000 )
		setString( hContact, "MirVer", "Miranda IM 0.5.x (MSN v.0.5.x)" );
	else if ( dwValue == 0x30000024 )
		setString( hContact, "MirVer", "Miranda IM 0.4.x (MSN v.0.4.x)" );
	else {
		unsigned wlmId = min(dwValue >> 28 & 0xff, SIZEOF(MirVerStr)-1);
		setString( hContact, "MirVer", MirVerStr[ wlmId ] );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes various invitations

void CMsnProto::sttInviteMessage( ThreadData* info, char* msgBody, char* email, char* nick )
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
	const char* Connectivity = tFileInfo[ "Connectivity" ];
 
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
		filetransfer* ft = info->mMsnFtp = new filetransfer(this);

		ft->std.hContact = MSN_HContactFromEmail( email, nick, true, true );
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
		ccs.hContact = MSN_HContactFromEmail( email, nick, true, true );
		ccs.szProtoService = PSR_FILE;
		ccs.wParam = 0;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		mir_free(pre.szMessage);
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
		newThread->startThread( &CMsnProto::MSNServerThread, this );
		return;
	}

	if ( Invcommand != NULL && Invcookie != NULL && Port == NULL && AuthCookie == NULL && SessionID == NULL ) { // send 1
		msnftp_startFileSend( info, Invcommand, Invcookie );
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

void CMsnProto::sttCustomSmiley( const char* msgBody, char* email, char* nick, int iSmileyType )
{
	HANDLE hContact = MSN_HContactFromEmail( email, nick, true, true );

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
			filetransfer* ft = new filetransfer(this);
			ft->std.hContact = hContact;

			ft->p2p_object = (char*)mir_alloc(sz + 1);
			memcpy(ft->p2p_object, tok1, sz);
			ft->p2p_object[sz] = 0;

			size_t slen = strlen(lastsml);
			size_t rlen = Netlib_GetBase64EncodedBufferSize(slen);
			char* buf = (char*)mir_alloc(rlen);

			NETLIBBASE64 nlb = { buf, rlen, (PBYTE)lastsml, slen };
			MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));

			char* smileyName = (char*)mir_alloc(rlen*3);
			UrlEncode(buf, smileyName, rlen*3);
			mir_free(buf);

			ft->std.currentFile = (char*)mir_alloc(MAX_PATH);
			MSN_GetCustomSmileyFileName(hContact, ft->std.currentFile, MAX_PATH, smileyName, iSmileyType);
			mir_free(smileyName);

			if (p2p_IsDlFileOk(ft))
				delete ft;
			else
			{
				MSN_DebugLog( "Custom Smiley p2p invite for object : %s", ft->p2p_object );
				p2p_invite( hContact, iSmileyType, ft );
				Sleep(3000);
			}
		}
		parseSmiley = !parseSmiley;
		tok1 = tok2 + 1;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_ReceiveMessage - receives message or a file from the server
/////////////////////////////////////////////////////////////////////////////////////////


void CMsnProto::MSN_ReceiveMessage( ThreadData* info, char* cmdString, char* params )
{
	union {
		char* tWords[ 4 ];
		struct { char *fromEmail, *fromNick, *strMsgBytes; } data;
		struct { char *fromEmail, *netId, *typeId, *strMsgBytes; } datau;
	};

	if ( sttDivideWords( params, 4, tWords ) < 3 ) {
		MSN_DebugLog( "Invalid %.3s command, ignoring", cmdString );
		return;
	}

	int msgBytes;
	char *nick, *email;
	
	if (strncmp(cmdString, "UBM", 3) == 0)
	{
		msgBytes = atol(datau.strMsgBytes);
		nick = datau.fromEmail;
		email = datau.fromEmail;
	}
	else
	{
		msgBytes = atol(data.strMsgBytes);
		nick = data.fromNick;
		email = data.fromEmail;
		UrlDecode(nick);
	}
	stripBBCode(nick);


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
	if (( strchr(email, '@') == NULL ) && _stricmp(email, "Hotmail"))
		return;

	const char* tContentType = tHeader[ "Content-Type" ];
	if ( tContentType == NULL )
		return;

	if ( !_strnicmp( tContentType, "text/x-clientcaps", 17 )) {
		MimeHeaders tFileInfo;
		tFileInfo.readFromBuffer( msgBody );
		info->firstMsgRecv = true;

		HANDLE hContact = MSN_HContactFromEmail(email, nick, false, false );
		const char* mirver = tFileInfo[ "Client-Name" ];
		if ( hContact != NULL && mirver != NULL )
			setString( hContact, "MirVer", mirver );
	}
	else {
		if ( !info->firstMsgRecv ) {
			info->firstMsgRecv = true;
			HANDLE hContact = MSN_HContactFromEmail(email, nick, false, false );
			if ( hContact != NULL )
				sttSetMirVer( hContact, getDword( hContact, "FlagBits", 0 ));
	}	}

	if ( !_strnicmp( tContentType, "text/plain", 10 )) {
		CCSDATA ccs = {0};
		HANDLE tContact = MSN_HContactFromEmail(email, nick, true, true );

		const char* p = tHeader[ "X-MMS-IM-Format" ];
		bool isRtl =  p != NULL && strstr( p, "RL=1" ) != NULL;

		if ( info->mJoinedCount > 1 && info->mJoinedContacts != NULL ) {
			if ( msnHaveChatDll )
				MSN_ChatStart( info );
			else
			{
				for ( int j=0; j < info->mJoinedCount; j++ ) {
					if ( info->mJoinedContacts[j] == tContact && j != 0 ) {
						ccs.hContact = info->mJoinedContacts[ 0 ];
						break;
				}	}
			}
		}
		else ccs.hContact = tContact;

		MSN_CallService( MS_PROTO_CONTACTISTYPING, WPARAM( tContact ), 0 );

		const char* tP4Context = tHeader[ "P4-Context" ];
		if ( tP4Context ) 
		{
			size_t newlen  = strlen( msgBody ) + strlen( tP4Context ) + 4;
			char* newMsgBody = ( char* )mir_alloc( newlen );
			mir_snprintf(newMsgBody, newlen, "[%s] %s", tP4Context, msgBody);
			mir_free(newbody);
			msgBody = newbody = newMsgBody;
		}

		if ( info->mChatID[0] ) {
			GCDEST gcd = { m_szProtoName, { NULL }, GC_EVENT_MESSAGE };
			gcd.ptszID = info->mChatID;

			GCEVENT gce = {0};
			gce.cbSize = sizeof(GCEVENT);
			gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
			gce.pDest = &gcd;
			gce.ptszUID = mir_a2t(email);
			gce.ptszNick = MSN_GetContactNameT( tContact );
			gce.time = time( NULL );
			gce.bIsMe = FALSE;

			#ifdef _UNICODE
				TCHAR* p;
				mir_utf8decode(( char* )msgBody, &p );
				gce.ptszText = EscapeChatTags( p );
				mir_free( p );
			#else
				mir_utf8decode(( char* )msgBody, NULL );
				gce.ptszText = EscapeChatTags( msgBody );
			#endif
			CallServiceSync( MS_GC_EVENT, 0, (LPARAM)&gce );
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
		replaceStr( msnExternalIP, tHeader[ "ClientIP" ] );
		replaceStr( abchMigrated, tHeader[ "ABCHMigrated" ] );
		langpref = atol(tHeader[ "lang_preference" ]);
	}
	else if ( !_strnicmp( tContentType, "text/x-msmsgscontrol", 20 )) {
		const char* tTypingUser = tHeader[ "TypingUser" ];

		if ( tTypingUser != NULL && info->mChatID[0] == 0 ) {
			HANDLE hContact = MSN_HContactFromEmail( tTypingUser, tTypingUser, true, true );
			MSN_CallService( MS_PROTO_CONTACTISTYPING, ( WPARAM ) hContact, 7 );
		}
	}
	else if ( !_strnicmp( tContentType, "text/x-msnmsgr-datacast", 23 )) {
		HANDLE tContact = info->mJoinedContacts[0];

		if (IsChatHandle(tContact))
		{
			GC_INFO gci = {0};
			gci.Flags = HCONTACT;
			gci.pszModule = m_szProtoName;
			gci.pszID = info->mChatID;
			CallServiceSync( MS_GC_GETINFO, 0, (LPARAM)&gci );
			tContact = gci.hContact;
		}

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
		sttInviteMessage( info, msgBody, email, nick);
	else if ( !_strnicmp( tContentType, "application/x-msnmsgrp2p", 24 ))
		p2p_processMsg( info, msgBody );
	else if ( !_strnicmp( tContentType, "text/x-mms-emoticon", 19 ))
		sttCustomSmiley( msgBody, email, nick, MSN_APPID_CUSTOMSMILEY );
	else if ( !_strnicmp( tContentType, "text/x-mms-animemoticon", 23 ))
		sttCustomSmiley( msgBody, email, nick, MSN_APPID_CUSTOMANIMATEDSMILEY );

	mir_free( newbody );
}


/////////////////////////////////////////////////////////////////////////////////////////
// Process Yahoo Find

void CMsnProto::sttProcessYFind( char* buf, size_t len )
{
	ezxml_t xmli = ezxml_parse_str(buf, len);
	
	ezxml_t dom  = ezxml_child(xmli, "d");
	const char* szDom = ezxml_attr(dom, "n");

	ezxml_t cont = ezxml_child(dom, "c");
	const char* szCont = ezxml_attr(cont, "n");
			
	char szEmail[128];
	mir_snprintf(szEmail, sizeof(szEmail), "%s@%s", szCont, szDom);

	const char *szNetId = ezxml_attr(cont, "t");
	if (msnSearchId != NULL)
	{
		if (szNetId != NULL )
		{
			PROTOSEARCHRESULT isr = {0};
			isr.cbSize = sizeof( isr );
			isr.nick = szEmail;
			isr.email = szEmail;

			SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, msnSearchId, ( LPARAM )&isr );
		}
		SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, msnSearchId, 0 );
	
		msnSearchId = NULL;
	}
	else
	{
		if (szNetId != NULL )
		{
			int netId = atol(szNetId);
			if (MSN_AddUser( NULL, szEmail, netId, LIST_FL ))
			{
				MSN_AddUser( NULL, szEmail, netId, LIST_BL + LIST_REMOVE );
				MSN_AddUser( NULL, szEmail, netId, LIST_AL );
			}
		}
	}

	ezxml_free(xmli);
}			


/////////////////////////////////////////////////////////////////////////////////////////
// Process user addition

void CMsnProto::sttProcessAdd( char* buf, size_t len )
{
	ezxml_t xmli = ezxml_parse_str(buf, len);
	ezxml_t dom  = ezxml_child(xmli, "d");
	while (dom != NULL)
	{
		const char* szDom = ezxml_attr(dom, "n");
		ezxml_t cont = ezxml_child(dom, "c");
		while (cont != NULL)
		{
			const char* szCont = ezxml_attr(cont, "n");
			const char* szNick = ezxml_attr(cont, "f");
			int listId =  atol(ezxml_attr(cont, "l"));
			int netId =  atol(ezxml_attr(cont, "t"));
			
			char szEmail[128];
			mir_snprintf(szEmail, sizeof(szEmail), "%s@%s", szCont, szDom);

			UrlDecode((char*)szNick);

			HANDLE hContact = MSN_HContactFromEmail(szEmail, szNick, true, false);
			if (listId == LIST_RL)
				Lists_Add(LIST_PL, netId, szEmail);

			MSN_AddUser(hContact, szEmail, netId, listId);
			int mask = Lists_GetMask(szEmail);
		
			MSN_SetContactDb(hContact, szEmail);
			
			if ( listId == LIST_RL && ( mask & ( LIST_FL+LIST_AL+LIST_BL )) == 0 )
				MSN_AddAuthRequest( hContact, szEmail, szNick );

			cont = ezxml_next(cont);
		}
		dom = ezxml_next(dom);
	}
	ezxml_free(xmli);
}


void CMsnProto::sttProcessRemove( char* buf, size_t len )
{
	ezxml_t xmli = ezxml_parse_str(buf, len);
	ezxml_t dom  = ezxml_child(xmli, "d");
	while (dom != NULL)
	{
		const char* szDom = ezxml_attr(dom, "n");
		ezxml_t cont = ezxml_child(dom, "c");
		while (cont != NULL)
		{
			const char* szCont = ezxml_attr(cont, "n");
			int listId =  atol(ezxml_attr(cont, "l"));
			
			char szEmail[128];
			mir_snprintf(szEmail, sizeof(szEmail), "%s@%s", szCont, szDom);
			Lists_Remove(listId, szEmail);

			listId = Lists_GetMask(szEmail);

			if ((listId & ( LIST_RL | LIST_FL )) == 0) 
			{
				HANDLE hContact = MSN_HContactFromEmail(szEmail, NULL, false, false);
				if (strcmp(szEmail, MyOptions.szEmail))
					MSN_CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
			}

			cont = ezxml_next(cont);
		}
		dom = ezxml_next(dom);
	}
	ezxml_free(xmli);
}


/////////////////////////////////////////////////////////////////////////////////////////
//	MSN_HandleCommands - process commands from the server
/////////////////////////////////////////////////////////////////////////////////////////

void CMsnProto::sttProcessStatusMessage( char* buf, unsigned len, HANDLE hContact )
{
	ezxml_t xmli = ezxml_parse_str(buf, len);

	// Process status message info
	const char* szStatMsg = ezxml_txt(ezxml_child(xmli, "PSM"));
	if (*szStatMsg)
	{
		stripBBCode((char*)szStatMsg);
		DBWriteContactSettingStringUtf( hContact, "CList", "StatusMsg", szStatMsg );
	}
	else
		DBDeleteContactSetting( hContact, "CList", "StatusMsg" );

	mir_utf8decode((char*)szStatMsg, NULL);
	SendBroadcast(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, NULL, (LPARAM)szStatMsg);

	// Process current media info
	const char* szCrntMda = ezxml_txt(ezxml_child(xmli, "CurrentMedia"));
	if (!*szCrntMda)
	{
		deleteSetting( hContact, "ListeningTo" );
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
		deleteSetting( hContact, "ListeningTo" );
		ezxml_free(xmli);
		return;
	}

	// Check if there is any info in the string
	bool foundUsefullInfo = false;
	for (unsigned i = 4; i < pCount; i++) {
		if ( parts[i][0] != '\0' )  {
			foundUsefullInfo = true;
			break;
		}
	}
	if ( !foundUsefullInfo ) {
		deleteSetting( hContact, "ListeningTo" );
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

		setStringUtf( hContact, "ListeningTo", format );
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
		setTString( hContact, "ListeningTo", cm );

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


void CMsnProto::sttProcessPage( char* buf, unsigned len )
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
		ccs.hContact = MSN_HContactFromEmail( szEmail, szEmail, true, true );
		ccs.szProtoService = PSR_MESSAGE;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

	}
	ezxml_free(xmlnot);
}


void CMsnProto::sttProcessNotificationMessage( char* buf, unsigned len )
{
	ezxml_t xmlnot = ezxml_parse_str(buf, len);

	if (strcmp(ezxml_attr(xmlnot, "siteid"), "0") == 0)
	{
		ezxml_free(xmlnot);
		return;
	}

	ezxml_t xmlmsg = ezxml_child(xmlnot, "MSG");
	ezxml_t xmlact = ezxml_child(xmlmsg, "ACTION");
	ezxml_t xmltxt = ezxml_get(xmlmsg, "BODY", 0, "TEXT", -1);

	if (xmltxt != NULL)
	{
		char fullurl[1024];
		size_t sz = 0;

		const char* acturl = ezxml_attr(xmlact, "url");
		if (acturl == NULL || strstr(acturl, "://") == NULL) 
			sz += mir_snprintf(fullurl+sz, sizeof(fullurl)-sz, "%s", ezxml_attr(xmlnot, "siteurl"));
		
		sz += mir_snprintf(fullurl+sz, sizeof(fullurl)-sz, "%s", acturl);
		if (sz != 0 && fullurl[sz-1] != '?')
			sz += mir_snprintf(fullurl+sz, sizeof(fullurl)-sz, "?");

		mir_snprintf(fullurl+sz, sizeof(fullurl)-sz, "notification_id=%s&message_id=%s",
			ezxml_attr(xmlnot, "id"), ezxml_attr(xmlmsg, "id"));

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

int CMsnProto::MSN_HandleCommands( ThreadData* info, char* cmdString )
{
	char* params = "";
	int trid = -1;

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
//	MSN_DebugLog("%s", cmdString);

	switch(( *( PDWORD )cmdString & 0x00FFFFFF ) | 0x20000000 )
	{
		case ' KCA':    //********* ACK: section 8.7 Instant Messages
			ReleaseSemaphore( info->hWaitEvent, 1, NULL );

//			if ( info->mJoinedCount > 0 && MyOptions.SlowSend )
//				SendBroadcast( info->mJoinedContacts[0], ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )trid, 0 );
			break;

		case ' YQF':	//********* FQY: Find Yahoo User
			char* tWords[ 1 ];
			if ( sttDivideWords( params, 1, tWords ) != 1 )
			{
				MSN_DebugLog( "Invalid %.3s command, ignoring", cmdString );
			}
			else
			{
				size_t len = atol(tWords[0]);
				sttProcessYFind((char*)HReadBuffer(info, 0).surelyRead(len), len); 
			}
			break;

		case ' LDA':	//********* ADL: Add to the list
		{
			char* tWords[ 1 ];
			if ( sttDivideWords( params, 1, tWords ) != 1 )
			{
LBL_InvalidCommand:
				MSN_DebugLog( "Invalid %.3s command, ignoring", cmdString );
				break;
			}

			if (strcmp(tWords[0], "OK") != 0)
			{
				size_t len = atol(tWords[0]);
				sttProcessAdd((char*)HReadBuffer(info, 0).surelyRead(len), len); 
			}
			break;
		}

		case ' SBS':
			break;

		case ' SNA':    //********* ANS: section 8.4 Getting Invited to a Switchboard Session
			if ( strcmp( params, "OK" ) == 0 ) 
			{
				info->sendCaps();
				if ( info->mJoinedCount == 1 ) 
				{
					MsgQueueEntry E;
					bool typing = false;
					HANDLE hContact = info->mJoinedContacts[0];

					for (int i=3; --i; )
					{
						while (MsgQueue_GetNext( hContact, E ))
						{
							if ( E.msgType == 'X' ) ;
							else if ( E.msgType == 2571 )
								typing = E.flags != 0;
							else if ( E.msgSize == 0 ) {
								info->sendMessage( E.msgType, NULL, 1, E.message, E.flags );
								SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )E.seq, 0 );
							}
							else 
								info->sendRawMessage( E.msgType, E.message, E.msgSize );

							mir_free( E.message );

							if ( E.ft != NULL )
								info->mMsnFtp = E.ft;
						}
						info->mInitialContact = NULL;
						Sleep( 100 );
					}

					if ( typing )
						MSN_StartStopTyping( info, true );

					if ( getByte( "EnableDeliveryPopup", 0 ))
						MSN_ShowPopup( hContact, TranslateT( "Chat session established by contact request" ), 0 );
				}
				else {
					if (info->mInitialContact != NULL && MsgQueue_CheckContact(info->mInitialContact))
						msnNsThread->sendPacket( "XFR", "SB" );
					info->mInitialContact = NULL;
				}
			}
			break;

		case ' PRP':
			break;

		case ' PLB':    //********* BLP: section 7.6 List Retrieval And Property Management
			break;

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

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, NULL, false, false );

			if ( getByte( "EnableSessionPopup", 0 ))
				MSN_ShowPopup( hContact, TranslateT( "Contact left channel" ), 0 );

			// modified for chat
			if ( msnHaveChatDll ) {
				GCDEST gcd = { m_szProtoName, { NULL }, GC_EVENT_QUIT };
				gcd.ptszID = info->mChatID;

				GCEVENT gce = {0};
				gce.cbSize = sizeof( GCEVENT );
				gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
				gce.pDest = &gcd;
				gce.ptszNick = MSN_GetContactNameT( hContact );
				gce.ptszUID = mir_a2t(data.userEmail);
				gce.time = time( NULL );
				gce.bIsMe = FALSE;
				CallServiceSync( MS_GC_EVENT, 0, ( LPARAM )&gce );
				mir_free(( void* )gce.pszUID );
			}

			// in here, the first contact is the chat ID, starting from the second will be actual contact
			// if only 1 person left in conversation
			int personleft = info->contactLeft( hContact );

			int temp_status = getWord(hContact, "Status", ID_STATUS_OFFLINE);
			if (temp_status == ID_STATUS_INVISIBLE && MSN_GetThreadByContact(hContact) == NULL)
				setWord( hContact, "Status", ID_STATUS_OFFLINE);

			// see if the session is quit due to idleness
			if ( personleft == 1 && !lstrcmpA( data.isIdle, "1" ) ) {
				GCDEST gcd = { m_szProtoName, { NULL }, GC_EVENT_INFORMATION };
				gcd.ptszID = info->mChatID;

				GCEVENT gce = {0};
				gce.cbSize = sizeof( GCEVENT );
				gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
				gce.pDest = &gcd;
				gce.bIsMe = FALSE;
				gce.time = time(NULL);
				gce.ptszText = TranslateT("This conversation has been inactive, participants will be removed.");
				CallServiceSync( MS_GC_EVENT, 0, ( LPARAM )&gce );
				gce.ptszText = TranslateT("To resume the conversation, please quit this session and start a new chat session.");
				CallServiceSync( MS_GC_EVENT, 0, ( LPARAM )&gce );
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
			int oldMode = m_iStatus;
			m_iStatus = MSNStatusToMiranda( params );
			if (oldMode == ID_STATUS_OFFLINE || ( oldMode>=ID_STATUS_CONNECTING && oldMode<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES ))
			{
				HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
				while ( hContact != NULL ) {
					if ( MSN_IsMyContact( hContact )) 
					{
						char tEmail[ MSN_MAX_EMAIL_LEN ];
						if ( getStaticString( hContact, "e-mail", tEmail, sizeof( tEmail )) == 0 && strncmp(tEmail, "tel:", 4) == 0 )
						{
							setWord( hContact, "Status", ID_STATUS_ONTHEPHONE );
						}
					}
					hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
			}	}

			if ( m_iStatus != ID_STATUS_IDLE )
			{
				SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,( HANDLE )oldMode, m_iStatus );
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
			break;

		case ' NLF':    //********* FLN: section 7.9 Notification Messages
		{	
			union {
				char* tWords[ 2 ];
				struct { char *userEmail, *netId; } data;
			};

			int tArgs = sttDivideWords( params, 2, tWords );
			if ( tArgs < 2 )
				goto LBL_InvalidCommand;

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, NULL, false, false );
			if ( hContact != NULL )
			{
				setWord( hContact, "Status", ID_STATUS_OFFLINE );
				setDword( hContact, "IdleTS", 0 );
	            ForkThread(&CMsnProto::MsgQueue_AllClearThread, hContact);
			}
			break;
		}
		case ' NLI':
		case ' NLN':    //********* ILN/NLN: section 7.9 Notification Messages
		{
			union {
				char* tWords[ 6 ];
				struct { char *userStatus, *userEmail, *netId, *userNick, *objid, *cmdstring; } data;
			};

			int tArgs = sttDivideWords( params, 6, tWords );
			if ( tArgs < 4 )
				goto LBL_InvalidCommand;

			UrlDecode( data.userNick );
			stripBBCode( data.userNick );

			WORD lastStatus = ID_STATUS_OFFLINE;
			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, NULL, false, false );
			if ( hContact != NULL ) 
			{
				setStringUtf( hContact, "Nick", data.userNick );
				lastStatus = getWord( hContact, "Status", ID_STATUS_OFFLINE );
				if ( lastStatus == ID_STATUS_OFFLINE || lastStatus == ID_STATUS_INVISIBLE )
					DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
				setWord( hContact, "Status", MSNStatusToMiranda( data.userStatus ));
				setDword( hContact, "IdleTS", strcmp( data.userStatus, "IDL" ) ? 0 : time( NULL ));
			}

			if ( tArgs > 4 && tArgs <= 6 ) {
				UrlDecode( data.cmdstring );
				DWORD dwValue = strtoul( data.objid, NULL, 10 );
				setDword( hContact, "FlagBits", dwValue );

				if ( lastStatus == ID_STATUS_OFFLINE ) {
					DBVARIANT dbv;
					if ( atol(data.netId) != 1 || getTString( hContact, "MirVer", &dbv ))
						sttSetMirVer( hContact, dwValue );
					else
						MSN_FreeVariant( &dbv );
				}

				if (( dwValue & 0xf0000000 ) && data.cmdstring[0] && strcmp( data.cmdstring, "0" )) {
					setString( hContact, "PictContext", data.cmdstring );

					char* szAvatarHash = MSN_GetAvatarHash(data.cmdstring);
					if (szAvatarHash == NULL)
						deleteSetting( hContact, "AvatarHash" );
					else
						setString( hContact, "AvatarHash", szAvatarHash );
					mir_free(szAvatarHash);

					if ( hContact != NULL ) {
						char szSavedContext[ 1024 ];
						int result = getStaticString( hContact, "PictSavedContext", szSavedContext, sizeof( szSavedContext ));
						if ( result || strcmp( szSavedContext, data.cmdstring ))
							SendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0 );
				}	}
				else {
					deleteSetting( hContact, "AvatarHash" );
					deleteSetting( hContact, "AvatarSavedHash" );
					deleteSetting( hContact, "PictContext" );
					deleteSetting( hContact, "PictSavedContext" );

//					char tFileName[ MAX_PATH ];
//					MSN_GetAvatarFileName( hContact, tFileName, sizeof( tFileName ));
//					remove( tFileName );

					SendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0 );
			}	}
			else {
				if ( lastStatus == ID_STATUS_OFFLINE )
					deleteSetting( hContact, "MirVer" );
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

			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, true, true );
			if ( tNumTokens == 5 )
				setDword( hContact, "FlagBits", strtoul( data.flags, NULL, 10 ));

			info->contactJoined( hContact );

			int temp_status = getWord(hContact, "Status", ID_STATUS_OFFLINE);
			if (temp_status == ID_STATUS_OFFLINE && Lists_IsInList(LIST_FL, data.userEmail))
				setWord( hContact, "Status", ID_STATUS_INVISIBLE);

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
			stripBBCode( data.userNick );
			HANDLE hContact = MSN_HContactFromEmail( data.userEmail, data.userNick, true, true );
			if ( tNumTokens == 3 )
				setDword( hContact, "FlagBits", strtoul( data.flags, NULL, 10 ));


			mir_utf8decode( data.userNick, NULL );
			MSN_DebugLog( "New contact in channel %s %s", data.userEmail, data.userNick );

			if ( info->contactJoined( hContact ) == 1 ) {
				info->sendCaps();

				bool typing = false;

				for (int i=3; --i; )
				{
					MsgQueueEntry E;
					while (MsgQueue_GetNext( hContact, E ))
					{
						if ( E.msgType == 'X' ) ;
						else if ( E.msgType == 2571 )
							typing = E.flags != 0;
						else if ( E.msgSize == 0 ) {
							info->sendMessage( E.msgType, NULL, 1, E.message, E.flags );
							SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )E.seq, 0 );
						}
						else info->sendRawMessage( E.msgType, E.message, E.msgSize );

						mir_free( E.message );

						if ( E.ft != NULL )
							info->mMsnFtp = E.ft;
					}
					info->mInitialContact = NULL;
					Sleep( 100 );
				}

				if ( typing )
					MSN_StartStopTyping( info, true );

				if ( getByte( "EnableDeliveryPopup", 0 ))
					MSN_ShowPopup( hContact, TranslateT( "Chat session established by my request" ), 0 );
			}
			else {
				bool chatCreated = info->mChatID[0] != 0;

				info->sendCaps();

				if ( msnHaveChatDll ) {
					if ( chatCreated ) {
						GCDEST gcd = { m_szProtoName, { NULL }, GC_EVENT_JOIN };
						gcd.ptszID = info->mChatID;

						GCEVENT gce = {0};
						gce.cbSize = sizeof(GCEVENT);
						gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
						gce.pDest = &gcd;
						gce.ptszNick = MSN_GetContactNameT( hContact );
						gce.ptszUID = mir_a2t(data.userEmail);
						gce.ptszStatus = TranslateT( "Others" );
						gce.time = time(NULL);
						gce.bIsMe = FALSE;
						CallServiceSync( MS_GC_EVENT, 0, ( LPARAM )&gce );
						mir_free(( void* )gce.ptszUID );
					}
					else MSN_ChatStart( info );
			}	}
			break;
		}

		case ' GSM':   //********* MSG: sections 8.7 Instant Messages, 8.8 Receiving an Instant Message
			MSN_ReceiveMessage( info, cmdString, params );
			break;

		case ' MBU':
			MSN_ReceiveMessage( info, cmdString, params );
			break;

		case ' KAN':   //********* NAK: section 8.7 Instant Messages
			if ( info->mJoinedCount > 0 && MyOptions.SlowSend )
				SendBroadcast( info->mJoinedContacts[0], ACKTYPE_MESSAGE, ACKRESULT_FAILED, 
					( HANDLE )trid, ( LPARAM )MSN_Translate( "Message delivery failed" ));
			MSN_DebugLog( "Message send failed (trid=%d)", trid );
			break;

		case ' TON':   //********* NOT: notification message
			sttProcessNotificationMessage((char*)HReadBuffer( info, 0 ).surelyRead( trid ), trid );
			break;

		case ' GPI':   //********* IPG: mobile page
			sttProcessPage((char*)HReadBuffer( info, 0 ).surelyRead( trid ), trid);
			break;

		case ' FCG':   //********* GCF: 
			HReadBuffer( info, 0 ).surelyRead(atol(params));
			break;

		case ' TUO':   //********* OUT: sections 7.10 Connection Close, 8.6 Leaving a Switchboard Session
			if ( !_stricmp( params, "OTH" )) {
				SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION );
				MSN_DebugLog( "You have been disconnected from the MSN server because you logged on from another location using the same MSN passport." );
			}

			if ( !_stricmp( params, "MIG" )) // ignore it
				break;

			return 1;

		case ' YRQ':   //********* QRY:
			break;

		case ' GNQ':	//********* QNG: reply to PNG
			msnPingTimeout = trid;
			if ( info->mType == SERVER_NOTIFICATION && hKeepAliveThreadEvt != NULL )
					SetEvent( hKeepAliveThreadEvt );
			break;

		case ' LMR':	//********* RML: Remove from the list
		{
			char* tWords[ 1 ];
			if ( sttDivideWords( params, 1, tWords ) != 1 )
				goto LBL_InvalidCommand;

			if (strcmp(tWords[0], "OK") != 0)
			{
				size_t len = atol(tWords[0]);
				sttProcessRemove((char*)HReadBuffer(info, 0).surelyRead(len), len); 
			}
			break;
		}

		case ' GNR':    //********* RNG: section 8.4 Getting Invited to a Switchboard Session
			//note: unusual message encoding: trid==sessionid
		{
			union {
				char* tWords[ 8 ];
				struct { char *newServer, *security, *authChallengeInfo, *callerEmail, *callerNick,
							  *type, *srcUrl, *genGateway; } data;
			};

			if ( sttDivideWords( params, 8, tWords ) != 8 )
				goto LBL_InvalidCommand;

			UrlDecode( data.newServer ); UrlDecode( data.callerEmail );
			UrlDecode( data.callerNick ); mir_utf8decode( data.callerNick, NULL );
			stripBBCode( data.callerNick );

			if ( strcmp( data.security, "CKI" )) {
				MSN_DebugLog( "Unknown security package in RNG command: %s", data.security );
				break;
			}

			ThreadData* newThread = new ThreadData;
			strcpy( newThread->mServer, data.newServer );
			newThread->gatewayType = atol( data.genGateway ) != 0;
			newThread->mType = SERVER_SWITCHBOARD;
			newThread->mInitialContact = MSN_HContactFromEmail( data.callerEmail, data.callerNick, true, true );
			mir_snprintf( newThread->mCookie, sizeof( newThread->mCookie ), "%s %d", data.authChallengeInfo, trid );

			ReleaseSemaphore( newThread->hWaitEvent, 5, NULL );

			MSN_DebugLog( "Opening caller's switchboard server '%s'...", data.newServer );
			newThread->startThread( &CMsnProto::MSNServerThread, this );
			break;
		}

		case ' XBU':   // UBX : MSNP11+ User Status Message
		{
			union {
				char* tWords[ 3 ];
				struct { char *email, *netId, *datalen; } data;
			};

			if ( sttDivideWords( params, 3, tWords ) != 3 )
				goto LBL_InvalidCommand;

			HANDLE hContact = MSN_HContactFromEmail( data.email, data.email, false, false );
			if ( hContact == NULL )
				break;

			int len = atol( data.datalen );
			if ( len < 0 || len > 4000 )
				goto LBL_InvalidCommand;

			sttProcessStatusMessage( (char*)HReadBuffer( info, 0 ).surelyRead( len ), len, hContact );
			break;
		}

		case ' NBU':	// UBN : MSNP13+ File sharing, P2P Bootstrap, TURN setup.
		{
			union {
				char* tWords[ 3 ];
				struct { char *email, *typeId, *datalen; } data;
			};

			if ( sttDivideWords( params, 3, tWords ) != 3 )
				goto LBL_InvalidCommand;

			HANDLE hContact = MSN_HContactFromEmail( data.email, data.email, false, false );
			if ( hContact == NULL )
				break;

			int len = atol( data.datalen );
			if ( len < 0 || len > 4000 )
				goto LBL_InvalidCommand;

			HReadBuffer buf(info, 0);
			char* msgBody = (char*)buf.surelyRead(len);

			switch (atol(data.typeId))
			{
				case 1:
					// File sharing stuff
					// sttProcessFileSharing(msgBody, len, hContact);
					break;

				case 3:
					// P2P Bootstrap
					p2p_processSIP(info, msgBody, NULL);
					break;

				case 10:
					// TURN setup
					break;
			}
		}

		case ' LRU':    // URL : Hotmail, Spaces URL 
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
				if ( getStaticString( hContact, "e-mail", tEmail, sizeof( tEmail ))) {
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
					char* tWords[ 4 ];
					struct { char *security, *sequence, *authChallengeInfo, *nonce; } data;
				};

				if ( sttDivideWords( params, 4, tWords ) != 4 )
					goto LBL_InvalidCommand;

				if ( !strcmp( data.security, "SSO" )) 
				{
					if ( MSN_GetPassportAuth()) 
					{
						SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
						return 1;
					}

					char* sec = GenerateLoginBlob(data.nonce);
					info->sendPacket( "USR", "SSO S %s %s", authStrToken ? authStrToken : "", sec );
					mir_free(sec);
				}
				else if ( !strcmp( data.security, "OK" )) 
				{
					if ( !MSN_RefreshContactList()) {
						SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NOSERVER );
						return 1;
					}

					DBVARIANT dbv;
					if ( !DBGetContactSettingStringUtf( NULL, m_szProtoName, "Nick", &dbv )) {
						if (dbv.pszVal[0]) MSN_SetNicknameUtf( dbv.pszVal );
						MSN_FreeVariant( &dbv );
					}
					MSN_SetServerStatus( m_iDesiredStatus );

					ForkThread( &CMsnProto::msn_keepAliveThread, NULL );
					ForkThread( &CMsnProto::MSNConnDetectThread, NULL );

					msnNsThread->sendPacket( "BLP", msnOtherContactsBlocked ? "BL" : "AL" );
					tridUrlInbox = msnNsThread->sendPacket( "URL", "INBOX" );
				}
				else {
					MSN_DebugLog( "Unknown security package '%s'", data.security );

					if ( info->mType == SERVER_NOTIFICATION || info->mType == SERVER_DISPATCH ) {
						SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPROTOCOL );
					}
					return 1;
			}	}
			break;

		case ' SFR':   // RFS: Refresh Contact List 
			if (!MSN_RefreshContactList()) {
				MSN_ShowError("Cannot retrieve contact list");
				return 1;
			}
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

            if ( strcmp( protocol1, msnProtID )) 
			{
				MSN_ShowError( "Server has requested an unknown protocol set (%s)", params );

				if ( info->mType == SERVER_NOTIFICATION || info->mType == SERVER_DISPATCH ) {
					SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPROTOCOL );
				}
				return 1;
			}
			break;
		}
		case ' RFX':    //******** XFR: sections 7.4 Referral, 8.1 Referral to Switchboard
		{
			union {
				char* tWords[ 7 ];
				struct { char *type, *newServer, *security, *authChallengeInfo,
							  *type2, *srcUrl, *genGateway; } data;
			};

			int numWords = sttDivideWords( params, 7, tWords );
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
				newThread->startThread( &CMsnProto::MSNServerThread, this );
				return 1;  //kill the old thread
			}

			if ( !strcmp( data.type, "SB" )) { //switchboard server
				UrlDecode( data.newServer );

				if ( numWords < 7 )
					goto LBL_InvalidCommand;

				if ( strcmp( data.security, "CKI" )) {
					MSN_DebugLog( "Unknown XFR SB security package '%s'", data.security );
					break;
				}

				ThreadData* newThread = new ThreadData;
				strcpy( newThread->mServer, data.newServer );
				newThread->gatewayType = atol( data.genGateway ) != 0;
				newThread->mType = SERVER_SWITCHBOARD;
				newThread->mCaller = 1;
				strcpy( newThread->mCookie, data.authChallengeInfo );

				MSN_DebugLog( "Opening switchboard server '%s'...", data.newServer );
				newThread->startThread( &CMsnProto::MSNServerThread, this );
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
