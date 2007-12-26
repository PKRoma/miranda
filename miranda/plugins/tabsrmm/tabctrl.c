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

#define CPLUSPLUS
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
extern ButtonSet g_ButtonSet;

static LRESULT CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK StatusBarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void GetIconSize(HICON hIcon, int* sizeX, int* sizeY);
extern void __fastcall IMG_RenderImageItem(HDC hdc, ImageItem *item, RECT *rc);

HMODULE hUxTheme = 0;

// function pointers, use typedefs for casting to shut up the compiler when using GetProcAddress()

PITA pfnIsThemeActive = 0;
POTD pfnOpenThemeData = 0;
PDTB pfnDrawThemeBackground = 0;
PCTD pfnCloseThemeData = 0;
PDTT pfnDrawThemeText = 0;
PITBPT pfnIsThemeBackgroundPartiallyTransparent = 0;
PDTPB  pfnDrawThemeParentBackground = 0;
PGTBCR pfnGetThemeBackgroundContentRect = 0;
BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD) = 0;

#define FIXED_TAB_SIZE 100                  // default value for fixed width tabs

/*
 * visual styles support (XP+)
 * returns 0 on failure
 */

int InitVSApi()
{
    if(!IsWinVerXPPlus())
        return 0;

    if((hUxTheme = LoadLibraryA("uxtheme.dll")) == 0)
        return 0;

    pfnIsThemeActive = (PITA)GetProcAddress(hUxTheme, "IsThemeActive");
    pfnOpenThemeData = (POTD)GetProcAddress(hUxTheme, "OpenThemeData");
    pfnDrawThemeBackground = (PDTB)GetProcAddress(hUxTheme, "DrawThemeBackground");
    pfnCloseThemeData = (PCTD)GetProcAddress(hUxTheme, "CloseThemeData");
    pfnDrawThemeText = (PDTT)GetProcAddress(hUxTheme, "DrawThemeText");
    pfnIsThemeBackgroundPartiallyTransparent = (PITBPT)GetProcAddress(hUxTheme, "IsThemeBackgroundPartiallyTransparent");
    pfnDrawThemeParentBackground = (PDTPB)GetProcAddress(hUxTheme, "DrawThemeParentBackground");
    pfnGetThemeBackgroundContentRect = (PGTBCR)GetProcAddress(hUxTheme, "GetThemeBackgroundContentRect");

    MyEnableThemeDialogTexture = (BOOL (WINAPI *)(HANDLE, DWORD))GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
    if(pfnIsThemeActive != 0 && pfnOpenThemeData != 0 && pfnDrawThemeBackground != 0 && pfnCloseThemeData != 0 
       && pfnDrawThemeText != 0 && pfnIsThemeBackgroundPartiallyTransparent != 0 && pfnDrawThemeParentBackground != 0
       && pfnGetThemeBackgroundContentRect != 0) {
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
    wce.cbWndExtra     = 4;
    wce.hbrBackground  = 0;
    wce.style          = CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC;
    RegisterClassEx(&wce);
	return 0;
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
*/

static void DrawWuLine( HDC pDC, int X0, int Y0, int X1, int Y1, COLORREF clrLine )
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
        int Temp = Y0; Y0 = Y1; Y1 = Temp;
        Temp = X0; X0 = X1; X1 = Temp;
    }
    
    SetPixel(pDC, X0, Y0, clrLine);
    
    DeltaX = X1 - X0;
    if(DeltaX >= 0)
        XDir = 1;
    else {
        XDir   = -1;
        DeltaX = 0 - DeltaX;
    }
    
    DeltaY = Y1 - Y0;

    if(DeltaY == 0) {
        while (DeltaX-- != 0) {
            X0 += XDir;
            SetPixel(pDC, X0, Y0, clrLine);
        }
        return;
    }
    if(DeltaX == 0) {
        do {
            Y0++;
            SetPixel(pDC, X0, Y0, clrLine);
        } while (--DeltaY != 0);
        return;
    }
    
    if(DeltaX == DeltaY) {
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
    
    if(DeltaY > DeltaX) {
        ErrorAdj = (unsigned short)(((unsigned long) DeltaX << 16) / (unsigned long) DeltaY);

        while(--DeltaY) {
            ErrorAccTemp = ErrorAcc;
            ErrorAcc += ErrorAdj;
            if (ErrorAcc <= ErrorAccTemp)
                X0 += XDir;
            Y0++;
            Weighting = ErrorAcc >> 8;
            
            clrBackGround = GetPixel( pDC, X0, Y0 );
            rb = GetRValue( clrBackGround );
            gb = GetGValue( clrBackGround );
            bb = GetBValue( clrBackGround );
            grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;
            
            rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:
		      (Weighting ^ 255)) ) / 255.0 * ( rb - rl ) + rl ) ) :
		      ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) )
		      / 255.0 * ( rl - rb ) + rb ) ) );
            gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:
		      (Weighting ^ 255)) ) / 255.0 * ( gb - gl ) + gl ) ) :
		      ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) )
		      / 255.0 * ( gl - gb ) + gb ) ) );
            br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:
		      (Weighting ^ 255)) ) / 255.0 * ( bb - bl ) + bl ) ) :
		      ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) )
		      / 255.0 * ( bl - bb ) + bb ) ) );
            SetPixel(pDC, X0, Y0, RGB( rr, gr, br ));
            
            clrBackGround = GetPixel(pDC, X0 + XDir, Y0);
            rb = GetRValue(clrBackGround);
            gb = GetGValue(clrBackGround);
            bb = GetBValue(clrBackGround);
            grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;
            
            rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):
		Weighting) ) / 255.0 * ( rb - rl ) + rl ) ) :
		( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) )
		/ 255.0 * ( rl - rb ) + rb ) ) );
            gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):
		Weighting) ) / 255.0 * ( gb - gl ) + gl ) ) :
		( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) )
		/ 255.0 * ( gl - gb ) + gb ) ) );
            br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):
		Weighting) ) / 255.0 * ( bb - bl ) + bl ) ) :
		( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) )
		/ 255.0 * ( bl - bb ) + bb ) ) );
            SetPixel(pDC, X0 + XDir, Y0, RGB( rr, gr, br ));
        }
        SetPixel(pDC, X1, Y1, clrLine);
        return;
    }
    ErrorAdj = (unsigned short)(((unsigned long) DeltaY << 16) / (unsigned long) DeltaX);

    while(--DeltaX) {
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
        
        rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:
		(Weighting ^ 255)) ) / 255.0 * ( rb - rl ) + rl ) ) : 
		( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) 
		/ 255.0 * ( rl - rb ) + rb ) ) );
        gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:
		(Weighting ^ 255)) ) / 255.0 * ( gb - gl ) + gl ) ) : 
		( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) 
		/ 255.0 * ( gl - gb ) + gb ) ) );
        br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:
		(Weighting ^ 255)) ) / 255.0 * ( bb - bl ) + bl ) ) : 
		( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) 
		/ 255.0 * ( bl - bb ) + bb ) ) );
        
        SetPixel(pDC, X0, Y0, RGB(rr, gr, br));
        
        clrBackGround = GetPixel(pDC, X0, Y0 + 1);
        rb = GetRValue(clrBackGround);
        gb = GetGValue(clrBackGround);
        bb = GetBValue(clrBackGround);
        grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;
        
        rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):
		Weighting) ) / 255.0 * ( rb - rl ) + rl ) ) : 
		( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) 
		/ 255.0 * ( rl - rb ) + rb ) ) );
        gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):
		Weighting) ) / 255.0 * ( gb - gl ) + gl ) ) : 
		( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) 
		/ 255.0 * ( gl - gb ) + gb ) ) );
        br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):
		Weighting) ) / 255.0 * ( bb - bl ) + bl ) ) : 
		( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) 
		/ 255.0 * ( bl - bb ) + bb ) ) );
        
        SetPixel(pDC, X0, Y0 + 1, RGB( rr, gr, br ));
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
    struct MessageWindowData *dat = 0;
    int iSize = 16;
    DWORD dwTextFlags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
    BOOL leftMost = FALSE;

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

        if(tabdat->m_moderntabs && rcItem->left < 10) {
            leftMost = TRUE;
            rcItem->left += 10;
        }

        if(!(dwStyle & TCS_BOTTOM))
            OffsetRect(rcItem, 0, 1);

        if(dat->dwFlags & MWF_ERRORSTATE)
            hIcon = myGlobals.g_iconErr;
        else if(dat->mayFlashTab)
            hIcon = dat->iFlashIcon;
		else {
            if(dat->si && dat->iFlashIcon) {
                int sizeX, sizeY;

                hIcon = dat->iFlashIcon;
                GetIconSize(hIcon, &sizeX, &sizeY);
                iSize = sizeX;
            }
			else if(dat->hTabIcon == dat->hTabStatusIcon && dat->hXStatusIcon)
				hIcon = dat->hXStatusIcon;
			else
				hIcon = dat->hTabIcon;
		}

        if(dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0) || !(myGlobals.m_TabAppearance & TCF_FLASHICON)) {
            DWORD ix = rcItem->left + tabdat->m_xpad - 1;
            DWORD iy = (rcItem->bottom + rcItem->top - iSize) / 2;
            if(dat->dwFlagsEx & MWF_SHOW_ISIDLE && myGlobals.m_IdleDetect)
                DrawDimmedIcon(dc, ix, iy, iSize, iSize, hIcon, 180);
            else
                DrawIconEx (dc, ix, iy, hIcon, iSize, iSize, 0, NULL, DI_NORMAL | DI_COMPAT); 
        }

        rcItem->left += (iSize + 2 + tabdat->m_xpad);
        
        if(dat->mayFlashTab == FALSE || (dat->mayFlashTab == TRUE && dat->bTabFlash != 0) || !(myGlobals.m_TabAppearance & TCF_FLASHLABEL)) {
            oldFont = (HFONT)SelectObject(dc, (HFONT)SendMessage(tabdat->hwnd, WM_GETFONT, 0, 0));
            if(tabdat->dwStyle & TCS_BUTTONS || !(tabdat->dwStyle & TCS_MULTILINE)) { // || (tabdat->m_moderntabs && leftMost)) {
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


static RECT rcTabPage = {0};

static void DrawItemRect(struct TabControlData *tabdat, HDC dc, RECT *rcItem, int nHint, int iItem)
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
            if(dwStyle & TCS_BOTTOM)
                rcItem->top++;
            else
                rcItem->bottom--;
            
            rcItem->right += 6;
            if(bClassicDraw) {
                if(tabdat->pContainer->bSkinned) {
                    StatusItems_t *item = nHint & HINT_ACTIVE_ITEM ? &StatusItems[ID_EXTBKBUTTONSPRESSED] : (nHint & HINT_HOTTRACK ? &StatusItems[ID_EXTBKBUTTONSMOUSEOVER] : &StatusItems[ID_EXTBKBUTTONSNPRESSED]);

                    if(!item->IGNORED) {
                        SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
                        DrawAlpha(dc, rcItem, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
                                  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
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
                /*
				if(tabdat->pContainer->bSkinned)
					SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				else
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                */
                if(!tabdat->pContainer->bSkinned && !tabdat->m_moderntabs)
                    FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                //else if(tabdat->pContainer->bSkinned)
                //    SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
                rcItem->bottom +=2;
            }
            else {
                rcItem->bottom += 2;
                /*
				if(tabdat->pContainer->bSkinned)
					SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
				else
					FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                */
                if(!tabdat->pContainer->bSkinned && !tabdat->m_moderntabs)
                    FillRect(dc, rcItem, GetSysColorBrush(COLOR_3DFACE));
                //else if(tabdat->pContainer->bSkinned)
                //    SkinDrawBG(tabdat->hwnd, tabdat->pContainer->hwnd, tabdat->pContainer, rcItem, dc);
                rcItem->bottom--;
                rcItem->top -=2;
            }
			if(tabdat->pContainer->bSkinned && !tabdat->m_moderntabs) {
				StatusItems_t *item = &StatusItems[dwStyle & TCS_BOTTOM ? ID_EXTBKTABITEMACTIVEBOTTOM : ID_EXTBKTABITEMACTIVE];
				if(!item->IGNORED) {
                    rcItem->left += item->MARGIN_LEFT; rcItem->right -= item->MARGIN_RIGHT;
                    rcItem->top += item->MARGIN_TOP; rcItem->bottom -= item->MARGIN_BOTTOM;
					DrawAlpha(dc, rcItem, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
					return;
				}
			}
        }
		if(tabdat->pContainer->bSkinned && !tabdat->m_moderntabs) {
			StatusItems_t *item = &StatusItems[dwStyle & TCS_BOTTOM ? (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACKBOTTOM : ID_EXTBKTABITEMBOTTOM) : 
                                               (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACK : ID_EXTBKTABITEM)];
			if(!item->IGNORED) {
                if(dwStyle & TCS_BOTTOM)
                    rcItem->top = (rcItem->top > rcTabPage.bottom + 5) ? --rcItem->top : rcItem->top;
                else
                    rcItem->bottom++;
                    //rcItem->bottom = (rcItem->bottom < rcTabPage.top - 5) ? ++rcItem->bottom : rcItem->bottom;

                rcItem->left += item->MARGIN_LEFT; rcItem->right -= item->MARGIN_RIGHT;
				DrawAlpha(dc, rcItem, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
						  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
				return;
			}
		}
        if(dwStyle & TCS_BOTTOM) {
            if(tabdat->m_moderntabs) {
                RECT rc = *rcItem;
                POINT pt[5], pts;
                BOOL active = nHint & HINT_ACTIVE_ITEM;
                HRGN rgn;
                TCITEM item = {0};
                struct MessageWindowData *dat = 0;
                HBRUSH bg;
                StatusItems_t *s_item;
                int item_id;

                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(tabdat->hwnd, iItem, &item);

                /*
                 * get the message window data for the session to which this tab item belongs
                 */

                if(IsWindow((HWND)item.lParam) && item.lParam != 0)
                    dat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);

                if(active && rc.left > 10)
                    rc.left -= 10;

                if(active) {
                    rc.bottom--;
                    //rc.right -1;
                }
                rc.right--;

                item_id = active ? ID_EXTBKTABITEMACTIVEBOTTOM : (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACKBOTTOM : ID_EXTBKTABITEMBOTTOM);
                s_item = &StatusItems[item_id];

                pt[0].x = rc.left; pt[0].y = rc.top - (active ? 1 : 0);
                if(active || rc.left < 10) {
                    pt[1].x = rc.left + 10; pt[1].y = rc.bottom - 4;
                    pt[2].x = rc.left + 16; pt[2].y = rc.bottom;
                } else {
                    pt[1].x = rc.left; pt[1].y = rc.bottom - 6;
                    pt[2].x = rc.left + 6; pt[2].y = rc.bottom;
                }
                pt[3].x = rc.right; pt[3].y = rc.bottom;
                pt[4].x = rc.right; pt[4].y = rc.top - (active ? 1 : 0);
                rgn = CreatePolygonRgn(pt, 5, WINDING);
                if(!s_item->IGNORED) {
                    if(active)
                        rc.top--;
                    SelectClipRgn(dc, rgn);
                    DrawAlpha(dc, &rc, s_item->COLOR, s_item->ALPHA, s_item->COLOR2, s_item->COLOR2_TRANSPARENT, s_item->GRADIENT,
                              s_item->CORNER, s_item->BORDERSTYLE, s_item->imageItem);
                    SelectClipRgn(dc, 0);
                } else if(dat) {
                    if(dat->mayFlashTab == TRUE)
                        bg = myGlobals.tabConfig.m_hBrushUnread;
                    else if(nHint & HINT_ACTIVE_ITEM)
                        bg = myGlobals.tabConfig.m_hBrushActive;
                    else if(nHint & HINT_HOTTRACK)
                        bg = myGlobals.tabConfig.m_hBrushHottrack;
                    else
                        bg = myGlobals.tabConfig.m_hBrushDefault;
                    FillRgn(dc, rgn, bg);
                }
                SelectObject(dc, myGlobals.tabConfig.m_hPenStyledDark);
                if(active || rc.left < 10) {
                    DrawWuLine(dc, pt[0].x, pt[0].y, pt[1].x, pt[1].y, myGlobals.tabConfig.colors[9]);
                    DrawWuLine(dc, pt[1].x, pt[1].y, pt[2].x, pt[2].y, myGlobals.tabConfig.colors[9]);
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
                }
                else {
                    MoveToEx(dc, pt[1].x, pt[1].y, &pts);
                    LineTo(dc, pt[2].x, pt[2].y);
                }
                LineTo(dc, pt[3].x, pt[3].y);
                LineTo(dc, pt[4].x, pt[4].y - 1);

                SelectObject(dc, myGlobals.tabConfig.m_hPenStyledLight);
                MoveToEx(dc, pt[4].x - 1, pt[4].y, &pts);
                LineTo(dc, pt[3].x - 1, pt[3].y - 1);
                LineTo(dc, pt[2].x, pt[2].y - 1);
                if(active || rc.left < 10) {
                    /*
                    short basecolor;
                    COLORREF clr = GetSysColor(COLOR_3DHILIGHT);
                    basecolor = (GetRValue(clr) + GetBValue(clr) + GetGValue(clr)) / 3;
                    DrawWuLineBW(dc, pt[2].x, pt[2].y - 1, pt[1].x, pt[1].y - 1, basecolor, 256, 8);
                    DrawWuLineBW(dc, pt[1].x, pt[1].y - 1, pt[0].x + 1, pt[0].y, basecolor, 256, 8);
                    */
                    DrawWuLine(dc, pt[2].x, pt[2].y - 1, pt[1].x + 1, pt[1].y - 1, myGlobals.tabConfig.colors[8]);
                    DrawWuLine(dc, pt[1].x + 1, pt[1].y - 1, pt[0].x + 1, pt[0].y, myGlobals.tabConfig.colors[8]);
                    //LineTo(dc, pt[1].x, pt[1].y - 1);
                    //LineTo(dc, pt[0].x + 1, pt[0].y);
                    if(rc.top > rcTabPage.bottom + 10 && !active) {
                        DrawWuLine(dc, pt[0].x, pt[0].y - 1, pt[4].x, pt[4].y - 1, myGlobals.tabConfig.colors[9]);
                        DrawWuLine(dc, pt[0].x + 1, pt[0].y, pt[4].x - 1, pt[4].y, myGlobals.tabConfig.colors[8]);
                    }
                }
                else
                    LineTo(dc, pt[1].x, pt[1].y - 1);
                DeleteObject(rgn);
            } else {
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
        }
        else {
            if(tabdat->m_moderntabs) {
                RECT rc = *rcItem;
                POINT pt[5], pts;
                BOOL active = nHint & HINT_ACTIVE_ITEM;
                HRGN rgn;
                TCITEM item = {0};
                struct MessageWindowData *dat = 0;
                HBRUSH bg;
                LONG bendPoint;
                StatusItems_t *s_item;
                int item_id;

                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(tabdat->hwnd, iItem, &item);

                if(IsWindow((HWND)item.lParam) && item.lParam != 0)
                    dat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);

                if(active && rc.left > 10)
                    rc.left -= 10;

                if(active)
                    rc.top++;

                item_id = active ? ID_EXTBKTABITEMACTIVE : (nHint & HINT_HOTTRACK ? ID_EXTBKTABITEMHOTTRACK : ID_EXTBKTABITEM);
                s_item = &StatusItems[item_id];

                rc.right--;

                pt[0].x = rc.left; pt[0].y = rc.bottom + (active ? 1 : 0);
                bendPoint = (rc.bottom - rc.top) - 4;

                if(active || rc.left < 10) {
                    pt[1].x = rc.left + 10; pt[1].y = rc.top + 4;
                    pt[2].x = rc.left + 16; pt[2].y = rc.top;
                } else {
                    pt[1].x = rc.left; pt[1].y = rc.top + 6;
                    pt[2].x = rc.left + 6; pt[2].y = rc.top;
                }
                pt[3].x = rc.right; pt[3].y = rc.top;
                pt[4].x = rc.right; pt[4].y = rc.bottom + 1;
                rgn = CreatePolygonRgn(pt, 5, WINDING);
                if(!s_item->IGNORED) {
                    if(active)
                        rc.bottom++;
                    SelectClipRgn(dc, rgn);
                    DrawAlpha(dc, &rc, s_item->COLOR, s_item->ALPHA, s_item->COLOR2, s_item->COLOR2_TRANSPARENT, s_item->GRADIENT,
                              s_item->CORNER, s_item->BORDERSTYLE, s_item->imageItem);
                    SelectClipRgn(dc, 0);
                } else if(dat) {
                    if(dat->mayFlashTab == TRUE)
                        bg = myGlobals.tabConfig.m_hBrushUnread;
                    else if(nHint & HINT_ACTIVE_ITEM)
                        bg = myGlobals.tabConfig.m_hBrushActive;
                    else if(nHint & HINT_HOTTRACK)
                        bg = myGlobals.tabConfig.m_hBrushHottrack;
                    else
                        bg = myGlobals.tabConfig.m_hBrushDefault;
                    FillRgn(dc, rgn, bg);
                }
                SelectObject(dc, myGlobals.tabConfig.m_hPenStyledDark);
                if(active || rc.left < 10) {
                    //MoveToEx(dc, pt[0].x, pt[0].y - (active ? 1 : 0), &pts);
                    //LineTo(dc, pt[1].x, pt[1].y);
                    //LineTo(dc, pt[2].x, pt[2].y);
                    DrawWuLine(dc, pt[0].x, pt[0].y - (active ? 1 : 0), pt[1].x, pt[1].y, myGlobals.tabConfig.colors[9]);
                    DrawWuLine(dc, pt[1].x, pt[1].y, pt[2].x, pt[2].y, myGlobals.tabConfig.colors[9]);
                    MoveToEx(dc, pt[2].x, pt[2].y, &pts);
                }
                else {
                    MoveToEx(dc, pt[1].x, pt[1].y, &pts);
                    LineTo(dc, pt[2].x, pt[2].y);
                }
                LineTo(dc, pt[3].x, pt[3].y);
                LineTo(dc, pt[4].x, pt[4].y);

                SelectObject(dc, myGlobals.tabConfig.m_hPenStyledLight);
                MoveToEx(dc, pt[4].x - 1, pt[4].y - 1, &pts);
                LineTo(dc, pt[3].x - 1, pt[3].y + 1);
                LineTo(dc, pt[2].x, pt[2].y + 1);
                if(active || rc.left < 10) {
                    //LineTo(dc, pt[1].x, pt[1].y + 1);
                    //LineTo(dc, pt[0].x, pt[0].y);
                    DrawWuLine(dc, pt[2].x, pt[2].y + 1, pt[1].x + 1, pt[1].y + 1, myGlobals.tabConfig.colors[8]);
                    DrawWuLine(dc, pt[1].x + 1, pt[1].y + 1, pt[0].x + 1, pt[0].y, myGlobals.tabConfig.colors[8]);

                    if(rc.bottom < rcTabPage.top - 10 && !active) {
                        DrawWuLine(dc, pt[0].x, pt[0].y + 1, pt[4].x, pt[4].y + 1, myGlobals.tabConfig.colors[9]);
                        DrawWuLine(dc, pt[0].x + 1, pt[0].y, pt[4].x - 1, pt[4].y, myGlobals.tabConfig.colors[8]);
                    }
                }
                else
                    LineTo(dc, pt[1].x, pt[1].y + 1);
                DeleteObject(rgn);
            } else {
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

    pBmpOld = (HBITMAP)SelectObject(dcMem, bmpMem);

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

        pcImg1 = (BYTE *)malloc(nSzBuffPS);

        if(pcImg1) {
            GetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg1, &biOut, DIB_RGB_COLORS);
            bihOut->biHeight = -szBmp.cy; 				// to mirror bitmap is eough to use negative height between Get/SetDIBits
            SetDIBits(pDC, bmpMem, nStart, szBmp.cy - nLenSub, pcImg1, &biOut, DIB_RGB_COLORS);
            free(pcImg1);
        }
    }
    
    /*
     * body may be *large* so rotating the final image can be very slow.
     * workaround: draw the skin item (tab pane) into a small dc, rotate this (small) image and render
     * it to the final DC with the IMG_RenderItem() routine.
     */

    if(bBody) {
        HDC hdcTemp = CreateCompatibleDC(pDC);
        HBITMAP hbmTemp = CreateCompatibleBitmap(pDC, 100, 50);
        HBITMAP hbmTempOld = SelectObject(hdcTemp, hbmTemp);
        RECT rcTemp = {0};
        ImageItem tempItem = {0};

        rcTemp.right = 100;
        rcTemp.bottom = 50;

        bihOut->biWidth = 100;
        bihOut->biHeight = 50;

        nBmpWdtPS = DWordAlign(100 * 3);
        nSzBuffPS = ((nBmpWdtPS * 50) / 8 + 2) * 8;

        FillRect(hdcTemp, &rcTemp, GetSysColorBrush(COLOR_3DFACE));
        DrawThemesPart(tabdat, hdcTemp, 9, 0, &rcTemp);	// TABP_PANE id = 9 
        pcImg = (BYTE *)malloc(nSzBuffPS);
        if(pcImg)	{									// get bits: 
            GetDIBits(hdcTemp, hbmTemp, nStart, 50 - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
            bihOut->biHeight = -50;
            SetDIBits(hdcTemp, hbmTemp, nStart, 50 - nLenSub, pcImg, &biOut, DIB_RGB_COLORS);
            free(pcImg);
        }
        tempItem.bBottom = tempItem.bLeft = tempItem.bTop = tempItem.bRight = 10;
        tempItem.hdc = hdcTemp;
        tempItem.hbm = hbmTemp;
        tempItem.dwFlags = IMAGE_FLAG_DIVIDED | IMAGE_FILLSOLID;
        tempItem.fillBrush = GetSysColorBrush(COLOR_3DFACE);
        tempItem.bf.SourceConstantAlpha = 255;
        tempItem.inner_height = 30; tempItem.inner_width = 80;
        tempItem.height = 50; tempItem.width = 100;
        IMG_RenderImageItem(pDC, &tempItem, rcItem);
        SelectObject(hdcTemp, hbmTempOld);
        DeleteObject(hbmTemp);
        DeleteDC(hdcTemp);

        SelectObject(dcMem, pBmpOld);
        DeleteObject(bmpMem);
        DeleteDC(dcMem);
        return;
    }
    else { 
        int iStateId = bSel ? 3 : (bHot ? 2 : 1);
        DrawThemesPart(tabdat, dcMem, rcItem->left < 20 ? 2 : 1, iStateId, &rcMem);
    }

    bihOut->biHeight = szBmp.cy;
    pcImg = (BYTE *)malloc(nSzBuffPS);
        
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

static POINT ptMouseT = {0};

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
            tabdat->fTipActive = FALSE;
            SendMessage(hwnd, EM_THEMECHANGED, 0, 0);
            OldTabControlClassProc = wcl.lpfnWndProc;
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
                if((tabdat->dwStyle & TCS_BOTTOM) && !bClassicDraw && myGlobals.tabConfig.m_bottomAdjust != 0)
                    InvalidateRect(hwnd, NULL, FALSE);
                else if(tabdat->m_moderntabs)
                    InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
        case EM_REFRESHWITHOUTCLIP:
            if(TabCtrl_GetItemCount(hwnd) > 1)
                return 0;
            else {
                tabdat->bRefreshWithoutClip = TRUE;
                RedrawWindow(hwnd, NULL, NULL, RDW_UPDATENOW | RDW_NOCHILDREN | RDW_INVALIDATE);
                tabdat->bRefreshWithoutClip = FALSE;
                return 0;
            }
        case TCM_GETITEMRECT:
            {
                RECT *rc = (RECT *)lParam;
                LRESULT result = CallWindowProc(OldTabControlClassProc, hwnd, msg, wParam, lParam);
                RECT rcControl;


                if(tabdat->m_moderntabs && tabdat->dwStyle & TCS_MULTILINE) {
                    GetClientRect(hwnd, &rcControl);
                    if(rc->left < 10)
                        rc->right +=10;
                    else if(rc->right > rcControl.right - 10)
                        rc->left +=10;
                    else
                        OffsetRect(rc, 10, 0);
                }
                return result;
            }
        case TCM_INSERTITEM:
        case TCM_DELETEITEM:
            if(!(tabdat->dwStyle & TCS_MULTILINE) || tabdat->dwStyle & TCS_BUTTONS) {
                LRESULT result;
                RECT rc;
                int iTabs = TabCtrl_GetItemCount(hwnd);
                if(iTabs >= 1 && msg == TCM_INSERTITEM) {
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
        case WM_SETCURSOR:
        {
            POINT pt;

            GetCursorPos(&pt);
            SendMessage(GetParent(hwnd), msg, wParam, lParam);
            if (abs(pt.x - ptMouseT.x) < 4  && abs(pt.y - ptMouseT.y) < 4) {
                return 1;
            }
            ptMouseT = pt;
            if(tabdat->fTipActive){
                KillTimer(hwnd, TIMERID_HOVER_T);
                CallService("mToolTip/HideTip", 0, 0);
                tabdat->fTipActive = FALSE;
            }
            KillTimer(hwnd, TIMERID_HOVER_T);
            SetTimer(hwnd, TIMERID_HOVER_T, 450, 0);
            break;
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
                if(iTabs >= nrTabsPerLine && nrTabsPerLine > 0)
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
        case WM_RBUTTONDOWN:
            KillTimer(hwnd, TIMERID_HOVER_T);
            CallService("mToolTip/HideTip", 0, 0);
            tabdat->fTipActive = FALSE;
            break;

        case WM_LBUTTONDOWN:
		{
			TCHITTESTINFO tci = {0};

            KillTimer(hwnd, TIMERID_HOVER_T);
            CallService("mToolTip/HideTip", 0, 0);
            tabdat->fTipActive = FALSE;

            if(GetKeyState(VK_CONTROL) & 0x8000) {
                tci.pt.x=(short)LOWORD(GetMessagePos());
                tci.pt.y=(short)HIWORD(GetMessagePos());
                if(DragDetect(hwnd, tci.pt) && TabCtrl_GetItemCount(hwnd) >1 ) {
                    int i; 
                    tci.flags = TCHT_ONITEM;

                    ScreenToClient(hwnd, &tci.pt);
                    i= TabCtrl_HitTest(hwnd, &tci);
                    if(i != -1) {
                        TCITEM tc;
                        struct MessageWindowData *dat = NULL;

                        tc.mask = TCIF_PARAM;
                        TabCtrl_GetItem(hwnd, i, &tc);
                        dat = (struct MessageWindowData *)GetWindowLong((HWND)tc.lParam, GWL_USERDATA);
                        if(dat)	{
                            tabdat->bDragging = TRUE;
                            tabdat->iBeginIndex = i;
                            tabdat->hwndDrag = (HWND)tc.lParam;
                            tabdat->dragDat = dat;
                            tabdat->fSavePos = TRUE;
                            tabdat->himlDrag = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16), 1, 0);
                            ImageList_AddIcon(tabdat->himlDrag, dat->hTabIcon);
                            ImageList_BeginDrag(tabdat->himlDrag, 0, 8, 8);
                            ImageList_DragEnter(hwnd, tci.pt.x, tci.pt.y);
                            SetCapture(hwnd);
                        }
                        return TRUE;
                    }
                }
            }

           if(GetKeyState(VK_MENU) & 0x8000) {
               tci.pt.x=(short)LOWORD(GetMessagePos());
               tci.pt.y=(short)HIWORD(GetMessagePos());
               if(DragDetect(hwnd, tci.pt) && TabCtrl_GetItemCount(hwnd) >1 ) {
                   int i; 
                   tci.flags = TCHT_ONITEM;

                   ScreenToClient(hwnd, &tci.pt);
                   i= TabCtrl_HitTest(hwnd, &tci);
                   if(i != -1) {
                       TCITEM tc;
                       struct MessageWindowData *dat = NULL;

                       tc.mask = TCIF_PARAM;
                       TabCtrl_GetItem(hwnd, i, &tc);
                       dat = (struct MessageWindowData *)GetWindowLong((HWND)tc.lParam, GWL_USERDATA);
                       if(dat)	{
                           tabdat->bDragging = TRUE;
                           tabdat->iBeginIndex = i;
                           tabdat->hwndDrag = (HWND)tc.lParam;
                           tabdat->dragDat = dat;
                           tabdat->himlDrag = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16), 1, 0);
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
		}
        break;

    	case WM_CAPTURECHANGED:
        {
            tabdat->bDragging = FALSE;
            ImageList_DragLeave(hwnd);
            ImageList_EndDrag();
            if(tabdat->himlDrag) {
                ImageList_RemoveAll(tabdat->himlDrag);
                ImageList_Destroy(tabdat->himlDrag);
                tabdat->himlDrag = 0;
            }
        }
        break;

    	case WM_MOUSEMOVE:
        {
            if(tabdat->bDragging) {
                TCHITTESTINFO tci = {0};
                tci.pt.x=(short)LOWORD(GetMessagePos());
                tci.pt.y=(short)HIWORD(GetMessagePos());
                ScreenToClient(hwnd, &tci.pt);
                ImageList_DragMove(tci.pt.x, tci.pt.y);
            }
        }
        break;

    	case WM_LBUTTONUP:
        {
            CallWindowProc(OldTabControlClassProc, hwnd, msg, wParam, lParam);
            if(tabdat->bDragging && ReleaseCapture()) {
                TCHITTESTINFO tci = {0};
                int i; 
                tci.pt.x=(short)LOWORD(GetMessagePos());
                tci.pt.y=(short)HIWORD(GetMessagePos());
                tci.flags = TCHT_ONITEM;
                tabdat->bDragging = FALSE;
                ImageList_DragLeave(hwnd);
                ImageList_EndDrag();

                ScreenToClient(hwnd, &tci.pt);
                i= TabCtrl_HitTest(hwnd, &tci);
                if(i != -1 && i != tabdat->iBeginIndex)
                    RearrangeTab(tabdat->hwndDrag, tabdat->dragDat, MAKELONG(i, 0xffff), tabdat->fSavePos);
                tabdat->hwndDrag = (HWND)-1;
                tabdat->dragDat = NULL;
                if(tabdat->himlDrag) {
                    ImageList_RemoveAll(tabdat->himlDrag);
                    ImageList_Destroy(tabdat->himlDrag);
                    tabdat->himlDrag = 0;
                }
            }
        }
        break;

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

            tabdat->m_moderntabs = (DBGetContactSettingByte(NULL, SRMSGMOD_T, "moderntabs", 0) &&
                !(tabdat->dwStyle & TCS_BUTTONS));

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

            bmpOld = (HBITMAP)SelectObject(hdc, bmpMem);

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

			hPenOld = (HPEN)SelectObject(hdc, myGlobals.tabConfig.m_hPenLight);
            /*
             * visual style support
             */

            CopyRect(&rcTabPage, &rctPage);
            if(!tabdat->bRefreshWithoutClip)
                ExcludeClipRect(hdc, rctClip.left, rctClip.top, rctClip.right, rctClip.bottom);
            if(!bClassicDraw && IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
                RECT rcClient = rctPage;
                if(dwStyle & TCS_BOTTOM) {
                    rcClient.bottom = rctPage.bottom;
                    uiFlags |= uiBottom;
                }
                else
                    rcClient.top = rctPage.top;
                DrawThemesXpTabItem(hdc, -1, &rcClient, uiFlags, tabdat);	// TABP_PANE=9,0,'TAB'
                if(tabdat->bRefreshWithoutClip)
                    goto skip_tabs;
            }
            else {
                if(IntersectRect(&rectTemp, &rctPage, &ps.rcPaint)) {
					if(tabdat->pContainer->bSkinned) {
						StatusItems_t *item = &StatusItems[ID_EXTBKTABPAGE];

						if(!item->IGNORED) {
							DrawAlpha(hdc, &rctPage, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
									  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
							goto page_done;
						}
					}

                    if(tabdat->bRefreshWithoutClip)
                        goto skip_tabs;

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

                        if(tabdat->m_moderntabs)
                            SelectObject(hdc, myGlobals.tabConfig.m_hPenItemShadow);

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
                            if(!tabdat->m_moderntabs) {
                                MoveToEx(hdc, rectTemp.right - 2, rectTemp.top + 1, &pt);
                                LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
                                LineTo(hdc, rctActive.right, rectTemp.bottom - 2);
                                MoveToEx(hdc, rctActive.left - 2, rectTemp.bottom - 2, &pt);
                                LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);
                            }
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
                                LineTo(hdc, rectTemp.right - (tabdat->m_moderntabs ? 1 : 2), rctActive.bottom);
                            }
                            else {
                                RECT rectItemLeftmost;
                                UINT nItemLeftmost = FindLeftDownItem(hwnd);
                                TabCtrl_GetItemRect(hwnd, nItemLeftmost, &rectItemLeftmost);
                                LineTo(hdc, rectTemp.right - 2, rctActive.bottom);
                            }
                            if(!tabdat->m_moderntabs) {
                                SelectObject(hdc, myGlobals.tabConfig.m_hPenItemShadow);
                                LineTo(hdc, rectTemp.right - 2, rectTemp.bottom - 2);
                                LineTo(hdc, rectTemp.left, rectTemp.bottom - 2);
                            }

                            SelectObject(hdc, tabdat->m_moderntabs ? myGlobals.tabConfig.m_hPenItemShadow : myGlobals.tabConfig.m_hPenShadow);
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

            if(tabdat->bRefreshWithoutClip)
                goto skip_tabs;

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
                            DrawItemRect(tabdat, hdc, &rcItem, nHint | (i == hotItem ? HINT_HOTTRACK : 0), i);
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
                    DrawItemRect(tabdat, hdc, &rcItem, HINT_ACTIVATE_RIGHT_SIDE|HINT_ACTIVE_ITEM | nHint, iActive);
                    DrawItem(tabdat, hdc, &rcItem, HINT_ACTIVE_ITEM | nHint, iActive);
                }
            }
skip_tabs:
            if(hPenOld)
                SelectObject(hdc, hPenOld);

            /*
             * finally, bitblt the contents of the memory dc to the real dc
             */
			//if(!tabdat->pContainer->bSkinned)
            if(!tabdat->bRefreshWithoutClip)
                ExcludeClipRect(hdcreal, rctClip.left, rctClip.top, rctClip.right, rctClip.bottom);
            BitBlt(hdcreal, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, bmpOld);
            DeleteObject(bmpMem);
            DeleteDC(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER:
        {
            if(wParam == TIMERID_HOVER_T &&  DBGetContactSettingByte(NULL, SRMSGMOD_T, "d_tooltips", 1)) {
                POINT pt;
                CLCINFOTIP ti = {0};
                ti.cbSize = sizeof(ti);

                KillTimer(hwnd, TIMERID_HOVER_T);
                GetCursorPos(&pt);
                if(abs(pt.x - ptMouseT.x) < 5 && abs(pt.y - ptMouseT.y) < 5) {
                    TCITEM item = {0};
                    int    nItem = 0;
					struct MessageWindowData *dat = 0;

                    ti.ptCursor = pt;
                    //ScreenToClient(hwnd, &pt);

                    item.mask = TCIF_PARAM;
                    nItem = GetTabItemFromMouse(hwnd, &pt);
                    if(nItem >= 0 && nItem < TabCtrl_GetItemCount(hwnd)) {
                        TabCtrl_GetItem(hwnd, nItem, &item);
                        /*
                         * get the message window data for the session to which this tab item belongs
                         */

                        if(IsWindow((HWND)item.lParam) && item.lParam != 0)
                            dat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);
                        if(dat) {
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
        case  WM_USER + 100:
        {
            if(tabdat->fTipActive) {
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

    myGlobals.tabConfig.m_fixedwidth = (myGlobals.tabConfig.m_fixedwidth < 60 ? 60 : myGlobals.tabConfig.m_fixedwidth);
    myGlobals.tabConfig.m_hPenStyledLight = CreatePen(PS_SOLID, 1, myGlobals.tabConfig.colors[8]);
    myGlobals.tabConfig.m_hPenStyledDark = CreatePen(PS_SOLID, 1, myGlobals.tabConfig.colors[9]);
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
    DeleteObject(myGlobals.tabConfig.m_hPenStyledDark);
    DeleteObject(myGlobals.tabConfig.m_hPenStyledLight);
}

/*
 * options dialog for setting up tab options
 */

void CALLBACK DlgProcTabConfig(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
            SendMessage(hwndDlg, WM_USER + 100, 0, 0);
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            return TRUE;
        }
        case WM_USER + 100:
        {
            DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", TCF_DEFAULT);
            int i = 0;
            COLORREF clr;

            CheckDlgButton(hwndDlg, IDC_FLATTABS2, dwFlags & TCF_FLAT);
            CheckDlgButton(hwndDlg, IDC_FLASHICON, dwFlags & TCF_FLASHICON);
            CheckDlgButton(hwndDlg, IDC_FLASHLABEL, dwFlags & TCF_FLASHLABEL);
            CheckDlgButton(hwndDlg, IDC_NOSKINNING, dwFlags & TCF_NOSKINNING);
            CheckDlgButton(hwndDlg, IDC_SINGLEROWTAB, dwFlags & TCF_SINGLEROWTABCONTROL);
            CheckDlgButton(hwndDlg, IDC_LABELUSEWINCOLORS, !g_skinnedContainers && dwFlags & TCF_LABELUSEWINCOLORS);
            CheckDlgButton(hwndDlg, IDC_BKGUSEWINCOLORS2, !g_skinnedContainers && dwFlags & TCF_BKGUSEWINCOLORS);

            CheckDlgButton(hwndDlg, IDC_STYLEDTABS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "moderntabs", 0));

            SendMessage(hwndDlg, WM_COMMAND, IDC_LABELUSEWINCOLORS, 0);

            if(myGlobals.m_VSApiEnabled == 0) {
                CheckDlgButton(hwndDlg, IDC_NOSKINNING, TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_NOSKINNING), FALSE);
            }
            EnableWindow(GetDlgItem(hwndDlg, IDC_NOSKINNING), IsDlgButtonChecked(hwndDlg, IDC_STYLEDTABS) ? FALSE : TRUE);
            while(tabcolors[i].szKey != NULL) {
                clr = (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, g_skinnedContainers ? tabcolors[i].szSkinnedKey : tabcolors[i].szKey, GetSysColor(tabcolors[i].defclr));
                SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_SETCOLOUR, 0, (LPARAM)clr);
                SendDlgItemMessage(hwndDlg, tabcolors[i].id, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(tabcolors[i].defclr));
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

            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTER, UDM_SETRANGE, 0, MAKELONG(50, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERRIGHT, UDM_SETRANGE, 0, MAKELONG(50, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERTOP, UDM_SETRANGE, 0, MAKELONG(40, 0));
            SendDlgItemMessage(hwndDlg, IDC_TABBORDERSPINOUTERBOTTOM, UDM_SETRANGE, 0, MAKELONG(40, 0));

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
            EnableWindow(GetDlgItem(hwndDlg, IDC_NOSKINNING), IsDlgButtonChecked(hwndDlg, IDC_STYLEDTABS) ? FALSE : TRUE);
            return 0;
        }
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                        {
                            int i = 0;
                            COLORREF clr;
                            BOOL translated;
                            int  fixedWidth;

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
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "y-pad", (BYTE)(GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE)));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "x-pad", (BYTE)(GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE)));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "tborder", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDER, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_left" : "tborder_outer_left", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTER, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_right" : "tborder_outer_right", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERRIGHT, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_top" : "tborder_outer_top", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERTOP, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, g_skinnedContainers ? "S_tborder_outer_bottom" : "tborder_outer_bottom", (BYTE) GetDlgItemInt(hwndDlg, IDC_TABBORDEROUTERBOTTOM, &translated, FALSE));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "bottomadjust", GetDlgItemInt(hwndDlg, IDC_BOTTOMTABADJUST, &translated, TRUE));

                            fixedWidth = GetDlgItemInt(hwndDlg, IDC_TABWIDTH, &translated, FALSE);
                            fixedWidth = (fixedWidth < 60 ? 60 : fixedWidth);
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "fixedwidth", fixedWidth);
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "moderntabs", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_STYLEDTABS) ? 1 : 0));
                            FreeTabConfig();
                            if((COLORREF)SendDlgItemMessage(hwndDlg, IDC_LIGHTSHADOW, CPM_GETCOLOUR, 0, 0) == RGB(255, 0, 255))
                                DBDeleteContactSetting(NULL, SRMSGMOD_T, "tab_lightshadow");
                            if((COLORREF)SendDlgItemMessage(hwndDlg, IDC_DARKSHADOW, CPM_GETCOLOUR, 0, 0) == RGB(255, 0, 255))
                                DBDeleteContactSetting(NULL, SRMSGMOD_T, "tab_darkshadow");

                            ReloadTabConfig();
                            while(pContainer) {
                                TabCtrl_SetPadding(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetDlgItemInt(hwndDlg, IDC_HTABPADDING, NULL, FALSE), GetDlgItemInt(hwndDlg, IDC_TABPADDING, NULL, FALSE));
                                RedrawWindow(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
                                pContainer = pContainer->pNextContainer;
                            }
                            return TRUE;
                        }
                    }
                    break;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_STYLEDTABS:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_NOSKINNING), IsDlgButtonChecked(hwndDlg, IDC_STYLEDTABS) ? FALSE : TRUE);
                    break;
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
                break;
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    }
    return FALSE;
}
