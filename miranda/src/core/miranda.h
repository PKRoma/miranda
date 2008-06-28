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

#include "m_protoint.h"

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

typedef HDESK (WINAPI* pfnOpenInputDesktop)( DWORD, BOOL, DWORD );
extern pfnOpenInputDesktop openInputDesktop;

typedef HDESK (WINAPI* pfnCloseDesktop)( HDESK );
extern pfnCloseDesktop closeDesktop;

/**** memory.c *************************************************************************/

#ifdef _STATIC
void*  mir_alloc( size_t );
void*  mir_calloc( size_t );
void*  mir_realloc( void* ptr, size_t );
void   mir_free( void* ptr );
char*  mir_strdup( const char* str );
WCHAR* mir_wstrdup( const WCHAR* str );

int    mir_snprintf(char *buffer, size_t count, const char* fmt, ...);
int    mir_sntprintf(TCHAR *buffer, size_t count, const TCHAR* fmt, ...);
int    mir_vsnprintf(char *buffer, size_t count, const char* fmt, va_list va);
int    mir_vsntprintf(TCHAR *buffer, size_t count, const TCHAR* fmt, va_list va);

WCHAR* mir_a2u_cp(const char* src, int codepage);
WCHAR* mir_a2u(const char* src);
char*  mir_u2a_cp(const wchar_t* src, int codepage);
char*  mir_u2a( const wchar_t* src);
#endif

/**** miranda.c ************************************************************************/

extern HINSTANCE hUser32;

/**** modules.c ************************************************************************/

void KillModuleEventHooks( HINSTANCE );
void KillModuleServices( HINSTANCE );

void KillObjectEventHooks( void* pObject );
void KillObjectServices( void* pObject );
void KillObjectThreads( void* pObject );

/**** utf.c ****************************************************************************/

char* Utf8Decode( char* str, wchar_t** ucs2 );
char* Utf8DecodeCP( char* str, int codepage, wchar_t** ucs2 );

wchar_t* Utf8DecodeUcs2( const char* str );

char* Utf8Encode( const char* str );
char* Utf8EncodeCP( const char* src, int codepage );

char* Utf8EncodeUcs2( const wchar_t* str );

/**** langpack.c ***********************************************************************/

int    LangPackGetDefaultCodePage();
int    LangPackGetDefaultLocale();
TCHAR* LangPackPcharToTchar( const char* pszStr );
char*  LangPackTranslateString(const char *szEnglish, const int W);

TCHAR*   a2t( const char* str );
char*    t2a( const TCHAR* src );
char*    u2a( const wchar_t* src );
wchar_t* a2u( const char* src );

/**** skin2icons.c *********************************************************************/

HANDLE IcoLib_AddNewIcon( SKINICONDESC* sid );
HICON  IcoLib_GetIcon( const char* pszIconName );
HICON  IcoLib_GetIconByHandle( HANDLE hItem );
HANDLE IcoLib_IsManaged( HICON hIcon );
int    IcoLib_ReleaseIcon( HICON hIcon, char* szIconName );

/**** skinicons.c **********************************************************************/

HICON LoadSkinProtoIcon( const char* szProto, int status );
HICON LoadSkinIcon( int idx );
HANDLE GetSkinIconHandle( int idx );

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

/**** clistmenus.c **********************************************************************/

extern int statusModeList[ MAX_STATUS_COUNT ];
extern int skinIconStatusList[ MAX_STATUS_COUNT ];
extern int skinIconStatusFlags[ MAX_STATUS_COUNT ];

/**** protocols.c ***********************************************************************/

#define OFFSET_PROTOPOS 200
#define OFFSET_VISIBLE  400
#define OFFSET_ENABLED  600
#define OFFSET_NAME     800

typedef struct
{
	PROTOACCOUNT** items;
	int count, limit, increment;
	FSortFunc sortFunc;
}
	TAccounts;

extern TAccounts accounts;

PROTOACCOUNT* Proto_GetAccount( const char* accName );
PROTOCOLDESCRIPTOR* Proto_IsProtocolLoaded( const char* szProtoName );

PROTO_INTERFACE* AddDefaultAccount( const char* szProtoName );
int  FreeDefaultAccount( PROTO_INTERFACE* ppi );

BOOL ActivateAccount( PROTOACCOUNT* pa );
void EraseAccount( PROTOACCOUNT* pa );
void DeactivateAccount( PROTOACCOUNT* pa );
void UnloadAccount( PROTOACCOUNT* pa, BOOL bIsDynamic );
void OpenAccountOptions( PROTOACCOUNT* pa );

void LoadDbAccounts( void );
void WriteDbAccounts( void );

int CallProtoServiceInt( HANDLE hContact, const char* szModule, const char* szService, WPARAM, LPARAM );
int CallContactService( HANDLE hContact, const char *szProtoService, WPARAM, LPARAM );

__inline static int CallProtoService( const char* szModule, const char* szService, WPARAM wParam, LPARAM lParam )
{
	return CallProtoServiceInt( NULL, szModule, szService, wParam, lParam );
}
