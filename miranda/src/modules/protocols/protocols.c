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

int LoadProtoChains(void);

static HANDLE hAckEvent,hTypeEvent;

struct
{
	PROTOCOLDESCRIPTOR** items;
	int count, limit, increment;
	FSortFunc sortFunc;
}
static protos;

static int Proto_BroadcastAck(WPARAM wParam,LPARAM lParam)
{
	return NotifyEventHooks(hAckEvent,wParam,lParam);
}

int Proto_IsProtocolLoaded(WPARAM wParam,LPARAM lParam)
{
	if ( lParam ) {
		int idx;
		PROTOCOLDESCRIPTOR tmp = { 0, ( char* )lParam, 0 };
		if ( List_GetIndex(( SortedList* )&protos, &tmp, &idx ))
			return ( int )protos.items[idx];
	}

	return 0;
}

static int Proto_EnumProtocols(WPARAM wParam,LPARAM lParam)
{
	*( int* )wParam = protos.count;
	*( PROTOCOLDESCRIPTOR*** )lParam = protos.items;
	return 0;
}

static int Proto_RegisterModule(WPARAM wParam,LPARAM lParam)
{
	PROTOCOLDESCRIPTOR* pd = ( PROTOCOLDESCRIPTOR* )lParam, *p;
	if ( pd->cbSize != sizeof( PROTOCOLDESCRIPTOR ))
		return 1;

	p = mir_alloc( sizeof( PROTOCOLDESCRIPTOR ));
	if ( !p )
		return 2;

	*p = *pd;
	p->szName = mir_strdup( pd->szName );
	List_InsertPtr(( SortedList* )&protos, p );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Basic core services

static int Proto_RecvFile(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT* pre = ( PROTORECVEVENT* )ccs->lParam;
	char* szFile = pre->szMessage + sizeof( DWORD );
	char* szDescr = szFile + strlen( szFile ) + 1;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = ( char* )CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ccs->hContact, 0);
	dbei.timestamp = pre->timestamp;
	dbei.flags = ( pre->flags & PREF_CREATEREAD ) ? DBEF_READ : 0;
	dbei.eventType = EVENTTYPE_FILE;
	dbei.cbBlob = sizeof( DWORD ) + strlen( szFile ) + strlen( szDescr ) + 2;
	dbei.pBlob = ( PBYTE )pre->szMessage;
	CallService( MS_DB_EVENT_ADD, ( WPARAM )ccs->hContact, ( LPARAM )&dbei );
	return 0;
}

static int Proto_RecvMessage(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT *pre = ( PROTORECVEVENT* )ccs->lParam;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = ( char* )CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ccs->hContact, 0);
	dbei.timestamp = pre->timestamp;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.cbBlob = strlen( pre->szMessage ) + 1;
	if ( pre->flags & PREF_CREATEREAD )
		dbei.flags += DBEF_READ;
	if ( pre->flags & PREF_UTF )
		dbei.flags += DBEF_UTF;
	if ( pre->flags & PREF_UNICODE )
		dbei.cbBlob += sizeof( wchar_t )*( wcslen(( wchar_t* )&pre->szMessage[dbei.cbBlob+1] )+1 );

	dbei.pBlob = ( PBYTE ) pre->szMessage;
	CallService( MS_DB_EVENT_ADD, ( WPARAM ) ccs->hContact, ( LPARAM )&dbei );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// User Typing Notification services

static int Proto_ValidTypingContact(HANDLE hContact, char *szProto)
{
	if ( !hContact || !szProto )
		return 0;

	return ( CallProtoService(szProto,PS_GETCAPS,PFLAGNUM_4,0) & PF4_SUPPORTTYPING ) ? 1 : 0;
}

static int Proto_SelfIsTyping(WPARAM wParam,LPARAM lParam)
{
	if ( lParam == PROTOTYPE_SELFTYPING_OFF || lParam == PROTOTYPE_SELFTYPING_ON ) {
		char* szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
		if ( !szProto )
			return 0;

		if ( Proto_ValidTypingContact(( HANDLE )wParam, szProto ))
			CallProtoService( szProto, PSS_USERISTYPING, wParam, lParam );
	}

	return 0;
}

static int Proto_ContactIsTyping(WPARAM wParam,LPARAM lParam)
{
	int type = (int)lParam;
	char *szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
	if ( !szProto )
		return 0;

	if ( CallService( MS_IGNORE_ISIGNORED, wParam, IGNOREEVENT_TYPINGNOTIFY ))
		return 0;

	if ( type < PROTOTYPE_CONTACTTYPING_OFF )
		return 0;

	if ( Proto_ValidTypingContact(( HANDLE )wParam, szProto ))
		NotifyEventHooks( hTypeEvent, wParam, lParam );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int CompareProtos( const PROTOCOLDESCRIPTOR* p1, const PROTOCOLDESCRIPTOR* p2 )
{
	return strcmp( p1->szName, p2->szName );
}

void UnloadProtocolsModule()
{
	if ( hAckEvent ) {
		DestroyHookableEvent(hAckEvent);
		hAckEvent = NULL;
	}

	if ( protos.count ) {
		int i;
		for( i=0; i < protos.count; i++ ) {
			mir_free( protos.items[i]->szName);
			mir_free( protos.items[i] );
		}
		List_Destroy(( SortedList* )&protos );
	}
}

int LoadProtocolsModule(void)
{
	if ( LoadProtoChains() )
		return 1;

	protos.increment = 10;
	protos.sortFunc = CompareProtos;

	hAckEvent = CreateHookableEvent(ME_PROTO_ACK);
	hTypeEvent = CreateHookableEvent(ME_PROTO_CONTACTISTYPING);

	CreateServiceFunction( MS_PROTO_BROADCASTACK,     Proto_BroadcastAck     );
	CreateServiceFunction( MS_PROTO_ISPROTOCOLLOADED, Proto_IsProtocolLoaded );
	CreateServiceFunction( MS_PROTO_ENUMPROTOCOLS,    Proto_EnumProtocols    );
	CreateServiceFunction( MS_PROTO_REGISTERMODULE,   Proto_RegisterModule   );
	CreateServiceFunction( MS_PROTO_SELFISTYPING,     Proto_SelfIsTyping     );
	CreateServiceFunction( MS_PROTO_CONTACTISTYPING,  Proto_ContactIsTyping  );

	CreateServiceFunction( MS_PROTO_RECVFILE,         Proto_RecvFile         );
	CreateServiceFunction( MS_PROTO_RECVMSG,          Proto_RecvMessage      );
	return 0;
}
