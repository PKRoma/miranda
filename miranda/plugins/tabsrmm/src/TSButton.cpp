/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * A skinnable button class for tabSRMM.
 *
 */

#include "commonheaders.h"
#include <ctype.h>

#define PBS_PUSHDOWNPRESSED 6

static LRESULT CALLBACK TSButtonWndProc(HWND hwnd, UINT  msg, WPARAM wParam, LPARAM lParam);

// External theme methods and properties

static CRITICAL_SECTION csTips;
static HWND hwndToolTips = NULL;
static BLENDFUNCTION bf_buttonglyph;
static HDC hdc_buttonglyph = 0;
static HBITMAP hbm_buttonglyph, hbm_buttonglyph_old;

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

static void TSAPI DestroyTheme(MButtonCtrl *ctl)
{
	if (M->isVSAPIState()) {
		if (ctl->hThemeButton) {
			CMimAPI::m_pfnCloseThemeData(ctl->hThemeButton);
			ctl->hThemeButton = NULL;
		}
		if (ctl->hThemeToolbar) {
			CMimAPI::m_pfnCloseThemeData(ctl->hThemeToolbar);
			ctl->hThemeToolbar = NULL;
		}
	}
}

static void TSAPI LoadTheme(MButtonCtrl *ctl)
{
	if (M->isVSAPIState()) {
		DestroyTheme(ctl);
		ctl->hThemeButton = CMimAPI::m_pfnOpenThemeData(ctl->hwnd, L"BUTTON");
		ctl->hThemeToolbar = (M->isAero() || IsWinVerVistaPlus()) ? CMimAPI::m_pfnOpenThemeData(ctl->hwnd, L"MENU") : CMimAPI::m_pfnOpenThemeData(ctl->hwnd, L"TOOLBAR");
		ctl->bThemed = TRUE;
	}
}

int TSAPI TBStateConvert2Flat(int state)
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


/**
 * convert button state (hot, pressed, normal) to REBAR part state ids
 *
 * @param state  int: button state
 *
 * @return int: state item id
 */
int TSAPI RBStateConvert2Flat(int state)
{
	switch (state) {
		case PBS_NORMAL:
			return 1;
		case PBS_HOT:
			return 2;
		case PBS_PRESSED:
			return 3;
		case PBS_DISABLED:
			return 1;
		case PBS_DEFAULTED:
			return 1;
	}
	return 1;
}

static void PaintWorker(MButtonCtrl *ctl, HDC hdcPaint)
{
	if (hdc_buttonglyph == 0) {
		hdc_buttonglyph = CreateCompatibleDC(hdcPaint);
		hbm_buttonglyph = CreateCompatibleBitmap(hdcPaint, 16, 16);
		hbm_buttonglyph_old = (HBITMAP)SelectObject(hdc_buttonglyph, hbm_buttonglyph);
		bf_buttonglyph.BlendFlags = 0;
		bf_buttonglyph.SourceConstantAlpha = 120;
		bf_buttonglyph.BlendOp = AC_SRC_OVER;
		bf_buttonglyph.AlphaFormat = 0;
	}
	if (hdcPaint) {
		HDC 	hdcMem;
		HBITMAP hbmMem, hOld;
		RECT 	rcClient, rcContent;
		HRGN 	clip = 0;
		bool 	fAero = M->isAero();
		bool 	fVSThemed = (!CSkin::m_skinEnabled && M->isVSThemed());
		HANDLE 	hbp = 0;

		_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(GetParent(ctl->hwnd), GWLP_USERDATA);
		GetClientRect(ctl->hwnd, &rcClient);
		CopyRect(&rcContent, &rcClient);

		if(CMimAPI::m_haveBufferedPaint)
			hbp = CMimAPI::m_pfnBeginBufferedPaint(hdcPaint, &rcClient, BPBF_TOPDOWNDIB, NULL, &hdcMem);
		else {
			hdcMem = CreateCompatibleDC(hdcPaint);
			hbmMem = CreateCompatibleBitmap(hdcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
			hOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
		}

		if (ctl->pushBtn && ctl->pbState)
			ctl->stateId = PBS_PRESSED;

		if (ctl->item) {
			RECT rcParent;
			POINT pt;
			HWND hwndParent = ctl->pContainer->hwnd;
			CImageItem *imgItem = ctl->stateId == PBS_HOT ? ctl->item->imgHover : (ctl->stateId == PBS_PRESSED ? ctl->item->imgPressed : ctl->item->imgNormal);

			if (imgItem == NULL)
				goto default_draw_bg;

			GetWindowRect(ctl->hwnd, &rcParent);
			pt.x = rcParent.left;
			pt.y = rcParent.top;

			if (CSkin::m_skinEnabled) {
				ScreenToClient(hwndParent, &pt);

				BitBlt(hdcMem, 0, 0, rcClient.right, rcClient.bottom, ctl->pContainer->cachedDC, pt.x, pt.y, SRCCOPY);
				DrawAlpha(hdcMem, &rcClient, 0, 0, 0, 0, 0, 0, 0, imgItem);
			}
			goto bg_done;
		}

default_draw_bg:

		if (ctl->flatBtn) {
			if (ctl->pContainer && CSkin::m_skinEnabled) {
				CSkinItem *item, *realItem = 0;
				if (ctl->bTitleButton)
					item = &SkinItems[ctl->stateId == PBS_NORMAL ? ID_EXTBKTITLEBUTTON : (ctl->stateId == PBS_HOT ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTONPRESSED)];
				else {
					item = &SkinItems[(ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) ? ID_EXTBKBUTTONSNPRESSED : (ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : ID_EXTBKBUTTONSPRESSED)];
					realItem = item;
					if (clip)
						item = &SkinItems[ID_EXTBKBUTTONSNPRESSED];
				}
				CSkin::SkinDrawBG(ctl->hwnd, ctl->pContainer->hwnd,  ctl->pContainer, &rcClient, hdcMem);
				if (!item->IGNORED) {
					RECT rc1 = rcClient;
					rc1.left += item->MARGIN_LEFT;
					rc1.right -= item->MARGIN_RIGHT;
					rc1.top += item->MARGIN_TOP;
					rc1.bottom -= item->MARGIN_BOTTOM;
					CSkin::DrawItem(hdcMem, &rc1, item);
					if (clip && realItem) {
						SelectClipRgn(hdcMem, clip);
						CSkin::DrawItem(hdcMem, &rc1, realItem);
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

					FillRect(hdcMem, &rc, (HBRUSH)GetSysColor(COLOR_3DFACE));
					if (rc.right < 20 || rc.bottom < 20)
						InflateRect(&rc, 2, 2);
					if((fAero || fVSThemed) && ctl->bToolbarButton) {
						if(dat) {
							RECT	rcWin;
							GetWindowRect(ctl->hwnd, &rcWin);
							POINT 	pt;
							pt.x = rcWin.left;
							ScreenToClient(dat->hwnd, &pt);
							BitBlt(hdcMem, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
								   dat->pContainer->cachedToolbarDC, pt.x, 1, SRCCOPY);
						}
					}
					else {
						if (CMimAPI::m_pfnIsThemeBackgroundPartiallyTransparent(ctl->hThemeToolbar, TP_BUTTON, TBStateConvert2Flat(state)))
							CMimAPI::m_pfnDrawThemeParentBackground(ctl->hwnd, hdcMem, &rc);
					}
					if(fAero || PluginConfig.m_WinVerMajor >= 6)
						CMimAPI::m_pfnDrawThemeBackground(ctl->hThemeToolbar, hdcMem, 8, RBStateConvert2Flat(state), &rc, &rc);
					else
						CMimAPI::m_pfnDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(state), &rc, &rc);
					if (clip && !(fAero || fVSThemed)) {
						SelectClipRgn(hdcMem, clip);
						CMimAPI::m_pfnDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(realState), &rc, &rc);
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
			if (ctl->pContainer && CSkin::m_skinEnabled) {
				CSkinItem *item, *realItem = 0;
				if (ctl->bTitleButton)
					item = &SkinItems[ctl->stateId == PBS_NORMAL ? ID_EXTBKTITLEBUTTON : (ctl->stateId == PBS_HOT ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTONPRESSED)];
				else {
					item = &SkinItems[(ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) ? ID_EXTBKBUTTONSNPRESSED : (ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : ID_EXTBKBUTTONSPRESSED)];
					realItem = item;
					if (clip)
						item = &SkinItems[ID_EXTBKBUTTONSNPRESSED];
				}
				CSkin::SkinDrawBG(ctl->hwnd, ctl->pContainer->hwnd,  ctl->pContainer, &rcClient, hdcMem);
				if (!item->IGNORED) {
					RECT rc1 = rcClient;
					rc1.left += item->MARGIN_LEFT;
					rc1.right -= item->MARGIN_RIGHT;
					rc1.top += item->MARGIN_TOP;
					rc1.bottom -= item->MARGIN_BOTTOM;
					CSkin::DrawItem(hdcMem, &rc1, item);
					if (clip && realItem) {
						SelectClipRgn(hdcMem, clip);
						CSkin::DrawItem(hdcMem, &rc1, realItem);
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

					if((fAero || fVSThemed) && ctl->bToolbarButton) {
						if(dat) {
							RECT	rcWin;
							GetWindowRect(ctl->hwnd, &rcWin);
							POINT 	pt;
							pt.x = rcWin.left;
							ScreenToClient(dat->hwnd, &pt);
							BitBlt(hdcMem, rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
								   dat->pContainer->cachedToolbarDC, pt.x, 1, SRCCOPY);
						}
					}
					else {
						if (CMimAPI::m_pfnIsThemeBackgroundPartiallyTransparent(ctl->hThemeButton, BP_PUSHBUTTON, state)) {
							CMimAPI::m_pfnDrawThemeParentBackground(ctl->hwnd, hdcMem, &rcClient);
						}
					}
					CMimAPI::m_pfnDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, state, &rcClient, &rcClient);

					if (clip) {
						SelectClipRgn(hdcMem, clip);
						CMimAPI::m_pfnDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, realState, &rcClient, &rcClient);
					}
					CMimAPI::m_pfnGetThemeBackgroundContentRect(ctl->hThemeToolbar, hdcMem, BP_PUSHBUTTON, PBS_NORMAL, &rcClient, &rcContent);
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

			DrawIconEx(hdcMem, rcClient.right - 15, (rcClient.bottom - rcClient.top) / 2 - (PluginConfig.m_smcyicon / 2),
					   PluginConfig.g_buttonBarIcons[16], 16, 16, 0, 0, DI_NORMAL);
			if (!ctl->flatBtn)
				DrawEdge(hdcMem, &rcContent, EDGE_BUMP, BF_LEFT);
			else if (ctl->pContainer && CSkin::m_skinEnabled) {
				HPEN hPenOld = (HPEN)SelectObject(hdcMem, CSkin::m_SkinLightShadowPen);
				POINT pt;

				MoveToEx(hdcMem, rcContent.left, rcContent.top, &pt);
				LineTo(hdcMem, rcContent.left, rcContent.bottom);
				SelectObject(hdcMem, CSkin::m_SkinDarkShadowPen);
				MoveToEx(hdcMem, rcContent.left + 1, rcContent.bottom - 1, &pt);
				LineTo(hdcMem, rcContent.left + 1, rcContent.top - 1);
				SelectObject(hdcMem, hPenOld);
			}
		}

		if (ctl->item) {
			HICON hIcon = 0;
			LONG_PTR *glyphMetrics = ctl->stateId == PBS_HOT ? ctl->item->hoverGlyphMetrics : (ctl->stateId == PBS_PRESSED ? ctl->item->pressedGlyphMetrics : ctl->item->normalGlyphMetrics);
			LONG xOff;
			HFONT hOldFont;
			SIZE  szText = {0};

			if (ctl->item->dwFlags & BUTTON_HASLABEL) {
				hOldFont = (HFONT)SelectObject(hdcMem, ctl->hFont);
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
				if (ctl->stateId != PBS_DISABLED || CMimAPI::m_MyAlphaBlend == 0)
					DrawIconEx(hdcMem, rcClient.right / 2 - szText.cx / 2, rcClient.bottom / 2 - 8, hIcon, 16, 16, 0, 0, DI_NORMAL);
				else {
					BitBlt(hdc_buttonglyph, 0, 0, 16, 16, hdcMem, rcClient.right / 2 - szText.cx / 2, rcClient.bottom / 2 - 8, SRCCOPY);
					DrawIconEx(hdc_buttonglyph, 0, 0, hIcon, 16, 16, 0, 0, DI_NORMAL);
					CMimAPI::m_MyAlphaBlend(hdcMem, rcClient.right / 2 - szText.cx / 2, rcClient.bottom / 2 - 8, 16, 16, hdc_buttonglyph, 0, 0, 16, 16, bf_buttonglyph);
				}
				xOff = 18;
			} else {
				const CImageItem	*glyphItem = Skin->getGlyphItem();
				szText.cx += glyphMetrics[2];
				if (glyphItem) {
					CMimAPI::m_MyAlphaBlend(hdcMem, (rcClient.right - szText.cx / 2), (rcClient.bottom - glyphMetrics[3]) / 2,
								 glyphMetrics[2], glyphMetrics[3], glyphItem->getDC(),
								 glyphMetrics[0], glyphMetrics[1], glyphMetrics[2],
								 glyphMetrics[3], glyphItem->getBF());
				}
				xOff = glyphMetrics[2] + 2;
			}
			if (ctl->item->dwFlags & BUTTON_HASLABEL) {
				RECT rc = rcClient;

				if (ctl->pContainer && CSkin::m_skinEnabled)
					SetTextColor(hdcMem, ctl->stateId != PBS_DISABLED ? CSkin::m_DefaultFontColor : GetSysColor(COLOR_GRAYTEXT));
				else
					SetTextColor(hdcMem, ctl->stateId != PBS_DISABLED ? GetSysColor(COLOR_BTNTEXT) : GetSysColor(COLOR_GRAYTEXT));
				SetBkMode(hdcMem, TRANSPARENT);
				rc.left = (rcClient.right - szText.cx) / 2 + xOff;
				DrawText(hdcMem, ctl->item->tszLabel, -1, &rc, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				SelectObject(hdcMem, hOldFont);
			}
		} else if (ctl->hIcon || ctl->hIconPrivate) {
			int ix = (rcClient.right - rcClient.left) / 2 - 8;
			int iy = (rcClient.bottom - rcClient.top) / 2 - 8;
			HICON hIconNew = ctl->hIconPrivate != 0 ? ctl->hIconPrivate : ctl->hIcon;

			if (ctl->stateId == PBS_PRESSED) {
				ix++;
				iy++;
			}

			if (ctl->arrow)
				ix -= 4;

			if (ctl->dimmed && PluginConfig.m_IdleDetect)
				CSkin::DrawDimmedIcon(hdcMem, ix, iy, PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, hIconNew, 180);
			else {
				if (ctl->stateId != PBS_DISABLED || CMimAPI::m_MyAlphaBlend == 0)
					DrawIconEx(hdcMem, ix, iy, hIconNew, 16, 16, 0, 0, DI_NORMAL);
				else {
					BitBlt(hdc_buttonglyph, 0, 0, 16, 16, hdcMem, ix, iy, SRCCOPY);
					DrawIconEx(hdc_buttonglyph, 0, 0, hIconNew, 16, 16, 0, 0, DI_NORMAL);
 					CMimAPI::m_MyAlphaBlend(hdcMem, ix, iy, PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, hdc_buttonglyph, 0, 0, 16, 16, bf_buttonglyph);
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
			hOldFont = (HFONT)SelectObject(hdcMem, ctl->hFont);
			// XP w/themes doesn't used the glossy disabled text.  Is it always using COLOR_GRAYTEXT?  Seems so.
			if (ctl->pContainer && CSkin::m_skinEnabled)
				SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd) ? CSkin::m_DefaultFontColor : GetSysColor(COLOR_GRAYTEXT));
			else
				SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd) || !ctl->hThemeButton ? GetSysColor(COLOR_BTNTEXT) : GetSysColor(COLOR_GRAYTEXT));
			GetTextExtentPoint32(hdcMem, szText, lstrlen(szText), &sz);
			if (ctl->cHot) {
				SIZE szHot;

				GetTextExtentPoint32A(hdcMem, "&", 1, &szHot);
				sz.cx -= szHot.cx;
			}
			if (ctl->arrow)
				DrawState(hdcMem, NULL, NULL, (LPARAM)ctl->arrow, 0, rcClient.right - rcClient.left - 5 - PluginConfig.m_smcxicon + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), (rcClient.bottom - rcClient.top) / 2 - PluginConfig.m_smcyicon / 2 + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, IsWindowEnabled(ctl->hwnd) ? DST_ICON : DST_ICON | DSS_DISABLED);
			SelectObject(hdcMem, ctl->hFont);
			DrawState(hdcMem, NULL, NULL, (LPARAM)szText, lstrlen(szText), (rcText.right - rcText.left - sz.cx) / 2 + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), ctl->hThemeButton ? (rcText.bottom - rcText.top - sz.cy) / 2 : (rcText.bottom - rcText.top - sz.cy) / 2 - (ctl->stateId == PBS_PRESSED ? 0 : 1), sz.cx, sz.cy, IsWindowEnabled(ctl->hwnd) || ctl->hThemeButton ? DST_PREFIXTEXT | DSS_NORMAL : DST_PREFIXTEXT | DSS_DISABLED);
			SelectObject(hdcMem, hOldFont);
		}
		if(hbp)
			CMimAPI::m_pfnEndBufferedPaint(hbp, TRUE);
		else {
			BitBlt(hdcPaint, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hOld);
			DeleteObject(hbmMem);
			DeleteDC(hdcMem);
		}

	}
}

static LRESULT CALLBACK TSButtonWndProc(HWND hwndDlg, UINT msg,  WPARAM wParam, LPARAM lParam)
{
	MButtonCtrl* bct = (MButtonCtrl *)GetWindowLongPtr(hwndDlg, 0);
	switch (msg) {
		case WM_NCCREATE: {
			SetWindowLongPtr(hwndDlg, GWL_STYLE, GetWindowLongPtr(hwndDlg, GWL_STYLE) | BS_OWNERDRAW);
			bct = (MButtonCtrl *)malloc(sizeof(MButtonCtrl));
			if (bct == NULL)
				return FALSE;
			ZeroMemory(bct, sizeof(MButtonCtrl));
			bct->hwnd = hwndDlg;
			bct->stateId = PBS_NORMAL;
			bct->focus = 0;
			bct->hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
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
			bct->item = 0;
			bct->sitem = 0;
			LoadTheme(bct);
			SetWindowLongPtr(hwndDlg, 0, (LONG_PTR)bct);
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
					ti.uId = (UINT_PTR)bct->hwnd;
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
			}
			break;	// DONT! fall thru
		}

		case WM_NCDESTROY:
			free(bct);
			SetWindowLongPtr(hwndDlg, 0, (LONG_PTR)NULL);
			break;

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
				if(bct->sitem)
					bct->sitem->RenderThis(hdcPaint);
				else
					PaintWorker(bct, hdcPaint);
				EndPaint(hwndDlg, &ps);
			}
			return(0);
		}
		case BM_SETIMAGE:
			if (wParam == IMAGE_ICON) {
				ICONINFO ii;
				BITMAP bm;

				if (bct->hIconPrivate)
					DestroyIcon(bct->hIconPrivate);

				GetIconInfo((HICON)lParam, &ii);
				GetObject(ii.hbmColor, sizeof(bm), &bm);
				if (bm.bmWidth != PluginConfig.m_smcxicon || bm.bmHeight != PluginConfig.m_smcyicon) {
					HIMAGELIST hImageList;
					hImageList = ImageList_Create(PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, PluginConfig.m_bIsXP ? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
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
		case BUTTONSETASTOOLBARBUTTON:
			bct->bToolbarButton = lParam;
			break;
		case BUTTONSETASSIDEBARBUTTON:
			bct->sitem = reinterpret_cast<CSideBarButton *>(lParam);
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
			ti.uId = (UINT_PTR)bct->hwnd;
			if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM)&ti)) {
				SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
			}
			ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			ti.uId = (UINT_PTR)bct->hwnd;
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

			if(bct->sitem) {
				if(bct->sitem->testCloseButton() != -1)
					return(TRUE);
				bct->stateId = PBS_PRESSED;
				InvalidateRect(bct->hwnd, NULL, TRUE);
				bct->sitem->activateSession();
			}

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
			int  showClick = 0;
			if (bct->sitem) {
				if(bct->sitem->testCloseButton() != -1) {
					SendMessage(bct->sitem->getDat()->hwnd, WM_CLOSE, 1, 0);
					return(TRUE);
				}
			}
			if (bct->pushBtn) {
				if (bct->pbState) bct->pbState = 0;
				else bct->pbState = 1;
			}
			if (bct->stateId != PBS_DISABLED) { // don't change states if disabled
				if(bct->stateId == PBS_PRESSED || bct->stateId == PBS_PUSHDOWNPRESSED)
					showClick = 1;
				if (msg == WM_LBUTTONUP)
					bct->stateId = PBS_HOT;
				else
					bct->stateId = PBS_NORMAL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			if (bct->arrow) {
				GetClientRect(bct->hwnd, &rc);
				if (LOWORD(lParam) > rc.right - 12) {
					if(showClick) {
						WORD w = (WORD)((int)bct->arrow & 0x0000ffff);
						SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(w, BN_CLICKED), (LPARAM)hwndDlg);
					}
					break;
				}
			}
			if(showClick)
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
			if(bct->sitem) {
				if(bct->sitem->testCloseButton() != -1) {
					if(bct->sitem->m_sideBar->getHoveredClose() != bct->sitem) {
						bct->sitem->m_sideBar->setHoveredClose(bct->sitem);
						InvalidateRect(hwndDlg, 0, FALSE);
					}
				}
				else {
					bct->sitem->m_sideBar->setHoveredClose(0);
					InvalidateRect(hwndDlg, 0, FALSE);
				}
			}
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
					if(bct->sitem) {
						bct->sitem->m_sideBar->setHoveredClose(0);
						InvalidateRect(hwndDlg, 0, FALSE);
					}
				}
			}
			break;
		}
		case WM_ERASEBKGND:
			return 0;
	}
	return DefWindowProc(hwndDlg, msg, wParam, lParam);
}
