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

static int              sessionCount = 0;
static filetransfer**   sessionList = NULL;
static CRITICAL_SECTION sessionLock;

/////////////////////////////////////////////////////////////////////////////////////////
// add file session to a list

void __stdcall p2p_registerSession( filetransfer* ft )
{
	EnterCriticalSection( &sessionLock );

	bool bIsFirst = true;
	for ( int i=0; i < sessionCount; i++ ) {
		if ( sessionList[i]->std.hContact == ft->std.hContact ) {
			bIsFirst = false;
			break;
	}	}

	sessionList = ( filetransfer** )realloc( sessionList, sizeof( void* ) * ( sessionCount+1 ));
	sessionList[ sessionCount++ ] = ft;

	LeaveCriticalSection( &sessionLock );
}

/////////////////////////////////////////////////////////////////////////////////////////
// remove file session from a list

void __stdcall p2p_unregisterSession( filetransfer* ft )
{
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		if ( sessionList[i] == ft ) {
         delete sessionList[i];
			while( i < sessionCount-1 ) {
				sessionList[ i ] = sessionList[ i+1 ];
				i++;
			}
			sessionCount--;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
}

/////////////////////////////////////////////////////////////////////////////////////////
// get session by some parameter

filetransfer* __stdcall p2p_getSessionByID( unsigned ID )
{
	if ( ID == 0 )
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->p2p_sessionid == ID ) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown session id %lu", ID );

	return ft;
}

filetransfer* __stdcall p2p_getSessionByMsgID( unsigned ID )
{
	if ( ID == 0 )
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->p2p_msgid == ID || FT->p2p_msgid == (ID + 1) ) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown message id %lu", ID );

	return ft;
}

filetransfer* __stdcall p2p_getAnotherContactSession( filetransfer* ft )
{
   filetransfer* result = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->std.hContact == ft->std.hContact && FT != ft ) {
			result = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	return result;
}

BOOL __stdcall p2p_sessionRegistered( filetransfer* ft )
{
    BOOL result = FALSE;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( sessionList[i] == ft ) {
			result = TRUE;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	return result;
}

filetransfer* __stdcall p2p_getFirstSession( HANDLE hContact )
{
   filetransfer* result = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->std.hContact == hContact ) {
			result = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	return result;
}

void __stdcall p2p_clearDormantSessions( void )
{
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( MSN_GetP2PThreadByContact( FT->std.hContact ) == NULL ) {
			LeaveCriticalSection( &sessionLock );
			p2p_unregisterSession( FT );
			EnterCriticalSection( &sessionLock );
			i = 0;
	}	}

	LeaveCriticalSection( &sessionLock );
}

void __stdcall p2p_redirectSessions( HANDLE hContact )
{
	EnterCriticalSection( &sessionLock );

	ThreadData* T = MSN_GetP2PThreadByContact( hContact );
	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->std.hContact == hContact && !FT->std.sending && 
			( T == NULL || ( FT->tType != T->mType && FT->tType != 0 ))) 
			p2p_sendRedirect( T, FT );
	}

	LeaveCriticalSection( &sessionLock );
}

void __stdcall p2p_cancelAllSessions( void )
{
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		p2p_sendCancel( MSN_GetP2PThreadByContact( FT->std.hContact ), FT );
	}

	LeaveCriticalSection( &sessionLock );
}

filetransfer* __stdcall p2p_getSessionByCallID( const char* CallID )
{
	if ( CallID == NULL )
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->p2p_callID == NULL )
			continue;

		if ( !strcmp( FT->p2p_callID, CallID )) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown session call id %s", CallID );

	return ft;
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

	for ( int i=0; i < sessionCount; i++ )
		delete sessionList[i];
	if ( sessionList != NULL )
		free( sessionList );

	LeaveCriticalSection( &sessionLock );
	DeleteCriticalSection( &sessionLock );
}
