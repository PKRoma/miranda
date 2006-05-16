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

*/

#include "commonheaders.h"
#pragma hdrstop
#include <uxtheme.h>

extern MYGLOBALS myGlobals;
static WNDPROC OldTabControlClassProc;
extern struct ContainerWindowData *pFirstContainer;
extern HINSTANCE g_hInst;
extern PSLWA pSetLayeredWindowAttributes;
extern StatusItems_t StatusItems[];
extern BOOL g_framelessSkinmode;
extern BOOL g_skinnedContainers;

extern BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD);
static LRESULT CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HMODULE hUxTheme = 0;

// function pointers, use typedefs for casting to shut up the compiler when using GetProcAddress()

typedef BOOL (WINAPI *PITA)();
typedef HANDLE (WINAPI *POTD)(HWND, LPCWSTR);
typedef UINT (WINAPI *PDTB)(HANDLE, HDC, int, int, RECT *, RECT *);
typedef UINT (WINAPI *PCTD)(HANDLE);
typedef UINT (WINAPI *PDTT)(HANDLE, HDC, int, int, LPCWSTR, int, DWORD, DWORD, RECT *);

PITA pfnIsThemeActive = 0;
POTD pfnOpenThemeData = 0;
PDTB pfnDrawThemeBackground = 0;
PCTD pfnCloseThemeData = 0;
PDTT pfnDrawThemeText = 0;

#define FIXED_TAB_SIZE 100                  // default value for fixed width tabs

/*
 * visual styles support (XP+)
 * returns 0 on failure
 */

int InitVSApi()
{
    if((hUxTheme = LoadLibraryA("uxtheme.dll")) == 0)
        return 0;

    pfnIsThemeActive = (PITA)GetProcAddress(hUxTheme, "IsThemeActive");
    pfnOpenThemeData = (POTD)GetProcAddress(hUxTheme, "OpenThemeData");
    pfnDrawThemeBackground = (PDTB)GetProcAddress(hUxTheme, "DrawThemeBackground");
    pfnCloseThemeData = (PCTD)GetProcAddress(hUxTheme, "CloseThemeData");
    pfnDrawThemeText = (PDTT)GetProcAddress(hUxTheme, "DrawThemeText");
    
    MyEnableThemeDialogTexture = (BOOL (WINAPI *)(HANDLE, DWORD))GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
    if(pfnIsThemeActive != 0 && pfnOpenThemeData != 0 && pfnDrawThemeBackground != 0 && pfnCloseThemeData != 0 && pfnDrawThemeText != 0) {
        return 1;
    }
    return 0;
}

/*
 * unload uxtheme.dll
 */

int FreeVSApi()
{
    if(hUxTheme != 0)
        FreeLibrary(hUxTheme);
    return 0;
}

/*
 * register the new tab control as a window class (TSTabCtrlClass)
 */

int RegisterTabCtrlClass(void) 
{
	WNDCLASSEXA wc;
	
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = "TSTabCtrlClass";
	wc.lpfnWndProc    = TabControlSubclassProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra     = sizeof(struct TabControlData *);
	wc.hbrBackground  = 0;
	wc.style          = CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC;
	RegisterClassExA(&wc);
	return 0;
}

static void RectScreenToClient(HWND hwnd, RECT *rc)
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

static UINT FindLeftDownItem(HWND hwnd)
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

/*
 * tab control color definitions, including the database setting key names
 */

static struct colOptions {UINT id; UINT defclr; char *szKey; char *szSkinnedKey; } tabcolors[] = {
    IDC_TXTCLRNORMAL, COLOR_BTNTEXT, "tab_txt_normal", "S_tab_txt_normal",
    IDC_TXTCLRACTIVE, COLOR_BTNTEXT, "tab_txt_active", "S_tab_txt_active",
    IDC_TXTCLRUNREAD, COLOR_HOTLIGHT, "tab_txt_unread", "S_tab_txt_unread",
    IDC_TXTCLRHOTTRACK, COLOR_HOTLIGHT, "tab_txt_hottrack", "S_tab_txt_hottrack",
    IDC_BKGCLRNORMAL, COLOR_3DFACE, "tab_bg_normal", "tab_bg_normal",
    IDC_BKGCLRACTIVE, COLOR_3DFACE, "tab_bg_active", "tab_bg_active",
    IDC_BKGCLRUNREAD, COLOR_3DFACE, "tab_bg_unread", "tab_bg_unread",
    IDC_BKGCOLORHOTTRACK, COLOR_3DFACE, "tab_bg_hottrack", "tab_bg_hottrack",
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
 * draws the item contents (icon and label)
 * it obtains the label and icon handle directly from the message window data
 * no image list is used and necessary, the message window dialog procedure has to provide a valid
 * icon handle in dat->hTabIcon
 */
 
static void DrawItem(struct TabControlData *tabdat, HDC dc, RECT *rcItem, int nHint, int nItem)
{
    TCITEM item = {0};
    struct MessageWindowData *dat = 0;
    int iSize = 16;
    DWORD dwTextFlags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
    
    item.mask = TCIF_PARAM;
    TabCtrl_GetItem(tabdat->hwnd, nItem, &item);

    /*
     * get the message window data for the session to which this tab item belongs
     */
    
    if(IsWindow((HWND)item.lParam) && item.lParam != 0)
        dat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);
    
    if(dat) {
        HICON hIcon;
        HBRUSH bg;
        HFONT oldFont;
        DWORD dwStyle = tabdat->dwStyle;
        BOOL bFill = ((dwStyle & TCS_BUTTONS && !tabdat->pContainer->bSkinned) && (tabdat->m_skinning == FALSE || myGlobals.m_TabAppearance & TCF_NOSKINNING));
        int oldMode = 0;
        InflateRect(rcItem, -1, -1);
        
        if(nHint & HINT_ACTIVE_ITEM)
            SetTextColor(dc, myGlobals.tabConfig.colors[1]);
        else if(dat->mayFlashTab == TRUE)
            SetTextColor(dc, myGlobals.tabConfig.colors[2]);
        else if(nHint & HINT_HOTTRACK)
            SetTextColor(dc, myGlobals.tabConfig.colors[3]);
        else
            SetTextColor(dc, myGlobals.tabConfig.colors[0]);

        oldMode = SetBkMode(dc, TRANSPARENT);

        if(bFill) {
            if(dat->mayFlashTab == TRUE)
                bg = myGlobals.tabConfig.m_hBrushUnread;
            else if(nHint & HINT_ACTIVE_ITEM)
                bg = myGlobals.tabConfig.m_hBrushActive;
            else if(nHint & HINT_HOTTRACK)
                bg = myGlobals.tabConfig.m_hBrushHottrack;
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
		else {
            if(dat->si && dat->iFlashIcon)
                hIcon = dat->iFlashIcon;
			else if(dat->hTabIcon == dat->hTabStatusIcon && dat->hXStatusIcon)
				hIcon = dat->hXStatusIcon;
			else
				hIcon = dat->hTabIcon;
		}

        if(dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0) || !(myGlobals.m_TabAppearance & TCF_FLASHICON)) {
            DWORD ix = rcItem->left + tabdat->m_xpad - 1;
            DWORD iy = (rcItem->bottom + rcItem->top - iSize) / 2;
            if(dat->dwEventIsShown & MWF_SHOW_ISIDLE && myGlobals.m_IdleDetect) {
                ImageList_ReplaceIcon(myGlobals.g_hImageList, 0, hIcon);
				ImageList_DrawEx(myGlobals.g_hImageList, 0, dc, ix, iy, 0, 0, CLR_NONE, CLR_NONE, ILD_SELECTED);
            }
            else
                DrawIconEx (dc, ix, iy, hIcon, iSize, iSize, 0, NULL, DI_NORMAL | DI_COMPAT); 
        }

        // draw the overlay for chat tabs
        
        /*if(dat->bType == SESSIONTYPE_CHAT && dat->iFlashIcon && dat->mayFlashTab == FALSE)
            DrawIconEx (dc, rcItem->left + tabdat->m_xpad - 1, rcItem->top + 1, dat->iFlashIcon, 10, 10, 0, NULL, DI_NORMAL | DI_COMPAT); 
        */
        
        rcItem->left += (iSize + 2 + tabdat->m_xpad);
        
        if(dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0) || !(myGlobals.m_TabAppearance & TCF_FLASHLABEL)) {
            oldFont = SelectObject(dc, (HFONT)SendMessage(tabdat->hwnd, WM_GETFONT, 0, 0));
            if(tabdat->dwStyle & TCS_BUTTONS || !(tabdat->dwStyle & TCS_MULTILINE)) {
                rcItem->right -= tabdat->m_xpad;
                dwTextFlags |= DT_WORD_ELLIPSIS;
            }
#if defined(_UNICODE)
            if(tabdat->m_skinning == FALSE || myGlobals.m_TabAppearance & TCF_NOSKINNING)
                DrawText(dc, dat->newtitle, _tcslen(dat->newtitle), rcItem, dwTextFlags);
            else
                pfnDrawThemeText(dwStyle & TCS_BUTTONS ? tabdat->hThemeButton : tabdat->hTheme, dc, 1, nHint & HINT_ACTIVE_ITEM ? 3 : (nHint & HINT_HOTTRACK ? 2 : 1), dat->newtitle, _tcslen(dat->newtitle), dwTextFlags, 0, rcItem);
#else
            DrawText(dc, dat->newtitle, _tcslen(dat->newtitle), rcItem, dwTextFlags);
#endif
            SelectObject(dc, oldFont);
        }
        if(oldMode)
            SetBkMode(dc, oldMode);
    }
}

/*
 * draws the item rect (the "tab") in *classic* style (no visual themes
 */

static void DrawItemRect(struct TabControlData *tabdat, HDC dc, RECT *rcItem, int nHint)
{
    POINT pt;
    DWORD dwStyle = tabdat->dwStyle;

	rcItem->bottom -= 1;
	if(rcItem->left >= 0) {

        /*
         * draw "button style" tabs... raised edge for hottracked, sunken edge for active (pushed)
         * otherwise, they get a normal border
         */
        
        if(dwStyle & TCS_BUTTONS) {
            BOOL bClassicDraw = (tabdat->m_skinning == FALSE) || (myGlobals.m_TabAppearance & TCF_NOSKINNING);

            // draw frame controls for button or bottom tabs
            if(dwStyle & TCS_BOTTOM) {
                rcItem->top++;
            }
            else
                rcItem->bottom--;
            
            rcItem->right += 6;
            if(bClassicDraw) {
                if(tabdat->pContainer->bSkinned) {
                    StatusItems_t *item = nHint & HINT_ACTIVE_ITEM ? &StatusItems[ID_EXTBKBUTTONSPRESSED] : (nHint & HINT_HOTTRACK ? &StatusItems[ID_EXTBKBUTTONSMOUSEOVER] : &StatusItems[ID_EXTBKBUTTONSNPRESSED]);

                    if(!item->IGNORED) {
                        SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
                        DrawAlpha(dc, rcItem, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
                                  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
                    }
                    else
                        goto b_nonskinned;
                }
                else {
b_nonskinned:                    
                    if(nHint & HINT_ACTIVE_ITEM)
                        DrawEdge(dc, rcItem, EDGE_ETCHED, BF_RECT|BF_SOFT);
                    else if(nHint & HINT_HOTTRACK)
                        DrawEdge(dc, rcItem, EDGE_BUMP, BF_RECT | BF_MONO | BF_SOFT);
                    else
                        DrawEdge(dc, rcItem, EDGE_RAISED, BF_RECT|BF_SOFT);
                }
            }
            else {
                FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                pfnDrawThemeBackground(tabdat->hThemeButton, dc, 1, nHint & HINT_ACTIVE_ITEM ? 3 : (nHint & HINT_HOTTRACK ? 2 : 1), rcItem, rcItem);
            }
            return;
        }
        SelectObject(dc, myGlobals.tabConfig.m_hPenLight);
		
		if(nHint & HINT_ACTIVE_ITEM) {
            if(dwStyle & TCS_BOTTOM) {
				if(tabdat->pContainer->bSkinned)
					SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				else
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                rcItem->bottom +=2;
            }
            else {
                rcItem->bottom += 2;
				if(tabdat->pContainer->bSkinned)
					SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				else
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                rcItem->bottom--;
                rcItem->top -=2;
            }
			if(tabdat->pContainer->bSkinned) {
				StatusItems_t *item = &StatusItems[dwStyle & TCS_BOTTOM ? ID_EXTBKTABITEMACTIVEBOTTOM : ID_EXTBKTABITEMACTIVE];
				if(!item->IGNORED) {
                    rcItem->left += item->MARGIN_LEFT; rcItem->right -= item->MARGIN_RIGHT;
					DrawAlpha(dc, rcItem, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
					return;
				}
			}
        }
		if(tabdat->pContainer->bSkinned) {
			StatusItems_t *item = &StatusItems[dwStyle & TCS_BOTTOM ? (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACKBOTTOM : ID_EXTBKTABITEMBOTTOM) : 
                                               (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACK : ID_EXTBKTABITEM)];
			if(!item->IGNORED) {
                rcItem->left += item->MARGIN_LEFT; rcItem->right -= item->MARGIN_RIGHT;
				DrawAlpha(dc, rcItem, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
						  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
				return;
			}
		}
        if(dwStyle & TCS_BOTTOM) {
            MoveToEx(dc, rcItem->left, rcItem->top - (nHint & HINT_ACTIVE_ITEM ? 1 : 0), &pt);
            LineTo(dc, rcItem->left, rcItem->bottom - 2);
            LineTo(dc, rcItem->left + 2, rcItem->bottom);
            SelectObject(dc, myGlobals.tabConfig.m_hPenShadow);
            LineTo(dc, rcItem->right - 3, rcItem->bottom);

            LineTo(dc, rcItem->right - 1, rcItem->bottom - 2);
            LineTo(dc, rcItem->right - 1, rcItem->top - 1);
            MoveToEx(dc, rcItem->right - 2, rcItem->top, &pt);
            SelectObject(dc, myGlobals.tabConfig.m_hPenItemShadow);
            LineTo(dc, rcItem->right - 2, rcItem->bottom - 1);
            MoveToEx(dc, rcItem->right - 3, rcItem->bottom - 1, &pt);
            LineTo(dc, rcItem->left + 2, rcItem->bottom - 1);
        }
        else {
            MoveToEx(dc, rcItem->left, rcItem->bottom, &pt);
            LineTo(dc, rcItem->left, rcItem->top + 2);
            LineTo(dc, rcItem->left + 2, rcItem->top);
            LineTo(dc, rcItem->right - 2, rcItem->top);
            SelectObject(dc, myGlobals.tabConfig.m_hPenItemShadow);

            MoveToEx(dc, rcItem->right - 2, rcItem->top + 1, &pt);
            LineTo(dc, rcItem->right - 2, rcItem->bottom + 1);
            SelectObject(dc, myGlobals.tabConfig.m_hPenShadow);
            MoveToEx(dc, rcItem->right - 1, rcItem->top + 2, &pt);
            LineTo(dc, rcItem->right - 1, rcItem->bottom + 1);
        }
	}
}

static int DWordAlign(int n)
{ 
    int rem = n % 4; 
    if(rem) 
        n += (4 - rem); 
    return n; 
}

/*
 * draws a theme part (identifier in uiPartNameID) using the given clipping rectangle
 */

static HRESULT DrawThemesPart(struct TabControlData *tabdat, HDC hDC, int iPartId, int iStateId, LPRECT prcBox)
{
    HRESULT hResult = 0;
    
    if(pfnDrawThemeBackground == NULL)
        return 0;
    
    if(tabdat->hTheme != 0)
        hResult = pfnDrawThemeBackground(tabdat->hTheme, hDC, iPartId, iStateId, prcBox, NULL);
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
    if(!bBottom) {
        if(bBody) 
            DrawThemesPart(tabdat, pDC, 9, 0, rcItem);	// TABP_PANE id = 9 
        else { 
            int iStateId = bSel ? 3 : (bHot ? 2 : 1);                       // leftmost item has different part id
            DrawThemesPart(tabdat, pDC, rcItem->left < 20 ? 2 : 1, iStateId, rcItem);
        }
        return;
    }
    
    /*
     * remaining code is for bottom tabs only.
     */
    
    dcMem = CreateCompatibleDC(pDC);
	bmpMem = CreateCompatibleBitmap(pDC, szBmp.cx, szBmp.cy);

    pBmpOld = SelectObject(dcMem, bmpMem);

    rcMem.left = rcMem.top = 0;
    rcMem.right = szBmp.cx;
    rcMem.bottom = szBmp.cy;
    
    ZeroMemory(&biOut,sizeof(BITMAPINFO));	// Fill local pixel arrays
    bihOut = &biOut.bmiHeader;

    bihOut->biSize = sizeof (BITMAPINFOHEADER);
    bihOut->biCompression = BI_RGB;
    bihOut->biPlanes = 1;		  
    bihOut->biBitCount = 24;	// force as RGB: 3 bytes, 24 bits
    bihOut->biWidth = szBmp.cx; 
    bihOut->biHeight = szBmp.cy;

    nBmpWdtPS = DWordAlign(szBmp.cx*3);
    nSzBuffPS = ((nBmpWdtPS * szBmp.cy) / 8 + 2) * 8;

    /*
     * blit the background to the memory dc, so that transparent tabs will draw properly
     * for bottom tabs, it's more complex, because the background part must not be mirrored
     * the body part does not need that (filling with the background color is much faster
     * and sufficient for the tab "page" part.
     */

    if(!bSel)
        FillRect(dcMem, &rcMem, GetSysColorBrush(COLOR_3DFACE));        // only active (bSel == TRUE) tabs can overwrite others. for inactive, it's enough to fill with bg color
    else {
        /*
         * mirror the background horizontally for bottom selected tabs (they can overwrite others)
         * needed, because after drawing the theme part the images will again be mirrored
         * to "flip" the tab item.
         */
        BitBlt(dcMem, 0, 0, szBmp.cx, szBmp.cy, pDC, rcItem->left, rcItem->top, SRCCOPY);

        pcImg1 = malloc(nSzBuffPS);

        if(pcImg1) {
            GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg1, &biOut, DIB_RGB_COLORS);
            bihOut->biHeight = -szBmp.cy; 				// to mirror bitmap is eough to use negative height between Get/SetDIBits
            SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg1, &biOut, DIB_RGB_COLORS);
            free(pcImg1);
        }
    }
    
    if(bBody) 
        DrawThemesPart(tabdat, dcMem, 9, 0, &rcMem);	// TABP_PANE id = 9 
    else { 
        int iStateId = bSel ? 3 : (bHot ? 2 : 1);
        DrawThemesPart(tabdat, dcMem, rcItem->left < 20 ? 2 : 1, iStateId, &rcMem);
    }

    bihOut->biHeight = szBmp.cy;
    pcImg = malloc(nSzBuffPS);
        
    if(pcImg)	{									// get bits: 
		GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
        bihOut->biHeight = -szBmp.cy;
        SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
        free(pcImg);
    }
    
    /*
     * finally, blit the result to the destination dc
     */
    
    BitBlt(pDC, rcItem->left, rcItem->top, szBmp.cx, szBmp.cy, dcMem, 0, 0, SRCCOPY);
	SelectObject(dcMem, pBmpOld);
    DeleteObject(bmpMem);
    DeleteDC(dcMem);
}

static LRESULT CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct TabControlData *tabdat = 0;
    tabdat = (struct TabControlData *)GetWindowLong(hwnd, GWL_USERDATA);
    
    if(tabdat) {
        if(tabdat->pContainer == NULL)
            tabdat->pContainer = (struct ContainerWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);
        tabdat->dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    }
    
    switch(msg) {
        case WM_NCCREATE:
        {
            WNDCLASSEXA wcl = {0};

            wcl.cbSize = sizeof(wcl);
            GetClassInfoExA(g_hInst, "SysTabControl32", &wcl);
            
            tabdat = (struct TabControlData *)malloc(sizeof(struct TabControlData));
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)tabdat);
            ZeroMemory((void *)tabdat, sizeof(struct TabControlData));
            tabdat->hwnd = hwnd;
            tabdat->cx = GetSystemMetrics(SM_CXSMICON);
            tabdat->cy = GetSystemMetrics(SM_CYSMICON);
            SendMessage(hwnd, EM_THEMECHANGED, 0, 0);
            OldTabControlClassProc = wcl.lpfnWndProc;

			//SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | (WS_EX_TRANSPARENT));
			//pSetLayeredWindowAttributes(hwnd, 0, 100, LWA_ALPHA);
            return TRUE;
        }
        case EM_THEMECHANGED:
            tabdat->m_xpad = DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 3);
            tabdat->m_skinning = FALSE;
            if(IsWinVerXPPlus() && myGlobals.m_VSApiEnabled != 0) {
                if(pfnIsThemeActive != 0)
                    if(pfnIsThemeActive()) {
                        tabdat->m_skinning = TRUE;
                        if(tabdat->hTheme != 0 && pfnCloseThemeData != 0) {
                            pfnCloseThemeData(tabdat->hTheme);
                            pfnCloseThemeData(tabdat->hThemeButton);
                        }
                        if(pfnOpenThemeData != 0) {
                            if((tabdat->hTheme = pfnOpenThemeData(hwnd, L"TAB")) == 0 || (tabdat->hThemeButton = pfnOpenThemeData(hwnd, L"BUTTON")) == 0)
                                tabdat->m_skinning = FALSE;
                        }
                    }
            }
            return 0;
        case EM_SEARCHSCROLLER:
        {
            HWND hwndChild;
            /*
             * search the updown control (scroll arrows) to subclass it...
             * the control is dynamically created and may not exist as long as it is
             * not needed. So we have to search it everytime we need to paint. However,
             * it is sufficient to search it once. So this message is called, whenever
             * a new tab is inserted
             */

            if((hwndChild = FindWindowEx(hwnd, 0, _T("msctls_updown32"), NULL)) != 0)
                DestroyWindow(hwndChild);
            return 0;
        }
        case EM_VALIDATEBOTTOM:
            {
                BOOL bClassicDraw = (tabdat->m_skinning == FALSE) || (myGlobals.m_TabAppearance & TCF_NOSKINNING);
                if((tabdat->dwStyle & TCS_BOTTOM) && !bClassicDraw && myGlobals.tabConfig.m_bottomAdjust != 0) {
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                break;
            }
        case TCM_INSERTITEM:
        case TCM_DELETEITEM:
            if(!(tabdat->dwStyle & TCS_MULTILINE) || tabdat->dwStyle & TCS_BUTTONS) {
                LRESULT result;
                RECT rc;
                if(TabCtrl_GetItemCount(hwnd) >= 1 && msg == TCM_INSERTITEM) {
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
            if(tabdat) {
                if(tabdat->hTheme != 0 && pfnCloseThemeData != 0) {
                    pfnCloseThemeData(tabdat->hTheme);
                    pfnCloseThemeData(tabdat->hThemeButton);
                }
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
            int iTabs = TabCtrl_GetItemCount(hwnd);
            
            if(!tabdat->pContainer)
                break;
            
            if(!(tabdat->dwStyle & TCS_MULTILINE)) {
                RECT rcClient, rc;
                DWORD newItemSize;
                if(iTabs > (tabdat->pContainer->dwFlags & CNT_HIDETABS ? 1 : 0)) {
                    GetClientRect(hwnd, &rcClient);
                    TabCtrl_GetItemRect(hwnd, iTabs - 1, &rc);
                    newItemSize = (rcClient.right - 6) - (tabdat->dwStyle & TCS_BUTTONS ? (iTabs) * 10 : 0);
                    newItemSize = newItemSize / iTabs;
                    if(newItemSize < myGlobals.tabConfig.m_fixedwidth) {
                        TabCtrl_SetItemSize(hwnd, newItemSize, rc.bottom - rc.top);
                    }
                    else {
                        TabCtrl_SetItemSize(hwnd, myGlobals.tabConfig.m_fixedwidth, rc.bottom - rc.top);
                    }
                    SendMessage(hwnd, EM_SEARCHSCROLLER, 0, 0);
                }
            }
            else if(tabdat->dwStyle & TCS_BUTTONS && iTabs > 0) {
                RECT rcClient, rcItem;
                int nrTabsPerLine;
                GetClientRect(hwnd, &rcClient);
                TabCtrl_GetItemRect(hwnd, 0, &rcItem);
                nrTabsPerLine = (rcClient.right) / myGlobals.tabConfig.m_fixedwidth;
                if(iTabs >= nrTabsPerLine)
                    TabCtrl_SetItemSize(hwnd, ((rcClient.right - 6) / nrTabsPerLine) - (tabdat->dwStyle & TCS_BUTTONS ? 8 : 0), rcItem.bottom - rcItem.top);
                else
                    TabCtrl_SetItemSize(hwnd, myGlobals.tabConfig.m_fixedwidth, rcItem.bottom - rcItem.top);
            }
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            POINT pt;
            GetCursorPos(&pt);
            SendMessage(GetParent(hwnd), DM_CLOSETABATMOUSE, 0, (LPARAM)&pt);
            break;
        }
        case WM_ERASEBKGND:
			if(tabdat->pContainer->bSkinned)
				return TRUE;
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdcreal, hdc;
            RECT rectTemp, rctPage, rctActive, rcItem, rctClip;
            RECT rectUpDn = {0,0,0,0};
            int nCount = TabCtrl_GetItemCount(hwnd), i;
            TCITEM item = {0};
            int iActive, hotItem;
            POINT pt;
            DWORD dwStyle = tabdat->dwStyle;
            HPEN hPenOld = 0;
            UINT uiFlags = 1;
            UINT uiBottom = 0;
            TCHITTESTINFO hti;
            BOOL bClassicDraw = (tabdat->m_skinning == FALSE) || (myGlobals.m_TabAppearance & TCF_NOSKINNING);
            HBITMAP bmpMem, bmpOld;
            DWORD cx, cy;
            
            hdcreal = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rctPage);
            iActive = TabCtrl_GetCurSel(hwnd);
            TabCtrl_GetItemRect(hwnd, iActive, &rctActive);
            cx = rctPage.right - rctPage.left;
            cy = rctPage.bottom - rctPage.top;
            
            /*
             * draw everything to a memory dc to avoid flickering
             */
            
			hdc = CreateCompatibleDC(hdcreal);
            bmpMem = CreateCompatibleBitmap(hdcreal, cx, cy);

            bmpOld = SelectObject(hdc, bmpMem);

			if(nCount == 1 && tabdat->pContainer->dwFlags & CNT_HIDETABS)
	            rctClip = rctPage;

			if(tabdat->pContainer->bSkinned)
				SkinDrawBG(hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, &rctPage, hdc);
			else
				FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
            
            if(dwStyle & TCS_BUTTONS) {
                RECT rc1;
                TabCtrl_GetItemRect(hwnd, nCount - 1, &rc1);
                if(dwStyle & TCS_BOTTOM) {
                    rctPage.bottom = rc1.top;
                    uiBottom = 8;
                }
                else {
                    rctPage.top = rc1.bottom + 2;
                    uiBottom = 0;
                }
            }
            else {
                if(dwStyle & TCS_BOTTOM) {
                    rctPage.bottom = rctActive.top;
                    uiBottom = 8;
                }
                else {
                    rctPage.top = rctActive.bottom;
                    uiBottom = 0;
                }
            }
            
			if(nCount > 1 || !(tabdat->pContainer->dwFlags & CNT_HIDETABS)) {
				rctClip = rctPage;
				InflateRect(&rctClip, -tabdat->pContainer->tBorder, -tabdat->pContainer->tBorder);
			}

			hPenOld = SelectObject(hdc, myGlobals.tabConfig.m_hPenLight);
            /*
             * visual style support
             */
            
            if(!bClassicDraw && IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
                RECT rcClient = rctPage;
                if(dwStyle & TCS_BOTTOM) {
                    rcClient.bottom = rctPage.bottom;
                    uiFlags |= uiBottom;
                }
                else
                    rcClient.top = rctPage.top;
                //ExcludeClipRect(hdc, rctClip.left, rctClip.top, rctClip.right, rctClip.bottom);
                DrawThemesXpTabItem(hdc, -1, &rcClient, uiFlags, tabdat);	// TABP_PANE=9,0,'TAB'
            }
            else {
                if(IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
					if(tabdat->pContainer->bSkinned) {
						StatusItems_t *item = &StatusItems[ID_EXTBKTABPAGE];

						if(!item->IGNORED) {
							DrawAlpha(hdc, &rctPage, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
									  item->GRADIENT, item->CORNER, item->RADIUS, item->imageItem);
							goto page_done;
						}
					}
                    if(dwStyle & TCS_BUTTONS) {
                        rectTemp = rctPage;
                        if(dwStyle & TCS_BOTTOM) {
                            rectTemp.top--;
                            rectTemp.bottom--;
                        }
                        else {
                            rectTemp.bottom--;
                            rectTemp.top++;
                        }
                        MoveToEx(hdc, rectTemp.left, rectTemp.bottom, &pt);
                        LineTo(hdc, rectTemp.left, rectTemp.top + 1);
                        LineTo(hdc, rectTemp.right - 1, rectTemp.top + 1);
                        SelectObject(hdc, myGlobals.tabConfig.m_hPenShadow);
                        LineTo(hdc, rectTemp.right - 1, rectTemp.bottom);
                        LineTo(hdc, rectTemp.left, rectTemp.bottom);
                    }
                    else {
                        rectTemp = rctPage;

                        MoveToEx(hdc, rectTemp.left, rectTemp.bottom - 1, &pt);
                        LineTo(hdc, rectTemp.left, rectTemp.top);

                        if(dwStyle & TCS_BOTTOM) {
                            LineTo(hdc, rectTemp.right - 1, rectTemp.top);
                            SelectObject(hdc, myGlobals.tabConfig.m_hPenShadow);
                            LineTo(hdc, rectTemp.right - 1, rectTemp.bottom - 1);
                            LineTo(hdc, rctActive.right, rectTemp.bottom - 1);
                            MoveToEx(hdc, rctActive.left - 2, rectTemp.bottom -1, &pt);
                            LineTo(hdc, rectTemp.left - 1, rectTemp.bottom - 1);
                            SelectObject(hdc, myGlobals.tabConfig.m_hPenItemShadow);
                            MoveToEx(hdc, rectTemp.right - 2, rectTemp.top + 1, &pt);
                            LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
                            LineTo(hdc, rctActive.right, rectTemp.bottom - 2);
                            MoveToEx(hdc, rctActive.left - 2, rectTemp.bottom - 2, &pt);
                            LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);
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
            }
page_done:
            uiFlags = 0;
            /*
             * figure out hottracked item (if any)
             */
            GetCursorPos(&hti.pt);
            ScreenToClient(hwnd, &hti.pt);
            hti.flags = 0;
            hotItem = TabCtrl_HitTest(hwnd, &hti);
            for(i = 0; i < nCount; i++) {
                if(i != iActive) {
                    TabCtrl_GetItemRect(hwnd, i, &rcItem);
                    if(!bClassicDraw && uiBottom) {
                        rcItem.top -= myGlobals.tabConfig.m_bottomAdjust;
                        rcItem.bottom -= myGlobals.tabConfig.m_bottomAdjust;
                    }
                    if (IntersectRect(&rectTemp, &rcItem, &ps.rcPaint) || bClassicDraw) {
                        int nHint = 0;
                        if(!bClassicDraw && !(dwStyle & TCS_BUTTONS)) {
                            DrawThemesXpTabItem(hdc, i, &rcItem, uiFlags | uiBottom | (i == hotItem ? 4 : 0), tabdat);
                            DrawItem(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0), i);
                        }
                        else {
                            DrawItemRect(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0));
                            DrawItem(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0), i);
                        }
                    }
                }
            }
            /*
             * draw the active item
             */
            if(!bClassicDraw && uiBottom) {
                rctActive.top -= myGlobals.tabConfig.m_bottomAdjust;
                rctActive.bottom -= myGlobals.tabConfig.m_bottomAdjust;
            }
            if (rctActive.left >= 0) {
                int nHint = 0;
                rcItem = rctActive;
                if(!bClassicDraw && !(dwStyle & TCS_BUTTONS)) {
                    InflateRect(&rcItem, 2, 2);
                    DrawThemesXpTabItem(hdc, iActive, &rcItem, 2 | uiBottom, tabdat);
                    DrawItem(tabdat, hdc, &rcItem, nHint | HINT_ACTIVE_ITEM, iActive);
                }
                else {
                    if(!(dwStyle & TCS_BUTTONS)) {
                        if(iActive == 0) {
                            rcItem.right+=2;
                            rcItem.left--;
                        }
                        else
                            InflateRect(&rcItem, 2, 0);
                    }
                    DrawItemRect(tabdat, hdc, &rcItem, HINT_ACTIVATE_RIGHT_SIDE|HINT_ACTIVE_ITEM | nHint);
                    DrawItem(tabdat, hdc, &rcItem, HINT_ACTIVE_ITEM | nHint, iActive);
                }
            }
            if(hPenOld)
                SelectObject(hdc, hPenOld);

            /*
             * finally, bitblt the contents of the memory dc to the real dc
             */
			//if(!tabdat->pContainer->bSkinned)
			ExcludeClipRect(hdcreal, rctClip.left, rctClip.top, rctClip.right, rctClip.bottom);
            BitBlt(hdcreal, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, bmpOld);
            DeleteObject(bmpMem);
            DeleteDC(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            short amount = (short)(HIWORD(wParam));
            if(lParam != -1)
                break;
            if(amount > 0)
                SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_PREV, 0);
            else if(amount < 0)
                SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_NEXT, 0);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
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
    BOOL iLabelDefault = myGlobals.m_TabAppearance & TCF_LABELUSEWINCOLORS;
    BOOL iBkgDefault = myGlobals.m_TabAppearance & TCF_BKGUSEWINCOLORS;
    
    myGlobals.tabConfig.m_hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
    myGlobals.tabConfig.m_hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
    myGlobals.tabConfig.m_hPenItemShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

    nclim.cbSize = sizeof(nclim);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nclim, 0);
    myGlobals.tabConfig.m_hMenuFont = CreateFontIndirect(&nclim.lfMessageFont);

    while(tabcolors[i].szKey != NULL) {
        if(i < 4)
            myGlobals.tabConfig.colors[i] = iLabelDefault ? GetSysColor(tabcolors[i].defclr) : DBGetContactSettingDword(NULL, SRMSGMOD_T, g_skinnedContainers ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
        else
            myGlobals.tabConfig.colors[i] = iBkgDefault ? GetSysColor(tabcolors[i].defclr) :  DBGetContactSettingDword(NULL, SRMSGMOD_T, g_skinnedContainers ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
        i++;
    }

    myGlobals.tabConfig.m_hBrushDefault = CreateSolidBrush(myGlobals.tabConfig.colors[4]);
    myGlobals.tabConfig.m_hBrushActive = CreateSolidBrush(myGlobals.tabConfig.colors[5]);
    myGlobals.tabConfig.m_hBrushUnread = CreateSolidBrush(myGlobals.tabConfig.colors[6]);
    myGlobals.tabConfig.m_hBrushHottrack = CreateSolidBrush(myGlobals.tabConfig.colors[7]);
    
    myGlobals.tabConfig.m_bottomAdjust = (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "bottomadjust", 0);
    myGlobals.tabConfig.m_fixedwidth = DBGetContactSettingDword(NULL, SRMSGMOD_T, "fixedwidth", FIXED_TAB_SIZE);
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
    DeleteObject(myGlobals.tabConfig.m_hBrushHottrack);
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

            TranslateDialogDefault(hwndDlg);
            
            SetWindowTextA(hwndDlg, Translate("Configure tab appearance"));
            CheckDlgButton(hwndDlg, IDC_FLATTABS2, dwFlags & TCF_FLAT);
            CheckDlgButton(hwndDlg, IDC_FLASHICON, dwFlags & TCF_FLASHICON);
            CheckDlgButton(hwndDlg, IDC_FLASHLABEL, dwFlags & TCF_FLASHLABEL);
            CheckDlgButton(hwndDlg, IDC_NOSKINNING, dwFlags & TCF_NOSKINNING);
            CheckDlgButton(hwndDlg, IDC_SINGLEROWTAB, dwFlags & TCF_SINGLEROWTABCONTROL);
            CheckDlgButton(hwndDlg, IDC_LABELUSEWINCOLORS, !g_skinnedContainers && dwFlags & TCF_LABELUSEWINCOLORS);
            CheckDlgButton(hwndDlg, IDC_BKGUSEWINCOLORS2, !g_skinnedContainers && dwFlags & TCF_BKGUSEWINCOLORS);
            
            SendMessage(hwndDlg, WM_COMMAND, IDC_LABELUSEWINCOLORS, 0);
            
            if(myGlobals.m_VSApiEnabled == 0) {
                CheckDlgButton(hwndDlg, IDC_NOSKINNING, TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_NOSKINNING), FALSE);
            }
            while(tabcolors[i].szKey != NULL) {
                clr = (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, g_skinnedContainers ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
                SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_SETCOLOUR, 0, (LPARAM)clr);
                i++;
            }

            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder" : "tborder", 2));
            SetDlgItemInt(hwndDlg, IDC_TABBORDER, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder" : "tborder", 2), FALSE);;

            SendDlgItemMessage(hwndDlg, IDC_BOTTOMTABADJUSTSPIN, UDM_SETRANGE, 0, MAKELONG(3, -3));
            SendDlgItemMessage(hwndDlg, IDC_BOTTOMTABADJUSTSPIN, UDM_SETPOS, 0, myGlobals.tabConfig.m_bottomAdjust);
            SetDlgItemInt(hwndDlg, IDC_BOTTOMTABADJUST, myGlobals.tabConfig.m_bottomAdjust, TRUE);

            SendDlgItemMessage(hwndDlg, IDC_TABWIDTHSPIN, UDM_SETRANGE, 0, MAKELONG(400, 50));
            SendDlgItemMessage(hwndDlg, IDC_TABWIDTHSPIN, UDM_SETPOS, 0, myGlobals.tabConfig.m_fixedwidth);
            SetDlgItemInt(hwndDlg, IDC_TABWIDTH, myGlobals.tabConfig.m_fixedwidth, TRUE);
            
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTER, UDM_SETRANGE, 0, MAKELONG(30, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERRIGHT, UDM_SETRANGE, 0, MAKELONG(30, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERTOP, UDM_SETRANGE, 0, MAKELONG(40, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERBOTTOM, UDM_SETRANGE, 0, MAKELONG(10, 0));

			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTER, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_left" : "tborder_outer_left", 2));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERRIGHT, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_right" : "tborder_outer_right", 2));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERTOP, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_top" : "tborder_outer_top", 2));
			SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERBOTTOM, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_bottom" : "tborder_outer_bottom", 2));

            SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(10, 1));
            SendDlgItemMessage(hwndDlg, IDC_SPIN3, UDM_SETRANGE, 0, MAKELONG(10, 1));
            SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3));
            SendDlgItemMessage(hwndDlg, IDC_SPIN3, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 4));
            SetDlgItemInt(hwndDlg, IDC_TABPADDING, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3), FALSE);;
            SetDlgItemInt(hwndDlg, IDC_HTABPADDING, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 4), FALSE);;
            EnableWindow(GetDlgItem(hwndDlg, IDC_NOSKINNING), g_skinnedContainers ? FALSE : TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_LABELUSEWINCOLORS), g_skinnedContainers ? FALSE : TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BKGUSEWINCOLORS2), g_skinnedContainers ? FALSE : TRUE);
            
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LABELUSEWINCOLORS:
                case IDC_BKGUSEWINCOLORS2:
                {
                    int i = 0;
                    BOOL iLabel = IsDlgButtonChecked(hwndDlg, IDC_LABELUSEWINCOLORS);
                    BOOL iBkg = IsDlgButtonChecked(hwndDlg, IDC_BKGUSEWINCOLORS2);
                    while(tabcolors[i].szKey != NULL) {
                        if(i < 4)
                            EnableWindow(GetDlgItem(hwndDlg, tabcolors[i].id), iLabel ? FALSE : TRUE);
                        else
                            EnableWindow(GetDlgItem(hwndDlg, tabcolors[i].id), iBkg ? FALSE : TRUE);
                        i++;
                    }
                    break;
                }
                case IDOK:
                {
                    int i = 0;
                    COLORREF clr;
                    BOOL translated;
                    
                    struct ContainerWindowData *pContainer = pFirstContainer;
                    
                    DWORD dwFlags = (IsDlgButtonChecked(hwndDlg, IDC_FLATTABS2) ? TCF_FLAT : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_FLASHICON) ? TCF_FLASHICON : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_FLASHLABEL) ? TCF_FLASHLABEL : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_SINGLEROWTAB) ? TCF_SINGLEROWTABCONTROL : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_LABELUSEWINCOLORS) ? TCF_LABELUSEWINCOLORS : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_BKGUSEWINCOLORS2) ? TCF_BKGUSEWINCOLORS : 0) |
                                    (IsDlgButtonChecked(hwndDlg, IDC_NOSKINNING) ? TCF_NOSKINNING : 0);
                    
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", dwFlags);
                    myGlobals.m_TabAppearance = dwFlags;
                    while(tabcolors[i].szKey != NULL) {
                        clr = SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_GETCOLOUR, 0, 0);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, g_skinnedContainers ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, clr);
                        i++;
                    }
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "y-pad", GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "x-pad", GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder" : "tborder", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDER, &translated, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_left" : "tborder_outer_left", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTER, &translated, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_right" : "tborder_outer_right", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERRIGHT, &translated, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_top" : "tborder_outer_top", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERTOP, &translated, FALSE));
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_bottom" : "tborder_outer_bottom", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERBOTTOM, &translated, FALSE));
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "bottomadjust", GetDlgItemInt(hwndDlg, IDC_BOTTOMTABADJUST, &translated, TRUE));
                    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "fixedwidth", GetDlgItemInt(hwndDlg, IDC_TABWIDTH, &translated, FALSE));
                    
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
