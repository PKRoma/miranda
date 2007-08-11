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

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Decode - converts UTF8-encoded string to the UCS2/MBCS format

char* Utf8DecodeCP( char* str, int codepage, wchar_t** ucs2 )
{
	int len, needs_free = 0;
	wchar_t* tempBuf;
	BOOL errFlag = FALSE;
	
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

	__try
	{
		tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		tempBuf = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
		needs_free = 1;
	}
	{
		wchar_t* d = tempBuf;
		BYTE* s = ( BYTE* )str;

		while( *s )
		{
			if (( *s & 0x80 ) == 0 ) {
				*d++ = *s++;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xE0 ) {
				if ( s[1] == 0 || ( s[1] & 0xC0 ) != 0x80 ) { errFlag = 1; goto LBL_Exit; }
				if ( s[2] == 0 || ( s[2] & 0xC0 ) != 0x80 ) { errFlag = 1; goto LBL_Exit; }
				*d++ = (( WORD )( s[0] & 0x0F) << 12 ) + ( WORD )(( s[1] & 0x3F ) << 6 ) + ( WORD )( s[2] & 0x3F );
				s += 3;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xC0 ) {
				if ( s[1] == 0 || ( s[1] & 0xC0 ) != 0x80 ) { errFlag = 1; goto LBL_Exit; }
				*d++ = ( WORD )(( s[0] & 0x1F ) << 6 ) + ( WORD )( s[1] & 0x3F );
				s += 2;
				continue;
			}

			*d++ = *s++;
		}

		*d = 0;
	}

	if ( ucs2 != NULL ) {
		int fullLen = ( len+1 )*sizeof( wchar_t );
		*ucs2 = ( wchar_t* )mir_alloc( fullLen );
		memcpy( *ucs2, tempBuf, fullLen );
	}

	WideCharToMultiByte( codepage, 0, tempBuf, -1, str, len, "?", &errFlag );

LBL_Exit:
   if ( needs_free )
		mir_free( tempBuf );

	return ( errFlag ) ? NULL : str;
}

char* Utf8Decode( char* str, wchar_t** ucs2 )
{
	return Utf8DecodeCP( str, LangPackGetDefaultCodePage(), ucs2 );
}

wchar_t* Utf8DecodeUcs2( const char* str )
{
	wchar_t* ucs2;
	char *tempBuffer = mir_strdup(str);
	Utf8Decode( tempBuffer, &ucs2 );
	mir_free(tempBuffer);
	return ucs2;

}

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Encode - converts MBCS string to the UTF8-encoded format

char* Utf8EncodeCP( const char* src, int codepage )
{
	int len;
	char* result;
	wchar_t* tempBuf;

	if ( src == NULL )
		return NULL;

	len = strlen( src );
	result = ( char* )mir_alloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
	MultiByteToWideChar( codepage, 0, src, -1, tempBuf, len );
	tempBuf[ len ] = 0;
	{
		wchar_t* s = tempBuf;
		BYTE*		d = ( BYTE* )result;

		while( *s ) {
			int U = *s++;

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
	}

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
	int len = wcslen( src );
	char* result = ( char* )mir_alloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	{	const wchar_t* s = src;
		BYTE*	d = ( BYTE* )result;

		while( *s ) {
			int U = *s++;

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
	}

	return result;
}
