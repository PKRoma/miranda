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
			PROTOACCOUNT* pa = mir_alloc( sizeof( PROTOACCOUNT ));
			if ( pa ) {
				memset( pa, 0, sizeof( PROTOACCOUNT ));
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

int LoadAccountsModule( void )
{
	int i;
	for ( i = 0; i < accounts.count; i++ ) {
		PROTOCOLDESCRIPTOR* ppd;
		PROTOACCOUNT* pa = accounts.items[i];
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

		pa->ppro = ppd->fnInit( pa->szModuleName, pa->tszAccountName );
	}

	return 0;
}
