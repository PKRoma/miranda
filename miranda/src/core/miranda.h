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

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

/**** memory.c *************************************************************************/

void*  mir_alloc( size_t );
void*  mir_calloc( size_t );
void*  mir_realloc( void* ptr, size_t );
void   mir_free( void* ptr );
char*  mir_strdup( const char* str );
WCHAR* mir_wstrdup( const WCHAR* str );

/**** utf.c ****************************************************************************/

char* Utf8Decode( char* str, wchar_t** ucs2 );
char* Utf8DecodeCP( char* str, int codepage, wchar_t** ucs2 );

char* Utf8Encode( const char* str );
char* Utf8EncodeCP( const char* src, int codepage );

char* Utf8EncodeUcs2( const wchar_t* str );

/**** langpack.c ***********************************************************************/

int    LangPackGetDefaultCodePage();
int    LangPackGetDefaultLocale();
TCHAR* LangPackPcharToTchar( const char* pszStr );
char*  LangPackTranslateString(const char *szEnglish, const int W);

TCHAR*   a2t( const char* str );
char*    u2a( const wchar_t* src );
wchar_t* a2u( const char* src );

/**** skinicons.c **********************************************************************/

HICON LoadIconEx(HINSTANCE hInstance, LPCTSTR lpIconName, BOOL bShared);
int ImageList_AddIcon_NotShared(HIMAGELIST hIml, LPCTSTR szResource);
int ImageList_ReplaceIcon_NotShared(HIMAGELIST hIml, int iIndex, HINSTANCE hInstance, LPCTSTR szResource);

int ImageList_AddIcon_IconLibLoaded(HIMAGELIST hIml, int iconId);
int ImageList_AddIcon_ProtoIconLibLoaded(HIMAGELIST hIml, const char* szProto, int iconId);
int ImageList_ReplaceIcon_IconLibLoaded(HIMAGELIST hIml, int nIndex, HICON hIcon);

void Button_SetIcon_IcoLib(HWND hDlg, int itemId, int iconId, const char* tooltip);
void Button_FreeIcon_IcoLib(HWND hDlg, int itemId);

void Window_SetIcon_IcoLib(HWND hWnd, int iconId);
void Window_SetProtoIcon_IcoLib(HWND hWnd, const char* szProto, int iconId);
void Window_FreeIcon_IcoLib(HWND hWnd);

#define IconLib_ReleaseIcon(hIcon, szName) CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, (LPARAM)szName)
#define Safe_DestroyIcon(hIcon) if (hIcon) DestroyIcon(hIcon)
