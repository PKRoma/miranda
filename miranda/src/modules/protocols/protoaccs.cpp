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

static BOOL bModuleInitialized = FALSE;

static int CompareAccounts( const PROTOACCOUNT* p1, const PROTOACCOUNT* p2 )
{
	return strcmp( p1->szModuleName, p2->szModuleName );
}

LIST<PROTOACCOUNT> accounts( 10, CompareAccounts );

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
			PROTOACCOUNT* pa = ( PROTOACCOUNT* )mir_calloc( sizeof( PROTOACCOUNT ));
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

				accounts.insert( pa );
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
	for ( i=0; i < accounts.getCount(); i++ ) {
		PROTOACCOUNT* pa = accounts[i];

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
	DBWriteContactSettingDword( 0, "Protocols", "ProtoCount", accounts.getCount() );
	DBWriteContactSettingDword( 0, "Protocols", "PrVer", 4 );
}

/////////////////////////////////////////////////////////////////////////////////////////

static int InitializeStaticAccounts( WPARAM wParam, LPARAM lParam )
{
	int count = 0;

	for ( int i = 0; i < accounts.getCount(); i++ ) {
		PROTOACCOUNT* pa = accounts[i];
		if ( !pa->ppro || !IsAccountEnabled( pa ))
			continue;

		pa->ppro->OnEvent( EV_PROTO_ONLOAD, 0, 0 );

		if ( !pa->bOldProto )
			count++;
	}

	if ( count == 0 )
		CallService( MS_PROTO_SHOWACCMGR, 0, 0 );
	return 0;
}

static int UninitializeStaticAccounts( WPARAM wParam, LPARAM lParam )
{
	for ( int i = 0; i < accounts.getCount(); i++ ) {
		PROTOACCOUNT* pa = accounts[i];
		if ( !pa->ppro || !IsAccountEnabled( pa ))
			continue;

		pa->ppro->OnEvent( EV_PROTO_ONREADYTOEXIT, 0, 0 );
		pa->ppro->OnEvent( EV_PROTO_ONEXIT, 0, 0 );
	}
	return 0;
}

int LoadAccountsModule( void )
{
	int i;

	bModuleInitialized = TRUE;

	for ( i = 0; i < accounts.getCount(); i++ ) {
		PROTOACCOUNT* pa = accounts[i];
		if ( pa->ppro || !IsAccountEnabled( pa ))
			continue;

		if ( !ActivateAccount( pa )) { // remove damaged account from list
			UnloadAccount( pa, FALSE );
			accounts.remove( i-- );
	}	}

	HookEvent( ME_SYSTEM_MODULESLOADED, InitializeStaticAccounts );
	HookEvent( ME_SYSTEM_PRESHUTDOWN, UninitializeStaticAccounts );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int stub1( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->AddToList( wParam, (PROTOSEARCHRESULT*)lParam );
}

static int stub2( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->AddToListByEvent( HIWORD(wParam), LOWORD(wParam), (HANDLE)lParam );
}

static int stub3( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->Authorize( ( HANDLE )wParam );
}

static int stub4( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->AuthDeny( ( HANDLE )wParam, ( const char* )lParam );
}

static int stub7( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->ChangeInfo( wParam, ( void* )lParam );
}

static int stub11( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	PROTOFILERESUME* pfr = ( PROTOFILERESUME* )lParam;
	return ( int )ppi->FileResume( ( HANDLE )wParam, &pfr->action, &pfr->szFilename );
}

static int stub12( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->GetCaps( wParam, (HANDLE)lParam );
}

static int stub13( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->GetIcon( wParam );
}

static int stub15( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->SearchBasic( ( char* )lParam );
}

static int stub16( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->SearchByEmail( ( char* )lParam );
}

static int stub17( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	PROTOSEARCHBYNAME* psbn = ( PROTOSEARCHBYNAME* )lParam;
	return ( int )ppi->SearchByName( psbn->pszNick, psbn->pszFirstName, psbn->pszLastName );
}

static int stub18( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->SearchAdvanced( ( HWND )lParam );
}

static int stub19( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->CreateExtendedSearchUI ( ( HWND )lParam );
}

static int stub29( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->SetStatus( wParam );
}

static int stub33( PROTO_INTERFACE* ppi, WPARAM wParam, LPARAM lParam )
{	return ( int )ppi->SetAwayMsg( wParam, ( const char* )lParam );
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
	if ( ppd == NULL ) {
		pa->bDynDisabled = TRUE;
		return FALSE;
	}

	if ( ppd->fnInit == NULL ) {
		pa->bDynDisabled = TRUE;
		return FALSE;
	}

	ppi = ppd->fnInit( pa->szModuleName, pa->tszAccountName );
	if ( ppi == NULL ) {
		pa->bDynDisabled = TRUE;
		return FALSE;
	}

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

/////////////////////////////////////////////////////////////////////////////////////////

struct DeactivationThreadParam
{
	tagPROTO_INTERFACE* ppro;
	pfnUninitProto      fnUninit;
	BOOL                bIsDynamic;
};

pfnUninitProto GetProtocolDestructor( char* szProto );

static int DeactivationThread( DeactivationThreadParam* param )
{
	tagPROTO_INTERFACE* p = ( tagPROTO_INTERFACE* )param->ppro;
	p->SetStatus(ID_STATUS_OFFLINE);

	if ( param->bIsDynamic ) {
		p->OnEvent( EV_PROTO_ONREADYTOEXIT, 0, 0 );
		p->OnEvent( EV_PROTO_ONEXIT, 0, 0 );
	}

	KillObjectThreads( p ); // waits for them before terminating

	if ( param->fnUninit )
		param->fnUninit( p );

	KillObjectServices( p );
	KillObjectEventHooks( p );
	delete param;
	return 0;
}

void DeactivateAccount( PROTOACCOUNT* pa, BOOL bIsDynamic )
{
	if ( !pa->ppro )
		return;

	if ( pa->hwndAccMgrUI ) {
		DestroyWindow(pa->hwndAccMgrUI);
		pa->hwndAccMgrUI = NULL;
		pa->bAccMgrUIChanged = FALSE;
	}

	DeactivationThreadParam* param = new DeactivationThreadParam;
	param->ppro = pa->ppro;
	param->fnUninit = GetProtocolDestructor( pa->szProtoName );
	param->bIsDynamic = bIsDynamic;
	pa->ppro = NULL;
	if ( bIsDynamic )
		mir_forkthread(( pThreadFunc )DeactivationThread, param );
	else 
		DeactivationThread( param );
}

/////////////////////////////////////////////////////////////////////////////////////////

void EraseAccount( PROTOACCOUNT* pa )
{
}

/////////////////////////////////////////////////////////////////////////////////////////

int IsAccountEnabled( PROTOACCOUNT* pa )
{
	if ( !pa )
		return FALSE;

	return ( pa->bIsEnabled && !pa->bDynDisabled );
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

	for( i=accounts.getCount()-1; i >= 0; i-- ) {
		PROTOACCOUNT* pa = accounts[ i ];
		UnloadAccount( pa, FALSE );
		accounts.remove(i);
	}

	accounts.destroy();
}
