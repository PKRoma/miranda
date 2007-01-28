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

#define TRAYICON_ID_BASE    100
#define TIM_CALLBACK   (WM_USER+1857)
#define TIM_CREATE     (WM_USER+1858)

static VOID CALLBACK TrayCycleTimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime);

extern HIMAGELIST hCListImages;

extern struct CluiData g_CluiData;

static int cycleTimerId = 0,cycleStep = 0;

struct trayIconInfo_t {
	int id;
	char *szProto;
	HICON hBaseIcon;
	int isBase;
};
static struct trayIconInfo_t *trayIcon = NULL;
static int trayIconCount;

BOOL g_trayTooltipActive = FALSE;
POINT tray_hover_pos = {0};

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010

/*
typedef struct _DllVersionInfo {
	DWORD cbSize;
	DWORD dwMajorVersion;                   // Major version
	DWORD dwMinorVersion;                   // Minor version
	DWORD dwBuildNumber;                    // Build number
	DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;*/
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

static DLLVERSIONINFO dviShell = {0};
static BOOL mToolTipTrayTips = FALSE;

static TCHAR szTip[2048];
static TCHAR* TrayIconMakeTooltip(const TCHAR *szPrefix, const char *szProto)
{
	char szProtoName[32];
	TCHAR *szStatus, *szSeparator, *sztProto;
	int tipSize;

	if(ServiceExists("mToolTip/ShowTip"))
		tipSize = 2048;
	else
		tipSize = 128;

	if(!mToolTipTrayTips)
		szSeparator = (IsWinVerMEPlus()) ? szSeparator = _T("\n") : _T(" | ");
	else
		szSeparator = _T("\n");

	if (szProto == NULL) {
		PROTOCOLDESCRIPTOR **protos;
		int count, netProtoCount, i;
		CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
		for (i = 0,netProtoCount = 0; i < count; i++) {
			if (protos[i]->type == PROTOTYPE_PROTOCOL && pcli->pfnGetProtocolVisibility(protos[i]->szName))
				netProtoCount++;
		}
		if (netProtoCount == 1)
			for (i = 0; i < count; i++) {
				if (protos[i]->type == PROTOTYPE_PROTOCOL && pcli->pfnGetProtocolVisibility(protos[i]->szName))
					return TrayIconMakeTooltip(szPrefix, protos[i]->szName);
			}
			if (szPrefix && szPrefix[0]) {
				lstrcpyn(szTip, szPrefix, tipSize);
				if (!DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT))
					return szTip;
			} else
				szTip[0] = '\0';
			szTip[tipSize - 1] = '\0';
			for (i = count - 1; i >= 0; i--) {
				if (protos[i]->type != PROTOTYPE_PROTOCOL || !pcli->pfnGetProtocolVisibility(protos[i]->szName))
					continue;
				CallProtoService(protos[i]->szName, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
				szStatus = pcli->pfnGetStatusModeDescription(CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0), 0);
				if(szStatus) {
					if(mToolTipTrayTips) {
						TCHAR tipline[256];
						#if defined( _UNICODE )
						{	TCHAR* p = a2u( szProtoName );
							mir_sntprintf(tipline, 256, _T("<b>%-12.12s</b>\t%s"), p, szStatus);
							free( p );
						}
						#else
							mir_sntprintf(tipline, 256, _T("<b>%-12.12s</b>\t%s"), szProtoName, szStatus);
						#endif
						if (szTip[0])
							_tcsncat(szTip, szSeparator, tipSize - 1 - _tcslen(szTip));
						_tcsncat(szTip, tipline, tipSize - 1 - _tcslen(szTip));
					}
					else {
						if (szTip[0])
							_tcsncat(szTip, szSeparator, tipSize - 1 - _tcslen(szTip));
						#if defined( _UNICODE )
						{	TCHAR* p = a2u( szProtoName );
							_tcsncat(szTip, p, SIZEOF(szTip) - 1 - _tcslen(szTip));
							free( p );
						}
						#else
							_tcsncat(szTip, szProtoName, SIZEOF(szTip) - 1 - _tcslen(szTip));
						#endif
						_tcsncat(szTip, _T(" "), tipSize - 1 - _tcslen(szTip));
						_tcsncat(szTip, szStatus, tipSize - 1 - _tcslen(szTip));
					}
				}
			}
	} else {
		CallProtoService(szProto, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
		#if defined( _UNICODE )
			sztProto = a2u( szProtoName );
		#else
			sztProto = szProtoName;
		#endif
		szStatus = pcli->pfnGetStatusModeDescription(CallProtoService(szProto, PS_GETSTATUS, 0, 0), 0);
		if (szPrefix && szPrefix[0]) {
			if (DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT)) {
				if(mToolTipTrayTips)
					mir_sntprintf(szTip, tipSize, _T("%s%s<b>%-12.12s</b>\t%s"), szPrefix, szSeparator, sztProto, szStatus);
				else
					mir_sntprintf(szTip, tipSize, _T("%s%s%s %s"), szPrefix, szSeparator, sztProto, szStatus);
			}
			else
				lstrcpyn(szTip, szPrefix, tipSize);
		} else {
			if(mToolTipTrayTips)
				mir_sntprintf(szTip, tipSize, _T("<b>%-12.12s</b>\t%s"), sztProto, szStatus);
			else
				mir_sntprintf(szTip, tipSize, _T("%s %s"), sztProto, szStatus);
		}
		#if defined( _UNICODE )
			free(sztProto);
		#endif
	}
	return szTip;
}

static int TrayIconAdd(HWND hwnd, const char *szProto, const char *szIconProto, int status)
{
	NOTIFYICONDATA nid = {0};
	int i, iIcon = 0;
	HICON hIcon = 0;

	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			break;
	}
	trayIcon[i].id = TRAYICON_ID_BASE + i;
	trayIcon[i].szProto = (char *) szProto;

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	nid.uID = trayIcon[i].id;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TIM_CALLBACK;
	iIcon = IconFromStatusMode(szIconProto ? szIconProto : trayIcon[i].szProto, status, 0, &hIcon);
	if(hIcon == 0)
		trayIcon[i].hBaseIcon = ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL);
	else
		trayIcon[i].hBaseIcon = CopyIcon(hIcon);
	nid.hIcon = trayIcon[i].hBaseIcon;  

	if (dviShell.dwMajorVersion >= 5)
		nid.uFlags |= NIF_INFO;

	TrayIconMakeTooltip(NULL, trayIcon[i].szProto);
	if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
		lstrcpyn(nid.szTip, szTip, SIZEOF(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	trayIcon[i]. isBase = 1;
	return i;
}

static void TrayIconRemove(HWND hwnd, const char *szProto)
{
	int i;
	NOTIFYICONDATA nid = {0};

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		if (lstrcmpA(szProto, trayIcon[i].szProto))
			continue;

		nid.uID = trayIcon[i].id;
		Shell_NotifyIcon(NIM_DELETE, &nid);
		DestroyIcon(trayIcon[i].hBaseIcon);
		trayIcon[i]. id = 0;
		break;
	}
}

static int TrayIconInit(HWND hwnd)
{
	int count, netProtoCount, i;
	int averageMode = 0;
	PROTOCOLDESCRIPTOR **protos;

	if(ServiceExists("mToolTip/ShowTip"))
		mToolTipTrayTips = TRUE;

	if (cycleTimerId) {
		KillTimer(NULL, cycleTimerId); 
		cycleTimerId = 0;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
	for (i = 0,netProtoCount = 0; i < count; i++) {
		if (protos[i]->type != PROTOTYPE_PROTOCOL || !pcli->pfnGetProtocolVisibility(protos[i]->szName))
			continue;

		cycleStep = i;
		netProtoCount++;
		if (averageMode == 0)
			averageMode = CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0);
		else if (averageMode != CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0)) {
			averageMode = -1; break;
		}
	}
	trayIconCount = count;
	trayIcon = (struct trayIconInfo_t *) mir_alloc(sizeof(struct trayIconInfo_t) * count);
	ZeroMemory(trayIcon, sizeof(struct trayIconInfo_t) * count);
	if (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI && (averageMode <= 0 || DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT))) {
		int i;
		for (i = count - 1; i >= 0; i--) {
			if (!pcli->pfnGetProtocolVisibility(protos[i]->szName))
				continue;
			if (protos[i]->type == PROTOTYPE_PROTOCOL)
				TrayIconAdd(hwnd, protos[i]->szName, NULL, CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0));
		}
	} else if (averageMode <= 0 && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_SINGLE) {
		DBVARIANT dbv = {DBVT_DELETED};
		char *szProto;
		if (DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv))
			szProto = NULL;
		else
			szProto = dbv.pszVal;
		TrayIconAdd(hwnd, NULL, szProto, szProto ? CallProtoService(szProto, PS_GETSTATUS, 0, 0) : CallService(MS_CLIST_GETSTATUSMODE, 0, 0));
		DBFreeVariant(&dbv);
	} else
		TrayIconAdd(hwnd, NULL, NULL, averageMode);
	if (averageMode <= 0 && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_CYCLE)
		cycleTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, TrayCycleTimerProc);
	return 0;
}

static void TrayIconDestroy(HWND hwnd)
{
	NOTIFYICONDATA nid = {
		0
	};
	int i;

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		nid.uID = trayIcon[i].id;
		Shell_NotifyIcon(NIM_DELETE, &nid);
		DestroyIcon(trayIcon[i].hBaseIcon);
	}
	if (trayIcon)
		mir_free(trayIcon);
	trayIcon = NULL;
	trayIconCount = 0;
}

int TrayIconUpdate(HICON hNewIcon, const TCHAR *szNewTip, const char *szPreferredProto, int isBase)
{
	NOTIFYICONDATA nid = {0};
	int i;
	char szProto = 0;

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = pcli->hwndContactList;
	nid.uFlags = NIF_ICON | NIF_TIP;
	nid.hIcon = hNewIcon;           

	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		if (lstrcmpA(trayIcon[i].szProto, szPreferredProto))
			continue;
		nid.uID = trayIcon[i].id;
		TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
		if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
			lstrcpyn(nid.szTip, szTip, SIZEOF(nid.szTip));
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		trayIcon[i].isBase = isBase;
		return i;
	}
	//if there wasn't a suitable icon, change all the icons

	if(szPreferredProto != NULL && !pcli->pfnGetProtocolVisibility((char *)szPreferredProto) && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI)
		return -1;

	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		nid.uID = trayIcon[i].id;
		TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
		if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
			lstrcpyn(nid.szTip, szTip, SIZEOF(nid.szTip));
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		trayIcon[i].isBase = isBase;
	}
	return -1;
}

int TrayIconSetBaseInfo(HICON hIcon, const char *szPreferredProto)
{
	int i;

	if(szPreferredProto != (const char *)NULL && !pcli->pfnGetProtocolVisibility((char *)szPreferredProto))
		return -1;

	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		if (lstrcmpA(trayIcon[i].szProto, szPreferredProto))
			continue;
		DestroyIcon(trayIcon[i].hBaseIcon);
		trayIcon[i]. hBaseIcon = hIcon;
		return i;
	}
	//if there wasn't a specific icon, there will only be one suitable
	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		DestroyIcon(trayIcon[i].hBaseIcon);
		trayIcon[i]. hBaseIcon = hIcon;
		return i;
	}
	DestroyIcon(hIcon);
	return -1;
}

void TrayIconUpdateWithImageList(int iImage, const TCHAR *szNewTip, char *szPreferredProto)
{
    HICON hIcon = ImageList_GetIcon(hCListImages, iImage, ILD_NORMAL);
    TrayIconUpdate(hIcon, szNewTip, szPreferredProto, 0);
    DestroyIcon(hIcon);
}

static VOID CALLBACK TrayCycleTimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
	int count;
	PROTOCOLDESCRIPTOR **protos;

	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
	for (cycleStep++; ; cycleStep++) {
		if (cycleStep >= count)
			cycleStep = 0;
		if (protos[cycleStep]->type == PROTOTYPE_PROTOCOL && pcli->pfnGetProtocolVisibility(protos[cycleStep]->szName))
			break;
	}
	DestroyIcon(trayIcon[0].hBaseIcon);
	trayIcon[0].hBaseIcon = ImageList_GetIcon(hCListImages, IconFromStatusMode(protos[cycleStep]->szName, CallProtoService(protos[cycleStep]->szName, PS_GETSTATUS, 0, 0), 0, NULL), ILD_NORMAL);
	if (trayIcon[0].isBase)
		TrayIconUpdate(trayIcon[0].hBaseIcon, NULL, NULL, 1);
}

void TrayIconUpdateBase(const char *szChangedProto)
{
	int i,count,netProtoCount,changed = -1;
	PROTOCOLDESCRIPTOR **protos;
	int averageMode = 0;
	HWND hwnd = pcli->hwndContactList;

	if (cycleTimerId) {
		KillTimer(NULL, cycleTimerId); cycleTimerId = 0;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
	for (i = 0,netProtoCount = 0; i < count; i++) {
		if (protos[i]->type != PROTOTYPE_PROTOCOL || !pcli->pfnGetProtocolVisibility(protos[i]->szName))
			continue;
		netProtoCount++;
		if (!lstrcmpA(szChangedProto, protos[i]->szName))
			cycleStep = i;
		if (averageMode == 0)
			averageMode = CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0);
		else if (averageMode != CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0)) {
			averageMode = -1; break;
		}
	}
	if (netProtoCount > 1) {
		if (averageMode > 0) {
			if (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI) {
				if (DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT)) {
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(szChangedProto, averageMode, 0, &hIcon);
					if(hIcon)
						changed = TrayIconSetBaseInfo(CopyIcon(hIcon), szChangedProto);
					else
						changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), szChangedProto);
				}
				else if (trayIcon && trayIcon[0].szProto != NULL) {
					TrayIconDestroy(hwnd);
					TrayIconInit(hwnd);
				} else {
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

					if(hIcon)
						changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
					else
						changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
				}
			} else {
				HICON hIcon = 0;
				int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

				if(hIcon)
					changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
				else
					changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
			}
		} else {
			switch (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT)) {
			case SETTING_TRAYICON_SINGLE:
				{
					DBVARIANT dbv = {DBVT_DELETED};
					int iIcon = 0;
					HICON hIcon = 0;
					char *szProto;
					if (DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv))
						szProto = NULL;
					else
						szProto = dbv.pszVal;
					iIcon = IconFromStatusMode(szProto, szProto ? CallProtoService(szProto, PS_GETSTATUS, 0, 0) : CallService(MS_CLIST_GETSTATUSMODE, 0, 0), 0, &hIcon);
					if(hIcon)
						changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
					else
						changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
					DBFreeVariant(&dbv);
					break;
				}
			case SETTING_TRAYICON_CYCLE:
				{
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), 0, &hIcon);

					cycleTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, TrayCycleTimerProc);
					if(hIcon)
						changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
					else
						changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
					break;
				}
			case SETTING_TRAYICON_MULTI:
				if (!trayIcon) {
					TrayIconRemove(NULL, NULL);
				} else if (DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT)) {
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), 0, &hIcon);

					if(hIcon)
						changed = TrayIconSetBaseInfo(CopyIcon(hIcon), szChangedProto);
					else
						changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), szChangedProto);
				}
				else {
					TrayIconDestroy(hwnd);
					TrayIconInit(hwnd);
				}
				break;
			}
		}
	} else {
		HICON hIcon = 0;
		int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

		if(hIcon)
			changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
		else
			changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
	}
	if (changed != -1 && trayIcon[changed].isBase)
		TrayIconUpdate(trayIcon[changed].hBaseIcon, NULL, trayIcon[changed].szProto, 1);
}

void TrayIconSetToBase(char *szPreferredProto)
{
	int i;

	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		if (lstrcmpA(trayIcon[i].szProto, szPreferredProto))
			continue;
		TrayIconUpdate(trayIcon[i].hBaseIcon, NULL, szPreferredProto, 1);
		return;
	}
	//if there wasn't a specific icon, there will only be one suitable
	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			continue;
		TrayIconUpdate(trayIcon[i].hBaseIcon, NULL, szPreferredProto, 1);
		return;
	}
}

void TrayIconIconsChanged(void)
{
	HWND hwnd = pcli->hwndContactList;
	TrayIconDestroy(hwnd);
	TrayIconInit(hwnd);
}

/////////////////////////////////////////////////////////////////////////////////////////

extern int ( *saveTrayIconProcessMessage )(WPARAM wParam, LPARAM lParam);

static void CALLBACK TrayToolTipTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
	if(!g_trayTooltipActive && !g_CluiData.bNoTrayTips) {
		CLCINFOTIP ti = {0};
		POINT pt;
		GetCursorPos(&pt);
		if(pt.x == tray_hover_pos.x && pt.y == tray_hover_pos.y) {
			ti.cbSize = sizeof(ti);
			ti.isTreeFocused = GetFocus() == pcli->hwndContactList ? 1 : 0;
			#if defined( _UNICODE )
			{	char* p = u2a( szTip );
	        	CallService("mToolTip/ShowTip", (WPARAM)p, (LPARAM)&ti);
				free( p );
			}
			#else
	        	CallService("mToolTip/ShowTip", (WPARAM)szTip, (LPARAM)&ti);
			#endif
			GetCursorPos(&tray_hover_pos);
			g_trayTooltipActive = TRUE;
		}
	}
	KillTimer(hwnd, id);
}

int TrayIconProcessMessage(WPARAM wParam, LPARAM lParam)
{
	MSG *msg = (MSG *) wParam;
	switch( msg->message ) {
	case TIM_CREATE:
		TrayIconInit(msg->hwnd);
      return FALSE;

	case WM_DESTROY:
		TrayIconDestroy(msg->hwnd);
      return FALSE;

	case TIM_CALLBACK:
		if (msg->lParam == WM_RBUTTONDOWN || msg->lParam == WM_LBUTTONDOWN) {
			KillTimer(pcli->hwndContactList, TIMERID_TRAYHOVER);
			CallService("mToolTip/HideTip", 0, 0);
		}
		if (msg->lParam == WM_MBUTTONUP) {
			pcli->pfnShowHide(0, 0);
		} else if (msg->lParam == (DBGetContactSettingByte(NULL, "CList", "Tray1Click", SETTING_TRAY1CLICK_DEFAULT) ? WM_LBUTTONUP : WM_LBUTTONDBLCLK)) {
			if ((GetAsyncKeyState(VK_CONTROL) & 0x8000))
				pcli->pfnShowHide(0, 0);
			else {
				if (pcli->pfnEventsProcessTrayDoubleClick())
					pcli->pfnShowHide(0, 0);
			}
		} 
		else if (msg->lParam == WM_MOUSEMOVE) {
			if(g_trayTooltipActive) {
				POINT pt;
				GetCursorPos(&pt);
				if(pt.x != tray_hover_pos.x || pt.y != tray_hover_pos.y) {
					CallService("mToolTip/HideTip", 0, 0);
					g_trayTooltipActive = FALSE;
				}
				break;
			}

			GetCursorPos(&tray_hover_pos);
			SetTimer(pcli->hwndContactList, TIMERID_TRAYHOVER, 600, TrayToolTipTimerProc);
		}
		else break;
		*((LRESULT *) lParam) = 0;
		return TRUE;
	}

	return saveTrayIconProcessMessage(wParam, lParam);
}

int CListTrayNotify(MIRANDASYSTRAYNOTIFY *msn)
{
    if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
		if (trayIcon) {
			NOTIFYICONDATAA nid = { 0 };
			nid.cbSize = sizeof(nid); // : NOTIFYICONDATAA_V1_SIZE;
			nid.hWnd = pcli->hwndContactList;
			if (msn->szProto) {
				int j;
				for (j = 0; j < trayIconCount; j++) {
					if (trayIcon[j].szProto != NULL) {
						if (strcmp(msn->szProto, trayIcon[j].szProto) == 0) {
							nid.uID = trayIcon[j].id;
							j = trayIconCount;
							continue;
						}
					}
					else {
						if (trayIcon[j].isBase) {
							nid.uID = trayIcon[j].id;
							j = trayIconCount;
							continue;
						}
					}
				}               //for
			}
			else {
				nid.uID = trayIcon[0].id;
                nid.hIcon = trayIcon[0].hBaseIcon;
			}
			nid.uFlags = NIF_INFO | NIF_ICON;
			lstrcpynA(nid.szInfo, msn->szInfo, sizeof(nid.szInfo));
			lstrcpynA(nid.szInfoTitle, msn->szInfoTitle, sizeof(nid.szInfoTitle));
			nid.uTimeout = msn->uTimeout;
			nid.dwInfoFlags = msn->dwInfoFlags;
			return Shell_NotifyIconA(NIM_MODIFY, (void *) &nid) == 0;
		}
		return 2;
	}
	return 1;
}

