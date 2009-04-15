/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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
int LoadProtoOptions( void );
int isProtoSuitable( PROTO_INTERFACE* ppi );

HANDLE hAccListChanged;
static HANDLE hAckEvent,hTypeEvent;
static BOOL bModuleInitialized = FALSE;

typedef struct
{
	int id;
	const char* name;
}
	TServiceListItem;

static int CompareServiceItems( const TServiceListItem* p1, const TServiceListItem* p2 )
{	return strcmp( p1->name, p2->name );
}

static LIST<TServiceListItem> serviceItems( 10, CompareServiceItems );

//------------------------------------------------------------------------------------

static int CompareProtos( const PROTOCOLDESCRIPTOR* p1, const PROTOCOLDESCRIPTOR* p2 )
{	return strcmp( p1->szName, p2->szName );
}

static LIST<PROTOCOLDESCRIPTOR> protos( 10, CompareProtos );

static INT_PTR Proto_BroadcastAck(WPARAM wParam,LPARAM lParam)
{
	return NotifyEventHooks(hAckEvent,wParam,lParam);
}

PROTOCOLDESCRIPTOR* Proto_IsProtocolLoaded( const char* szProtoName )
{
	if ( szProtoName ) {
		int idx;
		PROTOCOLDESCRIPTOR tmp;
		tmp.szName = ( char* )szProtoName;
		if (( idx = protos.getIndex( &tmp )) != -1 )
			return protos[idx];
	}

	return NULL;
}

INT_PTR srvProto_IsLoaded(WPARAM, LPARAM lParam)
{
	return (INT_PTR)Proto_GetAccount(( char* )lParam );
}

INT_PTR Proto_EnumProtocols(WPARAM wParam,LPARAM lParam)
{
	*( int* )wParam = protos.getCount();
	*( PROTOCOLDESCRIPTOR*** )lParam = protos.getArray();
	return 0;
}

static PROTO_INTERFACE* defInitProto( const char* szModuleName, const TCHAR* )
{
	return AddDefaultAccount( szModuleName );
}

static INT_PTR Proto_RegisterModule(WPARAM, LPARAM lParam)
{
	PROTOCOLDESCRIPTOR* pd = ( PROTOCOLDESCRIPTOR* )lParam, *p;
	if ( pd->cbSize != sizeof( PROTOCOLDESCRIPTOR ) && pd->cbSize != PROTOCOLDESCRIPTOR_V3_SIZE )
		return 1;

	p = ( PROTOCOLDESCRIPTOR* )mir_alloc( sizeof( PROTOCOLDESCRIPTOR ));
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
					pa->bIsVisible = isProtoSuitable(ppi);
                    pa->bIsEnabled = TRUE;
					pa->iOrder = accounts.getCount();
					accounts.insert( pa );
				}
				pa->bOldProto = TRUE;
				pa->ppro = ppi;
				p->fnInit = defInitProto;
				p->fnUninit = FreeDefaultAccount;
			}
		}
	}
	else *p = *pd;
	p->szName = mir_strdup( pd->szName );
	protos.insert( p );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Basic core services

static INT_PTR Proto_RecvFile(WPARAM,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT* pre = ( PROTORECVEVENT* )ccs->lParam;
	char* szFile = pre->szMessage + sizeof( DWORD );
	char* szDescr = szFile + strlen( szFile ) + 1;

	// Suppress the standard event filter
	if ( pre->lParam != NULL )
		*( DWORD* )pre->szMessage = 0;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = ( char* )CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ccs->hContact, 0);
	dbei.timestamp = pre->timestamp;
	dbei.flags = ( pre->flags & PREF_CREATEREAD ) ? DBEF_READ : 0;
	dbei.eventType = EVENTTYPE_FILE;
	dbei.cbBlob = (DWORD)(sizeof( DWORD ) + strlen( szFile ) + strlen( szDescr ) + 2);
	dbei.pBlob = ( PBYTE )pre->szMessage;
	HANDLE hdbe = ( HANDLE )CallService( MS_DB_EVENT_ADD, ( WPARAM )ccs->hContact, ( LPARAM )&dbei );

	if ( pre->lParam != NULL )
		PushFileEvent( ccs->hContact, hdbe, pre->lParam );
	return 0;
}

static INT_PTR Proto_RecvMessage(WPARAM,LPARAM lParam)
{
	CCSDATA *ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT *pre = ( PROTORECVEVENT* )ccs->lParam;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = ( char* )CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ccs->hContact, 0);
	dbei.timestamp = pre->timestamp;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.cbBlob = (DWORD)strlen( pre->szMessage ) + 1;
	if ( pre->flags & PREF_CREATEREAD )
		dbei.flags |= DBEF_READ;
	if ( pre->flags & PREF_UTF )
		dbei.flags |= DBEF_UTF;
	if ( pre->flags & PREF_UNICODE )
		dbei.cbBlob += sizeof( wchar_t )*( (DWORD)wcslen(( wchar_t* )&pre->szMessage[dbei.cbBlob+1] )+1 );

	dbei.pBlob = ( PBYTE ) pre->szMessage;
	return CallService( MS_DB_EVENT_ADD, ( WPARAM ) ccs->hContact, ( LPARAM )&dbei );
}

/////////////////////////////////////////////////////////////////////////////////////////
// User Typing Notification services

static int Proto_ValidTypingContact(HANDLE hContact, char *szProto)
{
	if ( !hContact || !szProto )
		return 0;

	return ( CallProtoService(szProto,PS_GETCAPS,PFLAGNUM_4,0) & PF4_SUPPORTTYPING ) ? 1 : 0;
}

static INT_PTR Proto_SelfIsTyping(WPARAM wParam,LPARAM lParam)
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

static INT_PTR Proto_ContactIsTyping(WPARAM wParam,LPARAM lParam)
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
	if (( idx = accounts.getIndex( &temp )) == -1 )
		return NULL;

	return accounts[idx];
}

static INT_PTR srvProto_GetAccount(WPARAM, LPARAM lParam)
{
	return ( INT_PTR )Proto_GetAccount(( char* )lParam );
}

static INT_PTR Proto_EnumAccounts(WPARAM wParam, LPARAM lParam)
{
	*( int* )wParam = accounts.getCount();
	*( PROTOACCOUNT*** )lParam = accounts.getArray();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CallProtoServiceInt( HANDLE hContact, const char *szModule, const char *szService, WPARAM wParam, LPARAM lParam )
{
	int idx;
	char svcName[ MAXMODULELABELLENGTH ];
	PROTOACCOUNT* pa = ( PROTOACCOUNT* )Proto_GetAccount( szModule );
	if ( pa && !pa->bOldProto ) {
		PROTO_INTERFACE* ppi;
		if (( ppi = pa->ppro ) == NULL )
			return CALLSERVICE_NOTFOUND;
		else {
			TServiceListItem item;
			item.name = szService;
			if (( idx = serviceItems.getIndex( &item )) != -1 ) {
				switch( serviceItems[ idx ]->id ) {
					case  1: return ( INT_PTR )ppi->AddToList( wParam, (PROTOSEARCHRESULT*)lParam ); break;
					case  2: return ( INT_PTR )ppi->AddToListByEvent( LOWORD(wParam), HIWORD(wParam), (HANDLE)lParam ); break;
					case  3: return ( INT_PTR )ppi->Authorize( ( HANDLE )wParam ); break;
					case  4: return ( INT_PTR )ppi->AuthDeny( ( HANDLE )wParam, ( const char* )lParam ); break;
					case  5: return ( INT_PTR )ppi->AuthRecv( hContact, ( PROTORECVEVENT* )lParam ); break;
					case  6: return ( INT_PTR )ppi->AuthRequest( hContact, ( char* )lParam ); break;
					case  7: return ( INT_PTR )ppi->ChangeInfo( wParam, ( void* )lParam ); break;
					case  8: return ( INT_PTR )ppi->FileAllow( hContact, ( HANDLE )wParam, ( char* )lParam ); break;
					case  9: return ( INT_PTR )ppi->FileCancel( hContact, ( HANDLE )wParam ); break;
					case 10: return ( INT_PTR )ppi->FileDeny( hContact, ( HANDLE )wParam, ( char* )lParam ); break;
					case 11: {
						PROTOFILERESUME* pfr = ( PROTOFILERESUME* )lParam;
						return ( INT_PTR )ppi->FileResume( ( HANDLE )wParam, &pfr->action, &pfr->szFilename ); break;
					}
					case 12: return ( INT_PTR )ppi->GetCaps( wParam, (HANDLE)lParam ); break;
					case 13: return ( INT_PTR )ppi->GetIcon( wParam ); break;
					case 14: return ( INT_PTR )ppi->GetInfo( hContact, wParam ); break;
					case 15: return ( INT_PTR )ppi->SearchBasic( ( char* )lParam ); break;
					case 16: return ( INT_PTR )ppi->SearchByEmail( ( char* )lParam ); break;
					case 17: {
						PROTOSEARCHBYNAME* psbn = ( PROTOSEARCHBYNAME* )lParam;
						return ( INT_PTR )ppi->SearchByName( psbn->pszNick, psbn->pszFirstName, psbn->pszLastName ); break;
					}
					case 18: return ( INT_PTR )ppi->SearchAdvanced( ( HWND )lParam ); break;
					case 19: return ( INT_PTR )ppi->CreateExtendedSearchUI ( ( HWND )lParam ); break;
					case 20: return ( INT_PTR )ppi->RecvContacts( hContact, ( PROTORECVEVENT* )lParam ); break;
					case 21: return ( INT_PTR )ppi->RecvFile( hContact, ( PROTORECVFILE* )lParam ); break;
					case 22: return ( INT_PTR )ppi->RecvMsg( hContact, ( PROTORECVEVENT* )lParam ); break;
					case 23: return ( INT_PTR )ppi->RecvUrl( hContact, ( PROTORECVEVENT* )lParam ); break;
					case 24: return ( INT_PTR )ppi->SendContacts( hContact, LOWORD(wParam), HIWORD(wParam), ( HANDLE* )lParam ); break;
					case 25: return ( INT_PTR )ppi->SendFile( hContact, ( const char* )wParam, ( char** )lParam ); break;
					case 26: return ( INT_PTR )ppi->SendMsg( hContact, wParam, ( const char* )lParam ); break;
					case 27: return ( INT_PTR )ppi->SendUrl( hContact, wParam, ( const char* )lParam ); break;
					case 28: return ( INT_PTR )ppi->SetApparentMode( hContact, wParam ); break;
					case 29: return ( INT_PTR )ppi->SetStatus( wParam ); break;
					case 30: return ( INT_PTR )ppi->GetAwayMsg( hContact ); break;
					case 31: return ( INT_PTR )ppi->RecvAwayMsg( hContact, wParam, ( PROTORECVEVENT* )lParam ); break;
					case 32: return ( INT_PTR )ppi->SendAwayMsg( hContact, ( HANDLE )wParam, ( const char* )lParam ); break;
					case 33: return ( INT_PTR )ppi->SetAwayMsg( wParam, ( const char* )lParam ); break;
					case 34: return ( INT_PTR )ppi->UserIsTyping( ( HANDLE )wParam, lParam ); break;
	}	}	}	}

	mir_snprintf( svcName, sizeof(svcName), "%s%s", szModule, szService );
	return CallService( svcName, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CallContactService( HANDLE hContact, const char *szProtoService, WPARAM wParam, LPARAM lParam )
{
	int i;
	DBVARIANT dbv;
	INT_PTR ret;
	PROTOACCOUNT* pa;
	CCSDATA ccs = { hContact, szProtoService, wParam, lParam };

	for ( i = 0;; i++ ) {
		char str[10];
		_itoa( i, str, 10 );
		if ( DBGetContactSettingString( hContact, "_Filter", str, &dbv ))
			break;

		if (( ret = CallProtoServiceInt( hContact, dbv.pszVal, szProtoService, i+1, ( LPARAM)&ccs )) != CALLSERVICE_NOTFOUND ) {
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

static void InsertServiceListItem( int id, const char* szName )
{
	TServiceListItem* p = ( TServiceListItem* )mir_alloc( sizeof( TServiceListItem ));
	p->id = id;
	p->name = szName;
	serviceItems.insert( p );
}

int LoadProtocolsModule(void)
{
	bModuleInitialized = TRUE;

	if ( LoadProtoChains() )
		return 1;

	InsertServiceListItem(  1, PS_ADDTOLIST );
	InsertServiceListItem(  2, PS_ADDTOLISTBYEVENT );
	InsertServiceListItem(  3, PS_AUTHALLOW );
	InsertServiceListItem(  4, PS_AUTHDENY );
	InsertServiceListItem(  5, PSR_AUTH );
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

	if ( protos.getCount() ) {
		for( i=0; i < protos.getCount(); i++ ) {
			mir_free( protos[i]->szName);
			mir_free( protos[i] );
		}
		protos.destroy();
	}

	for ( i=0; i < serviceItems.getCount(); i++ )
		mir_free( serviceItems[i] );
	serviceItems.destroy();
}

/////////////////////////////////////////////////////////////////////////////////////////

pfnUninitProto GetProtocolDestructor( char* szProto )
{
	int idx;	
	PROTOCOLDESCRIPTOR temp;
	temp.szName = szProto;
	if (( idx = protos.getIndex( &temp )) != -1 )
		return protos[idx]->fnUninit;

	return NULL;
}
