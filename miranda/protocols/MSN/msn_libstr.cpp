/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

#include "msn_global.h"

void replaceStr( char*& dest, const char* src )
{
	if ( src != NULL ) {
		mir_free( dest );
		dest = mir_strdup( src );
}	}

static TCHAR* a2tf( const TCHAR* str, bool unicode )
{
	if ( str == NULL )
		return NULL;

	return unicode ? mir_tstrdup( str ) : mir_a2t(( char* )str );
}

void overrideStr( TCHAR*& dest, const TCHAR* src, bool unicode, const TCHAR* def )
{
	if ( dest != NULL )
	{
		mir_free( dest );
		dest = NULL;
	}

	if ( src != NULL )
		dest = a2tf( src, unicode );
	else if ( def != NULL )
		dest = mir_tstrdup( def );
}

char* rtrim( char *string )
{
   char* p = string + strlen( string ) - 1;

   while ( p >= string )
   {  if ( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
         break;

		*p-- = 0;
   }
   return string;
}

wchar_t* rtrim( wchar_t* string )
{
   wchar_t* p = string + wcslen( string ) - 1;

   while ( p >= string )
   {  if ( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
         break;

		*p-- = 0;
   }
   return string;
}

char* arrayToHex(BYTE* data, size_t datasz)
{
	char* res = (char*)mir_alloc(2 * datasz + 1);

	char* resptr = res;
	for (unsigned i=0; i<datasz ; i++)
	{
		const BYTE ch = data[i];

		const char ch0 = ch >> 4;
		*resptr++ = (ch0 <= 9) ? ('0' + ch0) : (('a' - 10) + ch0);

		const char ch1 = ch & 0xF;
		*resptr++ = (ch1 <= 9) ? ('0' + ch1) : (('a' - 10) + ch1);
	}
	*resptr = '\0';
	return res;
} 

bool txtParseParam (const char* szData, const char* presearch, const char* start, const char* finish, char* param, const int size)
{
	const char *cp, *cp1;
	int len;
	
	if (szData == NULL) return false;

	if (presearch != NULL)
	{
		cp1 = strstr(szData, presearch);
		if (cp1 == NULL) return false;
	}
	else
		cp1 = szData;

	cp = strstr(cp1, start);
	if (cp == NULL) return false;
	cp += strlen(start);
	while (*cp == ' ') ++cp;

	if (finish)
	{
		cp1 = strstr(cp, finish);
		if (cp1 == NULL) return FALSE;
		while (*(cp1-1) == ' ' && cp1 > cp) --cp1;
	}
	else
		cp1 = strchr(cp, '\0');

	len = min(cp1 - cp, size - 1);
	memmove(param, cp, len);
	param[len] = 0;

	return true;
} 


/////////////////////////////////////////////////////////////////////////////////////////
// UrlDecode - converts URL chars like %20 into printable characters

static int SingleHexToDecimal(char c)
{
	if ( c >= '0' && c <= '9' ) return c-'0';
	if ( c >= 'a' && c <= 'f' ) return c-'a'+10;
	if ( c >= 'A' && c <= 'F' ) return c-'A'+10;
	return -1;
}

void  UrlDecode( char* str )
{
	char* s = str, *d = str;

	while( *s )
	{
		if ( *s == '%' ) {
			int digit1 = SingleHexToDecimal( s[1] );
			if ( digit1 != -1 ) {
				int digit2 = SingleHexToDecimal( s[2] );
				if ( digit2 != -1 ) {
					s += 3;
					*d++ = (char)(( digit1 << 4 ) | digit2);
					continue;
		}	}	}
		*d++ = *s++;
	}

	*d = 0;
}

void  HtmlDecode( char* str )
{
	char* p, *q;

	if ( str == NULL )
		return;

	for ( p=q=str; *p!='\0'; p++,q++ ) {
		if ( *p == '&' ) {
			if ( !strncmp( p, "&amp;", 5 )) {	*q = '&'; p += 4; }
			else if ( !strncmp( p, "&apos;", 6 )) { *q = '\''; p += 5; }
			else if ( !strncmp( p, "&gt;", 4 )) { *q = '>'; p += 3; }
			else if ( !strncmp( p, "&lt;", 4 )) { *q = '<'; p += 3; }
			else if ( !strncmp( p, "&quot;", 6 )) { *q = '"'; p += 5; }
			else { *q = *p;	}
		}
		else {
			*q = *p;
		}
	}
	*q = '\0';
}

/////////////////////////////////////////////////////////////////////////////////////////
// HtmlEncode - replaces special HTML chars

WCHAR*  HtmlEncodeW( const WCHAR* str )
{
	WCHAR* s, *p, *q;
	int c;

	if ( str == NULL )
		return NULL;

	for ( c=0,p=( WCHAR* )str; *p!=L'\0'; p++ ) {
		switch ( *p ) {
		case L'&': c += 5; break;
		case L'\'': c += 6; break;
		case L'>': c += 4; break;
		case L'<': c += 4; break;
		case L'"': c += 6; break;
		default: c++; break;
		}
	}
	if (( s=( WCHAR* )mir_alloc( (c+1) * sizeof(WCHAR) )) != NULL ) {
		for ( p=( WCHAR* )str,q=s; *p!=L'\0'; p++ ) {
			switch ( *p ) {
			case L'&': wcscpy( q, L"&amp;" ); q += 5; break;
			case L'\'': wcscpy( q, L"&apos;" ); q += 6; break;
			case L'>': wcscpy( q, L"&gt;" ); q += 4; break;
			case L'<': wcscpy( q, L"&lt;" ); q += 4; break;
			case L'"': wcscpy( q, L"&quot;" ); q += 6; break;
			default: *q = *p; q++; break;
			}
		}
		*q = L'\0';
	}

	return s;
}

char*  HtmlEncode( const char* str )
{
	char* s, *p, *q;
	int c;

	if ( str == NULL )
		return NULL;

	for ( c=0,p=( char* )str; *p!='\0'; p++ ) {
		switch ( *p ) {
		case '&': c += 5; break;
		case '\'': c += 6; break;
		case '>': c += 4; break;
		case '<': c += 4; break;
		case '"': c += 6; break;
		default: c++; break;
		}
	}
	if (( s=( char* )mir_alloc( c+1 )) != NULL ) {
		for ( p=( char* )str,q=s; *p!='\0'; p++ ) {
			switch ( *p ) {
			case '&': strcpy( q, "&amp;" ); q += 5; break;
			case '\'': strcpy( q, "&apos;" ); q += 6; break;
			case '>': strcpy( q, "&gt;" ); q += 4; break;
			case '<': strcpy( q, "&lt;" ); q += 4; break;
			case '"': strcpy( q, "&quot;" ); q += 6; break;
			default: *q = *p; q++; break;
			}
		}
		*q = '\0';
	}

	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UrlEncode - converts printable characters into URL chars like %20

void  UrlEncode( const char* src, char* dest, size_t cbDest )
{
	char* d = dest;
	size_t   i = 0;

	for( const char* s = src; *s; s++ ) {
		if (( *s <= '/' && *s != '.' && *s != '-' ) ||
			 ( *s >= ':' && *s <= '?' ) ||
			 ( *s >= '[' && *s <= '`' && *s != '_' ))
		{
			if ( i + 4 >= cbDest ) break;

			*d++ = '%';
			_itoa( *s, d, 16 );
			d += 2;
			i += 3;
		}
		else
		{
			if ( ++i >= cbDest ) break;
			*d++ = *s;
	}	}

	*d = '\0';
}


