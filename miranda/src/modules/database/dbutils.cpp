/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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

#include "commonheaders.h"
#include "profilemanager.h"

struct
{
	DBEVENTTYPEDESCR** items;
	int count, limit, increment;
	FSortFunc sortFunc;
}
static eventTypes;

static BOOL bModuleInitialized = FALSE;

static int DbEventTypeRegister(WPARAM wParam, LPARAM lParam)
{
	DBEVENTTYPEDESCR* et = ( DBEVENTTYPEDESCR* )lParam;

	int idx;
	if ( !List_GetIndex(( SortedList* )&eventTypes, et, &idx )) {
		DBEVENTTYPEDESCR* p = ( DBEVENTTYPEDESCR* )mir_alloc( sizeof( DBEVENTTYPEDESCR ));
		p->cbSize = DBEVENTTYPEDESCR_SIZE;
		p->module = mir_strdup( et->module );
		p->eventType = et->eventType; 
		p->descr  = mir_strdup( et->descr  );
		p->textService = NULL;
		p->iconService = NULL;
		p->eventIcon = NULL;
		p->flags = 0;
		if ( et->cbSize == DBEVENTTYPEDESCR_SIZE ) {
			if ( et->textService )
				p->textService = mir_strdup( et->textService );
			if ( et->iconService )
				p->iconService = mir_strdup( et->iconService );
			p->eventIcon = et->eventIcon;
			p->flags = et->flags;
		}
		if ( !p->textService ) {
			char szServiceName[100];
			mir_snprintf( szServiceName, sizeof(szServiceName), "%s/GetEventText%d", p->module, p->eventType );
			p->textService = mir_strdup( szServiceName );
		}
		if ( !p->iconService ) {
			char szServiceName[100];
			mir_snprintf( szServiceName, sizeof(szServiceName), "%s/GetEventIcon%d", p->module, p->eventType );
			p->iconService = mir_strdup( szServiceName );
		}
		List_Insert(( SortedList* )&eventTypes, p, idx );
	}

	return 0;
}

static int DbEventTypeGet(WPARAM wParam, LPARAM lParam)
{
	DBEVENTTYPEDESCR tmp;
	int idx;

	tmp.module = ( char* )wParam;
	tmp.eventType = lParam;
	if ( !List_GetIndex(( SortedList* )&eventTypes, &tmp, &idx ))
		return 0;

	return ( int )eventTypes.items[idx];
}

static int DbEventGetText(WPARAM wParam, LPARAM lParam)
{
	DBEVENTGETTEXT* egt = (DBEVENTGETTEXT*)lParam;
	BOOL bIsDenyUnicode = (egt->datatype & DBVTF_DENYUNICODE);

	DBEVENTINFO* dbei = egt->dbei;
	DBEVENTTYPEDESCR* et = ( DBEVENTTYPEDESCR* )DbEventTypeGet( ( WPARAM )dbei->szModule, ( LPARAM )dbei->eventType );

	if ( et && ServiceExists( et->textService ))
		return CallService( et->textService, wParam, lParam );

	if ( !dbei->pBlob ) return 0;

	if ( dbei->eventType == EVENTTYPE_FILE ) {
		char* filename = ((char *)dbei->pBlob) + sizeof(DWORD);
		char* descr = filename + lstrlenA( filename ) + 1;
		switch ( egt->datatype ) {
		case DBVT_WCHAR:
			return ( int )a2t( *descr == 0 ? filename : descr );
		case DBVT_ASCIIZ:
			return ( int )mir_strdup( *descr == 0 ? filename : descr );
		}
		return 0;
	}

	// temporary fix for bug with event types conflict between jabber chat states notifications
	// and srmm's status changes, must be commented out in future releases
	if ( dbei->eventType == 25368 && dbei->cbBlob == 1 && dbei->pBlob[0] == 1 )
		return 0;

	egt->datatype &= ~DBVTF_DENYUNICODE;
	if ( egt->datatype == DBVT_WCHAR )
	{
		WCHAR* msg;
		if ( dbei->flags & DBEF_UTF )
			Utf8DecodeCP( NEWSTR_ALLOCA(( char* )dbei->pBlob), egt->codepage, &msg );
		else {
			// ушлепкам типа скотта торжественно посвящается
			int msglen = strlen(( char* )dbei->pBlob) + 1, msglenW = 0;
			if ( msglen != (int) dbei->cbBlob ) {
				int i, count = (( dbei->cbBlob - msglen ) / sizeof( WCHAR ));
				WCHAR* p = ( WCHAR* )&dbei->pBlob[ msglen ];
				for (  i=0; i < count; i++ ) {
					if ( p[i] == 0 ) {
						msglenW = i;
						break;
			}	}	}

			if ( msglenW > 0 && msglenW < msglen && !bIsDenyUnicode )
				msg = mir_wstrdup(( WCHAR* )&dbei->pBlob[ msglen ] );
			else {
				msg = ( WCHAR* )mir_alloc( sizeof(TCHAR) * msglen );
				MultiByteToWideChar( egt->codepage, 0, (char *) dbei->pBlob, -1, msg, msglen );
		}	}
		return ( int )msg;
	}
	else if ( egt->datatype == DBVT_ASCIIZ ) {
		char* msg = mir_strdup(( char* )dbei->pBlob );
		if (dbei->flags & DBEF_UTF)
			Utf8DecodeCP( msg, egt->codepage, NULL );

      return ( int )msg;
	}
	return 0;
}

static int DbEventGetIcon( WPARAM wParam, LPARAM lParam )
{
	DBEVENTINFO* dbei = ( DBEVENTINFO* )lParam;
	HICON icon;
	DBEVENTTYPEDESCR* et = ( DBEVENTTYPEDESCR* )DbEventTypeGet( ( WPARAM )dbei->szModule, ( LPARAM )dbei->eventType );

	if ( et && ServiceExists( et->iconService )) {
		icon = ( HICON )CallService( et->iconService, wParam, lParam );
		if ( icon )
			return ( int )icon;
	}
	if ( et && et->eventIcon )
		icon = ( HICON )CallService( MS_SKIN2_GETICONBYHANDLE, 0, ( LPARAM )et->eventIcon );
	if ( !icon ) {
		char szName[100];
		mir_snprintf( szName, sizeof( szName ), "eventicon_%s%d", dbei->szModule, dbei->eventType );
		icon = ( HICON )CallService( MS_SKIN2_GETICON, 0, ( LPARAM )szName );
	}

	if ( !icon )
	{
		switch( dbei->eventType ) {
		case EVENTTYPE_URL:
			icon = LoadSkinIcon( SKINICON_EVENT_URL );
			break;

		case EVENTTYPE_FILE:
			icon = LoadSkinIcon( SKINICON_EVENT_FILE );
			break;

		default: // EVENTTYPE_MESSAGE and unknown types
			icon = LoadSkinIcon( SKINICON_EVENT_MESSAGE );
			break;
		}
	}

  if ( wParam & LR_SHARED )
    return ( int )icon;
  else
    return ( int )CopyIcon( icon );
}

static int CompareEventTypes( const DBEVENTTYPEDESCR* p1, const DBEVENTTYPEDESCR* p2 )
{
	int result = strcmp( p1->module, p2->module );
	if ( result )
		return result;

	return p1->eventType - p2->eventType;
}

int InitUtils()
{
	bModuleInitialized = TRUE;
	
	eventTypes.increment = 10;
	eventTypes.sortFunc = ( FSortFunc )CompareEventTypes;

	CreateServiceFunction(MS_DB_EVENT_REGISTERTYPE, DbEventTypeRegister);
	CreateServiceFunction(MS_DB_EVENT_GETTYPE, DbEventTypeGet);
	CreateServiceFunction(MS_DB_EVENT_GETTEXT, DbEventGetText);
	CreateServiceFunction(MS_DB_EVENT_GETICON, DbEventGetIcon);
	return 0;
}

void UnloadEventsModule()
{
	int i;

	if ( !bModuleInitialized ) return;

	for ( i=0; i < eventTypes.count; i++ ) {
		DBEVENTTYPEDESCR* p = eventTypes.items[i];
		mir_free( p->module );
		mir_free( p->descr );
		mir_free( p->textService );
		mir_free( p->iconService );
		mir_free( p );
	}

	List_Destroy(( SortedList* )&eventTypes );
}
