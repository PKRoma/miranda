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

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_session - invite another side to the new MSN P2P session

void __stdcall p2p_session( HANDLE hContact )
{
	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", hContact, szEmail, sizeof szEmail );

	srand( (unsigned)time( NULL ) );
	long sessionID = rand();

	filetransfer* ft = new filetransfer( NULL );
	ft->inmemTransfer = true;
	ft->fileBuffer = NULL;
	ft->p2p_appID = 1;
	ft->p2p_msgid = rand();
	ft->p2p_acksessid = rand();
	ft->p2p_dest = strdup( szEmail );

	//---- session init

	char szMyEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", NULL, szMyEmail, sizeof( szMyEmail ));

	char tBuffer[ 256 ];
	MSN_GetStaticString( "PictContext", hContact, tBuffer, sizeof( tBuffer ));
	{
		char* p = strstr( tBuffer, "Size=\"" );
		if ( p != NULL )
			ft->std.totalBytes = ft->std.currentFileSize = atol( p+6 );
	}

	char* body = ( char* )alloca( 600 );
	int tBytes = _snprintf( body, 600,
		"EUF-GUID: {A4268EEC-FEC5-49E5-95C3-F126696BDBF6}\r\n"
		"SessionID: %ld\r\n"
		"AppID: 1\r\n"
		"Context: ", sessionID );

	NETLIBBASE64 nlb = { body+tBytes, 600-tBytes, ( PBYTE )tBuffer, strlen( tBuffer )+1 };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	strcat( body, "\r\n\r\n" );
	int cbBody = strlen( body );

	int tHeaderBufLen = 350 + strlen( szEmail )*3;
	char* hdr1 = ( char* )alloca( tHeaderBufLen );
	int cbHeader = _snprintf( hdr1, tHeaderBufLen,
		"INVITE MSNMSGR:%s MSNSLP/1.0\r\n"
		"To: <msnmsgr:%s>\r\n"
		"From: <msnmsgr:%s>\r\n"
		"Via: MSNSLP/1.0/TLP ;branch={7DB907E2-15E2-11D8-9066-0003FF431510}\r\n"
		"CSeq: 0\r\n"
		"Call-ID: {7DB907E3-15E2-11D8-9066-0003FF431510}\r\n"
		"Max-Forwards: 0\r\n"
		"Content-Type: application/x-msnmsgr-sessionreqbody\r\n"
		"Content-Length: %d\r\n\r\n",
		szEmail, szEmail, szMyEmail, cbBody );

	char* msg = ( char* )alloca( 300 + cbBody + cbHeader );
	char* p = msg + sprintf( msg, sttP2Pheader, szEmail );

	P2P_Header* hdr = ( P2P_Header* )p;	p += sizeof( P2P_Header );
	memset( hdr, 0, sizeof P2P_Header );
	hdr->mID = ft->p2p_msgid++;
	hdr->mTotalSize = hdr->mPacketLen = cbBody + cbHeader + 1;
	hdr->mAckSessionID = ft->p2p_acksessid;

	memcpy( p, hdr1, cbHeader ); p += cbHeader;
	memcpy( p, body, cbBody ); p += cbBody;
	*p++ = 0;
	*( long* )p = 0; p += sizeof( long );

	ThreadData* T = MSN_GetThreadByContact( hContact );
	if ( T == NULL ) {
		MsgQueue_Add( hContact, msg, int( p - msg ), ft );
		MSN_SendPacket( msnNSSocket, "XFR", "SB" );
	}		 
	else {
		T->ft = ft;
		ft->mOwner = T;
		MSN_SendRawMessage( T->s, 'D', msg, int( p - msg ));
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// sttSavePicture2disk - final handler for avatars downloading 

static void sttSavePicture2disk( ThreadData* info )
{
	filetransfer* ft = info->ft;
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

void __stdcall p2p_sendAck( filetransfer* ft, P2P_Header* hdrdata )
{
	if ( ft == NULL || ft->mOwner == NULL ) {
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
	MSN_SendRawMessage( ft->mOwner->s, 'D', buf, int( p - buf ));
}

static int sttDigits( long iNumber )
{
	char tBuffer[ 20 ];
	ltoa( iNumber, tBuffer, 10 );
	return strlen( tBuffer );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendSlp - send MSN P2P SLP packet

void __stdcall p2p_sendSlp( filetransfer* ft, MimeHeaders& pHeaders, int iKind, const char* szContent, size_t szContLen )
{
	if ( ft == NULL || ft->mOwner == NULL ) {
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
		MSN_WS_Send( ft->mOwner->s, ( char* )&tLen, sizeof( DWORD ));
		MSN_WS_Send( ft->mOwner->s, buf, tLen );
	}
	else MSN_SendRawMessage( ft->mOwner->s, 'D', buf, int( p - buf ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendStatus - send MSN P2P status and its description

void __stdcall p2p_sendStatus( filetransfer* ft, long lStatus )
{
	MimeHeaders tHeaders( 20 );
	tHeaders.addLong( "CSeq", 1 );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

	char szContents[ 50 ];
	p2p_sendSlp( ft, tHeaders, lStatus, szContents,
		_snprintf( szContents, sizeof szContents, "SessionID: %ld\r\n\r\n%c", ft->p2p_sessionid, 0 ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_fileRecvThread - receives a file using a direct MSN P2P connection 

static void sttSendPacket( HANDLE s, P2P_Header& hdr )
{
	DWORD len = sizeof P2P_Header;
	MSN_WS_Send( s, ( char* )&len, sizeof DWORD );
	MSN_WS_Send( s, ( char* )&hdr, sizeof P2P_Header );
}

void __cdecl p2p_fileRecvThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_fileRecvThread() started: connecting to '%s'", info->mServer );

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

	filetransfer* ft = info->ft;

	if (( info->s = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn )) == NULL ) {
		{	TWinErrorCode err;
			MSN_DebugLog( "Connection Failed (%d): %s", err.mErrorCode, err.getText() );
		}

		return;
	}

	ft->mIsDirect = true;

	static char pkt1[8] = { 4, 0, 0, 0, 'f', 'o', 'o', 0  };
	MSN_WS_Send( info->s, pkt1, sizeof pkt1 );

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
		return;
	}

	cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( info->s, cbPacketLen )) == NULL ) 
		return;

	while( true ) {
		if (( p = buf.surelyRead( info->s, 4 )) == NULL )
			return;

		cbPacketLen = *( long* )p;
		if (( p = buf.surelyRead( info->s, cbPacketLen )) == NULL )
			return;

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

				MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0);
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
				MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
			break;
	}	}

	MSN_DebugLog( "File transfer succeeded" );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processMsg - processes all MSN P2P incoming messages

void __stdcall p2p_processMsg( ThreadData* info, char* msgbody )
{
	P2P_Header* hdrdata = ( P2P_Header* )msgbody; msgbody += sizeof( P2P_Header );
	sttLogHeader( hdrdata );

	filetransfer* ft = info->ft;

	if ( hdrdata->mFlags == 0 ) {
		char szContactEmail[ MSN_MAX_EMAIL_LEN ];
		if ( MSN_GetStaticString( "e-mail", info->mJoinedContacts[0], szContactEmail, sizeof szContactEmail ))
			return;

		//---- if we got a message
		int iMsgType = 0;
		if ( !memcmp( msgbody, "INVITE MSNMSGR:", 15 ))
			iMsgType = 1;
		else if ( !memcmp( msgbody, "BYE MSNMSGR:", 12 ))
			iMsgType = 2;
		else if ( !memcmp( msgbody, "MSNSLP/1.0 200 OK", 17 ))
			iMsgType = 3;

		if ( iMsgType ) {
			MimeHeaders tFileInfo, tFileInfo2;
			msgbody = tFileInfo.readFromBuffer( msgbody );
			msgbody = tFileInfo2.readFromBuffer( msgbody );

			const char* szContentType = tFileInfo[ "Content-Type" ];
			if ( szContentType == NULL ) {
				MSN_DebugLog( "Invalid or missing Content-Type field, exiting" );
				return;
			}

			if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-sessionreqbody" )) {
				//---- start of an avatar reading session. send a BaseIdentifier Message 
				char szMyEmail[ MSN_MAX_EMAIL_LEN ];
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

						MSN_DebugLog( "Decoded context field='%s'", szContext );
				}	}

				if ( szContext == NULL && memcmp( msgbody, "Context: ", 9 ) == 0 ) {
					msgbody += 9;
					int cbLen = strlen( msgbody );
					if ( cbLen > 252 )
						cbLen = 252;
					szContext = ( char* )alloca( cbLen+1 );
					
					NETLIBBASE64 nlb = { msgbody, cbLen, ( PBYTE )szContext, cbLen };
					MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));

					MSN_DebugLog( "Decoded context field='%s'", szContext );
				}

				if ( szSessionID == NULL || dwAppID == -1 || szEufGuid == NULL ) {
					MSN_DebugLog( "Ignoring invalid invitation: SessionID='%s', AppID=%ld, Branch='%s'", szSessionID, dwAppID, szEufGuid );
					return;
				}

				srand( time( NULL ));

				filetransfer* ft = new filetransfer( info );
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
					info->ft = ft;
				}
				else if ( dwAppID == 2 && !strcmp( szEufGuid, "{5D3E02AB-6190-11D3-BBBB-00C04F795683}" )) {
					WCHAR* wszFileName = ( WCHAR* )&szContext[ 20 ];
					char szFileName[ MAX_PATH ];
					WideCharToMultiByte( CP_ACP, 0, wszFileName, -1, szFileName, MAX_PATH, 0, 0 );

					if ( msnRunningUnderNT )
						ft->wszFileName = _wcsdup( wszFileName );

					info->ft = ft;
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
					return;
				}
				else {
					info->ft = NULL; delete ft;
					MSN_DebugLog( "Invalid or unknown AppID/EUF-GUID combination: %ld/%s", dwAppID, szEufGuid );
					return;
				}

				p2p_sendAck( ft, hdrdata );
				ft->p2p_msgid -= 3;

				//---- send 200 OK Message 
				p2p_sendStatus( ft, 200 );
			}
			else if ( iMsgType == 2 && !strcmp( szContentType, "application/x-msnmsgr-sessionclosebody" )) {
				if ( ft != NULL ) {
					p2p_sendAck( ft, hdrdata );

					info->ft->close();
					info->ft = NULL; delete ft;
				}
			}
			else if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-transreqbody" )) {
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

				_chdir( ft->std.workingDir );

				char filefull[ MAX_PATH ];
				_snprintf( filefull, sizeof( filefull ), "%s\\%s", ft->std.workingDir, ft->std.currentFile );
				ft->std.currentFile = strdup( filefull );

				if ( ft->inmemTransfer ) {
					if ( ft->fileBuffer == NULL ) {
						if (( ft->fileBuffer = ( char* )LocalAlloc( LPTR, DWORD( ft->std.totalBytes ))) == NULL ) {
							MSN_DebugLog( "Cannot create file '%s' during a file transfer", filefull );
							return;
				}	}	}
				else {
					if ( msnRunningUnderNT )
						ft->fileId = _wopen( ft->wszFileName, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
					else
						ft->fileId = _open( ft->std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
					if ( ft->fileId == -1 ) {
						MSN_DebugLog( "Cannot create file '%s' during a file transfer", filefull );
						return;
				}	}

				MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
				ft->p2p_msgid++;
				p2p_sendAck( ft, hdrdata );
				ft->p2p_msgid-=2;

				bool bUseDirect = false, bActAsServer = false;
				if ( atol( szNetID ) == 0 ) {
					if ( !strcmp( szConnType, "Direct-Connect" ) || !strcmp( szConnType, "Firewall" ))
						bUseDirect = true;
				}
/*				else if ( MSN_GetByte( "NLSpecifyIncomingPorts", 0 ))
					bUseDirect = bActAsServer = true; 
*/
				MimeHeaders tResult(20);
				tResult.addString( "CSeq", "1 " );
				tResult.addString( "Call-ID", ft->p2p_callID );
				tResult.addLong( "Max-Forwards", 0 );
				if ( bUseDirect )
					tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
				else
					tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );

				char szBody[ 512 ];
				{
					int  cbBodyLen;
					if ( bActAsServer ) {
						UUID myid;
						UuidCreate( &myid );

						cbBodyLen = _snprintf( szBody, sizeof szBody,
							"Bridge: TCPv1\r\n"
							"Listening: true\r\n"
							"Nonce: {%08X-%04X-%04X-%08X%08X}\r\n\r\n%c", 
							myid.Data1, (WORD)myid.Data2, (WORD)myid.Data3, *( DWORD* )myid.Data4, *( DWORD* )&myid.Data4[4],
							0 );
					}
					else 
						cbBodyLen = _snprintf( szBody, sizeof szBody,
							"Bridge: TCPv1\r\n"
							"Listening: false\r\n"
							"Nonce: {00000000-0000-0000-0000-000000000000}\r\n\r\n%c", 0 );

					p2p_sendSlp( ft, tResult, 200, szBody, cbBodyLen );
				}

				ft->p2p_msgid++;
			}
			else if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-transrespbody" )) {
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
				p2p_sendAck( ft, hdrdata );

				if ( !strcmp( szListening, "true" ) && strcmp( szNonce, "{00000000-0000-0000-0000-000000000000}" )) {
					ThreadData* newThread = new ThreadData;
					newThread->mType = SERVER_FILETRANS;
					newThread->ft = ft; info->ft = NULL;
					strncpy( newThread->mCookie, szNonce, sizeof newThread->mCookie );
					_snprintf( newThread->mServer, sizeof newThread->mServer, "%s:%s", szInternalAddress, szInternalPort );
					newThread->startThread(( pThreadFunc )p2p_fileRecvThread );
				}
				else p2p_sendStatus( ft, 603 );
			}
			else if ( iMsgType == 3 && ft != NULL ) {
				p2p_sendAck( ft, hdrdata );

				const char *szCallID = tFileInfo[ "Call-ID" ], *szBranch = tFileInfo[ "Via" ];
				if ( szBranch != NULL ) {
					szBranch = strstr( szBranch, "branch=" );
					if ( szBranch != NULL )
						szBranch += 7;
				}

				if ( szCallID == NULL || szBranch == NULL ) {
					MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch );
					return;
				}

				ft->p2p_branch = strdup( szBranch );
				ft->p2p_callID = strdup( szCallID );
			}

			return;
		}

		//---- accept the data preparation message ------
		long* pLongs = ( long* )msgbody;
		if ( pLongs[0] == 0 && pLongs[1] == 0x01000000 && hdrdata->mPacketLen == 4 ) {
			p2p_sendAck( ft, hdrdata );
			return;
	}	}

	if ( ft == NULL )
		return;

	//---- receiving data -----------
	if (( hdrdata->mFlags & 0x20 ) == 0x20 ) {
		if ( hdrdata->mOffset + hdrdata->mPacketLen > hdrdata->mTotalSize )
			hdrdata->mPacketLen = DWORD( hdrdata->mTotalSize - hdrdata->mOffset );

		if ( ft->inmemTransfer ) {
			if ( ft->fileBuffer == NULL )
				if (( ft->fileBuffer = ( char* )LocalAlloc( LPTR, DWORD( hdrdata->mTotalSize ))) == NULL )
					return;

			memcpy( ft->fileBuffer + hdrdata->mOffset, msgbody, hdrdata->mPacketLen );
			ft->std.totalBytes = ( long )hdrdata->mTotalSize;
		}				
		else ::_write( ft->fileId, msgbody, hdrdata->mPacketLen );

		if ( ft->p2p_appID == 2 ) {
			ft->std.totalProgress += hdrdata->mPacketLen;
			ft->std.currentFileProgress += hdrdata->mPacketLen;

			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
		}

		//---- send an ack: body was transferred correctly
		if ( hdrdata->mOffset + hdrdata->mPacketLen == hdrdata->mTotalSize ) {
			if ( ft->p2p_appID == 1 ) {
				MimeHeaders tResult(50);
				tResult.addLong( "CSeq", 0 );
				tResult.addString( "Call-ID", ft->p2p_callID );
				tResult.addLong( "Max-Forwards", 0 );
				tResult.addString( "Content-Type", "application/x-msnmsgr-sessionclosebody" );
				p2p_sendSlp( ft, tResult, -1, "\r\n\0", 3 );

				ft->p2p_ackID = 99;
			}
			else {
				MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0);
				p2p_sendAck( ft, hdrdata );
		}	}

		return;
	}

	//---- receiving ack -----------
	if (( hdrdata->mFlags & 0x02 ) == 0x02 && ft != NULL ) {
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
			MSN_SendRawMessage( info->s, 'D', buf, int( p - buf ));
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
					MSN_SendRawMessage( info->s, 'D', buf, int( p - buf ));
			}	}
			break;

		case 2000:
			break;

		case 99:
			p2p_sendAck( ft, hdrdata );

			sttSavePicture2disk( info );

			HANDLE hContact = info->mJoinedContacts[0];

			if ( ft->mOwnsThread )
				MSN_SendPacket( info->s, "OUT", NULL );

			info->ft->close();
			delete info->ft; info->ft = NULL;

			char tContext[ 256 ];
			if ( !MSN_GetStaticString( "PictContext", hContact, tContext, sizeof( tContext )))
				MSN_SetString( hContact, "PictSavedContext", tContext );
		}

		ft->p2p_ackID++;
}	}
