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

char *LangPackTranslateString(const char *szEnglish);

static int TranslateString(WPARAM wParam,LPARAM lParam)
{
	return (int)LangPackTranslateString((const char *)lParam);
}

static int TranslateMenu(WPARAM wParam,LPARAM lParam)
{
	HMENU hMenu=(HMENU)wParam;
	int i;
	MENUITEMINFO mii;
	char str[256];

	mii.cbSize=MENUITEMINFO_V4_SIZE;
	for(i=GetMenuItemCount(hMenu)-1;i>=0;i--) {
		mii.fMask=MIIM_TYPE|MIIM_SUBMENU;
		mii.dwTypeData=str;
		mii.cch=sizeof(str);
		GetMenuItemInfo(hMenu,i,TRUE,&mii);
		if(mii.cch&&mii.dwTypeData) {
			mii.dwTypeData=LangPackTranslateString(mii.dwTypeData);
			mii.fMask=MIIM_TYPE;
			SetMenuItemInfo(hMenu,i,TRUE,&mii);
		}
		if(mii.hSubMenu!=NULL) TranslateMenu((WPARAM)mii.hSubMenu,0);
	}
	return 0;
}

static BOOL CALLBACK TranslateDialogEnumProc(HWND hwnd,LPARAM lParam)
{
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	char title[2048],szClass[32];
	int i,id=GetDlgCtrlID(hwnd);
	
	if(lptd->ignoreControls!=NULL)
		for(i=0;lptd->ignoreControls[i];i++) if(lptd->ignoreControls[i]==id) return TRUE;
	GetClassName(hwnd,szClass,sizeof(szClass));
	if(!lstrcmpi(szClass,"static") || !lstrcmpi(szClass,"hyperlink") || !lstrcmpi(szClass,"button") || !strcmpi(szClass,"MButtonClass")) {
		GetWindowText(hwnd,title,sizeof(title));
		SetWindowText(hwnd,LangPackTranslateString(title));
	}
	else if(!lstrcmpi(szClass,"edit")) {
		if(lptd->flags&LPTDF_NOIGNOREEDIT || GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY) {
			GetWindowText(hwnd,title,sizeof(title));
			SetWindowText(hwnd,LangPackTranslateString(title));
		}
	}
	return TRUE;
}

static int TranslateDialog(WPARAM wParam,LPARAM lParam)
{
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	if(lptd->cbSize!=sizeof(LANGPACKTRANSLATEDIALOG)) return 1;
	if(!(lptd->flags&LPTDF_NOTITLE)) {
		char title[256];
		GetWindowText(lptd->hwndDlg,title,sizeof(title));
		SetWindowText(lptd->hwndDlg,LangPackTranslateString(title));
	}
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