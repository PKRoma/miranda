/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2005 Miranda ICQ/IM project, 
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

#define IDLEMOD "Idle"
#define IDL_USERIDLECHECK "UserIdleCheck"
#define IDL_IDLEMETHOD    "IdleMethod"
#define IDL_IDLETIME1ST   "IdleTime1st"
#define IDL_IDLEONSAVER   "IdleOnSaver" // IDC_SCREENSAVER
#define IDL_IDLEONLOCK    "IdleOnLock" // IDC_LOCKED
#define IDL_IDLEPRIVATE   "IdlePrivate" // IDC_IDLEPRIVATE
#define IDL_IDLESTATUSLOCK "IdleStatusLock" // IDC_IDLESTATUSLOCK
#define IDL_AAENABLE      "AAEnable"
#define IDL_AASTATUS      "AAStatus"

#define IdleObject_IsIdle(obj) (obj->state&0x1)
#define IdleObject_SetIdle(obj) (obj->state|=0x1)
#define IdleObject_ClearIdle(obj) (obj->state&=~0x1) 

// either use meth 0,1 or figure out which one
#define IdleObject_UseMethod0(obj) (obj->state&=~0x2)
#define IdleObject_UseMethod1(obj) (obj->state|=0x2)
#define IdleObject_GetMethod(obj) (obj->state&0x2)

#define IdleObject_IdleCheckSaver(obj) (obj->state&0x4)
#define IdleObject_SetSaverCheck(obj) (obj->state|=0x4)

#define IdleObject_IdleCheckWorkstation(obj) (obj->state&0x8)
#define IdleObject_SetWorkstationCheck(obj) (obj->state|=0x8)

#define IdleObject_IsPrivacy(obj) (obj->state&0x10)
#define IdleObject_SetPrivacy(obj) (obj->state|=0x10)

#define IdleObject_SetStatusLock(obj) (obj->state|=0x20)

typedef struct {
	UINT hTimer;
	unsigned int useridlecheck;
	unsigned int state; 
	unsigned int minutes;	// user setting, number of minutes of inactivity to wait for
	POINT mousepos;
	unsigned int mouseidle;
    int aastatus;
} IdleObject;

static int aa_Status[] = {ID_STATUS_AWAY, ID_STATUS_NA, ID_STATUS_OCCUPIED, ID_STATUS_DND, ID_STATUS_ONTHEPHONE, ID_STATUS_OUTTOLUNCH};

static IdleObject gIdleObject;
static HANDLE hIdleEvent;
static BOOL (WINAPI * MyGetLastInputInfo)(PLASTINPUTINFO);

void CALLBACK IdleTimer(HWND hwnd, UINT umsg, UINT idEvent, DWORD dwTime);
static BOOL IsWorkstationLocked(void);
static BOOL IsScreenSaverRunning(void);

static void IdleObject_ReadSettings(IdleObject * obj)
{
	obj->useridlecheck = DBGetContactSettingByte(NULL, IDLEMOD, IDL_USERIDLECHECK, 0);
	obj->minutes = DBGetContactSettingByte(NULL, IDLEMOD, IDL_IDLETIME1ST, 10);
    obj->aastatus = !DBGetContactSettingByte(NULL, IDLEMOD, IDL_AAENABLE, 0) ? 0 : DBGetContactSettingWord(NULL, IDLEMOD, IDL_AASTATUS, 0);
	if ( DBGetContactSettingByte(NULL, IDLEMOD, IDL_IDLEMETHOD, 0) ) IdleObject_UseMethod1(obj);
	else IdleObject_UseMethod0(obj);
	if ( DBGetContactSettingByte(NULL, IDLEMOD, IDL_IDLEONSAVER, 0) ) IdleObject_SetSaverCheck(obj);
	if ( DBGetContactSettingByte(NULL, IDLEMOD, IDL_IDLEONLOCK, 0 ) ) IdleObject_SetWorkstationCheck(obj);
	if ( DBGetContactSettingByte(NULL, IDLEMOD, IDL_IDLEPRIVATE, 0) ) IdleObject_SetPrivacy(obj);
	if ( DBGetContactSettingByte(NULL, IDLEMOD, IDL_IDLESTATUSLOCK, 0) ) IdleObject_SetStatusLock(obj); 
}

static void IdleObject_Create(IdleObject * obj)
{
	ZeroMemory(obj, sizeof(IdleObject));	
	obj->hTimer=SetTimer(NULL, 0, 2000, IdleTimer);
	IdleObject_ReadSettings(obj);
}

static void IdleObject_Destroy(IdleObject * obj)
{
	if (IdleObject_IsIdle(obj)) NotifyEventHooks(hIdleEvent, 0, 0);
	IdleObject_ClearIdle(obj);
	KillTimer(NULL, obj->hTimer);
}

static int IdleObject_IsUserIdle(IdleObject * obj)
{
	DWORD dwTick;
	if ( IdleObject_GetMethod(obj) ) {
		CallService(MS_SYSTEM_GETIDLE, 0, (DWORD)&dwTick);
		return GetTickCount() - dwTick > (obj->minutes * 60 * 1000);
	} else {
		if ( MyGetLastInputInfo != NULL ) {
			LASTINPUTINFO ii;
			ZeroMemory(&ii,sizeof(ii));
			ii.cbSize=sizeof(ii);
			if ( MyGetLastInputInfo(&ii) ) return GetTickCount() - ii.dwTime > (obj->minutes * 60 * 1000);
		} else {
			POINT pt;			
			GetCursorPos(&pt);
			if ( pt.x != obj->mousepos.x || pt.y != obj->mousepos.y ) {
				obj->mousepos=pt;
				obj->mouseidle=0;
			} else obj->mouseidle += 2;	
			if ( obj->mouseidle ) return obj->mouseidle * 1000 >= (obj->minutes * 60 * 1000);
		}
		return FALSE;
	}
}

static void IdleObject_Tick(IdleObject * obj)
{
	BOOL idle = ( obj->useridlecheck ? IdleObject_IsUserIdle(obj) : FALSE )
		|| ( IdleObject_IdleCheckSaver(obj) ? IsScreenSaverRunning() : FALSE  ) 
			|| ( IdleObject_IdleCheckWorkstation(obj) ? IsWorkstationLocked() : FALSE );

	unsigned int flags = IdleObject_IsPrivacy(obj) ? IDF_PRIVACY : 0;

	if ( !IdleObject_IsIdle(obj) && idle ) {
		IdleObject_SetIdle(obj);
		NotifyEventHooks(hIdleEvent, 0, IDF_ISIDLE | flags);
	}
	if ( IdleObject_IsIdle(obj) && !idle ) {
		IdleObject_ClearIdle(obj);
		NotifyEventHooks(hIdleEvent, 0, flags);
	}
}

void CALLBACK IdleTimer(HWND hwnd, UINT umsg, UINT idEvent, DWORD dwTime)
{
	if ( gIdleObject.hTimer == idEvent ) {
		IdleObject_Tick(&gIdleObject);
	}
}

// delphi code here http://www.swissdelphicenter.ch/torry/printcode.php?id=2048
static BOOL IsWorkstationLocked(void)
{
	BOOL rc=0;	
	HDESK hDesk = OpenDesktop( _T("default"), 0, FALSE, DESKTOP_SWITCHDESKTOP);
	if ( hDesk != 0 ) {
		rc = SwitchDesktop(hDesk) == FALSE;
		CloseDesktop(hDesk);
	}
	return rc;
}

static BOOL IsScreenSaverRunning()
{
	BOOL rc = FALSE;
	SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &rc, FALSE);
	return rc;
}

int IdleGetStatusIndex(WORD status) {
    int j;
    for (j = 0; j < SIZEOF(aa_Status); j++ ) {
        if (aa_Status[j]==status) return j;
    }
    return 0;
}

static BOOL CALLBACK IdleOptsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG:
	{
		int j;
		int method = DBGetContactSettingByte(NULL,IDLEMOD,IDL_IDLEMETHOD, 0);
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_IDLESHORT, DBGetContactSettingByte(NULL,IDLEMOD,IDL_USERIDLECHECK,0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_IDLEONWINDOWS, method == 0 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_IDLEONMIRANDA, method ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SCREENSAVER, DBGetContactSettingByte(NULL,IDLEMOD,IDL_IDLEONSAVER,0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOCKED, DBGetContactSettingByte(NULL,IDLEMOD,IDL_IDLEONLOCK,0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_IDLEPRIVATE, DBGetContactSettingByte(NULL,IDLEMOD,IDL_IDLEPRIVATE,0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_IDLESTATUSLOCK, DBGetContactSettingByte(NULL,IDLEMOD,IDL_IDLESTATUSLOCK,0) ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg, IDC_IDLESPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_IDLE1STTIME), 0);			
		SendDlgItemMessage(hwndDlg, IDC_IDLESPIN, UDM_SETRANGE32, 1, 60);
		SendDlgItemMessage(hwndDlg, IDC_IDLESPIN, UDM_SETPOS, 0, MAKELONG((short) DBGetContactSettingByte(NULL,IDLEMOD,IDL_IDLETIME1ST, 10), 0));
		SendDlgItemMessage(hwndDlg, IDC_IDLE1STTIME, EM_LIMITTEXT, (WPARAM)2, 0);
    
      CheckDlgButton(hwndDlg, IDC_AASHORTIDLE, DBGetContactSettingByte(NULL, IDLEMOD, IDL_AAENABLE, 0) ? BST_CHECKED:BST_UNCHECKED);
		for ( j = 0; j < SIZEOF(aa_Status); j++ ) {
			TCHAR* szDesc = LangPackPcharToTchar(( LPCSTR )CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)aa_Status[j], 0));
			SendDlgItemMessage(hwndDlg, IDC_AASTATUS, CB_ADDSTRING, 0, (LPARAM)szDesc );
			mir_free( szDesc );
		}

		j = IdleGetStatusIndex((WORD)(DBGetContactSettingWord(NULL, IDLEMOD, IDL_AASTATUS, 0)));
		SendDlgItemMessage(hwndDlg, IDC_AASTATUS, CB_SETCURSEL, j, 0);
		SendMessage(hwndDlg, WM_USER+2, 0, 0);
		return TRUE;
	}
	case WM_USER+2:
	{
		BOOL checked = IsDlgButtonChecked(hwndDlg, IDC_IDLESHORT) == BST_CHECKED;
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDLEONWINDOWS), checked);
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDLEONMIRANDA), checked);
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDLE1STTIME), checked);	
         EnableWindow(GetDlgItem(hwndDlg, IDC_AASTATUS), IsDlgButtonChecked(hwndDlg, IDC_AASHORTIDLE)==BST_CHECKED?1:0);
		EnableWindow(GetDlgItem(hwndDlg, IDC_IDLESTATUSLOCK), IsDlgButtonChecked(hwndDlg, IDC_AASHORTIDLE)==BST_CHECKED?1:0);
		break;
	}
	case WM_NOTIFY: 
	{
		NMHDR * hdr = (NMHDR *)lParam;
		if ( hdr && hdr->code == PSN_APPLY ) {
			int method = IsDlgButtonChecked(hwndDlg, IDC_IDLEONWINDOWS) == BST_CHECKED;
			int mins = SendDlgItemMessage(hwndDlg, IDC_IDLESPIN, UDM_GETPOS, 0, 0);
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_IDLETIME1ST, (BYTE)(HIWORD(mins) == 0 ? LOWORD(mins) : 10));
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_USERIDLECHECK, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_IDLESHORT) == BST_CHECKED));
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_IDLEMETHOD, (BYTE)(method ? 0 : 1));
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_IDLEONSAVER, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_SCREENSAVER) == BST_CHECKED));
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_IDLEONLOCK, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_LOCKED) == BST_CHECKED));
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_IDLEPRIVATE, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_IDLEPRIVATE) == BST_CHECKED));
               DBWriteContactSettingByte(NULL, IDLEMOD, IDL_AAENABLE, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_AASHORTIDLE)==BST_CHECKED?1:0));
			DBWriteContactSettingByte(NULL, IDLEMOD, IDL_IDLESTATUSLOCK, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_IDLESTATUSLOCK)==BST_CHECKED?1:0));
			{
				int curSel = SendDlgItemMessage(hwndDlg, IDC_AASTATUS, CB_GETCURSEL, 0, 0);
				if ( curSel != CB_ERR ) {
					DBWriteContactSettingWord(NULL, IDLEMOD, IDL_AASTATUS, (WORD)(aa_Status[curSel]));
				}
			}
			// destroy any current idle and reset settings.
			IdleObject_Destroy(&gIdleObject);
			IdleObject_Create(&gIdleObject);
		}
		break;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_IDLE1STTIME:
		{								
			int min;			
			if ( (HWND)lParam != GetFocus() || HIWORD(wParam) != EN_CHANGE ) return FALSE;
			min=GetDlgItemInt(hwndDlg, IDC_IDLE1STTIME, NULL, FALSE);
			if ( min == 0 && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_IDLE1STTIME)) ) 
				SendDlgItemMessage(hwndDlg, IDC_IDLESPIN, UDM_SETPOS, 0, MAKELONG((short) 1, 0));
			break;
		}
		case IDC_IDLESHORT:
		case IDC_AASHORTIDLE:
			SendMessage(hwndDlg, WM_USER+2, 0, 0);
			break;

		case IDC_AASTATUS:
			if ( HIWORD(wParam) != CBN_SELCHANGE )
				return TRUE;
		}
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	}
	return FALSE;
}

static int IdleOptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.position = 100000000;
	odp.hInstance = GetModuleHandle(NULL);
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_IDLE);
	odp.pszGroup = "Status";
	odp.pszTitle = "Idle";
	odp.pfnDlgProc = IdleOptsDlgProc;
	odp.flags = ODPF_BOLDGROUPS;
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );
	return 0;
}

static int IdleGetInfo(WPARAM wParam, LPARAM lParam)
{
	MIRANDA_IDLE_INFO *mii = (MIRANDA_IDLE_INFO*)lParam;

    if (!mii || mii->cbSize!=sizeof(MIRANDA_IDLE_INFO)) return 1;
	mii->idleTime = gIdleObject.minutes;
    mii->privacy = gIdleObject.state&0x10;
    mii->aaStatus = gIdleObject.aastatus;
	mii->aaLock = gIdleObject.state&0x20;
	return 0;
}

static int UnloadIdleModule(WPARAM wParam, LPARAM lParam)
{
	IdleObject_Destroy(&gIdleObject);
	DestroyHookableEvent(hIdleEvent);
	hIdleEvent=NULL;
	return 0;
}

int LoadIdleModule(void)
{
	MyGetLastInputInfo=(BOOL (WINAPI *)(LASTINPUTINFO*))GetProcAddress(GetModuleHandleA("user32"), "GetLastInputInfo");
	hIdleEvent=CreateHookableEvent(ME_IDLE_CHANGED);
	IdleObject_Create(&gIdleObject);
    CreateServiceFunction(MS_IDLE_GETIDLEINFO, IdleGetInfo);
	HookEvent(ME_SYSTEM_SHUTDOWN, UnloadIdleModule);
	HookEvent(ME_OPT_INITIALISE, IdleOptInit);
	return 0;
}


