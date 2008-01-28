/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"

extern LIST<void> arHooks;
extern LIST<void> arServices;

void CJabberProto::JCreateService( const char* szService, JServiceFunc serviceProc )
{
	char str[ MAXMODULELABELLENGTH ];
	strcpy( str, szProtoName );
	strcat( str, szService );
	arServices.insert( ::CreateServiceFunctionObj( str, ( MIRANDASERVICEOBJ )*( void** )&serviceProc, this ));
}

void CJabberProto::JCreateServiceParam( const char* szService, JServiceFuncParam serviceProc, LPARAM lParam )
{
	char str[ MAXMODULELABELLENGTH ];
	strcpy( str, szProtoName );
	strcat( str, szService );
	arServices.insert( ::CreateServiceFunctionObjParam( str, ( MIRANDASERVICEOBJPARAM )*( void** )&serviceProc, this, lParam ));
}

void CJabberProto::JHookEvent( const char* szEvent, JEventFunc handler )
{
	arHooks.insert( ::HookEventObj( szEvent, ( MIRANDAHOOKOBJ )*( void** )&handler, this ));
}

HANDLE CJabberProto::JCreateHookableEvent( const char* szService )
{
	char str[ MAXMODULELABELLENGTH ];
	strcpy( str, szProtoName );
	strcat( str, szService );
	return CreateHookableEvent( str );
}

/////////////////////////////////////////////////////////////////////////////////////////

#if !defined( _DEBUG )
int __stdcall JCallService( const char* szSvcName, WPARAM wParam, LPARAM lParam )
{
	return CallService( szSvcName, wParam, lParam );
}
#endif

void CJabberProto::JDeleteSetting( HANDLE hContact, const char* valueName )
{
   DBDeleteContactSetting( hContact, szProtoName, valueName );
}

DWORD CJabberProto::JGetByte( const char* valueName, int parDefltValue )
{
	return DBGetContactSettingByte( NULL, szProtoName, valueName, parDefltValue );
}

DWORD CJabberProto::JGetByte( HANDLE hContact, const char* valueName, int parDefltValue )
{
	return DBGetContactSettingByte( hContact, szProtoName, valueName, parDefltValue );
}

char* __stdcall JGetContactName( HANDLE hContact )
{
	return ( char* )JCallService( MS_CLIST_GETCONTACTDISPLAYNAME, WPARAM( hContact ), 0 );
}

DWORD CJabberProto::JGetDword( HANDLE hContact, const char* valueName, DWORD parDefltValue )
{
	return DBGetContactSettingDword( hContact, szProtoName, valueName, parDefltValue );
}

int CJabberProto::JGetStaticString( const char* valueName, HANDLE hContact, char* dest, int dest_len )
{
	DBVARIANT dbv;
	dbv.pszVal = dest;
	dbv.cchVal = dest_len;
	dbv.type = DBVT_ASCIIZ;

	DBCONTACTGETSETTING sVal;
	sVal.pValue = &dbv;
	sVal.szModule = szProtoName;
	sVal.szSetting = valueName;
	if ( JCallService( MS_DB_CONTACT_GETSETTINGSTATIC, ( WPARAM )hContact, ( LPARAM )&sVal ) != 0 )
		return 1;

	return ( dbv.type != DBVT_ASCIIZ );
}

int CJabberProto::JGetStringUtf( HANDLE hContact, char* valueName, DBVARIANT* dbv )
{
	return DBGetContactSettingStringUtf( hContact, szProtoName, valueName, dbv );
}

int CJabberProto::JGetStringT( HANDLE hContact, char* valueName, DBVARIANT* dbv )
{
	return DBGetContactSettingTString( hContact, szProtoName, valueName, dbv );
}

WORD CJabberProto::JGetWord( HANDLE hContact, const char* valueName, int parDefltValue )
{
	return DBGetContactSettingWord( hContact, szProtoName, valueName, parDefltValue );
}

void __fastcall JFreeVariant( DBVARIANT* dbv )
{
	DBFreeVariant( dbv );
}

int CJabberProto::JSendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam )
{
	ACKDATA ack = {0};
	ack.cbSize = sizeof( ACKDATA );
	ack.szModule = szProtoName;
	ack.hContact = hContact;
	ack.type = type;
	ack.result = result;
	ack.hProcess = hProcess;
	ack.lParam = lParam;
	return JCallService( MS_PROTO_BROADCASTACK, 0, ( LPARAM )&ack );
}

DWORD CJabberProto::JSetByte( const char* valueName, int parValue )
{
	return DBWriteContactSettingByte( NULL, szProtoName, valueName, parValue );
}

DWORD CJabberProto::JSetByte( HANDLE hContact, const char* valueName, int parValue )
{
	return DBWriteContactSettingByte( hContact, szProtoName, valueName, parValue );
}

DWORD CJabberProto::JSetDword( HANDLE hContact, const char* valueName, DWORD parValue )
{
	return DBWriteContactSettingDword( hContact, szProtoName, valueName, parValue );
}

DWORD CJabberProto::JSetString( HANDLE hContact, const char* valueName, const char* parValue )
{
	return DBWriteContactSettingString( hContact, szProtoName, valueName, parValue );
}

DWORD CJabberProto::JSetStringT( HANDLE hContact, const char* valueName, const TCHAR* parValue )
{
	return DBWriteContactSettingTString( hContact, szProtoName, valueName, parValue );
}

DWORD CJabberProto::JSetStringUtf( HANDLE hContact, const char* valueName, const char* parValue )
{
	return DBWriteContactSettingStringUtf( hContact, szProtoName, valueName, parValue );
}

DWORD CJabberProto::JSetWord( HANDLE hContact, const char* valueName, int parValue )
{
	return DBWriteContactSettingWord( hContact, szProtoName, valueName, parValue );
}

char* __fastcall JTranslate( const char* str )
{
	return Translate( str );
}

// save/load crypted strings
static void __forceinline sttCryptString(char *str)
{
	while (*str) if (*str != 0xc3) *str++ ^= 0xc3;
}

TCHAR* CJabberProto::JGetStringCrypt( HANDLE hContact, char* valueName )
{
	DBVARIANT dbv;

	if (DBGetContactSettingString( hContact, szProtoName, valueName, &dbv ))
		return NULL;

	sttCryptString(dbv.pszVal);

#ifdef UNICODE
	WCHAR *res = mir_utf8decodeW(dbv.pszVal);
#else
	mir_utf8decode(dbv.pszVal, NULL);
	char *res = mir_strdup(dbv.pszVal);
#endif

	DBFreeVariant(&dbv);
	return res;
}

DWORD CJabberProto::JSetStringCrypt( HANDLE hContact, char* valueName, const TCHAR* parValue )
{
	char *tmp = mir_utf8encodeT(parValue);
	sttCryptString(tmp);
	DWORD res = JSetString( hContact, valueName, tmp );
	mir_free(tmp);
	return res;
}
