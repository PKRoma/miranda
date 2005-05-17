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

static WNDPROC OldSpinCtrlProc;

BOOL CALLBACK SpinCtrlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_PAINT:
            break;
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
        BOOL bFill = (dwStyle & TCS_BOTTOM) || (dwStyle & TCS_BUTTONS);
        rcItem->left++;
        rcItem->right--;
        rcItem->top++;
        rcItem->bottom--;
        
        if(nHint & HINT_ACTIVE_ITEM) {
            SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            bg = GetSysColorBrush(COLOR_3DSHADOW);
            if(bFill)
                SetBkColor(dc, GetSysColor(COLOR_3DSHADOW));
        }
        else {
            SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
            bg = GetSysColorBrush(COLOR_3DFACE);
            if(bFill)
                SetBkColor(dc, GetSysColor(COLOR_3DFACE));
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
            if(!IsRectEmpty(&tabdat->m_rectUpDn) && rcItem->right >= tabdat->m_rectUpDn.left - 2) {
                rcItem->right = tabdat->m_rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN;
            }
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
		if(!IsRectEmpty(&tabdat->m_rectUpDn) && rcItem->right >= tabdat->m_rectUpDn.left) {
			rcItem->right = tabdat->m_rectUpDn.left - SHIFT_FROM_CUT_TO_SPIN;
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
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rectTemp, rctPage, rctActive, rcItem;
            RECT rectUpDn = {0,0,0,0};
            int nCount = TabCtrl_GetItemCount(hwnd), i;
            TCITEM item = {0};
            int iActive;
            POINT pt;
            HPEN hPenOld;
            
            hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
            GetClientRect(hwnd, &rctPage);
            iActive = TabCtrl_GetCurSel(hwnd);
            TabCtrl_GetItemRect(hwnd, iActive, &rctActive);

            if(tabdat->dwStyle & TCS_BOTTOM)
                rctPage.bottom = rctActive.top;
            else
                rctPage.top = rctActive.bottom;
            
            // paint the tabctrl frame

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
            for(i = 0; i < nCount; i++) {
                if(i != iActive) {
                    TabCtrl_GetItemRect(hwnd, i, &rcItem);
                    if (IntersectRect(&rectTemp, &rcItem, &ps.rcPaint)) {
                        DrawItemRect(tabdat, hdc, &rcItem, 0);
                        DrawItem(tabdat, hdc, &rcItem, 0, i);
                    }
                }
            }
            if (IntersectRect(&ps.rcPaint, &rctActive, &ps.rcPaint) && rctActive.left > 0) {
                rcItem = rctActive;
                DrawItemRect(tabdat, hdc, &rcItem, HINT_ACTIVATE_RIGHT_SIDE|HINT_ACTIVE_ITEM);
                DrawItem(tabdat, hdc, &rcItem, HINT_ACTIVE_ITEM, iActive);
            }
            SelectObject(hdc, hPenOld);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return CallWindowProc(OldTabControlProc, hwnd, msg, wParam, lParam); 
}

