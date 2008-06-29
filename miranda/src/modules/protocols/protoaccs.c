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

void UninitAccount( PROTOACCOUNT* pa );

static BOOL bModuleInitialized = FALSE;

TAccounts accounts;

/////////////////////////////////////////////////////////////////////////////////////////

void LoadDbAccounts()
{
	DBVARIANT dbv;
	int ver = DBGetContactSettingDword( NULL, "Protocols", "PrVer", -1 );
	int count = DBGetContactSettingDword( NULL, "Protocols", "ProtoCount", 0 ), i;

	for ( i=0; i < count; i++ ) {
		char buf[10];
		_itoa( i, buf, 10 );
		if ( !DBGetContactSettingString( NULL, "Protocols", buf, &dbv )) {
			PROTOACCOUNT* pa = mir_calloc( sizeof( PROTOACCOUNT ));
			if ( pa ) {
				pa->cbSize = sizeof( *pa );
				pa->type = PROTOTYPE_PROTOCOL;
				pa->szModuleName = mir_strdup( dbv.pszVal );
				DBFreeVariant( &dbv );

				_itoa( OFFSET_VISIBLE+i, buf, 10 );
				pa->bIsVisible = DBGetContactSettingDword( NULL, "Protocols", buf, 1 );

				_itoa( OFFSET_PROTOPOS+i, buf, 10 );
				pa->iOrder = DBGetContactSettingDword( NULL, "Protocols", buf, 1 );

				if ( ver >= 4 ) {
					DBFreeVariant( &dbv );
					_itoa( OFFSET_NAME+i, buf, 10 );
					if ( !DBGetContactSettingTString( NULL, "Protocols", buf, &dbv )) {
						pa->tszAccountName = mir_tstrdup( dbv.ptszVal );
						DBFreeVariant( &dbv );
					}

					_itoa( OFFSET_ENABLED+i, buf, 10 );
					pa->bIsEnabled = DBGetContactSettingDword( NULL, "Protocols", buf, 1 );

					if ( !DBGetContactSettingString( NULL, pa->szModuleName, "AM_BaseProto", &dbv )) {
						pa->szProtoName = mir_strdup( dbv.pszVal );
						DBFreeVariant( &dbv );
					}
				}
				else pa->bIsEnabled = TRUE;

				if ( !pa->szProtoName ) {
					pa->szProtoName = mir_strdup( pa->szModuleName );
					DBWriteContactSettingString( NULL, pa->szModuleName, "AM_BaseProto", pa->szProtoName );
				}

				if ( !pa->tszAccountName )
					pa->tszAccountName = mir_a2t( pa->szModuleName );

				List_InsertPtr(( SortedList* )&accounts, pa );
			}
			else DBFreeVariant( &dbv );
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	int  arrlen;
	char **pszSettingName;
}
	enumDB_ProtoProcParam;

static int enumDB_ProtoProc( const char* szSetting, LPARAM lParam )
{
	if ( szSetting ) {
		enumDB_ProtoProcParam* p = ( enumDB_ProtoProcParam* )lParam;

		p->arrlen++;
		p->pszSettingName = ( char** )mir_realloc( p->pszSettingName, p->arrlen*sizeof( char* ));
		p->pszSettingName[ p->arrlen-1 ] = mir_strdup( szSetting );
	}
	return 0;
}

void WriteDbAccounts()
{
	int i;

	// enum all old settings to delete
	enumDB_ProtoProcParam param = { 0, NULL };

	DBCONTACTENUMSETTINGS dbces;
	dbces.pfnEnumProc = enumDB_ProtoProc;
	dbces.szModule = "Protocols";
	dbces.ofsSettings = 0;
	dbces.lParam = ( LPARAM )&param;
	CallService( MS_DB_CONTACT_ENUMSETTINGS, 0, ( LPARAM )&dbces );

	// delete all settings
	if ( param.arrlen ) {
		int i;
		for ( i=0; i < param.arrlen; i++ ) {
			DBDeleteContactSetting( 0, "Protocols", param.pszSettingName[i] );
			mir_free( param.pszSettingName[i] );
		}
		mir_free( param.pszSettingName );
	}

	// write new data
	for ( i=0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[i];

		char buf[ 20 ];
		_itoa( i, buf, 10 );
		DBWriteContactSettingString( NULL, "Protocols", buf, pa->szModuleName );

		_itoa( OFFSET_PROTOPOS+i, buf, 10 );
		DBWriteContactSettingDword( NULL, "Protocols", buf, pa->iOrder );

		_itoa( OFFSET_VISIBLE+i, buf, 10 );
		DBWriteContactSettingDword( NULL, "Protocols", buf, pa->bIsVisible );

		_itoa( OFFSET_ENABLED+i, buf, 10 );
		DBWriteContactSettingDword( NULL, "Protocols", buf, pa->bIsEnabled );

		_itoa( OFFSET_NAME+i, buf, 10 );
		DBWriteContactSettingTString( NULL, "Protocols", buf, pa->tszAccountName );
	}

	DBDeleteContactSetting( 0, "Protocols", "ProtoCount" );
	DBWriteContactSettingDword( 0, "Protocols", "ProtoCount", accounts.count );
	DBWriteContactSettingDword( 0, "Protocols", "PrVer", 4 );
}

/////////////////////////////////////////////////////////////////////////////////////////

static int InitializeStaticAccounts( WPARAM wParam, LPARAM lParam )
{
	int i;

	for ( i = 0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[i];
		if ( !pa->ppro || !pa->bIsEnabled )
			continue;

		pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONLOAD, 0, 0 );
	}
	return 0;
}

static int UninitializeStaticAccounts( WPARAM wParam, LPARAM lParam )
{
	int i;

	for ( i = 0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[i];
		if ( !pa->ppro || !pa->bIsEnabled )
			continue;

		pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONREADYTOEXIT, 0, 0 );
		pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONEXIT, 0, 0 );
	}
	return 0;
}

int LoadAccountsModule( void )
{
	int i;

	bModuleInitialized = TRUE;

	for ( i = 0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[i];
		if ( pa->ppro || !pa->bIsEnabled )
			continue;

		if ( !ActivateAccount( pa )) { // remove damaged account from list
			List_Remove(( SortedList* )&accounts, i-- );
			UnloadAccount( pa, FALSE );
	}	}

	HookEvent( ME_SYSTEM_MODULESLOADED, InitializeStaticAccounts );
	HookEvent( ME_SYSTEM_PRESHUTDOWN, UninitializeStaticAccounts );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int stub1( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->AddToList( ppi, wParam, (PROTOSEARCHRESULT*)lParam );
}

static int stub2( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->AddToListByEvent( ppi, HIWORD(wParam), LOWORD(wParam), (HANDLE)lParam );
}

static int stub3( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->Authorize( ppi, ( HANDLE )wParam );
}

static int stub4( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->AuthDeny( ppi, ( HANDLE )wParam, ( const char* )lParam );
}

static int stub7( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->ChangeInfo( ppi, wParam, ( void* )lParam );
}

static int stub11( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	PROTOFILERESUME* pfr = ( PROTOFILERESUME* )lParam;
	return ( int )ppi->vtbl->FileResume( ppi, ( HANDLE )wParam, &pfr->action, &pfr->szFilename );
}

static int stub12( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->GetCaps( ppi, wParam, (HANDLE)lParam );
}

static int stub13( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->GetIcon( ppi, wParam );
}

static int stub15( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->SearchBasic( ppi, ( char* )lParam );
}

static int stub16( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->SearchByEmail( ppi, ( char* )lParam );
}

static int stub17( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	PROTOSEARCHBYNAME* psbn = ( PROTOSEARCHBYNAME* )lParam;
	return ( int )ppi->vtbl->SearchByName( ppi, psbn->pszNick, psbn->pszFirstName, psbn->pszLastName );
}

static int stub18( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->SearchAdvanced( ppi, ( HWND )lParam );
}

static int stub19( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->CreateExtendedSearchUI ( ppi, ( HWND )lParam );
}

static int stub29( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->SetStatus( ppi, wParam );
}

static int stub33( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->vtbl->SetAwayMsg( ppi, wParam, ( const char* )lParam );
}

static HANDLE CreateProtoServiceEx( const char* szModule, const char* szService, MIRANDASERVICEOBJ pFunc, void* param )
{
	char tmp[100];
	mir_snprintf( tmp, sizeof( tmp ), "%s%s", szModule, szService );
	return CreateServiceFunctionObj( tmp, pFunc, param );
}

BOOL ActivateAccount( PROTOACCOUNT* pa )
{
	PROTO_INTERFACE* ppi;
	PROTOCOLDESCRIPTOR* ppd = Proto_IsProtocolLoaded( pa->szProtoName );
	if ( ppd == NULL )
		return pa->bIsEnabled = FALSE;

	if ( ppd->fnInit == NULL )
		return pa->bIsEnabled = FALSE;

	ppi = ppd->fnInit( pa->szModuleName, pa->tszAccountName );
	if ( ppi != NULL ) {
		pa->ppro = ppi;
		ppi->m_iDesiredStatus = ppi->m_iStatus = ID_STATUS_OFFLINE;
		CreateProtoServiceEx( pa->szModuleName, PS_ADDTOLIST, (MIRANDASERVICEOBJ)stub1, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_ADDTOLISTBYEVENT, (MIRANDASERVICEOBJ)stub2, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_AUTHALLOW, (MIRANDASERVICEOBJ)stub3, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_AUTHDENY, (MIRANDASERVICEOBJ)stub4, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_CHANGEINFO, (MIRANDASERVICEOBJ)stub7, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_FILERESUME, (MIRANDASERVICEOBJ)stub11, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_GETCAPS, (MIRANDASERVICEOBJ)stub12, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_LOADICON, (MIRANDASERVICEOBJ)stub13, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_BASICSEARCH, (MIRANDASERVICEOBJ)stub15, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_SEARCHBYEMAIL, (MIRANDASERVICEOBJ)stub16, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_SEARCHBYNAME, (MIRANDASERVICEOBJ)stub17, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_SEARCHBYADVANCED, (MIRANDASERVICEOBJ)stub18, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_CREATEADVSEARCHUI, (MIRANDASERVICEOBJ)stub19, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_SETSTATUS, (MIRANDASERVICEOBJ)stub29, pa->ppro );
		CreateProtoServiceEx( pa->szModuleName, PS_SETAWAYMSG, (MIRANDASERVICEOBJ)stub33, pa->ppro );
		return TRUE;
	}

	return pa->bIsEnabled = FALSE;
}

void DeactivateAccount( PROTOACCOUNT* pa, BOOL bIsDynamic )
{
	if ( !pa->ppro )
		return;

	if ( pa->hwndAccMgrUI )
	{
		DestroyWindow(pa->hwndAccMgrUI);
		pa->hwndAccMgrUI = NULL;
		pa->bAccMgrUIChanged = FALSE;
	}

	KillObjectServices( pa->ppro );
	KillObjectEventHooks( pa->ppro );
	KillObjectThreads( pa->ppro );

	if ( bIsDynamic ) {
		pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONREADYTOEXIT, 0, 0 );
		pa->ppro->vtbl->OnEvent( pa->ppro, EV_PROTO_ONEXIT, 0, 0 );
	}

	UninitAccount( pa );
	pa->ppro = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////

void EraseAccount( PROTOACCOUNT* pa )
{
}

/////////////////////////////////////////////////////////////////////////////////////////

void UnloadAccount( PROTOACCOUNT* pa, BOOL bIsDynamic )
{
	DeactivateAccount( pa, bIsDynamic );

	mir_free( pa->tszAccountName );
	mir_free( pa->szProtoName );
	// szModuleName should be freed only on a program's exit.
	// otherwise many plugins dependand on static protocol names will crash!
	// do NOT fix this 'leak', please
	if ( !bIsDynamic ) {
		mir_free( pa->szModuleName );
		mir_free( pa );
	}
}

void UnloadAccountsModule()
{
	int i;

	if ( !bModuleInitialized ) return;

	for( i=accounts.count-1; i >= 0; i--, accounts.count-- ) {
		PROTOACCOUNT* pa = accounts.items[ i ];
		UnloadAccount( pa, FALSE );
	}

	List_Destroy(( SortedList* )&accounts );
}
