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

static char sttGatewayHeader[] =
	"POST %s HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Content-Type: text/xml; charset=utf-8\r\n"
	"Content-Length: %d\r\n"
	"User-Agent: %s\r\n"
	"Host: %s\r\n"
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n\r\n";

//=======================================================================================

int ThreadData::send( char* data, int datalen )
{
	if ( this == NULL )
		return 0;

	NETLIBBUFFER nlb = { data, datalen, 0 };

	mWaitPeriod = mJoinedCount ? 60 : 30;

	if ( MyOptions.UseGateway && !( mType == SERVER_FILETRANS || mType == SERVER_P2P_DIRECT )) {
		mGatewayTimeout = 2;

		if ( !MyOptions.UseProxy ) {
			TQueueItem* tNewItem = ( TQueueItem* )mir_alloc( datalen + sizeof( void* ) + sizeof( int ) + 1 );
			tNewItem->datalen = datalen;
			memcpy( tNewItem->data, data, datalen );
			tNewItem->data[datalen] = 0;
			tNewItem->next = NULL;

			WaitForSingleObject( hQueueMutex, INFINITE );
			TQueueItem **p = &mFirstQueueItem;
			while ( *p != NULL ) p = &(*p)->next;
			*p = tNewItem;
			++numQueueItems;
			ReleaseMutex( hQueueMutex );

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

	return TRUE;
}

bool ThreadData::isTimeout( void )
{
	bool res = false;

	if ( --mWaitPeriod > 0 ) return false;

	if ( !mIsMainThread && ( mJoinedCount <= 1 || mChatID[0] == 0 )) {
		if ( mJoinedCount == 0 )
			res = true;
		else if ( p2p_getThreadSession( mJoinedContacts[0], mType ) != NULL )
			res = false;
		else if ( mType == SERVER_SWITCHBOARD ) {
			MessageWindowInputData msgWinInData = { 
				sizeof( MessageWindowInputData ), mJoinedContacts[0], MSG_WINDOW_UFLAG_MSG_BOTH 
			};
			MessageWindowData msgWinData = {0};
			msgWinData.cbSize = sizeof( MessageWindowData );

			res = MSN_CallService( MS_MSG_GETWINDOWDATA, ( WPARAM )&msgWinInData, ( LPARAM )&msgWinData ) != 0;
			res = res || msgWinData.hwndWindow == NULL;
			if ( res ) {	
				msgWinInData.hContact = ( HANDLE )MSN_CallService( MS_MC_GETMETACONTACT, ( WPARAM )mJoinedContacts[0], 0 );
				if ( msgWinInData.hContact != NULL ) {
					res = MSN_CallService( MS_MSG_GETWINDOWDATA, ( WPARAM )&msgWinInData, ( LPARAM )&msgWinData ) != 0;
					res = res || msgWinData.hwndWindow == NULL;
				}
			}
		}
		else
			res = true;
	}

	if ( res ) {
		bool sbsess = mType == SERVER_SWITCHBOARD;

		MSN_DebugLog( "Dropping the idle %s due to inactivity", sbsess ? "switchboard" : "p2p");
		if ( !sbsess ) return true;

		if ( MSN_GetByte( "EnableSessionPopup", 0 )) {
			HANDLE hContact = mJoinedCount ? mJoinedContacts[0] : mInitialContact;
			MSN_ShowPopup( hContact, TranslateT( "Chat session dropped due to inactivity" ), 0 );
		}

		sendPacket( "OUT", NULL );
		mWaitPeriod = 15;
	}
	else
		mWaitPeriod = 60;

	return false;
}


//=======================================================================================
// Receving data
//=======================================================================================
char* ThreadData::httpTransact(char* szCommand, size_t cmdsz, size_t& ressz)
{
	NETLIBSELECT tSelect = {0};
	tSelect.cbSize = sizeof( tSelect );
	tSelect.dwTimeout = 8000;
	tSelect.hReadConns[ 0 ] = s;

	size_t bufSize = 4096;
	char* szResult = (char*)mir_alloc(bufSize);
	char* szBody = NULL; 

	for (unsigned rc=0; rc<3; ++rc)
	{
		if (s == NULL)
		{
			NETLIBOPENCONNECTION tConn = { 0 };
			tConn.cbSize = sizeof( tConn );
			tConn.flags = NLOCF_V2;
			tConn.szHost = mGatewayIP;
			tConn.wPort = MSN_DEFAULT_GATEWAY_PORT;
			s = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
			tSelect.hReadConns[ 0 ] = s;
		}

		szBody = NULL;
		int lstRes = Netlib_Send(s, szCommand, cmdsz, 0);
		if (lstRes != SOCKET_ERROR)
		{
			size_t ackSize = 0;

			ressz = 0;
			for(;;)
			{
				// Wait for the next packet
				lstRes = MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&tSelect );
				if ( lstRes <= 0 ) { 
					lstRes = SOCKET_ERROR; 
					break; }

				lstRes = Netlib_Recv(s, szResult + ackSize, bufSize - ackSize, 0);
				if ( lstRes == 0 ) 
					MSN_DebugLog( "Connection closed gracefully" );

				if ( lstRes < 0 )
					MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
				
				// Connection closed or aborted, all data received
				if ( lstRes <= 0 )break;

				ackSize += lstRes;

				if ((bufSize-1) <= ackSize)
				{
					bufSize += 4096;
					szResult = (char*)mir_realloc(szResult, bufSize);
				}

				// Insert null terminator to use string functions
				szResult[ackSize] = 0;

				// HTTP header found?
				if (szBody == NULL)
				{
					// Find HTTP header end
					szBody = strstr(szResult, "\r\n\r\n");
					if (szBody != NULL)
					{
						szBody += 4;
						size_t hdrSize = szBody - szResult;

						unsigned status; 
						MimeHeaders tHeaders;

						char* tbuf = (char*)mir_alloc(hdrSize + 1);
						memcpy(tbuf, szResult, hdrSize);
						tbuf[hdrSize] = 0;

						char* p = httpParseHeader( tbuf, status );
						tHeaders.readFromBuffer( p );

						if (status == 100)
						{
							ackSize -= hdrSize;
							memmove(szResult, szResult + hdrSize, ackSize+1);
							szBody = NULL;
							mir_free(tbuf);
							continue;
						}

						const char* contLenHdr = tHeaders[ "Content-Length" ];
						ressz = hdrSize + (contLenHdr ? atol( contLenHdr ) : 0);
						if (bufSize <= ressz)
						{
							bufSize = ressz + 1;
							szResult = (char*)mir_realloc(szResult, bufSize);
						}
						mir_free(tbuf);
					}
				}

				// Content-Length bytes reached, all data received
				if (ackSize >= ressz) break;
			}
		}
		else
			MSN_DebugLog( "Send failed: %d", WSAGetLastError() );

		if (lstRes > 0) break;
		ressz = 0;
		Netlib_CloseHandle(s);
		s = NULL;

		if (Miranda_Terminated()) break; 
	}
	if (ressz == 0)
	{
		mir_free(szResult);
		szResult = NULL;
	}
	
	return szResult;
}


int ThreadData::recv_dg( char* data, long datalen )
{
	for(;;)
	{
		if ( mReadAheadBuffer != NULL ) {
			int datasent = mEhoughData - (mReadAheadBufferPtr - mReadAheadBuffer);
			int tBytesToCopy = ( datalen >= datasent ) ? datasent : datalen;

			if ( tBytesToCopy == 0 ) {
				mir_free( mReadAheadBuffer );
				mReadAheadBuffer = NULL;
				mReadAheadBufferPtr = NULL;
				if (sessionClosed) return 0;
			}
			else 
			{
				memcpy( data, mReadAheadBufferPtr, tBytesToCopy );
				mReadAheadBufferPtr += tBytesToCopy;
				return tBytesToCopy;
			}
		}

		char* tBuffer = NULL;
		size_t cbBytes = 0;
		for ( int i=0; i < mGatewayTimeout; i++ ) {
			if ( numQueueItems > 0 ) break;

			Sleep(1000);
			
			// Timeout switchboard session if inactive
			if ( isTimeout() ) return 0;
		}	

		unsigned np = 0, dlen = 0;
		
		WaitForSingleObject( hQueueMutex, INFINITE );
		TQueueItem* QI = mFirstQueueItem;
		while ( QI != NULL && np < 5) { ++np; dlen += QI->datalen;  QI = QI->next;}

		char szHttpPostUrl[300];
		getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), dlen == 0 );

		tBuffer = ( char* )alloca( dlen + 512 );
		cbBytes = mir_snprintf( tBuffer, dlen + 512, sttGatewayHeader,
			szHttpPostUrl, dlen, MSN_USER_AGENT, mGatewayIP);
		
		for ( unsigned j=0; j<np; ++j ) {
			QI = mFirstQueueItem;
			memcpy( tBuffer+cbBytes, QI->data, QI->datalen );
			cbBytes += QI->datalen;

			mFirstQueueItem = QI->next;
			mir_free( QI );
			--numQueueItems;
		}
		ReleaseMutex( hQueueMutex );

		if ( dlen == 0 ) {
			mGatewayTimeout += 2;
			if ( mGatewayTimeout > 8 ) 
				mGatewayTimeout = 8;
		}

		size_t ressz;
		char* tResult = httpTransact(tBuffer, cbBytes, ressz);

		if (tResult == NULL) return SOCKET_ERROR;

		unsigned status;
		MimeHeaders tHeaders;

		char* tBody = httpParseHeader( tResult, status );
		tBody = tHeaders.readFromBuffer( tBody );

		const char* xMsnHdr = tHeaders[ "X-MSN-Messenger" ]; 
		if ( status != 200 || xMsnHdr == NULL ) {
			mir_free(tResult);
			return SOCKET_ERROR;
		}

		sessionClosed = strstr( xMsnHdr, "Session=close" ) != NULL;
		processSessionData( xMsnHdr );

		if ( ressz > (size_t)(tBody - tResult) )
		{
			mGatewayTimeout = 1;
			mWaitPeriod = mJoinedCount ? 60 : 30;
		}

		mReadAheadBuffer = tResult;
		mReadAheadBufferPtr = tBody;
		mEhoughData = ressz;
	}
}

int ThreadData::recv( char* data, long datalen )
{
	if ( MyOptions.UseGateway && !MyOptions.UseProxy )
		if ( mType != SERVER_FILETRANS && mType != SERVER_P2P_DIRECT )
			return recv_dg( data, datalen );

	NETLIBBUFFER nlb = { data, datalen, 0 };

LBL_RecvAgain:
	if ( !mIsMainThread && !MyOptions.UseGateway && !MyOptions.UseProxy ) {
		mWaitPeriod = mJoinedCount ? 60 : 30;
		NETLIBSELECT nls = { 0 };
		nls.cbSize = sizeof( nls );
		nls.dwTimeout = 1000;
		nls.hReadConns[0] = s;
		while ( MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls ) == 0 )
			if ( isTimeout() ) return 0;
	}

	int ret = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
	if ( ret == 0 ) {
		MSN_DebugLog( "Connection closed gracefully" );
		return 0;
	}

	if ( ret < 0 ) {
		MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
		return ret;
	}

	if ( MyOptions.UseGateway)
	{
		if ( ret == 1 && *data == 0 ) 
		{
			int tOldTimeout = MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( s ), 2 );
			tOldTimeout += 2;
			if ( tOldTimeout > 8 )
				tOldTimeout = 8;

			MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( s ), tOldTimeout );
			goto LBL_RecvAgain;
		}
		else MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( s ), 1 );
	}

	return ret;
}
