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

int DbEventGetText(WPARAM wParam, LPARAM lParam)
{
	DBEVENTINFO* dbei = ( DBEVENTINFO* )wParam;
	if ( dbei->eventType != EVENTTYPE_MESSAGE )
		return 0;

	if ( lParam == DBVT_WCHAR )
	{
		WCHAR* msg;
		if ( dbei->flags & DBEF_UTF )
			Utf8Decode( dbei->pBlob, &msg );
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

			if ( msglenW > 0 && msglenW < msglen )
				msg = mir_wstrdup(( WCHAR* )&dbei->pBlob[ msglen ] );
			else {
				msg = ( WCHAR* )mir_alloc( sizeof(TCHAR) * msglen );
				MultiByteToWideChar(CP_ACP, 0, (char *) dbei->pBlob, -1, msg, msglen);
		}	}
		return ( int )msg;
	}
	else if ( lParam == DBVT_ASCIIZ ) {
		char* msg = mir_strdup(( char* )dbei->pBlob );
		if (dbei->flags & DBEF_UTF)
			Utf8Decode( msg, NULL );

      return ( int )msg;
	}
	return 0;
}

int InitUtils()
{
	CreateServiceFunction(MS_DB_EVENT_GETTEXT, DbEventGetText);
	return 0;
}
