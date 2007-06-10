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

#pragma pack(1)

typedef struct
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

typedef struct
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
	MSN_DebugLog( "    Offset of data = %I64u", hdrdata->mOffset );
	MSN_DebugLog( "    Total amount of data = %I64u", hdrdata->mTotalSize );
	MSN_DebugLog( "    Data in packet = %lu bytes", hdrdata->mPacketLen );
	MSN_DebugLog( "    Flags = %08X", hdrdata->mFlags );
	MSN_DebugLog( "    Acknowledged session ID: %08X", hdrdata->mAckSessionID );
	MSN_DebugLog( "    Acknowledged message ID: %08X", hdrdata->mAckUniqueID );
	MSN_DebugLog( "    Acknowledged data size: %I64u", hdrdata->mAckDataSize );
	MSN_DebugLog( "------------------------" );
}

static char* getNewUuid()
{
	BYTE* p;
	UUID id;

	UuidCreate( &id );
	UuidToStringA( &id, &p );
	int len = strlen(( const char* )p );
	char* result = ( char* )mir_alloc( len+3 );
	result[0]='{';
	memcpy( result+1, p, len );
	result[ len+1 ] = '}';
	result[ len+2 ] = 0;
	strupr( result );
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


static int sttCreateListener(
	ThreadData* info,
	filetransfer* ft,
	directconnection *dc,
	char* szBody, size_t cbBody )
{
	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof( nlb );
	nlb.pfnNewConnectionV2 = MSN_ConnectionProc;
	HANDLE sb = (HANDLE) MSN_CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, ( LPARAM )&nlb);
	if ( sb == NULL ) {
		MSN_DebugLog( "Unable to bind the port for incoming transfers" );
		return 0;
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
	for( unsigned i=0; i<sizeof( hostname )/16 && he->h_addr_list[i] ; ++i ) {
		if ( i != 0 ) strcat( hostname, " " );
		strcat( hostname, inet_ntoa( *( PIN_ADDR )he->h_addr_list[i] ));
	}

	char* szUuid = dc->useHashedNonce ? dc->mNonceToHash() : dc->mNonceToText();

	int cbBodyLen = mir_snprintf( szBody, cbBody,
		"Bridge: TCPv1\r\n"
		"Listening: true\r\n"
		"%s: %s\r\n"
		"IPv4External-Addrs: %s\r\n"
		"IPv4External-Port: %u\r\n"
		"IPv4Internal-Addrs: %s\r\n"
		"IPv4Internal-Port: %u\r\n"
		"SessionID: %lu\r\n"
		"SChannelState: 0\r\n\r\n%c",
		dc->useHashedNonce ? "Hashed-Nonce" : "Nonce", szUuid,
		MyConnection.GetMyExtIPStr(), nlb.wExPort, hostname, nlb.wPort, ft->p2p_sessionid, 0 );

	mir_free( szUuid );

	return cbBodyLen;
}

/////////////////////////////////////////////////////////////////////////////////////////
// sttSavePicture2disk - final handler for avatars downloading

static void sttSavePicture2disk( filetransfer* ft )
{
	if ( ft->inmemTransfer == NULL )
		return;

	if ( ft->p2p_type == MSN_APPID_CUSTOMSMILEY || ft->p2p_type == MSN_APPID_CUSTOMANIMATEDSMILEY ) {
		char fileName[ MAX_PATH ];
		MSN_GetCustomSmileyFileName( ft->std.hContact, fileName, sizeof( fileName ), ft->std.currentFile, ft->p2p_type);

		int fileId = _open( fileName, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE );
		if ( fileId > -1 ) 
		{
			_write( fileId, ft->fileBuffer, ft->std.currentFileSize );
			_close( fileId );
		}
		return;
	}

	char tContext[ 256 ];
	if ( MSN_GetStaticString( "PictContext", ft->std.hContact, tContext, sizeof( tContext )))
		return;

	char* pshad = strstr( tContext, "SHA1D=\"" );
	if ( pshad == NULL )
		return;

	mir_sha1_ctx sha1ctx;
	BYTE sha[ MIR_SHA1_HASH_SIZE ];
	char szSha[ 40 ];
	NETLIBBASE64 nlb = { szSha, sizeof( szSha ), ( PBYTE )sha, sizeof( sha ) };

	mir_sha1_init( &sha1ctx );
	mir_sha1_append( &sha1ctx, ( BYTE* )ft->fileBuffer, ft->std.currentFileSize );
	mir_sha1_finish( &sha1ctx, sha );

	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	if ( strncmp( pshad + 7, szSha, strlen( szSha )) != 0 )
		return;

	PROTO_AVATAR_INFORMATION AI;
	AI.cbSize = sizeof( AI );
	AI.hContact = ft->std.hContact;
	MSN_GetAvatarFileName( AI.hContact, AI.filename, sizeof( AI.filename ));

	if ( *(unsigned short*)ft->fileBuffer == 0xd8ff )
	{
		AI.format =  PA_FORMAT_JPEG;
		strcat( AI.filename, "jpg" ); 
	}
	else
	{
		AI.format =  PA_FORMAT_PNG;
		strcat( AI.filename, "png" ); 
	}

	MSN_DebugLog( "Avatar for contact %08x saved to file '%s'", AI.hContact, AI.filename );
	int fileId = _open( AI.filename, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE );
	if ( fileId > -1 ) {
		_write( fileId, ft->fileBuffer, ft->std.currentFileSize );
		_close( fileId );

		MSN_SetString( ft->std.hContact, "PictSavedContext", tContext );
		MSN_SendBroadcast( AI.hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, &AI, NULL );

		// Store also avatar hash
		char* pshadEnd = strstr( pshad+7, "\"" );
		if ( pshadEnd != NULL )
			*pshadEnd = '\0';
		MSN_SetString( ft->std.hContact, "AvatarSavedHash", pshad+7 );
	}
	else {
		MSN_DeleteSetting( ft->std.hContact, "AvatarHash" );
		MSN_SendBroadcast( AI.hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, &AI, NULL );
}	}

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
		if ( MSN_GetUnconnectedThread( hContact ) == NULL )
			msnNsThread->sendPacket( "XFR", "SB" );
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
	
	if ( iKind < 0 )
		replaceStr(ft->p2p_branch, getNewUuid());

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
	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.flags = NLOCF_V2;
	tConn.szHost = info->mServer;
	tConn.timeout = 5;
	{
 		char* tPortDelim = strrchr( info->mServer, ':' );
		if ( tPortDelim != NULL ) {
			*tPortDelim = '\0';
			tConn.wPort = ( WORD )atol( tPortDelim+1 );
	}	}

	while( true ) {
		char* pSpace = strchr( info->mServer, ' ' );
		if ( pSpace != NULL )
			*pSpace = 0;

		MSN_DebugLog( "Connecting to %s:%d", info->mServer, tConn.wPort );

		HANDLE h = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
		if ( h != NULL ) {
			info->s = h;
			break;
		}
		{	TWinErrorCode err;
			MSN_DebugLog( "Connection Failed (%d): %s", err.mErrorCode, err.getText() );
		}

		if ( pSpace == NULL ) {
			MSN_StartP2PTransferByContact( info->mInitialContact );
			return false;
		}

		strdel( info->mServer, int( pSpace - info->mServer )+1 );
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

	if ( memcmp( p, p2p_greeting, 8 ) != NULL ) {
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
			ft->std.currentFileProgress < ft->std.currentFileSize )
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


/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processMsg - processes all MSN P2P incoming messages

static void sttInitFileTransfer(
	P2P_Header*		hdrdata,
	ThreadData*		info,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2,
	const char*		msgbody )
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
			if ( !strcmp( szEufGuid, "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}" )) {
				DBVARIANT dbv;
				bool pictmatch = !DBGetContactSetting( NULL, msnProtocolName, "PictObject", &dbv );
				if ( pictmatch ) 
				{
					char szCtBuf[32], szPtBuf[32];
					UrlDecode(dbv.pszVal);
					pictmatch &= txtParseParam( szContext,  "SHA1C=", "\"", "\"", szCtBuf, sizeof( szCtBuf ));
					pictmatch &= txtParseParam( dbv.pszVal, "SHA1C=", "\"", "\"", szPtBuf, sizeof( szPtBuf ));
					pictmatch = pictmatch && strcmp( szCtBuf, szPtBuf ) == 0;
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
						MSN_DebugLog( "Unable to open avatar file '%s', error %d", szFileName, errno );
						delete ft;
					}
					else
					{
						replaceStr(ft->std.currentFile, szFileName);
						MSN_DebugLog( "My avatar file opened for %s as %08p::%d", szContactEmail, ft, ft->fileId );
						ft->std.totalBytes = ft->std.currentFileSize = filelength( ft->fileId );
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
			if ( !strcmp( szEufGuid, "{5D3E02AB-6190-11D3-BBBB-00C04F795683}" )) 
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

		case 4:
			if ( !strcmp( szEufGuid, "{4BD96FC0-AB17-4425-A14A-439185962DC8}" )) {
				MSN_ShowPopup( ft->std.hContact,
					TranslateT( "Contact tried to send its webcam data (currently not supported)" ), 
					MSN_ALLOW_MSGBOX );
			}
			if ( !strcmp( szEufGuid, "{1C9AA97E-9C05-4583-A3BD-908A196F1E92}" )) {
				MSN_ShowPopup( ft->std.hContact,
					TranslateT( "Contact tried to view our webcam data (currently not supported)" ), 
					MSN_ALLOW_MSGBOX );
			}
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
	ThreadData*		info,
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
	if ( ft == NULL )
		return;

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

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);

	if ( p2p_isAvatarOnly( ft->std.hContact )) {
		p2p_sendStatus(ft, 1603);
		return;
	}

	directconnection *dc = new directconnection( ft );
	dc->useHashedNonce = szHashedNonce != NULL;
	if ( dc->useHashedNonce )
		dc->xNonce = mir_strdup( szHashedNonce );
	p2p_registerDC( dc );

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "1 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );

	MyConnectionType conType = {0};

	conType.extIP = atol( szNetID );
	conType.SetUdpCon(szConnType);
	conType.upnpNAT = strcmp(szUPnPNat, "true") == 0;
	conType.icf = strcmp(szICF, "true") == 0;
	conType.CalculateWeight();

	char szBody[ 512 ];
	int  cbBodyLen = 0;

	if (conType.weight > MyConnection.weight)
		cbBodyLen = sttCreateListener( info, ft, dc, szBody, sizeof( szBody ));

	if ( !cbBodyLen ) {
		const char* szUuid = dc->useHashedNonce ? dc->mNonceToHash() : sttVoidNonce;

		cbBodyLen = mir_snprintf( szBody, sizeof( szBody ),
			"Bridge: TCPv1\r\n"
			"Listening: false\r\n"
			"%s: %s\r\n"
			"SessionID: %u\r\n"
			"SChannelState: 0\r\n\r\n%c",
			dc->useHashedNonce ? "Hashed-Nonce" : "Nonce", szUuid, ft->p2p_sessionid, 0 );

		if ( dc->useHashedNonce ) mir_free(( void* )szUuid );
	}

	tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );

	p2p_getMsgId( ft->std.hContact, -2 );
	p2p_sendSlp( ft, tResult, 200, szBody, cbBodyLen );
	p2p_getMsgId( ft->std.hContact, 1 );
}

static void sttInitDirectTransfer2(
	P2P_Header*  hdrdata,
	ThreadData*  info,
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


	if ( !strcmp( szListening, "true" ) && strcmp( dc->xNonce, sttVoidNonce )) {
		bool extOk = szExternalAddress != NULL && szExternalPort != NULL;
		bool intOk = szInternalAddress != NULL && szInternalPort != NULL;

		ThreadData* newThread = new ThreadData;

		if ( extOk && ( strcmp( szExternalAddress, MyConnection.GetMyExtIPStr()) || !intOk ))
			mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szExternalAddress, szExternalPort );
		else if ( intOk )
			mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szInternalAddress, szInternalPort );
		else {
			MSN_DebugLog( "Invalid data packet, exiting..." );
			delete newThread;
			return;
		}

		newThread->mType = SERVER_P2P_DIRECT;
		newThread->mInitialContact = ft->std.hContact;

		strncpy( newThread->mCookie, szCallID, sizeof( newThread->mCookie ));
		newThread->startThread(( pThreadFunc )p2p_fileActiveThread );
	}
}

static void sttAcceptTransfer(
	P2P_Header*		hdrdata,
	ThreadData*		info,
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

	bool bAllowIncoming = ( !( MyOptions.UseGateway || MyOptions.UseProxy ) || !MSN_GetByte( "AutoGetHost", 1 ));

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "0 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );

	char* szBody = ( char* )alloca( 1024 );
	int   cbBody = 0;
	if ( !strcmp( szOldContentType, "application/x-msnmsgr-sessionreqbody" )) {
		p2p_sendFeedStart( ft );

		ThreadData* T = MSN_GetP2PThreadByContact( ft->std.hContact );
		if ( T != NULL && T->mType == SERVER_P2P_DIRECT )
			return;

		if ( MyOptions.UseGateway )
			return;

		directconnection* dc = new directconnection( ft );
		p2p_registerDC( dc );

		char* hn = dc->mNonceToHash();
		char* szNonce = ( char* )alloca( 256 );
		mir_snprintf( szNonce, 256, "Hashed-Nonce: %s\r\n", hn );

		mir_free( hn );

		tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );
		cbBody = mir_snprintf( szBody, 1024,
			"Bridges: TCPv1\r\nNetID: %i\r\nConn-Type: %s\r\nUPnPNat: %s\r\nICF: %s\r\n%s\r\n%c",
			MyConnection.extIP, MyConnection.GetMyUdpConStr(), 
			MyConnection.upnpNAT ? "true" : "false", MyConnection.icf ? "true" : "false", 
			szNonce, 0 );

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
		if ( !strcmp( szListening, "true" ) && ( szNonce == NULL || strcmp( szNonce, sttVoidNonce ))) {
			bool extOk = szExternalAddress != NULL && szExternalPort != NULL;
			bool intOk = szInternalAddress != NULL && szInternalPort != NULL;

			ThreadData* newThread = new ThreadData;

			if ( extOk && ( strcmp( szExternalAddress, MyConnection.GetMyExtIPStr()) || !intOk ))
				mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szExternalAddress, szExternalPort );
			else if ( intOk )
				mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szInternalAddress, szInternalPort );
			else {
				MSN_DebugLog( "Invalid data packet, exiting..." );
				delete newThread;
				return;
			}

			newThread->mType = SERVER_P2P_DIRECT;
			strncpy( newThread->mCookie, szCallID, sizeof( newThread->mCookie ));
			newThread->mCookie[sizeof( newThread->mCookie )-1] = 0;
			newThread->mInitialContact = ft->std.hContact;
			newThread->startThread(( pThreadFunc )p2p_fileActiveThread );
			return;
		}

		cbBody = sttCreateListener( info, ft, dc, szBody, 1024 );

		// no, send a file via server
		if ( cbBody == 0 ) {
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

		cbBody = sttCreateListener( info, ft, dc, szBody, 1024 );

		// no, send a file via server
		if ( cbBody == 0 ) {
			MSN_StartP2PTransferByContact( ft->std.hContact );
			return;
		}

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else goto LBL_Close;

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

void  p2p_processMsg( ThreadData* info,  const char* msgbody )
{
	P2P_Header* hdrdata = ( P2P_Header* )msgbody; msgbody += sizeof( P2P_Header );
	sttLogHeader( hdrdata );

	//---- if we got a message
	if ( hdrdata->mFlags == 0 && hdrdata->mSessionID == 0 )
	{
		char* newbody = NULL;

		if ( hdrdata->mPacketLen < hdrdata->mTotalSize )
		{
			char msgid[32];
			sprintf(msgid, "%08x_%08x", info->mJoinedContacts[0], hdrdata->mID );
			int idx = addCachedMsg(msgid, msgbody, (size_t)hdrdata->mOffset, hdrdata->mPacketLen, 
				(size_t)hdrdata->mTotalSize, false );

			size_t newsize;
			if (!getCachedMsg( idx, newbody, newsize ))
			{
				if ( hdrdata->mOffset + hdrdata->mPacketLen >= hdrdata->mTotalSize)
					clearCachedMsg(idx);
				return;
			}
			msgbody = newbody;
		}

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

		const char* peol = strstr( msgbody, "\r\n" );
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

		switch( iMsgType ) {
		case 1:
			if ( !strcmp( szContentType, "application/x-msnmsgr-sessionreqbody" ))
				sttInitFileTransfer( hdrdata, info, tFileInfo, tFileInfo2, msgbody );
			else if ( !strcmp( szContentType, "application/x-msnmsgr-transreqbody" ))
				sttInitDirectTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
			else if ( !strcmp( szContentType, "application/x-msnmsgr-transrespbody" ))
				sttInitDirectTransfer2( hdrdata, info, tFileInfo, tFileInfo2 );
			break;

		case 2:
			sttAcceptTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
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
		mir_free( newbody );
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

	if ( hdrdata->mFlags == 0 ) {
		//---- accept the data preparation message ------
		const unsigned* pLongs = ( unsigned* )msgbody;
		if ( pLongs[0] == 0 && pLongs[1] == htonl(ft->p2p_appID) && hdrdata->mPacketLen == 4 ) {
			p2p_sendAck( ft->std.hContact, hdrdata );
			return;
	}	}

	//---- receiving data -----------
	if ( hdrdata->mFlags == 0x01000030 || hdrdata->mFlags == 0x20 ) {

		if ( hdrdata->mOffset + hdrdata->mPacketLen > hdrdata->mTotalSize )
			hdrdata->mPacketLen = DWORD( hdrdata->mTotalSize - hdrdata->mOffset );

		if ( ft->p2p_sendmsgid == 0 ) {
			ft->tType = info->mType;
			ft->p2p_sendmsgid = hdrdata->mID;
		}

		int sk = 0;

		__int64 dsz = ft->std.currentFileSize - hdrdata->mOffset;
		if ( dsz > hdrdata->mPacketLen) dsz = hdrdata->mPacketLen;

		if ( ft->tType == info->mType ) {
			if ( dsz > 0 ) {
				if ( ft->inmemTransfer )
					memcpy( ft->fileBuffer + hdrdata->mOffset, msgbody, (size_t)dsz );
				else {
					::_lseeki64( ft->fileId, hdrdata->mOffset, SEEK_SET );
					::_write( ft->fileId, msgbody, (unsigned int)dsz );
				}

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

	ThreadData *thread = MSN_GetThreadByContact( hContact );

	srand( (unsigned)time( NULL ) );
	long sessionID = rand() << 16 | rand();

	if ( ft == NULL ) {
		ft = new filetransfer();
		ft->std.hContact = hContact;
	}

	ft->p2p_type = iAppID;
	ft->p2p_acksessid = rand() << 16 | rand();
	ft->p2p_sessionid = sessionID;
	ft->p2p_branch = getNewUuid();
	ft->p2p_callID = getNewUuid();

	if ( !p2p_sessionRegistered( ft )) {
		ft->p2p_dest = mir_strdup( szEmail );
		p2p_registerSession( ft );
	}

	BYTE* pContext;
	int   cbContext;
	char* p;
	char tBuffer[ 1024 ];

	switch ( iAppID ) 
	{
		case MSN_APPID_FILE:
			{
				HFileContext ctx = {0};
				ctx.len = sizeof( ctx );
				ctx.ver = 3; 
				ctx.type = MSN_TYPEID_FTNOPREVIEW;
				ctx.dwSize = ft->std.currentFileSize;
				ctx.id = 0xffffffff;
				if ( ft->wszFileName != NULL )
					wcsncpy( ctx.wszFileName, ft->wszFileName, sizeof( ctx.wszFileName ));
				else {
					char* pszFiles = strrchr( ft->std.currentFile, '\\' );
					if ( pszFiles )
						pszFiles++;
					else
						pszFiles = ft->std.currentFile;

					MultiByteToWideChar( CP_ACP, 0, pszFiles, -1, ctx.wszFileName, sizeof( ctx.wszFileName ));
				}
				pContext = ( BYTE* )&ctx;
				cbContext = sizeof( ctx );
				ft->p2p_appID = MSN_APPID_FILE;
			}
			break;

		default:
			ft->inmemTransfer = true;
			ft->fileBuffer = NULL;
			ft->std.sending = false;
			ft->p2p_appID = MSN_APPID_AVATAR2;

			if ( iAppID == MSN_APPID_AVATAR )
				MSN_GetStaticString( "PictContext", hContact, tBuffer, sizeof( tBuffer ));
			else {
				strncpy( tBuffer, ft->p2p_object, sizeof( tBuffer ));
				tBuffer[ sizeof( tBuffer )-1 ] = 0;
			}

			p = strstr( tBuffer, "Size=\"" );
			if ( p != NULL )
				ft->std.totalBytes = ft->std.currentFileSize = atol( p + 6 );

			p = strstr( tBuffer, "SHA1C=\"" );
			if ( p != NULL ) p = strchr( p+7, '\"' );
			if ( p != NULL ) strcpy(p+1, "/>");

			if ( ft->create() == -1 ) {
				MSN_DebugLog( "Avatar creation failed for MSNCTX=\'%s\'", tBuffer );
				return;
			}

			pContext = ( BYTE* )tBuffer;
			cbContext = strlen( tBuffer )+1;
			break;
	}

	size_t cbBody = Netlib_GetBase64EncodedBufferSize(cbContext) + 1000;
	char* body = ( char* )mir_alloc( cbBody );
	int tBytes = mir_snprintf( body, cbBody,
		"EUF-GUID: %s\r\n"
		"SessionID: %lu\r\n"
		"AppID: %d\r\n"
		"Context: ",
		szAppID, sessionID, ft->p2p_appID );

	NETLIBBASE64 nlb = { body+tBytes, cbBody-tBytes, pContext, cbContext };
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
