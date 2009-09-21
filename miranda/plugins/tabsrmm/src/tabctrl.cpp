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
 * a custom tab control, skinable, aero support, single/multi row, button
 * tabs support, proper rendering for bottom row tabs and more.
 *
 */

#include "commonheaders.h"
#pragma hdrstop

static WNDPROC		OldTabControlClassProc;
extern ButtonSet	g_ButtonSet;

static LRESULT CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void GetIconSize(HICON hIcon, int* sizeX, int* sizeY);

#define FIXED_TAB_SIZE 100

/*
 * register the new tab control as a window class (TSTabCtrlClass)
 */

int RegisterTabCtrlClass(void)
{
	WNDCLASSEXA wc;
	WNDCLASSEX wce;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = "TSTabCtrlClass";
	wc.lpfnWndProc    = TabControlSubclassProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra     = sizeof(struct TabControlData *);
	wc.hbrBackground  = 0;
	wc.style          = CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC;
	RegisterClassExA(&wc);

	ZeroMemory(&wce, sizeof(wce));
	wce.cbSize         = sizeof(wce);
	wce.lpszClassName  = _T("TSStatusBarClass");
	wce.lpfnWndProc    = StatusBarSubclassProc;
	wce.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wce.cbWndExtra     = sizeof(void*);
	wce.hbrBackground  = 0;
	wce.style          = CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC;
	RegisterClassEx(&wce);

	ZeroMemory(&wce, sizeof(wce));
	wce.cbSize         = sizeof(wce);
	wce.lpszClassName  = _T("SideBarClass");
	wce.lpfnWndProc    = CSideBar::wndProcStub;;
	wce.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wce.cbWndExtra     = sizeof(void*);
	wce.hbrBackground  = 0;
	wce.style          = CS_GLOBALCLASS;// | CS_DBLCLKS; // | CS_PARENTDC;
	RegisterClassEx(&wce);
	return 0;
}

static int TabCtrl_TestForCloseButton(const TabControlData *tabdat, HWND hwnd, POINT& pt)
{
	TCHITTESTINFO tci = {0};
	tci.pt.x = pt.x;
	tci.pt.y = pt.y;
	int iTab;

	ScreenToClient(hwnd, &tci.pt);
	iTab = TabCtrl_HitTest(hwnd, &tci);
	if(iTab != -1) {
		RECT  rcTab;

		if(tci.flags & TCHT_NOWHERE)
			return(-1);

		TabCtrl_GetItemRect(hwnd, iTab, &rcTab);
		rcTab.left = rcTab.right - 18;
		rcTab.right -= 5;
		rcTab.bottom -= 4;
		rcTab.top += 4;
		if(PtInRect(&rcTab, tci.pt))
			return(iTab);
	}
	return(-1);
}

/*
 * tabctrl helper function
 * Finds leftmost down item.
 */

static UINT FindLeftDownItem(HWND hwnd)
{
	RECT rctLeft = {100000, 0, 0, 0}, rctCur;
	int nCount = TabCtrl_GetItemCount(hwnd) - 1;
	UINT nItem = 0;
	int i;

	for (i = 0;i < nCount;i++) {
		TabCtrl_GetItemRect(hwnd, i, &rctCur);
		if (rctCur.left > 0 && rctCur.left <= rctLeft.left) {
			if (rctCur.bottom > rctLeft.bottom) {
				rctLeft = rctCur;
				nItem = i;
			}
		}
	}
	return nItem;
}

/*
 * tab control color definitions, including the database setting key names
 */

static struct colOptions {
	UINT id;
	UINT defclr;
	char *szKey;
	char *szSkinnedKey;
} tabcolors[] = {
	IDC_TXTCLRNORMAL, COLOR_BTNTEXT, "tab_txt_normal", "S_tab_txt_normal",
	IDC_TXTCLRACTIVE, COLOR_BTNTEXT, "tab_txt_active", "S_tab_txt_active",
	IDC_TXTCLRUNREAD, COLOR_HOTLIGHT, "tab_txt_unread", "S_tab_txt_unread",
	IDC_TXTCLRHOTTRACK, COLOR_HOTLIGHT, "tab_txt_hottrack", "S_tab_txt_hottrack",
	IDC_BKGCLRNORMAL, COLOR_3DFACE, "tab_bg_normal", "tab_bg_normal",
	IDC_BKGCLRACTIVE, COLOR_3DFACE, "tab_bg_active", "tab_bg_active",
	IDC_BKGCLRUNREAD, COLOR_3DFACE, "tab_bg_unread", "tab_bg_unread",
	IDC_BKGCOLORHOTTRACK, COLOR_3DFACE, "tab_bg_hottrack", "tab_bg_hottrack",
	IDC_LIGHTSHADOW, COLOR_3DHIGHLIGHT, "tab_lightshadow", "tab_lightshadow",
	IDC_DARKSHADOW, COLOR_3DDKSHADOW, "tab_darkshadow", "tab_darkshadow",
	0, 0, NULL, NULL
};

/*
 * hints for drawing functions
 */

#define HINT_ACTIVATE_RIGHT_SIDE 1
#define HINT_ACTIVE_ITEM 2
#define FLOAT_ITEM_HEIGHT_SHIFT 2
#define ACTIVE_ITEM_HEIGHT_SHIFT 2
#define SHIFT_FROM_CUT_TO_SPIN 4
#define HINT_TRANSPARENT 16
#define HINT_HOTTRACK 32

/*
 * draw an antialiased (smoothed) line from X0/Y0 to X1/Y1 using clrLine as basecolor
 * used by the "styled" tabs which look similar to visual studio UI tabs.
 */

static void __stdcall DrawWuLine(HDC pDC, int X0, int Y0, int X1, int Y1, COLORREF clrLine)
{
	int XDir, DeltaX, DeltaY;
	unsigned short ErrorAdj;
	unsigned short ErrorAccTemp, Weighting;
	unsigned short ErrorAcc = 0;
	double grayl;
	BYTE rl, gl, bl;
	BYTE rb, gb, bb, rr, gr, br;
	double grayb;
	COLORREF clrBackGround;

	if (Y0 > Y1) {
		int Temp = Y0;
		Y0 = Y1;
		Y1 = Temp;
		Temp = X0;
		X0 = X1;
		X1 = Temp;
	}

	SetPixel(pDC, X0, Y0, clrLine);

	DeltaX = X1 - X0;
	if (DeltaX >= 0)
		XDir = 1;
	else {
		XDir   = -1;
		DeltaX = 0 - DeltaX;
	}

	DeltaY = Y1 - Y0;

	if (DeltaY == 0) {
		while (DeltaX-- != 0) {
			X0 += XDir;
			SetPixel(pDC, X0, Y0, clrLine);
		}
		return;
	}
	if (DeltaX == 0) {
		do {
			Y0++;
			SetPixel(pDC, X0, Y0, clrLine);
		} while (--DeltaY != 0);
		return;
	}

	if (DeltaX == DeltaY) {
		do {
			X0 += XDir;
			Y0++;
			SetPixel(pDC, X0, Y0, clrLine);
		} while (--DeltaY != 0);
		return;
	}

	rl = GetRValue(clrLine);
	gl = GetGValue(clrLine);
	bl = GetBValue(clrLine);
	grayl = rl * 0.299 + gl * 0.587 + bl * 0.114;

	if (DeltaY > DeltaX) {
		ErrorAdj = (unsigned short)(((unsigned long) DeltaX << 16) / (unsigned long) DeltaY);

		while (--DeltaY) {
			ErrorAccTemp = ErrorAcc;
			ErrorAcc += ErrorAdj;
			if (ErrorAcc <= ErrorAccTemp)
				X0 += XDir;
			Y0++;
			Weighting = ErrorAcc >> 8;

			clrBackGround = GetPixel(pDC, X0, Y0);
			rb = GetRValue(clrBackGround);
			gb = GetGValue(clrBackGround);
			bb = GetBValue(clrBackGround);
			grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

			rr = (rb > rl ? ((BYTE)(((double)(grayl < grayb ? Weighting :
											  (Weighting ^ 255))) / 255.0 * (rb - rl) + rl)) :
						  ((BYTE)(((double)(grayl < grayb ? Weighting : (Weighting ^ 255)))
								  / 255.0 * (rl - rb) + rb)));
			gr = (gb > gl ? ((BYTE)(((double)(grayl < grayb ? Weighting :
											  (Weighting ^ 255))) / 255.0 * (gb - gl) + gl)) :
						  ((BYTE)(((double)(grayl < grayb ? Weighting : (Weighting ^ 255)))
								  / 255.0 * (gl - gb) + gb)));
			br = (bb > bl ? ((BYTE)(((double)(grayl < grayb ? Weighting :
											  (Weighting ^ 255))) / 255.0 * (bb - bl) + bl)) :
						  ((BYTE)(((double)(grayl < grayb ? Weighting : (Weighting ^ 255)))
								  / 255.0 * (bl - bb) + bb)));
			SetPixel(pDC, X0, Y0, RGB(rr, gr, br));

			clrBackGround = GetPixel(pDC, X0 + XDir, Y0);
			rb = GetRValue(clrBackGround);
			gb = GetGValue(clrBackGround);
			bb = GetBValue(clrBackGround);
			grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

			rr = (rb > rl ? ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) :
											  Weighting)) / 255.0 * (rb - rl) + rl)) :
						  ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) : Weighting))
								  / 255.0 * (rl - rb) + rb)));
			gr = (gb > gl ? ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) :
											  Weighting)) / 255.0 * (gb - gl) + gl)) :
						  ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) : Weighting))
								  / 255.0 * (gl - gb) + gb)));
			br = (bb > bl ? ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) :
											  Weighting)) / 255.0 * (bb - bl) + bl)) :
						  ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) : Weighting))
								  / 255.0 * (bl - bb) + bb)));
			SetPixel(pDC, X0 + XDir, Y0, RGB(rr, gr, br));
		}
		SetPixel(pDC, X1, Y1, clrLine);
		return;
	}
	ErrorAdj = (unsigned short)(((unsigned long) DeltaY << 16) / (unsigned long) DeltaX);

	while (--DeltaX) {
		ErrorAccTemp = ErrorAcc;
		ErrorAcc += ErrorAdj;
		if (ErrorAcc <= ErrorAccTemp)
			Y0++;
		X0 += XDir;
		Weighting = ErrorAcc >> 8;

		clrBackGround = GetPixel(pDC, X0, Y0);
		rb = GetRValue(clrBackGround);
		gb = GetGValue(clrBackGround);
		bb = GetBValue(clrBackGround);
		grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

		rr = (rb > rl ? ((BYTE)(((double)(grayl < grayb ? Weighting :
										  (Weighting ^ 255))) / 255.0 * (rb - rl) + rl)) :
					  ((BYTE)(((double)(grayl < grayb ? Weighting : (Weighting ^ 255)))
							  / 255.0 * (rl - rb) + rb)));
		gr = (gb > gl ? ((BYTE)(((double)(grayl < grayb ? Weighting :
										  (Weighting ^ 255))) / 255.0 * (gb - gl) + gl)) :
					  ((BYTE)(((double)(grayl < grayb ? Weighting : (Weighting ^ 255)))
							  / 255.0 * (gl - gb) + gb)));
		br = (bb > bl ? ((BYTE)(((double)(grayl < grayb ? Weighting :
										  (Weighting ^ 255))) / 255.0 * (bb - bl) + bl)) :
					  ((BYTE)(((double)(grayl < grayb ? Weighting : (Weighting ^ 255)))
							  / 255.0 * (bl - bb) + bb)));

		SetPixel(pDC, X0, Y0, RGB(rr, gr, br));

		clrBackGround = GetPixel(pDC, X0, Y0 + 1);
		rb = GetRValue(clrBackGround);
		gb = GetGValue(clrBackGround);
		bb = GetBValue(clrBackGround);
		grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

		rr = (rb > rl ? ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) :
										  Weighting)) / 255.0 * (rb - rl) + rl)) :
					  ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) : Weighting))
							  / 255.0 * (rl - rb) + rb)));
		gr = (gb > gl ? ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) :
										  Weighting)) / 255.0 * (gb - gl) + gl)) :
					  ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) : Weighting))
							  / 255.0 * (gl - gb) + gb)));
		br = (bb > bl ? ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) :
										  Weighting)) / 255.0 * (bb - bl) + bl)) :
					  ((BYTE)(((double)(grayl < grayb ? (Weighting ^ 255) : Weighting))
							  / 255.0 * (bl - bb) + bb)));

		SetPixel(pDC, X0, Y0 + 1, RGB(rr, gr, br));
	}
	SetPixel(pDC, X1, Y1, clrLine);
}
/*
 * draws the item contents (icon and label)
 * it obtains the label and icon handle directly from the message window data
 * no image list is used and necessary, the message window dialog procedure has to provide a valid
 * icon handle in dat->hTabIcon
 */

static void DrawItem(struct TabControlData *tabdat, HDC dc, RECT *rcItem, int nHint, int nItem)
{
	TCITEM item = {0};
	struct _MessageWindowData *dat = 0;
	int iSize = 16;
	DWORD dwTextFlags = DT_SINGLELINE | DT_VCENTER/* | DT_NOPREFIX*/;
	BOOL leftMost = FALSE;

	item.mask = TCIF_PARAM;
	TabCtrl_GetItem(tabdat->hwnd, nItem, &item);

	/*
	 * get the message window data for the session to which this tab item belongs
	 */

	if (IsWindow((HWND)item.lParam) && item.lParam != 0)
		dat = (struct _MessageWindowData *)GetWindowLongPtr((HWND)item.lParam, GWLP_USERDATA);

	if (dat) {
		HICON hIcon;
		HBRUSH bg;
		HFONT oldFont;
		DWORD dwStyle = tabdat->dwStyle;
		BOOL bFill = ((dwStyle & TCS_BUTTONS && !CSkin::m_skinEnabled) && (tabdat->m_VisualStyles == FALSE));
		int oldMode = 0;
		InflateRect(rcItem, -1, -1);

		if (nHint & HINT_ACTIVE_ITEM)
			SetTextColor(dc, PluginConfig.tabConfig.colors[1]);
		else if (dat->mayFlashTab == TRUE)
			SetTextColor(dc, PluginConfig.tabConfig.colors[2]);
		else if (nHint & HINT_HOTTRACK)
			SetTextColor(dc, PluginConfig.tabConfig.colors[3]);
		else
			SetTextColor(dc, PluginConfig.tabConfig.colors[0]);

		oldMode = SetBkMode(dc, TRANSPARENT);

		if (bFill) {
			if (dat->mayFlashTab == TRUE)
				bg = PluginConfig.tabConfig.m_hBrushUnread;
			else if (nHint & HINT_ACTIVE_ITEM)
				bg = PluginConfig.tabConfig.m_hBrushActive;
			else if (nHint & HINT_HOTTRACK)
				bg = PluginConfig.tabConfig.m_hBrushHottrack;
			else
				bg = PluginConfig.tabConfig.m_hBrushDefault;
			FillRect(dc, rcItem, bg);
		}
		rcItem->left++;
		rcItem->right--;
		rcItem->top++;
		rcItem->bottom--;

		if (tabdat->m_moderntabs && rcItem->left < 10) {
			leftMost = TRUE;
			rcItem->left += 10;
		}

		if (!(dwStyle & TCS_BOTTOM))
			OffsetRect(rcItem, 0, 1);

		if (dat->dwFlags & MWF_ERRORSTATE)
			hIcon = PluginConfig.g_iconErr;
		else if (dat->mayFlashTab)
			hIcon = dat->iFlashIcon;
		else {
			if (dat->si && dat->iFlashIcon) {
				int sizeX, sizeY;

				hIcon = dat->iFlashIcon;
				GetIconSize(hIcon, &sizeX, &sizeY);
				iSize = sizeX;
			} else if (dat->hTabIcon == dat->hTabStatusIcon && dat->hXStatusIcon)
				hIcon = dat->hXStatusIcon;
			else
				hIcon = dat->hTabIcon;
		}


		if (dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0) || !(dat->pContainer->dwFlagsEx & TCF_FLASHICON)) {
			DWORD ix = rcItem->left + tabdat->m_xpad - 1;
			DWORD iy = (rcItem->bottom + rcItem->top - iSize) / 2;
			if (dat->dwFlagsEx & MWF_SHOW_ISIDLE && PluginConfig.m_IdleDetect)
				CSkin::DrawDimmedIcon(dc, ix, iy, iSize, iSize, hIcon, 180);
			else
				DrawIconEx(dc, ix, iy, hIcon, iSize, iSize, 0, NULL, DI_NORMAL | DI_COMPAT);
		}

		rcItem->left += (iSize + 2 + tabdat->m_xpad);

		if(tabdat->fCloseButton) {
			if(tabdat->iHoveredCloseIcon != nItem)
				CSkin::m_default_bf.SourceConstantAlpha = 150;

			CMimAPI::m_MyAlphaBlend(dc, rcItem->right - 18, (rcItem->bottom + rcItem->top - 16) / 2, 16, 16, CSkin::m_tabCloseHDC,
									0, 0, 16, 16, CSkin::m_default_bf);

			rcItem->right -= 18;
			CSkin::m_default_bf.SourceConstantAlpha = 255;
		}

		if (dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0) || !(dat->pContainer->dwFlagsEx & TCF_FLASHLABEL)) {
			oldFont = (HFONT)SelectObject(dc, (HFONT)SendMessage(tabdat->hwnd, WM_GETFONT, 0, 0));
			if (tabdat->dwStyle & TCS_BUTTONS || !(tabdat->dwStyle & TCS_MULTILINE)) { // || (tabdat->m_moderntabs && leftMost)) {
				rcItem->right -= tabdat->m_xpad;
				dwTextFlags |= DT_WORD_ELLIPSIS;
			}
#if defined(_UNICODE)
			if(M->isAero())
				CSkin::RenderText(dc, dwStyle & TCS_BUTTONS ? tabdat->hThemeButton : tabdat->hTheme, dat->newtitle, rcItem, dwTextFlags, CSkin::m_glowSize);
			else if (tabdat->m_VisualStyles == FALSE || tabdat->m_moderntabs || CSkin::m_skinEnabled)
				DrawText(dc, dat->newtitle, (int)(lstrlen(dat->newtitle)), rcItem, dwTextFlags);
			else
				M->m_pfnDrawThemeText(dwStyle & TCS_BUTTONS ? tabdat->hThemeButton : tabdat->hTheme, dc, 1, nHint & HINT_ACTIVE_ITEM ? 3 : (nHint & HINT_HOTTRACK ? 2 : 1), dat->newtitle, (int)(lstrlen(dat->newtitle)), dwTextFlags, 0, rcItem);
#else
			DrawText(dc, dat->newtitle, lstrlen(dat->newtitle), rcItem, dwTextFlags);
#endif
			SelectObject(dc, oldFont);
		}
		if (oldMode)
			SetBkMode(dc, oldMode);
	}
}

/*
 * draws the item rect (the "tab") in *classic* style (no visual themes
 */

static RECT rcTabPage = {0};

static void DrawItemRect(struct TabControlData *tabdat, HDC dc, RECT *rcItem, int nHint, int iItem)
{
	POINT pt;
	DWORD dwStyle = tabdat->dwStyle;

	rcItem->bottom -= 1;
	if (rcItem->left >= 0) {

		/*
		 * draw "button style" tabs... raised edge for hottracked, sunken edge for active (pushed)
		 * otherwise, they get a normal border
		 */

		if (dwStyle & TCS_BUTTONS) {
			BOOL bClassicDraw = (tabdat->m_VisualStyles == FALSE);

			// draw frame controls for button or bottom tabs
			if (dwStyle & TCS_BOTTOM)
				rcItem->top++;
			else
				rcItem->bottom--;

			rcItem->right += 6;
			if (bClassicDraw) {
				if (CSkin::m_skinEnabled) {
					CSkinItem *item = nHint & HINT_ACTIVE_ITEM ? &SkinItems[ID_EXTBKBUTTONSPRESSED] : (nHint & HINT_HOTTRACK ? &SkinItems[ID_EXTBKBUTTONSMOUSEOVER] : &SkinItems[ID_EXTBKBUTTONSNPRESSED]);

					if (!item->IGNORED) {
						CSkin::SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
						CSkin::DrawItem(dc, rcItem, item);
					} else
						goto b_nonskinned;
				} else {
b_nonskinned:
					if (nHint & HINT_ACTIVE_ITEM)
						DrawEdge(dc, rcItem, EDGE_ETCHED, BF_RECT | BF_SOFT);
					else if (nHint & HINT_HOTTRACK)
						DrawEdge(dc, rcItem, EDGE_BUMP, BF_RECT | BF_MONO | BF_SOFT);
					else
						DrawEdge(dc, rcItem, EDGE_RAISED, BF_RECT | BF_SOFT);
				}
			} else {
				FillRect(dc, rcItem, (M->isAero() && !(dwStyle & TCS_BOTTOM)) ? CSkin::m_BrushBack : GetSysColorBrush(COLOR_3DFACE));
				CMimAPI::m_pfnDrawThemeBackground(tabdat->hThemeButton, dc, 1, nHint & HINT_ACTIVE_ITEM ? 3 : (nHint & HINT_HOTTRACK ? 2 : 1), rcItem, rcItem);
			}
			return;
		}
		SelectObject(dc, PluginConfig.tabConfig.m_hPenLight);

		if (nHint & HINT_ACTIVE_ITEM) {
			if (dwStyle & TCS_BOTTOM) {
				/*
				if(tabdat->pContainer->bSkinned)
					SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				else
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
				*/
				if (!CSkin::m_skinEnabled && !tabdat->m_moderntabs)
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
				//else if(tabdat->pContainer->bSkinned)
				//    SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				rcItem->bottom += 2;
			} else {
				rcItem->bottom += 2;
				/*
				if(tabdat->pContainer->bSkinned)
					SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				else
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
				*/
				if (!CSkin::m_skinEnabled && !tabdat->m_moderntabs)
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
				//else if(tabdat->pContainer->bSkinned)
				//    SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				rcItem->bottom--;
				rcItem->top -= 2;
			}
			if (CSkin::m_skinEnabled) {
				CSkinItem *item = &SkinItems[dwStyle & TCS_BOTTOM ? ID_EXTBKTABITEMACTIVEBOTTOM : ID_EXTBKTABITEMACTIVE];
				if (!item->IGNORED) {
					rcItem->left += item->MARGIN_LEFT;
					rcItem->right -= item->MARGIN_RIGHT;
					rcItem->top += item->MARGIN_TOP;
					rcItem->bottom -= item->MARGIN_BOTTOM;
					CSkin::DrawItem(dc, rcItem, item);
					return;
				}
			}
		}
		if (CSkin::m_skinEnabled) {
			CSkinItem *item = &SkinItems[dwStyle & TCS_BOTTOM ? (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACKBOTTOM : ID_EXTBKTABITEMBOTTOM) :
													   (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACK : ID_EXTBKTABITEM)];
			if (!item->IGNORED) {
				if (dwStyle & TCS_BOTTOM)
					rcItem->top = (rcItem->top > rcTabPage.bottom + 5) ? --rcItem->top : rcItem->top;
				else
					rcItem->bottom++;
				//rcItem->bottom = (rcItem->bottom < rcTabPage.top - 5) ? ++rcItem->bottom : rcItem->bottom;

				rcItem->left += item->MARGIN_LEFT;
				rcItem->right -= item->MARGIN_RIGHT;
				CSkin::DrawItem(dc, rcItem, item);
				return;
			}
		}
		if (dwStyle & TCS_BOTTOM) {
			if (tabdat->m_moderntabs) {
				RECT rc = *rcItem;
				POINT pt[5], pts;
				BOOL active = nHint & HINT_ACTIVE_ITEM;
				HRGN rgn;
				TCITEM item = {0};
				struct _MessageWindowData *dat = 0;
				HBRUSH bg;
				CSkinItem *s_item;
				int item_id;

				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(tabdat->hwnd, iItem, &item);

				/*
				 * get the message window data for the session to which this tab item belongs
				 */

				if (IsWindow((HWND)item.lParam) && item.lParam != 0)
					dat = (struct _MessageWindowData *)GetWindowLongPtr((HWND)item.lParam, GWLP_USERDATA);

				if (active && rc.left > 10)
					rc.left -= 10;

				if (active) {
					rc.bottom--;
					//rc.right -1;
				}
				rc.right--;

				item_id = active ? ID_EXTBKTABITEMACTIVEBOTTOM : (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACKBOTTOM : ID_EXTBKTABITEMBOTTOM);
				s_item = &SkinItems[item_id];

				pt[0].x = rc.left;
				pt[0].y = rc.top - (active ? 1 : 0);
				if (active || rc.left < 10) {
					pt[1].x = rc.left + 10;
					pt[1].y = rc.bottom - 4;
					pt[2].x = rc.left + 16;
					pt[2].y = rc.bottom;
				} else {
					pt[1].x = rc.left;
					pt[1].y = rc.bottom - 6;
					pt[2].x = rc.left + 6;
					pt[2].y = rc.bottom;
				}
				pt[3].x = rc.right;
				pt[3].y = rc.bottom;
				pt[4].x = rc.right;
				pt[4].y = rc.top - (active ? 1 : 0);
				rgn = CreatePolygonRgn(pt, 5, WINDING);
				if (!s_item->IGNORED) {
					if (active)
						rc.top--;
					SelectClipRgn(dc, rgn);
					CSkin::DrawItem(dc, &rc, s_item);
					SelectClipRgn(dc, 0);
				} else if (dat) {
					if (dat->mayFlashTab == TRUE)
						bg = PluginConfig.tabConfig.m_hBrushUnread;
					else if (nHint & HINT_ACTIVE_ITEM)
						bg = PluginConfig.tabConfig.m_hBrushActive;
					else if (nHint & HINT_HOTTRACK)
						bg = PluginConfig.tabConfig.m_hBrushHottrack;
					else
						bg = PluginConfig.tabConfig.m_hBrushDefault;
					FillRgn(dc, rgn, bg);
				}
				SelectObject(dc, PluginConfig.tabConfig.m_hPenStyledDark);
				if (active || rc.left < 10) {
					DrawWuLine(dc, pt[0].x, pt[0].y, pt[1].x, pt[1].y, PluginConfig.tabConfig.colors[9]);
					DrawWuLine(dc, pt[1].x, pt[1].y, pt[2].x, pt[2].y, PluginConfig.tabConfig.colors[9]);
					/*
					short basecolor;
					COLORREF clr = GetSysColor(COLOR_3DDKSHADOW);
					basecolor = (GetRValue(clr) + GetBValue(clr) + GetGValue(clr)) / 3;
					DrawWuLineBW(dc, pt[0].x, pt[0].y, pt[1].x, pt[1].y, basecolor, 256, 8);
					DrawWuLineBW(dc, pt[1].x, pt[1].y, pt[2].x, pt[2].y, basecolor, 256, 8);
					*/
					MoveToEx(dc, pt[2].x, pt[2].y, &pts);
					//MoveToEx(dc, pt[0].x, pt[0].y, &pts);
					//LineTo(dc, pt[1].x, pt[1].y);
					//LineTo(dc, pt[2].x, pt[2].y);
				} else {
					MoveToEx(dc, pt[1].x, pt[1].y, &pts);
					LineTo(dc, pt[2].x, pt[2].y);
				}
				LineTo(dc, pt[3].x, pt[3].y);
				LineTo(dc, pt[4].x, pt[4].y - 1);

				SelectObject(dc, PluginConfig.tabConfig.m_hPenStyledLight);
				MoveToEx(dc, pt[4].x - 1, pt[4].y, &pts);
				LineTo(dc, pt[3].x - 1, pt[3].y - 1);
				LineTo(dc, pt[2].x, pt[2].y - 1);
				if (active || rc.left < 10) {
					/*
					short basecolor;
					COLORREF clr = GetSysColor(COLOR_3DHILIGHT);
					basecolor = (GetRValue(clr) + GetBValue(clr) + GetGValue(clr)) / 3;
					DrawWuLineBW(dc, pt[2].x, pt[2].y - 1, pt[1].x, pt[1].y - 1, basecolor, 256, 8);
					DrawWuLineBW(dc, pt[1].x, pt[1].y - 1, pt[0].x + 1, pt[0].y, basecolor, 256, 8);
					*/
					DrawWuLine(dc, pt[2].x, pt[2].y - 1, pt[1].x + 1, pt[1].y - 1, PluginConfig.tabConfig.colors[8]);
					DrawWuLine(dc, pt[1].x + 1, pt[1].y - 1, pt[0].x + 1, pt[0].y, PluginConfig.tabConfig.colors[8]);
					//LineTo(dc, pt[1].x, pt[1].y - 1);
					//LineTo(dc, pt[0].x + 1, pt[0].y);
					if (rc.top > rcTabPage.bottom + 10 && !active) {
						DrawWuLine(dc, pt[0].x, pt[0].y - 1, pt[4].x, pt[4].y - 1, PluginConfig.tabConfig.colors[9]);
						DrawWuLine(dc, pt[0].x + 1, pt[0].y, pt[4].x - 1, pt[4].y, PluginConfig.tabConfig.colors[8]);
					}
				} else
					LineTo(dc, pt[1].x, pt[1].y - 1);
				DeleteObject(rgn);
			} else {
				MoveToEx(dc, rcItem->left, rcItem->top - (nHint & HINT_ACTIVE_ITEM ? 1 : 0), &pt);
				LineTo(dc, rcItem->left, rcItem->bottom - 2);
				LineTo(dc, rcItem->left + 2, rcItem->bottom);
				SelectObject(dc, PluginConfig.tabConfig.m_hPenShadow);
				LineTo(dc, rcItem->right - 3, rcItem->bottom);

				LineTo(dc, rcItem->right - 1, rcItem->bottom - 2);
				LineTo(dc, rcItem->right - 1, rcItem->top - 1);
				MoveToEx(dc, rcItem->right - 2, rcItem->top, &pt);
				SelectObject(dc, PluginConfig.tabConfig.m_hPenItemShadow);
				LineTo(dc, rcItem->right - 2, rcItem->bottom - 1);
				MoveToEx(dc, rcItem->right - 3, rcItem->bottom - 1, &pt);
				LineTo(dc, rcItem->left + 2, rcItem->bottom - 1);
			}
		} else {
			if (tabdat->m_moderntabs) {
				RECT rc = *rcItem;
				POINT pt[5], pts;
				BOOL active = nHint & HINT_ACTIVE_ITEM;
				HRGN rgn;
				TCITEM item = {0};
				struct _MessageWindowData *dat = 0;
				HBRUSH bg;
				LONG bendPoint;
				CSkinItem *s_item;
				int item_id;

				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(tabdat->hwnd, iItem, &item);

				if (IsWindow((HWND)item.lParam) && item.lParam != 0)
					dat = (struct _MessageWindowData *)GetWindowLongPtr((HWND)item.lParam, GWLP_USERDATA);

				if (active && rc.left > 10)
					rc.left -= 10;

				if (active)
					rc.top++;

				item_id = active ? ID_EXTBKTABITEMACTIVE : (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACK : ID_EXTBKTABITEM);
				s_item = &SkinItems[item_id];

				rc.right--;

				pt[0].x = rc.left;
				pt[0].y = rc.bottom + (active ? 1 : 0);
				bendPoint = (rc.bottom - rc.top) - 4;

				if (active || rc.left < 10) {
					pt[1].x = rc.left + 10;
					pt[1].y = rc.top + 4;
					pt[2].x = rc.left + 16;
					pt[2].y = rc.top;
				} else {
					pt[1].x = rc.left;
					pt[1].y = rc.top + 6;
					pt[2].x = rc.left + 6;
					pt[2].y = rc.top;
				}
				pt[3].x = rc.right;
				pt[3].y = rc.top;
				pt[4].x = rc.right;
				pt[4].y = rc.bottom + 1;
				rgn = CreatePolygonRgn(pt, 5, WINDING);
				if (!s_item->IGNORED) {
					if (active)
						rc.bottom++;
					SelectClipRgn(dc, rgn);
					DrawAlpha(dc, &rc, s_item->COLOR, s_item->ALPHA, s_item->COLOR2, s_item->COLOR2_TRANSPARENT, s_item->GRADIENT,
							  s_item->CORNER, s_item->BORDERSTYLE, s_item->imageItem);
					SelectClipRgn(dc, 0);
				} else if (dat) {
					if (dat->mayFlashTab == TRUE)
						bg = PluginConfig.tabConfig.m_hBrushUnread;
					else if (nHint & HINT_ACTIVE_ITEM)
						bg = PluginConfig.tabConfig.m_hBrushActive;
					else if (nHint & HINT_HOTTRACK)
						bg = PluginConfig.tabConfig.m_hBrushHottrack;
					else
						bg = PluginConfig.tabConfig.m_hBrushDefault;
					FillRgn(dc, rgn, bg);
				}
				SelectObject(dc, PluginConfig.tabConfig.m_hPenStyledDark);
				if (active || rc.left < 10) {
					//MoveToEx(dc, pt[0].x, pt[0].y - (active ? 1 : 0), &pts);
					//LineTo(dc, pt[1].x, pt[1].y);
					//LineTo(dc, pt[2].x, pt[2].y);
					DrawWuLine(dc, pt[0].x, pt[0].y - (active ? 1 : 0), pt[1].x, pt[1].y, PluginConfig.tabConfig.colors[9]);
					DrawWuLine(dc, pt[1].x, pt[1].y, pt[2].x, pt[2].y, PluginConfig.tabConfig.colors[9]);
					MoveToEx(dc, pt[2].x, pt[2].y, &pts);
				} else {
					MoveToEx(dc, pt[1].x, pt[1].y, &pts);
					LineTo(dc, pt[2].x, pt[2].y);
				}
				LineTo(dc, pt[3].x, pt[3].y);
				LineTo(dc, pt[4].x, pt[4].y);

				SelectObject(dc, PluginConfig.tabConfig.m_hPenStyledLight);
				MoveToEx(dc, pt[4].x - 1, pt[4].y - 1, &pts);
				LineTo(dc, pt[3].x - 1, pt[3].y + 1);
				LineTo(dc, pt[2].x, pt[2].y + 1);
				if (active || rc.left < 10) {
					//LineTo(dc, pt[1].x, pt[1].y + 1);
					//LineTo(dc, pt[0].x, pt[0].y);
					DrawWuLine(dc, pt[2].x, pt[2].y + 1, pt[1].x + 1, pt[1].y + 1, PluginConfig.tabConfig.colors[8]);
					DrawWuLine(dc, pt[1].x + 1, pt[1].y + 1, pt[0].x + 1, pt[0].y, PluginConfig.tabConfig.colors[8]);

					if (rc.bottom < rcTabPage.top - 10 && !active) {
						DrawWuLine(dc, pt[0].x, pt[0].y + 1, pt[4].x, pt[4].y + 1, PluginConfig.tabConfig.colors[9]);
						DrawWuLine(dc, pt[0].x + 1, pt[0].y, pt[4].x - 1, pt[4].y, PluginConfig.tabConfig.colors[8]);
					}
				} else
					LineTo(dc, pt[1].x, pt[1].y + 1);
				DeleteObject(rgn);
			} else {
				MoveToEx(dc, rcItem->left, rcItem->bottom, &pt);
				LineTo(dc, rcItem->left, rcItem->top + 2);
				LineTo(dc, rcItem->left + 2, rcItem->top);
				LineTo(dc, rcItem->right - 2, rcItem->top);
				SelectObject(dc, PluginConfig.tabConfig.m_hPenItemShadow);

				MoveToEx(dc, rcItem->right - 2, rcItem->top + 1, &pt);
				LineTo(dc, rcItem->right - 2, rcItem->bottom + 1);
				SelectObject(dc, PluginConfig.tabConfig.m_hPenShadow);
				MoveToEx(dc, rcItem->right - 1, rcItem->top + 2, &pt);
				LineTo(dc, rcItem->right - 1, rcItem->bottom + 1);
			}
		}
	}
}

static int DWordAlign(int n)
{
	int rem = n % 4;
	if (rem)
		n += (4 - rem);
	return n;
}

static HRESULT DrawThemesPartWithAero(const TabControlData *tabdat, HDC hDC, int iPartId, int iStateId, LPRECT prcBox)
{
	HRESULT hResult = 0;

	if (CMimAPI::m_pfnDrawThemeBackground == 0)
		return 0;

	if(tabdat->fAeroTabs) {
		int iState = (iStateId == PBS_NORMAL || iStateId == PBS_HOT) ? 2 : 1;
		if(tabdat->dwStyle & TCS_BOTTOM)// && iStateId == 3)
			prcBox->top += (iStateId == 3 ? 2 : 1);


		if(!(tabdat->dwStyle & TCS_BOTTOM)) {
			OffsetRect(prcBox, 0, 1);
			if(iStateId != 3)
				prcBox->bottom += 1;
		}

		FillRect(hDC, prcBox, CSkin::m_BrushBack);

		tabdat->helperItem->setAlphaFormat(AC_SRC_ALPHA, iStateId == 3 ? 255 : 240);
		tabdat->helperItem->Render(hDC, prcBox, true);
		tabdat->helperGlowItem->setAlphaFormat(AC_SRC_ALPHA, iStateId == 3 ? 220 : 180);

		/*
		 * glow effect for hot and/or selected tabs
		 */

		/*
		if(iStateId != PBS_NORMAL) {
			RECT rcGlow = *prcBox;
			if(tabdat->dwStyle & TCS_BOTTOM) {
				rcGlow.bottom -= 2;
				rcGlow.top = rcGlow.bottom - 18;
			} else {
				rcGlow.top += 2;
				rcGlow.bottom = rcGlow.top + 18;
			}
			rcGlow.left += 2;
			rcGlow.right -= 2;
			tabdat->helperGlowItem->Render(hDC, &rcGlow, true);
		}*/
		if(iStateId != PBS_NORMAL)
			tabdat->helperGlowItem->Render(hDC, prcBox, true);

		/*
		RECT rcEffect = *prcBox;
		InflateRect(&rcEffect, -3, 0);
		if(tabdat->dwStyle & TCS_BOTTOM)
			rcEffect.bottom -= 2;
		else
			rcEffect.top += 2;

		CSkin::ApplyAeroEffect(hDC, prcBox, CSkin::AERO_EFFECT_AREA_TAB_NORMAL |
							   (tabdat->dwStyle & TCS_BOTTOM ? CSkin::AERO_EFFECT_AREA_TAB_BOTTOM : CSkin::AERO_EFFECT_AREA_TAB_TOP), tabdat->hbp);
		*/
	}
	else {
		if (tabdat->hTheme != 0)
			hResult = CMimAPI::m_pfnDrawThemeBackground(tabdat->hTheme, hDC, iPartId, iStateId, prcBox, NULL);
	}

	return hResult;
}
/*
 * draws a theme part (identifier in uiPartNameID) using the given clipping rectangle
 */

static HRESULT DrawThemesPart(const TabControlData *tabdat, HDC hDC, int iPartId, int iStateId, LPRECT prcBox)
{
	HRESULT hResult = 0;

	if (CMimAPI::m_pfnDrawThemeBackground == 0)
		return 0;

	if (tabdat->hTheme != 0)
		hResult = CMimAPI::m_pfnDrawThemeBackground(tabdat->hTheme, hDC, iPartId, iStateId, prcBox, NULL);

	return hResult;
}

/*
 * draw a themed tab item. either a tab or the body pane
 * handles image mirroring for tabs at the bottom
 */

static void DrawThemesXpTabItem(HDC pDC, int ixItem, RECT *rcItem, UINT uiFlag, struct TabControlData *tabdat)
{
	BOOL bBody  = (uiFlag & 1) ? TRUE : FALSE;
	BOOL bSel   = (uiFlag & 2) ? TRUE : FALSE;
	BOOL bHot   = (uiFlag & 4) ? TRUE : FALSE;
	BOOL bBottom = (uiFlag & 8) ? TRUE : FALSE;	// mirror
	SIZE szBmp;
	HDC  dcMem;
	HBITMAP bmpMem, pBmpOld;
	RECT rcMem;
	BITMAPINFO biOut;
	BITMAPINFOHEADER *bihOut;
	int nBmpWdtPS;
	int nSzBuffPS;
	LPBYTE pcImg = NULL, pcImg1 = NULL;
	int nStart = 0, nLenSub = 0;
	szBmp.cx = rcItem->right - rcItem->left;
	szBmp.cy = rcItem->bottom - rcItem->top;

	/*
	 * for top row tabs, it's easy. Just draw to the provided dc (it's a mem dc already)
	 */

	//FillRect(pDC, rcItem, GetSysColorBrush(COLOR_3DFACE));
	if (!bBottom) {
		if (bBody) {
			if(PluginConfig.m_bIsVista) {
				rcItem->right += 2;							// hide right tab sheet shadow (only draw the actual border line)
				rcItem->bottom += 2;
			}
			DrawThemesPart(tabdat, pDC, 9, 0, rcItem);	// TABP_PANE id = 9
		} else {
			int iStateId = bSel ? 3 : (bHot ? 2 : 1);                       // leftmost item has different part id
			DrawThemesPartWithAero(tabdat, pDC, rcItem->left < 20 ? 2 : 1, iStateId, rcItem);
		}
		return;
	}
	else if(tabdat->fAeroTabs && !bBody) {
		int iStateId = bSel ? 3 : (bHot ? 2 : 1);                       // leftmost item has different part id
		DrawThemesPartWithAero(tabdat, pDC, rcItem->left < 20 ? 2 : 1, iStateId, rcItem);
		return;
	}

	/*
	 * remaining code is for bottom tabs only.
	 */

	dcMem = CreateCompatibleDC(pDC);
	bmpMem = CreateCompatibleBitmap(pDC, szBmp.cx, szBmp.cy);

	pBmpOld = (HBITMAP)SelectObject(dcMem, bmpMem);

	rcMem.left = rcMem.top = 0;
	rcMem.right = szBmp.cx;
	rcMem.bottom = szBmp.cy;

	ZeroMemory(&biOut, sizeof(BITMAPINFO));	// Fill local pixel arrays
	bihOut = &biOut.bmiHeader;

	bihOut->biSize = sizeof(BITMAPINFOHEADER);
	bihOut->biCompression = BI_RGB;
	bihOut->biPlanes = 1;
	bihOut->biBitCount = 24;	// force as RGB: 3 bytes, 24 bits
	bihOut->biWidth = szBmp.cx;
	bihOut->biHeight = szBmp.cy;

	nBmpWdtPS = DWordAlign(szBmp.cx * 3);
	nSzBuffPS = ((nBmpWdtPS * szBmp.cy) / 8 + 2) * 8;

	/*
	 * blit the background to the memory dc, so that transparent tabs will draw properly
	 * for bottom tabs, it's more complex, because the background part must not be mirrored
	 * the body part does not need that (filling with the background color is much faster
	 * and sufficient for the tab "page" part.
	 */

	if (!bSel)
		FillRect(dcMem, &rcMem, GetSysColorBrush(COLOR_3DFACE));        // only active (bSel == TRUE) tabs can overwrite others. for inactive, it's enough to fill with bg color
	else {
		/*
		 * mirror the background horizontally for bottom selected tabs (they can overwrite others)
		 * needed, because after drawing the theme part the images will again be mirrored
		 * to "flip" the tab item.
		 */
		BitBlt(dcMem, 0, 0, szBmp.cx, szBmp.cy, pDC, rcItem->left, rcItem->top, SRCCOPY);

		pcImg1 = (BYTE *)mir_alloc(nSzBuffPS);

		if (pcImg1) {
			GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg1, &biOut, DIB_RGB_COLORS);
			bihOut->biHeight = -szBmp.cy; 				// to mirror bitmap is eough to use negative height between Get/SetDIBits
			SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg1, &biOut, DIB_RGB_COLORS);
			mir_free(pcImg1);
		}
	}

	/*
	 * body may be *large* so rotating the final image can be very slow.
	 * workaround: draw the skin item (tab pane) into a small dc, rotate this (small) image and render
	 * it to the final DC with the IMG_RenderItem() routine.
	 */

	if (bBody) {
		HDC hdcTemp = CreateCompatibleDC(pDC);
		HBITMAP hbmTemp = CreateCompatibleBitmap(pDC, 100, 50);
		HBITMAP hbmTempOld = (HBITMAP)SelectObject(hdcTemp, hbmTemp);
		RECT rcTemp = {0};

		rcTemp.right = 100;
		rcTemp.bottom = 50;

		bihOut->biWidth = 100;
		bihOut->biHeight = 50;

		nBmpWdtPS = DWordAlign(100 * 3);
		nSzBuffPS = ((nBmpWdtPS * 50) / 8 + 2) * 8;

		FillRect(hdcTemp, &rcTemp, GetSysColorBrush(COLOR_3DFACE));
		DrawThemesPart(tabdat, hdcTemp, 9, 0, &rcTemp);	// TABP_PANE id = 9
		pcImg = (BYTE *)mir_alloc(nSzBuffPS);
		if (pcImg)	{									// get bits:
			GetDIBits(hdcTemp, hbmTemp, nStart, 50 - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
			bihOut->biHeight = -50;
			SetDIBits(hdcTemp, hbmTemp, nStart, 50 - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
			mir_free(pcImg);
		}
		CImageItem tempItem(10, 10, 10, 10, hdcTemp, 0, IMAGE_FLAG_DIVIDED | IMAGE_FILLSOLID,
							GetSysColorBrush(COLOR_3DFACE), 255, 30, 80, 50, 100);

		if(PluginConfig.m_bIsVista)	{								// hide right tab sheet shadow (only draw the actual border line)
			rcItem->right += 2;
			rcItem->bottom += 2;
		}

		tempItem.Render(pDC, rcItem, true);
		tempItem.Clear();
		SelectObject(hdcTemp, hbmTempOld);
		DeleteObject(hbmTemp);
		DeleteDC(hdcTemp);

		SelectObject(dcMem, pBmpOld);
		DeleteObject(bmpMem);
		DeleteDC(dcMem);
		return;
	} else {
		int iStateId = bSel ? 3 : (bHot ? 2 : 1);
		DrawThemesPart(tabdat, dcMem, rcItem->left < 20 ? 2 : 1, iStateId, &rcMem);
	}

	bihOut->biHeight = szBmp.cy;
	pcImg = (BYTE *)mir_alloc(nSzBuffPS);

	if (pcImg)	{									// get bits:
		GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
		bihOut->biHeight = -szBmp.cy;
		SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
		mir_free(pcImg);
	}

	/*
	 * finally, blit the result to the destination dc
	 */

	BitBlt(pDC, rcItem->left, rcItem->top, szBmp.cx, szBmp.cy, dcMem, 0, 0, SRCCOPY);
	SelectObject(dcMem, pBmpOld);
	DeleteObject(bmpMem);
	DeleteDC(dcMem);
}

static POINT ptMouseT = {0};

static LRESULT CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct		TabControlData *tabdat = 0;

	tabdat = (struct TabControlData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (tabdat) {
		if (tabdat->pContainer == NULL) {
			tabdat->pContainer = (ContainerWindowData *)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
			if(tabdat->pContainer)
				tabdat->m_moderntabs = (tabdat->pContainer->dwFlagsEx & TCF_STYLED);
		}
		tabdat->dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
	}

	switch (msg) {
		case WM_NCCREATE: {
			WNDCLASSEXA wcl = {0};

			wcl.cbSize = sizeof(wcl);
			GetClassInfoExA(g_hInst, "SysTabControl32", &wcl);

			tabdat = (struct TabControlData *)mir_alloc(sizeof(struct TabControlData));
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)tabdat);
			ZeroMemory((void *)tabdat, sizeof(struct TabControlData));
			tabdat->hwnd = hwnd;
			tabdat->cx = GetSystemMetrics(SM_CXSMICON);
			tabdat->cy = GetSystemMetrics(SM_CYSMICON);
			tabdat->fTipActive = FALSE;
			tabdat->iHoveredCloseIcon = -1;
			SendMessage(hwnd, EM_THEMECHANGED, 0, 0);
			OldTabControlClassProc = wcl.lpfnWndProc;
			return TRUE;
		}
		case EM_THEMECHANGED:
			tabdat->m_xpad = M->GetByte("x-pad", 3);
			tabdat->m_VisualStyles = FALSE;
			if (PluginConfig.m_bIsXP && M->isVSAPIState()) {
				if (CMimAPI::m_pfnIsThemeActive != 0)
					if (CMimAPI::m_pfnIsThemeActive()) {
						tabdat->m_VisualStyles = TRUE;
						if (tabdat->hTheme != 0 && CMimAPI::m_pfnCloseThemeData != 0) {
							CMimAPI::m_pfnCloseThemeData(tabdat->hTheme);
							CMimAPI::m_pfnCloseThemeData(tabdat->hThemeButton);
						}
						if (CMimAPI::m_pfnOpenThemeData != 0) {
							if ((tabdat->hTheme = CMimAPI::m_pfnOpenThemeData(hwnd, L"TAB")) == 0 || (tabdat->hThemeButton = CMimAPI::m_pfnOpenThemeData(hwnd, L"BUTTON")) == 0)
								tabdat->m_VisualStyles = FALSE;
						}
					}
			}
			return 0;
		case EM_SEARCHSCROLLER: {
			HWND hwndChild;
			/*
			 * search the updown control (scroll arrows) to subclass it...
			 * the control is dynamically created and may not exist as long as it is
			 * not needed. So we have to search it everytime we need to paint. However,
			 * it is sufficient to search it once. So this message is called, whenever
			 * a new tab is inserted
			 */

			if ((hwndChild = FindWindowEx(hwnd, 0, _T("msctls_updown32"), NULL)) != 0)
				DestroyWindow(hwndChild);
			return 0;
		}
		case EM_VALIDATEBOTTOM: {
			BOOL bClassicDraw = (tabdat->m_VisualStyles == FALSE);
			if ((tabdat->dwStyle & TCS_BOTTOM) && !bClassicDraw && PluginConfig.tabConfig.m_bottomAdjust != 0)
				InvalidateRect(hwnd, NULL, FALSE);
			else if (tabdat->m_moderntabs)
				InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
		case EM_REFRESHWITHOUTCLIP:
			if (TabCtrl_GetItemCount(hwnd) > 1)
				return 0;
			else {
				tabdat->bRefreshWithoutClip = TRUE;
				RedrawWindow(hwnd, NULL, NULL, RDW_UPDATENOW | RDW_NOCHILDREN | RDW_INVALIDATE);
				tabdat->bRefreshWithoutClip = FALSE;
				return 0;
			}
		case TCM_GETITEMRECT: {
			RECT *rc = (RECT *)lParam;
			LRESULT result = CallWindowProc(OldTabControlClassProc, hwnd, msg, wParam, lParam);
			RECT rcControl;


			if (tabdat->m_moderntabs && tabdat->dwStyle & TCS_MULTILINE) {
				GetClientRect(hwnd, &rcControl);
				if (rc->left < 10)
					rc->right += 10;
				else if (rc->right > rcControl.right - 10)
					rc->left += 10;
				else
					OffsetRect(rc, 10, 0);
			}
			return result;
		}
		case TCM_INSERTITEM:
		case TCM_DELETEITEM:
			tabdat->iHoveredCloseIcon = -1;
			if (!(tabdat->dwStyle & TCS_MULTILINE) || tabdat->dwStyle & TCS_BUTTONS) {
				LRESULT result;
				RECT rc;
				int iTabs = TabCtrl_GetItemCount(hwnd);
				if (iTabs >= 1 && msg == TCM_INSERTITEM) {
					TabCtrl_GetItemRect(hwnd, 0, &rc);
					TabCtrl_SetItemSize(hwnd, 10, rc.bottom - rc.top);
				}
				result = CallWindowProc(OldTabControlClassProc, hwnd, msg, wParam, lParam);
				TabCtrl_GetItemRect(hwnd, 0, &rc);
				SendMessage(hwnd, WM_SIZE, 0, 0);
				return result;
			}
			break;
		case WM_DESTROY:
			if (tabdat) {
				if (tabdat->hTheme != 0 && CMimAPI::m_pfnCloseThemeData != 0) {
					CMimAPI::m_pfnCloseThemeData(tabdat->hTheme);
					CMimAPI::m_pfnCloseThemeData(tabdat->hThemeButton);
				}
			}
			break;
		case WM_NCDESTROY:
			if(tabdat) {
				mir_free(tabdat);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, 0L);
			}
			break;
		case WM_MBUTTONDOWN: {
			POINT pt;
			GetCursorPos(&pt);
			SendMessage(GetParent(hwnd), DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
			return 1;
		}
		case WM_SETCURSOR: {
			POINT pt;

			GetCursorPos(&pt);
			SendMessage(GetParent(hwnd), msg, wParam, lParam);
			if (abs(pt.x - ptMouseT.x) < 4  && abs(pt.y - ptMouseT.y) < 4)
				return 1;
			ptMouseT = pt;
			if (tabdat->fTipActive) {
				KillTimer(hwnd, TIMERID_HOVER_T);
				CallService("mToolTip/HideTip", 0, 0);
				tabdat->fTipActive = FALSE;
			}
			KillTimer(hwnd, TIMERID_HOVER_T);
			if(tabdat->pContainer && (!tabdat->pContainer->SideBar->isActive() && (TabCtrl_GetItemCount(hwnd) > 1 || !(tabdat->pContainer->dwFlags & CNT_HIDETABS))))
				SetTimer(hwnd, TIMERID_HOVER_T, 750, 0);
			break;
		}
		case WM_SIZE: {
			int iTabs = TabCtrl_GetItemCount(hwnd);

			if (!tabdat->pContainer)
				break;

			if (!(tabdat->dwStyle & TCS_MULTILINE)) {
				RECT rcClient, rc;
				DWORD newItemSize;
				if (iTabs > (tabdat->pContainer->dwFlags & CNT_HIDETABS ? 1 : 0)) {
					GetClientRect(hwnd, &rcClient);
					TabCtrl_GetItemRect(hwnd, iTabs - 1, &rc);
					newItemSize = (rcClient.right - 6) - (tabdat->dwStyle & TCS_BUTTONS ? (iTabs) * 10 : 0);
					newItemSize = newItemSize / iTabs;
					if (newItemSize < PluginConfig.tabConfig.m_fixedwidth) {
						TabCtrl_SetItemSize(hwnd, newItemSize, rc.bottom - rc.top);
					} else {
						TabCtrl_SetItemSize(hwnd, PluginConfig.tabConfig.m_fixedwidth, rc.bottom - rc.top);
					}
					SendMessage(hwnd, EM_SEARCHSCROLLER, 0, 0);
				}
			} else if (tabdat->dwStyle & TCS_BUTTONS && iTabs > 0) {
				RECT rcClient, rcItem;
				int nrTabsPerLine;
				GetClientRect(hwnd, &rcClient);
				TabCtrl_GetItemRect(hwnd, 0, &rcItem);
				nrTabsPerLine = (rcClient.right) / PluginConfig.tabConfig.m_fixedwidth;
				if (iTabs >= nrTabsPerLine && nrTabsPerLine > 0)
					TabCtrl_SetItemSize(hwnd, ((rcClient.right - 6) / nrTabsPerLine) - (tabdat->dwStyle & TCS_BUTTONS ? 8 : 0), rcItem.bottom - rcItem.top);
				else
					TabCtrl_SetItemSize(hwnd, PluginConfig.tabConfig.m_fixedwidth, rcItem.bottom - rcItem.top);
			}
			break;
		}
		case WM_LBUTTONDBLCLK: {
			POINT pt;
			GetCursorPos(&pt);
			SendMessage(GetParent(hwnd), DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
			break;
		}
		case WM_RBUTTONDOWN:
			KillTimer(hwnd, TIMERID_HOVER_T);
			CallService("mToolTip/HideTip", 0, 0);
			tabdat->fTipActive = FALSE;
			break;

		case WM_LBUTTONDOWN: {
			TCHITTESTINFO tci = {0};

			KillTimer(hwnd, TIMERID_HOVER_T);
			CallService("mToolTip/HideTip", 0, 0);
			tabdat->fTipActive = FALSE;

			if (GetKeyState(VK_CONTROL) & 0x8000) {
				tci.pt.x = (short)LOWORD(GetMessagePos());
				tci.pt.y = (short)HIWORD(GetMessagePos());
				if (DragDetect(hwnd, tci.pt) && TabCtrl_GetItemCount(hwnd) > 1) {
					int i;
					tci.flags = TCHT_ONITEM;

					ScreenToClient(hwnd, &tci.pt);
					i = TabCtrl_HitTest(hwnd, &tci);
					if (i != -1) {
						TCITEM tc;
						struct _MessageWindowData *dat = NULL;

						tc.mask = TCIF_PARAM;
						TabCtrl_GetItem(hwnd, i, &tc);
						dat = (struct _MessageWindowData *)GetWindowLongPtr((HWND)tc.lParam, GWLP_USERDATA);
						if (dat)	{
							tabdat->bDragging = TRUE;
							tabdat->iBeginIndex = i;
							tabdat->hwndDrag = (HWND)tc.lParam;
							tabdat->dragDat = dat;
							tabdat->fSavePos = TRUE;
							tabdat->himlDrag = ImageList_Create(16, 16, ILC_MASK | (PluginConfig.m_bIsXP ? ILC_COLOR32 : ILC_COLOR16), 1, 0);
							ImageList_AddIcon(tabdat->himlDrag, dat->hTabIcon);
							ImageList_BeginDrag(tabdat->himlDrag, 0, 8, 8);
							ImageList_DragEnter(hwnd, tci.pt.x, tci.pt.y);
							SetCapture(hwnd);
						}
						return TRUE;
					}
				}
			}

			if (GetKeyState(VK_MENU) & 0x8000) {
				tci.pt.x = (short)LOWORD(GetMessagePos());
				tci.pt.y = (short)HIWORD(GetMessagePos());
				if (DragDetect(hwnd, tci.pt) && TabCtrl_GetItemCount(hwnd) > 1) {
					int i;
					tci.flags = TCHT_ONITEM;

					ScreenToClient(hwnd, &tci.pt);
					i = TabCtrl_HitTest(hwnd, &tci);
					if (i != -1) {
						TCITEM tc;
						_MessageWindowData *dat = NULL;

						tc.mask = TCIF_PARAM;
						TabCtrl_GetItem(hwnd, i, &tc);
						dat = (_MessageWindowData *)GetWindowLongPtr((HWND)tc.lParam, GWLP_USERDATA);
						if (dat)	{
							tabdat->bDragging = TRUE;
							tabdat->iBeginIndex = i;
							tabdat->hwndDrag = (HWND)tc.lParam;
							tabdat->dragDat = dat;
							tabdat->himlDrag = ImageList_Create(16, 16, ILC_MASK | (PluginConfig.m_bIsXP ? ILC_COLOR32 : ILC_COLOR16), 1, 0);
							tabdat->fSavePos = FALSE;
							ImageList_AddIcon(tabdat->himlDrag, dat->hTabIcon);
							ImageList_BeginDrag(tabdat->himlDrag, 0, 8, 8);
							ImageList_DragEnter(hwnd, tci.pt.x, tci.pt.y);
							SetCapture(hwnd);
						}
						return TRUE;
					}
				}
			}
			if(tabdat->fCloseButton) {
				POINT	pt;
				GetCursorPos(&pt);

				if(TabCtrl_TestForCloseButton(tabdat, hwnd, pt) != -1)
					return(TRUE);
			}
		}
		break;

		case WM_CAPTURECHANGED: {
			tabdat->bDragging = FALSE;
			ImageList_DragLeave(hwnd);
			ImageList_EndDrag();
			if (tabdat->himlDrag) {
				ImageList_RemoveAll(tabdat->himlDrag);
				ImageList_Destroy(tabdat->himlDrag);
				tabdat->himlDrag = 0;
			}
		}
		break;

		case WM_MOUSEMOVE: {
			if (tabdat->bDragging) {
				TCHITTESTINFO tci = {0};
				tci.pt.x = (short)LOWORD(GetMessagePos());
				tci.pt.y = (short)HIWORD(GetMessagePos());
				ScreenToClient(hwnd, &tci.pt);
				ImageList_DragMove(tci.pt.x, tci.pt.y);
			}
			if(tabdat->fCloseButton) {
				POINT	pt;

				GetCursorPos(&pt);
				int iOldHovered = tabdat->iHoveredCloseIcon;
				tabdat->iHoveredCloseIcon = TabCtrl_TestForCloseButton(tabdat, hwnd, pt);
				if(tabdat->iHoveredCloseIcon != iOldHovered)
					InvalidateRect(hwnd, NULL, FALSE);
			}
		}
		break;

		case WM_LBUTTONUP: {
			CallWindowProc(OldTabControlClassProc, hwnd, msg, wParam, lParam);
			if (tabdat->bDragging && ReleaseCapture()) {
				TCHITTESTINFO tci = {0};
				int i;
				tci.pt.x = (short)LOWORD(GetMessagePos());
				tci.pt.y = (short)HIWORD(GetMessagePos());
				tci.flags = TCHT_ONITEM;
				tabdat->bDragging = FALSE;
				ImageList_DragLeave(hwnd);
				ImageList_EndDrag();

				ScreenToClient(hwnd, &tci.pt);
				i = TabCtrl_HitTest(hwnd, &tci);
				if (i != -1 && i != tabdat->iBeginIndex)
					RearrangeTab(tabdat->hwndDrag, tabdat->dragDat, MAKELONG(i, 0xffff), tabdat->fSavePos);
				tabdat->hwndDrag = (HWND) - 1;
				tabdat->dragDat = NULL;
				if (tabdat->himlDrag) {
					ImageList_RemoveAll(tabdat->himlDrag);
					ImageList_Destroy(tabdat->himlDrag);
					tabdat->himlDrag = 0;
				}
			}
			if(tabdat->fCloseButton) {
				POINT	pt;

				GetCursorPos(&pt);
				int iItem = TabCtrl_TestForCloseButton(tabdat, hwnd, pt);
				if(iItem != -1)
					SendMessage(GetParent(hwnd), DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
			}
		}
		break;

		case WM_ERASEBKGND:
			if (tabdat->pContainer && (CSkin::m_skinEnabled || M->isAero()))
				return TRUE;
			return(0);
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdcreal, hdc;
			RECT rectTemp, rctPage, rctActive, rcItem, rctClip, rctOrig;
			RECT rectUpDn = {0, 0, 0, 0};
			int nCount = TabCtrl_GetItemCount(hwnd), i;
			TCITEM item = {0};
			int iActive, hotItem;
			POINT pt;
			DWORD dwStyle = tabdat->dwStyle;
			HPEN hPenOld = 0;
			UINT uiFlags = 1;
			UINT uiBottom = 0;
			TCHITTESTINFO hti;
			HBITMAP bmpMem, bmpOld;
			DWORD cx, cy;
			bool  isAero = M->isAero();
			HANDLE hpb = 0;

			BOOL bClassicDraw = !isAero && (tabdat->m_VisualStyles == FALSE || tabdat->m_moderntabs || CSkin::m_skinEnabled);

			if(GetUpdateRect(hwnd, NULL, TRUE) == 0)
				break;

			tabdat->fAeroTabs = isAero ? TRUE : FALSE;
			tabdat->fCloseButton = tabdat->pContainer ? (tabdat->pContainer->dwFlagsEx & TCF_CLOSEBUTTON ? TRUE : FALSE) : FALSE;

			tabdat->helperDat = 0;

			if(tabdat->fAeroTabs && tabdat->pContainer) {
				_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(tabdat->pContainer->hwndActive, GWLP_USERDATA);
				if(dat) {
					tabdat->helperDat = dat;
				}
				else
					tabdat->fAeroTabs = 0;

				tabdat->helperItem =  (dwStyle & TCS_BOTTOM) ? CSkin::m_tabBottom : CSkin::m_tabTop;
				tabdat->helperGlowItem = (dwStyle & TCS_BOTTOM) ? CSkin::m_tabGlowBottom : CSkin::m_tabGlowTop;
			}
			else
				tabdat->fAeroTabs = FALSE;

			if(tabdat->pContainer)
				tabdat->m_moderntabs = !tabdat->fAeroTabs && (tabdat->pContainer->dwFlagsEx & TCF_STYLED) &&
				!(tabdat->dwStyle & TCS_BUTTONS) && !(CSkin::m_skinEnabled);

			hdcreal = BeginPaint(hwnd, &ps);

			/*
			 * switchbar is active, don't paint a single pixel, the tab control won't be visible at all
			 */

			if(tabdat->pContainer->dwFlags & CNT_SIDEBAR) {
				if(nCount == 0)
					FillRect(hdcreal, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
				EndPaint(hwnd, &ps);
				return(0);
			}

			GetClientRect(hwnd, &rctPage);
			rctOrig = rctPage;
			iActive = TabCtrl_GetCurSel(hwnd);
			TabCtrl_GetItemRect(hwnd, iActive, &rctActive);
			cx = rctPage.right - rctPage.left;
			cy = rctPage.bottom - rctPage.top;

			/*
			 * draw everything to a memory dc to avoid flickering
			 */

			if(CMimAPI::m_haveBufferedPaint)
				hpb = tabdat->hbp = CSkin::InitiateBufferedPaint(hdcreal, rctPage, hdc);
			else {
				hdc = CreateCompatibleDC(hdcreal);
				bmpMem = tabdat->fAeroTabs ? CSkin::CreateAeroCompatibleBitmap(rctPage, hdcreal) : CreateCompatibleBitmap(hdcreal, cx, cy);
				bmpOld = (HBITMAP)SelectObject(hdc, bmpMem);
			}

			if (nCount == 1 && tabdat->pContainer->dwFlags & CNT_HIDETABS)
				rctClip = rctPage;

			if (CSkin::m_skinEnabled)
				CSkin::SkinDrawBG(hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, &rctPage, hdc);
			else
				//FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
				FillRect(hdc, &rctPage, GetSysColorBrush(COLOR_3DFACE));

			if (dwStyle & TCS_BUTTONS) {
				RECT rc1;
				TabCtrl_GetItemRect(hwnd, nCount - 1, &rc1);
				if (dwStyle & TCS_BOTTOM) {
					rctPage.bottom = rc1.top;
					uiBottom = 8;
				} else {
					rctPage.top = rc1.bottom + 2;
					uiBottom = 0;
				}
			} else {
				if (dwStyle & TCS_BOTTOM) {
					rctPage.bottom = rctActive.top;
					uiBottom = 8;
				} else {
					rctPage.top = rctActive.bottom;
					uiBottom = 0;
				}
			}

			if (nCount > 1 || !(tabdat->pContainer->dwFlags & CNT_HIDETABS)) {
				rctClip = rctPage;
				InflateRect(&rctClip, -tabdat->pContainer->tBorder, -tabdat->pContainer->tBorder);
			}

			hPenOld = (HPEN)SelectObject(hdc, PluginConfig.tabConfig.m_hPenLight);
			/*
			 * visual style support
			 */

			CopyRect(&rcTabPage, &rctPage);
			if (!tabdat->bRefreshWithoutClip)
				ExcludeClipRect(hdc, rctClip.left, rctClip.top, rctClip.right, rctClip.bottom);
			if (!bClassicDraw && IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
				RECT rcClient = rctPage;
				if (dwStyle & TCS_BOTTOM) {
					rcClient.bottom = rctPage.bottom;
					uiFlags |= uiBottom;
				} else
					rcClient.top = rctPage.top;
				DrawThemesXpTabItem(hdc, -1, &rcClient, uiFlags, tabdat);	// TABP_PANE=9,0,'TAB'
				if (tabdat->bRefreshWithoutClip)
					goto skip_tabs;
			} else {
				if (IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
					if (CSkin::m_skinEnabled) {
						CSkinItem *item = &SkinItems[ID_EXTBKTABPAGE];

						if (!item->IGNORED) {
							DrawAlpha(hdc, &rctPage, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
									  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
							goto page_done;
						}
					}

					if (tabdat->bRefreshWithoutClip)
						goto skip_tabs;

					if (dwStyle & TCS_BUTTONS) {
						rectTemp = rctPage;
						if (dwStyle & TCS_BOTTOM) {
							rectTemp.top--;
							rectTemp.bottom--;
						} else {
							rectTemp.bottom--;
							rectTemp.top++;
						}
						MoveToEx(hdc, rectTemp.left, rectTemp.bottom, &pt);
						LineTo(hdc, rectTemp.left, rectTemp.top + 1);
						LineTo(hdc, rectTemp.right - 1, rectTemp.top + 1);
						SelectObject(hdc, PluginConfig.tabConfig.m_hPenShadow);
						LineTo(hdc, rectTemp.right - 1, rectTemp.bottom);
						LineTo(hdc, rectTemp.left, rectTemp.bottom);
					} else {
						rectTemp = rctPage;

						if (tabdat->m_moderntabs)
							SelectObject(hdc, PluginConfig.tabConfig.m_hPenItemShadow);

						MoveToEx(hdc, rectTemp.left, rectTemp.bottom - 1, &pt);
						LineTo(hdc, rectTemp.left, rectTemp.top);

						if (dwStyle & TCS_BOTTOM) {
							LineTo(hdc, rectTemp.right - 1, rectTemp.top);
							SelectObject(hdc, PluginConfig.tabConfig.m_hPenShadow);
							LineTo(hdc, rectTemp.right - 1, rectTemp.bottom - 1);
							LineTo(hdc, rctActive.right, rectTemp.bottom - 1);
							MoveToEx(hdc, rctActive.left - 2, rectTemp.bottom - 1, &pt);
							LineTo(hdc, rectTemp.left - 1, rectTemp.bottom - 1);
							SelectObject(hdc, PluginConfig.tabConfig.m_hPenItemShadow);
							if (!tabdat->m_moderntabs) {
								MoveToEx(hdc, rectTemp.right - 2, rectTemp.top + 1, &pt);
								LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
								LineTo(hdc, rctActive.right, rectTemp.bottom - 2);
								MoveToEx(hdc, rctActive.left - 2, rectTemp.bottom - 2, &pt);
								LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);
							}
						} else {
							if (rctActive.left >= 0) {
								LineTo(hdc, rctActive.left, rctActive.bottom);
								if (IsRectEmpty(&rectUpDn))
									MoveToEx(hdc, rctActive.right, rctActive.bottom, &pt);
								else {
									if (rctActive.right >= rectUpDn.left)
										MoveToEx(hdc, rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN + 2, rctActive.bottom + 1, &pt);
									else
										MoveToEx(hdc, rctActive.right, rctActive.bottom, &pt);
								}
								LineTo(hdc, rectTemp.right - (tabdat->m_moderntabs ? 1 : 2), rctActive.bottom);
							} else {
								RECT rectItemLeftmost;
								UINT nItemLeftmost = FindLeftDownItem(hwnd);
								TabCtrl_GetItemRect(hwnd, nItemLeftmost, &rectItemLeftmost);
								LineTo(hdc, rectTemp.right - 2, rctActive.bottom);
							}
							if (!tabdat->m_moderntabs) {
								SelectObject(hdc, PluginConfig.tabConfig.m_hPenItemShadow);
								LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
								LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);
							}

							SelectObject(hdc, tabdat->m_moderntabs ? PluginConfig.tabConfig.m_hPenItemShadow : PluginConfig.tabConfig.m_hPenShadow);
							MoveToEx(hdc, rectTemp.right - 1, rctActive.bottom, &pt);
							LineTo(hdc, rectTemp.right - 1, rectTemp.bottom - 1);
							LineTo(hdc, rectTemp.left - 2, rectTemp.bottom - 1);
						}
					}
				}
			}
page_done:
			/*
			 * if aero is active _and_ the infopanel is visible in the current window, we "flatten" out the top area
			 * of the tab page by overpainting it black (thus it will appear transparent)
			 */
			if(tabdat->fAeroTabs && tabdat->helperDat) {
				RECT	rcLog, rcPage;
				POINT	pt;

				GetClientRect(hwnd, &rcPage);
				if(dwStyle & TCS_BOTTOM) {
					GetWindowRect(tabdat->helperDat->hwnd, &rcLog);
					pt.y = rcLog.bottom;
					pt.x = rcLog.left;
					ScreenToClient(hwnd, &pt);
					rcPage.top = pt.y + ((nCount > 1 || !(tabdat->helperDat->pContainer->dwFlags & CNT_HIDETABS)) ? tabdat->helperDat->pContainer->tBorder : 0);
					FillRect(hdc, &rcPage, CSkin::m_BrushBack);
					rcPage.top = 0;
				}
				GetWindowRect(GetDlgItem(tabdat->helperDat->hwnd, tabdat->helperDat->bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG), &rcLog);
				pt.y = rcLog.top;
				pt.x = rcLog.left;
				ScreenToClient(hwnd, &pt);
				rcPage.bottom = pt.y;
				FillRect(hdc, &rcPage, CSkin::m_BrushBack);
			}

			uiFlags = 0;
			/*
			 * figure out hottracked item (if any)
			 */

			if (tabdat->bRefreshWithoutClip)
				goto skip_tabs;

			GetCursorPos(&hti.pt);
			ScreenToClient(hwnd, &hti.pt);
			hti.flags = 0;
			hotItem = TabCtrl_HitTest(hwnd, &hti);
			for (i = 0; i < nCount; i++) {
				if (i != iActive) {
					TabCtrl_GetItemRect(hwnd, i, &rcItem);
					if (!bClassicDraw && uiBottom) {
						rcItem.top -= PluginConfig.tabConfig.m_bottomAdjust;
						rcItem.bottom -= PluginConfig.tabConfig.m_bottomAdjust;
					}
					if (IntersectRect(&rectTemp, &rcItem, &ps.rcPaint) || bClassicDraw) {
						int nHint = 0;
						if (!bClassicDraw && !(dwStyle & TCS_BUTTONS)) {
							DrawThemesXpTabItem(hdc, i, &rcItem, uiFlags | uiBottom | (i == hotItem ? 4 : 0), tabdat);
							DrawItem(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0), i);
						} else {
							DrawItemRect(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0), i);
							DrawItem(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0), i);
						}
					}
				}
			}
			/*
			 * draw the active item
			 */
			if (!bClassicDraw && uiBottom) {
				rctActive.top -= PluginConfig.tabConfig.m_bottomAdjust;
				rctActive.bottom -= PluginConfig.tabConfig.m_bottomAdjust;
			}
			if (rctActive.left >= 0) {
				int nHint = 0;
				rcItem = rctActive;
				if (!bClassicDraw && !(dwStyle & TCS_BUTTONS)) {
					InflateRect(&rcItem, 2, 2);
					DrawThemesXpTabItem(hdc, iActive, &rcItem, 2 | uiBottom, tabdat);
					DrawItem(tabdat, hdc, &rcItem, nHint | HINT_ACTIVE_ITEM, iActive);
				} else {
					if (!(dwStyle & TCS_BUTTONS)) {
						if (iActive == 0) {
							rcItem.right += 2;
							rcItem.left--;
						} else
							InflateRect(&rcItem, 2, 0);
					}
					DrawItemRect(tabdat, hdc, &rcItem, HINT_ACTIVATE_RIGHT_SIDE | HINT_ACTIVE_ITEM | nHint, iActive);
					DrawItem(tabdat, hdc, &rcItem, HINT_ACTIVE_ITEM | nHint, iActive);
				}
			}
skip_tabs:
			if (hPenOld)
				SelectObject(hdc, hPenOld);

			/*
			 * finally, bitblt the contents of the memory dc to the real dc
			 */
			//if(!tabdat->pContainer->bSkinned)
			if (!tabdat->bRefreshWithoutClip)
				ExcludeClipRect(hdcreal, rctClip.left, rctClip.top, rctClip.right, rctClip.bottom);

			if(hpb)
				CSkin::FinalizeBufferedPaint(hpb, &rctOrig);
				//CMimAPI::m_pfnEndBufferedPaint(hpb, TRUE);
			else {
				BitBlt(hdcreal, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY);
				SelectObject(hdc, bmpOld);
				DeleteObject(bmpMem);
				DeleteDC(hdc);
			}
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_TIMER: {
			if (wParam == TIMERID_HOVER_T &&  M->GetByte("d_tooltips", 0)) {
				POINT pt;
				CLCINFOTIP ti = {0};
				ti.cbSize = sizeof(ti);

				KillTimer(hwnd, TIMERID_HOVER_T);
				GetCursorPos(&pt);
				if (abs(pt.x - ptMouseT.x) < 5 && abs(pt.y - ptMouseT.y) < 5) {
					TCITEM item = {0};
					int    nItem = 0;
					struct _MessageWindowData *dat = 0;

					ti.ptCursor = pt;
					//ScreenToClient(hwnd, &pt);

					item.mask = TCIF_PARAM;
					nItem = GetTabItemFromMouse(hwnd, &pt);
					if (nItem >= 0 && nItem < TabCtrl_GetItemCount(hwnd)) {
						TabCtrl_GetItem(hwnd, nItem, &item);
						/*
						 * get the message window data for the session to which this tab item belongs
						 */

						if (IsWindow((HWND)item.lParam) && item.lParam != 0)
							dat = (struct _MessageWindowData *)GetWindowLongPtr((HWND)item.lParam, GWLP_USERDATA);
						if (dat) {
							tabdat->fTipActive = TRUE;
							ti.isGroup = 0;
							ti.hItem = dat->hContact;
							ti.isTreeFocused = 0;
							CallService("mToolTip/ShowTip", 0, (LPARAM)&ti);
						}
					}
				}
			}
			break;
		}
		case WM_MOUSEWHEEL: {
			short amount = (short)(HIWORD(wParam));
			if (lParam != -1)
				break;
			if (amount > 0)
				SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_PREV, 0);
			else if (amount < 0)
				SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_NEXT, 0);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
		case  WM_USER + 100: {
			if (tabdat->fTipActive) {
				tabdat->fTipActive = FALSE;
				CallService("mToolTip/HideTip", 0, 0);
			}
		}
	}
	return CallWindowProc(OldTabControlClassProc, hwnd, msg, wParam, lParam);
}

/*
 * load the tab control configuration data (colors, fonts, flags...
 */

void ReloadTabConfig()
{
	NONCLIENTMETRICS nclim;
	int i = 0;
	BOOL iLabelDefault = PluginConfig.m_TabAppearance & TCF_LABELUSEWINCOLORS;
	BOOL iBkgDefault = PluginConfig.m_TabAppearance & TCF_BKGUSEWINCOLORS;

	PluginConfig.tabConfig.m_hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	PluginConfig.tabConfig.m_hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
	PluginConfig.tabConfig.m_hPenItemShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

	nclim.cbSize = sizeof(nclim);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nclim, 0);
	PluginConfig.tabConfig.m_hMenuFont = CreateFontIndirect(&nclim.lfMessageFont);

	while (tabcolors[i].szKey != NULL) {
		if (i < 4)
			PluginConfig.tabConfig.colors[i] = iLabelDefault ? GetSysColor(tabcolors[i].defclr) : M->GetDword(CSkin::m_skinEnabled ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
		else
			PluginConfig.tabConfig.colors[i] = iBkgDefault ? GetSysColor(tabcolors[i].defclr) :  M->GetDword(CSkin::m_skinEnabled ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
		i++;
	}
	PluginConfig.tabConfig.m_hBrushDefault = CreateSolidBrush(PluginConfig.tabConfig.colors[4]);
	PluginConfig.tabConfig.m_hBrushActive = CreateSolidBrush(PluginConfig.tabConfig.colors[5]);
	PluginConfig.tabConfig.m_hBrushUnread = CreateSolidBrush(PluginConfig.tabConfig.colors[6]);
	PluginConfig.tabConfig.m_hBrushHottrack = CreateSolidBrush(PluginConfig.tabConfig.colors[7]);

	PluginConfig.tabConfig.m_bottomAdjust = (int)M->GetDword("bottomadjust", 0);
	PluginConfig.tabConfig.m_fixedwidth = M->GetDword("fixedwidth", FIXED_TAB_SIZE);

	PluginConfig.tabConfig.m_fixedwidth = (PluginConfig.tabConfig.m_fixedwidth < 60 ? 60 : PluginConfig.tabConfig.m_fixedwidth);
	PluginConfig.tabConfig.m_hPenStyledLight = CreatePen(PS_SOLID, 1, PluginConfig.tabConfig.colors[8]);
	PluginConfig.tabConfig.m_hPenStyledDark = CreatePen(PS_SOLID, 1, PluginConfig.tabConfig.colors[9]);
}

void FreeTabConfig()
{
	if(PluginConfig.tabConfig.m_hPenItemShadow)
		DeleteObject(PluginConfig.tabConfig.m_hPenItemShadow);

	if(PluginConfig.tabConfig.m_hPenLight)
		DeleteObject(PluginConfig.tabConfig.m_hPenLight);

	if(PluginConfig.tabConfig.m_hPenShadow)
		DeleteObject(PluginConfig.tabConfig.m_hPenShadow);

	if(PluginConfig.tabConfig.m_hMenuFont)
		DeleteObject(PluginConfig.tabConfig.m_hMenuFont);

	if(PluginConfig.tabConfig.m_hBrushActive)
		DeleteObject(PluginConfig.tabConfig.m_hBrushActive);

	if(PluginConfig.tabConfig.m_hBrushDefault)
		DeleteObject(PluginConfig.tabConfig.m_hBrushDefault);

	if(PluginConfig.tabConfig.m_hBrushUnread)
		DeleteObject(PluginConfig.tabConfig.m_hBrushUnread);

	if(PluginConfig.tabConfig.m_hBrushHottrack)
		DeleteObject(PluginConfig.tabConfig.m_hBrushHottrack);

	if(PluginConfig.tabConfig.m_hPenStyledDark)
		DeleteObject(PluginConfig.tabConfig.m_hPenStyledDark);

	if(PluginConfig.tabConfig.m_hPenStyledLight)
		DeleteObject(PluginConfig.tabConfig.m_hPenStyledLight);

	ZeroMemory(&PluginConfig.tabConfig, sizeof(myTabCtrl));
}

/*
 * options dialog for setting up tab options
 */

INT_PTR CALLBACK DlgProcTabConfig(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG: {
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_USER + 100, 0, 0);
			ShowWindow(hwndDlg, SW_SHOWNORMAL);
			return TRUE;
		}
		case WM_USER + 100: {
			DWORD dwFlags = M->GetDword("tabconfig", TCF_DEFAULT);
			int i = 0;
			COLORREF clr;

			CheckDlgButton(hwndDlg, IDC_LABELUSEWINCOLORS, !CSkin::m_skinEnabled && dwFlags & TCF_LABELUSEWINCOLORS);
			CheckDlgButton(hwndDlg, IDC_BKGUSEWINCOLORS2, !CSkin::m_skinEnabled && dwFlags & TCF_BKGUSEWINCOLORS);

			SendMessage(hwndDlg, WM_COMMAND, IDC_LABELUSEWINCOLORS, 0);

			while (tabcolors[i].szKey != NULL) {
				clr = (COLORREF)M->GetDword(CSkin::m_skinEnabled ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
				SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_SETCOLOUR, 0, (LPARAM)clr);
				SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(tabcolors[i].defclr));
				i++;
			}
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPIN, UDM_SETPOS, 0, (int)M->GetByte(CSkin::m_skinEnabled ? "S_tborder" : "tborder", 2));
			SetDlgItemInt(hwndDlg, IDC_TABBORDER, (int)M->GetByte(CSkin::m_skinEnabled ? "S_tborder" : "tborder", 2), FALSE);;

			SendDlgItemMessage(hwndDlg, IDC_BOTTOMTABADJUSTSPIN, UDM_SETRANGE, 0, MAKELONG(3, -3));
			SendDlgItemMessage(hwndDlg, IDC_BOTTOMTABADJUSTSPIN, UDM_SETPOS, 0, PluginConfig.tabConfig.m_bottomAdjust);
			SetDlgItemInt(hwndDlg, IDC_BOTTOMTABADJUST, PluginConfig.tabConfig.m_bottomAdjust, TRUE);

			SendDlgItemMessage(hwndDlg, IDC_TABWIDTHSPIN, UDM_SETRANGE, 0, MAKELONG(400, 50));
			SendDlgItemMessage(hwndDlg, IDC_TABWIDTHSPIN, UDM_SETPOS, 0, PluginConfig.tabConfig.m_fixedwidth);
			SetDlgItemInt(hwndDlg, IDC_TABWIDTH, PluginConfig.tabConfig.m_fixedwidth, TRUE);

			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTER, UDM_SETRANGE, 0, MAKELONG(50, 0));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERRIGHT, UDM_SETRANGE, 0, MAKELONG(50, 0));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERTOP, UDM_SETRANGE, 0, MAKELONG(40, 0));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERBOTTOM, UDM_SETRANGE, 0, MAKELONG(40, 0));

			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTER, UDM_SETPOS, 0, (int)M->GetByte(CSkin::m_skinEnabled ? "S_tborder_outer_left" : "tborder_outer_left", 2));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERRIGHT, UDM_SETPOS, 0, (int)M->GetByte(CSkin::m_skinEnabled ? "S_tborder_outer_right" : "tborder_outer_right", 2));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERTOP, UDM_SETPOS, 0, (int)M->GetByte(CSkin::m_skinEnabled ? "S_tborder_outer_top" : "tborder_outer_top", 2));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERBOTTOM, UDM_SETPOS, 0, (int)M->GetByte(CSkin::m_skinEnabled ? "S_tborder_outer_bottom" : "tborder_outer_bottom", 2));

			SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(10, 1));
			SendDlgItemMessage(hwndDlg, IDC_SPIN3, UDM_SETRANGE, 0, MAKELONG(10, 1));
			SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETPOS, 0, (LPARAM)M->GetByte("y-pad", 3));
			SendDlgItemMessage(hwndDlg, IDC_SPIN3, UDM_SETPOS, 0, (LPARAM)M->GetByte("x-pad", 4));
			SetDlgItemInt(hwndDlg, IDC_TABPADDING, (int)M->GetByte("y-pad", 3), FALSE);;
			SetDlgItemInt(hwndDlg, IDC_HTABPADDING, (int)M->GetByte("x-pad", 4), FALSE);;
			EnableWindow(GetDlgItem(hwndDlg, IDC_LABELUSEWINCOLORS), CSkin::m_skinEnabled ? FALSE : TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKGUSEWINCOLORS2), CSkin::m_skinEnabled ? FALSE : TRUE);

			SendDlgItemMessage(hwndDlg, IDC_AEROGLOW, CPM_SETCOLOUR, 0, (LPARAM)M->GetDword("aeroGlow", RGB(40, 40, 255)));
			return 0;
		}
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY: {
							int i = 0;
							COLORREF clr;
							BOOL translated;
							int  fixedWidth;

							struct ContainerWindowData *pContainer = pFirstContainer;

							DWORD dwFlags =	(IsDlgButtonChecked(hwndDlg, IDC_LABELUSEWINCOLORS) ? TCF_LABELUSEWINCOLORS : 0) |
											(IsDlgButtonChecked(hwndDlg, IDC_BKGUSEWINCOLORS2) ? TCF_BKGUSEWINCOLORS : 0);

							M->WriteDword(SRMSGMOD_T, "tabconfig", dwFlags);
							PluginConfig.m_TabAppearance = dwFlags;
							while (tabcolors[i].szKey != NULL) {
								clr = SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_GETCOLOUR, 0, 0);
								M->WriteDword(SRMSGMOD_T, CSkin::m_skinEnabled ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, clr);
								i++;
							}
							M->WriteByte(SRMSGMOD_T, "y-pad", (BYTE)(GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE)));
							M->WriteByte(SRMSGMOD_T, "x-pad", (BYTE)(GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE)));
							M->WriteByte(SRMSGMOD_T, "tborder", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDER, &translated, FALSE));
							M->WriteByte(SRMSGMOD_T, CSkin::m_skinEnabled ? "S_tborder_outer_left" : "tborder_outer_left", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTER, &translated, FALSE));
							M->WriteByte(SRMSGMOD_T, CSkin::m_skinEnabled ? "S_tborder_outer_right" : "tborder_outer_right", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERRIGHT, &translated, FALSE));
							M->WriteByte(SRMSGMOD_T, CSkin::m_skinEnabled ? "S_tborder_outer_top" : "tborder_outer_top", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERTOP, &translated, FALSE));
							M->WriteByte(SRMSGMOD_T, CSkin::m_skinEnabled ? "S_tborder_outer_bottom" : "tborder_outer_bottom", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERBOTTOM, &translated, FALSE));
							M->WriteDword(SRMSGMOD_T, "bottomadjust", GetDlgItemInt(hwndDlg, IDC_BOTTOMTABADJUST, &translated, TRUE));

							fixedWidth = GetDlgItemInt(hwndDlg, IDC_TABWIDTH, &translated, FALSE);
							fixedWidth = (fixedWidth < 60 ? 60 : fixedWidth);
							M->WriteDword(SRMSGMOD_T, "fixedwidth", fixedWidth);
							FreeTabConfig();
							if ((COLORREF)SendDlgItemMessage(hwndDlg, IDC_LIGHTSHADOW, CPM_GETCOLOUR, 0, 0) == RGB(255, 0, 255))
								DBDeleteContactSetting(NULL, SRMSGMOD_T, "tab_lightshadow");
							if ((COLORREF)SendDlgItemMessage(hwndDlg, IDC_DARKSHADOW, CPM_GETCOLOUR, 0, 0) == RGB(255, 0, 255))
								DBDeleteContactSetting(NULL, SRMSGMOD_T, "tab_darkshadow");

							FreeTabConfig();
							ReloadTabConfig();
							while (pContainer) {
								TabCtrl_SetPadding(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE), GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE));
								RedrawWindow(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
								pContainer = pContainer->pNextContainer;
							}
							M->WriteDword(SRMSGMOD_T, "aeroGlow", (DWORD)SendDlgItemMessage(hwndDlg, IDC_AEROGLOW, CPM_GETCOLOUR, 0, 0));
							M->WriteDword(SRMSGMOD_T, CSkin::m_skinEnabled ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, clr);
							Skin->setupAeroSkins();  // re-colorize
							return TRUE;
						}
					}
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_LABELUSEWINCOLORS:
				case IDC_BKGUSEWINCOLORS2: {
					int i = 0;
					BOOL iLabel = IsDlgButtonChecked(hwndDlg, IDC_LABELUSEWINCOLORS);
					BOOL iBkg = IsDlgButtonChecked(hwndDlg, IDC_BKGUSEWINCOLORS2);
					while (tabcolors[i].szKey != NULL) {
						if (i < 4)
							EnableWindow(GetDlgItem(hwndDlg, tabcolors[i].id), iLabel ? FALSE : TRUE);
						else
							EnableWindow(GetDlgItem(hwndDlg, tabcolors[i].id), iBkg ? FALSE : TRUE);
						i++;
					}
					break;
				}
				break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
	}
	return FALSE;
}

