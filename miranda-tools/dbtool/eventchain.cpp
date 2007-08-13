/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

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
#include "dbtool.h"

static DWORD ofsThisEvent,ofsPrevEvent;
static DWORD ofsDestPrevEvent;
static DWORD eventCount;
static DWORD ofsFirstUnread,timestampFirstUnread;
static DWORD memsize = 0;
static DBEvent* memblock = NULL;

char* Utf8EncodeUcs2( const wchar_t* src )
{
	int len = wcslen( src );
	char* result = ( char* )malloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	{	const wchar_t* s = src;
		BYTE*	d = ( BYTE* )result;

		while( *s ) {
			int U = *s++;

			if ( U < 0x80 ) {
				*d++ = ( BYTE )U;
			}
			else if ( U < 0x800 ) {
				*d++ = 0xC0 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x003F );
			}
			else {
				*d++ = 0xE0 + ( U >> 12 );
				*d++ = 0x80 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x3F );
		}	}

		*d = 0;
	}

	return result;
}

static void ConvertOldEvent( DBEvent* dbei )
{
	int msglen = strlen(( char* )dbei->blob) + 1, msglenW = 0;
	if ( msglen != (int) dbei->cbBlob ) {
		int i, count = (( dbei->cbBlob - msglen ) / sizeof( WCHAR ));
		WCHAR* p = ( WCHAR* )&dbei->blob[ msglen ];
		for (  i=0; i < count; i++ ) {
			if ( p[i] == 0 ) {
				msglenW = i;
				break;
	}	}	}

	if ( msglenW > 0 && msglenW <= msglen ) {
		char* utf8str = Utf8EncodeUcs2(( WCHAR* )&dbei->blob[ msglen ] );
		dbei->cbBlob = strlen( utf8str )+1;
		dbei->flags |= DBEF_UTF;
		if (offsetof(DBEvent,blob)+dbei->cbBlob > memsize) {
			memsize = offsetof(DBEvent,blob)+dbei->cbBlob;
			memblock = (DBEvent*)realloc(memblock, memsize);
		}
		memcpy( &dbei->blob, utf8str, dbei->cbBlob );
		free(utf8str);
}	}

static void WriteOfsNextToPrevious(DWORD ofsPrev,DBContact *dbc,DWORD ofsNext)
{
	if(ofsPrev)
		WriteSegment(ofsPrev+offsetof(DBEvent,ofsNext),&ofsNext,sizeof(DWORD));
	else
		dbc->ofsFirstEvent=ofsNext;
}

static void FinishUp(DWORD ofsLast,DBContact *dbc)
{
	WriteOfsNextToPrevious(ofsLast,dbc,0);
	if(eventCount!=dbc->eventCount)
		AddToStatus(STATUS_WARNING,TranslateT("Event count marked wrongly: correcting"));
	dbc->eventCount=eventCount;
	dbc->ofsLastEvent=ofsLast;
	if(opts.bMarkRead) {
		dbc->ofsFirstUnreadEvent=0;
		dbc->timestampFirstUnread=0;
	}
	else {
		dbc->ofsFirstUnreadEvent=ofsFirstUnread;
		dbc->timestampFirstUnread=timestampFirstUnread;
	}
	if (memsize && memblock) {
		free(memblock);
		memsize = 0;
		memblock = NULL;
	}
}

int WorkEventChain(DWORD ofsContact,DBContact *dbc,int firstTime)
{
	DBEvent *dbeNew,dbeOld;
	DWORD ofsDestThis;
	int isUnread=0;

	if(firstTime) {
		ofsPrevEvent=0;
		ofsDestPrevEvent=0;
		ofsThisEvent=dbc->ofsFirstEvent;
		eventCount=0;
		ofsFirstUnread=timestampFirstUnread=0;
		if(opts.bEraseHistory) {
			dbc->eventCount=0;
			dbc->ofsFirstEvent=0;
			dbc->ofsLastEvent=0;
			dbc->ofsFirstUnreadEvent=0;
			dbc->timestampFirstUnread=0;
			return ERROR_NO_MORE_ITEMS;
	}	}

	if(ofsThisEvent==0) {
		FinishUp(ofsDestPrevEvent,dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	if(!SignatureValid(ofsThisEvent,DBEVENT_SIGNATURE)) {
		AddToStatus(STATUS_ERROR,TranslateT("Event chain corrupted, further entries ignored"));
		FinishUp(ofsDestPrevEvent,dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	if(PeekSegment(ofsThisEvent,&dbeOld,sizeof(dbeOld))!=ERROR_SUCCESS) {
		FinishUp(ofsDestPrevEvent,dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	if(firstTime) {
		if(!(dbeOld.flags&DBEF_FIRST)) {
			AddToStatus(STATUS_WARNING,TranslateT("First event not marked as such: correcting"));
			dbeOld.flags|=DBEF_FIRST;
		}
		dbeOld.ofsPrev=ofsContact;
	}
	else if(dbeOld.flags&DBEF_FIRST) {
		AddToStatus(STATUS_WARNING,TranslateT("Event marked as first which is not: correcting"));
		dbeOld.flags&=~DBEF_FIRST;
	}

	if(dbeOld.flags&~(DBEF_FIRST|DBEF_READ|DBEF_SENT|DBEF_RTL|DBEF_UTF)) {
		AddToStatus(STATUS_WARNING,TranslateT("Extra flags found in event: removing"));
		dbeOld.flags&=(DBEF_FIRST|DBEF_READ|DBEF_SENT|DBEF_RTL|DBEF_UTF);
	}

	if(!(dbeOld.flags&(DBEF_READ|DBEF_SENT))) {
		if(opts.bMarkRead) dbeOld.flags|=DBEF_READ;
		else if(ofsFirstUnread==0) {
			if(dbc->ofsFirstUnreadEvent!=ofsThisEvent || dbc->timestampFirstUnread!=dbeOld.timestamp)
				AddToStatus(STATUS_WARNING,TranslateT("First unread event marked wrong: fixing"));
			isUnread=1;
	}	}

	if(dbeOld.cbBlob>1024*1024 || dbeOld.cbBlob==0) {
		AddToStatus(STATUS_ERROR,TranslateT("Infeasibly large event blob: skipping"));
		ofsThisEvent=dbeOld.ofsNext;
		return ERROR_SUCCESS;
	}

	if((dbeOld.ofsModuleName=ConvertModuleNameOfs(dbeOld.ofsModuleName))==0) {
		ofsThisEvent=dbeOld.ofsNext;
		return ERROR_SUCCESS;
	}

	if(!firstTime && dbeOld.ofsPrev!=ofsPrevEvent)
		AddToStatus(STATUS_WARNING,TranslateT("Event not backlinked correctly: fixing"));

	dbeOld.ofsPrev=ofsDestPrevEvent;
	if (offsetof(DBEvent,blob)+dbeOld.cbBlob > memsize) {
		memsize = offsetof(DBEvent,blob)+dbeOld.cbBlob;
		memblock = (DBEvent*)realloc(memblock, memsize);
	}

	dbeNew=memblock;
	if(ReadSegment(ofsThisEvent,dbeNew,offsetof(DBEvent,blob)+dbeOld.cbBlob)!=ERROR_SUCCESS) {
		FinishUp(ofsDestPrevEvent,dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	dbeNew->flags=dbeOld.flags;
	dbeNew->ofsModuleName=dbeOld.ofsModuleName;
	dbeNew->ofsPrev=dbeOld.ofsPrev;
	dbeNew->ofsNext=0;

	if ( dbeOld.eventType == EVENTTYPE_MESSAGE && opts.bConvertUtf && !( dbeOld.flags & DBEF_UTF ))
		ConvertOldEvent(dbeNew);

	if((ofsDestThis=WriteSegment(WSOFS_END,dbeNew,offsetof(DBEvent,blob)+dbeNew->cbBlob))==WS_ERROR) {
		free(memblock);
		memblock = NULL;
		memsize=0;
		return ERROR_HANDLE_DISK_FULL;
	}

	if(isUnread) {
		ofsFirstUnread=ofsDestThis;
		timestampFirstUnread=dbeOld.timestamp;
	}

	eventCount++;
	WriteOfsNextToPrevious(ofsDestPrevEvent,dbc,ofsDestThis);
	ofsDestPrevEvent=ofsDestThis;
	ofsPrevEvent=ofsThisEvent;
	ofsThisEvent=dbeOld.ofsNext;
	return ERROR_SUCCESS;
}
