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

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

typedef HMONITOR (WINAPI *pfnMyMonitorFromPoint)(POINT, DWORD);
extern pfnMyMonitorFromPoint MyMonitorFromPoint;

typedef HMONITOR (WINAPI *pfnMyMonitorFromRect)(LPCRECT, DWORD);
extern pfnMyMonitorFromRect MyMonitorFromRect;

typedef HMONITOR(WINAPI *pfnMyMonitorFromWindow) (HWND, DWORD);
extern pfnMyMonitorFromWindow MyMonitorFromWindow;

typedef BOOL(WINAPI *pfnMyGetMonitorInfo) (HMONITOR, LPMONITORINFO);
extern pfnMyGetMonitorInfo MyGetMonitorInfo;

typedef HRESULT (STDAPICALLTYPE *pfnSHAutoComplete)(HWND,DWORD);
extern pfnSHAutoComplete shAutoComplete;

typedef HRESULT (STDAPICALLTYPE *pfnSHGetSpecialFolderPathA)(HWND, LPSTR,  int, BOOL );
typedef HRESULT (STDAPICALLTYPE *pfnSHGetSpecialFolderPathW)(HWND, LPWSTR, int, BOOL );
extern pfnSHGetSpecialFolderPathA shGetSpecialFolderPathA;
extern pfnSHGetSpecialFolderPathW shGetSpecialFolderPathW;

#ifdef _UNICODE
#define shGetSpecialFolderPath shGetSpecialFolderPathW
#else
#define shGetSpecialFolderPath shGetSpecialFolderPathA
#endif

typedef HDESK (WINAPI* pfnOpenInputDesktop)(DWORD, BOOL, DWORD);
extern pfnOpenInputDesktop openInputDesktop;

typedef HDESK (WINAPI* pfnCloseDesktop)(HDESK);
extern pfnCloseDesktop closeDesktop;

typedef BOOL (WINAPI* pfnAnimateWindow)(HWND, DWORD, DWORD);
extern pfnAnimateWindow animateWindow;

typedef BOOL (WINAPI * pfnSetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
extern pfnSetLayeredWindowAttributes setLayeredWindowAttributes;

typedef HTHEME  ( STDAPICALLTYPE *pfnOpenThemeData )( HWND, LPCWSTR );
typedef HRESULT ( STDAPICALLTYPE *pfnIsThemeBackgroundPartiallyTransparent )( HTHEME, int, int );
typedef HRESULT ( STDAPICALLTYPE *pfnDrawThemeParentBackground )( HWND, HDC, const RECT * );
typedef HRESULT ( STDAPICALLTYPE *pfnDrawThemeBackground )( HTHEME, HDC, int, int, const RECT *, const RECT * );
typedef HRESULT ( STDAPICALLTYPE *pfnDrawThemeText)( HTHEME, HDC, int, int, LPCWSTR, int, DWORD, DWORD, const RECT *);
typedef HRESULT ( STDAPICALLTYPE *pfnDrawThemeTextEx)( HTHEME, HDC, int, int, LPCWSTR, int, DWORD, LPRECT, const struct _DTTOPTS * );
typedef HRESULT ( STDAPICALLTYPE *pfnGetThemeBackgroundContentRect)( HTHEME, HDC, int, int, LPCRECT, LPRECT );
typedef HRESULT ( STDAPICALLTYPE *pfnGetThemeFont)( HTHEME, HDC, int, int, int, LOGFONT * );
typedef HRESULT ( STDAPICALLTYPE *pfnCloseThemeData )( HTHEME );
typedef HRESULT ( STDAPICALLTYPE *pfnEnableThemeDialogTexture )( HWND hwnd, DWORD dwFlags );
typedef HRESULT ( STDAPICALLTYPE *pfnSetWindowTheme )( HWND, LPCWSTR, LPCWSTR );
typedef HRESULT ( STDAPICALLTYPE *pfnSetWindowThemeAttribute )( HWND, enum WINDOWTHEMEATTRIBUTETYPE, PVOID, DWORD );
typedef BOOL ( STDAPICALLTYPE *pfnIsThemeActive )();

extern pfnOpenThemeData openThemeData;
extern pfnIsThemeBackgroundPartiallyTransparent isThemeBackgroundPartiallyTransparent;
extern pfnDrawThemeParentBackground drawThemeParentBackground;
extern pfnDrawThemeBackground drawThemeBackground;
extern pfnDrawThemeText drawThemeText;
extern pfnDrawThemeTextEx drawThemeTextEx;
extern pfnGetThemeBackgroundContentRect getThemeBackgroundContentRect;
extern pfnGetThemeFont getThemeFont;
extern pfnCloseThemeData closeThemeData;
extern pfnEnableThemeDialogTexture enableThemeDialogTexture;
extern pfnSetWindowTheme setWindowTheme;
extern pfnSetWindowThemeAttribute setWindowThemeAttribute;
extern pfnIsThemeActive isThemeActive;

extern ITaskbarList3 * pTaskbarInterface;

typedef HRESULT ( STDAPICALLTYPE *pfnDwmExtendFrameIntoClientArea )( HWND hwnd, const MARGINS *margins );
typedef HRESULT ( STDAPICALLTYPE *pfnDwmIsCompositionEnabled )( BOOL * );

extern pfnDwmExtendFrameIntoClientArea dwmExtendFrameIntoClientArea;
extern pfnDwmIsCompositionEnabled dwmIsCompositionEnabled;

/**** file.c ***************************************************************************/

void PushFileEvent( HANDLE hContact, HANDLE hdbe, LPARAM lParam );

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

extern HINSTANCE hMirandaInst;
extern pfnExceptionFilter pMirandaExceptFilter;

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

__forceinline char* Utf8DecodeA(const char* src)
{
    char* tmp = mir_strdup(src);
    Utf8Decode(tmp, NULL);
    return tmp;
}


char* Utf8Encode( const char* str );
char* Utf8EncodeCP( const char* src, int codepage );

char* Utf8EncodeUcs2( const wchar_t* str );

#if defined( _UNICODE )
	#define Utf8DecodeT Utf8DecodeUcs2
	#define Utf8EncodeT Utf8EncodeUcs2
#else
	#define Utf8DecodeT Utf8DecodeA
	#define Utf8EncodeT Utf8Encode
#endif

/**** langpack.c ***********************************************************************/

int    LangPackGetDefaultCodePage();
int    LangPackGetDefaultLocale();
TCHAR* LangPackPcharToTchar( const char* pszStr );
char*  LangPackTranslateString(const char *szEnglish, const int W);

/**** path.c ***************************************************************************/

int pathToAbsolute(const char *pSrc, char *pOut, char* base);
void CreatePathToFile( char* wszFilePath );
int CreateDirectoryTree(const char *szDir);
#if defined( _UNICODE )
	void CreatePathToFileW( WCHAR* wszFilePath );
	int CreateDirectoryTreeW(const WCHAR *szDir);
	int pathToAbsoluteW(const TCHAR *pSrc, TCHAR *pOut, TCHAR* base);
	#define pathToAbsoluteT pathToAbsoluteW
	#define CreatePathToFileT CreatePathToFileW
	#define CreateDirectoryTreeT CreateDirectoryTreeW
#else
	#define pathToAbsoluteT pathToAbsolute
	#define CreatePathToFileT CreatePathToFile
	#define CreateDirectoryTreeT CreateDirectoryTree
#endif

/**** skin2icons.c *********************************************************************/

HANDLE IcoLib_AddNewIcon( SKINICONDESC* sid );
HICON  IcoLib_GetIcon( const char* pszIconName, bool big );
HICON  IcoLib_GetIconByHandle( HANDLE hItem, bool big );
HANDLE IcoLib_IsManaged( HICON hIcon );
int    IcoLib_ReleaseIcon( HICON hIcon, char* szIconName, bool big );

/**** skinicons.c **********************************************************************/

HICON LoadSkinProtoIcon( const char* szProto, int status, bool big = false );
HICON LoadSkinIcon( int idx, bool big = false );
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

#define IconLib_ReleaseIcon(hIcon, szName) IcoLib_ReleaseIcon(hIcon, szName, false);
#define Safe_DestroyIcon(hIcon) if (hIcon) DestroyIcon(hIcon)

/**** clistmenus.c **********************************************************************/

extern HANDLE hMainMenuObject, hContactMenuObject, hStatusMenuObject;
extern HANDLE hPreBuildMainMenuEvent, hPreBuildContactMenuEvent;

extern const int statusModeList[ MAX_STATUS_COUNT ];
extern const int skinIconStatusList[ MAX_STATUS_COUNT ];
extern const int skinIconStatusFlags[ MAX_STATUS_COUNT ];

int TryProcessDoubleClick( HANDLE hContact );

/**** protocols.c ***********************************************************************/

#define OFFSET_PROTOPOS 200
#define OFFSET_VISIBLE  400
#define OFFSET_ENABLED  600
#define OFFSET_NAME     800

extern LIST<PROTOACCOUNT> accounts;

PROTOACCOUNT* Proto_GetAccount( const char* accName );
PROTOCOLDESCRIPTOR* Proto_IsProtocolLoaded( const char* szProtoName );

int Proto_IsAccountEnabled( PROTOACCOUNT* pa );
int Proto_IsAccountLocked( PROTOACCOUNT* pa );

PROTO_INTERFACE* AddDefaultAccount( const char* szProtoName );
int  FreeDefaultAccount( PROTO_INTERFACE* ppi );

BOOL ActivateAccount( PROTOACCOUNT* pa );
void EraseAccount( const char* pszProtoName );
void DeactivateAccount( PROTOACCOUNT* pa, bool bIsDynamic, bool bErase );
void UnloadAccount( PROTOACCOUNT* pa, bool bIsDynamic, bool bErase );
void OpenAccountOptions( PROTOACCOUNT* pa );

void LoadDbAccounts( void );
void WriteDbAccounts( void );

INT_PTR CallProtoServiceInt( HANDLE hContact, const char* szModule, const char* szService, WPARAM, LPARAM );
INT_PTR CallContactService( HANDLE hContact, const char *szProtoService, WPARAM, LPARAM );

__inline static INT_PTR CallProtoService( const char* szModule, const char* szService, WPARAM wParam, LPARAM lParam )
{
	return CallProtoServiceInt( NULL, szModule, szService, wParam, lParam );
}

/**** utils.c **************************************************************************/

#if defined( _UNICODE )
	char*  rtrim(char* str);
#endif
TCHAR* rtrim(TCHAR* str);
char*  ltrim(char* str);
__inline char* lrtrim(char* str) { return ltrim(rtrim(str)); };

bool wildcmp(char * name, char * mask);

#ifdef _UNICODE
class StrConvT
{
private:
	wchar_t* m_body;

public:
	StrConvT( const char* pSrc ) :
		m_body( mir_a2u( pSrc )) {}

    ~StrConvT() {  mir_free( m_body ); }
	operator const wchar_t* () const { return m_body; }
};

class StrConvA
{
private:
	char* m_body;

public:
	StrConvA( const wchar_t* pSrc ) :
		m_body( mir_u2a( pSrc )) {}

    ~StrConvA() {  mir_free( m_body ); }
	operator const char*  () const { return m_body; }
	operator const wchar_t* () const { return ( wchar_t* )m_body; }  // type cast to fake the interface definition
	operator const LPARAM () const { return ( LPARAM )m_body; }
};

#else

#define StrConvT( x ) x
#define StrConvA( x ) x

#endif

