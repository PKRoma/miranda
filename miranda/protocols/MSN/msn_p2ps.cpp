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

static int              sessionCount = 0;
static filetransfer**   sessionList = NULL;
static CRITICAL_SECTION sessionLock;

/////////////////////////////////////////////////////////////////////////////////////////
// add file session to a list

void __stdcall p2p_registerSession( filetransfer* ft )
{
	EnterCriticalSection( &sessionLock );
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

filetransfer* __stdcall p2p_getSessionByID( long ID )
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
		MSN_DebugLog( "Ignoring unknown session id %ld", ID );

	return ft;
}

filetransfer* __stdcall p2p_getSessionByMsgID( long ID )
{
	if ( ID == 0 ) 
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection( &sessionLock );

	for ( int i=0; i < sessionCount; i++ ) {
		filetransfer* FT = sessionList[i];
		if ( FT->p2p_msgid-1 == ID ) {
			ft = FT;
			break;
	}	}

	LeaveCriticalSection( &sessionLock );
	if ( ft == NULL )
		MSN_DebugLog( "Ignoring unknown message id %ld", ID );

	return ft;
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

	LeaveCriticalSection( &sessionLock );
	DeleteCriticalSection( &sessionLock );
}
