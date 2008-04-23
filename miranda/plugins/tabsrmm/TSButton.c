/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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

$Id$

skinable button class for tabSRMM

*/

#include "commonheaders.h"

extern HINSTANCE g_hInst;
extern MYGLOBALS myGlobals;
extern BOOL g_skinnedContainers;
extern StatusItems_t StatusItems[];
extern PAB MyAlphaBlend;
extern ImageItem *g_glyphItem;

#define PBS_PUSHDOWNPRESSED 6

static LRESULT CALLBACK TSButtonWndProc(HWND hwnd, UINT  msg, WPARAM wParam, LPARAM lParam);

typedef struct {
	HWND    hwnd;
	int     stateId; // button state
	int     focus;   // has focus (1 or 0)
	HFONT   hFont;   // font
	HICON   arrow;   // uses down arrow
	int     defbutton; // default button
	HICON   hIcon, hIconPrivate;
	HBITMAP hBitmap;
	int     pushBtn;
	int     pbState;
	HANDLE  hThemeButton;
	HANDLE  hThemeToolbar;
	BOOL    bThemed;
	BOOL	bTitleButton;
	TCHAR	cHot;
	int     flatBtn;
	int     dimmed;
	struct ContainerWindowData *pContainer;
	ButtonItem *item;
} MButtonCtrl;

// External theme methods and properties

static CRITICAL_SECTION csTips;
static HWND hwndToolTips = NULL;
static BLENDFUNCTION bf_buttonglyph;
static HDC hdc_buttonglyph = 0;
static HBITMAP hbm_buttonglyph, hbm_buttonglyph_old;

extern PITA pfnIsThemeActive;
extern POTD pfnOpenThemeData;
extern PDTB pfnDrawThemeBackground;
extern PCTD pfnCloseThemeData;
extern PDTT pfnDrawThemeText;
extern PITBPT pfnIsThemeBackgroundPartiallyTransparent;
extern PDTPB  pfnDrawThemeParentBackground;
extern PGTBCR pfnGetThemeBackgroundContentRect;

int UnloadTSButtonModule(WPARAM wParam, LPARAM lParam)
{
	DeleteCriticalSection(&csTips);
	if (hdc_buttonglyph) {
		SelectObject(hdc_buttonglyph, hbm_buttonglyph_old);
		DeleteObject(hbm_buttonglyph);
		DeleteDC(hdc_buttonglyph);
	}
	return 0;
}

int LoadTSButtonModule(void)
{
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = _T("TSButtonClass");
	wc.lpfnWndProc    = TSButtonWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra     = sizeof(MButtonCtrl*);
	wc.hbrBackground  = 0;
	wc.style          = CS_GLOBALCLASS | CS_PARENTDC;
	RegisterClassEx(&wc);
	InitializeCriticalSection(&csTips);
	return 0;
}

// Used for our own cheap TrackMouseEvent
#define BUTTON_POLLID       100
#define BUTTON_POLLDELAY    50

#define MGPROC(x) GetProcAddress(themeAPIHandle,x)

static void DestroyTheme(MButtonCtrl *ctl)
{
	if (myGlobals.m_VSApiEnabled) {
		if (ctl->hThemeButton) {
			pfnCloseThemeData(ctl->hThemeButton);
			ctl->hThemeButton = NULL;
		}
		if (ctl->hThemeToolbar) {
			pfnCloseThemeData(ctl->hThemeToolbar);
			ctl->hThemeToolbar = NULL;
		}
	}
}

static void LoadTheme(MButtonCtrl *ctl)
{
	if (myGlobals.m_VSApiEnabled) {
		DestroyTheme(ctl);
		ctl->hThemeButton = pfnOpenThemeData(ctl->hwnd, L"BUTTON");
		ctl->hThemeToolbar = pfnOpenThemeData(ctl->hwnd, L"TOOLBAR");
		ctl->bThemed = TRUE;
	}
}

static TBStateConvert2Flat(int state)
{
	switch (state) {
		case PBS_NORMAL:
			return TS_NORMAL;
		case PBS_HOT:
			return TS_HOT;
		case PBS_PRESSED:
			return TS_PRESSED;
		case PBS_DISABLED:
			return TS_DISABLED;
		case PBS_DEFAULTED:
			return TS_NORMAL;
	}
	return TS_NORMAL;
}

static void PaintWorker(MButtonCtrl *ctl, HDC hdcPaint)
{
	if (hdc_buttonglyph == 0) {
		hdc_buttonglyph = CreateCompatibleDC(hdcPaint);
		hbm_buttonglyph = CreateCompatibleBitmap(hdcPaint, 16, 16);
		hbm_buttonglyph_old = SelectObject(hdc_buttonglyph, hbm_buttonglyph);
		bf_buttonglyph.BlendFlags = 0;
		bf_buttonglyph.SourceConstantAlpha = 120;
		bf_buttonglyph.BlendOp = AC_SRC_OVER;
		bf_buttonglyph.AlphaFormat = 0;
	}
	if (hdcPaint) {
		HDC hdcMem;
		HBITMAP hbmMem;
		HDC hOld;
		RECT rcClient, rcContent;
		HRGN clip = 0;

		GetClientRect(ctl->hwnd, &rcClient);
		CopyRect(&rcContent, &rcClient);

		hdcMem = CreateCompatibleDC(hdcPaint);
		hbmMem = CreateCompatibleBitmap(hdcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
		hOld = SelectObject(hdcMem, hbmMem);

		if (ctl->pushBtn && ctl->pbState)
			ctl->stateId = PBS_PRESSED;

		if (ctl->arrow && (ctl->stateId == PBS_HOT || ctl->stateId == PBS_PRESSED || ctl->stateId == PBS_PUSHDOWNPRESSED)) {
			POINT pt;

			GetCursorPos(&pt);
			ScreenToClient(ctl->hwnd, &pt);

			if (pt.x >= rcClient.right - 10)
				clip = CreateRectRgn(rcClient.right - 10, 0, rcClient.right, rcClient.bottom);
		}
		if (ctl->item) {
			RECT rcParent;
			POINT pt;
			HWND hwndParent = ctl->pContainer->hwnd;
			ImageItem *imgItem = ctl->stateId == PBS_HOT ? ctl->item->imgHover : (ctl->stateId == PBS_PRESSED ? ctl->item->imgPressed : ctl->item->imgNormal);

			if (imgItem == NULL)
				goto default_draw_bg;

			GetWindowRect(ctl->hwnd, &rcParent);
			pt.x = rcParent.left;
			pt.y = rcParent.top;

			if (ctl->pContainer->bSkinned) {
				ScreenToClient(hwndParent, &pt);

				BitBlt(hdcMem, 0, 0, rcClient.right, rcClient.bottom, ctl->pContainer->cachedDC, pt.x, pt.y, SRCCOPY);
				if (imgItem)
					DrawAlpha(hdcMem, &rcClient, 0, 0, 0, 0, 0, 0, 0, imgItem);
			}
			goto bg_done;
		}

default_draw_bg:

		if (ctl->flatBtn) {
			if (ctl->pContainer && ctl->pContainer->bSkinned) {
				StatusItems_t *item, *realItem = 0;
				if (ctl->bTitleButton)
					item = &StatusItems[ctl->stateId == PBS_NORMAL ? ID_EXTBKTITLEBUTTON : (ctl->stateId == PBS_HOT ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTONPRESSED)];
				else {
					item = &StatusItems[(ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) ? ID_EXTBKBUTTONSNPRESSED : (ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : ID_EXTBKBUTTONSPRESSED)];
					realItem = item;
					if (clip)
						item = &StatusItems[ID_EXTBKBUTTONSNPRESSED];
				}
				SkinDrawBG(ctl->hwnd, ctl->pContainer->hwnd,  ctl->pContainer, &rcClient, hdcMem);
				if (!item->IGNORED) {
					RECT rc1 = rcClient;
					rc1.left += item->MARGIN_LEFT;
					rc1.right -= item->MARGIN_RIGHT;
					rc1.top += item->MARGIN_TOP;
					rc1.bottom -= item->MARGIN_BOTTOM;
					DrawAlpha(hdcMem, &rc1, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
					if (clip && realItem) {
						SelectClipRgn(hdcMem, clip);
						DrawAlpha(hdcMem, &rc1, realItem->COLOR, realItem->ALPHA, realItem->COLOR2, realItem->COLOR2_TRANSPARENT,
								  realItem->GRADIENT, realItem->CORNER, realItem->BORDERSTYLE, realItem->imageItem);
					}
				} else
					goto flat_themed;
			} else {
flat_themed:
				if (ctl->hThemeToolbar && ctl->bThemed) {
					RECT rc = rcClient;
					int state = IsWindowEnabled(ctl->hwnd) ? (ctl->stateId == PBS_NORMAL && ctl->defbutton ? PBS_DEFAULTED : ctl->stateId) : PBS_DISABLED;
					int realState = state;

					if (clip)
						state = PBS_NORMAL;

					if (rc.right < 20 || rc.bottom < 20)
						InflateRect(&rc, 2, 2);
					if (pfnIsThemeBackgroundPartiallyTransparent(ctl->hThemeToolbar, TP_BUTTON, TBStateConvert2Flat(state))) {
						pfnDrawThemeParentBackground(ctl->hwnd, hdcMem, &rc);
					}
					pfnDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(state), &rc, &rc);
					if (clip) {
						SelectClipRgn(hdcMem, clip);
						pfnDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(realState), &rc, &rc);
					}
				} else {
					HBRUSH hbr;
					RECT rc = rcClient;

					if (ctl->stateId == PBS_PRESSED || ctl->stateId == PBS_HOT) {
						hbr = GetSysColorBrush(COLOR_3DLIGHT);
						FillRect(hdcMem, &rc, hbr);
					} else {
						HDC dc;
						HWND hwndParent;

						hwndParent = GetParent(ctl->hwnd);
						dc = GetDC(hwndParent);
						hbr = (HBRUSH)SendMessage(hwndParent, WM_CTLCOLORDLG, (WPARAM)dc, (LPARAM)hwndParent);
						ReleaseDC(hwndParent, dc);
						if (hbr) {
							FillRect(hdcMem, &rc, hbr);
							DeleteObject(hbr);
						}
					}
					if (ctl->stateId == PBS_HOT || ctl->focus) {
						if (ctl->pbState)
							DrawEdge(hdcMem, &rc, EDGE_ETCHED, BF_RECT | BF_SOFT);
						else
							DrawEdge(hdcMem, &rc, BDR_RAISEDOUTER, BF_RECT | BF_SOFT);
					} else if (ctl->stateId == PBS_PRESSED)
						DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT | BF_SOFT);
				}
			}
		} else {
			if (ctl->pContainer && ctl->pContainer->bSkinned) {
				StatusItems_t *item, *realItem = 0;
				if (ctl->bTitleButton)
					item = &StatusItems[ctl->stateId == PBS_NORMAL ? ID_EXTBKTITLEBUTTON : (ctl->stateId == PBS_HOT ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTONPRESSED)];
				else {
					item = &StatusItems[(ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) ? ID_EXTBKBUTTONSNPRESSED : (ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : ID_EXTBKBUTTONSPRESSED)];
					realItem = item;
					if (clip)
						item = &StatusItems[ID_EXTBKBUTTONSNPRESSED];
				}
				SkinDrawBG(ctl->hwnd, ctl->pContainer->hwnd,  ctl->pContainer, &rcClient, hdcMem);
				if (!item->IGNORED) {
					RECT rc1 = rcClient;
					rc1.left += item->MARGIN_LEFT;
					rc1.right -= item->MARGIN_RIGHT;
					rc1.top += item->MARGIN_TOP;
					rc1.bottom -= item->MARGIN_BOTTOM;
					DrawAlpha(hdcMem, &rc1, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
					if (clip && realItem) {
						SelectClipRgn(hdcMem, clip);
						DrawAlpha(hdcMem, &rc1, realItem->COLOR, realItem->ALPHA, realItem->COLOR2, realItem->COLOR2_TRANSPARENT,
								  realItem->GRADIENT, realItem->CORNER, realItem->BORDERSTYLE, realItem->imageItem);
					}
				} else
					goto nonflat_themed;
			} else {
nonflat_themed:
				if (ctl->hThemeButton && ctl->bThemed) {
					int state = IsWindowEnabled(ctl->hwnd) ? (ctl->stateId == PBS_NORMAL && ctl->defbutton ? PBS_DEFAULTED : ctl->stateId) : PBS_DISABLED;
					int realState = state;

					if (clip)
						state = PBS_NORMAL;

					if (pfnIsThemeBackgroundPartiallyTransparent(ctl->hThemeButton, BP_PUSHBUTTON, state)) {
						pfnDrawThemeParentBackground(ctl->hwnd, hdcMem, &rcClient);
					}
					pfnDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, state, &rcClient, &rcClient);

					if (clip) {
						SelectClipRgn(hdcMem, clip);
						pfnDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, realState, &rcClient, &rcClient);
					}
					pfnGetThemeBackgroundContentRect(ctl->hThemeToolbar, hdcMem, BP_PUSHBUTTON, PBS_NORMAL, &rcClient, &rcContent);
				} else {
					RECT rcFill = rcClient;
					UINT uType = ctl->stateId == PBS_PRESSED ? EDGE_ETCHED : EDGE_BUMP;
					DrawEdge(hdcMem, &rcClient, uType, BF_RECT | BF_SOFT | (ctl->stateId == PBS_HOT ? BF_MONO : 0));
					InflateRect(&rcFill, -1, -1);
					FillRect(hdcMem, &rcFill, GetSysColorBrush(COLOR_3DFACE));
				}

				// Draw focus rectangle if button has focus
				if (ctl->focus) {
					RECT focusRect = rcClient;
					InflateRect(&focusRect, -3, -3);
					DrawFocusRect(hdcMem, &focusRect);
				}
			}
		}
bg_done:
		if (clip) {
			SelectClipRgn(hdcMem, 0);
			DeleteObject(clip);
		}
		if (ctl->arrow) {
			rcContent.top += 2;
			rcContent.bottom -= 2;
			rcContent.left = rcClient.right - 12;
			rcContent.right = rcContent.left;

			DrawIconEx(hdcMem, rcClient.right - 13, (rcClient.bottom - rcClient.top) / 2 - (myGlobals.m_smcyicon / 2),
					   myGlobals.g_buttonBarIcons[16], 16, 16, 0, 0, DI_NORMAL);
			if (!ctl->flatBtn)
				DrawEdge(hdcMem, &rcContent, EDGE_BUMP, BF_LEFT);
			else if (ctl->pContainer && ctl->pContainer->bSkinned) {
				HPEN hPenOld = SelectObject(hdcMem, myGlobals.g_SkinLightShadowPen);
				POINT pt;

				MoveToEx(hdcMem, rcContent.left, rcContent.top, &pt);
				LineTo(hdcMem, rcContent.left, rcContent.bottom);
				SelectObject(hdcMem, myGlobals.g_SkinDarkShadowPen);
				MoveToEx(hdcMem, rcContent.left + 1, rcContent.bottom - 1, &pt);
				LineTo(hdcMem, rcContent.left + 1, rcContent.top - 1);
				SelectObject(hdcMem, hPenOld);
			}
		}

		if (ctl->item) {
			HICON hIcon = 0;
			LONG *glyphMetrics = ctl->stateId == PBS_HOT ? ctl->item->hoverGlyphMetrics : (ctl->stateId == PBS_PRESSED ? ctl->item->pressedGlyphMetrics : ctl->item->normalGlyphMetrics);
			LONG xOff;
			HFONT hOldFont;
			SIZE  szText = {0};

			if (ctl->item->dwFlags & BUTTON_HASLABEL) {
				hOldFont = SelectObject(hdcMem, ctl->hFont);
				GetTextExtentPoint32(hdcMem, ctl->item->tszLabel, lstrlen(ctl->item->tszLabel), &szText);
				szText.cx += 2;
			}
			if ((ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) && ctl->item->dwFlags & BUTTON_NORMALGLYPHISICON)
				hIcon = *((HICON *)ctl->item->normalGlyphMetrics[0]);
			else if (ctl->item->dwFlags & BUTTON_PRESSEDGLYPHISICON && ctl->stateId == PBS_PRESSED)
				hIcon = *((HICON *)ctl->item->pressedGlyphMetrics[0]);
			else if (ctl->item->dwFlags & BUTTON_HOVERGLYPHISICON && ctl->stateId == PBS_HOT)
				hIcon = *((HICON *)ctl->item->hoverGlyphMetrics[0]);

			if (hIcon) {
				szText.cx += 16;
				if (ctl->stateId != PBS_DISABLED || MyAlphaBlend == 0)
					DrawIconEx(hdcMem, rcClient.right / 2 - szText.cx / 2, rcClient.bottom / 2 - 8, hIcon, 16, 16, 0, 0, DI_NORMAL);
				else {
					BitBlt(hdc_buttonglyph, 0, 0, 16, 16, hdcMem, rcClient.right / 2 - szText.cx / 2, rcClient.bottom / 2 - 8, SRCCOPY);
					DrawIconEx(hdc_buttonglyph, 0, 0, hIcon, 16, 16, 0, 0, DI_NORMAL);
					MyAlphaBlend(hdcMem, rcClient.right / 2 - szText.cx / 2, rcClient.bottom / 2 - 8, 16, 16, hdc_buttonglyph, 0, 0, 16, 16, bf_buttonglyph);
				}
				xOff = 18;
			} else {
				szText.cx += glyphMetrics[2];
				if (g_glyphItem) {
					MyAlphaBlend(hdcMem, (rcClient.right - szText.cx / 2), (rcClient.bottom - glyphMetrics[3]) / 2,
								 glyphMetrics[2], glyphMetrics[3], g_glyphItem->hdc,
								 glyphMetrics[0], glyphMetrics[1], glyphMetrics[2],
								 glyphMetrics[3], g_glyphItem->bf);
				}
				xOff = glyphMetrics[2] + 2;
			}
			if (ctl->item->dwFlags & BUTTON_HASLABEL) {
				RECT rc = rcClient;

				if (ctl->pContainer && ctl->pContainer->bSkinned)
					SetTextColor(hdcMem, ctl->stateId != PBS_DISABLED ? myGlobals.skinDefaultFontColor : GetSysColor(COLOR_GRAYTEXT));
				else
					SetTextColor(hdcMem, ctl->stateId != PBS_DISABLED ? GetSysColor(COLOR_BTNTEXT) : GetSysColor(COLOR_GRAYTEXT));
				SetBkMode(hdcMem, TRANSPARENT);
				rc.left = (rcClient.right - szText.cx) / 2 + xOff;
				DrawText(hdcMem, ctl->item->tszLabel, -1, &rc, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				SelectObject(hdcMem, hOldFont);
			}
		} else if (ctl->hIcon || ctl->hIconPrivate) {
			int ix = (rcClient.right - rcClient.left) / 2 - (myGlobals.m_smcxicon / 2);
			int iy = (rcClient.bottom - rcClient.top) / 2 - (myGlobals.m_smcyicon / 2);
			HICON hIconNew = ctl->hIconPrivate != 0 ? ctl->hIconPrivate : ctl->hIcon;

			if (ctl->stateId == PBS_PRESSED) {
				ix++;
				iy++;
			}

			if (ctl->arrow)
				ix -= 4;

			if (ctl->dimmed && myGlobals.m_IdleDetect)
				DrawDimmedIcon(hdcMem, ix, iy, myGlobals.m_smcxicon, myGlobals.m_smcyicon, hIconNew, 180);
			else {
				if (ctl->stateId != PBS_DISABLED || MyAlphaBlend == 0)
					DrawIconEx(hdcMem, ix, iy, hIconNew, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL);
				else {
					BitBlt(hdc_buttonglyph, 0, 0, 16, 16, hdcMem, ix, iy, SRCCOPY);
					DrawIconEx(hdc_buttonglyph, 0, 0, hIconNew, 16, 16, 0, 0, DI_NORMAL);
 					MyAlphaBlend(hdcMem, ix, iy, myGlobals.m_smcxicon, myGlobals.m_smcyicon, hdc_buttonglyph, 0, 0, 16, 16, bf_buttonglyph);
				}
			}
		} else if (GetWindowTextLength(ctl->hwnd)) {
			// Draw the text and optinally the arrow
			TCHAR szText[MAX_PATH];
			SIZE sz;
			RECT rcText;
			HFONT hOldFont;

			CopyRect(&rcText, &rcClient);
			GetWindowText(ctl->hwnd, szText, MAX_PATH - 1);
			SetBkMode(hdcMem, TRANSPARENT);
			hOldFont = SelectObject(hdcMem, ctl->hFont);
			// XP w/themes doesn't used the glossy disabled text.  Is it always using COLOR_GRAYTEXT?  Seems so.
			if (ctl->pContainer && ctl->pContainer->bSkinned)
				SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd) ? myGlobals.skinDefaultFontColor : GetSysColor(COLOR_GRAYTEXT));
			else
				SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd) || !ctl->hThemeButton ? GetSysColor(COLOR_BTNTEXT) : GetSysColor(COLOR_GRAYTEXT));
			GetTextExtentPoint32(hdcMem, szText, lstrlen(szText), &sz);
			if (ctl->cHot) {
				SIZE szHot;

				GetTextExtentPoint32A(hdcMem, "&", 1, &szHot);
				sz.cx -= szHot.cx;
			}
			if (ctl->arrow)
				DrawState(hdcMem, NULL, NULL, (LPARAM)ctl->arrow, 0, rcClient.right - rcClient.left - 5 - myGlobals.m_smcxicon + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), (rcClient.bottom - rcClient.top) / 2 - myGlobals.m_smcyicon / 2 + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), myGlobals.m_smcxicon, myGlobals.m_smcyicon, IsWindowEnabled(ctl->hwnd) ? DST_ICON : DST_ICON | DSS_DISABLED);
			SelectObject(hdcMem, ctl->hFont);
			DrawState(hdcMem, NULL, NULL, (LPARAM)szText, lstrlen(szText), (rcText.right - rcText.left - sz.cx) / 2 + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), ctl->hThemeButton ? (rcText.bottom - rcText.top - sz.cy) / 2 : (rcText.bottom - rcText.top - sz.cy) / 2 - (ctl->stateId == PBS_PRESSED ? 0 : 1), sz.cx, sz.cy, IsWindowEnabled(ctl->hwnd) || ctl->hThemeButton ? DST_PREFIXTEXT | DSS_NORMAL : DST_PREFIXTEXT | DSS_DISABLED);
			SelectObject(hdcMem, hOldFont);
		}
		BitBlt(hdcPaint, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcMem, 0, 0, SRCCOPY);
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

	}
}

static LRESULT CALLBACK TSButtonWndProc(HWND hwndDlg, UINT msg,  WPARAM wParam, LPARAM lParam)
{
	MButtonCtrl* bct = (MButtonCtrl *)GetWindowLong(hwndDlg, 0);
	switch (msg) {
		case WM_NCCREATE: {
			SetWindowLong(hwndDlg, GWL_STYLE, GetWindowLong(hwndDlg, GWL_STYLE) | BS_OWNERDRAW);
			bct = malloc(sizeof(MButtonCtrl));
			if (bct == NULL) return FALSE;
			bct->hwnd = hwndDlg;
			bct->stateId = PBS_NORMAL;
			bct->focus = 0;
			bct->hFont = GetStockObject(DEFAULT_GUI_FONT);
			bct->arrow = NULL;
			bct->defbutton = 0;
			bct->hIcon = bct->hIconPrivate = NULL;
			bct->hBitmap = NULL;
			bct->pushBtn = 0;
			bct->pbState = 0;
			bct->hThemeButton = NULL;
			bct->hThemeToolbar = NULL;
			bct->cHot = 0;
			bct->flatBtn = 0;
			bct->bThemed = bct->bTitleButton = FALSE;
			bct->dimmed = 0;
			bct->pContainer = NULL;
			bct->item = NULL;
			LoadTheme(bct);
			SetWindowLong(hwndDlg, 0, (LONG)bct);
			if (((CREATESTRUCTA *)lParam)->lpszName) SetWindowTextA(hwndDlg, ((CREATESTRUCTA *)lParam)->lpszName);
			return TRUE;
		}
		case WM_DESTROY: {
			if (bct) {
				EnterCriticalSection(&csTips);
				if (hwndToolTips) {
					TOOLINFO ti;

					ZeroMemory(&ti, sizeof(ti));
					ti.cbSize = sizeof(ti);
					ti.uFlags = TTF_IDISHWND;
					ti.hwnd = bct->hwnd;
					ti.uId = (UINT)bct->hwnd;
					if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM)&ti)) {
						SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
					}
					if (SendMessage(hwndToolTips, TTM_GETTOOLCOUNT, 0, (LPARAM)&ti) == 0) {
						DestroyWindow(hwndToolTips);
						hwndToolTips = NULL;
					}
				}
				if (bct->hIconPrivate)
					DestroyIcon(bct->hIconPrivate);
				LeaveCriticalSection(&csTips);
				DestroyTheme(bct);
				free(bct);
			}
			SetWindowLong(hwndDlg, 0, (LONG)NULL);
			break;	// DONT! fall thru
		}
		case WM_SETTEXT: {
			bct->cHot = 0;
			if ((TCHAR *)lParam) {
				TCHAR *tmp = (TCHAR *)lParam;
				while (*tmp) {
					if (*tmp == (TCHAR)'&' && *(tmp + 1)) {
						bct->cHot = _totlower(*(tmp + 1));
						break;
					}
					tmp++;
				}
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		}
		case WM_KEYUP:
			if (bct->stateId != PBS_DISABLED && wParam == VK_SPACE && !(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
				if (bct->pushBtn) {
					if (bct->pbState) bct->pbState = 0;
					else bct->pbState = 1;
					InvalidateRect(bct->hwnd, NULL, TRUE);
				}
				SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM)hwndDlg);
				return 0;
			}
			break;
		case WM_SYSKEYUP:
			if (bct->stateId != PBS_DISABLED && bct->cHot && bct->cHot == tolower((int)wParam)) {
				if (bct->pushBtn) {
					if (bct->pbState) bct->pbState = 0;
					else bct->pbState = 1;
					InvalidateRect(bct->hwnd, NULL, TRUE);
				}
				SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM)hwndDlg);
				return 0;
			}
			break;
		case WM_THEMECHANGED: {
			// themed changed, reload theme object
			if (bct->bThemed)
				LoadTheme(bct);
			InvalidateRect(bct->hwnd, NULL, TRUE); // repaint it
			break;
		}
		case WM_SETFONT: { // remember the font so we can use it later
			bct->hFont = (HFONT)wParam; // maybe we should redraw?
			break;
		}
		case WM_NCPAINT:
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdcPaint;

			hdcPaint = BeginPaint(hwndDlg, &ps);
			if (hdcPaint) {
				PaintWorker(bct, hdcPaint);
				EndPaint(hwndDlg, &ps);
			}
			break;
		}
		case BM_SETIMAGE:
			if (wParam == IMAGE_ICON) {
				ICONINFO ii;
				BITMAP bm;

				if (bct->hIconPrivate)
					DestroyIcon(bct->hIconPrivate);

				GetIconInfo((HICON)lParam, &ii);
				GetObject(ii.hbmColor, sizeof(bm), &bm);
				if (bm.bmWidth != myGlobals.m_smcxicon || bm.bmHeight != myGlobals.m_smcyicon) {
					HIMAGELIST hImageList;
					hImageList = ImageList_Create(myGlobals.m_smcxicon, myGlobals.m_smcyicon, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
					ImageList_AddIcon(hImageList, (HICON)lParam);
					bct->hIconPrivate = ImageList_GetIcon(hImageList, 0, ILD_NORMAL);
					ImageList_RemoveAll(hImageList);
					ImageList_Destroy(hImageList);
					bct->hIcon = 0;
				} else {
					bct->hIcon = (HICON)lParam;
					bct->hIconPrivate = 0;
				}

				DeleteObject(ii.hbmMask);
				DeleteObject(ii.hbmColor);
				bct->hBitmap = NULL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			} else if (wParam == IMAGE_BITMAP) {
				bct->hBitmap = (HBITMAP)lParam;
				if (bct->hIconPrivate)
					DestroyIcon(bct->hIconPrivate);
				bct->hIcon = bct->hIconPrivate = NULL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		case BM_SETCHECK:
			if (!bct->pushBtn) break;
			if (wParam == BST_CHECKED) {
				bct->pbState = 1;
				bct->stateId = PBS_PRESSED;
			} else if (wParam == BST_UNCHECKED) {
				bct->pbState = 0;
				bct->stateId = PBS_NORMAL;
			}
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BM_GETCHECK:
			if (bct->pushBtn) {
				return bct->pbState ? BST_CHECKED : BST_UNCHECKED;
			}
			return 0;
		case BUTTONSETARROW: // turn arrow on/off
			bct->arrow = (HICON)wParam;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BUTTONSETDEFAULT:
			bct->defbutton = wParam ? 1 : 0;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BUTTONSETASPUSHBTN:
			bct->pushBtn = 1;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BUTTONSETASFLATBTN:
			bct->flatBtn = lParam == 0 ? 1 : 0;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BUTTONSETASFLATBTN + 10:
			bct->bThemed = lParam ? TRUE : FALSE;
			break;
		case BUTTONSETASFLATBTN + 11:
			bct->dimmed = lParam ? TRUE : FALSE;
			break;
		case BUTTONSETASFLATBTN + 12:
			bct->pContainer = (struct ContainerWindowData *)lParam;
			break;
		case BUTTONSETASFLATBTN + 13:
			bct->bTitleButton = TRUE;
			break;
		case BUTTONSETASFLATBTN + 14:
			bct->stateId = (wParam) ? PBS_NORMAL : PBS_DISABLED;
			InvalidateRect(bct->hwnd, NULL, FALSE);
			break;
		case BUTTONSETASFLATBTN + 15:
			return bct->stateId;

		case BUTTONSETASFLATBTN + 20:
			bct->item = (ButtonItem *)lParam;
			break;

		case BUTTONADDTOOLTIP: {
			TOOLINFO ti;

			if (!(char*)wParam)
				break;
			EnterCriticalSection(&csTips);
			if (!hwndToolTips) {
				hwndToolTips = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, _T(""), WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
			}
			ZeroMemory(&ti, sizeof(ti));
			ti.cbSize = sizeof(ti);
			ti.uFlags = TTF_IDISHWND;
			ti.hwnd = bct->hwnd;
			ti.uId = (UINT)bct->hwnd;
			if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM)&ti)) {
				SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
			}
			ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			ti.uId = (UINT)bct->hwnd;
			ti.lpszText = (TCHAR *)wParam;
			SendMessage(hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)&ti);
			SendMessage(hwndToolTips, TTM_SETMAXTIPWIDTH, 0, 300);
			LeaveCriticalSection(&csTips);
			break;
		}
		case WM_SETFOCUS: // set keybord focus and redraw
			bct->focus = 1;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case WM_KILLFOCUS: // kill focus and redraw
			bct->focus = 0;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case WM_WINDOWPOSCHANGED:
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case WM_ENABLE: { // windows tells us to enable/disable
			bct->stateId = wParam ? PBS_NORMAL : PBS_DISABLED;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		}
		case WM_MOUSELEAVE: { // faked by the WM_TIMER
			if (bct->stateId != PBS_DISABLED) { // don't change states if disabled
				bct->stateId = PBS_NORMAL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		}
		case WM_LBUTTONDOWN: {
			RECT rc;

			if (bct->arrow && bct->stateId != PBS_DISABLED) {
				GetClientRect(bct->hwnd, &rc);
				if (LOWORD(lParam) < rc.right - 12)
					bct->stateId = PBS_PRESSED;
				else
					bct->stateId = PBS_PUSHDOWNPRESSED;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			} else if (bct->stateId != PBS_DISABLED) {
				bct->stateId = PBS_PRESSED;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		}
		case WM_LBUTTONUP: {
			RECT rc;

			if (bct->pushBtn) {
				if (bct->pbState) bct->pbState = 0;
				else bct->pbState = 1;
			}
			if (bct->stateId != PBS_DISABLED) { // don't change states if disabled
				if (msg == WM_LBUTTONUP) bct->stateId = PBS_HOT;
				else bct->stateId = PBS_NORMAL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			if (bct->arrow) {
				GetClientRect(bct->hwnd, &rc);
				if (LOWORD(lParam) > rc.right - 12) {
					SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(bct->arrow, BN_CLICKED), (LPARAM)hwndDlg);
					break;
				}
			}
			SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM)hwndDlg);
			break;
		}
		case WM_MOUSEMOVE:
			if (bct->stateId == PBS_NORMAL) {
				bct->stateId = PBS_HOT;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			} else if (bct->arrow && bct->stateId == PBS_HOT) {
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			// Call timer, used to start cheesy TrackMouseEvent faker
			SetTimer(hwndDlg, BUTTON_POLLID, BUTTON_POLLDELAY, NULL);
			break;
		case WM_TIMER: { // use a timer to check if they have did a mouseout
			if (wParam == BUTTON_POLLID) {
				RECT rc;
				POINT pt;
				GetWindowRect(hwndDlg, &rc);
				GetCursorPos(&pt);
				if (!PtInRect(&rc, pt)) { // mouse must be gone, trigger mouse leave
					PostMessage(hwndDlg, WM_MOUSELEAVE, 0, 0L);
					KillTimer(hwndDlg, BUTTON_POLLID);
				}
			}
			break;
		}
		case WM_ERASEBKGND:
			return 1;
	}
	return DefWindowProc(hwndDlg, msg, wParam, lParam);
}
