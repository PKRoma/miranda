/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
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

/////////////////////////////////////////////////////////////////////////////////////////
// constructors and destructor

MimeHeaders::MimeHeaders() :
	mCount( 0 ),
	mAllocCount(0),
	mVals( NULL )
{
}

MimeHeaders::MimeHeaders( unsigned iInitCount ) :
	mCount( 0 )
{
	mAllocCount = iInitCount;
	mVals = ( MimeHeader* )mir_alloc( iInitCount * sizeof( MimeHeader ));
}

MimeHeaders::~MimeHeaders()
{
	if ( mCount == NULL )
		return;

	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		mir_free( H.name );
		mir_free( H.value );
	}

	mir_free( mVals );
}

/////////////////////////////////////////////////////////////////////////////////////////
// add various values

void MimeHeaders::addString( const char* name, const char* szValue )
{
	MimeHeader& H = mVals[ mCount++ ];
	H.name = mir_strdup( name );
	H.value = mir_strdup( szValue );
}

void MimeHeaders::addLong( const char* name, long lValue )
{
	MimeHeader& H = mVals[ mCount++ ];
	H.name = mir_strdup( name );

	char szBuffer[ 20 ];
	ltoa( lValue, szBuffer, 10 );
	H.value = mir_strdup( szBuffer );
}

/////////////////////////////////////////////////////////////////////////////////////////
// write all values to a buffer

size_t MimeHeaders::getLength()
{
	size_t iResult = 0;
	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		iResult += strlen( H.name ) + strlen( H.value ) + 4;
	}

	return iResult;
}

char* MimeHeaders::writeToBuffer( char* pDest )
{
	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		pDest += sprintf( pDest, "%s: %s\r\n", H.name, H.value );
	}

	return pDest;
}

/////////////////////////////////////////////////////////////////////////////////////////
// read set of values from buffer

const char* MimeHeaders::readFromBuffer( const char* parString )
{
	mCount = 0;

	while ( *parString ) {
		if ( parString[0] == '\r' && parString[1] == '\n' ) {
			parString += 2;
			break;
		}

		const char* peol = strchr( parString, '\r' );
		if ( peol == NULL )
			peol = parString + strlen(parString);

		size_t cbLen = peol - parString;
		if ( *++peol == '\n' )
			peol++;

		char* delim = ( char* )memchr( parString, ':', cbLen );
		if ( delim == NULL ) {
			MSN_DebugLog( "MSG: Invalid MIME header: '%s'", parString );
			parString = peol;
			continue;
		}
		size_t nmLen = delim++ - parString;
		
		while ( *delim == ' ' || *delim == '\t' )
			delim++;
		
		size_t vlLen = cbLen - ( delim - parString );

		if ( mCount >= mAllocCount ) 
		{
			mAllocCount += 10;
			mVals = ( MimeHeader* )mir_realloc( mVals, sizeof( MimeHeader ) * mAllocCount );
		}

		MimeHeader& H = mVals[ mCount ];

		
		H.name = ( char* )mir_alloc( nmLen + 1 ); 
		memcpy( H.name, parString, nmLen ); 
		H.name[nmLen] = 0;

		
		H.value = ( char* )mir_alloc( vlLen + 1 ); 
		memcpy( H.value, delim, vlLen ); 
		H.value[vlLen] = 0;

		mCount++;
		parString = peol;
	}

	return parString;
}

const char* MimeHeaders::find( const char* szFieldName )
{
	for ( unsigned i=0; i < mCount; i++ ) {
		MimeHeader& MH = mVals[i];
		if ( !strcmp( MH.name, szFieldName ))
			return MH.value;
	}

	return NULL;
}

static const struct
{
	unsigned cp;
	char* mimecp;
} cptbl[] =
{
	{   037, "IBM037" },		  // IBM EBCDIC US-Canada 
	{   437, "IBM437" },		  // OEM United States 
	{   500, "IBM500" },          // IBM EBCDIC International 
	{   708, "ASMO-708" },        // Arabic (ASMO 708) 
	{   720, "DOS-720" },         // Arabic (Transparent ASMO); Arabic (DOS) 
	{   737, "ibm737" },          // OEM Greek (formerly 437G); Greek (DOS) 
	{   775, "ibm775" },          // OEM Baltic; Baltic (DOS) 
	{   850, "ibm850" },          // OEM Multilingual Latin 1; Western European (DOS) 
	{   852, "ibm852" },          // OEM Latin 2; Central European (DOS) 
	{   855, "IBM855" },          // OEM Cyrillic (primarily Russian) 
	{   857, "ibm857" },          // OEM Turkish; Turkish (DOS) 
	{   858, "IBM00858" },        // OEM Multilingual Latin 1 + Euro symbol 
	{   860, "IBM860" },          // OEM Portuguese; Portuguese (DOS) 
	{   861, "ibm861" },          // OEM Icelandic; Icelandic (DOS) 
	{   862, "DOS-862" },         // OEM Hebrew; Hebrew (DOS) 
	{   863, "IBM863" },          // OEM French Canadian; French Canadian (DOS) 
	{   864, "IBM864" },          // OEM Arabic; Arabic (864) 
	{   865, "IBM865" },          // OEM Nordic; Nordic (DOS) 
	{   866, "cp866" },           // OEM Russian; Cyrillic (DOS) 
	{   869, "ibm869" },		  // OEM Modern Greek; Greek, Modern (DOS) 
	{   870, "IBM870" },          // IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2 
	{   874, "windows-874" },     // ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows) 
	{   875, "cp875" },           // IBM EBCDIC Greek Modern 
	{   932, "shift_jis" },       // ANSI/OEM Japanese; Japanese (Shift-JIS) 
	{   936, "gb2312" },          // ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312) 
	{   949, "ks_c_5601-1987" },  // ANSI/OEM Korean (Unified Hangul Code) 
	{   950, "big5" },            // ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5) 
	{  1026, "IBM1026" },         // IBM EBCDIC Turkish (Latin 5) 
	{  1047, "IBM01047" },        // IBM EBCDIC Latin 1/Open System 
	{  1140, "IBM01140" },        // IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)  
	{  1141, "IBM01141" },        // IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro) 
	{  1142, "IBM01142" },        // IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro) 
	{  1143, "IBM01143" },        // IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro) 
	{  1144, "IBM01144" },        // IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro) 
	{  1145, "IBM01145" },        // IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro) 
	{  1146, "IBM01146" },        // IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro) 
	{  1147, "IBM01147" },        // IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro) 
	{  1148, "IBM01148" },        // IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro) 
	{  1149, "IBM01149" },        // IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro) 
	{  1250, "windows-1250" },    // ANSI Central European; Central European (Windows)  
	{  1251, "windows-1251" },    // ANSI Cyrillic; Cyrillic (Windows) 
	{  1252, "windows-1252" },    // ANSI Latin 1; Western European (Windows)  
	{  1253, "windows-1253" },    // ANSI Greek; Greek (Windows) 
	{  1254, "windows-1254" },    // ANSI Turkish; Turkish (Windows) 
	{  1255, "windows-1255" },    // ANSI Hebrew; Hebrew (Windows) 
	{  1256, "windows-1256" },    // ANSI Arabic; Arabic (Windows) 
	{  1257, "windows-1257" },    // ANSI Baltic; Baltic (Windows) 
	{  1258, "windows-1258" },    // ANSI/OEM Vietnamese; Vietnamese (Windows) 
	{ 20127, "us-ascii" },        // US-ASCII (7-bit) 
	{ 20273, "IBM273" },          // IBM EBCDIC Germany 
	{ 20277, "IBM277" },          // IBM EBCDIC Denmark-Norway 
	{ 20278, "IBM278" },          // IBM EBCDIC Finland-Sweden 
	{ 20280, "IBM280" },          // IBM EBCDIC Italy 
	{ 20284, "IBM284" },          // IBM EBCDIC Latin America-Spain 
	{ 20285, "IBM285" },          // IBM EBCDIC United Kingdom 
	{ 20290, "IBM290" },          // IBM EBCDIC Japanese Katakana Extended 
	{ 20297, "IBM297" },          // IBM EBCDIC France 
	{ 20420, "IBM420" },          // IBM EBCDIC Arabic 
	{ 20423, "IBM423" },          // IBM EBCDIC Greek 
	{ 20424, "IBM424" },          // IBM EBCDIC Hebrew 
	{ 20838, "IBM-Thai" },        // IBM EBCDIC Thai 
	{ 20866, "koi8-r" },          // Russian (KOI8-R); Cyrillic (KOI8-R) 
	{ 20871, "IBM871" },          // IBM EBCDIC Icelandic 
	{ 20880, "IBM880" },          // IBM EBCDIC Cyrillic Russian 
	{ 20905, "IBM905" },          // IBM EBCDIC Turkish 
	{ 20924, "IBM00924" },        // IBM EBCDIC Latin 1/Open System (1047 + Euro symbol) 
	{ 20932, "EUC-JP" },          // Japanese (JIS 0208-1990 and 0121-1990) 
	{ 21025, "cp1025" },          // IBM EBCDIC Cyrillic Serbian-Bulgarian 
	{ 21866, "koi8-u" },          // Ukrainian (KOI8-U); Cyrillic (KOI8-U) 
	{ 28591, "iso-8859-1" },      // ISO 8859-1 Latin 1; Western European (ISO) 
	{ 28592, "iso-8859-2" },      // ISO 8859-2 Central European; Central European (ISO) 
	{ 28593, "iso-8859-3" },      // ISO 8859-3 Latin 3 
	{ 28594, "iso-8859-4" },      // ISO 8859-4 Baltic 
	{ 28595, "iso-8859-5" },      // ISO 8859-5 Cyrillic 
	{ 28596, "iso-8859-6" },      // ISO 8859-6 Arabic 
	{ 28597, "iso-8859-7" },      // ISO 8859-7 Greek 
	{ 28598, "iso-8859-8" },      // ISO 8859-8 Hebrew; Hebrew (ISO-Visual) 
	{ 28599, "iso-8859-9" },      // ISO 8859-9 Turkish 
	{ 28603, "iso-8859-13" },     // ISO 8859-13 Estonian 
	{ 28605, "iso-8859-15" },     // ISO 8859-15 Latin 9 
	{ 38598, "iso-8859-8-i" },    // ISO 8859-8 Hebrew; Hebrew (ISO-Logical) 
	{ 50220, "iso-2022-jp" },     // ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS) 
	{ 50221, "csISO2022JP" },     // ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana) 
	{ 50222, "iso-2022-jp" },     // ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI) 
	{ 50225, "iso-2022-kr" },     // ISO 2022 Korean  
	{ 50227, "ISO-2022-CN" },     // ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022) 
	{ 50229, "ISO-2022-CN-EXT" }, // ISO 2022 Traditional Chinese 
	{ 51932, "euc-jp" },          // EUC Japanese 
	{ 51936, "EUC-CN" },          // EUC Simplified Chinese; Chinese Simplified (EUC) 
	{ 51949, "euc-kr" },          // EUC Korean 
	{ 52936, "hz-gb-2312" },      // HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)  
	{ 54936, "GB18030" },         // Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)  
};


static unsigned FindCP( const char* mimecp )
{
	unsigned cp = CP_ACP;
	for (unsigned i = 0; i < SIZEOF(cptbl); ++i)
	{
		if (stricmp(mimecp, cptbl[i].mimecp) == 0)
		{
			cp = cptbl[i].cp;
			break;
		}
	}
	return cp;
}
			

wchar_t* MimeHeaders::decode(const char* fieldName)
{
	const char *val = find(fieldName);

	size_t ssz = strlen(val)+1;
	char* tbuf = (char*)alloca(ssz);
	memcpy(tbuf, val, ssz);

	wchar_t* res = (wchar_t*)mir_alloc(ssz * sizeof(wchar_t));
	wchar_t* resp = res;

	const char *p = tbuf;
	for (;;)
	{
		char *cp = strstr(p, "=?");
		if (cp == NULL) break;
		*cp = 0;

		int sz = MultiByteToWideChar(20127, 0, p, -1, resp, ssz); 
		ssz -= --sz; resp += sz * sizeof(wchar_t); cp += 2; 

		char *pe = strstr(cp, "?=");
		if (pe == NULL) break;
		*pe = 0;

		char *enc = strchr(cp, '?');
		if (enc == NULL) break;
		*(enc++) = 0;

		char *fld = strchr(enc, '?');
		if (fld == NULL) break;
		*(fld++) = 0;

		switch (*enc)
		{
		case 'b':
		case 'B':
			{
				char* dec = MSN_Base64Decode(fld);
				strcpy(fld, dec);
				mir_free(dec);
			}
			break;

		case 'q':
		case 'Q':
			UrlDecode(fld, true);
			break;
		}
		
		if (stricmp(cp, "UTF-8") == 0)
		{
			wchar_t *dec;
			mir_utf8decode(fld, &dec);
			sz = wcslen(dec); 
			wcsncpy(resp, dec, ssz);
			ssz -= sz; resp += sz * sizeof(wchar_t);
			mir_free(dec);
		}
		else {
			sz = MultiByteToWideChar(FindCP(cp), 0, p, -1, resp, ssz);
			if (sz == 0)
				sz = MultiByteToWideChar(CP_ACP, 0, p, -1, resp, ssz);
			ssz -= --sz; resp += sz * sizeof(wchar_t);
		}
 
		p = pe + 2;
	}

	MultiByteToWideChar(20127, 0, p, -1, resp, ssz); 

	return res;
}
