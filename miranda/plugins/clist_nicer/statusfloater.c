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

------------------------

implements a simple status floater as a layered (and skinnable) window with
a minimalistic UI (change status, access main menu). It also may hold a copy
of the event area.

*/

#include "commonheaders.h"

BYTE __forceinline percent_to_byte(UINT32 percent)
{
    return(BYTE) ((FLOAT) (((FLOAT) percent) / 100) * 255);
}

HWND g_hwndSFL = 0;
HDC g_SFLCachedDC = 0;
HBITMAP g_SFLhbmOld = 0, g_SFLhbm = 0;

extern StatusItems_t *StatusItems;
extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern BOOL (WINAPI *MyUpdateLayeredWindow)(HWND hwnd, HDC hdcDst, POINT *pptDst,SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags);
extern int g_nextExtraCacheEntry;
extern struct ExtraCache *g_ExtraCache;

extern struct CluiData g_CluiData;
extern HIMAGELIST hCListImages;
extern HWND g_hwndEventArea;
extern struct ClcData *g_clcData;
extern HDC g_HDC;

#define MAXFLOATERS 2

static int g_floaters[MAXFLOATERS];

LRESULT CALLBACK StatusFloaterClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_DESTROY:
			Utils_SaveWindowPosition(hwnd, 0, "CLUI", "sfl");
			if(g_SFLCachedDC) {
				SelectObject(g_SFLCachedDC, g_SFLhbmOld);
				DeleteObject(g_SFLhbm);
				DeleteDC(g_SFLCachedDC);
				g_SFLCachedDC = 0;
			}
			break;
		case WM_ERASEBKGND:
			{
				RECT rcClient;

				GetClientRect(hwnd, &rcClient);
				//FillRect((HDC)wParam, &rcClient, GetSysColorBrush(COLOR_3DFACE));
				return TRUE;
			}
		case WM_PAINT:
			{
				HDC hdc;
				PAINTSTRUCT ps;

				hdc = BeginPaint(hwnd, &ps);
				ps.fErase = FALSE;
				EndPaint(hwnd, &ps);
				return TRUE;
			}
        case WM_LBUTTONDOWN:
            {
                POINT ptMouse;
				RECT rcWindow;

				GetCursorPos(&ptMouse);
				GetWindowRect(hwnd, &rcWindow);
				rcWindow.right = rcWindow.left + 25;
				if(!PtInRect(&rcWindow, ptMouse))
					return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(ptMouse.x, ptMouse.y));
				break;
			}
		case WM_LBUTTONUP:
			{
                HMENU hmenu = (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
				RECT rcWindow;
				POINT pt;

				GetCursorPos(&pt);
				GetWindowRect(hwnd, &rcWindow);
				if(g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS) {
					if(pt.y > rcWindow.top + ((rcWindow.bottom - rcWindow.top) / 2))
						SendMessage(g_hwndEventArea, WM_COMMAND, MAKEWPARAM(IDC_NOTIFYBUTTON, 0), 0);
					else
						TrackPopupMenu(hmenu, TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON, rcWindow.left, rcWindow.bottom, 0, pcli->hwndContactList, NULL);
				}
				else
					TrackPopupMenu(hmenu, TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON, rcWindow.left, rcWindow.bottom, 0, pcli->hwndContactList, NULL);
				return 0;
			}
		case WM_CONTEXTMENU:
			{
                HMENU hmenu = (HMENU)CallService(MS_CLIST_MENUGETMAIN, 0, 0);
				RECT rcWindow;

				GetWindowRect(hwnd, &rcWindow);
                TrackPopupMenu(hmenu, TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON, rcWindow.left, rcWindow.bottom, 0, pcli->hwndContactList, NULL);
				return 0;
			}

	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ContactFloaterClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int iEntry = GetWindowLong(hwnd, GWL_USERDATA);
	struct ExtraCache *centry = NULL;

	if(iEntry >= 0 && iEntry <= g_nextExtraCacheEntry)
		centry = &g_ExtraCache[iEntry];

	switch(msg) {
		case WM_NCCREATE:
			{
				CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
				SetWindowLong(hwnd, GWL_USERDATA, (LONG)cs->lpCreateParams);
				iEntry = (int)cs->lpCreateParams;
				if(iEntry >= 0 && iEntry <= g_nextExtraCacheEntry)
					centry = &g_ExtraCache[iEntry];
				_DebugPopup(centry->hContact, "created flt as %d", iEntry);
				return TRUE;
			}
		case WM_DESTROY:
			if(centry) {
                WINDOWPLACEMENT wp = {0};

				SelectObject(centry->floater.hdc, centry->floater.hbmOld);
				DeleteObject(centry->floater.hbm);
				DeleteDC(centry->floater.hdc);
				_DebugPopup(centry->hContact, "dc killed", centry->floater.hdc);
				if(iEntry >= 0 && iEntry < g_nextExtraCacheEntry) {
					int i;

					for(i = 0; i < MAXFLOATERS; i++) {
						if(g_floaters[i] == iEntry) {
							g_floaters[i] = -1;
							_DebugPopup(centry->hContact, "removed floater #%d", i);
							break;
						}
					}
					Utils_SaveWindowPosition(hwnd, centry->hContact, "CList", "flt");
				}
				break;
			}
		case WM_PAINT:
			{
				HDC hdc;
				PAINTSTRUCT ps;

				hdc = BeginPaint(hwnd, &ps);
				ps.fErase = FALSE;
				EndPaint(hwnd, &ps);
				return TRUE;
			}
        case WM_LBUTTONDOWN:
            {
                POINT ptMouse;
				RECT rcWindow;

				GetCursorPos(&ptMouse);
				GetWindowRect(hwnd, &rcWindow);
				rcWindow.right = rcWindow.left + 25;
				if(!PtInRect(&rcWindow, ptMouse))
					return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(ptMouse.x, ptMouse.y));
				break;
			}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SFL_RegisterWindowClass()
{
	WNDCLASS wndclass;

    wndclass.style = 0;
    wndclass.lpfnWndProc = StatusFloaterClassProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = g_hInst;
    wndclass.hIcon = 0;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_3DFACE);
    wndclass.lpszMenuName = 0;
    wndclass.lpszClassName = _T("StatusFloaterClass");
    RegisterClass(&wndclass);

    wndclass.lpszClassName = _T("ContactFloaterClass");
    wndclass.lpfnWndProc = ContactFloaterClassProc;
    RegisterClass(&wndclass);

	memset(g_floaters, -1, sizeof(int) * MAXFLOATERS);
}

void SFL_UnregisterWindowClass()
{
	UnregisterClass(_T("StatusFloaterClass"), g_hInst);
	UnregisterClass(_T("ContactFloaterClass"), g_hInst);
}

void SFL_Destroy()
{
	if(g_hwndSFL)
		DestroyWindow(g_hwndSFL);
	g_hwndSFL = 0;
}

static HICON sfl_hIcon = (HICON)-1;
static int sfl_iIcon = -1;
static char sfl_statustext[100] = "";

void SFL_Update(HICON hIcon, int iIcon, HIMAGELIST hIml, const char *szText, BOOL refresh)
{
	RECT rcClient, rcWindow;
	POINT ptDest, ptSrc = {0};
	SIZE szDest, szT;
	BLENDFUNCTION bf = {0};
	HFONT hOldFont;
	StatusItems_t *item = &StatusItems[ID_EXTBKSTATUSFLOATER - ID_STATUS_OFFLINE];
	RECT rcStatusArea;
	LONG cy;

	if(g_hwndSFL == 0)
		return;

	GetClientRect(g_hwndSFL, &rcClient);
	GetWindowRect(g_hwndSFL, &rcWindow);

	ptDest.x = rcWindow.left;
	ptDest.y = rcWindow.top;
	szDest.cx = rcWindow.right - rcWindow.left;
	szDest.cy = rcWindow.bottom - rcWindow.top;

	if(item->IGNORED) {
		FillRect(g_SFLCachedDC, &rcClient, GetSysColorBrush(COLOR_3DFACE));
		SetTextColor(g_SFLCachedDC, GetSysColor(COLOR_BTNTEXT));
	}
	else {
		FillRect(g_SFLCachedDC, &rcClient, GetSysColorBrush(COLOR_3DFACE));
		DrawAlpha(g_SFLCachedDC, &rcClient, item->COLOR, 100, item->COLOR2, item->COLOR2_TRANSPARENT,
			item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
		SetTextColor(g_SFLCachedDC, item->TEXTCOLOR);
	}
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = 0;
	bf.SourceConstantAlpha = item->IGNORED ? 255 : percent_to_byte(item->ALPHA);

	rcStatusArea = rcClient;

	if(g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS)
		rcStatusArea.bottom = 20;

	cy = rcStatusArea.bottom - rcStatusArea.top;

	if(szText != NULL && refresh) {
		strncpy(sfl_statustext, szText, 100);
		sfl_statustext[99] = 0;
	}

	if(!hIcon) {
		HICON p_hIcon;

		if(refresh)
			sfl_iIcon = iIcon;
		if(sfl_iIcon != -1) {
			p_hIcon = ImageList_ExtractIcon(0, hCListImages, sfl_iIcon);
			DrawIconEx(g_SFLCachedDC, 5, (cy - 16) / 2, p_hIcon, 16, 16, 0, 0, DI_NORMAL);
			DestroyIcon(p_hIcon);
		}
	}
	else {
		if(refresh)
			sfl_hIcon = hIcon;
		if(sfl_hIcon != (HICON)-1)
			DrawIconEx(g_SFLCachedDC, 5, (cy - 16) / 2, sfl_hIcon, 16, 16, 0, 0, DI_NORMAL);
	}

	hOldFont = SelectObject(g_SFLCachedDC, GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(g_SFLCachedDC, TRANSPARENT);
	GetTextExtentPoint32A(g_SFLCachedDC, sfl_statustext, lstrlenA(sfl_statustext), &szT);
	TextOutA(g_SFLCachedDC, 24, (cy - szT.cy) / 2, sfl_statustext, lstrlenA(sfl_statustext));

	if(g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS) {
		RECT rcNA = rcClient;

		rcNA.top = 18;
		PaintNotifyArea(g_SFLCachedDC, &rcNA);
	}

	SelectObject(g_SFLCachedDC, hOldFont);

	if(MyUpdateLayeredWindow)
		MyUpdateLayeredWindow(g_hwndSFL, 0, &ptDest, &szDest, g_SFLCachedDC, &ptSrc, GetSysColor(COLOR_3DFACE), &bf, ULW_ALPHA | ULW_COLORKEY);
}

/*
 * set the floater
 * mode = 0/1 forced hide/show
 * OR -1 to set it depending on the clist state (visible/hidden) (this is actually reversed, because the function
 * is called *before* the clist is shown or hidden)
 */

void SFL_SetState(int uMode)
{
	BYTE bClistState;

	if(g_hwndSFL == 0 || !(g_CluiData.bUseFloater & CLUI_USE_FLOATER))
		return;

	if(uMode == -1) {
		if(g_CluiData.bUseFloater & CLUI_FLOATER_AUTOHIDE) {
			bClistState = DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);
			ShowWindow(g_hwndSFL, bClistState == SETTING_STATE_NORMAL ? SW_SHOW : SW_HIDE);
		}
		else
			ShowWindow(g_hwndSFL, SW_SHOW);
	}
	else
		ShowWindow(g_hwndSFL, uMode ? SW_SHOW : SW_HIDE);
}

// XXX improve size calculations for the floater window.

void SFL_SetSize()
{
	HDC hdc;
	LONG lWidth;
	RECT rcWindow;
	SIZE sz;
	char *szStatusMode;
	HFONT oldFont;
	int i;

	GetWindowRect(g_hwndSFL, &rcWindow);
	lWidth = rcWindow.right - rcWindow.left;

	hdc = GetDC(g_hwndSFL);
	oldFont = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	for(i = ID_STATUS_OFFLINE; i <= ID_STATUS_OUTTOLUNCH; i++) {
		szStatusMode = Translate((char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)i, 0));
		GetTextExtentPoint32A(hdc, szStatusMode, lstrlenA(szStatusMode), &sz);
		lWidth = max(lWidth, sz.cx + 16 + 8);
	}
	SetWindowPos(g_hwndSFL, HWND_TOPMOST, rcWindow.left, rcWindow.top, lWidth, max(g_CluiData.bUseFloater & CLUI_FLOATER_EVENTS ? 36 : 20, sz.cy + 4), SWP_SHOWWINDOW);
	GetWindowRect(g_hwndSFL, &rcWindow);

	if(g_SFLCachedDC) {
		SelectObject(g_SFLCachedDC, g_SFLhbmOld);
		DeleteObject(g_SFLhbm);
		DeleteDC(g_SFLCachedDC);
		g_SFLCachedDC = 0;
	}

	g_SFLCachedDC = CreateCompatibleDC(hdc);
	g_SFLhbm = CreateCompatibleBitmap(hdc, lWidth, rcWindow.bottom - rcWindow.top);
	g_SFLhbmOld = SelectObject(g_SFLCachedDC, g_SFLhbm);

	ReleaseDC(g_hwndSFL, hdc);
	CluiProtocolStatusChanged(0, 0);
}

void SFL_Create()
{
	if(g_hwndSFL == 0 && g_CluiData.bUseFloater & CLUI_USE_FLOATER && MyUpdateLayeredWindow != NULL)
		g_hwndSFL = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, _T("StatusFloaterClass"), _T("sfl"), WS_VISIBLE, 0, 0, 0, 0, 0, 0, g_hInst, 0);
	else
		return;

	SetWindowLong(g_hwndSFL, GWL_STYLE, GetWindowLong(g_hwndSFL, GWL_STYLE) & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW));

	Utils_RestoreWindowPosition(g_hwndSFL, 0, "CLUI", "sfl");
	SFL_SetSize();
}

void FLT_SetSize(struct ExtraCache *centry, LONG width, LONG height)
{
	HDC hdc;
	RECT rcWindow;
	HFONT oldFont;

	GetWindowRect(centry->floater.hwnd, &rcWindow);

	hdc = GetDC(centry->floater.hwnd);
	oldFont = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

	SetWindowPos(centry->floater.hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_SHOWWINDOW | SWP_NOMOVE);
	GetWindowRect(centry->floater.hwnd, &rcWindow);

	if(centry->floater.hdc) {
		SelectObject(centry->floater.hdc, centry->floater.hbmOld);
		DeleteObject(centry->floater.hbm);
		DeleteDC(centry->floater.hdc);
		centry->floater.hdc = 0;
	}

	centry->floater.hdc = CreateCompatibleDC(hdc);
	centry->floater.hbm = CreateCompatibleBitmap(hdc, width, rcWindow.bottom - rcWindow.top);
	centry->floater.hbmOld= SelectObject(centry->floater.hdc, centry->floater.hbm);

	ReleaseDC(centry->floater.hwnd, hdc);
}

int FLT_CheckAvail()
{
	int i;

	for(i = 0; i < MAXFLOATERS; i++) {
		if(g_floaters[i] == -1)
			return i;
	}
	if(i >= MAXFLOATERS) {
		MessageBox(0, _T("You have reached the maximum number of available contact floaters"), _T("Floating contacts"), MB_OK | MB_ICONINFORMATION);
		return -1;
	}
	return -1;
}
void FLT_Create(int iEntry)
{
	struct ExtraCache *centry = &g_ExtraCache[iEntry];

	if(iEntry >= 0 && iEntry <= g_nextExtraCacheEntry) {
		struct ClcContact *contact = NULL;
		struct ClcGroup *group = NULL;

		if(centry->floater.hwnd == 0 && MyUpdateLayeredWindow != NULL) {
			int i = FLT_CheckAvail();

			if(i == -1)
				return;

			centry->floater.hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, _T("ContactFloaterClass"), _T("sfl"), WS_VISIBLE, 0, 0, 0, 0, 0, 0, g_hInst, (LPVOID)iEntry);
			g_floaters[i] = iEntry;
		}
		else
			return;

		SetWindowLong(centry->floater.hwnd, GWL_STYLE, GetWindowLong(centry->floater.hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW));

        if(Utils_RestoreWindowPosition(centry->floater.hwnd, centry->hContact, "CList", "flt"))
			if(Utils_RestoreWindowPositionNoMove(centry->floater.hwnd, centry->hContact, "CList", "flt"))
				SetWindowPos(centry->floater.hwnd, 0, 50, 50, 150, 30, SWP_NOZORDER | SWP_NOACTIVATE);

		FLT_SetSize(centry, 100, 20);
		ShowWindow(centry->floater.hwnd, SW_SHOW);
		if(FindItem(pcli->hwndContactTree, g_clcData, centry->hContact, &contact, &group, 0)) {
			if(contact)
				FLT_Update(g_clcData, contact);
		}
	}
}

void FLT_Update(struct ClcData *dat, struct ClcContact *contact)
{
	RECT rcClient, rcWindow;
	POINT ptDest, ptSrc = {0};
	SIZE szDest;
	BLENDFUNCTION bf = {0};
	HWND hwnd;
	HDC hdc;
	BOOL firstDrawn = TRUE;
	struct ClcGroup *group = NULL;
	struct ClcContact *newContact = NULL;
	HRGN rgn;

	if(contact == NULL || dat == NULL)
		return;

	FLT_SetSize(&g_ExtraCache[contact->extraCacheEntry], 150, RowHeights_GetFloatingRowHeight(dat, pcli->hwndContactTree, contact));

	hwnd = g_ExtraCache[contact->extraCacheEntry].floater.hwnd;
	hdc = g_ExtraCache[contact->extraCacheEntry].floater.hdc;

	if(hwnd == 0)
		return;

	GetClientRect(hwnd, &rcClient);
	GetWindowRect(hwnd, &rcWindow);

	ptDest.x = rcWindow.left;
	ptDest.y = rcWindow.top;
	szDest.cx = rcWindow.right - rcWindow.left;
	szDest.cy = rcWindow.bottom - rcWindow.top;

	FillRect(hdc, &rcClient, GetSysColorBrush(COLOR_3DFACE));
	SetBkMode(hdc, TRANSPARENT);

	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = 0;
	bf.SourceConstantAlpha = 200;

	rgn = CreateRoundRectRgn(0, 0, rcClient.right, rcClient.bottom, 10, 10);
	SelectClipRgn(hdc, rgn);

	if(FindItem(pcli->hwndContactTree, dat, contact->hContact, &newContact, &group, 0)) {
		g_HDC = hdc;
		PaintItem(hdc, group, contact, 0, 0, dat, -4, pcli->hwndContactTree, 0, &rcClient, &firstDrawn, 0, rcClient.bottom - rcClient.top);
	}
	DeleteObject(rgn);
	if(MyUpdateLayeredWindow)
		MyUpdateLayeredWindow(hwnd, 0, &ptDest, &szDest, hdc, &ptSrc, GetSysColor(COLOR_3DFACE), &bf, ULW_COLORKEY | ULW_ALPHA);
}
