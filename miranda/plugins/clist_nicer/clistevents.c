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
#include "cluiframes/cluiframes.h"

static HWND hwndEventFrame = 0;
HFONT __fastcall ChangeToFont(HDC hdc, struct ClcData *dat, int id, int *fontHeight);

extern struct CListEvent* ( *saveAddEvent )(CLISTEVENT *cle);
extern void ( *saveRemoveEvent )(HANDLE hContact, HANDLE hDbEvent);
extern wndFrame *wndFrameEventArea;

extern pfnDrawAlpha pDrawAlpha;
extern struct ClcData *g_clcData;
extern HPEN g_hPenCLUIFrames;

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

struct CListImlIcon {
	int index;
	HICON hIcon;
};

static int iconsOn;
extern HIMAGELIST hCListImages;

HANDLE hNotifyFrame = (HANDLE)-1;

struct CListEvent* fnCreateEvent( void )
{
	struct CListEvent *p = mir_alloc(sizeof(struct CListEvent));
	if(p)
		ZeroMemory(p, sizeof(struct CListEvent));

	return p;
}

void HideShowNotifyFrame()
{
	int dwVisible = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, hNotifyFrame), 0) & F_VISIBLE;
    int desired;

    if(g_CluiData.dwFlags & CLUI_FRAME_AUTOHIDENOTIFY)
        desired = g_CluiData.notifyActive ? TRUE : FALSE;
    else
        desired = dwVisible;

    if(desired) {
		if(!dwVisible)
			CallService(MS_CLIST_FRAMES_SHFRAME, (WPARAM)hNotifyFrame, 0);
	}
	else {
		if(dwVisible)
			CallService(MS_CLIST_FRAMES_SHFRAME, (WPARAM)hNotifyFrame, 0);
	}
}

static CLISTEVENT* MyGetEvent(int iSelection)
{
	int i;

	for (i = 0; i < pcli->events.count; i++) {
		struct CListEvent* p = pcli->events.items[i];
		if (p->menuId == iSelection)
			return &p->cle;
	}
	return NULL;
}

LRESULT CALLBACK EventAreaWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL hasTitleBar = wndFrameEventArea ? wndFrameEventArea->TitleBar.ShowTitleBar : 0;

    switch(msg) {
	case WM_CREATE:
		hwndEventFrame = hwnd;
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
    case WM_NCCALCSIZE:
        return FrameNCCalcSize(hwnd, DefWindowProc, wParam, lParam, hasTitleBar);
    case WM_NCPAINT:
        return FrameNCPaint(hwnd, DefWindowProc, wParam, lParam, hasTitleBar);
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
					CLISTEVENT *cle = MyGetEvent(iSelection);
					if (cle) {
						CLISTEVENT *cle1 = NULL;
						CallService(cle->pszService, (WPARAM) NULL, (LPARAM) cle);
						// re-obtain the pointer, it may already be invalid/point to another event if the
						// event we're interested in was removed by the service (nasty one...)
						cle1 = MyGetEvent(iSelection);
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

struct CListEvent* AddEvent(CLISTEVENT *cle)
{
	struct CListEvent* p = saveAddEvent(cle);
	if ( p == NULL )
		return NULL;

	if (1) {
		if (p->cle.hContact != 0 && p->cle.hDbEvent != (HANDLE) 1 && !(p->cle.flags & CLEF_ONLYAFEW)) {
			int j;
			struct NotifyMenuItemExData *nmi = 0;
			char *szProto;
			TCHAR *szName;
			MENUITEMINFO mii = {0};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA | MIIM_BITMAP | MIIM_ID;
			if (p->cle.pszService && !strncmp("SRMsg/ReadMessage", p->cle.pszService, 17)) {
				// dup check only for msg events
				for (j = 0; j < GetMenuItemCount(g_CluiData.hMenuNotify); j++) {
					if (GetMenuItemInfo(g_CluiData.hMenuNotify, j, TRUE, &mii) != 0) {
						nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
						if (nmi != 0 && (HANDLE) nmi->hContact == (HANDLE) p->cle.hContact && nmi->iIcon == p->imlIconIndex)
							return p;
			}	}	}

			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) p->cle.hContact, 0);
			szName = pcli->pfnGetContactDisplayName(p->cle.hContact, 0);
			if (szProto && szName) {
				nmi = (struct NotifyMenuItemExData *) malloc(sizeof(struct NotifyMenuItemExData));
				if (nmi) {
					TCHAR szBuffer[128];
					TCHAR* szStatus = pcli->pfnGetStatusModeDescription(DBGetContactSettingWord(p->cle.hContact, szProto, "Status", ID_STATUS_OFFLINE), 0);
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
					nmi->hContact = p->cle.hContact;
					nmi->iIcon = p->imlIconIndex;
					nmi->hIcon = p->cle.hIcon;
					nmi->hDbEvent = p->cle.hDbEvent;
					mii.dwItemData = (ULONG) nmi;
					mii.wID = g_CluiData.wNextMenuID;
					SetMenuItemInfo(g_CluiData.hMenuNotify, g_CluiData.wNextMenuID, FALSE, &mii);
					p-> menuId = g_CluiData.wNextMenuID;
					g_CluiData.wNextMenuID++;
					if (g_CluiData.wNextMenuID > 0x7fff)
						g_CluiData.wNextMenuID = 1;
					g_CluiData.hIconNotify = p->imlIconIndex;
				}
			}
		} else if (p->cle.hContact != 0 && (p->cle.flags & CLEF_ONLYAFEW)) {
			g_CluiData.hIconNotify = p->imlIconIndex;
			g_CluiData.hUpdateContact = p->cle.hContact;
		}
		if (g_CluiData.dwFlags & CLUI_STICKYEVENTS) {
			HANDLE hItem = (HANDLE) SendMessage(pcli->hwndContactTree, CLM_FINDCONTACT, (WPARAM) p->cle.hContact, 0);
			if (hItem) {
				SendMessage(pcli->hwndContactTree, CLM_SETSTICKY, (WPARAM) hItem, 1);
				pcli->pfnClcBroadcast(INTM_PROTOCHANGED, (WPARAM) p->cle.hContact, 0);
			}
		}
		if (pcli->events.count > 0) {
			g_CluiData.bEventAreaEnabled = TRUE;
			if (g_CluiData.notifyActive == 0) {
				g_CluiData.notifyActive = 1;
				HideShowNotifyFrame();
			}
		}
		InvalidateRect(hwndEventFrame, NULL, FALSE);
		if(g_CluiData.bUseFloater & CLUI_USE_FLOATER && g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS)
			SFL_Update(0, 0, 0, NULL, FALSE);
	}

	return p;
}

// Removes an event from the contact list's queue
// wParam=(WPARAM)(HANDLE)hContact
// lParam=(LPARAM)(HANDLE)hDbEvent
// Returns 0 if the event was successfully removed, or nonzero if the event was not found
int RemoveEvent(HANDLE hContact, HANDLE hDbEvent)
{
	HANDLE hItem;
	int i;
	BOOL bUnstick = TRUE;

	// Find the event that should be removed
	for (i = 0; i < pcli->events.count; i++) {
		if ((pcli->events.items[i]->cle.hContact == hContact) && (pcli->events.items[i]->cle.hDbEvent == hDbEvent)) {
			break;
		}
	}

	// Event was not found
	if (i == pcli->events.count)
		return 1;

	// remove event from the notify menu
	if (1) {
		if (pcli->events.items[i]->menuId > 0) {
			MENUITEMINFO mii = {0};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA;
			if (GetMenuItemInfo(g_CluiData.hMenuNotify, pcli->events.items[i]->menuId, FALSE, &mii) != 0) {
				struct NotifyMenuItemExData *nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
				if (nmi && nmi->hContact == hContact && nmi->hDbEvent == hDbEvent) {
					free(nmi);
					DeleteMenu(g_CluiData.hMenuNotify, pcli->events.items[i]->menuId, MF_BYCOMMAND);
	}	}	}	}

	saveRemoveEvent(hContact, hDbEvent);

	if (pcli->events.count == 0) {
		g_CluiData.bEventAreaEnabled = FALSE;
		if (g_CluiData.dwFlags & CLUI_FRAME_AUTOHIDENOTIFY) {
			g_CluiData.notifyActive = 0;
			HideShowNotifyFrame();
	}	}

	if (bUnstick) {
		// clear "sticky" (sort) status

		hItem = (HANDLE) SendMessage(pcli->hwndContactTree, CLM_FINDCONTACT, (WPARAM)hContact, 0);
		if (hItem) {
			SendMessage(pcli->hwndContactTree, CLM_SETSTICKY, (WPARAM) hItem, 0);
			pcli->pfnClcBroadcast(INTM_PROTOCHANGED, (WPARAM)hContact, 0);
	}	}

	if (hContact == g_CluiData.hUpdateContact || (int)hDbEvent == 1)
		g_CluiData.hUpdateContact = 0;

	if (g_CluiData.notifyActive) {
		InvalidateRect(hwndEventFrame, NULL, FALSE);
		if(g_CluiData.bUseFloater & CLUI_USE_FLOATER && g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS)
			SFL_Update(0, 0, 0, NULL, FALSE);
	}

	return 0;
}
