/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2009 Boris Krasnovskiy.
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

static const char sttGatewayHeader[] =
	"POST %s HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Content-Type: text/xml; charset=utf-8\r\n"
	"Content-Length: %d\r\n"
	"User-Agent: %s\r\n"
	"Host: %s\r\n"
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n\r\n";

//=======================================================================================

int ThreadData::send( const char data[], int datalen )
{
	NETLIBBUFFER nlb = { (char*)data, datalen, 0 };

	mWaitPeriod = 60;

	if ( proto->MyOptions.UseGateway && !( mType == SERVER_FILETRANS || mType == SERVER_P2P_DIRECT )) {
		mGatewayTimeout = 2;

		if ( !proto->MyOptions.UseProxy ) {
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
		proto->MSN_DebugLog( "Send failed: %d", WSAGetLastError() );
		return FALSE;
	}

	return TRUE;
}

bool ThreadData::isTimeout( void )
{
	bool res = false;

	if ( --mWaitPeriod > 0 ) return false;

	if ( !mIsMainThread && ( mJoinedCount <= 1 || mChatID[0] == 0 )) {
		if ( mJoinedCount == 0 || termPending )
			res = true;
		else if ( proto->p2p_getThreadSession( mJoinedContacts[0], mType ) != NULL )
			res = false;
		else if ( mType == SERVER_SWITCHBOARD ) 
		{
			MessageWindowInputData msgWinInData = 
				{ sizeof( MessageWindowInputData ), mJoinedContacts[0], MSG_WINDOW_UFLAG_MSG_BOTH };
			MessageWindowData msgWinData = {0};
			msgWinData.cbSize = sizeof( MessageWindowData );

			res = MSN_CallService( MS_MSG_GETWINDOWDATA, ( WPARAM )&msgWinInData, ( LPARAM )&msgWinData ) != 0;
			res |= (msgWinData.hwndWindow == NULL);
			if ( res ) 
			{	
				msgWinInData.hContact = ( HANDLE )MSN_CallService( MS_MC_GETMETACONTACT, ( WPARAM )mJoinedContacts[0], 0 );
				if ( msgWinInData.hContact != NULL ) {
					res = MSN_CallService( MS_MSG_GETWINDOWDATA, ( WPARAM )&msgWinInData, ( LPARAM )&msgWinData ) != 0;
					res |= (msgWinData.hwndWindow == NULL);
				}
			}
			if ( res ) 
			{	
				WORD status = proto->getWord(mJoinedContacts[0], "Status", ID_STATUS_OFFLINE);
				if ((status == ID_STATUS_OFFLINE || status == ID_STATUS_INVISIBLE || proto->m_iStatus == ID_STATUS_INVISIBLE)) 
					res = false;
			}
		}
		else
			res = true;
	}

	if ( res ) 
	{
		bool sbsess = mType == SERVER_SWITCHBOARD;

		proto->MSN_DebugLog( "Dropping the idle %s due to inactivity", sbsess ? "switchboard" : "p2p");
		if (!sbsess || termPending) return true;

		if ( proto->getByte( "EnableSessionPopup", 0 )) {
			HANDLE hContact = mJoinedCount ? mJoinedContacts[0] : mInitialContact;
			proto->MSN_ShowPopup( hContact, TranslateT( "Chat session dropped due to inactivity" ), 0 );
		}

		sendPacket( "OUT", NULL );
		termPending = true;
		mWaitPeriod = 10;
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
	tSelect.dwTimeout = 6000;
	tSelect.hReadConns[ 0 ] = s;

	size_t bufSize = 4096;
	char* szResult = (char*)mir_alloc(bufSize);
	char* szBody;

	for (unsigned rc=4; --rc; )
	{
		ressz = 0;
		szBody = NULL;

		if (s == NULL)
		{
			NETLIBOPENCONNECTION tConn = { 0 };
			tConn.cbSize = sizeof( tConn );
			tConn.flags = NLOCF_V2;
			tConn.szHost = mGatewayIP;
			tConn.wPort = MSN_DEFAULT_GATEWAY_PORT;
			tConn.timeout = 5;
			proto->MSN_DebugLog("Connecting to gateway: %s:%d", tConn.szHost, tConn.wPort);
			s = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )proto->hNetlibUser, ( LPARAM )&tConn );
			if (s == NULL) 
			{
				Sleep(3000);
				continue;
			}
			tSelect.hReadConns[ 0 ] = s;
		}
		int lstRes = Netlib_Send(s, szCommand, cmdsz, 0);
		if (lstRes != SOCKET_ERROR)
		{
			size_t ackSize = 0;
			for(;;)
			{
				// Wait for the next packet
				lstRes = MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&tSelect );
				if ( lstRes < 0 ) { 
					proto->MSN_DebugLog( "Connection failed while waiting." );
					break; 
				}
				else if ( lstRes == 0 ) { 
					proto->MSN_DebugLog( "Receive Timeout. Bytes received: %u %u", ackSize, ressz );
					lstRes = SOCKET_ERROR; 
					break; 
				}

				lstRes = Netlib_Recv(s, szResult + ackSize, bufSize - ackSize, 0);
				if ( lstRes == 0 ) 
					proto->MSN_DebugLog( "Connection closed gracefully" );

				if ( lstRes < 0 )
					proto->MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
				
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
					unsigned status; 
					MimeHeaders tHeaders;
					char *tbuf = NULL, *hdrs = NULL;
					size_t hdrSize = 0;

					for (;;) 
					{
						// Find HTTP header end
						szBody = strstr(szResult, "\r\n\r\n");
						if (szBody == NULL) break;

						szBody += 4;
						hdrSize = szBody - szResult;

						// Make a copy of response headers for processing
						tbuf = (char*)mir_alloc(hdrSize + 1);
						memcpy(tbuf, szResult, hdrSize);
						tbuf[hdrSize] = 0;

						hdrs = httpParseHeader( tbuf, status );
						if (status != 100) break;

						proto->MSN_DebugLog( "Response 100 detected: %d", ackSize );
						// Remove 100 status response from response buffer
						ackSize -= hdrSize;
						memmove(szResult, szResult + hdrSize, ackSize+1);
						mir_free(tbuf);
					}
					if (szBody == NULL) continue;

					tHeaders.readFromBuffer( hdrs );

					// Calculate the size of the response packet
					const char* contLenHdr = tHeaders[ "Content-Length" ];
					ressz = hdrSize + (contLenHdr ? atol( contLenHdr ) : 0);
					// Adjust the buffer to hold complete response
					if (bufSize <= ressz)
					{
						bufSize = ressz + 1;
						szResult = (char*)mir_realloc(szResult, bufSize);
					}
					mir_free(tbuf);
				}

				// Content-Length bytes reached, all data received
				if (ackSize >= ressz) break;
			}
		}
		else
			proto->MSN_DebugLog( "Send failed: %d", WSAGetLastError() );

		if (lstRes > 0) break;

		proto->MSN_DebugLog( "Connection closed due to HTTP transaction failure" );
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
	time_t ts = time(NULL);
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
		while ((time(NULL) - ts)  < mGatewayTimeout) 
		{
			if ( numQueueItems > 0 ) break;
			Sleep(1000);
			// Timeout switchboard session if inactive
			if ( isTimeout() ) return 0;
		}	
		ts = time(NULL);

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

		size_t ressz;
		char* tResult = httpTransact(tBuffer, cbBytes, ressz);

		if (tResult == NULL) return SOCKET_ERROR;

		if ( dlen == 0 ) 
			mGatewayTimeout = min(mGatewayTimeout + 2, 20);

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
			mWaitPeriod = 60;
		}

		mReadAheadBuffer = tResult;
		mReadAheadBufferPtr = tBody;
		mEhoughData = ressz;
	}
}

int ThreadData::recv( char* data, long datalen )
{
	if ( proto->MyOptions.UseGateway && !proto->MyOptions.UseProxy )
		if ( mType != SERVER_FILETRANS && mType != SERVER_P2P_DIRECT )
			return recv_dg( data, datalen );

	NETLIBBUFFER nlb = { data, datalen, 0 };

LBL_RecvAgain:
	if ( !mIsMainThread && !proto->MyOptions.UseGateway && !proto->MyOptions.UseProxy ) {
		mWaitPeriod = 60;
		NETLIBSELECT nls = { 0 };
		nls.cbSize = sizeof( nls );
		nls.dwTimeout = 1000;
		nls.hReadConns[0] = s;

        for (;;)
        {
            int ret = MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls );
	        if ( ret < 0 ) {
		        proto->MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
		        return ret;
	        }
	        else if ( ret == 0 && isTimeout()) 
                return 0;
            else
                break;
        }
	}

	int ret = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )s, ( LPARAM )&nlb );
	if ( ret == 0 ) {
		proto->MSN_DebugLog( "Connection closed gracefully" );
		return 0;
	}

	if ( ret < 0 ) {
		proto->MSN_DebugLog( "Connection abortively closed, error %d", WSAGetLastError() );
		return ret;
	}

	if ( proto->MyOptions.UseGateway)
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
