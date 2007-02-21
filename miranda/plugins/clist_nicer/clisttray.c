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

extern HIMAGELIST hCListImages;

extern struct CluiData g_CluiData;

BOOL g_trayTooltipActive = FALSE;
POINT tray_hover_pos = {0};

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010

void TrayIconUpdateBase(const char *szChangedProto)
{
	int i,count,netProtoCount,changed = -1;
	PROTOCOLDESCRIPTOR **protos;
	int averageMode = 0;
	HWND hwnd = pcli->hwndContactList;

	if (pcli->cycleTimerId) {
		KillTimer(NULL, pcli->cycleTimerId); pcli->cycleTimerId = 0;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
	for (i = 0,netProtoCount = 0; i < count; i++) {
		if (protos[i]->type != PROTOTYPE_PROTOCOL || !pcli->pfnGetProtocolVisibility(protos[i]->szName))
			continue;
		netProtoCount++;
		if (!lstrcmpA(szChangedProto, protos[i]->szName))
			pcli->cycleStep = i;
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
						changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), szChangedProto);
					else
						changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), szChangedProto);
				}
				else if (pcli->trayIcon && pcli->trayIcon[0].szProto != NULL) {
					pcli->pfnTrayIconDestroy(hwnd);
					pcli->pfnTrayIconInit(hwnd);
				}
				else {
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

					if(hIcon)
						changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
					else
						changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
				}
			} else {
				HICON hIcon = 0;
				int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

				if(hIcon)
					changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
				else
					changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
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
						changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
					else
						changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
					DBFreeVariant(&dbv);
					break;
				}
			case SETTING_TRAYICON_CYCLE:
				{
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), 0, &hIcon);

					pcli->cycleTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, pcli->pfnTrayCycleTimerProc);
					if(hIcon)
						changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
					else
						changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
					break;
				}
			case SETTING_TRAYICON_MULTI:
				if ( !pcli->trayIcon )
					pcli->pfnTrayIconRemove(NULL, NULL);
				else if (DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT)) {
					HICON hIcon = 0;
					int iIcon = IconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), 0, &hIcon);

					if(hIcon)
						changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), szChangedProto);
					else
						changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), szChangedProto);
				}
				else {
					pcli->pfnTrayIconDestroy(hwnd);
					pcli->pfnTrayIconInit(hwnd);
				}
				break;
			}
		}
	} else {
		HICON hIcon = 0;
		int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

		if(hIcon)
			changed = pcli->pfnTrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
		else
			changed = pcli->pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
	}
	if (changed != -1 && pcli->trayIcon[changed].isBase)
		pcli->pfnTrayIconUpdate( pcli->trayIcon[changed].hBaseIcon, NULL, pcli->trayIcon[changed].szProto, 1);
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
			{	char* p = u2a( pcli->szTip );
	        	CallService("mToolTip/ShowTip", (WPARAM)p, (LPARAM)&ti);
				free( p );
			}
			#else
	        	CallService("mToolTip/ShowTip", (WPARAM)pcli->szTip, (LPARAM)&ti);
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
