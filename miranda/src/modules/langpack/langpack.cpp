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

#include "../netlib/netlib.h"

#define LANGPACK_BUF_SIZE 4000

int LoadLangPackServices(void);

struct LangPackMuuid
{
	MUUID muuid;
	PLUGININFOEX* pInfo;
};

static int CompareMuuids( const LangPackMuuid* p1, const LangPackMuuid* p2 )
{
	return memcmp( &p1->muuid, &p2->muuid, sizeof( MUUID ));
}

static LIST<LangPackMuuid> lMuuids( 10, CompareMuuids );
static LangPackMuuid* pCurrentMuuid = NULL;

static BOOL bModuleInitialized = FALSE;

struct LangPackEntry {
	DWORD englishHash;
	char *local;
	wchar_t *wlocal;
	LangPackMuuid* pMuuid;
};

struct LangPackStruct {
	TCHAR filename[MAX_PATH];
	TCHAR filePath[MAX_PATH];
	char  language[64];
	char  lastModifiedUsing[64];
	char  authors[256];
	char  authorEmail[128];
	LangPackEntry *entry;
	int entryCount, entriesAlloced;
	LCID localeID;
	DWORD defaultANSICp;
} static langPack;

static int IsEmpty(char *str)
{
	int i = 0;

	while (str[i])
	{
		if (str[i]!=' '&&str[i]!='\r'&&str[i]!='\n')
			return 0;
		i++;
	}
	return 1;
}

void ConvertBackslashes(char *str)
{
	char *pstr;
	for ( pstr = str; *pstr; pstr = CharNextExA(( WORD )langPack.defaultANSICp, pstr, 0 )) {
		if( *pstr == '\\' ) {
			switch( pstr[1] ) {
			case 'n': *pstr = '\n'; break;
			case 't': *pstr = '\t'; break;
			case 'r': *pstr = '\r'; break;
			default:  *pstr = pstr[1]; break;
			}
			memmove(pstr+1, pstr+2, strlen(pstr+2) + 1);
}	}	}

#ifdef _DEBUG
#pragma optimize( "gt", on )
#endif

// MurmurHash2
unsigned int __fastcall hash(const void * key, unsigned int len)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value
	unsigned int h = len;

	// Mix 4 bytes at a time into the hash
	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array
	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
			h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

unsigned int __fastcall hashstrW(const char * key)
{
	if (key == NULL) return 0;
	const unsigned int len = (unsigned int)wcslen((const wchar_t*)key);
	char* buf = (char*)alloca(len + 1);
	for (unsigned i = 0; i <= len ; ++i)
		buf[i] = key[i << 1];
	return hash(buf, len);
}

static int SortLangPackHashesProc(LangPackEntry *arg1,LangPackEntry *arg2)
{
	if (arg1->englishHash < arg2->englishHash) return -1;
	if (arg1->englishHash > arg2->englishHash) return 1;

	return (arg1->pMuuid < arg2->pMuuid) ? -1 : 1;
}

static void LoadLangPackFile( FILE* fp, char* line )
{
	while ( !feof( fp )) {
		if ( fgets( line, LANGPACK_BUF_SIZE, fp ) == NULL )
			break;

		if ( IsEmpty(line) || line[0] == ';' || line[0] == 0 )
			continue;

		rtrim( line );

		if ( line[0] == '#' ) {
			if ( !memcmp( line+1, "include", 7 )) {
				TCHAR tszFileName[ MAX_PATH ];
				TCHAR* fileName = mir_a2t( ltrim( line+9 ));
				mir_sntprintf( tszFileName, SIZEOF(tszFileName), _T("%s%s"), langPack.filePath, fileName );
				mir_free( fileName );

				FILE* p = _tfopen( tszFileName, _T("r"));
				if ( p ) {
					LoadLangPackFile( p, line );
					fclose( p );
				}
				continue;
			}

			if ( !memcmp( line+1, "muuid", 5 )) {
				MUUID t;
				if ( 11 != sscanf( line+7, "{%8x-%4x-%4x-%2x%2x%2x%2x%2x%2x%2x%2x}", &t.a, &t.b, &t.c, &t.d[0], &t.d[1], &t.d[2], &t.d[3], &t.d[4], &t.d[5], &t.d[6], &t.d[7] )) {
					NetlibLogf( NULL, "Invalid MUUID: %s\n", line+7 );
					continue;
				}

				LangPackMuuid* pNew = ( LangPackMuuid* )mir_alloc( sizeof( LangPackMuuid ));
				memcpy( &pNew->muuid, &t, sizeof( t ));
				pNew->pInfo = NULL;
				lMuuids.insert( pNew );
				pCurrentMuuid = pNew;
				continue;
			}
		}

		ConvertBackslashes( line );

		if ( line[0] == '[' && line[ lstrlenA(line)-1 ] == ']' ) {
			if ( langPack.entryCount && langPack.entry[ langPack.entryCount-1].local == NULL )
				langPack.entryCount--;

			char* pszLine = line+1;
			line[ lstrlenA(line)-1 ] = '\0';
			if ( ++langPack.entryCount > langPack.entriesAlloced ) {
				langPack.entriesAlloced += 128;
				langPack.entry = ( LangPackEntry* )mir_realloc( langPack.entry, sizeof(LangPackEntry)*langPack.entriesAlloced );
			}

			LangPackEntry* E = &langPack.entry[ langPack.entryCount-1 ];
			E->englishHash = hashstr(pszLine);
			E->local = NULL;
			E->wlocal = NULL;
			E->pMuuid = pCurrentMuuid;
			continue;
		}

		if ( !langPack.entryCount )
			continue;

		LangPackEntry* E = &langPack.entry[ langPack.entryCount-1 ];
		if ( E->local == NULL ) {
			E->local = mir_strdup( line );

			int iNeeded = MultiByteToWideChar(langPack.defaultANSICp, 0, line, -1, 0, 0);
			E->wlocal = (wchar_t *)mir_alloc((iNeeded+1) * sizeof(wchar_t));
			MultiByteToWideChar( langPack.defaultANSICp, 0, line, -1, E->wlocal, iNeeded );
		}
		else {
			E->local = ( char* )mir_realloc( E->local, lstrlenA(E->local)+lstrlenA(line)+2 );
			lstrcatA( E->local, "\n" );
			lstrcatA( E->local, line );

			int iNeeded = MultiByteToWideChar( langPack.defaultANSICp, 0, line, -1, 0, 0 );
			size_t iOldLen = wcslen( E->wlocal );
			E->wlocal = ( wchar_t* )mir_realloc( E->wlocal, ( sizeof(wchar_t) * ( iOldLen + iNeeded + 2)));
			wcscat( E->wlocal, L"\n" );
			MultiByteToWideChar( langPack.defaultANSICp, 0, line, -1, E->wlocal + iOldLen+1, iNeeded );
		}
	}
}

static int LoadLangPack(const TCHAR *szLangPack)
{
	int startOfLine=0;
	USHORT langID;

	lstrcpy( langPack.filename, szLangPack );
	lstrcpy( langPack.filePath, szLangPack );
	TCHAR* p = _tcsrchr( langPack.filePath, '\\' );
	if ( p )
		p[1] = 0;

	FILE *fp = _tfopen(szLangPack,_T("rt"));
	if ( fp == NULL )
		return 1;

	char line[ LANGPACK_BUF_SIZE ];
	fgets( line, SIZEOF(line), fp );
	lrtrim( line );
	if ( lstrcmpA( line, "Miranda Language Pack Version 1" )) {
		fclose(fp);
		return 2;
	}

	//headers
	while ( !feof( fp )) {
		startOfLine = ftell( fp );
		if ( fgets( line, SIZEOF(line), fp ) == NULL )
			break;

		lrtrim( line );
		if( IsEmpty( line ) || line[0]==';' || line[0]==0)
			continue;

		if ( line[0] == '[' || line[0] == '#' )
			break;

		char* pszColon = strchr( line,':' );
		if ( pszColon == NULL ) {
			fclose( fp );
			return 3;
		}

		*pszColon = 0;
		if(!lstrcmpA(line,"Language")) {mir_snprintf(langPack.language,sizeof(langPack.language),"%s",pszColon+1); lrtrim(langPack.language);}
		else if(!lstrcmpA(line,"Last-Modified-Using")) {mir_snprintf(langPack.lastModifiedUsing,sizeof(langPack.lastModifiedUsing),"%s",pszColon+1); lrtrim(langPack.lastModifiedUsing);}
		else if(!lstrcmpA(line,"Authors")) {mir_snprintf(langPack.authors,sizeof(langPack.authors),"%s",pszColon+1); lrtrim(langPack.authors);}
		else if(!lstrcmpA(line,"Author-email")) {mir_snprintf(langPack.authorEmail,sizeof(langPack.authorEmail),"%s",pszColon+1); lrtrim(langPack.authorEmail);}
		else if(!lstrcmpA(line, "Locale")) {
			char szBuf[20], *stopped;

			lrtrim(pszColon + 1);
			langID = (USHORT)strtol(pszColon + 1, &stopped, 16);
			langPack.localeID = MAKELCID(langID, 0);
			GetLocaleInfoA(langPack.localeID, LOCALE_IDEFAULTANSICODEPAGE, szBuf, 10);
			szBuf[5] = 0;                       // codepages have max. 5 digits
			langPack.defaultANSICp = atoi(szBuf);
		}
	}

	//body
	fseek( fp, startOfLine, SEEK_SET );
	langPack.entriesAlloced = 0;

	LoadLangPackFile( fp, line );

	qsort(langPack.entry,langPack.entryCount,sizeof(LangPackEntry),(int(*)(const void*,const void*))SortLangPackHashesProc);
	fclose(fp);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int SortLangPackHashesProc2(LangPackEntry *arg1,LangPackEntry *arg2)
{
	if (arg1->englishHash < arg2->englishHash) return -1;
	if (arg1->englishHash > arg2->englishHash) return 1;

	if (arg1->pMuuid == NULL || arg1->pMuuid == arg2->pMuuid)
		return 0;

	return (arg1->pMuuid < arg2->pMuuid) ? -1 : 1;
}

char *LangPackTranslateString(LangPackMuuid* pUuid, const char *szEnglish, const int W)
{
	LangPackEntry key,*entry;

	if ( langPack.entryCount == 0 || szEnglish == NULL ) return (char*)szEnglish;

	key.englishHash = W ? hashstrW(szEnglish) : hashstr(szEnglish);
	key.pMuuid = pUuid;
	entry=(LangPackEntry*)bsearch(&key,langPack.entry,langPack.entryCount,sizeof(LangPackEntry),(int(*)(const void*,const void*))SortLangPackHashesProc2);
	if(entry==NULL) return (char*)szEnglish;
	while(entry>langPack.entry)
	{
		entry--;
		if(entry->englishHash!=key.englishHash) {
			entry++;
			return W ? (char *)entry->wlocal : entry->local;
		}
	}
	return W ? (char *)entry->wlocal : entry->local;
}

int LangPackGetDefaultCodePage()
{
	return (langPack.defaultANSICp == 0) ? CP_ACP : langPack.defaultANSICp;
}

int LangPackGetDefaultLocale()
{
	return (langPack.localeID == 0) ? LOCALE_USER_DEFAULT : langPack.localeID;
}

TCHAR* LangPackPcharToTchar( const char* pszStr )
{
	if ( pszStr == NULL )
		return NULL;

	#if defined( _UNICODE )
	{	int len = (int)strlen( pszStr );
		TCHAR* result = ( TCHAR* )alloca(( len+1 )*sizeof( TCHAR ));
		MultiByteToWideChar( LangPackGetDefaultCodePage(), 0, pszStr, -1, result, len );
		result[len] = 0;
		return mir_wstrdup( TranslateW( result ));
	}
	#else
		return mir_strdup( Translate( pszStr ));
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////

LangPackMuuid* __fastcall LangPackLookupUuid( WPARAM wParam )
{
	int idx = (wParam >> 16) & 0xFFFF;
	return ( idx > 0 && idx <= lMuuids.getCount()) ? lMuuids[ idx-1 ] : NULL;
}

int LangPackMarkPluginLoaded( PLUGININFOEX* pInfo )
{
	LangPackMuuid tmp; tmp.muuid = pInfo->uuid;
	int idx = lMuuids.getIndex( &tmp );
	if ( idx == -1 )
		return 0;

	lMuuids[ idx ]->pInfo = pInfo;
	return (idx+1) << 16;
}

void LangPackDropUnusedItems( void )
{
	LangPackEntry *s = langPack.entry;
	bool bSortNeeded = false;

	for ( int i=0; i < langPack.entryCount; i++, s++ )
		if ( s->pMuuid != NULL && s->pMuuid->pInfo == NULL ) {
			s->pMuuid = NULL;
			bSortNeeded = true;
		}

	if ( bSortNeeded )
		qsort( langPack.entry, langPack.entryCount, sizeof(LangPackEntry), (int(*)(const void*,const void*))SortLangPackHashesProc);
}

/////////////////////////////////////////////////////////////////////////////////////////

int LoadLangPackModule(void)
{
	HANDLE hFind;
	TCHAR szSearch[MAX_PATH];
	WIN32_FIND_DATA fd;

	bModuleInitialized = TRUE;

	ZeroMemory(&langPack,sizeof(langPack));
	LoadLangPackServices();
	pathToAbsoluteT(_T("langpack_*.txt"), szSearch, NULL);
	hFind = FindFirstFile( szSearch, &fd );
	if( hFind != INVALID_HANDLE_VALUE ) {
		pathToAbsoluteT(fd.cFileName, szSearch, NULL);
		FindClose(hFind);
		LoadLangPack(szSearch);
	}
	return 0;
}

void UnloadLangPackModule()
{
	int i;

	if ( !bModuleInitialized ) return;

	for ( i=0; i < lMuuids.getCount(); i++ )
		mir_free( lMuuids[i] );
	lMuuids.destroy();

	for ( i=0; i < langPack.entryCount; i++ ) {
		mir_free(langPack.entry[i].local);
		mir_free(langPack.entry[i].wlocal);
	}
	if ( langPack.entryCount ) {
		mir_free(langPack.entry);
		langPack.entry=0;
		langPack.entryCount=0;
}	}
