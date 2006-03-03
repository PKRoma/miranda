/*
Miranda IM
Copyright (C) 2002 Robert Rainwater

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
#include "msgs.h"

// TODO:
// - Support for bitmap buttons (simple call to DrawIconEx())
extern HINSTANCE g_hInst;
extern MYGLOBALS myGlobals;
extern BOOL g_skinnedContainers;
extern StatusItems_t StatusItems[];

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
	char	cHot;
	int     flatBtn;
    int     dimmed;
	struct ContainerWindowData *pContainer;
} MButtonCtrl;

// External theme methods and properties
static HMODULE  themeAPIHandle = NULL; // handle to uxtheme.dll
static HANDLE   (WINAPI *MyOpenThemeData)(HWND,LPCWSTR);
static HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);
static BOOL     (WINAPI *MyIsThemeBackgroundPartiallyTransparent)(HANDLE,int,int);
static HRESULT  (WINAPI *MyDrawThemeParentBackground)(HWND,HDC,RECT *);
static HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *);
static HRESULT  (WINAPI *MyDrawThemeText)(HANDLE,HDC,int,int,LPCWSTR,int,DWORD,DWORD,const RECT *);
static HRESULT  (WINAPI *MyGetThemeBackgroundContentRect)(HANDLE, HDC, int, int, const RECT *, const RECT *);

static CRITICAL_SECTION csTips;
static HWND hwndToolTips = NULL;

int UnloadTSButtonModule(WPARAM wParam, LPARAM lParam) {
	DeleteCriticalSection(&csTips);
	return 0;
}

int LoadTSButtonModule(void) 
{
	WNDCLASSEXA wc;
	
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = "TSButtonClass";
	wc.lpfnWndProc    = TSButtonWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra     = sizeof(MButtonCtrl*);
	wc.hbrBackground  = 0;
	wc.style          = CS_GLOBALCLASS | CS_PARENTDC;
	RegisterClassExA(&wc);
	InitializeCriticalSection(&csTips);
	return 0;
}

// Used for our own cheap TrackMouseEvent
#define BUTTON_POLLID       100
#define BUTTON_POLLDELAY    50

#define MGPROC(x) GetProcAddress(themeAPIHandle,x)
static int ThemeSupport() {
	if (IsWinVerXPPlus()) {
		if (!themeAPIHandle) {
			themeAPIHandle = GetModuleHandleA("uxtheme");
			if (themeAPIHandle) {
				MyOpenThemeData = (HANDLE (WINAPI *)(HWND,LPCWSTR))MGPROC("OpenThemeData");
				MyCloseThemeData = (HRESULT (WINAPI *)(HANDLE))MGPROC("CloseThemeData");
				MyIsThemeBackgroundPartiallyTransparent = (BOOL (WINAPI *)(HANDLE,int,int))MGPROC("IsThemeBackgroundPartiallyTransparent");
				MyDrawThemeParentBackground = (HRESULT (WINAPI *)(HWND,HDC,RECT *))MGPROC("DrawThemeParentBackground");
				MyDrawThemeBackground = (HRESULT (WINAPI *)(HANDLE,HDC,int,int,const RECT *,const RECT *))MGPROC("DrawThemeBackground");
				MyDrawThemeText = (HRESULT (WINAPI *)(HANDLE,HDC,int,int,LPCWSTR,int,DWORD,DWORD,const RECT *))MGPROC("DrawThemeText");
				MyGetThemeBackgroundContentRect = (HRESULT (WINAPI *)(HANDLE, HDC, int, int, const RECT *, const RECT *))MGPROC("GetThemeBackgroundContentRect");
			}
		}
		// Make sure all of these methods are valid (i would hope either all or none work)
		if (MyOpenThemeData
				&&MyCloseThemeData
				&&MyIsThemeBackgroundPartiallyTransparent
				&&MyDrawThemeParentBackground
				&&MyDrawThemeBackground
				&&MyDrawThemeText) {
			return 1;
		}
	}
	return 0;
}

static void DestroyTheme(MButtonCtrl *ctl) {
	if (ThemeSupport()) {
		if (ctl->hThemeButton) {
			MyCloseThemeData(ctl->hThemeButton);
			ctl->hThemeButton = NULL;
		}
		if (ctl->hThemeToolbar) {
			MyCloseThemeData(ctl->hThemeToolbar);
			ctl->hThemeToolbar = NULL;
		}
	}
}

static void LoadTheme(MButtonCtrl *ctl) {
	if (ThemeSupport()) {
		DestroyTheme(ctl);
		ctl->hThemeButton = MyOpenThemeData(ctl->hwnd,L"BUTTON");
		ctl->hThemeToolbar = MyOpenThemeData(ctl->hwnd,L"TOOLBAR");
        ctl->bThemed = TRUE;
	}
}

static TBStateConvert2Flat(int state) {
	switch(state) {
		case PBS_NORMAL:    return TS_NORMAL;
		case PBS_HOT:       return TS_HOT;
		case PBS_PRESSED:   return TS_PRESSED;
		case PBS_DISABLED:  return TS_DISABLED;
		case PBS_DEFAULTED: return TS_NORMAL;
	}
	return TS_NORMAL;
}

static void PaintWorker(MButtonCtrl *ctl, HDC hdcPaint) {
	if (hdcPaint) {
		HDC hdcMem;
		HBITMAP hbmMem;
		HDC hOld;
		RECT rcClient, rcContent;
		HRGN clip = 0;

		GetClientRect(ctl->hwnd, &rcClient);
		CopyRect(&rcContent, &rcClient);

		hdcMem = CreateCompatibleDC(hdcPaint);
		hbmMem = CreateCompatibleBitmap(hdcPaint, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
		hOld = SelectObject(hdcMem, hbmMem);

		if (ctl->pushBtn && ctl->pbState) 
			ctl->stateId = PBS_PRESSED;

		if(ctl->arrow && (ctl->stateId == PBS_HOT || ctl->stateId == PBS_PRESSED)) {
			POINT pt;

			GetCursorPos(&pt);
			ScreenToClient(ctl->hwnd, &pt);

			if(pt.x >= rcClient.right - 12) {
				clip = CreateRectRgn(rcClient.right - 12, 0, rcClient.right, rcClient.bottom);
				/*
				CopyRect(&rcClip, &rcClient);
				rcClip.right = rcClient.right - 12;
				CopyRect(&rcInvClip, &rcClient);
				rcInvClip.left = rcClient.right - 11;
				invclip = CreateRectRgn(rcClient.right - 11, 0, rcClient.right, rcClient.bottom); */
			}/*
			else {
				CopyRect(&rcClip, &rcClient);
				rcClip.left = rcClient.right - 11;
				CopyRect(&rcInvClip, &rcClient);
				rcInvClip.right = rcClient.right - 12;
				invclip = CreateRectRgn(0, 0, rcClient.right - 12, rcClient.bottom);
				clip = CreateRectRgn(rcClient.right - 12, 0, rcClient.right, rcClient.bottom);
			}*/
		}
		if (ctl->flatBtn) {
			if(ctl->pContainer && ctl->pContainer->bSkinned) {
				StatusItems_t *item, *realItem = 0;
				if(ctl->bTitleButton)
					item = &StatusItems[ctl->stateId == PBS_NORMAL ? ID_EXTBKTITLEBUTTON : (ctl->stateId == PBS_HOT ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTONPRESSED)];
				else {
					item = &StatusItems[(ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) ? ID_EXTBKBUTTONSNPRESSED : (ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : ID_EXTBKBUTTONSPRESSED)];
					realItem = item;
					if(clip)
						item = &StatusItems[ID_EXTBKBUTTONSNPRESSED];
				}
				SkinDrawBG(ctl->hwnd, ctl->pContainer->hwnd,  ctl->pContainer, &rcClient, hdcMem);
				if(!item->IGNORED) {
					RECT rc1 = rcClient;
					rc1.left += item->MARGIN_LEFT; rc1.right -= item->MARGIN_RIGHT;
					rc1.top += item->MARGIN_TOP; rc1.bottom -= item->MARGIN_BOTTOM;
					DrawAlpha(hdcMem, &rc1, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
					if(clip && realItem) {
						SelectClipRgn(hdcMem, clip);
						DrawAlpha(hdcMem, &rc1, realItem->COLOR, realItem->ALPHA, realItem->COLOR2, realItem->COLOR2_TRANSPARENT,
								  realItem->GRADIENT, realItem->CORNER, realItem->RADIUS, realItem->imageItem);
					}
				}
				else 
					goto flat_themed;
			}
			else {
flat_themed:
				if (ctl->hThemeToolbar && ctl->bThemed) {
					RECT rc = rcClient;
					int state = IsWindowEnabled(ctl->hwnd)?(ctl->stateId==PBS_NORMAL&&ctl->defbutton?PBS_DEFAULTED:ctl->stateId):PBS_DISABLED;
					int realState = state;

					if(clip)
						state = PBS_NORMAL;

					if(rc.right < 20 || rc.bottom < 20)
						InflateRect(&rc, 2, 2);
					if (MyIsThemeBackgroundPartiallyTransparent(ctl->hThemeToolbar, TP_BUTTON, TBStateConvert2Flat(state))) {
						MyDrawThemeParentBackground(ctl->hwnd, hdcMem, &rc);
					}
					MyDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(state), &rc, &rc);
					if(clip) {
						SelectClipRgn(hdcMem, clip);
						MyDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(realState), &rc, &rc);
					}
				}
				else {
					HBRUSH hbr;
					RECT rc = rcClient;
					
					if (ctl->stateId==PBS_PRESSED||ctl->stateId==PBS_HOT) {
						hbr = GetSysColorBrush(COLOR_3DLIGHT);
						FillRect(hdcMem, &rc, hbr);
					}
					else {
						HDC dc;
						HWND hwndParent;

						hwndParent = GetParent(ctl->hwnd);
						dc=GetDC(hwndParent);
						hbr = (HBRUSH)SendMessage(hwndParent, WM_CTLCOLORDLG, (WPARAM)dc, (LPARAM)hwndParent);
						ReleaseDC(hwndParent,dc);
						if(hbr) {
							FillRect(hdcMem, &rc, hbr);
							DeleteObject(hbr);
						}
					}
					if (ctl->stateId==PBS_HOT||ctl->focus) {
						if (ctl->pbState)
							DrawEdge(hdcMem,&rc, EDGE_ETCHED,BF_RECT|BF_SOFT);
						else 
							DrawEdge(hdcMem,&rc, BDR_RAISEDOUTER,BF_RECT|BF_SOFT);
					}
					else if (ctl->stateId==PBS_PRESSED)
						DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER,BF_RECT|BF_SOFT);
				}
			}
		}
		else {
			if(ctl->pContainer && ctl->pContainer->bSkinned) {
				StatusItems_t *item, *realItem = 0;
				if(ctl->bTitleButton)
					item = &StatusItems[ctl->stateId == PBS_NORMAL ? ID_EXTBKTITLEBUTTON : (ctl->stateId == PBS_HOT ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTONPRESSED)];
				else {
					item = &StatusItems[(ctl->stateId == PBS_NORMAL || ctl->stateId == PBS_DISABLED) ? ID_EXTBKBUTTONSNPRESSED : (ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : ID_EXTBKBUTTONSPRESSED)];
					realItem = item;
					if(clip)
						item = &StatusItems[ID_EXTBKBUTTONSNPRESSED];
				}
				SkinDrawBG(ctl->hwnd, ctl->pContainer->hwnd,  ctl->pContainer, &rcClient, hdcMem);
				if(!item->IGNORED) {
					RECT rc1 = rcClient;
					rc1.left += item->MARGIN_LEFT; rc1.right -= item->MARGIN_RIGHT;
					rc1.top += item->MARGIN_TOP; rc1.bottom -= item->MARGIN_BOTTOM;
					DrawAlpha(hdcMem, &rc1, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
					if(clip && realItem) {
						SelectClipRgn(hdcMem, clip);
						DrawAlpha(hdcMem, &rc1, realItem->COLOR, realItem->ALPHA, realItem->COLOR2, realItem->COLOR2_TRANSPARENT,
								  realItem->GRADIENT, realItem->CORNER, realItem->RADIUS, realItem->imageItem);
					}
				}
				else 
					goto nonflat_themed;
			}
			else {
nonflat_themed:
				if (ctl->hThemeButton && ctl->bThemed) {
					int state = IsWindowEnabled(ctl->hwnd)?(ctl->stateId==PBS_NORMAL&&ctl->defbutton?PBS_DEFAULTED:ctl->stateId):PBS_DISABLED;
					int realState = state;

					if(clip)
						state = PBS_NORMAL;

					if (MyIsThemeBackgroundPartiallyTransparent(ctl->hThemeButton, BP_PUSHBUTTON, state)) {
						MyDrawThemeParentBackground(ctl->hwnd, hdcMem, &rcClient);
					}
					MyDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, state, &rcClient, &rcClient);

					if(clip) {
						SelectClipRgn(hdcMem, clip);
						MyDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, realState, &rcClient, &rcClient);
					}
					MyGetThemeBackgroundContentRect(ctl->hThemeToolbar, hdcMem, BP_PUSHBUTTON, PBS_NORMAL, &rcClient, &rcContent);
				}
				else {
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
		if(clip) {
			SelectClipRgn(hdcMem, 0);
			DeleteObject(clip);
		}
		if(ctl->arrow) {
			rcContent.top++;
			rcContent.bottom--;
			rcContent.left = rcClient.right - 12;
			rcContent.right = rcContent.left;

			DrawIconEx(hdcMem, rcClient.right - 13, (rcClient.bottom-rcClient.top)/2 - (myGlobals.m_smcyicon / 2),
					   myGlobals.g_buttonBarIcons[16], 16, 16, 0, 0, DI_NORMAL);
			if(!ctl->flatBtn || (ctl->pContainer && ctl->pContainer->bSkinned))
				DrawEdge(hdcMem, &rcContent, EDGE_BUMP, BF_LEFT);
		}

		// If we have an icon or a bitmap, ignore text and only draw the image on the button
		if (ctl->hIcon || ctl->hIconPrivate) {
			int ix = (rcClient.right-rcClient.left)/2 - (myGlobals.m_smcxicon / 2);
			int iy = (rcClient.bottom-rcClient.top)/2 - (myGlobals.m_smcyicon / 2);
            HICON hIconNew = ctl->hIconPrivate != 0 ? ctl->hIconPrivate : ctl->hIcon;

			if(ctl->arrow)
				ix -= 4;

            if(ctl->dimmed && myGlobals.m_IdleDetect) {
				ImageList_ReplaceIcon(myGlobals.g_hImageList, 0, hIconNew);
				ImageList_DrawEx(myGlobals.g_hImageList, 0, hdcMem, ix, iy, 0, 0, CLR_NONE, CLR_NONE, ILD_SELECTED);
            }
            else
                DrawState(hdcMem,NULL,NULL,(LPARAM)hIconNew,0,ix,iy,myGlobals.m_smcxicon,myGlobals.m_smcyicon,ctl->stateId != PBS_DISABLED ? DST_ICON | DSS_NORMAL : DST_ICON | DSS_DISABLED);
		}
		else if (ctl->hBitmap) {
			BITMAP bminfo;
			int ix,iy;

			GetObject(ctl->hBitmap, sizeof(bminfo), &bminfo);
			ix = (rcClient.right-rcClient.left)/2 - (bminfo.bmWidth/2);
			iy = (rcClient.bottom-rcClient.top)/2 - (bminfo.bmHeight/2);

			if(ctl->arrow)
				ix -= 9;

			if (ctl->stateId == PBS_PRESSED) {
				ix++;
				iy++;
			}
			DrawState(hdcMem,NULL,NULL,(LPARAM)ctl->hBitmap,0,ix,iy,bminfo.bmWidth,bminfo.bmHeight,IsWindowEnabled(ctl->hwnd)?DST_BITMAP:DST_BITMAP|DSS_DISABLED);
		}
		else if (GetWindowTextLengthA(ctl->hwnd)) {
			// Draw the text and optinally the arrow
			char szText[MAX_PATH * sizeof(TCHAR)];
			SIZE sz;
			RECT rcText;
			HFONT hOldFont;

			CopyRect(&rcText, &rcClient);
			GetWindowTextA(ctl->hwnd, szText, MAX_PATH - 1);
			SetBkMode(hdcMem, TRANSPARENT);
			hOldFont = SelectObject(hdcMem, ctl->hFont);
			// XP w/themes doesn't used the glossy disabled text.  Is it always using COLOR_GRAYTEXT?  Seems so.
			SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd)||!ctl->hThemeButton?GetSysColor(COLOR_BTNTEXT):GetSysColor(COLOR_GRAYTEXT));
			GetTextExtentPoint32A(hdcMem, szText, lstrlenA(szText), &sz);
			if (ctl->cHot) {
				SIZE szHot;
				
				GetTextExtentPoint32A(hdcMem, "&", 1, &szHot);
				sz.cx -= szHot.cx;
			}
			if (ctl->arrow) {
				DrawState(hdcMem,NULL,NULL,(LPARAM)ctl->arrow,0,rcClient.right-rcClient.left-5-myGlobals.m_smcxicon+(!ctl->hThemeButton&&ctl->stateId==PBS_PRESSED?1:0),(rcClient.bottom-rcClient.top)/2-myGlobals.m_smcyicon/2+(!ctl->hThemeButton&&ctl->stateId==PBS_PRESSED?1:0),myGlobals.m_smcxicon,myGlobals.m_smcyicon,IsWindowEnabled(ctl->hwnd)?DST_ICON:DST_ICON|DSS_DISABLED);
			}
			SelectObject(hdcMem, ctl->hFont);
			DrawStateA(hdcMem,NULL,NULL,(LPARAM)szText,0,(rcText.right-rcText.left-sz.cx)/2+(!ctl->hThemeButton&&ctl->stateId==PBS_PRESSED?1:0),ctl->hThemeButton?(rcText.bottom-rcText.top-sz.cy)/2:(rcText.bottom-rcText.top-sz.cy)/2-(ctl->stateId==PBS_PRESSED?0:1),sz.cx,sz.cy,IsWindowEnabled(ctl->hwnd)||ctl->hThemeButton?DST_PREFIXTEXT|DSS_NORMAL:DST_PREFIXTEXT|DSS_DISABLED);
			SelectObject(hdcMem, hOldFont);
		}
		BitBlt(hdcPaint, 0, 0, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, hdcMem, 0, 0, SRCCOPY);
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

	}
}

static LRESULT CALLBACK TSButtonWndProc(HWND hwndDlg, UINT msg,  WPARAM wParam, LPARAM lParam) {
	MButtonCtrl* bct =  (MButtonCtrl *)GetWindowLong(hwndDlg, 0);
	switch(msg) {
		case WM_NCCREATE:
		{
			SetWindowLong(hwndDlg, GWL_STYLE, GetWindowLong(hwndDlg, GWL_STYLE)|BS_OWNERDRAW);
			bct = malloc(sizeof(MButtonCtrl));
			if (bct==NULL) return FALSE;
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
            LoadTheme(bct);
			SetWindowLong(hwndDlg, 0, (LONG)bct);
			if (((CREATESTRUCTA *)lParam)->lpszName) SetWindowTextA(hwndDlg, ((CREATESTRUCTA *)lParam)->lpszName);
			return TRUE;
		}
		case WM_DESTROY:
		{
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
					if (SendMessage(hwndToolTips, TTM_GETTOOLCOUNT, 0, (LPARAM)&ti)==0) {
						DestroyWindow(hwndToolTips);
						hwndToolTips = NULL;
					}
				}
                if(bct->hIconPrivate)
                    DestroyIcon(bct->hIconPrivate);
				LeaveCriticalSection(&csTips);
                DestroyTheme(bct);
				free(bct);
			}
			SetWindowLong(hwndDlg,0,(LONG)NULL);
			break;	// DONT! fall thru
		}
		case WM_SETTEXT:
		{
			bct->cHot = 0;
			if ((char*)lParam) {
				char *tmp = (char*)lParam;
				while (*tmp) {
					if (*tmp=='&' && *(tmp+1)) {
						bct->cHot = tolower(*(tmp+1));
						break;
					}
					tmp++;
				}
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		}
		case WM_KEYUP:
			if (bct->stateId!=PBS_DISABLED && wParam == VK_SPACE && !(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
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
			if (bct->stateId!=PBS_DISABLED && bct->cHot && bct->cHot == tolower((int)wParam)) {
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
            if(bct->bThemed)
                LoadTheme(bct);
			InvalidateRect(bct->hwnd, NULL, TRUE); // repaint it
			break;
		}
		case WM_SETFONT: // remember the font so we can use it later
		{
			bct->hFont = (HFONT)wParam; // maybe we should redraw?
			break;
		}
		case WM_NCPAINT:
		case WM_PAINT:
		{
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

                if(bct->hIconPrivate)
                    DestroyIcon(bct->hIconPrivate);
                
                GetIconInfo((HICON)lParam, &ii);
                GetObject(ii.hbmColor, sizeof(bm), &bm);
                if(bm.bmWidth != myGlobals.m_smcxicon || bm.bmHeight != myGlobals.m_smcyicon) {
                    HIMAGELIST hImageList;
                    hImageList = ImageList_Create(myGlobals.m_smcxicon,myGlobals.m_smcyicon, IsWinVerXPPlus()? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
                    ImageList_AddIcon(hImageList, (HICON)lParam);
                    bct->hIconPrivate = ImageList_GetIcon(hImageList, 0, ILD_NORMAL);
                    ImageList_RemoveAll(hImageList);
                    ImageList_Destroy(hImageList);
                    bct->hIcon = 0;
                }
                else {
                    bct->hIcon = (HICON)lParam;
                    bct->hIconPrivate = 0;
                }
                
                DeleteObject(ii.hbmMask);
                DeleteObject(ii.hbmColor);
				bct->hBitmap = NULL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			else if (wParam == IMAGE_BITMAP) {
				bct->hBitmap = (HBITMAP)lParam;
                if(bct->hIconPrivate) 
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
			}
			else if (wParam == BST_UNCHECKED) {
				bct->pbState = 0;
                bct->stateId = PBS_NORMAL;
			}
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BM_GETCHECK:
			if (bct->pushBtn) {
				return bct->pbState?BST_CHECKED:BST_UNCHECKED;
			}
			return 0;
		case BUTTONSETARROW: // turn arrow on/off
			bct->arrow = (HICON)wParam;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		case BUTTONSETDEFAULT:
			bct->defbutton = wParam?1:0;
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

		case BUTTONADDTOOLTIP:
		{
			TOOLINFOA ti;

			if (!(char*)wParam) break;
            EnterCriticalSection(&csTips);
			if (!hwndToolTips) {
				hwndToolTips = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
			}
			ZeroMemory(&ti, sizeof(ti));
			ti.cbSize = sizeof(ti);
			ti.uFlags = TTF_IDISHWND;
			ti.hwnd = bct->hwnd;
			ti.uId = (UINT)bct->hwnd;
			if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM)&ti)) {
				SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
			}
			ti.uFlags = TTF_IDISHWND|TTF_SUBCLASS;
			ti.uId = (UINT)bct->hwnd;
			ti.lpszText=(char*)wParam;
			SendMessageA(hwndToolTips,TTM_ADDTOOLA,0,(LPARAM)&ti);
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
		case WM_ENABLE: // windows tells us to enable/disable
		{
			bct->stateId = wParam?PBS_NORMAL:PBS_DISABLED;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			break;
		}
		case WM_MOUSELEAVE: // faked by the WM_TIMER
		{
			if (bct->stateId!=PBS_DISABLED) { // don't change states if disabled
				bct->stateId = PBS_NORMAL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			if (bct->stateId!=PBS_DISABLED) { // don't change states if disabled
				bct->stateId = PBS_PRESSED;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			RECT rc;

			if (bct->pushBtn) {
				if (bct->pbState) bct->pbState = 0;
				else bct->pbState = 1;
			}
			if (bct->stateId!=PBS_DISABLED) { // don't change states if disabled
				if (msg==WM_LBUTTONUP) bct->stateId = PBS_HOT;
				else bct->stateId = PBS_NORMAL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			if(bct->arrow) {
				GetClientRect(bct->hwnd, &rc);
				if(LOWORD(lParam) > rc.right - 10) {
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
			} else if(bct->arrow && bct->stateId == PBS_HOT) {
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
			// Call timer, used to start cheesy TrackMouseEvent faker
			SetTimer(hwndDlg,BUTTON_POLLID,BUTTON_POLLDELAY,NULL);
			break;
		case WM_TIMER: // use a timer to check if they have did a mouseout
		{
            if (wParam==BUTTON_POLLID) {
			    RECT rc;
			    POINT pt;
			    GetWindowRect(hwndDlg,&rc);
			    GetCursorPos(&pt);
			    if(!PtInRect(&rc,pt)) { // mouse must be gone, trigger mouse leave
				    PostMessage(hwndDlg,WM_MOUSELEAVE,0,0L);
				    KillTimer(hwndDlg,BUTTON_POLLID);
			    }
            }
			break;
		}
		case WM_ERASEBKGND:
			return 1;
	}
	return DefWindowProc(hwndDlg, msg, wParam, lParam);
}
