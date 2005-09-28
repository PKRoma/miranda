/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005     George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "jabber.h"
#include "jabber_list.h"

static int count;
static JABBER_LIST_ITEM *lists;
static CRITICAL_SECTION csLists;

static void JabberListFreeItemInternal( JABBER_LIST_ITEM *item );

#ifdef _DEBUG
static void PrintResource( int index )
{
	if ( index<0 || index>=count ) {
		JabberLog( "PrintResource(): index out-of-bound" );
		return;
	}

	int resourceCount = lists[index].resourceCount;
	if ( resourceCount <= 0 ) {
		JabberLog( "Resource count is zero" );
		return;
	}

	char* str = ( char* )malloc( resourceCount * 48 );
	if ( str == NULL )
		return;

	char* p = str;
	for ( int i=0; i < resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS* resource = &( lists[index].resource[i] );
		sprintf( p, "( %d )", resource->status );
		for ( ; *p!='\0'; p++ );
		if ( resource->resourceName )
			sprintf( p, "%s-", resource->resourceName );
		else
			strcpy( p, "( NULL )" );
		for ( ; *p!='\0'; p++ );
	}
	JabberLog( "Resource is now '%s'", str );
	free( str );
}
#endif

void JabberListInit( void )
{
	lists = NULL;
	count = 0;
	InitializeCriticalSection( &csLists );
}

void JabberListUninit( void )
{
	JabberListWipe();
	DeleteCriticalSection( &csLists );
}

void JabberListWipe( void )
{
	int i;

	EnterCriticalSection( &csLists );
	for( i=0; i<count; i++ )
		JabberListFreeItemInternal( &( lists[i] ));
	if ( lists != NULL ) {
		free( lists );
		lists = NULL;
	}
	count=0;
	LeaveCriticalSection( &csLists );
}

static void JabberListFreeItemInternal( JABBER_LIST_ITEM *item )
{
	int i;

	if ( item == NULL )
		return;

	if ( item->jid ) free( item->jid );
	if ( item->nick ) free( item->nick );
	for ( i=0; i<item->resourceCount; i++ ) {
		if ( item->resource[i].resourceName ) free( item->resource[i].resourceName );
		if ( item->resource[i].statusMessage ) free( item->resource[i].statusMessage );
		if ( item->resource[i].software ) free( item->resource[i].software );
		if ( item->resource[i].version ) free( item->resource[i].version );
		if ( item->resource[i].system ) free( item->resource[i].system );
	}
	if ( item->resource ) free( item->resource );
	if ( item->statusMessage ) free( item->statusMessage );
	if ( item->group ) free( item->group );
	if ( item->photoFileName ) {
		DeleteFile( item->photoFileName );
		free( item->photoFileName );
	}
	if ( item->messageEventIdStr ) free( item->messageEventIdStr );
	if ( item->name ) free( item->name );
	if ( item->type ) free( item->type );
	if ( item->service ) free( item->service );
	if ( item->list==LIST_ROSTER && item->ft ) JabberFileFreeFt( item->ft );
}

int JabberListExist( JABBER_LIST list, const char* jid )
{
	char szSrc[ JABBER_MAX_JID_LEN ];
	JabberStripJid( jid, szSrc, sizeof szSrc );

	EnterCriticalSection( &csLists );
	for ( int i=0; i<count; i++ ) {
		if ( lists[i].list == list ) {
			char szTempJid[ JABBER_MAX_JID_LEN ];
			if ( !JabberUtfCompareI( szSrc, JabberStripJid( lists[i].jid, szTempJid, sizeof szTempJid ))) {
			  	LeaveCriticalSection( &csLists );
				return i+1;
	}	}	}

	LeaveCriticalSection( &csLists );
	return 0;
}

JABBER_LIST_ITEM *JabberListAdd( JABBER_LIST list, const char* jid )
{
	char* s, *p, *q;
	JABBER_LIST_ITEM *item;

	EnterCriticalSection( &csLists );
	if (( item=JabberListGetItemPtr( list, jid )) != NULL ) {
		LeaveCriticalSection( &csLists );
		return item;
	}

	s = strdup( jid );
	// strip resource name if any
	if (( p=strchr( s, '@' )) != NULL ) {
		if (( q=strchr( p, '/' )) != NULL )
			*q = '\0';
	}

	lists = ( JABBER_LIST_ITEM * ) realloc( lists, sizeof( JABBER_LIST_ITEM )*( count+1 ));
	item = &( lists[count] );
	ZeroMemory( item, sizeof( JABBER_LIST_ITEM ));
	item->list = list;
	item->jid = s;
	item->status = ID_STATUS_OFFLINE;
	item->resource = NULL;
	item->resourceMode = RSMODE_LASTSEEN;
	item->defaultResource = -1;
	count++;
	LeaveCriticalSection( &csLists );

	return item;
}

void JabberListRemove( JABBER_LIST list, const char* jid )
{
	EnterCriticalSection( &csLists );
	int i = JabberListExist( list, jid );
	if ( !i ) {
		LeaveCriticalSection( &csLists );
		return;
	}
	i--;
	JabberListFreeItemInternal( &( lists[i] ));
	count--;
	memmove( lists+i, lists+i+1, sizeof( JABBER_LIST_ITEM )*( count-i ));
	lists = ( JABBER_LIST_ITEM * ) realloc( lists, sizeof( JABBER_LIST_ITEM )*count );
	LeaveCriticalSection( &csLists );
}

void JabberListRemoveList( JABBER_LIST list )
{
	int i = 0;
	while (( i=JabberListFindNext( list, i )) >= 0 )
		JabberListRemoveByIndex( i );
}

void JabberListRemoveByIndex( int index )
{
	EnterCriticalSection( &csLists );
	if ( index>=0 && index<count ) {
		JabberListFreeItemInternal( &( lists[index] ));
		count--;
		memmove( lists+index, lists+index+1, sizeof( JABBER_LIST_ITEM )*( count-index ));
		lists = ( JABBER_LIST_ITEM * ) realloc( lists, sizeof( JABBER_LIST_ITEM )*count );
	}
	LeaveCriticalSection( &csLists );
}

int JabberListAddResource( JABBER_LIST list, const char* jid, int status, const char* statusMessage )
{
	int j;
	char* p, *q;

	EnterCriticalSection( &csLists );
	int i = JabberListExist( list, jid );
	if ( !i ) {
		LeaveCriticalSection( &csLists );
		return 0;
	}
	JABBER_LIST_ITEM* LI = &lists[i-1];

	int bIsNewResource = false;

	if (( p=strchr( jid, '@' )) != NULL ) {
		if (( q=strchr( p, '/' )) != NULL ) {
			char* resource = q+1;
			if ( resource[0] ) {
				JABBER_RESOURCE_STATUS* r = LI->resource;
				if ( r == NULL ) {
					r = ( JABBER_RESOURCE_STATUS * ) malloc( sizeof( JABBER_RESOURCE_STATUS ));
					bIsNewResource = true;
					LI->resourceCount = 1;

					memset( r, 0, sizeof JABBER_RESOURCE_STATUS );
					r->status = status;
					r->resourceName = _strdup( resource );
					if ( statusMessage )
						r->statusMessage = _strdup( statusMessage );
				}
				else {
					for ( j=0; j < LI->resourceCount; j++, r++ ) {
						if ( !strcmp( r->resourceName, resource )) {
							// Already exist, update status and statusMessage
							r->status = status;
							replaceStr( r->statusMessage, statusMessage );
							break;
					}	}

					if ( j >= LI->resourceCount ) {
						// Not already exist, add new resource
						r = ( JABBER_RESOURCE_STATUS * ) realloc( LI->resource, ( LI->resourceCount+1 )*sizeof( JABBER_RESOURCE_STATUS ));
						bIsNewResource = true;
						JABBER_RESOURCE_STATUS* r2 = r + LI->resourceCount;
						memset( r2, 0, sizeof JABBER_RESOURCE_STATUS );
						r2->status = status;
						r2->resourceName = _strdup( resource );
						if ( statusMessage )
							r2->statusMessage = _strdup( statusMessage );
						LI->resourceCount++;
				}	}

				LI->resource = r;
#ifdef _DEBUG
				PrintResource( i );
#endif
			}
		}
		else {
			// No resource, update the main statusMessage
			if ( LI->statusMessage != NULL )
				free( LI->statusMessage );
			if ( statusMessage )
				LI->statusMessage = _strdup( statusMessage );
			else
				LI->statusMessage = NULL;
	}	}

	LeaveCriticalSection( &csLists );
	return bIsNewResource;
}

void JabberListRemoveResource( JABBER_LIST list, const char* jid )
{
	int j;
	char* p, *q;

	EnterCriticalSection( &csLists );
	int i = JabberListExist( list, jid );
	JABBER_LIST_ITEM* LI = &lists[i-1];
	if ( !i || LI == NULL ) {
		LeaveCriticalSection( &csLists );
		return;
	}

	if (( p=strchr( jid, '@' )) != NULL ) {
		if (( q=strchr( p, '/' )) != NULL ) {
			char* resource = q+1;
			if ( resource[0] ) {
				JABBER_RESOURCE_STATUS* r = LI->resource;
				for ( j=0; j < LI->resourceCount; j++, r++ ) {
					if ( !strcmp( r->resourceName, resource ))
						break;
				}
				if ( j < LI->resourceCount ) {
					// Found resource to be removed
					if ( LI->defaultResource == j )
						LI->defaultResource = -1;
					else if ( LI->defaultResource > j )
						LI->defaultResource--;
					if ( r->resourceName ) free( r->resourceName );
					if ( r->statusMessage ) free( r->statusMessage );
					if ( r->software ) free( r->software );
					if ( r->version ) free( r->version );
					if ( r->system ) free( r->system );
					if ( LI->resourceCount-- == 1 ) {
						free( r );
						LI->resource = NULL;
					}
					else {
						memmove( r, r+1, ( LI->resourceCount-j )*sizeof( JABBER_RESOURCE_STATUS ));
						LI->resource = ( JABBER_RESOURCE_STATUS * )realloc( LI->resource, LI->resourceCount*sizeof( JABBER_RESOURCE_STATUS ));
					}
#ifdef _DEBUG
					PrintResource( i );
#endif
	}	}	}	}

	LeaveCriticalSection( &csLists );
}

char* JabberListGetBestResourceNamePtr( const char* jid )
{
	char* res;

	EnterCriticalSection( &csLists );
	int i = JabberListExist( LIST_ROSTER, jid );
	JABBER_LIST_ITEM* LI = &lists[i-1];
	if ( !i || LI == NULL ) {
		LeaveCriticalSection( &csLists );
		return NULL;
	}

	if ( LI->resourceCount == 1 )
		res = LI->resource[0].resourceName;
	else {
		res = NULL;
		if ( LI->resourceMode == RSMODE_MANUAL || LI->resourceMode == RSMODE_LASTSEEN ) {
			if ( LI->defaultResource>=0 && LI->defaultResource < LI->resourceCount )
				res = LI->resource[ LI->defaultResource ].resourceName;
	}	}

	LeaveCriticalSection( &csLists );
	return res;
}

char* JabberListGetBestClientResourceNamePtr( const char* jid )
{
	EnterCriticalSection( &csLists );
	int i = JabberListExist( LIST_ROSTER, jid );
	JABBER_LIST_ITEM* LI = &lists[i-1];

	if ( !i || LI == NULL ) {
		LeaveCriticalSection( &csLists );
		return NULL;
	}

	char* res = JabberListGetBestResourceNamePtr( jid );
	if ( res == NULL ) {
		JABBER_RESOURCE_STATUS* r = LI->resource;
		int status = ID_STATUS_OFFLINE;
		res = NULL;
		for ( i=0; i < LI->resourceCount; i++ ) {
			int s = r[i].status;
			BOOL foundBetter = FALSE;
			switch ( s ) {
			case ID_STATUS_FREECHAT:
				foundBetter = TRUE;
				break;
			case ID_STATUS_ONLINE:
				if ( status != ID_STATUS_FREECHAT )
					foundBetter = TRUE;
				break;
			case ID_STATUS_DND:
				if ( status != ID_STATUS_FREECHAT && status != ID_STATUS_ONLINE )
					foundBetter = TRUE;
				break;
			case ID_STATUS_AWAY:
				if ( status != ID_STATUS_FREECHAT && status != ID_STATUS_ONLINE && status != ID_STATUS_DND )
					foundBetter = TRUE;
				break;
			case ID_STATUS_NA:
				if ( status != ID_STATUS_FREECHAT && status != ID_STATUS_ONLINE && status != ID_STATUS_DND && status != ID_STATUS_AWAY )
					foundBetter = TRUE;
				break;
			}
			if ( foundBetter ) {
				res = r[i].resourceName;
				status = s;
	}	}	}

	LeaveCriticalSection( &csLists );
	return res;
}

int JabberListFindNext( JABBER_LIST list, int fromOffset )
{
	EnterCriticalSection( &csLists );
	int i = ( fromOffset>=0 ) ? fromOffset : 0;
	for( ; i<count; i++ )
		if ( lists[i].list == list ) {
		  	LeaveCriticalSection( &csLists );
			return i;
		}
	LeaveCriticalSection( &csLists );
	return -1;
}

JABBER_LIST_ITEM *JabberListGetItemPtr( JABBER_LIST list, const char* jid )
{
	EnterCriticalSection( &csLists );
	int i = JabberListExist( list, jid );
	if ( !i ) {
		LeaveCriticalSection( &csLists );
		return NULL;
	}
	i--;
	LeaveCriticalSection( &csLists );
	return &( lists[i] );
}

JABBER_LIST_ITEM *JabberListGetItemPtrFromIndex( int index )
{
	EnterCriticalSection( &csLists );
	if ( index>=0 && index<count ) {
		LeaveCriticalSection( &csLists );
		return &( lists[index] );
	}
	LeaveCriticalSection( &csLists );
	return NULL;
}