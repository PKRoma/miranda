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

static char sttGatewayHeader[] =
	"POST %s HTTP/1.1\r\n"
	"User-Agent: %s\r\n"
	"Host: %s\r\n"
	"Connection: keep-alive\r\n"
	"Content-Length: %d\r\n\r\n%s";

//=======================================================================================

BOOL __stdcall	MSN_WS_Init()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2,2),&wsd)!=0) return FALSE;

	return TRUE;
}

void __stdcall	MSN_WS_CleanUp()
{
	WSACleanup();
}

int __stdcall MSN_WS_Send( HANDLE s, char* data, int datalen )
{
	NETLIBBUFFER nlb = { data, datalen, 0 };

	if ( MyOptions.UseGateway && !MyOptions.UseProxy ) {
		ThreadData* T = MSN_GetThreadByConnection( s );
		if ( T == NULL ) {
			MSN_DebugLog( "Send failed: no owning thread" );
			return FALSE;
		}

		TQueueItem* tNewItem = ( TQueueItem* )malloc( datalen + sizeof( void* ) + 1 );
		strcpy( tNewItem->data, data );

		TQueueItem* p = T->mFirstQueueItem;
		if ( p != NULL ) {
			while ( p->next != NULL )
				p = p->next;

			p ->next = tNewItem;
		}

		tNewItem->next = NULL;
		T->mFirstQueueItem = tNewItem;
		return TRUE;
	}

	int rlen = MSN_CallService( MS_NETLIB_SEND, ( WPARAM )s, ( LPARAM )&nlb );
	if ( rlen == SOCKET_ERROR ) {
		// should really also check if sendlen is the same as datalen
		MSN_DebugLog( "Send failed: %d", WSAGetLastError() );
		return FALSE;
	}

	return TRUE;
}

//=======================================================================================
// Receving data
//=======================================================================================

static int MSN_WS_DG_Recv( HANDLE s, char* data, long datalen )
{
	ThreadData* T = MSN_GetThreadByConnection( s );
	if ( T == NULL )
		return 0;

	if ( T->mReadAheadBuffer != NULL ) {
		int tBytesToCopy = ( datalen >= T->mEhoughData ) ? T->mEhoughData : datalen;
		memcpy( data, T->mReadAheadBuffer, tBytesToCopy );
		T->mEhoughData -= tBytesToCopy;
		if ( T->mEhoughData == 0 ) {
			free( T->mReadAheadBuffer );
			T->mReadAheadBuffer = NULL;
		}
		else memmove( T->mReadAheadBuffer, T->mReadAheadBuffer + tBytesToCopy, T->mEhoughData );

		return tBytesToCopy;
	}

	bool bCanPeekMsg = true;

LBL_RecvAgain:
	int ret;
	{	
		NETLIBSELECT tSelect = {0};
		tSelect.cbSize = sizeof( tSelect );
		tSelect.dwTimeout = 1000;
		tSelect.hReadConns[ 0 ] = ( HANDLE )s;

		for ( int i=0; i < T->mGatewayTimeout; i++ ) {
			if ( bCanPeekMsg ) {
				TQueueItem* QI = T->mFirstQueueItem;
				if ( QI != NULL )
				{
					int tBufLen = strlen( QI->data );

					char szHttpPostUrl[300];
					T->getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), tBufLen == 0 );

					char* tBuffer = ( char* )alloca( tBufLen+400 );
					int cbBytes = _snprintf( tBuffer, tBufLen+400, sttGatewayHeader,
						szHttpPostUrl, MSN_USER_AGENT, T->mGatewayIP, tBufLen, QI->data );

					NETLIBBUFFER nlb = { tBuffer, cbBytes, 0 };
					ret = MSN_CallService( MS_NETLIB_SEND, ( WPARAM )s, ( LPARAM )&nlb );
					if ( ret == SOCKET_ERROR ) {
						MSN_DebugLog( "Send failed: %d", WSAGetLastError() );
						return 0;
					}

					T->mFirstQueueItem = QI->next;
					free( QI );

					ret = 1;
					break;
			}	}

			ret = MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&tSelect );
			if ( ret != 0 )
				break;
	}	}

	bCanPeekMsg = false;

	if ( ret == 0 ) {
		T->mGatewayTimeout *= 2;
		if ( T->mGatewayTimeout > 30 )
			T->mGatewayTimeout = 30;

		char szHttpPostUrl[300];
		T->getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), true );

		char szCommand[ 400 ];
		int cbBytes = _snprintf( szCommand, sizeof( szCommand ),
			sttGatewayHeader, szHttpPostUrl, MSN_USER_AGENT, T->mGatewayIP, 0, "" );

		NETLIBBUFFER nlb = { szCommand, cbBytes, 0 };
		MSN_CallService( MS_NETLIB_SEND, ( WPARAM )s, ( LPARAM )&nlb );
		goto LBL_RecvAgain;
	}

	NETLIBBUFFER nlb = { data, datalen, 0 };
	ret = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
	if ( ret == 0 ) {
		MSN_DebugLog( "Connection closed gracefully");
		return 0;
	}

	if ( ret < 0 ) {
		MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
		return ret;
	}

	char* p = strstr( data, "\r\n" );
	if ( p != NULL )
	{	int status = 0;
		sscanf( data, "HTTP/1.1 %d", &status );
		if ( status == 100 )
			goto LBL_RecvAgain;
	}

	bool  tIsSessionClosed = false;
	int   tContentLength = 0;
	char* rest;
	{
		MimeHeaders tHeaders;
		rest = tHeaders.readFromBuffer( p+2 );
		if ( *rest == '\r' )
			rest += 2;

		for ( int i=0; i < tHeaders.mCount; i++ )
		{
			MimeHeader& H = tHeaders.mVals[i];

			if ( stricmp( H.name, "X-MSN-Messenger" ) == 0 ) {
				if ( strstr( H.value, "Session=close" ) != 0 ) {
					tIsSessionClosed = true;
					break;
				}

				T->processSessionData( H.value );
			}

			if ( stricmp( H.name, "Content-Length" ) == 0 )
				tContentLength = atol( H.value );
	}	}

	bCanPeekMsg = true;

	if ( tContentLength == 0 )
		goto LBL_RecvAgain;

	int hdrLen = int( rest - data );
	ret -= hdrLen;
	if ( ret <= 0 ) {
		nlb.buf = data;
		nlb.len = datalen;
		ret = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
		if ( ret <= 0 )
			return ret;
	}
	else strdel( data, hdrLen );

	if ( tContentLength > ret ) {
		tContentLength -= ret;

		T->mReadAheadBuffer = ( char* )calloc( tContentLength+1, 1 );
		T->mReadAheadBuffer[ tContentLength ] = 0;
		T->mEhoughData = tContentLength;
		nlb.buf = T->mReadAheadBuffer;

		while ( tContentLength > 0 ) {
			nlb.len = tContentLength;
			int ret2 = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
			if ( ret2 <= 0 )
			{	free( T->mReadAheadBuffer );
				T->mReadAheadBuffer = NULL;
				return ret2;
			}

			tContentLength -= ret2;
			nlb.buf += ret2;
	}	}

	return ret;
}

int __stdcall MSN_WS_Recv( HANDLE s, char* data, long datalen )
{
	if ( MyOptions.UseGateway && !MyOptions.UseProxy )
		return MSN_WS_DG_Recv( s, data, datalen );

	NETLIBBUFFER nlb = { data, datalen, 0 };

LBL_RecvAgain:
	int ret = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
	if ( ret == 0 ) {
		MSN_DebugLog( "Connection closed gracefully" );
		return 0;
	}

	if ( ret < 0 ) {
		MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
		return ret;
	}

	if ( MyOptions.UseGateway && ret == 1 && *data == 0 ) {
		int tOldTimeout = MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( s ), 100 );
		tOldTimeout *= 2;
		if ( tOldTimeout > 30 )
			tOldTimeout = 30;

		MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( s ), tOldTimeout );
		goto LBL_RecvAgain;
	}

	return ret;
}
