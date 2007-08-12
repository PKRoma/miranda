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

int MSN_HandleCommands(ThreadData *info,char *cmdString);
int MSN_HandleErrors(ThreadData *info,char *cmdString);
int MSN_HandleMSNFTP( ThreadData *info, char *cmdString );

HANDLE hKeepAliveThreadEvt = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
//	Keep-alive thread for the main connection

int msnPingTimeout = 45;

void __cdecl msn_keepAliveThread( void* )
{
	bool keepFlag = true;

	hKeepAliveThreadEvt = CreateEvent( NULL, FALSE, FALSE, NULL );

	msnPingTimeout = 45;

	while ( keepFlag )
	{
		switch ( WaitForSingleObject( hKeepAliveThreadEvt, msnPingTimeout * 1000 ))
		{
			case WAIT_TIMEOUT:
				keepFlag = msnNsThread != NULL;
				if ( MyOptions.UseGateway )
					msnPingTimeout = 45;
				else {
					msnPingTimeout = 20;
					keepFlag = keepFlag && msnNsThread->send( "PNG\r\n", 5 );
				}
				p2p_clearDormantSessions();
				break;

			case WAIT_OBJECT_0:
				keepFlag = msnPingTimeout > 0;
				break;

			default:
				keepFlag = false;
				break;
	}	}

	CloseHandle( hKeepAliveThreadEvt ); hKeepAliveThreadEvt = NULL;
	MSN_DebugLog( "Closing keep-alive thread" );
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN server thread - read and process commands from a server

void __cdecl MSNServerThread( ThreadData* info )
{
	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.flags = NLOCF_V2;
	tConn.timeout = 5;

 	char* tPortDelim = strrchr( info->mServer, ':' );
	if ( tPortDelim != NULL )
		*tPortDelim = '\0';

	if (MyOptions.UseGateway) 
	{
		if (*info->mGatewayIP == 0 && MSN_GetStaticString("LoginServer", NULL, info->mGatewayIP, sizeof(info->mGatewayIP)))
			strcpy(info->mGatewayIP, MSN_DEFAULT_GATEWAY);
		if (*info->mServer == 0)
			strcpy(info->mServer, MSN_DEFAULT_LOGIN_SERVER); 
	}
	else
	{
		if (*info->mServer == 0 && MSN_GetStaticString("LoginServer", NULL, info->mServer, sizeof(info->mServer)))
			strcpy(info->mServer, MSN_DEFAULT_LOGIN_SERVER);
	}

	if ( MyOptions.UseGateway && !MyOptions.UseProxy ) 
	{
		tConn.szHost = info->mGatewayIP;
		tConn.wPort = MSN_DEFAULT_GATEWAY_PORT;
		info->hQueueMutex = CreateMutex(NULL, FALSE, NULL);
	}
	else 
	{
		tConn.szHost = info->mServer;
		tConn.wPort = MSN_DEFAULT_PORT;

		if (tPortDelim != NULL) 
		{
			int tPortNumber = atoi(tPortDelim+1);
			if (tPortNumber)
				tConn.wPort = ( WORD )tPortNumber;
		}	
	}

	MSN_DebugLog( "Thread started: server='%s', type=%d", tConn.szHost, info->mType );

	info->s = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
	if ( info->s == NULL ) {
		MSN_DebugLog( "Connection Failed (%d)", WSAGetLastError() );

		switch ( info->mType ) {
			case SERVER_NOTIFICATION:
			case SERVER_DISPATCH:
				MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NOSERVER );
				MSN_GoOffline();
				msnNsThread = NULL;
				if ( hKeepAliveThreadEvt ) {
					msnPingTimeout *= -1;
					SetEvent( hKeepAliveThreadEvt );
				}
				break;
		}

		return;
	}

	if ( MyOptions.UseGateway )
		MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( info->s ), 2 );

	MSN_DebugLog( "Connected with handle=%08X", info->s );

	if ( info->mType == SERVER_DISPATCH || info->mType == SERVER_NOTIFICATION ) {
		info->sendPacket( "VER", "MSNP12 MSNP11 MSNP10 CVR0" );
	}
	else if ( info->mType == SERVER_SWITCHBOARD ) {
		info->sendPacket( info->mCaller ? "USR" : "ANS", "%s %s", MyOptions.szEmail, info->mCookie );
	}
	else if ( info->mType == SERVER_FILETRANS && info->mCaller == 0 ) {
		info->send( "VER MSNFTP\r\n", 12 );
	}

	if ( info->mIsMainThread ) {
		MSN_EnableMenuItems( true );
		msnNsThread = info;
	}

	MSN_DebugLog( "Entering main recv loop" );
	info->mBytesInData = 0;
	for ( ;; ) {
		int recvResult = info->recv( info->mData + info->mBytesInData, sizeof( info->mData ) - info->mBytesInData );
		if ( recvResult == SOCKET_ERROR ) {
			MSN_DebugLog( "Connection %08p [%08X] was abortively closed", info->s, GetCurrentThreadId());
			break;
		}

		if ( !recvResult ) {
			MSN_DebugLog( "Connection %08p [%08X] was gracefully closed", info->s, GetCurrentThreadId());
			break;
		}

		info->mBytesInData += recvResult;

		if ( info->mCaller == 1 && info->mType == SERVER_FILETRANS ) {
			if ( MSN_HandleMSNFTP( info, info->mData ))
				break;
		}
		else {
			for( ;; ) {
				char* peol = strchr(info->mData,'\r');
				if ( peol == NULL )
					break;

				if ( info->mBytesInData < peol-info->mData+2 )
					break;  //wait for full line end

				char msg[ sizeof(info->mData) ];
				memcpy( msg, info->mData, peol-info->mData ); msg[ peol-info->mData ] = 0;

				if ( *++peol != '\n' )
					MSN_DebugLog( "Dodgy line ending to command: ignoring" );
				else
					peol++;

				info->mBytesInData -= peol - info->mData;
				memmove( info->mData, peol, info->mBytesInData );
				MSN_DebugLog( "RECV:%s", msg );

				if ( !isalnum( msg[0] ) || !isalnum(msg[1]) || !isalnum(msg[2]) || (msg[3] && msg[3]!=' ')) {
					MSN_DebugLog( "Invalid command name" );
					continue;
				}

				if ( info->mType != SERVER_FILETRANS ) {
					if ( info->mType == SERVER_NOTIFICATION )
						SetEvent( hKeepAliveThreadEvt );

					int handlerResult;
					if ( isdigit(msg[0]) && isdigit(msg[1]) && isdigit(msg[2]))   //all error messages
						handlerResult = MSN_HandleErrors( info, msg );
					else
						handlerResult = MSN_HandleCommands( info, msg );

					if ( handlerResult )
					{
						info->sendPacket( "OUT", NULL );
						info->termPending = true;
					}
				}
				else 
					if ( MSN_HandleMSNFTP( info, msg ))
						goto LBL_Exit;
		}	}

		if ( info->mBytesInData == sizeof( info->mData )) {
			MSN_DebugLog( "sizeof(data) is too small: the longest line won't fit" );
			break;
	}	}

LBL_Exit:
	if ( info->mIsMainThread ) {
		MSN_GoOffline();
		msnNsThread = NULL;

		if ( hKeepAliveThreadEvt ) {
			msnPingTimeout *= -1;
			SetEvent( hKeepAliveThreadEvt );
		}
	}

	MSN_DebugLog( "Thread [%d] ending now", GetCurrentThreadId() );
}

/////////////////////////////////////////////////////////////////////////////////////////
//  Added by George B. Hazan (ghazan@postman.ru)
//  The following code is required to abortively stop all started threads upon exit

static int CompareThreads( const ThreadData* p1, const ThreadData* p2 )
{
	return int( p1 - p2 );
}

static LIST<ThreadData> sttThreads( 10, CompareThreads );
static CRITICAL_SECTION	sttLock;

void  MSN_InitThreads()
{
	InitializeCriticalSection( &sttLock );
}

void  MSN_CloseConnections()
{
	EnterCriticalSection( &sttLock );

	NETLIBSELECT nls = {0};
	nls.cbSize = sizeof(nls);

	unsigned data;
	NETLIBBUFFER nlb = { (char*)&data, 1, MSG_PEEK };

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];

		switch (T->mType) 
		{
		case SERVER_DISPATCH :
		case SERVER_NOTIFICATION :
		case SERVER_SWITCHBOARD :
			if (T->s != NULL)
			{
				nls.hReadConns[0] = T->s;
				int res = MSN_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&nls );
				if (res > 0)
					res = MSN_CallService( MS_NETLIB_RECV, ( WPARAM )T->s, ( LPARAM )&nlb ) <= 0;
				if ( res == 0)
				{
					T->sendPacket( "OUT", NULL );
					T->termPending = true;
				}
			}
			break;

		case SERVER_P2P_DIRECT :
			{
				SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, LPARAM( T->s ), 0 );
				if ( s != INVALID_SOCKET )
					shutdown( s, 2 );
			}
			break;
	}	}

	LeaveCriticalSection( &sttLock );
}

void  MSN_CloseThreads()
{
	for (unsigned j=6; --j; )
	{	
		EnterCriticalSection( &sttLock );

		bool opcon = false;
		for ( int i=0; i < sttThreads.getCount(); i++ )
			opcon |= (sttThreads[ i ]->s != NULL);

		LeaveCriticalSection( &sttLock );
		
		if (!opcon) break;
		
		Sleep(250);
	}

	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) 
	{
		const ThreadData* T = sttThreads[ i ];
		
		if ( T->s != NULL )
		{
			SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, LPARAM( T->s ), 0 );
			if ( s != INVALID_SOCKET ) {

				shutdown( s, 2 );
			}
		}
	}

	LeaveCriticalSection( &sttLock );
}

void Threads_Uninit( void )
{
	DeleteCriticalSection( &sttLock );
	sttThreads.destroy();
}

ThreadData*  MSN_GetThreadByContact( HANDLE hContact, TInfoType type )
{
	ThreadData* result = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mJoinedCount == 0 || T->mJoinedContacts == NULL || T->s == NULL || T->mType != type )
			continue;

		if ( T->mJoinedContacts[0] == hContact && T->mInitialContact == NULL )
			result = T;
	}

	LeaveCriticalSection( &sttLock );
	return result;
}

ThreadData*  MSN_GetThreadByTimer( UINT timerId )
{
	ThreadData* result = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mType == SERVER_SWITCHBOARD && T->mTimerId == timerId ) {
			result = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return result;
}

ThreadData*  MSN_GetP2PThreadByContact( HANDLE hContact )
{
	ThreadData *p2pT = NULL, *sbT = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; p2pT == NULL && i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mJoinedCount == 0 || T->mJoinedContacts == NULL )
			continue;

		if ( T->mJoinedContacts[0] == hContact && T->mInitialContact == NULL ) {
			switch ( T->mType ) {
			case SERVER_SWITCHBOARD:
				sbT = T;
				break;

			case SERVER_P2P_DIRECT:
				p2pT = T;
				break;
	}	}	}

	LeaveCriticalSection( &sttLock );

	return ( p2pT ? p2pT : sbT );
}


void  MSN_StartP2PTransferByContact( HANDLE hContact )
{
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mJoinedCount == 0 || T->mJoinedContacts == NULL )
			continue;

		if ( T->mJoinedContacts[0] == hContact && T->mType == SERVER_FILETRANS
			  && T->hWaitEvent != INVALID_HANDLE_VALUE )
			ReleaseSemaphore( T->hWaitEvent, 1, NULL );
	}

	LeaveCriticalSection( &sttLock );
}


ThreadData*  MSN_GetOtherContactThread( ThreadData* thread )
{
	ThreadData* result = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mJoinedCount == 0 || T->mJoinedContacts == NULL || T->s == NULL )
			continue;

		if ( T != thread && T->mJoinedContacts[0] == thread->mJoinedContacts[0] ) {
			result = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return result;
}

ThreadData*  MSN_GetUnconnectedThread( HANDLE hContact )
{
	ThreadData* result = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mInitialContact == hContact && T->mType == SERVER_SWITCHBOARD ) {
			result = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return result;
}


ThreadData* MSN_StartSB(HANDLE hContact, bool& isOffline)
{
	isOffline = false;
	ThreadData* thread = MSN_GetThreadByContact(hContact);
	if (thread == NULL)
	{
		WORD wStatus = MSN_GetWord(hContact, "Status", ID_STATUS_OFFLINE);
		if (wStatus != ID_STATUS_OFFLINE && msnStatusMode != ID_STATUS_INVISIBLE)
		{
			if (MSN_GetUnconnectedThread(hContact) == NULL && MsgQueue_CheckContact(hContact, 5) == NULL)
				msnNsThread->sendPacket( "XFR", "SB" );
		}
		else
			isOffline = true;
	}
	return thread;
}



int  MSN_GetActiveThreads( ThreadData** parResult )
{
	int tCount = 0;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mType == SERVER_SWITCHBOARD && T->mJoinedCount != 0 && T->mJoinedContacts != NULL )
			parResult[ tCount++ ] = T;
	}

	LeaveCriticalSection( &sttLock );
	return tCount;
}

ThreadData*  MSN_GetThreadByConnection( HANDLE s )
{
	ThreadData* tResult = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->s == s ) {
			tResult = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return tResult;
}

ThreadData*  MSN_GetThreadByPort( WORD wPort )
{
	ThreadData* result = NULL;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < sttThreads.getCount(); i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T->mIncomingPort == wPort ) {
			result = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class ThreadData members

ThreadData::ThreadData()
{
	memset( this, 0, sizeof( ThreadData ));
	mGatewayTimeout = 2;
	mWaitPeriod = 30;
	hWaitEvent = CreateSemaphore( NULL, 0, 5, NULL );
}

ThreadData::~ThreadData()
{
	if ( s != NULL ) {
		MSN_DebugLog( "Closing connection handle %08X", s );
		Netlib_CloseHandle( s );
	}

	if ( mIncomingBoundPort != NULL ) {
		Netlib_CloseHandle( mIncomingBoundPort );
	}

	if ( mMsnFtp != NULL ) {
		delete mMsnFtp;
		mMsnFtp = NULL;
	}

	if ( hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( hWaitEvent );

	if ( mTimerId != 0 ) 
		KillTimer( NULL, mTimerId );

	p2p_clearDormantSessions();

	if (mType == SERVER_SWITCHBOARD)
	{
		for (int i=0; i<mJoinedCount; ++i)
		{
			const HANDLE hContact = mJoinedContacts[i];
			int temp_status = MSN_GetWord(hContact, "Status", ID_STATUS_OFFLINE);
			if (temp_status == ID_STATUS_INVISIBLE && MSN_GetThreadByContact(hContact) == NULL)
				MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE);
		}
	}

	mir_free( mJoinedContacts );

	if ( hQueueMutex ) WaitForSingleObject( hQueueMutex, INFINITE );
	while (mFirstQueueItem != NULL) {
		TQueueItem* QI = mFirstQueueItem;
		mFirstQueueItem = mFirstQueueItem->next;
		mir_free(QI);
		--numQueueItems;
	}
	if ( hQueueMutex )  {
		ReleaseMutex( hQueueMutex );
		CloseHandle( hQueueMutex );
	}

	if (mInitialContact != NULL && mType == SERVER_SWITCHBOARD && 
		MSN_GetThreadByContact(mInitialContact) == NULL &&
		MSN_GetUnconnectedThread(mInitialContact) == NULL)
	{
		MsgQueue_Clear(mInitialContact, true);
	}

}

void ThreadData::applyGatewayData( HANDLE hConn, bool isPoll )
{
	char szHttpPostUrl[300];
	getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), isPoll );

	MSN_DebugLog( "applying '%s' to %08X [%08X]", szHttpPostUrl, this, GetCurrentThreadId() );

	NETLIBHTTPPROXYINFO nlhpi = {0};
	nlhpi.cbSize = sizeof(nlhpi);
	nlhpi.flags = NLHPIF_HTTP11;
	nlhpi.szHttpGetUrl = NULL;
	nlhpi.szHttpPostUrl = szHttpPostUrl;
	nlhpi.firstPostSequence = 1;
	MSN_CallService( MS_NETLIB_SETHTTPPROXYINFO, (WPARAM)hConn, (LPARAM)&nlhpi);
}

void ThreadData::getGatewayUrl( char* dest, int destlen, bool isPoll )
{
	static const char openFmtStr[] = "http://%s/gateway/gateway.dll?Action=open&Server=%s&IP=%s";
	static const char pollFmtStr[] = "http://%s/gateway/gateway.dll?Action=poll&SessionID=%s";
	static const char cmdFmtStr[]  = "http://%s/gateway/gateway.dll?SessionID=%s";

	if ( mSessionID[0] == 0 ) {
		const char* svr = mType == SERVER_NOTIFICATION || mType == SERVER_DISPATCH ? "NS" : "SB";
		mir_snprintf( dest, destlen, openFmtStr, mGatewayIP, svr, mServer );
	}
	else
		mir_snprintf( dest, destlen, isPoll ? pollFmtStr : cmdFmtStr, mGatewayIP, mSessionID );

	if ( !MyOptions.UseProxy ) {
		char *slash = strchr(dest+7, '/');
		int len = strlen(dest) - (slash - dest) + 1;
		memmove(dest, slash, len);
	}
}

void ThreadData::processSessionData( const char* str )
{
	char tSessionID[40], tGateIP[ 40 ];

	char* tDelim = ( char* )strchr( str, ';' );
	if ( tDelim == NULL )
		return;

	*tDelim = 0; tDelim += 2;

	if ( !sscanf( str, "SessionID=%s", tSessionID ))
		return;

	char* tDelim2 = strchr( tDelim, ';' );
	if ( tDelim2 != NULL )
		*tDelim2 = '\0';

	if ( !sscanf( tDelim, "GW-IP=%s", tGateIP ))
		return;

//	MSN_DebugLog( "msn_httpGatewayUnwrapRecv printed '%s','%s' to %08X (%08X)", tSessionID, tGateIP, s, this );
	if (strcmp(mGatewayIP, tGateIP) != 0 && MyOptions.UseGateway && !MyOptions.UseProxy)
	{
		MSN_DebugLog("IP Changed %s %s", mGatewayIP, tGateIP);
		Netlib_CloseHandle(s);
		s = NULL;
	}
	strcpy( mGatewayIP, tGateIP );
	strcpy( mSessionID, tSessionID );
}

/////////////////////////////////////////////////////////////////////////////////////////
// thread start code

static void sttRegisterThread( ThreadData* s )
{
	if ( s == NULL ) return;

	EnterCriticalSection( &sttLock );
	sttThreads.insert( s );
	LeaveCriticalSection( &sttLock );
}

static void sttUnregisterThread( ThreadData* s )
{
	EnterCriticalSection( &sttLock );
	sttThreads.remove( s );
	LeaveCriticalSection( &sttLock );
}

/////////////////////////////////////////////////////////////////////////////////////////

static void __cdecl MSN_ThreadStub( ThreadData* info )
{
	sttRegisterThread( info );
	MSN_DebugLog( "Starting thread %08X (%08X)", GetCurrentThreadId(), info->mFunc );

	__try
	{
		info->mFunc( info );
	}
	__finally
	{
		MSN_DebugLog( "Leaving thread %08X (%08X)", GetCurrentThreadId(), info->mFunc );
		sttUnregisterThread( info );
		delete info;
}	}

void ThreadData::startThread( pThreadFunc parFunc )
{
	mFunc = parFunc;
	mir_forkthread(( pThreadFunc )MSN_ThreadStub, this );
}

/////////////////////////////////////////////////////////////////////////////////////////
// HReadBuffer members

HReadBuffer::HReadBuffer( ThreadData* T, int iStart )
{
	owner = T;
	buffer = ( BYTE* )T->mData;
	totalDataSize = T->mBytesInData;
	startOffset = iStart;
}

HReadBuffer::~HReadBuffer()
{
	owner->mBytesInData = totalDataSize - startOffset;
	if ( owner->mBytesInData != 0 )
		memmove( owner->mData, owner->mData + startOffset, owner->mBytesInData );
}

BYTE* HReadBuffer::surelyRead( int parBytes )
{
	if ( startOffset + parBytes > totalDataSize )
	{
		int tNewLen = totalDataSize - startOffset;
		if ( tNewLen > 0 )
			memmove( buffer, buffer + startOffset, tNewLen );
		else
			tNewLen = 0;

		startOffset = 0;
		totalDataSize = tNewLen;
	}

	int bufferSize = sizeof( owner->mData );
	if ( parBytes > bufferSize - startOffset ) {
		MSN_DebugLog( "HReadBuffer::surelyRead: not enough memory, %d %d %d", parBytes, bufferSize, startOffset );
		return NULL;
	}

	while( totalDataSize - startOffset < parBytes )
	{
		int recvResult = owner->recv(( char* )buffer + totalDataSize, bufferSize - totalDataSize );

		if ( recvResult <= 0 )
			return NULL;

		totalDataSize += recvResult;
	}

	BYTE* result = buffer + startOffset; startOffset += parBytes;
	return result;
}
