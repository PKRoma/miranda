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

// TODO:
// - Support for bitmap buttons (simple call to DrawIconEx())
extern HINSTANCE g_hInst;
extern LONG g_cxsmIcon, g_cysmIcon;
extern HWND hwndContactList;
extern struct CluiData g_CluiData;
extern pfnDrawAlpha pDrawAlpha;
extern StatusItems_t *StatusItems;

static LRESULT CALLBACK TSButtonWndProc(HWND hwnd, UINT  msg, WPARAM wParam, LPARAM lParam);

typedef struct {
    HWND hwnd;
    int stateId; // button state
    int focus;   // has focus (1 or 0)
    HFONT hFont;   // font
    HICON arrow;   // uses down arrow
    int defbutton; // default button
    HICON hIcon, hIconPrivate;
    HBITMAP hBitmap;
    int pushBtn;
    int pbState;
    HANDLE hThemeButton;
    HANDLE hThemeToolbar;
    BOOL bThemed;
    DWORD bSkinned;
    char cHot;
    int flatBtn;
    char szText[128];
    SIZE sLabel;
    HIMAGELIST hIml;
    int iIcon;
} MButtonCtrl;

// External theme methods and properties
static HMODULE themeAPIHandle = NULL; // handle to uxtheme.dll
static HANDLE   (WINAPI *MyOpenThemeData)(HWND, LPCWSTR);
static HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);
static BOOL     (WINAPI *MyIsThemeBackgroundPartiallyTransparent)(HANDLE, int,
                                                                  int);
static HRESULT  (WINAPI *MyDrawThemeParentBackground)(HWND, HDC, RECT *);
static HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE, HDC, int, int,
                                                const RECT *, const RECT *);
static HRESULT  (WINAPI *MyDrawThemeText)(HANDLE, HDC, int, int, LPCWSTR, int,
                                          DWORD, DWORD, const RECT *);

static CRITICAL_SECTION csTips;
static HWND hwndToolTips = NULL;

int UnloadTSButtonModule(WPARAM wParam, LPARAM lParam)
{
    DeleteCriticalSection(&csTips);
    return 0;
}

int LoadCLCButtonModule(void)
{
    WNDCLASSEXA wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = "CLCButtonClass";
    wc.lpfnWndProc = TSButtonWndProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.cbWndExtra = sizeof(MButtonCtrl *);
    wc.hbrBackground = 0;
    wc.style = CS_GLOBALCLASS;
    RegisterClassExA(&wc);
    InitializeCriticalSection(&csTips);
    return 0;
}

// Used for our own cheap TrackMouseEvent
#define BUTTON_POLLID       100
#define BUTTON_POLLDELAY    50

#define MGPROC(x) GetProcAddress(themeAPIHandle,x)
static int ThemeSupport()
{
    if (IsWinVerXPPlus()) {
        if (!themeAPIHandle) {
            themeAPIHandle = GetModuleHandleA("uxtheme");
            if (themeAPIHandle) {
                MyOpenThemeData = (HANDLE(WINAPI *)(HWND, LPCWSTR))MGPROC("OpenThemeData");
                MyCloseThemeData = (HRESULT(WINAPI *)(HANDLE))MGPROC("CloseThemeData");
                MyIsThemeBackgroundPartiallyTransparent = (BOOL(WINAPI *)(HANDLE, int, int))MGPROC("IsThemeBackgroundPartiallyTransparent");
                MyDrawThemeParentBackground = (HRESULT(WINAPI *)(HWND, HDC, RECT *))MGPROC("DrawThemeParentBackground");
                MyDrawThemeBackground = (HRESULT(WINAPI *)(HANDLE, HDC, int, int, const RECT *, const RECT *))MGPROC("DrawThemeBackground");
                MyDrawThemeText = (HRESULT(WINAPI *)(HANDLE, HDC, int, int, LPCWSTR, int, DWORD, DWORD, const RECT *))MGPROC("DrawThemeText");
            }
        }
    // Make sure all of these methods are valid (i would hope either all or none work)
        if (MyOpenThemeData && MyCloseThemeData && MyIsThemeBackgroundPartiallyTransparent && MyDrawThemeParentBackground && MyDrawThemeBackground && MyDrawThemeText) {
            return 1;
        }
    }
    return 0;
}

static void DestroyTheme(MButtonCtrl *ctl)
{
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

static void LoadTheme(MButtonCtrl *ctl)
{
    if (ThemeSupport()) {
        DestroyTheme(ctl);
        ctl->hThemeButton = MyOpenThemeData(ctl->hwnd, L"BUTTON");
        ctl->hThemeToolbar = MyOpenThemeData(ctl->hwnd, L"TOOLBAR");
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
    if (hdcPaint) {
        HDC hdcMem;
        HBITMAP hbmMem;
        HBITMAP hbmOld = 0;
        RECT rcClient;
        HFONT hOldFont = 0;
        int xOffset = 0;
        
        GetClientRect(ctl->hwnd, &rcClient);
        hdcMem = CreateCompatibleDC(hdcPaint);
        hbmMem = CreateCompatibleBitmap(hdcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
        hbmOld = SelectObject(hdcMem, hbmMem);

        hOldFont = SelectObject(hdcMem, ctl->hFont);
    // If its a push button, check to see if it should stay pressed
        if (ctl->pushBtn && ctl->pbState)
            ctl->stateId = PBS_PRESSED;

    // Draw the flat button
        if (ctl->flatBtn) {
            if (ctl->hThemeToolbar && ctl->bThemed) {
                RECT rc = rcClient;
                int state = IsWindowEnabled(ctl->hwnd) ? (ctl->stateId == PBS_NORMAL && ctl->defbutton ? PBS_DEFAULTED : ctl->stateId) : PBS_DISABLED;
                SkinDrawBg(ctl->hwnd, hdcMem);
                if (MyIsThemeBackgroundPartiallyTransparent(ctl->hThemeToolbar, TP_BUTTON, TBStateConvert2Flat(state))) {
                    MyDrawThemeParentBackground(ctl->hwnd, hdcMem, &rc);
                }
                MyDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(state), &rc, &rc);
            } else {
                HBRUSH hbr;
                RECT rc = rcClient;

                if(ctl->bSkinned) {      // skinned
                    RECT rcParent;
                    POINT pt;
                    HWND hwndParent = hwndContactList;
                    StatusItems_t *item;
                    int item_id;
                    
                    GetWindowRect(ctl->hwnd, &rcParent);
                    pt.x = rcParent.left;
                    pt.y = rcParent.top;
                    
                    ScreenToClient(hwndContactList, &pt);
                    
                    if(HIWORD(ctl->bSkinned))
                        item_id = ctl->stateId == PBS_HOT ? ID_EXTBKTBBUTTONMOUSEOVER : (ctl->stateId == PBS_PRESSED ? ID_EXTBKTBBUTTONSPRESSED : ID_EXTBKTBBUTTONSNPRESSED);
                        //GetItemByStatus(ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : (ctl->stateId == PBS_PRESSED ? ID_EXTBKTBBUTTONSPRESSED : ID_EXTBKTBBUTTONSNPRESSED), &item);
                    else
                        item_id = ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : (ctl->stateId == PBS_PRESSED ? ID_EXTBKBUTTONSPRESSED : ID_EXTBKBUTTONSNPRESSED);
                    item = &StatusItems[item_id - ID_STATUS_OFFLINE];
                        //GetItemByStatus(ctl->stateId == PBS_PRESSED ? ID_EXTBKBUTTONSPRESSED : ID_EXTBKBUTTONSNPRESSED, &item);
                    SetTextColor(hdcMem, item->TEXTCOLOR);
                    if(item->IGNORED) {
                        if(pt.y < 10 || g_CluiData.bWallpaperMode)
                            //SkinDrawBg(ctl->hwnd, hdcMem);
                            BitBlt(hdcMem, 0, 0, rc.right, rc.bottom, g_CluiData.hdcBg, pt.x, pt.y, SRCCOPY);
                        else
                            FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_3DFACE));
                    }
                    else {
                        if(pt.y < 10 || g_CluiData.bWallpaperMode)
                            //SkinDrawBg(ctl->hwnd, hdcMem);
                            BitBlt(hdcMem, 0, 0, rc.right, rc.bottom, g_CluiData.hdcBg, pt.x, pt.y, SRCCOPY);
                        else
                            FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_3DFACE));
                        rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
                        rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
                        DrawAlpha(hdcMem, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
                                  item->CORNER, item->BORDERSTYLE, item->imageItem);
                    }
                }
                else {
                    if (ctl->stateId == PBS_PRESSED || ctl->stateId == PBS_HOT)
                        hbr = GetSysColorBrush(COLOR_3DFACE);
                    else {
                        HDC dc;
                        HWND hwndParent;

                        hwndParent = GetParent(ctl->hwnd);
                        dc = GetDC(hwndParent);
                        hbr = (HBRUSH) SendMessage(hwndParent, WM_CTLCOLORDLG, (WPARAM) dc, (LPARAM) hwndParent);
                        ReleaseDC(hwndParent, dc);
                    }
                    if (hbr) {
                        FillRect(hdcMem, &rc, hbr);
                        DeleteObject(hbr);
                    }
                }
                if(!ctl->bSkinned) {
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
    // Draw background/border
            if (ctl->hThemeButton && ctl->bThemed) {
                int state = IsWindowEnabled(ctl->hwnd) ? (ctl->stateId == PBS_NORMAL && ctl->defbutton ? PBS_DEFAULTED : ctl->stateId) : PBS_DISABLED;
                POINT pt;
                RECT rcParent;
                
                GetWindowRect(ctl->hwnd, &rcParent);
                pt.x = rcParent.left;
                pt.y = rcParent.top;
                ScreenToClient(hwndContactList, &pt);
                BitBlt(hdcMem, 0, 0, rcClient.right, rcClient.bottom, g_CluiData.hdcBg, pt.x, pt.y, SRCCOPY);

                if (MyIsThemeBackgroundPartiallyTransparent(ctl->hThemeButton, BP_PUSHBUTTON, state)) {
                    MyDrawThemeParentBackground(ctl->hwnd, hdcMem, &rcClient);
                }
                MyDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, state, &rcClient, &rcClient);
            } else {
                UINT uState = DFCS_BUTTONPUSH | ((ctl->stateId == PBS_HOT) ? DFCS_HOT : 0) | ((ctl->stateId == PBS_PRESSED) ? DFCS_PUSHED : 0);
                if (ctl->defbutton && ctl->stateId == PBS_NORMAL)
                    uState |= DLGC_DEFPUSHBUTTON;
                DrawFrameControl(hdcMem, &rcClient, DFC_BUTTON, uState);
            }

    // Draw focus rectangle if button has focus
            if (ctl->focus) {
                RECT focusRect = rcClient;
                InflateRect(&focusRect, -3, -3);
                DrawFocusRect(hdcMem, &focusRect);
            }
        }

    // If we have an icon or a bitmap, ignore text and only draw the image on the button
        if (ctl->hIcon || ctl->hIconPrivate || ctl->iIcon) {
            int ix = (rcClient.right - rcClient.left) / 2 - (g_cxsmIcon / 2);
            int iy = (rcClient.bottom - rcClient.top) / 2 - (g_cxsmIcon / 2);
            HICON hIconNew = ctl->hIconPrivate != 0 ? ctl->hIconPrivate : ctl->hIcon;
            if (lstrlenA(ctl->szText) == 0) {
                if (ctl->iIcon)
                    ImageList_DrawEx(ctl->hIml, ctl->iIcon, hdcMem, ix, iy, g_cxsmIcon, g_cysmIcon, CLR_NONE, CLR_NONE, ILD_NORMAL);
                else
                    DrawState(hdcMem, NULL, NULL, (LPARAM) hIconNew, 0, ix, iy, g_cxsmIcon, g_cysmIcon, IsWindowEnabled(ctl->hwnd) ? DST_ICON | DSS_NORMAL : DST_ICON | DSS_DISABLED);
                ctl->sLabel.cx = ctl->sLabel.cy = 0;
            } else {
                GetTextExtentPoint32A(hdcMem, ctl->szText, lstrlenA(ctl->szText), &ctl->sLabel);

                if(g_cxsmIcon + ctl->sLabel.cx + 8 > rcClient.right - rcClient.left)
                    ctl->sLabel.cx = (rcClient.right - rcClient.left) - g_cxsmIcon - 8;
                else
                    ctl->sLabel.cx += 4;
                
                ix = (rcClient.right - rcClient.left) / 2 - ((g_cxsmIcon + ctl->sLabel.cx) / 2);
                if (ctl->iIcon)
                    ImageList_DrawEx(ctl->hIml, ctl->iIcon, hdcMem, ix, iy, g_cxsmIcon, g_cysmIcon, CLR_NONE, CLR_NONE, ILD_NORMAL);
                else
                    DrawState(hdcMem, NULL, NULL, (LPARAM) hIconNew, 0, ix, iy, g_cxsmIcon, g_cysmIcon, IsWindowEnabled(ctl->hwnd) ? DST_ICON | DSS_NORMAL : DST_ICON | DSS_DISABLED);
                xOffset = ix + g_cxsmIcon + 4;
            }
        } else if (ctl->hBitmap) {
            BITMAP bminfo;
            int ix, iy;

            GetObject(ctl->hBitmap, sizeof(bminfo), &bminfo);
            ix = (rcClient.right - rcClient.left) / 2 - (bminfo.bmWidth / 2);
            iy = (rcClient.bottom - rcClient.top) / 2 - (bminfo.bmHeight / 2);
            if (ctl->stateId == PBS_PRESSED) {
                ix++;
                iy++;
            }
            DrawState(hdcMem, NULL, NULL, (LPARAM) ctl->hBitmap, 0, ix, iy, bminfo.bmWidth, bminfo.bmHeight, IsWindowEnabled(ctl->hwnd) ? DST_BITMAP : DST_BITMAP | DSS_DISABLED);
        }
        if (GetWindowTextLengthA(ctl->hwnd)) {
    // Draw the text and optinally the arrow
            RECT rcText;

            CopyRect(&rcText, &rcClient);
            SetBkMode(hdcMem, TRANSPARENT);
    // XP w/themes doesn't used the glossy disabled text.  Is it always using COLOR_GRAYTEXT?  Seems so.
            if(!ctl->bSkinned)
                SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd) || !ctl->hThemeButton ? GetSysColor(COLOR_BTNTEXT) : GetSysColor(COLOR_GRAYTEXT));
            if (ctl->arrow)
                DrawState(hdcMem, NULL, NULL, (LPARAM) ctl->arrow, 0, rcClient.right - rcClient.left - 5 - g_cxsmIcon + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), (rcClient.bottom - rcClient.top) / 2 - g_cysmIcon / 2 + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), g_cxsmIcon, g_cysmIcon, IsWindowEnabled(ctl->hwnd) ? DST_ICON : DST_ICON | DSS_DISABLED);
            DrawStateA(hdcMem, NULL, NULL, (LPARAM) ctl->szText, 0, xOffset + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), ctl->hThemeButton ? (rcText.bottom - rcText.top - ctl->sLabel.cy) / 2 + 1 : (rcText.bottom - rcText.top - ctl->sLabel.cy) / 2 + (ctl->stateId == PBS_PRESSED ? 1 : 0), ctl->sLabel.cx, ctl->sLabel.cy, IsWindowEnabled(ctl->hwnd) || ctl->hThemeButton ? DST_PREFIXTEXT | DSS_NORMAL : DST_PREFIXTEXT | DSS_DISABLED);
        }
        if (hOldFont)
            SelectObject(hdcMem, hOldFont);
        BitBlt(hdcPaint, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        DeleteObject(hbmOld);
    }
}

static LRESULT CALLBACK TSButtonWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MButtonCtrl *bct = (MButtonCtrl *) GetWindowLong(hwndDlg, 0);
    switch (msg) {
        case WM_NCCREATE:
            {
                SetWindowLong(hwndDlg, GWL_STYLE, GetWindowLong(hwndDlg, GWL_STYLE) | BS_OWNERDRAW);
                bct = malloc(sizeof(MButtonCtrl));
                if (bct == NULL)
                    return FALSE;
                bct->hwnd = hwndDlg;
                bct->stateId = PBS_NORMAL;
                bct->focus = 0;
                bct->hFont = GetStockObject(DEFAULT_GUI_FONT);
                bct->arrow = NULL;
                bct->defbutton = 0;
                bct->hIcon = bct->hIconPrivate = 0;
                bct->iIcon = 0;
                bct->hIml = 0;
                bct->hBitmap = NULL;
                bct->pushBtn = 0;
                bct->pbState = 0;
                bct->hThemeButton = NULL;
                bct->hThemeToolbar = NULL;
                bct->cHot = 0;
                bct->flatBtn = 0;
                bct->bThemed = FALSE;
                bct->bSkinned = 0;
                LoadTheme(bct);
                SetWindowLong(hwndDlg, 0, (LONG) bct);
                if (((CREATESTRUCTA *) lParam)->lpszName)
                    SetWindowTextA(hwndDlg, ((CREATESTRUCTA *) lParam)->lpszName);
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
                        ti.uId = (UINT) bct->hwnd;
                        if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM) &ti)) {
                            SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM) &ti);
                        }
                        if (SendMessage(hwndToolTips, TTM_GETTOOLCOUNT, 0, (LPARAM) &ti) == 0) {
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
                SetWindowLong(hwndDlg, 0, (LONG) NULL);
                break;  // DONT! fall thru
            }
        case WM_SETTEXT:
            {
                bct->cHot = 0;
                if ((char*) lParam) {
                    char *tmp = (char *) lParam;
                    while (*tmp) {
                        if (*tmp == '&' && *(tmp + 1)) {
                            bct->cHot = tolower(*(tmp + 1));
                            break;
                        }
                        tmp++;
                    }
                    InvalidateRect(bct->hwnd, NULL, TRUE);
                    strncpy(bct->szText, (char*) lParam, 127);
                    bct->szText[127] = 0;
                }
                break;
            }
        case WM_SYSKEYUP:
            if (bct->stateId != PBS_DISABLED && bct->cHot && bct->cHot == tolower((int) wParam)) {
                if (bct->pushBtn) {
                    if (bct->pbState)
                        bct->pbState = 0;
                    else
                        bct->pbState = 1;
                    InvalidateRect(bct->hwnd, NULL, TRUE);
                }
                SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM) hwndDlg);
                return 0;
            }
            break;
        case WM_THEMECHANGED:
            {
            // themed changed, reload theme object
                if (bct->bThemed)
                    LoadTheme(bct);
                InvalidateRect(bct->hwnd, NULL, TRUE); // repaint it
                break;
            }
        case WM_SETFONT:
    // remember the font so we can use it later
            {
                bct->hFont = (HFONT) wParam; // maybe we should redraw?
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
        case BM_GETIMAGE:
            if(wParam == IMAGE_ICON)
                return (LRESULT)(bct->hIconPrivate ? bct->hIconPrivate : bct->hIcon);
            break;
        case BM_SETIMAGE:
            if(!lParam)
                break;
            bct->hIml = 0;
            bct->iIcon = 0;
            if (wParam == IMAGE_ICON) {
                ICONINFO ii = {0};
                BITMAP bm = {0};

                if (bct->hIconPrivate) {
                    DestroyIcon(bct->hIconPrivate);
                    bct->hIconPrivate = 0;
                }

                GetIconInfo((HICON) lParam, &ii);
                GetObject(ii.hbmColor, sizeof(bm), &bm);
                if (bm.bmWidth > g_cxsmIcon || bm.bmHeight > g_cysmIcon) {
                    HIMAGELIST hImageList;
                    hImageList = ImageList_Create(g_cxsmIcon, g_cysmIcon, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
                    ImageList_AddIcon(hImageList, (HICON) lParam);
                    bct->hIconPrivate = ImageList_GetIcon(hImageList, 0, ILD_NORMAL);
                    ImageList_RemoveAll(hImageList);
                    ImageList_Destroy(hImageList);
                    bct->hIcon = 0;
                } else {
                    bct->hIcon = (HICON) lParam;
                    bct->hIconPrivate = 0;
                }

                DeleteObject(ii.hbmMask);
                DeleteObject(ii.hbmColor);
                bct->hBitmap = NULL;
                InvalidateRect(bct->hwnd, NULL, TRUE);
            } else if (wParam == IMAGE_BITMAP) {
                bct->hBitmap = (HBITMAP) lParam;
                if (bct->hIconPrivate)
                    DestroyIcon(bct->hIconPrivate);
                bct->hIcon = bct->hIconPrivate = NULL;
                InvalidateRect(bct->hwnd, NULL, TRUE);
            }
            break;
        case BM_SETPRIVATEICON:
            bct->hIml = 0;
            bct->iIcon = 0; {
                if (bct->hIconPrivate)
                    DestroyIcon(bct->hIconPrivate);
                bct->hIconPrivate = DuplicateIcon(g_hInst, (HICON) lParam);
                bct->hIcon = 0;
                break;
            }
        case BM_SETIMLICON:
            {
                if (bct->hIconPrivate)
                    DestroyIcon(bct->hIconPrivate);
                bct->hIml = (HIMAGELIST) wParam;
                bct->iIcon = (int) lParam;
                bct->hIcon = bct->hIconPrivate = 0;
                InvalidateRect(bct->hwnd, NULL, TRUE);
                break;
            }
        case BM_SETSKINNED:
            bct->bSkinned = (DWORD)lParam;
            bct->bThemed = bct->bSkinned ? FALSE : bct->bThemed;
            InvalidateRect(bct->hwnd, NULL, TRUE);
            break;
        case BM_SETCHECK:
            if (!bct->pushBtn)
                break;
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
        case BUTTONSETARROW:
    // turn arrow on/off
            if (wParam) {
                if (!bct->arrow)
                    bct->arrow = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MINIMIZE), IMAGE_ICON, g_cxsmIcon, g_cysmIcon, 0);
            } else {
                if (bct->arrow) {
                    DestroyIcon(bct->arrow);
                    bct->arrow = NULL;
                }
            }
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
            bct->bSkinned = bct->bThemed ? 0 : bct->bSkinned;
            break;
        case BUTTONADDTOOLTIP:
            {
                TOOLINFOA ti;

                if (!(char*) wParam)
                    break;
                EnterCriticalSection(&csTips);
                if (!hwndToolTips) {
                    hwndToolTips = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
                }
                ZeroMemory(&ti, sizeof(ti));
                ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_IDISHWND;
                ti.hwnd = bct->hwnd;
                ti.uId = (UINT) bct->hwnd;
                if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM) &ti)) {
                    SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM) &ti);
                }
                ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                ti.uId = (UINT) bct->hwnd;
                ti.lpszText = (char*) wParam;
                SendMessageA(hwndToolTips, TTM_ADDTOOLA, 0, (LPARAM) &ti);
                LeaveCriticalSection(&csTips);
                break;
            }
        case WM_SETFOCUS:
    // set keybord focus and redraw
            bct->focus = 1;
            InvalidateRect(bct->hwnd, NULL, TRUE);
            break;
        case WM_KILLFOCUS:
    // kill focus and redraw
            bct->focus = 0;
            InvalidateRect(bct->hwnd, NULL, TRUE);
            break;
        case WM_WINDOWPOSCHANGED:
            InvalidateRect(bct->hwnd, NULL, TRUE);
            break;
        case WM_ENABLE:
    // windows tells us to enable/disable
            {
                bct->stateId = wParam ? PBS_NORMAL : PBS_DISABLED;
                InvalidateRect(bct->hwnd, NULL, TRUE);
                break;
            }
        case WM_MOUSELEAVE:
    // faked by the WM_TIMER
            {
                if (bct->stateId != PBS_DISABLED) {
                // don't change states if disabled
                    bct->stateId = PBS_NORMAL;
                    InvalidateRect(bct->hwnd, NULL, TRUE);
                }
                break;
            }
        case WM_LBUTTONDOWN:
            {
                if (bct->stateId != PBS_DISABLED) {
                // don't change states if disabled
                    bct->stateId = PBS_PRESSED;
                    InvalidateRect(bct->hwnd, NULL, TRUE);
                }
                break;
            }
        case WM_LBUTTONUP:
            {
                if (bct->pushBtn) {
                    if (bct->pbState)
                        bct->pbState = 0;
                    else
                        bct->pbState = 1;
                }
                if (bct->stateId != PBS_DISABLED) {
                // don't change states if disabled
                    if (msg == WM_LBUTTONUP)
                        bct->stateId = PBS_HOT;
                    else
                        bct->stateId = PBS_NORMAL;
                    InvalidateRect(bct->hwnd, NULL, TRUE);
                }
        // Tell your daddy you got clicked.
                SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM) hwndDlg);
                break;
            }
        case WM_MOUSEMOVE:
            if (bct->stateId == PBS_NORMAL) {
                bct->stateId = PBS_HOT;
                InvalidateRect(bct->hwnd, NULL, TRUE);
            }
    // Call timer, used to start cheesy TrackMouseEvent faker
            SetTimer(hwndDlg, BUTTON_POLLID, BUTTON_POLLDELAY, NULL);
            break;
        case WM_NCHITTEST:
            {
                LRESULT lr = SendMessage(GetParent(hwndDlg), WM_NCHITTEST, wParam, lParam);
                if(lr == HTLEFT || lr == HTRIGHT || lr == HTBOTTOM || lr == HTTOP || lr == HTTOPLEFT || lr == HTTOPRIGHT
                   || lr == HTBOTTOMLEFT || lr == HTBOTTOMRIGHT)
                    return HTTRANSPARENT;
                break;
            }
        case WM_TIMER:
    // use a timer to check if they have did a mouseout
            {
                if (wParam == BUTTON_POLLID) {
                    RECT rc;
                    POINT pt;
                    GetWindowRect(hwndDlg, &rc);
                    GetCursorPos(&pt);
                    if (!PtInRect(&rc, pt)) {
                    // mouse must be gone, trigger mouse leave
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
