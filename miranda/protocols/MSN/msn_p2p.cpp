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

#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

static char sttP2Pheader[] = 
	"Content-Type: application/x-msnmsgrp2p\r\n"
	"P2P-Dest: %s\r\n\r\n";

static char sttVoidNonce[] = "{00000000-0000-0000-0000-000000000000}";

static void sttLogHeader( P2P_Header* hdrdata )
{
	MSN_DebugLog( "--- Printing message header" );
	MSN_DebugLog( "    SessionID = %ld", hdrdata->mSessionID );
	MSN_DebugLog( "    MessageID = %ld", hdrdata->mID );
	MSN_DebugLog( "    Offset of data = %I64i", hdrdata->mOffset );
	MSN_DebugLog( "    Total amount of data = %I64i", hdrdata->mTotalSize );
	MSN_DebugLog( "    Data in packet = %ld bytes", hdrdata->mPacketLen );
	MSN_DebugLog( "    Flags = %08X", hdrdata->mFlags );
	MSN_DebugLog( "    Acknowledged session ID: %ld", hdrdata->mAckSessionID );
	MSN_DebugLog( "    Acknowledged message ID: %ld", hdrdata->mAckUniqueID );
	MSN_DebugLog( "    Acknowledged data size: %I64i", hdrdata->mAckDataSize );
	MSN_DebugLog( "------------------------" );
}

static char* getNewUuid()
{
	BYTE* p;
	UUID id;

	UuidCreate( &id );
	UuidToString( &id, &p );
	int len = strlen(( const char* )p );
	char* result = ( char* )malloc( len+3 );
	result[0]='{';
	memcpy( result+1, p, len );
	result[ len+1 ] = '}';
	result[ len+2 ] = 0;
	strupr( result );
	RpcStringFree( &p );
	return result;
}

static int sttCreateListener( filetransfer* ft, pThreadFunc thrdFunc, char* szBody, size_t cbBody )
{
	char ipaddr[256];
	if ( MSN_GetMyHostAsString( ipaddr, sizeof ipaddr )) {
		MSN_DebugLog( "Cannot detect my host address" );
		return 0;
	}

	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof NETLIBBIND;
	nlb.pfnNewConnection = MSN_ConnectionProc;
	nlb.wPort = 0;	// Use user-specified incoming port ranges, if available
	if (( ft->mIncomingBoundPort = (HANDLE) CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, (LPARAM) &nlb)) == NULL ) {
		MSN_DebugLog( "Unable to bind the port for incoming transfers" );
		return 0;
	}
	
	ft->mIncomingPort = nlb.wPort;

	char* szUuid = getNewUuid();
	{
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_FILETRANS;
		newThread->mCaller = 3;
		newThread->mTotalSend = 0;
		newThread->mP2pSession = ft;
		strncpy( newThread->mCookie, ( char* )szUuid, sizeof newThread->mCookie );
		ft->hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		newThread->startThread( thrdFunc );
	}

	int cbBodyLen = _snprintf( szBody, cbBody,
		"Bridge: TCPv1\r\n"
		"Listening: true\r\n"
		"Nonce: %s\r\n"
		"IPv4Internal-Addrs: %s\r\n"
		"IPv4Internal-Port: %d\r\n\r\n%c",
		szUuid, ipaddr, ft->mIncomingPort, 0 );
	free( szUuid );

	return cbBodyLen;
}

/////////////////////////////////////////////////////////////////////////////////////////
// sttSavePicture2disk - final handler for avatars downloading 

static void sttSavePicture2disk( ThreadData* info, filetransfer* ft )
{
	if ( !ft->inmemTransfer )
		return;

	//---- Save temporary PNG image to disk --------------------
	#if defined( _DEBUG )
		char* Path = getenv( "TEMP" );
		if ( Path == NULL )
			Path = getenv( "TMP" );

		char tPathName[ MAX_PATH ];
		if ( Path == NULL ) {
			MSN_DebugLog( "Temporary file is created in the current directory: %s",
			getcwd( tPathName, sizeof( tPathName )));
		}
		else {
			MSN_DebugLog( "Temporary path found: %s", Path );
			strcpy( tPathName, Path );
		}

		strcat( tPathName, "\\avatar.png" );
		MSN_DebugLog( "Opening temporary file '%s'", tPathName );
		{	FILE* out = fopen( tPathName, "wb" );
			if ( out ) {
				fwrite( ft->fileBuffer, ft->std.totalBytes, 1, out );
				fclose( out );
		}	}
	#endif

	//---- Converting memory buffer to bitmap and saving it to disk
	if ( !MSN_LoadPngModule() )
		return;

	BITMAPINFOHEADER* pDib;
	if ( !png2dibConvertor( ft->fileBuffer, ft->std.totalBytes, &pDib ))
		return;

	PROTO_AVATAR_INFORMATION AI;
	AI.cbSize = sizeof AI;
	AI.format = PA_FORMAT_BMP;
	AI.hContact = info->mJoinedContacts[0];
	MSN_GetAvatarFileName( info->mJoinedContacts[0], AI.filename, sizeof AI.filename );
	FILE* out = fopen( AI.filename, "wb" );
	if ( out != NULL ) {
		BITMAPFILEHEADER tHeader = { 0 };
		tHeader.bfType = 0x4d42;
		tHeader.bfOffBits = sizeof( tHeader ) + sizeof( BITMAPINFOHEADER );
		tHeader.bfSize = tHeader.bfOffBits + pDib->biSizeImage;
		fwrite( &tHeader, sizeof( tHeader ), 1, out );
		fwrite( pDib, sizeof( BITMAPINFOHEADER ), 1, out );
		fwrite( pDib+1, pDib->biSizeImage, 1, out );
		fclose( out );
	
		MSN_SendBroadcast( info->mJoinedContacts[0], ACKTYPE_AVATAR, ACKRESULT_SUCCESS, HANDLE( &AI ), NULL );
	}
	else MSN_SendBroadcast( info->mJoinedContacts[0], ACKTYPE_AVATAR, ACKRESULT_FAILED, HANDLE( &AI ), NULL );

   GlobalFree( pDib );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendAck - sends MSN P2P acknowledgement to the received message

static char sttVoidSession[] = "ACHTUNG!!! an attempt made to send a message via the empty session";

void __stdcall p2p_sendAck( filetransfer* ft, ThreadData* info, P2P_Header* hdrdata )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	char* buf = ( char* )alloca( 1000 + strlen( ft->p2p_dest ));
	char* p = buf + sprintf( buf, sttP2Pheader, ft->p2p_dest );

	P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
	memset( tHdr, 0, sizeof P2P_Header );
	tHdr->mSessionID = hdrdata->mSessionID;
	tHdr->mID = ft->p2p_msgid++;
	tHdr->mAckDataSize = tHdr->mTotalSize = hdrdata->mTotalSize;
	tHdr->mFlags = 2;
	tHdr->mAckSessionID = hdrdata->mID;
	tHdr->mAckUniqueID = hdrdata->mAckSessionID;
	*( long* )p = 0; p += sizeof( long );
	info->sendRawMessage( 'D', buf, int( p - buf ));
}

static int sttDigits( long iNumber )
{
	char tBuffer[ 20 ];
	ltoa( iNumber, tBuffer, 10 );
	return strlen( tBuffer );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendSlp - send MSN P2P SLP packet

void __stdcall p2p_sendSlp( 
	ThreadData*		info, 
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

	char szMyEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", NULL, szMyEmail, sizeof( szMyEmail ));

	char* buf = ( char* )alloca( 1000 + szContLen + pHeaders.getLength());
	char* p = buf;
	
	if ( !ft->mIsDirect )
		p += sprintf( p, sttP2Pheader, ft->p2p_dest );

	P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
	memset( tHdr, 0, sizeof P2P_Header );

	char* pktStart = p;

	switch ( iKind ) {
		case 200: p += sprintf( p, "MSNSLP/1.0 200 OK" );	break;
		case 603: p += sprintf( p, "MSNSLP/1.0 603 DECLINE" ); break;
		case -1:  p += sprintf( p, "BYE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest ); break;
		case -2:	 p += sprintf( p, "INVITE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest ); break;
		default: return;
	}

	p += sprintf( p,
		"\r\nTo: <msnmsgr:%s>\r\n"
		"From: <msnmsgr:%s>\r\n"
		"Via: MSNSLP/1.0/TLP ;branch=%s\r\n", ft->p2p_dest, szMyEmail, ft->p2p_branch );

	p = pHeaders.writeToBuffer( p );

	p += sprintf( p, "Content-Length: %d\r\n\r\n", szContLen );
	memcpy( p, szContent, szContLen );
	p += szContLen;
	tHdr->mID = ft->p2p_msgid++;
	tHdr->mAckSessionID = ft->p2p_acksessid;
	tHdr->mTotalSize = tHdr->mPacketLen = int( p - pktStart );

	*( long* )p = 0; p += sizeof( long );

	if ( ft->mIsDirect ) {
		DWORD tLen = int( p - buf );
		MSN_WS_Send( info->s, ( char* )&tLen, sizeof( DWORD ));
		MSN_WS_Send( info->s, buf, tLen );
	}
	else {
		if ( info == NULL ) {
			MsgQueue_Add( ft->std.hContact, buf, int( p - buf ), NULL );
			MSN_SendPacket( msnNSSocket, "XFR", "SB" );
		}
		else info->sendRawMessage( 'D', buf, int( p - buf ));
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendBye - closes P2P session

void __stdcall p2p_sendBye( ThreadData* info, filetransfer* ft )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addLong( "CSeq", 0 );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionclosebody" );

	p2p_sendSlp( info, ft, tHeaders, -1, "\r\n\x00", 3 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendStatus - send MSN P2P status and its description

void __stdcall p2p_sendStatus( filetransfer* ft, ThreadData* info, long lStatus )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders( 5 );
	tHeaders.addLong( "CSeq", 1 );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

	char szContents[ 50 ];
	p2p_sendSlp( info, ft, tHeaders, lStatus, szContents,
		_snprintf( szContents, sizeof szContents, "SessionID: %ld\r\n\r\n%c", ft->p2p_sessionid, 0 ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_connectTo - connects to a remote P2P server

static char p2p_greeting[8] = { 4, 0, 0, 0, 'f', 'o', 'o', 0  };

static void sttSendPacket( HANDLE s, P2P_Header& hdr )
{
	DWORD len = sizeof P2P_Header;
	MSN_WS_Send( s, ( char* )&len, sizeof DWORD );
	MSN_WS_Send( s, ( char* )&hdr, sizeof P2P_Header );
}

bool p2p_connectTo( ThreadData* info )
{
	P2P_Header reply;

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.szHost = info->mServer;
	{
 		char* tPortDelim = strrchr( info->mServer, ':' );
		if ( tPortDelim != NULL ) {
			*tPortDelim = '\0';
			tConn.wPort = ( WORD )atol( tPortDelim+1 );
	}	}

	filetransfer* ft = info->mP2pSession;

	if (( info->s = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn )) == NULL ) {
		{	TWinErrorCode err;
			MSN_DebugLog( "Connection Failed (%d): %s", err.mErrorCode, err.getText() );
		}

		return false;
	}

	MSN_WS_Send( info->s, p2p_greeting, sizeof p2p_greeting );

	memset( &reply, 0, sizeof P2P_Header );
	reply.mID = ft->p2p_msgid++;
	reply.mFlags = 0x100;

	strdel( info->mCookie, 1 );
	info->mCookie[ strlen( info->mCookie )-1 ] = 0;
	UuidFromString(( BYTE* )info->mCookie, ( UUID* )&reply.mAckSessionID );
	sttSendPacket( info->s, reply );

	long cbPacketLen;
	HReadBuffer buf( info, 0 );
	BYTE* p;
	if (( p = buf.surelyRead( info->s, 4 )) == NULL ) {
		MSN_DebugLog( "Error reading data, closing filetransfer" );
		return false;
	}

	cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( info->s, cbPacketLen )) == NULL ) 
		return false;

	ft->mIsDirect = true;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_listen - acts like a local P2P server

bool p2p_listen( ThreadData* info )
{
	filetransfer* ft = info->mP2pSession;

	switch( WaitForSingleObject( ft->hWaitEvent, 60000 )) {
	case WAIT_TIMEOUT:
	case WAIT_FAILED:
		MSN_DebugLog( "Incoming connection timed out, closing file transfer" );
LBL_Error:
		MSN_DebugLog( "File transfer failed" );
		return false;
	}

	HReadBuffer buf( info, 0 );
	BYTE* p;

	if (( p = buf.surelyRead( info->s, 8 )) == NULL )
		goto LBL_Error;

	if ( memcmp( p, p2p_greeting, 8 ) != NULL ) {
		MSN_DebugLog( "Invalid input data, exiting" );
		goto LBL_Error;
	}

	if (( p = buf.surelyRead( info->s, 4 )) == NULL ) {
		MSN_DebugLog( "Error reading data, closing filetransfer" );
		goto LBL_Error;
	}

	long cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( info->s, cbPacketLen )) == NULL ) 
		goto LBL_Error;
	
	UUID uuidCookie;
	strdel( info->mCookie, 1 );
	info->mCookie[ strlen(info->mCookie)-1 ] = 0;
	UuidFromString(( BYTE* )info->mCookie, &uuidCookie );

	P2P_Header* pCookie = ( P2P_Header* )p;
	if ( memcmp( &pCookie->mAckSessionID, &uuidCookie, sizeof( UUID ))) {
		MSN_DebugLog( "Invalid input cookie, exiting" );
		goto LBL_Error;
	}

	ft->mIsDirect = true;
	pCookie->mID = ft->p2p_msgid++;
	sttSendPacket( info->s, *pCookie );
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_receiveFile - receives a file via MSN P2P protocol

void p2p_receiveFile( ThreadData* info )
{
	filetransfer* ft = info->mP2pSession;
	BYTE* p;
	P2P_Header reply;
	HReadBuffer buf( info, 0 );

	if ( ft->create() == -1 ) {
		p2p_sendBye( info, ft );
		return;
	}

	while( true ) {
		if (( p = buf.surelyRead( info->s, 4 )) == NULL ) {
LBL_Error:
			MSN_DebugLog( "File transfer failed" );
			return;
		}

		long cbPacketLen = *( long* )p;
		if (( p = buf.surelyRead( info->s, cbPacketLen )) == NULL )
			goto LBL_Error;

		P2P_Header* H = ( P2P_Header* )p; p += sizeof( P2P_Header );
		sttLogHeader( H );
		
		if ( H->mFlags == 0x01000030 ) {
			::write( ft->fileId, p, H->mPacketLen );

			ft->std.totalProgress += H->mPacketLen;
			ft->std.currentFileProgress += H->mPacketLen;
			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );

			if ( H->mOffset + H->mPacketLen >= H->mTotalSize ) {
				memset( &reply, 0, sizeof P2P_Header );
				reply.mSessionID = H->mSessionID;
				reply.mID = ft->p2p_msgid++;
				reply.mFlags = 2;
				reply.mTotalSize = reply.mAckDataSize = H->mTotalSize;
				reply.mAckSessionID = H->mID;
				reply.mAckUniqueID = ft->p2p_acksessid;
				sttSendPacket( info->s, reply );
		}	}

		if ( H->mFlags == 2 && ft->bCanceled )
			break;

		if ( H->mFlags == 0x40 || ( H->mFlags == 0 && !memcmp( p, "BYE MSNMSGR:", 12 ))) {
			memset( &reply, 0, sizeof P2P_Header );
			reply.mID = ft->p2p_msgid++;
			reply.mFlags = 2;
			reply.mTotalSize = reply.mAckDataSize = H->mPacketLen;
			reply.mAckSessionID = H->mID;
			reply.mAckUniqueID = ft->p2p_acksessid;
			sttSendPacket( info->s, reply );

			if ( ft->std.totalProgress < ft->std.totalBytes )
				goto LBL_Error;
			break;
	}	}

	MSN_DebugLog( "File transfer succeeded" );
	ft->complete();
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFileDirectly - sends a file via MSN P2P protocol

void p2p_sendFileDirectly( ThreadData* info )
{
	filetransfer* ft = info->mP2pSession;
	BYTE* p;
	P2P_Header reply, *H;
	long portion;

	for ( unsigned long size = 0; size < ft->std.totalBytes; size += 1352 ) {
		if ( ft->bCanceled ) {
			p2p_sendBye( info, ft );
			MSN_DebugLog( "File transfer canceled" );
			return;
		}

		portion = 1352;
		if ( portion + size > ft->std.totalBytes )
			portion = ft->std.totalBytes - size;

		char databuf[ 1500 ];
		H = ( P2P_Header* )databuf;
		memset( H, 0, sizeof( P2P_Header ));
		H->mSessionID = ft->p2p_sessionid;
		H->mID = ft->p2p_msgid;
		H->mFlags = 0x01000030;
		H->mTotalSize = H->mAckDataSize = ft->std.currentFileSize;
		H->mOffset = size;
		H->mPacketLen = portion;
		H->mAckSessionID = ft->p2p_acksessid;

		::read( ft->fileId, databuf + sizeof( P2P_Header ), portion );
		portion += sizeof( P2P_Header );

		if ( !MSN_WS_Send( info->s, (char*)&portion, 4 )) {
LBL_Error:
			MSN_DebugLog( "File transfer failed" );
			return;
		}

		if ( !MSN_WS_Send( info->s, databuf, portion ))
			goto LBL_Error;

		ft->std.totalProgress += H->mPacketLen;
		ft->std.currentFileProgress += H->mPacketLen;
		MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
	}

	HReadBuffer buf( info, 0 );
	while( true ) {
		if (( p = buf.surelyRead( info->s, 4 )) == NULL )
			goto LBL_Error;

		long cbPacketLen = *( long* )p;
		if (( p = buf.surelyRead( info->s, cbPacketLen )) == NULL )
			goto LBL_Error;

		P2P_Header* H = ( P2P_Header* )p; p += sizeof( P2P_Header );
		sttLogHeader( H );

		if ( H->mFlags == 0x40 || ( H->mFlags == 0 && !memcmp( p, "BYE MSNMSGR:", 12 ))) {
			memset( &reply, 0, sizeof P2P_Header );
			reply.mID = ft->p2p_msgid++;
			reply.mFlags = 2;
			reply.mTotalSize = reply.mAckDataSize = H->mPacketLen;
			reply.mAckSessionID = H->mID;
			reply.mAckUniqueID = ft->p2p_acksessid;
			sttSendPacket( info->s, reply );

			if ( ft->std.totalProgress < ft->std.totalBytes )
				goto LBL_Error;

			break;
		}

		if ( H->mFlags == 2 && H->mTotalSize == ft->std.currentFileSize ) {
			p2p_sendBye( info, ft );
			break;
	}	}

	MSN_DebugLog( "File transfer succeeded" );
	ft->complete();
}

/////////////////////////////////////////////////////////////////////////////////////////
// bunch of thread functions to cover all variants of P2P file transfers

void __cdecl p2p_fileActiveRecvThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_fileActiveRecvThread() started: connecting to '%s'", info->mServer );

	if ( p2p_connectTo( info )) {
		p2p_receiveFile( info );
		p2p_unregisterSession( info->mP2pSession );
}	}

void __cdecl p2p_filePassiveRecvThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_filePassiveRecvThread() started: listening" );

	if ( p2p_listen( info )) {
		p2p_receiveFile( info );
		p2p_unregisterSession( info->mP2pSession );
}	}

void __cdecl p2p_fileActiveSendThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_fileActiveSendThread() started: listening" );

	if ( p2p_listen( info )) {
		p2p_sendFileDirectly( info );
		p2p_unregisterSession( info->mP2pSession );
}	}

void __cdecl p2p_filePassiveSendThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_filePassiveSendThread() started: connecting to '%s'", info->mServer );

	if ( p2p_connectTo( info )) {
		p2p_sendFileDirectly( info );
		p2p_unregisterSession( info->mP2pSession );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendViaServer - sends a file via server

void p2p_sendViaServer( filetransfer* ft, ThreadData* T )
{
	for ( unsigned long size = 0; size < ft->std.totalBytes; size += 1202 ) {
		if ( ft->bCanceled ) {
			p2p_sendBye( T, ft );
			MSN_DebugLog( "File transfer canceled" );
			return;
		}

		long portion = 1202;
		if ( portion + size > ft->std.totalBytes )
			portion = ft->std.totalBytes - size;

		char databuf[ 1500 ], *p = databuf;
		p += sprintf( p, sttP2Pheader, ft->p2p_dest );
		P2P_Header* H = ( P2P_Header* )p; p += sizeof( P2P_Header );
		memset( H, 0, sizeof( P2P_Header ));
		H->mSessionID = ft->p2p_sessionid;
		H->mID = ft->p2p_msgid;
		H->mFlags = 0x020;
		H->mTotalSize = ft->std.currentFileSize;
		H->mOffset = size;
		H->mPacketLen = portion;
		H->mAckSessionID = ft->p2p_acksessid;

		::read( ft->fileId, p, portion );

		p += portion;
		*( long* )p = 0x02000000; p += sizeof( long );

		T->sendRawMessage( 'D', databuf, int( p - databuf ));

		ft->std.totalProgress += H->mPacketLen;
		ft->std.currentFileProgress += H->mPacketLen;
		MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
	}

	ft->p2p_ackID = 3000;
	MSN_DebugLog( "File transfer succeeded" );
	ft->complete();
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processMsg - processes all MSN P2P incoming messages

static void sttInitFileTransfer( 
	P2P_Header*		hdrdata, 
	ThreadData*		info, 
	MimeHeaders&	tFileInfo, 
	MimeHeaders&	tFileInfo2, 
	char*				msgbody )
{
	char szMyEmail[ MSN_MAX_EMAIL_LEN ], szContactEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_GetStaticString( "e-mail", info->mJoinedContacts[0], szContactEmail, sizeof szContactEmail ))
		return;

	MSN_GetStaticString( "e-mail", NULL, szMyEmail, sizeof( szMyEmail ));

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

	char* szContext = NULL;
	long dwAppID = -1;

	const char	*szSessionID = tFileInfo2[ "SessionID" ],
					*szEufGuid = tFileInfo2[ "EUF-GUID" ];
	{	const char* p = tFileInfo2[ "AppID" ];
		if ( p )
			dwAppID = atol( p );
	}
	{	const char* p = tFileInfo2[ "Context" ];
		if ( p ) {
			int cbLen = strlen( p );
			szContext = ( char* )alloca( cbLen+1 );
			
			NETLIBBASE64 nlb = { ( char* )p, cbLen, ( PBYTE )szContext, cbLen };
			MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	}	}

	if ( szContext == NULL && memcmp( msgbody, "Context: ", 9 ) == 0 ) {
		msgbody += 9;
		int cbLen = strlen( msgbody );
		if ( cbLen > 252 )
			cbLen = 252;
		szContext = ( char* )alloca( cbLen+1 );
		
		NETLIBBASE64 nlb = { msgbody, cbLen, ( PBYTE )szContext, cbLen };
		MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	}

	if ( szSessionID == NULL || dwAppID == -1 || szEufGuid == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: SessionID='%s', AppID=%ld, Branch='%s'", szSessionID, dwAppID, szEufGuid );
		return;
	}

	srand( time( NULL ));

	filetransfer* ft = new filetransfer();
	ft->p2p_appID = dwAppID;
	ft->p2p_acksessid = 0x024F0000 + rand();
	ft->p2p_sessionid = atol( szSessionID );
	ft->p2p_msgid = ft->p2p_acksessid + 5;
	ft->p2p_ackID = ft->p2p_appID * 1000;
	ft->p2p_callID = strdup( szCallID );
	ft->p2p_branch = strdup( szBranch );
	ft->p2p_dest = strdup( szContactEmail );
	ft->mOwnsThread = info->mMessageCount == 0;

	if ( dwAppID == 1 && !strcmp( szEufGuid, "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}" )) {
		char szFileName[ MAX_PATH ];
		MSN_GetAvatarFileName( NULL, szFileName, sizeof szFileName );
		ft->fileId = open( szFileName, O_RDONLY | _O_BINARY, _S_IREAD | _S_IWRITE );
		if ( ft->fileId == -1 ) {
			MSN_DebugLog( "Unable to open avatar file '%s', error %d", szFileName, errno );
			delete ft;
			return;
		}
		ft->std.totalBytes = filelength( ft->fileId );

		p2p_sendAck( ft, info, hdrdata );
		ft->p2p_msgid -= 3;

		//---- send 200 OK Message 
		p2p_sendStatus( ft, info, 200 );
	}
	else if ( dwAppID == 2 && !strcmp( szEufGuid, "{5D3E02AB-6190-11D3-BBBB-00C04F795683}" )) {
		WCHAR* wszFileName = ( WCHAR* )&szContext[ 20 ];
		char szFileName[ MAX_PATH ];
		WideCharToMultiByte( CP_ACP, 0, wszFileName, -1, szFileName, MAX_PATH, 0, 0 );
		MSN_DebugLog( "File name: '%s'", szFileName );

		if ( msnRunningUnderNT )
			ft->wszFileName = _wcsdup( wszFileName );

		ft->std.hContact = info->mJoinedContacts[0];
		ft->std.currentFile = strdup( szFileName );
		ft->std.totalBytes =	ft->std.currentFileSize = *( long* )&szContext[ 8 ];
		ft->std.totalFiles = 1;
		ft->p2p_hdr = *hdrdata;

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
		pre.lParam = ( LPARAM )( char* )"";

		CCSDATA ccs;
		ccs.hContact = info->mJoinedContacts[0];
		ccs.szProtoService = PSR_FILE;
		ccs.wParam = 0;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
	}
	else {
		delete ft;
		MSN_DebugLog( "Invalid or unknown AppID/EUF-GUID combination: %ld/%s", dwAppID, szEufGuid );
		return;
	}

	p2p_registerSession( ft );
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
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', Branch='%s', SessionID=%s", szCallID, szBranch );
		return;
	}

	filetransfer* ft = p2p_getSessionByCallID( szCallID );
	if ( ft == NULL )
		return;

	if ( ft->p2p_callID ) free( ft->p2p_callID );  ft->p2p_callID = strdup( szCallID );
	if ( ft->p2p_branch ) free( ft->p2p_branch );  ft->p2p_branch = strdup( szBranch );
	ft->p2p_acksessid = 0x024B0000 + rand();

	const char	*szConnType = tFileInfo2[ "Conn-Type" ],
					*szUPnPNat = tFileInfo2[ "UPnPNat" ],
					*szNetID = tFileInfo2[ "NetID" ],
					*szICF = tFileInfo2[ "ICF" ];

	if ( szConnType == NULL || szUPnPNat == NULL || szICF == NULL || szNetID == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: ConnType='%s', UPnPNat='%s', ICF='%s', NetID='%s'", 
			szConnType, szUPnPNat, szICF, szNetID );
		return;
	}

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
	ft->p2p_msgid++;
	p2p_sendAck( ft, info, hdrdata );
	ft->p2p_msgid-=2;

	bool bUseDirect = false, bActAsServer = false;
	if ( atol( szNetID ) == 0 ) {
		if ( !strcmp( szConnType, "Direct-Connect" ) || !strcmp( szConnType, "Firewall" ))
			bUseDirect = true;
	}
	
	if ( MSN_GetByte( "NLSpecifyIncomingPorts", 0 )) {
		MSN_DebugLog( "My machine can accept incoming connections" );
		bUseDirect = bActAsServer = true;
	}

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "1 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );
	if ( bUseDirect )
		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	else
		tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );

	char szBody[ 512 ];
	int  cbBodyLen = 0;
	if ( bActAsServer )
		cbBodyLen = sttCreateListener( ft, ( pThreadFunc )p2p_filePassiveRecvThread, szBody, sizeof szBody );

	if ( !cbBodyLen )
		cbBodyLen = _snprintf( szBody, sizeof szBody, 
			"Bridge: TCPv1\r\n"
			"Listening: false\r\n"
			"Nonce: %s\r\n\r\n%c", sttVoidNonce, 0 );

	p2p_sendSlp( info, ft, tResult, 200, szBody, cbBodyLen );

	ft->p2p_msgid++;
}

static void sttInitDirectTransfer2( 
	P2P_Header*  hdrdata, 
	ThreadData*  info, 
	MimeHeaders& tFileInfo,
	MimeHeaders& tFileInfo2 )
{
	filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
	if ( ft == NULL )
		return;

	const char	*szInternalAddress = tFileInfo2[ "IPv4Internal-Addrs" ], 
					*szInternalPort = tFileInfo2[ "IPv4Internal-Port" ],
					*szExternalAddress = tFileInfo2[ "IPv4External-Addrs" ],
					*szExternalPort = tFileInfo2[ "IPv4External-Port" ],
					*szNonce = tFileInfo2[ "Nonce" ], 
					*szListening = tFileInfo2[ "Listening" ];
	
	if ( szNonce == NULL || szListening == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: Listening='%s', Nonce=%ld", szNonce, szListening );
		return;
	}

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
	p2p_sendAck( ft, info, hdrdata );

	if ( !strcmp( szListening, "true" ) && strcmp( szNonce, sttVoidNonce )) {
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_FILETRANS;
		newThread->mP2pSession = ft;
		strncpy( newThread->mCookie, szNonce, sizeof newThread->mCookie );
		_snprintf( newThread->mServer, sizeof newThread->mServer, "%s:%s", szInternalAddress, szInternalPort );
		newThread->startThread(( pThreadFunc )p2p_fileActiveRecvThread );
	}
	else p2p_sendStatus( ft, info, 603 );
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

	p2p_sendAck( ft, info, hdrdata );

	const char *szCallID = tFileInfo[ "Call-ID" ], *szBranch = tFileInfo[ "Via" ];
	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}

	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch );
LBL_Close:		
		p2p_sendBye( info, ft );
		return;
	}

	if ( !ft->std.sending ) {
      ft->p2p_branch = strdup( szBranch );
		ft->p2p_callID = strdup( szCallID );
		return;
	}

	const char* szOldContentType = tFileInfo[ "Content-Type" ];
	if ( szOldContentType == NULL ) 
		goto LBL_Close;

	bool bAllowIncoming = ( MSN_GetByte( "NLSpecifyIncomingPorts", 0 ) != 0 );

	MimeHeaders tResult(20);
	tResult.addLong( "CSeq", 0 );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );

	char* szBody = ( char* )alloca( 1024 );
	int   cbBody = 0;
	if ( !strcmp( szOldContentType, "application/x-msnmsgr-sessionreqbody" )) {
		if ( ft->fileId == -1 )
			if (( ft->fileId = open( ft->std.currentFile, O_RDONLY | _O_BINARY, _S_IREAD | _S_IWRITE )) == -1 ) {
				MSN_DebugLog( "Unable to open file '%s', error %d", ft->std.currentFile, errno );
				goto LBL_Close;
			}

		tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );
		cbBody = _snprintf( szBody, 1024, 
			"Bridges: TCPv1\r\nNetID: 0\r\nConn-Type: %s\r\nUPnPNat: false\r\nICF: false\r\n\r\n%c",
			( bAllowIncoming ) ? "Direct-Connect" : "Unknown-Connect", 0 );
	}
	else if ( !strcmp( szOldContentType, "application/x-msnmsgr-transrespbody" )) {
		const char	*szListening = tFileInfo2[ "Listening" ],
						*szNonce = tFileInfo2[ "Nonce" ],
						*szInternalAddress = tFileInfo2[ "IPv4Internal-Addrs" ],
						*szInternalPort = tFileInfo2[ "IPv4Internal-Port" ];
		if ( szListening == NULL || szNonce == NULL ) {
			MSN_DebugLog( "Invalid data packet, exiting..." );
			goto LBL_Close;
		}

		// another side reported that it will be a server.
		if ( !strcmp( szListening, "true" ) && strcmp( szNonce, sttVoidNonce )) {
			if ( szInternalAddress == NULL || szInternalPort == NULL ) {
				MSN_DebugLog( "Invalid data packet, exiting..." );
				goto LBL_Close;
			}

			ThreadData* newThread = new ThreadData;
			newThread->mType = SERVER_FILETRANS;
			newThread->mP2pSession = ft;
			strncpy( newThread->mCookie, szNonce, sizeof newThread->mCookie );
			_snprintf( newThread->mServer, sizeof newThread->mServer, "%s:%s", szInternalAddress, szInternalPort );
			newThread->startThread(( pThreadFunc )p2p_filePassiveSendThread );
			return;
		}

		// can we be a server?
		if ( bAllowIncoming )
			cbBody = sttCreateListener( ft, ( pThreadFunc )p2p_fileActiveSendThread, szBody, 1024 );

		// no, send a file via server
		if ( cbBody == 0 ) {
			p2p_sendViaServer( ft, info );
			return;
		}	

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else if ( !strcmp( szOldContentType, "application/x-msnmsgr-transreqbody" )) {
		// can we be a server?
		if ( bAllowIncoming )
			cbBody = sttCreateListener( ft, ( pThreadFunc )p2p_fileActiveSendThread, szBody, 1024 );

		// no, send a file via server
		if ( cbBody == 0 ) {
			p2p_sendViaServer( ft, info );
			return;
		}	

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else goto LBL_Close;

	p2p_sendSlp( info, ft, tResult, -2, szBody, cbBody );
}

static void sttCloseTransfer( P2P_Header* hdrdata, ThreadData* info, MimeHeaders& tFileInfo )
{
	filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
	if ( ft == NULL )
		return;

	p2p_sendAck( ft, info, hdrdata );
	p2p_unregisterSession( ft );
}

void __stdcall p2p_processMsg( ThreadData* info, char* msgbody )
{
	P2P_Header* hdrdata = ( P2P_Header* )msgbody; msgbody += sizeof( P2P_Header );
	sttLogHeader( hdrdata );

	//---- if we got a message
	int iMsgType = 0;
	if ( !memcmp( msgbody, "INVITE MSNMSGR:", 15 ))
		iMsgType = 1;
	else if ( !memcmp( msgbody, "MSNSLP/1.0 200 OK", 17 ))
		iMsgType = 2;
	else if ( !memcmp( msgbody, "BYE MSNMSGR:", 12 ))
		iMsgType = 3;

	if ( iMsgType ) {
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

		switch( iMsgType ) {
		case 1:
			if ( !strcmp( szContentType, "application/x-msnmsgr-sessionreqbody" ))
				sttInitFileTransfer( hdrdata, info, tFileInfo, tFileInfo2, msgbody );
			else if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-transreqbody" ))
				sttInitDirectTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
			else if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-transrespbody" ))
				sttInitDirectTransfer2( hdrdata, info, tFileInfo, tFileInfo2 );
			break;

		case 2:
			sttAcceptTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
			break;

		case 3:
			if (!strcmp( szContentType, "application/x-msnmsgr-sessionclosebody" ))
				sttCloseTransfer( hdrdata, info, tFileInfo );
			break;
		}
		return;
	}

	//---- receiving ack -----------
	if (( hdrdata->mFlags & 0x02 ) == 0x02 ) {
		filetransfer* ft = p2p_getSessionByMsgID( hdrdata->mAckSessionID );
		if ( ft == NULL )
			return;

		char szContactEmail[ MSN_MAX_EMAIL_LEN ];
		if ( MSN_GetStaticString( "e-mail", info->mJoinedContacts[0], szContactEmail, sizeof szContactEmail ))
			return;

		char* buf = ( char* )alloca( 2000 + 3*strlen( szContactEmail ));
		char* p = buf + sprintf( buf, sttP2Pheader, szContactEmail );
		P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
		memset( tHdr, 0, sizeof P2P_Header );

		switch( ft->p2p_ackID ) { 
		case 1000:
			//---- send Data Preparation Message
			tHdr->mSessionID = ft->p2p_sessionid;
			tHdr->mID = ft->p2p_msgid++;
			tHdr->mTotalSize = tHdr->mPacketLen = 4;
			tHdr->mAckSessionID = ft->p2p_acksessid;
			*( long* )p = 0; p += sizeof( long );
			*( long* )p = 0x01000000; p += sizeof( long );
			info->sendRawMessage( 'D', buf, int( p - buf ));
			break;

		case 1001:
			//---- send Data Messages
			{	for ( DWORD offset = 0; offset < ft->std.totalBytes; offset += 1202 ) {
					long cbPortion = 1202;
					if ( offset + cbPortion > ft->std.totalBytes )
						cbPortion = ft->std.totalBytes - offset;

					p = buf + sprintf( buf, sttP2Pheader, szContactEmail );

					tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
					memset( tHdr, 0, sizeof P2P_Header );
					tHdr->mSessionID = ft->p2p_sessionid;
					tHdr->mID = ft->p2p_msgid;
					tHdr->mTotalSize = ft->std.totalBytes;
					tHdr->mOffset = offset;
					tHdr->mPacketLen = cbPortion;
					tHdr->mFlags = 0x20;
					tHdr->mAckSessionID = ft->p2p_acksessid;

					::read( ft->fileId, p, cbPortion );
					p += cbPortion;
					*( long* )p = 0x01000000; p += sizeof( long );
					info->sendRawMessage( 'D', buf, int( p - buf ));
			}	}
			break;

		case 2000:
			break;

		case 3000: 
			p2p_sendBye( info, ft );
			p2p_unregisterSession( ft );
			break;

		case 99:
			p2p_sendAck( ft, info, hdrdata );

			sttSavePicture2disk( info, ft );

			HANDLE hContact = info->mJoinedContacts[0];

			if ( ft->mOwnsThread )
				info->sendPacket( "OUT", NULL );

			p2p_unregisterSession( ft );

			char tContext[ 256 ];
			if ( !MSN_GetStaticString( "PictContext", hContact, tContext, sizeof( tContext )))
				MSN_SetString( hContact, "PictSavedContext", tContext );
		}

		ft->p2p_ackID++;
		return;
	}

	filetransfer* ft = p2p_getSessionByID( hdrdata->mSessionID );
	if ( ft == NULL )
		return;

	if ( hdrdata->mFlags == 0 ) {
		//---- accept the data preparation message ------
		long* pLongs = ( long* )msgbody;
		if ( pLongs[0] == 0 && pLongs[1] == 0x01000000 && hdrdata->mPacketLen == 4 ) {
			p2p_sendAck( ft, info, hdrdata );
			return;
	}	}

	//---- receiving data -----------
	if (( hdrdata->mFlags & 0x20 ) == 0x20 ) {
		if ( hdrdata->mOffset + hdrdata->mPacketLen > hdrdata->mTotalSize )
			hdrdata->mPacketLen = DWORD( hdrdata->mTotalSize - hdrdata->mOffset );

		if ( ft->create() == -1 ) {
			p2p_sendBye( info, ft );
			p2p_unregisterSession( ft );
			return;
		}

		::_write( ft->fileId, msgbody, hdrdata->mPacketLen );

		if ( ft->p2p_appID == 2 ) {
			ft->std.totalProgress += hdrdata->mPacketLen;
			ft->std.currentFileProgress += hdrdata->mPacketLen;

			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
		}

		//---- send an ack: body was transferred correctly
		if ( hdrdata->mOffset + hdrdata->mPacketLen == hdrdata->mTotalSize ) {
			if ( ft->p2p_appID == 1 ) {
				p2p_sendBye( info, ft );
				ft->p2p_ackID = 99;
			}
			else {
				p2p_sendAck( ft, info, hdrdata );
				ft->complete();
}	}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_invite - invite another side to transfer an avatar

struct HFileContext
{
	DWORD len, dw1, dwSize, dw2, dw3;
	WCHAR wszFileName[ MAX_PATH ];
};

void __stdcall p2p_invite( HANDLE hContact, int iAppID, filetransfer* ft )
{
	const char* szAppID;
	switch( iAppID ) {
	case MSN_APPID_FILE:		szAppID = "{5D3E02AB-6190-11D3-BBBB-00C04F795683}";	break;
	case MSN_APPID_AVATAR:	szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	default:	
		return;
	}

	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", hContact, szEmail, sizeof szEmail );

	srand( (unsigned)time( NULL ) );
	long sessionID = rand();

	if ( ft == NULL ) {
		ft = new filetransfer();
		ft->std.hContact = hContact;
	}
	ft->p2p_appID = iAppID;
	ft->p2p_msgid = rand();
	ft->p2p_acksessid = rand();
	ft->p2p_sessionid = sessionID;
	ft->p2p_dest = strdup( szEmail );
	ft->p2p_branch = getNewUuid();
	ft->p2p_callID = getNewUuid();
	p2p_registerSession( ft );

	BYTE* pContext;
	int   cbContext;

	if ( iAppID == MSN_APPID_AVATAR ) {
		ft->inmemTransfer = true;
		ft->fileBuffer = NULL;

		char tBuffer[ 256 ];
		MSN_GetStaticString( "PictContext", hContact, tBuffer, sizeof( tBuffer ));
	
		char* p = strstr( tBuffer, "Size=\"" );
		if ( p != NULL )
			ft->std.totalBytes = ft->std.currentFileSize = atol( p+6 );

		pContext = ( BYTE* )tBuffer;
		cbContext = strlen( tBuffer )+1;
	}
	else {
		HFileContext ctx;
		memset( &ctx, 0, sizeof ctx );
		ctx.len = sizeof ctx;
		ctx.dwSize = ft->std.totalBytes;
		if ( ft->wszFileName != NULL )
			wcsncpy( ctx.wszFileName, ft->wszFileName, sizeof ctx.wszFileName );
		else {
			char* pszFiles = strrchr( ft->std.currentFile, '\\' );
			if ( pszFiles )
				pszFiles++;
			else
				pszFiles = ft->std.currentFile;

			MultiByteToWideChar( CP_ACP, 0, pszFiles, -1, ctx.wszFileName, sizeof ctx.wszFileName );
		}

		pContext = ( BYTE* )&ctx;
		cbContext = sizeof( ctx );
	}

	char* body = ( char* )alloca( 2000 );
	int tBytes = _snprintf( body, 2000,
		"EUF-GUID: %s\r\n"
		"SessionID: %ld\r\n"
		"AppID: %d\r\n"
		"Context: ", 
		szAppID, sessionID, iAppID );

	NETLIBBASE64 nlb = { body+tBytes, 2000-tBytes, pContext, cbContext };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	strcat( body, "\r\n\r\n" );
	int cbBody = strlen( body );
	body[ cbBody++ ] = 0;

	MimeHeaders tResult(20);
	tResult.addLong( "CSeq", 0 );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );
	tResult.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

	p2p_sendSlp( MSN_GetThreadByContact( hContact ), ft, tResult, -2, body, cbBody );
}
