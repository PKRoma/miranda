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
extern struct ContainerWindowData *pFirstContainer;
/*
 * visual styles support (XP+)
 * returns 0 on failure
 */

HMODULE hUxTheme = 0;

// function pointers

BOOL (PASCAL* pfnIsThemeActive)() = 0;
HANDLE (PASCAL* pfnOpenThemeData)(HWND hwnd, LPCWSTR pszClassList) = 0;
UINT (PASCAL* pfnDrawThemeBackground)(HANDLE htheme, HDC hdc, int iPartID, int iStateID, RECT* prcBx, RECT* prcClip) = 0;
UINT (PASCAL* pfnCloseThemeData)(HANDLE hTheme) = 0;

int InitVSApi()
{
    if((hUxTheme = LoadLibraryA("uxtheme.dll")) == 0)
        return FALSE;

    pfnIsThemeActive = GetProcAddress(hUxTheme, "IsThemeActive");
    pfnOpenThemeData = GetProcAddress(hUxTheme, "OpenThemeData");
    pfnDrawThemeBackground = GetProcAddress(hUxTheme, "DrawThemeBackground");
    pfnCloseThemeData = GetProcAddress(hUxTheme, "CloseThemeData");
    if(pfnIsThemeActive != 0 && pfnOpenThemeData != 0 && pfnDrawThemeBackground != 0 && pfnCloseThemeData != 0) {
        return 1;
    }
    return 0;
}

int FreeVSApi()
{
    if(hUxTheme != 0)
        FreeLibrary(hUxTheme);
    return 0;
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
				rctLeft = rctCur;
				nItem = i;	
			}
		}
	}
	return nItem;
}

static struct colOptions {UINT id; UINT defclr; char *szKey; } tabcolors[] = {
    IDC_TXTCLRNORMAL, COLOR_BTNTEXT, "tab_txt_normal",
    IDC_TXTCLRACTIVE, COLOR_BTNTEXT, "tab_txt_active",
    IDC_TXTCLRUNREAD, COLOR_BTNTEXT, "tab_txt_unread",
    IDC_BKGCLRNORMAL, COLOR_3DFACE, "tab_bg_normal",
    IDC_BKGCLRACTIVE, COLOR_HIGHLIGHT, "tab_bg_active",
    IDC_BKGCLRUNREAD, COLOR_HIGHLIGHT, "tab_bg_unread",
    0, 0, NULL
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
            break;
            rcItem.top++;
            rcItem.left++;
            rcItem.bottom--;
            rcItem.right--;
            FillRect(dc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
            DrawFrameControl(dc, &rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_MONO);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return CallWindowProc(OldSpinCtrlProc, hwnd, msg, wParam, lParam); 
}

/*
 * draws the item contents (icon and label)
 * it obtains the label and icon handle directly from the message window data
 */
 
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
        BOOL bFill = ((dwStyle & TCS_BOTTOM) || (dwStyle & TCS_BUTTONS)) && (!(tabdat->m_skinning) || myGlobals.m_TabAppearance & TCF_NOSKINNING);
        int oldMode = 0;
        InflateRect(rcItem, -1, -1);
        
        if(nHint & HINT_ACTIVE_ITEM)
            SetTextColor(dc, myGlobals.tabConfig.colors[1]);
        else if(dat->mayFlashTab == TRUE)
            SetTextColor(dc, myGlobals.tabConfig.colors[2]);
        else
            SetTextColor(dc, myGlobals.tabConfig.colors[0]);

        oldMode = SetBkMode(dc, TRANSPARENT);

        if(bFill) {
            if(dat->mayFlashTab == TRUE)
                bg = myGlobals.tabConfig.m_hBrushUnread;
            else if(nHint & HINT_ACTIVE_ITEM)
                bg = myGlobals.tabConfig.m_hBrushActive;
            else
                bg = myGlobals.tabConfig.m_hBrushDefault;
            FillRect(dc, rcItem, bg);
        }
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
        SelectObject(dc, myGlobals.tabConfig.m_hMenuFont);
        DrawText(dc, dat->newtitle, _tcslen(dat->newtitle), rcItem, DT_SINGLELINE | DT_VCENTER);
        if(oldMode)
            SetBkMode(dc, oldMode);
    }
}

/*
 * draws the item rect in *classic* style (no visual themes
 */

void DrawItemRect(struct myTabCtrl *tabdat, HDC dc, RECT *rcItem, int nHint)
{
    POINT pt;
    
    rcItem->bottom -= 1;
	if(rcItem->left > 0) {
		BOOL bDoNotCut = TRUE;

        if((tabdat->dwStyle & TCS_BUTTONS) || (tabdat->dwStyle & TCS_BOTTOM)) {
            // draw frame controls for button or bottom tabs
            if(tabdat->dwStyle & TCS_BOTTOM) {
                OffsetRect(rcItem, 0, 1);
                if(!(tabdat->dwStyle & TCS_BUTTONS))
                    InflateRect(rcItem, -1, 0);
            }
            DrawFrameControl(dc, rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | (nHint & HINT_ACTIVE_ITEM ? DFCS_MONO : DFCS_FLAT));
            return;
        }
        SelectObject(dc, myGlobals.tabConfig.m_hPenLight);
		
		if(nHint & HINT_ACTIVE_ITEM) {
            MoveToEx(dc, rcItem->left, rcItem->bottom + 1, &pt);
            rcItem->top--;
        }
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
			SelectObject(dc, myGlobals.tabConfig.m_hPenItemShadow);
		else {
			if(nHint & HINT_ACTIVATE_RIGHT_SIDE)
				SelectObject(dc, myGlobals.tabConfig.m_hPenShadow);
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

/*
 * draws a theme part (identifier in uiPartNameID) using the given clipping rectangle
 */

HRESULT DrawThemesPart(struct myTabCtrl *tabdat, HDC hDC, int iPartId, int iStateId, LPRECT prcBox)
{
    HRESULT hResult;
    if(pfnDrawThemeBackground == NULL)
        return 0;
    
    if(tabdat->hTheme != 0)
        hResult = pfnDrawThemeBackground(tabdat->hTheme, hDC, iPartId, iStateId, prcBox, NULL);
	return hResult;
}

/*
 * draw a themed tab item. mirrors the bitmap for bottom-aligned tabs
 */

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
    
    dcMem = CreateCompatibleDC(pDC);
	bmpMem = CreateCompatibleBitmap(pDC, szBmp.cx, szBmp.cy);

    pBmpOld = SelectObject(dcMem, bmpMem);

    rcMem.left = rcMem.top = 0;
    rcMem.right = szBmp.cx;
    rcMem.bottom = szBmp.cy;
    
    if(bSel) 
        rcMem.bottom++;

    if(bBody)
        DrawThemesPart(tabdat, dcMem, 9, 0, &rcMem);	// TABP_PANE=9,  0, 'TAB'
	else 
        DrawThemesPart(tabdat, dcMem, 1, bSel ? 3 : (bHot ? 2 : 1), &rcMem);
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
    // flip (mirror) the bitmap horizontally
    
	if(!bBody && bBottom)	{									// get bits: 
		GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
        bihOut->biHeight = -szBmp.cy; 				// to mirror bitmap is eough to use negative height between Get/SetDIBits
        SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
    }
	if(pcImg)
        free(pcImg);
    
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
            tabdat = (struct myTabCtrl *)malloc(sizeof(struct myTabCtrl));
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)tabdat);
            ZeroMemory((void *)tabdat, sizeof(struct myTabCtrl));
            tabdat->hwnd = hwnd;
            tabdat->cx = GetSystemMetrics(SM_CXSMICON);
            tabdat->cy = GetSystemMetrics(SM_CYSMICON);

            tabdat->m_skinning = 0;
            if(IsWinVerXPPlus()) {
                if(pfnIsThemeActive != 0)
                    if(pfnIsThemeActive()) {
                        tabdat->m_skinning = TRUE;
                        if(pfnOpenThemeData != 0) {
                            if((tabdat->hTheme = pfnOpenThemeData(NULL, L"TAB")) == 0)
                                tabdat->m_skinning = FALSE;
                        }
                    }
            }
            return 0;
        }
        case WM_THEMECHANGED:
            if(IsWinVerXPPlus()) {
                if(pfnIsThemeActive != 0)
                    if(pfnIsThemeActive()) {
                        tabdat->m_skinning = TRUE;
                        if(tabdat->hTheme != 0 && pfnCloseThemeData != 0)
                            pfnCloseThemeData(tabdat->hTheme);
                        if(pfnOpenThemeData != 0) {
                            if((tabdat->hTheme = pfnOpenThemeData(NULL, L"TAB")) == 0)
                                tabdat->m_skinning = FALSE;
                        }
                    }
            }
            break;
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
                if(tabdat->hTheme != 0 && pfnCloseThemeData != 0)
                    pfnCloseThemeData(tabdat->hTheme);
                // clean up pre-created gdi objects
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
            BOOL bClassicDraw = (tabdat->m_skinning == FALSE) || (myGlobals.m_TabAppearance & TCF_NOSKINNING) || (tabdat->dwStyle & TCS_BUTTONS) || (tabdat->dwStyle & TCS_BOTTOM && (myGlobals.m_TabAppearance & TCF_FLAT));
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
            
            if(!bClassicDraw && IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
                RECT rcClip, rcClient;
                
                GetClipBox(hdc, &rcClip);
                rcClient = rctPage;
                if(tabdat->dwStyle & TCS_BOTTOM) {
                    rcClient.bottom = rctPage.bottom;
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
                    hPenOld = SelectObject(hdc, myGlobals.tabConfig.m_hPenLight);

                    MoveToEx(hdc, rectTemp.left, rectTemp.bottom +1, &pt);
                    LineTo(hdc, rectTemp.left, rectTemp.top);

                    if(tabdat->dwStyle & TCS_BOTTOM) {
                        LineTo(hdc, rectTemp.right - 1, rectTemp.top + 1);
                        SelectObject(hdc, myGlobals.tabConfig.m_hPenShadow);
                        LineTo(hdc, rectTemp.right - 1, rectTemp.bottom + 1);
                        LineTo(hdc, rctActive.right - 2, rectTemp.bottom + 1);
                        MoveToEx(hdc, rctActive.left + 2, rectTemp.bottom + 1, &pt);
                        LineTo(hdc, rectTemp.left, rectTemp.bottom + 1);
                    }
                    else {
                        if(rctActive.left >= 0) {
                            LineTo(hdc, rctActive.left, rctActive.bottom);
                            if(IsRectEmpty(&rectUpDn))
                                MoveToEx(hdc, rctActive.right, rctActive.bottom, &pt);
                            else {
                                if(rctActive.right >= rectUpDn.left)
                                    MoveToEx(hdc, rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN + 2, rctActive.bottom + 1, &pt);
                                else
                                    MoveToEx(hdc, rctActive.right, rctActive.bottom, &pt);
                            }
                            LineTo(hdc, rectTemp.right - 2, rctActive.bottom);
                        }
                        else {
                            RECT rectItemLeftmost;
                            UINT nItemLeftmost = FindLeftDownItem(hwnd);
                            TabCtrl_GetItemRect(hwnd, nItemLeftmost, &rectItemLeftmost);
                            LineTo(hdc, rectTemp.right - 2, rctActive.bottom);
                        }
                        SelectObject(hdc, myGlobals.tabConfig.m_hPenItemShadow);
                        LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
                        LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);

                        SelectObject(hdc, myGlobals.tabConfig.m_hPenShadow);
                        MoveToEx(hdc, rectTemp.right - 1, rctActive.bottom, &pt);
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
                        if(!bClassicDraw) {
                            InflateRect(&rcItem,  0, 0);
                            DrawThemesXpTabItem(hdc, i, &rcItem, uiFlags | uiBottom | (i == hotItem ? 4 : 0), tabdat);
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
                if(!bClassicDraw) {
                    InflateRect(&rcItem, 2, 2);
                        rcItem.bottom--;
                    DrawThemesXpTabItem(hdc, iActive, &rcItem, 2 | uiBottom, tabdat);
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

void ReloadTabConfig()
{
    NONCLIENTMETRICS nclim;
    int i = 0;

    myGlobals.tabConfig.m_hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
    myGlobals.tabConfig.m_hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
    myGlobals.tabConfig.m_hPenItemShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

    nclim.cbSize = sizeof(nclim);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nclim, 0);
    myGlobals.tabConfig.m_hMenuFont = CreateFontIndirect(&nclim.lfMenuFont);

    while(tabcolors[i].szKey != NULL) {
        myGlobals.tabConfig.colors[i] = DBGetContactSettingDword(NULL, SRMSGMOD_T, tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
        i++;
    }

    myGlobals.tabConfig.m_hBrushDefault = CreateSolidBrush(myGlobals.tabConfig.colors[3]);
    myGlobals.tabConfig.m_hBrushActive = CreateSolidBrush(myGlobals.tabConfig.colors[4]);
    myGlobals.tabConfig.m_hBrushUnread = CreateSolidBrush(myGlobals.tabConfig.colors[5]);
}

void FreeTabConfig()
{
    DeleteObject(myGlobals.tabConfig.m_hPenItemShadow);
    DeleteObject(myGlobals.tabConfig.m_hPenLight);
    DeleteObject(myGlobals.tabConfig.m_hPenShadow);
    DeleteObject(myGlobals.tabConfig.m_hMenuFont);
    DeleteObject(myGlobals.tabConfig.m_hBrushActive);
    DeleteObject(myGlobals.tabConfig.m_hBrushDefault);
    DeleteObject(myGlobals.tabConfig.m_hBrushUnread);
}

/*
 * options dialog for setting up tab options
 */

BOOL CALLBACK DlgProcTabConfig(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", TCF_DEFAULT);
            int i = 0;
            COLORREF clr;
            
            SetWindowTextA(hwndDlg, Translate("Configure tab appearance"));
            CheckDlgButton(hwndDlg, IDC_FLATTABS2, dwFlags & TCF_FLAT);
            CheckDlgButton(hwndDlg, IDC_FLASHICON, dwFlags & TCF_FLASHICON);
            CheckDlgButton(hwndDlg, IDC_FLASHLABEL, dwFlags & TCF_FLASHLABEL);
            CheckDlgButton(hwndDlg, IDC_NOSKINNING, dwFlags & TCF_NOSKINNING);

            while(tabcolors[i].szKey != NULL) {
                clr = (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
                SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_SETCOLOUR, 0, (LPARAM)clr);
                i++;
            }
            SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(10, 1));
            SendDlgItemMessage(hwndDlg, IDC_SPIN3, UDM_SETRANGE, 0, MAKELONG(10, 1));
            SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3));
            SendDlgItemMessage(hwndDlg, IDC_SPIN3, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 3));
            SetDlgItemInt(hwndDlg, IDC_TABPADDING, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3), FALSE);;
            SetDlgItemInt(hwndDlg, IDC_HTABPADDING, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 3), FALSE);;
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                {
                    int i = 0;
                    COLORREF clr;
                    struct ContainerWindowData *pContainer = pFirstContainer;
                    
                    DWORD dwFlags = (IsDlgButtonChecked(hwndDlg, IDC_FLATTABS2) ? TCF_FLAT : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_FLASHICON) ? TCF_FLASHICON : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_FLASHLABEL) ? TCF_FLASHLABEL : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_NOSKINNING) ? TCF_NOSKINNING : 0);
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", dwFlags);
                    myGlobals.m_TabAppearance = dwFlags;
                    while(tabcolors[i].szKey != NULL) {
                        DBGetContactSettingDword(NULL, SRMSGMOD_T, tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
                        clr = SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_GETCOLOUR, 0, 0);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, tabcolors[i].szKey, clr);
                        i++;
                    }
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "y-pad", GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "x-pad", GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE));
                    FreeTabConfig();
                    ReloadTabConfig();
                    while(pContainer) {
                        TabCtrl_SetPadding(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE), GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE));
                        RedrawWindow(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
                        pContainer = pContainer->pNextContainer;
                    }
                    DestroyWindow(hwndDlg);
                    break;
                }
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
            }
    }
    return FALSE;
}


