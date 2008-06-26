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

static CRITICAL_SECTION sessionLock;

static int CompareFT( const filetransfer* p1, const filetransfer* p2 )
{
	return int( p1 - p2 );
}


static int CompareDC( const directconnection* p1, const directconnection* p2 )
{
	return int( p1 - p2 );
}

static OBJLIST<filetransfer> sessionList( 10, CompareFT );
static OBJLIST<directconnection> dcList( 10, CompareDC );


/////////////////////////////////////////////////////////////////////////////////////////
// add file session to a list

void  p2p_registerSession( filetransfer* ft )
{
	EnterCriticalSection( &sessionLock );
	sessionList.insert( ft );
	LeaveCriticalSection( &sessionLock );
}

/////////////////////////////////////////////////////////////////////////////////////////
// remove file session from a list

void  p2p_unregisterSession( filetransfer* ft )
{
	EnterCriticalSection( &sessionLock );
	int idx = sessionList.getIndex( ft );
	if ( idx > -1 ) {
		sessionList.remove( idx );
	}
	LeaveCriticalSection( &sessionLock );
}

/////////////////////////////////////////////////////////////////////////////////////////
// get session by some parameter

filetransfer*  p2p_getSessionByID( unsigned id )
{
	if ( id == 0 )
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->p2p_sessionid == id ) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown session id %08x", id );

	return ft;
}

filetransfer*  p2p_getSessionByUniqueID( unsigned id )
{
	if ( id == 0 )
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->p2p_acksessid == id ) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown unique id %08x", id );

	return ft;
}


bool  p2p_sessionRegistered( filetransfer* ft )
{
	if ( ft != NULL && ft->p2p_appID == 0)
		return true;

	EnterCriticalSection( &sessionLock );
	int idx = sessionList.getIndex( ft );
	LeaveCriticalSection( &sessionLock );
	return idx > -1;
}

filetransfer*  p2p_getThreadSession( HANDLE hContact, TInfoType mType )
{
	EnterCriticalSection( &sessionLock );

	filetransfer* result = NULL;
	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->std.hContact == hContact && FT->tType == mType ) {
			result = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	return result;
}

filetransfer*  p2p_getAvatarSession( HANDLE hContact )
{
	EnterCriticalSection( &sessionLock );

	filetransfer* result = NULL;
	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->std.hContact == hContact && !FT->std.sending && 
			FT->p2p_type == MSN_APPID_AVATAR ) {
			result = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	return result;
}

bool  p2p_isAvatarOnly( HANDLE hContact )
{
	EnterCriticalSection( &sessionLock );

	bool result = true;
	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		result &= FT->std.hContact != hContact || FT->p2p_type != MSN_APPID_FILE;
	}

	LeaveCriticalSection( &sessionLock );
	return result;
}

void  p2p_clearDormantSessions( void )
{
	EnterCriticalSection( &sessionLock );

	time_t ts = time( NULL );
	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->p2p_waitack && ( ts - FT->ts ) > 60 ) 
		{
			LeaveCriticalSection( &sessionLock );
			p2p_unregisterSession( FT );
			EnterCriticalSection( &sessionLock );
			i = 0;
	}	}

	for ( int j=0; j < dcList.getCount(); j++ ) {
		directconnection* DC = &dcList[j];
		if (( ts - DC->ts ) > 120 ) {
			LeaveCriticalSection( &sessionLock );
			p2p_unregisterDC( DC );
			EnterCriticalSection( &sessionLock );
			j = 0;
	}	}

	LeaveCriticalSection( &sessionLock );
}

void  p2p_redirectSessions( HANDLE hContact )
{
	EnterCriticalSection( &sessionLock );

	ThreadData* T = MSN_GetP2PThreadByContact( hContact );
	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->std.hContact == hContact && !FT->std.sending && 
			FT->std.currentFileProgress < FT->std.currentFileSize &&
			( T == NULL || ( FT->tType != T->mType && FT->tType != 0 ))) 
			p2p_sendRedirect( FT );
	}

	LeaveCriticalSection( &sessionLock );
}

void  p2p_cancelAllSessions( void )
{
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		p2p_sendCancel( FT );
	}

	dcList.destroy();

	LeaveCriticalSection( &sessionLock );
}

filetransfer*  p2p_getSessionByCallID( const char* CallID )
{
	if ( CallID == NULL )
		return NULL;

	EnterCriticalSection( &sessionLock );

	filetransfer* ft = NULL;
	for ( int i=0; i < sessionList.getCount(); i++ ) {
		filetransfer* FT = &sessionList[i];
		if ( FT->p2p_callID != NULL && !strcmp( FT->p2p_callID, CallID )) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown session call id %s", CallID );

	return ft;
}


void  p2p_registerDC( directconnection* dc )
{
	EnterCriticalSection( &sessionLock );
	dcList.insert( dc );
	LeaveCriticalSection( &sessionLock );
}

void  p2p_unregisterDC( directconnection* dc )
{
	EnterCriticalSection( &sessionLock );
	int idx = dcList.getIndex( dc );
	if ( idx > -1 ) {
		dcList.remove( idx );
	}
	LeaveCriticalSection( &sessionLock );
}

directconnection*  p2p_getDCByCallID( const char* CallID )
{
	if ( CallID == NULL )
		return NULL;

	EnterCriticalSection( &sessionLock );

	directconnection* dc = NULL;
	for ( int i=0; i < dcList.getCount(); i++ ) {
		directconnection* DC = &dcList[i];
		if ( DC->callId != NULL && !strcmp( DC->callId, CallID )) {
			dc = DC;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );

	return dc;
}

/////////////////////////////////////////////////////////////////////////////////////////
// external functions

void P2pSessions_Init()
{
	InitializeCriticalSection( &sessionLock );
}

void P2pSessions_Uninit()
{
	EnterCriticalSection( &sessionLock );

	sessionList.destroy();
	dcList.destroy();

	LeaveCriticalSection( &sessionLock );
	DeleteCriticalSection( &sessionLock );
}
