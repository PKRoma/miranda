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

// we assume that the output buffer has the appropriate lenght

static wchar_t* Utf8toUcs2( const char* in, wchar_t* out )
{
	BOOL errFlag = FALSE;	
	wchar_t* d = out;
	BYTE* s = ( BYTE* )in;

	while( *s ) {
		if (( *s & 0x80 ) == 0 ) {
			*d++ = *s++;
			continue;
		}

		if (( s[0] & 0xE0 ) == 0xE0 ) {
			if ( s[1] == 0 || ( s[1] & 0xC0 ) != 0x80 ) { errFlag = TRUE; goto LBL_Exit; }
			if ( s[2] == 0 || ( s[2] & 0xC0 ) != 0x80 ) { errFlag = TRUE; goto LBL_Exit; }
			*d++ = (( WORD )( s[0] & 0x0F) << 12 ) + ( WORD )(( s[1] & 0x3F ) << 6 ) + ( WORD )( s[2] & 0x3F );
			s += 3;
			continue;
		}

		if (( s[0] & 0xE0 ) == 0xC0 ) {
			if ( s[1] == 0 || ( s[1] & 0xC0 ) != 0x80 ) { errFlag = TRUE; goto LBL_Exit; }
			*d++ = ( WORD )(( s[0] & 0x1F ) << 6 ) + ( WORD )( s[1] & 0x3F );
			s += 2;
			continue;
		}

		*d++ = *s++;
	}

	*d = 0;

LBL_Exit:

	return ( errFlag ) ? NULL : out;
}

static char* Ucs2toUtf8( const wchar_t* in, char* out )
{
	const wchar_t* s = in;
	BYTE* d = ( BYTE* )out;
    int U;

	while( *s ) {
		U = *s++;

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

	return out;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Decode - converts UTF8-encoded string to the UCS2/MBCS format

char* Utf8DecodeCP( char* str, int codepage, wchar_t** ucs2 )
{
	size_t len; 
    bool needs_free = false;
	wchar_t* tempBuf = NULL;

	if ( str == NULL )
		return NULL;

	len = strlen( str );

	if ( len < 2 ) {
		if ( ucs2 != NULL ) {
			*ucs2 = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
			MultiByteToWideChar( codepage, 0, str, len, *ucs2, len );
			( *ucs2 )[ len ] = 0;
		}
		return str;
	}

	if ( ucs2 == NULL ) {
		__try
		{
			tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			tempBuf = NULL;
			needs_free = false;
		}
	}

	if ( tempBuf == NULL )
		tempBuf = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
	
	if ( Utf8toUcs2( str, tempBuf )) {
		WideCharToMultiByte( codepage, 0, tempBuf, -1, str, len, "?", NULL );

		if ( ucs2 )
			*ucs2 = tempBuf;
		else if ( needs_free )
			mir_free( tempBuf );

		return str;
	}
	else if ( ucs2 || needs_free )
		mir_free( tempBuf );

	return NULL;
}

char* Utf8Decode( char* str, wchar_t** ucs2 )
{
	return Utf8DecodeCP( str, LangPackGetDefaultCodePage(), ucs2 );
}

wchar_t* Utf8DecodeUcs2( const char* str )
{
	size_t len;
	wchar_t* ucs2;

	if ( str == NULL )
		return NULL;

	len = strlen( str );

	if ( len < 2 ) {
		ucs2 = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
		MultiByteToWideChar( LangPackGetDefaultCodePage(), 0, str, len, ucs2, len );
		( ucs2 )[ len ] = 0;
		return ucs2;
	}

	ucs2 = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));

	if ( Utf8toUcs2( str, ucs2 ))
		return ucs2;

	mir_free( ucs2 );

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Encode - converts MBCS string to the UTF8-encoded format

char* Utf8EncodeCP( const char* src, int codepage )
{
	size_t len; 
    bool needs_free = false;
	char* result;
	wchar_t* tempBuf;

	if ( src == NULL )
		return NULL;

	len = strlen( src );
	result = ( char* )mir_alloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	__try
	{
		tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		tempBuf = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
		needs_free = true;
	}

	MultiByteToWideChar( codepage, 0, src, -1, tempBuf, len );
	tempBuf[ len ] = 0;

	Ucs2toUtf8( tempBuf, result );

	if ( needs_free )
		mir_free( tempBuf );

	return result;
}

char* Utf8Encode( const char* src )
{
	return Utf8EncodeCP( src, LangPackGetDefaultCodePage());
}

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Encode - converts UCS2 string to the UTF8-encoded format

char* Utf8EncodeUcs2( const wchar_t* src )
{
	size_t len = wcslen( src );
	char* result = ( char* )mir_alloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	Ucs2toUtf8( src, result );

	return result;
}
