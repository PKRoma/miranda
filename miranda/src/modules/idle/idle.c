/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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

#define IDLEMODULE "Idle"

// globals
static BOOL (WINAPI * MyGetLastInputInfo)(PLASTINPUTINFO);
static UINT hIdleTimer = 0;
static HANDLE hIdleSettingsChanged = NULL;
static HANDLE hIdleEvent = NULL;

// live settings
static int idleCheck = 0;
static int idleMethod = 0;
static int idleTimeFirst = 5; // this is in mins
static int idleTimeSecond = 10; // 
static int idleTimeFirstOn = 0;
static int idleTimeSecondOn = 0;
static int idleOnSaver = 0;
static int idleOnLock = 0;
static int idleGLI = 1;
static int idlePrivate = 0;

// db settings keys
#define IDL_IDLECHECK "IdleCheck"
#define IDL_IDLEMETHOD "IdleMethod"
#define IDL_IDLEGLI		"IdleGLI"
#define IDL_IDLETIME1ST "IdleTime1st"
#define IDL_IDLETIME2ND "IdleTime2nd"
#define IDL_IDLEONSAVER "IdleOnSaver"
#define IDL_IDLEONLOCK "IdleOnLock"
#define IDL_IDLETIME1STON "IdleTime1stOn"
#define IDL_IDLETIME2NDON "IdleTime2ndOn"
#define IDL_IDLEPRIVATE "IdlePrivate"


static BOOL CALLBACK DlgProcIdleOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG: 
		{
			TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_IDLECHECK, idleCheck ? BST_CHECKED : BST_UNCHECKED);			
			// check/uncheck options
			CheckDlgButton(hwndDlg, IDC_IDLEONWINDOWS, idleMethod == 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_IDLEONMIRANDA, idleMethod == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_IDLEUSEGLI, idleGLI ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SCREENSAVER, idleOnSaver ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_LOCKED, idleOnLock ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_IDLESHORT, idleTimeFirstOn ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_IDLELONG, idleTimeSecondOn ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_IDLEPRIVATE, idlePrivate ? BST_CHECKED : BST_UNCHECKED);
			// set times
			SetDlgItemInt(hwndDlg, IDC_IDLE1STTIME, idleTimeFirst, FALSE);
			SetDlgItemInt(hwndDlg, IDC_IDLE2NDTIME, idleTimeSecond, FALSE);
			// enable options
			SendMessage(hwndDlg, WM_USER+1, (WPARAM)idleCheck, 0);
			return TRUE;
		}
		case WM_COMMAND:
		{
            if ((LOWORD(wParam)==IDC_IDLE1STTIME||LOWORD(wParam)==IDC_IDLE2NDTIME) && (HIWORD(wParam)!=EN_CHANGE || (HWND) lParam!=GetFocus()))
                return 0;
			switch ( LOWORD(wParam) )
			{
				case IDC_IDLECHECK:
				case IDC_IDLEONWINDOWS:
				case IDC_IDLEONMIRANDA:
				case IDC_IDLESHORT:
				case IDC_IDLELONG:
				{
					SendMessage(hwndDlg, WM_USER+1, (WPARAM)IsDlgButtonChecked(hwndDlg, IDC_IDLECHECK) == BST_CHECKED, 0);
					break;
				}
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_USER+1: 
		{
			DWORD nID[] = { IDC_IDLEONWINDOWS, IDC_IDLEUSEGLI, IDC_IDLEONMIRANDA, IDC_SCREENSAVER, IDC_LOCKED, IDC_IDLESHORT, IDC_IDLE1STTIME, IDC_IDLELONG, IDC_IDLE2NDTIME, IDC_IDLEPRIVATE};
			int j;
			// enable/disable all sub controls,
			for (j = 0; j < sizeof(nID) / sizeof(nID[0]); j++) {
				int nEnable = wParam;
				switch ( nID[j] ) {
					case IDC_IDLEUSEGLI: nEnable &= IsDlgButtonChecked(hwndDlg, IDC_IDLEONWINDOWS) == BST_CHECKED ? 1 : 0; break;
					case IDC_IDLE1STTIME: nEnable &= IsDlgButtonChecked(hwndDlg, IDC_IDLESHORT) == BST_CHECKED ? 1 : 0; break;
					case IDC_IDLE2NDTIME: nEnable &= IsDlgButtonChecked(hwndDlg, IDC_IDLELONG) == BST_CHECKED ? 1 : 0; break;
				}
				EnableWindow(GetDlgItem(hwndDlg, nID[j]), nEnable);				
			}			
			break;
		}
		case WM_NOTIFY:
		{
			if ( lParam && ((LPNMHDR)lParam)->code == PSN_APPLY ) {
				// these writes will cause ME_DB_CONTACT_SETTINGCHANGED and that will set new live vars
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLECHECK, IsDlgButtonChecked(hwndDlg, IDC_IDLECHECK) == BST_CHECKED ? 1 : 0);
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLEMETHOD, IsDlgButtonChecked(hwndDlg, IDC_IDLEONWINDOWS) == BST_CHECKED ? 0 : 1);
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLEMETHOD, IsDlgButtonChecked(hwndDlg, IDC_IDLEONMIRANDA) == BST_CHECKED ? 1 : 0);
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLEGLI, IsDlgButtonChecked(hwndDlg, IDC_IDLEUSEGLI) == BST_CHECKED ? 1 : 0);
				// options about instant idle
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLEONSAVER, IsDlgButtonChecked(hwndDlg, IDC_SCREENSAVER));
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLEONLOCK, IsDlgButtonChecked(hwndDlg, IDC_LOCKED));
				// options if short/long idle are enabled
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLETIME1STON, IsDlgButtonChecked(hwndDlg, IDC_IDLESHORT) == BST_CHECKED ? 1 : 0 );
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLETIME2NDON, IsDlgButtonChecked(hwndDlg, IDC_IDLELONG) == BST_CHECKED ? 1 : 0);
				// write out the time info
				{
					int num = 0;
					num = GetDlgItemInt(hwndDlg, IDC_IDLE1STTIME, NULL, FALSE);
					DBWriteContactSettingWord(NULL, IDLEMODULE, IDL_IDLETIME1ST, num ? num : 5);
					num = GetDlgItemInt(hwndDlg, IDC_IDLE2NDTIME, NULL, FALSE);
					DBWriteContactSettingWord(NULL, IDLEMODULE, IDL_IDLETIME2ND, num ? num : 5);
				}
				// private
				DBWriteContactSettingByte(NULL, IDLEMODULE, IDL_IDLEPRIVATE, IsDlgButtonChecked(hwndDlg, IDC_IDLEPRIVATE) == BST_CHECKED ? 1 : 0);
			}
			break;
		}
	}
	return FALSE;
}

static int IdleOptInit(WPARAM wParam, LPARAM lPAram)
{
	OPTIONSDIALOGPAGE odp;
	memset(&odp, 0, sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=100000000;
	odp.hInstance=GetModuleHandle(NULL);
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_IDLE);
	odp.pszGroup=Translate("Events");
	odp.pszTitle=Translate("Idle");
	odp.pfnDlgProc=DlgProcIdleOpts;
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	return 0;
}

// delphi code here http://www.swissdelphicenter.ch/torry/printcode.php?id=2048
static BOOL IsWorkstationLocked(void)
{
	BOOL rc=0;	
	HDESK hDesk = OpenDesktop("default", 0, FALSE, DESKTOP_SWITCHDESKTOP);
	if ( hDesk != 0 ) {
		rc = SwitchDesktop(hDesk) == FALSE;
		CloseDesktop(hDesk);
	}
	return rc;
}

// ticks every 2 seconds
static VOID CALLBACK IdleTimer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
	DWORD dwTick = GetTickCount();
	int isIdle[3] = {0};
	static int isEventFired[3] = {0};
	int j;

	if ( idleCheck == 0 ) return;

	if ( idleMethod == 0 ) {
		// use windows idle time
		if ( idleGLI && MyGetLastInputInfo != 0 ) {
			LASTINPUTINFO ii;
			memset(&ii,0,sizeof(ii));
			ii.cbSize=sizeof(ii);	
			if ( MyGetLastInputInfo(&ii) ) 	dwTick = ii.dwTime;			
		} else {
			// mouse check
			static int mouseIdle = 0;
			static POINT lastMousePos = {0};
			POINT pt;
			GetCursorPos(&pt);
			if ( pt.x != lastMousePos.x || pt.y != lastMousePos.y ) 
			{
				mouseIdle=0;
				lastMousePos=pt;
			}
			else mouseIdle += 2; // interval of timer
			if ( mouseIdle ) dwTick = GetTickCount() - (mouseIdle * 1000);
		}
	} else {
		// use miranda idle time
		CallService(MS_SYSTEM_GETIDLE, 0, (LPARAM)&dwTick);
	}

	// has the first idle elapsed?
	isIdle[0] = GetTickCount() - dwTick >= (DWORD)( idleTimeFirst * 60 * 1000 );
	// and the second?
	isIdle[1] = GetTickCount() - dwTick >= (DWORD)( idleTimeSecond * 60 * 1000 );

	if ( idleOnSaver ) { // check saver
		BOOL isScreenSaverRunning = FALSE;
		SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &isScreenSaverRunning, FALSE);
		isIdle[2] = isScreenSaverRunning;
	}

	// check workstation?
	if ( idleOnLock ) { // check station locked?
		isIdle[2] = IsWorkstationLocked();
	}

	for ( j = 0; j<3; j++ )
	{
		int flags = ( idlePrivate ? IDF_PRIVACY:0 );
		switch (j) {
			case 0: flags |= IDF_SHORT; break;
			case 1: flags |= IDF_LONG; break;
			case 2: flags |= IDF_ONFORCE; break;
		}
		if ( isIdle[j]==1  && isEventFired[j] == 0 ) { // idle and no one knows					
			isEventFired[j]=1;
			NotifyEventHooks( hIdleEvent, 0, IDF_ISIDLE | flags );			
		}
		if ( isIdle[j]==0 && isEventFired[j] == 1 ) { // not idle, no one knows
			isEventFired[j]=0;
			NotifyEventHooks( hIdleEvent, 0, flags );			
		}
	}//for	

}

static int IdleSettingsChanged(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	DBCONTACTWRITESETTING * dbw = (DBCONTACTWRITESETTING *) lParam;
	if ( wParam == 0 && dbw && dbw->szSetting && strcmp(IDLEMODULE, dbw->szModule)==0 ) 
	{
		if ( strcmp(IDL_IDLECHECK, dbw->szSetting) == 0 ) 
		{
			idleCheck = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLEMETHOD, dbw->szSetting) == 0 ) 
		{

			idleMethod = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLEGLI, dbw->szSetting) == 0 ) {

			idleGLI = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLETIME1ST, dbw->szSetting) == 0 ) 
		{

			idleTimeFirst = dbw->value.wVal;

		} else if ( strcmp(IDL_IDLETIME2ND, dbw->szSetting) == 0 ) 
		{

			idleTimeSecond = dbw->value.wVal;

		} else if ( strcmp(IDL_IDLEONSAVER, dbw->szSetting) == 0 ) {

			idleOnSaver = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLEONLOCK, dbw->szSetting) == 0 ) {

			idleOnLock = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLETIME1STON, dbw->szSetting) == 0 ) {

			idleTimeFirstOn = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLETIME2NDON, dbw->szSetting) == 0 ) {

			idleTimeSecondOn = dbw->value.bVal;

		} else if ( strcmp(IDL_IDLEPRIVATE, dbw->szSetting) == 0 ) {

			idlePrivate = dbw->value.bVal;

		}

	}
	return 0;
}

static int IdleGetInfo(WPARAM wParam, LPARAM lParam)
{
	MIRANDA_IDLE_INFO * mii = (MIRANDA_IDLE_INFO *) lParam;
	if (mii && mii->cbSize == sizeof(MIRANDA_IDLE_INFO)) {
		mii->enabled = idleCheck;
		mii->idleShortTime = idleTimeFirstOn ? idleTimeFirst : 0;
		mii->idleLongTime = idleTimeSecondOn ? idleTimeSecond : 0;
		mii->privacy = idlePrivate;
		return 0;
	}
	return 1;
}

static int UnloadIdleModule(WPARAM wParam, LPARAM lParam)
{
	KillTimer(NULL, hIdleTimer);
	if ( hIdleSettingsChanged != 0 ) UnhookEvent(hIdleSettingsChanged);
	DestroyHookableEvent(ME_IDLE_CHANGED);
	return 0;
}

int LoadIdleModule(void)
{
	hIdleEvent = CreateHookableEvent(ME_IDLE_CHANGED);
	HookEvent(ME_SYSTEM_SHUTDOWN, UnloadIdleModule);	
	MyGetLastInputInfo=(BOOL (WINAPI *)(PLASTINPUTINFO)) GetProcAddress( GetModuleHandle("user32"),"GetLastInputInfo" );
	// load settings into live ones
	idleCheck = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLECHECK, 0);
	idleMethod = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLEMETHOD, 0);
	idleGLI = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLEGLI, 1);
	idleTimeFirst = DBGetContactSettingWord(NULL, IDLEMODULE, IDL_IDLETIME1ST, 5);
	idleTimeSecond = DBGetContactSettingWord(NULL, IDLEMODULE, IDL_IDLETIME2ND, 10);
	idleTimeFirstOn = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLETIME1STON, 0);
	idleTimeSecondOn = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLETIME2NDON, 0);
	idleOnSaver = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLEONSAVER, 0);
	idleOnLock = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLEONLOCK, 0);	
	idlePrivate = DBGetContactSettingByte(NULL, IDLEMODULE, IDL_IDLEPRIVATE, 0);
	CreateServiceFunction(MS_IDLE_GETIDLEINFO, IdleGetInfo);
	hIdleSettingsChanged=HookEvent(ME_DB_CONTACT_SETTINGCHANGED, IdleSettingsChanged);
	hIdleTimer=SetTimer(NULL, 0, 2000, IdleTimer);
	HookEvent(ME_OPT_INITIALISE, IdleOptInit);
	return 0;
}
