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
#include "sdk/m_smileyadd.h"

#pragma pack(1)

typedef struct _tag_HFileContext
{
	unsigned len; 
	unsigned ver;
	unsigned __int64 dwSize;
	unsigned type;
	wchar_t wszFileName[ MAX_PATH ];
	char unknown[30];
	unsigned id;
	char unknown2[64];
} HFileContext;

typedef struct _tad_P2P_Header
{
	unsigned          mSessionID;
	unsigned          mID;
	unsigned __int64  mOffset;
	unsigned __int64  mTotalSize;
	unsigned          mPacketLen;
	unsigned          mFlags;
	unsigned          mAckSessionID;
	unsigned          mAckUniqueID;
	unsigned __int64  mAckDataSize;
} P2P_Header;

#pragma pack()

static const char sttP2Pheader[] =
	"Content-Type: application/x-msnmsgrp2p\r\n"
	"P2P-Dest: %s\r\n\r\n";

static const char sttVoidNonce[] = "{00000000-0000-0000-0000-000000000000}";

void __cdecl p2p_filePassiveThread( ThreadData* info );

static void sttLogHeader( P2P_Header* hdrdata )
{
	MSN_DebugLog( "--- Printing message header" );
	MSN_DebugLog( "    SessionID = %08X", hdrdata->mSessionID );
	MSN_DebugLog( "    MessageID = %08X", hdrdata->mID );
#ifndef __GNUC__
	MSN_DebugLog( "    Offset of data = %I64u", hdrdata->mOffset );
	MSN_DebugLog( "    Total amount of data = %I64u", hdrdata->mTotalSize );
#else
	MSN_DebugLog( "    Offset of data = %llu", hdrdata->mOffset );
	MSN_DebugLog( "    Total amount of data = %llu", hdrdata->mTotalSize );
#endif
	MSN_DebugLog( "    Data in packet = %lu bytes", hdrdata->mPacketLen );
	MSN_DebugLog( "    Flags = %08X", hdrdata->mFlags );
	MSN_DebugLog( "    Acknowledged session ID: %08X", hdrdata->mAckSessionID );
	MSN_DebugLog( "    Acknowledged message ID: %08X", hdrdata->mAckUniqueID );
#ifndef __GNUC__
	MSN_DebugLog( "    Acknowledged data size: %I64u", hdrdata->mAckDataSize );
#else
	MSN_DebugLog( "    Acknowledged data size: %llu", hdrdata->mAckDataSize );
#endif
	MSN_DebugLog( "------------------------" );
}

char* getNewUuid(void)
{
	BYTE* p;
	UUID id;

	UuidCreate( &id );
	UuidToStringA( &id, &p );
	size_t len = strlen((char*)p) + 3;
	char* result = (char*)mir_alloc(len);
	mir_snprintf(result, len, "{%s}", p);
	_strupr( result );
	RpcStringFreeA( &p );
	return result;
}

unsigned p2p_getMsgId( HANDLE hContact, int inc )
{
	unsigned cid = MSN_GetDword( hContact, "p2pMsgId", 0 );
	unsigned res = cid ? cid + inc : ( rand() << 16 | rand()); 
	if ( res != cid ) MSN_SetDword( hContact, "p2pMsgId", res );
	
	return res;
}


static bool p2p_createListener(filetransfer* ft, directconnection *dc, MimeHeaders& chdrs)
{
	if (MyConnection.extIP == 0) return 0;
	
	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof( nlb );
	nlb.pfnNewConnectionV2 = MSN_ConnectionProc;
	HANDLE sb = (HANDLE) MSN_CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, ( LPARAM )&nlb);
	if ( sb == NULL ) {
		MSN_DebugLog( "Unable to bind the port for incoming transfers" );
		return false;
	}

	ThreadData* newThread = new ThreadData;
	newThread->mType = SERVER_P2P_DIRECT;
	newThread->mCaller = 3;
	newThread->mIncomingBoundPort = sb;
	newThread->mIncomingPort = nlb.wPort;
	strncpy( newThread->mCookie, ( char* )dc->callId , sizeof( newThread->mCookie ));
	newThread->mInitialContact = ft->std.hContact;

	newThread->startThread(( pThreadFunc ) p2p_filePassiveThread );

	char hostname[256];

	gethostname( hostname, sizeof( hostname ));
	PHOSTENT he = gethostbyname( hostname );

	hostname[0] = 0;
	bool ipInt = false;
	for( unsigned i=0; i<sizeof( hostname )/16; ++i ) 
	{
		const PIN_ADDR addr = (PIN_ADDR)he->h_addr_list[i];

		if (addr == NULL) break;
		if (i != 0) strcat(hostname, " ");
		ipInt |= (addr->S_un.S_addr == MyConnection.extIP);
		strcat(hostname, inet_ntoa(*addr));
	}

	chdrs.addString("Bridge", "TCPv1");
	chdrs.addBool("Listening", true);

	if (dc->useHashedNonce)
		chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
	else
		chdrs.addString("Nonce", dc->mNonceToText(), 2);

	if (!ipInt)
	{
		chdrs.addString("IPv4External-Addrs", mir_strdup(MyConnection.GetMyExtIPStr()), 2);
		chdrs.addLong("IPv4External-Port", nlb.wExPort);
	}
	chdrs.addString("IPv4Internal-Addrs", mir_strdup(hostname), 2);
	chdrs.addLong("IPv4Internal-Port", nlb.wPort);
	chdrs.addULong("SessionID", ft->p2p_sessionid);
	chdrs.addLong("SChannelState", 0);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// sttSavePicture2disk - final handler for avatars downloading

static void sttPictureTransferFailed(filetransfer* ft)
{
		switch(ft->p2p_type)
		{
		case MSN_APPID_AVATAR:
		case MSN_APPID_AVATAR2:
			{
				PROTO_AVATAR_INFORMATION AI = {0};
				AI.cbSize = sizeof( AI );
				AI.hContact = ft->std.hContact;
				MSN_DeleteSetting( ft->std.hContact, "AvatarHash" );
				MSN_SendBroadcast( AI.hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, &AI, 0 );
			}
			break;
		}
		remove(ft->std.currentFile);
}

static void sttSavePicture2disk(filetransfer* ft)
{
	mir_sha1_ctx sha1ctx;
	BYTE sha[ MIR_SHA1_HASH_SIZE ];
	mir_sha1_init( &sha1ctx );

	ft->close();

	BYTE buf[4096];
	int fileId = _open(ft->std.currentFile, O_RDONLY | _O_BINARY, _S_IREAD);
	if ( fileId != -1 ) 
	{
		char* ext;
		int format;

		int bytes = _read(fileId, buf, sizeof(buf));
		if (bytes > 4)
			format = MSN_GetImageFormat(buf, &ext);
		else
		{
			sttPictureTransferFailed(ft);
			return;
		}
	
		while(bytes > 0)
		{
			mir_sha1_append(&sha1ctx, buf, bytes);
			bytes = _read(fileId, buf, sizeof(buf));
		}
		_close(fileId);
		mir_sha1_finish(&sha1ctx, sha);

		char* szAvatarHash = MSN_GetAvatarHash(ft->p2p_object);
		if (szAvatarHash == NULL) 
		{
			sttPictureTransferFailed(ft);
			return;
		}

		char *szSha = arrayToHex(sha, MIR_SHA1_HASH_SIZE);
		if ( strcmp( szAvatarHash, szSha ) == 0 )
		{
			switch(ft->p2p_type)
			{
			case MSN_APPID_AVATAR:
			case MSN_APPID_AVATAR2:
				{
					PROTO_AVATAR_INFORMATION AI = {0};
					AI.cbSize = sizeof( AI );
					AI.hContact = ft->std.hContact;
					MSN_GetAvatarFileName( AI.hContact, AI.filename, sizeof( AI.filename ) - 3);

					AI.format = format;
					strcpy(strchr(AI.filename, '\0'), ext);

					rename(ft->std.currentFile, AI.filename);

					MSN_SetString( ft->std.hContact, "PictSavedContext", ft->p2p_object );
					MSN_SendBroadcast( AI.hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, &AI, 0 );

					// Store also avatar hash
					MSN_SetString( ft->std.hContact, "AvatarSavedHash", szAvatarHash );
					MSN_DebugLog( "Avatar for contact %08x saved to file '%s'", AI.hContact, AI.filename );
				}
				break;

			case MSN_APPID_CUSTOMSMILEY:
			case MSN_APPID_CUSTOMANIMATEDSMILEY:
				{
					SMADD_CONT cont;
					cont.cbSize = sizeof(SMADD_CONT);
					cont.hContact = ft->std.hContact;
					cont.type = 1;

					char* pathcpy = mir_strdup(ft->std.currentFile);
					strcpy(strrchr(pathcpy, '.')+1, ext);
					rename(ft->std.currentFile, pathcpy);

					cont.path = mir_a2t(pathcpy);

					MSN_CallService(MS_SMILEYADD_LOADCONTACTSMILEYS, 0, (LPARAM)&cont);
					mir_free(cont.path);
					mir_free(pathcpy);
				}
				break;
			}
		}
		else
		{
			sttPictureTransferFailed(ft);
			return;
		}
		mir_free(szSha);
		mir_free(szAvatarHash);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendAck - sends MSN P2P acknowledgement to the received message

static const char sttVoidSession[] = "ACHTUNG!!! an attempt made to send a message via the empty session";

static void  p2p_sendMsg( HANDLE hContact, unsigned appId, P2P_Header& hdrdata, char* msgbody, size_t msgsz )
{
	unsigned msgType;

	ThreadData* info = MSN_GetP2PThreadByContact( hContact );
	if ( info == NULL )
	{
		msgType = 0;
		bool isOffline;
		info = MSN_StartSB(hContact, isOffline);
	}
	else if ( info->mType == SERVER_P2P_DIRECT ) msgType = 1;
	else msgType = 2;

	const unsigned fportion = msgType == 1 ? 1352 : 1202;

	char* buf = ( char* ) alloca( sizeof( sttP2Pheader )+ MSN_MAX_EMAIL_LEN + 
		sizeof( P2P_Header) + 20 + fportion );

	size_t offset = 0;
	do 
	{
		size_t portion = msgsz - offset;
		if ( portion > fportion ) portion = fportion;

		char* p = buf;

		// add message header
		if ( msgType == 1 ) {
 			*(unsigned*)p = portion + sizeof( P2P_Header); p += 4;
		}
		else
		{
			char email[MSN_MAX_EMAIL_LEN];
			if ( MSN_GetStaticString( "e-mail", hContact, email, sizeof( email )))
				return;
			else
				p += sprintf( p, sttP2Pheader, email );
		}

		// add message body
		P2P_Header* ph = ( P2P_Header* )p; *ph = hdrdata; p += sizeof(P2P_Header);
		
		if ( msgsz )
		{
			ph->mPacketLen = portion;
			ph->mOffset = offset;
			ph->mTotalSize = msgsz;
			
			memcpy( p, msgbody + offset, portion ); p += portion;
		}

		// add message footer
		if ( msgType != 1 ) 
		{ 
			*(unsigned*)p = htonl(appId);
			p += 4;
		}

		switch ( msgType ) 
		{
			case 0:
				MsgQueue_Add( hContact, 'D', buf, p - buf, NULL );
				break;

			case 1: 
				info->send( buf, p - buf );
				break;

			case 2:
				info->sendRawMessage( 'D', buf, p - buf );
				break;
		}
		offset += portion;
	} 
	while(  offset < msgsz  );
}


void  p2p_sendAck( HANDLE hContact, P2P_Header* hdrdata )
{	
	if (hdrdata == NULL) return;

	P2P_Header tHdr = {0};

	tHdr.mSessionID = hdrdata->mSessionID;
	tHdr.mID = p2p_getMsgId(hContact, 1);
	tHdr.mAckDataSize = hdrdata->mTotalSize;
	tHdr.mTotalSize = hdrdata->mTotalSize;
	tHdr.mFlags = 2;
	tHdr.mAckSessionID = hdrdata->mID;
	tHdr.mAckUniqueID = hdrdata->mAckSessionID;

	p2p_sendMsg( hContact, 0, tHdr, NULL, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendEndSession - sends MSN P2P file transfer end packet

static void  p2p_sendAbortSession( filetransfer* ft )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	P2P_Header tHdr = {0};

	tHdr.mSessionID = ft->p2p_sessionid;
	tHdr.mAckSessionID = ft->p2p_sendmsgid;

	if ( ft->std.sending) {
		tHdr.mFlags = 0x40;
		tHdr.mID = p2p_getMsgId( ft->std.hContact, 1 );
		tHdr.mAckSessionID = tHdr.mID - 2;
	}
	else {
		tHdr.mID = p2p_getMsgId( ft->std.hContact, 1 );
		tHdr.mAckUniqueID = 0x8200000f;
		tHdr.mFlags = 0x80;
		tHdr.mAckDataSize = ft->std.currentFileSize;
	}

	p2p_sendMsg( ft->std.hContact, 0, tHdr, NULL, 0 );
	ft->ts = time( NULL );
}

void  p2p_sendRedirect( filetransfer* ft )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	P2P_Header tHdr = {0};

	tHdr.mSessionID = ft->p2p_sessionid;
	tHdr.mID = p2p_getMsgId( ft->std.hContact, 1 );
	tHdr.mFlags = 0x01;
    tHdr.mAckSessionID = ft->p2p_sendmsgid;
	tHdr.mAckDataSize = ft->std.currentFileProgress;

	p2p_sendMsg( ft->std.hContact, 0, tHdr, NULL, 0 );

	ft->tTypeReq = MSN_GetP2PThreadByContact( ft->std.hContact ) ? SERVER_P2P_DIRECT : SERVER_SWITCHBOARD;
	ft->ts = time( NULL );
	ft->p2p_waitack = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendSlp - send MSN P2P SLP packet

void  p2p_sendSlp(
	filetransfer*	ft,
	MimeHeaders&	pHeaders,
	int				iKind,
	const char*		szContent,
	size_t			szContLen )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	char* buf = ( char* )alloca( pHeaders.getLength() + szContLen + 1000 );
	char* p = buf;

	switch ( iKind ) {
		case -2:   p += sprintf( p, "INVITE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest ); break;
		case -1:   p += sprintf( p, "BYE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest ); break;
		case 200:  p += sprintf( p, "MSNSLP/1.0 200 OK" );	break;
		case 481:  p += sprintf( p, "MSNSLP/1.0 481 No Such Call" ); break;
		case 500:  p += sprintf( p, "MSNSLP/1.0 500 Internal Error" ); break;
		case 603:  p += sprintf( p, "MSNSLP/1.0 603 DECLINE" ); break;
		case 1603: p += sprintf( p, "MSNSLP/1.0 603 Decline" ); break;
		default: return;
	}
	
	if ( iKind < 0 ) {
		mir_free(ft->p2p_branch);
		ft->p2p_branch = getNewUuid();
	}

	p += sprintf( p,
		"\r\nTo: <msnmsgr:%s>\r\n"
		"From: <msnmsgr:%s>\r\n"
		"Via: MSNSLP/1.0/TLP ;branch=%s\r\n", ft->p2p_dest, MyOptions.szEmail, ft->p2p_branch );

	p = pHeaders.writeToBuffer( p );

	p += sprintf( p, "Content-Length: %d\r\n\r\n", szContLen );
	memcpy( p, szContent, szContLen );
	p += szContLen;

	P2P_Header tHdr = {0};

	tHdr.mID = p2p_getMsgId( ft->std.hContact, 1 );
	tHdr.mAckSessionID = ft->p2p_acksessid;

	switch ( iKind ) {
		case -1: case 500: case 603: 
			ft->p2p_byemsgid  = tHdr.mID; 
			break;
	}

	p2p_sendMsg( ft->std.hContact, 0, tHdr, buf, p - buf );
	ft->ts = time( NULL );
	ft->p2p_waitack = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendBye - closes P2P session

void  p2p_sendBye( filetransfer* ft )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addString( "CSeq", "0 " );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionclosebody" );

	char szContents[ 50 ];
	p2p_sendSlp( ft, tHeaders, -1, szContents,
		mir_snprintf( szContents, sizeof( szContents ), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
		ft->p2p_sessionid, 0 ));
}

void  p2p_sendCancel( filetransfer* ft )
{
	p2p_sendBye(ft);
	p2p_sendAbortSession(ft);
}

void  p2p_sendNoCall( filetransfer* ft )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addString( "CSeq", "0 " );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-session-failure-respbody" );

	char szContents[ 50 ];
	p2p_sendSlp( ft, tHeaders, 481, szContents,
		mir_snprintf( szContents, sizeof( szContents ), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
		ft->p2p_sessionid, 0 ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendStatus - send MSN P2P status and its description

void  p2p_sendStatus( filetransfer* ft, long lStatus )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders( 5 );
	tHeaders.addString( "CSeq", "1 " );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	if (lStatus != 1603)
	{
		tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

		char szContents[ 50 ];
		p2p_sendSlp( ft, tHeaders, lStatus, szContents,
			mir_snprintf( szContents, sizeof( szContents ), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
			ft->p2p_sessionid, 0 ));
	}
	else
	{
		tHeaders.addString( "Content-Type", "null" );
		p2p_sendSlp( ft, tHeaders, lStatus, NULL, 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_connectTo - connects to a remote P2P server

static char p2p_greeting[8] = { 4, 0, 0, 0, 'f', 'o', 'o', 0  };

static void sttSendPacket( ThreadData* T, P2P_Header& hdr )
{
	DWORD len = sizeof( P2P_Header );
	T->send(( char* )&len, sizeof( DWORD ));
	T->send(( char* )&hdr, sizeof( P2P_Header ));
}

bool p2p_connectTo( ThreadData* info )
{
	NETLIBOPENCONNECTION tConn = {0};
	tConn.cbSize = sizeof(tConn);
	tConn.szHost = info->mServer;
	tConn.flags = NLOCF_V2;
	tConn.timeout = 5;

	char* tPortDelim = strrchr(info->mServer, ':');
	if (tPortDelim != NULL) 
	{
		*tPortDelim = '\0';
		tConn.wPort = (WORD)atol(tPortDelim + 1);
	}

	MSN_DebugLog("Connecting to %s:%d", tConn.szHost, tConn.wPort);

	info->s = (HANDLE)MSN_CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hNetlibUser, (LPARAM)&tConn);
	if (info->s == NULL)
	{
		TWinErrorCode err;
		MSN_DebugLog("Connection Failed (%d): %s", err.mErrorCode, err.getText());
		return false;
	}
	info->send( p2p_greeting, sizeof( p2p_greeting ));

	P2P_Header reply = {0};
	reply.mID = p2p_getMsgId( info->mInitialContact, 1 );
	reply.mFlags = 0x100;

	directconnection *dc = p2p_getDCByCallID( info->mCookie );
	if ( dc == NULL ) return false;

	if ( dc->useHashedNonce )
		memcpy( &reply.mAckSessionID, dc->mNonce, sizeof( UUID ));
	else
		dc->xNonceToBin(( UUID* )&reply.mAckSessionID );

 	sttSendPacket( info, reply );

	long cbPacketLen;
	HReadBuffer buf( info, 0 );
	BYTE* p;
	if (( p = buf.surelyRead( 4 )) == NULL ) {
		MSN_DebugLog( "Error reading data, closing filetransfer" );
		return false;
	}

	cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( cbPacketLen )) == NULL )
		return false;

	bool cookieMatch;
	P2P_Header* pCookie = ( P2P_Header* )p;

	if ( dc->useHashedNonce ) {
		char* hnonce = dc->calcHashedNonce(( UUID* )&pCookie->mAckSessionID );
		cookieMatch = strcmp( hnonce, dc->xNonce ) == 0;
		mir_free( hnonce );
	}
	else
		cookieMatch = memcmp( &pCookie->mAckSessionID, &reply.mAckSessionID, sizeof( UUID )) == 0;

	if ( !cookieMatch ) {
		MSN_DebugLog( "Invalid cookie received, exiting" );
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_listen - acts like a local P2P server

bool p2p_listen( ThreadData* info )
{
	switch( WaitForSingleObject( info->hWaitEvent, 6000 )) {
	case WAIT_TIMEOUT:
	case WAIT_FAILED:
		MSN_DebugLog( "Incoming connection timed out, closing file transfer" );
		MSN_StartP2PTransferByContact( info->mInitialContact );
LBL_Error:
		MSN_DebugLog( "File listen failed" );
		return false;
	}

	HReadBuffer buf( info, 0 );
	BYTE* p;

	if (( p = buf.surelyRead( 8 )) == NULL )
		goto LBL_Error;

	if ( memcmp( p, p2p_greeting, 8 ) != 0 ) {
		MSN_DebugLog( "Invalid input data, exiting" );
		goto LBL_Error;
	}

	if (( p = buf.surelyRead( 4 )) == NULL ) {
		MSN_DebugLog( "Error reading data, closing filetransfer" );
		goto LBL_Error;
	}

	long cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( cbPacketLen )) == NULL )
		goto LBL_Error;

	directconnection *dc = p2p_getDCByCallID( info->mCookie );
	if ( dc == NULL ) return false;

	bool cookieMatch;
	P2P_Header* pCookie = ( P2P_Header* )p;

	if ( dc->useHashedNonce ) {
		char* hnonce = dc->calcHashedNonce(( UUID* )&pCookie->mAckSessionID );
		cookieMatch = strcmp( hnonce, dc->xNonce ) == 0;
		mir_free( hnonce );
		memcpy( &pCookie->mAckSessionID, dc->mNonce, sizeof( UUID ));
	}
	else
		cookieMatch = memcmp( &pCookie->mAckSessionID, dc->mNonce, sizeof( UUID )) == 0;

	if ( !cookieMatch ) {
		MSN_DebugLog( "Invalid cookie received, exiting" );
		return false;
	}

	pCookie->mID = p2p_getMsgId( info->mInitialContact, 1);
	sttSendPacket( info, *pCookie );
	return true;
}

LONG  p2p_sendPortion( filetransfer* ft, ThreadData* T )
{
	LONG trid;
	char databuf[ 1500 ], *p = databuf;

	// Compute the amount of data to send
	const unsigned long fportion = T->mType == SERVER_P2P_DIRECT ? 1352 : 1202;
	const unsigned long dt = ft->std.currentFileSize - ft->std.currentFileProgress;
	const unsigned long portion = dt > fportion ? fportion : dt;

	// Fill data size for direct transfer

	if ( T->mType != SERVER_P2P_DIRECT )
		p += sprintf( p, sttP2Pheader, ft->p2p_dest );
	else
	{
		*( unsigned long* )p = portion + sizeof( P2P_Header );
		p += sizeof( unsigned long );
	}

	// Fill P2P header
	P2P_Header* H = ( P2P_Header* ) p;
	p += sizeof( P2P_Header );

	memset( H, 0, sizeof( P2P_Header ));
	H->mSessionID = ft->p2p_sessionid;
	H->mID = ft->p2p_sendmsgid;
	H->mFlags = ft->p2p_appID == MSN_APPID_FILE ? 0x01000030 : 0x20;
	H->mTotalSize = ft->std.currentFileSize;
	H->mOffset = ft->std.currentFileProgress;
	H->mPacketLen = portion;
	H->mAckSessionID = ft->p2p_acksessid;

	// Fill data (payload) for transfer
	_read( ft->fileId, p, portion );
	p += portion;

	if ( T->mType == SERVER_P2P_DIRECT )
		trid = T->send( databuf, p - databuf);
	else
	{
		// Define packet footer for server transfer
		*( unsigned long * )p = htonl(ft->p2p_appID);
		p += sizeof( unsigned long );

		trid = T->sendRawMessage( 'D', ( char * )databuf, p - databuf);
	}

	if ( trid != 0 ) {
		ft->std.totalProgress += portion;
		ft->std.currentFileProgress += portion;
		if ( ft->p2p_appID == MSN_APPID_FILE )
			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
	}
	else
		MSN_DebugLog(" Error sending");
	ft->ts = time( NULL );
	ft->p2p_waitack = true;

	return trid;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFeedThread - sends a file via server

void __cdecl p2p_sendFeedThread( ThreadData* info )
{
	MSN_DebugLog( "File send thread started" );

	switch( WaitForSingleObject( info->hWaitEvent, 6000 )) 
	{
		case WAIT_FAILED:
			MSN_DebugLog( "File send wait failed" );
			return;
	}

	HANDLE hLockHandle = NULL;

	filetransfer *ft = p2p_getSessionByCallID( info->mCookie );

	if ( ft != NULL && WaitForSingleObject( ft->hLockHandle, 2000 ) == WAIT_OBJECT_0 )
	{
		hLockHandle = ft->hLockHandle;

		if ( ft->p2p_sendmsgid == 0 )
			ft->p2p_sendmsgid = p2p_getMsgId( ft->std.hContact, 1 );

		ThreadData* T = MSN_GetP2PThreadByContact( ft->std.hContact );
		if ( T != NULL )
			ft->tType = T->mType;

		ReleaseMutex( hLockHandle );
	}
	else
		return;

	bool fault = false;
	while ( WaitForSingleObject( hLockHandle, 2000 ) == WAIT_OBJECT_0 &&
		ft->std.currentFileProgress < ft->std.currentFileSize && !ft->bCanceled)
	{
		ThreadData* T = MSN_GetThreadByContact( ft->std.hContact, ft->tType );
		bool cfault = (T == NULL || p2p_sendPortion( ft, T ) == 0);

		if ( cfault ) {
			if ( fault ) {
				MSN_DebugLog( "File send failed" );
				break;
			}
			else
				SleepEx( 3000, TRUE );  // Allow 3 sec for redirect request
		}
		fault = cfault;

		ReleaseMutex( hLockHandle );

		if ( T != NULL && T->mType != SERVER_P2P_DIRECT )
			WaitForSingleObject( T->hWaitEvent, 5000 );
	}
	ReleaseMutex( hLockHandle );
	MSN_DebugLog( "File send thread completed" );
}


void  p2p_sendFeedStart( filetransfer* ft )
{
	if ( ft->std.sending )
	{
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_FILETRANS;
		strcpy( newThread->mCookie, ft->p2p_callID );
		MSN_ContactJoined( newThread, ft->std.hContact );
		newThread->startThread(( pThreadFunc )p2p_sendFeedThread );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFileDirectly - sends a file via MSN P2P protocol

void p2p_sendRecvFileDirectly( ThreadData* info )
{
	long cbPacketLen = 0;
	long state = 0;

	HReadBuffer buf( info, 0 );

	MSN_ContactJoined( info, info->mInitialContact );
	info->mInitialContact = NULL;

	MSN_StartP2PTransferByContact( info->mJoinedContacts[0] );
	p2p_redirectSessions( info->mJoinedContacts[0] );

	for ( ;; ) {
		long len = state ? cbPacketLen : 4;

		BYTE* p = buf.surelyRead( len );

		if ( p == NULL )
			break;

		if ( state == 0 )
			cbPacketLen = *( long* )p;
		else
			p2p_processMsg( info, (char*)p );

		state = ( state + 1 ) % 2;
	}

	HANDLE hContact = info->mJoinedContacts[0];
	MSN_ContactLeft( info, hContact );
	p2p_redirectSessions( hContact );
}

/////////////////////////////////////////////////////////////////////////////////////////
// bunch of thread functions to cover all variants of P2P file transfers

void __cdecl p2p_fileActiveThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_fileActiveThread() started: connecting to '%s'", info->mServer );

	if ( p2p_connectTo( info ))
		p2p_sendRecvFileDirectly( info );

	MSN_DebugLog( "p2p_fileActiveThread() completed: connecting to '%s'", info->mServer );
}

void __cdecl p2p_filePassiveThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_filePassiveThread() started: listening" );

	if ( p2p_listen( info ))
		p2p_sendRecvFileDirectly( info );

	MSN_DebugLog( "p2p_filePassiveThread() completed" );
}


static void sttInitFileTransfer(
	P2P_Header*		hdrdata,
	ThreadData*		info,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2 )
{
	if ( info->mJoinedCount == 0 )
		return;

	char szContactEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_GetStaticString( "e-mail", info->mJoinedContacts[0], szContactEmail, sizeof( szContactEmail )))
		return;

	const char	*szCallID = tFileInfo[ "Call-ID" ],
				*szBranch = tFileInfo[ "Via" ];

	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}
	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch );
		return;
	}

	const char	*szSessionID = tFileInfo2[ "SessionID" ],
				*szEufGuid   = tFileInfo2[ "EUF-GUID" ],
				*szContext   = tFileInfo2[ "Context" ],
				*szAppId     = tFileInfo2[ "AppID" ];

	int dwAppID = szAppId ? atol( szAppId ) : -1;

	if ( szSessionID == NULL || dwAppID == -1 || szEufGuid == NULL || szContext == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: SessionID='%s', AppID=%ld, Branch='%s',Context='%s'",
			szSessionID, dwAppID, szEufGuid, szContext );
		return;
	}

	szContext = MSN_Base64Decode( szContext );

	srand( time( NULL ));

	filetransfer* ft = new filetransfer();
	ft->p2p_acksessid = rand() << 16 | rand();
	ft->p2p_sessionid = strtoul( szSessionID, NULL, 10 );
	ft->p2p_appID = dwAppID == MSN_APPID_AVATAR ? MSN_APPID_AVATAR2 : dwAppID;
	ft->p2p_type = dwAppID;
	ft->p2p_ackID = dwAppID == MSN_APPID_FILE ? 2000 : 1000;
	replaceStr( ft->p2p_callID, szCallID );
	replaceStr( ft->p2p_branch, szBranch );
	ft->p2p_dest = mir_strdup( szContactEmail );
	ft->std.hContact = info->mJoinedContacts[0];

	p2p_sendAck( ft->std.hContact, hdrdata );

	switch ( dwAppID )
	{
		case MSN_APPID_AVATAR:
		case MSN_APPID_AVATAR2:
			if ( !stricmp( szEufGuid, "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}" )) {
				DBVARIANT dbv;
				bool pictmatch = !DBGetContactSettingString( NULL, msnProtocolName, "PictObject", &dbv );
				if ( pictmatch ) 
				{
					UrlDecode(dbv.pszVal);

					ezxml_t xmlcon = ezxml_parse_str((char*)szContext, strlen(szContext));
					ezxml_t xmldb = ezxml_parse_str(dbv.pszVal, strlen(dbv.pszVal));

					const char *szCtBuf = ezxml_attr(xmlcon, "SHA1C");
					const char *szPtBuf = ezxml_attr(xmldb,  "SHA1C");
					pictmatch = szCtBuf && szPtBuf && strcmp( szCtBuf, szPtBuf ) == 0;

					ezxml_free(xmlcon);
					ezxml_free(xmldb);
					MSN_FreeVariant( &dbv );
				}
				if ( pictmatch ) 
				{
					char szFileName[ MAX_PATH ];
					MSN_GetAvatarFileName( NULL, szFileName, sizeof( szFileName ));
					ft->fileId = _open( szFileName, O_RDONLY | _O_BINARY, _S_IREAD );
					if ( ft->fileId == -1 ) 
					{
						p2p_sendStatus( ft, 603 );
						MSN_ShowError("Your avatar not set correctly. Avatar should be set in View/Change My Details | Avatar");
						MSN_DebugLog("Unable to open avatar file '%s', error %d", szFileName, errno );
						delete ft;
					}
					else
					{
						replaceStr(ft->std.currentFile, szFileName);
						MSN_DebugLog( "My avatar file opened for %s as %08p::%d", szContactEmail, ft, ft->fileId );
						ft->std.totalBytes = ft->std.currentFileSize = _filelength( ft->fileId );
						ft->std.sending = true;

						//---- send 200 OK Message
						p2p_sendStatus( ft, 200 );
						p2p_registerSession( ft );
						p2p_sendFeedStart( ft );
					}
				}
				else
				{
					p2p_sendStatus( ft, 603 );
					MSN_DebugLog( "Requested avatar does not match current avatar" );
					delete ft;
				}
			}
			break;

		case MSN_APPID_FILE:
			if ( !stricmp( szEufGuid, "{5D3E02AB-6190-11D3-BBBB-00C04F795683}" )) 
			{
				wchar_t* wszFileName = (( HFileContext* )szContext)->wszFileName;
				{	for ( wchar_t* p = wszFileName; *p != 0; p++ )
					{	switch( *p ) {
						case ':': case '?': case '/': case '\\': case '*':
							*p = '_';
				}	}	}

				#if defined( _UNICODE )
					ft->wszFileName = mir_wstrdup( wszFileName );
				#endif

				char szFileName[ MAX_PATH ];
				char cDefaultChar = '_';
				WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
					wszFileName, -1, szFileName, MAX_PATH, &cDefaultChar, 0 );
				MSN_DebugLog( "File name: '%s'", szFileName );

				replaceStr( ft->std.currentFile, szFileName );
				ft->std.totalBytes =	ft->std.currentFileSize = *( long* )&szContext[ 8 ];
				ft->std.totalFiles = 1;

				p2p_registerSession( ft );

				int tFileNameLen = strlen( ft->std.currentFile );
				char tComment[ 40 ];
				int tCommentLen = mir_snprintf( tComment, sizeof( tComment ), "%lu bytes", ft->std.currentFileSize );
				char* szBlob = ( char* )alloca( sizeof( DWORD ) + tFileNameLen + tCommentLen + 2 );
				*( PDWORD )szBlob = ( DWORD )ft;
				strcpy( szBlob + sizeof( DWORD ), ft->std.currentFile );
				strcpy( szBlob + sizeof( DWORD ) + tFileNameLen + 1, tComment );

				PROTORECVEVENT pre;
				pre.flags = 0;
				pre.timestamp = ( DWORD )time( NULL );
				pre.szMessage = ( char* )szBlob;
				pre.lParam = ( LPARAM )( char* )"";

				CCSDATA ccs;
				ccs.hContact = ft->std.hContact;
				ccs.szProtoService = PSR_FILE;
				ccs.wParam = 0;
				ccs.lParam = ( LPARAM )&pre;
				MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
			}
			break;

		case MSN_APPID_WEBCAM:
			if ( !stricmp( szEufGuid, "{4BD96FC0-AB17-4425-A14A-439185962DC8}" )) {
				MSN_ShowPopup( ft->std.hContact,
					TranslateT( "Contact tried to send its webcam data (currently not supported)" ), 
					MSN_ALLOW_MSGBOX );
			}
			if ( !stricmp( szEufGuid, "{1C9AA97E-9C05-4583-A3BD-908A196F1E92}" )) {
				MSN_ShowPopup( ft->std.hContact,
					TranslateT( "Contact tried to view our webcam data (currently not supported)" ), 
					MSN_ALLOW_MSGBOX );
			}
			p2p_sendStatus( ft, 603 );
			delete ft;
			break;

		default:
			p2p_sendStatus( ft, 603 );
			delete ft;
			MSN_DebugLog( "Invalid or unknown AppID/EUF-GUID combination: %ld/%s", dwAppID, szEufGuid );
			break;
	}

	mir_free(( void* )szContext );
}

static void sttInitDirectTransfer(
	P2P_Header*		hdrdata,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2 )
{
	const char	*szCallID = tFileInfo[ "Call-ID" ],
				*szBranch = tFileInfo[ "Via" ];

	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}
	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', Branch='%s'", szCallID, szBranch );
		return;
	}

	filetransfer* ft = p2p_getSessionByCallID( szCallID );
	if ( ft == NULL ) return;

	p2p_getMsgId( ft->std.hContact, 1 );
	p2p_sendAck( ft->std.hContact, hdrdata );

	replaceStr( ft->p2p_callID, szCallID );
	replaceStr( ft->p2p_branch, szBranch );
	ft->p2p_acksessid = rand() << 16 | rand();

	const char	*szConnType = tFileInfo2[ "Conn-Type" ],
				*szUPnPNat = tFileInfo2[ "UPnPNat" ],
				*szNetID = tFileInfo2[ "NetID" ],
				*szICF = tFileInfo2[ "ICF" ],
				*szHashedNonce = tFileInfo2[ "Hashed-Nonce" ];

	if ( szConnType == NULL || szUPnPNat == NULL || szICF == NULL || szNetID == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: ConnType='%s', UPnPNat='%s', ICF='%s', NetID='%s'",
			szConnType, szUPnPNat, szICF, szNetID );
		return;
	}

	if ( p2p_isAvatarOnly( ft->std.hContact )) {
//		p2p_sendStatus(ft, 1603);
//		return;
	}
	else
		MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);

	directconnection *dc = new directconnection( ft );
	dc->useHashedNonce = szHashedNonce != NULL;
	if ( dc->useHashedNonce )
		dc->xNonce = mir_strdup( szHashedNonce );
	p2p_registerDC( dc );

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "1 " );
	tResult.addString( "Call-ID", szCallID );
	tResult.addLong( "Max-Forwards", 0 );

	MyConnectionType conType = {0};

	conType.extIP = atol( szNetID );
	conType.SetUdpCon(szConnType);
	conType.upnpNAT = strcmp(szUPnPNat, "true") == 0;
	conType.icf = strcmp(szICF, "true") == 0;
	conType.CalculateWeight();

	MimeHeaders chdrs(12);
	bool listen = false;

	MSN_DebugLog( "Connection weight His: %d mine: %d", conType.weight, MyConnection.weight);
	if (conType.weight <= MyConnection.weight)
		listen = p2p_createListener( ft, dc, chdrs);

	if ( !listen ) 
	{
		chdrs.addString("Bridge", "TCPv1");
		chdrs.addBool("Listening", false);

		if (dc->useHashedNonce)
			chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
		else
			chdrs.addString("Nonce", sttVoidNonce);

		chdrs.addULong("SessionID", ft->p2p_sessionid);
		chdrs.addLong("SChannelState", 0);
	}

	tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );

	size_t cbBody = chdrs.getLength() + 1;
	char* szBody = (char*)alloca(cbBody);
	*chdrs.writeToBuffer(szBody) = 0;

	p2p_getMsgId( ft->std.hContact, -2 );
	p2p_sendSlp( ft, tResult, 200, szBody, cbBody );
	p2p_getMsgId( ft->std.hContact, 1 );
}


void p2p_startConnect(HANDLE hContact, const char* szCallID, const char* addr, const char* port)
{
	if (port == NULL) return;

	while (addr != NULL) 
	{
		char* pSpace = (char*)strchr(addr, ' ');
		if (pSpace != NULL) *(pSpace++) = 0;

		ThreadData* newThread = new ThreadData;

		newThread->mType = SERVER_P2P_DIRECT;
		newThread->mInitialContact = hContact;
		mir_snprintf( newThread->mCookie, sizeof( newThread->mCookie ), "%s", szCallID );
		mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", addr, port );

		newThread->startThread(( pThreadFunc )p2p_fileActiveThread );

		addr = pSpace;
	}
}

static void sttInitDirectTransfer2(
	P2P_Header*  hdrdata,
	MimeHeaders& tFileInfo,
	MimeHeaders& tFileInfo2 )
{
	const char	*szCallID = tFileInfo[ "Call-ID" ];
	filetransfer* ft = p2p_getSessionByCallID( szCallID );
	if ( ft == NULL )
		return;

	const char	*szInternalAddress = tFileInfo2[ "IPv4Internal-Addrs" ],
				*szInternalPort = tFileInfo2[ "IPv4Internal-Port" ],
				*szExternalAddress = tFileInfo2[ "IPv4External-Addrs" ],
				*szExternalPort = tFileInfo2[ "IPv4External-Port" ],
				*szNonce = tFileInfo2[ "Nonce" ],
				*szHashedNonce = tFileInfo2[ "Hashed-Nonce" ],
				*szListening = tFileInfo2[ "Listening" ];

	if (( szNonce == NULL && szHashedNonce == NULL ) || szListening == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: Listening='%s', Nonce=%s", szListening, szNonce );
		return;
	}

	directconnection* dc = p2p_getDCByCallID( szCallID );
	if ( dc == NULL ) {
		dc = new directconnection( ft );
		p2p_registerDC( dc );
	}

	dc->useHashedNonce = szHashedNonce != NULL;
	mir_free( dc->xNonce );
	dc->xNonce = mir_strdup( szHashedNonce ? szHashedNonce : szNonce );

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
	p2p_sendAck( ft->std.hContact, hdrdata );


	if ( !strcmp( szListening, "true" ) && strcmp( dc->xNonce, sttVoidNonce )) 
	{
		p2p_startConnect(ft->std.hContact, szCallID, szInternalAddress, szInternalPort);
		p2p_startConnect(ft->std.hContact, szCallID, szExternalAddress, szExternalPort);
	}
}

static void sttAcceptTransfer(
	P2P_Header*		hdrdata,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2 )
{
	filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
	if ( ft == NULL )
		return;

	p2p_getMsgId( ft->std.hContact, 1 );
	p2p_sendAck( ft->std.hContact, hdrdata );

	const char *szCallID = tFileInfo[ "Call-ID" ], *szBranch = tFileInfo[ "Via" ];
	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}

	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch );
LBL_Close:
		p2p_sendBye( ft );
		return;
	}

	if ( !ft->std.sending ) {
		replaceStr( ft->p2p_branch, szBranch );
		replaceStr( ft->p2p_callID, szCallID );
		return;
	}

	const char* szOldContentType = tFileInfo[ "Content-Type" ];
	if ( szOldContentType == NULL )
		goto LBL_Close;

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "0 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );

	MimeHeaders chdrs(12);

	if ( !strcmp( szOldContentType, "application/x-msnmsgr-sessionreqbody" )) {
		p2p_sendFeedStart( ft );

		ThreadData* T = MSN_GetP2PThreadByContact( ft->std.hContact );
		if ( T != NULL && T->mType == SERVER_P2P_DIRECT )
		{
			MSN_StartP2PTransferByContact( ft->std.hContact );
			return;
		}

		if ( MyOptions.UseGateway || MyOptions.UseProxy)
			MSN_StartP2PTransferByContact( ft->std.hContact );

		directconnection* dc = new directconnection( ft );
		p2p_registerDC( dc );

		tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );

		chdrs.addString("Bridges", "TCPv1");
		chdrs.addLong("NetID", MyConnection.extIP);
		chdrs.addString("Conn-Type", MyConnection.GetMyUdpConStr());
		chdrs.addBool("UPnPNat", MyConnection.upnpNAT);
		chdrs.addBool("ICF", MyConnection.icf);
		chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
	}
	else if ( !strcmp( szOldContentType, "application/x-msnmsgr-transrespbody" )) {
		const char	*szListening       = tFileInfo2[ "Listening" ],
					*szNonce           = tFileInfo2[ "Nonce" ],
					*szHashedNonce     = tFileInfo2[ "Hashed-Nonce" ],
					*szExternalAddress = tFileInfo2[ "IPv4External-Addrs" ],
					*szExternalPort    = tFileInfo2[ "IPv4External-Port"  ],
					*szInternalAddress = tFileInfo2[ "IPv4Internal-Addrs" ],
					*szInternalPort    = tFileInfo2[ "IPv4Internal-Port"  ];
		if (( szNonce == NULL && szHashedNonce == NULL ) || szListening == NULL ) {
			MSN_DebugLog( "Invalid data packet, exiting..." );
			goto LBL_Close;
		}

		directconnection* dc = p2p_getDCByCallID( szCallID );
		if ( dc == NULL ) return;

		dc->useHashedNonce = szHashedNonce != NULL;
		dc->xNonce = mir_strdup( szHashedNonce ? szHashedNonce : szNonce );

		// another side reported that it will be a server.
		if ( !strcmp( szListening, "true" ) && ( szNonce == NULL || strcmp( szNonce, sttVoidNonce ))) 
		{
			p2p_startConnect(ft->std.hContact, szCallID, szInternalAddress, szInternalPort);
			p2p_startConnect(ft->std.hContact, szCallID, szExternalAddress, szExternalPort);
			return;
		}

		// no, send a file via server
		if ( !p2p_createListener(ft, dc, chdrs)) 
		{
			MSN_StartP2PTransferByContact( ft->std.hContact );
			return;
		}

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else if ( !strcmp( szOldContentType, "application/x-msnmsgr-transreqbody" )) {
		const char *szHashedNonce = tFileInfo2[ "Hashed-Nonce" ];
		const char *szNonce       = tFileInfo2[ "Nonce" ];

		directconnection* dc = p2p_getDCByCallID( szCallID );
		if ( dc == NULL ) {
			dc = new directconnection( ft );
			p2p_registerDC( dc );
		}

		dc->useHashedNonce = szHashedNonce != NULL;
		dc->xNonce = mir_strdup( szHashedNonce ? szHashedNonce : szNonce );

		// no, send a file via server
		if ( !p2p_createListener( ft, dc, chdrs )) {
			MSN_StartP2PTransferByContact( ft->std.hContact );
			return;
		}

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else 
		return;

	size_t cbBody = chdrs.getLength() + 1;
	char* szBody = (char*)alloca(cbBody);
	*chdrs.writeToBuffer(szBody) = 0;

	p2p_getMsgId( ft->std.hContact, -2 );
	p2p_sendSlp( ft, tResult, -2, szBody, cbBody );
	p2p_getMsgId( ft->std.hContact, 1 );
}


static void sttCloseTransfer( P2P_Header* hdrdata, ThreadData* info, MimeHeaders& tFileInfo )
{
	const char* callID = tFileInfo[ "Call-ID" ];
	
	directconnection *dc = p2p_getDCByCallID( callID );
	if ( dc != NULL )
	{
		p2p_sendAck( info->mJoinedContacts[0], hdrdata );
		p2p_unregisterDC( dc );
		return;
	}

	filetransfer* ft = p2p_getSessionByCallID( callID );
	if ( ft == NULL )
		return;

	p2p_sendAck( ft->std.hContact, hdrdata );
	p2p_unregisterSession( ft );
}


/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processSIP - processes all MSN SIP commands

void p2p_processSIP( ThreadData* info, char* msgbody, void* hdr )
{
	P2P_Header* hdrdata = (P2P_Header*)hdr;

	int iMsgType = 0;
	if ( !memcmp( msgbody, "INVITE MSNMSGR:", 15 ))
		iMsgType = 1;
	else if ( !memcmp( msgbody, "MSNSLP/1.0 200 ", 15 ))
		iMsgType = 2;
	else if ( !memcmp( msgbody, "BYE MSNMSGR:", 12 ))
		iMsgType = 3;
	else if ( !memcmp( msgbody, "MSNSLP/1.0 603 ", 15 ))
		iMsgType = 4;
	else if ( !memcmp( msgbody, "MSNSLP/1.0 481 ", 15 ))
		iMsgType = 4;
	else if ( !memcmp( msgbody, "MSNSLP/1.0 500 ", 15 ))
		iMsgType = 4;

	char* peol = strstr( msgbody, "\r\n" );
	if ( peol != NULL )
		msgbody = peol+2;

	MimeHeaders tFileInfo, tFileInfo2;
	msgbody = tFileInfo.readFromBuffer( msgbody );
	msgbody = tFileInfo2.readFromBuffer( msgbody );

	const char* szContentType = tFileInfo[ "Content-Type" ];
	if ( szContentType == NULL ) {
		MSN_DebugLog( "Invalid or missing Content-Type field, exiting" );
		return;
	}

	switch( iMsgType ) 
	{
	case 1:
		if ( !strcmp( szContentType, "application/x-msnmsgr-sessionreqbody" ))
			sttInitFileTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
		else if ( !strcmp( szContentType, "application/x-msnmsgr-transreqbody" ))
			sttInitDirectTransfer( hdrdata, tFileInfo, tFileInfo2 );
		else if ( !strcmp( szContentType, "application/x-msnmsgr-transrespbody" ))
			sttInitDirectTransfer2( hdrdata, tFileInfo, tFileInfo2 );
		break;

	case 2:
		sttAcceptTransfer( hdrdata, tFileInfo, tFileInfo2 );
		break;

	case 3:
		if ( !strcmp( szContentType, "application/x-msnmsgr-sessionclosebody" ))
		{
			filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
			if ( ft != NULL )
			{
				p2p_sendAck( ft->std.hContact, hdrdata );
				if ( ft->std.currentFileProgress < ft->std.currentFileSize )
					p2p_sendAbortSession(ft);
				else
					if ( !ft->std.sending ) ft->bCompleted = true;

				p2p_sessionComplete( ft );
			}
		}
		break;

	case 4:
		sttCloseTransfer( hdrdata, info, tFileInfo );
		break;

	default:
		p2p_sendAck( info->mJoinedContacts[0], hdrdata );
		break;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processMsg - processes all MSN P2P incoming messages

void  p2p_processMsg( ThreadData* info,  char* msgbody )
{
	P2P_Header* hdrdata = ( P2P_Header* )msgbody; msgbody += sizeof( P2P_Header );
	sttLogHeader( hdrdata );

	//---- if we got a message
	if ( hdrdata->mFlags == 0 && hdrdata->mSessionID == 0 )
	{
		if ( hdrdata->mPacketLen < hdrdata->mTotalSize )
		{
			char msgid[32];
			mir_snprintf(msgid, sizeof(msgid), "%08p_%08x", info->mJoinedContacts[0], hdrdata->mID );
			int idx = addCachedMsg(msgid, msgbody, (size_t)hdrdata->mOffset, hdrdata->mPacketLen, 
				(size_t)hdrdata->mTotalSize, false );

			char* newbody;
			size_t newsize;
			if (getCachedMsg( idx, newbody, newsize ))
			{
				p2p_processSIP( info, newbody, hdrdata );
				mir_free( newbody );
			}
			else
			{
				if ( hdrdata->mOffset + hdrdata->mPacketLen >= hdrdata->mTotalSize)
					clearCachedMsg(idx);
			}
		}
		else
			p2p_processSIP( info, msgbody, hdrdata );

		return;
	}

	filetransfer* ft = p2p_getSessionByID( hdrdata->mSessionID );
	if ( ft == NULL )
		ft = p2p_getSessionByUniqueID( hdrdata->mAckUniqueID );

	if ( ft == NULL ) return;

	ft->ts = time( NULL );

	//---- receiving redirect -----------
	if ( hdrdata->mFlags == 0x01 ) {
		if ( WaitForSingleObject( ft->hLockHandle, INFINITE ) == WAIT_OBJECT_0 ) {
			int dp = (int)(ft->std.currentFileProgress - hdrdata->mAckDataSize);
			ft->std.totalProgress -= dp ;
			ft->std.currentFileProgress -= dp;
			_lseeki64( ft->fileId, ft->std.currentFileProgress, SEEK_SET );
			ft->tType = info->mType;
			ReleaseMutex( ft->hLockHandle );
	}	}

	//---- receiving ack -----------
	if ( hdrdata->mFlags == 0x02 ) {
		ft->p2p_waitack = false;

		if ( hdrdata->mAckSessionID == ft->p2p_sendmsgid ) {
			if ( ft->p2p_appID == MSN_APPID_FILE ) {
				ft->bCompleted = true;
				p2p_sendBye( ft );
			}
			return;
		}

		if ( hdrdata->mAckSessionID == ft->p2p_byemsgid )
		{
			p2p_sessionComplete( ft );
			return;
		}

		switch( ft->p2p_ackID ) {
		case 1000:
			{
				//---- send Data Preparation Message
				P2P_Header tHdr = {0};
				tHdr.mSessionID = ft->p2p_sessionid;
				tHdr.mID = p2p_getMsgId( ft->std.hContact, 1 );
				tHdr.mAckSessionID = ft->p2p_acksessid;
				unsigned body = 0;

				p2p_sendMsg( ft->std.hContact, ft->p2p_appID, tHdr, ( char* )&body, sizeof( body ));

				ft->ts = time( NULL );
				ft->p2p_waitack = true;
			}
			break;

		case 1001:
			//---- send Data Messages
			MSN_StartP2PTransferByContact( ft->std.hContact );
			break;
		}

		ft->p2p_ackID++;
		return;
	}

	if ( hdrdata->mFlags == 0 ) 
	{
		//---- accept the data preparation message ------
		const unsigned* pLongs = ( unsigned* )msgbody;
		if (pLongs[0] == 0 && hdrdata->mPacketLen == 4) 
		{
			p2p_sendAck(ft->std.hContact, hdrdata);
			return;
		}
		else
			hdrdata->mFlags = 0x20;
	}

	//---- receiving data -----------
	if ( hdrdata->mFlags == 0x01000030 || hdrdata->mFlags == 0x20 ) 
	{
		if ( hdrdata->mOffset + hdrdata->mPacketLen > hdrdata->mTotalSize )
			hdrdata->mPacketLen = DWORD( hdrdata->mTotalSize - hdrdata->mOffset );

		if ( ft->tTypeReq == 0 || ft->tTypeReq == info->mType) {
			ft->tType = info->mType;
			ft->p2p_sendmsgid = hdrdata->mID;
		}

		__int64 dsz = ft->std.currentFileSize - hdrdata->mOffset;
		if ( dsz > hdrdata->mPacketLen) dsz = hdrdata->mPacketLen;

		if ( ft->tType == info->mType ) {
			if ( dsz > 0 ) {
				if (ft->lstFilePtr != hdrdata->mOffset)
					_lseeki64( ft->fileId, hdrdata->mOffset, SEEK_SET );
				_write( ft->fileId, msgbody, (unsigned int)dsz );

				__int64 dp = hdrdata->mOffset + dsz - ft->std.currentFileProgress;
				if ( dp > 0) {
					ft->std.totalProgress += (unsigned long)dp;
					ft->std.currentFileProgress += (unsigned long)dp;

					if ( ft->p2p_appID == MSN_APPID_FILE )
						MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
				}

				//---- send an ack: body was transferred correctly
				MSN_DebugLog( "Transferred %lu bytes out of %lu", ft->std.currentFileProgress, hdrdata->mTotalSize );
			}

			if ( ft->std.currentFileProgress >= hdrdata->mTotalSize ) {
				p2p_sendAck( ft->std.hContact, hdrdata );
				if ( ft->p2p_appID == MSN_APPID_FILE )
					ft->bCompleted = true;
				else {
					sttSavePicture2disk( ft );
					p2p_sendBye( ft );
				}
	}	}	}

	if ( hdrdata->mFlags == 0x40 || hdrdata->mFlags == 0x80 ) {
		p2p_sendAbortSession( ft );
		p2p_unregisterSession( ft );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_invite - invite another side to transfer an avatar

void  p2p_invite( HANDLE hContact, int iAppID, filetransfer* ft )
{
	const char* szAppID;
	switch( iAppID ) {
	case MSN_APPID_FILE:			szAppID = "{5D3E02AB-6190-11D3-BBBB-00C04F795683}";	break;
	case MSN_APPID_AVATAR:			szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	case MSN_APPID_CUSTOMSMILEY:	szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	case MSN_APPID_CUSTOMANIMATEDSMILEY:	szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	default:
		return;
	}

	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", hContact, szEmail, sizeof( szEmail ));

	srand( (unsigned)time( NULL ) );
	long sessionID = rand() << 16 | rand();

	if ( ft == NULL ) {
		ft = new filetransfer();
		ft->std.hContact = hContact;
	}

	ft->p2p_type = iAppID;
	ft->p2p_acksessid = rand() << 16 | rand();
	ft->p2p_sessionid = sessionID;
	ft->p2p_callID = getNewUuid();

	char* pContext;
	int   cbContext;

	switch ( iAppID ) 
	{
		case MSN_APPID_FILE:
			{
				cbContext = sizeof(HFileContext);
				pContext = (char*)malloc(cbContext);
				HFileContext* ctx = (HFileContext*)pContext;
				memset(pContext, 0, cbContext);
				ctx->len = cbContext;
				ctx->ver = 3; 
				ctx->type = MSN_TYPEID_FTNOPREVIEW;
				ctx->dwSize = ft->std.currentFileSize;
				ctx->id = 0xffffffff;
				if ( ft->wszFileName != NULL )
					wcsncpy( ctx->wszFileName, ft->wszFileName, MAX_PATH);
				else {
					char* pszFiles = strrchr( ft->std.currentFile, '\\' );
					if ( pszFiles )
						pszFiles++;
					else
						pszFiles = ft->std.currentFile;

					MultiByteToWideChar( CP_ACP, 0, pszFiles, -1, ctx->wszFileName, MAX_PATH);
				}
	
				ft->p2p_appID = MSN_APPID_FILE;
			}
			break;

		default:
			ft->std.sending = false;
			ft->p2p_appID = MSN_APPID_AVATAR2;

			{
				char tBuffer[2048];
				if ( iAppID == MSN_APPID_AVATAR )
					MSN_GetStaticString( "PictContext", hContact, tBuffer, sizeof( tBuffer ));
				else {
					strncpy( tBuffer, ft->p2p_object, sizeof( tBuffer ));
					tBuffer[ sizeof( tBuffer )-1 ] = 0;
				}

				ezxml_t xmlo = ezxml_parse_str(tBuffer, strlen(tBuffer));
				ezxml_t xmlr = ezxml_new("msnobj");
				
				const char* p;
				p = ezxml_attr(xmlo, "Creator");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Creator", p);
				p = ezxml_attr(xmlo, "Size");
				if (p != NULL) {
					ezxml_set_attr(xmlr, "Size", p);
					ft->std.totalBytes = ft->std.currentFileSize = atol(p);
				}
				p = ezxml_attr(xmlo, "Type");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Type", p);
				p = ezxml_attr(xmlo, "Location");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Location", p);
				p = ezxml_attr(xmlo, "Friendly");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Friendly", p);
				p = ezxml_attr(xmlo, "SHA1D");
				if (p != NULL)
					ezxml_set_attr(xmlr, "SHA1D", p);
				p = ezxml_attr(xmlo, "SHA1C");
				if (p != NULL)
					ezxml_set_attr(xmlr, "SHA1C", p);

				pContext = ezxml_toxml(xmlr, false);
				cbContext = strlen( pContext )+1;

				ezxml_free(xmlr);
				ezxml_free(xmlo);
			}

			if ( ft->create() == -1 ) {
				MSN_DebugLog( "Avatar creation failed for MSNCTX=\'%s\'", pContext );
				free(pContext);
				delete ft;
				return;
			}

			break;
	}

	if ( !p2p_sessionRegistered( ft )) {
		ft->p2p_dest = mir_strdup( szEmail );
		p2p_registerSession( ft );
	}

	size_t cbBody = Netlib_GetBase64EncodedBufferSize(cbContext) + 1000;
	char* body = ( char* )mir_alloc( cbBody );
	int tBytes = mir_snprintf( body, cbBody,
		"EUF-GUID: %s\r\n"
		"SessionID: %lu\r\n"
		"AppID: %d\r\n"
		"Context: ",
		szAppID, sessionID, ft->p2p_appID );

	NETLIBBASE64 nlb = { body+tBytes, cbBody-tBytes, (PBYTE)pContext, cbContext };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	cbBody = tBytes + nlb.cchEncoded;
	strcpy( body + cbBody - 1, "\r\n\r\n" );
	cbBody += 4;

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "0 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );
	tResult.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

	p2p_sendSlp( ft, tResult, -2, body, cbBody );
	mir_free( body );
	free(pContext);
}


void  p2p_sessionComplete( filetransfer* ft )
{
	if ( ft->std.sending ) {
		if ( ft->openNext() == -1 ) {
			bool success = ft->std.currentFileNumber >= ft->std.totalFiles && ft->bCompleted;
			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, success ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, ft, 0);
			p2p_unregisterSession( ft );
		}
		else {
			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0);
			p2p_invite( ft->std.hContact, ft->p2p_appID, ft );
		}
	}
	else {
		MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ft->bCompleted ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, ft, 0);
		p2p_unregisterSession( ft );
	}
}
