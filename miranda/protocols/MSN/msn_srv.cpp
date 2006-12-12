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

struct ServerGroupItem
{
	char* id;
	char* name; // in UTF8
	int   number;

	ServerGroupItem* next;
};

static ServerGroupItem* sttFirst = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddGroup - adds new server group to the list

bool MSN_AddGroup( const char* pName, const char* pId )
{
	ServerGroupItem* p = new ServerGroupItem;
	if ( p == NULL )
		return false;

	p->number = -1;
	p->id = mir_strdup( pId );
	p->name = mir_strdup( pName );
	p->next = sttFirst;
	sttFirst = p;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DeleteGroup - deletes a group from the list

void MSN_DeleteGroup( const char* pId )
{
	ServerGroupItem* prev = NULL;

	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p->next ) {
		if ( !strcmp( p->id, pId )) {
			if ( prev == NULL ) sttFirst = p->next;
			else                prev->next = p->next;
			mir_free( p->id );
			mir_free( p->name );
			delete p;
			return;
		}

		prev = p;
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_FreeGroups - clears the server groups list

void MSN_FreeGroups()
{
	ServerGroupItem* p1;

	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p1 ) {
		p1 = p->next;

		mir_free( p->id );
		mir_free( p->name );
		delete p;
	}

	sttFirst = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetGroupById - tries to return a group name associated with given UUID

LPCSTR MSN_GetGroupById( const char* pId )
{
	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p->next )
		if ( stricmp( p->id, pId ) == 0 )
			return p->name;

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetGroupByName - tries to return a group UUID associated with the given name 

LPCSTR MSN_GetGroupByName( const char* pName )
{
	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p->next )
		if ( strcmp( p->name, pName ) == 0 )
			return p->id;

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetGroupByNumber - tries to return a group UUID associated with the given id 

LPCSTR MSN_GetGroupByNumber( int pNumber )
{
	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p->next )
		if ( p->number == pNumber )
			return p->id;

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_MoveContactToGroup - sends a contact to the specified group 

void MSN_MoveContactToGroup( HANDLE hContact, const char* grpName )
{
	if ( strcmp( grpName, "MetaContacts Hidden Group" ) == 0 )
		return;

	LPCSTR szId;
	char szContactID[ 100 ], szGroupID[ 100 ];
	if ( MSN_GetStaticString( "ID", hContact, szContactID, sizeof( szContactID )))
		return;

	if ( MSN_GetStaticString( "GroupID", hContact, szGroupID, sizeof( szGroupID )))
		szGroupID[ 0 ] = 0;

	bool bInsert = szGroupID[0] == 0, bDelete = szGroupID[0] != 0;

	if ( grpName != NULL )
	{
		if (( szId = MSN_GetGroupByName( grpName )) == NULL ) {
			MSN_AddServerGroup( grpName );
			if (( szId = MSN_GetGroupByName( grpName )) == NULL )
				return;
		}

		if ( !strcmp( szGroupID, szId ))	bDelete = false;
		else                             bInsert = true;
	}
	else bInsert = false;		

	if ( bInsert )
		msnNsThread->sendPacket( "ADC", "FL C=%s %s", szContactID, szId );

	if ( bDelete )
		msnNsThread->sendPacket( "REM", "FL %s %s", szContactID, szGroupID );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SetGroupName - sets a new name to a server group

void MSN_SetGroupName( const char* pId, const char* pNewName )
{
	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p->next ) {
		if ( strcmp( p->id, pId ) == 0 ) {
			mir_free( p->name );
			p->name = mir_strdup( pNewName );
			return;
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SetGroupNumber - associate a server group with Miranda's group number 

void MSN_SetGroupNumber( const char* pId, int pNumber )
{
	for ( ServerGroupItem* p = sttFirst; p != NULL; p = p->next ) {
		if ( strcmp( p->id, pId ) == 0 ) {
			p->number = pNumber;
			return;
}	}	}
