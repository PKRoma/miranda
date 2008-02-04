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
#include "m_protoint.h"

int LoadProtoChains(void);
int LoadProtoOptions( void );

HANDLE hAccListChanged;
static HANDLE hAckEvent,hTypeEvent;
static BOOL bModuleInitialized = FALSE;

typedef struct
{
	int id;
	const char* name;
}
	TServiceListItem;

struct
{
	PROTOCOLDESCRIPTOR** items;
	int count, limit, increment;
	FSortFunc sortFunc;
}
static protos;

struct
{
	TServiceListItem** items;
	int count, limit, increment;
	FSortFunc sortFunc;
}
static serviceItems;

static int Proto_BroadcastAck(WPARAM wParam,LPARAM lParam)
{
	return NotifyEventHooks(hAckEvent,wParam,lParam);
}

PROTOCOLDESCRIPTOR* Proto_IsProtocolLoaded( const char* szProtoName )
{
	if ( szProtoName ) {
		int idx;
		PROTOCOLDESCRIPTOR tmp;
		tmp.szName = ( char* )szProtoName;
		if ( List_GetIndex(( SortedList* )&protos, &tmp, &idx ))
			return protos.items[idx];
	}

	return NULL;
}

int srvProto_IsLoaded(WPARAM wParam,LPARAM lParam)
{
	return (int)Proto_IsProtocolLoaded(( char* )lParam );
}

int Proto_EnumProtocols(WPARAM wParam,LPARAM lParam)
{
	*( int* )wParam = protos.count;
	*( PROTOCOLDESCRIPTOR*** )lParam = protos.items;
	return 0;
}

static int Proto_RegisterModule(WPARAM wParam,LPARAM lParam)
{
	PROTOCOLDESCRIPTOR* pd = ( PROTOCOLDESCRIPTOR* )lParam, *p;
	if ( pd->cbSize != sizeof( PROTOCOLDESCRIPTOR ) && pd->cbSize != PROTOCOLDESCRIPTOR_V3_SIZE )
		return 1;

	p = mir_alloc( sizeof( PROTOCOLDESCRIPTOR ));
	if ( !p )
		return 2;

	if ( pd->cbSize == PROTOCOLDESCRIPTOR_V3_SIZE ) {
		memset( p, 0, sizeof( PROTOCOLDESCRIPTOR ));
		p->cbSize = PROTOCOLDESCRIPTOR_V3_SIZE;
		p->type = pd->type;
		if ( p->type == PROTOTYPE_PROTOCOL ) {
			// let's create a new container
			PROTO_INTERFACE* ppi = AddDefaultAccount( pd->szName );
			if ( ppi ) {
				PROTOACCOUNT* pa = Proto_GetAccount( pd->szName );
				if ( pa == NULL ) {
					pa = (PROTOACCOUNT*)mir_calloc( sizeof( PROTOACCOUNT ));
					pa->cbSize = sizeof(PROTOACCOUNT);
					pa->type = PROTOTYPE_PROTOCOL;
					pa->szModuleName = mir_strdup( pd->szName );
					pa->szProtoName = mir_strdup( pd->szName );
					pa->tszAccountName = mir_a2t( pd->szName );
					pa->bIsVisible = pa->bIsEnabled = TRUE;
					List_InsertPtr(( SortedList* )&accounts, pa );
				}
				pa->bOldProto = TRUE;
				pa->ppro = ppi;
				p->fnUninit = FreeDefaultAccount;
			}
		}
	}
	else *p = *pd;
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
// 0.8.0+ - accounts

PROTOACCOUNT* Proto_GetAccount( const char* accName )
{
	int idx;
	PROTOACCOUNT temp;
	temp.szModuleName = ( char* )accName;
	if ( List_GetIndex(( SortedList* )&accounts, &temp, &idx ) == 0 )
		return NULL;

	return accounts.items[idx];
}

static int srvProto_GetAccount(WPARAM wParam, LPARAM lParam)
{
	return ( int )Proto_GetAccount(( char* )lParam );
}

static int Proto_EnumAccounts(WPARAM wParam, LPARAM lParam)
{
	*( int* )wParam = accounts.count;
	*( PROTOACCOUNT*** )lParam = accounts.items;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CallProtoServiceInt( HANDLE hContact, const char *szModule, const char *szService, WPARAM wParam, LPARAM lParam )
{
	int idx;
	char svcName[ MAXMODULELABELLENGTH ];
	PROTOACCOUNT* pa = ( PROTOACCOUNT* )Proto_GetAccount( szModule );
	if ( pa == NULL )
		return CALLSERVICE_NOTFOUND;

	if ( !pa->bOldProto ) {
		PROTO_INTERFACE* ppi;
		if (( ppi = pa->ppro ) == NULL )
			return CALLSERVICE_NOTFOUND;
		else {
			TServiceListItem item;
			item.name = szService;
			if ( List_GetIndex(( SortedList* )&serviceItems, &item, &idx ) != 0 ) {
				switch( serviceItems.items[ idx ]->id ) {
					case  1: return ( int )ppi->vtbl->AddToList( ppi, wParam, (PROTOSEARCHRESULT*)lParam ); break;
					case  2: return ( int )ppi->vtbl->AddToListByEvent( ppi, HIWORD(wParam), LOWORD(wParam), (HANDLE)lParam ); break;
					case  3: return ( int )ppi->vtbl->Authorize( ppi, ( HANDLE )wParam ); break;
					case  4: return ( int )ppi->vtbl->AuthDeny( ppi, ( HANDLE )wParam, ( const char* )lParam ); break;
					case  5: return ( int )ppi->vtbl->AuthRecv( ppi, hContact, ( PROTORECVEVENT* )lParam ); break;
					case  6: return ( int )ppi->vtbl->AuthRequest( ppi, hContact, ( char* )lParam ); break;
					case  7: return ( int )ppi->vtbl->ChangeInfo( ppi, wParam, ( void* )lParam ); break;
					case  8: return ( int )ppi->vtbl->FileAllow( ppi, hContact, ( HANDLE )wParam, ( char* )lParam ); break;
					case  9: return ( int )ppi->vtbl->FileCancel( ppi, hContact, ( HANDLE )wParam ); break;
					case 10: return ( int )ppi->vtbl->FileDeny( ppi, hContact, ( HANDLE )wParam, ( char* )lParam ); break;
					case 11: {
						PROTOFILERESUME* pfr = ( PROTOFILERESUME* )lParam;
						return ( int )ppi->vtbl->FileResume( ppi, ( HANDLE )wParam, &pfr->action, &pfr->szFilename ); break;
					}
					case 12: return ( int )ppi->vtbl->GetCaps( ppi, wParam ); break;
					case 13: return ( int )ppi->vtbl->GetIcon( ppi, wParam ); break;
					case 14: return ( int )ppi->vtbl->GetInfo( ppi, hContact, wParam ); break;
					case 15: return ( int )ppi->vtbl->SearchBasic( ppi, ( char* )lParam ); break;
					case 16: return ( int )ppi->vtbl->SearchByEmail( ppi, ( char* )lParam ); break;
					case 17: {
						PROTOSEARCHBYNAME* psbn = ( PROTOSEARCHBYNAME* )lParam;
						return ( int )ppi->vtbl->SearchByName( ppi, psbn->pszNick, psbn->pszFirstName, psbn->pszLastName ); break;
					}
					case 18: return ( int )ppi->vtbl->SearchAdvanced( ppi, ( HWND )lParam ); break;
					case 19: return ( int )ppi->vtbl->CreateExtendedSearchUI ( ppi, ( HWND )lParam ); break;
					case 20: return ( int )ppi->vtbl->RecvContacts( ppi, hContact, ( PROTORECVEVENT* )lParam ); break;
					case 21: return ( int )ppi->vtbl->RecvFile( ppi, hContact, ( PROTORECVFILE* )lParam ); break;
					case 22: return ( int )ppi->vtbl->RecvMsg( ppi, hContact, ( PROTORECVEVENT* )lParam ); break;
					case 23: return ( int )ppi->vtbl->RecvUrl( ppi, hContact, ( PROTORECVEVENT* )lParam ); break;
					case 24: return ( int )ppi->vtbl->SendContacts( ppi, hContact, HIWORD(wParam), LOWORD(wParam), ( HANDLE* )lParam ); break;
					case 25: return ( int )ppi->vtbl->SendFile( ppi, hContact, ( const char* )wParam, ( char** )lParam ); break;
					case 26: return ( int )ppi->vtbl->SendMsg( ppi, hContact, wParam, ( const char* )lParam ); break;
					case 27: return ( int )ppi->vtbl->SendUrl( ppi, hContact, wParam, ( const char* )lParam ); break;
					case 28: return ( int )ppi->vtbl->SetApparentMode( ppi, hContact, wParam ); break;
					case 29: return ( int )ppi->vtbl->SetStatus( ppi, wParam ); break;
					case 30: return ( int )ppi->vtbl->GetAwayMsg( ppi, hContact ); break;
					case 31: return ( int )ppi->vtbl->RecvAwayMsg( ppi, hContact, wParam, ( PROTORECVEVENT* )lParam ); break;
					case 32: return ( int )ppi->vtbl->SendAwayMsg( ppi, hContact, ( HANDLE )wParam, ( const char* )lParam ); break;
					case 33: return ( int )ppi->vtbl->SetAwayMsg( ppi, wParam, ( const char* )lParam ); break;
					case 34: return ( int )ppi->vtbl->UserIsTyping( ppi, ( HANDLE )wParam, lParam ); break;
	}	}	}	}

	mir_snprintf( svcName, sizeof(svcName), "%s%s", szModule, szService );
	return CallService( svcName, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////

int CallContactService( HANDLE hContact, const char *szProtoService, WPARAM wParam, LPARAM lParam )
{
	int i;
	DBVARIANT dbv;
	int ret;
	PROTOACCOUNT* pa;
	CCSDATA ccs = { hContact, szProtoService, wParam, lParam };

	for ( i = 0;; i++ ) {
		char str[10];
		_itoa( i, str, 10 );
		if ( DBGetContactSettingString( hContact, "_Filter", str, &dbv ))
			break;

		if (( ret = CallProtoServiceInt( hContact, dbv.pszVal, szProtoService, i+1, lParam )) != CALLSERVICE_NOTFOUND ) {
			//chain was started, exit
			mir_free( dbv.pszVal );
			return ret;
		}
		mir_free( dbv.pszVal );
	}
	if ( DBGetContactSettingString( hContact, "Protocol", "p", &dbv ))
		return 1;

	pa = Proto_GetAccount( dbv.pszVal );
	if ( pa == NULL || pa->ppro == NULL )
		ret = 1;
	else {
		if ( pa->bOldProto )
			ret = CallProtoServiceInt( hContact, dbv.pszVal, szProtoService, (WPARAM)(-1), ( LPARAM)&ccs );
		else
			ret = CallProtoServiceInt( hContact, dbv.pszVal, szProtoService, wParam, lParam );
		if ( ret == CALLSERVICE_NOTFOUND )
			ret = 1;
	}

	mir_free( dbv.pszVal );
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int CompareProtos( const PROTOCOLDESCRIPTOR* p1, const PROTOCOLDESCRIPTOR* p2 )
{
	return strcmp( p1->szName, p2->szName );
}

static int CompareAccounts( const PROTOACCOUNT* p1, const PROTOACCOUNT* p2 )
{
	return strcmp( p1->szModuleName, p2->szModuleName );
}

static int CompareServiceItems( const TServiceListItem* p1, const TServiceListItem* p2 )
{
	return strcmp( p1->name, p2->name );
}

static void InsertServiceListItem( int id, const char* szName )
{
	TServiceListItem* p = ( TServiceListItem* )mir_alloc( sizeof( TServiceListItem ));
	p->id = id;
	p->name = szName;
	List_InsertPtr(( SortedList* )&serviceItems, p );
}

int LoadProtocolsModule(void)
{
	bModuleInitialized = TRUE;

	if ( LoadProtoChains() )
		return 1;

	protos.increment = 10;
	protos.sortFunc = CompareProtos;

	accounts.increment = 10;
	accounts.sortFunc = CompareAccounts;

	serviceItems.increment = 10;
	serviceItems.sortFunc = CompareServiceItems;

	InsertServiceListItem(  1, PS_ADDTOLIST );
	InsertServiceListItem(  2, PS_ADDTOLISTBYEVENT );
	InsertServiceListItem(  3, PS_AUTHALLOW );
	InsertServiceListItem(  4, PS_AUTHDENY );
	InsertServiceListItem(  5, PSS_ADDED );
	InsertServiceListItem(  6, PSS_AUTHREQUEST );
	InsertServiceListItem(  7, PS_CHANGEINFO );
	InsertServiceListItem(  8, PSS_FILEALLOW );
	InsertServiceListItem(  9, PSS_FILECANCEL );
	InsertServiceListItem( 10, PSS_FILEDENY );
	InsertServiceListItem( 11, PS_FILERESUME );
	InsertServiceListItem( 12, PS_GETCAPS );
	InsertServiceListItem( 13, PS_LOADICON );
	InsertServiceListItem( 14, PSS_GETINFO );
	InsertServiceListItem( 15, PS_BASICSEARCH );
	InsertServiceListItem( 16, PS_SEARCHBYEMAIL );
	InsertServiceListItem( 17, PS_SEARCHBYNAME );
	InsertServiceListItem( 18, PS_SEARCHBYADVANCED );
	InsertServiceListItem( 19, PS_CREATEADVSEARCHUI );
	InsertServiceListItem( 20, PSR_CONTACTS );
	InsertServiceListItem( 21, PSR_FILE );
	InsertServiceListItem( 22, PSR_MESSAGE );
	InsertServiceListItem( 23, PSR_URL );
	InsertServiceListItem( 24, PSS_CONTACTS );
	InsertServiceListItem( 25, PSS_FILE );
	InsertServiceListItem( 26, PSS_MESSAGE );
	InsertServiceListItem( 27, PSS_URL );
	InsertServiceListItem( 28, PSS_SETAPPARENTMODE );
	InsertServiceListItem( 29, PS_SETSTATUS );
	InsertServiceListItem( 30, PSS_GETAWAYMSG );
	InsertServiceListItem( 31, PSR_AWAYMSG );
	InsertServiceListItem( 32, PSS_AWAYMSG );
	InsertServiceListItem( 33, PS_SETAWAYMSG );
	InsertServiceListItem( 34, PSS_USERISTYPING );

	hAckEvent = CreateHookableEvent(ME_PROTO_ACK);
	hTypeEvent = CreateHookableEvent(ME_PROTO_CONTACTISTYPING);
	hAccListChanged = CreateHookableEvent(ME_PROTO_ACCLISTCHANGED);

	CreateServiceFunction( MS_PROTO_BROADCASTACK,     Proto_BroadcastAck     );
	CreateServiceFunction( MS_PROTO_ISPROTOCOLLOADED, srvProto_IsLoaded      );
	CreateServiceFunction( MS_PROTO_ENUMPROTOS,       Proto_EnumProtocols    );
	CreateServiceFunction( MS_PROTO_REGISTERMODULE,   Proto_RegisterModule   );
	CreateServiceFunction( MS_PROTO_SELFISTYPING,     Proto_SelfIsTyping     );
	CreateServiceFunction( MS_PROTO_CONTACTISTYPING,  Proto_ContactIsTyping  );

	CreateServiceFunction( MS_PROTO_RECVFILE,         Proto_RecvFile         );
	CreateServiceFunction( MS_PROTO_RECVMSG,          Proto_RecvMessage      );

	CreateServiceFunction( "Proto/EnumProtocols",     Proto_EnumAccounts     );
	CreateServiceFunction( MS_PROTO_ENUMACCOUNTS,     Proto_EnumAccounts     );
	CreateServiceFunction( MS_PROTO_GETACCOUNT,       srvProto_GetAccount    );

	return LoadProtoOptions();
}

void UnloadProtocolsModule()
{
	int i;

	if ( !bModuleInitialized ) return;

	if ( hAckEvent ) {
		DestroyHookableEvent(hAckEvent);
		hAckEvent = NULL;
	}
	if ( hAccListChanged ) {
		DestroyHookableEvent(hAccListChanged);
		hAccListChanged = NULL;
	}

	if ( protos.count ) {
		for( i=0; i < protos.count; i++ ) {
			mir_free( protos.items[i]->szName);
			mir_free( protos.items[i] );
		}
		List_Destroy(( SortedList* )&protos );
	}

	for ( i=0; i < serviceItems.count; i++ )
		mir_free( serviceItems.items[i] );
	List_Destroy(( SortedList* )&serviceItems );
}

/////////////////////////////////////////////////////////////////////////////////////////

void UninitAccount( PROTOACCOUNT* pa )
{
	int idx;	
	PROTOCOLDESCRIPTOR temp;
	temp.szName = pa->szProtoName;
	if ( List_GetIndex(( SortedList* )&protos, &temp, &idx ))
		if ( protos.items[idx]->fnUninit != NULL )
			protos.items[idx]->fnUninit( pa->ppro );
}
