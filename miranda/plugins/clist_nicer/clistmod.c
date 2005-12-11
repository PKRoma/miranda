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

int AddMainMenuItem(WPARAM wParam, LPARAM lParam);
int AddContactMenuItem(WPARAM wParam, LPARAM lParam);
int InitCustomMenus(void);
void UninitCustomMenus(void);
void UninitCListEvents(void);
int GetContactStatusMessage(WPARAM wParam, LPARAM lParam);
int CListOptInit(WPARAM wParam, LPARAM lParam);
void TrayIconUpdateBase(const char *szChangedProto);
int EventsProcessContactDoubleClick(HANDLE hContact);
int TrayIconProcessMessage(WPARAM wParam, LPARAM lParam);
int TrayIconPauseAutoHide(WPARAM wParam, LPARAM lParam);
void TrayIconIconsChanged(void);
int CompareContacts(WPARAM wParam, LPARAM lParam);
int SetHideOffline(WPARAM wParam, LPARAM lParam);
static int CListIconsChanged(WPARAM wParam, LPARAM lParam);
int MenuProcessCommand(WPARAM wParam, LPARAM lParam);

HANDLE hContactDoubleClicked, hStatusModeChangeEvent, hContactIconChangedEvent;
HIMAGELIST hCListImages;
extern int currentDesiredStatusMode;

BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE, SIZE_T, SIZE_T);
extern BYTE nameOrder[];
static int statusModeList[] = {
	ID_STATUS_OFFLINE, ID_STATUS_ONLINE, ID_STATUS_AWAY, ID_STATUS_NA, ID_STATUS_OCCUPIED, ID_STATUS_DND, ID_STATUS_FREECHAT, ID_STATUS_INVISIBLE, ID_STATUS_ONTHEPHONE, ID_STATUS_OUTTOLUNCH
};
static int skinIconStatusList[] = {
	SKINICON_STATUS_OFFLINE, SKINICON_STATUS_ONLINE, SKINICON_STATUS_AWAY, SKINICON_STATUS_NA, SKINICON_STATUS_OCCUPIED, SKINICON_STATUS_DND, SKINICON_STATUS_FREE4CHAT, SKINICON_STATUS_INVISIBLE, SKINICON_STATUS_ONTHEPHONE, SKINICON_STATUS_OUTTOLUNCH
};
struct ProtoIconIndex {
	char *szProto;
	int iIconBase;
} static *protoIconIndex;
static int protoIconIndexCount;

extern struct CluiData g_CluiData;

static int SetStatusMode(WPARAM wParam, LPARAM lParam)
{
	//todo: check wParam is valid so people can't use this to run random menu items
	MenuProcessCommand(MAKEWPARAM(LOWORD(wParam), MPCF_MAINMENU), 0);
	return 0;
}

static int GetStatusMode(WPARAM wParam, LPARAM lParam)
{
	return currentDesiredStatusMode;
}


int IconFromStatusMode(const char *szProto, int status, HANDLE hContact, HICON *phIcon)
{
	int index, i;
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
	for (index = 0; index < sizeof(statusModeList) / sizeof(statusModeList[0]); index++) {
		if (finalStatus == statusModeList[index])
			break;
	}
	if (index == sizeof(statusModeList) / sizeof(statusModeList[0]))
		index = 0;
	if (szFinalProto == NULL)
		return index + 1;
	for (i = 0; i < protoIconIndexCount; i++) {
		if (strcmp(szFinalProto, protoIconIndex[i].szProto))
			continue;
		return protoIconIndex[i].iIconBase + index;
	}
	return 1;
}


static int MenuItem_LockAvatar(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static int ContactListShutdownProc(WPARAM wParam, LPARAM lParam)
{
	UninitCustomMenus();
	UninitCListEvents();
	return 0;
}

int LoadContactListModule(void)
{
	HookEvent(ME_SYSTEM_SHUTDOWN, ContactListShutdownProc);
	HookEvent(ME_OPT_INITIALISE, CListOptInit);
	hStatusModeChangeEvent = CreateHookableEvent(ME_CLIST_STATUSMODECHANGE);
	CreateServiceFunction(MS_CLIST_SETSTATUSMODE, SetStatusMode);
	CreateServiceFunction(MS_CLIST_GETSTATUSMODE, GetStatusMode);
	CreateServiceFunction("CList/GetContactStatusMsg", GetContactStatusMessage);
	CreateServiceFunction(MS_CLIST_TRAYICONPROCESSMESSAGE, TrayIconProcessMessage);
	CreateServiceFunction(MS_CLIST_PAUSEAUTOHIDE, TrayIconPauseAutoHide);
	MySetProcessWorkingSetSize = (BOOL(WINAPI *)(HANDLE, SIZE_T, SIZE_T))GetProcAddress(GetModuleHandleA("kernel32"), "SetProcessWorkingSetSize");
	InitCustomMenus();
	IMG_InitDecoder();

	hCListImages = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16), 13, 0);
	HookEvent(ME_SKIN_ICONSCHANGED, CListIconsChanged);
	ImageList_AddIcon(hCListImages, LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_BLANK))); {
		int i;
		for (i = 0; i < sizeof(statusModeList) / sizeof(statusModeList[0]); i++) {
			ImageList_AddIcon(hCListImages, LoadSkinnedIcon(skinIconStatusList[i]));
		}
	}
	//see IMAGE_GROUP... in clist.h if you add more images above here
	ImageList_AddIcon(hCListImages, LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN));
	ImageList_AddIcon(hCListImages, LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT));
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
		POINT ptTest;
		int clip = (int)g_CluiData.bClipBorder;
		GetWindowRect(hWnd, &rc);
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;

		if (iStepX <= 0)
			iStepX = 4;
		if (iStepY <= 0)
			iStepY = 16;

		if(g_CluiData.bClipBorder != 0 || g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME) {
			if(g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME)
				rgn = CreateRoundRectRgn(rc.left + clip, rc.top + clip, rc.right - clip, rc.bottom - clip, 8 + clip, 8 + clip);
			else
				rgn = CreateRectRgn(rc.left + clip, rc.top + clip, rc.right - clip, rc.bottom - clip);
		}
		for (i = rc.top + 3 + clip; i < rc.bottom - 3 - clip; i += (height / iStepY)) {
			pt.y = i;
			for (j = rc.left + 3 + clip; j < rc.right - 3 - clip; j += (width / iStepX)) {
				if(rgn) {
					ptTest.x = j;
					ptTest.y = i;
					if(!PtInRegion(rgn, ptTest.x, ptTest.y)) {
						continue;
					}
				}
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

static int CListIconsChanged(WPARAM wParam, LPARAM lParam)
{
	int i, j;

	for (i = 0; i < sizeof(statusModeList) / sizeof(statusModeList[0]); i++) {
		ImageList_ReplaceIcon(hCListImages, i + 1, LoadSkinnedIcon(skinIconStatusList[i]));
	}
	ImageList_ReplaceIcon(hCListImages, IMAGE_GROUPOPEN, LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN));
	ImageList_ReplaceIcon(hCListImages, IMAGE_GROUPSHUT, LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT));
	for (i = 0; i < protoIconIndexCount; i++) {
		for (j = 0; j < sizeof(statusModeList) / sizeof(statusModeList[0]); j++) {
			ImageList_ReplaceIcon(hCListImages, protoIconIndex[i].iIconBase + j, LoadSkinnedProtoIcon(protoIconIndex[i].szProto, statusModeList[j]));
		}
	}
	TrayIconIconsChanged();
	InvalidateRect((HWND) CallService(MS_CLUI_GETHWND, 0, 0), NULL, TRUE);
	return 0;
}
