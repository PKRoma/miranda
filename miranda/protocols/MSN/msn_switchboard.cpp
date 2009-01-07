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

int  ThreadData::contactJoined( HANDLE hContact )
{
	for ( int i=0; i < mJoinedCount; i++ )
		if ( mJoinedContacts[i] == hContact )
			return i+1;

	int ret = ++mJoinedCount;
	mJoinedContacts = ( HANDLE* )mir_realloc( mJoinedContacts, sizeof( HANDLE )*ret );
	mJoinedContacts[ ret-1 ] = hContact;
	return ret;
}

int  ThreadData::contactLeft( HANDLE hContact )
{
	int i;

	for ( i=0; i < mJoinedCount; i++ )
		if ( mJoinedContacts[ i ] == hContact )
			break;

	if ( i == mJoinedCount )
		return i;

	int ret = --mJoinedCount;
	memmove( mJoinedContacts + i, mJoinedContacts+i+1, sizeof( HANDLE )*( ret-i ));
	mJoinedContacts = ( HANDLE* )mir_realloc( mJoinedContacts, sizeof( HANDLE )*ret );
	return ret;
}
