/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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

static int DbEventTypeRegister(WPARAM wParam, LPARAM lParam)
{
	DBEVENTTYPEDESCR* et = ( DBEVENTTYPEDESCR* )lParam;

	int idx;
	if ( !List_GetIndex(( SortedList* )&eventTypes, et, &idx )) {
		DBEVENTTYPEDESCR* p = mir_alloc( sizeof( DBEVENTTYPEDESCR ));
		*p = *et;
		p->module = mir_strdup( et->module );
		p->descr  = mir_strdup( et->descr  );
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
	
	char szServiceName[100];
	mir_snprintf( szServiceName, sizeof(szServiceName), "%s/GetEventText%d", dbei->szModule, dbei->eventType );
	if ( ServiceExists( szServiceName ))
		return CallService( szServiceName, wParam, lParam );
	
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

   egt->datatype &= ~DBVTF_DENYUNICODE;
	if ( egt->datatype == DBVT_WCHAR )
	{
		WCHAR* msg;
		if ( dbei->flags & DBEF_UTF )
			Utf8DecodeCP( dbei->pBlob, egt->codepage, &msg );
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

int InitUtils()
{
	CreateServiceFunction(MS_DB_EVENT_REGISTERTYPE, DbEventTypeRegister);
	CreateServiceFunction(MS_DB_EVENT_GETTYPE, DbEventTypeGet);
	CreateServiceFunction(MS_DB_EVENT_GETTEXT, DbEventGetText);
	return 0;
}
