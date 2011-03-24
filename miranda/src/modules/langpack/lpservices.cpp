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

#if defined( _UNICODE )
	#define FLAGS LANG_UNICODE
#else
	#define FLAGS 0
#endif

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR UuidTranslateString( WPARAM wParam, LPARAM lParam, LPARAM pVoid )
{
	return (INT_PTR)LangPackTranslateString(( LangPackMuuid* )pVoid, (const char *)lParam, (wParam & LANG_UNICODE) ? 1 : 0);
}

static INT_PTR TranslateString(WPARAM wParam,LPARAM lParam)
{
	return (INT_PTR)LangPackTranslateString( NULL, (const char *)lParam, (wParam & LANG_UNICODE) ? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR UuidTranslateMenu( WPARAM wParam, LPARAM lParam, LPARAM pVoid )
{
	HMENU        hMenu = ( HMENU )wParam;
	int          i;
	MENUITEMINFO mii;
	TCHAR        str[256];

	mii.cbSize = MENUITEMINFO_V4_SIZE;
	for ( i = GetMenuItemCount( hMenu )-1; i >= 0; i--) {
		mii.fMask = MIIM_TYPE|MIIM_SUBMENU;
		mii.dwTypeData = ( TCHAR* )str;
		mii.cch = SIZEOF(str);
		GetMenuItemInfo(hMenu, i, TRUE, &mii);

		if ( mii.cch && mii.dwTypeData ) {
			TCHAR* result = ( TCHAR* )LangPackTranslateString(( LangPackMuuid* )pVoid, ( const char* )mii.dwTypeData, FLAGS );
			if ( result != mii.dwTypeData ) {
				mii.dwTypeData = result;
				mii.fMask = MIIM_TYPE;
				SetMenuItemInfo( hMenu, i, TRUE, &mii );
		}	}

		if ( mii.hSubMenu != NULL ) UuidTranslateMenu(( WPARAM )mii.hSubMenu, lParam, pVoid);
	}
	return 0;
}

static INT_PTR TranslateMenu(WPARAM wParam,LPARAM lParam)
{
	return UuidTranslateMenu( wParam, lParam, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////

static void TranslateWindow( LangPackMuuid* pUuid, HWND hwnd )
{
	TCHAR title[2048];
	GetWindowText(hwnd, title, SIZEOF( title ));
	{
		TCHAR* result = ( TCHAR* )LangPackTranslateString( pUuid, ( const char* )title, FLAGS );
		if ( result != title )
			SetWindowText(hwnd, result );
}	}

struct LANGPACKTRANSLATEDIALOG2
{
	LANGPACKTRANSLATEDIALOG* lptd;
	LangPackMuuid* pUuid;
};

static BOOL CALLBACK TranslateDialogEnumProc(HWND hwnd,LPARAM lParam)
{
	LANGPACKTRANSLATEDIALOG2 *pParam = (LANGPACKTRANSLATEDIALOG2*)lParam;

	LANGPACKTRANSLATEDIALOG *lptd = pParam->lptd;
	TCHAR szClass[32];
	int i,id = GetDlgCtrlID( hwnd );

	if ( lptd->ignoreControls != NULL )
		for ( i=0; lptd->ignoreControls[i]; i++ )
			if ( lptd->ignoreControls[i] == id )
				return TRUE;

	GetClassName( hwnd, szClass, SIZEOF(szClass));
	if(!lstrcmpi(szClass,_T("static")) || !lstrcmpi(szClass,_T("hyperlink")) || !lstrcmpi(szClass,_T("button")) || !lstrcmpi(szClass,_T("MButtonClass")) || !lstrcmpi(szClass,_T("MHeaderbarCtrl")))
		TranslateWindow( pParam->pUuid, hwnd );
	else if ( !lstrcmpi( szClass,_T("edit"))) {
		if( lptd->flags & LPTDF_NOIGNOREEDIT || GetWindowLongPtr(hwnd,GWL_STYLE) & ES_READONLY )
			TranslateWindow( pParam->pUuid, hwnd );
	}
	return TRUE;
}

INT_PTR UuidTranslateDialog( WPARAM wParam, LPARAM lParam, LPARAM pVoid )
{
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	if ( lptd == NULL || lptd->cbSize != sizeof(LANGPACKTRANSLATEDIALOG))
		return 1;

	if ( !( lptd->flags & LPTDF_NOTITLE ))
		TranslateWindow(( LangPackMuuid* )pVoid, lptd->hwndDlg );

	LANGPACKTRANSLATEDIALOG2 lptd2 = { lptd, ( LangPackMuuid* )pVoid };
	EnumChildWindows( lptd->hwndDlg, TranslateDialogEnumProc, LPARAM( &lptd2 ));
	return 0;
}

static INT_PTR TranslateDialog(WPARAM wParam, LPARAM lParam)
{
	return UuidTranslateDialog( wParam, lParam, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR GetDefaultCodePage(WPARAM,LPARAM)
{
	return LangPackGetDefaultCodePage();
}

static INT_PTR GetDefaultLocale(WPARAM, LPARAM)
{
	return LangPackGetDefaultLocale();
}

static INT_PTR PcharToTchar(WPARAM, LPARAM lParam)
{
	return ( INT_PTR )LangPackPcharToTchar((char*)lParam );
}

int LoadLangPackServices(void)
{
	CreateServiceFunction(MS_LANGPACK_TRANSLATESTRING,TranslateString);
	CreateServiceFunction(MS_LANGPACK_TRANSLATEMENU,TranslateMenu);
	CreateServiceFunction(MS_LANGPACK_TRANSLATEDIALOG,TranslateDialog);
	CreateServiceFunction(MS_LANGPACK_GETCODEPAGE,GetDefaultCodePage);
	CreateServiceFunction(MS_LANGPACK_GETLOCALE,GetDefaultLocale);
	CreateServiceFunction(MS_LANGPACK_PCHARTOTCHAR,PcharToTchar);
	return 0;
}

