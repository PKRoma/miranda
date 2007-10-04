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

struct ServerGroupItem
{
	char* id;
	char* name; // in UTF8
};

static int CompareGrp( const ServerGroupItem* p1, const ServerGroupItem* p2 )
{
	return int( p1 - p2 );
}

static LIST<ServerGroupItem> grpList( 10, CompareGrp );


/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddGroup - adds new server group to the list

void MSN_AddGroup( const char* grpName, const char *grpId, bool init )
{
	ServerGroupItem* p = new ServerGroupItem;
	p->id = mir_strdup( grpId );
	p->name = mir_strdup( grpName );
	
	grpList.insert( p );

	if ( init )
	{
		wchar_t* szNewName;
		mir_utf8decode((char*)grpName, &szNewName);
#ifdef _UNICODE
		CallService(MS_CLIST_GROUPCREATE, 0, (LPARAM)szNewName);
#else
		CallService(MS_CLIST_GROUPCREATE, 0, (LPARAM)data.grpName);
#endif
		mir_free(szNewName);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DeleteGroup - deletes a group from the list

void MSN_DeleteGroup( const char* pId )
{
	ServerGroupItem* prev = NULL;

	for ( int i=0; i < grpList.getCount(); i++ ) 
	{
		ServerGroupItem* p = grpList[i];
		if ( !strcmp( p->id, pId )) 
		{
			mir_free( p->id );
			mir_free( p->name );
			delete p;
			grpList.remove(i);
			break;			
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_FreeGroups - clears the server groups list

void MSN_FreeGroups(void)
{
	for ( int i=0; i < grpList.getCount(); i++ ) 
	{
		ServerGroupItem* p = grpList[i];
		mir_free( p->id );
		mir_free( p->name );
		delete p;
	}
	grpList.destroy();
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetGroupById - tries to return a group name associated with given UUID

LPCSTR MSN_GetGroupById( const char* pId )
{
	for ( int i=0; i < grpList.getCount(); i++ ) 
	{
		const ServerGroupItem* p = grpList[i];
		if ( strcmp( p->id, pId ) == 0 )
			return p->name;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetGroupByName - tries to return a group UUID associated with the given name 

LPCSTR MSN_GetGroupByName( const char* pName )
{
	for ( int i=0; i < grpList.getCount(); i++ ) 
	{
		const ServerGroupItem* p = grpList[i];
		if ( strcmp( p->name, pName ) == 0 )
			return p->id;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SetGroupName - sets a new name to a server group

void MSN_SetGroupName( const char* pId, const char* pNewName )
{
	for ( int i=0; i < grpList.getCount(); i++ ) 
	{
		ServerGroupItem* p = grpList[i];
		if ( strcmp( p->id, pId ) == 0 ) 
		{
			replaceStr(p->name, pNewName );
			break;
		}	
	}	
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_MoveContactToGroup - sends a contact to the specified group 

void MSN_MoveContactToGroup( HANDLE hContact, const char* grpName )
{
	if ( !MyOptions.ManageServer ) return;

	if ( strcmp( grpName, "MetaContacts Hidden Group" ) == 0 )
		return;

	LPCSTR szId = NULL;
	char szContactID[ 100 ], szGroupID[ 100 ];
	if ( MSN_GetStaticString( "ID", hContact, szContactID, sizeof( szContactID )))
		return;

	if ( MSN_GetStaticString( "GroupID", hContact, szGroupID, sizeof( szGroupID )))
		szGroupID[ 0 ] = 0;

	bool bInsert = false, bDelete = szGroupID[0] != 0;

	if ( grpName != NULL )
	{
		szId = MSN_GetGroupByName( grpName );
		if ( szId == NULL )
		{
			MSN_ABAddGroup( grpName );
			szId = MSN_GetGroupByName( grpName );
		}
		else {
			if ( !strcmp( szGroupID, szId )) bDelete = false;
			else                             bInsert = true;
		}
	}

	if ( bInsert )
	{
		MSN_ABAddDelContactGroup(szContactID, szId, "ABGroupContactAdd");
		MSN_SetString( hContact, "GroupID", szId );
	}

	if ( bDelete )
 	{
		MSN_ABAddDelContactGroup(szContactID, szId, "ABGroupContactDelete");
		MSN_DeleteSetting( hContact, "GroupID" );
		MSN_RemoveEmptyGroups();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_RemoveEmptyGroups - removes empty groups from the server list

void MSN_RemoveEmptyGroups( void )
{
	if ( !MyOptions.ManageServer ) return;

	unsigned *cCount = ( unsigned* )mir_calloc( grpList.getCount() * sizeof( unsigned ));

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL )
	{
		if ( MSN_IsMyContact( hContact )) 
		{
			char szGroupID[ 100 ];
			if ( !MSN_GetStaticString( "GroupID", hContact, szGroupID, sizeof( szGroupID ))) 
			{
				for ( int i=0; i < grpList.getCount(); i++ ) 
					if ( strcmp( grpList[i]->id, szGroupID ) == 0 ) 
					{ ++cCount[i]; break; }
			}
		}
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
	}

	for ( int i=0; i < grpList.getCount(); i++ ) 
	{
		if ( cCount[i] == 0 ) 
			MSN_ABAddDelContactGroup(NULL, grpList[i]->id, "ABGroupDelete");
	}
	mir_free( cCount );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_RenameServerGroup - renames group on the server

void MSN_RenameServerGroup( LPCSTR szId, const char* newName )
{
	MSN_SetGroupName(szId, newName);
	MSN_ABRenameGroup(newName, szId);
}


/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DeleteServerGroup - deletes group from the server

void MSN_DeleteServerGroup( LPCSTR szId )
{
	if ( !MyOptions.ManageServer ) return;

	MSN_DeleteGroup(szId);
	MSN_ABAddDelContactGroup(NULL, szId, "ABGroupDelete");

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL )
	{
		char szGroupID[ 100 ];
		if ( !MSN_GetStaticString( "GroupID", hContact, szGroupID, sizeof( szGroupID ))) 
		{
			if (strcmp(szGroupID, szId) == 0)
				MSN_DeleteSetting( hContact, "GroupID" );
		}
	}
	hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_UploadServerGroups - adds a group to the server list and contacts into the group

void  MSN_UploadServerGroups( char* group )
{
	if ( !MyOptions.ManageServer ) return;

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		if ( MSN_IsMyContact( hContact )) {
			DBVARIANT dbv;
			if ( !DBGetContactSettingStringUtf( hContact, "CList", "Group", &dbv )) {
				char szGroupID[ 100 ];
				if ( group == NULL || ( strcmp( group, dbv.pszVal ) == 0 &&
					MSN_GetStaticString( "GroupID", hContact, szGroupID, sizeof( szGroupID )) != 0 )) 
				{
					MSN_MoveContactToGroup( hContact, dbv.pszVal );
				}
				MSN_FreeVariant( &dbv );
		}	}

		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SyncContactToServerGroup - moves contact into appropriate group according to server
// if contact in multiple server groups it get removed from all of them other themn it's
// in or the last one

void MSN_SyncContactToServerGroup( HANDLE hContact, const char* szContId, ezxml_t cgrp )
{
	if ( !MyOptions.ManageServer ) return;

	DBVARIANT dbv;
	if ( DBGetContactSettingStringUtf( hContact, "CList", "Group", &dbv ))
		dbv.pszVal = "";

	const char* szGrpIdF = NULL;
	while( cgrp != NULL)
	{
		const char* szGrpId  = ezxml_txt(cgrp);
		cgrp = ezxml_next(cgrp);

		if (strcmp(MSN_GetGroupById(szGrpId), dbv.pszVal) == 0 || 
			(cgrp == NULL && szGrpIdF == NULL)) 
			szGrpIdF = szGrpId;
		else 
			MSN_ABAddDelContactGroup(szContId, szGrpId, "ABGroupContactDelete");
	}

	if ( szGrpIdF != NULL ) {
		MSN_SetString( hContact, "GroupID", szGrpIdF );
		DBWriteContactSettingStringUtf( hContact, "CList", "Group", 
			MSN_GetGroupById( szGrpIdF ));
	}
	else {
		DBDeleteContactSetting( hContact, "CList", "Group" );
		MSN_DeleteSetting( hContact, "GroupID" );
	}	
	MSN_SetString( hContact, "ID", szContId );

	if ( *dbv.pszVal ) MSN_FreeVariant( &dbv );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Msn_SendNickname - update our own nickname on the server

void  MSN_SendNicknameUtf(char* nickname)
{
	MSN_SetStringUtf(NULL, "Nick", nickname);
	
	MSN_SetNicknameUtf(nickname);
	MSN_ABUpdateNick(nickname, NULL);
	mir_free(nickname);
}

void  MSN_SetNicknameUtf(char* nickname)
{
	const size_t urlNickSz = strlen(nickname) * 3 + 1;
	char* urlNick = (char*)alloca(urlNickSz);

	UrlEncode(nickname, urlNick, urlNickSz);
	msnNsThread->sendPacket("PRP", "MFN %s", urlNick);
}
