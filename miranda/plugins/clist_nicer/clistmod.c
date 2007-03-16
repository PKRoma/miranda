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

UNICODE done

*/
#include "commonheaders.h"

extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);

BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE, SIZE_T, SIZE_T) = 0;

extern int AddEvent(WPARAM wParam, LPARAM lParam);
extern int RemoveEvent(WPARAM wParam, LPARAM lParam);

int InitCustomMenus(void);
void UninitCustomMenus(void);
int GetContactStatusMessage(WPARAM wParam, LPARAM lParam);
void TrayIconUpdateBase(const char *szChangedProto);
int EventsProcessContactDoubleClick(HANDLE hContact);
int SetHideOffline(WPARAM wParam, LPARAM lParam);

HIMAGELIST hCListImages;

extern int       g_maxStatus;
extern HANDLE    hSvc_GetContactStatusMsg;
extern ImageItem *g_CLUIImageItem;

extern struct CluiData g_CluiData;

static int GetStatusMode(WPARAM wParam, LPARAM lParam)
{
	return(g_maxStatus == ID_STATUS_OFFLINE ? pcli->currentDesiredStatusMode : g_maxStatus);
}

extern int ( *saveIconFromStatusMode )( const char *szProto, int status, HANDLE hContact );

int IconFromStatusMode(const char *szProto, int status, HANDLE hContact, HICON *phIcon)
{
	char *szFinalProto;
	int finalStatus;

	if (szProto != NULL && !strcmp(szProto, "MetaContacts") && g_CluiData.bMetaAvail && hContact != 0 && !(g_CluiData.dwFlags & CLUI_USEMETAICONS)) {
		HANDLE hSubContact = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) hContact, 0);
		szFinalProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hSubContact, 0);
		finalStatus = (status == 0) ? (WORD) DBGetContactSettingWord(hSubContact, szFinalProto, "Status", ID_STATUS_OFFLINE) : status;
	} else {
		szFinalProto = (char*) szProto;
		finalStatus = status;
	}

	if(status >= ID_STATUS_CONNECTING && status < ID_STATUS_OFFLINE && phIcon != NULL) {
		if(szProto && g_CluiData.IcoLib_Avail) {
			char szBuf[128];
			mir_snprintf(szBuf, 128, "%s_conn", szProto);
			*phIcon = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)szBuf);
		}
		else if(szProto)
			*phIcon = g_CluiData.hIconConnecting;;
	}
	return saveIconFromStatusMode(szFinalProto, finalStatus, hContact);
}

static int MenuItem_LockAvatar(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static int ContactListShutdownProc(WPARAM wParam, LPARAM lParam)
{
	UninitCustomMenus();
	return 0;
}

int LoadContactListModule(void)
{
	HookEvent(ME_SYSTEM_SHUTDOWN, ContactListShutdownProc);
	CreateServiceFunction(MS_CLIST_GETSTATUSMODE, GetStatusMode);

	hSvc_GetContactStatusMsg = CreateServiceFunction("CList/GetContactStatusMsg", GetContactStatusMessage);
	InitCustomMenus();
	MySetProcessWorkingSetSize = (BOOL(WINAPI *)(HANDLE, SIZE_T, SIZE_T))GetProcAddress(GetModuleHandleA("kernel32"), "SetProcessWorkingSetSize");
	return 0;
}

/*
Begin of Hrk's code for bug
*/
#define GWVS_HIDDEN 1
#define GWVS_VISIBLE 2
#define GWVS_COVERED 3
#define GWVS_PARTIALLY_COVERED 4

int GetWindowVisibleState(HWND hWnd, int iStepX, int iStepY)
{
	RECT rc = {0}, rcUpdate = {0};
	POINT pt = {0};
	register int i = 0, j = 0, width = 0, height = 0, iCountedDots = 0, iNotCoveredDots = 0;
	BOOL bPartiallyCovered = FALSE;
	HWND hAux = 0;

	if (hWnd == NULL) {
		SetLastError(0x00000006); //Wrong handle
		return -1;
	}
	//Some defaults now. The routine is designed for thin and tall windows.

	if (IsIconic(hWnd) || !IsWindowVisible(hWnd))
		return GWVS_HIDDEN;
	else {
		HRGN rgn = 0;
		POINT ptTest, ptOrig;
        RECT  rcClient;
		int clip = (int)g_CluiData.bClipBorder;

        GetClientRect(hWnd, &rcClient);
        ptOrig.x = ptOrig.y = 0;
        ClientToScreen(hWnd, &ptOrig);
        rc.left = ptOrig.x;
        rc.top = ptOrig.y;
        rc.right = rc.left + rcClient.right;
        rc.bottom = rc.top + rcClient.bottom;

		//GetWindowRect(hWnd, &rc);
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;

		if (iStepX <= 0)
			iStepX = 4;
		if (iStepY <= 0)
			iStepY = 16;

		/*
		 * use a rounded clip region to determine which pixels are covered
		 * this will avoid problems with certain XP themes which are using transparency for rounded
		 * window frames (reluna being one popular example).

		 * the radius of 8 should be sufficient for most themes as they usually don't use bigger
		 * radii.
		 * also, clip at least 2 pixels from the border (same reason)
		 */

        if(g_CLUIImageItem)
            clip = 5;
        else
            clip = 0;
        //clip = max(clip, DBGetContactSettingByte(NULL, "CLUI", "ignoreframepixels", 2));
		//rgn = CreateRoundRectRgn(rc.left + clip, rc.top + clip, rc.right - clip, rc.bottom - clip, 10 + clip, 10 + clip);
        //rgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		//for (i = rc.top + 3 + clip; i < rc.bottom - 3 - clip; i += (height / iStepY)) {
        for (i = rc.top + clip; i < rc.bottom; i += (height / iStepY)) {
			pt.y = i;
			//for (j = rc.left + 3 + clip; j < rc.right - 3 - clip; j += (width / iStepX)) {
            for (j = rc.left + clip; j < rc.right; j += (width / iStepX)) {
				/*if(rgn) {
					ptTest.x = j;
					ptTest.y = i;
					if(!PtInRegion(rgn, ptTest.x, ptTest.y)) {
						continue;
					}
				}*/
				pt.x = j;
				hAux = WindowFromPoint(pt);
				while (GetParent(hAux) != NULL)
					hAux = GetParent(hAux);
				if (hAux != hWnd) //There's another window!
					bPartiallyCovered = TRUE;
				else
					iNotCoveredDots++; //Let's count the not covered dots.
				iCountedDots++; //Let's keep track of how many dots we checked.
			}
		}
		if(rgn)
			DeleteObject(rgn);

		if (iNotCoveredDots == iCountedDots) //Every dot was not covered: the window is visible.
			return GWVS_VISIBLE;
		else if (iNotCoveredDots == 0) //They're all covered!
			return GWVS_COVERED;
		else //There are dots which are visible, but they are not as many as the ones we counted: it's partially covered.
			return GWVS_PARTIALLY_COVERED;
	}
}

int ShowHide(WPARAM wParam, LPARAM lParam)
{
	BOOL bShow = FALSE;

	int iVisibleState = pcli->pfnGetWindowVisibleState(pcli->hwndContactList, 0, 0);

	//bShow is FALSE when we enter the switch.
	switch (iVisibleState) {
	case GWVS_PARTIALLY_COVERED:
		//If we don't want to bring it to top, we can use a simple break. This goes against readability ;-) but the comment explains it.
		if (!DBGetContactSettingByte(NULL, "CList", "BringToFront", SETTING_BRINGTOFRONT_DEFAULT))
			break;
	case GWVS_COVERED:     //Fall through (and we're already falling)
	case GWVS_HIDDEN:
		bShow = TRUE;
		break;
	case GWVS_VISIBLE:     //This is not needed, but goes for readability.
		bShow = FALSE;
		break;
	case -1:               //We can't get here, both cli.hwndContactList and iStepX and iStepY are right.
		return 0;
	}
	if (bShow == TRUE) {
		WINDOWPLACEMENT pl = { 0 };
		HMONITOR(WINAPI * MyMonitorFromWindow) (HWND, DWORD);
		RECT rcScreen, rcWindow;
		int offScreen = 0;

		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, FALSE);
		SetWindowPos(pcli->hwndContactList, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOCOPYBITS);
		if (!DBGetContactSettingByte(NULL, "CList", "OnTop", SETTING_ONTOP_DEFAULT))
			SetWindowPos(pcli->hwndContactList, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOCOPYBITS);
		SetForegroundWindow(pcli->hwndContactList);
		//SetActiveWindow(pcli->hwndContactList);
		ShowWindow(pcli->hwndContactList, SW_SHOW);
		DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);

        //this forces the window onto the visible screen
		MyMonitorFromWindow = (HMONITOR(WINAPI *) (HWND, DWORD)) GetProcAddress(GetModuleHandleA("USER32"), "MonitorFromWindow");
		GetWindowRect(pcli->hwndContactList, &rcWindow);
		if (MyMonitorFromWindow) {
			if (MyMonitorFromWindow(pcli->hwndContactList, 0) == NULL) {
				BOOL(WINAPI * MyGetMonitorInfoA) (HMONITOR, LPMONITORINFO);
				MONITORINFO mi = { 0 };
				HMONITOR hMonitor = MyMonitorFromWindow(pcli->hwndContactList, 2);
				MyGetMonitorInfoA = (BOOL(WINAPI *) (HMONITOR, LPMONITORINFO)) GetProcAddress(GetModuleHandleA("USER32"), "GetMonitorInfoA");
				mi.cbSize = sizeof(mi);
				MyGetMonitorInfoA(hMonitor, &mi);
				rcScreen = mi.rcWork;
				offScreen = 1;
			}
		}
		else {
			RECT rcDest;
			if (IntersectRect(&rcDest, &rcScreen, &rcWindow) == 0)
				offScreen = 1;
		}
		if (offScreen) {
			if (rcWindow.top >= rcScreen.bottom)
				OffsetRect(&rcWindow, 0, rcScreen.bottom - rcWindow.bottom);
			else if (rcWindow.bottom <= rcScreen.top)
				OffsetRect(&rcWindow, 0, rcScreen.top - rcWindow.top);
			if (rcWindow.left >= rcScreen.right)
				OffsetRect(&rcWindow, rcScreen.right - rcWindow.right, 0);
			else if (rcWindow.right <= rcScreen.left)
				OffsetRect(&rcWindow, rcScreen.left - rcWindow.left, 0);
			SetWindowPos(pcli->hwndContactList, 0, rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
				SWP_NOZORDER);
		}
	}
	else {                      //It needs to be hidden
		ShowWindow(pcli->hwndContactList, SW_HIDE);
		DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_HIDDEN);
		if (MySetProcessWorkingSetSize != NULL && DBGetContactSettingByte(NULL, "CList", "DisableWorkingSet", 1))
			MySetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
	}
	return 0;
}