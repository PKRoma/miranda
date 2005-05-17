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
---------------------------------------------------------------------------

custom tab control for tabSRMM. Allows for configuartion of colors and backgrounds
for different tab states (active,  unread etc..)

no visual style support yet - will be added later.

*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"

extern MYGLOBALS myGlobals;
extern WNDPROC OldTabControlProc;

/*
 * visual styles support (XP+)
 * returns 0 on failure
 */

HMODULE hUxTheme = 0;

// function pointers

BOOL (PASCAL* pfnIsThemeActive)() = 0;
UINT (PASCAL* pfnOpenThemeData)(HWND hwnd, LPCWSTR pszClassList) = 0;
UINT (PASCAL* pfnDrawThemeBackground)(UINT htheme, HDC hdc, int iPartID, int iStateID, RECT* prcBx, RECT* prcClip);

int InitVSApi()
{
    if((hUxTheme = LoadLibraryA("uxtheme.dll")) == 0)
        return FALSE;

    pfnIsThemeActive = GetProcAddress(hUxTheme, "IsThemeActive");
    pfnOpenThemeData = GetProcAddress(hUxTheme, "OpenThemeData");
    pfnDrawThemeBackground = GetProcAddress(hUxTheme, "DrawThemeBackground");

    if(pfnIsThemeActive != 0 && pfnOpenThemeData != 0 && pfnDrawThemeBackground != 0) {
        return 1;
    }
    return 0;
}

int FreeVSApi()
{
    if(hUxTheme != 0)
        FreeLibrary(hUxTheme);
}
void RectScreenToClient(HWND hwnd, RECT *rc)
{
    POINT p1, p2;

    p1.x = rc->left;
    p1.y = rc->top;
    p2.x = rc->right;
    p2.y = rc->bottom;
    ScreenToClient(hwnd, &p1);
    ScreenToClient(hwnd, &p2);
    rc->left = p1.x;
    rc->top = p1.y;
    rc->right = p2.x;
    rc->bottom = p2.y;
}

	/*
     * tabctrl helper function
	 * Finds leftmost down item.
     */

UINT FindLeftDownItem(HWND hwnd)
{
	RECT rctLeft = {100000,0,0,0}, rctCur;
	int nCount = TabCtrl_GetItemCount(hwnd) - 1;
	UINT nItem=0;
    int i;
    
	for(i = 0;i < nCount;i++) {
        TabCtrl_GetItemRect(hwnd, i, &rctCur);
		if(rctCur.left > 0 && rctCur.left <= rctLeft.left) {
			if(rctCur.bottom > rctLeft.bottom) {
				rctLeft=rctCur;
				nItem=i;	
			}
		}
	}
	return nItem;
}

struct myTabCtrl {
    HPEN m_hPenShadow, m_hPenItemShadow, m_hPenLight;
    RECT m_rectUpDn;
    HWND hwnd, m_hwndSpin;
    DWORD dwStyle;
    DWORD cx, cy;
    HFONT m_hMenuFont;
};

#define HINT_ACTIVATE_RIGHT_SIDE 1
#define HINT_ACTIVE_ITEM 2
#define FLOAT_ITEM_HEIGHT_SHIFT 2
#define ACTIVE_ITEM_HEIGHT_SHIFT 2
#define SHIFT_FROM_CUT_TO_SPIN 4
#define HINT_CUT 8
#define HINT_TRANSPARENT 16

static WNDPROC OldSpinCtrlProc;

BOOL CALLBACK SpinCtrlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rcItem = ps.rcPaint;
            rcItem.top++;
            rcItem.left++;
            rcItem.bottom--;
            rcItem.right--;
            FillRect(dc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
            DrawFrameControl(dc, &rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_MONO);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 0;
    }
    return CallWindowProc(OldSpinCtrlProc, hwnd, msg, wParam, lParam); 
}

void DrawItem(struct myTabCtrl *tabdat, HDC dc, RECT *rcItem, int nHint, int nItem)
{
    TCITEM item = {0};
    struct MessageWindowData *dat = 0;

    item.mask = TCIF_PARAM;
    TabCtrl_GetItem(tabdat->hwnd, nItem, &item);
    
    /*
     * get the message window data for the session to which this tab item belongs
     */
    
    dat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);
    
    if(dat) {
        HICON hIcon;
        HBRUSH bg;
        DWORD dwStyle = tabdat->dwStyle;
        BOOL bFill = ((dwStyle & TCS_BOTTOM) || (dwStyle & TCS_BUTTONS)) && !(nHint & HINT_TRANSPARENT);
        int oldMode = 0;
        rcItem->left++;
        rcItem->right--;
        rcItem->top++;
        rcItem->bottom--;
        
        if(nHint & HINT_ACTIVE_ITEM) {
            SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            if(nHint & HINT_TRANSPARENT)
                oldMode = SetBkMode(dc, TRANSPARENT);
            else {
                bg = GetSysColorBrush(COLOR_3DSHADOW);
                if(bFill)
                    SetBkColor(dc, GetSysColor(COLOR_3DSHADOW));
            }
        }
        else {
            SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
            if(nHint & HINT_TRANSPARENT)
                oldMode = SetBkMode(dc, TRANSPARENT);
            else {
                bg = GetSysColorBrush(COLOR_3DFACE);
                if(bFill)
                    SetBkColor(dc, GetSysColor(COLOR_3DFACE));
            }
        }

        if(bFill)
            FillRect(dc, rcItem, bg);
        rcItem->left++;
        rcItem->right--;
        rcItem->top++;
        rcItem->bottom--;
        
        if(dat->dwFlags & MWF_ERRORSTATE)
            hIcon = myGlobals.g_iconErr;
        else if(dat->mayFlashTab)
            hIcon = dat->iFlashIcon;
        else
            hIcon = dat->hTabIcon;

        if(dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0)) {
            DrawState(dc, NULL, NULL, (LPARAM) hIcon, 0,
                      //(dis->rcItem.right + dis->rcItem.left - cx) / 2,
                      rcItem->left,
                      (rcItem->bottom + rcItem->top - tabdat->cy) / 2,
                      tabdat->cx, tabdat->cy,
                      DST_ICON | DSS_NORMAL);
        }
        rcItem->left += (tabdat->cx + 2);
        SelectObject(dc, tabdat->m_hMenuFont);
        DrawText(dc, dat->newtitle, _tcslen(dat->newtitle), rcItem, DT_SINGLELINE | DT_VCENTER);
        if(oldMode)
            SetBkMode(dc, oldMode);
    }
}

void DrawItemRect(struct myTabCtrl *tabdat, HDC dc, RECT *rcItem, int nHint)
{
    POINT pt;
    
    rcItem->bottom -= 1;
	if(rcItem->left > 0) {
		BOOL bDoNotCut = TRUE;

        if((tabdat->dwStyle & TCS_BUTTONS) || (tabdat->dwStyle & TCS_BOTTOM)) {
            // draw frame controls for button or bottom tabs
            if(tabdat->dwStyle & TCS_BOTTOM)
                OffsetRect(rcItem, 0, 1);
            DrawFrameControl(dc, rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | (nHint & HINT_ACTIVE_ITEM ? DFCS_MONO : DFCS_FLAT));
            return;
        }
        SelectObject(dc, tabdat->m_hPenLight);
		
		if(nHint & HINT_ACTIVE_ITEM)
			MoveToEx(dc, rcItem->left, rcItem->bottom + 2, &pt);
		else
			MoveToEx(dc, rcItem->left, rcItem->bottom, &pt);
		
		LineTo(dc, rcItem->left, rcItem->top + 2);
		LineTo(dc, rcItem->left + 2, rcItem->top);

        if(nHint & HINT_CUT) {
			LineTo(dc, rcItem->right, rcItem->top);
			bDoNotCut = FALSE;
		}
		else
			LineTo(dc, rcItem->right - 2, rcItem->top);
		
		if(!nHint)
			SelectObject(dc, tabdat->m_hPenItemShadow);
		else {
			if(nHint & HINT_ACTIVATE_RIGHT_SIDE)
				SelectObject(dc, tabdat->m_hPenShadow);
		}
		if(bDoNotCut) {
			if(nHint & HINT_ACTIVE_ITEM) {
				LineTo(dc, rcItem->right, rcItem->top + 2);
				LineTo(dc, rcItem->right, rcItem->bottom + 2);
			}
			else {
				LineTo(dc, rcItem->right, rcItem->top + 2);
				LineTo(dc, rcItem->right, rcItem->bottom + 1);
			}
		}
		else {
			//	CLBTabCtrl is not in stacked mode and current item overlaps
			//	msctls_updown32 control. So let's draw current item's rect 
			//	as cut.
			int nYPos = rcItem->top + 2;

			nYPos += ((rcItem->bottom - rcItem->top) / 5);
			rcItem->right += 1;
			if(nYPos < rcItem->bottom)
				LineTo(dc, rcItem->right, nYPos);

            nYPos += ((rcItem->bottom - rcItem->top) / 3);
			rcItem->right -= 2;
			if(nYPos < rcItem->bottom)
				LineTo(dc, rcItem->right, nYPos);

            nYPos += ((rcItem->bottom - rcItem->top) / 5);
			rcItem->right += 1;
			if(nYPos < rcItem->bottom)
				LineTo(dc, rcItem->right, nYPos);

            nYPos += ((rcItem->bottom - rcItem->top) / 5);
			rcItem->right += 1;
			if(nYPos < rcItem->bottom)
				LineTo(dc, rcItem->right, nYPos);
			if(nHint & HINT_ACTIVE_ITEM) {
				LineTo(dc, rcItem->right, rcItem->bottom + 2);
			}
			else
				LineTo(dc, rcItem->right, rcItem->bottom + 1);

		}
	}
}

int DWordAlign(int n)
{ 
    int rem = n % 4; 
    if(rem) 
        n += (4 - rem); 
    return n; 
}

HRESULT DrawThemesPart(HDC hDC, int iPartId, int iStateId, wchar_t *uiPartNameID, LPRECT prcBox)
{
    HRESULT hResult;
    
    UINT hTheme = pfnOpenThemeData(NULL, uiPartNameID);
    if(hTheme != 0)
        hResult = pfnDrawThemeBackground(hTheme, hDC, iPartId, iStateId, prcBox, NULL);
	return hResult;
}

void DrawThemesXpTabItem(HDC pDC, int ixItem, RECT *rcItem, UINT uiFlag, struct myTabCtrl *tabdat) 
{
	BOOL bBody  = (uiFlag & 1) ? TRUE : FALSE;
	BOOL bSel   = (uiFlag & 2) ? TRUE : FALSE;
	BOOL bHot   = (uiFlag & 4) ? TRUE : FALSE;
	BOOL bBottom = (uiFlag & 8) ? TRUE : FALSE;	// mirror
    SIZE szBmp;
    HDC     dcMem;	
    HBITMAP bmpMem, pBmpOld;
    RECT rcMem;
    BITMAPINFO biOut; 
    BITMAPINFOHEADER *bihOut;
    int nBmpWdtPS;
    int nSzBuffPS;
    LPBYTE pcImg = NULL;
    int nStart = 0, nLenSub = 0;
    szBmp.cx = rcItem->right - rcItem->left;
    szBmp.cy = rcItem->bottom - rcItem->top;
    
	// 1st draw background
    dcMem = CreateCompatibleDC(pDC);
	bmpMem = CreateCompatibleBitmap(pDC, szBmp.cx, szBmp.cy);

    pBmpOld = SelectObject(dcMem, bmpMem);

    rcMem.left = rcMem.top = 0;
    rcMem.right = szBmp.cx;
    rcMem.bottom = szBmp.cy;
    
    if(bSel) 
        rcMem.bottom++;

    if(bBody)
        DrawThemesPart(dcMem, 9, 0, L"TAB", &rcMem);	// TABP_PANE=9,  0, 'TAB'
	else 
        DrawThemesPart(dcMem, 1, bSel ? 3 : (bHot ? 2 : 1), L"TAB", &rcMem);
																// TABP_TABITEM=1, TIS_SELECTED=3:TIS_HOT=2:TIS_NORMAL=1, 'TAB'
	// 2nd init some extra parameters
    ZeroMemory(&biOut,sizeof(BITMAPINFO));	// Fill local pixel arrays
    bihOut = &biOut.bmiHeader;
    
	bihOut->biSize = sizeof (BITMAPINFOHEADER);
	bihOut->biCompression = BI_RGB;
	bihOut->biPlanes = 1;		  
    bihOut->biBitCount = 24;	// force as RGB: 3 bytes,24 bits -> good for rotating bitmap in any resolution
	bihOut->biWidth =szBmp.cx; 
    bihOut->biHeight=szBmp.cy;

	nBmpWdtPS = DWordAlign(szBmp.cx*3);
	nSzBuffPS = ((nBmpWdtPS * szBmp.cy) / 8 + 2) * 8;
    
	if(bBottom)
        pcImg = malloc(nSzBuffPS);
        
	if(bBody && bBottom) {
        nStart = 3;
        nLenSub = 4;	// if bottom oriented flip the body contest only (no shadows were flipped)
    }
	// 3rd if it is left oriented tab, draw tab context before mirroring or rotating (before GetDIBits)

    // 4th get bits (for rotate) and mirror: body=(all except top) tab=(all except top)
	if(!bBody && bBottom)	{									// get bits: 
		GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
        bihOut->biHeight = -szBmp.cy; 				// to mirror bitmap is eough to use negative height between Get/SetDIBits
        SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
    }

    // 5th if it is bottom or right oriented tab, draw after mirroring background (do GetDIBits again)
	if(!bBody && ixItem >= 0) {
		if(bSel) 
            rcMem.bottom--;
        //DrawItem(tabdat, dcMem, rcItem, HINT_TRANSPARENT, ixItem);
		//DrawTabItem(&dcMem, ixItem, rcMem, uiFlag);
    }
	if(pcImg)
        free(pcImg);
    
	// 6th blit mirrored/rotated image to the screen
	BitBlt(pDC, rcItem->left, rcItem->top, szBmp.cx, szBmp.cy, dcMem, 0, 0, SRCCOPY);
	SelectObject(dcMem, pBmpOld);
    DeleteObject(bmpMem);
    DeleteDC(dcMem);
}

BOOL CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct myTabCtrl *tabdat = 0;
    tabdat = (struct myTabCtrl *)GetWindowLong(hwnd, GWL_USERDATA);
    
    if(tabdat)
        tabdat->dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    
    switch(msg) {
        case EM_SUBCLASSED:
        {
            NONCLIENTMETRICS nclim;
            
            tabdat = (struct myTabCtrl *)malloc(sizeof(struct myTabCtrl));
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)tabdat);
            ZeroMemory((void *)tabdat, sizeof(struct myTabCtrl));
            tabdat->m_hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
            tabdat->m_hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
            tabdat->m_hPenItemShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
            tabdat->hwnd = hwnd;
            tabdat->cx = GetSystemMetrics(SM_CXSMICON);
            tabdat->cy = GetSystemMetrics(SM_CYSMICON);

            nclim.cbSize = sizeof(nclim);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nclim, 0);
            tabdat->m_hMenuFont = CreateFontIndirect(&nclim.lfMenuFont);
            return 0;
        }
        case EM_SEARCHSCROLLER:
        {
            HWND hwndChild;
            char szClassName[128];
            /*
             * search the updown control (scroll arrows) to subclass it...
             * the control is dynamically created and may not exist as long as it is
             * not needed. So we have to search it everytime we need to paint. However,
             * it is sufficient to search it once. So this message is called, whenever
             * a new tab is inserted
             */

            if(tabdat->m_hwndSpin == 0) {
                hwndChild = GetWindow(hwnd, GW_CHILD);
                while(hwndChild) {
                    int nRet = GetClassNameA(hwndChild, szClassName, sizeof(szClassName));
                    if(nRet && strncmp(szClassName, "msctls_updown32", sizeof(szClassName)))
                        hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
                    else {
                        GetWindowRect(hwndChild, &tabdat->m_rectUpDn);

                        //subclass the updown spinner control
                        
                        if(tabdat->m_hwndSpin == 0) {
                            tabdat->m_hwndSpin = hwndChild;
                            OldSpinCtrlProc = (WNDPROC)SetWindowLong(hwndChild, GWL_WNDPROC, (LONG)SpinCtrlSubclassProc);
                            InvalidateRect(hwndChild, NULL, TRUE);
                        }
                        hwndChild = 0;
                    }
                }
            }
            return 0;
        }
        case WM_DESTROY:
        case EM_UNSUBCLASSED:
            if(tabdat) {
                if(tabdat->m_hwndSpin)
                    SetWindowLong(tabdat->m_hwndSpin, GWL_WNDPROC, (LONG)OldSpinCtrlProc);
                // clean up pre-created gdi objects
                DeleteObject(tabdat->m_hPenItemShadow);
                DeleteObject(tabdat->m_hPenLight);
                DeleteObject(tabdat->m_hPenShadow);
                DeleteObject(tabdat->m_hMenuFont);
                free(tabdat);
                SetWindowLong(hwnd, GWL_USERDATA, 0L);
            }
            break;
        case WM_MBUTTONDOWN: {
            POINT pt;
            GetCursorPos(&pt);
            SendMessage(GetParent(hwnd), DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
            return 1;
        }
        case WM_SIZE:
        {
            if(tabdat->m_hwndSpin != 0) {
                if(IsWindowVisible(tabdat->m_hwndSpin)) {
                    GetWindowRect(tabdat->m_hwndSpin, &tabdat->m_rectUpDn);
                    RectScreenToClient(hwnd, &tabdat->m_rectUpDn);
                }
                else
                    tabdat->m_rectUpDn.left = tabdat->m_rectUpDn.top = tabdat->m_rectUpDn.right = tabdat->m_rectUpDn.bottom = 0;
            }
            break;
        }
        case WM_ERASEBKGND:
            break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rectTemp, rctPage, rctActive, rcItem;
            RECT rectUpDn = {0,0,0,0};
            int nCount = TabCtrl_GetItemCount(hwnd), i;
            TCITEM item = {0};
            int iActive, hotItem;
            POINT pt;
            HPEN hPenOld;
            UINT uiFlags = 1;
            UINT uiBottom = 0;
            TCHITTESTINFO hti;
            BOOL bClassicDraw = (tabdat->dwStyle & TCS_BUTTONS) || (tabdat->dwStyle & TCS_BOTTOM && myGlobals.m_FlatTabs);
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rctPage);
            iActive = TabCtrl_GetCurSel(hwnd);
            TabCtrl_GetItemRect(hwnd, iActive, &rctActive);

            FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
            //_DebugPopup(0, "draw %d, %d, %d, %d", ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
            if(tabdat->dwStyle & TCS_BOTTOM)
                rctPage.bottom = rctActive.top;
            else
                rctPage.top = rctActive.bottom;
            
            /*
             * visual style support
             */
            
            if(!bClassicDraw && pfnIsThemeActive != 0 && pfnIsThemeActive() && IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
                RECT rcClip, rcClient;
                UINT uiVertBottm;
                
                GetClipBox(hdc, &rcClip);
                rcClient = rctPage;
                if(tabdat->dwStyle & TCS_BOTTOM) {
                    rcClient.bottom = rctPage.bottom + 1;
                    uiBottom = 8;
                    uiFlags |= uiBottom;
                }
                else
                    rcClient.top = rctPage.top;
                DrawThemesXpTabItem(hdc, -1, &rcClient, uiFlags, tabdat);	// TABP_PANE=9,0,'TAB'
            }
            else {
                if(!(tabdat->dwStyle & TCS_BUTTONS) && IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
                    rectTemp = rctPage;
                    hPenOld = SelectObject(hdc, tabdat->m_hPenLight);

                    MoveToEx(hdc, rectTemp.left, rectTemp.bottom +1, &pt);
                    LineTo(hdc, rectTemp.left, rectTemp.top + 1);

                    if(tabdat->dwStyle & TCS_BOTTOM) {
                        LineTo(hdc, rectTemp.right - 1, rectTemp.top + 1);
                        SelectObject(hdc, tabdat->m_hPenShadow);
                        LineTo(hdc, rectTemp.right - 1, rectTemp.bottom + 1);
                        LineTo(hdc, rctActive.right, rectTemp.bottom + 1);
                        MoveToEx(hdc, rctActive.left, rectTemp.bottom + 1, &pt);
                        LineTo(hdc, rectTemp.left, rectTemp.bottom + 1);
                    }
                    else {
                        if(rctActive.left >= 0) {
                            LineTo(hdc, rctActive.left, rctActive.bottom + 1);
                            if(IsRectEmpty(&rectUpDn))
                                MoveToEx(hdc, rctActive.right, rctActive.bottom + 1, &pt);
                            else {
                                if(rctActive.right >= rectUpDn.left)
                                    MoveToEx(hdc, rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN + 2, rctActive.bottom + 1, &pt);
                                else
                                    MoveToEx(hdc, rctActive.right, rctActive.bottom + 1, &pt);
                            }
                            LineTo(hdc, rectTemp.right - 2, rctActive.bottom + 1);
                        }
                        else {
                            RECT rectItemLeftmost;
                            UINT nItemLeftmost = FindLeftDownItem(hwnd);
                            TabCtrl_GetItemRect(hwnd, nItemLeftmost, &rectItemLeftmost);
                            LineTo(hdc, rectTemp.right - 2, rctActive.bottom + 1);
                        }
                        SelectObject(hdc, tabdat->m_hPenItemShadow);
                        LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
                        LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);

                        SelectObject(hdc, tabdat->m_hPenShadow);
                        MoveToEx(hdc, rectTemp.right - 1, rctActive.bottom + 1, &pt);
                        LineTo(hdc, rectTemp.right - 1, rectTemp.bottom - 1);
                        LineTo(hdc, rectTemp.left - 2, rectTemp.bottom - 1);
                    }
                }
            }
            uiFlags = 0;
            GetCursorPos(&hti.pt);
            ScreenToClient(hwnd, &hti.pt);
            hti.flags = 0;
            hotItem = TabCtrl_HitTest(hwnd, &hti);
            for(i = 0; i < nCount; i++) {
                if(i != iActive) {
                    TabCtrl_GetItemRect(hwnd, i, &rcItem);
                    if (IntersectRect(&rectTemp, &rcItem, &ps.rcPaint)) {
                        int nHint = 0;
                        if(!IsRectEmpty(&tabdat->m_rectUpDn) && rcItem.right >= tabdat->m_rectUpDn.left) {
                            nHint |= HINT_CUT;
                            rcItem.right = tabdat->m_rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN;
                        }
                        if(!bClassicDraw && pfnIsThemeActive != 0 && pfnIsThemeActive()) {
                            InflateRect(&rcItem, -1, 0);
                            DrawThemesXpTabItem(hdc, i, &rcItem, uiFlags | uiBottom | (i == hotItem ? 4 : 0), tabdat);
                            nHint |= HINT_TRANSPARENT;
                            DrawItem(tabdat, hdc, &rcItem, nHint, i);
                        }
                        else {
                            DrawItemRect(tabdat, hdc, &rcItem, nHint);
                            DrawItem(tabdat, hdc, &rcItem, nHint, i);
                        }
                    }
                }
            }
            if (IntersectRect(&rectTemp, &rctActive, &ps.rcPaint) && rctActive.left > 0) {
                int nHint = 0;
                rcItem = rctActive;
                //_DebugPopup(0, "active %d, %d, %d, %d", rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
                if(!IsRectEmpty(&tabdat->m_rectUpDn) && rcItem.right >= tabdat->m_rectUpDn.left) {
                    nHint |= HINT_CUT;
                    rcItem.right = tabdat->m_rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN;
                }
                if(!bClassicDraw && pfnIsThemeActive != 0 && pfnIsThemeActive()) {
                    InflateRect(&rcItem, 1, 2);
                    DrawThemesXpTabItem(hdc, iActive, &rcItem, 2 | uiBottom, tabdat);
                    nHint |= HINT_TRANSPARENT;
                    DrawItem(tabdat, hdc, &rcItem, nHint, iActive);
                }
                else {
                    DrawItemRect(tabdat, hdc, &rcItem, HINT_ACTIVATE_RIGHT_SIDE|HINT_ACTIVE_ITEM | nHint);
                    DrawItem(tabdat, hdc, &rcItem, HINT_ACTIVE_ITEM | nHint, iActive);
                }
            }
            SelectObject(hdc, hPenOld);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return CallWindowProc(OldTabControlProc, hwnd, msg, wParam, lParam); 
}

