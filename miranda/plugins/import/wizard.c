/*

Import plugin for Miranda IM

Copyright (C) 2001-2005 Martin �berg, Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include <string.h>
#include "resource.h"
#include "import.h"
#include <m_database.h>
#include <m_findadd.h>
#include <m_protomod.h>

BOOL CALLBACK WizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK FinishedPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MirabilisPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MirandaPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ICQserverPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

extern HINSTANCE hInst;
BOOL IsProtocolLoaded(char* pszProtocolName);


BOOL CALLBACK ImportTypePageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch(message)
	{

	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		CheckDlgButton(hdlg, IDC_MIRANDA, BST_CHECKED);

		// Disable Mirabilis import if ICQ isn't loaded.
		if (!IsProtocolLoaded(ICQOSCPROTONAME))
		{
			EnableWindow(GetDlgItem(hdlg, IDC_MIRABILIS), FALSE);
		}

		return TRUE;
		
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BACK:
			PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_WIZARDINTRO, (LPARAM)WizardIntroPageProc);
			break;

		case IDOK:
			if (IsDlgButtonChecked(hdlg, IDC_MIRANDA))
			{
				PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_MIRANDADB, (LPARAM)MirandaPageProc);
			}
			else if (IsDlgButtonChecked(hdlg, IDC_MIRABILIS))
			{
				PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_MIRABILISDB, (LPARAM)MirabilisPageProc);
			}
			else if (IsDlgButtonChecked(hdlg, IDC_USEFINDADD))
			{
				CallService(MS_FINDADD_FINDADD, 0, 0);
				PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_FINISHED, (LPARAM)FinishedPageProc);
			}
			break;

		case IDCANCEL:
			PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
			break;

		}
		break;
		
	}

	return FALSE;

}


BOOL CALLBACK WizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch(message)
	{
		
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
		return TRUE;
		
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{

		case IDOK:
			PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_IMPORTTYPE, (LPARAM)ImportTypePageProc);
			break;
			
		case IDCANCEL:
			PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
			break;
			
		}
		break;
		
	}
	
	return FALSE;
	
}



BOOL CALLBACK FinishedPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch(message)
	{

	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
		SendMessage(GetParent(hdlg), WIZM_SETCANCELTEXT, 0, (LPARAM)Translate("Finish"));
		CheckDlgButton(hdlg, IDC_DONTLOADPLUGIN, BST_UNCHECKED);
		return TRUE;
		
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{

		case IDOK:
			PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_IMPORTTYPE, (LPARAM)ImportTypePageProc);
			break;

		case IDCANCEL:
			if(IsDlgButtonChecked(hdlg, IDC_DONTLOADPLUGIN))
			{			
				char sModuleFileName[MAX_PATH];
				char *pszFileName;
				
				GetModuleFileName(hInst, sModuleFileName, sizeof(sModuleFileName));
				pszFileName = strrchr(sModuleFileName, '\\' );
				if (pszFileName == NULL)
					pszFileName = sModuleFileName;
				else
					pszFileName++;
				// We must lower case here because if a DLL is loaded in two
				// processes, its file name from GetModuleFileName in one process may
				// differ in case from its file name in the other process. This will
				// prevent the plugin from disabling/enabling correctly (this fix relies
				// on the plugin loader to ignore case)
				CharLower(pszFileName);
				DBWriteContactSettingByte(NULL, "PluginDisable", pszFileName, 1);
			}
			PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
			break;

		}
		break;

	}
	
	return FALSE;
	
}



BOOL CALLBACK WizardDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	static HWND hwndPage;
	

	switch (message)
	{
		
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		hwndPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_WIZARDINTRO), hdlg, WizardIntroPageProc);
		SetWindowPos(hwndPage, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		ShowWindow(hwndPage, SW_SHOW);
		ShowWindow(hdlg, SW_SHOW);
		SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst,MAKEINTRESOURCE(IDI_IMPORT)));
		return TRUE;

	case WIZM_GOTOPAGE:
		DestroyWindow(hwndPage);
		EnableWindow(GetDlgItem(hdlg, IDC_BACK), TRUE);
		EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
		EnableWindow(GetDlgItem(hdlg, IDCANCEL), TRUE);
		SetDlgItemText(hdlg, IDCANCEL, Translate("Cancel"));
		hwndPage = CreateDialog(hInst, MAKEINTRESOURCE(wParam), hdlg, (DLGPROC)lParam);
		SetWindowPos(hwndPage, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		ShowWindow(hwndPage, SW_SHOW);
		break;

	case WIZM_DISABLEBUTTON:
		switch (wParam)
		{

		case 0:
			EnableWindow(GetDlgItem(hdlg, IDC_BACK), FALSE);
			break;

		case 1:
			EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
			break;

		case 2:
			EnableWindow(GetDlgItem(hdlg, IDCANCEL), FALSE);
			break;

		}
		break;

	case WIZM_ENABLEBUTTON:
		switch (wParam)
		{

		case 0:
			EnableWindow(GetDlgItem(hdlg, IDC_BACK), TRUE);
			break;

		case 1:
			EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
			break;

		case 2:
			EnableWindow(GetDlgItem(hdlg, IDCANCEL), TRUE);
			break;

		}
		break;

	case WIZM_SETCANCELTEXT:
		SetDlgItemText(hdlg, IDCANCEL, (char*)lParam);
		break;

	case WM_COMMAND:
		SendMessage(hwndPage, WM_COMMAND, wParam, lParam);
		break;

	case WM_CLOSE:
		DestroyWindow(hwndPage);
		DestroyWindow(hdlg);
		break;
		
	}
	
	return FALSE;

}
