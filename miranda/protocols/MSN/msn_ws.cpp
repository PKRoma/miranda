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

static char sttGatewayHeader[] =
	"POST %s HTTP/1.1\r\n"
	"User-Agent: %s\r\n"
	"Host: %s\r\n"
	"Connection: keep-alive\r\n"
	"Content-Length: %d\r\n\r\n%s";

extern int msnPingTimeout, msnPingTimeoutCurrent;

//=======================================================================================

int ThreadData::send( char* data, int datalen )
{
	if ( this == NULL )
		return 0;

	NETLIBBUFFER nlb = { data, datalen, 0 };

	if ( mType == SERVER_NOTIFICATION )
		msnPingTimeout = 50;

	if ( MyOptions.UseGateway && !( mType == SERVER_FILETRANS && mP2pSession != NULL )) {
		if ( datalen != 5 && memcmp( data, "PNG\r\n", 5 ) != 0 )
			mGatewayTimeout = 2;

		if ( !MyOptions.UseProxy ) {
			TQueueItem* tNewItem = ( TQueueItem* )malloc( datalen + sizeof( void* ) + sizeof( int ) + 1 );
			tNewItem->datalen = datalen;
			memcpy( tNewItem->data, data, datalen );
			tNewItem->data[datalen] = 0;

			TQueueItem* p = mFirstQueueItem;
			if ( p != NULL ) {
				while ( p->next != NULL )
					p = p->next;

				p ->next = tNewItem;
			}
			else mFirstQueueItem = tNewItem;

			tNewItem->next = NULL;
			return TRUE;
		}

		MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( s ), mGatewayTimeout );
	}

	int rlen = MSN_CallService( MS_NETLIB_SEND, ( WPARAM )s, ( LPARAM )&nlb );
	if ( rlen == SOCKET_ERROR ) {
		// should really also check if sendlen is the same as datalen
		MSN_DebugLog( "Send failed: %d", WSAGetLastError() );
		return FALSE;
	}

	mWaitPeriod = 60;
	return TRUE;
}

//=======================================================================================
// Receving data
//=======================================================================================

int ThreadData::recv_dg( char* data, long datalen )
{
	if ( mReadAheadBuffer != NULL ) {
		int tBytesToCopy = ( datalen >= mEhoughData ) ? mEhoughData : datalen;
		memcpy( data, mReadAheadBuffer, tBytesToCopy );
		mEhoughData -= tBytesToCopy;
		if ( mEhoughData == 0 ) {
			free( mReadAheadBuffer );
			mReadAheadBuffer = NULL;
		}
		else memmove( mReadAheadBuffer, mReadAheadBuffer + tBytesToCopy, mEhoughData );

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

		for ( int i=0; i < mGatewayTimeout; i++ ) {
			if ( bCanPeekMsg ) {
				TQueueItem* QI = mFirstQueueItem;
				if ( QI != NULL )
				{
					char szHttpPostUrl[300];
					getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), QI->datalen == 0 );

					char* tBuffer = ( char* )alloca( QI->datalen+400 );
					int cbBytes = mir_snprintf( tBuffer, QI->datalen+400, sttGatewayHeader,
						szHttpPostUrl, MSN_USER_AGENT, mGatewayIP, QI->datalen, "" );
					memcpy( tBuffer+cbBytes, QI->data, QI->datalen );
					cbBytes += QI->datalen;
					tBuffer[ cbBytes ] = 0;

					NETLIBBUFFER nlb = { tBuffer, cbBytes, 0 };
					ret = MSN_CallService( MS_NETLIB_SEND, ( WPARAM )s, ( LPARAM )&nlb );
					if ( ret == SOCKET_ERROR ) {
						MSN_DebugLog( "Send failed: %d", WSAGetLastError() );
						return 0;
					}

					mFirstQueueItem = QI->next;
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
		mGatewayTimeout *= 2;
		if ( mGatewayTimeout > 30 )
			mGatewayTimeout = 30;

		char szHttpPostUrl[300];
		getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), true );

		char szCommand[ 400 ];
		int cbBytes = mir_snprintf( szCommand, sizeof( szCommand ),
			sttGatewayHeader, szHttpPostUrl, MSN_USER_AGENT, mGatewayIP, 0, "" );

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
	if ( p == NULL ) {
		MSN_DebugLog( "ACHTUNG! it's not a valid header: '%s'", data );
		goto LBL_RecvAgain;
	}

	int status = 0;
	sscanf( data, "HTTP/1.1 %d", &status );
	if ( status == 100 )
		goto LBL_RecvAgain;
	
	bool  tIsSessionClosed = false;
	int   tContentLength = 0, hdrLen;
	{
		MimeHeaders tHeaders;
		const char* rest = tHeaders.readFromBuffer( p+2 );
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

				processSessionData( H.value );
			}

			if ( stricmp( H.name, "Content-Length" ) == 0 )
				tContentLength = atol( H.value );
		}

		hdrLen = int( rest - data );
	}

	bCanPeekMsg = true;

	if ( tContentLength == 0 )
		goto LBL_RecvAgain;

	ret -= hdrLen;
	if ( ret <= 0 ) {
		nlb.buf = data;
		nlb.len = datalen;
		ret = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
		if ( ret <= 0 )
			return ret;
	}
	else memmove( data, data+hdrLen, ret );

	if ( tContentLength > ret ) {
		tContentLength -= ret;

		mReadAheadBuffer = ( char* )calloc( tContentLength+1, 1 );
		mReadAheadBuffer[ tContentLength ] = 0;
		mEhoughData = tContentLength;
		nlb.buf = mReadAheadBuffer;

		while ( tContentLength > 0 ) {
			nlb.len = tContentLength;
			int ret2 = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
			if ( ret2 <= 0 )
			{	free( mReadAheadBuffer );
				mReadAheadBuffer = NULL;
				return ret2;
			}

			tContentLength -= ret2;
			nlb.buf += ret2;
	}	}

	return ret;
}

int ThreadData::recv( char* data, long datalen )
{
	if ( MyOptions.UseGateway && !MyOptions.UseProxy )
		if ( mType != SERVER_FILETRANS || mP2pSession == 0 )
			return recv_dg( data, datalen );

	NETLIBBUFFER nlb = { data, datalen, 0 };

LBL_RecvAgain:
	if ( !mIsMainThread && !MyOptions.UseGateway && !MyOptions.UseProxy ) {
		mWaitPeriod = 60;
		while ( --mWaitPeriod >= 0 ) {
			NETLIBSELECT nls = { 0 };
			nls.cbSize = sizeof( nls );
			nls.dwTimeout = 1000;
			nls.hReadConns[0] = s;
			if ( MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls ) != 0 )
				break;
		}

		if ( mWaitPeriod < 0 && mJoinedCount <= 1 ) {
			MSN_DebugLog( "Dropping the idle switchboard due to the 60 sec timeout" );
			return 0;
	}	}

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
