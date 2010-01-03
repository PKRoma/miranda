/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan

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
#include "jabber_secur.h"

typedef BYTE (WINAPI *GetUserNameExType )( int NameFormat, LPTSTR lpNameBuffer, PULONG nSize );


/////////////////////////////////////////////////////////////////////////////////////////
// ntlm auth - LanServer based authorization

TNtlmAuth::TNtlmAuth( ThreadData* info, const char* mechanism ) :
	TJabberAuth( info )
{
	szName = mechanism;

	const TCHAR *szProvider;
	if ( !strcmp( mechanism, "GSS-SPNEGO" ))
		szProvider = _T("Negotiate");
	else if ( !strcmp( mechanism, "NTLM" ))
		szProvider = _T("NTLM");
	else {
		bIsValid = false;
		return;
	}

	if (( hProvider = Netlib_InitSecurityProvider2( szProvider, NULL )) == NULL )
		bIsValid = false;
}

TNtlmAuth::~TNtlmAuth()
{
	if ( hProvider != NULL )
		Netlib_DestroySecurityProvider( NULL, hProvider );
}

char* TNtlmAuth::getInitialRequest()
{
	if ( !hProvider )
		return NULL;

	// This generates login method advertisement packet
	char* result;
	if ( info->password[0] != 0 ) {
		TCHAR* pass = mir_a2t( info->password );
		result = Netlib_NtlmCreateResponse2( hProvider, "", info->username, pass, &complete );
		mir_free( pass );
	}
	else result = Netlib_NtlmCreateResponse2( hProvider, "", NULL, NULL, &complete );

	return result;
}

char* TNtlmAuth::getChallenge( const TCHAR* challenge )
{
	if ( !hProvider )
		return NULL;

	char *text = ( !lstrcmp( challenge, _T("="))) ? mir_strdup( "" ) : mir_t2a( challenge ), *result;
	if ( info->password[0] != 0 ) {
		TCHAR* pass = mir_a2t( info->password );
		result = Netlib_NtlmCreateResponse2( hProvider, text, info->username, pass, &complete );
		mir_free( pass );
	}
	else result = Netlib_NtlmCreateResponse2( hProvider, text, NULL, NULL, &complete );
	
	mir_free( text );
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// GSSAPI auth - Kerberos V authorization

TGssApiAuth::TGssApiAuth( ThreadData* info, const TCHAR* /* hostname */ ) :
	TJabberAuth( info )
{
	szName = "GSSAPI";

	// This not currently logged on user, bye bye
	if ( info->password[0] ) { bIsValid = false; return; }

#ifdef UNICODE
	GetUserNameExType myGetUserNameEx = 
		(GetUserNameExType) GetProcAddress(GetModuleHandleA( "secur32.dll" ), "GetUserNameExW");
#else
	GetUserNameExType myGetUserNameEx = 
		(GetUserNameExType) GetProcAddress(GetModuleHandleA( "secur32.dll" ), "GetUserNameExA");
#endif
	if (!myGetUserNameEx) { bIsValid = false; return; }

	TCHAR szFullUserName[128] = _T( "" );
	ULONG szFullUserNameLen = SIZEOF( szFullUserName );
	if (!myGetUserNameEx( 12, szFullUserName, &szFullUserNameLen )) {
		szFullUserName[ 0 ] = 0; 
		szFullUserNameLen = SIZEOF( szFullUserName );
		myGetUserNameEx( 2, szFullUserName, &szFullUserNameLen );
	}

	TCHAR* name = _tcsrchr(szFullUserName, '\\');
	if (name) *name = 0;
	else { bIsValid = false; return; }

	_tcsupr(szFullUserName);

	char* connectHost = info->manualHost[0] ? info->manualHost : info->server;

	unsigned long ip = inet_addr( connectHost );
	if ( ip == INADDR_NONE )
	{
		PHOSTENT host = gethostbyname( connectHost );
		if ( host != NULL )
			ip = (( PIN_ADDR )host->h_addr )->S_un.S_addr;
	}

	PHOSTENT host = gethostbyaddr(( char* )&ip, 4, AF_INET );
	if ( !host ) { bIsValid = false; return; }

	TCHAR *connectHostDSN = mir_a2t( host->h_name ); 

	TCHAR szSpn[256];
	mir_sntprintf( szSpn, SIZEOF(szSpn), _T("xmpp/%s@%s"), connectHostDSN, szFullUserName );

	mir_free( connectHostDSN );

	if (( hProvider = Netlib_InitSecurityProvider2( _T("Kerberos"), szSpn )) == NULL )
		bIsValid = false;
}

TGssApiAuth::~TGssApiAuth()
{
	if ( hProvider != NULL )
		Netlib_DestroySecurityProvider( NULL, hProvider );
}

char* TGssApiAuth::getInitialRequest()
{
	if ( !hProvider )
		return NULL;

	char* result = Netlib_NtlmCreateResponse2( hProvider, "", NULL, NULL, &complete );

	return result;
}

char* TGssApiAuth::getChallenge( const TCHAR* challenge )
{
	if ( !hProvider )
		return NULL;

	char *text = ( !lstrcmp( challenge, _T("="))) ? mir_strdup( "" ) : mir_t2a( challenge ), *result;
	if ( info->password[0] != 0 ) {
		TCHAR* pass = mir_a2t( info->password );
		result = Netlib_NtlmCreateResponse2( hProvider, text, info->username, pass, &complete );
		mir_free( pass );
	}
	else result = Netlib_NtlmCreateResponse2( hProvider, text, NULL, NULL, &complete );
	
	mir_free( text );
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// md5 auth - digest-based authorization

TMD5Auth::TMD5Auth( ThreadData* info ) :
	TJabberAuth( info ),
	iCallCount( 0 )
{
	szName = "DIGEST-MD5";
}

TMD5Auth::~TMD5Auth()
{
}

char* TMD5Auth::getChallenge( const TCHAR* challenge )
{
	if ( iCallCount > 0 )
		return NULL;

	iCallCount++;

	int resultLen;
	char* text = JabberBase64Decode( challenge, &resultLen );

	TStringPairs pairs( text );
	const char *realm = pairs["realm"], *nonce = pairs["nonce"];

	char randomNumber[40], cnonce[40], tmpBuf[40];
	srand( time(0));
	_itoa( rand(), randomNumber, 10 );

	DWORD digest[4], hash1[4], hash2[4];
	mir_md5_state_t ctx;
	mir_md5_init( &ctx );
	mir_md5_append( &ctx, ( BYTE* )randomNumber, (int)strlen(randomNumber));
	mir_md5_finish( &ctx, ( BYTE* )digest );
	sprintf( cnonce, "%08x%08x%08x%08x", htonl(digest[0]), htonl(digest[1]), htonl(digest[2]), htonl(digest[3]));

	char *uname = mir_utf8encodeT( info->username ), 
		  *passw = mir_utf8encode( info->password ), 
		  *serv  = mir_utf8encode( info->server );

	mir_md5_init( &ctx );
	mir_md5_append( &ctx, ( BYTE* )uname,  (int)strlen( uname ));
	mir_md5_append( &ctx, ( BYTE* )":",    1 );
	mir_md5_append( &ctx, ( BYTE* )realm,  (int)strlen( realm ));
	mir_md5_append( &ctx, ( BYTE* )":",    1 );
	mir_md5_append( &ctx, ( BYTE* )passw,  (int)strlen( passw ));
	mir_md5_finish( &ctx, ( BYTE* )hash1 );

	mir_md5_init( &ctx );
	mir_md5_append( &ctx, ( BYTE* )hash1,  16 );
	mir_md5_append( &ctx, ( BYTE* )":",    1 );
	mir_md5_append( &ctx, ( BYTE* )nonce,  (int)strlen( nonce ));
	mir_md5_append( &ctx, ( BYTE* )":",    1 );
	mir_md5_append( &ctx, ( BYTE* )cnonce, (int)strlen( cnonce ));
	mir_md5_finish( &ctx, ( BYTE* )hash1 );
	
	mir_md5_init( &ctx );
	mir_md5_append( &ctx, ( BYTE* )"AUTHENTICATE:xmpp/", 18 );
	mir_md5_append( &ctx, ( BYTE* )serv,   (int)strlen( serv ));
	mir_md5_finish( &ctx, ( BYTE* )hash2 );

	mir_md5_init( &ctx );
	sprintf( tmpBuf, "%08x%08x%08x%08x", htonl(hash1[0]), htonl(hash1[1]), htonl(hash1[2]), htonl(hash1[3]));
	mir_md5_append( &ctx, ( BYTE* )tmpBuf, (int)strlen( tmpBuf ));
	mir_md5_append( &ctx, ( BYTE* )":",    1 );
	mir_md5_append( &ctx, ( BYTE* )nonce,  (int)strlen( nonce ));
	sprintf( tmpBuf, ":%08d:", iCallCount );
	mir_md5_append( &ctx, ( BYTE* )tmpBuf, (int)strlen( tmpBuf ));
	mir_md5_append( &ctx, ( BYTE* )cnonce, (int)strlen( cnonce ));
	mir_md5_append( &ctx, ( BYTE* )":auth:", 6 );
	sprintf( tmpBuf, "%08x%08x%08x%08x", htonl(hash2[0]), htonl(hash2[1]), htonl(hash2[2]), htonl(hash2[3]));
	mir_md5_append( &ctx, ( BYTE* )tmpBuf, (int)strlen( tmpBuf ));
	mir_md5_finish( &ctx, ( BYTE* )digest );

	char* buf = (char*)alloca(8000);
	int cbLen = mir_snprintf( buf, 8000, 
		"username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\",nc=%08d,"
		"qop=auth,digest-uri=\"xmpp/%s\",charset=utf-8,response=%08x%08x%08x%08x",
		uname, realm, nonce, cnonce, iCallCount, serv,
		htonl(digest[0]), htonl(digest[1]), htonl(digest[2]), htonl(digest[3]));

	mir_free( uname );
	mir_free( passw );
	mir_free( serv );
	mir_free( text );

   return JabberBase64Encode( buf, cbLen );
}

/////////////////////////////////////////////////////////////////////////////////////////
// plain auth - the most simple one

TPlainAuth::TPlainAuth( ThreadData* info ) :
	TJabberAuth( info )
{
	szName = "PLAIN";
}

TPlainAuth::~TPlainAuth()
{
}

char* TPlainAuth::getInitialRequest()
{
	char *temp = mir_t2a(info->username);
	size_t size = strlen(temp)*2+strlen(info->server)+strlen(info->password)+3;
	char *toEncode = ( char* )alloca( size+1 );
	mir_snprintf( toEncode, size+1, "%s@%s%c%s%c%s", temp, info->server, 0, temp, 0, info->password );
	char* result = JabberBase64Encode( toEncode, (int)size );
	mir_free(temp);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// basic type

TJabberAuth::TJabberAuth( ThreadData* pInfo ) :
	bIsValid( true ),
	szName( NULL ),
	info( pInfo )
{
}

TJabberAuth::~TJabberAuth()
{
}

char* TJabberAuth::getInitialRequest()
{
	return NULL;
}

char* TJabberAuth::getChallenge( const TCHAR* )
{
	return NULL;
}
