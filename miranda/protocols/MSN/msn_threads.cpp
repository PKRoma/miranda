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

#include <direct.h>

int MSN_HandleCommands(ThreadData *info,char *cmdString);
int MSN_HandleErrors(ThreadData *info,char *cmdString);
int MSN_HandleMSNFTP( ThreadData *info, char *cmdString );

extern LONG (WINAPI *MyInterlockedIncrement)(PLONG pVal);

HANDLE hKeepAliveThreadEvt = NULL;

bool DoingNudge = false;

/////////////////////////////////////////////////////////////////////////////////////////
//	Keep-alive thread for the main connection

int msnPingTimeout, msnPingTimeoutCurrent;

void __cdecl msn_keepAliveThread( void* )
{
	msnPingTimeoutCurrent = msnPingTimeout = 45;

	while( TRUE )
	{
		while ( msnPingTimeout-- > 0 ) {
			if ( ::WaitForSingleObject( hKeepAliveThreadEvt, 1000 ) != WAIT_TIMEOUT ) {
				::CloseHandle( hKeepAliveThreadEvt ); hKeepAliveThreadEvt = NULL;
				MSN_DebugLog( "Closing keep-alive thread" );
				return;
		}	}

		msnPingTimeoutCurrent = msnPingTimeout = 45;

		/*
		 * if proxy is not used, every connection uses select() to send PNG
		 */

		if ( msnLoggedIn && !MyOptions.UseGateway )
			if ( MSN_GetByte( "KeepAlive", 0 ))
				msnNsThread->send( "PNG\r\n", 5 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN redirector detection thread - refreshes the information about the redirector

static bool sttRedirectorWasChecked = false;

void __cdecl msn_RedirectorThread( ThreadData* info )
{
	extern int MSN_CheckRedirector();
	MSN_CheckRedirector();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN server thread - read and process commands from a server

void __cdecl MSNServerThread( ThreadData* info )
{
	MSN_DebugLog( "Thread started: server='%s', type=%d", info->mServer, info->mType );

	if ( !sttRedirectorWasChecked ) {
		sttRedirectorWasChecked = true;
		MSN_StartThread(( pThreadFunc )msn_RedirectorThread, NULL );
	}

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.flags = NLOCF_V2;

 	char* tPortDelim = strrchr( info->mServer, ':' );
	if ( tPortDelim != NULL )
		*tPortDelim = '\0';

	if ( MyOptions.UseGateway && !MyOptions.UseProxy ) {
		tConn.szHost = MSN_DEFAULT_GATEWAY;
		tConn.wPort = 80;
	}
	else {
		tConn.szHost = info->mServer;
		tConn.wPort = MSN_DEFAULT_PORT;

		if ( tPortDelim != NULL ) {
			int tPortNumber;
			if ( sscanf( tPortDelim+1, "%d", &tPortNumber ) == 1 )
				tConn.wPort = ( WORD )tPortNumber;
	}	}

	info->s = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
	if ( info->s == NULL ) {
		MSN_DebugLog( "Connection Failed (%d)", WSAGetLastError() );

		switch ( info->mType ) {
		case SERVER_NOTIFICATION:
		case SERVER_DISPATCH:
			MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NOSERVER );
			MSN_GoOffline();
			break;
		}

		return;
	}

	if ( MyOptions.UseGateway )
		MSN_CallService( MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM( info->s ), 2 );

	MSN_DebugLog( "Connected with handle=%08X", info->s );

	if ( info->mType == SERVER_DISPATCH || info->mType == SERVER_NOTIFICATION ) {
		if ( MyOptions.UseMSNP11 )
			info->sendPacket( "VER", "MSNP11 MSNP10 CVR0" );
		else
			info->sendPacket( "VER", "MSNP10 MSNP9 CVR0" );
	}
	else if ( info->mType == SERVER_SWITCHBOARD ) {
		char tEmail[ MSN_MAX_EMAIL_LEN ];
		MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail ));
		info->sendPacket( info->mCaller ? "USR" : "ANS", "%s %s", tEmail, info->mCookie );
	}
	else if ( info->mType == SERVER_FILETRANS && info->mCaller == 0 ) {
		info->send( "VER MSNFTP\r\n", 12 );
	}

	if ( info->mType == SERVER_NOTIFICATION )
		info->mIsMainThread = true;
	else if ( info->mType == SERVER_DISPATCH && MyOptions.UseGateway )
		info->mIsMainThread = true;

	if ( info->mIsMainThread ) {
		MSN_EnableMenuItems( TRUE );

		msnNsThread = info;
		hKeepAliveThreadEvt = ::CreateEvent( NULL, TRUE, FALSE, NULL );
		MSN_StartThread(( pThreadFunc )msn_keepAliveThread, NULL );
	}

	MSN_DebugLog( "Entering main recv loop" );
	info->mBytesInData = 0;
	while ( TRUE ) {
		int handlerResult;

		int recvResult = info->recv( info->mData + info->mBytesInData, sizeof( info->mData ) - info->mBytesInData );
		if ( recvResult == SOCKET_ERROR ) {
			MSN_DebugLog( "Connection %08p [%d] was abortively closed", info->s, GetCurrentThreadId());
			break;
		}

		if ( !recvResult ) {
			MSN_DebugLog( "Connection %08p [%d] was gracefully closed", info->s, GetCurrentThreadId());
			break;
		}

		info->mBytesInData += recvResult;

		if ( info->mCaller == 1 && info->mType == SERVER_FILETRANS ) {
			handlerResult = MSN_HandleMSNFTP( info, info->mData );
			if ( handlerResult )
				break;
		}
		else {
			while( TRUE ) {
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
					if ( isdigit(msg[0]) && isdigit(msg[1]) && isdigit(msg[2]))   //all error messages
						handlerResult = MSN_HandleErrors( info, msg );
					else
						handlerResult = MSN_HandleCommands( info, msg );
				}
				else handlerResult = MSN_HandleMSNFTP( info, msg );

				if ( handlerResult )
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
		if ( hKeepAliveThreadEvt )
			SetEvent( hKeepAliveThreadEvt );
	}
	else if ( info->mType == SERVER_SWITCHBOARD ) {
		if ( info->mJoinedContacts != NULL )
			free( info->mJoinedContacts );
	}

	MSN_DebugLog( "Thread [%d] ending now", GetCurrentThreadId() );
}

/////////////////////////////////////////////////////////////////////////////////////////
//  Added by George B. Hazan (ghazan@postman.ru)
//  The following code is required to abortively stop all started threads upon exit

static ThreadData*		sttThreads[ MAX_THREAD_COUNT ];	// up to MAX_THREAD_COUNT threads
static CRITICAL_SECTION	sttLock;

void __stdcall MSN_InitThreads()
{
	memset( sttThreads, 0, sizeof( sttThreads ));
	InitializeCriticalSection( &sttLock );
}

void __stdcall MSN_CloseConnections()
{
	int i;

	EnterCriticalSection( &sttLock );

	for ( i=0; i < MAX_THREAD_COUNT; i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T == NULL )
			continue;

		if ( T->mType == SERVER_SWITCHBOARD && T->s != NULL )
			T->sendPacket( "OUT", NULL );
	}

	LeaveCriticalSection( &sttLock );
}

void __stdcall MSN_CloseThreads()
{
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ ) {
		ThreadData* T = sttThreads[ i ];
		if ( T == NULL || T->s == NULL )
			continue;

		SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, LPARAM( T->s ), 0 );
		if ( s != INVALID_SOCKET )
			shutdown( s, 2 );
	}

	LeaveCriticalSection( &sttLock );
}

void Threads_Uninit( void )
{
	DeleteCriticalSection( &sttLock );
}

ThreadData* __stdcall MSN_GetThreadByContact( HANDLE hContact )
{
	EnterCriticalSection( &sttLock );

	ThreadData* T = NULL;

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{
		T = sttThreads[ i ];
		if ( T == NULL )
			continue;

		if ( T->mJoinedCount == 0 || T->mJoinedContacts == NULL || T->s == NULL )
			continue;

		if ( T->mJoinedContacts[0] == hContact )
			break;
	}

	LeaveCriticalSection( &sttLock );
	return T;
}

ThreadData* __stdcall MSN_GetUnconnectedThread( HANDLE hContact )
{
	EnterCriticalSection( &sttLock );

	ThreadData* T = NULL;

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{
		T = sttThreads[ i ];
		if ( T == NULL )
			continue;

		if ( T->mInitialContact == hContact )
			break;
	}

	LeaveCriticalSection( &sttLock );
	return T;
}

int __stdcall MSN_GetActiveThreads( ThreadData** parResult )
{
	int tCount = 0;
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{
		ThreadData* T = sttThreads[ i ];
		if ( T == NULL )
			continue;

		if ( T->mType == SERVER_SWITCHBOARD && T->mJoinedCount != 0 && T->mJoinedContacts != NULL )
			parResult[ tCount++ ] = T;
	}

	LeaveCriticalSection( &sttLock );
	return tCount;
}

ThreadData* __stdcall MSN_GetThreadByConnection( HANDLE s )
{
	ThreadData* tResult = NULL;

	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{
		ThreadData* T = sttThreads[ i ];
		if ( T == NULL )
			continue;

		if ( T->s == s )
		{	tResult = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return tResult;
}

ThreadData* __stdcall MSN_GetThreadByPort( WORD wPort )
{
	ThreadData* tResult = NULL;

	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{
		ThreadData* T = sttThreads[ i ];
		if ( T == NULL )
			continue;

		if ( T->mP2pSession != NULL && T->mP2pSession->mIncomingPort == wPort ) {
			tResult = T;
			break;
		}

		if ( T->mMsnFtp != NULL && T->mMsnFtp->mIncomingPort == wPort ) {
			tResult = T;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
	return tResult;
}

void __stdcall MSN_PingParentThread( ThreadData* parentThread, filetransfer* ft )
{
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{
		if ( sttThreads[ i ] == parentThread ) {
			parentThread->mP2pSession = ft;
			parentThread->mP2PInitTrid = p2p_sendPortionViaServer( ft, parentThread );
			break;
	}	}

	LeaveCriticalSection( &sttLock );
}

/////////////////////////////////////////////////////////////////////////////////////////
// class ThreadData members

static LONG sttThreadID = 1;

ThreadData::ThreadData()
{
	memset( this, 0, sizeof ThreadData );
	mUniqueID = MyInterlockedIncrement( &sttThreadID );
	mGatewayTimeout = 2;
	mWaitPeriod = 60;
}

ThreadData::~ThreadData()
{
	if ( s != NULL ) {
		MSN_DebugLog( "Closing connection handle %08X", s );
		Netlib_CloseHandle( s );
	}

	if ( mMsnFtp != NULL )
		delete mMsnFtp;

	if ( mUniqueID )
		p2p_unregisterThreadSession( mUniqueID );
	
	while (mFirstQueueItem != NULL)
	{
		TQueueItem* QI = mFirstQueueItem;
		mFirstQueueItem = mFirstQueueItem->next;
		free(QI);
}	}

void ThreadData::applyGatewayData( HANDLE hConn, bool isPoll )
{
	char szHttpPostUrl[300];
	getGatewayUrl( szHttpPostUrl, sizeof( szHttpPostUrl ), isPoll );

	MSN_DebugLog( "applying '%s' to %08X [%d]", szHttpPostUrl, this, GetCurrentThreadId() );

	NETLIBHTTPPROXYINFO nlhpi = {0};
	nlhpi.cbSize = sizeof(nlhpi);
	nlhpi.flags = 0;
	nlhpi.szHttpGetUrl = NULL;
	nlhpi.szHttpPostUrl = szHttpPostUrl;
	nlhpi.firstPostSequence = 1;
	MSN_CallService( MS_NETLIB_SETHTTPPROXYINFO, (WPARAM)hConn, (LPARAM)&nlhpi);
}

static char sttFormatString[] = "/gateway/gateway.dll?Action=open&Server=%s&IP=%s";

void ThreadData::getGatewayUrl( char* dest, int destlen, bool isPoll )
{
	if ( mSessionID[0] == 0 ) {
		if ( mType == SERVER_NOTIFICATION || mType == SERVER_DISPATCH )
			mir_snprintf( dest, destlen, sttFormatString, "NS", "messenger.hotmail.com" );
		else
			mir_snprintf( dest, destlen, sttFormatString, "SB", mServer );
		strcpy( mGatewayIP, MSN_DEFAULT_GATEWAY );
	}
	else mir_snprintf( dest, destlen, "/gateway/gateway.dll?%sSessionID=%s",
		( isPoll ) ? "Action=poll&" : "", mSessionID );
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
	strcpy( mGatewayIP, tGateIP );
	strcpy( mSessionID, tSessionID );
}

/////////////////////////////////////////////////////////////////////////////////////////
// thread start code

static void sttRegisterThread( ThreadData* s )
{
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
		if ( sttThreads[ i ] == NULL )
		{	sttThreads[ i ] = s;
			break;
		}

	LeaveCriticalSection( &sttLock );
}

static void sttUnregisterThread( ThreadData* s )
{
	EnterCriticalSection( &sttLock );

	for ( int i=0; i < MAX_THREAD_COUNT; i++ )
	{	if ( sttThreads[ i ] == s )
		{	sttThreads[ i ] = NULL;
			break;
	}	}

	LeaveCriticalSection( &sttLock );
}

/////////////////////////////////////////////////////////////////////////////////////////

struct FORK_ARG {
	HANDLE hEvent;
	pThreadFunc threadcode;
	ThreadData* arg;
};

static void __cdecl forkthread_r(struct FORK_ARG *fa)
{
	pThreadFunc callercode = fa->threadcode;
	ThreadData* arg = fa->arg;
	MSN_CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
	sttRegisterThread( arg );
	MSN_DebugLog( "Starting thread %08X (%08X)", GetCurrentThreadId(), callercode );
	SetEvent(fa->hEvent);
	__try {
		callercode(arg);
	} __finally {
		MSN_DebugLog( "Leaving thread %08X (%08X)", GetCurrentThreadId(), callercode );
		sttUnregisterThread( arg );
		delete arg;

		MSN_CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	}
	return;
}

void ThreadData::startThread( pThreadFunc parFunc )
{
	FORK_ARG fa = { CreateEvent(NULL, FALSE, FALSE, NULL), parFunc, this };
	unsigned long rc = _beginthread(( pThreadFunc )forkthread_r, 0, &fa );
	if ((unsigned long) -1L != rc)
		WaitForSingleObject(fa.hEvent, INFINITE);
	CloseHandle(fa.hEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////

struct FORK_ARG2 {
	HANDLE hEvent;
	pThreadFunc threadcode;
	void* arg;
};

static void __cdecl forkthread_r2(struct FORK_ARG2 *fa)
{
	pThreadFunc callercode = fa->threadcode;
	void* arg = fa->arg;
	MSN_CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
	MSN_DebugLog( "Starting thread %08X (%08X)", GetCurrentThreadId(), callercode );
	SetEvent(fa->hEvent);

	__try {
		callercode(arg);
	} __finally {
		MSN_DebugLog( "Leaving thread %08X (%08X)", GetCurrentThreadId(), callercode );
		MSN_CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	}
	return;
}

void __stdcall MSN_StartThread( pThreadFunc parFunc, void* arg )
{
	FORK_ARG2 fa = { CreateEvent(NULL, FALSE, FALSE, NULL), parFunc, arg };
	unsigned long rc = _beginthread(( pThreadFunc )forkthread_r2, 0, &fa );
	if ((unsigned long) -1L != rc)
		WaitForSingleObject(fa.hEvent, INFINITE);
	CloseHandle(fa.hEvent);
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

	int bufferSize = sizeof owner->mData;
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
