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
#include "commonheaders.h"

void TrayIconSetToBase(char *szPreferredProto);

static int RemoveEvent(WPARAM wParam, LPARAM lParam);
static HWND hwndEventFrame = 0;
HFONT __fastcall ChangeToFont(HDC hdc, struct ClcData *dat, int id, int *fontHeight);

extern pfnDrawAlpha pDrawAlpha;
extern struct ClcData *g_clcData;

extern struct CluiData g_CluiData;
extern StatusItems_t *StatusItems;

HWND g_hwndEventArea = 0;

struct CListEvent {
	int imlIconIndex;
	int flashesDone;
	CLISTEVENT cle;

	int menuId;
	int imlIconOverlayIndex;
};

static struct CListEvent *event = NULL;
static int eventCount = 0;

struct CListImlIcon {
	int index;
	HICON hIcon;
};

static struct CListImlIcon *imlIcon;
static int imlIconCount;
static UINT flashTimerId;
static int iconsOn;
extern HIMAGELIST hCListImages;

int disableTrayFlash;
int disableIconFlash;

HANDLE hNotifyFrame = (HANDLE)-1;

void HideShowNotifyFrame()
{
	int dwVisible = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hNotifyFrame), 0) & F_VISIBLE;

	if(dwVisible) {
		if(!g_CluiData.notifyActive)
			CallService(MS_CLIST_FRAMES_SHFRAME, (WPARAM)hNotifyFrame, 0);
	}
	else {
		if(g_CluiData.notifyActive)
			CallService(MS_CLIST_FRAMES_SHFRAME, (WPARAM)hNotifyFrame, 0);
	}
}

LRESULT CALLBACK EventAreaWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
	case WM_CREATE:
		hwndEventFrame = hwnd;
		//g_CluiData.hwndNotifyButton = CreateWindowExA(0, "CLCButtonClass", "", BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 0, 0, 19, 16, hwnd, (HMENU) IDC_NOTIFYBUTTON, g_hInst, NULL);
		//SendMessage(g_CluiData.hwndNotifyButton, BUTTONSETASFLATBTN, 0, 0);
		//SendMessage(g_CluiData.hwndNotifyButton, BUTTONSETASFLATBTN + 10, 0, 0);
		//ShowWindow(g_CluiData.hwndNotifyButton, SW_SHOW);
		return FALSE;

	case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *lpi = (LPMEASUREITEMSTRUCT) lParam;
			MENUITEMINFOA mii = {0};

			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA | MIIM_ID;
			if (GetMenuItemInfoA(g_CluiData.hMenuNotify, lpi->itemID, FALSE, &mii) != 0) {
				if (mii.dwItemData == lpi->itemData) {
					lpi->itemWidth = 8 + 16;
					lpi->itemHeight = 0;
					return TRUE;
				}
			}
			break;
		}
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

			if (dis->hwndItem == (HWND) g_CluiData.hMenuNotify) {
				MENUITEMINFOA mii = {0};

				struct NotifyMenuItemExData *nmi = 0;
				int iIcon;
				HICON hIcon;

				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_DATA;
				if (GetMenuItemInfoA(g_CluiData.hMenuNotify, (UINT) dis->itemID, FALSE, &mii) != 0) {
					nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
					if (nmi) {
						iIcon = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) nmi->hContact, 0);
						hIcon = ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL);
						pcli->pfnDrawMenuItem(dis, hIcon, nmi->hIcon);
						return TRUE;
					}
				}
			}
			break;
		}
	case WM_LBUTTONUP:
		if(g_CluiData.bEventAreaEnabled)
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_NOTIFYBUTTON, 0), 0);
		break;
	case WM_COMMAND:
		if(LOWORD(wParam) == IDC_NOTIFYBUTTON) {
			int iSelection;
			MENUITEMINFO mii = {0};
			POINT pt;
			struct NotifyMenuItemExData *nmi = 0;
			int iCount = GetMenuItemCount(g_CluiData.hMenuNotify);
			BOOL result;

			GetCursorPos(&pt);
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA;
			if (iCount > 1)
				iSelection = TrackPopupMenu(g_CluiData.hMenuNotify, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
			else
				iSelection = GetMenuItemID(g_CluiData.hMenuNotify, 0);
			result = GetMenuItemInfo(g_CluiData.hMenuNotify, (UINT) iSelection, FALSE, &mii);
			if (result != 0) {
				nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
				if (nmi) {
					CLISTEVENT *cle = (CLISTEVENT *) MyGetEvent(0, (WPARAM) iSelection);
					if (cle) {
						CLISTEVENT *cle1 = NULL;
						CallService(cle->pszService, (WPARAM) NULL, (LPARAM) cle);
						// re-obtain the pointer, it may already be invalid/point to another event if the
						// event we're interested in was removed by the service (nasty one...)
						cle1 = (CLISTEVENT *) MyGetEvent(0, (WPARAM) iSelection);
						if (cle1 != NULL)
							CallService(MS_CLIST_REMOVEEVENT, (WPARAM) cle->hContact, (LPARAM) cle->hDbEvent);
					}
				}
			}
			break;
		}
		break;
	case WM_ERASEBKGND:
		return TRUE;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc, rcClient;
			HDC hdc = BeginPaint(hwnd, &ps);
			LONG dwLeft;
			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbm, hbmold;
			StatusItems_t *item;
			int height;
			HFONT hFontOld = 0;

			GetClientRect(hwnd, &rc);
			rcClient = rc;
			hbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
			hbmold = SelectObject(hdcMem, hbm);
			SetBkMode(hdcMem, TRANSPARENT);

			if(g_clcData)
				hFontOld = ChangeToFont(hdcMem, g_clcData, FONTID_EVENTAREA, &height);

			if(pDrawAlpha != NULL) {
				if(g_CluiData.bWallpaperMode)
					SkinDrawBg(hwnd, hdcMem);
				item = &StatusItems[ID_EXTBKEVTAREA - ID_STATUS_OFFLINE];
				if(item->IGNORED) {
					FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_3DFACE));
				}
				else {
					rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
					rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;

					DrawAlpha(hdcMem, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
						item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
					SetTextColor(hdcMem, item->TEXTCOLOR);
				}
			}
			else
				FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_3DFACE));

			dwLeft = rc.left;

			PaintNotifyArea(hdcMem, &rc);
			if(g_CluiData.bUseFloater & CLUI_USE_FLOATER && g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS)
				SFL_Update(0, 0, 0, NULL, FALSE);
			if(g_CluiData.dwFlags & CLUI_FRAME_EVENTAREASUNKEN) {
				rc.left = dwLeft;
				InflateRect(&rc, -2, -2);
				DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT);
			}
			BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hbmold);
			if(hFontOld)
				SelectObject(hdcMem, hFontOld);
			DeleteObject(hbm);
			DeleteDC(hdcMem);
			ps.fErase = FALSE;
			EndPaint(hwnd, &ps);
			return 0;
		}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return TRUE;
}

static int GetImlIconIndex(HICON hIcon)
{
	int i;

	for (i = 0; i < imlIconCount; i++) {
		if (imlIcon[i].hIcon == hIcon)
			return imlIcon[i].index;
	}
	imlIcon = (struct CListImlIcon *) mir_realloc(imlIcon, sizeof(struct CListImlIcon) * (imlIconCount + 1));
	imlIconCount++;
	imlIcon[i]. hIcon = hIcon;
	imlIcon[i]. index = ImageList_AddIcon(hCListImages, hIcon);
	return imlIcon[i].index;
}

static VOID CALLBACK IconFlashTimer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
	int i, j;

	if (eventCount) {
		char *szProto;
		if (event[0].cle.hContact == NULL)
			szProto = NULL;
		else
			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) event[0].cle.hContact, 0);
		pcli->pfnTrayIconUpdateWithImageList((iconsOn || disableTrayFlash) ? event[0].imlIconIndex : event[0].imlIconOverlayIndex, event[0].cle.ptszTooltip, szProto);
	}

	for (i = 0; i < eventCount; i++) {
		if (event[i].cle.flags & CLEF_ONLYAFEW) {
			if (0 == --event[i].flashesDone) {
				RemoveEvent((WPARAM) event[i].cle.hContact, (LPARAM) event[i].cle.hDbEvent);
				continue;
			}
		}
		for (j = 0; j < i; j++) {
			if (event[j].cle.hContact == event[i].cle.hContact)
				break;
		}
		if (j < i)
			continue;
		pcli->pfnChangeContactIcon(event[i].cle.hContact, iconsOn || disableIconFlash ? event[i].imlIconIndex : event[i].imlIconOverlayIndex, 0);
	}
	iconsOn = !iconsOn;
}

int AddEvent(WPARAM wParam, LPARAM lParam)
{
	MENUITEMINFO mii = {0};
	CLISTEVENT *cle = (CLISTEVENT *) lParam;
	int i;
	TCHAR szBuffer[128];
	HANDLE hItem;

	if (cle->cbSize != sizeof(CLISTEVENT))
		return 1;
	event = (struct CListEvent *) mir_realloc(event, sizeof(struct CListEvent) * (eventCount + 1));
	if (cle->flags & CLEF_URGENT) {
		for (i = 0; i < eventCount; i++) {
			if (!(cle->flags & CLEF_URGENT))
				break;
		}
	} else
		i = eventCount;
	MoveMemory(&event[i + 1], &event[i], sizeof(struct CListEvent) * (eventCount - i));
	event[i]. cle = *cle;
	event[i]. imlIconIndex = GetImlIconIndex(event[i].cle.hIcon);
	event[i]. imlIconOverlayIndex = 0;
	event[i]. flashesDone = 12;
	event[i].cle. pszService = mir_strdup(event[i].cle.pszService);
	#ifdef UNICODE
		if (event[i].cle.flags & CLEF_UNICODE)
			event[i].cle.ptszTooltip = _tcsdup((TCHAR*)event[i].cle.pszTooltip);
		else
			event[i].cle.ptszTooltip = a2u((char*)event[i].cle.pszTooltip); //if no flag defined it handled as unicode
	#else
		event[i].cle.pszTooltip = _strdup(event[i].cle.pszTooltip);//TODO convert from utf to mb
	#endif
	event[i]. menuId = 0;
	eventCount++;
	if (eventCount == 1) {
		char *szProto;
		if (event[0].cle.hContact == NULL)
			szProto = NULL;
		else
			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) event[0].cle.hContact, 0);
		iconsOn = 1;
		flashTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "IconFlashTime", 550), IconFlashTimer);
		//szFinalProto = DivertProtoToSubcontact(event[0].cle.hContact, szProto);
		pcli->pfnTrayIconUpdateWithImageList(iconsOn ? event[0].imlIconIndex : event[0].imlIconOverlayIndex, event[0].cle.ptszTooltip, szProto);
	}
	pcli->pfnChangeContactIcon(cle->hContact, event[eventCount - 1].imlIconIndex, 1);
	//SortContacts();
	if (g_CluiData.dwFlags & CLUI_FRAME_USEEVENTAREA) {
		if (event[i].cle.hContact != 0 && event[i].cle.hDbEvent != (HANDLE) 1 && !(event[i].cle.flags & CLEF_ONLYAFEW)) {
			int j;
			struct NotifyMenuItemExData *nmi = 0;
			char *szProto;
			TCHAR *szName;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA | MIIM_BITMAP | MIIM_ID;
			if (event[i].cle.pszService && !strncmp("SRMsg/ReadMessage", event[i].cle.pszService, 17)) {
				// dup check only for msg events
				for (j = 0; j < GetMenuItemCount(g_CluiData.hMenuNotify); j++) {
					if (GetMenuItemInfo(g_CluiData.hMenuNotify, j, TRUE, &mii) != 0) {
						nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
						if (nmi != 0 && (HANDLE) nmi->hContact == (HANDLE) event[i].cle.hContact && nmi->iIcon == event[i].imlIconIndex)
							goto duplicate;
					}
				}
			}
			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) event[i].cle.hContact, 0);
			szName = pcli->pfnGetContactDisplayName(event[i].cle.hContact, 0);
			if (szProto && szName) {
				nmi = (struct NotifyMenuItemExData *) malloc(sizeof(struct NotifyMenuItemExData));
				if (nmi) {
					TCHAR* szStatus = pcli->pfnGetStatusModeDescription(DBGetContactSettingWord(event[i].cle.hContact, szProto, "Status", ID_STATUS_OFFLINE), 0);
#if defined(_UNICODE)
					TCHAR szwProto[64];
					MultiByteToWideChar(CP_ACP, 0, szProto, -1, szwProto, 64);
					szwProto[63] = 0;
					_snwprintf(szBuffer, SIZEOF(szBuffer), L"%s: %s (%s)", szwProto, szName, szStatus);
#else
					_snprintf(szBuffer, SIZEOF(szBuffer), "%s: %s (%s)", szProto, szName, szStatus);
#endif
					szBuffer[127] = 0;
					AppendMenu(g_CluiData.hMenuNotify, MF_BYCOMMAND | MF_STRING, g_CluiData.wNextMenuID, szBuffer);
					mii.hbmpItem = HBMMENU_CALLBACK;
					nmi->hContact = event[i].cle.hContact;
					nmi->iIcon = event[i].imlIconIndex;
					nmi->hIcon = event[i].cle.hIcon;
					nmi->hDbEvent = event[i].cle.hDbEvent;
					mii.dwItemData = (ULONG) nmi;
					mii.wID = g_CluiData.wNextMenuID;
					SetMenuItemInfo(g_CluiData.hMenuNotify, g_CluiData.wNextMenuID, FALSE, &mii);
					event[i]. menuId = g_CluiData.wNextMenuID;
					g_CluiData.wNextMenuID++;
					if (g_CluiData.wNextMenuID > 0x7fff)
						g_CluiData.wNextMenuID = 1;
					g_CluiData.hIconNotify = event[i].imlIconIndex;
				}
			}
		} else if (event[i].cle.hContact != 0 && (event[i].cle.flags & CLEF_ONLYAFEW)) {
			g_CluiData.hIconNotify = event[i].imlIconIndex;
			g_CluiData.hUpdateContact = event[i].cle.hContact;
		}
		if (g_CluiData.dwFlags & CLUI_STICKYEVENTS) {
			hItem = (HANDLE) SendMessage(pcli->hwndContactTree, CLM_FINDCONTACT, (WPARAM) event[i].cle.hContact, 0);
			if (hItem) {
				SendMessage(pcli->hwndContactTree, CLM_SETSTICKY, (WPARAM) hItem, 1);
				pcli->pfnClcBroadcast(INTM_PROTOCHANGED, (WPARAM) event[i].cle.hContact, 0);
			}
		}
		if (eventCount > 0) {
			g_CluiData.bEventAreaEnabled = TRUE;
			if (g_CluiData.notifyActive == 0) {
				g_CluiData.notifyActive = 1;
				HideShowNotifyFrame();
			}
		}
		InvalidateRect(hwndEventFrame, NULL, FALSE);
	}
duplicate : return 0;
}



// Removes an event from the contact list's queue
// wParam=(WPARAM)(HANDLE)hContact
// lParam=(LPARAM)(HANDLE)hDbEvent
// Returns 0 if the event was successfully removed, or nonzero if the event was not found
int RemoveEvent(WPARAM wParam, LPARAM lParam)
{
	HANDLE hItem;
	int i, j;
	char *szProto;
	MENUITEMINFO mii = {0};
	BOOL bUnstick = TRUE;

	// Find the event that should be removed
	for (i = 0; i < eventCount; i++) {
		if ((event[i].cle.hContact == (HANDLE) wParam) && (event[i].cle.hDbEvent == (HANDLE) lParam)) {
			break;
		}
	}

	// Event was not found
	if (i == eventCount)
		return 1;

	// Update contact's icon
	szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);

	pcli->pfnChangeContactIcon(event[i].cle.hContact, IconFromStatusMode(szProto, szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord(event[i].cle.hContact, szProto, "Status", ID_STATUS_OFFLINE), event[i].cle.hContact, NULL), 0);

	// remove event from the notify menu

	if (g_CluiData.dwFlags & CLUI_FRAME_USEEVENTAREA) {
		if (event[i].menuId > 0) {
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA;
			if (GetMenuItemInfo(g_CluiData.hMenuNotify, event[i].menuId, FALSE, &mii) != 0) {
				struct NotifyMenuItemExData *nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
				if (nmi && nmi->hContact == (HANDLE) wParam && nmi->hDbEvent == (HANDLE) lParam) {
					free(nmi);
					DeleteMenu(g_CluiData.hMenuNotify, event[i].menuId, MF_BYCOMMAND);
					//_snprintf(szBuffer, sizeof(szBuffer), "remove: %d", event[i].menuId);
					//PUShowMessage(szBuffer, SM_WARNING);
				}
			}
		}
	}

	// Free any memory allocated to the event
	if (event[i].cle.pszService != NULL) {
		mir_free(event[i].cle.pszService);
		event[i].cle. pszService = NULL;
	}
	if (event[i].cle.pszTooltip != NULL) {
		mir_free(event[i].cle.pszTooltip);
		event[i].cle. pszTooltip = NULL;
	}

	// Shift events in array
	// The array size is not adjusted until a new event is added though.
	if (eventCount > 1)
		MoveMemory(&event[i], &event[i + 1], sizeof(struct CListEvent) * (eventCount - i - 1));

	eventCount--;

	if (eventCount == 0) {
		KillTimer(NULL, flashTimerId);
		TrayIconSetToBase((HANDLE) wParam == NULL ? NULL : szProto);
		//EnableWindow(g_CluiData.hwndNotifyButton, FALSE);
		g_CluiData.bEventAreaEnabled = FALSE;
		if (g_CluiData.dwFlags & CLUI_FRAME_AUTOHIDENOTIFY) {
			g_CluiData.notifyActive = 0;
			HideShowNotifyFrame();
		}
	} else {
		for (j = 0; j < eventCount; j++) {
			if (event[j].cle.hContact == (HANDLE) wParam) {
				bUnstick = FALSE;
				break;
			}
		}
		if (event[0].cle.hContact == NULL)
			szProto = NULL;
		else
			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) event[0].cle.hContact, 0);
		pcli->pfnTrayIconUpdateWithImageList(iconsOn ? event[0].imlIconIndex : event[0].imlIconOverlayIndex, event[0].cle.ptszTooltip, szProto);
	}

	if (bUnstick) {
		// clear "sticky" (sort) status

		hItem = (HANDLE) SendMessage(pcli->hwndContactTree, CLM_FINDCONTACT, wParam, 0);
		if (hItem) {
			SendMessage(pcli->hwndContactTree, CLM_SETSTICKY, (WPARAM) hItem, 0);
			pcli->pfnClcBroadcast(INTM_PROTOCHANGED, wParam, 0);
		}
	}

	if ((HANDLE) wParam == g_CluiData.hUpdateContact || lParam == 1)
		g_CluiData.hUpdateContact = 0;

	if (g_CluiData.notifyActive)
		InvalidateRect(hwndEventFrame, NULL, FALSE);
	return 0;
}


int MyGetEvent(WPARAM wParam, LPARAM lParam)
{
	int i;

	for (i = 0; i < eventCount; i++) {
		if (event[i].menuId == (int) lParam)
			return(int) &event[i].cle;
	}
	return(int) (CLISTEVENT *) NULL;
}

void UninitCListEvents(void)
{
	int i;

	for (i = 0; i < eventCount; i++) {
		if (event[i].cle.pszService)
			mir_free(event[i].cle.pszService);
		if (event[i].cle.pszTooltip)
			mir_free(event[i].cle.pszTooltip);
	}
	if (event != NULL)
		mir_free(event);
	if (imlIcon != NULL)
		mir_free(imlIcon);
}