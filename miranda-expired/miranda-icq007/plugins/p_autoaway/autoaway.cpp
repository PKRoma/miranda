/*
Easytalk: the free text to speech dll for MS Windows
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/

#include <windows.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <objbase.h>
#include <objerror.h>
#include <ole2ver.h>
#include <commctrl.h>

#include "resource.h"

#include "..\..\miranda32\miranda.h"
#include "..\..\miranda32\pluginapi.h"

#pragma data_seg("shared")
int		autoAway	= 1;
int		autoNA		= 1;
int		awayTM		= 10;
int		naTM		= 20;
int		cnt			= 0;
int		setaway		= 0;
HWND	gAppWnd		= NULL;
POINT	pt			= {0};
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")

DWORD		threadid;
HANDLE		evt;
HHOOK		khook;
HHOOK		mhook;
PI_CALLBACK	picb = {0};
HINSTANCE	ghInstance = NULL;

char szTitle[MAX_PLUG_TITLE] = "Autoaway";

HKEY GetAppRegKey(void);

static LRESULT CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hwndDlg, IDC_AWAYTM, awayTM, TRUE);
			SetDlgItemInt(hwndDlg, IDC_NATM, naTM, TRUE);

			if (autoAway) CheckDlgButton(hwndDlg, IDC_AWAY, BST_CHECKED);
			if (autoNA)   CheckDlgButton(hwndDlg, IDC_NA,   BST_CHECKED);

			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK:
				awayTM = GetDlgItemInt(hwndDlg,IDC_AWAYTM, NULL, TRUE);
				naTM   = GetDlgItemInt(hwndDlg, IDC_NATM, NULL, TRUE);

				autoAway = IsDlgButtonChecked(hwndDlg, IDC_AWAY) == BST_CHECKED;
				autoNA = IsDlgButtonChecked(hwndDlg, IDC_NA) == BST_CHECKED;

				picb.pSetPluginInt("Autoaway", (int)autoAway, "autoaway");
				picb.pSetPluginInt("Autoaway", (int)autoNA,   "autona");
				picb.pSetPluginInt("Autoaway", (int)awayTM,   "awaytm");
				picb.pSetPluginInt("Autoaway", (int)naTM,     "natm");
				break;				
			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				break;
			}
			break;
	}
	return FALSE;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	cnt = 0;
	if (setaway)
	{
		SendMessage(gAppWnd, WM_COMMAND, 40003, 0);
		setaway = 0;
	}
	return CallNextHookEx(khook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (wParam == WM_MOUSEMOVE && 
		(pt.x != ((MOUSEHOOKSTRUCT*)lParam)->pt.x ||
		 pt.y != ((MOUSEHOOKSTRUCT*)lParam)->pt.y))
	{
		cnt = 0;
		if (setaway)
		{
			SendMessage(gAppWnd, WM_COMMAND, 40003, 0);
			setaway = 0;
		}
	}
	pt = ((MOUSEHOOKSTRUCT*)lParam)->pt;
	return CallNextHookEx(mhook, nCode, wParam, lParam);
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	while (WaitForSingleObject(evt, 60 * 1000) == WAIT_TIMEOUT)
	{		
		cnt++;

		int status = picb.pGetStatus();

		if (status == STATUS_ONLINE && autoAway && awayTM == cnt)
		{
			SendMessage(gAppWnd, WM_COMMAND, 40005, 0);
			setaway = 1;
		}
		if ((status == STATUS_AWAY || status == STATUS_ONLINE) && autoNA && naTM == cnt)
		{
			SendMessage(gAppWnd, WM_COMMAND, 40007, 0);
			setaway = 1;
		}
	}
	return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Load(HWND hwnd, HINSTANCE hinst, char *title)
{	
	strcpy(title, szTitle);
	gAppWnd = hwnd;

	return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Unload(void)
{
	SetEvent(evt);

	Sleep(100);

	CloseHandle(evt);

	UnhookWindowsHookEx(khook);
	UnhookWindowsHookEx(mhook);		

	return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Notify(long msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case PM_SHOWOPTIONS:
			DialogBox(ghInstance, MAKEINTRESOURCE(IDD_MAIN), gAppWnd, (DLGPROC)MainDlgProc);
			break;
		case PM_STATUSCHANGE:
			break;
		case PM_CONTACTSTATUSCHANGE:
			break;
		case PM_GOTMESSAGE:
			break;
		case PM_GOTURL:
			break;
		case PM_ICQDEBUGMSG:
			break;
		case PM_REGCALLBACK:
			memcpy(&picb, (void*)wParam, sizeof(picb));
			break;
		case PM_START:
			picb.pGetPluginInt("Autoaway", (int*)&autoAway, "autoaway", 1);
			picb.pGetPluginInt("Autoaway", (int*)&autoNA,   "autona",   1);
			picb.pGetPluginInt("Autoaway", (int*)&awayTM,   "awaytm",   10);
			picb.pGetPluginInt("Autoaway", (int*)&naTM,     "natm",     20);

			evt = CreateEvent(NULL, FALSE, FALSE, "mirandaICQautoawayQuit");

			khook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, ghInstance, 0);
			mhook = SetWindowsHookEx(WH_MOUSE,	  MouseProc	  , ghInstance, 0);	

			CreateThread(NULL, 0, ThreadProc, evt, 0, &threadid);
			break;
		case PM_SAVENOW:
			picb.pSetPluginInt("Autoaway", (int)autoAway, "autoaway");
			picb.pSetPluginInt("Autoaway", (int)autoNA,   "autona");
			picb.pSetPluginInt("Autoaway", (int)awayTM,   "awaytm");
			picb.pSetPluginInt("Autoaway", (int)naTM,     "natm");
			break;
	}
	return FALSE;
}

BOOL APIENTRY DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) 
		ghInstance = (HINSTANCE)hinstDLL;
	
    return TRUE;
}