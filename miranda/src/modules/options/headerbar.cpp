/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2007 Artem Shpynov
Copyright 2000-2007 Miranda ICQ/IM project,

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
#include "m_iconheader.h"

//#include <vsstyle.h>
//#include <uxtheme.h>
//#include <dwmapi.h>

extern HINSTANCE hMirandaInst;


enum WINDOWPARTS {
	WP_CAPTION = 1,
	WP_SMALLCAPTION = 2,
	WP_MINCAPTION = 3,
	WP_SMALLMINCAPTION = 4,
	WP_MAXCAPTION = 5,
	WP_SMALLMAXCAPTION = 6,
	WP_FRAMELEFT = 7,
	WP_FRAMERIGHT = 8,
	WP_FRAMEBOTTOM = 9,
	WP_SMALLFRAMELEFT = 10,
	WP_SMALLFRAMERIGHT = 11,
	WP_SMALLFRAMEBOTTOM = 12,
	WP_SYSBUTTON = 13,
	WP_MDISYSBUTTON = 14,
	WP_MINBUTTON = 15,
	WP_MDIMINBUTTON = 16,
	WP_MAXBUTTON = 17,
	WP_CLOSEBUTTON = 18,
	WP_SMALLCLOSEBUTTON = 19,
	WP_MDICLOSEBUTTON = 20,
	WP_RESTOREBUTTON = 21,
	WP_MDIRESTOREBUTTON = 22,
	WP_HELPBUTTON = 23,
	WP_MDIHELPBUTTON = 24,
	WP_HORZSCROLL = 25,
	WP_HORZTHUMB = 26,
	WP_VERTSCROLL = 27,
	WP_VERTTHUMB = 28,
	WP_DIALOG = 29,
	WP_CAPTIONSIZINGTEMPLATE = 30,
	WP_SMALLCAPTIONSIZINGTEMPLATE = 31,
	WP_FRAMELEFTSIZINGTEMPLATE = 32,
	WP_SMALLFRAMELEFTSIZINGTEMPLATE = 33,
	WP_FRAMERIGHTSIZINGTEMPLATE = 34,
	WP_SMALLFRAMERIGHTSIZINGTEMPLATE = 35,
	WP_FRAMEBOTTOMSIZINGTEMPLATE = 36,
	WP_SMALLFRAMEBOTTOMSIZINGTEMPLATE = 37,
	WP_FRAME = 38,
};

enum CAPTIONSTATES {
	CS_ACTIVE = 1,
	CS_INACTIVE = 2,
	CS_DISABLED = 3,
};

enum LISTVIEWPARTS {
	LVP_LISTITEM = 1,
	LVP_LISTGROUP = 2,
	LVP_LISTDETAIL = 3,
	LVP_LISTSORTEDDETAIL = 4,
	LVP_EMPTYTEXT = 5,
	LVP_GROUPHEADER = 6,
	LVP_GROUPHEADERLINE = 7,
	LVP_EXPANDBUTTON = 8,
	LVP_COLLAPSEBUTTON = 9,
	LVP_COLUMNDETAIL = 10,
};

enum LISTITEMSTATES {
	LISS_NORMAL = 1,
	LISS_HOT = 2,
	LISS_SELECTED = 3,
	LISS_DISABLED = 4,
	LISS_SELECTEDNOTFOCUS = 5,
	LISS_HOTSELECTED = 6,
};

enum COMMUNICATIONSPARTS {
	CSST_TAB = 1,
};

enum TABSTATES {
	CSTB_NORMAL = 1,
	CSTB_HOT = 2,
	CSTB_SELECTED = 3,
};

enum TREEVIEWPARTS {
	TVP_TREEITEM = 1,
	TVP_GLYPH = 2,
	TVP_BRANCH = 3,
	TVP_HOTGLYPH = 4,
};

enum TREEITEMSTATES {
	TREIS_NORMAL = 1,
	TREIS_HOT = 2,
	TREIS_SELECTED = 3,
	TREIS_DISABLED = 4,
	TREIS_SELECTEDNOTFOCUS = 5,
	TREIS_HOTSELECTED = 6,
};

static BOOL IsAeroMode()
{
	BOOL result;
	return dwmIsCompositionEnabled && (dwmIsCompositionEnabled(&result) == S_OK) && result;
}

static BOOL IsVSMode()
{
	return isThemeActive && IsWinVerVistaPlus() && isThemeActive();
}

////////////////////////////////////////////////////////////////////////////////////
// Internals

static LRESULT CALLBACK MHeaderbarWndProc(HWND hwnd, UINT  msg, WPARAM wParam, LPARAM lParam);

// structure is used for storing list of tab info
struct MHeaderbarCtrl
{
	__inline void* operator new( size_t size )
	{	return calloc( 1, size );
	}
	__inline void operator delete( void* p )
	{	free( p );
	}

	MHeaderbarCtrl() {}
	~MHeaderbarCtrl() { free( controlsToRedraw ); }

	HWND		hwnd;

	// UI info
	RECT		rc;
	int			width, height;
    HICON       hIcon;
    
	// control colors
	RGBQUAD		rgbBkgTop, rgbBkgBottom;
	COLORREF	clText;

	int			nControlsToRedraw;
	HWND		*controlsToRedraw;
};

int UnloadHeaderbarModule(WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

int LoadHeaderbarModule()
{
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = _T("MHeaderbarCtrl"); //MIRANDAHEADERBARCLASS;
	wc.lpfnWndProc    = MHeaderbarWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra     = sizeof(MHeaderbarCtrl*);
	wc.hbrBackground  = 0; //GetStockObject(WHITE_BRUSH);
	wc.style          = CS_GLOBALCLASS|CS_SAVEBITS;
	RegisterClassEx(&wc);	
	HookEvent(ME_SYSTEM_SHUTDOWN, UnloadHeaderbarModule);
	return 0;
}

static void MHeaderbar_SetupColors(MHeaderbarCtrl *dat)
{
	COLORREF cl;

	cl = GetSysColor(COLOR_WINDOW);
	dat->rgbBkgBottom.rgbRed	= (dat->rgbBkgTop.rgbRed	= GetRValue(cl)) * .95;
	dat->rgbBkgBottom.rgbGreen	= (dat->rgbBkgTop.rgbGreen	= GetGValue(cl)) * .95;
	dat->rgbBkgBottom.rgbBlue	= (dat->rgbBkgTop.rgbBlue	= GetBValue(cl)) * .95;

	dat->clText			= GetSysColor(COLOR_WINDOWTEXT);
}

static void MHeaderbar_FillRect(HDC hdc, int x, int y, int width, int height, COLORREF cl)
{
	int oldMode			= SetBkMode(hdc, OPAQUE);
	COLORREF oldColor	= SetBkColor(hdc, cl);

	RECT rc; SetRect(&rc, x, y, x+width, y+height);
	ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rc, "", 0, 0);

	SetBkMode(hdc, oldMode);
	SetBkColor(hdc, oldColor);
}

static void MHeaderbar_DrawGradient(HDC hdc, int x, int y, int width, int height, RGBQUAD *rgb0, RGBQUAD *rgb1)
{
	int i;

	int oldMode			= SetBkMode(hdc, OPAQUE);
	COLORREF oldColor	= SetBkColor(hdc, 0);

	RECT rc; SetRect(&rc, x, 0, x+width, 0);
	for (i=y+height; --i >= y; )
	{
		COLORREF color = RGB(
			((height-i-1)*rgb0->rgbRed   + i*rgb1->rgbRed)   / height,
			((height-i-1)*rgb0->rgbGreen + i*rgb1->rgbGreen) / height,
			((height-i-1)*rgb0->rgbBlue  + i*rgb1->rgbBlue)  / height);
		rc.top = rc.bottom = i;
		++rc.bottom;
		SetBkColor(hdc, color);
		ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rc, "", 0, 0);
	}

	SetBkMode(hdc, oldMode);
	SetBkColor(hdc, oldColor);
}

static LRESULT MHeaderbar_OnPaint(HWND hwndDlg, MHeaderbarCtrl *mit, UINT  msg, WPARAM wParam, LPARAM lParam)
{
	int iTopSpace = IsAeroMode() ? 0 : 3;
	PAINTSTRUCT ps;
	HBITMAP hBmp, hOldBmp;

	int titleLength = GetWindowTextLength(hwndDlg) + 1;
	TCHAR *szTitle = (TCHAR *)mir_alloc(sizeof(TCHAR) * titleLength);
	GetWindowText(hwndDlg, szTitle, titleLength);

	TCHAR *szSubTitle = _tcschr(szTitle, _T('\n'));
	if (szSubTitle) *szSubTitle++ = 0;

	HDC hdc=BeginPaint(hwndDlg,&ps);
	HDC tempDC=CreateCompatibleDC(hdc);

	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = mit->width;
	bmi.bmiHeader.biHeight = -mit->height; // we need this for DrawThemeTextEx
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hBmp = CreateDIBSection(tempDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);

	hOldBmp=(HBITMAP)SelectObject(tempDC,hBmp);

	if (IsAeroMode()) {
		RECT temprc;
		temprc.left=0;
		temprc.right=mit->width;
		temprc.top=0;
		temprc.bottom=mit->width;
		FillRect(tempDC, &temprc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	} 
	else {
		if (IsVSMode())
			MHeaderbar_FillRect(tempDC, 0, 0, mit->width, mit->height, GetSysColor(COLOR_WINDOW));
		else
			MHeaderbar_DrawGradient(tempDC, 0, 0, mit->width, mit->height, &mit->rgbBkgTop, &mit->rgbBkgBottom);

		MHeaderbar_FillRect(tempDC, 0, mit->height-2, mit->width, 1, GetSysColor(COLOR_BTNSHADOW));
		MHeaderbar_FillRect(tempDC, 0, mit->height-1, mit->width, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
	}

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SetBkMode(tempDC, TRANSPARENT);
	SetTextColor(tempDC, mit->clText);

	LOGFONT lf;
	GetObject(hFont, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	HFONT hFntBold = CreateFontIndirect(&lf);
    
    if (mit->hIcon)
        DrawIcon(tempDC, 10, iTopSpace, mit->hIcon);
    else DrawIcon(tempDC, 10, iTopSpace, LoadIcon(hMirandaInst, MAKEINTRESOURCE(IDI_DETAILSLOGO)));

	RECT textRect;
	textRect.left=50;
	textRect.right=mit->width;
	textRect.top=2 + iTopSpace;
	textRect.bottom=GetSystemMetrics(SM_CYICON)-2 + iTopSpace;

	if (IsAeroMode()) {
		DTTOPTS dto = {0};
		dto.dwSize = sizeof(dto);
		dto.dwFlags = DTT_COMPOSITED|DTT_GLOWSIZE;
		dto.iGlowSize = 10;

		HANDLE hTheme = openThemeData(hwndDlg, _T("Window"));
		textRect.left=50;
		SelectObject(tempDC, hFntBold);
		drawThemeTextEx(hTheme, tempDC, WP_CAPTION, CS_ACTIVE, szTitle, -1, DT_TOP|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_END_ELLIPSIS, &textRect, &dto);

		if (szSubTitle) {
			textRect.left=66;
			SelectObject(tempDC, hFont);
			drawThemeTextEx(hTheme, tempDC, WP_CAPTION, CS_ACTIVE, szSubTitle, -1, DT_BOTTOM|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_END_ELLIPSIS, &textRect, &dto);
		}
		closeThemeData(hTheme);
	}
	else {
		textRect.left=50;
		SelectObject(tempDC, hFntBold);
		DrawText(tempDC, szTitle, -1, &textRect, DT_TOP|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_END_ELLIPSIS);

		if (szSubTitle) {
			textRect.left=66;
			SelectObject(tempDC, hFont);
			DrawText(tempDC, szSubTitle, -1, &textRect, DT_BOTTOM|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_END_ELLIPSIS);
	}	}

	DeleteObject(hFntBold);

	mir_free(szTitle);

	//Copy to output
	if (mit->nControlsToRedraw)
	{
		RECT temprc;
		temprc.left=0;
		temprc.right=mit->width;
		temprc.top=0;
		temprc.bottom=mit->width;
		HRGN hRgn = CreateRectRgnIndirect(&temprc);

		for (int i = 0; i < mit->nControlsToRedraw; ++i)
		{
			GetWindowRect(mit->controlsToRedraw[i], &temprc);
			MapWindowPoints(NULL, hwndDlg, (LPPOINT)&temprc, 2);
			HRGN hRgnTmp = CreateRectRgnIndirect(&temprc);
			CombineRgn(hRgn, hRgn, hRgnTmp, RGN_DIFF);
			DeleteObject(hRgnTmp);
		}
		SelectClipRgn(hdc,hRgn);
		DeleteObject(hRgn);
	}

	BitBlt(hdc,mit->rc.left,mit->rc.top,mit->width,mit->height,tempDC,0,0,SRCCOPY);

	SelectClipRgn(hdc,NULL);

	SelectObject(tempDC,hOldBmp);
	DeleteObject(hBmp);
	DeleteDC(tempDC);

	EndPaint(hwndDlg,&ps);

	return TRUE;
}

static LRESULT CALLBACK MHeaderbarWndProc(HWND hwndDlg, UINT  msg, WPARAM wParam, LPARAM lParam)
{
	MHeaderbarCtrl* itc =  (MHeaderbarCtrl *)GetWindowLong(hwndDlg, 0);
	switch(msg) {
	case WM_NCCREATE:
		itc = new MHeaderbarCtrl; //(MHeaderbarCtrl*)mir_alloc(sizeof(MHeaderbarCtrl));
		if (itc==NULL)
			return FALSE;

		SetWindowLong(hwndDlg, 0, (LONG)itc);
		MHeaderbar_SetupColors(itc);

		if (IsAeroMode()) {
			RECT rc; GetWindowRect(hwndDlg, &rc);
			MARGINS margins = {0,0,rc.bottom-rc.top,0};
			dwmExtendFrameIntoClientArea(GetParent(hwndDlg), &margins);

			WTA_OPTIONS opts;
			opts.dwFlags = opts.dwMask = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON;
			setWindowThemeAttribute(GetParent(hwndDlg), WTA_NONCLIENT, &opts, sizeof(opts));
		}

		{	HWND hParent = GetParent(hwndDlg);
			RECT rcWnd; GetWindowRect(hwndDlg, &rcWnd);
			itc->controlsToRedraw = 0;
			itc->nControlsToRedraw = 0;
			for (HWND hChild = FindWindowEx(hParent, NULL, NULL, NULL); hChild; hChild = FindWindowEx(hParent, hChild, NULL, NULL))
			{
				if (hChild != hwndDlg)
				{
					RECT rcChild; GetWindowRect(hChild, &rcChild);
					RECT rc;
					IntersectRect(&rc, &rcChild, &rcWnd);
					if (!IsRectEmpty(&rc))
					{
						++itc->nControlsToRedraw;
						itc->controlsToRedraw = (HWND *)realloc(itc->controlsToRedraw, sizeof(HWND) * itc->nControlsToRedraw);
						itc->controlsToRedraw[itc->nControlsToRedraw - 1] = hChild;
					}
				}
			}
		}

		break;

	case WM_SIZE:
		GetClientRect(hwndDlg,&itc->rc);
		itc->width=itc->rc.right-itc->rc.left;
		itc->height=itc->rc.bottom-itc->rc.top;
		return TRUE;

	case WM_THEMECHANGED:
	case WM_STYLECHANGED:
		MHeaderbar_SetupColors(itc);
		return TRUE;

	case WM_LBUTTONDOWN:
		SendMessage(GetParent(hwndDlg), WM_SYSCOMMAND, 0xF012, 0);
		return 0;
    
    case WM_SETICON:
		itc->hIcon = (HICON)lParam;
        InvalidateRect(hwndDlg, NULL, FALSE);
		break;
        
	case WM_ERASEBKGND:
		return 1;

	case WM_NCPAINT:
		InvalidateRect(hwndDlg, NULL, FALSE);
		break;

	case WM_PAINT:
		MHeaderbar_OnPaint(hwndDlg, itc, msg, wParam, lParam);
		break;

	case WM_DESTROY:
		delete itc;
		return TRUE;
	}
	return DefWindowProc(hwndDlg, msg, wParam, lParam);
}
