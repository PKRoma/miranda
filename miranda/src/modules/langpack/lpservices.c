/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
#include "../../core/commonheaders.h"

char *LangPackTranslateString(const char *szEnglish, const int W);

static int TranslateString(WPARAM wParam,LPARAM lParam)
{
    return (int)LangPackTranslateString((const char *)lParam, (wParam & LANG_UNICODE) ? 1 : 0);
}

static int TranslateMenu(WPARAM wParam,LPARAM lParam)
{
	HMENU hMenu=(HMENU)wParam;
	int i;
	MENUITEMINFOA mii;
	wchar_t str[256];
	int isUnicode = (lParam & LANG_UNICODE) ? 1 : 0;

	mii.cbSize = MENUITEMINFO_V4_SIZE;
	for ( i = GetMenuItemCount( hMenu )-1; i >= 0; i--) {
		mii.fMask = MIIM_TYPE|MIIM_SUBMENU;
		mii.dwTypeData = ( char* )str;
		mii.cch = SIZEOF(str);
		GetMenuItemInfoA(hMenu, i, TRUE, &mii);
		if ( mii.cch && mii.dwTypeData ) {
			int doUnicode = isUnicode;
			char* result = LangPackTranslateString( mii.dwTypeData, isUnicode );
			if ( result == mii.dwTypeData )
				doUnicode = 0;

			mii.dwTypeData = result;
			mii.fMask = MIIM_TYPE;
			if ( doUnicode )
				SetMenuItemInfoW( hMenu, i, TRUE, ( MENUITEMINFOW* )&mii );
			else
				SetMenuItemInfoA( hMenu, i, TRUE, ( MENUITEMINFOA* )&mii );
		}

		if ( mii.hSubMenu!=NULL ) TranslateMenu((WPARAM)mii.hSubMenu, lParam);
	}
	return 0;
}

static void TranslateWindow( HWND hwnd, int flags )
{
	char title[2048];
	GetWindowTextA(hwnd, title, sizeof( title ));
	{
		char* result = LangPackTranslateString(title, flags);
		if (( flags & LANG_UNICODE ) && result != title )
			SetWindowTextW(hwnd, ( WCHAR* )result );
		else
			SetWindowTextA(hwnd, result );
}	}

static BOOL CALLBACK TranslateDialogEnumProc(HWND hwnd,LPARAM lParam)
{
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	TCHAR szClass[32];
	int i,id = GetDlgCtrlID( hwnd );

	if(lptd->ignoreControls != NULL)
		for(i=0;lptd->ignoreControls[i];i++) if(lptd->ignoreControls[i]==id) return TRUE;

	GetClassName(hwnd,szClass,sizeof(szClass));
	if(!lstrcmpi(szClass,_T("static")) || !lstrcmpi(szClass,_T("hyperlink")) || !lstrcmpi(szClass,_T("button")) || !lstrcmpi(szClass,_T("MButtonClass")))
		TranslateWindow(hwnd, lptd->flags);
	else if(!lstrcmpi(szClass,_T("edit"))) {
		if(lptd->flags&LPTDF_NOIGNOREEDIT || GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY)
			TranslateWindow(hwnd, lptd->flags);
	}
	return TRUE;
}

static int TranslateDialog(WPARAM wParam,LPARAM lParam)
{
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	if(lptd==NULL||lptd->cbSize!=sizeof(LANGPACKTRANSLATEDIALOG)) return 1;
	if(!(lptd->flags&LPTDF_NOTITLE))
		TranslateWindow( lptd->hwndDlg, wParam );

	if ( wParam & LANG_UNICODE )
		lptd->flags += LANG_UNICODE;

	EnumChildWindows(lptd->hwndDlg,TranslateDialogEnumProc,lParam);
	return 0;
}

int LoadLangPackServices(void)
{
	CreateServiceFunction(MS_LANGPACK_TRANSLATESTRING,TranslateString);
	CreateServiceFunction(MS_LANGPACK_TRANSLATEMENU,TranslateMenu);
	CreateServiceFunction(MS_LANGPACK_TRANSLATEDIALOG,TranslateDialog);
	return 0;
}

