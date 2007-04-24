#include "commonheaders.h"
#include "commonprototypes.h"
#include "m_skinbutton.h"

#define BUTTON_POLLID       100
#define BUTTON_POLLDELAY    50
#define b2str(a) ((a) ? "True" : "False")

struct tagSKINBUTTONDATA
{
	HWND	hWnd;			// handle of safe window
	char	szButtonID[64];	// Unique stringID of button in form Module.Name
	int		nStateId;		// state of button
	BOOL	fFocused;		// button is focused flag
	BOOL	fSendOnDown;	// send event on button pushed
	BOOL	fHotMark;		// button is hot marked (e.g. current state)
	BOOL	defbutton;
	int		nFontID;		// internal font ID
	HFONT	hFont;			// font
	HICON   hIconPrivate;	// icon need to be destroyed
	HICON   hIcon;			// icon not need to be destroyed
	char	cHot;			// button hot key
	TCHAR	szText[128];	// text on the button
	RECT	rcMargins;		// margins of inner content
	BOOL	pushBtn;		// is it push button
	int		pbState;		// state of push button

	HANDLE	hThemeButton;
    HANDLE	hThemeToolbar;
    BOOL	bThemed;
};
typedef struct tagSKINBUTTONDATA SKINBUTTONDATA;


static CRITICAL_SECTION csTips;
static HWND hwndToolTips = NULL;
static LRESULT CALLBACK SkinButtonProc(HWND hwndDlg, UINT  msg, WPARAM wParam, LPARAM lParam);
static void PaintWorker(SKINBUTTONDATA *lpSBData, HDC hdcPaint , POINT * pOffset);
static BOOL	bThemed=FALSE;
static HANDLE hThemeButton=FALSE;
static HANDLE hThemeToolbar=FALSE;

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

BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD) = 0;

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
				MyEnableThemeDialogTexture = (BOOL (WINAPI *)(HANDLE, DWORD))MGPROC("EnableThemeDialogTexture");
			}
        }
    // Make sure all of these methods are valid (i would hope either all or none work)
        if (MyOpenThemeData && MyCloseThemeData && MyIsThemeBackgroundPartiallyTransparent && MyDrawThemeParentBackground && MyDrawThemeBackground && MyDrawThemeText) {
            return 1;
        }
    }
    return 0;
}

static void DestroyTheme(SKINBUTTONDATA *ctl)
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
	ctl->bThemed = FALSE;
}

static void LoadTheme(SKINBUTTONDATA *ctl)
{
    if (ThemeSupport()) {
        DestroyTheme(ctl);
		if (g_CluiData.fDisableSkinEngine)
		{
			ctl->hThemeButton = MyOpenThemeData(ctl->hWnd, L"BUTTON");
			ctl->hThemeToolbar = MyOpenThemeData(ctl->hWnd, L"TOOLBAR");
			ctl->bThemed = TRUE;
		}
    }
}

int LoadSkinButtonModule()
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpszClassName =SKINBUTTONCLASS;
	wc.lpfnWndProc = SkinButtonProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra = sizeof(SKINBUTTONDATA *);
	wc.hbrBackground = 0;
	wc.style = CS_GLOBALCLASS;
	RegisterClassEx(&wc);
	return 0;
}
int UnloadSkinButtonModule(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static void InvalidateParentRect(HWND hwndChild, RECT * lpRect, BOOL fErase)
{
	LONG lExStyle=GetWindowLong(hwndChild,GWL_EXSTYLE);
	if (lExStyle&WS_EX_TRANSPARENT)
	{
		NMHDR hdr;
		hdr.hwndFrom=hwndChild;
		hdr.idFrom=0;
		hdr.code=BUTTONNEEDREDRAW;
		SendMessage(GetParent(hwndChild),WM_NOTIFY,(WPARAM)hwndChild,(LPARAM)&hdr);
	}
	else
	{
		InvalidateRect(hwndChild,lpRect,fErase);
	}
}
static LRESULT CALLBACK SkinButtonProc(HWND hwndDlg, UINT  msg, WPARAM wParam, LPARAM lParam)
{
	SKINBUTTONDATA *lpSBData = (SKINBUTTONDATA *) GetWindowLong(hwndDlg, 0);
	switch (msg) 
	{
		case WM_NCCREATE:
			{
				SetWindowLong(hwndDlg, GWL_STYLE, GetWindowLong(hwndDlg, GWL_STYLE) | BS_OWNERDRAW);
				lpSBData = malloc(sizeof(SKINBUTTONDATA));
				if (lpSBData == NULL)
					return FALSE;
				memset(lpSBData,0,sizeof(SKINBUTTONDATA)); //I prefer memset to guarantee zeros
				
				lpSBData->hWnd = hwndDlg;
				lpSBData->nStateId = PBS_NORMAL;
				lpSBData->fFocused = FALSE;
				lpSBData->hFont = GetStockObject(DEFAULT_GUI_FONT);
				lpSBData->hIconPrivate = NULL;
				lpSBData->cHot = '\0';
				lpSBData->szText[0] = '\0';
				lpSBData->szButtonID[0] = '?';
				lpSBData->szButtonID[1] = '\0';
				lpSBData->pushBtn = FALSE;
				lpSBData->pbState = 0;
				lpSBData->fSendOnDown = FALSE;
				lpSBData->nFontID = -1;
				SetWindowLong(hwndDlg, 0, (LONG) lpSBData);
				if (((CREATESTRUCTA *) lParam)->lpszName)
					SetWindowText(hwndDlg, ((CREATESTRUCT *) lParam)->lpszName);
				LoadTheme(lpSBData);
				return TRUE;
			}
		case WM_DESTROY:
			{
				if (lpSBData) 
				{
					if (hwndToolTips) 
					{
						TOOLINFO ti;

						ZeroMemory(&ti, sizeof(ti));
						ti.cbSize = sizeof(ti);
						ti.uFlags = TTF_IDISHWND;
						ti.hwnd = lpSBData->hWnd;
						ti.uId = (UINT) lpSBData->hWnd;
						if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM) &ti)) 
						{
							SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM) &ti);
						}
						if (SendMessage(hwndToolTips, TTM_GETTOOLCOUNT, 0, (LPARAM) &ti) == 0) 
						{
							DestroyWindow(hwndToolTips);
							hwndToolTips = NULL;
						}
					}
					if (lpSBData->hIconPrivate)
						DestroyIcon(lpSBData->hIconPrivate);
					DestroyTheme(lpSBData);
					free(lpSBData);  // lpSBData was malloced by native malloc
				}
				SetWindowLong(hwndDlg, 0, (LONG) NULL);
				break;  // DONT! fall thru
			}
		case WM_SETTEXT:
			{
				lpSBData->cHot = 0;
				if ((TCHAR*) lParam) 
				{
					TCHAR *tmp = (TCHAR *) lParam;
					while (*tmp) 
					{
						if (*tmp == '&' && *(tmp + 1)) 
						{
							lpSBData->cHot = (char)tolower(*(tmp + 1));
							break;
						}
						tmp++;
					}
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
					lstrcpyn(lpSBData->szText, (TCHAR *)lParam, SIZEOF(lpSBData->szText)-1);
					lpSBData->szText[SIZEOF(lpSBData->szText)-1] = '\0';
				}
				break;
			}
		case WM_SYSKEYUP:
			if (lpSBData->nStateId != PBS_DISABLED && lpSBData->cHot && lpSBData->cHot == tolower((int) wParam)) 
			{
				if (lpSBData->pushBtn) 
				{
					if (lpSBData->pbState)
						lpSBData->pbState = 0;
					else
						lpSBData->pbState = 1;
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				}
				if(!lpSBData->fSendOnDown)
					SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM) hwndDlg);
				return 0;
			}
			break;
		
		case WM_THEMECHANGED:
			{
				// themed changed, reload theme object
				LoadTheme(lpSBData);
				InvalidateParentRect(lpSBData->hWnd, NULL, TRUE); // repaint it
				break;
			}
		
		case WM_SETFONT:			
			{	
				// remember the font so we can use it later
				lpSBData->hFont = (HFONT) wParam; // maybe we should redraw?
				lpSBData->nFontID = (int) lParam - 1;
				break;
			}
		case BUTTONSETMARGINS:
			{
				if (lParam)	lpSBData->rcMargins=*(RECT*)lParam;
				else 
				{
					RECT nillRect={0};
					lpSBData->rcMargins=nillRect;
				}
				return 0;
			}
		case BUTTONSETID:
			{
				lstrcpynA(lpSBData->szButtonID, (char *)lParam, SIZEOF(lpSBData->szButtonID)-1);
				lpSBData->szButtonID[SIZEOF(lpSBData->szButtonID)-1] = '\0';
				return 0;
			}
		case BUTTONDRAWINPARENT:
			{
				PaintWorker(lpSBData, (HDC) wParam, (POINT*) lParam);
				return 0;
			}
		case WM_NCPAINT:
		case WM_PAINT:
			{
				
				PAINTSTRUCT ps;
				HDC hdcPaint;
				hdcPaint = BeginPaint(hwndDlg, &ps);
				if (hdcPaint) 
				{
					PaintWorker(lpSBData, hdcPaint, NULL);
					EndPaint(hwndDlg, &ps);
				}
				ValidateRect(hwndDlg,NULL);
				return 0;
			}
		case BUTTONADDTOOLTIP:
			{
				TOOLINFO ti;

				if (!(char*) wParam)
					break;
				if (!hwndToolTips) 
				{
					hwndToolTips = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, _T(""), WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
					SetWindowPos(hwndToolTips, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
				}
				ZeroMemory(&ti, sizeof(ti));
				ti.cbSize = sizeof(ti);
				ti.uFlags = TTF_IDISHWND;
				ti.hwnd = lpSBData->hWnd;
				ti.uId = (UINT) lpSBData->hWnd;
				if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM) &ti)) {
					SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM) &ti);
				}
				ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
				ti.uId = (UINT) lpSBData->hWnd;
				ti.lpszText = (TCHAR *) wParam;
				SendMessage(hwndToolTips, TTM_ADDTOOL, 0, (LPARAM) &ti);
				break;
			}
		case WM_SETFOCUS:
			{
				// set keyboard focus and redraw
				lpSBData->fFocused = TRUE;
				InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				break;
			}
		case WM_KILLFOCUS:
			{
				// kill focus and redraw
				lpSBData->fFocused = FALSE;
				InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				break;
			}
		case WM_WINDOWPOSCHANGED:
			InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
			break;
		case WM_ENABLE:
			// windows tells us to enable/disable
			{
				lpSBData->nStateId = wParam ? PBS_NORMAL : PBS_DISABLED;
				InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				break;
			}
		case WM_MOUSELEAVE:			
			{
				// faked by the WM_TIMER
				if (lpSBData->nStateId != PBS_DISABLED) 
				{
					// don't change states if disabled
					lpSBData->nStateId = PBS_NORMAL;
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				}
				break;
			}
		case WM_LBUTTONDOWN:
			{
				if (lpSBData->nStateId != PBS_DISABLED && lpSBData->nStateId != PBS_PRESSED) 
				{
					lpSBData->nStateId = PBS_PRESSED;
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
					if(lpSBData->fSendOnDown) 
					{
						SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM) hwndDlg);
						lpSBData->nStateId = PBS_NORMAL;
						InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
					}
				}
				break;
			}
		case WM_LBUTTONUP:
			{
				if (lpSBData->pushBtn) 
				{
					if (lpSBData->pbState)
						lpSBData->pbState = FALSE;
					else
						lpSBData->pbState = TRUE;
				}

				if (lpSBData->nStateId != PBS_DISABLED)
				{
					// don't change states if disabled
					if (msg == WM_LBUTTONUP)
						lpSBData->nStateId = PBS_HOT;
					else
						lpSBData->nStateId = PBS_NORMAL;
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				}
				if(!lpSBData->fSendOnDown)
					SendMessage(GetParent(hwndDlg), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndDlg), BN_CLICKED), (LPARAM) hwndDlg);
				break;
			}
		case WM_MOUSEMOVE:
			if (lpSBData->nStateId == PBS_NORMAL) 
			{
				lpSBData->nStateId = PBS_HOT;
				InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
			}
			// Call timer, used to start cheesy TrackMouseEvent faker
			CLUI_SafeSetTimer(hwndDlg, BUTTON_POLLID, BUTTON_POLLDELAY, NULL);
			break;
		case WM_NCHITTEST:
			{
				LRESULT lr = SendMessage(GetParent(hwndDlg), WM_NCHITTEST, wParam, lParam);
				if(lr == HTLEFT || lr == HTRIGHT || lr == HTBOTTOM || lr == HTTOP || lr == HTTOPLEFT || lr == HTTOPRIGHT
					|| lr == HTBOTTOMLEFT || lr == HTBOTTOMRIGHT)
					return HTTRANSPARENT;
				break;
			}
		case WM_TIMER: // use a timer to check if they have did a mouse out		
			{
				if (wParam == BUTTON_POLLID)
				{
					RECT rc;
					POINT pt;
					GetWindowRect(hwndDlg, &rc);
					GetCursorPos(&pt);
					if (!PtInRect(&rc, pt)) 
					{
						// mouse must be gone, trigger mouse leave
						PostMessage(hwndDlg, WM_MOUSELEAVE, 0, 0L);
						KillTimer(hwndDlg, BUTTON_POLLID);
					}
				}
				break;
			}
		case WM_ERASEBKGND:
			{
				return 1;
			}
		case BM_GETIMAGE:
			{
				if(wParam == IMAGE_ICON)
					return (LRESULT)(lpSBData->hIconPrivate ? lpSBData->hIconPrivate : lpSBData->hIcon);
				break;
			}
		case BM_SETIMAGE:
			{
				if(!lParam)
					break;
				if (wParam == IMAGE_ICON) 
				{
					ICONINFO ii = {0};
					BITMAP bm = {0};

					if (lpSBData->hIconPrivate) 
					{
						DestroyIcon(lpSBData->hIconPrivate);
						lpSBData->hIconPrivate = 0;
					}

					GetIconInfo((HICON) lParam, &ii);
					GetObject(ii.hbmColor, sizeof(bm), &bm);
					if (bm.bmWidth > 16 || bm.bmHeight > 16) 
					{
						HIMAGELIST hImageList;
						hImageList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
						ImageList_AddIcon(hImageList, (HICON) lParam);
						lpSBData->hIconPrivate = ImageList_GetIcon(hImageList, 0, ILD_NORMAL);
						ImageList_RemoveAll(hImageList);
						ImageList_Destroy(hImageList);
						lpSBData->hIcon = 0;
					} 
					else 
					{
						lpSBData->hIcon = (HICON) lParam;
						lpSBData->hIconPrivate = NULL;
					}

					DeleteObject(ii.hbmMask);
					DeleteObject(ii.hbmColor);
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
				}
				else if (wParam == IMAGE_BITMAP) 
				{
					if (lpSBData->hIconPrivate)
						DestroyIcon(lpSBData->hIconPrivate);
					lpSBData->hIcon = lpSBData->hIconPrivate = NULL;
					InvalidateParentRect(lpSBData->hWnd, NULL, TRUE);
					return 0; // not supported
				}
				break;
			}
	}
	return DefWindowProc(hwndDlg, msg, wParam, lParam);
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

static void PaintWorker(SKINBUTTONDATA *lpSBData, HDC hdcPaint , POINT * pOffset)
{
	HDC hdcMem;
	HBITMAP hbmMem;	
	RECT rcClient;
	int width;
	int height;
	HBITMAP hbmOld = NULL;
	HFONT hOldFont = NULL;
	BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	POINT offset={0};
	if (pOffset) offset=*pOffset;

	if (!hdcPaint) return;  //early exit
	
	GetClientRect(lpSBData->hWnd, &rcClient);
	width  =rcClient.right - rcClient.left;
	height =rcClient.bottom - rcClient.top;
	
	hdcMem = pOffset?hdcPaint:CreateCompatibleDC(hdcPaint);
	hOldFont = SelectObject(hdcMem, lpSBData->hFont);
	if (!pOffset) 
	{
		hbmMem = SkinEngine_CreateDIB32(width, height);
		hbmOld = SelectObject(hdcMem, hbmMem);		
	}
	else
	{
		OffsetRect(&rcClient,offset.x,offset.y);
	}
	{
	if (!g_CluiData.fDisableSkinEngine)
		{
			char szRequest[128];
			/* painting */
			_snprintf(szRequest,sizeof(szRequest),"Button,ID=%s,Hovered=%s,Pressed=%s,Focused=%s",
				lpSBData->szButtonID,				// ID		
				b2str(lpSBData->nStateId==PBS_HOT),	// Hovered
				b2str(lpSBData->nStateId==PBS_PRESSED || lpSBData->pbState == TRUE),	// Pressed
				b2str(lpSBData->fFocused) );		// Focused
			
			SkinDrawGlyph(hdcMem,&rcClient,&rcClient,szRequest);
		}
		else 
		{
			if (lpSBData->bThemed)
			{
				RECT *rc = &rcClient;
				int state = IsWindowEnabled(lpSBData->hWnd) ? (lpSBData->nStateId == PBS_NORMAL && lpSBData->defbutton ? PBS_DEFAULTED : lpSBData->nStateId) : PBS_DISABLED;
				if (MyIsThemeBackgroundPartiallyTransparent(lpSBData->hThemeToolbar, TP_BUTTON, TBStateConvert2Flat(state)))
				{	
					//Draw parent?
					MyDrawThemeParentBackground(lpSBData->hWnd, hdcMem, rc);
				}
				MyDrawThemeBackground(lpSBData->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(state), rc, rc);
			}
		}

	}
	{
		
		RECT rcTemp	= rcClient;  //content rect
		BYTE bPressed = (lpSBData->nStateId==PBS_PRESSED || lpSBData->pbState == TRUE)?1:0;
		HICON hHasIcon = lpSBData->hIcon?lpSBData->hIcon:lpSBData->hIconPrivate?lpSBData->hIconPrivate:NULL;
		BOOL fHasText  = (lpSBData->szText[0]!='\0');
		
		/* formatter */
		RECT rcIcon;
		RECT rcText;

	
		if (!g_CluiData.fDisableSkinEngine)
		{
			/* correct rect according to rcMargins */
			
			rcTemp.left	+= lpSBData->rcMargins.left;
			rcTemp.top += lpSBData->rcMargins.top;
			rcTemp.bottom -= lpSBData->rcMargins.bottom;
			rcTemp.right -= lpSBData->rcMargins.right;
		}
			
		rcIcon = rcTemp;
		rcText = rcTemp;
		

		/* reposition button items */
		if (hHasIcon && fHasText )
		{
			rcIcon.right=rcIcon.left+16; /* CXSM_ICON */
			rcText.left=rcIcon.right+2;
		}
		else if (hHasIcon)
		{
			rcIcon.left+=(rcIcon.right-rcIcon.left)/2-8;
			rcIcon.right=rcIcon.left+16;
		}
		
		{
			/*	Check sizes*/
			if (hHasIcon &&
				(rcIcon.right>rcTemp.right ||
				 rcIcon.bottom>rcTemp.bottom ||
				 rcIcon.left<rcTemp.left ||
				 rcIcon.top<rcTemp.top)) 		 hHasIcon=NULL;

			if (fHasText &&
				(rcText.right>rcTemp.right ||
				 rcText.bottom>rcTemp.bottom ||
				 rcText.left<rcTemp.left ||
				 rcText.top<rcTemp.top)) 		 fHasText=FALSE;			
		}

		if (hHasIcon)
		{
			/* center icon vertically */
			rcIcon.top+=(rcClient.bottom-rcClient.top)/2 - 8; /* CYSM_ICON/2 */
			rcIcon.bottom=rcIcon.top + 16; /* CYSM_ICON */
			/* draw it */
			SkinEngine_DrawIconEx(hdcMem, rcIcon.left+bPressed, rcIcon.top+bPressed, hHasIcon,
				16, 16, 0, NULL, DI_NORMAL);
		}
		if (fHasText)
		{
			SetBkMode(hdcMem,TRANSPARENT);
			if (lpSBData->nFontID>=0)
				CLCPaint_ChangeToFont(hdcMem,NULL,lpSBData->nFontID,NULL);

			SkinEngine_DrawText(hdcMem, lpSBData->szText, -1, &rcText, DT_CENTER | DT_VCENTER);
		}
		if (!pOffset)
			BitBlt(hdcPaint,0,0,width,height,hdcMem,0,0,SRCCOPY);
	}

    // better to use try/finally but looks like last one is Microsoft specific

		SelectObject(hdcMem,hOldFont);
	if (!pOffset)
	{	
		SelectObject(hdcMem,hbmOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
	}
	return;
}
