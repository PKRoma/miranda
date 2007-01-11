/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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
#include <windows.h>
#include <newpluginapi.h>
#include <m_database.h>
#include <m_langpack.h>
#include "help.h"

#include <m_options.h>
#include "resource.h"

extern HINSTANCE hInst;

int GetStringOption(int id,char *pStr,int cbStr)
{
	char *pszSetting,*pszDefault;
	DBVARIANT dbv;
	char szDefault[128];

	switch(id) {
		case OPTION_SERVER:
			pszSetting="Server";
			pszDefault="miranda-icq.sourceforge.net";
			break;
		case OPTION_PATH:
			pszSetting="Path";
			pszDefault="/help/";
			break;
		case OPTION_LANGUAGE:
			pszSetting="Language";
			wsprintfA(szDefault,"%04x",PRIMARYLANGID(GetUserDefaultLangID()));
			pszDefault=szDefault;
			break;
#ifdef EDITOR
		case OPTION_AUTH:
			pszSetting="Auth";
			pszDefault="";
			break;
#endif
		default:
			return 1;
	}
	if(DBGetContactSetting(NULL,"HelpPlugin",pszSetting,&dbv))
		lstrcpynA(pStr,pszDefault,cbStr);
	else {
		lstrcpynA(pStr,dbv.pszVal,cbStr);
		DBFreeVariant(&dbv);
	}
	return 0;
}

static BOOL CALLBACK LangListDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			GetLanguageList(hwndDlg);
			return TRUE;
		case M_LANGLISTFAILED:
			SetDlgItemText(hwndDlg,IDC_STATUS,TranslateT("Download failed\n\nPlease check your Internet connection and try again."));
			SetDlgItemText(hwndDlg,IDOK,TranslateT("Retry"));
			EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
			break;
		case M_LANGLIST:
		{	int *langs=(int*)lParam;
			int i,iItem;
			char szName[64];
			int currentLang;

			GetStringOption(OPTION_LANGUAGE,szName,sizeof(szName));
			currentLang=strtol(szName,NULL,0x10);
			for(i=0;i<(int)wParam;i++) {
				GetLocaleInfoA(MAKELCID(MAKELANGID(langs[i],SUBLANG_NEUTRAL),SORT_DEFAULT),LOCALE_SLANGUAGE,szName,sizeof(szName));
				iItem=SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_ADDSTRING,0,(LPARAM)szName);
				SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_SETITEMDATA,iItem,langs[i]);
				if(langs[i]==currentLang) SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_SETCURSEL,iItem,0);
			}
			free(langs);
			if(SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_GETCURSEL,0,0)==-1) {
				for(i=SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_GETCOUNT,0,0)-1;i>=0;i--) {
					if(SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_GETITEMDATA,i,0)==LANG_ENGLISH) {
						SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_SETCURSEL,i,0);
						break;
					}
				}
				if(i<0) SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_SETCURSEL,0,0);
			}
			ShowWindow(GetDlgItem(hwndDlg,IDC_STATUS),SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg,IDC_LANGLIST),SW_SHOW);
			EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_LANGLIST:
					if(HIWORD(wParam)!=LBN_DBLCLK) break;
					//fall thru
				case IDOK:
					if(IsWindowVisible(GetDlgItem(hwndDlg,IDC_STATUS))) {
						SetDlgItemText(hwndDlg,IDC_STATUS,TranslateT("Downloading language list\n\nPlease wait..."));
						SetDlgItemText(hwndDlg,IDOK,TranslateT("OK"));
						EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
						GetLanguageList(hwndDlg);
						break;
					}
					{	char str[5];
						wsprintfA(str,"%04x",SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_LANGLIST,LB_GETCURSEL,0,0),0));
						DBWriteContactSettingString(NULL,"HelpPlugin","Language",str);
					}
					EndDialog(hwndDlg,IDOK);
					break;
				case IDCANCEL:
					EndDialog(hwndDlg,wParam);
					break;
			}
			break;
	}
	return FALSE;
}

#define M_UPDATELANGUAGEFIELD  (WM_USER+0x100)
static BOOL CALLBACK DlgProcEditorOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
#ifdef EDITOR
			{	char str[1024];
				GetStringOption(OPTION_SERVER,str,sizeof(str));
				SetDlgItemText(hwndDlg,IDC_SERVER,str);
				GetStringOption(OPTION_PATH,str,sizeof(str));
				SetDlgItemText(hwndDlg,IDC_PATH,str);
				GetStringOption(OPTION_AUTH,str,sizeof(str));
				SetDlgItemText(hwndDlg,IDC_AUTH,str);
			}
#endif
			SendMessage(hwndDlg,M_UPDATELANGUAGEFIELD,0,0);
			return TRUE;
		case M_UPDATELANGUAGEFIELD:
		{	char szLangId[5],szLangName[64];
			GetStringOption(OPTION_LANGUAGE,szLangId,sizeof(szLangId));
			GetLocaleInfoA(MAKELCID(MAKELANGID(strtol(szLangId,NULL,0x10),SUBLANG_NEUTRAL),SORT_DEFAULT),LOCALE_SLANGUAGE,szLangName,sizeof(szLangName));
#ifdef EDITOR
			{	char str[128];
				wsprintf(str,"%s (%s)",szLangName,szLangId);
				SetDlgItemText(hwndDlg,IDC_LANGUAGE,str);
			}
#else
			SetDlgItemTextA(hwndDlg,IDC_LANGUAGE,szLangName);
#endif
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_SERVER:
				case IDC_PATH:
				case IDC_LANGUAGE:
				case IDC_AUTH:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return TRUE;
					break;
				case IDC_LANGLIST:
					if(DialogBox(hInst,MAKEINTRESOURCE(IDD_LANGLIST),hwndDlg,LangListDlgProc)!=IDOK)
						return TRUE;
					SendMessage(hwndDlg,M_UPDATELANGUAGEFIELD,0,0);
					break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
#ifdef EDITOR
							{	char str[1024];
								GetDlgItemText(hwndDlg,IDC_SERVER,str,sizeof(str));
								DBWriteContactSettingString(NULL,"HelpPlugin","Server",str);
								GetDlgItemText(hwndDlg,IDC_PATH,str,sizeof(str));
								DBWriteContactSettingString(NULL,"HelpPlugin","Path",str);
								GetDlgItemText(hwndDlg,IDC_AUTH,str,sizeof(str));
								DBWriteContactSettingString(NULL,"HelpPlugin","Auth",str);
							}
#endif
							return TRUE;
					}
					break;
			}
			break;
	}
	return FALSE;
}

static int OptInitialise(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp={0};

	odp.cbSize=sizeof(odp);
	odp.position=910000000;
	odp.hInstance=hInst;
	odp.pszGroup=Translate("Plugins");
	odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_HELP);
#ifdef EDITOR
	odp.pszTitle=Translate("Help Editor");
#else
	odp.pszTitle=Translate("Help");
#endif
	odp.pfnDlgProc=DlgProcEditorOptions;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

int InitOptions(void)
{
	HookEvent(ME_OPT_INITIALISE,OptInitialise);
	return 0;
}