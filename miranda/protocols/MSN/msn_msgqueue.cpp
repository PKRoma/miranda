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

//a few little functions to manage queuing send message requests until the
//connection is established

static MsgQueueEntry* msgQueue;
static size_t msgQueueCount, msgQueueAllocCount;
static CRITICAL_SECTION csMsgQueue;
static int msgQueueSeq;

void MsgQueue_Init(void)
{
	msgQueueCount = 0;
	msgQueueAllocCount = 0;
	msgQueue = NULL;
	msgQueueSeq = 1;
	InitializeCriticalSection(&csMsgQueue);
}

void MsgQueue_Uninit(void)
{
	MsgQueue_Clear();
	DeleteCriticalSection(&csMsgQueue);
}

int  MsgQueue_Add( HANDLE hContact, int msgType, const char* msg, int msgSize, filetransfer* ft, int flags )
{
	EnterCriticalSection( &csMsgQueue );
	if (msgQueueCount >= msgQueueAllocCount)
		msgQueue = ( MsgQueueEntry* )mir_realloc( msgQueue, sizeof( MsgQueueEntry )*( ++msgQueueAllocCount ));

	int seq = msgQueueSeq++;

	MsgQueueEntry& E = msgQueue[ msgQueueCount++ ];
	E.hContact = hContact;
	E.msgSize = msgSize;
	E.msgType = msgType;
	if ( msgSize <= 0 )
		E.message = mir_strdup( msg );
	else
		memcpy( E.message = ( char* )mir_alloc( msgSize ), msg, msgSize );
	E.ft = ft;
	E.seq = seq;
	E.flags = flags;
	E.allocatedToThread = 0;
	E.ts = time(NULL);

	LeaveCriticalSection( &csMsgQueue );
	return seq;
}

// shall we create another session?
HANDLE  MsgQueue_CheckContact(HANDLE hContact, time_t tsc)
{
	EnterCriticalSection(&csMsgQueue);

	time_t ts = time(NULL);
	HANDLE ret = NULL;
	for(unsigned i=0; i < msgQueueCount; i++)
	{
		if (msgQueue[i].hContact == hContact && (tsc == 0 || (ts - msgQueue[i].ts) < tsc))
		{	
			ret = hContact;
			break;
		}	
	}

	LeaveCriticalSection(&csMsgQueue);
	return ret;
}

//for threads to determine who they should connect to
HANDLE  MsgQueue_GetNextRecipient(void)
{
	EnterCriticalSection( &csMsgQueue );

	HANDLE ret = NULL;
	for( unsigned i=0; i < msgQueueCount; i++ )
	{
		MsgQueueEntry& E = msgQueue[ i ];
		if ( !E.allocatedToThread )
		{
			E.allocatedToThread = 1;
			ret = E.hContact;

			while( ++i < msgQueueCount )
				if ( msgQueue[i].hContact == ret )
					msgQueue[i].allocatedToThread = 1;

			break;
	}	}

	LeaveCriticalSection( &csMsgQueue );
	return ret;
}

//deletes from list. Must mir_free() return value
bool  MsgQueue_GetNext( HANDLE hContact, MsgQueueEntry& retVal )
{
	unsigned i;

	EnterCriticalSection( &csMsgQueue );
	for( i=0; i < msgQueueCount; i++ )
		if ( msgQueue[ i ].hContact == hContact )
			break;
	
	bool res = i != msgQueueCount;
	if ( res )
	{	
		retVal = msgQueue[ i ];
		msgQueueCount--;
		memmove( msgQueue+i, msgQueue+i+1, sizeof( MsgQueueEntry )*( msgQueueCount-i ));
	}
	LeaveCriticalSection( &csMsgQueue );
	return res;
}

int  MsgQueue_NumMsg( HANDLE hContact )
{
	int res = 0;
	EnterCriticalSection( &csMsgQueue );

	for( unsigned i=0; i < msgQueueCount; i++ )
		res += msgQueue[ i ].hContact == hContact;
	
	LeaveCriticalSection( &csMsgQueue );
	return res;
}

void  MsgQueue_Clear( HANDLE hContact, bool msg )
{
	unsigned i;

	EnterCriticalSection( &csMsgQueue );
	if (hContact == NULL)
	{

		for(i=0; i < msgQueueCount; i++ )
		{
			const MsgQueueEntry& E = msgQueue[ i ];
			if ( E.msgSize == 0 )
				MSN_SendBroadcast( E.hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, 
					( HANDLE )E.seq, ( LPARAM )MSN_Translate( "Message delivery failed" ));
			mir_free( E.message );
		}
		mir_free( msgQueue );

		msgQueueCount = 0;
		msgQueueAllocCount = 0;
		msgQueue = NULL;
		msgQueueSeq = 1;
	}
	else
	{
		time_t ts = time(NULL);
		for(i=0; i < msgQueueCount; i++)
		{
			const MsgQueueEntry& E = msgQueue[i];
			if (E.hContact == hContact && (!msg || E.msgSize == 0))
			{
				bool msgfnd = E.msgSize == 0 && E.ts < ts;
				int seq = E.seq;
				
				mir_free( E.message );
				msgQueueCount--;
				memmove( msgQueue+i, msgQueue+i+1, sizeof( MsgQueueEntry )*( msgQueueCount-i ));

				if ( msgfnd ) {
					LeaveCriticalSection(&csMsgQueue);
					MSN_SendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE )seq, 
						( LPARAM )MSN_Translate( "Message delivery failed" ));
					i = 0;
					EnterCriticalSection( &csMsgQueue );
				}
			}
		}
	}
	LeaveCriticalSection(&csMsgQueue);
}
