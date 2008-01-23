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

int Proto_IsProtocolLoaded(WPARAM wParam,LPARAM lParam);
void UninitAccount(char *szModule, PROTOACCOUNT *pa);

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

static int stub1( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->AddToList( ppi, wParam, (PROTOSEARCHRESULT*)lParam );
}

static int stub2( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->AddToListByEvent( ppi, HIWORD(wParam), LOWORD(wParam), (HANDLE)lParam );
}

static int stub3( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->Authorize( ppi, ( HANDLE )wParam );
}

static int stub4( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->AuthDeny( ppi, ( HANDLE )wParam, ( const char* )lParam );
}

static int stub7( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->ChangeInfo( ppi, wParam, ( void* )lParam );
}

static int stub11( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	PROTOFILERESUME* pfr = ( PROTOFILERESUME* )lParam;
	return ( int )ppi->vtbl->FileResume( ppi, ( HANDLE )wParam, &pfr->action, &pfr->szFilename );
}

static int stub12( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->GetCaps( ppi, wParam );
}

static int stub13( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->GetIcon( ppi, wParam );
}

static int stub15( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->SearchBasic( ppi, ( char* )lParam );
}

static int stub16( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->SearchByEmail( ppi, ( char* )lParam );
}

static int stub17( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	PROTOSEARCHBYNAME* psbn = ( PROTOSEARCHBYNAME* )lParam;
	return ( int )ppi->vtbl->SearchByName( ppi, psbn->pszNick, psbn->pszFirstName, psbn->pszLastName );
}

static int stub18( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->SearchAdvanced( ppi, ( HWND )lParam );
}

static int stub19( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->CreateExtendedSearchUI ( ppi, ( HWND )lParam );
}

static int stub29( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->SetStatus( ppi, wParam );
}

static int stub33( WPARAM wParam, LPARAM lParam, PROTO_INTERFACE* ppi )
{	return ( int )ppi->vtbl->SetAwayMsg( ppi, wParam, ( const char* )lParam );
}

static HANDLE CreateProtoServiceEx( const char* szModule, const char* szService, MIRANDASERVICEPARAM pFunc, void* param )
{
	char tmp[100];
	mir_snprintf( tmp, sizeof( tmp ), "%s%s", szModule, szService );
	return CreateServiceFunctionParam( tmp, pFunc, ( LPARAM )param );
}

int LoadAccountsModule( void )
{
	int i;

	bModuleInitialized = TRUE;

	for ( i = 0; i < accounts.count; i++ ) {
		PROTOCOLDESCRIPTOR* ppd;
		PROTOACCOUNT* pa = accounts.items[i];
		PROTO_INTERFACE* ppi;
		if ( pa->ppro != NULL )
			continue;

		if (( ppd = ( PROTOCOLDESCRIPTOR* )Proto_IsProtocolLoaded( 0, ( LPARAM )pa->szProtoName )) == NULL ) {
			pa->bIsEnabled = FALSE;
			continue;
		}

		if ( ppd->fnInit == NULL ) {
			pa->bIsEnabled = FALSE;
			continue;
		}

		ppi = ppd->fnInit( pa->szModuleName, pa->tszAccountName );
		if ( ppi != NULL ) {
			pa->ppro = ppi;
			ppi->services[0] = CreateProtoServiceEx( pa->szModuleName, PS_ADDTOLIST, (MIRANDASERVICEPARAM)stub1, pa->ppro );
			ppi->services[1] = CreateProtoServiceEx( pa->szModuleName, PS_ADDTOLISTBYEVENT, (MIRANDASERVICEPARAM)stub2, pa->ppro );
			ppi->services[2] = CreateProtoServiceEx( pa->szModuleName, PS_AUTHALLOW, (MIRANDASERVICEPARAM)stub3, pa->ppro );
			ppi->services[3] = CreateProtoServiceEx( pa->szModuleName, PS_AUTHDENY, (MIRANDASERVICEPARAM)stub4, pa->ppro );
			ppi->services[4] = CreateProtoServiceEx( pa->szModuleName, PS_CHANGEINFO, (MIRANDASERVICEPARAM)stub7, pa->ppro );
			ppi->services[5] = CreateProtoServiceEx( pa->szModuleName, PS_FILERESUME, (MIRANDASERVICEPARAM)stub11, pa->ppro );
			ppi->services[6] = CreateProtoServiceEx( pa->szModuleName, PS_GETCAPS, (MIRANDASERVICEPARAM)stub12, pa->ppro );
			ppi->services[7] = CreateProtoServiceEx( pa->szModuleName, PS_LOADICON, (MIRANDASERVICEPARAM)stub13, pa->ppro );
			ppi->services[8] = CreateProtoServiceEx( pa->szModuleName, PS_BASICSEARCH, (MIRANDASERVICEPARAM)stub15, pa->ppro );
			ppi->services[9] = CreateProtoServiceEx( pa->szModuleName, PS_SEARCHBYEMAIL, (MIRANDASERVICEPARAM)stub16, pa->ppro );
			ppi->services[10] = CreateProtoServiceEx( pa->szModuleName, PS_SEARCHBYNAME, (MIRANDASERVICEPARAM)stub17, pa->ppro );
			ppi->services[11] = CreateProtoServiceEx( pa->szModuleName, PS_SEARCHBYADVANCED, (MIRANDASERVICEPARAM)stub18, pa->ppro );
			ppi->services[12] = CreateProtoServiceEx( pa->szModuleName, PS_CREATEADVSEARCHUI, (MIRANDASERVICEPARAM)stub19, pa->ppro );
			ppi->services[13] = CreateProtoServiceEx( pa->szModuleName, PS_SETSTATUS, (MIRANDASERVICEPARAM)stub29, pa->ppro );
			ppi->services[14] = CreateProtoServiceEx( pa->szModuleName, PS_SETAWAYMSG, (MIRANDASERVICEPARAM)stub33, pa->ppro );
		}
		else pa->bIsEnabled = FALSE;
	}

	return 0;
}

void UnloadAccountsModule()
{
	int i;

	if ( !bModuleInitialized ) return;

	for( i=0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[ i ];
		int idx;
		if ( pa->ppro ) {
			for ( idx = 0; idx < SIZEOF(pa->ppro->services); idx++ )
				if ( pa->ppro->services[i] )
					DestroyServiceFunction( pa->ppro->services[i] );

			UninitAccount(accounts.items[i]->szModuleName, pa);
		}
		mir_free( pa->tszAccountName );
		mir_free( pa->szModuleName );
		mir_free( pa->szProtoName );
		mir_free( pa );
	}
	List_Destroy(( SortedList* )&accounts );
}
